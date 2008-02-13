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

// global Vars
volatile bool    m_automatic_mode;          // auto mode (false: manu mode)

// global Vars for default values: temperatures and speed
volatile uint8_t m_reftemp;                 // actual desired temperatur
volatile uint8_t m_reftemp_mem[TEMP_SLOTS]; // desired temperatur memories (high, low)
volatile motor_speed_t m_speed;             // motor speed

// global Vars for keypress and wheel status
volatile bool    m_key_action;              // keypress ISR came up, process it
volatile uint8_t m_state_keys;              // state of keys (Bits: see KEYMASK_
volatile uint8_t m_state_keys_prev;
volatile uint8_t m_state_wheel;             // state of wheel sensor
volatile uint8_t m_state_wheel_prev;
volatile uint8_t m_wheel;                   // wheel position (0-0xff)


// prototypes 
int main(void);                           // main with main loop
void init(void);                          // init the whole thing
void load_defauls(void);                  // load default values (later from eeprom using config.c)
void callback_settemp(uint8_t);           // called from RTC to set new reftemp
void setautomode(bool);                   // activate/deactivate automode 



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
    bool last_state_mnt;        // motor mounted
    bool state_mnt;             // motor mounted
    bool err;                   // error
    
    uint8_t last_statekey;      // state of keys on last loop
    uint8_t last_second;        // RTC-second of last main cycle
    uint8_t ref_position;       // desired position in percent
    motor_speed_t speed;        // motor speed (fast or quiet)
    
    uint8_t i;           

    // initalization
    init();

    // load/set default values
    load_defauls();
    
    // Enable interrupts
    sei();

    // POST Screen
    LCD_AllSegments(LCD_MODE_ON);                   // all segments on
    delay(1000);
    LCD_AllSegments(LCD_MODE_OFF);        
    LCD_PrintDec(REVHIGH, 1, LCD_MODE_ON);          // print version
    LCD_PrintDec(REVLOW, 0, LCD_MODE_ON);
    LCD_Update();
    delay(1000);
    LCD_AllSegments(LCD_MODE_OFF);                  // all off

    // Send Wakeup MSG
    // TODO

    speed=full;
    last_statekey = 0;
    ref_position = 90;

    last_state_mnt = false ;      // motor not mounted

    ISR(PCINT1_vect);
    //m_state_keys = ~PINB & 0x6f; // low active, mask for input keys (PB0,1,2,3,5,6,)
    //m_key_action=true

    // Main loop
    // 1) process keypresses
    //    - m_state_keys and m_wheel are set from IRQ, only process them
    //    - you can set m_wheel to a new value
    //    - controll content of LCD
    // 2) calc new valveposition if new temp available
    //    - temp is measured by IRQ
    // 3) calibrate motor
    //    - start calibration is valve mounted changed to on
    //      (during calibration the main loop stops at least for 10 seconds)
    //    - reset calibration is valve mounted changed to off
    // 4) start motor if
    //    - actual valveposition != desired valveposition && motor is off
    // 5) if motor is on call MOTOR_CheckBlocked at least once a second
    //    - that switches motor of if it is blocked
    // 6) store keystate at end of loop before going to sleep
    // 7) goto sleep if
    //    - motor is of
    //    - no key is pressed (AUTO, C, PROG)
    //    - no serial communication active
    for (;;){        // change displaystate every 10 seconds 0 to 5
        // Activate Auto Mode
        // setautomode(true);

        if (m_key_action){
            m_key_action = false;
            state_mnt = !(m_state_keys & KEYMASK_MOUNT);

            // State of keys AUTO, C and PROG and valve mounted
            if (m_state_keys & KEYMASK_AUTO){
                ref_position = 10;
                LCD_SetSeg(LCD_SEG_AUTO, LCD_MODE_ON);
            } else {
                LCD_SetSeg(LCD_SEG_AUTO, LCD_MODE_OFF);
            }
            if (m_state_keys & KEYMASK_C){
                ref_position = 50;
                LCD_SetSeg(LCD_SEG_MANU, LCD_MODE_ON);
            } else {
                LCD_SetSeg(LCD_SEG_MANU, LCD_MODE_OFF);
            }
            if (m_state_keys & KEYMASK_PROG){
                ref_position = 90;
                LCD_SetSeg(LCD_SEG_PROG, LCD_MODE_ON);
            } else {
                LCD_SetSeg(LCD_SEG_PROG, LCD_MODE_OFF);
            }

            // 2) calc new valveposition if new temp available
            //    - temp is measured by IRQ


            // 3) calibrate motor
            //    - start calibration is valve mounted changed to on
            //      (during calibration the main loop stops at least for 10 seconds)
            //    - reset calibration is valve mounted changed to off
            if (last_state_mnt != state_mnt) {
                if (state_mnt) {
                    LCD_PrintChar(LCD_CHAR_C, 3, LCD_MODE_ON);
                    LCD_PrintChar(LCD_CHAR_A, 2, LCD_MODE_ON);
                    LCD_PrintChar(LCD_CHAR_L, 1, LCD_MODE_ON);
                    LCD_Update();
                    MOTOR_Calibrate(90, speed);
                    LCD_ClearNumbers();
                } else {
                    MOTOR_ResetCalibration();
                    LCD_PrintChar(LCD_CHAR_S, 0, LCD_MODE_ON);
                }
            }

            // 4) start motor if
            //    - actual valveposition != desired valveposition
            //    - motor is off
            //    - motor is calibrated
            
            if ((!MOTOR_On()) && (MOTOR_IsCalibrated())){
                if (ref_position != MOTOR_GetPosPercent()){
                    err = MOTOR_Goto(ref_position, speed);
                }
            }

            // 5) if motor is on call MOTOR_CheckBlocked at least once a second
            //    - that switches motor of if it is blocked

            if (MOTOR_On()){
                if (last_second != RTC_GetSecond()){
                    MOTOR_CheckBlocked();
                    last_second = RTC_GetSecond();
                }
            }
            
            
            // 6) store keystate at end of loop before going to sleep
            last_statekey = m_state_keys;
            last_state_mnt = state_mnt;
            // 7) goto sleep if
            //    - motor is of
            //    - no key is pressed (AUTO, C, PROG)
            //    - no serial communication active
            

        }
        // Bar 24 on if calibrated
        if (MOTOR_IsCalibrated()) {
            LCD_SetSeg(LCD_SEG_BAR24, LCD_MODE_ON);
        } else {
            LCD_SetSeg(LCD_SEG_BAR24, LCD_MODE_OFF);
        }

        // Hour 0 on if state_mnt
        if (state_mnt) {
            LCD_SetSeg(LCD_SEG_B0, LCD_MODE_ON);
        } else {
            LCD_SetSeg(LCD_SEG_B0, LCD_MODE_OFF);
        }
        
        // Hour 1 on if last_state_mnt
        if (last_state_mnt) {
            LCD_SetSeg(LCD_SEG_B1, LCD_MODE_ON);
        } else {
            LCD_SetSeg(LCD_SEG_B1, LCD_MODE_OFF);
        }

        if (!MOTOR_IsCalibrated()) {
            LCD_PrintChar(LCD_CHAR_E, 3, LCD_MODE_ON);
            LCD_PrintChar(LCD_CHAR_2, 2, LCD_MODE_ON);
        } else {
            LCD_PrintDec(ref_position,  1, LCD_MODE_ON);
            LCD_PrintDec(MOTOR_GetPosPercent(),  0, LCD_MODE_ON);
        }
        
        // impulses = MOTOR_GetImpulses();

        // update Display each main loop
        LCD_Update();
    } //End Main loop
    return 0;
}

// Collection of functions which can be used from here
#if 0

    // ************
    // *** KEYS ***
    // ************
    // m_state_keys stores values of keys
    LCD_PrintHex(m_state_keys, 1, LCD_MODE_ON);
    // m_wheel position 
    LCD_PrintHex(m_wheel, 1, LCD_MODE_ON);

    // *************
    // *** MOTOR ***
    // *************
    // goto position
    MOTOR_Goto(100, full);
    // calibrate motor and goto position
    MOTOR_Calibrate(100, fast);
    // shal not be used from here, for test only
    MOTOR_Control(stop, full);
    MOTOR_Control(open, full);
    MOTOR_Control(close, full);
    MOTOR_Control(open, quiet);
    MOTOR_Control(close, quiet);

    // ***********
    // *** LCD ***
    // ***********
    LCD_Update();
    // print DEC and HEX values
    LCD_PrintDec(i, 0, LCD_MODE_ON);
    LCD_PrintHex(i, 1, LCD_MODE_ON);
    // m_reftemp
    LCD_PrintTemp(m_reftemp, LCD_MODE_ON);
    LCD_PrintTemp (i, LCD_MODE_ON);
    // Hour Bar shows displaystate
    LCD_SetHourBarVal(i, LCD_MODE_ON);
    LCD_SetHourBarBar(i, LCD_MODE_ON);
    LCD_SetHourBarSeg(i, LCD_MODE_ON);
    // mm:ss
    LCD_PrintDec(RTC_GetMinute(), 1, LCD_MODE_ON);
    LCD_PrintDec(RTC_GetSecond(), 0, LCD_MODE_ON);
    // hh:mm
    LCD_PrintDec(RTC_GetHour()  , 1, LCD_MODE_ON);
    LCD_PrintDec(RTC_GetMinute(), 0, LCD_MODE_ON);
    // DD.MM
    LCD_PrintDec(RTC_GetDay()   , 1, LCD_MODE_ON);
    LCD_PrintDec(RTC_GetMonth() , 0, LCD_MODE_ON);
    // YY YY
    tmp= RTC_GetYearYY();
    LCD_PrintDec( (20+(tmp/100) ), 1, LCD_MODE_ON);
    LCD_PrintDec( (tmp%100)      , 0, LCD_MODE_ON);
    // Weekday
    LCD_PrintDayOfWeek(RTC_GetDayOfWeek(), LCD_MODE_ON);
    // 24 hour bar 
    LCD_SetSeg(LCD_SEG_BAR24, LCD_MODE_ON);
    LCD_SetHourBarSeg(i, LCD_MODE_OFF);
    LCD_SetHourBarVal(12, LCD_MODE_ON);
    LCD_SetHourBarBat(12, LCD_MODE_ON);
    // column
    LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_COL2, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_BLINK_1);
    LCD_SetSeg(LCD_SEG_COL2, LCD_MODE_BLINK_2);
    // symbols
    LCD_SetSeg(LCD_SEG_AUTO, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_MANU, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_PROG, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_COL1, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_COL2, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_SUN, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_MOON, LCD_MODE_ON);
    LCD_SetSeg(LCD_SEG_SNOW, LCD_MODE_ON);
    // control more segments at once
    LCD_AllSegments(LCD_MODE_ON);
    LCD_ClearHourBar();
    LCD_ClearSymbols();
    LCD_ClearNumbers();
    LCD_AllSegments(LCD_MODE_OFF);

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
    // ATTENTION: no pullup on lighteye input watch circuit diagram
    PORTB = (1<<PB0)|(1<<PB1)|(1<<PB2)|(1<<PB3)|(1<<PB5)|(1<<PB6);  // keys and m_wheel

    // PCINT0_vect for lighteye (motor monitor) is activated in motor.c if needed
    // PCMSK0 = (1<<PCINT4);
    // PCINT1_vect for keyactions
    PCMSK1 = (1<<PCINT8)|(1<<PCINT9)|(1<<PCINT10)|(1<<PCINT11)|(1<<PCINT13);
    // activate PCINT0 + PCINT1
    EIMSK = (1<<PCIE1)|(1<<PCIE0);

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
void load_defauls(void)
{
    uint8_t i;

    // Set Time and date
    RTC_SetDay   (BOOT_DD);
    RTC_SetMonth (BOOT_MM);
    RTC_SetYear  (BOOT_YY);
    RTC_SetHour  (BOOT_hh);
    RTC_SetMinute(BOOT_mm);
    RTC_SetSecond(0);

    // m_reftemps (high and low)
    m_reftemp_mem[0] = DEG_2_UINT8 (BOOT_TEMP_H);
    m_reftemp_mem[1] = DEG_2_UINT8 (BOOT_TEMP_L);

    // DOW times
    for (i=0; i<7; i++){
        RTC_DowTimerSet(i, 0, BOOT_ON1);
        RTC_DowTimerSet(i, 1, BOOT_OFF1);
        RTC_DowTimerSet(i, 2, BOOT_ON2);
        RTC_DowTimerSet(i, 3, BOOT_OFF2);
    }

    // motor speed
    m_speed = full;

    // initial keystates
    m_key_action = false;
    m_state_keys = 0;
    m_state_keys_prev = 0;
    m_state_wheel = 0;
    m_state_wheel_prev = 0;
}

/*****************************************************************************
*
*   Function name:  settemp
*
*   returns:        None
*
*   parameters:     temperatre slot
*
*   global vars:    m_reftemp, m_reftemp_mem[]
*
*   purpose:        set desired m_reftemp from m_reftemp_mem[]
*                   is called from RTC
*
*****************************************************************************/
void callback_settemp(uint8_t slot){
    LCD_SetSeg(LCD_SEG_SNOW, LCD_MODE_ON);
    if (m_automatic_mode){
        m_reftemp = m_reftemp_mem[(slot%2)];
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
*   global vars:    m_reftemp
*                   m_automatic_mode
*
*   purpose:        achtivate or deactivate automatic mode
*
*****************************************************************************/
void setautomode(bool newmode){
    if (newmode){
        // set m_reftemp according to last occured timer index MOD 2
        // as timers 0, 2, .. are timers for high temp, 1, 3, .. for low temp.
        m_reftemp = m_reftemp_mem[(RTC_DowTimerGetActualIndex()%2)];
        LCD_SetSeg(LCD_SEG_AUTO, LCD_MODE_ON);
        LCD_SetSeg(LCD_SEG_MANU, LCD_MODE_OFF);
    } else {
        LCD_SetSeg(LCD_SEG_AUTO, LCD_MODE_OFF);
        LCD_SetSeg(LCD_SEG_MANU, LCD_MODE_ON);
    }
    m_automatic_mode = newmode;
}


/*****************************************************************************
*
*   Function name:  ISR(PCINT1_vect)
*
*   returns:        none
*
*   parameters:     Pinchange Interupt INT1
*
*   purpose:        process buttons, m_wheel and mount contact
*
*   global vars:    m_key_action
*                   m_state_keys
*                   m_state_keys_prev
*                   m_state_wheel
*                   m_state_wheel_prev
*                   m_wheel
*
*****************************************************************************/
ISR(PCINT1_vect){

    // keys
    m_state_keys = ~PINB & 0x6f; // low active, mask for input keys (PB0,1,2,3,5,6,)

    if (m_state_keys_prev != m_state_keys){
        m_state_keys_prev = m_state_keys;

        // m_wheel
        m_state_wheel = ( m_state_keys & (0x60) );   // m_wheel on PB5 and PB6
        if (m_state_wheel !=m_state_wheel_prev){
            m_state_wheel_prev = m_state_wheel;
            if ( (m_state_wheel == 0x20) || (m_state_wheel == 0x40) ) {
                m_wheel++;
            } else if ( (m_state_wheel == 0x00) || (m_state_wheel == 0x60) ){
                m_wheel--;
            }
        }

        // set flag for main
        m_key_action = true;
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



