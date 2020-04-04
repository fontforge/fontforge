# Distributed under the original FontForge BSD 3-clause license

add_executable(fontforgeexe main.c)
set_target_properties(fontforgeexe PROPERTIES OUTPUT_NAME "fontforge")

if(ENABLE_GUI)
  target_sources(fontforgeexe PRIVATE
    alignment.c
    anchorsaway.c
    autowidth2dlg.c
    basedlg.c
    bdfinfo.c
    bitmapdlg.c
    bitmapview.c
    charinfo.c
    charview_private.h
    charview.c
    clipui.c
    combinations.c
    contextchain.c
    cursors.c
    cvaddpoints.c
    cvdebug.c
    cvdgloss.c
    cvexportdlg.c
    cvfreehand.c
    cvgetinfo.c
    cvgridfit.c
    cvhand.c
    cvhints.c
    cvimportdlg.c
    cvknife.c
    cvpalettes.c
    cvpointer.c
    cvruler.c
    cvruler.h
    cvshapes.c
    cvstroke.c
    cvtranstools.c
    deltaui.c
    displayfonts.c
    effectsui.c
    encodingui.c
    fontinfo.c
    fontview.c
    freetypeui.c
    fvfontsdlg.c
    fvmetricsdlg.c
    gotodlg.c
    groupsdlg.c
    histograms.c
    images.c
    justifydlg.c
    kernclass.c
    layer2layer.c
    lookupui.c
    macencui.c
    math.c
    metricsview.c
    mmdlg.c
    nonlineartransui.c
    openfontdlg.c
    prefs.c
    problems.c
    pythonui.c
    savefontdlg.c
    scriptingdlg.c
    scstylesui.c
    searchview.c
    sftextfield.c
    sftextfieldP.h
    sfundo.c
    sfundo.h
    showatt.c
    simplifydlg.c
    splashimage.c
    startui.c
    statemachine.c
    tilepath.c
    transform.c
    ttfinstrsui.c
    uiutil.c
    usermenu.c
    usermenu.h
    windowmenu.c
    wordlistparser.c
    wordlistparser.h
  )

  if(APPLE)
    target_sources(fontforgeexe PRIVATE macobjective.m)
    target_link_libraries(fontforgeexe
      PRIVATE
      "-framework Cocoa"
    )
  endif()

  if(ENABLE_FREETYPE_DEBUGGER)
    target_link_libraries(fontforgeexe PRIVATE Threads::Threads)
  endif()

  target_link_libraries(fontforgeexe PRIVATE gdraw)
else()
  target_sources(fontforgeexe PRIVATE startnoui.c)
endif()

if(ENABLE_PYTHON_SCRIPTING_RESULT)
  target_link_libraries(fontforgeexe PRIVATE Python3::Python)
endif()

target_link_libraries(fontforgeexe PRIVATE fontforge Iconv::Iconv MathLib::MathLib)
install(TARGETS fontforgeexe RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(UNIX)
  install(CODE "
    foreach(_script fontimage.pe fontlint.pe sfddiff.pe)
      get_filename_component(_output \"\${_script}\" NAME_WE)
      file(READ \"${CMAKE_CURRENT_SOURCE_DIR}/\${_script}\" _input)
      file(WRITE \"${CMAKE_CURRENT_BINARY_DIR}/\${_output}\" \"#!\${CMAKE_INSTALL_PREFIX}/bin/fontforge -lang=ff\\n\${_input}\")
    endforeach()
  " COMPONENT nativescripts)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fontimage" "${CMAKE_CURRENT_BINARY_DIR}/fontlint" "${CMAKE_CURRENT_BINARY_DIR}/sfddiff"
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT nativescripts
  )
else()
  install(FILES fontimage.pe fontlint.pe sfddiff.pe DESTINATION ${CMAKE_INSTALL_DATADIR}/fontforge/nativescripts COMPONENT nativescripts)
endif()
install(FILES fontforge.1 sfddiff.1 fontlint.1 fontimage.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

add_subdirectory(pixmaps)
