#!/bin/bash 

mkdir -p $TRAVIS_BUILD_DIR/target/usr/share/applications/ # FIXME: #3372
cp desktop/fontforge.desktop $TRAVIS_BUILD_DIR/target/usr/share/applications/ # FIXME: #3372
mkdir -p $TRAVIS_BUILD_DIR/target/usr/share/icons/hicolor/ # FIXME: #3372
cp -r desktop/icons/* $TRAVIS_BUILD_DIR/target/usr/share/icons/hicolor/ ; rm -rf $TRAVIS_BUILD_DIR/target/usr/share/icons/hicolor/src # FIXME: #3372
mkdir -p $TRAVIS_BUILD_DIR/target/usr/share/fontforge/hotkeys ; cp share/default $TRAVIS_BUILD_DIR/target/usr/share/fontforge/hotkeys/ # FIXME: #3372
find .  | grep "pixmaps/resources"
mkdir -p $TRAVIS_BUILD_DIR/target/usr/share/fontforge/pixmaps ; find . -type f -name "resources" -exec cp {} $TRAVIS_BUILD_DIR/target/usr/share/fontforge/pixmaps/ \;
# TODO: AppStream metainfo
( cd $TRAVIS_BUILD_DIR/target ; dpkg -x /var/cache/apt/archives/libpython3.6-minimal*.deb . )
( cd $TRAVIS_BUILD_DIR/target ; dpkg -x /var/cache/apt/archives/libpython3.6-stdlib*.deb . )
find $TRAVIS_BUILD_DIR/target/
wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod a+x linuxdeployqt-continuous-x86_64.AppImage
export VERSION=$(git rev-parse --short HEAD) # linuxdeployqt uses this for naming the file
./linuxdeployqt*.AppImage $TRAVIS_BUILD_DIR/target/usr/share/applications/*.desktop -bundle-non-qt-libs
# Manually invoke appimagetool so that the custom AppRun stays intact
./linuxdeployqt*.AppImage --appimage-extract
export PATH=$(readlink -f ./squashfs-root/usr/bin):$PATH
rm $TRAVIS_BUILD_DIR/target/AppRun ; cp Packaging/AppDir/AppRun $TRAVIS_BUILD_DIR/target/AppRun ; chmod +x $TRAVIS_BUILD_DIR/target/AppRun # custom AppRun
./squashfs-root/usr/bin/appimagetool -g $TRAVIS_BUILD_DIR/target/
find $TRAVIS_BUILD_DIR/target -executable -type f -exec ldd {} \; | grep " => /usr" | cut -d " " -f 2-3 | sort | uniq
curl --upload-file FontForge*.AppImage https://transfer.sh/FontForge-git.$(git rev-parse --short HEAD)-x86_64.AppImage # For testing only
