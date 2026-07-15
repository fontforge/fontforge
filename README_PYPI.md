# FontForge Python Module

FontForge is a powerful, open-source font editor. This package provides the
Python bindings for FontForge, allowing you to create, edit, and convert fonts
programmatically.

## Installation

```bash
pip install fontforge
```

## Quick Start

```python
import fontforge

# Create a new font
font = fontforge.font()
font.familyname = "MyFont"
font.fontname = "MyFont-Regular"

# Create a glyph
glyph = font.createChar(ord('A'))
pen = glyph.glyphPen()
# Draw the glyph...

# Open an existing font
font = fontforge.open("path/to/font.ttf")

# Access glyphs
for glyph in font.glyphs():
    print(glyph.glyphname, glyph.unicode)

# Generate output
font.generate("output.otf")
```

## Features

- **Read/Write Font Formats**: OTF, TTF, UFO, Type1, WOFF, WOFF2, and more
- **Glyph Editing**: Create and modify glyph outlines programmatically
- **OpenType Features**: Build and manipulate GSUB/GPOS tables
- **Font Conversion**: Convert between font formats
- **Metrics & Kerning**: Set sidebearings, create kerning pairs
- **Validation**: Check fonts for common issues
- **Batch Processing**: Script operations across multiple fonts

## Included Modules

This package installs two Python modules:

- **`fontforge`** - The main font manipulation module
- **`psMat`** - PostScript transformation matrix utilities

```python
import fontforge
import psMat

# Create a transformation matrix (scale 2x)
matrix = psMat.scale(2.0)

# Apply to a glyph layer
glyph.transform(matrix)
```

## Documentation

- [Python Scripting Guide](https://fontforge.org/docs/scripting/python.html)
- [Python API Reference](https://fontforge.org/docs/scripting/python/fontforge.html)
- [FontForge Documentation](https://fontforge.org/docs/)

## Requirements

- Python 3.8 or later
- No additional Python dependencies required

The wheel includes all required native libraries (FreeType, libxml2, etc.).

## Platform Support

Pre-built wheels are available for:

- **Linux**: x86_64, aarch64 (manylinux_2_28)
- **macOS**: x86_64, arm64 (macOS 11.0+)
- **Windows**: x86_64

## Security Considerations

FontForge was designed for interactive use with trusted font files. The
codebase has known vulnerabilities, including buffer overruns, that can cause
crashes and may be exploitable. **Do not process untrusted input without
thorough sandboxing.**

## Building from Source

If no wheel is available for your platform, pip will attempt to build from source. This requires:

- CMake 3.15+
- C/C++ compiler
- FreeType, libxml2, zlib development headers

See the [build documentation](https://github.com/fontforge/fontforge#building) for details.

## FontForge Application

This package provides only the Python module. For the full FontForge GUI application, see:

- [FontForge Downloads](https://fontforge.org/en-US/downloads/)
- [GitHub Releases](https://github.com/fontforge/fontforge/releases)

## License

FontForge is licensed under the [GPLv3+](https://github.com/fontforge/fontforge/blob/master/COPYING.gplv3).

## Links

- [Homepage](https://fontforge.org)
- [GitHub Repository](https://github.com/fontforge/fontforge)
- [Issue Tracker](https://github.com/fontforge/fontforge/issues)
