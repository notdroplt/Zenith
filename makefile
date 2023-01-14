# both gcc and cland work
CC = gcc
CXX = g++ 
CFLAGS :=-Wall -Wextra -Werror -pedantic -g3
CCFLAGS :=$(CFLAGS) -fno-rtti

CSOURCES := $(wildcard src/*.c) 
CCSOURCES := $(wildcard src/*.cc)
all: $(CSOURCES:src/%.c=build/%.o) $(CCSOURCES:src/%.cc=build/%.o)
	$(CXX) $(CFLAGS) -o build/zenith $^ 

build/%.o: src/%.c 
	$(CC) $(CFLAGS) --std=c2x -c $< -o $@

build/%.o: src/%.cc 
	$(CXX) $(CCFLAGS) --std=c++17 -c $< -o $@


.PHONY: clean setup docs

setup:
	$(MKDIR) build

docs: 
	doxygen

clean:
	rm build/* -rf
