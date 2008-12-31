/*
 *  Open HR20 - RFM12 master
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
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

#if (RFM == 1)
	#include "rfm_config.h"
	#include "../common/rfm.h"
#endif

#if (SECURITY == 1)
	#include "security.h"
#endif

int main(void);                            // main with main loop
static inline void init(void);                           // init the whole thing

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

    //! Enable interrupts
    sei();
    
  	COM_init();


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
				    RFM_OFF();		// turn everything off
				    RFM_WRITE(0);	// Clear TX-IRQ

					// actually now its time to switch into listening
					continue;
			}
  			else if (rfm_mode == rfmmode_rx)
  			{
  				if (rfm_framepos>=1) {
                    if (rfm_framepos >= rfm_framebuf[0]) {
                        COM_dump_packet(rfm_framebuf, rfm_framepos);
                        
                    }
                }
  			}
  			
  
  			//BIT_SET(PCMSK0, RFM_SDO_PCINT); // re-enable pin change interrupt
  
  			//if BIT_GET(RFM_SDO_PIN, RFM_SDO_BITPOS) {
  				// insurance to protect interrupt lost
  				//BIT_CLR(PCMSK0, RFM_SDO_PCINT);// disable RFM interrupt
  			//	task |= TASK_RFM; // create task for main loop
  				// PORTE &= ~(1<<PE2);
  			//}
      }
		#endif

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
	//cli();
	BIT_SET(MCUCR, JTD); // Write one to the JTD bit in MCUCR
	BIT_SET(MCUCR, JTD); // ... which must be done twice within exactly 4 cycles.
#endif

    //! set Clock to 4 Mhz
    //CLKPR = (1<<CLKPCE);            // prescaler change enable
    //CLKPR = (1<<CLKPS0);            // prescaler = 2 (internal RC runs @ 8MHz)

    //! Disable Analog Comparator (power save)
    ACSR = (1<<ACD);

    PORTB = (0<<PB0)|(1<<PB1)|(1<<PB2)|(1<<PB3)|(0<<PB6);
    DDRB  = (1<<PB4)|(1<<PB5)|(1<<PB7); 

#if (RFM==1)
	RFM_init();
	RFM_OFF();
#endif
    
}
