######################################################################
# Communi
######################################################################

TARGET = communiplugin
QT = core network qml
TARGETPATH = Communi
DESTDIR = ../../../qml/$$TARGETPATH

SOURCES += plugin.cpp
OTHER_FILES += qmldir plugins.qmltypes

# qml_plugin.prf: insert the plugins URI into its meta data
# to enable usage of static plugins in QtDeclarative:
QMAKE_MOC_OPTIONS += -Muri=$$TARGETPATH

isEmpty(IRC_INSTALL_QML):IRC_INSTALL_QML = $$[QT_INSTALL_QML]

!no_install_qml {
    target.path = $$IRC_INSTALL_QML/$$TARGETPATH
    INSTALLS += target

    other_files.files = $$OTHER_FILES
    other_files.path = $$IRC_INSTALL_QML/$$TARGETPATH
    INSTALLS += other_files
}

for(other_file, OTHER_FILES) {
    ARGUMENTS = $${PWD}$${QMAKE_DIR_SEP}$$other_file $$DESTDIR
    !isEmpty(QMAKE_POST_LINK):QMAKE_POST_LINK += &&
    QMAKE_POST_LINK += $$QMAKE_COPY $$replace(ARGUMENTS, /, $$QMAKE_DIR_SEP)
}

include(../imports.pri)
