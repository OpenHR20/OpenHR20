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
 * \file       rtc.h
 * \brief      header file for rtc.c, to control the HR20 clock and timers
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>
 * \date       01.02.2008
 * $Rev: 35 $
 */


/*****************************************************************************
*   Macros
*****************************************************************************/

//! How many timers per day of week, original 4 (2 on and 2 off)
#define RTC_TIMERS_PER_DOW    4

//! Do we support calibrate_rco
#define	HAS_CALIBRATE_RCO     0

/*****************************************************************************
*   Typedefs
*****************************************************************************/

//! day of week
typedef enum {
    MO, TU, WE, TH, FR, SA, SU, TIMER_ACTIVE
} rtc_dow_t;

//! DOW timer callback function
typedef void (*rtc_callback_t) (uint8_t);

/*****************************************************************************
*   Prototypes
*****************************************************************************/

void RTC_Init(rtc_callback_t);                 // init Timer, activate 500ms IRQ
uint8_t RTC_GetHour(void);                     // get hour
uint8_t RTC_GetMinute(void);                   // get minute
uint8_t RTC_GetSecond(void);                   // get second
uint8_t RTC_GetDay(void);                      // get day
uint8_t RTC_GetMonth(void);                    // get month
uint8_t RTC_GetYearYY(void);                   // get year (00-255)
uint8_t RTC_GetDayOfWeek(void);                // get day of week (0:monday)
uint16_t RTC_GetYearYYYY(void);                // get year (2000-2255)

void RTC_SetHour(uint8_t);                     // Set hour
void RTC_SetMinute(uint8_t);                   // Set minute
void RTC_SetSecond(uint8_t);                   // Set second
void RTC_SetDay(uint8_t);                      // Set day
void RTC_SetMonth(uint8_t);                    // Set month
void RTC_SetYear(uint8_t);                     // Set year

void    RTC_DowTimerSet(rtc_dow_t, uint8_t, uint8_t); // set day of week timer
uint8_t RTC_DowTimerGet(rtc_dow_t, uint8_t);          // get day of week timer
uint8_t RTC_DowTimerGetStartOfDay(void);              // timer status at 0:00
uint8_t RTC_DowTimerGetActualIndex(void);             // timer status now

#if	HAS_CALIBRATE_RCO
void calibrate_rco(void);
#endif


