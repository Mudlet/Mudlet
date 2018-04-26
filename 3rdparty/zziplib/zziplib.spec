# norootforbuild
Summary:      ZZipLib - libZ-based ZIP-access Library with an Easy-to-Use API
Name:         zziplib
Version:      0.13.62
Release:      1
License:      LGPLv2.1+
Group:        System/Libraries
URL:          http://zziplib.sf.net
Vendor:       Guido Draheim <guidod@gmx.de>
Source0:      http://prdownloads.sf.net/%{name}/%{name}-%{version}.tar.bz2
BuildRoot:    /var/tmp/%{name}-%{version}-%{release}

Distribution: Original
Packager:     Guido Draheim <guidod@gmx.de>
Requires:      zlib
BuildRequires: zlib-devel
BuildRequires: SDL-devel
BuildRequires: zip

Provides:     libzzip0 = %version
Provides:     libzzip-0.so.10
Provides:     zziplib-lib010 = %version

#Begin3
# Author1:        too@iki.fi (Tomi Ollila)
# Author2:        guidod@gmx.de (Guido Draheim)
# Maintained-by:  guidod@gmx.de (Guido Draheim)
# Primary-Site:   zziplib.sf.net
# Keywords:       zip zlib inflate archive gamedata
# Platforms:      zlib posix
# Copying-Policy: Lesser GPL Version 2
#End

%package doc
Summary:      ZZipLib - Documentation Files
Group:        Development/Languages/C and C++
BuildRequires: python
BuildRequires: xmlto
PreReq: scrollkeeper

%package devel
Summary:      ZZipLib - Development Files
Group:        Development/Languages/C and C++
Requires:     zziplib = %version
Requires:     pkgconfig

%package SDL_rwops-devel
Summary:      ZZipLib - Development Files for SDL_rwops
Group:        Development/Languages/C and C++
Requires:     zziplib = %version
Requires:     pkgconfig
BuildRequires: SDL-devel

%description
 : zziplib provides read access to zipped files in a zip-archive,
 : using compression based solely on free algorithms provided by zlib.
 zziplib provides an additional API to transparently access files
 being either real files or zipped files with the same filepath argument.
 This is handy to package many files being shared data into a single
 zip file - as it is sometimes used with gamedata or script repositories.
 The library itself is fully multithreaded, and it is namespace clean
 using the zzip_ prefix for its exports and declarations.

%description doc
 : zziplib provides read access to zipped files in a zip-archive,
 : using compression based solely on free algorithms provided by zlib.
 these are the (html) docs, mostly generated actually.

%description devel
 : zziplib provides read access to zipped files in a zip-archive,
 : using compression based solely on free algorithms provided by zlib.
 these are the header files needed to develop programs using zziplib.
 there are test binaries to hint usage of the library in user programs.

%description SDL_rwops-devel
 : zziplib provides read access to zipped files in a zip-archive,
 : using compression based solely on free algorithms provided by zlib.
 these are example headers and implementation along with a pkgconfig
 script that allows to easily use zziplib through SDL_rwops calls.

%prep
#'
%setup


CFLAGS="$RPM_OPT_FLAGS" \
sh configure --prefix=%{_prefix} \
             --mandir=%{_mandir} \
             --bindir=%{_bindir} \
             --libdir=%{_libdir} \
             --with-docdir=%{_docdir} \
             --disable-static --with-pic \
             --enable-sdl  TIMEOUT=9
%__make zzip64-setup

%build
%__make %{?jobs:-j%jobs}
%__make %{?jobs:-j%jobs} zzip64-build
%__make %{?jobs:-j%jobs} doc

%check
%__make check
%__make test-sdl

%install
%__rm -rf %{buildroot}
%__make zzip64-install DESTDIR=%{buildroot}
%__make install DESTDIR=%{buildroot}
%__make zzip32-postinstall DESTDIR=%{buildroot}
%__make zzip-postinstall
%__make install-doc DESTDIR=%{buildroot}
%__make install-mans DESTDIR=%{buildroot}
%__make install-sdl DESTDIR=%{buildroot}

%clean
%__rm -rf %{buildroot}

%files
      %defattr(-,root,root)
      %{_libdir}/lib*.so.*

%post -p /sbin/ldconfig 
%postun -p /sbin/ldconfig

%files doc
      %defattr(-,root,root)
      %{_datadir}/doc/*
%dir  %{_datadir}/omf/%{name}
      %{_datadir}/omf/%{name}/*

%post doc
test ! -f %_bindir/scrollkeeper-update || %_bindir/scrollkeeper-update
%postun doc
test ! -f %_bindir/scrollkeeper-update || %_bindir/scrollkeeper-update

%files devel
      %defattr(-,root,root)
%doc  ChangeLog README TODO
      %{_bindir}/*
%dir  %{_includedir}/zzip
      %{_includedir}/zzip/*
      %{_includedir}/*.h
      %{_libdir}/lib*.so
      %{_libdir}/lib*.a
      %{_libdir}/lib*.la
      %{_libdir}/pkgconfig/zzip*
      %{_datadir}/aclocal/%{name}*.m4
      %{_mandir}/man3/*

%files SDL_rwops-devel
      %defattr(-,root,root)
      %{_libdir}/pkgconfig/SDL*zzip*
%dir  %{_includedir}/SDL_rwops_zzip
      %{_includedir}/SDL_rwops_zzip/*

%changelog
* So Mar 11 2012 guidod <guidod@gmx.de> 0.13.62-1
- next version 
