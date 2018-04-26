dnl /usr/share/aclocal/guidod-cvs/ax_expand_prefix.m4
dnl @synopsis AX_EXPAND_PREFIX
dnl
dnl when $prefix and $exec_prefix are still set to NONE then set them
dnl to the usual default values - being based on $ac_default_prefix. -
dnl this macro can be AC_REQUIREd by other macros that need to compute
dnl values for installation directories. It has been observed that it
dnl was done wrong over and over again, so this is a bit more safe to
dnl do.
dnl
dnl remember - setting exec_prefix='${prefix}' needs you interpolate
dnl directories multiple times, it is not sufficient to just say
dnl MYVAR="${datadir}/putter" but you do have to run `eval` a few
dnl times, sth. like MYVAR=`eval "echo \"$MYVAR\""` done atleast two
dnl times.
dnl
dnl The implementation of this macro simply picks up the lines that
dnl would be run at the start of AC_OUTPUT anyway to set the
dnl prefix/exec_prefix defaults. Between AC_INIT and the first command
dnl to AC_REQUIRE this macro you can set the two variables to something
dnl explicit instead. Probably, any command to compute installation
dnl directories should be run _after_ AM_INIT_AUTOMAKE
dnl
dnl @category Misc
dnl @author Guido U. Draheim <guidod@gmx.de>
dnl @version 2005-01-21
dnl @license GPLWithACException

AC_DEFUN([AX_EXPAND_PREFIX],[dnl
  # The prefix default can be set in configure.ac (otherwise it is /usr/local)
  test "x$prefix" = xNONE && prefix=$ac_default_prefix
  # Let make expand exec_prefix. Allows to override the makevar 'prefix' later
  test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
])

