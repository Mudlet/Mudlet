######################################################################
# Communi
######################################################################

!verbose:CONFIG += silent

CONFIG(debug, debug|release) {
    OBJECTS_DIR = debug
    MOC_DIR = debug
    RCC_DIR = debug
    UI_DIR = debug
} else {
    OBJECTS_DIR = release
    MOC_DIR = release
    RCC_DIR = release
    UI_DIR = release
}

developer {
    include(../src/src.pri)
} else {
    IRC_MODULES = IrcCore IrcModel IrcUtil
    include(../src/module_deps.pri)
}
