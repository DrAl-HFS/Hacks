#!/usr/bin/env python3
# testMPSSE.py - hacky initial "howto?" test of libmpsse
# https://github.com/DrAl-HFS/Hacks.git
# Licence: GPL V3
# (c) Project Contributors August-Sept 2020

from mpsse import *

try:
    mpI2C= MPSSE(I2C, FOUR_HUNDRED_KHZ)

    print("%s initialized at %dHz (I2C)" % (mpI2C.GetDescription(), mpI2C.GetClock()))
    a= NACK
    na= 0
    mpI2C.Start()
    mpI2C.Write("\x48"*2) # dev addr
    a= mpI2C.GetAck()
    if ACK == a:
        na+= 1
        mpI2C.Write("\x00") # dev reg
        a= mpI2C.GetAck()
        if ACK == a:
            na+= 1
            data = mpI2C.Read(2) # reg val
            print(len(data), ':', data)
    else:
        print("ERROR: Write, Ack ->", a, na)

    mpI2C.SendNacks()
    mpI2C.Stop()
    mpI2C.Close()

except ValueError as e:
    print("MPSSE ERROR:", e.args)

