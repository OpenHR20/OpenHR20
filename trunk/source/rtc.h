/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  compiler:    WinAVR-20071221
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
 * \file       rtc.h
 * \brief      header file for rtc.c, to control the HR20 clock and timers
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>
 * \date       $Date$
 * $Rev$
 */

#ifndef RTC_H
#define RTC_H

/*****************************************************************************
*   Macros
*****************************************************************************/

//! How many timers per day of week, original 4 we use 8
#define RTC_TIMERS_PER_DOW    8

//! Do we support calibrate_rco
#define	HAS_CALIBRATE_RCO     0

/*****************************************************************************
*   Typedefs
*****************************************************************************/

//! day of week
typedef enum {
   COMPLETE_WEEK, MO, TU, WE, TH, FR, SA, SU, 
} rtc_dow_t;

//! 
typedef enum {
    temperature0=0, temperature1=1, temperature2=2, temperature3=3
} timermode_t;

/*****************************************************************************
*   Prototypes
*****************************************************************************/
extern uint8_t RTC_hh;   //!< \brief Time: Hours
extern uint8_t RTC_mm;   //!< \brief Time: Minutes
extern uint8_t RTC_ss;   //!< \brief Time: Seconds
extern uint8_t RTC_DD;   //!< \brief Date: Day
extern uint8_t RTC_MM;   //!< \brief Date: Month
extern uint8_t RTC_YY;   //!< \brief Date: Year (0-255) -> 2000 - 2255
extern uint8_t RTC_DOW;  //!< Date: Day of Week
extern uint32_t RTC_Ticks; //!< Ticks since last RTC.Init

void RTC_Init(void);                 // init Timer, activate 500ms IRQ
#define RTC_GetHour() ((uint8_t) RTC_hh)                 // get hour
#define RTC_GetMinute() ((uint8_t) RTC_mm)               // get minute
#define RTC_GetSecond() ((uint8_t) RTC_ss)               // get second
#define RTC_GetDay() ((uint8_t) RTC_DD)                  // get day
#define RTC_GetMonth() ((uint8_t) RTC_MM)                // get month
#define RTC_GetYearYY() ((uint8_t) RTC_YY)               // get year (00-255)
#define RTC_GetYearYYYY() (2000 + (uint16_t) RTC_YY)  // get year (2000-2255) 
#define RTC_GetDayOfWeek() ((uint8_t) RTC_DOW)          // get day of week (0:monday)
#define RTC_GetTicks() ((uint32_t) RTC_Ticks)          // 1s ticks from startup


void RTC_SetHour(int8_t);                     // Set hour
void RTC_SetMinute(int8_t);                    // Set minute
void RTC_SetSecond(int8_t);                   // Set second
void RTC_SetDay(int8_t);                      // Set day
void RTC_SetMonth(int8_t);                    // Set month
void RTC_SetYear(uint8_t);                     // Set year
#if 0
    bool RTC_SetDate(int8_t, int8_t, int8_t);   // Set Date, and do all the range checking
#else
    #define RTC_SetDate(d,m,y) (RTC_SetYear(y),RTC_SetMonth(m),RTC_SetDay(d))
#endif

bool RTC_DowTimerSet(rtc_dow_t, uint8_t, uint16_t, timermode_t timermode); // set day of week timer
uint16_t RTC_DowTimerGet(rtc_dow_t dow, uint8_t slot, timermode_t *timermode);
uint8_t RTC_ActualTimerTemperature(bool exact);
int32_t RTC_DowTimerGetHourBar(uint8_t dow);
bool RTC_AddOneSecond(void);

#define TCCR2A_INIT ((1<<CS22) | (1<<CS20))     // select precaler: 32.768 kHz / 128 =
                                        // => 1 sec between each overflow

#if	HAS_CALIBRATE_RCO
void calibrate_rco(void);
#endif

#endif /* RTC_H */
