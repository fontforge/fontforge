# Distributed under the original FontForge BSD 3-clause license

add_library(gunicode OBJECT
  ArabicForms.c
  char.c
  combiners.h
  is_Ligature_data.h
  is_Ligature.c
  memory.c
  ucharmap.c
  unialt.c
  ustring.c
  utype.c
)

if(BUILD_SHARED_LIBS)
  set_property(TARGET gunicode PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
  make_object_interface(gunicode
    fontforge_common_headers
  )
else()
  target_link_libraries(gunicode
    PUBLIC
      fontforge_common_headers
  )
endif()

if (ENABLE_MAINTAINER_TOOLS)
  add_executable(makeutype makeutype.c)
  target_link_libraries(makeutype PRIVATE fontforge_common_headers)

  add_executable(makebuildtables makebuildtables.c)
  target_link_libraries(makebuildtables PRIVATE fontforge_common_headers)

  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(makeutype PRIVATE -Wall)
    target_compile_options(makebuildtables PRIVATE -Wall)
  endif()
endif()
