# Installing FontForge from Git Source Code

## On Mac OS X with Homebrew

You can install FontForge from the current github source code using a package manager, such as Homebrew, MacPorts or Fink

We recommend Homebrew, which can be installed by following the instructions at <http://brew.sh>

With Homebrew installed, first install the dependencies, and then FontForge itself - either the most recent release, or the latest git source code.

```sh
# install dependencies;
brew install python gettext libpng jpeg libtiff giflib cairo pango libspiro czmq fontconfig automake libtool pkg-config glib pango;

# install fontforge release;
brew install -v --debug --with-giflib --with-x11 --with-libspiro fontforge;

# or, install fontforge source:
brew install -v --debug --with-giflib --with-x11 --with-libspiro fontforge  --HEAD;
```

If this does not work, please [file an issue](When_Things_Go_Wrong_With_Fontforge_Itself)

## Doing It By Hand

First clone a copy of the Github source repository:

```sh
mkdir -p ~/src/github.com/fontforge;
cd ~/src/github.com/fontforge;
git clone https://github.com/fontforge/fontforge.git;
cd fontforge;
```

Second, install all your typical build tools, build dependencies and runtime dependencies. 
The exact method to do this depends on your OS distribution.
You can see a list of the dependencies for Debian in the [debian/control](https://github.com/fontforge/fontforge/blob/master/debian/control) file, or to generate a list on Debian-like systems with `aptitude` installed, run `sudo ./debian/deb-build-dep`

To download all dependencies on Ubuntu, run:

```sh
sudo apt-get install packaging-dev pkg-config python-dev libpango1.0-dev libglib2.0-dev libxml2-dev giflib-dbg libjpeg-dev libtiff-dev uthash-dev libspiro-dev;
```

Now run the build and installation scripts, and ensure you can open shared object files:

```sh
./bootstrap;
./configure;
make;
sudo make install;
sudo ldconfig;
```

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

* If `make` fails with `no such file or directory msgfmt` then with Hhomebrew run:
```
brew install gettext;
```
Then edit then your shell profile (eg, `~/.bash_profile` or `~/.zprofile`) and add:
```
export PATH=${PATH}:/usr/local/opt/gettext/bin
```
