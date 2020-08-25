############################################################################
#    Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       #
#    Copyright (C) 2015-2018, 2020 by Stephen Lyons                        #
#                                                - slysven@virginmedia.com #
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
  list(APPEND mudlet_RCCS mudlet_fonts_common.qrc)
  if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    list(APPEND mudlet_RCCS mudlet_fonts_linux.qrc)
  endif()
endif()

if(WIN32)
  list(APPEND mudlet_RCCS icons/mudlet_main_512x512_6XS_icon.ico)
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
    dlgSourceEditorFindArea.cpp
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
    TEncodingTable.cpp
    TEntityHandler.cpp
    TEntityResolver.cpp
    TFlipButton.cpp
    TForkedProcess.cpp
    TimerUnit.cpp
    TKey.cpp
    TLabel.cpp
    TLinkStore.cpp
    TLuaInterpreter.cpp
    TMap.cpp
    TMedia.cpp
    TMxpElementDefinitionHandler.cpp
    TMxpElementRegistry.cpp
    TMxpFormattingTagsHandler.cpp
    TMxpBRTagHandler.cpp
    TMxpColorTagHandler.cpp
    TMxpCustomElementTagHandler.cpp
    TMxpEntityTagHandler.cpp
    TMxpFontTagHandler.cpp
    TMxpLinkTagHandler.cpp
    TMxpMusicTagHandler.cpp
    TMxpNodeBuilder.cpp
    TMxpMudlet.cpp
    TMxpProcessor.cpp
    TMxpSendTagHandler.cpp
    TMxpSoundTagHandler.cpp
    TMxpSupportTagHandler.cpp
    MxpTag.cpp
    TMxpTagHandler.cpp
    TMxpTagParser.cpp
    TMxpTagProcessor.cpp
    TMxpVarTagHandler.cpp
    TMxpVersionTagHandler.cpp
    TriggerUnit.cpp
    TRoom.cpp
    TRoomDB.cpp
    TScript.cpp
    TSplitter.cpp
    TSplitterHandle.cpp
    TStringUtils.cpp
    TTabBar.cpp
    TTextCodec.cpp
    TTextEdit.cpp
    TTimer.cpp
    TToolBar.cpp
    TTreeWidget.cpp
    TTrigger.cpp
    TVar.cpp
    VarUnit.cpp
    XMLexport.cpp
    XMLimport.cpp)

# This is for compiled UI files, not those used at runtime
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
    ui/source_editor_find_area.ui
    ui/system_message_area.ui
    ui/timers_main_area.ui
    ui/triggers_main_area.ui
    ui/trigger_editor.ui
    ui/trigger_pattern_edit.ui
    ui/vars_main_area.ui)

set(mudlet_HDRS
    ActionUnit.h
    AliasUnit.h
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
    dlgSourceEditorFindArea.h
    dlgSystemMessageArea.h
    dlgTimersMainArea.h
    dlgTriggerEditor.h
    dlgTriggerPatternEdit.h
    dlgTriggersMainArea.h
    dlgVarsMainArea.h
    EAction.h
    exitstreewidget.h
    FontManager.h
    Host.h
    HostManager.h
    ircmessageformatter.h
    KeyUnit.h
    LuaInterface.h
    mudlet.h
    post_guard.h
    pre_guard.h
    ScriptUnit.h
    T2DMap.h
    TAction.h
    TAlias.h
    TArea.h
    TAstar.h
    TBuffer.h
    TCommandLine.h
    TConsole.h
    TDebug.h
    TDockWidget.h
    TEasyButtonBar.h
    TEncodingTable.h
    TEntityHandler.h
    TEntityResolver.h
    testdbg.h
    TEvent.h
    TForkedProcess.h
    TFlipButton.h
    TimerUnit.h
    TKey.h
    TLabel.h
    TLinkStore.h
    TLuaInterpreter.h
    TMap.h
    TMatchState.h
    TMedia.h
    TMxpBRTagHandler.h
    TMxpClient.h
    TMxpColorTagHandler.h
    TMxpCustomElementTagHandler.h
    TMxpEntityTagHandler.h
    TMxpFontTagHandler.h
    TMxpLinkTagHandler.h
    TMxpMusicTagHandler.h
    TMxpSoundTagHandler.h
    TMxpElementDefinitionHandler.h
    TMxpElementRegistry.h
    TMxpContext.h
    TMxpFormattingTagsHandler.h
    TMxpMudlet.h
    TMxpNodeBuilder.h
    TMxpProcessor.h
    TMxpSendTagHandler.h
    MxpTag.h
    TMxpTagHandler.h
    TMxpTagParser.h
    TMxpTagProcessor.h
    TMxpSupportTagHandler.cpp
    TMxpVarTagHandler.h
    TMxpVersionTagHandler.h
    Tree.h
    TriggerUnit.h
    TRoom.h
    TRoomDB.h
    TScript.h
    TSplitter.h
    TSplitterHandle.h
    TStringUtils.h
    TTabBar.h
    TTextCodec.h
    TTextEdit.h
    TTimer.h
    TToolBar.h
    TTreeWidget.h
    TTrigger.h
    TVar.h
    VarUnit.h
    widechar_width.h
    XMLexport.h
    XMLimport.h)

if(USE_UPDATER)
  list(APPEND mudlet_SRCS updater.cpp)
  list(APPEND mudlet_HDRS updater.h)
endif(USE_UPDATER)

if(USE_3DMAPPER)
  list(APPEND mudlet_SRCS glwidget.cpp)
  list(APPEND mudlet_HDRS glwidget.h)
endif(USE_3DMAPPER)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(
  Qt5 5.11 REQUIRED
  COMPONENTS Core
             Multimedia
             Network
             OpenGL
             UiTools
             Widgets
             Concurrent)
find_package(Qt5 COMPONENTS Gamepad QUIET)
find_package(Qt5 COMPONENTS TextToSpeech QUIET)

if(Qt5Core_VERSION VERSION_LESS 5.11)
  message(
    FATAL_ERROR "Mudlet requires Qt 5.11 or later, yours is ${Qt5Core_VERSION}")
else()
  # Qt Speech (or at least the TTS part) was officially release in 5.10
  message(STATUS "Qt version: ${Qt5Core_VERSION}")
endif()

message(STATUS "Using ${CMAKE_CXX_COMPILER_ID} compiler")

find_package(ZIP REQUIRED)
find_package(Lua51 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PCRE REQUIRED)
find_package(YAJL REQUIRED)
find_package(PUGIXML REQUIRED)
find_package(HUNSPELL REQUIRED)
find_package(Boost 1.44)

if(NOT USE_OWN_QT5KEYCHAIN)
  # Only use this when we are using a system package and NOT our own local build
  find_package(Qt5Keychain REQUIRED)

  # No need to test for it being found as the REQUIRED will make it error out
  # if it is NOT found!
  message(STATUS "Using system QtKeyChain (Qt5) library (found version \"${Qt5Keychain_VERSION}\").")
endif()

if(USE_3DMAPPER)
  if(POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW)
  endif()
  find_package(OpenGL REQUIRED)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(mudlet ${mudlet_SRCS} ${mudlet_RCCS} ${mudlet_HDRS}
                      ${mudlet_UIS})

set_target_properties(
  mudlet
  PROPERTIES AUTOMOC ON AUTOUIC ON AUTOUIC_SEARCH_PATHS
                                   "${CMAKE_CURRENT_SOURCE_DIR}/ui" AUTORCC ON
             POSITION_INDEPENDENT_CODE ON)

if(USE_FONTS)
  target_compile_definitions(mudlet PRIVATE INCLUDE_FONTS)
endif()

if(UNIX)
  set(LUA_DEFAULT_DIR "${CMAKE_INSTALL_PREFIX}/share/mudlet/lua")
endif(UNIX)

# Define a preprocessor symbol with the default fallback location from which to
# load installed mudlet lua files. Set LUA_DEFAULT_DIR to a platform-specific
# value. If LUA_DEFAULT_DIR is unset, the root directory will be used.
target_compile_definitions(mudlet
  PRIVATE
    APP_VERSION="${APP_VERSION}"
    APP_BUILD="${APP_BUILD}"
    APP_TARGET="${APP_TARGET}"
    LUA_DEFAULT_PATH="${LUA_DEFAULT_DIR}")

if(USE_OWN_QT5KEYCHAIN)
    target_compile_definitions(mudlet PRIVATE INCLUDE_OWN_QT5_KEYCHAIN QTKEYCHAIN_NO_EXPORT)
endif()

target_link_libraries(
  mudlet
  communi
  edbee-lib
  Boost::boost
  HUNSPELL::HUNSPELL
  LUA51::LUA51
  PCRE::PCRE
  PUGIXML::PUGIXML
  Qt5::Concurrent
  Qt5::Core
  Qt5::Network
  Qt5::Multimedia
  Qt5::OpenGL
  Qt5::UiTools
  Qt5::Widgets
  YAJL::YAJL
  ZIP::ZIP
  ZLIB::ZLIB
  qt5keychain)

if(Qt5TextToSpeech_FOUND)
  target_link_libraries(mudlet Qt5::TextToSpeech)
  message(STATUS "Using TextToSpeech module")
endif(Qt5TextToSpeech_FOUND)

if(Qt5Gamepad_FOUND)
  target_link_libraries(mudlet Qt5::Gamepad)
  message(STATUS "Using Gamepad module")
endif(Qt5Gamepad_FOUND)

if(USE_UPDATER)
  target_link_libraries(mudlet dblsqd)
  target_compile_definitions(mudlet PRIVATE INCLUDE_UPDATER)
endif(USE_UPDATER)

if(USE_3DMAPPER)
  target_link_libraries(mudlet OpenGL::GLU)
  target_compile_definitions(mudlet PRIVATE INCLUDE_3DMAPPER)
endif()

if(USE_VARIABLE_SPLASH_SCREEN)
  target_compile_definitions(mudlet PRIVATE INCLUDE_VARIABLE_SPLASH_SCREEN)
endif()

if(WIN32)
  target_link_libraries(mudlet ws2_32)
  set_target_properties(mudlet PROPERTIES WIN32_EXECUTABLE ON)
  if(USE_MAIN_BUILD_SYSTEM)
    target_compile_definitions(mudlet PRIVATE INCLUDE_MAIN_BUILD_SYSTEM)
  endif()
  # The UNICODE and _UNICODE definitions are needed to make
  # (QString) mudlet::getShortPathName(const QString& name) work in Windows:
  target_compile_definitions(mudlet PRIVATE UNICODE _UNICODE)
endif()

if(APPLE)
  if(USE_UPDATER)
    target_link_libraries(mudlet cocoa-qt-glue "-framework AppKit")
  endif(USE_UPDATER)
  set_target_properties(mudlet PROPERTIES MACOSX_BUNDLE ON
                                          MACOSX_BUNDLE_BUNDLE_NAME Mudlet)
endif()

target_compile_options(mudlet PRIVATE -Wno-deprecated)

# *nix "make install" provision:
if(UNIX AND NOT APPLE)
  # CMAKE_INSTALL_PREFIX is automagically set on Unix to DESTDIR environment
  # variable and is prefixed onto relative DESTINATION values
  install(
    DIRECTORY "mudlet-lua/lua"
    DESTINATION "share/mudlet"
    FILES_MATCHING
    PATTERN "*.lua" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
  install(
    DIRECTORY "mudlet-lua/tests"
    DESTINATION "share/mudlet"
    FILES_MATCHING
    PATTERN "*.lua" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
  install(
    DIRECTORY "../3rdparty/lcf"
    DESTINATION "share/mudlet/lua"
    PATTERN ".git" EXCLUDE PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ
                                       WORLD_READ)
  install(
    TARGETS mudlet
    RUNTIME
      DESTINATION "bin"
      PERMISSIONS
        OWNER_READ
        OWNER_WRITE
        OWNER_EXECUTE
        GROUP_READ
        GROUP_EXECUTE
        WORLD_READ
        WORLD_EXECUTE
      CONFIGURATIONS Debug Release
      COMPONENT All)
endif()
