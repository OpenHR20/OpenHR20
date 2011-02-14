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
#include "keyboard.h"

// global Vars for default values: temperatures and speed
uint8_t CTL_temp_wanted=0;   // actual desired temperature
uint8_t CTL_temp_wanted_last=0xff;   // desired temperature value used for last PID control
uint8_t CTL_temp_auto=0;   // actual desired temperature by timer
bool CTL_mode_auto=true;   // actual desired temperature by timer
uint8_t CTL_mode_window = 0; // open window (0=closed, >0 open-timmer)
#if (HW_WINDOW_DETECTION)
	static uint8_t window_timer=AVERAGE_LEN+1;
#else
	uint16_t CTL_open_window_timeout;
#endif
static uint16_t PID_update_timeout=AVERAGE_LEN+1;   // timer to next PID controler action/first is 16 sec after statup
int8_t PID_force_update=AVERAGE_LEN+1;      // signed value, val<0 means disable force updates \todo rename

static uint8_t pid_Controller(int16_t setPoint, int16_t processValue, int8_t old_result);

uint8_t CTL_error=0;

#if (HW_WINDOW_DETECTION)
static void CTL_window_detection(void) {
	bool w = ((PINE & (1<<PE2))!=0) && config.window_open_detection_enable;

	if (CTL_mode_window != w) {
		if (window_timer==0) {
			CTL_mode_window = w;
			PID_force_update = 0;
			// kb_events |= KB_EVENT_UPDATE_LCD;
		} else {
			window_timer--;
			return;
		}
	}
	window_timer=(w)?(config.window_close_detection_delay):(config.window_open_detection_delay);
}
#else
static void CTL_window_detection(void) {
    uint8_t i = (ring_buf_temp_avgs_pos+AVGS_BUFFER_LEN
        -((CTL_mode_window!=0)?config.window_close_detection_time:config.window_open_detection_time)
        )%AVGS_BUFFER_LEN;
    int16_t min = 10000;
    int16_t max = 0;
    while (1) {
        int16_t x = ring_buf_temp_avgs[i];
        if (x!=0) { // startup condition
            if (x<min) min = x;
            if (x>max) max = x;
        }
        if (i==ring_buf_temp_avgs_pos) break;
        i=(i+1)%AVGS_BUFFER_LEN;
    }
    if ((temp_average-min) > (int16_t) config.window_close_detection_diff) {
        if (CTL_mode_window!=0) {
            CTL_mode_window=0;
            PID_force_update = 0;
			//kb_events |= KB_EVENT_UPDATE_LCD;
        }
    } else {
        if ((CTL_mode_window==0) && ((max-temp_average) > (int16_t) config.window_open_detection_diff)) {
            CTL_mode_window=config.window_open_timeout;
            PID_force_update = 0;
			//kb_events |= KB_EVENT_UPDATE_LCD;
        }
    }
}
#endif

/*!
 *******************************************************************************
 *  Controller update
 *  \note call it once per second
 *  \param minute_ch is true when minute is changed
 *  \returns valve position
 *
 ******************************************************************************/
uint8_t CTL_update(bool minute_ch, uint8_t valve) {
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

    CTL_window_detection();
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
            sumError=0;
            goto UPDATE_NOW; // optimization
        }
        if (PID_update_timeout == 0) {
            UPDATE_NOW:
            PID_update_timeout = (config.PID_interval * 5); // new PID pooling
            if (temp>TEMP_MAX) {
                valve = config.valve_max;
            } else {
                valve = pid_Controller(calc_temp(temp),temp_average,valve);
            }
        }
        COM_print_debug(valve);
        PID_force_update = -1; // invalid value = not used
    }
    // batt error detection
    if (bat_average) {
		if (bat_average < 20*(uint16_t)config.bat_low_thld) {
    	    CTL_error |=  CTL_ERR_BATT_LOW | CTL_ERR_BATT_WARNING;
	    } else {
	        if (bat_average < 20*(uint16_t)config.bat_warning_thld) {
	            CTL_error |=  CTL_ERR_BATT_WARNING;
				#if (BATT_ERROR_REVERSIBLE)
	            	CTL_error &= ~CTL_ERR_BATT_LOW;
				#endif
	        } else {
				#if (BATT_ERROR_REVERSIBLE)
	            	CTL_error &= ~(CTL_ERR_BATT_WARNING|CTL_ERR_BATT_LOW);
				#endif
	        }
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

static uint8_t menu_temp_rewoke;
/*!
 *******************************************************************************
 *  Change controller mode
 *
 ******************************************************************************/
void CTL_change_mode(int8_t m) {
    if (m == CTL_CHANGE_MODE) {
        // change
  		menu_temp_rewoke=CTL_temp_auto;
        CTL_mode_auto=!CTL_mode_auto;
        PID_force_update = 10;
    } else if (m == CTL_CHANGE_MODE_REWOKE) {
        //rewoke
  		CTL_temp_auto=menu_temp_rewoke;
        CTL_mode_auto=!CTL_mode_auto;
        PID_force_update = 10;
    } else {
        if (m >= 0) CTL_mode_auto=m;
        PID_force_update = 0;
    }
    if (CTL_mode_auto && (m != CTL_CHANGE_MODE_REWOKE)) {
    	CTL_temp_wanted=(CTL_temp_auto=RTC_ActualTimerTemperature(false));
    	// CTL_temp_auto=0;  //refresh wanted temperature in next step
    }
    CTL_mode_window = 0;
}

//! Last process value, used to find derivative of process value.
static int16_t lastProcessValue=0;

//! Summation of errors, used for integrate calculations
int16_t sumError=0;
//! The scalling_factor for PID constants
#define scalling_factor  (256)
#if (scalling_factor != 256)
    #error optimized only for (scalling_factor == 256)
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
  int32_t /*error2,*/ pi_term;
  int16_t error16, maxSumError;
  error16 = setPoint - processValue;

  // maximum error is 20 degree C
  if (error16 > 2000) {
    error16=2000;
  } else if (error16 < -2000) {
    error16=-2000;
  }

  // Calculate Iterm and limit integral runaway
  {
    int16_t d = lastProcessValue - processValue;
    if  ((lastProcessValue!=0)
	&& (
		((d==0) && (abs(error16)>config.temp_tolerance))
	     || ((d>0)  && (error16>0))
	     || ((d<0)  && (error16<0))
	   )) {
        // update sumError if turn is wrong only
        int16_t max = (int16_t)scalling_factor*50/config.P_Factor;
        if (error16 > max) {  // maximum sumError change + limiter
          sumError += max;
        } else if (error16 < -max) { // maximum sumError change - limiter
          sumError -= max;
        } else {
          sumError += error16;
        }
    }
    lastProcessValue = processValue;
  }
  if (config.I_Factor == 0) {
      maxSumError = 12800; // scalling_factor*50/1
  } else {
      // for overload protection: maximum is scalling_factor*50/1 = 12800
      maxSumError = ((int16_t)scalling_factor*50)/config.I_Factor;
  }
  if(sumError > maxSumError){
    sumError = maxSumError;
  } else if(sumError < -maxSumError){
    sumError = -maxSumError;
  }

  pi_term = ((int32_t)config.PP_Factor * (int32_t)abs(error16));
  pi_term += (int32_t)((uint16_t)config.P_Factor <<8);
  pi_term *= (int32_t)error16;
  pi_term >>= 8;
  // pi_term - > for overload limit: maximum is +-(((255*2000)+255)*2000) = +-1020510000/256=3986367

  pi_term += (int16_t)(config.I_Factor) * (int16_t)sumError; // maximum is (scalling_factor*50/I_Factor)*I_Factor
  // pi_term - > for overload limit: maximum is +- (3986367 + scalling_factor*50) = +-3999167

  pi_term += (int16_t)(config.valve_center)*scalling_factor;
  if(pi_term > 100*scalling_factor){
    return config.valve_max;
  } else if(pi_term < 0){
    return config.valve_min;
  }
  // now we can use 16bit value
  {
    int16_t pi_term16 = pi_term;

    // ignore changes < 1%
    if (abs(pi_term16-((int16_t)(old_result)*scalling_factor))<scalling_factor) return old_result;

    pi_term16 >>=8; //= scalling_factor;

    if(pi_term16 > config.valve_max){
      return config.valve_max;
    } else if(pi_term16 < config.valve_min){
      return config.valve_min;
    }
    return((uint8_t)pi_term16);
  }
}

