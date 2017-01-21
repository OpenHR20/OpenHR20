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
 * \file       debug.h
 * \brief      debug options for project
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date: 2010-02-01 12:53:27 +0100 (Po, 01 úno 2010) $
 * $Rev: 273 $
 */

#pragma once
#include "config.h"

#define DEBUG_MODE 0
#define DEBUG_PRINT_ADDITIONAL_TIMESTAMPS DEBUG_MODE
