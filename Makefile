CXX ?= g++
CXXFLAGS ?= -std=c++11
COMPILE.cc = ${CXX} ${CXXFLAGS} ${CPPFLAGS} -c

TARGET = main
SOURCES = $(filter-out main_libstem.cc,$(wildcard [^_]*.cc))
OBJECTS = ${SOURCES:.cc=.o}

UNAME = $(shell uname)
ifeq (${UNAME},Darwin)
  LD_FLAGS_LIBSTEM = libstem_gamepad/build/library/release-macosx/libstem_gamepad.a

  C_FLAGS =
  LD_FLAGS = -framework IOKit -framework CoreFoundation ${LD_FLAGS_LIBSTEM}
endif
ifeq (${UNAME},Linux)
  CFLAGS_EVDEV = $(shell pkg-config --cflags libevdev)
  LD_FLAGS_EVDEV = $(shell pkg-config --libs libevdev)

  C_FLAGS = ${CFLAGS_EVDEV}
  LD_FLAGS = -lpthread ${LD_FLAGS_EVDEV}
endif

%.o: %.cc
	${COMPILE.cc} -o $@ ${C_FLAGS} $<

all: ${OBJECTS}
	g++ -o ${TARGET} ${OBJECTS} ${LD_FLAGS}

# Debug target that only uses libstem_gamepad
libstem: main_libstem.o
	g++ -o main_libstem main_libstem.o ${LD_FLAGS}

clean:
	${RM} ${OBJECTS} ${TARGET}
