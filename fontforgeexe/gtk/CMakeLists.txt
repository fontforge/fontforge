# Distributed under the original FontForge BSD 3-clause license

add_subdirectory(widgets)

add_library(ffgtk OBJECT
  application.hpp
  application.cpp
  bitmaps_dlg.hpp
  bitmaps_dlg.cpp
  css_builder.hpp
  css_builder.cpp
  dialog_base.hpp
  dialog_base.cpp
  find_problems.hpp
  find_problems.cpp
  language_list.hpp
  language_list.cpp
  show_histogram.hpp
  show_histogram.cpp
  print_dialog.hpp
  print_dialog.cpp
  print_preview.hpp
  print_preview.cpp
  simple_dialogs.hpp
  simple_dialogs.cpp
  utils.hpp
  utils.cpp
  win32_utils.hpp
  win32_utils.cpp
  widgets/aspect_frame_bg.hpp
  widgets/rich_text_editor.hpp
  widgets/rich_text_editor.cpp
)

target_link_libraries(ffgtk PUBLIC
  fontforge_common_headers
  ffgtk_widgets
  fontforge
  PkgConfig::GTKMM
)
