# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
CPackProperties
---------------

This is used to set up additional variables that are used when
generating a source archive to make a Debian package. The expected use
case is something like:

DEB_PACKAGER_NAME="your name" DEB_PACKAGER_EMAIL=your@email ninja deb-src

These defaults are all geared towards making a release for the Ubuntu PPA

#]=======================================================================]

if("$ENV{FONTFORGE_SOURCE_PACK_MODE}" STREQUAL "DEB")
  if(NOT DEFINED ENV{DEB_PRODUCT_VERSION})
    execute_process(
      COMMAND
        git describe
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      RESULT_VARIABLE GIT_RETVAL
      OUTPUT_VARIABLE GIT_OUTPUT
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(${GIT_RETVAL} EQUAL 0)
      set(DEB_PRODUCT_VERSION "${GIT_OUTPUT}")
      set(DEB_PRODUCT_VERSION_INFERRED TRUE)
    else()
      message(FATAL_ERROR "Could not infer the product version, set DEB_PRODUCT_VERSION")
    endif()
  else()
    set(DEB_PRODUCT_VERSION "$ENV{DEB_PRODUCT_VERSION}")
  endif()
  
  set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${DEB_PRODUCT_VERSION}")
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${DEB_PRODUCT_VERSION}")
  
  if (DEFINED ENV{DEB_UBUNTU_RELEASE})
    if(DEB_PRODUCT_VERSION_INFERRED)
      set(DEB_PRODUCT_VERSION "${DEB_PRODUCT_VERSION}-0ubuntu1~$ENV{DEB_UBUNTU_RELEASE}")
    endif()
    set(DEB_OS_RELEASE "$ENV{DEB_UBUNTU_RELEASE}")
  endif()
  
  if(NOT DEFINED ENV{DEB_PACKAGE_DATE})
    execute_process(
      COMMAND
        date "+%a, %d %b %Y %H:%M:%S %z"
        RESULT_VARIABLE DATE_RETVAL
        OUTPUT_VARIABLE DATE_OUTPUT
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(${DATE_RETVAL} EQUAL 0)
      set(DEB_PACKAGE_DATE "${DATE_OUTPUT}")
    else()
      message(FATAL_ERROR "Could not infer the package date, set DEB_PACKAGE_DATE")
    endif()
  else()
    set(DEB_PACKAGE_DATE "$ENV{DEB_PACKAGE_DATE}")
  endif()
endif()
