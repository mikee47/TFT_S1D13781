/* ======================================================================
 * driver.h
 * Header file for S1D13781 Shield Hardware library
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
#pragma once

#include <HSPI/MemoryDevice.h>
#include "SeColor.h"

const uint32_t S1D13781_LUT1_BASE = 0x060000;
const uint32_t S1D13781_LUT2_BASE = 0x060400;
const uint32_t S1D13781_REG_BASE = 0x060800;

#define seSwap(A, B)                                                                                                   \
	{                                                                                                                  \
		auto T = (A);                                                                                                  \
		(A) = (B);                                                                                                     \
		(B) = T;                                                                                                       \
	}

/** @brief Two consective registers contain an (x, y) co-ordinate and may be handled in a single transfer */
union SePos {
	struct {
		uint16_t x;
		uint16_t y;
	};
	uint32_t val;

	SePos()
	{
		x = 0;
		y = 0;
	}

	SePos(uint16_t x, uint16_t y)
	{
		set(x, y);
	}

	void set(uint16_t x, uint16_t y)
	{
		this->x = x;
		this->y = y;
	}
};

union SeSize {
	struct {
		uint16_t width;
		uint16_t height;
	};
	uint32_t val;

	SeSize()
	{
		width = 0;
		height = 0;
	}

	SeSize(uint16_t w, uint16_t h)
	{
		set(w, h);
	}

	void set(uint16_t w, uint16_t h)
	{
		width = w;
		height = h;
	}
};

namespace S1D13781
{
/** @brief Defines various clock parameters */
struct S1DTiming {
	uint8_t nCounter;
	uint16_t mDivider;
	uint16_t lCounter;
	uint32_t pfdclk;
	uint32_t mclk;
	uint32_t pclk;
	uint32_t frameInterval; // Time in microseconds between frames, fps = 1e6 / interval
};

//possible destination windows
enum class Window {
	main,
	pip,
	invalid,
};

//possible modes for the PIP window
enum class PipEffect {
	/** Stops the PIP from being displayed immediately */
	disabld,
	/** Causes the PIP to be displayed immediately. The PIP will be displayed with the
	 * currently set alpha blending mode. If the alpha blending ratio is changed while
	 * the PIP is displayed the effect will take place on the next frame. */
	normal,
	/** PIP layer toggles between the set alpha blend mode and no PIP. */
	blink1,
	/** PIP layer toggles between normal and invert. Alpha blend ratio remains constant. */
	blink2,
	/** Causes the PIP to fade from the current alpha blend value to 0x0000 (blank) */
	fadeOut,
	/** Causes the PIP layer to fade from 0x0000 to the set alpha blend ratio */
	fadeIn,
	/* Cycles between alpha blend 0x0000 and the current set alpha blend value. */
	continuous,
};

enum class BltCmd : uint16_t {
	movePositive, ///< Copy rectangular area in VRAM, new address > old address
	moveNegative, ///< Copy rectangular area in VRAM, new address < old address
	solidFill,	///< Fill rectangular area
	reserved03,
	moveExpand, ///< Copy area bits expanding to current foreground/background colours
};

//base class with hardware accessor functions
class Driver : public HSPI::MemoryDevice
{
public:
	Driver(HSPI::Controller& controller);
	~Driver();

	/** @brief This method should be run once to setup the SPI interface used by
	 * 	the S1D13781 evaluation board and configure the registers.
	 * 	Note that bus speed must already be configured.
	 */
	bool begin(HSPI::PinSet pinSet, uint8_t chipSelect);

	// Direct register and memory access

	/** @brief Write a uint16_t to a S1D13781 register.
	 *  @note
	 *
	 * The regIndex argument should use the predefined register names found
	 * in the S1D13781_registers.h file, so as to prevent reading
	 * mis-aligned or invalid (non-existent) registers.
	 *
	 * Note:
	 * - NOT all registers can be written at any given time. Some
	 *   registers cannot be written if the S1D13781 is in NMM (see
	 *   specification for more details).
	 *
	 * param	regIndex	Register index to write.
	 *
	 * param	regValue	Data to write to the register.
	 *
	 */
	void regWrite(uint8_t regIndex, uint16_t regValue);
	void regWrite32(uint8_t regIndex, uint32_t regValue);

	/** @brief Read a uint16_t (16-bit unsigned int) from a S1D13781 register.
	 * 	@note
	 * The regIndex argument should use the predefined register names found
	 * in the S1D13781_registers.h file, so as to prevent reading
	 * mis-aligned or invalid (non-existent) registers.
	 *
	 * Note:
	 * - NOT all registers can be read at any given time. Some
	 *   registers cannot be read if the S1D13781 is in PSM0 (see
	 *   specification for more details).
	 *
	 * param	regIndex	Index (offset) of the register to read.
	 *
	 * return:
	 * - Returns the contents of the specified S1D13781 register.
	 *
	 */
	uint16_t regRead(uint8_t regIndex)
	{
		return readWord(S1D13781_REG_BASE + regIndex, 2);
	}

	uint32_t regRead32(uint8_t regIndex)
	{
		return readWord(S1D13781_REG_BASE + regIndex, 4);
	}

	uint16_t regReadCached(uint8_t regIndex);
	uint32_t regReadCached32(uint8_t regIndex);

	/** @brief Modify the contents of a S1D13781 register using bitmasks.
	 * @note Performed using a read-modify-write cycle via the cache
	 *
	 * param	regIndex	Register index to modify.
	 *
	 * param	clearBits	Bitmask of register bits to be cleared (set to '0').
	 * 						Any bit positions set to '1' in this mask will be
	 * 						cleared in the register value.
	 *
	 * param	setBits		Bitmask of register bits to set ( to '1').
	 *
	 * return:
	 * - The return value is the value that was written to the register.
	 *
	 */
	uint16_t regModify(uint8_t regIndex, uint16_t clearBits, uint16_t setBits);

	/** @brief Set specific bits in a S1D13781 register using a bitmask.
	 * 	@note Performed using a read-modify-write cycle via the cache
	 *
	 * param	regIndex	Register index to clear bits in.
	 *
	 * param	setBits		Bitmask of bits to be set to '1' in the register.
	 *
	 * return:
	 * - Returns the updated contents of the register.
	 *
	 */
	uint16_t regSetBits(uint8_t regIndex, uint16_t setBits);

	/** @brief Clear specific bits in a S1D13781 register using a bitmask.
	 * 	@note Performed using a read-modify-write cycle via the cache
	 *
	 * param	regIndex	Register index to clear bits in.
	 *
	 * param	clearBits	Bitmask of bits to be set to '0' in the register.
	 * 						Any bit positions set to '1' in this mask will
	 * 						be cleared in the register.
	 *
	 * return:
	 * - Returns the updated contents of the register.
	 */
	uint16_t regClearBits(uint8_t regIndex, uint16_t clearBits);

	/** @brief Set the rotation of the main layer.
	 *
	 * param	rotationDegrees		Counter-clockwise rotation of the main layer in degrees.
	 * 								Acceptable values (0, 90, 180, 270)
	 *
	 */
	void setRotation(Window window, uint16_t rotationDegress);

	/** @brief Get the current rotation of the main layer.
	 *
	 * return:
	 * - current counter-clockwise rotation in degrees (0, 90, 180, 270)
	 *
	 */
	uint16_t getRotation(Window window);

	/** @brief Set the color depth of a window
	 *
	 * @param colorDepth
	 */
	void setColorDepth(Window window, ImageDataFormat colorDepth);

	/** @brief Get the current color depth of the main layer.
	 *
	 * @retval ImageDataFormat current color depth
	 */
	ImageDataFormat getColorDepth(Window window);

	/** @brief Get the number of bytes used per pixel based on the main layer color depth.
	 *
	 * @retval number of bytes per pixel (possible: 1, 2, or 3; 0 if window is invalid)
	 */
	uint8_t getBytesPerPixel(Window window)
	{
		return ::getBytesPerPixel(getColorDepth(window));
	}

	/** @brief Set the video memory start address for a window
	 * 	@note The start address must be 32-bit aligned (must be divisible by 4).
	 *
	 * @param lcdStartAddress Offset, in bytes, into display memory where the layer image starts
	 */
	void setStartAddress(Window window, uint32_t lcdStartAddress);

	/** @brief Get the video memory start address for a window.
	 *  @note The start address must be 32-bit aligned (must be divisible by 4).
	 *
	 * @retval uint32_t the layer image start address.
	 *
	*/
	uint32_t getStartAddress(Window window);
	uint32_t getAddress(Window window, uint16_t x, uint16_t y);
	uint32_t getAddress(Window window, SePos pos)
	{
		return getAddress(window, pos.x, pos.y);
	}

	/** @brief Get physical display width */
	uint16_t getDisplayWidth();

	/** @brief Get physical display height */
	uint16_t getDisplayHeight();

	/** @brief Get physical display dimensions */
	SeSize getDisplaySize()
	{
		return SeSize(getDisplayWidth(), getDisplayHeight());
	}

	/** @brief Set the width of a window, in pixels
	 *  @note For the main window, this sets the physical LCD width regardless of
	 *  the current rotation setting and must be an integer multiple of 8.
	 *
	 * param width in pixels
	 *
	*/
	void setWidth(Window window, uint16_t width);

	/** @brief Get the width of a window, accounting for rotation
	 * @note
	 * - for 0 and 180 degree rotation, the width of the Main Layer is based
	 *   on the width of the physical LCD
	 * - for 90 and 270 degree rotation, the width of the Main Layer is
	 *   based on the height of the physical LCD
	 *
	 * return:
	 * - width of the window in pixels
	 *
	*/
	uint16_t getWidth(Window window);

	/** @brief Set the height of a window in pixels
	 *  @note For the main window, this sets the physical LCD height regardless of rotation
	 *
	 * param	height	height of the window, in lines (pixels)
	 *
	 */
	void setHeight(Window window, uint16_t height);

	/** @brief Get the height of a window, accounting for rotation
	 * @note
	 * - for 0 and 180 degree rotation, the height of the Main Layer is based
	 *   on the height of the physical LCD
	 * - for 90 and 270 degree rotation, the height of the Main Layer is
	 *   based on the width of the physical LCD
	 *
	 * return:
	 * - height of the window in pixels
	 *
	*/
	uint16_t getHeight(Window window);

	SeSize getWindowSize(Window window);

	/** @brief Get the window stride, in bytes
	 *  @note
	 * Stride is the number of bytes in one line (or row) of the image. It can be used as
	 * the number that must be added to the address of a pixel in display memory to obtain
	 * the address of the pixel directly below it.
	 *
	 * return:
	 * - Main Layer stride, in bytes
	 *
	*/
	uint16_t getStride(Window window);

	//pip layer functions

	/** @brief Set the effect (blink/fade) for the PIP window.
	 *
	 * @param newEffect
	*/
	void pipSetDisplayMode(PipEffect newEffect);

	/** @brief Get the current effect (blink/fade) for the PIP window.
	 *
	 * @retval PipEffect
	*/
	PipEffect pipGetDisplayMode();

	/** @brief Determine if the Main and PIP layers have the same rotation.
	 *
	 * return:
	 * - True if same rotation, False if different rotation.
	 *
	 */
	bool pipIsOrthogonal();

	/** @brief Set the top left corner position of the PIP window
	 *  @note
	 * - the PIP x,y coordinates must be set within the panel display area
	 * - Main Layer rotation is not checked, so the PIP position is always
	 *   relative to the top left corner of the panel.
	 *
	 * param	xPos	new x PIP window coordinate, in pixels
	 * param	yPos	new y PIP window coordinate, in pixels
	 *
	*/
	void pipSetPosition(SePos pos);

	void pipSetPosition(uint16_t xPos, uint16_t yPos)
	{
		pipSetPosition(SePos(xPos, yPos));
	}

	/** @brief Get the position of the PIP window (x,y in pixels)
	 *
	 * param	xPos	pointer to the x starting position value, in pixels
	 * param	yPos	pointer to the y starting position value, in pixels
	 *
	*/
	SePos pipGetPosition();

	/** @brief Set the Blink/Fade period (in frames) for the PIP window.
	 * @note
	 *
	 * When the PIP layer is set to use Fade In, Fade Out, or continuous
	 * Fade In/Out, the fadeRate value is used to determine the number of
	 * frames to pause before setting the Alpha blend ratio to the next step.
	 *
	 * param	fadeRate	number of frames to wait between automatic
	 * 						alpha blend ratio steps. Range is from 1 to 64.
	 *
	*/
	void pipSetFadeRate(uint8_t fadeRate);

	/** @brief Get the current Blink/Fade period (in frames) for the PIP window.
	 *	@note
	 * When the PIP layer is set to use Fade In, Fade Out, or continuous
	 * Fade In/Out, the fadeRate value is used to determine the number of
	 * frames to pause before setting the Alpha blend ratio to the next step.
	 *
	 * @retval uint8_t Blink/fade period of PIP window, in frames
	 *
	*/
	uint8_t pipGetFadeRate();

	/** @brief Wait for the current PIP Blink/Fade operation to complete.
	 * @note
	 *
	 * This method is normally used for one-time Fade Out and Fade In
	 * PIP Effects to check when the fade-out or fade-in has finished. It
	 * is also used when transitioning from the Blink1, Blink2, and
	 * Fade In/Out Continuous effects to the Normal or Blank effects to
	 * check when the blinking or fading has finished.
	 *
	 * param	maxTime		maximum time to wait (timeout value)
	 *
	 * return:
	 * - True for fade has completed, False if maxTime reached
	 *
	*/
	bool pipWaitForFade(uint16_t maxTime);

	/** @brief Set the Alpha Blend step for the PIP window.
	 * @note
	 *
	 * The alpha blend step determines the increment/decrement steps for the
	 * alpha blend value during fade in/ fade out effects. If the PIP window
	 * Alpha Blend Ratio is not set to "Full PIP" (100%), the blend step
	 * should be set such that the step value is evenly divisible into the
	 * Alpha Blending Ratio.
	 *
	 * param	step	alpha blend step (acceptable values: 1, 2, 4, 8)
	 *
	*/
	void pipSetAlphaBlendStep(uint8_t step);

	/** @brief Get the Alpha Blend step for the PIP window.
	 * @note
	 * The alpha blend step determines the increment/decrement steps for the
	 * alpha blend value during fade in/ fade out effects. If the PIP window
	 * Alpha Blend Ratio is not set to "Full PIP" (100%), the blend step
	 * should be set such that the step value is evenly divisible into the
	 * Alpha Blending Ratio.
	 *
	 * return:
	 * - alpha blend step value (possible values: 1, 2, 4, 8)
	 * - a return value of 0 indicates an error
	 *
	*/
	uint16_t pipGetAlphaBlendStep();

	/** @brief Set the Alpha Blend ratio (in percent) for the PIP window.
	 * @note
	 * The PIP layer can be alpha blended with the Main layer image. The
	 * S1D13781 supports 64 levels of blending, but this function simplifies'
	 * by allowing the ratio to be specified as a percentage which is
	 * calculated to the nearest value.
	 *
	 * param	ratio	percentage for the PIP layer alpha blend, ranging
	 * 					from 0% (PIP not visible) to 100% (only PIP visible)
	 *
	*/
	void pipSetAphaBlendRatio(uint8_t ratio);

	/** @brief Get the Alpha Blend ratio (in percent) for the PIP window.

	 * @retval PIP layer alpha blend ratio to the nearest percent (%)
	*/
	uint8_t pipGetAlphaBlendRatio();

	/** @brief Enable/disable the Transparency function for the PIP window.
	 *
	 * param	enable	Set to True to enable transparency, False to disable
	 * 					transparency
	 *
	*/
	void pipEnableTransparency(bool enable);

	/** @brief Get the current Transparency state for the PIP window.
	 *
	 * @retval True if transparency is enabled, false if transparency is disabled
	 *
	*/
	bool pipGetTransparency();

	/** @brief Set the Transparency Key Color for the PIP window.
	 * 	@note
	 *
	 * When the PIP layer transparency is enabled, any pixels in the PIP
	 * layer that match the transparent color become transparent and the
	 * Main layer pixels are displayed instead.
	 *
	 * The transparent color is defined as a 32-bit RGB8888 as follows:
	 *   - x (bits 31-24) = unused
	 *   - r (bits 23-16) = 8-bits of Red
	 *   - g (bits 15-8) = 8-bits of Green
	 *   - b (bits 7-0) = 8-bits of Blue
	 *
	 * Note:
	 * - for modes other than RGB888, refer to the specification to see
	 *   which bits are significant
	 *
	 * param	xrgbColor	transparent key color as described above
	 *
	*/
	void pipSetTransColor(uint32_t xrgbColor);

	/** @brief Get the Transparency Key Color for the PIP window.
	 *
	 * @retval SeColor transparency key color
	*/
	SeColor pipGetTransColor();

	/** @brief Setup a PIP window with a single function call.
	 * @note
	 * This function permits setting PIP x,y coordinates, width, height and
	 * stride with one function call. Use this function to initially setup
	 * PIP or any time several parameters need to be simultaneously updated.
	 *
	 * param	xPos	new X position, in pixels, for the PIP window
	 * 					relative to the top left corner of the main window.
	 *
	 * param	yPos	new Y position, in pixels, for the PIP window
	 * 					relative to the top left corner of the main window.
	 *
	 * param	pipWidth	new width, in pixels, for the PIP window.
	 *
	 * param	pipHeight	new height, in pixels, for the PIP window.
	 *
	*/
	void pipSetupWindow(uint16_t xPos, uint16_t yPos, uint16_t pipWidth, uint16_t pipHeight);

	//lut functions

	/** @brief Set a specific LUT entry
	 * @note
	 * For details, refer to the specification on LUT Architecture
	 *
	 * WindowDestination is either:
	 * - Window::main uses LUT1 at address 0x00060000
	 * - Window::pip uses LUT2 at address 0x00060400
	 *
	 * xrgbData is defined as:
	 *   - bits 31-24 = unused
	 *   - bits 23-16 = 8-bits of Red
	 *   - bits 15-8 = 8-bits of Green
	 *   - bits 7-0 = 8-bits of Blue
	 *
	 * Note:
	 *   - for modes other than RGB888, refer to the specification to see
	 *     which bits are significant
	 *
	 * param	index		the LUT index to write the LUT data to
	 *
	 * param	xrgbData	the data to write to the LUT index
	 *
	 * param	window		which LUT to access:
	 * 						- Window::main will access LUT1
	 * 						- Window::pip will access LUT2
	 *
	*/
	void setLutEntry(Window window, uint16_t index, SeColor rgbData);

	/** @brief Get a specific LUT entry value.
	 *
	 * param	window		which LUT to access:
	 * param	index		the LUT index to read the LUT data to
	 *
	 * return:
	 * - the data from the specified LUT entry
	 *
	*/
	unsigned int getLutEntry(Window window, uint16_t index);

	/** @brief Setup a LUT
	 * @todo
	*/
	void setLut(Window window, uint16_t startIndex, const SeColor* rgbData, uint16_t count);

	/** @brief Get a set of LUT values
	 * @todo
	*/
	void getLut(Window window, uint16_t startIndex, SeColor* rgbData, uint16_t count);

	/** @brief Set the LUTs with default values.
	 * @note
	 * For details, refer to the specification on LUT Architecture
	 *
	 * WindowDestination is either:
	 * - Window::main uses LUT1 at address 0x00060000
	 * - Window::pip uses LUT2 at address 0x00060400
	 *
	 * param	window		determines whether LUT1 or LUT2 is set
	 *
	 */
	void setLutDefault(Window window);

	/** @brief Return the mapped colour code for the given RGB value
	 * 	@param rgb
	 * 	@retval uint32_t mapped colour code
	 * 	@note for LUT colour spaces we map the code here
	 */
	SeColor lookupColor(Window window, SeColor color)
	{
		/*
		 * @todo For LUT modes this should reflect the inverse mapping of setLutDefault().
		 * Dealing with custom mappings is a bit trickier.
		 */
		return RGBColor(color).getColor(getColorDepth(window));
	}

	bool bltSolidFill(Window window, SePos pos, SeSize size, SeColor color);
	bool bltMoveExpand(Window window, uint32_t srcAddr, SePos dstPos, SeSize dstSize, SeColor fgColor, SeColor bgColor);
	bool bltMove(Window window, BltCmd cmd, SePos srcPos, SePos dstPos, SeSize size);

	void scrollUp(Window window, unsigned lineCount, SeColor bgColor)
	{
		auto size = getWindowSize(window);
		bltMove(window, BltCmd::movePositive, SePos(0, lineCount), SePos(0, 0),
				SeSize(size.width, size.height - lineCount));
		bltSolidFill(window, SePos(0, size.height - lineCount), SeSize(size.width, lineCount), bgColor);
	}

	/** @brief Poll a display register and wait for a specific value, or until timeout
	 * @note
	 *
	 * The specified register is repeatedly read and will return true when:
	 *
	 * 		(register[regIndex] & regMask) == regValue
	 *
	 * @param regIndex Index of register to poll
	 * @param regMask Which bits to monitor
	 * @param regValue Matching value required
	 * @param timeoutMs	Maximum time to wait (in ms) before giving up polling
	 * Specify 0 to check register only once.
	 *
	 * @retval bool true on success, false on timeout
	 */
	bool regWait(uint8_t regIndex, uint16_t regMask, uint16_t regValue, unsigned int timeoutMs);

	bool regWaitForLow(uint8_t regIndex, uint16_t regMask, unsigned int timeoutMs)
	{
		return regWait(regIndex, regMask, 0, timeoutMs);
	}

	bool regWaitForHigh(uint8_t regIndex, uint16_t regMask, unsigned int timeoutMs)
	{
		return regWait(regIndex, regMask, regMask, timeoutMs);
	}

	const S1DTiming& getTiming()
	{
		return timing;
	}

	/** @brief Calculate display time for the given number of frames
	 *  @param frameCount
	 *  @retval uint32_t time in milliseconds
	 */
	uint16_t getDisplayTime(uint16_t frameCount)
	{
		return frameCount * timing.frameInterval / 1000UL;
	}

	/** @brief Calculate number of frames displayed an a given time
	 *  @param time in milliseconds
	 *  @retval uint16_t frame count
	 */
	uint16_t getFrameCount(uint16_t ms)
	{
		return 1000UL * ms / timing.frameInterval;
	}

	void writeWord(uint32_t address, uint32_t value, unsigned byteCount)
	{
		MemoryDevice::writeWord(reqWr, address, value, byteCount);
	}

	// HSPI::MemoryDevice
	void prepareWrite(HSPI::Request& req, uint32_t address) override;
	void prepareRead(HSPI::Request& req, uint32_t address) override;

private:
	/** @brief Private method to initialize the S1D13781 registers
	 *
	 * Note:
	 * 	Uses displaySettings[] defined in S1D13781_init.h.
	 * - this method should only be called once by ::begin() to initialize
	 *   the register values to their appropriate values. New initialization
	 *   values can be generated by the S1D13781 Configuration Utility,
	 *   exported to a "C Header File for S1D13781 Generic Drivers", and
	 *   then copied to the regInitValues[] sequence in the init.h file.
	 *
	 */
	bool initRegs();

	/** @brief Calculate and store commonly used timing values */
	void updateTiming();

	bool regReadWindow(Window window, uint16_t& value);

	// Member data

	// Small writes can be handle asynchronously
	HSPI::Request reqWr;

	uint16_t* cache{nullptr}; ///< Register cache
	S1DTiming timing;
};

} // namespace S1D13781
