
project(mudlet)

cmake_minimum_required(VERSION 2.8.11)

IF(WIN32)
    set(APP_TARGET mudlet.exe)
ELSEIF(APPLE)
    set(APP_TARGET Mudlet)
ELSE()
    set(APP_TARGET mudlet)
ENDIF()

set(APP_VERSION 3.0.1)
set(APP_BUILD -rc1-development)
# APP_BUILD should only be empty/null in official "release" builds,
# developers may like to set it to their user and branch names to make it easier
# to tell different builds apart!
#
# Changing the above pair of values affects: ctelnet.cpp, main.cpp, mudlet.cpp
# dlgAboutDialog.cpp and TLuaInterpreter.cpp.  It will not necessarily cause
# those files to be automatically rebuilt so you may need to 'touch' those files if
# the variables are changed and you are not doing a full, clean, rebuild!
# Use APP_VERSION, APP_BUILD and APP_TARGET defines in the source code if needed.
# IMPORTANT:
# To insure consistency please ensure the SAME of the first two values are also
# assigned to the "VERSION" and "BUILD" variables in the native qmake project file,
# i.e. ./src/src.pro

add_subdirectory(src)

