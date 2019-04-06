# Installing FontForge from Git Source Code

## Doing It By Hand

Clone a copy of the Github source repository:

```sh
mkdir -p ~/src/github.com/fontforge;
cd ~/src/github.com/fontforge;
git clone https://github.com/fontforge/fontforge.git;
```

Install all your typical build tools, build dependencies and runtime dependencies - all the packages that allow you to build of software from source code.
The exact method to do this depends on your OS distribution.
You can see a list of the dependencies for Debian in the [debian/control](https://github.com/fontforge/fontforge/blob/master/debian/control) file, or to generate a list on Debian-like systems with `aptitude` installed, run `sudo ./debian/deb-build-dep`

To download all dependencies on Debian Jessie, run:

```sh
sudo apt-get install autotools-dev libjpeg-dev libtiff5-dev libpng-dev libgif-dev libxt-dev libfreetype6-dev autoconf automake libtool libxml2-dev libuninameslist-dev libspiro-dev python-dev libpango1.0-dev libcairo2-dev chrpath
```

To download all dependencies on Ubuntu, run:

```sh
sudo apt-get install packaging-dev pkg-config python-dev libpango1.0-dev libglib2.0-dev libxml2-dev giflib-dbg libjpeg-dev libtiff-dev uthash-dev libspiro-dev build-essential automake flex bison;
```

Install the *unifont* package to get a full display of the reference glyphs.
[Unifont](http://savannah.gnu.org/projects/unifont) includes glyphs for all Unicode codepoints, and FontForge will use it if it is installed.

```sh
sudo apt-get install unifont;
```

FontForge uses *[libspiro](http://github.com/fontforge/libspiro)* to simplify the drawing of curves.
Download and install the latest code like this:

```
git clone https://github.com/fontforge/libspiro.git
cd libspiro
autoreconf -i
automake --foreign -Wall
./configure
make
sudo make install
cd ..
```

Build *libuninameslist*

FontForge uses [libuninameslist](http://github.com/fontforge/libuninameslist) to access attribute data about each Unicode code point.

Download the code:

```
git clone https://github.com/fontforge/libuninameslist.git
```

Run the following commands in sequence (that is, wait for each one to complete before running the next):

```
cd libuninameslist
autoreconf -i
automake --foreign
./configure
make
sudo make install
cd ..
```

Now run the build and installation scripts, and ensure you can open shared object files:

```sh
cd fontforge;
./bootstrap;
./configure;
make;
sudo make install;
sudo ldconfig;
```

**Attention, Designers Who Love TrueType Hinting:** 
You like to run `./configure` with the `--with-freetype-source`<sup>1</sup> option. 
This option enables advanced features for debugging TrueType font hints, such as stepping through hinting instructions one by one.

<sup>1</sup> use absolute path to freetype source root. i.e. `--with-freetype-source=/abspath/to/freetype`

## Common Problems

* If you can not install due to problems with Python, run ```./configure``` line like this (paths may vary):
```
PYTHON_CFLAGS="-I/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7" \
PYTHON_LIBS="-L/System/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/config" \
./configure \
--disable-programs \
--disable-native-scripting \
--without-x \
--without-cairo \
--without-giflib \
--without-libjpeg \
--without-libtiff \
--without-libpng \
--without-libspiro \
--without-libuninameslist \
--without-libunicodenames \
--without-iconv \
--without-libzmq \
--without-libreadline \
--enable-python-scripting \
--enable-python-extension;
```
Or try this:
```sh
PKG_CONFIG_PATH=/opt/X11/lib/pkgconfig/ \
PYTHON_CFLAGS="-I/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7" \
PYTHON_LIBS="-L/System/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/config" \
./configure;
```

* Problems with lack of libraries can be solved with Homebrew or similar systems:
```
brew install autoconf;
brew install automake;
brew install glib;
brew install pango;
```

* If `make` fails with `no such file or directory msgfmt` then with Homebrew run:
```
brew install gettext;
```
Then edit then your shell profile (eg, `~/.bash_profile` or `~/.zprofile`) and add:
```
export PATH=${PATH}:/usr/local/opt/gettext/bin
```

### Python modules on Debian

Debian-provided python executables by default only look for modules in
`dist-packages` and not `site-packages`. FontForge's build environment uses
`Autoconf`, which does not provide an easy means of specifying an alternate
installation location for python modules. Users building on Debian may
therefore have to move the modules to the appropriate location after running
`make install`, and anyone designing Debian packaging may have to add a hook to
do this.

## On Mac OS X with Homebrew

Homebrew no longer packages FontForge with a UI, but the release GUI app bundles are available from Cask. 
You can install FontForge from the current github source code using a package manager, such as MacPorts or Fink. 

Here is how to install FontForge without a UI:

```
ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```
**Check everything's ok** (fix any issues before moving on)
```
brew doctor
```
**Install dependencies**
```
brew install python gettext libpng jpeg libtiff giflib cairo pango libspiro czmq fontconfig automake libtool pkg-config glib pango
```

**Install the latest release or build from source**  
Release:
```
brew install -v --debug --with-giflib --with-libspiro fontforge
```
Source:
```
brew install -v --debug --with-giflib --with-libspiro fontforge --HEAD
```

If this does not work, please [file an issue](When_Things_Go_Wrong_With_Fontforge_Itself)

To install the UI with Cask:

```
brew tap caskroom/cask; # Install cask
brew cask install fontforge; # Install fontforge
```
