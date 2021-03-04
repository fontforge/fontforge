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
  crctab.c
  cvexport.c
  cvimages.c
  cvundoes.c
  dumpbdf.c
  dumppfa.c
  effects.c
  encoding.c
  featurefile.c
  flaglist.c
  fontviewbase.c
  freetype.c
  ftdelta.c
  fvcomposite.c
  fvfonts.c
  fvimportbdf.c
  fvmetrics.c
  glif_name_hash.c
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
  plugin.c
  print.c
  psread.c
  pua.c
  python.c
  savefont.c
  scripting.c
  scstyles.c
  search.c
  sfd.c
  sfd1.c
  sflayout.c
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

if(ENABLE_WOFF2_RESULT)
  target_sources(fontforge PRIVATE woff2.cc)
endif()

# Turn up warnings on a few source files
set_property(SOURCE splineoverlap.c APPEND PROPERTY COMPILE_OPTIONS ${FONTFORGE_EXTRA_CFLAGS})

set_property(TARGET fontforge PROPERTY VERSION 4)
if(BUILD_SHARED_LIBS)
  if(WIN32 OR CYGWIN)
    target_link_options(fontforge PRIVATE -Wl,--export-all-symbols) # FIXME
  endif()
endif()

# This is pretty bad...
target_include_directories(fontforge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(fontforge
  PUBLIC
    fontforge_common_headers
    Freetype::Freetype
    LibXml2::LibXml2
    GLIB::GLIB
    Intl::Intl
  PRIVATE
    MathLib::MathLib
    Iconv::Iconv
    ZLIB::ZLIB
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
  target_link_libraries(fontforge PRIVATE Python3::Python)
endif()
if(ENABLE_WOFF2_RESULT)
  target_link_libraries(fontforge PRIVATE WOFF2::WOFF2)
endif()

if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
  target_link_libraries(fontforge PRIVATE gunicode_interface gutils_interface)
else()
  target_link_libraries(fontforge PRIVATE gunicode gutils)
endif()

# No dev package -> no need to install if static
if(BUILD_SHARED_LIBS)
  if(WIN32 OR CYGWIN)
    install(TARGETS fontforge RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
  else()
    install(TARGETS fontforge RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
  endif()
endif()
