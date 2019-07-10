# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
FindLibspiro
------------

Find the libspiro library.

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``Libspiro::Libspiro``
  The Libspiro ``libspiro`` library, if found

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``Libspiro_FOUND``
  true if the libspiro headers and libraries were found
``Libspiro_INCLUDE_DIRS``
  directories containing the libspiro headers.
``Libspiro_LIBRARIES``
  the library to link against
``Libspiro_VERSION``
  the version of libspiro found
``Libspiro_FEATURE_LEVEL``
  the feature level of libspiro. This value is set to:
    * 2 if ``LibSpiroVersion`` is present
    * 1 if ``TaggedSpiroCPsToBezier0`` is present
    * 0 otherwise

#]=======================================================================]

find_package(PkgConfig)
pkg_check_modules(PC_LIBSPIRO QUIET libspiro)

# This is so bad... The version isn't encoded anywhere outside of this
if(PC_LIBSPIRO_FOUND)
  set(Libspiro_VERSION ${PC_LIBSPIRO_VERSION})
endif()

find_path(Libspiro_INCLUDE_DIRS
  NAMES spiroentrypoints.h
  HINTS ${PC_LIBSPIRO_INCLUDEDIR}
        ${PC_LIBSPIRO_INCLUDE_DIRS}
)

find_library(Libspiro_LIBRARIES
  NAMES spiro
  HINTS ${PC_LIBSPIRO_LIBDIR}
        ${PC_LIBSPIRO_LIBRARY_DIRS}
)

# Because we like doing things the hard way...
if(Libspiro_INCLUDE_DIRS)
  include(CheckSymbolExists)
  include(CMakePushCheckState)

  cmake_push_check_state(RESET)
  set(CMAKE_REQUIRED_INCLUDES ${Libspiro_INCLUDE_DIRS})
  set(CMAKE_REQUIRED_LIBRARIES ${Libspiro_LIBRARIES})

  check_symbol_exists(LibSpiroVersion "spiroentrypoints.h" _LibSpiroVersion_PRESENT)
  if(${_LibSpiroVersion_PRESENT})
    set(Libspiro_FEATURE_LEVEL 2)
  else()
    check_symbol_exists(TaggedSpiroCPsToBezier0 "spiroentrypoints.h" _TaggedSpiroCPsToBezier0_PRESENT)
    if(${_TaggedSpiroCPsToBezier0_PRESENT})
      set(Libspiro_FEATURE_LEVEL 1)
    else ()
      set(Libspiro_FEATURE_LEVEL 0)
    endif()
  endif()
  cmake_pop_check_state()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Libspiro
  REQUIRED_VARS
    Libspiro_LIBRARIES
    Libspiro_INCLUDE_DIRS
  VERSION_VAR
    Libspiro_VERSION
)

mark_as_advanced(
  Libspiro_LIBRARIES
  Libspiro_INCLUDE_DIRS
)

if(Libspiro_FOUND AND NOT TARGET Libspiro::Libspiro)
  add_library(Libspiro::Libspiro INTERFACE IMPORTED)
  set_property(TARGET Libspiro::Libspiro PROPERTY
               INTERFACE_INCLUDE_DIRECTORIES "${Libspiro_INCLUDE_DIRS}")
  set_property(TARGET Libspiro::Libspiro PROPERTY
               INTERFACE_LINK_LIBRARIES "${Libspiro_LIBRARIES}")
endif()
