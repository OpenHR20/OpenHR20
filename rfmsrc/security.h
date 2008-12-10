/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  compiler:   WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Mario Fischer (MarioFischer-at-gmx-dot-net)
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
 * \brief      functions to secure the RFM12 Radio Transceiver Module's communication
 * \author     Mario Fischer <MarioFischer-at-gmx-dot-net>
 * \date       $Date$
 * $Rev$
 */

#pragma once // multi-iclude prevention. gcc knows this pragma

#define XTEA_KEYSIZE		16 //!< XTEA encryption property. dont change!
#define XTEA_BLOCKSIZE		8  //!< XTEA encryption property. dont change!
#define XTEA_NUMOFROUNDS	64 //!< num of rounds of one XTEA encryption procedure. suggestion: 64

#include "config.h"

void security_init(void);
uint16_t security_generate_challenge(void);
uint16_t security_generate_response(uint16_t remote_chall);
bool security_check_response(uint16_t remote_resp);


void security_crypt(uint8_t* msgbuf, uint8_t msgsize, bool encrypt);

#define security_encrypt(msgbuf, msgsize)	security_crypt(msgbuf, msgsize, true)
#define security_decrypt(msgbuf, msgsize)	security_crypt(msgbuf, msgsize, false)



