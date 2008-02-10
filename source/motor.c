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

// typedefs


// vars
volatile uint8_t MOTOR_PositionPercent;   // percental position of motor
                                           //   0=closed 100=Open 255:not calibrated

volatile uint16_t MOTOR_PositionAbsolute;  // absolute pos. in lighteye impulses
                                           //   (0=closed)

volatile motor_mode_t MOTOR_mode;          // actual mode of motor

uint16_t MOTOR_ImpulsesPerPercent;         // lighteye impulses for one percent
                                           //   (0 if not calibrated)

uint16_t MOTOR_ImpulseMax;                 // max position in lighteye impulses
                                           //   (when opened)


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
    MOTOR_PositionPercent = 255;       // not calibrated
    MOTOR_PositionAbsolute = 0;        // absolute position 
    MOTOR_mode = stop;                 // actual mode of motor
    MOTOR_ImpulsesPerPercent = 0;      // not calibrated
    MOTOR_ImpulseMax = 0;              // not calibrated
    
    MOTOR_Control(stop);               // stop motor
}


/*****************************************************************************
*
*   function:       MOTOR_GetPosition
*
*   returns:        actual position of motor in percent
*
*   parameters:     None
*
*   purpose:        read actual position of motor
*
*   global vars:    MOTOR_PositionPercent
*
*****************************************************************************/
uint8_t MOTOR_GetPosition(void)
{
    return MOTOR_PositionPercent;
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
    MOTOR_Control(stop);
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
*
*                   - mode       PG3  PG4  PB7  PB4/PWM(OC0A)
*                       stop:    0    0    0    0                STOP TIMER 0
*                       open:    1    0    0   (44.5us:19.5us) = 178 non inv mode
*                       close:   0    1    1   (19.5us:44.5us) = 178 invert. mode
*
*                   - duty cycle = MOTOR_FULL_PWM   if open or close
*                                MOTOR_QUIET_PWM  if open_quiet or close_quiet
*
*                   - PWM @ 15,625 kHz
*
*   global vars:    MOTOR_mode
*
*****************************************************************************/
void MOTOR_Control(motor_mode_t mode)
{
    if (mode == stop){                                              // motor off
        // set all pins of H-Bridge to LOW
        MOTOR_HR20_PG3_P &= ~(1<<MOTOR_HR20_PG3);       // PG3 LOW
        MOTOR_HR20_PG4_P &= ~(1<<MOTOR_HR20_PG4);       // PG4 LOW
        MOTOR_HR20_PB7_P &= ~(1<<MOTOR_HR20_PB7);       // PB7 LOW
        // stop pwm signal
        TCCR0A = (1<<WGM00) | (1<<WGM01); // 0b 0000 0011
        // switch off photo eye
        MOTOR_HR20_PE3_P &= ~(1<<MOTOR_HR20_PE3);
    } else {                                                        // motor on
        // switch on photo eye
        MOTOR_HR20_PE3_P |= (1<<MOTOR_HR20_PE3);
        // set pwm value to percentage ( factor 255/100)
        if ((mode == open_quiet) || (mode == close_quiet)){
            OCR0A = MOTOR_QUIET_PWM * 2.55;
        } else {
            OCR0A = MOTOR_FULL_PWM * 2.55;
        }
        if ( (mode == open)  || (mode == open_quiet) ){             // open
            // set pins of H-Bridge
            MOTOR_HR20_PG3_P |=  (1<<MOTOR_HR20_PG3);   // PG3 HIGH
            MOTOR_HR20_PG4_P &= ~(1<<MOTOR_HR20_PG4);   // PG4 LOW
            MOTOR_HR20_PB7_P &= ~(1<<MOTOR_HR20_PB7);   // PB7 LOW
            // set PWM non inverting mode
            TCCR0A = (1<<WGM00) | (1<<WGM01) | (1<<COM0A1) | (1<<CS00);
        } else if ( (mode == close) || (mode == close_quiet)){      // close
            // set pins of H-Bridge
            MOTOR_HR20_PG3_P &= ~(1<<MOTOR_HR20_PG3);   // PG3 LOW
            MOTOR_HR20_PG4_P |=  (1<<MOTOR_HR20_PG4);   // PG4 HIGH
            MOTOR_HR20_PB7_P |=  (1<<MOTOR_HR20_PB7);   // PB7 HIGH
            // set PWM inverting mode
            TCCR0A = (1<<WGM00) | (1<<WGM01) | (1<<COM0A1) | (1<<COM0A0) | (1<<CS00);
        }
    }
    // set new mode
    MOTOR_mode = mode;
}
