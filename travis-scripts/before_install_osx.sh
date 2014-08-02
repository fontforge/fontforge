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

#
# this forces version 4.0.4 and 2.2.0 respectively.
pushd .
cd $( brew --prefix )
git checkout ab7f37834a28b4d6f3c584f08e5b993d8c191653 Library/Formula/zeromq.rb
git checkout 3ad14e1e3f7d0131b3eee7fb4ce38a65e22a5187 Library/Formula/czmq.rb
brew install czmq zeromq
popd


