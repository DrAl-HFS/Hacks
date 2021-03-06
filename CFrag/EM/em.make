# em.make - makefile for Expectation Maximisation test hacks
# https://github.com/DrAl-HFS/Hacks.git
# Licence: GPL V3
# (c) Project Contributors August-Sept 2020
# NB: Hashbang and make are poorly suited... !/usr/bin/env make

.PHONY : default shared all run clean


TARGET := em
CLI_TARGET := $(TARGET)
SHR_TARGET := $(TARGET).so
MAKEFILE := $(TARGET).make
SRC := em.c dump.c
HDR := em.h dump.h
CLI_SRC := $(SRC) emTest.c
CLI_HDR := $(HDR) emTest.h
SHR_SRC := $(SRC)
SHR_HDR := $(HDR)

CC := gcc -Wall
DBGDEF := -g -O0
OPTDEF := -O3 -march=native
SHRDEF := -fPIC -shared -DLIB_TARGET

INCDEF :=
#-I$(CMN_DIR)
LIBDEF := -lm

#OPT += -mfpu=neon
#OPT += -mfpu=neon-fp16 -mfp16-format=ieee
#OPT += -ffast-math

# Small source - full build every time
$(CLI_TARGET) : $(CLI_SRC) $(CLI_HDR) $(MAKEFILE)
	$(CC) $(DBGDEF) $(INCDEF) $(LIBDEF) $(CLI_SRC) -o $@

$(SHR_TARGET) : $(SHR_SRC) $(SHR_HDR) $(MAKEFILE)
	$(CC) $(OPTDEF) $(INCDEF) $(LIBDEF) $(SHRDEF) $(SHR_SRC) -o $@

#clean run all

default : $(CLI_TARGET)

shared : $(SHR_TARGET)

all : default shared

run : $(CLI_TARGET)
	./$(CLI_TARGET) testImg.png

clean :
	@rm -rf $(CLI_TARGET) $(SHR_TARGET) *.o *.dwf *.pdb prof
