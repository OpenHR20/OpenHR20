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
 * \date       $Date: 2008-10-11 09:25:54 +0200 (so, 11 X 2008) $
 * $Rev: 19 $
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#include "config.h"
#include "eeprom.h"
#include "com.h"
#include "rs232_485.h"
#include "main.h"
#include "rtc.h"
#include "adc.h"


#define TX_BUFF_SIZE 128
#define RX_BUFF_SIZE 32

static char tx_buff[TX_BUFF_SIZE];
static char rx_buff[RX_BUFF_SIZE];

static uint8_t tx_buff_in=0;
static uint8_t tx_buff_out=0;
static uint8_t rx_buff_in=0;
static uint8_t rx_buff_out=0;

static int COM_getchar(FILE *stream);
static int COM_putchar(char c, FILE *stream);
static FILE COM_stdout = FDEV_SETUP_STREAM(COM_putchar, NULL, _FDEV_SETUP_WRITE);
static FILE COM_stdin  = FDEV_SETUP_STREAM(NULL, COM_getchar, _FDEV_SETUP_READ);



/*!
 *******************************************************************************
 *  transmit bytes
 *
 *  \note
 ******************************************************************************/
static int COM_putchar(char c, FILE *stream) {
	cli();
	if ((tx_buff_in+1)%TX_BUFF_SIZE!=tx_buff_out) {
		tx_buff[tx_buff_in++]=c;
		tx_buff_in%=TX_BUFF_SIZE;
	}
	sei();
	return 0;
}

/*!
 *******************************************************************************
 *  support for interrupt for transmit bytes
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
 *  support for interrupt for receive bytes
 *
 *  \note
 ******************************************************************************/
void COM_rx_char_isr(char c) {
	if ((rx_buff_in+1)%RX_BUFF_SIZE!=rx_buff_out) {
		rx_buff[rx_buff_in++]=c;
		rx_buff_in%=RX_BUFF_SIZE;
	}
}

/*!
 *******************************************************************************
 *  receive bytes
 *
 *  \note
 ******************************************************************************/
static int COM_getchar(FILE *stream) {
	cli();
	char c='\0';
	if (rx_buff_in!=rx_buff_out) {
		c=rx_buff[rx_buff_out++];
		rx_buff_out%=RX_BUFF_SIZE;
	}
	sei();
	return c;
}

/*!
 *******************************************************************************
 *  flush output buffer
 *
 *  \note
 ******************************************************************************/
static void COM_flush (void) {
	#if (defined COM_RS232) || (defined COM_RS485)
		RS_Init(COM_BAUD_RATE);
		RS_startSend();
	#else 
		#error "need todo"
	#endif
}


/*!
 *******************************************************************************
 *  helper function print 2 digit dec number
 *
 *  \note only unsigned numbers
 ******************************************************************************/
static void print_decXX(uint8_t i) {
	if (i>=100) {
		putchar(i/100+'\0');
		i%=100;
	}
	putchar(i/10+'0');
	putchar(i%10+'0');
}

/*!
 *******************************************************************************
 *  helper function print 4 digit dec number
 *
 *  \note only unsigned numbers
 ******************************************************************************/
static void print_decXXXX(uint16_t i) {
	print_decXX(i/100);
	print_decXX(i%100);
}

/*!
 *******************************************************************************
 *  helper function print 2 digit dec number
 *
 *  \note only unsigned numbers
 ******************************************************************************/
static void print_hexXX(uint8_t i) {
	uint8_t x = i>>4;
	if (x>10) {
		putchar(x+'a'-10);	
	} else {
		putchar(x+'0');
	}	
	x = i & 0xf;
	if (x>10) {
		putchar(x+'a'-10);	
	} else {
		putchar(x+'0');
	}	
}

/*!
 *******************************************************************************
 *  helper function print 4 digit dec number
 *
 *  \note only unsigned numbers
 ******************************************************************************/
static void print_hexXXXX(uint16_t i) {
	print_hexXX(i>>8);
	print_hexXX(i&0xff);
}


/*!
 *******************************************************************************
 *  helper function print string without \n2 digit dec number
 *
 *  \note only unsigned numbers
 ******************************************************************************/
static void print_s(char * s) {
	while (*s != '\0') {
		putchar(*(s++));
	}
}


/*!
 *******************************************************************************
 *  init communication
 *
 *  \note
 ******************************************************************************/
void COM_init(void) {
	stdout = &COM_stdout;
	stdin = &COM_stdin;
	print_s("Hello, world!\n");
	COM_flush();
}


/*!
 *******************************************************************************
 *  receive bytes
 *
 *  \note
 ******************************************************************************/
void debug_print(void) {
	print_s("D: ");
	print_decXX(RTC_GetDay());
	putchar('.');
	print_decXX(RTC_GetMonth());
	putchar('.');
	print_decXX(RTC_GetYearYY());
	putchar(' ');
	print_decXX(RTC_GetHour());
	putchar(':');
	print_decXX(RTC_GetMinute());
	putchar(':');
	print_decXX(RTC_GetSecond());
	print_s(" V: ");
	print_decXX(valve_wanted); // or MOTOR_GetPosPercent()
	print_s(" I: ");
	print_decXXXX(temp_average);
	print_s(" S: ");
	print_decXXXX(calc_temp(temp_wanted));
	putchar('\n');
	print_hexXXXX(0x1234);
	COM_flush();
}

