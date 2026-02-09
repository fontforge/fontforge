# FontForge Python Module / PyPI Distribution Plan

This document outlines the plan for making FontForge's Python bindings installable via `pip` and distributable through PyPI.

## Goals

1. Enable `pip install fontforge` from PyPI with pre-built wheels
2. Support source distribution (sdist) for users who can build from source
3. Maintain compatibility with distro packaging and full application builds
4. Use a single CMake codebase for all build scenarios

## Module Structure

Two top-level modules (historical, not changing):
- `fontforge` - Main module with font manipulation API
- `psMat` - PostScript matrix operations

## Build Scenarios

| Scenario | Detection | GUI | `fontforge.version()` | libfontforge Location | Module Location |
|----------|-----------|-----|----------------------|----------------------|-----------------|
| **Wheel** | `SKBUILD` defined | OFF | CalVer (`2025.10.9`) | Wheel root | Wheel root |
| **Distro module-only** | Manual cmake | OFF | CalVer (`2025.10.9`) | System libdir | site-packages |
| **App + module** | Manual cmake | ON | Numeric (`20251009`) | System libdir | site-packages |

The version format allows detection of context: CalVer = standalone module, Numeric = embedded in app.

## Dependencies

### Required (always linked)
| Library | Purpose |
|---------|---------|
| libxml2 | XML/SVG parsing |
| zlib | Compression |
| libiconv | Character encoding |

### Include in wheels (bundle where possible)
| Library | Purpose | Notes |
|---------|---------|-------|
| FreeType | Bitmap rasterization | Preferred; falls back to internal rasterizer if unavailable |
| libpng | PNG image support | Include |
| libjpeg | JPEG image support | Include |
| libtiff | TIFF image support | Include |
| libspiro | Spiro curve support | Include |
| woff2 | WOFF2 format support | Include |

### Not needed for wheels
| Library | Purpose | Notes |
|---------|---------|-------|
| GLIB | GObject, GIO | Only needed for GUI builds |
| Gettext/Intl | Internationalization | Only needed for GUI builds |
| Readline | CLI editing | Only needed for interactive scripting |

## Versioning

### Current System
- `PROJECT_VERSION = 20251009` (YYYYMMDD integer)
- Used for app builds and distro module installs

### Python/PyPI Version
- CalVer format: `2025.10.9` (YYYY.M.D, no leading zeros)
- Derived automatically from PROJECT_VERSION
- Used only for wheel builds

### Transformation (in CMake)
```cmake
string(SUBSTRING "${PROJECT_VERSION}" 0 4 _FF_YEAR)
string(SUBSTRING "${PROJECT_VERSION}" 4 2 _FF_MONTH)
string(SUBSTRING "${PROJECT_VERSION}" 6 2 _FF_DAY)
math(EXPR _FF_MONTH_INT "${_FF_MONTH}")
math(EXPR _FF_DAY_INT "${_FF_DAY}")
set(FONTFORGE_PYTHON_VERSION "${_FF_YEAR}.${_FF_MONTH_INT}.${_FF_DAY_INT}")
```

### Pre-release Versions
- Development: `2025.10.9.dev0`
- Alpha: `2025.10.9a1`
- Release candidate: `2025.10.9rc1`

## Build System

### pyproject.toml
```toml
[build-system]
requires = ["scikit-build-core>=0.5"]
build-backend = "scikit_build_core.build"

[project]
name = "fontforge"
dynamic = ["version"]
description = "A font editor and converter"
requires-python = ">=3.8"

[project.urls]
Homepage = "https://fontforge.org"
Documentation = "https://fontforge.org/docs/"
Repository = "https://github.com/fontforge/fontforge"

[tool.scikit-build]
cmake.args = [
    "-DENABLE_GUI=OFF",
    "-DENABLE_PYTHON_SCRIPTING=ON",
    "-DENABLE_PYTHON_EXTENSION=ON",
    "-DENABLE_DOCS=OFF",
    "-DBUILD_SHARED_LIBS=ON",
]
wheel.packages = []
wheel.install-dir = "."
cmake.build-type = "Release"

[tool.scikit-build.metadata]
version.provider = "scikit_build_core.metadata.cmake"
version.variable = "FONTFORGE_PYTHON_VERSION"
```

### CMake Changes

#### Root CMakeLists.txt additions
```cmake
# Detect wheel build (scikit-build-core sets SKBUILD)
if(DEFINED SKBUILD)
  set(BUILDING_WHEEL TRUE)
  message(STATUS "Building for Python wheel")
else()
  set(BUILDING_WHEEL FALSE)
endif()

# Generate Python-compatible version
string(SUBSTRING "${PROJECT_VERSION}" 0 4 _FF_YEAR)
string(SUBSTRING "${PROJECT_VERSION}" 4 2 _FF_MONTH)
string(SUBSTRING "${PROJECT_VERSION}" 6 2 _FF_DAY)
math(EXPR _FF_MONTH_INT "${_FF_MONTH}")
math(EXPR _FF_DAY_INT "${_FF_DAY}")
set(FONTFORGE_PYTHON_VERSION "${_FF_YEAR}.${_FF_MONTH_INT}.${_FF_DAY_INT}")
```

#### pyhook/CMakeLists.txt changes
```cmake
if(BUILDING_WHEEL)
  # Wheel: bundle everything together
  if(NOT WIN32)
    set_target_properties(fontforge_pyhook psMat_pyhook PROPERTIES
      INSTALL_RPATH "$ORIGIN"
      BUILD_WITH_INSTALL_RPATH TRUE
    )
  endif()
  install(TARGETS fontforge_pyhook psMat_pyhook LIBRARY DESTINATION ".")
else()
  # System install: use Python's site-packages
  # (existing sysconfig-based logic)
  install(TARGETS fontforge_pyhook psMat_pyhook LIBRARY DESTINATION "${PYHOOK_INSTALL_DIR}")
endif()
```

#### fontforge/CMakeLists.txt changes
```cmake
if(BUILD_SHARED_LIBS)
  if(BUILDING_WHEEL)
    # Wheel: install alongside Python modules
    install(TARGETS fontforge RUNTIME DESTINATION "." LIBRARY DESTINATION ".")
  else()
    # System install: standard lib directory
    install(TARGETS fontforge RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
  endif()
endif()
```

## CI / Wheel Building

### cibuildwheel configuration
```toml
[tool.cibuildwheel]
# Skip PyPy, 32-bit, and musllinux
skip = ["pp*", "*-win32", "*-manylinux_i686", "*-musllinux*"]

# Run full test suite
test-command = "ctest --test-dir {package}/build --output-on-failure"

[tool.cibuildwheel.linux]
manylinux-x86_64-image = "manylinux_2_28"
before-all = [
    "yum install -y freetype-devel libxml2-devel zlib-devel libpng-devel libjpeg-devel libtiff-devel",
]
repair-wheel-command = "auditwheel repair -w {dest_dir} {wheel}"

[tool.cibuildwheel.macos]
archs = ["x86_64", "arm64"]  # Separate wheels, not universal2
before-all = ["brew install freetype libxml2 libpng jpeg libtiff libspiro woff2"]
repair-wheel-command = "delocate-wheel -w {dest_dir} {wheel}"

[tool.cibuildwheel.windows]
archs = ["AMD64"]
before-build = "pip install delvewheel"
repair-wheel-command = "delvewheel repair -w {dest_dir} {wheel}"
```

### Dependency bundling
- **Linux**: `auditwheel repair` bundles .so files, creates manylinux wheel
- **macOS**: `delocate-wheel` bundles dylibs, fixes @rpath
- **Windows**: `delvewheel` bundles DLLs (vcpkg provides build-time deps)

### Target platforms
- Linux: x86_64 (manylinux_2_28, based on RHEL 8)
- macOS: x86_64 and arm64 (separate wheels)
- Windows: x64 (MSVC via vcpkg)

### Python versions
- 3.8, 3.9, 3.10, 3.11, 3.12 (and newer as released)

## Source Distribution (sdist)

An sdist allows users to build from source when no wheel matches their platform.

Requirements for the user:
- C/C++ compiler
- CMake
- Required library development packages (libxml2-dev, zlib-dev, etc.)
- Python development headers

The sdist will contain the full source needed for a module-only build.

## Documentation

Existing Python docs: `doc/sphinx/scripting/python/` (5,300+ lines in Sphinx RST format)

### Approach
- Primary docs remain at fontforge.org
- Link from PyPI via `[project.urls]` Documentation field
- Consider also publishing Python subset to ReadTheDocs for discoverability
- PyPI landing page uses a Python-focused README (TBD)

### PyPI URLs
```toml
[project.urls]
Homepage = "https://fontforge.org"
Documentation = "https://fontforge.org/docs/scripting/python.html"
Repository = "https://github.com/fontforge/fontforge"
Issues = "https://github.com/fontforge/fontforge/issues"
```

## Release Process

### Trigger
- Git tag push matching CalVer pattern (e.g., `v2025.10.9`)
- Manual workflow dispatch for testing

### Workflow (afdko-style)
1. Tag triggers wheel build workflow
2. cibuildwheel builds wheels for all platforms
3. Run full test suite on each wheel
4. Build sdist on Linux
5. Upload all artifacts
6. Publish to PyPI via `pypa/gh-action-pypi-publish`
7. Create GitHub Release with changelog

### Pre-release versions
- Development: tag with `.dev0` suffix → `v2025.10.9.dev0`
- Alpha: tag with `a1` suffix → `v2025.10.9a1`
- Release candidate: tag with `rc1` suffix → `v2025.10.9rc1`

## Licensing

- FontForge is GPL-3.0-or-later for code that requires it
- BSD-3-Clause for components that allow it
- Verify all bundled dependencies are compatible with redistribution

## Implementation Steps

1. [x] Add `BUILDING_WHEEL` detection to root CMakeLists.txt
2. [x] Add Python version generation to CMakeLists.txt
3. [x] Modify pyhook/CMakeLists.txt for wheel install layout
4. [x] Modify fontforge/CMakeLists.txt for wheel install layout
5. [x] Create pyproject.toml with scikit-build-core and cibuildwheel config
6. [x] Create custom version provider (`_build_meta/version_provider.py`)
7. [x] Test local wheel build with `pip wheel .`
8. [x] Set up cibuildwheel in GitHub Actions (`wheels.yml`)
9. [x] Test CI wheel builds on all platforms (Linux, macOS, Windows)
10. [x] Create README_PYPI.md for PyPI landing page
11. [x] Add ReadTheDocs configuration (`.readthedocs.yaml`)
12. [x] Implement context-aware version: CalVer for pyhook, numeric for app
13. [x] Create dist-info on app install for pip compatibility
14. [x] Add install conflict detection (skip if pip version detected)
15. [ ] Configure PyPI trusted publishing
16. [ ] Create and test sdist build
17. [ ] Initial PyPI release

## Version Behavior

`fontforge.version()` returns different formats depending on context:

| Context | Returns | Example |
|---------|---------|---------|
| Standalone module (pyhook) | CalVer | `2025.10.9` |
| Embedded in app | Numeric | `20251009` |

Detection uses `ff_is_pyhook_context()` function defined in:
- `pyhook/fontforgepyhook.c` → returns 1
- `fontforgeexe/main.c` → returns 0

## Install Conflict Handling

### App install over pip-installed module
- Detects CalVer format in existing `fontforge.version()`
- Skips module installation, prints message suggesting `pip uninstall fontforge`

### App install (fresh or over previous app install)
- Installs modules to site-packages
- Creates `fontforge-{version}.dist-info/` with:
  - `METADATA` - package metadata
  - `INSTALLER` - set to `fontforge-app`
  - `RECORD` - list of installed files
  - `top_level.txt` - lists `fontforge` and `psMat`
- This allows `pip list` to show the install and `pip uninstall` to work

### pip install over app-installed module
- pip sees existing dist-info with matching CalVer version
- Performs proper upgrade/reinstall

## Resolved Decisions

- FreeType API check (`hasFreeType()`): Not needed for now
- Optional dependencies: Include all (freetype, libpng, libjpeg, libtiff, libspiro, woff2)
- Wheel variants: Single "full" variant only
- manylinux version: manylinux_2_28 (same as afdko)
- macOS architectures: Separate x86_64 and arm64 wheels
- macOS deployment target: 11.0 (required for arm64)
- Release trigger: Git tags
- Test suite: Full tests run in CI for wheels
- PyPI README: Created `README_PYPI.md` with wheel-specific docs
- ReadTheDocs: Configured for versioned docs, version from `READTHEDOCS_VERSION` env

## Open Questions

- sdist contents: What to exclude? (TBD)
- ReadTheDocs project name: Need to set up at readthedocs.org
