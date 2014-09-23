######################################################################
# Communi
######################################################################

TEMPLATE = lib
TARGET = $$qtLibraryTarget($$TARGET)
DEFINES += BUILD_PLUGIN
CONFIG += plugin
win32|mac:!wince*:!win32-msvc:!macx-xcode:CONFIG += debug_and_release build_all

COMMUNI_LIBDIR = $$OUT_PWD/../../../lib
include(../../communi.pri)
