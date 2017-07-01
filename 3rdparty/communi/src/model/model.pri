######################################################################
# Communi
######################################################################

DEFINES += BUILD_IRC_MODEL

INCDIR = $$PWD/../../include/IrcModel

DEPENDPATH += $$PWD $$INCDIR
INCLUDEPATH += $$PWD $$INCDIR

CONV_HEADERS = $$INCDIR/IrcBuffer
CONV_HEADERS += $$INCDIR/IrcBufferModel
CONV_HEADERS += $$INCDIR/IrcChannel
CONV_HEADERS += $$INCDIR/IrcModel
CONV_HEADERS += $$INCDIR/IrcUser
CONV_HEADERS += $$INCDIR/IrcUserModel

PUB_HEADERS = $$INCDIR/ircbuffer.h
PUB_HEADERS += $$INCDIR/ircbuffermodel.h
PUB_HEADERS += $$INCDIR/ircchannel.h
PUB_HEADERS += $$INCDIR/ircmodel.h
PUB_HEADERS += $$INCDIR/ircuser.h
PUB_HEADERS += $$INCDIR/ircusermodel.h

PRIV_HEADERS = $$INCDIR/ircbuffer_p.h
PRIV_HEADERS += $$INCDIR/ircbuffermodel_p.h
PRIV_HEADERS += $$INCDIR/ircchannel_p.h
PRIV_HEADERS += $$INCDIR/ircuser_p.h
PRIV_HEADERS += $$INCDIR/ircusermodel_p.h

HEADERS *= $$PUB_HEADERS
HEADERS *= $$PRIV_HEADERS

SOURCES *= $$PWD/ircbuffer.cpp
SOURCES *= $$PWD/ircbuffermodel.cpp
SOURCES *= $$PWD/ircchannel.cpp
SOURCES *= $$PWD/ircmodel.cpp
SOURCES *= $$PWD/ircuser.cpp
SOURCES *= $$PWD/ircusermodel.cpp
