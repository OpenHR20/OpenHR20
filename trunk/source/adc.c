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
 * \date       02.10.2008
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
#include "main.h"
#include "adc.h"
#include "task.h"

// typedefs

// vars
static uint8_t state_ADC;
volatile uint8_t sleep_with_ADC=0;


/*!
 *******************************************************************************
 * Ring buffer for last minute
 * 
 * make average for  last 15 second
 * used for calculate 1 minute difference
 ******************************************************************************/
	
#define BUFFER_LEN 60
#define AVERAGE_LEN 15
#if (BUFFER_LEN<AVERAGE_LEN)
	#error AVERAGE_LEN len must be less than BUFFER_LEN
#endif
static int16_t ring_buf [2][BUFFER_LEN];
static uint8_t ring_pos=0;
static uint8_t ring_used=1; 
static int32_t ring_sum [2] = {0,0};
int32_t ring_average [2] = {0,0};
int16_t ring_difference [2] = {INT16_MAX,INT16_MAX}; //INT16_MAX is error value;

static void shift_ring(void) {
	ring_pos = (ring_pos+1) % BUFFER_LEN;
	if (ring_used<BUFFER_LEN) {
		ring_used++;
	}
}

static void update_ring(uint8_t type, int16_t value) {

	ring_sum[type]+=value;
	if (ring_used>=AVERAGE_LEN) {
		ring_sum[type]-=ring_buf[type][(ring_pos+BUFFER_LEN-AVERAGE_LEN) % BUFFER_LEN];
	}
	if (ring_used>=BUFFER_LEN) {
		ring_difference[type] = value - ring_buf[type][ring_pos];
	}
		
	ring_buf[type][ring_pos]=value;
	ring_average[type] = (int16_t) (ring_sum[type]
		/(int32_t)((ring_used>=AVERAGE_LEN)?AVERAGE_LEN:ring_used));
}


/*!
 *******************************************************************************
 * Values for ACD to Temperatur conversion. 
 * \todo stored in EEPROM, so they can be adapted later over cfg interface
 ******************************************************************************/
uint8_t  kz = 7;                                          // No. Values
uint16_t kx[]={ 304,  340,  397,  472,  549,  614, 675};  // ADC Values
uint16_t ky[]={3400, 3000, 2500, 2000, 1500, 1000, 500};  // Temp Values [1/100°C]



/*!
 *******************************************************************************
 *  Get temperature
 *
 *  \returns temperature in 1/100 degrees Celsius (1987: 19,87°C)
 *
 *  \note
 *  - measurment has been performed before using \ref ADC_Measure_Temp
 *  - Attention: negative values are possible 
 ******************************************************************************/
#if 0
int16_t ADC_Get_Temp_Degree(void)            // Get Temperature in 1/100 Deg °C
{
    int32_t degree;

    degree = ADC_Convert_To_Degree();    
     
    if (degree < INT16_MIN){
        return (INT16_MIN);
    }else if (degree > INT16_MAX){ 
        return (INT16_MAX);
    }else{ 
        return ((int16_t) degree);        
    }
}
#endif
 

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
int16_t ADC_Get_Bat_Voltage(uint16_t adc)             // Get Batteriy Voltage in mV
{
    uint32_t millivolt;
    millivolt = 1126400;
    millivolt /= adc; 
    return ((int16_t) millivolt);
}


/*!
 *******************************************************************************
 *  Get Batteriy status
 *
 *  \returns false if battery is low
 *
 *  \note
 *  - none
 ******************************************************************************/
#if 0
bool ADC_Get_Bat_isOk(void)
{
	return (ADC_Get_Bat_Voltage() > ADC_LOW_BATT_LEVEL);
}
#endif


/*!
 *******************************************************************************
 *  convert ACD value to temperatur 
 *
 *  \returns temperatur in 1/100 degrees Celsius
 *
 *  \todo: store values for conversion in EEPROM 
 ******************************************************************************/
int16_t ADC_Convert_To_Degree(uint16_t adc)
{
    int32_t dummy;
    uint8_t i;

    for (i=1; i<kz; i++){
        if (adc<kx[i]){
            break;
        }        
    }

    dummy =  ((int32_t) ky[i] - (int32_t) ky[i-1]);
	dummy *= ((int32_t) adc - (int32_t) kx[i-1]);
    dummy /= ((int32_t) kx[i] - (int32_t) kx[i-1]);
    dummy += (int32_t)  ky[i-1];

    if (dummy < INT16_MIN){
        return (INT16_MIN);
    }else if (dummy > INT16_MAX){ 
        return (INT16_MAX);
    }else{ 
        return ((int16_t) dummy);        
    }
}




/*!
 *******************************************************************************
 * ADC task
 * \note
 ******************************************************************************/
void start_task_ADC(void) {
	state_ADC=1;
    // set reference
    ADMUX = (1<<REFS0); //AVCC with external capacitor at AREF pin

    // set ADC control and status register
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS0)|(1<<ADIE);         // prescaler=32
    
    // free running mode, (not needed, because auto trigger disabled)
    ADCSRB = 0;

	ADMUX = ADC_UB_MUX | (1<<REFS0);
	sleep_with_ADC=1;
}

/*!
 *******************************************************************************
 * ADC task
 ******************************************************************************/
uint8_t task_ADC(void) {
	switch (++state_ADC) {
	case 2: //step 2
		// ADC conversion from 1 (battery is done)
		// first conversion put to trash
		// start new with same configuration;
		sleep_with_ADC=1;
		break;
	case 3: //step 3
		update_ring(BAT_RING_TYPE,ADC_Get_Bat_Voltage((uint16_t)ADCL | ((uint16_t)ADCH << 8)));

	    // activate voltage divider
	    ADC_ACT_TEMP_P |= (1<<ADC_ACT_TEMP);
		ADMUX = ADC_TEMP_MUX | (1<<REFS0);
		sleep_with_ADC=1;
		break;
	case 4: //step 4
		update_ring(TEMP_RING_TYPE,ADC_Convert_To_Degree((uint16_t)ADCL | ((uint16_t)ADCH << 8)));
		shift_ring();
	default:
		// deactivate voltage divider
    	ADC_ACT_TEMP_P &= ~(1<<ADC_ACT_TEMP);
	    // set ADC control and status register / disable ADC
	    ADCSRA = (0<<ADEN)|(1<<ADPS2)|(1<<ADPS0)|(1<<ADIE); 
		break;
	}
	return sleep_with_ADC;
}



/*!
 *******************************************************************************
 * ADC Interupt
 *
 * \note not implemented at this time, to be more accurate use sleepmode and irq
 *
 ******************************************************************************/

ISR (ADC_vect){
	task|=TASK_ADC;
	sleep_with_ADC=0;
}

