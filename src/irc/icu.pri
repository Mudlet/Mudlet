######################################################################
# LibIrcClient-Qt
######################################################################

defineReplace(icuLibTarget) {
    unset(LIBRARY_NAME)
    LIBRARY_NAME = $$1
    CONFIG(debug, debug|release) {
        !debug_and_release|build_pass {
            mac:RET = $$member(LIBRARY_NAME, 0)_debug
            else:win32:RET = $$member(LIBRARY_NAME, 0)d
        }
    }
    isEmpty(RET):RET = $$LIBRARY_NAME
    return($$RET)
}

DEFINES += HAVE_ICU
contains(DEFINES, HAVE_ICU) {
    win32:INCLUDEPATH += C:/ICU/include
    macx:INCLUDEPATH += /opt/local/include
    win32:LIBS += -LC:/ICU/lib
    macx:LIBS += -L/opt/local/lib
    win32:LIBS += -l$$icuLibTarget(icuin)
    unix:LIBS += -licui18n -licudata
    LIBS += -l$$icuLibTarget(icuuc)
}
