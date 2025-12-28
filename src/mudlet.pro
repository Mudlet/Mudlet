############################################################################
#    Copyright (C) 2013-2015, 2017-2018, 2020-2025 by Stephen Lyons        #
#                                                - slysven@virginmedia.com #
#    Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            #
#    Copyright (C) 2017 by Ian Adkins - ieadkins@gmail.com                 #
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

############################################################################
#                                                                          #
#    NOTICE: FreeBSD, OpenBSD and GNU/Hurd are not officially supported    #
#    platforms as such; the work on getting them working has been done by  #
#    myself, and other developers, unless they have explicitly said so,    #
#    are not able to address issues relating specifically to these         #
#    Operating Systems. Nevertheless users of these operating systems are  #
#    equally welcome to contribute to the development of Mudlet - bugfixes #
#    and enhancements are welcome from all!                                #
#        Stephen Lyons, February 2018, updated March 2021 & October 2022   #
#                                                                          #
############################################################################

if(lessThan(QT_MAJOR_VERSION,6)) {
    error("Mudlet requires Qt 6.8.2 or later")
}
if(equals(QT_MAJOR_VERSION,6):lessThan(QT_MINOR_VERSION,8)) {
    error("Mudlet requires Qt 6.8.2 or later")
}
if(equals(QT_MAJOR_VERSION,6):equals(QT_MINOR_VERSION,8):lessThan(QT_PATCH_VERSION,2)) {
    error("Mudlet requires Qt 6.8.2 or later")
}

# Including IRC Library
include(../3rdparty/communi/communi.pri)

!build_pass{
    include(../translations/translated/updateqm.pri)
}

# Before we impose OUR idea about the optimisation levels to use, remove any
# that Qt tries to put in automatically for us for release builds, only the
# last, ours, is supposed to apply but it can be confusing to see multiple
# alternatives during compilations.
QMAKE_CXXFLAGS_RELEASE ~= s/-O[0123s]//g
QMAKE_CFLAGS_RELEASE ~= s/-O[0123s]//g
# NOW we can put ours in:
QMAKE_CXXFLAGS_RELEASE += -O3
QMAKE_CFLAGS_RELEASE += -O3
# There is NO need to put in the -g option as it is done already for debug bugs
# For gdb type debugging it helps if there is NO optimisations so use -O0.
QMAKE_CXXFLAGS_DEBUG += -O0
QMAKE_CFLAGS_DEBUG += -O0

# c++20 for Qt 6
CONFIG += c++20

# Mac specific flags.
macx:QMAKE_MACOSX_DEPLOYMENT_TARGET = 12.0

# Used to force an include of winsock2.h BEFORE Qt tries to include winsock.h
# from windows.h - only needed on Windows builds but we cannot use Q_OS_WINDOWS
# for an #ifdef because we need a symbol that is defined BEFORE we include
# any Qt header file!
win32 {
    DEFINES += INCLUDE_WINSOCK2
}

# Qt6 Core5Compat is needed by third-party libraries (communi, edbee-lib)
# Mudlet itself doesn't use it, but these dependencies still require it
QT += network uitools multimedia multimediawidgets gui concurrent core5compat
qtHaveModule(texttospeech) {
    QT += texttospeech
    !build_pass : message("Using TextToSpeech module")
}

TEMPLATE = app

# Define a variable for the Git executable
GIT_EXECUTABLE = git

# Run the command to get the short SHA1 hash of the current HEAD, UNLESS there
# is a Git SHA1 in the BRANCH_COMMIT enviromental variable (which has been set
# by Mudlet's own CI/CB build system. This is to account for the manipulation
# that happens during a build of a PR on Mudlet's own GitHub repository which
# combines the PR state with that of the currently development branch such that
# the HEAD of that branch does not match what git rev-parse HEAD would return:
BUILD_COMMIT_TEST = $$(BUILD_COMMIT)
BUILD_COMMIT_TEST = $$lower($$BUILD_COMMIT_TEST)
!isEmpty( BUILD_COMMIT_TEST ) {
  # Building a PR in the AppVeyor environment of Mudlet's own repository
  GIT_SHA1 = $${BUILD_COMMIT_TEST}
  # Report the above, for debugging purposes:
  !build_pass:message("Git SHA1 set from the environment: " $${GIT_SHA1})
} else {
  GIT_SHA1 = $$system($$GIT_EXECUTABLE rev-parse --short HEAD)
  # Report the above, for debugging purposes:
  !build_pass:message("Git SHA1 used: " $${GIT_SHA1})
}


# Set Mudlet version (update in CMakeLists.txt as well)
VERSION = 4.19.1

# Set BUILD based on environment variable MUDLET_VERSION_BUILD or default
BUILD = $$(MUDLET_VERSION_BUILD)
!isEmpty(BUILD) {
    BUILD = $${BUILD}-$${GIT_SHA1}
} else {
    BUILD = "-dev-"$${GIT_SHA1}
}

# Write BUILD to app-build.txt, note that this adds a newline to the file
write_file(app-build.txt, BUILD)

# Log the value written to app-build.txt
!build_pass {
    isEmpty(BUILD) {
        message("Value written to app-build.txt file: {nothing}")
    } else {
        message("Value written to app-build.txt file: " $${BUILD})
    }
}

# As the above also modifies the splash screen image (so developers get reminded
# what they are working with!) Packagers (e.g. for Linux distributions) will
# want to set the environmental variable WITH_VARIABLE_SPLASH_SCREEN to NO so
# that their build does not appear to be a "DEV"elopment build!
WITH_VS_SCREEN_TEST = $$upper($$(WITH_VARIABLE_SPLASH_SCREEN))
isEmpty( WITH_VS_SCREEN_TEST ) | !equals(WITH_VS_SCREEN_TEST, "NO" ) {
    DEFINES += INCLUDE_VARIABLE_SPLASH_SCREEN
}

# Changing BUILD and VERSION values affects: ctelnet.cpp, main.cpp, mudlet.cpp
# dlgAboutDialog.cpp and TLuaInterpreter.cpp.  It does NOT cause those files to
# be automatically rebuilt so you will need to 'touch' them...!
# Use APP_VERSION and APP_TARGET defines in the source code if needed.
# APP_BUILD is going away (it is not currently used in the source code now as
# Mudlet instead reads it from the resource file) however until the CI/CB system
# is cleaned up to not use it in any way in the
# /CI/travis.validate_deployment.sh script we probably have to
# leave it in place:
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

# Some use-cases - currently Windows builds in a MSYS2+Mingw-w64 environment
# put the Lua 5.1 headers in a version numbered sub-directory of the main/normal
# system "includes" (header files) location and if there are newer version files
# they could be put in that main/normal location - in which case the wrong ones
# will be used if we don't modify the '#include' lines.
win32 {
    MSYSTEM_TEST = $$(MSYSTEM)
    # At some point in the future we might also want to check for CLANGARM64
    # as GH Actions is going to add that...
    equals(MSYSTEM_TEST, "MINGW64") | equals(MSYSTEM_TEST, "CLANG64") | equals(MSYSTEM_TEST, "UCRT64") {
        DEFINES += INCLUDE_VERSIONED_LUA_HEADERS
    }
}

################## DejuVu and Ubuntu Fonts inclusion detection #################
# To skip bundling Bitstream Vera Sans and Ubuntu Mono fonts with Mudlet,
# set the environment WITH_FONTS variable to "NO"
# ie: export WITH_UPDATER="NO" qmake
#
# Note for Mudlet developers: as WITH_FONTS could be a number, a string,
# something else (or not even # exist) we need to be careful in checking it
# exists before doing much else with it. Also as an environmental variable it
# is tricky to handle unless we read it into a qmake variable first:
FONT_TEST = $$upper($$(WITH_FONTS))
isEmpty( FONT_TEST ) | !equals(FONT_TEST, "NO" ) {
    DEFINES += INCLUDE_FONTS
    # Can download and extract latest Unbuntu font files (currently X.YY is
    # 0.83) from:
    # https://launchpad.net/ubuntu/+archive/primary/+files/ubuntu-font-family-sources_X.YY.orig.tar.gz
    # It would be nice if we could automate the download and extraction of all
    # the font and associate documentation (but NOT the "sources" sub-directory)
    # contents into the ./src/fonts/ directory structure only if this option is
    # set to ON; however that would be platform specific and add more complexity
    # and it is not obvious that there is a demand to do this currently.
}

######################### Auto Updater setting detection #########,#############
# To remove the built-in updater, set the environment WITH_UPDATER variable to "NO"
# ie: export WITH_UPDATER="NO" qmake
#
# Note for Mudlet developers: as WITH_UPDATER could be a number, a string,
# something else (or not even exist) we need to be careful in checking it exists
# before doing much else with it. Also as an environmental variable it is tricky
# to handle unless we read it into a qmake variable first:
linux|macx|win32 {
    # We are on one of the supported platforms
    UPDATER_TEST = $$upper($$(WITH_UPDATER))
    isEmpty( UPDATER_TEST ) | !equals(UPDATER_TEST, "NO" ) {
       # The environmental variable does not exist or it does and it is NOT the
       # particular value we are looking out for - so include the updater code:
       DEFINES += INCLUDE_UPDATER
    }
    # else the environment variable is the specific "don't include the updater
    # code" setting - so don't!
}
# else we are on another platform which the updater code will not support so
# don't include it either


############################### 3D mapper toggle ###############################
# To remove the 3D mapper, set the environment WITH_3DMAPPER variable to "NO"
# ie: export WITH_3DMAPPER="NO" qmake
#
3DMAPPER_TEST = $$upper($$(WITH_3DMAPPER))
isEmpty( 3DMAPPER_TEST ) | !equals(3DMAPPER_TEST, "NO" ) {
    DEFINES += INCLUDE_3DMAPPER
}

######################### Shader hot-reload toggle ############################
# To enable shader hot-reloading, set the environment WITH_SHADER_HOT_RELOAD variable to "YES"
# ie: export WITH_SHADER_HOT_RELOAD="YES" qmake
#
SHADER_HOT_RELOAD_TEST = $$upper($$(WITH_SHADER_HOT_RELOAD))
equals(SHADER_HOT_RELOAD_TEST, "YES" ) {
    DEFINES += USE_SHADER_HOT_RELOAD
    !build_pass{
        message("Shader hot-reloading is enabled in this configuration")
    }
} else {
    !build_pass{
        message("Shader hot-reloading is disabled in this configuration")
    }
}

######################## System QtKeyChain library #############################
# To use a system provided QtKeyChain library set the environmental variable
# WITH_OWN_QTKEYCHAIN variable to "NO". Note that this is only likely to be
# useful on \*nix OSes (not MacOS nor Windows). If NOT specified, (or set to
# any other value than "NO" then the build process will download and link to a
# locally built copy of the library. This is designed to help Linux and other
# distribution package builders integrate Mudlet into their system - if a system
# provided one is specified and the library is NOT available then the
# build will fail both at the compilation and the linking stages.
OWN_QTKEYCHAIN_TEST = $$upper($$(WITH_OWN_QTKEYCHAIN))
isEmpty( OWN_QTKEYCHAIN_TEST ) | !equals( OWN_QTKEYCHAIN_TEST, "NO" ) {
  DEFINES += INCLUDE_OWN_QT6_KEYCHAIN
}

###################### Platform Specific Paths and related #####################
# Specify default location for Lua files, in OS specific LUA_DEFAULT_DIR value
# below, if this is not done then a hardcoded default of a ./mudlet-lua/lua
# from the executable's location will be used.  Mudlet will now moan and ask
# the user to find them if the files (and specifically the <10KByte
# "LuaGlobal.lua" one) is not accessible (read access only required) during
# startup.  The precise directory is remembered once found (and stored in the
# Mudlet configuration file as "systemLuaFilePath") but if the installer places
# the files in the place documented here the user will not be bothered by this.
#
# (Geyser files should be in a "geyser" subdirectory of this)

# We should consider the XDG specifications in:
# https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html

########################### Debugging code inclusions ##########################
# Controls the include/selection of extra code to aide developers debug various
# features of Mudlet, generally speaking they will not be '#define'd in
# production builds - uncomment the 'DEFINES+=DEBUG_XXX' line here to include
# the relevant code:
#
# Other DEFINES+=DEBUG_XXX expected here as a result of:
# https://github.com/Mudlet/Mudlet/issues/6314
#
# * Produce a timestamped message on the OS's command line when an autosave of
# the map file gets requested and when such a request gets actioned (or
# rejected because the profile is being loaded):
# DEFINES+=DEBUG_MAPAUTOSAVE
#
# * Produce all the time the surprise that normally will only occur on the first
# day of the fourth month of the Gregorian calendar year:
# DEFINES+=DEBUG_EASTER_EGGS
#
# * Uncomment this and set to a integer value with the following bits to:
#   1 - debugging messages about WILL/WONT/DO/DONT and other commands for
#       suboptions
#   2 - more detail about the size or nature of the command:
#   4 - to monitor the use of and choice of both sockets (one each for IPv4
#       and IPv6 prototcols)
DEFINES+=DEBUG_TELNET=1
#
# * Produce qDebug() messages about the decoding of UTF-8 data when it is not
# the single bytes of pure ASCII text:
# DEFINES+=DEBUG_UTF8_PROCESSING
#
# * Produce qDebug() messages about the decoding of GB2312/GBK/GB18030 data when
# it is not the single bytes of pure ASCII text:
# DEFINES+=DEBUG_GB_PROCESSING
#
# * Produce qDebug() messages about the decoding of BIG5 data when it is not the
# single bytes of pure ASCII text:
# DEFINES+=DEBUG_BIG5_PROCESSING
#
# * Produce qDebug() messages about the decoding of EUC-KR data when it is not
# the single bytes of pure ASCII text:
# DEFINES+=DEBUG_EUC_KR_PROCESSING
#
# * Produce qDebug() messages about the decoding of ANSI SGR sequences:
# DEFINES+=DEBUG_SGR_PROCESSING
#
# * Produce qDebug() messages about the decoding of ANSI OSC sequences:
# DEFINES+=DEBUG_OSC_PROCESSING
#
# * Produce qDebug() messages about the decoding of ANSI MXP sequences although
# there is not much against this item at present {only an announcement of the
# type (?) of an `\x1b[?z` received}:
# DEFINES+=DEBUG_MXP_PROCESSING
#
# * Enable the features associated with reporting problems in processing Unicode
# codepoints that cannot be displayed on screen in a `TConsole`:
# DEFINES+=DEBUG_CODEPOINT_PROBLEMS
#
# * Produce qDebug() messages about window handling operations like dock widget
# transfers, profile switching, and detached window management:
# DEFINES+=DEBUG_WINDOW_HANDLING
#
# * Enable player icon adjustment controls in the 3D mapper for debugging and
# alignment purposes - these are normally hidden in production builds:
# DEFINES+=DEBUG_PLAYER_ICON_CONTROLS
#
# * Produce qDebug() messages about undo/redo operations in the trigger editor,
# including command execution, stack operations, and edbee text undo integration:
# DEFINES+=DEBUG_UNDO_REDO

unix:!macx {
# Distribution packagers would be using PREFIX = /usr but this is accepted
# destination place for local builds for software for all users:
    isEmpty( PREFIX ) PREFIX = /usr/local
    # Now picks up the first element of the environmental XDG_DATA_DIRS if
    # not overridden by providing a DATAROOTDIR:
    isEmpty( DATAROOTDIR ) {
        XDG_DATA_DIRS_TEST = $$(XDG_DATA_DIRS)
        ! isEmpty( XDG_DATA_DIRS_TEST ) {
            XDG_DATA_DIRS_TEST_SPLIT = $$split(XDG_DATA_DIRS_TEST, :)
            DATAROOTDIR = $$first(XDG_DATA_DIRS_TEST_SPLIT)
            message("First (most significant) element of XDG_DATA_DIRS has been determined to be: $${DATAROOTDIR} ...")
        }
        isEmpty( DATAROOTDIR ) DATAROOTDIR = $${PREFIX}/share
    }

    isEmpty( DATADIR ) DATADIR = $${DATAROOTDIR}/mudlet
# According to Linux FHS /usr/local/games is an alternative location for leasure time BINARIES 8-):
    isEmpty( BINDIR ) BINDIR = $${PREFIX}/bin
# Again according to FHS /usr/local/share/games is the corresponding place for locally built games documentation:
    isEmpty( DOCDIR ) DOCDIR = $${DATAROOTDIR}/doc/mudlet
    openbsd {
        LIBS += \
# Some OS platforms have a hyphen (I think Cygwin does as well):
            -llua5.1 \
            -lhunspell-1.7
        INCLUDEPATH += \
            /usr/local/include \
            /usr/local/include/lua-5.1
    }
    freebsd {
        LIBS += \
# Some OS platforms have a hyphen (I think Cygwin does as well):
            -llua-5.1 \
# FreeBSD appends the version number to hunspell:
            -lhunspell-1.7 \
# Needed for sysinfo(...) call in mudlet class:
            -lsysinfo
# FreeBSD (at least) supports multiple Lua versions (and 5.1 is not the default anymore):
        INCLUDEPATH += \
            /usr/local/include/lua51
    }
    linux {
        LIBS += \
            -llua5.1 \
            -lhunspell
        INCLUDEPATH += /usr/include/lua5.1
    }
    LIBS += -lpcre \
        -L/usr/local/lib/ \
        -lzip \
        -lz \
        -lpugixml

    isEmpty( 3DMAPPER_TEST ) | !equals(3DMAPPER_TEST, "NO" ) {
       LIBS += \
         -lGLU \
         -lassimp
    }

    LUA_DEFAULT_DIR = $${DATADIR}/lua
} else:win32 {
    MINGW_BASE_DIR_TEST = $$(MINGW_BASE_DIR)
    isEmpty( MINGW_BASE_DIR_TEST ) {
        # 32-BIT Build systems have been obsolete since 2020/05/15
        # https://www.msys2.org/news/#2020-05-17-32-bit-msys2-no-longer-actively-supported
        #
        # 32-Bit Targets have now been dropped - whilst a temporary reprieve
        # for some Qt libraries that Mudlet needed was granted they are no
        # longer available:
        # https://www.msys2.org/news/#2023-12-13-starting-to-drop-some-32-bit-packages
        # although it might be possible for dedicated parties to build them
        # locally for a while:
        error($$escape_expand("Build aborted as environmental variable MINGW_BASE_DIR not set to the root of \\n"\
        "the MINGW64/CLANG64/UCRT64 part typically this:\\n"\
        "'C:\msys64\mingw64' {64 Bit (MINGW64) Mudlet built on a 64 Bit Host}\\n"))
    }
    # For users/developers building with MSYS2 on Windows:
    # AND for CI building with MSYS2 for Windows in a GH Workflow:
    LIBS +=  \
        -L$${MINGW_BASE_DIR_TEST}/bin \
        -llua5.1 \
        -llibhunspell-1.7 \
        -lpcre \
        -lzip \                 # for dlgPackageExporter
        -lz \                   # for ctelnet.cpp
        -lpugixml \
        -lws2_32 \
        -loleaut32
    isEmpty( 3DMAPPER_TEST ) | !equals(3DMAPPER_TEST, "NO" ) {
        LIBS += -lassimp
    }

    INCLUDEPATH += \
        $${MINGW_BASE_DIR_TEST}/include/lua5.1 \
        $${MINGW_BASE_DIR_TEST}/include/pugixml

    # Leave this unset - we do not need it on Windows:
    # LUA_DEFAULT_DIR =
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
        LUA_TRANSLATIONS.path = $${LUA.path}/translations
        LUA_LCF.path = $${LUA.path}/lcf
        LUA_TESTS.path = $${LUA.path}/tests
# and say what will happen:
        message("Lua files will be installed to "$${LUA.path}"...")
        message("Geyser lua files will be installed to "$${LUA_GEYSER.path}"...")
        message("Lua Code Formatter lua files will be installed to "$${LUA_LCF.path}"...")
        message("Test lua files will be installed to "$${LUA_TESTS.path}"...")
    }
}

# use pkg-config whenever possible for linking on a mac
# the same should be done on the Linux platform as well
macx {
    # http://stackoverflow.com/a/16972067
    QT_CONFIG -= no-pkg-config
    CONFIG += link_pkgconfig
    PKGCONFIG += hunspell lua5.1 libpcre libzip pugixml
    INCLUDEPATH += /usr/local/include
}

# use ccache if available
BASE_CXX = $$QMAKE_CXX
BASE_C = $$QMAKE_CC
# common linux location

exists(/usr/bin/ccache)|exists(/usr/local/bin/ccache)|exists(C:/Program Files/ccache/ccache.exe)|exists(/usr/bin/ccache.exe)|exists(C:\msys64\mingw64\bin\ccache.exe)|exists(C:\msys64\mingw32\bin\ccache.exe) {
    QMAKE_CXX = ccache $$BASE_CXX
    QMAKE_CC = ccache $$BASE_C
} else {
    message("Unable to find ccache in /usr/bin/ccache, /usr/local/bin/ccache, C:/Program Files/ccache/ccache.exe, /usr/bin/ccache.exe, C:\msys64\mingw64\bin\ccache.exe, or C:\msys64\mingw64\bin\ccache.exe")
}

# There does not seem to be an obvious pkg-config option for this one, it is
# for the zlib that is used in cTelnet to expand MCCP1/2 compressed data streams:
macx:LIBS += -lz

INCLUDEPATH += ../3rdparty/discord/rpc/include

# Define a preprocessor symbol with the default fallback location from which
# to load installed mudlet lua files. Set LUA_DEFAULT_DIR to a
# platform-specific value. If LUA_DEFAULT_DIR is unset, the root directory
# will be used.
DEFINES += LUA_DEFAULT_PATH=\\\"$${LUA_DEFAULT_DIR}\\\"


####################### Git Submodules check and install #######################
# The "exists" tests and "include" directives uses qmakes internal path handling
# (always uses '/' directory separators); the git operations need to be done
# somewhere within the main git repository (which may not be the case for
# "shadow builds" so we now explicitly change directory using native shell
# command before carrying them out - however Windows cmd.exe uses a different
# command separator to Powershell (and *nix shells) so the git commands may need
# tweaking in that situation

# Edbee widget needed in all cases.
# Mudlet customised (Lua 5.1 specific) lua-code-format source code needed in
# all cases. (The code is built into a lcf module within
# TLuaInterpreter::initIndenterGlobals() on demand) - and we need to get the
# git submodule from Mudlet's own GitHub repository

# NOTE: It does SEEM possible to prebuild and install this into a system wide
# luarocks installation by changing to the "./3rdparty/lcf" directory with the
# files checked out by git (which is currently the "5.1" branch on the "Mudlet"
# repository) and creating a "lcf" "rock" using:
# "luarocks make lcf-scn-1.rockspec" {run via "sudo" on Linux!}
# there, this will result in the customised "lcf" rock being installed with a
# version of "scm-1" and it will then be handled in the same way as the other
# lua packages used via the luarocks sub-system, most specifically "utf8".  This
# method has NOT been checked thoroughly though, so YMMV.
win32 {
    # Use a check explicitly based on where the project file is in the sources
    !exists("$${PWD}/../3rdparty/edbee-lib/edbee-lib/edbee-lib.pri") {
        message("git submodule for required edbee-lib editor widget missing from source code, executing 'git submodule update --init' to get it...")
        # Changing the directory here only relates to the command following it
        # and that the change does not persist, note that ';' only works for
        # PowerShell - for cmd.exe the nearest equivalent is '&'
        system("cd $${PWD}\.. & git submodule update --init 3rdparty/edbee-lib")
    }
    !exists("$${PWD}/../3rdparty/lcf/lcf-scm-1.rockspec") {
        message("git submodule for required lua code formatter source code missing, executing 'git submodule update --init' to get it...")
        system("cd $${PWD}\.. & git submodule update --init 3rdparty/lcf")
    }
    contains( DEFINES, "INCLUDE_OWN_QT6_KEYCHAIN" ) {
        !exists("$${PWD}/../3rdparty/qtkeychain/keychain.h") {
            message("git submodule for required QtKeychain source code missing, executing 'git submodule update --init' to get it...")
            system("cd $${PWD}\.. & git submodule update --init 3rdparty/qtkeychain")
        }
    }
} else {
    !exists("$${PWD}/../3rdparty/edbee-lib/edbee-lib/edbee-lib.pri") {
        message("git submodule for required edbee-lib editor widget missing from source code, executing 'git submodule update --init' to get it...")
        system("cd $${PWD}/.. ; git submodule update --init 3rdparty/edbee-lib")
    }
    !exists("$${PWD}/../3rdparty/lcf/lcf-scm-1.rockspec") {
        message("git submodule for required lua code formatter source code missing, executing 'git submodule update --init' to get it...")
        system("cd $${PWD}/.. ; git submodule update --init 3rdparty/lcf")
    }
    contains( DEFINES, "INCLUDE_OWN_QT6_KEYCHAIN" ) {
        !exists("$${PWD}/../3rdparty/qtkeychain/keychain.h") {
            message("git submodule for required QtKeychain source code missing, executing 'git submodule update --init' to get it...")
            system("cd $${PWD}/.. ; git submodule update --init 3rdparty/qtkeychain")
        }
    }
}

contains( DEFINES, INCLUDE_UPDATER ) {
    win32 {
        # DBLSQD updater - needed for Windows
        !exists("$${PWD}/../3rdparty/dblsqd/dblsqd-sdk-qt.pri") {
            message("git submodule for optional but wanted Dblsqd updater missing from source code, executing 'git submodule update --init' to get it...")
            system("cd $${PWD}\.. & git submodule update --init 3rdparty/dblsqd")
        }
    } else {
        # DBLSQD updater - needed for Linux and MacOS
        !exists("$${PWD}/../3rdparty/dblsqd/dblsqd-sdk-qt.pri") {
            message("git submodule for optional but wanted Dblsqd updater missing from source code, executing 'git submodule update --init' to get it...")
            system("cd $${PWD}/.. ; git submodule update --init 3rdparty/dblsqd")
        }
    }
}

# Now include the submodules - and abort if the needed one are STILL missing
exists("$${PWD}/../3rdparty/edbee-lib/edbee-lib/edbee-lib.pri") {
    # Include shiny, new (and quite substantial) editor widget
    include("$${PWD}/../3rdparty/edbee-lib/edbee-lib/edbee-lib.pri")
} else {
    error("Cannot locate edbee-lib editor widget submodule source code, build abandoned!")
}

!exists("$${PWD}/../3rdparty/lcf/lcf-scm-1.rockspec") {
    error("Cannot locate lua code formatter submodule source code, build abandoned!")
}

contains( DEFINES, "INCLUDE_OWN_QT6_KEYCHAIN" ) {
    exists("$${PWD}/../3rdparty/qtkeychain/qtkeychain.pri") {
        include("$${PWD}/../3rdparty/qtkeychain/qtkeychain.pri")
    } else {
        error("Cannot locate QtKeychain submodule source code, build abandoned!")
    }
}

contains( DEFINES, INCLUDE_UPDATER ) {
    # dblsqd is needed for all update-able platforms:
    exists("$${PWD}/../3rdparty/dblsqd/dblsqd-sdk-qt.pri") {
        include("$${PWD}/../3rdparty/dblsqd/dblsqd-sdk-qt.pri")
    } else {
        error("Cannot locate Dblsqd updater submodule source code, build abandoned!")
    }
}

WITH_SENTRY {
    DEFINES += WITH_SENTRY


    CONFIG(debug, debug|release) {
        BUILD_SUBDIR = debug
    } else {
        BUILD_SUBDIR = release
    }

    DEFINES += SENTRY_DSN=\\\"$$SENTRY_DSN\\\"

    APP_DIR_PATH = $$OUT_PWD/$$BUILD_SUBDIR
    DEFINES += APP_DIR_PATH=\\\"$$APP_DIR_PATH\\\"
    DEFINES += SENTRY_BUILD_STATIC

    SENTRY_PATH = $$PWD/../3rdparty/sentry-native

    # Check if sentry-native submodule is initialized
    !exists($$SENTRY_PATH/CMakeLists.txt) {
        error("Sentry is enabled (WITH_SENTRY) but the sentry-native submodule is not initialized. Either: 1) Initialize it: git submodule update --init 3rdparty/sentry-native, or 2) Disable Sentry by removing WITH_SENTRY from CONFIG")
    }

    !exists($$SENTRY_PATH/install) {
        message("Sentry install missing, building sentry-native from sources")

        system(cmake -S $$SENTRY_PATH -B $$SENTRY_PATH/build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DSENTRY_BACKEND=crashpad -D SENTRY_BUILD_SHARED_LIBS=OFF --log-level=ERROR)
        system(cmake --build $$SENTRY_PATH/build --parallel)
        system(cmake --install $$SENTRY_PATH/build --prefix $$SENTRY_PATH/install)
    }

    INCLUDEPATH += $$SENTRY_PATH/install/include
    LIBS        += -L$$SENTRY_PATH/install/lib
    LIBS += -lsentry \
        -lcrashpad_client -lcrashpad_handler_lib -lcrashpad_minidump \
        -lcrashpad_mpack -lcrashpad_snapshot -lcrashpad_tools \
        -lcrashpad_util -lmini_chromium -lcrashpad_compat -lwinhttp -ldbghelp -lversion

    QMAKE_CFLAGS_RELEASE    += -g
    QMAKE_CXXFLAGS_RELEASE  += -g
    QMAKE_LFLAGS_RELEASE    += -g -gcodeview
    QMAKE_LFLAGS_RELEASE    -= -Wl,-s


    QMAKE_POST_LINK += $$quote($$QMAKE_MOVE $$OUT_PWD/mudlet.pdb $$APP_DIR_PATH/) ;
    QMAKE_POST_LINK += $$quote(cp -f $$SENTRY_PATH/install/bin/* $$APP_DIR_PATH/) ;

    system(cmake -S $$PWD/crash_reporter/ -B $$PWD/crash_reporter/build/ -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=$$APP_DIR_PATH/)
    system(cmake --build $$PWD/crash_reporter/build/)

    SENTRY_SEND_DEBUG = $$SENTRY_SEND_DEBUG
    equals(SENTRY_SEND_DEBUG, 1) {
        SENTRY_AUTH_TOKEN = $$getenv(SENTRY_AUTH_TOKEN)
        isEmpty(SENTRY_AUTH_TOKEN) {
            error("[Option SENTRY_SEND_DEBUG enabled] The environment variable SENTRY_AUTH_TOKEN is missing. SENTRY_AUTH_TOKEN is required to authenticate with Sentry before uploading debug files. Fix: try exporting SENTRY_AUTH_TOKEN=\"...\"")
        }
        QMAKE_POST_LINK += $$quote(bash "$$PWD/../CI/send_debug_files_to_sentry.sh" "$$APP_DIR_PATH/mudlet.exe") ;
    }

    QMAKE_POST_LINK += $$quote(strip --strip-debug $$APP_DIR_PATH/mudlet.exe) ;
}

################################## File Lists ##################################
SOURCES += \
    ActionUnit.cpp \
    AliasUnit.cpp \
    AltFocusMenuBarDisable.cpp \
    WideComboBox.cpp \
    ctelnet.cpp \
    DarkTheme.cpp \
    discord.cpp \
    dlgAboutDialog.cpp \
    dlgActionMainArea.cpp \
    dlgAliasMainArea.cpp \
    dlgColorTrigger.cpp \
    dlgComposer.cpp \
    dlgConnectionProfiles.cpp \
    dlgIRC.cpp \
    dlgKeysMainArea.cpp \
    dlgModuleManager.cpp \
    dlgMapLabel.cpp \
    dlgMapper.cpp \
    dlgNotepad.cpp \
    dlgPackageExporter.cpp \
    dlgPackageManager.cpp \
    PackageItemDelegate.cpp \
    dlgProfilePreferences.cpp \
    dlgRoomExits.cpp \
    dlgRoomProperties.cpp \
    dlgScriptsMainArea.cpp \
    dlgSourceEditorArea.cpp \
    dlgSourceEditorFindArea.cpp \
    dlgSystemMessageArea.cpp \
    dlgTimersMainArea.cpp \
    dlgTriggerEditor.cpp \
    ../test/dlgTriggerEditorUndoRedoTest.cpp \
    dlgTriggerPatternEdit.cpp \
    dlgTriggersMainArea.cpp \
    dlgVarsMainArea.cpp \
    EAction.cpp \
    EditorItemXMLHelpers.cpp \
    exitstreewidget.cpp \
    FontManager.cpp \
    FileOpenHandler.cpp \
    GifTracker.cpp \
    GMCPAuthenticator.cpp \
    TrailingWhitespaceMarker.cpp \
    Host.cpp \
    HostManager.cpp \
    ircmessageformatter.cpp \
    KeyUnit.cpp \
    LlamaFileManager.cpp \
    LuaInterface.cpp \
    main.cpp \
    mapInfoContributorManager.cpp \
    mudlet.cpp \
    MudletInstanceCoordinator.cpp \
    EditorAddItemCommand.cpp \
    EditorDeleteItemCommand.cpp \
    EditorModifyPropertyCommand.cpp \
    EditorMoveItemCommand.cpp \
    EditorToggleActiveCommand.cpp \
    EditorUndoStack.cpp \
    MxpTag.cpp \
    ScriptUnit.cpp \
    SecureStringUtils.cpp \
    CredentialManager.cpp \
    ShortcutsManager.cpp \
    SingleLineTextEdit.cpp \
    T2DMap.cpp \
    CustomLineDrawContextMenuHandler.cpp \
    CustomLineDrawHandler.cpp \
    CustomLineEditContextMenuHandler.cpp \
    CustomLineEditHandler.cpp \
    CustomLineSession.cpp \
    LabelInteractionHandler.cpp \
    MiddleMousePanHandler.cpp \
    PanInteractionHandler.cpp \
    RoomContextMenuHandler.cpp \
    RoomMoveActivationHandler.cpp \
    RoomMoveDragHandler.cpp \
    SelectionRectangleHandler.cpp \
    SentryWrapper.cpp \
    TAccessibleTextEdit.cpp \
    TAction.cpp \
    TAlias.cpp \
    TArea.cpp \
    TBuffer.cpp \
    TCommandLine.cpp \
    TConsole.cpp \
    TDebug.cpp \
    TDockWidget.cpp \
    TEasyButtonBar.cpp \
    TEncodingTable.cpp \
    TEntityHandler.cpp \
    TEntityResolver.cpp \
    TFlipButton.cpp \
    TForkedProcess.cpp \
    THyperlinkCompactManager.cpp \
    THyperlinkSelectionManager.cpp \
    THyperlinkVisibilityManager.cpp \
    TimerUnit.cpp \
    TKey.cpp \
    TLabel.cpp \
    TScrollBox.cpp \
    TLinkStore.cpp \
    TLuaInterpreter.cpp \
    TLuaInterpreterAI.cpp \
    TLuaInterpreterDiscord.cpp \
    TLuaInterpreterMapper.cpp \
    TLuaInterpreterMedia.cpp \
    TLuaInterpreterMudletObjects.cpp \
    TLuaInterpreterNetworking.cpp \
    TLuaInterpreterUI.cpp \
    TMainConsole.cpp \
    TMap.cpp \
    TMapLabel.cpp \
    TMedia.cpp \
    TMediaPlaylist.cpp \
    TMxpBRTagHandler.cpp \
    TMxpHRTagHandler.cpp \
    TMxpElementDefinitionHandler.cpp \
    TMxpElementRegistry.cpp \
    TMxpEntityTagHandler.cpp \
    TMxpExpireTagHandler.cpp \
    TLuaInterpreterTextToSpeech.cpp \
    TMxpFormattingTagsHandler.cpp \
    TMxpColorTagHandler.cpp \
    TMxpCustomElementTagHandler.cpp \
    TMxpFontTagHandler.cpp \
    TMxpLinkTagHandler.cpp \
    TMxpMusicTagHandler.cpp \
    TMxpSoundTagHandler.cpp \
    TMxpMudlet.cpp \
    TMxpNodeBuilder.cpp \
    TMxpProcessor.cpp \
    TMxpSendTagHandler.cpp \
    TMxpSupportTagHandler.cpp \
    TMxpTagHandler.cpp \
    TMxpTagParser.cpp \
    TMxpTagProcessor.cpp \
    TMxpVersionTagHandler.cpp \
    TMxpVarTagHandler.cpp \
    TriggerHighlighter.cpp \
    TriggerUnit.cpp \
    TRoom.cpp \
    TRoomDB.cpp \
    TScript.cpp \
    TSplitter.cpp \
    TSplitterHandle.cpp \
    TStringUtils.cpp \
    TTabBar.cpp \
    TDetachedWindow.cpp \
    TTextCodec.cpp \
    TEncodingHelper.cpp \
    TTextEdit.cpp \
    TTimer.cpp \
    TToolBar.cpp \
    TTreeWidget.cpp \
    TTrigger.cpp \
    TVar.cpp \
    VarUnit.cpp \
    XMLexport.cpp \
    XMLimport.cpp

HEADERS += \
    ../3rdparty/discord/rpc/include/discord_register.h \
    ../3rdparty/discord/rpc/include/discord_rpc.h \
    ActionUnit.h \
    AliasUnit.h \
    AltFocusMenuBarDisable.h \
    WideComboBox.h \
    ctelnet.h \
    DarkTheme.h \
    discord.h \
    dlgAboutDialog.h \
    dlgActionMainArea.h \
    dlgAliasMainArea.h \
    dlgColorTrigger.h \
    dlgComposer.h \
    dlgConnectionProfiles.h \
    dlgIRC.h \
    dlgKeysMainArea.h \
    dlgMapLabel.h \
    dlgMapper.h \
    dlgModuleManager.h \
    dlgNotepad.h \
    dlgPackageExporter.h \
    dlgPackageManager.h \
    PackageItemDelegate.h \
    dlgProfilePreferences.h \
    dlgRoomExits.h \
    dlgRoomProperties.h \
    dlgScriptsMainArea.h \
    dlgSourceEditorArea.h \
    dlgSourceEditorFindArea.h \
    dlgSystemMessageArea.h \
    dlgTimersMainArea.h \
    dlgTriggerEditor.h \
    dlgTriggerPatternEdit.h \
    dlgTriggersMainArea.h \
    dlgVarsMainArea.h \
    EAction.h \
    EditorItemXMLHelpers.h \
    exitstreewidget.h \
    FileOpenHandler.h \
    GifTracker.h \
    GMCPAuthenticator.h \
    TrailingWhitespaceMarker.h \
    Host.h \
    HostManager.h \
    ircmessageformatter.h \
    KeyUnit.h \
    LlamaFileManager.h \
    LuaInterface.h \
    mapInfoContributorManager.h \
    mudlet.h \
    MudletInstanceCoordinator.h \
    EditorCommand.h \
    EditorAddItemCommand.h \
    EditorDeleteItemCommand.h \
    EditorModifyPropertyCommand.h \
    EditorMoveItemCommand.h \
    EditorToggleActiveCommand.h \
    EditorUndoStack.h \
    MxpTag.h \
    ScriptUnit.h \
    SecureStringUtils.h \
    CredentialManager.h \
    ShortcutsManager.h \
    SingleLineTextEdit.h \
    T2DMap.h \
    CustomLineDrawContextMenuHandler.h \
    CustomLineDrawHandler.h \
    CustomLineEditContextMenuHandler.h \
    CustomLineEditHandler.h \
    CustomLineSession.h \
    LabelInteractionHandler.h \
    MiddleMousePanHandler.h \
    PanInteractionHandler.h \
    RoomContextMenuHandler.h \
    RoomMoveActivationHandler.h \
    RoomMoveDragHandler.h \
    SelectionRectangleHandler.h \
    TAccessibleConsole.h \
    TAccessibleTextEdit.h \
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
    TEncodingTable.h \
    TEntityHandler.h \
    TEntityResolver.h \
    TEvent.h \
    TFlipButton.h \
    TForkedProcess.h \
    TGameDetails.h \
    THyperlinkCompactManager.h \
    THyperlinkSelectionManager.h \
    THyperlinkVisibilityManager.h \
    TimerUnit.h \
    TKey.h \
    TLabel.h \
    TLinkStore.h \
    TLuaInterpreter.h \
    TMainConsole.h \
    TMap.h \
    TMapLabel.h \
    TMatchState.h \
    TMedia.h \
    TMediaData.h \
    TMediaPlaylist.h \
    TMxpBRTagHandler.h \
    TMxpHRTagHandler.h \
    TMxpClient.h \
    TMxpColorTagHandler.h \
    TMxpCustomElementTagHandler.h \
    TMxpFontTagHandler.h \
    TMxpLinkTagHandler.h \
    TMxpMusicTagHandler.h \
    TMxpSoundTagHandler.h \
    TMxpElementDefinitionHandler.h \
    TMxpElementRegistry.h \
    TMxpEntityTagHandler.h \
    TMxpExpireTagHandler.h \
    TMxpContext.h \
    TMxpFormattingTagsHandler.h \
    TMxpMudlet.h \
    TMxpNodeBuilder.h \
    TMxpProcessor.h \
    TMxpSendTagHandler.h \
    TMxpTagHandler.h \
    TMxpTagParser.h \
    TMxpTagProcessor.h \
    TMxpSupportTagHandler.h \
    TMxpVarTagHandler.h \
    TMxpVersionTagHandler.h \
    Tree.h \
    TriggerUnit.h \
    TRoom.h \
    TRoomDB.h \
    TScript.h \
    TScrollBox.h \
    TSplitter.h \
    TSplitterHandle.h \
    TStringUtils.h \
    TriggerHighlighter.h \
    TTabBar.h \
    TDetachedWindow.h \
    TTextCodec.h \
    TEncodingHelper.h \
    TTextEdit.h \
    TTimer.h \
    TToolBar.h \
    TTreeWidget.h \
    TTrigger.h \
    TVar.h \
    VarUnit.h \
    utils.h \
    XMLexport.h \
    XMLimport.h \
    widechar_width.h \
    ../3rdparty/discord/rpc/include/discord_register.h \
    ../3rdparty/discord/rpc/include/discord_rpc.h


# This is for compiled UI files, not those used at runtime through the resource file.
FORMS += \
    ui/about_dialog.ui \
    ui/actions_main_area.ui \
    ui/aliases_main_area.ui \
    ui/color_trigger.ui \
    ui/composer.ui \
    ui/connection_profiles.ui \
    ui/dlgPackageExporter.ui \
    ui/glyph_usage.ui \
    ui/irc.ui \
    ui/keybindings_main_area.ui \
    ui/main_window.ui \
    ui/module_manager.ui \
    ui/map_label.ui \
    ui/mapper.ui \
    ui/notes_editor.ui \
    ui/package_manager.ui \
    ui/profile_preferences.ui \
    ui/room_exits.ui \
    ui/room_properties.ui \
    ui/scripts_main_area.ui \
    ui/source_editor_area.ui \
    ui/source_editor_find_area.ui \
    ui/system_message_area.ui \
    ui/timers_main_area.ui \
    ui/triggers_main_area.ui \
    ui/trigger_editor.ui \
    ui/trigger_pattern_edit.ui \
    ui/vars_main_area.ui

RESOURCES += \
    mudlet.qrc \
    ../translations/translated/qm.qrc

contains(DEFINES, "INCLUDE_VARIABLE_SPLASH_SCREEN") {
    RESOURCES += \
        additional_splash_screens.qrc
}

contains(DEFINES, INCLUDE_FONTS) {
    RESOURCES += \
        mudlet_fonts_common.qrc

    linux|freebsd {
        RESOURCES += \
            mudlet_fonts_posix.qrc
    }

    !build_pass{
        # On windows or on platforms that support CONFIG having debug_and_release"
        # then there can be three passes through this file and we only want the
        # message once (the non build_pass in that case):
        message("Including additional font resources within the Mudlet executable")
    }
} else {
    !build_pass{
        message("No font resources are to be included within the Mudlet executable")
    }
}

linux|macx|win32 {
    contains( DEFINES, INCLUDE_UPDATER ) {
        HEADERS += updater.h
        SOURCES += updater.cpp
        !build_pass{
            message("The updater code is included in this configuration")
        }
    } else {
        !build_pass{
            message("The updater code is excluded from this configuration")
        }
    }
} else {
    !build_pass{
        message("The Updater code is excluded as on-line updating is not available on this platform")
    }
}


contains( DEFINES, INCLUDE_3DMAPPER ) {
    HEADERS += glwidget.h \
               modern_glwidget.h \
               glwidget_integration.h \
               CameraController.h \
               GeometryManager.h \
               RenderCommand.h \
               RenderCommandQueue.h \
               ResourceManager.h \
               ShaderManager.h
    SOURCES += glwidget.cpp \
               modern_glwidget.cpp \
               glwidget_integration.cpp \
               CameraController.cpp \
               GeometryManager.cpp \
               RenderCommand.cpp \
               RenderCommandQueue.cpp \
               ResourceManager.cpp \
               ShaderManager.cpp

    # Enable shader hot-reloading when USE_SHADER_HOT_RELOAD is defined
    contains( DEFINES, USE_SHADER_HOT_RELOAD ) {
        DEFINES += MUDLET_SHADER_HOT_RELOAD=1
    }

    !build_pass{
        message("The 3D mapper code with both OpenGL implementations is included in this configuration for runtime selection")
    }

    QT += opengl

    win32 {
        LIBS += -lopengl32 \
                -lglu32 \
                -lassimp
    }
} else {
    !build_pass{
        message("The 3D mapper code is excluded from this configuration")
    }
}

contains( DEFINES, "INCLUDE_OWN_QT6_KEYCHAIN" ) {
    !build_pass{
        message("Including own copy of QtKeyChain library code in this configuration")
    }
} else {
    LIBS += -lqt6keychain
    !build_pass{
        message("Linking with system QtKeyChain library code in this configuration")
    }
}

TRANSLATIONS = $$files(../translations/translated/*.ts)

# To use QtCreator as a Unix installer the generated Makefile must have the
# following lists of files EXPLICITLY stated - IT DOESN'T WORK IF A WILD-CARD
# PATH IS GIVEN AS AN ENTRY TO THE .files LIST as the needed relative positions
# (sub-directories) for each file is not preserved in that case. This is not
# apparently an issue for or applies in the inclusion of the Lua files in the
# macOs bundle case!
#
# Select Qt Creator's "Project" Side tab and under the "Build and Run" top tab
# choose your Build Kit's "Run"->"Run Settings" ensure you have a "Make" step
# that - if you are NOT running QT Creator as root, which is the safest way
# (i.e safe = NOT root) - against:
# "Override <path to?>/make" has the entry: "/usr/bin/sudo"
# without the quotes, assuming /usr/bin is the location of "sudo"
# and against:
# "Make arguments" has the entry: "-A sh -c '/usr/bin/make install'"
# without the DOUBLE quotes but with the SINGLE quotes, assuming /usr/bin is the
# location of "make"
#
# Modify the "Build Environment" (and/or) the "Run Environment" so that there
# is a SUDO_ASKPASS entry with a value that points to a "ssh-askpass" or
# similar GUI password requester utility.
#
# This then will run "make install" via sudo with root privileges when you use
# the relevant "Deploy" option on the "Build" menu - and will ask you for YOUR
# password via a GUI dialog if needed - so that the files can be placed in the
# specified system directories to which a normal user (you?) does not have write
# access to normally.

# Main lua files:
LUA.files = \
    $${PWD}/mudlet-lua/lua/CursorShapes.lua \
    $${PWD}/mudlet-lua/lua/DB.lua \
    $${PWD}/mudlet-lua/lua/DebugTools.lua \
    $${PWD}/mudlet-lua/lua/GMCP.lua \
    $${PWD}/mudlet-lua/lua/GUIUtils.lua \
    $${PWD}/mudlet-lua/lua/IDManager.lua \
    $${PWD}/mudlet-lua/lua/KeyCodes.lua \
    $${PWD}/mudlet-lua/lua/TTSValues.lua \
    $${PWD}/mudlet-lua/lua/LuaGlobal.lua \
    $${PWD}/mudlet-lua/lua/Other.lua \
    $${PWD}/mudlet-lua/lua/StringUtils.lua \
    $${PWD}/mudlet-lua/lua/TableUtils.lua \
    $${PWD}/mudlet-lua/lua/utf8_filenames.lua
LUA.depends = mudlet

# Geyser lua files:
LUA_GEYSER.files = \
    $${PWD}/mudlet-lua/lua/geyser/Geyser.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserAdjustableContainer.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserColor.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserCommandLine.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserContainer.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserGauge.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserGeyser.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserHBox.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserLabel.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserMapper.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserMiniConsole.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserReposition.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserScrollBox.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserSetConstraints.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserTests.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserStyleSheet.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserUserWindow.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserUtil.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserVBox.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserWindow.lua \
    $${PWD}/mudlet-lua/lua/geyser/GeyserButton.lua
LUA_GEYSER.depends = mudlet

LUA_TRANSLATIONS.files = \
    $${PWD}/../translations/lua/*
LUA_TRANSLATIONS.depends = mudlet

# Our customised 5.1 Lua Code Formatter files, unfortunately to get the Qt
# makefile constructor to reproduce the exact directory structure we have to
# list EVERY SINGLE B****Y FILE in a per directory grouping; also in reproducing
# the path names in the variable names below there are limits to the characters
# that can be use, so that '_' represents '/' and '__' represents '_':
LUA_LCF.files = \
    $${PWD}/../3rdparty/lcf/get_ast.lua \
    $${PWD}/../3rdparty/lcf/get_formatter_ast.lua \
    $${PWD}/../3rdparty/lcf/lcf-scm-1.rockspec \
    $${PWD}/../3rdparty/lcf/LICENSE \
    $${PWD}/../3rdparty/lcf/lua_get_ast.lua \
    $${PWD}/../3rdparty/lcf/lua_get_formatter_ast.lua \
    $${PWD}/../3rdparty/lcf/lua_reformat.lua \
    $${PWD}/../3rdparty/lcf/readme.md \
    $${PWD}/../3rdparty/lcf/reformat.lua \
# The path for the root directory has already been specified

LUA_LCF_L1_GET__AST.files = $${PWD}/../3rdparty/lcf/get_ast/get_params.lua
LUA_LCF_L1_GET__AST.path = $${LUA_LCF.path}/get_ast

LUA_LCF_L1_GET__FORMATTER__AST.files = $${PWD}/../3rdparty/lcf/get_formatter_ast/get_params.lua
LUA_LCF_L1_GET__FORMATTER__AST.path = $${LUA_LCF.path}/get_formatter_ast

LUA_LCF_L1_REFORMAT.files = \
    $${PWD}/../3rdparty/lcf/reformat/get_params.lua \
    $${PWD}/../3rdparty/lcf/reformat/usage_text.lua
LUA_LCF_L1_REFORMAT.path = $${LUA_LCF.path}/reformat

LUA_LCF_L1_WORKSHOP.files = $${PWD}/../3rdparty/lcf/workshop/base.lua
LUA_LCF_L1_WORKSHOP.path = $${LUA_LCF.path}/workshop

LUA_LCF_L2_WORKSHOP_FILE.files = \
    $${PWD}/../3rdparty/lcf/workshop/file/as_string.lua \
    $${PWD}/../3rdparty/lcf/workshop/file/convert.lua \
    $${PWD}/../3rdparty/lcf/workshop/file/exists.lua \
    $${PWD}/../3rdparty/lcf/workshop/file/get_size.lua \
    $${PWD}/../3rdparty/lcf/workshop/file/safe_open.lua \
    $${PWD}/../3rdparty/lcf/workshop/file/text_file_as_string.lua
LUA_LCF_L2_WORKSHOP_FILE.path = $${LUA_LCF_L1_WORKSHOP.path}/file

LUA_LCF_L3_WORKSHOP_FORMATS_LUA.files = \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/is_identifier.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/keywords.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/quote_string.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/run_formatter.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/transform_ast.lua
LUA_LCF_L3_WORKSHOP_FORMATS_LUA.path = $${LUA_LCF_L1_WORKSHOP.path}/formats/lua

LUA_LCF_L4_WORKSHOP_FORMATS_LUA_AST__TRANSFORMER.files = \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/align_nodes.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/interface.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/move_comments.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/restruc_nodes.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/run.lua
LUA_LCF_L4_WORKSHOP_FORMATS_LUA_AST__TRANSFORMER.path = $${LUA_LCF_L3_WORKSHOP_FORMATS_LUA.path}/ast_transformer

LUA_LCF_L5_WORKSHOP_FORMATS_LUA_AST__TRANSFORMER_HANDLERS.files = \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/handlers/assignment.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/handlers/else_part.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/handlers/elseif_part.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/handlers/expression.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/handlers/function_call.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/handlers/function_params.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/handlers/generic_for_block.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/handlers/if_block.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/handlers/if_part.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/handlers/interface.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/handlers/key_val.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/handlers/local_assignment.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/handlers/local_named_function.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/handlers/named_function.lua\
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/handlers/numeric_for_block.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/handlers/repeat_block.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/handlers/type_function.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/handlers/var_ref.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/ast_transformer/handlers/while_block.lua \
LUA_LCF_L5_WORKSHOP_FORMATS_LUA_AST__TRANSFORMER_HANDLERS.path = $${LUA_LCF_L4_WORKSHOP_FORMATS_LUA_AST__TRANSFORMER.path}/handlers

LUA_LCF_L4_WORKSHOP_FORMATS_LUA_FORMATTER.files = \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/get_result.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/init.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/interface.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/process_block.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/process_block_multiline.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/process_block_oneline.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/process_list.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/process_list_variative.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/process_node.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/represent.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/representation_is_allowed.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/run.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/variate.lua
LUA_LCF_L4_WORKSHOP_FORMATS_LUA_FORMATTER.path = $${LUA_LCF_L3_WORKSHOP_FORMATS_LUA.path}/formatter

LUA_LCF_L5_WORKSHOP_FORMATS_LUA_FORMATTER_HANDLERS.files = \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/expression.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/interface.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/statements.lua
LUA_LCF_L5_WORKSHOP_FORMATS_LUA_FORMATTER_HANDLERS.path = $${LUA_LCF_L4_WORKSHOP_FORMATS_LUA_FORMATTER.path}/handlers

LUA_LCF_L6_WORKSHOP_FORMATS_LUA_FORMATTER_HANDLERS_EXPRESSIONS.files = \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/expressions/function_call.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/expressions/string.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/expressions/table.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/expressions/type_function.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/expressions/var_ref.lua
LUA_LCF_L6_WORKSHOP_FORMATS_LUA_FORMATTER_HANDLERS_EXPRESSIONS.path = $${LUA_LCF_L5_WORKSHOP_FORMATS_LUA_FORMATTER_HANDLERS.path}/expressions

LUA_LCF_L6_WORKSHOP_FORMATS_LUA_FORMATTER_HANDLERS_STATEMENTS.files = \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/statements/assignment.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/statements/break_statement.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/statements/comment.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/statements/goto_statement.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/statements/label_statement.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/statements/local_assignment.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/statements/return_statement.lua
LUA_LCF_L6_WORKSHOP_FORMATS_LUA_FORMATTER_HANDLERS_STATEMENTS.path = $${LUA_LCF_L5_WORKSHOP_FORMATS_LUA_FORMATTER_HANDLERS.path}/statements

LUA_LCF_L7_WORKSHOP_FORMATS_LUA_FORMATTER_HANDLERS_STATEMENTS_BLOCKS.files = \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/statements/blocks/do_block.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/statements/blocks/generic_for_block.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/statements/blocks/if_block.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/statements/blocks/local_named_function.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/statements/blocks/named_function.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/statements/blocks/numeric_for_block.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/statements/blocks/repeat_block.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/statements/blocks/while_block.lua
LUA_LCF_L7_WORKSHOP_FORMATS_LUA_FORMATTER_HANDLERS_STATEMENTS_BLOCKS.path = $${LUA_LCF_L6_WORKSHOP_FORMATS_LUA_FORMATTER_HANDLERS_STATEMENTS.path}/blocks

LUA_LCF_L6_WORKSHOP_FORMATS_LUA_FORMATTER_HANDLERS_WRAPPERS.files = \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/wrappers/bracket_expr.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/wrappers/colon_name.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/wrappers/dot_list.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/wrappers/dot_name.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/wrappers/expr_list.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/wrappers/func_args.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/wrappers/function_params.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/wrappers/name_list.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/wrappers/name_parts.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/wrappers/par_expr.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/handlers/wrappers/ref_list.lua
LUA_LCF_L6_WORKSHOP_FORMATS_LUA_FORMATTER_HANDLERS_WRAPPERS.path = $${LUA_LCF_L5_WORKSHOP_FORMATS_LUA_FORMATTER_HANDLERS.path}/wrappers

LUA_LCF_L5_WORKSHOP_FORMATS_LUA_FORMATTER_STATE__KEEPER.files = \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/state_keeper/enter_level.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/state_keeper/get_child_state.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/state_keeper/get_state.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/state_keeper/init.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/state_keeper/interface.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/state_keeper/leave_level.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/state_keeper/set_child_state.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/formatter/state_keeper/set_state.lua
LUA_LCF_L5_WORKSHOP_FORMATS_LUA_FORMATTER_STATE__KEEPER.path = $${LUA_LCF_L4_WORKSHOP_FORMATS_LUA_FORMATTER.path}/state_keeper

LUA_LCF_L4_WORKSHOP_FORMATS_LUA_QUOTE__STRING.files = \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/quote_string/custom_quotes.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/quote_string/dump.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/quote_string/intact.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/quote_string/linear.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/quote_string/quote_char.lua
LUA_LCF_L4_WORKSHOP_FORMATS_LUA_QUOTE__STRING.path = $${LUA_LCF_L3_WORKSHOP_FORMATS_LUA.path}/quote_string

LUA_LCF_L4_WORKSHOP_FORMATS_LUA_SYNTAX.files = \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/expression.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/statements.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/type_boolean.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/type_function.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/type_nil.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/type_number.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/type_string.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/type_table.lua
LUA_LCF_L4_WORKSHOP_FORMATS_LUA_SYNTAX.path = $${LUA_LCF_L3_WORKSHOP_FORMATS_LUA.path}/syntax

LUA_LCF_L5_WORKSHOP_FORMATS_LUA_SYNTAX_QUALIFIERS.files = $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/qualifiers/var_or_call.lua
LUA_LCF_L5_WORKSHOP_FORMATS_LUA_SYNTAX_QUALIFIERS.path = $${LUA_LCF_L4_WORKSHOP_FORMATS_LUA_SYNTAX.path}/syntax

LUA_LCF_L5_WORKSHOP_FORMATS_LUA_SYNTAX_STATEMENTS.files = \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/statements/assign_or_call.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/statements/break_statement.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/statements/do_block.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/statements/empty_statement.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/statements/function_body.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/statements/generic_for_block.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/statements/if_block.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/statements/local_statement.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/statements/named_function.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/statements/numeric_for_block.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/statements/repeat_block.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/statements/return_statement.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/statements/while_block.lua
LUA_LCF_L5_WORKSHOP_FORMATS_LUA_SYNTAX_STATEMENTS.path = $${LUA_LCF_L4_WORKSHOP_FORMATS_LUA_SYNTAX.path}/statements

LUA_LCF_L5_WORKSHOP_FORMATS_LUA_SYNTAX_WORDS.files = \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/words/comment.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/words/name.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/words/opt_spc.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/words/syntel.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/words/vararg.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/words/word.lua
LUA_LCF_L5_WORKSHOP_FORMATS_LUA_SYNTAX_WORDS.path = $${LUA_LCF_L4_WORKSHOP_FORMATS_LUA_SYNTAX.path}/words

LUA_LCF_L6_WORKSHOP_FORMATS_LUA_SYNTAX_WORDS_PARTICLES.files = $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/words/particles/long_bracket.lua.lua
LUA_LCF_L6_WORKSHOP_FORMATS_LUA_SYNTAX_WORDS_PARTICLES.path = $${LUA_LCF_L5_WORKSHOP_FORMATS_LUA_SYNTAX_WORDS.path}/partcles

LUA_LCF_L5_WORKSHOP_FORMATS_LUA_SYNTAX_WRAPPERS.files = \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/wrappers/bracket_expr.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/wrappers/colon_name.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/wrappers/dot_name.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/wrappers/expr_list.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/wrappers/name_list.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua/syntax/wrappers/par_expr.lua
LUA_LCF_L5_WORKSHOP_FORMATS_LUA_SYNTAX_WRAPPERS.path = $${LUA_LCF_L4_WORKSHOP_FORMATS_LUA_SYNTAX.path}/words

LUA_LCF_L3_WORKSHOP_FORMATS_LUA__TABLE.files = $${PWD}/../3rdparty/lcf/workshop/formats/lua_table/save.lua
LUA_LCF_L3_WORKSHOP_FORMATS_LUA__TABLE.path = $${LUA_LCF_L1_WORKSHOP.path}/formats/lua_table

LUA_LCF_L4_WORKSHOP_FORMATS_LUA__TABLE_SAVE.files = \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua_table/save/get_ast.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua_table/save/init.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua_table/save/interface.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua_table/save/serialize_ast.lua
LUA_LCF_L4_WORKSHOP_FORMATS_LUA__TABLE_SAVE.path = $${LUA_LCF_L3_WORKSHOP_FORMATS_LUA__TABLE.path}/save

LUA_LCF_L5_WORKSHOP_FORMATS_LUA__TABLE_SAVE_INSTALL__NODE__HANDLERS.files = \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua_table/save/install_node_handlers/minimal.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua_table/save/install_node_handlers/readable.lua
LUA_LCF_L5_WORKSHOP_FORMATS_LUA__TABLE_SAVE_INSTALL__NODE__HANDLERS.path = $${LUA_LCF_L4_WORKSHOP_FORMATS_LUA__TABLE_SAVE.path}/install_node_handlers

LUA_LCF_L3_WORKSHOP_FORMATS_LUA__TABLE__CODE.files = \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua_table_code/load.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua_table_code/save.lua
LUA_LCF_L3_WORKSHOP_FORMATS_LUA__TABLE__CODE.path = $${LUA_LCF_L1_WORKSHOP.path}/formats/lua_table_code

LUA_LCF_L4_WORKSHOP_FORMATS_LUA__TABLE__CODE_SAVE.files = \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua_table_code/save/get_ast.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua_table_code/save/init.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua_table_code/save/install_node_handlers.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua_table_code/save/interface.lua \
    $${PWD}/../3rdparty/lcf/workshop/formats/lua_table_code/save/serialize_ast.lua
LUA_LCF_L4_WORKSHOP_FORMATS_LUA__TABLE__CODE_SAVE.path = $${LUA_LCF_L3_WORKSHOP_FORMATS_LUA__TABLE__CODE.path}/save

LUA_LCF_L3_WORKSHOP_FORMATS_SH.files = $${PWD}/../3rdparty/lcf/formats/sh/load.lua
LUA_LCF_L3_WORKSHOP_FORMATS_SH.path = $${LUA_LCF_L1_WORKSHOP.path}/formats/sh

LUA_LCF_L3_WORKSHOP_FRONTEND_TEXT.files = $${PWD}/../3rdparty/lcf/workshop/frontend/text/print_msg_with_delta_time.lua
LUA_LCF_L3_WORKSHOP_FRONTEND_TEXT.path = $${LUA_LCF_L1_WORKSHOP.path}/frontend/text

LUA_LCF_L2_WORKSHOP_LUA.files =  $${PWD}/../3rdparty/lcf/workshop/lua/data_types.lua
LUA_LCF_L2_WORKSHOP_LUA.path = $${LUA_LCF_L1_WORKSHOP.path}/lua

LUA_LCF_L3_WORKSHOP_LUA_CODE.files = \
    $${PWD}/../3rdparty/lcf/workshop/lua/code/ast_as_code.lua \
    $${PWD}/../3rdparty/lcf/workshop/lua/code/get_ast.lua
LUA_LCF_L3_WORKSHOP_LUA_CODE.path = $${LUA_LCF_L1_WORKSHOP.path}/lua/code

LUA_LCF_L3_WORKSHOP_LUA_STRING.files = $${PWD}/../3rdparty/lcf/workshop/lua/string/quote.lua
LUA_LCF_L3_WORKSHOP_LUA_STRING.path = $${LUA_LCF_L1_WORKSHOP.path}/lua/string

LUA_LCF_L2_WORKSHOP_MECHS.files = \
    $${PWD}/../3rdparty/lcf/workshop/mechs/generic_loader.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/indents_table.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/name_giver.lua
LUA_LCF_L2_WORKSHOP_MECHS.path = $${LUA_LCF_L1_WORKSHOP.path}/mechs

LUA_LCF_L3_WORKSHOP_MECHS_COMMAND__LINE__PROCESSOR.files = \
    $${PWD}/../3rdparty/lcf/workshop/mechs/command_line_processor/assert_type_is_correct.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/command_line_processor/classify_item.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/command_line_processor/get_key_name.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/command_line_processor/interface.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/command_line_processor/parse_args.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/command_line_processor/run.lua
LUA_LCF_L3_WORKSHOP_MECHS_COMMAND__LINE__PROCESSOR.path = $${LUA_LCF_L2_WORKSHOP_MECHS.path}/command_line_processor

LUA_LCF_L3_WORKSHOP_MECHS_GENERIC__FILE__CONVERTER.files = \
    $${PWD}/../3rdparty/lcf/workshop/mechs/generic_file_converter/compile.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/generic_file_converter/init.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/generic_file_converter/interface.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/generic_file_converter/run.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/generic_file_converter/say.lua
LUA_LCF_L3_WORKSHOP_MECHS_GENERIC__FILE__CONVERTER.path = $${LUA_LCF_L2_WORKSHOP_MECHS.path}/generic_file_converter

LUA_LCF_L5_WORKSHOP_MECHS_GEOMETRY_1D_SEGMENTS.files = \
    $${PWD}/../3rdparty/lcf/workshop/mechs/geometry/1d/segments/is_inside.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/geometry/1d/segments/subtract_inner.lua
LUA_LCF_L5_WORKSHOP_MECHS_GEOMETRY_1D_SEGMENTS.path = $${LUA_LCF_L2_WORKSHOP_MECHS.path}/geometry/1d/segments

LUA_LCF_L3_WORKSHOP_MECHS_GRAPH.files = \
    $${PWD}/../3rdparty/lcf/workshop/mechs/graph/assembly_order.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/graph/dfs.lua
LUA_LCF_L3_WORKSHOP_MECHS_GRAPH.path = $${LUA_LCF_L2_WORKSHOP_MECHS.path}/graph

LUA_LCF_L4_WORKSHOP_MECHS_GRAPH_DFS.files = \
    $${PWD}/../3rdparty/lcf/workshop/mechs/graph/dfs/dfs.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/graph/dfs/get_children.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/graph/dfs/interface.lua
LUA_LCF_L4_WORKSHOP_MECHS_GRAPH_DFS.path = $${LUA_LCF_L3_WORKSHOP_MECHS_GRAPH.path}/dfs

LUA_LCF_L4_WORKSHOP_MECHS_NUMBER_REPRESENTER.files = \
    $${PWD}/../3rdparty/lcf/workshop/mechs/number/representer/interface.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/number/representer/represent.lua
LUA_LCF_L4_WORKSHOP_MECHS_NUMBER_REPRESENTER.path = $${LUA_LCF_L2_WORKSHOP_MECHS.path}/number/representer

LUA_LCF_L5_WORKSHOP_MECHS_NUMBER_REPRESENTER_UNITS.files = \
    $${PWD}/../3rdparty/lcf/workshop/mechs/number/representer/units/binary_bytes.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/number/representer/units/binary_units.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/number/representer/units/frequency.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/number/representer/units/general_number.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/number/representer/units/general_time.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/number/representer/units/interface.lua
LUA_LCF_L5_WORKSHOP_MECHS_NUMBER_REPRESENTER_UNITS.path = $${LUA_LCF_L4_WORKSHOP_MECHS_NUMBER_REPRESENTER.path}/units

LUA_LCF_L3_WORKSHOP_MECHS_PARSER.files = \
    $${PWD}/../3rdparty/lcf/workshop/mechs/parser/get_struc.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/parser/handy.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/parser/on_match.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/parser/parse.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/parser/populate.lua
LUA_LCF_L3_WORKSHOP_MECHS_PARSER.path = $${LUA_LCF_L2_WORKSHOP_MECHS.path}/parser

LUA_LCF_L4_WORKSHOP_MECHS_PARSER_FOLDER.files = \
    $${PWD}/../3rdparty/lcf/workshop/mechs/parser/folder/fold.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/parser/folder/get_struc.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/parser/folder/init.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/parser/folder/interface.lua
LUA_LCF_L4_WORKSHOP_MECHS_PARSER_FOLDER.path = $${LUA_LCF_L3_WORKSHOP_MECHS_PARSER.path}/folder

LUA_LCF_L3_WORKSHOP_MECHS_PROCESSOR.files = \
    $${PWD}/../3rdparty/lcf/workshop/mechs/processor/handy.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/processor/link.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/processor/optimize.lua
LUA_LCF_L3_WORKSHOP_MECHS_PROCESSOR.path = $${LUA_LCF_L2_WORKSHOP_MECHS.path}/processor

LUA_LCF_L4_WORKSHOP_MECHS_PROCESSOR_CORE.files = \
    $${PWD}/../3rdparty/lcf/workshop/mechs/processor/core/init.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/processor/core/interface.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/processor/core/match.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/processor/core/on_match.lua
LUA_LCF_L4_WORKSHOP_MECHS_PROCESSOR_CORE.path = $${LUA_LCF_L3_WORKSHOP_MECHS_PROCESSOR.path}/core

LUA_LCF_L4_WORKSHOP_MECHS_STREAMS_MERGEABLE.files = \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/mergeable/block_read.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/mergeable/get_segment.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/mergeable/get_slot.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/mergeable/init.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/mergeable/interface.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/mergeable/set_next_position.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/mergeable/set_relative_position.lua
LUA_LCF_L4_WORKSHOP_MECHS_STREAMS_MERGEABLE.path = $${LUA_LCF_L2_WORKSHOP_MECHS.path}/streams/mergeable

LUA_LCF_L5_WORKSHOP_MECHS_STREAMS_MERGEABLE_STRING.files = \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/mergeable/string/get_length.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/mergeable/string/get_position.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/mergeable/string/init.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/mergeable/string/interface.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/mergeable/string/match_regexp.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/mergeable/string/match_string.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/mergeable/string/read.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/mergeable/string/set_position.lua
LUA_LCF_L5_WORKSHOP_MECHS_STREAMS_MERGEABLE_STRING.path = $${LUA_LCF_L4_WORKSHOP_MECHS_STREAMS_MERGEABLE.path}/string

LUA_LCF_L4_WORKSHOP_MECHS_STREAMS_SEQUENCE.files = \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/sequence/get_position.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/sequence/init.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/sequence/interface.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/sequence/read.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/sequence/set_position.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/streams/sequence/write.lua
LUA_LCF_L4_WORKSHOP_MECHS_STREAMS_SEQUENCE.path = $${LUA_LCF_L2_WORKSHOP_MECHS.path}/streams/sequence

LUA_LCF_L3_WORKSHOP_MECHS_TEXT__BLOCKS.files = \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/dec_indent.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/inc_indent.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/init.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/interface.lua
LUA_LCF_L3_WORKSHOP_MECHS_TEXT__BLOCKS.path = $${LUA_LCF_L2_WORKSHOP_MECHS.path}/text_block

LUA_LCF_L4_WORKSHOP_MECHS_TEXT__BLOCKS_LINE.files = \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/line/add.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/line/get_line_length.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/line/get_line.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/line/get_text_length.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/line/init.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/line/interface.lua
LUA_LCF_L4_WORKSHOP_MECHS_TEXT__BLOCKS_LINE.path = $${LUA_LCF_L3_WORKSHOP_MECHS_TEXT__BLOCKS.path}/line

LUA_LCF_L4_WORKSHOP_MECHS_TEXT__BLOCKS_TEXT.files = \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/text/add_curline.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/text/add_textline.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/text/get_block_width.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/text/get_text.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/text/get_text_width.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/text/include.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/text/new_line.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/text/on_clean_line.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/text/request_clean_line.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/text/request_empty_line.lua \
    $${PWD}/../3rdparty/lcf/workshop/mechs/text_block/text/store_textline.lua
LUA_LCF_L4_WORKSHOP_MECHS_TEXT__BLOCKS_TEXT.path = $${LUA_LCF_L3_WORKSHOP_MECHS_TEXT__BLOCKS.path}/text

LUA_LCF_L2_WORKSHOP_NUMBER.files = \
    $${PWD}/../3rdparty/lcf/workshop/number/represent_size.lua \
    $${PWD}/../3rdparty/lcf/workshop/number/represent_time.lua
LUA_LCF_L2_WORKSHOP_NUMBER.path = $${LUA_LCF_L1_WORKSHOP.path}/number

LUA_LCF_L2_WORKSHOP_STRING.files = \
    $${PWD}/../3rdparty/lcf/workshop/string/content_attributes.lua \
    $${PWD}/../3rdparty/lcf/workshop/string/save_to_file.lua \
    $${PWD}/../3rdparty/lcf/workshop/string/trim_head.lua \
    $${PWD}/../3rdparty/lcf/workshop/string/trim_linefeed.lua \
    $${PWD}/../3rdparty/lcf/workshop/string/trim.lua \
    $${PWD}/../3rdparty/lcf/workshop/string/trim_tail.lua
LUA_LCF_L2_WORKSHOP_STRING.path = $${LUA_LCF_L1_WORKSHOP.path}/string

LUA_LCF_L3_WORKSHOP_STRING_LINES.files = $${PWD}/../3rdparty/lcf/workshop/string/lines/get_next_line.lua
LUA_LCF_L3_WORKSHOP_STRING_LINES.path = $${LUA_LCF_L2_WORKSHOP_STRING.path}/lines

LUA_LCF_L2_WORKSHOP_STRUC.files = $${PWD}/../3rdparty/lcf/workshop/struc/compile.lua
LUA_LCF_L2_WORKSHOP_STRUC.path = $${LUA_LCF_L1_WORKSHOP.path}/struc

LUA_LCF_L2_WORKSHOP_SYSTEM.files = \
    $${PWD}/../3rdparty/lcf/workshop/system/install_assert_functions.lua \
    $${PWD}/../3rdparty/lcf/workshop/system/install_is_functions.lua
LUA_LCF_L2_WORKSHOP_SYSTEM.path = $${LUA_LCF_L1_WORKSHOP.path}/system

LUA_LCF_L2_WORKSHOP_TABLE.files = \
    $${PWD}/../3rdparty/lcf/workshop/table/as_string.lua \
    $${PWD}/../3rdparty/lcf/workshop/table/clone.lua \
    $${PWD}/../3rdparty/lcf/workshop/table/get_key_vals.lua \
    $${PWD}/../3rdparty/lcf/workshop/table/map_values.lua \
    $${PWD}/../3rdparty/lcf/workshop/table/merge.lua \
    $${PWD}/../3rdparty/lcf/workshop/table/new.lua \
    $${PWD}/../3rdparty/lcf/workshop/table/ordered_pass.lua \
    $${PWD}/../3rdparty/lcf/workshop/table/replace.lua \
    $${PWD}/../3rdparty/lcf/workshop/table/patch.lua \
    $${PWD}/../3rdparty/lcf/workshop/table/unfold.lua
LUA_LCF_L2_WORKSHOP_TABLE.path = $${LUA_LCF_L1_WORKSHOP.path}/table

LUA_LCF_L3_WORKSHOP_TABLE_ORDERED__PASS.files = $${PWD}/../3rdparty/lcf/workshop/table/ordered_pass/default_comparator.lua
LUA_LCF_L3_WORKSHOP_TABLE_ORDERED__PASS.path = $${LUA_LCF_L1_WORKSHOP.path}/table/ordered_pass
LUA_LCF.depends = mudlet

# Test lua files:
LUA_TESTS.files = \
    $${PWD}/mudlet-lua/tests/DB_spec.lua \
    $${PWD}/mudlet-lua/tests/GUIUtils_spec.lua \
    $${PWD}/mudlet-lua/tests/MudletBusted_spec.lua \
    $${PWD}/mudlet-lua/tests/Other_spec.lua
LUA_TESTS.depends = mudlet


macx {
    # Copy mudlet-lua into the .app bundle
    # the location is relative to src.pro, so just use mudlet-lua
    APP_MUDLET_LUA_FILES.files = \
        mudlet-lua \
        de_AT_frami.aff \
        de_AT_frami.dic \
        de_CH_frami.aff \
        de_CH_frami.dic \
        de_DE_frami.aff \
        de_DE_frami.dic \
        el_GR.aff \
        el_GR.dic \
        en_GB.aff \
        en_GB.dic \
        en_US.aff \
        en_US.dic \
        es_ES.aff \
        es_ES.dic \
        fr.aff \
        fr.dic \
        it_IT.aff \
        it_IT.dic \
        nl_NL.aff \
        nl_NL.dic \
        pl_PL.aff \
        pl_PL.dic \
        pt_PT.aff \
        pt_PT.dic \
        pt_BR.aff \
        pt_BR.dic \
        ru_RU.dic \
        ru_RU.aff

    APP_MUDLET_LUA_FILES.path  = Contents/Resources
    QMAKE_BUNDLE_DATA += APP_MUDLET_LUA_FILES

    APP_MUDLET_LUA_TRANSLATION.files = \
        ../translations/lua

    APP_MUDLET_LUA_TRANSLATION.path = Contents/Resources/translations
    QMAKE_BUNDLE_DATA += APP_MUDLET_LUA_TRANSLATION

    # Set the macOS application's icon
    contains(BUILD, "-ptb.+") {
        ICON = icons/mudlet_ptb.icns
    } else {
        contains(BUILD, "-dev.+")|contains(BUILD, "-test.+") {
            ICON = icons/mudlet_dev.icns
        } else {
            ICON = icons/mudlet.icns
        }
    }

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

        OBJECTIVE_SOURCES += sparkleupdater.mm
        HEADERS += sparkleupdater.h
        # Copy Sparkle into the app bundle
        sparkle.path = Contents/Frameworks
        sparkle.files = $$SPARKLE_PATH/Sparkle.framework
        QMAKE_BUNDLE_DATA += sparkle
    }

    # And add frameworks to the rpath so that the app can find the framework.
    QMAKE_RPATHDIR += @executable_path/../Frameworks
}

win32 {
    # set the Windows binary icon, a proper .ico file will contains several
    # images/layers in specific formats and is used in MORE than one way!
    contains(BUILD, "-ptb.+") {
        RC_ICONS = icons/mudlet_ptb.ico
    } else {
        contains(BUILD, "-dev.+")|contains(BUILD, "-test.+") {
            RC_ICONS = icons/mudlet_dev.ico
        } else {
            RC_ICONS = icons/mudlet.ico
        }
    }

    # specify some windows information about the binary
    QMAKE_TARGET_COMPANY = "Mudlet makers"
    QMAKE_TARGET_DESCRIPTION = "Mudlet the MUD client"

    # Product name determines the Windows Start Menu shortcut name
    contains(BUILD, "-ptb.+") {
        QMAKE_TARGET_PRODUCT = "Mudlet PTB"
    } else {
        QMAKE_TARGET_PRODUCT = "Mudlet"
    }
}

# This is a list of files that we want to show up in the Qt Creator IDE that are
# not otherwise used by the main project:
OTHER_FILES += \
    ../.ai/ai-instructions.md \
    ../.crowdin.yml \
    ../.cursorrules \
    ../.devcontainer/Dockerfile \
    ../.devcontainer/devcontainer.json \
    ../.devcontainer/library-scripts/desktop-lite-debian.sh \
    ../.github/CODEOWNERS \
    ../.github/codeql/codeql-config.yml \
    ../.github/codespell-wordlist.txt \
    ../.github/copilot-instructions.md \
    ../.github/dependabot.yml \
    ../.github/ISSUE_TEMPLATE.md \
    ../.github/pr-labeler.yml \
    ../.github/PULL_REQUEST_TEMPLATE.md \
    ../.github/repo-metadata.yml \
    ../.github/workflows/build-mudlet.yml \
    ../.github/workflows/build-mudlet-win.yml \
    ../.github/workflows/clangtidy-diff-analysis.yml \
    ../.github/workflows/clangtidy-post-comments.yml \
    ../.github/workflows/codeql-analysis.yml \
    ../.github/workflows/codespell-analysis.yml \
    ../.github/workflows/dangerjs.yml \
    ../.github/workflows/generate-changelog.yml \
    ../.github/workflows/generate-coder-attribution.yml \
    ../.github/workflows/qridlayout-ordering.yml \
    ../.github/workflows/link-ptbs-to-dblsqd.yml \
    ../.github/workflows/performance-analysis.yml \
    ../.github/workflows/tag-pull-requests.yml \
    ../.github/workflows/update-3rdparty.yml \
    ../.github/workflows/update-autocompletion.yml \
    ../.github/workflows/update-en-us-plural.yml \
    ../.github/workflows/update-geyser-docs.yml \
    ../.github/workflows/update-translations.yml \
    ../.gitignore \
    ../AGENTS.md \
    ../CLAUDE.md \
    ../CI/build-mudlet-for-windows.sh \
    ../CI/deploy-mudlet-for-windows.sh \
    ../CI/fix.grid.ui.ordering.js \
    ../CI/generate-changelog.lua \
    ../CI/lua-5.1.5-so.patch \
    ../CI/org.mudlet.mudlet.yml \
    ../CI/qt-silent-install.qs \
    ../CI/register-windows-release.sh \
    ../CI/package-mudlet-for-windows.sh \
    ../CI/setup-windows-sdk.sh \
    ../CI/travis.after_success.sh \
    ../CI/travis.before_install.sh \
    ../CI/travis.install.sh \
    ../CI/travis.linux.after_success.sh \
    ../CI/travis.linux.before_install.sh \
    ../CI/travis.linux.install.sh \
    ../CI/travis.osx.after_success.sh \
    ../CI/travis.osx.before_install.sh \
    ../CI/travis.osx.install.sh \
    ../CI/travis.set-build-info.sh \
    ../CI/travis.validate_deployment.sh \
    ../CI/update-autocompletion.lua \
    ../CI/validate-deployment-for-windows.sh \
    ../dangerfile.js \
    ../docker/.env.template \
    ../docker/docker-compose.override.linux.yml \
    ../docker/docker-compose.yml \
    ../docker/Dockerfile \
    ../docs/AI-ASSISTANTS.md \
    ../docs/CODE_OF_CONDUCT.md \
    ../docs/CONTRIBUTING.md \
    ../docs/FUNDING.yml \
    ../docs/SUPPORT.md \
    ../test/CMakeLists.txt \
    ../test/GUIConsoleTests.mpackage \
    ../test/CredentialManagerTest.cpp \
    ../test/SecureStringUtilsTest.cpp \
    ../test/TEntityHandlerTest.cpp \
    ../test/TEntityResolverTest.cpp \
    ../test/TLinkStoreTest.cpp \
    ../test/TLuaInterfaceTest.cpp \
    ../test/TMxpCustomElementTagHandlerTest.cpp \
    ../test/TMxpEntityTagHandlerTest.cpp \
    ../test/TMxpFormattingTagsTest.cpp \
    ../test/TMxpSendTagHandlerTest.cpp \
    ../test/TMxpStubClient.h \
    ../test/TMxpTagParserTest.cpp \
    ../test/TMxpVersionTagTest.cpp \
    ../test/TMxpElementDefinitionHandlerTest.cpp \
    mac-deploy.sh \
    mudlet-lua/genDoc.sh \
    mudlet-lua/lua/ldoc.css

# Unix Makefile installer:
# lua file installation, needs install, sudo, and a setting in /etc/sudo.conf
# or via environmental variable SUDO_ASKPASS to something like ssh-askpass
# to provide a graphic password requestor needed to install software
unix:!macx {
# say what we want to get installed by "make install" (executed by 'deployment' step):
INSTALLS += \
        target \
        LUA \
        LUA_GEYSER \
        LUA_TRANSLATIONS \
        LUA_LCF \
        LUA_LCF_L1_GET__AST \
        LUA_LCF_L1_GET__FORMATTER__AST \
        LUA_LCF_L1_REFORMAT \
        LUA_LCF_L1_WORKSHOP \
        LUA_LCF_L2_WORKSHOP_FILE \
        LUA_LCF_L3_WORKSHOP_FORMATS_LUA \
        LUA_LCF_L4_WORKSHOP_FORMATS_LUA_FORMATTER \
        LUA_LCF_L5_WORKSHOP_FORMATS_LUA_FORMATTER_HANDLERS \
        LUA_LCF_L6_WORKSHOP_FORMATS_LUA_FORMATTER_HANDLERS_EXPRESSIONS \
        LUA_LCF_L6_WORKSHOP_FORMATS_LUA_FORMATTER_HANDLERS_STATEMENTS \
        LUA_LCF_L7_WORKSHOP_FORMATS_LUA_FORMATTER_HANDLERS_STATEMENTS_BLOCKS \
        LUA_LCF_L6_WORKSHOP_FORMATS_LUA_FORMATTER_HANDLERS_WRAPPERS \
        LUA_LCF_L5_WORKSHOP_FORMATS_LUA_FORMATTER_STATE__KEEPER \
        LUA_LCF_L4_WORKSHOP_FORMATS_LUA_QUOTE__STRING \
        LUA_LCF_L4_WORKSHOP_FORMATS_LUA_SYNTAX \
        LUA_LCF_L5_WORKSHOP_FORMATS_LUA_SYNTAX_QUALIFIERS \
        LUA_LCF_L5_WORKSHOP_FORMATS_LUA_SYNTAX_STATEMENTS \
        LUA_LCF_L5_WORKSHOP_FORMATS_LUA_SYNTAX_WORDS \
        LUA_LCF_L6_WORKSHOP_FORMATS_LUA_SYNTAX_WORDS_PARTICLES \
        LUA_LCF_L5_WORKSHOP_FORMATS_LUA_SYNTAX_WRAPPERS \
        LUA_LCF_L3_WORKSHOP_FORMATS_LUA__TABLE \
        LUA_LCF_L4_WORKSHOP_FORMATS_LUA__TABLE_SAVE \
        LUA_LCF_L5_WORKSHOP_FORMATS_LUA__TABLE_SAVE_INSTALL__NODE__HANDLERS \
        LUA_LCF_L3_WORKSHOP_FORMATS_LUA__TABLE__CODE \
        LUA_LCF_L4_WORKSHOP_FORMATS_LUA__TABLE__CODE_SAVE \
        LUA_LCF_L3_WORKSHOP_FRONTEND_TEXT \
        LUA_LCF_L2_WORKSHOP_LUA \
        LUA_LCF_L3_WORKSHOP_LUA_CODE \
        LUA_LCF_L3_WORKSHOP_LUA_STRING \
        LUA_LCF_L2_WORKSHOP_MECHS \
        LUA_LCF_L3_WORKSHOP_MECHS_COMMAND__LINE__PROCESSOR \
        LUA_LCF_L3_WORKSHOP_MECHS_GENERIC__FILE__CONVERTER \
        LUA_LCF_L5_WORKSHOP_MECHS_GEOMETRY_1D_SEGMENTS \
        LUA_LCF_L3_WORKSHOP_MECHS_GRAPH \
        LUA_LCF_L4_WORKSHOP_MECHS_GRAPH_DFS \
        LUA_LCF_L4_WORKSHOP_MECHS_NUMBER_REPRESENTER \
        LUA_LCF_L5_WORKSHOP_MECHS_NUMBER_REPRESENTER_UNITS \
        LUA_LCF_L3_WORKSHOP_MECHS_PARSER \
        LUA_LCF_L4_WORKSHOP_MECHS_PARSER_FOLDER \
        LUA_LCF_L3_WORKSHOP_MECHS_PROCESSOR \
        LUA_LCF_L4_WORKSHOP_MECHS_PROCESSOR_CORE \
        LUA_LCF_L4_WORKSHOP_MECHS_STREAMS_MERGEABLE \
        LUA_LCF_L5_WORKSHOP_MECHS_STREAMS_MERGEABLE_STRING \
        LUA_LCF_L4_WORKSHOP_MECHS_STREAMS_SEQUENCE \
        LUA_LCF_L3_WORKSHOP_MECHS_TEXT__BLOCKS \
        LUA_LCF_L4_WORKSHOP_MECHS_TEXT__BLOCKS_LINE \
        LUA_LCF_L4_WORKSHOP_MECHS_TEXT__BLOCKS_TEXT \
        LUA_LCF_L2_WORKSHOP_NUMBER \
        LUA_LCF_L2_WORKSHOP_STRING \
        LUA_LCF_L3_WORKSHOP_STRING_LINES \
        LUA_LCF_L2_WORKSHOP_STRUC \
        LUA_LCF_L2_WORKSHOP_SYSTEM \
        LUA_LCF_L2_WORKSHOP_TABLE \
        LUA_LCF_L3_WORKSHOP_TABLE_ORDERED__PASS \
        LUA_TESTS
    }
# Unfortunately, because (it seems) there are some directories in the above
# that do not, themselves contain any actual files and only sub-directories
# the current Qt qmake makefile does not do a clean "make uninstall" as it
# leaves those empty sub-directories behind and that prevents their parents
# from being deleted as well

# This is the extra files that are needed to do a `make dist` given a makefile
DISTFILES += \
    $${LUA.files} \
    $${LUA_GEYSER.files} \
    $${LUA_TESTS.files} \
    $${LUA_TRANSLATIONS.files} \
    ../.clang-tidy \
    ../.gitmodules \
    ../cmake/FindHUNSPELL.cmake \
    ../cmake/FindLua51.cmake \
    ../cmake/FindPCRE.cmake \
    ../cmake/FindPUGIXML.cmake \
    ../cmake/FindSparkle.cmake \
    ../cmake/FindYAJL.cmake \
    ../cmake/FindZIP.cmake \
    ../cmake/FindZZIPLIB.cmake \
    ../cmake/IncludeOptionalModule.cmake \
    ../cmake/InitGitSubmodule.cmake \
    ../CMakeLists.txt \
    ../COMMITMENT \
    ../COMPILE \
    ../COPYING \
    ../INSTALL \
    ../mudlet.desktop \
    ../mudlet.png \
    ../mudlet.svg \
    ../README.md \
    ../translations/translated/CMakeLists.txt \
    ../translations/translated/generate-translation-stats.lua \
    ../translations/translated/updateqm.pri \
    .clang-format \
    CF-loader.xml \
    CMakeLists.txt \
    mg-loader.xml \
    mudlet-lua/lua/generic-mapper/generic_mapper.xml \
    mudlet-lua/lua/generic-mapper/versions.lua \
    mudlet-lua/tests/DB.lua \
    mudlet-lua/tests/GUIUtils.lua \
    mudlet-lua/tests/Other.lua \
    mudlet-lua/tests/README.md
