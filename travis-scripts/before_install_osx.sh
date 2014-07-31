#!/bin/bash
set -ev

brew update

echo "checking system for existing fontforge.rb"
#find /usr -name "fontforge.rb"
ls -l /usr/local/Library/Formula/fontforge.rb
echo "------------------------"
#sed -i -e "s|url 'https://github.com/fontforge/fontforge.git'.*|url 'https://github.com/fontforge/fontforge.git', :branch => 'BRANCHNAME'|g" ./travis-scripts/fontforge.rb
#cat ./travis-scripts/fontforge.rb
#sed -i -e "s|BRANCHNAME|+refs/pull/${TRAVIS_PULL_REQUEST}/merge|g" ./travis-scripts/fontforge.rb

sed -i -e "s|TRAVIS_PULL_REQUEST|${TRAVIS_PULL_REQUEST}|g" ./travis-scripts/fontforge.rb
sudo cp ./travis-scripts/fontforge.rb /usr/local/Library/Formula/fontforge.rb
cat ./travis-scripts/fontforge.rb
echo "updated the brew fontforge Formula..."


pushd .
cd /tmp
wget http://xquartz.macosforge.org/downloads/SL/XQuartz-2.7.6.dmg
hdiutil attach XQuartz-2.7.6.dmg 
sudo installer -pkg /Volumes/XQuartz-*/XQuartz.pkg -target /
popd 

echo "doing an OSX before install step."
brew install cairo python libspiro fontconfig
brew link python




