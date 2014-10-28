#!/bin/bash

# A quick sanity check to show if we are working on an osx machine.
type -a brew

#
# Take the collection of secure environment variables and reconstitute an SSH key
#
for id in $(seq -f "id_rsa_%02g"  0 29)
do
  echo -n ${!id} >> ~/.ssh/id_rsa_base64
done
base64 --decode ~/.ssh/id_rsa_base64 > ~/.ssh/id_rsa
chmod 600 ~/.ssh/id_rsa
echo -e "Host bigv\n\tBatchMode yes\n\tStrictHostKeyChecking no\n\tHostname fontforge.default.fontforge.uk0.bigv.io\n\tUser travisci\n\tIdentityFile ~/.ssh/id_rsa\n" >> ~/.ssh/config
# wipe them out just in case a loose 'set' or whatever happens.
for i in {00..30}; do unset id_rsa_$i;  done
for i in {00..09}; do unset id_rsa_0$i; done

#
# now that we have the key setup, bring in the SYNC function.
#
. ./travis-scripts/common.sh
set -ev

# test the secure env variables and ability to upload
date >| $TO_BIGV_OUTPUTPATH/osx-build-start-timestamp
SYNC_TO_BIGV

type -all brew
brew update
brew config

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
# python may require a little force to install. In October 2014
# there were issues with easy_install and pip not letting the install
# step complete.
set +ev
brew install python
brew link --overwrite python
set -ev
brew install cairo libspiro fontconfig

#
# this forces version 4.0.4 and 2.2.0 respectively.
pushd .
cd $( brew --prefix )
git checkout ab7f37834a28b4d6f3c584f08e5b993d8c191653 Library/Formula/zeromq.rb
git checkout 3ad14e1e3f7d0131b3eee7fb4ce38a65e22a5187 Library/Formula/czmq.rb
brew install czmq zeromq
popd


