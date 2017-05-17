#-------------------------------------------------
#
# Project created by QtCreator 2013-01-03T08:11:25
#
#-------------------------------------------------

QT += core gui widgets

TARGET = edbee
TEMPLATE = lib
CONFIG += staticlib

# This seems to be required for Windows
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
#DEFINES += QT_NODLL
DEFINES += DLL

QMAKE_CXXFLAGS += -Wall -Wno-missing-field-initializers -Wno-sized-deallocation -Wno-delete-incomplete -Wno-unused-but-set-variable -Wno-deprecated-declarations -Wno-terminate
QMAKE_CFLAGS += -Wall -Wno-missing-field-initializers -Wno-unused-but-set-variable -Wno-deprecated-declarations

include(edbee-lib.pri)
