/*
 * linux-fonts.h
 *
 *  Created on: 23 Dec 2018
 *      Author: Mike
 */

#pragma once

#include <cstdint>
#include <FakePgmSpace.h>

#define DEFINE_FONT(name, ...) extern const uint8_t fontData_##name[] PROGMEM = {__VA_ARGS__};
#define DECLARE_FONT(name, width, height) extern const uint8_t fontData_##name[];

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
