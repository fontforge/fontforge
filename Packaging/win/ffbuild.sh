#!/bin/bash
# FontForge build script.
# Uses MSYS2/MinGW-w64
# Author: Jeremy Tan
# This script retrieves and installs all libraries required to build FontForge.
# It then attempts to compile the latest version of FontForge, and to
# subsequently make a redistributable package.

reconfigure=0
nomake=0
yes=0
makedebug=0
appveyor=0
ghactions=0
depsonly=0
qt=0
github="fontforge"

function dohelp() {
    echo "Usage: `basename $0` [options]"
    echo "  -h, --help           Prints this help message"
    echo "  -g, --github <user>  Use forked FontForge repository by github <user>"
    echo "  -r, --reconfigure    Forces the configure script to be rerun for the currently"
    echo "                       worked-on package."
    echo "  -n, --nomake         Don't make/make install FontForge but do everything else"
    echo "  -y, --yes            Say yes to all build script prompts"
    echo "  -d, --makedebug      Adds in debugging utilities into the build (adds a gdb"
    echo "                       automation script)"
    echo "  -t, --qt             Builds the UI using Qt instead of GDK"
    echo "  -a, --appveyor       AppVeyor specific settings (in-source build)"
    echo "  --github-actions     GitHub Actions specific settings (in-source build)"
    echo "  -l, --depsonly       Only install the dependencies and not FontForge itself."
    exit $1
}

# Colourful text
# Red text
function log_error() {
    echo -ne "\e[31m"; echo "$@"; echo -ne "\e[0m"
}

# Yellow text
function log_status() {
    echo -ne "\e[33m"; echo "$@"; echo -ne "\e[0m"
}

# Green text
function log_note() {
    echo -ne "\e[32m"; echo "$@"; echo -ne "\e[0m"
}

function bail () {
    echo -ne "\e[31m\e[1m"; echo "!!! Build failed at: ${@}"; echo -ne "\e[0m"
    exit 1
}

function detect_arch_switch () {
    mkdir -p "$BUILD_DIR"
    local from="$(ls -1 "$BUILD_DIR"/.building-* 2>/dev/null)"
    local to="$BUILD_DIR/.building-$1"

    if [ ! -z "$from" ] && [ "$from" != "$to" ]; then
        if (($yes)); then
            rm -rf "$RELEASE" || bail "Could not reset ReleasePackage"
        else
            read -p "Architecture change detected! ReleasePackage must be reset. Continue? [y/N]: " arch_confirm
            case $arch_confirm in
                [Yy]* )
                    rm -rf "$RELEASE" || bail "Could not reset ReleasePackage"
                    rm -rf "$DBSYMBOLS"
                    mkdir -p "$DBSYMBOLS"
                    ;;
                * ) bail "Not overwriting ReleasePackage" ;;
            esac
        fi
    fi

    rm -f $from
    touch $to
}

function get_archive() {
    local archive=$1
    local url=$2
    local url2=$3

    if [ ! -f "$archive" ]; then
        log_note "$archive does not exist, downloading from $url"
        wget --tries 4 "$url" -O "$archive" || ([ ! -z "$url2" ] && wget --tries 4 "$url2" -O "$archive")
    fi
}

# Preamble
log_note "MSYS2 FontForge build script..."

# Retrieve input arguments to script
optspec=":hrnydalg-:"
while getopts "$optspec" optchar; do
    case "${optchar}" in
        -)
            case "${OPTARG}" in
                reconfigure)
                    reconfigure=$((1-reconfigure)) ;;
                nomake)
                    nomake=$((1-nomake)) ;;
                makedebug)
                    makedebug=$((1-makedebug)) ;;
                appveyor)
                    appveyor=$((1-appveyor)) ;;
                github-actions)
                    ghactions=$((1-ghactions)) ;;
                depsonly)
                    depsonly=$((1-depsonly)) ;;
                yes)
                    yes=$((1-yes)) ;;
                qt)
                    qt=$((1-qt)) ;;
                github)
                    github="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 )) ;;
                help)
                    dohelp 0;;
                *)
                    log_error "Unknown option --${OPTARG}"
                    dohelp 1 ;;
            esac;;
        r)
            reconfigure=$((1-reconfigure)) ;;
        n)
            nomake=$((1-nomake)) ;;
        d)
            makedebug=$((1-makedebug)) ;;
        a)
            appveyor=$((1-appveyor)) ;;
        l)
            depsonly=$((1-depsonly)) ;;
        y)
            yes=$((1-yes)) ;;
        t)
            qt=$((1-qt)) ;;
        g)
            github="${!OPTIND}"; OPTIND=$(( $OPTIND + 1 )) ;;
        h)
            dohelp 0 ;;
        *)
            log_error "Unknown argument -${OPTARG}"
            dohelp 1 ;;
    esac
done

# Set working folders
BASE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR=${BUILD_DIR:-$BASE/build}
# Convert Windows paths to Unix paths if needed (for MSYS2 compatibility)
if command -v cygpath &> /dev/null; then
    BUILD_DIR=$(cygpath -u "$BUILD_DIR")
fi

# Source directories (in repo)
LAUNCHER=$BASE/launcher/
RUNTIME=$BASE/runtime/
DEBUGFILES=$BASE/debug/

# Build directories (ephemeral)
SOURCE=$BUILD_DIR/downloads/
BINARY=$BUILD_DIR/downloads/
RELEASE=$BUILD_DIR/ReleasePackage/
DBSYMBOLS=$BUILD_DIR/debugging-symbols/.debug/

# Determine if we're building x86 or arm64 bit.
if [ "$MSYSTEM" = "UCRT64" ]; then
    log_note "Building x86 64-bit ucrt version!"

    ARCHNUM="64"
    MINGVER=ucrt64
    PMARCH=ucrt-x86_64
elif [ "$MSYSTEM" = "CLANGARM64" ]; then
    log_note "Building ARM 64-bit version!"

    ARCHNUM="64"
    MINGVER=clangarm64
    PMARCH=clang-aarch64
else
    bail "Unknown build system!"
fi
export PMPREFIX="mingw-w64-$PMARCH"

# Early detection
detect_arch_switch $MINGVER

# Common options
TARGET=$BUILD_DIR/target/$MINGVER/
WORK=$BUILD_DIR/work/$MINGVER/
PMTEST="$BUILD_DIR/.pacman-$MINGVER-installed"
PYINST=python

# Check for AppVeyor specific settings
if (($appveyor)); then
    yes=1
    TAR="tar axf"
    export PYTHONHOME=/$MINGVER
    FFPATH=`cygpath -m $APPVEYOR_BUILD_FOLDER`
elif (($ghactions)); then
    yes=1
    TAR="tar axf"
    export PYTHONHOME=/$MINGVER
    FFPATH=`cygpath -m $GITHUB_WORKSPACE/repo`
else
    FFPATH=$WORK/fontforge
    TAR="tar axvf"
fi

# Make the output directories
mkdir -p "$BUILD_DIR"
mkdir -p "$SOURCE"
mkdir -p "$BINARY"
mkdir -p "$WORK"
mkdir -p "$RELEASE/bin"
mkdir -p "$RELEASE/lib"
mkdir -p "$RELEASE/share/glib-2.0"
mkdir -p "$DBSYMBOLS"

# Set pkg-config path to also search mingw libs
export PATH="$TARGET/bin:$PATH"
export PKG_CONFIG_PATH="/$MINGVER/lib/pkgconfig:/usr/local/lib/pkgconfig:/lib/pkgconfig:/usr/local/share/pkgconfig"
# aclocal path
export ACLOCAL_PATH="m4:/$MINGVER/share/aclocal"
export ACLOCAL="/bin/aclocal"
export M4="/bin/m4"
# Compiler flags
export LDFLAGS="-L/$MINGVER/lib -L/usr/local/lib -L/lib"
export CFLAGS="-DWIN32 -I/$MINGVER/include -I/usr/local/include -I/include -g"
export CPPFLAGS="${CFLAGS}"
export LIBS=""

# Install all the available precompiled binaries
if (( ! $nomake )) && [ ! -f $PMTEST ]; then
    log_status "First time run; installing MSYS and MinGW libraries..."

    pacman -Syy --noconfirm

    IOPTS="-S --noconfirm --needed"

    if (( $appveyor+$ghactions )); then
        # Try to fix python issues
        pacman -Rcns --noconfirm $PMPREFIX-python3
        # Upgrade gcc
        pacman $IOPTS --nodeps $PMPREFIX-{gcc,gcc-libs}
    fi

    # Install the base MSYS packages needed
    pacman $IOPTS diffutils findutils make patch tar pkgconf winpty

    # Install MinGW related stuff
    pacman $IOPTS $PMPREFIX-{gcc,gmp,ntldd-git,gettext,libiconv,cmake,ninja,ccache}

    ## Other libs
    pacman $IOPTS $PMPREFIX-$PYINST
    pacman $IOPTS $PMPREFIX-$PYINST-pip
    pacman $IOPTS $PMPREFIX-$PYINST-setuptools

    log_status "Installing precompiled devel libraries..."
    # Libraries
    pacman $IOPTS $PMPREFIX-libspiro
    pacman $IOPTS $PMPREFIX-libuninameslist
    pacman $IOPTS $PMPREFIX-{zlib,libpng,giflib,libtiff,libjpeg-turbo,libxml2,potrace}
    pacman $IOPTS $PMPREFIX-{freetype,fontconfig,glib2,pixman,harfbuzz,woff2}
    
    if (($qt)); then
        pacman $IOPTS $PMPREFIX-{qt6,qt6-tools}
    else
        pacman $IOPTS $PMPREFIX-{gtk3,gtkmm3}
    fi

    touch $PMTEST
    log_note "Finished installing precompiled libraries!"
else
    log_note "Detected that precompiled libraries are already installed."
    log_note "  Delete '$PMTEST' and run this script again if"
    log_note "  this is not the case."
fi # pacman installed

FREETYPE_VERSION="$(pacman -Qi $PMPREFIX-freetype | awk '/Version/{print $3}' | cut -d- -f1)"
FREETYPE_NAME="freetype-${FREETYPE_VERSION}"
FREETYPE_ARCHIVE="${FREETYPE_NAME}.tar.xz"
if [ -z "$FREETYPE_VERSION" ]; then
    bail "Failed to infer the installed FreeType version"
fi
log_note "Inferred installed FreeType version as $FREETYPE_VERSION"

PYVER="$(pacman -Qi $PMPREFIX-${PYINST} | awk '/Version/{print $3}' | cut -d- -f1 | cut -d. -f1-2)"
if [ -z "$PYVER" ]; then
    bail "Failed to infer the installed Python version"
fi
PYVER="python${PYVER}"
log_note "Inferred installed Python version as $PYVER"

log_status "Retrieving supplementary archives (if necessary)"
get_archive "$SOURCE/$FREETYPE_ARCHIVE" \
            "http://download.savannah.gnu.org/releases/freetype/$FREETYPE_ARCHIVE" \
            "https://sourceforge.net/projects/freetype/files/freetype2/${FREETYPE_VERSION}/${FREETYPE_ARCHIVE}" || bail "FreeType2 archive retreival"

cd $WORK

# run_fontforge, TODO - rethink this, now that gdk is used
if [ ! -f run_fontforge/run_fontforge.complete ]; then
    log_status "Building run_fontforge..."
    mkdir -p run_fontforge
    cd run_fontforge
    windres "$LAUNCHER/run_fontforge.rc" -O coff -o run_fontforge.res
    gcc -Wall -Werror -pedantic -std=c99 -O2 -mwindows -municode -o run_fontforge.exe "$LAUNCHER/run_fontforge.c" run_fontforge.res \
    || bail "run_fontforge"
    touch run_fontforge.complete
    cd ..
fi

if (($depsonly)); then
    log_note "Installation of dependencies complete."
    exit 0
fi

if (( ! $nomake )); then
    # For the source only; to enable the debugger in FontForge
    if [ ! -d "$WORK/$FREETYPE_NAME" ]; then
        log_status "Extracting the FreeType $FREETYPE_VERSION source to $WORK..."
        tar -C "$WORK" -axf "$SOURCE/$FREETYPE_ARCHIVE" || bail "FreeType extraction"
    fi

    log_status "Finished installing prerequisites, attempting to install FontForge!"
    # fontforge
    if (( ! ($appveyor+$ghactions) )) && [ ! -d fontforge ]; then
        log_status "Cloning the fontforge repository from https://github.com/$github/fontforge..."
        git clone "https://github.com/$github/fontforge" || bail "Cloning fontforge"
        cd "fontforge"
    else
        cd "$FFPATH";
    fi

    if [ -f CMakeLists.txt ]; then
        if [ ! -f build/fontforge.configure-complete ] || (($reconfigure)); then
            log_status "Running the configure script..."
            
            if (($appveyor+$ghactions)); then
                EXTRA_CMAKE_OPTS="-DENABLE_FONTFORGE_EXTRAS=yes -DENABLE_DOCS=yes -DSPHINX_USE_VENV=yes"
            else
                log_note "Will use ccache when building FontForge"
                EXTRA_CMAKE_OPTS="-DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_BUILD_TYPE=Debug"
            fi
            
            if (($qt)); then
                EXTRA_CMAKE_OPTS="-DENABLE_QT=yes"
            fi

            mkdir -p build && cd build
            time cmake -GNinja \
                -DCMAKE_INSTALL_PREFIX="$TARGET" \
                -DENABLE_FREETYPE_DEBUGGER="$WORK/$FREETYPE_NAME" \
                -DENABLE_LIBREADLINE=no \
                -DPython3_EXECUTABLE=/$MINGVER/bin/python.exe \
                $EXTRA_CMAKE_OPTS \
                .. \
                || bail "FontForge configure"
            touch fontforge.configure-complete
            cd "$FFPATH"
        fi
        
        log_status "Compiling FontForge..."
        time ninja -C build || bail "FontForge build"

        if (($appveyor+$ghactions)); then
            log_status "Running the test suite..."
            CTEST_PARALLEL_LEVEL=100 ninja -C build check || bail "FontForge check"
        fi

        log_status "Installing FontForge..."
        ninja -C build install || bail "FontForge install"
    else
        if [ ! -f fontforge.configure-complete ] || (($reconfigure)); then
            log_status "Running the configure script..."

            if [ ! -f configure ]; then
                log_note "No configure script detected; running ./boostrap..."
                ./bootstrap --force || bail "FontForge autogen"
            fi

            # libreadline is disabled because it causes issues when used from the command line (e.g Ctrl+C doesn't work)
            # windows-cross-compile to disable check for libuuid
            # gdi32 linking is needed for AddFontResourceEx
            LIBS="${LIBS} -lgdi32" \
            PYTHON=$PYINST \
            time ./configure \
                --prefix $TARGET \
                --enable-static \
                --enable-shared \
                --enable-gdk=gdk3 \
                --with-freetype-source="$WORK/$FREETYPE_NAME" \
                --without-libreadline \
                --enable-fontforge-extras \
                --enable-woff2 \
                || bail "FontForge configure"
            touch fontforge.configure-complete
        fi

        log_status "Compiling FontForge..."
        time make -j$(($(nproc)+1)) || bail "FontForge make"

        if (($appveyor+$ghactions)); then
            log_status "Running the test suite..."
            make check -j$(($(nproc)+1)) || bail "FontForge check"
        fi

        log_status "Installing FontForge..."
        make -j$(($(nproc)+1)) install || bail "FontForge install"
    fi
fi

log_status "Assembling the release package..."

log_status "Copying runtime templates..."
cp -f "$RUNTIME/fontforge.bat" "$RELEASE/"
cp -f "$RUNTIME/fontforge-console.bat" "$RELEASE/"
cp -f "$RUNTIME/CHANGELOG.txt" "$RELEASE/"

log_status "Copying glib spawn helpers..."
strip "/$MINGVER/bin/gspawn-win$ARCHNUM-helper.exe" -so "$RELEASE/bin/gspawn-win$ARCHNUM-helper.exe" || bail "Glib spawn helper not found!"
strip "/$MINGVER/bin/gspawn-win$ARCHNUM-helper-console.exe" -so "$RELEASE/bin/gspawn-win$ARCHNUM-helper-console.exe" || bail "Glib spawn helper not found!"

log_status "Copying the shared folder of FontForge..."
cp -rf $TARGET/share/fontforge "$RELEASE/share/"
cp -rf $TARGET/share/locale "$RELEASE/share/"
rm -f "$RELEASE/share/prefs"

log_note "Installing custom binaries..."
cd $WORK

# potrace - http://potrace.sourceforge.net/#downloading
if [ ! -f $RELEASE/bin/potrace.exe ]; then
    log_status "Installing potrace..."
    strip "/$MINGVER/bin/potrace.exe" -so $RELEASE/bin/potrace.exe
fi


log_status "Installing run_fontforge..."
strip $WORK/run_fontforge/run_fontforge.exe -so "$RELEASE/run_fontforge.exe" \
    || bail "run_fontforge"

log_status "Copying UI fonts..."
#Remove the old/outdated Inconsolata/Cantarell from pixmaps
rm "$RELEASE/share/fontforge/pixmaps/"*.ttf > /dev/null 2>&1
rm "$RELEASE/share/fontforge/pixmaps/"*.otf > /dev/null 2>&1
#Copy standard fonts
mkdir -p "$RELEASE/share/fonts"
cp $TARGET/share/fontforge/pixmaps/Cantarell* "$RELEASE/share/fonts" 2>/dev/null || true

log_status "Copying sfd icon..."
cp "$LAUNCHER/sfd-icon.ico" "$RELEASE/share/fontforge/"

log_status "Copying the Python executable and libraries..."
# Name the python binary to something custom to avoid clobbering any Python installation that the user already has
strip "/$MINGVER/bin/$PYINST.exe" -so "$RELEASE/bin/ffpython.exe"
cd $BASE
if [ -d "$RELEASE/lib/$PYVER" ]; then
    log_note "Skipping python library copy because folder already exists, and copying is slow."
else
    if [ ! -d "$BINARY/$PYVER" ]; then
        log_note "Python folder not found - running 'strip-python'..."
        $BASE/strip-python.sh "$PYVER" || bail "Python generation failed"
    fi
    cp -r "$BINARY/$PYVER" "$RELEASE/lib" || bail "Python folder could not be copied"
fi
if [ -d "$RELEASE/lib/tcl8.6" ]; then
    log_note "Skipping copying tcl/tk folder because it already exists"
else
    log_status "Copying tcl/tk..."
    cp -r /$MINGVER/lib/{tcl,tk}8.* "$RELEASE/lib/" ||  bail "Couldn't copy tcl/tk"
fi

if [ -d "$RELEASE/lib/gdk-pixbuf-2.0" ]; then
    log_note "Skipping copying gdk-pixbuf folder because it already exists"
else
    log_status "Copying gdk-pixbuf..."
    cp -r /$MINGVER/lib/gdk-pixbuf-2.0 "$RELEASE/lib/" ||  bail "Couldn't copy gdk-pixbuf"
fi

if [ -d "$RELEASE/share/glib-2.0/schemas" ]; then
    log_note "Skipping copying schemas folder because it already exists"
else
    log_status "Copying schemas..."
    cp -r /$MINGVER/share/glib-2.0/schemas "$RELEASE/share/glib-2.0/" ||  bail "Couldn't copy schemas"
fi

cd $WORK

log_status "Stripping Python cache files (*.pyc,*.pyo,__pycache__)..."
find "$RELEASE/lib/$PYVER" -regextype sed -regex ".*\.py[co]" | xargs rm -rfv
find "$RELEASE/lib/$PYVER" -name "__pycache__" | xargs rm -rfv

PY_DLLS_SRC_PATH=`/$MINGVER/bin/python.exe -c "import sysconfig as sc; print(sc.get_path('platlib', vars={'platbase': '.'}))"`

log_status "Copying the Python extension dlls..."
cp -f "$TARGET/$PY_DLLS_SRC_PATH/fontforge.pyd" "$RELEASE/lib/$PYVER/site-packages/" || bail "Couldn't copy pyhook dlls"
cp -f "$TARGET/$PY_DLLS_SRC_PATH/psMat.pyd" "$RELEASE/lib/$PYVER/site-packages/" || bail "Couldn't copy pyhook dlls"


ffex=`which fontforge.exe`
MSYSROOT=`cygpath -w /`
FFEXROOT=`cygpath -w $(dirname "${ffex}")`
log_note "The executable: $ffex"
log_note "MSYS root: $MSYSROOT"
log_note "FFEX root: $FFEXROOT"

fflibs=`ntldd -D "$(dirname \"${ffex}\")" -R "$ffex" $RELEASE/lib/$PYVER/lib-dynload/*.dll $RELEASE/lib/gdk-pixbuf-2.0/*/loaders/*.dll $RELEASE/bin/potrace.exe \
| grep =.*dll \
| sed -e '/^[^\t]/ d'  \
| sed -e 's/\t//'  \
| sed -e 's/.*=..//'  \
| sed -e 's/ (0.*)//' \
| grep -F -e "$MSYSROOT" -e "$FFEXROOT" \
`

log_status "Copying the FontForge executable..."
strip "$ffex" -so "$RELEASE/bin/fontforge.exe"
#cp "$ffex" "$RELEASE/bin/fontforge.exe"
objcopy --only-keep-debug "$ffex" "$DBSYMBOLS/fontforge.debug"
objcopy --add-gnu-debuglink="$DBSYMBOLS/fontforge.debug" "$RELEASE/bin/fontforge.exe"
log_status "Copying the libraries required by FontForge..."
for f in $fflibs; do
    filename="$(basename $f)"
    filenoext="${filename%.*}"
    strip "$f" -so "$RELEASE/bin/$filename"
    #cp "$f" "$RELEASE/bin/"
    if [ -f "$TARGET/bin/$filename" ]; then
        #Only create debug files for the ones we compiled!
        objcopy --only-keep-debug "$f" "$DBSYMBOLS/$filenoext.debug"
        objcopy --add-gnu-debuglink="$DBSYMBOLS/$filenoext.debug" "$RELEASE/bin/$filename"
    fi
done

if (($qt)); then
    log_status "Running windeployqt..."
    windeployqt-qt6 "$RELEASE/bin/fontforge.exe"
fi

log_status "Generating the version file..."
current_date=`date "+%c %z"`
actual_branch=`git -C $FFPATH rev-parse --abbrev-ref HEAD`
actual_hash=`git -C $FFPATH rev-parse HEAD`
if (($appveyor+$ghactions)); then
    version_hash=`git -C $FFPATH ls-remote origin master | awk '{ printf $1 }'`
else
    version_hash=`git -C $FFPATH rev-parse master`
fi


printf "FontForge Windows build ($ARCHNUM-bit)\r\n$current_date\r\n$actual_hash [$actual_branch]\r\nBased on master: $version_hash\r\n\r\n" > $RELEASE/VERSION.txt
printf "A copy of the changelog follows.\r\n\r\n" >> $RELEASE/VERSION.txt
cat $RELEASE/CHANGELOG.txt >> $RELEASE/VERSION.txt

if (($makedebug)); then
    log_note "Adding in debugging utilities..."
    cp -f "$DEBUGFILES/ffdebugscript.txt" "$RELEASE/" || bail "Couldn't copy debug script"
    cp -f "$DEBUGFILES/fontforge-debug.bat" "$RELEASE/" || bail "Couldn't copy fontforge-debug.bat"
    cp -f "$BINARY/gdb-$ARCHNUM.exe" "$RELEASE/bin/gdb.exe" || bail "Couldn't copy GDB"
    cp -f "$BINARY/wtee.exe" "$RELEASE/bin/" || bail "Couldn't copy wtee"
    cp -rf "$DBSYMBOLS" "$RELEASE/bin/" || bail "Couldn't copy debugging symbols"
fi

# Might as well auto-generate everything
#sed -bi "s/^git .*$/git $version_hash ($current_date).\r/g" $RELEASE/VERSION.txt

log_note "Build complete."
