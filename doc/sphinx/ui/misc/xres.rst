Changing FontForge's UI Appearance
==================================

FontForge has an extensive appearance editor that evolved out of the X resource
system but now bears little relation to it beyond its file format.  As of 2021
every parameter is represented in the editor and almost all have a tooltip
similar to the explanations below. The editor is therefore the primary source
of information about the parameters and this document is mostly useful for its
explanation of the new font specification system.

.. _xres.font:

Fonts
-----

A font specification consists of a *size* directive, one or more *style*
directives a *weight* directive, an explicit or implicit *family list*, and/or
a reference font name.

Explicit Size Directives
~~~~~~~~~~~~~~~~~~~~~~~~

An explicit size directive is either an integer followed by "pt" for a point
size (as in "10pt") or an integer followed by "px" for a pixel size (as in
"12px").

Style Directives
~~~~~~~~~~~~~~~~

A style directive is one or more of "italic", "oblique", "small-caps",
"extended", and "condensed".

Weight Directives
~~~~~~~~~~~~~~~~~

A weight directive is either one of the strings "normal", "light", or "bold" or
an integer multiple of 100, where 400 is equivalent to "normal" and 700 is
equivalent to "bold".

Family Lists
~~~~~~~~~~~~

A family list is a comma-separated list of families, where a given family name
can contain spaces. When present a family list must come last in the specification,
whereas other directives can occur in any order.

The directive "bold italic 10pt dejavu sans,helvetica,sans" therefore specifies
10 point bold italic DejaVu Sans as the font, with Helvetica and Sans possibly
used as fallbacks for missing glyphs.

Reference Font Names
~~~~~~~~~~~~~~~~~~~~

Instead of specifying a font from scratch you can do so in relation to a
reference font, given by the name of the resource preceded by a caret. The
primary fonts to reference are "View.DefaultFont", "View.LabelFont",
"View.MonoFont", and "View.SerifFont". These are (or should be) specified with
appropriate family lists for a sans-serif, "unicode", monospace, and serif font
respectively. They also specify a reference font size, which is 12 points by
default. The label "unicode" font is typically used in cases where extended
or obscure unicode characters are directly displayed (most often as glyph
slot labels in the FontView window).

To copy the exact specification of a different font you can just include the
reference name, as in "^View.DefaultFont". To override the size, weight or
style just include one of those directives, as in "10pt bold ^View.MonoFont".
Note that style directives are not cumulative, so if "FontInfo.Font" is bold
then "italic ^FontInfo.Font" will just be italic, not bold italic.

Finally, and most usefully, you can specify a size as a *percentage* rather
than an absolute size, with an integer followed by a percent sign. "83% bold
^View.DefaultFont" therefore specifies whatever size (in points or pixels)
would be 83% of that font.

The default resource file now has these relative font settings, so that
changing the point size of View.DefaultFont scales the whole UI. There are some
glitches but for the most part it works at lower point sizes (less than 17).

Other Info
----------

By default FontForge loads the system configuration file. You can change this
behavior by adding a custom file in your preferences, which happens
automatically when you choose Save As" in the :doc:`File->Appearance Editor
<resedit>` and keep the button checked.  Also see
:ref:`File->Preference->Generic->ResourceFile <prefs.ResourceFile>`.

When editing resources some will take effect immediately, some when a new window
is opened, and some on the next restart of the program. It is therefore best to
restart after each significant change.

The Directives
--------------

View
~~~~

.. object:: fontforge.View.DefaultFont

   Specifies the main font family used throughout the program, which is usually
   sans-serif, along with a reference size (12 points by default).

.. object:: fontforge.View.MonoFont

   Specifies the monospace font family used in various fields and tables along
   with a reference size (copied from View.DefaultFont by default).

.. object:: fontforge.View.SerifFont

   Specifies the serif font family used in a few contexts along with a reference
   size (copied from View.DefaultFont by default).

.. object:: fontforge.View.Background

   Sets the background color for the drawing areas of the fontview, glyph view,
   bitmap view and metrics view.

Font View
~~~~~~~~~

.. object:: fontforge.FontView.GlyphFGColor

   The color of the glyph image in the FontView when not selected

.. object:: fontforge.FontView.GlyphInfoColor

   Sets the color used to display information about selected glyph, between the
   FontView menu bar and the glyph array.

.. object:: fontforge.FontView.SlotOutlineColor

   The color of the box around each glyph.

.. object:: fontforge.FontView.SlotDivisionColor

   The color of the line between the glyph label and glyph image

.. object:: fontforge.FontView.LabelColor

   The default color of the label

.. object:: fontforge.FontView.UnencodedLabelColor

   The color of the label when the glyph is not part of the encoding

.. object:: fontforge.FontView.MissingLabelColor

   The color of the substitute label when the proper one is unknown or unavailable.

.. object:: fontforge.FontView.EmptySlotFgColor

   Sets the color of crosses marking empty code points.

.. object:: fontforge.FontView.SelectedColor

   Sets the background color of selected glyphs.

.. object:: fontforge.FontView.SelectedFgColor

   Sets the foreground color of selected glyphs.

.. object:: fontforge.FontView.ChangedColor

   Sets the color used to mark changed glyphs.

.. object:: fontforge.FontView.MissingBitmapColor

   In a font with both outline and bitmaps this marks a slot with an outline but not a bitmap

.. object:: fontforge.FontView.MissingOutlineColor

   In a font with both outline and bitmaps this marks a slot with a bitmap but no outline

.. object:: fontforge.FontView.HintingNeededColor

   Sets the color of markings for glyphs that need hinting or instructing.

.. object:: fontforge.FontView.MetricsAdvanceAtColor
.. object:: fontforge.FontView.MetricsAdvanceToColor
.. object:: fontforge.FontView.MetricsBaselineColor
.. object:: fontforge.FontView.MetricsOriginColor

   The respective colors of these metrics when they are set as visible in the View menu

.. object:: fontforge.FontView.Font

   The font used for the lables, which by default is just ^View.DefaultFont

Outline Points
~~~~~~~~~~~~~~

.. object:: fontforge.CharView.PointColor

   Sets the color used to draw curved, corner or tangent points in the outline
   character view.

.. object:: fontforge.CharView.FirstPointColor

   Sets the color of the first point on a contour.

.. object:: fontforge.CharView.SelectedPointColor

   Sets the color used to draw selected curved, corner or tangent points in the
   outline character view.

.. object:: fontforge.CharView.SelectedPointWidth

   Sets the width of the line used to outline selected curved, corner or tangent
   points in the outline character view.

.. object:: fontforge.CharView.ExtremePointColor

   Sets the color of a point which is an extremum.

.. object:: fontforge.CharView.PointOfInflectionColor

   Sets the color of a location which is a point of inflection.

.. object:: fontforge.CharView.AlmostHVColor

   Sets the color used to mark lines and curves which are almost, but not quite
   horizontal or vertical.

.. object:: fontforge.CharView.NextCPColor

   Sets the color used to draw the "Next" control point.

.. object:: fontforge.CharView.PrevCPColor

   Sets the color used to draw the "Previous" control point.

.. object:: fontforge.CharView.SelectedCPColor

   Sets the color used to draw a control point that has been selected.

.. object:: fontforge.CharView.AnchorColor

   Sets the color used to draw an anchor point

.. object:: fontforge.CharView.LabelFont

   Used for point and contour names, anchor point names, etc.

.. object:: fontforge.CharView.IconFont

   Used to build window decoration icons in some cases

.. object:: fontforge.CharView.PointNumberFont

   Used for point numbers, hints, etc.

Outline Lines/Fills
~~~~~~~~~~~~~~~~~~~

.. object:: fontforge.CharView.ForegroundOutlineColor

   Sets the color used to draw foreground outlines.

.. object:: fontforge.CharView.ForegroundThickOutlineColor

   The color of thick outlines in the active layer (when zoomed in)

.. object:: fontforge.CharView.FillColor

   Sets the color used to draw a character's fill

.. object:: fontforge.CharView.PreviewFillColor

   The color to use when performing a preview fill. If this is not set then
   FontForge will fallback to using fontforge.CharView.FillColor. Neither of
   these resources are set then black will be used.

.. object:: fontforge.CharView.OpenPathColor

   The color of the line of an open path. (The "thin" color will be with
   the alpha removed and the "thick" color will be with the alpha included.)

.. object:: fontforge.CharView.ClipPathColor

   The color of a clip path.

.. object:: fontforge.CharView.BackgroundOutlineColor

   Sets the color used to draw background outlines.

.. object:: fontforge.CharView.BackgroundThickOutlineColor

   The color of "thick" background outlines (when zoomed in).

.. object:: fontforge.CharView.WidthColor

   Sets the color used to draw the advance width line.

.. object:: fontforge.CharView.WidthSelColor

   Sets the color used to draw the advance width if it is selected.

.. object:: fontforge.CharView.LBearingSelColor

   The color of the left bearing line wien selected

.. object:: fontforge.CharView.LigatureCaretColor

   Sets the color used to draw ligature caret lines.

.. object:: fontforge.CharView.AnchoredOutlineColor

   The color of another glyph drawn in the current view to show
   where it would be placed by an anchor lookup.

.. object:: fontforge.CharView.CoordinateLineColor

   Sets the color used to draw the baseline and x=0 line.

.. object:: fontforge.CharView.AscentDescentColor

   Sets the color used to draw the ascent and descent lines.

.. object:: fontforge.CharView.ItalicCoordColor

   Sets the color used to draw various horizontal metrics lines when they have
   been skewed appropriately for an italic font.

.. object:: fontforge.CharView.MetricsLabelColor

   Sets the color used to label metrics lines

.. object:: fontforge.CharView.TemplateOutlineColor

   Sets the color used to draw a template outline. (not currently used)

.. object:: fontforge.CharView.RulerBigTickColor

   Sets the color of coarse-grained ruler ticks.

.. object:: fontforge.CharView.RulerCurrentTickColor

   Sets the color used to draw a vertical and a horizontal tick
   corresponding to the mouse position.

.. object:: fontforge.CharView.RulerFont

   Font uesd for ruler numbers and other ruler notations.

.. object:: fontforge.CharView.GuideOutlineColor

   Sets the color used to draw outlines in the Guide layer.

.. object:: fontforge.CharView.GuideDragColor

   The color used to display a new guide line dragged from the ruler.

Outline Tools
~~~~~~~~~~~~~

.. object:: fontforge.CharView.TraceColor

   Sets the color used to draw the trace of the freehand tool.

.. object:: fontforge.CharView.OldOutlineColor

   Sets the color used to draw the original outline of a set of splines being
   transformed with one of the transform tools (flip, rotate, scale, etc.)

.. object:: fontforge.CharView.TransformOriginColor

   Sets the color used to draw the origin of the current transformation.

.. object:: fontforge.CharView.DraggingComparisonOutlineColor

   The color used to draw the outline of the old spline when you
   are interactively modifying a glyph

.. object:: fontforge.CharView.DraggingComparisonAlphaChannelColor

   Only the alpha value of this parameter is used. If non zero it will
   set the alpha channel for the control points, bezier information
   and other non spline indicators for the Dragging Comparison Outline
   spline

.. object:: fontforge.CharView.MeasureToolLineColor

   The color used to draw the measure tool line.

.. object:: fontforge.CharView.MeasureToolPointColor

   The color used to draw the measure tool points.

.. object:: fontforge.CharView.MeasureToolPointSnappedColor

   The color used to draw the measure tool points when snapped.

.. object:: fontforge.CharView.MeasureToolCanvasNumbersColor

   The color used to draw the measure tool numbers on the canvas.

.. object:: fontforge.CharView.MeasureToolCanvasNumbersSnappedColor

   The color used to draw the measure tool numbers on the canvas when snapped.

.. object:: fontforge.CharView.MeasureToolWindowForeground

   The measure tool "window" foreground color.

.. object:: fontforge.CharView.MeasureToolWindowBackground

   The measure tool "window" background color.

.. object:: fontforge.CharView.MeasureToolFont

   The font used to display the information in the measure tool
   "window".

Outline Hints
~~~~~~~~~~~~~

.. object:: fontforge.CharView.BlueValuesStippledCol

   Sets the color used to draw the BlueValues and OtherBlues zones.

.. object:: fontforge.CharView.FamilyBlueStippledColor

   Sets the color used to draw the FamilyBlueValues and FamilyOtherBlues zones.

.. object:: fontforge.CharView.MDHintColor

   Sets the color used to draw minimum distance hints

.. object:: fontforge.CharView.HintLabelColor

   Sets the color used to label hint lines (and blue value lines)

.. object:: fontforge.CharView.DHintColor

   Sets the color used to draw diagonal hints

.. object:: fontforge.CharView.HHintColor

   Sets the color used to draw horizontal stem hints

.. object:: fontforge.CharView.VHintColor

   Sets the color used to draw vertical stem hints

.. object:: fontforge.CharView.HFlexHintColor

   Sets the color used to draw the halo around horizontal flex hints

.. object:: fontforge.CharView.VFlexHintColor

   Sets the color used to draw the halo around vertical flex hints.

.. object:: fontforge.CharView.ConflictHintColor

   Sets the color used to draw hints when they conflict

.. object:: fontforge.CharView.HHintActiveColor

   Sets the color used to draw a horizontal stem hint when it is active in the
   review hints dlg.

.. object:: fontforge.CharView.VHintActiveColor

   Sets the color used to draw a vertical stem hint when it is active in the
   review hints dlg.

.. object:: fontforge.CharView.DeltaGridColor

   Indicates a notable grid pixel when suggesting deltas.

Outline Raster
~~~~~~~~~~~~~~

.. object:: fontforge.CharView.GridFitOutlineColor

   Sets the color used to draw outlines which have been gridfit (this should
   probably be the same as BackgroundOutlineColor as both are in the background
   layer).

.. object:: fontforge.CharView.GridFitWidthColor

   Sets the color used to draw the advance width once it has been grid fit (if
   :menuselection:`View --> Show Grid Fit` is on)

.. object:: fontforge.CharView.RasterColor

   Sets the color used to draw the pixels of a rasterized bitmap (if
   :menuselection:`View --> Show Grid Fit` or :menuselection:`Hints --> Debug`
   is on)

.. object:: fontforge.CharView.RasterNewColor

   Sets the color used to draw the pixels of a rasterized bitmap if they have
   recently been turned on (if :menuselection:`Hints --> Debug` is on)

.. object:: fontforge.CharView.RasterOldColor

   Sets the color used to draw the pixels of a rasterized bitmap f they have
   recently been turned off (if :menuselection:`Hints --> Debug` is on)

.. object:: fontforge.CharView.RasterGridColor

   Sets the color used to draw the pixel grid used by the rasterizer (if
   :menuselection:`View --> Show Grid Fit` or :menuselection:`Hints --> Debug`
   is on)

.. object:: fontforge.CharView.RasterDarkColor

   When doing anti-aliased debugging, sets the color used for the darkest pixel.
   Other pixels will be interpolated between this and the background.

.. object:: fontforge.CharView.BackgroundImageColor

   Sets the color used to draw background images.

Palettes
~~~~~~~~

.. object:: fontforge.CharView.CVPaletteForegroundColor

   The foreground color of the tools and layers palettes.

.. object:: fontforge.CharView.CVPaletteBackgroundColor

   The background color of the tools and layers palettes.

.. object:: fontforge.CharView.Button3DEdgeLightColor

   The color of the light edge of palette buttons when Button3d is True.

.. object:: fontforge.CharView.Button3DEdgeDarkColor

   The color of the dark edge of palette buttons when Button3d is True.

.. object:: fontforge.CharView.Button3D

   When True palette buttons are displayed with a 3D effect.

.. object:: fontforge.LayersPalette.Font

   The font used in the layers palettes dialog.

.. object:: fontforge.ToolsPalette.Font

   The font used in the Tools Palette dialog for labelling tool options.

Bitmap View
~~~~~~~~~~~

.. object:: fontforge.BitmapView.BitmapColor

   The color of the large bitmap.

.. object:: fontforge.BitmapView.OverviewColor

   The color of the small bitmap view.

.. object:: fontforge.BitmapView.GuideColor

   The color of the guide lines for glyph metrics.

.. object:: fontforge.BitmapView.WidthGuideColor

   The color of the guide line for the advance width.

.. object:: fontforge.BitmapView.GridColor

   The color of the guide lines for the bitmap grid.

.. object:: fontforge.BitmapView.OutlineColor

   The color of the outline.

.. object:: fontforge.BitmapView.ActiveToolColor

   The color of the preview for drawing lines, rectangles, and ellipses.

.. object:: fontforge.BitmapView.SelectedRegionColor

   The color of the selected region.

.. object:: fontforge.BitmapView.ReferenceColor

   The color of a reference.

.. object:: fontforge.BitmapView.SelectedReferenceColor

   The color of the selected reference.

.. object:: fontforge.BitmapView.ReferenceBorderColor

   The color used to outline a reference.

.. object:: fontforge.BitmapView.SelectedReferenceBorderColor

   The color used to outline the selected reference.

Metrics View
~~~~~~~~~~~~

.. object:: fontforge.MetricsView.Font

   The font used to display labels in the metrics view.

.. object:: fontforge.MetricsView.GlyphColor

   The foreground color of the glyph display area.

.. object:: fontforge.MetricsView.SelectedGlyphColor

   The color for the currently selected glyph.

.. object:: fontforge.MetricsView.AdvanceWidthColor

   The color of field divider lines.

.. object:: fontforge.MetricsView.AdvanceWidthColor

   Sets the color for the grid lines in the metrics view when nothing special
   is happening to them.

.. object:: fontforge.MetricsView.ItalicAdvanceColor

   In an italic font, this will be the color used to draw the line at the
   italicAngle which corresponds to the italic advance width.

.. object:: fontforge.MetricsView.KernLineColor

   Sets the color for the grid line in the metrics view which currently may be
   moved to change a glyph's kerning.

.. object:: fontforge.MetricsView.SideBearingLneColor

   Sets the color for the grid line in the metrics view which currently may be
   moved to change a glyph's right side bearing (or bottom side bearing).

Misc Windows
~~~~~~~~~~~~

.. object:: fontforge.BDFProperties.Font

   Sets the font used in the BDF Properties dialog for stand alone text

.. object:: fontforge.Combinations.Font

   Sets the font used in the kern and anchor combinations dialog for labelling
   the combinations

.. object:: fontforge.CVT.Font

   Sets the font used in the 'cvt ' table dialog

.. object:: fontforge.DebugView.Background

   The background of the TTF debugging window.

.. object:: fontforge.DebugView.Font

   The font used to display the truetype instructions being debugged.

.. object:: fontforge.FontInfo.OriginLineColor

   The color used for the baseline and x=0 line in kerning dialogs.

.. object:: fontforge.FontInfo.Font

   The font used for Font Info dialog scrolling lists.

.. object:: fontforge.GlyphInfo.Font

   Sets the font used in the glyph info dialog for stand alone text

.. object:: fontforge.Groups.Font

   Sets the font used in the Groups dialog

.. object:: fontforge.Histogram.Font

   Sets the font used in the Histogram dialog

.. object:: fontforge.KernClass.TextColor

   Color for kerning class names.

.. object:: fontforge.KernClass.Font

   Sets the font used in the kern class and pair dialogs

.. object:: fontforge.KernFormat.Font

   The normal font used in the kernig format dialog.

.. object:: fontforge.KernFormat.BoldFont

   The bold font used in the kernig format dialog.

.. object:: fontforge.Math.Font

   The normal font used in the Math dialog.

.. object:: fontforge.Math.BoldFont

   The bold font used in the Math dialog.

Misc Windows 2
~~~~~~~~~~~~~~

.. object:: fontforge.Prefs.MonoFont

   The monospace font used in the preferences dialog.

.. object:: fontforge.SearchView.Font

   Sets the font used in the find and replace dialog

.. object:: fontforge.SearchView.BoldFont

   Sets the bold font used in the find and replace dialog

.. object:: fontforge.ShowATT.SelectColor

   Color used for currently selected entry in the Show ATT dialog.

.. object:: fontforge.ShowATT.GlyphNameColor

   Color used for (some) glyph names in the Show ATT dialog.

.. object:: fontforge.ShowATT.Font

   Sets the font used in the Show ATT dialog

.. object:: fontforge.ShowATT.MonoFont

   Sets the monospaced font used in the Show ATT dialog

.. object:: fontforge.StateMachine.Font

   Sets the font used in the Apple state machine dialog

.. object:: fontforge.TilePath.Font

   Sets the font used in the Tile Path dialog

.. object:: fontforge.TilePath.BoldFont

   Sets the bold font used in the Tile Path dialog

.. object:: fontforge.TTInstruction.Font

   Sets the font used in the various dialogs which edit truetype instructions
   ('fpgm' table, glyph instructions, etc.)

.. object:: fontforge.Validate.Font

   Sets the font used in the Validate dialog

.. object:: fontforge.Warnings.Font

   Sets the font used in the Warnings dialog

Splash Screen
~~~~~~~~~~~~~

.. object:: fontforge.Splash.Foreground

   The foreground color of the About dialog.

.. object:: fontforge.Splash.Background

   The background color of the About... dialog.

.. object:: fontforge.Splash.Font

   Sets the font used in the splash screen and About FontForge dialog.

.. object:: fontforge.Splash.ItalicFont

   Sets the italic font used in the About... dialog.

.. object:: fontforge.Splash.MonoFont

   The monospace font used in the About... dialog.

.. object:: fontforge.Splash.Image

   The image used on the splash screen and About... dialog.

SFTextArea
~~~~~~~~~~

.. object:: fontforge.SFTextArea.Box.ActiveInner
.. object:: fontforge.SFTextArea.Box.Padding

   See the :ref:`GGadget Box <xres.GGadgetBox>` section.

.. object:: fontforge.SFTextArea.Font

   Sets the font used in the Print dialog and its variants dialog -- except I
   don't think this ever gets used.

GDraw
~~~~~

.. object:: Gdraw.Background

   The default background color in contexts other than View windows and GGadgets.

.. object:: Gdraw.Foreground

   The default foreground color in contexts other than View windows and GGadgets.

.. object:: Gdraw.WarningForeground

   A color appropriate for displaying warning and error messages relative to
   GDraw.Background and other background colors.

.. object:: Gdraw.ScreenResolution

   The resolution of the screen in dots per inch. (Don't set this or set it to
   zero for the system default resolution.)

.. object:: Gdraw.MultiClickTime

   An integer (milliseconds)

   The maximum amount of time allowed between two clicks for them to be
   considered a double (triple, etc.) click.

.. object:: Gdraw.MultiClickWiggle

   An integer (pixels)

   The maximum number of pixels the mouse is allowed to move between two clicks
   and have them still be considered a double click.

.. object:: Gdraw.SelectionNotifyTimeout

   An integer (seconds)

   Gdraw will wait this many seconds after making a request for a selection (ie.
   when doing a Paste). If it gets no responce after that period it reports a
   failure.

.. object:: Gdraw.TwoButtonFixup

   A boolean

   On a windows keyboard use the modifier key with the flag on it to simulate
   mouse button 2 (middle button). If this key is depressed when a mouse button
   is pressed or released then pretend it was button 2 that was pressed or
   release.

.. object:: Gdraw.MacOSXCmd

   A boolean

   On Mac OS X the user will probably expect to use the Command (apple,
   cloverleaf) key to control the menu (rather than the Control key). If this is
   set then the command key will be mapped to the control key internally.

.. object:: Gdraw.Synchronize

   A boolean

   Whether to synchronize the display before raising the first window.

GDraw (X backend only)
~~~~~~~~~~~~~~~~~~~~~~

These are not included in the appearance editor and need to be set
some other way, perhaps through the normal X Resources system.

.. object:: Gdraw.Depth

   An integer (1, 8, 16, 32)

   You can use this to request a different depth than the default one. Not all
   servers will support all depths. If FontForge can't find a visual with the
   desired depth it will use the default depth.

.. object:: Gdraw.VisualClass

   A string ("StaticGray", "GrayScale", "StaticColor", "PseudoColor",
   "TrueColor", "DirectColor")

   FontForge will search for a visual with the given class (and possibly depth
   if the depth argument is specified too).

.. _xres.Colormap:

.. object:: Gdraw.Colormap

   An string ("Current", "Copy", "Private")

   You can use this to control what FontForge does about the colormap on an 8bit
   screen

   * Current -- FontForge will attempt to allocate its colors in the current
     colormap.
   * Copy -- FontForge will allocate what colors it can and then copy the current
     color map into a private copy. This means FontForge has access to a much
     wider range of colors, and (as long as the shared colormap doesn't change)
     FontForge's colormap will match that of the rest of the screen.
   * Private -- FontForge will allocate a private colormap and set the colors just
     as it wants them. It will almost certainly not match the shared colormap.

.. _xres.Keyboard:

.. object:: Gdraw.Keyboard

   ibm | mac | sun | ppc | 0 | 1 | 2 | 3

   Allows you to specify the type of keyboard. Currently this is only relevant
   when generating menus. The modifier keys are in different locations on
   different keyboards (under different operating systems) and if FontForge
   knows what keyboard you are using it can make the hot-keys have better
   labels.

   * ibm | 0

     Uses the Control and Alt keys
   * mac | 1

     Uses the Command and Option keys (Mac OS/X, Mac keyboard)
   * ppc | 3

     Uses the Control and Command keys (Suse ppc linux, Mac keyboard)
   * sun | 2

     Uses the Control and Meta keys

Popup
~~~~~

.. object:: Gdraw.GGadget.Popup.Font

   A :ref:`font <xres.font>`

   Specifies the font to use in a popup (tooltip) message.

.. object:: Gdraw.GGadget.Popup.Foreground

   A :ref:`color <xres.color>`

   Specifies the foreground color of popup (tooltip) messages.

.. object:: Gdraw.GGadget.Popup.Background

   A :ref:`color <xres.color>`

   Specifies the background color of popup (tooltip) messages.

.. object:: Gdraw.GGadget.Popup.Delay

   An integer (milliseconds).

   Specifies the amount of time the cursor must remain motionless before a popup
   message pops up.

.. object:: Gdraw.GGadget.Popup.LifeTime

   An integer (milliseconds).

   Specifies the length of time the message will display.

Progress
~~~~~~~~

.. object:: Gdraw.GGadget.Progress.Font

   A :ref:`font <xres.font>`

   Specifies the font to use in a progress window.

.. object:: Gdraw.GGadget.Progress.Background

   A :ref:`color <xres.color>`

   Specifies the background color of progress window.

.. object:: Gdraw.GGadget.Progress.Foreground

   A :ref:`color <xres.color>`

   Specifies the foreground color of progress window.

.. object:: Gdraw.GGadget.Progress.FillCol

   A :ref:`color <xres.color>`

   Specifies the color of the progress bar in the progress window.

GGadget
~~~~~~~

.. _xres.GGadgetBox:

.. object:: Gdraw.GGadget...

   Every ggadget in enclosed in a box. No gadget is actually a GGadget, but
   every other gadget inherits (potentially with modification) from this
   abstract class. The following information may be supplied for any box:

   .. object:: ...Box.BorderType

      one of "none", "box", "raised", "lowered", "engraved", "embossed",
      "double"

      For a description of these see the css manual.

   .. object:: ...Box.BorderShape

      one of "rect", "roundrect", "ellipse", "diamond"

      Describes the basic shape of the box. (some ggadgets must be in
      rectangles).

   .. object:: ...Box.BorderWidth

      An integer (points)

      Specifies the width of the box's border in points (NOT pixels)

   .. object:: ...Box.Padding

      An integer (points)

      Specifies the padding between the interior of the box and the border

   .. object:: ...Box.Radius

      An integer (points)

      Specifies the radius of a roundrect. Ignored for everything else.

   .. object:: ...Box.BorderInner

      A boolean (true, on or 1, false, off or 0)

      Specifies whether a line should be drawn inside the border.

   .. object:: ...Box.BorderInnerCol

      A :ref:`color <xres.color>`

      Specifies a color of line that should be drawn inside a border.

   .. object:: ...Box.BorderOuter

      A boolean (true, on or 1, false, off or 0)

      Specifies whether a black line should be drawn outside the border.

   .. object:: ...Box.BorderOuterCol

      A :ref:`color <xres.color>`

      Specifies a color of line that should be drawn outside a border.

   .. object:: ...Box.ActiveInner

      A boolean (true, on or 1, false, off or 0)

      Specifies whether a yellow line should be drawn inside the border when the
      gadget is active (not all gadgets support this).

   .. object:: ...Box.DoDepressedBackground

      A boolean (true, on or 1, false, off or 0)

      Changes the color of the background while a button is depressed.

   .. object:: ...Box.GradientBG

      A boolean (true, on or 1, false, off or 0)

      Draws a gradient from GradientStartCol (at top and bottom edge) to
      Background (in the center).

   .. object:: ...Box.BorderBrightest

      A :ref:`color <xres.color>`

      The color of the brightest edge of the border (usually the left edge)

   .. object:: ...Box.BorderBrighter

      A :ref:`color <xres.color>`

      The color of the next to brightest edge of the border (usually the top
      edge)

   .. object:: ...Box.BorderDarkest

      A :ref:`color <xres.color>`

      The color of the darkest edge of the border (usually the right edge)

   .. object:: ...Box.BorderDarker

      A :ref:`color <xres.color>`

      The color of the next to next to darkest edge of the border. (usually the
      bottom edge)

   .. object:: ...Box.NormalBackground

      A :ref:`color <xres.color>`

      The color of a normal background (not disabled, not depressed)

   .. object:: ...Box.NormalForeground

      A :ref:`color <xres.color>`

      The color of a normal foreground (not disabled)

   .. object:: ...Box.DisabledBackground

      A :ref:`color <xres.color>`

      The color of a disabled background .

   .. object:: ...Box.DisabledForeground

      A :ref:`color <xres.color>`

      The color of a normal foreground.

   .. object:: ...Box.ActiveBorder

      A :ref:`color <xres.color>`

      The color of an ActiveInner border.

   .. object:: ...Box.PressedBackground

      A :ref:`color <xres.color>`

      The color of a depressed background.

   .. object:: ...Box.GradientStartCol

      A :ref:`color <xres.color>`

      Only meaningful if GradientBG is set. Draws a gradient of colors for the
      background with this color as the start point at the top and bottom edges
      of the gadget, and Background as the end point in the center of it.

   .. object:: ...Font

      A :ref:`font <xres.font>`

      Specifies the default font for a ggadget.

GGadget 2
~~~~~~~~~

.. object:: Gdraw.GGadget.Skip

   Space (in points) to skip between gadget elements.

.. object:: Gdraw.GGadget.LineSkip

   Space (in points) to skip after a line.

.. object:: Gdraw.GGadget.FirstLine

   Space (in points) to skip before a line when it is the first element

.. object:: Gdraw.GGadget.LeftMargin

   The default left margin (in points)

.. object:: Gdraw.GGadget.TextImageSkip

   Space (in points) left between images and text any labels, buttons,
   menu items, etc. that have both.

.. object:: Gdraw.GGadget.ImagePath

   A unix style path string, with directories separated by ":". The sequence
   "~/" at the start of a directory will be interpreted as the user's home
   directory. If a directory is "=" then the installed pixmap directory will be
   used.

   Specifies the search path for images. Specifically those used in the menus,
   and those used in various gadgets listed below.

The Gadgets
~~~~~~~~~~~

.. object:: Gdraw.GListMark... controls the shape of the mark used to show the menu of a combo box.

   .. image:: /images/GListMark.png

   See below for additional directives.
.. object:: Gdraw.GLabel...

   .. image:: /images/GLabel.png
.. object:: Gdraw.GButton...

   .. image:: /images/GButton.png

   See below for additional directives.
.. object:: Gdraw.GDefaultButton... Inherits from GButton

   .. image:: /images/GDefaultButton.png
.. object:: Gdraw.GCancelButton...  Inherits from GButton

   .. image:: /images/GCancelButton.png
.. object:: Gdraw.GDropList...

   .. image:: /images/GDropList.png
.. object:: Gdraw.GRadio...

   .. image:: /images/GRadio.png
.. object:: Gdraw.GCheckBox...
            Gdraw.GVisibilityBox...

   Two forms of checkbox-like element, the first a traditional checkbox
   and the second an visibility switch in the layer palette.

.. object:: Gdraw.GRadioOn...
            Gdraw.GRadioOff...
            Gdraw.GCheckBoxOn...
            Gdraw.GCheckBoxOff...
            Gdraw.GVisibilityBoxOn...
            Gdraw.GVisibilityBoxOff...

   These are mostly means of specifiying images for the radio button and
   checkboxes, but you can also use them to customize how an activated
   button looks vs a deactivated one.

   See below for additional directives.
.. object:: Gdraw.GTextField...

   .. image:: /images/GTextField.png
.. object:: Gdraw.GComboBox...    Inherits from GTextField

   .. image:: /images/GComboBox.png

   Also called a "List Field"
.. object:: Gdraw.GComboBoxMenu...    Inherits from GComboBox (This is the box drawn around the GListMark in a ComboBox)

   .. image:: /images/GComboBoxMenu.png
.. object:: Gdraw.GNumericField...    Inherits from GTextField

   .. image:: /images/GNumericField.png
.. object:: Gdraw.GNumericFieldSpinner...    Inherits from GNumericField

   .. image:: /images/GNumericFieldSpinner.png
.. object:: Gdraw.GScrollBar...

   A scroll bar widget. See below for additional directives.
.. object:: Gdraw.GList...
            Gdraw.GScrollBarThumb...
            Gdraw.GGroup... -- a frame around groups of gadgets.
            Gdraw.GLine...
            Gdraw.GMenu...
            Gdraw.GMenuBar...
            Gdraw.GTabSet...
            Gdraw.GVTabSet...

   As above.

   Specifies the box, font, color, etc. for this particular type of ggadget.
   See below for additional GMenu directives.

.. object:: Gdraw.GHVBox

   A group of gadgets that sits inside ``GGroup`` and supports graceful reflow
   of window contents in event of resizing. Modelled after GTK boxes. It's
   supposed to be invisible, but interface developers might actually want to
   style it.

.. object:: Gdraw.GScrollBar.Width

   An integer (points)

   Specifies the scrollbar width in points (for horizontal scrollbars it
   specifies the height)

.. object:: Gdraw.GScrollBar.StartTime

   An integer specifying the repeat latency in milliseconds.

.. object:: Gdraw.GScrollBar.RepeatTime

   An integer specifying the time between repeats in milliseconds.

.. object:: Gdraw.GListMark.Width

   An integer (points)

   Specifies the width for the little mark at the end of comboboxes and drop
   lists.

.. object:: Gdraw.GListMark.Image

   A filename of an image file

   Will be used instead of GListMark.Box if present. This is either a fully
   qualified pathname, or the filename of an image in the pixmap directory.

.. object:: Gdraw.GListMark.DisabledImage

   A filename of an image file

   Will be used instead of GListMark.Box for disabled (non-clickable) instances,
   if present. This is either a fully qualified pathname, or the filename of an
   image in the pixmap directory.

.. object:: Gdraw.GMenu.Grab

   A boolean

   Controls whether menus do pointer grabs. Debugging is easier if they don't.
   Default is for them to do grabs.

.. object:: Gdraw.GMenu.MacIcons

   A boolean

   Controls whether menus show shortcuts as the standard mac icons (cloverleaf
   for Command key, up arrow for shift, ^ for control and weird squiggle for
   Option(Meta/Alt)) or as text ("Cnt-Shft-A"). Default is True on the mac and
   False elsewhere.

   .. list-table::

      * - .. figure:: /images/MenuWithMacIcons.png

             True
        - .. figure:: /images/MenuWithoutMacIcons.png

             False

.. object:: Gdraw.GRadioOn.Image

   A filename of an image file.

   Used for drawing the "On" state of a radio button. (This is drawn within the
   ``GRadioOn`` box, if you intend the image to be the entire radio marker you
   should probably make the ``GRadioOn`` box be a blank rectangle). This is
   either a fully qualified pathname, or the filename of an image in the pixmap
   directory.

.. object:: Gdraw.GRadioOn.DisabledImage

   A filename of an image file.

   Used for drawing the "On" state of a disabled (non-clickable) radio button.
   (This is drawn within the ``GRadioOn`` box, if you intend the image to be the
   entire radio marker you should probably make the ``GRadioOn`` box be a blank
   rectangle). This is either a fully qualified pathname, or the filename of an
   image in the pixmap directory.

.. object:: Gdraw.GRadioOff.Image

   A filename of an image file.

   Used for drawing the "Off" state of a radio button. (This is drawn within the
   ``GRadioOff`` box, if you intend the image to be the full radio marker you
   should probably make the ``GRadioOff`` box be a blank rectangle). This is
   either a fully qualified pathname, or the filename of an image in the pixmap
   directory.

.. object:: Gdraw.GRadioOff.DisabledImage

   A filename of an image file.

   Used for drawing the "Off" state of a disabled (non-clickable) radio button.
   (This is drawn within the ``GRadioOff`` box, if you intend the image to be
   the full radio marker you should probably make the ``GRadioOff`` box be a
   blank rectangle). This is either a fully qualified pathname, or the filename
   of an image in the pixmap directory.

.. object:: Gdraw.GCheckBoxOn.Image

   A filename of an image file.

   Used for drawing the "On" state of a check box button. (This is drawn within
   the ``GCheckBoxOn`` box, if you intend the image to be the complete check box
   marker you should probably make the ``GCheckBoxOn`` box be a blank
   rectangle). This is either a fully qualified pathname, or the filename of an
   image in the pixmap directory.

.. object:: Gdraw.GCheckBoxOn.DisabledImage

   A filename of an image file.

   Used for drawing the "On" state of a disabled (non-clickable) check box
   button. (This is drawn within the ``GCheckBoxOn`` box, if you intend the
   image to be the complete check box marker you should probably make the
   ``GCheckBoxOn`` box be a blank rectangle). This is either a fully qualified
   pathname, or the filename of an image in the pixmap directory.

.. object:: Gdraw.GCheckBoxOff.Image

   A filename of an image file.

   Used for drawing the "Off" state of a check box button. (This is drawn within
   the ``GCheckBoxOff`` box, if you intend the image to be the sole check box
   marker you should probably make the ``GCheckBoxOff`` box be a blank
   rectangle). This is either a fully qualified pathname, or the filename of an
   image in the pixmap directory.

.. object:: Gdraw.GCheckBoxOff.DisabledImage

   A filename of an image file.

   Used for drawing the "Off" state of a disabled )non-clickable) check box
   button. (This is drawn within the ``GCheckBoxOff`` box, if you intend the
   image to be the sole check box marker you should probably make the
   ``GCheckBoxOff`` box be a blank rectangle). This is either a fully qualified
   pathname, or the filename of an image in the pixmap directory.

.. object:: Gdraw.GVisibilityBoxOn.Image

   A filename of an image file.

   Used for drawing the "On" state of a visibility box button. (This is the
   "eye" drawn within the layers palette of glyph view). This is either a fully
   qualified pathname, or the filename of an image in the pixmap directory.

.. object:: Gdraw.GVisibilityBoxOn.DisabledImage

   A filename of an image file.

   Used for drawing the "On" state of a disabled (non-clickable) visibility box
   button. (This is the "eye" drawn within the layers palette of glyph view).
   This is either a fully qualified pathname, or the filename of an image in the
   pixmap directory.

.. object:: Gdraw.GVisibilityBoxOff.Image

   A filename of an image file.

   Used for drawing the "Off" state of a visibility box button. (This is the
   "eye" drawn within the layers palette of glyph view). This is either a fully
   qualified pathname, or the filename of an image in the pixmap directory.

.. object:: Gdraw.GVisibilityBoxOff.DisabledImage

   A filename of an image file.

   Used for drawing the "Off" state of a disabled (non-clickable) visibility box
   button. (This is the "eye" drawn within the layers palette of glyph view).
   This is either a fully qualified pathname, or the filename of an image in the
   pixmap directory.

.. object:: Gdraw.GMatrixEdit.TitleFont

   A font.

   The font used to draw titles in a GMatrixEdit. By default this is smaller and
   bolder than the font used for text in the matrix edit.

.. object:: Gdraw.GMatrixEdit.TitleBG

   A color.

   Background color used for the titles of a matrix edit.

.. object:: Gdraw.GMatrixEdit.TitleFG

   A color.

   Foreground color used to draw the text of the titles of a matrix edit.

.. object:: Gdraw.GMatrixEdit.TitleDivider

   A color.

   Color used to draw the divider lines in the titles of a matrix edit.

.. object:: Gdraw.GMatrixEdit.RuleCol

   A color.

   Used to draw the horizontal and vertical lines in the body of a matrix edit.

.. object:: Gdraw.GMatrixEdit.FrozenCol

   A color.

   Used to draw text in a cell which is frozen (cannot but updated by the user)

.. object:: Gdraw.GMatrixEdit.ActiveCol

   A color.

   Used to draw text in the cell which is active (and used for the "<New>"
   entry).

.. object:: Gdraw.GMatrixEdit.ActiveBG

   A color.

   The background color of a cell that is active (and used for the "<New>"
   entry).

.. object:: Gdraw.GMatrixEdit.TitleFont

   A font.

   Used in the title area of a GMatrixEdit.

.. object:: ...

.. _xres.deprecated:

.. object:: Deprecated

   The following resources are deprecated and will be silently ignored.

   * ``fontforge.FontView.FontSize``
   * ``fontforge.FontView.FontFamily``
   * ``fontforge.FontView.SerifFamily``
   * ``fontforge.FontView.ScriptFamily``
   * ``fontforge.FontView.FrakturFamily``
   * ``fontforge.FontView.DoubleStruckFamily``
   * ``fontforge.FontView.SansFamily``
   * ``fontforge.FontView.MonoFamily``
   * ``Gdraw.GHVGroupBox``
   * ``Gdraw.ScreenWidthCentimeters``
   * ``Gdraw.ScreenWidthInches``

.. _xres.color:

.. object:: Colors

   Colors may be specified as:

   * rgb(r,g,b)

     where r,g and b are doubles between 0 and 1.0
   * argb(a,r,g,b)

     where a,r,g, and b are doubles between 0 and 1.0

     (The alpha channel is only supported in windows with cairo -- that is the
     glyph view. Alpha 1.0 is fully opaque, alpha 0.0 should be fully transparent,
     values in between are translucent. Since drawing something fully transparent
     has no effect, FontForge treats transparent spot colors as fully opaque).
   * rgb(r%,g%,b%)

     where r, g, and b are doubles between 0% and 100%
   * hsv(h,s,v)

     A color expressed as hue (between 0 and 360), saturation (0.0 and 1.0) and
     value (0.0 and 1.0)
   * hsl(h,s,l)

     A color expressed as hue (between 0 and 360), saturation (0.0 and 1.0) and
     luminosity (0.0 and 1.0)
   * r g b

     where r, g, and b are decimal integers between 0 and 255
   * #rgb

     where r, g, and b are hex digits between 0 and 15 (0xf)
   * #rrggbb

     where rr, gg, bb are hex numbers between 0x00 and 0xff
   * #aarrggbb

     where aa, rr, gg, bb are hex numbers between 0x00 and 0xff

     (The alpha channel is only supported in cairo windows. If alpha is 0, then
     fontforge will treat the color as opaque because drawing a completely
     transparent spot color does nothing).
   * #rrrrggggbbbb

     where rrrr, gggg, bbbb are hex numbers between 0x0000 and 0xffff
   * or one of the color names accepted on the net (red, green, blue, cyan,
     magenta, yellow, white, black, maroon, olive, navy, purple, lime, aqua, teal,
     fuchsia, silver)

.. _xres.Keyboards:

Keyboards and Mice.
-------------------

FontForge assumes that your keyboard has a control key and some equivalent of a
meta key. FontForge works best with a three button mouse.

Almost all keyboards now-a-days will have the needed modifier keys, but which
key is used for what will depend on the keyboard and the OS (for instance
XDarwin and suse linux use quite different mappings for the modifier keys on the
mac keyboard). Usually this is only relevant for menus (and mnemonics).
FontForge tries to guess the keyboard from the environment in which it was
compiled. But with X this may not always be appropriate. So the
":ref:`Gdraw.Keyboard <xres.Keyboard>`" resource above may be used to change
this. (Currently this setting only control the labels that appear in menus for
the hotkeys).

Mice are more problematic. On PCs we usually have two button mice and on mac
single button mice. Many linuxes that run on a PC will give you an option of
simulating the middle button of the mouse by depressing the left and right
buttons simultaneously. FontForge will also allow you to simulate it by holding
down the super key (usually this is the one with the picture of a windows flag
on it) while depressing either mouse button.

On the mac I don't see any good way of simulating a three button mouse...
