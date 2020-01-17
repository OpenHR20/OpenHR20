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
 * \file       keyboard.c
 * \brief      the keyboard scan for Open HR20 project
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

// HR20 Project includes
#include "config.h"
#include "main.h"
#include "task.h"
#include "keyboard.h"
#include "controller.h"
#include "debug.h"
#include "common/rtc.h"


// global Vars for keypress and wheel status
static uint8_t state_front_prev;
uint8_t state_wheel_prev;
static uint8_t long_press;
static uint8_t long_quiet;
static volatile uint8_t keys = 0;  // must be volatile, shared into interrupt
uint16_t kb_events = 0;
volatile bool kb_timeout = true;
static bool allow_rewoke = false;


/*!
 *******************************************************************************
 *  Recognize keypress and wheel
 *
 *  \note
 ******************************************************************************/
void task_keyboard(void)
{
	{                                                       // wheel
		uint8_t wheel = keys & (KBI_ROT1 | KBI_ROT2);
		if ((wheel ^ state_wheel_prev) & KBI_ROT1)      //only ROT1 have interrupt, change detection
		{
			if ((wheel == 0) || (wheel == (KBI_ROT1 | KBI_ROT2)))
			{
#ifdef LCD_UPSIDE_DOWN
				kb_events |= KB_EVENT_WHEEL_PLUS;
#else
				kb_events |= KB_EVENT_WHEEL_MINUS;
#endif
				long_quiet = 0;
			}
			else
			{
#ifdef LCD_UPSIDE_DOWN
				kb_events |= KB_EVENT_WHEEL_MINUS;
#else
				kb_events |= KB_EVENT_WHEEL_PLUS;
#endif
				long_quiet = 0;
			}
			state_wheel_prev = wheel;
		}
	} // wheel
	{ // other keys
		uint8_t front = keys & (KBI_PROG | KBI_C | KBI_AUTO);
		if (front != state_front_prev)
		{
			if (front && (state_front_prev == 0) && kb_timeout)
			{
				if (front == KBI_PROG)
				{
					kb_events |= KB_EVENT_PROG;
					allow_rewoke = true;
				}
				else if (front == KBI_C)
				{
					kb_events |= KB_EVENT_C;
					allow_rewoke = true;
				}
				else if (front == KBI_AUTO)
				{
					kb_events |= KB_EVENT_AUTO;
					allow_rewoke = true;
				}
			}

			if (allow_rewoke)
			{
				// some key was pressed, and keep key pressed and add some other key
				// we need "REVOKE" event
				if ((state_front_prev == KBI_PROG) && ((front ^ KBI_PROG) != KBI_PROG))
				{
					kb_events |= KB_EVENT_PROG_REWOKE;
					allow_rewoke = false;
				}
				if ((state_front_prev == KBI_C) && ((front ^ KBI_C) != KBI_C))
				{
					kb_events |= KB_EVENT_C_REWOKE;
					allow_rewoke = false;
				}
				if ((state_front_prev == KBI_AUTO) && ((front ^ KBI_AUTO) != KBI_AUTO))
				{
					kb_events |= KB_EVENT_AUTO_REWOKE;
					allow_rewoke = false;
				}
			}
			if (kb_timeout)   // keyboard noise cancellation
			{
				kb_timeout = false;
				// while (ASSR & (1<<OCR2UB)) {;} //this is not needed; kb_timeout==true means that OCR2A must be free
				RTC_timer_set(RTC_TIMER_KB, TCNT2 + KEYBOARD_NOISE_CANCELATION);
			}
			state_front_prev = front;

			state_front_prev = front;
			long_press = 0; // long press detection RESET
			long_quiet = 0;
		}
	}
}

/*!
 *******************************************************************************
 *  Keyboard long press detection
 *
 *  \note
 ******************************************************************************/
void task_keyboard_long_press_detect(void)
{
	if (!state_front_prev)
	{
		if (++long_quiet == 0)
		{
			// overload protection, nothing to do, event was generated previous
			long_quiet--;
			return;
		}
		if (long_quiet == LONG_QUIET_THLD)
		{
			kb_events |= KB_EVENT_NONE_LONG;
		}
	}
	else     // if (! state_front_prev)
	{
		if (++long_press == 0)
		{
			// overload protection, nothing to do, event was generated previous
			long_press--;
			return;
		}
		if (long_press == LONG_PRESS_THLD)
		{
			switch (state_front_prev)
			{
			case KBI_PROG:
				kb_events |= KB_EVENT_PROG_LONG | KB_EVENT_PROG_REWOKE;
				break;
			case KBI_C:
				kb_events |= KB_EVENT_C_LONG | KB_EVENT_C_REWOKE;
				break;
			case KBI_AUTO:
				kb_events |= KB_EVENT_AUTO_LONG | KB_EVENT_AUTO_REWOKE;
				break;
			case KBI_AUTO | KBI_C:
				kb_events |= KB_EVENT_LOCK_LONG;
				break;
			case KBI_PROG | KBI_C | KBI_AUTO:
				kb_events |= KB_EVENT_ALL_LONG;
				break;
			default:
				break;
			}
		}
	} //if (! state_front_prev)
}

/*!
 *******************************************************************************
 * Update mont contact status
 *
 *  - create task for keyboard scan and wake up
 *  - read keyboard status
 ******************************************************************************/
bool mont_contact_pooling(void)
{
#if THERMOTRONIC || DEBUG_IGNORE_MONT_CONTACT
	return 1; //no contact - exit!
#else
	bool mont_contact;
	enable_mont_input();
	nop(); nop();
	// crazy order of instructions, but we need any instructions
	// between PORTB setting and reading PINB due to AVR design
#if DEBUG_IGNORE_MONT_CONTACT == 1
	mont_contact = 1;
#else
	mont_contact = ~PINB & (KBI_MONT | KBI_PROG | KBI_C);
#endif
	// low active
	disable_mont_input();
	if (mont_contact & KBI_MONT)
	{
		CTL_set_error(CTL_ERR_MONTAGE);
		sumError = 0;
		return 0;
	}
	else
	{
		CTL_clear_error(CTL_ERR_MONTAGE);
		if (mont_contact & KBI_C)
		{
			return 2;
		}
		if (mont_contact & KBI_PROG)
		{
			return 3;
		}
		return 1;
	}
#endif
}


/*!
 *******************************************************************************
 * Interrupt Routine
 *
 *  - create task for keyboard scan and wake up
 *  - read keyboard status
 ******************************************************************************/
ISR(PCINT1_vect)
{
	enable_rot2_input();
	task |= TASK_KB;
	asm volatile ("nop");

	// crazy oder of instructions, bud we need any instructions
	// between PORTB setting and reading PINB due to AVR design
	keys = ~PINB
	       & (KBI_C | KBI_PROG | KBI_AUTO | KBI_ROT1 | KBI_ROT2);
	// low active

	disable_rot2_input();
}
