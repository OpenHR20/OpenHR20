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
 * \file       com.h
 * \brief      communication layer
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

#pragma once

#include "debug.h"

char COM_tx_char_isr(void);

void COM_rx_char_isr(char c);

void COM_init(void);

void COM_print_debug(uint8_t type);

void COM_commad_parse(void);
#if RFM == 1
void COM_wireless_command_parse(uint8_t *rfm_framebuf, uint8_t rfm_framepos);
#endif

void COM_debug_print_motor(int8_t dir, uint16_t m, uint8_t pwm);
void COM_debug_print_temperature(uint16_t t);

#if DEBUG_DUMP_RFM
void COM_dump_packet(uint8_t *d, uint8_t len, bool mac_ok);
// void COM_mac_ok(void);
#else
#define COM_dump_packet(d, len, mac_ok)
// #define COM_mac_ok() ()
#endif
#if DEBUG_PRINT_ADDITIONAL_TIMESTAMPS
void COM_print_time(uint8_t c);
#else
#define COM_print_time(c)
#endif

void COM_putchar(char c);
void COM_flush(void);
void COM_printStr16(const char *s, uint16_t x);
