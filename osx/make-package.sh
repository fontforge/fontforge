#!/bin/bash

cd work/fontforge-*/
pwd >| /tmp/bundle.txt
sudo chmod +x ./osx/create-osx-app-bundle.sh
sudo ./osx/create-osx-app-bundle.sh >> /tmp/bundle.txt
cat /tmp/FontForge.app.dmg > ~/Desktop/FontForge.app.dmg
