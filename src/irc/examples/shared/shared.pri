######################################################################
# Communi
######################################################################

DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD
QMAKE_CLEAN += $$PWD/*~

HEADERS += $$PWD/channelinfo.h
HEADERS += $$PWD/commandparser.h
HEADERS += $$PWD/connectioninfo.h
HEADERS += $$PWD/messageformatter.h
HEADERS += $$PWD/messagehandler.h
HEADERS += $$PWD/messagereceiver.h
HEADERS += $$PWD/session.h
HEADERS += $$PWD/sortedusermodel.h
HEADERS += $$PWD/streamer.h
HEADERS += $$PWD/usermodel.h

SOURCES += $$PWD/commandparser.cpp
SOURCES += $$PWD/messageformatter.cpp
SOURCES += $$PWD/messagehandler.cpp
SOURCES += $$PWD/session.cpp
SOURCES += $$PWD/sortedusermodel.cpp
SOURCES += $$PWD/usermodel.cpp
