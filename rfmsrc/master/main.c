/*
 *  Open HR20 - RFM12 master
 *
 *  target:     ATmega16 @ 10 MHz in Honnywell Rondostat HR20E master
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

// HR20 Project includes
#include "config.h"
#include "com.h"
#include "task.h"
#include "../common/rtc.h"

#if (RFM == 1)
	#include "rfm_config.h"
	#include "../common/rfm.h"
#endif

#if (SECURITY == 1)
	#include "security.h"
#endif

int main(void);                            // main with main loop
static inline void init(void);             // init the whole thing

// Check AVR LibC Version >= 1.6.0
#if __AVR_LIBC_VERSION__ < 10600UL
#warning "avr-libc >= version 1.6.0 recommended"
#warning "This code has not been tested with older versions."
#endif

volatile uint8_t task;

/*!
 *******************************************************************************
 * main program
 ******************************************************************************/
int main(void)
{
    //! initalization
    init();
    RFM_SPI_16(RFM_FIFO_IT(8) | RFM_FIFO_FF | RFM_FIFO_DR);
    RFM_SPI_16(RFM_LOW_BATT_DETECT_D_10MHZ);
    RFM_RX_ON();
	rfm_mode = rfmmode_rx;
	MCUCSR |= _BV(ISC2);

   	COM_init();

    //! Enable interrupts
    sei();
    RFM_INT_EN();

	rfm_framebuf[ 5] = 0; // DEBUG !!!!!!!!!


	// We should do the following once here to have valid data from the start

		
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
					rfm_mode    = rfmmode_stop;
					rfm_framesize = 5;
					RFM_INT_DIS();
				    RFM_SPI_16(RFM_FIFO_IT(8) |               RFM_FIFO_DR);
				    RFM_SPI_16(RFM_FIFO_IT(8) | RFM_FIFO_FF | RFM_FIFO_DR);
                    RFM_RX_ON();    //re-enable RX
					rfm_framepos=0;
					rfm_mode = rfmmode_rx;
				    RFM_INT_EN(); // enable RFM interrupt

					// actually now its time to switch into listening
					continue;
			}
  			else if ((rfm_mode == rfmmode_rx) || (rfm_mode == rfmmode_rx_owf))
  			{
  				if (rfm_framepos>=1) {
					LED_on();
                    if ((rfm_framepos >= rfm_framebuf[0]) 
                            || (rfm_framebuf[0] >= RFM_FRAME_MAX)  // reject noise
                            || (rfm_framepos >= RFM_FRAME_MAX)) {
                        COM_dump_packet(rfm_framebuf, rfm_framepos);
                        rfm_framepos=0;
						rfm_mode = rfmmode_rx;
                            RFM_INT_DIS(); // disable RFM interrupt
                        	RFM_SPI_16(RFM_FIFO_IT(8) |               RFM_FIFO_DR);
	                        RFM_SPI_16(RFM_FIFO_IT(8) | RFM_FIFO_FF | RFM_FIFO_DR);
	                        RFM_INT_EN(); // enable RFM interrupt
							LED_off();
                    }
                }
  			}
        }
		#endif
        if (task & TASK_RTC) {
            task&=~TASK_RTC;
            {
                bool minute = RTC_AddOneSecond();
                if (minute || RTC_GetSecond()==30) {
                    rfm_putchar(RTC_GetYearYY());
                    uint8_t d = RTC_GetDay(); 
                    rfm_putchar((RTC_GetMonth()<<4) + (d>>3)); 
                    rfm_putchar((d<<5) + RTC_GetHour());
                    rfm_putchar((RTC_GetMinute()<<1) + ((RTC_GetSecond()==30)?1:0));
                
                    RFM_TX_ON_PRE();
					rfm_framebuf[ 0] = 0xaa; // preamble
					rfm_framebuf[ 1] = 0xaa; // preamble
					rfm_framebuf[ 2] = 0x2d; // rfm fifo start pattern
					rfm_framebuf[ 3] = 0xd4; // rfm fifo start pattern

					rfm_framebuf[ 4] = (rfm_framesize-4+4) | 0xc0; // length (sync)

					rfm_framebuf[rfm_framesize++] = 0xab; // dummy MAC
					rfm_framebuf[rfm_framesize++] = 0xcd; // dummy MAC
					rfm_framebuf[rfm_framesize++] = 0xef; // dummy MAC
					rfm_framebuf[rfm_framesize++] = 0x00; // dummy MAC


					rfm_framebuf[rfm_framesize++] = 0xaa; // dummy byte
					rfm_framebuf[rfm_framesize++] = 0xaa; // dummy byte

					rfm_framepos  = 0;
					rfm_mode    = rfmmode_tx;
					RFM_TX_ON();
    			    RFM_SPI_SELECT; // set nSEL low: from this moment SDO indicate FFIT or RGIT
                    RFM_INT_EN(); // enable RFM interrupt

                    COM_print_datetime();
                }
            }
        }
        // serial communication
		if (task & TASK_COM) {
			task&=~TASK_COM;
			COM_commad_parse();
			continue; // on most case we have only 1 task, iprove time to sleep
		}


    } //End Main loop
	return 0;
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

    PORTB = (0<<PB0)|(1<<PB1)|(1<<PB2)|(1<<PB3)|(0<<PB6);
    DDRB  = (1<<PB4)|(1<<PB5)|(1<<PB7);
	
	DDRD = _BV(PD1) | _BV(PD6);
	PORTD = 0xff;
	PORTA = 0xff;
	PORTB = 0xff; 
	
	RTC_Init();

#if (RFM==1)
	RFM_init();
	RFM_OFF();
#endif
}

/*!
 *******************************************************************************
 * Pinchange Interupt INT2
 *  
 * \note level interrupt is better, but I want to have same code for master as for HR20 (jdobry)
 ******************************************************************************/
#if (RFM==1)
ISR (INT2_vect){
  // RFM module interupt
  while (RFM_SDO_PIN & _BV(RFM_SDO_BITPOS)) {
    GICR &= ~_BV(INT2); // disable RFM interrupt
    sei(); // enable global interrupts
    if (rfm_mode == rfmmode_tx) {
        RFM_WRITE(rfm_framebuf[rfm_framepos++]);
        if (rfm_framepos >= rfm_framesize) {
          rfm_mode = rfmmode_tx_done;
    	  task |= TASK_RFM; // inform the rfm task about end of transmition
    	  return; // \note !!WARNING!!
    	}
    } else if (rfm_mode == rfmmode_rx) {
        rfm_framebuf[rfm_framepos++]=RFM_READ_FIFO();
        if (rfm_framepos >= RFM_FRAME_MAX) rfm_mode = rfmmode_rx_owf;
    	task |= TASK_RFM; // inform the rfm task about next RX byte
    } else if (rfm_mode == rfmmode_rx_owf) {
    	task |= TASK_RFM; // inform the rfm task about next RX byte
	}
    cli(); // disable global interrupts
    asm volatile("nop"); // we must have one instruction after cli() 
    GICR |= _BV(INT2); // enable RFM interrupt
    asm volatile("nop"); // we must have one instruction after
  }
  // do NOT add anything after RFM part
}
#endif
