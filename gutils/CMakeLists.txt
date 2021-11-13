# Distributed under the original FontForge BSD 3-clause license

add_library(gutils OBJECT
  dlist.c
  fsys.c
  gcol.c
  gimagebmpP.h
  gimage.c
  gimagereadbmp.c
  gimageread.c
  gimagereadgif.c
  gimagereadjpeg.c
  gimagereadpng.c
  gimagereadras.c
  gimagereadrgb.c
  gimagereadtiff.c
  gimagereadxbm.c
  gimagereadxpm.c
  gimagewritebmp.c
  gimagewritegimage.c
  gimagewritejpeg.c
  gimagewritepng.c
  gimagewritexbm.c
  gimagewritexpm.c
  gutils.c
  gwwintl.c
  prefs.c
)

if(BUILD_SHARED_LIBS)
  set_property(TARGET gutils PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

list(APPEND gutils_LIBRARIES Intl::Intl)
if(ENABLE_LIBGIF_RESULT)
  list(APPEND gutils_LIBRARIES GIF::GIF)
endif()
if(ENABLE_LIBJPEG_RESULT)
  list(APPEND gutils_LIBRARIES JPEG::JPEG)
endif()
if(ENABLE_LIBPNG_RESULT)
  list(APPEND gutils_LIBRARIES PNG::PNG)
endif()
if(ENABLE_LIBTIFF_RESULT)
  list(APPEND gutils_LIBRARIES TIFF::TIFF)
endif()

if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
  make_object_interface(gutils
    fontforge_common_headers
    gunicode_interface
    ${gutils_LIBRARIES}
  )
else()
  target_link_libraries(gutils
    PUBLIC
      fontforge_common_headers
      gunicode
      ${gutils_LIBRARIES}
  )
endif()
