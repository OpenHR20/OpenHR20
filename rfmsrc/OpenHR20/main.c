/*
 *  Open HR20
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
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/version.h>

// HR20 Project includes
#include "config.h"
#include "main.h"
#include "adc.h"
#include "lcd.h"
#include "motor.h"
#include "../common/rtc.h"
#include "task.h"
#include "keyboard.h"
#include "eeprom.h"
#include "debug.h"
#include "menu.h"
#include "com.h"
#include "../common/rs232_485.h"
#include "../common/rfm.h"
#include "controller.h"

#if RFM
	#include "rfm_config.h"
	#include "../common/rfm.h"
#endif

// global Vars
//volatile bool    m_automatic_mode;         // auto mode (false: manu mode)

// global Vars for default values: temperatures and speed
uint8_t valve_wanted=0;

// prototypes 
int main(void);                            // main with main loop
static inline void init(void);                           // init the whole thing
void load_defauls(void);                   // load default values
                                           // (later from eeprom using config.c)
void callback_settemp(uint8_t);            // called from RTC to set new reftemp
void setautomode(bool);                    // activate/deactivate automode
uint8_t input_temp(uint8_t);

#if RFM
    int8_t RFM_sync_tmo=0;
#endif

// Check AVR LibC Version >= 1.6.0
#if __AVR_LIBC_VERSION__ < 10600UL
#warning "avr-libc >= version 1.6.0 recommended"
#warning "This code has not been tested with older versions."
#endif

/*!
 *******************************************************************************
 * main program
 ******************************************************************************/
int __attribute__((naked)) main(void)
// __attribute__((naked)) mean that we not need prologue and epilogue for main()
{
    //! initalization
    init();
    
	task=0;

    //! Enable interrupts
    sei();
    
    /* check EEPROM layout */
    if (EEPROM_read((uint16_t)&ee_layout)!=EE_LAYOUT) {
        LCD_PrintStringID(LCD_STRING_EEPr,LCD_MODE_ON);
        task_lcd_update();
        for(;;) {;}  //fatal error, stop startup
    }

	COM_init();
    #if RFM
        // enable persistent RX for initial sync
        RFM_SPI_16(RFM_FIFO_IT(8) | RFM_FIFO_FF | RFM_FIFO_DR);
        RFM_SPI_16(RFM_LOW_BATT_DETECT_D_10MHZ);
        RFM_RX_ON();
		RFM_INT_EN(); // enable RFM interrupt
    	rfm_mode = rfmmode_rx;
    #endif

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
            if(timer0_need_clock() || RS_need_clock()) {
			    SMCR = (0<<SM1)|(0<<SM0)|(1<<SE); // Idle mode
            } else {
    			if (sleep_with_ADC) {
    				SMCR = (0<<SM1)|(1<<SM0)|(1<<SE); // ADC noise reduction mode
				} else {
				    SMCR = (1<<SM1)|(1<<SM0)|(1<<SE); // Power-save mode
                }
            }

			if (sleep_with_ADC) {
				sleep_with_ADC=0;
				// start conversions
		        ADCSRA |= (1<<ADSC);
			}

			DEBUG_BEFORE_SLEEP();
			asm volatile ("sei");	//  sequence from ATMEL datasheet chapter 6.8.
			asm volatile ("sleep");
			asm volatile ("nop");
			DEBUG_AFTER_SLEEP(); 
			SMCR = (1<<SM1)|(1<<SM0)|(0<<SE); // Power-save mode
		} else {
			asm volatile ("sei");
		}

		#if RFM
		  // RFM12
		  if (task & TASK_RFM) {
			task &= ~TASK_RFM;
			// PORTE |= (1<<PE2);

			if (rfm_mode == rfmmode_tx_done)
			{
					rfm_mode    = rfmmode_stop;
					rfm_framesize = 6;
					rfm_framepos=0;
					RFM_INT_DIS();
				    RFM_OFF();		// turn everything off
				    //RFM_WRITE(0);	// Clear TX-IRQ

					// actually now its time to switch into listening
					continue;
			}
  			else if ((rfm_mode == rfmmode_rx) || (rfm_mode == rfmmode_rx_owf))
  			{
  				if (rfm_framepos>=1) {
                    if ((rfm_framepos >= (rfm_framebuf[0]&0x3f)) 
                            || ((rfm_framebuf[0]&0x3f)>= RFM_FRAME_MAX)  // reject noise
                            || (rfm_framepos >= RFM_FRAME_MAX)) {
                        COM_dump_packet(rfm_framebuf, rfm_framepos);
                        rfm_framepos=0;
						rfm_mode = rfmmode_rx;
                        RFM_INT_DIS(); // disable RFM interrupt
                      	RFM_SPI_16(RFM_FIFO_IT(8) |               RFM_FIFO_DR);
                        RFM_SPI_16(RFM_FIFO_IT(8) | RFM_FIFO_FF | RFM_FIFO_DR);
                        RFM_INT_EN(); // enable RFM interrupt
                        if ((rfm_framebuf[0]&0x3f)>5) { 
                          if (cmac_calc(rfm_framebuf+1,(rfm_framebuf[0]&0x3f)-5,NULL,true)) {
                            COM_mac_ok();
                            if (rfm_framebuf[0] == 0xc9) {
                                //sync packet
                    			RTC_s256=10;
                                RFM_sync_tmo=10;
                                rfm_mode = rfmmode_stop;
                                RFM_OFF();
                                RTC_SetYear(rfm_framebuf[1]);
                                RTC_SetMonth(rfm_framebuf[2]>>4);
                                RTC_SetDay((rfm_framebuf[3]>>5)+((rfm_framebuf[2]<<3)&0x18));
                    			RTC_SetHour(rfm_framebuf[3]&0x1f);
                    			RTC_SetMinute(rfm_framebuf[4]>>1);
                    			RTC_SetSecond((rfm_framebuf[4]&1)?30:00);
                            }
                          }
                        }
                    }
                }
  			}
		}
		#endif

        // update LCD task
		if (task & TASK_LCD) {
			task&=~TASK_LCD;
			task_lcd_update();
			continue; // on most case we have only 1 task, iprove time to sleep
		}

		if (task & TASK_ADC) {
			task&=~TASK_ADC;
			if (task_ADC()==0) {
                // ADC is done
#if RFM
				if ((config.RFM_devaddr!=0)
				    && (RFM_sync_tmo>1)
                    && (RTC_GetSecond() == config.RFM_devaddr)) // collission protection: every HR20 shall send when the second counter is equal to it's own address.
				// if ((RFM_sync_tmo>1) && (config.RFM_devaddr!=0) && !(RTC_GetSecond() % 4) ) // for testing all 4 seconds ...
				{
					RFM_TX_ON_PRE();

					rfm_framebuf[ 0] = 0xaa; // preamble
					rfm_framebuf[ 1] = 0xaa; // preamble
					rfm_framebuf[ 2] = 0x2d; // rfm fifo start pattern
					rfm_framebuf[ 3] = 0xd4; // rfm fifo start pattern

					rfm_framebuf[ 4] = rfm_framesize-4+4;    // length
					rfm_framebuf[ 5] = config.RFM_devaddr;

                    encrypt_decrypt (rfm_framebuf+6, rfm_framesize-6);
                    cmac_calc(rfm_framebuf+5,rfm_framesize-5,(uint8_t*)&RTC,false);
                    RTC.pkt_cnt++;
                    rfm_framesize+=4; //MAC
					rfm_framebuf[rfm_framesize++] = 0xaa; // dummy byte
					rfm_framebuf[rfm_framesize++] = 0xaa; // dummy byte

					rfm_framepos  = 0;
					rfm_mode = rfmmode_tx;
					RFM_TX_ON();
    			    RFM_SPI_SELECT; // set nSEL low: from this moment SDO indicate FFIT or RGIT
                    RFM_INT_EN(); // enable RFM interrupt
				}
				if ((config.RFM_devaddr!=0)
				    && (RFM_sync_tmo>1)
                    && ((RTC_GetSecond() == 00) || (RTC_GetSecond() == 30)))
                {
                      	RFM_SPI_16(RFM_FIFO_IT(8) |               RFM_FIFO_DR);
                        RFM_SPI_16(RFM_FIFO_IT(8) | RFM_FIFO_FF | RFM_FIFO_DR);
                        RFM_RX_ON();
	    			    RFM_SPI_SELECT; // set nSEL low: from this moment SDO indicate FFIT or RGIT
                        RFM_INT_EN(); // enable RFM interrupt
                    	rfm_mode = rfmmode_rx;
                }
#endif
            }
			continue; // on most case we have only 1 task, iprove time to sleep
		}
		
        // communication
		if (task & TASK_COM) {
			task&=~TASK_COM;
			COM_commad_parse();
			continue; // on most case we have only 1 task, iprove time to sleep
		}

        // motor stop
        if (task & TASK_MOTOR_STOP) {
            task&=~TASK_MOTOR_STOP;
            MOTOR_timer_stop();
			continue; // on most case we have only 1 task, iprove time to sleep
        }

        // update motor possition
        if (task & TASK_MOTOR_PULSE) {
            task&=~TASK_MOTOR_PULSE;
            MOTOR_updateCalibration(mont_contact_pooling());
            MOTOR_timer_pulse();
			continue; // on most case we have only 1 task, iprove time to sleep
        }


		//! check keyboard and set keyboards events
		if (task & TASK_KB) {
			task&=~TASK_KB;
			task_keyboard();
		}

        if (task & TASK_RTC) {
            task&=~TASK_RTC;
            {
                bool minute = RTC_AddOneSecond();
                valve_wanted = CTL_update(minute,valve_wanted);
                if (minute && (RTC_GetDayOfWeek()==6) && (RTC_GetHour()==10) && (RTC_GetMinute()==0)) {
                    // every sunday 10:00AM
                    // TODO: improve this code!
                    // valve protection / CyCL
                    MOTOR_updateCalibration(0);
                }
                #if RFM
                    if (minute) {
                        RFM_sync_tmo--;
                        if (RFM_sync_tmo<=0) {
                            if (RFM_sync_tmo==0) {
                            		RFM_INT_DIS();
                				    RFM_SPI_16(RFM_FIFO_IT(8) |               RFM_FIFO_DR);
				                    RFM_SPI_16(RFM_FIFO_IT(8) | RFM_FIFO_FF | RFM_FIFO_DR);
                                    RFM_RX_ON();    //re-enable RX
                					rfm_framepos=0;
                					rfm_mode = rfmmode_rx;
                				    RFM_INT_EN(); // enable RFM interrupt
                            } else if (RFM_sync_tmo<-4) {
            					RFM_INT_DIS();
            				    RFM_OFF();		// turn everything off
            				    CTL_error |= CTL_ERR_RFM_SYNC;
                            } 
                        }
                    }
                #endif
            }
            MOTOR_updateCalibration(mont_contact_pooling());
            MOTOR_Goto(valve_wanted);
            task_keyboard_long_press_detect();
            start_task_ADC();
            if (menu_auto_update_timeout>=0) {
                menu_auto_update_timeout--;
            }
            menu_view(false); // TODO: move it, it is wrong place
            LCD_Update(); // TODO: move it, it is wrong place
            // do not use continue here (menu_auto_update_timeout==0)
        }

		// menu state machine
		if (kb_events || (menu_auto_update_timeout==0)) {
           bool update = menu_controller(false);
           if (update) {
               menu_controller(true); // menu updated, call it again
           } 
           menu_view(update); // TODO: move it, it is wrong place
	       LCD_Update(); // TODO: move it, it is wrong place
		}
    } //End Main loop
	return 0;
}



/*!
 *******************************************************************************
 * Initialize all modules
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



    //! Calibrate the internal RC Oszillator
    //! \todo test calibrate_rco();

    //! set Clock to 4 Mhz
    CLKPR = (1<<CLKPCE);            // prescaler change enable
    CLKPR = (1<<CLKPS0);            // prescaler = 2 (internal RC runs @ 8MHz)

    //! Disable Analog Comparator (power save)
    ACSR = (1<<ACD);

    //! Disable Digital input on PF0-7 (power save)
    DIDR0 = 0xFF;

    //! Power reduction mode
    power_down_ADC();

    //! digital I/O port direction
    DDRG = (1<<PG3)|(1<<PG4); // PG3, PG4 Motor out

    //! enable pullup on all inputs (keys and m_wheel)
    //! ATTENTION: PB0 & PB6 is input, but we will select it only for read
    PORTB = (0<<PB0)|(1<<PB1)|(1<<PB2)|(1<<PB3)|(0<<PB6);
    DDRB  = (1<<PB0)|(1<<PB4)|(1<<PB7)|(1<<PB6); // PB4, PB7 Motor out

#if (RFM_WIRE_MARIOJTAG == 1)
    DDRE  = (1<<PE3)|                  (1<<PE1); // ACTLIGHTEYE | TXD
	PORTE =                   (1<<PE2)|(1<<PE1)|(1<<PE0); // RFMSDO(pullup) | TXD | RXD
	//PORTE =                            (1<<PE1)|(1<<PE0); // TXD | RXD
	DDRF  =          (1<<PF6)|(1<<PF5)|(1<<PF4)|(1<<PF3); // RFMSDI | RFMNSEL | RFMSCK | ACTTEMPSENS
    PORTF = (1<<PF7)|         (1<<PF5); // JTAGTDI | RFMNSEL;

#elif (RFM_WIRE_JD_INTERNAL == 1)
    DDRE  = (1<<PE3)|(1<<PE2)|(1<<PE1);  // PE3  activate lighteye
	PORTE = (1<<PE6)|(1<<PE1)|(1<<PE0);  // RFMSDO(pullup) | TXD | RXD(pullup)
    DDRF  = (1<<PF0)|(1<<PF1)|(1<<PF3);  // RFMSDI | RFMSCK | PF3  activate tempsensor
    PORTF = 0xf0;
    PORTA = (1<<PA3); // RFMnSEL
    DDRA = (1<<PA3); // RFMnSEL
    
#else
    DDRE = (1<<PE3)|(1<<PE2)|(1<<PE1);  // PE3  activate lighteye
    PORTE = 0x03;
    DDRF = (1<<PF3);          // PF3  activate tempsensor
    PORTF = 0xf3;
#endif

    //! remark for PCMSK0:
    //!     PCINT0 for lighteye (motor monitor) is activated in motor.c using
    //!     mask register PCMSK0: PCMSK0=(1<<PCINT4) and PCMSK0&=~(1<<PCINT4)

    //! PCMSK1 for keyactions
    PCMSK1 = (1<<PCINT9)|(1<<PCINT10)|(1<<PCINT11)|(1<<PCINT13);

    //! activate PCINT0 + PCINT1
    EIMSK = (1<<PCIE1)|(1<<PCIE0);

    //! Initialize the RTC
    RTC_Init();

    eeprom_config_init((~PINB & (KBI_PROG | KBI_C | KBI_AUTO))==(KBI_PROG | KBI_C | KBI_AUTO));

	crypto_init();

    //! Initialize the motor
    MOTOR_Init();

    //1 Initialize the LCD
    LCD_Init();

#if RFM
	RFM_init();
	RFM_OFF();
#endif
    
	// init keyboard by one dummy call
    task_keyboard();
}
