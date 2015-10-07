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
DRYRUN=0

# fuses are as follows:
# Brown out detection is turned off to save power, no bootloader
# Be aware old some versions of avrdude reverse fuses on printout in verbose mode!

# From the 329 manual:
# 
# Extended Fuse Byte Bit No Description Default Value
# – 7– 1
# – 6– 1
# – 5– 1
# – 4– 1
# – 5– 1
# BODLEVEL1(1) 2 Brown-out Detector trigger level 1 (unprogrammed)
# BODLEVEL0(1) 1 Brown-out Detector trigger level 1 (unprogrammed)
# RSTDISBL(2) 0 External Reset Disable 1 (unprogrammed)

# Low Byte
# CKDIV8(4) 7 Divide clock by 8 0 (programmed)
# CKOUT(3) 6 Clock output 1 (unprogrammed)
# SUT1 5 Select start-up time 1 (unprogrammed)(1)
# SUT0 4 Select start-up time 0 (programmed)(1)
# CKSEL3 3 Select Clock source 0 (programmed)(2)
# CKSEL2 2 Select Clock source 0 (programmed)(2)
# CKSEL1 1 Select Clock source 1 (unprogrammed)(2)
# CKSEL0 0 Select Clock source 0 (programmed)(2)

# Table 27-4. Fuse High Byte Fuse High Byte Bit No Description Default Value
# OCDEN(4) 7 Enable OCD 1 (unprogrammed, OCD disabled)
# JTAGEN(5) 6 Enable JTAG 0 (programmed, JTAG enabled)
# SPIEN(1) 5 Enable Serial Program and Data  Downloading 0 (programmed, SPI prog. enabled)
# WDTON(3) 4 Watchdog Timer always on 1 (unprogrammed)
# EESAVE 3 EEPROM memory is preserved through the Chip Erase 1 (unprogrammed,  EEPROM not preserved)
# BOOTSZ1 2 Select Boot Size (see Table 27-6 for details) 0 (programmed)(2)
# BOOTSZ0 1 Select Boot Size (see Table 27-6 for details) 0 (programmed)(2)
# BOOTRST 0 Select Reset Vector 1 (unprogrammed)

# Starting at 1MHz (0x62) or 8MHz (0xE2) makes no odds. main.c overrides this value.
# Let's choose 1MHz
LFUSE=0x62
HFUSE_PROTECTED_EEPROM=0x97
HFUSE_UNPROTECTED_EEPROM=0x9f
# no brown out detect. This may save some power, but may require a longer period of
# no batteries to reset the unit. Use 0xFD for brown out detect 1.8V
EFUSE=0xFF


# -- noEEPROM not working?

protectEEPROM() {
	$DUDE -U hfuse:w:${HFUSE_PROTECTED_EEPROM}:m
}

unprotectEEPROM() {
	$DUDE -U hfuse:w:${HFUSE_UNPROTECTED_EEPROM}:m
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
	elif [ "$1" == "--dryRun" ]; then
		DRYRUN=1; shift;
	else
		echo "unknown options starting at $*"
		echo "e.g. $0 --port <programmerPort> --prog <programmer> --noBackup --noEEPROM --progOpts <programmer options> --setFuses --HW <typeOfHW> --addr <addr> --dryRun"
		echo "defaults: --port $PROGRAMMER_PORT --prog $PROGRAMMER --HW $BASENAME"
		exit 1
	fi
done

if [ "$BASENAME" == "hr20" ]; then
	PROC=m169
else
	PROC=m329p
fi

if [ "$EEPROM" == "1" ]; then
	WRITE_EEPROM="-e -U eeprom:w:${BASENAME}.eep"
else
	WRITE_EEPROM=
fi

BDIR=backup/${ADDR}/`date  "+%F_%T"`
DUDE="avrdude -p ${PROC} -c ${PROGRAMMER} -P ${PROGRAMMER_PORT} ${PROGRAMMER_OPTS}"

if [ "$DRYRUN" != "0" ]; then
	DUDE="echo DRY RUN: $DUDE"
fi

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
	$DUDE -U hfuse:w:${HFUSE_PROTECTED_EEPROM}:m -U lfuse:w:${LFUSE}:m -U efuse:w:${EFUSE}:m
	sleep 3
fi

[ "$EEPROM" == "1" ] && unprotectEEPROM

echo "*** writing openhr20 flash (and possibly eeprom)"
$DUDE -U flash:w:${BASENAME}.hex $WRITE_EEPROM

# if we wrote the eeprom, then protect the eeprom from erase next time
# so that we can just update the code without blowing away the eeprom

[ "$EEPROM" == "1" ] && protectEEPROM

echo "*** done!"
