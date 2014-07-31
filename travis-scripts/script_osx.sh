#!/bin/bash
set -ev

brew install --verbose fontforge --HEAD --with-x
fontforge -version
