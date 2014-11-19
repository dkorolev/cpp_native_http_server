.PHONY: all indent clean

CPP=g++
CPPFLAGS=-std=c++11 -g
LDFLAGS=

SRC=$(wildcard *.cc)
BIN=$(SRC:%.cc=build/%)

all: build ${BIN}

indent:
	find . -regextype posix-egrep -regex ".*\.(cc|h)" | xargs clang-format-3.5 -i

clean:
	rm -rf build

build:
	mkdir -p build

build/%: %.cc
	${CPP} ${CPPFLAGS} -o $@ $<
