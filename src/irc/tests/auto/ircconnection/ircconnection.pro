######################################################################
# Communi
######################################################################

SOURCES += tst_ircconnection.cpp

# FakeQmlConnection::createCtcpReply()
*g++*|*clang*:QMAKE_CXXFLAGS_WARN_ON += -Wno-overloaded-virtual

include(../shared/shared.pri)
include(../auto.pri)
