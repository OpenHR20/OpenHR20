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
 *				2009 Thomas Vosshagen (mod. for THERMOTronic) (openhr20-at-vosshagen-dot-com)
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
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz> Thomas Vosshagen (mod. for THERMOTronic) <openhr20-at-vosshagen-dot-com>
 * \date       $Date$
 * $Rev$
 */


#define EEPROM __attribute__((section(".eeprom")))

typedef struct { // each variables must be uint8_t or int8_t without exception
    /* 00 */ uint8_t lcd_contrast;
    /* 01 */ uint8_t temperature0;   //!< temperature 0  - frost protection (unit is 0.5stC)
    /* 02 */ uint8_t temperature1;   //!< temperature 1  - energy save (unit is 0.5stC)
    /* 03 */ uint8_t temperature2;   //!< temperature 2  - comfort (unit is 0.5stC)
    /* 04 */ uint8_t temperature3;   //!< temperature 3  - supercomfort (unit is 0.5stC)
    /* 05 */ uint8_t PP_Factor;  //!< Proportional kvadratic tuning constant, multiplied with 256
    /* 06 */ uint8_t P_Factor;  //!< Proportional tuning constant, multiplied with 256
    /* 07 */ uint8_t I_Factor;  //!< Integral tuning constant, multiplied with 256
    /* 08 */ uint8_t temp_tolerance;  //!< tolerance of temperature in 1/100 degree to lazy integrator (improve stability)
    /* 09 */ uint8_t PID_interval; //!< PID_interval*5 = interval in seconds    
    /* 0a */ uint8_t valve_min; // valve position limiter min
    /* 0b */ uint8_t valve_center;  //!< default valve position for "zero - error" - improve stabilization after change temperature
    /* 0c */ uint8_t valve_max; // valve position limiter max
    /* 0d */ uint8_t motor_pwm_min;   //!< min PWM for motor 
    /* 0e */ uint8_t motor_pwm_max;  //!< max PWM for motor
    /* 0f */ uint8_t motor_eye_low;  //!< min signal lenght to accept low level (multiplied by 2)
    /* 10 */ uint8_t motor_eye_high; //!< min signal lenght to accept high level (multiplied by 2)
    /* 11 */ uint8_t motor_end_detect_cal; //!< stop timer threshold in % to previous average 
    /* 12 */ uint8_t motor_end_detect_run; //!< stop timer threshold in % to previous average 
    /* 13 */ uint8_t motor_speed; //!< /8 
    /* 14 */ uint8_t motor_speed_ctl_gain;
    /* 15 */ uint8_t motor_pwm_max_step;
    /* 16 */ uint8_t MOTOR_ManuCalibration_L;
    /* 17 */ uint8_t MOTOR_ManuCalibration_H;
    /* 18 */ uint8_t temp_cal_table0; //!< temperature calibration table
    /* 19 */ uint8_t temp_cal_table1; //!< temperature calibration table
    /* 1a */ uint8_t temp_cal_table2; //!< temperature calibration table
    /* 1b */ uint8_t temp_cal_table3; //!< temperature calibration table
    /* 1c */ uint8_t temp_cal_table4; //!< temperature calibration table
    /* 1d */ uint8_t temp_cal_table5; //!< temperature calibration table
    /* 1e */ uint8_t temp_cal_table6; //!< temperature calibration table
    /* 1f */ uint8_t timer_mode; //!< =0 only one program, =1 programs for weekdays
    /* 20 */ uint8_t bat_warning_thld; //!< treshold for battery warning [unit 0.02V]=[unit 0.01V per cell]
    /* 21 */ uint8_t bat_low_thld; //!< threshold for battery low [unit 0.02V]=[unit 0.01V per cell]
    /* 22 */ uint8_t window_open_close_thld; //!< threshold for window open/close detection unit is 0.1C
    /* 23 */ uint8_t window_open_noise_filter;
    /* 24 */ uint8_t window_close_noise_filter;
    /* 25 */ uint8_t allow_ADC_during_motor;
} config_t;

extern config_t config;
#define config_raw ((uint8_t *) &config)
#define kx_d ((uint8_t *) &config.temp_cal_table0)
#define temperature_table ((uint8_t *) &config.temperature0)
#define CONFIG_RAW_SIZE (sizeof(config_t))

extern uint16_t EEPROM ee_timers[8][RTC_TIMERS_PER_DOW];
extern uint8_t EEPROM ee_layout;

// Boot Timeslots -> move to CONFIG.H
// 10 Minutes after BOOT_hh:00
#define BOOT_ON1       (7*60+0x2000) //!<  7:00
#define BOOT_OFF1      (9*60+0x1000) //!<  9:00
#define BOOT_ON2      (16*60+0x2000) //!<  16:00
#define BOOT_OFF2     (21*60+0x1000) //!<  21:00

#define EE_LAYOUT (0x07) //!< EEPROM layout version 

#ifdef __EEPROM_C__
// this is definition, not just declaration
// used only if header is included from eeprom.c
// this part is in header file, because it is very close to config_t type
// DO NOT change order of items !!
// ALL values in EEPROM must have init values, without exception

uint8_t EEPROM ee_reserved1 = 0x00; // do not use EEPROM address 0
uint8_t EEPROM ee_reserved2 = 0x00;
uint8_t EEPROM ee_reserved3 = 0x00;
uint8_t EEPROM ee_layout    = EE_LAYOUT; //!< EEPROM layout version 

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
    { BOOT_ON1, BOOT_OFF1, BOOT_ON2, BOOT_OFF2, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF},
    { BOOT_ON1, BOOT_OFF1, BOOT_ON2, BOOT_OFF2, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF},
    { BOOT_ON1, BOOT_OFF1, BOOT_ON2, BOOT_OFF2, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF},
    { BOOT_ON1, BOOT_OFF1, BOOT_ON2, BOOT_OFF2, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF},
    { BOOT_ON1, BOOT_OFF1, BOOT_ON2, BOOT_OFF2, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF},
    { BOOT_ON1, BOOT_OFF1, BOOT_ON2, BOOT_OFF2, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF},
    { BOOT_ON1, BOOT_OFF1, BOOT_ON2, BOOT_OFF2, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF},
    { BOOT_ON1, BOOT_OFF1, BOOT_ON2, BOOT_OFF2, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF}
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
}
    
; // reserved for future
#if CONFIG_ENABLE_D
    #error D_Factor have not EEPROM configuration
#endif

uint8_t EEPROM ee_config[][4] ={  // must be alligned to 4 bytes
// order on this table depend to config_t
// /*idx*/ {value,  default,    min,    max},
  /* 00 */  {14,        14,         0,      15},    //!< lcd_contrast  (unit 0.5stC)
  /* 01 */  {10,        10,  TEMP_MIN,TEMP_MAX},    //!< temperature 0  - frost protection (unit is 0.5stC)
  /* 02 */  {33,        33,  TEMP_MIN,TEMP_MAX},    //!< temperature 1  - energy save (unit is 0.5stC)
  /* 03 */  {40,        40,  TEMP_MIN,TEMP_MAX},    //!< temperature 2  - comfort (unit is 0.5stC)
  /* 04 */  {42,        42,  TEMP_MIN,TEMP_MAX},    //!< temperature 3  - supercomfort (unit is 0.5stC)
  /* 05 */  {44,        44,         0,      255},   //!< PP_Factor;
  /* 06 */  {15,        15,         0,      255},   //!< P_Factor;
  /* 07 */  {3,          3,         0,      255},   //!< I_Factor;
  /* 08 */  {50,        50,         0,      255},   //!< temp_tolerance
  /* 09 */  {240/5,     240/5,      20/5,   255},   //!< PID_interval*5 = interval in seconds;  min=20sec, max=21.25 minutes
  /* 0a */  {30,         30,        0,      100},   //!< valve_min
  /* 0b */  {55,         55,        0,      100},   //!< valve_center
  /* 0c */  {80,         80,        0,      100},   //!< valve_max
  /* 0d */  {32,         32,        32,     255},   //!< min motor_pwm PWM setting
  /* 0e */  {250,       250,        180,    255},   //!< max motor_pwm PWM setting
  /* 0f */  {100,       100,        1,      255},   //!< motor_eye_low
  /* 10 */  {25,         25,        1,      255},   //!< motor_eye_high
  /* 11 */  {130,       130,      110,      250},   //!< motor_end_detect_cal; stop timer threshold in % to previous average 
  /* 12 */  {150,       150,      110,      250},   //!< motor_end_detect_run; stop timer threshold in % to previous average 
  /* 13 */  {176,       176,       10,      255},   //!< motor_speed
  /* 14 */  {50,         50,       10,      200},   //!< motor_speed_ctl_gain
  /* 15 */  {10,         10,        1,       64},   //!< motor_pwm_max_step             
  /* 16 */  {255,       255,        0,      255},   //!< manual calibration L
  /* 17 */  {255,       255,        0,      255},   //!< manual calibration H
#ifdef THERMOTRONIC
  /* 18 */  {605-TEMP_CAL_OFFSET,605-TEMP_CAL_OFFSET, 0,        255},   //!< value for 35C => 605 temperature calibration table 
  /* 19 */  {645-605,645-605,      16,      255},   //!< value for 30C => 645 temperature calibration table
  /* 1a */  {685-645,685-645,      16,      255},   //!< value for 25C => 685 temperature calibration table
  /* 1b */  {825-685,825-685,      16,      255},   //!< value for 20C => 825 temperature calibration table
  /* 1c */  {865-825,865-825,      16,      255},   //!< value for 15C => 865 temperature calibration table
  /* 1d */  {905-865,905-865,      16,      255},   //!< value for 10C => 905 temperature calibration table
  /* 1e */  {945-905,945-905,      16,      255},   //!< value for 05C => 945 temperature calibration table
#else
  /* 18 */  {295-TEMP_CAL_OFFSET,295-TEMP_CAL_OFFSET, 0,        255},   //!< value for 35C => 295 temperature calibration table 
  /* 19 */  {340-295,340-295,      16,      255},   //!< value for 30C => 340 temperature calibration table
  /* 1a */  {397-340,397-340,      16,      255},   //!< value for 25C => 397 temperature calibration table
  /* 1b */  {472-397,472-397,      16,      255},   //!< value for 20C => 472 temperature calibration table
  /* 1c */  {549-472,549-472,      16,      255},   //!< value for 15C => 549 temperature calibration table
  /* 1d */  {614-549,614-549,      16,      255},   //!< value for 10C => 614 temperature calibration table
  /* 1e */  {675-614,675-614,      16,      255},   //!< value for 05C => 675 temperature calibration table
#endif
  /* 1f */  {0,           0,        0,        1},   //!< timer_mode; =0 only one program, =1 programs for weekdays 
  /* 20 */  {120,       120,       80,      160},   //!< bat_warning_thld; treshold for battery warning [unit 0.02V]=[unit 0.01V per cell]
  /* 21 */  {100,       100,       80,      160},   //!< bat_low_thld; treshold for battery low [unit 0.02V]=[unit 0.01V per cell]
  /* 22 */  {32,         32,        7,      255},   //!< window_open_close_thld; reshold for window open/close detection unit is 0.01C
  /* 23 */  {5,           5,        2,       60},   //!< window_open_noise_filter
  /* 24 */  {15,         15,        2,      255},   //!< window_close_noise_filter
  /* 25 */  {1,           1,        0,        1},   //!< allow_ADC_during_motor
};



#endif //__EEPROM_C__

uint8_t config_read(uint8_t cfg_address, uint8_t cfg_type);
uint8_t EEPROM_read(uint16_t address);
void eeprom_config_init(bool restore_default);
void eeprom_config_save(uint8_t idx);

uint16_t eeprom_timers_read_raw(uint16_t offset);
#define timers_get_raw_index(dow,slot) (dow*RTC_TIMERS_PER_DOW+slot)
void eeprom_timers_write_raw(uint16_t offset, uint16_t value);
#define eeprom_timers_write(dow,slot,value) (eeprom_timers_write_raw((dow*RTC_TIMERS_PER_DOW+slot),value))

extern uint8_t  timmers_patch_offset;
extern uint16_t timmers_patch_data;


#define CONFIG_VALUE 0
#define CONFIG_DEFAULT 1
#define CONFIG_MIN 2
#define CONFIG_MAX 3

#define config_value(i) (config_read((i), CONFIG_VALUE))
#define config_default(i) (config_read((i), CONFIG_DEFAULT))
#define config_min(i) (config_read((i), CONFIG_MIN))
#define config_max(i) (config_read((i), CONFIG_MAX))

#define MOTOR_ManuCalibration (*((int16_t *)(&config.MOTOR_ManuCalibration_L)))
