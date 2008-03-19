/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  ompiler:    WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Juergen Sachs (juergen-sachs-at-gmx-dot-de)
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
 * \file       serialprotocol.h
 * \brief      functions to parse the serial protocol
 * \author     Juergen Sachs (juergen-sachs-at-gmx-dot-de)
 * \date       16.03.2008
 * $Rev$
 */

#ifndef SERIAL_PARSER_H
#define SERIAL_PARSER_H

// AVR LibC includes 
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/version.h>

// Maximum size for incomming Data
#define MAX_BUFFER_SIZE 32
// Maximum parameter count of a command or request
#define MAX_PARA_CNT 3
// our command types
#define CMD_TYPE_REQUEST '?'
#define CMD_TYPE_COMMAND '!'
#define CMD_TYPE_REPLY '$'
#define CMD_TYPE_NOTIFY '@'

/*!
 *******************************************************************************
 *  Possible Error Codes of the protocolParser and system
 *
 *  \note
 *  - Initialize buffers ONLY
 ******************************************************************************/
enum
{
	P_ERR_NONE = 0,			// No Error
	// Protocoll errors
	P_ERR_PROTOCOL = 100,	// More a basis value, not realy a code
	P_ERR_PARAMETER,		// Parameter missing or wrong
	P_ERR_IN_OVERFLOW,		// Input buffer overflow
	P_ERR_OUT_OVERFLOW,		// Output buffer overflow
	P_ERR_CMD_TYPE,			// Illegal command type

	// System Errors
	P_ERR_SYSTEM = 200,		// More a basis value, not realy a code
	P_ERR_BATT_LOW,			// Batterie low
	P_ERR_MOTOR_BLOCK		// Motor is blocked
};

/*!
 *******************************************************************************
 *  Structure containing all data for buffer handling
 *
 *  \note
 *  - none
 ******************************************************************************/
struct buffer
{
	char buff[MAX_BUFFER_SIZE+1];	// the physical buffer
	uint8_t bytesInBuffer;			// How many bytes are in the buffer
};


/*!
 *******************************************************************************
 *  Structure that contains all pointers to command parts NULL Terminated
 *
 *  \note
 *  - All Pointers should point to null temrinated string
 *    So it should be same to use std functions
 ******************************************************************************/
struct command
{
	char *cmd;
	char *par[MAX_PARA_CNT];				// parameter count
	uint8_t parCnt;							// Anzahl gefundener Parameter
	char type;								// type of the command (request, command, ...)
};

void parserInit(uint16_t baud);				// Initialize and clear serial buffer, init baudrate
bool parserProcess(void);					// Check if a valid command is in the receive buffer and process it
void parserReceiverChar(char c);			// checks received char and add to inBuffer
void parserProcessCmd(struct command *cmd);	// process this command
void parserSendError(char cmdType, uint8_t errCode);		// send a specific error code
void parserOutAppend_p(const char *data);	// Appends the FLASH string to the serial output buffer
void parserOutAppend(const char *data);		// Appends the RAM string to the serial output buffer
void parserOutAppend_c(const char data);		// Appends on char to the serial output buffer
void parserOutAppendInt8(const uint8_t data);// Appends the Integer as string to the serial output buffer
void parserStartSend(void);					// Starts sending the content of the output buffer

#endif /* SERIAL_PARSER_H */
