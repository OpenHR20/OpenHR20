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


#include "config.h"
#include "eeprom.h"
#include "com.h"
#include "rs232_485.h"
#include "main.h"
#include "rtc.h"
#include "adc.h"
#include "task.h"
#include "watch.h"


#define TX_BUFF_SIZE 128
#define RX_BUFF_SIZE 32

static char tx_buff[TX_BUFF_SIZE];
static char rx_buff[RX_BUFF_SIZE];

static uint8_t tx_buff_in=0;
static uint8_t tx_buff_out=0;
static uint8_t rx_buff_in=0;
static uint8_t rx_buff_out=0;

static int COM_putchar(char c, FILE *stream);
static FILE COM_stdout = FDEV_SETUP_STREAM(COM_putchar, NULL, _FDEV_SETUP_WRITE);


static volatile uint8_t COM_commad_terminations=0;



/*!
 *******************************************************************************
 *  \brief transmit bytes
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
			COM_commad_terminations++;
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
		putchar(i/100+'\0');
		i%=100;
	}
	putchar(i/10+'0');
	putchar(i%10+'0');
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
		putchar(x+'a'-10);	
	} else {
		putchar(x+'0');
	}	
	x = i & 0xf;
	if (x>=10) {
		putchar(x+'a'-10);	
	} else {
		putchar(x+'0');
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
      putchar(c);
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
	stdout = &COM_stdout;
	print_version();
	RS_Init(COM_BAUD_RATE);
	COM_flush();
}


/*!
 *******************************************************************************
 *  \brief Print debug line
 *
 *  \note
 ******************************************************************************/
void COM_print_debug(uint8_t logtype) {
	print_s_p(PSTR("D: "));
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
	print_s_p(PSTR(" V: "));
	print_decXX(valve_wanted); // or MOTOR_GetPosPercent()
	print_s_p(PSTR(" I: "));
	print_decXXXX(temp_average);
	print_s_p(PSTR(" S: "));
	print_decXXXX(calc_temp(temp_wanted));
	if (logtype!=0) {
		print_s_p(PSTR(" X"));
	}
	putchar('\n');
	COM_flush();
}


/*!
 *******************************************************************************
 *  \brief parse hex number (helper function)
 *
 *	\note hex numbers use ONLY lowcase chars, upcase is reserved for commands
 *  \note 2 digits only
 *  \returns 0x00-0xff for correct input, -(char) for wrong char
 *	
 ******************************************************************************/
static int16_t COM_hex_parse (void) {
	uint8_t hex;
	char c=COM_getchar();
	if ( c>='0' && c<='9') {
		hex= (c-'0')<<4; 
	} else if ( c>='a' && c<='f') {
		hex= (c-('a'-10))<<4; 
	} else return -(int16_t)c; //error
	c=COM_getchar();
	if ( c>='0' && c<='9') {
		hex+= (c-'0'); 
	} else if ( c>='a' && c<='f') {
		hex+= (c-('a'-10)); 
	} else return -(int16_t)c; //error
	return (int16_t)hex;
}


/*!
 *******************************************************************************
 *  \brief parse command
 *
 *  \note command have FIXED format
 *  \note command X.....\n    - X is upcase char as commad name, \n is termination char
 *	\note hex numbers use ONLY lowcase chars, upcase is reserved for commands
 *	\note 		  V\n - print version information
 *	\note		  D\n - print status line 
 *	\note		  Taa\n - print watched wariable aa (return 2 or 4 hex numbers) see to \ref watch.c
 *	\note         Gaa\n - get configuration byte wit hex address aa see to \ref eeprom.h
 *	\note		  Saadd\n - set configuration byte aa to value dd (hex)
 *	\note		  \todo: R1324\n - reboot, 1324 is password (fixed at this moment)
 *	
 ******************************************************************************/
void COM_commad_parse (void) {
	char c='\0';
	while (COM_commad_terminations>0) {
		if (c=='\0') c=COM_getchar();
		switch(c) {
		case 'V':
			c=COM_getchar();
			if (c=='\n') print_version();
			break;
		case 'D':
			c=COM_getchar();
			if (c=='\n') COM_print_debug(1);
			break;
		case 'T':
			{
				uint16_t val;
				int16_t aa = COM_hex_parse();
				if (aa<0) { c = (char)-aa; break; }
				print_s_p(PSTR("T: "));
				print_hexXX(aa);
				putchar(':');
				val=watch(aa);
				print_hexXX((uint8_t)(val>>8));
				print_hexXX((uint8_t)(val&0xff));
				putchar('\n');
				c='\0';
			}
			break;
		case 'G':
			{
				int16_t aa = COM_hex_parse();
				if (aa<0) { c = (char)-aa; break; }
				print_s_p(PSTR("G: "));
				print_hexXX(aa);
				putchar(':');
				print_hexXX(config_raw[aa]);
				putchar('\n');
				c='\0';
			}
			break;
		case 'S':
			{
				int16_t dd;
				int16_t aa = COM_hex_parse();
				if (aa<0) { c = (char)-aa; break; }
				dd = COM_hex_parse();
				if (dd<0) { c = (char)-dd; break; }
				if (aa<CONFIG_RAW_SIZE) {
					config_raw[aa]=(uint8_t)dd;
					eeprom_config_save(aa);
					print_s_p(PSTR("S: OK\n"));
				} else {
					print_s_p(PSTR("S: KO!\n"));
				}
				c='\0';
			}
			break;
		//case 'R':
			// \todo command 'R'
		//	break;
		case '\n':
			COM_commad_terminations--;
		default:
			c='\0';
			break;
		}
		COM_flush();
	}
}
