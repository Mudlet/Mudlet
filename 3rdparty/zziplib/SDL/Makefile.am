AUTOMAKE_OPTIONS = 1.4 foreign
AUTOTOOL_VERSION=autoconf-2.52 automake-1.5 libtool-1.4.2

zzipsdldir = ${includedir}/SDL_rwops_zzip
zzipsdl_DATA = SDL_rwops_zzip.c SDL_rwops_zzip.h

EXTRA_DIST = $(sdl_DATA)
check_PROGRAMS = zzcatsdl

zzcatsdl_SOURCES = SDL_rwops_zzcat.c SDL_rwops_zzip.c
ZZIP_CFLAGS= `export PKG_CONFIG_PATH=../zzip; pkg-config zziplib --cflags` 
ZZIP_LIBS= `export PKG_CONFIG_PATH=../zzip; pkg-config zziplib --libs`
SDL_CFLAGS= `export PKG_CONFIG_PATH=.; pkg-config sdl --cflags` 
SDL_LIBS= `export PKG_CONFIG_PATH=.; pkg-config sdl --libs`
CFLAGS= -I$(srcdir) $(ZZIP_CFLAGS) $(SDL_CFLAGS)
LIBS= $(ZZIP_LIBS) $(SDL_LIBS)

default: test-sdl
*.$(OBJEXT): ../zzip/zziplib.pc
../zzip/zziplib.pc : ../zzip/Makefile
	cd ../zzip && $(MAKE) zziplib.pc

test.zip : @top_srcdir@/test/test.zip
	$(LN_S) $? $@
testsdl test-sdl : test.zip zzcatsdl$(EXEEXT)
	./zzcatsdl$(EXEEXT) test/README > test.out
	diff -q @top_srcdir@/README test.out && rm test.out

all: @SDL_GENERATE@
sdl : all
install-sdl : install-data # and not the binary

CLEANFILES = *.pc *.omf
# ----------------------------------------------------------------------
# we do not install a library, so this about equivalent to some SDL.pc
pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_HEADERS= SDL_rwops_zzip.pc
ERRNULL= 2>/dev/null

SDL_rwops_zzip.pc : Makefile
	@ echo 'prefix=$(prefix)' >$@
	@ echo 'zzipsdldir=$(zzipsdldir)' >>$@
	@ echo " " >>$@
	@ echo "Name: SDL_rwops_zzip" >>$@
	@ echo "Version: $(VERSION)" >>$@
	@ echo "Description: SDL_rwops for ZZipLib" >>$@
	echo 'Requires: sdl, zziplib' >>$@
	echo 'Cflags: -I$${zzipsdldir}' >>$@

sdl-pc : sdl-uninstalled.pc
sdl-uninstalled.pc : Makefile
	@ echo "prefix=`sdl-config --prefix`" >$@
	@ echo "Name: sdl" >>$@
	@ echo "Version: `sdl-config --version`" >>$@
	@ echo "Description: sdl - Simple DirectMedia Layer" >>$@
	echo "Libs: `sdl-config --libs`" >> $@
	echo "Cflags: `sdl-config --cflags`" >>$@
