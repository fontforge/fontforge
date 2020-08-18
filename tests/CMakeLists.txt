# Distributed under the original FontForge BSD 3-clause license

# Note that you should invoke the `check` target instead of
# the `test` target so that test-specific prerequisites are checked.

include(TestUtils)

if(NOT ENABLE_NATIVE_SCRIPTING AND NOT ENABLE_PYTHON_SCRIPTING_RESULT)
  return()
endif()

add_executable(systestdriver systestdriver.c)
target_link_libraries(systestdriver PRIVATE GLIB::GLIB)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(systestdriver PRIVATE -Wall -pedantic)
endif()

# Downloaded files
add_download_target("NotoSans-Regular.ttc" "https://github.com/fontforge/debugfonts/raw/master/NotoSans-Regular.ttc")
add_download_target("MunhwaGothic-Bold" "https://github.com/fontforge/debugfonts/raw/master/MunhwaGothic-Bold")

add_custom_target(test_dependencies
  DEPENDS
  "${CMAKE_CURRENT_BINARY_DIR}/fonts/NotoSans-Regular.ttc"
  "${CMAKE_CURRENT_BINARY_DIR}/fonts/MunhwaGothic-Bold"
)

add_custom_target(check
  COMMAND "${CMAKE_CTEST_COMMAND}" "--output-on-failure" "--force-new-ctest-process"
  DEPENDS
    test_dependencies
    fontforgeexe
    systestdriver
  VERBATIM
  USES_TERMINAL
)

if(ENABLE_PYTHON_SCRIPTING_RESULT AND ENABLE_PYTHON_EXTENSION_RESULT)
  add_dependencies(check fontforge_pyhook psMat_pyhook)
endif()

if(ENABLE_NATIVE_SCRIPTING)
  add_ff_test(test001.pe "Caliban.sfd" "Zapfino-4.0d3.dfont" "Zapfino-4.1d6.dfont" "Generate a few font files")
  add_ff_test(test002.pe "HanSans-Bold.otf"                                        "OTF CID font check")
  add_ff_test(test003.pe "MunhwaGothic-Bold"                                       "Non-OTF CID font check")
  add_ff_test(test004.pe "mingliu.ttc"                                             "Chunk of memory check")
  add_ff_test(test005.pe "VKern.sfd"                                               "Kerning information checks")
  add_ff_test(test006.pe "mingliu.ttc" "GillSans.dfont" "ItalicHand.sfd" "ItalicHand.svg" "MunhwaGothic-Bold" "Caliban.otf" "Incorrect names and count checks")
  add_ff_test(test007.pe "FormalScript.sfd" "dvng10a-crash.sfd"                    "Contextual chaining tables don't crash")
  add_ff_test(test008.pe "n019003l.pfb"                                            "Contextual chaining tables don't crash")
  add_ff_test(test009.pe "Ambrosia.sfd"                                            "Read and generate Type3 fonts")
  add_ff_test(test010.pe "GillSans.dfont"                                          "Generate some error messages")
  add_ff_test(test101.pe                                                           "Check for unexpected")
  add_ff_test(test102.pe "AHBugs.sfd"                                              "Infinite loop test on 0 length spline")
  add_ff_test(test103.pe                                                           "Copy/Paste clipboard test for crash")
  add_ff_test(test104.pe "SplineOverlapBug1.sfd"                                   "Remove overlap test #1")
  add_ff_test(test105.pe                                                           "while loop check  loop 10x")
  add_ff_test(test106.pe "OmittedCharBugs.sfd"                                     "Simple 'Bad Kerning' check")
  add_ff_test(test107.pe "Ambrosia.sfd" "AmbrosiaBold.sfd" "AmbrosiaItalic.sfd"    "Generate a font family") # also calls 'helper107.pe'
  add_ff_test(test108.pe "ItalicHand.sfd"                                          "Make GPOS  GSUB  morx without crashing")
  add_ff_test(test109.pe "OverlapBugs.sfd" "feta20.pfb"                            "Remove overlap test #2")
  add_ff_test(test110.pe "SimplifyBugs.sfd"                                        "Test simplify")
  add_ff_test(test111.pe "cmbsy10.pfb"                                             "Test merge fonts")
  add_ff_test(test112.pe "CourierNew.ttf"                                          "Test quadratic conversion")
  add_ff_test(test113.pe "CourierNew.ttf"                                          "Count contour points properly")
  add_ff_test(test114.pe "CaslonMM.sfd"                                            "Generate  save and open a .pfa file")
  add_ff_test(test115.pe "n019003l.pfb"                                            "Replace with reference")
  add_ff_test(test116.pe "merged.cff"                                              "Merged")
  add_ff_test(test117.pe "ecrm1000.pfb" "ecrm1000.tfm"                             "Merge kern")
  add_ff_test(test118.pe "NimbusLGCUni-Regular.sfd"                                "Test Unicode encodings") # also uses helper118A.pe helper118B.pe
  add_ff_test(test119.pe "DejaVuSerif.sfd"                                         "The 'i' glyph test")
  add_ff_test(test120.pe "NumberPoints.sfd"                                        "Another deja-vu point number")
  add_ff_test(test121.pe "AddExtremumTest.sfd"                                     "Don't distort the curve (by much)")
  #add_ff_test(test122.pe "Hinting.sfd"                                            "Check AutoHint works and retains hints")
  add_ff_test(test123.pe "MinionPro-Regular.otf"                                   "Produce equivalent font in CFF check")
  add_ff_test(test124.pe                                                           "Check that detach glyph works #1")
  add_ff_test(test125.pe "MinionPro-Regular.fea"                                   "Check that detach glyph works #2")
  add_ff_test(test126.pe "DataURI.sfd"                                             "Try SVG and PT3")
  add_ff_test(test127.pe "Ambrosia.sfd"                                            "Test generation of TrueType instructions")
  #add_ff_test(svg2ttf.pe "")
  add_ff_test(test128.pe                                                           "Test StrSplit overrun")
  add_ff_test(test129.pe                                                           "Ligatures and Fractions Table Lookups")
  add_ff_test(test130.pe "NotoSans-Regular.ttc"                                    "TTC loading")
  add_ff_test(test131.pe                                                           "libnameslist checks")
  if(ENABLE_WOFF2_RESULT)
    add_ff_test(test132.pe "CantarellMin.woff2"                                    "WOFF2 round tripping")
  endif()
  add_ff_test(test133.pe "test133.fea"                                             "Allowed characters in glyph class names")
  add_ff_test(test134.pe "test134.fea"                                             "Short-hand contextual multiple substitution")
  add_ff_test(test135.pe "test135.fea"                                             "GDEF glyph classes")
  add_ff_test(test136.pe "test136.fea"                                             "Re-using mark filtering set")
  add_ff_test(test137.pe "Ambrosia.sfd"                                            "file:// protocol")
  add_ff_test(test138.pe                                                           "Array sanity checking")
  add_ff_test(test139.pe "StrokeTests.sfd"                                            "ExpandStroke parameters")
endif()

if(ENABLE_PYTHON_SCRIPTING_RESULT)
  add_py_test(test0001.py "Ambrosia.sfd" "FontForge .sfd file open check")
  add_py_test(test0002.py "AGL For New Fonts.txt" "Loading duplicate name list")
  add_py_test(test0003.py "BoxesAndBounds.sfd" "Bounding box sanity tests")
  add_py_test(test0004.py "OpenType script tag tests")
  add_py_test(test0101.py "NotoSans-Regular.ttc" "FontForge .ttc file open check")
  add_py_test(test1001.py "Caliban.sfd" "Generate several font files")
  add_py_test(test1001a.py "Zapfino-4.0d3.dfont" "Point count check")
  add_py_test(test1001b.py "Zapfino-4.1d6.dfont" "Odd morx test without crashing")
  add_py_test(test1001c.py "Make new font")
  add_py_test(test1002.py "nuvo-medium-woff-demo.woff" "WOFF major minor meta check")
  add_py_test(test1003.py "StrokeTests.sfd" "Various stroke tests")
  add_py_test(test1004.py "DirectionTest.sfd" "Clockwise direction test")
  add_py_test(test1005.py "AddExtremaTest2.sfd" "Generate duplicate fonts test")
  add_py_test(test1006.py "Math table test")
  add_py_test(test1007.py "ayn+meem.init.svg" "Check for splinestroke 0 len fail")
  add_py_test(test1008.py "OverlapBugs.sfd" "PythonAPI: font glyph iteration")
  add_py_test(test1009.py "Ligatures and Fractions Table Lookups")
  add_py_test(test1010.py "libnameslist checks")
  add_py_test(test1011.py "test1011.fea" "Feature file kerning with r2l lookup flag")
  add_py_test(test1012.py "test1012.otf" "GPOS kerning with r2l lookup flag")
  add_py_test(test1013.py "NotoSerifTibetan-Regular.ttf" "Read glyph names from unusual post table")
  add_py_test(test1014.py "PfEd layers round tripping")
  add_py_test(test1015.py "Caliban.sfd" "Reverse chaining tables")
  #add_py_test(findoverlapbugs.py "find overlap bug")
  add_py_test(test926.py "DejaVuSerif.sfd" "Validate WOFF output")
  if(ENABLE_WOFF2_RESULT)
    add_py_test(test927.py "CantarellMin.woff2" "WOFF2 round tripping")
  endif()
  add_py_test(test928.py "DejaVuSerif.sfd" "String Conversion")
  add_py_test(test929.py "NotoSans-Regular.ttc" "Open Subfonts")
  add_py_test(test930.py "Point, contour, and Layer fidelity")
  add_py_test(test931.py "Point type conversion tests")
  add_py_test(test932.py "Round-trip point selection tests")
endif()
