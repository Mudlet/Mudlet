AUTOMAKE_OPTIONS = 1.4 foreign
AUTOTOOL_VERSION=autoconf-2.52 automake-1.5 libtool-1.4.2
DEFAULT_INCLUDES = # nothing - no default of -I. -I$(srcdir)
DEFS = @DEFS@ -I$(top_builddir) -I$(top_srcdir) # also for automake 1.4

EXTRA_DIST = test.zip test1.zip
DISTCLEANFILES = test*.dat test2.zip test3.zip

zzcat     = ../bins/zzcat$(EXEEXT)
zzdir     = ../bins/zzdir$(EXEEXT)
zziptest  = ../bins/zziptest$(EXEEXT)
zzxorcat  = ../bins/zzxorcat$(EXEEXT)
zzxordir  = ../bins/zzxordir$(EXEEXT)
zzxorcopy = ../bins/zzxorcopy$(EXEEXT)

$(zzcat) $(zzdir) $(zziptest) $(zzxorcat) $(zzxordir) $(xorcopy) : \
        @top_srcdir@/bins/*.c @top_srcdir@/zzip/*.c
	(cd ../bins && $(MAKE))

# -------------------------------------------------------------------
NULL=/dev/null
README = $(top_srcdir)/README
testdatadir = $(top_builddir)/testdata.d

testzip testzips : test.zip test1.zip test2.zip test3.zip 

test.zip tmp/../test.zip : $(README)
	- test -d tmp || mkdir tmp
	- $(MAKE) test0.zip && mv test0.zip $@
	- test -s $@ || cp $(srcdir)/test.zip $@
test0.zip : $(README)
	test -f README || ln -s $(README) .
	- $(MKZIP) test_.zip README
	@ if test -s test_.zip ;then echo mv test_.zip $@ ; mv test_.zip $@ ;fi
	@      test -d $(testdatadir) || mkdir $(testdatadir); if test -s $@ \
	; then echo cp $@ $(testdatadir)/ ; cp $@ $(testdatadir)/ \
	; else echo cp $(testdatadir)/$@ .; cp $(testdatadir)/$@ . \
	; fi ; echo test -s $@ ; test -s $@

test1.zip :
	@ echo $(MKZIP) $@ ... "(10 files)" \
	; mkdir test.tmp \
	; for i in 1 2 3 4 5 6 8 9 \
	; do echo "file-$i" >test.tmp/file.$$i ; done \
	; echo " a plain text file here" >test.tmp/README \
	; (cd test.tmp && $(MKZIP) ../$@ file.* >$(NULL)) \
	; rm -rf test.tmp
	@      test -d $(testdatadir) || mkdir $(testdatadir); if test -s $@ \
	; then echo cp $@ $(testdatadir)/ ; cp $@ $(testdatadir)/ \
	; else echo cp $(testdatadir)/$@ .; cp $(testdatadir)/$@ . \
	; fi ; echo test -s $@ ; test -s $@

test2.zip : # will not be shipped in source tarball
	@ echo $(MKZIP) $@ ... "(100 files)" \
	; mkdir test.tmp \
	; for i in 1 2 3 4 5 6 8 9 ; do for j in 1 2 3 4 5 6 7 8 9 \
	; do echo "file-$i" >test.tmp/file.$$j$$i ; done done \
	; (cd test.tmp && $(MKZIP) ../$@ file.* >$(NULL)) \
	; rm -rf test.tmp
	@      test -d $(testdatadir) || mkdir $(testdatadir); if test -s $@ \
	; then echo cp $@ $(testdatadir)/ ; cp $@ $(testdatadir)/ \
	; else echo cp $(testdatadir)/$@ .; cp $(testdatadir)/$@ . \
	; fi ; echo test -s $@ ; test -s $@

test3.zip : # will not be shipped in source tarball
	@ echo $(MKZIP) $@ ... "(1000 files)" \
	; mkdir test.tmp ; for h in 1 2 3 4 5 6 7 8 9 ; do : \
	; for i in 1 2 3 4 5 6 8 9 ; do for j in 1 2 3 4 5 6 7 8 9 \
	; do echo "file-$i" >test.tmp/file.$$h$$j$$i ; done done done \
	; (cd test.tmp && $(MKZIP) ../$@ file.* >$(NULL)) \
	; rm -rf test.tmp
	@      test -d $(testdatadir) || mkdir $(testdatadir); if test -s $@ \
	; then echo cp $@ $(testdatadir)/ ; cp $@ $(testdatadir)/ \
	; else echo cp $(testdatadir)/$@ .; cp $(testdatadir)/$@ . \
	; fi ; echo test -s $@ ; test -s $@

test4.zip : # will not be shipped in source tarball
	@ echo $(MKZIP) $@ ... "(1000 files)" \
	; mkdir test.tmp ; for h in 1 2 3 4 5 6 7 8 9 ; do : \
	; for i in 1 2 3 4 5 6 8 9 ; do for j in 1 2 3 4 5 6 7 8 9 \
	; do echo "file-$i" >test.tmp/file.$$h$$j$$i ; done done done \
	; (cd test.tmp && $(MKZIP) -n : ../$@ file.* >$(NULL)) \
	; rm -rf test.tmp
	@      test -d $(testdatadir) || mkdir $(testdatadir); if test -s $@ \
	; then echo cp $@ $(testdatadir)/ ; cp $@ $(testdatadir)/ \
	; else echo cp $(testdatadir)/$@ .; cp $(testdatadir)/$@ . \
	; fi ; echo test -s $@ ; test -s $@

test.dat : test.zip
	$(zzxorcopy) $? $@
test1.dat : test1.zip
	$(zzxorcopy) $? $@
test2.dat : test2.zip
	$(zzxorcopy) $? $@
test3.dat : test3.zip
	$(zzxorcopy) $? $@

# -------------------------------------------------------------------

stdtests = @ZIPTESTS@ check-test0 check-test1 check-zzdir check-zzcat
xortests = @ZIPTESTS@ check-zzxor check-zzxordir check-zzxorcat
sfxtests = @ZIPTESTS@ check-sfx

check-local: check-readme $(stdtests) $(xortests) $(sfxtests)

check-readme : $(zzcat)  test.zip
	@ test -f test.zip || $(MAKE) tmp/../test.zip 
	test -s test.zip
	@ echo :$@: checking $(zzcat) test/README 
	@ $(zzcat) test/README >test.out
	@ if diff test.out $(README) >$(NULL) \
	; then rm test.out ; echo check OK ; true \
	; else rm test.out ; echo check FAIL ; false ; fi

check-zzxor : $(zzxorcat) test.dat
	@ echo :$@: checking $(zzxorcat) test.dat/README 
	@ $(zzxorcat) test.dat/README >test.out
	@ if diff test.out $(README) >$(NULL) \
	; then rm test.out ; echo check OK ; true \
	; else rm test.out ; echo check FAIL ; false ; fi

test: test0 test1 test2 test3
test0: $(zziptest)
	$(zziptest) test.zip
test1: $(zziptest)
	$(zziptest) test1.zip
test2: $(zzcat)
	$(zzcat) test/README
test3: $(zzdir)
	$(zzdir) test1.zip
test4: $(zzdir) test4.zip
	$(zzcat) test4/file.113

check-test0 : $(zziptest)
	@ echo :$@: quick selftest
	$(zziptest) test.zip >test.out
	rm test.out
check-test1 : $(zziptest)
	@ test -f test1.zip || ln -s $(srcdir)/test1.zip test1.zip
	@ echo :$@: longer selftest "(about ten seconds)"
	$(zziptest) test1.zip >test.out
	rm test.out

check-zzdir : test1.zip test2.zip test3.zip $(zzdir) 
	@ echo :$@: zip directory access
	   ./$(zzdir) test1 > test.out
	@ X="$(zzdir) test1 "\
	; if grep file.1 test.out >$(NULL) ; then : \
	; if grep file.9 test.out >$(NULL) ; then : \
	; echo ..$$X.. OK ; true ; else echo ..$$X.. FAIL ; false ; fi fi
	   ./$(zzdir) test2 > test.out
	@ X="$(zzdir) test2 "\
	; if grep file.11 test.out >$(NULL) ; then : \
	; if grep file.99 test.out >$(NULL) ; then : \
	; echo ..$$X.. OK ; true ; else echo ..$$X.. FAIL ; false ; fi fi
	   ./$(zzdir) test3 > test.out
	@ X="$(zzdir) test3 "\
	; if grep file.111 test.out >$(NULL) ; then : \
	; if grep file.999 test.out >$(NULL) ; then : \
	; echo ..$$X.. OK ; true ; else echo ..$$X.. FAIL ; false ; fi fi
	@ X="$(zzdir) test000" ; echo ./$$X "(negative test)" \
	; if ./$(zzdir) test000 >test.out ; then : \
	; echo ..$$X.. FAIL ; false ; else echo ..$$X.. OK ; true ; fi 
	rm test.out

check-zzcat : test1.zip test2.zip test3.zip $(zzcat)
	@ echo ___ $@ ___
	   ./$(zzcat) test1/file.1 test1/file.9 > test.out
	@ X="$(zzcat) test1/file.1 test1/file.9 "\
	; if grep file-1 test.out >$(NULL) ; then : \
	; if grep file-9 test.out >$(NULL) ; then : \
	; echo ..$$X.. OK ; true ; else echo ..$$X.. FAIL ; false ; fi fi
	   ./$(zzcat) test2/file.11 test2/file.99 > test.out
	@ X="$(zzcat) test2/file.10 test2/file.99 "\
	; if grep file-10 test.out >$(NULL) ; then : \
	; if grep file-99 test.out >$(NULL) ; then : \
	; echo ..$$X.. OK ; true ; else echo ..$$X.. FAIL ; false ; fi fi
	   ./$(zzcat) test3/file.111 test3/file.999 > test.out
	@ X="$(zzcat) test3/file.111 test3/file.999 "\
	; if grep file-100 test.out >$(NULL) ; then : \
	; if grep file-999 test.out >$(NULL) ; then : \
	; echo ..$$X.. OK ; true ; else echo ..$$X.. FAIL ; false ; fi fi
	@ X="$(zzcat) test1/file.999" ; echo ./$$X "(negative test)" \
	; if ./$(zzdir) test1/file.999 >test.out ; then : \
	; echo ..$$X.. FAIL ; false ; else echo ..$$X.. OK ; true ; fi 
	rm test.out

check-zzxordir : test1.dat test2.dat $(zzxordir) 
	@ echo ___ $@ ___
	   ./$(zzxordir) test1 > test.out
	@ X="$(zzxordir) test1 "\
	; if grep file.1 test.out >$(NULL) ; then : \
	; if grep file.9 test.out >$(NULL) ; then : \
	; echo ..$$X.. OK ; true ; else echo ..$$X.. FAIL ; false ; fi fi
	   ./$(zzxordir) test2.dat > test.out
	@ X="$(zzxordir) test2.dat "\
	; if grep file.11 test.out >$(NULL) ; then : \
	; if grep file.99 test.out >$(NULL) ; then : \
	; echo ..$$X.. OK ; true ; else echo ..$$X.. FAIL ; false ; fi fi
	rm test.out

check-zzxorcat : test1.dat test2.dat $(zzxorcat)
	@ echo ___ $@ ___
	   ./$(zzxorcat) test1/file.1 test1/file.9 > test.out
	@ X="$(zzxorcat) test1/file.1 test1/file.9 "\
	; if grep file-1 test.out >$(NULL) ; then : \
	; if grep file-9 test.out >$(NULL) ; then : \
	; echo ..$$X.. OK ; true ; else echo ..$$X.. FAIL ; false ; fi fi
	   ./$(zzxorcat) test2.dat/file.11 test2/file.99 > test.out
	@ X="$(zzxorcat) test2.dat/file.10 test2/file.99 "\
	; if grep file-10 test.out >$(NULL) ; then : \
	; if grep file-99 test.out >$(NULL) ; then : \
	; echo ..$$X.. OK ; true ; else echo ..$$X.. FAIL ; false ; fi fi
	rm test.out

# -------------------------------------------------------------------
ECHON = echo -n
BRUTE = 0    # default is less than a thousand tests
brute :      # which is still a brute force selftest
	@ echo $(MKZIP) $@: \
	; for u in 1 2 3 4 5 6 8 9 ; do for v in 1 2 3 4 5 6 7 8 9 ; do : \
	; for w in 1 2 3 4 5 6 8 9 ; do for x in $(BRUTE) ; do : \
	; $(ECHON) $$u$$v$$w$$x: \
	; rm -rf test.tmp ; mkdir test.tmp \
	; for i in 1 2 3 4 5 6 8 9 ; do for j in 1 2 3 4 5 6 7 8 9 ; do : \
	; for m in 1 2 3 4 5 6 8 9 ; do for n in $(BRUTE) ; do : \
	; echo "file-$$i$$j$$m$$n" >test.tmp/file$$i$$j$$m$$n.txt \
	;        test "$$i$$j$$m$$n" = "$$u$$v$$w$$x" && break \
	; done ; test "$$i$$j$$m$$n" = "$$u$$v$$w$$x" && break \
	; done ; test "$$i$$j$$m$$n" = "$$u$$v$$w$$x" && break \
	; done ; test "$$i$$j$$m$$n" = "$$u$$v$$w$$x" && break \
	; done ; test -f test.tmp/file$$i$$j$$m$$n.txt || exit 2 \
	; (cd test.tmp && $(MKZIP) ../test$$u$$v$$w$$x.zip file*.* >$(NULL)) \
	; rm -rf test.tmp \
	; for i in 1 2 3 4 5 6 8 9 ; do for j in 1 2 3 4 5 6 7 8 9 ; do : \
	; for m in 1 2 3 4 5 6 8 9 ; do for n in $(BRUTE) ; do : \
	; ./$(zzcat) test$$u$$v$$w$$x/file$$i$$j$$m$$n.txt >test.out \
	; if grep file-$$i$$j$$m$$n test.out >$(NULL) ; then rm test.out \
	; else echo ERROR file-$$i$$j$$m$$n diff in test$$u$$v$$w$$x \
	; exit 1 ; fi \
	; ./$(zzdir) test$$u$$v$$w$$x >test.out \
	; if grep file$$i$$j$$m$$n.txt test.out >$(NULL) ; then rm test.out \
	; else echo ERROR file$$i$$j$$m$$n.txt not in test$$u$$v$$w$$x \
	; exit 1 ; fi \
	;        test "$$i$$j$$m$$n" = "$$u$$v$$w$$x" && break \
	; done ; test "$$i$$j$$m$$n" = "$$u$$v$$w$$x" && break \
	; done ; test "$$i$$j$$m$$n" = "$$u$$v$$w$$x" && break \
	; done ; test "$$i$$j$$m$$n" = "$$u$$v$$w$$x" && break \
	; done ; rm test$$u$$v$$w$$x.zip \
	; done ; done ; done ; done ; true

# -------------------------------------------------------------------

noinst_PROGRAMS = zzipself zzipsetstub

zzipself_LDFLAGS = @ZZIPLIB_LDFLAGS@
zzipself_LDADD = ../zzip/libzzip.la @RESOLVES@ -lz

../zzip/libzzip.la : @top_srcdir@/zzip/*.c
	(cd ../zzip && $(MAKE) `basename $@`)

zzshowme$(EXEEXT) : zzipself$(EXEEXT) zzipsetstub$(EXEEXT)
	$(MKZIP) -0 -j $@ .libs/zzipself$(EXEEXT)
	$(MKZIP) -9 -j $@ $(README)
	- test -s $@ || cp test2.zip $@
	test ! -f $@.zip || mv $@.zip $@
	./zzipsetstub $@ .libs/zzipself$(EXEEXT)
	- chmod +x $@

check-sfx : zzshowme$(EXEEXT)
	echo :$@: "./zzshowme readme >readme.out 2>readme.err" 
	export LD_LIBRARY_PATH="../zzip/.libs:$$LD_LIBRARY_PATH" \
	; ./zzshowme readme >readme.out 2>readme.err ; true
	@ echo 'diff readme.out $(README) || grep "libzzip-" readme.err' \
	; if test -s readme.out ; then diff readme.out $(README) \
        ; else grep "libzzip-" readme.err || echo "readme.out is empty!" ; fi
	rm readme.out readme.err
