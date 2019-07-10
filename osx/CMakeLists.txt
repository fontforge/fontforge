# Distributed under the original FontForge BSD 3-clause license

configure_file(
  FontForge.app/Contents/Info.plist.in
  Info.plist
  @ONLY
)
configure_file(
  FontForge.app/Contents/Resources/English.lproj/InfoPlist.string.in
  InfoPlist.string
  @ONLY
)

# This probably could be done with CPack, but whatever
add_custom_target(macbundle
  "${CMAKE_COMMAND}" -E remove_directory "FontForge.app"
  COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/FontForge.app" "FontForge.app"
  COMMAND "${CMAKE_COMMAND}" -E remove -f "FontForge.app/Contents/Info.plist.in" "FontForge.app/Contents/Resources/English.lproj/Info.plist.in"
  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_BINARY_DIR}/Info.plist" "FontForge.app/Contents/Info.plist"
  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_BINARY_DIR}/InfoPlist.string" "FontForge.app/Contents/Resources/English.lproj/InfoPlist.string"
  COMMAND "${CMAKE_COMMAND}" "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/FontForge.app/Contents/Resources/opt/local" -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
  COMMAND "${CMAKE_SOURCE_DIR}/travis-scripts/ffosxbuild.sh" "${CMAKE_CURRENT_BINARY_DIR}/FontForge.app" "${FONTFORGE_GIT_VERSION}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  VERBATIM
  USES_TERMINAL
)
