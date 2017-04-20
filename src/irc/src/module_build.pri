######################################################################
# Communi
######################################################################

isEmpty(IRC_MODULE):error(IRC_MODULE must be set)

TEMPLATE = lib
TARGET = $$qtLibraryTarget($$IRC_MODULE)
QT = core network
!verbose:CONFIG += silent
contains(QT_CONFIG, debug_and_release) {
    win32|mac:!wince*:!win32-msvc:!macx-xcode:CONFIG += debug_and_release build_all
}

include(../version.pri)
!win32:VERSION = $$IRC_VERSION

isEmpty(IRC_BUILDDIR):IRC_BUILDDIR = $$OUT_PWD/../..

DESTDIR = $$IRC_BUILDDIR/lib
DLLDESTDIR = $$IRC_BUILDDIR/bin

!flat {
    CONFIG(debug, debug|release) {
        OBJECTS_DIR = debug
        MOC_DIR = debug
    } else {
        OBJECTS_DIR = release
        MOC_DIR = release
    }
}

DISTFILES += $$CONV_HEADERS

coverage {
    QMAKE_CLEAN += $$OBJECTS_DIR/*.gcda $$OBJECTS_DIR/*.gcno

    LIBS += -lgcov
    QMAKE_CXXFLAGS += -g -Wall -fprofile-arcs -ftest-coverage -O0
    QMAKE_LDFLAGS += -g -Wall -fprofile-arcs -ftest-coverage  -O0

    zerocounters.commands = @lcov --directory \$(OBJECTS_DIR) --zerocounters
    QMAKE_EXTRA_TARGETS += zerocounters

    capture.file = ../../coverage/$${IRC_MODULE}.cov
    capture.commands = @mkdir -p ../../coverage
    capture.commands += && lcov --base-directory $$_PRO_FILE_PWD_ --directory \$(OBJECTS_DIR) --capture --output-file $$capture.file
    capture.filters = \"/usr/*\" \"moc_*.cpp\" \"*3rdparty/*\" \"*QtCore/*\" \"*QtNetwork/*\" \"*corelib/*\" \"*network/*\"
    !isEqual(IRC_MODULE, "IrcCore"):capture.filters += \"*/IrcCore/*\"
    !isEqual(IRC_MODULE, "IrcModel"):capture.filters += \"*/IrcModel/*\"
    capture.commands += && lcov --remove $$capture.file $$capture.filters --output-file $$capture.file
    QMAKE_EXTRA_TARGETS += capture

    genhtml.dir = ../../coverage/$${IRC_MODULE}
    genhtml.commands = @genhtml --output-directory $$genhtml.dir $$capture.file
    QMAKE_EXTRA_TARGETS += genhtml
}
