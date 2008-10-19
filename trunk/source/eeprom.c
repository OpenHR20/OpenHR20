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
 * \file       eeprom.c
 * \brief      EEPROM storage
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

#include <avr/eeprom.h>
#include "debug.h"
#include "main.h"
#include "rtc.h"
#include "adc.h"
#define __EEPROM_C__
#include "eeprom.h"

// test for compilation
#if RTC_TIMERS_PER_DOW != 8
#error EEPROM layout is prepared for RTC_TIMERS_PER_DOW
#endif 


/*!
 *******************************************************************************
 *  \note standard asm/eeprom.h is not used. 
 *  Reason: eeprom_write_byte use only uint8_t address
 ******************************************************************************/

config_t config;

/*!
 *******************************************************************************
 *  generic EEPROM read
 *
 ******************************************************************************/

uint8_t EEPROM_read(uint16_t address) {
	/* Wait for completion of previous write */
	while(EECR & (1<<EEWE))
		;
	EEAR = address;
	EECR |= (1<<EERE);
	return EEDR;
}

/*!
 *******************************************************************************
 *  config_read
 *	it is similar as EEPROM_read, but optimized for special usage
 ******************************************************************************/

uint8_t config_read(uint8_t cfg_address, uint8_t cfg_type) {
	/* Wait for completion of previous write */
	while(EECR & (1<<EEWE))
		;
	EEAR = (((uint16_t) cfg_address) << 2) + cfg_type + (uint16_t)(&ee_config);
	EECR |= (1<<EERE);
	return EEDR;
}


/*!
 *******************************************************************************
 *  EEPROM_write
 *
 *  \note private function
 *  \note write to ee_config is limited 
 ******************************************************************************/
#define config_write(cfg_address,data) (EEPROM_write((((uint16_t) cfg_address) << 2) + CONFIG_VALUE + (uint16_t)(&ee_config),data))

static void EEPROM_write(uint16_t address, uint8_t data) {
	/* Wait for completion of previous write */
	if ((address >= (uint16_t)&ee_config) && (address & 3)) {
	  // write to eeconfig area is allowed only to column 0 / alligned to 4
    return; // write protection for configuration default/min/max data
  }
  while(EECR & (1<<EEWE))
		;
	EEAR = address;
	EEDR = data;
	asm ("cli");
	EECR |= (1<<EEMWE);
	EECR |= (1<<EEWE);
	asm ("sei");
}



/*!
 *******************************************************************************
 *  Init configuration storage
 *
 *  \note
 ******************************************************************************/

void eeprom_config_init(void) {
	
	uint16_t i;
	uint8_t *config_ptr = config_raw;
	for (i=0;i<CONFIG_RAW_SIZE;i++) {
		*config_ptr =  config_value(i);
		if ((*config_ptr < config_min(i)) //min
		 || (*config_ptr > config_max(i))) { //max
			*config_ptr = config_default(i); // default value
		}
		config_ptr++;
	}
}


/*!
 *******************************************************************************
 *  Update configuration storage
 *
 *  \note
 ******************************************************************************/
void eeprom_config_save(uint8_t idx) {
	if (idx<CONFIG_RAW_SIZE) {
		if (config_raw[idx] != config_value(idx)) {
			if ((config_raw[idx] < config_min(idx)) //min
		 	|| (config_raw[idx] > config_max(idx))) { //max
				config_raw[idx] = config_default(idx); // default value
			}
			config_write(idx, config_raw[idx]);
		}
	}
}


/*!
 *******************************************************************************
 *  Init timers storage
 *
 *  \note
 ******************************************************************************/

void eeprom_timers_init(void) {
	
	uint16_t i;
	uint8_t *timers_ptr = (uint8_t *) RTC_Dow_Timer;
	for (i=0;i<sizeof(RTC_Dow_Timer);i++) {
		*timers_ptr =  EEPROM_read(((uint16_t)&ee_timers)+i);
		timers_ptr++;
	}
}


/*!
 *******************************************************************************
 *  Update timers storage for dow and slot
 *
 *  \note
 ******************************************************************************/
void eeprom_timers_save(rtc_dow_t dow, uint8_t slot) {
	uint16_t i;
	uint8_t *timers_ptr = (uint8_t *) (RTC_Dow_Timer+RTC_TIMERS_PER_DOW*dow+slot);
    uint16_t eeaddr = (uint16_t)(ee_timers+RTC_TIMERS_PER_DOW*dow+slot);
	for (i=0;i<sizeof(RTC_Dow_Timer[0][0]);i++) {
		if (*timers_ptr !=  EEPROM_read(eeaddr+i)) {
		  EEPROM_write(eeaddr+i ,*timers_ptr);
        }
		timers_ptr++;
	}
}


