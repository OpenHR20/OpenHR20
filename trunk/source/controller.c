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
 * $Rev: 39 $
 */

#include <stdint.h>
#include <stdlib.h>
#include <avr/pgmspace.h>

#include "main.h"
#include "com.h"
#include "rtc.h"
#include "pid.h"
#include "adc.h"
#include "eeprom.h"
#include "controller.h"

// global Vars for default values: temperatures and speed
uint8_t CTL_temp_wanted=0;   // actual desired temperatur
uint8_t CTL_temp_wanted_last=0xff;   // desired temperatur value used for last PID control
uint8_t CTL_temp_auto=0;   // actual desired temperatur by timer
bool CTL_mode_auto=true;   // actual desired temperatur by timer
uint8_t CTL_mode_window = 0; // open window
uint16_t CTL_open_window_timeout;

static uint16_t PID_update_timeout=16;   // timer to next PID controler action/first is 16 sec after statup 
int8_t PID_force_update=16;      // signed value, val<0 means disable force updates \todo rename

uint8_t CTL_error=0;

static volatile int8_t CTL_last_dir=0;

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
    
    /* window open detection 
     * use difference of current temperature and 1 minute old tempereature
     * for status change condition must be true twice / noise protection
     */ 
    if (CTL_mode_window<2) {
        /* window open detection */
        if ((-temp_difference/10) > config.window_thld) {
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
            if (( temp_difference/10) > config.window_thld) {
                CTL_mode_window++;
                if (CTL_mode_window >= 4) {
                    PID_force_update = 0;
                    CTL_mode_window = 0;    
                }
            } else {
                CTL_mode_window = 2;
            }
        }
    }  

    if (PID_update_timeout>0) PID_update_timeout--;
    if (PID_force_update>0) PID_force_update--;
    if ((PID_update_timeout == 0)||(PID_force_update==0)) {
        uint8_t temp;
        if ((CTL_temp_wanted<TEMP_MIN) || CTL_mode_window) {
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
                uint8_t new_valve;
                uint8_t old_valve = valve;
                new_valve = pid_Controller(calc_temp(temp),temp_average+
                    ((uint16_t)valve*config.human_temperature_feeling/100));
                if ((new_valve==0) || (new_valve==100)) {
                    valve = new_valve;
                } else {
                    int8_t x = CTL_last_dir*(int8_t)(valve-new_valve);
                    if ((x>20) || (x<=0))  {
                        valve = new_valve;
                    }
                }
                if (new_valve > old_valve) CTL_last_dir =  1; 
                if (new_valve < old_valve) CTL_last_dir = -1; 
            }
        } 
        PID_force_update = -1;
        COM_print_debug(valve);
    }
    // batt error detection
    if (bat_average < 20*(uint16_t)config.bat_warning_thld) {
        CTL_error |=  CTL_ERR_BATT_WARNING;
    } else {
        CTL_error &= ~CTL_ERR_BATT_WARNING;
    }
    if (bat_average < 20*(uint16_t)config.bat_low_thld) {
        CTL_error |=  CTL_ERR_BATT_LOW;
    } else {
        CTL_error &= ~CTL_ERR_BATT_LOW;
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
	CTL_mode_window = false;
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
    CTL_mode_window = false;
    PID_force_update = 10; 
}
