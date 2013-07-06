#!/bin/bash

ORIGDIR=`pwd`
TEMPDIR=/tmp/fontforge-app-bundle
rm -rf /tmp/fontforge-app-bundle
mkdir $TEMPDIR

bundle_res=$TEMPDIR/FontForge.app/Contents/Resources
bundle_bin="$bundle_res/opt/local/bin"
bundle_lib="$bundle_res/opt/local/lib"
bundle_etc="$bundle_res/opt/local/etc"

cp ./fontforge/MacFontForgeAppBuilt.zip $TEMPDIR/
unzip -d $TEMPDIR $TEMPDIR/MacFontForgeAppBuilt.zip
DESTDIR=$bundle_res make install

sed -i -e "s|Gdraw.ScreenWidthCentimeters:.*|Gdraw.ScreenWidthCentimeters: 34|g" \
       "$bundle_res/opt/local/share/fontforge/pixmaps/resources"
sed -i -e "s|Gdraw.GMenu.MacIcons:.*|Gdraw.GMenu.MacIcons: True|g" \
       "$bundle_res/opt/local/share/fontforge/pixmaps/resources"


cd $bundle_bin
dylibbundler --overwrite-dir --bundle-deps --fix-file \
  ./fontforge \
  --install-path @executable_path/../lib \
  --dest-dir ../lib
dylibbundler --overwrite-dir --bundle-deps --fix-file \
  ./FontForgeInternal/fontforge-internal-collab-server \
  --install-path @executable_path/collablib \
  --dest-dir ./FontForgeInternal/collablib

cp -av /usr/lib/libedit* $bundle_lib/
cp -av /usr/lib/libedit* $bundle_bin/FontForgeInternal/collablib/

#########
# libedit.2 is on some older machines.
install_name_tool -change /usr/lib/libedit.3.dylib @executable_path/../lib/libedit.3.dylib fontforge 
cd ./FontForgeInternal
install_name_tool -change /usr/lib/libedit.3.dylib @executable_path/collablib/libedit.3.dylib fontforge-internal-collab-server
cd ..

mkdir -p $bundle_lib
cp -av /opt/local/lib/pango   $bundle_lib

OLDPREFIX=/opt/local
NEWPREFIX=/Applications/FontForge.app/Contents/Resources/opt/local

mkdir -p $bundle_etc
cp -av /opt/local/etc/fonts   $bundle_etc
cp -av /opt/local/etc/pango   $bundle_etc
cd $bundle_etc/pango
sed -i -e "s|$OLDPREFIX|$NEWPREFIX|g" pangorc
sed -i -e "s|$OLDPREFIX|$NEWPREFIX|g" pango.modules
cd $bundle_etc/fonts
sed -i -e "s|$OLDPREFIX|$NEWPREFIX|g" fonts.conf 
sed -i -e 's|<fontconfig>|<fontconfig><dir>/Applications/FontForge.app/Contents/Resources/opt/local/share/fontforge/pixmaps/</dir>|g' fonts.conf

cd $bundle_lib/pango/
cd `find . -type d -maxdepth 1 -mindepth 1`
cd `find . -type d -maxdepth 1 -mindepth 1`

echo "Bundling up the pango libs to point in the right directions... at `pwd`"

for if in `pwd`/*.so
do
    dylibbundler -x "$if" \
      --install-path @executable_path/../lib \
      --dest-dir "$bundle_lib"
done


cd $bundle_res
mkdir -p opt/local/share/mime
cp -av /opt/local/share/mime/mime.cache opt/local/share/mime
cp -av /opt/local/share/mime/globs      opt/local/share/mime
cp -av /opt/local/share/mime/magic      opt/local/share/mime


cd $TEMPDIR
rm -f  ~/FontForge.app.zip
zip -9 -r ~/FontForge.app.zip FontForge.app
cp -f  ~/FontForge.app.zip /tmp/
chmod o+r /tmp/FontForge.app.zip

echo "Completed at `date`"
ls -lh `echo ~`/FontForge.app.zip




