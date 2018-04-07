AUTOMAKE_OPTIONS = 1.4 foreign
AUTOTOOL_VERSION=autoconf-2.52 automake-1.5 libtool-1.4.2
DEFAULT_INCLUDES = # nothing - no default of -I. -I$(srcdir)
DEFS = @DEFS@ -I$(top_builddir) -I$(top_srcdir) # also for automake 1.4

noinst_PROGRAMS = zzipwrap
lib_LTLIBRARIES = libzzipwrap.la
zzipdir = ${includedir}/zzip
zzip_HEADERS = $(libzzipwrap_la_headers)
#
VERSION_INFO=@VERSION_INFO@
RELEASE_INFO=@RELEASE_INFO@
THREAD_SAFE=@THREAD_SAFE@
#

libzzipwrap_la_SOURCES = \
	wrap.c
libzzipwrap_la_headers = \
	wrap.h

libzzipwrap_la_LDFLAGS= @ZZIPLIB_LDFLAGS@ $(RELEASE_INFO) $(VERSION_INFO) \
		        $(THREAD_SAFE)
libzzipwrap_la_LIBADD= ../zzip/libzzip.la @RESOLVES@ -lz

../zzip/libzzip.la : @top_srcdir@/zzip/*.c
	(cd ../zzip && $(MAKE) `basename $@`)


WCC10_CLEAN = *.obj *.lk1 *.mk1 *.mk *.sym *.tgt

EXTRA_DIST = 
CLEANFILES = $(WCC10_CLEAN)  *.pc *.omf

zzipwrap_LDFLAGS =  @ZZIPLIB_LDFLAGS@
zzipwrap_LDADD = libzzipwrap.la @RESOLVES@ ../zzip/libzzip.la -lz

# ----------------------------------------------------------------------
pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_HEADERS= zzipwrap.pc

zzipwrap.pc : Makefile
	@ echo "prefix=$(prefix)" >$@
	@ echo "libdir=$(libdir)" >>$@
	@ echo "includedir=$(includedir)" >>$@
	@ echo " " >>$@
	@ echo "Name: zzipwrap" >>$@
	@ echo "Version: $(VERSION)" >>$@
	@ echo "Description: Callback Wrappers for ZZipLib" >>$@
	echo 'Requires: zziplib' >>$@
	echo 'Libs: -L$${libdir} -lzzipwrap' >>$@
	echo 'Cflags: -I$${includedir}' >>$@
