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
 * \file       controller.h
 * \brief      Controller for temperature
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

#pragma once

extern uint8_t CTL_temp_wanted; //!< wanted temperature
extern uint8_t CTL_temp_wanted_last;   // desired temperatur value used for last PID control
extern uint8_t CTL_temp_auto;
extern uint8_t CTL_mode_auto;
extern int8_t PID_force_update;      // signed value, val<0 means disable force updates
extern uint8_t CTL_error;
extern uint8_t CTL_mode_window;
#define mode_window() (CTL_mode_window!=0)

#define CTL_update_temp_auto() (CTL_temp_auto=0)
#define CTL_test_auto() (CTL_mode_auto && (CTL_temp_auto==CTL_temp_wanted))
#define CTL_set_temp(t) (PID_force_update = 10, CTL_temp_wanted=t)

uint8_t CTL_update(bool minute_ch, uint8_t valve);
void CTL_temp_change_inc (int8_t ch);

#define CTL_CHANGE_MODE        -1
#define CTL_CHANGE_MODE_REWOKE -2
void CTL_change_mode(int8_t dif);

#define CTL_OPEN_WINDOW_TIMEOUT (30*60) //30 minutes

// ERRORS
#define CTL_ERR_BATT_LOW                (1<<7)
#define CTL_ERR_BATT_WARNING            (1<<6)
#define CTL_ERR_NA_5                    (1<<5)
#define CTL_ERR_RFM_SYNC                (1<<4)
#define CTL_ERR_MOTOR                   (1<<3)
#define CTL_ERR_MONTAGE                 (1<<2)
#define CTL_ERR_NA_1                    (1<<1)
#define CTL_ERR_NA_0                    (1<<0)

extern int16_t sumError;

