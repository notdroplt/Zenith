#!/bin/env make

CXXFLAGS:= -O3 -Wall -Wextra -Werror -Wformat=2 -std=c++23 -pedantic -pedantic-errors 
LDFLAGS	:=
LDLIBS	:= -lm
CXX := g++
SRCS = $(wildcard *.cc) 
OBJS = $(SRCS:%.cc=%.o)

all: zenith

zenith: $(OBJS)
	$(CXX) -flto -o $@  $^ $(CFLAGS) $(LDLIBS)
	
.PHONY: clean

clean:
	$(RM) $(OBJS)