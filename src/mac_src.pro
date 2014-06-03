# disable Qt adding -Wall for us, insert it ourselves so we can add -Wno-* after
CONFIG += warn_off
# ignore unused parameters, because boost has a ton of them and that's not something we need to know
QMAKE_CXXFLAGS_RELEASE += -O3 -Wall -Wno-deprecated -Wno-unused-parameter -mmacosx-version-min=10.5
QMAKE_CXXFLAGS_DEBUG += -O0 -g -Wall -Wno-deprecated -Wno-unused-parameter -mmacosx-version-min=10.5
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5

# We should use the default compiler preferred by Qt on the system (clang vs gcc)
!macx {
  QMAKE_CC = gcc
  QMAKE_CXX = g++
}

MOC_DIR = ./tmp
OBJECTS_DIR = ./tmp
QT += network opengl uitools multimedia

cache()

# Set the current Mudlet Version, unfortunately the Qt documentation suggests
# that only a #.#.# form without any other alphanumberic suffixes is required:
VERSION = 3.0.1

# Leave the value of the following empty, line should be "BUILD =" without quotes
# (it is NOT a Qt built-in variable) for a release build or, if you are
# distributing modified code, it would be useful if you could put something to
# distinguish the version:
## I'll tag MY versions with something containing "slysven" but please edit it
## to something else and take this 2 line extra comment out! 8-) - Slysven:
BUILD = -rc2-slysven

# Changing the above pair of values affects: ctelnet.cpp, main.cpp, mudlet.cpp
# dlgAboutDialog.cpp and TLuaInterpreter.cpp.  It does NOT cause those files to
# be automatically rebuilt so you will need to 'touch' them...!
# Use APP_VERSION, APP_BUILD and APP_TARGET defines in the source code if needed.
DEFINES += APP_VERSION=\\\"$${VERSION}\\\"
DEFINES += APP_BUILD=\\\"$${BUILD}\\\"
win32{
    TARGET = mudlet
} else:macx{
# Capitalize the name for Mudlet, so it appears as 'Mudlet'
# and not 'mudlet' in the .dmg installer
    TARGET = Mudlet
} else{
    TARGET = mudlet
}

# Create a record of what the executable will be called by hand
# NB. "cygwin-g++" although a subset of "unix" NOT "win32" DOES create
# executables with an ".exe" extension!
DEFINES += APP_TARGET=\\\"$${TARGET}$${TARGET_EXT}\\\"

# use pkg-config whenever possible for linking on a mac
# the same should be done on the Linux platform as well
macx {
    # http://stackoverflow.com/a/16972067
    QT_CONFIG -= no-pkg-config
    CONFIG += link_pkgconfig
    PKGCONFIG += hunspell lua yajl libpcre libzip
}

macx:LIBS += \
    # I didn't see an obvious pkg-config option for these two
    -lz \
    -lzzip

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
    dlgPackageExporter.cpp \
    TRoomDB.cpp \
    TVar.cpp \
    LuaInterface.cpp \
    VarUnit.cpp \
    dlgVarsMainArea.cpp


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
    dlgPackageExporter.h \
    TRoomDB.h \
    TVar.h \
    LuaInterface.h \
    VarUnit.h \
    dlgVarsMainArea.h

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

unix!macx: {
    SOURCES += lua_yajl1.c
}

macx: {
    SOURCES += lua_yajl.c
}

TEMPLATE = app

RESOURCES = mudlet_alpha.qrc
DISTFILES += paragraph.css
unix!macx: {
    luaglobal.path = $$SHARE_DIR
    luaglobal.files = LuaGlobal.lua
    documentation.path = $$SHARE_DIR
    documentation.files = mudlet_documentation.html
    fonts.path = $$SHARE_DIR
    fonts.files = fonts/ttf-bitstream-vera-1.10/*
    target.path = $$BIN_DIR
}

macx: {
    # Copy mudlet-lua into the .app bundle
    # the location is relative to mac_src.pro, so just use mudlet-lua
    APP_MUDLET_LUA_FILES.files = mudlet-lua
    APP_MUDLET_LUA_FILES.path  = Contents/Resources
    QMAKE_BUNDLE_DATA += APP_MUDLET_LUA_FILES

    # Set the .app's icns file
    ICON = osx-installer/osx.icns
}

# Copy stuff into install folder on all platforms but OSX, where it never worked
# and just gives errors
!macx: {
    INSTALLS += fonts \
        luaglobal \
        documentation \
        target
}

OTHER_FILES += \
    mudlet_documentation.txt \
    mac-deploy.sh

