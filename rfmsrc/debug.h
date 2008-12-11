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
 * \file       debug.h
 * \brief      debug options for project
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

#pragma once

#define DEBUG_MODE 1
#define DEBUG_SKIP_DATETIME_SETTING_AFTER_RESET 0


#if (DEBUG_MODE == 1)
    #define DEBUG_BEFORE_SLEEP() //(PORTE &= ~(1<<PE2))
    #define DEBUG_AFTER_SLEEP()  //(PORTE |= (1<<PE2))
#else
    #define DEBUG_BEFORE_SLEEP()
    #define DEBUG_AFTER_SLEEP()
#endif



#define KEEP_ALIVE_FOR_COMMUNICATION DEBUG_MODE

#define DEBUG_PRINT_MOTOR 1
#define DEBUG_PRINT_MEASURE 0
#define DEBUG_PRINT_I_SUM 1
#define DEBUG_IGNORE_MONT_CONTACT 0
