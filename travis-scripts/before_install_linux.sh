#!/bin/bash
set -ev

# To reconstitute the private SSH key from within the Travis-CI build (typically from 'before_script')
#echo -n $id_rsa_{00..30} >> ~/.ssh/id_rsa_base64
#base64 --decode --ignore-garbage ~/.ssh/id_rsa_base64 > ~/.ssh/id_rsa
#chmod 600 ~/.ssh/id_rsa
#echo -e "Host github.com\n\tStrictHostKeyChecking no\n" >> ~/.ssh/config


sudo apt-get update -qq
sudo apt-get install -qq autotools-dev libjpeg-dev libtiff4-dev libpng12-dev libgif-dev libxt-dev autoconf automake libtool bzip2 libxml2-dev libuninameslist-dev libspiro-dev python3-dev libpango1.0-dev libcairo2-dev chrpath uuid-dev
