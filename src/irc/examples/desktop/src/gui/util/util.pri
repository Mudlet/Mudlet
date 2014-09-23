######################################################################
# Communi: util.pri
######################################################################

DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD
QMAKE_CLEAN += $$PWD/*~

HEADERS += $$PWD/completer.h
HEADERS += $$PWD/historylineedit.h
HEADERS += $$PWD/sharedtimer.h
HEADERS += $$PWD/textbrowser.h

SOURCES += $$PWD/completer.cpp
SOURCES += $$PWD/historylineedit.cpp
SOURCES += $$PWD/sharedtimer.cpp
SOURCES += $$PWD/textbrowser.cpp
