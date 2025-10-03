#!/bin/bash

echo "Build started: $(date '+%Y-%m-%d %H:%M:%S')"

rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=/Users/niravdd/Qt/6.5.3/macos
cmake --build .

echo "Build completed: $(date '+%Y-%m-%d %H:%M:%S')"
