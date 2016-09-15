#!/bin/bash
set -ev

export LIBRARY_PATH=/usr/local/lib
export LD_LIBRARY_PATH=/usr/local/lib
pushd ..
wget 'http://download.zeromq.org/zeromq-4.0.4.tar.gz' -O - | tar -zxf -
pushd zeromq-4.0.4 && ./configure && make && sudo make install && popd
wget 'http://download.zeromq.org/czmq-2.2.0.tar.gz' -O - |tar -zxf -
pushd czmq-2.2.0 && ./configure && make && sudo make install && popd 
wget 'https://bitbucket.org/sortsmill/libunicodenames/downloads/libunicodenames-1.1.0_beta1.tar.xz' -O - | tar -Jxf -
pushd libunicodenames-1.1.0_beta1 && ./configure && make && sudo make install && popd
wget --tries 1 'http://download.savannah.gnu.org/releases/freetype/freetype-2.5.0.1.tar.gz' || \
    wget 'https://sourceforge.net/projects/freetype/files/freetype2/2.5.0/freetype-2.5.0.1.tar.gz'
tar -zxf freetype-2.5.0.1.tar.gz
pushd freetype-2.5.0.1 && ./configure && make && sudo make install && popd
popd
