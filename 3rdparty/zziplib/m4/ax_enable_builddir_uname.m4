dnl /usr/share/aclocal/guidod-cvs/ax_enable_builddir_uname.m4
dnl @synopsis AX_ENABLE_BUILDDIR_UNAME [(dirstring-or-command [,Makefile.mk])]
dnl
dnl if the current configure was run within the srcdir then we move all
dnl configure-files into a subdir and let the configure steps continue
dnl there. We provide an option --disable-builddir to suppress the move
dnl into a separate builddir.
dnl
dnl Defaults:
dnl
dnl   $1 = $build (defaults to `uname -msr`.d)
dnl   $2 = Makefile.mk
dnl   $3 = -all
dnl
dnl This macro must be called before AM_INIT_AUTOMAKE.
dnl
dnl it creates a default toplevel srcdir Makefile from the information
dnl found in the created toplevel builddir Makefile. It just copies the
dnl variables and rule-targets, each extended with a default
dnl rule-execution that recurses into the build directory of the
dnl current "BUILD". You can override the auto-dection through
dnl `uname -msr | tr " /" "__"`.d at build-time of course, as in
dnl
dnl   make BUILD=i386-mingw-cross
dnl
dnl After the default has been created, additional rules can be
dnl appended that will not just recurse into the subdirectories and
dnl only ever exist in the srcdir toplevel makefile - these parts are
dnl read from the $2 = Makefile.mk file
dnl
dnl The automatic rules are usually scanning the toplevel Makefile for
dnl lines like '#### $build |$builddir' to recognize the place where to
dnl recurse into. Usually, the last one is the only one used. However,
dnl almost all targets have an additional "*-all" rule which makes the
dnl script to recurse into _all_ variants of the current BUILD (!!)
dnl setting. The "-all" suffix can be overriden for the macro as well.
dnl
dnl a special rule is only given for things like "dist" that will copy
dnl the tarball from the builddir to the sourcedir (or $(PUB)) for
dnl reason of convenience.
dnl
dnl @category Misc
dnl @author Guido U. Draheim
dnl @version 2005-12-03
dnl @license GPLWithACException

AC_DEFUN([AX_ENABLE_BUILDDIR_UNAME],[
AC_REQUIRE([AC_CANONICAL_HOST])[]dnl
AC_REQUIRE([AX_CONFIGURE_ARGS])[]dnl
AC_BEFORE([$0],[AM_INIT_AUTOMAKE])dnl
AS_VAR_PUSHDEF([SUB],[ax_enable_builddir])dnl
AS_VAR_PUSHDEF([SED],[ax_enable_builddir_sed])dnl
SUB="."
AC_ARG_ENABLE([builddir], AC_HELP_STRING(
  [--disable-builddir],[disable automatic build in subdir of sources])
  ,[SUB="$enableval"], [SUB="yes"])
if test ".$ac_srcdir_defaulted" != ".no" ; then
if test ".$srcdir" = ".." ; then
  if test -f config.status ; then
    AC_MSG_NOTICE(toplevel srcdir already configured... skipping subdir build)
  else
    test ".$SUB" = "."  && SUB="."
    test ".$SUB" = ".no"  && SUB="."
    test ".$BUILD" = "." && BUILD=`uname -msr | tr " /" "__"`.d
    test ".$SUB" = ".yes" && SUB="m4_ifval([$1], [$1],[$BUILD])"
    if test ".$SUB" != ".." ; then    # we know where to go and
      AS_MKDIR_P([$SUB])
      echo __.$SUB.__ > $SUB/conftest.tmp
      cd $SUB
      if grep __.$SUB.__ conftest.tmp >/dev/null 2>/dev/null ; then
        rm conftest.tmp
        AC_MSG_RESULT([continue configure in default builddir "./$SUB"])
      else
        AC_MSG_ERROR([could not change to default builddir "./$SUB"])
      fi
      srcdir=`echo "$SUB" |
              sed -e 's,^\./,,;s,[[^/]]$,&/,;s,[[^/]]*/,../,g;s,[[/]]$,,;'`
      # going to restart from subdirectory location
      test -f $srcdir/config.log   && mv $srcdir/config.log   .
      test -f $srcdir/confdefs.h   && mv $srcdir/confdefs.h   .
      test -f $srcdir/conftest.log && mv $srcdir/conftest.log .
      test -f $srcdir/$cache_file  && mv $srcdir/$cache_file  .
      AC_MSG_RESULT(....exec $SHELL "$srcdir/[$]0" "--srcdir=$srcdir" "--enable-builddir=$SUB" $ac_configure_args)
      case "[$]0" in # restart
       [/\\]*) eval $SHELL "'[$]0'" "'--srcdir=$srcdir'" "'--enable-builddir=$SUB'" $ac_configure_args ;;
       *) eval $SHELL "'$srcdir/[$]0'" "'--srcdir=$srcdir'" "'--enable-builddir=$SUB'" $ac_configure_args ;;
      esac ; exit $?
    fi
  fi
fi fi
dnl ac_path_prog uses "set dummy" to override $@ which would defeat the "exec"
AC_PATH_PROG(SED,gsed sed, sed)
AS_VAR_POPDEF([SED])dnl
AS_VAR_POPDEF([SUB])dnl
AC_CONFIG_COMMANDS([buildir],[dnl .............. config.status ..............
AS_VAR_PUSHDEF([SUB],[ax_enable_builddir])dnl
AS_VAR_PUSHDEF([TOP],[top_srcdir])dnl
AS_VAR_PUSHDEF([SRC],[ac_top_srcdir])dnl
AS_VAR_PUSHDEF([SED],[ax_enable_builddir_sed])dnl
pushdef([END],[Makefile.mk])dnl
  SRC="$ax_enable_builddir_srcdir"
  if test ".$SUB" = "." ; then
    if test -f "$TOP/Makefile" ; then
      AC_MSG_NOTICE([skipping TOP/Makefile - left untouched])
    else
      AC_MSG_NOTICE([skipping TOP/Makefile - not created])
    fi
  else
    AC_MSG_NOTICE([create TOP/Makefile guessed from local Makefile])
     x='`' ; cat >$tmp/conftemp.sed <<_EOF
/^\$/n
x
/^\$/bS
x
/\\\\\$/{H;d;}
{H;s/.*//;x;}
bM
:S
x
/\\\\\$/{h;d;}
{h;s/.*//;x;}
:M
s/\\(\\n\\)	/\\1 /g
/^	/d
/^[[ 	]]*[[\\#]]/d
/^VPATH *=/d
s/^srcdir *=.*/srcdir = ./
s/^top_srcdir *=.*/top_srcdir = ./
/[[:=]]/!d
/^\\./d
s/:.*/:/
/:\$/s/ /  /g
/:\$/s/  / /g
/^.*[[=]]/!s%\$% ; (cd \$(BUILD) \\&\\& \$(ISNOTSRCDIR) \\&\\& \$(MAKE) "\$\@") || exit ; \$(MAKE) done "RULE=\$\@"%
_EOF
    test ".$USE_MAINTAINER_MODE" = ".no" || \
  	cp "$tmp/conftemp.sed" "$SRC/makefile.sed~"            ## DEBUGGING
    echo 'BUILD=`uname -msr | tr " /" "__"`.d' >$SRC/Makefile
    echo 'ISNOTSRCDIR=test ! -f configure' >>$SRC/Makefile
    $SED -f $tmp/conftemp.sed Makefile >>$SRC/Makefile
    echo 'done: ;@ if grep "$(RULE)-done .*:" Makefile > /dev/null; then dnl
    echo $(MAKE) $(RULE)-done ; $(MAKE) $(RULE)-done ; else true ; fi' dnl
    >> $SRC/Makefile
    if test -f "$SRC/m4_ifval([$2],[$2],[END])" ; then
      AC_MSG_NOTICE([extend TOP/Makefile with TOP/m4_ifval([$2],[$2],[END])])
      cat $SRC/END >>$SRC/Makefile
    fi 
    AC_MSG_NOTICE([make uses BUILD=$SUB (on $ax_enable_builddir_host:)])
  fi
popdef([END])dnl
AS_VAR_POPDEF([SED])dnl
AS_VAR_POPDEF([SRC])dnl
AS_VAR_POPDEF([TOP])dnl
AS_VAR_POPDEF([SUB])dnl
],[dnl
ax_enable_builddir_srcdir="$srcdir"                    # $srcdir
ax_enable_builddir_host="$HOST"                        # $HOST / $host
ax_enable_builddir_version="$VERSION"                  # $VERSION
ax_enable_builddir_package="$PACKAGE"                  # $PACKAGE
ax_enable_builddir_sed="$ax_enable_builddir_sed"       # $SED
ax_enable_builddir="$ax_enable_builddir"               # $SUB
])dnl
])

