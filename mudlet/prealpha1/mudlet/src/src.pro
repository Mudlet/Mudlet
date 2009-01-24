
QT += webkit \
 network 


unix : LIBS += -lqscintilla2 \
	-llua5.1

win32 : LIBS += -Lc:\Qscintilla-gpl-2.3.2\qt4\release -lqscintilla2 \
 -Lc:\lua-5.1.4\src -llua51 

unix : INCLUDEPATH += /usr/include/Qsci \
 /usr/include/lua5.1

win32 : INCLUDEPATH += C:\Qscintilla-gpl-2.3.2\qt4 \
 c:\lua-5.1.4\src \
 c:\zlib-1.2.3

SOURCES += mudlet.cpp \
 TConsole.cpp \
	ctelnet.cpp \
 main.cpp \
	Host.cpp \
	HostManager.cpp \
	HostPool.cpp \
	dlgConnectionProfiles.cpp \
	dlgTriggerEditor.cpp \
	TTrigger.cpp \
 TriggerUnit.cpp \
	TLuaInterpreter.cpp \
 dlgTriggersMainArea.cpp \
 dlgOptionsAreaTriggers.cpp \
 dlgOptionsAreaTimers.cpp \
 dlgOptionsAreaScripts.cpp \
 TCommandLine.cpp \
 TTreeWidget.cpp \
 TTreeWidgetItem.cpp \
 TTimer.cpp \
 TScript.cpp \
 TAlias.cpp \
 dlgTimersMainArea.cpp \
 dlgSystemMessageArea.cpp \
 dlgSourceEditorArea.cpp \
 TimerUnit.cpp \
 ScriptUnit.cpp \
 AliasUnit.cpp \
 dlgScriptsMainArea.cpp \
 dlgAliasMainArea.cpp \
 dlgOptionsAreaAlias.cpp \
 dlgSearchArea.cpp \
 TAction.cpp \
 ActionUnit.cpp \
 dlgActionMainArea.cpp \
 dlgOptionsAreaAction.cpp \
 EAction.cpp \
 dlgHelpDialog.cpp \
 dlgAboutDialog.cpp \
 TDebug.cpp \
 dlgKeysMainArea.cpp \
 TKey.cpp \
 KeyUnit.cpp \
 dlgProfilePreferences.cpp \
 TTextEdit.cpp \
 TBuffer.cpp


HEADERS += mudlet.h \
 EAction.h \
 TConsole.h \
 ctelnet.h \
 Host.h \
 HostManager.h \
 HostPool.h \
 dlgConnectionProfiles.h \
 dlgTriggerEditor.h \
 TTrigger.h \
 TLuaInterpreter.h \
 dlgTriggers_main_area.h \
 dlgOptionsAreaTriggers.h \
 TCommandLine.h \
 TTreeWidget.h \
 TTreeWidgetItem.h \
 TTimer.h \
 TScript.h \
 TAlias.h \
 dlgTimersMainArea.h \
 dlgSourceEditorArea.h \
 dlgSystemMessageArea.h \
 TimerUnit.h \
 ScriptUnit.h \
 AliasUnit.h \
 dlgScriptsMainArea.h \
 dlgAliasMainArea.h \
 dlgOptionsAreaAlias.h \
 dlgOptionsAreaScripts.h \
 dlgOptionsAreaTimers.h \
 dlgSearchArea.h \
 TAction.h \
 ActionUnit.h \
 dlgActionMainArea.h \
 dlgOptionsAreaAction.h \
 dlgHelpDialog.h \
 dlgAboutDialog.h \
 TMatchState.h \
 TEvent.h \
 TDebug.h \
 dlgKeysMainArea.h \
 TKey.h \
 KeyUnit.h \
 dlgProfilePreferences.h \
 TTextEdit.h \
 TBuffer.h



FORMS += ui/connection_profiles.ui \
 ui/console.ui \
	ui/main_window.ui \
	ui/trigger_editor.ui \
	ui/options_area_triggers.ui \
 ui/options_area_timers.ui \
 ui/options_area_aliases.ui \
 ui/options_area_scripts.ui \
	ui/triggers_main_area.ui \
 ui/scripts_main_area.ui \
 ui/aliases_main_area.ui \
	ui/system_message_area.ui \
	ui/source_editor_area.ui \
 ui/extended_search_area.ui \
 ui/actions_main_area.ui \
 ui/options_area_actions.ui \
 ui/help_dialog.ui \
	ui/timers_main_area.ui \
 ui/about_dialog.ui \
 ui/keybindings_main_area.ui \
 ui/profile_preferences.ui


TEMPLATE = app

TARGET = mudlet

RESOURCES = application.qrc 

DISTFILES += paragraph.css

