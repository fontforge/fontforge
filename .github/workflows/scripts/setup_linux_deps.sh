#!/bin/bash

set -eo pipefail

PYTHON=python3

sudo apt-get remove python3-pip
sudo add-apt-repository -y ppa:deadsnakes/ppa && sudo apt-get update -y
sudo apt-get install -y autoconf automake libtool gcc g++ gettext \
    libjpeg-dev libtiff5-dev libpng-dev libfreetype-dev libgif-dev \
    libx11-dev libgtk-3-dev libxml2-dev libpango1.0-dev libcairo2-dev \
    libgtkmm-3.0-dev \
    libbrotli-dev libwoff-dev ninja-build cmake lcov $PYTHON-dev $PYTHON-venv
curl https://bootstrap.pypa.io/get-pip.py | sudo $PYTHON

PREFIX=$GITHUB_WORKSPACE/target
DEPSPREFIX=$GITHUB_WORKSPACE/deps/install
echo "PREFIX=$PREFIX" >> $GITHUB_ENV
echo "DEPSPREFIX=$DEPSPREFIX" >> $GITHUB_ENV
echo "PATH=$PATH:$DEPSPREFIX/bin:$PREFIX/bin:~/.local/bin" >> $GITHUB_ENV
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$DEPSPREFIX/lib:$PREFIX/lib" >> $GITHUB_ENV
echo "PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$DEPSPREFIX/lib/pkgconfig" >> $GITHUB_ENV
echo "PYTHONPATH=$PYTHONPATH:$PREFIX/$($PYTHON "$GITHUB_WORKSPACE/repo/pyhook/get_pyhook_install_dir.py" "${DESTDIR}${PREFIX}")" >> $GITHUB_ENV

FTVER=`dpkg -s libfreetype-dev | perl -ne 'print $1 if /^Version: (\d+(?:\.\d+)+)/'`
SFFTVER=`echo $FTVER | perl -ne 'print $1 if /^(\d+(?:\.\d+){1,2})/'`

if [ ! -d deps/install ]; then
    echo "Custom dependencies not present - will build them"
    rm -rf deps && mkdir deps && cd deps

    git clone --depth 1 https://github.com/fontforge/libspiro
    git clone --depth 1 https://github.com/fontforge/libuninameslist
    git clone --depth 1 --branch v1.0.2 https://github.com/google/woff2

    # GitHub Actions exposes RUNNER_ARCH as uppercase (X64/ARM64); use ${var,,}
    # to lowercase it so it matches the Crowdin asset name, e.g. x64 or arm64.
    CROWDIN_ARCH="${RUNNER_ARCH,,}"
    wget -O crowdin-linux-${RUNNER_ARCH} "https://github.com/crowdin/crowdin-cli/releases/latest/download/crowdin-linux-${CROWDIN_ARCH}"

    pushd libspiro && autoreconf -fiv && ./configure --prefix=$DEPSPREFIX && make -j4 && make install && popd
    pushd libuninameslist && autoreconf -fiv && ./configure --enable-pscript --prefix=$DEPSPREFIX && make -j4 && make install && popd
    pushd woff2 && mkdir build && cd build && cmake -GNinja .. -DCMAKE_INSTALL_PREFIX=$DEPSPREFIX -DCMAKE_INSTALL_LIBDIR=lib && ninja install && popd
    chmod a+x crowdin-linux-${RUNNER_ARCH} && mv crowdin-linux-${RUNNER_ARCH} $DEPSPREFIX
fi

FREETYPE_H="$DEPSPREFIX/freetype/include/freetype/freetype.h"
CACHED_FTVER=
if [ -f "$FREETYPE_H" ]; then
    CACHED_FTVER=$(awk '
        $2 == "FREETYPE_MAJOR" { major = $3 }
        $2 == "FREETYPE_MINOR" { minor = $3 }
        $2 == "FREETYPE_PATCH" { patch = $3 }
        END {
            if (major && minor && patch) {
                printf "%s.%s.%s", major, minor, patch
            }
        }
    ' "$FREETYPE_H")
fi

if [ "$CACHED_FTVER" != "$FTVER" ]; then
    echo "FreeType source version ${CACHED_FTVER:-missing} does not match $FTVER - refreshing it"
    FT_TMP=$(mktemp -d)
    (
        cd "$FT_TMP"
        wget --tries 1 "http://download.savannah.gnu.org/releases/freetype/freetype-$FTVER.tar.gz" || \
            wget "https://sourceforge.net/projects/freetype/files/freetype2/$SFFTVER/freetype-$FTVER.tar.gz"
        tar -zxf "freetype-$FTVER.tar.gz"
    )
    rm -rf "$DEPSPREFIX/freetype"
    mv "$FT_TMP/freetype-$FTVER" "$DEPSPREFIX/freetype"
    rmdir "$FT_TMP"
fi
