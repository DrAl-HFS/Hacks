#!/usr/bin/env python3
# testMPSSE.py - hacky initial "howto?" test of libmpsse
# https://github.com/DrAl-HFS/Hacks.git
# Licence: GPL V3
# (c) Project Contributors August-Sept 2020

from mpsse import *
from time import sleep

#def CmdRead (busAddr,devReg):
#    print( type(busAddr), busAddr)
#    r= bytearray(2)
#    r[0]= 1 + (busAddr<<1) # 1+(2*busAddr)
#    r[1]= devReg
#    return bytes(r)

try:
    #adsRd0Cmd= CmdRead(b"\x48",b"\x00")
    #print("adsRd0Cmd[", len(adsRd0Cmd), "]:", hex(adsRd0Cmd))
    mpI2C= MPSSE(I2C, FOUR_HUNDRED_KHZ)

    print("%s initialized at %dHz (I2C)" % (mpI2C.GetDescription(), mpI2C.GetClock()))
    a= NACK
    na= 0
    for i in range(100):
       mpI2C.Start()
       mpI2C.Write( "\x91\x00" )
       a= mpI2C.GetAck()
       if ACK == a:
           na+= 1
           #mpI2C.Start()
           data = mpI2C.Read(2) # reg val
           print(len(data), ':', data)
       else:
           print("ERROR: Write, Ack ->", a, na)
       sleep(0.1)

    mpI2C.SendNacks()
    mpI2C.Stop()
    mpI2C.Close()

except ValueError as e:
    print("MPSSE ERROR:", e.args)

