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
 * \file       keyboard.h
 * \brief      Keyboard driver header
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */


#define EEPROM __attribute__((section(".eeprom")))

typedef struct { // each variables must be uint8_t or int8_t without exception
	/*  0 */ uint8_t lcd_contrast;
    /*  1 */ uint8_t temperature0;   //!< temperature 0  - frost protection (unit is 0.5stC)
    /*  2 */ uint8_t temperature1;   //!< temperature 1  - energy save (unit is 0.5stC)
    /*  3 */ uint8_t temperature2;   //!< temperature 2  - comfort (unit is 0.5stC)
    /*  4 */ uint8_t temperature3;   //!< temperature 3  - supercomfort (unit is 0.5stC)
	/*  5 */ uint8_t P_Factor;  //!< Proportional tuning constant, multiplied with scalling_factor
	/*  6 */ uint8_t I_Factor;  //!< Integral tuning constant, multiplied with scalling_factor
	/*  7 */ uint8_t D_Factor;  //!< Derivative tuning constant, multiplied with scalling_factor
	/*  8 */ uint8_t scalling_factor;
    /*  9 */ uint8_t PID_interval; //!< PID_interval*5 = interval in seconds	
    /* 10 */ uint8_t motor_speed_open;   //!< PWM for motor open 
    /* 11 */ uint8_t motor_speed_close;  //!< PWM for motor close
    /* 12 */  int8_t motor_run_timeout; //!< motor_run_timeout (unit 61Hz timer ticks)
        /*! TODO: describe  motor_protection and motor_hysteresis better, picture wanted */
    /* 13 */ uint8_t motor_protection;  //!< defines area for regulation valve as valve range - 2*motor_protection (1* on bottom 1* on top)
    /* 14 */ uint8_t motor_hysteresis; //!< additional impulses for 0 or 100% relative to area for regulation
	/* 15 */ uint8_t motor_reserved1;
	/* 16 */ uint8_t motor_reserved2;
	/* 15 */ uint8_t keep_alive_for_communication;
} config_t;

extern config_t config;
#define config_raw ((uint8_t *) &config)
#define CONFIG_RAW_SIZE (sizeof(config_t))

// Boot Timeslots -> move to CONFIG.H
// 10 Minutes after BOOT_hh:00
#define BOOT_ON1      BOOT_hh*10 + 1 //!< Boot Timeslot 0 \todo move to CONFIG.H
#define BOOT_OFF1     BOOT_ON1 + 1   //!< Boot Timeslot 1 \todo move to CONFIG.H
#define BOOT_ON2      BOOT_ON1 + 2   //!< Boot Timeslot 2 \todo move to CONFIG.H
#define BOOT_OFF2     BOOT_ON1 + 3   //!< Boot Timeslot 3 \todo move to CONFIG.H




#ifdef __EEPROM_C__
// this is definition, not just declaration
// used only if header is included from eeprom.c
// this part is in header file, because it is very close to config_t type
// DO NOT change order of items !!
// ALL values in EEPROM must have init values, without exception

uint32_t EEPROM ee_reserved1 = 0x00000000;   //4bytes reserved / do not use EEPROM address 0

uint16_t EEPROM ee_timers[8][8] = { //128bytes 
    // TODO add default timers, now it is empty
    /*! ee_timers value means:
     *          value & 0x0fff  = time in minutes from midnight
     *          value & 0x3000  = 0x0000 - temperature 0  - frost protection
     *                            0x1000 - temperature 1  - energy save
     *                            0x2000 - temperature 2  - comfort
     *                            0x3000 - temperature 3  - supercomfort
     *          value & 0xc000  - reserved for future
     */                                 
	{ 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF},
	{ 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF},
	{ 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF},
	{ 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF},
	{ 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF},
	{ 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF},
	{ 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF},
	{ 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF, 0x1FFF, 0x2FFF}
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
  /*  0 */  {14,	    14,         0,		15},	//!< lcd_contrast  (unit 0.5stC)
  /*  1 */  {10,	    10,  TEMP_MIN,TEMP_MAX},	//!< temperature 0  - frost protection (unit is 0.5stC)
  /*  2 */  {34,	    34,  TEMP_MIN,TEMP_MAX},    //!< temperature 1  - energy save (unit is 0.5stC)
  /*  3 */  {42,	    42,  TEMP_MIN,TEMP_MAX},    //!< temperature 2  - comfort (unit is 0.5stC)
  /*  4 */  {48,	    48,  TEMP_MIN,TEMP_MAX},    //!< temperature 3  - supercomfort (unit is 0.5stC)
  /*  5 */  {64,	    64,			0,		255},	//!< P_Factor;
  /*  6 */  {12,	    12,			0,		255},	//!< I_Factor;
  /*  7 */  {12,	    12,			0,		255},	//!< D_Factor;
  /*  8 */  {128,	    128,		1,		255},	//!< scalling_factor != 0
  /*  9 */  {240/5,     240/5,      20/5,   255},   //!< PID_interval*5 = interval in seconds;  min=20sec, max=21.25 minutes
  /* 10 */  {180,	    180,        178,	234},	//!< open motor_speed PWM setting
  /* 11 */  {200,	    200,        178,	234},	//!< close motor_speed PWM setting
  /* 12 */  {20,         20,        6,      120},   //!< motor_run_timeout (unit 61Hz timer ticks); !!signed value => max is 127!!    
  /* 13 */  {15,         15,        0,      120},   //!< motor_protection
  /* 14 */  {10,         10,        0,      120},   //!< additional impulses for 0 or 100%
  /* 15 */  {1,           1,        0,        1},   //!< motor_reserved1
  /* 16 */  {1,           1,        0,        1},   //!< motor_reserved2
  /* 17 */  {KEEP_ALIVE_FOR_COMMUNICATION,0,0,1},   //!< keep_alive_for_communication
};



#endif //__EEPROM_C__

uint8_t config_read(uint8_t cfg_address, uint8_t cfg_type);
uint8_t EEPROM_read(uint16_t address);
void eeprom_config_init(void);
void eeprom_config_save(void);
void eeprom_timers_init(void);
void eeprom_timers_save(void);

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
