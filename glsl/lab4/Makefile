COMMON_DIR = ../common/src/

CXX = /usr/bin/g++
CXXFLAGS = -O3 -I${COMMON_DIR}

LIBS = -lGL -lglut -lGLEW -lIL -lILU

COMMON_OBJECTS = framebuffer.o pointgrid.o quad.o shader.o texture.o
LAB_OBJECTS = main.o

lab3: ${COMMON_OBJECTS} ${LAB_OBJECTS}
	${CXX} ${CXXFLAGS} ${LIBS} -o lab4 ${COMMON_OBJECTS} ${LAB_OBJECTS}

main.o: main.cpp
	${CXX} ${CXXFLAGS} -c -o main.o main.cpp

clean:
	rm -f *.o
	rm -f lab4

include ../common/src/common.mk
