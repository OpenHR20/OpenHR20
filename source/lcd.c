/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  ompiler:    WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Dario Carluccio (hr20-at-carluccio-dot-de)
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
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>
 * \date       24.12.2007
 * $Rev: 35 $
 */

// AVR LibC includes
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/version.h>

// HR20 Project includes
#include "main.h"
#include "lcd.h"

// Local Defines
#define LCD_CONTRAST_MIN       0   //!< \brief contrast minimum
#define LCD_CONTRAST_MAX      15   //!< \brief contrast maxmum
#define LCD_MAX_POS            4   //!< \brief number of 7 segment chars
#define LCD_MAX_CHARS  37   //!< \brief no. of chars in \ref LCD_CharTablePrgMem
#define LCD_BITPLANES          2   //!< \brief two bitplanes for blinking
#define LCD_REGISTER_COUNT     9   //!< \brief no. of registers each bitplane


// Vars
volatile uint8_t LCD_BlinkCounter;   //!< \brief counter for bitplane change
volatile uint8_t LCD_Bitplane;       //!< \brief currently active bitplane
volatile uint8_t LCD_UpdateRequired; //!< \brief LCD Update requested

//! segment data for the segment registers in each bitplane
volatile uint8_t LCD_Data[LCD_BITPLANES][LCD_REGISTER_COUNT];


// Look-up table to convert value to LCD display data (segment control)
// Get value with: bitmap = pgm_read_byte(&LCD_CharTablePrgMem[position]);
uint8_t LCD_CharTablePrgMem[] PROGMEM =
{
    0x3F, //      0:0x3F   1:0x06   2:0x5B   3:0x4F   4:0x66   5:0x6D   6:0x7D
    0x06, // 1    ******        *   ******   ******   *    *   ******   ******
    0x5B, // 2    *    *        *        *        *   *    *   *        *
    0x4F, // 3    *    *        *   ******   ******   ******   ******   ******
    0x66, // 4    *    *        *   *             *        *        *   *    *
    0x6D, // 5    ******        *   ******   ******        *   ******   ******
    0x7D, // 6   0111111  0000110  1011011  1001111  1100110  1101101  1111101

    0x07, // 7    7:0x07   8:0x7F   9:0x6F  10:0x77  11:0x7C  12:0x39  13:0x5E
    0x7F, // 8    ******   ******   ******   ******   *        ******        *
    0x6F, // 9         *   *    *   *    *   *    *   *        *             *
    0x77, //10         *   ******   ******   ******   ******   *        ******
    0x7C, //11         *   *    *        *   *    *   *    *   *        *    *
    0x39, //12         *   ******   ******   *    *   ******   ******   ******
    0x5E, //13   0000111  1111111  1101111  1110111  1111100  0111001  1011110

    0x79, //14   14:0x79  15:0x71  16:0x63  17:0x54  18:0x73  19:0x76 20:0x30
    0x71, //15    ******   ******   ******            ******   *    *  *
    0x63, //16    *        *        *    *            *    *   *    *  *
    0x54, //17    ******   ******   ******   ******   ******   ******  *
    0x73, //18    *        *                 *    *   *        *    *  *
    0x76, //19    ******   *                 *    *   *        *    *  *
    0x30, //20   1111001  1110001  1100011  1010100  1110011  1110110  0110000

    0x08, //21   21:0x08  22:0x40  23:0x01  24:0x48  25:0x41  26:0x11  27:0x49
    0x40, //22                      ******            ******   ******   ******
    0x01, //23
    0x48, //24             ******            ******   ******            ******
    0x41, //25
    0x11, //26    ******                     ******            ******   ******
    0x49, //27   0001000  1000000  0000001  1001000  1000001  0001001  1001001

    0x50, //28   28:0x50  29:0x54  30:0x10  31:0x5c  32:0x00  33:0x33  34:0x27
    0x54, //29                                                 ******   ******
    0x10, //30                                                 *    *   *    *
    0x5c, //31    ******   ******   *        ******   Space    *    *   *    *
    0x00, //32    *        *    *   *        *    *            *             *
    0x33, //33    *        *    *   *        ******            *             *
    0x27, //34   1010000  1010100  0010000  1011100  0000000  0110011  0100111

    0x38  //35   35:0x38
          //36    *
          //37    *
          //38    *
          //39    *
          //40    ******
          //41   0111000
};

// Look-up chars table for weekdays (german)
uint8_t LCD_WeekdayTablePrgMem[7][4] PROGMEM =
{
    {33,34,31,32},    //!<  Montag:     'rno '
    {32,13,30,32},    //!<  Dienstag:   ' di '
    {33,34,30,32},    //!<  Mittwoch:   'rni '
    {32,13,31,32},    //!<  Donnerstag: ' do '
    {32,15,28,32},    //!<  Freitag:    ' Fr '
    {32, 5,10,32},    //!<  Samstag:    ' SA '
    {32, 5,31,32}     //!<  Sonntag:    ' So '
};

// Look-up table to adress element F for one Position. ( 32 : 10 )
uint8_t LCD_FieldOffsetTablePrgMem[] PROGMEM =
{
    40,    //!<  Field 0
    36,    //!<  Field 1              
    31,    //!<  Field 2
    27     //!<  Field 3
};

// Look-up table to adress a segment inside a field
uint8_t LCD_SegOffsetTablePrgMem[] PROGMEM =
{
     2,    //  Seg A            AAAA
     3,    //  Seg B           F    B
    27,    //  Seg C           F    B
    25,    //  Seg D            GGGG
    24,    //  Seg E           E    C
     0,    //  Seg F           E    C
     1     //  Seg G            DDDD
};

//! Look-up table for adress hour-bar segments
uint8_t LCD_SegHourBarOffsetTablePrgMem[] PROGMEM =
{
    LCD_SEG_B0,    LCD_SEG_B1,    LCD_SEG_B2,    LCD_SEG_B3,    LCD_SEG_B4,
    LCD_SEG_B5,    LCD_SEG_B6,    LCD_SEG_B7,    LCD_SEG_B8,    LCD_SEG_B9,
    LCD_SEG_B10,   LCD_SEG_B11,   LCD_SEG_B12,   LCD_SEG_B13,   LCD_SEG_B14,
    LCD_SEG_B15,   LCD_SEG_B16,   LCD_SEG_B17,   LCD_SEG_B18,   LCD_SEG_B19,
    LCD_SEG_B20,   LCD_SEG_B21,   LCD_SEG_B22,   LCD_SEG_B23
};


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
    LCD_AllSegments(false);

    //Set the initial LCD contrast level
    LCD_ContrastAdjust(0);

    // LCD Control and Status Register B
    //   - clock source is TOSC1 pin
    //   - COM0:2 connected
    //   - SEG0:22 connected
	LCDCRB = (1<<LCDCS) | (1<<LCDMUX1) | (1<<LCDPM2)| (1<<LCDPM0);

    // LCD Frame Rate Register
    //   - LCD Prescaler Select N=16       @32.768Hz ->   2048Hz
    //   - LCD Duty Cycle 1/3 (K=6)       2048Hz / 6 -> 341,33Hz
    //   - LCD Clock Divider  (D=5)     341,33Hz / 7 ->  47,76Hz
    LCDFRR = ((1<<LCDCD2)|(1<<LCDCD1));

    // LCD Control and Status Register A
    //  - Enable LCD
    //  - Set Low Power Waveform
    LCDCRA = (1<<LCDEN) | (1<<LCDAB);

    // Enable LCD start of frame interrupt
    LCDCRA |= (1<<LCDIE);

}

/*!
 *******************************************************************************
 *  Init LCD
 *
 *  \note
 *  - increase/decrese contrast
 *  - set LCD Contrast Control Register
 *
 *  \param mode
 *       -  0: set LCD Contrast Control Register to default
 *       -  1: increase contrast by 1<BR>
 *       - -1: decrease contrast by 1<BR>
 *
 *  \returns false if contrast at limit
 ******************************************************************************/
bool LCD_ContrastAdjust (int8_t mode)
{
    uint8_t contrast_level;
    
    // read actual contrast
    contrast_level = LCDCCR & 15;

    // 0: contrast to default value
    if (mode == 0) {
        contrast_level = LCD_CONTRAST_INITIAL;
    }
    // 1: increase contrast
    if (mode == 1) {
        if (contrast_level < LCD_CONTRAST_MAX) {
            contrast_level++;
        } else {
            return false;
        }
    }
    // -1: decrese contrast
    if (mode == -1) {
        if (contrast_level> LCD_CONTRAST_MIN) {
            contrast_level--;
        } else {
            return false;
        }
    }
    // Set LCD Contrast Control Register
    //   - don't care about LCDDC0:2 as they are 0
    LCDCCR = (contrast_level << LCDCC0);
    return true;

}


/*!
 *******************************************************************************
 *  Switch LCD on/off
 *
 *  \param mode
 *       -      0: clears all digits
 *       -  other: set all digits
 ******************************************************************************/
void LCD_AllSegments(uint8_t mode)
{
    uint8_t bp;
    uint8_t i;
    uint8_t val;

    // Set bits in each bitplane
    for (bp=0; bp<LCD_BITPLANES;  bp++){
        if (mode & (1<<bp)){
            val = 0xff;    // Set Bitplane
        } else {
            val = 0x00;    // Clear Bitplane
        }
        for (i=0; i<LCD_REGISTER_COUNT; i++){
                LCD_Data[bp][i] = val;
        }
    }
    LCD_UpdateRequired = true;
}

/*!
 *******************************************************************************
 *  Print char in LCD field
 *
 *  \note  segments insinde one 7 segment array are adressed using adress of
 *         segment "F" \ref LCD_FieldOffsetTablePrgMem[] as base adress adding
 *         \ref LCD_SegOffsetTablePrgMem[] *
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatic at change of bitframe
 *
 *  \param value char to print see \ref LCD_CharTablePrgMem[]
 *  \param pos   position in lcd from left to right <tt> 01 : 23 </tt>
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
    if ((pos < LCD_MAX_POS) && (value < LCD_MAX_CHARS)) {

        // Get Fieldbase for Position
        fieldbase = pgm_read_byte(&LCD_FieldOffsetTablePrgMem[pos]);

        // Get Bitmap for Value
        bitmap = pgm_read_byte(&LCD_CharTablePrgMem[value]);
        mask = 1;

        // Set 7 Bits
        for (i=0; i<7; i++){
            seg = fieldbase + pgm_read_byte(&LCD_SegOffsetTablePrgMem[i]);
            // Set or Clear?
            if (bitmap & mask){
                LCD_SetSeg(seg, mode);
            } else {
                LCD_SetSeg(seg, LCD_MODE_OFF);
            }
            mask <<= 1;
        }
    }
}


/*!
 *******************************************************************************
 *  Print Hex value in LCD field
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatic at change of bitframe
 *
 *  \param value value to be printed (0-0xff)
 *  \param pos   position in lcd 0:left, 1:right
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintHex(uint8_t value, uint8_t pos, uint8_t mode)
{
    uint8_t tmp;
    // Boundary Check
    if (pos < 2) {
        // Start at Pos 0 or 2
        pos *= 2;
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
 *  Print decimal value in LCD field
 *
 *  \note You have to call \ref LCD_Update() to trigger update on LCD if not
 *        it is triggered automatic at change of bitframe
 *
 *  \param value value to be printed (0-99)
 *  \param pos   position in lcd 0:left, 1:right
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintDec(uint8_t value, uint8_t pos, uint8_t mode)
{
    uint8_t tmp;
    // Boundary Check
    if ((pos < 2) && (value < 100)) {
        // Start at Pos 0 or 2
        pos *= 2;
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
 *  Print temperatur on LCD
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatic at change of bitframe
 *
 *  \param temp<BR>
 *     -     0 : \c OFF <BR>
 *     -   255 : \c ON  <BR>
 *     - other : temperature = 4,9°C + (temp/10)
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintTemp(uint8_t temp, uint8_t mode)
{
    uint16_t tmp16;
    uint8_t tmp8;

    // Upper Column off
    LCD_SetSeg(LCD_SEG_COL2, LCD_MODE_OFF);
    
    if (temp==0){
        // OFF
        LCD_PrintChar(LCD_CHAR_0, 3, mode);
        LCD_PrintChar(LCD_CHAR_F, 2, mode);
        LCD_PrintChar(LCD_CHAR_F, 1, mode);
        LCD_PrintChar(LCD_CHAR_NULL, 0, mode);
        LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_OFF);
    } else if (temp==0xff){
        // On
        LCD_PrintChar(LCD_CHAR_0, 3, mode);
        LCD_PrintChar(LCD_CHAR_n, 2, mode);
        LCD_PrintChar(LCD_CHAR_NULL, 1, mode);
        LCD_PrintChar(LCD_CHAR_NULL, 0, mode);
        LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_OFF);
    } else {
        // 21.3C  ( temperature = 4,9 + (temp/10) )
        tmp16 = 49 + (uint16_t) temp;
        tmp8 = (uint8_t) (tmp16 / 10);
        if (tmp8 < 10) {
            LCD_PrintChar(LCD_CHAR_NULL, 3, mode);
            LCD_PrintChar(tmp8, 2, mode);
        } else {
            LCD_PrintDec(tmp8, 1, mode);
        }
        tmp8 = (uint8_t) (tmp16 % 10);
        LCD_PrintChar(tmp8, 1, mode);
        LCD_PrintChar(LCD_CHAR_C, 0, mode);
        LCD_SetSeg(LCD_SEG_COL1, mode);
    }
}


/*!
 *******************************************************************************
 *  Print day of week on LCD
 *
 *  \note  something weird due to 7 Segments
 *
 *  \note  using german format: <tt> Mo, Di, Mi, Do, Fr, Sa, So </tt>
 *
 *  \note You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatic at change of bitframe
 *
 *  \param dow day of week<BR>
 *     -     0 : monday<BR>
 *     -     7 : sunday<BR>
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_PrintDayOfWeek(uint8_t dow, uint8_t mode)
{
    uint8_t i;
    uint8_t tmp;
    // Boundary correction
    dow %= 7;
    // Put 4 chars
    for (i=0; i<4; i++) {
        tmp = pgm_read_byte(&LCD_WeekdayTablePrgMem[dow][i]);
        LCD_PrintChar(tmp, 3-i, mode);
    }
    LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_OFF);
    LCD_SetSeg(LCD_SEG_COL2, LCD_MODE_OFF);
}


/*!
 *******************************************************************************
 *  Set segment of the hour-bar
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatic at change of bitframe
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
 *  Set all segment from left up to val and clear all other segments
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatic at change of bitframe
 *
 *  \param seg No of the last hour bar segment to be set 0-23
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_SetHourBarBar(uint8_t val, uint8_t mode)
{
    uint8_t i;
    // Only Segment 0:23
    if (val > 23){
        val = 23;
    }
    // For each Segment 0:23
    for (i=0; i<24; i++) {
        if (i > val){
            LCD_SetHourBarSeg(i, LCD_MODE_OFF);
        } else {
            LCD_SetHourBarSeg(i, mode);
        } 
    }
}


/*!
 *******************************************************************************
 *  Set only one segment and clear all others
 *
 *  \note You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatic at change of bitframe
 *
 *  \param seg No of the hour bar segment to be set 0-23
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_SetHourBarVal(uint8_t val, uint8_t mode)
{
    uint8_t i;
    // Only Segment 0:23
    if (val > 23){
        val = 23;
    }
    // For each Segment 0:23
    for (i=0; i<24; i++) {
        if (i == val){
            LCD_SetHourBarSeg(i, mode);
        } else {
            LCD_SetHourBarSeg(i, LCD_MODE_OFF);
        }
    }
}


/*!
 *******************************************************************************
 *  Clear LCD Display
 *
 *  \note Sets all Segments of the to \ref LCD_MODE_OFF
 ******************************************************************************/
void LCD_ClearAll(void)
{
    LCD_AllSegments(LCD_MODE_OFF);
}


/*!
 *******************************************************************************
 *  Clear hour-bar 
 *
 *  \note Sets all Hour-Bar Segments to \ref LCD_MODE_OFF
 ******************************************************************************/
void LCD_ClearHourBar(void)
{
    LCD_SetHourBarVal(23, LCD_MODE_OFF);
    LCD_UpdateRequired = true;
}


/*!
 *******************************************************************************
 *  Clear all Symbols 
 *
 *  \note  Sets Symbols <tt> AUTO MANU PROG SUN MOON SNOW</tt>
 *         to \ref LCD_MODE_OFF
 ******************************************************************************/
void LCD_ClearSymbols(void)
{
    LCD_SetSeg(LCD_SEG_AUTO, LCD_MODE_OFF);
    LCD_SetSeg(LCD_SEG_MANU, LCD_MODE_OFF);
    LCD_SetSeg(LCD_SEG_PROG, LCD_MODE_OFF);
    LCD_SetSeg(LCD_SEG_SUN, LCD_MODE_OFF);
    LCD_SetSeg(LCD_SEG_MOON, LCD_MODE_OFF);
    LCD_SetSeg(LCD_SEG_SNOW, LCD_MODE_OFF);
    LCD_UpdateRequired = true;
}


/*!
 *******************************************************************************
 *  Clear all 7 segment fields
 *
 *  \note  Sets the four 7 Segment and the Columns to \ref LCD_MODE_OFF
 ******************************************************************************/
void LCD_ClearNumbers(void)
{
    LCD_PrintChar(LCD_CHAR_NULL, 3, LCD_MODE_OFF);
    LCD_PrintChar(LCD_CHAR_NULL, 2, LCD_MODE_OFF);
    LCD_PrintChar(LCD_CHAR_NULL, 1, LCD_MODE_OFF);
    LCD_PrintChar(LCD_CHAR_NULL, 0, LCD_MODE_OFF);
    LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_OFF);
    LCD_SetSeg(LCD_SEG_COL2, LCD_MODE_OFF);
    LCD_UpdateRequired = true;
}


/*!
 *******************************************************************************
 *  Set segment of LCD
 *
 *  \note  You have to call \ref LCD_Update() to trigger update on LCD if not
 *         it is triggered automatic at change of bitframe
 *
 *  \param seg No of the segment to be set see \ref LCD_SEG_B0 ...
 *  \param mode  \ref LCD_MODE_ON, \ref LCD_MODE_OFF, \ref LCD_MODE_BLINK_1
 ******************************************************************************/
void LCD_SetSeg(uint8_t seg, uint8_t mode)
{
    uint8_t r;
    uint8_t b;
    uint8_t bp;

    // Register = segment DIV 8
    r = seg / 8;
    // Bitposition = segment mod 8
    b = seg % 8;

    // Set bits in each bitplane
    for (bp=0; bp<LCD_BITPLANES;  bp++){
        if (mode & (1<<bp)){
            // Set Bit in Bitplane if ON (0b11) or Blinkmode 1 (0b01)
            LCD_Data[bp][r] |= (1<<b);
        } else {
            // Clear Bit in Bitplane if OFF (0b00) or Blinkmode 2 (0b10)
            LCD_Data[bp][r] &= ~(1<<b);
        }
    }
}


/*!
 *******************************************************************************
 *  Request LCD Update immediately
 *
 *  \note  update it is triggered automaticaly at change of each bitframe
 *
 *  \note  Must be called after usage of \ref LCD_SetSeg and \ref LCD_PrintChar
 *         to trigger update of LCDDR Registers
 ******************************************************************************/
void LCD_Update(void)
{
    // Update at next LCD_ISR
    LCD_UpdateRequired = true;
}


/*!
 *******************************************************************************
 *  LCD Interrupt Routine
 *
 *  \note copy LCD_Data to LCDREG
 *
 *  \note cycle bitplanes every LCD_BLINK_FRAMES
 ******************************************************************************/
ISR(LCD_vect)
{
    // Bitplane 0 active
    if (LCD_BlinkCounter == 0) {
        LCD_Bitplane = 0;
        LCD_UpdateRequired = true;
    }
    // Bitplane 1 active
    if (LCD_BlinkCounter == LCD_BLINK_FRAMES) {
        LCD_Bitplane = 1;
        LCD_UpdateRequired = true;
    }
    if (LCD_UpdateRequired){
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
    }
    // inc Blink Counter
    LCD_BlinkCounter++;
    // Restart Blink Counter
    if (LCD_BlinkCounter > (2*LCD_BLINK_FRAMES)){
        LCD_BlinkCounter=0;
    }

}
