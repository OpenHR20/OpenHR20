/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  compiler:   WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Dario Carluccio (hr20-at-carluccio-dot-de)
 *				2008 Jiri Dobry (jdobry-at-centrum-dot-cz)
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
 * \file       adc.c
 * \brief      functions to control Analog / Digtial - Converter
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>; Jiri Dobry <jdobry-at-centrum-dot-cz>
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


// HR20 Project includes
#include "debug.h"
#include "main.h"
#include "adc.h"
#include "task.h"
#include "common/rtc.h"
#include "eeprom.h"
#include "com.h"

// typedefs

// vars
static uint8_t state_ADC;
bool sleep_with_ADC = 0;


/*!
 *******************************************************************************
 * Ring buffer for last minute
 *
 * make average for last 15 seconds
 * used for calculate 1 minute difference
 ******************************************************************************/

static int16_t ring_buf[2][AVERAGE_LEN];
#if !HW_WINDOW_DETECTION
int16_t ring_buf_temp_avgs [AVGS_BUFFER_LEN];
uint8_t ring_buf_temp_avgs_pos = 0;
#endif

static uint8_t ring_pos = 0;
static uint8_t ring_used = 1;
static int32_t ring_sum [2] = { 0, 0 };
int16_t ring_average [2] = { 0, 0 };

static void shift_ring(void)
{
	ring_pos = (ring_pos + 1) % AVERAGE_LEN;
	if (ring_used < AVERAGE_LEN)
	{
		ring_used++;
	}

#if !HW_WINDOW_DETECTION
	if (ring_pos == 0)
	{
		ring_buf_temp_avgs[ring_buf_temp_avgs_pos] = temp_average;
		ring_buf_temp_avgs_pos = (ring_buf_temp_avgs_pos + 1) % AVGS_BUFFER_LEN;
	}
#endif
}

static void update_ring(uint8_t type, int16_t value)
{
	ring_sum[type] += value;
	ring_sum[type] -= ring_buf[type][ring_pos]; // note for boot: it is OK, ring_buf is initialized to zeroes
	ring_buf[type][ring_pos] = value;
	ring_average[type] = (int16_t)(ring_sum[type] / (int32_t)(ring_used));
}


/*!
 *******************************************************************************
 *  Get battery voltage
 *
 *  \returns battery voltage in mV
 *
 *  \note
 *  - measurment has been performed before using \ref ADC_Measure_Ub
 *  - Battery voltage = (V_ref * 1024) / ADC_Val_Ub with V_ref=1.1V
 ******************************************************************************/
static int16_t ADC_Get_Bat_Voltage(uint16_t adc)             // Get Batteriy Voltage in mV
{
	uint32_t millivolt;

	millivolt = 1126400;
	millivolt /= adc;
	return (int16_t)millivolt;
}


/*!
 *******************************************************************************
 *  convert ACD value to temperature
 *
 *  \returns temperature in 1/100 degrees Celsius
 *
 *  \todo: store values for conversion in EEPROM
 ******************************************************************************/
static int16_t ADC_Convert_To_Degree(int16_t adc)
{
	int16_t dummy;
	uint8_t i;
	int16_t kx = TEMP_CAL_OFFSET + (int16_t)kx_d[0];

	for (i = 1; i < TEMP_CAL_N - 1; i++)
	{
		if (adc < kx + kx_d[i])
		{
			break;
		}
		else
		{
			kx += kx_d[i];
		}
	} // if condintion in loop is not reach i==TEMP_CAL_N-1

	/*! dummy never overload int16_t
	 *  check values for this condition / prevent overload
	 *        values in kx_d[1]..kx_d[TEMP_CAL_N-1] is >=16 see to \ref ee_config
	 *        ADC value is <1024 (OK, only 10-bit AD converter)
	 */
	dummy = (int16_t)(
		(((int32_t)(adc - kx)) * (-TEMP_CAL_STEP))
		/ (int32_t)(kx_d[i])
	);

	dummy += TEMP_CAL_N * TEMP_CAL_STEP - ((int16_t)(i - 1)) * TEMP_CAL_STEP;
#if TEMP_COMPENSATE_OPTION
	dummy += (int16_t)config.room_temp_offset * 10;
#endif

	return dummy;
}


/*!
 *******************************************************************************
 * ADC task
 * \note
 ******************************************************************************/
void start_task_ADC(void)
{
	state_ADC = 1;
	// power up ADC
	power_up_ADC();

	// set ADC control and status register
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADIE);     // prescaler=16

	// free running mode, (not needed, because auto trigger disabled)
	ADCSRB = 0;

	ADMUX = ADC_UB_MUX | (1 << REFS0);
	sleep_with_ADC = 1;
}
#define ADC_TOLERANCE 3
static int16_t dummy_adc = 0;
/*!
 *******************************************************************************
 * ADC task
 ******************************************************************************/
bool task_ADC(void)
{
	int16_t ad;

	switch (state_ADC)
	{
	case 1:                                                                         //step 1
		// set ADC control and status register
		ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS0) | (1 << ADIE);       // prescaler=32
		// ADC conversion from 1 (battery is done)
		// first conversion put to trash
		// start new with same configuration;
		break;
	case 3: //step 3
	{
		ad = ADCW;
		if ((ad > dummy_adc + ADC_TOLERANCE) || (ad < dummy_adc - ADC_TOLERANCE))
		{
			// adc noise protection, repeat measure
REPEAT_ADC:
			dummy_adc = ad;
			sleep_with_ADC = true;
			return true;
		}
#if DEBUG_BATT_ADC
		COM_printStr16(PSTR("batAD x"), ad);
#endif
		update_ring(BAT_RING_TYPE, ADC_Get_Bat_Voltage(ad));

		// activate voltage divider
		ADC_ACT_TEMP_P |= (1 << ADC_ACT_TEMP);
		ADMUX = ADC_TEMP_MUX | (1 << REFS0);
	}
	break;
	case 2: //step 2
	case 4: //step 4
		dummy_adc = ADCW;
		break;
	case 5: //step 5
	{
		ad = ADCW;
		if ((ad > dummy_adc + ADC_TOLERANCE) || (ad < dummy_adc - ADC_TOLERANCE))
		{
			// adc noise protection, repeat measure
			goto REPEAT_ADC; // optimization
		}
		int16_t t = ADC_Convert_To_Degree(ad);
		update_ring(TEMP_RING_TYPE, t);
#if DEBUG_PRINT_MEASURE
		COM_debug_print_temperature(t);
#endif
		shift_ring();
	}
	// do not use break here
	default:
		// deactivate voltage divider
		ADC_ACT_TEMP_P &= ~(1 << ADC_ACT_TEMP);
		// set ADC control and status register / disable ADC
		ADCSRA = (0 << ADEN) | (1 << ADPS2) | (1 << ADPS0) | (1 << ADIE);
		// power down ADC
		power_down_ADC();
		return false;
	}
	state_ADC++;
	sleep_with_ADC = true;
	return true;
}



/*!
 *******************************************************************************
 * ADC Interrupt
 ******************************************************************************/

#if !TASK_IS_SFR
// not optimized
ISR(ADC_vect)
{
	task |= TASK_ADC;
}
#else
// optimized
ISR_NAKED ISR(ADC_vect)
{
	/* note: __zero_reg__ is not used, It can be reused on some other user assembler code */
	asm volatile (
		"	sbi %0,%1""\t\n"
		"	reti""\t\n"
		/* epilogue end */
		: : "I" (_SFR_IO_ADDR(task)), "I" (TASK_ADC_BIT)
	);
}
#endif
