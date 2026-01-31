#!/bin/bash 
set -ex -o pipefail

SCRIPT_BASE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

APPDIR=$1
export LINUXDEPLOY_OUTPUT_VERSION=${2:0:7} # linuxdeploy uses this for naming the file

if [ -z "$APPDIR" ] || [ -z "$LINUXDEPLOY_OUTPUT_VERSION" ]; then
    echo "Usage: `basename $0` appdir version"
    echo "  appdir is the location to the dumped location that cmake generated"
    echo "  version is the version hash of this build"
    echo
    echo "  This script does not support overwriting an existing appdir."
    exit 1
fi

echo "Starting appimage build, folder is $APPDIR with version $LINUXDEPLOY_OUTPUT_VERSION"

# AppImage still needs libfuse2. See https://github.com/AppImage/AppImageKit/issues/1235.
sudo apt-get install -y libfuse2

# TODO: AppStream metainfo
PYVER=$(ldd $APPDIR/usr/bin/fontforge | grep -Eom1 'python[0-9\.]+[0-9]+' | head -1 | cut -c 7-)
echo "FontForge built against Python $PYVER"

# In Ubuntu 20-22 the libpython3-minimal is not automatically included with the default Python installation.
# In Ubuntu 24.04 this doesn't seem to be necessary anymore.
( cd $APPDIR ; apt-get download -y libpython${PYVER}-minimal ; dpkg -x libpython${PYVER}-minimal*.deb . )
( cd $APPDIR ; apt-get download -y libpython${PYVER}-stdlib ; dpkg -x libpython${PYVER}-stdlib*.deb . )
"python${PYVER}" -m pip install -I --target "$APPDIR/usr/lib/python${PYVER}/dist-packages" setuptools

if [ ! -f linuxdeploy.AppImage ]; then
    curl -Lo linuxdeploy.AppImage "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod +x linuxdeploy.AppImage
fi

if [ ! -f linuxdeploy-plugin-gtk.sh ]; then
    curl -Lo linuxdeploy-plugin-gtk.sh "https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh"
    chmod +x linuxdeploy-plugin-gtk.sh
fi

if [ ! -f appstream-util.AppImage ]; then
    curl -Lo appstream-util.AppImage "https://github.com/fontforge/debugfonts/releases/download/r1/appstream-util-ef4c8e9-x86_64.AppImage"
    chmod +x appstream-util.AppImage
fi

./appstream-util.AppImage validate-strict $APPDIR/usr/share/metainfo/org.fontforge.FontForge.appdata.xml

export DEPLOY_GTK_VERSION=3

# linuxdeploy automatically detectes and uses custom AppRun
install -m 755 $SCRIPT_BASE/../../../Packaging/AppDir/AppRun $APPDIR/AppRun

./linuxdeploy.AppImage --appdir $APPDIR --plugin gtk --output appimage -i ../desktop/tango/scalable/org.fontforge.FontForge.svg -d ../desktop/org.fontforge.FontForge.desktop

# List remaining external dependencies for debug purposes
./linuxdeploy.AppImage --appimage-extract
find $APPDIR -executable -type f -exec ldd {} \; | grep " => /lib" | cut -d " " -f 2-3 | sort | uniq

mv FontForge*.AppImage FontForge-$(date +%Y-%m-%d)-$LINUXDEPLOY_OUTPUT_VERSION-x86_64.AppImage
