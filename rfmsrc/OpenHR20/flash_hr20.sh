#!/bin/bash 

set -o nounset
set -o errexit
#set -o xtrace

# defaults
PROGRAMMER=dragon_jtag
PROGRAMMER_PORT=usb
PROGRAMMER_OPTS=
# should we set fuses in hr20's atmega? this has to be done only once
SETFUSES=0
BASENAME=hr20
BACKUP=1
EEPROM=1
ADDR=unknown

protectEEPROM() {
	if [ "$BASENAME" == "hr20" ]; then
		$DUDE -U hfuse:w:0x93:m
	else
		$DUDE -U hfuse:w:0x90:m
	fi
}

unprotectEEPROM() {
	if [ "$BASENAME" == "hr20" ]; then
		$DUDE -U hfuse:w:0x9b:m
	else
		$DUDE -U hfuse:w:0x98:m
	fi
}

while [ "$#" -ne 0 ] ; do
	if [ "$1" == "--port" ]; then
		PROGRAMMER_PORT=$2; shift; shift;
	elif [ "$1" == "--addr" ]; then
		ADDR=$2; shift; shift;
	elif [ "$1" == "--prog" ]; then
		PROGRAMMER=$2; shift; shift;
	elif [ "$1" == "--noBackup" ]; then
		BACKUP=0; shift;
	elif [ "$1" == "--noEEPROM" ]; then
		EEPROM=0; shift;
	elif [ "$1" == "--progOpts" ]; then
		PROGRAMMER_OPTS="$PROGRAMMER_OPTS $2"; shift; shift;
	elif [ "$1" == "--setFuses" ]; then
		SETFUSES=1; shift;
	elif [ "$1" == "--HW" ] || [ "$1" == "--hw" ]; then
		BASENAME=$2; shift; shift;
	else
		echo "unknown options starting at $*"
		echo "e.g. $0 --port <programmerPort> --prog <programmer> --noBackup --noEEPROM --progOpts <programmer options> --setFuses --HW <typeOfHW> --addr <addr>"
		echo "defaults: --port $PROGRAMMER_PORT --prog $PROGRAMMER --HW $BASENAME"
		exit 1
	fi
done

if [ "$BASENAME" == "hr20" ]; then
	PROC=m169
else
	PROC=m329
fi

if [ "$EEPROM" == "1" ]; then
	WRITE_EEPROM="-e -U eeprom:w:${BASENAME}.eep"
else
	WRITE_EEPROM=
fi

BDIR=backup/${ADDR}/`date  "+%F_%T"`
DUDE="avrdude -p ${PROC} -c ${PROGRAMMER} -P ${PROGRAMMER_PORT} ${PROGRAMMER_OPTS}"

if [ "$BACKUP" != "0" ]; then
	# do a backup
	echo "*** making backup to ${BDIR}"
	if [ ! -x $BDIR ]; then
		mkdir -p $BDIR
	fi
	echo "*** backing up fuses..."
	$DUDE -U lfuse:r:${BDIR}/lfuse.hex:h -U hfuse:r:${BDIR}/hfuse.hex:h -U efuse:r:${BDIR}/efuse.hex:h || exit
	sleep 3

	echo "*** backing up flash and eeprom..."
	$DUDE -U flash:r:${BDIR}/${BASENAME}.hex:i -U eeprom:r:${BDIR}/${BASENAME}.eep:i
	sleep 3
fi

#flash files from current dir
if [ $SETFUSES -eq 1 ]; then
	echo "*** setting fuses..."
	if [ "$BASENAME" == "hr20" ]; then
		$DUDE -U hfuse:w:0x93:m -U lfuse:w:0xE2:m
	else
		$DUDE -U hfuse:w:0x90:m -U lfuse:w:0x62:m -U efuse:w:0xfd:m
	fi
	sleep 3
fi

[ "$EEPROM" == "1" ] && unprotectEEPROM

echo "*** writing openhr20 flash (and possibly eeprom)"
$DUDE -U flash:w:${BASENAME}.hex $WRITE_EEPROM

# if we wrote the eeprom, then protect the eeprom from erase next time
# so that we can just update the code without blowing away the eeprom

[ "$EEPROM" == "1" ] && protectEEPROM

echo "*** done!"
