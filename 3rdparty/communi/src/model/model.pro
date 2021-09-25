######################################################################
# Communi
######################################################################

IRC_MODULE = IrcModel
include(model.pri)
include(../module_build.pri)
include(../module_install.pri)

IRC_MODULES = IrcCore
include(../module_deps.pri)
