cmake_minimum_required(VERSION 3.16)
project(MPChess VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt settings
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Important: Add appropriate preprocessor defines for Qt
add_definitions(
    -DQT_DEPRECATED_WARNINGS
    -DQT_DISABLE_DEPRECATED_BEFORE=0x060000
)

# Find Qt packages
find_package(Qt6 COMPONENTS Core REQUIRED)
find_package(Qt6 COMPONENTS Gui REQUIRED)
find_package(Qt6 COMPONENTS Widgets REQUIRED) 
find_package(Qt6 COMPONENTS Network REQUIRED)
find_package(Qt6 COMPONENTS Multimedia REQUIRED)
find_package(Qt6 COMPONENTS Svg REQUIRED)

# Include directories
include_directories(
    src
    \${Qt6Core_INCLUDE_DIRS}
    \${Qt6Gui_INCLUDE_DIRS}
    \${Qt6Widgets_INCLUDE_DIRS}
    \${Qt6Network_INCLUDE_DIRS}
    \${Qt6Multimedia_INCLUDE_DIRS}
    \${Qt6Svg_INCLUDE_DIRS}
)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")

# Client executable - using direct file paths
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

# Server executable - using direct file paths
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

# Platform-specific settings
if(APPLE)
    target_compile_options(chess_client PRIVATE 
        -Wno-deprecated-declarations 
        -Wno-nullability-completeness
        -Wno-reserved-user-defined-literal
        -Wno-unknown-pragmas
        -Wno-inconsistent-missing-override
    )
    target_compile_options(chess_server PRIVATE 
        -Wno-deprecated-declarations 
        -Wno-nullability-completeness
        -Wno-reserved-user-defined-literal
        -Wno-unknown-pragmas
        -Wno-inconsistent-missing-override
    )
    
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")
endif()

# Link libraries for client
target_link_libraries(chess_client PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
    Qt6::Svg
)

# Link libraries for server
target_link_libraries(chess_server PRIVATE
    Qt6::Core
    Qt6::Network
)

# Installation rules
install(TARGETS chess_client chess_server
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

install(DIRECTORY assets/
    DESTINATION share/mpchess
    FILES_MATCHING PATTERN "*.png"
                  PATTERN "*.svg"
                  PATTERN "*.wav"
                  PATTERN "*.json"
)