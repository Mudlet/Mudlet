dnl /usr/share/aclocal/guidod-cvs/ax_spec_file.m4
dnl @synopsis AX_SPEC_FILE [(rpmspecfile [,subdirpath])]
dnl
dnl set the specfile - if no argument has been given then check whether
dnl the ac_unique_file looks like a .spec file and use it. This macro
dnl is ac_REQUIRED by many AX_SPEC_ routines.
dnl
dnl the AX_SPEC_EXTRACT macro is a helper used by many AX_SPEC_
dnl routines and therefore placed here to be automatically included in
dnl the macro set of (older) aclocal/acinclude.
dnl
dnl AX_SPEC_EXTRACT(shellvar [,specvar [,defaultvalue]])
dnl
dnl @category Misc
dnl @author Guido U. Draheim <guidod@gmx.de>
dnl @version 2005-01-21
dnl @license GPLWithACException

AC_DEFUN([AX_SPEC_FILE],
[ AC_MSG_CHECKING([rpm spec file])
  if test ".$1" != "." ; then
    ax_spec_file_="$1"
    ax_spec_file=`basename $1`
  else
    case ".$ac_unique_file" in
      *.spec) ax_spec_file="$ac_unique_file"
              ax_spec_file_="$ac_unique_file" ;;
      *) ax_spec_file="TODO"
         if test ".$PACKAGE" != "."
         then ax_spec_file_="$PACKAGE.spec"
         else ax_spec_file_="README"
         fi
   cat m4_ifset([AS_MESSAGE_LOG_FD],[>&AS_MESSAGE_LOG_FD],[>>config.log]) <<EOF
         no ax_spec_file argument has been given, using defaults of
         (1) $ax_spec_file_
         (2) $ax_spec_file
EOF
         ;;
    esac
  fi
  # find specfile
  for i in ifelse($2,,,$2) . $srcdir $srcdir/.. .. ; do
    if test -f "$i/$ax_spec_file_" ; then
      ax_spec_dir="$i"
      ax_spec_file="$ax_spec_dir/$ax_spec_file_"
      break
    fi
    if test -f "$i/$ax_spec_file" ; then
      ax_spec_dir="$i"
      ax_spec_file="$ax_spec_dir/$ax_spec_file"
      break
    fi
  done
  case "$ax_spec_file" in
   ./*) if test "$ax_spec_file" = "./$ax_spec_file_" ; then
          ax_spec_file="$ax_spec_file_"
        fi
        AC_MSG_RESULT([$ax_spec_file]) ;;
   */*) AC_MSG_RESULT([$ax_spec_file]) ;;
   *)   AC_MSG_ERROR([no rpm spec file found]) ;;
  esac
  m4_define([m4_ax_spec_file],[ax_spec_file])
])

dnl AX_SPEC_EXTRACT(shellvar [,specvar [,defaultvalue]])

AC_DEFUN([AX_SPEC_EXTRACT],
[
    if test ".$[]$1" = "." ; then
    $1=`grep -i '^[[ 	]]*m4_ifval([$2],[$2],[$1])[[ 	]]*:' dnl
  "$ax_spec_file" | sed -e 's/.*:[[ 	]]*//' dnl
    -e 's/[[ 	]][[ 	]]*/ /g' -e 's/^ //' -e 's/ $[]//' -e 'q'`
    if test ".$[]$1" = "." ; then
  for $1 in $2 $1 m4_tolower([$1]) ; do
    $1=`echo "$[]$1" | sed -e 's/^%//g'`
    $1=`sed dnl
    -e  "/^%[[ 	]]*def[]ine[[ 	]][[ 	]]*$[]$1[[ 	]]/!d"  dnl
    -e "s/^%[[ 	]]*def[]ine[[ 	]][[ 	]]*$[]$1[[ 	]]*//" dnl
    -e 's/[[ 	]][[ 	]]*/ /' -e 's/ $[]//' -e 'q' $ax_spec_file`
    test ".$1" != "." && break
  done
    fi
  case ".$[]$1" in
    .%{*)
    $1=`echo "$[]$1" | sed -e 's/%{//' -e 's/}$[]//'`
    $1=`sed dnl
   -e  "/^%[[ 	]]*def[]ine[[ 	]][[ 	]]*$[]$1[[ 	]]/!d"  dnl
   -e "s/^%[[ 	]]*def[]ine[[ 	]][[ 	]]*$[]$1[[ 	]]*//" dnl
   -e 's/[[ 	]][[ 	]]*/ /' -e 's/ $[]//' -e 'q' $ax_spec_file` ;;
    .%*)
    $1=`echo "$[]$1" | sed -e 's/%//'`
    $1=`sed dnl
    -e  "/^%[[ 	]]*def[]ine[[ 	]][[ 	]]*$[]$1[[ 	]]/!d"  dnl
    -e "s/^%[[ 	]]*def[]ine[[ 	]][[ 	]]*$[]$1[[ 	]]*//" dnl
    -e 's/[[ 	]][[ 	]]*/ /' -e 's/ $[]//' -e 'q' $ax_spec_file` ;;
  esac
  m4_ifval([$3],[test ".$[]$1" = "." && $1="$3"])
    fi # test ".$[]$1" = "."
])

