EXTRA_INCLUDEPATH += /home/qt/openssl-1.1.1d/include
EXTRA_LIBDIR += /home/qt/openssl-1.1.1d/lib
!host_build|!cross_compile {
    QMAKE_LFLAGS_APP+=-s
}
QT_CPU_FEATURES.x86_64 = mmx sse sse2
QT.global_private.enabled_features = sse2 alloca_h alloca avx2 dbus dbus-linked dlopen gui network posix_fallocate reduce_exports reduce_relocations relocatable sql system-zlib testlib widgets xml
QT.global_private.disabled_features = alloca_malloc_h android-style-assets private_tests gc_binaries libudev release_tools stack-protector-strong zstd
PKG_CONFIG_EXECUTABLE = /usr/bin/pkg-config
QMAKE_LIBS_DBUS = -ldbus-1
QMAKE_INCDIR_DBUS = /usr/include/dbus-1.0 /usr/lib64/dbus-1.0/include
QMAKE_LIBS_LIBDL = -ldl
QT_COORD_TYPE = double
QMAKE_LIBS_ZLIB = -lz
CONFIG += sse2 aesni sse3 ssse3 sse4_1 sse4_2 avx avx2 avx512f avx512bw avx512cd avx512dq avx512er avx512ifma avx512pf avx512vbmi avx512vl compile_examples enable_new_dtags f16c force_debug_info largefile precompile_header rdrnd shani x86SimdAlways
QT_BUILD_PARTS += libs tools
QT_HOST_CFLAGS_DBUS += -I/usr/include/dbus-1.0 -I/usr/lib64/dbus-1.0/include
EXTRA_RPATHS += .
