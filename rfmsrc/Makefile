

all: rfm_int_sww rfm_int_hww rfm_ext_sww original_sww original_hww thermotronic_sww master1
clean:
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/rfm_int_sww/rfm_int_sww OBJDIR=rfm_int_sww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/rfm_int_hww/rfm_int_hww OBJDIR=rfm_int_hww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/rfm_ext_sww/rfm_ext_sww OBJDIR=rfm_ext_sww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/original_sww/original_sww OBJDIR=original_sww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/original_hww/original_hww OBJDIR=original_hww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/thermotronic_sww/thermotronic_sww OBJDIR=thermotronic_sww
	 $(MAKE) clean -C master TARGET=../$(DEST)/master1/master1 OBJDIR=master1


VER=

DEST=bin

rfm_int_sww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=0

rfm_int_hww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=1

rfm_ext_sww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=0\
		RFM=-DRFM_WIRE_MARIOJTAG=1
	 
original_sww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=0 \
		RFM=-DRFM=0

original_hww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=1 \
		RFM=-DRFM=0
		
thermotronic_sww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=0 \
		RFM=-DRFM=0 \
		HW=THERMOTRONIC

master1:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C master \
		TARGET=../$(DEST)/$@/master \
		OBJDIR=$@ 
		
