# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
TestUtils
---------

``add_download_target` creates a custom target that will ensure that
``font`` exists. If it is not present in the source tree, it will be
downloaded via ``url``.

``add_ff_test`` and ``add_py_test`` both wrap ``add_test`` to add a
standard system test, which involves invoking either a native or Python
script. System tests are performed by invoking ``systestdriver``, which
handles determining if a test should be skipped, based on missing inputs,
and also runs each test in its own test folder.

#]=======================================================================]

function(add_download_target font url)
  add_custom_command(
    OUTPUT
      "${CMAKE_CURRENT_BINARY_DIR}/fonts/${font}"
    COMMAND "${CMAKE_COMMAND}"
      -D DEST:FILEPATH="${CMAKE_CURRENT_BINARY_DIR}/fonts/${font}"
      -D SOURCE:FILEPATH="${CMAKE_CURRENT_SOURCE_DIR}/fonts/${font}"
      -D URL:STRING="${url}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/scripts/DownloadIfMissing.cmake"
  )
endfunction()

function(_add_systest test_mode binary test_script)
  get_filename_component(_test_name "${test_script}" NAME_WE)
  set(_test_name "${_test_name}_${test_mode}")

  list(LENGTH ARGN _arglen)
  if (${_arglen} LESS 1)
    message(FATAL_ERROR "Must pass a description as the last argument")
  endif()
  list(GET ARGN -1 _description)
  list(REMOVE_AT ARGN -1)

  # Prior to CMake 3.9, skipped tests fail the suite
  if(${CMAKE_VERSION} VERSION_LESS "3.9")
    set(_skip_arg --skip-as-pass)
  endif()

  add_test(NAME ${_test_name}
    COMMAND systestdriver
      --mode ${test_mode}
      --binary "${binary}"
      --script "${CMAKE_CURRENT_SOURCE_DIR}/${test_script}"
      --exedir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
      --libdir "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
      --argdir "${CMAKE_CURRENT_BINARY_DIR}/fonts"
      --argdir "${CMAKE_CURRENT_SOURCE_DIR}/fonts"
      --desc "${_description}"
      ${_skip_arg}
      ${ARGN}
    WORKING_DIRECTORY
      "${CMAKE_CURRENT_BINARY_DIR}"
  )
  set_tests_properties(${_test_name} PROPERTIES SKIP_RETURN_CODE 77)
endfunction()

function(add_ff_test test_script)
  # Native scripting tests require fontforgeexe, not available in wheel builds
  if(NOT BUILDING_WHEEL)
    _add_systest(ff "$<TARGET_FILE:fontforgeexe>" "${test_script}" ${ARGN})
  endif()
endfunction()

function(add_py_test test_script)
  list(FIND ARGN PYHOOK_DISABLED _disable_pyhook_index)
  if(NOT _disable_pyhook_index LESS 0)
    list(REMOVE_AT ARGN ${_disable_pyhook_index})
    set(_disable_pyhook 1)
  endif()

  # fontforgeexe-based Python tests not available in wheel builds
  if(NOT BUILDING_WHEEL)
    _add_systest(py "$<TARGET_FILE:fontforgeexe>" "${test_script}" ${ARGN})
  endif()
  if(ENABLE_PYTHON_EXTENSION_RESULT AND NOT _disable_pyhook)
    _add_systest(pyhook "${Python3_EXECUTABLE}" "${test_script}" ${ARGN})
  endif()
endfunction()
