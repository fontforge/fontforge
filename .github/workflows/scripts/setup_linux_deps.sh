#!/bin/bash

set -eo pipefail

sudo apt-get remove python3-pip
sudo add-apt-repository -y ppa:deadsnakes/ppa && sudo apt-get update -y
sudo apt-get install -y autoconf automake libtool gcc g++ gettext \
    libjpeg-dev libtiff5-dev libpng-dev libfreetype6-dev libgif-dev \
    libx11-dev libgtk-3-dev libxml2-dev libpango1.0-dev libcairo2-dev \
    libbrotli-dev libwoff-dev ninja-build cmake lcov $PYTHON-dev $PYTHON-venv
curl https://bootstrap.pypa.io/get-pip.py | sudo $PYTHON

PREFIX=$GITHUB_WORKSPACE/target
DEPSPREFIX=$GITHUB_WORKSPACE/deps/install
echo "PREFIX=$PREFIX" >> $GITHUB_ENV
echo "DEPSPREFIX=$DEPSPREFIX" >> $GITHUB_ENV
echo "PATH=$PATH:$DEPSPREFIX/bin:$PREFIX/bin:~/.local/bin" >> $GITHUB_ENV
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$DEPSPREFIX/lib:$PREFIX/lib" >> $GITHUB_ENV
echo "PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$DEPSPREFIX/lib/pkgconfig" >> $GITHUB_ENV
echo "PYTHONPATH=$PYTHONPATH:$PREFIX/$($PYTHON -c "import distutils.sysconfig as sc; print(sc.get_python_lib(prefix='', plat_specific=True,standard_lib=False))")" >> $GITHUB_ENV

if [ ! -d deps/install ]; then
    echo "Custom dependencies not present - will build them"
    rm -rf deps && mkdir deps && cd deps

    FTVER=`dpkg -s libfreetype6-dev | perl -ne 'print $1 if /^Version: (\d+(?:\.\d+)+)/'`
    SFFTVER=`echo $FTVER | perl -ne 'print $1 if /^(\d+(?:\.\d+){1,2})/'`

    git clone --depth 1 https://github.com/fontforge/libspiro
    git clone --depth 1 https://github.com/fontforge/libuninameslist
    git clone --depth 1 --branch v1.0.2 https://github.com/google/woff2
    wget --tries 1 "http://download.savannah.gnu.org/releases/freetype/freetype-$FTVER.tar.gz" || \
        wget "https://sourceforge.net/projects/freetype/files/freetype2/$SFFTVER/freetype-$FTVER.tar.gz"
    wget https://downloads.crowdin.com/cli/v2/crowdin-cli.zip

    pushd libspiro && autoreconf -fiv && ./configure --prefix=$DEPSPREFIX && make -j4 && make install && popd
    pushd libuninameslist && autoreconf -fiv && ./configure --enable-pscript --prefix=$DEPSPREFIX && make -j4 && make install && popd
    pushd woff2 && mkdir build && cd build && cmake -GNinja .. -DCMAKE_INSTALL_PREFIX=$DEPSPREFIX -DCMAKE_INSTALL_LIBDIR=lib && ninja install && popd
    tar -zxf freetype-$FTVER.tar.gz && mv freetype-$FTVER $DEPSPREFIX/freetype
    mkdir crowdin && pushd crowdin && unzip ../crowdin-cli.zip && mv */* . && mv crowdin-cli.jar $DEPSPREFIX && popd
fi