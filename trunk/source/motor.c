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
#include "com.h"
#include "debug.h"

// typedefs


// vars
static volatile int16_t MOTOR_PosAct=0;      //!< actual position
int16_t MOTOR_PosMax=0;      /*!< position if complete open (100%) <BR>
                                          0 if not calibrated */

//static int16_t MOTOR_ManuCalibration=0;

static volatile int16_t MOTOR_PosStop;     //!< stop at this position
static volatile motor_dir_t MOTOR_Dir;          //!< actual direction
// bool MOTOR_Mounted;         //!< mountstatus true: if valve is mounted
int8_t MOTOR_calibration_step=-2; // not calibrated
volatile uint16_t motor_diag = 0;
static volatile uint16_t motor_diag_cnt = 0;

static volatile uint16_t motor_max_time_for_impulse[2];
static volatile uint16_t motor_eye_noise_protection[2];
static uint32_t motor_diag_sum;
static int16_t motor_diag_count;

static volatile uint16_t motor_timer = 0;


static void MOTOR_Control(motor_dir_t); // control H-bridge of motor

static uint8_t MOTOR_wait_for_new_calibration = 5;

// eye_timer is noise canceler for eye input / improve accuracy
static volatile uint16_t eye_timer=0; 

/*!
 *******************************************************************************
 *  Reset calibration data
 *
 *  \param cal_type 0 = unmounted
 *  \param cal_type 1 = 
 *  \param cal_type 2 = switch to manual  
 *  \param cal_type 3 = switch to auto  
 *  \note
 *  - must be called if the motor is unmounted
 ******************************************************************************/
void MOTOR_updateCalibration(uint8_t cal_type)
{
    if (cal_type == 0) {
        MOTOR_Control(stop);       // stop motor
        MOTOR_PosAct=0;                  // not calibrated
        MOTOR_PosMax=0;                  // not calibrated
        MOTOR_calibration_step=-2;     // not calibrated
        MOTOR_wait_for_new_calibration = 5;
        CTL_error &= ~CTL_ERR_MOTOR;
    } else {
        if (MOTOR_wait_for_new_calibration != 0) {
            MOTOR_wait_for_new_calibration--;
        } else {
            if (MOTOR_calibration_step==1) {
                motor_max_time_for_impulse[0] = DEFAULT_motor_max_time_for_impulse; 
                motor_max_time_for_impulse[1] = DEFAULT_motor_max_time_for_impulse; 
                motor_eye_noise_protection[0] = DEFAULT_motor_eye_noise_protection;
                motor_eye_noise_protection[1] = DEFAULT_motor_eye_noise_protection;
                MOTOR_calibration_step++;
                MOTOR_PosStop= +MOTOR_MAX_IMPULSES;
                MOTOR_Control(open);
            }
        }
        if (MOTOR_calibration_step==-2) {
            if (cal_type == 3) {
                MOTOR_ManuCalibration=-1;               // automatic calibration
                eeprom_config_save((uint16_t)(&config.MOTOR_ManuCalibration_L)-(uint16_t)(&config));
                eeprom_config_save((uint16_t)(&config.MOTOR_ManuCalibration_H)-(uint16_t)(&config));
            }  else if (cal_type == 2) { 
                MOTOR_ManuCalibration=0;               // not calibrated
            }
            MOTOR_calibration_step=1;
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
        if (MOTOR_Dir==stop) {
            int16_t a=MOTOR_PosAct; // volatile variable optimization
            int16_t s=MOTOR_PosStop; // volatile variable optimization
            if (a > s){
                MOTOR_Control(close);
            } else if (a < s){
                MOTOR_Control(open);
            }
        }
        return true;
    } else {
        return false;
    }
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
static void MOTOR_Control(motor_dir_t direction) {
    if (direction == stop){                             // motor off
        // photo eye
        TIMSK0 = 0;                                     // disable interrupt
        PCMSK0 &= ~(1<<PCINT4);                         // disable interrupt
        MOTOR_HR20_PE3_P &= ~(1<<MOTOR_HR20_PE3);       // deactivate photo eye
		MOTOR_H_BRIDGE_stop();
        // stop pwm signal
        TCCR0A = (1<<WGM00) | (1<<WGM01); // 0b 0000 0011
	    MOTOR_Dir = stop;
    } else {                                            // motor on
        if (MOTOR_Dir != direction){
            motor_diag_sum=0;
            motor_diag_count = -MOTOR_IGNORE_IMPULSES;
            motor_diag_cnt=0;
		    MOTOR_Dir = direction;
            // photo eye
            MOTOR_HR20_PE3_P |= (1<<MOTOR_HR20_PE3);    // activate photo eye
            motor_timer = (motor_max_time_for_impulse[(direction==close)?0:1]<<2); // *4 (for motor start-up)
            eye_timer = motor_eye_noise_protection[(direction==close)?0:1];
            TIFR0 = (1<<TOV0);
            TCNT0 = 0;
            TIMSK0 = (1<<TOIE0); //enable interrupt from timer0 overflow
            // PCMSK0 |= (1<<PCINT4);  // enable interrupt from eye
            if ( direction == close) {
                // set pins of H-Bridge
                MOTOR_H_BRIDGE_close();
                // set PWM non inverting mode
	            OCR0A = config.motor_speed_close; // set pwm value
                TCCR0A = (1<<WGM00) | (1<<WGM01) | (1<<COM0A1) | (1<<CS00);
            // close
            } else {
                // set pins of H-Bridge
				MOTOR_H_BRIDGE_open();
                // set PWM inverting mode
	            OCR0A = config.motor_speed_open;  // set pwm value
                TCCR0A=(1<<WGM00)|(1<<WGM01)|(1<<COM0A1)|(1<<COM0A0)|(1<<CS00);
            }
		}
    }
}

/*!
 *******************************************************************************
 * motor eye pulse
 *
 * \note called by TASK_MOTOR_PULSE event
 *
 ******************************************************************************/
void MOTOR_timer_pulse(void) {
    motor_dir_t d = MOTOR_Dir;
    if (motor_diag <= MOTOR_MAX_VALID_TIMER) {
        if (motor_diag_count >= 0) { //ignote 
			motor_diag_sum += motor_diag;
		}
		motor_diag_count++;
        if ((motor_diag_count > MOTOR_UPDATE_IMPULSES_THLD) && (d!=stop)) {
            uint32_t tmp = motor_diag_sum / motor_diag_count;
            {
                uint16_t b = tmp * config.motor_end_detect /100;
                cli();
                motor_max_time_for_impulse[(d==close)?0:1] = b;
                sei();
            }
            {
                uint16_t b = tmp * config.motor_eye_noise_protect /100;
                cli();
                motor_eye_noise_protection[(d==close)?0:1] = b;
                sei();
            }
        }
    } 

    #if DEBUG_PRINT_MOTOR
        COM_debug_print_motor(MOTOR_Dir, motor_diag);
    #endif
}

/*!
 *******************************************************************************
 * motor timer
 *
 * \note called by TASK_MOTOR_STOP event
 *
 ******************************************************************************/
void MOTOR_timer_stop(void) {
	motor_dir_t d = MOTOR_Dir;
    MOTOR_Control(stop);
    if (eye_timer == 0xffff) { // normal stop on wanted position 
			if ((MOTOR_calibration_step == 3) && (MOTOR_ManuCalibration>0)) {
				MOTOR_calibration_step = 0;
			} else if (MOTOR_calibration_step != 0) {
                MOTOR_calibration_step = -1;     // calibration error
		        CTL_error |=  CTL_ERR_MOTOR;
            }
	} else { // stop after motor eye timeout
        if (d == open) { // stopped on end
            {
                int16_t a = MOTOR_PosAct; // volatile variable optimization
                if (MOTOR_calibration_step == 2) {
                    MOTOR_Control(close);
                    MOTOR_PosStop= ((MOTOR_ManuCalibration<=0)?(a-MOTOR_MAX_IMPULSES):0);
                    MOTOR_PosMax = a;
                    MOTOR_calibration_step = 3;
                } else { 
                    if ((MOTOR_ManuCalibration==0) && (a>=MOTOR_MIN_IMPULSES)) {
                        MOTOR_ManuCalibration = (MOTOR_PosMax = a);
                        eeprom_config_save((uint16_t)(&config.MOTOR_ManuCalibration_L)-(uint16_t)(&config));
                        eeprom_config_save((uint16_t)(&config.MOTOR_ManuCalibration_H)-(uint16_t)(&config));
                    } else {
                        MOTOR_PosAct = MOTOR_PosMax; // motor position cleanup
                    }
                }
            }
        } else if (d == close) { // stopped on end
            {
                if (MOTOR_calibration_step == 3 ) {
                    MOTOR_calibration_step = 0;     // calibration DONE
                    if (MOTOR_ManuCalibration<0) { // only for automatic calibration
                        MOTOR_PosMax -= MOTOR_PosAct;
                    }
                }
                if (MOTOR_PosAct>MOTOR_MIN_IMPULSES) {
                    MOTOR_calibration_step = -1;     // calibration error
                    CTL_error |=  CTL_ERR_MOTOR;
                }
                MOTOR_PosAct = 0; // cleanup position
            }
        }
    }
    if ((MOTOR_calibration_step == 0) &&
        (MOTOR_PosMax<MOTOR_MIN_IMPULSES)) {
        MOTOR_calibration_step = -1;     // calibration error
        CTL_error |=  CTL_ERR_MOTOR;
    } 
    #if DEBUG_PRINT_MOTOR
        COM_debug_print_motor(stop, motor_diag);
    #endif
}

// interrupts: 

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
    // motor eye
    // count only on HIGH impulses
    if ((PCMSK0 & (1<<PCINT4)) && ((pine & ~pine_last & (1<<PE4)) != 0)) {
        MOTOR_PosAct+=MOTOR_Dir;
        motor_diag = motor_diag_cnt;
        motor_diag_cnt=0;
        task|=TASK_MOTOR_PULSE;
        PCMSK0 &= ~(1<<PCINT4); // disable eye interrupt
        if (MOTOR_PosAct == MOTOR_PosStop) {
            // motor will be stopped after MOTOR_RUN_OVERLOAD time
            eye_timer = 0xffff;
            motor_timer = motor_eye_noise_protection[(MOTOR_Dir==close)?0:1]; // STOP timeout
        } else {
            eye_timer = motor_eye_noise_protection[(MOTOR_Dir==close)?0:1]; // in timer0 overflow ticks
            motor_timer = motor_max_time_for_impulse[(MOTOR_Dir==close)?0:1];
        }
    }
	pine_last=pine;
}

/*! 
 *******************************************************************************
 * Timer0 overflow interupt
 * runs at 15,625 kHz (variant 1953.125)
 * \note Timer0 is active only if motor runing
 ******************************************************************************/
ISR (TIMER0_OVF_vect){
    motor_diag_cnt++;
    if ( motor_timer > 0) {
        motor_timer--;
        {
            uint16_t e = eye_timer; // optimization for volatile variable 
            if (e>0)  {
                if (e != 0xffff) eye_timer=e-1;      
            } else {
                pine_last = 0;
                PCMSK0 |= (1<<PCINT4); // enable eye interrupt
            }
        }
    } else {
        motor_diag = motor_diag_cnt;
        // motor fast STOP
        MOTOR_H_BRIDGE_stop();
        // complete STOP will be done later
		TCCR0A = (1<<WGM00) | (1<<WGM01); // 0b 0000 0011
        task|=(TASK_MOTOR_STOP);
        PCMSK0 &= ~(1<<PCINT4); // disable eye interrupt

    }
}
