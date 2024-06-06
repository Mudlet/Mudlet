
add_library(Qt5::QGeoPositionInfoSourceFactorySerialNmea MODULE IMPORTED)


_populate_Positioning_plugin_properties(QGeoPositionInfoSourceFactorySerialNmea RELEASE "position/libqtposition_serialnmea.so" FALSE)

list(APPEND Qt5Positioning_PLUGINS Qt5::QGeoPositionInfoSourceFactorySerialNmea)
set_property(TARGET Qt5::Positioning APPEND PROPERTY QT_ALL_PLUGINS_position Qt5::QGeoPositionInfoSourceFactorySerialNmea)
set_property(TARGET Qt5::QGeoPositionInfoSourceFactorySerialNmea PROPERTY QT_PLUGIN_TYPE "position")
set_property(TARGET Qt5::QGeoPositionInfoSourceFactorySerialNmea PROPERTY QT_PLUGIN_EXTENDS "")
