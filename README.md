# OpenHR20

[![Build Status](https://travis-ci.org/OpenHR20/OpenHR20.svg?branch=master)](https://travis-ci.org/OpenHR20/OpenHR20)

This repository contains open firmware for Honeywell Rondostat HR20 and similar, Atmega-MCU based radiator thermostats. It is not based on the original, proprietary firmware, but rather a complete rewrite. It was started around 2008 by Jiri Dobry and Dario Carluccio, but has been changed and extended by many people since.

Currently supported thermostats are:
* HR20
* HR25
* THERMOTRONIC

Main improvements of this firmware are addition of wirelles and/or wired communication with central hub.

Original repository is still available at [SourceForge](https://sourceforge.net/projects/openhr20/). Original description page with a lot of interesting information is available in German [here](https://www.mikrocontroller.net/articles/Heizungssteuerung_mit_Honeywell_HR20).

## Compiling

As installing this firmware needs flashing a program to the thermostat MCU with hardware programmer, at least basic understanding of working with AVR MCUs and some additional hardware is required.

To compile the sources, avr compatible gcc crosscompiler is required. On many linux distributions, you can install this via packages, e.g. on debian based distros, installing "gcc-avr" package should install the whole required toolchain. For flashing, "avrdude" package is also required. For Windows, the [WinAVR](https://sourceforge.net/projects/winavr/) package should get you all the tools needed.

To compile the default configuration - HR20 version with RFM12B radio:

`make`

To compile the sources without wireless extension:

`make RFM=0`

To compile with predefined REVision ID

`make REV=-DREVISION=\\\"123456_XYZ\\\"`

To compile with hardware window open contact

`make HW_WINDOW_DETECTION=1`

thermotronic HW

`make HW=THERMOTRONIC`

## PINOUT HR20

The externally accesible connector on HR20/25 thermostats allows direct connection to the MCU for flashing via JTAG, or for wired communication. The connector layout is:

| ATmega169PV | <Func>(<Port,Pin>/<No.>) | | | |
| --- | ----------- | ----------- | ----------- | ------------ |
| Vcc | RXD(PE0/02) | TDO(PF6/55) | TMS(PF5/56) | /RST(PG5/20) |
| GND | TDI(PF7/54) | TXD(PE1/03) | TCK(PF4/57) |     (PE2/04) |

