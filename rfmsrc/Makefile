#############
# Thermostat settings

# Enable slave programming also via buttons/wheel
#HRFLAGS += -DREMOTE_SETTING_ONLY=0

# Enable slave motor startup battery compensation
#HRFLAGS += -DMOTOR_COMPENSATE_BATTERY=1

#############
# Master settings

# Set the proper master type
#MASTERTYPE=JEENODE=1
#MASTERTYPE=NANODE=1

#############
# RFM settings

# Enable diagnostic to fine tune RFM frequency
#RFMFLAGS += -DRFM_TUNING=1

# Set default HR slave address, valid range is 1-28
# Do not set on master, thus use HRFLAGS
HRFLAGS += -DRFM_DEVICE_ADDRESS=28

# Set default security keys, shared by master and slave
RFMFLAGS += -DSECURITY_KEY_0=0x01
RFMFLAGS += -DSECURITY_KEY_1=0x23
RFMFLAGS += -DSECURITY_KEY_2=0x45
RFMFLAGS += -DSECURITY_KEY_3=0x67
RFMFLAGS += -DSECURITY_KEY_4=0x89
RFMFLAGS += -DSECURITY_KEY_5=0x01
RFMFLAGS += -DSECURITY_KEY_6=0x23
RFMFLAGS += -DSECURITY_KEY_7=0x45

# Main RFM frequency (depends on RFM variant)
RFMFLAGS += -DRFM_FREQ_MAIN=868

# Decimal part of the RFM frequency
RFMFLAGS += -DRFM_FREQ_FINE=0.35

#############

default: HR20_rfm_int_sww master1

all: HR20_rfm_int_sww HR20_rfm_int_hww HR20_rfm_ext_sww HR20_original_sww HR20_original_hww HR25_original_sww HR25_rfm_int_sww thermotronic_sww master1
	 cp OpenHR20/license.txt $(DEST)/

clean:
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/HR20_rfm_int_sww/hr20 OBJDIR=HR20_rfm_int_sww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/HR20_rfm_int_hww/hr20 OBJDIR=HR20_rfm_int_hww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/HR20_rfm_ext_sww/hr20 OBJDIR=HR20_rfm_ext_sww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/HR20_original_sww/hr20 OBJDIR=HR20_original_sww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/HR20_original_hww/hr20 OBJDIR=HR20_original_hww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/HR25_original_sww/hr20 OBJDIR=HR25_original_sww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/HR25_rfm_int_sww/hr20 OBJDIR=HR25_rfm_int_sww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/thermotronic_sww/hr20 OBJDIR=thermotronic_sww
	 $(MAKE) clean -C master TARGET=../$(DEST)/master1/master OBJDIR=master1


VER=

DEST=bin

HR20_rfm_int_sww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=0 \
		RFMFLAGS='${RFMFLAGS}' \
		HRFLAGS='${HRFLAGS}' \
		REV=-DREVISION=\\\"$(REV)\\\"

HR20_rfm_int_hww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=1 \
		RFMFLAGS='${RFMFLAGS}' \
		HRFLAGS='${HRFLAGS}' \
		REV=-DREVISION=\\\"$(REV)\\\"

HR20_rfm_ext_sww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=0\
		RFM=-DRFM_WIRE_MARIOJTAG=1 \
		RFMFLAGS='${RFMFLAGS}' \
		HRFLAGS='${HRFLAGS}' \
		REV=-DREVISION=\\\"$(REV)\\\"
	 
HR20_original_sww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=0 \
		RFM=-DRFM=0 \
		RFMFLAGS='${RFMFLAGS}' \
		HRFLAGS='${HRFLAGS}' \
		REV=-DREVISION=\\\"$(REV)\\\"

HR20_original_hww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=1 \
		RFM=-DRFM=0 \
		RFMFLAGS='${RFMFLAGS}' \
		HRFLAGS='${HRFLAGS}' \
		REV=-DREVISION=\\\"$(REV)\\\"

HR25_original_sww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=0 \
		RFM=-DRFM=0 \
		HW=HR25 \
		RFMFLAGS='${RFMFLAGS}' \
		HRFLAGS='${HRFLAGS}' \
		REV=-DREVISION=\\\"$(REV)\\\"

HR25_rfm_int_sww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=0 \
		RFM=-DRFM_WIRE_TK_INTERNAL=1 \
		HW=HR25 \
		RFMFLAGS='${RFMFLAGS}' \
		HRFLAGS='${HRFLAGS}' \
		REV=-DREVISION=\\\"$(REV)\\\"

thermotronic_sww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=0 \
		RFM=-DRFM=0 \
		HW=THERMOTRONIC \
		RFMFLAGS='${RFMFLAGS}' \
		HRFLAGS='${HRFLAGS}' \
		REV=-DREVISION=\\\"$(REV)\\\"

master1:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C master \
		TARGET=../$(DEST)/$@/master \
		OBJDIR=$@ \
		RFMFLAGS='${RFMFLAGS}' \
		${MASTERTYPE} \
		REV=-DREVISION=\\\"$(REV)\\\"
