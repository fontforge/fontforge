
#
# Source this file to setup a tree for files you might
# like to sync over to the server (eg, app.zip files)
#
# To use this simply write your files anywhere inside
# $TO_BIGV_OUTPUTPATH and call SYNC_TO_BIGV to have those files copied
# over to the server.
#
# Both the Linux and OSX before install scripts are updated to allow
# the SSH key to be reconstituted. That code might be ported into this
# file in the future, beware that there are some subtle differences to
# that code because of Linux/OSX shell differences.
#
BASEPATH=/tmp/bigvbase
if [ ! -e $BASEPATH ]; then
  DATEPART=$(date '+%Y-%m-%d')
  TO_BIGV_OUTPUTPATH="$BASEPATH/${DATEPART}-${TRAVIS_PULL_REQUEST}-${TRAVIS_COMMIT}/"
  mkdir -p "$TO_BIGV_OUTPUTPATH"
  date > "$TO_BIGV_OUTPUTPATH/creation-time"
  echo "$TO_BIGV_OUTPUTPATH" > /tmp/bigvbase/path
else
  TO_BIGV_OUTPUTPATH=$(cat /tmp/bigvbase/path)
fi
echo "TO_BIGV_OUTPUTPATH: $TO_BIGV_OUTPUTPATH"

#
# create or use a cache file to tell if we had an SSH key when we were
# first called. Creating and using a file cache allows multiple
# scripts to share the state
#
if [ ! -e /tmp/bigvbase/have-key ]; then
  if [ y"$id_rsa_11" = y ]; then 
    echo "WARNING: no secure env variables... no upload to server for you..."; 
    echo 0 > /tmp/bigvbase/have-key
  else
    echo 1 > /tmp/bigvbase/have-key
    HAVE_SSH_KEY=1;
  fi
else
  if [ y$(cat /tmp/bigvbase/have-key) = y1 ]; then 
    HAVE_SSH_KEY=1;
  else
    HAVE_SSH_KEY=0; 
  fi

fi

#
# Call this whenever you like to copy all the files from $BASEPATH
# over to the server. If there is no SSH key setup then this function
# will just issue a warning.
#
SYNC_TO_BIGV() {
    if [ "y$HAVE_SSH_KEY" = y1 ]; then
      rsync -av $BASEPATH bigv:/tmp/ 
    else
      echo "WARNING: No SSH key setup, no server sync for this build..."
    fi
}


# NB: build from source takes > 50 minutes and breaks Travis CI.
#export HOMEBREW_BUILD_FROM_SOURCE=1
#export MACOSX_DEPLOYMENT_TARGET=10.6
