/* ======================================================================
 * S1d13781_init.h
 * Register initialization sequence for S1D13781 Shield. The default
 * panel configuration is 480x272@24bpp. For different panel types and
 * resolutions, different configuration values are required.
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

#include <FakePgmSpace.h>

/** @brief Defines the source input clock for the display */
const uint32_t S1D13781_CLKI = 1000000UL;


/*
 * S1D13781 register init sequence for Shield with 800x480@8bpp panel
 *
 * This uses all available RAM, so to test PIP operations we need to re-use part of main window RAM.
 * We'll set this at the end.
 */

const unsigned VRAM_SIZE = 0x60000;
const unsigned MAIN_WIDTH = 800;
const unsigned MAIN_HEIGHT = 480;
const unsigned PIP_WIDTH = 128;//200;
const unsigned PIP_HEIGHT = 72;//100;
const unsigned PIP_SADDR = VRAM_SIZE - (PIP_WIDTH * PIP_HEIGHT);

static const SeSetting displaySettings[] PROGMEM = {
	{scRegWrite, REG06_RESET, 0x0100},
	{scDelayOn, 0, 10000},
	{scRegWrite, REG04_POWER_SAVE, 0x0000},
	{scRegWrite, REG10_PLL_0, 0x0000},
	{scRegWrite, REG12_PLL_1, 0x0000},
	{scRegWrite, REG14_PLL_2, 40 - 1},
	{scRegWrite, REG10_PLL_0, 0x0001},
	{scPllWait, 0, 10000},
	{scRegWrite, REG16_INTCLK, 0},
	{scRegWrite, REG04_POWER_SAVE, 0x0001},
	{scPllWait, 0, 10000},
	{scRegWrite, REG20_PANEL_SET, 0x006F},	 // DE active high, PCLK falling edge, colour, 24-bit, TFT
	{scRegWrite, REG22_DISP_SET, 0x0001},	  // Panel interface enable
	{scRegWrite, REG24_HDISP, MAIN_WIDTH / 8}, // Horizontal display width in bytes
	{scRegWrite, REG26_HNDP, 48 + 40 + 3},	 // Horizontal non-display period (>= HPS + HWS)
	{scRegWrite, REG2C_HSW, 48},			   // Horizontal sync active low, pulse width 48 PCLKs
	{scRegWrite, REG2E_HPS, 3},				   // Horizontal sync pulse start delay, in PCLKs
	{scRegWrite, REG28_VDISP, MAIN_HEIGHT},	// Vertical display height in lines
	{scRegWrite, REG2A_VNDP, 45},			   // Vertical non-display period, max 255, also 2 <= VNDP < 1024 - VDISP
	{scRegWrite, REG30_VSW, 3},				   // Vertical sync pulse active low, width 3 PCLKs
	{scRegWrite, REG32_VPS, 7 - 1},			   // Vertical sync pulse start position = lines - 1
	{scRegWrite, REG40_MAIN_SET, (0x03 << 3) | 6},	// rotate 270 deg, format_RGB_332LUT
	{scRegWrite32, REG42_MAIN_SADDR_0, 0x00000000},
	{scRegWrite, REG50_PIP_SET, (0x03 << 3) | 6}, // rotate 270 deg, format_RGB_332LUT
	{scRegWrite32, REG52_PIP_SADDR_0, PIP_SADDR},
	{scRegWrite, REG56_PIP_WIDTH, PIP_WIDTH},
	{scRegWrite, REG58_PIP_HEIGHT, PIP_HEIGHT},
	{scRegWrite, REG5A_PIP_XSTART, 200},
	{scRegWrite, REG5C_PIP_YSTART, 200},
	{scRegWrite, REG60_PIP_EN, 0x0000},
	{scRegWrite, REG62_ALPHA, 0x0040},
	{scRegWrite, REG64_TRANS, 0x0000},
	{scRegWrite32, REG66_KEY_0, 0x00000000},
	{scRegWrite, REGD0_GPIO_CONFIG, 0x0000},
	{scRegWrite, REGD2_GPIO_STATUS, 0x0000},
	{scRegWrite, REGD4_GPIO_PULLDOWN, 0x000E},
	{scRegWrite, REG04_POWER_SAVE, 0x0002}};

/*
 * Example values for a typical 320x240 panel. Note these values may
 * not be valid for all configurations. Please confirm the register
 * values before initializing the system with these values.
 * 
regData regInitValues[] = {
	{ REG06_RESET,                          0x0100 },
	{ S1D_REGDELAYON,                       0x2710 },
	{ REG04_POWER_SAVE,                     0x0000 },
	{ REG10_PLL_0,                          0x0000 },
	{ REG12_PLL_1,                          0x0000 },
	{ REG14_PLL_2,                          0x003F },
	{ REG10_PLL_0,                          0x0001 },
	{ S1D_REGDELAYPLL,                      0x09C4 },
	{ REG16_INTCLK,                         0x0009 },
	{ REG04_POWER_SAVE,                     0x0001 },
	{ S1D_REGDELAYPLL,                      0x09C4 },
	{ REG20_PANEL_SET,                      0x006F },
	{ REG22_DISP_SET,                       0x0001 },
	{ REG24_HDISP,                          0x0028 },
	{ REG26_HNDP,                           0x0058 },
	{ REG28_VDISP,                          0x00F0 },
	{ REG2A_VNDP,                           0x0017 },
	{ REG2C_HSW,                            0x0001 },
	{ REG2E_HPS,                            0x0012 },
	{ REG30_VSW,                            0x0001 },
	{ REG32_VPS,                            0x000A },
	{ REG40_MAIN_SET,                       0x0000 },
	{ REG42_MAIN_SADDR_0,                   0x0000 },
	{ REG44_MAIN_SADDR_1,                   0x0000 },
	{ REG50_PIP_SET,                        0x0000 },
	{ REG52_PIP_SADDR_0,                    0x8400 },
	{ REG54_PIP_SADDR_1,                    0x0003 },
	{ REG56_PIP_WIDTH,                      0x0040 },
	{ REG58_PIP_HEIGHT,                     0x0040 },
	{ REG5A_PIP_XSTART,                     0x00A0 },
	{ REG5C_PIP_YSTART,                     0x0000 },
	{ REG60_PIP_EN,                         0x0001 },
	{ REG62_ALPHA,                          0x0040 },
	{ REG64_TRANS,                          0x0000 },
	{ REG66_KEY_0,                          0x0000 },
	{ REG68_KEY_1,                          0x0000 },
	{ REGD0_GPIO_CONFIG,                    0x0000 },
	{ REGD2_GPIO_STATUS,                    0x0000 },
	{ REGD4_GPIO_PULLDOWN,                  0x000E },
	{ REG04_POWER_SAVE,                     0x0002 },
	{ REGFLAG_END_OF_TABLE,                 0x0000 }
};
*/

/*
 * Example values for a typical 480x272@24bpp panel. Note these values may
 * not be valid for all configurations. Please confirm the register
 * values before initializing the system with these values.
 *
regData regInitValues[] = {
	{ REG06_RESET,                          0x0100 },
	{ S1D_REGDELAYON,                       0x2710 },
	{ REG04_POWER_SAVE,                     0x0000 },
	{ REG10_PLL_0,                          0x0000 },
	{ REG12_PLL_1,                          0x0000 },
	{ REG14_PLL_2,                          0x003E },
	{ REG10_PLL_0,                          0x0001 },
	{ S1D_REGDELAYPLL,                      0x09C4 },
	{ REG16_INTCLK,                         0x0006 },
	{ REG04_POWER_SAVE,                     0x0001 },
	{ S1D_REGDELAYPLL,                      0x09C4 },
	{ REG20_PANEL_SET,                      0x004F },
	{ REG22_DISP_SET,                       0x0001 },
	{ REG24_HDISP,                          0x003C },
	{ REG26_HNDP,                           0x002D },
	{ REG28_VDISP,                          0x0110 },
	{ REG2A_VNDP,                           0x0010 },
	{ REG2C_HSW,                            0x0001 },
	{ REG2E_HPS,                            0x0005 },
	{ REG30_VSW,                            0x0001 },
	{ REG32_VPS,                            0x0008 },
	{ REG40_MAIN_SET,                       0x0000 },
	{ REG42_MAIN_SADDR_0,                   0x0000 },
	{ REG44_MAIN_SADDR_1,                   0x0000 },
	{ REG50_PIP_SET,                        0x0000 },
	{ REG52_PIP_SADDR_0,                    0xFA00 },
	{ REG54_PIP_SADDR_1,                    0x0005 },
	{ REG56_PIP_WIDTH,                      0x0040 },
	{ REG58_PIP_HEIGHT,                     0x0040 },
	{ REG5A_PIP_XSTART,                     0x00A0 },
	{ REG5C_PIP_YSTART,                     0x0000 },
	{ REG60_PIP_EN,                         0x0000 },
	{ REG62_ALPHA,                          0x0040 },
	{ REG64_TRANS,                          0x0000 },
	{ REG66_KEY_0,                          0x0000 },
	{ REG68_KEY_1,                          0x0000 },
	{ REGD0_GPIO_CONFIG,                    0x0000 },
	{ REGD2_GPIO_STATUS,                    0x0000 },
	{ REGD4_GPIO_PULLDOWN,                  0x000E },
	{ REG04_POWER_SAVE,                     0x0002 },
	{ REGFLAG_END_OF_TABLE,                 0x0000 }
};
*/

/*
 * Example values for a typical 480x272@16bpp panel. Note these values may
 * not be valid for all configurations. Please confirm the register
 * values before initializing the system with these values.
 * 
regData regInitValues[] = {
	{ REG06_RESET,                          0x0100 },
	{ S1D_REGDELAYON,                       0x2710 },
	{ REG04_POWER_SAVE,                     0x0000 },
	{ REG10_PLL_0,                          0x0000 },
	{ REG12_PLL_1,                          0x0000 },
	{ REG14_PLL_2,                          0x003E },
	{ REG10_PLL_0,                          0x0001 },
	{ S1D_REGDELAYPLL,                      0x09C4 },
	{ REG16_INTCLK,                         0x0006 },
	{ REG04_POWER_SAVE,                     0x0001 },
	{ S1D_REGDELAYPLL,                      0x09C4 },
	{ REG20_PANEL_SET,                      0x004F },
	{ REG22_DISP_SET,                       0x0001 },
	{ REG24_HDISP,                          0x003C },
	{ REG26_HNDP,                           0x002D },
	{ REG28_VDISP,                          0x0110 },
	{ REG2A_VNDP,                           0x0010 },
	{ REG2C_HSW,                            0x0001 },
	{ REG2E_HPS,                            0x0005 },
	{ REG30_VSW,                            0x0001 },
	{ REG32_VPS,                            0x0008 },
	{ REG40_MAIN_SET,                       0x0001 },
	{ REG42_MAIN_SADDR_0,                   0x0000 },
	{ REG44_MAIN_SADDR_1,                   0x0000 },
	{ REG50_PIP_SET,                        0x0000 },
	{ REG52_PIP_SADDR_0,                    0xFC00 },
	{ REG54_PIP_SADDR_1,                    0x0003 },
	{ REG56_PIP_WIDTH,                      0x0040 },
	{ REG58_PIP_HEIGHT,                     0x0040 },
	{ REG5A_PIP_XSTART,                     0x00A0 },
	{ REG5C_PIP_YSTART,                     0x0000 },
	{ REG60_PIP_EN,                         0x0001 },
	{ REG62_ALPHA,                          0x0040 },
	{ REG64_TRANS,                          0x0000 },
	{ REG66_KEY_0,                          0x0000 },
	{ REG68_KEY_1,                          0x0000 },
	{ REGD0_GPIO_CONFIG,                    0x0000 },
	{ REGD2_GPIO_STATUS,                    0x0000 },
	{ REGD4_GPIO_PULLDOWN,                  0x000E },
	{ REG04_POWER_SAVE,                     0x0002 },
	{ REGFLAG_END_OF_TABLE,                 0x0000 }
};
*/
