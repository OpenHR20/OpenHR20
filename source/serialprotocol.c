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
 * \file       serialprotocol.c
 * \brief      functions to parse the serial protocol
 * \author     Juergen Sachs (juergen-sachs-at-gmx-dot-de)
 * \date       16.03.2008
 * $Rev$
 */

// AVR LibC includes
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/version.h>
#include <string.h>

// HR20 Project includes
#include "main.h"
#include "serialprotocol.h"

// Enums


// global variables
volatile struct buffer inBuffer;
volatile struct buffer outBuffer;
volatile bool	cmdComplete;	// True is we received a "CR", so command should be processed
volatile uint8_t parserError;	// containes the Error number to generate
volatile char parserErrorCmdType;
volatile uint8_t byteToSend;

// Error strings in Flash
// Never prepend the COMMAND TYPE, this is done later by the function
const char errUnknown[] PROGMEM = "unknown error";
// Protocol errors
const char errProtParam[] PROGMEM = "Parameter error";
const char errProtInBuff[] PROGMEM = "Input Buffer Overflow";
const char errProtOutBuff[] PROGMEM = "Output Buffer Overflow";

// System Errors
const char errSysBattLow[] PROGMEM = "Battery Low";
const char errSysMotorBlocked[] PROGMEM = "Motor is blocked";

// Functions

/*!
 *******************************************************************************
 *  Initialize and clear serial buffer, setup Baudrate
 *
 *  \note
 *  - Initialize buffers
 *  - set Baudrate
 ******************************************************************************/
void parserInit(uint16_t baud)
{
	inBuffer.bytesInBuffer = 0;
	outBuffer.bytesInBuffer = 0;
	byteToSend = 0;
	parserError = P_ERR_NONE;
	parserErrorCmdType = CMD_TYPE_REPLY;
	cmdComplete = false;

	// Baudrate berechnen
	long ubrr_val = ((F_CPU)/(baud*16L)-1);
 
 	UBRRL = (ubrr_val & 0xFF);
	UCSRB = (_BV(TXEN) | _BV(RXEN) | _BV(RXCIE));      // UART TX einschalten
    UCSRC = (_BV(URSEL)| _BV(UCSZ0) | _BV(UCSZ1));     // Asynchron 8N1 
}

/*!
 *******************************************************************************
 *  process a received byte, and add it to buffer
 *
 *  \note
 *  - it does not actually process a complete packet, but sets a flag
 *    Mainline will call function "parserProcess()" to do this.
 ******************************************************************************/
void parserReceiverChar(char c)
{
	if (c == 0x10)	// Ignore Linefeed
		return;
	else	// add char to buffer
	{
		register uint8_t cnt = inBuffer.bytesInBuffer;
		if (cnt < MAX_BUFFER_SIZE)
		{
			inBuffer.buff[cnt] = c;
			cnt++;
			inBuffer.bytesInBuffer = cnt;
			if (c == 0x0D)
				cmdComplete = true;
		}
		else
		{
			parserError = P_ERR_IN_OVERFLOW;	// This will generate a Errormessage
			// Remove all until the last CR
			uint8_t i;
			for (i = inBuffer.bytesInBuffer ; i > 0 ; i--)
			{
				if (inBuffer.buff[i] == 0x0D)  break;
			}
			inBuffer.bytesInBuffer = i;
		}
	}
    return;
}

/*!
 *******************************************************************************
 *  Check if a valid command is in the receive buffer and process it
 *
 *  \note
 *  - should be called in mainline before sleep mode is entered
 *    to prevent long parsing withing Interrupts
 *  - we wait for the output buffer to be empty, this is makes all a lot easier.
 ******************************************************************************/
bool parserProcess(void)
{
	/***********************************************************************************/
	// We first check if the output buffer is empty
	if (outBuffer.bytesInBuffer != 0)
		return false;

/*	else if (parserError == P_ERR_NONE)
	{
		//parserOutAppend("test\r\n");
		parserError = 10;
	}
*/
	/***********************************************************************************/
	// Now we check if we have to send a error message.
	else if (parserError != P_ERR_NONE)
	{
		parserSendError(CMD_TYPE_NOTIFY, parserError);	// we do this until we are successfull !
	}

	/***********************************************************************************/
	else if (cmdComplete)	// did we have a complete command to parse
	{
		/***********************************************************************************/
		// note no string is NULL Terminated, so we can not use Std. functions !
		uint8_t crPos;
		uint8_t tmp;
		struct command cmd;

		/***********************************************************************************/
		// find end of command (CR)
		for (crPos = 0; crPos < MAX_BUFFER_SIZE; crPos++)
		{
			if (inBuffer.buff[crPos] == 0x0d) 
			{
				inBuffer.buff[crPos] = 0;
				break;
			}
		}
		if (crPos < MAX_BUFFER_SIZE)	// if valid
		{
			/***********************************************************************************/
			// find end of command		
			for (tmp = 0; tmp < crPos; tmp++)
			{
				if (inBuffer.buff[tmp] == '-') 
					break;
			}
			inBuffer.buff[tmp] = 0;
			cmd.cmd = (char*)(inBuffer.buff + 1);
			cmd.type = inBuffer.buff[0];


			/***********************************************************************************/
			// find all Parameters
			cmd.parCnt = 0;
			for (uint8_t p = 0 ; p < MAX_PARA_CNT; p++)
			{
				// "tmp" points to last command or param end, so add one
				tmp++;
				register uint8_t start = tmp;	// Remember start
				for ( ; tmp < crPos; tmp++)
				{
					register char c = inBuffer.buff[tmp];
					if ((c == ',') || (c == 0))
					{
						(cmd.par[p]) = (char*)(start + inBuffer.buff);
						inBuffer.buff[tmp] = 0;
						cmd.parCnt++;
						break;
					}
				}
			}
			
			/***********************************************************************************/
			// Now parse command
			parserProcessCmd(&cmd);	// make it a bit more readable to have a seperate function

			/***********************************************************************************/
			// Now remove parsed data, but be carefull. Maybe there is already more data
			if (crPos == (MAX_BUFFER_SIZE - 1))	// this command filed the buffer 100%
				inBuffer.bytesInBuffer = 0;
			else
			{
				// copy everthing to front of buffer
				crPos++;	// skip CR
				for (uint8_t t = 0; crPos < inBuffer.bytesInBuffer; t++)
				{
					inBuffer.buff[t] = inBuffer.buff[(crPos + t)];
				}
				// At last mark bytes as free
				// WARNING this should be atomic, otherwise Interrupt will interupt this ?!
				inBuffer.bytesInBuffer -= (crPos - 1);	//FIXME this should be atomic, otherwise Interrupt will interupt this ?!
			}

			/***********************************************************************************/
			// Finish
			return true;	// Yes well done, maybe there is more ?
		}
		else	// we do not have any more CR, so we have parsed all data !
		{
			if (inBuffer.bytesInBuffer >= MAX_BUFFER_SIZE) inBuffer.bytesInBuffer = 0;	// inBuffer is Garbage, we should have found one
			cmdComplete = false;	// Do this now and not earlier
		}
	}
	return false;	// dont call me again
}

/*!
 *******************************************************************************
 *  process this command
 *
 *  \note
 *  - none
 ******************************************************************************/
void parserProcessCmd(struct command *cmd)
{
	switch(cmd->type)
	{
		case CMD_TYPE_REQUEST:

			break;
		case CMD_TYPE_COMMAND:

			break;
		case CMD_TYPE_REPLY:
			parserSendError(CMD_TYPE_REPLY, P_ERR_CMD_TYPE);	// We only send this
			break;
		case CMD_TYPE_NOTIFY:
			parserSendError(CMD_TYPE_REPLY, P_ERR_CMD_TYPE);	// WE only send this
			break;
		default:
			parserSendError(CMD_TYPE_REPLY, P_ERR_CMD_TYPE);
			break;
	}
	return;
}

/*!
 *******************************************************************************
 *  Send the specific Error code
 *
 *  \note
 *  - remeber the last Error code and send it later in case there is not enough
 *    space in the output buffer
 ******************************************************************************/
void parserSendError(char cmdType, uint8_t errCode)
{
	parserError = errCode;	// Remember in Case we cna not send it now.
	parserErrorCmdType = cmdType;
	if (outBuffer.bytesInBuffer != 0) return;	// Wait until outbuffer is free, we will be called later

	// Now send Error code
	parserOutAppend_c(cmdType);
	parserOutAppend("ERR-");
	switch(errCode)
	{
		case P_ERR_PROTOCOL:			// More a basis value, not realy a code
			parserOutAppend_p(errUnknown);
			break;
		case P_ERR_PARAMETER:		// Parameter missing or wrong
			parserOutAppend_p(errProtParam);
			break;
		case P_ERR_IN_OVERFLOW:		// Input buffer overflow
			parserOutAppend_p(errProtInBuff);
			break;
		case P_ERR_OUT_OVERFLOW:		// Output buffer overflow
			parserOutAppend_p(errProtOutBuff);
			break;

		// System Errors
		case P_ERR_SYSTEM:		// More a basis value, not realy a code
			parserOutAppend_p(errUnknown);
			break;
		case P_ERR_BATT_LOW:			// Batterie low
			parserOutAppend_p(errSysBattLow);
			break;
		case P_ERR_MOTOR_BLOCK:		// Motor is blocked
			parserOutAppend_p(errSysMotorBlocked);
			break;
		default:
			parserOutAppend_p(errUnknown);
			break;
	}
	parserOutAppend("\r\n");
	parserStartSend();
	parserError = P_ERR_NONE;
	return;
}

/*!
 *******************************************************************************
 *  Appends on char to the serial output buffer
 *
 *  \note
 *  - none
 ******************************************************************************/
void parserOutAppend_c(const char data)
{
	if ((outBuffer.bytesInBuffer + 1) < MAX_BUFFER_SIZE)
	{
		outBuffer.buff[outBuffer.bytesInBuffer] = data;
		outBuffer.bytesInBuffer++;
	}
}

/*!
 *******************************************************************************
 *  Appends the FLASH string to the serial output buffer
 *
 *  \note
 *  - none
 ******************************************************************************/
void parserOutAppend_p(const char *data)
{
	uint8_t len = strlen_P(data);
	if ((outBuffer.bytesInBuffer + len) < MAX_BUFFER_SIZE)
	{
		strncpy_P((outBuffer.buff + outBuffer.bytesInBuffer), data, len);
		outBuffer.bytesInBuffer += len;
	}
}

/*!
 *******************************************************************************
 *  Appends the RAM string to the serial output buffer
 *
 *  \note
 *  - none
 ******************************************************************************/
void parserOutAppend(const char *data)
{
	uint8_t len = strlen(data);
	if ((outBuffer.bytesInBuffer + len) < MAX_BUFFER_SIZE)
	{
		strncpy((outBuffer.buff + outBuffer.bytesInBuffer), data, len);
		outBuffer.bytesInBuffer += len;
	}
}

/*!
 *******************************************************************************
 *  Appends the Integer as string to the serial output buffer
 *
 *  \note
 *  - none
 ******************************************************************************/
void parserOutAppendInt8(const uint8_t data)
{
	
}

/*!
 *******************************************************************************
 *  Starts sending the content of the output buffer
 *
 *  \note
 *  - we send the first char to serial port
 *  - start the interrupt
 *  - increase out byte send marker
 ******************************************************************************/
void parserStartSend(void)
{
	if (byteToSend == 0)	// start sending
	{
		UDR = outBuffer.buff[byteToSend];
		byteToSend++;
	}
	UCSRB |= _BV(UDRIE);	// save is save, we enable this every time
}


// One Char received
ISR(USART_RXC_vect)
{
	char c = UDR;
	parserReceiverChar(c);
}

// space to send more chars
ISR(USART_UDRE_vect)
{
	if (byteToSend < outBuffer.bytesInBuffer)
	{
		UDR = outBuffer.buff[byteToSend];
		byteToSend++;
	}
	else	// no more chars, disable Interrupt
	{
		UCSRB &= ~(_BV(UDRIE));
		byteToSend = 0;
		outBuffer.bytesInBuffer = 0;	// all send
	}
}
