/* ======================================================================
 * S1D13781.cpp
 * Source file for S1D13781 Shield Hardware library
 * 
 * (C)SEIKO EPSON CORPORATION 2015. All rights reserved.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * ======================================================================
*/

/*
 * @author mikee47 7/12/2018
 *
 * Note that 16-bit transfers are big-endian, MSB first, however internal
 * byte-ordering of the controller is little-endian, as is the ESP8266.
 *
 */

#include "include/S1D13781/Driver.h"
#include "include/S1D13781/registers.h"
#include "init.h"
#include <Digital.h>
#include <Clock.h>
#include <Platform/Timers.h>

namespace S1D13781
{
// S1D13781 SPI Commands
const uint8_t SPIWRITE_8BIT = 0x80;
const uint8_t SPIREAD_8BIT = 0xC0;
const uint8_t SPIWRITE_16BIT = 0x88;
const uint8_t SPIREAD_16BIT = 0xC8;

#define debug_reg debug_none
//#define debug_reg debug_i

#define CACHE_ENABLE

/*
 * Mechanism for optimising cacheable register accesses.
 * We only cache certain register values.
 */
#define CACHED_REGISTERS(XX)                                                                                           \
	XX(REG12_PLL_1)                                                                                                    \
	XX(REG14_PLL_2)                                                                                                    \
	XX(REG16_INTCLK)                                                                                                   \
	XX(REG24_HDISP)                                                                                                    \
	XX(REG26_HNDP)                                                                                                     \
	XX(REG28_VDISP)                                                                                                    \
	XX(REG2A_VNDP)                                                                                                     \
	XX(REG40_MAIN_SET)                                                                                                 \
	XX(REG42_MAIN_SADDR_0)                                                                                             \
	XX(REG44_MAIN_SADDR_1)                                                                                             \
	XX(REG50_PIP_SET)                                                                                                  \
	XX(REG52_PIP_SADDR_0)                                                                                              \
	XX(REG54_PIP_SADDR_1)                                                                                              \
	XX(REG56_PIP_WIDTH)                                                                                                \
	XX(REG58_PIP_HEIGHT)                                                                                               \
	XX(REG5A_PIP_XSTART)                                                                                               \
	XX(REG5C_PIP_YSTART)                                                                                               \
	XX(REG60_PIP_EN)                                                                                                   \
	XX(REG62_ALPHA)                                                                                                    \
	XX(REG64_TRANS)                                                                                                    \
	XX(REG66_KEY_0)                                                                                                    \
	XX(REG68_KEY_1)

#define CACHED_REG_MIN REG12_PLL_1
#define CACHED_REG_MAX REG68_KEY_1

#define CACHED_REG_COUNT (((CACHED_REG_MAX - CACHED_REG_MIN) / 2) + 1)

#define CACHE_INDEX(reg) (((reg)-CACHED_REG_MIN) / 2)

#pragma pack(1)
// Register layout for BLT operations so we can use burst write
struct __attribute__((packed)) SeBltParam {
	uint16_t ctrl0;
	uint16_t ctrl1;
	uint16_t status;
	BltCmd cmd;
	uint32_t ssAddr;
	uint32_t dsAddr;
	uint16_t rectOffset;
	uint16_t width;
	uint16_t height;
	uint32_t bgColor;
	uint32_t fgColor;
};
#pragma pack()

Driver::Driver(HSPI::Controller& controller) : MemoryDevice(controller)
{
	cache = new uint16_t[CACHED_REG_COUNT];
}

Driver::~Driver()
{
	delete cache;
}

bool Driver::begin(HSPI::PinSet pinSet, uint8_t chipSelect)
{
	if(!MemoryDevice::begin(pinSet, chipSelect)) {
		return false;
	}

	setBitOrder(MSBFIRST);
	setClockMode(HSPI::ClockMode::mode0);
	setIoMode(HSPI::IoMode::SPIHD);
	return initRegs();
}

void Driver::updateTiming()
{
	uint16_t reg12 = regReadCached(REG12_PLL_1);
	timing.nCounter = (reg12 >> 10) & 0x0F;
	timing.mDivider = reg12 & 0x03FF;
	timing.lCounter = regReadCached(REG14_PLL_2) & 0x03FF;
	timing.pfdclk = S1D13781_CLKI * (timing.mDivider + 1);
	timing.mclk = (timing.lCounter + 1) * (timing.nCounter + 1) * timing.pfdclk;
	timing.pclk = timing.mclk / ((regReadCached(REG16_INTCLK) & 0x000F) + 1);

	uint16_t hdisp = getDisplayWidth();
	uint16_t hndp = regReadCached(REG26_HNDP);
	uint16_t vdisp = getDisplayHeight();
	uint16_t vndp = regReadCached(REG2A_VNDP);

	timing.frameInterval = 1000UL * (hdisp + hndp) * (vdisp + vndp) / (timing.pclk / 1000UL);
}

bool Driver::initRegs()
{
	// Initialise cache
	read(S1D13781_REG_BASE + CACHED_REG_MIN, cache, CACHED_REG_COUNT * 2);

	for(auto setting : displaySettings) {
		switch(setting.cmd) {
		case scRegWrite:
			regWrite(setting.regIndex, setting.value);
			break;

		case scRegWrite32:
			regWrite32(setting.regIndex, setting.value);
			break;

		case scDelayOff:
		case scDelayOn:
			delayMicroseconds(setting.value);
			break;

		case scPllWait: {
			// Proceed only if PLL is not bypassed
			if(regRead(REG10_PLL_0) & BIT(1)) {
				break;
			}

			ElapseTimer timer(setting.value);
			// Wait for PLL output to become stable, typically 2.5ms
			while(!(regRead(REG10_PLL_0) & BIT(15))) {
				if(timer.expired()) {
					// Timeout - failure
					return false;
				}
			}

			break;
		}

		default:
			return false; // Unknown command
		}
	}

	updateTiming();
	return true;
}

bool Driver::regWait(uint8_t regIndex, uint16_t regMask, uint16_t regValue, unsigned int timeoutMs)
{
	OneShotFastMs timer(timeoutMs);
	do {
		if((regRead(regIndex) & regMask) == regValue) {
			return true;
		}
	} while(!timer.expired());

	return false;
}

uint16_t Driver::regModify(uint8_t regIndex, uint16_t clearBits, uint16_t setBits)
{
	uint16_t regData = regReadCached(regIndex);
	regData &= ~clearBits;
	regData |= setBits;
	regWrite(regIndex, regData);
	return regData;
}

uint16_t Driver::regSetBits(uint8_t regIndex, uint16_t setBits)
{
	uint16_t regData = regReadCached(regIndex);
	regData |= setBits;
	regWrite(regIndex, regData);
	return regData;
}

uint16_t Driver::regClearBits(uint8_t regIndex, uint16_t clearBits)
{
	uint16_t regData = regReadCached(regIndex);
	regData &= ~clearBits;
	regWrite(regIndex, regData);
	return regData;
}

void Driver::prepareWrite(HSPI::Request& req, uint32_t address)
{
	wait(req);
	req.setCommand8(SPIWRITE_8BIT);
	req.setAddress24(address);
	req.dummyLen = 0;
}

void Driver::prepareRead(HSPI::Request& req, uint32_t address)
{
	wait(req);
	req.setCommand8(SPIREAD_8BIT);
	req.setAddress24(address);
	req.dummyLen = 8;
}

void Driver::regWrite(uint8_t regIndex, uint16_t regValue)
{
	debug_reg("regWrite(0x%02x, 0x%04x)", regIndex, regValue);
	writeWord(S1D13781_REG_BASE + regIndex, regValue, 2);

#ifdef CACHE_ENABLE
	switch(regIndex) {
#define XX(reg)                                                                                                        \
	case reg:                                                                                                          \
		cache[CACHE_INDEX(reg)] = regValue;                                                                            \
		break;
		CACHED_REGISTERS(XX)
#undef XX
	default:; // Not cached
	}
#endif
}

void Driver::regWrite32(uint8_t regIndex, uint32_t regValue)
{
	debug_reg("regWrite32(0x%02x, 0x%08x)", regIndex, regValue);
	writeWord(S1D13781_REG_BASE + regIndex, regValue, 4);

#ifdef CACHE_ENABLE
	switch(regIndex) {
#define XX(reg)                                                                                                        \
	case reg:                                                                                                          \
		cache[CACHE_INDEX(reg)] = regValue >> 16;                                                                      \
		cache[CACHE_INDEX(reg) + 1] = regValue & 0xFFFF;                                                               \
		break;
		CACHED_REGISTERS(XX)
#undef XX
	default:; // Not cached
	}
#endif
}

uint16_t Driver::regReadCached(uint8_t regIndex)
{
	uint16_t value;
#ifdef CACHE_ENABLE
	switch(regIndex) {
#define XX(reg)                                                                                                        \
	case reg:                                                                                                          \
		value = cache[CACHE_INDEX(reg)];                                                                               \
		break;
		CACHED_REGISTERS(XX)
#undef XX
	default:
#else
	{
#endif
		value = regRead(regIndex);
	}
	debug_reg("regReadCached(0x%02x) = 0x%04x", regIndex, value);
	return value;
}

uint32_t Driver::regReadCached32(uint8_t regIndex)
{
	uint32_t value;
#ifdef CACHE_ENABLE
	switch(regIndex) {
#define XX(reg)                                                                                                        \
	case reg:                                                                                                          \
		value = (cache[CACHE_INDEX(reg)] << 16) | cache[CACHE_INDEX(reg) + 1];                                         \
		break;
		CACHED_REGISTERS(XX)
#undef XX
	default:
#else
	{
#endif
		value = regRead32(regIndex);
	}
	debug_reg("regReadCached32(0x%02x) = 0x%08x", regIndex, value);
	return value;
}

/* =====================================================================
 * LCD/Main Layer Methods
 *
 * ===================================================================== 
*/

static __forceinline uint8_t regSelect(Window window, uint8_t regMain, uint8_t regPip)
{
	if(window == Window::main) {
		return regMain;
	} else if(window == Window::pip) {
		return regPip;
	} else {
		return 0;
	}
}

void Driver::setRotation(Window window, uint16_t rotationDegrees)
{
	auto reg = regSelect(window, REG40_MAIN_SET, REG50_PIP_SET);
	if(reg) {
		regModify(reg, 0x0018, (rotationDegrees / 90) << 3);
	}
}

uint16_t Driver::getRotation(Window window)
{
	auto reg = regSelect(window, REG40_MAIN_SET, REG50_PIP_SET);
	return reg ? (regReadCached(reg) >> 3) * 90 : 0;
}

void Driver::setColorDepth(Window window, ImageDataFormat colorDepth)
{
	auto reg = regSelect(window, REG40_MAIN_SET, REG50_PIP_SET);
	if(reg) {
		regModify(reg, 0x0007, colorDepth);
	}
}

ImageDataFormat Driver::getColorDepth(Window window)
{
	auto reg = regSelect(window, REG40_MAIN_SET, REG50_PIP_SET);
	return ImageDataFormat(regReadCached(reg) & 0x0007);
}

void Driver::setStartAddress(Window window, uint32_t lcdStartAddress)
{
	auto reg = regSelect(window, REG42_MAIN_SADDR_0, REG52_PIP_SADDR_0);
	if(reg) {
		regWrite32(reg, lcdStartAddress);
	}
}

uint32_t Driver::getStartAddress(Window window)
{
	auto reg = regSelect(window, REG42_MAIN_SADDR_0, REG52_PIP_SADDR_0);
	return reg ? regReadCached32(reg) : 0xFFFFFFFF;
}

uint32_t Driver::getAddress(Window window, uint16_t x, uint16_t y)
{
	uint32_t addr = getStartAddress(window);
	if(int(addr) >= 0) {
		addr += (y * getStride(window)) + (x * getBytesPerPixel(window));
	}
	return addr;
}

uint16_t Driver::getDisplayWidth()
{
	return regReadCached(REG24_HDISP) * 8;
}

uint16_t Driver::getDisplayHeight()
{
	return regReadCached(REG28_VDISP);
}

void Driver::setWidth(Window window, uint16_t width)
{
	if(window == Window::pip) {
		regWrite(REG56_PIP_WIDTH, width);
	} else if(window == Window::main) {
		regWrite(REG24_HDISP, width / 8);
	}
}

uint16_t Driver::getWidth(Window window)
{
	uint16_t width = 0;
	if(window == Window::pip) {
		width = regReadCached(REG56_PIP_WIDTH);
	} else if(window == Window::main) {
		auto rotation = getRotation(window);
		if(rotation == 90 || rotation == 270) {
			width = getDisplayHeight();
		} else {
			width = getDisplayWidth();
		}
	}
	return width;
}

void Driver::setHeight(Window window, uint16_t height)
{
	auto reg = regSelect(window, REG28_VDISP, REG58_PIP_HEIGHT);
	if(reg) {
		regWrite(reg, height);
	}
}

uint16_t Driver::getHeight(Window window)
{
	uint16_t height = 0;
	if(window == Window::pip) {
		height = regReadCached(REG58_PIP_HEIGHT);
	} else if(window == Window::main) {
		auto rotation = getRotation(window);
		if(rotation == 90 || rotation == 270) {
			height = getDisplayWidth();
		} else {
			height = getDisplayHeight();
		}
	}
	return height;
}

SeSize Driver::getWindowSize(Window window)
{
	SeSize size;
	if(window == Window::pip) {
		size.val = regReadCached32(REG56_PIP_WIDTH);
	} else if(window == Window::main) {
		size.width = getDisplayWidth();
		size.height = getDisplayHeight();
	} else {
		return size;
	}

	auto rotation = getRotation(window);
	if(rotation == 90 || rotation == 270) {
		seSwap(size.width, size.height);
	}

	return size;
}

uint16_t Driver::getStride(Window window)
{
	return getWidth(window) * getBytesPerPixel(window);
}

/* =====================================================================
 * PIP Layer Methods
 *
 * ===================================================================== 
*/

void Driver::pipSetDisplayMode(PipEffect newEffect)
{
	//check whether the effect needs to finish first
	PipEffect currentEffect = pipGetDisplayMode();
	switch(currentEffect) {
	case PipEffect::fadeIn:
	case PipEffect::fadeOut:
	case PipEffect::continuous:
		pipWaitForFade(250);
		break;
	default:;
	}

	regModify(REG60_PIP_EN, 0x0007, unsigned(newEffect) & 0x0007);
}

PipEffect Driver::pipGetDisplayMode()
{
	return PipEffect(regReadCached(REG60_PIP_EN) & 0x0007);
}

bool Driver::pipIsOrthogonal()
{
	return getRotation(Window::main) == getRotation(Window::pip);
}

/** @brief Rotate a point within a window based on its rotation setting */
static void rotatePos(SePos& pos, const SeSize size, uint16_t degrees)
{
	return;

	switch(degrees) {
	case 90:
		pos.x -= size.height;
		break;
	case 180:
		pos.x += size.width;
		pos.y += size.height;
		break;
	case 270:
		pos.y += size.height;
		break;
	case 0:
	case 360:
	default:; // Leave as-is
	}
}

void Driver::pipSetPosition(SePos pos)
{
	SePos p1 = pos;
	rotatePos(pos, getWindowSize(Window::pip), getRotation(Window::pip));
	debug_i("lcdc.pipSetPosition(%u, %u) -> (%u, %u)", p1.x, p1.y, pos.x, pos.y);
	regWrite32(REG5A_PIP_XSTART, pos.val);
}

SePos Driver::pipGetPosition()
{
	SePos pos;
	pos.val = regReadCached32(REG5A_PIP_XSTART);
	SePos p1 = pos;
	rotatePos(pos, getWindowSize(Window::pip), 360 - getRotation(Window::pip));
	debug_i("lcdc.pipGetPosition(%u, %u) -> (%u, %u)", p1.x, p1.y, pos.x, pos.y);
	return pos;
}

void Driver::pipSetFadeRate(uint8_t fadeRate)
{
	regModify(REG60_PIP_EN, 0xFE00, (fadeRate - 1) << 9);
}

uint8_t Driver::pipGetFadeRate()
{
	return (regReadCached(REG60_PIP_EN) >> 9) + 1;
}

bool Driver::pipWaitForFade(uint16_t maxTime)
{
	return regWaitForLow(REG60_PIP_EN, BIT(3), maxTime);
}

void Driver::pipSetAlphaBlendStep(uint8_t step)
{
	uint8_t bits;
	if(step <= 1)
		bits = 0x00;
	else if(step <= 2)
		bits = 0x01;
	else if(step <= 4)
		bits = 0x02;
	else
		bits = 0x03;
	regModify(REG62_ALPHA, 0x0300, bits << 8);
}

uint16_t Driver::pipGetAlphaBlendStep()
{
	return 1 << (regReadCached(REG62_ALPHA) >> 8);
}

void Driver::pipSetAphaBlendRatio(uint8_t ratio)
{
	// Calculate the nearest register value (alphaRegValue) to the Alpha %
	uint8_t alphaRegValue = (ratio * 64 / 100) & 0x007F;
	regModify(REG62_ALPHA, 0x007F, alphaRegValue);
}

uint8_t Driver::pipGetAlphaBlendRatio()
{
	return (regReadCached(REG62_ALPHA) & 0x007F) * 100 / 64;
}

void Driver::pipEnableTransparency(bool enable)
{
	if(enable) {
		regSetBits(REG64_TRANS, 0x0001);
	} else {
		regClearBits(REG64_TRANS, 0x0001);
	}
}

bool Driver::pipGetTransparency()
{
	return regReadCached(REG64_TRANS) ? true : false;
}

void Driver::pipSetTransColor(uint32_t xrgbColor)
{
	regWrite32(REG66_KEY_0, xrgbColor);
}

SeColor Driver::pipGetTransColor()
{
	return regReadCached32(REG66_KEY_0);
}

void Driver::pipSetupWindow(uint16_t xPos, uint16_t yPos, uint16_t pipWidth, uint16_t pipHeight)
{
	//write the width and height of the PIP window
	SeSize pipSize = {pipWidth, pipHeight};
	regWrite32(REG56_PIP_WIDTH, pipSize.val);

	//set the x and y position according to the current PIP rotation
	SePos pos(xPos, yPos);
	rotatePos(pos, pipSize, getRotation(Window::pip));

	regWrite32(REG5A_PIP_XSTART, pos.val);
}

/* =====================================================================
 * LUT Methods
 *
 * ===================================================================== 
*/

static uint32_t getLutAddress(Window window, uint16_t index)
{
	uint32_t addr = (window == Window::pip) ? S1D13781_LUT2_BASE : S1D13781_LUT1_BASE;
	return addr + (index * 4);
}

void Driver::setLutEntry(Window window, uint16_t index, SeColor xrgbData)
{
	writeWord(getLutAddress(window, index), xrgbData.value, 4);
}

uint32_t Driver::getLutEntry(Window window, uint16_t index)
{
	return readWord(getLutAddress(window, index), 4);
}

void Driver::setLut(Window window, uint16_t startIndex, const SeColor* rgbData, uint16_t count)
{
	write(getLutAddress(window, startIndex), rgbData, count * 4);
}

void Driver::getLut(Window window, uint16_t startIndex, SeColor* rgbData, uint16_t count)
{
	read(getLutAddress(window, startIndex), rgbData, count * 4);
}

void Driver::setLutDefault(Window window)
{
	unsigned int lutSize;
	unsigned int temp;
	unsigned int lutArray;
	uint8_t red, green, blue;

	unsigned int i; //loop var

	//determine the format of the selected window
	auto windowFormat = getColorDepth(window);

	//setup the LUT entry values correctly
	/*
	 * @todo Use temporary buffer to reduce number of SPI transactions required
	 * Not urgent as we wouldn't expect this method to be called a lot.
	 */
	if(windowFormat == format_RGB_565 || windowFormat == format_RGB_565LUT) { // 16bpp
		lutSize = 64;
		for(i = 0; i < lutSize; i++) {
			temp = (uint8_t)(i << 3);
			red = (uint8_t)(temp | (temp >> 5));
			temp = (uint8_t)(i << 2);
			green = (uint8_t)(temp | (temp >> 6));
			temp = (uint8_t)(i << 3);
			blue = (uint8_t)(temp | (temp >> 5));
			lutArray = blue | (green << 8) | (red << 16);
			setLutEntry(window, i, lutArray);
		}
	} else if(windowFormat == format_RGB_332LUT) { // 8bpp
		lutSize = 256;
		for(i = 0; i < lutSize; i++) {
			temp = (uint8_t)((i & 0xE0) << 0);
			red = (uint8_t)(temp | (temp >> 3) | (temp >> 6));
			temp = (uint8_t)((i & 0x1C) << 3);
			green = (uint8_t)(temp | (temp >> 3) | (temp >> 6));
			temp = (uint8_t)((i & 0x03) << 6);
			blue = (uint8_t)(temp | (temp >> 2) | (temp >> 4) | (temp >> 6));
			lutArray = blue | (green << 8) | (red << 16);
			setLutEntry(window, i, lutArray);
		}
	} else { // 24bpp
		lutSize = 256;
		for(i = 0; i < lutSize; i++) {
			// Red component = Green component = Blue component
			lutArray = i | i << 8 | i << 16;
			setLutEntry(window, i, lutArray);
		}
	}
}

#define bltExecute(blt)                                                                                                \
	{                                                                                                                  \
		regWaitForLow(REG84_BLT_STATUS, BIT(0), 1000);                                                                 \
		write(S1D13781_REG_BASE + REG80_BLT_CTRL_0, &blt, sizeof(blt));                                                \
		regWrite(REG80_BLT_CTRL_0, 0x0001);                                                                            \
	}

bool Driver::bltSolidFill(Window window, SePos pos, SeSize size, SeColor color)
{
	uint16_t bytesPerPixel = getBytesPerPixel(window);
	if(bytesPerPixel == 0) {
		return false;
	}

	SeBltParam blt{
		.ctrl0 = 0x0080,
		.ctrl1 = uint16_t((bytesPerPixel - 1) << 2),
		.status = 0,
		.cmd = BltCmd::solidFill,
		.ssAddr = 0,
		.dsAddr = getAddress(window, pos),
		.rectOffset = getWidth(window),
		.width = size.width,
		.height = size.height,
		.bgColor = 0,
		.fgColor = lookupColor(window, color),
	};

	bltExecute(blt);

	return true;
}

bool Driver::bltMoveExpand(Window window, uint32_t srcAddr, SePos dstPos, SeSize dstSize, SeColor fgColor,
						   SeColor bgColor)
{
	uint8_t bytesPerPixel = getBytesPerPixel(window);
	if(bytesPerPixel == 0) {
		return false;
	}

	SeBltParam blt{
		.ctrl0 = 0x0080,
		.ctrl1 = uint16_t(((bytesPerPixel - 1) << 2) | 0x0001),
		.status = 0,
		.cmd = BltCmd::moveExpand,
		.ssAddr = srcAddr,
		.dsAddr = getAddress(window, dstPos),
		.rectOffset = getWidth(window),
		.width = dstSize.width,
		.height = dstSize.height,
		.bgColor = lookupColor(window, bgColor),
		.fgColor = lookupColor(window, fgColor),
	};

	bltExecute(blt);

	return true;
}

bool Driver::bltMove(Window window, BltCmd cmd, SePos srcPos, SePos dstPos, SeSize size)
{
	uint8_t bytesPerPixel = getBytesPerPixel(window);
	if(bytesPerPixel == 0) {
		return false;
	}

	SeBltParam blt{
		.ctrl0 = 0x0080,
		.ctrl1 = uint16_t(((bytesPerPixel - 1) << 2) | 0x0001),
		.status = 0,
		.cmd = cmd,
		.ssAddr = getAddress(window, srcPos),
		.dsAddr = getAddress(window, dstPos),
		.rectOffset = getWidth(window),
		.width = size.width,
		.height = size.height,
	};

	bltExecute(blt);

	return true;
}

} // namespace S1D13781
