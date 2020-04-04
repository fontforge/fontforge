# Distributed under the original FontForge BSD 3-clause license

add_library(fontforge_pyhook MODULE fontforgepyhook.c)
add_library(psMat_pyhook MODULE psMatpyhook.c)

set_target_properties(fontforge_pyhook PROPERTIES OUTPUT_NAME "fontforge" PREFIX "")
set_target_properties(psMat_pyhook PROPERTIES OUTPUT_NAME "psMat" PREFIX "")

if(WIN32)
  set_target_properties(fontforge_pyhook PROPERTIES SUFFIX ".pyd")
  set_target_properties(psMat_pyhook PROPERTIES SUFFIX ".pyd")
endif()

target_link_libraries(fontforge_pyhook PRIVATE fontforge)
target_link_libraries(psMat_pyhook PRIVATE fontforge)

target_link_libraries(fontforge_pyhook PRIVATE Python3::Module)
target_link_libraries(psMat_pyhook PRIVATE Python3::Module)

# FindPython3 provides Python3_SITEARCH, but this is an absolute path
# So do it ourselves, getting the prefix-relative path instead
if(NOT DEFINED PYHOOK_INSTALL_DIR)
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c "import distutils.sysconfig as sc; print(sc.get_python_lib(prefix='', plat_specific=True,standard_lib=False))"
    RESULT_VARIABLE _pyhook_install_dir_result
    OUTPUT_VARIABLE PYHOOK_INSTALL_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT PYHOOK_INSTALL_DIR OR _pyhook_install_dir_result)
    message(FATAL_ERROR "Could not determine the path to install pyhook binaries to, got '${PYHOOK_INSTALL_DIR}' with return value ${_pyhook_install_dir_result}")
  endif()
  set(PYHOOK_INSTALL_DIR "${PYHOOK_INSTALL_DIR}" CACHE STRING "Path to install pyhook to")
endif()

install(TARGETS fontforge_pyhook psMat_pyhook LIBRARY DESTINATION "${PYHOOK_INSTALL_DIR}")
