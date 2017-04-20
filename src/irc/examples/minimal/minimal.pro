######################################################################
# Communi
######################################################################

TEMPLATE = app
TARGET = minimal
DEPENDPATH += .
INCLUDEPATH += .
CONFIG += console
CONFIG -= app_bundle
QT = core network

# Input
SOURCES += main.cpp

include(../examples.pri)
