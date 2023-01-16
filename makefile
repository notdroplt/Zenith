# both gcc and cland work
CC = gcc
CXX = g++ 
CFLAGS :=-Wall -Wextra -Werror -pedantic -O3 -flto -march=native -mtune=native
CCFLAGS :=$(CFLAGS) -fno-rtti

CSOURCES := $(wildcard src/*.c) 
CCSOURCES := $(wildcard src/*.cc)
all: $(CSOURCES:src/%.c=build/%.o) $(CCSOURCES:src/%.cc=build/%.o)
	$(CXX) $(CFLAGS) -s -o build/zenith $^ 

build/%.o: src/%.c 
	$(CC) $(CFLAGS) -std=c17 -c $< -o $@

build/%.o: src/%.cc 
	$(CXX) $(CCFLAGS) --std=c++17 -c $< -o $@


.PHONY: clean setup docs

setup:
	$(MKDIR) build

docs: 
	doxygen

clean:
	rm build/* -rf
