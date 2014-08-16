######################################################################
# Communi: qtdocktile.pri
######################################################################

DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD
QMAKE_CLEAN += $$PWD/*~

HEADERS += $$PWD/qtdocktile.h
HEADERS += $$PWD/qtdocktile_p.h

SOURCES += $$PWD/qtdocktile.cpp

mac {
    LIBS += -framework AppKit
    OBJECTIVE_SOURCES += $$PWD/qtdocktile_mac.mm
}

unix:!mac {
    QT += dbus
    SOURCES += $$PWD/qtdocktile_unity.cpp
}

win32 {
    LIBS += -lole32
    HEADERS += $$PWD/winutils.h
    SOURCES += $$PWD/qtdocktile_win.cpp
}
