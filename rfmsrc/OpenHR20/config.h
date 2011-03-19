/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  compiler:    WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Juergen Sachs (juergen-sachs-at-gmx-dot-de)
 *				2008 Jiri Dobry (jdobry-at-centrum-dot-cz)
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
 * \file       config.h
 * \brief      This headerfile only contains information about the configuration of the HR20E and its functionality
 * \author     Juergen Sachs (juergen-sachs-at-gmx-dot-de) Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

/*
In this file we define only configuration parameters, for example what kind of control port we have.
*/

#pragma once

#ifndef CONFIG_H
#define CONFIG_H

// AVR LibC includes
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/version.h>


#define LANG_uni 1
#define LANG_de 2
#define LANG_cs 3
//! language
#define LANG LANG_uni
//#define LANG LANG_de
//#define LANG LANG_cs

#define BATT_ERROR_REVERSIBLE 0

#define _STR(x) #x
#define STR(x) _STR(x)

// our Version
#define REVHIGH  1  //! Revision number high
#define REVLOW   1  //! Revision number low
#define VERSION_N (0x0000 + REVLOW + (REVHIGH<<8)) //! Version as HEX value F0.92 (E for Experimental)


#ifndef REVISION
 #define REVISION "$Rev$"
#endif

// Parameters for the COMM-Port
#define COM_BAUD_RATE 9600
#if THERMOTRONIC!=1 //No serialport implementet yet
// Note we should only enable of of the following at one time
/* we support RS232 */
#define COM_RS232 1
/* we support RS485 */
/* #define COM_RS485  */
/* Our default Adress, if not set or invalid */
/* #define COM_DEF_ADR 1 */
#endif

#define DEFAULT_TEMPERATURE 2000

#ifndef HW_WINDOW_DETECTION
	#define HW_WINDOW_DETECTION 0
#endif
#ifndef TEMP_COMPENSATE_OPTION
	#define TEMP_COMPENSATE_OPTION 0
#endif

#if THERMOTRONIC
    #ifdef RFM
        #if RFM
            #error "THERMOTRONIC with RFM12 not implemented"
        #endif
    #else
        #define RFM 0
    #endif
#else
    #ifndef RFM
        #define RFM 1 //!< define RFM to 1 if you want to have support for the RFM Radio Moodule in the Code
    #endif
#endif

#if (RFM == 1)
	#ifndef RFM_WIRE_MARIOJTAG 
		#define RFM_WIRE_MARIOJTAG 0 //!< define that if you want to wire your RFM to external JTAG pins
	#endif
    #if RFM_WIRE_MARIOJTAG
		#define RFM_WIRE_JD_INTERNAL 0 //!< define that if you want to wire your RFM to free internal pins
	#else 
		#define RFM_WIRE_JD_INTERNAL 1 //!< define that if you want to wire your RFM to free internal pins
	#endif

	#define RFM12 1 // just a synonym
	#define RFM_DEVICE_ADDRESS 0x00

	#if (RFM_WIRE_MARIOJTAG == 1)
		#define DISABLE_JTAG 1 //!< define DISABLE_JTAG if your RFM's connection uses any JTAG pins
		#if (HW_WINDOW_DETECTION)
			#error HW_WINDOW_DETECTION is not compatible with RFM_WIRE_MARIOJTAG
		#endif
	#endif
	#define SECURITY_KEY_0		0x01
	#define SECURITY_KEY_1		0x23
	#define SECURITY_KEY_2		0x45
	#define SECURITY_KEY_3		0x67
	#define SECURITY_KEY_4		0x89
	#define SECURITY_KEY_5     	0xab
	#define SECURITY_KEY_6		0xcd
	#define SECURITY_KEY_7		0xef
#else
	#define RFM12                  0
	#define RFM_WIRE_MARIOJTAG     0
	#define RFM_WIRE_JD_INTERNAL   0
	#define DISABLE_JTAG           0
#endif


// Some default Values
#define BOOT_DD         1  //!< Boot-Up date: day
#define BOOT_MM         1  //!< Boot-Up date: month
#define BOOT_YY        10  //!< Boot-Up date: year
#define BOOT_hh        12  //!< Boot-Up time: hour
#define BOOT_mm        00  //!< Boot-Up time: minutes

#define MENU_SHOW_BATTERY 1
#define MOTOR_COMPENSATE_BATTERY 0

/**********************/
/* code configuration */
/**********************/

// enable D part of PID controller
#define CONFIG_ENABLE_D 0


/* compiler compatibility */
#ifndef ISR_NAKED
#   define ISR_NAKED      __attribute__((naked))
#endif


/* revision remarks
 **********************************************
 */


// some handy macros

#define	ELEMCNT(A)							(sizeof(A) / ELEMSIZE(A))
#define	OFFSETOF(TYPE, ELEM)				((size_t)((char *)&((TYPE *) 0)->ELEM - (char*)((TYPE*)NULL)))
#define CMP(X,Y)							(((X) == (Y)) ? 0 : (((X) < (Y)) ? -1 : +1))
#define LITERAL_STRLEN(S)					(sizeof(S) - sizeof('\0'))
#define ARRAY(TYPE, PTR, INDEX)				((TYPE*)(PTR))[INDEX]

#define IN_RANGE(MIN, TEST, MAX)			(((MIN) <= (TEST)) && ((TEST) <= (MAX)))
#define OUTOF_RANGE(MIN, TEST, MAX)			(((MIN) > (TEST)) || ((TEST) > (MAX)))
#define TOLOWER(C)							((C) | 0x20)
#define TOUPPER(C)							((C) & ~0x20)
#define CHRTOINT(C)							((C) - (char)('0'))
#define CHRXTOINT(C)						(IN_RANGE('0', (C), '9') ? ((C) - (char)('0')) : (TOLOWER(C) - 'a' + 0x0a) )

#define HIBYTE(I16)							((I16) >> 8)
#define LOBYTE(I16)							((I16) & 0xff)

#define MAX(A, B)							(((A) > (B)) ? (A) : (B))
#define MIN(A, B)							(((A) < (B)) ? (A) : (B))

#define MKINT16(HI8, LO8)					(((HI8) << 8) | LO8)

// typedefs
typedef enum { false, true } bool;

#if RFM
	#define VER_NAME "OpenHR20rfm"
#else
	#define VER_NAME "OpenHR20"
#endif

#define VERSION_STRING  ":" VER_NAME " " STR(REVHIGH) "." STR(REVLOW) " " __DATE__ " " __TIME__ " " REVISION

#endif /* CONFIG_H */

