cmake_minimum_required(VERSION 3.16)
project(MPChess VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Set Qt path explicitly
set(CMAKE_PREFIX_PATH "/Users/niravdd/Qt/6.5.3/macos")

# Find Qt packages for both client and server
find_package(Qt6 COMPONENTS Core Network REQUIRED)
find_package(Qt6 COMPONENTS Gui Widgets Svg Multimedia Charts REQUIRED)

# Create server executable
add_executable(MPChessServer
    server/MPChessServer.cpp
    server/MPChessServer.h
)

target_link_libraries(MPChessServer PRIVATE
    Qt6::Core
    Qt6::Network
)

# Create client executable
set(CLIENT_RESOURCE_FILES
    client/resources/resources.qrc
)

# Create UI file for client
qt_wrap_ui(CLIENT_UI_HEADERS client/MPChessClient.ui)

add_executable(MPChessClient
    client/MPChessClient.cpp
    client/MPChessClient.h
    client/resources/resources.qrc
    client/MPChessClient.ui
)

target_link_libraries(MPChessClient PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Svg
    Qt6::Multimedia
    Qt6::Charts
)

# Create necessary directories for server
add_custom_command(TARGET MPChessServer POST_BUILD
    COMMAND \${CMAKE_COMMAND} -E make_directory \${CMAKE_BINARY_DIR}/data
    COMMAND \${CMAKE_COMMAND} -E make_directory \${CMAKE_BINARY_DIR}/data/game_history
    COMMAND \${CMAKE_COMMAND} -E make_directory \${CMAKE_BINARY_DIR}/data/players
    COMMAND \${CMAKE_COMMAND} -E make_directory \${CMAKE_BINARY_DIR}/data/logs
)

# Install
install(TARGETS MPChessClient MPChessServer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION \${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION \${CMAKE_INSTALL_BINDIR}
)

# For macOS bundle
if(APPLE)
    set_target_properties(MPChessClient PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.niravdoshi.mpchessclient"
        MACOSX_BUNDLE_BUNDLE_NAME "Multiplayer Chess"
        MACOSX_BUNDLE_BUNDLE_VERSION \${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING \${PROJECT_VERSION_MAJOR}.\${PROJECT_VERSION_MINOR}
    )
endif()