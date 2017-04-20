######################################################################
# Communi
######################################################################

IRC_MODULE = IrcUtil
include(util.pri)
include(../module_build.pri)
include(../module_install.pri)

IRC_MODULES = IrcCore IrcModel
include(../module_deps.pri)
