#!/bin/bash
set -ev

LOGFILE=/tmp/travisci-osx-brewlog.txt
rm -f /Users/travis/build/fontforge/fontforge/.git/shallow
set +e
echo "brew build starting..."
brew install --verbose fontforge --HEAD --with-x 
echo "brew build complete..."
PR=$TRAVIS_PULL_REQUEST
chmod +x ./travis-scripts/create-osx-app-bundle-homebrew.sh
./travis-scripts/create-osx-app-bundle-homebrew.sh >/tmp/bundle-output-${PR}.log 2>&1
ls -lh /tmp/bundle-output-${PR}.log
echo "grabbing logs to server for inspection..."
scp /tmp/bundle-output*.log ~/FontForge.app.zip bigv:/tmp/
set -e
fontforge -version


