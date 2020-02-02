# Distributed under the original FontForge BSD 3-clause license

add_library(gunicode OBJECT
  ArabicForms.c
  char.c
  combiners.h
  gwwiconv.c
  is_Ligature_data.h
  is_Ligature.c
  memory.c
  ucharmap.c
  unialt.c
  ustring.c
  utype.c

  charset/alphabet.c
  charset/big5.c
  charset/big5hkscs.c
  charset/gb2312.c
  charset/iso_8859_10.c
  charset/iso_8859_11.c
  charset/iso_8859_13.c
  charset/iso_8859_14.c
  charset/iso_8859_15.c
  charset/iso_8859_16.c
  charset/iso_8859_1.c
  charset/iso_8859_2.c
  charset/iso_8859_3.c
  charset/iso_8859_4.c
  charset/iso_8859_5.c
  charset/iso_8859_6.c
  charset/iso_8859_7.c
  charset/iso_8859_8.c
  charset/iso_8859_9.c
  charset/jis201.c
  charset/jis.c
  charset/johab.c
  charset/koi8_r.c
  charset/ksc5601.c
  charset/mac.c
  charset/MacSymbol.c
  charset/win.c
  charset/ZapfDingbats.c
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
