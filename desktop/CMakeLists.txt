# Distributed under the original FontForge BSD 3-clause license

if(UNIX)
  install(FILES org.fontforge.FontForge.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install(FILES fontforge.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)

  foreach(_size 16x16 22x22 24x24 32x32 48x48 64x64 128x128 256x256 512x512)
    install(FILES "${THEME}/${_size}/org.fontforge.FontForge.png" DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_size}/apps)
  endforeach()
  install(FILES "${THEME}/scalable/org.fontforge.FontForge.svg" DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

  file(STRINGS "${PROJECT_SOURCE_DIR}/po/LINGUAS" _langs)
  foreach(_lang ${_langs})
    list(APPEND _pofiles "${PROJECT_SOURCE_DIR}/po/${_lang}.po")
  endforeach()

  macro(msgfmt_desktop_file _mode _file _installdir)
    set(_input "${CMAKE_CURRENT_SOURCE_DIR}/${_file}.in")
    set(_output "${CMAKE_CURRENT_BINARY_DIR}/${_file}")
    add_custom_command(OUTPUT "${_output}"
      COMMAND "${GETTEXT_MSGFMT_EXECUTABLE}"
        --${_mode} --template "${_input}"
        -d "${PROJECT_SOURCE_DIR}/po"
        -o "${_output}"
      DEPENDS "${_input}" ${_pofiles}
      VERBATIM
    )
    install(FILES "${_output}" DESTINATION "${_installdir}")
    list(APPEND _desktop_dependencies "${_output}")
  endmacro()

  # This would work, except there's a bug on msgfmt 0.19.7 which causes
  # desktop files to be mis-parsed if there's lines longer than 100 chars
  # msgfmt_desktop_file(desktop org.fontforge.FontForge.desktop "${CMAKE_INSTALL_DATADIR}/applications")

  msgfmt_desktop_file(xml org.fontforge.FontForge.appdata.xml "${CMAKE_INSTALL_DATADIR}/metainfo")

  add_custom_target(appstream ALL DEPENDS ${_desktop_dependencies})
endif()
