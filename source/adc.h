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
 * \file       adc.h
 * \brief      header file for adc.c, functions to control A/D Converter
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>
 * \date       15.03.2008
 * $Rev$
 */


/*****************************************************************************
*   Macros
*****************************************************************************/

// How is the H-Bridge connected to the AVR?
#define ADC_TEMP_MUX       0x02    //!< ADC-Channel for Temp-Sensor (ADMUX)
#define ADC_UB_MUX         0x1E    //!< ADC-Channel for Ub          (ADMUX)
#define ADC_ACT_TEMP       PF3     //!< Bit to activate the TempSensor
#define ADC_ACT_TEMP_P     PORTF   //!< Prot of ADC_ACT_TEMP


/*****************************************************************************
*   Typedefs
*****************************************************************************/

/*****************************************************************************
*   Prototypes
*****************************************************************************/
void ADC_Init(void);                        // Init motor control

void ADC_Measure_Ub(void);                  // Sample Battery Voltage
void ADC_Measure_Temp(void);                // Sample Temperature Value

uint16_t ADC_Get_Temp_Deg(void);            // Get Temperature in 1/100 Deg °C
uint16_t ADC_Get_Temp_Val(void);            // Get Temperature ADC Value 0-1023
uint16_t ADC_Get_Bat_Voltage(void);         // Get Batteriy Voltage in mV
uint16_t ADC_Get_Bat_Val(void);             // Get Batteriy ADC Value 0-1023

uint16_t ADC_Sample_Channel(uint8_t);
