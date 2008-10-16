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
 * \date       02.10.2008
 * $Rev$
 */
 
#include <stdint.h>
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
static int32_t maxSumError;


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
  maxSumError = ((int32_t)maxError*(int32_t)P_Factor)/(int32_t)I_Factor;
}


/*! \brief PID control algorithm.
 *
 *  Calculates output from setpoint, process value and PID status.
 *
 *  \param setPoint  Desired value.
 *  \param processValue  Measured value.
 */
int8_t pid_Controller(int16_t setPoint, int16_t processValue)
{
  int16_t error, p_term, d_term;
  int32_t i_term, ret;

  error = setPoint - processValue;

  // Calculate Pterm and limit error overflow
  if (error > maxError){
    p_term = MAX_INT;
  } else if (error < -maxError){
    p_term = -MAX_INT;
  } else{
    p_term = P_Factor * error;
  }

  // Calculate Iterm and limit integral runaway
  sumError += error;
  if(sumError > maxSumError){
    sumError = maxSumError;
  } else if(sumError < -maxSumError){
    sumError = -maxSumError;
  }
  i_term = I_Factor * sumError;

  // Calculate Dterm
  d_term = D_Factor * (lastProcessValue - processValue);
  lastProcessValue = processValue;

  ret = (p_term + i_term + d_term) / scalling_factor;

  if(ret > 100 /* MAX_INT */){
    ret = 100; //MAX_INT;
  } else if(ret < 0 /*-MAX_INT*/){
    ret = 0; //-MAX_INT;
  }
  return((int8_t)ret);
}
