# Wheel Build Status

Temporary tracking document for PyPI wheel distribution work.

For technical documentation, see `doc/sphinx/scripting/python-extension.rst`.

## cibuildwheel Configuration

```toml
[tool.cibuildwheel]
skip = ["pp*", "*-win32", "*-manylinux_i686", "*-musllinux*"]

[tool.cibuildwheel.linux]
manylinux-x86_64-image = "manylinux_2_28"
repair-wheel-command = "auditwheel repair -w {dest_dir} {wheel}"

[tool.cibuildwheel.macos]
archs = ["x86_64", "arm64"]
repair-wheel-command = "delocate-wheel -w {dest_dir} {wheel}"

[tool.cibuildwheel.windows]
archs = ["AMD64"]
repair-wheel-command = "delvewheel repair -w {dest_dir} {wheel}"
```

## Implementation Checklist

- [x] `BUILDING_WHEEL` detection in CMakeLists.txt
- [x] Python version generation (CalVer)
- [x] Wheel install layout in pyhook/CMakeLists.txt
- [x] pyproject.toml with scikit-build-core
- [x] Custom version provider
- [x] cibuildwheel in GitHub Actions
- [x] README_PYPI.md
- [x] Context-aware version (CalVer vs numeric)
- [x] dist-info on app install
- [ ] PyPI trusted publishing
- [ ] sdist build testing
- [ ] Initial PyPI release

## Notes

- Windows wheels require MSVC build (separate from MinGW GUI build)
- Wheels bundle all dependencies via auditwheel/delocate/delvewheel
- Python versions: support current CPython releases (check pyproject.toml)
