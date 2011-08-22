common: framebuffer.o quad.o shader.o texture.o

framebuffer.o: ${COMMON_DIR}/framebuffer.cpp ${COMMON_DIR}/framebuffer.hpp
	${CXX} ${CXXFLAGS} -c -o framebuffer.o ${COMMON_DIR}/framebuffer.cpp

quad.o: ${COMMON_DIR}/quad.cpp ${COMMON_DIR}/quad.hpp
	${CXX} ${CXXFLAGS} -c -o quad.o ${COMMON_DIR}/quad.cpp

shader.o: ${COMMON_DIR}/shader.cpp ${COMMON_DIR}/shader.hpp
	${CXX} ${CXXFLAGS} -c -o shader.o ${COMMON_DIR}/shader.cpp

texture.o: ${COMMON_DIR}/texture.cpp ${COMMON_DIR}/texture.hpp
	${CXX} ${CXXFLAGS} -c -o texture.o ${COMMON_DIR}/texture.cpp
