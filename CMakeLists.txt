cmake_minimum_required(VERSION 3.16)

project(Chat_Client VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    qt_client.cpp
    src/protocol.c
)

# 强制 protocol.c 作为 C 语言编译
set_source_files_properties(src/protocol.c PROPERTIES LANGUAGE C)

# 添加 include 路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if(${QT_VERSION_MAJOR} EQUAL 6)
    qt_add_executable(Chat_Client ${PROJECT_SOURCES})
else()
    add_executable(Chat_Client ${PROJECT_SOURCES})
endif()

target_link_libraries(Chat_Client PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    pthread
    cjson
)

target_include_directories(Chat_Client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

install(TARGETS Chat_Client
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(${QT_VERSION_MAJOR} EQUAL 6)
    qt_finalize_executable(Chat_Client)
endif()
