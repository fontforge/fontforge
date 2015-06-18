#!/bin/bash

. ./travis-scripts/common.sh
set -ev

LOGFILE=/tmp/travisci-osx-brewlog.txt
rm -f /Users/travis/build/fontforge/fontforge/.git/shallow

set +e
mkdir -p /tmp/fontforge-source-tree
echo "brew build starting..."
brew install --verbose fontforge --HEAD --with-libspiro
echo "brew build complete..."
PR=$TRAVIS_PULL_REQUEST
chmod +x ./travis-scripts/create-osx-app-bundle-homebrew.sh
cd /tmp/fontforge-source-tree
./travis-scripts/create-osx-app-bundle-homebrew.sh >/tmp/bundle-output-${PR}.log 2>&1
ls -lh /tmp/bundle-output-${PR}.log
echo "build zip info..."
ls -lh ~/FontForge.app.zip
echo "grabbing logs to server for inspection..."
cp /tmp/bundle-output-${PR}.log $TO_BIGV_OUTPUTPATH
cp ~/FontForge.app.zip          $TO_BIGV_OUTPUTPATH
date >| $TO_BIGV_OUTPUTPATH/osx-build-end-timestamp
echo "These are the files going over"
ls -lhR $TO_BIGV_OUTPUTPATH

SYNC_TO_BIGV
set -e
fontforge -version


