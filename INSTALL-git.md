Installing FontForge
====================

1. Dependencies
---------------

Note that you should have build tools, build dependencies
and runtime dependencies installed. The exact method to do this
depends on your OS.

On Ubuntu, for example, you should get:

```
sudo apt-get install packaging-dev pkg-config python-dev libpango1.0-dev libglib2.0-dev libxml2-dev giflib-dbg libjpeg-dev libtiff-dev uthash-dev
libspiro-dev
```

2. Installing
-------------

To install from a git checkout:

```
./bootstrap
./configure
make
[sudo] make install
```

**Note:** you can see the build dependencies in the
[debian/control](https://github.com/fontforge/fontforge/blob/master/debian/control)
file (or you can run `sudo ./debian/deb-build-dep` if you have aptitude to automate
this).

3. Linking
----------

If after installation, running `fontforge` gives you some
"*.so cannot open shared object file" style errors, you
might have to run this as well:

```
sudo ldconfig
```

4. Common Mac OS issues
----------

* Problems with Python, run ```./configure``` line like this (paths may vary):
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
--enable-python-extension

# Other solution
PKG_CONFIG_PATH=/opt/X11/lib/pkgconfig/ \
PYTHON_CFLAGS="-I/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7" \
PYTHON_LIBS="-L/System/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/config" \
./configure
```

* Problems with lack of libs (homebrew needed for this code):
```
# Install if needed
brew install autoconf
brew install automake
brew install glib
brew install pango
```

* Make command replies ```no such file or directory msgfmt``` (homebrew needed for this code):
```
brew install gettext
```

Edit then your ```.bash_profile``` or ```.zprofile``` (if you use zsh) and add:
```
export PATH=${PATH}:/usr/local/opt/gettext/bin
```

* You can't find the built FontForge app (paths may vary):
```
mv /usr/local/share/fontforge/osx/FontForge.app /Applications
```
