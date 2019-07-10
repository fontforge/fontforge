# Distributed under the original FontForge BSD 3-clause license

if(ENABLE_MAINTAINER_TOOLS)
  add_subdirectory(cidmap)
endif()
if(ENABLE_FONTFORGE_EXTRAS)
  add_subdirectory(fonttools)
endif()