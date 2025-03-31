cmake_minimum_required(VERSION 3.16)
project(ChessGame VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt packages
find_package(Qt6 COMPONENTS 
    Core
    Gui
    Widgets
    Network
    Multimedia
    Svg
    REQUIRED
)

# Include directories
include_directories(
    \${CMAKE_CURRENT_SOURCE_DIR}/src
    \${CMAKE_CURRENT_SOURCE_DIR}/src/core
    \${CMAKE_CURRENT_SOURCE_DIR}/src/network
    \${CMAKE_CURRENT_SOURCE_DIR}/src/gui
    \${CMAKE_CURRENT_SOURCE_DIR}/src/util
    \${Qt6Core_INCLUDE_DIRS}
    \${Qt6Gui_INCLUDE_DIRS}
    \${Qt6Widgets_INCLUDE_DIRS}
    \${Qt6Network_INCLUDE_DIRS}
    \${Qt6Multimedia_INCLUDE_DIRS}
    \${Qt6Svg_INCLUDE_DIRS}
)

# Client executable
add_executable(chess_client
    client/main.cpp
    src/core/ChessPiece.cpp
    src/core/ChessBoard.cpp
    src/core/ChessGame.cpp
    src/network/ChessClient.cpp
    src/network/ChessServer.cpp
    src/network/NetworkClient.cpp
    src/gui/MainWindow.cpp
    src/gui/ChessBoardView.cpp
    src/gui/ChessPieceItem.cpp
    src/gui/GameControlPanel.cpp
    src/gui/MoveHistoryWidget.cpp
    src/gui/SettingsDialog.cpp
    src/gui/ConnectDialog.cpp
    src/util/Settings.cpp
    src/util/ThemeManager.cpp
    src/util/SoundManager.cpp
    assets/pieces.qrc
    assets/sounds.qrc
    assets/themes.qrc
)

# Server executable
add_executable(chess_server
    server/main.cpp
    src/core/ChessPiece.cpp
    src/core/ChessBoard.cpp
    src/core/ChessGame.cpp
    src/network/ChessClient.cpp
    src/network/ChessServer.cpp
    src/network/NetworkClient.cpp
    src/gui/MainWindow.cpp
    src/gui/ChessBoardView.cpp
    src/gui/ChessPieceItem.cpp
    src/gui/GameControlPanel.cpp
    src/gui/MoveHistoryWidget.cpp
    src/gui/SettingsDialog.cpp
    src/gui/ConnectDialog.cpp
    src/util/Settings.cpp
    src/util/ThemeManager.cpp
    src/util/SoundManager.cpp
)

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

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY \${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY \${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY \${CMAKE_BINARY_DIR}/lib)

# Add compiler warnings
if(MSVC)
    target_compile_options(chess_client PRIVATE /W4)
    target_compile_options(chess_server PRIVATE /W4)
else()
    target_compile_options(chess_client PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(chess_server PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Installation rules
install(TARGETS chess_client chess_server
    RUNTIME DESTINATION bin
)

# Install assets (if not using resource system)
install(DIRECTORY assets/
    DESTINATION share/chessgame
    FILES_MATCHING PATTERN "*.png"
                  PATTERN "*.svg"
                  PATTERN "*.wav"
                  PATTERN "*.json"
)