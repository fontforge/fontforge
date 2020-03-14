# Distributed under the original FontForge BSD 3-clause license

# * Run the 'pofiles' command to generate the binary .mo files
# * Run the 'potfiles' ocmmand to generate the master .pot file
#   (which cotnains all translatable strings)
# * Run the 'fullpo' command to generate the .po files, merged
#   with the master .pot file and with context

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
    ../desktop/org.fontforge.FontForge.desktop
  COMMAND patch "${CMAKE_CURRENT_BINARY_DIR}/FontForge.pot" "pottitle.patch"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

file(STRINGS LINGUAS _langs)

foreach(_lang ${_langs})
  set(_input "${CMAKE_CURRENT_SOURCE_DIR}/${_lang}.po")
  set(_output "${CMAKE_CURRENT_BINARY_DIR}/${_lang}.mo")
  set(_full "${CMAKE_CURRENT_BINARY_DIR}/full/${_lang}.po")
  add_custom_command(OUTPUT "${_output}"
    COMMAND "${GETTEXT_MSGFMT_EXECUTABLE}" --check -o "${_output}" "${_input}"
    DEPENDS "${_input}"
    VERBATIM
  )
  add_custom_command(OUTPUT "${_full}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/full"
    COMMAND "${GETTEXT_MSGMERGE_EXECUTABLE}" -N "${_input}" -F "${CMAKE_CURRENT_BINARY_DIR}/FontForge.pot" -o "${_full}"
    DEPENDS "${_input}" potfiles
    VERBATIM
  )
  install(FILES "${_output}" DESTINATION "${CMAKE_INSTALL_LOCALEDIR}/${_lang}/LC_MESSAGES" RENAME "FontForge.mo" COMPONENT pofiles)
  list(APPEND _outputs "${_output}")
  list(APPEND _fulls "${_full}")
endforeach()

add_custom_target(pofiles ALL DEPENDS ${_outputs})
add_custom_target(fullpo DEPENDS ${_fulls})

if(ENABLE_MAINTAINER_TOOLS)
  add_executable(toengb toengb.c)
endif()
