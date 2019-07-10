# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
FindMathLib
-----------

Determines if linking to 'm' is required for math functionality.

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``MathLib::MathLib``

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``MathLib_IS_BUILT_IN``
  set if the math library is built-in to the C standard library
``MathLib_LIBRARIES``
  the math library to link against, if any

#]=======================================================================]


include(CheckFunctionExists)
include(CMakePushCheckState)

if(NOT DEFINED MathLib_IS_BUILT_IN)
  if(NOT DEFINED MathLib_LIBRARIES)
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_QUIET TRUE)
    check_function_exists(pow MathLib_IS_BUILT_IN)
    cmake_pop_check_state()
  else()
    set(MathLib_IS_BUILT_IN FALSE)
  endif()
endif()

if(NOT MathLib_IS_BUILT_IN)
  find_library(MathLib_LIBRARIES
    NAMES m
    DOC "math library")
  set(MathLib_OK ${MathLib_LIBRARIES})
else()
  set(MathLib_OK "built-in")
endif()

mark_as_advanced(MathLib_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MathLib REQUIRED_VARS MathLib_OK)

if(MathLib_FOUND AND NOT TARGET MathLib::MathLib)
  add_library(MathLib::MathLib INTERFACE IMPORTED)
  set_property(TARGET MathLib::MathLib PROPERTY INTERFACE_LINK_LIBRARIES "${MathLib_LIBRARIES}")
endif()
