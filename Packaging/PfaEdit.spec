Name:        pfaedit
Version:     010512
Release:     1
Summary:     A PostScript font editor
Copyright:   BSD
Group:       Applications/Publishing
Icon:        fficon.gif
Source0:     http://pfaedit.sourceforge.net/pfaedit_src-%{version}.tgz
Source1:     http://pfaedit.sourceforge.net/libgunicode-070501.tgz
Source2:     http://pfaedit.sourceforge.net/libgdraw-100501.tgz
Url:         http://pfaedit.sourceforge.net/
Vendor:      George Williams <gww@silcom.com>, Scott Pakin <pakin@uiuc.edu>
Prefix:      /usr
BuildRoot:   /var/tmp/%{name}-%{version}
BuildPreReq: libjpeg-devel, libtiff-devel, libpng-devel, libungif-devel

%description
PfaEdit allows you to edit outline and bitmap fonts.  You can create
new ones or modify old ones.  It is also a font format converter and
can convert among PostScript (ASCII & binary Type 1, some Type 3s,
some Type 0s), TrueType, and OpenType (Type2).

%prep
%setup -T -b 0 -n pfaedit
%setup -T -b 1 -n pfaedit -D
%setup -T -b 2 -n pfaedit -D

%build
CFLAGS="$RPM_OPT_FLAGS"
%configure
make

%install
mkdir -p ${RPM_BUILD_ROOT}%{_bindir}
mkdir -p ${RPM_BUILD_ROOT}%{_mandir}/man1
rm -rf ${RPM_BUILD_ROOT}%{_libdir}
mkdir -p ${RPM_BUILD_ROOT}%{_libdir}
touch ${RPM_BUILD_ROOT}%{_libdir}/libgdraw.so.1
touch ${RPM_BUILD_ROOT}%{_libdir}/libgunicode.so.1
%makeinstall

#%post
#ldconfig

#%postun
#ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_bindir}/pfaedit
%{_mandir}/man1/pfaedit.1*
%doc LICENSE README

%changelog
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
