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
 * \brief      optimized version of the PID controller for free HR20E project
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>, inspired from AVR221
 * \date       $Date$
 * $Rev$
 */

#include <stdint.h>
#include <stdlib.h>
#include "main.h"
#include "../common/rtc.h"
#include "eeprom.h"
#include "pid.h"

// Maximum value of variables
#define MAX_INT         INT16_MAX

//! Last process value, used to find derivative of process value.
static int16_t lastProcessValue;

//! Summation of errors, used for integrate calculations
int16_t sumError=0;
//! The Proportional tuning constant, multiplied with scalling_factor
#define P_Factor (config.P_Factor)
//! The Integral tuning constant, multiplied with scalling_factor
#define I_Factor (config.I_Factor)
//! The Derivative tuning constant, multiplied with scalling_factor
#define D_Factor (config.D_Factor)
//! The scalling_factor for PID constants \TODO scalling > scaling
#define scalling_factor  (config.scalling_factor)

#if CONFIG_ENABLE_D
void pid_Init( int16_t processValue)
// Set up PID controller parameters
{
  lastProcessValue =  processValue;
}
#endif


/*! \brief non-linear  PID control algorithm.
 *
 *  Calculates output from setpoint, process value and PID status.
 *
 *  \param setPoint  Desired value.
 *  \param processValue  Measured value.
 */
int8_t pid_Controller(int16_t setPoint, int16_t processValue, int8_t old_result)
{
  int32_t error32, pi_term;
  int16_t error16, maxSumError;
#if CONFIG_ENABLE_D
  int32_t d_term,
#endif
  error16 = setPoint - processValue;
  
  // maximum error is 40 degree C
  if (error16 > 4000) { 
    error16=4000;
  } else if (error16 < -4000) {
    error16=-4000;
  }

  // Calculate Iterm and limit integral runaway  
  {
    int16_t d = lastProcessValue - processValue;
    if (((d>0)&&(error16>0)) || ((d<0)&&(error16<0))) {
        // update sumError if turn is wrong only 
        int16_t max = (int16_t)scalling_factor*config.P_max/P_Factor;
        if (error16 > max) {  // maximum sumError change + limmiter
          sumError += max;  
        } else if (error16 < -max) { // maximum sumError change - limmiter
          sumError -= max;  
        } else {
          sumError += error16;
        }
    }
  }
  if (I_Factor == 0) {
      maxSumError = 12750; // 255*50/1
  } else {
      // for overload protection: maximum is 255*50/1 = 12750
      maxSumError = ((int16_t)scalling_factor*50)/I_Factor;
  }
  if(sumError > maxSumError){
    sumError = maxSumError;
  } else if(sumError < -maxSumError){
    sumError = -maxSumError;
  }

  error32 = (int32_t)error16 * (int32_t)abs(error16); // non linear P characteristic 
  error32 /= 100L; // P gain
  // error32 -> for overload limit: maximum is +-(4000*4000/200) = +-160000

  // Calculate Pterm
  pi_term = (P_Factor * error32);
  { 
    uint16_t max_p_term = (uint16_t)scalling_factor * (uint16_t)(config.P_max);
    if (pi_term > max_p_term) { pi_term = max_p_term; }
    else if (pi_term < -(int32_t)max_p_term) { pi_term = -(int32_t)max_p_term; }
  }
  // pi_term - > for overload limit: maximum is +-(40800000+3251250) = +-44051250
  
  pi_term += I_Factor * sumError;
  // pi_term - > for overload limit: maximum is +- 255*12750 = 3251250
   

#if CONFIG_ENABLE_D
  // Calculate Dterm
  d_term = lastProcessValue - processValue;
  d_term *= labs(d_term); // non linear characteristic 
  d_term /= scalling_factor;
  d_term *= D_Factor;
  pi_term += d_term;
#endif
  lastProcessValue = processValue;

  if (labs(pi_term-((int32_t)old_result*scalling_factor))<config.pid_hysteresis) return old_result;
  
  pi_term /= scalling_factor;

  if(pi_term > 50){
    return 50;
  } else if(pi_term < -50){
    return -50; 
  }
  return((int8_t)pi_term);
}
