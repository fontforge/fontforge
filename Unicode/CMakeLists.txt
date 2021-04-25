# Distributed under the original FontForge BSD 3-clause license

add_library(gunicode OBJECT
  ArabicForms.c
  char.c
  memory.c
  ucharmap.c
  unialt.c
  uninames.c
  ustring.c
  utype.c
)

if(BUILD_SHARED_LIBS)
  set_property(TARGET gunicode PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
  make_object_interface(gunicode
    fontforge_common_headers
    Iconv::Iconv
    Intl::Intl
  )
else()
  target_link_libraries(gunicode
    PUBLIC
      fontforge_common_headers
      Iconv::Iconv
      Intl::Intl
  )
endif()

target_compile_options(gunicode PRIVATE ${FONTFORGE_EXTRA_CFLAGS})
