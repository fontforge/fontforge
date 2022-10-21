# Distributed under the original FontForge BSD 3-clause license

build_option(INSTALL_IDENTITY_CIDMAP BOOL OFF "Install Adobe-Identity-0.cidmap")
build_option(CIDMAP_DIR STRING "${CMAKE_INSTALL_DATADIR}/fontforge" "Adobe CIDMap files directory")

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

set(
  CIDMAPS
    Adobe-CNS1-6.cidmap
    Adobe-GB1-5.cidmap
    Adobe-Identity-0.cidmap
    Adobe-Japan1-5.cidmap
    Adobe-Japan1-6.cidmap
    Adobe-Japan1-7.cidmap
    Adobe-Japan2-0.cidmap
    Adobe-Korea1-2.cidmap
)

if (NOT INSTALL_IDENTITY_CIDMAP)
  list(REMOVE_ITEM CIDMAPS Adobe-Identity-0.cidmap)
endif()

install(
  FILES ${CIDMAPS}
  DESTINATION "${CIDMAP_DIR}"
  COMPONENT cidmaps
)

if(NOT INSTALL_IDENTITY_CIDMAP)
  install(
    CODE "
      include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/RmIdentity0.cmake)
      remove_identity_0_cidmap(${CIDMAP_DIR})
    "
    COMPONENT cidmaps
  )
endif()
