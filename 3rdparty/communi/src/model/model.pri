######################################################################
# Communi
######################################################################

DEFINES += BUILD_IRC_MODEL

INCDIR = $$PWD/../../include/IrcModel

DEPENDPATH += $$PWD $$INCDIR
INCLUDEPATH += $$PWD $$INCDIR

CONV_HEADERS1 += $$INCDIR/IrcBuffer
CONV_HEADERS1 += $$INCDIR/IrcBufferModel
CONV_HEADERS1 += $$INCDIR/IrcChannel
CONV_HEADERS1 += $$INCDIR/IrcModel
CONV_HEADERS1 += $$INCDIR/IrcUser
CONV_HEADERS1 += $$INCDIR/IrcUserModel

PUB_HEADERS1 += $$INCDIR/ircbuffer.h
PUB_HEADERS1 += $$INCDIR/ircbuffermodel.h
PUB_HEADERS1 += $$INCDIR/ircchannel.h
PUB_HEADERS1 += $$INCDIR/ircmodel.h
PUB_HEADERS1 += $$INCDIR/ircuser.h
PUB_HEADERS1 += $$INCDIR/ircusermodel.h

PRIV_HEADERS1 += $$INCDIR/ircbuffer_p.h
PRIV_HEADERS1 += $$INCDIR/ircbuffermodel_p.h
PRIV_HEADERS1 += $$INCDIR/ircchannel_p.h
PRIV_HEADERS1 += $$INCDIR/ircuser_p.h
PRIV_HEADERS1 += $$INCDIR/ircusermodel_p.h

HEADERS += $$PUB_HEADERS1
HEADERS += $$PRIV_HEADERS1

SOURCES += $$PWD/ircbuffer.cpp
SOURCES += $$PWD/ircbuffermodel.cpp
SOURCES += $$PWD/ircchannel.cpp
SOURCES += $$PWD/ircmodel.cpp
SOURCES += $$PWD/ircuser.cpp
SOURCES += $$PWD/ircusermodel.cpp
