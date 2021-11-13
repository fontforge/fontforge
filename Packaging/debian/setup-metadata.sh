#!/bin/bash

# Sets up the Debian packaging metadata
# Run this from an extracted dist archive, e.g.
# tar axf fontforge-20190801.tar.xz
# cd fontforge-20190801
# Packaging/debian/setup-metadata.sh
# debuild -S -sa # To generate the source archive, or
# debuild # To generate the binaries too

set -e

BASE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION=$(grep -m1 project.*VERSION "$BASE/../../CMakeLists.txt"  | sed 's/.*VERSION \([0-9]*\).*/\1/g')

DEBIAN_DIR="$(realpath $BASE/../../debian)"

if [ -d "$DEBIAN_DIR" ]; then
    echo "$DEBIAN_DIR already exists, not touching it"
    exit 1
fi

echo "Copying debian metadata to $DEBIAN_DIR..."
cp -rv "$BASE/cp-src" "$DEBIAN_DIR"

echo

read -p "Setup debian/changelog? [Y/n] " yn
case $yn in
    [Nn]* ) exit 0 ;;
esac

read -p "Version? [$VERSION] " uv
if [ ! -z "$uv" ]; then
    VERSION="$uv"
fi

read -p "Is this an Ubuntu release? [Y/n] " yn
case $yn in
    [Nn]* ) ;;
    * )
        read -p "Ubuntu version? [xenial] " DEB_DIST_NAME
        if [ -z "$DEB_DIST_NAME" ]; then DEB_DIST_NAME=xenial; fi
        ;;
esac

if [ -z "$DEB_DIST_NAME" ]; then
    read -p "Distribution name? [unstable] " DEB_DIST_NAME
    if [ -z "$DISTNAME" ]; then
        DEB_DIST_NAME=unstable
    fi
    DEB_PRODUCT_VERSION="$VERSION"
else
    DEB_PRODUCT_VERSION="$VERSION-0ubuntu1~$DEB_DIST_NAME"
fi

read -p "Changelog message? [Release] " DEB_CHANGELOG
if [ -z "$DEB_CHANGELOG" ]; then
    DEB_CHANGELOG="Release"
fi

read -p "Packager name? [Package Maintainer] " DEB_PACKAGER_NAME
if [ -z "$DEB_PACKAGER_NAME" ]; then
    DEB_PACKAGER_NAME="Package Maintainer"
fi

read -p "Packager email? [releases@fontforge.org] " DEB_PACKAGER_EMAIL
if [ -z "$DEB_PACKAGER_EMAIL" ]; then
    DEB_PACKAGER_EMAIL="releases@fontforge.org"
fi

DEB_PACKAGE_DATE=$(date "+%a, %d %b %Y %H:%M:%S %z")

sed "s/@DEB_PRODUCT_VERSION@/$DEB_PRODUCT_VERSION/g; \
     s/@DEB_DIST_NAME@/$DEB_DIST_NAME/g; \
     s/@DEB_CHANGELOG@/$DEB_CHANGELOG/g; \
     s/@DEB_PACKAGER_NAME@/$DEB_PACKAGER_NAME/g; \
     s/@DEB_PACKAGER_EMAIL@/$DEB_PACKAGER_EMAIL/g; \
     s/@DEB_PACKAGE_DATE@/$DEB_PACKAGE_DATE/g" \
    "$BASE/changelog.in" > "$DEBIAN_DIR/changelog"

echo "Changelog written to $DEBIAN_DIR/changelog:"
cat "$DEBIAN_DIR/changelog"