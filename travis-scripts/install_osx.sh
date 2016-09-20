#!/bin/bash
set -v

echo "osx install..."
brew install ./travis-scripts/fontforge.rb --HEAD --only-dependencies || true
brew link gettext --force
