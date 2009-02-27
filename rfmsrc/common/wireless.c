/*
 *  Open HR20
 *
 *  target:     ATmega169 in Honnywell Rondostat HR20E / ATmega8
 *
 *  compiler:   WinAVR-20071221
 *              avr-libc 1.6.0
 *              GCC 4.2.2
 *
 *  copyright:  2008 Jiri Dobry (jdobry-at-centrum-dot-cz)
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
 * \file       wireless.c
 * \brief      wireless layer
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */


#include "config.h"
#include <avr/pgmspace.h>
#include <string.h>
#include "../common/xtea.h"
#include "eeprom.h"
#include "../common/wireless.h"
#include "../common/cmac.h"
#include "com.h"
#if !defined(MASTER_CONFIG_H)
    #include "controller.h"
#endif


uint8_t Keys[5*8]; // 40 bytes

#define WIRELESS_BUF_MAX (RFM_FRAME_MAX-(4+4+2))

static uint8_t wireless_framebuf[WIRELESS_BUF_MAX];
uint8_t wireless_buf_ptr=0;

static uint8_t Km_upper[8] PROGMEM = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef
};

/*!
 *******************************************************************************
 *  init crypto keys
 ******************************************************************************/

void crypto_init(void) {
    uint8_t i;
    memcpy(K_m,config.security_key,8);
    memcpy_P(K_m+8,Km_upper,sizeof(Km_upper)); 
    for (i=0;i<3*8;i++) {
        Keys[i]=0xc0+i;
    }
    xtea_enc(K_mac, K_mac, K_m); /* generate K_mac low 8 bytes */
    xtea_enc(K_enc, K_enc, K_m); /* generate K_mac high 8 bytes  and K_enc low 8 bytes*/
    xtea_enc(K_enc+8, K_enc+8, K_m); /* generate K_enc high 8 bytes */
    for (i=0;i<8;i++) { // smaller&faster than memset
        K1[i]=0;
    }
    xtea_enc(K1, K1, K_mac);
    asm (
    "   movw  R30,%A0   \n"
    "   rcall left_roll \n" /* generate K1 */
    "   ldi r30,lo8(" STR(K2) ") \n"
    "   ldi r31,hi8(" STR(K2) ") \n"
    "   rcall left_roll \n" /* generate K2 */
    :: "y" (K1)
    :"r26", "r27", "r30","r31" 
    );
}
/* internal function for crypto_init */
/* use loop inside - short/slow */
asm (
    "left_roll:               \n"
    "   ldd r26,Y+7           \n"
    "   lsl r26               \n"
    "   in r27,__SREG__       \n"   // save carry
    "   ldi r26,7             \n"   // 8 times
    "roll_loop:               \n"
    "   ld __tmp_reg__,Y      \n"
    "   out __SREG__,r27      \n"   // restore carry
    "   rol __tmp_reg__       \n"
    "   in r27,__SREG__       \n"   // save carry
    "   st Z,__tmp_reg__      \n"
    "   adiw r28,1            \n"   // Y++
    "   adiw r30,1            \n"   // Z++
    "   subi r26,1            \n"
    "   brcc roll_loop        \n"   // 8 times loop
    "   sbiw r28,8            \n"   // Y-=8
    "   ret "
);

/*!
 *******************************************************************************
 *  encrypt / decrypt
 *  \note symetric operation 
 ******************************************************************************/


static void encrypt_decrypt (uint8_t* p, uint8_t len) {
    uint8_t i=0;
    uint8_t buf[8];
    while(i<len) {
        xtea_enc(buf,&RTC,K_enc);
        RTC.pkt_cnt++;
        do {
            p[i]^=buf[i&7];
            i++;
            if (i>=len) return; //done
        } while ((i&7)!=0);
    }
}

/*!
 *******************************************************************************
 *  wireless send data packet
 ******************************************************************************/

void wirelessSendPacket(void) {
	RFM_TX_ON_PRE();

	rfm_framebuf[ 0] = 0xaa; // preamble
	rfm_framebuf[ 1] = 0xaa; // preamble
	rfm_framebuf[ 2] = 0x2d; // rfm fifo start pattern
	rfm_framebuf[ 3] = 0xd4; // rfm fifo start pattern
    
    rfm_framesize=wireless_buf_ptr+2+4;

	rfm_framebuf[ 4] = rfm_framesize;    // length
#if defined(MASTER_CONFIG_H)
	rfm_framebuf[ 5] = 0;
#else
	rfm_framebuf[ 5] = config.RFM_devaddr;
#endif
	
	memcpy(rfm_framebuf+6,wireless_framebuf,wireless_buf_ptr);

    encrypt_decrypt (rfm_framebuf+6, wireless_buf_ptr);
    cmac_calc(rfm_framebuf+5,rfm_framesize-5,(uint8_t*)&RTC,false);
    RTC.pkt_cnt++;
    rfm_framesize+=4; //MAC
	rfm_framebuf[rfm_framesize++] = 0xaa; // dummy byte
	rfm_framebuf[rfm_framesize++] = 0xaa; // dummy byte

	rfm_framepos  = 0;
	rfm_mode = rfmmode_tx;
	RFM_TX_ON();
    RFM_SPI_SELECT; // set nSEL low: from this moment SDO indicate FFIT or RGIT
    RFM_INT_EN(); // enable RFM interrupt
}

/*!
 *******************************************************************************
 *  wireless send SYNC packet
 ******************************************************************************/

#if defined(MASTER_CONFIG_H)
void wirelessSendSync(void) {
    RFM_TX_ON_PRE();
	rfm_framebuf[ 0] = 0xaa; // preamble
	rfm_framebuf[ 1] = 0xaa; // preamble
	rfm_framebuf[ 2] = 0x2d; // rfm fifo start pattern
	rfm_framebuf[ 3] = 0xd4; // rfm fifo start pattern

	rfm_framebuf[ 4] = (wireless_buf_ptr+1+4) | 0xc0; // length (sync)

	memcpy(rfm_framebuf+5,wireless_framebuf,wireless_buf_ptr);

	cmac_calc(rfm_framebuf+5,wireless_buf_ptr,NULL,false);
	
	rfm_framesize=wireless_buf_ptr+4+1+4;

	rfm_framebuf[rfm_framesize++] = 0xaa; // dummy byte
	rfm_framebuf[rfm_framesize++] = 0xaa; // dummy byte

	rfm_framepos  = 0;
	rfm_mode    = rfmmode_tx;
	RFM_TX_ON();
    RFM_SPI_SELECT; // set nSEL low: from this moment SDO indicate FFIT or RGIT
    RFM_INT_EN(); // enable RFM interrupt
}
#else
int8_t time_sync_tmo=0;
#endif

/*!
 *******************************************************************************
 *  wireless receive data packet
 ******************************************************************************/
void wirelessReceivePacket(void) {
	if (rfm_framepos>=1) {
        if ((rfm_framepos >= (rfm_framebuf[0]&0x3f)) 
                || ((rfm_framebuf[0]&0x3f)>= RFM_FRAME_MAX)  // reject noise
                || (rfm_framepos >= RFM_FRAME_MAX)) {
            RFM_INT_DIS(); // disable RFM interrupt
            if (rfm_framepos>(rfm_framebuf[0]&0x3f)) rfm_framepos=(rfm_framebuf[0]&0x3f);
            if (rfm_framepos>=2+4) {
                bool mac_ok;
                #if ! defined(MASTER_CONFIG_H)
                if (rfm_framebuf[0] == 0xc9) {
                //sync packet
                    mac_ok=cmac_calc(rfm_framebuf+1,(rfm_framebuf[0]&0x3f)-5,NULL,true);
                    COM_dump_packet(rfm_framebuf, rfm_framepos,mac_ok);
                    if (mac_ok) {
                        time_sync_tmo=10;
                        rfm_mode = rfmmode_stop;
                        RFM_OFF();
                        RTC_s256=10; 
                        RTC_SetYear(rfm_framebuf[1]);
                        RTC_SetMonth(rfm_framebuf[2]>>4);
                        RTC_SetDay((rfm_framebuf[3]>>5)+((rfm_framebuf[2]<<3)&0x18));
            			RTC_SetHour(rfm_framebuf[3]&0x1f);
            			RTC_SetMinute(rfm_framebuf[4]>>1);
            			RTC_SetSecond((rfm_framebuf[4]&1)?30:00);
                        cli(); RTC_timer_done&=~_BV(RTC_TIMER_OVF); sei();
                        }
                } else 
                #endif
                {
                    RTC.pkt_cnt+= (rfm_framepos+7-2-4)/8;
                    mac_ok = cmac_calc(rfm_framebuf+1,rfm_framepos-1-4,(uint8_t*)&RTC,true);
                    RTC.pkt_cnt-= (rfm_framepos+7-2-4)/8;
                    encrypt_decrypt (rfm_framebuf+2, rfm_framepos-2-4);
                    RTC.pkt_cnt++;
                    COM_dump_packet(rfm_framebuf, rfm_framepos,mac_ok);
                }
            }
            rfm_framepos=0;
            #if 1 || defined(MASTER_CONFIG_H)
			    rfm_mode = rfmmode_rx;
              	RFM_SPI_16(RFM_FIFO_IT(8) |               RFM_FIFO_DR);
                RFM_SPI_16(RFM_FIFO_IT(8) | RFM_FIFO_FF | RFM_FIFO_DR);
                RFM_INT_EN(); // enable RFM interrupt
			#else
			    rfm_mode = rfmmode_stop;
                RFM_OFF();
			#endif
        }
    }
}

/*!
 *******************************************************************************
 *  wireless Check time synchronization
 ******************************************************************************/
#if ! defined(MASTER_CONFIG_H)
void wirelesTimeSyncCheck(void) {
    time_sync_tmo--;
    if (time_sync_tmo<=0) {
        if (time_sync_tmo==0) {
        		RFM_INT_DIS();
			    RFM_SPI_16(RFM_FIFO_IT(8) |               RFM_FIFO_DR);
                RFM_SPI_16(RFM_FIFO_IT(8) | RFM_FIFO_FF | RFM_FIFO_DR);
                RFM_RX_ON();    //re-enable RX
				rfm_framepos=0;
				rfm_mode = rfmmode_rx;
			    RFM_INT_EN(); // enable RFM interrupt
        } else if (time_sync_tmo<-4) {
			RFM_INT_DIS();
		    RFM_OFF();		// turn everything off
		    CTL_error |= CTL_ERR_RFM_SYNC;
        } 
    }
}
#endif

/*!
 *******************************************************************************
 *  wireless put one byte
 ******************************************************************************/
void wireless_putchar(uint8_t b) {
    if (wireless_buf_ptr<WIRELESS_BUF_MAX) {
        wireless_framebuf[wireless_buf_ptr++] = b;
    }
}
