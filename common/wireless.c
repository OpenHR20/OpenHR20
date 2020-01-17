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
 * \file       wireless.c
 * \brief      wireless layer
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */


#include "config.h"
#include <avr/pgmspace.h>
#include <string.h>
#include "xtea.h"
#include "eeprom.h"
#include "wireless.h"
#include "cmac.h"
#include "com.h"
#include "debug.h"
#if defined(MASTER_CONFIG_H)
#include "queue.h"
#else
#include "controller.h"
#include "task.h"
#endif

#if RFM

uint8_t Keys[5 * 8]; // 40 bytes

#define WIRELESS_BUF_MAX (RFM_FRAME_MAX - (4 + 2 + 4))

static uint8_t wireless_framebuf[WIRELESS_BUF_MAX];
/* buffer structure:
 * 4 bytes preamble
 * 2 bytes header
 * x bytes data
 * 4 bytes signature
 * not realy in buffer, 2 bytes virtual dummy
 */

uint8_t wireless_buf_ptr = 0;

static const uint8_t Km_upper[8] PROGMEM = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef
};

static const uint8_t wl_header[4] PROGMEM = {
	0xaa, 0xaa, 0x2d, 0xd4
};

#if defined(MASTER_CONFIG_H)
static void wirelessSendPacket(void);
#else
static void wirelessSendPacket(bool cpy);
#endif


/*!
 *******************************************************************************
 *  init crypto keys
 ******************************************************************************/
void crypto_init(void)
{
	uint8_t i;

	memcpy(K_m, config.security_key, 8);
	memcpy_P(K_m + 8, Km_upper, sizeof(Km_upper));
	for (i = 0; i < 3 * 8; i++)
	{
		Keys[i] = 0xc0 + i;
	}
	xtea_enc(K_mac, K_mac, K_m);            /* generate K_mac low 8 bytes */
	xtea_enc(K_enc, K_enc, K_m);            /* generate K_mac high 8 bytes  and K_enc low 8 bytes*/
	xtea_enc(K_enc + 8, K_enc + 8, K_m);    /* generate K_enc high 8 bytes */
	for (i = 0; i < 8; i++)                 // smaller&faster than memset
	{
		K1[i] = 0;
	}
	xtea_enc(K1, K1, K_mac);
	asm (
		"   movw  R30,%A0   \n"
		"   rcall left_roll \n" /* generate K1 */
		"   ldi r30,lo8(" STR(K2) ") \n"
		"   ldi r31,hi8(" STR(K2) ") \n"
		"   rcall left_roll \n" /* generate K2 */
		:: "y" (K1)
		: "r26", "r27", "r30", "r31"
	);
#if defined(MASTER_CONFIG_H)
	LED_RX_off();
	LED_sync_off();
#endif
}
/* internal function for crypto_init */
/* use loop inside - short/slow */
asm (
	"left_roll:               \n"
	"   ldd r26,Y+7           \n"
	"   lsl r26               \n"
	"   in r27,__SREG__       \n"   // save carry
	"   ldi r26,7             \n"   // 8 times
	"roll_loop:               \n"
	"   ld __tmp_reg__,Y      \n"
	"   out __SREG__,r27      \n"   // restore carry
	"   rol __tmp_reg__       \n"
	"   in r27,__SREG__       \n"   // save carry
	"   st Z,__tmp_reg__      \n"
	"   adiw r28,1            \n"   // Y++
	"   adiw r30,1            \n"   // Z++
	"   subi r26,1            \n"
	"   brcc roll_loop        \n"   // 8 times loop
	"   sbiw r28,8            \n"   // Y-=8
	"   ret "
);

/*!
 *******************************************************************************
 *  encrypt / decrypt
 *  \note symetric operation
 ******************************************************************************/
static void encrypt_decrypt(uint8_t *p, uint8_t len)
{
	uint8_t i = 0;
	uint8_t buf[8];

	while (i < len)
	{
		xtea_enc(buf, &RTC, K_enc);
		RTC.pkt_cnt++;
		do
		{
			p[i] ^= buf[i & 7];
			i++;
			if (i >= len)
			{
				return;       //done
			}
		} while ((i & 7) != 0);
	}
}

/*!
 *******************************************************************************
 *  wireless send Done
 ******************************************************************************/
void wirelessSendDone(void)
{
	RFM_INT_DIS();
	rfm_mode = rfmmode_stop;
#if defined(MASTER_CONFIG_H)
	wireless_buf_ptr = 0;
#endif
	rfm_framepos = 0;

	RFM_FIFO_OFF();
	RFM_FIFO_ON();
	RFM_RX_ON();    //re-enable RX
	rfm_mode = rfmmode_rx;
	RFM_INT_EN();   // enable RFM interrupt

#if !defined(MASTER_CONFIG_H)
	wirelessTimerCase = WL_TIMER_RX_TMO;
	while (ASSR & (_BV(TCR2UB)))
	{
		;
	}
	RTC_timer_set(RTC_TIMER_RFM, (uint8_t)(RTC_s256 + WLTIME_TIMEOUT));
	COM_print_time('r');
#endif
}

/*!
 *******************************************************************************
 *  wireless Timer
 ******************************************************************************/
void wirelessTimer(void)
{
#if !defined(MASTER_CONFIG_H)
	COM_print_time('t');
	switch (wirelessTimerCase)
	{
	case WL_TIMER_FIRST:
		wirelessSendPacket(true);
		break;
	case WL_TIMER_SYNC:
		RFM_INT_DIS();
		wl_force_addr1 = 0;
		wl_force_addr2 = 0;
		RFM_FIFO_OFF();
		RFM_FIFO_ON();
		RFM_RX_ON();
		RFM_SPI_SELECT; // set nSEL low: from this moment SDO indicate FFIT or RGIT
		RFM_INT_EN();   // enable RFM interrupt
		rfm_framepos = 0;
		rfm_mode = rfmmode_rx;
		wirelessTimerCase = WL_TIMER_RX_TMO;
		while (ASSR & (_BV(TCR2UB)))
		{
			;
		}
		RTC_timer_set(RTC_TIMER_RFM, (uint8_t)(RTC_s256 + WLTIME_SYNC_TIMEOUT));
		return;
	case WL_TIMER_RX_TMO:
		if (rfm_mode != rfmmode_tx)
		{
			RFM_INT_DIS();
			rfm_mode = rfmmode_stop;
			RFM_OFF();
		}
		break;
	default:
		break;
	}
	wirelessTimerCase = WL_TIMER_NONE;
}
#else
	LED_RX_off();
}

void wirelessTimer2(void)
{
	LED_sync_off();
}
#endif

#if !defined(MASTER_CONFIG_H)
wirelessTimerCase_t wirelessTimerCase = WL_TIMER_NONE;
#endif

/*!
 *******************************************************************************
 *  wireless send data packet
 ******************************************************************************/

#if defined(MASTER_CONFIG_H)
static void wirelessSendPacket(void)
{
#else
static void wirelessSendPacket(bool cpy)
{
	COM_print_time('S');
#endif
	RFM_INT_DIS();
	RFM_TX_ON_PRE();

	memcpy_P(rfm_framebuf, wl_header, 4);

#if defined(MASTER_CONFIG_H)
	rfm_framebuf[5] = 0;
#else
	rfm_framebuf[5] = config.RFM_devaddr;
	if (cpy)
#endif
	{
		rfm_framesize = wireless_buf_ptr + 2 + 4;
		memcpy(rfm_framebuf + 6, wireless_framebuf, wireless_buf_ptr);
	}

	rfm_framebuf[4] = rfm_framesize;     // length

	encrypt_decrypt(rfm_framebuf + 6, rfm_framesize - 4 - 2);
	cmac_calc(rfm_framebuf + 5, rfm_framesize - 5, (uint8_t *)&RTC, false);
	RTC.pkt_cnt++;
	rfm_framesize += 4 + 2; //4 MAC + 2 dummy
	// rfm_framebuf[rfm_framesize++] = 0xaa; // dummy byte is not significant
	// rfm_framebuf[rfm_framesize++] = 0xaa; // dummy byte is not significant

	rfm_framepos = 0;
	rfm_mode = rfmmode_tx;
	RFM_TX_ON();
	RFM_SPI_SELECT; // set nSEL low: from this moment SDO indicate FFIT or RGIT
	RFM_INT_EN();   // enable RFM interrupt
#if !defined(MASTER_CONFIG_H)
	COM_print_time('s');
#endif
}

/*!
 *******************************************************************************
 *  wireless send SYNC packet
 ******************************************************************************/
uint8_t wl_force_addr1;
uint8_t wl_force_addr2;
uint32_t wl_force_flags;

#if defined(MASTER_CONFIG_H)
void wirelessSendSync(void)
{
	LED_sync_on();
	RTC_timer_set(RTC_TIMER_RFM2, (uint8_t)(RTC_s100 + WLTIME_LED_TIMEOUT));
	RFM_INT_DIS();
	RFM_TX_ON_PRE();
	memcpy_P(rfm_framebuf, wl_header, 4);

	rfm_framebuf[4] = (wireless_buf_ptr + 1 + 4) | 0x80; // length (sync)

	memcpy(rfm_framebuf + 5, wireless_framebuf, wireless_buf_ptr);
	cmac_calc(rfm_framebuf + 5, wireless_buf_ptr, NULL, false);

	rfm_framesize = wireless_buf_ptr + 4 + 1 + 4 + 2; // 4 preamble 1 length 4 signature 2 dummy

	// rfm_framebuf[rfm_framesize++] = 0xaa; // dummy byte is not significant
	// rfm_framebuf[rfm_framesize++] = 0xaa; // dummy byte is not significant

	rfm_framepos = 0;
	rfm_mode = rfmmode_tx;
	RFM_TX_ON();
	RFM_SPI_SELECT; // set nSEL low: from this moment SDO indicate FFIT or RGIT
	RFM_INT_EN();   // enable RFM interrupt
}

uint8_t wl_packet_bank = 0;
#else
int8_t time_sync_tmo = 0;
#if (WL_SKIP_SYNC)
uint8_t wl_skip_sync = 0;
#endif
#endif

#if DEBUG_PRINT_ADDITIONAL_TIMESTAMPS
static bool debug_R_send = false;
#endif
/*!
 *******************************************************************************
 *  wireless receive data packet
 ******************************************************************************/
void wirelessReceivePacket(void)
{
#if DEBUG_PRINT_ADDITIONAL_TIMESTAMPS
	if (!debug_R_send)
	{
		COM_print_time('R');
		debug_R_send = true;
	}
#endif
	if (rfm_framepos >= 1)
	{
		if (((rfm_framebuf[0] & 0x7f) >= RFM_FRAME_MAX) // reject noise
		    || ((rfm_framebuf[0] & 0x7f) < 4 + 2)
		    || (rfm_framepos >= RFM_FRAME_MAX))
		{
			// reject invalid data
#if DEBUG_PRINT_ADDITIONAL_TIMESTAMPS
			debug_R_send = false;
			COM_putchar('\n');
			COM_flush();
#endif
			rfm_framepos = 0;
			return; // !!! return !!!
		}

		if (rfm_framepos >= (rfm_framebuf[0] & 0x7f))
		{
#if DEBUG_PRINT_ADDITIONAL_TIMESTAMPS
			debug_R_send = false;
#endif

			RFM_INT_DIS(); // disable RFM interrupt
			if (rfm_framepos > (rfm_framebuf[0] & 0x7f))
			{
				rfm_framepos = (rfm_framebuf[0] & 0x7f);
			}

			{
				bool mac_ok;
#if !defined(MASTER_CONFIG_H)
				if ((rfm_framebuf[0] & 0x80) == 0x80)
				{
					//sync packet
					mac_ok = cmac_calc(rfm_framebuf + 1, (rfm_framebuf[0] & 0x7f) - 5, NULL, true);
					COM_dump_packet(rfm_framebuf, rfm_framepos, mac_ok);
					if (mac_ok)
					{
						rfm_mode = rfmmode_stop;
						RFM_OFF();
						RTC_timer_destroy(WL_TIMER_RX_TMO);

						if (rfm_framebuf[0] == 0x8b)
						{
							wl_force_addr1 = rfm_framebuf[5];
							wl_force_addr2 = rfm_framebuf[6];
						}
						else if (rfm_framebuf[0] == 0x8d)
						{
							wl_force_addr1 = 0xff;
							// wl_force_addr2=0xff;
							memcpy(&wl_force_flags, rfm_framebuf + 5, 4);
						}
						else
						{
#if (WL_SKIP_SYNC)
							wl_skip_sync = WL_SKIP_SYNC;
#endif
						}
						time_sync_tmo = 20;
						while (ASSR & (_BV(TCR2UB)))
						{
							;
						}
						/*
						 * Reading of the TCNT2 Register shortly after wake-up from Power-save may give an incorrect
						 * result. Since TCNT2 is clocked on the asynchronous TOSC clock, reading TCNT2 must be
						 * done through a register synchronized to the internal I/O clock domain. Synchronization takes
						 * place for every rising TOSC1 edge.
						 */
						if (RTC_s256 > 0x80)
						{
							// round to upper number compencastion
							RTC_s256 = 4;
							cli(); RTC_timer_done |= _BV(RTC_TIMER_OVF); sei();
							task |= TASK_RTC;
						}
						GTCCR = _BV(PSR2);
						RTC_s256 = 10;
						while (ASSR & (_BV(TCN2UB)))
						{
							;
							// wait for clock sync
							// it can take 2*1/32768 sec = 61us
							// ATmega169 datasheet chapter 17.8.1
						}
						CTL_clear_error(CTL_ERR_RFM_SYNC);
						RTC_SetYear(rfm_framebuf[1]);
						RTC_SetMonth(rfm_framebuf[2] >> 4);
						RTC_SetDay((rfm_framebuf[3] >> 5) + ((rfm_framebuf[2] << 3) & 0x18));
						RTC_SetHour(rfm_framebuf[3] & 0x1f);
						RTC_SetMinute(rfm_framebuf[4] >> 1);
						RTC_SetSecond((rfm_framebuf[4] & 1) ? 30 : 00);
						cli(); RTC_timer_done &= ~_BV(RTC_TIMER_RTC); sei(); // do not add one second
						return;
					}
				}
				else
#endif
				{
					RTC.pkt_cnt += (rfm_framepos + 7 - 2 - 4) / 8;
					mac_ok = cmac_calc(rfm_framebuf + 1, rfm_framepos - 1 - 4, (uint8_t *)&RTC, true);
					RTC.pkt_cnt -= (rfm_framepos + 7 - 2 - 4) / 8;
					encrypt_decrypt(rfm_framebuf + 2, rfm_framepos - 2 - 4);
					RTC.pkt_cnt++;
					COM_dump_packet(rfm_framebuf, rfm_framepos, mac_ok);
#if defined(MASTER_CONFIG_H)
					uint8_t addr = rfm_framebuf[1];
					if (mac_ok)
					{
						LED_RX_on();
						RTC_timer_set(RTC_TIMER_RFM, (uint8_t)(RTC_s100 + WLTIME_LED_TIMEOUT));
						q_item_t *p;
						uint8_t skip = 0;
						uint8_t i = 0;
						while ((p = Q_get(addr, wl_packet_bank, skip++)) != NULL)
						{
							for (i = 0; i < (*p).len; i++)
							{
								wireless_putchar((*p).data[i]);
							}
						}
						wl_packet_bank++;
						wirelessSendPacket();
						return;
					}
#else
					if (mac_ok && (rfm_framebuf[1] == 0))   // Accept commands from master only
					{
						wireless_buf_ptr = 0;
						RTC_timer_destroy(WL_TIMER_RX_TMO);
						if (rfm_framepos == 4 + 2)   // empty packet don't need reply
						{
							rfm_mode = rfmmode_stop;
							RFM_OFF();
							return;
						}
						rfm_framesize = 4 + 2;
						{
							// !! hack
							// move input to top of buffer, begining of buffer will be used for output
							uint8_t i;
							uint8_t j = RFM_FRAME_MAX - 1;
							for (i = rfm_framepos - 5; i >= 2; i--)
							{
								rfm_framebuf[j--] = rfm_framebuf[i];
							}
						}
						COM_wireless_command_parse(rfm_framebuf + RFM_FRAME_MAX + 6 - rfm_framepos, rfm_framepos - 6);
						wirelessSendPacket(false);
						return;
					}
#endif
				}
			}
			rfm_framepos = 0;
			rfm_mode = rfmmode_rx;
			RFM_FIFO_OFF();
			RFM_FIFO_ON();
			RFM_INT_EN(); // enable RFM interrupt
		}
	}
}

/*!
 *******************************************************************************
 *  wireless Check time synchronization
 ******************************************************************************/
#if !defined(MASTER_CONFIG_H)
void wirelesTimeSyncCheck(void)
{
	time_sync_tmo--;
	if (time_sync_tmo <= 0)
	{
		if ((time_sync_tmo == 0) || (time_sync_tmo < -30))
		{
			time_sync_tmo = 0;
			RFM_INT_DIS();
			RFM_FIFO_OFF();
			RFM_FIFO_ON();
			RFM_RX_ON(); //re-enable RX
			rfm_framepos = 0;
			rfm_mode = rfmmode_rx;
			RFM_INT_EN();     // enable RFM interrupt
		}
		else if (time_sync_tmo < -4)
		{
			RFM_INT_DIS();
			RFM_OFF();      // turn everything off
			CTL_set_error(CTL_ERR_RFM_SYNC);
		}
	}
}
#endif

#if !defined(MASTER_CONFIG_H)
bool wireless_async = false;
/*!
 *******************************************************************************
 *  wireless put one byte into buffer
 ******************************************************************************/
void wireless_putchar(uint8_t b)
{
	if (!wireless_async)
	{
		// synchronous buffer
		if (rfm_framesize < RFM_FRAME_MAX - 4 - 2)
		{
			rfm_framebuf[rfm_framesize++] = b;
		}
	}
	else
	{
#else
void wireless_putchar(uint8_t b)
{
	{
#endif
		// asynchronous buffer
		if (wireless_buf_ptr < WIRELESS_BUF_MAX)
		{
			wireless_framebuf[wireless_buf_ptr++] = b;
		}
	}
}

#endif // RFM
