/*
 * fonts.h
 *
 *  Created on: 23 Dec 2018
 *      Author: Mike
 */

#pragma once

#include <S1D13781/SeFont.h>
#include <FlashString/Array.hpp>

#define DEFINE_FONT(name, ...) extern const uint8_t fontData_##name[] PROGMEM = {__VA_ARGS__};
#define DECLARE_FONT(name, width, height) extern const uint8_t fontData_##name[];

// Fonts taken from Linux source tree
#define LINUX_FONT_LIST(XX)                                                                                            \
	XX(6x10, 6, 10)                                                                                                    \
	XX(6x11, 6, 11)                                                                                                    \
	XX(7x14, 7, 14)                                                                                                    \
	XX(8x8, 8, 8)                                                                                                      \
	XX(8x16, 8, 16)                                                                                                    \
	XX(sun8x16, 8, 16)                                                                                                 \
	XX(10x18, 10, 18)                                                                                                  \
	XX(acorn_8x8, 8, 8)                                                                                                \
	XX(pearl_8x8, 8, 8)                                                                                                \
	XX(sun12x22, 12, 22)

LINUX_FONT_LIST(DECLARE_FONT)

// PFI fonts
#define FONT_LIST(XX)                                                                                                  \
	XX(Ascii4x6)                                                                                                       \
	XX(Ascii4x6p)                                                                                                      \
	XX(Ascii6x10)                                                                                                      \
	XX(Ascii6x10p)                                                                                                     \
	XX(Ascii7x11)                                                                                                      \
	XX(Ascii7x11p)                                                                                                     \
	XX(Ascii9x13)                                                                                                      \
	XX(Ascii9x13p)                                                                                                     \
	XX(AsciiCaps4x6)                                                                                                   \
	XX(AsciiCaps4x6p)                                                                                                  \
	XX(Latin6x10)                                                                                                      \
	XX(Latin6x10p)                                                                                                     \
	XX(LineDraw6x10)

DECLARE_FSTR_ARRAY(fontTable, FontDef);

// All fonts accessible by name
enum FontName {
#define XX(name) font_##name,
	FONT_LIST(XX)
#undef XX
#define XX(name, width, height) font_##name,
		LINUX_FONT_LIST(XX)
#undef XX
};
