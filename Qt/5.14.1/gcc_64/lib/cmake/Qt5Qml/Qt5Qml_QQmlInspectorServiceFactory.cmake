
add_library(Qt5::QQmlInspectorServiceFactory MODULE IMPORTED)


_populate_Qml_plugin_properties(QQmlInspectorServiceFactory RELEASE "qmltooling/libqmldbg_inspector.so" FALSE)

list(APPEND Qt5Qml_PLUGINS Qt5::QQmlInspectorServiceFactory)
set_property(TARGET Qt5::Qml APPEND PROPERTY QT_ALL_PLUGINS_qmltooling Qt5::QQmlInspectorServiceFactory)
set_property(TARGET Qt5::QQmlInspectorServiceFactory PROPERTY QT_PLUGIN_TYPE "qmltooling")
set_property(TARGET Qt5::QQmlInspectorServiceFactory PROPERTY QT_PLUGIN_EXTENDS "")
