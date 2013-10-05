#!/bin/bash

BASEDIR=`pwd`
SRCDIR=`pwd`/packages
TARGETDIR=`pwd`/expanded

mkdir -p $TARGETDIR
cd $TARGETDIR
for if in $SRCDIR/*rpm;
do
  echo "expanding $if "
  rpm2cpio "$if" | cpio -id
done

cd $BASEDIR
cd $TARGETDIR/usr/i686-w64-mingw32/sys-root/mingw
pwd
chmod -R o+rx $SRCDIR/../overlay
cp -av $SRCDIR/../overlay/* .

cd $TARGETDIR/usr/i686-w64-mingw32/sys-root/mingw
rm -rf ./lib
rm -rf ./share/doc
rm -rf ./share/man


