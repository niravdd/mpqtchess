#/bin/bash

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
cmake --build .

# Install (optional)
cmake --install .

# Create package (optional)
cpack


# Run server
./chess_server -p 12345

# Run client
./chess_client