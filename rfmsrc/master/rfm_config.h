/*
 *  Open HR20
 *
 *  target:     ATmega16 @ 10 MHz in Honnywell Rondostat HR20E master
 *
 *  compiler:   WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Dario Carluccio (hr20-at-carluccio-dot-de)
 *              2008 Jiri Dobry (jdobry-at-centrum-dot-cz) 
 *              2008 Mario Fischer (MarioFischer-at-gmx-dot-net)
 *              2007 Michael Smola (Michael-dot-Smola-at-gmx-dot-net)
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
 * \file       rfm.c
 * \brief      configurations to control the RFM12 Radio Transceiver Module
 * \author     Mario Fischer <MarioFischer-at-gmx-dot-net>; Michael Smola <Michael-dot-Smola-at-gmx-dot-net>
 * \date       $Date$
 * $Rev$
 */

#pragma once // multi-iclude prevention. gcc knows this pragma


///////////////////////////////////////////////////////////////////////////////
//
// RFM wiring an SPI backend
//
///////////////////////////////////////////////////////////////////////////////

// schematics: 
// for wiring see file wiring.txt

#define RFM_SCK_DDR			DDRB
#define RFM_SCK_PORT		PORTB
#define RFM_SCK_BITPOS		7

#define RFM_SDI_DDR			DDRB
#define RFM_SDI_PORT		PORTB
#define RFM_SDI_BITPOS		5

#define RFM_NSEL_DDR		DDRB
#define RFM_NSEL_PORT		PORTB
#define RFM_NSEL_BITPOS		4

#define RFM_SDO_DDR			DDRB
#define RFM_SDO_PIN			PINB
#define RFM_SDO_BITPOS		6

/*
#define RFM_NIRQ_DDR		DDRE
#define RFM_NIRQ_PIN		PINE
#define RFM_NIRQ_BITPOS		2
*/

#define RFM_CLK_OUTPUT 1

void INT2_vect(void);

#define RFM_INT_EN() (GICR |= _BV(INT2), INT2_vect())
#define RFM_INT_DIS() (GICR &= ~_BV(INT2))
