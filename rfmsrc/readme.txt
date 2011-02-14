

Compile this branch without wireless extension:
cd OpenHR20
make RFM=-DRFM=0

Compile with predefined REVision ID
cd OpenHR20
make REV=-DREVISION=\\\"123456_XYZ\\\"

compile with hardware window open contact
cd OpenHR20
make HW_WINDOW_DETECTION=-DHW_WINDOW_DETECTION=1

thermotronic HW
cd OpenHR20
make HW=THERMOTRONIC
