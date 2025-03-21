# Distributed under the original FontForge BSD 3-clause license

add_library(ffgtk OBJECT
  css_builder.hpp
  css_builder.cpp
  dialog.hpp
  dialog.cpp
  simple_dialogs.hpp
  simple_dialogs.cpp
  utils.hpp
  utils.cpp
)

target_link_libraries(ffgtk PRIVATE
  fontforge_common_headers
  PkgConfig::GTKMM
)