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
 * \file       watch.h
 * \brief      watch variable for debug
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       02.10.2008
 * $Rev$
 */

#include <stdint.h>
#include <stdlib.h>
#include <avr/pgmspace.h>

#include "main.h"
#include "adc.h"
#include "pid.h"


#define B8 0x0000
#define B16 0x8000
#define B_MASK 0x8000



static uint16_t watch_map[] PROGMEM = {
    /* 00 */ ((uint16_t) &temp_average) + B16, // temperature 
    /* 01 */ ((uint16_t) &bat_average) + B16,  // battery 
    /* 02 */ ((uint16_t) &sumError) + 2 + B16,
    /* 03 */ ((uint16_t) &sumError) + B16,
};



uint16_t watch(uint8_t addr) {
	uint16_t p;
	if (addr >= sizeof(watch_map)/2) return 0;

	p=pgm_read_word(&watch_map[addr]);
	if ((p&B_MASK)==B16) { // 16 bit value
		return *((uint16_t *)(p & ~B_MASK));
	} else { // 8 bit value
		return (uint16_t)(*((uint8_t *)(p & ~B_MASK)));
	}
}

