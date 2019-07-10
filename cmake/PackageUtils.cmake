# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
PackageUtils
------------

``find_package_with_target` calls ``find_package`` with the provided
arguments. If searching for package ``PACKAGE``, then if ``PACKAGE``
is found but the imported target ``PACKAGE::PACKAGE`` does not exist,
a basic imported target of that name is created, on the assumption
that ``PACKAGE_INCLUDE_DIRS`` and ``PACKAGE_LIBRARIES`` are correctly
set. This is for use with find targets that do not natively provide
an imported target.

``find_package_auto`` calls ``find_package`` with the provided
arguments, iff ``auto_option`` evaluates to a truthy value or ``AUTO``.
If ``auto_option`` is truthy (but not ``AUTO``), ``REQUIRED`` is
passed into ``find_package``. ``auto_option_RESULT`` is set to
the ``find_package`` result.

#]=======================================================================]

macro(find_package_with_target)
  find_package(${ARGV})
  if(${ARGV0}_FOUND AND NOT TARGET ${ARGV0}::${ARGV0})
    add_library(${ARGV0}::${ARGV0} INTERFACE IMPORTED)
    set_property(TARGET ${ARGV0}::${ARGV0} PROPERTY
                INTERFACE_INCLUDE_DIRECTORIES "${${ARGV0}_INCLUDE_DIRS}")
    set_property(TARGET ${ARGV0}::${ARGV0} PROPERTY
                INTERFACE_LINK_LIBRARIES "${${ARGV0}_LIBRARIES}")
  endif()
endmacro()

macro(find_package_auto auto_option)
  if(${auto_option})
    unset(_find_package_auto_required)
    if(NOT ${${auto_option}} STREQUAL "AUTO")
      set(_find_package_auto_required REQUIRED)
    endif()

    find_package(${ARGN} ${_find_package_auto_required})
    set(${auto_option}_RESULT ${${ARGV1}_FOUND})
  endif()
endmacro()
