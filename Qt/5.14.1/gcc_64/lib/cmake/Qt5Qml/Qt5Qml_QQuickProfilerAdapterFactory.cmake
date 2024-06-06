
add_library(Qt5::QQuickProfilerAdapterFactory MODULE IMPORTED)


_populate_Qml_plugin_properties(QQuickProfilerAdapterFactory RELEASE "qmltooling/libqmldbg_quickprofiler.so" FALSE)

list(APPEND Qt5Qml_PLUGINS Qt5::QQuickProfilerAdapterFactory)
set_property(TARGET Qt5::Qml APPEND PROPERTY QT_ALL_PLUGINS_qmltooling Qt5::QQuickProfilerAdapterFactory)
set_property(TARGET Qt5::QQuickProfilerAdapterFactory PROPERTY QT_PLUGIN_TYPE "qmltooling")
set_property(TARGET Qt5::QQuickProfilerAdapterFactory PROPERTY QT_PLUGIN_EXTENDS "")
