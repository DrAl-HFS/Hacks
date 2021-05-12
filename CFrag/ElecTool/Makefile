# electools
BUILD := FLLSRC

#CC := clang-6.0
# -std=c11 -Wall -g
CC := gcc -Wall -g -march=native
# -std=c11 -D__USE_MISC
# -pedantic

TARGET := tlcr

SRC_DIR := src
HDR_DIR := $(SRC_DIR)
CMN_DIR := Common/src
SRC := $(shell ls $(SRC_DIR)/*.c)
HDR := $(shell ls $(HDR_DIR)/*.h)
CMN_SRC := $(CMN_DIR)/sciFmt.c

#OBJ_DIR := obj
#OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)


LIBS := -lm
INCDEF := -I$(CMN_DIR) -DPERMIT_ELEC_FMT

ifeq ($(BUILD),FLLSRC)
# Full build from source every time
# $(HDR)
$(TARGET) : $(SRC) $(CMN_SRC)
	$(CC) $(INCDEF) $(LIBS) $^ -o $@
	@echo ---
	./$@

else

%.o : $(SRC_DIR)/%.c $(HDR_DIR)/%.h
	$(CC) $(DEFINES) $< -c

$(OBJ_DIR)/%.o : %.o
	mv $< $@

$(TARGET) : $(OBJ)
	$(CC) $(LIBS) $^ -o $@
	@echo ---
	./$@

endif
