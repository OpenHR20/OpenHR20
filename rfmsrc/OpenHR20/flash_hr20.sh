#!/bin/bash
PROGRAMMER=dragon_jtag
PROGRAMMER_PORT=usb
# should we set fuses in hr20's atmega? this has to be done only once
SETFUSES=0

BDIR=`date  "+%F_%T"`
DUDE="avrdude -p m169 -c $PROGRAMMER -P $PROGRAMMER_PORT"

# do a backup
echo "*** making backup..."
if [ ! -x $BDIR ]; then
	mkdir $BDIR
fi
echo "*** backing up fuses..."
$DUDE -U lfuse:r:${BDIR}/lfuse.hex:h -U hfuse:r:${BDIR}/hfuse.hex:h -U efuse:r:${BDIR}/efuse.hex:h || exit
sleep 3

echo "*** backing up flash and eeprom..."
$DUDE -U flash:r:${BDIR}/hr20.hex:i -U eeprom:r:${BDIR}/hr20.eep:i
sleep 3

#flash files from current dir
if [ $SETFUSES -eq 1 ]; then
	echo "*** setting fuses..."
	$DUDE -U hfuse:w:0x9B:m -U lfuse:w:0xE2:m
	sleep 3
fi
echo "*** writing openhr20 flash and eeprom..."
$DUDE -U flash:w:hr20.hex -U eeprom:w:hr20.eep
echo "*** done!"
