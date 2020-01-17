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
#include "config.h"
#include "main.h"
#include "rtc.h"
#include "task.h"
#if !defined(MASTER_CONFIG_H)
#include "eeprom.h"
#include "menu.h"
#endif

// Vars

rtc_t RTC = {
	BOOT_YY,
	BOOT_MM,
	BOOT_DD,
	BOOT_hh,
	BOOT_mm,
	0,
	0,
#if (RFM == 1)
	0
#endif
};

uint8_t RTC_DS;         //!< Daylightsaving Flag
#ifdef RTC_TICKS
uint32_t RTC_Ticks = 0; //!< Ticks since last Reset
#endif

// prototypes
static void    RTC_AddOneDay(void);             // add one day to actual date
static uint8_t RTC_DaysOfMonth(void);           // how many days in (RTC_MM, RTC_YY)
static void    RTC_SetDayOfWeek(void);          // calc day of week (RTC_DD, RTC_MM, RTC_YY)
static bool    RTC_IsLastSunday(void);          // check actual date if last sun in mar/oct

// year mod 100 = 0 is only every 400 years a leap year
// we calculate only till the year 2255, so don't care
#define RTC_NoLeapyear() (RTC.YY % 4)

// Progmem constants

//! day of month for each month from january to december
const uint8_t RTC_DayOfMonthTablePrgMem[] PROGMEM =
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
#if !defined(MASTER_CONFIG_H)
	TIMSK2 &= ~(1 << TOIE2);        // disable OCIE2A and TOIE2
	ASSR = (1 << AS2);              // Timer2 asynchronous operation
	TCNT2 = 0;                      // clear TCNT2A
	TCCR2A |= TCCR2A_INIT;          // select precaler: 32.768 kHz / 128 =
	// => 1 sec between each overflow

	// wait for TCN2UB and TCR2UB to be cleared
	while ((ASSR & (_BV(TCN2UB) | _BV(TCR2UB))) != 0)
	{
		;
	}

	TIFR2 = 0xFF;                           // clear interrupt-flags
	TIMSK2 |= (1 << TOIE2);                 // enable Timer2 overflow interrupt
#else
#if (NANODE == 1 || JEENODE == 1)
#define TIFR TIFR1
#define TIMSK TIMSK1
#endif
	OCR1A = (F_CPU / 800) - 1;              // 1/100s interrupt
	TCCR1B = _BV(CS11) | _BV(WGM12);        // clk/8 CTC mode
	TIFR = _BV(OCF1A);                      // clear interrupt-flags
	TIMSK |= _BV(OCIE1A);
#endif

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

	RTC.DD = (uint8_t)(day + (-1 + day_in_m)) % day_in_m + 1;
	RTC_SetDayOfWeek();
}

/*!
 *******************************************************************************
 *  set actual date
 *  \param month new value for month
 ******************************************************************************/
void RTC_SetMonth(int8_t month)
{
	RTC.MM = (uint8_t)(month + (-1 + 12)) % 12 + 1;
	RTC_SetDayOfWeek();
}

/*!
 *******************************************************************************
 *  set actual date
 *  \param year new value for year
 ******************************************************************************/
void RTC_SetYear(uint8_t year)
{
	RTC.YY = year;
	RTC_SetDayOfWeek();
}

/*!
 *******************************************************************************
 *  set actual time
 *  \param hour new value for hour
 ******************************************************************************/
void RTC_SetHour(int8_t hour)
{
	RTC.hh = (uint8_t)(hour + 24) % 24;
}


/*!
 *******************************************************************************
 *  set actual time
 *  \param minute new value for minute
 ******************************************************************************/
void RTC_SetMinute(int8_t minute)
{
	RTC.mm = (uint8_t)(minute + 60) % 60;
}


/*!
 *******************************************************************************
 *  set actual time
 *  \param second new value for second
 ******************************************************************************/
void RTC_SetSecond(int8_t second)
{
	RTC.ss = (uint8_t)(second + 60) % 60;
}


#if defined(MASTER_CONFIG_H)
/*!
 *******************************************************************************
 *  set actual time
 *  \param second new value for 1/100 seconds
 ******************************************************************************/
void RTC_SetSecond100(uint8_t second100)
{
	RTC_s100 = (second100 + 100) % 100;
}
#endif

#if !defined(MASTER_CONFIG_H)
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
	if (dow >= 8)
	{
		return false;
	}
	if (slot >= RTC_TIMERS_PER_DOW)
	{
		return false;
	}
	if (timermode > temperature3)
	{
		return false;
	}
	if (time >= 60 * 25)
	{
		time = 0xfff;
	}
	// to table format see to \ref ee_timers
	eeprom_timers_write(dow, slot, time | ((uint16_t)timermode << 12));
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
	uint16_t raw = eeprom_timers_read_raw(timers_get_raw_index(dow, slot));

	*timermode = (timermode_t)(raw >> 12);
	return raw & 0xfff;
}

/*!
 *******************************************************************************
 *
 *  get timer for dow and time
 *
 *  \param exact=true time must be equal
 *  \param dow - day of week
 *  \param time - time in minutes
 *
 *  \returns  index of timer
 *
 ******************************************************************************/
static uint8_t RTC_FindTimerRawIndex(uint8_t dow, uint16_t time_minutes)
{
	uint8_t search_timers = (dow > 0) ? 8 : 2;
	int8_t raw_index = -1;
	uint8_t i;

	for (i = 0; i < search_timers; i++)
	{
		{
			uint8_t idx_raw = timers_get_raw_index(dow, 0);
			uint8_t stop = idx_raw + RTC_TIMERS_PER_DOW;
			uint16_t maxtime = 0;
			// each timer until time_minutes
			for (; idx_raw < stop; idx_raw++)
			{
				// check if timer > maxtime and timer <= actual time
				uint16_t table_time = eeprom_timers_read_raw(idx_raw) & 0x0fff;
				if (table_time >= 24 * 60)
				{
					continue;
				}
				if ((table_time >= maxtime) && (table_time <= time_minutes))
				{
					maxtime = table_time;
					raw_index = idx_raw;
				}
			}
		}
		if (raw_index >= 0)
		{
			break;
		}
		if (dow > 0)
		{
			dow = (dow + (7 - 2)) % 7 + 1;
		}
		time_minutes = 24 * 60;
	}
	return raw_index;
}

/*!
 *******************************************************************************
 *
 *  get hour bar bitmap for DOW
 *
 *  \returns bitmap
 *
 *  \note battery expensive function, use buffer for result
 *
 ******************************************************************************/
int32_t RTC_DowTimerGetHourBar(uint8_t dow)
{
	int16_t time = 24 * 60;
	int8_t bar_pos = 23;
	uint32_t bitmap = 0;

	while (time > 0)
	{
		int8_t raw_idx = RTC_FindTimerRawIndex(dow, time);
		uint16_t table_time = ((raw_idx >= 0) ? eeprom_timers_read_raw(raw_idx) : 0);
		bool bit = ((table_time & 0x3000) >= 0x2000);
		if ((table_time & 0xfff) < time)
		{
			time = (table_time & 0xfff) - 1;
		}
		else
		{
			time = -1;
		}
		{
			while (((int16_t)(bar_pos)) * 60 > time)
			{
				if (bit)
				{
					bitmap |= 1;
				}
				if (bar_pos > 0)
				{
					bitmap = (bitmap << 1);
				}
				bar_pos--;
			}
		}
	}
	return bitmap;
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
uint8_t RTC_ActualTimerTemperatureType(bool exact)
{
	uint16_t minutes = RTC.hh * 60 + RTC.mm;
	int8_t dow = ((config.timer_mode == 1) ? RTC.DOW : 0);
	int8_t raw_index = RTC_FindTimerRawIndex(dow, minutes);
	uint16_t data = eeprom_timers_read_raw(raw_index);

	if (raw_index < 0)
	{
		return TEMP_TYPE_INVALID;            //not found
	}
	if (exact)
	{
		if ((data & 0xfff) != minutes)
		{
			return TEMP_TYPE_INVALID;
		}
		if ((raw_index / RTC_TIMERS_PER_DOW) != dow)
		{
			return TEMP_TYPE_INVALID;
		}
	}
	return (data >> 12) & 3;
}
#endif // !defined(MASTER_CONFIG_H)

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
void RTC_AddOneSecond(void)
{
#ifdef RTC_TICKS
	RTC_Ticks++;      // overflow every 136 Years
#endif
#if (RFM == 1)
	RTC.pkt_cnt = 0;
#endif
	if (++RTC.ss >= 60)
	{
		RTC.ss = 0;
		// notify com.c about the changed minute
		if (++RTC.mm >= 60)
		{
			RTC.mm = 0;
			// add one hour
			if (++RTC.hh >= 24)
			{
				RTC.hh = 0;
				RTC_AddOneDay();
			}
			// start of summertime: March, 2:00:00 ?
			if ((RTC.MM == 3) && (RTC.hh == 2))
			{
				// Last Sunday ?
				if (RTC_IsLastSunday())
				{
					RTC.hh++; // 2:00 -> 3:00
				}
			}
			// end of summertime: October, 03:00, RTC_DS == 0
			if ((RTC.MM == 10) && (RTC.hh == 3) && (RTC_DS == 0))
			{
				// Last Sunday ?
				if (RTC_IsLastSunday())
				{
					RTC.hh--; // 3:00 -> 2:00
					RTC_DS = 1;
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
static void RTC_AddOneDay(void)
{
	uint8_t dom;

	// How many day has actual month
	dom = RTC_DaysOfMonth();
	if (++RTC.DD > dom)                     // Next Month
	{
		RTC.DD = 1;
		if (++RTC.MM > 12)              // Next year
		{
			RTC.MM = 1;
			RTC.YY++;
		}
		// Clear Daylight saving Flag
		RTC_DS = 0;
	}
	// next day of week
	RTC.DOW = (RTC.DOW % 7) + 1; // Monday = 1 Sat=7
#if !defined(MASTER_CONFIG_H)
	// update hourbar
	menu_update_hourbar((config.timer_mode == 1) ? RTC.DOW : 0);
#endif
}


/*!
 *******************************************************************************
 *
 *  \returns number of days for actual month (1-12) and year (0-255: 2000-2255)
 *
 ******************************************************************************/
static uint8_t RTC_DaysOfMonth()
{
	uint8_t dom = pgm_read_byte(&RTC_DayOfMonthTablePrgMem[RTC.MM - 1]);

	if ((RTC.MM == 2) && (!RTC_NoLeapyear()))
	{
		return 29; // leapyear feb=29
	}
	return dom;
}


/*!
 *******************************************************************************
 *
 *  \returns \ref true if actual date is last sunday in march or october
 *
 ******************************************************************************/
static bool RTC_IsLastSunday(void)
{
	if (RTC.DOW != 7)       // sunday ?
	{
		return 0;       // not sunday
	}
	// March or October ?
	if (RTC.MM == 3)
	{
		// last seven days of month
		return RTC.DD > (31 - 7);
	}
	else if (RTC.MM == 10)
	{
		// last seven days of month
		return RTC.DD > (30 - 7);
	}
	else
	{
		return 0; // not march or october
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
static const uint16_t daysInYear [12] PROGMEM = {
	0,
	31,
	31 + 28,
	31 + 28 + 31,
	31 + 28 + 31 + 30,
	31 + 28 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30,
	31 + 28 + 31 + 30 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30
};

static void RTC_SetDayOfWeek(void)
{
	uint16_t day_of_year;
	uint16_t tmp_dow;

	// Day of year
	day_of_year = pgm_read_word(&(daysInYear[RTC.MM - 1])) + RTC.DD;
	if (RTC.MM > 2)   // february
	{
		if (!RTC_NoLeapyear())
		{
			day_of_year++;
		}
	}
	// calc weekday
	tmp_dow = RTC.YY + ((RTC.YY - 1) / 4) - ((RTC.YY - 1) / 100) + day_of_year;
	// set DOW
	RTC.DOW = (uint8_t)((tmp_dow + 5) % 7) + 1;

#if !defined(MASTER_CONFIG_H)
	menu_update_hourbar((config.timer_mode == 1) ? RTC.DOW : 0);
#endif
}

uint8_t RTC_timer_todo = 0;
uint8_t RTC_timer_done = 0;
static uint8_t RTC_timer_time[RTC_TIMERS];
#if defined(MASTER_CONFIG_H)
static uint8_t RTC_next_compare;
#endif

void RTC_timer_set(uint8_t timer_id, uint8_t time)
{
	uint8_t t2, i, next, dif;

	// next is uninitialized, it is correct

	cli();
	RTC_timer_todo |= _BV(timer_id);
	RTC_timer_time[timer_id - 1] = time;
	t2 = TCNT2;
	dif = 255;
	for (i = 0; i < RTC_TIMERS; i++)
	{
		if ((RTC_timer_todo & (2 << i)))
		{
			if ((RTC_timer_time[i] - t2) <= dif)
			{
				next = RTC_timer_time[i];
				dif = next - t2;
			}
		}
	}

// Ignore maybe ununitialized for next, following above comment
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

#if defined(MASTER_CONFIG_H)
	// cppcheck-suppress uninitvar
	RTC_next_compare = next;
#else
	if (OCR2A != next)
	{
		while (ASSR & (1 << OCR2UB))
		{
			;                       // ATmega169 datasheet chapter 17.8.1
		}
		OCR2A = next;
	}
#endif

#pragma GCC diagnostic pop

	sei();
#if !defined(MASTER_CONFIG_H)
	TIMSK2 |= (1 << OCIE2A); // enable interupt again
#endif
}

#if !defined(MASTER_CONFIG_H)
/*!
 *******************************************************************************
 *
 *  timer/counter2 overflow interrupt routine
 *
 *  \note
 *  - add one second to internal clock
 *
 ******************************************************************************/
#if !TASK_IS_SFR || DEBUG_PRINT_RTC_TICKS
// not optimized
ISR(TIMER2_OVF_vect)
{
	task |= TASK_RTC;   // increment second and check Dow_Timer
	RTC_timer_done |= _BV(RTC_TIMER_OVF) | _BV(RTC_TIMER_RTC);
#if (DEBUG_PRINT_RTC_TICKS)
	COM_putchar('|');
#endif
}
#else
// optimized
ISR_NAKED ISR(TIMER2_OVF_vect)
{
	asm volatile (
		"__my_tmp_reg__ = 16" "\n"
		/* prologue */
		"	push __my_tmp_reg__""\n"
		"   in __my_tmp_reg__,__SREG__" "\n"
		"	push __my_tmp_reg__""\n"
		/* prologue end  */
		"	sbi %0,%1""\t\n"
		"   lds __my_tmp_reg__,RTC_timer_done" "\n"
		"	ori __my_tmp_reg__,%2""\n"
		"   sts RTC_timer_done,__my_tmp_reg__" "\n"
		/* epilogue */
		"	pop __my_tmp_reg__""\n"
		"   out __SREG__,__my_tmp_reg__" "\n"
		"	pop __my_tmp_reg__""\n"
		"	reti""\n"
		/* epilogue end */
		: : "I" (_SFR_IO_ADDR(task)), "I" (TASK_RTC_BIT), "M" (_BV(RTC_TIMER_OVF) | _BV(RTC_TIMER_RTC))
	);
}
#endif
extern volatile bool kb_timeout;
/*!
 *******************************************************************************
 *
 *  timer/counter2 compare interrupt routine
 *
 *  \note - clear keyboard timeout flag
 *  \note - disable this interrupt
 *
 ******************************************************************************/
ISR(TIMER2_COMP_vect)
{
	uint8_t t2 = TCNT2 - 1;

	task |= TASK_RTC;
#if (DEBUG_PRINT_RTC_TICKS)
	COM_putchar('%');
#endif
	if ((RTC_timer_todo & _BV(RTC_TIMER_KB)) && (t2 == RTC_timer_time[RTC_TIMER_KB - 1]))
	{
		kb_timeout = true; // keyboard noise cancelation
		RTC_timer_todo &= ~_BV(RTC_TIMER_KB);
	}
	{
		uint8_t i;
		for (i = 2; i <= RTC_TIMERS; i++)
		{
			if ((RTC_timer_todo & _BV(i)) && (t2 == RTC_timer_time[i - 1]))
			{
				RTC_timer_done |= _BV(i);
				RTC_timer_todo &= ~_BV(i);
			}
		}
	}
	uint8_t dif = 255;
	uint8_t i, next;  // next is uninitialized, it is correct
	for (i = 0; i < RTC_TIMERS; i++)
	{
		if ((RTC_timer_todo & (2 << i)))
		{
			if ((RTC_timer_time[i] - t2) <= dif)
			{
				next = RTC_timer_time[i];
				dif = next - t2;
			}
		}
	}
// Ignore maybe ununitialized for next, following above comment
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
	if (OCR2A != next)
	{
		// while (ASSR & (1<<OCR2UB)) {;} // ATmega169 datasheet chapter 17.8.1
		// waiting is not needed, it not allow timer state machine
		OCR2A = next;
	}
#pragma GCC diagnostic pop
	if (RTC_timer_todo == 0)
	{
		TIMSK2 &= ~(1 << OCIE2A);
	}
}
#else
/*!
 *******************************************************************************
 *
 *  timer/counter1 overflow interrupt routine
 *
 *  \note
 *  - add 1/100 second to internal clock
 *
 ******************************************************************************/
volatile uint8_t RTC_s100 = 0;
ISR(TIMER1_COMPA_vect)
{
	if (++RTC_s100 >= 100)
	{
		task |= TASK_RTC; // increment second and check Dow_Timer
		RTC_s100 = 0;
		// RTC_timer_done |= _BV(RTC_TIMER_OVF);
	}
	if (RTC_timer_todo && (RTC_next_compare == RTC_s100))
	{
		uint8_t i;
		for (i = 1; i <= RTC_TIMERS; i++)
		{
			if ((RTC_timer_todo & _BV(i)) && (RTC_s100 == RTC_timer_time[i - 1]))
			{
				RTC_timer_done |= _BV(i);
				RTC_timer_todo &= ~_BV(i);
				task |= TASK_TIMER; // increment second and check Dow_Timer
			}
		}
		uint8_t dif = 255;
		uint8_t next;
		for (i = 0; i < RTC_TIMERS; i++)
		{
			if ((RTC_timer_todo & (2 << i)))
			{
				if ((RTC_timer_time[i] - RTC_s100) < dif)
				{
					next = RTC_timer_time[i];
					dif = next - RTC_s100;
				}
			}
		}
		// cppcheck-suppress uninitvar
// Ignore maybe ununitialized for next, following above comment
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
		RTC_next_compare = next;
#pragma GCC diagnostic pop
	}
}
#endif


#if HAS_CALIBRATE_RCO
/*!
 *******************************************************************************
 *
 *  Calibrate the internal OSCCAL byte,
 *  using the external 32,768 kHz crystal as reference
 *  Implementation taken from Atmel AVR055 application note.
 *
 *  \note
 *     global interrupt has to be disabled before if not if will be
 *     disabled by this function.
 *
 ******************************************************************************/
#define EXTERNAL_TICKS 200      // ticks on XTAL. Modify to increase/decrease accuracy
#define XTAL_FREQUENCY 32768    // Frequency of the external oscillator
#define LOOP_CYCLES    7        // Number of CPU cycles the loop takes to execute
void calibrate_rco(void)
{
	// Computes countVal for use in the calibration
	static const uint16_t countVal = (EXTERNAL_TICKS * F_CPU) / (XTAL_FREQUENCY * LOOP_CYCLES);

	cli();

	// Set up timer to be ASYNCHRONOUS from the CPU clock with a second
	// EXTERNAL 32,768kHz CRYSTAL driving it. No prescaling on asynchronous timer.
	ASSR = (1 << AS2);
	TCCR2A = (1 << CS20);

	// Calibrate using 128(0x80) calibration cycles
	uint8_t cycles = 0x80;
	uint16_t count;
	do
	{
		count = 0;
		TCNT2 = 0x00; // Reset async timer/counter

		// Wait until async timer is updated  (Async Status reg. busy flags).
		while (ASSR & ((1 << OCR2UB) | (1 << TCN2UB) | (1 << TCR2UB)))
		{
			;
		}

		// this loop needs to take exactly LOOP_CYCLES CPU cycles!
		// make sure your compiler does not mess with it!
		do
		{
			count++;
		} while (TCNT2 < EXTERNAL_TICKS);

		if (count > countVal)
		{
			OSCCAL--;
		}
		else if (count < countVal)
		{
			OSCCAL++;
		}
		else
		{
			break; //cycles=0xFF;
		}
	} while (--cycles);
}
#endif
