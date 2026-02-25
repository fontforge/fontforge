# Distributed under the original FontForge BSD 3-clause license

cmake_minimum_required(VERSION 3.5)

# Update the version for each new release
project(fontforge VERSION 20251009 LANGUAGES C CXX)

# Detect if we're being built by scikit-build-core for wheel packaging
# SKBUILD is set automatically by scikit-build-core
if(DEFINED SKBUILD)
  set(BUILDING_WHEEL TRUE)
  message(STATUS "Building for Python wheel (detected scikit-build-core)")
else()
  set(BUILDING_WHEEL FALSE)
endif()

# Generate Python-compatible CalVer version from YYYYMMDD format
# 20251009 -> 2025.10.9
string(SUBSTRING "${PROJECT_VERSION}" 0 4 _FF_YEAR)
string(SUBSTRING "${PROJECT_VERSION}" 4 2 _FF_MONTH)
string(SUBSTRING "${PROJECT_VERSION}" 6 2 _FF_DAY)
math(EXPR _FF_MONTH_INT "${_FF_MONTH}")
math(EXPR _FF_DAY_INT "${_FF_DAY}")
set(FONTFORGE_PYTHON_VERSION "${_FF_YEAR}.${_FF_MONTH_INT}.${_FF_DAY_INT}")
if(BUILDING_WHEEL)
  message(STATUS "Python wheel version: ${FONTFORGE_PYTHON_VERSION}")
endif()

# No in source builds
if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are disallowed. Create a build folder to run CMake from.")
endif()

# Add folder for custom cmake modules
list(INSERT CMAKE_MODULE_PATH 0 ${PROJECT_SOURCE_DIR}/cmake ${PROJECT_SOURCE_DIR}/cmake/packages)
if(${CMAKE_VERSION} VERSION_LESS "3.15.7")
  # This could be more targeted, but keep it simple
  list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/backports/3.15.7)
endif()

# Include any required modules
include(BuildUtils)
include(CheckIncludeFile)
include(CPackSetup)
include(CTest)
include(GNUInstallDirs)
include(PackageUtils)
include(TargetUtils)

# Set any global defines
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_THREAD_PREFER_PTHREAD 1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

set_default_build_type(RelWithDebInfo) # Sets CMAKE_BUILD_TYPE
set_default_rpath()
add_uninstall_target()

# Options
build_option(BUILD_SHARED_LIBS       BOOL ON  "Build libfontforge as a shared library")
build_option(ENABLE_GUI              BOOL ON  "Build FontForge with GUI support")

build_option(ENABLE_NATIVE_SCRIPTING BOOL ON  "Enables FontForge's native scripting support")
build_option(ENABLE_PYTHON_SCRIPTING BOOL ON  "Enables FontForge's Python scripting support")
build_option(ENABLE_PYTHON_EXTENSION AUTO ON  "Builds the Python models for use with system python")

build_option(ENABLE_LIBSPIRO         BOOL ON  "Enables libspiro support")
build_option(ENABLE_LIBGIF           AUTO ON  "Enables GIF support")
build_option(ENABLE_LIBJPEG          AUTO ON  "Enables JPEG support")
build_option(ENABLE_LIBPNG           AUTO ON  "Enables PNG support")
build_option(ENABLE_LIBREADLINE      AUTO ON  "Enables Readline support")
build_option(ENABLE_LIBTIFF          AUTO ON  "Enables TIFF support")
build_option(ENABLE_WOFF2            AUTO ON  "Enables WOFF2 support")
build_option(ENABLE_LIBUNIBREAK      AUTO ON  "Enables libunibreak for Unicode line breaking")
build_option(ENABLE_HARFBUZZ         AUTO ON  "Enables HarfBuzz shaper support")

build_option(ENABLE_DOCS             AUTO ON  "Enables building and installing documentation. Sphinx is required to build it.")
build_option(ENABLE_CODE_COVERAGE    BOOL OFF "Build with code coverage support")
build_option(ENABLE_DEBUG_RAW_POINTS BOOL OFF "Add a raw mode to the points window of the debugger")
build_option(ENABLE_FONTFORGE_EXTRAS BOOL OFF "Builds programs from the contrib directory")
build_option(ENABLE_MAINTAINER_TOOLS BOOL OFF "Build programs normally only used by FontForge maintainers and developers")
build_option(ENABLE_TILE_PATH        BOOL OFF "Enable a 'tile path' command (a variant of 'expand stroke')")
build_option(ENABLE_WRITE_PFM        BOOL OFF "Add the ability to save a PFM file without creating the associated font file")
build_option(ENABLE_SANITIZER        ENUM "none" "Enables a sanitizer. Requires support from the compiler."
                                          "none" "address" "leak" "thread" "undefined" "memory")

build_option(ENABLE_FREETYPE_DEBUGGER PATH "" "Use FreeType's internal debugger within FontForge.\
 If enabling this option, this must be set to the path of the FreeType source.\
 The version of that source must match the version that FontForge will link to.")

build_option(SPHINX_USE_VENV BOOL OFF "If building documentation and Sphinx\
 is not installed, try to install and use it from a python3 venv.")

build_option(REAL_TYPE ENUM "double" "Sets the floating point type used." "double" "float")
build_option(GUI_THEME     ENUM "tango"  "Sets the GUI theme." "tango" "2012")
add_definitions( -DGUI_THEME="${GUI_THEME}" )

build_option(FONT_FAMILIES_SERIF     STRING "default"  "Serif font family names to use in the UI separated by commas")
build_option(FONT_FAMILIES_UI        STRING "default"  "Font family names to use in the UI separated by commas (typically sans-serif)")
build_option(FONT_FAMILIES_LABEL     STRING "default"  "Font family names for labeling unicode characters (very wide unicode coverage, e.g. unifont, recommended)")
build_option(FONT_FAMILIES_MONO      STRING "default"  "Monospace font family names to use in the UI separated by commas")

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  list(APPEND _test_flags CFLAGS -Wall -Werror=implicit-function-declaration -Werror=int-conversion -Wno-misleading-indentation)
  if(CMAKE_GENERATOR STREQUAL "Ninja") # https://github.com/ninja-build/ninja/wiki/FAQ
    list(APPEND _test_flags BOTH -fdiagnostics-color=always)
  endif()
  set_supported_compiler_flags(FONTFORGE_DEFAULT_CFLAGS FONTFORGE_DEFAULT_CXXFLAGS ${_test_flags})
  list(APPEND FONTFORGE_EXTRA_CFLAGS -Wextra -pedantic -Wno-unused-parameter)
  add_compile_options(
    "$<$<COMPILE_LANGUAGE:C>:${FONTFORGE_DEFAULT_CFLAGS}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${FONTFORGE_DEFAULT_CXXFLAGS}>"
  )
endif()

enable_sanitizer("${ENABLE_SANITIZER}")

set_font_family(serif "${FONT_FAMILIES_SERIF}")
set_font_family(ui "${FONT_FAMILIES_UI}")
set_font_family(label "${FONT_FAMILIES_LABEL}")
set_font_family(mono "${FONT_FAMILIES_MONO}")

# Required and default dependencies
# Note: When adding a dependency, ensure it has an imported target (wherever appropriate)
find_package(Freetype 2.3.7 REQUIRED)
# Gettext/Intl and GLIB only needed for GUI builds
if(ENABLE_GUI)
  find_package(PkgConfig REQUIRED)
  find_package(Gettext REQUIRED)
  find_package_with_target(Intl REQUIRED)
  if(UNIX AND NOT APPLE)
    find_package(GLIB 2.6 REQUIRED COMPONENTS gio gobject)
  else()
    find_package(GLIB 2.58 REQUIRED COMPONENTS gio gobject)
  endif()
  pkg_check_modules(GLibMM REQUIRED IMPORTED_TARGET glibmm-2.4)
endif()
find_package(Iconv REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(MathLib REQUIRED)
find_package(ZLIB REQUIRED)

check_include_file(pthread.h HAVE_PTHREAD_H)
if(HAVE_PTHREAD_H)
  find_package(Threads)
endif()

if (ENABLE_HARFBUZZ)
  unset(_harfbuzz_required)
  if(NOT ${ENABLE_HARFBUZZ} STREQUAL "AUTO")
    set(_harfbuzz_required REQUIRED)
  endif()

  if(MSVC)
    find_package(harfbuzz CONFIG ${_harfbuzz_required})
    set(ENABLE_HARFBUZZ_RESULT ${harfbuzz_FOUND})
    if(harfbuzz_FOUND)
      # Can't use ALIAS because harfbuzz::harfbuzz is itself an alias
      add_library(PkgConfig::HarfBuzz INTERFACE IMPORTED)
      target_link_libraries(PkgConfig::HarfBuzz INTERFACE harfbuzz::harfbuzz)
    endif()
  else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(HarfBuzz ${_harfbuzz_required} IMPORTED_TARGET harfbuzz)
    set(ENABLE_HARFBUZZ_RESULT ${HarfBuzz_FOUND})
  endif()
endif()

# Requirements for options
if(ENABLE_GUI)
  pkg_check_modules(GDK3 REQUIRED IMPORTED_TARGET gdk-3.0>=3.10)
  include(${CMAKE_CURRENT_LIST_DIR}/cmake/TargetUtils.cmake)
  alias_imported_target(GDK3::GDK3 PkgConfig::GDK3)
  pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
  pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-3.0)
  if (ENABLE_HARFBUZZ)
    pkg_check_modules(HarfBuzz REQUIRED IMPORTED_TARGET harfbuzz)
  endif()
endif()

if(ENABLE_FREETYPE_DEBUGGER)
  if(NOT HAVE_PTHREAD_H)
    message(FATAL_ERROR "pthreads must be present to enable the FreeType debugger")
  else()
    set(FreeTypeSource_HINT ${ENABLE_FREETYPE_DEBUGGER})
    find_package(FreeTypeSource "${FREETYPE_VERSION_STRING}" EXACT REQUIRED)
  endif()
endif()

set(Python3_FIND_REGISTRY LAST)
if(BUILD_SHARED_LIBS)
  set(Python3_USE_STATIC_LIBS FALSE)
endif()
# Wheel builds only need Development.Module (not Embed)
# Full builds need Development (Module + Embed) for fontforgeexe
if(BUILDING_WHEEL)
  find_package_auto(ENABLE_PYTHON_SCRIPTING Python3 3.8 COMPONENTS Development.Module Interpreter)
else()
  find_package_auto(ENABLE_PYTHON_SCRIPTING Python3 3.8 COMPONENTS Development Interpreter)
endif()
find_package_auto(ENABLE_DOCS             Sphinx) # Should come after ENABLE_PYTHON_SCRIPTING
find_package_auto(ENABLE_LIBSPIRO         Libspiro)
find_package_auto(ENABLE_LIBGIF           GIF)
find_package_auto(ENABLE_LIBJPEG          JPEG)
find_package_auto(ENABLE_LIBPNG           PNG)
find_package(PkgConfig)  # Needed for pkg_check_auto calls below
pkg_check_auto(ENABLE_LIBREADLINE         Readline readline)
find_package_auto(ENABLE_LIBTIFF          TIFF)
pkg_check_auto(ENABLE_WOFF2               WOFF2 "libwoff2enc libwoff2dec")
pkg_check_auto(ENABLE_LIBUNIBREAK         Libunibreak libunibreak)
find_package_auto(ENABLE_CODE_COVERAGE    Gcov)

add_subdirectory(inc)
add_subdirectory(extern)
add_subdirectory(Unicode)
add_subdirectory(gutils)
add_subdirectory(fontforge)
if(ENABLE_GUI)
  add_subdirectory(gdraw)
endif()
if(NOT BUILDING_WHEEL)
  add_subdirectory(fontforgeexe)
endif()
if(ENABLE_PYTHON_EXTENSION)
  if(ENABLE_PYTHON_SCRIPTING_RESULT AND BUILD_SHARED_LIBS)
    set(ENABLE_PYTHON_EXTENSION_RESULT ON)
  elseif(NOT "${ENABLE_PYTHON_EXTENSION}" STREQUAL "AUTO")
    message(FATAL_ERROR "ENABLE_PYTHON_EXTENSION cannot be enabled if either ENABLE_PYTHON_SCRIPTING or BUILD_SHARED_LIBS is not")
  else()
    set(ENABLE_PYTHON_EXTENSION_RESULT OFF)
  endif()

  if(ENABLE_PYTHON_EXTENSION_RESULT)
    add_subdirectory(pyhook)
  endif()
endif()
if(NOT BUILDING_WHEEL)
  add_subdirectory(contrib)
endif()
if(ENABLE_PYTHON_SCRIPTING_RESULT AND NOT BUILDING_WHEEL)
  add_subdirectory(pycontrib)
endif()
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
if(ENABLE_GUI)
  add_subdirectory(desktop)
endif()
if(ENABLE_DOCS AND Sphinx_FOUND AND NOT BUILDING_WHEEL)
  add_subdirectory(doc)
endif()
if(NOT BUILDING_WHEEL AND ENABLE_GUI)
  add_subdirectory(po)
endif()
if(NOT BUILDING_WHEEL)
  add_subdirectory(share)
endif()
if(APPLE AND ENABLE_GUI)
  add_subdirectory(osx)
endif()

setup_cpack()
print_build_options()

add_custom_target(
    format
    COMMAND cat .clang-files | xargs -I % sh -c 'clang-format --verbose -i --style=file:.clang-format %'
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-format"
)
