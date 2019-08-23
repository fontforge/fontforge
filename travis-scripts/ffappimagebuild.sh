#!/bin/bash 
set -ex -o pipefail

SCRIPT_BASE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

APPDIR=$1
export VERSION=${2:0:7} # linuxdeployqt uses this for naming the file

if [ -z "$APPDIR" ] || [ -z "$VERSION" ]; then
    echo "Usage: `basename $0` appdir version"
    echo "  appdir is the location to the dumped location that cmake generated"
    echo "  version is the version hash of this build"
    echo
    echo "  This script does not support overwriting an existing appdir."
    exit 1
fi

echo "Starting appimage build, folder is $APPDIR with version $VERSION"

# TODO: AppStream metainfo
PYVER=$(ldd $APPDIR/usr/bin/fontforge | grep -Eom1 'python[0-9.]{3}' | head -1 | cut -c 7-)

( cd $APPDIR ; dpkg -x /var/cache/apt/archives/libpython${PYVER}-minimal*.deb . )
( cd $APPDIR ; dpkg -x /var/cache/apt/archives/libpython${PYVER}-stdlib*.deb . )

if [ ! -f linuxdeployqt.AppImage ]; then
    curl -Lo linuxdeployqt.AppImage "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
    chmod +x linuxdeployqt.AppImage
fi

if [ ! -f appstream-util.AppImage ]; then
    curl -Lo appstream-util.AppImage "https://github.com/fontforge/debugfonts/releases/download/r1/appstream-util-ef4c8e9-x86_64.AppImage"
    chmod +x appstream-util.AppImage
fi

./appstream-util.AppImage validate-strict $APPDIR/usr/share/metainfo/org.fontforge.FontForge.appdata.xml

./linuxdeployqt.AppImage $APPDIR/usr/share/applications/*.desktop -bundle-non-qt-libs #-unsupported-allow-new-glibc
# Manually invoke appimagetool so that the custom AppRun stays intact
./linuxdeployqt.AppImage --appimage-extract

export PATH=$(readlink -f ./squashfs-root/usr/bin):$PATH
rm $APPDIR/AppRun
install -m 755 $SCRIPT_BASE/../Packaging/AppDir/AppRun $APPDIR/AppRun # custom AppRun
./squashfs-root/usr/bin/appimagetool -g $APPDIR/
find $APPDIR -executable -type f -exec ldd {} \; | grep " => /usr" | cut -d " " -f 2-3 | sort | uniq

mv FontForge*.AppImage FontForge-$(date +%Y-%m-%d)-$VERSION-x86_64.AppImage

if [ ! -z "$CI" ]; then
    # Update the bintray descriptor... sigh. If this fails, then oh well, no bintray
    echo "Updating the bintray descriptor..."
    BINTRAY_DESCRIPTOR=$SCRIPT_BASE/bintray_descriptor.json
    sed -i "s/ciXXXX/$(date +appimage-ci-%Y-%m-%d)/g" $BINTRAY_DESCRIPTOR || true
    sed -i "s/releaseXXXX/$(date +%Y-%m-%d)/g" $BINTRAY_DESCRIPTOR || true
    echo "Bintray descriptor:"
    cat $BINTRAY_DESCRIPTOR
fi
