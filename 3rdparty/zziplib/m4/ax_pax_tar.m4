dnl @synopsis AX_PAX_TAR
dnl
dnl Most people will not know about the "tar"-wars a long while back.
dnl In the end the proponents of cpio lost just as all the oldish
dnl tar formats were dumped in favor of a new format that based on
dnl the successful "ustar" format. The extensions did mostly cover
dnl a portable definition of filenames and such stuff.

dnl The most interesting thing however is that the opengroup (the.
dnl UNIX standardization body) did not document the tool for the
dnl portable tar format under the name of "tar" but instead it did
dnl use the name "pax" which stands for "portable archiver". Even
dnl more so, the "pax" utitility is required to understand the old
dnl tar and cpio formats but it will default to a "tar"-like variant.
dnl The "pax" tool has been in the Unix standard ever since UNIX92.
dnl
dnl Interestingly gnu-tar will use the pax features as soon as they
dnl are required - but you can not be sure if gnu-tar is available,
dnl and it is quite likely that the system tar will default to the
dnl old format of the system in whatever it was. On any currrent
dnl Unix system however one should be able to find "pax", so one
dnl should prefer that one as the most portable tool.

dnl The downside is however that the commandline options of "pax"
dnl are different from "tar"/"gtar". That's why we define two
dnl subst'ed names here - PAX_TAR_CREATE and PAX_TAR_EXTRACT, For
dnl extra portability the first argument should be "*.tar" file
dnl name but you should be aware that in most cases the "pax" tool
dnl is invoked. Nota bene: do not use "*.pax" as the file name
dnl extension because some file managers interpret it as a TeX file.
dnl
dnl @category C
dnl @author Guido U. Draheim <guidod@gmx.de>
dnl @version 2009-05-22
dnl @license GPLWithACException

AC_DEFUN([_AX_PAX_TAR_TOOL],[
if test -z "$ac_cv_pax_tar_tool"; then
  AC_PATH_PROG([PAX],[pax], :)
  if test "$ac_cv_path_PAX" != ":"; then
    ac_cv_pax_tar_tool="pax"
  else #3
  AC_PATH_PROG([GNUTAR],[gnutar], :)
  if test "$ac_cv_path_GNUTAR" != ":"; then
    ac_cv_pax_tar_tool="gnutar"
  else #1
  AC_PATH_PROG([GTAR],[gtar], :)
  if test "$ac_cv_path_GTAR" != ":"; then
    ac_cv_pax_tar_tool="gtar"
  else #2
  AC_PATH_PROG([TAR],[tar], :)
  if test "$ac_cv_path_TAR" != ":"; then
    ac_cv_pax_tar_tool="tar"
  fi
  fi fi fi #3 #2 #1
  AC_MSG_CHECKING([for portable tar tool])
  AC_MSG_RESULT([$ac_cv_pax_tar_tool])
fi
])

dnl TODO: on MacOsX the "--help" documentation of the tool differs
dnl from the capabilities of the installed pax tool. So test reality.
AC_DEFUN([_AX_PAX_SINGLE_ARCHIVE],[
  if test "$ac_cv_pax_tar_tool$ax_pax_single_archive" = "pax"; then
    AC_MSG_CHECKING([for pax single archive option])
       echo foo > conftest.txt ; rm -f conftest.tar
       AC_RUN_LOG(["$ac_cv_path_PAX" -w -O -f conftest.tar conftest.txt])
       if test -s conftest.tar; then
           ax_pax_single_archive="-O" ; ac_hint="(probably a BSD pax)"
       else
           ax_pax_single_archive=" "  ; ac_hint="(the -O option did not work)"
       fi
     AC_MSG_RESULT([$ax_pax_single_archive $ac_hint])
  fi
])

AC_DEFUN([_AX_PAX_TAR_CREATE],[
  _AX_PAX_TAR_TOOL
  _AX_PAX_SINGLE_ARCHIVE
  AC_MSG_CHECKING([for command to create portable tar archives])
  if test "$ac_cv_pax_tar_tool" = "gnutar"; then
    ax_pax_tar_create="'$ac_cv_path_GNUTAR' cf"
  elif test "$ac_cv_pax_tar_tool" = "gtar"; then
    ax_pax_tar_create="'$ac_cv_path_GTAR' cf"
  elif test "$ac_cv_pax_tar_tool" = "pax"; then
    ax_pax_tar_create="'$ac_cv_path_PAX' -w $ax_pax_single_archive -f"
  elif test "$ac_cv_pax_tar_tool" = "tar"; then
    ax_pax_tar_create="'$ac_cv_path_TAR' cf"
  else
    ax_pax_tar_create=": 'unknown pax tar tool $ac_cv_pax_tar_tool'"
  fi
  AC_MSG_RESULT([$ax_pax_tar_create])
])

AC_DEFUN([_AX_PAX_TAR_EXTRACT],[
  _AX_PAX_TAR_TOOL
  _AX_PAX_SINGLE_ARCHIVE
  AC_MSG_CHECKING([for command to extract portable tar archives])
  if test "$ac_cv_pax_tar_tool" = "gnutar"; then
    ax_pax_tar_extract="'$ac_cv_path_GNUTAR' xf"
  elif test "$ac_cv_pax_tar_tool" = "gtar"; then
    ax_pax_tar_extract="'$ac_cv_path_GTAR' xf"
  elif test "$ac_cv_pax_tar_tool" = "pax"; then
    ax_pax_tar_extract="'$ac_cv_path_PAX' -r $ax_pax_single_archive -f"
  elif test "$ac_cv_pax_tar_tool" = "tar"; then
    ax_pax_tar_extract="'$ac_cv_path_TAR' xf"
  else
    ax_pax_tar_extract=": 'unknown pax tar tool $ac_cv_pax_tar_tool'"
  fi
  AC_MSG_RESULT([$ax_pax_tar_extract])
])

AC_DEFUN([AX_PAX_TAR_CREATE],[dnl
_AX_PAX_TAR_CREATE
m4_ifval([$1],[dnl
$1="$ax_pax_tar_create"
AC_SUBST([$1])
],[dnl
PAX_TAR_CREATE="$ax_pax_tar_create"
AC_SUBST([PAX_TAR_CREATE])
])
])

AC_DEFUN([AX_PAX_TAR_EXTRACT],[dnl
_AX_PAX_TAR_EXTRACT
m4_ifval([$1],[dnl
$1="$ax_pax_tar_extract"
AC_SUBST([$1])
],[dnl
PAX_TAR_EXTRACT="$ax_pax_tar_extract"
AC_SUBST([PAX_TAR_EXTRACT])
])
])

