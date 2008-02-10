//***************************************************************************
//
//  File........: main.c
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
//***************************************************************************

// AVR LibC includes 
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/version.h>

// HR20 Project includes
#include "main.h"
#include "lcd.h"
#include "rtc.h"
#include "motor.h"

#define  KEYMASK_AUTO      (1<<PB3)       // mask for state_keys: AUTO Button
#define  KEYMASK_PROG      (1<<PB2)       // mask for state_keys: PROG Button
#define  KEYMASK_C         (1<<PB1)       // mask for state_keys:   °C Button
#define  KEYMASK_MOUNT     (1<<PB0)       // mask for state_keys: mounted contact

// global Vars
bool     automatic_mode;                  // Auto mode (false: manu Mode)
volatile uint8_t reftemp;                 // actual desired temperatur
volatile uint8_t reftemp_mem[TEMP_SLOTS]; // desired temperatur memories (high, low)

// global Vars fpr keypress wheel
volatile bool    key_action;              // keypress ISR came up, process it
volatile bool    lighteye_action;         // lighteye ISR came up, process it
volatile bool    state_lighteye;          // state of lighteye
volatile bool    state_lighteye_prev;     // state of lighteye
volatile uint8_t state_keys;              // state of keys (Bits: see KEYMASK_
volatile uint8_t state_keys_prev;
volatile uint8_t state_wheel;             // state of wheel sensor
volatile uint8_t state_wheel_prev;        
volatile uint8_t wheel;                   // wheel position (0-0xff)

// prototypes 
int main(void);
void init(void);                        // init the whole thing
void load_defauls(void);                // Load default values (from eeprom)
void delay(uint16_t);                   // delay
void callback_settemp (uint8_t);        // RTC callback set new reftemp

void test(void);                        // Debugging
void setautomode(bool);

// Check AVR LibC Version >= 1.6.0
#if __AVR_LIBC_VERSION__ < 10600UL
#warning "avr-libc >= version 1.6.0 recommended"
#warning "This code has not been tested with older versions."
#endif


/*****************************************************************************
*
*   Function name:  main
*
*   returns:        None
*
*   parameters:     None
*
*   purpose:        Contains the main loop of the program
*
*****************************************************************************/

int main(void)
{
    uint8_t displaystate;          // State of Display
    uint8_t tmp;                   // 
    uint8_t i;                     //
    uint8_t ii;                    //
    motor_mode_t mode;
    uint8_t value;

    // initalization
    init();

    // load/set default values
    load_defauls();
    
    // Enable interrupts
    sei();

    // POST Screen
    LCD_AllSegments(LCD_MODE_ON);
    delay(2000);
    LCD_AllSegments(LCD_MODE_OFF);
    LCD_SetSeg(LCD_SEG_BAR24, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_BLINK_1);
    LCD_SetSeg(LCD_SEG_COL2, LCD_MODE_BLINK_1);

    // Activate Auto Mode
    // setautomode(true);

    // Send Wakeup MSG
    // TODO

    displaystate = 0;
    i = 0;
    wheel=24;
    
    // Main loop
    for (;;){        // change displaystate every 10 seconds 0 to 5

        if (key_action){
            key_action = false;
            i++;
            // State of keys AUTO, C and PROG
            if (state_keys & KEYMASK_AUTO){
                LCD_SetSeg(LCD_SEG_AUTO, LCD_MODE_OFF);
            } else {
                LCD_SetSeg(LCD_SEG_AUTO, LCD_MODE_ON);
            }
            if (state_keys & KEYMASK_C){
                LCD_SetSeg(LCD_SEG_MANU, LCD_MODE_OFF);
            } else {
                LCD_SetSeg(LCD_SEG_MANU, LCD_MODE_ON);
            }
            if (state_keys & KEYMASK_PROG){
                LCD_SetSeg(LCD_SEG_PROG, LCD_MODE_OFF);
            } else {
                LCD_SetSeg(LCD_SEG_PROG, LCD_MODE_ON);
            }
            
            // LCD_PrintHex(state_keys, 1, LCD_MODE_ON);
            // value controlled by wheel

            LCD_PrintHex(state_keys,  1, LCD_MODE_ON);
            LCD_PrintDec(wheel,        0, LCD_MODE_ON);
            
        }
        // update Display each main loop
        LCD_Update();
    } //End Main loop
    return 0;
}

#if 0
void test(void)
{
    MOTOR_Control((motor_mode_t) i);

    LCD_PrintDec(i, 0, LCD_MODE_ON);
    LCD_PrintHex(i, 1, LCD_MODE_ON);

    // Hour Bar shows displaystate
    LCD_SetHourBarVal(i, LCD_MODE_ON);
    LCD_SetHourBarBar(i, LCD_MODE_ON);
    LCD_SetHourBarSeg(i, LCD_MODE_ON);

    // mm:ss
    LCD_PrintDec(RTC_GetMinute(), 1, LCD_MODE_ON);
    LCD_PrintDec(RTC_GetSecond(), 0, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_COL2, LCD_MODE_ON);

    // hh:mm
    LCD_PrintDec(RTC_GetHour()  , 1, LCD_MODE_ON);
    LCD_PrintDec(RTC_GetMinute(), 0, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_BLINK_1);
    LCD_SetSeg(LCD_SEG_COL2, LCD_MODE_BLINK_1);

    // DD.MM
    LCD_PrintDec(RTC_GetDay()   , 1, LCD_MODE_ON);
    LCD_PrintDec(RTC_GetMonth() , 0, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_COL2, LCD_MODE_OFF);

    // YY YY
    tmp= RTC_GetYearYY();
    LCD_PrintDec( (20+(tmp/100) ), 1, LCD_MODE_ON);
    LCD_PrintDec( (tmp%100)      , 0, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_OFF);
    LCD_SetSeg(LCD_SEG_COL2, LCD_MODE_OFF);

    // Weekday
    LCD_PrintDayOfWeek(RTC_GetDayOfWeek(), LCD_MODE_ON);

    // reftemp
    LCD_PrintTemp(reftemp, LCD_MODE_ON);

    LCD_AllSegments(LCD_MODE_ON);
    LCD_ClearHourBar();
    LCD_ClearSymbols();
    LCD_ClearNumbers();
    LCD_AllSegments(LCD_MODE_OFF);
    LCD_SetSeg(LCD_SEG_BAR24, LCD_MODE_ON);

    LCD_SetHourBarSeg(i, LCD_MODE_OFF);
    LCD_SetHourBarVal(12, LCD_MODE_ON);
    LCD_SetHourBarBat(12, LCD_MODE_ON);
    
    LCD_SetSeg(LCD_SEG_AUTO, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_MANU, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_PROG, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_COL2, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_SUN, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_MOON, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_SNOW, LCD_MODE_ON);

    
    LCD_PrintTemp (i, LCD_MODE_ON);

    // Check 1.1. of each year
    RTC_SetDay(1);
    RTC_SetMonth(1);                    // Set Month
    for (i=0; i<14; i++){
        RTC_SetYear(i);
        if (i==11){RTC_SetYear(50);}
        if (i==12){RTC_SetYear(100);}
        if (i==13){RTC_SetYear(150);}
        tmp = RTC_GetYearYY();
        LCD_PrintDec( (20+(tmp/100) ), 1, LCD_MODE_ON);
        LCD_PrintDec( (tmp%100)      , 0, LCD_MODE_ON);
        LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_OFF);
        LCD_SetSeg(LCD_SEG_COL2, LCD_MODE_OFF);
        LCD_SetHourBarSeg(i, LCD_MODE_ON);
        LCD_Update();
        delay(1000);
        LCD_PrintDayOfWeek(RTC_GetDayOfWeek(), LCD_MODE_ON);
        LCD_Update();
        delay(3000);
    }

    // Check 1.1. of each month in 2008
    RTC_SetDay(1);
    RTC_SetYear(8);
    for (i=1; i<13; i++){
        RTC_SetMonth(i);
        LCD_PrintDec(RTC_GetDay()   , 1, LCD_MODE_ON);
        LCD_PrintDec(RTC_GetMonth() , 0, LCD_MODE_ON);
        LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_ON);
        LCD_SetSeg(LCD_SEG_COL2, LCD_MODE_OFF);
        LCD_Update();
        delay(1000);
        LCD_PrintDayOfWeek(RTC_GetDayOfWeek(), LCD_MODE_ON);
        LCD_Update();
        delay(3000);
    }

}
#endif


/*****************************************************************************
*
*   Function name:  init
*
*   returns:        None
*
*   parameters:     None
*
*   purpose:        Initializate the different modules
*
*****************************************************************************/
void init(void)
{
    // Calibrate the internal RC Oszillator
    // TODO: test calibrate_rco();
        
    // set Clock to 4 Mhz
    CLKPR = (1<<CLKPCE);            // prescaler change enable
    CLKPR = (1<<CLKPS0);            // prescaler = 2 (internal RC runs @ 8MHz)

    // Disable Analog Comparator (power save)
    ACSR = (1<<ACD);

    // Disable Digital input on PF0-7 (power save)
    DIDR0 = 0xFF;

    // digital I/O port direction
    DDRB = (1<<PB4)|(1<<PB7);
    DDRG = (1<<PG3)|(1<<PG4);
    DDRE = (1<<PE3);

    // enable pullup on all inputs
    PORTB = (1<<PB0)|(1<<PB1)|(1<<PB2)|(1<<PB3)|(1<<PB5)|(1<<PB6);  // keys and wheel
    PORTE = (1<<PE4);                                               // light eye 

    // INT1 Mask
    EIMSK = (1<<PCIE1)|(1<<PCIE0);
    // EIMSK = (1<<PCIE1);
    PCMSK1 = (1<<PCINT8)|(1<<PCINT9)|(1<<PCINT10)|(1<<PCINT11)|(1<<PCINT13);
    PCMSK0 = (1<<PCINT4);

    // Initialize the USART
    // TODO: USART_Init();

    // Initialize the LCD
    LCD_Init();                     

    // Initialize the RTC, callback funcrion is settemp(rtc_timerslot_t)
    RTC_Init(callback_settemp);

    // Initialize the motor
    MOTOR_Init();
}


/*****************************************************************************
*
*   Function name:  load_defauls
*
*   returns:        None
*
*   parameters:     None
*
*   purpose:        Load default values from eeprom 
*
*   remark:         TODO: EEPROM management, untin now values are fix
*
*****************************************************************************/
void load_defauls(void){

    uint8_t i;

    // Set Time and date
    RTC_SetDay   (BOOT_DD);
    RTC_SetMonth (BOOT_MM);
    RTC_SetYear  (BOOT_YY);
    RTC_SetHour  (BOOT_hh);
    RTC_SetMinute(BOOT_mm);
    RTC_SetSecond(0);

    // reftemps (high and low)
    reftemp_mem[0] = DEG_2_UINT8 (BOOT_TEMP_H);
    reftemp_mem[1] = DEG_2_UINT8 (BOOT_TEMP_L);

    // DOW times
    for (i=0; i<7; i++){
        RTC_DowTimerSet(i, 0, BOOT_ON1);
        RTC_DowTimerSet(i, 1, BOOT_OFF1);
        RTC_DowTimerSet(i, 2, BOOT_ON2);
        RTC_DowTimerSet(i, 3, BOOT_OFF2);
    }

    // initial keystates
    key_action = false;
    lighteye_action = false;
    state_lighteye = false;
    state_lighteye_prev = false;
    state_keys = 0;
    state_keys_prev = 0;
    state_wheel = 0;
    state_wheel_prev = 0;
}

/*****************************************************************************
*
*   Function name:  settemp
*
*   returns:        None
*
*   parameters:     temperatre slot
*
*   global vars:    reftemp, reftemp_mem[]
*
*   purpose:        set desired reftemp from reftemp_mem[]
*                   is called from RTC
*
*****************************************************************************/
void callback_settemp(uint8_t slot){
    LCD_SetSeg(LCD_SEG_SNOW, LCD_MODE_ON);
    if (automatic_mode){
        reftemp = reftemp_mem[(slot%2)];
    }
}

/*****************************************************************************
*
*   Function name:  setautomode
*
*   returns:        None
*
*   parameters:     true: auto, false: manu
*
*   global vars:    reftemp
*                   automatic_mode
*
*   purpose:        achtivate or deactivate automatic mode
*
*****************************************************************************/
void setautomode(bool newmode){
    if (newmode){
        // set reftemp according to last occured timer index MOD 2
        // as timers 0, 2, .. are timers for high temp, 1, 3, .. for low temp.
        reftemp = reftemp_mem[(RTC_DowTimerGetActualIndex()%2)];
        LCD_SetSeg(LCD_SEG_AUTO, LCD_MODE_ON);
        LCD_SetSeg(LCD_SEG_MANU, LCD_MODE_OFF);
    } else {
        LCD_SetSeg(LCD_SEG_AUTO, LCD_MODE_OFF);
        LCD_SetSeg(LCD_SEG_MANU, LCD_MODE_ON);
    }
    automatic_mode = newmode;
}


/*****************************************************************************
*
*   Function name:  ISR(PCINT0_vect)
*
*   returns:        None
*
*   parameters:     Pinchange Interupt INT0
*
*   purpose:        process light eye sensor
*
*   global vars:    lighteye_action
*                   state_lighteye
*                   state_lighteye_prev
*
*****************************************************************************/
ISR (PCINT0_vect){
    // status of lighteye
    state_lighteye = ((PINE & (1<<PE4)) != 0);
    if (state_lighteye_prev != state_lighteye) {
        state_lighteye_prev = state_lighteye;
        lighteye_action = true;
    }
}

/*****************************************************************************
*
*   Function name:  ISR(PCINT1_vect)
*
*   returns:        none
*
*   parameters:     Pinchange Interupt INT1
*
*   purpose:        process buttons, wheel and mount contact
*
*   global vars:    key_action
*                   state_keys
*                   state_keys_prev
*                   state_wheel
*                   state_wheel_prev
*                   wheel
*
*****************************************************************************/
ISR(PCINT1_vect){

    // keys
    state_keys = PINB & 0x6f; // mask for input keys (PB0,1,2,3,5,6,)

    if (state_keys_prev != state_keys){
        state_keys_prev = state_keys;

        // wheel
        state_wheel = ( state_keys & (0x60) );   // wheel = 0x00,0x20,0x40,0x60
        if (state_wheel !=state_wheel_prev){
            state_wheel_prev = state_wheel;
            if ( (state_wheel == 0x20) || (state_wheel == 0x40) ) {
                wheel++;
            } else if ( (state_wheel == 0x00) || (state_wheel == 0x60) ){
                wheel--;
            }
        }

        // set flag for main
        key_action = true;
    }
}


/*****************************************************************************
*
*   Function name:  delay
*
*   returns:        None
*
*   parameters:     millisec
*
*   purpose:        delay-loop
*
*****************************************************************************/
void delay(uint16_t millisec)
{
	uint8_t i;
    while (millisec--){
		for (i=0; i<255; i++){
			asm volatile ("nop"::);
		}
		for (i=0; i<255; i++){
			asm volatile ("nop"::);
		}
		for (i=0; i<255; i++){
			asm volatile ("nop"::);
		}
	}
}

