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
 * \file       motor.h
 * \brief      header file for motor.c, functions to control the HR20 motor
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>
 * \date       07.02.2008
 * $Rev$
 */


/*****************************************************************************
*   Macros
*****************************************************************************/

// How is the H-Bridge connected to the AVR?
#define MOTOR_HR20_PB4     PB4     //!< if PWM output != OC0A you have a problem
#define MOTOR_HR20_PB4_P   PORTB   //!< Port for PWM output
#define MOTOR_HR20_PB7     PB7     //!< change this for other hardware e.g. HR40
#define MOTOR_HR20_PB7_P   PORTB   //!< change this for other hardware e.g. HR40
#define MOTOR_HR20_PG3     PG3     //!< change this for other hardware e.g. HR40
#define MOTOR_HR20_PG3_P   PORTG   //!< change this for other hardware e.g. HR40
#define MOTOR_HR20_PG4     PG4     //!< change this for other hardware e.g. HR40
#define MOTOR_HR20_PG4_P   PORTG   //!< change this for other hardware e.g. HR40

#define MOTOR_HR20_PE3     PE3     //!< HR20: pin to activate photo eye
#define MOTOR_HR20_PE3_P   PORTE   //!< HR20: port to activate photo eye

#define MOTOR_HR20_PE_IN   PE4     //!< HR20: input pin from photo eye
#define MOTOR_HR20_PE_IN_P PINE    //!< HR20: input port from photo eye

//! How many photoeye impulses maximal form one endposition to the other. <BR>
//! The value measured on a HR20 are 737 to 740 = so more than 1000 should
//! never occure if it is mounted
#define	MOTOR_MAX_IMPULSES 1000

#define EYE_TIMER_NOISE_PROTECTION 5 


/*****************************************************************************
*   Typedefs
*****************************************************************************/
//! motor direction
typedef enum {                                      
    stop, open, close
} motor_dir_t;

/*****************************************************************************
*   Prototypes
*****************************************************************************/
void MOTOR_Init(void);                        // Init motor control
bool MOTOR_Goto(uint8_t);                     // Goto position in percent
void MOTOR_CheckBlocked(void);                // stop if motor is blocked
bool MOTOR_On(void);                          // is motor on
void MOTOR_Calibrate(uint8_t percent);        // calibrate the motor
bool MOTOR_IsCalibrated(void);                // is motor successful calibrated?
void MOTOR_updateCalibration(bool unmounted, uint8_t percent);            // reset the calibration 
void MOTOR_Control(motor_dir_t); // control H-bridge of motor
void MOTOR_SetMountStatus(bool);     // set status if is mounted or not
uint8_t MOTOR_GetPosPercent(void);  // get percental position of motor (0-100%)
void MOTOR_update_pos(void);
void MOTOR_timer(void);

extern motor_dir_t MOTOR_Dir;
