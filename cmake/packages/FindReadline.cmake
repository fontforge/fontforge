# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
FindReadline
------------

Find the Readline library.

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``Readline::Readline``
  The Readline ``readline`` library, if found

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``Readline_FOUND``
  true if the Readline headers and libraries were found
``Readline_INCLUDE_DIRS``
  directories containing the Readline headers.
``Readline_LIBRARIES``
  the library to link against
``Readline_VERSION``
  the version of Readline found

#]=======================================================================]

find_path(Readline_INCLUDE_DIRS
  NAMES readline.h
  HINTS ${Readline_ROOT_DIR}
  PATH_SUFFIXES readline
)

find_library(Readline_LIBRARIES
  NAMES readline termcap
  HINTS ${Readline_ROOT_DIR}
)

if(Readline_INCLUDE_DIRS)
  if(EXISTS "${Readline_INCLUDE_DIRS}/readline.h")
    file(READ "${Readline_INCLUDE_DIRS}/readline.h" READLINE_H_CONTENTS)
    string(REGEX MATCH "#define RL_VERSION_MAJOR[\t ]+([0-9]+)" _dummy "${READLINE_H_CONTENTS}")
    set(Readline_VERSION_MAJOR "${CMAKE_MATCH_1}")
    string(REGEX MATCH "#define RL_VERSION_MINOR[\t ]+([0-9]+)" _dummy "${READLINE_H_CONTENTS}")
    set(Readline_VERSION_MINOR "${CMAKE_MATCH_1}")
    set(Readline_VERSION "${Readline_VERSION_MAJOR}.${Readline_VERSION_MINOR}")
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Readline
  REQUIRED_VARS
    Readline_LIBRARIES
    Readline_INCLUDE_DIRS
  VERSION_VAR
    Readline_VERSION
)

mark_as_advanced(
  Readline_INCLUDE_DIRS
  Readline_LIBRARIES
)

if(Readline_FOUND AND NOT TARGET Readline::Readline)
  add_library(Readline::Readline INTERFACE IMPORTED)
  set_property(TARGET Readline::Readline PROPERTY
               INTERFACE_INCLUDE_DIRECTORIES "${Readline_INCLUDE_DIRS}")
  set_property(TARGET Readline::Readline PROPERTY
               INTERFACE_LINK_LIBRARIES "${Readline_LIBRARIES}")
endif()
