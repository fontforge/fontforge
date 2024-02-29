# Distributed under the original FontForge BSD 3-clause license

add_library(ffgtk OBJECT
  dialogs.hpp
  dialogs.cpp
)

target_link_libraries(ffgtk PRIVATE
  fontforge_common_headers
  PkgConfig::GTKMM
)