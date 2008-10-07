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
 * \file       com.h
 * \brief      functions to parse the serial protocol
 * \author     Juergen Sachs (juergen-sachs-at-gmx-dot-de)
 * \date       16.03.2008
 * $Rev$
 */

#ifndef COM_PARSER_H
#define COM_PARSER_H

// AVR LibC includes 
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/version.h>

// Maximum size for incomming Data
#define MAX_BUFFER_SIZE 40

// Maximum parameter count of a command or request
#define MAX_PARA_CNT 3

/*!
 *******************************************************************************
 * Available Notification Flags, They are Bitcoded !
 *
 *  \note
 * - use "setNotify(flagname)" to set flag !
 * - flags are send from LSB  to MSB order first (0x01)
 *   so give major flags a lower number !
 ******************************************************************************/
static const uint16_t NOTIFY_MOTOR_BLOCKED 	= 0x0001;
static const uint16_t NOTIFY_APP_NAME 			= 0x0002;
static const uint16_t NOTIFY_VERSION 				= 0x0004;
static const uint16_t NOTIFY_TEMP 					= 0x0008;
static const uint16_t NOTIFY_BATT 					= 0x0010;
static const uint16_t NOTIFY_TIME 					= 0x0020;
static const uint16_t NOTIFY_CALIBRATE 			= 0x0040;
static const uint16_t NOTIFY_VALVE 					= 0x0080;
static const uint16_t NOTIFY_ECHO 					= 0x0100; 

/*!
 *******************************************************************************
 *  Possible Error Codes of the protocolParser and system
 *
 *  \note
 *  - Initialize buffers ONLY
 ******************************************************************************/
enum
{
	COM_ERR_NONE = 0,			// No Error
	// Protocoll errors
	COM_ERR_PROTOCOL = 100,		// More a basis value, not realy a code
	COM_ERR_PARAMETER,		// Parameter missing or wrong
 	COM_ERR_COMMAND,			// Illegal command
	COM_ERR_IN_OVERFLOW,		// Input buffer overflow
	COM_ERR_OUT_OVERFLOW,		// Output buffer overflow
	COM_ERR_CMD_TYPE,			// Illegal command type
 	COM_ERR_VALUE,			// Illegal Value for Parameter

	// System Errors
	COM_ERR_SYSTEM = 200,		// More a basis value, not realy a code
	COM_ERR_BATT_LOW,			// Batterie low
	COM_ERR_MOTOR_BLOCK		// Motor is blocked
};

/*!
 *******************************************************************************
 *  Structure containing all data for buffer handling
 *
 *  \note
 *  - none
 ******************************************************************************/
struct COM_buffer
{
	char buff[MAX_BUFFER_SIZE+1];	// the physical buffer
	uint8_t bytesInBuffer;			// How many bytes are in the buffer
};


/*!
 *******************************************************************************
 *  Structure that contains all pointers to command parts NULL Terminated
 *
 *  \note
 *  - All Pointers should point to null terminated string
 *    So it should be save to use std functions
 ******************************************************************************/
struct COM_command
{
	char *cmd;
	char *par[MAX_PARA_CNT];		// parameter count
	char *val[MAX_PARA_CNT];		// the corresponding values, if any
	uint8_t parCnt;				// Anzahl gefundener Parameter
	char type;				// type of the command (request, command, ...)
};

extern bool doLocalEcho;

void COM_Init(uint16_t baud);			// Initialize and clear serial buffer, init baudrate
bool COM_Process(void);			// Check if a valid command is in the receive buffer and process it
void COM_ReceiverChar(char c);		// checks received char and add to inBuffer
void COM_ProcessCmd(struct COM_command *cmd);	// process this command
void COM_SendError(bool isReply, uint8_t errCode);		// send a specific error code
void COM_OutAppend_p(const char *data);	// Appends the FLASH string to the serial output buffer
void COM_OutAppend(const char *data);		// Appends the RAM string to the serial output buffer
void COM_OutAppend_c(const char data);	// Appends on char to the serial output buffer
void COM_OutAppendInt8(const uint8_t data, uint8_t len);	// Appends the Unsigned Integer as string to the serial output buffer
void COM_OutAppendInt16(const int16_t data, uint8_t len);	// Appends the Integer as string to the serial output buffer
void COM_setNotify(uint16_t val);			// Sets Notification flags
char *COM_format(char *d, uint8_t len);

// Functions for Reply messages
void COM_SendTemp(bool isReply);		// Generate a temperature feedback
void COM_SendVersion(bool isReply);
void COM_SendBattery(bool isReply);
void COM_SendClock(bool isReply);
void COM_SendCalibrate(bool isReply);
void COM_SendValve(bool isReply);
void COM_SendAppName(bool isReply);
void COM_SendEchoStatus(bool isReply);
void COM_SendRebootStatus(bool isReply);

// Hardware
void COM_StartSend(void);			// Starts sending the content of the output buffer
inline uint8_t COM_getOutByte(uint8_t);	// Gets a specific byte of the output buffer
inline uint8_t COM_getBytesInBuffer(void);	// gets number of bytes in buffer
void COM_finishedOutput(void);

#endif /* COM_PARSER_H */
