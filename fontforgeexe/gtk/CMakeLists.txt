# Placeholder for GTK C++ sources
# TODO: replace "INTERFACE" with "OBJECT"
add_library(ffgtk INTERFACE
)

# TODO: replace "INTERFACE" with "PRIVATE"
target_link_libraries(ffgtk INTERFACE
  fontforge_common_headers
  PkgConfig::GTKMM
)