dnl @synopsis AC_SYS_LARGEFILE_SENSITIVE
dnl
dnl checker whether the current system is sensitive to -Ddefines making
dnl off_t having different types/sizes. Automatically define a config.h
dnl symbol LARGEFILE_SENSITIVE if that is the case, otherwise leave
dnl everything as is.
dnl
dnl This macro builds on top of AC_SYS_LARGEFILE to detect whether
dnl special options are neede to make the code use 64bit off_t - in
dnl many setups this will also make the code use 64bit off_t
dnl immediatly.
dnl
dnl The common use of a LARGEFILE_SENSITIVE config.h-define is to
dnl rename exported functions, usually adding a 64 to the original
dnl function name. Such renamings are only needed on systems being both
dnl (a) 32bit off_t by default and (b) implementing large.file
dnl extensions (as for unix98).
dnl
dnl a renaming section could look like this:
dnl
dnl  #if defined LARGEFILE_SENSITIVE && _FILE_OFFSET_BITS+0 == 64
dnl  #define zzip_open zzip_open64
dnl  #define zzip_seek zzip_seek64
dnl  #endif
dnl
dnl for libraries, it is best to take advantage of the prefix-config.h
dnl macro, otherwise you want to export a renamed LARGEFILE_SENSITIVE
dnl in an installed header file. -> see AX_PREFIX_CONFIG_H
dnl
dnl @category Misc
dnl @author Guido U. Draheim <guidod@gmx.de>
dnl @version 2009-07-27
dnl @license GPLWithACException

AC_DEFUN([AC_SYS_LARGEFILE_SENSITIVE],[dnl
AC_REQUIRE([AC_SYS_LARGEFILE])dnl
# we know about some internals of ac_sys_largefile here...
AC_MSG_CHECKING(whether system differentiates 64bit off_t by defines)
ac_cv_sys_largefile_sensitive="no"
if test ".${ac_cv_sys_file_offset_bits-no}${ac_cv_sys_large_files-no}" != ".nono"
then ac_cv_sys_largefile_sensitive="yes"
  AC_DEFINE(LARGEFILE_SENSITIVE, 1,
  [whether the system defaults to 32bit off_t but can do 64bit when requested])
fi
AC_MSG_RESULT([$ac_cv_sys_largefile_sensitive])
])
