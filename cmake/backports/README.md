Backports
=========

These are modules that have been copied near 1:1 from newer CMake modules (3.15.7 at the time of writing) so that there is consistency in usage of these modules on older versions of CMake.

Unless otherwise stated, the only change has been to change `include` statements from file includes to module includes.

## FindGIF
Provides an imported target from 3.14 and later

## FindIconv
Introduced in 3.11

## FindJPEG
Provides an imported target from 3.12 and later

## FindLibXml2
Provides an imported target from 3.12 and later

## FindPkgConfig
`pkg_check_modules` supports generating imported targets from 3.6 and later

## FindPython3
Introduced in 3.12. Replaces `FindPythonInterp` and `FindPythonLibs`. Provides an imported target.

Minor changes in `Support.cmake` to not use `VERSION_GREATER_EQUAL` and to set `_Python3_CMAKE_ROLE` to `PROJECT` (as `CMAKE_ROLE` was only introduced with 3.14)

From 3.15 an later, provides a Python3::Module imported target for use with linking modules.

## FindX11
Provides imported targets from 3.14 and later
