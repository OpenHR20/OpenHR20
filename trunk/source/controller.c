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

// global Vars for default values: temperatures and speed
uint8_t temp_wanted=0;   // actual desired temperatur
uint8_t temp_wanted_last=0xff;   // desired temperatur value used for last PID control
uint8_t temp_auto=0;   // actual desired temperatur by timer

static uint16_t PID_update_timeout=16;   // timer to next PID controler action/first is 16 sec after statup 
int8_t PID_force_update=16;      // signed value, val<0 means disable force updates


/*!
 *******************************************************************************
 *  Controller update
 *
 *  \param minute_ch is true when minute is changed
 *  \returns valve position
 ******************************************************************************/
uint8_t CTL_update(bool minute_ch, uint8_t valve) {
    if ( minute_ch || (temp_auto==0) ) {
        // minutes changed or we need return to timers
        uint8_t t=RTC_ActualTimerTemperature(!(temp_auto==0));
        if (t!=0) {
            temp_auto=t;
            if (mode_auto) {
                temp_wanted=temp_auto; 
                if ((PID_force_update<0)&&(temp_wanted!=temp_wanted_last)) {
                    PID_force_update=0;
                }
            }
        }
    }
    if (PID_update_timeout>0) PID_update_timeout--;
    if (PID_force_update>0) PID_force_update--;
    if ((PID_update_timeout == 0)||(PID_force_update==0)) {
        //update now
        if (temp_wanted<TEMP_MIN) temp_wanted=TEMP_MIN;
        if (temp_wanted>TEMP_MAX) temp_wanted=TEMP_MAX;
        if (temp_wanted!=temp_wanted_last) {
            temp_wanted_last=temp_wanted;
            pid_Init(temp_average); // restart PID controler
            PID_update_timeout = 0; //update now
        }
        if (PID_update_timeout == 0) {
            PID_update_timeout = (config.PID_interval * 5); // new PID pooling
            if (temp_wanted<TEMP_MIN) {
                valve = pid_Controller(500,temp_average); // frost protection to 5C
            } else if (temp_wanted>TEMP_MAX) {
                valve = 100;
            } else {
                valve = pid_Controller(calc_temp(temp_wanted),temp_average);
            }
        } 
        PID_force_update = -1;
        COM_print_debug(0);
    }
    return valve;
}
