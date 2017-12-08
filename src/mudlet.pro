############################################################################
#    Copyright (C) 2013-2015, 2017 by Stephen Lyons                        #
#                                                - slysven@virginmedia.com #
#    Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            #
#    Copyright (C) 2017 by Ian Adkins - ieadkins@gmail.com                 #
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

lessThan(QT_MAJOR_VERSION, 5)|if(lessThan(QT_MAJOR_VERSION,6):lessThan(QT_MINOR_VERSION, 6)) {
    error("Mudlet requires Qt 5.6 or later")
}

# Including IRC Library
include(../3rdparty/communi/communi.pri)

# Include lua_yajl (run time lua module needed)
include(../3rdparty/lua_yajl/lua_yajl.pri)

# Include luazip module (run time lua module - but not needed on Linux/Windows as
# is available prebuilt for THOSE platforms!
macx {
    include(../3rdparty/luazip/luazip.pri)
}

# disable Qt adding -Wall for us, insert it ourselves so we can add -Wno-* after.
!msvc:CONFIG += warn_off
# ignore unused parameters, because boost has a ton of them and that is not something we need to know.
!msvc:QMAKE_CXXFLAGS += -Wall -Wno-deprecated -Wno-unused-local-typedefs -Wno-unused-parameter
# Before we impose OUR idea about the optimisation levels to use, remove any
# that Qt tries to put in automatically for us for release builds, only the
# last, ours, is supposed to apply but it can be confusing to see multiple
# alternatives during compilations.
!msvc {
    QMAKE_CXXFLAGS_RELEASE ~= s/-O[0123s]//g
    QMAKE_CFLAGS_RELEASE ~= s/-O[0123s]//g
# NOW we can put ours in:
    QMAKE_CXXFLAGS_RELEASE += -O3
    QMAKE_CFLAGS_RELEASE += -O3
# There is NO need to put in the -g option as it is done already for debug bugs
# For gdb type debugging it helps if there is NO optimisations so use -O0.
    QMAKE_CXXFLAGS_DEBUG += -O0
    QMAKE_CFLAGS_DEBUG += -O0
}

# enable C++11 for builds.
CONFIG += c++11

# MSVC specific flags. Enable multiprocessor MSVC builds.
msvc:QMAKE_CXXFLAGS += -MP

# Mac specific flags.
macx:QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.10

QT += network opengl uitools multimedia gui concurrent
qtHaveModule(gamepad): QT += gamepad

############################# TEMPORARY TESTING PART ###########################
# Tempory tests to determine what scope variables are correct, it seems that
# they are related to the complete mkspecs base directory names
# (e.g. linux-g++-64) the first part at least seems to match up with what we
#  have been using so far {"macx", "win32"} "unix" seems to be an oddity.
clear(scopes)
cygwin {
  scopes += "cygwin"
}
freebsd {
  scopes += "freebsd"
}
freebsd-clang {
  scopes += "freebsd-clang"
}
freebsd-g++ {
  scopes += "freebsd-g++"
}
linux-clang-libc++ {
  scopes += "linux-clang-libc++"
}
linux-g++-32 {
  scopes += "linux-g++-32"
}
linux-g++-64 {
# not seen when expected:
  scopes += "linux-g++-64"
}
linux-llvm {
  scopes += "linux-llvm"
}
linux-lsb-g++ {
  scopes += "linux-lsb-g++"
}
macx-g++ {
  scopes += "macx-g++"
}
macx-xcode {
  scopes += "macx-xcode"
}
win32-clang-msvc {
  scopes += "win32-clang-msvc"
}
win32-msvc {
  scopes += "win32-msvc"
}
win64 {
  scopes += "win64"
}

!isEmpty( scopes ) : message("Previously unreported scope variables tested and found: $${scopes} - slysven would like to know about them.")
# Confirmed (where):
# darwin(Travis CI), linux(local), linux-clang(local), linux-g++(local),
# macx(Travis CI), macx-clang(Travis CI), win32(AppVeyor CI),
# win32-g++(AppVeyor CI), unix(local)

# Suspected not to work:
# linux-g++-32, linux-g++-64



########################## Version and Build setting ###########################
# Set the current Mudlet Version, unfortunately the Qt documentation suggests
# that only a #.#.# form without any other alphanumberic suffixes is required:
VERSION = 3.6.1

# if you are distributing modified code, it would be useful if you
# put something distinguishing into the MUDLET_VERSION_BUILD environment
# variable to make identification of the used version simple
# the qmake BUILD variable is NOT built-in
BUILD = $$(MUDLET_VERSION_BUILD)
isEmpty( BUILD ) {
# Leave the value of the following empty for a release build
# i.e. the line should be "BUILD =" without quotes
  BUILD = "-dev"
}

# Changing BUILD and VERSION values affects: ctelnet.cpp, main.cpp, mudlet.cpp
# dlgAboutDialog.cpp and TLuaInterpreter.cpp.  It does NOT cause those files to
# be automatically rebuilt so you will need to 'touch' them...!
# Use APP_VERSION, APP_BUILD and APP_TARGET defines in the source code if needed.
DEFINES += APP_VERSION=\\\"$${VERSION}\\\"
DEFINES += APP_BUILD=\\\"$${BUILD}\\\"
# Capitalize the name for Mudlet, so it appears as 'Mudlet' and not 'mudlet' in the .dmg installer
macx {
    TARGET = Mudlet
} else {
    TARGET = mudlet
}

# Create a record of what the executable will be called by hand
# NB. "cygwin-g++" although a subset of "unix" NOT "win32" DOES create
# executables with an ".exe" extension!
DEFINES += APP_TARGET=\\\"$${TARGET}$${TARGET_EXT}\\\"


######################### Auto Updater setting detection #######################
# Enable the auto updater system by default
# unless the environmental variable WITH_UPDATER is already defined and is not
# set to (case insenstive) "no". Linux packagers will find it useful to do this
# since package managers will want to handle updates themselves
# Note: WITH_UPDATER is an environmental value/variable (could be a number, a
#           string, something else or not even exist).
#       UPDATER_TEST is a qmake variables (probably a string).
#       INCLUDE_UPDATER is a C preprocessor symbol.
linux|macx|win32 {
  # We are on one of the supported platforms
  UPDATER_TEST = $$(WITH_UPDATER)
  isEmpty( UPDATER_TEST ) | !equals($$upper(UPDATER_TEST), "NO" ) {
    # The environmental variable does not exist or it does and it is NOT the
    # particular value we are looking out for - so include the updater code:
    DEFINES += INCLUDE_UPDATER
  }
  # else the environment variable is the specific "don't include the updater
  # code" setting - so don't!
}
# else we are on another platform which the updater code will not support so
# don't include it either


###################### Platform Specific Paths and related #####################
# Specify default location for Lua files, in OS specific LUA_DEFAULT_DIR value
# below, if this is not done then a hardcoded default of a ./mudlet-lua/lua
# from the executable's location will be used.  Mudlet will now moan and ask
# the user to find them if the files (and specifically the <10KByte
# "LuaGlobal.lua" one) is not accessable (read access only required) during
# startup.  The precise directory is remembered once found (and stored in the
# Mudlet configuration file as "systemLuaFilePath") but if the installer places
# the files in the place documented here the user will not be bothered by this.
#
# (Geyser files should be in a "geyser" subdirectory of this)

# We should consider the XDG specifications in:
# https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html

unix:!macx {
# Distribution packagers would be using PREFIX = /usr but this is accepted
# destination place for local builds for software for all users:
    isEmpty( PREFIX ) PREFIX = /usr/local
    # Now picks up the first element of the environmental XDG_DATA_DIRS if
    # not overridden:
    isEmpty( DATAROOTDIR ) {
        DATAROOTDIR = $$first($$replace( XDG_DATA_DIRS, ":", " "))
        isEmpty( DATAROOTDIR ) DATAROOTDIR = $${PREFIX}/share
    }

    isEmpty( DATADIR ) DATADIR = $${DATAROOTDIR}/mudlet
# According to Linux FHS /usr/local/games is an alternative location for leasure time BINARIES 8-):
    isEmpty( BINDIR ) BINDIR = $${PREFIX}/bin
# Again according to FHS /usr/local/share/games is the corresponding place for locally built games documentation:
    isEmpty( DOCDIR ) DOCDIR = $${DATAROOTDIR}/doc/mudlet
    freebsd {
        LIBS += \
# Some OS platforms have a hyphen (I think Cygwin does as well):
            -llua-5.1\
# FreeFSB appends the version number to hunspell:
            -lhunspell-1.6
# FreeFSB (at least) supports multiple Lua versions (and 5.1 is not the default anymore):
        INCLUDEPATH += \
            /usr/local/include/lua51
    } else {
        LIBS += \
            -llua5.1 \
            -lhunspell
        INCLUDEPATH += /usr/include/lua5.1
    }
    LIBS += -lpcre \
        -L/usr/local/lib/ \
        -lyajl \
        -lGLU \
        -lzip \
        -lz
    LUA_DEFAULT_DIR = $${DATADIR}/lua
} else:win32 {
    LIBS += -L"C:\\mingw32\\bin" \
        -L"C:\\mingw32\\lib" \
        -llua51 \
        -lpcre-1 \
        -llibhunspell-1.4 \
        -lzip \                 # for dlgPackageExporter
        -lz \                   # for ctelnet.cpp
        -lyajl \
        -lopengl32 \
        -lglut \
        -lglu32
    INCLUDEPATH += "C:\\mingw32\\include"
# Leave this undefined so mudlet::readSettings() preprocessing will fall back to
# hard-coded executable's /mudlet-lua/lua/ subdirectory
#    LUA_DEFAULT_DIR = $$clean_path($$system(echo %ProgramFiles%)/lua)
}

unix:!macx {
#   the "target" install set is handled automagically, just not very well...
    target.path = $${BINDIR}
    message("$${TARGET} will be installed to "$${target.path}"...")
#     DOCS.path = $${DOCS_DIR}
#     message("Documentation will be installed to "$${DOCS.path}"...")
    !isEmpty( LUA_DEFAULT_DIR ) {
# if a directory has been set for the lua files move the detail into the
# installation details for the unix case:
        LUA.path = $${LUA_DEFAULT_DIR}
        LUA_GEYSER.path = $${LUA.path}/geyser
# and say what will happen:
        message("Lua files will be installed to "$${LUA.path}"...")
        message("Geyser lua files will be installed to "$${LUA_GEYSER.path}"...")
    }
}

# use pkg-config whenever possible for linking on a mac
# the same should be done on the Linux platform as well
macx {
    # http://stackoverflow.com/a/16972067
    QT_CONFIG -= no-pkg-config
    CONFIG += link_pkgconfig
    PKGCONFIG += hunspell lua5.1 yajl libpcre libzip
    INCLUDEPATH += /usr/local/include
}

# use ccache if available
unix {
    BASE_CXX = $$QMAKE_CXX
    # common linux location
    exists(/usr/bin/ccache):QMAKE_CXX = ccache $$BASE_CXX
    # common macos location
    exists(/usr/local/bin/ccache):QMAKE_CXX = ccache $$BASE_CXX
}

# There does not seem to be an obvious pkg-config option for these two
macx:LIBS += \
    -lz \
    -lzzip

# Define a preprocessor symbol with the default fallback location from which
# to load installed mudlet lua files. Set LUA_DEFAULT_DIR to a
# platform-specific value. If LUA_DEFAULT_DIR is unset, the root directory
# will be used.
DEFINES += LUA_DEFAULT_PATH=\\\"$${LUA_DEFAULT_DIR}\\\"


####################### Git Submodules check and install #######################
# The "exists" tests and "include" directives uses qmakes internal path handling
# (always uses '/' dirextroy separators); the git operations need to be done
# somewhere within the main git repository (which may not be the case for
# "shadow builds" so we now explicitly change directory using native shell
# command before carrying them out - however Windows cmd.exe uses a different
# command separator to Powershell (and *nix shells) so the git commands may need
# tweaking in that situation

# Edbee widget needed in all cases
win32 {
    # Use a check explicitly based on where the project file is in the sources
    !exists("$${_PRO_FILE_PWD_}/../3rdparty/edbee-lib/edbee-lib/edbee-lib.pri") {
        message("git submodule for required edbee-lib editor widget missing from source code, executing 'git submodule update --init' to get it...")
        # Lets presume that changing the directory here only relates to the
        # command following it and that the change does not persist, note that
        # ';' only works for PowerShell - for cmd.exe the nearest equivalent is
        # '&'
        system("cd $${_PRO_FILE_PWD_}\.. & git submodule update --init 3rdparty/edbee-lib")
    }

    # AppVeyor is used to build Windows binaries and it has to include the
    # lua-code-formatter {a.k.a "lcf" module} - users should instead use
    # luarocks to get this and one or two other lua modules that Mudlet looks
    # for on profile loading/startup ..."
    APPVEYOR_TEST = $$(APPVEYOR)
    !isEmpty( APPVEYOR_TEST ) {
        equals(APPVEYOR_TEST, "True") {
            # We are running in the AppVeyor CI environment
            !exists("$${_PRO_FILE_PWD_}/../3rdparty/edbee-lib/edbee-lib/edbee-lib.pri") {
                message("git submodule for lua formatter for Appveyor CI build missing from source code, executing 'git submodule update --init' to get it...")
                system("cd $${_PRO_FILE_PWD_}\.. & git submodule update --init 3rdparty/lua-code-formatter")
            }
        }
    }
} else {
    # Use a check explicitly based on where the project file is in the sources
    !exists("$${_PRO_FILE_PWD_}/../3rdparty/edbee-lib/edbee-lib/edbee-lib.pri") {
        message("git submodule for required edbee-lib editor widget missing from source code, executing 'git submodule update --init' to get it...")
        # Lets presume that changing the directory here only relates to the
        # command following it and that the change does not persist
        system("cd $${_PRO_FILE_PWD_}/.. ; git submodule update --init 3rdparty/edbee-lib")
    }
}

contains( DEFINES, INCLUDE_UPDATER ) {
    win32 {
        # DBLSQD updater - needed for Windows
        !exists("$${_PRO_FILE_PWD_}/../3rdparty/dblsqd/dblsqd-sdk-qt.pri") {
            message("git submodule for optional but wanted Dblsqd updater missing from source code, executing 'git submodule update --init' to get it...")
            system("cd $${_PRO_FILE_PWD_}\.. & git submodule update --init 3rdparty/dblsqd")
        }
    } else {
        # DBLSQD updater - needed for Linux and possibly MacOS but that needs checking
        !exists("$${_PRO_FILE_PWD_}/../3rdparty/dblsqd/dblsqd-sdk-qt.pri") {
            message("git submodule for optional but wanted Dblsqd updater missing from source code, executing 'git submodule update --init' to get it...")
            system("cd $${_PRO_FILE_PWD_}/.. ; git submodule update --init 3rdparty/dblsqd")
        }

        # Sparkle glue code - only needed for MacOs
        macx {
            !exists("$${_PRO_FILE_PWD_}/../3rdparty/sparkle-glue/mixing-cocoa-and-qt.pro") {
                message("git submodule for optional but wanted Sparkle glue missing from source code, executing 'git submodule update --init' to get it...")
                system("cd $${_PRO_FILE_PWD_}/.. ; git submodule update --init 3rdparty/sparkle-glue")
            }
        }
    }
}

# Now include the submodules - and abort if the needed one are STILL missing
exists("$${_PRO_FILE_PWD_}/../3rdparty/edbee-lib/edbee-lib/edbee-lib.pri") {
    # Include shiny, new (and quite substantial) editor widget
    include("$${_PRO_FILE_PWD_}/../3rdparty/edbee-lib/edbee-lib/edbee-lib.pri")
} else {
    error("Cannot locate edbee-lib editor widget submodule source code, build abandoned!")
}

win32 {
    APPVEYOR_TEST = $$(APPVEYOR)
    !isEmpty( APPVEYOR_TEST ) {
        equals(APPVEYOR_TEST, "True") {
            # We are running in the AppVeyor CI environment
            !exists("$${_PRO_FILE_PWD_}/../3rdparty/edbee-lib/edbee-lib/edbee-lib.pri") {
                error("Cannot locate lua-code-formatter submodule source code, build abandoned!")
            }
        }
    }
}

contains( DEFINES, INCLUDE_UPDATER ) {
    # CHECK: Is dblsqd needed for macx? If not it can be excluded for that platform
    exists("$${_PRO_FILE_PWD_}/../3rdparty/dblsqd/dblsqd-sdk-qt.pri") {
        include("$${_PRO_FILE_PWD_}/../3rdparty/dblsqd/dblsqd-sdk-qt.pri")
    } else {
        error("Cannot locate Dblsqd updater submodule source code, build abandoned!")
    }

    macx {
        # We do not actually have to do anything to include it here - it is
        # pulled in by the Sparkle complation below
        !exists("$${_PRO_FILE_PWD_}/../3rdparty/sparkle-glue/mixing-cocoa-and-qt.pro") {
            error("Cannot locate Sparkle glue library submodule source code, build abandoned!")
        }
    }
}

################################## File Lists ##################################
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
    dlgPackageExporter.cpp \
    dlgProfilePreferences.cpp \
    dlgRoomExits.cpp \
    dlgScriptsMainArea.cpp \
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
    ircmessageformatter.cpp \
    KeyUnit.cpp \
    LuaInterface.cpp \
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
    TDockWidget.cpp \
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
    TTrigger.cpp \
    TVar.cpp \
    VarUnit.cpp \
    XMLexport.cpp \
    XMLimport.cpp

contains( DEFINES, INCLUDE_UPDATER ) : SOURCES += updater.cpp


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
    dlgPackageExporter.h \
    dlgProfilePreferences.h \
    dlgRoomExits.h \
    dlgScriptsMainArea.h \
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
    ircmessageformatter.h \
    KeyUnit.h \
    LuaInterface.h \
    mudlet.h \
    pre_guard.h \
    post_guard.h \
    ScriptUnit.h \
    T2DMap.h \
    TAction.h \
    TAlias.h \
    TArea.h \
    TAstar.h \
    TBuffer.h \
    TCommandLine.h \
    TConsole.h \
    TDebug.h \
    TDockWidget.h \
    TEasyButtonBar.h \
    testdbg.h \
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
    TTrigger.h \
    TVar.h \
    VarUnit.h \
    XMLexport.h \
    XMLimport.h

contains( DEFINES, INCLUDE_UPDATER ) : HEADERS += updater.h


# This is for compiled UI files, not those used at runtime through the resource file.
FORMS += \
    ui/about_dialog.ui \
    ui/actions_main_area.ui \
    ui/aliases_main_area.ui \
    ui/color_trigger.ui \
    ui/composer.ui \
    ui/connection_profiles.ui \
    ui/dlgPackageExporter.ui \
    ui/irc.ui \
    ui/keybindings_main_area.ui \
    ui/main_window.ui \
    ui/mapper.ui \
    ui/notes_editor.ui \
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

RESOURCES = \
    mudlet.qrc

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
    $${PWD}/mudlet-lua/lua/KeyCodes.lua \
    $${PWD}/mudlet-lua/lua/LuaGlobal.lua \
    $${PWD}/mudlet-lua/lua/Other.lua \
    $${PWD}/mudlet-lua/lua/StringUtils.lua \
    $${PWD}/mudlet-lua/lua/TableUtils.lua
LUA.depends = mudlet

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

macx {
    # Copy mudlet-lua into the .app bundle
    # the location is relative to src.pro, so just use mudlet-lua
    APP_MUDLET_LUA_FILES.files = mudlet-lua en_US.aff en_US.dic
    APP_MUDLET_LUA_FILES.path  = Contents/Resources
    QMAKE_BUNDLE_DATA += APP_MUDLET_LUA_FILES

    # Set the .app's icns file
    ICON = icons/osx.icns

    LIBS += -framework AppKit

    contains( DEFINES, INCLUDE_UPDATER ) {
        # allow linker to find sparkle framework if we bundle it in
        SPARKLE_PATH = $$PWD/../3rdparty/cocoapods/Pods/Sparkle

        !exists($$SPARKLE_PATH) {
            message("Sparkle CocoaPod is missing, running 'pod install' to get it...")
            system("cd ../3rdparty/cocoapods && pod install");
        }

        LIBS += -F$$SPARKLE_PATH
        LIBS += -framework Sparkle

        # necessary for Sparkle to compile
        QMAKE_LFLAGS += -F $$SPARKLE_PATH
        QMAKE_OBJECTIVE_CFLAGS += -F $$SPARKLE_PATH

        SOURCES += ../3rdparty/sparkle-glue/AutoUpdater.cpp

        OBJECTIVE_SOURCES += ../3rdparty/sparkle-glue/SparkleAutoUpdater.mm \
            ../3rdparty/sparkle-glue/CocoaInitializer.mm

        HEADERS += ../3rdparty/sparkle-glue/AutoUpdater.h \
            ../3rdparty/sparkle-glue/SparkleAutoUpdater.h \
            ../3rdparty/sparkle-glue/CocoaInitializer.h

        # Copy Sparkle into the app bundle
        sparkle.path = Contents/Frameworks
        sparkle.files = $$SPARKLE_PATH/Sparkle.framework
        QMAKE_BUNDLE_DATA += sparkle
    }

    # And add frameworks to the rpath so that the app can find the framework.
    QMAKE_RPATHDIR += @executable_path/../Frameworks
}

win32 {
    # set the Windows binary icon
    RC_ICONS = icons/mudlet_main_512x512_6XS_icon.ico

    # specify some windows information about the binary
    QMAKE_TARGET_COMPANY = "Mudlet makers"
    QMAKE_TARGET_DESCRIPTION = "Mudlet the MUD client"
}

# Pull the docs and lua files into the project so they show up in the Qt Creator project files list
OTHER_FILES += \
    ${LUA.files} \
    ${LUA_GEYSER.files} \
    ${DISTFILES} \
    ../README \
    ../COMPILE \
    ../COPYING \
    ../INSTALL \
    mac-deploy.sh

# Unix Makefile installer:
# lua file installation, needs install, sudo, and a setting in /etc/sudo.conf
# or via enviromental variable SUDO_ASKPASS to something like ssh-askpass
# to provide a graphic password requestor needed to install software
unix:!macx {
# say what we want to get installed by "make install" (executed by 'deployment' step):
    INSTALLS += \
        target \
        LUA \
        LUA_GEYSER
}

DISTFILES += \
    ../.travis.yml \
    ../CMakeLists.txt \
    ../CI/travis.before_install.sh \
    ../CI/travis.install.sh \
    ../CI/travis.linux.before_install.sh \
    ../CI/travis.linux.install.sh \
    ../CI/travis.osx.before_install.sh \
    ../CI/travis.osx.install.sh \
    ../CI/travis.set-build-info.sh \
    ../cmake/FindHUNSPELL.cmake \
    ../cmake/FindPCRE.cmake \
    ../cmake/FindYAJL.cmake \
    ../cmake/FindZIP.cmake \
    .clang-format \
    CMakeLists.txt \
    ../.appveyor.yml \
    ../CI/travis.after_success.sh \
    ../CI/travis.linux.after_success.sh \
    ../CI/travis.osx.after_success.sh \
    ../CI/appveyor.after_success.ps1 \
    ../CI/appveyor.install.ps1 \
    ../CI/appveyor.set-build-info.ps1 \
    ../CI/appveyor.build.ps1
