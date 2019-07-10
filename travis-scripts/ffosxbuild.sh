#!/usr/bin/env bash

set -e -o pipefail
SCRIPT_BASE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
INVOKE_BASE="$(pwd)"

APPDIR=$(realpath $1)
HASH=${2:0:7}

if [ -z "$APPDIR" ]; then
    echo "Usage: `basename $0` appdir hash"
    echo "  appdir is the location to the FontForge.app that cmake generated"
    echo "  hash is the version hash of this build"
    echo
    echo "  This script does not support overwriting an existing bundle."
    exit 1
fi

builddate=`date +%Y-%m-%d`

if [ -z "$LDDX" ]; then
    LDDX="$SCRIPT_BASE/lddx"
    if [ ! -f "$LDDX" ]; then
        LDDX="$INVOKE_BASE/lddx"
        if [ ! -f "$LDDX" ]; then
            echo "Fetching lddx..."
            curl -L https://github.com/jtanx/lddx/releases/download/v0.1.0/lddx-0.1.0.tar.xz | tar -Jxf - -C "$INVOKE_BASE"
        fi
    fi
fi

# Now we bundle the Python libraries
echo "Bundling Python libraries..."

PYLIB=$(otool -L $APPDIR/Contents/Resources/opt/local/bin/fontforge | grep -i python | sed -e 's/ \(.*\)//')
PYVER=$(echo $PYLIB | rev | cut -d/ -f2 | rev)
PYTHON=python$PYVER
pycruft=$(realpath $(dirname $PYLIB)/../../..)

echo "pycruft: $pycruft"
mkdir -p $APPDIR/Contents/Frameworks
cp -a $pycruft/Python.framework $APPDIR/Contents/Frameworks
pushd $APPDIR/Contents/Frameworks/Python.framework/Versions/$PYVER/lib/$PYTHON/
rm site-packages || rm -rf site-packages
ln -s ../../../../../../Resources/opt/local/lib/$PYTHON/site-packages
popd

find "$APPDIR/Contents/Frameworks/Python.framework" -type f -name '*.pyc' | xargs rm -rf
ln -s ../Frameworks/Python.framework/Versions/$PYVER/bin/$PYTHON "$APPDIR/Contents/MacOS/FFPython"

pushd $APPDIR/Contents/Resources/opt/local
echo "Collecting and patching dependent libraries..."
$LDDX --overwrite --modify-special-paths --recursive --ignore-prefix /opt/X11 --collect lib \
    bin/ $APPDIR/Contents/Frameworks/Python.framework/ lib/
popd

# Package it up
if [ ! -z "$CI" ]; then
    echo "Creating the dmg..."
    if [[ "$PYVER" < "3" ]]; then
        dmgname=FontForge-$builddate-${HASH:0:7}-$PYTHON.app.dmg
    else
        dmgname=FontForge-$builddate-${HASH:0:7}.app.dmg
    fi

    hdiutil create -size 800m   \
        -volname   FontForge     \
        -srcfolder $APPDIR       \
        -ov        -format UDBZ  \
        $dmgname

    # Update the bintray descriptor... sigh. If this fails, then oh well, no bintray
    echo "Updating the bintray descriptor..."
    sed -i '' s/ciXXXX/$(date +mac-ci-%Y-%m-%d)/g $SCRIPT_BASE/bintray_descriptor.json || true
    sed -i '' s/releaseXXXX/$(date +%Y-%m-%d)/g $SCRIPT_BASE/bintray_descriptor.json || true
    echo "Bintray descriptor:"
    cat $SCRIPT_BASE/bintray_descriptor.json
fi