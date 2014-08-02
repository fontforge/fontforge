#!/bin/bash
set -ev

brew update

sed -i -e "s|{TRAVIS_PULL_REQUEST}|${TRAVIS_PULL_REQUEST}|g" ./travis-scripts/fontforge.rb
sudo cp ./travis-scripts/fontforge.rb /usr/local/Library/Formula/fontforge.rb
echo "*****"
echo "*****"
echo "***** using homebrew formula fontforge.rb:"
cat ./travis-scripts/fontforge.rb
echo "*****"
echo "*****"

# optional: can cut out X this block while testing git stuff
pushd .
cd /tmp
wget http://xquartz.macosforge.org/downloads/SL/XQuartz-2.7.6.dmg
hdiutil attach XQuartz-2.7.6.dmg 
sudo installer -pkg /Volumes/XQuartz-*/XQuartz.pkg -target /
popd 

echo "doing an OSX before install step."
brew install cairo python libspiro fontconfig
brew link python




