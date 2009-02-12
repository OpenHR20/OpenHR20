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

#include "config.h"
#include "../common/xtea.h"

#warning "test only"

uint8_t K_mac[16] = {
    0x00, 0x01, 0x02, 0x03, 
    0x10, 0x11, 0x12, 0x13, 
    0x20, 0x21, 0x22, 0x23, 
    0x30, 0x31, 0x32, 0x33 
};

static uint8_t K1[8];
static uint8_t K2[8];

/* internal function for cmac_init */
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
void cmac_init(void) {
    uint8_t i;
    for (i=0;i<8;i++) { // smaller&faster than memset
        K1[i]=0;
    }
    xtea_enc(K1, K1, K_mac);
    asm (
    "   movw  R30,%A0   \n"
    "   call left_roll \n"
    "   ldi r30,lo8(K2) \n"
    "   ldi r31,hi8(K2) \n"
    "   call left_roll \n"
    :: "y" (K1)
    :"r25","r30","r31" 
    );
}


void cmac_calc_add (uint8_t* m, uint8_t bytes) {
/*  1.Let Mlen = message length in bits
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
  
    uint8_t blocks = (bytes-1)/8;
    uint8_t i,j;

    uint8_t buf[8];
    for (i=0;i<8;i++) { // smaller&faster than memset
        buf[i]=0;
        m[i+bytes]=0;   // for step 4 incomplete blocks / slow but small
    }
    
//    i=8-bytes%8;  // faster but longer
//    while(i>0) {
//        m[(i--)+bytes]=0;  // for step 4 incomplete blocks
//    }
    m[bytes]=0x80;     // for step 4 incomplete blocks

    for (i=0; i<=blocks; i++) {
        if (i==blocks) {  // step 4
            uint8_t* Kx;
            Kx = ((blocks%8) != 0)?K2:K1;
            for (j=0;j<8;j++) {
                buf[j] ^= Kx[j];
            }
        }    
        for (j=0;j<8;j++) {
            buf[j] ^= m[(i<<3) + j]; 
        }
        xtea_enc(buf, buf, K_mac);
    }
    memcpy(m+bytes,buf,4);
    #if 1
    // hack to use __prologue_saves__ and __epilogue_restores__ rather push&pop
    // it save 78 bytes
    asm ( "" ::: 
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", 
        "r9", "r10", "r11", "r12", "r13", "r14", "r15", "r16", 
        "r17", "r18", "r19", "r20", "r21", "r22", "r23", "r24" 
        ); 
    #endif
}
