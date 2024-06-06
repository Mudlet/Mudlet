
add_library(Qt5::QLinuxFbIntegrationPlugin MODULE IMPORTED)


_populate_Gui_plugin_properties(QLinuxFbIntegrationPlugin RELEASE "platforms/libqlinuxfb.so" FALSE)

list(APPEND Qt5Gui_PLUGINS Qt5::QLinuxFbIntegrationPlugin)
set_property(TARGET Qt5::Gui APPEND PROPERTY QT_ALL_PLUGINS_platforms Qt5::QLinuxFbIntegrationPlugin)
set_property(TARGET Qt5::QLinuxFbIntegrationPlugin PROPERTY QT_PLUGIN_TYPE "platforms")
set_property(TARGET Qt5::QLinuxFbIntegrationPlugin PROPERTY QT_PLUGIN_EXTENDS "-")
