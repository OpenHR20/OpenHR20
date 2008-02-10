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
#define	MOTOR_MAX_IMPULSES 1000

#define MOTOR_QUIET_PWM     70     // duty cycle of PWM in quiet mode
#define MOTOR_FULL_PWM      95     // duty cycle of PWM in normal mode


/*****************************************************************************
*   Typedefs
*****************************************************************************/
typedef enum {                                      // motormode
    stop, open, close, open_quiet, close_quiet
} motor_mode_t;


/*****************************************************************************
*   Prototypes
*****************************************************************************/

void    MOTOR_Init(void);                      // Init motor control
void    MOTOR_Calibrate(void);                 // calibrate the motor (find start end endposition

void    MOTOR_SetPosition(void);               // set position of the motor (0-100%)
uint8_t MOTOR_GetPosition(void);               // get position of the motor (0-100%)

void    MOTOR_PhotoeyeImpulse(void);           // one photoeye impulse occured

void MOTOR_Stop(void);
void MOTOR_Control(motor_mode_t mode);




