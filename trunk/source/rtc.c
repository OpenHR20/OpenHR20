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
 *				2008 Jiri Dobry (jdobry-at-centrum-dot-cz)
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
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>; Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
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
#include "task.h"
#include "eeprom.h"

// Vars


uint8_t RTC_hh;   //!< \brief Time: Hours
uint8_t RTC_mm;   //!< \brief Time: Minutes
uint8_t RTC_ss;   //!< \brief Time: Seconds
uint8_t RTC_DD;   //!< \brief Date: Day
uint8_t RTC_MM;   //!< \brief Date: Month
uint8_t RTC_YY;   //!< \brief Date: Year (0-255) -> 2000 - 2255
uint8_t RTC_DOW;  //!< Date: Day of Week

uint8_t RTC_DS;     //!< Daylightsaving Flag
uint32_t RTC_Ticks; //!< Ticks since last RTC.Init

// prototypes
void    RTC_AddOneDay(void);        // add one day to actual date
uint8_t RTC_DaysOfMonth(void);      // how many days in (RTC_MM, RTC_YY)
void    RTC_SetDayOfWeek(void);     // calc day of week (RTC_DD, RTC_MM, RTC_YY)
bool    RTC_IsLastSunday(void);     // check actual date if last sun in mar/oct

    // year mod 100 = 0 is only every 400 years a leap year
    // we calculate only till the year 2255, so don't care
#define RTC_NoLeapyear() (RTC_YY % 4) 

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
void RTC_Init(void)
{
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
    RTC_hh = BOOT_hh;
    RTC_mm = BOOT_mm;
    RTC_ss = 0;
    // initial date 1.01.2008
    RTC_DD = BOOT_DD;
    RTC_MM = BOOT_MM;
    RTC_YY = BOOT_YY;
    // day of week
    RTC_SetDayOfWeek();
    
    //! \note OCR2A register and interrupt is used in \ref keyboard.c
}

/*!
 *******************************************************************************
 *  set actual date
 *  \param day new value for day
 ******************************************************************************/
void RTC_SetDay(int8_t day)
{
    uint8_t day_in_m = RTC_DaysOfMonth();
    RTC_DD = (day+(-1+day_in_m))%day_in_m + 1;
    RTC_SetDayOfWeek();
}

/*!
 *******************************************************************************
 *  set actual date
 *  \param month new value for month
 ******************************************************************************/
void RTC_SetMonth(int8_t month)
{
    RTC_MM = (month+(-1+12))%12 + 1;
    RTC_SetDayOfWeek();
}

/*!
 *******************************************************************************
 *  set actual date
 *  \param year new value for year
 ******************************************************************************/
void RTC_SetYear(int8_t year)
{
    RTC_YY = year;
    RTC_SetDayOfWeek();
}

/*!
 *******************************************************************************
 *  set actual time
 *  \param hour new value for hour
 ******************************************************************************/
void RTC_SetHour(int8_t hour)
{
    RTC_hh = (hour+24)%24;
}


/*!
 *******************************************************************************
 *  set actual time
 *  \param minute new value for minute
 ******************************************************************************/
void RTC_SetMinute(int8_t minute)
{
    RTC_mm = (minute+60)%60;
}


/*!
 *******************************************************************************
 *  set actual time
 *  \param second new value for second
 ******************************************************************************/
void RTC_SetSecond(int8_t second)
{
    RTC_ss = (second+60)%60;
}

/*!
 *******************************************************************************
 *
 *  set timer for one timerslot for one weekday
 *
 *  \param dow day of week
 *  \param slot timeslot number
 *  \param time time
 *  \param timermode (type \ref timermode_t) 
 *
 *  \note
 *      - maximum timers for each weekday with \ref RTC_TIMERS_PER_DOW
 *      - 1 minute intervals supported
 *      - to deactivate timer: set value 0xfff
 *
 ******************************************************************************/
bool RTC_DowTimerSet(rtc_dow_t dow, uint8_t slot, uint16_t time, timermode_t timermode)
{
    if (dow>=8) return false;
    if (slot>=RTC_TIMERS_PER_DOW) return false;
    if (timermode>temperature3) return false;
    if (time>=60*25) time=0xfff;
    // to table format see to \ref ee_timers
    eeprom_timers_write(dow,slot,time | ((uint16_t)timermode<<12));
    return true;
}

/*!
 *******************************************************************************
 *
 *  get timer for one timerslot for one weekday
 *
 *  \param dow day of week
 *  \param slot timeslot number
 *  \param *timermode (type \ref timermode_t) 
 *  \returns time 
 *
 *  \note
 *      - maximum timers for each weekday with \ref RTC_TIMERS_PER_DOW
 *      - 1 minute intervals supported
 *      - to deactivate timer: set value 0xfff
 *
 ******************************************************************************/
uint16_t RTC_DowTimerGet(rtc_dow_t dow, uint8_t slot, timermode_t *timermode)
{
    // to table format see to \ref ee_timers
    uint16_t raw=eeprom_timers_read(dow,slot);
    *timermode = (timermode_t) (raw >> 12);
    return raw & 0xfff;
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
static int8_t RTC_DowTimerGetIndex(uint8_t dow,uint16_t time_minutes, bool exact)
{
    uint8_t i;
    int8_t index;
    uint16_t maxtime;
    maxtime=0;
    index = -1;
    // each timer until time_minutes
    for (i=1; i<RTC_TIMERS_PER_DOW; i++){
        // check if timer > maxtime and timer < actual time
        uint16_t table_time = eeprom_timers_read(dow,i) & 0x0fff;
        if ((table_time > maxtime) && 
            (((table_time < time_minutes) && !exact) || (table_time == time_minutes))) {
            maxtime = table_time;
            index=i;
        }
    }
    return index;
}

/*!
 *******************************************************************************
 *
 *  get actual temperature from timers
 *  
 *  \param exact=true time must be equal  
 *
 *  \returns temperature [see to \ref c2temp]
 *
 ******************************************************************************/

uint8_t RTC_ActualTimerTemperature(bool exact) {
    uint16_t minutes=RTC_hh*60 + RTC_mm;
    int8_t dow=((config.timer_mode==1)?RTC_DOW:0);
    int8_t index=RTC_DowTimerGetIndex(dow,minutes,exact);
    
    if ((index<0) && ! exact) {
        uint8_t i;
        uint8_t search_timers=(config.timer_mode==1)?7:1;
        for (i=0;i<search_timers;i--) {
            if (config.timer_mode==1) dow=(dow+7-2)%7+1;            
            index=RTC_DowTimerGetIndex(dow,24*60,exact);
            if (index>=0) {
                break;
            }
        }
    }
    
    if (index<0) return 0; //not found
    else return temperature_table((eeprom_timers_read(dow,index) & 0x3000) >> 12);
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
 *         reset FLAG RTC_DS on next day
 *
 *  \returns true if minutes changed, false otherwise  
 ******************************************************************************/
bool RTC_AddOneSecond(void)
{
    RTC_Ticks++;          // overflow every 136 Years
	if (++RTC_ss >= 60) {
		RTC_ss = 0;
		// notify com.c about the changed minute
		if (++RTC_mm >= 60) {
			RTC_mm = 0;
			// add one hour
			if (++RTC_hh >= 24) {
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
        return true;
 	}
 	return false;
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
    if (++RTC_DD > dom) {                   // Next Month
		RTC_DD = 1;
		if (++RTC_MM > 12) {                    // Next year
			RTC_MM = 1;
			RTC_YY++;
		}
		// Clear Daylight saving Flag
        RTC_DS=0;
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
    uint8_t dom = pgm_read_byte(&RTC_DayOfMonthTablePrgMem[RTC_MM]);
    if ((RTC_MM == 2)&&(!RTC_NoLeapyear()))
        return 29; // leapyear feb=29
    return dom;
}


/*!
 *******************************************************************************
 *
 *  \returns \ref true if actual date is last sunday in march or october
 *
 ******************************************************************************/
bool RTC_IsLastSunday(void)
{
    if (RTC_DOW == 7){ // sunday ?
        return 0;  // not sunday
    }
    // March or October ?
    if (RTC_MM == 3){
        // last seven days of month
        return (RTC_DD > (31-7));
    } else if (RTC_MM == 10){
        // last seven days of month
        return (RTC_DD > (30-7));
    } else {
        return 0;  // not march or october
    }    
}


/*!
 *******************************************************************************
 *
 *  set day of week for actual date
 *
 *  \note
 *     valid for years from 2000 to 2255
 *  \return 1=monday to 7=sunday
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
    if (RTC_MM > 2) { // february
        if (! RTC_NoLeapyear() ){
            day_of_year ++;
        }
    }
    // calc weekday
    tmp_dow = RTC_YY + ((RTC_YY-1) / 4) - ((RTC_YY-1) / 100) + day_of_year;
    // set DOW
    RTC_DOW = (uint8_t) ((tmp_dow + 5) % 7) +1;
}
#if 0
/*!
 *******************************************************************************
 *
 *  Set Date, and do all the range checking
 *
 *  \note
 *     return true if date is valid and set, otherwise false
 *
 ******************************************************************************/
bool RTC_SetDate(int8_t dd, int8_t mm, int8_t yy)
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
#endif


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
    task |= TASK_RTC;   // increment second and check Dow_Timer

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
