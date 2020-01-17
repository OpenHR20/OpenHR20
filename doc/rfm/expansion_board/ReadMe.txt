Information about wireless "extension" board:

- ATMEGA16 / ATMEGA32 CPU depend to destination
- RFM12b

- optional SHT21 humidity/temperature sensor
- optional DS18S20/DS18B20 temperature sensor
- optional FTDI FT232RL USB, this PCB can act as master board
- optional LE33 stabilizer (3v3 power from USB, optional if 50mA from
FTDI is not sufficient or for VUSB)
- optional VUSB interface (http://www.obdev.at/products/vusb/index.html)
- optional I2C EEPROM (24c02 - 24c1024)
- optional LCD from Nokia 3310 with optional backlight (LCD is low
power, visible without back light. Destination for show temperature or
status)
- optional power from 2xAA or 1xLipol
- it have I2C and SPI on connector
- 7 A/D free lines in connector or 7 digital I/O
- PCB size 50x50mm


NOTE: this board have RFM22/23 option, but it can't be RFM12 compatible
on wireless, it is for another project.

