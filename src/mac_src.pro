CONFIG += release
CONFIG -= app_bundle
QMAKE_CXXFLAGS_RELEASE += -O3 -Wno-deprecated -Wno-unused-parameter -mmacosx-version-min=10.5
QMAKE_CXXFLAGS_DEBUG += -O0 -g -Wno-deprecated -Wno-unused-parameter
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5

QMAKE_CC = gcc
QMAKE_CXX = g++

MOC_DIR = ./tmp
OBJECTS_DIR = ./tmp
QT += network opengl uitools multimedia

cache()

# workaround https://bugreports.qt-project.org/browse/QTBUG-22829 when using Boost 1.48+
# QMAKE_MOC = $$QMAKE_MOC -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED

#macx:INCLUDEPATH += /Users/heikokoehn/Downloads/lua5_1_4_MacOS106_lib/include \
#/Users/heikokoehn/Downloads/boost_1_46_1 \
#/usr/local/include/yajl/include

# macx:INCLUDEPATH += /opt/local/include

# use pkg-config whenever possible
# I'm not certain whenever latest 1.54 boost will work, or we need 1.48
#  (or whenever we actually need to link to it here)
macx {
    # http://stackoverflow.com/a/16972067
    QT_CONFIG -= no-pkg-config
    CONFIG += link_pkgconfig
    PKGCONFIG += hunspell lua yajl libpcre libzip
}


macx:LIBS += \
#/Users/heikokoehn/lib/liblua.a \
# -L /opt/local/lib \ #don't link to macports libraries - should be self-contained
#-llua \ #statically include Lua, because we aren't shipping a dylib
#/opt/local/lib/liblua.a \
#/usr/local/lib/libluajit-5.1.a \
#/usr/local/lib/libzzip.a \
#-lzzip \ #statically include zzip, because we aren't shipping a dylib
#/opt/local/lib/libzzip.a \
#-F../Frameworks \
#-L../Frameworks \
# -lpcre \ # while we include a pcre dylib, it is version 1 while this Mudlet is built against 2 as reported by otool - static-linking for safety
#/opt/local/lib/libpcre.a \
#-lhunspell \ #macports handles this via pkg-config
    -lz \
    -lGLU
#/usr/local/lib/libyajl_s.a #-lyajl or even lyajl_s fail, because the linker tries to use the .dylib then - and fails

#unix:INCLUDEPATH += /usr/include/lua5.1

INCLUDEPATH += irc/include

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
    dlgAboutDialog.cpp \
    TDebug.cpp \
    dlgKeysMainArea.cpp \
    TKey.cpp \
    KeyUnit.cpp \
    dlgProfilePreferences.cpp \
    TTextEdit.cpp \
    XMLexport.cpp \
    XMLimport.cpp \
    FontManager.cpp \
    TFlipButton.cpp \
    TToolBar.cpp \
    TLabel.cpp \
    TEasyButtonBar.cpp \
    TForkedProcess.cpp \
    dlgColorTrigger.cpp \
    dlgTriggerPatternEdit.cpp \
    TSplitter.cpp \
    TSplitterHandle.cpp \
    mudlet.cpp \
    dlgNotepad.cpp \
    THighlighter.cpp \
    dlgComposer.cpp \
    TArea.cpp \
    glwidget.cpp \
    dlgMapper.cpp \
    TRoom.cpp \
    TMap.cpp \
    #lua_yajl.c \
    TBuffer.cpp \
    irc/src/ircbuffer.cpp \
    irc/src/irc.cpp \
    irc/src/ircsession.cpp \
    irc/src/ircutil.cpp \
    dlgIRC.cpp \
    T2DMap.cpp \
    dlgRoomExits.cpp \
    exitstreewidget.cpp \
    luazip.c \
    dlgPackageExporter.cpp


HEADERS += mudlet.h \
    TTimer.h \
    EAction.h \
    TConsole.h \
    ctelnet.h \
    Host.h \
    TMap.h \
    TAStar.h \
    HostManager.h \
    HostPool.h \
    dlgConnectionProfiles.h \
    dlgTriggerEditor.h \
    TTrigger.h \
    TLuaInterpreter.h \
    dlgTriggers_main_area.h \
    dlgOptionsAreaTriggers.h \
    dlgTriggerPatternEdit.h \
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
    TBuffer.h \
    TriggerUnit.h \
    TLabel.h \
    TEasyButtonBar.h \
    TForkedProcess.h \
    dlgColorTrigger.h \
    TSplitter.h \
    TSplitterHandle.h \
    dlgNotepad.h \
    THighlighter.h \
    dlgComposer.h \
    TRoom.h \
    TArea.h \
    TMap.h \
    glwidget.h \
    dlgMapper.h \
    Tree.h \
    irc/include/ircbuffer.h \
    irc/include/irc.h \
    irc/include/ircsession.h \
    irc/include/ircutil.h \
    dlgIRC.h \
    T2DMap.h \
    dlgRoomExits.h \
    exitstreewidget.h \
    dlgPackageExporter.h

FORMS += ui/connection_profiles.ui \
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
    ui/timers_main_area.ui \
    ui/about_dialog.ui \
    ui/keybindings_main_area.ui \
    ui/color_trigger.ui \
    ui/notes_editor.ui \
    ui/trigger_pattern_edit.ui \
    ui/composer.ui \
    ui/mapper.ui \
    ui/profile_preferences.ui \
    ui/irc.ui \
    ui/mapper_room_color.ui \
    ui/room_exits.ui \
    ui/lacking_mapper_script.ui \
    ui/package_manager.ui \
    ui/module_manager.ui \
    ui/package_manager_unpack.ui \
    ui/dlgPackageExporter.ui \
    ui/custom_lines.ui \
    ui/vars_main_area.ui

win32: {
    SOURCES += lua_yajl.c
}

unix: {
    SOURCES += lua_yajl1.c
}

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

OTHER_FILES += \
    mudlet_documentation.txt

