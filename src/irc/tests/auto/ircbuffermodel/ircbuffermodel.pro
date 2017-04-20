######################################################################
# Communi
######################################################################

SOURCES += tst_ircbuffermodel.cpp

# FakeQmlBufferModel::createXxx()
*g++*|*clang*:QMAKE_CXXFLAGS_WARN_ON += -Wno-overloaded-virtual

include(../shared/shared.pri)
include(../auto.pri)
