# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#
# This file is a "template" file used by various FindPython modules.
#

if (NOT ${CMAKE_VERSION} VERSION_LESS "3.14")
  cmake_policy (GET CMP0094 _${_PYTHON_PREFIX}_LOOKUP_POLICY)
endif()

cmake_policy (VERSION 3.5)

if (_${_PYTHON_PREFIX}_LOOKUP_POLICY)
  cmake_policy (SET CMP0094 ${_${_PYTHON_PREFIX}_LOOKUP_POLICY})
endif()

#
# Initial configuration
#
if (NOT DEFINED _PYTHON_PREFIX)
  message (FATAL_ERROR "FindPython: INTERNAL ERROR")
endif()
if (NOT DEFINED _${_PYTHON_PREFIX}_REQUIRED_VERSION_MAJOR)
  message (FATAL_ERROR "FindPython: INTERNAL ERROR")
endif()
if (_${_PYTHON_PREFIX}_REQUIRED_VERSION_MAJOR EQUAL 3)
  set(_${_PYTHON_PREFIX}_VERSIONS 3.8 3.7 3.6 3.5 3.4 3.3 3.2 3.1 3.0)
elseif (_${_PYTHON_PREFIX}_REQUIRED_VERSION_MAJOR EQUAL 2)
  set(_${_PYTHON_PREFIX}_VERSIONS 2.7 2.6 2.5 2.4 2.3 2.2 2.1 2.0)
else()
  message (FATAL_ERROR "FindPython: INTERNAL ERROR")
endif()

if (${CMAKE_VERSION} VERSION_LESS "3.14")
  set(_${_PYTHON_PREFIX}_CMAKE_ROLE PROJECT)
else()
  get_property(_${_PYTHON_PREFIX}_CMAKE_ROLE GLOBAL PROPERTY CMAKE_ROLE)
endif()


#
# helper commands
#
macro (_PYTHON_DISPLAY_FAILURE _PYTHON_MSG)
  if (${_PYTHON_PREFIX}_FIND_REQUIRED)
    message (FATAL_ERROR "${_PYTHON_MSG}")
  else()
    if (NOT ${_PYTHON_PREFIX}_FIND_QUIETLY)
      message(STATUS "${_PYTHON_MSG}")
    endif ()
  endif()

  set (${_PYTHON_PREFIX}_FOUND FALSE)
  string (TOUPPER "${_PYTHON_PREFIX}" _${_PYTHON_PREFIX}_UPPER_PREFIX)
  set (${_PYTHON_UPPER_PREFIX}_FOUND FALSE)
  return()
endmacro()


macro (_PYTHON_FIND_FRAMEWORKS)
  set (${_PYTHON_PREFIX}_FRAMEWORKS)
  if (CMAKE_HOST_APPLE OR APPLE)
    set (_pff_frameworks ${CMAKE_FRAMEWORK_PATH}
                    $ENV{CMAKE_FRAMEWORK_PATH}
                    ~/Library/Frameworks
                    /usr/local/Frameworks
                    ${CMAKE_SYSTEM_FRAMEWORK_PATH})
    list (REMOVE_DUPLICATES _pff_frameworks)
    foreach (_pff_framework IN LISTS _pff_frameworks)
      if (EXISTS ${_pff_framework}/Python.framework)
        list (APPEND ${_PYTHON_PREFIX}_FRAMEWORKS ${_pff_framework}/Python.framework)
      endif()
    endforeach()
    unset (_pff_frameworks)
    unset (_pff_framework)
  endif()
endmacro()

function (_PYTHON_GET_FRAMEWORKS _PYTHON_PGF_FRAMEWORK_PATHS _PYTHON_VERSION)
  set (_PYTHON_FRAMEWORK_PATHS)
  foreach (_PYTHON_FRAMEWORK IN LISTS ${_PYTHON_PREFIX}_FRAMEWORKS)
    list (APPEND _PYTHON_FRAMEWORK_PATHS
          "${_PYTHON_FRAMEWORK}/Versions/${_PYTHON_VERSION}")
  endforeach()
  set (${_PYTHON_PGF_FRAMEWORK_PATHS} ${_PYTHON_FRAMEWORK_PATHS} PARENT_SCOPE)
endfunction()

function (_PYTHON_GET_REGISTRIES _PYTHON_PGR_REGISTRY_PATHS _PYTHON_VERSION)
  string (REPLACE "." "" _PYTHON_VERSION_NO_DOTS ${_PYTHON_VERSION})
  set (${_PYTHON_PGR_REGISTRY_PATHS}
       [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\${_PYTHON_VERSION}-${_${_PYTHON_PREFIX}_ARCH}\\InstallPath]
       [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\${_PYTHON_VERSION}-${_${_PYTHON_PREFIX}_ARCH2}\\InstallPath]
       [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\${_PYTHON_VERSION}\\InstallPath]
       [HKEY_CURRENT_USER\\SOFTWARE\\Python\\ContinuumAnalytics\\Anaconda${_PYTHON_VERSION_NO_DOTS}-${_${_PYTHON_PREFIX}_ARCH}\\InstallPath]
       [HKEY_CURRENT_USER\\SOFTWARE\\Python\\ContinuumAnalytics\\Anaconda${_PYTHON_VERSION_NO_DOTS}-${_${_PYTHON_PREFIX}_ARCH2}\\InstallPath]
       [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_PYTHON_VERSION}-${_${_PYTHON_PREFIX}_ARCH}\\InstallPath]
       [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_PYTHON_VERSION}-${_${_PYTHON_PREFIX}_ARCH2}\\InstallPath]
       [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_PYTHON_VERSION}\\InstallPath]
       [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\ContinuumAnalytics\\Anaconda${_PYTHON_VERSION_NO_DOTS}-${_${_PYTHON_PREFIX}_ARCH}\\InstallPath]
       [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\ContinuumAnalytics\\Anaconda${_PYTHON_VERSION_NO_DOTS}-${_${_PYTHON_PREFIX}_ARCH2}\\InstallPath]
       PARENT_SCOPE)
endfunction()

function (_PYTHON_GET_PATH_SUFFIXES _PYTHON_PGPS_PATH_SUFFIXES _PYTHON_VERSION _PYTHON_TYPE)
  set (path_suffixes)

  if (_PYTHON_TYPE STREQUAL "LIBRARY")
    if (CMAKE_LIBRARY_ARCHITECTURE)
      list (APPEND path_suffixes lib/${CMAKE_LIBRARY_ARCHITECTURE})
    endif()
    list (APPEND path_suffixes lib libs)

    if (CMAKE_LIBRARY_ARCHITECTURE)
      list (APPEND path_suffixes lib/python${_PYTHON_VERSION}/config-${_PYTHON_VERSION}mu-${CMAKE_LIBRARY_ARCHITECTURE}
                                 lib/python${_PYTHON_VERSION}/config-${_PYTHON_VERSION}m-${CMAKE_LIBRARY_ARCHITECTURE}
                                 lib/python${_PYTHON_VERSION}/config-${CMAKE_MATCH_1}u-${CMAKE_LIBRARY_ARCHITECTURE}
                                 lib/python${_PYTHON_VERSION}/config-${CMAKE_MATCH_1}-${CMAKE_LIBRARY_ARCHITECTURE})
    endif()
    list (APPEND path_suffixes lib/python${_PYTHON_VERSION}/config-${_PYTHON_VERSION}mu
                               lib/python${_PYTHON_VERSION}/config-${_PYTHON_VERSION}m
                               lib/python${_PYTHON_VERSION}/config-${_PYTHON_VERSION}u
                               lib/python${_PYTHON_VERSION}/config-${_PYTHON_VERSION}
                               lib/python${_PYTHON_VERSION}/config)

  elseif (_PYTHON_TYPE STREQUAL "INCLUDE")
    list (APPEND path_suffixes include/python${_PYTHON_VERSION}mu
                               include/python${_PYTHON_VERSION}m
                               include/python${_PYTHON_VERSION}u
                               include/python${_PYTHON_VERSION}
                               include)
  endif()

  set (${_PYTHON_PGPS_PATH_SUFFIXES} ${path_suffixes} PARENT_SCOPE)
endfunction()

function (_PYTHON_GET_LIB_NAMES _PYTHON_PGLN_NAMES _PYTHON_VERSION)
  string (REPLACE "." "" _PYTHON_VERSION_NO_DOTS ${_PYTHON_VERSION})

  if (ARGC EQUAL 3 AND ARGV2 STREQUAL "DEBUG")
    set (${_PYTHON_PGLN_NAMES} python${_PYTHON_VERSION_NO_DOTS}_d PARENT_SCOPE)
  else()
    set (${_PYTHON_PGLN_NAMES} python${_PYTHON_VERSION_NO_DOTS}
                               python${_PYTHON_VERSION}mu
                               python${_PYTHON_VERSION}m
                               python${_PYTHON_VERSION}u
                               python${_PYTHON_VERSION}
         PARENT_SCOPE)
  endif()
endfunction()


function (_PYTHON_VALIDATE_INTERPRETER)
  if (NOT ${_PYTHON_PREFIX}_EXECUTABLE)
    return()
  endif()

  cmake_parse_arguments (_PVI "EXACT" "" "" ${ARGN})
  if (_PVI_UNPARSED_ARGUMENTS)
    set (expected_version ${_PVI_UNPARSED_ARGUMENTS})
  else()
    unset (expected_version)
  endif()

  get_filename_component (python_name "${${_PYTHON_PREFIX}_EXECUTABLE}" NAME)

  if (expected_version AND NOT python_name STREQUAL "python${expected_version}${CMAKE_EXECUTABLE_SUFFIX}")
    # executable found must have a specific version
    execute_process (COMMAND "${${_PYTHON_PREFIX}_EXECUTABLE}" -c
                             "import sys; sys.stdout.write('.'.join([str(x) for x in sys.version_info[:2]]))"
                     RESULT_VARIABLE result
                     OUTPUT_VARIABLE version
                     ERROR_QUIET
                     OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (result OR (_PVI_EXACT AND NOT version VERSION_EQUAL expected_version) OR (version VERSION_LESS expected_version))
      # interpreter not usable or has wrong major version
      set (${_PYTHON_PREFIX}_EXECUTABLE ${_PYTHON_PREFIX}_EXECUTABLE-NOTFOUND CACHE INTERNAL "" FORCE)
      return()
    endif()
  else()
    if (NOT python_name STREQUAL "python${_${_PYTHON_PREFIX}_REQUIRED_VERSION_MAJOR}${CMAKE_EXECUTABLE_SUFFIX}")
      # executable found do not have version in name
      # ensure major version is OK
      execute_process (COMMAND "${${_PYTHON_PREFIX}_EXECUTABLE}" -c
                               "import sys; sys.stdout.write(str(sys.version_info[0]))"
                       RESULT_VARIABLE result
                       OUTPUT_VARIABLE version
                       ERROR_QUIET
                       OUTPUT_STRIP_TRAILING_WHITESPACE)
      if (result OR NOT version EQUAL _${_PYTHON_PREFIX}_REQUIRED_VERSION_MAJOR)
        # interpreter not usable or has wrong major version
        set (${_PYTHON_PREFIX}_EXECUTABLE ${_PYTHON_PREFIX}_EXECUTABLE-NOTFOUND CACHE INTERNAL "" FORCE)
        return()
      endif()
    endif()
  endif()

  if (CMAKE_SIZEOF_VOID_P AND "Development" IN_LIST ${_PYTHON_PREFIX}_FIND_COMPONENTS
      AND NOT CMAKE_CROSSCOMPILING)
    # In this case, interpreter must have same architecture as environment
    execute_process (COMMAND "${${_PYTHON_PREFIX}_EXECUTABLE}" -c
                             "import sys, struct; sys.stdout.write(str(struct.calcsize(\"P\")))"
                     RESULT_VARIABLE result
                     OUTPUT_VARIABLE size
                     ERROR_QUIET
                     OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (result OR NOT size EQUAL CMAKE_SIZEOF_VOID_P)
      # interpreter not usable or has wrong architecture
      set (${_PYTHON_PREFIX}_EXECUTABLE ${_PYTHON_PREFIX}_EXECUTABLE-NOTFOUND CACHE INTERNAL "" FORCE)
      return()
    endif()
  endif()
endfunction()


function (_PYTHON_VALIDATE_COMPILER expected_version)
  if (NOT ${_PYTHON_PREFIX}_COMPILER)
    return()
  endif()

  cmake_parse_arguments (_PVC "EXACT" "" "" ${ARGN})
  if (_PVC_UNPARSED_ARGUMENTS)
    set (major_version FALSE)
    set (expected_version ${_PVC_UNPARSED_ARGUMENTS})
  else()
    set (major_version TRUE)
    set (expected_version ${_${_PYTHON_PREFIX}_REQUIRED_VERSION_MAJOR})
    set (_PVC_EXACT TRUE)
  endif()

  # retrieve python environment version from compiler
  set (working_dir "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/PythonCompilerVersion.dir")
  if (major_version)
    # check only major version
    file (WRITE "${working_dir}/version.py" "import sys; sys.stdout.write(str(sys.version_info[0]))")
  else()
    file (WRITE "${working_dir}/version.py" "import sys; sys.stdout.write('.'.join([str(x) for x in sys.version_info[:2]]))\n")
  endif()
  execute_process (COMMAND "${${_PYTHON_PREFIX}_COMPILER}" /target:exe /embed "${working_dir}/version.py"
                   WORKING_DIRECTORY "${working_dir}"
                   OUTPUT_QUIET
                   ERROR_QUIET
                   OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process (COMMAND "${working_dir}/version"
                   WORKING_DIRECTORY "${working_dir}"
                   RESULT_VARIABLE result
                   OUTPUT_VARIABLE version
                   ERROR_QUIET)
  file (REMOVE_RECURSE "${_${_PYTHON_PREFIX}_VERSION_DIR}")

  if (result OR (_PVC_EXACT AND NOT version VERSION_EQUAL expected_version) OR (version VERSION_LESS expected_version))
    # Compiler not usable or has wrong version
    set (${_PYTHON_PREFIX}_COMPILER ${_PYTHON_PREFIX}_COMPILER-NOTFOUND CACHE INTERNAL "" FORCE)
  endif()
endfunction()


function (_PYTHON_FIND_RUNTIME_LIBRARY _PYTHON_LIB)
  string (REPLACE "_RUNTIME" "" _PYTHON_LIB "${_PYTHON_LIB}")
  # look at runtime part on systems supporting it
  if (CMAKE_SYSTEM_NAME STREQUAL "Windows" OR
      (CMAKE_SYSTEM_NAME MATCHES "MSYS|CYGWIN"
        AND ${_PYTHON_LIB} MATCHES "${CMAKE_IMPORT_LIBRARY_SUFFIX}$"))
    set (CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_SHARED_LIBRARY_SUFFIX})
    # MSYS has a special syntax for runtime libraries
    if (CMAKE_SYSTEM_NAME MATCHES "MSYS")
      list (APPEND CMAKE_FIND_LIBRARY_PREFIXES "msys-")
    endif()
    find_library (${ARGV})
  endif()
endfunction()


function (_PYTHON_SET_LIBRARY_DIRS _PYTHON_SLD_RESULT)
  unset (_PYTHON_DIRS)
  set (_PYTHON_LIBS ${ARGV})
  list (REMOVE_AT _PYTHON_LIBS 0)
  foreach (_PYTHON_LIB IN LISTS _PYTHON_LIBS)
    if (${_PYTHON_LIB})
      get_filename_component (_PYTHON_DIR "${${_PYTHON_LIB}}" DIRECTORY)
      list (APPEND _PYTHON_DIRS "${_PYTHON_DIR}")
    endif()
  endforeach()
  if (_PYTHON_DIRS)
    list (REMOVE_DUPLICATES _PYTHON_DIRS)
  endif()
  set (${_PYTHON_SLD_RESULT} ${_PYTHON_DIRS} PARENT_SCOPE)
endfunction()


# If major version is specified, it must be the same as internal major version
if (DEFINED ${_PYTHON_PREFIX}_FIND_VERSION_MAJOR
    AND NOT ${_PYTHON_PREFIX}_FIND_VERSION_MAJOR VERSION_EQUAL _${_PYTHON_PREFIX}_REQUIRED_VERSION_MAJOR)
  _python_display_failure ("Could NOT find ${_PYTHON_PREFIX}: Wrong major version specified is \"${${_PYTHON_PREFIX}_FIND_VERSION_MAJOR}\", but expected major version is \"${_${_PYTHON_PREFIX}_REQUIRED_VERSION_MAJOR}\"")
endif()


# handle components
if (NOT ${_PYTHON_PREFIX}_FIND_COMPONENTS)
  set (${_PYTHON_PREFIX}_FIND_COMPONENTS Interpreter)
  set (${_PYTHON_PREFIX}_FIND_REQUIRED_Interpreter TRUE)
endif()
if ("NumPy" IN_LIST ${_PYTHON_PREFIX}_FIND_COMPONENTS)
  list (APPEND ${_PYTHON_PREFIX}_FIND_COMPONENTS "Interpreter" "Development")
  list (REMOVE_DUPLICATES ${_PYTHON_PREFIX}_FIND_COMPONENTS)
endif()
foreach (_${_PYTHON_PREFIX}_COMPONENT IN LISTS ${_PYTHON_PREFIX}_FIND_COMPONENTS)
  set (${_PYTHON_PREFIX}_${_${_PYTHON_PREFIX}_COMPONENT}_FOUND FALSE)
endforeach()
unset (_${_PYTHON_PREFIX}_FIND_VERSIONS)

# Set versions to search
## default: search any version
set (_${_PYTHON_PREFIX}_FIND_VERSIONS ${_${_PYTHON_PREFIX}_VERSIONS})

if (${_PYTHON_PREFIX}_FIND_VERSION_COUNT GREATER 1)
  if (${_PYTHON_PREFIX}_FIND_VERSION_EXACT)
    set (_${_PYTHON_PREFIX}_FIND_VERSIONS ${${_PYTHON_PREFIX}_FIND_VERSION_MAJOR}.${${_PYTHON_PREFIX}_FIND_VERSION_MINOR})
  else()
    unset (_${_PYTHON_PREFIX}_FIND_VERSIONS)
    # add all compatible versions
    foreach (_${_PYTHON_PREFIX}_VERSION IN LISTS _${_PYTHON_PREFIX}_VERSIONS)
      if ((_${_PYTHON_PREFIX}_VERSION VERSION_GREATER ${_PYTHON_PREFIX}_FIND_VERSION) OR
          (_${_PYTHON_PREFIX}_VERSION VERSION_EQUAL ${_PYTHON_PREFIX}_FIND_VERSION))
        list (APPEND _${_PYTHON_PREFIX}_FIND_VERSIONS ${_${_PYTHON_PREFIX}_VERSION})
      endif()
    endforeach()
  endif()
endif()

# Define lookup strategy
if (_${_PYTHON_PREFIX}_LOOKUP_POLICY STREQUAL "NEW")
  set (_${_PYTHON_PREFIX}_FIND_STRATEGY "LOCATION")
else()
  set (_${_PYTHON_PREFIX}_FIND_STRATEGY "VERSION")
endif()
if (DEFINED ${_PYTHON_PREFIX}_FIND_STRATEGY)
  if (NOT ${_PYTHON_PREFIX}_FIND_STRATEGY MATCHES "^(VERSION|LOCATION)$")
    message (AUTHOR_WARNING "Find${_PYTHON_PREFIX}: ${${_PYTHON_PREFIX}_FIND_STRATEGY}: invalid value for '${_PYTHON_PREFIX}_FIND_STRATEGY'. 'VERSION' or 'LOCATION' expected.")
    set (_${_PYTHON_PREFIX}_FIND_STRATEGY "VERSION")
  else()
    set (_${_PYTHON_PREFIX}_FIND_STRATEGY "${${_PYTHON_PREFIX}_FIND_STRATEGY}")
  endif()
endif()

# Python and Anaconda distributions: define which architectures can be used
if (CMAKE_SIZEOF_VOID_P)
  # In this case, search only for 64bit or 32bit
  math (EXPR _${_PYTHON_PREFIX}_ARCH "${CMAKE_SIZEOF_VOID_P} * 8")
  set (_${_PYTHON_PREFIX}_ARCH2 ${_${_PYTHON_PREFIX}_ARCH})
else()
  # architecture unknown, search for both 64bit and 32bit
  set (_${_PYTHON_PREFIX}_ARCH 64)
  set (_${_PYTHON_PREFIX}_ARCH2 32)
endif()

# IronPython support
if (CMAKE_SIZEOF_VOID_P)
  # In this case, search only for 64bit or 32bit
  math (EXPR _${_PYTHON_PREFIX}_ARCH "${CMAKE_SIZEOF_VOID_P} * 8")
  set (_${_PYTHON_PREFIX}_IRON_PYTHON_NAMES ipy${_${_PYTHON_PREFIX}_ARCH} ipy)
else()
  # architecture unknown, search for natural interpreter
  set (_${_PYTHON_PREFIX}_IRON_PYTHON_NAMES ipy)
endif()
set (_${_PYTHON_PREFIX}_IRON_PYTHON_PATH_SUFFIXES net45 net40)

# Apple frameworks handling
_python_find_frameworks ()

set (_${_PYTHON_PREFIX}_FIND_FRAMEWORK "FIRST")

if (DEFINED ${_PYTHON_PREFIX}_FIND_FRAMEWORK)
  if (NOT ${_PYTHON_PREFIX}_FIND_FRAMEWORK MATCHES "^(FIRST|LAST|NEVER)$")
    message (AUTHOR_WARNING "Find${_PYTHON_PREFIX}: ${${_PYTHON_PREFIX}_FIND_FRAMEWORK}: invalid value for '${_PYTHON_PREFIX}_FIND_FRAMEWORK'. 'FIRST', 'LAST' or 'NEVER' expected. 'FIRST' will be used instead.")
  else()
    set (_${_PYTHON_PREFIX}_FIND_FRAMEWORK ${${_PYTHON_PREFIX}_FIND_FRAMEWORK})
  endif()
elseif (DEFINED CMAKE_FIND_FRAMEWORK)
  if (CMAKE_FIND_FRAMEWORK STREQUAL "ONLY")
    message (AUTHOR_WARNING "Find${_PYTHON_PREFIX}: CMAKE_FIND_FRAMEWORK: 'ONLY' value is not supported. 'FIRST' will be used instead.")
  elseif (NOT CMAKE_FIND_FRAMEWORK MATCHES "^(FIRST|LAST|NEVER)$")
    message (AUTHOR_WARNING "Find${_PYTHON_PREFIX}: ${CMAKE_FIND_FRAMEWORK}: invalid value for 'CMAKE_FIND_FRAMEWORK'. 'FIRST', 'LAST' or 'NEVER' expected. 'FIRST' will be used instead.")
  else()
    set (_${_PYTHON_PREFIX}_FIND_FRAMEWORK ${CMAKE_FIND_FRAMEWORK})
  endif()
endif()

# Save CMAKE_FIND_APPBUNDLE
if (DEFINED CMAKE_FIND_APPBUNDLE)
  set (_${_PYTHON_PREFIX}_CMAKE_FIND_APPBUNDLE ${CMAKE_FIND_APPBUNDLE})
else()
  unset (_${_PYTHON_PREFIX}_CMAKE_FIND_APPBUNDLE)
endif()
# To avoid app bundle lookup
set (CMAKE_FIND_APPBUNDLE "NEVER")

# Save CMAKE_FIND_FRAMEWORK
if (DEFINED CMAKE_FIND_FRAMEWORK)
  set (_${_PYTHON_PREFIX}_CMAKE_FIND_FRAMEWORK ${CMAKE_FIND_FRAMEWORK})
else()
  unset (_${_PYTHON_PREFIX}_CMAKE_FIND_FRAMEWORK)
endif()
# To avoid framework lookup
set (CMAKE_FIND_FRAMEWORK "NEVER")

# Windows Registry handling
if (DEFINED ${_PYTHON_PREFIX}_FIND_REGISTRY)
  if (NOT ${_PYTHON_PREFIX}_FIND_REGISTRY MATCHES "^(FIRST|LAST|NEVER)$")
    message (AUTHOR_WARNING "Find${_PYTHON_PREFIX}: ${${_PYTHON_PREFIX}_FIND_REGISTRY}: invalid value for '${_PYTHON_PREFIX}_FIND_REGISTRY'. 'FIRST', 'LAST' or 'NEVER' expected. 'FIRST' will be used instead.")
    set (_${_PYTHON_PREFIX}_FIND_REGISTRY "FIRST")
  else()
    set (_${_PYTHON_PREFIX}_FIND_REGISTRY ${${_PYTHON_PREFIX}_FIND_REGISTRY})
  endif()
else()
  set (_${_PYTHON_PREFIX}_FIND_REGISTRY "FIRST")
endif()

# virtual environments handling
if (DEFINED ENV{VIRTUAL_ENV})
  if (DEFINED ${_PYTHON_PREFIX}_FIND_VIRTUALENV)
    if (NOT ${_PYTHON_PREFIX}_FIND_VIRTUALENV MATCHES "^(FIRST|ONLY|STANDARD)$")
      message (AUTHOR_WARNING "Find${_PYTHON_PREFIX}: ${${_PYTHON_PREFIX}_FIND_VIRTUALENV}: invalid value for '${_PYTHON_PREFIX}_FIND_VIRTUALENV'. 'FIRST', 'ONLY' or 'STANDARD' expected. 'FIRST' will be used instead.")
      set (_${_PYTHON_PREFIX}_FIND_VIRTUALENV "FIRST")
    else()
      set (_${_PYTHON_PREFIX}_FIND_VIRTUALENV ${${_PYTHON_PREFIX}_FIND_VIRTUALENV})
    endif()
  else()
    set (_${_PYTHON_PREFIX}_FIND_VIRTUALENV FIRST)
  endif()
else()
  set (_${_PYTHON_PREFIX}_FIND_VIRTUALENV STANDARD)
endif()


unset (_${_PYTHON_PREFIX}_REQUIRED_VARS)
unset (_${_PYTHON_PREFIX}_CACHED_VARS)


# first step, search for the interpreter
if ("Interpreter" IN_LIST ${_PYTHON_PREFIX}_FIND_COMPONENTS)
  list (APPEND _${_PYTHON_PREFIX}_CACHED_VARS ${_PYTHON_PREFIX}_EXECUTABLE)
  if (${_PYTHON_PREFIX}_FIND_REQUIRED_Interpreter)
    list (APPEND _${_PYTHON_PREFIX}_REQUIRED_VARS ${_PYTHON_PREFIX}_EXECUTABLE)
  endif()

  set (_${_PYTHON_PREFIX}_HINTS "${${_PYTHON_PREFIX}_ROOT_DIR}" ENV ${_PYTHON_PREFIX}_ROOT_DIR)

  if (_${_PYTHON_PREFIX}_FIND_STRATEGY STREQUAL "LOCATION")
    unset (_${_PYTHON_PREFIX}_NAMES)
    unset (_${_PYTHON_PREFIX}_FRAMEWORK_PATHS)
    unset (_${_PYTHON_PREFIX}_REGISTRY_PATHS)

    foreach (_${_PYTHON_PREFIX}_VERSION IN LISTS _${_PYTHON_PREFIX}_FIND_VERSIONS)
      # build all executable names
      list (APPEND _${_PYTHON_PREFIX}_NAMES python${_${_PYTHON_PREFIX}_VERSION})

      # Framework Paths
      _python_get_frameworks (_${_PYTHON_PREFIX}_VERSION_PATHS ${_${_PYTHON_PREFIX}_VERSION})
      list (APPEND _${_PYTHON_PREFIX}_FRAMEWORK_PATHS ${_${_PYTHON_PREFIX}_VERSION_PATHS})

      # Registry Paths
      _python_get_registries (_${_PYTHON_PREFIX}_VERSION_PATHS ${_${_PYTHON_PREFIX}_VERSION})
      list (APPEND _${_PYTHON_PREFIX}_REGISTRY_PATHS ${_${_PYTHON_PREFIX}_VERSION_PATHS}
                   [HKEY_LOCAL_MACHINE\\SOFTWARE\\IronPython\\${_${_PYTHON_PREFIX}_VERSION}\\InstallPath])
    endforeach()
    list (APPEND _${_PYTHON_PREFIX}_NAMES python${_${_PYTHON_PREFIX}_REQUIRED_VERSION_MAJOR} python)

    while (TRUE)
      # Virtual environments handling
      if (_${_PYTHON_PREFIX}_FIND_VIRTUALENV MATCHES "^(FIRST|ONLY)$")
        find_program (${_PYTHON_PREFIX}_EXECUTABLE
                      NAMES ${_${_PYTHON_PREFIX}_NAMES}
                      NAMES_PER_DIR
                      HINTS ${_${_PYTHON_PREFIX}_HINTS}
                      PATHS ENV VIRTUAL_ENV
                      PATH_SUFFIXES bin Scripts
                      NO_CMAKE_PATH
                      NO_CMAKE_ENVIRONMENT_PATH
                      NO_SYSTEM_ENVIRONMENT_PATH
                      NO_CMAKE_SYSTEM_PATH)

        _python_validate_interpreter (${${_PYTHON_PREFIX}_FIND_VERSION})
        if (${_PYTHON_PREFIX}_EXECUTABLE)
          break()
        endif()
        if (NOT _${_PYTHON_PREFIX}_FIND_VIRTUALENV STREQUAL "ONLY")
          break()
        endif()
      endif()

      # Apple frameworks handling
      if (CMAKE_HOST_APPLE AND _${_PYTHON_PREFIX}_FIND_FRAMEWORK STREQUAL "FIRST")
        find_program (${_PYTHON_PREFIX}_EXECUTABLE
                      NAMES ${_${_PYTHON_PREFIX}_NAMES}
                      NAMES_PER_DIR
                      HINTS ${_${_PYTHON_PREFIX}_HINTS}
                      PATHS ${_${_PYTHON_PREFIX}_FRAMEWORK_PATHS}
                      PATH_SUFFIXES bin
                      NO_CMAKE_PATH
                      NO_CMAKE_ENVIRONMENT_PATH
                      NO_SYSTEM_ENVIRONMENT_PATH
                      NO_CMAKE_SYSTEM_PATH)
        _python_validate_interpreter (${${_PYTHON_PREFIX}_FIND_VERSION})
        if (${_PYTHON_PREFIX}_EXECUTABLE)
          break()
        endif()
      endif()
      # Windows registry
      if (CMAKE_HOST_WIN32 AND _${_PYTHON_PREFIX}_FIND_REGISTRY STREQUAL "FIRST")
        find_program (${_PYTHON_PREFIX}_EXECUTABLE
                      NAMES ${_${_PYTHON_PREFIX}_NAMES}
                            ${_${_PYTHON_PREFIX}_IRON_PYTHON_NAMES}
                      NAMES_PER_DIR
                      HINTS ${_${_PYTHON_PREFIX}_HINTS}
                      PATHS ${_${_PYTHON_PREFIX}_REGISTRY_PATHS}
                      PATH_SUFFIXES bin ${_${_PYTHON_PREFIX}_IRON_PYTHON_PATH_SUFFIXES}
                      NO_SYSTEM_ENVIRONMENT_PATH
                      NO_CMAKE_SYSTEM_PATH)
        _python_validate_interpreter (${${_PYTHON_PREFIX}_FIND_VERSION})
        if (${_PYTHON_PREFIX}_EXECUTABLE)
          break()
        endif()
      endif()

      # try using HINTS and standard paths
      find_program (${_PYTHON_PREFIX}_EXECUTABLE
                    NAMES ${_${_PYTHON_PREFIX}_NAMES}
                          ${_${_PYTHON_PREFIX}_IRON_PYTHON_NAMES}
                    NAMES_PER_DIR
                    HINTS ${_${_PYTHON_PREFIX}_HINTS}
                    PATH_SUFFIXES bin ${_${_PYTHON_PREFIX}_IRON_PYTHON_PATH_SUFFIXES})
      _python_validate_interpreter (${${_PYTHON_PREFIX}_FIND_VERSION})
      if (${_PYTHON_PREFIX}_EXECUTABLE)
        break()
      endif()

      # Apple frameworks handling
      if (CMAKE_HOST_APPLE AND _${_PYTHON_PREFIX}_FIND_FRAMEWORK STREQUAL "LAST")
        find_program (${_PYTHON_PREFIX}_EXECUTABLE
                      NAMES ${_${_PYTHON_PREFIX}_NAMES}
                      NAMES_PER_DIR
                      PATHS ${_${_PYTHON_PREFIX}_FRAMEWORK_PATHS}
                      PATH_SUFFIXES bin
                      NO_DEFAULT_PATH)
        _python_validate_interpreter (${${_PYTHON_PREFIX}_FIND_VERSION})
        if (${_PYTHON_PREFIX}_EXECUTABLE)
          break()
        endif()
      endif()
      # Windows registry
      if (CMAKE_HOST_WIN32 AND _${_PYTHON_PREFIX}_FIND_REGISTRY STREQUAL "LAST")
        find_program (${_PYTHON_PREFIX}_EXECUTABLE
                      NAMES ${_${_PYTHON_PREFIX}_NAMES}
                            ${_${_PYTHON_PREFIX}_IRON_PYTHON_NAMES}
                      NAMES_PER_DIR
                      PATHS ${_${_PYTHON_PREFIX}_REGISTRY_PATHS}
                      PATH_SUFFIXES bin ${_${_PYTHON_PREFIX}_IRON_PYTHON_PATH_SUFFIXES}
                      NO_DEFAULT_PATH)
        _python_validate_interpreter (${${_PYTHON_PREFIX}_FIND_VERSION})
        if (${_PYTHON_PREFIX}_EXECUTABLE)
          break()
        endif()
      endif()

      break()
    endwhile()
  else()
    # look-up for various versions and locations
    foreach (_${_PYTHON_PREFIX}_VERSION IN LISTS _${_PYTHON_PREFIX}_FIND_VERSIONS)
      set (_${_PYTHON_PREFIX}_NAMES python${_${_PYTHON_PREFIX}_VERSION}
                                    python${_${_PYTHON_PREFIX}_REQUIRED_VERSION_MAJOR}
                                    python)

      _python_get_frameworks (_${_PYTHON_PREFIX}_FRAMEWORK_PATHS ${_${_PYTHON_PREFIX}_VERSION})
      _python_get_registries (_${_PYTHON_PREFIX}_REGISTRY_PATHS ${_${_PYTHON_PREFIX}_VERSION})

      # Virtual environments handling
      if (_${_PYTHON_PREFIX}_FIND_VIRTUALENV MATCHES "^(FIRST|ONLY)$")
        find_program (${_PYTHON_PREFIX}_EXECUTABLE
                      NAMES ${_${_PYTHON_PREFIX}_NAMES}
                      NAMES_PER_DIR
                      HINTS ${_${_PYTHON_PREFIX}_HINTS}
                      PATHS ENV VIRTUAL_ENV
                      PATH_SUFFIXES bin Scripts
                      NO_CMAKE_PATH
                      NO_CMAKE_ENVIRONMENT_PATH
                      NO_SYSTEM_ENVIRONMENT_PATH
                      NO_CMAKE_SYSTEM_PATH)

        _python_validate_interpreter (${_${_PYTHON_PREFIX}_VERSION} EXACT)
        if (${_PYTHON_PREFIX}_EXECUTABLE)
          break()
        endif()
        if (_${_PYTHON_PREFIX}_FIND_VIRTUALENV STREQUAL "ONLY")
          continue()
        endif()
      endif()

      # Apple frameworks handling
      if (CMAKE_HOST_APPLE AND _${_PYTHON_PREFIX}_FIND_FRAMEWORK STREQUAL "FIRST")
        find_program (${_PYTHON_PREFIX}_EXECUTABLE
                      NAMES ${_${_PYTHON_PREFIX}_NAMES}
                      NAMES_PER_DIR
                      HINTS ${_${_PYTHON_PREFIX}_HINTS}
                      PATHS ${_${_PYTHON_PREFIX}_FRAMEWORK_PATHS}
                      PATH_SUFFIXES bin
                      NO_CMAKE_PATH
                      NO_CMAKE_ENVIRONMENT_PATH
                      NO_SYSTEM_ENVIRONMENT_PATH
                      NO_CMAKE_SYSTEM_PATH)
      endif()

      # Windows registry
      if (CMAKE_HOST_WIN32 AND _${_PYTHON_PREFIX}_FIND_REGISTRY STREQUAL "FIRST")
        find_program (${_PYTHON_PREFIX}_EXECUTABLE
                      NAMES ${_${_PYTHON_PREFIX}_NAMES}
                            ${_${_PYTHON_PREFIX}_IRON_PYTHON_NAMES}
                      NAMES_PER_DIR
                      HINTS ${_${_PYTHON_PREFIX}_HINTS}
                      PATHS ${_${_PYTHON_PREFIX}_REGISTRY_PATHS}
                            [HKEY_LOCAL_MACHINE\\SOFTWARE\\IronPython\\${_${_PYTHON_PREFIX}_VERSION}\\InstallPath]
                      PATH_SUFFIXES bin ${_${_PYTHON_PREFIX}_IRON_PYTHON_PATH_SUFFIXES}
                      NO_SYSTEM_ENVIRONMENT_PATH
                      NO_CMAKE_SYSTEM_PATH)
      endif()
      _python_validate_interpreter (${_${_PYTHON_PREFIX}_VERSION} EXACT)
      if (${_PYTHON_PREFIX}_EXECUTABLE)
        break()
      endif()

      # try using HINTS
      find_program (${_PYTHON_PREFIX}_EXECUTABLE
                    NAMES ${_${_PYTHON_PREFIX}_NAMES}
                          ${_${_PYTHON_PREFIX}_IRON_PYTHON_NAMES}
                    NAMES_PER_DIR
                    HINTS ${_${_PYTHON_PREFIX}_HINTS}
                    PATH_SUFFIXES bin ${_${_PYTHON_PREFIX}_IRON_PYTHON_PATH_SUFFIXES}
                    NO_SYSTEM_ENVIRONMENT_PATH
                    NO_CMAKE_SYSTEM_PATH)
      _python_validate_interpreter (${_${_PYTHON_PREFIX}_VERSION} EXACT)
      if (${_PYTHON_PREFIX}_EXECUTABLE)
        break()
      endif()
      # try using standard paths.
      # NAMES_PER_DIR is not defined on purpose to have a chance to find
      # expected version.
      # For example, typical systems have 'python' for version 2.* and 'python3'
      # for version 3.*. So looking for names per dir will find, potentially,
      # systematically 'python' (i.e. version 2) even if version 3 is searched.
      if (CMAKE_HOST_WIN32)
        find_program (${_PYTHON_PREFIX}_EXECUTABLE
                      NAMES python${_${_PYTHON_PREFIX}_VERSION}
                            python
                            ${_${_PYTHON_PREFIX}_IRON_PYTHON_NAMES})
      else()
        find_program (${_PYTHON_PREFIX}_EXECUTABLE
                      NAMES python${_${_PYTHON_PREFIX}_VERSION})
      endif()
      _python_validate_interpreter (${_${_PYTHON_PREFIX}_VERSION} EXACT)
      if (${_PYTHON_PREFIX}_EXECUTABLE)
        break()
      endif()

      # Apple frameworks handling
      if (CMAKE_HOST_APPLE AND _${_PYTHON_PREFIX}_FIND_FRAMEWORK STREQUAL "LAST")
        find_program (${_PYTHON_PREFIX}_EXECUTABLE
                      NAMES ${_${_PYTHON_PREFIX}_NAMES}
                      NAMES_PER_DIR
                      PATHS ${_${_PYTHON_PREFIX}_FRAMEWORK_PATHS}
                      PATH_SUFFIXES bin
                      NO_DEFAULT_PATH)
      endif()

      # Windows registry
      if (CMAKE_HOST_WIN32 AND _${_PYTHON_PREFIX}_FIND_REGISTRY STREQUAL "LAST")
        find_program (${_PYTHON_PREFIX}_EXECUTABLE
                      NAMES ${_${_PYTHON_PREFIX}_NAMES}
                            ${_${_PYTHON_PREFIX}_IRON_PYTHON_NAMES}
                      NAMES_PER_DIR
                      PATHS ${_${_PYTHON_PREFIX}_REGISTRY_PATHS}
                            [HKEY_LOCAL_MACHINE\\SOFTWARE\\IronPython\\${_${_PYTHON_PREFIX}_VERSION}\\InstallPath]
                      PATH_SUFFIXES bin ${_${_PYTHON_PREFIX}_IRON_PYTHON_PATH_SUFFIXES}
                      NO_DEFAULT_PATH)
      endif()

      _python_validate_interpreter (${_${_PYTHON_PREFIX}_VERSION} EXACT)
      if (${_PYTHON_PREFIX}_EXECUTABLE)
        break()
      endif()
    endforeach()

    if (NOT ${_PYTHON_PREFIX}_EXECUTABLE AND
        NOT _${_PYTHON_PREFIX}_FIND_VIRTUALENV STREQUAL "ONLY")
      # No specific version found. Retry with generic names and standard paths.
      # NAMES_PER_DIR is not defined on purpose to have a chance to find
      # expected version.
      # For example, typical systems have 'python' for version 2.* and 'python3'
      # for version 3.*. So looking for names per dir will find, potentially,
      # systematically 'python' (i.e. version 2) even if version 3 is searched.
      find_program (${_PYTHON_PREFIX}_EXECUTABLE
                    NAMES python${_${_PYTHON_PREFIX}_REQUIRED_VERSION_MAJOR}
                          python
                          ${_${_PYTHON_PREFIX}_IRON_PYTHON_NAMES})

      _python_validate_interpreter ()
    endif()
  endif()

  # retrieve exact version of executable found
  if (${_PYTHON_PREFIX}_EXECUTABLE)
    execute_process (COMMAND "${${_PYTHON_PREFIX}_EXECUTABLE}" -c
                             "import sys; sys.stdout.write('.'.join([str(x) for x in sys.version_info[:3]]))"
                     RESULT_VARIABLE _${_PYTHON_PREFIX}_RESULT
                     OUTPUT_VARIABLE ${_PYTHON_PREFIX}_VERSION
                     ERROR_QUIET
                     OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (NOT _${_PYTHON_PREFIX}_RESULT)
      string (REGEX MATCHALL "[0-9]+" _${_PYTHON_PREFIX}_VERSIONS "${${_PYTHON_PREFIX}_VERSION}")
      list (GET _${_PYTHON_PREFIX}_VERSIONS 0 ${_PYTHON_PREFIX}_VERSION_MAJOR)
      list (GET _${_PYTHON_PREFIX}_VERSIONS 1 ${_PYTHON_PREFIX}_VERSION_MINOR)
      list (GET _${_PYTHON_PREFIX}_VERSIONS 2 ${_PYTHON_PREFIX}_VERSION_PATCH)
    else()
      # Interpreter is not usable
      set (${_PYTHON_PREFIX}_EXECUTABLE ${_PYTHON_PREFIX}_EXECUTABLE-NOTFOUND CACHE INTERNAL "" FORCE)
      unset (${_PYTHON_PREFIX}_VERSION)
    endif()
  endif()

  if (${_PYTHON_PREFIX}_EXECUTABLE
      AND ${_PYTHON_PREFIX}_VERSION_MAJOR VERSION_EQUAL _${_PYTHON_PREFIX}_REQUIRED_VERSION_MAJOR)
    set (${_PYTHON_PREFIX}_Interpreter_FOUND TRUE)
    # Use interpreter version for future searches to ensure consistency
    set (_${_PYTHON_PREFIX}_FIND_VERSIONS ${${_PYTHON_PREFIX}_VERSION_MAJOR}.${${_PYTHON_PREFIX}_VERSION_MINOR})
  endif()

  if (${_PYTHON_PREFIX}_Interpreter_FOUND)
    if (NOT CMAKE_SIZEOF_VOID_P)
      # determine interpreter architecture
      execute_process (COMMAND "${${_PYTHON_PREFIX}_EXECUTABLE}" -c "import sys; print(sys.maxsize > 2**32)"
                       RESULT_VARIABLE _${_PYTHON_PREFIX}_RESULT
                       OUTPUT_VARIABLE ${_PYTHON_PREFIX}_IS64BIT
                       ERROR_VARIABLE ${_PYTHON_PREFIX}_IS64BIT)
      if (NOT _${_PYTHON_PREFIX}_RESULT)
        if (${_PYTHON_PREFIX}_IS64BIT)
          set (_${_PYTHON_PREFIX}_ARCH 64)
          set (_${_PYTHON_PREFIX}_ARCH2 64)
        else()
          set (_${_PYTHON_PREFIX}_ARCH 32)
          set (_${_PYTHON_PREFIX}_ARCH2 32)
        endif()
      endif()
    endif()

    # retrieve interpreter identity
    execute_process (COMMAND "${${_PYTHON_PREFIX}_EXECUTABLE}" -V
                     RESULT_VARIABLE _${_PYTHON_PREFIX}_RESULT
                     OUTPUT_VARIABLE ${_PYTHON_PREFIX}_INTERPRETER_ID
                     ERROR_VARIABLE ${_PYTHON_PREFIX}_INTERPRETER_ID)
    if (NOT _${_PYTHON_PREFIX}_RESULT)
      if (${_PYTHON_PREFIX}_INTERPRETER_ID MATCHES "Anaconda")
        set (${_PYTHON_PREFIX}_INTERPRETER_ID "Anaconda")
      elseif (${_PYTHON_PREFIX}_INTERPRETER_ID MATCHES "Enthought")
        set (${_PYTHON_PREFIX}_INTERPRETER_ID "Canopy")
      else()
        string (REGEX REPLACE "^([^ ]+).*" "\\1" ${_PYTHON_PREFIX}_INTERPRETER_ID "${${_PYTHON_PREFIX}_INTERPRETER_ID}")
        if (${_PYTHON_PREFIX}_INTERPRETER_ID STREQUAL "Python")
          # try to get a more precise ID
          execute_process (COMMAND "${${_PYTHON_PREFIX}_EXECUTABLE}" -c "import sys; print(sys.copyright)"
                           RESULT_VARIABLE _${_PYTHON_PREFIX}_RESULT
                           OUTPUT_VARIABLE ${_PYTHON_PREFIX}_COPYRIGHT
                           ERROR_QUIET)
          if (${_PYTHON_PREFIX}_COPYRIGHT MATCHES "ActiveState")
            set (${_PYTHON_PREFIX}_INTERPRETER_ID "ActivePython")
          endif()
        endif()
      endif()
    else()
      set (${_PYTHON_PREFIX}_INTERPRETER_ID Python)
    endif()
  else()
    unset (${_PYTHON_PREFIX}_INTERPRETER_ID)
  endif()

  # retrieve various package installation directories
  execute_process (COMMAND "${${_PYTHON_PREFIX}_EXECUTABLE}" -c "import sys; from distutils import sysconfig;sys.stdout.write(';'.join([sysconfig.get_python_lib(plat_specific=False,standard_lib=True),sysconfig.get_python_lib(plat_specific=True,standard_lib=True),sysconfig.get_python_lib(plat_specific=False,standard_lib=False),sysconfig.get_python_lib(plat_specific=True,standard_lib=False)]))"

                   RESULT_VARIABLE _${_PYTHON_PREFIX}_RESULT
                   OUTPUT_VARIABLE _${_PYTHON_PREFIX}_LIBPATHS
                   ERROR_QUIET)
  if (NOT _${_PYTHON_PREFIX}_RESULT)
    list (GET _${_PYTHON_PREFIX}_LIBPATHS 0 ${_PYTHON_PREFIX}_STDLIB)
    list (GET _${_PYTHON_PREFIX}_LIBPATHS 1 ${_PYTHON_PREFIX}_STDARCH)
    list (GET _${_PYTHON_PREFIX}_LIBPATHS 2 ${_PYTHON_PREFIX}_SITELIB)
    list (GET _${_PYTHON_PREFIX}_LIBPATHS 3 ${_PYTHON_PREFIX}_SITEARCH)
  else()
    unset (${_PYTHON_PREFIX}_STDLIB)
    unset (${_PYTHON_PREFIX}_STDARCH)
    unset (${_PYTHON_PREFIX}_SITELIB)
    unset (${_PYTHON_PREFIX}_SITEARCH)
  endif()

  mark_as_advanced (${_PYTHON_PREFIX}_EXECUTABLE)
endif()


# second step, search for compiler (IronPython)
if ("Compiler" IN_LIST ${_PYTHON_PREFIX}_FIND_COMPONENTS)
  list (APPEND _${_PYTHON_PREFIX}_CACHED_VARS ${_PYTHON_PREFIX}_COMPILER)
  if (${_PYTHON_PREFIX}_FIND_REQUIRED_Compiler)
    list (APPEND _${_PYTHON_PREFIX}_REQUIRED_VARS ${_PYTHON_PREFIX}_COMPILER)
  endif()

  # IronPython specific artifacts
  # If IronPython interpreter is found, use its path
  unset (_${_PYTHON_PREFIX}_IRON_ROOT)
  if (${_PYTHON_PREFIX}_Interpreter_FOUND AND ${_PYTHON_PREFIX}_INTERPRETER_ID STREQUAL "IronPython")
    get_filename_component (_${_PYTHON_PREFIX}_IRON_ROOT "${${_PYTHON_PREFIX}_EXECUTABLE}" DIRECTORY)
  endif()

  if (_${_PYTHON_PREFIX}_FIND_STRATEGY STREQUAL "LOCATION")
    set (_${_PYTHON_PREFIX}_REGISTRY_PATHS)

    foreach (_${_PYTHON_PREFIX}_VERSION IN LISTS _${_PYTHON_PREFIX}_FIND_VERSIONS)
      # Registry Paths
      list (APPEND _${_PYTHON_PREFIX}_REGISTRY_PATHS
                   [HKEY_LOCAL_MACHINE\\SOFTWARE\\IronPython\\${_${_PYTHON_PREFIX}_VERSION}\\InstallPath])
    endforeach()

    while (TRUE)
      if (_${_PYTHON_PREFIX}_FIND_REGISTRY STREQUAL "FIRST")
        find_program (${_PYTHON_PREFIX}_COMPILER
                      NAMES ipyc
                      HINTS ${_${_PYTHON_PREFIX}_IRON_ROOT} ${_${_PYTHON_PREFIX}_HINTS}
                      PATHS ${_${_PYTHON_PREFIX}_REGISTRY_PATHS}
                      PATH_SUFFIXES ${_${_PYTHON_PREFIX}_IRON_PYTHON_PATH_SUFFIXES}
                      NO_SYSTEM_ENVIRONMENT_PATH
                      NO_CMAKE_SYSTEM_PATH)
        _python_validate_compiler (${${_PYTHON_PREFIX}_FIND_VERSION})
        if (${_PYTHON_PREFIX}_COMPILER)
          break()
        endif()
      endif()

      find_program (${_PYTHON_PREFIX}_COMPILER
                    NAMES ipyc
                    HINTS ${_${_PYTHON_PREFIX}_IRON_ROOT} ${_${_PYTHON_PREFIX}_HINTS}
                    PATH_SUFFIXES ${_${_PYTHON_PREFIX}_IRON_PYTHON_PATH_SUFFIXES}
                    NO_SYSTEM_ENVIRONMENT_PATH
                    NO_CMAKE_SYSTEM_PATH)
      _python_validate_compiler (${${_PYTHON_PREFIX}_FIND_VERSION})
      if (${_PYTHON_PREFIX}_COMPILER)
        break()
      endif()

      if (_${_PYTHON_PREFIX}_FIND_REGISTRY STREQUAL "LAST")
        find_program (${_PYTHON_PREFIX}_COMPILER
                      NAMES ipyc
                      PATHS ${_${_PYTHON_PREFIX}_REGISTRY_PATHS}
                      PATH_SUFFIXES ${_${_PYTHON_PREFIX}_IRON_PYTHON_PATH_SUFFIXES}
                      NO_DEFAULT_PATH)
      endif()

      break()
    endwhile()
  else()
    # try using root dir and registry
    foreach (_${_PYTHON_PREFIX}_VERSION IN LISTS _${_PYTHON_PREFIX}_FIND_VERSIONS)
      if (_${_PYTHON_PREFIX}_FIND_REGISTRY STREQUAL "FIRST")
        find_program (${_PYTHON_PREFIX}_COMPILER
                      NAMES ipyc
                      HINTS ${_${_PYTHON_PREFIX}_IRON_ROOT} ${_${_PYTHON_PREFIX}_HINTS}
                      PATHS [HKEY_LOCAL_MACHINE\\SOFTWARE\\IronPython\\${_${_PYTHON_PREFIX}_VERSION}\\InstallPath]
                      PATH_SUFFIXES ${_${_PYTHON_PREFIX}_IRON_PYTHON_PATH_SUFFIXES}
                      NO_SYSTEM_ENVIRONMENT_PATH
                      NO_CMAKE_SYSTEM_PATH)
        _python_validate_compiler (${_${_PYTHON_PREFIX}_VERSION} EXACT)
        if (${_PYTHON_PREFIX}_COMPILER)
          break()
        endif()
      endif()

      find_program (${_PYTHON_PREFIX}_COMPILER
                    NAMES ipyc
                    HINTS ${_${_PYTHON_PREFIX}_IRON_ROOT} ${_${_PYTHON_PREFIX}_HINTS}
                    PATH_SUFFIXES ${_${_PYTHON_PREFIX}_IRON_PYTHON_PATH_SUFFIXES}
                    NO_SYSTEM_ENVIRONMENT_PATH
                    NO_CMAKE_SYSTEM_PATH)
      _python_validate_compiler (${_${_PYTHON_PREFIX}_VERSION} EXACT)
      if (${_PYTHON_PREFIX}_COMPILER)
        break()
      endif()

      if (_${_PYTHON_PREFIX}_FIND_REGISTRY STREQUAL "LAST")
        find_program (${_PYTHON_PREFIX}_COMPILER
                      NAMES ipyc
                      PATHS [HKEY_LOCAL_MACHINE\\SOFTWARE\\IronPython\\${_${_PYTHON_PREFIX}_VERSION}\\InstallPath]
                      PATH_SUFFIXES ${_${_PYTHON_PREFIX}_IRON_PYTHON_PATH_SUFFIXES}
                      NO_DEFAULT_PATH)
        _python_validate_compiler (${_${_PYTHON_PREFIX}_VERSION} EXACT)
        if (${_PYTHON_PREFIX}_COMPILER)
          break()
        endif()
      endif()
    endforeach()

    # no specific version found, re-try in standard paths
    find_program (${_PYTHON_PREFIX}_COMPILER
                  NAMES ipyc
                  HINTS ${_${_PYTHON_PREFIX}_IRON_ROOT} ${_${_PYTHON_PREFIX}_HINTS}
                  PATH_SUFFIXES ${_${_PYTHON_PREFIX}_IRON_PYTHON_PATH_SUFFIXES})
  endif()

  if (${_PYTHON_PREFIX}_COMPILER)
    # retrieve python environment version from compiler
    set (_${_PYTHON_PREFIX}_VERSION_DIR "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/PythonCompilerVersion.dir")
    file (WRITE "${_${_PYTHON_PREFIX}_VERSION_DIR}/version.py" "import sys; sys.stdout.write('.'.join([str(x) for x in sys.version_info[:3]]))\n")
    execute_process (COMMAND "${${_PYTHON_PREFIX}_COMPILER}" /target:exe /embed "${_${_PYTHON_PREFIX}_VERSION_DIR}/version.py"
                     WORKING_DIRECTORY "${_${_PYTHON_PREFIX}_VERSION_DIR}"
                     OUTPUT_QUIET
                     ERROR_QUIET)
    execute_process (COMMAND "${_${_PYTHON_PREFIX}_VERSION_DIR}/version"
                     WORKING_DIRECTORY "${_${_PYTHON_PREFIX}_VERSION_DIR}"
                     RESULT_VARIABLE _${_PYTHON_PREFIX}_RESULT
                     OUTPUT_VARIABLE _${_PYTHON_PREFIX}_VERSION
                     ERROR_QUIET)
    if (NOT _${_PYTHON_PREFIX}_RESULT)
      string (REGEX MATCHALL "[0-9]+" _${_PYTHON_PREFIX}_VERSIONS "${_${_PYTHON_PREFIX}_VERSION}")
      list (GET _${_PYTHON_PREFIX}_VERSIONS 0 _${_PYTHON_PREFIX}_VERSION_MAJOR)
      list (GET _${_PYTHON_PREFIX}_VERSIONS 1 _${_PYTHON_PREFIX}_VERSION_MINOR)
      list (GET _${_PYTHON_PREFIX}_VERSIONS 2 _${_PYTHON_PREFIX}_VERSION_PATCH)

      if (NOT ${_PYTHON_PREFIX}_Interpreter_FOUND)
        # set public version information
        set (${_PYTHON_PREFIX}_VERSION ${_${_PYTHON_PREFIX}_VERSION})
        set (${_PYTHON_PREFIX}_VERSION_MAJOR ${_${_PYTHON_PREFIX}_VERSION_MAJOR})
        set (${_PYTHON_PREFIX}_VERSION_MINOR ${_${_PYTHON_PREFIX}_VERSION_MINOR})
        set (${_PYTHON_PREFIX}_VERSION_PATCH ${_${_PYTHON_PREFIX}_VERSION_PATCH})
      endif()
    else()
      # compiler not usable
      set (${_PYTHON_PREFIX}_COMPILER ${_PYTHON_PREFIX}_COMPILER-NOTFOUND CACHE INTERNAL "" FORCE)
    endif()
    file (REMOVE_RECURSE "${_${_PYTHON_PREFIX}_VERSION_DIR}")
  endif()

  if (${_PYTHON_PREFIX}_COMPILER)
    if (${_PYTHON_PREFIX}_Interpreter_FOUND)
      # Compiler must be compatible with interpreter
      if (${_${_PYTHON_PREFIX}_VERSION_MAJOR}.${_${_PYTHON_PREFIX}_VERSION_MINOR} VERSION_EQUAL ${${_PYTHON_PREFIX}_VERSION_MAJOR}.${${_PYTHON_PREFIX}_VERSION_MINOR})
        set (${_PYTHON_PREFIX}_Compiler_FOUND TRUE)
      endif()
    elseif (${_PYTHON_PREFIX}_VERSION_MAJOR VERSION_EQUAL _${_PYTHON_PREFIX}_REQUIRED_VERSION_MAJOR)
      set (${_PYTHON_PREFIX}_Compiler_FOUND TRUE)
    # Use compiler version for future searches to ensure consistency
    set (_${_PYTHON_PREFIX}_FIND_VERSIONS ${${_PYTHON_PREFIX}_VERSION_MAJOR}.${${_PYTHON_PREFIX}_VERSION_MINOR})
    endif()
  endif()

  if (${_PYTHON_PREFIX}_Compiler_FOUND)
    set (${_PYTHON_PREFIX}_COMPILER_ID IronPython)
  else()
    unset (${_PYTHON_PREFIX}_COMPILER_ID)
  endif()

  mark_as_advanced (${_PYTHON_PREFIX}_COMPILER)
endif()


# third step, search for the development artifacts
## Development environment is not compatible with IronPython interpreter
if ("Development" IN_LIST ${_PYTHON_PREFIX}_FIND_COMPONENTS
    AND NOT ${_PYTHON_PREFIX}_INTERPRETER_ID STREQUAL "IronPython")
  list (APPEND _${_PYTHON_PREFIX}_CACHED_VARS ${_PYTHON_PREFIX}_LIBRARY
                                              ${_PYTHON_PREFIX}_LIBRARY_RELEASE
                                              ${_PYTHON_PREFIX}_RUNTIME_LIBRARY_RELEASE
                                              ${_PYTHON_PREFIX}_LIBRARY_DEBUG
                                              ${_PYTHON_PREFIX}_RUNTIME_LIBRARY_DEBUG
                                              ${_PYTHON_PREFIX}_INCLUDE_DIR)
  if (${_PYTHON_PREFIX}_FIND_REQUIRED_Development)
    list (APPEND _${_PYTHON_PREFIX}_REQUIRED_VARS ${_PYTHON_PREFIX}_LIBRARY
                                                  ${_PYTHON_PREFIX}_INCLUDE_DIR)
  endif()

  # Support preference of static libs by adjusting CMAKE_FIND_LIBRARY_SUFFIXES
  unset (_${_PYTHON_PREFIX}_CMAKE_FIND_LIBRARY_SUFFIXES)
  if (DEFINED ${_PYTHON_PREFIX}_USE_STATIC_LIBS AND NOT WIN32)
    set(_${_PYTHON_PREFIX}_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
    if(${_PYTHON_PREFIX}_USE_STATIC_LIBS)
      set (CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
    else()
      list (REMOVE_ITEM CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
    endif()
  endif()

  # if python interpreter is found, use its location and version to ensure consistency
  # between interpreter and development environment
  unset (_${_PYTHON_PREFIX}_PREFIX)
  unset (_${_PYTHON_PREFIX}_EXEC_PREFIX)
  unset (_${_PYTHON_PREFIX}_BASE_EXEC_PREFIX)
  if (${_PYTHON_PREFIX}_Interpreter_FOUND)
    execute_process (COMMAND "${${_PYTHON_PREFIX}_EXECUTABLE}" -c
                             "import sys; from distutils import sysconfig; sys.stdout.write(sysconfig.EXEC_PREFIX)"
                     RESULT_VARIABLE _${_PYTHON_PREFIX}_RESULT
                     OUTPUT_VARIABLE _${_PYTHON_PREFIX}_EXEC_PREFIX
                     ERROR_QUIET
                     OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (_${_PYTHON_PREFIX}_RESULT)
      unset (_${_PYTHON_PREFIX}_EXEC_PREFIX)
    endif()

    if (NOT ${_PYTHON_PREFIX}_FIND_VIRTUALENV STREQUAL "STANDARD")
      execute_process (COMMAND "${${_PYTHON_PREFIX}_EXECUTABLE}" -c
                               "import sys; from distutils import sysconfig; sys.stdout.write(sysconfig.BASE_EXEC_PREFIX)"
                       RESULT_VARIABLE _${_PYTHON_PREFIX}_RESULT
                       OUTPUT_VARIABLE _${_PYTHON_PREFIX}_BASE_EXEC_PREFIX
                       ERROR_QUIET
                       OUTPUT_STRIP_TRAILING_WHITESPACE)
      if (_${_PYTHON_PREFIX}_RESULT)
        unset (_${_PYTHON_PREFIX}_BASE_EXEC_PREFIX)
      endif()
    endif()
  endif()
  set (_${_PYTHON_PREFIX}_HINTS "${_${_PYTHON_PREFIX}_EXEC_PREFIX}" "${_${_PYTHON_PREFIX}_BASE_EXEC_PREFIX}" "${${_PYTHON_PREFIX}_ROOT_DIR}" ENV ${_PYTHON_PREFIX}_ROOT_DIR)

  if (_${_PYTHON_PREFIX}_FIND_STRATEGY STREQUAL "LOCATION")
    set (_${_PYTHON_PREFIX}_CONFIG_NAMES)

    foreach (_${_PYTHON_PREFIX}_VERSION IN LISTS _${_PYTHON_PREFIX}_FIND_VERSIONS)
      if (DEFINED CMAKE_LIBRARY_ARCHITECTURE)
        list (APPEND _${_PYTHON_PREFIX}_CONFIG_NAMES "${CMAKE_LIBRARY_ARCHITECTURE}-python${_${_PYTHON_PREFIX}_VERSION}-config")
      endif()
      list (APPEND _${_PYTHON_PREFIX}_CONFIG_NAMES "python${_${_PYTHON_PREFIX}_VERSION}-config")
    endforeach()

    find_program (_${_PYTHON_PREFIX}_CONFIG
                  NAMES ${_${_PYTHON_PREFIX}_CONFIG_NAMES}
                  NAMES_PER_DIR
                  HINTS ${_${_PYTHON_PREFIX}_HINTS}
                  PATH_SUFFIXES bin)

    if (_${_PYTHON_PREFIX}_CONFIG AND DEFINED CMAKE_LIBRARY_ARCHITECTURE)
      # check that config tool match library architecture
      execute_process (COMMAND "${_${_PYTHON_PREFIX}_CONFIG}" --configdir
                       RESULT_VARIABLE _${_PYTHON_PREFIX}_RESULT
                       OUTPUT_VARIABLE _${_PYTHON_PREFIX}_CONFIGDIR
                       ERROR_QUIET
                       OUTPUT_STRIP_TRAILING_WHITESPACE)
      if (_${_PYTHON_PREFIX}_RESULT)
        unset (_${_PYTHON_PREFIX}_CONFIG CACHE)
      else()
        string(FIND "${_${_PYTHON_PREFIX}_CONFIGDIR}" "${CMAKE_LIBRARY_ARCHITECTURE}" _${_PYTHON_PREFIX}_RESULT)
        if (_${_PYTHON_PREFIX}_RESULT EQUAL -1)
          unset (_${_PYTHON_PREFIX}_CONFIG CACHE)
        endif()
      endif()
    endif()
  else()
    foreach (_${_PYTHON_PREFIX}_VERSION IN LISTS _${_PYTHON_PREFIX}_FIND_VERSIONS)
      # try to use pythonX.Y-config tool
      set (_${_PYTHON_PREFIX}_CONFIG_NAMES)
      if (DEFINED CMAKE_LIBRARY_ARCHITECTURE)
        set (_${_PYTHON_PREFIX}_CONFIG_NAMES "${CMAKE_LIBRARY_ARCHITECTURE}-python${_${_PYTHON_PREFIX}_VERSION}-config")
      endif()
      list (APPEND _${_PYTHON_PREFIX}_CONFIG_NAMES "python${_${_PYTHON_PREFIX}_VERSION}-config")
      find_program (_${_PYTHON_PREFIX}_CONFIG
                    NAMES ${_${_PYTHON_PREFIX}_CONFIG_NAMES}
                    NAMES_PER_DIR
                    HINTS ${_${_PYTHON_PREFIX}_HINTS}
                    PATH_SUFFIXES bin)
      unset (_${_PYTHON_PREFIX}_CONFIG_NAMES)

      if (NOT _${_PYTHON_PREFIX}_CONFIG)
        continue()
      endif()
      if (DEFINED CMAKE_LIBRARY_ARCHITECTURE)
        # check that config tool match library architecture
        execute_process (COMMAND "${_${_PYTHON_PREFIX}_CONFIG}" --configdir
                         RESULT_VARIABLE _${_PYTHON_PREFIX}_RESULT
                         OUTPUT_VARIABLE _${_PYTHON_PREFIX}_CONFIGDIR
                         ERROR_QUIET
                         OUTPUT_STRIP_TRAILING_WHITESPACE)
        if (_${_PYTHON_PREFIX}_RESULT)
          unset (_${_PYTHON_PREFIX}_CONFIG CACHE)
          continue()
        endif()
        string(FIND "${_${_PYTHON_PREFIX}_CONFIGDIR}" "${CMAKE_LIBRARY_ARCHITECTURE}" _${_PYTHON_PREFIX}_RESULT)
        if (_${_PYTHON_PREFIX}_RESULT EQUAL -1)
          unset (_${_PYTHON_PREFIX}_CONFIG CACHE)
          continue()
        endif()
      endif()

      if (_${_PYTHON_PREFIX}_CONFIG)
        break()
      endif()
    endforeach()
  endif()

  if (_${_PYTHON_PREFIX}_CONFIG)
    # retrieve root install directory
    execute_process (COMMAND "${_${_PYTHON_PREFIX}_CONFIG}" --prefix
                     RESULT_VARIABLE _${_PYTHON_PREFIX}_RESULT
                     OUTPUT_VARIABLE _${_PYTHON_PREFIX}_PREFIX
                     ERROR_QUIET
                     OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (_${_PYTHON_PREFIX}_RESULT)
      # python-config is not usable
      unset (_${_PYTHON_PREFIX}_CONFIG CACHE)
    endif()
  endif()

  if (_${_PYTHON_PREFIX}_CONFIG)
    set (_${_PYTHON_PREFIX}_HINTS "${_${_PYTHON_PREFIX}_PREFIX}")

    unset (_${_PYTHON_PREFIX}_LIB_DIRS)
    unset (_${_PYTHON_PREFIX}_PATH_SUFFIXES)
    unset (_${_PYTHON_PREFIX}_LIB_NAMES)

    # retrieve library
    execute_process (COMMAND "${_${_PYTHON_PREFIX}_CONFIG}" --ldflags
                     RESULT_VARIABLE _${_PYTHON_PREFIX}_RESULT
                     OUTPUT_VARIABLE _${_PYTHON_PREFIX}_FLAGS
                     ERROR_QUIET
                     OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (NOT _${_PYTHON_PREFIX}_RESULT)
      # retrieve library directory
      string (REGEX MATCHALL "-L[^ ]+" _${_PYTHON_PREFIX}_LIB_DIRS "${_${_PYTHON_PREFIX}_FLAGS}")
      string (REPLACE "-L" "" _${_PYTHON_PREFIX}_LIB_DIRS "${_${_PYTHON_PREFIX}_LIB_DIRS}")
      if (_${_PYTHON_PREFIX}_CONFIG MATCHES "python([0-9.]+)-config")
        _python_get_path_suffixes (_${_PYTHON_PREFIX}_PATH_SUFFIXES ${CMAKE_MATCH_1} LIBRARY)
      endif()

      # retrieve library name
      string (REGEX MATCHALL "-lpython[^ ]+" _${_PYTHON_PREFIX}_LIB_NAMES "${_${_PYTHON_PREFIX}_FLAGS}")
      string (REPLACE "-l" "" _${_PYTHON_PREFIX}_LIB_NAMES "${_${_PYTHON_PREFIX}_LIB_NAMES}")
      list (REMOVE_DUPLICATES _${_PYTHON_PREFIX}_LIB_NAMES)
    endif()

    execute_process (COMMAND "${_${_PYTHON_PREFIX}_CONFIG}" --configdir
                     RESULT_VARIABLE _${_PYTHON_PREFIX}_RESULT
                     OUTPUT_VARIABLE _${_PYTHON_PREFIX}_CONFIGDIR
                     ERROR_QUIET
                     OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (NOT _${_PYTHON_PREFIX}_RESULT)
      list (APPEND _${_PYTHON_PREFIX}_LIB_DIRS "${_${_PYTHON_PREFIX}_CONFIGDIR}")
    endif()
    list (REMOVE_DUPLICATES _${_PYTHON_PREFIX}_LIB_DIRS)
    list (APPEND _${_PYTHON_PREFIX}_HINTS ${_${_PYTHON_PREFIX}_LIB_DIRS})

    if (NOT _${_PYTHON_PREFIX}_LIB_NAMES)
      # config tool do not specify "-l" option (it is the case starting with 3.8)
      # extract version from the config tool name and list all possible lib names
      if (_${_PYTHON_PREFIX}_CONFIG MATCHES "python([0-9.]+)-config")
        _python_get_lib_names (_${_PYTHON_PREFIX}_LIB_NAMES ${CMAKE_MATCH_1})
      endif()
    endif()

    list (APPEND _${_PYTHON_PREFIX}_HINTS "${${_PYTHON_PREFIX}_ROOT_DIR}" ENV ${_PYTHON_PREFIX}_ROOT_DIR)

    find_library (${_PYTHON_PREFIX}_LIBRARY_RELEASE
                  NAMES ${_${_PYTHON_PREFIX}_LIB_NAMES}
                  NAMES_PER_DIR
                  HINTS ${_${_PYTHON_PREFIX}_HINTS}
                  PATH_SUFFIXES ${_${_PYTHON_PREFIX}_PATH_SUFFIXES}
                  NO_SYSTEM_ENVIRONMENT_PATH
                  NO_CMAKE_SYSTEM_PATH)

    # retrieve runtime library
    if (${_PYTHON_PREFIX}_LIBRARY_RELEASE)
      get_filename_component (_${_PYTHON_PREFIX}_PATH "${${_PYTHON_PREFIX}_LIBRARY_RELEASE}" DIRECTORY)
      get_filename_component (_${_PYTHON_PREFIX}_PATH2 "${_${_PYTHON_PREFIX}_PATH}" DIRECTORY)
      _python_find_runtime_library (${_PYTHON_PREFIX}_RUNTIME_LIBRARY_RELEASE
                                    NAMES ${_${_PYTHON_PREFIX}_LIB_NAMES}
                                    NAMES_PER_DIR
                                    HINTS "${_${_PYTHON_PREFIX}_PATH}" "${_${_PYTHON_PREFIX}_PATH2}" ${_${_PYTHON_PREFIX}_HINTS}
                                    PATH_SUFFIXES bin
                                    NO_SYSTEM_ENVIRONMENT_PATH
                                    NO_CMAKE_SYSTEM_PATH)
    endif()

    # retrieve include directory
    execute_process (COMMAND "${_${_PYTHON_PREFIX}_CONFIG}" --includes
                     RESULT_VARIABLE _${_PYTHON_PREFIX}_RESULT
                     OUTPUT_VARIABLE _${_PYTHON_PREFIX}_FLAGS
                     ERROR_QUIET
                     OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (NOT _${_PYTHON_PREFIX}_RESULT)
      # retrieve include directory
      string (REGEX MATCHALL "-I[^ ]+" _${_PYTHON_PREFIX}_INCLUDE_DIRS "${_${_PYTHON_PREFIX}_FLAGS}")
      string (REPLACE "-I" "" _${_PYTHON_PREFIX}_INCLUDE_DIRS "${_${_PYTHON_PREFIX}_INCLUDE_DIRS}")
      list (REMOVE_DUPLICATES _${_PYTHON_PREFIX}_INCLUDE_DIRS)

      find_path (${_PYTHON_PREFIX}_INCLUDE_DIR
                 NAMES Python.h
                 HINTS ${_${_PYTHON_PREFIX}_INCLUDE_DIRS}
                 NO_SYSTEM_ENVIRONMENT_PATH
                 NO_CMAKE_SYSTEM_PATH)
    endif()
  endif()

  # Rely on HINTS and standard paths if config tool failed to locate artifacts
  if (NOT ${_PYTHON_PREFIX}_LIBRARY_RELEASE OR NOT ${_PYTHON_PREFIX}_INCLUDE_DIR)
    set (_${_PYTHON_PREFIX}_HINTS "${${_PYTHON_PREFIX}_ROOT_DIR}" ENV ${_PYTHON_PREFIX}_ROOT_DIR)

    if (_${_PYTHON_PREFIX}_FIND_STRATEGY STREQUAL "LOCATION")
      unset (_${_PYTHON_PREFIX}_LIB_NAMES)
      unset (_${_PYTHON_PREFIX}_LIB_NAMES_DEBUG)
      unset (_${_PYTHON_PREFIX}_FRAMEWORK_PATHS)
      unset (_${_PYTHON_PREFIX}_REGISTRY_PATHS)
      unset (_${_PYTHON_PREFIX}_PATH_SUFFIXES)

      foreach (_${_PYTHON_PREFIX}_LIB_VERSION IN LISTS _${_PYTHON_PREFIX}_FIND_VERSIONS)
        # library names
        _python_get_lib_names (_${_PYTHON_PREFIX}_VERSION_NAMES ${_${_PYTHON_PREFIX}_LIB_VERSION})
        list (APPEND _${_PYTHON_PREFIX}_LIB_NAMES ${_${_PYTHON_PREFIX}_VERSION_NAMES})
        _python_get_lib_names (_${_PYTHON_PREFIX}_VERSION_NAMES ${_${_PYTHON_PREFIX}_LIB_VERSION} DEBUG)
        list (APPEND _${_PYTHON_PREFIX}_LIB_NAMES_DEBUG ${_${_PYTHON_PREFIX}_VERSION_NAMES})

        # Framework Paths
        _python_get_frameworks (_${_PYTHON_PREFIX}_VERSION_PATHS ${_${_PYTHON_PREFIX}_LIB_VERSION})
        list (APPEND _${_PYTHON_PREFIX}_FRAMEWORK_PATHS ${_${_PYTHON_PREFIX}_VERSION_PATHS})

        # Registry Paths
        _python_get_registries (_${_PYTHON_PREFIX}_VERSION_PATHS ${_${_PYTHON_PREFIX}_LIB_VERSION})
        list (APPEND _${_PYTHON_PREFIX}_REGISTRY_PATHS ${_${_PYTHON_PREFIX}_VERSION_PATHS})

        # Paths suffixes
        _python_get_path_suffixes (_${_PYTHON_PREFIX}_VERSION_PATHS ${_${_PYTHON_PREFIX}_LIB_VERSION} LIBRARY)
        list (APPEND _${_PYTHON_PREFIX}_PATH_SUFFIXES ${_${_PYTHON_PREFIX}_VERSION_PATHS})
      endforeach()

      if (APPLE AND _${_PYTHON_PREFIX}_FIND_FRAMEWORK STREQUAL "FIRST")
        find_library (${_PYTHON_PREFIX}_LIBRARY_RELEASE
                      NAMES ${_${_PYTHON_PREFIX}_LIB_NAMES}
                      NAMES_PER_DIR
                      HINTS ${_${_PYTHON_PREFIX}_HINTS}
                      PATHS ${_${_PYTHON_PREFIX}_FRAMEWORK_PATHS}
                      PATH_SUFFIXES ${_${_PYTHON_PREFIX}_PATH_SUFFIXES}
                      NO_CMAKE_PATH
                      NO_CMAKE_ENVIRONMENT_PATH
                      NO_SYSTEM_ENVIRONMENT_PATH
                      NO_CMAKE_SYSTEM_PATH)
      endif()

      if (WIN32 AND _${_PYTHON_PREFIX}_FIND_REGISTRY STREQUAL "FIRST")
        find_library (${_PYTHON_PREFIX}_LIBRARY_RELEASE
                      NAMES ${_${_PYTHON_PREFIX}_LIB_NAMES}
                      NAMES_PER_DIR
                      HINTS ${_${_PYTHON_PREFIX}_HINTS}
                      PATHS ${_${_PYTHON_PREFIX}_REGISTRY_PATHS}
                      PATH_SUFFIXES ${_${_PYTHON_PREFIX}_PATH_SUFFIXES}
                      NO_SYSTEM_ENVIRONMENT_PATH
                      NO_CMAKE_SYSTEM_PATH)
      endif()

      # search in HINTS locations
      find_library (${_PYTHON_PREFIX}_LIBRARY_RELEASE
                    NAMES ${_${_PYTHON_PREFIX}_LIB_NAMES}
                    NAMES_PER_DIR
                    HINTS ${_${_PYTHON_PREFIX}_HINTS}
                    PATH_SUFFIXES ${_${_PYTHON_PREFIX}_PATH_SUFFIXES}
                    NO_SYSTEM_ENVIRONMENT_PATH
                    NO_CMAKE_SYSTEM_PATH)

      if (APPLE AND _${_PYTHON_PREFIX}_FIND_FRAMEWORK STREQUAL "LAST")
        set (__${_PYTHON_PREFIX}_FRAMEWORK_PATHS ${_${_PYTHON_PREFIX}_FRAMEWORK_PATHS})
      else()
        unset (__${_PYTHON_PREFIX}_FRAMEWORK_PATHS)
      endif()

      if (WIN32 AND _${_PYTHON_PREFIX}_FIND_REGISTRY STREQUAL "LAST")
        set (__${_PYTHON_PREFIX}_REGISTRY_PATHS ${_${_PYTHON_PREFIX}_REGISTRY_PATHS})
      else()
        unset (__${_PYTHON_PREFIX}_REGISTRY_PATHS)
      endif()

      # search in all default paths
      find_library (${_PYTHON_PREFIX}_LIBRARY_RELEASE
                    NAMES ${_${_PYTHON_PREFIX}_LIB_NAMES}
                    NAMES_PER_DIR
                    PATHS ${__${_PYTHON_PREFIX}_FRAMEWORK_PATHS}
                          ${__${_PYTHON_PREFIX}_REGISTRY_PATHS}
                    PATH_SUFFIXES ${_${_PYTHON_PREFIX}_PATH_SUFFIXES})

      if (${_PYTHON_PREFIX}_LIBRARY_RELEASE)
        # extract version from library name
        if (${_PYTHON_PREFIX}_LIBRARY_RELEASE MATCHES "python([23])([0-9]+)")
          set (_${_PYTHON_PREFIX}_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}")
        elseif (${_PYTHON_PREFIX}_LIBRARY_RELEASE MATCHES "python([23])\\.([0-9]+)")
          set (_${_PYTHON_PREFIX}_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}")
        endif()
      endif()

      if (WIN32)
        # search for debug library
        if (${_PYTHON_PREFIX}_LIBRARY_RELEASE)
          # use library location as a hint
          _python_get_lib_names (_${_PYTHON_PREFIX}_LIB_NAMES_DEBUG ${_${_PYTHON_PREFIX}_VERSION} DEBUG)
          get_filename_component (_${_PYTHON_PREFIX}_PATH "${${_PYTHON_PREFIX}_LIBRARY_RELEASE}" DIRECTORY)
          find_library (${_PYTHON_PREFIX}_LIBRARY_DEBUG
                        NAMES ${_${_PYTHON_PREFIX}_LIB_NAMES_DEBUG}
                        NAMES_PER_DIR
                        HINTS "${_${_PYTHON_PREFIX}_PATH}" ${_${_PYTHON_PREFIX}_HINTS}
                        NO_DEFAULT_PATH)
        else()
          # search first in known locations
          if (_${_PYTHON_PREFIX}_FIND_REGISTRY STREQUAL "FIRST")
            find_library (${_PYTHON_PREFIX}_LIBRARY_DEBUG
                          NAMES ${_${_PYTHON_PREFIX}_LIB_NAMES_DEBUG}
                          NAMES_PER_DIR
                          HINTS ${_${_PYTHON_PREFIX}_HINTS}
                          PATHS ${_${_PYTHON_PREFIX}_REGISTRY_PATHS}
                          PATH_SUFFIXES lib libs
                          NO_SYSTEM_ENVIRONMENT_PATH
                          NO_CMAKE_SYSTEM_PATH)
          endif()
          # search in all default paths
          find_library (${_PYTHON_PREFIX}_LIBRARY_DEBUG
                        NAMES ${_${_PYTHON_PREFIX}_LIB_NAMES_DEBUG}
                        NAMES_PER_DIR
                        HINTS ${_${_PYTHON_PREFIX}_HINTS}
                        PATHS ${__${_PYTHON_PREFIX}_REGISTRY_PATHS}
                        PATH_SUFFIXES lib libs)

          # extract version from library name
          if (${_PYTHON_PREFIX}_LIBRARY_DEBUG MATCHES "python([23])([0-9]+)")
            set (_${_PYTHON_PREFIX}_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}")
          elseif (${_PYTHON_PREFIX}_LIBRARY_DEBUG MATCHES "python([23])\\.([0-9]+)")
            set (_${_PYTHON_PREFIX}_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}")
          endif()
        endif()
      endif()
    else()
      foreach (_${_PYTHON_PREFIX}_LIB_VERSION IN LISTS _${_PYTHON_PREFIX}_FIND_VERSIONS)
        _python_get_lib_names (_${_PYTHON_PREFIX}_LIB_NAMES ${_${_PYTHON_PREFIX}_LIB_VERSION})
        _python_get_lib_names (_${_PYTHON_PREFIX}_LIB_NAMES_DEBUG ${_${_PYTHON_PREFIX}_LIB_VERSION} DEBUG)

        _python_get_frameworks (_${_PYTHON_PREFIX}_FRAMEWORK_PATHS ${_${_PYTHON_PREFIX}_LIB_VERSION})
        _python_get_registries (_${_PYTHON_PREFIX}_REGISTRY_PATHS ${_${_PYTHON_PREFIX}_LIB_VERSION})

        _python_get_path_suffixes (_${_PYTHON_PREFIX}_PATH_SUFFIXES ${_${_PYTHON_PREFIX}_LIB_VERSION} LIBRARY)

        if (APPLE AND _${_PYTHON_PREFIX}_FIND_FRAMEWORK STREQUAL "FIRST")
          find_library (${_PYTHON_PREFIX}_LIBRARY_RELEASE
                        NAMES ${_${_PYTHON_PREFIX}_LIB_NAMES}
                        NAMES_PER_DIR
                        HINTS ${_${_PYTHON_PREFIX}_HINTS}
                        PATHS ${_${_PYTHON_PREFIX}_FRAMEWORK_PATHS}
                        PATH_SUFFIXES ${_${_PYTHON_PREFIX}_PATH_SUFFIXES}
                        NO_CMAKE_PATH
                        NO_CMAKE_ENVIRONMENT_PATH
                        NO_SYSTEM_ENVIRONMENT_PATH
                        NO_CMAKE_SYSTEM_PATH)
        endif()

        if (WIN32 AND _${_PYTHON_PREFIX}_FIND_REGISTRY STREQUAL "FIRST")
          find_library (${_PYTHON_PREFIX}_LIBRARY_RELEASE
                        NAMES ${_${_PYTHON_PREFIX}_LIB_NAMES}
                        NAMES_PER_DIR
                        HINTS ${_${_PYTHON_PREFIX}_HINTS}
                        PATHS ${_${_PYTHON_PREFIX}_REGISTRY_PATHS}
                        PATH_SUFFIXES ${_${_PYTHON_PREFIX}_PATH_SUFFIXES}
                        NO_SYSTEM_ENVIRONMENT_PATH
                        NO_CMAKE_SYSTEM_PATH)
        endif()

        # search in HINTS locations
        find_library (${_PYTHON_PREFIX}_LIBRARY_RELEASE
                      NAMES ${_${_PYTHON_PREFIX}_LIB_NAMES}
                      NAMES_PER_DIR
                      HINTS ${_${_PYTHON_PREFIX}_HINTS}
                      PATH_SUFFIXES ${_${_PYTHON_PREFIX}_PATH_SUFFIXES}
                      NO_SYSTEM_ENVIRONMENT_PATH
                      NO_CMAKE_SYSTEM_PATH)

       if (APPLE AND _${_PYTHON_PREFIX}_FIND_FRAMEWORK STREQUAL "LAST")
         set (__${_PYTHON_PREFIX}_FRAMEWORK_PATHS ${_${_PYTHON_PREFIX}_FRAMEWORK_PATHS})
       else()
         unset (__${_PYTHON_PREFIX}_FRAMEWORK_PATHS)
       endif()

       if (WIN32 AND _${_PYTHON_PREFIX}_FIND_REGISTRY STREQUAL "LAST")
         set (__${_PYTHON_PREFIX}_REGISTRY_PATHS ${_${_PYTHON_PREFIX}_REGISTRY_PATHS})
       else()
         unset (__${_PYTHON_PREFIX}_REGISTRY_PATHS)
       endif()

       # search in all default paths
       find_library (${_PYTHON_PREFIX}_LIBRARY_RELEASE
                      NAMES ${_${_PYTHON_PREFIX}_LIB_NAMES}
                      NAMES_PER_DIR
                      PATHS ${__${_PYTHON_PREFIX}_FRAMEWORK_PATHS}
                            ${__${_PYTHON_PREFIX}_REGISTRY_PATHS}
                      PATH_SUFFIXES ${_${_PYTHON_PREFIX}_PATH_SUFFIXES})

        if (WIN32)
          # search for debug library
          if (${_PYTHON_PREFIX}_LIBRARY_RELEASE)
            # use library location as a hint
            get_filename_component (_${_PYTHON_PREFIX}_PATH "${${_PYTHON_PREFIX}_LIBRARY_RELEASE}" DIRECTORY)
            find_library (${_PYTHON_PREFIX}_LIBRARY_DEBUG
                          NAMES ${_${_PYTHON_PREFIX}_LIB_NAMES_DEBUG}
                          NAMES_PER_DIR
                          HINTS "${_${_PYTHON_PREFIX}_PATH}" ${_${_PYTHON_PREFIX}_HINTS}
                          NO_DEFAULT_PATH)
          else()
            # search first in known locations
            if (_${_PYTHON_PREFIX}_FIND_REGISTRY STREQUAL "FIRST")
              find_library (${_PYTHON_PREFIX}_LIBRARY_DEBUG
                            NAMES ${_${_PYTHON_PREFIX}_LIB_NAMES_DEBUG}
                            NAMES_PER_DIR
                            HINTS ${_${_PYTHON_PREFIX}_HINTS}
                            PATHS ${_${_PYTHON_PREFIX}_REGISTRY_PATHS}
                            PATH_SUFFIXES lib libs
                            NO_SYSTEM_ENVIRONMENT_PATH
                            NO_CMAKE_SYSTEM_PATH)
            endif()
            # search in all default paths
            find_library (${_PYTHON_PREFIX}_LIBRARY_DEBUG
                          NAMES ${_${_PYTHON_PREFIX}_LIB_NAMES_DEBUG}
                          NAMES_PER_DIR
                          HINTS ${_${_PYTHON_PREFIX}_HINTS}
                          PATHS ${__${_PYTHON_PREFIX}_REGISTRY_PATHS}
                          PATH_SUFFIXES lib libs)
          endif()
        endif()

        if (${_PYTHON_PREFIX}_LIBRARY_RELEASE OR ${_PYTHON_PREFIX}_LIBRARY_DEBUG)
          set (_${_PYTHON_PREFIX}_VERSION ${_${_PYTHON_PREFIX}_LIB_VERSION})
          break()
        endif()
      endforeach()
    endif()

    # retrieve runtime libraries
    if (${_PYTHON_PREFIX}_LIBRARY_RELEASE)
      _python_get_lib_names (_${_PYTHON_PREFIX}_LIB_NAMES ${_${_PYTHON_PREFIX}_VERSION})
      get_filename_component (_${_PYTHON_PREFIX}_PATH "${${_PYTHON_PREFIX}_LIBRARY_RELEASE}" DIRECTORY)
      get_filename_component (_${_PYTHON_PREFIX}_PATH2 "${_${_PYTHON_PREFIX}_PATH}" DIRECTORY)
      _python_find_runtime_library (${_PYTHON_PREFIX}_RUNTIME_LIBRARY_RELEASE
                                    NAMES ${_${_PYTHON_PREFIX}_LIB_NAMES}
                                    NAMES_PER_DIR
                                    HINTS "${_${_PYTHON_PREFIX}_PATH}" "${_${_PYTHON_PREFIX}_PATH2}" ${_${_PYTHON_PREFIX}_HINTS}
                                    PATH_SUFFIXES bin)
    endif()
    if (${_PYTHON_PREFIX}_LIBRARY_DEBUG)
      _python_get_lib_names (_${_PYTHON_PREFIX}_LIB_NAMES_DEBUG ${_${_PYTHON_PREFIX}_VERSION} DEBUG)
      get_filename_component (_${_PYTHON_PREFIX}_PATH "${${_PYTHON_PREFIX}_LIBRARY_DEBUG}" DIRECTORY)
      get_filename_component (_${_PYTHON_PREFIX}_PATH2 "${_${_PYTHON_PREFIX}_PATH}" DIRECTORY)
      _python_find_runtime_library (${_PYTHON_PREFIX}_RUNTIME_LIBRARY_DEBUG
                                    NAMES ${_${_PYTHON_PREFIX}_LIB_NAMES_DEBUG}
                                    NAMES_PER_DIR
                                    HINTS "${_${_PYTHON_PREFIX}_PATH}" "${_${_PYTHON_PREFIX}_PATH2}" ${_${_PYTHON_PREFIX}_HINTS}
                                    PATH_SUFFIXES bin)
    endif()

    # Don't search for include dir if no library was founded
    if (${_PYTHON_PREFIX}_LIBRARY_RELEASE OR ${_PYTHON_PREFIX}_LIBRARY_DEBUG)
      unset (_${_PYTHON_PREFIX}_INCLUDE_HINTS)

      if (${_PYTHON_PREFIX}_EXECUTABLE)
        # pick up include directory from configuration
        execute_process (COMMAND "${${_PYTHON_PREFIX}_EXECUTABLE}" -c
                                 "import sys; import sysconfig; sys.stdout.write(sysconfig.get_path('include'))"
                         RESULT_VARIABLE _${_PYTHON_PREFIX}_RESULT
                         OUTPUT_VARIABLE _${_PYTHON_PREFIX}_PATH
                         ERROR_QUIET
                         OUTPUT_STRIP_TRAILING_WHITESPACE)
        if (NOT _${_PYTHON_PREFIX}_RESULT)
          file (TO_CMAKE_PATH "${_${_PYTHON_PREFIX}_PATH}" _${_PYTHON_PREFIX}_PATH)
          list (APPEND _${_PYTHON_PREFIX}_INCLUDE_HINTS "${_${_PYTHON_PREFIX}_PATH}")
        endif()
      endif()

      foreach (_${_PYTHON_PREFIX}_LIB IN ITEMS ${_PYTHON_PREFIX}_LIBRARY_RELEASE ${_PYTHON_PREFIX}_LIBRARY_DEBUG)
        if (${_${_PYTHON_PREFIX}_LIB})
          # Use the library's install prefix as a hint
          if (${_${_PYTHON_PREFIX}_LIB} MATCHES "^(.+/Frameworks/Python.framework/Versions/[0-9.]+)")
            list (APPEND _${_PYTHON_PREFIX}_INCLUDE_HINTS "${CMAKE_MATCH_1}")
          elseif (${_${_PYTHON_PREFIX}_LIB} MATCHES "^(.+)/lib(64|32)?/python[0-9.]+/config")
            list (APPEND _${_PYTHON_PREFIX}_INCLUDE_HINTS "${CMAKE_MATCH_1}")
          elseif (DEFINED CMAKE_LIBRARY_ARCHITECTURE AND ${_${_PYTHON_PREFIX}_LIB} MATCHES "^(.+)/lib/${CMAKE_LIBRARY_ARCHITECTURE}")
            list (APPEND _${_PYTHON_PREFIX}_INCLUDE_HINTS "${CMAKE_MATCH_1}")
          else()
            # assume library is in a directory under root
            get_filename_component (_${_PYTHON_PREFIX}_PREFIX "${${_${_PYTHON_PREFIX}_LIB}}" DIRECTORY)
            get_filename_component (_${_PYTHON_PREFIX}_PREFIX "${_${_PYTHON_PREFIX}_PREFIX}" DIRECTORY)
            list (APPEND _${_PYTHON_PREFIX}_INCLUDE_HINTS "${_${_PYTHON_PREFIX}_PREFIX}")
          endif()
        endif()
      endforeach()
      list (REMOVE_DUPLICATES _${_PYTHON_PREFIX}_INCLUDE_HINTS)

      _python_get_frameworks (_${_PYTHON_PREFIX}_FRAMEWORK_PATHS ${_${_PYTHON_PREFIX}_VERSION})
      _python_get_registries (_${_PYTHON_PREFIX}_REGISTRY_PATHS ${_${_PYTHON_PREFIX}_VERSION})
      _python_get_path_suffixes (_${_PYTHON_PREFIX}_PATH_SUFFIXES ${_${_PYTHON_PREFIX}_VERSION} INCLUDE)

      if (APPLE AND _${_PYTHON_PREFIX}_FIND_FRAMEWORK STREQUAL "FIRST")
        find_path (${_PYTHON_PREFIX}_INCLUDE_DIR
                   NAMES Python.h
                   HINTS ${_${_PYTHON_PREFIX}_INCLUDE_HINTS} ${_${_PYTHON_PREFIX}_HINTS}
                   PATHS ${_${_PYTHON_PREFIX}_FRAMEWORK_PATHS}
                   PATH_SUFFIXES ${_${_PYTHON_PREFIX}_PATH_SUFFIXES}
                   NO_CMAKE_PATH
                   NO_CMAKE_ENVIRONMENT_PATH
                   NO_SYSTEM_ENVIRONMENT_PATH
                   NO_CMAKE_SYSTEM_PATH)
      endif()

      if (WIN32 AND _${_PYTHON_PREFIX}_FIND_REGISTRY STREQUAL "FIRST")
        find_path (${_PYTHON_PREFIX}_INCLUDE_DIR
                   NAMES Python.h
                   HINTS ${_${_PYTHON_PREFIX}_INCLUDE_HINTS} ${_${_PYTHON_PREFIX}_HINTS}
                   PATHS ${_${_PYTHON_PREFIX}_REGISTRY_PATHS}
                   PATH_SUFFIXES ${_${_PYTHON_PREFIX}_PATH_SUFFIXES}
                   NO_SYSTEM_ENVIRONMENT_PATH
                   NO_CMAKE_SYSTEM_PATH)
      endif()

      if (APPLE AND _${_PYTHON_PREFIX}_FIND_FRAMEWORK STREQUAL "LAST")
        set (__${_PYTHON_PREFIX}_FRAMEWORK_PATHS ${_${_PYTHON_PREFIX}_FRAMEWORK_PATHS})
      else()
        unset (__${_PYTHON_PREFIX}_FRAMEWORK_PATHS)
      endif()

      if (WIN32 AND _${_PYTHON_PREFIX}_FIND_REGISTRY STREQUAL "LAST")
        set (__${_PYTHON_PREFIX}_REGISTRY_PATHS ${_${_PYTHON_PREFIX}_REGISTRY_PATHS})
      else()
        unset (__${_PYTHON_PREFIX}_REGISTRY_PATHS)
      endif()

      find_path (${_PYTHON_PREFIX}_INCLUDE_DIR
                 NAMES Python.h
                 HINTS ${_${_PYTHON_PREFIX}_INCLUDE_HINTS} ${_${_PYTHON_PREFIX}_HINTS}
                 PATHS ${__${_PYTHON_PREFIX}_FRAMEWORK_PATHS}
                       ${__${_PYTHON_PREFIX}_REGISTRY_PATHS}
                 PATH_SUFFIXES ${_${_PYTHON_PREFIX}_PATH_SUFFIXES}
                 NO_SYSTEM_ENVIRONMENT_PATH
                 NO_CMAKE_SYSTEM_PATH)
    endif()

    # search header file in standard locations
    find_path (${_PYTHON_PREFIX}_INCLUDE_DIR
               NAMES Python.h)
  endif()

  if (${_PYTHON_PREFIX}_INCLUDE_DIR)
    # retrieve version from header file
    file (STRINGS "${${_PYTHON_PREFIX}_INCLUDE_DIR}/patchlevel.h" _${_PYTHON_PREFIX}_VERSION
          REGEX "^#define[ \t]+PY_VERSION[ \t]+\"[^\"]+\"")
    string (REGEX REPLACE "^#define[ \t]+PY_VERSION[ \t]+\"([^\"]+)\".*" "\\1"
                          _${_PYTHON_PREFIX}_VERSION "${_${_PYTHON_PREFIX}_VERSION}")
    string (REGEX MATCHALL "[0-9]+" _${_PYTHON_PREFIX}_VERSIONS "${_${_PYTHON_PREFIX}_VERSION}")
    list (GET _${_PYTHON_PREFIX}_VERSIONS 0 _${_PYTHON_PREFIX}_VERSION_MAJOR)
    list (GET _${_PYTHON_PREFIX}_VERSIONS 1 _${_PYTHON_PREFIX}_VERSION_MINOR)
    list (GET _${_PYTHON_PREFIX}_VERSIONS 2 _${_PYTHON_PREFIX}_VERSION_PATCH)

    if (NOT ${_PYTHON_PREFIX}_Interpreter_FOUND AND NOT ${_PYTHON_PREFIX}_Compiler_FOUND)
      # set public version information
      set (${_PYTHON_PREFIX}_VERSION ${_${_PYTHON_PREFIX}_VERSION})
      set (${_PYTHON_PREFIX}_VERSION_MAJOR ${_${_PYTHON_PREFIX}_VERSION_MAJOR})
      set (${_PYTHON_PREFIX}_VERSION_MINOR ${_${_PYTHON_PREFIX}_VERSION_MINOR})
      set (${_PYTHON_PREFIX}_VERSION_PATCH ${_${_PYTHON_PREFIX}_VERSION_PATCH})
    endif()
  endif()

  # define public variables
  include(SelectLibraryConfigurations)
  select_library_configurations (${_PYTHON_PREFIX})
  if (${_PYTHON_PREFIX}_RUNTIME_LIBRARY_RELEASE)
    set (${_PYTHON_PREFIX}_RUNTIME_LIBRARY "${${_PYTHON_PREFIX}_RUNTIME_LIBRARY_RELEASE}")
  elseif (${_PYTHON_PREFIX}_RUNTIME_LIBRARY_DEBUG)
    set (${_PYTHON_PREFIX}_RUNTIME_LIBRARY "${${_PYTHON_PREFIX}_RUNTIME_LIBRARY_DEBUG}")
  else()
    set (${_PYTHON_PREFIX}_RUNTIME_LIBRARY "$${_PYTHON_PREFIX}_RUNTIME_LIBRARY-NOTFOUND")
  endif()

  _python_set_library_dirs (${_PYTHON_PREFIX}_LIBRARY_DIRS
                            ${_PYTHON_PREFIX}_LIBRARY_RELEASE ${_PYTHON_PREFIX}_LIBRARY_DEBUG)
  if (UNIX)
    if (${_PYTHON_PREFIX}_LIBRARY_RELEASE MATCHES "${CMAKE_SHARED_LIBRARY_SUFFIX}$"
        OR ${_PYTHON_PREFIX}_LIBRARY_RELEASE MATCHES "${CMAKE_SHARED_LIBRARY_SUFFIX}$")
      set (${_PYTHON_PREFIX}_RUNTIME_LIBRARY_DIRS ${${_PYTHON_PREFIX}_LIBRARY_DIRS})
    endif()
  else()
      _python_set_library_dirs (${_PYTHON_PREFIX}_RUNTIME_LIBRARY_DIRS
                                ${_PYTHON_PREFIX}_RUNTIME_LIBRARY_RELEASE ${_PYTHON_PREFIX}_RUNTIME_LIBRARY_DEBUG)
  endif()

  set (${_PYTHON_PREFIX}_INCLUDE_DIRS "${${_PYTHON_PREFIX}_INCLUDE_DIR}")

  mark_as_advanced (${_PYTHON_PREFIX}_RUNTIME_LIBRARY_RELEASE
                    ${_PYTHON_PREFIX}_RUNTIME_LIBRARY_DEBUG
                    ${_PYTHON_PREFIX}_INCLUDE_DIR)

  if ((${_PYTHON_PREFIX}_LIBRARY_RELEASE OR ${_PYTHON_PREFIX}_LIBRARY_DEBUG)
      AND ${_PYTHON_PREFIX}_INCLUDE_DIR)
    if (${_PYTHON_PREFIX}_Interpreter_FOUND OR ${_PYTHON_PREFIX}_Compiler_FOUND)
      # development environment must be compatible with interpreter/compiler
      if (${_${_PYTHON_PREFIX}_VERSION_MAJOR}.${_${_PYTHON_PREFIX}_VERSION_MINOR} VERSION_EQUAL ${${_PYTHON_PREFIX}_VERSION_MAJOR}.${${_PYTHON_PREFIX}_VERSION_MINOR})
        set (${_PYTHON_PREFIX}_Development_FOUND TRUE)
      endif()
    elseif (${_PYTHON_PREFIX}_VERSION_MAJOR VERSION_EQUAL _${_PYTHON_PREFIX}_REQUIRED_VERSION_MAJOR)
      set (${_PYTHON_PREFIX}_Development_FOUND TRUE)
    endif()
  endif()

  # Restore the original find library ordering
  if (DEFINED _${_PYTHON_PREFIX}_CMAKE_FIND_LIBRARY_SUFFIXES)
    set (CMAKE_FIND_LIBRARY_SUFFIXES ${_${_PYTHON_PREFIX}_CMAKE_FIND_LIBRARY_SUFFIXES})
  endif()
endif()

if ("NumPy" IN_LIST ${_PYTHON_PREFIX}_FIND_COMPONENTS AND ${_PYTHON_PREFIX}_Interpreter_FOUND)
  list (APPEND _${_PYTHON_PREFIX}_CACHED_VARS ${_PYTHON_PREFIX}_NumPy_INCLUDE_DIR)
  if (${_PYTHON_PREFIX}_FIND_REQUIRED_NumPy)
    list (APPEND _${_PYTHON_PREFIX}_REQUIRED_VARS ${_PYTHON_PREFIX}_NumPy_INCLUDE_DIR)
  endif()
  execute_process(
      COMMAND "${${_PYTHON_PREFIX}_EXECUTABLE}" -c
              "from __future__ import print_function\ntry: import numpy; print(numpy.get_include(), end='')\nexcept:pass\n"
      RESULT_VARIABLE _${_PYTHON_PREFIX}_RESULT
      OUTPUT_VARIABLE _${_PYTHON_PREFIX}_NumPy_PATH
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (NOT _${_PYTHON_PREFIX}_RESULT)
    find_path(${_PYTHON_PREFIX}_NumPy_INCLUDE_DIR
              NAMES "numpy/arrayobject.h" "numpy/numpyconfig.h"
              HINTS "${_${_PYTHON_PREFIX}_NumPy_PATH}"
              NO_DEFAULT_PATH)
  endif()
  if(${_PYTHON_PREFIX}_NumPy_INCLUDE_DIR)
    set(${_PYTHON_PREFIX}_NumPy_INCLUDE_DIRS "${${_PYTHON_PREFIX}_NumPy_INCLUDE_DIR}")
    set(${_PYTHON_PREFIX}_NumPy_FOUND TRUE)
  endif()
  if(${_PYTHON_PREFIX}_NumPy_FOUND)
    execute_process(
            COMMAND "${${_PYTHON_PREFIX}_EXECUTABLE}" -c
            "from __future__ import print_function\ntry: import numpy; print(numpy.__version__, end='')\nexcept:pass\n"
            RESULT_VARIABLE _${_PYTHON_PREFIX}_RESULT
            OUTPUT_VARIABLE _${_PYTHON_PREFIX}_NumPy_VERSION)
    if (NOT _${_PYTHON_PREFIX}_RESULT)
       set(${_PYTHON_PREFIX}_NumPy_VERSION "${_${_PYTHON_PREFIX}_NumPy_VERSION}")
    endif()
  endif()
  # final step: set NumPy founded only if Development component is founded as well
  if (NOT ${_PYTHON_PREFIX}_Development_FOUND)
    set(${_PYTHON_PREFIX}_NumPy_FOUND FALSE)
  endif()
endif()

# final validation
if (${_PYTHON_PREFIX}_VERSION_MAJOR AND
    NOT ${_PYTHON_PREFIX}_VERSION_MAJOR VERSION_EQUAL _${_PYTHON_PREFIX}_REQUIRED_VERSION_MAJOR)
  _python_display_failure ("Could NOT find ${_PYTHON_PREFIX}: Found unsuitable major version \"${${_PYTHON_PREFIX}_VERSION_MAJOR}\", but required major version is exact version \"${_${_PYTHON_PREFIX}_REQUIRED_VERSION_MAJOR}\"")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args (${_PYTHON_PREFIX}
                                   REQUIRED_VARS ${_${_PYTHON_PREFIX}_REQUIRED_VARS}
                                   VERSION_VAR ${_PYTHON_PREFIX}_VERSION
                                   HANDLE_COMPONENTS)

# Create imported targets and helper functions
if(_${_PYTHON_PREFIX}_CMAKE_ROLE STREQUAL "PROJECT")
  if ("Interpreter" IN_LIST ${_PYTHON_PREFIX}_FIND_COMPONENTS
      AND ${_PYTHON_PREFIX}_Interpreter_FOUND
      AND NOT TARGET ${_PYTHON_PREFIX}::Interpreter)
    add_executable (${_PYTHON_PREFIX}::Interpreter IMPORTED)
    set_property (TARGET ${_PYTHON_PREFIX}::Interpreter
                  PROPERTY IMPORTED_LOCATION "${${_PYTHON_PREFIX}_EXECUTABLE}")
  endif()

  if ("Compiler" IN_LIST ${_PYTHON_PREFIX}_FIND_COMPONENTS
      AND ${_PYTHON_PREFIX}_Compiler_FOUND
      AND NOT TARGET ${_PYTHON_PREFIX}::Compiler)
    add_executable (${_PYTHON_PREFIX}::Compiler IMPORTED)
    set_property (TARGET ${_PYTHON_PREFIX}::Compiler
                  PROPERTY IMPORTED_LOCATION "${${_PYTHON_PREFIX}_COMPILER}")
  endif()

  if ("Development" IN_LIST ${_PYTHON_PREFIX}_FIND_COMPONENTS
      AND ${_PYTHON_PREFIX}_Development_FOUND)

    macro (__PYTHON_IMPORT_LIBRARY __name)
      if (${_PYTHON_PREFIX}_LIBRARY_RELEASE MATCHES "${CMAKE_SHARED_LIBRARY_SUFFIX}$"
          OR ${_PYTHON_PREFIX}_LIBRARY_DEBUG MATCHES "${CMAKE_SHARED_LIBRARY_SUFFIX}$"
          OR ${_PYTHON_PREFIX}_RUNTIME_LIBRARY_RELEASE OR ${_PYTHON_PREFIX}_RUNTIME_LIBRARY_DEBUG)
        set (_${_PYTHON_PREFIX}_LIBRARY_TYPE SHARED)
      else()
        set (_${_PYTHON_PREFIX}_LIBRARY_TYPE STATIC)
      endif()

      add_library (${__name} ${_${_PYTHON_PREFIX}_LIBRARY_TYPE} IMPORTED)

      set_property (TARGET ${__name}
                    PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${${_PYTHON_PREFIX}_INCLUDE_DIR}")

      if ((${_PYTHON_PREFIX}_LIBRARY_RELEASE AND ${_PYTHON_PREFIX}_RUNTIME_LIBRARY_RELEASE)
          OR (${_PYTHON_PREFIX}_LIBRARY_DEBUG AND ${_PYTHON_PREFIX}_RUNTIME_LIBRARY_DEBUG))
        # System manage shared libraries in two parts: import and runtime
        if (${_PYTHON_PREFIX}_LIBRARY_RELEASE AND ${_PYTHON_PREFIX}_LIBRARY_DEBUG)
          set_property (TARGET ${__name} PROPERTY IMPORTED_CONFIGURATIONS RELEASE DEBUG)
          set_target_properties (${__name}
                                 PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
                                            IMPORTED_IMPLIB_RELEASE "${${_PYTHON_PREFIX}_LIBRARY_RELEASE}"
                                            IMPORTED_LOCATION_RELEASE "${${_PYTHON_PREFIX}_RUNTIME_LIBRARY_RELEASE}")
          set_target_properties (${__name}
                                 PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
                                            IMPORTED_IMPLIB_DEBUG "${${_PYTHON_PREFIX}_LIBRARY_DEBUG}"
                                            IMPORTED_LOCATION_DEBUG "${${_PYTHON_PREFIX}_RUNTIME_LIBRARY_DEBUG}")
        else()
          set_target_properties (${__name}
                                 PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                                            IMPORTED_IMPLIB "${${_PYTHON_PREFIX}_LIBRARY}"
                                            IMPORTED_LOCATION "${${_PYTHON_PREFIX}_RUNTIME_LIBRARY}")
        endif()
      else()
        if (${_PYTHON_PREFIX}_LIBRARY_RELEASE AND ${_PYTHON_PREFIX}_LIBRARY_DEBUG)
          set_property (TARGET ${_PYTHON_PREFIX}::Python PROPERTY IMPORTED_CONFIGURATIONS RELEASE DEBUG)
          set_target_properties (${__name}
                                 PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
                                            IMPORTED_LOCATION_RELEASE "${${_PYTHON_PREFIX}_LIBRARY_RELEASE}")
          set_target_properties (${__name}
                                 PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
                                            IMPORTED_LOCATION_DEBUG "${${_PYTHON_PREFIX}_LIBRARY_DEBUG}")
        else()
          set_target_properties (${__name}
                                 PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                                            IMPORTED_LOCATION "${${_PYTHON_PREFIX}_LIBRARY}")
        endif()
      endif()

      if (_${_PYTHON_PREFIX}_CONFIG AND _${_PYTHON_PREFIX}_LIBRARY_TYPE STREQUAL "STATIC")
        # extend link information with dependent libraries
        execute_process (COMMAND "${_${_PYTHON_PREFIX}_CONFIG}" --ldflags
                         RESULT_VARIABLE _${_PYTHON_PREFIX}_RESULT
                         OUTPUT_VARIABLE _${_PYTHON_PREFIX}_FLAGS
                         ERROR_QUIET
                         OUTPUT_STRIP_TRAILING_WHITESPACE)
        if (NOT _${_PYTHON_PREFIX}_RESULT)
          string (REGEX MATCHALL "-[Ll][^ ]+" _${_PYTHON_PREFIX}_LINK_LIBRARIES "${_${_PYTHON_PREFIX}_FLAGS}")
          # remove elements relative to python library itself
          list (FILTER _${_PYTHON_PREFIX}_LINK_LIBRARIES EXCLUDE REGEX "-lpython")
          foreach (_${_PYTHON_PREFIX}_DIR IN LISTS ${_PYTHON_PREFIX}_LIBRARY_DIRS)
            list (FILTER _${_PYTHON_PREFIX}_LINK_LIBRARIES EXCLUDE REGEX "-L${${_PYTHON_PREFIX}_DIR}")
          endforeach()
          set_property (TARGET ${__name}
                        PROPERTY INTERFACE_LINK_LIBRARIES ${_${_PYTHON_PREFIX}_LINK_LIBRARIES})
        endif()
      endif()
    endmacro()

    if (NOT TARGET ${_PYTHON_PREFIX}::Python)
      __python_import_library (${_PYTHON_PREFIX}::Python)
    endif()

    if (NOT TARGET ${_PYTHON_PREFIX}::Module)
      if (CMAKE_SYSTEM_NAME MATCHES "^(Windows.*|CYGWIN|MSYS)$")
        # On Windows/CYGWIN/MSYS, Python::Module is the same as Python::Python
        # but ALIAS cannot be used because the imported library is not GLOBAL.
        __python_import_library (${_PYTHON_PREFIX}::Module)
      else()
        add_library (${_PYTHON_PREFIX}::Module INTERFACE IMPORTED)
        set_property (TARGET ${_PYTHON_PREFIX}::Module
                      PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${${_PYTHON_PREFIX}_INCLUDE_DIR}")

        # When available, enforce shared library generation with undefined symbols
        if (APPLE)
          set_property (TARGET ${_PYTHON_PREFIX}::Module
                        PROPERTY INTERFACE_LINK_OPTIONS "LINKER:-undefined,dynamic_lookup")
        endif()
        if (CMAKE_SYSTEM_NAME STREQUAL "SunOS")
          set_property (TARGET ${_PYTHON_PREFIX}::Module
                        PROPERTY INTERFACE_LINK_OPTIONS "LINKER:-z,nodefs")
         endif()
        if (CMAKE_SYSTEM_NAME STREQUAL "AIX")
          set_property (TARGET ${_PYTHON_PREFIX}::Module
                        PROPERTY INTERFACE_LINK_OPTIONS "LINKER:-b,erok")
         endif()
      endif()
    endif()

    #
    # PYTHON_ADD_LIBRARY (<name> [STATIC|SHARED|MODULE] src1 src2 ... srcN)
    # It is used to build modules for python.
    #
    function (__${_PYTHON_PREFIX}_ADD_LIBRARY prefix name)
      cmake_parse_arguments (PARSE_ARGV 2 PYTHON_ADD_LIBRARY
        "STATIC;SHARED;MODULE" "" "")

      unset (type)
      if (NOT (PYTHON_ADD_LIBRARY_STATIC
            OR PYTHON_ADD_LIBRARY_SHARED
            OR PYTHON_ADD_LIBRARY_MODULE))
        set (type MODULE)
      endif()
      add_library (${name} ${type} ${ARGN})

      get_property (type TARGET ${name} PROPERTY TYPE)

      if (type STREQUAL "MODULE_LIBRARY")
        target_link_libraries (${name} PRIVATE ${prefix}::Module)
        # customize library name to follow module name rules
        set_property (TARGET ${name} PROPERTY PREFIX "")
        if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
          set_property (TARGET ${name} PROPERTY SUFFIX ".pyd")
        endif()
      else()
        target_link_libraries (${name} PRIVATE ${prefix}::Python)
      endif()
    endfunction()
  endif()

  if ("NumPy" IN_LIST ${_PYTHON_PREFIX}_FIND_COMPONENTS AND ${_PYTHON_PREFIX}_NumPy_FOUND
      AND NOT TARGET ${_PYTHON_PREFIX}::NumPy AND TARGET ${_PYTHON_PREFIX}::Module)
    add_library (${_PYTHON_PREFIX}::NumPy INTERFACE IMPORTED)
    set_property (TARGET ${_PYTHON_PREFIX}::NumPy
                  PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${${_PYTHON_PREFIX}_NumPy_INCLUDE_DIR}")
    target_link_libraries (${_PYTHON_PREFIX}::NumPy INTERFACE ${_PYTHON_PREFIX}::Module)
  endif()
endif()

# final clean-up

# Restore CMAKE_FIND_APPBUNDLE
if (DEFINED _${_PYTHON_PREFIX}_CMAKE_FIND_APPBUNDLE)
  set (CMAKE_FIND_APPBUNDLE ${_${_PYTHON_PREFIX}_CMAKE_FIND_APPBUNDLE})
  unset (_${_PYTHON_PREFIX}_CMAKE_FIND_APPBUNDLE)
else()
  unset (CMAKE_FIND_APPBUNDLE)
endif()
# Restore CMAKE_FIND_FRAMEWORK
if (DEFINED _${_PYTHON_PREFIX}_CMAKE_FIND_FRAMEWORK)
  set (CMAKE_FIND_FRAMEWORK ${_${_PYTHON_PREFIX}_CMAKE_FIND_FRAMEWORK})
  unset (_${_PYTHON_PREFIX}_CMAKE_FIND_FRAMEWORK)
else()
  unset (CMAKE_FIND_FRAMEWORK)
endif()

unset (_${_PYTHON_PREFIX}_CONFIG CACHE)
