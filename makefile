CXX = clang++

DEBUG := -O3
CFLAGS := -Wall -Wextra -Werror -Wdocumentation -pedantic --std=c++17 $(DEBUG) -fno-rtti

CCSOURCES := $(wildcard src/*.cc)

all: $(CSOURCES:src/%.c=build/%.o) $(CCSOURCES:src/%.cc=build/%.o)
	$(CXX) $(CFLAGS) -s -flto -o build/zenith $^ 

build/%.o: src/%.cc 
	$(CXX) $(CFLAGS) -c $< -o $@

.PHONY: clean setup docs

setup:
	$(MKDIR) build
	$(MKDIR) docs

docs: 
	doxygen

clean:
	rm build/* -rf
