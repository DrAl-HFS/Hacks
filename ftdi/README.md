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

"Deek Robot" (?) FT232RL, unbranded FT2232H from HK seller.

# Testing

OK: uart-loop, bit-bang.

TODO: investigate MPSSE (for I2C, SPI, FIFO, JTAG) to beter understand
limitations and configuration issues. Found existing MPSSE utility library
(in C with python bindings) but haven't properly evaluated yet:-

 https://github.com/SjB/libmpsse
