cmake_minimum_required(VERSION 3.16)
project(MPChess VERSION 1.0 LANGUAGES CXX)

# Ensure modern CMake practices
set(CMAKE_MODULE_PATH \${CMAKE_MODULE_PATH} "\${CMAKE_SOURCE_DIR}/cmake")

# Require C++17 or higher
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Explicitly set Qt installation paths
set(QT_ROOT "\$ENV{HOME}/Qt/6.5.3/macos")
set(Qt6_DIR "\${QT_ROOT}/lib/cmake/Qt6")

# Add multiple potential paths to help CMake find Qt
list(APPEND CMAKE_PREFIX_PATH 
    "\${QT_ROOT}"
    "\${QT_ROOT}/lib"
    "\${QT_ROOT}/lib/cmake"
    "\${QT_ROOT}/lib/cmake/Qt6"
)

# Explicitly find Threads package
find_package(Threads REQUIRED)

# Find Qt packages with explicit version and components
find_package(Qt6 6.5 COMPONENTS 
    Core 
    Gui 
    Widgets 
    Network 
    Multimedia 
    Svg 
    REQUIRED
)

# Qt Configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# macOS Framework support
if(APPLE)
    # Add Framework search paths
    set(CMAKE_FRAMEWORK_PATH 
        "\${QT_ROOT}/lib"
        \${CMAKE_FRAMEWORK_PATH}
    )

    # Use Frameworks for linking
    set(CMAKE_FIND_FRAMEWORK FIRST)

    # Deployment target
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")
endif()

# Add definitions for Qt compatibility and warnings
add_definitions(
    -DQT_DEPRECATED_WARNINGS
    -DQT_DISABLE_DEPRECATED_BEFORE=0x060000
)

# Client Executable
add_executable(chess_client 
    # Main file
    client/main.cpp
    
    # Core files
    src/core/ChessBoard.cpp
    src/core/ChessGame.cpp
    src/core/ChessPiece.cpp
    
    # Network files
    src/network/ChessClient.cpp
    src/network/ChessServer.cpp
    src/network/NetworkClient.cpp
    
    # GUI files
    src/gui/ChessBoardView.cpp
    src/gui/ChessPieceItem.cpp
    src/gui/ConnectDialog.cpp
    src/gui/GameControlPanel.cpp
    src/gui/MainWindow.cpp
    src/gui/MoveHistoryWidget.cpp
    src/gui/SettingsDialog.cpp
    
    # Utility files
    src/util/Settings.cpp
    src/util/SoundManager.cpp
    src/util/ThemeManager.cpp
    
    # Resource files
    assets/pieces.qrc
    assets/sounds.qrc
    assets/themes.qrc
)

# Server Executable
add_executable(chess_server
    # Main files
    server/main.cpp
    server/server.cpp
    
    # Core files
    src/core/ChessBoard.cpp
    src/core/ChessGame.cpp
    src/core/ChessPiece.cpp
    
    # Network files
    src/network/ChessClient.cpp
    src/network/ChessServer.cpp
    src/network/NetworkClient.cpp
)

# Compiler-specific warnings and options
if(APPLE)
    target_compile_options(chess_client PRIVATE 
        -Wno-nullability-completeness
        -Wno-reserved-user-defined-literal
        -Wno-unknown-pragmas
        -Wno-deprecated-declarations
    )
    
    target_compile_options(chess_server PRIVATE 
        -Wno-nullability-completeness
        -Wno-reserved-user-defined-literal
        -Wno-unknown-pragmas
        -Wno-deprecated-declarations
    )
endif()

# Link libraries to targets
target_link_libraries(chess_client PRIVATE 
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
    Qt6::Svg
    Threads::Threads
)

target_link_libraries(chess_server PRIVATE 
    Qt6::Core 
    Qt6::Network
    Threads::Threads
)

# Installation rules
install(TARGETS chess_client chess_server
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

install(DIRECTORY assets/
    DESTINATION share/mpchess
    FILES_MATCHING 
    PATTERN "*.png"
    PATTERN "*.svg"
    PATTERN "*.wav"
    PATTERN "*.json"
)