.PHONY: all indent clean check

CPP=g++
CPPFLAGS=-std=c++11 -g
LDFLAGS=-pthread

PWD=$(shell pwd)
SRC=$(wildcard *.cc)
BIN=$(SRC:%.cc=build/%)

all: build ${BIN}

indent:
	find . -regextype posix-egrep -regex ".*\.(cc|h)" | xargs clang-format-3.5 -i

clean:
	rm -rf build

check: build build/CHECK_OK

build/CHECK_OK: build *.h
	for i in *.h ; do \
		echo -n $(basename $$i)': ' ; \
		ln -sf ${PWD}/$$i ${PWD}/build/$$i.cc ; \
		if [ ! -f build/$$i.h.o -o build/$$i.h.cc -nt build/$$i.h.o ] ; then \
			${CPP} -I . ${CPPFLAGS} -c build/$$i.cc -o build/$$i.h.o ${LDFLAGS} || exit 1 ; echo 'OK' ; \
		else \
			echo 'Already OK' ; \
		fi \
	done && echo OK >$@

build:
	mkdir -p build

build/%: %.cc *.h
	${CPP} ${CPPFLAGS} -o $@ ${LDFLAGS} $<
