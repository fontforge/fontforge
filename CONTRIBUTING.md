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
* Build System: OS X (Applicatoin bundle, Homebrew) - Dr Ben Martin (monkeyiq)
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
fontforge/fontforge repository repository, Travis CI will allow secure
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

### Building a Debian source package

A Debian source package consists of a source tarball (with specific
metadata) and several accompanying files and allows one to build a
product in a neutral build environment.

A source package is specific to the distribution (but not the
architecture) that it targets. The most common build target is
currely Ubuntu Precise, a long-term support release with Launchpad
build support. A binary package built on and for Precise also
installs and functions correctly on Debian testing and on Ubuntu
Trusty.

After downloading the FontForge source, change into the source
directory and run
```
./bootstrap; ./configure; make F_METADATA_REGENERATE=1 \
UBUNTU_RELEASE=precise deb-src;
```
.

One can omit the `UBUNTU_RELEASE` argument and use an arbitrary
`DEB_OS_RELEASE` argument instead in order to target a Debian
distribution. One can use a custom debian directory by populating
it and omitting the `F_METADATA_REGENERATE` option.

Upon successful completion, building of the source package will
leave several files in the parent directory, with names like this:
```
fontforge_20140813-92-g203bca8-0ubuntu1~precise.dsc
fontforge_20140813-92-g203bca8-0ubuntu1~precise_source.build
fontforge_20140813-92-g203bca8-0ubuntu1~precise_source.changes
fontforge_20140813-92-g203bca8-0ubuntu1~precise.tar.gz
```
.

In order to upload to a Launchpad repository for building, one can
then run dput on the `.changes` file with the target repository as
the first argument, like this:
```
dput ppa:fontforge/fontforge fontforge_20140813-92-g203bca8-0ubuntu1~precise_source.changes
```
. This will, upon success, leave a file named something like
`fontforge_20140813-92-g203bca8-0ubuntu1~precise_source.ppa.upload`,
which blocks duplicate uploads.

Upon validation of the uploaded package, Launchpad will build the
package for all supported architectures. One can then copy the
binary packages from precise to other Ubuntu versions via the
Launchpad web interface.

See [here] (https://help.launchpad.net/Packaging/PPA) for more
information about Launchpad.

One can also build a binary package from the source package locally.
Simply extract the `tar.gz` file generated previously into a new
directory, enter the directory, and run `debuild`.

### Building a Red Hat source package

One can build a Red Hat source package by entering a clean FontForge
source tree and running
```
./bootstrap; ./configure; make F_METADATA_REGENERATE=1 rpm-src;
```
.

This will leave a `tar.gz` file and a `.spec` file in the parent
directory.

In order to build the binary package locally, copy the source file to
`~/rpmbuild/SOURCES` and the spec file to `~/rpmbuild/SPECS`, and run
`rpmbuild -ba ~/rpmbuild/SPECS/(name of spec file)`. Upon success,
this will leave binary packages in `~/rpmbuild/RPMS` and source packages
in `~/rpmbuild/SRPMS`.

### Building an OSX app bundle

There are two scripts to build an app bundle, which one you use will
depend on if you are using homebrew or macports on your OSX machine.
Both scripts aim to create a complete stand alone bundle of FontForge
and all the libraries, data files, and dependencies that it needs,
including Python.

The osx/create-osx-app-bundle.sh works on a macports machine whereas
the travis-scripts/create-osx-app-bundle-homebrew.sh script is for
homebrew. It is possible that these scripts will be merged into a
single script that can work on both platforms in the future.

For macports there is an osx/Portfile and for homebrew you might like
to see travis-scripts/fontforge.rb for changes. Both of these files
are geared to build current git master.

The osx/create-osx-app-bundle.sh script has been used since early 2013
to create the daily bundle for FontForge. Both the bundle scripts
expect to be run from the root directory of where FontForge was
compiled. For example, doing the below command to install fontforge on OSX:

    sudo port -v -k install fontforge

and then move to
the/opt/local/var/macports/build/.../work/fontforge-2.0.0_beta1 and
run the ./osx/create-osx-app-bundle.sh script from that directory.

On homebrew you have to run the
travis-scripts/create-osx-app-bundle-homebrew.sh from the directory in
/private/tmp that homebrew uses to compile the FontForge source tree
at. Note that the bundle-homebrew.sh script is used on Travis CI to
build a bundle and has not been extensively tested outside that
environment.

In both cases you will see a new file at ~/FontForge.app.zip (or
perhaps a dmg in the future) which is the output from running the
create-osx-app-bundle script.
