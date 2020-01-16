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
 * \file       eeprom.h
 * \brief      Keyboard driver header
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

#pragma once

#include "config.h"
#include <avr/eeprom.h>
#include "debug.h"
#include "main.h"
#include "common/rtc.h"
#include "adc.h"
#if (RFM == 1)
#include "rfm_config.h"
#include "common/rfm.h"
#endif


#define EEPROM __attribute__((section(".eeprom")))

typedef struct                                                  // each variables must be uint8_t or int8_t without exception
{
	/* 00 */ uint8_t lcd_contrast;
	/* 01 */ uint8_t temperature0;                          //!< temperature 0  - frost protection (unit is 0.5stC)
	/* 02 */ uint8_t temperature1;                          //!< temperature 1  - energy save (unit is 0.5stC)
	/* 03 */ uint8_t temperature2;                          //!< temperature 2  - comfort (unit is 0.5stC)
	/* 04 */ uint8_t temperature3;                          //!< temperature 3  - supercomfort (unit is 0.5stC)
	/* 05 */ uint8_t P3_Factor;                             //!< Proportional cubic tuning constant
	/* 06 */ uint8_t P_Factor;                              //!< Proportional tuning constant
	/* 07 */ uint8_t I_Factor;                              //!< Integral tuning constant
	/* 08 */ uint8_t I_max_credit;                          //!< credit for interator limitation
	/* 09 */ uint8_t I_credit_expiration;                   //!< unit is PID_interval
	/* 0a */ uint8_t PID_interval;                          //!< PID_interval*5 = interval in seconds
	/* 0b */ uint8_t valve_min;                             //!< valve position limiter min
	/* 0c */ uint8_t valve_center;                          //!< default valve position for "zero - error" - improve stabilization after change temperature
	/* 0d */ uint8_t valve_max;                             //!< valve position limiter max
	/* 0e */ uint8_t valve_hysteresis;                      //!< valve movement hysteresis (unit is 1/128%)
	/* 0f */ uint8_t motor_pwm_min;                         //!< min PWM for motor
	/* 10 */ uint8_t motor_pwm_max;                         //!< max PWM for motor
	/* 11 */ uint8_t motor_eye_low;                         //!< min signal lenght to accept low level (multiplied by 2)
	/* 12 */ uint8_t motor_eye_high;                        //!< min signal lenght to accept high level (multiplied by 2)
	/* 13 */ uint8_t motor_close_eye_timeout;               //!<time from last pulse to disable eye [1/61sec]
	/* 14 */ uint8_t motor_end_detect_cal;                  //!< stop timer threshold in % to previous average
	/* 15 */ uint8_t motor_end_detect_run;                  //!< stop timer threshold in % to previous average
	/* 16 */ uint8_t motor_speed;                           //!< /8
	/* 17 */ uint8_t motor_speed_ctl_gain;
	/* 18 */ uint8_t motor_pwm_max_step;
	/* 19 */ uint8_t MOTOR_ManuCalibration_L;
	/* 1a */ uint8_t MOTOR_ManuCalibration_H;
	/* 1b */ uint8_t temp_cal_table0;                       //!< temperature calibration table
	/* 1c */ uint8_t temp_cal_table1;                       //!< temperature calibration table
	/* 1d */ uint8_t temp_cal_table2;                       //!< temperature calibration table
	/* 1e */ uint8_t temp_cal_table3;                       //!< temperature calibration table
	/* 1f */ uint8_t temp_cal_table4;                       //!< temperature calibration table
	/* 20 */ uint8_t temp_cal_table5;                       //!< temperature calibration table
	/* 21 */ uint8_t temp_cal_table6;                       //!< temperature calibration table
	/* 22 */ uint8_t timer_mode;                            //!< bit0: timermode; =0 only one program, =1 programs for weekdays
	//                                                                            >1 manual mode, the higher bits contain the saved temperature << 1
#if HR25
	/*    */ uint8_t bat_half_thld;                         //!< treshold for half battery indicator [unit 0.02V]=[unit 0.01V per cell]
#endif
	/*    */ uint8_t bat_warning_thld;                      //!< treshold for battery warning [unit 0.02V]=[unit 0.01V per cell]
	/*    */ uint8_t bat_low_thld;                          //!< threshold for battery low [unit 0.02V]=[unit 0.01V per cell]
	/*    */ uint8_t allow_ADC_during_motor;
#if HW_WINDOW_DETECTION
	/*    */ uint8_t window_open_detection_enable;
	/*    */ uint8_t window_open_detection_delay;           //!< window open detection delay [sec]
	/*    */ uint8_t window_close_detection_delay;          //!< window close detection delay [sec]
#else
	/*    */ uint8_t window_open_detection_diff;            //!< threshold for window open detection unit is 0.1C
	/*    */ uint8_t window_close_detection_diff;           //!< threshold for window close detection unit is 0.1C
	/*    */ uint8_t window_open_detection_time;
	/*    */ uint8_t window_close_detection_time;
	/*    */ uint8_t window_open_timeout;                   //!< maximum time for window open state [minutes]
#endif
#if BOOST_CONTROLER_AFTER_CHANGE
	/*    */ uint8_t temp_boost_setpoint_diff;
	/*    */ uint8_t temp_boost_hystereses;
	/*    */ uint8_t temp_boost_error;
	/*    */ uint8_t temp_boost_tempchange;
	/*    */ uint8_t temp_boost_time_cool;
	/*    */ uint8_t temp_boost_time_heat;
#endif
#if TEMP_COMPENSATE_OPTION
	/*    */ int8_t room_temp_offset;
#endif
#if (RFM == 1)
	/*    */ uint8_t RFM_devaddr;                           //!< HR20's own device address in RFM radio networking. =0 mean disable radio
	/*    */ uint8_t security_key[8];                       //!< key for encrypted radio messasges
#if (RFM_TUNING > 0)
	/*    */ int8_t RFM_freqAdjust;                         //!< RFM12 Frequency adjustment
	/*    */ uint8_t RFM_tuning;                            //!< RFM12 tuning mode
#endif
	/* unused */
#endif
} config_t;

extern config_t config;
#define config_raw ((uint8_t *)&config)
#define kx_d ((uint8_t *)&config.temp_cal_table0)
#define temperature_table ((uint8_t *)&config.temperature0)
#define CONFIG_RAW_SIZE (sizeof(config_t))

extern uint16_t EEPROM ee_timers[8][RTC_TIMERS_PER_DOW];
extern uint8_t EEPROM ee_layout;

// Boot Timeslots -> move to CONFIG.H
// 10 Minutes after BOOT_hh:00
#define BOOT_ON1       (7 * 60 + 0x2000)        //!<  7:00
#define BOOT_OFF1      (9 * 60 + 0x1000)        //!<  9:00
#define BOOT_ON2      (16 * 60 + 0x2000)        //!<  16:00
#define BOOT_OFF2     (21 * 60 + 0x1000)        //!<  21:00

#if (HW_WINDOW_DETECTION)
#define EE_LAYOUT (0x15)
#else
#define EE_LAYOUT (0x14)
#endif
#if (BOOST_CONTROLER_AFTER_CHANGE) || (TEMP_COMPENSATE_OPTION)
#define EE_LAYOUT (0xff)
// for this options we haven't reserved EE_LAYOUT number yet
#endif

#ifdef __EEPROM_C__
// this is definition, not just declaration
// used only if header is included from eeprom.c
// this part is in header file, because it is very close to config_t type
// DO NOT change order of items !!
// ALL values in EEPROM must have init values, without exception

uint8_t EEPROM ee_reserved1 = 0x00; // do not use EEPROM address 0
uint8_t EEPROM ee_reserved2 = 0x00;
uint8_t EEPROM ee_reserved3 = 0x00;
uint8_t EEPROM ee_layout = EE_LAYOUT;    //!< EEPROM layout version

/* eeprom address 0x004 */
uint16_t EEPROM ee_timers[8][RTC_TIMERS_PER_DOW] = { //128bytes
	// TODO add default timers, now it is empty
	/*! ee_timers value means:
	 *          value & 0x0fff  = time in minutes from midnight
	 *          value & 0x3000  = 0x0000 - temperature 0  - frost protection
	 *                            0x1000 - temperature 1  - energy save
	 *                            0x2000 - temperature 2  - comfort
	 *                            0x3000 - temperature 3  - supercomfort
	 *          value & 0xc000  - reserved for future
	 */
	{ BOOT_ON1, BOOT_OFF1, BOOT_ON2, BOOT_OFF2, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF },
	{ BOOT_ON1, BOOT_OFF1, BOOT_ON2, BOOT_OFF2, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF },
	{ BOOT_ON1, BOOT_OFF1, BOOT_ON2, BOOT_OFF2, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF },
	{ BOOT_ON1, BOOT_OFF1, BOOT_ON2, BOOT_OFF2, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF },
	{ BOOT_ON1, BOOT_OFF1, BOOT_ON2, BOOT_OFF2, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF },
	{ BOOT_ON1, BOOT_OFF1, BOOT_ON2, BOOT_OFF2, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF },
	{ BOOT_ON1, BOOT_OFF1, BOOT_ON2, BOOT_OFF2, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF },
	{ BOOT_ON1, BOOT_OFF1, BOOT_ON2, BOOT_OFF2, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF }
};

uint8_t EEPROM ee_reserved2_60 [60] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff
};

;                                       // reserved for future

uint8_t EEPROM ee_config[][4] = {       // must be alligned to 4 bytes
// order on this table depend to config_t
//      /*idx */ {                 value,               default,      min,                        max},
	/* 00 */ {                    14,                    14,        0,                        15 }, //!< lcd_contrast  (unit 0.5stC)
	/* 01 */ {                    10,                    10, TEMP_MIN,                  TEMP_MAX }, //!< temperature 0  - frost protection (unit is 0.5stC)
	/* 02 */ {                    34,                    34, TEMP_MIN,                  TEMP_MAX }, //!< temperature 1  - energy save (unit is 0.5stC)
	/* 03 */ {                    42,                    42, TEMP_MIN,                  TEMP_MAX }, //!< temperature 2  - comfort (unit is 0.5stC)
	/* 04 */ {                    48,                    48, TEMP_MIN,                  TEMP_MAX }, //!< temperature 3  - supercomfort (unit is 0.5stC)
	/* 05 */ {                    33,                    33,        0,                       255 }, //!< P3_Factor;
	/* 06 */ {                     8,                     8,        0,                       255 }, //!< P_Factor;
	/* 07 */ {                    32,                    32,        0,                       255 }, //!< I_Factor;
	/* 08 */ {                    40,                    40,        0,                       127 }, //!< I_max_credit
	/* 09 */ {                    30,                    30,        0,                       255 }, //!< I_credit_expiration unit is PID_interval, default 2 hour
	/* 0a */ {               240 / 5,               240 / 5,   20 / 5,                       255 }, //!< PID_interval*5 = interval in seconds;  min=20sec, max=21.25 minutes
	/* 0b */ {                    30,                    30,        0,                       100 }, //!< valve_min
	/* 0c */ {                    45,                    45,        0,                       100 }, //!< valve_center
	/* 0d */ {                    80,                    80,        0,                       100 }, //!< valve_max
	/* 0e */ {                    64,                    64,        0,                       127 }, //!< valve_hysteresis; valve movement hysteresis (unit is 1/128%), must be <128
	/* 0f */ {                    32,                    32,       32,                       255 }, //!< min motor_pwm PWM setting
	/* 10 */ {                   250,                   250,       50,                       255 }, //!< max motor_pwm PWM setting
	/* 11 */ {                   100,                   100,        1,                       255 }, //!< motor_eye_low
	/* 12 */ {                    25,                    25,        1,                       255 }, //!< motor_eye_high
	/* 13 */ {                    78,                    78,        5,                       255 }, //!< motor_close_eye_timeout; time from last pulse to disable eye [1/61sec]
	/* 14 */ {                   130,                   130,      110,                       250 }, //!< motor_end_detect_cal; stop timer threshold in % to previous average
	/* 15 */ {                   150,                   150,      110,                       250 }, //!< motor_end_detect_run; stop timer threshold in % to previous average
	/* 16 */ {                   184,                   184,       10,                       255 }, //!< motor_speed
	/* 17 */ {                    50,                    50,       10,                       200 }, //!< motor_speed_ctl_gain
	/* 18 */ {                    10,                    10,        1,                        64 }, //!< motor_pwm_max_step
	/* 19 */ {                   255,                   255,        0,                       255 }, //!< manual calibration L
	/* 1a */ {                   255,                   255,        0,                       255 }, //!< manual calibration H
#if THERMOTRONIC == 1
	/* 1b */ { 605 - TEMP_CAL_OFFSET, 605 - TEMP_CAL_OFFSET,        0,                       255 }, //!< value for 35C => 605 temperature calibration table
	/* 1c */ {             645 - 605,             645 - 605,       16,                       255 }, //!< value for 30C => 645 temperature calibration table
	/* 1d */ {             685 - 645,             685 - 645,       16,                       255 }, //!< value for 25C => 685 temperature calibration table
	/* 1e */ {             825 - 685,             825 - 685,       16,                       255 }, //!< value for 20C => 825 temperature calibration table
	/* 1f */ {             865 - 825,             865 - 825,       16,                       255 }, //!< value for 15C => 865 temperature calibration table
	/* 20 */ {             905 - 865,             905 - 865,       16,                       255 }, //!< value for 10C => 905 temperature calibration table
	/* 21 */ {             945 - 905,             945 - 905,       16,                       255 }, //!< value for 05C => 945 temperature calibration table
#else
	/* 1b */ { 295 - TEMP_CAL_OFFSET, 295 - TEMP_CAL_OFFSET,        0,                       255 }, //!< value for 35C => 295 temperature calibration table
	/* 1c */ {             340 - 295,             340 - 295,       16,                       255 }, //!< value for 30C => 340 temperature calibration table
	/* 1d */ {             397 - 340,             397 - 340,       16,                       255 }, //!< value for 25C => 397 temperature calibration table
	/* 1e */ {             472 - 397,             472 - 397,       16,                       255 }, //!< value for 20C => 472 temperature calibration table
	/* 1f */ {             549 - 472,             549 - 472,       16,                       255 }, //!< value for 15C => 549 temperature calibration table
	/* 20 */ {             614 - 549,             614 - 549,       16,                       255 }, //!< value for 10C => 614 temperature calibration table
	/* 21 */ {             675 - 614,             675 - 614,       16,                       255 }, //!< value for 05C => 675 temperature calibration table
#endif
	/* 22 */ {                     0,                     0,        0, ((TEMP_MAX + 1) << 1) + 1 }, //!< bit0: timer_mode; =0 only one program, =1 programs for weekdays
	//                                                                                                                     >1 manual mode, the higher bits contain the saved temperature << 1
#if HR25
	/*    */ {                   125,                   125,       80,                       160 }, //!< bat_half_thld; treshold for half battery indicator [unit 0.02V]=[unit 0.01V per cell]
#endif
	/*    */ {                   120,                   120,       80,                       160 }, //!< bat_warning_thld; treshold for battery warning [unit 0.02V]=[unit 0.01V per cell]
	/*    */ {                   100,                   100,       80,                       160 }, //!< bat_low_thld; treshold for battery low [unit 0.02V]=[unit 0.01V per cell]
	/*    */ {                     1,                     1,        0,                         1 }, //!< allow_ADC_during_motor
#if HW_WINDOW_DETECTION
	/*    */ {                     1,                     1,        0,                         1 }, //!< window_open_detection_enable
	/*    */ {                     5,                     5,        0,                       240 }, //!< window_open_detection_delay [sec] max 4 minutes
	/*    */ {                     5,                     5,        0,                       240 }, //!< window_close_detection_delay [sec] max 4 minutes
#else
	/*    */ {                    50,                    50,        7,                       255 }, //!< window_open_detection_diff; reshold for window open/close detection unit is 0.01C
	/*    */ {                    50,                    50,        7,                       255 }, //!< window_close_detection_diff; reshold for window open/close detection unit is 0.01C
	/*    */ {                     8,                     8,        1,           AVGS_BUFFER_LEN }, //!< window_open_detection_time unit 15sec = 1/4min
	/*    */ {                     8,                     8,        1,           AVGS_BUFFER_LEN }, //!< window_close_detection_time unit 15sec = 1/4min
	/*    */ {                    90,                    90,        2,                       255 }, //!< window_open_timeout
#endif
#if BOOST_CONTROLER_AFTER_CHANGE
	/*    */ {                    50,                     0,        0,                       255 }, //!< temp_boost_setpoint_diff, unit 0,01°C
	/*    */ {                    10,                     0,        0,                       255 }, //!< temp_boost_hystereses, unit 0,01°C
	/*    */ {                    30,                     0,        0,                       255 }, //!< temp_boost_error, unit 0,01°C
	/*    */ {                     5,                     0,        0,                       255 }, //!< temp_boost_tempchange_heat,0,1°C, boosttime=error/10(0,1°C)*time/tempchange
	/*    */ {                    64,                     0,        0,                       255 }, //!< temp_boost_time_cool, minutes
	/*    */ {                    15,                     0,        0,                       255 }, //!< temp_boost_time_heat, minutes
#endif
#if TEMP_COMPENSATE_OPTION
	/*    */ {                     0,                     0,        0,                       255 }, //!< offset to roomtemp 1=0,1°C, binary complement for <0
#endif
#if (RFM == 1)
	/*    */ {    RFM_DEVICE_ADDRESS,    RFM_DEVICE_ADDRESS,        0,                        29 }, //!< RFM_devaddr: HR20's own device address in RFM radio networking.
	/*    */ {        SECURITY_KEY_0,        SECURITY_KEY_0,     0x00,                      0xff }, //!< security_key[0] for encrypted radio messasges
	/*    */ {        SECURITY_KEY_1,        SECURITY_KEY_1,     0x00,                      0xff }, //!< security_key[1] for encrypted radio messasges
	/*    */ {        SECURITY_KEY_2,        SECURITY_KEY_2,     0x00,                      0xff }, //!< security_key[2] for encrypted radio messasges
	/*    */ {        SECURITY_KEY_3,        SECURITY_KEY_3,     0x00,                      0xff }, //!< security_key[3] for encrypted radio messasges
	/*    */ {        SECURITY_KEY_4,        SECURITY_KEY_4,     0x00,                      0xff }, //!< security_key[4] for encrypted radio messasges
	/*    */ {        SECURITY_KEY_5,        SECURITY_KEY_5,     0x00,                      0xff }, //!< security_key[5] for encrypted radio messasges
	/*    */ {        SECURITY_KEY_6,        SECURITY_KEY_6,     0x00,                      0xff }, //!< security_key[6] for encrypted radio messasges
	/*    */ {        SECURITY_KEY_7,        SECURITY_KEY_7,     0x00,                      0xff }, //!< security_key[7] for encrypted radio messasges
 #if (RFM_TUNING > 0)
	/*    */ {                     0,                     0,     0x00,                      0xff }, //!< RFM12 Frequency adjustment, 2's complement
	/*    */ {       RFM_TUNING_MODE,                     0,     0x00,                      0x01 }, //!< RFM12 tuning mode, 0 = tuning mode off (narrow, high data rate),
	//                                                                                                                      1 = tuning mode on (wide, low data rate)
 #endif
#endif
};

#endif //__EEPROM_C__


uint8_t config_read(uint8_t cfg_address, uint8_t cfg_type);
uint8_t EEPROM_read(uint16_t address);
void EEPROM_write(uint16_t address, uint8_t data);
void eeprom_config_init(bool restore_default);
void eeprom_config_save(uint8_t idx);

// valid temperature types are 0-3, use next value to indicate invalid type
#define TEMP_TYPE_INVALID 4

uint16_t eeprom_timers_read_raw(uint8_t offset);
#define timers_get_raw_index(dow, slot) (dow * RTC_TIMERS_PER_DOW + slot)
void eeprom_timers_write_raw(uint8_t offset, uint16_t value);
#define eeprom_timers_write(dow, slot, value) (eeprom_timers_write_raw((dow * RTC_TIMERS_PER_DOW + slot), value))

extern uint8_t timers_patch_offset;
extern uint16_t timers_patch_data;


#define CONFIG_VALUE 0
#define CONFIG_DEFAULT 1
#define CONFIG_MIN 2
#define CONFIG_MAX 3

#define config_value(i) (config_read((i), CONFIG_VALUE))
#define config_default(i) (config_read((i), CONFIG_DEFAULT))
#define config_min(i) (config_read((i), CONFIG_MIN))
#define config_max(i) (config_read((i), CONFIG_MAX))

#define MOTOR_ManuCalibration (*((int16_t *)(&config.MOTOR_ManuCalibration_L)))
