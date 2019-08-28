# Distributed under the original FontForge BSD 3-clause license

cmake_minimum_required(VERSION 3.5)

# Update the version for each new release
project(fontforge VERSION 20190801 LANGUAGES C CXX)

# Add folder for custom cmake modules
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_SOURCE_DIR}/cmake/packages)
if(${CMAKE_VERSION} VERSION_LESS "3.14")
  # This could be more targeted, but keep it simple
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/backports/3.14.5)
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
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_THREAD_PREFER_PTHREAD 1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set_default_build_type(RelWithDebInfo) # Sets CMAKE_BUILD_TYPE
set_default_rpath()
add_uninstall_target()

# Options
build_option(BUILD_SHARED_LIBS       BOOL ON  "Build libfontforge as a shared library")
build_option(ENABLE_GUI              BOOL ON  "Build FontForge with GUI support")
build_option(ENABLE_X11              BOOL OFF "Build the GUI using the X backend INSTEAD of the GDK backend" "ENABLE_GUI")

build_option(ENABLE_NATIVE_SCRIPTING BOOL ON  "Enables FontForge's native scripting support")
build_option(ENABLE_PYTHON_SCRIPTING BOOL ON  "Enables FontForge's Python scripting support")
build_option(ENABLE_PYTHON_EXTENSION BOOL ON  "Builds the Python models for use with system python" "ENABLE_PYTHON_SCRIPTING")

build_option(ENABLE_LIBSPIRO         BOOL ON  "Enables libspiro support")
build_option(ENABLE_LIBUNINAMESLIST  BOOL ON  "Enables libuninameslist support")
build_option(ENABLE_LIBGIF           AUTO ON  "Enables GIF support")
build_option(ENABLE_LIBJPEG          AUTO ON  "Enables JPEG support")
build_option(ENABLE_LIBPNG           AUTO ON  "Enables PNG support")
build_option(ENABLE_LIBREADLINE      AUTO ON  "Enables Readline support")
build_option(ENABLE_LIBTIFF          AUTO ON  "Enables TIFF support")
build_option(ENABLE_WOFF2            AUTO ON  "Enables WOFF2 support")

build_option(ENABLE_CODE_COVERAGE    BOOL OFF "Build with code coverage support")
build_option(ENABLE_DEBUG            BOOL OFF "Turn on debug build and FONTFORGE_DEBUG conditional code")
build_option(ENABLE_DEBUG_RAW_POINTS BOOL OFF "Add a raw mode to the points window of the debugger")
build_option(ENABLE_FONTFORGE_EXTRAS BOOL OFF "Builds programs from the contrib directory")
build_option(ENABLE_MAINTAINER_TOOLS BOOL OFF "Build programs normally only used by FontForge maintainers and developers")
build_option(ENABLE_TILE_PATH        BOOL OFF "Enable a 'tile path' command (a variant of 'expand stroke')")
build_option(ENABLE_WRITE_PFM        BOOL OFF "Add the ability to save a PFM file without creating the associated font file")

build_option(ENABLE_FREETYPE_DEBUGGER PATH "" "Use FreeType's internal debugger within FontForge.\
 If enabling this option, this must be set to the path of the FreeType source.\
 The version of that source must match the version that FontForge will link to.")

build_option(REAL_TYPE ENUM "double" "Sets the floating point type used." "double" "float")
build_option(THEME     ENUM "tango"  "Sets the GUI theme." "tango" "2012")

if(CMAKE_COMPILER_IS_GNUCC)
  set_supported_c_compiler_flags(FONTFORGE_DEFAULT_CFLAGS -Werror=implicit-function-declaration -Werror=int-conversion)
endif()

# Required and default dependencies
# Note: When adding a dependency, ensure it has an imported target (wherever appropriate) 
find_package(Freetype 2.3.7 REQUIRED)
find_package(Gettext REQUIRED)
find_package_with_target(Intl REQUIRED)
find_package(GLIB 2.6 REQUIRED COMPONENTS gio)
find_package(Iconv REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(MathLib REQUIRED)
find_package(ZLIB REQUIRED)

check_include_file(pthread.h HAVE_PTHREAD_H)
if(HAVE_PTHREAD_H)
  find_package(Threads)
endif()

# Requirements for options
if(ENABLE_GUI)
  if(ENABLE_X11)
    find_package(Cairo 1.6 REQUIRED)
    find_package(Pango 1.10 COMPONENTS pangocairo pangoxft REQUIRED)
    find_package(X11 REQUIRED)
  else()
    find_package(GDK3 3.10 REQUIRED)
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
find_package_auto(ENABLE_PYTHON_SCRIPTING Python3 3.3 COMPONENTS Development Interpreter)
find_package_auto(ENABLE_LIBSPIRO         Libspiro)
find_package_auto(ENABLE_LIBUNINAMESLIST  Libuninameslist)
find_package_auto(ENABLE_LIBGIF           GIF)
find_package_auto(ENABLE_LIBJPEG          JPEG)
find_package_auto(ENABLE_LIBPNG           PNG)
find_package_auto(ENABLE_LIBREADLINE      Readline)
find_package_auto(ENABLE_LIBTIFF          TIFF)
find_package_auto(ENABLE_WOFF2            WOFF2)
find_package_auto(ENABLE_CODE_COVERAGE    Gcov)

add_subdirectory(inc)
add_subdirectory(Unicode)
add_subdirectory(gutils)
add_subdirectory(fontforge)
if(ENABLE_GUI)
  add_subdirectory(gdraw)
endif()
add_subdirectory(fontforgeexe)
if(ENABLE_PYTHON_SCRIPTING_RESULT AND ENABLE_PYTHON_EXTENSION)
  add_subdirectory(pyhook)
endif()
add_subdirectory(contrib)
if(ENABLE_PYTHON_SCRIPTING_RESULT)
  add_subdirectory(pycontrib)
endif()
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
add_subdirectory(desktop)
add_subdirectory(doc)
add_subdirectory(po)
add_subdirectory(share)
if(APPLE)
  add_subdirectory(osx)
endif()

setup_cpack()
print_build_options()
