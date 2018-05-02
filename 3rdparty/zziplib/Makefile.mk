PUB=pub/

auto:
	aclocal -I m4 && autoconf -I m4 && autoheader && automake

boottrap:
	rm -rf .deps .libs
	rm -f config.guess config.sub stamp-h.in
	rm -f install-sh ltconfig ltmain.sh depcomp mkinstalldirs
	rm -f config.h config.h.in config.log config.cache configure
	rm -f aclocal.m4 Makefile Makefile.in
	aclocal 
	autoconf 
	autoheader 
	automake -a -c 

rpm2: dist-bzip $(PACKAGE).spec
	rpmbuild -ta pub/$(PACKAGE)-$(VERSION).tar.bz2

dist-bzip : dist-bzip2
	$(MAKE) dist-bzip2-done
dist-bzip2-done dist-done :
	test -d $(PUB) || mkdir $(PUB)
	@ echo cp $(BUILD)/$(PACKAGE)-$(VERSION).tar.bz2 $(PUB). \
	;      cp $(BUILD)/$(PACKAGE)-$(VERSION).tar.bz2 $(PUB).
snapshot:
	$(MAKE) dist-bzip2 VERSION=`date +%Y.%m%d`
distclean-done:
	- rm -r *.d

configsub :
	cp ../savannah.config/config.guess uses/config.guess
	cp ../savannah.config/config.sub   uses/config.sub

