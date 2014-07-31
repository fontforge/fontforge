#!/bin/bash
set -ev

echo "doing an OSX before install step."
brew update
brew install cairo python libspiro fontconfig
echo "checking system for existing fontforge.rb"
find /usr -name "fontforge.rb"
echo "------------------------"
cp ./travis-scripts/fontforge.rb /usr/local/Library/Formula/fontforge.rb
echo "updated the brew fontforge Formula..."
