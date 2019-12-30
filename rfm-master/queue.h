/*
 *  Open HR20 - RFM12 master
 *
 *  target:     ATmega32 @ 10 MHz in Honnywell Rondostat HR20E master
 *
 *  compiler:    WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Dario Carluccio (hr20-at-carluccio-dot-de)
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
 * \file       queue.h
 * \brief      command queue to slaves
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>; Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */


#define Q_ITEMS 50

typedef struct
{
	uint8_t len;
	uint8_t addr;
	uint8_t bank;
	uint8_t data[4];
} q_item_t;

// extern q_item_t Q_buf[Q_ITEMS];


uint8_t *Q_push(uint8_t len, uint8_t addr, uint8_t bank);
void Q_clean(uint8_t addr_preserve);
q_item_t *Q_get(uint8_t addr, uint8_t bank, uint8_t skip);
