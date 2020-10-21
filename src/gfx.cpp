/* ======================================================================
 * S1D13781_gfx.cpp
 * Source file for S1D13781 Shield Graphics library
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

#include <S1D13781/gfx.h>
#include <S1D13781/registers.h>
#include "PixelBuffer.h"
#include "BitBuffer.h"
#include <stringutil.h>

#define seSameSigns(A, B) (((A) >= 0) ^ ((B) < 0))

uint16_t S1D13781_gfx::fillWindow(WindowDestination window, SeColor color)
{
	return drawFilledRect(window, 0, 0, getWidth(window), getHeight(window), color);
}

uint16_t S1D13781_gfx::drawPixel(WindowDestination window, int x, int y, SeColor color, bool update_params)
{
	static uint32_t vramAddress;   //video memory address
	static uint16_t stride;		   //number of bytes in line
	static uint16_t bytesPerPixel; //number of bytes per pixel
	static uint16_t width;		   //width of window (used for range checking)
	static uint16_t height;		   //height of window (used for range checking)

	//only update the params if necessary, save a bunch of register reads
	if(update_params) {
		vramAddress = getStartAddress(window);
		stride = getStride(window); //number of bytes in line
		bytesPerPixel = getBytesPerPixel(window);
		width = getWidth(window);
		height = getHeight(window);
	}

	//check that pixel coordinate is valid
	if((x >= width) || (y >= height)) {
		return 2; //error coordinate out of window
	}

	if(bytesPerPixel == 0) {
		return 2; //error invalid window image format
	}

	//calculate the memory offset of the pixel
	unsigned pixelOffset = vramAddress + (y * stride) + (x * bytesPerPixel);

	//draw the pixel color according to image format
	writeWord(pixelOffset, lookupColor(window, color), bytesPerPixel);

	return 0;
}

SeColor S1D13781_gfx::getPixel(WindowDestination window, int x, int y)
{
	SeSize size = getWindowSize(window);
	//check that pixel coordinate is valid
	if((x >= size.width) || (y >= size.height)) {
		return 0xFE000000; //error coordinate out of window
	}

	auto format = getColorDepth(window);
	uint8_t bytesPerPixel = ::getBytesPerPixel(format);

	//read the correct number of bytes based on the image format
	if(bytesPerPixel == 0) {
		return 0xFD000000; //error invalid window image format
	}

	//calculate the memory offset of the pixel
	unsigned pixelOffset = getStartAddress(window) + (y * getStride(window)) + (x * bytesPerPixel);

	uint32_t value = readWord(pixelOffset, bytesPerPixel);
	return SeColor(value, format);
}

uint16_t S1D13781_gfx::drawLine(WindowDestination window, int x1, int y1, int x2, int y2, SeColor color)
{
	color = lookupColor(window, color);

	uint16_t width = getWidth(window);
	uint16_t height = getHeight(window);

	_cropLine(&x1, &y1, &x2, &y2, 0, 0, width, height);

	if(x1 > x2) {
		seSwap(x1, x2);
		seSwap(y1, y2);
	}

	int dx = x2 - x1;
	if(dx < 0) {
		dx = -dx;
	}

	int dy = y2 - y1;
	if(dy < 0) {
		dy = -dy;
	}

	if(dx > dy) {
		if(x1 > x2) {
			seSwap(x1, x2);
			seSwap(y1, y2);
		}

		int yincr = (y2 > y1) ? 1 : -1;
		dx = x2 - x1;
		dy = y2 - y1; // dy = abs( y2-y1 );
		if(dy < 0) {
			dy = -dy;
		}

		int d = 2 * dy - dx;
		int Aincr = 2 * (dy - dx);
		int Bincr = 2 * dy;

		int x = x1;
		int y = y1;
		drawPixel(window, x, y, color, true);

		for(x = x1 + 1; x <= x2; x++) {
			if(d >= 0) {
				y += yincr;
				d += Aincr;
			} else {
				d += Bincr;
			}

			drawPixel(window, x, y, color, false);
		}
	} else {
		if(y1 > y2) {
			seSwap(x1, x2);
			seSwap(y1, y2);
		}

		int xincr = (x2 > x1) ? 1 : -1;
		dy = y2 - y1;
		dx = x2 - x1; // dx = abs( x2-x1 );
		if(dx < 0) {
			dx = -dx;
		}

		int d = 2 * dx - dy;
		int Aincr = 2 * (dx - dy);
		int Bincr = 2 * dx;

		int x = x1;
		int y = y1;
		drawPixel(window, x, y, color, true);

		for(y = y1 + 1; y <= y2; y++) {
			if(d >= 0) {
				x += xincr;
				d += Aincr;
			} else {
				d += Bincr;
			}
			drawPixel(window, x, y, color, false);
		}
	}

	return 0;
}

uint16_t S1D13781_gfx::drawRect(WindowDestination window, int xStart, int yStart, int width, int height, SeColor color)
{
	//	uint16_t stride;		//number of bytes in line
	//	uint16_t bytesPerPixel; //number of bytes per pixel

	int x2 = xStart + width - 1;
	int y2 = yStart + height - 1;

	color = lookupColor(window, color);
	drawLine(window, xStart, yStart, x2, yStart, color);
	drawLine(window, xStart, y2, x2, y2, color);
	drawLine(window, xStart, yStart, xStart, y2, color);
	drawLine(window, x2, yStart, x2, y2, color);

	return 0;
}

uint16_t S1D13781_gfx::drawFilledRectSlow(WindowDestination window, const SeRect& rect, SeColor color)
{
	auto format = getColorDepth(window);
	uint8_t bytesPerPixel = ::getBytesPerPixel(format);
	if(bytesPerPixel == 0) {
		return 1; //error invalid window
	}

	// Display rect (the area of the given rect that exists on the surface)
	SeRect r(getWindowSize(window));
	if(!r.intersect(rect)) {
		return 1;
	}

	//	debug_i("(%u, %u, %u, %u)", r.x, r.y, r.width, r.height);

	PixelBuffer buffer;
	if(!buffer.initialise(r.width, 1, format)) {
		return 3;
	}

	// Create first line
	buffer.fill(lookupColor(window, color));

	// Burst-write using the line buffer to the display
	uint16_t stride = getStride(window);
	uint32_t addr = getAddress(window, r.x, r.y);
	auto ptr = buffer.getPtr();
	auto size = buffer.getSize();
	while(r.height--) {
		write(addr, ptr, size);
		addr += stride;
	}

	return 0;
}

uint16_t S1D13781_gfx::drawPattern(WindowDestination window, PatternType pattern, uint8_t intensity)
{
	if(getBytesPerPixel(window) == 0) {
		return 1; // invalid window
	}

	switch(pattern) {
	case patternRgbHorizBars:
		_drawRgbHorizBars(window, intensity);
		break;

	case patternRgbHorizGradient:
		_drawRgbHorizGradient(window);
		break;

	case patternVertBars:
		_drawVertBars(window, intensity);
		break;

	default:
		return 3; //invalid pattern error
	}

	return 0;
}

IntersectType S1D13781_gfx::_getIntersectionPoint(int AX1, int AY1, int AX2, int AY2, int BX1, int BY1, int BX2,
												  int BY2, int* intersectX, int* intersectY)
{
	long a1, a2, b1, b2, c1, c2; // Coefficients of line eqns.
	long r1, r2, r3, r4;		 // 'Sign' values
	long denom, offset, num;	 // Intermediate values

	// Compute a1, b1, c1, where line joining points 1 and 2 is "a1 IntersectX  +  b1 IntersectY  +  c1  =  0".
	a1 = AY2 - AY1;
	b1 = AX1 - AX2;
	c1 = AX2 * AY1 - AX1 * AY2;

	// Compute r3 and r4.
	r3 = a1 * BX1 + b1 * BY1 + c1;
	r4 = a1 * BX2 + b1 * BY2 + c1;

	// Check signs of r3 and r4.  If both point 3 and point 4 lie on same side of line 1, the line segments do not intersect.
	if(r3 != 0 && r4 != 0 && seSameSigns(r3, r4))
		return noIntersect;

	// Compute a2, b2, c2
	a2 = BY2 - BY1;
	b2 = BX1 - BX2;
	c2 = BX2 * BY1 - BX1 * BY2;

	// Compute r1 and r2
	r1 = a2 * AX1 + b2 * AY1 + c2;
	r2 = a2 * AX2 + b2 * AY2 + c2;

	// Check signs of r1 and r2.  If both point 1 and point 2 lie on same side of second line segment, the line segments do not intersect.
	if(r1 != 0 && r2 != 0 && seSameSigns(r1, r2))
		return noIntersect;

	// Line segments intersect: compute intersection point.
	denom = a1 * b2 - a2 * b1;
	if(denom == 0)
		return collinear;

	offset = (denom < 0) ? -denom / 2 : denom / 2;
	// The denom/2 is to get rounding instead of truncating.
	// It is added or subtracted to the numerator, depending upon the sign of the numerator.

	num = b1 * c2 - b2 * c1;
	*intersectX = (num < 0 ? num - offset : num + offset) / denom;

	num = a2 * c1 - a1 * c2;
	*intersectY = (num < 0 ? num - offset : num + offset) / denom;

	return intersect;
}

bool S1D13781_gfx::_cropLine(int* X1, int* Y1, int* X2, int* Y2, int cropRegionX, int cropRegionY, int cropRegionWidth,
							 int cropRegionHeight)
{
	bool lineCropped = false;
	int RX1 = cropRegionX;
	int RY1 = cropRegionY;
	int RX2 = cropRegionX + cropRegionWidth - 1;
	int RY2 = cropRegionY + cropRegionHeight - 1;
	int ix = 0;
	int iy = 0;

	// Does line intersect top horizontal line?
	if(_getIntersectionPoint(*X1, *Y1, *X2, *Y2, RX1, RY1, RX2, RY1, &ix, &iy) == intersect) {
		if(*Y1 <= RY1) {
			*X1 = ix;
			*Y1 = iy;
		} else {
			*X2 = ix;
			*Y2 = iy;
		}
		lineCropped = true;
	}

	// Does line intersect bottom horizontal line?
	if(_getIntersectionPoint(*X1, *Y1, *X2, *Y2, RX1, RY2, RX2, RY2, &ix, &iy) == intersect) {
		if(*Y1 >= RY2) {
			*X1 = ix;
			*Y1 = iy;
		} else {
			*X2 = ix;
			*Y2 = iy;
		}
		lineCropped = true;
	}

	// Does line intersect left vertical line?
	if(_getIntersectionPoint(*X1, *Y1, *X2, *Y2, RX1, RY1, RX1, RY2, &ix, &iy) == intersect) {
		if(*X1 <= RX1) {
			*X1 = ix;
			*Y1 = iy;
		} else {
			*X2 = ix;
			*Y2 = iy;
		}
		lineCropped = true;
	}

	// Does line intersect right vertical line?
	if(_getIntersectionPoint(*X1, *Y1, *X2, *Y2, RX2, RY1, RX2, RY2, &ix, &iy) == intersect) {
		if(*X1 >= RX2) {
			*X1 = ix;
			*Y1 = iy;
		} else {
			*X2 = ix;
			*Y2 = iy;
		}
		lineCropped = true;
	}

	return lineCropped;
}

void S1D13781_gfx::_drawRgbHorizBars(WindowDestination window, unsigned int intensity)
{
	SeRect r(getWindowSize(window));
	r.height /= 3;
	drawFilledRect(window, r, scaleColor(aclRed, intensity));
	r.y += r.height;
	drawFilledRect(window, r, scaleColor(aclGreen, intensity));
	r.y += r.height;
	drawFilledRect(window, r, scaleColor(aclBlue, intensity));
}

void S1D13781_gfx::_drawRgbHorizGradient(WindowDestination window)
{
	auto windowSize = getWindowSize(window);

	//initialize the values used for the gradients
	const uint8_t topRed = 0;
	const uint8_t topGreen = 0;
	const uint8_t topBlue = 0;
	const uint8_t bottomRed = 0xFF - topRed;	 // 8 bits Red
	const uint8_t bottomGreen = 0xFF - topGreen; //8 bits Green
	const uint8_t bottomBlue = 0xFF - topBlue;   //8 bits Blue

	//do red gradient
	unsigned bandHeight = windowSize.height / 3;
	SePos pos(0, 0);
	SeSize size(windowSize.width, 1);
	for(unsigned i = 0; i < bandHeight; ++i) {
		RGBColor color(topRed + (bottomRed * i / bandHeight), 0, 0);
		bltSolidFill(window, pos, size, color.value);
		++pos.y;
	}

	for(unsigned i = 0; i < bandHeight; ++i) {
		RGBColor color(0, topGreen + (bottomGreen * i / bandHeight), 0);
		bltSolidFill(window, pos, size, color.value);
		++pos.y;
	}

	bandHeight = windowSize.height - pos.y;
	for(unsigned i = 0; i < bandHeight; ++i) {
		RGBColor color(0, 0, topBlue + (bottomBlue * i / bandHeight));
		bltSolidFill(window, pos, size, color.value);
		++pos.y;
	}
}

void S1D13781_gfx::_drawVertBars(WindowDestination window, unsigned int intensity)
{
	const uint32_t colors[] = {
		aclWhite, aclYellow, aclCyan, aclGreen, aclMagenta, aclRed, aclBlue, aclBlack,
	};

	SePos pos(0, 0);
	auto windowSize = getWindowSize(window);
	SeSize size(windowSize.width / ARRAY_SIZE(colors), windowSize.height);
	for(unsigned i = 0; i < ARRAY_SIZE(colors); ++i) {
		auto color = scaleColor(colors[i], intensity);
		bltSolidFill(window, pos, size, color);
		pos.x += size.width;
	}
}

unsigned int S1D13781_gfx::drawTextTransparent(WindowDestination window, const SeFont& font, const char* text, int X,
											   int Y, unsigned int width, SeColor fgColor, bool wordCrop, bool* cropped)
{
	// Optimise DrawPixel by caching parameters on first call
	bool newPixelDraw = true;

	SeRect rcWin(getWindowSize(window));

	//initialize some values that we need
	unsigned displayWidth = (width == 0) ? rcWin.width : width;
	SeRect rcChar;
	rcChar.height = font.getHeight();
	rcChar.x = X;
	rcChar.y = Y;
	int yStart = (Y >= 0) ? 0 : -Y;
	int yEnd = (Y + rcChar.height < rcWin.height) ? rcChar.height : rcWin.height - Y;

	// Determine how many characters to draw.
	unsigned nChars = font.measureText(text, displayWidth, wordCrop, cropped);
	unsigned nCharsToDraw = nChars;
	while(nCharsToDraw > 0 && (text[nCharsToDraw - 1] == ' ' || text[nCharsToDraw - 1] == '\t')) {
		nCharsToDraw--;
	}

	fgColor = lookupColor(window, fgColor);
	int xo, yo;
	for(unsigned iText = 0; iText < nCharsToDraw; iText++) {
		rcChar.width = font.getCharWidth(text[iText]);

		if(rcWin.overlap(rcChar)) {
			int xStart = (rcChar.x >= 0) ? 0 : -rcChar.x;
			int xEnd = (rcChar.x2() < int(rcWin.width)) ? rcChar.width : rcWin.width - rcChar.x;
			SeCharOffset offset = font.getCharOffset(wchar_t(uint8_t(text[iText])));

			if(rcChar.width > 0) {
				if(offset.X != seNO_GLYPH) {
					// Draw character glyph.
					for(yo = yStart; yo < yEnd; yo++) {
						uint32_t w = font.getGlyphBits(offset, yo);
						w <<= xStart;
						for(xo = xStart; xo < xEnd; xo++) {
							if(w & 0x80000000) {
								drawPixel(window, rcChar.x + xo, rcChar.y + yo, fgColor, newPixelDraw);
								newPixelDraw = false;
							}
							w <<= 1;
						}
					}
				}
			}
		}

		rcChar.x += rcChar.width;
	}

	return nChars;
}

/*
 * @todo If bgColor is specified then this whole thing can be made considerably faster using burst writes,
 * preparing each line of pixels in a buffer. The S1D13781 has a BLT operation with colour translation but we'd need
 * some free VRAM to store the font bitmap data. We only need 1 bit per pixel for this, so 100 bytes is all we'd need.
 * If PIP isn't enabled we can use the LUT2 memory (1024 bytes or 8192 bits). Procedure is:
 * 	- For each display line of text:
 * 		- Copy font bitmap data into temporary RAM buffer
 * 		- Burst write the RAM buffer into LUT2
 * 		- Perform a BLT operation
 * Using the SPI interrupt mode the transfers can be queued to happen in the background whilst we're preparing the
 * next line of data.
 *
 * First step: Buffer full pixels and just use burst write, needs maximum 2400 bytes.
 */
unsigned int S1D13781_gfx::drawText(WindowDestination window, const SeFont& font, const char* text, int X, int Y,
									unsigned int width, SeColor fgColor, SeColor bgColor, bool wordCrop, bool* cropped)
{
	if(bgColor == aclTransparent) {
		return drawTextTransparent(window, font, text, X, Y, width, fgColor, wordCrop, cropped);
	}

	// Determine visible area
	SeSize windowSize = getWindowSize(window);
	unsigned displayWidth = (width == 0) ? windowSize.width : width;
	// Determine how many characters to draw
	unsigned nChars = font.measureText(text, displayWidth, wordCrop, cropped);
	unsigned nCharsToDraw = nChars;
	//	while(nCharsToDraw > 0) {
	//		char c = text[nCharsToDraw - 1];
	//		if(c == ' ' || c == '\t' || c == '\r' || c == '\n') {
	//			--nCharsToDraw;
	//		} else {
	//			break;
	//		}
	//	}

	// Use a rect to maintain the position and size of the character being drawn
	SeRect rcChar;
	rcChar.height = font.getHeight();
	rcChar.y = Y;
	unsigned yStartOffset = (Y >= 0) ? 0 : unsigned(-Y);
	unsigned yEndOffset = (rcChar.y2() < windowSize.height) ? rcChar.height : unsigned(windowSize.height - Y);

	// Built text in buffer, 1 bit per pixel
	BitBuffer buffer;
	buffer.initialise(displayWidth, rcChar.height);

	for(unsigned yOffset = yStartOffset; yOffset < yEndOffset; ++yOffset) {
		rcChar.x = X;
		for(unsigned iText = 0; iText < nCharsToDraw; ++iText) {
			rcChar.width = font.getCharWidth(text[iText]);
			if(rcChar.width == 0) {
				continue;
			}

			if(rcChar.x2() > windowSize.width) {
				//				debug_e("CHAR OVERFLOW, %u > %u", rcChar.x2(), windowSize.width);
				break;
			}

			SeCharOffset charOffset = font.getCharOffset(wchar_t(text[iText]));
			if(charOffset.X == seNO_GLYPH) {
				buffer.skip(rcChar.width);
			} else {
				uint32_t w = font.getGlyphBits(charOffset, yOffset);
				for(unsigned xOffset = 0; xOffset < rcChar.width; ++xOffset) {
					buffer.setPixel(w & 0x80000000);
					w <<= 1;
				}

				//				for(unsigned xOffset = 0; xOffset < rcChar.width; ++xOffset) {
				//					buffer.setPixel(font.testGlyphPixel(charOffset, xOffset, yOffset));
				//				}
			}

			rcChar.x += rcChar.width;
		}
	}

	//		buffer.print("T", vramAddress);

	uint32_t srcAddr = S1D13781_LUT1_BASE - buffer.getPos();
	write(srcAddr, buffer.getPtr(), buffer.getPos());

	//	if(srcBufPos > srcBufSize) {
	//		debug_e("SRCBUF OVERFLOW: %u, %u", srcBufPos, srcBufSize);
	//	}

	//	m_printHex("scratch", buffer, bufPos);

	bltMoveExpand(window, srcAddr, SePos(X, Y), SeSize(rcChar.x - X, rcChar.height), fgColor, bgColor);

	//	debug_i("drawText() - %u", nChars);

	return nChars;
}

unsigned int S1D13781_gfx::drawMultiLineText(WindowDestination window, const SeFont& font, const char* text, int X,
											 int Y, unsigned int width, SeColor fgColor, SeColor bgColor, bool wordCrop,
											 bool* cropped, unsigned int* linesDrawn)
{
	bool lastLineCropped = false;
	unsigned int fontHeight = font.getHeight();
	uint16_t windowHeight = getHeight(window);
	int y = Y;
	unsigned i = 0;
	while(text[i] != '\0') {
		if(y > (int)windowHeight) {
			lastLineCropped = true;
			break;
		}

		i += drawText(window, font, text + i, X, y, width, fgColor, bgColor, wordCrop, cropped);
		y += fontHeight;
	}

	if(cropped != nullptr) {
		*cropped = lastLineCropped;
	}
	if(linesDrawn != nullptr) {
		*linesDrawn = (y - Y) / fontHeight;
	}

	return i;
}

/*
unsigned int S1D13781_gfx::drawMultiLineTextW(WindowDestination window, const SeFont& font, const wchar_t* text, int X,
											  int Y, unsigned int width, SeColor fgColor, SeColor bgColor,
											  bool wordCrop, bool* cropped, unsigned int* linesDrawn)
{
	bool lastLineCropped = false;
	unsigned int fontHeight = font.getHeight();
	int y = Y;
	uint16_t windowHeight = getHeight(window);

	unsigned i;
	for(i = 0; text[i] != 0; y += fontHeight) {
		if(y > (int)windowHeight) {
			lastLineCropped = true;
			break;
		}

		i += drawTextW(window, font, text + i, X, y, width, fgColor, bgColor, wordCrop, cropped);
	}

	if(cropped != nullptr) {
		*cropped = lastLineCropped;
	}
	if(linesDrawn != nullptr) {
		*linesDrawn = (y - Y) / fontHeight;
	}

	return i;
}

*/

void S1D13781_gfx::drawImage(const FSTR::ObjectBase& image, WindowDestination window, int x, int y, unsigned imageWidth,
							 unsigned imageHeight)
{
	unsigned xs = 1;
	unsigned ys = 1;

	// Assume 24-bit RGB source data
	PixelBuffer sourceBuffer;
	if(!sourceBuffer.initialise(imageWidth, 1, format_RGB_888)) {
		return;
	}

	PixelBuffer destBuffer;
	if(!destBuffer.initialise(imageWidth * xs, 1, getColorDepth(window))) {
		return;
	}

	unsigned windowStride = getStride(window);
	unsigned vramAddress = getStartAddress(window) + (y * windowStride) + (x * destBuffer.getBytesPerPixel());
	unsigned imageStride = sourceBuffer.getStride();
	for(unsigned y = 0; y < imageHeight; ++y) {
		image.read(y * imageStride, static_cast<char*>(sourceBuffer.getPtr()), imageStride);
		for(unsigned x = 0; x < imageWidth; ++x) {
			auto color = sourceBuffer.getPixel(x, 0);
			color = HSPI::bswap24(color);
			color = lookupColor(window, color);
			for(unsigned i = 0; i < xs; ++i) {
				destBuffer.setPixel((x * xs) + i, 0, color);
			}
		}

		for(unsigned i = 0; i < ys; ++i) {
			write(vramAddress, destBuffer.getPtr(), destBuffer.getStride());
			vramAddress += windowStride;
		}
	}
}

//TODO add to future versions of library

/*
 uint16_t S1D13781_gfx::drawImage()
 {
 ;
 }
 */

uint16_t S1D13781_gfx::copyArea(WindowDestination srcWindow, WindowDestination destWindow, SeRect area, int destX,
								int destY)
{
	//check for invalid window error
	if(((srcWindow != window_Main) && (srcWindow != window_Pip)) ||
	   ((destWindow != window_Main) && (destWindow != window_Pip))) {
		return 1;
	}

	//get some window information
	ImageDataFormat srcFormat = getColorDepth(srcWindow);
	ImageDataFormat destFormat = getColorDepth(destWindow);

	//if window formats are the same, use BitBLT function
	if((srcWindow == destWindow) && (srcFormat == destFormat)) {
		return _bitBLTRegion(srcWindow, area, destX, destY);
	} else {
		// Surface formats are different, use simply copy
		return _copyRegion(srcWindow, destWindow, area, destX, destY);
	}
}

uint16_t S1D13781_gfx::_bitBLTRegion(WindowDestination window, const SeRect& area, int destX, int destY)
{
	// Check to see if regions are overlapping in a way that requires a reverse copy
	BltCommand command;
	SePos srcPos, dstPos;
	if((destY >= area.y) && area.overlap(SeRect(destX, destY, area.width, area.height))) {
		command = bltcmd_MoveNegative;
		srcPos.set(area.x + area.width, area.y + area.height - 1);
		dstPos.set(destX + area.width, destY + area.height - 1);
	} else {
		command = bltcmd_MovePositive;
		srcPos.set(area.x, area.y);
		dstPos.set(destX, destY);
	}

	return bltMove(window, command, srcPos, dstPos, SeSize(area.width, area.height)) ? 0 : 1;
}

uint16_t S1D13781_gfx::_copyRegion(WindowDestination srcWindow, WindowDestination destWindow, SeRect area,
								   int16_t destX, int16_t destY)
{
	int16_t xStart = 0;
	int16_t yStart = 0;
	int xInc = 1;
	int yInc = 1;
	int16_t X, Y; //loop vars
	int16_t xCount, yCount;

	uint16_t returnValue = 0; //default return is 0

	uint16_t srcStride = getStride(srcWindow);
	uint16_t destStride = getStride(destWindow);

	if(srcStride == 0 || destStride == 0) {
		return 1; //invalid window error
	}

	//get some window information and initialize some values
	ImageDataFormat srcFormat = getColorDepth(srcWindow);
	uint32_t srcStartAddr = getStartAddress(srcWindow);
	uint16_t srcBytesPP = getBytesPerPixel(srcWindow);

	ImageDataFormat destFormat = getColorDepth(destWindow);
	uint32_t destStartAddr = getStartAddress(destWindow);

	// Calculate width / height based on destination
	uint16_t width = getWidth(destWindow);
	width = destX + area.width > int(width) ? uint16_t(width - destX) : area.width;

	uint16_t height = getHeight(destWindow);
	height = destY + area.height > int(height) ? uint16_t(height - destY) : area.height;

	int16_t xEnd = width;
	int16_t yEnd = height;

	// Check to see if regions are overlapping in a way that requires a reverse copy
	SeRect r1(area.x + xStart, area.y + yStart, xEnd - xStart + 1, yEnd - yStart + 1);
	SeRect r2(destX + xStart, destY + yStart, xEnd - xStart + 1, yEnd - yStart + 1);
	if(srcWindow == destWindow && (destY >= area.y) && r1.overlap(r2)) {
		seSwap(xStart, xEnd);
		seSwap(yStart, yEnd);
		xStart--;
		xEnd--;
		yStart--;
		yEnd--;
		xInc = -xInc;
		yInc = -yInc;
	}

	//if window formats are the same, simply copy the image data
	if(srcFormat == destFormat) {
		unsigned rowCopyBytes = srcBytesPP * width; //how many bytes to copy
		unsigned srcByteOffset, destByteOffset;

		//copy each line a stride at a time using a line buffer to increase performance.
		srcByteOffset = srcBytesPP * area.x;
		destByteOffset = srcBytesPP * destX;
		//allocate line buffer memory
		auto lineOfPixelData = new uint8_t[rowCopyBytes]; //allocate memory
		if(lineOfPixelData == nullptr) {
			return 2; // memory alloc failed
		}

		//now copy the image data
		for(yCount = height, Y = yStart; yCount > 0; yCount--, Y += yInc) {
			read(srcStartAddr + ((area.y + Y) * srcStride) + srcByteOffset, lineOfPixelData, rowCopyBytes);
			write(destStartAddr + ((destY + Y) * destStride) + destByteOffset, lineOfPixelData, rowCopyBytes);
		}

		delete[] lineOfPixelData;

	} else {
		// Surface formats are different, so do a slow pixel-by-pixel copy.
		//options are from RGB888 to RGB565 and vice-versa
		SeColor color;

		// Optimise DrawPixel by caching parameters on first call
		bool newPixelDraw = true;

		if(srcFormat == format_RGB_888) { //go RGB888 to RGB565
			for(yCount = height, Y = yStart; yCount > 0; yCount--, Y += yInc) {
				for(xCount = width, X = xStart; xCount > 0; xCount--, X += xInc) {
					color = getPixel(srcWindow, area.x + X,
									 area.y + Y); //get pixel value
					color = ((color & 0x00F80000) >> 8) | ((color & 0x0000FC00) >> 5) |
							((color & 0x000000F8) >> 3); //transform color
					returnValue = drawPixel(destWindow, destX + X, destY + Y, color, newPixelDraw);
					if(returnValue != 0) {
						return returnValue;
					}
					newPixelDraw = false;
				}
			}
		} else { //go RGB565 to RGB888
			for(yCount = height, Y = yStart; yCount > 0; yCount--, Y += yInc) {
				for(xCount = width, X = xStart; xCount > 0; xCount--, X += xInc) {
					color = getPixel(srcWindow, area.x + X,
									 area.y + Y); //get pixel value
					color = ((color & 0x0000F800) << 8) | ((color & 0x0000E000) << 3) | ((color & 0x000007E0) << 5) |
							((color & 0x00000600)) | ((color & 0x0000001F) << 3) |
							((color & 0x0000001C) >> 2); //transform color
					returnValue = drawPixel(destWindow, destX + X, destY + Y, color, newPixelDraw);
					if(returnValue != 0) {
						return returnValue;
					}
					newPixelDraw = false;
				}
			}
		}
	}

	return 0;
}
