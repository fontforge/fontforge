#!/bin/bash

echo "... updating local git repo from github ... "
cd /usr/local/src/github-fontforge/fontforge
git pull
cd -

PORFILE=Portfile
NEWREV=`date '+%s'`
sed -i -e "s/revision        .*/revision        $NEWREV/g" $PORFILE
#sed -i -e "s/fetch.type/## fetch.type/g" $PORFILE
#sed -i -e "s/git.url/## git.url/g" $PORFILE
#sed -i -e "s/git.branch/## git.branch/g" $PORFILE
sed -i -e "s/### \(.*\)#MIQ/\1/g" $PORFILE

sudo port clean fontforge
port clean fontforge
sudo rm -f /opt/local/var/macports/distfiles/fontforge/2.0.0_beta1/fontforge-2.0.0_beta1.tar.gz

# export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:/opt/local/home/ben/bdwgc/lib/pkgconfig"

echo "... starting build ... "
sudo port -v -k install fontforge
