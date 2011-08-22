COMMON_DIR = ../common/src/

CXX = /usr/bin/g++
CXXFLAGS = -O3 -I${COMMON_DIR}

LIBS = -lGL -lglut -lGLEW -lIL -lILU

COMMON_OBJECTS = camera.o framebuffer.o matrix4x4.o pointgrid.o quad.o shader.o texture.o
LAB_OBJECTS = main.o

lab5: ${COMMON_OBJECTS} ${LAB_OBJECTS}
	${CXX} ${CXXFLAGS} ${LIBS} -o lab5 ${COMMON_OBJECTS} ${LAB_OBJECTS}

main.o: main.cpp
	${CXX} ${CXXFLAGS} -c -o main.o main.cpp

clean:
	rm -f *.o
	rm -f lab5

include ../common/src/common.mk
