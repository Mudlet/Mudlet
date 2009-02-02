cmake_minimum_required (VERSION 2.6)
project (mudlet)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

FIND_PACKAGE(Qt4 REQUIRED)
FIND_PACKAGE(Lua51 REQUIRED)
FIND_PACKAGE(QScintilla REQUIRED)
INCLUDE(${QSCINTILLA_INCLUDE_DIR})
INCLUDE(${LUA_INCLUDE_DIR})

SET(QT_USE_QTNETWORK 1)
SET(QT_USE_QTWEBKIT 1)
INCLUDE(${QT_USE_FILE})

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
dlgHelpDialog.cpp
dlgAboutDialog.cpp
TDebug.cpp
dlgKeysMainArea.cpp
TKey.cpp
KeyUnit.cpp
dlgProfilePreferences.cpp
TTextEdit.cpp
TBuffer.cpp)

qt4_automoc(${mudlet_SRCS})

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
ui/profile_preferences.ui)

SET(mudlet_MOC_HDRS mudlet.h
EAction.h
TConsole.h
ctelnet.h
Host.h
HostManager.h
HostPool.h
dlgConnectionProfiles.h
dlgTriggerEditor.h
TTrigger.h
TLuaInterpreter.h
dlgTriggers_main_area.h
dlgOptionsAreaTriggers.h
TCommandLine.h
TTreeWidget.h
TTreeWidgetItem.h
TTimer.h
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
dlgHelpDialog.h
dlgAboutDialog.h
TMatchState.h
TEvent.h
TDebug.h
dlgKeysMainArea.h
TKey.h
KeyUnit.h
dlgProfilePreferences.h
TTextEdit.h
TBuffer.h)

QT4_WRAP_UI(mudlet_UIS_H ${mudlet_UIS})
QT4_WRAP_CPP(mudlet_MOC_SRCS ${mudlet_MOC_HDRS})
SET(mudlet_RCCS application.qrc)
QT4_ADD_RESOURCES(mudlet_RCC_SRCS ${mudlet_RCCS})

add_executable(mudlet ${mudlet_SRCS} ${mudlet_UIS_H} ${mudlet_MOC_SRCS} ${mudlet_RCC_SRCS})
TARGET_LINK_LIBRARIES(mudlet ${QT_LIBRARIES} ${QSCINTILLA_LIBRARIES} ${LUA_LIBRARIES})
