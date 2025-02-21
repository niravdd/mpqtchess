#/bin/bash

# Create build directory
mkdir build
cd build

# Configure with CMake
# cmake -DCMAKE_PREFIX_PATH=/Users/niravdd/Qt/6.5.3/macos ..
cmake ..

# Build
# cmake --build . -j 8
cmake --build .

# Install (optional)
cmake --install .

# Create package (optional)
cpack


# Run server
./chess_server -p 12345

# Run client
./chess_client