cmake_minimum_required(VERSION 3.16)
project(ChessClient VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find the Qt libraries
find_package(Qt6 COMPONENTS Core Widgets Network Multimedia REQUIRED)

set(SOURCES
    main.cpp
    ChessClient.cpp
    NetworkManager.cpp
    ChessBoard.cpp
    LoginDialog.cpp
    RegistrationDialog.cpp
    PlayerInfoWidget.cpp
    MatchmakingDialog.cpp
    GameAnalysisWidget.cpp
    LeaderboardDialog.cpp
)

set(HEADERS
    ChessClient.h
    NetworkManager.h
    ChessBoard.h
    LoginDialog.h
    RegistrationDialog.h
    PlayerInfoWidget.h
    MatchmakingDialog.h
    GameAnalysisWidget.h
    LeaderboardDialog.h
)

# Create resources file
qt_add_resources(RESOURCES
    resources.qrc
)

# Create the executable
add_executable(ChessClient \${SOURCES} \${HEADERS} \${RESOURCES})

# Link the Qt libraries
target_link_libraries(ChessClient PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
)

# Install the executable
install(TARGETS ChessClient
    RUNTIME DESTINATION bin
)