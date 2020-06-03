# Installing FontForge from source

Install all your typical build tools, build dependencies and runtime dependencies - all the packages that allow you to build of software from source code.
The exact method to do this depends on your OS distribution.

To download all dependencies on Ubuntu, run:

```sh
sudo apt-get install libjpeg-dev libtiff5-dev libpng-dev libfreetype6-dev libgif-dev libgtk-3-dev libxml2-dev libpango1.0-dev libcairo2-dev libspiro-dev libuninameslist-dev python3-dev ninja-build cmake build-essential gettext;
```

Now run the build and installation scripts:

```sh
cd fontforge
mkdir build
cd build
cmake -GNinja ..
ninja
ninja install
```

For even more details on the build system itself, see [this wiki page](https://github.com/fontforge/fontforge/wiki/CMake-guide-for-FontForge).

**Attention, Designers Who Love TrueType Hinting:** 
Run `cmake` with `-DENABLE_FREETYPE_DEBUGGER=/path/to/freetype/source`. 
This option enables advanced features for debugging TrueType font hints, such as stepping through hinting instructions one by one.

Note that the version of the FreeType source must *exactly* match the installed version of FreeType.

## Common Problems
### MacOS
* Missing packages can be installed with Homebrew, like:
```
brew install cmake glib pango
```
* `msgfmt` is not found (when using Homebrew):

   ```bash
   brew install gettext
   brew link gettext
   ```

#### Executables

If you want to do autotracing around character images you should also download either

- Peter Selinger's [potrace](http://potrace.sf.net/)
- Martin Weber's [autotrace program.](http://sourceforge.net/projects/autotrace/)

#### Libraries

If your system comes with a package manager, use it. 
It makes installing these libraries easier.

Most of these are not required for the proper compilation/execution of FontForge, if the libraries are not present they will not be used.
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
-   [Unifont](http://savannah.gnu.org/projects/unifont) includes glyphs for all Unicode codepoints, and FontForge will use it if it is installed.
