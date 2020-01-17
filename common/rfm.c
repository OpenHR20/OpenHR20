/*
 *  Open HR20
 *
 *  target:     ATmega169 in Honnywell Rondostat HR20E / ATmega8
 *
 *  compiler:   WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Dario Carluccio (hr20-at-carluccio-dot-de)
 *              2008 Jiri Dobry (jdobry-at-centrum-dot-cz)
 *              2008 Mario Fischer (MarioFischer-at-gmx-dot-net)
 *              2007 Michael Smola (Michael-dot-Smola-at-gmx-dot-net)
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
 * \file       rfm.c
 * \brief      functions to control the RFM12 Radio Transceiver Module
 * \author     Mario Fischer <MarioFischer-at-gmx-dot-net>; Michael Smola <Michael-dot-Smola-at-gmx-dot-net>
 * \date       $Date$
 * $Rev$
 */

#include "config.h"
#include "rfm_config.h"
#include "rfm.h"
#include "eeprom.h"
#include "task.h"


#if (RFM == 1)

uint8_t rfm_framebuf[RFM_FRAME_MAX];
uint8_t rfm_framesize = 6;
uint8_t rfm_framepos = 0;
rfm_mode_t rfm_mode = rfmmode_stop;

/*!
 *******************************************************************************
 *  RFM SPI access
 *  \note does the SPI clockin clockout stuff
 *  \param outval the value that shall be clocked out to the RFM
 *  \returns the value that is clocked in from the RFM
 *
 ******************************************************************************/
uint16_t rfm_spi16(uint16_t outval)
{
	uint8_t i;
	uint16_t ret = 0; // <- not needeed will be shifted out ; Reintroduced to fix maybe-uninitialized

	RFM_SPI_SELECT;

	for (i = 16; i != 0; i--)
	{
		if (0x8000 & outval)
		{
			RFM_SPI_MOSI_HIGH;
		}
		else
		{
			RFM_SPI_MOSI_LOW;
		}
		outval <<= 1;

		RFM_SPI_SCK_HIGH;

		{
			ret <<= 1;
			if (RFM_SPI_MISO_GET)
			{
				ret |= 1;
			}
		}
		RFM_SPI_SCK_LOW;
	}

	RFM_SPI_DESELECT;
	RFM_SPI_SELECT;

	return ret;
}


///////////////////////////////////////////////////////////////////////////////
//
// Initialise RF module
//
///////////////////////////////////////////////////////////////////////////////

void RFM_init(void)
{
#if (NANODE == 1 || JEENODE == 1)
	// disable SPI
	SPCR &= ~(1 << SPE);
#endif

	// 0. Init the SPI backend
	//RFM_TESTPIN_INIT;

	RFM_READ_STATUS();

	// 1. Configuration Setting Command
	RFM_SPI_16(
		RFM_CONFIG_EL |
		RFM_CONFIG_EF |
		RFM_CONFIG_Band(RFM_FREQ_MAIN) |
		RFM_CONFIG_X_12_0pf
	);

	// 2. Power Management Command
	//RFM_SPI_16(
	//	 RFM_POWER_MANAGEMENT     // switch all off
	//	 );

	// 3. Frequency Setting Command
#if (RFM_TUNING > 0)
	int8_t adjust = config.RFM_freqAdjust;
#else
	int8_t adjust = 0;
#endif
	RFM_SPI_16(
		RFM_FREQUENCY |
		(RFM_FREQ_Band(RFM_FREQ_MAIN)(RFM_FREQ_DEC) + adjust)
	);

	// 4. Data Rate Command
	RFM_SPI_16(RFM_SET_DATARATE(RFM_BAUD_RATE));

	// 5. Receiver Control Command
	RFM_SPI_16(
		RFM_RX_CONTROL_P20_VDI  |
		RFM_RX_CONTROL_VDI_MED |
		RFM_RX_CONTROL_BW(RFM_BAUD_RATE) |
		RFM_RX_CONTROL_GAIN_6   |
		RFM_RX_CONTROL_RSSI_103
	);

	// 6. Data Filter Command
	RFM_SPI_16(
		RFM_DATA_FILTER_DQD(4)
	);

	// 7. FIFO and Reset Mode Command
	RFM_SPI_16(
		RFM_FIFO_IT(8) |
		RFM_FIFO_DR
	);

	// 8. Synchron Pattern Command

	// 9. Receiver FIFO Read

	// 10. AFC Command
	RFM_SPI_16(
		RFM_AFC_AUTO_VDI |
		RFM_AFC_RANGE_LIMIT_7_8 |
		RFM_AFC_EN |
		RFM_AFC_OE |
		RFM_AFC_FI
	);

	// 11. TX Configuration Control Command
	RFM_SPI_16(
		RFM_TX_CONTROL_MOD(RFM_BAUD_RATE) |
		RFM_TX_CONTROL_POW_0
	);

	// 12. PLL Setting Command
	RFM_SPI_16(
		RFM_PLL |
		RFM_PLL_uC_CLK_10 |
		RFM_PLL_DELAY_OFF |
		RFM_PLL_DITHER_OFF |
		RFM_PLL_BIRATE_LOW
	);

	// 13. Transmitter Register Write Command

	// 14. Wake-Up Timer Command

	// 15. Low Duty-Cycle Command

	// 16. Low Battery Detector Command
#if RFM_CLK_OUTPUT
	RFM_SPI_16(RFM_LOW_BATT_DETECT_D_10MHZ);
#endif

	//RFM_SPI_16(
	//	 RFM_LOW_BATT_DETECT |
	//	 3      // 2.2V + v * 0.1V
	//	 );

	// 17. Status Read Command
}

///////////////////////////////////////////////////////////////////////////////

/*!
 *******************************************************************************
 * Pinchange Interupt, RFM handling part
 ******************************************************************************/
#if !defined(MASTER_CONFIG_H)
void RFM_interrupt(uint8_t pine)
{
	if (PCMSK0 & _BV(RFM_SDO_PCINT))
	{
		// RFM module interupt
		while (RFM_SDO_PIN & _BV(RFM_SDO_BITPOS))
		{
			PCMSK0 &= ~_BV(RFM_SDO_PCINT);  // disable RFM interrupt
			sei();                          // enable global interrupts
			if (rfm_mode == rfmmode_tx)
			{
				RFM_WRITE(rfm_framebuf[rfm_framepos++]);
				if (rfm_framepos >= rfm_framesize)
				{
					rfm_mode = rfmmode_tx_done;
					task |= TASK_RFM;       // inform the rfm task about end of transmition
					return;                 // \note !!WARNING!!
				}
			}
			else if (rfm_mode == rfmmode_rx)
			{
				rfm_framebuf[rfm_framepos++] = RFM_READ_FIFO();
				task |= TASK_RFM; // inform the rfm task about next RX byte
				if (rfm_framepos >= RFM_FRAME_MAX)
				{
					rfm_mode = rfmmode_rx_owf;
					// ignore any data in buffer
					return; // RFM interrupt disabled
				}
			}
			cli();                          // disable global interrupts
			asm volatile ("nop");           // we must have one instruction after cli()
			PCMSK0 |= _BV(RFM_SDO_PCINT);   // enable RFM interrupt
			asm volatile ("nop");           // we must have one instruction after
		}
	}
}

#else // !defined(MASTER_CONFIG_H)

/*!
 *******************************************************************************
 * Pinchange Interupt INT2
 *
 * \note level interrupt is better, but I want to have same code for master as for HR20 (jdobry)
 ******************************************************************************/
// RFM module interupt
volatile uint8_t afc = 0;
ISR(RFM_INT_vect)
{
#if (JEENODE == 1)
	uint16_t status = RFM_READ_STATUS();    // this also clears most interrupt sources
	if (status & RFM_STATUS_RGIT)           // we are using level interrupt on jeenode
	{
#else
	while (RFM_SDO_PIN & _BV(RFM_SDO_BITPOS))
	{
#endif
		RFM_INT_DIS();                          // disable RFM interrupt
		sei();                                  // enable global interrupts
#if (JEENODE == 0)
		uint16_t status = RFM_READ_STATUS();    // this also clears most interrupt sources
#endif
		if (rfm_mode == rfmmode_tx)
		{
			RFM_WRITE(rfm_framebuf[rfm_framepos++]);
			if (rfm_framepos >= rfm_framesize)
			{
				rfm_mode = rfmmode_tx_done;
				task |= TASK_RFM;       // inform the rfm task about end of transmition
				return;                 // \note !!WARNING!!
			}
		}
		else if (rfm_mode == rfmmode_rx)
		{
			rfm_framebuf[rfm_framepos++] = RFM_READ_FIFO();
#if (RFM_TUNING > 0)
			if (rfm_framepos == 6)   // get AFC value
			{
				afc = status & 0x1f;
			}

#endif
			if (rfm_framepos >= RFM_FRAME_MAX)
			{
				rfm_mode = rfmmode_rx_owf;
			}
			task |= TASK_RFM; // inform the rfm task about next RX byte
		}
		else if (rfm_mode == rfmmode_rx_owf)
		{
			RFM_DISCARD_FIFO();
			task |= TASK_RFM;       // inform the rfm task about next RX byte
		}
		cli();                          // disable global interrupts
		asm volatile ("nop");           // we must have one instruction after cli()
		RFM_INT_EN_NOCALL();
		asm volatile ("nop");           // we must have one instruction after
	}
	// do NOT add anything after RFM part
}
#endif  // !defined(MASTER_CONFIG_H)

#endif  // ifdef RFM
