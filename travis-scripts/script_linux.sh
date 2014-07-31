#!/bin/bash
set -ev

./bootstrap
PYTHON=python3 ./configure --with-freetype-source=../freetype-2.5.0.1
make V=1
sudo make install
fontforge -version
make DISTCHECK_CONFIGURE_FLAGS="PYTHON=python3 UPDATE_MIME_DATABASE=/bin/true UPDATE_DESKTOP_DATABASE=/bin/true" distcheck
