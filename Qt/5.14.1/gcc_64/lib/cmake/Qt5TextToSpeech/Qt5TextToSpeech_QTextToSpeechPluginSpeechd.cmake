
add_library(Qt5::QTextToSpeechPluginSpeechd MODULE IMPORTED)


_populate_TextToSpeech_plugin_properties(QTextToSpeechPluginSpeechd RELEASE "texttospeech/libqtexttospeech_speechd.so" FALSE)

list(APPEND Qt5TextToSpeech_PLUGINS Qt5::QTextToSpeechPluginSpeechd)
set_property(TARGET Qt5::TextToSpeech APPEND PROPERTY QT_ALL_PLUGINS_texttospeech Qt5::QTextToSpeechPluginSpeechd)
set_property(TARGET Qt5::QTextToSpeechPluginSpeechd PROPERTY QT_PLUGIN_TYPE "texttospeech")
set_property(TARGET Qt5::QTextToSpeechPluginSpeechd PROPERTY QT_PLUGIN_EXTENDS "")
