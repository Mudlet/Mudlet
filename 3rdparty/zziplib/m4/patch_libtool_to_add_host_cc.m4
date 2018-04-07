dnl /usr/share/aclocal/guidod-cvs/patch_libtool_to_add_host_cc.m4
dnl @synopsis PATCH_LIBTOOL_TO_ADD_HOST_CC
dnl
dnl The libtool 1.4.x processing (and patched 1.3.5) uses a little
dnl "impgen" tool to turn a "*.dll" into an import "*.lib" as it is
dnl needed for win32 targets. However, this little tool is not shipped
dnl by binutils, it is not even a command option of dlltool or dllwrap.
dnl It happens to be a C source snippet implanted into the libtool
dnl sources - it gets written to ".libs", compiled into a binary
dnl on-the-fly, and executed right away on the "dll" file to create the
dnl import-lib (dll.a files in gcc-speak).
dnl
dnl This mode works fine for a native build within mingw or cygwin, but
dnl it does not work in cross-compile mode since CC is a crosscompiler
dnl - it will create an .exe file on a non-win32 system, and as a
dnl result an impgen.exe is created on-the-fly that can not be executed
dnl on-the-fly. Luckily, the actual libtool snippet uses HOST_CC to
dnl compile the sources which has a fallback to CC when the HOST_CC
dnl variable was not set.
dnl
dnl this ac-macro is trying to detect a valid HOST_CC which is not a
dnl cross-compiler. This is done by looking into the $PATH for a "cc"
dnl and the result is patched into libtool a HOST_CC, iow it adds
dnl another configured variable at the top of the libtool script.
dnl
dnl In discussions on the libtool mailinglist it occurred that later
dnl gcc/binutils generations are able to link with dlls directly, i.e.
dnl there is no import-lib needed anymore. The import-table is created
dnl within the linker itself (in-memory) and bound to the .exe/.dll
dnl currently in the making. The whole stuff of impgen exe and
dnl compiling it on-the-fly, well, it is superflouos then.
dnl
dnl Since mingw crosscompilers tend to be quite a fresh development it
dnl was agreed to remove the impgen stuff completly from libtool
dnl sources. Still however, this macro does not hurt since it does not
dnl patch impgen cmds but it just adds HOST_CC which might be useful in
dnl other cross-compiling cases as well. Therefore, you can leave it in
dnl for maximum compatibility and portability.
dnl
dnl @category Misc
dnl @category C
dnl @author Guido U. Draheim <guidod@gmx.de>
dnl @version 2005-01-21
dnl @license GPLWithACException

AC_DEFUN([PATCH_LIBTOOL_TO_ADD_HOST_CC],
[# patch libtool to add HOST_CC sometimes needed in crosscompiling a win32 dll
if grep "HOST_CC" libtool >/dev/null; then
  if test "$build" != "$host" ; then
    if test "_$HOST_CC" = "_" ; then
      HOST_CC="false"
      for i in `echo $PATH | sed 's,:, ,g'` ; do
      test -x $i/cc && HOST_CC=$i/cc
      done
    fi
AC_MSG_RESULT(patching libtool to add HOST_CC=$HOST_CC)
    test -f libtool.old || (mv libtool libtool.old && cp libtool.old libtool)
    sed -e "/BEGIN.*LIBTOOL.*CONFIG/a\\
HOST_CC=$HOST_CC" libtool >libtool.new
    (test -s libtool.new || rm libtool.new) 2>/dev/null
    test -f libtool.new && mv libtool.new libtool # not 2>/dev/null !!
    test -f libtool     || mv libtool.old libtool
  fi
fi
])

