# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
BuiltUtils
----------

``build_option`` standardises the way in which to specify a build option.
Supports any type that the ``set()`` function supports. Options are
stored in the cache.

For ``BOOL`` options, passing in a 5th parameter (after the description)
will lead to that parameter being evaluated. If it evaluates to false,
the build option is forced to ``OFF``.

The ``AUTO`` type is for tri-state Boolean options. The ``value`` parameter
is ignored; it will always be initialised to ``AUTO``.

The ``ENUM`` type is for options that may be one of a number of defined
values. All arguments afer the description are treated as allowed values
of the enumeration.

All other types are passed directly to ``set()``.

``print_build_options`` lists all build options, along with their
current status.

``set_default_build_type sets the default build type if none is
explicitly specified.

``set_default_rpath`` sets the default RPATH to be used on platforms
that support it. Can only be called after GNUInstallDirs has been
included.

``enable_sanitizer`` checks if the specified sanitizer is supported,
and adds the required compiler flags to enable it if so. If unsupported,
it will error out.

#]=======================================================================]

function(build_option variable type value description)
  if(${type} STREQUAL BOOL)
    if(${ARGC} EQUAL 4)
      option("${variable}" "${description}" "${value}")
    elseif(${ARGC} EQUAL 5)
      if(${ARGV4})
        option("${variable}" "${description}" "${value}")
      else()
        set("${variable}" OFF CACHE BOOL "${description}" FORCE)
      endif()
    else()
        message(FATAL_ERROR "Invalid number of arguments for dependent option")
    endif()
  elseif(${type} STREQUAL AUTO)
    set("${variable}" AUTO CACHE STRING "${description}")
    set_property(CACHE "${variable}" PROPERTY STRINGS AUTO ON OFF)
  elseif(${type} STREQUAL ENUM)
    if(${ARGC} LESS 5)
      message(FATAL_ERROR "Must pass at least one enum type")
    endif()
    set("${variable}" "${value}" CACHE STRING "${description} Valid values: ${ARGN}")
    set_property(CACHE "${variable}" PROPERTY STRINGS ${ARGN})
    if(NOT "${${variable}}" IN_LIST ARGN)
      message(FATAL_ERROR "'${${variable}}' is not a valid value for ${variable}, expect one of ${ARGN}")
    endif()
  else()
    set("${variable}" "${value}" CACHE "${type}" "${description}")
  endif()

  if(NOT "${variable}" IN_LIST BUILD_OPTIONS)
    set(BUILD_OPTIONS "${BUILD_OPTIONS};${variable}" CACHE INTERNAL "List of build options")
  endif()
endfunction()

function(print_build_options)
  message(STATUS "Build options: ")
  foreach(opt ${BUILD_OPTIONS})
    if("${${opt}}" STREQUAL "AUTO" AND DEFINED ${opt}_RESULT)
      if(${opt}_RESULT)
        message(STATUS "  ${opt} = ${${opt}} => ON")
      else()
        message(STATUS "  ${opt} = ${${opt}} => OFF")
      endif()
    else()
      message(STATUS "  ${opt} = ${${opt}}")
    endif()
  endforeach()
endfunction()

function(set_default_build_type default_build_type)
  # https://blog.kitware.com/cmake-and-the-default-build-type
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
      "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
  endif()
  if(NOT "CMAKE_BUILD_TYPE" IN_LIST BUILD_OPTIONS)
    set(BUILD_OPTIONS "${BUILD_OPTIONS};CMAKE_BUILD_TYPE" CACHE INTERNAL "List of build options")
  endif()
endfunction()

function(set_default_rpath)
  list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_FULL_LIBDIR}" _present)
  if(_present LESS 0)
    list(FIND CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}" _present)
    if(_present LESS 0)
      list(APPEND CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}")
    endif()
  endif()
  if(APPLE)
    list(APPEND CMAKE_INSTALL_RPATH "@loader_path/../lib")
  endif()
  set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_RPATH} PARENT_SCOPE)
endfunction()

function(enable_sanitizer type)
  if("${type}" STREQUAL "none")
    return()
  elseif(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "Require a GCC-like compiler to enable sanitizers.")
  endif()

  include(CheckCCompilerFlag)
  include(CMakePushCheckState)

  cmake_push_check_state(RESET)
  set(CMAKE_REQUIRED_LIBRARIES "-fsanitize=${type}")
  check_c_compiler_flag("-fsanitize=${type} -fno-omit-frame-pointer" "_sanitizer_${type}")
  cmake_pop_check_state()

  if(NOT _sanitizer_${type})
    message(FATAL_ERROR "Sanitizer flags not supported: -fsanitize=${type} -fno-omit-frame-pointer")
  endif()

  # kind of dirty but what can you do
  add_compile_options("-fsanitize=${type}" "-fno-omit-frame-pointer")
  link_libraries("-fsanitize=${type}")
endfunction()
