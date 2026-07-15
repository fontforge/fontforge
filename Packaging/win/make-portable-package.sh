#!/bin/bash

#Detect 7-zip version to use
WSZIP="/c/Program Files/7-Zip/7z.exe"
if [ -f "$WSZIP" ]; then
    SZIP="$WSZIP"
else
    SZIP="7za"
fi

# Colourful text
# Red text
function log_error() {
    echo -e "\e[31m$@\e[0m"
}

# Yellow text
function log_status() {
    echo -e "\e[33m$@\e[0m"
}

# Green text
function log_note() {
    echo -e "\e[32m$@\e[0m"
}

function bail () {
    echo -e "\e[31m\e[1m!!! Build failed at: ${@}\e[0m"
    exit 1
}

function check_overwrite() {
	if [ -f "$1" ]; then
		read -p "File exists. Overwrite? [y/N]: " overwrite
		case $overwrite in
			[Yy]* ) rm -fv "$1" || bail "Could not remove $1"; break;;
			* ) bail "Not overwriting $1." ;;
		esac
	fi
}

function szip_folder() {
	# Enable dotglob to include hidden files/directories (like .debug)
	shopt -s dotglob
	"$SZIP" a -t7z -mx=9 -m0=lzma2 -md=128m "$1" "$2"/*
	local result=$?
	shopt -u dotglob
	return $result
}

if [ -z "$1" ]; then
    postfix="r1"
else
    postfix="$1"
fi

if [ "$MSYSTEM" = "UCRT64" ]; then
    MINGVER=ucrt64
    PMARCH=ucrt-x86_64
elif [ "$MSYSTEM" = "CLANGARM64" ]; then
    MINGVER=clangarm64
    PMARCH=clang-aarch64
else
    bail "Unknown build system!"
fi
PKGPREFIX="FontForge-mingw-w64-$PMARCH"

BASE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR=${BUILD_DIR:-$BASE/build}
# Convert Windows paths to Unix paths if needed
if command -v cygpath &> /dev/null; then
    BUILD_DIR=$(cygpath -u "$BUILD_DIR")
fi
SETUP=$BASE/setup
WORK=$BUILD_DIR/work/$MINGVER
RELEASE=$BUILD_DIR/ReleasePackage
DEBUG=$BUILD_DIR/debugging-symbols

log_note "Packaging $MINGVER release..."
pacman -S --noconfirm --needed p7zip > /dev/null 2>&1 

if [ -z "$FFPATH" ]; then
    FFPATH=$WORK/fontforge
fi

version_hash=`git -C $FFPATH rev-parse master -- || git -C $FFPATH log --pretty=format:%H -n 1`
version_hash=${version_hash:0:6}
log_status "Version hash is $version_hash"
log_status "Working directory: `pwd`"

log_status "Building the release archive..."
filename="$PKGPREFIX-$version_hash-$postfix.7z"
log_status "Name is $filename"
check_overwrite "$filename"
szip_folder "$filename" "$RELEASE"

log_status "Building the debug archive..."
debugname="$PKGPREFIX-$version_hash-$postfix-debugging-symbols.7z"
log_status "Name is $debugname"
check_overwrite "$debugname"
szip_folder "$debugname" "$DEBUG"
