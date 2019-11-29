#!/bin/bash
PROGRAMMER=dragon_isp
PROGRAMMER_PORT=usb

BDIR=`date  "+%F_%T"`
DUDE="avrdude -p m169 -c $PROGRAMMER -P $PROGRAMMER_PORT"

# do a backup
echo "*** making backup..."
if [ ! -x $BDIR ]; then
	mkdir $BDIR
fi
$DUDE -U flash:r:${BDIR}/master.bin:r -U eeprom:r:${BDIR}/master.eep:i || exit
sleep 3

echo "*** writing master flash and eeprom..."
#flash files from current dir
$DUDE -U flash:w:master.bin:r -U eeprom:w:master.eep || exit
echo "*** done!"
