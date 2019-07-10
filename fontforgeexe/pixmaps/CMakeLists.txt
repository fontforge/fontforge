# Distributed under the original FontForge BSD 3-clause license

# Set this for configuring the resources file
if(APPLE)
  set(GDRAW_GMENU_MACIONS "True")
else()
  set(GDRAW_GMENU_MACIONS "False")
endif()

add_subdirectory(${THEME})
