# Distributed under the original FontForge BSD 3-clause license

add_library(ffgtk OBJECT
  font_view_shim.hpp
  font_view_shim.cpp
  utils.hpp
  utils.cpp
)

target_link_libraries(ffgtk PRIVATE
  fontforge_common_headers
  PkgConfig::GTKMM
)
