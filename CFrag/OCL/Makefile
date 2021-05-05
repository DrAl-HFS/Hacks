TARGET:= ocl1
MAKEFILE:=Makefile
SRC:= $(TARGET).c
OCL_VER:= 120

DEFINES:= -DCL_TARGET_OPENCL_VERSION=$(OCL_VER)
LIBS:= -lOpenCL -lm

all : $(TARGET) $(MAKEFILE)

$(TARGET) : $(SRC) $(MAKEFILE)
	clang $(SRC) $(DEFINES) $(LIBS) -o $@

run : $(TARGET)
	./$(TARGET)

clean :
	rm $(TARGET)
