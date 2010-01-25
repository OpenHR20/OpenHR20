#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define BUG 1

void RTC_Init(void)
{
        TIMSK2 &= ~(1<<TOIE2);              // disable OCIE2A and TOIE2
        ASSR = (1<<AS2);                    // Timer2 asynchronous operation
        TCNT2 = 0;                          // clear TCNT2A
        TCCR2A = ((1<<CS22) | (1<<CS20));   // select precaler: 32.768 kHz / 128 =
                                            // => 1 sec between each overflow
    	
    	// wait for TCN2UB and TCR2UB to be cleared
        while((ASSR & (_BV(TCN2UB)|_BV(TCR2UB))) != 0);   
    
        TIFR2 = 0xFF;                       // clear interrupt-flags
        TIMSK2 |= (1<<TOIE2);               // enable Timer2 overflow interrupt
}

void RS_Init(void)
{
	uint16_t ubrr_val = ((F_CPU)/(9600*8L)-1);
 		UCSR0A = _BV(U2X0);
		UBRR0H = (unsigned char)(ubrr_val>>8);
		UBRR0L = (unsigned char)(ubrr_val & 0xFF);
		UCSR0C = (_BV(UCSZ00) | _BV(UCSZ01));     // Asynchron 8N1 
		UCSR0B = _BV(TXEN0);
}

ISR(TIMER2_OVF_vect) {
	UDR0='*'; // simple char every second
}

void main(void) {
	RTC_Init();
	RS_Init();

	sei();
	while (1) {
		{
			uint16_t i;
			for (i=5;i>0;i--) {
				// dummy short code
				asm volatile ("nop");
			}
		}
#if BUG
/*
see to doc8018.pdf datasheet chapter 17.8.1 


	If Timer/Counter2 is used to wake the device up from Power-save or ADC Noise Reduction
	mode, precautions must be taken if the user wants to re-enter one of these modes: The
	interrupt logic needs one TOSC1 cycle to be reset. If the time between wake-up and reentering
	sleep mode is less than one TOSC1 cycle, the interrupt will not occur, and the device
	will fail to wake up. If the user is in doubt whether the time before re-entering Power-save or
	ADC Noise Reduction mode is sufficient, the following algorithm can be used to ensure that
	one TOSC1 cycle has elapsed:
	a. Write a value to TCCR2A, TCNT2, or OCR2A.
	b. Wait until the corresponding Update Busy Flag in ASSR returns to zero.
	c. Enter Power-save or ADC Noise Reduction mode.

	Reading of the TCNT2 Register shortly after wake-up from Power-save may give an incorrect
	result. Since TCNT2 is clocked on the asynchronous TOSC clock, reading TCNT2 must be
	done through a register synchronized to the internal I/O clock domain. Synchronization takes
	place for every rising TOSC1 edge. When waking up from Power-save mode, and the I/O clock
	(clkI/O) again becomes active, TCNT2 will read as the previous value (before entering sleep)
	until the next rising TOSC1 edge. The phase of the TOSC clock after waking up from Powersave
	mode is essentially unpredictable, as it depends on the wake-up time. The recommended
	procedure for reading TCNT2 is thus as follows:
	a. Write any value to either of the registers OCR2A or TCCR2A.
	b. Wait for the corresponding Update Busy Flag to be cleared.
	c. Read TCNT2.

This example not contain sleep code, it just ilustrate that recomended sequence will cause interrupt lost.
Normally it happen few times every day, this extreme example to show bug inmediately.
It not depend to used register (TCCR2A, TCNT2, or OCR2A)
Without bug in CPU, it show one '*' every second, but it must be buggy, it not work.

*/
			TCCR2A = ((1<<CS22) | (1<<CS20)); // everytime same value, just for sync described in datasheet
			while ((ASSR & _BV(TCR2UB)) != 0) {;}
			//while ((ASSR & (_BV(OCR2UB)|_BV(TCN2UB)|_BV(TCR2UB))) != 0) {;}
#endif	
			
	}

}

FUSES = 
{
    .low = (CKSEL0 & CKSEL2 & CKSEL3 & SUT0 & CKDIV8),  //0x62
    .high = (BOOTSZ0 & BOOTSZ1 & EESAVE & SPIEN & JTAGEN),
    .extended = (BODLEVEL0),
};

