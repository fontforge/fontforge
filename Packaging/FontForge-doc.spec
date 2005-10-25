Name:        FontForge-doc
Version:     020208
Release:     1
Summary:     Docs for FontForge, a PostScript font editor
License:     BSD
Group:       Documentation
#Group:       Applications/Publishing
Icon:        pfaicon.gif
Source0:     http://fontforge.sourceforge.net/fontforge_htdocs-%{version}.tgz
Url:         http://fontforge.sourceforge.net/
Vendor:      George Williams <gww@silcom.com>
Prefix:      /usr
BuildRoot:   /var/tmp/%{name}-%{version}

%description
FontForge allows you to edit outline and bitmap fonts.  You can create
new ones or modify old ones.  It is also a font format converter and
can convert among PostScript (ASCII & binary Type 1, some Type 3s,
some Type 0s), TrueType, OpenType (Type2) and CID-keyed fonts.

%prep
%setup -T -c -a 0 -n fontforge

%install
mkdir -p ${RPM_BUILD_ROOT}%{_bindir}/../share/doc/fontforge
cp *.{html,png,gif,gz,tgz,txt} ${RPM_BUILD_ROOT}%{_bindir}/../share/doc/fontforge

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_bindir}/../share/doc/fontforge/

%changelog
* Fri Feb 8 2002 George Williams <gww@silcom.com>
- made doc rpm
