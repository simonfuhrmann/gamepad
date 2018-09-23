CXX ?= g++
CXXFLAGS ?= -std=c++11
COMPILE.cc = ${CXX} ${CXXFLAGS} ${CPPFLAGS} -c

TARGET = main
SOURCES = $(filter-out main_libstem.cc,$(wildcard [^_]*.cc))
OBJECTS = ${SOURCES:.cc=.o}

LD_LIBS_MACOS = libstem_gamepad/build/library/release-macosx/libstem_gamepad.a
LD_LIBS_LINUX = libstem_gamepad/build/library/release-linux64/libstem_gamepad.a
LD_FLAGS_MACOS = -framework IOKit -framework CoreFoundation
LD_FLAGS_LINUX = -lpthread

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
	g++ -o ${TARGET} ${OBJECTS} ${LD_FLAGS}

# Debug target that only uses the original library.
libstem: main_libstem.o
	g++ -o main_libstem main_libstem.o ${LD_FLAGS}

clean:
	${RM} ${OBJECTS} ${TARGET}
