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
 * \date       $Date$
 * $Rev$
 */

#pragma once

/*****************************************************************************
*   Macros
*****************************************************************************/


#define MOTOR_eye_enable() PORTE |= _BV(PE3);   // activate photo eye
#define MOTOR_eye_disable() PORTE &= ~_BV(PE3); // deactivate photo eye
#define MOTOR_eye_test() (PORTE & _BV(PE3))

// How is the H-Bridge connected to the AVR?

static inline void MOTOR_H_BRIDGE_open(void)
{
	PORTG = (1 << PG4);     // PG3 LOW, PG4 HIGH
#if THERMOTRONIC != 1           //not needed (no enable-Pin for motor)
	PORTB |= (1 << PB7);    // PB7 HIGH
#endif
}
static inline void MOTOR_H_BRIDGE_close(void)
{
	PORTG = (1 << PG3);     // PG3 HIGH, PG4 LOW
#if THERMOTRONIC != 1           //not needed (no enable-Pin for motor)
	PORTB &= ~(1 << PB7);   // PB7 LOW
#endif
}
static inline void MOTOR_H_BRIDGE_stop(void)
{
	PORTG = 0;              // PG3 LOW, PG4 LOW
#if THERMOTRONIC != 1           //not needed (no enable-Pin for motor)
	PORTB &= ~(1 << PB7);   // PB7 LOW
#endif
}

#define MOTOR_run_test() ((PORTG & ((1 << PG3) | (1 << PG4))) != 0)

//! How many photoeye impulses maximal form one endposition to the other. <BR>
//! The value measured on a HR20 are 737 to 740 = so more than 1000 should
//! never occure if it is mounted
#if THERMOTRONIC == 1
#define MOTOR_MAX_IMPULSES 500
#define MOTOR_MIN_IMPULSES 50
#else
#define MOTOR_MAX_IMPULSES 1000
#define MOTOR_MIN_IMPULSES 100
#endif
/* MOTOR_MAX_VALID_TIMER
 *  Minimum is 1024  (not recomended)
 *  Max is 65236
 *  value/15625 = time [seconds]
 */
#define MOTOR_MAX_VALID_TIMER 20000

#define MOTOR_IGNORE_IMPULSES       2
#define DEFAULT_motor_max_time_for_impulse 3072
#define DEFAULT_motor_eye_noise_protection 120

/*****************************************************************************
*   Typedefs
*****************************************************************************/
//! motor direction
typedef enum
{
	close = -1, stop = 0, open = 1
} motor_dir_t;

/*****************************************************************************
*   Prototypes
*****************************************************************************/
#define MOTOR_Init(void) (MOTOR_updateCalibration(1))           // Init motor control
void MOTOR_Goto(uint8_t);                                       // Goto position in percent
#define MOTOR_IsCalibrated() (MOTOR_calibration_step == 0)      // is motor successful calibrated?
void MOTOR_updateCalibration(uint8_t cal_type);                 // reset the calibration
uint8_t MOTOR_GetPosPercent(void);                              // get percental position of motor (0-100%)
void MOTOR_timer_stop(void);
void MOTOR_timer_pulse(void);
void MOTOR_interrupt(uint8_t pine);

#define timer0_need_clock() (TCCR0A & ((1 << CS02) | (1 << CS01) | (1 << CS00)))

extern volatile int16_t MOTOR_PosAct;
extern volatile uint16_t motor_diag;
extern int8_t MOTOR_calibration_step;
extern uint16_t motor_diag_count;
extern motor_dir_t MOTOR_Dir;           //!< actual direction
extern volatile uint8_t MOTOR_PosOvershoot;
extern uint32_t MOTOR_counter;          //!< count volume of motor pulses for dianostic
