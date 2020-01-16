/*
 *  Open HR20
 *
 *  target:     ATmega32 @ 10 MHz in Honnywell Rondostat HR20E
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
 * \date       $Date: 2009-02-13 20:51:00 +0100 (pá, 13 II 2009) $
 * $Rev: 188 $
 */

#pragma once

#include "config.h"
#include <avr/eeprom.h>
#include "common/rtc.h"
#if (RFM == 1)
#include "rfm_config.h"
#include "common/rfm.h"
#endif


#define EEPROM __attribute__((section(".eeprom")))

typedef struct                                          // each variables must be uint8_t or int8_t without exception
{
 #if (RFM == 1)
	/* 00...07 */ uint8_t security_key[8];          //!< key for encrypted radio messasges
#if (RFM_TUNING > 0)
	/*      08 */ int8_t RFM_freqAdjust;            //!< RFM12 Frequency adjustment
	/*      09 */ uint8_t RFM_tuning;               //!< RFM12 tuning mode
#endif
#endif
} config_t;

extern config_t config;
#define config_raw ((uint8_t *)&config)
#define kx_d ((uint8_t *)&config.temp_cal_table0)
#define temperature_table ((uint8_t *)&config.temperature0)
#define CONFIG_RAW_SIZE (sizeof(config_t))

extern uint8_t EEPROM ee_layout;

#define EE_LAYOUT (0xE1) //!< EEPROM layout version (Experimental 1)

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

/* eeprom address 0x040 */

uint8_t EEPROM ee_config[][4] = {  // must be alligned to 4 bytes
// order on this table depend to config_t
//      /*idx*/  {          value,         default,   min,  max },
#if (RFM == 1)
	/* 00 */ { SECURITY_KEY_0,  SECURITY_KEY_0,  0x00, 0xff },              //!< security_key[0] for encrypted radio messasges
	/* 01 */ { SECURITY_KEY_1,  SECURITY_KEY_1,  0x00, 0xff },              //!< security_key[1] for encrypted radio messasges
	/* 02 */ { SECURITY_KEY_2,  SECURITY_KEY_2,  0x00, 0xff },              //!< security_key[2] for encrypted radio messasges
	/* 03 */ { SECURITY_KEY_3,  SECURITY_KEY_3,  0x00, 0xff },              //!< security_key[3] for encrypted radio messasges
	/* 04 */ { SECURITY_KEY_4,  SECURITY_KEY_4,  0x00, 0xff },              //!< security_key[4] for encrypted radio messasges
	/* 05 */ { SECURITY_KEY_5,  SECURITY_KEY_5,  0x00, 0xff },              //!< security_key[5] for encrypted radio messasges
	/* 06 */ { SECURITY_KEY_6,  SECURITY_KEY_6,  0x00, 0xff },              //!< security_key[6] for encrypted radio messasges
	/* 07 */ { SECURITY_KEY_7,  SECURITY_KEY_7,  0x00, 0xff },              //!< security_key[7] for encrypted radio messasges
#if (RFM_TUNING > 0)
	/*    */ {              0,               0,  0x00, 0xff },              //!< RFM12 Frequency adjustment, 2's complement
	/*    */ { RFM_TUNING_MODE, RFM_TUNING_MODE, 0x00, 0xff },              //!< RFM12 tuning mode, 0 = tuning mode off (narrow, high data rate), 1 = tuning mode on (wide, low data rate)
#endif

#endif
};
#endif //__EEPROM_C__


uint8_t config_read(uint8_t cfg_address, uint8_t cfg_type);
uint8_t EEPROM_read(uint16_t address);
void EEPROM_write(uint16_t address, uint8_t data);
void eeprom_config_init(bool restore_default);
void eeprom_config_save(uint8_t idx);

uint16_t eeprom_timers_read_raw(uint16_t offset);
#define timers_get_raw_index(dow, slot) (dow * RTC_TIMERS_PER_DOW + slot)
void eeprom_timers_write_raw(uint16_t offset, uint16_t value);
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
