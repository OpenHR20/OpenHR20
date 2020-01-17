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


extern uint8_t Keys[5 * 8]; // 40 bytes
#define K_mac (Keys + 0 * 8)
#define K_enc (Keys + 1 * 8)
#define K1 (Keys + 3 * 8)
#define K2 (Keys + 4 * 8)
#define K_m (Keys + 3 * 8)  /* share same position as K1 & K2 */
// note: do not change order of keys in Keys array, it depend to crypto_init

void crypto_init(void);
void wirelessReceivePacket(void);
#if defined(MASTER_CONFIG_H)
void wirelessSendSync(void);
extern uint8_t wl_packet_bank;
void wirelessTimer2(void);
#else
extern bool wireless_async;
void wirelesTimeSyncCheck(void);
#endif
void wirelessSendDone(void);
void wirelessTimer(void);

#if (RFM == 1)
void wireless_putchar(uint8_t ch);
#else
#define wireless_putchar(ch)
#endif


extern int8_t time_sync_tmo;
extern uint8_t wireless_buf_ptr;
extern uint8_t wl_force_addr1;
extern uint8_t wl_force_addr2;
extern uint32_t wl_force_flags;

#if !defined(MASTER_CONFIG_H)
#define WLTIME_SYNC (0xfa)                              // prepare to receive timesync / slave only
#define WLTIME_START (RTC_TIMER_CALC(200))              // communication start
#define WLTIME_TIMEOUT (RTC_TIMER_CALC(80))             // slave RX timeout
#define WLTIME_SYNC_TIMEOUT (RTC_TIMER_CALC(80))        // slave RX timeout
#define WLTIME_STOP (RTC_TIMER_CALC(700))               // last possible communication
#endif
#define WLTIME_LED_TIMEOUT (RTC_TIMER_CALC(300))        // packet blink time

/* this allow to ignore defined sync packets
 * it is allowed only if last received sync not contain any communication request
 */
#define WL_SKIP_SYNC 3
extern uint8_t wl_skip_sync;

#if !defined(MASTER_CONFIG_H)
typedef enum
{
	WL_TIMER_NONE,
	WL_TIMER_FIRST,
	WL_TIMER_RX_TMO,
	WL_TIMER_SYNC // slave only
} wirelessTimerCase_t;
extern wirelessTimerCase_t wirelessTimerCase;
#endif
