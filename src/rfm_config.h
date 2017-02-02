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
// jiris internal wiring:       rfm_sck=pf1     rfm_sdi=pf0     rfm_nsel=pa3     rfm_sdo=pe6=pcint6    rfm_nirq=open
// marios external jtag wiring: rfm_sck=pf4=tck rfm_sdi=pf6=tdo rfm_nsel=pf5=tms rfm_sdo=pe2=pcint2    rfm_nirq=open
// tomas internal wiring:       rfm_sck=pf1     rfm_sdi=pf7     rfm_nsel=pf0     rfm_sdo=pe6=pcint6    rfm_nirq=open


#if (RFM_WIRE_MARIOJTAG == 1)

#define RFM_SCK_DDR                     DDRF
#define RFM_SCK_PORT            PORTF
#define RFM_SCK_BITPOS          4

#define RFM_SDI_DDR                     DDRF
#define RFM_SDI_PORT            PORTF
#define RFM_SDI_BITPOS          6

#define RFM_NSEL_DDR            DDRF
#define RFM_NSEL_PORT           PORTF
#define RFM_NSEL_BITPOS         5

#define RFM_SDO_DDR                     DDRE
#define RFM_SDO_PIN                     PINE
#define RFM_SDO_BITPOS          2

#define RFM_SDO_PCINT           PCINT2

/*
 * #define RFM_NIRQ_DDR		DDRE
 * #define RFM_NIRQ_PIN		PINE
 * #define RFM_NIRQ_BITPOS		2
 */
#elif (RFM_WIRE_TK_INTERNAL == 1)
#define RFM_SCK_DDR                     DDRF
#define RFM_SCK_PORT            PORTF
#define RFM_SCK_BITPOS          1

#define RFM_SDI_DDR                     DDRF
#define RFM_SDI_PORT            PORTF
#define RFM_SDI_BITPOS          7

#define RFM_NSEL_DDR            DDRF
#define RFM_NSEL_PORT           PORTF
#define RFM_NSEL_BITPOS         0

#define RFM_SDO_DDR                     DDRE
#define RFM_SDO_PIN                     PINE
#define RFM_SDO_BITPOS          6

#define RFM_SDO_PCINT           PCINT6
#elif (RFM_WIRE_JD_INTERNAL == 1)
#define RFM_SCK_DDR                     DDRF
#define RFM_SCK_PORT            PORTF
#define RFM_SCK_BITPOS          1

#define RFM_SDI_DDR                     DDRF
#define RFM_SDI_PORT            PORTF
#define RFM_SDI_BITPOS          0

#define RFM_NSEL_DDR            DDRA
#define RFM_NSEL_PORT           PORTA
#define RFM_NSEL_BITPOS         3

#define RFM_SDO_DDR                     DDRE
#define RFM_SDO_PIN                     PINE
#define RFM_SDO_BITPOS          6

#define RFM_SDO_PCINT           PCINT6
#endif

#define RFM_CLK_OUTPUT 0

void PCINT0_vect(void);

#define RFM_INT_EN() (PCMSK0 |= _BV(RFM_SDO_PCINT), PCINT0_vect())
#define RFM_INT_DIS() (PCMSK0 &= ~_BV(RFM_SDO_PCINT))

#ifndef RFM_TUNING
#define RFM_TUNING 0
#endif

#ifndef RFM_TUNING_MODE
#define RFM_TUNING_MODE 0
#endif
