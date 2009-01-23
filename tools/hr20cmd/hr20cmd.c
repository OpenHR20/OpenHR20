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
 * \file	hr20cmd.c
 * \brief	main file for this project. parses the command line and executes corresponding functions
 * \author	Bjoern Biesenbach <bjoern at bjoern-b dot de>
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/signal.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sysexits.h>
#include <getopt.h>

#include "serial.h"
#include "hr20.h"

#define HR20CMD_VERSION "0.2"

#define FLAG_DATETIME 1
#define FLAG_TEMPERATURE 2
#define FLAG_MODE 4
#define FLAG_TIMERS 8
#define FLAG_SET_TIMER 16

static int flags;


static struct option long_options[] = 
{

	{"port", required_argument, 0, 'p'},
	{"set_datetime", no_argument, 0, 'd'},
	{"set_temperature", required_argument, 0, 't'},
	{"set_mode", required_argument, 0, 'm'},
	{"get_timers", no_argument, 0, 'g'},
	{"set_timer", required_argument, 0, 'a'},
	{"help", no_argument, 0, 'h'},
	{0,0,0,0}
};


static void printUsage(void)
{
	printf("hr20cmd version %s\n", HR20CMD_VERSION);
	printf("Options:\n\n");
	printf(" -p, --port                serial port (default /dev/ttyS0)\n");
	printf(" -d, --set_datetime        set current local date and time\n");
	printf(" -t, --set_temperature t   set wanted temperature (19.5°C = 195)\n");
	printf(" -m, --set_mode mode       set mode auto/manu\n");
	printf(" -g, --get_timers          get the whole timing table\n");
	printf(" -a, --set_timer AB[CDDEE] set a specific timer\n");
	printf("                           A day 0-7, B slot, C mode, DDEE time in hours and minutes\n");
	printf("                           Modes: 0 frost protection, 1 energy save, 2 comfort, 3 supercomfort\n");
	printf("                           if only day and slot specified, the slot will be unset\n");
	printf("                           example: 1020700 stands for comfort mode on monday 7:00\n");
	printf(" -h, --help                this help\n\n");
}


int main(int argc, char* argv[])
{
	char serialPort[255];
	int desired_temperature;
	char mode[5];
	char timer_string[10];

	strcpy(serialPort,"/dev/ttyS0");

	int c;
	while(1)
	{
		int option_index = 0;

		c = getopt_long(argc, argv, "p:t:hdm:ga:", long_options, &option_index);

		if( c == -1 )
			break;

		switch(c)
		{
			case 'p': 	strcpy(serialPort, optarg);
					break;

			case 't': 	desired_temperature = atoi(optarg);
					flags |= FLAG_TEMPERATURE;
					break;

			case 'd': 	flags |= FLAG_DATETIME;
					break;

			case 'h': 	printUsage();
					exit(0);
			
			case 'g': 	flags |= FLAG_TIMERS;
					break;

			case 'a': 	flags |= FLAG_SET_TIMER;
					if(strlen(optarg) <=10)
					{
						strcpy(timer_string, optarg);
						timer_string[7] = '\0';
					}
					break;
			case 'm': 	if(strlen(optarg) > 4)
					{
						printf("error in mode\n");
						exit(1);
					}
					strcpy(mode, optarg);
					flags |= FLAG_MODE;
					break;

			default: abort();
		}
	}
	
	if(!initSerial(serialPort))
	{
		printf("Could not open serial device\n");
		exit(EX_NOINPUT);
	}
	

	if(flags & FLAG_DATETIME)
	{
		hr20SetDateAndTime();
	}
	
	if(flags & FLAG_TEMPERATURE)
	{
		if(desired_temperature < 50 ||
			desired_temperature > 300)
		{
			printf("Temperature %d out of range. Must be between 50 (5°C) and 300 (30°C)\n",
				desired_temperature);
		}
		else if(desired_temperature % 5)
			printf("Temperature %d is not a multiple of 5\n", desired_temperature);
		else
			hr20SetTemperature(desired_temperature);
	}
	
	if(flags & FLAG_MODE)
	{
		if(!strcmp(mode,"auto"))
			hr20SetModeAuto();
		else if(!strcmp(mode,"manu"))
			hr20SetModeManu();
		else
			printf("Unknown mode: %s\n",mode);
	}
	if(flags & FLAG_TIMERS)
	{
		hr20GetAllTimers();
	}

	if(flags & FLAG_SET_TIMER)
	{
		if(strlen(timer_string) == 2)
		{
			if((timer_string[0]-48 < 0) || (timer_string[0]-48 > 7))
				printf("Day must be between 0 and 7\n");
			else
				hr20UnsetTimer(timer_string[0]-48, timer_string[1]-48);
		}
		else
			hr20SetTimer(timer_string);
	}
	
	if(!flags)
	{
		char response[255];
		hr20GetStatusLine(response);
		hr20ParseStatusLine(response);
	}

	return 0;

}

