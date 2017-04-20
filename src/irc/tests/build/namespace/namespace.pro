######################################################################
# Communi
######################################################################

TEMPLATE = app
TARGET = embed
QT += network

DEFINES += IRC_NAMESPACE=Communi
include(../../../src/src.pri)

SOURCES += main.cpp
