# Distributed under the original FontForge BSD 3-clause license

add_library(shapers OBJECT
  builtin.hpp
  builtin.cpp
  harfbuzz.hpp
  harfbuzz.cpp
  shaper_shim.hpp
  shaper_shim.cpp
)

if(BUILD_SHARED_LIBS)
  set_property(TARGET shapers PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

target_link_libraries(shapers PRIVATE
  fontforge_common_headers
  gunicode
)

if(ENABLE_HARFBUZZ)
  target_link_libraries(shapers PRIVATE
    PkgConfig::HarfBuzz
  )
  add_definitions(-DENABLE_HARFBUZZ)

  if(HarfBuzz_VERSION VERSION_GREATER "10.2.0")
    add_definitions(-DHB_OT_SHAPE_PLAN_GET_FEATURE_TAGS)
  endif()
endif()
