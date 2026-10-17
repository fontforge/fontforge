# Distributed under the original FontForge BSD 3-clause license

add_subdirectory(fontforge)
if(ENABLE_FONTFORGE_EXTRAS)
  add_subdirectory(fonttools)
endif()
