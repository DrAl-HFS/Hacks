#!/usr/bin/env make

.PHONY : default shared

TARGET := em
SHR_TARGET := $(TARGET).so
MAKEFILE := $(TARGET).make
SRC := em.c

CC := gcc -Wall
OPTDEF := -O3 -march=native
SHRDEF := -fPIC -shared -DLIB_TARGET

INCDEF :=
#-I$(CMN_DIR)
LIBDEF := -lm

#NOPT += -mfpu=neon
#NOPT += -mfpu=neon-fp16 -mfp16-format=ieee
#NOPT += -ffast-math

# Full build from source every time $(MAKEFILE)
$(TARGET) : $(SRC)
	$(CC) $(OPTDEF) $(INCDEF) $(LIBDEF) $^ -o $@

$(SHR_TARGET) : $(SRC)
	$(CC) $(OPTDEF) $(INCDEF) $(LIBDEF) $(SHRDEF) $^ -o $@

#clean run all

default : $(TARGET)

shared : $(SHR_TARGET)

