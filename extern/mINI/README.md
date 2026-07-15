# mINI - INI file reader and writer

This is a vendored copy of the mINI library.

## Source

- **Repository**: https://github.com/pulzed/mINI
- **Version**: 0.9.18
- **License**: MIT
- **Downloaded**: 2026-01-31

## Usage in FontForge

mINI is used for reading and writing plugin configuration files. We enable
case-sensitive mode by defining `MINI_CASE_SENSITIVE` before including the
header.

## Updating

To sync with upstream:

```bash
curl -sL https://raw.githubusercontent.com/pulzed/mINI/master/src/mini/ini.h \
     -o extern/mINI/ini.h
```

Then update the version number in this README based on the version comment
in the header file (look for `/mINI/ vX.Y.Z` near the top).

## Modifications

None. This is an unmodified copy of the upstream source.
