######################################################################
# Communi
######################################################################

DEFINES += BUILD_IRC_UTIL

INCDIR = $$PWD/../../include/IrcUtil

DEPENDPATH += $$PWD $$INCDIR
INCLUDEPATH += $$PWD $$INCDIR

CONV_HEADERS2 += $$INCDIR/IrcCommandParser
CONV_HEADERS2 += $$INCDIR/IrcCommandQueue
CONV_HEADERS2 += $$INCDIR/IrcCompleter
CONV_HEADERS2 += $$INCDIR/IrcLagTimer
CONV_HEADERS2 += $$INCDIR/IrcPalette
CONV_HEADERS2 += $$INCDIR/IrcTextFormat
CONV_HEADERS2 += $$INCDIR/IrcUtil

PUB_HEADERS2 += $$INCDIR/irccommandparser.h
PUB_HEADERS2 += $$INCDIR/irccommandqueue.h
PUB_HEADERS2 += $$INCDIR/irccompleter.h
PUB_HEADERS2 += $$INCDIR/irclagtimer.h
PUB_HEADERS2 += $$INCDIR/ircpalette.h
PUB_HEADERS2 += $$INCDIR/irctextformat.h
PUB_HEADERS2 += $$INCDIR/ircutil.h

PRIV_HEADERS2 += $$INCDIR/irccommandparser_p.h
PRIV_HEADERS2 += $$INCDIR/irccommandqueue_p.h
PRIV_HEADERS2 += $$INCDIR/irclagtimer_p.h
PRIV_HEADERS2 += $$INCDIR/irctoken_p.h

HEADERS += $$PUB_HEADERS2
HEADERS += $$PRIV_HEADERS2

SOURCES += $$PWD/irccommandparser.cpp
SOURCES += $$PWD/irccommandqueue.cpp
SOURCES += $$PWD/irccompleter.cpp
SOURCES += $$PWD/irclagtimer.cpp
SOURCES += $$PWD/ircpalette.cpp
SOURCES += $$PWD/irctextformat.cpp
SOURCES += $$PWD/irctoken.cpp
SOURCES += $$PWD/ircutil.cpp
