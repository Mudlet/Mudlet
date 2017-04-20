######################################################################
# Communi
######################################################################

TEMPLATE = lib
TARGET = $$qtLibraryTarget($$TARGET)
CONFIG += plugin
!verbose:CONFIG += silent
contains(QT_CONFIG, debug_and_release) {
    win32|mac:!wince*:!win32-msvc:!macx-xcode:CONFIG += debug_and_release build_all
}

IRC_SOURCEDIR = $$PWD/../../
IRC_BUILDDIR = $$OUT_PWD/../../../

IRC_MODULES = IrcCore IrcModel IrcUtil
include(../module_deps.pri)
