CXX ?= g++
CXXFLAGS ?= -std=c++11
COMPILE.cc = ${CXX} ${CXXFLAGS} ${CPPFLAGS} -c

TARGET = main
SOURCES = $(wildcard [^_]*.cc)
OBJECTS = ${SOURCES:.cc=.o}

LD_LIBS_MACOS = libstem_gamepad/build/library/release-macosx/libstem_gamepad.a
LD_LIBS_LINUX = libstem_gamepad/build/library/release-linux64/libstem_gamepad.a
LD_FLAGS_MACOS = -framework IOKit -framework CoreFoundation
LD_FLAGS_LINUX = -lphread

UNAME = $(shell uname)
ifeq (${UNAME},Darwin)
  LD_FLAGS = ${LD_LIBS_MACOS} ${LD_FLAGS_MACOS}
endif
ifeq (${UNAME},Linux)
  LD_FLAGS = ${LD_LIBS_LINUX} ${LD_FLAGS_LINUX}
endif

%.o: %.cc
	${COMPILE.cc} -o $@ $<

all: ${OBJECTS}
	g++ -o ${TARGET} ${OBJECTS} ${GAMEPAD_LIB} ${LD_FLAGS}

clean:
	${RM} ${OBJECTS} ${TARGET}
