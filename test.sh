#!/usr/bin/env bash

mkdir -p ./build
cd ./build || exit
cmake ..
make
cd .. || exit
./build/errors
