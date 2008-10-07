/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  ompiler:    WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Juergen Sachs (juergen-sachs-at-gmx-dot-de)
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
 * \file       rs232_hw.c
 * \brief      hardware layer of the rs232
 * \author     Juergen Sachs (juergen-sachs-at-gmx-dot-de)
 * \date       12.04.2008
 * $Rev$
 */

// AVR LibC includes
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/version.h>
#include <stdlib.h>
#include <string.h>

// HR20 Project includes
#include "config.h"
#include "main.h"
#include "com.h"

/* The following must be AFTER the last include line */
#if defined COM_RS232

volatile uint8_t 	byteToSend;	// counter which byte in the buffer we send next

/*!
 *******************************************************************************
 *  Interrupt for receiving bytes from serial port
 *
 *  \note
 ******************************************************************************/
#ifdef _AVR_IOM169P_H_
ISR(USART0_RX_vect)
#elif _AVR_IOM169_H_
ISR(USART0_RX_vect)
#endif
{
	register char c;
	#ifdef _AVR_IOM169P_H_
		c = UDR0;
	#elif _AVR_IOM169_H_
		c = UDR;
	#endif
	COM_ReceiverChar(c);	// Add char to input buffer
	// do local Echo
	if (doLocalEcho)
	{
		//while(!(UCSRA & UDRE)){};	// wait till free, it should never realy be blocked, since we receive at the same baud
	#ifdef _AVR_IOM169P_H_
		UDR0 = c;
	#elif _AVR_IOM169_H_
		UDR = c ;
	#endif
	}
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
#elif _AVR_IOM169_H_
ISR(USART0_UDRE_vect)
#endif
{
	if (byteToSend < COM_getBytesInBuffer())
	{
		#ifdef _AVR_IOM169P_H_
			UDR0 = COM_getOutByte(byteToSend);
		#elif _AVR_IOM169_H_
			UDR = COM_getOutByte(byteToSend);
		#endif
		byteToSend++;
	}
	else	// no more chars, disable Interrupt
	{
		#ifdef _AVR_IOM169P_H_
			UCSR0B &= ~(_BV(UDRIE0));
		#elif _AVR_IOM169_H_
			UCSRB &= ~(_BV(UDRIE));
		#endif
		byteToSend = 0;
		COM_finishedOutput();	// we are finished, do what ever you want with the buffer
	}
}

/*!
 *******************************************************************************
 *  Initialize serial, setup Baudrate
 *
 *  \note
 *  - set Baudrate
 ******************************************************************************/
void RS232_Init(uint16_t baud)
{
	byteToSend = 0;
	
	// Baudrate berechnen
	//long ubrr_val = ((F_CPU)/(baud*16L)-1);
	uint16_t ubrr_val = ((F_CPU)/(baud*16L)-1);
 
	#ifdef _AVR_IOM169P_H_
		UBRR0H = (unsigned char)(ubrr_val>>8);
		UBRR0L = (unsigned char)(ubrr_val & 0xFF);
		UCSR0B = (_BV(TXEN0) | _BV(RXEN0) | _BV(RXCIE0));      // UART TX einschalten
		UCSR0C = (_BV(UCSZ00) | _BV(UCSZ01));     // Asynchron 8N1 
	#elif _AVR_IOM169_H_
		UBRRH = (unsigned char)(ubrr_val>>8);
		UBRRL = (unsigned char)(ubrr_val & 0xFF);
		UCSRB = (_BV(TXEN) | _BV(RXEN) | _BV(RXCIE));      // UART TX einschalten
		UCSRC = (_BV(UCSZ0) | _BV(UCSZ1));     // Asynchron 8N1 
	#else
		#warn 'your CPU is not supported !'
	#endif
}

/*!
 *******************************************************************************
 *  Starts sending the content of the output buffer
 *
 *  \note
 *  - we send the first char to serial port
 *  - start the interrupt
 *  - increase our byte send marker
 *	- Calc checksum, if there is any
 ******************************************************************************/
void RS232_startSend(void)
{
	if (COM_getBytesInBuffer() > 0)	// Is there something to send ?
	{
		if (byteToSend == 0)	// start sending, we have to start sending 1st byte NOW
		{
			/* if you have to do a Checksum, DO IT HERE !
			You could easyly loop over the byte in the buffer
			for (uint8_t i = 0 ; i < COM_getBytesInBuffer() ; i++)
			{
				chk += COM_getOutByte(i); // calc checksum
			}
			Now start sending the data and checksum.
			Do not forget to call COM_finishedOutput() when done !!!
			*/
			
			/*
			We do not send out first byte here. It may be that the transmitter is still busy
			sending. This could be caused by the local echo.
			If the receiver is free we will get an interrupt after enabling it and all is done
			by the ISR
			*/
			#ifdef _AVR_IOM169P_H_
				//UDR0 = COM_getOutByte(byteToSend);
			#elif _AVR_IOM169_H_
				//UDR = COM_getOutByte(byteToSend);
			#endif
			//byteToSend++;
		}
		#ifdef _AVR_IOM169P_H_
			UCSR0B |= _BV(UDRIE0);	// save is save, we enable this every time
		#elif _AVR_IOM169_H_
			UCSRB |= _BV(UDRIE);	// save is save, we enable this every time
		#endif
	}
}

#endif /* COM_RS232 */
