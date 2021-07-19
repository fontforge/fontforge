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

file(GLOB_RECURSE cidmaps *.cidmap)

install(
    FILES ${cidmaps}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/fontforge/cidmap
)
