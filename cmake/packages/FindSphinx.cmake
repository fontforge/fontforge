# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
FindSphinx
----------

Finds Sphinx.

Options
^^^^^^^

``SPHINX_DIR``
  Set to a directory to search for sphinx-build.
``SPHINX_USE_VIRTUALENV``
  If set to true and Sphinx is not found, will try to install it into
  a virtualenv.

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``Sphinx_FOUND``
  true if a usable Sphinx installation has been found
``Sphinx_BUILD_BINARY``
  path to the sphinx-build binary
``Sphinx_VERSION``
  the version of sphinx being used

#]=======================================================================]

function(_find_sphinx)
  find_program(Sphinx_BUILD_BINARY NAMES sphinx-build
    HINTS
      $ENV{SPHINX_DIR}
      ${CMAKE_BINARY_DIR}/sphinx-venv/Scripts
      ${CMAKE_BINARY_DIR}/sphinx-venv/bin
    PATH_SUFFIXES bin
    DOC "Sphinx documentation generator"
  )
endfunction()

function(_sphinx_from_venv)
  find_package(Python3 3.3 COMPONENTS Interpreter)
  if(NOT Python3_Interpreter_FOUND)
    message(STATUS "Python3 not found, skipping")
    return()
  endif()
  execute_process(COMMAND "${Python3_EXECUTABLE}" -m venv "${CMAKE_BINARY_DIR}/sphinx-venv")

  find_program(_venv_bin NAMES python
    NO_DEFAULT_PATH
    HINTS
      "${CMAKE_BINARY_DIR}/sphinx-venv/Scripts"
      "${CMAKE_BINARY_DIR}/sphinx-venv/bin"
  )
  if(NOT _venv_bin)
    message(WARNING "could not make venv")
    return()
  endif()

  execute_process(COMMAND "${_venv_bin}" -m pip install sphinx typing RESULT_VARIABLE _pip_result)
  if(_pip_result)
    message(WARNING "could not install sphinx")
    return()
  endif()

  _find_sphinx()
endfunction()


_find_sphinx()

if(NOT Sphinx_BUILD_BINARY AND SPHINX_USE_VENV AND NOT EXISTS "${CMAKE_BINARY_DIR}/sphinx-venv")
  message(STATUS "sphinx-build not found, attempting to install it into a venv...")
  _sphinx_from_venv()
endif()

if(Sphinx_BUILD_BINARY)
    execute_process(COMMAND "${Sphinx_BUILD_BINARY}" --version
      OUTPUT_VARIABLE Sphinx_VERSION_STRING
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX REPLACE "^.* ([0-9\.]+)$" "\\1" Sphinx_VERSION "${Sphinx_VERSION_STRING}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Sphinx
  REQUIRED_VARS
    Sphinx_BUILD_BINARY
  VERSION_VAR
    Sphinx_VERSION
)
