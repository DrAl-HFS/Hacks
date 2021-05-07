CC:= clang
OPT:= -g
TARGET:= ocl1
MAKEFILE:=Makefile
SRC:= $(TARGET).cpp
HDR:=  $(shell ls *.hpp)
OCL_VER:= 120

DEFINES:= -DCL_TARGET_OPENCL_VERSION=$(OCL_VER) -DOPENCL_LIB_200
LIBS:= -lOpenCL -lm -lstdc++

all : $(TARGET) $(MAKEFILE)

$(TARGET) : $(SRC) $(HDR) $(MAKEFILE)
	$(CC) $(OPT) $(SRC) $(DEFINES) $(LIBS) -o $@

run : $(TARGET)
	./$(TARGET)

clean :
	rm $(TARGET)
