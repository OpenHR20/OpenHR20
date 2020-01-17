/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
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
 * \file       rfm.h
 * \brief      functions to control the RFM12 Radio Transceiver Module
 * \author     Mario Fischer <MarioFischer-at-gmx-dot-net>; Michael Smola <Michael-dot-Smola-at-gmx-dot-net>
 * \date       $Date$
 * $Rev$
 */

#pragma once // multi-iclude prevention. gcc knows this pragma
#if RFM

#define RFM_SPI_16(OUTVAL)                      rfm_spi16(OUTVAL) //<! a function that gets a uint16_t (clocked out value) and returns a uint16_t (clocked in value)

#define RFM_SPI_SELECT                  (RFM_NSEL_PORT &= ~_BV(RFM_NSEL_BITPOS))
#define RFM_SPI_DESELECT                (RFM_NSEL_PORT |= _BV(RFM_NSEL_BITPOS))

#define RFM_SPI_MOSI_LOW                (RFM_SDI_PORT &= ~_BV(RFM_SDI_BITPOS))
#define RFM_SPI_MOSI_HIGH               (RFM_SDI_PORT |= _BV(RFM_SDI_BITPOS))

#define RFM_SPI_MISO_GET                        (RFM_SDO_PIN & _BV(RFM_SDO_BITPOS))

#define RFM_SPI_SCK_LOW                 (RFM_SCK_PORT &= ~_BV(RFM_SCK_BITPOS))
#define RFM_SPI_SCK_HIGH                (RFM_SCK_PORT |= _BV(RFM_SCK_BITPOS))

#define RFM_TESTPIN_INIT
#define RFM_TESTPIN_ON
#define RFM_TESTPIN_OFF
#define RFM_TESTPIN_TOG

#define RFM_CONFIG_DISABLE                      0x00    //<! RFM_CONFIG_*** are combinable flags, what the RFM shold do
#define RFM_CONFIG_BROADCASTSTATUS      0x01            //<! Flag that enables the HR20's status broadcast every minute

#define RFM_CONFIG_ENABLEALL            0xff


///////////////////////////////////////////////////////////////////////////////
//
// RFM status bits
//
///////////////////////////////////////////////////////////////////////////////

// Interrupt bits, latched ////////////////////////////////////////////////////

#define RFM_STATUS_FFIT 0x8000  // RX FIFO reached the progr. number of bits
                                // Cleared by any FIFO read method

#define RFM_STATUS_RGIT 0x8000  // TX register is ready to receive
                                // Cleared by TX write

#define RFM_STATUS_POR  0x4000  // Power On reset
                                // Cleared by read status

#define RFM_STATUS_RGUR 0x2000  // TX register underrun, register over write
                                // Cleared by read status

#define RFM_STATUS_FFOV 0x2000  // RX FIFO overflow
                                // Cleared by read status

#define RFM_STATUS_WKUP 0x1000  // Wake up timer overflow
                                // Cleared by read status

#define RFM_STATUS_EXT  0x0800  // Interupt changed to low
                                // Cleared by read status

#define RFM_STATUS_LBD  0x0400  // Low battery detect

// Status bits ////////////////////////////////////////////////////////////////

#define RFM_STATUS_FFEM 0x0200  // FIFO is empty
#define RFM_STATUS_ATS  0x0100  // TX mode: Strong enough RF signal
#define RFM_STATUS_RSSI 0x0100  // RX mode: signal strength above programmed limit
#define RFM_STATUS_DQD  0x0080  // Data Quality detector output
#define RFM_STATUS_CRL  0x0040  // Clock recovery lock
#define RFM_STATUS_ATGL 0x0020  // Toggling in each AFC cycle

///////////////////////////////////////////////////////////////////////////////
//
// 1. Configuration Setting Command
//
///////////////////////////////////////////////////////////////////////////////

#define RFM_CONFIG               0x8000

#define RFM_CONFIG_EL            0x8080 // Enable TX Register
#define RFM_CONFIG_EF            0x8040 // Enable RX FIFO buffer
#define RFM_CONFIG_BAND_315      0x8000 // Frequency band
#define RFM_CONFIG_BAND_433      0x8010
#define RFM_CONFIG_BAND_868      0x8020
#define RFM_CONFIG_BAND_915      0x8030
#define RFM_CONFIG_X_8_5pf       0x8000 // Crystal Load Capacitor
#define RFM_CONFIG_X_9_0pf       0x8001
#define RFM_CONFIG_X_9_5pf       0x8002
#define RFM_CONFIG_X_10_0pf      0x8003
#define RFM_CONFIG_X_10_5pf      0x8004
#define RFM_CONFIG_X_11_0pf      0x8005
#define RFM_CONFIG_X_11_5pf      0x8006
#define RFM_CONFIG_X_12_0pf      0x8007
#define RFM_CONFIG_X_12_5pf      0x8008
#define RFM_CONFIG_X_13_0pf      0x8009
#define RFM_CONFIG_X_13_5pf      0x800A
#define RFM_CONFIG_X_14_0pf      0x800B
#define RFM_CONFIG_X_14_5pf      0x800C
#define RFM_CONFIG_X_15_0pf      0x800D
#define RFM_CONFIG_X_15_5pf      0x800E
#define RFM_CONFIG_X_16_0pf      0x800F

///////////////////////////////////////////////////////////////////////////////
//
// 2. Power Management Command
//
///////////////////////////////////////////////////////////////////////////////

#define RFM_POWER_MANAGEMENT     0x8200

#define RFM_POWER_MANAGEMENT_ER  0x8280 // Enable receiver
#define RFM_POWER_MANAGEMENT_EBB 0x8240 // Enable base band block
#define RFM_POWER_MANAGEMENT_ET  0x8220 // Enable transmitter
#define RFM_POWER_MANAGEMENT_ES  0x8210 // Enable synthesizer
#define RFM_POWER_MANAGEMENT_EX  0x8208 // Enable crystal oscillator
#define RFM_POWER_MANAGEMENT_EB  0x8204 // Enable low battery detector
#define RFM_POWER_MANAGEMENT_EW  0x8202 // Enable wake-up timer
#define RFM_POWER_MANAGEMENT_DC  0x8201 // Disable clock output of CLK pin

#ifndef RFM_CLK_OUTPUT
#error RFM_CLK_OUTPUT must be defined to 0 or 1
#endif
#if RFM_CLK_OUTPUT
#define RFM_TX_ON_PRE() RFM_SPI_16( \
		RFM_POWER_MANAGEMENT_ES | \
		RFM_POWER_MANAGEMENT_EX)
#define RFM_TX_ON()     RFM_SPI_16( \
		RFM_POWER_MANAGEMENT_ET | \
		RFM_POWER_MANAGEMENT_ES | \
		RFM_POWER_MANAGEMENT_EX)
#define RFM_RX_ON()     RFM_SPI_16( \
		RFM_POWER_MANAGEMENT_ER | \
		RFM_POWER_MANAGEMENT_EBB | \
		RFM_POWER_MANAGEMENT_ES | \
		RFM_POWER_MANAGEMENT_EX)
#define RFM_OFF()       RFM_SPI_16( \
		RFM_POWER_MANAGEMENT_EX)
#else
#define RFM_TX_ON_PRE() RFM_SPI_16( \
		RFM_POWER_MANAGEMENT_DC | \
		RFM_POWER_MANAGEMENT_ES | \
		RFM_POWER_MANAGEMENT_EX)
#define RFM_TX_ON()     RFM_SPI_16( \
		RFM_POWER_MANAGEMENT_DC | \
		RFM_POWER_MANAGEMENT_ET | \
		RFM_POWER_MANAGEMENT_ES | \
		RFM_POWER_MANAGEMENT_EX)
#define RFM_RX_ON()     RFM_SPI_16( \
		RFM_POWER_MANAGEMENT_DC | \
		RFM_POWER_MANAGEMENT_ER | \
		RFM_POWER_MANAGEMENT_EBB | \
		RFM_POWER_MANAGEMENT_ES | \
		RFM_POWER_MANAGEMENT_EX)
#define RFM_OFF()       RFM_SPI_16(RFM_POWER_MANAGEMENT_DC)
#endif
///////////////////////////////////////////////////////////////////////////////
//
// 3. Frequency Setting Command
//
///////////////////////////////////////////////////////////////////////////////

#define RFM_FREQUENCY            0xA000

#define RFM_FREQ_315Band(v) (uint16_t)((v / 10.0 - 31) * 4000)
#define RFM_FREQ_433Band(v) (uint16_t)((v / 10.0 - 43) * 4000)
#define RFM_FREQ_868Band(v) (uint16_t)((v / 20.0 - 43) * 4000)
#define RFM_FREQ_915Band(v) (uint16_t)((v / 30.0 - 30) * 4000)

// helper macros to derive macro name from main frequency
#define IntRFM_FREQ_Band(v) RFM_FREQ_ ## v ## Band
#define IntRFM_CONFIG_Band(v) RFM_CONFIG_BAND_ ## v
#define RFM_FREQ_Band(v) IntRFM_FREQ_Band(v)
#define RFM_CONFIG_Band(v) IntRFM_CONFIG_Band(v)

///////////////////////////////////////////////////////////////////////////////
//
// 4. Data Rate Command
//
/////////////////////////////////////////////////////////////////////////////////

#ifndef RFM_BAUD_RATE
#define RFM_BAUD_RATE           19200
#endif

#ifndef RFM_FREQ_MAIN
#define RFM_FREQ_MAIN           868
#endif

#ifndef RFM_FREQ_FINE
#define RFM_FREQ_FINE           0.35
#endif

#define RFM_FREQ_DEC            (RFM_FREQ_MAIN + RFM_FREQ_FINE)

#define RFM_DATA_RATE            0xC600

#define RFM_DATA_RATE_CS         0xC680
#define RFM_DATA_RATE_4800       0xC647
#define RFM_DATA_RATE_9600       0xC623
#define RFM_DATA_RATE_19200      0xC611
#define RFM_DATA_RATE_38400      0xC608
#define RFM_DATA_RATE_57600      0xC605

// Using this formula as specified in the datasheet results in a slightly inflated data rate due to rounding. Original: #define RFM_SET_DATARATE_ORIG(baud)		( ((baud)<5400) ? (RFM_DATA_RATE_CS|((43104/(baud))-1)) : (RFM_DATA_RATE|((344828UL/(baud))-1)) )
#define RFM_SET_DATARATE(baud)          (((baud) < 4800) ? (RFM_DATA_RATE_CS | ((43104 / (baud)))) : (RFM_DATA_RATE | ((344828UL / (baud)))))

///////////////////////////////////////////////////////////////////////////////
//
// 5. Receiver Control Command
//
///////////////////////////////////////////////////////////////////////////////

#define RFM_RX_CONTROL           0x9000

#define RFM_RX_CONTROL_P20_INT   0x9000 // Pin20 = ExternalInt
#define RFM_RX_CONTROL_P20_VDI   0x9400 // Pin20 = VDI out

#define RFM_RX_CONTROL_VDI_FAST  0x9000 // fast   VDI Response time
#define RFM_RX_CONTROL_VDI_MED   0x9100 // medium
#define RFM_RX_CONTROL_VDI_SLOW  0x9200 // slow
#define RFM_RX_CONTROL_VDI_ON    0x9300 // Always on

#define RFM_RX_CONTROL_BW_400    0x9020 // bandwidth 400kHz
#define RFM_RX_CONTROL_BW_340    0x9040 // bandwidth 340kHz
#define RFM_RX_CONTROL_BW_270    0x9060 // bandwidth 270kHz
#define RFM_RX_CONTROL_BW_200    0x9080 // bandwidth 200kHz
#define RFM_RX_CONTROL_BW_134    0x90A0 // bandwidth 134kHz
#define RFM_RX_CONTROL_BW_67     0x90C0 // bandwidth 67kHz

#define RFM_RX_CONTROL_GAIN_0    0x9000 // LNA gain  0db
#define RFM_RX_CONTROL_GAIN_6    0x9008 // LNA gain -6db
#define RFM_RX_CONTROL_GAIN_14   0x9010 // LNA gain -14db
#define RFM_RX_CONTROL_GAIN_20   0x9018 // LNA gain -20db

#define RFM_RX_CONTROL_RSSI_103  0x9000 // DRSSI threshold -103dbm
#define RFM_RX_CONTROL_RSSI_97   0x9001 // DRSSI threshold -97dbm
#define RFM_RX_CONTROL_RSSI_91   0x9002 // DRSSI threshold -91dbm
#define RFM_RX_CONTROL_RSSI_85   0x9003 // DRSSI threshold -85dbm
#define RFM_RX_CONTROL_RSSI_79   0x9004 // DRSSI threshold -79dbm
#define RFM_RX_CONTROL_RSSI_73   0x9005 // DRSSI threshold -73dbm
//#define RFM_RX_CONTROL_RSSI_67   0x9006 // DRSSI threshold -67dbm // RF12B reserved
//#define RFM_RX_CONTROL_RSSI_61   0x9007 // DRSSI threshold -61dbm // RF12B reserved

// See datasheet page 37
#define RFM_RX_CONTROL_BW(baud)         (((baud) < 20000)   \
					 ?          RFM_RX_CONTROL_BW_67   \
					 : ( \
						 ((baud) < 100000)   \
						 ? RFM_RX_CONTROL_BW_134   \
						 : RFM_RX_CONTROL_BW_200 \
					 ))

///////////////////////////////////////////////////////////////////////////////
//
// 6. Data Filter Command
//
///////////////////////////////////////////////////////////////////////////////

#define RFM_DATA_FILTER          0xC228

#define RFM_DATA_FILTER_AL       0xC2A8 // clock recovery auto-lock
#define RFM_DATA_FILTER_ML       0xC268 // clock recovery fast mode
#define RFM_DATA_FILTER_DIG      0xC228 // data filter type digital
#define RFM_DATA_FILTER_ANALOG   0xC238 // data filter type analog
#define RFM_DATA_FILTER_DQD(level) (RFM_DATA_FILTER | (level & 0x7))

///////////////////////////////////////////////////////////////////////////////
//
// 7. FIFO and Reset Mode Command
//
///////////////////////////////////////////////////////////////////////////////

#define RFM_FIFO                 0xCA00

#define RFM_FIFO_AL              0xCA04 // FIFO Start condition sync-word/always
#define RFM_FIFO_FF              0xCA02 // Enable FIFO fill
#define RFM_FIFO_DR              0xCA01 // Disable hi sens reset mode
#define RFM_FIFO_IT(level)       (RFM_FIFO | (((level) & 0xF) << 4))

#define RFM_FIFO_OFF()            RFM_SPI_16(RFM_FIFO_IT(8) | RFM_FIFO_DR)
#define RFM_FIFO_ON()             RFM_SPI_16(RFM_FIFO_IT(8) | RFM_FIFO_FF | RFM_FIFO_DR)

/////////////////////////////////////////////////////////////////////////////
//
// 8. Synchron Pattern Command
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// 9. Receiver FIFO Read
//
/////////////////////////////////////////////////////////////////////////////

#define RFM_READ_FIFO()           (RFM_SPI_16(0xB000) & 0xFF)
// Introduced for when there is no need to use the byte read from FIFO
#define RFM_DISCARD_FIFO()        (RFM_SPI_16(0xB000))

/////////////////////////////////////////////////////////////////////////////
//
// 10. AFC Command
//
/////////////////////////////////////////////////////////////////////////////

#define RFM_AFC                  0xC400

#define RFM_AFC_EN               0xC401
#define RFM_AFC_OE               0xC402
#define RFM_AFC_FI               0xC404
#define RFM_AFC_ST               0xC408

// Limits the value of the frequency offset register to the next values:

#define RFM_AFC_RANGE_LIMIT_NO    0xC400        // 0: No restriction
#define RFM_AFC_RANGE_LIMIT_15_16 0xC410        // 1: +15 fres to -16 fres
#define RFM_AFC_RANGE_LIMIT_7_8   0xC420        // 2: +7 fres to -8 fres
#define RFM_AFC_RANGE_LIMIT_3_4   0xC430        // 3: +3 fres to -4 fres

// fres=2.5 kHz in 315MHz and 433MHz Bands
// fres=5.0 kHz in 868MHz Band
// fres=7.5 kHz in 915MHz Band

#define RFM_AFC_AUTO_OFF         0xC400 // 0: Auto mode off (Strobe is controlled by microcontroller)
#define RFM_AFC_AUTO_ONCE        0xC440 // 1: Runs only once after each power-up
#define RFM_AFC_AUTO_VDI         0xC480 // 2: Keep the foffset only during receiving(VDI=high)
#define RFM_AFC_AUTO_INDEPENDENT 0xC4C0 // 3: Keep the foffset value independently trom the state of the VDI signal

///////////////////////////////////////////////////////////////////////////////
//
// 11. TX Configuration Control Command
//
///////////////////////////////////////////////////////////////////////////////

#define RFM_TX_CONTROL           0x9800

#define RFM_TX_CONTROL_POW_0     0x9800
#define RFM_TX_CONTROL_POW_3     0x9801
#define RFM_TX_CONTROL_POW_6     0x9802
#define RFM_TX_CONTROL_POW_9     0x9803
#define RFM_TX_CONTROL_POW_12    0x9804
#define RFM_TX_CONTROL_POW_15    0x9805
#define RFM_TX_CONTROL_POW_18    0x9806
#define RFM_TX_CONTROL_POW_21    0x9807
#define RFM_TX_CONTROL_MOD_15    0x9800
#define RFM_TX_CONTROL_MOD_30    0x9810
#define RFM_TX_CONTROL_MOD_45    0x9820
#define RFM_TX_CONTROL_MOD_60    0x9830
#define RFM_TX_CONTROL_MOD_75    0x9840
#define RFM_TX_CONTROL_MOD_90    0x9850
#define RFM_TX_CONTROL_MOD_105   0x9860
#define RFM_TX_CONTROL_MOD_120   0x9870
#define RFM_TX_CONTROL_MOD_135   0x9880
#define RFM_TX_CONTROL_MOD_150   0x9890
#define RFM_TX_CONTROL_MOD_165   0x98A0
#define RFM_TX_CONTROL_MOD_180   0x98B0
#define RFM_TX_CONTROL_MOD_195   0x98C0
#define RFM_TX_CONTROL_MOD_210   0x98D0
#define RFM_TX_CONTROL_MOD_225   0x98E0
#define RFM_TX_CONTROL_MOD_240   0x98F0
#define RFM_TX_CONTROL_MP        0x9900

// See datasheet page 37
#define RFM_TX_CONTROL_MOD(baud)        (((baud) < 20000)   \
					 ?            RFM_TX_CONTROL_MOD_45   \
					 : ( \
						 ((baud) < 100000)   \
						 ? RFM_TX_CONTROL_MOD_90   \
						 : RFM_TX_CONTROL_MOD_120 \
					 ) \
)

/////////////////////////////////////////////////////////////////////////////
//
// 12. PLL Setting Command
//
/////////////////////////////////////////////////////////////////////////////

#define RFM_PLL                                 0xCC02
#define RFM_PLL_uC_CLK_10               0x70
#define RFM_PLL_uC_CLK_3_3              0x50
#define RFM_PLL_uC_CLK_2_5              0x30

#define RFM_PLL_DELAY_ON                0x08
#define RFM_PLL_DELAY_OFF               0x00
#define RFM_PLL_DITHER_ON               0x00
#define RFM_PLL_DITHER_OFF              0x04
#define RFM_PLL_BIRATE_HI               0x01
#define RFM_PLL_BIRATE_LOW              0x00

/////////////////////////////////////////////////////////////////////////////
//
// 13. Transmitter Register Write Command
//
/////////////////////////////////////////////////////////////////////////////

//#define RFM_WRITE(byte)  RFM_SPI_16(0xB800 | ((byte) & 0xFF))
#define RFM_WRITE(byte)  RFM_SPI_16(0xB800 | (byte))

///////////////////////////////////////////////////////////////////////////////
//
// 14. Wake-up Timer Command
//
///////////////////////////////////////////////////////////////////////////////

#define RFM_WAKEUP_TIMER         0xE000
#define RFM_WAKEUP_SET(time)     RFM_SPI_16(RFM_WAKEUP_TIMER | (time))

#define RFM_WAKEUP_480s          (RFM_WAKEUP_TIMER | (11 << 8) | 234)
#define RFM_WAKEUP_240s          (RFM_WAKEUP_TIMER | (10 << 8) | 234)
#define RFM_WAKEUP_120s          (RFM_WAKEUP_TIMER | (9 << 8) | 234)
#define RFM_WAKEUP_119s          (RFM_WAKEUP_TIMER | (9 << 8) | 232)

#define RFM_WAKEUP_60s           (RFM_WAKEUP_TIMER | (8 << 8) | 235)
#define RFM_WAKEUP_59s           (RFM_WAKEUP_TIMER | (8 << 8) | 230)

#define RFM_WAKEUP_30s           (RFM_WAKEUP_TIMER | (7 << 8) | 235)
#define RFM_WAKEUP_29s           (RFM_WAKEUP_TIMER | (7 << 8) | 227)

#define RFM_WAKEUP_8s            (RFM_WAKEUP_TIMER | (5 << 8) | 250)
#define RFM_WAKEUP_7s            (RFM_WAKEUP_TIMER | (5 << 8) | 219)
#define RFM_WAKEUP_6s            (RFM_WAKEUP_TIMER | (6 << 8) | 94)
#define RFM_WAKEUP_5s            (RFM_WAKEUP_TIMER | (5 << 8) | 156)
#define RFM_WAKEUP_4s            (RFM_WAKEUP_TIMER | (5 << 8) | 125)
#define RFM_WAKEUP_1s            (RFM_WAKEUP_TIMER | (2 << 8) | 250)
#define RFM_WAKEUP_900ms         (RFM_WAKEUP_TIMER | (2 << 8) | 225)
#define RFM_WAKEUP_800ms         (RFM_WAKEUP_TIMER | (2 << 8) | 200)
#define RFM_WAKEUP_700ms         (RFM_WAKEUP_TIMER | (2 << 8) | 175)
#define RFM_WAKEUP_600ms         (RFM_WAKEUP_TIMER | (2 << 8) | 150)
#define RFM_WAKEUP_500ms         (RFM_WAKEUP_TIMER | (2 << 8) | 125)
#define RFM_WAKEUP_400ms         (RFM_WAKEUP_TIMER | (2 << 8) | 100)
#define RFM_WAKEUP_300ms         (RFM_WAKEUP_TIMER | (2 << 8) | 75)
#define RFM_WAKEUP_200ms         (RFM_WAKEUP_TIMER | (2 << 8) | 50)
#define RFM_WAKEUP_100ms         (RFM_WAKEUP_TIMER | (2 << 8) | 25)

///////////////////////////////////////////////////////////////////////////////
//
// 15. Low Duty-Cycle Command
//
///////////////////////////////////////////////////////////////////////////////

#define RFM_LOW_DUTY_CYCLE       0xC800

///////////////////////////////////////////////////////////////////////////////
//
// 16. Low Battery Detector Command
//
///////////////////////////////////////////////////////////////////////////////

#define RFM_LOW_BATT_DETECT           0xC000
#define RFM_LOW_BATT_DETECT_D_1MHZ    0xC000
#define RFM_LOW_BATT_DETECT_D_1_25MHZ 0xC020
#define RFM_LOW_BATT_DETECT_D_1_66MHZ 0xC040
#define RFM_LOW_BATT_DETECT_D_2MHZ    0xC060
#define RFM_LOW_BATT_DETECT_D_2_5MHZ  0xC080
#define RFM_LOW_BATT_DETECT_D_3_33MHZ 0xC0A0
#define RFM_LOW_BATT_DETECT_D_5MHZ    0xC0C0
#define RFM_LOW_BATT_DETECT_D_10MHZ   0xC0E0

///////////////////////////////////////////////////////////////////////////////
//
// 17. Status Read Command
//
///////////////////////////////////////////////////////////////////////////////

#define RFM_READ_STATUS()       RFM_SPI_16(0x0000)

///////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
void RFM_init(void);
uint16_t rfm_spi16(uint16_t outval);

///////////////////////////////////////////////////////////////////////////////

// RFM air protocol flags:

#define RFMPROTO_FLAGS_BITASK_PACKETTYPE                0b11000000      //!< the uppermost 2 bits of the flags field encode the packettype
#define RFMPROTO_FLAGS_PACKETTYPE_BROADCAST             0b00000000      //!< broadcast packettype (message from hr20, protocol; step 1)
#define RFMPROTO_FLAGS_PACKETTYPE_COMMAND               0b01000000      //!< command packettype (message to hr20, protocol; step 2)
#define RFMPROTO_FLAGS_PACKETTYPE_REPLY                 0b10000000      //!< reply packettype (message from hr20, protocol; step 3)
#define RFMPROTO_FLAGS_PACKETTYPE_SPECIAL               0b11000000      //!< currently unused packettype

#define RFMPROTO_FLAGS_BITASK_DEVICETYPE                0b00011111      //!< the lowermost 5 bytes denote the device type. this way other sensors and actors may coexist
#define RFMPROTO_FLAGS_DEVICETYPE_OPENHR20              0b00010100      //!< topen HR20 device type. 10100 is for decimal 20

#define RFMPROTO_IS_PACKETTYPE_BROADCAST(FLAGS) (RFMPROTO_FLAGS_PACKETTYPE_BROADCAST == ((FLAGS)&RFMPROTO_FLAGS_BITASK_PACKETTYPE))
#define RFMPROTO_IS_PACKETTYPE_COMMAND(FLAGS)   (RFMPROTO_FLAGS_PACKETTYPE_COMMAND == ((FLAGS)&RFMPROTO_FLAGS_BITASK_PACKETTYPE))
#define RFMPROTO_IS_PACKETTYPE_REPLY(FLAGS)             (RFMPROTO_FLAGS_PACKETTYPE_REPLY == ((FLAGS)&RFMPROTO_FLAGS_BITASK_PACKETTYPE))
#define RFMPROTO_IS_PACKETTYPE_SPECIAL(FLAGS)   (RFMPROTO_FLAGS_PACKETTYPE_SPECIAL == ((FLAGS)&RFMPROTO_FLAGS_BITASK_PACKETTYPE))
#define RFMPROTO_IS_DEVICETYPE_OPENHR20(FLAGS)  (RFMPROTO_FLAGS_DEVICETYPE_OPENHR20 == ((FLAGS)&RFMPROTO_FLAGS_BITASK_DEVICETYPE))

///////////////////////////////////////////////////////////////////////////////

// RFM send and receive buffer:

#define RFM_FRAME_MAX 80

typedef enum { rfmmode_stop     = 0,
	       rfmmode_start_tx = 1,
	       rfmmode_tx       = 2,
	       rfmmode_tx_done  = 3,
	       rfmmode_rx       = 4,
	       rfmmode_rx_owf   = 5, } rfm_mode_t;

extern uint8_t rfm_framebuf[RFM_FRAME_MAX];
extern uint8_t rfm_framesize;
extern uint8_t rfm_framepos;
extern rfm_mode_t rfm_mode;

#if !defined(MASTER_CONFIG_H)
void RFM_interrupt(uint8_t pine);
#endif // !defined(MASTER_CONFIG_H)

#define rfm_start_tx()
// (rfm_mode=rfmmode_start_tx)
#endif // RFM
