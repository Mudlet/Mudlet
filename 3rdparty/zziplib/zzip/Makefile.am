AUTOMAKE_OPTIONS = 1.5 foreign
AUTOTOOL_VERSION=autoconf-2.52 automake-1.5 libtool-1.4.2
DEFAULT_INCLUDES = # nothing - no default of -I. -I$(srcdir)
DEFS = @DEFS@ -I$(top_builddir) -I$(top_srcdir) # also for automake 1.4

lib_LTLIBRARIES = libzzip.la libzzipmmapped.la libzzipfseeko.la
zzipdir = ${includedir}/zzip
zzip_HEADERS = $(libzzip_la_headers) \
               $(libzzipmmapped_la_headers) \
               $(libzzipfseeko_la_headers)
noinst_HEADERS = __debug.h __hints.h __mmap.h __dirent.h __fnmatch.h
#
VERSION_INFO=@VERSION_INFO@
RELEASE_INFO=@RELEASE_INFO@
THREAD_SAFE=@THREAD_SAFE@
#

libzzip_la_SOURCES = \
	zip.c \
	file.c \
	dir.c \
	stat.c \
	info.c \
	err.c \
	plugin.c \
	write.c \
	fetch.c
libzzip_la_headers = \
	stdint.h \
	file.h \
	plugin.h \
	lib.h \
	zzip.h \
	zzip32.h \
        write.h \
	fetch.h \
	format.h \
        info.h \
	autoconf.h \
	types.h \
	conf.h \
	_config.h \
        _msvc.h
libzzip_la_LDFLAGS= @ZZIPLIB_LDFLAGS@ $(RELEASE_INFO) $(VERSION_INFO) \
                    $(THREAD_SAFE)
libzzip_la_LIBADD= -lz @RESOLVES@

libzzipmmapped_la_SOURCES =  mmapped.c memdisk.c fetch.c
libzzipmmapped____headers =  mmapped.h memdisk.h fetch.h
libzzipmmapped_la_headers =  mmapped.h memdisk.h
libzzipmmapped_la_LDFLAGS= @ZZIPLIB_LDFLAGS@ $(RELEASE_INFO) $(VERSION_INFO) \
                    $(THREAD_SAFE)
libzzipmmapped_la_LIBADD= -lz @RESOLVES@

libzzipfseeko_la_SOURCES = fseeko.c fetch.c
libzzipfseeko____headers = fseeko.h fetch.h
libzzipfseeko_la_headers = fseeko.h
libzzipfseeko_la_LDFLAGS= @ZZIPLIB_LDFLAGS@ $(RELEASE_INFO) $(VERSION_INFO) \
                    $(THREAD_SAFE)
libzzipfseeko_la_LIBADD= -lz @RESOLVES@

WCC10_DIST = zziplib.wpj
WCC10_CLEAN = *.obj *.lk1 *.mk1 *.mk *.sym *.tgt

EXTRA_DIST =  $(WCC10_DIST) _msvc.sed 
CONFIG_CLEAN_FILES = _config.h 
MAINTAINERCLEANFILES = _msvc.h
CLEANFILES = $(WCC10_CLEAN)  *.pc *.omf *~ *.tmp *.tmp.*

# here is some backward compatibility with version 0.10.* of zziplib
install-data-hook : install-zzipHEADERS
	echo '#ifndef ZZIP_WARNING'   > $(DESTDIR)$(includedir)/zzip.h
	echo '#define ZZIP_WARNING 1' >>$(DESTDIR)$(includedir)/zzip.h
	echo '#ifdef __GNUC__'        >>$(DESTDIR)$(includedir)/zzip.h
	echo '#warning do no not use <zzip.h>, update to include <zzip/lib.h>'\
	                              >>$(DESTDIR)$(includedir)/zzip.h
	echo '#else'                 >>$(DESTDIR)$(includedir)/zzip.h
	echo '#error   do no not use <zzip.h>, update to include <zzip/lib.h>'\
	                              >>$(DESTDIR)$(includedir)/zzip.h
	echo '#endif'                 >>$(DESTDIR)$(includedir)/zzip.h
	echo '#endif'                 >>$(DESTDIR)$(includedir)/zzip.h
	echo '#include "zzip/lib.h"'  >>$(DESTDIR)$(includedir)/zzip.h
	sed -e 's|zzip.h|zziplib.h|' -e 's|zzip/lib.h|zzip/zzip.h|' \
	$(DESTDIR)$(includedir)/zzip.h >$(DESTDIR)$(includedir)/zziplib.h
	sed -e 's|zzip.h|zzip-io.h|' -e 's|zzip/lib.h|zzip/plugin.h|' \
	$(DESTDIR)$(includedir)/zzip.h >$(DESTDIR)$(includedir)/zzip-io.h

install-exec-hook:
	@ for i in . $(DESTDIR)$(libdir)/libzzip*.so.13 \
        ; do test -d $$i && continue ; test -f $$i || continue \
        ; lib=`basename "$$i" .so.13` \
        ; echo "$(DESTDIR)$(libdir): ln -s $$lib.so.13 $$lib.so.10" \
        ; (cd $(DESTDIR)$(libdir) && ln -s $$lib.so.13 $$lib.so.10) \
        ; echo "$(DESTDIR)$(libdir): ln -s $$lib.so.13 $$lib.so.11" \
        ; (cd $(DESTDIR)$(libdir) && ln -s $$lib.so.13 $$lib.so.11) \
        ; echo "$(DESTDIR)$(libdir): ln -s $$lib.so.13 $$lib.so.12" \
        ; (cd $(DESTDIR)$(libdir) && ln -s $$lib.so.13 $$lib.so.12) \
	; done ; true

uninstall-local :
	(cd $(DESTDIR)$(includedir) && rm zziplib.h  zzip.h  zzip-io.h)


INDENT = indent
INDENTFLAGS = \
--blank-lines-after-procedures \
--blank-lines-before-block-comments \
--brace-indent0 \
--braces-after-if-line \
--case-indentation0 \
--cuddle-else \
--space-after-cast \
--honour-newlines \
--indent-level4 \
--line-length80 \
--continue-at-parentheses \
--leave-preprocessor-space \
--dont-break-function-decl-args \
--dont-cuddle-do-while \
--dont-format-first-column-comments \
--no-space-after-function-call-names \
--no-space-after-parentheses \
--no-tabs \
--ignore-profile \
--procnames-start-lines \
--space-after-for \
--space-after-if \
--space-after-while \
--space-special-semicolon 

DIFF_U0 = diff -U0
# __indent1 = "s|^\\([^{};]*if[^{};]*\\)\\(return[^{};]*;\\)|\\1 { \\2 }|" 
__indent1 = ":begin"
__indent2 = "s|^\\([^{}';]*\\)\\({[^{}']*}\\)\\([^{}';]*\\)$$|\\1(' \\2 ');\\3|"
__indent3 = "s|___+[;].*|/*&*/|"
__indent4 = ":end"
__indent6 = "s|!\\([^ !=)]\\)|! \\1|g"
__indent7 = "s|(' \\({[^{}']*}\\) ');|\\1|"
__indent8 = "s|/[*]\\(___+[;].*\\)[*]/$$|\\1|"
__indent9 = ":end"
indent-check:
	for file in $(srcdir)/*.c ; do f=`basename "$$file"`  \
	; sed -e $(__indent1) -e $(__indent2) -e $(__indent3) -e $(__indent4) \
          $(INDENTBEFORE) $$file \
	| $(INDENT) $(INDENTFLAGS) $(INDENTOPTS) -st - \
	| sed -e $(__indent6) -e $(__indent7) -e $(__indent8) -e $(__indent9) \
          $(INDENTAFTER) > $$f.tmp \
	; echo $(DIFF_U0) "$$file" "$$f.tmp" \
	; $(DIFF_U0) "$$file" "$$f.tmp" ; done

i:
	@ f=`basename $(file) .tmp` \
	; echo cp $(file) $(srcdir)/$$f ; sleep 2 \
	; cp $(file) $(srcdir)/$$f

# ----------------------------------------------------------------------
pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_HEADERS= zzip-zlib-config.pc zziplib.pc \
                    zzipmmapped.pc zzipfseeko.pc

zziplib.pc : Makefile
	(cd .. && sh config.status zzip/zziplib.pc)

zzip-zlib-config.pc : Makefile
	@ echo "zlib_libs=@ZLIB_LDIR@ -lz" >$@
	@ echo "zlib_cflags=@ZLIB_INCL@" >>$@
	@ echo " " >>$@
	@ echo "Name: zzip-zlib-config" >>$@
	@ echo "Version: @ZLIB_VERSION@" >>$@
	@ echo "Description: ZLib Config (for ZZipLib)" >>$@
	echo 'Libs: $${zlib_libs}' >>$@
	echo 'Cflags: $${zlib_cflags}' >>$@

zzipmmapped.pc : zziplib.pc
	sed -e 's/-lzzip/-lzzipmmapped/' -e 's/zziplib/zzipmmapped/' \
	zziplib.pc >$@
zzipfseeko.pc : zziplib.pc
	sed -e 's/-lzzip/-lzzipfseeko/' -e 's/zziplib/zzipfseeko/' \
	zziplib.pc >$@

__zziplib.pc : Makefile
	@ echo "prefix=$(prefix)" >$@
	@ echo "libdir=$(libdir)" >>$@
	@ echo "includedir=$(includedir)" >>$@
	@ echo "largefile=$(LARGEFILE_CFLAGS)" >>$@
	@ echo "zlib_libs=@ZLIB_LDIR@ -lz" >>$@
	@ echo "zlib_cflags=@ZLIB_INCL@" >>$@
	@ echo " " >>$@
	@ echo "Name: zziplib" >>$@
	@ echo "Version: $(VERSION)" >>$@
	@ head $(top_srcdir)/$(PACKAGE).spec \
	| grep Summary | sed -e s,Summary,Description, >> $@
	echo 'Requires: zzip-zlib-config' >>$@
	echo 'Libs: -L$${libdir} -lzzip' >>$@
	echo 'Cflags: $${largefile} -I$${includedir}' >>$@

# ----------------------------------------------------------------------
.c.i :
	$(COMPILE) -E $< >$@
