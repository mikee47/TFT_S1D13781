/* ======================================================================
 * S1D13781_gfx.h
 * Header file for S1D13781 Shield Graphics library
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

#include "SeFont.h"
#include "driver.h"
#include "SeColor.h"
#include <algorithm>

#define S1D13781_SHIELD_SWVERSION "S1D13781 Shield Graphics Library V1.0.2"
#define S1D13781_SHIELD_SWRELDATE "Nov 6, 2015"

struct SeRect {
	int16_t x = 0;
	int16_t y = 0;
	uint16_t width = 0;
	uint16_t height = 0;

	SeRect()
	{
	}

	SeRect(int16_t x, int16_t y, uint16_t w, uint16_t h) : x(x), y(y), width(w), height(h)
	{
	}

	SeRect(const SePos& pos, const SeSize& size) : x(pos.x), y(pos.y), width(size.width), height(size.height)
	{
	}

	SeRect(const SeSize& size) : x(0), y(0), width(size.width), height(size.height)
	{
	}

	SePos getPos() const
	{
		return SePos(x, y);
	}

	SeSize getSize() const
	{
		return SeSize(width, height);
	}

	/** @brief bounding x co-ordinate outside rectangle */
	int16_t x2() const
	{
		return x + int16_t(width);
	}

	/** @brief bounding y co-ordinate outside rectangle */
	int16_t y2() const
	{
		return y + int16_t(height);
	}

	/** @brief Intersect self with another rectangle
	 *  @param r
	 *  @retval bool true if rectangles overlap
	 */
	bool intersect(const SeRect& r)
	{
		int16_t xo = std::max(x, r.x);
		int16_t yo = std::max(y, r.y);
		uint16_t wo = std::min(x2(), r.x2()) - xo;
		uint16_t ho = std::min(y2(), r.y2()) - yo;

		if(wo < 0 || ho < 0) {
			width = 0;
			height = 0;
			return false;
		} else {
			x = xo;
			y = yo;
			width = wo;
			height = ho;
			return true;
		}
	}

	bool overlap(const SeRect& r) const
	{
		return x < r.x2() && r.x < x2() && y < r.y2() && r.y < y2();
	}
};

//possible sample patterns
enum PatternType {
	patternRgbHorizBars,
	patternRgbHorizGradient,
	patternVertBars,
};

enum IntersectType {
	noIntersect,
	collinear,
	intersect,
};

// Graphics library functions
class S1D13781_gfx : public S1D13781
{
public:
	using S1D13781::S1D13781;

	/** @brief Fill the destination window with a specified color.
	 *
	 * param	window	Destination window to be filled.
	 *
	 * param	color	Color value
	 *
	 * return
	 * - Zero (0) indicates no errors.
	 * - 1 indicates invalid window error.
	 * - 2 indicates invalid image format error.
	 *
	 */
	uint16_t fillWindow(WindowDestination window, SeColor color);

	uint16_t clearWindow(WindowDestination window)
	{
		return fillWindow(window, aclBlack);
	}

	/* drawPixel()
	 * Method to draw a pixel at the specified x,y coordinate using the
	 * specified color.
	 *
	 * param	window	Destination window for the pixel.
	 *
	 * param	x		X coordinate of the pixel relative to 0,0 of
	 * 					the window.
	 *
	 * param	y		Y coordinate of the pixel relative to 0,0 of
	 * 					the window.
	 *
	 * param	color	Color value as specified above.
	 *
	 * param	update_params	Determines whether draw_pixel will use the
	 * 					"cached" parameters for this pixel draw. If in doubt,
	 * 					set true to update the values (slower but no
	 * 					potential for problems)
	 *
	 * return
	 * - Zero (0) indicates no errors.
	 * - 1 indicates invalid window error.
	 * - 2 indicates coordinate out of window error.
	 *
	 */
	uint16_t drawPixel(WindowDestination window, int x, int y, SeColor color, bool update_params);

	/** @param Read the color value of a pixel at the specified x,y coordinates.
	 *
	 *
	 * param	window		Source window for the pixel.
	 *
	 * param	x			X coordinate of the pixel relative to 0,0 of
	 * 						the window.
	 *
	 * param	y			Y coordinate of the pixel relative to 0,0 of
	 * 						the window.
	 *
	 * return
	 * - 0x00000000 to 0x00FFFFFF - Color value of the pixel depending on color format.
	 * - 0xFF000000 - indicates invalid window error.
	 * - 0xFE000000 - indicates coordinate out of window error.
	 * - 0xFD000000 - indicates invalid window image format error.
	 *
	 */
	SeColor getPixel(WindowDestination window, int x, int y);

	/** @param Draw a line between 2 specified x,y coordinates using the specified color.
	 *
	 *
	 * param	window	Destination window for the line.
	 *
	 * param	x1		X1 coordinate of the first endpoint for the line
	 * 					relative to 0,0 of the window.
	 *
	 * param	y1		Y1 coordinate of the first endpoint for the line
	 * 					relative to 0,0 of the window.
	 *
	 * param	x2		X2 coordinate of the second endpoint for the line
	 * 					relative to 0,0 of the window.
	 *
	 * param	y2		Y2 coordinate of the second endpoint for the line
	 * 					relative to 0,0 of the window.
	 *
	 * param	color	Color value as specified above.
	 *
	 * return
	 * - Zero (0) indicates no errors.
	 * - 1 indicates invalid window error.
	 * - 2 indicates coordinate out of window error.
	 */
	uint16_t drawLine(WindowDestination window, int x1, int y1, int x2, int y2, SeColor color);

	/** @brief Draw a rectangle of a specified width,height starting at pixel coordinate x,y using the specified color.
	 *
	 * param	window	Destination window for the rectangle.
	 *
	 * param	xStart	X coordinate of rectangle start point relative
	 * 					to 0,0 of the window.
	 *
	 * param	yStart	Y coordinate of rectangle start point relative
	 * 					to 0,0 of the window.
	 *
	 * param	width	Width of the rectangle.
	 *
	 * param	height	Height of the rectangle.
	 *
	 * param	color	Color value as specified above.
	 *
	 * return
	 * - Zero (0) indicates no errors.
	 * - 1 indicates invalid window error.
	 * - 2 indicates coordinate out of window error.
	 */
	uint16_t drawRect(WindowDestination window, int xStart, int yStart, int width, int height, SeColor color);

	/** Draw a filled rectangle of a specified width,height starting at pixel
	 * coordinate x,y using the specified color. This function uses the BitBLT
	 * function of the S1D13781 instead of the traditional line buffer method
	 * (which is slower). See also drawFilledRectSlow().
	 *
	 * param	window	Destination window for the rectangle.
	 *
	 * param	xStart	X coordinate of rectangle start point relative
	 * 					to 0,0 of the window.
	 *
	 * param	yStart	Y coordinate of rectangle start point relative
	 * 					to 0,0 of the window.
	 *
	 * param	width	Width of the rectangle.
	 *
	 * param	height	Height of the rectangle.
	 *
	 * param	color	Color value as specified above.
	 *
	 * return
	 * - Zero (0) indicates no errors.
	 * - 1 indicates invalid window error.
	 * - 2 indicates invalid window format error.
	 *
	 */
	uint16_t drawFilledRect(WindowDestination window, int xStart, int yStart, int width, int height, SeColor color)
	{
		return bltSolidFill(window, SePos(xStart, yStart), SeSize(width, height), color) ? 0 : 2;
	}
	uint16_t drawFilledRect(WindowDestination window, const SeRect& rect, SeColor color)
	{
		return bltSolidFill(window, rect.getPos(), rect.getSize(), color) ? 0 : 2;
	}

	/** @brief Draw a filled rectangle of a specified width,height
	 * starting at pixel coordinate x,y using the specified color. This
	 * function uses the slower line buffer method (instead of BitBLT)
	 * and is primarily for test purposes.
	 *
	 * param	window	Destination window for the rectangle.
	 *
	 * param	xStart	X coordinate of rectangle start point relative
	 * 					to 0,0 of the window.
	 *
	 * param	yStart	Y coordinate of rectangle start point relative
	 * 					to 0,0 of the window.
	 *
	 * param	width	Width of the rectangle.
	 *
	 * param	height	Height of the rectangle.
	 *
	 * param	color	Color value as specified above.
	 *
	 * return
	 * - Zero (0) indicates no errors.
	 * - 1 indicates invalid window error.
	 * - 2 indicates invalid window format error.
	 *
	 */
	uint16_t drawFilledRectSlow(WindowDestination window, const SeRect& rect, SeColor color);
	uint16_t drawFilledRectSlow(WindowDestination window, int xStart, int yStart, unsigned width, unsigned height,
								SeColor color)
	{
		return drawFilledRectSlow(window, SeRect(xStart, yStart, width, height), color);
	}

	/** @brief Draw useful test patterns to the specified window.
	 *
	 * param	window		Destination window for the rectangle.
	 *
	 * param	pattern		Test pattern type:
	 * 						- patternRgbHorizBars (solid horizontal RGB bars)
	 * 						- patternRgbHorizGradient (gradient horiz. RGB bars)
	 * 						- patternVertBars (vertical TV style color bars)
	 *
	 * param	intensity
	 * 						Intensity of colors used for patternRgbHorizBars
	 * 						and patternVertBars as a percentage (0 = black,
	 * 						100 = full intensity colors)
	 *						Note: This setting is not used for
	 * 						      patternRgbHorizGradient.
	 *
	 * return
	 * - Zero (0) indicates no errors.
	 * - 1 indicates invalid window error.
	 * - 2 indicates invalid image format error.
	 * - 3 indicates invalid pattern error.
	 *
	 */
	uint16_t drawPattern(WindowDestination window, PatternType pattern, uint8_t intensity);

	/** @brief Draw text containing "Chars" to the specified window using the given font.
	 *
	 * param	window		Destination window where the text is drawn.
	 * param	font		The font that will be used for the text
	 * param	text		The text to be drawn.
	 * param	X,Y			X and Y position where the text will be drawn.
	 * param	width		The cropping area width (in pixels). A value of 0 means to crop at window width.
	 * param	fgColor		The color of the text.
	 * param	bgColor		The background color behind the text. A nullptr value specifies the background will be unchanged.
	 * param	wordCrop	True if cropping should be done on uint16_t boundaries.
	 * param	cropped		Returns True if text was cropped (can be set to nullptr).
	 *
	 * return unsigned int The number of characters drawn (after cropping).
	 *
	 */
	unsigned int drawText(WindowDestination window, const SeFont& font, const char* text, int X, int Y,
						  unsigned int width, SeColor fgColor, SeColor bgColor, bool wordCrop, bool* cropped = nullptr);
	unsigned int drawTextTransparent(WindowDestination window, const SeFont& font, const char* text, int X, int Y,
									 unsigned int width, SeColor fgColor, bool wordCrop, bool* cropped = nullptr);

	/** @brief Draw text containing "Wchars" to the specified window using the given font.
	 *
	 * @param	window		Destination window where the text is drawn.
	 *
	 * @param	font		The font that will be used for the text. The
	 * 						Font information must have been previously
	 * 						created with createFont().
	 *
	 * @param	text		The text to be drawn.
	 *
	 * @param	X,Y			X and Y position where the text will be drawn.
	 *
	 * @param	width		The cropping area width (in pixels). A value of
	 * 						0 means to crop at window width.
	 *
	 * @param	fgColor		The color of the text.
	 *
	 * @param	bgColor		The background color behind the text. A nullptr
	 * 						value specifies the background will be unchanged.
	 *
	 * @param	wordCrop	True if cropping should be done on uint16_t
	 * 						boundaries.
	 *
	 * @param	cropped		Returns True if text was cropped (can be set
	 * 						to nullptr).
	 *
	 * @retval unsigned The number of characters drawn (after cropping).
	 *
	 */
	unsigned int drawTextW(WindowDestination window, const SeFont& font, const wchar_t* text, int X, int Y,
						   unsigned int width, SeColor fgColor, SeColor bgColor, bool wordCrop,
						   bool* cropped = nullptr);

	/** @brief Draw multiple lines of text containing "Chars" to the specified window using the given font.
	 *
	 * param	window		Destination window where the text is drawn.
	 *
	 * param	font		The font that will be used for the text. The
	 * 						Font information must have been previously
	 * 						created with createFont().
	 *
	 * param	text		The text to be drawn.
	 *
	 * param	X,Y			X and Y position where the text will be drawn.
	 *
	 * param	width		The cropping area width (in pixels). A value of
	 * 						0 means to crop at window width.
	 *
	 * param	fgColor		The color of the text.
	 *
	 * param	bgColor		The background color behind the text. A nullptr
	 * 						value specifies the background will be unchanged.
	 *
	 * param	wordCrop	True if cropping should be done on uint16_t
	 * 						boundaries.
	 *
	 * param	cropped		Returns True if text was cropped (can be set
	 * 						to nullptr).
	 *
	 * param	linesDrawn	Returns the number of lines it took to draw the
	 * 						text.
	 *
	 * return
	 * - The number of characters drawn (after cropping).
	 *
	 */
	unsigned int drawMultiLineText(WindowDestination window, const SeFont& font, const char* text, int X, int Y,
								   unsigned int width, SeColor fgColor, SeColor bgColor, bool wordCrop,
								   bool* cropped = nullptr, unsigned int* linesDrawn = nullptr);

	/** Draw multiple lines of text containing "Wchars" to the specified window using the given font.
	 *
	 * param	window		Destination window where the text is drawn.
	 *
	 * param	font		The font that will be used for the text. The
	 * 						Font information must have been previously
	 * 						created with createFont().
	 *
	 * param	text		The text to be drawn.
	 *
	 * param	X,Y			X and Y position where the text will be drawn.
	 *
	 * param	width		The cropping area width (in pixels). A value of
	 * 						0 means to crop at window width.
	 *
	 * param	fgColor		The color of the text.
	 *
	 * param	bgColor		The background color behind the text. A nullptr
	 * 						value specifies the background will be unchanged.
	 *
	 * param	wordCrop	True if cropping should be done on uint16_t
	 * 						boundaries.
	 *
	 * param	cropped		Returns True if text was cropped (can be set
	 * 						to nullptr).
	 *
	 * param	linesDrawn	Returns the number of lines it took to draw the
	 * 						text.
	 *
	 * return
	 * - The number of characters drawn (after cropping).
	 *
	 */
	unsigned int drawMultiLineTextW(WindowDestination window, const SeFont& font, const wchar_t* text, int X, int Y,
									unsigned int width, SeColor fgColor, SeColor bgColor, bool wordCrop,
									bool* cropped = nullptr, unsigned int* linesDrawn = nullptr);

	/** @brief Draw a raw RGB image
	 *  @param image The image data, stored in flash memory
	 *  @param window
	 *  @param x
	 *  @param y
	 *  @param imageWidth
	 *  @param imageHeight
	 */
	void drawImage(const FSTR::ObjectBase& image, WindowDestination window, int x, int y, unsigned imageWidth,
				   unsigned imageHeight);

	//uint16_t drawImage();  //TODO add for future versions of library
	//uint16_t copyArea(WindowDestination srcWindow, WindowDestination destWindow, S1D13781_gfx::seRect area, int destX, int destY);

	/** @brief Copy a rectangular area to another area.
	 *
	 * param	srcWindow	Source window area for the copy.
	 *
	 * param	destWindow	Destination window to copy the area to.
	 *
	 * param	area		Source rectangle to be copied (x,y,w,h).
	 *
	 * param	destX		Destination X coordinate.
	 *
	 * param	destY		Destination Y coordinate.
	 *
	 * return
	 * - Returns 0 if successful.
	 * - Returns 1 if invalid window error.
	 * - Returns 2 if line buffer memory allocation error.
	 * - Returns 3 if drawPixel error.
	 *
	 */
	uint16_t copyArea(WindowDestination srcWindow, WindowDestination destWindow, SeRect area, int destX, int destY);

private:
	/** @brief Private Method used by the S1D13781 graphics library functions.
	 */
	IntersectType _getIntersectionPoint(int AX1, int AY1, int AX2, int AY2, int BX1, int BY1, int BX2, int BY2,
										int* intersectX, int* intersectY);

	/** @brief Private Method used by the S1D13781 graphics library functions.
	 */
	bool _cropLine(int* X1, int* Y1, int* X2, int* Y2, int cropRegionX, int cropRegionY, int cropRegionWidth,
				   int cropRegionHeight);

	/** @brief Draw a pattern of solid horizontal RGB color bars. This
	 *
	 * param	window		Destination window for pattern.
	 * param	intensity   Color intensity, max 100
	 *
	 * return
	 * - None.
	 *
	 */
	void _drawRgbHorizBars(WindowDestination window, unsigned int intensity);

	/** @brief Draw a pattern of gradient horizontal RGB color bars.
	 *
	 * param	window		Destination window for pattern.
	 *
	 * return
	 * - None.
	 *
	 */
	void _drawRgbHorizGradient(WindowDestination window);

	/** @brief Draw a pattern of solid vertical color bars.
	 *
	 * param	window		Destination window for pattern.
	 *
	 * param	intensity
	 * 						Intensity of colors used for generating the
	 * 						vertical color bars as a percentage (0 = black,
	 * 						100 = full intensity colors)
	 *
	 * return
	 * - None.
	 *
	 */
	void _drawVertBars(WindowDestination window, unsigned int intensity);

	//void _copyImage

	/** @brief Copy a rectangular area to another area without any format translation
	 *
	 * param	window		Source window area for the copy.
	 *
	 * param	area		Source rectangle to be copied (x,y,w,h).
	 *
	 * param	destX		Destination X coordinate.
	 *
	 * param	destY		Destination Y coordinate.
	 *
	 * return
	 * - Returns 0 if successful.
	 * - Returns 1 if invalid window error.
	 * - Returns 2 if line buffer memory allocation error.
	 * - Returns 3 if drawPixel error.
	 *
	 */
	uint16_t _bitBLTRegion(WindowDestination srcWindow, const SeRect& area, int destX, int destY);

	/** @brief Copy a rectangular area to another area with color translation
	 *
	 * param	srcWindow	Source window area for the copy.
	 *
	 * param	destWindow	Destination window to copy the area to.
	 *
	 * param	area		Source rectangle to be copied (x,y,w,h).
	 *
	 * param	destX		Destination X coordinate.
	 *
	 * param	destY		Destination Y coordinate.
	 *
	 * return
	 * - Returns 0 if successful.
	 * - Returns 1 if invalid window error.
	 * - Returns 2 if line buffer memory allocation error.
	 * - Returns 3 if drawPixel error.
	 *
	 */
	uint16_t _copyRegion(WindowDestination srcWindow, WindowDestination destWindow, SeRect area, int16_t destX,
						 int16_t destY);
};
