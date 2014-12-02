#!/bin/bash

BASEDIR=~/fontforge-builds
TOPDIR="$BASEDIR/`date '+%Y%m'`"
DIRNAME="$TOPDIR/`date '+%d_%H%M'`"

mkdir -p $BASEDIR
mkdir -p $TOPDIR
mkdir -p $DIRNAME
cp /tmp/FontForge.app.zip $DIRNAME/
cp /tmp/FontForge.app.dmg $DIRNAME/
find $BASEDIR -type d -exec chmod o+rx {} \;
find $BASEDIR -type f -exec chmod o+r  {} \;
rsync -av  --exclude .git $BASEDIR/.  $REMOVE_RSYNC_URL

cd ~/fontforge-builds
git add $DIRNAME/FontForge.app.zip
git commit -a -m 'another day, another build...'
