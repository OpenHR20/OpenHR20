/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  ompiler:    WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Juergen Sachs (juergen-sachs-at-gmx-dot-de)
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
 * \file       engine.c
 * \brief      
 * \author     Juergen Sachs (juergen-sachs-at-gmx-dot-de)
 * \date       13.08.2008
 * $Rev$
 */

// AVR LibC includes
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/version.h>
#include <stdlib.h>
#include <string.h>

// HR20 Project includes
#include "main.h"
#include "engine.h"

struct serState st;

/*
Meassure all values and store them in internal structures with min and max values
Also basic control should be placed here, like valve positioning
*/
void e_meassure(void)
{
	static uint8_t lastsec;
	static uint8_t state;
	register uint8_t time = RTC_GetSecond();
	register uint16_t temp;
	
	time = time >> 1;	// We are not interested in every second :-)
	if (lastsec != time)
	{
		lastsec = time;
		state++;
		switch(state)
		{
			case 1:
			{
				ADC_Measure_Ub();
				temp = ADC_Get_Temp_Degree();	// Get temp to make shure it is converted already
				if (temp != st.tempCur)
				{
					st.tempCur = temp;
					COM_setNotify(NOTIFY_TEMP);
				}
				if (temp > st.tempMax)
				{
					st.tempMax = temp;
					COM_setNotify(NOTIFY_TEMP);
				}
				if (temp < st.tempMin)
				{
					st.tempMin = temp;
					COM_setNotify(NOTIFY_TEMP);
				}
				break;
			}
			case 2:
			{
				ADC_Measure_Temp();
				uint16_t volt = ADC_Get_Bat_Voltage();	// Get voltage to make shure it is converted already
				if (st.volt != volt)
				{
					st.volt = volt;
					COM_setNotify(NOTIFY_BATT);

				}
				break;
			}
			default:
			{
				state = 0;
				break;
			}
		};
	}
}

void e_Init(void)
{
	e_resetTemp();
}

void e_resetTemp(void)
{
	register uint16_t temp = ADC_Get_Temp_Degree();
	st.tempCur = temp;
	st.tempMax = temp;
	st.tempMin = temp;
}


