dnl /usr/share/aclocal/guidod-cvs/ax_warning_default_pkgconfig.m4
dnl @synopsis AX_WARNING_DEFAULT_PKGCONFIGDIR [(dirvariable [,[defsetting][,[A][,[N/A]]]])]
dnl @synopsis AX_ENABLE_DEFAULT_PKGCONFIGDIR [(dirvariable [,defsetting])]
dnl
dnl print a warning message if the $(datadir)/aclocal directory is not
dnl in the dirlist searched by the aclocal tool. This macro is useful
dnl if some `make install` would target $(datadir)/aclocal to install
dnl an autoconf m4 file of your project to be picked up by other
dnl projects.
dnl
dnl  default $1 dirvariable = pkgconfigdir
dnl  default $2 defsetting  = ${libdir}/pkgconfig
dnl  default $3 action = nothing to do
dnl  default $4 action = warn the user about mismatch
dnl
dnl In the _WARNING_ variant, the defsetting is not placed in
dnl dirvariable nor is it ac_subst'ed in any way. The default
dnl fail-action $4 is to send a warning message to the user, and the
dnl default accept-action $3 is nothing. It is expected that a Makefile
dnl is generated with pkgconfigdir=${libdir}/pkgconfig
dnl
dnl The _ENABLE_ variant however will set not only the $pkgconfigdir
dnl shell var of the script, but it is also AC-SUBST'ed on default -
dnl and furthermore a configure option "--enable-default-pkgconfigdir"
dnl is provided. Only if that option is set then $2 default is not set
dnl to the canonic default in the a $prefix subpath but instead $2
dnl default is set to the primary path where `pkg-config` looks for .pc
dnl files. The user may also override the default on the command line.
dnl
dnl @category Misc
dnl @author Guido U. Draheim <guidod@gmx.de>
dnl @version 2005-01-21
dnl @license GPLWithACException

AC_DEFUN([AX_WARNING_DEFAULT_PKGCONFIGDIR],[dnl
AC_REQUIRE([AX_EXPAND_PREFIX])dnl
AS_VAR_PUSHDEF([DIR],[ax_warning_default_pkgconfig_dir])dnl
AS_VAR_PUSHDEF([BIN],[ax_warning_default_pkgconfig_bin])dnl
AS_VAR_PUSHDEF([LOC],[ax_warning_default_pkgconfig_loc])dnl
LOC='m4_if([$2],,[${libdir}/pkgconfig],[$2])'
m4_ifval([$1],[test ".$[]$1" != "." && LOC="$[]$1"])
 if test ".$PKG_CONFIG" = "." ; then # we use the same default as in pkg.m4
    AC_PATH_PROG([PKG_CONFIG],[pkg-config],[no])
 fi
 if test "$PKG_CONFIG" = "no"
 then DIR="/" ; test -d "/usr/lib/pkgconfig" && DIR="/usr/lib/pkgconfig"
 else BIN=`AS_DIRNAME(["$DIR"])` ;
      if test -d "$BIN/lib/pkgconfig" ; then
          DIR="$BIN/lib/pkgconfig"
      else BIN=`AS_DIRNAME(["$DIR"])`
      if test -d "$BIN/lib/pkgconfig" ; then
          DIR="$BIN/lib/pkgconfig"
      else
      if test -d "/usr/lib/pkgconfig" ; then
          DIR="/usr/lib/pkgconfig"
      else
          DIR="/"
      fi fi fi
 fi
AC_RUN_LOG([: last pkgconfig dir is assumed as "$DIR"])
DIR=`eval "echo $DIR"`
DIR=`eval "echo $DIR"`
LOC=`eval "echo $LOC"`
LOC=`eval "echo $LOC"`
LOC=`eval "echo $LOC"`
LOC=`eval "echo $LOC"`
for DIR in `echo "$PKG_CONFIG_PATH:$DIR" | sed -e 's,:, ,g'` ; do
    AC_RUN_LOG([: test ".$LOC" = ".$DIR"])
    test ".$LOC" = ".$DIR" && break
done
if  test "$LOC" != "$DIR" ; then
        m4_ifval([$4],[$4],[dnl
 AC_MSG_NOTICE([warning: m4_if([$1],,[pkgconfigdir],[$1])=$LOC dnl
(see config.log)])
   AC_MSG_NOTICE([perhaps: make install m4_if([$1],,[pkgconfigdir],[$1])=$DIR])
   cat m4_ifset([AS_MESSAGE_LOG_FD],[>&AS_MESSAGE_LOG_FD],[>>config.log]) <<EOF
 pkgconfigdir:  the m4_if([$1],,[default pkgconfigdir],[$1 value]) of $LOC
 pkgconfigdir:  is not listed in the dirlist where pkg-config will look for
 pkgconfigdir:  package-configs - you can override the install-path using
 pkgconfigdir:  make install m4_if([$1],,[pkgconfigdir],[$1])=$DIR
 pkgconfigdir:  or set/append the directory to the environment variable
 pkgconfigdir:  PKG_CONFIG_PATH="$LOC"
EOF
   m4_ifvaln([$5],[$5])])dnl
   m4_ifvaln([$3],[else $3])dnl
fi
AS_VAR_POPDEF([LOC])dnl
AS_VAR_POPDEF([BIN])dnl
AS_VAR_POPDEF([DIR])dnl
])

AC_DEFUN([AX_ENABLE_DEFAULT_PKGCONFIGDIR],[dnl
AS_VAR_PUSHDEF([BIN],[ax_warning_default_pkgconfig_bin])dnl
AS_VAR_PUSHDEF([DIR],[ax_warning_default_pkgconfig_def])dnl
AS_VAR_PUSHDEF([DEF],[ax_warning_default_pkgconfig_def])dnl
AC_ARG_ENABLE([enable-default-pkgconfigdir],
[  --enable-default-pkgconfigdir(=PATH) override the libdir/pkgconfig default])
test ".$enable_default_pkgconfigdir" = "." && enable_default_pkgconfigdir="no"
case ".$enable_default_pkgconfigdir" in
  .no) DIR='m4_if([$2],,[${libdir}/pkgconfig],[$2])' ;;
  .yes) # autodetect
 if test ".$PKG_CONFIG" = "." ; then # we use the same default as in pkg.m4
    AC_PATH_PROG([PKG_CONFIG],[pkg-config],[no])
 fi
 if test "$PKG_CONFIG" = "no"
 then DIR="/tmp" ; test -d "/usr/lib/pkgconfig" && DIR="/usr/lib/pkgconfig"
 else BIN=`AS_DIRNAME(["$DIR"])` ;
      if test -d "$BIN/lib/pkgconfig" ; then
          DIR="$BIN/lib/pkgconfig"
      else BIN=`AS_DIRNAME(["$DIR"])`
      if test -d "$BIN/lib/pkgconfig" ; then
          DIR="$BIN/lib/pkgconfig"
      else
      if test -d "/usr/lib/pkgconfig" ; then
          DIR="/usr/lib/pkgconfig"
      else
          DIR="/tmp"
      fi fi fi
 fi ;;
  *) DIR="$enable_default_pkgconfigdir" ;;
esac
AX_WARNING_DEFAULT_PKGCONFIGDIR([$1],[$DEF],[$3],[$4],[$5])
m4_if([$1],,[pkgconfigdir],[$1])="$ax_warning_default_pkgconfig_dir"
AC_SUBST(m4_if([$1],,[pkgconfigdir],[$1]))
AS_VAR_POPDEF([DEF])dnl
AS_VAR_POPDEF([DIR])dnl
AS_VAR_POPDEF([BIN])dnl
])

