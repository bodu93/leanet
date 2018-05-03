cmake_minimum_required(VERSION 2.6)

project(leanet C CXX)

enable_testing()

set(CXX_FLAGS
	-g
	-Wall
	-Wextra
	-Werror
	-Wconversion
	-Wno-unused-parameter
	-Wconversion
	-Wold-style-cast
	-Woverloaded-virtual
	-Wpointer-arith
	# -Wshadow
	-Wwrite-strings
	-std=c++11
	-DLEANET_STD_STRING
	-DCHECK_PTHREAD_RETURN_VALUE
	-D_FILE_OFFSET_BITS=64
	)

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_subdirectory(src/leanet)
# add_subdirectory(src/test)
