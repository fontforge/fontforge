#!/usr/bin/env bash

set -e -o pipefail
SCRIPT_BASE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
INVOKE_BASE="$(pwd)"

APPDIR=$(realpath $1)
HASH=${2:0:7}
PYTHON="$3"

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

PY_DLLS_PATH=`$PYTHON "$GITHUB_WORKSPACE/repo/pyhook/get_pyhook_install_dir.py" "" "APPLE"`

PYLIB=$(otool -L $APPDIR/Contents/Resources/opt/local/bin/fontforge | grep -i python | sed -e 's/ \(.*\)//')
PYVER=$(echo $PYLIB | rev | cut -d/ -f2 | rev)
PYTHON=python$PYVER
pycruft=$(realpath $(dirname $PYLIB)/../../..)

echo "pycruft: $pycruft"
mkdir -p $APPDIR/Contents/Frameworks
cp -a $pycruft/Python.framework $APPDIR/Contents/Frameworks
pushd $APPDIR/Contents/Frameworks/Python.framework/Versions/$PYVER/lib/$PYTHON/
rm site-packages || rm -rf site-packages
ln -s ../../../../../../Resources/opt/local/$PY_DLLS_PATH
popd

pushd $APPDIR/Contents/Resources/opt/local/$PY_DLLS_PATH
cp -Rn "$pycruft/Python.framework/Versions/$PYVER/lib/$PYTHON/site-packages/" .
popd

find "$APPDIR/Contents/Frameworks/Python.framework" -type f -name '*.pyc' | xargs rm -rf

pushd $APPDIR/Contents/Resources/opt/local
echo "Collecting and patching dependent libraries..."
$LDDX --overwrite --modify-special-paths --recursive --ignore-prefix /opt/X11 --collect lib \
    bin/ $APPDIR/Contents/Frameworks/Python.framework/ lib/
popd

mkdir -p $APPDIR/Contents/MacOS
ln -s ../Frameworks/Python.framework/Versions/$PYVER/bin/$PYTHON "$APPDIR/Contents/MacOS/FFPython"

# --- Code signing -----------------------------------------------------------
# This MUST run after lddx: its install_name_tool rewrites above invalidate the
# linker's ad-hoc signatures on the collected dylibs/modules. On Apple Silicon
# every Mach-O must carry a valid signature to load, so we re-sign everything.
#
# Defaults to ad-hoc ("-"), which makes the bundle launchable on arm64 with the
# usual Gatekeeper bypass (right-click -> Open). To produce a hardened-runtime,
# notarizable bundle instead, set CODESIGN_IDENTITY to a real
# "Developer ID Application: Name (TEAMID)" identity (see #5112); CI sets this
# automatically when signing secrets are present.
IDENTITY="${CODESIGN_IDENTITY:--}"
ENTITLEMENTS="${CODESIGN_ENTITLEMENTS:-$SCRIPT_BASE/../../../osx/entitlements.plist}"

if [ "$IDENTITY" = "-" ]; then
    echo "Ad-hoc signing the bundle..."
    LIB_SIGN=(--force --timestamp=none --sign -)
    APP_SIGN=(--force --timestamp=none --sign -)
else
    echo "Signing the bundle with identity: $IDENTITY"
    # Hardened runtime + secure timestamp are required for notarization.
    # Entitlements (incl. disable-library-validation, so the embedded Python
    # interpreter can load third-party C extensions) go on the main app only.
    LIB_SIGN=(--force --options runtime --timestamp --sign "$IDENTITY")
    APP_SIGN=(--force --options runtime --timestamp --sign "$IDENTITY" --entitlements "$ENTITLEMENTS")
fi

# 1. Sign every nested Mach-O (dylibs, Python .so modules, helper executables).
#    -type f skips the symlinks created above.
echo "Signing nested Mach-O files..."
find "$APPDIR" -type f | while read -r f; do
    if file "$f" | grep -q "Mach-O"; then
        codesign "${LIB_SIGN[@]}" "$f"
    fi
done

# 2. Sign the embedded Python framework version.
codesign "${LIB_SIGN[@]}" "$APPDIR/Contents/Frameworks/Python.framework/Versions/$PYVER"

# 3. Sign the app bundle itself last (main executable receives entitlements).
codesign "${APP_SIGN[@]}" "$APPDIR"

# Verify integrity. Do NOT use spctl here: it rejects ad-hoc signatures by
# design and would fail CI spuriously. notarytool (when enabled) is the real
# Gatekeeper-level check.
codesign --verify --strict --verbose=2 "$APPDIR"
# ---------------------------------------------------------------------------

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
fi
