# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
FindWOFF2
---------

Find the WOFF2 library.

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``WOFF2::WOFF2``
  The WOFF2 ``woff2`` library, if found

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``WOFF2_FOUND``
  true if the WOFF2 headers and libraries were found
``WOFF2_INCLUDE_DIRS``
  directories containing the WOFF2 headers.
``WOFF2_LIBRARIES``
  the library to link against
``WOFF2_VERSION``
  the version of WOFF2 found

#]=======================================================================]

find_package(PkgConfig)
pkg_check_modules(WOFF2 QUIET IMPORTED_TARGET libwoff2enc libwoff2dec)

if(WOFF2_FOUND)
  set(WOFF2_VERSION ${WOFF2_libwoff2enc_VERSION})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  WOFF2
  REQUIRED_VARS
    WOFF2_LIBRARIES
  VERSION_VAR
   WOFF2_VERSION
)

include(${CMAKE_CURRENT_LIST_DIR}/../TargetUtils.cmake)
alias_imported_target(WOFF2::WOFF2 PkgConfig::WOFF2)
