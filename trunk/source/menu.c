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
 * \date       06.10.2008
 * $Rev: 70 $
 */

#include <stdint.h>

#include "config.h"
#include "main.h"
#include "keyboard.h"
#include "adc.h"
#include "lcd.h"
#include "rtc.h"


/*!
 *******************************************************************************
 * menu_auto_update_timeout is timer for autoupdates of menu
 * val<0 means no autoupdate 
 ******************************************************************************/
uint8_t menu_auto_update_timeout = 2;

typedef enum {
    // startup
    menu_startup, menu_version,
    // home screens
    menu_home, menu_home2, menu_home3,
    // lock message
    menu_lock,
    // service menu
    menu_service,
    // datetime setting
    menu_set_year, menu_set_month, menu_set_day, menu_set_hour, menu_set_minute,
    
    menu_empty
} menu_t;

menu_t menu_state;
static bool locked = false; 


/*!
 *******************************************************************************
 * kb_events common reactions
 * 
 * \returns true for controler restart
 ******************************************************************************/

static int8_t wheel_proccess(void) {
    int8_t ret=0;
    if (kb_events & KB_EVENT_WHEEL_PLUS) {
        ret= 1;
    } else if (kb_events & KB_EVENT_WHEEL_MINUS) {
        ret= -1;
    } 
    return ret;
}

/*!
 *******************************************************************************
 * kb_events common reactions
 * 
 * \returns true for controler restart
 ******************************************************************************/

static bool events_common(void) {
    bool ret=false;
    if (kb_events & KB_EVENT_LOCK_LONG) {
        menu_state = menu_lock;
        locked = ! locked;
        kb_events = 0;
        ret=true;
    } else if (kb_events & KB_EVENT_ALL_LONG) { // service menu
        menu_state = menu_service;
        kb_events = 0;
        ret=true;
    }
    return ret;
}

/*!
 *******************************************************************************
 * menu Controller
 * 
 * \returns true for controler restart  
 ******************************************************************************/
bool menu_controller(bool new_state) {
	int8_t whell = wheel_proccess(); //signed number
	bool ret = false;
    switch (menu_state) {
    case menu_startup:
        if (new_state) {
            menu_auto_update_timeout=2;
        }
        if (menu_auto_update_timeout==0) {
            menu_state = menu_version;
            ret=true;
        }
        break;
    case menu_version:
        if (new_state) {
            menu_auto_update_timeout=2;
        }
        if (menu_auto_update_timeout==0) {
            #if DEBUG_SKIP_DATETIME_SETTING_AFTER_RESET
                menu_state = menu_home;
            #else
                menu_state = menu_set_year;
            #endif
            ret=true;
        }
        break;
    case menu_set_year:
        if (whell != 0) RTC_SetYear(RTC_GetYearYY()+whell);
        if ( kb_events & KB_EVENT_PROG) {
            menu_state = menu_set_month;
            ret=true;
        } else {
            ret = events_common();
        }
        break;
    case menu_set_month:
        if (whell != 0) RTC_SetMonth(RTC_GetMonth()+whell);
        if ( kb_events & KB_EVENT_PROG ) {
            menu_state = menu_set_day;
            ret=true;
        } else {
            ret = events_common();
        }
        break;
    case menu_set_day:
        if (whell != 0) RTC_SetDay(RTC_GetDay()+whell);
        if ( kb_events & KB_EVENT_PROG ) {
            menu_state = menu_set_hour;
            ret=true;
        } else {
            ret = events_common();
        }
        break;
    case menu_set_hour:
        if (whell != 0) RTC_SetHour(RTC_GetHour()+whell);
        if ( kb_events & KB_EVENT_PROG ) {
            menu_state = menu_set_minute;
            ret=true;
        } else {
            ret = events_common();
        }
        break;
    case menu_set_minute:
        if (whell != 0) {
            RTC_SetMinute(RTC_GetMinute()+whell);
            RTC_SetSecond(0);
        }
        if ( kb_events & KB_EVENT_PROG ) {
            menu_state = menu_empty;
            ret=true;
        } else {
            ret = events_common();
        }
        break;
    default:
	case menu_empty: // show temperature, blackhole
        if (new_state || (menu_auto_update_timeout==0)) {
            menu_auto_update_timeout=1;
            // nothing todo, it is blackhole, just update value
        }
        //empty        
    }
    kb_events = 0; // clear unused keys
    return ret;
} 

/*!
 *******************************************************************************
 * menu View
 ******************************************************************************/
void menu_view(bool update) {
  switch (menu_state) {
    case menu_startup:
        if (update) {
            LCD_AllSegments(LCD_MODE_ON);                   // all segments on
        }
        break;
    case menu_version:
        if (update) {
            LCD_AllSegments(LCD_MODE_OFF);                   // all segments off
            LCD_PrintHexW(VERSION_N,LCD_MODE_ON);
            LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_ON);           // decimal point
        }
        break;
    case menu_set_year:
        if (update) {
            LCD_AllSegments(LCD_MODE_OFF);                   // all segments off
        }
        LCD_PrintDecW(RTC_GetYearYYYY(),LCD_MODE_BLINK_1);
       break;
    case menu_set_month:
        if (update) {
            LCD_AllSegments(LCD_MODE_OFF);                   // all segments off
            LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_ON);           // decimal point
        }
        LCD_PrintDec(RTC_GetMonth(), 2, LCD_MODE_BLINK_1);
        LCD_PrintDec(RTC_GetDay(), 0, LCD_MODE_ON);
       break;
    case menu_set_day:
        if (update) {
            LCD_AllSegments(LCD_MODE_OFF);                   // all segments off
            LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_ON);           // decimal point
        }
        LCD_PrintDec(RTC_GetMonth(), 2, LCD_MODE_ON);
        LCD_PrintDec(RTC_GetDay(), 0, LCD_MODE_BLINK_1);
       break;
    case menu_set_hour:
        if (update) {
            LCD_AllSegments(LCD_MODE_OFF);                   // all segments off
            LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_ON);           // decimal point
            LCD_SetSeg(LCD_SEG_COL2, LCD_MODE_ON);           // decimal point
        }
        LCD_PrintDec(RTC_GetHour(), 2, LCD_MODE_BLINK_1);
        LCD_PrintDec(RTC_GetMinute(), 0, LCD_MODE_ON);
       break;
    case menu_set_minute:
        if (update) {
            LCD_AllSegments(LCD_MODE_OFF);                   // all segments off
            LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_ON);           // decimal point
        }
        LCD_PrintDec(RTC_GetHour(), 2, LCD_MODE_ON);
        LCD_PrintDec(RTC_GetMinute(), 0, LCD_MODE_BLINK_1);
       break;
    default:
	case menu_empty: // show temperature
        if (update) {
            LCD_AllSegments(LCD_MODE_OFF);                   // all segments off
            LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_ON);           // decimal point
        }
        LCD_PrintDecW(temp_average,LCD_MODE_ON);
        break;
    }
}
