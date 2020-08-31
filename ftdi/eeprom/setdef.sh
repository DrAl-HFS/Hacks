# setdef.sh - set default info on FT232H device EEPROM
modinfo ftdi-sio
ftdi_eeprom --flash-eeprom default.cfg
