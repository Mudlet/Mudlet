
add_library(Qt5::QJpegPlugin MODULE IMPORTED)


_populate_Gui_plugin_properties(QJpegPlugin RELEASE "imageformats/libqjpeg.so" FALSE)

list(APPEND Qt5Gui_PLUGINS Qt5::QJpegPlugin)
set_property(TARGET Qt5::Gui APPEND PROPERTY QT_ALL_PLUGINS_imageformats Qt5::QJpegPlugin)
set_property(TARGET Qt5::QJpegPlugin PROPERTY QT_PLUGIN_TYPE "imageformats")
set_property(TARGET Qt5::QJpegPlugin PROPERTY QT_PLUGIN_EXTENDS "")
