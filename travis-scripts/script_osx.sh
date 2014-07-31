#!/bin/bash
set -ev

rm -f /Users/travis/build/fontforge/fontforge/.git/shallow
brew install --verbose fontforge --HEAD #--with-x
fontforge -version
