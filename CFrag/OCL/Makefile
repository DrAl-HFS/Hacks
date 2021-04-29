TARGET:= ocl1
MAKEFILE:=Makefile
SRC:= $(TARGET).c
OCL_VER:= 110

DEFINES:= -DCL_TARGET_OPENCL_VERSION=$(OCL_VER)
LIBS:= -lOpenCL

all : $(TARGET) $(MAKEFILE)

$(TARGET) : $(SRC)
	clang $< $(DEFINES) $(LIBS) -o $@

run : $(TARGET)
	./$(TARGET)

clean :
	rm $(TARGET)
