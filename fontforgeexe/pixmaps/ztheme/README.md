# ztheme

A light-mode SVG icon theme for FontForge's legacy UI, created by [Adolfo Ovalles](https://www.behance.net/adolfo_ovalles) as part of the [zTheme project](https://github.com/adolfo-ovalles/zThemes).

Filenames follow the existing [Tango](../tango) icon set's naming convention, allowing this theme to serve as a drop-in SVG replacement. Icons are intended to work with FontForge's symbolic SVG support, where a single monochrome icon set adapts automatically to light/dark theme colors — removing the need for separate icon sets per theme.

## Status: Draft / Pending Review

This PR is submitted as a **draft** for review and discussion before merging. A few items still need confirmation:

### Confirming intentionally distinct icon pairs

Following Tango's own convention, filenames that look conceptually similar are kept as separate files rather than merged, since some are referenced independently by different dialogs (as already confirmed for `rmoverlap`/`overlaprm`). Flagging the full list here so you can confirm the rest follow the same pattern, or point out any that should actually be merged:

`exclude` / `overlapexclude` · `fliphor` / `transformfliphor` · `flipvert` / `transformflipvert` · `changeweight` / `styleschangeweight` · `extendcondense` / `stylesextendcondense` · `inline` / `stylesinline` · `oblique` / `stylesoblique` · `text12210` / `stylesitalic` · `outline` / `stylesoutline` · `wireframe` / `styleswireframe` · `shadow` / `stylesshadow` · `rotate180` / `transformrotate180` · `rotateccw` / `transformrotateccw` · `rotatecw` / `transformrotatecw` · `skew` / `transformskew` · `rmoverlap` / `overlaprm` · `findinter` / `overlapfindinter` · `fileclose2` / `fileclose` · `intersection` / `overlapintersection`

### Known placeholders

`elementtilepath.svg` and `elementtilepattern.svg` (Element menu → Tile Path / Tile Pattern) are currently **intentionally blank** — empty SVGs sized to match the rest of the set. I wasn't able to locate where these commands appear in FontForge's UI to design dedicated icons. Pointers on the right dialog/menu would be appreciated, or confirmation that a blank icon is acceptable for now.

### Extra files

`paletteline-selected.svg`, `palettepencil-selected.svg` and `paletteshift-selected.svg` are required to correctly reproduce the selection state in the tool palette under BitmapView.

### Icon updates

A few icons were refined or newly created specifically for this FontForge contribution, beyond what exists in the original zTheme releases:

- **Refined:** `chooserhomefolder`, `chooserupdir`, `chooserback`, `chooserforward`, `chooserdir`, `choosersfdir`, `elementclockwise`, `elementanticlock`, `elementcorrectdir`
- **New:** `elementharmonize`, `elementaddinflections`, `elementbalance` 

### SVG format notes

Icons are exported as plain SVG using presentation attributes (no inline `<style>`/CSS classes), with Illustrator's internal `id`/`data-name` metadata stripped.

### Fill color convention

Paths intended to follow the theme (light/dark) currently have **no fill attribute set** — they rely on the SVG default (implicit black) as a placeholder, with the intent that these become symbolic/theme-aware. Paths intended to keep a **fixed, non-themable color** (e.g., the `selectblue.svg`, `selectred.svg` swatches) already have an explicit `fill` attribute assigned.

Before finalizing, I need to confirm: for the themable paths, should I add `fill="currentColor"` explicitly, or does your symbolic SVG implementation expect a different convention (e.g., specific sentinel hex values per the older GTK symbolic spec)?

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