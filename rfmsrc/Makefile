

all: rfm_int_sww rfm_int_hww rfm_ext_sww original_sww original_hww thermotronic_sww master1
	 cp OpenHR20/license.txt $(DEST)/

clean:
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/rfm_int_sww/hr20 OBJDIR=rfm_int_sww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/rfm_int_hww/hr20 OBJDIR=rfm_int_hww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/rfm_ext_sww/hr20 OBJDIR=rfm_ext_sww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/original_sww/hr20 OBJDIR=original_sww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/original_hww/hr20 OBJDIR=original_hww
	 $(MAKE) clean -C OpenHR20 TARGET=../$(DEST)/thermotronic_sww/hr20 OBJDIR=thermotronic_sww
	 $(MAKE) clean -C master TARGET=../$(DEST)/master1/master OBJDIR=master1


VER=

DEST=bin

rfm_int_sww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=0 \
		REV=-DREVISION=\\\"$(REV)\\\"

rfm_int_hww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=1 \
		REV=-DREVISION=\\\"$(REV)\\\"

rfm_ext_sww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=0\
		RFM=-DRFM_WIRE_MARIOJTAG=1 \
		REV=-DREVISION=\\\"$(REV)\\\"
	 
original_sww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=0 \
		RFM=-DRFM=0 \
		REV=-DREVISION=\\\"$(REV)\\\"

original_hww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=1 \
		RFM=-DRFM=0 \
		REV=-DREVISION=\\\"$(REV)\\\"
		
thermotronic_sww:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C OpenHR20 \
		TARGET=../$(DEST)/$@/hr20 \
		OBJDIR=$@ \
		HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=0 \
		RFM=-DRFM=0 \
		HW=THERMOTRONIC \
		REV=-DREVISION=\\\"$(REV)\\\"

master1:
	 $(shell mkdir $(DEST)/$@ 2>/dev/null)
	 $(MAKE) -C master \
		TARGET=../$(DEST)/$@/master \
		OBJDIR=$@ \
		REV=-DREVISION=\\\"$(REV)\\\"
		
