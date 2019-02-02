############################################################################
#    Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            #
#    Copyright (C) 2015-2018 by Stephen Lyons - slysven@virginmedia.com    #
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

############################################################################
#                                                                          #
#    NOTICE: FreeBSD is not an officially supported platform as such;      #
#    the work on getting it working has been done by myself, and other     #
#    developers, unless they have explicitly said so, are not able to      #
#    address issues relating specifically to that Operating System.        #
#    Nevertheless users of FreeBSD are equally welcome to contribute       #
#    to the development of Mudlet - bugfixes and enhancements are          #
#    welcome from all!                                                     #
#                                           Stephen Lyons, February 2018   #
#                                                                          #
############################################################################

# Should be called before PROJECT.
cmake_minimum_required(VERSION 3.1)
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12") # Qt 5.11.2 with which we build on OS X supports 10.11 and up, but some tools are not provided as bottles for that anymore.
endif()

project(mudlet)

if(CMAKE_CONFIGURATION_TYPES)
  add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                    --force-new-ctest-process --output-on-failure
                    --build-config "$<CONFIGURATION>")
else()
  add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                    --force-new-ctest-process --output-on-failure)
endif()

enable_testing()

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()

if(WIN32)
  set(APP_TARGET mudlet.exe)
elseif(APPLE)
  set(APP_TARGET Mudlet)
else()
  set(APP_TARGET mudlet)
endif()

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
set(APP_VERSION 3.17.0)
if(DEFINED ENV{MUDLET_VERSION_BUILD} AND NOT $ENV{MUDLET_VERSION_BUILD} STREQUAL "")
  SET(APP_BUILD $ENV{MUDLET_VERSION_BUILD})
else()
  set(APP_BUILD "-dev")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Enable the built-in updater by default on the three main supported OS
# unless environmental variable WITH_UPDATER is defined and set to "NO" (case
# insensitive). Linux packagers will find it useful to do this since they are
# likely to be responsible for updates there. Automatically exclude the
# update stuff from other OSs such as FreeBSD and Cygwin because if they ever
# get finished they have their own packaging system - and we do not support
# updating them.
# WITH_UPDATER is an environmental value/variable (so could be a number, a
# string, something else or not even exist).
set(UPDATER_TEST $ENV{WITH_UPDATER})
if((CMAKE_SYSTEM_NAME STREQUAL "Linux") OR (CMAKE_SYSTEM_NAME STREQUAL "Windows") OR (CMAKE_SYSTEM_NAME STREQUAL "Darwin"))
  # We are on one of the supported platforms
  if(DEFINED UPDATER_TEST)
    string(TOUPPER ${UPDATER_TEST} UPDATER_TEST)
    if(UPDATER_TEST STREQUAL "NO")
      # The specific tested for value was seen so set the option "no don't
      # include the updater"
      option(USE_UPDATER "Include libraries and code to allow release versions to be updated on demand" OFF)
    else()
      # Any other value was seen so ignore it and set "yes, include the updater"
      option(USE_UPDATER "Include libraries and code to allow release versions to be updated on demand" ON)
    endif()
  else()
    # An environmental variable not detected, apply platform default of "yes, include the updater"
    option(USE_UPDATER "Include libraries and code to allow release versions to be updated on demand" ON)
  endif()
else()
  # Option not available for other platforms
  set(USE_UPDATER NO)
endif()

# Enable the inclusion of fonts currently carried in the source code by default
# unless the environmental variable WITH_FONTS is defined AND is set to a
# (case insensitive) value of "NO". Linux packagers will find it useful to do
# this since package managers may already package the relevant fonts - or are
# not willing or able to include them.
# WITH_FONTS is an environmental value/variable (so could be a number, a string,
# something else or not even exist).
set(FONT_TEST $ENV{WITH_FONTS})
if(DEFINED FONT_TEST)
  string(TOUPPER ${FONT_TEST} FONT_TEST)
else()
  # Set the CMake variable to the default if it (as a result of WITH_FONTS not
  # existing) has not been defined, without the if(DEFINED ...) string(...)
  # errors out - and so may further tests on the variable...
  set(FONT_TEST "YES")
endif()
if(FONT_TEST STREQUAL "NO")
  option(USE_FONTS "Include Deja Vu and Ubuntu fonts in the executable" OFF)
else()
  option(USE_FONTS "Include Deja Vu and Ubuntu fonts in the executable" ON)
  # Can download and extract latest Unbuntu font files (currently X.YY is
  # 0.83) from:
  # https://launchpad.net/ubuntu/+archive/primary/+files/ubuntu-font-family-sources_X.YY.orig.tar.gz
  # It would be nice if we could automate the download and extraction of all
  # the font and associate documentation (but NOT the "sources" sub-directory)
  # contents into the ./src/fonts/ directory structure only if this option is
  # set to ON; however that would be plaform specific and add more complexity
  # and it is not obvious that there is a demand to do this currenly.
endif()

if(APPLE)
  # Needed (just) on MacOs as an #include in luazip.h:
  add_subdirectory(3rdparty/luazip)
endif()

if(NOT EXISTS "${CMAKE_HOME_DIRECTORY}/3rdparty/edbee-lib/CMakeLists.txt")
  # The above CMakeList.txt is the top level one - we actually use the one
  # another level down in 3rdparty/edbee-lib/edbee-lib/CMakeLists.txt
  message(STATUS "git submodule for required edbee-lib editor widget missing from source code, will attempt to get it...")
  execute_process(COMMAND git submodule update --init 3rdparty/edbee-lib
    TIMEOUT 30
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
    OUTPUT_VARIABLE output_text
    ERROR_VARIABLE error_text)
  if(output_text OR error_text)
    message(STATUS ${output_text} ${error_text})
  endif()
endif()

if(NOT EXISTS "${CMAKE_HOME_DIRECTORY}/3rdparty/lcf/lcf-scm-1.rockspec")
  message(STATUS "git submodule for required lua code formatter source code missing, will attempt to get it...")
  execute_process(COMMAND git submodule update --init 3rdparty/lcf
    TIMEOUT 30
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
    OUTPUT_VARIABLE output_text
    ERROR_VARIABLE error_text)
  if(output_text OR error_text)
    message(STATUS ${output_text} ${error_text})
  endif()
endif()

if(USE_UPDATER)
  if(NOT EXISTS "${CMAKE_HOME_DIRECTORY}/3rdparty/dblsqd/CMakeLists.txt")
    message(STATUS "git submodule for optional but wanted DBLSQD updater missing from source code, will attempt to get it...")
    execute_process(COMMAND git submodule update --init 3rdparty/dblsqd
      TIMEOUT 30
      WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
      OUTPUT_VARIABLE output_text
      ERROR_VARIABLE error_text)
    if(output_text OR error_text)
      message(STATUS ${output_text} ${error_text})
    endif()
  endif()
endif()

if(APPLE)
  if(USE_UPDATER)
    if(NOT EXISTS "${CMAKE_HOME_DIRECTORY}/3rdparty/sparkle-glue/CMakeLists.txt")
      message(STATUS "git submodule for optional but wanted Sparkle glue for updater missing from source code, will attempt to get it...")
      execute_process(COMMAND git submodule update --init 3rdparty/sparkle-glue
        TIMEOUT 30
        WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
        OUTPUT_VARIABLE output_text
        ERROR_VARIABLE error_text)
      if(output_text OR error_text)
        message(STATUS ${output_text} ${error_text})
      endif()
    endif()
  endif()
endif()

if(EXISTS "${CMAKE_HOME_DIRECTORY}/3rdparty/edbee-lib/CMakeLists.txt")
  add_subdirectory(3rdparty/edbee-lib/edbee-lib)
elseif()
  message(FATAL_ERROR "Cannot locate edbee-lib editor widget submodule source code, build abandoned!")
endif()

file(GLOB_RECURSE lua_files RELATIVE "${CMAKE_HOME_DIRECTORY}/src/mudlet-lua/lua/" "${CMAKE_HOME_DIRECTORY}/src/mudlet-lua/lua/*.lua")
list(LENGTH lua_files lua_file_count)
if(lua_file_count EQUAL 26)
  message(STATUS "Found Mudlet & Geyser lua files")
else()
  message(WARNING "Found ${lua_file_count} Mudlet & Geyser lua files but 26 were expected:")
  foreach(lua_file ${lua_files})
    message(STATUS "    ${lua_file}")
  endforeach(lua_file)
endif()

if(EXISTS "${CMAKE_HOME_DIRECTORY}/3rdparty/lcf/lcf-scm-1.rockspec")
  file(GLOB_RECURSE lcf_files RELATIVE "${CMAKE_HOME_DIRECTORY}/3rdparty/" "${CMAKE_HOME_DIRECTORY}/3rdparty/lcf/*")
  list(REMOVE_ITEM lcf_files "lcf/.git")
  list(LENGTH lcf_files lcf_file_count)
  if(lcf_file_count EQUAL 275)
    message(STATUS "Found Lua Code Formatter files")
  else()
    message(WARNING "Found ${lcf_file_count} Lua Code Formatter files but 275 were expected:")
    foreach(lcf_file ${lcf_files})
      message(STATUS "    ${lcf_file}")
    endforeach(lcf_file)
  endif()
elseif()
  message(FATAL_ERROR "Cannot locate lua code formatter submodule source code, build abandoned!")
endif()

if(USE_UPDATER)
  if(EXISTS "${CMAKE_HOME_DIRECTORY}/3rdparty/dblsqd/CMakeLists.txt")
    add_subdirectory(3rdparty/dblsqd)
    add_definitions(-DINCLUDE_UPDATER)
  else()
    message(FATAL_ERROR "Cannot locate DBLSQD updater submodule source code, build abandoned!")
  endif()

  if(APPLE)
    if(EXISTS "${CMAKE_HOME_DIRECTORY}/3rdparty/sparkle-glue/CMakeLists.txt")
      add_subdirectory(3rdparty/sparkle-glue)
    else()
      message(FATAL_ERROR "Cannot locate Sparkle glue for updater submodule source code, build abandoned!")
    endif()

    if(NOT EXISTS "${CMAKE_HOME_DIRECTORY}/3rdparty/cocoapods/Pods/Sparkle")
      message(STATUS "Sparkle CocoaPod is missing, running 'pod install' to get it...")
      execute_process(TIMEOUT 30
        WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/3rdparty/cocoapods"
        COMMAND pod install)
    endif()
  endif()
endif()


find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

add_subdirectory(src)
add_subdirectory(3rdparty/communi)
add_subdirectory(translations/translated)
