/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  ompiler:    WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Dario Carluccio (hr20-at-carluccio-dot-de)
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
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>
 * \date       08.02.2008
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
#include "com.h"

// typedefs


// vars
volatile uint16_t MOTOR_PosAct;      //!< actual position
volatile uint16_t MOTOR_PosMax;      /*!< position if complete open (100%) <BR>
                                          0 if not calibrated */
volatile uint16_t MOTOR_PosStop;     //!< stop at this position
volatile uint16_t MOTOR_PosLast;     //!< last position for check if blocked
volatile motor_dir_t MOTOR_Dir;      //!< actual direction
volatile bool MOTOR_Mounted;         //!< mountstatus true: if valve is mounted



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
    MOTOR_PosAct=0;                  // not calibrated
    MOTOR_PosMax=0;                  // not calibrated
    MOTOR_PosStop=0;                 // not calibrated
    MOTOR_Control(stop, full);       // stop motor
}


/*!
 *******************************************************************************
 *  Reset calibration data
 *
 *  \note
 *  - must be called if the motor is unmounted
 ******************************************************************************/
void MOTOR_ResetCalibration(void)
{
    MOTOR_PosAct=0;                  // not calibrated
    MOTOR_PosMax=0;                  // not calibrated
    MOTOR_PosStop=0;                 // not calibrated
    MOTOR_Control(stop, full);       // stop motor
}


/*!
 *******************************************************************************
 *  \returns
 *   - actual position of motor in percent
 *   - 255 if motor is not calibrated
 ******************************************************************************/
uint8_t MOTOR_GetPosPercent(void)
{
    if (MOTOR_PosMax > 10){
        return (uint8_t) ( (MOTOR_PosAct * 10) / (MOTOR_PosMax/10) );
    } else {
        return 255;
    }
}


/*!
 *******************************************************************************
 * Set the actual status for the motor
 *  \param value
 *   - true:  motor is mounted to valve
 *   - false: motor is not mounted
 ******************************************************************************/
void MOTOR_SetMountStatus(bool value)
{
    MOTOR_Mounted = value;
    if (value==false){
        MOTOR_ResetCalibration();
    }
}


/*!
 *******************************************************************************
 * \returns
 *   - true:  motor is on
 *   - false: motor is off
 ******************************************************************************/
bool MOTOR_On(void)
{
    return (MOTOR_Dir != stop);
}


/*!
 *******************************************************************************
 * \returns
 *   - true:  calibration finished succesfully
 *   - false: motor is not calibrated 
 ******************************************************************************/
bool MOTOR_IsCalibrated(void)
{
    return (MOTOR_PosMax != 0);
}


/*!
 *******************************************************************************
 * drive motor to desired position in percent
 * \param  percent desired endposition 0-100
 *         - 0 : closed
 *         - 100 : open
 * \param speed  motorspeed in percent for PWM
 *
 * \note   works only if calibrated before
 ******************************************************************************/
bool MOTOR_Goto(uint8_t percent, motor_speed_t speed)
{
    // works only if calibrated
    if (MOTOR_PosMax > 0){
        // set stop position
        if (percent == 100) {
            MOTOR_PosStop = MOTOR_PosMax + MOTOR_HYSTERESIS;
        } else if (percent == 0) {
            MOTOR_PosStop = 0;
            MOTOR_PosAct += MOTOR_HYSTERESIS;
        } else {
            MOTOR_PosStop = (uint16_t) (percent * MOTOR_PosMax / 100);
        }
        // switch motor on
        if (MOTOR_PosAct > MOTOR_PosStop){
            MOTOR_Control(close, speed);
        } else if (MOTOR_PosAct < MOTOR_PosStop){
            MOTOR_Control(open, speed);
        }
        // Notify COM
      	COM_setNotify(NOTIFY_VALVE);
        return true;
    } else {
        return false;
    }
}


/*!
 *******************************************************************************
 * calibrate the motor with default values
 *
 * \note called by com.c, but belongs here !
 * \todo: check if MOTOR_Calibrate(0, full) can be used instead
 ******************************************************************************/
void MOTOR_Do_Calibrate(void)
{
	MOTOR_ResetCalibration();
	MOTOR_Calibrate(0, full);
	return;
}


/*!
 *******************************************************************************
 * calibrate the motor and drive it to position in percent
 * \ returns
 *    - true if calibration successfull 
 *    - false if not successfull possible reasons:
 *      - motor is not stopped after \ref MOTOR_MAX_IMPULSES impulses
 *      - motor has been dismounted
 *
 * \param  percent desired position after calibration 0-100
 *         - 0 : closed
 *         - 100 : open
 *
 * \param speed  motorspeed in percent for PWM
 *
 * \note minimises motor time (save power)
 ******************************************************************************/
bool MOTOR_Calibrate(uint8_t percent, motor_speed_t speed)
{
    uint16_t postmp;
    if (percent > 50) {
        // - close till no movement or more than MOTOR_MAX_IMPULSES
        MOTOR_PosAct = MOTOR_MAX_IMPULSES;
        MOTOR_PosStop = 0;
        MOTOR_Control(close, speed);
        do {
            postmp = MOTOR_PosAct;
            delay(200);
        } while ((MOTOR_Dir != stop) && (postmp != MOTOR_PosAct) &&
                  MOTOR_Mounted); // motor still on, moving and mounted
        // stopped by ISR?, turning too long, not mounted -> error
        if ((MOTOR_Dir == stop) || (MOTOR_Mounted==false)){
            return false;
        }
        // motor is on, but not turning any more -> endposition reached -> stop the motor
        MOTOR_Control(stop, speed);
        // now open till no movement or more than MOTOR_MAX_IMPULSES
        MOTOR_PosAct = 0;
        MOTOR_PosStop = MOTOR_MAX_IMPULSES;
        MOTOR_Control(open, speed);
        do {
            postmp = MOTOR_PosAct;
            delay(200);
        } while ((MOTOR_Dir != stop) && (postmp != MOTOR_PosAct) &&
                  MOTOR_Mounted); // motor still on, moving and mounted
        // stopped by ISR?, turning too long, not mounted -> error
        if ((MOTOR_Dir == stop) || (MOTOR_Mounted==false)){
            return false;
        }
        // motor is on, but not turning any more -> endposition reached -> stop the motor
        MOTOR_Control(stop, speed);
        MOTOR_PosMax = MOTOR_PosAct;
    } else {
        // - open till no movement or more than MOTOR_MAX_IMPULSES
        MOTOR_PosAct = 0;
        MOTOR_PosStop = MOTOR_MAX_IMPULSES;
        MOTOR_Control(open, speed);
        do {
            postmp = MOTOR_PosAct;
            delay(200);
        } while ((MOTOR_Dir != stop) && (postmp != MOTOR_PosAct) &&
                  MOTOR_Mounted); // motor still on, moving and mounted
        // stopped by ISR?, turning too long, not mounted -> error
        if ((MOTOR_Dir == stop) || (MOTOR_Mounted==false)){
            return false;
        }
        // motor is on, but not turning any more -> endposition reached -> stop the motor
        MOTOR_Control(stop, speed);
        // now close till no movement or more than MOTOR_MAX_IMPULSES
        MOTOR_PosAct = MOTOR_MAX_IMPULSES;
        MOTOR_PosStop = 0;
        MOTOR_Control(close, speed);
        do {
            postmp = MOTOR_PosAct;
            delay(200);
        } while ((MOTOR_Dir != stop) && (postmp != MOTOR_PosAct) &&
                  MOTOR_Mounted); // motor still on, moving and mounted
        // stopped by ISR?, turning too long, not mounted -> error
        if ((MOTOR_Dir == stop) || (MOTOR_Mounted==false)){
            return false;
        }
        // motor is on, but not turning any more -> endposition reached -> stop the motor
        MOTOR_Control(stop, speed);
        MOTOR_PosMax = MOTOR_MAX_IMPULSES - MOTOR_PosAct;
        MOTOR_PosAct = 0;
    }
    // now MOTOR_PosMax and MOTOR_PosAct calibated
    // goto desired position
    MOTOR_Goto(percent, speed);
    // Send out notify to com.c
    COM_setNotify(NOTIFY_CALIBRATE);	
    // finished
    return true;
}


/*!
 *******************************************************************************
 * stop the motor
 ******************************************************************************/
void MOTOR_Stop(void)
{
    MOTOR_Control(stop, full);
}


/*!
 *******************************************************************************
 * controll motor movement
 *
 * \param  direction motordirection
 *         - stop
 *         - ope
 *         - close
 *
 * \param  speed motorspeed
 *         - full   (MOTOR_FULL_PWM  /100) * 255
 *         - quiet  (MOTOR_QUIET_PWM /100) * 255
 *
 * \param speed motorspeed in percent for PWM
 *
 * \note PWM runs at 15,625 kHz
 *
 * \note Output for direction: \verbatim
     direction  PG3  PG4  PB7  PB4/PWM(OC0A)    PE3    PCINT4
       stop:     0    0    0    0                0      off
       open:     0    1    1   invert. mode      1      on
       close:    1    0    0   non inv mode      1      on       \endverbatim
 ******************************************************************************/
void MOTOR_Control(motor_dir_t direction, motor_speed_t speed)
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
            MOTOR_HR20_PE3_P |= (1<<MOTOR_HR20_PE3);    // activate photo eye
            PCMSK0 = (1<<PCINT4);                       // activate interrupt
            // set pwm value to percentage ( factor 255/100)
            if (speed == full){
                OCR0A = MOTOR_FULL_PWM * 2.55;
            } else {
                OCR0A = MOTOR_QUIET_PWM * 2.55;
            }
            // Reset last Position for MOTOR_CheckBlocked
            MOTOR_PosLast = 0xffff;
            // open
            if ( direction == close) {
                // set pins of H-Bridge
                MOTOR_HR20_PG3_P |=  (1<<MOTOR_HR20_PG3);   // PG3 HIGH
                MOTOR_HR20_PG4_P &= ~(1<<MOTOR_HR20_PG4);   // PG4 LOW
                MOTOR_HR20_PB7_P &= ~(1<<MOTOR_HR20_PB7);   // PB7 LOW
                // set PWM non inverting mode
                TCCR0A = (1<<WGM00) | (1<<WGM01) | (1<<COM0A1) | (1<<CS00);
            // close
            } else {
                // set pins of H-Bridge
                MOTOR_HR20_PG3_P &= ~(1<<MOTOR_HR20_PG3);   // PG3 LOW
                MOTOR_HR20_PG4_P |=  (1<<MOTOR_HR20_PG4);   // PG4 HIGH
                MOTOR_HR20_PB7_P |=  (1<<MOTOR_HR20_PB7);   // PB7 HIGH
                // set PWM inverting mode
                TCCR0A=(1<<WGM00)|(1<<WGM01)|(1<<COM0A1)|(1<<COM0A0)|(1<<CS00);
            }
        }
    }
    // set new speed and direction
    MOTOR_Dir = direction;
}


/*!
 *******************************************************************************
 * check if motor is blocked 
 *
 * \note stops the motor if blocked
 *
 * \note checks if \ref MOTOR_PosAct == \ref MOTOR_PosLast
 *
 * \note must be called at least every 1s <BR>
 *       not quicker than 500ms
 ******************************************************************************/
void MOTOR_CheckBlocked(void){
    // blocked if last position == actual position
    if (MOTOR_PosAct == MOTOR_PosLast){
        MOTOR_Control(stop, full);
        // Send out notify to com.c
      	COM_setNotify(NOTIFY_MOTOR_BLOCKED);
    } else {
        MOTOR_PosLast = MOTOR_PosAct;
    }
}


/*!
 *******************************************************************************
 * Pinchange Interupt INT0
 *
 * \note count light eye impulss: \ref MOTOR_PosAct
 *
 * \note stops the motor if \ref MOTOR_PosStop is reached
 ******************************************************************************/
ISR (PCINT0_vect){
    // count only on HIGH impulses
    if (((PINE & (1<<PE4)) != 0)) {
        if (MOTOR_Dir == open) {
            MOTOR_PosAct++;
            if (!(MOTOR_PosAct < MOTOR_PosStop)){
                MOTOR_Control(stop, full);
            }
        } else {
            MOTOR_PosAct--;
            if (!(MOTOR_PosAct > MOTOR_PosStop)){
                MOTOR_Control(stop, full);
            }
        }
    }
}

