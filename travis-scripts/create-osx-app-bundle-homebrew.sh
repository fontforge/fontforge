#!/bin/bash


IPYTHON_COUNT=$(pip list |grep 'ipython (2.' | wc -l | sed 's/ //g')
if [ y${IPYTHON_COUNT} = y0 ]; then
  echo "the bundle includes IPython..."
  echo " a sudo command is now run to install that if it is not already installed"
  sudo pip install ipython
fi

alias ldd='otool -L'

ORIGDIR=`pwd`
TEMPDIR=/tmp/fontforge-app-bundle
rm -rf /tmp/fontforge-app-bundle
mkdir $TEMPDIR

scriptdir=$TEMPDIR/FontForge.app/Contents/MacOS
bundle_res=$TEMPDIR/FontForge.app/Contents/Resources
bundle_bin="$bundle_res/opt/local/bin"
bundle_lib="$bundle_res/opt/local/lib"
bundle_libexec="$bundle_res/opt/local/libexec"
bundle_etc="$bundle_res/opt/local/etc"
bundle_share="$bundle_res/opt/local/share"
export PATH="$PATH:$scriptdir:/usr/local/bin"
srcdir=$(pwd)


#cp ./fontforge/MacFontForgeAppBuilt.zip $TEMPDIR/
#unzip -d $TEMPDIR $TEMPDIR/MacFontForgeAppBuilt.zip
echo "...doing the make install..."
DESTDIR=$bundle_res make install


echo "...creating links to work with homebrew install too"
mkdir -p $bundle_res/opt
#cd $bundle_res/opt
#ln -s ../usr/local/Cellar/fontforge/HEAD local
#cd $ORIGDIR

echo "...setup FontForge.app bundle..."
echo "rsync from $bundle_res/opt/local/share/fontforge/osx/FontForge.app  ..to..  $TEMPDIR/ "
echo "...before..."
ls -l /tmp/fontforge-app-bundle/FontForge.app/Contents/Resources/opt/local/share/
#rsync -av $bundle_res/opt/local/share/fontforge/osx/FontForge.app $TEMPDIR/
cd $bundle_res/usr/local/Cellar/fontforge/HEAD/share/fontforge/osx
rsync -av FontForge.app $TEMPDIR/
cd $ORIGDIR
echo "...after..."
ls -l  $bundle_res/opt
find $bundle_res/opt
cd $bundle_res/opt
mv local/etc ../usr/local/Cellar/fontforge/HEAD/
rmdir local
echo "...after2..."
cd $bundle_res/opt
ln -s ../usr/local/Cellar/fontforge/HEAD local
ls -l /tmp/fontforge-app-bundle/FontForge.app/Contents/Resources/opt/local/share/
cd $ORIGDIR

# FIXME
#cp -av /tmp/ldd /tmp/fontforge-app-bundle/FontForge.app/Contents/MacOS/


#echo "...patching..."
#patch -d $TEMPDIR/FontForge.app -p0 < osx/ipython-embed-fix.patch

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
cd $(pwd -P)
echo -n "starting dylibbundler at "
pwd
dylibbundler --overwrite-dir --bundle-deps --fix-file \
  ./fontforge \
  --install-path @executable_path/../lib \
  --dest-dir ../lib

echo "... dylibbunder on main fontforge executable is now complete..."


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
echo "... dylibbunder on collab server is complete ..."

cp -av /usr/lib/libedit* $bundle_lib/
cp -av /usr/lib/libedit* $bundle_libexec/bin/FontForgeInternal/collablib/

cd $bundle_lib
for if in libXcomposite.1.dylib libXcursor.1.dylib libXdamage.1.dylib libXfixes.3.dylib libXinerama.1.dylib libXrandr.2.dylib 
do
    echo $if
    ls -l /opt/X11/lib/$if
    cp -av /opt/X11/lib/$if $bundle_lib/
    library-paths-opt-local-to-absolute.sh $bundle_lib/$if 
done
for if in libatk-1.0.0.dylib libgdk-x11-2.0.0.dylib libgdk_pixbuf-2.0.0.dylib libgtk-x11-2.0.0.dylib
do
    echo $if
    ls -l /usr/local/lib/$if
    cp -avH /usr/local/lib/$if $bundle_lib/
    chmod +w $bundle_lib/$if
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



echo "...setting up fc-cache..."
############
# Grab fc-cache so we can show the cache update before the actual
# fontforge process is run
#
mkdir -p $scriptdir/fc
cp /usr/local/bin/fc-cache $scriptdir/fc
chmod +x $scriptdir/fc
chmod +w $scriptdir/fc/fc-cache
cd $scriptdir/fc
dylibbundler --overwrite-dir --bundle-deps --fix-file \
  $scriptdir/fc/fc-cache \
  --install-path @executable_path/lib \
  --dest-dir $scriptdir/fc/lib
cd $bundle_bin


#########
# libedit.2 is on some older machines.
echo "...creating a fix for depending on libedit.2 for older osx machines..."
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
echo "... grabbing python into the bundle..."
cd $bundle_bin
cp -av /usr/local/Cellar/python/2.7.8/Frameworks/Python.framework/Versions/2.7/Python .
cd $bundle_bin
cp -av /usr/local/Cellar/python/2.7.8/Frameworks/Python.framework/Versions/2.7 Python.framework.2.7

cd ./Python.framework.2.7/lib/python2.7
rm -f site-packages 
cp -avL /usr/local/lib/python2.7/site-packages .
chmod -R +w ./site-packages


mkdir -p $bundle_lib
cd $bundle_lib
for if in *dylib 
do 
  echo $if 
  otool -L  $if | grep libedit
  install_name_tool -change /opt/local/Library/Frameworks/Python.framework/Versions/2.7/Python @executable_path/Python $if
  install_name_tool -change /usr/local/Frameworks/Python.framework/Versions/2.7/Python         @executable_path/Python $if
done
cd $bundle_bin
install_name_tool -change /opt/local/Library/Frameworks/Python.framework/Versions/2.7/Python @executable_path/Python fontforge 
install_name_tool -change /usr/local/Frameworks/Python.framework/Versions/2.7/Python         @executable_path/Python fontforge 
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
echo "...bundling pygtk into the staging area..."
cd ./Python.framework.2.7/lib/
cp -avH /usr/local/lib/libpyglib-2.0-python.0.dylib /usr/local/lib/libpyglib-2.0-python.dylib .
for if in libpyglib-2.0-python*dylib
do
   library-paths-opt-local-to-absolute.sh $if
done
cd $bundle_bin

MADE_APPS_FF_LINK=0
if [ ! -e /Applications/FontForge.app ]; then
  echo "making a temporary link from /Applications/FontForge.app to setup paths."
  # only remove the link if we made it.
  MADE_APPS_FF_LINK=1
  ln -s /tmp/fontforge-app-bundle/FontForge.app /Applications/FontForge.app 
fi

cd ./Python.framework.2.7/lib/python2.7/site-packages
for if in $(find . -name "*so")
do
    echo $if
    library-paths-opt-local-to-absolute.sh $if
#    install_name_tool -change \
#       /opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/libpyglib-2.0-python2.7.0.dylib \
#       /Applications/FontForge.app/Contents/Resources/opt/local/bin/Python.framework.2.7/lib/libpyglib-2.0-python2.7.0.dylib \
#       $if
    install_name_tool -change \
       /usr/local/Frameworks/Python.framework/Versions/2.7/Python \
       /Applications/FontForge.app/Contents/Resources/usr/local/Cellar/fontforge/HEAD/bin/Python.framework.2.7/lib/libpyglib-2.0-python.dylib \
       $if
    install_name_tool -change \
       /usr/local/Frameworks/Python.framework/Versions/2.7/Python \
       /Applications/FontForge.app/Contents/Resources/usr/local/Cellar/fontforge/HEAD/bin/Python.framework.2.7/lib/libpyglib-2.0-python.dylib \
       $if
done
if [ y${MADE_APPS_FF_LINK} = y1 ]; then
   rm -f /Applications/FontForge.app
fi


echo "...pygtk updating the libs..."
cd $bundle_lib
for if in libpango*dylib libgobject-2.0.0.dylib libglib-2.0.0.dylib libintl.8.dylib   libgmodule-2.0.0.dylib libgthread-2.0.0.dylib  libcairo.2.dylib libfontconfig.1.dylib  libfreetype.6.dylib  libxcb.1.dylib  libxcb-shm.0.dylib  libxcb-render.0.dylib libX*dylib libharfbuzz.0.dylib  libgio-2.0.0.dylib
do    
   echo "checking lib $if"
   library-paths-to-absolute.sh $if; 
done


cd $bundle_bin


####
#
# ipython
#
echo '...ipython bundling support...'

cp -avH /usr/local/Cellar/readline/*/lib/libreadline.dylib $bundle_lib/
chmod +w $bundle_lib/libreadline.dylib

cd $bundle_bin
cd ./Python.framework.2.7/lib/python2.7
#cp /opt/local/bin/Python.framework.2.7/lib/python2.7/md5.py .
cd ./lib-dynload
#cp /opt/local/bin/Python.framework.2.7/lib/python2.7/lib-dynload/_hashlib.so .
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
##cp -av /usr/local/src/even/build-even-Desktop-Debug/Even.app $scriptdir/
cd $scriptdir/
# FIXME
curl -L https://github.com/monkeyiq/fontforge-osx-bundle-deps/raw/master/Even.app.tar.gz | tar xzf - 
#cat ~/Even.app.tar.gz | tar xzf -
cd $bundle_lib

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
cp -avH /usr/local/bin/node .
chmod +w node
cd $bundle_lib
if [ ! -f ../lib/libssl.1.0.0.dylib ]; then
    #cp -av /opt/local/lib/libssl*dylib ../lib/
    cp -avH /usr/local/opt/openssl/lib/libssl*.dylib ../lib/
    chmod +w ../lib/libssl*
fi
if [ ! -f ../lib/libcrypto.1.0.0.dylib ]; then
    #cp -av /opt/local/lib/libcrypto*dylib ../lib/
    cp -avH /usr/local/opt/openssl/lib/libcrypto*.dylib ../lib/
    chmod +w ../lib/libcrypto*
fi

cd $bundle_bin
install_name_tool -change /opt/local/lib/libssl.1.0.0.dylib    @executable_path/../lib/libssl.1.0.0.dylib node 
install_name_tool -change /opt/local/lib/libcrypto.1.0.0.dylib @executable_path/../lib/libcrypto.1.0.0.dylib node 
cd $bundle_lib
for if in libssl.1.0.0.dylib libcrypto.1.0.0.dylib;
do
    install_name_tool -change /usr/local/opt/openssl/lib/libcrypto.1.0.0.dylib @executable_path/../lib/libcrypto.1.0.0.dylib $if
    install_name_tool -change /usr/local/opt/openssl/lib/libssl.1.0.0.dylib    @executable_path/../lib/libssl.1.0.0.dylib $if
    install_name_tool -change /usr/local/lib/libz.1.dylib                      @executable_path/../lib/libz.1.dylib $if
done
cd $bundle_share/fontforge/nodejs/collabwebview
echo "installing socket.io into bundle for the collabwebview nodejs code to use"
# this will npm install socket.io locally, without network/build interaction.
#cp -av ~/macports/categories/fontforge/node/node_modules .
npm install socket.io
cd $bundle_bin






#########


mkdir -p $bundle_lib
cp -avH /usr/local/lib/pango   $bundle_lib
chmod -R +w $bundle_lib

OLDPREFIX=/usr/local
NEWPREFIX=/Applications/FontForge.app/Contents/Resources/opt/local

mkdir -p $bundle_etc
#cp -av /opt/local/etc/fonts   $bundle_etc
cp -avL /usr/local/etc/fonts/conf.d   $bundle_etc/fonts/
cp -avL /usr/local/etc/pango   $bundle_etc
chmod -R +w $bundle_etc
cd $bundle_etc/pango
if [ ! -f pangorc ]; then
  echo "[Pango]" >> pangorc
  echo "ModuleFiles = /Applications/FontForge.app/Contents/Resources/opt/local/etc/pango/pango.modules" >> pangorc
fi
sed -i -e "s|$OLDPREFIX|$NEWPREFIX|g" pangorc
sed -i -e "s|$OLDPREFIX|$NEWPREFIX|g" pango.modules
sed -i -e "s|Cellar/pango/[^/]*/||g" pangorc
sed -i -e "s|Cellar/pango/[^/]*/||g" pango.modules


# This is now handled by shipping a fonts.conf which is hand made.
#cd $bundle_etc/fonts
#sed -i -e "s|$OLDPREFIX|$NEWPREFIX|g" fonts.conf 
#sed -i -e 's|<fontconfig>|<fontconfig><dir>/Applications/FontForge.app/Contents/Resources/opt/local/share/fontforge/pixmaps/</dir>|g' fonts.conf
#cd $bundle_etc/fonts
#cp -f ../../share/fontforge/osx/FontForge.app/Contents/Resources/opt/local/etc/fonts/fonts.conf .

cd $bundle_lib/pango/
cd `find . -type d -maxdepth 1 -mindepth 1`
cd `find . -type d -maxdepth 1 -mindepth 1`

echo "...bundling up the pango libs to point in the right directions... at `pwd`"

for if in `pwd`/*.so
do
    dylibbundler -x "$if" \
      --install-path @executable_path/../lib \
      --dest-dir "$bundle_lib"
done
library-paths-to-absolute.sh pango-basic-fc.so


cd $bundle_res
mkdir -p opt/local/share/mime
cp -av /usr/local/share/mime/mime.cache opt/local/share/mime
cp -av /usr/local/share/mime/globs      opt/local/share/mime
cp -av /usr/local/share/mime/magic      opt/local/share/mime

mkdir -p $bundle_share/X11
cp -av /opt/X11/share/X11/locale $bundle_share/X11


###########
#
# Turn all links to Cellar into .../usr/local/lib ones
# to avoid confusing the dynamic linker.
echo "...converting Cellar links"
cd $bundle_lib
for if in *dylib; do 
  echo $if; 
  for LIB in $(otool -L $if | grep Cellar | sed 's/dylib.*/dylib /g'); do
     echo "LIB $LIB"
     install_name_tool -change $LIB /Applications/FontForge.app/Contents/Resources/opt/local/lib/$(basename $LIB)  $if
  done
done



##################
#
# Wrap it all up.
#

cd $TEMPDIR
find FontForge.app -exec touch {} \;
rm -f  ~/FontForge.app.zip
zip -9 --symlinks -r ~/FontForge.app.zip FontForge.app
cp -f  ~/FontForge.app.zip /tmp/
chmod o+r /tmp/FontForge.app.zip

echo "Completed at `date`"
ls -lh `echo ~`/FontForge.app.zip




