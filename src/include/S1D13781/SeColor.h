/* ======================================================================
 * S1d13781_common.h
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

#include <stdint.h>
#include <sming_attr.h>

/** @brief RGB color value, layout depends on colour depth in use
 *
 * This is a 32-bit unsigned int used as follows:
* - for RGB 8:8:8 modes (24 bpp):
*   - bits 31-24 = unused
*   - bits 23-16 = 8-bits of Red
*   - bits 15-8 = 8-bits of Green
*   - bits 7-0 = 8-bits of Blue
* - for RGB 5:6:5 modes (16 bpp):
*   - bits 31-16 = unused
*   - bits 15-11 = 5-bits of Red
*   - bits 10-5 = 6-bits of Green
*   - bits 4-0 = 5-bits of Blue
* - for 8 bpp modes:
*   - bits 31-8 = unused
*   - bits 7-0 = 8-bit color value
*
* See Hardware Specification for details on the color formats supported by the S1D13781.
*/

//possible image data formats
enum ImageDataFormat {
	format_RGB_888 = 0, //< RGB 8:8:8
	format_RGB_565 = 1, //< RGB 5:6:5
	format_Reserved1 = 2,
	format_Reserved2 = 3,
	format_RGB_888LUT = 4,
	format_RGB_565LUT = 5,
	format_RGB_332LUT = 6, //< RGB 3:3:2 Color
	format_Invalid = 7	 //< Invalid Format
};

static __forceinline uint8_t getBytesPerPixel(ImageDataFormat format)
{
	switch(format) {
	case format_RGB_332LUT:
		return 1;
	case format_RGB_565:
	case format_RGB_565LUT:
		return 2;
	case format_RGB_888:
	case format_RGB_888LUT:
		return 3;
	default:
		return 0;
	}
}

union SeColor {
	uint32_t value;
	struct {
		uint32_t code : 24;
		uint32_t format : 8;
	};

	SeColor(uint32_t value = 0) : value(value)
	{
	}

	SeColor(uint32_t code, ImageDataFormat format) : code(code), format(format)
	{
	}

	operator uint32_t() const
	{
		return value;
	}

	ImageDataFormat getFormat()
	{
		return ImageDataFormat(format);
	}
};

// Common color constants
enum ColorConstants {
	aclAliceBlue = 0x00F0F8FF,
	aclAntiqueWhite = 0x00FAEBD7,
	aclAqua = 0x0000FFFF,
	aclAquamarine = 0x007FFFD4,
	aclAzure = 0x00F0FFFF,
	aclBeige = 0x00F5F5DC,
	aclBisque = 0x00FFE4C4,
	aclBlack = 0x00000000,
	aclBlanchedAlmond = 0x00FFEBCD,
	aclBlue = 0x000000FF,
	aclBlueViolet = 0x008A2BE2,
	aclBrown = 0x00A52A2A,
	aclBurlyWood = 0x00DEB887,
	aclCadetBlue = 0x005F9EA0,
	aclChartreuse = 0x007FFF00,
	aclChocolate = 0x00D2691E,
	aclCoral = 0x00FF7F50,
	aclCornflowerBlue = 0x006495ED,
	aclCornsilk = 0x00FFF8DC,
	aclCrimson = 0x00DC143C,
	aclCyan = 0x0000FFFF,
	aclDarkBlue = 0x0000008B,
	aclDarkCyan = 0x00008B8B,
	aclDarkGoldenrod = 0x00B8860B,
	aclDarkGray = 0x00A9A9A9,
	aclDarkGreen = 0x00006400,
	aclDarkKhaki = 0x00BDB76B,
	aclDarkMagenta = 0x008B008B,
	aclDarkOliveGreen = 0x00556B2F,
	aclDarkOrange = 0x00FF8C00,
	aclDarkOrchid = 0x009932CC,
	aclDarkRed = 0x008B0000,
	aclDarkSalmon = 0x00E9967A,
	aclDarkSeaGreen = 0x008FBC8B,
	aclDarkSlateBlue = 0x00483D8B,
	aclDarkSlateGray = 0x002F4F4F,
	aclDarkTurquoise = 0x0000CED1,
	aclDarkViolet = 0x009400D3,
	aclDeepPink = 0x00FF1493,
	aclDeepSkyBlue = 0x0000BFFF,
	aclDimGray = 0x00696969,
	aclDodgerBlue = 0x001E90FF,
	aclFirebrick = 0x00B22222,
	aclFloralWhite = 0x00FFFAF0,
	aclForestGreen = 0x00228B22,
	aclFuchsia = 0x00FF00FF,
	aclGainsboro = 0x00DCDCDC,
	aclGhostWhite = 0x00F8F8FF,
	aclGold = 0x00FFD700,
	aclGoldenrod = 0x00DAA520,
	aclGray = 0x00808080,
	aclGreen = 0x00008000,
	aclGreenYellow = 0x00ADFF2F,
	aclHoneydew = 0x00F0FFF0,
	aclHotPink = 0x00FF69B4,
	aclIndianRed = 0x00CD5C5C,
	aclIndigo = 0x004B0082,
	aclIvory = 0x00FFFFF0,
	aclKhaki = 0x00F0E68C,
	aclLavender = 0x00E6E6FA,
	aclLavenderBlush = 0x00FFF0F5,
	aclLawnGreen = 0x007CFC00,
	aclLemonChiffon = 0x00FFFACD,
	aclLightBlue = 0x00ADD8E6,
	aclLightCoral = 0x00F08080,
	aclLightCyan = 0x00E0FFFF,
	aclLightGoldenrodYellow = 0x00FAFAD2,
	aclLightGray = 0x00D3D3D3,
	aclLightGreen = 0x0090EE90,
	aclLightPink = 0x00FFB6C1,
	aclLightSalmon = 0x00FFA07A,
	aclLightSeaGreen = 0x0020B2AA,
	aclLightSkyBlue = 0x0087CEFA,
	aclLightSlateGray = 0x00778899,
	aclLightSteelBlue = 0x00B0C4DE,
	aclLightYellow = 0x00FFFFE0,
	aclLime = 0x0000FF00,
	aclLimeGreen = 0x0032CD32,
	aclLinen = 0x00FAF0E6,
	aclMagenta = 0x00FF00FF,
	aclMaroon = 0x00800000,
	aclMediumAquamarine = 0x0066CDAA,
	aclMediumBlue = 0x000000CD,
	aclMediumOrchid = 0x00BA55D3,
	aclMediumPurple = 0x009370DB,
	aclMediumSeaGreen = 0x003CB371,
	aclMediumSlateBlue = 0x007B68EE,
	aclMediumSpringGreen = 0x0000FA9A,
	aclMediumTurquoise = 0x0048D1CC,
	aclMediumVioletRed = 0x00C71585,
	aclMidnightBlue = 0x00191970,
	aclMintCream = 0x00F5FFFA,
	aclMistyRose = 0x00FFE4E1,
	aclMoccasin = 0x00FFE4B5,
	aclNavajoWhite = 0x00FFDEAD,
	aclNavy = 0x00000080,
	aclOldLace = 0x00FDF5E6,
	aclOlive = 0x00808000,
	aclOliveDrab = 0x006B8E23,
	aclOrange = 0x00FFA500,
	aclOrangeRed = 0x00FF4500,
	aclOrchid = 0x00DA70D6,
	aclPaleGoldenrod = 0x00EEE8AA,
	aclPaleGreen = 0x0098FB98,
	aclPaleTurquoise = 0x00AFEEEE,
	aclPaleVioletRed = 0x00DB7093,
	aclPapayaWhip = 0x00FFEFD5,
	aclPeachPuff = 0x00FFDAB9,
	aclPeru = 0x00CD853F,
	aclPink = 0x00FFC0CB,
	aclPlum = 0x00DDA0DD,
	aclPowderBlue = 0x00B0E0E6,
	aclPurple = 0x00800080,
	aclRed = 0x00FF0000,
	aclRosyBrown = 0x00BC8F8F,
	aclRoyalBlue = 0x004169E1,
	aclSaddleBrown = 0x008B4513,
	aclSalmon = 0x00FA8072,
	aclSandyBrown = 0x00F4A460,
	aclSeaGreen = 0x002E8B57,
	aclSeaShell = 0x00FFF5EE,
	aclSienna = 0x00A0522D,
	aclSilver = 0x00C0C0C0,
	aclSkyBlue = 0x0087CEEB,
	aclSlateBlue = 0x006A5ACD,
	aclSlateGray = 0x00708090,
	aclSnow = 0x00FFFAFA,
	aclSpringGreen = 0x0000FF7F,
	aclSteelBlue = 0x004682B4,
	aclTan = 0x00D2B48C,
	aclTeal = 0x00008080,
	aclThistle = 0x00D8BFD8,
	aclTomato = 0x00FF6347,
	aclTurquoise = 0x0040E0D0,
	aclViolet = 0x00EE82EE,
	aclWheat = 0x00F5DEB3,
	aclWhite = 0x00FFFFFF,
	aclWhiteSmoke = 0x00F5F5F5,
	aclYellow = 0x00FFFF00,
	aclYellowGreen = 0x009ACD32,
	//
	aclTransparent = 0xFFFFFFFF, // Pixel is transparent
};

struct RGBColor {
	union {
		struct {
			uint8_t b;
			uint8_t g;
			uint8_t r;
			uint8_t x;
		};
		uint32_t value = 0;
	};

	RGBColor()
	{
	}

	RGBColor(uint8_t r, uint8_t g, uint8_t b) : b(b), g(g), r(r), x(0)
	{
	}

	RGBColor(SeColor color)
	{
		set(color);
	}

	void set(SeColor color)
	{
		switch(color.getFormat()) {
		case format_RGB_565:
		case format_RGB_565LUT:
			r = 255 * ((color.code >> 11) & 0x1f) / 31;
			g = 255 * ((color.code >> 5) & 0x3f) / 63;
			b = 255 * (color.code & 0x1f) / 31;
			x = 0;
			break;

		case format_RGB_332LUT:
			r = 255 * ((color.code >> 5) & 0x07) / 7;
			g = 255 * ((color.code >> 2) & 0x07) / 7;
			b = 255 * (color.code & 0x03) / 3;
			x = 0;
			break;

		case format_RGB_888:
		case format_RGB_888LUT:
		default:
			value = color.code;
		}
	}

	SeColor getColor(ImageDataFormat format = format_RGB_888)
	{
		SeColor tmp(0, format);
		switch(format) {
		case format_RGB_565:
		case format_RGB_565LUT:
			tmp.code = ((r * 31 / 255) << 11) | ((g * 63 / 255) << 5) | (b * 31 / 255);
			break;

		case format_RGB_332LUT:
			tmp.code = ((r * 7 / 255) << 5) | ((g * 7 / 255) << 2) | (b * 3 / 255);
			break;

		case format_RGB_888:
		case format_RGB_888LUT:
		default:
			tmp.code = value;
		}

		return tmp;
	}

	void scale(unsigned num, unsigned denom)
	{
		r = r * num / denom;
		g = g * num / denom;
		b = b * num / denom;
	}
};

/** @brief Scaled the intensity of a colour value
 *  @param color In any supported colour format
 *  @param intensity percentage brightness
 *  @retval Adjusted colour value, in RGB format
 */
static inline SeColor scaleColor(SeColor color, uint8_t intensity)
{
	const uint8_t imax = 100U;
	if(intensity > imax) {
		intensity = imax;
	}
	RGBColor rgb(color);
	rgb.scale(intensity, imax);
	return rgb.getColor();
}
