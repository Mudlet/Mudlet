dnl /usr/share/aclocal/guidod-cvs/patch_libtool_sys_lib_search_path_spec.m4
dnl ______ /usr/share/aclocal/guidod-cvs/patch_libtool_sys_lib_search_path_spec.m4 ______
dnl @synopsis PATCH_LIBTOOL_SYS_LIB_SEARCH_PATH_SPEC
dnl
dnl Cross-compiling to win32 from a unix system reveals a bug - the
dnl path-separator has been set to ";" depending on the target system.
dnl However, the crossgcc search_path_spec works in a unix-environment
dnl with unix-style directories and unix-stylish path_separator. The
dnl result: the search_path_spec is a single word still containing the
dnl ":" separators.
dnl
dnl This macro fixes the situation: when we see the libtool PATH_SEP to
dnl be ":" and search_path_spec to contain ":" characters, then these
dnl are replaced with spaces to let the resulting string work as a
dnl for-loop argument in libtool scripts that resolve -no-undefined
dnl libraries.
dnl
dnl Later libtool generations have fixed the situation with using
dnl $PATH_SEPARATOR in the first place as the original path delimiter
dnl that will be scanned for and replaced into spaces.
dnl
dnl @category Misc
dnl @author Guido U. Draheim <guidod@gmx.de>
dnl @version 2005-01-21
dnl @license GPLWithACException

AC_DEFUN([PATCH_LIBTOOL_SYS_LIB_SEARCH_PATH_SPEC],
[# patch libtool to fix sys_lib_search_path (e.g. crosscompiling a win32 dll)
if test "_$PATH_SEPARATOR" = "_:" ; then
  if grep "^sys_lib_search_path_spec.*:" libtool >/dev/null ; then
AC_MSG_RESULT(patching libtool to fix sys_lib_search_path_spec)
    test -f libtool.old || (mv libtool libtool.old && cp libtool.old libtool)
    sed -e "/^sys_lib_search_path_spec/s/:/ /g" libtool >libtool.new
    (test -s libtool.new || rm libtool.new) 2>/dev/null
    test -f libtool.new && mv libtool.new libtool # not 2>/dev/null !!
    test -f libtool     || mv libtool.old libtool
  fi
fi
])


