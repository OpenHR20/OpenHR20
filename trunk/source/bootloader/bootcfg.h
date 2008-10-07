/*

                           e Y8b    Y8b YV3.58P888 88e
                          d8b Y8b    Y8b Y888P 888 888D
                         d888b Y8b    Y8b Y8P  888 88"
                        d8WuHan888b    Y8b Y   888 b,
                       d8888888b Y8b    Y8P    888 88b,
           8888 8888       ,e,                                  888
           8888 888820088e  " Y8b Y888P ,e e, 888,8, dP"Y ,"Y88b888
           8888 8888888 88b888 Y8b Y8P d88 88b888 " C88b "8" 888888
           8888 8888888 888888  Y8b "  888   ,888    Y88D,ee 888888
           'Y88 88P'888 888888   Y8P    "YeeP"888   d,dP "88 888888
   888 88b,                    d8  888                     888
   888 88P' e88 88e  e88 88e  d88  888 e88 88e  ,"Y88b e88 888 ,e e, 888,8,
   888 8K  d888 888bd888 8Shaoziyang88d888 888b"8" 888d888 888d88 88b888 "
   888 88b,Y888 888PY888 888P 888  888Y888 888P,ee 888Y888 888888   ,888
   888 88P' "88 88"  "88 88"  888  888 "88 88" "88 888 "88 888 "YeeP"888


  Project:       AVR Universal BootLoader
  File:          bootcfg.h
                 user's configuration.
  Version:       3.5

  Compiler:      GCC 4.2.2 + AVR Studio 4.13sp2 b571

  Author:        Shaoziyang
                 Shaoziyang@gmail.com
                 http://avrubd.googlepages.com
                 
  Date:          2007.6
  Made by:       AVRUBD autocode, 01.04.2008 12:30:42

  Modify:        Add your modify log here

  See readme.txt to get more information.

*/

#ifndef _BOOTCFG_H_
#define _BOOTCFG_H_        1

//define uart buffer's length
#define BUFFERSIZE         128

//system clock(Hz)
#ifndef F_CPU
#define F_CPU              8000000UL
#endif

//baudrate
#define BAUDRATE           9600

//Boot section start address(byte)
//define BootStart to 0 will disable this function
#define BootStart          0x1E00 * 2
//#define BootStart          0x1C00 * 2

//verify flash's data while write
//ChipCheck will only take effect while BootStart enable also
#define ChipCheck          0

//Bootloader launch  0:comport password  1:port level
#define LEVELMODE          0

#define LEVELPORT          D
#define LEVELPIN           PD7
//port level  1:High  0:Low
#define PINLEVEL           0

//max wait password time = TimeOutCnt * timeclk
//timeout count
#define TimeOutCnt         15

//basic timer interval(ms)
#define timeclk            200

//max wait data time = TimeOutCntC * timeclk
//send 'C' command count
#define TimeOutCntC        100

//password length
#define CONNECTCNT         1

//password
#if LEVELMODE == 0
unsigned char KEY[] = {0x64};
#endif

//comport number: 0/1/2..
#define COMPORTNo          0

//enable watchdog
#define WDGEn              0

//enable RS485/RS422 mode
#define RS485              0
//RS485/RS422 send control port
#define RS485PORT          D
#define RS485TXEn          PD5

//enable LED indication
#define LEDEn              0
//LED control port
#define LEDPORT            D
#define LEDPORTNo          PD6

//some kind of AVR need special delay after comport initialization
#define InitDelay          0

//communication checksum method   0:CRC16  1:add up
#define CRCMODE            0

//Verbose mode: display more prompt message
#define VERBOSE            0

#endif

//End of file: bootcfg.h
