# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
TargetUtils
-----------

``fixup_link_options`` inspects link options and fixes link options,
particularly those inferred from pkg-config. Specifically for MacOS,
 -framework are fixed. target_link_options deduplicates flags,
so ``-framework A -framework B`` becomes ``-framework A B``. This
is incorrect syntax. To fix this, the list must be altered to specify
framework options as ``SHELL:-framework A`` ``SHELL:-framework B``

``alias_imported_target` copies an imported target of name ``src` into
a new imported target of name ``dest``. This is used because CMake
currently does not support aliasing imported targets.

``make_object_interface`` creates an interface library from an
object library of name ``objlib``. This interface includes the
target objects from ``objlib``. Additional arguments passed into
this function will be treated as libraries to link to this interface
library. This function is for use on CMake versions less than 3.12,
which do not support calling target_link_libraries directly on
object libraries.

``set_supported_c_compiler_flags`` checks to see if the provided list
of flags are supported by the C compiler. Supported flags are stored
into ``dst`` as a cache variable.

``add_uninstall_target`` add an uninstall target

#]=======================================================================]

function(fixup_link_options dest)
  unset(_item_buffer)
  foreach(_item ${ARGN})
    if (APPLE AND "${_item}" STREQUAL "-framework")
      set(_item_buffer "-framework")
    elseif(DEFINED _item_buffer)
      list(APPEND _items "SHELL:${_item_buffer} ${_item}")
      unset(_item_buffer)
    else()
      list(APPEND _items "${_item}")
    endif()
  endforeach()
  if(DEFINED _item_buffer)
    message(FATAL_ERROR "Could not fixup/parse ${ARGN} --> stray argument ${_item_buffer}")
  endif()
  set(${dest} ${_items} PARENT_SCOPE)
endfunction()

function(alias_imported_target dest src)
  if(TARGET ${src} AND NOT TARGET ${dest})
    add_library(${dest} INTERFACE IMPORTED)
    foreach(prop INTERFACE_INCLUDE_DIRECTORIES INTERFACE_LINK_LIBRARIES INTERFACE_LINK_OPTIONS INTERFACE_COMPILE_OPTIONS)
      get_property(_prop_val TARGET ${src} PROPERTY ${prop})
      if(APPLE AND prop STREQUAL INTERFACE_LINK_OPTIONS)
        fixup_link_options(_prop_val ${_prop_val})
      endif()
      set_property(TARGET ${dest} PROPERTY ${prop} ${_prop_val})
    endforeach()
  endif()
endfunction()

function(make_object_interface objlib)
  add_library(${objlib}_interface INTERFACE)
  target_sources(${objlib}_interface INTERFACE $<TARGET_OBJECTS:${objlib}>)
  target_link_libraries(${objlib}_interface
    INTERFACE
      ${ARGN}
  )
  target_include_directories(${objlib}
    PUBLIC
      $<TARGET_PROPERTY:${objlib}_interface,INTERFACE_INCLUDE_DIRECTORIES>
  )
endfunction()

function(set_supported_c_compiler_flags dst)
  if(NOT DEFINED "${dst}")
    include(CheckCCompilerFlag)
    include(CMakePushCheckState)
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_QUIET TRUE)

    foreach(_arg ${ARGN})
      message(STATUS "Checking if ${_arg} is supported...")
      check_c_compiler_flag(${_arg} _supported_flag)
      if(_supported_flag)
        message(STATUS "  Flag is supported: ${_arg}")
        list(APPEND _supported_flags ${_arg})
      else()
        message(STATUS "  Flag is unsupported: ${_arg}")
      endif()
      unset(_supported_flag CACHE)
    endforeach()

    set("${dst}" "${_supported_flags}" CACHE STRING "Supported compiler flags")
    cmake_pop_check_state()
  endif()
endfunction()

# Based on https://gitlab.kitware.com/cmake/community/wikis/FAQ#can-i-do-make-uninstall-with-cmake
function(add_uninstall_target)
  if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_SOURCE_DIR}/cmake/scripts/Uninstall.cmake.in"
        "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY
    )
    add_custom_target(uninstall
        COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake"
        VERBATIM USES_TERMINAL
    )
  endif()
endfunction()
