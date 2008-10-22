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
 * $Rev: 39 $
 */

extern uint8_t temp_wanted; //!< wanted temperature
extern uint8_t temp_wanted_last;   // desired temperatur value used for last PID control
extern uint8_t temp_auto;

extern int8_t PID_force_update;      // signed value, val<0 means disable force updates

#define CTL_need_update() (PID_force_update = 10)

#define CTL_changed_temp() (temp_wanted!=temp_auto)

#define CTL_need_auto() (temp_auto=0)

#define CTL_temp_wanted temp_wanted_last

uint8_t CTL_update(bool minute_ch, uint8_t valve);
