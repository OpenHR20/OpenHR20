//////////////////////////////////////////////////////////////////////////////

#include <stdint.h>

#include "security.h"
#include "config.h"
#include "eeprom.h" // for config and eeprom reading xtea's init vector


#if (SECURITY==1)

#include <stdlib.h> 
#include <avr/io.h>

/*!
 *******************************************************************************
 *
 *  Generate a seed for the stdlib's random number generator
 *  \note see http://www.roboternetz.de/wissen/index.php/Zufallszahlen_mit_avr-gcc
 *
 *  \return a pseudo random value from mangled SRAM content
 *
 ******************************************************************************/
static unsigned short security_get_seed()
{
	// TODO: this could be done smaller or even better by seeding with som least significant ADC bits

   unsigned short seed = 0;
   unsigned short *p = (unsigned short*) (RAMEND+1);
   extern unsigned short __heap_start;
    
   while (p >= &__heap_start + 1)
      seed ^= * (--p);
    
   return seed;
}

/*!
 *******************************************************************************
 *
 *  Initializes the security subsystem
 *  \note Generate a seed for the stdlib's random number generator
 *        see http://www.roboternetz.de/wissen/index.php/Zufallszahlen_mit_avr-gcc
 *
 ******************************************************************************/
void security_init(void)
{
	srand(security_get_seed());
}

/*!
 *******************************************************************************
 *
 *  XTEA-blockencryption
 *  \note LET'S DO THAT IN ASSEMBLER LATER
 *
 *  \param v pointer to the plaintext which will be encrypted (in place)
 *  \param k pointer to the key (16 byte)
 *
 ******************************************************************************/
static void xtea_encipher(uint32_t* v, uint32_t* k) 
{
    uint32_t v0=v[0], v1=v[1], i;
    uint32_t sum=0, delta=0x9E3779B9;
    for(i=0; i<XTEA_NUMOFROUNDS; i++) {
        v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
        sum += delta;
        v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum>>11) & 3]);
    }
    v[0]=v0; v[1]=v1;
}

/*!
 *******************************************************************************
 *
 *  XTEA-blockdecryption 
 *  \note LET'S DO THAT IN ASSEMBLER LATER
 *  \note since we dont need decryption because we drive 
 *
 *  \param v pointer to the ciphertext which will be decrypted (in place)
 *  \param k pointer to the key (16 byte)
 *
 ******************************************************************************/
/* we dont need decryption when driving in CFB mode:
static void xtea_decipher(uint32_t* v, uint32_t* k)
{
    uint32_t v0=v[0], v1=v[1], i;
    uint32_t delta=0x9E3779B9, sum=delta*XTEA_NUMOFROUNDS;
    for(i=0; i<XTEA_NUMOFROUNDS; i++) {
        v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum>>11) & 3]);
        sum -= delta;
        v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
    }
    v[0]=v0; v[1]=v1;
}
*/

/*!
 *******************************************************************************
 *
 *  internal XTEA function for preparing encryption.
 *
 *  \param xtea_key   pointer to a XTEA_KEYSIZE sized buffer that will be filled with the XTEA Key
 *  \param xtea_block pointer to a XTEA_BLOCKSIZE sized buffer that will be filled with the init vector
 *
 ******************************************************************************/
static void xtea_init(uint8_t* xtea_key, uint8_t* xtea_block)
{
	uint8_t i;

#if (SECURITY_KEYSIZE < XTEA_KEYSIZE)
	// expand the key to xtea-size. simply via bytewise-repeating
	for (i=0; i<XTEA_KEYSIZE; i++)
	{
		xtea_key[i] = config.security_key[i % SECURITY_KEYSIZE];
	}
#else
	// no key expansion required ...
#endif

	// load the init vector from the eeprom
	for (i=0; i<XTEA_BLOCKSIZE; i++)
	{
		xtea_block[i] = EEPROM_read((uint16_t)(ee_xtea_initvector) + i);
	}
}

/*!
 *******************************************************************************
 *
 *  XTEA-en- or decrypts a buffer in OFB or CFB mode. (define SECURITY_OFB or SECURITY_CFB)
 *
 *  \param msgbuf  pointer to the message which will be encrypted (in place).
 *  \param msgsize size of the message buffer 
 *  \param encrypt if true, message will be encrypted, otherwise decrypted
 *
 *  \note
 *      - the key used for encryption is taken from config.security_key[]
 *
 ******************************************************************************/
void security_crypt(uint8_t* msgbuf, uint8_t msgsize, bool encrypt)
{
	uint8_t xtea_block[XTEA_BLOCKSIZE];
	uint8_t xtea_key[XTEA_KEYSIZE];
	xtea_init(xtea_key, xtea_block);

	uint8_t m=0;
	while (m < msgsize)
	{
		xtea_encipher((uint32_t*)xtea_block, (uint32_t*)(xtea_key));

		uint8_t i;
		for (i=0; i<XTEA_BLOCKSIZE; i++)
		{
			uint8_t msgbuf_m = msgbuf[m];
			msgbuf[m]       ^= xtea_block[i];
			xtea_block[i]    = encrypt ? 
							   msgbuf[m] : // next rount cipher input is the xored msgblock
							   msgbuf_m;   // next rount cipher input is the xored msgblock

			m++;
			if (m == msgsize)
			{
				return;
			}
		}
	}
}

static uint16_t local_chall; //!< the last generated challenge. we must store it for checking that later

/*!
 *******************************************************************************
 *
 *  generates a challenge (nonce) for challenge-response authentication
 *
 *  \return a rtandom value for the authentication's opponent to use for responding
 *
 ******************************************************************************/
uint16_t security_generate_challenge(void)
{
	local_chall = rand();
	return local_chall;
}

/*!
 *******************************************************************************
 *
 *  generates a response for the opponent's given challenge in challenge-response authentication
 *
 *  \param chall the opponent's given challenge
 *  \return the challenge, encrypted with the system key
 *
 ******************************************************************************/
uint16_t security_generate_response(uint16_t remote_chall)
{

	/*
	uint8_t xtea_block[XTEA_BLOCKSIZE];
	uint8_t xtea_key[XTEA_KEYSIZE];

	xtea_init(xtea_key, xtea_block);

	xtea_block[0] = HIBYTE(remote_chall);
	xtea_block[1] = LOBYTE(remote_chall);

	xtea_encipher((uint32_t*)xtea_block, (uint32_t*)(xtea_key));

	return MKINT16(xtea_block[0], xtea_block[1]);
	*/
}

/*!
 *******************************************************************************
 *
 *  checks wheather the opponent's response is as expected in challenge-response authentication
 *
 *  \param resp the opponent's response
 *  \return true, if opponent knew our system key as well.
 *
 ******************************************************************************/
bool security_check_response(uint16_t remote_resp)
{
	return (remote_resp == security_generate_response(local_chall));
}


#endif // #if (SECURITY==1)
