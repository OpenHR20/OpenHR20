/*

                           e Y8b    Y8b YV3.18P888 88e
                          d8b Y8b    Y8b Y888P 888 888D
                         d888b Y8b    Y8b Y8P  888 88"
                        d888WuHan8b    Y8b Y   888 b,
                       d8888888b Y8b    Y8P    888 88b,
           8888 8888       ,e,                                  888
           8888 888820078e  " Y8b Y888P ,e e, 888,8, dP"Y ,"Y88b888
           8888 8888888 88b888 Y8b Y8P d88 88b888 " C88b "8" 888888
           8888 8888888 888888  Y8b "  888   ,888    Y88D,ee 888888
           'Y88 88P'888 888888   Y8P    "YeeP"888   d,dP "88 888888
   888 88b,                    d8  888                     888
   888 88P' e88 88e  e88 88e  d88  888 e88 88e  ,"Y88b e88 888 ,e e, 888,8,
   888 8K  d888 888bd888 8Shaoziyang88d888 888b"8" 888d888 888d88 88b888 "
   888 88b,Y888 888PY888 888P 888  888Y888 888P,ee 888Y888 888888   ,888
   888 88P' "88 88"  "88 88"  888  888 "88 88" "88 888 "88 888 "YeeP"888


  Project:       AVR Universal BootLoader
  File:          bootldr.c
                 main program code
  Version:       3.2

  Compiler:      GCC 4.1.2 + AVR Studio 4.13.528

  Author:        Shaoziyang
                 Shaoziyang@gmail.com
                 http://avrubd.googlepages.com
  Date:          2007.9

  Modify:        Add your modify log here

  See readme.txt to get more information.

*/

#include "bootcfg.h"
#include "bootldr.h"

//user's application start address
#define PROG_START         0x0000

//receive buffer' size will not smaller than SPM_PAGESIZE
#if BUFFERSIZE < SPM_PAGESIZE
#define BUFSIZE SPM_PAGESIZE
#else
#define BUFSIZE BUFFERSIZE
#endif

//define receive buffer
unsigned char buf[BUFSIZE];

#if BUFSIZE > 255
unsigned int bufptr, pagptr;
#else
unsigned char bufptr, pagptr;
#endif

unsigned char ch, cl;

//Flash address
#if FLASHEND > 0xFFFFUL
unsigned long int FlashAddr;
#else
unsigned int FlashAddr;
#endif


//update one Flash page
void write_one_page(unsigned char *buf)
{
  boot_page_erase(FlashAddr);                  //erase one Flash page
  boot_spm_busy_wait();
  for(pagptr = 0; pagptr < SPM_PAGESIZE; pagptr += 2) //fill data to Flash buffer
  {
    boot_page_fill(pagptr, buf[pagptr] + (buf[pagptr + 1] << 8));
  }
  boot_page_write(FlashAddr);                  //write buffer to one Flash page
  boot_spm_busy_wait();                        //wait Flash page write finish
}

//jump to user's application
void quit()
{
  boot_rww_enable();                           //enable application section
  (*((void(*)(void))PROG_START))();            //jump
}

//send data to comport
void WriteCom(unsigned char dat)
{
#if RS485
  RS485Enable();
#endif

  UDRREG(COMPORTNo) = dat;
  //wait send finish
  while(!(UCSRAREG(COMPORTNo) & (1<<TXCBIT(COMPORTNo))));
  UCSRAREG(COMPORTNo) |= (1 << TXCBIT(COMPORTNo));

#if RS485
  RS485Disable();
#endif
}

//wait receive a data from comport
unsigned char WaitCom()
{
  while(!DataInCom());
  return ReadCom();
}

#if VERBOSE
void putstr(const char *str)
{
  while(*str)
    WriteCom(*str++);
  WriteCom(0x0D);
  WriteCom(0x0A);
}
#endif

//calculate CRC checksum
#if BUFSIZE > 255
void crc16(unsigned char *buf)
{
  unsigned int j;
#else
void crc16(unsigned char *buf)
{
  unsigned char j;
#endif

#if CRCMODE == 0
  unsigned char i;
  unsigned int t;
#endif
  unsigned int crc;

  crc = 0;
  for(j = BUFFERSIZE; j > 0; j--)
  {
#if CRCMODE == 0
    //CRC1021 checksum
    crc = (crc ^ (((unsigned int) *buf) << 8));
    for(i = 8; i > 0; i--)
    {
      t = crc << 1;
      if(crc & 0x8000)
        t = t ^ 0x1021;
      crc = t;
    }
#elif CRCMODE == 1
    //word add up checksum
    crc += (unsigned int)(*buf);
#elif

#endif
    buf++;
  }
  ch = crc / 256;
  cl = crc % 256;
}

int main(void)
{
  unsigned char cnt;
  unsigned char packNO;
  unsigned char crch, crcl;

#if InitDelay > 255
  unsigned int di;
#elif InitDelay
  unsigned char di;
#endif

#if BUFFERSIZE > 255
  unsigned int li;
#else
  unsigned char li;
#endif

  //disable interrupt
  __asm__ __volatile__("cli": : );

#if WDGEn
  //if enable watchdog, setup timeout
  wdt_enable(WDTO_1S);
#endif

  //initialize timer1, CTC mode
  TimerInit();

#if RS485
  DDRREG(RS485PORT) |= (1 << RS485TXEn);
  RS485Disable();
#endif

#if LEDEn
  //set LED control port to output
  DDRREG(LEDPORT) = (1 << LEDPORTNo);
#endif

  //initialize comport with special config value
  ComInit();

#if InitDelay
  //some kind of avr mcu need special delay after comport initialization
  for(di = InitDelay; di > 0; di--)
    __asm__ __volatile__ ("nop": : );
#endif

#if LEVELMODE
  //port level launch boot
  //set port to input
  DDRREG(LEVELPORT) &= ~(1 << LEVELPIN);
#if PINLEVEL
  if(PINREG(LEVELPORT) & (1 << LEVELPIN))
#else
  if(!(PINREG(LEVELPORT) & (1 << LEVELPIN)))
#endif
  {
#if VERBOSE
    //prompt enter boot mode
    putstr(msg6);
#endif
  }
  else
  {
#if VERBOSE
    //prompt execute user application
    putstr(msg7);
#endif

    quit();
  }

#else
  //comport launch boot

#if VERBOSE
  //prompt waiting for password
  putstr(msg1);
#endif

  cnt = TimeOutCnt;
  cl = 0;
  while(1)
  {
#if WDGEn
    //clear watchdog
    wdt_reset();
#endif

    if(TIFRREG & (1<<OCF1A))    //T1 overflow
    {
      TIFRREG |= (1 << OCF1A);

      if(cl == CONNECTCNT)
        break;

#if LEDEn
      //toggle LED
      LEDAlt();
#endif

      cnt--;
      if(cnt == 0)
      {

#if VERBOSE
        //prompt timeout
        putstr(msg2);
#endif
        //quit bootloader
        quit();
      }
    }

    if(DataInCom())
    {
      if(ReadCom() == KEY[cl])
        cl++;
      else
        cl = 0;
    }
  }

#endif  //LEVELMODE

#if VERBOSE
  //prompt waiting for data
  putstr(msg3);
#endif

  //every interval send a "C",waiting XMODEM control command <soh>
  cnt = TimeOutCntC;
  while(1)
  {
    if(TIFRREG & (1 << OCF1A))  //T1 overflow
    {
      TIFRREG |= (1 << OCF1A);
      WriteCom(XMODEM_RWC) ;    //send "C"

#if LEDEn
      //toggle LED
      LEDAlt();
#endif

      cnt--;
      if(cnt == 0)
      {
#if VERBOSE
        //prompt timeout
        putstr(msg2);
#endif
        quit();
      }
    }

#if WDGEn
    //clear watchdog
    wdt_reset();
#endif

    if(DataInCom())
    {
      if(ReadCom() == XMODEM_SOH)  //XMODEM command <soh>
        break;
    }
  }
  //close timer1
  TCCR1B = 0;

  //begin to receive data
  packNO = 0;
  bufptr = 0;
  cnt = 0;
  FlashAddr = 0;
  do
  {
    packNO++;
    ch =  WaitCom();                          //get package number
    cl = ~WaitCom();
    if ((packNO == ch) && (packNO == cl))
    {
      for(li = 0; li < BUFFERSIZE; li++)      //receive a full data frame
      {
        buf[bufptr++] = WaitCom();
      }
      crch = WaitCom();                       //get checksum
      crcl = WaitCom();
      crc16(&buf[bufptr - BUFFERSIZE]);       //calculate checksum
      if((crch == ch) && (crcl == cl))
      {
#if BootStart
        if(FlashAddr < BootStart)             //avoid write to boot section
        {
#endif

#if BUFFERSIZE < SPM_PAGESIZE
          if(bufptr >= SPM_PAGESIZE)          //Flash page full, write flash page;otherwise receive next frame
          {                                   //receive multi frames, write one page
            write_one_page(buf);              //write data to Flash
            FlashAddr += SPM_PAGESIZE;        //modify Flash page address
            bufptr = 0;
          }
#else
          while(bufptr > 0)                   //receive one frame, write multi pages
          {
            write_one_page(&buf[BUFSIZE - bufptr]);
            FlashAddr += SPM_PAGESIZE;        //modify Flash page address
            bufptr -= SPM_PAGESIZE;
          }
#endif

#if BootStart
        }
        else                                  //ignore flash write when Flash address exceed BootStart
        {
          bufptr = 0;                         //reset receive pointer
        }
#endif

//read flash, compare with buffer's content
#if (ChipCheck > 0) && (BootStart > 0)
#if BUFFERSIZE < SPM_PAGESIZE
        if((bufptr == 0) && (FlashAddr <= BootStart))
#else
        if(FlashAddr <= BootStart)
#endif
        {
          boot_rww_enable();                  //enable application section
          cl = 1;                             //clear error flag
          for(pagptr = 0; pagptr < BUFSIZE; pagptr++)
          {
            if(pgm_read_byte(FlashAddr - BUFSIZE + pagptr) != buf[pagptr])
            {
              cl = 0;                         //set error flag
              break;
            }
          }
          if(cl)                              //checksum equal, send ACK
          {
            WriteCom(XMODEM_ACK);
            cnt = 0;
          }
          else
          {
            WriteCom(XMODEM_NAK);             //checksum error, ask resend
            cnt++;                            //increase error counter
            FlashAddr -= BUFSIZE;             //modify Flash page address
          }
        }
        else                                  //no verify, send ACK directly
        {
          WriteCom(XMODEM_ACK);
          cnt = 0;
        }
#else
        //no verify, send ACK directly
        WriteCom(XMODEM_ACK);
        cnt = 0;
#endif

#if WDGEn
        //clear watchdog
        wdt_reset();
#endif

#if LEDEn
        //LED indicate update status
        LEDAlt();
#endif
      }
      else //CRC
      {
        //ask resend
        WriteCom(XMODEM_NAK);
        cnt++;
      }
    }
    else //PackNo
    {
      //ask resend
      WriteCom(XMODEM_NAK);
      cnt++;
    }
    //too much error, update abort
    if(cnt > 3)
      break;
  }
  while(WaitCom() != XMODEM_EOT);
  WriteCom(XMODEM_ACK);


#if VERBOSE
  if(cnt == 0)
  {
    //prompt update success
    putstr(msg4);
  }
  else
  {
    //prompt update fail
    putstr(msg5);

#if WDGEn
    //dead loop, wait watchdog reset
    while(1);
#endif

  }
#endif

  //quit boot mode
  quit();
  return 0;
}

//End of file: bootldr.c
