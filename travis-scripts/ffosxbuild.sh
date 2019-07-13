#!/usr/bin/env bash

set -e
BASE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
workdir=$1
hash=$2

if [ -z $workdir ]; then
    echo "Usage: `basename $0` workdir hash"
    echo "  workdir is the location to the prefix that FontForge was installed to."
    echo "  hash is the version hash of this build"
    echo
    echo "  This script does not support overwriting an existing bundle."
    exit 1
fi

builddate=`date +%Y-%m-%d`
#outdir=FontForge-$builddate-${hash:0:7}.app
outdir=FontForge.app

if [ ! -f $BASE/lddx ]; then
    echo "Fetching lddx..."
    wget https://github.com/jtanx/lddx/releases/download/v0.0.1/lddx-0.0.1.tar.bz2 -O - | tar -jxf - -C $BASE
fi

workdir=$(realpath $workdir)
outdir=$(realpath $outdir)

echo "Taking a dump into $outdir..."

cp -r $workdir/share/fontforge/osx/FontForge.app $outdir
mkdir -p $outdir/Contents/Resources/opt/local/lib
cp -r $workdir/bin $outdir/Contents/Resources/opt/local/
cp -r $workdir/share $outdir/Contents/Resources/opt/local/
rm -r $outdir/Contents/Resources/opt/local/share/fontforge/osx
cp -r $workdir/lib/python2.7 $outdir/Contents/Resources/opt/local/lib/python2.7

pushd $outdir/Contents/MacOS
ln -s ../Frameworks/Python.framework/Versions/Current/bin/python FFPython
popd

# Now we bundle the Python libraries
echo "Bundling Python libraries..."
otool -L $workdir/bin/fontforge

pylib=$(otool -L $workdir/bin/fontforge | grep -i python | sed -e 's/ \(.*\)//')
pycruft=$(realpath $(dirname $pylib)/../../..)
echo "pycruft: $pycruft"
mkdir -p $outdir/Contents/Frameworks
cp -av $pycruft/Python.framework $outdir/Contents/Frameworks
pushd $outdir/Contents/Frameworks/Python.framework/Versions/2.7/lib/python2.7/
rm site-packages || rm -rf site-packages
ln -s ../../../../../../Resources/opt/local/lib/python2.7/site-packages
popd
pushd $outdir/Contents/Frameworks/Python.framework && \
    find . -type f -name '*.pyc' | xargs rm -rfv && popd
#pycruft=/usr/local/opt/python/Frameworks/Python.framework/Versions/2.7/lib/python2.7

pushd $outdir/Contents/Resources/opt/local
echo "Collecting and patching dependent libraries..."
$BASE/lddx --modify-special-paths --recursive --ignore-prefix /opt/X11 --collect lib \
    bin/ $outdir/Contents/Frameworks/Python.framework/
popd

echo "Writing package metadata..."
sed -i -e "s|Gdraw.ScreenWidthCentimeters:.*|Gdraw.ScreenWidthCentimeters: 34|g" \
       "$outdir/Contents/Resources/opt/local/share/fontforge/pixmaps/resources"
sed -i -e "s|Gdraw.GMenu.MacIcons:.*|Gdraw.GMenu.MacIcons: True|g" \
       "$outdir/Contents/Resources/opt/local/share/fontforge/pixmaps/resources"

#
# This block updates the metadata that finder will show for the app.
#
osxmetadata_file=$outdir/Contents/Resources/English.lproj/InfoPlist.string

FONTFORGE_MODTIME_STR=$(grep 'FONTFORGE_MODTIME_STR ' $workdir/include/fontforge/fontforge-config.h | head -1 | sed -E 's/^.* "?([^"]+)"?/\1/g')
FONTFORGE_VERSION=$(grep 'FONTFORGE_VERSION ' $workdir/include/fontforge/fontforge-config.h | head -1 | sed -E 's/^.* "?([^"]+)"?/\1/g')
FONTFORGE_GIT_VERSION=$(grep 'FONTFORGE_GIT_VERSION ' $workdir/include/fontforge/fontforge-config.h | head -1 | sed -E 's/^.* "?([^"]+)"?/\1/g')
echo "got: modtime     = $FONTFORGE_MODTIME_STR"
echo "got: versiondata = $FONTFORGE_VERSION"
echo "got: git ver     = $FONTFORGE_GIT_VERSION"

CFBundleShortVersionString="Version $FONTFORGE_VERSION"
CFBundleGetInfoString="FontForge version $FONTFORGE_VERSION based on sources from $FONTFORGE_MODTIME_STR git:$FONTFORGE_GIT_VERSION";

# Replace version strings in InfoPlist.string
sed -i -e "s/CFBundleShortVersionString.*/CFBundleShortVersionString = \"$CFBundleShortVersionString\"/g" $osxmetadata_file
sed -i -e "s/CFBundleGetInfoString.*/CFBundleGetInfoString = \"$CFBundleGetInfoString\"/g" $osxmetadata_file

# Replace version strings in Info.plist
sed -i -e "s/CFBundleShortVersionStringChangeMe/$CFBundleShortVersionString/g" $outdir/Contents/Info.plist
sed -i -e "s/CFBundleGetInfoStringChangeMe/$CFBundleGetInfoString/g" $outdir/Contents/Info.plist
sed -i -e "s/CFBundleVersionChangeMe/$FONTFORGE_VERSION/g" $outdir/Contents/Info.plist

# Package it up
dmgname=FontForge-$builddate-${hash:0:7}.app.dmg

hdiutil create -size 800m   \
   -volname   FontForge     \
   -srcfolder $outdir       \
   -ov        -format UDBZ  \
   $dmgname

# Update the bintray descriptor... sigh. If this fails, then oh well, no bintray
echo "Updating the bintray descriptor..."
sed -i '' s/ciXXXX/$(date +mac-ci-%Y-%m-%d)/g $BASE/bintray_descriptor.json || true
sed -i '' s/releaseXXXX/$(date +%Y-%m-%d)/g $BASE/bintray_descriptor.json || true
echo "Bintray descriptor:"
cat $BASE/bintray_descriptor.json

echo "Done."

