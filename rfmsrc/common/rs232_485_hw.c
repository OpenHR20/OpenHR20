/*
 *  Open HR20
 *
 *  target:     ATmega169 in Honnywell Rondostat HR20E / ATmega8
 *
 *  compiler:   WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Juergen Sachs (juergen-sachs-at-gmx-dot-de)
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
 * \file       rs232_485_hw.c
 * \brief      hardware layer of the rs232 and rs485
 * \author     Juergen Sachs (juergen-sachs-at-gmx-dot-de); Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

// AVR LibC includes
#include <stdint.h>
#include <avr/io.h>
#include <stdio.h>

// HR20 Project includes
#include "config.h"
#include "main.h"
#include "com.h"

/* The following must be AFTER the last include line */
#if (defined COM_RS232) || (defined COM_RS485)


/*!
 *******************************************************************************
 *  Interrupt for receiving bytes from serial port
 *
 *  \note
 ******************************************************************************/
#ifdef _AVR_IOM169P_H_
ISR(USART0_RX_vect)
#elif defined(_AVR_IOM169_H_)
ISR(USART0_RX_vect)
#elif defined(_AVR_IOM16_H_)
ISR(USART_RXC_vect)
#endif
{
	#ifdef _AVR_IOM169P_H_
		COM_rx_char_isr(UDR0);	// Add char to input buffer
		UCSR0B &= ~(_BV(RXEN0)|_BV(RXCIE0)); // disable receive
	#elif defined(_AVR_IOM169_H_) || defined(_AVR_IOM16_H_)
		COM_rx_char_isr(UDR);	// Add char to input buffer
		UCSRB &= ~(_BV(RXEN)|_BV(RXCIE)); // disable receive
	#endif
	#if !defined(_AVR_IOM16_H_)
    PCMSK0 |= (1<<PCINT0); // activate interrupt
  #endif
}

/*!
 *******************************************************************************
 *  Interrupt, we can send one more byte to serial port
 *
 *  \note
 *  - We send one byte
 *  - If we have send all, disable interrupt
 ******************************************************************************/
#ifdef _AVR_IOM169P_H_
ISR(USART0_UDRE_vect)
#elif defined(_AVR_IOM169_H_) || defined(_AVR_IOM16_H_)
ISR(USART_UDRE_vect)
#endif
{
	char c;
	if ((c=COM_tx_char_isr())!='\0')
	{
		#ifdef _AVR_IOM169P_H_
			UDR0 = c;
		#elif defined(_AVR_IOM169_H_) || defined(_AVR_IOM16_H_)
			UDR = c;
		#endif
	}
	else	// no more chars, disable Interrupt
	{
		#ifdef _AVR_IOM169P_H_
			UCSR0B &= ~(_BV(UDRIE0));
			UCSR0A |= _BV(TXC0); // clear interrupt flag
			UCSR0B |= (_BV(TXCIE0));
		#elif defined(_AVR_IOM169_H_) || defined(_AVR_IOM16_H_)
			UCSRB &= ~(_BV(UDRIE));
			UCSRA |= _BV(TXC); // clear interrupt flag
			UCSRB |= (_BV(TXCIE));
		#endif
	}
}

/*!
 *******************************************************************************
 *  Interrupt for transmit done
 *
 *  \note
 ******************************************************************************/
#ifdef _AVR_IOM169P_H_
ISR(USART0_TX_vect)
#elif defined(_AVR_IOM169_H_)
ISR(USART0_TX_vect)
#elif defined(_AVR_IOM16_H_)
ISR(USART_TXC_vect)
#endif
{
	#if defined COM_RS485
		// TODO: change rs485 to receive
		#error "code is not complete for rs485"
	#endif
	#ifdef _AVR_IOM169P_H_
		UCSR0B &= ~(_BV(TXCIE0)|_BV(TXEN0));
	#elif defined(_AVR_IOM169_H_) || defined(_AVR_IOM16_H_)
		UCSRB &= ~(_BV(TXCIE)|_BV(TXEN));
	#endif
}

/*!
 *******************************************************************************
 *  Initialize serial, setup Baudrate
 *
 *  \note
 *  - set Baudrate
 ******************************************************************************/
void RS_Init(uint16_t baud)
{
	
	// Baudrate
	//long ubrr_val = ((F_CPU)/(baud*8L)-1);
	uint16_t ubrr_val = ((F_CPU)/(baud*8L)-1);
 
	#ifdef _AVR_IOM169P_H_
		UCSR0C = _BV(U2X0);
		UBRR0H = (unsigned char)(ubrr_val>>8);
		UBRR0L = (unsigned char)(ubrr_val & 0xFF);
		UCSR0C = (_BV(UCSZ00) | _BV(UCSZ01));     // Asynchron 8N1 
	#elif defined(_AVR_IOM169_H_) || defined(_AVR_IOM16_H_)
		UCSRA = _BV(U2X);
		UBRRH = (unsigned char)(ubrr_val>>8);
		UBRRL = (unsigned char)(ubrr_val & 0xFF);
		#if defined(_AVR_IOM16_H_)
            UCSRC = (1<<URSEL) | (_BV(UCSZ0) | _BV(UCSZ1));     // Asynchron 8N1
        #else 
            UCSRC = (_BV(UCSZ0) | _BV(UCSZ1));     // Asynchron 8N1
        #endif
	#else
		#error 'your CPU is not supported !'
	#endif
	#if !defined(_AVR_IOM16_H_)
    PCMSK0 |= (1<<PCINT0); // activate interrupt
  #endif
}

/*!
 *******************************************************************************
 *  Starts sending the content of the output buffer
 *
 *  \note
 *  - we send the first char to serial port
 *  - start the interrupt
 ******************************************************************************/
void RS_startSend(void)
{
	cli();
	#if defined COM_RS485
		// TODO: change rs485 to transmit
		#error "code is not complete for rs485"
	#endif
	#ifdef _AVR_IOM169P_H_
		if ((UCSR0B & _BV(UDRIE0))==0) {
			UCSR0B &= ~(_BV(TXCIE0));
			UCSR0A |= _BV(TXC0); // clear interrupt flag
			UCSR0B |= _BV(UDRIE0) | _BV(TXEN0);
			// UDR0 = COM_tx_char_isr(); // done in interrupt
		}
	#elif defined(_AVR_IOM169_H_) || defined(_AVR_IOM16_H_)
		if ((UCSRB & _BV(UDRIE))==0) {
			UCSRB &= ~(_BV(TXCIE));
			UCSRA |= _BV(TXC); // clear interrupt flag
			UCSRB |= _BV(UDRIE)  | _BV(TXEN); 
			//UDR = COM_tx_char_isr(); // done in interrupt
		}
	#endif
	sei();
}

#endif /* COM_RS232 */
