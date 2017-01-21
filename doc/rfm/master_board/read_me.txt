This is the master hardware for the OpenHR20.
It's only needed if you want to use the OpenHR20s with RFM12B wireless modules.

Populate L1 only if you do not use USB and the FTDI converter! Otherwise you would shorten +5V USB supply 
and +3.3V from the FTDI internal regulator.

R4 is not needed with a RFM12B, only with RFM12. But a RFM12 might not be compatible with actual firmware!




Changelog:
v0.6 (2010-02-07)
- used FTDI internal voltage regulator for 3.3V VCC
a patch for v0.5 can be found at http://embdev.net/topic/158895#1581675

v0.5 (2010-01-05)
- first public version




RFM12 eagle library
http://www.mikrocontroller.net/attachment/30893/RFM12.lbr