#!/bin/bash

libname=${1:?Supply library as arg1};

for if in $(otool -L $libname |grep /opt/local/lib/ | sed 's@/opt/local/lib/@@g' | sed 's/dylib .*/dylib/g' );
do
  echo $if
  install_name_tool -change /opt/local/lib/$if /Applications/FontForge.app/Contents/Resources/opt/local/lib/$if $libname

done

