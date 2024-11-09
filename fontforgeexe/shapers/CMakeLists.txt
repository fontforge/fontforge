# Distributed under the original FontForge BSD 3-clause license

add_library(shapers OBJECT
  shaper_shim.hpp
  shaper_shim.cpp
)

target_link_libraries(shapers PRIVATE
  fontforge_common_headers
)

if(ENABLE_HARFBUZZ)
  target_link_libraries(shapers PRIVATE
    PkgConfig::HarfBuzz
  )
  add_definitions(-DENABLE_HARFBUZZ)
endif()
