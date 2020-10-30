############################################################################
#    Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            #
#    Copyright (C) 2015-2018, 2020 by Stephen Lyons                        #
#                                                - slysven@virginmedia.com #
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
cmake_minimum_required(VERSION 3.3)

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13") # Qt 5.11.2 with which we build on OS
                                           # X supports 10.11 and up, but some
                                           # tools are not provided as bottles
                                           # for that anymore.
endif()

project(mudlet)

if(CMAKE_CONFIGURATION_TYPES)
  add_custom_target(
    check COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process
                  --output-on-failure --build-config "$<CONFIGURATION>")
else()
  add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                                  --force-new-ctest-process --output-on-failure)
endif()

enable_testing()

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()

if(POLICY CMP0057)
  cmake_policy(SET CMP0057 NEW)
endif()

if(WIN32)
  set(APP_TARGET mudlet.exe)
elseif(APPLE)
  set(APP_TARGET Mudlet)
else()
  set(APP_TARGET mudlet)
endif()

# APP_BUILD should only be empty/null in official "release" builds, developers
# may like to set the MUDLET_VERSION_BUILD environment variable to their user
# and branch names to make it easier to tell different builds apart!
#
# Changing this pair of values affects: ctelnet.cpp, main.cpp, mudlet.cpp
# dlgAboutDialog.cpp and TLuaInterpreter.cpp.  It will not necessarily cause
# those files to be automatically rebuilt so you may need to 'touch' those files
# if the variables are changed and you are not doing a full, clean, rebuild! Use
# APP_VERSION, APP_BUILD and APP_TARGET defines in the source code if needed.
# IMPORTANT: To insure consistency please ensure the SAME of the first two
# values are also assigned to the "VERSION" and "BUILD" variables in the native
# qmake project file, which is NOW called: ./src/mudlet.pro
set(APP_VERSION 4.9.1)
if(DEFINED ENV{MUDLET_VERSION_BUILD} AND NOT $ENV{MUDLET_VERSION_BUILD}
                                         STREQUAL "")
  set(APP_BUILD $ENV{MUDLET_VERSION_BUILD})
else()
  set(APP_BUILD "-dev")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

include(IncludeOptionalModule)

include_optional_module(ENVIRONMENT_VARIABLE WITH_UPDATER
                        OPTION_VARIABLE USE_UPDATER
                        READABLE_NAME "updater"
                        SUPPORTED_SYSTEMS "Linux"
                                          "Windows"
                                          "Darwin")

include_optional_module(ENVIRONMENT_VARIABLE WITH_FONTS
                        OPTION_VARIABLE USE_FONTS
                        READABLE_NAME "fonts")
include_optional_module(ENVIRONMENT_VARIABLE WITH_3DMAPPER
                        OPTION_VARIABLE USE_3DMAPPER
                        READABLE_NAME "3D mapper")
include_optional_module(ENVIRONMENT_VARIABLE WITH_VARIABLE_SPLASH_SCREEN
                        OPTION_VARIABLE USE_VARIABLE_SPLASH_SCREEN
                        READABLE_NAME "build-type splash screen")
include_optional_module(ENVIRONMENT_VARIABLE WITH_OWN_QTKEYCHAIN
                        OPTION_VARIABLE USE_OWN_QT5KEYCHAIN
                        READABLE_NAME "own QKeychain (Qt5) library")
include_optional_module(ENVIRONMENT_VARIABLE WITH_MAIN_BUILD_SYSTEM
                        OPTION_VARIABLE USE_MAIN_BUILD_SYSTEM
                        READABLE_NAME "original CI (setup-windows-sdk.ps1) windows build system"
                        SUPPORTED_SYSTEMS "Windows")

include(InitGitSubmodule)

git_submodule_init(
  CHECK_FILE "3rdparty/edbee-lib/CMakeLists.txt" SUBMODULE_PATH
  "3rdparty/edbee-lib" READABLE_NAME "edbee-lib editor widget")

if(USE_OWN_QT5KEYCHAIN)
  git_submodule_init(
    CHECK_FILE "3rdparty/qtkeychain/CMakeLists.txt"
    SUBMODULE_PATH "3rdparty/qtkeychain"
    READABLE_NAME "QtKeychain")
  add_subdirectory(3rdparty/qtkeychain)
endif()

git_submodule_init(
  CHECK_FILE "3rdparty/lcf/lcf-scm-1.rockspec" SUBMODULE_PATH "3rdparty/lcf"
  READABLE_NAME "lua code formatter source code")

if(USE_UPDATER)
  git_submodule_init(CHECK_FILE "3rdparty/dblsqd/CMakeLists.txt" SUBMODULE_PATH
                     "3rdparty/dblsqd" READABLE_NAME "DBLSQD updater")
endif()

if(APPLE)
  if(USE_UPDATER)
    git_submodule_init(
      CHECK_FILE "3rdparty/sparkle-glue/CMakeLists.txt" SUBMODULE_PATH
      "3rdparty/sparkle-glue" READABLE_NAME "Sparkle glue for updater")

    if(NOT EXISTS "${CMAKE_HOME_DIRECTORY}/3rdparty/cocoapods/Pods/Sparkle")
      message(
        STATUS "Sparkle CocoaPod is missing, running 'pod install' to get it..."
      )
      execute_process(
        TIMEOUT 30
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

add_subdirectory(3rdparty/edbee-lib/edbee-lib)
add_subdirectory(3rdparty/communi)
add_subdirectory(translations/translated)
add_subdirectory(src)
add_subdirectory(test)

if(USE_UPDATER)
  add_subdirectory(3rdparty/dblsqd)

  if(APPLE)
    add_subdirectory(3rdparty/sparkle-glue)
  endif()

endif()
