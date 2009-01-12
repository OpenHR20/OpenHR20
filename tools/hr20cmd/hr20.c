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

#include "serial.h"
#include "hr20.h"

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

	sprintf(buffer,"A%x\r", temperature/5);

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

	sprintf(send_string,"Y%02x%02x%02x\r",ptm->tm_year-100, ptm->tm_mon+1, ptm->tm_mday);
	serialCommand(send_string, 0);
	
	sprintf(send_string,"H%02x%02x%02x\r",ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
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
	serialCommand("M00\r", 0);
}

/*!
 ********************************************************************************
 * hr20SetModeAuto
 *
 * set the mode to automatic control
 *******************************************************************************/
void hr20SetModeAuto()
{
	serialCommand("M01\r", 0);
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
	int i;
	for(i=0; i < 5; i++)
	{
		serialCommand("D\r", line);
		if(line[0] == 'D' )
			break;
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


