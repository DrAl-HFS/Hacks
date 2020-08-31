# Makefile - ftdi test hacks
# https://github.com/DrAl-HFS/Hacks.git
# Licence: GPL V3
# (c) Project Contributors August-Sept 2020

.PHONY : run

CC := gcc -Wall
DBGDEF := -g -O0

SRC := ftTest.c
TARGET := ft
MAKEFILE := Makefile
LIBDEF := -lftdi1

$(TARGET) : $(SRC) $(MAKEFILE)
	$(CC) $(DBGDEF) $(SRC) $(LIBDEF) -o $(TARGET)

run : $(TARGET)
	./$(TARGET)

