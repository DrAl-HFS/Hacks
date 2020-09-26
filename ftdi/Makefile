# Makefile - ftdi test hacks
# https://github.com/DrAl-HFS/Hacks.git
# Licence: GPL V3
# (c) Project Contributors August-Sept 2020

.PHONY : all run

CC := gcc -Wall
DBGDEF := -g -O0

MAKEFILE := Makefile

FTSRC := ftTest.c
FTLIBDEF := -lftdi1

MTSRC := i2cMPSSE.c
MTLIBDEF := -lmpsse


ft : $(FTSRC) $(MAKEFILE)
	$(CC) $(DBGDEF) $(FTSRC) $(FTLIBDEF) -o $@

mt : $(MTSRC) $(MAKEFILE)
	$(CC) $(DBGDEF) $(MTSRC) $(MTLIBDEF) -o $@

all : ft mt


RUN_TARGET := ft

run : $(RUN_TARGET)
	./$(RUN_TARGET)

