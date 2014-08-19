#!/bin/bash

#
# This is a script that will encode and break apart an SSH private key and show
# you the Travis CI secure environment variables which you can then use to recreate
# the SSH key inside the CI system.
#
# Note that it 
#
keyfile=${1:?supply the name of the key as arg1}
ghrepo=${2:?supply the github repo as arg2, eg. fontforge/fontforge }

keyfile=$(realpath $keyfile)
echo "keyfile: $keyfile"

# To generate secure SSH deploy key for a github repo to be used from Travis
base64 --wrap=0 $keyfile >| ${keyfile}_base64

rm -rfv   /tmp/traviscikeystmp
mkdir -p  /tmp/traviscikeystmp
cd        /tmp/traviscikeystmp
split --bytes=100 --numeric-suffixes --suffix-length=2 ${keyfile}_base64 id_rsa_

for if in id_rsa*
do
    #echo $if
    echo -n "  - secure: " 
    travis encrypt --no-interactive "$if=$(cat $if)" -r $ghrepo 2>/dev/null
done
