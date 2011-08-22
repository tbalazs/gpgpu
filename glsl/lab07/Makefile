COMMON_DIR = ../common/src/

CXX = /usr/bin/g++
CXXFLAGS = -O3 -I${COMMON_DIR}

LIBS = -lGL -lglut -lGLEW -lIL -lILU

COMMON_OBJECTS = framebuffer.o pointgrid.o quad.o shader.o texture.o
LAB_OBJECTS = main.o

lab7: ${COMMON_OBJECTS} ${LAB_OBJECTS}
	${CXX} ${CXXFLAGS} ${LIBS} -o lab7 ${COMMON_OBJECTS} ${LAB_OBJECTS}

main.o: main.cpp
	${CXX} ${CXXFLAGS} -c -o main.o main.cpp

clean:
	rm -f *.o
	rm -f *.txt
	rm -f lab7

include ../common/src/common.mk
