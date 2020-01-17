/*
 *  Open HR20
 *
 *  target:     ATmega32 @ 10 MHz in Honnywell Rondostat HR20E
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
 * In this file we define only configuration parameters, for example what kind of control port we have.
 */

#pragma once

#define CONFIG_H  1
#define MASTER_CONFIG_H  1

// AVR LibC includes
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/version.h>
#include <avr/fuse.h>


#define LANG_uni 1
#define LANG_de 2
#define LANG_cs 3
//! language
#define LANG LANG_uni
//#define LANG LANG_de
//#define LANG LANG_cs

#define _STR(x) #x
#define STR(x) _STR(x)

#ifndef REVISION
#define REVISION "$Rev$"
#endif
#define VERSION_STRING  "V: OpenHR20 master SW version 1.1 build " __DATE__ " " __TIME__ " " REVISION

// Parameters for the COMM-Port
#define COM_BAUD_RATE 38400
// Note we should only enable of of the following at one time
/* we support UART */
#define COM_UART 1

/* Our default Adress, if not set or invalid */
/* #define COM_DEF_ADR 1 */

#if (NANODE == 1)
#define LED_RX_on() (PORTD |= _BV(PD5))
#define LED_RX_off() (PORTD &= ~_BV(PD5))

#define LED_sync_on()        (PORTD |= _BV(PD6))
#define LED_sync_off()  (PORTD &= ~_BV(PD6))
#elif (JEENODE == 1)
// jeenode has only one LED, use it for both functions
#define LED_RX_on()   (PORTB &= ~_BV(PB1))
#define LED_RX_off()    (PORTB |= _BV(PB1))

#define LED_sync_on() (PORTB &= ~_BV(PB1))
#define LED_sync_off()  (PORTB |= _BV(PB1))
#else
#if (ATMEGA32_DEV_BOARD == 1)
#define LED_RX_on() (PORTA |= _BV(PA1))
#define LED_RX_off() (PORTA &= ~_BV(PA1))
#else
#define LED_RX_on() (PORTD |= _BV(PD7))
#define LED_RX_off() (PORTD &= ~_BV(PD7))
#endif
#define LED_sync_on()        (PORTA |= _BV(PA2))
#define LED_sync_off()  (PORTA &= ~_BV(PA2))
#endif

#define RFM 1 //!< define RFM to 1 if you want to have support for the RFM Radio Moodule in the Code

#if (RFM == 1)
#define RFM_DEVICE_ADDRESS 0x00
#define DISABLE_JTAG           0
#else
#define DISABLE_JTAG           0
#define RFM_TUNING             0
#endif

/* compiler compatibility */
#ifndef ISR_NAKED
#   define ISR_NAKED      __attribute__((naked))
#endif

// some handy macros

#define ELEMCNT(A)                                                      (sizeof(A) / ELEMSIZE(A))
#define OFFSETOF(TYPE, ELEM)                            ((size_t)((char *)&((TYPE *)0)->ELEM - (char *)((TYPE *)NULL)))
#define CMP(X, Y)                                                        (((X) == (Y)) ? 0 : (((X) < (Y)) ? -1 : +1))
#define LITERAL_STRLEN(S)                                       (sizeof(S) - sizeof('\0'))
#define ARRAY(TYPE, PTR, INDEX)                         ((TYPE *)(PTR))[INDEX]

#define IN_RANGE(MIN, TEST, MAX)                        (((MIN) <= (TEST)) && ((TEST) <= (MAX)))
#define OUTOF_RANGE(MIN, TEST, MAX)                     (((MIN) > (TEST)) || ((TEST) > (MAX)))
#define TOLOWER(C)                                                      ((C) | 0x20)
#define TOUPPER(C)                                                      ((C)&~0x20)
#define CHRTOINT(C)                                                     ((C)-(char)('0'))
#define CHRXTOINT(C)                                            (IN_RANGE('0', (C), '9') ? ((C)-(char)('0')) : (TOLOWER(C) - 'a' + 0x0a))

#define HIBYTE(I16)                                                     ((I16) >> 8)
#define LOBYTE(I16)                                                     ((I16) & 0xff)

#define MAX(A, B)                                                       (((A) > (B)) ? (A) : (B))
#define MIN(A, B)                                                       (((A) < (B)) ? (A) : (B))

#define MKINT16(HI8, LO8)                                       (((HI8) << 8) | LO8)


// typedefs
typedef enum { false, true } bool;

// Some default Values
#define BOOT_DD         1       //!< Boot-Up date: day
#define BOOT_MM         1       //!< Boot-Up date: month
#define BOOT_YY        13       //!< Boot-Up date: year
#define BOOT_hh        12       //!< Boot-Up time: hour
#define BOOT_mm        00       //!< Boot-Up time: minutes
