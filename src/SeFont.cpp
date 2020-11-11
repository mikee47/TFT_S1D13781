/* ======================================================================
 * SeFont.cpp
 *
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

#include "include/S1D13781/SeFont.h"
#include "stringutil.h"

#define seSwap(A, B)                                                                                                   \
	{                                                                                                                  \
		auto T = (A);                                                                                                  \
		(A) = (B);                                                                                                     \
		(B) = T;                                                                                                       \
	}

#define seSkipChar(Buf, BufSize, i, c)                                                                                 \
	while((i) < (BufSize) && (Buf)[(i)] == (c))                                                                        \
	(i)++
#define seSkipChars2(Buf, BufSize, i, c1, c2)                                                                          \
	while((i) < (BufSize) && ((Buf)[(i)] == (c1) || (Buf)[(i)] == (c2)))                                               \
	(i)++
#define seSkipChars3(Buf, BufSize, i, c1, c2, c3)                                                                      \
	while((i) < (BufSize) && ((Buf)[(i)] == (c1) || (Buf)[(i)] == (c2) || (Buf)[(i)] == (c3)))                         \
	(i)++
#define seSkipChars4(Buf, BufSize, i, c1, c2, c3, c4)                                                                  \
	while((i) < (BufSize) && ((Buf)[(i)] == (c1) || (Buf)[(i)] == (c2) || (Buf)[(i)] == (c3) || (Buf)[(i)] == (c4)))   \
	(i)++

#define seFindChar(Buf, BufSize, i, c)                                                                                 \
	while((i) < (BufSize) && (Buf)[(i)] != (c))                                                                        \
	(i)++
#define seFindChars2(Buf, BufSize, i, c1, c2)                                                                          \
	while((i) < (BufSize) && (Buf)[(i)] != (c1) && (Buf)[(i)] != (c2))                                                 \
	(i)++
#define seFindChars3(Buf, BufSize, i, c1, c2, c3)                                                                      \
	while((i) < (BufSize) && (Buf)[(i)] != (c1) && (Buf)[(i)] != (c2) && (Buf)[(i)] != (c3))                           \
	(i)++
#define seFindChars4(Buf, BufSize, i, c1, c2, c3, c4)                                                                  \
	while((i) < (BufSize) && (Buf)[(i)] != (c1) && (Buf)[(i)] != (c2) && (Buf)[(i)] != (c3) && (Buf)[(i)] != (c4))     \
	(i)++

#define seIsHexDigit(c) (((c) >= '0' && (c) <= '9') || ((c) >= 'A' && (c) <= 'F') || ((c) >= 'a' && (c) <= 'f'))

/* _strLen()
 * Private method to return the length of a string of characters.
 *
 * param	str		String of characters.
 *
 * return
 * - number of characters in the string.
 *
 */
static unsigned _strLen(const char* str)
{
	unsigned i = 0;

	while(str[i] != '\0') {
		i++;
	}

	return i;
}

/* _strLenW()
 * Private method to return the length of a string of wide characters
 * (in this case wchar_t).
 *
 * param	str		String of wide characters.
 *
 * return
 * - number of wide characters in the string.
 *
 */
//static unsigned _strLenW(const wchar_t* str)
//{
//	unsigned i = 0;
//
//	while(str[i] != (wchar_t)0) {
//		i++;
//	}
//
//	return i;
//}

/* _strToUInt()
 * Private method to convert a string of characters into the numerical
 * equivalent. This function is used similar to atoi().
 *
 * param	str			String of characters to search.
 *
 * param	strSize		Maximum length of the string buffer.
 *
 * param	iStr		Index into str of where to start. This parameter
 * 						will be incremented to character after integer.
 *
 * param	radix		The radix to parse (2=BIN, 8=OCT, 10=DEC, 16=HEX).
 *
 * param	value		The return value--the integer that was parsed.
 *
 * param	badBreak	TRUE if an unexpected character was encountered
 * 						(expected characters are like whitespace or
 * 						line-terminating characters).
 *
 * return
 * - true if an integer is parsed.
 * - false if failed to parse an integer.
 *
 */
static bool _strToUInt(const char* str, unsigned strSize, unsigned* iStr, unsigned radix, unsigned* value,
					   bool* badBreak)
{
	bool numberFound = false;

	*value = 0;
	*badBreak = false;
	seSkipChars2(str, strSize, *iStr, ' ', '\t');

	while(*iStr < strSize) {
		char c = str[*iStr];
		if((c >= '0' && c <= '1') || (radix > 2 && c >= '2' && c <= '8') || (radix > 8 && c == '9')) {
			numberFound = true;
			*value = (*value * radix) + (c - '0');
		} else if(radix == 16 && c >= 'a' && c <= 'f') {
			numberFound = true;
			*value = (*value * radix) + (c - 'a' + 10);
		} else if(radix == 16 && c >= 'A' && c <= 'F') {
			numberFound = true;
			*value = (*value * radix) + (c - 'A' + 10);
		} else if(c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\0') {
			break;
		} else {
			*badBreak = true; // Unexpected character found.
			break;
		}
		(*iStr)++;
	}

	return numberFound;
}

/* _uIntToStr()
 * Private method to convert a unsigned to the equivalent
 * alphabetical string. This function is used similar to itoa(). A '\0'
 * character is placed at the end of the string.
 *
 * param	value	The number to write into the string.
 *
 * param	str		The destination string to write into.
 *
 * param	strSize	Maximum length of the string buffer.
 *
 * param	iStr	Index into Str of where to start. This parameter
 * 					will be incremented to character after number is
 * 					written.
 *
 * param	radix	The radix to parse (2=BIN, 8=OCT, 10=DEC, 16=HEX).
 *
 * return
 * - Returns a reference to the start of the number in the string.
 *
 */
static char* _uIntToStr(unsigned value, char* str, unsigned strSize, unsigned* iStr, unsigned radix)
{
	unsigned iStart, iEnd;

	for(iStart = *iStr; value > 0; (*iStr)++) {
		if(*iStr >= strSize) {
			str[iStart] = '\0';
			*iStr = iStart;
			return str + iStart;
		}

		unsigned n = value % radix;
		value = value / radix;
		str[*iStr] = hexchar(n);
	}

	str[*iStr] = '\0';

	// Reverse the string now.
	iEnd = *iStr - 1;
	while(iStart < iEnd) {
		seSwap(str[iStart], str[iEnd]);
	}

	return str + iStart;
}

/* createFont()
 * Method to create the structures needed to draw a simple raster font
 * from the contents of an image file and an index font file.
 * 
 * To reduce memory footprint, font routines will refer to the ImageData
 * buffer when drawing text. This routine will not allocate and copy
 * this buffer for future use, so do NOT free the buffer you passed in
 * as ImageData. There is allocation made for the font index
 * information, so be sure to call the unload() routine to free
 * resources when you exit the application. You can free the IndexData
 * buffer after calling this routine.
 * 
 * param	imageData		The contents of a font image file (.pbm).
 * 							See NOTE below.
 * 
 * param	imageDataSize	Size of the contents in the image file.
 * 
 * param	indexData		The contents of a font index file (.pfi).
 * 							See NOTE below.
 * 
 * param	indexDataSize	Size of the contents in the index file.
 * 
 * return
 * - Returns a handle to the font created.
 * 
 * NOTE:
 * ImageData must point to a Non-ASCII "P4" .pbm image file.  It must be
 * the P4 binary format as the font routine will directly refer to the
 * ImageData buffer that you provided.  The IndexData can be either
 * ASCII (F1) or Binary (F4).  For more information about the F1 and F4
 * formats, please refer to the README.txt file.
 * 
 */
bool SeFont::load(const FontDef& fontDef)
{
	/*
	auto imageBuffer = new uint8_t[fontDef.imageData.size()];
	memcpy_P(imageBuffer, fontDef.imageData.data(), fontDef.imageData.size());
	LOAD_FSTR(indexBuffer, fontDef.indexData);
	return load(imageBuffer, fontDef.imageData.length(), reinterpret_cast<uint8_t*>(indexBuffer),
				fontDef.indexData.length());

*/
	unload();

	memcpy_P(&def, &fontDef, sizeof(def));

	if(def.indexData == nullptr) {
		imageWidth = def.width;
		imageHeight = def.height * 256; // 256 characters
		imageStride = (imageWidth + 7) / 8;
		defaultChar = 0;

		// We'll assume for now that each line of image data contains one character
		unsigned y = 0;
		for(auto& off : isoLatin1Chars.offset) {
			if(y >= imageHeight) {
				off.X = off.Y = seUNDEFINED;
			} else {
				off.X = 0;
				off.Y = y;
				y += def.height;
			}
		}

		return true;
	}

	unsigned dataStart = parseImageData(*static_cast<const FlashString*>(def.imageData));
	if(dataStart == 0) {
		return false;
	}

	LOAD_FSTR(indexData, *def.indexData);
	unsigned indexDataSize = def.indexData->length();

	// Only IndexData of F1 or F4 are supported
	if(indexDataSize < 2 || indexData[0] != 'F' || (indexData[1] != '1' && indexData[1] != '4')) {
		return false;
	}

	// Parse index header (the first three lines).
	bool isBinary = false;
	unsigned iLine;
	unsigned iData = 0;
	for(iLine = 0; iLine < 3; iLine++) {
		while(indexData[iData] == '#') {
			// Ignore line.
			seFindChars2(indexData, indexDataSize, iData, '\n', '\r');
			seSkipChars2(indexData, indexDataSize, iData, '\n', '\r');
		}

		if(iLine == 0) {
			// Ignore line.
			seFindChars2(indexData, indexDataSize, iData, '\n', '\r');
			seSkipChars2(indexData, indexDataSize, iData, '\n', '\r');
		} else if(iLine == 1) {
			// Process font name.
			unsigned iName = iData;

			seFindChars2(indexData, indexDataSize, iData, '\n', '\r');
			iData = iName;
			for(iName = 0; indexData[iData] != '\n' && indexData[iData] != '\r'; iData++, iName++) {
				def.name[iName] = indexData[iData];
			}
			def.name[iName] = '\0';
			seSkipChars2(indexData, indexDataSize, iData, '\n', '\r');
		} else {
			// Process font width.
			if(indexData[iData] == '?') {
				// Proportional font.
				def.width = 0;
				isoLatin1Chars.width = new uint8_t[256];
				if(isoLatin1Chars.width == nullptr) {
					unload();
					return false;
				}
				for(unsigned i = 0; i < 256; i++) {
					isoLatin1Chars.width[i] = 0;
				}
				iData++;
			} else { // Non-proportional font.
				unsigned value;
				bool badBreak;
				if(!_strToUInt((const char*)indexData, indexDataSize, &iData, 10, &value, &badBreak) || !badBreak) {
					unload();
					return false;
				}
				def.width = value;
			}

			// Skip 'x'.
			seSkipChar(indexData, indexDataSize, iData, 'x');

			// Process font height.
			if(iData < indexDataSize) {
				unsigned value;
				bool badBreak;
				if(!_strToUInt((const char*)indexData, indexDataSize, &iData, 10, &value, &badBreak) || badBreak) {
					unload();
					return false;
				}
				def.height = (uint8_t)value;
			}
			seSkipChars4(indexData, indexDataSize, iData, ' ', '\t', '\n', '\r');

			// Determine data start.
			if(iData + 1 >= indexDataSize) {
				unload();
				return false;
			}
			if(indexData[iData] == 0xFF) {
				isBinary = true;
				iData++;
			}
			dataStart = iData;
			break;
		}
	}
	for(unsigned i = 0; i < 256; i++) {
		isoLatin1Chars.offset[i].X = isoLatin1Chars.offset[i].Y = seUNDEFINED;
	}

	// Count the number of extended characters.
	if(!_parseFontIndices(reinterpret_cast<uint8_t*>(indexData), indexDataSize, dataStart, isBinary, false)) {
		unload();
		return false;
	}

	// Allocate the exact memory needed for the number of extended characters found.
	if(nExtendedChars > 0) {
		extendedChars = new SeCharDetails*[nExtendedChars];
		if(extendedChars == nullptr) {
			unload();
			return false;
		}

		for(unsigned i = 0; i < nExtendedChars; i++) {
			extendedChars[i] = nullptr;
		}
	}

	// Re-parse index data and set the internal arrays.
	if(!_parseFontIndices(reinterpret_cast<uint8_t*>(indexData), indexDataSize, dataStart, isBinary, true)) {
		unload();
		return false;
	}

	return true;
}

unsigned SeFont::parseImageData(const FlashString& imageData)
{
	// Load information and parse - we only need header info. from data buffer
	uint8_t header[128];
	imageData.read(0, reinterpret_cast<char*>(header), sizeof(header));

	// Only ImageData of P1 or P4 are supported
	if(header[0] != 'P' || (header[1] != '1' && header[1] != '4')) {
		return 0;
	}

	// Parse image data.
	unsigned parameters[3];
	unsigned nParameters = 0;
	unsigned dataStart = 0;
	if(_pnmParseHeader(header, sizeof(header), parameters, &nParameters, &dataStart) == false) {
		unload();
		return false;
	}

	imageWidth = parameters[0];
	imageHeight = parameters[1];
	//TODO: update to accomodate various formats (for now only 1bpp)
	imageStride = imageWidth / 8;
	if(imageWidth % 8) {
		++imageStride;
	}
	def.imageData = reinterpret_cast<const uint8_t*>(imageData.data()) + dataStart;

	return dataStart;
}

/* unload()
 * Method to invalidate the specified font and free the system resources
 * associated with it.
 * 
 * param	font	The font to free.
 * 
 * return
 * - nullptr
 * 
 */
void SeFont::unload()
{
	delete[] isoLatin1Chars.width;
	isoLatin1Chars.width = nullptr;

	if(extendedChars != nullptr) {
		for(unsigned i = 0; i < nExtendedChars; i++) {
			delete extendedChars[i];
			extendedChars[i] = nullptr;
		}
		delete[] extendedChars;
		extendedChars = nullptr;
		nExtendedChars = 0;
	}

	imageWidth = imageHeight = imageStride = 0;
	memset(&def, 0, sizeof(def));
	defaultChar = 0;
}

/* measureText()
 * Method to measure text containing "Chars" in pixels, and returns the
 * number of characters that can be shown in the given width.
 * 
 * param	font		The font that will be used for the text
 * param	text		The text to be measured.
 * param	width		The cropping area width (in pixels).
 * param	wordCrop	True if cropping should be done on uint16_t boundaries.
 * param	cropped		Returns True if text would be cropped (can be set to nullptr).
 * 
 * return
 * - The number of characters that would be drawn.
 * 
 */
unsigned SeFont::measureText(const char* text, unsigned width, bool wordCrop, bool* cropped) const
{
	if(text == nullptr) {
		return 0;
	}

	unsigned pixelWidth = 0;
	unsigned i = 0;

	if(cropped != nullptr) {
		*cropped = false;
	}

	char c;
	while((c = text[i]) != '\0') {
		if(c == '\r') {
			++i;
			if(text[i] == '\n') {
				++i;
			}
			break;
		}
		if(c == '\n') {
			++i;
			if(text[i] == '\r') {
				++i;
			}
			break;
		}
		pixelWidth += getCharWidth(c);
		if(pixelWidth > width) {
			if(cropped != nullptr) {
				*cropped = true;
			}
			if(wordCrop) {
				// Move to last uint16_t break.
				while(i > 0) {
					c = text[i];
					if(c == ' ' || c == '\t') {
						do {
							++i;
							c = text[i];
						} while(c == ' ' || c == '\t');
						break;
					}
					if(text[i - 1] == '-') {
						break;
					}
					i--;
				}
			}

			return i;
		}
		i++;
	}

	return i;
}

/* measureTextW()
 * Method to measure text containing "Wchars" in pixels, and returns the
 * number of characters that can be shown in the given width.
 * 
 * param	font		The font that will be used for the text. The
 * 						Font information must have been previously
 * 						created with createFont().
 * 
 * param	text		The text to be measured.
 * 
 * param	width		The cropping area width (in pixels).
 * 
 * param	wordCrop	True if cropping should be done on uint16_t
 * 						boundaries.
 * 
 * param	cropped		Returns True if text would be cropped (can be
 * 						set to nullptr).
 * 
 * return
 * - The number of characters that would be drawn.
 * 
 */
unsigned SeFont::measureTextW(const wchar_t* text, unsigned width, bool wordCrop, bool* cropped) const
{
	unsigned pixelWidth = 0;
	unsigned i = 0;

	if(cropped != nullptr) {
		*cropped = false;
	}

	while(text[i] != '\0') {
		pixelWidth += getCharWidthW(text[i]);
		if(pixelWidth > width) {
			if(cropped != nullptr) {
				*cropped = true;
			}
			if(wordCrop) {
				// Move to last uint16_t break.
				while(i > 0) {
					if(text[i] == ' ' || text[i] == '\t') {
						while(text[i] == ' ' || text[i] == '\t') {
							i++;
						}
						break;
					}
					if(text[i - 1] == '-') {
						break;
					}
					i--;
				}
			}

			return i;
		}
		i++;
	}

	return i;
}

/* getCharWidth()
 * Method to return the width of the specified "Char" character for a
 * given font, in pixels.
 * 
 * Note: For proportional fonts, the character width may not be the same
 * 		 for all characters.
 * 
 * param	font		The font that will be used for the text. The
 * 						Font information must have been previously
 * 						created with createFont().
 * 
 * param	character	The "Char" character to measure.
 * 
 * return
 * - The width of the character, in pixels.
 * 
 */
unsigned SeFont::getCharWidth(char character) const
{
	if(def.width != 0) {
		return def.width;
	}

	// Proportional font
	if(isoLatin1Chars.offset[(uint8_t)character].X == seUNDEFINED) {
		return getCharWidthW(defaultChar);
	}

	return isoLatin1Chars.width[(uint8_t)character];
}

/* getCharWidthW()
 * Method to return the width of the specified "Wchar" character for a
 * given font, in pixels.
 * 
 * Note: For proportional fonts, the character width may not be the same
 * 		 for all characters.
 * 
 * param	font		The font that will be used for the text. The
 * 						Font information must have been previously
 * 						created with createFont().
 * 
 * param	character	The "Wchar" character to measure.
 * 
 * return
 * - The width of the character, in pixels.
 * 
 */
unsigned SeFont::getCharWidthW(wchar_t character) const
{
	if(def.width != 0) {
		return def.width;
	}

	// Proportional font
	if(character < 256) {
		return getCharWidth((char)character);
	}

	auto p = _lookupExtendedChar(character);
	if(p != nullptr) {
		return p->width;
	}

	return getCharWidthW(defaultChar);
}

/* getTextWidth()
 * Method to return the width of specified text consisting of "Char"
 * characters for a given font, in pixels.
 * 
 * Note: For proportional fonts, the character width may not be the same
 * 		 for all characters.
 * 
 * param	font		The font that will be used for the text. The
 * 						Font information must have been previously
 * 						created with createFont().
 * 
 * param	text		The text consisting of "Char" characters.
 * 
 * param	textLen		The number of characters in the text to be 
 * 						measured.
 * 
 * return
 * - The width of the text, in pixels.
 * 
 */
unsigned SeFont::getTextWidth(const char* text, unsigned textLen) const
{
	if(textLen == 0) {
		textLen = _strLen(text);
	}

	unsigned pixelWidth = 0;
	for(unsigned i = 0; i < textLen; i++) {
		pixelWidth += getCharWidth(text[i]);
	}

	return pixelWidth;
}

/* getTextWidthW()
 * Method to return the width of specified text consisting of "Wchar"
 * characters for a given font, in pixels.
 * 
 * Note: For proportional fonts, the character width may not be the same
 * 		 for all characters.
 * 
 * param	font		The font that will be used for the text. The
 * 						Font information must have been previously
 * 						created with createFont().
 * 
 * param	text		The text consisting of "Wchar" characters.
 * 
 * param	textLen		The number of characters in the text to be 
 * 						measured.
 * 
 * return
 * - The width of the text, in pixels.
 * 
 */
unsigned SeFont::getTextWidthW(const wchar_t* text, unsigned textLen) const
{
	unsigned pixelWidth = 0;
	for(unsigned i = 0; i < textLen; i++) {
		pixelWidth += getCharWidthW(text[i]);
	}

	return pixelWidth;
}

/* captureFontIndexFile()
 * Method to generate the contents for a font index file from a given
 * Font. This method is provided as a convenience to genereate binary
 * versions of the index files (which are smaller than the ASCII
 * versions).
 * 
 * param	font			The font that will be used for the text. The
 * 							Font information must have been previously
 * 							created with createFont().
 * 
 * param	dFileBuffer		The destination buffer that will receive
 * 							the font index file contents.
 * 
 * param	dFileBufferSize	The allocation size of the destination
 * 							buffer.
 * 
 * param	binaryData		Determines if the index data is written in
 * 							binary or ASCII.
 * 
 * return
 * - The the number of bytes written into dBuffer (will return 0 if
 * 	 operation failed)
 * 
 */
unsigned SeFont::captureFontIndexFile(uint8_t* dFileBuffer, unsigned dFileBufferSize, bool binaryData)
{
#define seCaptureByte(b, i, errval)                                                                                    \
	if((i) >= dFileBufferSize) {                                                                                       \
		return errval;                                                                                                 \
	}                                                                                                                  \
	dFileBuffer[(i)++] = (uint8_t)(b)

	unsigned fontNameLen = _strLen(def.name);
	unsigned iBuf = 0;
	unsigned i;

	// F1 or F4
	seCaptureByte('F', iBuf, 0);
	seCaptureByte(binaryData ? '4' : '1', iBuf, 0);
	seCaptureByte('\n', iBuf, 0);

	// Font name
	for(i = 0; i < fontNameLen; i++) {
		seCaptureByte(def.name[i], iBuf, 0);
	}
	seCaptureByte('\n', iBuf, 0);

	// 6x10 or ?x10
	if(def.width == 0) {
		seCaptureByte('?', iBuf, 0);
	} else {
		if(*_uIntToStr(def.width, (char*)dFileBuffer, dFileBufferSize, &iBuf, 10) == '\0') {
			return 0;
		}
	}
	seCaptureByte('x', iBuf, 0);
	if(*_uIntToStr(def.height, (char*)dFileBuffer, dFileBufferSize, &iBuf, 10) == '\0') {
		return 0;
	}
	seCaptureByte('\n', iBuf, 0);

	// Binary marker.
	if(binaryData) {
		seCaptureByte(0xFF, iBuf, 0);
	}

	// Always write out the DefaultChar first.
	if(!_captureFontIndexCharacter(dFileBuffer, dFileBufferSize, &iBuf, defaultChar, binaryData)) {
		return 0;
	}

	// Write out ISO Latin-1 characters.
	for(i = 0; i < 256; i++) {
		if(i != defaultChar) {
			if(!_captureFontIndexCharacter(dFileBuffer, dFileBufferSize, &iBuf, (wchar_t)i, binaryData)) {
				return 0;
			}
		}
	}

	// Write out rest of the Unicode characters.
	for(i = 0; i < nExtendedChars; i++) {
		if(extendedChars[i]->charcode != defaultChar) {
			if(!_captureFontIndexCharacter(dFileBuffer, dFileBufferSize, &iBuf, extendedChars[i]->charcode,
										   binaryData)) {
				return 0;
			}
		}
	}

	return iBuf;
}

/* _captureFontIndexCharacter()
 * Private Method used by the S1D13781 graphics library functions.
 * 
 */
bool SeFont::_captureFontIndexCharacter(uint8_t* dFileBuffer, unsigned dFileBufferSize, unsigned* iBuf,
										wchar_t character, bool binaryData)
{
	unsigned parameters[3];
	unsigned nParameters = 0;
	unsigned i;

	// Get character parameters.
	if(character < 256) {
		if(isoLatin1Chars.offset[character].X == seUNDEFINED) {
			return true;
		}

		if(def.width == 0) {
			parameters[nParameters++] = isoLatin1Chars.width[character];
		}
		if(isoLatin1Chars.offset[character].X != seNO_GLYPH) {
			parameters[nParameters++] = isoLatin1Chars.offset[character].X;
			parameters[nParameters++] = isoLatin1Chars.offset[character].Y;
		}
	} else {
		const SeCharDetails* p = _lookupExtendedChar(character);

		if(p == nullptr) {
			return true;
		}

		if(def.width == 0) {
			parameters[nParameters++] = p->width;
		}
		if(p->offset.X != seNO_GLYPH) {
			parameters[nParameters++] = p->offset.X;
			parameters[nParameters++] = p->offset.Y;
		}
	}

	// Write character record to buffer.
	if(binaryData) {
		seCaptureByte(character >> 8, *iBuf, false);
		seCaptureByte(character & 0x00FF, *iBuf, false);
		seCaptureByte(nParameters, *iBuf, false);
		for(i = 0; i < nParameters; i++) {
			seCaptureByte(parameters[i], *iBuf, false);
		}
	} else {
		if(character >= '!' && character <= '~') {
			seCaptureByte(character, *iBuf, false);
		} else {
			seCaptureByte('0', *iBuf, false);
			if(*_uIntToStr(character, (char*)dFileBufferSize, dFileBufferSize, iBuf, 16) == '\0') {
				return false;
			}
		}
		for(i = 0; i < nParameters; i++) {
			seCaptureByte(' ', *iBuf, false);
			if(*_uIntToStr(parameters[i], (char*)dFileBufferSize, dFileBufferSize, iBuf, 16) == '\0') {
				return false;
			}
		}
	}

	return true;
}

/* _parseFontIndices()
 * Private Method used by the S1D13781 graphics library functions.
 * 
 */
bool SeFont::_parseFontIndices(const uint8_t* indexData, unsigned indexDataSize, unsigned indexDataStart,
							   bool indexDataIsBinary, bool setData)
{
	SeCharDetails c;
	unsigned iData = indexDataStart;
	unsigned parameters[3];
	unsigned nParameters;
	unsigned value;
	bool badBreak;
	unsigned i;

	nExtendedChars = 0;
	while(iData < indexDataSize) {
		if(indexDataIsBinary) {
			if(iData + 3 >= indexDataSize) {
				return false;
			}
			c.charcode = (wchar_t)((uint16_t)indexData[iData] << 9 | (uint16_t)indexData[iData + 1]);
			iData += 2;
			nParameters = indexData[iData++];
			for(i = 0; i < nParameters; i++) {
				parameters[i] = indexData[iData++];
			}
		} else { // Index data is ASCII
			seSkipChars2(indexData, indexDataSize, iData, ' ', '\t');
			if(seIsHexDigit(indexData[iData]) && (iData + 1) < indexDataSize && seIsHexDigit(indexData[iData + 1])) {
				// Character code found.
				if(!_strToUInt((const char*)indexData, indexDataSize, &iData, 16, &value, &badBreak) || badBreak) {
					return false;
				}
				c.charcode = (wchar_t)value;
			} else {
				// Actual character given.
				c.charcode = (wchar_t)indexData[iData++];
			}
			nParameters = 0;
			while(iData < indexDataSize) {
				seSkipChars2(indexData, indexDataSize, iData, ' ', '\t');
				if(indexData[iData] == '\n' || indexData[iData] == '\r') {
					seSkipChars2(indexData, indexDataSize, iData, '\n', '\r');
					break;
				}

				if(nParameters == 3) {
					return false;
				}
				if(!_strToUInt((const char*)indexData, indexDataSize, &iData, 10, &parameters[nParameters++],
							   &badBreak) ||
				   badBreak) {
					return false;
				}
			}
		}

		if(setData) {
			i = 0;
			switch(nParameters) {
			case 0:
				c.width = 0;
				c.offset.X = c.offset.Y = seNO_GLYPH;
				break;

			case 1:
				c.width = (uint8_t)parameters[0];
				c.offset.X = c.offset.Y = seNO_GLYPH;
				break;

			case 2:
				c.width = 0;
				c.offset.X = (uint16_t)parameters[i++];
				c.offset.Y = (uint16_t)parameters[i];
				break;

			case 3:
				c.width = (uint8_t)parameters[i++];
				c.offset.X = (uint16_t)parameters[i++];
				c.offset.Y = (uint16_t)parameters[i];
				break;

			default:
				return false;
			}

			if(c.charcode < 256) {
				isoLatin1Chars.offset[c.charcode].X = c.offset.X;
				isoLatin1Chars.offset[c.charcode].Y = c.offset.Y;
				if(isoLatin1Chars.width != nullptr) {
					isoLatin1Chars.width[c.charcode] = c.width;
				}
			} else {
				extendedChars[nExtendedChars] = new SeCharDetails;
				if(extendedChars[nExtendedChars] == nullptr) {
					return false;
				}
				extendedChars[nExtendedChars]->charcode = c.charcode;
				extendedChars[nExtendedChars]->offset.X = c.offset.X;
				extendedChars[nExtendedChars]->offset.Y = c.offset.Y;
				extendedChars[nExtendedChars]->width = (def.width > 0) ? def.width : c.width;
			}
		}
		if(defaultChar == 0) {
			defaultChar = c.charcode;
		}
		if(c.charcode >= 256) {
			++nExtendedChars;
		}
	}

	return true;
}

/* _lookupExtendedChar()
 * Private Method used by the S1D13781 graphics library functions.
 * 
 */
const SeCharDetails* SeFont::_lookupExtendedChar(wchar_t character) const
{
	if(extendedChars != nullptr) {
		for(unsigned i = 0; i < nExtendedChars; i++) {
			if(extendedChars[i] != nullptr && extendedChars[i]->charcode == character) {
				return extendedChars[i];
			}
		}
	}

	return nullptr;
}

/* getCharOffset()
 *
 * Used by the S1D13781 graphics library functions to determine offset within
 * font image data for the given character.
 * 
 */
SeCharOffset SeFont::getCharOffset(wchar_t character) const
{
	if(character < 256) {
		if(isoLatin1Chars.offset[character].X == seUNDEFINED) {
			return getCharOffset(defaultChar);
		}

		return isoLatin1Chars.offset[character];
	}

	auto p = _lookupExtendedChar(character);
	if(p != nullptr) {
		return p->offset;
	}

	return getCharOffset(defaultChar);
}

/* _pnmParseHeader()
 * Private Method used by the S1D13781 graphics library functions.
 * 
 */
bool SeFont::_pnmParseHeader(const uint8_t* sFileBuffer, unsigned sFileBufferLen, unsigned* parameters,
							 unsigned* nParameters, unsigned* dataStart)
{
	unsigned count = 0;
	unsigned iBuf;
	bool badBreak;

	if(sFileBufferLen < 7 || sFileBuffer[0] != 'P') {
		return false;
	}

	switch(sFileBuffer[1]) {
	case '1':
		*nParameters = 2;
		break;
	case '2':
		*nParameters = 3;
		break;
	case '3':
		*nParameters = 3;
		break;
	case '4':
		*nParameters = 2;
		break;
	case '5':
		*nParameters = 3;
		break;
	case '6':
		*nParameters = 3;
		break;
	default:
		return false;
	}

	iBuf = 3;
	while(count < *nParameters) {
		if(iBuf >= sFileBufferLen) {
			return false;
		}

		switch(sFileBuffer[iBuf]) {
		case '#':
			// Skip comment line.
			seFindChar(sFileBuffer, sFileBufferLen, iBuf, '\n');
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			// Parameter found, parse it.
			if(!_strToUInt((const char*)sFileBuffer, sFileBufferLen, &iBuf, 10, &parameters[count++], &badBreak) ||
			   badBreak) {
				return false;
			}
			break;

		case ' ':
		case '\t':
		case '\n':
			// Skip whitespace.
			iBuf++;
			break;

		default:
			return false;
			break;
		}

		seSkipChars2(sFileBuffer, sFileBufferLen, iBuf, ' ', '\t');
		seSkipChar(sFileBuffer, sFileBufferLen, iBuf, '\n');
	}
	if(dataStart != nullptr) {
		*dataStart = iBuf;
	}

	return (iBuf < sFileBufferLen && count == *nParameters);
}

uint32_t SeFont::getGlyphBits(const SeCharOffset& offset, unsigned row) const
{
	// Get word-aligned address for flash data
	uint32_t dataAddr = reinterpret_cast<uint32_t>(def.imageData) + ((offset.Y + row) * imageStride) + (offset.X / 8);
	unsigned byteOffset = dataAddr & 3;
	auto flashPtr = reinterpret_cast<const uint32_t*>(dataAddr & ~3);
	uint32_t w;
	if(byteOffset == 0) {
		// Data contained within a single word
		union {
			uint32_t w;
			uint8_t b[4];
		} buf = {flashPtr[0]};
		w = (buf.b[0] << 24) | (buf.b[1] << 16) | (buf.b[2] << 8) | buf.b[3];
	} else {
		// Bits straddle a word boundary
		union {
			uint32_t w[2];
			uint8_t b[8];
		} buf = {flashPtr[0], flashPtr[1]};
		// Data contained in max. 3 bytes (for 24 pixel-wide font); this should work
		w = (buf.b[byteOffset] << 24) | (buf.b[byteOffset + 1] << 16) | (buf.b[byteOffset + 2] << 8) |
			buf.b[byteOffset + 3];
	}
	w <<= offset.X % 8;
	return w;
}
