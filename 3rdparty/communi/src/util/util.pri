######################################################################
# Communi
######################################################################

DEFINES += BUILD_IRC_UTIL

INCDIR = $$PWD/../../include/IrcUtil

DEPENDPATH += $$PWD $$INCDIR
INCLUDEPATH += $$PWD $$INCDIR

CONV_HEADERS = $$INCDIR/IrcCommandParser
CONV_HEADERS += $$INCDIR/IrcCommandQueue
CONV_HEADERS += $$INCDIR/IrcCompleter
CONV_HEADERS += $$INCDIR/IrcLagTimer
CONV_HEADERS += $$INCDIR/IrcPalette
CONV_HEADERS += $$INCDIR/IrcTextFormat
CONV_HEADERS += $$INCDIR/IrcUtil

PUB_HEADERS = $$INCDIR/irccommandparser.h
PUB_HEADERS += $$INCDIR/irccommandqueue.h
PUB_HEADERS += $$INCDIR/irccompleter.h
PUB_HEADERS += $$INCDIR/irclagtimer.h
PUB_HEADERS += $$INCDIR/ircpalette.h
PUB_HEADERS += $$INCDIR/irctextformat.h
PUB_HEADERS += $$INCDIR/ircutil.h

PRIV_HEADERS = $$INCDIR/irccommandparser_p.h
PRIV_HEADERS += $$INCDIR/irccommandqueue_p.h
PRIV_HEADERS += $$INCDIR/irclagtimer_p.h
PRIV_HEADERS += $$INCDIR/irctoken_p.h

HEADERS *= $$PUB_HEADERS
HEADERS *= $$PRIV_HEADERS

SOURCES *= $$PWD/irccommandparser.cpp
SOURCES *= $$PWD/irccommandqueue.cpp
SOURCES *= $$PWD/irccompleter.cpp
SOURCES *= $$PWD/irclagtimer.cpp
SOURCES *= $$PWD/ircpalette.cpp
SOURCES *= $$PWD/irctextformat.cpp
SOURCES *= $$PWD/irctoken.cpp
SOURCES *= $$PWD/ircutil.cpp
