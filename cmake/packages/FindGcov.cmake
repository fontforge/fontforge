# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
FindGcov
--------

Checks for gcov/code coverage support.

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``Gcov::Gcov``
  An imported target that provides the required compiler flags
  to generate code coverage data.

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``Gcov_FOUND``
  true if gcov appears to be functional

#]=======================================================================]

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  message(WARNING "Set CMAKE_BUILD_TYPE to Debug for accurate code coverage results")
endif()

if(NOT DEFINED Gcov_FOUND)
  include(CheckCSourceCompiles)
  include(CMakePushCheckState)
  cmake_push_check_state(RESET)

  set(Gcov_COMPILE_DEFINITIONS "-g;-O0;-fprofile-arcs;-ftest-coverage" CACHE STRING "Gcov compiler flags")
  set(Gcov_LINK_OPTIONS "-lgcov" CACHE STRING "Gcov link flags")
  set(CMAKE_REQUIRED_FLAGS ${Gcov_COMPILE_DEFINITIONS})
  set(CMAKE_REQUIRED_LINK_OPTIONS "${Gcov_LINK_OPTIONS}")
  check_c_source_compiles("int main() {return 0;}" Gcov_FOUND)
  cmake_pop_check_state()
endif()

find_package_handle_standard_args(
  Gcov
  REQUIRED_VARS
    Gcov_FOUND
    Gcov_COMPILE_DEFINITIONS
    Gcov_LINK_OPTIONS
)

if(Gcov_FOUND AND NOT TARGET Gcov::Gcov)
  add_library(Gcov::Gcov INTERFACE IMPORTED)
  set_property(TARGET Gcov::Gcov PROPERTY INTERFACE_COMPILE_OPTIONS "${Gcov_COMPILE_DEFINITIONS}")
  if(${CMAKE_VERSION} VERSION_LESS "3.13")
    set_property(TARGET Gcov::Gcov PROPERTY INTERFACE_LINK_LIBRARIES "${Gcov_LINK_OPTIONS}")
  else()
    set_property(TARGET Gcov::Gcov PROPERTY INTERFACE_LINK_OPTIONS "${Gcov_LINK_OPTIONS}")
  endif()
endif()
