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
 * \file       rtc.c
 * \brief      functions to control clock and timers
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>
 * \date       01.02.2008
 * $Rev$
 */

// AVR LibC includes
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/version.h>


// HR20 Project includes
#include "main.h"
#include "rtc.h"
#include "com.h"

// Vars


volatile uint8_t RTC_hh;   //!< \brief Time: Hours
volatile uint8_t RTC_mm;   //!< \brief Time: Minutes
volatile uint8_t RTC_ss;   //!< \brief Time: Seconds
volatile uint8_t RTC_DD;   //!< \brief Date: Day
volatile uint8_t RTC_MM;   //!< \brief Date: Month
volatile uint8_t RTC_YY;   //!< \brief Date: Year (0-255) -> 2000 - 2255
volatile uint8_t RTC_DOW;  //!< Date: Day of Week

volatile uint8_t RTC_DS;     //!< Daylightsaving Flag
volatile uint32_t RTC_Ticks; //!< Ticks since last RTC.Init
volatile uint8_t RTC_Dow_Timer[7][RTC_TIMERS_PER_DOW];  //!< DOW Timer entrys
rtc_callback_t RTC_DowTimerCallbackFunc; //!< Timer callback function

// prototypes
void    RTC_AddOneSecond(void);     // add one second to actual time
void    RTC_AddOneDay(void);        // add one day to actual date
uint8_t RTC_NoLeapyear(void);       // is (RTC_YY) a leapyear?
uint8_t RTC_DaysOfMonth(void);      // how many days in (RTC_MM, RTC_YY)
void    RTC_SetDayOfWeek(void);     // calc day of week (RTC_DD, RTC_MM, RTC_YY)
bool    RTC_IsLastSunday(void);     // check actual date if last sun in mar/oct


// Progmem constants

//! day of month for each month from january to december
uint8_t RTC_DayOfMonthTablePrgMem[] PROGMEM =
{
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};


/*!
 *******************************************************************************
 *  Init RTC
 *
 *  \note
 *  - Start Timer/Counter2 in asynchronous operation using a 32.768kHz crystal
 *  - Set Date after Reset
 *  - set RTC_Ticks = 0
 *
 *  \param  pFunc   pointer to callback function for DOW timer
 ******************************************************************************/
void RTC_Init(rtc_callback_t pFunc)
{
    RTC_DowTimerCallbackFunc = pFunc;   // callback function for DOW Timer

    TIMSK2 &= ~(1<<TOIE2);              // disable OCIE2A and TOIE2
    ASSR = (1<<AS2);                    // Timer2 asynchronous operation
    TCNT2 = 0;                          // clear TCNT2A
    TCCR2A |= (1<<CS22) | (1<<CS20);    // select precaler: 32.768 kHz / 128 =
                                        // => 1 sec between each overflow

    // wait for TCN2UB and TCR2UB to be cleared
    while((ASSR & 0x01) | (ASSR & 0x04));   

    TIFR2 = 0xFF;                       // clear interrupt-flags
    TIMSK2 |= (1<<TOIE2);               // enable Timer2 overflow interrupt

    // Restart Tick Timer
    RTC_Ticks=0;
    // initial time 00:00:00
    RTC_hh = 0;
    RTC_mm = 0;
    RTC_ss = 0;
    // initial date 1.01.2008
    RTC_DD = 1;
    RTC_MM = 1;
    RTC_YY = 8;
    // day of week
    RTC_SetDayOfWeek();
}


/*!
 *******************************************************************************
 *  \returns actual Ticks since last RTC_Init
 ******************************************************************************/
uint32_t RTC_GetTicks(void)
{
    cli();
    uint32_t retval=RTC_Ticks;
    sei();
    return retval;
}


/*!
 *******************************************************************************
 *  set actual date
 *  \param day new value for day
 ******************************************************************************/
void RTC_SetDay(uint8_t day)
{
    if ((day > 0) && (day < 32)) {
        RTC_DD = day;
    }
    RTC_SetDayOfWeek();
}


/*!
 *******************************************************************************
 *  set actual date
 *  \param month new value for month
 ******************************************************************************/
void RTC_SetMonth(uint8_t month)
{
    if ((month > 0) && (month < 13)) {
        RTC_MM = month;
    }
    RTC_SetDayOfWeek();
}

/*!
 *******************************************************************************
 *  set actual date
 *  \param year new value for year
 ******************************************************************************/
void RTC_SetYear(uint8_t year)
{
    RTC_YY = year;
    RTC_SetDayOfWeek();
}

/*!
 *******************************************************************************
 *  set actual time
 *  \param hour new value for hour
 ******************************************************************************/
void RTC_SetHour(uint8_t hour)
{
    if (hour < 24) {
        RTC_hh = hour;
    }
}


/*!
 *******************************************************************************
 *  set actual time
 *  \param minute new value for minute
 ******************************************************************************/
void RTC_SetMinute(uint8_t minute)
{
    if (minute < 60) {
        RTC_mm = minute;
    }
}


/*!
 *******************************************************************************
 *  set actual time
 *  \param second new value for second
 ******************************************************************************/
void RTC_SetSecond(uint8_t second)
{
    if (second < 60) {
        RTC_ss = second;
    }
}

/*!
 *******************************************************************************
 *
 *  set timer for one timerslot for one weekday
 *
 *  \param dow day of week
 *  \param slot timeslot number
 *  \param time time
 *
 *  \note
 *      - maximum timers for each weekday with \ref RTC_TIMERS_PER_DOW
 *      - only 10 minute intervals supported: <BR>
 *        hh:m0 is coded in one byte: hh*10 + m <BR>
 *        e.g.: 12:00 = 120, 17:30 = 173, 22:10 = 221
 *      - to deactivate timer: set value 0xff
 *
 ******************************************************************************/
void RTC_DowTimerSet(rtc_dow_t dow, uint8_t slot, uint8_t time)
{
    RTC_Dow_Timer[dow][slot] = time;
}

/*!
 *******************************************************************************
 *
 *  get timer for one timerslot for one weekday
 *
 *  \param dow day of week
 *  \param slot timeslot number
 *  \returns switching time of dow-timer
 *
 *  \note
 *      - maximum timers for each weekday with \ref RTC_TIMERS_PER_DOW
 *      - only 10 minute intervals supported: <BR>
 *        hh:m0 is coded in one byte: hh*10 + m <BR>
 *        e.g.: 12:00 = 120, 17:30 = 173, 22:10 = 221
 *      - deactivates timer returns 0xff
 *
 ******************************************************************************/
uint8_t RTC_DowTimerGet(rtc_dow_t dow, uint8_t slot)
{
    return RTC_Dow_Timer[dow][slot];
}

/*!
 *******************************************************************************
 *
 *  get actual timer index
 *
 *  \returns timerslot index
 *
 *  \note
 *       what is the last occured timer index
 *       needed to calculate the actual valid timer index
 *
 ******************************************************************************/
uint8_t RTC_DowTimerGetActualIndex(void)
{
    uint8_t i;
    uint8_t index;
    uint8_t acttime;
    uint8_t maxtime;

    maxtime=0;
    acttime=(RTC_hh*10)+(RTC_mm/10);

    // Get index from this day at 00:00
    index = RTC_DowTimerGetStartOfDay();
    
    // each timer until now
    for (i=1; i<RTC_TIMERS_PER_DOW; i++){
        // check if timer > maxtime and timer < actual time
        if ((RTC_Dow_Timer[RTC_DOW][i] > maxtime)
            && !(RTC_Dow_Timer[RTC_DOW][i] > acttime)){
            maxtime = RTC_Dow_Timer[RTC_DOW][i];
            index=i;
        }
    }
    return index;
}


/*!
 *******************************************************************************
 *
 *  get timer index for previous day
 *
 *  \returns timerslot index
 *
 *  \note
 *        what was status at 00:00 of the actual day
 *        needed to calculate the actual valid timer index
 *
 ******************************************************************************/
uint8_t RTC_DowTimerGetStartOfDay(void)
{
    uint8_t dow;
    uint8_t day;
    uint8_t i;
    uint8_t index;
    uint8_t maxtime;

    maxtime=0;
    index=0xff;
    day=0;
    
    // if previous day has no timer set, do this for last 7 days
    while ( (index == 0xff) && (day < 7) ){
    
        // dow of previous day
        dow = (RTC_DOW+6)%7;
    
        // dow of previous day
        for (i=1; i<RTC_TIMERS_PER_DOW; i++){
            // check if > maxtime and (active (<240)
            if ((RTC_Dow_Timer[dow][i]>maxtime) && (RTC_Dow_Timer[dow][i]<240)){
                maxtime = RTC_Dow_Timer[RTC_DOW][i];
                index=i;
            }
        }
        day++;
    }
    return index;
}


/*!
 *******************************************************************************
 *
 *  Add one second to clock variables
 *
 *  \note
 *    - calculate overflows, regarding leapyear, etc.
 *    - process daylight saving
 *       - last sunday in march 1:59:59 -> 3:00:00
 *       - last sunday in october 2:59:59 -> 2:00:00 <BR>
 *         ONLY ONE TIME -> set FLAG RTC_DS <BR>
 *         reset FLAG RTC_DS on November 1st
 *
 ******************************************************************************/
void RTC_AddOneSecond(void)
{
    uint8_t i;
	if (++RTC_ss == 60) {
		RTC_ss = 0;
		// notify com.c about the changed minute
    COM_setNotify(NOTIFY_TIME);             
		if (++RTC_mm == 60) {
			RTC_mm = 0;
			// add ome hour
			if (++RTC_hh == 24) {
				RTC_hh = 0;
				RTC_AddOneDay();
			}
			// start of summertime: March, 2:00:00 ?
			if ((RTC_MM==3)&&(RTC_hh==2)){
                // Last Sunday ?
                if (RTC_IsLastSunday()){
                    RTC_hh++; // 2:00 -> 3:00
                }
            }
			// end of summertime: October, 03:00, RTC_DS == 0
			if ((RTC_MM==10)&&(RTC_hh==3)&&(RTC_DS==0)){
                // Last Sunday ?
                if (RTC_IsLastSunday()){
                    RTC_hh--; // 3:00 -> 2:00
                    RTC_DS=1;
                }
			}
        }
        // Dow_Timer check every 10 minutes
        if ((RTC_mm % 10) == 0){
            // RTC_DowTimerCallbackFunc(1); // to test callback
            for (i=0; i<RTC_TIMERS_PER_DOW; i++){
                if (RTC_Dow_Timer[RTC_DOW][i] == ((RTC_hh*10) + (RTC_mm/10))){
                    RTC_DowTimerCallbackFunc(i);
                }
            }
        }
 	}
}


/*!
 *******************************************************************************
 *
 *  Add one day to date variables
 *
 *  \note
 *       calculate overflows, regarding leapyear, etc.
 *
 ******************************************************************************/
void RTC_AddOneDay(void)
{
    uint8_t dom;
    // How many day has actual month
    dom = RTC_DaysOfMonth();
    if (++RTC_DD == (dom+1)) {                   // Next Month
		RTC_DD = 1;
		if (++RTC_MM == 13) {                    // Next year
			RTC_MM = 1;
			RTC_YY++;
		}
		// Clear Daylight saving Flag
		if (RTC_MM == 11) {                    
            RTC_DS=0;
        }
    // next day of week
    RTC_DOW++;
    RTC_DOW %= 7;
	}
}


/*!
 *******************************************************************************
 *
 *  \returns number of days for actual month (1-12) and year (0-255: 2000-2255)
 *
 ******************************************************************************/
uint8_t RTC_DaysOfMonth()
{
    if (RTC_MM != 2) {
	    return pgm_read_byte(&RTC_DayOfMonthTablePrgMem[RTC_MM]);
    } else {
        if (RTC_NoLeapyear() ) {
            return 28;  // february no leapyear
        } else {
            return 29;	// leapyear feb=29
        }
    }
}


/*!
 *******************************************************************************
 *
 *  \returns \ref true if actual year is not a leapyear
 *
 ******************************************************************************/
bool RTC_NoLeapyear()
{
	if ( RTC_YY % 100) {
        return (bool) (RTC_YY % 4);
    } else {
        // year mod 100 = 0 is only every 400 years a leap year
        // we calculate only till the year 2255, so don't care
        return false;
    }
}


/*!
 *******************************************************************************
 *
 *  \returns \ref true if actual date is last sunday in march or october
 *
 ******************************************************************************/
bool RTC_IsLastSunday(void)
{
    int days_of_month;
    
    // March or October ?
    if (RTC_MM == 3){
        days_of_month=31;
    } else if (RTC_MM == 10){
        days_of_month=30;
    } else {
        return 0;  // not march or october
    }
    
    // last seven days of month
    if (RTC_DD << (days_of_month-7)){
        return 0;  // not last seven days
    }

    // sunday
    if (RTC_DOW == 7){
        return 1;  // sunday
    } else {
        return 0;  // not a sunday
    }
}


/*!
 *******************************************************************************
 *
 *  set day of week for actual date
 *
 *  \note
 *     valid for years from 2000 to 2255
 *
 ******************************************************************************/
static const uint16_t daysInYear [12] PROGMEM= {
	0, 
	31, 
	31+28, 
	31+28+31, 
	31+28+31+30,  
	31+28+31+30+31,
	31+28+31+30+31+30,
	31+28+31+30+31+30+31,
	31+28+31+30+31+30+31+31,
	31+28+31+30+31+30+31+31+30,
	31+28+31+30+31+30+31+31+30+31,
	31+28+31+30+31+30+31+31+30+31+30};

void RTC_SetDayOfWeek(void)
{
    uint16_t day_of_year;
    uint16_t tmp_dow;

    // Day of year
    day_of_year = pgm_read_word(&(daysInYear[RTC_MM-1])) + RTC_DD;
    
    if (RTC_MM > 2) {                       // february
        if (! RTC_NoLeapyear() ){
            day_of_year ++;
        }
    }

    // calc weekday
    tmp_dow = RTC_YY + ((RTC_YY-1) / 4) - ((RTC_YY-1) / 100) + day_of_year;
    // set DOW
    RTC_DOW = (uint8_t) ((tmp_dow + 5) % 7) ;
}

/*!
 *******************************************************************************
 *
 *  Set Date, and do all the range checking
 *
 *  \note
 *     return true if date is valid and set, otherwise false
 *
 ******************************************************************************/
bool RTC_SetDate(uint8_t dd, uint8_t mm, uint8_t yy)
{
	// Get current date for restauration in case of an error
	uint8_t old_yy = RTC_GetYearYY();
	uint8_t old_mm = RTC_GetMonth();
	
	// Set and check values
	if (mm > 0 && mm < 13 && dd > 0) {
		RTC_SetYear(yy);
		RTC_SetMonth(mm);
		if (dd <= RTC_DaysOfMonth()) {
			RTC_SetDay(dd);
			return true;
		}
	}
	
	// Restore old date
	RTC_SetYear(old_yy);
	RTC_SetMonth(old_mm);
	return false;
}

/*!
 *******************************************************************************
 *
 *  timer/counter2 overflow interrupt routine
 *
 *  \note
 *  - add one second to internal clock
 *  - increment tick counter
 *
 ******************************************************************************/
ISR(TIMER2_OVF_vect)
{
    RTC_Ticks++;          // overflow every 136 Years
    RTC_AddOneSecond();   // increment second and check Dow_Timer
}


#if HAS_CALIBRATE_RCO

/*!
 *******************************************************************************
 *
 *  Calibrate the internal OSCCAL byte,
 *  using the external 32,768 kHz crystal as reference
 *
 *  \note
 *     global interrupt has to be disabled before if not if will be
 *     disabled by this function.
 *
 ******************************************************************************/
void calibrate_rco(void)
{
    unsigned char calibrate = FALSE;
    int temp;

    cli();                               // disable global interrupt
    CLKPR = (1<<CLKPCE);                 // set Clock Prescaler Change Enable
    CLKPR = (1<<CLKPS1) | (1<<CLKPS0);   // set prescaler = 8  =>  1Mhz
    TIMSK2 = 0;                          // disable OCIE2A and TOIE2
    ASSR = (1<<AS2);                     // timer2 asynchronous 32,768kHz)
    OCR2A = 200;                         // set timer2 compare value
    TIMSK0 = 0;                          // delete any interrupt sources
    TCCR1B = (1<<CS10);                  // start timer1 with no prescaling
    TCCR2A = (1<<CS20);                  // start timer2 with no prescaling

    while((ASSR & 0x01) | (ASSR & 0x04));// wait for clear TCN2UB and TCR2UB
    // delay(1000);                      // wait for external crystal stabilise

    while(!calibrate)
    {
        TIFR1 = 0xFF;   // delete TIFR1 flags
        TIFR2 = 0xFF;   // delete TIFR2 flags
        TCNT1H = 0;     // clear timer1 counter
        TCNT1L = 0;
        TCNT2 = 0;      // clear timer2 counter

        while ( !(TIFR2 & (1<<OCF2A)) );   // wait for timer2 compareflag

        TCCR1B = 0;     // stop timer1

        if (TIFR1 & (1<<TOV1))  {
            temp = 0xFFFF;                // timer1 overflow, set temp to 0xFFFF
        } else {
            temp = (TCNT1H << 8) | TCNT1L;  // read out the timer1 counter value
        }

        if (temp > 6250) {
            OSCCAL--;   // internRC oscillator to fast, decrease the OSCCAL
        } else if (temp < 6120) {
            OSCCAL++;   // internRC oscillator to slow, increase the OSCCAL
        } else {
            calibrate = TRUE;   // the interRC is correct
        }
        TCCR1B = (1<<CS10); // start timer1
    }
}
#endif
