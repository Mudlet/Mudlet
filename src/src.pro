
QT += webkit \
 network 


LIBLUA = -llua5.1
!exists(/usr/lib/liblua5.1.a) {
	LIBLUA = -llua
}

unix : LIBS += -lqscintilla2 \
 -lpcre $$LIBLUA

win32 : LIBS += -Lc:\Qscintilla-gpl-2.3.2\qt4\release -lqscintilla2 \
 -Lc:\lua-5.1.4\src -llua51 

unix : INCLUDEPATH += /usr/include/Qsci \
 /usr/include/lua5.1

win32 : INCLUDEPATH += C:\Qscintilla-gpl-2.3.2\qt4 \
 c:\lua-5.1.4\src \
 c:\zlib-1.2.3

unix: isEmpty( INSTALL_PREFIX ) {
	INSTALL_PREFIX = /usr/local
}

unix: {
 SHARE_DIR = /usr/local/share/mudlet
 BIN_DIR = $$INSTALL_PREFIX/bin
}


SOURCES += TConsole.cpp \
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
XMLexport.cpp \
XMLimport.cpp \
TBuffer.cpp \
FontManager.cpp \
TFlipButton.cpp \
TToolBar.cpp \
mudlet.cpp


HEADERS += mudlet.h \
TTimer.h \
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
TFlipButton.h \
TToolBar.h \
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

RESOURCES = mudlet_alpha.qrc 

DISTFILES += paragraph.css

unix: {
 luaglobal.path = $$SHARE_DIR
 luaglobal.files = LuaGlobal.lua
 documentation.path = $$SHARE_DIR
 documentation.files = mudlet_documentation.html
 fonts.path = $$SHARE_DIR
 fonts.files = fonts/ttf-bitstream-vera-1.10/*
 target.path = $$BIN_DIR
}

INSTALLS += fonts \
luaglobal \
documentation \
target

