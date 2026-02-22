Python Extension Technical Details
==================================

This document covers the technical details of FontForge's Python extension
module, including building wheels for PyPI distribution.

Modules
-------

FontForge provides two Python modules:

- ``fontforge`` - Main font manipulation API
- ``psMat`` - PostScript matrix operations

Build Scenarios
---------------

.. list-table::
   :header-rows: 1

   * - Scenario
     - Detection
     - GUI
     - Install Location
   * - Wheel (pip)
     - ``SKBUILD`` defined
     - OFF
     - Wheel root
   * - System install
     - Manual cmake
     - ON/OFF
     - site-packages

Dependencies
------------

Required (always linked):
   libxml2, zlib, libiconv

Bundled in wheels:
   FreeType, libpng, libjpeg, libtiff, libspiro, woff2

Not needed for wheels (GUI-only):
   GLIB, Gettext/Intl, Readline

Versioning
----------

The source uses ``PROJECT_VERSION = 20251009`` (YYYYMMDD format).
Wheels use CalVer format ``2025.10.9``.

``fontforge.version()`` returns:

- CalVer when imported as a module (from site-packages or wheel)
- Numeric format when Python is embedded in the running FontForge application

CMake transformation::

   string(SUBSTRING "${PROJECT_VERSION}" 0 4 _FF_YEAR)
   string(SUBSTRING "${PROJECT_VERSION}" 4 2 _FF_MONTH)
   string(SUBSTRING "${PROJECT_VERSION}" 6 2 _FF_DAY)
   math(EXPR _FF_MONTH_INT "${_FF_MONTH}")
   math(EXPR _FF_DAY_INT "${_FF_DAY}")
   set(FONTFORGE_PYTHON_VERSION "${_FF_YEAR}.${_FF_MONTH_INT}.${_FF_DAY_INT}")

Windows MSVC Build
------------------

Windows wheels require MSVC because Python.org's Windows Python is built
with MSVC. The MinGW build continues for the GUI application.

MSVC compatibility patterns::

   // UNUSED macro
   #ifdef _MSC_VER
   # define UNUSED(x) x
   #elif defined(__GNUC__)
   # define UNUSED(x) UNUSED_ ## x __attribute__((unused))
   #endif

DLL exports: MinGW uses ``--export-all-symbols``. MSVC needs explicit
``__declspec(dllexport)`` or a DEF file.

vcpkg provides dependencies::

   {
     "name": "fontforge",
     "dependencies": ["freetype", "libxml2", "glib", "gettext", "libiconv", "zlib"]
   }

Build Configuration
-------------------

Wheels are built using scikit-build-core. See ``pyproject.toml`` for the
current configuration.

::

   [build-system]
   requires = ["scikit-build-core>=0.5"]
   build-backend = "scikit_build_core.build"

   [project]
   name = "fontforge"
   dynamic = ["version"]

   [tool.scikit-build]
   cmake.args = ["-DENABLE_GUI=OFF", "-DENABLE_PYTHON_EXTENSION=ON"]
   wheel.install-dir = "."

Wheel Target Platforms
----------------------

The project builds wheels for:

- Linux: x86_64, aarch64 (manylinux_2_28)
- macOS: x86_64, arm64 (separate wheels, macOS 11.0+)
- Windows: x64 (MSVC)

Install Conflict Handling
-------------------------

App installs create ``fontforge-{version}.dist-info/`` with
``INSTALLER = fontforge-app``. This allows ``pip list`` to show the install
and ``pip uninstall`` to work.

If a pip-installed version exists, app install skips and suggests
``pip uninstall fontforge``.
