# Distributed under the original FontForge BSD 3-clause license

set(FONTFORGE_NOINST_HEADERS
  asmfpst.h
  autohint.h
  autosave.h
  autotrace.h
  autowidth.h
  autowidth2.h
  bitmapchar.h
  bitmapcontrol.h
  bvedit.h
  clipnoui.h
  crctab.h
  cvexport.h
  cvimages.h
  cvundoes.h
  dumpbdf.h
  dumppfa.h
  effects.h
  encoding.h
  featurefile.h
  fontforgeui.h
  fvcomposite.h
  fvfonts.h
  fvimportbdf.h
  ikarus.h
  langfreq.h
  macbinary.h
  macenc.h
  mathconstants.h
  multidialog.h
  namelist.h
  othersubrs.h
  palmfonts.h
  parsepdf.h
  parsepfa.h
  parsettf.h
  parsettfatt.h
  parsettfbmf.h
  parsettfvar.h
  plugin.h
  psread.h
  pua.h
  savefont.h
  scstyles.h
  sfd.h
  spiro.h
  splinefill.h
  splinefit.h
  splineorder2.h
  splineoverlap.h
  splinerefigure.h
  splinesave.h
  splinesaveafm.h
  splinestroke.h
  splineutil.h
  splineutil2.h
  start.h
  std_maps.hpp
  svg.h
  tottf.h
  tottfaat.h
  tottfgpos.h
  tottfvar.h
  ttfspecial.h
  utanvec.h
  winfonts.h
  woff.h
  zapfnomen.h
)

set(FONTFORGE_INST_HEADERS
  autowidth.h
  autowidth2.h
  baseviews.h
  bezctx_ff.h
  bitmapcontrol.h
  delta.h
  edgelist.h
  edgelist2.h
  encoding.h
  fffreetype.h
  ffpython.h
  flaglist.h
  fontforge.h
  fontforgevw.h
  fvmetrics.h
  getline.h
  glif_name_hash.h
  glyphcomp.h
  groups.h
  lookups.h
  mem.h
  mm.h
  namehash.h
  nonlineartrans.h
  ofl.h
  PfEd.h
  print.h
  psfont.h
  savefont.h
  scriptfuncs.h
  scripting.h
  sd.h
  search.h
  sfd1.h
  sflayoutP.h
  splinefont.h
  stemdb.h
  ttf.h
  ttfinstrs.h
  uiinterface.h
  unicoderange.h
  views.h
)

add_library(fontforge
  ${FONTFORGE_NOINST_HEADERS}
  ${FONTFORGE_INST_HEADERS}
  activeinui.c
  ffglib_compat.cpp
  asmfpst.c
  autohint.c
  autosave.c
  autotrace.c
  autowidth.c
  autowidth2.c
  bezctx_ff.c
  bitmapchar.c
  bitmapcontrol.c
  bvedit.c
  clipnoui.c
  crctab.cpp
  cvexport.c
  cvimages.c
  cvundoes.c
  dumpbdf.c
  dumppfa.c
  effects.c
  encoding.c
  featurefile.c
  ffprocess.c
  flaglist.c
  fontviewbase.c
  freetype.c
  ftdelta.c
  fvcomposite.c
  fvfonts.c
  fvimportbdf.c
  fvmetrics.c
  getline.c
  glif_name_hash.cpp
  glyphcomp.c
  groups.c
  ikarus.c
  langfreq.c
  lookups.c
  macbinary.c
  macenc.c
  mathconstants.c
  mem.c
  mm.c
  namelist.c
  nonlineartrans.c
  noprefs.c
  nouiutil.c
  nowakowskittfinstr.c
  ofl.c
  othersubrs.c
  palmfonts.c
  parsepdf.c
  parsepfa.c
  parsettf.c
  parsettfatt.c
  parsettfbmf.c
  parsettfvar.c
  plugin.cpp
  print.cpp
  psread.c
  pua.c
  python.cpp
  savefont.c
  scripting.cpp
  scstyles.c
  search.c
  sfd.cpp
  sfd1.c
  sflayout.cpp
  spiro.c
  splinechar.c
  splinefill.c
  splinefit.c
  splinefont.c
  splineorder2.c
  splineoverlap.c
  splinerefigure.c
  splinesave.c
  splinesaveafm.c
  splinestroke.c
  splineutil.c
  splineutil2.c
  start.c
  stemdb.c
  std_maps.cpp
  svg.c
  tottf.c
  tottfaat.c
  tottfgpos.c
  tottfvar.c
  ttfinstrs.c
  ttfspecial.c
  ufo.c
  unicoderange.c
  utanvec.c
  winfonts.c
  woff.c
  zapfnomen.c
)

add_subdirectory(shapers)

if(ENABLE_WOFF2_RESULT)
  target_sources(fontforge PRIVATE woff2.cc)
endif()

# Turn up warnings on a few source files
set_property(SOURCE splineoverlap.c APPEND PROPERTY COMPILE_OPTIONS ${FONTFORGE_EXTRA_CFLAGS})

set_property(TARGET fontforge PROPERTY VERSION 4)
if(BUILD_SHARED_LIBS)
  if(WIN32 OR CYGWIN)
    # Both fontforge.exe and *.pyd Python modules look for libfontforge.dll in
    # their directory, so we need to produce a second copy.
    add_custom_command(TARGET fontforge POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:fontforge> ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    )
    target_link_options(fontforge PRIVATE -Wl,--export-all-symbols) # FIXME
  endif()
endif()

# This is pretty bad...
target_include_directories(fontforge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Export all symbols from the DLL on Windows (mimics MinGW's --export-all-symbols)
set_target_properties(fontforge PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

target_link_libraries(fontforge
  PUBLIC
    fontforge_common_headers
    shapers
    Freetype::Freetype
    LibXml2::LibXml2
    $<$<BOOL:${Intl_FOUND}>:Intl::Intl>
  PRIVATE
    MathLib::MathLib
    Iconv::Iconv
    ZLIB::ZLIB
    mINI
)

if(APPLE)
  target_link_libraries(fontforge
    PRIVATE
    "-framework CoreServices"
  )
endif()

if(ENABLE_FREETYPE_DEBUGGER)
  target_link_libraries(fontforge PUBLIC FreeTypeSource::FreeTypeSource)
endif()
if(ENABLE_LIBSPIRO_RESULT)
  target_link_libraries(fontforge PUBLIC Libspiro::Libspiro)
endif()
if(ENABLE_LIBREADLINE_RESULT)
  target_link_libraries(fontforge PUBLIC Readline::Readline)
endif()
if(ENABLE_PYTHON_SCRIPTING_RESULT)
  if(APPLE)
    # On macOS, don't link libfontforge directly to Python library.
    # The Python symbols should be resolved at runtime from the interpreter
    # that loads the extension module. Use -undefined dynamic_lookup to allow
    # undefined symbols that will be resolved when the extension is loaded.
    target_include_directories(fontforge PRIVATE ${Python3_INCLUDE_DIRS})
    target_link_options(fontforge PRIVATE -undefined dynamic_lookup)
  else()
    target_link_libraries(fontforge PRIVATE Python3::Python)
  endif()
endif()
if(ENABLE_WOFF2_RESULT)
  target_link_libraries(fontforge PRIVATE WOFF2::WOFF2)
endif()
if(ENABLE_LIBUNIBREAK_RESULT)
  target_link_libraries(fontforge PRIVATE Libunibreak::Libunibreak)
endif()

if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
  target_link_libraries(fontforge PRIVATE gunicode_interface gutils_interface)
else()
  target_link_libraries(fontforge PRIVATE gunicode gutils)
endif()

# No dev package -> no need to install if static
if(BUILD_SHARED_LIBS)
  if(BUILDING_WHEEL)
    # Wheel: install alongside Python modules for bundling
    install(TARGETS fontforge
      RUNTIME DESTINATION "."
      LIBRARY DESTINATION "."
    )
  elseif(WIN32 OR CYGWIN)
    install(TARGETS fontforge RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
  else()
    install(TARGETS fontforge RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
  endif()
endif()
