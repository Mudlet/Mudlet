######################################################################
# Communi
######################################################################

TEMPLATE = app
TARGET = quick
DEPENDPATH += .
INCLUDEPATH += .
QT += network quick

lessThan(QT_MAJOR_VERSION, 5)|lessThan(QT_MINOR_VERSION, 2) {
    error(The Qt Quick 2 based example requires Qt 5.2 or newer but Qt $$[QT_VERSION] was detected.)
}

# Input
SOURCES += main.cpp
RESOURCES += quick.qrc
OTHER_FILES += qml/main.qml
OTHER_FILES += qml/BufferListView.qml
OTHER_FILES += qml/ChatPage.qml
OTHER_FILES += qml/ConnectPage.qml
OTHER_FILES += qml/MessageFormatter.qml
OTHER_FILES += qml/TextBrowser.qml
OTHER_FILES += qml/TextEntry.qml
OTHER_FILES += qml/TopicLabel.qml
OTHER_FILES += qml/UserListView.qml

include(../examples.pri)
