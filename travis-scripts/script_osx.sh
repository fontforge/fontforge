#!/bin/bash
set -ev

#brew install --verbose fontforge --HEAD --with-x

./bootstrap


HOMEBREW_PREFIX=$(brew --prefix);
PYLIBDIR=$(${HOMEBREW_PREFIX}/bin/python -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())")
PYPKGCONFIG="$( cd "$PYLIBDIR/../../pkgconfig" && pwd )"
export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$PYPKGCONFIG"


export ZLIB_CFLAGS="-I/usr/include"
export ZLIB_LIBS="-L/usr/lib -lz"

export LDFLAGS="$LDFLAGS -lintl"

./configure --prefix=/usr/local --with-x \
            --with-pythonbinary=/usr/local/opt/python/bin/python \
            --with-static-imagelibs

fontforge -version
