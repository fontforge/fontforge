# Distributed under the original FontForge BSD 3-clause license

if(ENABLE_MAINTAINER_TOOLS)
  add_executable(mapcns1 src/mapcns1.c)
  add_executable(mapgb1 src/mapgb1.c)
  add_executable(mapjapan1 src/mapjapan1.c)
  add_executable(mapjapan2 src/mapjapan2.c)
  add_executable(mapkorean src/mapkorean.c)

  target_link_libraries(mapcns1 PRIVATE fontforge)
  target_link_libraries(mapgb1 PRIVATE fontforge)
  target_link_libraries(mapjapan1 PRIVATE fontforge)
  target_link_libraries(mapjapan2 PRIVATE fontforge)
  target_link_libraries(mapkorean PRIVATE fontforge)
endif()

install(
  FILES
    Adobe-CNS1-6.cidmap
    Adobe-GB1-5.cidmap
    Adobe-Japan1-5.cidmap
    Adobe-Japan1-6.cidmap
    Adobe-Japan1-7.cidmap
    Adobe-Japan2-0.cidmap
    Adobe-Korea1-2.cidmap
  DESTINATION ${CMAKE_INSTALL_DATADIR}/fontforge/cidmap
  COMPONENT cidmaps
)
