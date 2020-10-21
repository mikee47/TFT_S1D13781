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

#include <S1D13781/driver.h>
#include <S1D13781/registers.h>
#include "init.h"
#include <Digital.h>
#include <Clock.h>
#include <Platform/Timers.h>

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
	uint16_t cmd; // BltCommand
	uint32_t ssAddr;
	uint32_t dsAddr;
	uint16_t rectOffset;
	uint16_t width;
	uint16_t height;
	uint32_t bgColor;
	uint32_t fgColor;
};
#pragma pack()

S1D13781::S1D13781(HSPI::Device& dev) : spidev(dev)
{
	cache = new uint16_t[CACHED_REG_COUNT];
}

S1D13781::~S1D13781()
{
	delete cache;
}

bool S1D13781::begin()
{
	spidev.setBitOrder(MSBFIRST);
	spidev.setClockMode(HSPI::ClockMode::mode0);
	spidev.setIoMode(HSPI::IoMode::SPIHD);
	return initRegs();
}

void S1D13781::updateTiming()
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

bool S1D13781::initRegs()
{
	// Initialise cache
	memBurstReadBytes(S1D13781_REG_BASE + CACHED_REG_MIN, cache, CACHED_REG_COUNT * 2);

	for(unsigned i = 0; i < ARRAY_SIZE(displaySettings); ++i) {
		SeSetting setting;
		memcpy_P(&setting, &displaySettings[i], sizeof(setting));

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

bool S1D13781::regWait(uint8_t regIndex, uint16_t regMask, uint16_t regValue, unsigned int timeoutMs)
{
	OneShotFastMs timer(timeoutMs);
	do {
		if((regRead(regIndex) & regMask) == regValue) {
			//			debug_i("regWait, %u us", timer.elapsed());
			return true;
		}
	} while(!timer.expired());

	debug_i("regWait, timeout");
	return false;
}

uint16_t S1D13781::regModify(uint8_t regIndex, uint16_t clearBits, uint16_t setBits)
{
	uint16_t regData = regReadCached(regIndex);
	regData &= ~clearBits;
	regData |= setBits;
	regWrite(regIndex, regData);
	return regData;
}

uint16_t S1D13781::regSetBits(uint8_t regIndex, uint16_t setBits)
{
	uint16_t regData = regReadCached(regIndex);
	regData |= setBits;
	regWrite(regIndex, regData);
	return regData;
}

uint16_t S1D13781::regClearBits(uint8_t regIndex, uint16_t clearBits)
{
	uint16_t regData = regReadCached(regIndex);
	regData &= ~clearBits;
	regWrite(regIndex, regData);
	return regData;
}

void S1D13781::memWriteWord32(uint32_t memAddress, uint32_t memValue, uint8_t valueLen)
{
	reqWr.prepare();
	reqWr.setCommand8(SPIWRITE_8BIT);
	reqWr.setAddress24(memAddress);
	reqWr.dummyLen = 0;
	reqWr.out.set32(memValue, valueLen);
	reqWr.in.clear();
	reqWr.async = true;
	reqWr.callback = nullptr;
	spidev.execute(reqWr);
}

uint32_t S1D13781::memReadWord32(uint32_t memAddress, uint8_t valueLen)
{
	reqRd.prepare();
	reqRd.setCommand8(SPIREAD_8BIT);
	reqRd.setAddress24(memAddress);
	reqRd.dummyLen = 8;
	reqRd.out.clear();
	reqRd.in.set32(0, valueLen);
	reqRd.async = false;
	reqRd.callback = nullptr;
	spidev.execute(reqRd);

	//	debug_i("memReadWord(0x%06X) = 0x%08X", memAddress, reqRd.in.data32);

	/*
	 * 8-bit
	 * 16 1 0
	 * 32 1 0 3 2
	 */
	return reqRd.in.data32;
}

void S1D13781::memBurstWriteBytes(uint32_t memAddress, const void* memValues, uint16_t count, HSPI::Callback callback,
								  void* param)
{
	//	debug_i("memBurstWriteBytes(0x%08x, 0x%08x, %u", memAddress, memValues, count);

	/*
	if(memAddress >= S1D13781_REG_BASE) {
		debug_i("Bad address: 0x%08x", memAddress);
		return; // Invalid address
	}
	if(memAddress + count > S1D13781_REG_BASE) {
		debug_i("Bad address range: 0x%08x, %u", memAddress, count);
		// Reduce write size to avoid overwriting register space
		count = S1D13781_REG_BASE - memAddress;
	}

*/
	reqWr.prepare();
	reqWr.setCommand8(SPIWRITE_8BIT);
	reqWr.setAddress24(memAddress);
	reqWr.dummyLen = 0;
	reqWr.out.set(memValues, count);
	reqWr.in.clear();
	reqWr.async = (callback != nullptr);
	reqWr.callback = callback;
	reqWr.param = param;
	spidev.execute(reqWr);
}

void S1D13781::memBurstReadBytes(uint32_t memAddress, void* memValues, uint16_t count, HSPI::Callback callback,
								 void* param)
{
	reqRd.prepare();
	reqRd.setCommand8(SPIREAD_8BIT);
	reqRd.setAddress24(memAddress);
	reqRd.dummyLen = 8;
	reqRd.out.clear();
	reqRd.in.set(memValues, count);
	reqRd.async = (callback != nullptr);
	reqRd.callback = callback;
	reqRd.param = param;
	spidev.execute(reqRd);
}

void S1D13781::regWrite(uint8_t regIndex, uint16_t regValue)
{
	debug_reg("regWrite(0x%02x, 0x%04x)", regIndex, regValue);
	memWriteWord(S1D13781_REG_BASE + regIndex, regValue);

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

void S1D13781::regWrite32(uint8_t regIndex, uint32_t regValue)
{
	debug_reg("regWrite32(0x%02x, 0x%08x)", regIndex, regValue);
	memWriteWord32(S1D13781_REG_BASE + regIndex, regValue);

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

uint16_t S1D13781::regReadCached(uint8_t regIndex)
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

uint32_t S1D13781::regReadCached32(uint8_t regIndex)
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

static __forceinline uint8_t regSelect(WindowDestination window, uint8_t regMain, uint8_t regPip)
{
	if(window == window_Main) {
		return regMain;
	} else if(window == window_Pip) {
		return regPip;
	} else {
		return 0;
	}
}

void S1D13781::setRotation(WindowDestination window, uint16_t rotationDegrees)
{
	auto reg = regSelect(window, REG40_MAIN_SET, REG50_PIP_SET);
	if(reg) {
		regModify(reg, 0x0018, (rotationDegrees / 90) << 3);
	}
}

uint16_t S1D13781::getRotation(WindowDestination window)
{
	auto reg = regSelect(window, REG40_MAIN_SET, REG50_PIP_SET);
	return reg ? (regReadCached(reg) >> 3) * 90 : 0;
}

void S1D13781::setColorDepth(WindowDestination window, ImageDataFormat colorDepth)
{
	auto reg = regSelect(window, REG40_MAIN_SET, REG50_PIP_SET);
	if(reg) {
		regModify(reg, 0x0007, colorDepth);
	}
}

ImageDataFormat S1D13781::getColorDepth(WindowDestination window)
{
	auto reg = regSelect(window, REG40_MAIN_SET, REG50_PIP_SET);
	return ImageDataFormat(regReadCached(reg) & 0x0007);
}

void S1D13781::setStartAddress(WindowDestination window, uint32_t lcdStartAddress)
{
	auto reg = regSelect(window, REG42_MAIN_SADDR_0, REG52_PIP_SADDR_0);
	if(reg) {
		regWrite32(reg, lcdStartAddress);
	}
}

uint32_t S1D13781::getStartAddress(WindowDestination window)
{
	auto reg = regSelect(window, REG42_MAIN_SADDR_0, REG52_PIP_SADDR_0);
	return reg ? regReadCached32(reg) : 0xFFFFFFFF;
}

uint32_t S1D13781::getAddress(WindowDestination window, uint16_t x, uint16_t y)
{
	uint32_t addr = getStartAddress(window);
	if(int(addr) >= 0) {
		addr += (y * getStride(window)) + (x * getBytesPerPixel(window));
	}
	return addr;
}

uint16_t S1D13781::getDisplayWidth()
{
	return regReadCached(REG24_HDISP) * 8;
}

uint16_t S1D13781::getDisplayHeight()
{
	return regReadCached(REG28_VDISP);
}

void S1D13781::setWidth(WindowDestination window, uint16_t width)
{
	if(window == window_Pip) {
		regWrite(REG56_PIP_WIDTH, width);
	} else if(window == window_Main) {
		regWrite(REG24_HDISP, width / 8);
	}
}

uint16_t S1D13781::getWidth(WindowDestination window)
{
	uint16_t width = 0;
	if(window == window_Pip) {
		width = regReadCached(REG56_PIP_WIDTH);
	} else if(window == window_Main) {
		auto rotation = getRotation(window);
		if(rotation == 90 || rotation == 270) {
			width = getDisplayHeight();
		} else {
			width = getDisplayWidth();
		}
	}
	return width;
}

void S1D13781::setHeight(WindowDestination window, uint16_t height)
{
	auto reg = regSelect(window, REG28_VDISP, REG58_PIP_HEIGHT);
	if(reg) {
		regWrite(reg, height);
	}
}

uint16_t S1D13781::getHeight(WindowDestination window)
{
	uint16_t height = 0;
	if(window == window_Pip) {
		height = regReadCached(REG58_PIP_HEIGHT);
	} else if(window == window_Main) {
		auto rotation = getRotation(window);
		if(rotation == 90 || rotation == 270) {
			height = getDisplayWidth();
		} else {
			height = getDisplayHeight();
		}
	}
	return height;
}

SeSize S1D13781::getWindowSize(WindowDestination window)
{
	SeSize size;
	if(window == window_Pip) {
		size.val = regReadCached32(REG56_PIP_WIDTH);
	} else if(window == window_Main) {
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

uint16_t S1D13781::getStride(WindowDestination window)
{
	return getWidth(window) * getBytesPerPixel(window);
}

/* =====================================================================
 * PIP Layer Methods
 *
 * ===================================================================== 
*/

void S1D13781::pipSetDisplayMode(PipEffect newEffect)
{
	//check whether the effect needs to finish first
	PipEffect currentEffect = pipGetDisplayMode();
	switch(currentEffect) {
	case pipFadeIn:
	case pipFadeOut:
	case pipContinous:
		pipWaitForFade(250);
		break;
	default:;
	}

	regModify(REG60_PIP_EN, 0x0007, newEffect & 0x0007);
}

PipEffect S1D13781::pipGetDisplayMode()
{
	return PipEffect(regReadCached(REG60_PIP_EN) & 0x0007);
}

bool S1D13781::pipIsOrthogonal()
{
	return getRotation(window_Main) == getRotation(window_Pip);
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

void S1D13781::pipSetPosition(SePos pos)
{
	SePos p1 = pos;
	rotatePos(pos, getWindowSize(window_Pip), getRotation(window_Pip));
	debug_i("lcdc.pipSetPosition(%u, %u) -> (%u, %u)", p1.x, p1.y, pos.x, pos.y);
	regWrite32(REG5A_PIP_XSTART, pos.val);
}

SePos S1D13781::pipGetPosition()
{
	SePos pos;
	pos.val = regReadCached32(REG5A_PIP_XSTART);
	SePos p1 = pos;
	rotatePos(pos, getWindowSize(window_Pip), 360 - getRotation(window_Pip));
	debug_i("lcdc.pipGetPosition(%u, %u) -> (%u, %u)", p1.x, p1.y, pos.x, pos.y);
	return pos;
}

void S1D13781::pipSetFadeRate(uint8_t fadeRate)
{
	regModify(REG60_PIP_EN, 0xFE00, (fadeRate - 1) << 9);
}

uint8_t S1D13781::pipGetFadeRate()
{
	return (regReadCached(REG60_PIP_EN) >> 9) + 1;
}

bool S1D13781::pipWaitForFade(uint16_t maxTime)
{
	return regWaitForLow(REG60_PIP_EN, BIT(3), maxTime);
}

void S1D13781::pipSetAlphaBlendStep(uint8_t step)
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

uint16_t S1D13781::pipGetAlphaBlendStep()
{
	return 1 << (regReadCached(REG62_ALPHA) >> 8);
}

void S1D13781::pipSetAphaBlendRatio(uint8_t ratio)
{
	// Calculate the nearest register value (alphaRegValue) to the Alpha %
	uint8_t alphaRegValue = (ratio * 64 / 100) & 0x007F;
	regModify(REG62_ALPHA, 0x007F, alphaRegValue);
}

uint8_t S1D13781::pipGetAlphaBlendRatio()
{
	return (regReadCached(REG62_ALPHA) & 0x007F) * 100 / 64;
}

void S1D13781::pipEnableTransparency(bool enable)
{
	if(enable) {
		regSetBits(REG64_TRANS, 0x0001);
	} else {
		regClearBits(REG64_TRANS, 0x0001);
	}
}

bool S1D13781::pipGetTransparency()
{
	return regReadCached(REG64_TRANS) ? true : false;
}

void S1D13781::pipSetTransColor(uint32_t xrgbColor)
{
	regWrite32(REG66_KEY_0, xrgbColor);
}

SeColor S1D13781::pipGetTransColor()
{
	return regReadCached32(REG66_KEY_0);
}

void S1D13781::pipSetupWindow(uint16_t xPos, uint16_t yPos, uint16_t pipWidth, uint16_t pipHeight)
{
	//write the width and height of the PIP window
	SeSize pipSize = {pipWidth, pipHeight};
	regWrite32(REG56_PIP_WIDTH, pipSize.val);

	//set the x and y position according to the current PIP rotation
	SePos pos(xPos, yPos);
	rotatePos(pos, pipSize, getRotation(window_Pip));

	regWrite32(REG5A_PIP_XSTART, pos.val);
}

/* =====================================================================
 * LUT Methods
 *
 * ===================================================================== 
*/

static uint32_t getLutAddress(WindowDestination window, uint16_t index)
{
	uint32_t addr = (window == window_Pip) ? S1D13781_LUT2_BASE : S1D13781_LUT1_BASE;
	return addr + (index * 4);
}

void S1D13781::setLutEntry(WindowDestination window, uint16_t index, SeColor xrgbData)
{
	memWriteWord32(getLutAddress(window, index), xrgbData);
}

uint32_t S1D13781::getLutEntry(WindowDestination window, uint16_t index)
{
	return memReadWord32(getLutAddress(window, index));
}

void S1D13781::setLut(WindowDestination window, uint16_t startIndex, const SeColor* rgbData, uint16_t count)
{
	memBurstWriteBytes(getLutAddress(window, startIndex), rgbData, count * 4);
}

void S1D13781::getLut(WindowDestination window, uint16_t startIndex, SeColor* rgbData, uint16_t count)
{
	memBurstReadBytes(getLutAddress(window, startIndex), rgbData, count * 4);
}

void S1D13781::setLutDefault(WindowDestination window)
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
		memBurstWriteBytes(S1D13781_REG_BASE + REG80_BLT_CTRL_0, &blt, sizeof(blt));                                   \
		regWrite(REG80_BLT_CTRL_0, 0x0001);                                                                            \
	}

bool S1D13781::bltSolidFill(WindowDestination window, SePos pos, SeSize size, SeColor color)
{
	uint16_t bytesPerPixel = getBytesPerPixel(window);
	if(bytesPerPixel == 0) {
		return false;
	}

	SeBltParam blt{
		.ctrl0 = 0x0080,
		.ctrl1 = uint16_t((bytesPerPixel - 1) << 2),
		.status = 0,
		.cmd = bltcmd_SolidFill,
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

bool S1D13781::bltMoveExpand(WindowDestination window, uint32_t srcAddr, SePos dstPos, SeSize dstSize, SeColor fgColor,
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
		.cmd = bltcmd_MoveExpand,
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

bool S1D13781::bltMove(WindowDestination window, BltCommand cmd, SePos srcPos, SePos dstPos, SeSize size)
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
