# Distributed under the original FontForge BSD 3-clause license

add_library(layout OBJECT
  layout_shim.hpp
  layout_shim.cpp
)

if(ENABLE_GUI)
  target_sources(layout PRIVATE
    cairo_painter.hpp
    cairo_painter.cpp
  )
  target_link_libraries(layout PRIVATE
    fontforge_common_headers
    gunicode
    shapers
    PkgConfig::CairoMM
  )
  if(ENABLE_FREETYPE_DEBUGGER)
    target_link_libraries(layout PRIVATE FreeTypeSource::FreeTypeSource)
  endif()
  if(ENABLE_LIBSPIRO_RESULT)
    target_link_libraries(layout PUBLIC Libspiro::Libspiro)
  endif()
  target_compile_definitions(layout PRIVATE ENABLE_CAIRO)
endif()

if(BUILD_SHARED_LIBS)
  set_property(TARGET layout PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()
