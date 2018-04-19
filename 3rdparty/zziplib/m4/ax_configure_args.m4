dnl /usr/share/aclocal/guidod-cvs/ax_configure_args.m4
dnl @synopsis AX_CONFIGURE_ARGS
dnl
dnl Helper macro for AX_ENABLE_BUILDDIR and AX_ENABLE_BUILDDIR_UNAME
dnl
dnl The traditional way of starting a subdir-configure is running the
dnl script with ${1+"$@"} but since autoconf 2.60 this is broken.
dnl Instead we have to rely on eval'ing $ac_configure_args however
dnl some old autoconf versions do not provide that. To ensure maximum
dnl portability of autoconf extension macros this helper can be 
dnl AC_REQUIRE'd so that $ac_configure_args will alsways be present.
dnl
dnl Sadly, the traditional "exec $SHELL" of the enable_builddir macros
dnl is spoiled now and must be replaced by eval + exit $?
dnl
dnl example:
dnl    AC_DEFUN([AX_ENABLE_SUBDIR],[dnl
dnl      AC_REQUIRE([AX_CONFIGURE_ARGS])dnl
dnl      eval $SHELL $ac_configure_args || exit $?
dnl      ...])
dnl
dnl @category Misc
dnl @author Guido Draheim
dnl @version 2007-02-01
dnl @license GPLWithACException

AC_DEFUN([AX_CONFIGURE_ARGS],[
   # [$]@ is unsable in 2.60+ but earlier autoconf had no ac_configure_args
   if test "${ac_configure_args+set}" != "set" ; then
      ac_configure_args=
      for ac_arg in ${1+"[$]@"}; do
         ac_configure_args="$ac_configure_args '$ac_arg'"
      done
   fi         
])

