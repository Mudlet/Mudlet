dnl /usr/share/aclocal/guidod-cvs/ax_maintainer_mode_auto_silent.m4
dnl @synopsis AX_MAINTAINER_MODE_AUTO_SILENT
dnl
dnl Set autotools to error/sleep settings so that they are not run when
dnl being errornously triggered. Likewise make libtool-silent when
dnl libtool has been used.
dnl
dnl I am using the macro quite a lot since some automake versions had
dnl the tendency to try to rerun some autotools on a mere make even
dnl when not quite in --maintainer-mode. That is very annoying.
dnl Likewise, a user who installs from source does not want to see
dnl doubled compiler messages.
dnl
dnl I did not put an AC-REQUIRE(MAINTAINER_MODE) in here - should I?
dnl
dnl @category Misc
dnl @category Automake
dnl @author Guido U. Draheim <guidod@gmx.de>
dnl @version 2005-01-21
dnl @license GPLWithACException

AC_DEFUN([AX_MAINTAINER_MODE_AUTO_SILENT],[dnl
dnl ac_REQUIRE([am_MAINTAINER_MODE])dn
AC_MSG_CHECKING(auto silent in maintainer mode)
if test "$USE_MAINTAINER_MODE" = "no" ; then
   test ".$TIMEOUT" = "." && TIMEOUT="9"
   AUTOHEADER="sleep $TIMEOUT ; true || autoheader || skipped"
   AUTOMAKE="sleep $TIMEOUT ; true || automake || skipped"
   AUTOCONF="sleep $TIMEOUT ; true || autoconf || skipped"
   if test ".$LIBTOOL" != "." ; then
      LIBTOOL="$LIBTOOL --silent"
      AC_MSG_RESULT([libtool-silent, auto-sleep-9])
   else
      AC_MSG_RESULT([auto-sleep-9])
   fi
else
      AC_MSG_RESULT([no])
fi
])

