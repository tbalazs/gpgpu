common: camera.o clwrapper.o matrix4x4.o

clwrapper.o: ${COMMON_DIR}/clwrapper.cpp ${COMMON_DIR}/clwrapper.hpp
	${CXX} ${CXXFLAGS} -c -o clwrapper.o ${COMMON_DIR}/clwrapper.cpp

camera.o: ${COMMON_DIR}/camera.cpp ${COMMON_DIR}/camera.hpp
	${CXX} ${CXXFLAGS} -c -o camera.o ${COMMON_DIR}/camera.cpp

matrix4x4.o: ${COMMON_DIR}/matrix4x4.cpp ${COMMON_DIR}/matrix4x4.hpp
	${CXX} ${CXXFLAGS} -c -o matrix4x4.o ${COMMON_DIR}/matrix4x4.cpp

