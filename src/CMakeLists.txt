############################################################################
#    Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       #
#    Copyright (C) 2015-2018, 2020, 2023 by Stephen Lyons                  #
#                                                - slysven@virginmedia.com #
#    Copyright (C) 2018 by Huadong Qi - novload@outlook.com                #
#    Copyright (C) 2022 by Thiago Jung Bauermann - bauermann@kolabnow.com  #
#    Copyright (C) 2022 by Piotr Wilczynski - delwing@gmail.com            #
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
configure_file("${mudlet_SOURCE_DIR}/translations/translated/qm.qrc" "${mudlet_BINARY_DIR}/translations/translated/qm.qrc" COPYONLY)

if(USE_VARIABLE_SPLASH_SCREEN)
  list(APPEND mudlet_RCCS additional_splash_screens.qrc)
endif()

if(USE_FONTS)
  list(APPEND mudlet_RCCS mudlet_fonts_common.qrc)
  if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    list(APPEND mudlet_RCCS mudlet_fonts_linux.qrc)
  endif()
endif()

if(WIN32)
  # APP_BUILD should now be be available

  if(APP_BUILD matches "-ptb.+")
    list(APPEND mudlet_RCCS icons/mudlet_ptb.ico)
  elseif((APP_BUILD matches "-dev.+") or (APP_BUILD matches "-test.+"))
    # The above test may not be correct as it won't pick up builds where
    # MUDLET_VERSION_BUILD has been set in the environment - OTOH for
    # "Packagers" they will likely want the original icon anyhow for their
    # modified for their system packages of Mudlet:
    list(APPEND mudlet_RCCS icons/mudlet_dev.ico)
  else()
    list(APPEND mudlet_RCCS icons/mudlet.ico)
  endif()
endif()

set(mudlet_SRCS
    ActionUnit.cpp
    AliasUnit.cpp
    AltFocusMenuBarDisable.cpp
    ctelnet.cpp
    DarkTheme.cpp
    discord.cpp
    dlgAboutDialog.cpp
    dlgActionMainArea.cpp
    dlgAliasMainArea.cpp
    dlgColorTrigger.cpp
    dlgComposer.cpp
    dlgConnectionProfiles.cpp
    dlgIRC.cpp
    dlgKeysMainArea.cpp
    dlgMapLabel.cpp
    dlgMapper.cpp
    dlgModuleManager.cpp
    dlgNotepad.cpp
    dlgPackageExporter.cpp
    dlgPackageManager.cpp
    dlgProfilePreferences.cpp
    dlgRoomExits.cpp
    dlgRoomProperties.cpp
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
    FileOpenHandler.cpp
    FontManager.cpp
    GifTracker.cpp
    GMCPAuthenticator.cpp
    Host.cpp
    HostManager.cpp
    ircmessageformatter.cpp
    KeyUnit.cpp
    LuaInterface.cpp
    main.cpp
    mapInfoContributorManager.cpp
    mudlet.cpp
    MudletInstanceCoordinator.cpp
    MxpTag.cpp
    ScriptUnit.cpp
    ShortcutsManager.cpp
    T2DMap.cpp
    TAccessibleTextEdit.cpp
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
    TLuaInterpreterDiscord.cpp
    TLuaInterpreterMapper.cpp
    TLuaInterpreterMedia.cpp
    TLuaInterpreterMudletObjects.cpp
    TLuaInterpreterNetworking.cpp
    TLuaInterpreterTextToSpeech.cpp
    TLuaInterpreterUI.cpp
    TMainConsole.cpp
    TMap.cpp
    TMapLabel.cpp
    TMedia.cpp
    TMediaPlaylist.cpp
    TMxpElementDefinitionHandler.cpp
    TMxpElementRegistry.cpp
    TMxpFormattingTagsHandler.cpp
    TMxpBRTagHandler.cpp
    TMxpColorTagHandler.cpp
    TMxpCustomElementTagHandler.cpp
    TMxpElementDefinitionHandler.cpp
    TMxpElementRegistry.cpp
    TMxpEntityTagHandler.cpp
    TMxpFontTagHandler.cpp
    TMxpFormattingTagsHandler.cpp
    TMxpLinkTagHandler.cpp
    TMxpMudlet.cpp
    TMxpMusicTagHandler.cpp
    TMxpNodeBuilder.cpp
    TMxpProcessor.cpp
    TMxpSendTagHandler.cpp
    TMxpSoundTagHandler.cpp
    TMxpSupportTagHandler.cpp
    TMxpTagHandler.cpp
    TMxpTagParser.cpp
    TMxpTagProcessor.cpp
    TMxpVarTagHandler.cpp
    TMxpVersionTagHandler.cpp
    TrailingWhitespaceMarker.cpp
    TriggerUnit.cpp
    TRoom.cpp
    TRoomDB.cpp
    TScript.cpp
    TScrollBox.cpp
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

if(APPLE)
  list(APPEND mudlet_SRCS AnnouncerMac.mm)
  # Sparkle updater for macOS
  list(APPEND mudlet_SRCS sparkleupdater.mm)
  list(APPEND mudlet_HDRS sparkleupdater.h)
  set_source_files_properties(
    src/sparkleupdater.mm
    src/sparkleupdater.h
    PROPERTIES
    LANGUAGE OBJCXX
    COMPILE_OPTIONS "-fobjc-arc"
)
elseif(WIN32)
  list(APPEND mudlet_SRCS AnnouncerWindows.cpp)
  list(APPEND mudlet_SRCS uiawrapper.cpp)
else()
  list(APPEND mudlet_SRCS AnnouncerUnix.cpp)
endif()

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
    ui/module_manager.ui
    ui/map_label.ui
    ui/mapper.ui
    ui/notes_editor.ui
    ui/package_manager.ui
    ui/profile_preferences.ui
    ui/room_exits.ui
    ui/room_properties.ui
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
    AltFocusMenuBarDisable.h
    Announcer.h
    ctelnet.h
    DarkTheme.h
    discord.h
    dlgAboutDialog.h
    dlgActionMainArea.h
    dlgAliasMainArea.h
    dlgColorTrigger.h
    dlgComposer.h
    dlgConnectionProfiles.h
    dlgIRC.h
    dlgKeysMainArea.h
    dlgMapLabel.h
    dlgMapper.h
    dlgModuleManager.h
    dlgNotepad.h
    dlgPackageExporter.h
    dlgPackageManager.h
    dlgProfilePreferences.h
    dlgRoomExits.h
    dlgRoomProperties.h
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
    FileOpenHandler.h
    FontManager.h
    GifTracker.h
    GMCPAuthenticator.h
    Host.h
    HostManager.h
    ircmessageformatter.h
    KeyUnit.h
    LuaInterface.h
    mapInfoContributorManager.h
    mudlet.h
    MudletInstanceCoordinator.h
    MxpTag.h
    post_guard.h
    pre_guard.h
    ScriptUnit.h
    ShortcutsManager.h
    T2DMap.h
    TAccessibleConsole.h
    TAccessibleTextEdit.h
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
    TFlipButton.h
    TForkedProcess.h
    TimerUnit.h
    TKey.h
    TLabel.h
    TLinkStore.h
    TLuaInterpreter.h
    TMainConsole.h
    TMap.h
    TMapLabel.h
    TMatchState.h
    TMedia.h
    TMediaPlaylist.h
    TMxpBRTagHandler.h
    TMxpClient.h
    TMxpColorTagHandler.h
    TMxpContext.h
    TMxpCustomElementTagHandler.h
    TMxpElementDefinitionHandler.h
    TMxpElementRegistry.h
    TMxpEntityTagHandler.h
    TMxpFontTagHandler.h
    TMxpFormattingTagsHandler.h
    TMxpLinkTagHandler.h
    TMxpMudlet.h
    TMxpMusicTagHandler.h
    TMxpNodeBuilder.h
    TMxpProcessor.h
    TMxpSendTagHandler.h
    TMxpSoundTagHandler.h
    TMxpSupportTagHandler.cpp
    TMxpTagHandler.h
    TMxpTagParser.h
    TMxpTagProcessor.h
    TMxpVarTagHandler.h
    TMxpVersionTagHandler.h
    TrailingWhitespaceMarker.h
    Tree.h
    TriggerUnit.h
    TRoom.h
    TRoomDB.h
    TScript.h
    TScrollBox.h
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
    utils.h
    VarUnit.h
    widechar_width.h
    XMLexport.h
    XMLimport.h)

if(WIN32)
  list(APPEND mudlet_HDRS uiawrapper.h)
endif()

if(USE_UPDATER)
  list(APPEND mudlet_SRCS updater.cpp)
  list(APPEND mudlet_HDRS updater.h)
endif(USE_UPDATER)

if(USE_3DMAPPER)
  list(APPEND mudlet_SRCS glwidget.cpp)
  list(APPEND mudlet_HDRS glwidget.h)
endif(USE_3DMAPPER)

# credit to https://github.com/heavyai/heavydb/blob/72c90bc290b79dd30240da41c103a00720f6b050/CMakeLists.txt#L119
macro(set_alternate_linker linker)
  find_program(LINKER_EXECUTABLE ld.${USE_ALTERNATE_LINKER} ${USE_ALTERNATE_LINKER})
  if(LINKER_EXECUTABLE)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS 12.1.0 AND "${USE_ALTERNATE_LINKER}" STREQUAL "mold")
      # LINKER_EXECUTABLE will be a full path to ld.mold, so we replace the end of the path, resulting in the relative
      # libexec/mold dir, and tell GCC to look there first for an override version of executables, in this case, ld
      string(REPLACE "bin/ld.mold" "libexec/mold" PATH_TO_LIBEXEC_MOLD ${LINKER_EXECUTABLE})
      add_link_options("-B${PATH_TO_LIBEXEC_MOLD}")
    else()
      add_link_options("-fuse-ld=${USE_ALTERNATE_LINKER}")
    endif()
  else()
    set(USE_ALTERNATE_LINKER "" CACHE STRING "Use alternate linker" FORCE)
  endif()
endmacro()

set(USE_ALTERNATE_LINKER "" CACHE STRING "Use alternate linker. Leave empty for system default; alternatives are 'gold', 'lld', 'bfd', 'mold'")
if(NOT "${USE_ALTERNATE_LINKER}" STREQUAL "")
  set_alternate_linker(${USE_ALTERNATE_LINKER})
endif()

# Enable sanitizers support
include(../3rdparty/cmake-scripts/sanitizers.cmake)

set_sanitizer_options(address DEFAULT -fno-omit-frame-pointer)
set_sanitizer_options(thread DEFAULT -fno-omit-frame-pointer)

set(USE_SANITIZER
      "address"
      CACHE STRING
      "Compile with sanitizers. Available sanitizers are: Address, Memory, MemoryWithOrigins, Undefined, Thread, or Leak")

add_sanitizer_support(${USE_SANITIZER})
message(STATUS "Compiling with the following sanitizer(s) enabled: ${USE_SANITIZER}")


if(WITH_QT6)
find_package(
  Qt6 6.2.0 REQUIRED
  COMPONENTS Core
             Multimedia
             Network
             OpenGL
             UiTools
             Widgets
             Concurrent
             Core5Compat)
  find_package(Qt6 COMPONENTS TextToSpeech QUIET)

  message(STATUS "Qt version: ${Qt6Core_VERSION}")

  set(QT_MAJOR_VERSION 6)
  set(QT_LIBS Qt6::Concurrent
              Qt6::Core
              Qt6::Network
              Qt6::Multimedia
              Qt6::OpenGL
              Qt6::UiTools
              Qt6::Widgets
              Qt6::Core5Compat
              qt6keychain)

  if(Qt6TextToSpeech_FOUND)
    list(APPEND QT_LIBS Qt6::TextToSpeech)
    message(STATUS "Using TextToSpeech module")
  endif()
else()
  find_package(
          Qt5 5.14 REQUIRED
          COMPONENTS Core
          Multimedia
          Network
          OpenGL
          UiTools
          Widgets
          Concurrent)
  find_package(Qt5 COMPONENTS TextToSpeech QUIET)

  message(STATUS "Qt version: ${Qt5Core_VERSION}")

  set(QT_MAJOR_VERSION 5)
  set(QT_LIBS Qt5::Concurrent
              Qt5::Core
              Qt5::Network
              Qt5::Multimedia
              Qt5::OpenGL
              Qt5::UiTools
              Qt5::Widgets
              qt5keychain)

  if(Qt5TextToSpeech_FOUND)
    list(APPEND QT_LIBS Qt5::TextToSpeech)
    message(STATUS "Using TextToSpeech module")
  endif()
endif()

message(STATUS "Using ${CMAKE_CXX_COMPILER_ID} compiler")

find_package(ZIP REQUIRED)
find_package(Lua51 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PCRE REQUIRED)
find_package(PUGIXML REQUIRED)
find_package(HUNSPELL REQUIRED)
find_package(Boost 1.44)


# FreeBSD's sysinfo() call is done in a package, libsysinfo.
# Find the library and include it appropriately.

if (CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
  find_library(SYSINFO_LIBRARY NAMES sysinfo)
  find_path(SYSINFO_INCLUDE_DIR NAMES sys/sysinfo.h)
  find_library(KVM_LIBRARY NAMES kvm)
endif()

if(NOT USE_OWN_QTKEYCHAIN)
  # Only use this when we are using a system package and NOT our own local build
  find_package(Qt${QT_MAJOR_VERSION}Keychain REQUIRED)

  # No need to test for it being found as the REQUIRED will make it error out
  # if it is NOT found!
  message(STATUS "Using system QtKeyChain (Qt${QT_MAJOR_VERSION}) library (found version \"${Qt${QT_MAJOR_VERSION}Keychain_VERSION}\").")
endif()

if(USE_3DMAPPER)
  if(POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW)
  endif()
  find_package(OpenGL REQUIRED)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(mudlet ${mudlet_SRCS} ${mudlet_RCCS} ${mudlet_HDRS} ${mudlet_UIS})

########################### Debugging code inclusions ##########################
# Controls the include/selection of extra code to aide developers debug various
# features of Mudlet, generally speaking they will not be '#define'd in
# production builds - uncomment the
# 'target_compile_definitions(mudlet PRIVATE DEBUG__XXX)'
# line here to include the relevant code:
#
# Other DEFINES+=DEBUG_XXX expected here as a result of:
# https://github.com/Mudlet/Mudlet/issues/6314
#
# * Produce a timestamped message on the OS's command line when an autosave of
#   the map file gets requested and when such a request gets actioned (or
#   rejected because the profile is being loaded):
# target_compile_definitions(mudlet PRIVATE DEBUG_MAPAUTOSAVE)
#
# * Produce all the time the surprise that normally will only occur on the
#   first day of the fourth month of the Gregorian calendar year:
# target_compile_definitions(mudlet PRIVATE DEBUG_EASTER_EGGS)
#
# * Comment this to not get debugging messages about WILL/WONT/DO/DONT and other
#   commands for suboptions - change the value to 2 to get a bit more detail
#   about the size or nature of the command:
target_compile_definitions(mudlet PRIVATE DEBUG_TELNET=1)
#
# * Produce qDebug() messages about the decoding of UTF-8 data when it is not
#   the single bytes of pure ASCII text:
# target_compile_definitions(mudlet PRIVATE DEBUG_UTF8_PROCESSING)
#
# * Produce qDebug() messages about the decoding of GB2312/GBK/GB18030 data when
#   it is not the single bytes of pure ASCII text:
# target_compile_definitions(mudlet PRIVATE DEBUG_GB_PROCESSING)
#
# * Produce qDebug() messages about the decoding of BIG5 data when it is not the
#   single bytes of pure ASCII text:
# target_compile_definitions(mudlet PRIVATE DEBUG_BIG5_PROCESSING)
#
# * Produce qDebug() messages about the decoding of EUC-KR data when it is not
#   the single bytes of pure ASCII text:
# target_compile_definitions(mudlet PRIVATE DEBUG_EUC_KR_PROCESSING)
#
# * Produce qDebug() messages about the decoding of ANSI SGR sequences:
#   target_compile_definitions(mudlet PRIVATE DEBUG_SGR_PROCESSING)
#
# * Produce qDebug() messages about the decoding of ANSI OSC sequences:
# target_compile_definitions(mudlet PRIVATE DEBUG_OSC_PROCESSING)
#
# * Produce qDebug() messages about the decoding of ANSI MXP sequences although
#   there is not much against this item at present {only an announcement of the
#   type (?) of an `\x1b[?z` received}:
# target_compile_definitions(mudlet PRIVATE DEBUG_MXP_PROCESSING)
#
# * Enable the features associated with reporting problems in processing Unicode
#   codepoints that cannot be displayed on screen in a `TConsole`:
# target_compile_definitions(mudlet PRIVATE DEBUG_CODEPOINT_PROBLEMS)

target_sources(mudlet PRIVATE "${mudlet_BINARY_DIR}/translations/translated/qm.qrc")

set_target_properties(mudlet PROPERTIES
  AUTOUIC ON AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui"
  AUTOMOC ON AUTORCC ON POSITION_INDEPENDENT_CODE ON
  AUTOGEN_ORIGIN_DEPENDS OFF)

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
    APP_TARGET="${APP_TARGET}"
    LUA_DEFAULT_PATH="${LUA_DEFAULT_DIR}")

if(USE_OWN_QTKEYCHAIN)
    target_compile_definitions(mudlet PRIVATE INCLUDE_OWN_QT5_KEYCHAIN QTKEYCHAIN_NO_EXPORT)
endif()

# Used to force an include of winsock2.h BEFORE Qt tries to include winsock.h
# from windows.h - only needed on Windows builds but we cannot use Q_OS_WIN32
# for an #ifdef because we need a symbol that is defined BEFORE we include
# any Qt header file!
if(WIN32)
    target_compile_definitions(mudlet PRIVATE INCLUDE_WINSOCK2)
endif()

# One of IRC_STATIC or IRC_SHARED must be defined for the communi IRC library
target_compile_definitions(communi PUBLIC IRC_STATIC)

target_link_libraries(
        mudlet
        communi
        edbee-lib
        Boost::boost
        HUNSPELL::HUNSPELL
        LUA51::LUA51
        PCRE::PCRE
        PUGIXML::PUGIXML
        ZIP::ZIP
        ZLIB::ZLIB
# PLACEMARKER: sample benchmarking code
#       nanobench
        ${QT_LIBS})

if (SYSINFO_LIBRARY)
  target_link_libraries(mudlet ${SYSINFO_LIBRARY} ${KVM_LIBRARY})
endif (SYSINFO_LIBRARY)

if (SYSINFO_INCLUDE_DIR)
  target_include_directories(mudlet INTERFACE ${SYSINFO_INCLUDE_DIR})
endif (SYSINFO_INCLUDE_DIR)

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
  # The UNICODE and _UNICODE definitions are needed to make
  # (QString) mudlet::getShortPathName(const QString& name) work in Windows:
  target_compile_definitions(mudlet PRIVATE UNICODE _UNICODE)
endif()

if(APPLE)
  target_link_libraries(mudlet "-framework AppKit")

  if(USE_UPDATER)
    # need to specifically call out Objective-C sources so that automoc builds them.
    target_sources(mudlet PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/sparkleupdater.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/sparkleupdater.mm")
    find_package(Sparkle)
    target_link_libraries(mudlet "-framework AppKit" Sparkle::Sparkle)
  endif(USE_UPDATER)

  set_target_properties(mudlet PROPERTIES
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_BUNDLE_NAME Mudlet
    MACOSX_BUNDLE_BUNDLE_VERSION ${APP_VERSION}${APP_BUILD}
    MACOSX_BUNDLE_GUI_IDENTIFIER org.mudlet.mudlet
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${APP_VERSION}
  )

  if(${APP_BUILD} MATCHES "-ptb.+")
    set_target_properties(mudlet PROPERTIES
      MACOSX_BUNDLE_ICON_FILE mudlet_ptb.icns
      MACOSX_BUNDLE_LONG_VERSION_STRING ${APP_VERSION}${APP_BUILD}
    )
    file(GLOB ICON_FILE "icons/mudlet_ptb.icns")
  elseif((${APP_BUILD} MATCHES "-dev.+") OR (${APP_BUILD} MATCHES "-test.+"))
    set_target_properties(mudlet PROPERTIES
      MACOSX_BUNDLE_ICON_FILE mudlet_dev.icns
      MACOSX_BUNDLE_LONG_VERSION_STRING ${APP_VERSION}${APP_BUILD}
    )
    file(GLOB ICON_FILE "icons/mudlet_dev.icns")
  else()
    set_target_properties(mudlet PROPERTIES
      MACOSX_BUNDLE_ICON_FILE mudlet.icns
      MACOSX_BUNDLE_LONG_VERSION_STRING ${APP_VERSION}
    )
    file(GLOB ICON_FILE "icons/mudlet.icns")
  endif()

  file(GLOB_RECURSE MUDLET_LUA_FILES LIST_DIRECTORIES true "mudlet-lua/*")
  file(GLOB_RECURSE LUA_TRANSLATIONS LIST_DIRECTORIES true "../translations/lua/*")
  file(GLOB DIC_FILES "*.dic")
  file(GLOB AFF_FILES "*.aff")
  target_sources(mudlet PRIVATE ${MUDLET_LUA_FILES} ${LUA_TRANSLATIONS} ${DIC_FILES} ${AFF_FILES} ${ICON_FILE})

  set_source_files_properties(${DIC_FILES} ${AFF_FILES} ${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  foreach(FILE ${MUDLET_LUA_FILES})
    string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR} "" PATH_IN_BUNDLE ${FILE} )
    get_filename_component(PATH_IN_BUNDLE "${PATH_IN_BUNDLE}" DIRECTORY)
    set_source_files_properties(${FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/${PATH_IN_BUNDLE})
  endforeach()
  foreach(FILE ${LUA_TRANSLATIONS})
    # convert relative paths to absolute ones, necessary for the ../translations
    file(REAL_PATH ${FILE} FILE)
    string(REPLACE ${CMAKE_SOURCE_DIR}/translations "" PATH_IN_BUNDLE ${FILE} )
    get_filename_component(PATH_IN_BUNDLE "${PATH_IN_BUNDLE}" DIRECTORY)
    set_source_files_properties(${FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/translations/${PATH_IN_BUNDLE})
  endforeach()
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
    FILES "../mudlet.desktop"
    DESTINATION "share/applications" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ
      WORLD_READ)

  install(
    FILES "../mudlet.desktop"
    DESTINATION "share/applications" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ
      WORLD_READ)
  install(
    FILES "../mudlet.svg"
    DESTINATION "share/icons/hicolor/scalable/apps" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ
    WORLD_READ)
  install(
    FILES "../mudlet.png"
    DESTINATION "share/icons/hicolor/256x256/apps" PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ
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
        WORLD_EXECUTE)
endif()
