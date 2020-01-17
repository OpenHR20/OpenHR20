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
 * \date       $Date$
 * $Rev$
 */

#pragma once

/*****************************************************************************
*   Macros
*****************************************************************************/

// hardware configuration
#if THERMOTRONIC == 1
#define ADC_TEMP_MUX       0x00         //!< ADC-Channel for Temp-Sensor (ADMUX)
#define ADC_UB_MUX         0x1E         //!< ADC-Channel for Ub          (ADMUX)
#define ADC_ACT_TEMP       PF2          //!< Bit to activate the TempSensor
#else
#define ADC_TEMP_MUX       0x02         //!< ADC-Channel for Temp-Sensor (ADMUX)
#define ADC_UB_MUX         0x1E         //!< ADC-Channel for Ub          (ADMUX)
#define ADC_ACT_TEMP       PF3          //!< Bit to activate the TempSensor
#endif
#define ADC_ACT_TEMP_P     PORTF        //!< Prot of ADC_ACT_TEMP

#define TEMP_RING_TYPE 1
#define BAT_RING_TYPE 0
#define temp_average (ring_average[TEMP_RING_TYPE])
#define bat_average (ring_average[BAT_RING_TYPE])
#define AVGS_BUFFER_LEN (4 * 8) // 4 per minute * 8
#define AVERAGE_LEN 15

#if THERMOTRONIC == 1
#define TEMP_CAL_OFFSET 380     // offset of calibration points [ADC units]
#else
#define TEMP_CAL_OFFSET 256     // offset of calibration points [ADC units]
#endif
#define TEMP_CAL_STEP 500       // step between 2 calibration points [1/100°C]
#define TEMP_CAL_N 7            // // No. Values


/*****************************************************************************
*   Typedefs
*****************************************************************************/

/*****************************************************************************
*   Prototypes
*****************************************************************************/

bool task_ADC(void);
void start_task_ADC(void);


extern bool sleep_with_ADC;
extern int16_t ring_average[];
extern int16_t ring_difference[];
extern int16_t ring_buf_temp_avgs [AVGS_BUFFER_LEN];
extern uint8_t ring_buf_temp_avgs_pos;
