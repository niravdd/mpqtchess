For this project, I recommend Qt 6.5.x (specifically Qt 6.5.3, which is the latest stable release as of now). Here's the step-by-step guide:

Download Qt:

Go to: https://www.qt.io/download-open-source 
Click "Download the Qt Online Installer"
Select the appropriate installer for your OS:
Windows: qt-unified-windows-x64-online.exe
macOS: qt-unified-mac-x64-online.dmg
Linux: qt-unified-linux-x64-online.run
Run the Installer:

Create/Login with a Qt Account (free)
In the installer, select:
Qt 6.5.3
├── MSVC 2019 64-bit (on Windows)
├── MinGW 64-bit (on Windows)
├── macOS (on Mac)
├── GCC 64-bit (on Linux)
└── Additional Libraries
    ├── Qt Multimedia
    ├── Qt Network
    └── Qt Widgets
Also select:
Developer and Designer Tools
├── Qt Creator
├── Debugging Tools
├── CMake
├── Ninja
└── Qt Design Studio
System Requirements:

Windows:

- Windows 10/11 64-bit
- Visual Studio 2019 or 2022 with C++ workload
- OR MinGW 11.2
macOS:

- macOS 11 or later
- Xcode 13 or later
- Command Line Tools
Linux:

- Ubuntu 20.04/22.04 or similar
- GCC 9.4 or later
- Required packages:
  sudo apt-get install build-essential libgl1-mesa-dev
  sudo apt-get install libfontconfig1-dev libfreetype6-dev
  sudo apt-get install libx11-dev libx11-xcb-dev libxext-dev
  sudo apt-get install libxfixes-dev libxi-dev libxrender-dev
  sudo apt-get install libxcb1-dev libxcb-glx0-dev libxcb-keysyms1-dev
  sudo apt-get install libxcb-image0-dev libxcb-shm0-dev libxcb-icccm4-dev
  sudo apt-get install libxcb-sync-dev libxcb-xfixes0-dev libxcb-shape0-dev
  sudo apt-get install libxcb-randr0-dev libxcb-render-util0-dev
  sudo apt-get install libxkbcommon-dev libxkbcommon-x11-dev
Environment Setup:

Windows (CMD or PowerShell):

# If using MSVC:
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# Add Qt to PATH:
set PATH=C:\Qt\6.5.3\msvc2019_64\bin;%PATH%
macOS/Linux (bash/zsh):

# Add Qt to PATH
export PATH="\$HOME/Qt/6.5.3/gcc_64/bin:\$PATH"    # Linux
export PATH="\$HOME/Qt/6.5.3/macos/bin:\$PATH"     # macOS
Verify Installation:

# Check Qt version
qmake --version

# Check CMake version
cmake --version
Building the Chess Game:

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake -DCMAKE_PREFIX_PATH=/path/to/Qt/6.5.3/gcc_64 ..   # Linux
cmake -DCMAKE_PREFIX_PATH=/path/to/Qt/6.5.3/msvc2019_64 ..   # Windows
cmake -DCMAKE_PREFIX_PATH=/path/to/Qt/6.5.3/macos ..   # macOS

# Build
cmake --build . --config Release
IDE Setup (Qt Creator):

Open Qt Creator
File → Open File or Project
Select the CMakeLists.txt from your chess project
Configure the project with the appropriate kit
Click "Configure Project"
Common Issues & Solutions:

If CMake can't find Qt:

# Add to CMakeLists.txt before find_package
set(CMAKE_PREFIX_PATH "path/to/your/Qt/installation")
If building fails due to missing dependencies:

# Linux: Install additional dependencies
sudo apt-get install libgl1-mesa-dev

# Windows: Ensure Visual Studio C++ workload is installed
# macOS: Ensure Xcode Command Line Tools are installed
xcode-select --install
If running the application fails:

# Copy Qt DLLs to executable directory (Windows)
windeployqt.exe chess_client.exe

# Fix rpath (macOS)
macdeployqt chess_client.app
Would you like me to provide more specific details for your particular operating system or explain any part in more detail?