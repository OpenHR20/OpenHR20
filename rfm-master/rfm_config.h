/*
 *  Open HR20
 *
 *  target:     ATmega32 @ 10 MHz in Honnywell Rondostat HR20E master
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

#if (NANODE == 1)
// nanode has the following pins used: (PORT D=digital 0-7, PORT B=digital 8-13)
// D3 (digital 3) = RFM12B Interrupt (INT 1) - connect to SDO (B4, digital 12)
// B2 Digital 10	 Slave Select for RFM12B - if installed
// B3 Digital 11	 SPI bus: Shared MOSI (Master Output, Slave Input)
// B4 Digital 12	 SPI bus: Shared MISO (Master Input, Slave Output)
// B5 Digital 13	 SPI bus: Shared Serial Clock (output from master)
#define RFM_SCK_DDR                 DDRB
#define RFM_SCK_PORT                PORTB
#define RFM_SCK_BITPOS              5

#define RFM_SDI_DDR                 DDRB
#define RFM_SDI_PORT                PORTB
#define RFM_SDI_BITPOS              3

#define RFM_NSEL_DDR                DDRB
#define RFM_NSEL_PORT               PORTB
#define RFM_NSEL_BITPOS             2

#define RFM_SDO_DDR                 DDRB
#define RFM_SDO_PIN                 PINB
#define RFM_SDO_BITPOS              4
#elif (JEENODE == 1)
// jeenode (http://www.jeelabs.org/; Atmega328P) has the following pins used:
// D2 Digital 3      RFM12B Interrupt (INT 0)
// B2 Digital 10	 Slave Select for RFM12B
// B3 Digital 11	 SPI bus: Shared MOSI (Master Output, Slave Input)
// B4 Digital 12	 SPI bus: Shared MISO (Master Input, Slave Output)
// B5 Digital 13	 SPI bus: Shared Serial Clock (output from master)
#define RFM_SCK_DDR                 DDRB
#define RFM_SCK_PORT                PORTB
#define RFM_SCK_BITPOS              5

#define RFM_SDI_DDR                 DDRB
#define RFM_SDI_PORT                PORTB
#define RFM_SDI_BITPOS              3

#define RFM_NSEL_DDR                DDRB
#define RFM_NSEL_PORT               PORTB
#define RFM_NSEL_BITPOS             2

#define RFM_SDO_DDR                 DDRB
#define RFM_SDO_PIN                 PINB
#define RFM_SDO_BITPOS              4
#else
// original master
#define RFM_SCK_DDR                 DDRB
#define RFM_SCK_PORT                PORTB
#define RFM_SCK_BITPOS              7

#define RFM_SDI_DDR                 DDRB
#define RFM_SDI_PORT                PORTB
#define RFM_SDI_BITPOS              5

#define RFM_NSEL_DDR                DDRB
#define RFM_NSEL_PORT               PORTB
#define RFM_NSEL_BITPOS             4

#define RFM_SDO_DDR                 DDRB
#define RFM_SDO_PIN                 PINB
#define RFM_SDO_BITPOS              6
#endif
/*
 * #define RFM_NIRQ_DDR		DDRE
 * #define RFM_NIRQ_PIN		PINE
 * #define RFM_NIRQ_BITPOS		2
 */

#if (NANODE == 1)
#define RFM_CLK_OUTPUT 1
void INT1_vect(void);     // should be able to disable clock output?
#define RFM_INT_vect INT1_vect
#define RFM_INT_EN_NOCALL() (EIMSK |= _BV(INT1))
#define RFM_INT_DIS() (EIMSK &= ~_BV(INT1))
#elif (JEENODE == 1)
#define RFM_CLK_OUTPUT 0
void INT0_vect(void);
#define RFM_INT_vect INT0_vect
#define RFM_INT_EN_NOCALL() (EIMSK |= _BV(INT0))
#define RFM_INT_DIS() (EIMSK &= ~_BV(INT0))
#else
#define RFM_CLK_OUTPUT 1
void INT2_vect(void);
#define RFM_INT_vect INT2_vect
#define RFM_INT_EN_NOCALL() (GICR |= _BV(INT2))
#define RFM_INT_DIS() (GICR &= ~_BV(INT2))
#endif

#define RFM_INT_EN() (RFM_INT_EN_NOCALL(), RFM_INT_vect())

#ifndef RFM_TUNING
#define RFM_TUNING 0
#endif

#ifndef RFM_TUNING_MODE
#define RFM_TUNING_MODE 0
#endif
