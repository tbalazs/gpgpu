COMMON_DIR = ../common/src/
OPENCL_DIR = /usr/local/cuda/include/

CXX = /usr/bin/g++
CXXFLAGS = -O0 -g -I${COMMON_DIR} -I${OPENCL_DIR}

LIBS = -lOpenCL

COMMON_OBJECTS = clwrapper.o
LAB_OBJECTS = main.o

lab13: ${COMMON_OBJECTS} ${LAB_OBJECTS}
	${CXX} ${CXXFLAGS} ${LIBS} -std=c++11 -o lab15 ${COMMON_OBJECTS} ${LAB_OBJECTS}

main.o: main.cpp
	${CXX} ${CXXFLAGS} -std=c++11 -c -o main.o main.cpp

clean:
	rm -f *.o
	rm -f *.txt
	rm -f lab15

include ../common/src/common.mk
