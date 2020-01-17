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
 * \file       queue.c
 * \brief      command queue to slaves
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

#include <stdint.h>
#include <string.h>

// HR20 Project includes
#include "config.h"
#include "queue.h"

static q_item_t Q_buf[Q_ITEMS];

/*!
 *******************************************************************************
 *  \brief push one item si queue
 *
 *  \note
 ******************************************************************************/
uint8_t *Q_push(uint8_t len, uint8_t addr, uint8_t bank)
{
	uint8_t i;
	uint8_t free = 0xff;

	for (i = 0; i < Q_ITEMS; i++)
	{
		if ((Q_buf[i].addr == addr) && (Q_buf[i].bank == bank))
		{
			free = 0xff;
		}
		else
		if ((free == 0xff) && (Q_buf[i].addr == 0))
		{
			free = i;
		}
	}
	if (free == 0xff)
	{
		return NULL;
	}
	Q_buf[free].len = len;
	Q_buf[free].addr = addr;
	Q_buf[free].bank = bank;
	return Q_buf[free].data;
}

/*!
 *******************************************************************************
 *  \brief clean buffer for addr
 *
 *  \note
 ******************************************************************************/
void Q_clean(uint8_t addr_preserve)
{
	uint8_t i;

	for (i = 0; i < Q_ITEMS; i++)
	{
		if (Q_buf[i].addr != addr_preserve)
		{
			Q_buf[i].addr = 0;
		}
	}
}

/*!
 *******************************************************************************
 *  \brief get items for addr_bank
 *
 *  \note
 ******************************************************************************/
q_item_t *Q_get(uint8_t addr, uint8_t bank, uint8_t skip)
{
	uint8_t i;

	for (i = 0; i < Q_ITEMS; i++)
	{
		if ((Q_buf[i].addr == addr) && (Q_buf[i].bank == bank))
		{
			if ((skip--) == 0)
			{
				return Q_buf + i;
			}
		}
	}
	return NULL;
}
