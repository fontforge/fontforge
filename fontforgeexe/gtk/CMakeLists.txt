# Distributed under the original FontForge BSD 3-clause license

add_subdirectory(widgets)

add_library(ffgtk OBJECT
  application.hpp
  application.cpp
  c_context.h
  char_grid.hpp
  char_grid.cpp
  dialog.hpp
  dialog.cpp
  font_view.hpp
  font_view.cpp
  font_view_menu.cpp
  font_view_shim.hpp
  font_view_shim.cpp
  i_char_grid_containter.hpp
  kerning_format.hpp
  kerning_format.cpp
  menu_builder.hpp
  menu_builder.cpp
  menu_ids.h
  select_glyphs.hpp
  select_glyphs.cpp
  ui_context.hpp
  utils.hpp
  utils.cpp
)

target_link_libraries(ffgtk PRIVATE
  fontforge_common_headers
  ffgtk_widgets
  PkgConfig::GTKMM
)
