/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  ompiler:    WinAVR-20071221
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
 * \date       24.12.2007
 * $Rev$
 */

#ifndef MAIN_H
#define MAIN_H

#include "config.h"	// General Setup configuration

//! HR20 runs on 4MHz
#ifndef F_CPU
#define F_CPU 4000000UL
#endif

// Bitmask for PortB where Keys are connected
#define  KEYMASK_AUTO   (1<<PB3)   //!< bitmask for: AUTO Button
#define  KEYMASK_PROG   (1<<PB2)   //!< bitmask for: PROG Button
#define  KEYMASK_C      (1<<PB1)   //!< bitmask for:   �C Button
#define  KEYMASK_MOUNT  (1<<PB0)   //!< bitmask for: mounted contact

//! temperature slots: 2 different Temperatures can switched by DOW-Timer
#define TEMP_SLOTS 2       //!< high, low

// Boot Timeslots -> move to CONFIG.H
// 10 Minutes after BOOT_hh:00
#define BOOT_ON1      BOOT_hh*10 + 1 //!< Boot Timeslot 0 \todo move to CONFIG.H
#define BOOT_OFF1     BOOT_ON1 + 1   //!< Boot Timeslot 1 \todo move to CONFIG.H
#define BOOT_ON2      BOOT_ON1 + 2   //!< Boot Timeslot 2 \todo move to CONFIG.H
#define BOOT_OFF2     BOOT_ON1 + 3   //!< Boot Timeslot 3 \todo move to CONFIG.H


#define BOOT_TEMP_H   22 //! Boot Temperature high (22�C) -> move to CONFIG.H
#define BOOT_TEMP_L   16 //! Boot Temperature low (16�C) -> move to CONFIG.H

#define nop() asm("nop;")

// global vars
extern uint16_t serialNumber;	//!< Unique serial number \todo move to CONFIG.H

extern int16_t temp_wanted; //!< wanted temperature

// public prototypes
void delay(uint16_t);                   // delay

// typedefs
typedef enum { false, true } bool;

// functional defines
#define DEG_2_UINT8(deg)    (deg*10)-49 ;   //!< convert degree to uint8_t value

#endif /* MAIN_H */
