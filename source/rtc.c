//***************************************************************************
//
//  File........: rtc.c
//
//  Author(s)...:
//
//  Target(s)...: ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
//
//  Compiler....: WinAVR-20071221
//                avr-libc 1.6.0
//                GCC 4.2.2
//
//  Description.: Functions used to control the Clock
//
//  Revisions...:
//
//  YYYYMMDD - VER. - COMMENT                                     - SIGN.
//
//  20080201   0.0    created                                     - D.Carluccio
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
#include "rtc.h"

// Vars
volatile uint8_t RTC_hh;          // Global Time: Hours
volatile uint8_t RTC_mm;          //              Minutes
volatile uint8_t RTC_ss;          //              Seconds
volatile uint8_t RTC_DD;          // Global Date  Day
volatile uint8_t RTC_MM;          //              Month
volatile uint8_t RTC_YY;          //              Years (0-255) -> 2000 - 2255
volatile uint8_t RTC_DOW;         // Global Date  Day of Week
volatile uint8_t RTC_DS;          // Daylightsaving Flag

volatile uint8_t RTC_Dow_Timer[7][RTC_TIMERS_PER_DOW];  // DOW Timer entrys [dow][no]
rtc_callback_t RTC_DowTimerCallbackFunc;                // DOW Timer callback function

// prototypes
void RTC_AddOneSecond(void);                   // add one second to actual time
void RTC_AddOneDay(void);                      // add one day to actual date
uint8_t RTC_NoLeapyear(void);                  // is (RTC_YY) a leapyear?
uint8_t RTC_DaysOfMonth(void);                 // how many days in (RTC_MM, RTC_YY)
void    RTC_SetDayOfWeek(void);                // calc day of week (RTC_DD, RTC_MM, RTC_YY)
bool    RTC_IsLastSunday(void);                // check if actual date is last Sunday in march / october

// constants


uint8_t RTC_DayOfMonthTablePrgMem[] PROGMEM =
{
    31,    //  january
    28,    //  february
    31,    //  march
    30,    //  april
    31,    //  may
    30,    //  june
    31,    //  july
    31,    //  august
    30,    //  september
    31,    //  october
    30,    //  november
    31     //  december
};


/*****************************************************************************
*
*   function:       RTC_Init
*
*   returns:        None
*
*   parameters:     pFunc Callback function for DOW timer
*
*   purpose:        Start Timer/Counter2 in asynchronous operation 
*                         using a 32.768kHz crystal
*                   Set Date after Reset
*
*   global vars:    RTC_hh, RTC_mm, RTC_ss, RTC_DD, RTC_MM, RTC_YYYY, RTC_DOW
*
*****************************************************************************/
void RTC_Init(rtc_callback_t pFunc)
{
    RTC_DowTimerCallbackFunc = pFunc;   // callback function for DOW Timer

    TIMSK2 &= ~(1<<TOIE2);              // disable OCIE2A and TOIE2
    ASSR = (1<<AS2);                    // select asynchronous operation of Timer2
    TCNT2 = 0;                          // clear TCNT2A
    TCCR2A |= (1<<CS22) | (1<<CS20);    // select precaler: 32.768 kHz / 128 =
                                        // => 1 sec between each overflow

    // wait for TCN2UB and TCR2UB to be cleared
    while((ASSR & 0x01) | (ASSR & 0x04));   

    TIFR2 = 0xFF;                       // clear interrupt-flags
    TIMSK2 |= (1<<TOIE2);               // enable Timer2 overflow interrupt

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


/*****************************************************************************
*
*   function:       RTC_GetDay
*
*   returns:        actual date
*
*   parameters:     None
*
*   purpose:        read actual date
*
*   global vars:    RTC_DD
*
*****************************************************************************/
uint8_t RTC_GetDay(void)
{
    return RTC_DD;
}


/*****************************************************************************
*
*   function:       RTC_GetMonth
*
*   returns:        actual date
*
*   parameters:     None
*
*   purpose:        read actual date
*
*   global vars:    RTC_MM
*
*****************************************************************************/
uint8_t RTC_GetMonth(void)
{
    return RTC_MM;
}


/*****************************************************************************
*
*   function:       RTC_GetYearYY
*
*   returns:        actual date
*
*   parameters:     None
*
*   purpose:        read actual date
*
*   global vars:    RTC_YY
*
*****************************************************************************/
uint8_t RTC_GetYearYY(void)
{
    return RTC_YY;
}


/*****************************************************************************
*
*   function:       RTC_GetYearYYYY
*
*   returns:        actual date
*
*   parameters:     None
*
*   purpose:        read actual date
*
*   global vars:    RTC_YY
*
*****************************************************************************/
uint16_t RTC_GetYearYYYY(void)
{
    return 2000 + (uint16_t) RTC_YY;
}


/*****************************************************************************
*
*   function:       RTC_GetDayOfWeek
*
*   returns:        actual day of week
*
*   parameters:     None
*
*   purpose:        read actual date
*
*   global vars:    RTC_DOW
*
*   Remarks:        0=Monday, 6=Sunday
*
*****************************************************************************/
uint8_t RTC_GetDayOfWeek(void)
{
    return RTC_DOW;
}


/*****************************************************************************
*
*   function:       RTC_GetHour
*
*   returns:        actual time
*
*   parameters:     None
*
*   purpose:        read actual time
*
*   global vars:    RTC_hh
*
*****************************************************************************/
uint8_t RTC_GetHour(void)
{
    return RTC_hh;
}


/*****************************************************************************
*
*   function:       RTC_GetMinute
*
*   returns:        actual time
*
*   parameters:     None
*
*   purpose:        read actual time
*
*   global vars:    RTC_mm
*
*****************************************************************************/
uint8_t RTC_GetMinute(void)
{
    return RTC_mm;
}


/*****************************************************************************
*
*   function:       RTC_GetSecond
*
*   returns:        actual time 
*
*   parameters:     None
*
*   purpose:        read actual time
*
*   global vars:    RTC_ss
*
*****************************************************************************/
uint8_t RTC_GetSecond(void)
{
    return RTC_ss;
}


/*****************************************************************************
*
*   function:       RTC_SetDay
*
*   returns:        None
*
*   parameters:     None
*
*   purpose:        set actual date
*
*   global vars:    RTC_DD
*
*****************************************************************************/
void RTC_SetDay(uint8_t newday)
{
    if ((newday > 0) && (newday < 32)) {
        RTC_DD = newday;
    }
    RTC_SetDayOfWeek();
}


/*****************************************************************************
*
*   function:       RTC_SetMonth
*
*   returns:        None
*
*   parameters:     None
*
*   purpose:        set actual date
*
*   global vars:    RTC_MM
*
*****************************************************************************/
void RTC_SetMonth(uint8_t newmonth)
{
    if ((newmonth > 0) && (newmonth < 13)) {
        RTC_MM = newmonth;
    }
    RTC_SetDayOfWeek();
}


/*****************************************************************************
*
*   function:       RTC_SetYear
*
*   returns:        None
*
*   parameters:     None
*
*   purpose:        set actual date
*
*   global vars:    RTC_YYYY
*
*****************************************************************************/
void RTC_SetYear(uint8_t newyear)
{
    RTC_YY = newyear;
    RTC_SetDayOfWeek();
}


/*****************************************************************************
*
*   function:       RTC_SetHour
*
*   returns:        None
*
*   parameters:     None
*
*   purpose:        set actual time
*
*   global vars:    RTC_hh
*
*****************************************************************************/
void RTC_SetHour(uint8_t newhour)
{
    if (newhour < 24) {
        RTC_hh = newhour;
    }
}


/*****************************************************************************
*
*   function:       RTC_SetMinute
*
*   returns:        None
*
*   parameters:     None
*
*   purpose:        set actual time
*
*   global vars:    RTC_mm
*
*****************************************************************************/
void RTC_SetMinute(uint8_t newminute)
{
    if (newminute < 60) {
        RTC_mm = newminute;
    }
}


/*****************************************************************************
*
*   function:       RTC_SetSecond
*
*   returns:        None
*
*   parameters:     None
*
*   purpose:        set actual time
*
*   global vars:    RTC_ss
*
*****************************************************************************/
void RTC_SetSecond(uint8_t newsecond)
{
    if (newsecond < 60) {
        RTC_ss = newsecond;
    }
}

/*****************************************************************************
*
*   function:       RTC_DowTimerSet
*
*   returns:        None
*
*   parameters:     None
*
*   purpose:        Set time for a timerslot for one weekday
*
*   global vars:    RTC_Dow_Timer[day_of_week][timer_no]
*
*   Remarks:        - RTC_TIMERS_PER_DOW timers for each weekday
*                   - only 10 minute intervals supported:
*                     hh:m0 is coded in one byte: hh*1ß+m
*                     e.g.: 12:00 = 120, 17:30 = 173, 22:10 = 221
*                   - to deactivate timer: set value > 0xff
*
*****************************************************************************/
void RTC_DowTimerSet(rtc_dow_t dow, uint8_t slot, uint8_t time)
{
    RTC_Dow_Timer[dow][slot] = time;
}

/*****************************************************************************
*
*   function:       RTC_DowTimerGet
*
*   returns:        time of selected timer
*
*   parameters:     None
*
*   purpose:        Get time for a timerslot for one weekday
*
*   global vars:    RTC_Dow_Timer[]
*
*   Remarks:        - RTC_TIMERS_PER_DOW timers for each weekday
*                   - only 10 minute intervals supported:
*                     hh:m0 is coded in one byte: hh*1ß+m
*                     e.g.: 12:00 = 120, 17:30 = 173, 22:10 = 221
*                   - to deactivate timer: set value > 0xff
*
*****************************************************************************/
uint8_t RTC_DowTimerGet(rtc_dow_t dow, uint8_t slot)
{
    return RTC_Dow_Timer[dow][slot];
}

/*****************************************************************************
*
*   function:       RTC_DowTimerGetStartOfDay
*
*   returns:        timer index
*
*   parameters:     None
*
*   purpose:        Get timer index for last active timer 
*
*   global vars:    RTC_Dow_Timer[]
*
*   Remarks:        what is the last occured timer index
*                   needed to calculate the actual valid timer index
*
*****************************************************************************/
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
        if ((RTC_Dow_Timer[RTC_DOW][i] > maxtime) && !(RTC_Dow_Timer[RTC_DOW][i] > acttime)){
            maxtime = RTC_Dow_Timer[RTC_DOW][i];
            index=i;
        }
    }
    return index;
}


/*****************************************************************************
*
*   function:       RTC_DowTimerGetStartOfDay
*
*   returns:        timer index 
*
*   parameters:     None
*
*   purpose:        Get timer index for last active timer for previous day
*
*   global vars:    RTC_Dow_Timer[]
*
*   Remarks:        what was status at 00:00 of the actual day
*                   needed to calculate the actual valid timer index
*
*****************************************************************************/
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
    while ((index == 0xff)&&(day<7)){
    
        // dow of previous day
        dow = (RTC_DOW+6)%7;
    
        // dow of previous day
        for (i=1; i<RTC_TIMERS_PER_DOW; i++){
            // check if > maxtime and (active (<240)
            if ((RTC_Dow_Timer[dow][i] > maxtime) && (RTC_Dow_Timer[dow][i] < 240)){
                maxtime = RTC_Dow_Timer[RTC_DOW][i];
                index=i;
            }
        }
        day++;
    }
    return index;
}


/*****************************************************************************
*
*   function:       RTC_AddOneSecond
*
*   returns:        None
*
*   parameters:     None
*
*   purpose:        Add one second to clock variables
*                   calculate overflows, regarding leapyear, etc.
*
*   global vars:    RTC_YYYY,  RTC_MM,   RTC_DD,
*                   RTC_hh,    RTC_mm,   RTC_ss,
*                   RTC_DOW
*
*   Remarks:        Daylight-Saving:
*                   a) last sunday in march 1:59:59 -> 3:00:00
*                   b) last sunday in october 2:59:59 -> 2:00:00
*                      ONLY ONE TIME -> set FLAG RTC_DS
*                                       reset FLAG RTC_DS on November 1st
*
*****************************************************************************/
void RTC_AddOneSecond(void)
{
    uint8_t i;
	if (++RTC_ss == 60) {
		RTC_ss = 0;
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


/*****************************************************************************
*
*   function:       RTC_AddOneDay
*
*   returns:        None
*
*   parameters:     None
*
*   purpose:        Add one Day to Date variables
*                   calculate overflows, regarding leapyear, etc.
*
*   global vars:    RTC_YYYY,  RTC_MM,   RTC_DD, RTC_DOW
*
*****************************************************************************/
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


/*****************************************************************************
*
*   function:       RTC_DaysOfMonth
*
*   returns:        days in month
*
*   parameters:     month: 1-12
*                   year:  0-255 (2000-2255)
*
*   purpose:        calculate maximal possible day for this month and year
*
*   global vars:    None
*
*****************************************************************************/
uint8_t RTC_DaysOfMonth()
{
    if (RTC_MM != 2) {
	    return (uint8_t) pgm_read_byte(&RTC_DayOfMonthTablePrgMem[(uint8_t)RTC_MM]);
    } else {
        if (RTC_NoLeapyear() ) {
            return 28;  // february no leapyear
        } else {
            return 29;	// leapyear feb=29
        }
    }
}


/*****************************************************************************
*
*   function:       RTC_NoLeapyear
*
*   returns:        true if year is _not_ a leapyear
*
*   parameters:     year:  0-65535
*
*   purpose:        calculate leapyear
*
*   global vars:    None
*
*****************************************************************************/
bool RTC_NoLeapyear()
{
	if ( RTC_YY % 100) {
        return (bool) (RTC_YY % 4);
    } else {
        // year mod 100 = 0 is only every 400 years a leap year
        // we calculate only till the year 2255, so  don't care
        return false;
    }
}


/*****************************************************************************
*
*   function:       RTC_IsLastSunday
*
*   returns:        true if last sunday of month
*
*   parameters:     None
*
*   purpose:        check if actual date is the last sunday of the month
*
*   global vars:    None
*
*   remark:         valid only in march and october, used for daylight saving
*
*****************************************************************************/
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


/*****************************************************************************
*
*   function:       RTC_SetDayOfWeek
*
*   returns:        Day of Week
*
*   parameters:     None
*
*   purpose:        calculate maximal possible day for this month and year
*
*   global vars:    RTC_DD: day   1-31
*                   RTC_MM: month 1-12
*                   RTC_YY: year  0-255 (2000-2255)
*
*   remark:         Valid for years from 2000 to 2255
*
*****************************************************************************/
void RTC_SetDayOfWeek(void)
{
    uint16_t day_of_year;
    uint16_t tmp_dow;

    // Day of year
    day_of_year = 31 * (RTC_MM-1) + RTC_DD;
    
    // subtract month < 31 days
    if (RTC_MM > 2) {                       // february
        if ( RTC_NoLeapyear() ){
            day_of_year = day_of_year - 3;
        } else {
            day_of_year = day_of_year - 2;
        }
    }
    if (RTC_MM >  4) {day_of_year--;}       // april
    if (RTC_MM >  6) {day_of_year--;}       // june
    if (RTC_MM >  9) {day_of_year--;}       // september
    if (RTC_MM > 11) {day_of_year--;}       // november

    // calc weekday
    tmp_dow = RTC_YY + ((RTC_YY-1) / 4) - ((RTC_YY-1) / 100) + day_of_year;
    if (RTC_YY > 0) {
        tmp_dow++;                          // 2000 was leapyear
    }

    // set DOW
    RTC_DOW = (uint8_t) ((tmp_dow + 4) % 7) ;
}


/*****************************************************************************
*
*   function:       Timer/Counter2 Overflow Interrupt Routine
*
*   returns:        None
*
*   parameters:     None
*
*   purpose:        add one second to internal clock
*
*   global vars:    None
*
*****************************************************************************/
ISR(TIMER2_OVF_vect)
{
    RTC_AddOneSecond();   // increment second and check Dow_Timer
}


#if HAS_CALIBRATE_RCO
/*****************************************************************************
*
*   Function name : calibrate_rco
*
*   returns:        None
*
*   parameters:     None
*
*   purpose:        Calibrate the internal OSCCAL byte, using the external
*                   32,768 kHz crystal as reference
*
*   remark:         global interrupt has to be disabled before
*                   if not if will be disabled by this function.
*
*****************************************************************************/
void calibrate_rco(void)
{
    unsigned char calibrate = FALSE;
    int temp;

    cli();                               // disable global interrupt
    CLKPR = (1<<CLKPCE);                 // set Clock Prescaler Change Enable
    CLKPR = (1<<CLKPS1) | (1<<CLKPS0);   // set prescaler = 8  =>  1Mhz
    TIMSK2 = 0;                          // disable OCIE2A and TOIE2
    ASSR = (1<<AS2);                     // select asynchronous operation of timer2 (32,768kHz)
    OCR2A = 200;                         // set timer2 compare value
    TIMSK0 = 0;                          // delete any interrupt sources
    TCCR1B = (1<<CS10);                  // start timer1 with no prescaling
    TCCR2A = (1<<CS20);                  // start timer2 with no prescaling

    while((ASSR & 0x01) | (ASSR & 0x04));  // wait for TCN2UB and TCR2UB to be cleared

    // delay(1000);                       // wait for external crystal to stabilise

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
            temp = 0xFFFF;                    // timer1 overflow, set the temp to 0xFFFF
        } else {
            temp = (TCNT1H << 8) | TCNT1L;    // read out the timer1 counter value
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
