/*
 *  Open HR20
 *
 *  target:     ATmega32 @ 10 MHz in Honnywell Rondostat HR20E
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
#include <string.h>
#include <avr/wdt.h>


#include "config.h"
#include "main.h"
#include "com.h"
#include "common/uart.h"
#include "common/rtc.h"
#include "common/wireless.h"
#include "task.h"
#include "eeprom.h"
#include "queue.h"


#if defined(_AVR_IOM32_H_) || defined(__AVR_ATmega328P__)
#define TX_BUFF_SIZE 512
#else
#define TX_BUFF_SIZE 256
#endif
#define RX_BUFF_SIZE 64

static char tx_buff[TX_BUFF_SIZE];
static char rx_buff[RX_BUFF_SIZE];

#if (TX_BUFF_SIZE > 256)
static uint16_t tx_buff_in = 0;
static uint16_t tx_buff_out = 0;
#else
static uint8_t tx_buff_in = 0;
static uint8_t tx_buff_out = 0;
#endif
static uint8_t rx_buff_in = 0;
static uint8_t rx_buff_out = 0;

extern uint8_t onsync;

/*!
 *******************************************************************************
 *  \brief transmit bytes
 *
 *  \note
 ******************************************************************************/
static void COM_putchar(char c)
{
	cli();
	if ((tx_buff_in + 1) % TX_BUFF_SIZE != tx_buff_out)
	{
		tx_buff[tx_buff_in++] = c;
		tx_buff_in %= TX_BUFF_SIZE;
	}
	else
	{
		// mark end on buffer owerflow to recognize this situation
		if (tx_buff_in == 0)
		{
			tx_buff[TX_BUFF_SIZE - 2] = '*';
			tx_buff[TX_BUFF_SIZE - 1] = '\n';
		}
		else if (tx_buff_in == 1)
		{
			tx_buff[TX_BUFF_SIZE - 1] = '*';
			tx_buff[0] = '\n';
		}
		else
		{
			tx_buff[tx_buff_in - 2] = '*';
			tx_buff[tx_buff_in - 1] = '\n';
		}
	}
	sei();
}

/*!
 *******************************************************************************
 *  \brief support for interrupt for transmit bytes
 *
 *  \note
 ******************************************************************************/
char COM_tx_char_isr(void)
{
	wdt_reset();
	char c = '\0';
	if (tx_buff_in != tx_buff_out)
	{
		c = tx_buff[tx_buff_out++];
		tx_buff_out %= TX_BUFF_SIZE;
	}
	return c;
}

static volatile uint8_t COM_requests;
/*!
 *******************************************************************************
 *  \brief support for interrupt for receive bytes
 *
 *  \note
 ******************************************************************************/
void COM_rx_char_isr(char c)
{
	if (c != '\0')                          // ascii based protocol, \0 char is not alloweed, ignore it
	{
		if (c == '\r')
		{
			c = '\n';               // mask diffrence between operating systems
		}
		rx_buff[rx_buff_in++] = c;
		rx_buff_in %= RX_BUFF_SIZE;
		if (rx_buff_in == rx_buff_out)   // buffer overloaded, drop oldest char
		{
			rx_buff_out++;
			rx_buff_out %= RX_BUFF_SIZE;
		}
		if (c == '\n')
		{
			task |= TASK_COM;
			COM_requests++;
		}
	}
}

/*!
 *******************************************************************************
 *  \brief receive bytes
 *
 *  \note
 ******************************************************************************/
static char COM_getchar(void)
{
	char c;

	cli();
	if (rx_buff_in != rx_buff_out)
	{
		c = rx_buff[rx_buff_out++];
		rx_buff_out %= RX_BUFF_SIZE;
		if (c == '\n')
		{
			COM_requests--;
		}
	}
	else
	{
		COM_requests = 0;
		c = '\0';
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
static void COM_flush(void)
{
	if (tx_buff_in != tx_buff_out)
	{
#ifdef COM_UART
		UART_startSend();
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
static void print_decXX(uint8_t i)
{
	if (i >= 100)
	{
		COM_putchar(i / 100 + '0');
		i %= 100;
	}
	COM_putchar(i / 10 + '0');
	COM_putchar(i % 10 + '0');
}

/*!
 *******************************************************************************
 *  \brief helper function print 4 digit dec number
 *
 *  \note only unsigned numbers
 ******************************************************************************/
static void print_decXXXX(uint16_t i)
{
	print_decXX(i / 100);
	print_decXX(i % 100);
}

/*!
 *******************************************************************************
 *  \brief helper function print 2 digit dec number
 *
 *  \note only unsigned numbers
 ******************************************************************************/
static void print_hexXX(uint8_t i)
{
	uint8_t x = i >> 4;

	if (x >= 10)
	{
		COM_putchar(x + 'a' - 10);
	}
	else
	{
		COM_putchar(x + '0');
	}
	x = i & 0xf;
	if (x >= 10)
	{
		COM_putchar(x + 'a' - 10);
	}
	else
	{
		COM_putchar(x + '0');
	}
}

/*!
 *******************************************************************************
 *  \brief helper function print 4 digit dec number
 *
 *  \note only unsigned numbers
 ******************************************************************************/
static void print_hexXXXX(uint16_t i)
{
	print_hexXX(i >> 8);
	print_hexXX(i & 0xff);
}


/*!
 *******************************************************************************
 *  \brief helper function print string without \n2 digit dec number
 *
 *  \note only unsigned numbers
 ******************************************************************************/
void print_s_p(const char *s)
{
	char c;

	for (c = pgm_read_byte(s); c; ++s, c = pgm_read_byte(s))
	{
		COM_putchar(c);
	}
}

/*!
 *******************************************************************************
 *  \brief helper function print version string
 *
 *  \note
 ******************************************************************************/
static void print_version(void)
{
	print_s_p(PSTR(VERSION_STRING "\n"));
}


/*!
 *******************************************************************************
 *  \brief init communication
 *
 *  \note
 ******************************************************************************/
void COM_init(void)
{
	print_version();
	UART_init();
	COM_flush();
}

/*!
 *******************************************************************************
 *  \brief Print debug line
 *
 *  \note
 ******************************************************************************/
void COM_print_debug(int8_t valve)
{
	print_s_p(PSTR("D: "));
	print_hexXX(RTC_GetDayOfWeek() + 0xd0);
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
	COM_putchar('.');
	print_decXX(RTC_GetS100());
	COM_putchar('\n');
	COM_flush();
}



/*!
 *  \note dirty trick with shared array for \ref COM_hex_parse and \ref COM_commad_parse
 *  code size optimalization
 */
static uint8_t com_hex[4];

/*!
 *******************************************************************************
 *  \brief parse hex number (helper function)
 *
 *	\note hex numbers use ONLY lowcase chars, upcase is reserved for commands
 *
 ******************************************************************************/
static char COM_hex_parse(uint8_t n, bool n_test)
{
	uint8_t i;

	for (i = 0; i < n; i++)
	{
		uint8_t c = COM_getchar() - '0';
		if (c > 9)   // chars < '0' overload var c
		{
			if ((c >= ('a' - '0')) && (c <= ('f' - '0')))
			{
				c -= (('a' - '0') - 10);
			}
			else
			{
				return c + '0';
			}
		}
		if (i & 1)
		{
			com_hex[i >> 1] += c;
		}
		else
		{
			com_hex[i >> 1] = (uint8_t)c << 4;
		}
	}
	if (!n_test)
	{
		return '\0';
	}
	{
		char c;
		if ((c = COM_getchar()) != '\n')
		{
			return c;
		}
	}
	return '\0';
}

/*!
 *******************************************************************************
 *  \brief print X[xx]=
 *
 ******************************************************************************/
static void print_idx(char t)
{
	COM_putchar(t);
	COM_putchar('[');
	print_hexXX(com_hex[0]);
	COM_putchar(']');
	COM_putchar('=');
}

/*!
 *******************************************************************************
 *  \brief print incomplete packet mark
 *
 ******************************************************************************/
static void print_incomplete_mark(int8_t len)
{
	COM_putchar('!');
	print_hexXX(-len);
	COM_putchar('!');
	//COM_putchar('=');
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
 *  \note   Yyymmdd\n - set, year yy, month mm, day dd; HEX values!!!
 *  \note   HhhmmSSss\n - set, hour hh, minute mm, second SS, 1/100 second ss; HEX values!!!
 *
 ******************************************************************************/
void COM_commad_parse(void)
{
	char c;

	while (COM_requests)
	{
		switch (c = COM_getchar())
		{
		case 'V':
			if (COM_getchar() == '\n')
			{
				print_version();
			}
			c = '\0';
			break;
		case 'D':
			if (COM_getchar() == '\n')
			{
				COM_print_debug(-1);
			}
			c = '\0';
			break;
		case 'Y':
			if (COM_hex_parse(3 * 2, true) != '\0')
			{
				break;
			}
			RTC_SetDate(com_hex[2], com_hex[1], com_hex[0]);
			print_s_p(PSTR("OK"));
			break;
		case 'H':
			if (COM_hex_parse(4 * 2, true) != '\0')
			{
				break;
			}
			cli();
			RTC_SetHour(com_hex[0]);
			RTC_SetMinute(com_hex[1]);
			RTC_SetSecond(com_hex[2]);
			RTC_SetSecond100(com_hex[3]);
			onsync = 255;
			sei();
			print_s_p(PSTR("OK"));
			break;
#if (RFM == 1)
		case 'O':
			if (COM_hex_parse(2 * 2, true) != '\0')
			{
				break;
			}
			wl_force_addr1 = com_hex[0];
			wl_force_addr2 = com_hex[1];
			print_s_p(PSTR("OK"));
			break;
		case 'P':
			if (COM_hex_parse(4 * 2, true) != '\0')
			{
				break;
			}
			memcpy(&wl_force_flags, com_hex, 4);
			wl_force_addr1 = 0xff;
			print_s_p(PSTR("OK"));
			break;
#endif
		case ':': // intel hex for writing eeprom
			if (COM_hex_parse(4 * 2, false) != '\0')
			{
				break;
			}
			uint8_t byteCount = com_hex[0];
			uint16_t address = (com_hex[1] << 8) | com_hex[2];
			uint8_t recordType = com_hex[3];
			if (recordType != 0)
			{
				COM_hex_parse(2 * 1, true);     // soak up the checksum and newline
				break;                          // not a data record - we'll guess it is an end record
			}
			uint8_t len = 0;
			while (len != byteCount)
			{
				if (COM_hex_parse(2 * 1, false) != '\0')
				{
					break;
				}
				EEPROM_write(address + len, com_hex[0]);
				len++;
			}
			com_hex[0] = address & 0xff;
			print_idx(c);
			COM_hex_parse(2 * 1, true); // soak up the checksum and newline
			byteCount = 0;
			while (len != byteCount)
			{
				print_hexXX(EEPROM_read(address + byteCount));
				byteCount++;
			}
			break;
		case 'G':
		case 'S':
			if (c == 'G')
			{
				if (COM_hex_parse(1 * 2, true) != '\0')
				{
					break;
				}
			}
			else
			{
				if (COM_hex_parse(2 * 2, true) != '\0')
				{
					break;
				}
				if (com_hex[0] < CONFIG_RAW_SIZE)
				{
					config_raw[com_hex[0]] = (uint8_t)(com_hex[1]);
					eeprom_config_save(com_hex[0]);
				}
			}
			print_idx(c);
			if (com_hex[0] == 0xff)
			{
				print_hexXX(EE_LAYOUT);
			}
			else
			{
				print_hexXX(config_raw[com_hex[0]]);
			}
			break;
		case '(':
		{
			if (COM_hex_parse(1 * 2, false) != '\0')
			{
				break;
			}
			uint8_t addr = com_hex[0];
			if (COM_getchar() != '-')
			{
				break;
			}
			if (COM_hex_parse(1, false) != '\0')
			{
				break;
			}
			uint8_t bank = com_hex[0] >> 4;
			if (COM_getchar() != ')')
			{
				break;
			}
			uint8_t ch = COM_getchar();
			uint8_t len = 0;
			switch (ch)
			{
			case 'D':
			case 'V':
				len = 0;
				break;
			case 'M':
			case 'A':
			case 'L':
			case 'T':
			case 'G':
			case 'R':
				len = 1;
				break;
			case 'S':
			case 'B':
				len = 2;
				break;
			case 'W':
				len = 3;
				break;
			default:
				break;
			}
			if (COM_hex_parse(len * 2, true) != '\0')
			{
				break;
			}
			uint8_t *d = Q_push(len + 1, addr, bank);
			if (d == NULL)
			{
				break;
			}
			d[0] = ch;
			memcpy(d + 1, com_hex, len);
			print_s_p(PSTR("OK"));
		}
		break;
		case 'B':
		{
			if (COM_hex_parse(2 * 2, true) != '\0')
			{
				break;
			}
			if ((com_hex[0] == 0x13) && (com_hex[1] == 0x24))
			{
				cli();
				wdt_enable(WDTO_15MS);  //wd on,15ms
				while (1)
				{
					;               //loop till reset
				}
			}
		}
		break;
		default:
			c = '\0';
			break;
		}
		if (c != '\0')
		{
			COM_putchar('\n');
		}
		COM_flush();
	}
}

#define calc_temp(t) (((uint16_t)t) * 50)   // result unit is 1/100 C

/*!
 *******************************************************************************
 *  \brief dump data from *d length len
 *
 *  \note
 ******************************************************************************/
static uint16_t seq = 0;
void COM_dump_packet(uint8_t *d, int8_t len, bool mac_ok)
{
	uint8_t addr = d[1];

	COM_putchar('@');
	print_decXX(RTC_GetSecond());
	COM_putchar('.');
	print_decXX(RTC_s100);
	if (mac_ok && (len >= (2 + 4)))
	{
		print_s_p(PSTR(" PKT"));
		print_hexXXXX(seq++);
#if (RFM_TUNING > 0)
		COM_putchar(' ');
		COM_putchar('A');
		COM_putchar('F');
		COM_putchar('C');
		// manipulate afc to be in a form suitable to store in the openhr20
		// eeprom. AFC 0x10 = sign bit. 0xf = value
		if (afc > 0xf)
		{
			afc |= 0xf0;
		}
		print_hexXX(0 - afc);
#endif
		len -= 6; // mac is correct and not needed
		d += 2;
		COM_putchar('\n');
	}
	else
	{
		print_s_p(PSTR(" ERR"));
		print_hexXXXX(seq++);
		bool dots = false;
		if (len > 10)
		{
			len = 10; // debug output limitation
			dots = true;
		}
		while ((len--) > 0)
		{
			COM_putchar(' ');
			print_hexXX(*(d++));
		}
		if (dots)
		{
			print_s_p(PSTR("..."));
		}
		COM_putchar('\n');
		COM_flush();
		return;
	}
	if (len == 0)
	{
		COM_flush();
		return;
	}
	else
	{
		COM_putchar('(');
		print_hexXX(addr);
		COM_putchar(')');
		print_s_p(PSTR("{\n"));
	}

	while (len > 0)
	{
		if (d[0] & 0x80)
		{
			COM_putchar('*');
		}
		else
		{
			COM_putchar('-');
		}
		d[0] &= 0x7f;
		switch (d[0])
		{
		case 'V':
			while (1)
			{
				if ((--len) < 0)
				{
					print_incomplete_mark(len);
					break;
				}
				if (*d == '\n')
				{
					d++;
					break;
				}
				COM_putchar((*d++) & 0x7f);
			}
			break;
		case 'D':
		case 'A':
		case 'M':
			COM_putchar(d[0]);
			len -= 10;
			if (len < 0)
			{
				print_incomplete_mark(len);
				break;
			}
			print_s_p(PSTR(" m"));
			print_decXX(d[1] & 0x3f);
			print_s_p(PSTR(" s"));
			print_decXX(d[2] & 0x3f);
			COM_putchar(' ');
			COM_putchar(((d[1] & 0x80) != 0) ? ((d[1] & 0x40) ? 'A' : '-') : 'M');
			print_s_p(PSTR(" V"));
			print_decXX(d[9]);
			print_s_p(PSTR(" I"));
			print_decXXXX(((uint16_t)d[4] << 8) | d[5]);
			print_s_p(PSTR(" S"));
			print_decXXXX(calc_temp(d[8]));
			print_s_p(PSTR(" B"));
			print_decXXXX(((uint16_t)d[6] << 8) | d[7]);
			print_s_p(PSTR(" E"));
			print_hexXX(d[3]);
			if ((d[2] & 0x40) != 0)
			{
				print_s_p(PSTR(" W"));
			}
			if ((d[2] & 0x80) != 0)
			{
				print_s_p(PSTR(" L"));
			}
			d += 10;
			break;
		case 'T':
		case 'R':
		case 'W':
			COM_putchar(d[0]);
			len -= 4;
			if (len < 0)
			{
				print_incomplete_mark(len);
				break;
			}
			COM_putchar('[');
			print_hexXX(d[1]);
			COM_putchar(']');
			COM_putchar('=');
			print_hexXX(d[2]);
			print_hexXX(d[3]);
			d += 4;
			break;
		case 'G':
		case 'S':
			COM_putchar(d[0]);
			len -= 3;
			if (len < 0)
			{
				print_incomplete_mark(len);
				break;
			}
			COM_putchar('[');
			print_hexXX(d[1]);
			COM_putchar(']');
			COM_putchar('=');
			print_hexXX(d[2]);
			d += 3;
			break;
		case 'L':
			COM_putchar(d[0]);
			len -= 2;
			if (len < 0)
			{
				print_incomplete_mark(len);
				break;
			}
			print_hexXX(d[1]);
			d += 2;
			break;
		default:
			while ((len--) > 0)
			{
				COM_putchar(' ');
				print_hexXX(*(d++));
			}
			break;
		}
		COM_putchar('\n');
	}
	print_s_p(PSTR("}\n"));
	COM_flush();
}

void COM_print_datetime()
{
	print_hexXX(RTC_GetDayOfWeek() + 0xd0);
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

void COM_req_RTC(void)
{
	uint8_t s = RTC_GetSecond();

	if (s == 0)
	{
		print_s_p(PSTR("RTC?\n"));
	}
	if ((s == 29) || (s == 59))
	{
		COM_putchar('N');
		COM_putchar((s == 59) ? '0' : '1');
		COM_putchar('?');
		COM_putchar('\n');
		COM_flush();
#if (RFM == 1)
		wl_force_addr1 = 0;
		wl_force_addr2 = 0;
#endif
		return;
	}
	if (s >= 30)
	{
#if (RFM == 1)
		if (wl_force_addr1 > 0)
		{
			if (wl_force_addr1 == 0xff)
			{
				s -= 29;
				if (((wl_force_flags >> s) & 1) == 0)
				{
					return;
				}
			}
			else
			{
				if (RTC_GetSecond() & 1)
				{
					s = wl_force_addr2;
				}
				else
				{
					s = wl_force_addr1;
				}
			}
		}
		else
#endif
		return;
	}
	else
	{
		s++;
	}
	COM_putchar('(');
	print_hexXX(s);
	COM_putchar(')');
	COM_putchar('?');
	COM_putchar('\n');
	COM_flush();
}
