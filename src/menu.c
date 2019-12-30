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
 * \file       menu.c
 * \brief      menu view & controler for free HR20E project
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

#include <stdint.h>

#include "config.h"
#include "main.h"
#include "keyboard.h"
#include "adc.h"
#include "lcd.h"
#include "common/rtc.h"
#include "motor.h"
#include "eeprom.h"
#include "debug.h"
#include "controller.h"
#include "menu.h"
#include "watch.h"

#if HR25
#define DISPLAY_HAS_LOCK_ICON 1
#else
#define DISPLAY_HAS_LOCK_ICON 0
#endif

/*!
 *******************************************************************************
 * \brief menu_auto_update_timeout is timer for autoupdates of menu
 * \note val<0 means no autoupdate
 ******************************************************************************/
int8_t menu_auto_update_timeout = 2;

/*!
 *******************************************************************************
 * \brief flag for locking keys of the unit
 ******************************************************************************/
bool menu_locked = false;

/*!
 *******************************************************************************
 * \brief possible states for the menu state machine
 ******************************************************************************/
typedef enum
{
	// startup
	menu_startup, menu_version,
#if (!REMOTE_SETTING_ONLY)
	// preprogramed temperatures
	menu_preset_temp0, menu_preset_temp1, menu_preset_temp2, menu_preset_temp3,
#endif
	// home screens
	menu_home_no_alter, menu_home, menu_home2, menu_home3, menu_home4,
#if MENU_SHOW_BATTERY
	menu_home5,
#endif
#if !DISPLAY_HAS_LOCK_ICON
	// lock message
	menu_lock,
#endif
	// service menu
	menu_service1, menu_service2, menu_service_watch,
#if (!REMOTE_SETTING_ONLY)
	// datetime setting
	menu_set_year, menu_set_month, menu_set_day, menu_set_hour, menu_set_minute,

	// timers setting
	menu_set_timer_dow, menu_set_timer
#endif
} menu_t;

/*!
 *******************************************************************************
 * \brief kb_events common reactions
 *
 * \returns true for controler restart
 ******************************************************************************/
static int8_t wheel_proccess(void)
{
	int8_t ret = 0;

	if (kb_events & KB_EVENT_WHEEL_PLUS)
	{
		ret = 1;
	}
	else if (kb_events & KB_EVENT_WHEEL_MINUS)
	{
		ret = -1;
	}
	return ret;
}

/*!
 *******************************************************************************
 * local menu static variables
 ******************************************************************************/
static menu_t menu_state;
static uint8_t menu_set_dow;
#define menu_set_temp menu_set_dow //reuse variable in another part of menu
static uint16_t menu_set_time;
static timermode_t menu_set_mode;
static uint8_t menu_set_slot;
static uint8_t service_idx = 0;
static uint8_t service_watch_n = 0;
static uint32_t hourbar_buff;

/*!
 *******************************************************************************
 * \brief kb_events common reactions to key presses
 *
 * \returns true to request display update
 ******************************************************************************/
static bool events_common(void)
{
	bool ret = false;

	if (kb_events & KB_EVENT_LOCK_LONG)             // key lock
	{
		menu_locked = !menu_locked;
#if !DISPLAY_HAS_LOCK_ICON
		menu_auto_update_timeout = LONG_PRESS_THLD + 1;
		menu_state = (menu_locked) ? menu_lock : menu_home;
#else
		menu_state = menu_home;
#endif
		ret = true;
	}
	else if (!menu_locked)
	{
		if (kb_events & KB_EVENT_ALL_LONG)      // service menu
		{
			menu_state = menu_service1;
			menu_auto_update_timeout = 0;
			ret = true;
#if (!REMOTE_SETTING_ONLY)
		}
		else if (kb_events & KB_EVENT_AUTO_LONG)       // set unit date/time
		{
			menu_state = menu_set_year;
			menu_auto_update_timeout = 0;
			ret = true;
		}
		else if (kb_events & KB_EVENT_PROG_LONG)       // set switching times
		{
			menu_set_dow = ((config.timer_mode == 1) ? RTC_GetDayOfWeek() : 0);
			menu_state = menu_set_timer_dow;
			ret = true;
		}
		else if (kb_events & KB_EVENT_C_LONG)       // set temperatures
		{
			menu_state = menu_preset_temp0;
			menu_set_temp = temperature_table[0];
			menu_auto_update_timeout = 0;
			ret = true;
#endif
		}
		else if ((kb_events & KB_EVENT_NONE_LONG)     // retun to main home screen on timeout
#if NO_AUTORETURN_FROM_ALT_MENUES
			 && !(((menu_state >= menu_home2) && (menu_state <= menu_home5)) || menu_state == menu_service_watch)
#endif
		)
		{
			menu_state = menu_home;
			ret = true;
		}
	}
	return ret;
}

/*!
 *******************************************************************************
 * \brief menu Controller: state transitions in the menu state machine
 *
 * \returns true to request display update
 ******************************************************************************/
bool menu_controller(void)
{
	int8_t wheel = wheel_proccess(); //signed number
	bool ret = false;

	switch (menu_state)
	{
	case menu_startup:
		if (menu_auto_update_timeout == 0)
		{
			menu_state = menu_version;
			menu_auto_update_timeout = 2;
			ret = true;
		}
		break;
	case menu_version:
		if (menu_auto_update_timeout == 0)
		{
#if (DEBUG_SKIP_DATETIME_SETTING_AFTER_RESET) || (REMOTE_SETTING_ONLY)
			menu_state = menu_home;
#else
			menu_state = menu_set_year;
#endif
			ret = true;
		}
		break;
#if (!REMOTE_SETTING_ONLY)
	case menu_set_year:
		if (wheel != 0)
		{
			RTC_SetYear(RTC_GetYearYY() + wheel);
		}
		if (kb_events & KB_EVENT_PROG)
		{
			menu_state = menu_set_month;
			menu_auto_update_timeout = 0;
			CTL_update_temp_auto();
			ret = true;
		}
		break;
	case menu_set_month:
		if (wheel != 0)
		{
			RTC_SetMonth(RTC_GetMonth() + wheel);
		}
		if (kb_events & KB_EVENT_PROG)
		{
			menu_state = menu_set_day;
			menu_auto_update_timeout = 0;
			CTL_update_temp_auto();
			ret = true;
		}
		break;
	case menu_set_day:
		if (wheel != 0)
		{
			RTC_SetDay(RTC_GetDay() + wheel);
		}
		if (kb_events & KB_EVENT_PROG)
		{
			menu_state = menu_set_hour;
			menu_auto_update_timeout = 0;
			CTL_update_temp_auto();
			ret = true;
		}
		break;
	case menu_set_hour:
		if (wheel != 0)
		{
			RTC_SetHour(RTC_GetHour() + wheel);
		}
		if (kb_events & KB_EVENT_PROG)
		{
			menu_state = menu_set_minute;
			menu_auto_update_timeout = 0;
			CTL_update_temp_auto();
			ret = true;
		}
		break;
	case menu_set_minute:
		if (wheel != 0)
		{
			RTC_SetMinute(RTC_GetMinute() + wheel);
			RTC_SetSecond(0);
		}
		if (kb_events & KB_EVENT_PROG)
		{
			menu_state = menu_home;
			CTL_update_temp_auto();
			ret = true;
		}
		break;
	case menu_set_timer_dow:
		if (wheel != 0)
		{
			menu_set_dow = (menu_set_dow + wheel + 8) % 8;
		}
		if (kb_events & KB_EVENT_PROG)     // confirm
		{
			menu_state = menu_set_timer;
			menu_set_slot = 0;
			config.timer_mode = (menu_set_dow > 0);
			eeprom_config_save((uint16_t)(&config.timer_mode) - (uint16_t)(&config)); // save value to eeprom
			menu_update_hourbar((config.timer_mode == 1) ? RTC_GetDayOfWeek() : 0);
			menu_set_time = RTC_DowTimerGet(menu_set_dow, menu_set_slot, &menu_set_mode);
			if (menu_set_time > 24 * 60)
			{
				menu_set_time = 24 * 60;
			}
			ret = true;
		}
		else if (kb_events & KB_EVENT_AUTO)       // exit without save
		{
			menu_state = menu_home;
			ret = true;
		}
		break;
	case menu_set_timer:
		if (wheel != 0)
		{
			menu_set_time = ((menu_set_time / 10 + (24 * 6 + 1) + wheel) % (24 * 6 + 1)) * 10;
		}
		if (kb_events & KB_EVENT_C)             // select temperature mode
		{
			menu_set_mode = (menu_set_mode + 5) % 4;
		}
		else if (kb_events & KB_EVENT_PROG)     // confirm
		{
			RTC_DowTimerSet(menu_set_dow, menu_set_slot, menu_set_time, menu_set_mode);
			if (++menu_set_slot >= RTC_TIMERS_PER_DOW)
			{
				if (menu_set_dow != 0)
				{
					menu_set_dow = menu_set_dow % 7 + 1;
				}
				menu_state = menu_set_timer_dow;
			}
			else
			{
				menu_set_time = RTC_DowTimerGet(menu_set_dow, menu_set_slot, &menu_set_mode);
			}
			CTL_update_temp_auto();
			menu_update_hourbar((config.timer_mode == 1) ? RTC_GetDayOfWeek() : 0);
			ret = true;
		}
		else if (kb_events & KB_EVENT_AUTO)       // exit without save
		{
			menu_state = menu_home;
			ret = true;
		}
		break;
	case menu_preset_temp0:
	case menu_preset_temp1:
	case menu_preset_temp2:
	case menu_preset_temp3:
		if (wheel != 0)
		{
			menu_set_temp += wheel;
			if (menu_set_temp > TEMP_MAX + 1)
			{
				menu_set_temp = TEMP_MAX + 1;
			}
			if (menu_set_temp < TEMP_MIN - 1)
			{
				menu_set_temp = TEMP_MIN - 1;
			}
		}
		if (kb_events & KB_EVENT_PROG)     // confirm
		{
			temperature_table[menu_state - menu_preset_temp0] = menu_set_temp;
			eeprom_config_save(menu_state + ((temperature_table - config_raw) - menu_preset_temp0));
			menu_state++; // menu_preset_temp3+1 == menu_home
			menu_auto_update_timeout = 0;
			CTL_update_temp_auto();
			if (menu_state <= menu_preset_temp3)
			{
				menu_set_temp = temperature_table[menu_state - menu_preset_temp0];
			}
			ret = true;
		}
		else if (kb_events & KB_EVENT_AUTO)       // exit without save
		{
			menu_state = menu_home;
			ret = true;
		}
		break;
#endif
	case menu_home_no_alter:        // same as home screen, but without alternate content
	case menu_home:                 // home screen
	case menu_home2:                // alternate version, real temperature
	case menu_home3:                // alternate version, valve pos
	case menu_home4:                // alternate version, time
#if MENU_SHOW_BATTERY
	case menu_home5:                // alternate version, battery
#endif
		if (kb_events & KB_EVENT_C)
		{
			menu_state++; // go to next alternate home screen
#if MENU_SHOW_BATTERY
			if (menu_state > menu_home5)
			{
				menu_state = menu_home;
			}
#else
			if (menu_state > menu_home4)
			{
				menu_state = menu_home;
			}
#endif
			ret = true;
		}
		else
		{
			if (!menu_locked)
			{
				if ((menu_state == menu_home) || (menu_state == menu_home_no_alter))
				{
					if (wheel != 0)
					{
						CTL_temp_change_inc(wheel);
						menu_state = menu_home_no_alter;
						ret = true;
					}
					if (kb_events & KB_EVENT_AUTO)
					{
						CTL_change_mode(CTL_CHANGE_MODE); // change mode
						menu_state = menu_home_no_alter;
						ret = true;
					}
					else if (kb_events & KB_EVENT_AUTO_REWOKE)
					{
						CTL_change_mode(CTL_CHANGE_MODE_REWOKE); // change mode
						menu_state = menu_home_no_alter;
						ret = true;
					}
				}
				else
				{
					if (kb_events & (
						    KB_EVENT_WHEEL_PLUS | KB_EVENT_WHEEL_MINUS | KB_EVENT_PROG
						    | KB_EVENT_AUTO | KB_EVENT_PROG_REWOKE | KB_EVENT_C_REWOKE | KB_EVENT_AUTO_REWOKE
						    | KB_EVENT_PROG_LONG | KB_EVENT_C_LONG | KB_EVENT_AUTO_LONG))
					{
						menu_state = menu_home;
						ret = true;
					}
				}
				// TODO ....
#if !DISPLAY_HAS_LOCK_ICON
			}
			else     // locked
			{
				if (kb_events & (
					    KB_EVENT_WHEEL_PLUS | KB_EVENT_WHEEL_MINUS | KB_EVENT_PROG
					    | KB_EVENT_AUTO | KB_EVENT_PROG_REWOKE | KB_EVENT_C_REWOKE | KB_EVENT_AUTO_REWOKE
					    | KB_EVENT_PROG_LONG | KB_EVENT_C_LONG | KB_EVENT_AUTO_LONG))
				{
					menu_auto_update_timeout = LONG_PRESS_THLD + 1;
					menu_state = menu_lock;
					ret = true;
				}
#endif
			}
		}
		break;
#if !DISPLAY_HAS_LOCK_ICON
	default:        // should never happen
	case menu_lock: // "bloc" message
		if (menu_auto_update_timeout == 0)
		{
			menu_state = menu_home; ret = true;
		}
		break;
#endif
	case menu_service1: // service menu
	case menu_service2:
		if (kb_events & (KB_EVENT_AUTO | KB_EVENT_C))
		{
			// leaving service menu, revert config to default value
			config_raw[service_idx] = config_value(service_idx);
			if (kb_events & KB_EVENT_AUTO)
			{
				menu_state = menu_home;
			}
			else
			{
				menu_state = menu_service_watch;
				menu_auto_update_timeout = 0;
			}
			ret = true;
		}
		else if (kb_events & KB_EVENT_PROG)                     // confirm selection
		{
			if (menu_state == menu_service2)
			{
				eeprom_config_save(service_idx);        // save current value
				menu_state = menu_service1;
				menu_auto_update_timeout = 0;
			}
			else
			{
				menu_state = menu_service2;
				menu_auto_update_timeout = 0;
			}
		}
		else                             // wheel movement
		{
			if (menu_state == menu_service1)
			{
				// change index
				service_idx = (service_idx + wheel + CONFIG_RAW_SIZE) % CONFIG_RAW_SIZE;
			}
			else
			{
				// change value in RAM, to save press PROG
				int16_t min = (int16_t)config_min(service_idx);
				int16_t max_min_1 = (int16_t)(config_max(service_idx)) - min + 1;
				config_raw[service_idx] = (uint8_t)(
					((int16_t)(config_raw[service_idx]) + (int16_t)wheel - min + max_min_1) % max_min_1 + min);
				if (service_idx == 0)
				{
					LCD_Init();
				}
			}
		}
		break;
	case menu_service_watch:
		if (kb_events & KB_EVENT_AUTO)
		{
			menu_state = menu_home;
			ret = true;
		}
		else if (kb_events & KB_EVENT_C)
		{
			menu_state = menu_service1;
			menu_auto_update_timeout = 0;
			ret = true;
		}
		else
		{
			service_watch_n = (service_watch_n + wheel + WATCH_N) % WATCH_N;
			if (wheel != 0)
			{
				ret = true;
			}
		}
		break;
	}
	if (events_common())
	{
		ret = true;
	}
	// turn off LCD blinking on wheel activity
	if (wheel != 0 && (
#if (!REMOTE_SETTING_ONLY)
		    (menu_state >= menu_set_year && menu_state <= menu_set_timer) ||
		    (menu_state >= menu_preset_temp0 && menu_state <= menu_preset_temp3) ||
#endif
		    (menu_state >= menu_service1 && menu_state <= menu_service_watch)))
	{
		menu_auto_update_timeout = 2;
	}
	kb_events = 0; // clear unused keys
	return ret;
}

/*!
 *******************************************************************************
 * view helper funcions for clear display
 *******************************************************************************/
/*
 *  // not used generic version, easy to read but longer code
 *  static void clr_show(uint8_t n, ...) {
 *      uint8_t i;
 *      uint8_t *p;
 *      LCD_AllSegments(LCD_MODE_OFF);
 *      p=&n+1;
 *      for (i=0;i<n;i++) {
 *          LCD_SetSeg(*p, LCD_MODE_ON);
 *          p++;
 *      }
 *  }
 */
static void clr_show1(uint8_t seg1)
{
	LCD_AllSegments(LCD_MODE_OFF);
	LCD_SetSeg(seg1, LCD_MODE_ON);
}

static void clr_show2(uint8_t seg1, uint8_t seg2)
{
	LCD_AllSegments(LCD_MODE_OFF);
	LCD_SetSeg(seg1, LCD_MODE_ON);
	LCD_SetSeg(seg2, LCD_MODE_ON);
}
#if HR25
static void clr_show3(uint8_t seg1, uint8_t seg2, uint8_t seg3)
{
	LCD_AllSegments(LCD_MODE_OFF);
	LCD_SetSeg(seg1, LCD_MODE_ON);
	LCD_SetSeg(seg2, LCD_MODE_ON);
	LCD_SetSeg(seg3, LCD_MODE_ON);
}
#endif
static void show_selected_temperature_type(uint8_t type, uint8_t mode)
{
	//  temperature0    SNOW
	//  temperature1         MOON
	//  temperature2         MOON SUN
	//  temperature3              SUN
	LCD_SetSeg(LCD_SEG_SNOW, ((type == temperature0)
				  ? mode : LCD_MODE_OFF));
	LCD_SetSeg(LCD_SEG_MOON, (((type == temperature1) || (type == temperature2))
				  ? mode : LCD_MODE_OFF));
	LCD_SetSeg(LCD_SEG_SUN, ((type >= temperature2)
				 ? mode : LCD_MODE_OFF));
}

/*!
 *******************************************************************************
 * \brief Update hourbar bitmap buffer to be used by LCD update
 *
 * \returns true for controler restart
 ******************************************************************************/
void menu_update_hourbar(uint8_t dow)
{
	hourbar_buff = RTC_DowTimerGetHourBar(dow);
}

/*!
 *******************************************************************************
 * \brief menu View
 ******************************************************************************/
void menu_view(bool clear)
{
	uint8_t lcd_blink_mode = menu_auto_update_timeout > 0 ? LCD_MODE_ON : LCD_MODE_BLINK_1;

	switch (menu_state)
	{
	case menu_startup:
		LCD_AllSegments(LCD_MODE_ON);           // all segments on
		break;
	case menu_version:
		clr_show1(LCD_DECIMAL_DOT);
		LCD_PrintHexW(VERSION_N, LCD_MODE_ON);
		break;
#if (!REMOTE_SETTING_ONLY)
	case menu_set_year:
		LCD_AllSegments(LCD_MODE_OFF); // all segments off
		LCD_PrintDecW(RTC_GetYearYYYY(), lcd_blink_mode);
		break;
	case menu_set_month:
	case menu_set_day:
		clr_show1(LCD_DECIMAL_DOT);   // decimal point
		LCD_PrintDec(RTC_GetMonth(), 0, ((menu_state == menu_set_month) ? lcd_blink_mode : LCD_MODE_ON));
		LCD_PrintDec(RTC_GetDay(), 2, ((menu_state == menu_set_day) ? lcd_blink_mode : LCD_MODE_ON));
		break;
	case menu_set_hour:
	case menu_set_minute:
	case menu_home4: // time
		if (clear)
		{
			clr_show2(LCD_SEG_COL1, LCD_SEG_COL2);
		}
		LCD_PrintDec(RTC_GetHour(), 2, ((menu_state == menu_set_hour) ? lcd_blink_mode : LCD_MODE_ON));
		LCD_PrintDec(RTC_GetMinute(), 0, ((menu_state == menu_set_minute) ? lcd_blink_mode : LCD_MODE_ON));
		break;
	case menu_set_timer_dow:
		clr_show1(LCD_SEG_PROG); // all segments off
		LCD_PrintDayOfWeek(menu_set_dow, lcd_blink_mode);
#if HR25
		// day of week icon
		if (menu_set_dow == 0)
		{
			// we rely on the fact that all day segments are in the same register!
			LCD_SetSegReg(LCD_SEG_D1 / 8, 0x7F, LCD_MODE_ON);
		}
		else
		{
			LCD_SetSeg(LCD_SEG_D1 + menu_set_dow - 1, LCD_MODE_ON);
		}
#endif
		break;
	case menu_set_timer:
		//! \todo calculate "hourbar" status, actual position in mode LCD_MODE_BLINK_1
#if HR25
		clr_show3(LCD_SEG_BAR24, LCD_SEG_BAR_SUB, LCD_SEG_PROG);
#else
		clr_show2(LCD_SEG_BAR24, LCD_SEG_PROG);
#endif
		// "patch" eeprom to show the value selected by user, not saved to eeprom yet
		timers_patch_offset = timers_get_raw_index(menu_set_dow, menu_set_slot);
		timers_patch_data = menu_set_time + ((uint16_t)menu_set_mode << 12);
		LCD_HourBarBitmap(RTC_DowTimerGetHourBar(menu_set_dow));
		timers_patch_offset = 0xff;

		LCD_SetHourBarSeg(menu_set_time / 60, lcd_blink_mode);
		LCD_SetSeg(LCD_SEG_COL1, lcd_blink_mode);
		LCD_SetSeg(LCD_SEG_COL2, lcd_blink_mode);
		if (menu_set_time < 24 * 60)
		{
			LCD_PrintDec(menu_set_time / 60, 2, lcd_blink_mode);
			LCD_PrintDec(menu_set_time % 60, 0, lcd_blink_mode);
		}
		else
		{
			LCD_PrintStringID(LCD_STRING_4xminus, lcd_blink_mode);
		}
		show_selected_temperature_type(menu_set_mode, LCD_MODE_ON);
#if HR25
		// day of week icon
		if (menu_set_dow == 0)
		{
			// we rely on the fact that all day segments are in the same register!
			LCD_SetSegReg(LCD_SEG_D1 / 8, 0x7F, LCD_MODE_ON);
		}
		else
		{
			LCD_SetSeg(LCD_SEG_D1 + menu_set_dow - 1, LCD_MODE_ON);
		}
#endif
		break;
	case menu_preset_temp0:
	case menu_preset_temp1:
	case menu_preset_temp2:
	case menu_preset_temp3:
		LCD_AllSegments(LCD_MODE_OFF);
		LCD_PrintTemp(menu_set_temp, lcd_blink_mode);
		show_selected_temperature_type(menu_state - menu_preset_temp0, LCD_MODE_ON);
		break;
#else
	case menu_home4: // time
		if (clear)
		{
			clr_show2(LCD_SEG_COL1, LCD_SEG_COL2);
		}
		LCD_PrintDec(RTC_GetHour(), 2, LCD_MODE_ON);
		LCD_PrintDec(RTC_GetMinute(), 0, LCD_MODE_ON);
		break;
#endif
#if MENU_SHOW_BATTERY
	case menu_home5:        // battery
		LCD_AllSegments(LCD_MODE_OFF);
		LCD_PrintDec(bat_average / 100, 2, LCD_MODE_ON);
		LCD_PrintDec(bat_average % 100, 0, LCD_MODE_ON);
		break;
#endif


	case menu_home: // wanted temp / error code / adaptation status
		if (clear)
		{
			clr_show1(CTL_mode_auto ? LCD_SEG_AUTO : LCD_SEG_MANU);
		}
		if (MOTOR_calibration_step > 0)
		{
			LCD_PrintChar(LCD_CHAR_A, 3, LCD_MODE_ON);
// Ignore strict aliasing for MOTOR_ManuCalibration
// (saved in EEPROM as single bytes and accessed as int16)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
			if (MOTOR_ManuCalibration == -1)
			{
				LCD_PrintChar(LCD_CHAR_d, 2, LCD_MODE_ON);
			}
#pragma GCC diagnostic pop
			LCD_PrintChar(MOTOR_calibration_step % 10, 0, LCD_MODE_ON);
			break;
		}
		else
		{
#if !HR25
			if (CTL_error == 0)
			{
				if (mode_window())
				{
					LCD_PrintStringID(LCD_STRING_OPEn, LCD_MODE_ON);
					break;
				}
			}
			else
			{
				if (CTL_error & CTL_ERR_BATT_LOW)
				{
					LCD_PrintStringID(LCD_STRING_BAtt, LCD_MODE_BLINK_1);
				}
				else if (CTL_error & CTL_ERR_BATT_WARNING)
				{
					LCD_PrintStringID(LCD_STRING_BAtt, LCD_MODE_ON);
				}
				else
#else
			// HR25 reports battery and open window by special LCD segments
			if ((CTL_error & ~(CTL_ERR_BATT_LOW | CTL_ERR_BATT_WARNING)) != 0)
			{
#endif
				if (CTL_error & CTL_ERR_MONTAGE)
				{
					LCD_PrintStringID(LCD_STRING_E2, LCD_MODE_ON);
				}
				else if (CTL_error & CTL_ERR_MOTOR)
				{
					LCD_PrintStringID(LCD_STRING_E3, LCD_MODE_ON);
				}
				else if (CTL_error & CTL_ERR_RFM_SYNC)
				{
					LCD_PrintStringID(LCD_STRING_E4, LCD_MODE_ON);
				}
				break;
			}
		}
	// do not use break at this position / optimization
	case menu_home_no_alter: // wanted temp
		if (clear)
		{
			if (CTL_mode_auto)
			{
#if HR25
				clr_show3(LCD_SEG_AUTO, LCD_SEG_BAR24, LCD_SEG_BAR_SUB);
#else
				clr_show2(LCD_SEG_AUTO, LCD_SEG_BAR24);
#endif
				//! \note hourbar status calculation is complex we don't want calculate it every view, use cache
				LCD_HourBarBitmap(hourbar_buff);
			}
			else
			{
				clr_show1(LCD_SEG_MANU);
			}
		}
#if HR25
		// day of week icon
		LCD_SetSeg(LCD_SEG_D1 + RTC_GetDayOfWeek() - 1, LCD_MODE_ON);
#endif
		// display active temperature type in automatic mode
		if (CTL_test_auto())
		{
			show_selected_temperature_type(CTL_temp_auto_type, LCD_MODE_ON);
		}
		LCD_PrintTemp(CTL_temp_wanted, LCD_MODE_ON);
		break;
	case menu_home2: // real temperature
		if (clear)
		{
			LCD_AllSegments(LCD_MODE_OFF);
		}
		LCD_PrintTempInt(temp_average, LCD_MODE_ON);
		break;
	case menu_home3: // valve pos
		if (clear)
		{
			LCD_AllSegments(LCD_MODE_OFF);
		}
		// LCD_PrintDec3(MOTOR_GetPosPercent(), 1 ,LCD_MODE_ON);
		// LCD_PrintChar(LCD_CHAR_2lines,0,LCD_MODE_ON);
		{
			uint8_t prc = MOTOR_GetPosPercent();
			if (prc <= 100)
			{
				LCD_PrintDec3(MOTOR_GetPosPercent(), 0, LCD_MODE_ON);
#if HR25
				// percent sign
				LCD_SetSeg(LCD_SEG_PERCENT, LCD_MODE_ON);
#endif
			}
			else
			{
				LCD_PrintStringID(LCD_STRING_minusCminus, LCD_MODE_ON);
			}
		}
		break;
#if !DISPLAY_HAS_LOCK_ICON
	case menu_lock:                         // "bloc" message
		LCD_AllSegments(LCD_MODE_OFF);  // all segments off
		LCD_PrintStringID(LCD_STRING_bloc, LCD_MODE_ON);
		break;
#endif
	case menu_service1:
	case menu_service2:
		// service menu; left side index, right value
		LCD_AllSegments(LCD_MODE_ON);
		LCD_PrintHex(service_idx, 2, ((menu_state == menu_service1) ? lcd_blink_mode : LCD_MODE_ON));
		LCD_PrintHex(config_raw[service_idx], 0, ((menu_state == menu_service2) ? lcd_blink_mode : LCD_MODE_ON));
		break;
	case menu_service_watch:
		LCD_AllSegments(LCD_MODE_ON);
		LCD_PrintHexW(watch(service_watch_n), LCD_MODE_ON);
		LCD_SetHourBarSeg(service_watch_n, lcd_blink_mode);
		break;
	default:
		break;
	}

#if DISPLAY_HAS_LOCK_ICON
	if (menu_locked)
	{
		LCD_SetSeg(LCD_PADLOCK, LCD_MODE_ON);
	}
#endif
#if HR25
	if (mode_window())
	{
		LCD_SetSeg(LCD_SEG_DOOR_OPEN, LCD_MODE_ON);
	}
	if (bat_average < 20 * (uint16_t)config.bat_low_thld)
	{
		// do not blink outline when all segments are on
		if (menu_state != menu_startup &&
		    (menu_state < menu_service1 || menu_state > menu_service_watch))
		{
			LCD_SetSeg(LCD_SEG_BAT_OUTLINE, LCD_MODE_BLINK_1);
		}
	}
	else
	{
		LCD_SetSeg(LCD_SEG_BAT_OUTLINE, LCD_MODE_ON);
		if (bat_average > 20 * (uint16_t)config.bat_warning_thld)
		{
			LCD_SetSeg(LCD_SEG_BAT_BOTTOM, LCD_MODE_ON);
		}
		if (bat_average > 20 * (uint16_t)config.bat_half_thld)
		{
			LCD_SetSeg(LCD_SEG_BAT_TOP, LCD_MODE_ON);
		}
	}
#endif
	LCD_Update();
}

/*!
 * \note Switch used in this file can generate false warning on some AvrGCC versions
 *       we can ignore it
 *       details:  http://osdir.com/ml/hardware.avr.libc.devel/2006-11/msg00005.html
 */
