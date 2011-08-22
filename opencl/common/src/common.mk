common: clwrapper.o

clwrapper.o: ${COMMON_DIR}/clwrapper.cpp ${COMMON_DIR}/clwrapper.hpp
	${CXX} ${CXXFLAGS} -c -o clwrapper.o ${COMMON_DIR}/clwrapper.cpp


