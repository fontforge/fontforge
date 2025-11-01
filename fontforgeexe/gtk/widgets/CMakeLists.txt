# Distributed under the original FontForge BSD 3-clause license

add_library(ffgtk_widgets STATIC
  num_entry.hpp
)

set_target_properties(ffgtk_widgets PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(ffgtk_widgets PRIVATE
  PkgConfig::GTKMM
)
