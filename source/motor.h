//*****************************************************************************
//
//  File........: motor.h
//
//  Author(s)...:
//
//  Target(s)...: ATmega169 @ 4 MHz in Honnywell Rondostat HR20E
//
//  Compiler....: WinAVR-20071221
//                avr-libc 1.6.0
//                GCC 4.2.2
//
//  Description.: Functions used to control the motor
//
//  Revisions...: 0.1
//
//  YYYYMMDD - VER. - COMMENT                                     - SIGN.
//
//  20080207   0.0    created                                     - D.Carluccio
//
//*****************************************************************************

/*****************************************************************************
*   Macros
*****************************************************************************/

// How is the H-Bridge connected to the AVR?
#define MOTOR_HR20_PB4     PB4     // if PWM output != OC0A you have a problem
#define MOTOR_HR20_PB4_P   PORTB   
#define MOTOR_HR20_PB7     PB7     // change this for other hardware e.g. HR40
#define MOTOR_HR20_PB7_P   PORTB
#define MOTOR_HR20_PG3     PG3     // change this for other hardware e.g. HR40
#define MOTOR_HR20_PG3_P   PORTG
#define MOTOR_HR20_PG4     PG4     // change this for other hardware e.g. HR40
#define MOTOR_HR20_PG4_P   PORTG

#define MOTOR_HR20_PE3     PE3     // HR20: activate photo eye
#define MOTOR_HR20_PE3_P   PORTE

#define MOTOR_HR20_PE_IN   PE4     // HR20: activate photo eye
#define MOTOR_HR20_PE_IN_P PINE

// How many photoeye impulses maximal form one endposition to the other
// The value measured on a HR20 are 820 to 900 = so more than 1000 should no occure
#define	MOTOR_MAX_IMPULSES 1000
#define	MOTOR_HYSTERESIS     50    // additional impulses for 0 or 100%

#define MOTOR_QUIET_PWM     70     // duty cycle of PWM in quiet mode
#define MOTOR_FULL_PWM      95     // duty cycle of PWM in normal mode


/*****************************************************************************
*   Typedefs
*****************************************************************************/
typedef enum {                                      // motor direction
    stop, open, close
} motor_dir_t;

typedef enum {                                      // motorSpeed
    full, quiet
} motor_speed_t;


/*****************************************************************************
*   Prototypes
*****************************************************************************/
void MOTOR_Init(void);                        // Init motor control
void MOTOR_Stop(void);                        // stop motor
bool MOTOR_Goto(uint8_t, motor_speed_t);      // Goto position in percent
void MOTOR_CheckBlocked(void);                // stop if motor is blocked
bool MOTOR_On(void);                          // is motor on

bool MOTOR_Calibrate(uint8_t, motor_speed_t); // calibrate the motor and goto position in percent
bool MOTOR_IsCalibrated(void);                // tre if successful calibrated
void MOTOR_ResetCalibration(void);            // reset the calibration data

uint8_t  MOTOR_GetPosPercent(void);           // get position of the motor (0-100%)
uint16_t MOTOR_GetPosAbs(void);               // get absolute position of the motor

void MOTOR_Control(motor_dir_t, motor_speed_t);


void MOTOR_test(uint8_t);
