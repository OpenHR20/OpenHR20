/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  compiler:   WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Jiri Dobry (jdobry-at-centrum-dot-cz)
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
 * \file       controller.c
 * \brief      Controller for temperature
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

#include <stdint.h>
#include <stdlib.h>
#include <avr/pgmspace.h>

#include "config.h" 
#include "main.h"
#include "com.h"
#include "../common/rtc.h"
#include "adc.h"
#include "eeprom.h"
#include "controller.h"

// global Vars for default values: temperatures and speed
uint8_t CTL_temp_wanted=0;   // actual desired temperature
uint8_t CTL_temp_wanted_last=0xff;   // desired temperature value used for last PID control
uint8_t CTL_temp_auto=0;   // actual desired temperature by timer
bool CTL_mode_auto=true;   // actual desired temperature by timer
uint8_t CTL_mode_window = 0; // open window
uint16_t CTL_open_window_timeout;

static uint16_t PID_update_timeout=16;   // timer to next PID controler action/first is 16 sec after statup 
int8_t PID_force_update=16;      // signed value, val<0 means disable force updates \todo rename

static uint8_t pid_Controller(int16_t setPoint, int16_t processValue, int8_t old_result);

uint8_t CTL_error=0;

/*!
 *******************************************************************************
 *  Controller update
 *  \note call it once per second
 *  \param minute_ch is true when minute is changed
 *  \returns valve position
 *   
 ******************************************************************************/
int8_t CTL_update(bool minute_ch, int8_t valve) {
    if ( minute_ch || (CTL_temp_auto==0) ) {
        // minutes changed or we need return to timers
        uint8_t t=RTC_ActualTimerTemperature(!(CTL_temp_auto==0));
        if (t!=0) {
            CTL_temp_auto=t;
            if (CTL_mode_auto) {
                CTL_temp_wanted=CTL_temp_auto; 
                if ((PID_force_update<0)&&(CTL_temp_wanted!=CTL_temp_wanted_last)) {
                    PID_force_update=0;
                }
            }
        }
    }
    
    /* window open detection 
     * use difference of current temperature and 1 minute old tempereature
     * for status change condition must be true twice / noise protection
     */ 
    if (CTL_mode_window<config.window_open_noise_filter) {
        /* window open detection */
        if (-temp_difference > config.window_open_thld) {
            CTL_mode_window++;
            if (mode_window()) { 
                PID_force_update=0;
                CTL_open_window_timeout = CTL_OPEN_WINDOW_TIMEOUT;
            }
        } else {
            CTL_mode_window = 0;
        }        
    } else { 
        /* window close detection */
        if (CTL_open_window_timeout > 0) {
            CTL_open_window_timeout--; 
            if ( temp_difference >= 0 ) {
                if (temp_difference>0) {
                    CTL_mode_window++;
                    if (CTL_mode_window >= (config.window_open_noise_filter+config.window_close_noise_filter)) {
                        PID_force_update = 0;
                        CTL_mode_window = 0;    
                    }
                }
            } else {
                CTL_mode_window = config.window_open_noise_filter;
            }
        }
    }  

    if (PID_update_timeout>0) PID_update_timeout--;
    if (PID_force_update>0) PID_force_update--;
    if ((PID_update_timeout == 0)||(PID_force_update==0)) {
        uint8_t temp;
        if ((CTL_temp_wanted<TEMP_MIN) || mode_window()) {
            temp = TEMP_MIN;  // frost protection to TEMP_MIN
        } else {
            temp = CTL_temp_wanted;
        }
        //update now
        if (temp!=CTL_temp_wanted_last) {
            CTL_temp_wanted_last=temp;
            pid_Init(temp_average); // restart PID controler
            goto UPDATE_NOW; // optimization
        }
        if (PID_update_timeout == 0) {
            UPDATE_NOW:
            PID_update_timeout = (config.PID_interval * 5); // new PID pooling
            if (temp>TEMP_MAX) {
                valve = 100;
            } else {
                valve = (int16_t)pid_Controller(calc_temp(temp),temp_average,valve-50);
            }
        } 
        PID_force_update = -1;
        COM_print_debug(valve);
    }
    // batt error detection
    if (bat_average < 20*(uint16_t)config.bat_low_thld) {
        CTL_error |=  CTL_ERR_BATT_LOW | CTL_ERR_BATT_WARNING;
    } else {
        if (bat_average < 20*(uint16_t)config.bat_warning_thld) {
            CTL_error |=  CTL_ERR_BATT_WARNING;
            CTL_error &= ~CTL_ERR_BATT_LOW;
        } else {
            CTL_error &= ~(CTL_ERR_BATT_WARNING|CTL_ERR_BATT_LOW);
        }
    }
    
    return valve;
}

/*!
 *******************************************************************************
 *  Change controller temperature (+-)
 *
 *  \param ch relative change
 ******************************************************************************/
void CTL_temp_change_inc (int8_t ch) {
    CTL_temp_wanted+=ch;
	if (CTL_temp_wanted<TEMP_MIN-1) {
		CTL_temp_wanted= TEMP_MIN-1;
	} else if (CTL_temp_wanted>TEMP_MAX+1) {
		CTL_temp_wanted= TEMP_MAX+1; 
	}    
	CTL_mode_window = 0;
    PID_force_update = 10; 
}

static uint8_t menu_temp_rewoke1;
static uint8_t menu_temp_rewoke2;
/*!
 *******************************************************************************
 *  Change controller mode
 *
 ******************************************************************************/
void CTL_change_mode(int8_t m) {
    if (m == CTL_CHANGE_MODE) {
        // change
    	if (!CTL_mode_auto || (CTL_temp_wanted!=CTL_temp_auto)) {
    		menu_temp_rewoke1=CTL_temp_wanted;
    		menu_temp_rewoke2=CTL_temp_auto; // \todo 
    	} else {
    		menu_temp_rewoke1=0;
    		menu_temp_rewoke2=0;
    	}
        CTL_mode_auto=!CTL_mode_auto;
        if (CTL_mode_auto) CTL_temp_wanted=CTL_temp_auto;
    } else if (m == CTL_CHANGE_MODE_REWOKE) {
        //rewoke
    	if (CTL_mode_auto || (menu_temp_rewoke2==CTL_temp_auto)) {
    		CTL_temp_wanted=menu_temp_rewoke1;
    	}
        CTL_mode_auto=!CTL_mode_auto;
    } else {
        CTL_mode_auto=m;
    }
    CTL_mode_window = 0;
    PID_force_update = 10; 
}

//! Last process value, used to find derivative of process value.
static int16_t lastProcessValue;

//! Summation of errors, used for integrate calculations
int16_t sumError=0;
//! The scalling_factor for PID constants
#define scalling_factor  (256)

#if CONFIG_ENABLE_D
void pid_Init( int16_t processValue)
// Set up PID controller parameters
{
  lastProcessValue =  processValue;
}
#endif


/*! \brief non-linear  PID control algorithm.
 *
 *  Calculates output from setpoint, process value and PID status.
 *
 *  \param setPoint  Desired value.
 *  \param processValue  Measured value.
 */
static uint8_t pid_Controller(int16_t setPoint, int16_t processValue, int8_t old_result)
{
  int32_t error32, pi_term;
  int16_t error16, maxSumError;
#if CONFIG_ENABLE_D
  int32_t d_term,
#endif
  error16 = setPoint - processValue;
  
  // maximum error is 40 degree C
  if (error16 > 4000) { 
    error16=4000;
  } else if (error16 < -4000) {
    error16=-4000;
  }

  // Calculate Iterm and limit integral runaway  
  {
    int16_t d = lastProcessValue - processValue;
    if (((d>0)&&(error16>0)) || ((d<0)&&(error16<0))) {
        // update sumError if turn is wrong only 
        int16_t max = (int16_t)scalling_factor*50/config.P_Factor;
        if (error16 > max) {  // maximum sumError change + limmiter
          sumError += max;  
        } else if (error16 < -max) { // maximum sumError change - limmiter
          sumError -= max;  
        } else {
          sumError += error16;
        }
    }
  }
  if (config.I_Factor == 0) {
      maxSumError = 12750; // 255*50/1
  } else {
      // for overload protection: maximum is 255*50/1 = 12750
      maxSumError = ((int16_t)scalling_factor*50)/config.I_Factor;
  }
  if(sumError > maxSumError){
    sumError = maxSumError;
  } else if(sumError < -maxSumError){
    sumError = -maxSumError;
  }

  error32 = (int32_t)error16 * (int32_t)abs(error16); // non linear P characteristic 
  error32 /= 100L; // P gain
  // error32 -> for overload limit: maximum is +-(4000*4000/200) = +-160000

  // Calculate Pterm
  pi_term = (config.P_Factor * error32);
  // pi_term - > for overload limit: maximum is +-(40800000+3251250) = +-44051250
  
  pi_term += config.I_Factor * sumError;
  // pi_term - > for overload limit: maximum is +- 255*12750 = 3251250
   

#if CONFIG_ENABLE_D
  // Calculate Dterm
  d_term = lastProcessValue - processValue;
  d_term *= labs(d_term); // non linear characteristic 
  d_term /= scalling_factor;
  d_term *= config.D_Factor;
  pi_term += d_term;
#endif
  lastProcessValue = processValue;

  if(pi_term > 50*256){
    return config.valve_max;
  } else if(pi_term < -50*256){
    return config.valve_min; 
  }
  // now we can use 16bit value
  {
    int16_t pi_term16 = pi_term;
    
    if (abs(pi_term16-((int16_t)old_result*scalling_factor))<config.pid_hysteresis) return old_result;
    
    pi_term16 /= scalling_factor;
    pi_term16 += 50;
  
    if(pi_term16 > config.valve_max){
      return config.valve_max;
    } else if(pi_term16 < config.valve_min){
      return config.valve_min; 
    }
    return((uint8_t)pi_term16);
  }

}

