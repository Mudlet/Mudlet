
add_library(Qt5::QICOPlugin MODULE IMPORTED)


_populate_Gui_plugin_properties(QICOPlugin RELEASE "imageformats/libqico.so" FALSE)

list(APPEND Qt5Gui_PLUGINS Qt5::QICOPlugin)
set_property(TARGET Qt5::Gui APPEND PROPERTY QT_ALL_PLUGINS_imageformats Qt5::QICOPlugin)
set_property(TARGET Qt5::QICOPlugin PROPERTY QT_PLUGIN_TYPE "imageformats")
set_property(TARGET Qt5::QICOPlugin PROPERTY QT_PLUGIN_EXTENDS "")
