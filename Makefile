CXX ?= g++
CXXFLAGS ?= -std=c++11
COMPILE.cc = ${CXX} ${CXXFLAGS} ${CPPFLAGS} -c

TARGET = main
SOURCES = $(wildcard [^_]*.cc)
OBJECTS = ${SOURCES:.cc=.o}

C_FLAGS = 
LD_FLAGS =

UNAME = $(shell uname)
ifeq (${UNAME},Darwin)
  LD_FLAGS += -framework IOKit -framework CoreFoundation
endif
ifeq (${UNAME},Linux)
  C_FLAGS += $(shell pkg-config --cflags libevdev)
  LD_FLAGS = $(shell pkg-config --libs libevdev)
endif

%.o: %.cc
	${COMPILE.cc} -o $@ ${C_FLAGS} $<

all: ${OBJECTS}
	${CXX} -o ${TARGET} ${OBJECTS} ${LD_FLAGS}

clean:
	${RM} ${OBJECTS} ${TARGET}
