######################################################################
# LibIrcClient-Qt
######################################################################

TEMPLATE = lib
TARGET = $$qtLibraryTarget(ircclient-qt)
DEFINES += BUILD_IRC
QT = core network
CONFIG += silent
DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII
!win32:VERSION = 0.5.0

DESTDIR = lib
DEPENDPATH += include src
INCLUDEPATH += include
CONFIG(debug, debug|release) {
    OBJECTS_DIR = debug
    MOC_DIR = debug
} else {
    OBJECTS_DIR = release
    MOC_DIR = release
}

static {
    DEFINES += IRC_STATIC
    system(echo DEFINES+=IRC_STATIC > libircclient-qt-config.prf)
} else {
    DEFINES += IRC_SHARED
    system(echo DEFINES+=IRC_SHARED > libircclient-qt-config.prf)
}

macx {
    # Libircclient-qt is installed using the same format as Qt itself. This
    # makes it easier for other applications to link against it, as Qmake
    # assumes that.

    !qt_no_framework {
        CONFIG += lib_bundle
        FRAMEWORK_HEADERS.version = Versions
        FRAMEWORK_HEADERS.files = $$HEADERS
        FRAMEWORK_HEADERS.path = Headers
        QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS

        QMAKE_POST_LINK  = install_name_tool -id
        QMAKE_POST_LINK +=   "$$[QT_INSTALL_LIBS]/ircclient-qt.framework/Versions/0/$${TARGET}"
        QMAKE_POST_LINK +=   "$${DESTDIR}/ircclient-qt.framework/Versions/0/$${TARGET}"
    } else {
        CONFIG -= lib_bundle
        QMAKE_POST_LINK  =  install_name_tool -id
        QMAKE_POST_LINK +=   "$$[QT_INSTALL_LIBS]/lib$${TARGET}.0.dylib"
        QMAKE_POST_LINK +=   "$${DESTDIR}/lib$${TARGET}.0.dylib"
    }
}

CONV_HEADERS += include/Irc include/IrcBuffer include/IrcGlobal include/IrcSession include/IrcUtil
PUB_HEADERS += irc.h ircbuffer.h ircglobal.h ircsession.h ircutil.h
PRIV_HEADERS += ircbuffer_p.h ircsession_p.h
HEADERS = $$PUB_HEADERS $$PRIV_HEADERS
SOURCES += irc.cpp ircbuffer.cpp ircsession.cpp ircutil.cpp

mkspecs.files = libircclient-qt.prf libircclient-qt-config.prf
mkspecs.path = $$[QT_INSTALL_DATA]/mkspecs/features
INSTALLS += mkspecs

headers.files = $$PUB_HEADERS $$CONV_HEADERS
headers.path = $$[QT_INSTALL_HEADERS]/ircclient-qt
INSTALLS += headers

target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target

dlltarget.path = $$[QT_INSTALL_BINS]
INSTALLS += dlltarget

!build_pass {
    macx {
        # See above for an explanation of this.
        !qt_no_framework {
            message(Building LibIrcClient-Qt $$VERSION (framework))
        } else {
            message(Building LibIrcClient-Qt $$VERSION (dylib))
        }
    } else {
        message(Building LibIrcClient-Qt $$VERSION)
    }

    !no_icu {
        message(ICU support enabled. Run \'qmake -config no_icu\' to disable ICU support.)
        include(icu.pri)
    } else {
        message(ICU support disabled.)
    }

    !no_deprecated {
        message(Deprecated functionality enabled. Run \'qmake -config no_deprecated\' to disable deprecated functionality.)
    } else {
        message(Deprecated functionality disabled.)
        DEFINES += IRC_NO_DEPRECATED
    }
}
