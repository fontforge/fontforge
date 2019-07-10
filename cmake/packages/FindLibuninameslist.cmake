# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
FindLibuninameslist
-------------------

Find the libuninameslist library.

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``Libuninameslist::Libuninameslist``
  The Libuninameslist ``libuninameslist`` library, if found

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``Libuninameslist_FOUND``
  true if the libuninameslist headers and libraries were found
``Libuninameslist_INCLUDE_DIRS``
  directories containing the libuninameslist headers.
``Libuninameslist_LIBRARIES``
  the library to link against
``Libuninameslist_VERSION``
  the version of libuninameslist found
``Libuninameslist_FEATURE_LEVEL``
  the feature level of libuninameslist. This value is set to:
    * 5 if ``uniNamesList_names2anU`` is present
    * 4 if ``uniNamesList_blockCount`` is present
    * 3 if ``uniNamesList_NamesListVersion`` is present
    * 0 otherwise

#]=======================================================================]

find_package(PkgConfig)
pkg_check_modules(PC_LIBUNINAMESLIST QUIET libuninameslist)

# This is so bad... The version isn't encoded anywhere outside of this
if(PC_LIBUNINAMESLIST_FOUND)
  set(Libuninameslist_VERSION ${PC_LIBUNINAMESLIST_VERSION})
endif()

find_path(Libuninameslist_INCLUDE_DIRS
  NAMES uninameslist.h
  HINTS ${PC_LIBUNINAMESLIST_INCLUDEDIR}
        ${PC_LIBUNINAMESLIST_INCLUDE_DIRS}
)

find_library(Libuninameslist_LIBRARIES
  NAMES uninameslist
  HINTS ${PC_LIBUNINAMESLIST_LIBDIR}
        ${PC_LIBUNINAMESLIST_LIBRARY_DIRS}
)

# Because we like doing things the hard way...
if(Libuninameslist_INCLUDE_DIRS)
  include(CheckSymbolExists)
  include(CMakePushCheckState)

  cmake_push_check_state(RESET)
  set(CMAKE_REQUIRED_INCLUDES ${Libuninameslist_INCLUDE_DIRS})
  set(CMAKE_REQUIRED_LIBRARIES ${Libuninameslist_LIBRARIES})

  check_symbol_exists(uniNamesList_names2anU "uninameslist.h" _names2anU_PRESENT)
  if(${_names2anU_PRESENT})
    set(Libuninameslist_FEATURE_LEVEL 5)
  else ()
    check_symbol_exists(uniNamesList_blockCount "uninameslist.h" _blockCount_PRESENT)
    if(${_blockCount_PRESENT})
      set(Libuninameslist_FEATURE_LEVEL 4)
    else ()
      check_symbol_exists(uniNamesList_NamesListVersion "uninameslist.h" _NamesListVersion_PRESENT)
      if(${_NamesListVersion_PRESENT})
        set(Libuninameslist_FEATURE_LEVEL 3)
      else ()
        set(Libuninameslist_FEATURE_LEVEL 0)
      endif()
    endif()
  endif()
  cmake_pop_check_state()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Libuninameslist
  REQUIRED_VARS
    Libuninameslist_LIBRARIES
    Libuninameslist_INCLUDE_DIRS
  VERSION_VAR
    Libuninameslist_VERSION
)

mark_as_advanced(
  Libuninameslist_LIBRARIES
  Libuninameslist_INCLUDE_DIRS
)

if(Libuninameslist_FOUND AND NOT TARGET Libuninameslist::Libuninameslist)
  add_library(Libuninameslist::Libuninameslist INTERFACE IMPORTED)
  set_property(TARGET Libuninameslist::Libuninameslist PROPERTY
               INTERFACE_INCLUDE_DIRECTORIES "${Libuninameslist_INCLUDE_DIRS}")
  set_property(TARGET Libuninameslist::Libuninameslist PROPERTY
               INTERFACE_LINK_LIBRARIES "${Libuninameslist_LIBRARIES}")
endif()
