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
 * \file       lcd.h
 * \brief      header file for lcd.c, functions to control the HR20 LCD
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>
 * \date       $Date$
 * $Rev$
 */

#pragma once

/*****************************************************************************
*   Macros
*****************************************************************************/

// Modes for LCD_SetSymbol
#define LCD_MODE_OFF           0        //!< (0b00) segment off
#define LCD_MODE_BLINK_1       1        //!< (0b01) segment on during 1. frame (blinking)
#define LCD_MODE_BLINK_2       2        //!< (0b10) segment on during 2. frame (blinking)
#define LCD_MODE_ON            3        //!< (0b11) segment permanent on

#define LCD_CONTRAST_INITIAL  14        //!< initial LCD contrast (0-15)
#define LCD_BLINK_FRAMES      12        //!< refreshes for each frame @ 48 frames/s; 12 refreshes -> 4Hz Blink frequency
#define LCD_BITPLANES          2        //!< \brief two bitplanes for blinking

/*****************************************************************************
*   Global Vars
*****************************************************************************/
extern volatile uint8_t LCD_used_bitplanes;     //!< \brief number of used bitplanes / used for power
extern uint8_t LCD_force_update;                //!< \brief force update LCD

/*****************************************************************************
*   Prototypes
*****************************************************************************/

void LCD_Init(void);                                            // Init the LCD Controller
void LCD_AllSegments(uint8_t);                                  // Set all segments to LCD_MODE

void LCD_PrintDec(uint8_t, uint8_t, uint8_t);                   // Print DEC-val (0-99)
void LCD_PrintDec3(uint16_t value, uint8_t pos, uint8_t mode);  // Print DEC-val (0-255)
void LCD_PrintDecW(uint16_t, uint8_t);                          // Print DEC-val (0-9999)
void LCD_PrintHex(uint8_t, uint8_t, uint8_t);                   // Print HEX-val (0-ff)
void LCD_PrintHexW(uint16_t, uint8_t);                          // Print HEX-val (0-ffff)
void LCD_PrintChar(uint8_t, uint8_t, uint8_t);                  // Print one digit
void LCD_PrintTemp(uint8_t, uint8_t);                           // Print temperature (val+4,9)°C
void LCD_PrintTempInt(int16_t, uint8_t);                        // Print temperature (val/100)°C
void LCD_PrintDayOfWeek(uint8_t, uint8_t);                      // Print Day of Week (german)
void LCD_PrintStringID(uint8_t id, uint8_t mode);               // Print LCD string ID
#define LCD_PrintDayOfWeek(dow, mode) (LCD_PrintStringID(dow, mode))

void LCD_SetSeg(uint8_t, uint8_t);              // Set one Segment (0-69)
void LCD_SetSegReg(uint8_t, uint8_t, uint8_t);  // Set multiple LCD Segment(s) with common register
void LCD_SetHourBarSeg(uint8_t, uint8_t);       // Set HBS (0-23) (Hour-Bar-Segment)
void LCD_HourBarBitmap(uint32_t bitmap);        // Set HBS like bitmap
void task_lcd_update(void);

#define  LCD_Update()  ((LCDCRA |= (1 << LCDIE)), (LCD_force_update = 1))
// Update at next LCD_ISR
// Enable LCD start of frame interrupt



//***************************
// LCD Chars:
//***************************
#define LCD_CHAR_0       0      //!< char "0"
#define LCD_CHAR_1       1      //!< char "1"
#define LCD_CHAR_2       2      //!< char "2"
#define LCD_CHAR_3       3      //!< char "3"
#define LCD_CHAR_4       4      //!< char "4"
#define LCD_CHAR_5       5      //!< char "5"
#define LCD_CHAR_6       6      //!< char "6"

#define LCD_CHAR_7       7      //!< char "7"
#define LCD_CHAR_8       8      //!< char "8"
#define LCD_CHAR_9       9      //!< char "9"
#define LCD_CHAR_A      10      //!< char "A"
#define LCD_CHAR_b      11      //!< char "b"
#define LCD_CHAR_C      12      //!< char "C"
#define LCD_CHAR_d      13      //!< char "d"

#define LCD_CHAR_E      14      //!< char "E"
#define LCD_CHAR_F      15      //!< char "F"
#define LCD_CHAR_deg    16      //!< symbol degree
#define LCD_CHAR_n      17      //!< char "n"
#define LCD_CHAR_P      18      //!< char "P"
#define LCD_CHAR_H      19      //!< char "H"
#define LCD_CHAR_I      20      //!< char "I"
#define LCD_CHAR_neg    22      //!< char "-"
#define LCD_CHAR_2lines 26      //!< line on top, line on bottom
#define LCD_CHAR_3lines 27      //!< 3 horizontal lines
#define LCD_CHAR_r      28      //!< char "r"
#define LCD_CHAR_o      31      //!< char "r"
#define LCD_CHAR_L      35      //!< char "L"
#define LCD_CHAR_c      36      //!< char "c"
#define LCD_CHAR_U      37      //!< char "U"
#define LCD_CHAR_t      38      //!< char "t"
#define LCD_CHAR_y      39      //!< char "y"
#define LCD_CHAR_S       5      //!< char "5" = "S"

#define LCD_CHAR_NULL  32       //!< space

/*! \verbatim
 *******************************************************************************
 *  LCD Layout:
 *
 *        B|B|B|B|B|B|B|B|B|B|B |B |B |B |B |B |B |B |B |B |B |B |B |B |
 *        0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|
 *
 *        ---------------------------BAR24------------------------------
 *
 *      Auto      3A         2A                  1A         0A       SUN
 *             3F    3B   2F    2B            1F    1B   0F    0B
 *      Manu      3g         2g       Col2       1g         0g       MOON
 *             3E    3C   2E    2C            1E    1C   0E    0C
 *      Prog      3D         2D       Col1       1D         0D       STAR
 *
 *******************************************************************************
 * \endverbatim   */

#if THERMOTRONIC == 1
// LCD_SEG:_xx for LCD_SetSeg   // LCDDR | AtMega169 |  LCD_Data[]
#define LCD_SEG_B0          1   //  0, 0 |  SEG000   |  [0], BIT 0
                                //  0, 1 |  SEG001   |  [0], BIT 1
#define LCD_SEG_B1          2   //  0, 2 |  SEG002   |  [0], BIT 2
#define LCD_SEG_B2          3   //  0, 3 |  SEG003   |  [0], BIT 3
#define LCD_SEG_B3          4   //  0, 4 |  SEG004   |  [0], BIT 4
#define LCD_SEG_B4          5   //  0, 5 |  SEG005   |  [0], BIT 5
#define LCD_SEG_B5          6   //  0, 6 |  SEG006   |  [0], BIT 6
#define LCD_SEG_B6          7   //  0, 7 |  SEG007   |  [0], BIT 7
#define LCD_SEG_B7          8   //  1, 0 |  SEG008   |  [1], BIT 0
#define LCD_SEG_B8          9   //  1, 1 |  SEG009   |  [1], BIT 1
#define LCD_SEG_B9         10   //  1, 2 |  SEG010   |  [1], BIT 2
#define LCD_SEG_BAR24       0   //  1, 3 |  SEG011   |  [1], BIT 3
#define LCD_SEG_B14        11   //  1, 4 |  SEG012   |  [1], BIT 4
#define LCD_SEG_B15        12   //  1, 5 |  SEG013   |  [1], BIT 5
#define LCD_SEG_B16        13   //  1, 6 |  SEG014   |  [1], BIT 6
#define LCD_SEG_B17        14   //  1, 7 |  SEG015   |  [1], BIT 7
#define LCD_SEG_B18        15   //  2, 0 |  SEG016   |  [2], BIT 0
#define LCD_SEG_B19        16   //  2, 1 |  SEG017   |  [2], BIT 1
#define LCD_SEG_B20        17   //  2, 2 |  SEG018   |  [2], BIT 2
#define LCD_SEG_B21        18   //  2, 3 |  SEG019   |  [2], BIT 3
#define LCD_SEG_B22        19   //  2, 4 |  SEG020   |  [2], BIT 4
#define LCD_SEG_B23        20   //  2, 5 |  SEG021   |  [2], BIT 5
//*****************************************************************
#define LCD_SEG_AUTO       25   //  5, 0 |  SEG100   |  [3], BIT 0
                                //  5, 1 |  SEG101   |  [3], BIT 1
#define LCD_SEG_PROG       26   //  5, 2 |  SEG102   |  [3], BIT 2
#define LCD_SEG_3F         27   //  5, 3 |  SEG103   |  [3], BIT 3
#define LCD_SEG_3G         28   //  5, 4 |  SEG104   |  [3], BIT 4
#define LCD_SEG_3A         29   //  5, 5 |  SEG105   |  [3], BIT 5
#define LCD_SEG_3B         30   //  5, 6 |  SEG106   |  [3], BIT 6
#define LCD_SEG_2F         31   //  5, 7 |  SEG107   |  [3], BIT 7
#define LCD_SEG_2G         32   //  6, 0 |  SEG108   |  [4], BIT 0
#define LCD_SEG_2A         33   //  6, 1 |  SEG109   |  [4], BIT 1
#define LCD_SEG_2B         34   //  6, 2 |  SEG110   |  [4], BIT 2
#define LCD_SEG_COL2       24   //  6, 3 |  SEG111   |  [4], BIT 3
#define LCD_SEG_1F         35   //  6, 4 |  SEG112   |  [4], BIT 4
#define LCD_SEG_1G         36   //  6, 5 |  SEG113   |  [4], BIT 5
#define LCD_SEG_1A         37   //  6, 6 |  SEG114   |  [4], BIT 6
#define LCD_SEG_1B         38   //  6, 7 |  SEG115   |  [4], BIT 7
#define LCD_SEG_0F         39   //  7, 0 |  SEG116   |  [5], BIT 0
#define LCD_SEG_0G         40   //  7, 1 |  SEG117   |  [5], BIT 1
#define LCD_SEG_0A         41   //  7, 2 |  SEG118   |  [5], BIT 2
#define LCD_SEG_0B         42   //  7, 3 |  SEG119   |  [5], BIT 3
#define LCD_SEG_SNOW       43   //  7, 4 |  SEG120   |  [5], BIT 4
#define LCD_SEG_SUN        44   //  7, 5 |  SEG121   |  [5], BIT 5
//*****************************************************************
//                                 10, 0 |  SEG200   |  [6], BIT 0
//                                 10, 1 |  SEG201   |  [6], BIT 1
#define LCD_SEG_MANU       50   // 10, 2 |  SEG202   |  [6], BIT 2
#define LCD_SEG_3E         51   // 10, 3 |  SEG203   |  [6], BIT 3
#define LCD_SEG_3D         52   // 10, 4 |  SEG204   |  [6], BIT 4
#define LCD_SEG_B11        53   // 10, 5 |  SEG205   |  [6], BIT 5
#define LCD_SEG_3C         54   // 10, 6 |  SEG206   |  [6], BIT 6
#define LCD_SEG_2E         55   // 10, 7 |  SEG207   |  [6], BIT 7
#define LCD_SEG_2D         56   // 11, 0 |  SEG208   |  [7], BIT 0
#define LCD_SEG_B10        57   // 11, 1 |  SEG209   |  [7], BIT 1
#define LCD_SEG_2C         58   // 11, 2 |  SEG210   |  [7], BIT 2
#define LCD_SEG_COL1       48   // 11, 3 |  SEG211   |  [7], BIT 3
#define LCD_SEG_1E         59   // 11, 4 |  SEG212   |  [7], BIT 4
#define LCD_SEG_1D         60   // 11, 5 |  SEG213   |  [7], BIT 5
#define LCD_SEG_B13        61   // 11, 6 |  SEG214   |  [7], BIT 6
#define LCD_SEG_1C         62   // 11, 7 |  SEG215   |  [7], BIT 7
#define LCD_SEG_0E         63   // 12, 0 |  SEG216   |  [8], BIT 0
#define LCD_SEG_0D         64   // 12, 1 |  SEG217   |  [8], BIT 1
#define LCD_SEG_B12        65   // 12, 2 |  SEG218   |  [8], BIT 2
#define LCD_SEG_0C         66   // 12, 3 |  SEG219   |  [8], BIT 3
                                // 12, 4 |  SEG220   |  [8], BIT 4
#define LCD_SEG_MOON       68   // 12, 5 |  SEG221   |  [8], BIT 5
//*****************************************************************

#define LCD_DECIMAL_DOT     LCD_SEG_COL1

#else

#ifdef HR25
/*! \verbatim
 *******************************************************************************
 *  LCD Layout:
 *        ---------------------------BAR24------------------------------
 *
 *        0|1|2|3|4|5|6|7|8|9|0a|0b|0c|0d|0e|B |0f|B |B |B |B |B |B |B |
 *        0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|
 *        --------------------------BAR_SUB-----------------------------
 *
 *  MOON SUN SUPER2 D1   D2   D3      D4       D5    D6   D7         PADLOCK
 *  BATOUT SPANNER
 *  BAT1,2 SNOW   3A         2A                  1A         0A       DEGREE CELCIUS
 *      Prog   3F    3B   2F    2B    Col2    1F    1B   0F    0B    PERCENT
 *      Auto      3g         2g       Col1       1g         0g
 *      Eco    3E    3C   2E    2C            1E    1C   0E    0C    MINUS/VERTPLUS
 *      Manu      3D   Col3  2D       Col4       1D   Col5  0D       DOOROPEN
 *
 *******************************************************************************
 * \endverbatim   */
#define LCD_SEG_B0          0
#define LCD_SEG_B1          1
#define LCD_SEG_B2          2
#define LCD_SEG_B3          3
#define LCD_SEG_B4          4
#define LCD_SEG_B5          5
#define LCD_SEG_B6          6
#define LCD_SEG_B7          7

#define LCD_SEG_B8          8
#define LCD_SEG_B9          9
#define LCD_SEG_B10        10
#define LCD_SEG_B11        11
#define LCD_SEG_B12        12
#define LCD_SEG_B13        13
#define LCD_SEG_B14        14
#define LCD_SEG_B16        15

#define LCD_SEG_B17        16
#define LCD_SEG_B20        17
#define LCD_SEG_B21        18
#define LCD_SEG_BAR_SUB    19
#define LCD_SEG_B23        20

#define LCD_SEG_ECO        24
#define LCD_SEG_PROG       25
#define LCD_SEG_3F         26
#define LCD_SEG_3G         27
#define LCD_SEG_3A         28
#define LCD_SEG_3B         29
#define LCD_SEG_2F         30
#define LCD_SEG_2G         31


#define LCD_SEG_2A         32
#define LCD_SEG_2B         33
#define LCD_SEG_COL2       34
#define LCD_SEG_1F         35
#define LCD_SEG_1G         36
#define LCD_SEG_1A         37
#define LCD_SEG_1B         38
#define LCD_SEG_0F         39

#define LCD_SEG_0G         40
#define LCD_SEG_0A         41
#define LCD_SEG_0B         42
#define LCD_SEG_MINUS      43
#define LCD_DEGREE         44
#define LCD_PADLOCK        45

#define LCD_SEG_MANU       48
#define LCD_SEG_AUTO       49
#define LCD_SEG_3E         50
#define LCD_SEG_3D         51
#define LCD_SEG_3C         52
#define LCD_SEG_COL3       53
#define LCD_SEG_2E         54
#define LCD_SEG_2D         55

#define LCD_SEG_2C         56
#define LCD_SEG_COL4       57
#define LCD_SEG_COL1       58
#define LCD_SEG_1E         59
#define LCD_SEG_1D         60
#define LCD_SEG_1C         61
#define LCD_SEG_COL5       62
#define LCD_SEG_0E         63

#define LCD_SEG_0D         64
#define LCD_SEG_0C         65
#define LCD_SEG_DOOR_OPEN  66
#define LCD_SEG_VERTPLUS   67
#define LCD_SEG_PERCENT    68
#define LCD_SEG_CELCIUS    69

#define LCD_SEG_BAT_OUTLINE 72
#define LCD_SEG_BAT_TOP     73  // BAT1
#define LCD_SEG_BAT_BOTTOM  74  // BAT2
#define LCD_SEG_SNOW        75
#define LCD_SEG_SPANNER     76
#define LCD_SEG_MOON        77
#define LCD_SEG_SUN         78
#define LCD_SEG_SUPER2      79

#define LCD_SEG_D1          80
#define LCD_SEG_D2          81
#define LCD_SEG_D3          82
#define LCD_SEG_D4          83
#define LCD_SEG_D5          84
#define LCD_SEG_D6          85
#define LCD_SEG_D7          86
#define LCD_SEG_B15         87

#define LCD_SEG_B18         88
#define LCD_SEG_B19         89
#define LCD_SEG_B22         90
#define LCD_SEG_BAR24       91

#define LCD_DECIMAL_DOT     LCD_SEG_COL4

#else /* HR20 */

// LCD_SEG:_xx for LCD_SetSeg   // LCDDR | AtMega169 |  LCD_Data[]
#define LCD_SEG_B0          0   //  0, 0 |  SEG000   |  [0], BIT 0
                                //  0, 1 |  SEG001   |  [0], BIT 1
#define LCD_SEG_B1          2   //  0, 2 |  SEG002   |  [0], BIT 2
#define LCD_SEG_B2          3   //  0, 3 |  SEG003   |  [0], BIT 3
#define LCD_SEG_B3          4   //  0, 4 |  SEG004   |  [0], BIT 4
#define LCD_SEG_B4          5   //  0, 5 |  SEG005   |  [0], BIT 5
#define LCD_SEG_B5          6   //  0, 6 |  SEG006   |  [0], BIT 6
#define LCD_SEG_B6          7   //  0, 7 |  SEG007   |  [0], BIT 7
#define LCD_SEG_B7          8   //  1, 0 |  SEG008   |  [1], BIT 0
#define LCD_SEG_B8          9   //  1, 1 |  SEG009   |  [1], BIT 1
#define LCD_SEG_B9         10   //  1, 2 |  SEG010   |  [1], BIT 2
#define LCD_SEG_BAR24      11   //  1, 3 |  SEG011   |  [1], BIT 3
#define LCD_SEG_B14        12   //  1, 4 |  SEG012   |  [1], BIT 4
#define LCD_SEG_B15        13   //  1, 5 |  SEG013   |  [1], BIT 5
#define LCD_SEG_B16        14   //  1, 6 |  SEG014   |  [1], BIT 6
#define LCD_SEG_B17        15   //  1, 7 |  SEG015   |  [1], BIT 7
#define LCD_SEG_B18        16   //  2, 0 |  SEG016   |  [2], BIT 0
#define LCD_SEG_B19        17   //  2, 1 |  SEG017   |  [2], BIT 1
#define LCD_SEG_B20        18   //  2, 2 |  SEG018   |  [2], BIT 2
#define LCD_SEG_B21        19   //  2, 3 |  SEG019   |  [2], BIT 3
#define LCD_SEG_B22        20   //  2, 4 |  SEG020   |  [2], BIT 4
#define LCD_SEG_B23        21   //  2, 5 |  SEG021   |  [2], BIT 5
//*****************************************************************
#define LCD_SEG_AUTO       24   //  5, 0 |  SEG100   |  [3], BIT 0
                                //  5, 1 |  SEG101   |  [3], BIT 1
#define LCD_SEG_PROG       26   //  5, 2 |  SEG102   |  [3], BIT 2
#define LCD_SEG_3F         27   //  5, 3 |  SEG103   |  [3], BIT 3
#define LCD_SEG_3G         28   //  5, 4 |  SEG104   |  [3], BIT 4
#define LCD_SEG_3A         29   //  5, 5 |  SEG105   |  [3], BIT 5
#define LCD_SEG_3B         30   //  5, 6 |  SEG106   |  [3], BIT 6
#define LCD_SEG_2F         31   //  5, 7 |  SEG107   |  [3], BIT 7
#define LCD_SEG_2G         32   //  6, 0 |  SEG108   |  [4], BIT 0
#define LCD_SEG_2A         33   //  6, 1 |  SEG109   |  [4], BIT 1
#define LCD_SEG_2B         34   //  6, 2 |  SEG110   |  [4], BIT 2
#define LCD_SEG_COL2       35   //  6, 3 |  SEG111   |  [4], BIT 3
#define LCD_SEG_1F         36   //  6, 4 |  SEG112   |  [4], BIT 4
#define LCD_SEG_1G         37   //  6, 5 |  SEG113   |  [4], BIT 5
#define LCD_SEG_1A         38   //  6, 6 |  SEG114   |  [4], BIT 6
#define LCD_SEG_1B         39   //  6, 7 |  SEG115   |  [4], BIT 7
#define LCD_SEG_0F         40   //  7, 0 |  SEG116   |  [5], BIT 0
#define LCD_SEG_0G         41   //  7, 1 |  SEG117   |  [5], BIT 1
#define LCD_SEG_0A         42   //  7, 2 |  SEG118   |  [5], BIT 2
#define LCD_SEG_0B         43   //  7, 3 |  SEG119   |  [5], BIT 3
#define LCD_SEG_SNOW       44   //  7, 4 |  SEG120   |  [5], BIT 4
#define LCD_SEG_SUN        45   //  7, 5 |  SEG121   |  [5], BIT 5
//*****************************************************************
//                                 10, 0 |  SEG200   |  [6], BIT 0
//                                 10, 1 |  SEG201   |  [6], BIT 1
#define LCD_SEG_MANU       50   // 10, 2 |  SEG202   |  [6], BIT 2
#define LCD_SEG_3E         51   // 10, 3 |  SEG203   |  [6], BIT 3
#define LCD_SEG_3D         52   // 10, 4 |  SEG204   |  [6], BIT 4
#define LCD_SEG_B11        53   // 10, 5 |  SEG205   |  [6], BIT 5
#define LCD_SEG_3C         54   // 10, 6 |  SEG206   |  [6], BIT 6
#define LCD_SEG_2E         55   // 10, 7 |  SEG207   |  [6], BIT 7
#define LCD_SEG_2D         56   // 11, 0 |  SEG208   |  [7], BIT 0
#define LCD_SEG_B10        57   // 11, 1 |  SEG209   |  [7], BIT 1
#define LCD_SEG_2C         58   // 11, 2 |  SEG210   |  [7], BIT 2
#define LCD_SEG_COL1       59   // 11, 3 |  SEG211   |  [7], BIT 3
#define LCD_SEG_1E         60   // 11, 4 |  SEG212   |  [7], BIT 4
#define LCD_SEG_1D         61   // 11, 5 |  SEG213   |  [7], BIT 5
#define LCD_SEG_B13        62   // 11, 6 |  SEG214   |  [7], BIT 6
#define LCD_SEG_1C         63   // 11, 7 |  SEG215   |  [7], BIT 7
#define LCD_SEG_0E         64   // 12, 0 |  SEG216   |  [8], BIT 0
#define LCD_SEG_0D         65   // 12, 1 |  SEG217   |  [8], BIT 1
#define LCD_SEG_B12        66   // 12, 2 |  SEG218   |  [8], BIT 2
#define LCD_SEG_0C         67   // 12, 3 |  SEG219   |  [8], BIT 3
                                // 12, 4 |  SEG220   |  [8], BIT 4
#define LCD_SEG_MOON       69   // 12, 5 |  SEG221   |  [8], BIT 5
//*****************************************************************

#define LCD_DECIMAL_DOT     LCD_SEG_COL1

#endif  /*HR25*/
#endif  /*!THERMOTRONIC*/

//***************************
// LCD Strings:
//***************************

#define LCD_STRING_bloc         8
#define LCD_STRING_4xminus      9
#define LCD_STRING_minusCminus 10
#define LCD_STRING_Err         11
#define LCD_STRING_OFF         12
#define LCD_STRING_On          13
#define LCD_STRING_OPEn        14
#define LCD_STRING_BAtt        15
#define LCD_STRING_E2          16
#define LCD_STRING_E3          17
#define LCD_STRING_E4          18
#define LCD_STRING_EEPr        19
