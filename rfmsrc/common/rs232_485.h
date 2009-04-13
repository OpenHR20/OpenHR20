/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  compiler:    WinAVR-20071221
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
 * \file       rs232_485.h
 * \brief      rs232 and rs485 header
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

#pragma once

#if (defined COM_RS232) || (defined COM_RS485)
	
	#ifdef _AVR_IOM169P_H_
		#define RS_need_clock() (UCSR0B & (_BV(TXEN0) | _BV(RXEN0)))
		#define RS_enable_rx() (UCSR0B |= _BV(RXEN0) | _BV(RXCIE0))
	#elif defined(_AVR_IOM169_H_) || defined(_AVR_IOM16_H_) || defined(_AVR_IOM32_H_)
		#define RS_need_clock() (UCSRB & (_BV(TXEN) | _BV(RXEN)))
		#define RS_enable_rx() (UCSRB |= _BV(RXEN) | _BV(RXCIE))
	#endif
	void RS_startSend(void);
	void RS_Init(void);
#else 
	#define RS_need_clock() (0)
#endif
