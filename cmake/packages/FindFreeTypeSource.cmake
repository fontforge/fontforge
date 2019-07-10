# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
FindFreeTypeSource
------------------

Find the location to the FreeType source.

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``FreeTypeSource::FreeTypeSource``
  The ``FreeType`` source library, if found

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``FreeTypeSource_FOUND``
  true if the FreeTypeSource headers and libraries were found
``FreeTypeSource_INCLUDE_DIRS``
  directories containing the FreeTypeSource headers.
``FreeTypeSource_LIBRARIES``
  the library to link against
``FreeTypeSource_VERSION``
  the version of FreeTypeSource found

#]=======================================================================]

find_path(FreeTypeSource_INCLUDE_DIR
  NAMES ft2build.h
  HINTS ${FreeTypeSource_ROOT}
        ${FreeTypeSource_HINT}
  PATH_SUFFIXES include
)

find_path(FreeTypeSource_FREETYPE_INCLUDE_DIR
  NAMES freetype.h
  HINTS ${FreeTypeSource_ROOT}
        ${FreeTypeSource_HINT}
  PATH_SUFFIXES include/freetype
)

find_path(FreeTypeSource_TRUETYPE_INCLUDE_DIR
  NAMES tterrors.h
  HINTS ${FreeTypeSource_ROOT}
        ${FreeTypeSource_HINT}
  PATH_SUFFIXES src/truetype
)

if(FreeTypeSource_INCLUDE_DIR AND FreeTypeSource_FREETYPE_INCLUDE_DIR AND FreeTypeSource_TRUETYPE_INCLUDE_DIR)
  set (FreeTypeSource_INCLUDE_DIRS
       ${FreeTypeSource_INCLUDE_DIR}
       ${FreeTypeSource_FREETYPE_INCLUDE_DIR}
       ${FreeTypeSource_TRUETYPE_INCLUDE_DIR}
       CACHE INTERNAL "FreeType source include directories")

  if(EXISTS "${FreeTypeSource_FREETYPE_INCLUDE_DIR}/freetype.h")
    set(FREETYPE_H "${FreeTypeSource_FREETYPE_INCLUDE_DIR}/freetype.h")
  elseif(EXISTS "${FreeTypeSource_INCLUDE_DIR}/freetype.h")
    set(FREETYPE_H "${FreeTypeSource_INCLUDE_DIR}/freetype.h")
  endif()

  if(FREETYPE_H)
    file(STRINGS "${FREETYPE_H}" freetype_version_str
         REGEX "^#[\t ]*define[\t ]+FREETYPE_(MAJOR|MINOR|PATCH)[\t ]+[0-9]+$")

    unset(FreeTypeSource_VERSION)
    foreach(VPART MAJOR MINOR PATCH)
      foreach(VLINE ${freetype_version_str})
        if(VLINE MATCHES "^#[\t ]*define[\t ]+FREETYPE_${VPART}[\t ]+([0-9]+)$")
          set(FreeTypeSource_VERSION_PART "${CMAKE_MATCH_1}")
          if(FreeTypeSource_VERSION)
            string(APPEND FreeTypeSource_VERSION ".${FreeTypeSource_VERSION_PART}")
          else ()
            set(FreeTypeSource_VERSION "${FreeTypeSource_VERSION_PART}")
          endif()
          unset(FreeTypeSource_VERSION_PART)
        endif()
      endforeach()
    endforeach()
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  FreeTypeSource
  REQUIRED_VARS
    FreeTypeSource_INCLUDE_DIRS
  VERSION_VAR
    FreeTypeSource_VERSION
)

mark_as_advanced(FreeTypeSource_INCLUDE_DIRS)

if(FreeTypeSource_FOUND AND NOT TARGET FreeTypeSource::FreeTypeSource)
  add_library(FreeTypeSource::FreeTypeSource INTERFACE IMPORTED)
  set_property(TARGET FreeTypeSource::FreeTypeSource PROPERTY
               INTERFACE_INCLUDE_DIRECTORIES "${FreeTypeSource_INCLUDE_DIRS}")
endif()
