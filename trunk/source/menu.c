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
#include "rtc.h"
#include "motor.h"
#include "eeprom.h"


static uint8_t service_idx;

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
    menu_home, menu_home2, menu_home3, menu_home4,
    // lock message
    menu_lock,
    // service menu
    menu_service1, menu_service2,
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
    } else if (!locked) {    
        if (kb_events & KB_EVENT_ALL_LONG) { // service menu
            menu_state = menu_service1;
            kb_events = 0;
            service_idx = 0;
            ret=true;
        }
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
	int8_t wheel = wheel_proccess(); //signed number
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
        if (wheel != 0) RTC_SetYear(RTC_GetYearYY()+wheel);
        if ( kb_events & KB_EVENT_PROG) {
            menu_state = menu_set_month;
            ret=true;
        } else {
            ret = events_common();
        }
        break;
    case menu_set_month:
        if (wheel != 0) RTC_SetMonth(RTC_GetMonth()+wheel);
        if ( kb_events & KB_EVENT_PROG ) {
            menu_state = menu_set_day;
            ret=true;
        } else {
            ret = events_common();
        }
        break;
    case menu_set_day:
        if (wheel != 0) RTC_SetDay(RTC_GetDay()+wheel);
        if ( kb_events & KB_EVENT_PROG ) {
            menu_state = menu_set_hour;
            ret=true;
        } else {
            ret = events_common();
        }
        break;
    case menu_set_hour:
        if (wheel != 0) RTC_SetHour(RTC_GetHour()+wheel);
        if ( kb_events & KB_EVENT_PROG ) {
            menu_state = menu_set_minute;
            ret=true;
        } else {
            ret = events_common();
        }
        break;
    case menu_set_minute:
        if (wheel != 0) {
            RTC_SetMinute(RTC_GetMinute()+wheel);
            RTC_SetSecond(0);
        }
        if ( kb_events & KB_EVENT_PROG ) {
            menu_state = menu_home;
            ret=true;
        } else {
            ret = events_common();
        }
        break;
    case menu_home:         // home screen
    case menu_home2:        // alternate version, real temperature
    case menu_home3:        // alternate version, valve pos
    case menu_home4:        // alternate version, time    
        if (new_state) {
            menu_auto_update_timeout=10;
        }
		if (wheel != 0) {
            menu_auto_update_timeout = 10; //< \todo create symbol for constatnt
            PID_force_update = 10; //< \todo create symbol for constatnt
			temp_wanted+=wheel;
			if (temp_wanted<TEMP_MIN-1) {
				temp_wanted= TEMP_MIN-1;
			} else if (temp_wanted>TEMP_MAX+1) {
				temp_wanted= TEMP_MAX+1; 
			}
		}			 
        if (menu_auto_update_timeout==0) {
            menu_state=menu_home; // retun to main home screen
            ret=true; 
        }
        if (locked) {
            if ( kb_events & (
                    KB_EVENT_WHEEL_PLUS  | KB_EVENT_WHEEL_MINUS | KB_EVENT_PROG | KB_EVENT_C
                    | KB_EVENT_AUTO | KB_EVENT_PROG_REWOKE | KB_EVENT_C_REWOKE | KB_EVENT_AUTO_REWOKE
                    | KB_EVENT_PROG_LONG | KB_EVENT_C_LONG | KB_EVENT_AUTO_LONG )) {
                menu_state=menu_lock;
                ret=true;
                }
        } else { // not locked
            if ( kb_events & KB_EVENT_C ) {
                menu_state++;       // go to next alternate home screen
                if (menu_state > menu_home4) menu_state=menu_home;
                ret=true; 
            } else if ( kb_events & KB_EVENT_PROG_LONG ) {
    			menu_state=menu_set_year;
                ret=true; 
            }
            // TODO KB_EVENT_AUTO
            // TODO ....  
        } 
        ret = ret || events_common();
        break;
    default:
    case menu_lock:        // "bloc" message
        ret = events_common();
        if (! locked) { menu_state=menu_home; ret=true; } 
        if (new_state) menu_auto_update_timeout=LONG_PRESS_THLD+1;
        if (menu_auto_update_timeout==0) { menu_state=menu_home; ret=true; } 
        break;        
    case menu_service1:     // service menu        
    case menu_service2:        
        if ((new_state)||(wheel != 0)) menu_auto_update_timeout=20;
        if ((menu_auto_update_timeout==0) || (kb_events & KB_EVENT_AUTO)) { 
            menu_state=menu_home; 
            ret=true;
            eeprom_config_init(); //return to saved values
        }
        if (kb_events & KB_EVENT_PROG) {
            menu_auto_update_timeout=20;
            if (menu_state == menu_service2) {
                eeprom_config_save(); // save current value
                menu_state = menu_service1;
            } else {
                menu_state = menu_service2;
            }
        }
        if (menu_state == menu_service1) {
            // change index
            service_idx = (service_idx+wheel+CONFIG_RAW_SIZE)%CONFIG_RAW_SIZE;
        } else {
            // change value in RAM, to save press PROG
            int16_t min = (int16_t)config_min(service_idx);
            int16_t max_min_1 = (int16_t)(config_max(service_idx))-min+1;
            config_raw[service_idx] = (uint8_t) (
                    ((int16_t)(config_raw[service_idx])+(int16_t)wheel-min+max_min_1)%max_min_1+min);
            if (service_idx==0) { LCD_Init(); ret=true; }
        }
        ret = ret ||  events_common();
        break;        
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
 * view helpers funcion for clear display
 *******************************************************************************/
/*
    // not used generic version, easy to read but longer code
    static void clr_show(uint8_t n, ...) {
        uint8_t i;
        uint8_t *p;
        LCD_AllSegments(LCD_MODE_OFF);
        p=&n+1;
        for (i=0;i<n;i++) {
            LCD_SetSeg(*p, LCD_MODE_ON);
            p++;
        }
    }
*/


static void clr_show1(uint8_t seg1) {
    LCD_AllSegments(LCD_MODE_OFF);
    LCD_SetSeg(seg1, LCD_MODE_ON);
}

static void clr_show2(uint8_t seg1, uint8_t seg2) {
    LCD_AllSegments(LCD_MODE_OFF);
    LCD_SetSeg(seg1, LCD_MODE_ON);
    LCD_SetSeg(seg2, LCD_MODE_ON);
}

static void clr_show3(uint8_t seg1, uint8_t seg2, uint8_t seg3) {
    LCD_AllSegments(LCD_MODE_OFF);
    LCD_SetSeg(seg1, LCD_MODE_ON);
    LCD_SetSeg(seg2, LCD_MODE_ON);
    LCD_SetSeg(seg3, LCD_MODE_ON);
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
            clr_show1(LCD_SEG_COL1);
            LCD_PrintHexW(VERSION_N,LCD_MODE_ON);
        } 
        break;
    case menu_set_year:
        if (update) LCD_AllSegments(LCD_MODE_OFF); // all segments off
        LCD_PrintDecW(RTC_GetYearYYYY(),LCD_MODE_BLINK_1);
       break;
    case menu_set_month:
        if (update) clr_show1(LCD_SEG_COL1);           // decimal point
        LCD_PrintDec(RTC_GetMonth(), 2, LCD_MODE_BLINK_1);
        LCD_PrintDec(RTC_GetDay(), 0, LCD_MODE_ON);
       break;
    case menu_set_day:
        if (update) clr_show1(LCD_SEG_COL1);           // decimal point
        LCD_PrintDec(RTC_GetMonth(), 2, LCD_MODE_ON);
        LCD_PrintDec(RTC_GetDay(), 0, LCD_MODE_BLINK_1);
       break;
    case menu_set_hour:
    case menu_set_minute:
    case menu_home4: // time
        if (update) clr_show2(LCD_SEG_COL1,LCD_SEG_COL2);
        LCD_PrintDec(RTC_GetHour(), 2, ((menu_state == menu_set_hour) ? LCD_MODE_BLINK_1 : LCD_MODE_ON));
        LCD_PrintDec(RTC_GetMinute(), 0, ((menu_state == menu_set_minute) ? LCD_MODE_BLINK_1 : LCD_MODE_ON));
       break;
    case menu_home: // wanted temp, status / TODO
        if (update) clr_show2(LCD_SEG_BAR24,LCD_SEG_MANU);
        LCD_SetHourBarSeg(1, LCD_MODE_OFF); //just test TODO
        LCD_PrintTemp(temp_wanted,LCD_MODE_ON);
       break;
    case menu_home2: // real temperature
        if (update) clr_show1(LCD_SEG_COL1);           // decimal point
        LCD_PrintTempInt(temp_average,LCD_MODE_ON);
        break;
    case menu_home3: // valve pos
        if (update) LCD_AllSegments(LCD_MODE_OFF);
        // LCD_PrintDec3(MOTOR_GetPosPercent(), 1 ,LCD_MODE_ON);
        // LCD_PrintChar(LCD_CHAR_2lines,0,LCD_MODE_ON);
        {
            uint8_t prc = MOTOR_GetPosPercent();
            if (prc<=100) {
                LCD_PrintDec3(MOTOR_GetPosPercent(), 0 ,LCD_MODE_ON);
            } else {
                LCD_PrintChar(LCD_CHAR_neg,2,LCD_MODE_ON);
                LCD_PrintChar(LCD_CHAR_C  ,1,LCD_MODE_ON);
                LCD_PrintChar(LCD_CHAR_neg,0,LCD_MODE_ON);
            }
        }
        break;
    case menu_lock:        // "bloc" message
        if (update) LCD_AllSegments(LCD_MODE_OFF); // all segments off
        LCD_PrintChar(LCD_CHAR_b,3,LCD_MODE_ON);
        LCD_PrintChar(LCD_CHAR_1,2,LCD_MODE_ON);
        LCD_PrintChar(LCD_CHAR_o,1,LCD_MODE_ON);
        LCD_PrintChar(LCD_CHAR_c,0,LCD_MODE_ON);
        break;
    case menu_service1: 
    case menu_service2:
        // service menu; left side index, right value
        if (update) LCD_AllSegments(LCD_MODE_ON);
        LCD_PrintHex(service_idx, 2, ((menu_state == menu_service1) ? LCD_MODE_BLINK_1 : LCD_MODE_ON));
        LCD_PrintHex(config_raw[service_idx], 0, ((menu_state == menu_service2) ? LCD_MODE_BLINK_1 : LCD_MODE_ON));
       break;

    default:
	case menu_empty: // show temperature
        if (update) clr_show1(LCD_SEG_COL1);           // decimal point
        LCD_PrintTempInt(temp_average,LCD_MODE_ON);
        break;
    }
}
