#!/bin/bash
set -ev

echo "doing an OSX before install step."
brew update
brew install cairo python libspiro fontconfig
