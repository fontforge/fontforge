# Copyright (C) 2012 Raphael Kubo da Costa <rakuco@webkit.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND ITS CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ITS
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#[=======================================================================[.rst:
FindCairo
---------

Find the Cairo library.

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``Cairo::Cairo``
  The Cairo ``cairo`` library, if found

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``Cairo_FOUND``
  true if the Cairo headers and libraries were found
``Cairo_INCLUDE_DIRS``
  directories containing the Cairo headers.
``Cairo_LIBRARIES``
  the library to link against
``Cairo_VERSION``
  the version of Cairo found

#]=======================================================================]

find_package(PkgConfig)
pkg_check_modules(PC_CAIRO QUIET cairo)

find_path(Cairo_INCLUDE_DIRS
  NAMES cairo.h
  HINTS ${PC_CAIRO_INCLUDEDIR}
        ${PC_CAIRO_INCLUDE_DIRS}
  PATH_SUFFIXES cairo
)

find_library(Cairo_LIBRARIES
  NAMES cairo
  HINTS ${PC_CAIRO_LIBDIR}
        ${PC_CAIRO_LIBRARY_DIRS}
)

if(Cairo_INCLUDE_DIRS)
  if(EXISTS "${Cairo_INCLUDE_DIRS}/cairo-version.h")
    FILE(READ "${Cairo_INCLUDE_DIRS}/cairo-version.h" CAIRO_VERSION_CONTENT)

    STRING(REGEX MATCH "#define +CAIRO_VERSION_MAJOR +([0-9]+)" _dummy "${CAIRO_VERSION_CONTENT}")
    SET(Cairo_VERSION_MAJOR "${CMAKE_MATCH_1}")

    STRING(REGEX MATCH "#define +CAIRO_VERSION_MINOR +([0-9]+)" _dummy "${CAIRO_VERSION_CONTENT}")
    SET(Cairo_VERSION_MINOR "${CMAKE_MATCH_1}")

    STRING(REGEX MATCH "#define +CAIRO_VERSION_MICRO +([0-9]+)" _dummy "${CAIRO_VERSION_CONTENT}")
    SET(Cairo_VERSION_MICRO "${CMAKE_MATCH_1}")

    SET(Cairo_VERSION "${Cairo_VERSION_MAJOR}.${Cairo_VERSION_MINOR}.${Cairo_VERSION_MICRO}")
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Cairo REQUIRED_VARS Cairo_INCLUDE_DIRS Cairo_LIBRARIES
                                        VERSION_VAR Cairo_VERSION)

mark_as_advanced(
  Cairo_INCLUDE_DIRS
  Cairo_LIBRARIES
)

if(Cairo_FOUND AND NOT TARGET Cairo::Cairo)
  add_library(Cairo::Cairo INTERFACE IMPORTED)
  set_property(TARGET Cairo::Cairo PROPERTY
               INTERFACE_INCLUDE_DIRECTORIES "${Cairo_INCLUDE_DIRS}")
  set_property(TARGET Cairo::Cairo PROPERTY
               INTERFACE_LINK_LIBRARIES "${Cairo_LIBRARIES}")
endif()
