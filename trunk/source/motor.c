/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  compiler:   WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Dario Carluccio (hr20-at-carluccio-dot-de)
 *              2008 Jiri Dobry (jdobry-at-centrum-dot-cz) 
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
 * \file       motor.c
 * \brief      functions to control the HR20 motor
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
#include "main.h"
#include "motor.h"
#include "lcd.h"
#include "rtc.h"
#include "eeprom.h"
#include "task.h"
#include "rs232_485.h"
#include "controller.h"

// typedefs


// vars
static int16_t MOTOR_PosAct;      //!< actual position
static int16_t MOTOR_PosMax;      /*!< position if complete open (100%) <BR>
                                          0 if not calibrated */
static int16_t MOTOR_PosStop;     //!< stop at this position
motor_dir_t MOTOR_Dir;          //!< actual direction
// bool MOTOR_Mounted;         //!< mountstatus true: if valve is mounted
static int8_t MOTOR_calibration_step=-2; // not calibrated
static int8_t MOTOR_run_timeout=0;
static volatile int8_t MOTOR_eye_buffer=0;
static volatile int8_t MOTOR_timer_buffer=0;

#define MOTOR_RUN_OVERLOAD (3+1)
static uint8_t MOTOR_run_overload=0;



/*!
 *******************************************************************************
 *  Init motor
 *
 *  \note
 *  - configure pwm
 *  - stop motor
 *  - init values
 ******************************************************************************/
void MOTOR_Init(void)
{
    MOTOR_updateCalibration(true,0);
    TIMSK0 = (1<<TOIE0); //enable interrupt from timer0 overflow
}


static uint8_t MOTOR_wait_for_new_calibration = 5;
/*!
 *******************************************************************************
 *  Reset calibration data
 *
 *  \note
 *  - must be called if the motor is unmounted
 ******************************************************************************/
void MOTOR_updateCalibration(bool unmounted, uint8_t percent)
{
    if (unmounted) {
        MOTOR_PosAct=0;                  // not calibrated
        MOTOR_PosMax=0;                  // not calibrated
        MOTOR_PosStop=0;                 // not calibrated
        MOTOR_calibration_step=-2;     // not calibrated
        MOTOR_wait_for_new_calibration = 5;
        MOTOR_Control(stop);       // stop motor
    } else {
        if (MOTOR_wait_for_new_calibration != 0) {
            MOTOR_wait_for_new_calibration--;
        } else {
			if (MOTOR_calibration_step==-2) {
            	MOTOR_Calibrate(percent);
			}
        }
    }
}


/*!
 *******************************************************************************
 *  \returns
 *   - actual position of motor in percent
 *   - 255 if motor is not calibrated
 ******************************************************************************/
uint8_t MOTOR_GetPosPercent(void)
{
    if ((MOTOR_PosMax > 10) && (MOTOR_calibration_step==0)){
        return (uint8_t) ( (MOTOR_PosAct * 10) / (MOTOR_PosMax/10) );
    } else {
        return 255;
    }
}

/*!
 *******************************************************************************
 * \returns
 *   - true:  calibration finished succesfully
 *   - false: motor is not calibrated 
 ******************************************************************************/
bool MOTOR_IsCalibrated(void)
{
    return ((MOTOR_PosMax != 0) && (MOTOR_calibration_step==0));
}


/*!
 *******************************************************************************
 * drive motor to desired position in percent
 * \param  percent desired endposition 0-100
 *         - 0 : closed
 *         - 100 : open
 *
 * \note   works only if calibrated before
 ******************************************************************************/
bool MOTOR_Goto(uint8_t percent)
{
    // works only if calibrated
    if (MOTOR_calibration_step==0){
        // set stop position
        if (percent == 100) {
            MOTOR_PosStop = MOTOR_PosMax + config.motor_hysteresis - config.motor_protection;
        } else if (percent == 0) {
            MOTOR_PosStop = config.motor_protection-config.motor_hysteresis;
        } else {
			// MOTOR_PosMax>>2 and 100>>2 => overload protection
			#if (MOTOR_MAX_IMPULSES>>2)*(100>>2) > INT16_MAX
			 #error OVERLOAD possible
			#endif
            MOTOR_PosStop = (int16_t) 
                (percent * 
                    ((MOTOR_PosMax-config.motor_protection-config.motor_protection)>>2) / (100>>2)
                ) + config.motor_protection;
        }
        // switch motor on
        if (MOTOR_PosAct > MOTOR_PosStop){
            MOTOR_Control(close);
        } else if (MOTOR_PosAct < MOTOR_PosStop){
            MOTOR_Control(open);
        }
        return true;
    } else {
        return false;
    }
}

/*!
 *******************************************************************************
 * calibrate the motor and drive it to position in percent
 *
 * \param  percent desired position after calibration 0-100
 *         - 0 : closed
 *         - 100 : open
 *
 * \note minimises motor time (save power)
 ******************************************************************************/
void MOTOR_Calibrate(uint8_t percent)
{
    MOTOR_PosAct=0;                  // not calibrated
    MOTOR_PosMax=0;                  // not calibrated
    MOTOR_PosStop=0;                 // not calibrated

    if (percent > 50) {
        MOTOR_PosStop= -MOTOR_MAX_IMPULSES;
        MOTOR_Control(close);
    } else {
        MOTOR_PosStop= +MOTOR_MAX_IMPULSES;
        MOTOR_Control(open);
    }
    MOTOR_calibration_step=1;
}

/*!
 *******************************************************************************
 * controll motor movement
 *
 * \param  direction motordirection
 *         - stop
 *         - open
 *         - close
 *
 * \note PWM runs at 15,625 kHz
 *
 * \note Output for direction: \verbatim
     direction  PG3  PG4  PB7  PB4/PWM(OC0A)    PE3    PCINT4
       stop:     0    0    0    0                0      off
       open:     0    1    1   invert. mode      1      on
       close:    1    0    0   non inv mode      1      on       \endverbatim
 ******************************************************************************/
void MOTOR_Control(motor_dir_t direction)
{
    if (direction == stop){                             // motor off
        // photo eye
        PCMSK0 &= ~(1<<PCINT4);                         // deactivate interrupt
        MOTOR_HR20_PE3_P &= ~(1<<MOTOR_HR20_PE3);       // deactivate photo eye
        // set all pins of H-Bridge to LOW
        MOTOR_HR20_PG3_P &= ~(1<<MOTOR_HR20_PG3);       // PG3 LOW
        MOTOR_HR20_PG4_P &= ~(1<<MOTOR_HR20_PG4);       // PG4 LOW
        MOTOR_HR20_PB7_P &= ~(1<<MOTOR_HR20_PB7);       // PB7 LOW
        // stop pwm signal
        TCCR0A = (1<<WGM00) | (1<<WGM01); // 0b 0000 0011
    } else {                                            // motor on
        if (MOTOR_Dir != direction){
            // photo eye
            MOTOR_HR20_PE3_P |= (1<<MOTOR_HR20_PE3);    // activate photo eye / can generate false IRQ
        	MOTOR_run_timeout = config.motor_run_timeout;
			MOTOR_timer_buffer=0;

            PCMSK0 |= (1<<PCINT4);  // activate interrupt, false IRQ must be cleared before
			MOTOR_run_overload=0;
            // open
            if ( direction == close) {
                // set pins of H-Bridge
                MOTOR_HR20_PG3_P |=  (1<<MOTOR_HR20_PG3);   // PG3 HIGH
                MOTOR_HR20_PG4_P &= ~(1<<MOTOR_HR20_PG4);   // PG4 LOW
                MOTOR_HR20_PB7_P &= ~(1<<MOTOR_HR20_PB7);   // PB7 LOW
                // set PWM non inverting mode
	            OCR0A = config.motor_speed_close; // set pwm value
                TCCR0A = (1<<WGM00) | (1<<WGM01) | (1<<COM0A1) | (1<<CS00);
            // close
            } else {
                // set pins of H-Bridge
                MOTOR_HR20_PG3_P &= ~(1<<MOTOR_HR20_PG3);   // PG3 LOW
                MOTOR_HR20_PG4_P |=  (1<<MOTOR_HR20_PG4);   // PG4 HIGH
                MOTOR_HR20_PB7_P |=  (1<<MOTOR_HR20_PB7);   // PB7 HIGH
                // set PWM inverting mode
	            OCR0A = config.motor_speed_open;  // set pwm value
                TCCR0A=(1<<WGM00)|(1<<WGM01)|(1<<COM0A1)|(1<<COM0A0)|(1<<CS00);
            }
		}
    }
    // set new speed and direction
    MOTOR_Dir = direction;
}

/*!
 *******************************************************************************
 * Update motor position
 *
 * \note called by TASK_UPDATE_MOTOR_POS event
 *
 ******************************************************************************/
void MOTOR_update_pos(void){
    MOTOR_run_timeout = config.motor_run_timeout;  
    if (MOTOR_Dir == open) {
        cli();
            MOTOR_PosAct+=MOTOR_eye_buffer;
            MOTOR_eye_buffer=0;
        sei();
        if (!(MOTOR_PosAct < MOTOR_PosStop)){
			// add small time to go over point of motor clock generation
			// it improve precision
            MOTOR_run_overload=MOTOR_RUN_OVERLOAD;
        }
    } else {
        cli();
            MOTOR_PosAct-=MOTOR_eye_buffer;
            MOTOR_eye_buffer=0;
        sei();
        if (!(MOTOR_PosAct > MOTOR_PosStop)){
			// add small time to go over point of motor clock generation
			// it improve precision
            MOTOR_run_overload=MOTOR_RUN_OVERLOAD;
        }
    }
}

/*!
 *******************************************************************************
 * motor timer
 *
 * \note called by TASK_MOTOR_TIMER event
 *
 ******************************************************************************/
void MOTOR_timer(void) {
    if (MOTOR_run_overload>0) {
		if (MOTOR_run_overload==1) {
            MOTOR_Control(stop);
			MOTOR_run_overload=0;
            if (MOTOR_calibration_step != 0) {
                MOTOR_calibration_step = -1;     // calibration error
            }
		} else {
			MOTOR_run_overload--;
		}
	} 
    if (MOTOR_run_timeout>0) {
        cli();  // MOTOR_timer_ovf is interrupt handled
            MOTOR_run_timeout-=MOTOR_timer_buffer;
            MOTOR_timer_buffer=0;
        sei();
    }
    if (MOTOR_run_timeout<=0) {  // time out
        if (MOTOR_Dir == open) { // stopped on end
            MOTOR_Control(stop); // position on end
            MOTOR_PosMax = MOTOR_PosAct; // recalibrate it on the end
            if (MOTOR_calibration_step == 1) {
                MOTOR_Control(close);
                MOTOR_PosStop= MOTOR_PosAct-MOTOR_MAX_IMPULSES;
                MOTOR_calibration_step = 2;
            } else if (MOTOR_calibration_step == 2) {
                MOTOR_calibration_step = 0;     // calibration DONE
            }
        } else if (MOTOR_Dir == close) { // stopped on end
            MOTOR_Control(stop); // position on end
            MOTOR_PosMax -= MOTOR_PosAct; // recalibrate it on the end
			MOTOR_PosAct = 0;
            if (MOTOR_calibration_step == 1) {
                MOTOR_PosStop= MOTOR_PosAct+MOTOR_MAX_IMPULSES;
                MOTOR_Control(open);
                MOTOR_calibration_step = 2;
            } else if (MOTOR_calibration_step == 2) {
                MOTOR_calibration_step = 0;     // calibration DONE
            }
        } else {
            MOTOR_Control(stop); // just ensurance 
        }  
        if ((MOTOR_calibration_step == 0) &&
            ((MOTOR_PosAct>MOTOR_MAX_IMPULSES+1) || (MOTOR_PosMax<MOTOR_MIN_IMPULSES))) {
            CTL_error |=  CTL_ERR_MOTOR;
        } else {
            CTL_error &= ~CTL_ERR_MOTOR;
        }
    }
}

// interrupts: 

// eye_timer is noise canceler for eye input / improve accuracy
static uint8_t eye_timer=0; // non volatile, itsn't shared to non-interrupt code 
static uint8_t pine_last=0;
/*!
 *******************************************************************************
 * Pinchange Interupt INT0
 *
 * \note count light eye impulss: \ref MOTOR_PosAct
 *
 * \note create TASK_UPDATE_MOTOR_POS
 ******************************************************************************/
ISR (PCINT0_vect){
	uint8_t pine=PINE;
	#if (defined COM_RS232) || (defined COM_RS485)
		if ((pine & (1<<PE0)) == 0) {
			RS_enable_rx(); // it is macro, not function
		    PCMSK0 &= ~(1<<PCINT0); // deactivate interrupt
		}
	#endif
    // count only on HIGH impulses
    if ((((pine & ~pine_last & (1<<PE4)) != 0)) && (eye_timer==0)) {
        task|=TASK_UPDATE_MOTOR_POS;
        MOTOR_eye_buffer++;
        eye_timer = EYE_TIMER_NOISE_PROTECTION; // in timer0 overflow ticks
    }
	pine_last=pine;
}

static uint8_t timer0_cnt=0; // non volatile, itsn't shared to non-interrupt code 
/*!
 *******************************************************************************
 * Timer0 overflow interupt
 * runs at 15,625 kHz (variant 1953.125)
 * \note Timer0 is active only if motor runing
 ******************************************************************************/
ISR (TIMER0_OVF_vect){
    if ((++timer0_cnt) == 0) {  // 61.03515625 Hz (variant 7.62939453125)
        task|=TASK_MOTOR_TIMER;
        MOTOR_timer_buffer++;
    }
    if (eye_timer) eye_timer--;
}

