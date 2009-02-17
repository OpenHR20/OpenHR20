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
 * \file       cmac.c
 * \brief      CMAC implementation (http://csrc.nist.gov/publications/nistpubs/800-38B/SP_800-38B.pdf)
 * \author     Jiri Dobry <jdobry-at-centrum-dot-cz>
 * \date       $Date$
 * $Rev$
 */

#include <avr/pgmspace.h>
#include <string.h>
#include "config.h"
#include "../common/xtea.h"
#include "eeprom.h"

#warning "test only"

uint8_t Keys[5*8]; // 40 bytes
#define K_mac (Keys+0*8)
#define K_enc (Keys+1*8)
#define K1 (Keys+3*8)
#define K2 (Keys+4*8)
#define K_m (Keys+3*8)  /* share same position as K1 & K2 */
// note: do not change order of keys in Keys array, it depend to crypto_init

uint8_t Km_upper[8] PROGMEM = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef
};

/* internal function for crypto_init */
asm (
    "left_roll:               \n"
    "   ld r25,Y              \n"
    "   lsl r25               \n"
    "   ldd __tmp_reg__,Y+1   \n"
    "   rol __tmp_reg__       \n"
    "   std Z+1,__tmp_reg__   \n"
    "   ldd __tmp_reg__,Y+2   \n"
    "   rol __tmp_reg__       \n"
    "   std Z+2,__tmp_reg__   \n"
    "   ldd __tmp_reg__,Y+3   \n"
    "   rol __tmp_reg__       \n"
    "   std Z+3,__tmp_reg__   \n"
    "   ldd __tmp_reg__,Y+4   \n"
    "   rol __tmp_reg__       \n"
    "   std Z+4,__tmp_reg__   \n"
    "   ldd __tmp_reg__,Y+5   \n"
    "   rol __tmp_reg__       \n"
    "   std Z+5,__tmp_reg__   \n"
    "   ldd __tmp_reg__,Y+6   \n"
    "   rol __tmp_reg__       \n"
    "   std Z+6,__tmp_reg__   \n"
    "   ldd __tmp_reg__,Y+7   \n"
    "   rol __tmp_reg__       \n"
    "   std Z+7,__tmp_reg__   \n"
    "   brcc left_roll_brcc   \n"
    "   ori r25,1             \n"
    "left_roll_brcc:          \n"
    "   st Z,r25              \n"
    "   ret "
);
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
    :"r25","r30","r31" 
    );
}

bool cmac_calc (uint8_t* m, uint8_t bytes, uint8_t* data_prefix, bool check) {
/*   reference: http://csrc.nist.gov/publications/nistpubs/800-38B/SP_800-38B.pdf
 *   1.Let Mlen = message length in bits
 *   2.Let n = Mlen / 64
 *   3.Let M1, M2, ... , Mn-1, Mn
 *    denote the unique sequence of bit strings such that
 *     M = M1 || M2 || ... || Mn-1 || Mn,
 *      where M1, M2,..., Mn-1 are complete 8 byte blocks.
 *   4.If Mn is a complete block, let Mn = K1 XOR Mn else,
 *    let Mn = K2 XOR (Mn ||10j), where j = n*64 – Mlen - 1.
 *   5.Let C0 = 0
 *   6.For i = 1 to n, let Ci = ENC_KMAC(Ci-1 XOR Mi).
 *   7.Let MAC = MSB32(Cn). (4 most significant byte)
 *   8.Add MAC to end of "m" 
 */
  
    uint8_t i,j;
    uint8_t buf[8];

#if defined(MASTER_CONFIG_H)
    if (data_prefix==NULL) {
        for (i=0;i<8;buf[i++]=0) {;}
    } else 
#endif
    {
        memcpy(buf,data_prefix,sizeof(buf));
        xtea_enc(buf, buf, K_mac);
    } 


    for (i=0; i<bytes; ) {
        uint8_t x=i;
        i+=8;
        uint8_t* Kx;
        if (i>=bytes) Kx=((i==bytes)?K1:K2);
        for (j=0;j<8;j++,x++) {
            uint8_t tmp;
            if (x<bytes) tmp=m[x];
            else tmp=((x==bytes)?0x80:0);
            if (i>=bytes) tmp ^= Kx[j];
            buf[j] ^= tmp;
        }
        xtea_enc(buf, buf, K_mac);
    }
    if (check) {
        for (i=0;i<4;i++) {
            if (m[bytes+i]!=buf[i]) return false;
        }
    } else {
        memcpy(m+bytes,buf,4);
    }
    return true;
    #if 0
    // hack to use __prologue_saves__ and __epilogue_restores__ rather push&pop
    asm ( "" ::: 
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", 
        "r9", "r10", "r11", "r12", "r13", "r14", "r15", "r16", 
        "r17", "r18", "r19", "r20", "r21", "r22", "r23", "r24" 
        ); 
    #endif
}
