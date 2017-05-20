#-------------------------------------------------
#
# Project created by QtCreator 2013-01-03T08:11:25
#
#-------------------------------------------------

QT += core gui widgets

TARGET = edbee
TEMPLATE = lib
CONFIG += staticlib

#QMAKE_CXXFLAGS+=-fsanitize=address -fsanitize=bounds
#QMAKE_LFLAGS+=-fsanitize=address -fsanitize=bounds

# This seems to be required for Windows
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
DEFINES += QT_NODLL

include(edbee-lib.pri)
