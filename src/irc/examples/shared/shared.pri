######################################################################
# LibIrcClient-Qt
######################################################################

QT = core network
CONFIG += console no_keywords
CONFIG += libircclient-qt

INCLUDEPATH += ../shared
DEPENDPATH  += ../shared

HEADERS += session.h
SOURCES += session.cpp
