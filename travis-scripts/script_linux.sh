#!/bin/bash
. ./travis-scripts/common.sh
set -ev

export LIBRARY_PATH=/usr/local/lib
export LD_LIBRARY_PATH=/usr/local/lib
export PYTHON=python3

# Travis is weird and installs many versions of Python in /opt.
# pkg-config can't play well with this and neither does AM_PATH_PYTHON
# set PYTHON_CFLAGS and PYTHON_LIBS manually to avoid pkg-config
# Then update LD_LIBRARY_PATH so that libpython can be loaded
# Then set PYTHONPATH to include wherever AM_PATH_PYTHON decides
# to dump the fontforge DLLs to.
PYBIN=$(dirname `which $PYTHON`)
PYLIB=$(dirname $PYBIN)/lib
export PATH=$PYBIN:$PATH
export LD_LIBRARY_PATH=$PYLIB:$LD_LIBRARY_PATH
export PYTHONPATH=$PYTHONPATH:/usr/local/lib/python$($PYTHON -c "import sys; print('{0}.{1}'.format(sys.version_info.major, sys.version_info.minor))")/site-packages
export PYTHON_CFLAGS=`python-config --cflags`
export PYTHON_LIBS="-L$PYLIB $(python-config --libs)"
#export PYTHON_PREFIX=`python-config --prefix`
#export PYTHON_EXEC_PREFIX=`python-config --exec-prefix`
#export PYTHON_LDFLAGS=`python-config --ldflags`

./bootstrap
./configure --with-freetype-source=../freetype-2.5.0.1
make V=1
sudo make install
fontforge -version
$PYTHON -v -c "import fontforge; print(fontforge.__version__, fontforge.version());"
make DISTCHECK_CONFIGURE_FLAGS="UPDATE_MIME_DATABASE=/bin/true UPDATE_DESKTOP_DATABASE=/bin/true" distcheck

date >| $TO_BIGV_OUTPUTPATH/linux-build-finish-timestamp
SYNC_TO_BIGV
