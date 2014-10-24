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
