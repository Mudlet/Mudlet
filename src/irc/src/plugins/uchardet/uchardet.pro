######################################################################
# Communi
######################################################################

TARGET = uchardetplugin
DESTDIR = ../../../plugins/communi

SOURCES += plugin.cpp
include(../../3rdparty/uchardet-0.0.1/uchardet.pri)

isEmpty(COMMUNI_INSTALL_PLUGINS) {
    contains(MEEGO_EDITION,harmattan) {
        !no_rpath:QMAKE_RPATHDIR += /opt/communi/lib
        COMMUNI_INSTALL_PLUGINS = /opt/communi/plugins
    } else {
        COMMUNI_INSTALL_PLUGINS = $$[QT_INSTALL_PLUGINS]
    }
}

target.path = $$COMMUNI_INSTALL_PLUGINS/communi
INSTALLS += target

include(../plugins.pri)
