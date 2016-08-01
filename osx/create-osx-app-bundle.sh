#!/bin/bash

alias ldd='otool -L'

ORIGDIR=`pwd`
TEMPDIR=/tmp/fontforge-app-bundle
rm -rf /tmp/fontforge-app-bundle
mkdir $TEMPDIR

scriptdir=$TEMPDIR/FontForge.app/Contents/MacOS
frameworkdir=$TEMPDIR/FontForge.app/Contents/Frameworks
bundle_res=$TEMPDIR/FontForge.app/Contents/Resources
bundle_bin="$bundle_res/opt/local/bin"
bundle_lib="$bundle_res/opt/local/lib"
bundle_libexec="$bundle_res/opt/local/libexec"
bundle_etc="$bundle_res/opt/local/etc"
bundle_share="$bundle_res/opt/local/share"
export PATH="$PATH:$scriptdir"
srcdir=$(pwd)


echo "...doing the make install..."
DESTDIR=$bundle_res make install
echo "...setup FontForge.app bundle..."
rsync -av $bundle_res/opt/local/share/fontforge/osx/FontForge.app $TEMPDIR/

echo "...patching..."
patch -d $TEMPDIR/FontForge.app -p0 < osx/ipython-embed-fix.patch

echo "...on with the rest..."
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

# # dylibbundler wants to do overwrite-dir in order to fix the shared libraries too
# # but we want to preserve any existing subdirs, so we do the dylibundle to another
# # dir and then move the nice generated output to ./lib before moving on.
# cd ../lib-bundle
# cp -av . ../lib
# cd ..
# rm -rf ./lib-bundle
# cd ./bin
cd $bundle_libexec/bin
dylibbundler --overwrite-dir --bundle-deps --fix-file \
  ./FontForgeInternal/fontforge-internal-collab-server \
  --install-path @executable_path/collablib \
  --dest-dir ./FontForgeInternal/collablib
cd -

cp -av /usr/lib/libedit* $bundle_lib/
cp -av /usr/lib/libedit* $bundle_libexec/bin/FontForgeInternal/collablib/

cd $bundle_lib
for if in libXcomposite.1.dylib libXcursor.1.dylib libXdamage.1.dylib libXfixes.3.dylib libXinerama.1.dylib libXrandr.2.dylib libatk-1.0.0.dylib libgdk-x11-2.0.0.dylib libgdk_pixbuf-2.0.0.dylib libgtk-x11-2.0.0.dylib libtest-1.0.0.dylib
do
    cp -av /opt/local/lib/$if $bundle_lib/
    library-paths-opt-local-to-absolute.sh $bundle_lib/$if 
done

# cd $bundle_lib
# cd ./python2.7/site-packages/fontforge.so
# for if in $(find . -name "*so")
# do
#     echo $if
#     library-paths-opt-local-to-absolute.sh $if

#     install_name_tool -change /opt/local/home/ben/bdwgc/lib/libgc.1.dylib /Applications/FontForge.app/Contents/Resources/opt/local/lib/libgc.1.dylib $if
#     install_name_tool -change /usr/lib/libedit.3.dylib /Applications/FontForge.app/Contents/Resources/opt/local/lib/libedit.3.dylib $if
#     install_name_tool -change /opt/local/Library/Frameworks/Python.framework/Versions/2.7/Python \
#        /Applications/FontForge.app/Contents/Resources//opt/local/Library/Frameworks/Python.framework/Versions/2.7/Python \
#        $if

# done




cd $bundle_bin



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
cd $bundle_libexec/bin/FontForgeInternal/collablib/
for if in *dylib 
do 
  echo $if 
  otool -L  $if | grep libedit
  install_name_tool -change /usr/lib/libedit.3.dylib @executable_path/../lib/libedit.3.dylib $if
done

cd $bundle_bin
install_name_tool -change /usr/lib/libedit.3.dylib @executable_path/../lib/libedit.3.dylib fontforge 
cd $bundle_libexec/bin/FontForgeInternal/
install_name_tool -change /usr/lib/libedit.3.dylib @executable_path/collablib/libedit.3.dylib fontforge-internal-collab-server
cd $bundle_bin



##############################
#
# python can't be assumed to exist on the machine
#
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

#
# use links in filesystem instead of explicit code to handle the name change.
#
cd $bundle_lib
ln -s libgdraw.5.dylib libgdraw.so.5
cd $bundle_bin


####
#
# pygtk
#
cd ./Python.framework.2.7/lib/
cp -av /opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/libpyglib-2.0-python2.7.0.dylib .
for if in libpyglib-2.0-python*dylib
do
   library-paths-opt-local-to-absolute.sh $if
done
cd $bundle_bin

cd ./Python.framework.2.7/lib/python2.7/site-packages
for if in $(find . -name "*so")
do
    echo $if
    library-paths-opt-local-to-absolute.sh $if
    install_name_tool -change \
       /opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/libpyglib-2.0-python2.7.0.dylib \
       /Applications/FontForge.app/Contents/Resources/opt/local/bin/Python.framework.2.7/lib/libpyglib-2.0-python2.7.0.dylib \
       $if
done

cd $bundle_lib
for if in libpango*dylib libgobject-2.0.0.dylib libglib-2.0.0.dylib libintl.8.dylib   libgmodule-2.0.0.dylib libgthread-2.0.0.dylib  libcairo.2.dylib libfontconfig.1.dylib  libfreetype.6.dylib  libxcb.1.dylib  libxcb-shm.0.dylib  libxcb-render.0.dylib libX*dylib libharfbuzz.0.dylib  libgio-2.0.0.dylib
do    
   library-paths-to-absolute.sh $if; 
done


cd $bundle_bin


####
#
# ipython
#

cd $bundle_bin
cd ./Python.framework.2.7/lib/python2.7
cp /opt/local/bin/Python.framework.2.7/lib/python2.7/md5.py .
cd ./lib-dynload
cp /opt/local/bin/Python.framework.2.7/lib/python2.7/lib-dynload/_hashlib.so .
library-paths-opt-local-to-absolute.sh _hashlib.so
library-paths-opt-local-to-absolute.sh readline.so
cd ../lib/python2.7/site-packages/readline
library-paths-opt-local-to-absolute.sh readline.so

cd $bundle_lib
for if in libncurses.5.dylib libedit.0.dylib
do
    cp -av /opt/local/lib/$if $bundle_lib/
    library-paths-opt-local-to-absolute.sh $bundle_lib/$if 
done


####
#
# python - even - http://xxyxyz.org/even/
#
echo "###################"
echo "Bundling up Even..."
echo "###################"
cp -av /usr/local/src/even/build-even-Desktop-Debug/Even.app $scriptdir/
rm -rf $scriptdir/Even.app/Contents/Resources/pypy
ln -s  $scriptdir/Even.app/Contents/MacOS/Even $bundle_share/fontforge/python/Even
cd $scriptdir/Even.app/Contents/MacOS/
mkdir -p ./lib 
for if in $(ldd Even |grep Qt5|sed -E 's/	//g' | sed 's/ (.*//g' )
do
  echo "grabbing: $if"
  cp "$if" ./lib
done
for if in $(ldd Even |grep Qt5|sed -E 's/	//g' | sed 's/ (.*//g' )
do
  echo "fixing Even link to: $if"
  fn=$(basename "$if");
  install_name_tool -change "$if" "/Applications/FontForge.app/Contents/MacOS/Even.app/Contents/MacOS/lib/$fn" Even
done


mkdir -p ./platforms
cd ./platforms
for if in $(find /opt/local/home/ben/Qt5* -name "libqcocoa.dylib" | grep -v Creator)
do
  echo "grabbing: $if"
  cp "$if" .
done

for if in $(ldd libqcocoa.dylib | grep QtPrintSup |sed -E 's/	//g' | sed 's/ (.*//g' )
do
    cp "$if" ../lib/
done
for if in $(ldd libqcocoa.dylib | grep Qt5|sed -E 's/	//g' | sed 's/ (.*//g' )
do
  echo "fixing libqcocoa.dylib"
  fn=$(basename "$if");
  install_name_tool -change "$if" "/Applications/FontForge.app/Contents/MacOS/Even.app/Contents/MacOS/lib/$fn" libqcocoa.dylib
done
cd ../lib/
for if in Qt*
do
    echo "fixing $if"
    for dylibif in $(ldd $if | grep Qt|sed -E 's/	//g' | sed 's/ (.*//g' )
    do
       fn=$(basename "$dylibif");
       install_name_tool -change "$dylibif" "/Applications/FontForge.app/Contents/MacOS/Even.app/Contents/MacOS/lib/$fn" $if
    done
done

cd $bundle_bin

#
# Fix the dangling absolute link
#
cd "$bundle_share/fontforge/python"
rm -f Even
ln -s /Applications/FontForge.app/Contents/MacOS/Even.app/Contents/MacOS/Even .
cd $bundle_bin


########################
#
# we want nodejs in the bundle for collab
#
echo "###################"
echo "bundling up nodejs "
echo "###################"
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


#####################
#
# Some of this might be able to be taken out again, it is mainly to get
# breakpad going in the first place, and to allow command line execution
# of the fontforge binary which doesn't play well with @executable_path paths.
#
cd $bundle_bin
install_name_tool -change                                                       \
    @executable_path/../Frameworks/Breakpad.framework/Versions/A/Breakpad       \
    /Applications/FontForge.app/Contents/Frameworks/Breakpad.framework/Breakpad \
    fontforge 
cd $bundle_lib
install_name_tool -change                                                       \
    @executable_path/../Frameworks/Breakpad.framework/Versions/A/Breakpad       \
    /Applications/FontForge.app/Contents/Frameworks/Breakpad.framework/Breakpad \
    libfontforgeexe.2.dylib 
cd $frameworkdir/Breakpad.framework
install_name_tool -change                                                               \
    @executable_path/../Frameworks/Breakpad.framework/Resources/breakpadUtilities.dylib \
    /Applications/FontForge.app/Contents/Frameworks/Breakpad.framework/Resources/breakpadUtilities.dylib \
    Breakpad

cd $bundle_bin

#cp -av ~/bak/dump_syms $frameworkdir/Breakpad.framework/dump_syms
DUMPSYMS=$frameworkdir/Breakpad.framework/dump_syms
mkdir -p $bundle_res/breakpad/symbols

cd $bundle_bin
for arch in x86_64 i386; do
  mkdir -p $bundle_res/breakpad/symbols/$arch
  $DUMPSYMS -a $arch fontforge  > $bundle_res/breakpad/symbols/$arch/fontforge
done


cd $bundle_lib
for if in libfontforgeexe.?.dylib libfontforge.?.dylib libgdraw.?.dylib; do
  for arch in x86_64 i386; do
    mkdir -p $bundle_res/breakpad/symbols/$arch
    dsymutil $if
    $DUMPSYMS -a $arch $if.dSYM  > $bundle_res/breakpad/symbols/$arch/$if
  done
done

for arch in x86_64 i386; do
  cd $bundle_res/breakpad/symbols/$arch/
  for symfile in *; do
      hash=$(head -1 "$symfile"|cut -d' ' -f 4);
      fn=$(head -1   "$symfile"|cut -d' ' -f 5);
      if [ ! -z "$hash" -a ! -z "$fn" ]; then
	  mv "$fn" "$fn.sym"
	  mkdir -p "$fn/$hash"
          mv "$fn.sym" "$fn/$hash/"
      fi
  done
done

cd $bundle_bin

#
# I create the uncompressed tree so that a server script running on
# the same machine can easily access them and turn the minidump into
# human readable automatically
#
mkdir -p ~/fontforge-builds/breakpad-symbols-by-hash/$FONTFORGE_GIT_VERSION
rsync -av $bundle_res/breakpad \
          ~/fontforge-builds/breakpad-symbols-by-hash/$FONTFORGE_GIT_VERSION
cd ~/fontforge-builds/breakpad-symbols-by-hash
tar czf $FONTFORGE_GIT_VERSION.tar.gz $FONTFORGE_GIT_VERSION

cd $bundle_bin

######################


mkdir -p $bundle_lib
cp -av /opt/local/lib/pango   $bundle_lib

OLDPREFIX=/opt/local
NEWPREFIX=/Applications/FontForge.app/Contents/Resources/opt/local

mkdir -p $bundle_etc
#cp -av /opt/local/etc/fonts   $bundle_etc
cp -avL /opt/local/etc/fonts/conf.d   $bundle_etc/fonts/
cp -av /opt/local/etc/pango   $bundle_etc
cd $bundle_etc/pango
sed -i -e "s|$OLDPREFIX|$NEWPREFIX|g" pangorc
sed -i -e "s|$OLDPREFIX|$NEWPREFIX|g" pango.modules

# This is now handled by shipping a fonts.conf which is hand made.
#cd $bundle_etc/fonts
#sed -i -e "s|$OLDPREFIX|$NEWPREFIX|g" fonts.conf 
#sed -i -e 's|<fontconfig>|<fontconfig><dir>/Applications/FontForge.app/Contents/Resources/opt/local/share/fontforge/pixmaps/</dir>|g' fonts.conf
#cd $bundle_etc/fonts
#cp -f ../../share/fontforge/osx/FontForge.app/Contents/Resources/opt/local/etc/fonts/fonts.conf .

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
library-paths-to-absolute.sh pango-basic-fc.so


cd $bundle_res
mkdir -p opt/local/share/mime
cp -av /opt/local/share/mime/mime.cache opt/local/share/mime
cp -av /opt/local/share/mime/globs      opt/local/share/mime
cp -av /opt/local/share/mime/magic      opt/local/share/mime

mkdir -p $bundle_share/X11
cp -av /opt/local/share/X11/locale $bundle_share/X11

#####
#
#
FFBUILDBASE=/opt/local/var/macports/build/_usr_local_src_github-fontforge_fontforge_osx/fontforge/work/fontforge-2.0.0_beta1
DEBUGOBJBASE=$TEMPDIR/FontForge.app/Contents/Resources/opt/local/var/
for if in fontforgeexe fontforge gdraw gutils; do
    cd $FFBUILDBASE/$if/.libs 
    mkdir -p   $DEBUGOBJBASE/$if/.libs/
    cp -av *.o $DEBUGOBJBASE/$if/.libs/
done
cd $bundle_lib

# byte string length compatible
OLDBASE="/opt/local/var/macports/build/_usr_local_src_github-fontforge_fontforge_osx/fontforge/work/fontforge-2.0.0_beta1"
NEWBASE="/Applications/FontForge.app/Contents/Resources/opt/local/var////////////////////////////////////////////////////"
echo "changing the location of the object files used for debug (fontforgeexe)"
for ifpath in $FFBUILDBASE/fontforgeexe/.libs/*.o; do
    if=$(basename "$ifpath");
    LANG=C sed -i -e "s#$OLDBASE/fontforgeexe/.libs/$if#$NEWBASE/fontforgeexe/.libs/$if#g" libfontforgeexe.2.dylib
done
echo "changing the location of the object files used for debug (fontforge)"
for ifpath in $FFBUILDBASE/fontforge/.libs/*.o; do
    if=$(basename "$ifpath");
    LANG=C sed -i -e "s#$OLDBASE/fontforge/.libs/$if#$NEWBASE/fontforge/.libs/$if#g" libfontforge.2.dylib
done
echo "changing the location of the object files used for debug (gdraw)"
for ifpath in $FFBUILDBASE/gdraw/.libs/*.o; do
    if=$(basename "$ifpath");
    LANG=C sed -i -e "s#$OLDBASE/gdraw/.libs/$if#$NEWBASE/gdraw/.libs/$if#g" libgdraw.5.dylib
done
echo "changing the location of the object files used for debug (gutils)"
for ifpath in $FFBUILDBASE/gutils/.libs/*.o; do
    if=$(basename "$ifpath");
    LANG=C sed -i -e "s#$OLDBASE/gutils/.libs/$if#$NEWBASE/gutils/.libs/$if#g" libgutils.2.dylib
done


cd $TEMPDIR
find FontForge.app -exec touch {} \;
# if you don't maintain the timestamp then lldb will not 
# load the .o file anymore.
for if in fontforgeexe fontforge gdraw gutils; do
    cd $FFBUILDBASE/$if/.libs 
    for objfile in *.o; do
        touch -r $objfile $DEBUGOBJBASE/$if/.libs/$objfile
    done
done
cd $TEMPDIR

rm -f  ~/FontForge.app.dmg
# it seems that on 10.8 if you don't specify a size then you'll likely
# get a result of hdiutil: create failed - error -5341
hdiutil create -size 800m   \
   -volname   FontForge     \
   -srcfolder FontForge.app \
   -ov        -format UDBZ  \
   ~/FontForge.app.dmg
cp -f  ~/FontForge.app.dmg /tmp/
chmod o+r /tmp/FontForge.app.dmg


echo "Completed at `date`"
ls -lh `echo ~`/FontForge.app.dmg




