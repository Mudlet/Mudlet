######################################################################
# Communi
######################################################################

TEMPLATE = app
TARGET = bot
DEPENDPATH += .
INCLUDEPATH += .
CONFIG += console
CONFIG -= app_bundle
QT = core network

# Input
HEADERS += ircbot.h
SOURCES += ircbot.cpp main.cpp

include(../examples.pri)
