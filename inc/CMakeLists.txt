# Distributed under the original FontForge BSD 3-clause license

add_library(fontforge_common_headers INTERFACE)

include(FontForgeConfigure)
fontforge_generate_config(fontforge-config.h.in fontforge-config.h)
fontforge_generate_version_extras(fontforge-version-extras.h.in fontforge-version-extras.h)

target_include_directories(fontforge_common_headers
  INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(fontforge_common_headers
  INTERFACE
    GLIB::GLIB
)

if(ENABLE_CODE_COVERAGE_RESULT)
target_link_libraries(fontforge_common_headers
  INTERFACE
    Gcov::Gcov
)
endif()

set(FONTFORGE_INST_COMMON_HEADERS
  basics.h
  carbon.h
  chardata.h
  charset.h
  dlist.h
  ffglib.h
  "${CMAKE_CURRENT_BINARY_DIR}/fontforge-config.h"
  gdraw.h
  gfile.h
  ggadget.h
  gicons.h
  gimage.h
  gio.h
  gkeysym.h
  gprogress.h
  gresedit.h
  gresource.h
  gwidget.h
  gwwiconv.h
  intl.h
  ustring.h
  utype.h
)

set_target_properties(fontforge_common_headers PROPERTIES PUBLIC_HEADER "${FONTFORGE_INST_COMMON_HEADERS}")

install(TARGETS fontforge_common_headers PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
