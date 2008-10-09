/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  compiler:   WinAVR-20071221
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
 * \file       main.h
 * \brief      header file for main.c, the main file for Open HR20 project
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>
 * \date       $Date$
 * $Rev$
 */

#ifndef MAIN_H
#define MAIN_H

#include "config.h"	// General Setup configuration

//! HR20 runs on 4MHz
#ifndef F_CPU
#define F_CPU 4000000UL
#endif

#define nop() asm("nop;")

// global vars
extern uint16_t serialNumber;	//!< Unique serial number \todo move to CONFIG.H

extern uint8_t temp_wanted; //!< wanted temperature
extern int8_t PID_force_update;      // signed value, val<0 means disable force updates

#define c2temp(c) (c*2)
#define calc_temp(t) (((uint16_t)t)*50)   // result unit is 1/100 C
#define TEMP_MIN    c2temp (5)   // 5�C
#define TEMP_MAX    c2temp (30) // 30�C

// public prototypes
void delay(uint16_t);                   // delay

// typedefs
typedef enum { false, true } bool;

// functional defines
#define DEG_2_UINT8(deg)    (deg*10)-49 ;   //!< convert degree to uint8_t value

#endif /* MAIN_H */
