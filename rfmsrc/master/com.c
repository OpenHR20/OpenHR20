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
 * \file       com.c
 * \brief      comunication
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/wdt.h>


#include "config.h"
#include "com.h"
#include "../common/rs232_485.h"
#include "../common/rtc.h"
#include "task.h"


#define TX_BUFF_SIZE 128
#define RX_BUFF_SIZE 32

static char tx_buff[TX_BUFF_SIZE];
static char rx_buff[RX_BUFF_SIZE];

static uint8_t tx_buff_in=0;
static uint8_t tx_buff_out=0;
static uint8_t rx_buff_in=0;
static uint8_t rx_buff_out=0;

/*!
 *******************************************************************************
 *  \brief transmit bytes
 *
 *  \note
 ******************************************************************************/
static void COM_putchar(char c) {
	cli();
	if ((tx_buff_in+1)%TX_BUFF_SIZE!=tx_buff_out) {
		tx_buff[tx_buff_in++]=c;
		tx_buff_in%=TX_BUFF_SIZE;
	}
	sei();
}

/*!
 *******************************************************************************
 *  \brief support for interrupt for transmit bytes
 *
 *  \note
 ******************************************************************************/
char COM_tx_char_isr(void) {
	char c='\0';
	if (tx_buff_in!=tx_buff_out) {
		c=tx_buff[tx_buff_out++];
		tx_buff_out%=TX_BUFF_SIZE;
	}
	return c;
}

/*!
 *******************************************************************************
 *  \brief support for interrupt for receive bytes
 *
 *  \note
 ******************************************************************************/
void COM_rx_char_isr(char c) {
	if (c!='\0') {  // ascii based protocol, \0 char is not alloweed, ignore it
		if (c=='\r') c='\n';  // mask diffrence between operating systems
		rx_buff[rx_buff_in++]=c;
		rx_buff_in%=RX_BUFF_SIZE;
		if (rx_buff_in==rx_buff_out) { // buffer overloaded, drop oldest char 
			rx_buff_out++;
			rx_buff_out%=RX_BUFF_SIZE;
		}
		if (c=='\n') {
			task |= TASK_COM;
		}
	}
}

/*!
 *******************************************************************************
 *  \brief receive bytes
 *
 *  \note
 ******************************************************************************/
static char COM_getchar(void) {
	char c='\0';
	cli();
	if (rx_buff_in!=rx_buff_out) {
		c=rx_buff[rx_buff_out++];
		rx_buff_out%=RX_BUFF_SIZE;
	}
	sei();
	return c;
}

/*!
 *******************************************************************************
 *  \brief flush output buffer
 *
 *  \note
 ******************************************************************************/
static void COM_flush (void) {
	if (tx_buff_in!=tx_buff_out) {
		#if (defined COM_RS232) || (defined COM_RS485)
			RS_startSend();
		#else 
			#error "need todo"
		#endif
	}
}


/*!
 *******************************************************************************
 *  \brief helper function print 2 digit dec number
 *
 *  \note only unsigned numbers
 ******************************************************************************/
static void print_decXX(uint8_t i) {
	if (i>=100) {
		COM_putchar(i/100+'0');
		i%=100;
	}
	COM_putchar(i/10+'0');
	COM_putchar(i%10+'0');
}

/*!
 *******************************************************************************
 *  \brief helper function print 4 digit dec number
 *
 *  \note only unsigned numbers
 ******************************************************************************/
static void print_decXXXX(uint16_t i) {
	print_decXX(i/100);
	print_decXX(i%100);
}

/*!
 *******************************************************************************
 *  \brief helper function print 2 digit dec number
 *
 *  \note only unsigned numbers
 ******************************************************************************/
static void print_hexXX(uint8_t i) {
	uint8_t x = i>>4;
	if (x>=10) {
		COM_putchar(x+'a'-10);	
	} else {
		COM_putchar(x+'0');
	}	
	x = i & 0xf;
	if (x>=10) {
		COM_putchar(x+'a'-10);	
	} else {
		COM_putchar(x+'0');
	}	
}

/*!
 *******************************************************************************
 *  \brief helper function print 4 digit dec number
 *
 *  \note only unsigned numbers
 ******************************************************************************/
static void print_hexXXXX(uint16_t i) {
	print_hexXX(i>>8);
	print_hexXX(i&0xff);
}


/*!
 *******************************************************************************
 *  \brief helper function print string without \n2 digit dec number
 *
 *  \note only unsigned numbers
 ******************************************************************************/
static void print_s_p(const char * s) {
	char c;
	for (c = pgm_read_byte(s); c; ++s, c = pgm_read_byte(s)) {
      COM_putchar(c);
   	}
}

/*!
 *******************************************************************************
 *  \brief helper function print version string
 *
 *  \note
 ******************************************************************************/
static void print_version(void) {
	print_s_p(PSTR(VERSION_STRING "\n"));
}


/*!
 *******************************************************************************
 *  \brief init communication
 *
 *  \note
 ******************************************************************************/
void COM_init(void) {
	print_version();
	RS_Init(COM_BAUD_RATE);
	COM_flush();
}

/*!
 *******************************************************************************
 *  \brief dump data from *d length len
 *
 *  \note
 ******************************************************************************/
static uint16_t seq=0;
void COM_dump_packet(uint8_t *d, uint8_t len) {
	print_decXX(RTC_GetSecond());
	COM_putchar('.');
	print_decXX(RTC_s100);
    print_s_p(PSTR(" PKT"));
    print_hexXXXX(seq++);
    COM_putchar(':');
	while ((len--)>0) {
        COM_putchar(' ');
        print_hexXX(*(d++));
    }
    COM_putchar('\n');
	COM_flush();
}

void COM_print_datetime() {
    print_hexXX(RTC_GetDayOfWeek()+0xd0);
	COM_putchar(' ');
	print_decXX(RTC_GetDay());
	COM_putchar('.');
	print_decXX(RTC_GetMonth());
	COM_putchar('.');
	print_decXX(RTC_GetYearYY());
	COM_putchar(' ');
	print_decXX(RTC_GetHour());
	COM_putchar(':');
	print_decXX(RTC_GetMinute());
	COM_putchar(':');
	print_decXX(RTC_GetSecond());
	COM_putchar('\n');
    COM_flush();
}
