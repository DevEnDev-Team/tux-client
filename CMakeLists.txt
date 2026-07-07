cmake_minimum_required(VERSION 3.16)

project(post-it VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(post-it
    main.cpp
    notemodel.h
    istorageprovider.h
    localstorageprovider.h
    localstorageprovider.cpp
    stickywindow.h
    stickywindow.cpp
    notesmanager.h
    notesmanager.cpp
    noteslistwindow.h
    noteslistwindow.cpp
    syncmanager.h
    syncmanager.cpp
    setupwizard.h
    setupwizard.cpp
    resources.qrc
)

target_link_libraries(post-it PRIVATE Qt6::Widgets Qt6::Network)
