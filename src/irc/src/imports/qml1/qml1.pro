######################################################################
# Communi
######################################################################

TARGET = communiplugin
QT = core network declarative
TARGETPATH = Communi
DESTDIR = ../../../imports/$$TARGETPATH

SOURCES += plugin.cpp
OTHER_FILES += qmldir plugins.qmltypes

isEmpty(IRC_INSTALL_IMPORTS):IRC_INSTALL_IMPORTS = $$[QT_INSTALL_IMPORTS]

!no_install_imports {
    target.path = $$IRC_INSTALL_IMPORTS/$$TARGETPATH
    INSTALLS += target

    other_files.files = $$OTHER_FILES
    other_files.path = $$IRC_INSTALL_IMPORTS/$$TARGETPATH
    INSTALLS += other_files
}

for(other_file, OTHER_FILES) {
    ARGUMENTS = $${PWD}$${QMAKE_DIR_SEP}$$other_file $$DESTDIR
    !isEmpty(QMAKE_POST_LINK):QMAKE_POST_LINK += &&
    QMAKE_POST_LINK += $$QMAKE_COPY $$replace(ARGUMENTS, /, $$QMAKE_DIR_SEP)
}

include(../imports.pri)
