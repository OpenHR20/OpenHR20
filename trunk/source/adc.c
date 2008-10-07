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
 * \file       adc.c
 * \brief      functions to control Analog / Digtial - Converter
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>
 * \date       15.03.2008
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

// typedefs

// vars
volatile uint16_t ADC_Val_Temp;      //!< A/D Value temperature 
volatile uint16_t ADC_Val_Ub;        //!< A/D Value batterie voltage

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
 *  Init ADC
 *
 *  \note
 *  - disable ADC interrupt ADIE
 *  - auto trigger disable (ADATE=0)
 *  - ADC Prescaler = 32 (ADPS2, ADPS0)
 *  - input clock to the ADC =125kHz (4MhZ/32)
 *  - one conversion takes 104 to 200us  (1st 25, normal 13 Cycles @ 125kHz)
 *  - ADSC handled during conversion
 *  - reference AVCC with external capacitor at AREF pin (REFS1=0, REFS0=1)
 *  - no left adjust result (ADLAR=0)
 *  - free running mode
 *  - digital input on PF0-7 is disabled in main.c:init()
 *  \todo
 *  - ADC using interrupt amd sleep mode (ADC Noise Canceler)
 ******************************************************************************/
void ADC_Init(void)
{
    // set reference
    ADMUX = (1<<REFS0);

    // set ADC control and status register
    ADCSRA = (1<<ADPS2) | (1<<ADPS0);         // prescaler=32
    
    // free running mode, (not needed, because auto trigger disabled)
    ADCSRB = 0;
}

/*!
 *******************************************************************************
 *  Measure battery voltage
 *
 *  \note
 *  - sample battery voltage
 *  - store result in ADC_Val_Ub
 *  - enable ADC before and disable after conversion
 ******************************************************************************/
void ADC_Measure_Ub(void)
{
    ADC_Val_Ub = ADC_Sample_Channel(ADC_UB_MUX);
}

/*!
 *******************************************************************************
 *  Measure temperature
 *
 *  \note
 *  - activate voltage divider on PF3
 *  - sample temperature Channel: ADC2 (PF2)
 *  - store result in ADC_Val_Temp
 *  - enable ADC before and disable after conversion
 *  - deactivate voltage divider
 ******************************************************************************/
void ADC_Measure_Temp(void)                // Sample Temperature Value
{
    // activate voltage divider
    ADC_ACT_TEMP_P |= (1<<ADC_ACT_TEMP);
    
    // sample temperature sensor
    ADC_Val_Temp = ADC_Sample_Channel(ADC_TEMP_MUX);    

    // deactivate voltage divider
    ADC_ACT_TEMP_P &= ~(1<<ADC_ACT_TEMP);
}

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
int16_t ADC_Get_Temp_Degree(void)            // Get Temperature in 1/100 Deg °C
{
    int32_t degree;

    degree = ADC_Convert_To_Degree(ADC_Val_Temp);    
     
    if (degree < INT16_MIN){
        return (INT16_MIN);
    }else if (degree > INT16_MAX){ 
        return (INT16_MAX);
    }else{ 
        return ((int16_t) degree);        
    }
}
 
/*!
 *******************************************************************************
 *  Get temperature sample value
 *
 *  \returns ADC value from temperature measurement (0-1023)
 *
 *  \note
 *  - measurment has been performed before using \ref ADC_Measure_Temp 
 ******************************************************************************/
uint16_t ADC_Get_Temp_Val(void)            // Get Temperature ADC Value 0-1023
{
    return (ADC_Val_Temp);
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
uint16_t ADC_Get_Bat_Voltage(void)             // Get Batteriy Voltage in mV
{
    uint32_t millivolt;
    millivolt = 1126400;
    millivolt /= ADC_Val_Ub; 
    return ((uint16_t) millivolt);
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
bool ADC_Get_Bat_isOk(void)
{
	return (ADC_Get_Bat_Voltage() > ADC_LOW_BATT_LEVEL);
}


/*!
 *******************************************************************************
 *  Get battery sample value
 *
 *  \returns ADC value from battery voltage measurement (0-1023)
 *
 *  \note
 *  - measurment has been performed before using \ref ADC_Measure_Ub 
 ******************************************************************************/
uint16_t ADC_Get_Bat_Val(void)             // Get Batteriy Voltage in mV
{
    return (ADC_Val_Ub);
}


/*!
 *******************************************************************************
 *  convert ACD value to temperatur 
 *
 *  \returns temperatur in 1/100 degrees Celsius
 *
 *  \param  adc value from ADC
 *
 *  \todo: store values for conversion in EEPROM 
 ******************************************************************************/
int32_t ADC_Convert_To_Degree(uint16_t adc)
{
    int32_t dummy;
    uint8_t i;

    for (i=1; i<kz; i++){
        if (adc<kx[i]){
            break;
        }        
    }

    dummy =  ((int32_t) ky[i] - (int32_t) ky[i-1]);
	dummy *= ((int32_t) adc   - (int32_t) kx[i-1]);
    dummy /= ((int32_t) kx[i] - (int32_t) kx[i-1]);
    dummy += (int32_t)  ky[i-1];

    return dummy;
}


/*!
 *******************************************************************************
 * Sample Channel
 * \returns value from second conversion *
 * \note
 * - set MUX
 * - start dummy conversion
 * - get value from second conversion 
 ******************************************************************************/
uint16_t ADC_Sample_Channel(uint8_t mux)
{
    uint8_t i;
    uint16_t value;

    // enable ADC
    ADCSRA |= (1<<ADEN);

    // set mux
    ADMUX = mux;

    // set reference to AVCC 
    ADMUX |= (1<<REFS0);

    // 2 conversions (first to warmup ADC)
    for (i=0; i<2; i++){
        // start conversions
        ADCSRA |= (1<<ADSC);
        // wait for conversion to be finished
        while ( ADCSRA & (1<<ADSC) ) {
        }
        value = ADCL;
        value += ((uint16_t) ADCH) * 256;
    }

    // disable ADC
    ADCSRA &= ~(1<<ADEN);
    
    // return last conversion result
    return (value);
}


/*!
 *******************************************************************************
 * ADC Interupt
 *
 * \note not implemented at this time, to be more accurate use sleepmode and irq
 *
 ******************************************************************************/
/*
ISR (ADC_vect){
    // conversion finished, store result

}
*/
