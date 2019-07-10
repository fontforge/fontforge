# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
FindPango
---------

Find the Pango library. This currently depends on pkg-config to work.

If additional ``COMPONENTS`` are passed in, these are passed on to
``pkg_check_modules()``. Typical usages for this are to include additional
dependencies on ``pangocairo``, ``pangoft2``, ``pangoxft`` and ``pangowin32``.

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``Pango::Pango``
  The Pango ``pango`` library, if found

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``Pango_FOUND``
  true if the Pango headers and libraries were found
``Pango_INCLUDE_DIRS``
  directories containing the Pango headers.
``Pango_LIBRARIES``
  the library to link against
``Pango_VERSION``
  the version of Pango found

#]=======================================================================]

find_package(PkgConfig)
pkg_check_modules(Pango QUIET IMPORTED_TARGET pango ${Pango_FIND_COMPONENTS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Pango
  REQUIRED_VARS
    Pango_LIBRARIES
    Pango_INCLUDE_DIRS
  VERSION_VAR
    Pango_pango_VERSION
)

include(${CMAKE_CURRENT_LIST_DIR}/../TargetUtils.cmake)
alias_imported_target(Pango::Pango PkgConfig::Pango)