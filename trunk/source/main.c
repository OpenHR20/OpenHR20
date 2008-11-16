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
#include "rtc.h"
#include "task.h"
#include "keyboard.h"
#include "eeprom.h"
#include "pid.h"
#include "debug.h"
#include "menu.h"
#include "com.h"
#include "rs232_485.h"
#include "controller.h"

// global Vars
volatile bool    m_automatic_mode;         // auto mode (false: manu mode)

// global Vars for default values: temperatures and speed
uint8_t valve_wanted=0;
// serial number
uint16_t serialNumber;	//!< Unique serial number \todo move to CONFIG.H

// prototypes 
int main(void);                            // main with main loop
static inline void init(void);                           // init the whole thing
void load_defauls(void);                   // load default values
                                           // (later from eeprom using config.c)
void callback_settemp(uint8_t);            // called from RTC to set new reftemp
void setautomode(bool);                    // activate/deactivate automode
uint8_t input_temp(uint8_t);

// Check AVR LibC Version >= 1.6.0
#if __AVR_LIBC_VERSION__ < 10600UL
#warning "avr-libc >= version 1.6.0 recommended"
#warning "This code has not been tested with older versions."
#endif


/*!
 *******************************************************************************
 * main program
 ******************************************************************************/
int main(void)
{
    //! initalization
    init();

	task=0;

    //! Enable interrupts
    sei();

	COM_init();

    LCD_AllSegments(LCD_MODE_ON);                   // all segments on
	LCD_Update();

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

			if (sleep_with_ADC==1) {
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
                // TODO
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
            }
            MOTOR_updateCalibration(mont_contact_pooling());
            MOTOR_Goto(valve_wanted);
            task_keyboard_long_press_detect();
            start_task_ADC();
            if (menu_auto_update_timeout>0) {
                menu_auto_update_timeout--;
            }
            menu_view(false); // TODO: move it, it is wrong place
            LCD_Update(); // TODO: move it, it is wrong place
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
 * Initializate all modules
 ******************************************************************************/
static inline void init(void)
{
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
    DDRE = (1<<PE3)|(1<<PE2)|(1<<PE1);  // PE3  activate lighteye
    PORTE = 0x03;
    DDRF = (1<<PF3);          // PF3  activate tempsensor
    PORTF = 0xf3;

    //! enable pullup on all inputs (keys and m_wheel)
    //! ATTENTION: PB0 & PB6 is input, but we will select it only for read
    PORTB = (0<<PB0)|(1<<PB1)|(1<<PB2)|(1<<PB3)|(0<<PB6);
    DDRB = (1<<PB0)|(1<<PB4)|(1<<PB7)|(1<<PB6); // PB4, PB7 Motor out


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

    //! Initialize the motor
    MOTOR_Init();


    //1 Initialize the LCD
    LCD_Init();

}
