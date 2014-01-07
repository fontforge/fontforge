#!/bin/bash

libname=${1:?Supply library as arg1};

for if in $(otool -L $libname |grep @execut| sed 'sA.*@execut.*lib/AAg' | sed 's/dylib .*/dylib/g' );
do
  echo $if
  install_name_tool -change @executable_path/../lib/$if /Applications/FontForge.app/Contents/Resources/opt/local/lib/$if $libname

done

