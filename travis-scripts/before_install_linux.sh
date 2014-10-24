#!/bin/bash
. ./travis-scripts/common.sh
set -ev

#
# Take the collection of secure environment variables and reconstitute an SSH key
#
echo -n $id_rsa_{00..30} >> ~/.ssh/id_rsa_base64
base64 --decode --ignore-garbage ~/.ssh/id_rsa_base64 > ~/.ssh/id_rsa
chmod 600 ~/.ssh/id_rsa
echo -e "Host bigv\n\tBatchMode yes\n\tStrictHostKeyChecking no\n\tHostname fontforge.default.fontforge.uk0.bigv.io\n\tUser travisci\n\tIdentityFile ~/.ssh/id_rsa\n" >> ~/.ssh/config
# wipe them out just in case a loose 'set' or whatever happens.
for i in {00..30}; do unset id_rsa_$i; done

# test the secure env variables and ability to upload
date >| $TO_BIGV_OUTPUTPATH/linux-build-start-timestamp
SYNC_TO_BIGV


#
# install deps
#
sudo apt-get update -qq
sudo apt-get install -qq autotools-dev libjpeg-dev libtiff4-dev libpng12-dev libgif-dev libxt-dev autoconf automake libtool bzip2 libxml2-dev libuninameslist-dev libspiro-dev python3-dev libpango1.0-dev libcairo2-dev chrpath uuid-dev
