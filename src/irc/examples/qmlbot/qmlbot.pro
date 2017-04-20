######################################################################
# Communi
######################################################################

TEMPLATE = app
TARGET = qmlbot
DEPENDPATH += .
INCLUDEPATH += .
CONFIG += console
!static:CONFIG -= app_bundle
QT = core network qml

lessThan(QT_MAJOR_VERSION, 5)|lessThan(QT_MINOR_VERSION, 1) {
    error(The QML2 based example requires Qt 5.1 or newer but Qt $$[QT_VERSION] was detected.)
}

# Input
SOURCES += main.cpp
RESOURCES += qmlbot.qrc
OTHER_FILES += qml/main.qml

include(../examples.pri)
