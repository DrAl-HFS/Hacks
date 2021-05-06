TARGET:= ocl1
MAKEFILE:=Makefile
SRC:= $(TARGET).cpp
OCL_VER:= 120

DEFINES:= -DCL_TARGET_OPENCL_VERSION=$(OCL_VER)
LIBS:= -lOpenCL -lm -lstdc++

all : $(TARGET) $(MAKEFILE)

$(TARGET) : $(SRC) $(MAKEFILE)
	clang $(SRC) $(DEFINES) $(LIBS) -o $@

run : $(TARGET)
	./$(TARGET)

clean :
	rm $(TARGET)
