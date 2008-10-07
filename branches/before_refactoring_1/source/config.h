/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  ompiler:    WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Juergen Sachs (juergen-sachs-at-gmx-dot-de)
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
 * \author     Juergen Sachs (juergen-sachs-at-gmx-dot-de)
 * \date       21.09.2008
 * $Rev$
 */

/*
In this file we define only configuration parameters, for example what kind of control port we have.
*/

#ifndef CONFIG_H
#define CONFIG_H

// AVR LibC includes 
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/version.h>

// our Version
#define REVHIGH  0  //! Revision number high
#define REVLOW   61 //! Revision number low
#define SVNREV   1	//! Subversion Revision, aka build number

// Parameters for the COMM-Port
#define COM_BAUD_RATE 9600
// Note we should only enable of of the following at one time
/* we support RS232 */
//#define COM_RS232 
/* we support RS485 */
#define COM_RS485 
/* Our default Adress, if not set or invalid */
#define COM_DEF_ADR 1


// Some default Values
#define BOOT_DD         7  //!< Boot-Up date: day
#define BOOT_MM         2  //!< Boot-Up date: month
#define BOOT_YY         8  //!< Boot-Up date: year
#define BOOT_hh        22  //!< Boot-Up time: hour
#define BOOT_mm        29  //!< Boot-Up time: minutes




#endif /* CONFIG_H */
