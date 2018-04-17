dnl /usr/share/aclocal/guidod-cvs/ax_not_enable_frame_pointer.m4
dnl @synopsis AX_NOT_ENABLE_FRAME_POINTER ([shellvar])
dnl
dnl add --enable-frame-pointer option, the default will add the gcc
dnl --fomit-frame-pointer option to the shellvar (per default CFLAGS)
dnl and remove the " -g " debuginfo option from it. In other words, the
dnl default is "--disable-frame-pointer"
dnl
dnl @category C
dnl @author Guido U. Draheim <guidod@gmx.de>
dnl @version 2005-01-22
dnl @license GPLWithACException

AC_DEFUN([AX_NOT_ENABLE_FRAME_POINTER],[dnl
AS_VAR_PUSHDEF([VAR],[enable_frame_pointer])dnl
AC_MSG_CHECKING([m4_ifval($1,$1,CFLAGS) frame-pointer])
AC_ARG_ENABLE([frame-pointer], AC_HELP_STRING(
  [--enable-frame-pointer],[enable callframe generation for debugging]))
case ".$VAR" in
  .|.no|.no,*) test ".$VAR" = "." && VAR="no"
     m4_ifval($1,$1,CFLAGS)=`echo dnl
  " $m4_ifval($1,$1,CFLAGS) " | sed -e 's/ -g / /'`
     if test ".$GCC" = ".yes" ; then
        m4_ifval($1,$1,CFLAGS)="$m4_ifval($1,$1,CFLAGS) -fomit-frame-pointer"
        AC_MSG_RESULT([$VAR, -fomit-frame-pointer added])
     else
        AC_MSG_RESULT([$VAR, -g removed])
     fi  ;;
   *)  AC_MSG_RESULT([$VAR, kept]) ;;
esac
AS_VAR_POPDEF([VAR])dnl
])

