
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

