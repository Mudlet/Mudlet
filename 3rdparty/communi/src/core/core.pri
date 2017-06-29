######################################################################
# Communi
######################################################################

DEFINES += BUILD_IRC_CORE

INCDIR = $$PWD/../../include/IrcCore

DEPENDPATH += $$PWD $$INCDIR
INCLUDEPATH += $$PWD $$INCDIR

CONV_HEADERS  = $$INCDIR/Irc
CONV_HEADERS += $$INCDIR/IrcCommand
CONV_HEADERS += $$INCDIR/IrcCommandFilter
CONV_HEADERS += $$INCDIR/IrcConnection
CONV_HEADERS += $$INCDIR/IrcCore
CONV_HEADERS += $$INCDIR/IrcGlobal
CONV_HEADERS += $$INCDIR/IrcMessage
CONV_HEADERS += $$INCDIR/IrcMessageFilter
CONV_HEADERS += $$INCDIR/IrcNetwork
CONV_HEADERS += $$INCDIR/IrcProtocol

PUB_HEADERS  = $$INCDIR/irc.h
PUB_HEADERS += $$INCDIR/irccommand.h
PUB_HEADERS += $$INCDIR/ircconnection.h
PUB_HEADERS += $$INCDIR/irccore.h
PUB_HEADERS += $$INCDIR/ircfilter.h
PUB_HEADERS += $$INCDIR/ircglobal.h
PUB_HEADERS += $$INCDIR/ircmessage.h
PUB_HEADERS += $$INCDIR/ircnetwork.h
PUB_HEADERS += $$INCDIR/ircprotocol.h

PRIV_HEADERS  = $$INCDIR/irccommand_p.h
PRIV_HEADERS += $$INCDIR/ircconnection_p.h
PRIV_HEADERS += $$INCDIR/ircdebug_p.h
PRIV_HEADERS += $$INCDIR/ircmessage_p.h
PRIV_HEADERS += $$INCDIR/ircmessagecomposer_p.h
PRIV_HEADERS += $$INCDIR/ircmessagedecoder_p.h
PRIV_HEADERS += $$INCDIR/ircnetwork_p.h

HEADERS *= $$PUB_HEADERS
HEADERS *= $$PRIV_HEADERS

SOURCES *= $$PWD/irc.cpp
SOURCES *= $$PWD/irccommand.cpp
SOURCES *= $$PWD/ircconnection.cpp
SOURCES *= $$PWD/irccore.cpp
SOURCES *= $$PWD/ircfilter.cpp
SOURCES *= $$PWD/ircmessage.cpp
SOURCES *= $$PWD/ircmessage_p.cpp
SOURCES *= $$PWD/ircmessagecomposer.cpp
SOURCES *= $$PWD/ircmessagedecoder.cpp
SOURCES *= $$PWD/ircnetwork.cpp
SOURCES *= $$PWD/ircprotocol.cpp

include(../3rdparty/mozilla/mozilla.pri)

CONFIG(icu, icu|no_icu) {
    DEFINES += HAVE_ICU
    SOURCES += $$PWD/ircmessagedecoder_icu.cpp
    include(../3rdparty/icu/icu.pri)
} else:CONFIG(uchardet, uchardet|no_uchardet) {
    DEFINES += HAVE_UCHARDET
    SOURCES += $$PWD/ircmessagedecoder_uchardet.cpp
    include(../3rdparty/uchardet-0.0.1/uchardet.pri)
} else {
    SOURCES += $$PWD/ircmessagedecoder_none.cpp
}
