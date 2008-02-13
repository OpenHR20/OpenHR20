//***************************************************************************
//
//  File........: motor.c
//
//  Author(s)...:
//
//  Target(s)...: ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
//
//  Compiler....: WinAVR-20071221
//                avr-libc 1.6.0
//                GCC 4.2.2
//
//  Description.: Functions used to control the motor
//
//
//  Revisions...:
//
//  YYYYMMDD - VER. - COMMENT                                     - SIGN.
//
//  20080208   0.0    created                                     - D.Carluccio
//
//***************************************************************************

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

// typedefs


// vars
volatile uint16_t MOTOR_PosAct;      // actual position
volatile uint16_t MOTOR_PosMax;      // position if complete open (100%) if 0 not calibrated
volatile uint16_t MOTOR_PosStop;     // stop at this position
volatile uint16_t MOTOR_PosLast;     // last position for check if blocked
volatile motor_dir_t MOTOR_Dir;      // actual direction

// prototypes


// constants

/*****************************************************************************
*
*   function:       MOTOR_Init
*
*   returns:        none
*
*   parameters:     none
*
*   purpose:        init 
*                   - configure pwm
*                   - stop motor
*
*   global vars:    
*
*****************************************************************************/
void MOTOR_Init(void)
{
    MOTOR_PosAct=0;                  // not calibrated
    MOTOR_PosMax=0;                  // not calibrated
    MOTOR_PosStop=0;                 // not calibrated
    MOTOR_Control(stop, full);       // stop motor
}


/*****************************************************************************
*
*   function:       MOTOR_ResetCalibration
*
*   returns:        none
*
*   parameters:     none
*
*   purpose:        reset calibration data
*
*   global vars:    MOTOR_PosAct (w)
*                   MOTOR_PosMax (w)
*                   MOTOR_PosStop (w)
*
*****************************************************************************/
void MOTOR_ResetCalibration(void)
{
    MOTOR_PosAct=0;                  // not calibrated
    MOTOR_PosMax=0;                  // not calibrated
    MOTOR_PosStop=0;                 // not calibrated
    MOTOR_Control(stop, full);       // stop motor
}


/*****************************************************************************
*
*   function:       MOTOR_GetPosPercent
*
*   returns:        actual position of motor in percent
*
*   parameters:     None
*
*   purpose:        read actual position of motor
*
*   global vars:    MOTOR_PosMax (r)
*                   MOTOR_PosAct (r)
*
*****************************************************************************/
uint8_t MOTOR_GetPosPercent(void)
{
    if (MOTOR_PosMax > 10){
        return (uint8_t) ( (MOTOR_PosAct * 10) / (MOTOR_PosMax/10) );
    }
}

/*****************************************************************************
*
*   function:       MOTOR_GetPosAbs
*
*   returns:        actual position of motor in percent
*
*   parameters:     None
*
*   purpose:        read actual position of motor
*
*   global vars:    MOTOR_PosAct (r)
*
*****************************************************************************/
uint16_t MOTOR_GetPosAbs(void)
{
    return MOTOR_PosAct;

}


/*****************************************************************************
*
*   function:       MOTOR_On
*
*   returns:        true if motor is on
*
*   parameters:     None
*
*   purpose:        get state of motor
*
*   global vars:    MOTOR_Dir (r)
*
*****************************************************************************/
bool MOTOR_On(void)
{
    return (MOTOR_Dir != stop);
}


/*****************************************************************************
*
*   function:       MOTOR_IsCalibrated
*
*   returns:        true calibration finished succesfully
*
*   parameters:     None
*
*   purpose:        get state of calibration
*
*   global vars:    MOTOR_PosMax (r)
*
*****************************************************************************/
bool MOTOR_IsCalibrated(void)
{
    return (MOTOR_PosMax != 0);
}


/*****************************************************************************
*
*   function:       MOTOR_Goto
*
*   returns:        true if motor started
*                   false if not calibrated
*
*   parameters:     percental value 0-100 (0:closed - 100:open)
*
*   purpose:        drive motor to desired position in percent
*
*   global vars:    MOTOR_PosAct   (w)
*                   MOTOR_PosStop  (w)
*                   MOTOR_PosMax   (r)
*
*   remarks:        works only if calibrated before
*
*****************************************************************************/
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
        return true;
    } else {
        return false;
    }
}


/*****************************************************************************
*
*   function:       MOTOR_Calibrate
*
*   returns:        true if OK
*                   false if motor is not stopped after MOTOR_MAX_IMPULSES impulses
*
*   parameters:     percent: position after calibration 0-100 (closed-open)
*                   speed:   motor speed
*
*   purpose:        calibrate the motor and drive it to position in percent
*                   needs position to minimise motor time (save power)
*
*   global vars:    MOTOR_PosAct
*                   MOTOR_PosMax
*                   MOTOR_PosStop
*                   MOTOR_Dir
*
*****************************************************************************/
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
        } while ((MOTOR_Dir != stop) && (postmp != MOTOR_PosAct)); // motor still on and moving
        // motor stopped by ISR? -> MOTOR_MAX_IMPULSES -> error
        if (MOTOR_Dir == stop){
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
        } while ((MOTOR_Dir != stop) && (postmp != MOTOR_PosAct)); // motor still on and moving
        // motor stopped by ISR? -> MOTOR_MAX_IMPULSES -> error
        if (MOTOR_Dir == stop){
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
        } while ((MOTOR_Dir != stop) && (postmp != MOTOR_PosAct)); // motor still on and moving
        // motor stopped by ISR? -> MOTOR_MAX_IMPULSES -> error
        if (MOTOR_Dir == stop){
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
        } while ((MOTOR_Dir != stop) && (postmp != MOTOR_PosAct)); // motor still on and moving
        // motor stopped by ISR? -> MOTOR_MAX_IMPULSES -> error
        if (MOTOR_Dir == stop){
            return false;
        }
        // motor is on, but not turning any more -> endposition reached -> stop the motor
        MOTOR_Control(stop, speed);
        MOTOR_PosMax = MOTOR_MAX_IMPULSES - MOTOR_PosAct;
        MOTOR_PosAct = 0;
    }
    // now MOTOR_PosMax and MOTOR_PosAct calibated
    // goto desired position
    // MOTOR_Goto(percent, speed);
    return true;
}


/*****************************************************************************
*
*   function:       MOTOR_Stop
*
*   returns:        none
*
*   parameters:     none
*
*   purpose:        stop motor
*
*   global vars:
*
*****************************************************************************/
void MOTOR_Stop(void)
{
    MOTOR_Control(stop, full);
}


/*****************************************************************************
*
*   function:       MOTOR_Control
*
*   returns:        none
*
*   parameters:     none
*
*   purpose:        control motor
*                                                                  lightexe
*                   - direction  PG3  PG4  PB7  PB4/PWM(OC0A)    PE3    PCINT4
*                       stop:     0    0    0    0                0      off
*                       open:     0    1    1   invert. mode      1      on
*                      close:     1    0    0   non inv mode      1      on
*
*                   - duty cycle = full:    (MOTOR_FULL_PWM  /100) * 255
*                                  quiet:   (MOTOR_QUIET_PWM /100) * 255
*
*                   - PWM @ 15,625 kHz
*
*   global vars:    MOTOR_Dir     (w)
*                   MOTOR_PosLast (w)
*
*****************************************************************************/
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
        // photo eye
        MOTOR_HR20_PE3_P |= (1<<MOTOR_HR20_PE3);        // activate photo eye
        PCMSK0 = (1<<PCINT4);                           // activate interrupt
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
            TCCR0A = (1<<WGM00) | (1<<WGM01) | (1<<COM0A1) | (1<<COM0A0) | (1<<CS00);
        }
    }
    // set new speed and direction
    MOTOR_Dir = direction;
}


/*****************************************************************************
*
*   Function name:  MOTOR_CheckBlocked
*
*   returns:        none
*
*   parameters:     none
*
*   purpose:        check if motor is blocked (MOTOR_PosAct == MOTOR_PosLast)
*                   stops the motor if blocked
*
*   global vars:    MOTOR_PosAct   (r)
*                   MOTOR_PosLast  (w)
*
*   remarks:        must be called every 500ms not quicker
*
*****************************************************************************/
void MOTOR_CheckBlocked(void){
    // blocked if last position == actual position
    if (MOTOR_PosAct == MOTOR_PosLast){
        MOTOR_Control(stop, full);
    } else {
        MOTOR_PosLast = MOTOR_PosAct;
    }
}


/*****************************************************************************
*
*   Function name:  ISR(PCINT0_vect)
*
*   returns:        None
*
*   parameters:     Pinchange Interupt INT0
*
*   purpose:        count light eye impulss: MOTOR_PosAct
*                   stops the motor if MOTOR_PosEnd is reached
*
*   global vars:    MOTOR_PosAct
*                   MOTOR_PosStop
*                   MOTOR_Dir
*
*****************************************************************************/
ISR (PCINT0_vect){
    // count only on HIGH impulses
    if (((PINE & (1<<PE4)) != 0)) {
        if (MOTOR_Dir==open){
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

