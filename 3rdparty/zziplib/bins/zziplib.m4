dnl this macro has been copied from /usr/share/aclocal/pkg.m4
dnl modified to check only for zziplib. You are advised to use
dnl pgkconfig macro directly instead. It is only installed to
dnl the convenience of software makers who do not want to have
dnl yet another package dependency when building zziplib-based
dnl projects. In general, you can include in your configure.ac
dnl some line like
dnl PKG_CHECK_ZZIPLIB([ZZIP],[0.10.75])
dnl in order to get the two autoconf/automake subst variables
dnl named ZZIP_CFLAGS and ZZIP_LIBS respectivly.

dnl PKG_CHECK_ZZIPLIB(ZZIP, minversion, action-if, action-not)
dnl defines ZZIP_LIBS, ZZIP_CFLAGS, see pkg-config man page
dnl also defines ZZIP_PKG_ERRORS on error
AC_DEFUN([PKG_CHECK_ZZIPLIB], [# checking for zziplib cflags/libs $2
  succeeded=no

  if test -z "$PKG_CONFIG"; then
    AC_PATH_PROG([PKG_CONFIG],[pkg-config],[no])
  fi

  if test "$PKG_CONFIG" = "no" ; then
    dnl we stick in an extra section that takes advantage of `zzip-config`
    dnl script that might be there and carry cflags/libs info as well.
    AC_PATH_PROG([ZZIP_CONFIG], [zzip-config], [no])
    if test "$ZZIP_CONFIG" = "no" ; then
      echo "*** Neither config spec could be found - there was no archaic"
      echo "*** zzip-config script around and no pkg-config script in the"
      echo "*** PATH. Make sure that either one is in your path, or set"
      echo "*** set the PKG_CONFIG environment variable to the full"
      echo "*** path to pkg-config. See http://zziplib.sf.net - or see"
      echo "*** http://www.freedesktop.org/software/pkgconfig for more."
    else
      AC_MSG_CHECKING($1_LIBS)
      $1_LIBS=`$ZZIP_CONFIG --libs 2>/dev/null`
      AC_MSG_RESULT($[]$1_LIBS)

      if test ".$[]$1_LIBS" != "." ; then
        AC_MSG_CHECKING($1_CFLAGS)
        $1_CFLAGS=`$ZZIP_CONFIG --cflags 2>/dev/null`
        AC_MSG_RESULT($[]$1_CFLAGS)
        succeeded="yes"
      else
        AC_MSG_CHECKING($1_CFLAGS)
        $1_CFLAGS=`$ZZIP_CONFIG --cflags 2>&1`
        AC_MSG_RESULT([(detecting errors...)])
        AC_MSG_WARN([... there is a problem with zzip-config]) 
        AC_MSG_WARN([... $[]$1_CFLAGS])
        AC_MSG_WARN([... inference of library requirements from prefix])
        succeeded=`echo $ZZIP_CONFIG | sed -e 's,/[^/]*,,'` # dirname
        $1_CFLAGS=""
        $1_LIBS=""
        if test "/$succeeded" != "//usr" ; then
          test -d "$succeeded/include" && $1_CFLAGS="-I$succeeded/include "
          test -d "$succeeded/lib"     && $1_LIBS="-L$succeeded/lib "
        fi
        $1_LIBS="$[]$1_LIBS -lzzip -lz"
        succeeded="yes"
        AC_MSG_CHECKING($1_CFLAGS... guessed)
        AC_MSG_RESULT($[]$1_CFLAGS)
        AC_MSG_CHECKING($1_LIBS... guessed)
        AC_MSG_RESULT($[]$1_LIBS)
      fi
    fi
  else
     PKG_CONFIG_MIN_VERSION=0.9.0
     if $PKG_CONFIG --atleast-pkgconfig-version $PKG_CONFIG_MIN_VERSION; then
        AC_MSG_CHECKING(for zziplib[]ifelse([$2],,,[ >= $2]))

        if $PKG_CONFIG --exists "zziplib[]ifelse([$2],,,[ >= $2])" ; then
            AC_MSG_RESULT(yes)
            succeeded=yes

            AC_MSG_CHECKING($1_CFLAGS)
            $1_CFLAGS=`$PKG_CONFIG --cflags "zziplib[]ifelse([$2],,,[ >= $2])"`
            AC_MSG_RESULT($$1_CFLAGS)

            AC_MSG_CHECKING($1_LIBS)
            $1_LIBS=`$PKG_CONFIG --libs "zziplib[]ifelse([$2],,,[ >= $2])"`
            AC_MSG_RESULT($$1_LIBS)
        else
            $1_CFLAGS=""
            $1_LIBS=""
            ## If we have a custom action on failure, don't print errors, but 
            ## do set a variable so people can do so.
            $1_PKG_ERRORS=`$PKG_CONFIG dnl
        --errors-to-stdout --print-errors "zziplib[]ifelse([$2],,,[ >= $2])"`
            ifelse([$4], ,echo $$1_PKG_ERRORS,)
        fi

        AC_SUBST($1_CFLAGS)
        AC_SUBST($1_LIBS)
     else
        echo "*** Your version of pkg-config is too old. You need version $PKG_CONFIG_MIN_VERSION or newer."
        echo "*** See http://www.freedesktop.org/software/pkgconfig"
     fi
  fi

  if test $succeeded = yes; then
     ifelse([$3], , :, [$3])
  else
     succeeded="zziplib[]ifelse([$2],,,[ >= $2])"
     ifelse([$4], ,[AC_MSG_ERROR([Library requirements ($succeeded) not met; consider adjusting the PKG_CONFIG_PATH environment variable if your libraries are in a nonstandard prefix so pkg-config can find them.])], [$4])
  fi
])
