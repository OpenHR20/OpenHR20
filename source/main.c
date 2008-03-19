/*
 *  Open HR20
 *
 *  target:     ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
 *
 *  ompiler:    WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Dario Carluccio (hr20-at-carluccio-dot-de)
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
 * \file       main.c
 * \brief      the main file for Open HR20 project
 * \author     Dario Carluccio <hr20-at-carluccio-dot-de>
 * \date       24.12.2007
 * $Rev: 40 $
 */

// AVR LibC includes 
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/version.h>

// HR20 Project includes
#include "main.h"
#include "adc.h"
#include "lcd.h"
#include "rtc.h"
#include "motor.h"

// global Vars
volatile bool    m_automatic_mode;         // auto mode (false: manu mode)

// global Vars for default values: temperatures and speed
volatile uint8_t m_reftemp;                // actual desired temperatur
volatile uint8_t m_reftemp_mem[TEMP_SLOTS];// desired temperatur memories
                                           // (high, low)
volatile motor_speed_t m_speed;            // motor speed

// global Vars for keypress and wheel status
volatile bool    m_key_action;             // keypress ISR came up, process it
volatile uint8_t m_state_keys;             // state of keys (Bits: see KEYMASK_
volatile uint8_t m_state_keys_prev;
volatile uint8_t m_state_wheel;            // state of wheel sensor
volatile uint8_t m_state_wheel_prev;
volatile uint8_t m_wheel;                  // wheel position (0-0xff)


// prototypes 
int main(void);                            // main with main loop
void init(void);                           // init the whole thing
void load_defauls(void);                   // load default values
                                           // (later from eeprom using config.c)
void callback_settemp(uint8_t);            // called from RTC to set new reftemp
void setautomode(bool);                    // activate/deactivate automode



// Check AVR LibC Version >= 1.6.0
#if __AVR_LIBC_VERSION__ < 10600UL
#warning "avr-libc >= version 1.6.0 recommended"
#warning "This code has not been tested with older versions."
#endif


/*!
 *******************************************************************************
 * main program
 ******************************************************************************/
int main(void)
{
    bool last_state_mnt;        //!< motor mounted
    bool state_mnt;             //!< motor mounted
    bool err;                   //!< error
    bool ref_pos_changed;       //!< ref Position changed

    uint8_t last_statekey;      //!< state of keys on last loop
    uint8_t last_second;        //!< RTC-second of last main cycle
    uint8_t ref_position;       //!< desired position in percent
    motor_speed_t speed;        //!< motor speed (fast or quiet)
    uint8_t display_mode;       //!< desired display output
    uint16_t value16;           //!< 16 Bit value
    uint8_t value8;             //!<  8 Bit value

    //! initalization
    init();

    //! load/set default values
    load_defauls();
    
    //! Enable interrupts
    sei();

    //! show POST Screen
    LCD_AllSegments(LCD_MODE_ON);                   // all segments on
    delay(1000);
    LCD_AllSegments(LCD_MODE_OFF);        
    LCD_PrintDec(REVHIGH, 1, LCD_MODE_ON);          // print version
    LCD_PrintDec(REVLOW, 0, LCD_MODE_ON);
    LCD_Update();
    delay(1000);
    LCD_AllSegments(LCD_MODE_OFF);                  // all off

    //! \todo Send Wakeup MSG

    state_mnt=false;
    ref_pos_changed=true;
    last_second=99;
    speed=full;
    last_statekey = 0;
    last_state_mnt = false;
    m_key_action = true;
    ref_position = 10;

    ISR(PCINT1_vect);                  // get keystate

    /*!
    ****************************************************************************
    * main loop
    *
    * 1) process keypresses
    *    - m_state_keys and m_wheel are set from IRQ, only process them
    *    - you can set m_wheel to a new value
    *    - controll content of LCD
    * 2) \todo calc new valveposition if new temp available
    *    - temp is measured by IRQ
    * 3) calibrate motor
    *    - start calibration is valve mounted changed to on
    *      (during calibration the main loop stops at least for 10 seconds)
    *    - reset calibration is valve mounted changed to off
    * 4) start motor if
    *    - actual valveposition != desired valveposition && motor is off
    * 5) if motor is on call MOTOR_CheckBlocked at least once a second
    *    - that switches motor of if it is blocked
    * 6) store keystate at end of loop before going to sleep
    * 7) \todo goto sleep if
    *    - motor is of
    *    - no key is pressed (AUTO, C, PROG)
    *    - no serial communication active
    ***************************************************************************/
    for (;;){        // change displaystate every 10 seconds 0 to 5
        // Activate Auto Mode
        // setautomode(true);

        if (m_key_action){
            m_key_action = false;
            state_mnt = !(m_state_keys & KEYMASK_MOUNT);

            // State of keys AUTO, C and PROG and valve mounted
            if (m_state_keys & KEYMASK_AUTO){
                display_mode--;
                LCD_SetHourBarVal(display_mode+1, LCD_MODE_ON);
                ref_pos_changed = true;
                LCD_SetSeg(LCD_SEG_AUTO, LCD_MODE_ON);
            } else {
                LCD_SetSeg(LCD_SEG_AUTO, LCD_MODE_OFF);
            }
            if (m_state_keys & KEYMASK_C){
                display_mode=0;
                LCD_SetHourBarVal(display_mode+1, LCD_MODE_ON);                
                ref_pos_changed = true;
                LCD_SetSeg(LCD_SEG_MANU, LCD_MODE_ON);
            } else {
                LCD_SetSeg(LCD_SEG_MANU, LCD_MODE_OFF);
            }
            if (m_state_keys & KEYMASK_PROG){
                display_mode++;
                LCD_SetHourBarVal(display_mode+1, LCD_MODE_ON);
                ref_pos_changed = true;
                LCD_SetSeg(LCD_SEG_PROG, LCD_MODE_ON);
            } else {
                LCD_SetSeg(LCD_SEG_PROG, LCD_MODE_OFF);
            }

            // 2) calc new valveposition if new temp available
            //    - temp is measured by IRQ


            // 3) calibrate motor
            //    - start calibration if valve is now mounted
            //      TODO:
            //      (during calibration the main loop stops for a long time
            //       maybe add global var to cancel callibration, e.g.: if
            //       HR20 removed from ther gear)
            //    - reset calibration is valve mounted changed to off
            if (last_state_mnt != state_mnt) {
                MOTOR_SetMountStatus(state_mnt);
                if (state_mnt) {
                    LCD_ClearNumbers();
                    LCD_PrintChar(LCD_CHAR_C, 3, LCD_MODE_ON);
                    LCD_PrintChar(LCD_CHAR_A, 2, LCD_MODE_ON);
                    LCD_PrintChar(LCD_CHAR_L, 1, LCD_MODE_ON);
                    LCD_Update();
                    // DEBUG: if next line is disabled, not calibration and no
                    //        motor control is done
                    // MOTOR_Calibrate(ref_position, speed);
                    LCD_ClearNumbers();
                } 
            }

            // 4) start motor if
            //    - actual valveposition != desired valveposition
            //    - motor is off
            //    - motor is calibrated
            if ((ref_pos_changed) && (MOTOR_IsCalibrated())){
                err = MOTOR_Goto(ref_position, speed);
                ref_pos_changed = false;
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
        
        if (display_mode==0) {
            ADC_Measure_Ub();
            value16 = ADC_Get_Bat_Val();
        }else if (display_mode==1) {
            ADC_Measure_Temp();
            value16 = ADC_Get_Temp_Val();
        }else if (display_mode==2) {
            ADC_Measure_Ub();            
            value16 = ADC_Get_Bat_Voltage();
        }else if (display_mode==3) {
            ADC_Measure_Temp();
            value16 = ADC_Get_Temp_Deg();
        }else if (display_mode==4) {
            value16 = 567;
        }else{
            value16 = display_mode;                      
        }           
        value8 = (uint8_t) (value16 / 100);
        LCD_PrintDec(value8,  1, LCD_MODE_ON);
        value8 = (uint8_t) (value16 % 100);
        LCD_PrintDec(value8,  0, LCD_MODE_ON);
                
        /*
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
        */
        
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


/*!
 *******************************************************************************
 * Initializate all modules
 ******************************************************************************/
void init(void)
{
    //! Calibrate the internal RC Oszillator
    //! \todo test calibrate_rco();
        
    //! set Clock to 4 Mhz
    CLKPR = (1<<CLKPCE);            // prescaler change enable
    CLKPR = (1<<CLKPS0);            // prescaler = 2 (internal RC runs @ 8MHz)

    //! Disable Analog Comparator (power save)
    ACSR = (1<<ACD);

    //! Disable Digital input on PF0-7 (power save)
    DIDR0 = 0xFF;

    //! digital I/O port direction
    DDRB = (1<<PB4)|(1<<PB7); // PB4, PB7 Motor out
    DDRG = (1<<PG3)|(1<<PG4); // PG3, PG4 Motor out
    DDRE = (1<<PE3);          // PE3  activate lighteye
    DDRF = (1<<PF3);          // PF3  activate tempsensor

    //! enable pullup on all inputs (keys and m_wheel)
    //! ATTENTION: no pullup on lighteye input watch circuit diagram
    PORTB = (1<<PB0)|(1<<PB1)|(1<<PB2)|(1<<PB3)|(1<<PB5)|(1<<PB6);

    //! remark for PCMSK0:
    //!     PCINT0 for lighteye (motor monitor) is activated in motor.c using
    //!     mask register PCMSK0: PCMSK0=(1<<PCINT4) and PCMSK0&=~(1<<PCINT4)

    //! PCMSK1 for keyactions
    PCMSK1 = (1<<PCINT8)|(1<<PCINT9)|(1<<PCINT10)|(1<<PCINT11)|(1<<PCINT13);

    //! activate PCINT0 + PCINT1
    EIMSK = (1<<PCIE1)|(1<<PCIE0);
    
    //! Initialize the USART
    //! \todo USART_Init();

    //1 Initialize the LCD
    LCD_Init();                     

    //! Initialize the RTC, pass pointer to timer callback function
    RTC_Init(callback_settemp);

    //! Initialize the motor
    MOTOR_Init();
}


/*!
 *******************************************************************************
 * Load default values from eeprom
 *
 * \todo EEPROM management, until now values are fix
 ******************************************************************************/
void load_defauls(void)
{
    uint8_t i;

    //! Set Time and date
    RTC_SetDay   (BOOT_DD);
    RTC_SetMonth (BOOT_MM);
    RTC_SetYear  (BOOT_YY);
    RTC_SetHour  (BOOT_hh);
    RTC_SetMinute(BOOT_mm);
    RTC_SetSecond(0);

    //! m_reftemps (high and low)
    m_reftemp_mem[0] = DEG_2_UINT8 (BOOT_TEMP_H);
    m_reftemp_mem[1] = DEG_2_UINT8 (BOOT_TEMP_L);

    //! DOW times
    for (i=0; i<7; i++){
        RTC_DowTimerSet(i, 0, BOOT_ON1);
        RTC_DowTimerSet(i, 1, BOOT_OFF1);
        RTC_DowTimerSet(i, 2, BOOT_ON2);
        RTC_DowTimerSet(i, 3, BOOT_OFF2);
    }

    //! motor speed
    m_speed = full;

    //! initial keystates
    m_key_action = false;
    m_state_keys = 0;
    m_state_keys_prev = 0;
    m_state_wheel = 0;
    m_state_wheel_prev = 0;
}

/*!
 *******************************************************************************
 * set desired m_reftemp from m_reftemp_mem[]
 * is called from RTC
 *
 * \param slot temperatre slot, ID of DOW timer which called this function
 *
 ******************************************************************************/
void callback_settemp(uint8_t slot){
    LCD_SetSeg(LCD_SEG_SNOW, LCD_MODE_ON);
    if (m_automatic_mode){
        m_reftemp = m_reftemp_mem[(slot%2)];
    }
}

/*!
 *******************************************************************************
 * achtivate or deactivate automatic mode
 *
 * \param newmode
 *   true: auto
 *   false: manu
 ******************************************************************************/
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


/*!
 *******************************************************************************
 * Interupt Routine
 *
 *  - check state of buttons
 *  - check state of wheel
 *  - check mount contact
 ******************************************************************************/
ISR(PCINT1_vect){

    // keys
    m_state_keys = ~PINB & 0x6f; // low active, mask for input (PB 0,1,2,3,5,6)

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

/*!
 *******************************************************************************
 * delay function
 *  quick and dirty used only for test purpose
 ******************************************************************************/
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



