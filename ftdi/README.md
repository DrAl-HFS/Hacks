# Hacks/ftdi
Experiments with FTDI USB-serial/parallel bridge adaptors.

# Notes:
Unable to initialise EEPROM using /usr/bin/ftdi_eeprom. Got started
using some hacky DIY code instead. Looks like EEPROM can be used to
configure MPSSE features at boot time, and seems to be used for app
key data e.g. on commercial FPGA toolkit. Perhaps useful to understand
this better?

Despite lingering rumours, use of libftdi is completely unaffected by
kernel module ftdi_sio. The devices /dev/ttyUSB# simply disappear when
libftdi is used to take control of the relevant USB device endpoint.


# Additional setup

(apt/yum/whatever) install libftdi1* ftdi-eeprom

# Additional resources:

 https://gist.github.com/bjornvaktaren/d2461738ec44e3ad8b3bae4ce69445b4
 https://www.linux.com/training-tutorials/linux-kernel-module-management-101

# Test HW:

FT232RL branded "Deek Robot", looks much like an old Adafruit design.

FT2232H unbranded, features 74HC585 driving indicator LED's but connection
to main chip (FT2232H) is obscured.

# Testing

OK: uart-loop, bit-bang.

TODO: investigate MPSSE (for I2C, SPI, FIFO, JTAG) to better understand
limitations and configuration issues.


# Existing Library

Found numerous copies of open source MPSSE utility library in C with
Python bindings. Some are many years out of date but at least one seems
to be actively maintained:-

 https://github.com/l29ah/libmpsse.git

 \+ Clear instructions in INSTALL.md

 \- Glitches in Python3 support. Fix:
 
     * edit examples: "print -> print()", "Exception -> ValueError()" etc.
     
     * verify/create folder "/usr/local/lib/python3.7/site-packages" (before
       executing "make install") then add path as necessary:
       e.g. "export PYTHONPATH=/usr/local/lib/python3.7/site-packages:$PYTHONPATH" (shell/login script)

