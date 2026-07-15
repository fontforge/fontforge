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

if(ENABLE_CODE_COVERAGE_RESULT)
target_link_libraries(fontforge_common_headers
  INTERFACE
    Gcov::Gcov
)
endif()
