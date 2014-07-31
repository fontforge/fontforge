#!/bin/bash
set -ev

sudo apt-get update -qq
sudo apt-get install -qq autotools-dev libjpeg-dev libtiff4-dev libpng12-dev libgif-dev libxt-dev autoconf automake libtool bzip2 libxml2-dev libuninameslist-dev libspiro-dev python3-dev libpango1.0-dev libcairo2-dev chrpath uuid-dev
