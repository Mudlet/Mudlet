######################################################################
# Communi: gui.pri
######################################################################

DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD
QMAKE_CLEAN += $$PWD/*~

FORMS += $$PWD/messageview.ui

HEADERS += $$PWD/addviewdialog.h
HEADERS += $$PWD/lineeditor.h
HEADERS += $$PWD/messageview.h
HEADERS += $$PWD/multisessiontabwidget.h
HEADERS += $$PWD/searcheditor.h
HEADERS += $$PWD/sessiontabwidget.h
HEADERS += $$PWD/sessiontreedelegate.h
HEADERS += $$PWD/sessiontreeitem.h
HEADERS += $$PWD/sessiontreewidget.h
HEADERS += $$PWD/settings.h
HEADERS += $$PWD/tabwidget.h
HEADERS += $$PWD/tabwidget_p.h
HEADERS += $$PWD/userlistview.h

SOURCES += $$PWD/addviewdialog.cpp
SOURCES += $$PWD/lineeditor.cpp
SOURCES += $$PWD/messageview.cpp
SOURCES += $$PWD/multisessiontabwidget.cpp
SOURCES += $$PWD/searcheditor.cpp
SOURCES += $$PWD/sessiontabwidget.cpp
SOURCES += $$PWD/sessiontreedelegate.cpp
SOURCES += $$PWD/sessiontreeitem.cpp
SOURCES += $$PWD/sessiontreewidget.cpp
SOURCES += $$PWD/settings.cpp
SOURCES += $$PWD/tabwidget.cpp
SOURCES += $$PWD/userlistview.cpp

include(util/util.pri)
include(3rdparty/3rdparty.pri)
