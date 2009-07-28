project (mudlet)

CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake ${CMAKE_MODULE_PATH})
SET(QT_MIN_VERSION "4.4.0") # we need at least version 4.4 of Qt now
SET(QT_USE_QTNETWORK 1)
SET(QT_USE_QTPHONON 1 )
SET(QT_USE_QTWEBKIT 1)
SET(CMAKE_CONFIGURATION_TYPES "Debug;Release")

SET(mudlet_RCCS mudlet_alpha.qrc)

SET( mudlet_SRCS mudlet.cpp
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
TForkedProcess.cpp
dlgTriggersMainArea.cpp
dlgOptionsAreaTriggers.cpp
dlgOptionsAreaTimers.cpp
dlgOptionsAreaScripts.cpp
TCommandLine.cpp
TTreeWidget.cpp
TTreeWidgetItem.cpp
TTimer.cpp
TToolBar.cpp
TFlipButton.cpp
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
dlgHelpDialog.cpp
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
TToolBar.cpp
TFlipButton.cpp
TBuffer.cpp
TEasyButtonBar.cpp
dlgColorTrigger.cpp
TSplitter.cpp
TSplitterHandle.cpp
dlgNotepad.cpp
THighlighter.cpp
TLabel.cpp)

SET(mudlet_UIS ui/connection_profiles.ui
ui/console.ui
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
ui/help_dialog.ui
ui/timers_main_area.ui
ui/about_dialog.ui
ui/keybindings_main_area.ui
ui/color_trigger.ui
ui/notes_editor.ui
ui/profile_preferences.ui)

SET(mudlet_MOC_HDRS mudlet.h
EAction.h
TConsole.h
TToolBar.h
ctelnet.h
dlgConnectionProfiles.h
dlgTriggerEditor.h
TLuaInterpreter.h
TForkedProcess.h
dlgTriggers_main_area.h
dlgOptionsAreaTriggers.h
TCommandLine.h
TTreeWidget.h
dlgTimersMainArea.h
dlgSourceEditorArea.h
dlgSystemMessageArea.h
dlgScriptsMainArea.h
dlgAliasMainArea.h
dlgOptionsAreaAlias.h
dlgOptionsAreaScripts.h
dlgOptionsAreaTimers.h
dlgSearchArea.h
dlgActionMainArea.h
dlgOptionsAreaAction.h
dlgHelpDialog.h
dlgAboutDialog.h
dlgKeysMainArea.h
dlgProfilePreferences.h
TToolBar.h
TTextEdit.h
TEasyButtonBar.h
dlgColorTrigger.h
TSplitter.h
TSplitterHandle.h
dlgNotepad.h
THighlighter.h
TLabel.h)

FIND_PACKAGE(Qt4 REQUIRED)
FIND_PACKAGE(Lua51 REQUIRED)
FIND_PACKAGE(QScintilla REQUIRED)
FIND_PACKAGE(ZLIB REQUIRED)
FIND_PACKAGE(PCRE REQUIRED)

INCLUDE(../MudletCPack.cmake) # Use CPack
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${mudlet_BINARY_DIR} ${QSCINTILLA_INCLUDE_DIR} ${LUA_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${PCRE_INCLUDE_DIR})

QT4_AUTOMOC(${mudlet_SRCS})
QT4_WRAP_UI(mudlet_UIS_H ${mudlet_UIS})
QT4_WRAP_CPP(mudlet_MOC_SRCS ${mudlet_MOC_HDRS})
QT4_ADD_RESOURCES(mudlet_RCC_SRCS ${mudlet_RCCS})

add_executable(mudlet WIN32 ${mudlet_SRCS} ${mudlet_UIS_H} ${mudlet_MOC_SRCS} ${mudlet_RCC_SRCS})

TARGET_LINK_LIBRARIES(mudlet ${QT_LIBRARIES} ${QSCINTILLA_LIBRARIES} ${LUA_LIBRARIES} ${ZLIB_LIBRARIES} ${PCRE_LIBRARIES})

install( PROGRAMS mudlet DESTINATION "/usr/local/bin" )
install( FILES LuaGlobal.lua mudlet_documentation.html fonts/ttf-bitstream-vera-1.10/COPYRIGHT.TXT fonts/ttf-bitstream-vera-1.10/local.conf fonts/ttf-bitstream-vera-1.10/README.TXT fonts/ttf-bitstream-vera-1.10/RELEASENOTES.TXT fonts/ttf-bitstream-vera-1.10/VeraBd.ttf fonts/ttf-bitstream-vera-1.10/VeraBI.ttf fonts/ttf-bitstream-vera-1.10/VeraIt.ttf fonts/ttf-bitstream-vera-1.10/VeraMoBd.ttf fonts/ttf-bitstream-vera-1.10/VeraMoBI.ttf fonts/ttf-bitstream-vera-1.10/VeraMoIt.ttf fonts/ttf-bitstream-vera-1.10/VeraMono.ttf fonts/ttf-bitstream-vera-1.10/VeraSeBd.ttf fonts/ttf-bitstream-vera-1.10/VeraSe.ttf fonts/ttf-bitstream-vera-1.10/Vera.ttf DESTINATION "/usr/local/share/mudlet" )



