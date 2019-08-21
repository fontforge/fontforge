Name:        fontforge
Version:     20051201
Release:     1
Summary:     An outline font editor
License:     BSD
Group:       Applications/Publishing
Icon:        ffanvil32.gif
Source0:     http://fontforge.sourceforge.net/fontforge_full-%{version}.tgz
Source1:     fontforge.desktop
Url:         http://fontforge.sourceforge.net/
Vendor:      George Williams <gww@silcom.com>, Scott Pakin <pakin@uiuc.edu>
Prefix:      /usr/local
BuildRoot:   /var/tmp/%{name}-%{version}
BuildPreReq: libpng-devel, libungif-devel, libuninameslist, libxml2

%description
FontForge allows you to edit outline and bitmap fonts.  You can create
new ones or modify old ones.  It is also a font format converter and
can convert among PostScript (ASCII & binary Type 1, some Type 3s,
some Type 0s), TrueType, OpenType (Type2), CID-keyed, SVG, CFF and
multiple-master fonts.

%prep
%setup -T -b 0 -n fontforge

%build
%configure
#configure --with-regular-link --with-freetype-bytecode=no
make

%install
%makeinstall

%post
ldconfig

%postun
ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_bindir}/fontforge
%{_libdir}/libgunicode.*
%{_libdir}/libgdraw.*
%{_libdir}/pkgconfig/fontforge.pc
%{_datadir}/fontforge
%{_mandir}/man1/fontforge.1*
%{_mandir}/man1/sfddiff.1*
%{_bindir}/../share/locale/*/LC_MESSAGES/FontForge.mo
%doc LICENSE README-Unix.html

%changelog
* Wed Nov 27 2002 Scott Pakin <pakin@uiuc.edu>
- Corrected inclusion of shared libraries and cleaned up some other things

* Fri Nov  2 2001 George Williams <gww@silcom.com>
- Went from three source packages down to just one which includes the others

* Thu May 10 2001 George Williams <gww@silcom.com>
- My first attempt at rpm, updated to 10 May sources

* Tue May 01 2001 Scott Pakin <pakin@uiuc.edu>
- Removed (unused) dynamic library files

* Sun Apr 29 2001 Scott Pakin <pakin@uiuc.edu>
- Upgraded from 220401 to 280401.

* Tue Apr 24 2001 Scott Pakin <pakin@uiuc.edu>
- Upgraded from 190401 to 220401.

* Fri Apr 20 2001 Scott Pakin <pakin@uiuc.edu>
- Upgraded from 020401 to 190401.

* Tue Apr 10 2001 Scott Pakin <pakin@uiuc.edu>
- Upgraded from 210301 to 020401.

* Thu Mar 22 2001 Scott Pakin <pakin@uiuc.edu>
- Initial release
