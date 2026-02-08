# Distributed under the original FontForge BSD 3-clause license

# Enable generator expressions in install(CODE)
cmake_policy(SET CMP0087 NEW)

add_library(fontforge_pyhook MODULE fontforgepyhook.c)
add_library(psMat_pyhook MODULE psMatpyhook.c)

set_target_properties(fontforge_pyhook PROPERTIES OUTPUT_NAME "fontforge" PREFIX "")
set_target_properties(psMat_pyhook PROPERTIES OUTPUT_NAME "psMat" PREFIX "")

if(WIN32)
  set_target_properties(fontforge_pyhook PROPERTIES SUFFIX ".pyd")
  set_target_properties(psMat_pyhook PROPERTIES SUFFIX ".pyd")
  # Put import libraries in a subdirectory to avoid collision with fontforge.lib from the main DLL
  set_target_properties(fontforge_pyhook psMat_pyhook PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/pyhook")
endif()

target_link_libraries(fontforge_pyhook PRIVATE fontforge Python3::Module)
target_link_libraries(psMat_pyhook PRIVATE fontforge Python3::Module)

if(BUILDING_WHEEL)
  # Wheel build: modules find libfontforge in same directory via $ORIGIN RPATH
  if(NOT WIN32)
    set_target_properties(fontforge_pyhook psMat_pyhook PROPERTIES
      INSTALL_RPATH "$ORIGIN"
      BUILD_WITH_INSTALL_RPATH TRUE
    )
  endif()
  # scikit-build-core handles install location via CMAKE_INSTALL_PREFIX
  install(TARGETS fontforge_pyhook psMat_pyhook LIBRARY DESTINATION ".")
else()
  # System install: use Python's site-packages
  # FindPython3 provides Python3_SITEARCH, but this is an absolute path
  # So do it ourselves, getting the prefix-relative path instead
  if(NOT DEFINED PYHOOK_INSTALL_DIR)
    if(APPLE)
      set(_PYHOOK_SYSCONFIG_PREFIX " 'posix_prefix',")
    endif()
    execute_process(
      COMMAND "${Python3_EXECUTABLE}" -c "import sysconfig as sc; print(sc.get_path('platlib',${_PYHOOK_SYSCONFIG_PREFIX} vars={'platbase': '.'}))"
      RESULT_VARIABLE _pyhook_install_dir_result
      OUTPUT_VARIABLE PYHOOK_INSTALL_DIR
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT PYHOOK_INSTALL_DIR OR _pyhook_install_dir_result)
      message(FATAL_ERROR "Could not determine the path to install pyhook binaries to, got '${PYHOOK_INSTALL_DIR}' with return value ${_pyhook_install_dir_result}")
    endif()
    set(PYHOOK_INSTALL_DIR "${PYHOOK_INSTALL_DIR}" CACHE STRING "Path to install pyhook to")
  endif()

  # Check at install time if pip-installed modules should be preserved
  # If existing fontforge module has a dotted version (e.g., 2025.10.9), it's from pip
  # If it has a plain numeric version (e.g., 20251009), it's from an app install
  install(CODE "
    set(_python_exe \"${Python3_EXECUTABLE}\")
    set(_skip_pyhook FALSE)

    # Check if fontforge module exists and get its version
    execute_process(
      COMMAND \"\${_python_exe}\" -c \"import fontforge; print(fontforge.version())\"
      RESULT_VARIABLE _ff_result
      OUTPUT_VARIABLE _ff_version
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(_ff_result EQUAL 0 AND _ff_version)
      # Check if version contains a dot (pip wheel format: 2025.10.9)
      string(FIND \"\${_ff_version}\" \".\" _dot_pos)
      if(NOT _dot_pos EQUAL -1)
        message(STATUS \"Existing fontforge module (version \${_ff_version}) appears to be pip-installed, skipping Python module installation\")
        message(STATUS \"To reinstall app modules, first run: pip uninstall fontforge\")
        set(_skip_pyhook TRUE)
      else()
        message(STATUS \"Existing fontforge module (version \${_ff_version}) is from app install, will be updated\")
      endif()
    endif()

    if(NOT _skip_pyhook)
      set(_dest \"\${CMAKE_INSTALL_PREFIX}/${PYHOOK_INSTALL_DIR}\")
      set(_version \"${FONTFORGE_PYTHON_VERSION}\")

      # Install the module files
      file(INSTALL
        DESTINATION \"\${_dest}\"
        TYPE MODULE
        FILES
          \"$<TARGET_FILE:fontforge_pyhook>\"
          \"$<TARGET_FILE:psMat_pyhook>\"
      )

      # Create dist-info directory so pip recognizes this installation
      set(_distinfo \"\${_dest}/fontforge-\${_version}.dist-info\")
      file(MAKE_DIRECTORY \"\${_distinfo}\")

      # METADATA - minimal package metadata
      file(WRITE \"\${_distinfo}/METADATA\" \"Metadata-Version: 2.1
Name: fontforge
Version: \${_version}
Summary: FontForge font editor - Python bindings (installed with app)
Home-page: https://fontforge.org
License: GPL-3.0-or-later
\")

      # INSTALLER - marks this as installed by the app, not pip
      file(WRITE \"\${_distinfo}/INSTALLER\" \"fontforge-app\")

      # top_level.txt - which top-level modules are installed
      file(WRITE \"\${_distinfo}/top_level.txt\" \"fontforge
psMat
\")

      # RECORD - list of installed files (with empty hashes, which is allowed)
      # Get the actual filenames
      get_filename_component(_ff_name \"$<TARGET_FILE:fontforge_pyhook>\" NAME)
      get_filename_component(_psmat_name \"$<TARGET_FILE:psMat_pyhook>\" NAME)
      file(WRITE \"\${_distinfo}/RECORD\" \"\${_ff_name},,
\${_psmat_name},,
fontforge-\${_version}.dist-info/METADATA,,
fontforge-\${_version}.dist-info/INSTALLER,,
fontforge-\${_version}.dist-info/top_level.txt,,
fontforge-\${_version}.dist-info/RECORD,,
\")

      message(STATUS \"Created fontforge-\${_version}.dist-info for pip compatibility\")
    endif()
  ")
endif()
