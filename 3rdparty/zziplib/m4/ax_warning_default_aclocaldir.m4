dnl /usr/share/aclocal/guidod-cvs/ax_warning_default_aclocaldir.m4
dnl @synopsis AX_WARNING_DEFAULT_ACLOCALDIR [(dirvariable [,[defsetting][,[A][,[N/A]]]])]
dnl @synopsis AX_ENABLE_DEFAULT_ACLOCALDIR [(dirvariable [,defsetting])]
dnl
dnl print a warning message if the $(datadir)/aclocal directory is not
dnl in the dirlist searched by the aclocal tool. This macro is useful
dnl if some `make install` would target $(datadir)/aclocal to install
dnl an autoconf m4 file of your project to be picked up by other
dnl projects.
dnl
dnl  default $1 dirvariable = aclocaldir
dnl  default $2 defsetting  = ${datadir}/aclocal
dnl  default $3 action = nothing to do
dnl  default $4 action = warn the user about mismatch
dnl
dnl In the _WARNING_ variant, the defsetting is not placed in
dnl dirvariable nor is it ac_subst'ed in any way. The default
dnl fail-action $4 is to send a warning message to the user, and the
dnl default accept-action $3 is nothing. It is expected that a Makefile
dnl is generated with aclocaldir=${datadir}/aclocal
dnl
dnl The _ENABLE_ variant however will set not only the $aclocaldir
dnl shell var of the script, but it is also AC-SUBST'ed on default -
dnl and furthermore a configure option "--enable-default-aclocaldir" is
dnl provided. Only if that option is set then $2 default is not set to
dnl the canonic default in the a $prefix subpath but instead $2 default
dnl is set to the primary path where `aclocal` looks for macros. The
dnl user may also override the default on the command line.
dnl
dnl @category Misc
dnl @category Automake
dnl @author Guido U. Draheim <guidod@gmx.de>
dnl @version 2005-01-21
dnl @license GPLWithACException

AC_DEFUN([AX_WARNING_DEFAULT_ACLOCALDIR],[dnl
AC_REQUIRE([AX_EXPAND_PREFIX])dnl
AS_VAR_PUSHDEF([DIR],[ax_warning_default_aclocal_dir])dnl
AS_VAR_PUSHDEF([BIN],[ax_warning_default_aclocal_bin])dnl
AS_VAR_PUSHDEF([LOC],[ax_warning_default_aclocal_loc])dnl
LOC='m4_if([$2],,[${datadir}/aclocal],[$2])'
m4_ifval([$1],[test ".$[]$1" != "." && LOC="$[]$1"])
 if test ".$ACLOCAL" = "." ; then
    AC_PATH_PROG([ACLOCAL],[aclocal],[:])
 fi
 BIN="$ACLOCAL"
 test ".$BIN" = "." && BIN="aclocal"
 DIR=`test ".$SHELL" = "." && SHELL="'sh'" ; eval "$BIN --print-ac-dir"`
 test ".$DIR" = "." && test -d "/usr/share/aclocal" && DIR="/usr/share/aclocal"
 test ".$DIR" = "." && DIR="/tmp"
DIR=`eval "echo $DIR"`  # we need to expand
DIR=`eval "echo $DIR"`
LOC=`eval "echo $LOC"`
LOC=`eval "echo $LOC"`
LOC=`eval "echo $LOC"`
LOC=`eval "echo $LOC"`
AC_RUN_LOG([: test "$LOC" = "$DIR"])
if test "$LOC" != "$DIR" ; then
   if test -f "$DIR/dirlist" ; then
      for DIR in `cat $DIR/dirlist` $DIR ; do
          AC_RUN_LOG([: test "$LOC" = "$DIR"])
          test "$LOC" = "$DIR" && break
      done
   fi
   if test "$LOC" != "$DIR" ; then
      m4_ifval([$4],[$4],[dnl
      AC_MSG_NOTICE([warning: m4_if([$1],,[aclocaldir],[$1])=$LOC dnl
(see config.log)])
   AC_MSG_NOTICE([perhaps: make install m4_if([$1],,[aclocaldir],[$1])=$DIR])
   cat m4_ifset([AS_MESSAGE_LOG_FD],[>&AS_MESSAGE_LOG_FD],[>>config.log]) <<EOF
  aclocaldir:   the m4_if([$1],,[default aclocaldir],[$1 value]) of $LOC
  aclocaldir:   is not listed in the dirlist where aclocal will look
  aclocaldir:   for macros - you can override the install-path using
  aclocaldir:   make install aclocaldir=$DIR
  aclocaldir:   or append the directory to aclocal reconfigures later as
  aclocaldir:   aclocal -I $LOC
  aclocaldir:   when an autoconf macro is needed from that directory
EOF
   m4_ifvaln([$5],[$5])])dnl
   m4_ifvaln([$3],[else $3])dnl
   fi
fi
AS_VAR_POPDEF([LOC])dnl
AS_VAR_POPDEF([BIN])dnl
AS_VAR_POPDEF([DIR])dnl
])

AC_DEFUN([AX_ENABLE_DEFAULT_ACLOCALDIR],[dnl
AS_VAR_PUSHDEF([BIN],[ax_warning_default_aclocal_bin])dnl
AS_VAR_PUSHDEF([DIR],[ax_warning_default_aclocal_def])dnl
AS_VAR_PUSHDEF([DEF],[ax_warning_default_aclocal_def])dnl
AC_ARG_ENABLE([enable-default-aclocaldir],
[  --enable-default-aclocaldir(=PATH)   override the datadir/aclocal default])
test ".$enable_default_aclocaldir" = "." && enable_default_aclocaldir="no"
case ".$enable_default_aclocaldir" in
  .no) DIR='m4_if([$2],,[${datadir}/aclocal],[$2])' ;;
  .yes) # autodetect
 if test ".$ACLOCAL" = "." ; then
    AC_PATH_PROG([ACLOCAL],[aclocal],[:])
 fi
 BIN="$ACLOCAL"
 test ".$BIN" = "." && BIN="aclocal"
 DIR=`test ".$SHELL" = "." && SHELL="'sh'" ; eval "$BIN --print-ac-dir"`
 test ".$DIR" = "." && test -d "/usr/share/aclocal" && DIR="/usr/share/aclocal"
 test ".$DIR" = "." && DIR="/tmp" ;;
  *) DIR="$enable_default_aclocaldir" ;;
esac
AX_WARNING_DEFAULT_ACLOCALDIR([$1],[$DEF],[$3],[$4],[$5])
m4_if([$1],,[aclocaldir],[$1])="$ax_warning_default_aclocal_dir"
AC_SUBST(m4_if([$1],,[aclocaldir],[$1]))
AS_VAR_POPDEF([DEF])dnl
AS_VAR_POPDEF([DIR])dnl
AS_VAR_POPDEF([BIN])dnl
])

