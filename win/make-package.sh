#!/bin/bash

makensis setup/fontforge.nsi
mkdir -p fontforge-builds
/bin/mv -f setup/FontForge-setup.exe fontforge-builds/
