AC_INIT([zziplib.spec])
AC_PREREQ(2.61)
AC_COPYRIGHT([Guido Draheim <guido@gmx.de> for ZZipLib.SF.net])
AC_REVISION($Revision: 1.15 $)
AC_CONFIG_AUX_DIR([uses])
AC_CONFIG_MACRO_DIR([m4])
# =======================================================================
AC_CANONICAL_SYSTEM
dnl AM_ENABLE_MULTILIB([Makefile],[$host])
AX_ENABLE_BUILDDIR_UNAME
AC_SET_DEFAULT_PATHS_SYSTEM
# -----------------------------------------------------------------------
AC_ARG_ENABLE([thread-safe],
AS_HELP_STRING([--disable-thread-safe],[disable thread-safe libtool option])
   ,,[ enable_thread_safe="yes"])
THREAD_SAFE=""
test "$enable_thread_safe" = "yes" && THREAD_SAFE="-thread-safe"
test "$enable_thread_safe" = "$host_os" && THREAD_SAFE="-thread-safe"
AC_SUBST(THREAD_SAFE)
if test -n "$THREAD_SAFE" ; then
  AC_MSG_RESULT(...using thread-safe linkage)
else
  AC_MSG_RESULT(disabled thread-safe linkage)
fi
# ----------------------------------------------------------------
AC_MSG_CHECKING(zlib install prefix)
AC_ARG_WITH(zlib, 
AS_HELP_STRING([--with-zlib],[zlib prefix (e.g. '$HOME/software')])
    ,,[with_zlib="no"])
if test "${with_zlib}" = yes ; then # choose --prefix paths
   if test "${prefix}" = NONE ; then
      AC_MSG_RESULT([oops, but only itself...])
      AC_MSG_ERROR(--with-zlib requires a path argument or a --prefix path set)
   fi
   with_zlib="$prefix"
fi

case ".$with_zlib" in
   .no|.no,*)
      AC_MSG_RESULT(none)
      with_zlib="no, not configured" ;;
  ./usr)
      AC_MSG_RESULT([oops, ignored (may try --with-zlib=/usr/.)])
      with_zlib="no, /usr ignored" ;;
  *)  if test ! -d "$with_zlib/include" ; then
         AC_MSG_RESULT([oops, given but...])
         AC_MSG_ERROR("$with_zlib/include" is not a directory)
      fi
      if test ! -d "$with_zlib/lib" ; then
         AC_MSG_RESULT([oops, given but...])
         AC_MSG_ERROR("$with_zlib/lib" is not a directory)
      fi
      AC_MSG_RESULT($with_zlib)
      ZLIB_INCL="-I$with_zlib/include"
      ZLIB_LDIR="-L$with_zlib/lib"
      AC_MSG_RESULT(..adding CPPFLAGS $ZLIB_INCL)
      AC_MSG_RESULT(..adding LDFLAGS  $ZLIB_LDIR)
      CPPFLAGS="$CPPFLAGS $ZLIB_INCL"
      LDFLAGS="$LDFLAGS $ZLIB_LDIR"
  ;;
esac
AC_SUBST(ZLIB_INCL)
AC_SUBST(ZLIB_LDIR)
# -----------------------------------------------------------------------
AX_SPEC_DEFAULTS
AM_INIT_AUTOMAKE($PACKAGE,$VERSION)
dnl test ".$CFLAGS" = "." && CFLAGS=""
AC_PROG_CC
dnl test ".$CFLAGS" = "." && test "$GCC" = "yes" && CFLAGS="-O3"
AC_PROG_INSTALL
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL
AX_SET_VERSION_INFO

# -----------------------------------------------------------------------
AC_MSG_RESULT([creating zzip/_msvc.in via zzip/_msvc.sed of config.h.in])
test -d zzip || mkdir zzip
sed -f $srcdir/zzip/_msvc.sed $srcdir/config.h.in >zzip/_msvc.in
AC_CONFIG_HEADERS([config.h])
AX_PREFIX_CONFIG_H([zzip/_config.h],[zzip],[config.h])
AX_PREFIX_CONFIG_H([zzip/_msvc.h],[zzip],[zzip/_msvc.in])
AM_MAINTAINER_MODE

AX_CREATE_PKGCONFIG_INFO(dnl
[zzip/zziplib.pc], [zzip-zlib-config],[-lzzip],,[ ])
# -----------------------------------------------------------------------
AX_PAX_TAR_CREATE
AX_PAX_TAR_EXTRACT
AC_PATH_PROGS(PERL, perl5 perl, echo no perl found for)
AC_PATH_PROGS(PYTHON, python, echo no python found for)
AC_PATH_PROGS(MKZIP, zip pkzip, :)
AC_PATH_PROGS(XMLTO, xmlto, :)

AC_C_INLINE
AC_C_CONST
ACX_C_RESTRICT
AC_STDC_HEADERS
AC_HEADER_DIRENT
AC_CHECK_HEADERS(stdint.h unistd.h windows.h winnt.h winbase.h sys/int_types.h)
AC_CHECK_HEADERS(sys/types.h sys/mman.h sys/stat.h sys/param.h) dnl posix'ish
AC_CHECK_HEADERS(io.h direct.h zlib.h byteswap.h)
AC_CHECK_HEADERS(fnmatch.h)
AC_CHECK_FUNCS( strcasecmp strndup )

AC_TYPE_OFF_T
AC_TYPE_SIZE_T
dnl AC_TYPE_SSIZE_T
AC_CHECK_TYPE(ssize_t,int)
AC_CHECK_TYPE(off64_t,_zzip_off_t)
AC_CHECK_TYPE(__int64,long long)
AC_COMPILE_CHECK_SIZEOF(short)
AC_COMPILE_CHECK_SIZEOF(int)
AC_COMPILE_CHECK_SIZEOF(long)
AC_C_BIGENDIAN
AX_CHECK_ALIGNED_ACCESS_REQUIRED
dnl -------------------------------------------------------------
# the default is _disabled_!! anyway, result is prefixed in zzip/_config.h!!
AC_SYS_LARGEFILE_SENSITIVE
test ".$LARGEFILE_CFLAGS" = "." && LARGEFILE_CFLAGS=""
AC_ARG_WITH(largefile,AC_HELP_STRING(
    [--with-largefile],[compile as off64_t instead of 32bit off_t])
    ,,[with_largefile="no"])
if test ".$ac_cv_sys_largefile_sensitive" = ".no" ; then
  AC_MSG_RESULT(compiles library with the only off_t seen)
elif test ".$with_largefile" != ".no" ; then
  AC_MSG_RESULT(compiles library as 64bit off_t variant dnl
- and renaming some function names)
  LARGEFILE_CFLAGS="$LARGEFILE_CFLAGS -D_LARGEFILE_SOURCE"
  AC_MSG_RESULT(..adding CFLAGS $LARGEFILE_CFLAGS)
  CFLAGS="$CFLAGS $LARGEFILE_CFLAGS"
  AC_MSG_RESULT(..adding 64 into RELEASE_INFO for the libraries)
  RELEASE_INFO="$RELEASE_INFO-64"
else
  AC_MSG_RESULT(compiles library as 32bit off_t variant dnl
- largefile is not the default here)
fi
AC_SUBST(LARGEFILE_CFLAGS)
dnl -------------------------------------------------------------
AC_MSG_CHECKING(for zlib being found)
if test "_$ac_cv_header_zlib_h" != "_yes" ; then
  AC_MSG_RESULT(dnl
...oops: zlib.h not found - try adding some CPPFLAGS to configure call)
         AC_MSG_RESULT(dnl
...note: and when linkage fails you might also need to set some LDFLAGS)
  case "$prefix" in
    NONE|/usr)
            AC_MSG_RESULT(dnl
...note: both can be set by specifying the install prefix of the zlib library)
            AC_MSG_RESULT(dnl
...note: via --with-zlib=path ... it will test for -Ipath/include -Lpath/lib)
    ;;
    *) if test -d $prefix/include ; then
         AC_MSG_RESULT(dnl
...note: -I$prefix/include is not added to CPPFLAGS automatically and)
         AC_MSG_RESULT(dnl
...note: neither is -L$prefix/lib for LDFLAGS .. both will be if you specify)
         AC_MSG_RESULT(dnl
...note: --with-zlib to configure call, or use --with-zlib=installprefix)
       fi
     ;;
  esac
  AC_MSG_ERROR(zlib.h not found: "zlib" is required to build "$PACKAGE")
  exit 1
fi
cat >conftest.c <<EOF
#include <zlib.h>
const char* zlib_VeRsIoN = ZLIB_VERSION "dEtEcT";
EOF
ZLIB_VERSION=`(eval "$ac_cpp conftest.c") 2>/dev/null \
| grep zlib_VeRsIoN | sed -e 's,.*= *.,,' -e 's,. *.dEtEcT.*,,'`
rm conftest.c
AC_SUBST(ZLIB_VERSION)
AC_MSG_RESULT(yes - $ZLIB_VERSION)
# ----------------------------------------------------------------
AC_MSG_CHECKING(docdir default)
AC_ARG_WITH(docdir, AC_HELP_STRING(
    [--with-docdir],[docdir prefix (prefix/share/doc)])
    ,,[with_docdir="no"])
case "${with_docdir}" in
 prefix/*)  docdir=` echo "$withval" | sed 's/^prefix/${prefix}'` ;;
 datadir/*) docdir=` echo "$withval" | sed 's/^datadir/${datadir}'` ;;
 yes) docdir='${prefix}/doc' ;;
 no)  docdir='${datadir}/doc' ;;
 /*|\\*|[a-zA-Z]:*) docdir="$with_docdir"  ;;
 *) AC_MSG_RESULT(oops)
    AC_MSG_ERROR(bad value ${withval} for --with-docdir)
 ;;
esac
AC_SUBST(docdir)
AC_MSG_RESULT($docdir)
# ----------------------------------------------------------------
AC_MSG_CHECKING(whether using debugging messages in libraries)
AC_ARG_WITH(debug, AC_HELP_STRING(
    [--with-debug],[debug messages in library])
    , [case "${withval}" in
		yes|no) with_debug="$withval" ;;
		*) AC_MSG_RESULT(oops)
                   AC_MSG_ERROR(bad value ${withval} for --with-debug) ;;
       esac], [with_debug="no"])
test "$with_debug" != "yes" && with_debug="no" # yes or no
test "$with_debug" = "yes" && export CFLAGS="$CFLAGS -DDEBUG"
result="$with_debug" ; test "$result" = "yes" && result="yes, added -DDEBUG"
AC_MSG_RESULT($result)
if test "$with_debug" = "yes" ; then if test -n "$RELEASE_INFO" ; then
   RELEASE_INFO="$RELEASE_INFO-dbg"
   AC_MSG_RESULT(modified release, $RELEASE_INFO)
fi fi
# ----------------------------------------------------------------
AC_MSG_CHECKING(whether thread stack is too small for bufsiz buffers)
 # many embedded platforms, superthreaded systems and kernel usage
 # will severely limit the stackspace for its threads. Use _LOWSTK
 # compile-define on those platforms.
AC_ARG_WITH(lowstk, AC_HELP_STRING(
    [--with-lowstk], [limited thread stack])
    , [case "${withval}" in
		yes|no|auto) with_lowstk="$withval" ;;
		*) AC_MSG_ERROR(bad value ${withval} for --with-lowstk) ;;
       esac], [with_lowstk="auto"])
if test "$with_lowstk" = "auto"; then
  case $target in
	*-palmos-*) $with_lowstk="yes" ;;
  esac
fi
test "$with_lowstk" != "yes" && with_lowstk="no" # yes or no
test "$with_lowstk" = "yes" && CFLAGS="$CFLAGS -D_LOWSTK"
result="$with_lowstk" ; test "$result" = "yes" && result="yes, added -D_LOWSTK"
AC_MSG_RESULT($with_lowstk)
# ---------------------------------------------------------------
AC_MSG_CHECKING(whether can use mmap for file scanning)
 # system with mmap are probably better performing an searching
 # and parsing - atleast memory consumption is far better
AC_ARG_ENABLE(mmap, AC_HELP_STRING(
   [--disable-mmap],[disable mmap usage])
   , [case "${enableval}" in
		yes|no|auto) enable_mmap="${enableval}" ;;
		*) AC_MSG_RESULT(oops)
                   AC_MSG_ERROR(bad value ${enableval} for --disable-mmap) ;;
      esac], [enable_mmap="auto"])
if test "$enable_mmap" = "auto"
then
    test "$ac_cv_header_winbase_h"  && enable_mmap="yes, winbase.h"
    test "$ac_cv_header_sys_mman_h" && enable_mmap="yes, sys/mman.h"
fi
case "$enable_mmap" in
   yes|yes,*) CFLAGS="$CFLAGS -D_USE_MMAP"
      AC_MSG_RESULT($enable_mmap, added -D_USE_MMAP) ;;
   *) enable_mmap="no"
      AC_MSG_RESULT(no (just a bit slower)) ;;
esac
# ---------------------------------------------------------------
AC_MSG_CHECKING(whether making SDL parts)
SDL=""
SDL_GENERATE=""
AC_ARG_ENABLE(sdl, AC_HELP_STRING(
    [--enable-sdl],[enable SDL usage (if present)])
    ,,[enable_sdl="no"])
if test "$enable_sdl" = "no" ; then
   AC_MSG_RESULT([no, disabled])
else
   enable_sdl=`pkg-config --modversion sdl 2>/dev/null`
   if test -z "$enable_sdl"; then
      enable_sdl=`sdl-config --version 2>/dev/null`
      SDL_GENERATE="sdl-uninstalled.pc"
      enable_sdl="$enable_sdl (sdl-config)"
   else
      enable_sdl="$enable_sdl (pkgconfig)"
   fi
   enable_sdl="$enable_sdl."
   case ".$enable_sdl" in
     .1.*) AC_MSG_RESULT([yes, using SDL $enable_sdl])
           SDL="SDL" ;;
     .2.*) AC_MSG_RESULT([yes, untested $enable_sdl])
           SDL="SDL" ;;
     *)    AC_MSG_RESULT([no, $enable_sdl]) ;;
   esac
   if test -n "$SDL_GENERATE"; then
      AC_MSG_RESULT([enabled SDL generation of... $SDL_GENERATE])
   fi
fi
AC_SUBST([SDL])
AC_SUBST([SDL_GENERATE])
# ---------------------------------------------------------------
AC_MSG_CHECKING(running extra tests on make check)
if test "MKZIP" != ":" ; then
  AC_MSG_RESULT(yes)
  ZIPTESTS=""
else
  AC_MSG_RESULT(no)
  ZIPTESTS="#"
fi
AC_SUBST(ZIPTESTS)
# ---------------------------------------------------------------
AC_MSG_CHECKING([link options])
ZZIPLIB_LDFLAGS=""
case "$host_os" in mingw*)
    # we may assume that the backend is gnu-ld but DLLs need full resolution
    ZZIPLIB_LDFLAGS="${wl}-no-undefined ${wl}-export-all-symbols -mconsole"
    RESOLVES=' '
;; darwin*)
    # darwin never uses gnu-ld but the linker can do it anyway
    ZZIPLIB_LDFLAGS="-export-dynamic"
    RESOLVES=" # "
;; *)
    if test ".$can_build_shared" != ".no" ; then
      ZZIPLIB_LDFLAGS="-export-dynamic"
      if test ".$lt_cv_prog_gnu_ld" == ".yes" ; then
         ZZIPLIB_LDFLAGS="${wl}--export-dynamic"
         # TODO: that is for backward compatibility only
      fi
    fi
    RESOLVES=" # "
;; esac
AC_MSG_RESULT([$ZZIPLIB_LDFLAGS $RESOLVES])
AC_SUBST([ZZIPLIB_LDFLAGS])
AC_SUBST([RESOLVES])
AC_ARG_WITH([defines], 
AS_HELP_STRING([--with-defines],[easy,harden,allow_modulo_entries]),
    ,[with_defines="normal"])

#
case ",$with_defines," in *,easy,*)
  AC_MSG_RESULT([..adding CFLAGS -DZZIP_EASY])
  CFLAGS="$CFLAGS -DZZIP_EASY"
;; esac
case ",$with_defines," in *,harden,*)
  AC_MSG_RESULT([..adding CFLAGS -DZZIP_HARDEN])
  CFLAGS="$CFLAGS -DZZIP_HARDEN"
;; esac
case ",$with_defines," in *,allow_modulo_entries,*)
  AC_MSG_RESULT([..adding CFLAGS -DZZIP_ALLOW_MODULO_ENTRIES])
  CFLAGS="$CFLAGS -DZZIP_ALLOW_MODULO_ENTRIES"
;; esac

# ---------------------------------------------------------------
AX_MAINTAINER_MODE_AUTO_SILENT
AX_NOT_ENABLE_FRAME_POINTER
AX_CFLAGS_WARN_ALL
AX_CFLAGS_GCC_OPTION(-Wpointer-arith)
AX_CFLAGS_GCC_OPTION(-Wsign-compare)
AX_CFLAGS_GCC_OPTION(-Wmissing-declarations)
AX_CFLAGS_GCC_OPTION(-Wdeclaration-after-statement)
AX_CFLAGS_GCC_OPTION(-Werror-implicit-function-declaration)
AX_CFLAGS_GCC_OPTION(-Wstrict-aliasing)
AX_CFLAGS_GCC_OPTION(-Warray-bounds)
AX_CFLAGS_NO_WRITABLE_STRINGS
AX_CFLAGS_STRICT_PROTOTYPES
# ---------------------------------------------------------------
AX_ENABLE_DEFAULT_PKGCONFIGDIR
AX_ENABLE_DEFAULT_ACLOCALDIR
AC_SUBST([CONFIG_FILES])
AC_OUTPUT([Makefile
           zzip/Makefile zzipwrap/Makefile SDL/Makefile
           docs/Makefile test/Makefile bins/Makefile])
dnl -------------------------------------------------------------
PATCH_LIBTOOL_ON_DARWIN_ZSH_OVERQUOTING
PATCH_LIBTOOL_SYS_LIB_SEARCH_PATH_SPEC
PATCH_LIBTOOL_TO_ADD_HOST_CC
# ---------------------------------------------------------------
echo '-----' $PACKAGE $VERSION $prefix
$SHELL libtool --features || true
echo "CPPFLAGS=$CPPFLAGS"
echo "CFLAGS=$CFLAGS"
test "$enable_mmap" != "no" && echo "enable  mmap   ($enable_mmap)"
test "$enable_mmap"  = "no" && echo "disable mmap   ($enable_mmap)"
test "$with_lowstk" != "no" && echo "with    lowstk ($with_lowstk)"
test "$with_lowstk"  = "no" && echo "without lowstk ($with_lowstk)"
test "$with_debug"  != "no" && echo "with    debug  ($with_debug)"
test "$with_debug"   = "no" && echo "without debug  ($with_debug)"
# test -f ./zziplib.spec && echo warning - do not build in the source dirs
echo '# make  && make check  && make install'
