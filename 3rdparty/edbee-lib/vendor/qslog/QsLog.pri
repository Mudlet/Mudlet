INCLUDEPATH += $$PWD

# Enable / disable line numbers!
DEFINES += QS_LOG_LINE_NUMBERS_SHORT

SOURCES += $$PWD/QsLogDest.cpp \
    $$PWD/QsLog.cpp \
    $$PWD/QsDebugOutput.cpp

HEADERS += $$PWD/QSLogDest.h \
    $$PWD/QsLog.h \
    $$PWD/QsDebugOutput.h \
    $$PWD/QsLogLevel.h
