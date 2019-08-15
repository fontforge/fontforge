# Distributed under the original FontForge BSD 3-clause license

file(STRINGS LINGUAS _langs)

foreach(_lang ${_langs})
  set(_input "${CMAKE_CURRENT_SOURCE_DIR}/${_lang}.po")
  set(_output "${CMAKE_CURRENT_BINARY_DIR}/${_lang}.mo")
  add_custom_command(OUTPUT "${_output}"
    COMMAND "${GETTEXT_MSGFMT_EXECUTABLE}" --check -o "${_output}" "${_input}"
    DEPENDS "${_input}"
    VERBATIM
  )
  install(FILES "${_output}" DESTINATION "${CMAKE_INSTALL_LOCALEDIR}/${_lang}/LC_MESSAGES" RENAME "FontForge.mo" COMPONENT pofiles)
  list(APPEND _outputs "${_output}")
endforeach()

add_custom_target(pofiles ALL DEPENDS ${_outputs})

# Run this target to regenerate the potfile
# TODO: Probably should add a FindXGettext or something
add_custom_target(potfiles
  COMMAND xgettext
    --from-code=utf-8
    -k_ -kN_ -kS_ -kU_ -kNU_ -kP_:1,2
    --sort-by-file
    --add-comments=GT:
    "-o${CMAKE_CURRENT_BINARY_DIR}/FontForge.pot"
    ../fontforge/*.c
    ../fontforgeexe/*.c
    ../gdraw/*.c
    ../gutils/*.c
    ../inc/*.h
    ../desktop/org.fontforge.FontForge.appdata.xml.in
    ../desktop/org.fontforge.FontForge.desktop.in
  COMMAND patch "${CMAKE_CURRENT_BINARY_DIR}/FontForge.pot" "pottitle.patch"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

if(ENABLE_MAINTAINER_TOOLS)
  add_executable(toengb toengb.c)
endif()
