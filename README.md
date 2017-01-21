svn-openhr20
============

Clone of openhr20.svn.sourceforge.net.
branches: master-svn - pull from http://openhr20.svn.sourceforge.net
          master - main 'release' branch


Compile this branch without wireless extension:
cd OpenHR20
make RFM=0

Compile with predefined REVision ID
cd OpenHR20
make REV=-DREVISION=\\\"123456_XYZ\\\"

compile with hardware window open contact
cd OpenHR20
make HW_WINDOW_DETECTION=1

thermotronic HW
cd OpenHR20
make HW=THERMOTRONIC
