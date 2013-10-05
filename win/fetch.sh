#!/bin/bash

mv packages /tmp/packages-$(date -Iseconds)
mkdir -p packages
cp -av yum-config packages/ 
cd ./packages

rm -f ./mingw32-fontforge*rpm

yumdownloader -c yum-config/yum.conf  \
 mingw32-fontconfig             \
 mingw32-freetype               \
 mingw32-glib2                  \
 mingw32-libcairo2              \
 mingw32-libpng                 \
 mingw32-libstdc++              \
 mingw32-libtiff                \
 mingw32-libxml2                \
 mingw32-zlib                   \
 mingw32-libgcc                 \
 mingw32-pixman                 \
 mingw32-libintl                \
 mingw32-libffi                 \
 mingw32-libjpeg                \
 mingw32-libharfbuzz            \
 mingw32-gdk-pixbuf             \
 mingw32-librsvg                \
 mingw32-gdb mingw32-libexpat   \
 xeyes                          \
 pango xproto kbproto inputproto xtrans libX11 libXt libXmu libXext \
 libXrender libXft libXau libICE libSM \
 plibc czmq zeromq spiro

yumdownloader -c yum-config/yum.conf  \
 mingw32-libicu mingw32-fontforge              

#wget http://download.opensuse.org/repositories/home:/monkeyiq:/fontforgewin/mingw32support/noarch/mingw32-fontforge-2.0.0git.1377553312-132.2.noarch.rpm


rm -f libX11*rpm
cp ../gold/libX11*rpm .
