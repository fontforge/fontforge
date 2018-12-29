#!/bin/bash 
set -ex

export APPDIR=appdir

echo "Starting appimage build, folder is $APPDIR"

mkdir -p $APPDIR
cp -rP $PREFIX $APPDIR/usr

find $APPDIR/

# TODO: AppStream metainfo
PYVER=$($PYTHON -c "import sys; print('{0}.{1}'.format(sys.version_info.major, sys.version_info.minor))")

( cd $APPDIR ; dpkg -x /var/cache/apt/archives/libpython${PYVER}-minimal*.deb . )
( cd $APPDIR ; dpkg -x /var/cache/apt/archives/libpython${PYVER}-stdlib*.deb . )
find $APPDIR/

wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod a+x linuxdeployqt-continuous-x86_64.AppImage

export VERSION=$(git rev-parse --short HEAD) # linuxdeployqt uses this for naming the file
./linuxdeployqt*.AppImage $APPDIR/usr/share/applications/*.desktop -bundle-non-qt-libs #-unsupported-allow-new-glibc
# Manually invoke appimagetool so that the custom AppRun stays intact
./linuxdeployqt*.AppImage --appimage-extract

export PATH=$(readlink -f ./squashfs-root/usr/bin):$PATH
rm $APPDIR/AppRun ; cp Packaging/AppDir/AppRun $APPDIR/AppRun ; chmod +x $APPDIR/AppRun # custom AppRun
./squashfs-root/usr/bin/appimagetool -g $APPDIR/
find $APPDIR -executable -type f -exec ldd {} \; | grep " => /usr" | cut -d " " -f 2-3 | sort | uniq

# Update the bintray descriptor... sigh. If this fails, then oh well, no bintray
echo "Updating the bintray descriptor..."
BINTRAY_DESCRIPTOR=$TRAVIS_BUILD_DIR/travis-scripts/bintray_descriptor.json
sed -i "s/ciXXXX/$(date +appimage-ci-%Y-%m-%d)/g" $BINTRAY_DESCRIPTOR || true
sed -i "s/releaseXXXX/$(date +%Y-%m-%d)/g" $BINTRAY_DESCRIPTOR || true
echo "Bintray descriptor:"
cat $BINTRAY_DESCRIPTOR

pwd
ls -lh
