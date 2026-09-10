# ztheme

A light-mode SVG icon theme for FontForge's legacy UI, created by [Adolfo Ovalles](https://www.behance.net/adolfo_ovalles) as part of the [zTheme project](https://github.com/adolfo-ovalles/zThemes).

Filenames follow the existing [Tango](../tango) icon set's naming convention, allowing this theme to serve as a drop-in SVG replacement. Icons are intended to work with FontForge's symbolic SVG support, where a single monochrome icon set adapts automatically to light/dark theme colors — removing the need for separate icon sets per theme.

## Status

The icon set has been reviewed. Redundant icons flagged during review have been removed. Merging is expected before symbolic SVG integration lands in FontForge's rendering code, which is deferred until after the next release for stability reasons.

### Removed redundant icons

The following icons were confirmed unused and removed from the set, since a differently-named file already covers the same UI element: `exclude`, `fliphor`, `flipvert`, `changeweight`, `extendcondense`, `inline`, `oblique`, `text12210`, `outline`, `wireframe`, `shadow`, `rotate180`, `rotateccw`, `rotatecw`, `skew`, `findinter`, `fileclose2`, `intersection`, `rmoverlap`.

`rmoverlap` was also flagged as mistakenly referenced in Metrics View. That reference should be updated to point to `overlaprm` instead, which is the file kept in this set.

### Known placeholders

`elementtilepath.svg` and `elementtilepattern.svg` (Element menu → Tile Path / Tile Pattern) are intentionally blank. These commands have been disabled in FontForge for years and are not currently reachable in the UI, so blank icons are acceptable here.

### Extra files

`paletteline-selected.svg`, `palettepencil-selected.svg` and `paletteshift-selected.svg` are required to correctly reproduce the selection state in the tool palette under BitmapView.

### Icon updates

A few icons were refined or newly created specifically for this FontForge contribution, beyond what exists in the original zTheme releases:

- Refined: `chooserhomefolder`, `chooserupdir`, `chooserback`, `chooserforward`, `chooserdir`, `choosersfdir`, `elementclockwise`, `elementanticlock`, `elementcorrectdir`
- New: `elementharmonize`, `elementaddinflections`, `elementbalance`

### SVG format notes

Icons are exported as plain SVG using presentation attributes (no inline `<style>`/CSS classes), with Illustrator's internal `id`/`data-name` metadata stripped.

### Fill color convention

Paths intended to follow the theme (light/dark) currently have no fill attribute set. They rely on the SVG default (implicit black) as a placeholder, with the intent that these become symbolic/theme-aware. Paths intended to keep a fixed, non-themable color (e.g., the `selectblue.svg`, `selectred.svg` swatches) already have an explicit `fill` attribute assigned.

## What's included

- SVG icons matching Tango's existing filenames (converted from original vector source)
- `resources.in` — theme resource/color definitions
- `CMakeLists.txt` — build integration
- Original PSD design references and Illustrator vector source are available in the [zTheme repository](https://github.com/adolfo-ovalles/zThemes)

## Source files

Original design files (Illustrator + PSD, pixel-grid aligned) are maintained separately at:
https://github.com/adolfo-ovalles/zThemes/tree/main/Source

## License

Distributed under the FontForge project's existing BSD 3-clause license — see the main repository [LICENSE](../../../LICENSE).