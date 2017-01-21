svn-openhr20
============

Clone of openhr20.svn.sourceforge.net.
branches: master-svn - pull from http://openhr20.svn.sourceforge.net
          master - main 'release' branch


Compile this branch without wireless extension:
make RFM=0

Compile with predefined REVision ID
make REV=-DREVISION=\\\"123456_XYZ\\\"

compile with hardware window open contact
make HW_WINDOW_DETECTION=1

thermotronic HW
make HW=THERMOTRONIC
