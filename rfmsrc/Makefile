#############
# RFM settings - adjust as needed

# Enable diagnostic to fine tune RFM frequency
#HRFLAGS += -DRFM_TUNING=1

# Set default HR slave address. Needs to be >0, as 0 is master.
HRFLAGS += -DRFM_DEVICE_ADDRESS=29

# Set default security keys, shared by master and slave
HRFLAGS += -DSECURITY_KEY_0=0x01
HRFLAGS += -DSECURITY_KEY_1=0x23
HRFLAGS += -DSECURITY_KEY_2=0x45
HRFLAGS += -DSECURITY_KEY_3=0x67
HRFLAGS += -DSECURITY_KEY_4=0x89
HRFLAGS += -DSECURITY_KEY_5=0x01
HRFLAGS += -DSECURITY_KEY_6=0x23
HRFLAGS += -DSECURITY_KEY_7=0x45

# Main RFM frequency (depends on RFM variant)
HRFLAGS += -DRFM_FREQ_MAIN=433

# Decimal part of the RFM frequency
HRFLAGS += -DRFM_FREQ_FINE=0.35

# Enable slave programming also via buttons/wheel
#HRFLAGS += -DREMOTE_SETTING_ONLY=0

# Enable slave motor startup battery compensation
#HRFLAGS += -DMOTOR_COMPENSATE_BATTERY=1

#############

default: HR20_rfm_int_sww

all: HR20_rfm_int_sww HR20_rfm_int_hww HR20_rfm_ext_sww HR20_original_sww HR20_original_hww HR25_original_sww thermotronic_sww master1
	 cp OpenHR20/license.txt $(DEST)/

clean:
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/HR20_rfm_int_sww/hr20 OBJDIR=HR20_rfm_int_sww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/HR20_rfm_int_hww/hr20 OBJDIR=HR20_rfm_int_hww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/HR20_rfm_ext_sww/hr20 OBJDIR=HR20_rfm_ext_sww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/HR20_original_sww/hr20 OBJDIR=HR20_original_sww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/HR20_original_hww/hr20 OBJDIR=HR20_original_hww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/HR25_original_sww/hr20 OBJDIR=HR25_original_sww
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
		HRFLAGS='${HRFLAGS}' \
		REV=-DREVISION=\\\"$(REV)\\\"

HR20_rfm_int_hww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=1 \
		HRFLAGS='${HRFLAGS}' \
		REV=-DREVISION=\\\"$(REV)\\\"

HR20_rfm_ext_sww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=0\
		RFM=-DRFM_WIRE_MARIOJTAG=1 \
		HRFLAGS='${HRFLAGS}' \
		REV=-DREVISION=\\\"$(REV)\\\"
	 
HR20_original_sww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=0 \
		RFM=-DRFM=0 \
		HRFLAGS='${HRFLAGS}' \
		REV=-DREVISION=\\\"$(REV)\\\"

HR20_original_hww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=1 \
		RFM=-DRFM=0 \
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
		HRFLAGS='${HRFLAGS}' \
		REV=-DREVISION=\\\"$(REV)\\\"

master1:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C master \
		TARGET=../$(DEST)/$@/master \
		OBJDIR=$@ \
		HRFLAGS='${HRFLAGS}' \
		REV=-DREVISION=\\\"$(REV)\\\"
		
