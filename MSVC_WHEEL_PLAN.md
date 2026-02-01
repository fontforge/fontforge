# FontForge Python Module PyPi Distribution Plan

## Problem Statement

FontForge's Python modules (`fontforge.pyd`, `psMat.pyd`) are built with MinGW on Windows, but Python.org's Windows Python is compiled with MSVC. These have incompatible ABIs, preventing FontForge's Python modules from working with standard Windows Python installations.

## Root Cause Analysis

| Build | Compiler | ABI |
|-------|----------|-----|
| FontForge Windows | MinGW (GCC) | GCC/MinGW |
| Python.org Windows | MSVC | MSVC |

Python C extensions must match the Python interpreter's compiler ABI.

## Chosen Approach

**Minimal first release** with **parallel builds**:
- Keep MinGW build for full GUI application (existing workflow)
- Add new MSVC build specifically for Python wheel distribution
- Target Python 3.11 on Windows x64 initially
- Focus on Python.org builds + embedded Python use cases
- Iterate based on feedback

### Rationale

1. **Only reliable solution** for ABI compatibility
2. **Industry standard** - how Pillow, NumPy handle Windows wheels
3. **Reduced scope** - no GTK/GDK dependencies needed
4. **vcpkg support** - most dependencies available (FreeType, LibXml2, GLIB, zlib)
5. **Lower risk** - existing MinGW build continues working

---

## Implementation Details

### 1. MSVC Compatibility Fixes

**Files requiring changes**:
- `inc/basics.h` - Add `_MSC_VER` alternative for `__GNUC__` UNUSED macro
- `inc/ustring.h` - Add MSVC alternatives for format string attributes
- `fontforge/CMakeLists.txt` - Add MSVC compile options and export declarations

**Key pattern to fix**:
```c
// Before (GCC only)
#define UNUSED(x) UNUSED_ ## x __attribute__((unused))

// After (portable)
#ifdef _MSC_VER
# define UNUSED(x) x
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#else
# define UNUSED(x) x
#endif
```

**DLL exports**: Current MinGW build uses `--export-all-symbols`. MSVC requires explicit `__declspec(dllexport)` declarations or a DEF file. Options:
1. Create `fontforge.def` with exported symbols
2. Add `FF_API` macro to public functions

### 2. CMake Updates

**MSVC compiler flags**:
```cmake
if(MSVC)
  add_compile_options(/W4 /WX- /utf-8)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()
```

**New build option for wheel-only builds**:
```cmake
build_option(BUILD_PYTHON_WHEEL_ONLY BOOL OFF
  "Build only for Python wheel distribution (no GUI)")
```

### 3. CI Configuration

**Add to `.github/workflows/main.yml`** (new job):
```yaml
windows-msvc:
  runs-on: windows-latest
  steps:
    - uses: actions/checkout@v4
    - uses: lukka/run-vcpkg@v11
      with:
        vcpkgJsonGlob: 'vcpkg.json'
    - uses: ilammy/msvc-dev-cmd@v1
    - name: Configure
      run: |
        cmake -B build -G Ninja ^
          -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake ^
          -DENABLE_GUI=OFF ^
          -DENABLE_PYTHON_SCRIPTING=ON
    - name: Build
      run: cmake --build build
```

### 4. vcpkg Manifest

**Create `vcpkg.json`**:
```json
{
  "name": "fontforge",
  "version-string": "20251009",
  "dependencies": [
    "freetype",
    "libxml2",
    "glib",
    "gettext",
    "libiconv",
    "zlib"
  ]
}
```

Note: Start with minimal dependencies; add libspiro, woff2, etc. as needed.

---

## Testing Strategy

1. **Smoke tests**: Extend `tests/pyhook_smoketest.py`
2. **CI verification**: Add Windows MSVC build job to `main.yml`
3. **TestPyPI**: Publish test releases for community testing

---

## Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| GLIB Windows issues | High | Test early; may need feature flags |
| Complex dual build maintenance | Medium | Clear docs; consider full MSVC migration |
| Subtle ABI issues | High | Extensive testing; beta period |
| DLL conflicts | Medium | Use delvewheel for DLL bundling |

---

## Initial Target

- **Platform**: Windows x64
- **Python**: 3.11 (single version to start)
- **Use cases**: Python.org builds, embedded Python

---

## Execution Plan

### Step 1: Audit MSVC Compatibility in CI
Add a Windows MSVC build job to `main.yml` to identify compilation errors iteratively.

### Step 2: Fix Critical Compiler Issues
Address errors identified in CI, prioritizing:
1. GCC-specific macros (`__attribute__`, format attributes)
2. DLL export declarations
3. POSIX header alternatives

### Step 3: vcpkg Integration
Set up dependency management with vcpkg manifest.

### Step 4: Manual Wheel Build
Create working wheel manually before automating with cibuildwheel.

### Step 5: Iterate
Add more Python versions and platforms based on success.
