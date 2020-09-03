# Hacks/ftdi
Experiments with FTDI USB-serial/parallel bridge adaptors.

# Notes:
Unable to initialise EEPROM using /usr/bin/ftdi_eeprom. Got started
using some hacky DIY code instead. EEPROM seems to be a critical part
of MPSSE - looks like setup has to happen at boot time. This could be
explained by some form of PLD/FPGA within FTDI devices?

Despite lingering rumours, use of libftdi is completely unaffected by
kernel module ftdi_sio. The devices /dev/ttyUSB# simply disappear when
libftdi is used to take control of the relevant USB device endpoint.


# Additional setup & resources

(apt/yum/whatever) install libftdi1* ftdi-eeprom

# Additional resources:

 https://gist.github.com/bjornvaktaren/d2461738ec44e3ad8b3bae4ce69445b4
 https://www.linux.com/training-tutorials/linux-kernel-module-management-101

# Test HW:

FT232RL branded "Deek Robot", looks much like an old AdaFruit design.
FT2232H unbranded, features 74HC585 driving indicator LED's but serial
connection to the FT2232H is not visible.

# Testing

OK: uart-loop, bit-bang.

TODO: investigate MPSSE (for I2C, SPI, FIFO, JTAG) to beter understand
limitations and configuration issues. Found numerous copies of open source
MPSSE utility library in C with Python bindings. Some are many years out of
date but at least one seems to be actively maintained:-

 https://github.com/l29ah/libmpsse.git

 /+ Clear instructions in INSTALL.md
 /- Glitches in Python3 support. Fix:
     edit examples, bracketing "print", "exception" etc.
     verify/create folder "/usr/local/lib/python3.7/site-packages" (before
     executing "make install") then add path as necessary:
       e.g. "export PYTHONPATH=/usr/local/lib/python3.7/site-packages:$PYTHONPATH" (shell/login script)

