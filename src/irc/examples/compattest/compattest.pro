######################################################################
# LibIrcClient-Qt
######################################################################

TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .

QT = core network
CONFIG += console no_keywords
CONFIG += libircclient-qt

# Input
HEADERS += session.h
SOURCES += session.cpp main.cpp
