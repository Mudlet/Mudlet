######################################################################
# Communi
######################################################################

OTHER_FILES += $$PWD/bot.dox
OTHER_FILES += $$PWD/client.dox
OTHER_FILES += $$PWD/communi.png
OTHER_FILES += $$PWD/debugging.dox
OTHER_FILES += $$PWD/enums.dox
OTHER_FILES += $$PWD/ircv3.dox
OTHER_FILES += $$PWD/mainpage.dox
OTHER_FILES += $$PWD/minimal.dox
OTHER_FILES += $$PWD/modules.dox
OTHER_FILES += $$PWD/qml.dox
OTHER_FILES += $$PWD/qmlbot.dox
OTHER_FILES += $$PWD/usage.dox

isEqual(_PRO_FILE_PWD_, $$OUT_PWD) {
    docs.commands += QT_INSTALL_DOCS=$$[QT_INSTALL_DOCS] doxygen
    QMAKE_EXTRA_TARGETS += docs
}
