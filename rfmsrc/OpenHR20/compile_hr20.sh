#!/bin/bash
# set slave (hr20's) address; must be >0

set -o nounset
set -o errexit

# defaults
ADDR=
REMOTE_ONLY=0
HW=HONEYWELL
KEY0=01
KEY1=23
KEY2=45
KEY3=67
KEY4=89
KEY5=01
KEY6=23
KEY7=45

while [ "$#" -ne 0 ]; do
	if [ "$1" == "--addr" ]; then
		ADDR=$2; shift; shift;
	elif [ "$1" == "--key" ]; then
		shift;
		KEY0=$1; KEY1=$2; KEY2=$3; KEY3=$4;
		KEY4=$5; KEY5=$6; KEY6=$7; KEY7=$8;
		shift; shift; shift; shift;
		shift; shift; shift; shift;
	elif [ "$1" == "--HW" ] || [ "$1" == "--hw" ]; then
		HW=$2; shift; shift;
	elif [ "$1" == "--remoteOnly" ]; then
		REMOTE_ONLY=1; shift;
    else
		echo "unknown options starting at $*"
		echo "e.g. $0 --key <k0> <k1> <k2> <k3> <k4> <k5> <k6> <k7>] [--HW HR20|HR25|...] [--addr <addr>] [-remoteOnly]"
		echo "defaults: --key $KEY0 $KEY1 $KEY2 $KEY3 $KEY4 $KEY5 $KEY6 $KEY7 --HW $HW --addr $ADDR"
		exit 1
	fi
done

if [ -z "$ADDR" ]; then
	echo "Must set --addr <addr>. Use -h for help"
	exit 1;
fi

#-DRFM_TUNING=1 \
make clean HW=$HW
make RFM="\
-DRFM=1 \
-DRFM_TUNING=1 \
-DRFM_FREQ=868 \
-DRFM_BAUD_RATE=9600 \
-DSECURITY_KEY_0=0x${KEY0} \
-DSECURITY_KEY_1=0x${KEY1} \
-DSECURITY_KEY_2=0x${KEY2} \
-DSECURITY_KEY_3=0x${KEY3} \
-DSECURITY_KEY_4=0x${KEY4} \
-DSECURITY_KEY_5=0x${KEY5} \
-DSECURITY_KEY_6=0x${KEY6} \
-DSECURITY_KEY_7=0x${KEY7} \
-DRFM_DEVICE_ADDRESS=${ADDR} \
-DRFM_WIRE_MARIOJTAG=1" \
HW=${HW} \
REMOTE_SETTING_ONLY=-DREMOTE_SETTING_ONLY=${REMOTE_ONLY} \
MOTOR_COMPENSATE_BATTERY=-DMOTOR_COMPENSATE_BATTERY=1

