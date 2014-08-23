############################################################################
#    Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            #
#    Copyright (C) 2013-2014 by Stephen Lyons - slysven@virginmedia.com    #
#                                                                          #
#    This program is free software; you can redistribute it and/or modify  #
#    it under the terms of the GNU General Public License version 2 as     #
#    published by the Free Software Foundation.                            #
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

# Be a bit helpful - some people currently may have default system Qt4
# libraries even if they have added in a local copy of a Qt5 installation!
lessThan(QT_MAJOR_VERSION, 5): error("Mudlet now requires Qt 5 libraries, if your system has more than one Qt version please check you are using the right qmake!")


# disable Qt adding -Wall for us, insert it ourselves so we can add -Wno-*
# after, unfortunately the current state of affairs suggests that qmake does not
# do this correctly for Microsoft Visual (Studios) C.
!msvc:CONFIG += warn_off


## Uncomment these lines to remove any existing optimisation flags - a bit
## naughty, as it overrides the supplied defaults from the mkspecs files and is
## probably a gcc-specfic option anyway...
# !msvc:QMAKE_CXXFLAGS_RELEASE ~= s/-O[s0123]/
## ... so that we can force -O3, though one could also use any of the others (-Os
## optimises GCC for size not speed)
# !msvc:QMAKE_CXXFLAGS_RELEASE += -O3

# There should not be a need to specify a "-g" for the debug build - it should
# be added in by the relevent <QtBase>/gcc/mkspecs bit.
## For some debugging tasks (with valgrid) it is recommended to use -O2, under
## other cases (stepping through code execution with gdb) it may be more
## straightforward without any optimisation (-O0)
## !msvc:QMAKE_CXXFLAGS_DEBUG += -O2

# All the other things:
# ignore unused parameters, because boost has a ton of them and that is not
# something we need to know. (deprecated-declarations are NOT turned off by
# -Wno-deprecated and the ircqt library throws up quite a few of them.)
!msvc:QMAKE_CXXFLAGS += -Wall -Wno-deprecated -Wno-deprecated-declarations -Wno-unused-local-typedefs -Wno-unused-parameter


# We need these extra Qt modules at least:
QT += network opengl uitools multimedia

# Build type, one of "debug", "release" or "debug_and_release" (both, except NOT
# possible on macx, ???DEFAULT??? on Windows) - the last of these choices causes
# THREE passes through the project file when qmake is "run":
CONFIG += debug


# Mac specific:
macx:QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5


# Windows (MSVC) specific:

## Do we need this to correctly handle a compiled Mudlet executable to be used
## on other machines (from QMake manual | Platform Notes) - SlySven:
## CONFIG -= embed_manifest_exe

## Uncomment to enable multiprocessor builds in nmake.exe part of MSVC:
#msvc:QMAKE_CXXFLAGS += -MP


# Set the current Mudlet Version, unfortunately the Qt documentation suggests
# that only a #.#.# form without any other alphanumberic suffixes is required:
VERSION = 3.0.1


# For a release build: leave the value of the following empty, (line should be
# "BUILD =" without quotes). BUILD is NOT a Qt built-in variable;
# For a develop-er/-ment build or, if you are distributing modified code, it
# would be useful if you could put something to distinguish the version - and
# for developers working on more than one sub-version it'll help you tell them
# apart at run time - though github users WILL need to make sure it gets changed
# to the "right" value when pulled into the main stream codebase :)
BUILD = -dev

# Changing the above pair of values affects: ctelnet.cpp, main.cpp, mudlet.cpp
# dlgAboutDialog.cpp and TLuaInterpreter.cpp.  It does NOT cause those files to
# be automatically rebuilt so you will need to 'touch' them...!
# Use APP_VERSION, APP_BUILD and APP_TARGET defines in the source code if needed.
#
# IMPORTANT: Ensure the values for VERSION and BUILD are the same as APP_VERSION
# and APP_BUILD in the TOP-level alternative cmake project file,
# i.e. in the ./../CMakeLists.txt file !
DEFINES += APP_VERSION=\\\"$${VERSION}\\\"
DEFINES += APP_BUILD=\\\"$${BUILD}\\\"


# Capitalize the name for Mudlet, so it appears as 'Mudlet' and not 'mudlet' in
# the (Mac) .dmg installer
macx {
    TARGET = Mudlet
} else {
    TARGET = mudlet
}

## if building BOTH version (especially with CONFIG = debug_and_release)
## uncomment this so it can rename the debug version so if installed to the same
## location they do not clash, not for Macx in case it breaks bundling, though
## it might be doable by someone more skilled in that area
##unix:!macx:CONFIG(debug, debug|release): TARGET = $$join(TARGET,,,_debug)
##win32:CONFIG(debug, debug|release): TARGET = $$join(TARGET,,,d)


# Create a record of what the executable will be called by hand
# NB. "cygwin-g++" although a subset of "unix" NOT "win32" DOES create
# executables with an ".exe" extension!
DEFINES += APP_TARGET=\\\"$${TARGET}$${TARGET_EXT}\\\"

# Main system related stuff:
unix {
    ! macx {
########################## Unixes but NOT Macs #################################

# Distribution packagers would be using PREFIX = /usr but /usr/local is accepted
# destination place for local builds for software for ALL users of a system:
        isEmpty( PREFIX ) PREFIX = /usr/local
        isEmpty( DATAROOTDIR ) DATAROOTDIR = $${PREFIX}/share
        isEmpty( DATADIR ) DATADIR = $${DATAROOTDIR}/mudlet
# According to Linux FHS /usr/local/games is an alternative location for leasure
# time BINARIES 8-):
        isEmpty( BINDIR ) BINDIR = $${PREFIX}/bin
# Again according to FHS /usr/local/share/games is the corresponding place for
# locally built games documentation:
        isEmpty( DOCDIR ) DOCDIR = $${DATAROOTDIR}/doc/mudlet

# Where to find other librarys needed (-L), and what those libraries are (-l):
        LIBS += \
            -L/usr/local/lib/ \
            -lpcre \
            -llua5.1 \
            -lhunspell \
            -lyajl \
            -lGLU \
            -lzip \
            -lz

# Where to find headers for the libraries, besides "default" ones:
        INCLUDEPATH += /usr/include/lua5.1

# If not otherwse specified, where to put the common "sharable" lua files used
# by Mudlet:
        isEmpty( LUA_DEFAULT_DIR ) LUA_DEFAULT_DIR = $${DATADIR}/lua

# Where to put the Mudlet executable:
#   the "target" install set is handled automagically, just not very well...
        target.path = $${BINDIR}
        message("$${TARGET} will be installed to \"$${target.path}\"...")

## TODO!
# Where to put the Documents, e.g. README, CHANGELOG (if we had one!) and so on
#     DOCS.path = $${DOCS_DIR}
#     message("Documentation will be installed to \"$${DOCS.path}\"...")

        !isEmpty( LUA_DEFAULT_DIR ) {
# if a directory has been set for the lua files move the detail into the
# installation details for the unix case:
            LUA.path = $${LUA_DEFAULT_DIR}
            LUA_GEYSER.path = $${LUA.path}/geyser
# and define a preprocessor symbol LUA_DEFAULT_PATH with the value:
            DEFINES += LUA_DEFAULT_PATH=\\\"$${LUA_DEFAULT_DIR}\\\"
# and say what will happen:
            message("Lua files will be installed to \"$${LUA.path}\"...")
            message("Geyser lua files will be installed to \"$${LUA_GEYSER.path}\"...")
        }

    } else {
################################# Macs #########################################

# use pkg-config whenever possible for linking on a mac
# the same should be done on the Linux platform as well
# http://stackoverflow.com/a/16972067
        QT_CONFIG -= no-pkg-config
        CONFIG += link_pkgconfig
        PKGCONFIG += \
            hunspell \
            lua \
            yajl \
            libpcre \
            libzip

# There does not seem to be an obvious pkg-config option for these two
        LIBS += \
            -lz \
            -lzzip

# Install locations are relative to the installed Application bundle:
## CHECK:
## This only puts in the actual lua files needed to run Mudlet, also note that
## we are using the "lua" subdirectory and not mudlet-lua/lua - the code is
## implicitly FOR Mudlet and the extra intermediate directory seems pointless
## Please delete this ## prefix comment when/if committed to development branch
## and it has proven "to work" SlySven!
        LUA.path = Contents/Resources/lua
        LUA_GEYSER.path = $${LUA.path}/geyser
        message("Lua files will be installed to \"<bundle>$${LUA.path}\"...")
        message("Geyser lua files will be installed to \"<bundle>$${LUA_GEYSER.path}\"...")
    }
} else:win32: {
################################ Windows #######################################

    LIBS += -L"C:\\mudlet5_package" \
        -L"C:\\mingw32\\lib" \
        -llua51 \
        -lpcre \
        -lhunspell \
        -llibzip \
        -lzlib \
        -llibzip \
        -L"C:\\mudlet5_package\\yajl-master\\yajl-2.0.5\\lib" \
        -lyajl

    INCLUDEPATH += "c:\\mudlet_package_MINGW\\Lua_src\\include" \
        "C:\\mingw32\\include" \
        "c:\\mudlet_package_MINGW\\zlib-1.2.5" \
        "C:\\mudlet5_package\\boost_1_54_0" \
        "c:\\mudlet_package_MINGW\\pcre-8.0-lib\\include" \
        "C:\\mudlet5_package\\yajl-master\\yajl-2.0.5\\include" \
        "C:\\mudlet5_package\\libzip-0.11.1\\lib" \
        "C:\\mudlet_package_MINGW\\hunspell-1.3.1\\src"
# Leave this undefined so mudlet::readSettings() preprocessing will fall back to
# hard-coded executable's /mudlet-lua/lua/ subdirectory
#    LUA_DEFAULT_DIR = $$clean_path($$system(echo %ProgramFiles%)/lua)
}

INCLUDEPATH += irc/include

SOURCES += \
    ActionUnit.cpp \
    AliasUnit.cpp \
    ctelnet.cpp \
    dlgAboutDialog.cpp \
    dlgActionMainArea.cpp \
    dlgAliasMainArea.cpp \
    dlgColorTrigger.cpp \
    dlgComposer.cpp \
    dlgConnectionProfiles.cpp \
    dlgIRC.cpp \
    dlgKeysMainArea.cpp \
    dlgMapper.cpp \
    dlgNotepad.cpp \
    dlgOptionsAreaAction.cpp \
    dlgOptionsAreaAlias.cpp \
    dlgOptionsAreaScripts.cpp \
    dlgOptionsAreaTimers.cpp \
    dlgOptionsAreaTriggers.cpp \
    dlgPackageExporter.cpp \
    dlgProfilePreferences.cpp \
    dlgRoomExits.cpp \
    dlgScriptsMainArea.cpp \
    dlgSearchArea.cpp \
    dlgSourceEditorArea.cpp \
    dlgSystemMessageArea.cpp \
    dlgTimersMainArea.cpp \
    dlgTriggerEditor.cpp \
    dlgTriggerPatternEdit.cpp \
    dlgTriggersMainArea.cpp \
    dlgVarsMainArea.cpp \
    EAction.cpp \
    exitstreewidget.cpp \
    FontManager.cpp \
    glwidget.cpp \
    Host.cpp \
    HostManager.cpp \
    HostPool.cpp \
    irc/src/irc.cpp \
    irc/src/ircbuffer.cpp \
    irc/src/ircsession.cpp \
    irc/src/ircutil.cpp \
    KeyUnit.cpp \
    LuaInterface.cpp \
    lua_yajl.c \
    main.cpp \
    mudlet.cpp \
    ScriptUnit.cpp \
    T2DMap.cpp \
    TAction.cpp \
    TAlias.cpp \
    TArea.cpp \
    TBuffer.cpp \
    TCommandLine.cpp \
    TConsole.cpp \
    TDebug.cpp \
    TEasyButtonBar.cpp \
    TFlipButton.cpp \
    TForkedProcess.cpp \
    THighlighter.cpp \
    TimerUnit.cpp \
    TKey.cpp \
    TLabel.cpp \
    TLuaInterpreter.cpp \
    TMap.cpp \
    TriggerUnit.cpp \
    TRoom.cpp \
    TRoomDB.cpp \
    TScript.cpp \
    TSplitter.cpp \
    TSplitterHandle.cpp \
    TTextEdit.cpp \
    TTimer.cpp \
    TToolBar.cpp \
    TTreeWidget.cpp \
    TTreeWidgetItem.cpp \
    TTrigger.cpp \
    TVar.cpp \
    VarUnit.cpp \
    XMLexport.cpp \
    XMLimport.cpp


HEADERS += \
    ActionUnit.h \
    AliasUnit.h \
    ctelnet.h \
    dlgAboutDialog.h \
    dlgActionMainArea.h \
    dlgAliasMainArea.h \
    dlgColorTrigger.h \
    dlgComposer.h \
    dlgConnectionProfiles.h \
    dlgIRC.h \
    dlgKeysMainArea.h \
    dlgMapper.h \
    dlgNotepad.h \
    dlgOptionsAreaAction.h \
    dlgOptionsAreaAlias.h \
    dlgOptionsAreaScripts.h \
    dlgOptionsAreaTimers.h \
    dlgOptionsAreaTriggers.h \
    dlgPackageExporter.h \
    dlgProfilePreferences.h \
    dlgRoomExits.h \
    dlgScriptsMainArea.h \
    dlgSearchArea.h \
    dlgSourceEditorArea.h \
    dlgSystemMessageArea.h \
    dlgTimersMainArea.h \
    dlgTriggerEditor.h \
    dlgTriggerPatternEdit.h \
    dlgTriggersMainArea.h \
    dlgVarsMainArea.h \
    EAction.h \
    exitstreewidget.h \
    glwidget.h \
    Host.h \
    HostManager.h \
    HostPool.h \
    irc/include/irc.h \
    irc/include/ircbuffer.h \
    irc/include/ircsession.h \
    irc/include/ircutil.h \
    KeyUnit.h \
    LuaInterface.h \
    mudlet.h \
    mudlet-lua/lua/luaLocation.h \
    ScriptUnit.h \
    T2DMap.h \
    TAction.h \
    TAlias.h \
    TArea.h \
    TAStar.h \
    TBuffer.h \
    TCommandLine.h \
    TConsole.h \
    TDebug.h \
    TEasyButtonBar.h \
    TEvent.h \
    TFlipButton.h \
    TForkedProcess.h \
    THighlighter.h \
    TimerUnit.h \
    TKey.h \
    TLabel.h \
    TLuaInterpreter.h \
    TMap.h \
    TMatchState.h \
    Tree.h \
    TriggerUnit.h \
    TRoom.h \
    TRoomDB.h \
    TScript.h \
    TSplitter.h \
    TSplitterHandle.h \
    TTextEdit.h \
    TTimer.h \
    TToolBar.h \
    TTreeWidget.h \
    TTreeWidgetItem.h \
    TTrigger.h \
    TVar.h \
    VarUnit.h


FORMS += \
    ui/about_dialog.ui \
    ui/actions_main_area.ui \
    ui/aliases_main_area.ui \
    ui/color_trigger.ui \
    ui/composer.ui \
    ui/connection_profiles.ui \
    ui/dlgPackageExporter.ui \
    ui/extended_search_area.ui \
    ui/irc.ui \
    ui/keybindings_main_area.ui \
    ui/main_window.ui \
    ui/mapper.ui \
    ui/notes_editor.ui \
    ui/options_area_actions.ui \
    ui/options_area_aliases.ui \
    ui/options_area_scripts.ui \
    ui/options_area_timers.ui \
    ui/options_area_triggers.ui \
    ui/profile_preferences.ui \
    ui/room_exits.ui \
    ui/scripts_main_area.ui \
    ui/source_editor_area.ui \
    ui/system_message_area.ui \
    ui/timers_main_area.ui \
    ui/triggers_main_area.ui \
    ui/trigger_editor.ui \
    ui/trigger_pattern_edit.ui \
    ui/vars_main_area.ui

# These are not required when compiling the application but are loaded in by
# QiuLoader during run-time, they are listed here so they still show in the
# Qt IDE and don't get forgotton
RUNTIME_FORMS = \
    ui/custom_lines.ui \
    ui/custom_lines_properties.ui \
    ui/delete_profile_confirmation.ui \
    ui/lacking_mapper_script.ui \
    ui/module_manager.ui \
    ui/package_manager.ui \
    ui/package_manager_unpack.ui \
    ui/set_room_area.ui

RESOURCES = mudlet_alpha.qrc

TEMPLATE = app

# To use QtCreator as a Unix installer the generated Makefile must have the
# following lists of files EXPLICITLY stated - IT IS NOT WORKABLE IF ONLY
# A PATH IS GIVEN AS AN ENTRY TO THE .files LIST - as was the case for a
# previous incarnation for macs.
#
# Select Qt Creator's "Project" Side tab and under the "Build and Run" top tab
# choose your Build Kit's "Run"->"Run Settings" ensure you have a "Make" step
# that - if you are NOT runnning QT Creator as root, which is the safest way
# (i.e safe = NOT root) - against:
# "Override <path to?>/make" has the entry: "/usr/bin/sudo"
# without the quotes, assuming /usr/bin is the location of "sudo"
# and against:
# "Make arguments" has the entry: "-A sh -c '/usr/bin/make install'"
# without the DOUBLE quotes but with the SINGLE quotes, assuming /usr/bin is the
# location of "make"
#
# This then will run "make install" via sudo with root privileges when you use
# the relevant "Deploy" option on the "Build" menu - and will ask you for YOUR
# password via a GUI dialog if needed - so that the files can be placed in the
# specified system directories to which a normal user (you?) does not have write
# access normally.

# Main lua files:
LUA.files = \
    $${PWD}/mudlet-lua/lua/DB.lua \
    $${PWD}/mudlet-lua/lua/DebugTools.lua \
    $${PWD}/mudlet-lua/lua/GMCP.lua \
    $${PWD}/mudlet-lua/lua/GUIUtils.lua \
    $${PWD}/mudlet-lua/lua/LuaGlobal.lua \
    $${PWD}/mudlet-lua/lua/Other.lua \
    $${PWD}/mudlet-lua/lua/StringUtils.lua \
    $${PWD}/mudlet-lua/lua/TableUtils.lua
LUA.depends = mudlet

# A variable used to write a file who's sole purpose is to record the absolute
# location of the above files, no trailing slash:
LUA_SOURCE_LOCATION = $${PWD}/mudlet-lua/lua

# Geyser lua files:
LUA_GEYSER.files = \
    $${PWD}/mudlet-lua/lua/geyser/Geyser.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserColor.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserContainer.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserGauge.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserGeyser.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserHBox.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserLabel.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserMapper.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserMiniConsole.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserReposition.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserSetConstraints.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserTests.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserUtil.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserVBox.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserWindow.lua
LUA_GEYSER.depends = mudlet

# Pull the docs and lua files into the project so they show up in the Qt Creator
# project files list:
OTHER_FILES += \
    ${LUA.files} \
    ${LUA_GEYSER.files} \
    $${RUNTIME_FORMS} \
    ../README \
    ../COMPILE \
    ../COPYING \
    ../Doxyfile \
    ../INSTALL \
    mudlet_documentation.txt \
    mac-deploy.sh \
    ../CMakeLists.txt \
    CMakeLists.txt \
    irc/CMakeLists.txt \
    osx-installer/mudlet-appdmg.json \
    osx-installer/mudlet_osx_installer_background.png

# Unix Makefile installer:
# lua file installation, needs install, sudo, and a setting in /etc/sudo.conf
# or via enviromental variable SUDO_ASKPASS to something like ssh-askpass
# to provide a graphic password requestor needed to install software
unix:!macx: {
# say what we want to get installed by "make install" (executed by 'deployment' step):
    INSTALLS += \
        target \
        LUA \
        LUA_GEYSER
} else:macx: {
# Install for macx:
    # Copy mudlet-lua into the .app bundle
    QMAKE_BUNDLE_DATA += \
        LUA \
        LUA_GEYSER

    # Set the .app's icns file
    ICON = osx-installer/osx.icns
}
# No install process for Windows (yet?)

SUBDIRS += \
    irc/libircclient-qt.pro

# Write out a new luaLocation.h to provide the current location of the Mudlet
# core Lua files in the source files for the project, if changed please ensure
# the effective contents are the same in the toplevel cmake project file, ie.
# ../CMAkeLists.txt, for qmake the hash character has to be entered as
# $${LITERAL_HASH} otherwise it comments out the remainder of the line in this
# project file, also I have not found a way to insert an empty line into the
# file in this manner so have had to insert a single space on each such line
# which git may moan about:
luaLocationHeaderData = \
"$${LITERAL_HASH}ifndef MUDLET_LUALOCATION_H" \
"$${LITERAL_HASH}define MUDLET_LUALOCATION_H" \
"/***************************************************************************" \
" *   Copyright (C) 2014 by Stephen Lyons - slysven@virginmedia.com         *" \
" *                                                                         *" \
" *   This program is free software; you can redistribute it and/or modify  *" \
" *   it under the terms of the GNU General Public License as published by  *" \
" *   the Free Software Foundation; either version 2 of the License, or     *" \
" *   (at your option) any later version.                                   *" \
" *                                                                         *" \
" *   This program is distributed in the hope that it will be useful,       *" \
" *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *" \
" *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *" \
" *   GNU General Public License for more details.                          *" \
" *                                                                         *" \
" *   You should have received a copy of the GNU General Public License     *" \
" *   along with this program; if not, write to the                         *" \
" *   Free Software Foundation, Inc.,                                       *" \
" *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *" \
" ***************************************************************************/" \
" " \
"/*" \
" * THIS FILE IS MODIFIED/REWRITTED EACH TIME THAT THE QMAKE OR CMAKE PROJECT" \
" * FILES ARE PARSED - USER CHANGES TO THIS FILE WILL BE OVERWRITTEN!" \
" */" \
" " \
"/*" \
" * It should NOT be copied to a shared, readonly, location when the other," \
" * actual Lua files in this directory are installed on systems which support" \
" * such a situation, e.g. most *nixes with a /usr/share part in their file" \
" * system; its purpose is to permit the Mudlet application to differentiate" \
" * between: a development version of those Lua files, in the collection of" \
" * source files for this project and a normal working set installed into the" \
" * system for normal use.  For systems/installations where two such sets are" \
" * not maintained, or for release builds in package form in a distribution and" \
" * used by others, the physical absence of this file at run-time will cleanly" \
" * disable the ability to try to choose non-existant source versions of the" \
" * core Mudlet Lua files." \
" */" \
" " \
"$${LITERAL_HASH}include <QString>" \
" " \
"const QString sourceLuaHeaderPathFile = QStringLiteral( \"$${LUA_SOURCE_LOCATION}/luaLocation.h\" );" \
" " \
"$${LITERAL_HASH}endif // MUDLET_LUALOCATION_H"

message( "(Re)Writing luaLocation.h ..." )
write_file( $${LUA_SOURCE_LOCATION}/luaLocation.h, luaLocationHeaderData )
