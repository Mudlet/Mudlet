dnl /usr/share/aclocal/guidod-cvs/patch_libtool_on_darwin_zsh_overquoting.m4
dnl @synopsis PATCH_LIBTOOL_ON_DARWIN_ZSH_OVERQUOTING
dnl
dnl libtool 1.4.x has a bug on darwin where the "zsh" is installed as
dnl the bourne shell replacement. Of course, the zsh is called in a
dnl compatibility mode but there is a common problem with it, probably
dnl a bug of zsh. Newer darwin systems have a "bash" installed now, but
dnl the configure-default will be "zsh" in most systems still.
dnl
dnl The bug revelas itself as an overquoted statement in the libtool
dnl cmds-spec for sharedlib creation on testing for "module" builds.
dnl Later libtool has gone rid of it by simply removing the quotes at
dnl that point . Here we maintain the original style and simply remove
dnl the extra escape character, i.e. we look for "archive_cmds" and
dnl replace a sequence of triple-backslash-and-doublequote with
dnl single-backslash-and-doublequote.
dnl
dnl @category Misc
dnl @author Guido U. Draheim <guidod@gmx.de>
dnl @version 2003-03-23
dnl @license GPLWithACException

AC_DEFUN([PATCH_LIBTOOL_ON_DARWIN_ZSH_OVERQUOTING],
[# libtool-1.4 specific, on zsh target the final requoting does one too much
case "$host_os" in
  darwin*)
    if grep "1.92" libtool >/dev/null ; then
AC_MSG_RESULT(patching libtool on .so-sharedlib creation (zsh overquoting))
      test -f libtool.old || (mv libtool libtool.old && cp libtool.old libtool)
      sed -e '/archive_cmds=/s:[[\\]][[\\]][[\\]]*":\\":g' libtool >libtool.new
      (test -s libtool.new || rm libtool.new) 2>/dev/null
      test -f libtool.new && mv libtool.new libtool # not 2>/dev/null !!
      test -f libtool     || mv libtool.old libtool
    fi
  ;;
esac
])

