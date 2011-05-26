project (mudlet)

CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake ${CMAKE_MODULE_PATH})
SET(QT_MIN_VERSION "4.4.0") # we need at least version 4.4 of Qt now
SET(QT_USE_QTNETWORK 1)
SET(QT_USE_QOPENGL 1)
SET(QT_USE_PHONON 1)
SET(QT_USE_OPENGL 1)
SET(CMAKE_CONFIGURATION_TYPES "Release" "Debug")
SET(mudlet_RCCS mudlet_alpha.qrc)
SET( mudlet_SRCS 
TConsole.cpp
    ctelnet.cpp
    main.cpp
    Host.cpp
    HostManager.cpp
    HostPool.cpp
    dlgConnectionProfiles.cpp
    dlgTriggerEditor.cpp
    TTrigger.cpp
    TriggerUnit.cpp
    TLuaInterpreter.cpp
    dlgTriggersMainArea.cpp
    dlgOptionsAreaTriggers.cpp
    dlgOptionsAreaTimers.cpp
    dlgOptionsAreaScripts.cpp
    TCommandLine.cpp
    TTreeWidget.cpp
    TTreeWidgetItem.cpp
    TTimer.cpp
    TScript.cpp
    TAlias.cpp
    dlgTimersMainArea.cpp
    dlgSystemMessageArea.cpp
    dlgSourceEditorArea.cpp
    TimerUnit.cpp
    ScriptUnit.cpp
    AliasUnit.cpp
    dlgScriptsMainArea.cpp
    dlgAliasMainArea.cpp
    dlgOptionsAreaAlias.cpp
    dlgSearchArea.cpp
    TAction.cpp
    ActionUnit.cpp
    dlgActionMainArea.cpp
    dlgOptionsAreaAction.cpp
    EAction.cpp
    dlgAboutDialog.cpp
    TDebug.cpp
    dlgKeysMainArea.cpp
    TKey.cpp
    KeyUnit.cpp
    dlgProfilePreferences.cpp
    TTextEdit.cpp
    XMLexport.cpp
    XMLimport.cpp
    FontManager.cpp
    TFlipButton.cpp
    TToolBar.cpp
    TLabel.cpp
    TEasyButtonBar.cpp
    TForkedProcess.cpp
    dlgColorTrigger.cpp
    dlgTriggerPatternEdit.cpp
    TSplitter.cpp
    TSplitterHandle.cpp
    mudlet.cpp
    dlgNotepad.cpp
    THighlighter.cpp
    dlgComposer.cpp
    TArea.cpp
    glwidget.cpp
    dlgMapper.cpp
    TRoom.cpp
    TMap.cpp
    TBuffer.cpp
    T2DMap.cpp
    dlgRoomExits.cpp
	lua_yajl.c
	dlgIrc.cpp
)

SET(mudlet_UIS 
ui/connection_profiles.ui
    ui/main_window.ui
    ui/trigger_editor.ui
    ui/options_area_triggers.ui
    ui/options_area_timers.ui
    ui/options_area_aliases.ui
    ui/options_area_scripts.ui
    ui/triggers_main_area.ui
    ui/scripts_main_area.ui
    ui/aliases_main_area.ui
    ui/system_message_area.ui
    ui/source_editor_area.ui
    ui/extended_search_area.ui
    ui/actions_main_area.ui
    ui/options_area_actions.ui
    ui/timers_main_area.ui
    ui/about_dialog.ui
    ui/keybindings_main_area.ui
    ui/color_trigger.ui
    ui/notes_editor.ui
    ui/trigger_pattern_edit.ui
    ui/composer.ui
    ui/mapper.ui
    ui/profile_preferences.ui
    ui/mapper_room_color.ui
    ui/room_exits.ui
    ui/lacking_mapper_script.ui
)

SET(mudlet_MOC_HDRS 
mudlet.h
    TTimer.h
    EAction.h
    TConsole.h
    ctelnet.h
    Host.h
    TMap.h
    TAStar.h
    HostManager.h
    HostPool.h
    dlgConnectionProfiles.h
    dlgTriggerEditor.h
    TTrigger.h
    TLuaInterpreter.h
    dlgTriggers_main_area.h
    dlgOptionsAreaTriggers.h
    dlgTriggerPatternEdit.h
    TCommandLine.h
    TTreeWidget.h
    TTreeWidgetItem.h
    TScript.h
    TAlias.h
    dlgTimersMainArea.h
    dlgSourceEditorArea.h
    dlgSystemMessageArea.h
    TimerUnit.h
    ScriptUnit.h
    AliasUnit.h
    dlgScriptsMainArea.h
    dlgAliasMainArea.h
    dlgOptionsAreaAlias.h
    dlgOptionsAreaScripts.h
    dlgOptionsAreaTimers.h
    dlgSearchArea.h
    TAction.h
    ActionUnit.h
    dlgActionMainArea.h
    dlgOptionsAreaAction.h
    dlgAboutDialog.h
    TMatchState.h
    TEvent.h
    TDebug.h
    dlgKeysMainArea.h
    TKey.h
    KeyUnit.h
    dlgProfilePreferences.h
    TTextEdit.h
    TFlipButton.h
    TToolBar.h
    TBuffer.h
    TriggerUnit.h
    TLabel.h
    TEasyButtonBar.h
    TForkedProcess.h
    dlgColorTrigger.h
    TSplitter.h
    TSplitterHandle.h
    dlgNotepad.h
    THighlighter.h
    dlgComposer.h
    TRoom.h
    TArea.h
    TMap.h
    glwidget.h
    dlgMapper.h
    Tree.h
    T2DMap.h
    dlgRoomExits.h
	dlgIrc.h
)

FIND_PACKAGE(Qt4 REQUIRED)
#FIND_PACKAGE(Lua51 REQUIRED)
#FIND_PACKAGE(ZLIB REQUIRED)
#FIND_PACKAGE(PCRE REQUIRED)


INCLUDE(../MudletCPack.cmake) # Use CPack
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${mudlet_BINARY_DIR} 
"C:\\mudlet_package_MSVC\\pcre-8.0-lib\\include" 
"C:\\mudlet_package_MSVC\\Lua_src\\include" 
"C:\\mudlet_package_MSVC\\zlib-1.2.5"
"c:\\mudlet_package_MSVC\\boost_1_45_0"
"c:\\mudlet_package_MSVC\\pcre-8.0-lib\\include"
"C:\\mudlet_package_MSVC\\hunspell-1.3.1\\src"
"C:\\mudlet_package_MSVC\\lloyd-yajl-f4b2b1a\\yajl-2.0.1\\include"
"C:\\mudlet_package_MSVC\\libircclient-qt-0.5.0\\include"
)

IF(CMAKE_COMPILER_IS_GNUCXX)
  ADD_DEFINITIONS(${CMAKE_CXX_FLAGS} "-Wno-deprecated  -DUNICODE  -D_UNICODE")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

QT4_AUTOMOC(${mudlet_SRCS})
QT4_WRAP_UI(mudlet_UIS_H ${mudlet_UIS})
QT4_WRAP_CPP(mudlet_MOC_SRCS ${mudlet_MOC_HDRS})
QT4_ADD_RESOURCES(mudlet_RCC_SRCS ${mudlet_RCCS})

add_executable(mudlet WIN32 ${mudlet_SRCS} ${mudlet_UIS_H} ${mudlet_MOC_SRCS} ${mudlet_RCC_SRCS})

TARGET_LINK_LIBRARIES( 
mudlet 
c:\\Qt_VC\\4.7.3\\lib\\qtmain.lib
C:\\Qt_VC\\4.7.3\\lib\\QtOpenGL4.lib
C:\\Qt_VC\\4.7.3\\lib\\QtCore4.lib
C:\\Qt_VC\\4.7.3\\lib\\QtGui4.lib
C:\\Qt_VC\\4.7.3\\lib\\QtNetwork4.lib
C:\\Qt_VC\\4.7.3\\lib\\QtUiTools.lib
C:\\Qt_VC\\4.7.3\\lib\\phonon4.lib
c:\\mudlet_package_MSVC\\zlib_125\\zlibwapi.lib
C:\\mudlet_package_MSVC\\Lua_src\\lib\\lua51.lib
C:\\mudlet_package_MSVC\\pcre-8.12\\Release\\pcre.lib
C:\\mudlet_package_MSVC\\hunspell-1.3.1\\src\\win_api\\Release_dll\\libhunspell\\libhunspell.lib
"C:\\Program Files (x86)\\Microsoft SDKs\\Windows\\v7.0A\\Lib\\OpenGL32.Lib"
"C:\\Program Files (x86)\\Microsoft SDKs\\Windows\\v7.0A\\Lib\\GlU32.Lib"
"C:\\Users\\heiko\\Desktop\\lloyd-yajl-f4b2b1a\\yajl-2.0.1\\lib\\Release\\yajl.lib"
"C:\\mudlet_package_MSVC\\libircclient-qt-0.5.0\\lib\\ircclient-qt.lib"
)

install( PROGRAMS mudlet DESTINATION "/usr/local/bin" )
install( FILES LuaGlobal.lua mudlet_documentation.html fonts/ttf-bitstream-vera-1.10/COPYRIGHT.TXT fonts/ttf-bitstream-vera-1.10/local.conf fonts/ttf-bitstream-vera-1.10/README.TXT fonts/ttf-bitstream-vera-1.10/RELEASENOTES.TXT fonts/ttf-bitstream-vera-1.10/VeraBd.ttf fonts/ttf-bitstream-vera-1.10/VeraBI.ttf fonts/ttf-bitstream-vera-1.10/VeraIt.ttf fonts/ttf-bitstream-vera-1.10/VeraMoBd.ttf fonts/ttf-bitstream-vera-1.10/VeraMoBI.ttf fonts/ttf-bitstream-vera-1.10/VeraMoIt.ttf fonts/ttf-bitstream-vera-1.10/VeraMono.ttf fonts/ttf-bitstream-vera-1.10/VeraSeBd.ttf fonts/ttf-bitstream-vera-1.10/VeraSe.ttf fonts/ttf-bitstream-vera-1.10/Vera.ttf DESTINATION "/usr/local/share/mudlet" )



