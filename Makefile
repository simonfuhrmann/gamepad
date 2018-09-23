CXX ?= g++
COMPILE.cc = ${CXX} ${CXXFLAGS} ${CPPFLAGS} -c

TARGET = main
SOURCES = $(wildcard [^_]*.cc)
OBJECTS = ${SOURCES:.cc=.o}
LIBS = -lpthread
GCC_FLAGS = -std=c++11

GAMEPAD_LIB = libstem_gamepad/build/library/release-linux64/libstem_gamepad.a

%.o: %.cc
	${COMPILE.cc} ${GCC_FLAGS} -o $@ $<

all: ${OBJECTS}
	g++ -o ${TARGET} ${OBJECTS} ${GAMEPAD_LIB} ${LIBS}

clean:
	${RM} ${OBJECTS} ${TARGET}
