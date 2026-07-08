cmake_minimum_required(VERSION 3.16)

project(tux-it VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(tux-it
    src/main.cpp
    src/notemodel.h
    src/istorageprovider.h
    src/localstorageprovider.h
    src/localstorageprovider.cpp
    src/stickywindow.h
    src/stickywindow.cpp
    src/notesmanager.h
    src/notesmanager.cpp
    src/noteslistwindow.h
    src/noteslistwindow.cpp
    src/syncmanager.h
    src/syncmanager.cpp
    src/setupwizard.h
    src/setupwizard.cpp
    src/syncqrcodedialog.h
    src/syncqrcodedialog.cpp
    src/resources.qrc
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
install(FILES src/ressources/logo.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps RENAME org.devendev.TuxIt.png)

