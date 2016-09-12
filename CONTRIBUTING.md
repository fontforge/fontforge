# Contributing to FontForge

## Reporting An Issue

See [When Things Go Wrong With FontForge Itself](http://designwithfontforge.com/en-US/When_Things_Go_Wrong_With_Fontforge_Itself.html)

## Contributing Code

We request that all pull requests be actively reviewed by at least one other developer, and in 2014 and for 2015, Frank Trampe has volunteered to do that if no one else gets there first. 
He aims to review your PR within 1 week of your most recent commit.

### How To Contribute, Step by Step

Contribute directly to the codebase using GitHub's Pull Requests. 
See [Github Guides](https://guides.github.com/) to learn more, but the basic process is:

- Fork the [FontForge repository](https://github.com/fontforge/fontforge) from GitHub.
- Commit your changes locally using `git`, and push them to your personal fork.
- From the main page of your fork, click on the green “Fork” button in order to submit a Pull
  Request.
- Your pull request will be tested via [Travis CI](https://travis-ci.org/) to automatically indicate that your changes do not prevent compilation. FontForge is a big program, so Travis can easily take over 20 minutes to confirm your changes are buildable. Please be patient. More details about using Travis are below.
- If it reports back that there are problems, you can log into the Travis system and check the log report for your pull request to see what the problem was. If no error is shown, just re-run the Travis test for your pull-request (that failed) to see a fresh report since the last report may be for someone else that did a later pull request, or for mainline code. If you add new code to fix your issue/problem, then take note that you need to check the next pull request in the Travis system. Travis issue numbers are different from GitHub issue numbers.

Fontforge supports Python `>=` 2.6, but the testsuite is solely Python 3-compatible.

### Coding Style

Some of these guidelines are not followed in the oldest code in the repository, however we want to use them for all new code since 2012 when FontForge transitioned from a lone-genius project to a collaborative community project.

1. One statement per line, to make semi-automatic processing and reading of diffs much easier.
2. Boolean Variables should use `stdbool.h`'s names `true` and `false`, not an integer ([reference](https://github.com/fontforge/fontforge/issues/724)).
3. `return` statements should be inline with the indentation level they are being put on; don't put them at the left margin as in much of the existing code ([reference](https://github.com/fontforge/fontforge/issues/1208)).
4. Use POSIX/gnulib APIs in preference to glib, e.g. `strdup` instead of `g_strdup` and `xvasprintf` instead of `g_printf_strdup`. 
This minimizes the impact of non-standard types and functions.

### People To Ask

Various areas of the codebase have been worked on by different people in recent years, so if you are unfamiliar with the general area you're working in, please feel free to chat with people who have experience in that area:

* Build System: Debian - Frank Trampe (frank-trampe)
* Build System: OS X (Application bundle, Homebrew) - Dr Ben Martin (monkeyiq)
* Build System: Windows - Jeremie Tan (jtanx)
* Feature: Collab - Dr Ben Martin
* Feature: Multi Glyph CharView - Dr Ben Martin
* Feature: UFO import/export - Frank Trampe (frank-trampe)
* Crashes: Frank Trampe, Adrien Tetar (adrientetar)

### Using Travis CI

The Travis CI system is used to verify changes that a pull request
makes. The CI run will also build binary osx packages to allow members
of the FontForge team to quickly test out your changes. Because it is
a security risk to allow github forks access to secure variables you
have to push your branch into the main fontforge repository for binary
packages to be built. 

To have Travis CI build OSX packages, first create a branch with your username 
as the first part and push it to the main repository using the following setup:

    $ cat .git/config
    ...
    [remote "upstreampush"]
            url = git@github.com:fontforge/fontforge.git
    ...
    $ git push --verbose upstreampush johndoe/2014/aug/this-will-help-because-of-yyy

That will make a branch this-will-help-because-of-yyy in the
fontforge/fontforge repository which you can then create a pull
request from. Because that pull request is coming from the
fontforge/fontforge repository, Travis CI will allow secure
environment variables to be used and an OSX bundle will be created for
the pull request.

You can update your local branch, commit to it, and then run the above
to push that update upstream to your branch in fontforge/fontforge and
that will trigger another OSX bundle build.

Builds are synced over to the bigv server, currently at /tmp/bigvbase
though the base directory may change in the future. Relative to the
base directory you will see subdirectories of the form shown below. If
all has gone well you should see a FontForge.app.zip file in that
subdirectory which is around 60mb in size. If you have access to OSX
10.9.3 or later you can install that app.zip and test it out.

    2014-08-20-1664-0d986d9242da9e741d50c5f543faba2f37b7723d
    yyyy-mm-dd-PRnum-githash

#### Travis SSH keys

Inside the travis-scripts subdirectory you will see a encrypt-key.sh
script which can be used to marshal and carve up an SSH key to setup
Travis CI secure environment variables that allow the CI system to
reconstitute the SSH key.

Usage of encrypt-key.sh is as follows, where fontforge/fontforge is
the github repository that you want to encrypt the SSH key for.

    ./encrypt-key.sh the-ssh-private-key-filename fontforge/fontforge

The Linux and OSX Travis build scripts have been updated to
reconstitute the SSH key. See the travis-scripts/common.sh file for
how to easily use the key to upload information to the server.

## Building Packages

### Building Latest Source

If you're not familiar with compiling source code, the follow provides some introductory guidance.

Building FontForge from the latest source is described in [INSTALL-git.md](https://github.com/fontforge/fontforge/blob/master/INSTALL-git.md)

You can download the latest source as a snapshot of the repository's current state as a zip, [master.zip](https://github.com/fontforge/fontforge/archive/master.zip), or clone it using git with this command:

    git clone git://github.com/fontforge/fontforge.git

For building from source for a release versions, go to the [Release Page](https://github.com/fontforge/fontforge/releases) to find out a release tag, and in your cloned copy of the repo use that tag. For example:

     git checkout tags/20141014

The configure script allows you to turn off and on various features of FontForge that might not be appropriate for your system. For a complete list of options, type

     ./configure --help

Some of the most useful options are:

- **Building FontForge without X.**
  If you don't want to install X11 on your system, you can use FontForge as a command line tool which can execute scripts to manipulate fonts.
  FontForge's scripting language is described in detail [in the section on scripting](scripting.html), or the [section on python scripting](python.html).

     `./configure --without-x`

- **Building FontForge (also) as a python extension**
  If you want to write python scripts in normal python (as opposed to within the python embedded in FontForge)

     `./configure --enable-pyextension`

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
    -   [libpng](http://www.libpng.org/pub/png/libpng.html) (and required helper [zlib](http://www.gzip.org/zlib/))
    -   [libtiff](http://www.libtiff.org/)
    -   [libungif](http://gnuwin32.sourceforge.net/packages/libungif.htm)
    -   [libjpeg](http://www.ijg.org/)
-   [libxml2](http://xmlsoft.org/) To parse SVG files and fonts
-   [libspiro](https://github.com/fonrforge/libspiro) Raph Levien's clothoid to bezier spline conversion routines. If this is available FontForge will allow you to edit with clothoid splines (spiro).
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

-   [Kanou's fontview fonts](http://khdd.net/kanou/fonts/ff/fontviewfont-en.html) [![](flags/Nisshoki-Japan.png)](http://khdd.net/kanou/fonts/ff/fontviewfont.html)
-   [The unifont](http://czyborra.com/unifont/)
-   [ClearlyU's font](http://clr.nmsu.edu/~mleisher/cu.html)
-   [The FreeFont project](http://www.nongnu.org/freefont/)
-   [X fixed](http://www.cl.cam.ac.uk/~mgk25/ucs-fonts.html)
-   [Computer Modern Unicode fonts](http://canopus.iacp.dvo.ru/~panov/cm-unicode/) - [Unicode Font Guide for Free/Libre Open Source Operating Systems](http://eyegene.ophthy.med.umich.edu/unicode/fontguide/)

FontForge has [conventions for non-BMP unicode bitmap fonts](http://fontforge.github.io/nonBMP/). 
To install these, put them in a directory, and in that directory type:

     mkfontdir
     xset fp+ `pwd`

You should make sure that the xset line happens whenever X is started on your machine (put it in your .xsession file).

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

The `osx/create-osx-app-bundle.sh` works on a macports machine whereas the `travis-scripts/create-osx-app-bundle-homebrew.sh` script is for homebrew. 
It is possible that these scripts will be merged into a single script that can work on both platforms in the future.

For macports there is an `osx/Portfile` and for homebrew you might like to see `travis-scripts/fontforge.rb` for changes. 
Both of these files are geared to build current git master.

The `osx/create-osx-app-bundle.sh` script has been used since early 2013 to create the daily bundle for FontForge.
Both the bundle scripts expect to be run from the root directory of where FontForge was compiled. 
For example, doing the below command to install fontforge on OSX:

    sudo port -v -k install fontforge

and then move to the `/opt/local/var/macports/build/.../work/fontforge-2.0.0_beta1` and run the `./osx/create-osx-app-bundle.sh` script from that directory.

On homebrew you have to run the `travis-scripts/create-osx-app-bundle-homebrew.sh` from the directory in `/private/tmp` that homebrew uses to compile the FontForge source tree at. 
Note that the bundle-homebrew.sh script is used on Travis CI to build a bundle and has not been extensively tested outside that environment.

In both cases you will see a new file at `~/FontForge.app.zip` (or perhaps a dmg in the future) which is the output from running the `create-osx-app-bundle` script.

