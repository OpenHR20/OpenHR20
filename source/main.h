//*****************************************************************************
//
//  File........: main.h
//
//  Author(s)...:
//
//  Target(s)...: ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
//
//  Compiler....: WinAVR-20071221
//                avr-libc 1.6.0
//                GCC 4.2.2
//
//  Description.: main HR20 thermo application
//
//  Revisions...: 
//
//  YYYYMMDD - VER. - COMMENT                                     - SIGN.
//
//  20072412   0.0    created                                     - D.Carluccio
//
//*****************************************************************************

// Revisions number
#define REVHIGH  3
#define REVLOW   1
#define REVSVN   5

// HR20 runs on 4MHz
#ifndef F_CPU
#define F_CPU 4000000UL
#endif

// Bitmask fpr PortB where Keys are connected
#define  KEYMASK_AUTO   (1<<PB3)       // mask for: AUTO Button
#define  KEYMASK_PROG   (1<<PB2)       // mask for: PROG Button
#define  KEYMASK_C      (1<<PB1)       // mask for:   °C Button
#define  KEYMASK_MOUNT  (1<<PB0)       // mask for: mounted contact

// temperature slots: 2 different Temperatures can switched by DOW-Timer
#define TEMP_SLOTS 2     // high, low

// Boot Up Time and Date -> move to CONFIG.H
#define BOOT_DD         7
#define BOOT_MM         2
#define BOOT_YY         8
#define BOOT_hh        22
#define BOOT_mm        29

// Boot Timeslots -> move to CONFIG.H
#define BOOT_ON1      BOOT_hh*10 + 1  // 10 Minutes after BOOT_hh:00
#define BOOT_OFF1     BOOT_ON1 + 1
#define BOOT_ON2      BOOT_ON1 + 2
#define BOOT_OFF2     BOOT_ON1 + 3

// Boot Temperatures -> move to CONFIG.H
#define BOOT_TEMP_H   22 // 22°C
#define BOOT_TEMP_L   16 // 22°C

// public prototypes
void delay(uint16_t);                   // delay

// typedefs
typedef enum { false, true } bool;


// functional defines
#define DEG_2_UINT8(deg)    (deg*10)-49 ;         // convert degree to uint8_t value
//#define SETBIT(port,bit)  (port |= (1<<bit))      // set bit in port
//#define RSTBIT(port,bit)  (port &= ~(1<<bit))     // reset bit in port

