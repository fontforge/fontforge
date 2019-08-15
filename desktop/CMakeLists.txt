# Distributed under the original FontForge BSD 3-clause license

if(UNIX)
  install(FILES org.fontforge.FontForge.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install(FILES org.fontforge.FontForge.appdata.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
  install(FILES fontforge.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)

  foreach(_size 16x16 22x22 24x24 32x32 48x48 64x64 128x128 256x256 512x512)
    install(FILES "${THEME}/${_size}/org.fontforge.FontForge.png" DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_size}/apps)
  endforeach()
  install(FILES "${THEME}/scalable/org.fontforge.FontForge.svg" DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

  install(
    FILES
      "${THEME}/pixmaps/org.fontforge.FontForge.xpm"
      "${THEME}/32x32/org.fontforge.FontForge.png"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps
  )
endif()
