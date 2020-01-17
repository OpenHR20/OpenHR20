/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  compiler:    WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Dario Carluccio (hr20-at-carluccio-dot-de)
 *                              2008 Jiri Dobry (jdobry-at-centrum-dot-cz)
 *
 *  license:    This program is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU Library General Public
 *              License as published by the Free Software Foundation; either
 *              version 2 of the License, or (at your option) any later version.
 *
 *              This program is distributed in the hope that it will be useful,
 *              but WITHOUT ANY WARRANTY; without even the implied warranty of
 *              MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *              GNU General Public License for more details.
 *
 *              You should have received a copy of the GNU General Public License
 *              along with this program. If not, see http:*www.gnu.org/licenses
 */

/*!
 * \file       lcd.c
 * \brief      functions to control the HR20 LCD
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>, Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

// AVR LibC includes
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/version.h>

// redefine variable names for newer processor
#ifdef _AVR_IOM329_H_
#define LCDDR7 LCDDR07
#define LCDDR6 LCDDR06
#define LCDDR5 LCDDR05
#define LCDDR2 LCDDR02
#define LCDDR1 LCDDR01
#define LCDDR0 LCDDR00
#endif

// HR20 Project includes
#include "main.h"
#include "lcd.h"
#include "task.h"
#include "common/rtc.h"
#include "eeprom.h"

// Local Defines
#define LCD_CONTRAST_MIN       0                        //!< \brief contrast minimum
#define LCD_CONTRAST_MAX      15                        //!< \brief contrast maxmum
#define LCD_MAX_POS            4                        //!< \brief number of 7 segment chars
#define LCD_MAX_CHARS  (sizeof(LCD_CharTablePrgMem))    //!< \brief no. of chars in \ref LCD_CharTablePrgMem
#ifdef HR25
#define LCD_REGISTER_COUNT     12                       //!< \brief no. of registers each bitplane
#else
#define LCD_REGISTER_COUNT     9                        //!< \brief no. of registers each bitplane
#endif

// Vars
volatile uint8_t LCD_used_bitplanes = 1; //!< \brief number of used bitplanes / used for power save

//! segment data for the segment registers in each bitplane
volatile uint8_t LCD_Data[LCD_BITPLANES][LCD_REGISTER_COUNT];

#ifdef LCD_UPSIDE_DOWN
#define LCD_upside_down 1
#else
#define LCD_upside_down 0
#endif

// Look-up table to convert value to LCD display data (segment control)
// Get value with: bitmap = pgm_read_byte(&LCD_CharTablePrgMem[position]);
const uint8_t LCD_CharTablePrgMem[] PROGMEM =
{
	0x77,   //      0:0x77   1:0x06   2:0x6B   3:0x4F   4:0x1e   5:0x5D   6:0x7D
	0x06,   // 1    ******        *   ******   ******   *    *   ******   ******
	0x6B,   // 2    *    *        *        *        *   *    *   *        *
	0x4F,   // 3    *    *        *   ******   ******   ******   ******   ******
	0x1e,   // 4    *    *        *   *             *        *        *   *    *
	0x5d,   // 5    ******        *   ******   ******        *   ******   ******
	0x7d,   // 6   1110111  0000110  1101011  1001111  0011110  1011101  1111101

	0x07,   // 7    7:0x07   8:0x7F   9:0x5F  10:0x3f  11:0x7C  12:0x71  13:0x5e
	0x7F,   // 8    ******   ******   ******   ******   *        ******        *
	0x5F,   // 9         *   *    *   *    *   *    *   *        *             *
	0x3f,   //10         *   ******   ******   ******   ******   *        ******
	0x7C,   //11         *   *    *        *   *    *   *    *   *        *    *
	0x71,   //12         *   ******   ******   *    *   ******   ******   ******
	0x6E,   //13   0000111  1111111  1011111  0111111  1111100  1110001  1101110

	0x79,   //14   14:0x79  15:0x39  16:0x1b  17:0x2c  18:0x3b  19:0x3e 20:0x30
	0x39,   //15    ******   ******   ******            ******   *    *  *
	0x1b,   //16    *        *        *    *            *    *   *    *  *
	0x2c,   //17    ******   ******   ******   ******   ******   ******  *
	0x3b,   //18    *        *                 *    *   *        *    *  *
	0x3e,   //19    ******   *                 *    *   *        *    *  *
	0x30,   //20   1111001  0111001  0011011  0101100  0111011  0111110  0110000

	0x40,   //21   21:0x40  22:0x08  23:0x01  24:0x48  25:0x09  26:0x41  27:0x49
	0x08,   //22                      ******            ******   ******   ******
	0x01,   //23
	0x48,   //24             ******            ******   ******            ******
	0x09,   //25
	0x41,   //26    ******                     ******            ******   ******
	0x49,   //27   1000000  0001000  0000001  1001000  0001001  1000001  1001001

	0x28,   //28   28:0x28  29:0x2c  30:0x20  31:0x6c  32:0x00  33:0x33  34:0x17
	0x2c,   //29                                                 ******   ******
	0x20,   //30                                                 *    *   *    *
	0x6c,   //31    ******   ******   *        ******   Space    *    *   *    *
	0x00,   //32    *        *    *   *        *    *            *             *
	0x33,   //33    *        *    *   *        ******            *             *
	0x17,   //34   0101000  0101100  0100000  1101100  0000000  0110011  0010111

	0x70,   //35   35:0x70  36:0x68  37:0x76  38:0x78  39:0x5e
	0x68,   //36    *                 *    *   *        *    *
	0x76,   //37    *                 *    *   *        *    *
	0x78,   //38    *        ******   *    *   ******   ******
	0x5e    //39    *        *        *    *   *             *
	        //40    ******   ******   ******   ******   ******
	        //41   1110000  1101000  1110110  1111000  1011110
};

#if LANG == LANG_uni
// Look-up chars table for LCD strings (universal/numbers)
const uint8_t LCD_StringTable[][4] PROGMEM =
{
	{ 32, 1,   22,  7   },  //!<  " 1-7"
	{ 32, 22,  1,   22  },  //!<  " -1-" MO
	{ 32, 22,  2,   22  },  //!<  " -2-" TU
	{ 32, 22,  3,   22  },  //!<  " -3-" WE
	{ 32, 22,  4,   22  },  //!<  " -4-" TH
	{ 32, 22,  5,   22  },  //!<  " -5-" FR
	{ 32, 22,  6,   22  },  //!<  " -6-" SA
	{ 32, 22,  7,   22  },  //!<  " -7-" SU
	{ 11, 1,   31,  36  },  //!<  "b1oc"    LCD_STRING_bloc
	{ 22, 22,  22,  22  },  //!<  "----"    LCD_STRING_4xminus
	{ 32, 22,  12,  22  },  //!<  " -C-"    LCD_STRING_minusCminus
	{ 32, 14,  28,  28  },  //!<  " Err"    LCD_STRING_Err
	{ 0,  15,  15,  32  },  //!<  "OFF "    LCD_STRING_OFF
	{ 0,  29,  32,  32  },  //!<  "On  "    LCD_STRING_On
	{ 0,  18,  14,  29  },  //!<  "OPEn"    LCD_STRING_OPEn
	{ 11, 10,  38,  38  },  //!<  "BAtt"    LCD_STRING_BAtt
	{ 32, 14,  2,   32  },  //!<  " E2 "    LCD_STRING_E2
	{ 32, 14,  3,   32  },  //!<  " E3 "    LCD_STRING_E3
	{ 32, 14,  4,   32  },  //!<  " E4 "    LCD_STRING_E4
	{ 14, 14,  18,  28  },  //!<  "EEPr"    LCD_STRING_EEPr
};
#elif LANG == LANG_de
// Look-up chars table for LCD strings (german)
const uint8_t LCD_StringTable[][4] PROGMEM =
{
	{ 32, 1,   22,  7   },  //!<  " 1-7"
	{ 33, 34,  31,  32  },  //!<  Montag:     'rno '
	{ 32, 13,  30,  32  },  //!<  Dienstag:   ' di '
	{ 33, 34,  30,  32  },  //!<  Mittwoch:   'rni '
	{ 32, 13,  31,  32  },  //!<  Donnerstag: ' do '
	{ 32, 15,  28,  32  },  //!<  Freitag:    ' Fr '
	{ 32, 5,   10,  32  },  //!<  Samstag:    ' SA '
	{ 32, 5,   31,  32  },  //!<  Sonntag:    ' So '
	{ 11, 1,   31,  36  },  //!<  "b1oc"    LCD_STRING_bloc
	{ 22, 22,  22,  22  },  //!<  "----"    LCD_STRING_4xminus
	{ 32, 22,  12,  22  },  //!<  " -C-"    LCD_STRING_minusCminus
	{ 32, 14,  28,  28  },  //!<  " Err"    LCD_STRING_Err
	{ 0,  15,  15,  32  },  //!<  "OFF "    LCD_STRING_OFF
	{ 0,  29,  32,  32  },  //!<  "On  "    LCD_STRING_On
	{ 0,  18,  14,  29  },  //!<  "OPEn"    LCD_STRING_OPEn
	{ 11, 10,  38,  38  },  //!<  "BAtt"    LCD_STRING_BAtt
	{ 32, 14,  2,   32  },  //!<  " E2 "    LCD_STRING_E2
	{ 32, 14,  3,   32  },  //!<  " E3 "    LCD_STRING_E3
	{ 32, 14,  4,   32  },  //!<  " E4 "    LCD_STRING_E4
	{ 14, 14,  18,  28  },  //!<  "EEPr"    LCD_STRING_EEPr
};
#elif LANG == LANG_cs
// Look-up chars table for LCD strings (czech)
const uint8_t LCD_StringTable[][4] PROGMEM =
{
	{ 32, 1,   22,  7   },  //!<  " 1-7"
	{ 32, 18,  31,  22  },  //!<  " Po "
	{ 32, 37,  38,  22  },  //!<  " Ut "
	{ 32, 5,   38,  22  },  //!<  " St "
	{ 32, 12,  38,  22  },  //!<  " Ct "
	{ 32, 18,  10,  22  },  //!<  " PA "
	{ 32, 5,   31,  22  },  //!<  " So "
	{ 32, 29,  14,  22  },  //!<  " nE "
	{ 11, 1,   31,  36  },  //!<  "b1oc"    LCD_STRING_bloc
	{ 22, 22,  22,  22  },  //!<  "----"    LCD_STRING_4xminus
	{ 32, 22,  12,  22  },  //!<  " -C-"    LCD_STRING_minusCminus
	{ 32, 14,  28,  28  },  //!<  " Err"    LCD_STRING_Err
	{ 0,  15,  15,  32  },  //!<  "OFF "    LCD_STRING_OFF
	{ 0,  29,  32,  32  },  //!<  "On  "    LCD_STRING_On
	{ 0,  18,  14,  29  },  //!<  "OPEn"    LCD_STRING_OPEn
	{ 11, 10,  38,  38  },  //!<  "BAtt"    LCD_STRING_BAtt
	{ 32, 14,  2,   32  },  //!<  " E2 "    LCD_STRING_E2
	{ 32, 14,  3,   32  },  //!<  " E3 "    LCD_STRING_E3
	{ 32, 14,  4,   32  },  //!<  " E4 "    LCD_STRING_E4
	{ 14, 14,  18,  28  },  //!<  "EEPr"    LCD_STRING_EEPr
};
#endif



// Look-up table to adress element F for one Position. ( 32 : 10 )
const uint8_t LCD_FieldOffsetTablePrgMem[] PROGMEM =
{
#if THERMOTRONIC == 1
	39,     //!<  Field 0
	35,     //!<  Field 1
	31,     //!<  Field 2
	27      //!<  Field 3
#elif HR25 == 1
	39,     //!<  Field 0
	35,     //!<  Field 1
	30,     //!<  Field 2
	26      //!<  Field 3
#else
	40,     //!<  Field 0
	36,     //!<  Field 1
	31,     //!<  Field 2
	27      //!<  Field 3
#endif
};


#ifdef HR25
// Look-up table to adress a segment inside a field
const uint8_t LCD_SegOffsetTablePrgMem[] PROGMEM =
{
	2,      //  Seg A            AAAA
	3,      //  Seg B           E    B
	26,     //  Seg C           E    B
	1,      //  Seg D            DDDD
	0,      //  Seg E           F    C
	24,     //  Seg F           F    C
	25      //  Seg G            GGGG
};
#else
// Look-up table to adress a segment inside a field
const uint8_t LCD_SegOffsetTablePrgMem[] PROGMEM =
{
	2,      //  Seg A            AAAA
	3,      //  Seg B           E    B
	27,     //  Seg C           E    B
	1,      //  Seg D            DDDD
	0,      //  Seg E           F    C
	24,     //  Seg F           F    C
	25      //  Seg G            GGGG
};
#endif

//! Look-up table for adress hour-bar segments
const uint8_t LCD_SegHourBarOffsetTablePrgMem[] PROGMEM =
{
	LCD_SEG_B0,  LCD_SEG_B1,  LCD_SEG_B2,  LCD_SEG_B3,  LCD_SEG_B4,
	LCD_SEG_B5,  LCD_SEG_B6,  LCD_SEG_B7,  LCD_SEG_B8,  LCD_SEG_B9,
	LCD_SEG_B10, LCD_SEG_B11, LCD_SEG_B12, LCD_SEG_B13, LCD_SEG_B14,
	LCD_SEG_B15, LCD_SEG_B16, LCD_SEG_B17, LCD_SEG_B18, LCD_SEG_B19,
	LCD_SEG_B20, LCD_SEG_B21, LCD_SEG_B22, LCD_SEG_B23
};


static void LCD_calc_used_bitplanes(uint8_t mode);

/*!
 *******************************************************************************
 *  Init LCD
 *
 *  \note
 *  - Initialize LCD Global Vars
 *  - Set up the LCD (timing, contrast, etc.)
 ******************************************************************************/
void LCD_Init(void)
{
	// Clear segment buffer.
	LCD_AllSegments(LCD_MODE_OFF);

	//Set the initial LCD contrast level
	LCDCCR = (config.lcd_contrast << LCDCC0);

#ifdef HR25
	// LCD Control and Status Register B
	//   - clock source is TOSC1 pin
	//   - COM0:2 connected
	//   - SEG0:22 connected
	LCDCRB = (1 << LCDCS) | (1 << LCDMUX1) | (1 << LCDMUX0) | (1 << LCDPM2) | (1 << LCDPM0);
	// LCD Frame Rate Register
	//   - LCD Prescaler Select N=16       @32.768Hz ->   2048Hz
	//   - LCD Duty Cycle 1/4 (K=8)       2048Hz / 8 ->    256Hz
	//   - LCD Clock Divider  (D=5)        256Hz / 5 ->   51,2Hz
	LCDFRR = ((1 << LCDCD2));
#else
	// LCD Control and Status Register B
	//   - clock source is TOSC1 pin
	//   - COM0:3 connected
	//   - SEG0:22 connected
	LCDCRB = (1 << LCDCS) | (1 << LCDMUX1) | (1 << LCDPM2) | (1 << LCDPM0);
	// LCD Frame Rate Register
	//   - LCD Prescaler Select N=16       @32.768Hz ->   2048Hz
	//   - LCD Duty Cycle 1/3 (K=6)       2048Hz / 6 -> 341,33Hz
	//   - LCD Clock Divider  (D=7)     341,33Hz / 7 ->  47,76Hz
	LCDFRR = ((1 << LCDCD2) | (1 << LCDCD1));
#endif

	// LCD Control and Status Register A
	//  - Enable LCD
	//  - Set Low Power Waveform
	LCDCRA = (1 << LCDEN) | (1 << LCDAB);

	// Enable LCD start of frame interrupt
	LCDCRA |= (1 << LCDIE);

	LCD_used_bitplanes = 1;
}


/*!
 *******************************************************************************
 *  Switch LCD on/off
 *
 *  \param mode
 *       -      0: clears all digits
 *       -  other: set all digits
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF
 ******************************************************************************/
void LCD_AllSegments(uint8_t mode)
{
	uint8_t i;
	uint8_t val = (mode == LCD_MODE_ON) ? 0xff : 0x00;

	for (i = 0; i < LCD_REGISTER_COUNT * LCD_BITPLANES; i++)
	{
		((uint8_t *)LCD_Data)[i] = val;
	}
	LCD_used_bitplanes = 1;
	LCD_Update();
}

/*!
 *******************************************************************************
 *  Print char in LCD field
 *
 *  \note  segments inside one 7 segment array are adressed using address of
 *         segment "F" \ref LCD_FieldOffsetTablePrgMem[] as base address adding
 *         \ref LCD_SegOffsetTablePrgMem[] *
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatically at change of bitframe
 *
 *  \param value char to print see \ref LCD_CharTablePrgMem[]
 *  \param pos   position in lcd 0=right to 3=left <tt> 32 : 10 </tt>
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 *  \param value
 *        - 0: clears all digits,
 *        - other: set all digits
 ******************************************************************************/
void LCD_PrintChar(uint8_t value, uint8_t pos, uint8_t mode)
{
	uint8_t fieldbase;
	uint8_t bitmap;
	uint8_t seg;
	uint8_t i;
	uint8_t mask;

	// Boundary Check
	if ((pos < LCD_MAX_POS) && (value < LCD_MAX_CHARS))
	{
		// Get Fieldbase for Position
		if (LCD_upside_down)
		{
			pos = 3 - pos;
		}
		fieldbase = pgm_read_byte(&LCD_FieldOffsetTablePrgMem[pos]);
		// Get Bitmap for Value
		bitmap = pgm_read_byte(&LCD_CharTablePrgMem[value]);
		mask = 1;

		// Set 7 Bits
		for (i = 0; i < 7; i++)
		{
			seg = fieldbase + pgm_read_byte(&LCD_SegOffsetTablePrgMem[LCD_upside_down ? 6 - i : i]);
			// Set or Clear?
			LCD_SetSeg(seg, ((bitmap & mask) ? mode : LCD_MODE_OFF));
			mask <<= 1;
		}
	}
}


/*!
 *******************************************************************************
 *  Print Hex value in LCD field
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatically at change of bitframe
 *
 *  \param value value to be printed (0-0xff)
 *  \param pos   position in lcd 0:left, 1:right
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintHex(uint8_t value, uint8_t pos, uint8_t mode)
{
	uint8_t tmp;

	// Boundary Check
	if (pos <= 2)
	{
		// 1st Digit at 0 (2)
		tmp = value % 16;
		LCD_PrintChar(tmp, pos, mode);
		pos++;
		// 2nd Digit at 1 (3)
		tmp = value / 16;
		LCD_PrintChar(tmp, pos, mode);
	}
}


/*!
 *******************************************************************************
 *  Print decimal value in LCD field (only 2 digits)
 *
 *  \note You have to call \ref LCD_Update() to trigger update on LCD if not
 *        it is triggered automatically at change of bitframe
 *
 *  \param value value to be printed (0-99)
 *  \param pos   position in lcd
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintDec(uint8_t value, uint8_t pos, uint8_t mode)
{
	uint8_t tmp;

	// Boundary Check
	if ((pos <= 2) && (value < 100))
	{
		// 1st Digit at 0 (2)
		tmp = value % 10;
		LCD_PrintChar(tmp, pos, mode);
		pos++;
		// 2nd Digit at 1 (3)
		tmp = value / 10;
		LCD_PrintChar(tmp, pos, mode);
	}
}
/*!
 *******************************************************************************
 *  Print decimal value in LCD field (3 digits)
 *
 *  \note You have to call \ref LCD_Update() to trigger update on LCD if not
 *        it is triggered automatically at change of bitframe
 *
 *  \param value value to be printed (0-999)
 *  \param pos   position in lcd
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintDec3(uint16_t value, uint8_t pos, uint8_t mode)
{
	if (value > 999)
	{
		value = 999;
	}
	if (pos <= 1)
	{
		// 3nd Digit
		LCD_PrintChar(value / 100, pos + 2, mode);
		LCD_PrintDec(value % 100, pos, mode);
	}
}


/*!
 *******************************************************************************
 *  Print decimal uint16 value in LCD field
 *
 *  \note You have to call \ref LCD_Update() to trigger update on LCD if not
 *        it is triggered automatically at change of bitframe
 *
 *  \param value value to be printed (0-9999)
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintDecW(uint16_t value, uint8_t mode)
{
	uint8_t tmp;

	// Boundary Check
	if (value > 9999)
	{
		value = 9999;
	}
	// Print
	tmp = (uint8_t)(value / 100);
	LCD_PrintDec(tmp, 2, mode);
	tmp = (uint8_t)(value % 100);
	LCD_PrintDec(tmp, 0, mode);
}


/*!
 *******************************************************************************
 *  Print hex uint16 value in LCD field
 *
 *  \note You have to call \ref LCD_Update() to trigger update on LCD if not
 *        it is triggered automatically at change of bitframe
 *
 *  \param value value to be printed (0-0xffff)
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintHexW(uint16_t value, uint8_t mode)
{
	uint8_t tmp;

	// Print
	tmp = (uint8_t)(value >> 8);
	LCD_PrintHex(tmp, 2, mode);
	tmp = (uint8_t)(value & 0xff);
	LCD_PrintHex(tmp, 0, mode);
}


/*!
 *******************************************************************************
 *  Print BYTE as temperature on LCD (desired temperature)
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatically at change of bitframe
 *
 *  \note  range for desired temperature 5,0°C - 30°C, OFF and ON
 *
 *  \param temp<BR>
 *     - TEMP_MIN-1          : \c OFF <BR>
 *     - TEMP_MIN to TEMP_MAX : temperature = temp/2  [5,0°C - 30°C]
 *     - TEMP_MAX+1          : \c ON  <BR>
 *     -    other: \c invalid <BR>
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintTemp(uint8_t temp, uint8_t mode)
{
	if (temp == TEMP_MIN - 1)
	{
		// OFF
		LCD_PrintStringID(LCD_STRING_OFF, mode);
	}
	else if (temp == TEMP_MAX + 1)
	{
		// On
		LCD_PrintStringID(LCD_STRING_On, mode);
	}
	else if (temp > TEMP_MAX + 1)
	{
		// Error -E rr
		LCD_PrintStringID(LCD_STRING_Err, mode);
	}
	else
	{
#ifdef HR25
#define START_POS 0
		LCD_PrintChar(LCD_CHAR_NULL, 3, mode);
		LCD_SetSeg(LCD_DEGREE, mode);           // Display degrees sign
		LCD_SetSeg(LCD_SEG_CELCIUS, mode);      // Display celsius sign
		LCD_SetSeg(LCD_SEG_COL5, mode);         // decimal point
#else
#define START_POS 1
		LCD_PrintChar(LCD_CHAR_C, 0, mode);     // Print C on last segment
		LCD_SetSeg(LCD_SEG_COL1, mode);         // decimal point
#endif
		LCD_PrintDec(temp >> 1, START_POS + 1, mode);
		LCD_PrintChar(((temp & 1) ? 5 : 0), START_POS, mode);
		if (temp < (100 / 5))
		{
			LCD_PrintChar(LCD_CHAR_NULL, START_POS + 2, mode);
		}
	}
}


/*!
 *******************************************************************************
 *  Print INT as temperature on LCD (measured temperature)
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatically at change of bitframe
 *
 *
 *  \param temp temperature in 1/100 deg C<BR>
 *     min:  -999 => -9,9°C
 *     max:  9999 => 99,9°C
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintTempInt(int16_t temp, uint8_t mode)
{
	bool neg;

	// check min / max
	if (temp < -999)
	{
		temp = -999;
	}

	// negative ?
	neg = (temp < 0);
	if (neg)
	{
		temp = -temp;
	}

#ifdef HR25
#define START_POS 0
	LCD_PrintChar(LCD_CHAR_NULL, 3, mode);
	LCD_SetSeg(LCD_DEGREE, mode);           // Display degrees sign
	LCD_SetSeg(LCD_SEG_CELCIUS, mode);      // Display celsius sign
	LCD_SetSeg(LCD_SEG_COL5, mode);         // decimal point
#else
#define START_POS 1
	LCD_PrintChar(LCD_CHAR_C, 0, mode);     // Print C on last segment
	LCD_SetSeg(LCD_SEG_COL1, mode);         // decimal point
#endif

	// 1/100°C not printed
	LCD_PrintDec3(temp / 10, START_POS, mode);

	if (neg)
	{
		// negative Temp
		LCD_PrintChar(LCD_CHAR_neg, START_POS + 2, mode);
	}
	else if (temp < 1000)
	{
		// Temp < 10°C
		LCD_PrintChar(LCD_CHAR_NULL, START_POS + 2, mode);
	}
}

/*!
 *******************************************************************************
 *  Print LCD string from table
 *
 *  \note  something weird due to 7 Segments
 *
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintStringID(uint8_t id, uint8_t mode)
{
	uint8_t i;
	uint8_t tmp;

	// Put 4 chars
	for (i = 0; i < 4; i++)
	{
		tmp = pgm_read_byte(&LCD_StringTable[id][i]);
		LCD_PrintChar(tmp, 3 - i, mode);
	}
}


/*!
 *******************************************************************************
 *  Set segment of the hour-bar
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatically at change of bitframe
 *
 *  \param seg No of the hour bar segment 0-23
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_SetHourBarSeg(uint8_t seg, uint8_t mode)
{
	uint8_t segment;

	// Get segment number for this element
	segment = pgm_read_byte(&LCD_SegHourBarOffsetTablePrgMem[seg]);
	// Set segment
	LCD_SetSeg(segment, mode);
}

/*!
 *******************************************************************************
 *  Set all segments of the hour-bar (ON/OFF) like bitmap
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatically at change of bitframe
 *
 *  \param bitmap of hour bar segment 0-23 (bit0 is segment0 etc.)
 *  \note blink is not supported
 ******************************************************************************/
void LCD_HourBarBitmap(uint32_t bitmap)
{
	asm volatile (
		"    movw r14,r22                                     " "\n"
		"    mov  r16,r24                                     " "\n"
		"    ldi r28,lo8(LCD_SegHourBarOffsetTablePrgMem)     " "\n"
		"    ldi r29,hi8(LCD_SegHourBarOffsetTablePrgMem)     " "\n"
		"L2:                                                  " "\n"
		"    movw r30,r28                                     " "\n"
		"	 lpm r24, Z                                       ""\n"
		"	 clr r22                                          ""\n"
		"	 lsr r16                                          ""\n"
		"	 ror r15                                          ""\n"
		"	 ror r14                                          ""\n"
		"    brcc L3                                          " "\n"
		"    ldi r22,lo8(%0)                                  " "\n"
		"L3:                                                  " "\n"
		"	call LCD_SetSeg                                   ""\n"
		"	adiw r28,1                                        ""\n"
		"	cpi r28,lo8(LCD_SegHourBarOffsetTablePrgMem+24)   ""\n"
		"	brne L2                                           ""\n"
		:
		: "I" (LCD_MODE_ON)
		: "r14", "r15", "r16", "r28", "r29", "r30", "r31"
	);
}


/*!
 *******************************************************************************
 *  Set segment of LCD
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatically at change of bitframe
 *
 *  \param seg No of the segment to be set see \ref LCD_SEG_B0 ...
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_SetSeg(uint8_t seg, uint8_t mode)
{
	LCD_SetSegReg(seg / 8, 1 << (seg % 8), mode);
}

/*!
 *******************************************************************************
 *  Set segment of LCD
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatically at change of bitframe
 *
 *  \param Register = segment DIV 8 to be set see \ref LCD_SEG_B0 ...
 *  \param Bitposition = segment mod 8
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_SetSegReg(uint8_t r, uint8_t b, uint8_t mode)
{
	// Set bits in each bitplane
#if LCD_BITPLANES == 2
	if (mode & 1)
	{
		// Set Bit in Bitplane if ON (0b11) or Blinkmode 1 (0b01)
		LCD_Data[0][r] |= b;
	}
	else
	{
		// Clear Bit in Bitplane if OFF (0b00) or Blinkmode 2 (0b10)
		LCD_Data[0][r] &= ~b;
	}
	if (mode & 2)
	{
		// Set Bit in Bitplane if ON (0b11) or Blinkmode 2 (0b10)
		LCD_Data[1][r] |= b;
	}
	else
	{
		// Clear Bit in Bitplane if OFF (0b00) or Blinkmode 1 (0b01)
		LCD_Data[1][r] &= ~b;
	}

#else
	{
		uint8_t bp;
		for (bp = 0; bp < LCD_BITPLANES; bp++)
		{
			if (mode & (1 << bp))
			{
				// Set Bit in Bitplane if ON (0b11) or Blinkmode 1 (0b01)
				LCD_Data[bp][r] |= b;
			}
			else
			{
				// Clear Bit in Bitplane if OFF (0b00) or Blinkmode 2 (0b10)
				LCD_Data[bp][r] &= ~b;
			}
		}
	}
#endif
	LCD_calc_used_bitplanes(mode);
}

/*!
 *******************************************************************************
 *  Calculate used bitplanes
 *
 *	\note used only for update LCD, in any other cases intterupt is disabled
 *  \note copy LCD_Data to LCDREG
 *
 ******************************************************************************/
static void LCD_calc_used_bitplanes(uint8_t mode)
{
	uint8_t i;

	if ((mode == LCD_MODE_BLINK_1) || (mode == LCD_MODE_BLINK_2))
	{
		LCD_used_bitplanes = 2;
		return; // just optimalization
	}
	// mode must be LCD_MODE_ON or LCD_MODE_OFF
	if (LCD_used_bitplanes == 1)
	{
		return; // just optimalization, nothing to do

	}
	for (i = 0; i < LCD_REGISTER_COUNT; i++)
	{
#if LCD_BITPLANES != 2
#error optimized for 2 bitplanes                         // TODO?
#endif
		if (LCD_Data[0][i] != LCD_Data[1][i])
		{
			LCD_used_bitplanes = 2;
			return; // it is done
		}
	}
	LCD_used_bitplanes = 1;
}


/*!
 *******************************************************************************
 *
 *	LCD_BlinkCounter and LCD_Bitplane for LCD blink
 *
 ******************************************************************************/
static uint8_t LCD_BlinkCounter;        //!< \brief counter for bitplane change
static uint8_t LCD_Bitplane;            //!< \brief currently active bitplane
uint8_t LCD_force_update = 0;           //!< \brief force update LCD


/*!
 *******************************************************************************
 *  LCD Interrupt Routine
 *
 *	\note used only for update LCD, in any other cases intterupt is disabled
 *  \note copy LCD_Data to LCDREG
 *
 ******************************************************************************/
void task_lcd_update(void)
{
	if (++LCD_BlinkCounter > LCD_BLINK_FRAMES)
	{
#if LCD_BITPLANES == 2
		// optimized version for LCD_BITPLANES == 2
		LCD_Bitplane = (LCD_Bitplane + 1) & 1;
#else
		LCD_Bitplane = (LCD_Bitplane + 1) % LCD_BITPLANES;
#endif
		LCD_BlinkCounter = 0;
		LCD_force_update = 1;
	}


	if (LCD_force_update)
	{
		LCD_force_update = 0;
		// Copy desired segment buffer to the real segments
		LCDDR0 = LCD_Data[LCD_Bitplane][0];
		LCDDR1 = LCD_Data[LCD_Bitplane][1];
		LCDDR2 = LCD_Data[LCD_Bitplane][2];
		LCDDR5 = LCD_Data[LCD_Bitplane][3];
		LCDDR6 = LCD_Data[LCD_Bitplane][4];
		LCDDR7 = LCD_Data[LCD_Bitplane][5];
		LCDDR10 = LCD_Data[LCD_Bitplane][6];
		LCDDR11 = LCD_Data[LCD_Bitplane][7];
		LCDDR12 = LCD_Data[LCD_Bitplane][8];
#ifdef HR25
		LCDDR15 = LCD_Data[LCD_Bitplane][9];
		LCDDR16 = LCD_Data[LCD_Bitplane][10];
		LCDDR17 = LCD_Data[LCD_Bitplane][11];
#endif
	}



	if (LCD_used_bitplanes == 1)
	{
		// only one bitplane used, no blinking
		// Updated; disable LCD start of frame interrupt
		LCDCRA &= ~(1 << LCDIE);
	}
}

/*!
 *******************************************************************************
 *  LCD Interrupt Routine
 *
 *	\note used only for update LCD, in any other cases intterupt is disabled
 *  \note copy LCD_Data to LCDREG
 *
 ******************************************************************************/
#if !TASK_IS_SFR
// not optimized
ISR(LCD_vect)
{
	task |= TASK_LCD;
}
#else
// optimized
ISR_NAKED ISR(LCD_vect)
{
	asm volatile (
		// prologue and epilogue is not needed, this code  not touch flags in SREG
		"	sbi %0,%1""\t\n"
		"	reti""\t\n"
		: : "I" (_SFR_IO_ADDR(task)), "I" (TASK_LCD_BIT)
	);
}
#endif
