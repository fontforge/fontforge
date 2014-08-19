#!/bin/bash
set -ev

LOGFILE=/tmp/travisci-osx-brewlog.txt
rm -f /Users/travis/build/fontforge/fontforge/.git/shallow
brew install --verbose fontforge --HEAD --with-x | tee $LOGFILE
scp $LOGFILE ~/FontForge.app.zip bigv:/tmp/
fontforge -version


