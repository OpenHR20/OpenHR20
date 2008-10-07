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
 * \file       task.h
 * \brief      task definition for main loop
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       02.10.2008
 * $Rev: 70 $
 */

// #include <stdint.h>

//extern volatile unsigned char task;
#define  task GPIOR0

#define TASK_KB			      1
#define TASK_RTC		      2
#define TASK_ADC		      4
#define TASK_LCD		      8
#define TASK_UPDATE_MOTOR_POS 16
#define TASK_MOTOR_TIMER 	  32
//#define TASK_			64
//#define TASK_			128




