# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
FindLibunibreak
---------------

Find the libunibreak library.

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``Libunibreak::Libunibreak``
  The libunibreak library, if found

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``Libunibreak_FOUND``
  true if the libunibreak headers and libraries were found
``Libunibreak_INCLUDE_DIRS``
  directories containing the libunibreak headers.
``Libunibreak_LIBRARIES``
  the library to link against
``Libunibreak_VERSION``
  the version of libunibreak found

#]=======================================================================]

find_package(PkgConfig)
pkg_check_modules(PC_LIBUNIBREAK QUIET libunibreak)

if(PC_LIBUNIBREAK_FOUND)
  set(Libunibreak_VERSION ${PC_LIBUNIBREAK_VERSION})
endif()

find_path(Libunibreak_INCLUDE_DIRS
  NAMES linebreak.h
  HINTS ${PC_LIBUNIBREAK_INCLUDEDIR}
        ${PC_LIBUNIBREAK_INCLUDE_DIRS}
  PATH_SUFFIXES libunibreak
)

find_library(Libunibreak_LIBRARIES
  NAMES unibreak
  HINTS ${PC_LIBUNIBREAK_LIBDIR}
        ${PC_LIBUNIBREAK_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Libunibreak
  REQUIRED_VARS
    Libunibreak_LIBRARIES
    Libunibreak_INCLUDE_DIRS
  VERSION_VAR
    Libunibreak_VERSION
)

mark_as_advanced(
  Libunibreak_LIBRARIES
  Libunibreak_INCLUDE_DIRS
)

if(Libunibreak_FOUND AND NOT TARGET Libunibreak::Libunibreak)
  add_library(Libunibreak::Libunibreak INTERFACE IMPORTED)
  set_property(TARGET Libunibreak::Libunibreak PROPERTY
               INTERFACE_INCLUDE_DIRECTORIES "${Libunibreak_INCLUDE_DIRS}")
  set_property(TARGET Libunibreak::Libunibreak PROPERTY
               INTERFACE_LINK_LIBRARIES "${Libunibreak_LIBRARIES}")
endif()
