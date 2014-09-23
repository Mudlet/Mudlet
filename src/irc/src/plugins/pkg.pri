######################################################################
# Communi
######################################################################

defineTest(pkgExists) {
    isEmpty(PKG_CONFIG):PKG_CONFIG = pkg-config
    unix:system($$PKG_CONFIG --exists $$1):return(true)
    return(false)
}
