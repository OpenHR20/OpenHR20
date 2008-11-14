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
#include "debug.h"
#include "controller.h"
#include "menu.h"
#include "watch.h"

static int8_t service_idx=-1;

/*!
 *******************************************************************************
 * \brief menu_auto_update_timeout is timer for autoupdates of menu
 * \note val<0 means no autoupdate 
 ******************************************************************************/
uint8_t menu_auto_update_timeout = 2;

typedef enum {
    // startup
    menu_startup, menu_version,
    // preprogramed temperatures
    menu_preset_temp0, menu_preset_temp1, menu_preset_temp2, menu_preset_temp3, 
    // home screens
    menu_home_no_alter, menu_home, menu_home2, menu_home3, menu_home4,
    // lock message
    menu_lock,
    // service menu
    menu_service1, menu_service2, menu_service_watch,
    // datetime setting
    menu_set_year, menu_set_month, menu_set_day, menu_set_hour, menu_set_minute,

    // datetime setting
    menu_set_timmer_dow_start, menu_set_timmer_dow, menu_set_timmer,
    
    
    
    menu_empty
} menu_t;

menu_t menu_state;
static bool locked = false; 
uint32_t hourbar_buff;

/*!
 *******************************************************************************
 * \brief kb_events common reactions
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
 * \brief kb_events common reactions
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
        } else if ( kb_events & KB_EVENT_AUTO_LONG ) {
    		menu_state=menu_set_year;
            ret=true; 
        } else if ( kb_events & KB_EVENT_PROG_LONG ) {
			menu_state=menu_set_timmer_dow_start;
            ret=true; 
        } else if ( kb_events & KB_EVENT_NONE_LONG ) {
            menu_state=menu_home; // retun to main home screen
            ret=true;
        } else if ( kb_events & KB_EVENT_C_LONG ) {
            menu_state=menu_preset_temp0; 
            ret=true;
        }

	}
    return ret;
}

/*!
 *******************************************************************************
 * local menu static variables
 ******************************************************************************/
static uint8_t menu_set_dow;
#define menu_set_temp menu_set_dow //reuse variable in another part of menu
static uint16_t menu_set_time;
static timermode_t menu_set_mode;
static uint8_t menu_set_slot;
static uint8_t service_watch_n = 0;

/*!
 *******************************************************************************
 * \brief menu Controller
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
        }
        break;
    case menu_set_month:
        if (wheel != 0) RTC_SetMonth(RTC_GetMonth()+wheel);
        if ( kb_events & KB_EVENT_PROG ) {
            menu_state = menu_set_day;
            ret=true;
        }
        break;
    case menu_set_day:
        if (wheel != 0) RTC_SetDay(RTC_GetDay()+wheel);
        if ( kb_events & KB_EVENT_PROG ) {
            menu_state = menu_set_hour;
            ret=true;
        }
        break;
    case menu_set_hour:
        if (wheel != 0) RTC_SetHour(RTC_GetHour()+wheel);
        if ( kb_events & KB_EVENT_PROG ) {
            menu_state = menu_set_minute;
            ret=true;
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
        }
        break;
    case menu_home_no_alter: // same as home screen, but without alternate contend
    case menu_home:         // home screen
    case menu_home2:        // alternate version, real temperature
    case menu_home3:        // alternate version, valve pos
    case menu_home4:        // alternate version, time    
        if (locked) {
            if ( kb_events & (
                    KB_EVENT_WHEEL_PLUS  | KB_EVENT_WHEEL_MINUS | KB_EVENT_PROG | KB_EVENT_C
                    | KB_EVENT_AUTO | KB_EVENT_PROG_REWOKE | KB_EVENT_C_REWOKE | KB_EVENT_AUTO_REWOKE
                    | KB_EVENT_PROG_LONG | KB_EVENT_C_LONG | KB_EVENT_AUTO_LONG )) {
                menu_state=menu_lock;
                ret=true;
                }
        } else { // not locked
			if ((wheel != 0) &&   ((menu_state == menu_home) 
							    || (menu_state == menu_home_no_alter))) {
				CTL_temp_change_inc(wheel);
				menu_state = menu_home_no_alter;
			}			 
            if ( kb_events & KB_EVENT_C ) {
                menu_state++;       // go to next alternate home screen
                if (menu_state > menu_home4) menu_state=menu_home;
                ret=true; 
            } 
            if ( kb_events & KB_EVENT_AUTO ) {
                CTL_change_mode(CTL_CHANGE_MODE); // change mode
                menu_state=menu_home_no_alter;
                ret=true; 
            }
            if ( kb_events & KB_EVENT_AUTO_REWOKE ) {
                CTL_change_mode(CTL_CHANGE_MODE_REWOKE); // change mode
                menu_state=menu_home_no_alter;
                ret=true; 
            }
            // TODO ....  
        } 
        break;
    case menu_set_timmer_dow_start:
        if (new_state) menu_set_dow=((config.timer_mode==1)?RTC_GetDayOfWeek():0);
        menu_state = menu_set_timmer_dow;
        // do not use break here
    case menu_set_timmer_dow:
		if (wheel != 0) menu_set_dow=(menu_set_dow+wheel+8)%8;
        if ( kb_events & KB_EVENT_PROG ) {
            menu_state=menu_set_timmer;
            menu_set_slot=0;
            config.timer_mode = (menu_set_dow>0);
            eeprom_config_save((uint16_t)(&config.timer_mode)-(uint16_t)(&config)); // save value to eeprom
            	// update hourbar
        	menu_update_hourbar((config.timer_mode==1)?RTC_DOW:0);
            ret=true; 
        }
        if ( kb_events & KB_EVENT_AUTO ) { // exit without save
            menu_state=menu_home;
            ret=true; 
        }
        break;        
    case menu_set_timmer:
        if (new_state) {
            menu_set_time= RTC_DowTimerGet(menu_set_dow, menu_set_slot, &menu_set_mode);
			if (menu_set_time>24*60) menu_set_time=24*60;
        }
        if (wheel != 0) {
            menu_set_time=((menu_set_time/10+(24*6+1)+wheel)%(24*6+1))*10;
        }
        if ( kb_events & KB_EVENT_C ) {
            menu_set_mode=(menu_set_mode+5)%4;
        }
        if ( kb_events & KB_EVENT_PROG ) {
            RTC_DowTimerSet(menu_set_dow, menu_set_slot, menu_set_time, menu_set_mode);
            if (++menu_set_slot>=RTC_TIMERS_PER_DOW) {
                if (menu_set_dow!=0) menu_set_dow=menu_set_dow%7+1; 
                menu_state=menu_set_timmer_dow;
            }
            CTL_update_temp_auto();
        	menu_update_hourbar((config.timer_mode==1)?RTC_DOW:0);
            ret=true; 
        }
        if ( kb_events & KB_EVENT_AUTO ) { // exit without save
            menu_state=menu_home;
            ret=true; 
        }
        break;
    case menu_preset_temp0:
    case menu_preset_temp1:
    case menu_preset_temp2:
    case menu_preset_temp3:
        if (new_state) menu_set_temp=temperature_table[menu_state-menu_preset_temp0];
        if (wheel != 0) {
            menu_set_temp+=wheel;
            if (menu_set_temp > TEMP_MAX+1) menu_set_temp = TEMP_MAX+1;
            if (menu_set_temp < TEMP_MIN-1) menu_set_temp = TEMP_MIN-1;
        }
        if ( kb_events & KB_EVENT_PROG ) {
            temperature_table[menu_state-menu_preset_temp0]=menu_set_temp;
            eeprom_config_save(menu_state+((temperature_table-config_raw)-menu_preset_temp0));
            menu_state++; // menu_preset_temp3+1 == menu_home
            CTL_update_temp_auto();
            ret=true; 
        }
        if ( kb_events & KB_EVENT_AUTO ) { // exit without save
            menu_state=menu_home;
            ret=true; 
        }
		break;

    default:
    case menu_lock:        // "bloc" message
        if (! locked) { menu_state=menu_home; ret=true; } 
        if (new_state) menu_auto_update_timeout=LONG_PRESS_THLD+1;
        if (menu_auto_update_timeout==0) { menu_state=menu_home; ret=true; } 
        break;        
    case menu_service1:     // service menu        
    case menu_service2:        
        if (kb_events & KB_EVENT_AUTO) { 
            menu_state=menu_home; 
            ret=true;
        }
        if (kb_events & KB_EVENT_C) { 
            menu_state=menu_service_watch; 
            ret=true;
        }
        if (kb_events & KB_EVENT_PROG) {
            if (menu_state == menu_service2) {
                eeprom_config_save(service_idx); // save current value
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
        break;        
	case menu_service_watch:
        if (kb_events & KB_EVENT_AUTO) { 
            menu_state=menu_home; 
            ret=true;
        }
        if (kb_events & KB_EVENT_C) { 
            menu_state=menu_service1; 
            ret=true;
        }
        service_watch_n=(service_watch_n+wheel+WATCH_N)%WATCH_N;
        if (wheel != 0) ret=true;
        break;
	    
	case menu_empty: // show temperature, blackhole
        if (new_state) {
            menu_auto_update_timeout=1;
            // nothing todo, it is blackhole, just update value
        }
        //empty        
    }
    if (events_common()) ret=true;
    if (ret && (service_idx>=0) && (service_idx<CONFIG_RAW_SIZE)) {
        // back to default value
        config_raw[service_idx] = config_value(service_idx);
        service_idx = -1;
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

static void show_selected_temperature_type (uint8_t type, uint8_t mode) {
    //  temperature0    SNOW         
    //  temperature1         MOON    
    //  temperature2         MOON SUN
    //  temperature3              SUN
    LCD_SetSeg(LCD_SEG_SNOW,((type==temperature0)
        ?mode:LCD_MODE_OFF));
    LCD_SetSeg(LCD_SEG_MOON,(((type==temperature1)||(type==temperature2))
        ?mode:LCD_MODE_OFF));
    LCD_SetSeg(LCD_SEG_SUN,((type>=temperature2)
        ?mode:LCD_MODE_OFF));
}

/*!
 *******************************************************************************
 * \brief menu View
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
    case menu_set_day:
        if (update) clr_show1(LCD_SEG_COL1);           // decimal point
        LCD_PrintDec(RTC_GetMonth(), 0, ((menu_state==menu_set_month)?LCD_MODE_BLINK_1:LCD_MODE_ON));
        LCD_PrintDec(RTC_GetDay(), 2, ((menu_state==menu_set_day)?LCD_MODE_BLINK_1:LCD_MODE_ON));
       break;
    case menu_set_hour:
    case menu_set_minute: 
    case menu_home4: // time
        if (update) clr_show2(LCD_SEG_COL1,LCD_SEG_COL2);
        LCD_PrintDec(RTC_GetHour(), 2, ((menu_state == menu_set_hour) ? LCD_MODE_BLINK_1 : LCD_MODE_ON));
        LCD_PrintDec(RTC_GetMinute(), 0, ((menu_state == menu_set_minute) ? LCD_MODE_BLINK_1 : LCD_MODE_ON));
       break;                                                               
    case menu_home: // wanted temp / error code / adaptation status
        if (MOTOR_calibration_step>0) {
            clr_show1(LCD_SEG_BAR24);
            LCD_PrintChar(LCD_CHAR_A,3,LCD_MODE_ON);
            if (MOTOR_ManuCalibration==-1) LCD_PrintChar(LCD_CHAR_d,2,LCD_MODE_ON);
            LCD_PrintChar(MOTOR_calibration_step%10, 0, LCD_MODE_ON);
            goto MENU_COMMON_STATUS; // optimization
        } else {
            if (update) clr_show1(LCD_SEG_BAR24);
            if (CTL_error!=0) {
                if (CTL_error & CTL_ERR_BATT_LOW) {
                    LCD_PrintStringID(LCD_STRING_BAtt,LCD_MODE_BLINK_1);
                } else if (CTL_error & CTL_ERR_MONTAGE) {
                    LCD_PrintStringID(LCD_STRING_E2,LCD_MODE_ON);
                } else if (CTL_error & CTL_ERR_MOTOR) {
                    LCD_PrintStringID(LCD_STRING_E3,LCD_MODE_ON);
                } else if (CTL_error & CTL_ERR_BATT_WARNING) {
                    LCD_PrintStringID(LCD_STRING_BAtt,LCD_MODE_ON);
                }
                goto MENU_COMMON_STATUS; // optimization
            } else {
                if (CTL_mode_window) {
                    LCD_PrintStringID(LCD_STRING_OPEn,LCD_MODE_ON);
                    goto MENU_COMMON_STATUS; // optimization
                }
            }
        } 
        // do not use break at this position / optimization
    case menu_home_no_alter: // wanted temp
        if (update) clr_show1(LCD_SEG_BAR24);
        LCD_PrintTemp(CTL_temp_wanted,LCD_MODE_ON);
        //! \note hourbar status calculation is complex we don't want calculate it every view, use chache
        MENU_COMMON_STATUS:
        LCD_SetSeg(LCD_SEG_AUTO, (CTL_test_auto()?LCD_MODE_ON:LCD_MODE_OFF));
        LCD_SetSeg(LCD_SEG_MANU, (CTL_mode_auto?LCD_MODE_OFF:LCD_MODE_ON));
        LCD_HourBarBitmap(hourbar_buff);
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
                LCD_PrintStringID(LCD_STRING_minusCminus,LCD_MODE_ON);
            }
        }
        break;                                                             
    case menu_set_timmer_dow:
        if (update) clr_show1(LCD_SEG_PROG); // all segments off
        LCD_PrintDayOfWeek(menu_set_dow, LCD_MODE_BLINK_1);
        break;
    case menu_set_timmer:
        //! \todo calculate "hourbar" status, actual position in mode LCD_MODE_BLINK_1
        
        if (update) clr_show3(LCD_SEG_COL1,LCD_SEG_COL2,LCD_SEG_PROG);
        timmers_patch_offset=timers_get_raw_index(menu_set_dow, menu_set_slot);
        timmers_patch_data = menu_set_time +  ((uint16_t)menu_set_mode<<12);
        LCD_HourBarBitmap(RTC_DowTimerGetHourBar(menu_set_dow));
        timmers_patch_offset=0xff;
        LCD_SetHourBarSeg(menu_set_time/60, LCD_MODE_BLINK_1);
        if (menu_set_time < 24*60) {
            LCD_PrintDec(menu_set_time/60, 2, LCD_MODE_BLINK_1);
            LCD_PrintDec(menu_set_time%60, 0, LCD_MODE_BLINK_1);        
        } else {
            LCD_PrintStringID(LCD_STRING_4xminus,LCD_MODE_BLINK_1);
        }
        show_selected_temperature_type(menu_set_mode,LCD_MODE_BLINK_1);
        break;                                                             
    case menu_lock:        // "bloc" message
        if (update) LCD_AllSegments(LCD_MODE_OFF); // all segments off
        LCD_PrintStringID(LCD_STRING_bloc,LCD_MODE_ON);
        break;
    case menu_service1: 
    case menu_service2:
        // service menu; left side index, right value
        if (update) LCD_AllSegments(LCD_MODE_ON);
        LCD_PrintHex(service_idx, 2, ((menu_state == menu_service1) ? LCD_MODE_BLINK_1 : LCD_MODE_ON));
        LCD_PrintHex(config_raw[service_idx], 0, ((menu_state == menu_service2) ? LCD_MODE_BLINK_1 : LCD_MODE_ON));
       break;
    case menu_service_watch:
        if (update) LCD_AllSegments(LCD_MODE_ON);
        LCD_PrintHexW(watch(service_watch_n),LCD_MODE_ON);
        LCD_SetHourBarSeg(service_watch_n, LCD_MODE_BLINK_1);
        break;
    case menu_preset_temp0:
    case menu_preset_temp1:
    case menu_preset_temp2:
    case menu_preset_temp3:
        if (update) clr_show1(LCD_SEG_COL1);           // decimal point
        LCD_PrintTemp(menu_set_temp,LCD_MODE_BLINK_1);
        show_selected_temperature_type(menu_state-menu_preset_temp0,LCD_MODE_ON);
        break;                                                             
    default:
	case menu_empty: // show temperature
        if (update) clr_show1(LCD_SEG_COL1);           // decimal point
        LCD_PrintTempInt(temp_average,LCD_MODE_ON);
        break;
    }
}

/*!
 * \note Switch used in this file can generate false warning on some AvrGCC versions
 *       we can ignore it
 *       details:  http://osdir.com/ml/hardware.avr.libc.devel/2006-11/msg00005.html
 */
