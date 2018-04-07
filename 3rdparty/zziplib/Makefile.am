AUTOMAKE_OPTIONS = 1.4 foreign dist-bzip2
ACLOCAL_AMFLAGS = -I m4
WANT_AUTOMAKE = 1.7
WANT_AUTOCONF = 2.57

DIST_SUBDIRS = zzip zzipwrap bins test docs  SDL
SUBDIRS =      zzip zzipwrap bins test docs @SDL@

rpm: dist-bzip2 $(PACKAGE).spec
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.bz2
	test -d ../pub && cp $(PACKAGE)-$(VERSION).tar.bz2 ../pub/

OSC_ROOT = /my/own/osc
OSC_SRCDIR = $(OSC_ROOT)
OSC_PROJECT = home:guidod:zziplib-13
OSC_PACKAGE = $(PACKAGE)
OSC_SPECFILE = $(top_srcdir)/$(PACKAGE).spec
OSC_TARBALL = $(PACKAGE)-$(VERSION).tar.bz2
OSC_BUILDDIR = $(OSC_ROOT)
OSC_PACKAGECACHEDIR = $(OSC_BUILDDIR)/packagecache
OSC_BUILDROOT = $(OSC_BUILDDIR)/buildroot
OSC_REPOSITORY = openSUSE_11.2
OSC_ARCH = x86_64
osc-copy:
	test -s $(OSC_TARBALL)
	test -s $(OSC_SPECFILE)
	test -d $(OSC_SRCDIR)
	test -d $(OSC_SRCDIR)/$(OSC_PROJECT) \
	|| ( cd $(OSC_SRCDIR) && osc co $(OSC_PROJECT) )
	cd $(OSC_SRCDIR)/$(OSC_PROJECT) && osc up
	cp $(OSC_TARBALL)  $(OSC_SRCDIR)/$(OSC_PROJECT)/$(OSC_PACKAGE)
	cp $(OSC_SPECFILE) $(OSC_SRCDIR)/$(OSC_PROJECT)/$(OSC_PACKAGE)

DOIT = :
osc-ci osc-commit upload-osc: osc-copy
	tarball=`basename $(OSC_TARBALL)` \
	; ls  $(OSC_SRCDIR)/$(OSC_PROJECT)/$(OSC_PACKAGE)/*.tar.* \
	| { while read f; do tarname=`basename $$f` \
	; if test "$$tarname" != "$$tarball" \
	; then echo osc rm -f $$f ; osc rm -f $$f \
	; else echo osc add $$f ; osc add $$f || true \
	; fi ; done }
	osc status $(OSC_SRCDIR)/$(OSC_PROJECT)/$(OSC_PACKAGE)/
	$(DOIT) osc commit -m "update $(OSC_TARBALL)" $(OSC_SRCDIR)/$(OSC_PROJECT)/$(OSC_PACKAGE)

osc-build: osc-copy
	cd $(OSC_SRCDIR)/$(OSC_PROJECT)/$(OSC_PACKAGE)/ \
	BUILD_DESCR=`basename $(OSC_SPECFILE)` ;\
	ls -l $$SPEC ; \
	OSC_PACKAGECACHEDIR=$(OSC_PACKAGECACHEDIR) \
	OSC_BUILD_ROOT=$(OSC_BUILDROOT) \
	osc build --release=0 --local-package --clean \
	$(OSC_REPOSITORY) $(OSC_ARCH) $$BUILD_DESCR
osc-clean:
	- test -f $(OSC_SRCDIR)/$(OSC_PROJECT)/$(OSC_PACKAGE)/*.spec \
	&& rm $(OSC_SRCDIR)/$(OSC_PROJECT)/$(OSC_PACKAGE)/*
osc-distclean:
	- test -f $(OSC_SRCDIR)/$(OSC_PROJECT)/$(OSC_PACKAGE)/*.spec \
	&& rm -rf $(OSC_SRCDIR)/*
		

indent-check:
	(cd zzip && $(MAKE) `basename $@`)

doc docs docu clean-doc clean-docs clean-docu   zzip.html zzip.xml zzip.pdf \
man mans manpages htmpages unpack clean-unpack  changes.htm pdfs \
omf install-omf install-doc install-docs install-man3 install-mans \
upload-sourceforge www:
	(cd docs && $(MAKE) `basename $@`)

sdl testsdl test-sdl install-sdl :
	if test -z "$(SDL)"; then echo "skip $@ (no SDL)"; else \
	(cd SDL && $(MAKE) `basename $@`) ; fi

check-test0 check-test1 check-zzdir check-zzcat \
check-zzxor check-zzxordir check-zzxorcat \
check-sfx     check-readme : \
	(cd test && $(MAKE) `basename $@`)

MSVC8 =   msvc8/README.TXT msvc8/zip.exe msvc8/test1.zip msvc8/test.zip \
msvc8/zzdir.vcproj    msvc8/zzipself.vcproj     msvc8/zzip.vcproj \
msvc8/zziplib.sln     msvc8/zzipsetstub.sln     msvc8/zzobfuscated.sln \
msvc8/zziplib.vcproj  msvc8/zzipsetstub.vcproj  msvc8/zzobfuscated.vcproj \
msvc8/zzcat.sln       msvc8/zzipself.bat        msvc8/zzip.sln \
msvc8/zzcat.vcproj    msvc8/zzipself.sln        msvc8/zziptest.sln \
msvc8/zzdir.sln       msvc8/zzipself.txt        msvc8/zziptest.vcproj \
                      msvc8/zzipfseeko.vcproj   msvc8/zzipmmapped.vcproj 

MSVC7 =   msvc7/pkzip.exe msvc7/test1.zip msvc7/test.zip \
msvc7/zzdir.vcproj    msvc7/zzipself.vcproj     msvc7/zzip.vcproj \
msvc7/zziplib.sln     msvc7/zzipsetstub.sln     msvc7/zzobfuscated.sln \
msvc7/zziplib.vcproj  msvc7/zzipsetstub.vcproj  msvc7/zzobfuscated.vcproj \
msvc7/zzcat.sln       msvc7/zzipself.bat        msvc7/zzip.sln \
msvc7/zzcat.vcproj    msvc7/zzipself.sln        msvc7/zziptest.sln \
msvc7/zzdir.sln       msvc7/zzipself.txt        msvc7/zziptest.vcproj

MSVC6 = \
msvc6/zzcat.dsp        msvc6/zziplib.dsp      msvc6/zzipwrap.dsp \
msvc6/zzdir.dsp        msvc6/zziplib.dsw      msvc6/zzobfuscated.dsp \
msvc6/zziptest.dsp     msvc6/zzip.dsp  

EXTRA_DIST =   zziplib.spec zzipback.sed Makefile.mk \
               $(MSVC8) $(MSVC7) $(MSVC6) $(am__aclocal_m4_deps)

PHONY = auto boottrap rpm doc docs man manpages htmpages sdl testsdl \
	comp compats msvc6 msvc7
# ------------------------------------------------------------------------

test-comp:
	test ! -d testing || rm -r testing
	mkdir testing
	(cd testing && tar xzvf ../$(PACKAGE)-$(VERSION).tar.gz)
	(cd "testing/$(PACKAGE)-$(VERSION)/zziplib" && sh configure)
	$(MAKE) -C  "testing/$(PACKAGE)-$(VERSION)/zziplib"
	$(MAKE) -C  "testing/$(PACKAGE)-$(VERSION)/zziplib"  check

clean-comp:
	test ! -d testing || rm -r testing

msvc :
	mkdir bin
	mkdir lib
	mkdir include
	mkdir include/zzip
	cp $(srcdir)/msvc6/Release/*.exe bin/
	cp $(srcdir)/msvc6/Release/*.dll bin/
	cp $(srcdir)/msvc6/Release/*.lib lib/
	cp $(srcdir)/zzip/*.h include/zzip/
	zip -9r $(srcdir)/$(PACKAGE)-$(VERSION)-msvc6-bin.zip bin/
	zip -9r $(srcdir)/$(PACKAGE)-$(VERSION)-msvc6-lib.zip lib/ include/
	rm -r bin lib include

# ------------------------------------------------------------------ zzip64
_FILE_OFFSET64 = -D_ZZIP_LARGEFILE -D_FILE_OFFSET_BITS=64
_RELEASEINFO64 = "RELEASE_INFO=-release 0-64"

zzip64-setup: zzip64-setup.tmp
zzip64-setup.tmp : zzip/Makefile zzip/_config.h zzip/.deps/* zzip/zziplib.pc
	- mkdir zzip64
	cp -a zzip/Makefile zzip/_config.h zzip/.deps/ zzip/zziplib.pc zzip64/
	date > zzip64.dir
zzip64-build: zzip64-build.tmp
zzip64-build.tmp : zzip64-setup.tmp $(top_srcdir)/zzip/*
	cd zzip64 && $(MAKE) "AM_CFLAGS=$(_FILE_OFFSET64)" $(_RELEASEINFO64)
zzip64-install: zzip64-install.tmp
zzip64-install.tmp : zzip64-build.tmp
	cd zzip64 && $(MAKE) install $(_RELEASEINFO64)
	cd $(DESTDIR)$(libdir) && mv    libzzip.so   libzzip64.so
	- cd $(DESTDIR)$(libdir) && mv    libzzip.a    libzzip64.a
	cd $(DESTDIR)$(libdir) && \
        sed -e 's/zzip.so/zzip64.so/' -e 's/zzip.a/zzip64.a/' \
             libzzip.la > libzzip64.la
	cd $(DESTDIR)$(libdir)/pkgconfig && \
	sed -e 's/largefile=/largefile= %_FILE_OFFSET64/' \
	    -e 's/-lzzip/-lzzip64/' -e 's/zziplib/zziplib64/' \
	     zziplib.pc > zziplib64.pc
zzip32-postinstall:
	cd   $(DESTDIR)$(libdir) && mv    libzzip.so   libzzip32.so
	- cd $(DESTDIR)$(libdir) && mv    libzzip.a    libzzip32.a
	cd   $(DESTDIR)$(libdir) && ln -s libzzip32.so libzzip.so
	- cd $(DESTDIR)$(libdir) && ln -s libzzip32.a  libzzip.a
	cd $(DESTDIR)$(libdir) && \
	sed -e 's/zzip.so/zzip32.so/' -e 's/zzip.a/zzip32.a/' \
	    libzzip.la > libzzip32.la
	cd $(DESTDIR)$(libdir)/pkgconfig && \
	sed -e 's/-lzzip/-lzzip32/' -e 's/zziplib/zziplib32/' \
	    zziplib.pc > zziplib32.pc
zzip-postinstall:
	: "the 12.8x and 11.8x and 10.8x packages are all the same actually"
	cd $(DESTDIR)$(libdir) || exit 1 \
	; for i in libzzip*.so.1? ; do : \
	; v10=`echo $$i | sed -e "s/.so.../.so.10/"` \
	; v11=`echo $$i | sed -e "s/.so.../.so.11/"` \
	; v12=`echo $$i | sed -e "s/.so.../.so.12/"` \
	; test ! -e "$$v10" && test -e "$$v12" && test "$$v10" != "$$v12" && ln -s "$$v12" "$$v10" \
	; test ! -e "$$v12" && test -e "$$v10" && test "$$v12" != "$$v10" && ln -s "$$v10" "$$v12" \
	; test "$$v10" != "$$v11" && ln -s "$$v10" "$$v11" || true; done
