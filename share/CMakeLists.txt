# Distributed under the original FontForge BSD 3-clause license

install(FILES default DESTINATION ${CMAKE_INSTALL_DATADIR}/fontforge/hotkeys)
install(FILES prefs DESTINATION ${CMAKE_INSTALL_DATADIR}/fontforge)

# The following is merely to make it easier to run fontforge directly
# from the build folder to assist in debugging

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/fontforge/prefs")
  file(COPY prefs DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/fontforge")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/fontforge/hotkeys/default")
  file(COPY default DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/fontforge/hotkeys")
endif()
