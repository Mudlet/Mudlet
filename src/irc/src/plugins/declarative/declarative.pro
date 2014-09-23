######################################################################
# Communi
######################################################################

TARGET = communiplugin
QT += declarative
TARGETPATH = Communi
DESTDIR = ../../../imports/$$TARGETPATH

SOURCES += plugin.cpp
OTHER_FILES += qmldir

isEmpty(COMMUNI_INSTALL_IMPORTS) {
    contains(MEEGO_EDITION,harmattan) {
        !no_rpath:QMAKE_RPATHDIR += /opt/communi/lib
        COMMUNI_INSTALL_IMPORTS = /opt/communi/imports
    } else {
        COMMUNI_INSTALL_IMPORTS = $$[QT_INSTALL_IMPORTS]
    }
}

target.path = $$COMMUNI_INSTALL_IMPORTS/$$TARGETPATH
INSTALLS += target

other_files.files = $$OTHER_FILES
other_files.path = $$COMMUNI_INSTALL_IMPORTS/$$TARGETPATH
INSTALLS += other_files

for(other_file, OTHER_FILES) {
    ARGUMENTS = $${PWD}$${QMAKE_DIR_SEP}$$other_file $$DESTDIR
    !isEmpty(QMAKE_POST_LINK):QMAKE_POST_LINK += &&
    QMAKE_POST_LINK += $$QMAKE_COPY $$replace(ARGUMENTS, /, $$QMAKE_DIR_SEP)
}

include(../plugins.pri)
