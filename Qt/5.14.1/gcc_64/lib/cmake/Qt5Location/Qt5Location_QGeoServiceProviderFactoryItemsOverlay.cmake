
add_library(Qt5::QGeoServiceProviderFactoryItemsOverlay MODULE IMPORTED)


_populate_Location_plugin_properties(QGeoServiceProviderFactoryItemsOverlay RELEASE "geoservices/libqtgeoservices_itemsoverlay.so" FALSE)

list(APPEND Qt5Location_PLUGINS Qt5::QGeoServiceProviderFactoryItemsOverlay)
set_property(TARGET Qt5::Location APPEND PROPERTY QT_ALL_PLUGINS_geoservices Qt5::QGeoServiceProviderFactoryItemsOverlay)
set_property(TARGET Qt5::QGeoServiceProviderFactoryItemsOverlay PROPERTY QT_PLUGIN_TYPE "geoservices")
set_property(TARGET Qt5::QGeoServiceProviderFactoryItemsOverlay PROPERTY QT_PLUGIN_EXTENDS "")
