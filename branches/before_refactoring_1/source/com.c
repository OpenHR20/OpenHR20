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
 * \file       com.c
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
#include <avr/wdt.h> 
#include <stdlib.h>
#include <string.h>

// HR20 Project includes
#include "config.h"
#include "main.h"
#include "adc.h"
#include "rtc.h"
#include "motor.h"
#include "com.h"
#include "engine.h"

#if defined COM_RS232
#include "rs232_hw.h"
#elif defined COM_RS485
#include "rs485_hw.h"
#else
#error 'Serial config not supported, check config in config.h'
#endif

// our command types
const char CMD_TYPE_REQUEST = '?';
const char CMD_TYPE_COMMAND = '!';
const char CMD_TYPE_REPLY 	= '$';
const char CMD_TYPE_NOTIFY 	= '@';

const char CHAR_DASH = '-';
const char CHAR_EQUAL = '=';
const char CHAR_COLON = ':';
const char CHAR_DELIM = ',';
const char CHAR_DOT = '.';

// All known commands
const char cmdTemp[] PROGMEM	= "TEMP";
const char cmdVer[] PROGMEM 	= "VER";
const char cmdBatt[] PROGMEM 	= "BATT";
const char cmdClock[] PROGMEM 	= "CLOCK";
const char cmdERR[] PROGMEM 	= "ERR";
const char cmdValve[] PROGMEM 	= "VALVE";
const char cmdApp[] PROGMEM	= "APP";
const char cmdEcho[] PROGMEM	= "ECHO";
const char cmdReboot[] PROGMEM = "REBOOT";

// All known parameters, these are used amog many commands
const char parCur[] PROGMEM 	= "CUR";
const char parMax[] PROGMEM 	= "MAX";
const char parMin[] PROGMEM 	= "MIN";
const char parTime[] PROGMEM 	= "TIME";
const char parDate[] PROGMEM 	= "DATE";
const char parLow[] PROGMEM 	= "LOW";
const char parHigh[] PROGMEM 	= "HIGH";
const char parReset[] PROGMEM 	= "RESET";
const char parOk[] PROGMEM 		= "OK";
const char parCalibrate[] PROGMEM 	= "CAL";
const char parApp[] PROGMEM		= "openHR20E";
const char parSerial[] PROGMEM	= "SER";

const char LINEEND[] PROGMEM 	= "\r\n";

const char comModeRS232[] PROGMEM	= "RS232";
const char comModeRS485[] PROGMEM	= "RS485";
const char comModeUnknown[] PROGMEM	= "RS???";

// Error strings in Flash
// Never prepend the COMMAND TYPE, this is done later by the function
const char errUnknown[] 	PROGMEM 	= "unknown error";
// Protocol errors
const char errProtParam[] 	PROGMEM = "Parameter error";
const char errProtInBuff[] 	PROGMEM = "Input Buf Overflow";
const char errProtOutBuff[] 	PROGMEM = "Output Buf Overflow";
const char errProtUnknownType[] PROGMEM = "Unknown Cmd Type";
const char errProtUnknownCommand[] PROGMEM = "Unknown Cmd";
const char errProtIllegalValue[] PROGMEM = "Illegal Value";
// System Errors
const char errSysBattLow[] 	PROGMEM	= "Batt Low";
const char errSysMotorBlocked[] PROGMEM = "Motor blocked";

// global variables
volatile struct 	COM_buffer inBuffer;
volatile struct 	COM_buffer outBuffer;
volatile bool		cmdComplete;	// True is we received a "CR", so command should be processed
volatile uint8_t 	parserError;	// containes the Error number to generate
volatile char 		parserErrorCmdType;
volatile uint16_t	sendNotify;	// Bitwise flags which notifications have to be send
					// the actual send is done in "parserProcess()"

// should all entered data echoed back to host ?
// This must be done and checked in the hardware layer routines, this is flag ONLY
bool doLocalEcho;	


/*!
 *******************************************************************************
 *  Initialize and clear buffer, also calls hardware init functions
 *
 *  \note
 *  - Initialize buffers
 ******************************************************************************/
void COM_Init(uint16_t baud)
{
	inBuffer.bytesInBuffer = 0;
	outBuffer.bytesInBuffer = 0;
	parserError = COM_ERR_NONE;
	parserErrorCmdType = CMD_TYPE_REPLY;
	cmdComplete = false;
	doLocalEcho = false;

#if defined COM_RS232
	RS232_Init(baud);
#elif defined COM_RS485
	RS485_Init(baud);
#else
#warning 'Serial Init not set'
#endif
	
	// Send notification for whatever value we want.
	COM_setNotify(NOTIFY_VERSION);	// must
	COM_setNotify(NOTIFY_APP_NAME);	// must
	COM_setNotify(NOTIFY_BATT);		// nice to have
	COM_setNotify(NOTIFY_TEMP);		// nice to have
	COM_setNotify(NOTIFY_VALVE);	// nice to have
	COM_setNotify(NOTIFY_TIME);		// nice to have
	COM_setNotify(NOTIFY_ECHO);		// nice to have
	
}

/*!
 *******************************************************************************
 *  process a received byte, and add it to buffer
 *
 *  \note
 *  - it does not actually process a complete packet, but sets a flag
 *    Mainline will call function "COM_Process()" to do this.
 ******************************************************************************/
void COM_ReceiverChar(char c)
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
			parserError = COM_ERR_IN_OVERFLOW;	// This will generate a Errormessage
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
 *  Starts sending the content of the output buffer
 *
 *  \note
 ******************************************************************************/
void COM_StartSend(void)
{
	COM_OutAppend_p(LINEEND);
#if defined COM_RS232
	RS232_startSend();
#elif defined COM_RS485
	RS485_startSend();
#else
#warning 'Serial startSend not set'
#endif
}

/*!
 *******************************************************************************
 *  Get a specific byte of the output buffer
 *
 *  \note
 *	- There is no range check done here !
 ******************************************************************************/
uint8_t COM_getOutByte(uint8_t pos)	// Gets a specific byte of the output buffer
{
	return outBuffer.buff[pos];
}

/*!
 *******************************************************************************
 *  Get the current number of valid bytes in the output buffer
 *
 *  \note
 *	- none
 ******************************************************************************/
uint8_t COM_getBytesInBuffer(void)	// gets number of bytes in buffer
{
	return outBuffer.bytesInBuffer;
}

/*!
 *******************************************************************************
 *  Call this function when all bytes are send.
 *	If you do not do this, no future replyes will be done
 *
 *  \note
 *	- none
 ******************************************************************************/
void COM_finishedOutput(void)
{
	outBuffer.bytesInBuffer = 0;	// all send
}

/*!
 *******************************************************************************
 *  Check if a valid command is in the receive buffer and process it
 *
 *  \note
 *  - should be called in mainline before sleep mode is entered
 *    to prevent long parsing withing Interrupts
 *  - wait for the output buffer to be empty before processing next command in imput buffer. This makes all a lot easier.
 *  - if there is an outstanding error, send this one first
 ******************************************************************************/
bool COM_Process(void)
{
	/***********************************************************************************/
	// We first check if the output buffer is empty
	if (outBuffer.bytesInBuffer != 0)
		return false;

	/***********************************************************************************/
	// Now we check if we have to send a error message.
	else if (parserError != COM_ERR_NONE)
	{
		// since we send this unintended, it is a notify, not a command
		COM_SendError(false, parserError);	// we do this until we are successfull !
	}

	/***********************************************************************************/
	else if (cmdComplete)	// did we have a complete command to parse (Receiver CR)
	{
		/***********************************************************************************/
		// note no string is NULL Terminated, so we can not use Std. functions !
		uint8_t crPos;
		uint8_t tmp;
		struct COM_command cmd;

		/***********************************************************************************/
		// find end of command (CR)
		for (crPos = 0; crPos < MAX_BUFFER_SIZE; crPos++)
		{
			if (inBuffer.buff[crPos] == 0x0D) 
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
				if (inBuffer.buff[tmp] == CHAR_DASH) 
					break;
			}
			inBuffer.buff[tmp] = 0;					// this is save, if not found we are on the CR
			cmd.cmd = (char*)(inBuffer.buff + 1);	// pointer to command without startchar
			cmd.type = inBuffer.buff[0];			// Startchar


			/***********************************************************************************/
			// find all Parameters
			cmd.parCnt = 0;
			for (uint8_t p = 0 ; p < MAX_PARA_CNT; p++)
			{
				// "tmp" points to last command or param end, so add one
				tmp++;
				(cmd.par[cmd.parCnt]) = 0;
				(cmd.val[cmd.parCnt]) = 0;
				register uint8_t start = tmp;	// Remember start
				for ( ; tmp <= crPos; tmp++)
				{
					register char c = inBuffer.buff[tmp];
					if ((c == ',') || (c == 0))
					{
						(cmd.par[cmd.parCnt]) = (char*)(start + inBuffer.buff);
						inBuffer.buff[tmp] = 0;
						cmd.parCnt++;
						start = tmp;
						break;
					}
					else if (c == '=')
					{
						if (tmp < crPos)	// Is there realy some data
						{
							(cmd.val[cmd.parCnt]) = (char*)(tmp + inBuffer.buff + 1);
						}
						else
						{
							(cmd.val[cmd.parCnt]) = 0;
						}
						inBuffer.buff[tmp] = 0;
					}
				}
			}
			
			/***********************************************************************************/
			// Now parse command
			COM_ProcessCmd(&cmd);	// make it a bit more readable to have a seperate function

			/***********************************************************************************/
			// Now remove parsed data, but be carefull. Maybe there is already more data
			if (crPos == (MAX_BUFFER_SIZE - 1))	// this command filled the buffer 100%
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
				inBuffer.bytesInBuffer -= (crPos);	//FIXME this should be atomic, otherwise Interrupt will interupt this ?!
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
	/*
	We only send notifications, IF 
	- the local echo is on and no data is in the input buffer (confusing screen output)
	- local echo is off
	*/
	else if ((sendNotify != 0) && !(doLocalEcho && (inBuffer.bytesInBuffer > 0) ) )
	{
		// Send Notification and clear flag
		if 			(sendNotify & NOTIFY_MOTOR_BLOCKED) 	{ COM_SendError(false, COM_ERR_MOTOR_BLOCK); sendNotify ^= NOTIFY_MOTOR_BLOCKED; }
		else if (sendNotify & NOTIFY_APP_NAME) 				{ COM_SendAppName(false); sendNotify ^= NOTIFY_APP_NAME; }
		else if (sendNotify & NOTIFY_VERSION) 				{ COM_SendVersion(false); 	sendNotify ^= NOTIFY_VERSION; }
		else if (sendNotify & NOTIFY_TEMP) 						{ COM_SendTemp(false); 	sendNotify ^= NOTIFY_TEMP; }
		else if (sendNotify & NOTIFY_VALVE) 					{ COM_SendValve(false); 	sendNotify ^= NOTIFY_VALVE; }
		else if (sendNotify & NOTIFY_BATT) 						{ COM_SendBattery(false); 	sendNotify ^= NOTIFY_BATT; }
		else if (sendNotify & NOTIFY_TIME) 						{ COM_SendClock(false); 	sendNotify ^= NOTIFY_TIME; }
		else if (sendNotify & NOTIFY_CALIBRATE) 			{ COM_SendCalibrate(false); 	sendNotify ^= NOTIFY_CALIBRATE; }
		else if (sendNotify & NOTIFY_ECHO) 						{ COM_SendEchoStatus(false); 	sendNotify ^= NOTIFY_ECHO; }
		else 																					{ sendNotify = 0; }	// Garbage, clean up
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
void COM_ProcessCmd(struct COM_command *cmd)
{
	bool error = false;
	if (cmd->type == CMD_TYPE_REQUEST)
	{
		if (0 == strcmp_P(cmd->cmd, cmdTemp))
		{
			if  (cmd->parCnt == 0)
			{
				COM_SendTemp(true);
			}
			else
			{
				COM_SendError(true, COM_ERR_PARAMETER);
			}
		}
		else if (0 == strcmp_P(cmd->cmd, cmdVer))
		{
			COM_SendVersion(true);
		}
		else if (0 == strcmp_P(cmd->cmd, cmdClock))
		{
			COM_SendClock(true);
		}
		else if (0 == strcmp_P(cmd->cmd, cmdBatt))
		{
			COM_SendBattery(true);
		}
		else if (0 == strcmp_P(cmd->cmd, cmdValve))
		{
			COM_SendValve(true);
		}
		else if (0 == strcmp_P(cmd->cmd, cmdApp))
		{
			COM_SendAppName(true);
		}
		else if (0 == strcmp_P(cmd->cmd, cmdEcho))
		{
			COM_SendEchoStatus(true);
		}
		else
		{
			COM_SendError(true, COM_ERR_COMMAND);
		}
	}			
	else if (cmd->type == CMD_TYPE_COMMAND)
	{
		// local Echo on off
		if (0 == strcmp_P(cmd->cmd, cmdEcho))
		{
			if (cmd->parCnt == 1)
			{
				uint8_t t = atoi(cmd->par[0]);
				if (t) doLocalEcho = true;
				else doLocalEcho = false;
				COM_SendEchoStatus(true);
			}
			else
			{
				COM_SendError(true, COM_ERR_PARAMETER);
			}
		}
		else if (0 == strcmp_P(cmd->cmd, cmdReboot))
		{
			if (cmd->parCnt == 0)
			{
				COM_SendRebootStatus(true);
				//FIXME 'We loop here until a REBOOT command feedback is send and then do the final reboot, we might want to change this'
				while(COM_getBytesInBuffer() > 0)
				{
					nop();
				};
				wdt_enable(WDTO_2S);	// We force a reboot sing the watchdog. The only way to do a real reboot
				while(1)
				{
					nop();
				};
			}
			else
			{
				COM_SendError(true, COM_ERR_PARAMETER);
			}
		}
		// Clock Setting
		else if (0 == strcmp_P(cmd->cmd, cmdClock))
		{
			for (uint8_t i = 0 ; i < cmd->parCnt ; i++)
			{
				if (0 == strcmp_P(cmd->par[i], parTime))
				{
					char *p = (cmd->val[i]);
					uint8_t hh = atoi(p);
					// find colon
					while (*p != ':' && *p != 0){p++;};
					if (*p != 0)
					{
						p++;
						uint8_t mm = atoi(p);
						// find colon
						while (*p != ':' && *p != 0){p++;};
						if (*p != 0)
						{
							p++;
							uint8_t ss = atoi(p);
							// check values
							if (hh <= 23 && mm <= 59 && ss <= 59)
							{
								RTC_SetHour(hh);
								RTC_SetMinute(mm);
								RTC_SetSecond(ss);
							}
							else
							{
								error = true;
								COM_SendError(true, COM_ERR_VALUE);
								break;
							}
						}
						else
						{
							error = true;
							COM_SendError(true, COM_ERR_VALUE);
							break;
						}
					}
					else
					{
						error = true;
						COM_SendError(true, COM_ERR_VALUE);
						break;
					}
				}
				else if (0 == strcmp_P(cmd->par[i], parDate))
				{
					uint8_t dd, mm, yy;
					char *v = cmd->val[i];
					
					dd = atoi(v);
					while (*v != '.' && *v != 0){v++;};
					
					if (v != 0)
					{
						v++;
						mm = atoi(v);
						while (*v != '.' && *v != 0){v++;};
						
						if (v != 0)
						{
							v++;
							yy = atoi(v);
							
							if (!RTC_SetDate(dd, mm, yy))	// Wrong values 
								COM_SendError(true, COM_ERR_VALUE);
						}
						else
							COM_SendError(true, COM_ERR_VALUE);
					}
					else
						COM_SendError(true, COM_ERR_VALUE);
				}
				else
				{
					error = true;
					COM_SendError(true, COM_ERR_PARAMETER);
					break;
				}
			}
			if (error == false)
			{
				COM_SendClock(true);
			}
		}
		// Valve position and calibration
		else if (0 == strcmp_P(cmd->cmd, cmdValve))
		{
			if ((0 == strcmp_P(cmd->par[0], parCur)) && (cmd->parCnt == 1))
			{
				register uint8_t t = atoi(cmd->val[0]);
				if (t <= 100)
				{
					MOTOR_Goto(t, full);
					COM_SendValve(true);
				}
				else
				{
					COM_SendError(true, COM_ERR_VALUE);
				}
			}
			else if ((0 == strcmp_P(cmd->par[0], parCalibrate)) && (cmd->parCnt == 1))
			{
				MOTOR_Do_Calibrate();
			}
			else
			{
				COM_SendError(true, COM_ERR_PARAMETER);
			}
		}
		else if (0 == strcmp_P(cmd->cmd, cmdTemp))
		{
			if (0 == strcmp_P(cmd->par[0], parReset))
			{
				e_resetTemp();
				COM_SendTemp(true);
			}
			else
			{
				COM_SendError(true, COM_ERR_PARAMETER);
			}
		}
		// Unknown command
		else
		{
			COM_SendError(true, COM_ERR_COMMAND);
		}
	}
	else 
	{
		COM_SendError(true, COM_ERR_CMD_TYPE);	// We only send this
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
void COM_SendError(bool isReply, uint8_t errCode)
{
	parserError = errCode;	// Remember in Case we cna not send it now.
	parserErrorCmdType = isReply;
	if (outBuffer.bytesInBuffer != 0) return;	// Wait until outbuffer is free, we will be called later

	// Now send Error code
	if (isReply)
		COM_OutAppend_c(CMD_TYPE_REPLY);
	else
		COM_OutAppend_c(CMD_TYPE_NOTIFY);
	COM_OutAppend_p(cmdERR);
	COM_OutAppend_c(CHAR_DASH);
	COM_OutAppendInt8(errCode, 3);
	COM_OutAppend_c(CHAR_EQUAL);

	switch(errCode)
	{
		// Protocol errors
		case COM_ERR_PROTOCOL:		// More a basis value, not realy a code
			COM_OutAppend_p(errUnknown);
			break;
		case COM_ERR_PARAMETER:		// Parameter missing or wrong
			COM_OutAppend_p(errProtParam);
			break;
		case COM_ERR_IN_OVERFLOW:		// Input buffer overflow
			COM_OutAppend_p(errProtInBuff);
			break;
		case COM_ERR_OUT_OVERFLOW:	// Output buffer overflow
			COM_OutAppend_p(errProtOutBuff);
			break;
		case COM_ERR_CMD_TYPE:		// Output buffer overflow
			COM_OutAppend_p(errProtUnknownType);
			break;
		case COM_ERR_COMMAND:		// Illegal command
			COM_OutAppend_p(errProtUnknownCommand);
			break;
		case COM_ERR_VALUE:		// Illegal Value for Parameter
			COM_OutAppend_p(errProtIllegalValue);
			break;


		// System Errors
		case COM_ERR_SYSTEM:		// More a basis value, not realy a code
			COM_OutAppend_p(errUnknown);
			break;
		case COM_ERR_BATT_LOW:			// Batterie low
			COM_OutAppend_p(errSysBattLow);
			break;
		case COM_ERR_MOTOR_BLOCK:		// Motor is blocked
			COM_OutAppend_p(errSysMotorBlocked);
			break;
		default:
			COM_OutAppend_p(errUnknown);
			break;
	}
	COM_StartSend();
	parserError = COM_ERR_NONE;
	return;
}

/*!
 *******************************************************************************
 *  Appends on char to the serial output buffer
 *
 *  \note
 *  - none
 ******************************************************************************/
void COM_OutAppend_c(const char data)
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
void COM_OutAppend_p(const char *data)
{
	uint8_t len = strlen_P(data);
	if ((outBuffer.bytesInBuffer + len) < MAX_BUFFER_SIZE)
	{
		strncpy_P((char*)(outBuffer.buff + outBuffer.bytesInBuffer), data, len);
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
void COM_OutAppend(const char *data)
{
	uint8_t len = strlen(data);
	if ((outBuffer.bytesInBuffer + len) < MAX_BUFFER_SIZE)
	{
		strncpy((char*)(outBuffer.buff + outBuffer.bytesInBuffer), data, len);
		outBuffer.bytesInBuffer += len;
	}
}

/*!
 *******************************************************************************
 *  Appends the Unsigned Integer as string to the serial output buffer
 *
 *  \note
 *  - none
 ******************************************************************************/
void COM_OutAppendInt8(const uint8_t data, uint8_t len)
{
	char t[5];
	itoa(data, t, 10);
	if (len > 0)	// format text
	{
		if (len > 4) len = 4;
		COM_format(t, len);
	}
	COM_OutAppend(t);
}

/*!
 *******************************************************************************
 *  Formats the String to Zero prepended number
 *
 *  \note
 *  - this function cost abaout 150 Bytes of FLash :-( 
 *    If we run out of flash this would be a good point to save some !
 ******************************************************************************/
char *COM_format(char *d, uint8_t len)
{
		uint8_t l = strlen(d);
		if (l < len)
		{
			uint8_t offs;
			uint8_t i;
			
			//l++; // add NULL Byte
			offs = len - l; // offset
			//shift all to end
			for (i = l+1 ; i < 255 ; i--)	// we will overlap from Zero to 255 at the last char !
			{
				d[i+offs] = d[i];
			}
			// prepend with Zeros ASCII
			for (i = 0 ; i < offs ; i++)
			{
				d[i] = '0';
			}
		}
		return d;
}

/*!
 *******************************************************************************
 *  Appends the Integer as string to the serial output buffer
 *
 *  \note
 *  - none
 ******************************************************************************/
void COM_OutAppendInt16(const int16_t data, uint8_t len)
{
	char t[10];
	itoa(data, t, 10);
	if (len > 0)	// format text
	{
		if (len > 9) len = 9;
		COM_format(t, len);
	}
	COM_OutAppend(t);
}

/*!
 *******************************************************************************
 *  Sets Notification flags
 *
 *  \note
 *  - this will send out a notification about the change, as soon as possible
 *  - currently this function is to make the code more readable
 *    every call should be removed by the optimizer hopefully.
 ******************************************************************************/
void COM_setNotify(uint16_t val)
{
	sendNotify |= val;
}

/*!
 *******************************************************************************
 *  Generate a temperature feedback
 *
 *  \note
 *  - set isReply, if called upon a serial request
 *  \todo
 *  - uint16_t ADC_Get_Temp_Deg() depriciated 
 *    -> use int16_t ADC_Get_Temp_Degree()  
 ******************************************************************************/
void COM_SendTemp(bool isReply)
{
	// int16_t ADC_Get_Temp_Degree()
	//int16_t temp = ADC_Get_Temp_Degree();	// get current value
	//uint16_t temp = 1234;
	uint16_t temp;
	
	// Startbyte
	if (isReply) COM_OutAppend_c(CMD_TYPE_REPLY);
	else COM_OutAppend_c(CMD_TYPE_NOTIFY);
	
	// command
	COM_OutAppend_p(cmdTemp);

	// add delimiter
	COM_OutAppend_c(CHAR_DASH);

	temp = st.tempCur;
	// add parameter
	COM_OutAppend_p(parCur);
	COM_OutAppend_c(CHAR_EQUAL);
	COM_OutAppendInt16((temp / 100), 2);
	COM_OutAppend_c('.');
	COM_OutAppendInt16((temp % 100), 2);
	
	temp = st.tempMin;
	COM_OutAppend_c(',');
	// add parameter
	COM_OutAppend_p(parMin);
	COM_OutAppend_c(CHAR_EQUAL);
	COM_OutAppendInt16((temp / 100), 2);
	COM_OutAppend_c('.');
	COM_OutAppendInt16((temp % 100), 2);
	
	temp = st.tempMax;
	COM_OutAppend_c(',');
	// add parameter
	COM_OutAppend_p(parMax);
	COM_OutAppend_c(CHAR_EQUAL);
	COM_OutAppendInt16((temp / 100), 2);
	COM_OutAppend_c('.');
	COM_OutAppendInt16((temp % 100), 2);
		
	// done, send all
	COM_StartSend();
	return;
}

/*!
 *******************************************************************************
 *  Generate a version feedback
 *
 *  \note
 *  - set isReply, if called upon a serial request
 ******************************************************************************/
void COM_SendVersion(bool isReply)
{
	// Startbyte
	if (isReply) COM_OutAppend_c(CMD_TYPE_REPLY);
	else COM_OutAppend_c(CMD_TYPE_NOTIFY);
	
	// command
	COM_OutAppend_p(cmdVer);

	// add delimiter
	COM_OutAppend_c(CHAR_DASH);

	// add parameter
	COM_OutAppendInt8(REVHIGH, 0);
	COM_OutAppend_c('.');
	COM_OutAppendInt8(REVLOW, 0);
	COM_OutAppend_c('.');
	COM_OutAppendInt8(SVNREV, 0);
	
	// We append our currently implemented Interface
	COM_OutAppend_c(',');
#if defined COM_RS232
	COM_OutAppend_p(comModeRS232);
#elif defined COM_RS485
	COM_OutAppend_p(comModeRS485);
#else
	COM_OutAppend_p(comModeUnknown);
#endif
	
	// done, send all
	COM_StartSend();
	return;
}

/*!
 *******************************************************************************
 *  Generate a version feedback
 *
 *  \note
 *  - set isReply, if called upon a serial request
 ******************************************************************************/
void COM_SendRebootStatus(bool isReply)
{
	// Startbyte
	if (isReply) COM_OutAppend_c(CMD_TYPE_REPLY);
	else COM_OutAppend_c(CMD_TYPE_NOTIFY);
	
	// command
	COM_OutAppend_p(cmdReboot);

	// done, send all
	COM_StartSend();
	return;
}

/*!
 *******************************************************************************
 *  Generate a Applikation name feedback
 *
 *  \note
 *  - set isReply, if called upon a serial request
 ******************************************************************************/
void COM_SendAppName(bool isReply)
{
	// Startbyte
	if (isReply) COM_OutAppend_c(CMD_TYPE_REPLY);
	else COM_OutAppend_c(CMD_TYPE_NOTIFY);
	
	// command
	COM_OutAppend_p(cmdApp);

	// add delimiter
	COM_OutAppend_c(CHAR_DASH);

	// add parameter
	COM_OutAppend_p(parApp);
	
	COM_OutAppend_c(CHAR_DELIM);
	
	COM_OutAppendInt16(serialNumber, 5);
	
	// done, send all
	COM_StartSend();
	return;
}

/*!
 *******************************************************************************
 *  Generate a Battery feedback
 *
 *  \note
 *  - set isReply, if called upon a serial request
 ******************************************************************************/
void COM_SendBattery(bool isReply)
{
	// Startbyte
	if (isReply) COM_OutAppend_c(CMD_TYPE_REPLY);
	else COM_OutAppend_c(CMD_TYPE_NOTIFY);
	
	// command
	COM_OutAppend_p(cmdBatt);

	// add delimiter
	COM_OutAppend_c(CHAR_DASH);

	// add parameter
	register uint16_t bat = ADC_Get_Bat_Voltage();
	COM_OutAppendInt16(bat / 1000, 0);
	COM_OutAppend_c('.');
	COM_OutAppendInt16(bat % 1000, 0);
	
	COM_OutAppend_c(CHAR_DELIM);
	if (ADC_Get_Bat_isOk())
		COM_OutAppend_p(parOk);
	else
		COM_OutAppend_p(parLow);
	
	// done, send all
	COM_StartSend();
	return;
}

/*!
 *******************************************************************************
 *  Generate a Clock feedback
 *
 *  \note
 *  - set isReply, if called upon a serial request
 ******************************************************************************/
void COM_SendClock(bool isReply)
{
	// Startbyte
	if (isReply) COM_OutAppend_c(CMD_TYPE_REPLY);
	else COM_OutAppend_c(CMD_TYPE_NOTIFY);
	
	// command
	COM_OutAppend_p(cmdClock);

	// add delimiter
	COM_OutAppend_c(CHAR_DASH);

	// add parameter
	COM_OutAppend_p(parTime);
	COM_OutAppend_c(CHAR_EQUAL);
	COM_OutAppendInt8(RTC_GetHour(), 2);
	COM_OutAppend_c(CHAR_COLON);
	COM_OutAppendInt8(RTC_GetMinute(), 2);
	COM_OutAppend_c(CHAR_COLON);
	COM_OutAppendInt8(RTC_GetSecond(), 2);
	
	COM_OutAppend_c(CHAR_DELIM);
	
	COM_OutAppend_p(parDate);
	COM_OutAppend_c(CHAR_EQUAL);
	COM_OutAppendInt8(RTC_GetDay(), 2);
	COM_OutAppend_c(CHAR_DOT);
	COM_OutAppendInt8(RTC_GetMonth(), 2);
	COM_OutAppend_c(CHAR_DOT);
	COM_OutAppendInt8(RTC_GetYearYY(), 2);

	// done, send all
	COM_StartSend();
	return;
}

/*!
 *******************************************************************************
 *  Generate a Calibrate feedback
 *
 *  \note
 *  - set isReply, if called upon a serial request
 ******************************************************************************/
void COM_SendCalibrate(bool isReply)
{
	// Startbyte
	if (isReply) COM_OutAppend_c(CMD_TYPE_REPLY);
	else COM_OutAppend_c(CMD_TYPE_NOTIFY);
	
	// command
	COM_OutAppend_p(cmdValve);

	// add delimiter
	COM_OutAppend_c(CHAR_DASH);

	// add parameter
	COM_OutAppend_p(parCalibrate);

	// done, send all
	COM_StartSend();
	return;
}

/*!
 *******************************************************************************
 *  Generate a Valve feedback
 *
 *  \note
 *  - set isReply, if called upon a serial request
 ******************************************************************************/
void COM_SendValve(bool isReply)
{
	// Startbyte
	if (isReply) COM_OutAppend_c(CMD_TYPE_REPLY);
	else COM_OutAppend_c(CMD_TYPE_NOTIFY);
	
	// command
	COM_OutAppend_p(cmdValve);

	// add delimiter
	COM_OutAppend_c(CHAR_DASH);

	// add parameter
	COM_OutAppend_p(parCur);
	COM_OutAppend_c(CHAR_EQUAL);
	COM_OutAppendInt8(MOTOR_GetPosPercent(), 3);

	// add delimiter
	COM_OutAppend_c(CHAR_DELIM);

	// add parameter
	COM_OutAppend_p(parCalibrate);
	COM_OutAppend_c(CHAR_EQUAL);
	if (MOTOR_IsCalibrated())
		COM_OutAppendInt8(1, 0);
	else
		COM_OutAppendInt8(0, 0);
	
	// done, send all
	COM_StartSend();
	return;
}

/*!
 *******************************************************************************
 *  Generate a lokal echo status Feedback
 *
 *  \note
 *  - set isReply, if called upon a serial request
 ******************************************************************************/
void COM_SendEchoStatus(bool isReply)
{
	// Startbyte
	if (isReply) COM_OutAppend_c(CMD_TYPE_REPLY);
	else COM_OutAppend_c(CMD_TYPE_NOTIFY);
	
	// command
	COM_OutAppend_p(cmdEcho);

	// add delimiter
	COM_OutAppend_c(CHAR_DASH);

	// add parameter
	if (doLocalEcho) 
		COM_OutAppend_c('1');
	else
		COM_OutAppend_c('0');

	// done, send all
	COM_StartSend();
}

