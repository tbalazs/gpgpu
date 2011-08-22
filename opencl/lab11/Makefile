COMMON_DIR = ../common/src/
OPENCL_DIR = /usr/local/cuda/include/

CXX = /usr/bin/g++
CXXFLAGS = -O0 -g -I${COMMON_DIR} -I${OPENCL_DIR}

LIBS = -lOpenCL -lGL -lglut -lGLEW

COMMON_OBJECTS = camera.o clwrapper.o matrix4x4.o
LAB_OBJECTS = main.o

lab11: ${COMMON_OBJECTS} ${LAB_OBJECTS}
	${CXX} ${CXXFLAGS} ${LIBS} -o lab11 ${COMMON_OBJECTS} ${LAB_OBJECTS}

main.o: main.cpp
	${CXX} ${CXXFLAGS} -c -o main.o main.cpp

clean:
	rm -f *.o
	rm -f *.txt
	rm -f lab11

include ../common/src/common.mk
