#!/bin/bash
. ./travis-scripts/common.sh
set -ev

export LIBRARY_PATH=/usr/local/lib
export LD_LIBRARY_PATH=/usr/local/lib
export PYTHON=python3
#For some reason on travis, the FF hook installs to this instead of the version-specific (e.g python3.2) path
export PYTHONPATH=$PYTHONPATH:/usr/local/lib/$PYTHON/dist-packages

./bootstrap
./configure --with-freetype-source=../freetype-2.5.0.1
make V=1
sudo make install
fontforge -version
$PYTHON -c "import fontforge; print(fontforge.__version__, fontforge.version());"
make DISTCHECK_CONFIGURE_FLAGS="UPDATE_MIME_DATABASE=/bin/true UPDATE_DESKTOP_DATABASE=/bin/true" distcheck

date >| $TO_BIGV_OUTPUTPATH/linux-build-finish-timestamp
SYNC_TO_BIGV
