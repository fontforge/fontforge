To install from a git checkout:

```
./bootstrap
./configure
make
[sudo] make install
```

Note that you should have build tools, build dependencies
and runtime dependencies installed. The exact method to do this
depends on your OS. On Ubuntu, for example, you should get:
 - [packaging-dev](https://apps.ubuntu.com/cat/applications/packaging-dev/)
   should get you covered for building
 - you can see the build dependencies in the
   [debian/control](https://github.com/fontforge/fontforge/blob/master/debian/control)
   file

(or you can run `sudo ./debian/deb-build-dep` if you have aptitude to automate this)

 - for the runtime dependencies you also need
   to install [libspiro](https://github.com/fontforge/libspiro)
 
 
If after installation, running `fontforge` gives you some
"*.so cannot open shared object file" style errors, you
might have to run this as well:

    sudo ldconfig
