######################################################################
# Communi
######################################################################

isEmpty(IRC_MODULES):error(IRC_MODULES must be set)

isEmpty(IRC_BUILDDIR):IRC_BUILDDIR = $$OUT_PWD/../..
IRC_LIBDIR = $$IRC_BUILDDIR/lib

isEmpty(IRC_SOURCEDIR):IRC_SOURCEDIR = $$PWD/..
IRC_INCDIR = $$IRC_SOURCEDIR/include

INCLUDEPATH += $$IRC_INCDIR

# order matters for static libs
IRC_MODULES_ORDERED =
for(IRC_MODULE, $$list(IrcUtil IrcModel IrcCore)) {
   contains(IRC_MODULES, $$IRC_MODULE):IRC_MODULES_ORDERED += $$IRC_MODULE
}

for(IRC_MODULE, IRC_MODULES_ORDERED) {
    !contains(DEFINES, IRC_STATIC):macx:!qt_no_framework {
        INCLUDEPATH += $$IRC_LIBDIR/$${IRC_MODULE}.framework/Headers
        QMAKE_LFLAGS += -F$$IRC_LIBDIR # inject before system frameworks
        LIBS += -framework $$IRC_MODULE
        install_name {
            !isEmpty(QMAKE_POST_LINK):QMAKE_POST_LINK += &&
            QMAKE_POST_LINK += install_name_tool -change \
                "$$[QT_INSTALL_LIBS]/$${IRC_MODULE}.framework/Versions/3/$${IRC_MODULE}" \
                "$$IRC_LIBDIR/$${IRC_MODULE}.framework/Versions/3/$${IRC_MODULE}" $$TARGET
        }
    } else {
        QMAKE_LIBDIR += $$IRC_LIBDIR # injects before system libdirs
        REAL_TEMPLATE = $$TEMPLATE
        TEMPLATE = fakelib
        LIBS += -l$$qtLibraryTarget($$IRC_MODULE)
        TEMPLATE = $$REAL_TEMPLATE
        !no_rpath:QMAKE_RPATHDIR += $$IRC_LIBDIR
    }
    INCLUDEPATH += $$IRC_INCDIR/$$IRC_MODULE
    DEPENDPATH += $$IRC_INCDIR/$$IRC_MODULE
}
