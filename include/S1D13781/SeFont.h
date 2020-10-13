/* ======================================================================
 * SeFont.h
 * Header file for SeFont implementation
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

#include <WString.h>

#define seUNDEFINED 0xFFFF
#define seNO_GLYPH 0xFFFE

struct SeCharOffset {
	uint16_t X; // X offset into the image buffer (if 0xFFFF, then this character has no pixels).
	uint16_t Y; // Y offset into the image buffer.
};

struct SeIsoLatin1Char {
	SeCharOffset offset[256]; // Array of definitions for character below 256.
	uint8_t* width = nullptr; // Array of widths for characters below 256 (=NULL for non-porportional fonts).
};

struct SeCharDetails {
	wchar_t charcode;	// Character code.
	uint8_t width;		 // Width of the character.
	SeCharOffset offset; // X,Y offset into the image buffer.
};

struct FontDef {
	const FlashString* indexData;
	const void* imageData;
	char name[14];
	uint8_t width;
	uint8_t height;
};

/*
 * This class is a container into which a single font may be loaded.
 */
class SeFont
{
public:
	~SeFont()
	{
		unload();
	}

	bool load(const FontDef& fontDef);
	void unload();
	unsigned int measureText(const char* text, unsigned int width, bool wordCrop, bool* cropped) const;
	unsigned int measureTextW(const wchar_t* text, unsigned int width, bool wordCrop, bool* cropped) const;
	const char* getName() const
	{
		return def.name;
	}
	unsigned int getHeight() const
	{
		return def.height;
	}
	unsigned int getCharWidth(char character) const;
	unsigned int getCharWidthW(wchar_t character) const;
	unsigned int getTextWidth(const char* text, unsigned int textLen = 0) const;
	unsigned int getTextWidthW(const wchar_t* text, unsigned int textLen) const;
	unsigned int captureFontIndexFile(uint8_t* dFileBuffer, unsigned int dFileBufferSize, bool binaryData);
	SeCharOffset getCharOffset(wchar_t character) const;

	bool isDefined(wchar_t character) const
	{
		return (character < 256) && (isoLatin1Chars.offset[character].X != seUNDEFINED);
	}

	/** @brief Get a line of bits for the glyph
	 *  @retval uint32_t bits are MSB first, so bit 31 represents first pixel
	 *  @note Bits are packed into memory. For large fonts we cannot load the entire set into RAM,
	 *  so instead just read a single scanline.
	 */
	uint32_t getGlyphBits(const SeCharOffset& offset, unsigned row) const;

private:
	unsigned parseImageData(const FlashString& imageData);

	bool _captureFontIndexCharacter(uint8_t* dFileBuffer, unsigned int dFileBufferSize, unsigned int* iBuf,
									wchar_t character, bool binaryData);
	bool _parseFontIndices(const uint8_t* indexData, unsigned int indexDataSize, unsigned int indexDataStart,
						   bool indexDataIsBinary, bool setData);
	bool _pnmParseHeader(const uint8_t* sFileBuffer, unsigned int sFileBufferLen, unsigned int* parameters,
						 unsigned int* nParameters, unsigned int* dataStart);
	const SeCharDetails* _lookupExtendedChar(wchar_t character) const;

private:
	FontDef def;
	uint16_t imageWidth = 0;				 // Width of the image buffer.
	uint16_t imageHeight = 0;				 // Height of the image buffer.
	uint16_t imageStride = 0;				 // Stride of the image buffer.
	uint16_t nExtendedChars = 0;			 // Number of character definitions beyond 256.
	wchar_t defaultChar = 0;				 // Shown for characters not defined in the font file.
	SeCharDetails** extendedChars = nullptr; // Array of character definitions beyond 256 (see NOTE below).
	SeIsoLatin1Char isoLatin1Chars;			 // Array of character definitions for the first 256 (see NOTE below).
};
