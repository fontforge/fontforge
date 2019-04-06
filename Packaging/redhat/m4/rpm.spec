dnl This gets processed by m4.
dnl Macros:
dnl   PACKAGE_NAME
dnl   PACKAGE_VERSION
dnl   BINARY
dnl   BINARY_TARGET
dnl   PREFIX
dnl   SOURCE_TARBALL_NAME

Name: PACKAGE_NAME
Version: PACKAGE_VERSION
Release: 1
License: GPLv3
Vendor: FontForge
URL: http://fontforge.github.io/
Packager: Package Maintainer <releases@fontforge.org>
Summary: Computer typeface editor
Source: SOURCE_TARBALL_NAME
Requires: libgdraw4 = PACKAGE_VERSION`', libfontforge1 = PACKAGE_VERSION`', fontforge-common = PACKAGE_VERSION`'

%define _unpackaged_files_terminate_build 0

%prep
%setup

%description
FontForge provides a complete suite of tools for producing high-quality typefaces and supports a wide range of file formats including TrueType, OpenType, PostScript, and U. F. O..

%build
./bootstrap;
./configure --prefix=`'PREFIX`' --libdir=`'PREFIX`'/lib --mandir=`'PREFIX`'/share/man --with-regular-link --enable-devicetables --enable-type3 --enable-pyextension;
make;

%install
make install DESTDIR="$RPM_BUILD_ROOT";
find "$RPM_BUILD_ROOT"/`'PREFIX`'/lib -name "*.la" -delete;
chrpath -d "$RPM_BUILD_ROOT"/`'PREFIX`'/bin/fontforge;
chmod 0644 "$RPM_BUILD_ROOT"/`'PREFIX`'/lib/lib*.so.*;
(cd "$RPM_BUILD_ROOT"/`'PREFIX`'/lib; for i in python*; do if [ -e $i/site-packages ] ; then mv $i/site-packages $i/dist-packages; fi ; done)
mkdir -p "$RPM_BUILD_ROOT"`'PREFIX`'/share/pixmaps ;
cp -pRP desktop/icons/* "$RPM_BUILD_ROOT"/`'PREFIX`'/share/icons/ ;
cp -pRP Packaging/debian/cp-src/fontforge.xpm "$RPM_BUILD_ROOT"/`'PREFIX`'/share/pixmaps/ ;
cp -pRP desktop/fontforge.desktop "$RPM_BUILD_ROOT"/`'PREFIX`'/share/applications/ ;

%pre

%post

%preun

%postun

%clean

%files
%defattr(-,root,root)
%attr(0755,root,root) `'PREFIX`'/bin/fontforge
%attr(0755,root,root) `'PREFIX`'/bin/fontimage
%attr(0755,root,root) `'PREFIX`'/bin/fontlint
%attr(0755,root,root) `'PREFIX`'/bin/sfddiff
%attr(0644,root,root) `'PREFIX`'/share/applications/fontforge.desktop
%attr(0644,root,root) `'PREFIX`'/share/man/man1/fontforge.1.gz
%attr(0644,root,root) `'PREFIX`'/share/man/man1/fontimage.1.gz
%attr(0644,root,root) `'PREFIX`'/share/man/man1/fontlint.1.gz
%attr(0644,root,root) `'PREFIX`'/share/man/man1/sfddiff.1.gz
%attr(0644,root,root) `'PREFIX`'/share/mime/packages/fontforge.xml

%package -n libfontforge1
Requires: libgcc1
Summary: FontForge libraries

%description -n libfontforge1
FontForge provides a complete suite of tools for producing high-quality typefaces and supports a wide range of file formats including TrueType, OpenType, PostScript, and U. F. O..

%pre -n libfontforge1

%post -n libfontforge1

%preun -n libfontforge1

%postun -n libfontforge1

%files -n libfontforge1
%defattr(-,root,root)
`'PREFIX`'/lib/libgunicode.so
`'PREFIX`'/lib/libfontforge.so
`'PREFIX`'/lib/libfontforgeexe.so
`'PREFIX`'/lib/libgutils.so
`'PREFIX`'/lib/libgunicode.so.*
`'PREFIX`'/lib/libfontforge.so.*
`'PREFIX`'/lib/libfontforgeexe.so.*
`'PREFIX`'/lib/libgutils.so.*

%package -n fontforge-devel
Requires: libfontforge1 = PACKAGE_VERSION`', libgdraw4 = PACKAGE_VERSION`'
Summary: FontForge development headers

%description -n fontforge-devel
FontForge provides a complete suite of tools for producing high-quality typefaces and supports a wide range of file formats including TrueType, OpenType, PostScript, and U. F. O..

%pre -n fontforge-devel

%post -n fontforge-devel

%preun -n fontforge-devel

%postun -n fontforge-devel

%files -n fontforge-devel
%defattr(-,root,root)
%attr(0644,root,root) `'PREFIX`'/include/fontforge/*.h
%attr(0644,root,root) `'PREFIX`'/lib/pkgconfig/*.pc

%package -n libgdraw4
Requires: libgcc1
Summary: gdraw libraries

%description -n libgdraw4
FontForge provides a complete suite of tools for producing high-quality typefaces and supports a wide range of file formats including TrueType, OpenType, PostScript, and U. F. O..

%pre -n libgdraw4

%post -n libgdraw4

%preun -n libgdraw4

%postun -n libgdraw4

%files -n libgdraw4
%defattr(-,root,root)
`'PREFIX`'/lib/libgdraw.so
`'PREFIX`'/lib/libgdraw.so.*

%package -n python-fontforge
Requires: libfontforge1 = PACKAGE_VERSION`'
Summary: Python bindings

%description -n python-fontforge
FontForge provides a complete suite of tools for producing high-quality typefaces and supports a wide range of file formats including TrueType, OpenType, PostScript, and U. F. O..

%pre -n python-fontforge

%post -n python-fontforge

%preun -n python-fontforge

%postun -n python-fontforge

%files -n python-fontforge
%defattr(-,root,root)
%attr(0644,root,root) `'PREFIX`'/lib*/python*/[a-z]*-packages/psMat.so
%attr(0644,root,root) `'PREFIX`'/lib*/python*/[a-z]*-packages/fontforge.so
%attr(0644,root,root) `'PREFIX`'/share/fontforge/python/excepthook.py

%package common
Summary: Platform-independent support files

%description common
FontForge provides a complete suite of tools for producing high-quality typefaces and supports a wide range of file formats including TrueType, OpenType, PostScript, and U. F. O..

%pre common

%post common

%preun common

%postun common

%files common
%defattr(-,root,root)
%attr(0644,root,root) `'PREFIX`'/share/locale/*/LC_MESSAGES/FontForge.mo
%attr(0644,root,root) `'PREFIX`'/share/icons/*x*/apps/fontforge.png
%attr(0644,root,root) `'PREFIX`'/share/icons/src/icon-*x*-apps-fontforge.svg
%attr(0644,root,root) `'PREFIX`'/share/icons/scalable/apps/fontforge.svg
%attr(0644,root,root) `'PREFIX`'/share/pixmaps/fontforge.xpm

%package doc
Summary: Documentation

%description doc
FontForge provides a complete suite of tools for producing high-quality typefaces and supports a wide range of file formats including TrueType, OpenType, PostScript, and U. F. O..

%pre doc

%post doc

%preun doc

%postun doc

%files common
%defattr(-,root,root)
%attr(0644,root,root) `'PREFIX`'/share/doc/fontforge/*.*
%attr(0644,root,root) `'PREFIX`'/share/doc/fontforge/flags/*.*

