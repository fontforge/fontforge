Name:        fontforge
Version:     030730
Release:     1
Summary:     A PostScript font editor
License:     BSD
Group:       Applications/Publishing
Icon:        pfaicon.gif
Source0:     http://fontforge.sourceforge.net/fontforge_full-%{version}.tgz
Url:         http://fontforge.sourceforge.net/
Vendor:      George Williams <gww@silcom.com>, Scott Pakin <pakin@uiuc.edu>
Prefix:      /usr
BuildRoot:   /var/tmp/%{name}-%{version}
BuildPreReq: libpng-devel, libungif-devel, libxml2
#BuildPreReq: libjpeg-devel, libtiff-devel, libpng-devel, libungif-devel, freetype, libxml2

%description
FontForge allows you to edit outline and bitmap fonts.  You can create
new ones or modify old ones.  It is also a font format converter and
can convert among PostScript (ASCII & binary Type 1, some Type 3s,
some Type 0s), TrueType, OpenType (Type2), CID-keyed and SVG fonts.

%prep
%setup -T -b 0 -n fontforge

%build
CFLAGS="$RPM_OPT_FLAGS"
./configure
make

%install
mkdir -p ${RPM_BUILD_ROOT}%{_bindir}
mkdir -p ${RPM_BUILD_ROOT}%{_mandir}/man1
mkdir -p ${RPM_BUILD_ROOT}%{_bindir}/../share/fontforge
rm -rf ${RPM_BUILD_ROOT}%{_libdir}
mkdir -p ${RPM_BUILD_ROOT}%{_libdir}
%makeinstall

#%post
#ldconfig

#%postun
#ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_bindir}/fontforge
%{_bindir}/../share/fontforge/
%{_mandir}/man1/fontforge.1*
%doc LICENSE README

%changelog
* Fri Nov  2 2001 George Williams <gww@silcom.com>
- went from three source packages down to just one which includes the others

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
