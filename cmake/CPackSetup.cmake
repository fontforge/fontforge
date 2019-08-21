# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
CPackSetup
----------

``setup_cpack`` sets up CPack. This currently means it sets it up to
create the desired source archive. It also creates the dist target.

This could probably be extended to provide things like Deb/RPMs
directly, but that is left for another time.

#]=======================================================================]

function(setup_cpack)
  set(CPACK_GENERATOR "TXZ")
  set(CPACK_SOURCE_GENERATOR "TXZ")
  set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${PROJECT_VERSION}")
  set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
  set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION})
  set(CPACK_PACKAGE_VERSION_MINOR 0)
  set(CPACK_PACKAGE_VERSION_PATCH 0)
  set(CPACK_SOURCE_IGNORE_FILES
    # Version control:
    "/\\\\.git/"
    "\\\\.swp$"
    "\\\\.#"
    "/#"
    "/\\\\.gitignore$"
    "/\\\\.gitattributes$"

    # Development & Runtime created files
    "~$"
    "\\\\.mode"
    "\\\\.pbxuser$"
    "\\\\.perspective"
    "\\\\.pyc$"
    "\\\\.pyo$"
    "/cmake-build/"
    "/build/"

    # System trash
    "/\\\\.DS_Store"
    "/\\\\._"
    "/\\\\.Spotlight-V100$"
    "/\\\\.Trashes$"
    "/ethumbs.db$"
    "/Thumbs.db$"
  )
  # CPACK_SOURCE_INSTALLED_DIRECTORIES won't work because the files we want to include
  # can be in an ignored directory (build/). So use a script instead
  set(CPACK_INSTALL_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/CPackExtraDist.cmake")
  set(CPACK_PROPERTIES_FILE "${CMAKE_SOURCE_DIR}/cmake/scripts/CPackProperties.cmake")
  set(DEB_CHANGELOG_IN "${CMAKE_SOURCE_DIR}/Packaging/debian/changelog.in")
  set(DEB_CP_SRC "${CMAKE_SOURCE_DIR}/Packaging/debian/cp-src")
  configure_file("${CMAKE_SOURCE_DIR}/cmake/scripts/ExtraDist.cmake.in" "CPackExtraDist.cmake" @ONLY)
  include(CPack)

  add_custom_target(dist
    COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target package_source
    DEPENDS test_dependencies
    VERBATIM
    USES_TERMINAL
  )

  if(NOT CMAKE_CPACK_COMMAND)
    set(CMAKE_CPACK_COMMAND cpack)
  endif()

  add_custom_target(deb-src
    COMMAND "${CMAKE_COMMAND}"
      -E env "FONTFORGE_SOURCE_PACK_MODE=DEB"
        "${CMAKE_CPACK_COMMAND}"
        -G TGZ
        --config CPackSourceConfig.cmake
    DEPENDS test_dependencies
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    VERBATIM
    USES_TERMINAL
  )

  # Kind of an odd place to put this I guess...
  add_custom_target(rpm-spec
    COMMAND "${CMAKE_COMMAND}"
    "-DINPUT:PATH=${CMAKE_SOURCE_DIR}/Packaging/redhat/FontForge.spec.in"
    "-DOUTPUT:PATH=${CMAKE_BINARY_DIR}/FontForge.spec"
    "-DPROJECT_VERSION:STRING=${PROJECT_VERSION}"
    -P "${CMAKE_SOURCE_DIR}/cmake/scripts/ConfigureFile.cmake"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    VERBATIM
    USES_TERMINAL
  )
endfunction()
