TARGET:= ocl1
SRC:= $(TARGET).c
OCL_VER:= 110

DEFINES:= -DCL_TARGET_OPENCL_VERSION=$(OCL_VER)
LIBS:= -lOpenCL

all : $(TARGET)

$(TARGET) : $(SRC)
	gcc $< $(DEFINES) $(LIBS) -o $@

run : $(TARGET)
	./$(TARGET)
