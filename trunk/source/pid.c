/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  compiler:   WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Jiri Dobry (jdobry-at-centrum-dot-cz)
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
 * \file       pid.c
 * \brief      optimized version of PID controler for free HR20E project
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>, inspired from AVR221
 * \date       $Date$
 * $Rev$
 */
 
#include <stdint.h>
#include <stdlib.h>
#include "main.h"
#include "rtc.h"
#include "eeprom.h"
#include "pid.h"

// Maximum value of variables
#define MAX_INT         INT16_MAX
#define MAX_LONG        INT32_MAX
#define MAX_I_TERM      (MAX_LONG / 2)


//! Last process value, used to find derivative of process value.
#if CONFIG_ENABLE_D
static int16_t lastProcessValue;
#endif
//! Summation of errors, used for integrate calculations
int32_t sumError=0;
//! The Proportional tuning constant, multiplied with scalling_factor
#define P_Factor (config.P_Factor)
//! The Integral tuning constant, multiplied with scalling_factor
#define I_Factor (config.I_Factor)
//! The Derivative tuning constant, multiplied with scalling_factor
#define D_Factor (config.D_Factor)
//! The scalling_factor for PID constants
#define scalling_factor  (config.scalling_factor)

//! Maximum allowed error, avoid overflow
static int16_t maxError=0;
//! Maximum allowed sumerror, avoid overflow
int16_t maxSumError;


void pid_Init( int16_t processValue)
// Set up PID controller parameters
{
#if CONFIG_ENABLE_D
  lastProcessValue =  processValue;
#endif
  // Limits to avoid overflow
  maxError = MAX_INT / (P_Factor + 1);
  if (I_Factor == 0) {
      maxSumError = MAX_INT;
  } else {
      maxSumError = ((int16_t)scalling_factor*50)/I_Factor;
  }
}


/*! \brief non-linear  PID control algorithm.
 *
 *  Calculates output from setpoint, process value and PID status.
 *
 *  \param setPoint  Desired value.
 *  \param processValue  Measured value.
 */
int8_t pid_Controller(int16_t setPoint, int16_t processValue, int8_t old_result)
{
  int32_t error32, pi_term, ret;
  int16_t error16;
#if CONFIG_ENABLE_D
  int32_t d_term,
#endif
  error16 = setPoint - processValue;

  // Calculate Iterm and limit integral runaway  
  if (abs(error16)<((int16_t)scalling_factor*50/P_Factor)) { 
      // update sumError only for error < limit of P
      sumError += error16;
  }
  if(sumError > maxSumError){
    sumError = maxSumError;
  } else if(sumError < -maxSumError){
    sumError = -maxSumError;
  }
  pi_term = I_Factor * sumError;

  error32 = (int32_t)error16 * (int32_t)abs(error16); // non linear P characteristic 
  error32 /= 100L; // P gain

  // Calculate Pterm and limit error overflow
  if (error32 > maxError){
    pi_term += MAX_INT;
  } else if (error32 < -maxError){
    pi_term += -MAX_INT;
  } else{
    pi_term += (P_Factor * error32);
  }

#if CONFIG_ENABLE_D
  // Calculate Dterm
  
  d_term = lastProcessValue - processValue;
  d_term *= labs(d_term); // non linear characteristic 
  d_term /= scalling_factor;
  d_term *= D_Factor;
  lastProcessValue = processValue;

  ret = pi_term + d_term;
#else
  ret = pi_term;
#endif

  if (labs(ret-((int32_t)old_result*scalling_factor))<config.pid_hysteresis) return old_result;
  
  ret /= scalling_factor;

  if(ret > 50){
    return 50;
  } else if(ret < -50){
    return -50; 
  }
  return((int8_t)ret);
}
