############################################################################
#    Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            #
#    Copyright (C) 2015-2017 by Stephen Lyons - slysven@virginmedia.com    #
#                                                                          #
#    This program is free software; you can redistribute it and/or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 2 of the License, or     #
#    (at your option) any later version.                                   #
#                                                                          #
#    This program is distributed in the hope that it will be useful,       #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    GNU General Public License for more details.                          #
#                                                                          #
#    You should have received a copy of the GNU General Public License     #
#    along with this program; if not, write to the                         #
#    Free Software Foundation, Inc.,                                       #
#    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
############################################################################

# Should be called before PROJECT.
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

PROJECT(mudlet)

IF(CMAKE_CONFIGURATION_TYPES)
  ADD_CUSTOM_TARGET(check COMMAND ${CMAKE_CTEST_COMMAND}
                    --force-new-ctest-process --output-on-failure
                    --build-config "$<CONFIGURATION>")
ELSE()
  ADD_CUSTOM_TARGET(check COMMAND ${CMAKE_CTEST_COMMAND}
                    --force-new-ctest-process --output-on-failure)
ENDIF()

ENABLE_TESTING()

IF(POLICY CMP0020)
  CMAKE_POLICY(SET CMP0020 NEW)
ENDIF()

IF(WIN32)
  SET(APP_TARGET mudlet.exe)
ELSEIF(APPLE)
  SET(APP_TARGET Mudlet)
ELSE()
  SET(APP_TARGET mudlet)
ENDIF()

# APP_BUILD should only be empty/null in official "release" builds,
# developers may like to set the MUDLET_VERSION_BUILD environment variable to
# their user and branch names to make it easier to tell different builds apart!
#
# Changing this pair of values affects: ctelnet.cpp, main.cpp, mudlet.cpp
# dlgAboutDialog.cpp and TLuaInterpreter.cpp.  It will not necessarily cause
# those files to be automatically rebuilt so you may need to 'touch' those files if
# the variables are changed and you are not doing a full, clean, rebuild!
# Use APP_VERSION, APP_BUILD and APP_TARGET defines in the source code if needed.
# IMPORTANT:
# To insure consistency please ensure the SAME of the first two values are also
# assigned to the "VERSION" and "BUILD" variables in the native qmake project
# file, which is NOW called: ./src/mudlet.pro
SET(APP_VERSION 3.6.0)
IF(DEFINED ENV{MUDLET_VERSION_BUILD} AND NOT $ENV{MUDLET_VERSION_BUILD} STREQUAL "")
  SET(APP_BUILD $ENV{MUDLET_VERSION_BUILD})
else()
  SET(APP_BUILD "")
ENDIF()

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Enable the built-in updater by default unless NO_INCLUDE_UPDATER is
# already defined. Linux packagers will find it useful to do this since
# package managers are responsible for updates there. Automatically exclude
# the update stuff from FreeBSD and Cygwin because if they ever get finished
# they have their own packaging system
IF((DEFINED ENV{NO_INCLUDE_UPDATER} AND NOT $ENV{MUDLET_VERSION_BUILD} STREQUAL "") OR (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD") OR (CMAKE_SYSTEM_NAME STREQUAL "Cygwin"))
  OPTION(WITH_UPDATER "Include libraries and code to allow release versions to be updated on demand" OFF)
ELSE()
  OPTION(WITH_UPDATER "Include libraries and code to allow release versions to be updated on demand" ON)
ENDIF()

IF(APPLE)
  # Needed (just) on MacOs as an #include in luazip.h:
  ADD_SUBDIRECTORY(3rdparty/luazip)
ENDIF()

IF(NOT EXISTS "${CMAKE_HOME_DIRECTORY}/3rdparty/edbee-lib/CMakeLists.txt")
  # The above CMakeList.txt is the top level one - we actually use the one
  # another level down in 3rdparty/edbee-lib/edbee-lib/CMakeLists.txt
  MESSAGE(STATUS "git submodule for required edbee-lib editor widget missing from source code, will attempt to get it...")
  EXECUTE_PROCESS(COMMAND git submodule update --init 3rdparty/edbee-lib
    TIMEOUT 30
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
    OUTPUT_VARIABLE output_text
    ERROR_VARIABLE error_text)
  IF(output_text OR error_text)
    MESSAGE(STATUS ${output_text} ${error_text})
  ENDIF()
ENDIF()

IF(WITH_UPDATER)
  IF(NOT EXISTS "${CMAKE_HOME_DIRECTORY}/3rdparty/dblsqd/CMakeLists.txt")
    MESSAGE(STATUS "git submodule for optional but wanted DBLSQD updater missing from source code, will attempt to get it...")
    EXECUTE_PROCESS(COMMAND git submodule update --init 3rdparty/dblsqd
      TIMEOUT 30
      WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
      OUTPUT_VARIABLE output_text
      ERROR_VARIABLE error_text)
    IF(output_text OR error_text)
      MESSAGE(STATUS ${output_text} ${error_text})
    ENDIF()
  ENDIF()
ENDIF()

IF(APPLE)
  IF(WITH_UPDATER)
    IF(NOT EXISTS "${CMAKE_HOME_DIRECTORY}/3rdparty/sparkle-glue/CMakeLists.txt")
      MESSAGE(STATUS "git submodule for optional but wanted Sparkle glue for updater missing from source code, will attempt to get it...")
      EXECUTE_PROCESS(COMMAND git submodule update --init 3rdparty/sparkle-glue
        TIMEOUT 30
        WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
        OUTPUT_VARIABLE output_text
        ERROR_VARIABLE error_text)
      IF(output_text OR error_text)
        MESSAGE(STATUS ${output_text} ${error_text})
      ENDIF()
    ENDIF()
  ENDIF()
ENDIF()

IF(EXISTS "${CMAKE_HOME_DIRECTORY}/3rdparty/edbee-lib/CMakeLists.txt")
  ADD_SUBDIRECTORY(3rdparty/edbee-lib/edbee-lib)
ELSEIF()
  MESSAGE(FATAL_ERROR "Cannot locate edbee-lib editor widget submodule source code, build abandoned!")
ENDIF()

IF(WITH_UPDATER)
  IF(EXISTS "${CMAKE_HOME_DIRECTORY}/3rdparty/dblsqd/CMakeLists.txt")
    ADD_SUBDIRECTORY(3rdparty/dblsqd)
    ADD_DEFINITIONS(-DINCLUDE_UPDATER)
  ELSE()
    MESSAGE(FATAL_ERROR "Cannot locate DBLSQD updater submodule source code, build abandoned!")
  ENDIF()

  IF(APPLE)
    IF(EXISTS "${CMAKE_HOME_DIRECTORY}/3rdparty/sparkle-glue/CMakeLists.txt")
      ADD_SUBDIRECTORY(3rdparty/sparkle-glue)
    ELSE()
      MESSAGE(FATAL_ERROR "Cannot locate Sparkle glue for updater submodule source code, build abandoned!")
    ENDIF()
  ENDIF()
ENDIF()

IF(APPLE AND WITH_UPDATER AND NOT EXISTS "${CMAKE_HOME_DIRECTORY}/3rdparty/cocoapods/Pods/Sparkle")
  MESSAGE(STATUS "Sparkle CocoaPod is missing, running 'pod install' to get it...")
  EXECUTE_PROCESS(TIMEOUT 30
    WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/3rdparty/cocoapods"
    COMMAND pod install)
ENDIF()

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(3rdparty/communi)
ADD_SUBDIRECTORY(3rdparty/lua_yajl)
