qml_files.source = qml/meego
DEFINES += COMMUNI_PLATFORM=MeeGo
DEFINES += COMMUNI_EXAMPLE_REVISION=2
DEFINES += COMMUNI_QML_DIR=qml/meego
DEFINES += COMMUNI_IMPORT_PATH=/opt/communi/imports
DEFINES += COMMUNI_PLUGIN_PATH=/opt/communi/plugins

# Add more folders to ship with the application, here
qml_files.target = qml
qml_images.source = qml/images
qml_images.target = qml
DEPLOYMENTFOLDERS = qml_files qml_images

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

TARGET = communi

# If your application uses the Qt Mobility libraries, uncomment the following
# lines and add the respective components to the MOBILITY variable.
# CONFIG += mobility
# MOBILITY +=

# Speed up launching on MeeGo/Harmattan when using applauncherd daemon
# CONFIG += qdeclarative-boostable

INCLUDEPATH += src
DEPENDPATH += src

# Sources
HEADERS += abstractsessionitem.h
HEADERS += completer.h
HEADERS += sessionchilditem.h
HEADERS += sessionitem.h
HEADERS += sessionmanager.h
HEADERS += settings.h

SOURCES += abstractsessionitem.cpp
SOURCES += completer.cpp
SOURCES += main.cpp
SOURCES += sessionchilditem.cpp
SOURCES += sessionitem.cpp
SOURCES += sessionmanager.cpp
SOURCES += settings.cpp

# Communi
QT += network
include(../examples.pri)
include(../shared/shared.pri)
QMAKE_RPATHDIR += /opt/communi/lib

# Please do not modify the following two lines. Required for deployment.
include(qmlapplicationviewer/qmlapplicationviewer.pri)
qtcAddDeployment()

OTHER_FILES += \
    qtc_packaging/debian_harmattan/rules \
    qtc_packaging/debian_harmattan/README \
    qtc_packaging/debian_harmattan/manifest.aegis \
    qtc_packaging/debian_harmattan/copyright \
    qtc_packaging/debian_harmattan/control \
    qtc_packaging/debian_harmattan/compat \
    qtc_packaging/debian_harmattan/changelog \
    qtc_packaging/debian_harmattan/rules \
    qtc_packaging/debian_harmattan/README \
    qtc_packaging/debian_harmattan/manifest.aegis \
    qtc_packaging/debian_harmattan/copyright \
    qtc_packaging/debian_harmattan/control \
    qtc_packaging/debian_harmattan/compat \
    qtc_packaging/debian_harmattan/changelog \
    qtc_packaging/debian_harmattan/rules \
    qtc_packaging/debian_harmattan/README \
    qtc_packaging/debian_harmattan/manifest.aegis \
    qtc_packaging/debian_harmattan/copyright \
    qtc_packaging/debian_harmattan/control \
    qtc_packaging/debian_harmattan/compat \
    qtc_packaging/debian_harmattan/changelog


