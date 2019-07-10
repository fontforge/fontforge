# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
FindGDK3
---------

Find the GDK3 library. This currently depends on pkg-config to work.

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``GDK3::GDK3``
  The GDK3 ``GDK3`` library, if found

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``GDK3_FOUND``
  true if the GDK3 headers and libraries were found
``GDK3_INCLUDE_DIRS``
  directories containing the GDK3 headers.
``GDK3_LIBRARIES``
  the library to link against
``GDK3_VERSION``
  the version of GDK3 found

#]=======================================================================]

find_package(PkgConfig)
pkg_check_modules(GDK3 QUIET IMPORTED_TARGET gdk-3.0)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  GDK3
  REQUIRED_VARS
    GDK3_LIBRARIES
    GDK3_INCLUDE_DIRS
  VERSION_VAR
    GDK3_VERSION
)

include(${CMAKE_CURRENT_LIST_DIR}/../TargetUtils.cmake)
alias_imported_target(GDK3::GDK3 PkgConfig::GDK3)
