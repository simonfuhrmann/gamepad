CXX ?= g++
CXXFLAGS ?= -std=c++11
COMPILE.cc = ${CXX} ${CXXFLAGS} ${CPPFLAGS} -c

TARGET = main
SOURCES = $(wildcard [^_]*.cc)
OBJECTS = ${SOURCES:.cc=.o}

UNAME = $(shell uname)
ifeq (${UNAME},Darwin)
  C_FLAGS =
  LD_FLAGS = -framework IOKit -framework CoreFoundation
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

clean:
	${RM} ${OBJECTS} ${TARGET}
