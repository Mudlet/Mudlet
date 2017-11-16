############################################################################
#    Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       #
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

SET(mudlet_RCCS mudlet.qrc)

SET(mudlet_SRCS
    ActionUnit.cpp
    AliasUnit.cpp
    ctelnet.cpp
    dlgAboutDialog.cpp
    dlgActionMainArea.cpp
    dlgAliasMainArea.cpp
    dlgColorTrigger.cpp
    dlgComposer.cpp
    dlgConnectionProfiles.cpp
    dlgIRC.cpp
    dlgKeysMainArea.cpp
    dlgMapper.cpp
    dlgNotepad.cpp
    dlgPackageExporter.cpp
    dlgProfilePreferences.cpp
    dlgRoomExits.cpp
    dlgScriptsMainArea.cpp
    dlgSourceEditorArea.cpp
    dlgSystemMessageArea.cpp
    dlgTimersMainArea.cpp
    dlgTriggerEditor.cpp
    dlgTriggerPatternEdit.cpp
    dlgTriggersMainArea.cpp
    dlgVarsMainArea.cpp
    EAction.cpp
    exitstreewidget.cpp
    FontManager.cpp
    glwidget.cpp
    Host.cpp
    HostManager.cpp
    KeyUnit.cpp
    LuaInterface.cpp
    main.cpp
    mudlet.cpp
    ScriptUnit.cpp
    T2DMap.cpp
    TAction.cpp
    TAlias.cpp
    TArea.cpp
    TBuffer.cpp
    TCommandLine.cpp
    TConsole.cpp
    TDebug.cpp
    TDockWidget.cpp
    TEasyButtonBar.cpp
    TFlipButton.cpp
    TForkedProcess.cpp
    THighlighter.cpp
    TimerUnit.cpp
    TKey.cpp
    TLabel.cpp
    TLuaInterpreter.cpp
    TMap.cpp
    TriggerUnit.cpp
    TRoom.cpp
    TRoomDB.cpp
    TScript.cpp
    TSplitter.cpp
    TSplitterHandle.cpp
    TTextEdit.cpp
    TTimer.cpp
    TToolBar.cpp
    TTreeWidget.cpp
    TTrigger.cpp
    TVar.cpp
    VarUnit.cpp
    XMLexport.cpp
    XMLimport.cpp
    ircmessageformatter.cpp
    updater.cpp
)

# This is for compiled UI files, not those used at runtime though the resource file.
SET(mudlet_UIS
    ui/about_dialog.ui
    ui/actions_main_area.ui
    ui/aliases_main_area.ui
    ui/color_trigger.ui
    ui/composer.ui
    ui/connection_profiles.ui
    ui/dlgPackageExporter.ui
    ui/irc.ui
    ui/keybindings_main_area.ui
    ui/main_window.ui
    ui/mapper.ui
    ui/notes_editor.ui
    ui/profile_preferences.ui
    ui/room_exits.ui
    ui/scripts_main_area.ui
    ui/source_editor_area.ui
    ui/system_message_area.ui
    ui/timers_main_area.ui
    ui/triggers_main_area.ui
    ui/trigger_editor.ui
    ui/trigger_pattern_edit.ui
    ui/vars_main_area.ui
)

SET(mudlet_MOC_HDRS
    ctelnet.h
    dlgAboutDialog.h
    dlgActionMainArea.h
    dlgAliasMainArea.h
    dlgColorTrigger.h
    dlgComposer.h
    dlgConnectionProfiles.h
    dlgIRC.h
    dlgKeysMainArea.h
    dlgMapper.h
    dlgNotepad.h
    dlgPackageExporter.h
    dlgProfilePreferences.h
    dlgRoomExits.h
    dlgScriptsMainArea.h
    dlgSourceEditorArea.h
    dlgSystemMessageArea.h
    dlgTimersMainArea.h
    dlgTriggerEditor.h
    dlgTriggerPatternEdit.h
    dlgTriggersMainArea.h
    dlgVarsMainArea.h
    EAction.h
    exitstreewidget.h
    glwidget.h
    mudlet.h
    T2DMap.h
    TCommandLine.h
    TConsole.h
    TEasyButtonBar.h
    TForkedProcess.h
    THighlighter.h
    TLabel.h
    TLuaInterpreter.h
    TMap.h
    TSplitter.h
    TSplitterHandle.h
    TTextEdit.h
    TToolBar.h
    TTreeWidget.h
    updater.h

)

SET(mudlet_HDRS
    ActionUnit.h
    AliasUnit.h
    FontManager.h
    Host.h
    HostManager.h
    KeyUnit.h
    LuaInterface.h
    pre_guard.h
    post_guard.h
    ScriptUnit.h
    TAction.h
    TAlias.h
    TArea.h
    TAstar.h
    TBuffer.h
    TDebug.h
    TDockWidget.h
    testdbg.h
    TEvent.h
    TFlipButton.h
    TimerUnit.h
    TKey.h
    TMatchState.h
    Tree.h
    TriggerUnit.h
    TRoom.h
    TRoomDB.h
    TScript.h
    TSplitterHandle.h
    TTimer.h
    TTrigger.h
    TVar.h
    VarUnit.h
    XMLexport.h
    XMLimport.h
    ircmessageformatter.h
    updater.h
)

IF(MSVC)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MP") # parallel builds
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Zc:inline") # improve link times
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Zc:rvalueCast") # improve conformance
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd4305") # '=' : truncation from 'double' to 'float'
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd4309") # 'initializing' : truncation of constant value
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd4996") # deprecation
ELSE()
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
  IF(APPLE)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
  ENDIF()
ENDIF()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

SET(PCRE_DIR "${PROJECT_SOURCE_DIR}/.." CACHE PATH "PCRE install directory.")

SET(YAJL_DIR "${PROJECT_SOURCE_DIR}/.." CACHE PATH "YAJL install directory.")

SET(HUNSPELL_DIR "${PROJECT_SOURCE_DIR}/.." CACHE PATH "HUNSPELL install directory.")

SET(ZIP_DIR "${PROJECT_SOURCE_DIR}/.." CACHE PATH "ZIP install directory.")

IF(EXISTS "${PROJECT_SOURCE_DIR}/../include/lua.h")
  SET(LUA_INCLUDE_DIR_TEMP "${PROJECT_SOURCE_DIR}/../include")
  GET_FILENAME_COMPONENT(LUA_INCLUDE_DIR ${LUA_INCLUDE_DIR_TEMP} ABSOLUTE CACHE)
  IF(EXISTS "${PROJECT_SOURCE_DIR}/../lib/lualib.lib")
    SET(LUA_LIBRARY_RELEASE_TEMP "${PROJECT_SOURCE_DIR}/../lib/lualib.lib")
    GET_FILENAME_COMPONENT(LUA_LIBRARY_RELEASE ${LUA_LIBRARY_RELEASE_TEMP} ABSOLUTE CACHE)
  ENDIF()
  IF(EXISTS "${PROJECT_SOURCE_DIR}/../lib/lualibd.lib")
    SET(LUA_LIBRARY_DEBUG_TEMP "${PROJECT_SOURCE_DIR}/../lib/lualibd.lib")
    GET_FILENAME_COMPONENT(LUA_LIBRARY_DEBUG ${LUA_LIBRARY_DEBUG_TEMP} ABSOLUTE CACHE)
  ENDIF()
  IF(LUA_LIBRARY_DEBUG AND LUA_LIBRARY_RELEASE)
    SET(LUA_LIBRARY optimized ${LUA_LIBRARY_RELEASE} debug ${LUA_LIBRARY_DEBUG} )
  ELSEIF(LUA_LIBRARY_RELEASE)
    SET(LUA_LIBRARY ${LUA_LIBRARY_RELEASE} )
  ELSEIF(LUA_LIBRARY_DEBUG)
    SET(LUA_LIBRARY ${LUA_LIBRARY_DEBUG} )
  ENDIF()
ENDIF()

IF(WIN32)
  IF(EXISTS "${PROJECT_SOURCE_DIR}/../include/zlib.h")
    SET(ZLIB_INCLUDE_DIR_TEMP "${PROJECT_SOURCE_DIR}/../include")
    GET_FILENAME_COMPONENT(ZLIB_INCLUDE_DIR ${ZLIB_INCLUDE_DIR_TEMP} ABSOLUTE CACHE)
    SET(ZLIB_ROOT "${PROJECT_SOURCE_DIR}/..")
    IF(EXISTS "${PROJECT_SOURCE_DIR}/../lib/zlibstat.lib")
      SET(ZLIB_LIBRARY_RELEASE_TEMP "${PROJECT_SOURCE_DIR}/../lib/zlibstat.lib")
      GET_FILENAME_COMPONENT(ZLIB_LIBRARY_RELEASE ${ZLIB_LIBRARY_RELEASE_TEMP} ABSOLUTE CACHE)
    ENDIF()
    IF(EXISTS "${PROJECT_SOURCE_DIR}/../lib/zlibstatd.lib")
      SET(ZLIB_LIBRARY_DEBUG_TEMP "${PROJECT_SOURCE_DIR}/../lib/zlibstatd.lib")
      GET_FILENAME_COMPONENT(ZLIB_LIBRARY_DEBUG ${ZLIB_LIBRARY_DEBUG_TEMP} ABSOLUTE CACHE)
    ENDIF()
    IF(ZLIB_LIBRARY_DEBUG AND ZLIB_LIBRARY_RELEASE)
      SET(ZLIB_LIBRARY optimized ${ZLIB_LIBRARY_RELEASE} debug ${ZLIB_LIBRARY_DEBUG} )
    ELSEIF(ZLIB_LIBRARY_RELEASE)
      SET(ZLIB_LIBRARY ${ZLIB_LIBRARY_RELEASE} )
    ELSEIF(ZLIB_LIBRARY_DEBUG)
      SET(ZLIB_LIBRARY ${ZLIB_LIBRARY_DEBUG} )
    ENDIF()
  ENDIF()
ENDIF()

# Break each step into a separate command so any status message is output straight away
IF(APPLE)
  # The include directory setup for Zip is unusual in that as well as e.g. /usr/include/zip.h
  # we need the path to an interal header zipconf.g that it calls for using '<''>'s
  # i.e. SYSTEM #include delimiters which are typically located at e.g. /usr/lib/libzip/include/zipconf.h
  # and using pkg-config is the recommended way to get the details.
  # Spotted recommendation to use pkg-config here https://github.com/Homebrew/homebrew/issues/13390
  FIND_PACKAGE(PkgConfig)
  IF(NOT(PKG_CONFIG_FOUND))
    MESSAGE(WARNING "Unable to use pkg_config - will possibly fail to find/use Zip library...")
  ENDIF()
ENDIF()

IF((APPLE) AND (PKG_CONFIG_FOUND))
  PKG_SEARCH_MODULE(PC_ZIP zip libzip)
  # Use a PC_ prefix to distinguish between what pkg-config finds and a direct use of FIND_PACKAGE(ZIP)
  # Package "zip" is called "libzip" at least on MY Linux box so look for BOTH
  IF(PC_ZIP_FOUND)
    IF(PC_ZIP_zip_FOUND)
      MESSAGE(STATUS "Using pkg_config, found \"zip\" version: ${PC_ZIP_zip_VERSION} with:")
    ELSEIF(PC_ZIP_libzip_FOUND)
      MESSAGE(STATUS "Using pkg_config, found \"libzip\" version: ${PC_ZIP_libzip_VERSION} with:")
    ELSE()
      MESSAGE(STATUS "Using pkg_config, found Zip version: ${PC_ZIP_VERSION} with:")
    ENDIF()
    MESSAGE(STATUS "  include directory(ies), ZIP_INCLUDE_DIRS: ${PC_ZIP_INCLUDE_DIRS} .")
    MESSAGE(STATUS "  library(ies): ZIP_LIBRARY_DIRS: ${PC_ZIP_LIBRARY_DIRS}; ZIP_LIBDIR: ${PC_ZIP_LIBDIR}. ")
  ELSE()
    MESSAGE(WARNING "Using pkg_config, failed to find any version of Zip library!")
  ENDIF()
ENDIF()

IF((APPLE) AND (PKG_CONFIG_FOUND))
  PKG_SEARCH_MODULE(PC_HUNSPELL hunspell libhunspell)
  IF(PC_HUNSPELL_FOUND)
    IF(PC_HUNSPELL_hunspell_FOUND)
      MESSAGE(STATUS "Using pkg_config, found \"hunspell\" version: ${PC_HUNSPELL_hunspell_VERSION} with:")
    ELSEIF(PC_HUNSPELL_libhunspell_FOUND)
      MESSAGE(STATUS "Using pkg_config, found \"libhunspell\" version: ${PC_HUNSPELL_libhunspell_VERSION} with:")
    ELSE()
      MESSAGE(STATUS "Using pkg_config, found Hunspell version: ${PC_HUNSPELL_VERSION} with:")
    ENDIF()
    MESSAGE(STATUS "  include directory(ies), HUNSPELL_INCLUDE_DIRS: ${PC_HUNSPELL_INCLUDE_DIRS} .")
    MESSAGE(STATUS "  library(ies): HUNSPELL_LIBRARY_DIRS: ${PC_HUNSPELL_LIBRARY_DIRS}; HUNSPELL_LIBRARY_DIR: ${PC_HUNSPELL_LIBRARY_DIR}. ")
  ELSEIF()
    MESSAGE(WARNING "Using pkg_config, failed to find any version of Hunspell library!")
  ENDIF()
ENDIF()

FIND_PACKAGE(Qt5Core REQUIRED)
IF (Qt5Core_VERSION VERSION_LESS 5.6)
  message(FATAL_ERROR "Mudlet requires Qt 5.6 or later")
ELSE()
  message(STATUS "Qt version: ${Qt5Core_VERSION}")
ENDIF()

FIND_PACKAGE(Qt5Multimedia REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)
FIND_PACKAGE(Qt5OpenGL REQUIRED)
FIND_PACKAGE(Qt5UiTools REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS Concurrent)
FIND_PACKAGE(Qt5Concurrent REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS Gamepad QUIET)

IF(WIN32)
  SET(mudlet_RC_ICONS = icons/mudlet_main_512x512_6XS_icon.ico)
  QT5_ADD_RESOURCES(RC_ICONS ${mudlet_RC_ICONS})
ENDIF()

IF(NOT(PC_ZIP_FOUND))
  FIND_PACKAGE(ZIP REQUIRED)
ENDIF()
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(Lua51 REQUIRED)
FIND_PACKAGE(ZLIB REQUIRED)
FIND_PACKAGE(PCRE REQUIRED)
FIND_PACKAGE(YAJL REQUIRED)

# Fall back on original FIND_PACKAGE process on Apple if Package Config failed
# Originally we did use that but it started to fail on Travis C.I. in June 2016
# So we now try to use PackageConfig for MacOs platform
IF(APPLE)
  IF(NOT(PC_HUNSPELL_FOUND))
    FIND_PACKAGE(HUNSPELL REQUIRED)
  ENDIF()
ELSE()
  FIND_PACKAGE(HUNSPELL REQUIRED)
ENDIF()

FIND_PACKAGE(Boost 1.44 COMPONENTS graph)

ADD_DEFINITIONS(-DAPP_VERSION="${APP_VERSION}" -DAPP_BUILD="${APP_BUILD}" -DAPP_TARGET="${APP_TARGET}")

IF(UNIX)
    SET(DATAROOTDIR "${CMAKE_INSTALL_PREFIX}/share" CACHE STRING "The platform-specific directory in which read-only data is generally installed")
    SET(DATADIR "${DATAROOTDIR}/mudlet" CACHE STRING "Directory in which mudlet installs its read-only data")
    SET(LUA_DEFAULT_DIR "${DATADIR}/lua" CACHE STRING "Directory in which mudlet installs its read-only lua scripts")
ENDIF(UNIX)

# Define a preprocessor symbol with the default fallback location from which
# to load installed mudlet lua files. Set LUA_DEFAULT_DIR to a
# platform-specific value. If LUA_DEFAULT_DIR is unset, the root directory
# will be used.
ADD_DEFINITIONS(-DLUA_DEFAULT_PATH="${LUA_DEFAULT_DIR}")

# Enable the built-in updater by default. Linux packagers will find it useful to disable it
# since package managers are responsible for updates there
ADD_DEFINITIONS(-DINCLUDE_UPDATER)

# Enable leak detection for MSVC debug builds.
if(MSVC)
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -FItestdbg.h")
endif()

SET(CMAKE_INCLUDE_CURRENT_DIR ON)
INCLUDE_DIRECTORIES(
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
    ${Qt5OpenGL_INCLUDE_DIRS}
    ${Qt5Multimedia_INCLUDE_DIRS}
    ${Qt5UiTools_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIR}
    ${PCRE_INCLUDE_DIR}
    ${YAJL_INCLUDE_DIR}
)

IF(Qt5Gamepad_FOUND)
    ADD_DEFINITIONS(${Qt5Gamepad_DEFINITIONS})
    INCLUDE_DIRECTORIES(${Qt5Gamepad_INCLUDE_DIRS})
    MESSAGE(STATUS "Using Gamepad module")
ENDIF(Qt5Gamepad_FOUND)

# Need to use the plural variables as there can be more than ONE directory to specify:
IF(PC_ZIP_FOUND)
    INCLUDE_DIRECTORIES( ${PC_ZIP_INCLUDE_DIRS} )
ELSE()
    INCLUDE_DIRECTORIES( ${ZIP_INCLUDE_DIRS} )
ENDIF()

INCLUDE_DIRECTORIES( ${ZLIB_INCLUDE_DIR} )

QT5_WRAP_UI(mudlet_UIS_H ${mudlet_UIS})
QT5_WRAP_CPP(mudlet_MOC_SRCS ${mudlet_MOC_HDRS})
QT5_ADD_RESOURCES(mudlet_RCC_SRCS ${mudlet_RCCS})

ADD_EXECUTABLE(mudlet WIN32
    ${mudlet_SRCS}
    ${mudlet_MOC_SRCS}
    ${mudlet_RCC_SRCS}
    ${mudlet_HDRS}
    ${mudlet_UIS_H}
    ${mudlet_MOC_HDRS}
)

TARGET_LINK_LIBRARIES(mudlet
    ${Qt5Core_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Network_LIBRARIES}
    ${Qt5Multimedia_LIBRARIES}
    ${Qt5OpenGL_LIBRARIES}
    ${Qt5UiTools_LIBRARIES}
    ${Qt5Concurrent_LIBRARIES}
    ${Boost_LIBRARIES}
    ${LUA_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${PCRE_LIBRARIES}
    ${YAJL_LIBRARIES}
    ${ZLIB_LIBRARIES}
    communi
    lua_yajl
    edbee-lib
    dblsqd
)

IF(Qt5Gamepad_FOUND)
    TARGET_LINK_LIBRARIES(mudlet ${Qt5Gamepad_LIBRARIES})
ENDIF(Qt5Gamepad_FOUND)

IF(PC_ZIP_FOUND)
    TARGET_LINK_LIBRARIES(mudlet ${PC_ZIP_LIBRARIES})
ELSE()
    TARGET_LINK_LIBRARIES(mudlet ${ZIP_LIBRARIES})
ENDIF()

IF(APPLE)
  IF(NOT EXISTS "${CMAKE_HOME_DIRECTORY}/3rdparty/cocoapods/Pods/Sparkle")
    MESSAGE(STATUS "Sparkle CocoaPod is missing, running 'pod install' to get it...")
    EXECUTE_PROCESS(TIMEOUT 30
            WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/3rdparty/cocoapods"
            COMMAND pod install )
  ENDIF()

  target_link_libraries(mudlet
          luazip
          cocoa-qt-glue
          "-framework Sparkle"
          "-framework AppKit"
          "-F${CMAKE_HOME_DIRECTORY}/3rdparty/cocoapods/Pods/Sparkle")
ENDIF()

IF(PC_HUNSPELL_FOUND)
  TARGET_LINK_LIBRARIES(mudlet ${PC_HUNSPELL_LIBRARIES})
ELSE()
  TARGET_LINK_LIBRARIES(mudlet ${HUNSPELL_LIBRARIES})
ENDIF()
