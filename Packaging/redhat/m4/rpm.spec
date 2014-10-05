
Name: fontforge
Version: 20140813.234.g392958d
Release: 1
License: GPLv3
Vendor: FontForge
URL: http://fontforge.github.io/
Packager: Package Maintainer <releases@fontforge.org>
Summary: Computer typeface editor
Source: fontforge-20140813.234.g392958d.tar.gz
Requires: libgdraw4 = 20140813.234.g392958d, libfontforge1 = 20140813.234.g392958d, fontforge-common = 20140813.234.g392958d

%define _unpackaged_files_terminate_build 0

%prep
%setup

%description
FontForge provides a complete suite of tools for producing high-quality typefaces and supports a wide range of file formats including TrueType, OpenType, PostScript, and U. F. O..

%build
./bootstrap;
./configure --prefix=/usr --libdir=/usr/lib --mandir=/usr/share/man --with-regular-link --enable-devicetables --enable-type3 --enable-pyextension;
make;

%install
make install DESTDIR="$RPM_BUILD_ROOT";
find "$RPM_BUILD_ROOT"//usr/lib -name "*.la" -delete;
chrpath -d "$RPM_BUILD_ROOT"//usr/bin/fontforge;
chmod 0644 "$RPM_BUILD_ROOT"//usr/lib/lib*.so.*;
(cd "$RPM_BUILD_ROOT"//usr/lib; for i in python*; do if [ -e $i/site-packages ] ; then mv $i/site-packages $i/dist-packages; fi ; done)
mkdir -p "$RPM_BUILD_ROOT"/usr/share/pixmaps ;
cp -pRP desktop/icons/* "$RPM_BUILD_ROOT"//usr/share/icons/ ;
cp -pRP Packaging/debian/cp-src/fontforge.xpm "$RPM_BUILD_ROOT"//usr/share/pixmaps/ ;
cp -pRP desktop/fontforge.desktop "$RPM_BUILD_ROOT"//usr/share/applications/ ;

%pre

%post

%preun

%postun

%clean

%files
%defattr(-,root,root)
%attr(0755,root,root) /usr/bin/fontforge
%attr(0755,root,root) /usr/bin/fontimage
%attr(0755,root,root) /usr/bin/fontlint
%attr(0755,root,root) /usr/bin/sfddiff
%attr(0644,root,root) /usr/share/applications/fontforge.desktop
%attr(0644,root,root) /usr/share/man/man1/fontforge.1.gz
%attr(0644,root,root) /usr/share/man/man1/fontimage.1.gz
%attr(0644,root,root) /usr/share/man/man1/fontlint.1.gz
%attr(0644,root,root) /usr/share/man/man1/sfddiff.1.gz
%attr(0644,root,root) /usr/share/mime/packages/fontforge.xml

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
/usr/lib/libgioftp.so
/usr/lib/libgunicode.so
/usr/lib/libfontforge.so
/usr/lib/libfontforgeexe.so
/usr/lib/libgutils.so
/usr/lib/libgioftp.so.*
/usr/lib/libgunicode.so.*
/usr/lib/libfontforge.so.*
/usr/lib/libfontforgeexe.so.*
/usr/lib/libgutils.so.*

%package -n fontforge-devel
Requires: libfontforge1 = 20140813.234.g392958d, libgdraw4 = 20140813.234.g392958d
Summary: FontForge development headers

%description -n fontforge-devel
FontForge provides a complete suite of tools for producing high-quality typefaces and supports a wide range of file formats including TrueType, OpenType, PostScript, and U. F. O..

%pre -n fontforge-devel

%post -n fontforge-devel

%preun -n fontforge-devel

%postun -n fontforge-devel

%files -n fontforge-devel
%defattr(-,root,root)
%attr(0644,root,root) /usr/include/fontforge/*.h
%attr(0644,root,root) /usr/lib/pkgconfig/*.pc

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
/usr/lib/libgdraw.so
/usr/lib/libgdraw.so.*

%package -n python-fontforge
Requires: libfontforge1 = 20140813.234.g392958d
Summary: Python bindings

%description -n python-fontforge
FontForge provides a complete suite of tools for producing high-quality typefaces and supports a wide range of file formats including TrueType, OpenType, PostScript, and U. F. O..

%pre -n python-fontforge

%post -n python-fontforge

%preun -n python-fontforge

%postun -n python-fontforge

%files -n python-fontforge
%defattr(-,root,root)
%attr(0644,root,root) /usr/lib*/python*/[a-z]*-packages/psMat.so
%attr(0644,root,root) /usr/lib*/python*/[a-z]*-packages/fontforge.so
%attr(0644,root,root) /usr/share/fontforge/python/excepthook.py

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
%attr(0644,root,root) /usr/share/locale/*/LC_MESSAGES/FontForge.mo
%attr(0644,root,root) /usr/share/icons/*x*/apps/fontforge.png
%attr(0644,root,root) /usr/share/icons/src/icon-*x*-apps-fontforge.svg
%attr(0644,root,root) /usr/share/icons/scalable/apps/fontforge.svg
%attr(0644,root,root) /usr/share/pixmaps/fontforge.xpm

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
%attr(0644,root,root) /usr/share/doc/fontforge/*.*
%attr(0644,root,root) /usr/share/doc/fontforge/flags/*.*

