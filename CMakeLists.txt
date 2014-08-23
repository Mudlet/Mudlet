############################################################################
#    Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            #
#    Copyright (C) 2014 by Stephen Lyons - slysven@virginmedia.com         #
#                                                                          #
#    This program is free software; you can redistribute it and/or modify  #
#    it under the terms of the GNU General Public License version 2 as     #
#    published by the Free Software Foundation.                            #
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

# What IS this project:
PROJECT(mudlet)

# Miscrosoft Visual 'C' Studio needs some features that are not in some earlier
# releases:
IF(MSVC)
    CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)
ELSE()
    CMAKE_MINIMUM_REQUIRED(VERSION 2.8.9)
ENDIF()

# Uncomment to display the details of each make step as it happens, especially
# useful if one needs to debug compilation / linker flags
SET(CMAKE_VERBOSE_MAKEFILE 1)

IF(CMAKE_BUILD_TYPE STREQUAL "")
# CMake defaults to leaving CMAKE_BUILD_TYPE empty. This makes it hard to
# differentiation between debug and release builds.
    MESSAGE(WARNING "Forcing a RELEASE type build, to remove this warning please specify the build type when invoking CMake with the command line argument \"-DCMAKE_BULD_TYPE=<type>\" where <type> is one of None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used), RELEASE, DEBUG, RELWITHDEBINFO or MINSIZEREL.")
    SET(CMAKE_BUILD_TYPE RELEASE)
ELSE()
    IF(CMAKE_BUILD_TYPE STREQUAL "None")
        MESSAGE(STATUS "Building a version of Mudlet with settings as per CMAKE_CXX_FLAGS.")
    ELSE()
        MESSAGE(STATUS "Building a ${CMAKE_BUILD_TYPE} version of Mudlet.")
    ENDIF()
ENDIF()

# Set "Bad variable reference syntax is an error", to help find bugs in scripts?
IF(POLICY CMP0010)
    CMAKE_POLICY(SET CMP0010 NEW)
ENDIF()

IF(POLICY CMP0020)
    CMAKE_POLICY(SET CMP0020 NEW)
ENDIF()

# What do we call the target - used in the command line only --help / --version
# invocations, but also used to work out the final name of the main executable
# does get modified for versions with debug info in next level down's CMakeLists.txt
IF(APPLE)
    SET(APP_TARGET Mudlet)
    SET(APP_EXT "")
ELSEIF(WIN32)
    SET(APP_TARGET mudlet)
    SET(APP_EXT .exe)
ELSE()
    SET(APP_TARGET mudlet)
    SET(APP_EXT "")
ENDIF()

# Inform various parts of the application, so they can tell things that
# interact with it (user, user's Lua scripts, other systems), what vintage it is:
SET(APP_VERSION 3.0.1)
SET(APP_BUILD -dev)
# APP_BUILD should only be empty/null in official "release" builds,
# developers may like to set it to their user and branch names to make it easier
# to tell different builds apart - but please ensure it is updated as
# appropriate if pulled into the main Mudlet codebase!

# Changing the above pair of values affects: ctelnet.cpp, main.cpp, mudlet.cpp
# dlgAboutDialog.cpp and TLuaInterpreter.cpp.  It will not necessarily cause
# those files to be automatically rebuilt so you may need to 'touch' those files
# if the variables are changed and you are not doing a full, clean, rebuild!
# Use APP_VERSION, APP_BUILD and APP_TARGET defines in the source code if
# needed.
# IMPORTANT:
# To insure consistency please ensure the SAME values are also assigned to the
# "VERSION" and "BUILD" variables in the native qmake project file,
# i.e. ./src/src.pro

# Configure things for the different target platforms, including settings for
# installation if appropriate
IF(CMAKE_SYSTEM_NAME MATCHES "Linux|Unix")
########################### Unix system target #################################
    MESSAGE(STATUS "Install prefix is: ${CMAKE_INSTALL_PREFIX}")
    IF(NOT CMAKE_INSTALL_PREFIX )
        SET(CMAKE_INSTALL_PREFIX "/usr/local")
    ENDIF()
#    IF(NOT DATAROOTDIR)
#        SET(DATAROOTDIR "${CMAKE_INSTALL_PREFIX}/share")
#    ENDIF()
#    IF(NOT DATADIR)
#        SET(DATADIR "${DATAROOTDIR}/mudlet")
#    ENDIF()
    IF(NOT LUA_DEFAULT_DIR)
        SET(LUA_DEFAULT_DIR "${CMAKE_INSTALL_PREFIX}/share/mudlet/lua")
    ENDIF()
    IF(NOT LUA_GEYSER_DIR)
        SET(LUA_GEYSER_DIR "${LUA_DEFAULT_DIR}/geyser")
    ENDIF()

    ADD_DEFINITIONS(-DLUA_DEFAULT_PATH="${LUA_DEFAULT_DIR}")

ELSEIF(CMAKE_SYSTEM_NAME MATCHES "Darwin")
######################### Macintosh system target ##############################
# UNTESTED, may not even be complete!
    SET(LUA_DEFAULT_DIR "Contents/Resources/lua")
    SET(LUA_GEYSER_DIR ${LUA_DEFAULT_DIR}/geyser)

    ADD_DEFINITIONS(-DLUA_DEFAULT_PATH="${LUA_DEFAULT_DIR}")

ELSEIF(CMAKE_SYSTEM_NAME MATCHES "Windows")
########################## Windows system target ###############################
# TODO - assuming that it could be done and is actually needed for Windoze builds!
ELSE()
# I'd plans for Cygwin and someone might want to a *BSD or a Solaris build...
    MESSAGE(WARNING "Unhandled system type ${CMAKE_SYSTEM_NAME}, please let the developers know so they can fix this.")
ENDIF()

MESSAGE(STATUS "(Re)Writing luaLocation.h ..." )
SET( LUA_SOURCE_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/src/mudlet-lua/lua" )
# Write out a new luaLocation.h to provide the current location of the Mudlet
# core Lua files in the source files for the project, if changed please ensure
# the effective contents are the same in the qmake project file, ie.
# ./src/src.pro, the contents here need ';' to be escaped which is required for
# cmake:
SET(LUA_LOCATION_HEADER_DATA
"#ifndef MUDLET_LUALOCATION_H
#define MUDLET_LUALOCATION_H
/***************************************************************************
 *   Copyright (C) 2014 by Stephen Lyons - slysven@virginmedia.com         *
 *                                                                         *
 *   This program is free software\; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation\; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY\; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program\; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
 
/*
 * THIS FILE IS MODIFIED/REWRITTED EACH TIME THAT THE QMAKE OR CMAKE PROJECT
 * FILES ARE PARSED - USER CHANGES TO THIS FILE WILL BE OVERWRITTEN!
 */
 
/*
 * It should NOT be copied to a shared, readonly, location when the other,
 * actual Lua files in this directory are installed on systems which support
 * such a situation, e.g. most *nixes with a /usr/share part in their file
 * system\; its purpose is to permit the Mudlet application to differentiate
 * between: a development version of those Lua files, in the collection of
 * source files for this project and a normal working set installed into the
 * system for normal use.  For systems/installations where two such sets are
 * not maintained, or for release builds in package form in a distribution and
 * used by others, the physical absence of this file at run-time will cleanly
 * disable the ability to try to choose non-existant source versions of the
 * core Mudlet Lua files.
 */
 
#include <QString>
 
const QString sourceLuaHeaderPathFile = QStringLiteral( \"${LUA_SOURCE_LOCATION}/luaLocation.h\" )\;
 
#endif // MUDLET_LUALOCATION_H"
)

# Clear file:
file( WRITE ${LUA_SOURCE_LOCATION}/luaLocation.h )
# Write out each line, need to append a newline with "\n" and to quote FILELINE
# variable otherwise the embedded ';' disappear:
foreach( FILELINE ${LUA_LOCATION_HEADER_DATA} )
    file( APPEND ${LUA_SOURCE_LOCATION}/luaLocation.h "${FILELINE}\n" )
endforeach( FILELINE )


ADD_SUBDIRECTORY(src)


# Actual installation instructions:
IF(CMAKE_SYSTEM_NAME MATCHES "Linux|Unix")
############################# Unix system target ###############################
    MESSAGE(STATUS "Current dir is: ${CMAKE_SOURCE_DIR}.")
# The built Mudlet application installer is in the next level down's
# CMakeLists.txt
    MESSAGE(STATUS "Installing Mudlet core lua files to ${LUA_DEFAULT_DIR} ...")
    INSTALL(FILES src/mudlet-lua/lua/DB.lua DESTINATION ${LUA_DEFAULT_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/DebugTools.lua DESTINATION ${LUA_DEFAULT_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/GMCP.lua DESTINATION ${LUA_DEFAULT_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/GUIUtils.lua DESTINATION ${LUA_DEFAULT_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/LuaGlobal.lua DESTINATION ${LUA_DEFAULT_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/Other.lua DESTINATION ${LUA_DEFAULT_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/StringUtils.lua DESTINATION ${LUA_DEFAULT_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/TableUtils.lua DESTINATION ${LUA_DEFAULT_DIR} )
    MESSAGE(STATUS "Installing Mudlet geyser lua files to ${LUA_GEYSER_DIR} ...")
    INSTALL(FILES src/mudlet-lua/lua/geyser/Geyser.lua DESTINATION ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserColor.lua DESTINATION ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserContainer.lua DESTINATION ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserGauge.lua DESTINATION ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserGeyser.lua DESTINATION ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserHBox.lua DESTINATION ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserLabel.lua DESTINATION ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserMapper.lua DESTINATION ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserMiniConsole.lua DESTINATION ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserReposition.lua DESTINATION ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserSetConstraints.lua DESTINATION ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserTests.lua DESTINATION ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserUtil.lua DESTINATION ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserVBox.lua DESTINATION ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserWindow.lua DESTINATION ${LUA_GEYSER_DIR} )

    ADD_DEFINITIONS(-DLUA_DEFAULT_PATH="${LUA_DEFAULT_DIR}")

ELSEIF(CMAKE_SYSTEM_NAME MATCHES "Darwin")
########################## Macintosh system target #############################
# UNTESTED, INCOMPLETE!

    INSTALL(FILES src/mudlet-lua/lua/DB.lua DESTINATION ${LUA_DEFAULT_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/DebugTools.lua DESTINATION ${LUA_DEFAULT_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/GMCP.lua DESTINATION ${LUA_DEFAULT_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/GUIUtils.lua DESTINATION ${LUA_DEFAULT_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/LuaGlobal.lua DESTINATION ${LUA_DEFAULT_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/Other.lua DESTINATION ${LUA_DEFAULT_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/StringUtils.lua DESTINATION ${LUA_DEFAULT_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/TableUtils.lua DESTINATION ${LUA_DEFAULT_DIR} )

    INSTALL(FILES src/mudlet-lua/lua/geyser/Geyser.lua ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserColor.lua ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserContainer.lua ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserGauge.lua ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserGeyser.lua ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserHBox.lua ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserLabel.lua ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserMapper.lua ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserMiniConsole.lua ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserReposition.lua ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserSetConstraints.lua ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserTests.lua ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserUtil.lua ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserVBox.lua ${LUA_GEYSER_DIR} )
    INSTALL(FILES src/mudlet-lua/lua/geyser/GeyserWindow.lua ${LUA_GEYSER_DIR} )

    ADD_DEFINITIONS(-DLUA_DEFAULT_PATH="${LUA_DEFAULT_DIR}")
ELSEIF(CMAKE_SYSTEM_NAME MATCHES "Windows")
############################# Windows system target ############################
# TO DO
ELSE()
    MESSAGE(WARNING "Unhandled system type ${CMAKE_SYSTEM_NAME}, please let the developers know so they can fix this.")
ENDIF()
