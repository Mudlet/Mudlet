######################################################################
# Communi
######################################################################

TEMPLATE = app
TARGET = client
DEPENDPATH += .
INCLUDEPATH += .
QT = core network gui
greaterThan(QT_MAJOR_VERSION, 4):QT += widgets

# Input
HEADERS += ircclient.h ircmessageformatter.h
SOURCES += ircclient.cpp ircmessageformatter.cpp main.cpp

include(../examples.pri)
