
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

SYNC_TO_BIGV() {
   rsync -av $BASEPATH bigv:/tmp/ 
}


export HOMEBREW_BUILD_FROM_SOURCE=1
export MACOSX_DEPLOYMENT_TARGET=10.6
