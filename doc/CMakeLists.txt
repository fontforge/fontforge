# Distributed under the original FontForge BSD 3-clause license

# If you add or remove files from the sphinx folder, run the docs-manifest
# target to regenerate the manifest. This is so that CMake can track the
# dependencies correctly, so it knows when to rerun sphinx.

# CAVEAT:
# sphinx runs its own dependency tracking so it does not regenerate
# everything if already run before. But its dependency tracking is not
# perfect, so if you are moving files around or renaming documents, it is
# best to manually delete the output folder and rerun from scratch.

add_custom_target(docs-manifest
  "${CMAKE_COMMAND}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/scripts/GenerateDocsManifest.cmake"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Read dependencies
file(STRINGS manifest.txt _manifest)
configure_file(manifest.txt manifest.txt COPYONLY)

add_custom_command(OUTPUT sphinx-docs
  COMMAND "${Sphinx_BUILD_BINARY}" -q -n -W -b html -d doctrees "${CMAKE_CURRENT_SOURCE_DIR}/sphinx" sphinx-docs
  DEPENDS ${_manifest}
)

add_custom_target(docs ALL DEPENDS sphinx-docs)
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/sphinx-docs/"
  DESTINATION ${CMAKE_INSTALL_DOCDIR}
  COMPONENT docs
)
