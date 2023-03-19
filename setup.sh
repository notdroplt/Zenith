#!/usr/bin/env bash

function check_exec {
	local color_red='\033[0;31m'
	local color_green='\033[0;32m'
	local color_normal='\033[0m'
	echo -n " - $1: "
	if command -v $1 &> /dev/null; then
		printf "${color_green}found\n"
	else
		printf "${color_red}not found\n"
	fi
	printf $color_normal
}

if [ -z $1 ]; then
	echo "small setup script for Zenith, options are:"
	echo " - '$0 check' to check for build and other dependencies"
	echo " - '$0 config' to generate project setup"
	echo " - '$0 clean' to clean builds"
	echo " - '$0 build' to build the project"
	echo " - '$0 test' to test the project"
	echo " - '$0 run' to run the compiled file"
	exit 0
fi
while [ -n $1 ]; do
	[[ -n $1 ]] || break
	if [ $1 = "check" ]; then
		check_exec "gcc"
		check_exec "clang"
		check_exec "cmake"
		check_exec "ctest"
	fi
	
	if [ $1 = "config" ]; then
		cmake -B ./build
	fi
	
	if [ $1 = "clean" ]; then
		cmake --build ./build --target clean
	fi
	
	if [ $1 = "build" ]; then
		cmake --build ./build
	fi
	
	if [ $1 = "test" ]; then
		cd build && ctest
	fi

	if [ $1 = "run" ]; then 
		./build/zenith
	fi
	shift 1
done

