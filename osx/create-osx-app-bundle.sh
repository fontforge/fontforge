#!/bin/bash

alias ldd='otool -L'

ORIGDIR=`pwd`
TEMPDIR=/tmp/fontforge-app-bundle
rm -rf /tmp/fontforge-app-bundle
mkdir $TEMPDIR

scriptdir=$TEMPDIR/FontForge.app/Contents/MacOS
bundle_res=$TEMPDIR/FontForge.app/Contents/Resources
bundle_bin="$bundle_res/opt/local/bin"
bundle_lib="$bundle_res/opt/local/lib"
bundle_etc="$bundle_res/opt/local/etc"
bundle_share="$bundle_res/opt/local/share"

#cp ./fontforge/MacFontForgeAppBuilt.zip $TEMPDIR/
#unzip -d $TEMPDIR $TEMPDIR/MacFontForgeAppBuilt.zip
DESTDIR=$bundle_res make install
rsync -av $bundle_res/opt/local/share/fontforge/osx/FontForge.app $TEMPDIR/


sed -i -e "s|Gdraw.ScreenWidthCentimeters:.*|Gdraw.ScreenWidthCentimeters: 34|g" \
       "$bundle_res/opt/local/share/fontforge/pixmaps/resources"
sed -i -e "s|Gdraw.GMenu.MacIcons:.*|Gdraw.GMenu.MacIcons: True|g" \
       "$bundle_res/opt/local/share/fontforge/pixmaps/resources"

#
# This block updates the metadata that finder will show for the app.
# by sorting fontforge-config.h prefix names will come first, eg
# FONTFORGE_MODTIME before FONTFORGE_MODTIME_RAW so that the head -1
# will pick the prefix name from the sorted include file.
#
osxmetadata_file=$TEMPDIR/FontForge.app/Contents/Resources/English.lproj/InfoPlist.string
sort ./inc/fontforge-config.h >|/tmp/fontforge-config-sorted

FONTFORGE_MODTIME_STR=$(grep FONTFORGE_MODTIME_STR /tmp/fontforge-config-sorted | head -1 | sed 's/^[^ ]*[ ][^ ]*[ ]//g')
FONTFORGE_VERSIONDATE_RAW=$(grep FONTFORGE_VERSIONDATE_RAW /tmp/fontforge-config-sorted | head -1 | sed 's/^[^ ]*[ ][^ ]*[ ]//g')
FONTFORGE_MODTIME_STR_RAW=$(grep FONTFORGE_MODTIME_STR_RAW /tmp/fontforge-config-sorted | head -1 | sed 's/^[^ ]*[ ][^ ]*[ ]//g')
FONTFORGE_GIT_VERSION=$(grep FONTFORGE_GIT_VERSION /tmp/fontforge-config-sorted | head -1 | sed 's/^[^ ]*[ ][^ ]*[ ]//g')
FONTFORGE_GIT_VERSION=$(echo $FONTFORGE_GIT_VERSION | sed 's/"//g')
echo "got: modtime     = $FONTFORGE_MODTIME_STR"
echo "got: versiondata = $FONTFORGE_VERSIONDATE_RAW"
echo "got: git ver     = $FONTFORGE_GIT_VERSION"

CFBundleShortVersionString="Version $FONTFORGE_VERSIONDATE_RAW"
CFBundleGetInfoString="FontForge version $FONTFORGE_VERSIONDATE_RAW based on sources from $FONTFORGE_MODTIME_STR_RAW git:$FONTFORGE_GIT_VERSION";

# Replace version strings in InfoPlist.string 
sed -i -e "s/CFBundleShortVersionString.*/CFBundleShortVersionString = \"$CFBundleShortVersionString\"/g" $osxmetadata_file
sed -i -e "s/CFBundleGetInfoString.*/CFBundleGetInfoString = \"$CFBundleGetInfoString\"/g" $osxmetadata_file

# Replace version strings in Info.plist 
sed -i -e "s/CFBundleShortVersionStringChangeMe/$CFBundleShortVersionString/g" $TEMPDIR/FontForge.app/Contents/Info.plist 
sed -i -e "s/CFBundleGetInfoStringChangeMe/$CFBundleGetInfoString/g" $TEMPDIR/FontForge.app/Contents/Info.plist
sed -i -e "s/CFBundleVersionChangeMe/$FONTFORGE_VERSIONDATE_RAW/g" $TEMPDIR/FontForge.app/Contents/Info.plist


#
# Now to fix the binaries to have shared library paths that are all inside
# the distrubtion instead of off into /opt/local or where macports puts things.
#
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

############
# Grab fc-cache so we can show the cache update before the actual
# fontforge process is run
#
mkdir -p $scriptdir/fc
cp /opt/local/bin/fc-cache $scriptdir/fc
chmod +x $scriptdir/fc
cd $scriptdir/fc
dylibbundler --overwrite-dir --bundle-deps --fix-file \
  $scriptdir/fc/fc-cache \
  --install-path @executable_path/lib \
  --dest-dir $scriptdir/fc/lib
cd $bundle_bin

#########
# libedit.2 is on some older machines.
mkdir -p $bundle_lib
cd $bundle_lib
for if in *dylib 
do 
  echo $if 
  otool -L  $if | grep libedit
  install_name_tool -change /usr/lib/libedit.3.dylib @executable_path/../lib/libedit.3.dylib $if
done
cd $bundle_bin/FontForgeInternal/collablib/
for if in *dylib 
do 
  echo $if 
  otool -L  $if | grep libedit
  install_name_tool -change /usr/lib/libedit.3.dylib @executable_path/../lib/libedit.3.dylib $if
done

cd $bundle_bin
install_name_tool -change /usr/lib/libedit.3.dylib @executable_path/../lib/libedit.3.dylib fontforge 
cd ./FontForgeInternal
install_name_tool -change /usr/lib/libedit.3.dylib @executable_path/collablib/libedit.3.dylib fontforge-internal-collab-server
cd $bundle_bin



#########
# python can't be assumed to exist on the machine
cd $bundle_bin
cp -av /opt/local/Library/Frameworks/Python.framework/Versions/2.7/Python .
cd $bundle_bin
cp -av /opt/local/Library/Frameworks/Python.framework/Versions/2.7 Python.framework.2.7


mkdir -p $bundle_lib
cd $bundle_lib
for if in *dylib 
do 
  echo $if 
  otool -L  $if | grep libedit
  install_name_tool -change /opt/local/Library/Frameworks/Python.framework/Versions/2.7/Python @executable_path/Python $if
done
cd $bundle_bin
install_name_tool -change /opt/local/Library/Frameworks/Python.framework/Versions/2.7/Python @executable_path/Python fontforge 
cd $bundle_bin


#########
# we want nodejs in the bundle for collab

mkdir -p $bundle_lib  $bundle_bin
cd $bundle_bin
cp -av /opt/local/bin/node .
cd $bundle_lib
if [ ! -f ../lib/libssl.1.0.0.dylib ]; then
    cp -av /opt/local/lib/libssl*dylib ../lib/
fi
if [ ! -f ../lib/libcrypto.1.0.0.dylib ]; then
    cp -av /opt/local/lib/libcrypto*dylib ../lib/
fi

cd $bundle_bin
install_name_tool -change /opt/local/lib/libssl.1.0.0.dylib    @executable_path/../lib/libssl.1.0.0.dylib node 
install_name_tool -change /opt/local/lib/libcrypto.1.0.0.dylib @executable_path/../lib/libcrypto.1.0.0.dylib node 
cd $bundle_lib
for if in libssl.1.0.0.dylib libcrypto.1.0.0.dylib;
do
    install_name_tool -change /opt/local/lib/libcrypto.1.0.0.dylib @executable_path/../lib/libcrypto.1.0.0.dylib $if
    install_name_tool -change /opt/local/lib/libssl.1.0.0.dylib    @executable_path/../lib/libssl.1.0.0.dylib $if
    install_name_tool -change /opt/local/lib/libz.1.dylib          @executable_path/../lib/libz.1.dylib $if
done
cd $bundle_share/fontforge/nodejs/collabwebview
# this will npm install socket.io locally, without network/build interaction.
cp -av ~/macports/categories/fontforge/node/node_modules .
cd $bundle_bin






#########


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

mkdir -p $bundle_share/X11
cp -av /opt/local/share/X11/locale $bundle_share/X11


cd $TEMPDIR
find FontForge.app -exec touch {} \;
rm -f  ~/FontForge.app.zip
zip -9 -r ~/FontForge.app.zip FontForge.app
cp -f  ~/FontForge.app.zip /tmp/
chmod o+r /tmp/FontForge.app.zip

echo "Completed at `date`"
ls -lh `echo ~`/FontForge.app.zip




