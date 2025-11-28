# Distributed under the original FontForge BSD 3-clause license

add_library(ffgtk OBJECT
  application.hpp
  application.cpp
  bitmaps_dlg.hpp
  bitmaps_dlg.cpp
  css_builder.hpp
  css_builder.cpp
  dialog.hpp
  dialog.cpp
  language_list.hpp
  language_list.cpp
  simple_dialogs.hpp
  simple_dialogs.cpp
  utils.hpp
  utils.cpp
)

target_link_libraries(ffgtk PRIVATE
  fontforge_common_headers
  gunicode
  PkgConfig::GTKMM
)
