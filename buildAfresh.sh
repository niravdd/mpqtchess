#!/bin/bash
# Script to perform a clean build of the chess project

echo "== Complete cleanup =="
rm -rf build CMakeCache.txt CMakeFiles
find . -name "*.autogen" -type d -exec rm -rf {} + 2>/dev/null || true
find . -name "moc_*" -exec rm -f {} + 2>/dev/null || true

echo "== Creating fresh build directory =="
mkdir -p build
cd build

echo "== Configuring project =="
cmake -DCMAKE_PREFIX_PATH=/Users/niravdd/Qt/6.5.3/macos \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      ..

echo "== Building project =="
cmake --build . -j 8
mkdir ../bin
mv chess_client ../bin
mv chess_server ../bin
cd ..

echo "== Build completed successfully =="
echo "== Binaries location: $(pwd)/bin =="
