#!/bin/bash

libname=${1:?Supply library as arg1};

for if in $(otool -L $libname |grep /opt/local/lib/ | sed 's@/opt/local/lib/@@g' | sed 's/dylib .*/dylib/g' );
do
  echo $if
  install_name_tool -change /opt/local/lib/$if /Applications/FontForge.app/Contents/Resources/opt/local/lib/$if $libname
  install_name_tool -change /usr/local/lib/$if /Applications/FontForge.app/Contents/Resources/opt/local/lib/$if $libname

done


for if in $(otool -L $libname |grep /usr/local/lib/ | sed 's@/usr/local/lib/@@g' | sed 's/dylib .*/dylib/g' );
do
  echo $if
  install_name_tool -change /opt/local/lib/$if /Applications/FontForge.app/Contents/Resources/opt/local/lib/$if $libname
  install_name_tool -change /usr/local/lib/$if /Applications/FontForge.app/Contents/Resources/opt/local/lib/$if $libname

done

install_name_tool -change /usr/local/opt/gettext/lib/libintl.8.dylib /Applications/FontForge.app/Contents/Resources/opt/local/lib/libintl.8.dylib $libname
