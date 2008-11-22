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
static int16_t lastProcessValue;
//! Summation of errors, used for integrate calculations
int32_t sumError;
//! The Proportional tuning constant, multiplied with scalling_factor
#define P_Factor (config.P_Factor)
//! The Integral tuning constant, multiplied with scalling_factor
#define I_Factor (config.I_Factor)
//! The Derivative tuning constant, multiplied with scalling_factor
#define D_Factor (config.D_Factor)
//! The scalling_factor for PID constants
#define scalling_factor  (config.scalling_factor)

//! Maximum allowed error, avoid overflow
static int16_t maxError;
//! Maximum allowed sumerror, avoid overflow
int16_t maxSumError;


void pid_Init( int16_t processValue)
// Set up PID controller parameters
{
  // Start values for PID controller
  sumError = 0;
  lastProcessValue =  processValue;
  // Limits to avoid overflow
  maxError = MAX_INT / (P_Factor + 1);
  // not recomended to use => maxSumError = MAX_I_TERM / (I_Factor + 1);
  // used limitation => P*maxError == I*maxSumError;
  // maxSumError = ((int32_t)maxError*(int32_t)P_Factor)/(int32_t)I_Factor;
  if (I_Factor == 0) {
    maxSumError = MAX_INT;
  } else {
    maxSumError = ((int16_t)scalling_factor * (int16_t)config.max_I_effect) / (int16_t)I_Factor;
  }
}


/*! \brief non-linear  PID control algorithm.
 *
 *  Calculates output from setpoint, process value and PID status.
 *
 *  \param setPoint  Desired value.
 *  \param processValue  Measured value.
 */
int8_t pid_Controller(int16_t setPoint, int16_t processValue)
{
  int32_t error, d_term, pi_term, ret;

  error = setPoint - processValue;
  error *= labs(error); // non linear characteristic 
  error /= scalling_factor;

  // Calculate Pterm and limit error overflow
  if (error > maxError){
    pi_term = MAX_INT;
  } else if (error < -maxError){
    pi_term = -MAX_INT;
  } else{
    pi_term = (P_Factor * error);
  }

  // Calculate Iterm and limit integral runaway
  sumError += error;
  if(sumError > maxSumError){
    sumError = maxSumError;
  } else if(sumError < -maxSumError){
    sumError = -maxSumError;
  }
  pi_term += I_Factor * sumError;

  // Calculate Dterm
  
  d_term = lastProcessValue - processValue;
  d_term *= labs(d_term); // non linear characteristic 
  d_term /= scalling_factor;
  d_term *= D_Factor;
  lastProcessValue = processValue;

  ret = (pi_term + d_term) / scalling_factor;

  if(ret > 50){
    return 50;
  } else if(ret < -50){
    return -50; 
  }
  return((int8_t)ret);
}
