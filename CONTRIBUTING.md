# Contributing to FontForge

## Reporting An Issue

See [When Things Go Wrong With FontForge Itself](http://designwithfontforge.com/en-US/When_Things_Go_Wrong_With_Fontforge_Itself.html)

## Contributing Code

Merging into the master branch requires review and approval of the pull request
by an active core contributor to the project. The reviewer must fully
comprehend the code being modified, and any changes must be strictly
non-regressive and non-breaking unless discussed in advance on the mailing
list. FontForge is an extremely complex piece of software, and keeping it in
top form requires this level of care. Please understand that large pull
requests may take a long time to be approved or may be declined, even if well
written, due to limited code review resources.

### How To Contribute, Step by Step

Contribute directly to the codebase using GitHub's Pull Requests. 
See [Github Guides](https://guides.github.com/) to learn more, but the basic process is:

- Fork the [FontForge repository](https://github.com/fontforge/fontforge) from GitHub.
- Commit your changes locally using `git`, and push them to your personal fork.
- From the main page of your fork, click on the green “Fork” button in order to submit a Pull
  Request.
- Your pull request will be tested via [Travis CI](https://travis-ci.org/) to automatically indicate that your changes do not prevent compilation. FontForge is a big program, so Travis can easily take over 20 minutes to confirm your changes are buildable. Please be patient. More details about using Travis are below.
- If it reports back that there are problems, you can follow the "Details" link to check the log report for your pull request to see what the problem was. 

Fontforge supports Python `>=` 2.6 and is fully compatible with Python 3 through at least version 3.7. 

### Debugging FontForge

FontForge is most commonly debugged by contributors with the [GNU Debugger](https://www.gnu.org/software/gdb/) (`gdb`). While hacking on FontForge, it is common to have multiple different versions of FontForge on your system. Depending on your `PATH`. `LD_LIBRARY_PATH`, `gdb`'s internal `solib-search-path`, whether FontForge was compiled statically, in some cases the shell you are using, and potentially other subtle factors, you could be testing the wrong version of FontForge even if you are pointing `gdb` to the right `fontforge` executable.

**If testing Git patches locally, be sure to run `gdb` as `libtool --mode=execute gdb fontforgeexe/fontforge`** instead of just `gdb fontforgeexe/.libs/fontforge` or similar to avoid `gdb` potentially loading the system's FontForge library while executing the local `fontforge` binary.

### Coding Style

Some of these guidelines are not followed in the oldest code in the repository, however we want to use them for all new code since 2012 when FontForge transitioned from a lone-genius project to a collaborative community project.

1. One statement per line, to make semi-automatic processing and reading of diffs much easier.
2. Boolean Variables should use `stdbool.h`'s names `true` and `false`, not an integer ([reference](https://github.com/fontforge/fontforge/issues/724)).
3. `return` statements should be inline with the indentation level they are being put on; don't put them at the left margin as in much of the existing code ([reference](https://github.com/fontforge/fontforge/issues/1208)).

Beyond these guidelines, try to follow the style used in the file being modified.

### People To Ask

Various areas of the codebase have been worked on by different people in recent years, so if you are unfamiliar with the general area you're working in, please feel free to chat with people who have experience in that area:

* Build System: Debian - Frank Trampe (frank-trampe)
* Build System: OS X (Application bundle, Homebrew) - Jeremy Tan (jtanx)
* Build System: Windows - Jeremy Tan (jtanx)
* Feature: UFO import/export - Frank Trampe (frank-trampe)
* Feature: Collab - Dr Ben Martin (monkeyiq)
* Feature: Python interface - Skef Iterum (skef)
* Crashes: Frank Trampe, Adrien Tetar (adrientetar)

### Accessing Travis and Appveyor Build Archives

After each push request `Appveyor` will attempt to build and package
a Windows installer. When that build is successful it can be accessed
by following the `Appveyor` "Details" link and choosing the "Artifacts"
tab. 

Note: `Appveyor` builds with the `FF_PORTABLE` flag, which changes various
initialization and configuration search paths. 

The Travis system also attempts to build a Mac OS X application and a
Linux Appimage. When those builds are successful they can be downloaded
from:

    https://dl.bintray.com/fontforge/fontforge/

## Building Packages

### Building Latest Source

If you're not familiar with compiling source code, the follow provides some
introductory guidance.

Building FontForge from the latest source is described in
[INSTALL-git.md](https://github.com/fontforge/fontforge/blob/master/INSTALL-git.md)

You can download the latest source as a snapshot of the repository's current
state as a zip,
[master.zip](https://github.com/fontforge/fontforge/archive/master.zip), or
clone it using git with this command:

    git clone git://github.com/fontforge/fontforge.git

For building from source for a release versions, go to the [Release
Page](https://github.com/fontforge/fontforge/releases) to find out a release
tag, and in your cloned copy of the repo use that tag. For example:

     git checkout tags/20141014

After the checkout, you need to first run the bootsrap script to checkout
dependencies and generate the configure script:

    ./bootstrap

The configure script allows you to turn off and on various features of
FontForge that might not be appropriate for your system. For a complete list of
options, type

     ./configure --help

Some of the most useful options are:

- **Building FontForge without X.**
  If you don't want to install X11 on your system, you can use FontForge as a
  command line tool which can execute scripts to manipulate fonts.
  FontForge's scripting language is described in detail [in the section 
  on scripting](scripting.html), or the [section on python scripting](python.html).

     `./configure --without-x`

- **Building with the GDK UI**
  FontForge now supports a GDK compatibility layer. This is compatible with X,
  but also works with other "native" GDK compatibility layers. To use this
  alternate UI implementation use

     `./configure --enable-gdk`

- **Installing FontForge somewhere other than `/usr/local`**
  If you want to install FontForge in a different directory (say in `/usr/bin`)

     `./configure --prefix=/usr`

### Dependencies (External Libraries/Helper Programs)

FontForge tries to avoid hard dependencies.
If a library is missing then FontForge will (in most cases, but not on cygwin) be able to continue to run, it will just lack whatever functionality the library provides.
So if you don't need to import tiff images, you don't need libtiff.
If you don't need to handle SVG fonts you don't need libxml2, etc.

#### Executables

If you want to do autotracing around character images you should also download either

- Peter Selinger's [potrace](http://potrace.sf.net/)
- Martin Weber's [autotrace program.](http://sourceforge.net/projects/autotrace/)

#### Libraries

If your system comes with a package manager, use it. 
It makes installing thee libraries easier.

None are required for the proper compilation/execution of FontForge, if the libraries are not present they will not be used.
(If the machine on which your executable was build didn't have them, then you must not only install the libraries, but also **rebuild FontForge from source**. 
If your machine doesn't have them and you want them they are available from:

-   Image Libraries (to allow FontForge to import images in those
    formats generally used as backgrounds for autotracing)
    -   [libpng](http://www.libpng.org/pub/png/libpng.html) (and required helper [zlib](http://www.zlib.net/))
    -   [libtiff](http://www.libtiff.org/)
    -   [libungif](http://gnuwin32.sourceforge.net/packages/libungif.htm)
    -   [libjpeg](http://www.ijg.org/)
-   [libxml2](http://xmlsoft.org/) To parse SVG files and fonts
-   [libspiro](https://github.com/fontforge/libspiro) Raph Levien's clothoid to bezier spline conversion routines. If this is available FontForge will allow you to edit with clothoid splines (spiro).
-   [libuninameslist](https://github.com/fontforge/libuninameslist) To display unicode names and annotations.
-   [libiconv](http://www.gnu.org/software/libiconv/) Only important for systems with no built-in iconv().
    If not present FontForge contains a minimal version of the library which allows it to work.
    But if you want to use libiconv you must configure it with `--enable-extra-encodings`, as FontForge requires Shift-JIS.
-   [freetype](http://freetype.org/)
    To do a better job rasterizing bitmaps, and to enable the truetype debugger.
    Some of FontForge's commands depend on you compiling freetype with the byte code interpreter enabled.
    It used to be disabled by default because of some [patents granted to Apple](http://freetype.org/patents.html).
    Now that they have expired, you no longer need to worry about this, unless your setup happens to use an old library version.
    Then you may enable the interpreter by setting the appropriate macro in `*.../include/freetype/config/ftoption.h*` before you build the library (see the README.UNX file on the top level of the freetype distribution).
    To enable the truetype debugger, FontForge needs to have the freetype source directories available when it is built (there are some include files there which it depends on.)
-   libintl Is standard on most unixes. It is part of the fink package on the mac. Handles UI localization.
-   [libpython](http://www.python.org/) If present when FontForge is compiled, allows the user to execute python scripts within FontForge (and you can configure FontForge so that FontForge's functionality can be imported into python -- that is FontForge both *extends* and *embeds* python)
-   [libX](http://x.org/) Normally FontForge depends on the X11 windowing system, but if you are just interested in the scripting engines (with no user interface), it may be built on systems without X (the configure script should figure this out).
-   [libcairo](http://www.cairographics.org/) Cairo handles drawing anti-aliased splines in the outline glyph view. It is dependent on libfontconfig, libXft and perhaps other libraries.
-   [libpango](http://www.pango.org/) Pango draws text for complex scripts. It depends on glib-2.0, libfontconfig, libfreetype, libXft, and perhaps other libraries.

### Extra Files

If you want to edit CID keyed fonts you need the character set descriptions in [`/contrib/cidmap`](https://github.com/fontforge/fontforge/tree/master/contrib/cidmap)

You might want to pull down some old unicode bitmap fonts.

-   [The unifont](http://czyborra.com/unifont/)
-   [The FreeFont project](http://www.nongnu.org/freefont/)
-   [X fixed](http://www.cl.cam.ac.uk/~mgk25/ucs-fonts.html)
-   [Computer Modern Unicode fonts](http://canopus.iacp.dvo.ru/~panov/cm-unicode/) - [Unicode Font Guide for Free/Libre Open Source Operating Systems](http://eyegene.ophthy.med.umich.edu/unicode/fontguide/)

### Building a Debian source package

A Debian source package consists of a source tarball (with specific metadata) and several accompanying files and allows one to build a product in a neutral build environment.

A source package is specific to the distribution (but not the architecture) that it targets. 
The most common build target is currently Ubuntu Precise, a long-term support release with Launchpad build support. 
A binary package built on and for Precise also installs and functions correctly on Debian testing and on Ubuntu Trusty.

After downloading the FontForge source, change into the source directory and run

    ./bootstrap; 
    ./configure; 
    make \
    F_METADATA_REGENERATE=1 \
    F_PACKAGER_NAME="Name" \
    F_PACKAGER_MAIL="you@domain.tld" \
    UBUNTU_RELEASE="precise" \
    deb-src ;

One can omit the `UBUNTU_RELEASE` argument and use an arbitrary `DEB_OS_RELEASE` argument instead in order to target a Debian distribution. 
One can use a custom debian directory by populating it and omitting the `F_METADATA_REGENERATE` option.

Upon successful completion, building of the source package will leave several files in the parent directory, with names like this:

    fontforge_20140813-92-g203bca8-0ubuntu1~precise.dsc
    fontforge_20140813-92-g203bca8-0ubuntu1~precise_source.build
    fontforge_20140813-92-g203bca8-0ubuntu1~precise_source.changes
    fontforge_20140813-92-g203bca8-0ubuntu1~precise.tar.gz

In order to upload to a Launchpad repository for building, one can then run dput on the `.changes` file with the target repository as the first argument, like this:

    dput ppa:fontforge/fontforge fontforge_20140813-92-g203bca8-0ubuntu1~precise_source.changes

This will, upon success, leave a file named something like `fontforge_20140813-92-g203bca8-0ubuntu1~precise_source.ppa.upload` which blocks duplicate uploads.

Upon validation of the uploaded package, Launchpad will build the package for all supported architectures.
One can then copy the binary packages from precise to other Ubuntu versions via the Launchpad web interface.

See [here](https://help.launchpad.net/Packaging/PPA) for more information about Launchpad.

One can also build a binary package from the source package locally. 
Simply extract the `tar.gz` file generated previously into a new directory, enter the directory, and run `debuild`

### Building a Red Hat source package (.rpm)

One can build a Red Hat source package by entering a clean FontForge source tree and running

    ./bootstrap; 
    ./configure; 
    make F_METADATA_REGENERATE=1 rpm-src;

This will leave a `tar.gz` file and a `.spec` file in the parent directory

In order to build the binary package locally, copy the source file to `~/rpmbuild/SOURCES` and the spec file to `~/rpmbuild/SPECS`, and run `rpmbuild -ba ~/rpmbuild/SPECS/(name of spec file)`. 
For Example:

    mkdir -p ~/rpmbuild/SOURCES/;
    cp TARBALL ~/rpmbuild/SOURCES/;
    rpmbuild -ba --nodeps SPECFILE

Upon success, this will leave binary packages in `~/rpmbuild/RPMS` and source packages in `~/rpmbuild/SRPMS`.

You may need to install dependencies, typically packaged for Fedora-derived systems as:

    rpm-devel rpm-build git perl autoconf automake tar libtool glibc-devel m4 gcc cpp python-devel libjpeg-turbo-devel libtiff-devel libpng-devel giflib-devel freetype-devel uuid-devel libtool-ltdl-devel bzip2-devel libxml2-devel libuninameslist-devel libspiro-devel pango-devel cairo-devel chrpath;

### Building a Mac OS X app bundle

There are two scripts to build an app bundle, which one you use will depend on if you are using homebrew or macports on your OSX machine.
Both scripts aim to create a complete stand alone bundle of FontForge and all the libraries, data files, and dependencies that it needs, including Python.

The `osx/create-osx-app-bundle.sh` works on a macports machine whereas the `travis-scripts/ffosxbuild.sh` script is for homebrew.
It is possible that these scripts will be merged into a single script that can work on both platforms in the future.

It's highly recommended to not have both macports and homebrew installed, as it can cause conflicts in which dependencies are used at build time.

For macports there is an `osx/Portfile` and for homebrew you might like to see `travis-scripts/fontforge.rb` for changes. 
Both of these files are geared to build current git master.

The `travis-scripts/ffosxbuild.sh` has been used to create bundles off Travis CI builds. It expects that FontForge will be installed to a prefix (i.e. with the `--prefix` option). It is then invoked as such:

```
./ffosxbuild.sh /path/to/prefix version-hash
```

This will create an app bundle as `FontForge.app`, and also create a corresponding dmg. The application bundle is relocatable.

The `osx/create-osx-app-bundle.sh` script was used since early 2013 to create the daily bundle for FontForge.
It is no longer actively used, having been superseded by `travis-scripts/ffosxbuild.sh`.

This script expects to be run from the root directory of where FontForge was compiled.
For example, doing the below command to install fontforge on OSX:

    sudo port -v -k install fontforge

and then move to the `/opt/local/var/macports/build/.../work/fontforge-2.0.0_beta1` and run the `./osx/create-osx-app-bundle.sh` script from that directory.

