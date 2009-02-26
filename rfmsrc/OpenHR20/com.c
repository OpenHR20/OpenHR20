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
#include "main.h"
#include "../common/rtc.h"
#include "adc.h"
#include "task.h"
#include "watch.h"
#include "eeprom.h"
#include "controller.h"
#include "../common/rfm.h"
#include "debug.h"


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

#if (RFM==1)
static void super_COM_putchar(char c) {
    COM_putchar(c);
    rfm_putchar(c);
}
#else
#define super_COM_putchar(c) 
#endif
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

#if (RFM==1)
static void super_print_hexXX(uint8_t i) {
    print_hexXX(i);
    rfm_putchar(i);
}
#else
#define super_print_hexXX(i) print_hexXX(i)
#endif

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

#if (RFM==1)
static void super_print_hexXXXX(uint16_t i) {
	print_hexXX(i>>8);
	rfm_putchar(i>>8);
	print_hexXX(i&0xff);
	rfm_putchar(i&0xff);
}
#else
#define super_print_hexXXXX(i) print_hexXXXX(i)
#endif


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
 *  \brief Print debug line
 *
 *  \note
 ******************************************************************************/
void COM_print_debug(int8_t valve) {
    print_s_p(PSTR("D: "));
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
	COM_putchar(' ');
	COM_putchar((CTL_mode_auto)?'A':'M');
	print_s_p(PSTR(" V: "));
	print_decXX( (valve>=0) ? valve : valve_wanted);
	print_s_p(PSTR(" I: "));
	print_decXXXX(temp_average);
	print_s_p(PSTR(" S: "));
	if (CTL_temp_wanted_last>TEMP_MAX) {
		print_s_p(PSTR("BOOT"));
	} else {
		print_decXXXX(calc_temp(CTL_temp_wanted_last));
	}
	print_s_p(PSTR(" B: "));
	print_decXXXX(bat_average);
#if DEBUG_PRINT_I_SUM
	print_s_p(PSTR(" Is: "));
	print_hexXXXX(sumError);
#endif
    if (CTL_error!=0) {
		print_s_p(PSTR(" E:"));
        print_hexXX(CTL_error);
    }                   
	if (valve<0) {
		print_s_p(PSTR(" X"));
	}
	if (mode_window()) {
		print_s_p(PSTR(" W"));
	}
	COM_putchar('\n');
	COM_flush();
#if (RFM==1)
    rfm_putchar('D');
	rfm_putchar(temp_average >> 8); // current temp
	rfm_putchar(temp_average & 0xff);
	rfm_putchar(bat_average >> 8); // current temp
	rfm_putchar(bat_average & 0xff);
	rfm_putchar(CTL_temp_wanted); // wanted temp
	rfm_putchar((valve>=0) ? valve : valve_wanted); // valve pos
	rfm_start_tx();
#endif
    
}

/*! 
    \note dirty trick with shared array for \ref COM_hex_parse and \ref COM_commad_parse
    code size optimalization
*/
static uint8_t com_hex[3];
 

/*!
 *******************************************************************************
 *  \brief parse hex number (helper function)
 *
 *	\note hex numbers use ONLY lowcase chars, upcase is reserved for commands
 *	
 ******************************************************************************/
static char COM_hex_parse (uint8_t n) {
	uint8_t i;
	for (i=0;i<n;i++) {
    	uint8_t c = COM_getchar()-'0';
    	if ( c>9 ) {  // chars < '0' overload var c
			if ((c>=('a'-'0')) && (c<=('f'-'0'))) {
    			c-= (('a'-'0')-10);
			} else return c+'0';
		}
    	if (i&1) {
    	   com_hex[i>>1]+=c;
        } else {
    	   com_hex[i>>1]=(uint8_t)c<<4;
        }
    }
	{
		char c;
    	if ((c=COM_getchar())!='\n') return c;
	}
	return '\0';
}

/*!
 *******************************************************************************
 *  \brief print X[xx]=
 *
 ******************************************************************************/
static void print_idx(char t) {
    super_COM_putchar(t);
    COM_putchar('[');
    super_print_hexXX(com_hex[0]);
    COM_putchar(']');
    COM_putchar('=');
}



/*!
 *******************************************************************************
 *  \brief parse command
 *
 *  \note commands have FIXED format
 *  \note command X.....\n    - X is upcase char as commad name, \n is termination char
 *  \note hex numbers use ONLY lowcase chars, upcase is reserved for commands
 *  \note   V\n - print version information
 *  \note   D\n - print status line 
 *  \note   Taa\n - print watched variable aa (return 2 or 4 hex numbers) see to \ref watch.c
 *  \note   Gaa\n - get configuration byte with hex address aa see to \ref eeprom.h 0xff address returns EEPROM layout version
 *  \note   Saadd\n - set configuration byte aa to value dd (hex)
 *  \note   Rab\n - get timer for day a slot b, return cddd=(timermode c time ddd) (hex)
 *  \note   Wabcddd\n - set timer  for day a slot b timermode c time ddd (hex)
 *  \note   B1324\n - reboot, 1324 is password (fixed at this moment)
 *  \note   Yyymmdd\n - set, year yy, month mm, day dd; HEX values!!!
 *  \note   Hhhmmss\n - set, hour hh, minute mm, second ss; HEX values!!!
 *  \note   Axx\n - set wanted temperature [unit 0.5C]
 *  \note   Mxx\n - set mode to 0=manu 1=auto
 *	
 ******************************************************************************/
void COM_commad_parse (void) {
	char c;
	while ((c=COM_getchar())!='\0') {
		switch(c) {
		case 'V':
			if (COM_getchar()=='\n') print_version();
			c='\0';
			break;
		case 'D':
			if (COM_getchar()=='\n') COM_print_debug(-1);
			c='\0';
			break;
		case 'T':
			{
				if (COM_hex_parse(1*2)!='\0') { break; }
                print_idx(c);
  				super_print_hexXXXX(watch(com_hex[0]));
			}
			break;
		case 'G':
		case 'S':
			if (c=='G') {
				if (COM_hex_parse(1*2)!='\0') { break; }
			} else {
				if (COM_hex_parse(2*2)!='\0') { break; }
  				if (com_hex[0]<CONFIG_RAW_SIZE) {
  					config_raw[com_hex[0]]=(uint8_t)(com_hex[1]);
  					eeprom_config_save(com_hex[0]);
  				}
			}
            print_idx(c);
			if (com_hex[0]==0xff) {
			     super_print_hexXX(EE_LAYOUT);
            } else {
			     super_print_hexXX(config_raw[com_hex[0]]);
			}
			break;
		case 'R':
		case 'W':
			if (c=='R') {
				if (COM_hex_parse(1*2)!='\0') { break; }
			} else {
				if (COM_hex_parse(3*2)!='\0') { break; }
  				RTC_DowTimerSet(com_hex[0]>>4, com_hex[0]&0xf, (((uint16_t) (com_hex[1])&0xf)<<8)+(uint16_t)(com_hex[2]), (com_hex[1])>>4);
				CTL_update_temp_auto();
			}
            print_idx(c);
			super_print_hexXXXX(eeprom_timers_read_raw(
                timers_get_raw_index((com_hex[0]>>4),(com_hex[0]&0xf))));
			break;
		case 'Y':
			if (COM_hex_parse(3*2)!='\0') { break; }
			RTC_SetDate(com_hex[2],com_hex[1],com_hex[0]);
			COM_print_debug(-1);
			c='\0';
			break;
		case 'H':
			if (COM_hex_parse(3*2)!='\0') { break; }
			RTC_SetHour(com_hex[0]);
			RTC_SetMinute(com_hex[1]);
			RTC_SetSecond(com_hex[2]);
			COM_print_debug(-1);
			c='\0';
			break;
		case 'B':
			{
				if (COM_hex_parse(2*2)!='\0') { break; }
  				if ((com_hex[0]==0x13) && (com_hex[1]==0x24)) {
                      cli();
                      wdt_enable(WDTO_15MS); //wd on,15ms
                      while(1); //loop till reset
    			}
			}
			break;
        case 'M':
            if (COM_hex_parse(1*2)!='\0') { break; }
            CTL_change_mode(com_hex[0]==1);
            // COM_print_debug(-1);
            break;
        case 'A':
            if (COM_hex_parse(1*2)!='\0') { break; }
            if (com_hex[0]<TEMP_MIN-1) { break; }
            if (com_hex[0]>TEMP_MAX+1) { break; }
            CTL_set_temp(com_hex[0]);
            COM_print_debug(-1);
            break;
		//case '\n':
		//case '\0':
		default:
			c='\0';
			break;
		}
		if (c!='\0') COM_putchar('\n');
		COM_flush();
		#if (RFM==1)
		  rfm_start_tx();
		#endif
	}
}

#if DEBUG_PRINT_MOTOR
void COM_debug_print_motor(int8_t dir, uint16_t m, uint8_t pwm) {
    if (dir>0) {
		COM_putchar('+');
	} else if (dir<0) {
		COM_putchar('-');
	}
    COM_putchar(' ');
	print_hexXXXX(m);
    COM_putchar(' ');
	print_hexXX(pwm);

    COM_putchar('\n');
    COM_flush();
}
#endif

#if DEBUG_PRINT_MEASURE
void COM_debug_print_temperature(uint16_t t) {
    print_s_p(PSTR("T: "));
    print_decXXXX(t);
    COM_putchar('\n');
    COM_flush();
}
#endif


#if DEBUG_DUMP_RFM
/*!
 *******************************************************************************
 *  \brief dump data from *d length len
 *
 *  \note
 ******************************************************************************/
static uint16_t seq=0;
void COM_dump_packet(uint8_t *d, uint8_t len) {
	print_decXX(RTC_GetSecond());
	COM_putchar('x');
	print_hexXX(RTC_s256);
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

void COM_mac_ok(void) {
    print_s_p(PSTR("MAC OK!\n"));
	COM_flush();
}
#endif
