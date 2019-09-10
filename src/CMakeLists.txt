############################################################################
#    Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       #
#    Copyright (C) 2015-2018 by Stephen Lyons - slysven@virginmedia.com    #
#    Copyright (C) 2018 by Huadong Qi - novload@outlook.com                #
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

set(mudlet_RCCS mudlet.qrc)

if(USE_FONTS)
  set(mudlet_fonts_RCCS mudlet_fonts.qrc)
  add_definitions(-DINCLUDE_FONTS)
endif()

set(mudlet_SRCS
    ActionUnit.cpp
    AliasUnit.cpp
    ctelnet.cpp
    discord.cpp
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
    Host.cpp
    HostManager.cpp
    ircmessageformatter.cpp
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
    TTabBar.cpp
    TTextEdit.cpp
    TTimer.cpp
    TToolBar.cpp
    TTreeWidget.cpp
    TTrigger.cpp
    TVar.cpp
    VarUnit.cpp
    XMLexport.cpp
    XMLimport.cpp
)

# This is for compiled UI files, not those used at runtime though the resource file.
set(mudlet_UIS
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
  ui/vars_main_area.ui)

set(mudlet_MOC_HDRS
    ctelnet.h
    discord.h
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
    Host.h
    mudlet.h
    T2DMap.h
    TCommandLine.h
    TConsole.h
    TEasyButtonBar.h
    TForkedProcess.h
    TLabel.h
    TLuaInterpreter.h
    TMap.h
    TSplitter.h
    TSplitterHandle.h
    TTextEdit.h
    TToolBar.h
    TTreeWidget.h
    XMLexport.h
)

set(mudlet_HDRS
    ActionUnit.h
    AliasUnit.h
    discord.h
    FontManager.h
    HostManager.h
    ircmessageformatter.h
    KeyUnit.h
    LuaInterface.h
    post_guard.h
    pre_guard.h
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
    TTabBar.h
    TTimer.h
    TTrigger.h
    TVar.h
    VarUnit.h
    widechar_width.h
    XMLimport.h
)

if(USE_UPDATER)
  list(APPEND mudlet_SRCS updater.cpp)
  list(APPEND mudlet_MOC_HDRS updater.h)
endif(USE_UPDATER)

if(USE_3DMAPPER)
  list(APPEND mudlet_SRCS glwidget.cpp)
  list(APPEND mudlet_MOC_HDRS glwidget.h)
endif(USE_3DMAPPER)

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MP") # parallel builds
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Zc:inline") # improve link times
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Zc:rvalueCast") # improve conformance
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd4305") # '=' : truncation from 'double' to 'float'
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd4309") # 'initializing' : truncation of constant value
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd4996") # deprecation
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
  if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
  endif()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PCRE_DIR "${PROJECT_SOURCE_DIR}/.." CACHE PATH "PCRE install directory.")

set(YAJL_DIR "${PROJECT_SOURCE_DIR}/.." CACHE PATH "YAJL install directory.")

set(HUNSPELL_DIR "${PROJECT_SOURCE_DIR}/.." CACHE PATH "HUNSPELL install directory.")

set(ZIP_DIR "${PROJECT_SOURCE_DIR}/.." CACHE PATH "ZIP install directory.")

if(EXISTS "${PROJECT_SOURCE_DIR}/../include/lua.h")
  set(LUA_INCLUDE_DIR_TEMP "${PROJECT_SOURCE_DIR}/../include")
  get_filename_component(LUA_INCLUDE_DIR ${LUA_INCLUDE_DIR_TEMP} ABSOLUTE CACHE)
  if(EXISTS "${PROJECT_SOURCE_DIR}/../lib/lualib.lib")
    set(LUA_LIBRARY_RELEASE_TEMP "${PROJECT_SOURCE_DIR}/../lib/lualib.lib")
    get_filename_component(LUA_LIBRARY_RELEASE ${LUA_LIBRARY_RELEASE_TEMP} ABSOLUTE CACHE)
  endif()
  if(EXISTS "${PROJECT_SOURCE_DIR}/../lib/lualibd.lib")
    set(LUA_LIBRARY_DEBUG_TEMP "${PROJECT_SOURCE_DIR}/../lib/lualibd.lib")
    get_filename_component(LUA_LIBRARY_DEBUG ${LUA_LIBRARY_DEBUG_TEMP} ABSOLUTE CACHE)
  endif()
  if(LUA_LIBRARY_DEBUG AND LUA_LIBRARY_RELEASE)
    set(LUA_LIBRARY optimized ${LUA_LIBRARY_RELEASE} debug ${LUA_LIBRARY_DEBUG} )
  elseif(LUA_LIBRARY_RELEASE)
    set(LUA_LIBRARY ${LUA_LIBRARY_RELEASE} )
  elseif(LUA_LIBRARY_DEBUG)
    set(LUA_LIBRARY ${LUA_LIBRARY_DEBUG} )
  endif()
endif()

if(WIN32)
  if(EXISTS "${PROJECT_SOURCE_DIR}/../include/zlib.h")
    set(ZLIB_INCLUDE_DIR_TEMP "${PROJECT_SOURCE_DIR}/../include")
    get_filename_component(ZLIB_INCLUDE_DIR ${ZLIB_INCLUDE_DIR_TEMP} ABSOLUTE CACHE)
    set(ZLIB_ROOT "${PROJECT_SOURCE_DIR}/..")
    if(EXISTS "${PROJECT_SOURCE_DIR}/../lib/zlibstat.lib")
      set(ZLIB_LIBRARY_RELEASE_TEMP "${PROJECT_SOURCE_DIR}/../lib/zlibstat.lib")
      get_filename_component(ZLIB_LIBRARY_RELEASE ${ZLIB_LIBRARY_RELEASE_TEMP} ABSOLUTE CACHE)
    endif()
    if(EXISTS "${PROJECT_SOURCE_DIR}/../lib/zlibstatd.lib")
      set(ZLIB_LIBRARY_DEBUG_TEMP "${PROJECT_SOURCE_DIR}/../lib/zlibstatd.lib")
      get_filename_component(ZLIB_LIBRARY_DEBUG ${ZLIB_LIBRARY_DEBUG_TEMP} ABSOLUTE CACHE)
    endif()
    if(ZLIB_LIBRARY_DEBUG AND ZLIB_LIBRARY_RELEASE)
      set(ZLIB_LIBRARY optimized ${ZLIB_LIBRARY_RELEASE} debug ${ZLIB_LIBRARY_DEBUG} )
    elseif(ZLIB_LIBRARY_RELEASE)
      set(ZLIB_LIBRARY ${ZLIB_LIBRARY_RELEASE} )
    elseif(ZLIB_LIBRARY_DEBUG)
      set(ZLIB_LIBRARY ${ZLIB_LIBRARY_DEBUG} )
    endif()
  endif()
endif()

# Break each step into a separate command so any status message is output straight away
if(APPLE)
  # The include directory setup for Zip is unusual in that as well as e.g. /usr/include/zip.h
  # we need the path to an interal header zipconf.h that it calls for using '<''>'s
  # i.e. SYSTEM #include delimiters which are typically located at e.g. /usr/lib/libzip/include/zipconf.h
  # and using pkg-config is the recommended way to get the details.
  # Spotted recommendation to use pkg-config here https://github.com/Homebrew/homebrew/issues/13390
  find_package(PkgConfig)
  if(NOT(PKG_CONFIG_FOUND))
    message(WARNING "Unable to use pkg_config - will possibly fail to find/use Zip library...")
  endif()
endif()

if((APPLE) AND (PKG_CONFIG_FOUND))
  PKG_SEARCH_MODULE(PC_ZIP zip libzip)
  # Use a PC_ prefix to distinguish between what pkg-config finds and a direct use of find_package(ZIP)
  # Package "zip" is called "libzip" at least on MY Linux box so look for BOTH
  if(PC_ZIP_FOUND)
    if(PC_ZIP_zip_FOUND)
      message(STATUS "Using pkg_config, found \"zip\" version: ${PC_ZIP_zip_VERSION} with:")
    elseif(PC_ZIP_libzip_FOUND)
      message(STATUS "Using pkg_config, found \"libzip\" version: ${PC_ZIP_libzip_VERSION} with:")
    else()
      message(STATUS "Using pkg_config, found Zip version: ${PC_ZIP_VERSION} with:")
    endif()
    message(STATUS "  include directory(ies), ZIP_INCLUDE_DIRS: ${PC_ZIP_INCLUDE_DIRS} .")
    message(STATUS "  library(ies): ZIP_LIBRARY_DIRS: ${PC_ZIP_LIBRARY_DIRS}; ZIP_LIBDIR: ${PC_ZIP_LIBDIR}. ")
  else()
    message(WARNING "Using pkg_config, failed to find any version of Zip library!")
  endif()
endif()

if((APPLE) AND (PKG_CONFIG_FOUND))
  PKG_SEARCH_MODULE(PC_HUNSPELL hunspell libhunspell)
  if(PC_HUNSPELL_FOUND)
    if(PC_HUNSPELL_hunspell_FOUND)
      message(STATUS "Using pkg_config, found \"hunspell\" version: ${PC_HUNSPELL_hunspell_VERSION} with:")
    elseif(PC_HUNSPELL_libhunspell_FOUND)
      message(STATUS "Using pkg_config, found \"libhunspell\" version: ${PC_HUNSPELL_libhunspell_VERSION} with:")
    else()
      message(STATUS "Using pkg_config, found Hunspell version: ${PC_HUNSPELL_VERSION} with:")
    endif()
    message(STATUS "  include directory(ies), HUNSPELL_INCLUDE_DIRS: ${PC_HUNSPELL_INCLUDE_DIRS} .")
    message(STATUS "  library(ies): HUNSPELL_LIBRARY_DIRS: ${PC_HUNSPELL_LIBRARY_DIRS}; HUNSPELL_LIBRARY_DIR: ${PC_HUNSPELL_LIBRARY_DIR}. ")
  elseif()
    message(WARNING "Using pkg_config, failed to find any version of Hunspell library!")
  endif()
endif()

find_package(Qt5 5.11 REQUIRED COMPONENTS Core Multimedia Network OpenGL UiTools Widgets Concurrent)
find_package(Qt5 COMPONENTS Gamepad QUIET)
find_package(Qt5 COMPONENTS TextToSpeech QUIET)

if (Qt5Core_VERSION VERSION_LESS 5.11)
  message(FATAL_ERROR "Mudlet requires Qt 5.11 or later, yours is ${Qt5Core_VERSION}")
else()
  # Qt Speech (or at least the TTS part) was officially release in 5.10
  message(STATUS "Qt version: ${Qt5Core_VERSION}")
endif()

message(STATUS "Using ${CMAKE_CXX_COMPILER_ID} compiler")

if(USE_FONTS)
  message(STATUS "Including additional font resources within the Mudlet executable")
else()
  message(STATUS "No font resources are to be included within the Mudlet executable")
endif()

if((CMAKE_SYSTEM_NAME STREQUAL "Linux") OR (CMAKE_SYSTEM_NAME STREQUAL "Windows") OR (CMAKE_SYSTEM_NAME STREQUAL "Darwin"))
  if(USE_UPDATER)
    message(STATUS "The updater code is included in this configuration")
  else(USE_UPDATER)
    message(STATUS "The updater code is excluded from this configuration")
  endif(USE_UPDATER)
else()
  message(STATUS "The Updater code is excluded as on-line updating is not available on this platform")
endif()

if((CMAKE_SYSTEM_NAME STREQUAL "Linux") OR (CMAKE_SYSTEM_NAME STREQUAL "Windows") OR (CMAKE_SYSTEM_NAME STREQUAL "Darwin"))
  if(USE_3DMAPPER)
    message(STATUS "The 3D mapper code is included in this configuration")
  else(USE_3DMAPPER)
    message(STATUS "The 3D mapper code is excluded from this configuration")
  endif(USE_3DMAPPER)
else()
  message(STATUS "The 3D mapper code is excluded as it might not be supported on this platform")
endif()

if(WIN32)
  set(mudlet_RC_ICONS = icons/mudlet_main_512x512_6XS_icon.ico)
  QT5_ADD_RESOURCES(RC_ICONS ${mudlet_RC_ICONS})
endif()

if(NOT(PC_ZIP_FOUND))
  find_package(ZIP REQUIRED)
endif()
find_package(OpenGL REQUIRED)
find_package(Lua51 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PCRE REQUIRED)
find_package(YAJL REQUIRED)
find_package(PUGIXML REQUIRED)

# Fall back on original FIND_PACKAGE process on Apple if Package Config failed
# Originally we did use that but it started to fail on Travis C.I. in June 2016
# So we now try to use PackageConfig for MacOs platform
if(APPLE)
  if(NOT(PC_HUNSPELL_FOUND))
    find_package(HUNSPELL REQUIRED)
  endif()
else()
  find_package(HUNSPELL REQUIRED)
endif()

find_package(Boost 1.44 COMPONENTS graph)

add_definitions(-DAPP_VERSION="${APP_VERSION}" -DAPP_BUILD="${APP_BUILD}" -DAPP_TARGET="${APP_TARGET}")

if(UNIX)
  set(LUA_DEFAULT_DIR "${CMAKE_INSTALL_PREFIX}/share/mudlet/lua")
  set(LCF_DIR "${CMAKE_INSTALL_PREFIX}/share/mudlet/lua/lcf")
endif(UNIX)

# Define a preprocessor symbol with the default fallback location from which
# to load installed mudlet lua files. Set LUA_DEFAULT_DIR to a
# platform-specific value. If LUA_DEFAULT_DIR is unset, the root directory
# will be used.
add_definitions(-DLUA_DEFAULT_PATH="${LUA_DEFAULT_DIR}")

# Enable leak detection for MSVC debug builds.
if(MSVC)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -FItestdbg.h")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(
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
    ${PUGIXML_INCLUDE_DIR}
)

if(Qt5TextToSpeech_FOUND)
  add_definitions(${Qt5TextToSpeech_DEFINITIONS})
  include_directories(${Qt5TextToSpeech_INCLUDE_DIRS})
  message(STATUS "Using TextToSpeech module")
endif(Qt5TextToSpeech_FOUND)

if(Qt5Gamepad_FOUND)
  add_definitions(${Qt5Gamepad_DEFINITIONS})
  include_directories(${Qt5Gamepad_INCLUDE_DIRS})
  message(STATUS "Using Gamepad module")
endif(Qt5Gamepad_FOUND)

# Need to use the plural variables as there can be more than ONE directory to specify:
if(PC_ZIP_FOUND)
  include_directories( ${PC_ZIP_INCLUDE_DIRS} )
else()
  include_directories( ${ZIP_INCLUDE_DIRS} )
endif()

include_directories( ${ZLIB_INCLUDE_DIR} )

QT5_WRAP_UI(mudlet_UIS_H ${mudlet_UIS})
QT5_WRAP_CPP(mudlet_MOC_SRCS ${mudlet_MOC_HDRS})
if(USE_FONTS)
  QT5_ADD_RESOURCES(mudlet_RCC_SRCS ${mudlet_RCCS} ${mudlet_fonts_RCCS})
else()
  QT5_ADD_RESOURCES(mudlet_RCC_SRCS ${mudlet_RCCS})
endif()

if(WIN32)
  add_executable(mudlet WIN32
    ${mudlet_SRCS}
    ${mudlet_MOC_SRCS}
    ${mudlet_RCC_SRCS}
    ${mudlet_HDRS}
    ${mudlet_UIS_H}
    ${mudlet_MOC_HDRS}
    ${mudlet_LUA_FILES})
else()
  add_executable(mudlet
    ${mudlet_SRCS}
    ${mudlet_MOC_SRCS}
    ${mudlet_RCC_SRCS}
    ${mudlet_HDRS}
    ${mudlet_UIS_H}
    ${mudlet_MOC_HDRS}
    ${mudlet_LUA_FILES})
endif()

target_link_libraries(mudlet
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
    ${PUGIXML_LIBRARIES}
    communi
    edbee-lib
    qt5keychain
)

if(Qt5TextToSpeech_FOUND)
  target_link_libraries(mudlet ${Qt5TextToSpeech_LIBRARIES})
endif(Qt5TextToSpeech_FOUND)

if(USE_UPDATER)
  target_link_libraries(mudlet dblsqd)
endif(USE_UPDATER)

if(Qt5Gamepad_FOUND)
  target_link_libraries(mudlet ${Qt5Gamepad_LIBRARIES})
endif(Qt5Gamepad_FOUND)

if(PC_ZIP_FOUND)
  target_link_libraries(mudlet ${PC_ZIP_LIBRARIES})
else()
  target_link_libraries(mudlet ${ZIP_LIBRARIES})
endif()

if(WIN32)
    target_link_libraries(mudlet ws2_32
    )
endif()

if(APPLE)
  target_link_libraries(mudlet luazip)

  if(USE_UPDATER)
    target_link_libraries(mudlet
      cocoa-qt-glue
      "-framework Sparkle"
      "-framework AppKit"
      "-F${CMAKE_HOME_DIRECTORY}/3rdparty/cocoapods/Pods/Sparkle")
  endif(USE_UPDATER)
endif()

if(PC_HUNSPELL_FOUND)
  target_link_libraries(mudlet ${PC_HUNSPELL_LIBRARIES})
else()
  target_link_libraries(mudlet ${HUNSPELL_LIBRARIES})
endif()

# *nix "make install" provision:
if(UNIX)
  # CMAKE_INSTALL_PREFIX is automagically set on Unix to DESTDIR environment
  # variable and is prefixed onto relative DESTINATION values
  foreach(lua_file ${lua_files})
    get_filename_component(lua_file_name ${lua_file} NAME)
    get_filename_component(lua_file_path ${lua_file} DIRECTORY)
    install(FILES "${CMAKE_HOME_DIRECTORY}/src/mudlet-lua/lua/${lua_file}"
      DESTINATION "share/mudlet/lua/${lua_file_path}"
      PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
      CONFIGURATIONS Debug Release)
  endforeach()
  foreach(lua_test_file ${lua_test_files})
    get_filename_component(lua_test_file_name ${lua_test_file} NAME)
    get_filename_component(lua_test_file_path ${lua_test_file} DIRECTORY)
    install(FILES "${CMAKE_HOME_DIRECTORY}/src/mudlet-lua/tests/${lua_test_file}"
      DESTINATION "share/mudlet/tests/${lua_test_file_path}"
      PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
      CONFIGURATIONS Debug Release)
  endforeach()
  foreach(lcf_file ${lcf_files})
    get_filename_component(lcf_file_name ${lcf_file} NAME)
    get_filename_component(lcf_file_path ${lcf_file} DIRECTORY)
    install(FILES "${CMAKE_HOME_DIRECTORY}/3rdparty/${lcf_file}"
      DESTINATION "share/mudlet/lua/${lcf_file_path}"
      PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
      CONFIGURATIONS Debug Release)
  endforeach(lcf_file)
  install(TARGETS mudlet
    RUNTIME
    DESTINATION "bin"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    CONFIGURATIONS Debug Release
    COMPONENT All)
endif()
