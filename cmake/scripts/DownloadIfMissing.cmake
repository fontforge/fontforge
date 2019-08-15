# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
DownloadIfMissing
-----------------

A small helper script to download or copy a file to ``DEST`` if it does
not exist. If it is found in ``SOURCE``, it is copied to ``DEST``.
Otherwise, the file is downloaded from ``URL`.

Used to download some supplementary fonts for the test suite.

#]=======================================================================]

if(NOT EXISTS "${DEST}")
  if(EXISTS "${SOURCE}")
    message(STATUS "Copying ${SOURCE} to ${DEST}...")
    configure_file("${SOURCE}" "${DEST}" COPYONLY)
  else()
    message(STATUS "Fetching ${URL} to ${DEST}...")
    file(DOWNLOAD "${URL}" "${DEST}")
  endif()
endif()
