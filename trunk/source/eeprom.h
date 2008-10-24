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


#define EEPROM __attribute__((section(".eeprom")))

typedef struct { // each variables must be uint8_t or int8_t without exception
	/* 00 */ uint8_t lcd_contrast;
    /* 01 */ uint8_t temperature0;   //!< temperature 0  - frost protection (unit is 0.5stC)
    /* 02 */ uint8_t temperature1;   //!< temperature 1  - energy save (unit is 0.5stC)
    /* 03 */ uint8_t temperature2;   //!< temperature 2  - comfort (unit is 0.5stC)
    /* 04 */ uint8_t temperature3;   //!< temperature 3  - supercomfort (unit is 0.5stC)
	/* 05 */ uint8_t P_Factor;  //!< Proportional tuning constant, multiplied with scalling_factor
	/* 06 */ uint8_t I_Factor;  //!< Integral tuning constant, multiplied with scalling_factor
	/* 07 */ uint8_t D_Factor;  //!< Derivative tuning constant, multiplied with scalling_factor
	/* 08 */ uint8_t scalling_factor;
    /* 09 */ uint8_t PID_interval; //!< PID_interval*5 = interval in seconds	
    /* 0a */ uint8_t motor_speed_open;   //!< PWM for motor open 
    /* 0b */ uint8_t motor_speed_close;  //!< PWM for motor close
    /* 0c */  int8_t motor_run_timeout; //!< motor_run_timeout (unit 61Hz timer ticks)
        /*! TODO: describe  motor_protection and motor_hysteresis better, picture wanted */
    /* 0d */ uint8_t motor_protection;  //!< defines area for regulation valve as valve range - 2*motor_protection (1* on bottom 1* on top)
    /* 0e */ uint8_t motor_hysteresis; //!< additional impulses for 0 or 100% relative to area for regulation
	/* 0f */ uint8_t motor_reserved1;
	/* 10 */ uint8_t motor_reserved2;
	/* 11 */ uint8_t temp_cal_table0; //!< temperature calibration table
	/* 12 */ uint8_t temp_cal_table1; //!< temperature calibration table
	/* 13 */ uint8_t temp_cal_table2; //!< temperature calibration table
	/* 14 */ uint8_t temp_cal_table3; //!< temperature calibration table
	/* 15 */ uint8_t temp_cal_table4; //!< temperature calibration table
	/* 16 */ uint8_t temp_cal_table5; //!< temperature calibration table
	/* 17 */ uint8_t temp_cal_table6; //!< temperature calibration table
	/* 18 */ uint8_t timer_mode; //!< =0 only one program, =1 programs for weekdays
	/* 19 */ uint8_t bat_warning_thld; //!< treshold for battery warning [unit 0.02V]=[unit 0.01V per cell]
	/* 1a */ uint8_t bat_low_thld; //!< treshold for battery low [unit 0.02V]=[unit 0.01V per cell]
} config_t;

extern config_t config;
#define config_raw ((uint8_t *) &config)
#define kx_d ((uint8_t *) &config.temp_cal_table0)
#define temperature_table(i) (((uint8_t *) &config.temperature0)[i])
#define CONFIG_RAW_SIZE (sizeof(config_t))

extern uint16_t EEPROM ee_timers[8][RTC_TIMERS_PER_DOW];

// Boot Timeslots -> move to CONFIG.H
// 10 Minutes after BOOT_hh:00
#define BOOT_ON1       (7*60+0x2000) //!<  7:00
#define BOOT_OFF1      (9*60+0x1000) //!<  9:00
#define BOOT_ON2      (16*60+0x2000) //!<  16:00
#define BOOT_OFF2     (21*60+0x1000) //!<  21:00


#ifdef __EEPROM_C__
// this is definition, not just declaration
// used only if header is included from eeprom.c
// this part is in header file, because it is very close to config_t type
// DO NOT change order of items !!
// ALL values in EEPROM must have init values, without exception

uint32_t EEPROM ee_reserved1 = 0x00000000;   //4bytes reserved / do not use EEPROM address 0

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

uint8_t EEPROM ee_reserved2 [60] = {
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

uint8_t EEPROM ee_config[][4] ={  // must be alligned to 4 bytes
// order on this table depend to config_t
// /*idx*/ {value,	default, 	min,	max},
  /* 00 */  {14,	    14,         0,		15},	//!< lcd_contrast  (unit 0.5stC)
  /* 01 */  {10,	    10,  TEMP_MIN,TEMP_MAX},	//!< temperature 0  - frost protection (unit is 0.5stC)
  /* 02 */  {34,	    34,  TEMP_MIN,TEMP_MAX},    //!< temperature 1  - energy save (unit is 0.5stC)
  /* 03 */  {42,	    42,  TEMP_MIN,TEMP_MAX},    //!< temperature 2  - comfort (unit is 0.5stC)
  /* 04 */  {48,	    48,  TEMP_MIN,TEMP_MAX},    //!< temperature 3  - supercomfort (unit is 0.5stC)
  /* 05 */  {64,	    64,			0,		255},	//!< P_Factor;
  /* 06 */  {12,	    12,			0,		255},	//!< I_Factor;
  /* 07 */  {12,	    12,			0,		255},	//!< D_Factor;
  /* 08 */  {128,	    128,		1,		255},	//!< scalling_factor != 0
  /* 09 */  {240/5,     240/5,      20/5,   255},   //!< PID_interval*5 = interval in seconds;  min=20sec, max=21.25 minutes
  /* 0a */  {180,	    180,        178,	234},	//!< open motor_speed PWM setting
  /* 0b */  {200,	    200,        178,	234},	//!< close motor_speed PWM setting
  /* 0c */  {20,         20,        6,      120},   //!< motor_run_timeout (unit 61Hz timer ticks); !!signed value => max is 127!!    
  /* 0d */  {15,         15,        0,      120},   //!< motor_protection
  /* 0e */  {10,         10,        0,      120},   //!< additional impulses for 0 or 100%
  /* 0f */  {1,           1,        0,        1},   //!< motor_reserved1
  /* 10 */  {1,           1,        0,        1},   //!< motor_reserved2
  /* 11 */  {295-TEMP_CAL_OFFSET,295-TEMP_CAL_OFFSET, 0,        255},   //!< value for 35C => 295 temperature calibration table 
  /* 12 */  {340-295,340-295,      16,      255},   //!< value for 30C => 340 temperature calibration table
  /* 13 */  {397-340,397-340,      16,      255},   //!< value for 25C => 397 temperature calibration table
  /* 14 */  {472-397,472-397,      16,      255},   //!< value for 20C => 472 temperature calibration table
  /* 15 */  {549-472,549-472,      16,      255},   //!< value for 15C => 549 temperature calibration table
  /* 16 */  {614-549,614-549,      16,      255},   //!< value for 10C => 614 temperature calibration table
  /* 17 */  {675-614,675-614,      16,      255},   //!< value for 05C => 675 temperature calibration table
  /* 18 */  {0,           0,        0,        1},   //!< timer_mode; =0 only one program, =1 programs for weekdays 
  /* 19 */  {120,       120,       80,      160},   //!< bat_warning_thld; treshold for battery warning [unit 0.02V]=[unit 0.01V per cell]
  /* 1a */  {100,       100,       80,      160},   //!< bat_low_thld; treshold for battery low [unit 0.02V]=[unit 0.01V per cell]

};



#endif //__EEPROM_C__

uint8_t config_read(uint8_t cfg_address, uint8_t cfg_type);
uint8_t EEPROM_read(uint16_t address);
void eeprom_config_init(void);
void eeprom_config_save(uint8_t idx);

uint16_t eeprom_timers_read_raw(uint16_t offset);
#define eeprom_timers_read(dow,slot) (eeprom_timers_read_raw((dow*RTC_TIMERS_PER_DOW+slot)*sizeof(ee_timers[0][0])))
void eeprom_timers_write_raw(uint16_t offset, uint16_t value);
#define eeprom_timers_write(dow,slot,value) (eeprom_timers_write_raw((dow*RTC_TIMERS_PER_DOW+slot)*sizeof(ee_timers[0][0]),value))



#define CONFIG_VALUE 0
#define CONFIG_DEFAULT 1
#define CONFIG_MIN 2
#define CONFIG_MAX 3

#ifdef __EEPROM_C__
	//only for internal usage
#define config_value(i) (config_read((i), CONFIG_VALUE))
#endif
#define config_default(i) (config_read((i), CONFIG_DEFAULT))
#define config_min(i) (config_read((i), CONFIG_MIN))
#define config_max(i) (config_read((i), CONFIG_MAX))
