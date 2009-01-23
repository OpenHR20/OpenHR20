/*
 * Copyright (C) 2009 Bjoern Biesenbach <bjoern@bjoern-b.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*!
* \file	hr20.c
* \brief	functions to communicate with openhr20
* \author	Bjoern Biesenbach <bjoern at bjoern-b dot de>
*/

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>

#include "serial.h"
#include "hr20.h"
/*!
 ********************************************************************************
 * hr20GetTimer
 *
 * get the content of a timer
 *
 * \param day day
 * \param slot slot
 * \param *value returns cddd where c is the mode (comfort,save,etc) and ddd the minutes since 00:00 in hex
 *******************************************************************************/
void hr20GetTimer(int day, int slot, char *value)
{
	char buffer[20];
	char response[255];
	char *result;

	sprintf(buffer,"\rR%d%d\r",day,slot);
	
	while(1)
	{
		serialCommand(buffer, response);
		if(response[0] == 'R' )
			break;
		usleep(1000);
	}
	
	result = strtok(response,"=");
	result = strtok(NULL,"=");
	strcpy(value,result);
}

int hexCharToInt(char c)
{
	if(c <= 57)
		return c - 48;
	return c - 87;
}

/*!
 ********************************************************************************
 * hr20ParseTimer
 *
 * parse the timer string
 *
 * \param *timer timer string
 *******************************************************************************/
void hr20ParseTimer(char *timer)
{
	int minutes = 0;

	printf("Mode:");
	if(timer[0] == '0')
		printf("frost protection  ");
	else if(timer[0] == '1')
		printf("energy save       ");
	else if(timer[0] == '2')
		printf("comfort           ");
	else if(timer[0] == '3')
		printf("supercomfort      ");

	minutes = 256 * hexCharToInt(timer[1]);
	minutes += 16 * hexCharToInt(timer[2]);
	minutes +=      hexCharToInt(timer[3]);

	printf("Time: %02d:%02d\n",minutes/60,minutes%60);
}

void hr20SetTimer(char *timer_string)
{
	int day, slot, mode, minutes;
	char buffer[20];

	if(strlen(timer_string) != 7)
	{
		printf("Wrong format in string!\n");
		return;
	}

	day = timer_string[0]-48;
	slot = timer_string[1]-48;
	mode = timer_string[2]-48;

	if(day < 0 || day > 7)
	{
		printf("Day must be between 0 and 7\n");
		return;
	}
	if(mode < 0 || mode > 3)
	{
		printf("Mode must be between 0 and 3\n");
		return;
	}

	minutes = 600 * (timer_string[3]-48) + 60 * (timer_string[4]-48) + 10 * (timer_string[5]-48) + (timer_string[6]-48);
	
	sprintf(buffer,"\rW%d%d%d%03x\r",day,slot,mode,minutes);

	serialCommand(buffer,0);
}

void hr20UnsetTimer(int day, int slot)
{
	char buffer[20];

	sprintf(buffer,"\rW%d%d0fff\r",day,slot);
	serialCommand(buffer,0);
}
	

void hr20GetAllTimers()
{
	int day,slot;
	char result[5];

	for(day=0;day<8;day++)
	{
		printf("Day   %d\n",day);
		slot = 0;
		while(1)
		{
			hr20GetTimer(day,slot,result);
			if(result[1] == 'f')
				break;
			printf("Slot  %d   ",slot);
			hr20ParseTimer(result);
			slot++;
		}
		printf("\n");
	}
}


/*!
 ********************************************************************************
 * hr20SetTemperature
 *
 * set the wanted temperature
 *
 * \param temperature the wanted temperature multiplied with 10. only steps
 *  	of 5 are allowed
 * \returns returns 1 on success, 0 on failure
 *******************************************************************************/
int hr20SetTemperature(int temperature)
{
	if(temperature % 5) // temperature may only be XX.5°C
		return 0;

	char buffer[255];
	char response[255];

	sprintf(buffer,"\rA%x\r", temperature/5);

	serialCommand(buffer,response);

	return 1;
}

/*!
 ********************************************************************************
 * hr20SetDateAndTime
 *
 * transfer current date and time to the openhr20 device
 *
 * \returns returns 1 on success
 *******************************************************************************/
int hr20SetDateAndTime()
{
	time_t rawtime;
	struct tm *ptm;

	char send_string[255];

	time(&rawtime);
	ptm = localtime(&rawtime);

	sprintf(send_string,"\rY%02x%02x%02x\r",ptm->tm_year-100, ptm->tm_mon+1, ptm->tm_mday);
	serialCommand(send_string, 0);
	
	sprintf(send_string,"\rH%02x%02x%02x\r",ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	serialCommand(send_string, 0);
	return 1;
}

/*!
 ********************************************************************************
 * hr20SetModeManu
 *
 * set the mode to manual control
 *******************************************************************************/
void hr20SetModeManu()
{
	serialCommand("\rM00\r", 0);
}

/*!
 ********************************************************************************
 * hr20SetModeAuto
 *
 * set the mode to automatic control
 *******************************************************************************/
void hr20SetModeAuto()
{
	serialCommand("\rM01\r", 0);
}

/*!
 ********************************************************************************
 * hr20GetStatusLine
 *
 * gets the statusline from openhr20. still can't get a better solution than
 * trying to read the line several times
 *
 * \param *line will return the statusline
 *******************************************************************************/
void hr20GetStatusLine(char *line)
{
	while(1)
	{
		serialCommand("D\r", line);
		if(line[0] == 'D' )
			break;
		usleep(1000);
	}
}

/*!
 ********************************************************************************
 * hr20ParseStatusLine
 *
 * parses the status line you get with "D<CR>" and prints the results to stdout
 *
 * \param *line the line to parse
 *******************************************************************************/
void hr20ParseStatusLine(char *line)
{
	/* example line as we receive it:
	  D: d6 10.01.09 22:19:14 M V: 54 I: 1975 S: 2000 B: 3171 Is: 00b9 X
	 */

	if(!line)
		return;

	char *trenner = (char*)strtok(line," ");
	if(!trenner)
		return;

	if(trenner[0] != 'D')
	{
		printf("This is no status line\n");
		return;
	}
	
	trenner = (char*)strtok(NULL," ");
	if(trenner)
		printf("Weekday: %d\n", trenner[1]-48);

	trenner = (char*)strtok(NULL," ");
	if(trenner)
		printf("Date:    %s\n", trenner);

	trenner = (char*)strtok(NULL," ");
	if(trenner)
		printf("Time:    %s\n", trenner);

	trenner = (char*)strtok(NULL," ");
	if(trenner[0] == 'M')
		printf("Mode:    manual\n");
	else if(trenner[0] == 'A')
		printf("Mode:    auto\n");
	else
		printf("Mode:    unknown\n");

	trenner = (char*)strtok(NULL," ");
	trenner = (char*)strtok(NULL," ");
	if(trenner)
		printf("Valve:   %s%%\n",trenner);

	trenner = (char*)strtok(NULL," ");
	trenner = (char*)strtok(NULL," ");
	if(trenner)
		printf("T is:    %.2f C\n",(float)atoi(trenner)/100);

	trenner = (char*)strtok(NULL," ");
	trenner = (char*)strtok(NULL," ");
	if(trenner)
		printf("T set:   %.2f C\n",(float)atoi(trenner)/100);

	trenner = (char*)strtok(NULL," ");
	trenner = (char*)strtok(NULL," ");
	if(trenner)
		printf("Bat:     %.2f V\n",(float)atoi(trenner)/1000);

	return;
}


