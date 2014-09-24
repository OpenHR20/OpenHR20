/*
 *  Open HR20 - RFM12 master
 *
 *  target:     ATmega32 @ 10 MHz in Honnywell Rondostat HR20E master
 *
 *  compiler:    WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Dario Carluccio (hr20-at-carluccio-dot-de)
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
 * \file       main.c
 * \brief      the main file for Open HR20 project
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>; Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

// AVR LibC includes 
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/version.h>
#include <string.h>
#include <avr/wdt.h>

// HR20 Project includes
#include "config.h"
#include "main.h"
#include "com.h"
#include "task.h"
#include "eeprom.h"
#include "queue.h"
#include "../common/rtc.h"
#include "../common/cmac.h"
#include "../common/wireless.h"

#if (RFM == 1)
	#include "rfm_config.h"
	#include "../common/rfm.h"
#endif

int main(void);                            // main with main loop
static inline void init(void);             // init the whole thing

// Check AVR LibC Version >= 1.6.0
#if __AVR_LIBC_VERSION__ < 10600UL
#warning "avr-libc >= version 1.6.0 recommended"
#warning "This code has not been tested with older versions."
#endif
#if ! TASK_IS_SFR 
#warning task variable manipulation is not protected to interrupt, fix it
#endif


volatile uint8_t task;
uint8_t onsync=0; // \todo comments, header

/*!
 *******************************************************************************
 * main program
 ******************************************************************************/
int __attribute__ ((noreturn)) main(void)
// __attribute__((noreturn)) mean that we not need prologue and epilogue for main()
{
    //! initalization
    init();

   	COM_init();

    //! Enable interrupts
    sei();
#if (RFM==1)
    RFM_INT_EN();

	rfm_framebuf[ 5] = 0; // DEBUG !!!!!!!!!
#endif

    wdt_enable(WDTO_2S);

	// We should do the following once here to have valid data from the start
#if (RFM_TUNING>0)
	if (config.RFM_tuning) {
	  print_s_p(PSTR(" RFM Tuning mode enabled."));
	}
#endif
		
    /*!
    ****************************************************************************
    * main loop
    ***************************************************************************/
    for (;;){        
		// go to sleep with ADC conversion start
		asm volatile ("cli");
		if (! task) {
  			// nothing to do, go to sleep
			   // SMCR = (0<<SM1)|(0<<SM0)|(1<<SE); // Idle mode

			asm volatile ("sei");	//  sequence from ATMEL datasheet chapter 6.8.
			asm volatile ("sleep");
			asm volatile ("nop");
			//SMCR = (1<<SM1)|(1<<SM0)|(0<<SE); // Power-save mode
		} else {
			asm volatile ("sei");
		}

#if (RFM==1)
		  // RFM12
		  if (task & TASK_RFM) {
  			task &= ~TASK_RFM;
  			// PORTE |= (1<<PE2);

			if (rfm_mode == rfmmode_tx_done)
			{
                wirelessSendDone();
			}
  			else if ((rfm_mode == rfmmode_rx) || (rfm_mode == rfmmode_rx_owf))
  			{
  			   wirelessReceivePacket();
  			}
			continue; // on most case we have only 1 task, iprove time to sleep
        }
#endif
        if (task & TASK_RTC) {
            task&=~TASK_RTC;
            {
#if (RFM==1)
                wl_packet_bank=0;
#endif
                RTC_AddOneSecond();
                bool minute=(RTC_GetSecond()==0);
                if (RTC_GetSecond()<30) {
                    Q_clean(RTC_GetSecond());
                } else {
                    wdt_reset();  // spare WDT reset (notmaly it is in send data interrupt)
#if (RFM==1)
                    if (wl_force_addr1!=0) {
                        if (wl_force_addr1==0xff) {
                            Q_clean(RTC_GetSecond()-30);
                        } else {
                            if (RTC_GetSecond()&1) Q_clean(wl_force_addr1);
                            else Q_clean(wl_force_addr2); 
                        }
                    }
#endif
                }
                if ((onsync)&&(minute || RTC_GetSecond()==30)) {
                    onsync--;
#if (RFM==1)
					rfm_mode = rfmmode_stop;
					wireless_buf_ptr = 0;
                    wireless_putchar(RTC_GetYearYY());
                    uint8_t d = RTC_GetDay(); 
                    wireless_putchar((RTC_GetMonth()<<4) + (d>>3)); 
                    wireless_putchar((d<<5) + RTC_GetHour());
                    wireless_putchar((RTC_GetMinute()<<1) + ((RTC_GetSecond()==30)?1:0));
                    if (wl_force_addr1!=0xfe) {
                        if (wl_force_addr1==0xff) {
                            wireless_putchar(((uint8_t*)&wl_force_flags)[0]);
                            wireless_putchar(((uint8_t*)&wl_force_flags)[1]);
                            wireless_putchar(((uint8_t*)&wl_force_flags)[2]);
                            wireless_putchar(((uint8_t*)&wl_force_flags)[3]);
                        } else {
                            wireless_putchar(wl_force_addr1);
                            wireless_putchar(wl_force_addr2);
                        }
                    }
                    wirelessSendSync();
#endif
                    COM_print_datetime();
                }
                COM_req_RTC();
            }
        }
		if (task & TASK_TIMER) {
		    task &= ~TASK_TIMER;
#if (RFM==1)
            if (RTC_timer_done&_BV(RTC_TIMER_RFM))
            {   
                cli(); RTC_timer_done&=~_BV(RTC_TIMER_RFM); sei();
                wirelessTimer();
            }  			
            if (RTC_timer_done&_BV(RTC_TIMER_RFM2))
            {   
                cli(); RTC_timer_done&=~_BV(RTC_TIMER_RFM2); sei();
                wirelessTimer2();
            }  			
#endif
        }
        // serial communication
		if (task & TASK_COM) {
			task&=~TASK_COM;
			COM_commad_parse();
			continue; // on most case we have only 1 task, iprove time to sleep
		}


    } //End Main loop
}



/*!
 *******************************************************************************
 * Initializate all modules
 ******************************************************************************/
static inline void init(void)
{
#if (DISABLE_JTAG == 1)
	{
	   //cli();
	   uint8_t t = MCUCR | _BV(JTD);
	   MCUCR=t;
	   MCUCR=t;
	}
#endif

    //! Disable Analog Comparator (power save)
    ACSR = (1<<ACD);

#if (NANODE == 1)
	DDRB = 0x2f;
	DDRD = _BV(PD1) | _BV(PD2) | _BV(PD5) | _BV(PD6);
	PORTB = 0xff; // 0x7?
#elif (JEENODE == 1)
    // OUT: PB1 (led), PB2 (nSEL), PB3 (MOSI), PB5 (SCK)
    //  IN: PB4 (MISO), PB6,7 (XTal)
    DDRB = _BV(PB1) | _BV(PB2) | _BV(PB3) | _BV(PB5);       // outputs
    PORTB = _BV(PB4);                                       // pull-ups
    // OUT: PD1 (Tx)
    //  IN: PD0 (Rx), PD2 (RFM-IRQ)
    DDRD = _BV(PD1);                                        // outputs
    PORTD = _BV(PD0) | _BV(PD2) | _BV(PD3) | 0xF0;
#else
 #if (ATMEGA32_DEV_BOARD == 1)
	DDRA = _BV(PA2)|_BV(PA1);       //Green LED for Sync
    DDRD = _BV(PD1);
 #else
	DDRA = _BV(PA2);        //Green LED for Sync
    DDRD = _BV(PD1) | _BV(PD7);
 #endif
    DDRB = _BV(PB4)|_BV(PB5)|_BV(PB7);
    PORTA = 0xff;
    PORTB = ~_BV(PB2); 
#endif
    DDRC = 0;
	PORTC = 0xff; 
	PORTD = 0xff;

	RTC_Init();

#if (RFM==1)
	RFM_init();
	RFM_OFF();
#endif

    eeprom_config_init(false);
    
#if (RFM==1)
   	crypto_init();
    RFM_FIFO_ON();
    RFM_RX_ON();
	rfm_mode = rfmmode_rx;
#if (NANODE == 1)
      EICRA |= ((1<<ISC10) | (1<<ISC11)); // rising edge interrupt
      EIMSK |= _BV(INT1);
#elif (JEENODE == 1)
      EICRA |= ((1<<ISC00) | (0<<ISC00)); // low-level interrupt
      EIMSK |= _BV(INT0);
#else
      MCUCSR |= _BV(ISC2);                // rising edge interrupt
#endif

#endif
}


/* see to wdt.h for following function */
//uint8_t mcusr_mirror _attribute_ ((section (".noinit")));

void get_mcusr(void) \
  __attribute__((naked)) \
  __attribute__((section(".init3")));
void get_mcusr(void)
{
//  mcusr_mirror = MCUSR;
  MCUSR = 0;
  wdt_disable();
}

// default fuses for ELF file

FUSES = 
{
    .low = 0xA0,
    .high = 0x91,
};
