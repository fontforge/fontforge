# Windows Build Scripts

This directory contains scripts for building FontForge on Windows using MSYS2/MinGW-w64.

## Directory Structure

- `ffbuild.sh` - Main build script that installs dependencies and compiles FontForge
- `make-portable-package.sh` - Creates portable 7z archives
- `strip-python.sh` - Prepares a stripped-down Python distribution for bundling
- `launcher/` - Source for the `run_fontforge.exe` wrapper and icons
- `runtime/` - Template files copied into the release package (batch scripts, fontconfig)
- `setup/` - Inno Setup installer configuration
- `debug/` - Debug utilities (GDB scripts)

## Building Locally

Requirements: MSYS2 with UCRT64 or CLANGARM64 environment.

```bash
# From MSYS2 UCRT64 or CLANGARM64 shell:
cd Packaging/win

# Install dependencies only
./ffbuild.sh --depsonly

# Full build
./ffbuild.sh

# Create portable package
./make-portable-package.sh
```

By default, build outputs go to `Packaging/win/build/`. You can override this
with the `BUILD_DIR` environment variable:

```bash
BUILD_DIR=/tmp/ffbuild ./ffbuild.sh
```

The build creates `$BUILD_DIR/ReleasePackage/` containing the distributable files.

## CI/CD

The GitHub Actions workflow (`.github/workflows/main.yml`) automates this process,
building for both x64 (UCRT64) and ARM64 (CLANGARM64) architectures.

## Credits

These scripts were originally developed by Jeremy Tan as the separate
[fontforgebuilds](https://github.com/fontforge/fontforgebuilds) repository,
based on earlier work by Matthew Petroff.

## License

- Build scripts (`ffbuild.sh`, etc.): BSD 2-clause license
- Icons and graphics by Matthew Petroff: Creative Commons Attribution 3.0 Unported
