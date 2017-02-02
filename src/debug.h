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
#include "config.h"

#define DEBUG_MODE 0
#define DEBUG_SKIP_DATETIME_SETTING_AFTER_RESET 0


#define DEBUG_BEFORE_SLEEP()
#define DEBUG_AFTER_SLEEP()

#define KEEP_ALIVE_FOR_COMMUNICATION DEBUG_MODE

#define DEBUG_PRINT_RTC_TICKS 0
#define DEBUG_PRINT_MOTOR 0
#define DEBUG_PRINT_MEASURE 0
#define DEBUG_PRINT_I_SUM 1
#define DEBUG_PRINT_ADDITIONAL_TIMESTAMPS DEBUG_MODE
#define DEBUG_IGNORE_MONT_CONTACT 0
#define DEBUG_MOTOR_COUNTER  1

#define DEBUG_BATT_ADC 0

#define DEBUG_PRINT_STR_16 (DEBUG_BATT_ADC)

#if RFM
#define DEBUG_DUMP_RFM DEBUG_MODE
#else
#define DEBUG_DUMP_RFM 0
#endif
