/*
 *  Open HR20
 *
 *  target:     ATmega169 in Honnywell Rondostat HR20E / ATmega8
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
 * \file       cmac.c
 * \brief      CMAC implementation (http://csrc.nist.gov/publications/nistpubs/800-38B/SP_800-38B.pdf)
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

#include <string.h>
#include "config.h"
#include "xtea.h"
#include "wireless.h"

#if RFM

bool cmac_calc(uint8_t *m, uint8_t bytes, uint8_t *data_prefix, bool check)
{
/*   reference: http://csrc.nist.gov/publications/nistpubs/800-38B/SP_800-38B.pdf
 *   1.Let Mlen = message length in bits
 *   2.Let n = Mlen / 64
 *   3.Let M1, M2, ... , Mn-1, Mn
 *    denote the unique sequence of bit strings such that
 *     M = M1 || M2 || ... || Mn-1 || Mn,
 *      where M1, M2,..., Mn-1 are complete 8 byte blocks.
 *   4.If Mn is a complete block, let Mn = K1 XOR Mn else,
 *    let Mn = K2 XOR (Mn ||10j), where j = n*64 - Mlen - 1.
 *   5.Let C0 = 0
 *   6.For i = 1 to n, let Ci = ENC_KMAC(Ci-1 XOR Mi).
 *   7.Let MAC = MSB32(Cn). (4 most significant byte)
 *   8.Add MAC to end of "m"
 */

	uint8_t i, j;
	uint8_t buf[8];

	if (data_prefix == NULL)
	{
		for (i = 0; i < 8; buf[i++] = 0)
		{
			;
		}
	}
	else
	{
		memcpy(buf, data_prefix, 8);
		xtea_enc(buf, buf, K_mac);
	}


	for (i = 0; i < bytes; )   // i modification inside loop
	{
		uint8_t x = i;
		i += 8;
// Ignore maybe ununitialized for Kx, since it is filled (line 77) before being used (line 82)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
		uint8_t *Kx;
#pragma GCC diagnostic pop
		if (i >= bytes)
		{
			Kx = ((i == bytes) ? K1 : K2);
		}
		for (j = 0; j < 8; j++, x++)
		{
			uint8_t tmp;
			if (x < bytes)
			{
				tmp = m[x];
			}
			else
			{
				tmp = ((x == bytes) ? 0x80 : 0);
			}
			if (i >= bytes)
			{
				tmp ^= Kx[j];
			}
			buf[j] ^= tmp;
		}
		xtea_enc(buf, buf, K_mac);
	}
	if (check)
	{
		for (i = 0; i < 4; i++)
		{
			if (m[bytes + i] != buf[i])
			{
				return false;
			}
		}
	}
	else
	{
		memcpy(m + bytes, buf, 4);
	}
	return true;
}

#endif // RFM
