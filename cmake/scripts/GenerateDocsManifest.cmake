# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
GenerateDocsManifest
--------------------

Generates the manifest of dependencies for the documentation.

#]=======================================================================]

file(GLOB_RECURSE _manifest_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" sphinx/*)
list(FILTER _manifest_files EXCLUDE REGEX "(^.*\\.gitignore)|(^.*\\.py[co])")
string(REGEX REPLACE ";" "\n" _manifest_files "${_manifest_files}")
file(WRITE manifest.txt ${_manifest_files})
