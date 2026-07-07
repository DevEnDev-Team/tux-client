cmake_minimum_required(VERSION 3.16)

project(tux-it VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(tux-it
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
    syncqrcodedialog.h
    syncqrcodedialog.cpp
    resources.qrc
)

target_link_libraries(tux-it PRIVATE Qt6::Widgets Qt6::Network)

include(GNUInstallDirs)

install(TARGETS tux-it
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install desktop file
install(FILES org.devendev.TuxIt.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

# Install appdata/metainfo file
install(FILES org.devendev.TuxIt.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

# Install icon
install(FILES ressources/logo.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps RENAME org.devendev.TuxIt.png)

