dnl /usr/share/aclocal/guidod-cvs/ac_set_default_paths_system.m4
dnl @synopsis AC_SET_DEFAULT_PATHS_SYSTEM
dnl
dnl the most interesting changes go about windows-targets - where the
dnl default_prefix is set to /programs, and quite some directories are
dnl aliased: sbindir := libdir := bindir and the docprefix-defaults are
dnl also a bit different, even on FHS2-compliant systems where the
dnl mandir is going to $prefix/man only if prefix=/usr, otherwise they
dnl shall go to $datadir/man. We use an extra docprefix to express it
dnl which is either defined as being prefix or datadir. not SUBSTed
dnl here.
dnl
dnl @category Misc
dnl @author Guido U. Draheim <guidod@gmx.de>
dnl @version 2001-08-25
dnl @license GPLWithACException

AC_DEFUN([AC_SET_DEFAULT_PATHS_SYSTEM],
[AC_REQUIRE([AC_CANONICAL_HOST]) # --------------------------------------------
case "$prefix:$ac_default_prefix" in
  NONE:/usr/local)
    result=""
    AC_MSG_CHECKING(default prefix path)
    case "${target_os}" in
      *cygwin* | *mingw* | *uwin* | *djgpp | *emx* )
	if test "${host_os}" = "${target_os}" ; then
           ac_default_prefix="/programs"
           result="(win/dos target)"
        else
           case "$PATH" in
              *:/usr/local/cross-tools/$target_alias/bin:*)
	          ac_default_prefix="/usr/local/cross-tools/$target_alias" ;;
              *:/usr/local/$target_alias/bin:*)
	          ac_default_prefix="/usr/local/$target_alias" ;;
              *:/usr/local/$target_cpu-$target_os/bin:*)
	          ac_default_prefix="/usr/local/$target_cpu-$target_os" ;;
              *)
                  ac_default_prefix="/programs" ;;
           esac
           result="(win/dos cross-compiler)"
        fi
    ;;
    esac
    AC_MSG_RESULT($ac_default_prefix $result)
  ;;
esac
AC_MSG_CHECKING(default prefix system)
result="$prefix" ; test "$result" = "NONE" && result="$ac_default_prefix"
case ${result} in
  /programs | /programs/*) result="is win-/programs"
     # on win/dos, .exe .dll and .cfg live in the same directory
     libdir=`echo $libdir |sed -e 's:^..exec_prefix./lib$:${bindir}:'`
     sbindir=`echo $sbindir |sed -e 's:^..exec_prefix./sbin$:${libdir}:'`
     sysconfdir=`echo $sysconfdir |sed -e 's:^..prefix./etc$:${sbindir}:'`
     libexecdir=`echo $libexecdir |sed -e 's:/libexec$:/system:'`
     # help-files shall be set with --infodir, docprefix is datadir
     docprefix="${datadir}"
     mandir=`echo $mandir \
	                     |sed -e 's:^..prefix./man$:${datadir}/info:'`
     includedir=`echo $includedir \
                |sed -e 's:^..prefix./include$:${datadir}/include:'`
     # other state files (but /etc) are moved to datadir
     sharedstatedir=`echo $sharedstatedir \
                     |sed -e 's:^..prefix./com$:${datadir}/default:'`
     localstatedir=`echo $localstatedir \
                     |sed -e 's:^..prefix./var$:${datadir}/current:'`
  ;;
  /usr) result="is /usr-shipped"
     # doc files are left at prefix
     docprefix="${prefix}"
     # state files go under /top
     sysconfdir=`echo $sysconfdir |sed -e 's:^..prefix./etc$:/etc:'`
     sharedstatedir=`echo $sharedstatedir \
                     |sed -e 's:^..prefix./com$:/etc/default:'`
     # $prefix/var is going to end up in /var/lib
     localstatedir=`echo $localstatedir \
                     |sed -e 's:^..prefix./var$:/var/lib:'`
  ;;
  /opt | /opt/*) result="is /opt-package"
     # state files go under /top/prefix
     sysconfdir=`echo $sysconfdir \
                     |sed -e 's:^..prefix./etc$:/etc${prefix}:'`
     sharedstatedir=`echo $sharedstatedir \
                     |sed -e 's:^..prefix./com$:/etc/default${prefix}:'`
     # $prefix/var is going to to be /var$prefix... once again
     localstatedir=`echo $localstatedir \
                     |sed -e 's:^..prefix./var$:/var${prefix}:'`
     # doc files are left at prefix
     docprefix="${prefix}"
  ;;
  *) result="is /local-package"
     # doc files are moved from prefix down to datadir
     docprefix="${datadir}"
     mandir=`echo $mandir \
                     |sed -e 's:^..prefix./man$:${datadir}/man:'`
     infodir=`echo $infodir \
                     |sed -e 's:^..prefix./infodir$:${datadir}/info:'`
     # never use $prefix/com - that is no good idea
     sharedstatedir=`echo $sharedstatedir \
                     |sed -e 's:^..prefix./com$:${sysconfdir}/default:'`
  ;;
esac
AC_MSG_RESULT($result)
# --------------------------------------------------------
])

