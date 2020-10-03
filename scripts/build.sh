#!/bin/bash

mkdir build 2>/dev/null

if [ "$1" == "clean" ]; then
	rm -fr build/* 2>/dev/null
fi

cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ../
make -j 1
