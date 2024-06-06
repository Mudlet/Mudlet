
add_library(Qt5::QGeoPositionInfoSourceFactoryGeoclue2 MODULE IMPORTED)


_populate_Positioning_plugin_properties(QGeoPositionInfoSourceFactoryGeoclue2 RELEASE "position/libqtposition_geoclue2.so" FALSE)

list(APPEND Qt5Positioning_PLUGINS Qt5::QGeoPositionInfoSourceFactoryGeoclue2)
set_property(TARGET Qt5::Positioning APPEND PROPERTY QT_ALL_PLUGINS_position Qt5::QGeoPositionInfoSourceFactoryGeoclue2)
set_property(TARGET Qt5::QGeoPositionInfoSourceFactoryGeoclue2 PROPERTY QT_PLUGIN_TYPE "position")
set_property(TARGET Qt5::QGeoPositionInfoSourceFactoryGeoclue2 PROPERTY QT_PLUGIN_EXTENDS "")
