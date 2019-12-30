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
 * \file       controller.c
 * \brief      Controller for temperature
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

#include <stdint.h>
#include <stdlib.h>
#include <avr/pgmspace.h>

#include "config.h"
#include "main.h"
#include "task.h"
#include "com.h"
#include "common/rtc.h"
#include "adc.h"
#include "eeprom.h"
#include "controller.h"
#include "keyboard.h"

// global Vars for default values: temperatures and speed
uint8_t CTL_temp_wanted = 0;                    // actual desired temperature
uint8_t CTL_temp_wanted_last = 0xff;            // desired temperature value used for last PID control
uint8_t CTL_temp_auto_type = TEMP_TYPE_INVALID; // actual desired temperature type by timer
bool CTL_mode_auto = true;                      // actual desired temperature by timer
uint8_t CTL_mode_window = 0;                    // open window (0=closed, >0 open-timer)
#if (HW_WINDOW_DETECTION)
static uint8_t window_timer = AVERAGE_LEN + 1;
#else
uint16_t CTL_open_window_timeout;
#endif
int8_t CTL_interatorCredit;
uint8_t CTL_creditExpiration;
#if BOOST_CONTROLER_AFTER_CHANGE
uint8_t PID_boost_timeout = 0;                          //boost timout in minutes
#endif
static uint16_t PID_update_timeout = AVERAGE_LEN + 1;   // timer to next PID controler action/first is 16 sec after statup
int8_t PID_force_update = AVERAGE_LEN + 1;              // signed value, val<0 means disable force updates \todo rename
uint8_t valveHistory[VALVE_HISTORY_LEN];

static uint8_t pid_Controller(int16_t setPoint, int16_t processValue, uint8_t old_result, bool updateNow);

uint8_t CTL_error = 0;

__attribute__((noinline))    // do not inline in this file
void CTL_set_error(int8_t err_code)
{
	int8_t old_err = CTL_error;

	CTL_error |= err_code;
	if (CTL_error != old_err)
	{
		display_task = DISP_TASK_CLEAR | DISP_TASK_UPDATE;
	}
}

__attribute__((noinline))    // do not inline in this file
void CTL_clear_error(int8_t err_code)
{
	int8_t old_err = CTL_error;

	CTL_error &= ~err_code;
	if (CTL_error != old_err)
	{
		display_task = DISP_TASK_CLEAR | DISP_TASK_UPDATE;
	}
}

#if (HW_WINDOW_DETECTION)
static void CTL_window_detection(void)
{
	bool w;

	// PORTE |= _BV(PE2); // enable pull-up
	// nop();nop();
	w = ((PINE & _BV(PE2)) != 0) && config.window_open_detection_enable;
	if (!w)
	{
		PORTE &= ~_BV(PE2); // disable pullup for save energy

	}
	if (CTL_mode_window != w)
	{
		if (window_timer == 0)
		{
			CTL_mode_window = w;
			PID_force_update = 0;
			// kb_events |= KB_EVENT_UPDATE_LCD;
		}
		else
		{
			window_timer--;
			return;
		}
	}
	window_timer = (w) ? (config.window_close_detection_delay) : (config.window_open_detection_delay);
}
#else
static void CTL_window_detection(void)
{
	uint8_t i = (ring_buf_temp_avgs_pos + AVGS_BUFFER_LEN
		     - ((CTL_mode_window != 0) ? config.window_close_detection_time : config.window_open_detection_time)
		    ) % AVGS_BUFFER_LEN;
	int16_t min = 10000;
	int16_t max = 0;

	while (1)
	{
		int16_t x = ring_buf_temp_avgs[i];
		if (x != 0)   // startup condition
		{
			if (x < min)
			{
				min = x;
			}
			if (x > max)
			{
				max = x;
			}
		}
		if (i == ring_buf_temp_avgs_pos)
		{
			break;
		}
		i = (i + 1) % AVGS_BUFFER_LEN;
	}
	if ((temp_average - min) > (int16_t)config.window_close_detection_diff)
	{
		if (CTL_mode_window != 0)
		{
			CTL_mode_window = 0;
			PID_force_update = 0;
			//kb_events |= KB_EVENT_UPDATE_LCD;
		}
	}
	else
	{
		if ((CTL_mode_window == 0) && ((max - temp_average) > (int16_t)config.window_open_detection_diff))
		{
			CTL_mode_window = config.window_open_timeout;
			PID_force_update = 0;
			//kb_events |= KB_EVENT_UPDATE_LCD;
		}
	}
}
#endif

/*!
 *******************************************************************************
 *  Controller update
 *  \note call it once per second
 *  \param minute_ch is true when minute is changed
 *  \returns valve position
 *
 ******************************************************************************/
void CTL_update(bool minute_ch)
{
#if (HW_WINDOW_DETECTION)
	PORTE |= _BV(PE2); // enable pull-up
#endif

	if (minute_ch || (CTL_temp_auto_type == TEMP_TYPE_INVALID))
	{
		// minutes changed or we need return to timers
		uint8_t t = RTC_ActualTimerTemperatureType(!(CTL_temp_auto_type == TEMP_TYPE_INVALID));
		if (t != TEMP_TYPE_INVALID)
		{
			CTL_temp_auto_type = t;
			if (CTL_mode_auto)
			{
				CTL_temp_wanted = temperature_table[CTL_temp_auto_type];
				if ((PID_force_update < 0) && (CTL_temp_wanted != CTL_temp_wanted_last))
				{
					PID_force_update = 0;
				}
			}
		}
	}
#if BOOST_CONTROLER_AFTER_CHANGE
	if (minute_ch && (PID_boost_timeout > 0))
	{
		PID_boost_timeout--;
		if (PID_boost_timeout == 0)
		{
			PID_force_update = 0;
		}
	}
#endif

	CTL_window_detection();
	if (PID_update_timeout > 0)
	{
		PID_update_timeout--;
	}
	if (PID_force_update > 0)
	{
		PID_force_update--;
	}
	else if ((PID_update_timeout == 0) || (PID_force_update == 0))
	{
		uint8_t temp;
		if ((CTL_temp_wanted < TEMP_MIN) || mode_window())
		{
			temp = TEMP_MIN; // frost protection to TEMP_MIN
		}
		else
		{
			temp = CTL_temp_wanted;
		}
		bool updateNow = (temp != CTL_temp_wanted_last);
		if (updateNow || (PID_update_timeout == 0))
		{
			PID_update_timeout = (config.PID_interval * 5); // new PID pooling
			uint8_t new_valve;
			if (temp > TEMP_MAX)
			{
				new_valve = config.valve_max;
			}
			else
			{
				new_valve = pid_Controller(calc_temp(temp), temp_average, valveHistory[0], updateNow);
			}
			CTL_temp_wanted_last = temp;
			{
				int8_t i;
#if BLOCK_INTEGRATOR_AFTER_VALVE_CHANGE
				if (valveHistory[0] != new_valve)
				{
					CTL_integratorBlock = DEFINE_INTEGRATOR_BLOCK;               //block Integrator if valve moves

				}
#endif

				for (i = VALVE_HISTORY_LEN - 1; i > 0; i--)
				{
					if (updateNow || (new_valve <= config.valve_max) || (new_valve >= config.valve_min))
					{
						// condition inside loop is stupid, but produce shorter code
						valveHistory[i] = new_valve;
					}
					else
					{
						// cppcheck-suppress negativeIndex
						valveHistory[i] = valveHistory[i - 1];
					}
				}
				valveHistory[0] = new_valve;
			}
		}
		COM_print_debug(0);
		PID_force_update = -1; // invalid value = not used
	}
	// batt error detection
	if (bat_average)
	{
		if (bat_average < 20 * (uint16_t)config.bat_low_thld)
		{
			CTL_set_error(CTL_ERR_BATT_LOW | CTL_ERR_BATT_WARNING);
		}
		else
		{
			if (bat_average < 20 * (uint16_t)config.bat_warning_thld)
			{
				CTL_set_error(CTL_ERR_BATT_WARNING);
#if (BATT_ERROR_REVERSIBLE)
				CTL_clear_error(CTL_ERR_BATT_LOW);
#endif
			}
			else
			{
#if (BATT_ERROR_REVERSIBLE)
				CTL_clear_error(CTL_ERR_BATT_WARNING | CTL_ERR_BATT_LOW);
#endif
			}
		}
	}
}

/*!
 *******************************************************************************
 *  Change controller temperature (+-)
 *
 *  \param ch relative change
 ******************************************************************************/
void CTL_temp_change_inc(int8_t ch)
{
	CTL_temp_wanted += ch;
	if (CTL_temp_wanted < TEMP_MIN - 1)
	{
		CTL_temp_wanted = TEMP_MIN - 1;
	}
	else if (CTL_temp_wanted > TEMP_MAX + 1)
	{
		CTL_temp_wanted = TEMP_MAX + 1;
	}
	CTL_mode_window = 0;
	PID_force_update = 9;
	if (!CTL_mode_auto)
	{
		// save temp to config.timer_mode
		config.timer_mode = (CTL_temp_wanted << 1) + (config.timer_mode & 0x01);
		eeprom_config_save((uint16_t)(&config.timer_mode) - (uint16_t)(&config));
	}
}

static uint8_t menu_temp_rewoke_type;
/*!
 *******************************************************************************
 *  Change controller mode
 *
 ******************************************************************************/
void CTL_change_mode(int8_t m)
{
	if (m == CTL_CHANGE_MODE)
	{
		// change
		menu_temp_rewoke_type = CTL_temp_auto_type;
#if BOOST_CONTROLER_AFTER_CHANGE
		PID_boost_timeout = 0;         //jr disable boost
#endif
		CTL_mode_auto = !CTL_mode_auto;
		PID_force_update = 9;
	}
	else if (m == CTL_CHANGE_MODE_REWOKE)
	{
		//rewoke
		CTL_temp_auto_type = menu_temp_rewoke_type;
		CTL_mode_auto = !CTL_mode_auto;
		PID_force_update = 9;
	}
	else
	{
		if (m >= 0)
		{
			CTL_mode_auto = m;
		}
		PID_force_update = 0;
	}
	if (CTL_mode_auto && (m != CTL_CHANGE_MODE_REWOKE))
	{
		CTL_temp_auto_type = RTC_ActualTimerTemperatureType(false);
		CTL_temp_wanted = (CTL_temp_auto_type != TEMP_TYPE_INVALID ? temperature_table[CTL_temp_auto_type] : 0);
		config.timer_mode = config.timer_mode & 0x01;
		eeprom_config_save((uint16_t)(&config.timer_mode) - (uint16_t)(&config));
	}
	else
	{
		// save temp to config.timer_mode
		config.timer_mode = (CTL_temp_wanted << 1) + (config.timer_mode & 0x01);
		eeprom_config_save((uint16_t)(&config.timer_mode) - (uint16_t)(&config));
	}
	CTL_mode_window = 0;
	display_task = DISP_TASK_CLEAR | DISP_TASK_UPDATE;
}

//! Summation of errors, used for integrate calculations
int32_t sumError = 0;
static uint8_t lastErrorSign = 0;
static uint16_t lastAbsError;
static uint16_t last2AbsError;
uint8_t CTL_integratorBlock;
//! The scalling_factor for PID constants
#define scalling_factor  (256)
#if (scalling_factor != 256)
#error optimized only for (scalling_factor == 256)
#endif

//! Last process value, used to find derivative of process value.
static int16_t lastProcessValue = 0;
/*! \brief non-linear  PID control algorithm.
 *
 *  Calculates output from setpoint, process value and PID status.
 *
 *  \param setPoint  Desired value.
 *  \param processValue  Measured value.
 */

static uint16_t lastTempChangeErrorAbs;
static int32_t lastTempChangeSumError;

static void testIntegratorRevert(uint16_t absErr)
{
	if ((absErr >= ((lastTempChangeErrorAbs * 3) >> 2))
	    && (absErr > (I_ERR_TOLLERANCE_AROUND_0 * 2)))
	{
		// if error could not be reduced to 3/4 and Error is larger than I_ERR_TOLLERANCE_AROUND_0°C
		sumError = lastTempChangeSumError;
	}
	lastTempChangeErrorAbs = 0xffff; // function can be called more time but only first is valid
}


static uint8_t pid_Controller(int16_t setPoint, int16_t processValue, uint8_t old_result, bool updateNow)
{
	int32_t /*error2,*/ pi_term;
	int16_t error16;

	error16 = setPoint - processValue;

	// maximum error is 12 degree C
	if (error16 > 1200)
	{
		error16 = 1200;
	}
	else if (error16 < -1200)
	{
		error16 = -1200;
	}

	{
		int16_t absErr = abs(error16);
		if (updateNow)
		{
			CTL_interatorCredit = config.I_max_credit;
			CTL_creditExpiration = config.I_credit_expiration;
			CTL_integratorBlock = DEFINE_INTEGRATOR_BLOCK; // do not allow update integrator immediately after temp change
			testIntegratorRevert(lastAbsError);
			lastTempChangeErrorAbs = absErr;
			lastTempChangeSumError = sumError;
#if BOOST_CONTROLER_AFTER_CHANGE
			if ((abs(setPoint - CTL_temp_wanted_last) >= config.temp_boost_setpoint_diff)                                                                   // change of wanted temp large enough to start boost (0,5°C)
			    && (absErr >= (int16_t)config.temp_boost_error))                                                                                            // error large enough to start boost (0,3°C)
			{
				PID_boost_timeout = (error16 >= 0) ? config.temp_boost_time_heat : config.temp_boost_time_cool;
				PID_boost_timeout = (uint8_t)(MIN(255, abs(error16) / 10 * (int16_t)PID_boost_timeout / (int16_t)config.temp_boost_tempchange));        //boosttime=error/10(0,1°C)*time/tempchange
			}
#endif
		}
		else
		{
			if (CTL_integratorBlock == 0)
			{
				if (CTL_creditExpiration > 0)
				{
					CTL_creditExpiration--;
				}
				else
				{
					CTL_interatorCredit = 0;
				}
				if ((error16 >= 0) ? (old_result < config.valve_max) : (old_result > config.valve_min))
				{
					if (((lastErrorSign != ((uint8_t)(error16 >> 8) & 0x80))) ||
					    ((absErr == last2AbsError) && (absErr <= I_ERR_TOLLERANCE_AROUND_0)))       //sign of last error16 != sign of current OR abserror around 0
					{
						CTL_interatorCredit = config.I_max_credit;                              // ? optional
						CTL_creditExpiration = config.I_credit_expiration;
						goto INTEGRATOR;                                                        // next integration, do not change CTL_interatorCredit
					}
					if (CTL_interatorCredit > 0)
					{
						if (absErr >= last2AbsError)                                    // error can grow only limited time
						{
							CTL_interatorCredit -= (absErr / I_ERR_WEIGHT) + 1;     // max is 1200/20+1 = 61
INTEGRATOR:
							sumError += error16 * 8;
						}
					}
				}
				if (CTL_interatorCredit <= 0)
				{
					// credit is empty, test result
					testIntegratorRevert(absErr);
				}
			}
			else
			{
				CTL_integratorBlock--;
			}
			last2AbsError = lastAbsError;
			lastAbsError = absErr;
			lastErrorSign = (uint8_t)(error16 >> 8) & 0x80;
		}
	}
	lastProcessValue = processValue;

#if BOOST_CONTROLER_AFTER_CHANGE
	if (PID_boost_timeout > 0)
	{
		if ((abs(error16) <= (int16_t)(config.temp_boost_error - config.temp_boost_hystereses)))        // error <= temp_boost_error-temp_boost_hystereses
		{
			PID_boost_timeout = 0;                                                                  // end boost earlier, if error got to small (0,2°)
		}
		else
		{
			CTL_integratorBlock = DEFINE_INTEGRATOR_BLOCK;                                          //block Integrator
			if (error16 > 0)
			{
				return config.valve_max;                                                        //boost to max, no PID calculation
			}
			else
			{
				return config.valve_min;                                                        //boost to min, no PID calculation
			}
		}
	}
#endif

	if (config.I_Factor > 0)
	{
		int32_t maxSumError;
		// for overload protection: maximum is scalling_factor*scalling_factor*50/1 = 3276800
		maxSumError = ((int32_t)scalling_factor * (int32_t)scalling_factor * 50) / config.I_Factor;
		if (sumError > maxSumError)
		{
			sumError = maxSumError;
		}
		else if (sumError < -maxSumError)
		{
			sumError = -maxSumError;
		}
	}

	pi_term = (int32_t)(error16);
	pi_term *= pi_term * (int32_t)config.P3_Factor;
	pi_term >>= 8;
	pi_term += ((uint16_t)config.P_Factor << 8);
	pi_term *= (int32_t)error16;
	pi_term += (int32_t)(config.I_Factor) * sumError; // maximum is 65536*50=(scalling_factor*scalling_factor*50/I_Factor)*I_Factor
	/*
	 * pi_term - > for overload limit:
	 * maximum is +-(((255*1200*1200/256)+255)*1200+65536*50)
	 * = +-1724832800 fit into signed 32bit
	 */
	pi_term += (int32_t)(config.valve_center) * scalling_factor * scalling_factor;
	pi_term >>= 8; // /=scalling_factor

	if (pi_term > (int32_t)((uint16_t)config.valve_max * scalling_factor))
	{
		return config.valve_max;
	}
	else if (pi_term < 0)
	{
		return config.valve_min;
	}
	// now we can use 16bit value ( 0 < pi_term < 25600 )
	{
		uint16_t pi_term16 = pi_term;

		{
			bool gt = (uint8_t)(pi_term16 >> 8 /*/scalling_factor*/) >= old_result;
			// asymetric round, ignore changes < 3/4%
			pi_term16 += scalling_factor / 2;               // prepare for round
			if (gt)
			{
				pi_term16 -= config.valve_hysteresis;   // prepare for round
			}
			else
			{
				pi_term16 += config.valve_hysteresis;   // prepare for round
			}
			pi_term16 >>= 8;                                // /= scalling_factor;
		}
		if (pi_term16 < config.valve_min)
		{
			return config.valve_min;
		}

		return (uint8_t)pi_term16;
	}
}
