#-------------------------------------------------
#
# Project created by QtCreator 2017-07-14T20:08:38
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dblsqd-sdk-qt
TEMPLATE = lib

SOURCES += \
    release.cpp \
    semver.cpp \
    update_dialog.cpp \
    feed.cpp

HEADERS  += \
    release.h \
    semver.h \
    update_dialog.h \
    feed.h

FORMS    += \
    update_dialog.ui

VERSION = 0.1.0

DISTFILES += \
    README.md \
    LICENSE \
    Doxyfile
