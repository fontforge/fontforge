/* Copyright (C) 2000-2012 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <fontforge-config.h>

#include "autohint.h"
#include "autosave.h"
#include "autotrace.h"
#include "autowidth.h"
#include "charview_private.h"
#include "cvruler.h"
#include "cvundoes.h"
#include "dlist.h"
#include "dumppfa.h"
#include "encoding.h"
#include "ffglib.h"
#include "fontforgeui.h"
#include "fvcomposite.h"
#include "fvfonts.h"
#include "gkeysym.h"
#include "gresedit.h"
#include "hotkeys.h"
#include "lookups.h"
#include "mm.h"
#include "namelist.h"
#include "prefs.h"
#include "sfd.h"
#include "spiro.h"
#include "splinefill.h"
#include "splineorder2.h"
#include "splineoverlap.h"
#include "splinesaveafm.h"
#include "splineutil.h"
#include "splineutil2.h"
#include "ustring.h"
#include "utype.h"
#include "wordlistparser.h"

#include <locale.h>
#include <math.h>

#ifdef HAVE_IEEEFP_H
# include <ieeefp.h>		/* Solaris defines isnan in ieeefp rather than math.h */
#endif

/* Barry wants to be able to redefine menu bindings only in the charview (I think) */
/*  the menu parser will first check for something like "CV*Open|Ctl+O", and */
/*  if that fails will strip off "CV*" and check for "Open|Ctl+O" */
#undef H_
#define H_(str) ("CV*" str)

extern void UndoesFreeButRetainFirstN( Undoes** undopp, int retainAmount );
static void CVMoveInWordListByOffset( CharView* cv, int offset );
extern void CVDebugFree( DebugView *dv );

extern int _GScrollBar_Width;

int additionalCharsToShowLimit = 50;

int ItalicConstrained=true;
float arrowAmount=1;
float arrowAccelFactor=10.;
float snapdistance=3.5;
float snapdistancemeasuretool=3.5;
int updateflex = false;
extern int clear_tt_instructions_when_needed;
int use_freetype_with_aa_fill_cv = 1;
int interpCPsOnMotion=false;
int DrawOpenPathsWithHighlight = 1;
#define default_cv_width 540
#define default_cv_height 540
int cv_width = default_cv_width;
int cv_height = default_cv_height;
int cv_show_fill_with_space = 1;

#define prefs_cvEditHandleSize_default 5.0
float prefs_cvEditHandleSize = prefs_cvEditHandleSize_default;

int   prefs_cvInactiveHandleAlpha = 255;

int prefs_cv_show_control_points_always_initially = 0;
int prefs_create_dragging_comparison_outline = 0;

extern struct lconv localeinfo;
extern char *coord_sep;
struct cvshows CVShows = {
	1,		/* show foreground */
	1,		/* show background */
	1,		/* show grid plane */
	1,		/* show horizontal hints */
	1,		/* show vertical hints */
	1,		/* show diagonal hints */
	1,		/* show points */
	0,		/* show filled */
	1,		/* show rulers */
	1,		/* show points which are to be rounded to the ttf grid and aren't on hints */
	1,		/* show x minimum distances */
	1,		/* show y minimum distances */
	1,		/* show horizontal metrics */
	1,		/* show vertical metrics */
	0,		/* mark extrema */
	0,		/* show points of inflection */
	1,		/* show blue values */
	1,		/* show family blues too */
	1,		/* show anchor points */
	0,		/* show control point info when moving them */
	1,		/* show tabs containing names of former glyphs */
	1,		/* show side bearings */
	1,		/* show the names of references */
 	1,		/* snap outlines to pixel grid */
	0,		/* show lines which are almost, but not exactly horizontal or vertical */
	0,		/* show curves which are almost, but not exactly horizontal or vertical at the end-points */
	3,		/* number of em-units a coord difference must be less than to qualify for almost hv */
	1,		/* Check for self-intersections in the element view */
	1		/* In tt debugging, mark changed rasters differently */
};
struct cvshows CVShowsPrevewToggleSavedState;

#define CID_Base	      1001
#define CID_getValueFromUser  CID_Base + 1

static Color pointcol = 0xff0000;
static Color firstpointcol = 0x707000;
static Color selectedpointcol = 0xc8c800;
static int selectedpointwidth = 2;
static Color extremepointcol = 0xc00080;
static Color pointofinflectioncol = 0x008080;
static Color almosthvcol = 0x00ff80;
Color nextcpcol = 0x007090;
Color prevcpcol = 0xcc00cc;
static Color selectedcpcol = 0xffffff;
static Color coordcol = 0x808080;
static Color ascentdescentcol = 0xff808080;
Color widthcol = 0x000000;
static Color widthselcol = 0x00ff00;
static Color lbearingselcol = 0x00ff00;
static Color widthgridfitcol = 0x009800;
static Color lcaretcol = 0x909040;
static Color rastercol = 0xffa0a0a0;		/* Translucent */
static Color rasternewcol = 0xff909090;
static Color rasteroldcol = 0xffc0c0c0;
static Color rastergridcol = 0xffb0b0ff;
static Color rasterdarkcol = 0xff606060;
static Color deltagridcol = 0xcc0000;
static Color italiccoordcol = 0x909090;
static Color metricslabelcol = 0x000000;
static Color hintlabelcol = 0x00cccc;
static Color bluevalstipplecol = 0x808080ff;	/* Translucent */
static Color fambluestipplecol = 0x80ff7070;	/* Translucent */
static Color mdhintcol = 0x80e04040;		/* Translucent */
static Color dhintcol = 0x80d0a0a0;		/* Translucent */
static Color hhintcol = 0x80a0d0a0;		/* Translucent */
static Color vhintcol = 0x80c0c0ff;		/* Translucent */
static Color hflexhintcol = 0x00ff00;
static Color vflexhintcol = 0x00ff00;
static Color conflicthintcol = 0x00ffff;
static Color hhintactivecol = 0x00a000;
static Color vhintactivecol = 0x0000ff;
static Color anchorcol = 0x0040ff;
static Color anchoredoutlinecol = 0x0040ff;
static Color templateoutlinecol = 0x009800;
static Color oldoutlinecol = 0x008000;
static Color transformorigincol = 0x000000;
static Color guideoutlinecol = 0x808080;
static Color guidedragcol = 0x000000;
static Color gridfitoutlinecol = 0x009800;
static Color backoutlinecol = 0x009800;
static Color foreoutlinecol = 0x000000;
static Color clippathcol = 0x0000ff;
static Color openpathcol = 0x40660000;
static Color backimagecol = 0x707070;
static Color fillcol = 0x80707070;		/* Translucent */
static Color tracecol = 0x008000;
static Color rulerbigtickcol = 0x008000;
static Color previewfillcol = 0x0f0f0f;
static Color DraggingComparisonOutlineColor = 0x8800BB00;
static Color DraggingComparisonAlphaChannelOverride = 0x88000000;
static Color foreoutthicklinecol = 0x20707070;
static Color backoutthicklinecol = 0x20707070;
static Color rulercurtickcol = 0xff0000;
Color cvpalettefgcol = 0x000000;
Color cvpalettebgcol = 0xf5fffa;
Color cvpaletteactborcol;
int cvbutton3d = 1;
Color cvbutton3dedgelightcol = 0xe0e0e0;
Color cvbutton3dedgedarkcol = 0x707070;
static GResFont cv_labelfont = GRESFONT_INIT("400 10pt " SANS_UI_FAMILIES);
static GResFont cv_iconfont = GRESFONT_INIT("400 24pt " SERIF_UI_FAMILIES);
GResFont cv_pointnumberfont = GRESFONT_INIT("400 10px " SANS_UI_FAMILIES);
GResFont cv_rulerfont = GRESFONT_INIT("400 10px " SANS_UI_FAMILIES);
extern GResFont cv_measuretoolfont;
extern GResFont layerspalette_font;
extern GResFont toolspalette_font;


int prefs_cv_outline_thickness = 1;

// Format is 0x AA RR GG BB.

static int CV_OnCharSelectorTextChanged( GGadget *g, GEvent *e );
static void CVHScrollSetPos( CharView *cv, int newpos );

static void CVClear(GWindow,GMenuItem *mi, GEvent *);
static void CVMouseMove(CharView *cv, GEvent *event );
static void CVMouseUp(CharView *cv, GEvent *event );
static void CVHScroll(CharView *cv,struct sbevent *sb);
static void CVVScroll(CharView *cv,struct sbevent *sb);
/*static void CVElide(GWindow gw,struct gmenuitem *mi,GEvent *e);*/
static void CVMenuSimplify(GWindow gw,struct gmenuitem *mi,GEvent *e);
static void CVMenuSimplifyMore(GWindow gw,struct gmenuitem *mi,GEvent *e);
static void CVPreviewModeSet(GWindow gw, int checked);
static void CVExposeRulers(CharView *cv, GWindow pixmap);
static void CVDrawGuideLine(CharView *cv, int guide_pos);

static int cvcolsinited = false;

static struct resed charviewpoints_re[] = {
    { N_("Point Color"), "PointColor", rt_color, &pointcol, N_("The color of an on-curve point"), NULL, { 0 }, 0, 0 },
    { N_("First Point Color"), "FirstPointColor", rt_color, &firstpointcol, N_("The color of the point which is the start of a contour"), NULL, { 0 }, 0, 0 },
    { N_("Selected Point Color"), "SelectedPointColor", rt_color, &selectedpointcol, N_("The color of a selected point"), NULL, { 0 }, 0, 0 },
    { N_("Selected Point Width"), "SelectedPointWidth", rt_int, &selectedpointwidth, N_("The width of the line used to draw selected points"), NULL, { 0 }, 0, 0 },
    { N_("Extrema Point Color"), "ExtremePointColor", rt_color, &extremepointcol, N_("The color used to draw points at extrema (if that mode is active)"), NULL, { 0 }, 0, 0 },
    { N_("Point of Inflection Color"), "PointOfInflectionColor", rt_color, &pointofinflectioncol, N_("The color used to draw points of inflection (if that mode is active)"), NULL, { 0 }, 0, 0 },
    { N_("Almost H/V Color"), "AlmostHVColor", rt_color, &almosthvcol, N_("The color used to draw markers for splines which are almost, but not quite horizontal or vertical at their end-points"), NULL, { 0 }, 0, 0 },
    { N_("Next CP Color"), "NextCPColor", rt_color, &nextcpcol, N_("The color used to draw the \"next\" control point of an on-curve point"), NULL, { 0 }, 0, 0 },
    { N_("Prev CP Color"), "PrevCPColor", rt_color, &prevcpcol, N_("The color used to draw the \"previous\" control point of an on-curve point"), NULL, { 0 }, 0, 0 },
    { N_("Selected CP Color"), "SelectedCPColor", rt_color, &selectedcpcol, N_("The color used to draw a selected control point of an on-curve point"), NULL, { 0 }, 0, 0 },
    { N_("Anchor Color"), "AnchorColor", rt_color, &anchorcol, N_("The color of anchor stars"), NULL, { 0 }, 0, 0 },
    { N_("LabelFont"), "LabelFont", rt_font, &cv_labelfont, N_("Used for point and contour names, anchor point names, etc."), NULL, { 0 }, 0, 0 },
    { N_("IconFont"), "IconFont", rt_font, &cv_iconfont, N_("Used for window decoration icon when there is no existing spline or reference"), NULL, { 0 }, 0, 0 },
    { N_("PointNumberFont"), "PointNumberFont", rt_font, &cv_pointnumberfont, N_("Used for point numbers, hints, etc."), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};

static struct resed charviewlinesfills_re[] = {
    { N_("Active Layer Color"), "ForegroundOutlineColor", rt_color, &foreoutlinecol, N_("The color of outlines in the active layer"), NULL, { 0 }, 0, 0 },
    { N_("Active Thick Layer Color"), "ForegroundThickOutlineColor", rt_coloralpha, &foreoutthicklinecol, N_("The color of thick outlines in the active layer"), NULL, { 0 }, 0, 0 },
    { N_("Fill Color"), "FillColor", rt_coloralpha, &fillcol, N_("The color used to fill the outline if that mode is active"), NULL, { 0 }, 0, 0 },
    { N_("Preview Fill Color"), "PreviewFillColor", rt_coloralpha, &previewfillcol, N_("The color used to fill the outline when in preview mode"), NULL, { 0 }, 0, 0 },
    { N_("Open Path Color"), "OpenPathColor", rt_coloralpha, &openpathcol, N_("The color of the open path"), NULL, { 0 }, 0, 0 },
    { N_("Clip Path Color"), "ClipPathColor", rt_color, &clippathcol, N_("The color of the clip path"), NULL, { 0 }, 0, 0 },
    { N_("Inactive Layer Color"), "BackgroundOutlineColor", rt_color, &backoutlinecol, N_("The color of outlines in inactive layers"), NULL, { 0 }, 0, 0 },
    { N_("Inactive Thick Layer Color"), "BackgroundThickOutlineColor", rt_coloralpha, &backoutthicklinecol, N_("The color of thick outlines in inactive layers"), NULL, { 0 }, 0, 0 },
    { N_("Width Color"), "WidthColor", rt_color, &widthcol, N_("The color of the line marking the advance width"), NULL, { 0 }, 0, 0 },
    { N_("Selected Width Color"), "WidthSelColor", rt_color, &widthselcol, N_("The color of the line marking the advance width when it is selected"), NULL, { 0 }, 0, 0 },
    { N_("Selected LBearing Color"), "LBearingSelColor", rt_color, &lbearingselcol, N_("The color of the line marking the left bearing when it is selected"), NULL, { 0 }, 0, 0 },
    { N_("Ligature Caret Color"), "LigatureCaretColor", rt_color, &lcaretcol, N_("The color of the line(s) marking ligature carets"), NULL, { 0 }, 0, 0 },
    { N_("Anchored Line Color"), "AnchoredOutlineColor", rt_color, &anchoredoutlinecol, N_("The color of another glyph drawn in the current view to show where it would be placed by an anchor lookup"), NULL, { 0 }, 0, 0 },
    { N_("Coordinate Line Color"), "CoordinateLineColor", rt_color, &coordcol, N_("Color of the x=0 and y=0 lines"), NULL, { 0 }, 0, 0 },
    { N_("Ascent/Descent Color"), "AscentDescentColor", rt_coloralpha, &ascentdescentcol, N_("Color of the ascent and descent lines"), NULL, { 0 }, 0, 0 },
    { N_("Italic Coord. Color"), "ItalicCoordColor", rt_color, &italiccoordcol, NULL, NULL, { 0 }, 0, 0 },
    { N_("Metrics Label Color"), "MetricsLabelColor", rt_color, &metricslabelcol, NULL, NULL, { 0 }, 0, 0 },
    { N_("Template Outline Color"), "TemplateOutlineColor", rt_color, &templateoutlinecol, NULL, NULL, { 0 }, 0, 0 },
    { N_("Ruler Big Tick Color"), "RulerBigTickColor", rt_color, &rulerbigtickcol, N_("The color used to draw the large tick marks in rulers."), NULL, { 0 }, 0, 0 },
    { N_("Ruler Current Tick Color"), "RulerCurrentTickColor", rt_color, &rulercurtickcol, N_("The color used to draw a vertical and a horizontal tick corresponding to the mouse position."), NULL, { 0 }, 0, 0 },
    { N_("Ruler Font"), "Ruler Font", rt_font, &cv_rulerfont, N_("Used for ruler numbers and other ruler notations."), NULL, { 0 }, 0, 0 },
    { N_("Guide Layer Color"), "GuideOutlineColor", rt_color, &guideoutlinecol, NULL, NULL, { 0 }, 0, 0 },
    { N_("Guide Drag Color"), "GuideDragColor", rt_color, &guidedragcol, N_("The color used to display a new guide line dragged from the ruler."), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};

static struct resed charviewtools_re[] = {
    { N_("Trace Color"), "TraceColor", rt_color, &tracecol, NULL, NULL, { 0 }, 0, 0 },
    { N_("Old Outline Color"), "OldOutlineColor", rt_color, &oldoutlinecol, NULL, NULL, { 0 }, 0, 0 },
    { N_("Transform Original Color"), "TransformOriginColor", rt_color, &transformorigincol, NULL, NULL, { 0 }, 0, 0 },
    { N_("Dragging Comparison Outline Color"), "DraggingComparisonOutlineColor", rt_coloralpha, &DraggingComparisonOutlineColor, N_("The color used to draw the outline of the old spline when you are interactively modifying a glyph"), NULL, { 0 }, 0, 0 },
    { N_("Dragging Comparison Alpha Channel"), "DraggingComparisonAlphaChannelOverride", rt_coloralpha, &DraggingComparisonAlphaChannelOverride, N_("Only the alpha value is used and if non zero it will set the alpha channel for the control points, bezier information and other non spline indicators for the Dragging Comparison Outline spline"), NULL, { 0 }, 0, 0 },
    { N_("Measure Tool Line Color"), "MeasureToolLineColor", rt_color, &measuretoollinecol, N_("The color used to draw the measure tool line."), NULL, { 0 }, 0, 0 },
    { N_("Measure Tool Point Color"), "MeasureToolPointColor", rt_color, &measuretoolpointcol, N_("The color used to draw the measure tool points."), NULL, { 0 }, 0, 0 },
    { N_("Measure Tool Point Snapped Color"), "MeasureToolPointSnappedColor", rt_color, &measuretoolpointsnappedcol, N_("The color used to draw the measure tool points when snapped."), NULL, { 0 }, 0, 0 },
    { N_("Measure Tool Canvas Number Color"), "MeasureToolCanvasNumbersColor", rt_color, &measuretoolcanvasnumberscol, N_("The color used to draw the measure tool numbers on the canvas."), NULL, { 0 }, 0, 0 },
    { N_("Measure Tool Canvas Number Snapped Color"), "MeasureToolCanvasNumbersSnappedColor", rt_color, &measuretoolcanvasnumberssnappedcol, N_("The color used to draw the measure tool numbers on the canvas when snapped."), NULL, { 0 }, 0, 0 },
    { N_("Measure Tool Windows Foreground Color"), "MeasureToolWindowForeground", rt_color, &measuretoolwindowforegroundcol, N_("The measure tool window foreground color."), NULL, { 0 }, 0, 0 },
    { N_("Measure Tool Windows Background Color"), "MeasureToolWindowBackground", rt_color, &measuretoolwindowbackgroundcol, N_("The measure tool window background color."), NULL, { 0 }, 0, 0 },
    { N_("MeasureToolFont"), "MeasureToolFont", rt_font, &cv_measuretoolfont, NULL, NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};

static struct resed charviewhints_re[] = {
    { N_("Blue Values Color"), "BlueValuesStippledColor", rt_coloralpha, &bluevalstipplecol, N_("The color used to mark blue zones in the blue values entry of the private dictionary"), NULL, { 0 }, 0, 0 },
    { N_("Family Blue Color"), "FamilyBlueStippledColor", rt_coloralpha, &fambluestipplecol, N_("The color used to mark blue zones in the family blues entry of the private dictionary"), NULL, { 0 }, 0, 0 },
    { N_("Minimum Distance Hint Color"), "MDHintColor", rt_coloralpha, &mdhintcol, N_("The color used to draw minimum distance hints"), NULL, { 0 }, 0, 0 },
    { N_("Hint Label Color"), "HintLabelColor", rt_color, &hintlabelcol,NULL, NULL, { 0 }, 0, 0 },
    { N_("Diagonal Hint Color"), "DHintColor", rt_coloralpha, &dhintcol, N_("The color used to draw diagonal hints"), NULL, { 0 }, 0, 0 },
    { N_("Horiz. Hint Color"), "HHintColor", rt_coloralpha, &hhintcol, N_("The color used to draw horizontal hints"), NULL, { 0 }, 0, 0 },
    { N_("Vert. Hint Color"), "VHintColor", rt_coloralpha, &vhintcol, N_("The color used to draw vertical hints"), NULL, { 0 }, 0, 0 },
    { N_("HFlex Hint Color"), "HFlexHintColor", rt_color, &hflexhintcol, NULL, NULL, { 0 }, 0, 0 },
    { N_("VFlex Hint Color"), "VFlexHintColor", rt_color, &vflexhintcol, NULL, NULL, { 0 }, 0, 0 },
    { N_("Conflict Hint Color"), "ConflictHintColor", rt_color, &conflicthintcol, N_("The color used to draw a hint which conflicts with another"), NULL, { 0 }, 0, 0 },
    { N_("HHint Active Color"), "HHintActiveColor", rt_color, &hhintactivecol, N_("The color used to draw the active horizontal hint which the Review Hints dialog is examining"), NULL, { 0 }, 0, 0 },
    { N_("VHint Active Color"), "VHintActiveColor", rt_color, &vhintactivecol, N_("The color used to draw the active vertical hint which the Review Hints dialog is examining"), NULL, { 0 }, 0, 0 },
    { N_("Delta Grid Color"), "DeltaGridColor", rt_color, &deltagridcol, N_("Indicates a notable grid pixel when suggesting deltas."), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};

static struct resed charviewraster_re[] = {
    { N_("Grid Fit Color"), "GridFitOutlineColor", rt_color, &gridfitoutlinecol, N_("The color of grid-fit outlines"), NULL, { 0 }, 0, 0 },
    { N_("Grid Fit Width Color"), "GridFitWidthColor", rt_color, &widthgridfitcol, N_("The color of the line marking the grid-fit advance width"), NULL, { 0 }, 0, 0 },
    { N_("Raster Color"), "RasterColor", rt_coloralpha, &rastercol, N_("The color of grid-fit (and other) raster blocks"), NULL, { 0 }, 0, 0 },
    { N_("Raster New Color"), "RasterNewColor", rt_coloralpha, &rasternewcol, N_("The color of raster blocks which have just been turned on (in the debugger when an instruction moves a point)"), NULL, { 0 }, 0, 0 },
    { N_("Raster Old Color"), "RasterOldColor", rt_coloralpha, &rasteroldcol, N_("The color of raster blocks which have just been turned off (in the debugger when an instruction moves a point)"), NULL, { 0 }, 0, 0 },
    { N_("Raster Grid Color"), "RasterGridColor", rt_coloralpha, &rastergridcol, NULL, NULL, { 0 }, 0, 0 },
    { N_("Raster Dark Color"), "RasterDarkColor", rt_coloralpha, &rasterdarkcol, N_("When debugging in grey-scale this is the color of a raster block which is fully covered."), NULL, { 0 }, 0, 0 },
    { N_("Background Image Color"), "BackgroundImageColor", rt_coloralpha, &backimagecol, N_("The color used to draw bitmap (single bit) images which do not specify a clut"), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};

static struct resed cvpalettes_re[] = {
    { N_("Palette Foreground Color"), "CVPaletteForegroundColor", rt_color, &cvpalettefgcol, N_("When buttons are 3D, the light edge color"), NULL, { 0 }, 0, 0 },
    { N_("Palette Background Color"), "CVPaletteBackgroundColor", rt_color, &cvpalettebgcol, N_("When buttons are 3D, the light edge color"), NULL, { 0 }, 0, 0 },
    { N_("3D Light Edge Color"), "Button3DEdgeLightColor", rt_color, &cvbutton3dedgelightcol, N_("When buttons are 3D, the light edge color"), NULL, { 0 }, 0, 0 },
    { N_("3D Dark Edge Color"), "Button3DEdgeDarkColor", rt_color, &cvbutton3dedgedarkcol, N_("When buttons are 3D, the dark edge color"), NULL, { 0 }, 0, 0 },
    { N_("3D Buttons"), "Button3d", rt_bool, &cvbutton3d, N_("Whether buttons on the CharView pallettes have a 3D appearance"), NULL, { 0 }, 0, 0 },
    {N_("LayersPalette.Font"), "LayersPalette.Font", rt_font, &layerspalette_font, N_("Font used in the outline view layers palette"), NULL, { 0 }, 0, 0 },
    {N_("ToolsPalette.Font"), "ToolsPalette.Font", rt_font, &toolspalette_font, N_("Font used in the outline view tools palette"), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};

/* return 1 if anything changed */
static void update_spacebar_hand_tool(CharView *cv) {
    if ( GDrawKeyState(cv->v, ' ') ) {
	if ( !cv->spacebar_hold  && !cv_auto_goto ) {
	    cv->spacebar_hold = 1;
	    cv->b1_tool_old = cv->b1_tool;
	    cv->b1_tool = cvt_hand;
	    cv->active_tool = cvt_hand;
	    CVMouseDownHand(cv);
	    CVPreviewModeSet(cv->gw, cv_show_fill_with_space);
	}
    } else {
	if ( cv->spacebar_hold ) {
	    cv->spacebar_hold = 0;
	    cv->b1_tool = cv->b1_tool_old;
	    cv->active_tool = cvt_none;
	    cv->b1_tool_old = cvt_none;
	    CVPreviewModeSet(cv->gw, false);
	}
    }
}


int CVInSpiro( CharView *cv )
{
    int inspiro = 0;
    int canspiro = hasspiro();
    if( cv ) 
	inspiro = canspiro && cv->b.sc->inspiro;
    return inspiro;
}

/**
 * Returns the number of points which are currently selected in this
 * charview. Handy for menus and the like which might like to grey out
 * if there are <2, or <3 points actively selected.
 */
int CVCountSelectedPoints(CharView *cv) {
    SplinePointList *spl;
    Spline *spline, *first;
    int ret = 0;

    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next ) {
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    if( spline == spl->first->next ) {
		if ( spline->from->selected ) {
		    ret++;
		}
	    }
	    if ( spline->to->selected ) {
		if( spline->to != spl->first->next->from )
		    ret++;
	    }
	    if ( first==NULL ) {
		first = spline;
	    }
	}
    }
    return ret;
}



/* floor(pt) would _not_ be more correct, as we want
 * shapes not to cross axes multiple times while scaling.
 */
static double rpt(CharView *cv, double pt) {
    return cv->snapoutlines ? rint(pt) : pt;
}

static int shouldShowFilledUsingCairo(CharView *cv) {
    if ( cv->showfilled && GDrawHasCairo(cv->v) & gc_buildpath ) {
	return 1;
    }
    return 0;
}

extern GResInfo charviewpoints_ri, charviewlinesfills_ri, charviewtools_ri;
extern GResInfo charviewhints_ri, charviewraster_ri, cvpalettes_ri;
void CVColInit( void ) {
    if ( cvcolsinited )
	return;
    GResEditDoInit(&charviewpoints_ri);
    GResEditDoInit(&charviewlinesfills_ri);
    GResEditDoInit(&charviewtools_ri);
    GResEditDoInit(&charviewhints_ri);
    GResEditDoInit(&charviewraster_ri);
    GResEditDoInit(&cvpalettes_ri);
    cvcolsinited = true;
}

GDevEventMask input_em[] = {
	/* Event masks for wacom devices */
    /* negative utility in opening Mouse1 */
    /* No point in distinguishing cursor from core mouse */
    { (1<<et_mousemove)|(1<<et_mousedown)|(1<<et_mouseup)|(1<<et_char), "stylus" },
    { (1<<et_mousemove)|(1<<et_mousedown)|(1<<et_mouseup), "eraser" },
    { 0, NULL }
};
const int input_em_cnt = sizeof(input_em)/sizeof(input_em[0])-1;

/* Positions on the info line */
#define RPT_BASE	5		/* Place to draw the pointer icon */
#define RPT_DATA	13		/* x,y text after above */
#define SPT_BASE	83		/* Place to draw selected pt icon */
#define SPT_DATA	97		/* Any text for it */
#define SOF_BASE	157		/* Place to draw selection to pointer icon */
#define SOF_DATA	179		/* Any text for it */
#define SDS_BASE	259		/* Place to draw distance icon */
#define SDS_DATA	281		/* Any text for it */
#define SAN_BASE	331		/* Place to draw angle icon */
#define SAN_DATA	353		/* Any text for it */
#define MAG_BASE	383		/* Place to draw magnification icon */
#define MAG_DATA	394		/* Any text for it */
#define LAYER_DATA	454		/* Text to show the current layer */
#define CODERANGE_DATA	574		/* Text to show the current code range (if the debugger be active) */
#define FLAGS_DATA	724		/* Text to show the current drawmode flags */

CharViewTab* CVGetActiveTab(CharView *cv) {
    if (cv->tabs == NULL) {
        // This happens when used by the bitmap view, which doesn't support tabs
        return &cv->cvtabs[0];
    }
    int tab = GTabSetGetSel(cv->tabs);
    return &cv->cvtabs[tab];
}

void CVDrawRubberRect(GWindow pixmap, CharView *cv) {
    GRect r;
    CharViewTab *tab = CVGetActiveTab(cv);
    if ( !cv->p.rubberbanding )
return;
    r.x =  tab->xoff + rint(cv->p.cx*tab->scale);
    r.y = -tab->yoff + cv->height - rint(cv->p.cy*tab->scale);
    r.width = rint( (cv->p.ex-cv->p.cx)*tab->scale);
    r.height = -rint( (cv->p.ey-cv->p.cy)*tab->scale);
    if ( r.width<0 ) {
	r.x += r.width;
	r.width = -r.width;
    }
    if ( r.height<0 ) {
	r.y += r.height;
	r.height = -r.height;
    }
    GDrawSetDashedLine(pixmap,2,2,0);
    GDrawSetLineWidth(pixmap,0);
    GDrawDrawRect(pixmap,&r,oldoutlinecol);
    GDrawSetDashedLine(pixmap,0,0,0);
}

static void CVDrawRubberLine(GWindow pixmap, CharView *cv) {
    int x,y, xend,yend;
    CharViewTab* tab = CVGetActiveTab(cv);
    Color col = cv->active_tool==cvt_ruler ? measuretoollinecol : oldoutlinecol;
    if ( !cv->p.rubberlining )
return;
    x =  tab->xoff + rint(cv->p.cx*tab->scale);
    y = -tab->yoff + cv->height - rint(cv->p.cy*tab->scale);
    xend =  tab->xoff + rint(cv->info.x*tab->scale);
    yend = -tab->yoff + cv->height - rint(cv->info.y*tab->scale);
	GDrawSetLineWidth(pixmap,0);
    GDrawDrawLine(pixmap,x,y,xend,yend,col);
}

static void CVDrawBB(CharView *cv, GWindow pixmap, DBounds *bb) {
    GRect r;
    CharViewTab* tab = CVGetActiveTab(cv);
    int off = tab->xoff+cv->height-tab->yoff;

    r.x =  tab->xoff + rint(bb->minx*tab->scale);
    r.y = -tab->yoff + cv->height - rint(bb->maxy*tab->scale);
    r.width = rint((bb->maxx-bb->minx)*tab->scale);
    r.height = rint((bb->maxy-bb->miny)*tab->scale);
    GDrawSetDashedLine(pixmap,1,1,off);
    GDrawDrawRect(pixmap,&r,GDrawGetDefaultForeground(NULL));
    GDrawSetDashedLine(pixmap,0,0,0);
}

/* Sigh. I have to do my own clipping because at large magnifications */
/*  things can easily exceed 16 bits */
static int CVSplineOutside(CharView *cv, Spline *spline) {
    CharViewTab* tab = CVGetActiveTab(cv);
    int x[4], y[4];

    x[0] =  tab->xoff + rint(spline->from->me.x*tab->scale);
    y[0] = -tab->yoff + cv->height - rint(spline->from->me.y*tab->scale);

    x[1] =  tab->xoff + rint(spline->to->me.x*tab->scale);
    y[1] = -tab->yoff + cv->height - rint(spline->to->me.y*tab->scale);

    if ( spline->from->nonextcp && spline->to->noprevcp ) {
	if ( (x[0]<0 && x[1]<0) || (x[0]>=cv->width && x[1]>=cv->width) ||
		(y[0]<0 && y[1]<0) || (y[0]>=cv->height && y[1]>=cv->height) )
return( true );
    } else {
	x[2] =  tab->xoff + rint(spline->from->nextcp.x*tab->scale);
	y[2] = -tab->yoff + cv->height - rint(spline->from->nextcp.y*tab->scale);
	x[3] =  tab->xoff + rint(spline->to->prevcp.x*tab->scale);
	y[3] = -tab->yoff + cv->height - rint(spline->to->prevcp.y*tab->scale);
	if ( (x[0]<0 && x[1]<0 && x[2]<0 && x[3]<0) ||
		(x[0]>=cv->width && x[1]>=cv->width && x[2]>=cv->width && x[3]>=cv->width ) ||
		(y[0]<0 && y[1]<0 && y[2]<0 && y[3]<0 ) ||
		(y[0]>=cv->height && y[1]>=cv->height && y[2]>=cv->height && y[3]>=cv->height) )
return( true );
    }

return( false );
}

static int CVLinesIntersectScreen(CharView *cv, LinearApprox *lap) {
    CharViewTab* tab = CVGetActiveTab(cv);
    LineList *l;
    int any = false;
    int x,y;
    int bothout;

    for ( l=lap->lines; l!=NULL; l=l->next ) {
	l->asend.x = l->asstart.x = tab->xoff + l->here.x;
	l->asend.y = l->asstart.y = -tab->yoff + cv->height-l->here.y;
	l->flags = 0;
	if ( l->asend.x<0 || l->asend.x>=cv->width || l->asend.y<0 || l->asend.y>=cv->height ) {
	    l->flags = cvli_clipped;
	    any = true;
	}
    }
    if ( !any ) {
	for ( l=lap->lines; l!=NULL; l=l->next )
	    l->flags = cvli_onscreen;
	lap->any = true;
return( true );
    }

    any = false;
    for ( l=lap->lines; l->next!=NULL; l=l->next ) {
	if ( !(l->flags&cvli_clipped) && !(l->next->flags&cvli_clipped) )
	    l->flags = cvli_onscreen;
	else {
	    bothout = (l->flags&cvli_clipped) && (l->next->flags&cvli_clipped);
	    if (( l->asstart.x<0 && l->next->asend.x>0 ) ||
		    ( l->asstart.x>0 && l->next->asend.x<0 )) {
		y = -(l->next->asend.y-l->asstart.y)*(double)l->asstart.x/(l->next->asend.x-l->asstart.x) +
			l->asstart.y;
		if ( l->asstart.x<0 ) {
		    l->asstart.x = 0;
		    l->asstart.y = y;
		} else {
		    l->next->asend.x = 0;
		    l->next->asend.y = y;
		}
	    } else if ( l->asstart.x<0 && l->next->asend.x<0 )
    continue;
	    if (( l->asstart.x<cv->width && l->next->asend.x>cv->width ) ||
		    ( l->asstart.x>cv->width && l->next->asend.x<cv->width )) {
		y = (l->next->asend.y-l->asstart.y)*(double)(cv->width-l->asstart.x)/(l->next->asend.x-l->asstart.x) +
			l->asstart.y;
		if ( l->asstart.x>cv->width ) {
		    l->asstart.x = cv->width;
		    l->asstart.y = y;
		} else {
		    l->next->asend.x = cv->width;
		    l->next->asend.y = y;
		}
	    } else if ( l->asstart.x>cv->width && l->next->asend.x>cv->width )
    continue;
	    if (( l->asstart.y<0 && l->next->asend.y>0 ) ||
		    ( l->asstart.y>0 && l->next->asend.y<0 )) {
		x = -(l->next->asend.x-l->asstart.x)*(double)l->asstart.y/(l->next->asend.y-l->asstart.y) +
			l->asstart.x;
		if (( x<0 || x>=cv->width ) && bothout )
    continue;			/* Not on screen */;
		if ( l->asstart.y<0 ) {
		    l->asstart.y = 0;
		    l->asstart.x = x;
		} else {
		    l->next->asend.y = 0;
		    l->next->asend.x = x;
		}
	    } else if ( l->asstart.y<0 && l->next->asend.y< 0 )
    continue;
	    if (( l->asstart.y<cv->height && l->next->asend.y>cv->height ) ||
		    ( l->asstart.y>cv->height && l->next->asend.y<cv->height )) {
		x = (l->next->asend.x-l->asstart.x)*(double)(cv->height-l->asstart.y)/(l->next->asend.y-l->asstart.y) +
			l->asstart.x;
		if (( x<0 || x>=cv->width ) && bothout )
    continue;			/* Not on screen */;
		if ( l->asstart.y>cv->height ) {
		    l->asstart.y = cv->height;
		    l->asstart.x = x;
		} else {
		    l->next->asend.y = cv->height;
		    l->next->asend.x = x;
		}
	    } else if ( l->asstart.y>cv->height && l->next->asend.y>cv->height )
    continue;
	    l->flags |= cvli_onscreen;
	    any = true;
	}
    }
    lap->any = any;
return( any );
}

typedef struct gpl { struct gpl *next; GPoint *gp; int cnt; } GPointList;

static void GPLFree(GPointList *gpl) {
    GPointList *next;

    while ( gpl!=NULL ) {
	next = gpl->next;
	free( gpl->gp );
	free( gpl );
	gpl = next;
    }
}

/* Before we did clipping this was a single polygon. Now it is a set of */
/*  sets of line segments. If no clipping is done, then we end up with */
/*  one set which is the original polygon, otherwise we get the segments */
/*  which are inside the screen. Each set of segments is contiguous */
static GPointList *MakePoly(CharView *cv, SplinePointList *spl) {
    int i, len;
    LinearApprox *lap;
    LineList *line, *prev;
    Spline *spline, *first;
    GPointList *head=NULL, *last=NULL, *cur;
    int closed;

    for ( i=0; i<2; ++i ) {
	len = 0; first = NULL;
	closed = true;
	cur = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    if ( !CVSplineOutside(cv,spline) && !isnan(spline->splines[0].a) && !isnan(spline->splines[1].a)) {
        CharViewTab* tab = CVGetActiveTab(cv);
		lap = SplineApproximate(spline,tab->scale);
		if ( i==0 )
		    CVLinesIntersectScreen(cv,lap);
		if ( lap->any ) {
		    for ( prev = lap->lines, line=prev->next; line!=NULL; prev=line, line=line->next ) {
			if ( !(prev->flags&cvli_onscreen) ) {
			    closed = true;
		    continue;
			}
			if ( closed || (prev->flags&cvli_clipped) ) {
			    if ( i==0 ) {
				cur = calloc(1,sizeof(GPointList));
				if ( head==NULL )
				    head = cur;
				else {
				    last->cnt = len;
				    last->next = cur;
				}
				last = cur;
			    } else {
				if ( cur==NULL )
				    cur = head;
				else
				    cur = cur->next;
				cur->gp = malloc(cur->cnt*sizeof(GPoint));
				cur->gp[0].x = prev->asstart.x;
				cur->gp[0].y = prev->asstart.y;
			    }
			    len=1;
			    closed = false;
			}
			if ( i!=0 ) {
			    if ( len>=cur->cnt )
				fprintf( stderr, "Clipping is screwed up, about to die %d (should be less than %d)\n", len, cur->cnt );
			    cur->gp[len].x = line->asend.x;
			    cur->gp[len].y = line->asend.y;
			}
			++len;
			if ( line->flags&cvli_clipped )
			    closed = true;
		    }
		} else
		    closed = true;
	    } else
		closed = true;
	    if ( first==NULL ) first = spline;
	}
	if ( i==0 && cur!=NULL )
	    cur->cnt = len;
    }
return( head );
}

static void DrawTangentPoint( GWindow pixmap, int x, int y,
			      BasePoint *unit, int outline, Color col )
{
    int dir;
    const int gp_sz = 4;
    GPoint gp[5];

    dir = 0;
    if ( unit->x!=0 || unit->y!=0 ) {
	float dx = unit->x, dy = unit->y;
	if ( dx<0 ) dx= -dx;
	if ( dy<0 ) dy= -dy;
	if ( dx>2*dy ) {
	    if ( unit->x>0 ) dir = 0 /* right */;
	    else dir = 1 /* left */;
	} else if ( dy>2*dx ) {
	    if ( unit->y>0 ) dir = 2 /* up */;
	    else dir = 3 /* down */;
	} else {
	    if ( unit->y>0 && unit->x>0 ) dir=4;
	    else if ( unit->x>0 ) dir=5;
	    else if ( unit->y>0 ) dir=7;
	    else dir = 6;
	}
    }

    float sizedelta = 4;
    if( prefs_cvEditHandleSize > prefs_cvEditHandleSize_default )
	sizedelta *= prefs_cvEditHandleSize / prefs_cvEditHandleSize_default;

    if ( dir==1 /* left */ || dir==0 /* right */) {
	gp[0].y = y; gp[0].x = (dir==0)?x+sizedelta:x-sizedelta;
	gp[1].y = y-sizedelta; gp[1].x = x;
	gp[2].y = y+sizedelta; gp[2].x = x;
    } else if ( dir==2 /* up */ || dir==3 /* down */ ) {
	gp[0].x = x; gp[0].y = dir==2?y-sizedelta:y+sizedelta;	/* remember screen coordinates are backwards in y from character coords */
	gp[1].x = x-sizedelta; gp[1].y = y;
	gp[2].x = x+sizedelta; gp[2].y = y;
    } else {
	/* at a 45 angle, a value of 4 looks too small. I probably want 4*1.414 */
	sizedelta = 5;
	if( prefs_cvEditHandleSize > prefs_cvEditHandleSize_default )
	    sizedelta *= prefs_cvEditHandleSize / prefs_cvEditHandleSize_default;
	int xdiff = unit->x > 0 ?   sizedelta  : -1*sizedelta;
	int ydiff = unit->y > 0 ? -1*sizedelta :    sizedelta;

	gp[0].x = x+xdiff/2; gp[0].y = y+ydiff/2;
	gp[1].x = gp[0].x-xdiff; gp[1].y = gp[0].y;
	gp[2].x = gp[0].x; gp[2].y = gp[0].y-ydiff;
    }
    gp[3] = gp[0];
    if ( outline )
	GDrawDrawPoly(pixmap,gp,gp_sz,col);
    else
	GDrawFillPoly(pixmap,gp,4,col);
}

static GRect* DrawPoint_SetupRectForSize( GRect* r, int cx, int cy, float sz )
{
    float sizedelta = sz;
    if( prefs_cvEditHandleSize > prefs_cvEditHandleSize_default )
	sizedelta *= prefs_cvEditHandleSize / prefs_cvEditHandleSize_default;

    r->x = cx - sizedelta;
    r->y = cy - sizedelta;
    r->width  = 1 + sizedelta * 2;
    r->height = 1 + sizedelta * 2;
    return r;
}


static Color MaybeMaskColorToAlphaChannelOverride( Color c, Color AlphaChannelOverride )
{
    if( AlphaChannelOverride )
    {
	c &= 0x00FFFFFF;
	c |= (AlphaChannelOverride & 0xFF000000);
    }
    return c;
}

/*
 * Format the given real value to .001 precision, discarding trailing
 * zeroes, and the decimal point if not needed.
 */
static int FmtReal(char *buf, int buflen, real r)
{
    if (buf && buflen > 0) {
	int ct;

	if ((ct = snprintf(buf, buflen, "%.3f", r)) >= buflen)
	    ct = buflen-1;
    
	if (strchr(buf, '.')) {
	    while (buf[ct-1] == '0') {
		--ct;
	    }
	    if (buf[ct-1] == '.') {
		--ct;
	    }
	    buf[ct] = '\0';
	}
	return ct;
    }
    return -1;
}	    
	
/*
 * Write the given spline point's coordinates in the form (xxx,yyy)
 * into the buffer provided, rounding fractions to the nearest .001
 * and discarding trailing zeroes.
 */ 
static int SplinePointCoords(char *buf, int buflen, SplinePoint *sp)
{
    if (buf && buflen > 0) {
	int ct = 0, clen;

	/* 4 chars for parens, comma and trailing NUL, then divide what
	   remains between the two coords */
	clen = (buflen - 4) / 2;
	if (clen > 0) {
	    buf[ct++] = '(';
	    ct += FmtReal(buf+ct, clen, sp->me.x);
	    buf[ct++] = ',';
	    ct += FmtReal(buf+ct, clen, sp->me.y);
	    buf[ct++] = ')';
	}
	buf[ct] = '\0';

	return ct;
    }
    return -1;
}

static void DrawPoint( CharView *cv, GWindow pixmap, SplinePoint *sp,
		       SplineSet *spl, int onlynumber, int truetype_markup,
                       Color AlphaChannelOverride )
{
    GRect r;
    int x, y, cx, cy;
    CharViewTab* tab = CVGetActiveTab(cv);
    Color col = sp==spl->first ? firstpointcol : pointcol;
    int pnum;
    char buf[16];
    int isfake;

    if ( cv->markextrema && SpIsExtremum(sp) && sp!=spl->first )
	col = extremepointcol;
    if ( sp->selected )
	col = selectedpointcol;
    else {
	col = col&0x00ffffff;
	col |= prefs_cvInactiveHandleAlpha << 24;
    }

    col = MaybeMaskColorToAlphaChannelOverride( col, AlphaChannelOverride );
    Color subcolmasked;
    Color nextcpcolmasked = MaybeMaskColorToAlphaChannelOverride( nextcpcol, AlphaChannelOverride );
    Color prevcpcolmasked = MaybeMaskColorToAlphaChannelOverride( prevcpcol, AlphaChannelOverride );
    Color selectedpointcolmasked = MaybeMaskColorToAlphaChannelOverride( selectedpointcol, AlphaChannelOverride );
    Color selectedcpcolmasked = MaybeMaskColorToAlphaChannelOverride( selectedcpcol, AlphaChannelOverride );

    x =  tab->xoff + rint(sp->me.x*tab->scale);
    y = -tab->yoff + cv->height - rint(sp->me.y*tab->scale);
    if ( x<-4000 || y<-4000 || x>cv->width+4000 || y>=cv->height+4000 )
return;

    /* draw the control points if it's selected */
    if ( sp->selected
	 || cv->showpointnumbers
	 || cv->alwaysshowcontrolpoints
	 || cv->show_ft_results
	 || cv->dv )
    {
	if ( !sp->nonextcp ) {
	    cx =  tab->xoff + rint(sp->nextcp.x*tab->scale);
	    cy = -tab->yoff + cv->height - rint(sp->nextcp.y*tab->scale);
	    if ( cx<-100 ) {		/* Clip */
		cy = cx==x ? x : (cy-y) * (double)(-100-x)/(cx-x) + y;
		cx = -100;
	    } else if ( cx>cv->width+100 ) {
		cy = cx==x ? x : (cy-y) * (double)(cv->width+100-x)/(cx-x) + y;
		cx = cv->width+100;
	    }
	    if ( cy<-100 ) {
		cx = cy==y ? y : (cx-x) * (double)(-100-y)/(cy-y) + x;
		cy = -100;
	    } else if ( cy>cv->height+100 ) {
		cx = cy==y ? y : (cx-x) * (double)(cv->height+100-y)/(cy-y) + x;
		cy = cv->height+100;
	    }
	    subcolmasked = nextcpcolmasked;

	    //
	    // If the next BCP is selected we should decorate the
	    // drawing to let the user know that. The primary (last)
	    // selected BCP is drawn with a backing rectangle of size
	    // 3, the secondary BCP (2nd, 3rd, 4th last selected BCP)
	    // are drawn with slightly smaller highlights.
	    //
	    if( !onlynumber && SPIsNextCPSelected( sp, cv ))
	    {
		float sz = 2;
		if( SPIsNextCPSelectedSingle( sp, cv ))
		    sz *= 1.5;

		DrawPoint_SetupRectForSize( &r, cx, cy, sz );
		GDrawFillRect(pixmap,&r, nextcpcol);
		subcolmasked = selectedcpcolmasked;
	    }
	    else if ( truetype_markup )
	    {
		if ( sp->flexy ) {
		    /* cp is about to be moved (or changed in some other way) */
		    DrawPoint_SetupRectForSize( &r, cx, cy, 3 );
		    GDrawFillRect(pixmap,&r, selectedpointcol);
		}
		if ( sp->flexx ) {
		    /* cp is a reference point */
		    DrawPoint_SetupRectForSize( &r, cx, cy, 5 );
		    GDrawDrawElipse(pixmap,&r,selectedpointcol );
		}
	    }
	    if ( !onlynumber )
	    {
		float sizedelta = 3;
		if( prefs_cvEditHandleSize > prefs_cvEditHandleSize_default )
		    sizedelta *= prefs_cvEditHandleSize / prefs_cvEditHandleSize_default;
		GDrawDrawLine(pixmap,x,y,cx,cy, nextcpcolmasked );
		GDrawDrawLine(pixmap,cx-sizedelta,cy-sizedelta,cx+sizedelta,cy+sizedelta,subcolmasked);
		GDrawDrawLine(pixmap,cx+sizedelta,cy-sizedelta,cx-sizedelta,cy+sizedelta,subcolmasked);
	    }
	    if ( cv->showpointnumbers || cv->show_ft_results || cv->dv ) {
		pnum = sp->nextcpindex;
		if ( pnum!=0xffff && pnum!=0xfffe ) {
                    if (cv->showpointnumbers == 2)
			SplinePointCoords( buf, sizeof(buf), sp);
                    else
		        sprintf( buf,"%d", pnum );
		    GDrawDrawText8(pixmap,cx,cy-6,buf,-1,nextcpcol);
		}
	    }
	}
	if ( !sp->noprevcp ) {
	    cx =  tab->xoff + rint(sp->prevcp.x*tab->scale);
	    cy = -tab->yoff + cv->height - rint(sp->prevcp.y*tab->scale);
	    if ( cx<-100 ) {		/* Clip */
		cy = cx==x ? x : (cy-y) * (double)(-100-x)/(cx-x) + y;
		cx = -100;
	    } else if ( cx>cv->width+100 ) {
		cy = cx==x ? x : (cy-y) * (double)(cv->width+100-x)/(cx-x) + y;
		cx = cv->width+100;
	    }
	    if ( cy<-100 ) {
		cx = cy==y ? y : (cx-x) * (double)(-100-y)/(cy-y) + x;
		cy = -100;
	    } else if ( cy>cv->height+100 ) {
		cx = cy==y ? y : (cx-x) * (double)(cv->height+100-y)/(cy-y) + x;
		cy = cv->height+100;
	    }
	    subcolmasked = prevcpcolmasked;
	    if( !onlynumber && SPIsPrevCPSelected( sp, cv ))
	    {
		float sz = 2;
		if( SPIsPrevCPSelectedSingle( sp, cv ))
		    sz *= 1.5;
		DrawPoint_SetupRectForSize( &r, cx, cy, sz );
		GDrawFillRect(pixmap,&r, prevcpcol);
		subcolmasked = selectedcpcolmasked;
	    }
	    if ( !onlynumber ) {
		float sizedelta = 3;
		if( prefs_cvEditHandleSize > prefs_cvEditHandleSize_default )
		    sizedelta *= prefs_cvEditHandleSize / prefs_cvEditHandleSize_default;
		GDrawDrawLine(pixmap,x,y,cx,cy, prevcpcolmasked);
		GDrawDrawLine(pixmap,cx-sizedelta,cy-sizedelta,cx+sizedelta,cy+sizedelta,subcolmasked);
		GDrawDrawLine(pixmap,cx+sizedelta,cy-sizedelta,cx-sizedelta,cy+sizedelta,subcolmasked);
	    }
	}
    }

    if ( x<-4 || y<-4 || x>cv->width+4 || y>=cv->height+4 )
return;
    r.x = x-2;
    r.y = y-2;
    r.width = r.height = 0;
    r.width  += prefs_cvEditHandleSize;
    r.height += prefs_cvEditHandleSize;
    if ( sp->selected )
	GDrawSetLineWidth(pixmap,selectedpointwidth);
    isfake = false;
    if ( cv->b.layerheads[cv->b.drawmode]->order2 &&
	    cv->b.layerheads[cv->b.drawmode]->refs==NULL )
    {
	int mightbe_fake = SPInterpolate(sp);
        if ( !mightbe_fake && sp->ttfindex==0xffff )
	    sp->ttfindex = 0xfffe;	/* if we have no instructions we won't call instrcheck and won't notice when a point stops being fake */
	else if ( mightbe_fake )
	    sp->ttfindex = 0xffff;
	isfake = sp->ttfindex==0xffff;
    }
    if ( onlynumber )
    {
	/* Draw Nothing */;
    }
    else if ( sp->pointtype==pt_curve )
    {
	r.width +=2; r.height += 2;
	r.x = x - r.width  / 2;
	r.y = y - r.height / 2;
	if ( sp->selected || isfake )
	    GDrawDrawElipse(pixmap,&r,col);
	else
	    GDrawFillElipse(pixmap,&r,col);
    }
    else if ( sp->pointtype==pt_corner )
    {
	r.x = x - r.width  / 2;
	r.y = y - r.height / 2;
	if ( sp->selected || isfake )
	    GDrawDrawRect(pixmap,&r,col);
	else
	    GDrawFillRect(pixmap,&r,col);
    }
    else if ( sp->pointtype==pt_hvcurve )
    {
	const int gp_sz = 5;
	GPoint gp[5];

	float sizedelta = 3;
	float offsetdelta = 0; // 4 * tab->scale;
	if( prefs_cvEditHandleSize > prefs_cvEditHandleSize_default )
	{
	    sizedelta   *= prefs_cvEditHandleSize / prefs_cvEditHandleSize_default;
	    offsetdelta *= prefs_cvEditHandleSize / prefs_cvEditHandleSize_default;
	}

	float basex = r.x + 3 + offsetdelta;
	float basey = r.y + 3 + offsetdelta;
	gp[0].x = basex - sizedelta; gp[0].y = basey + 0;
	gp[1].x = basex + 0;         gp[1].y = basey + sizedelta;
	gp[2].x = basex + sizedelta; gp[2].y = basey + 0;
	gp[3].x = basex + 0;         gp[3].y = basey - sizedelta;
	gp[4] = gp[0];

	if ( sp->selected || isfake )
	    GDrawDrawPoly(pixmap,gp,gp_sz,col);
	else
	    GDrawFillPoly(pixmap,gp,gp_sz,col);
    }
    else
    {
	BasePoint *cp=NULL;
	BasePoint unit;

	if ( !sp->nonextcp )
	    cp = &sp->nextcp;
	else if ( !sp->noprevcp )
	    cp = &sp->prevcp;
	memset(&unit,0,sizeof(unit));
	if ( cp!=NULL ) {
	    unit.x = cp->x-sp->me.x; unit.y = cp->y-sp->me.y;
	}
	DrawTangentPoint(pixmap, x, y, &unit, sp->selected || isfake, col);
    }
    GDrawSetLineWidth(pixmap,0);
    if ( (cv->showpointnumbers || cv->show_ft_results || cv->dv )
	 && sp->ttfindex!=0xffff )
    {
	if ( sp->ttfindex==0xfffe )
	    strcpy(buf,"??");
        else if (cv->showpointnumbers == 2)
	    SplinePointCoords(buf, sizeof(buf), sp);
	else
	    sprintf( buf,"%d", sp->ttfindex );
	GDrawDrawText8(pixmap,x,y-6,buf,-1,col);
    }
    if ( truetype_markup && sp->roundx ) {
	r.x = x-5; r.y = y-5;
	r.width = r.height = 11;
	GDrawDrawElipse(pixmap,&r,selectedpointcolmasked);
    } else if ( !onlynumber && !truetype_markup ) {
	if ((( sp->roundx || sp->roundy ) &&
		 (((cv->showrounds&1) && tab->scale>=.3) || (cv->showrounds&2))) ||
		(sp->watched && cv->dv!=NULL) ||
		sp->hintmask!=NULL ) {
	    r.x = x-5; r.y = y-5;
	    r.width = r.height = 11;
	    GDrawDrawElipse(pixmap,&r,col);
	}
	if (( sp->flexx && cv->showhhints ) || (sp->flexy && cv->showvhints)) {
	    r.x = x-5; r.y = y-5;
	    r.width = r.height = 11;
	    GDrawDrawElipse(pixmap,&r,
			    MaybeMaskColorToAlphaChannelOverride( sp->flexx ? hflexhintcol : vflexhintcol,
								  AlphaChannelOverride ));
	}
    }
}

static void DrawSpiroPoint( CharView *cv, GWindow pixmap, spiro_cp *cp,
			    SplineSet *spl, int cp_i, Color AlphaChannelOverride )
{
    CharViewTab* tab = CVGetActiveTab(cv);
    GRect r;
    int x, y;
    Color col = cp==&spl->spiros[0] ? firstpointcol : pointcol;
    char ty = cp->ty&0x7f;
    int selected = SPIRO_SELECTED(cp);
    GPoint gp[5];

    if ( selected )
	 col = selectedpointcol;

    if( !selected )
    {
	col = col & 0x00ffffff;
	col |= prefs_cvInactiveHandleAlpha << 24;
    }

    col = MaybeMaskColorToAlphaChannelOverride( col, AlphaChannelOverride );


    x =  tab->xoff + rint(cp->x*tab->scale);
    y = -tab->yoff + cv->height - rint(cp->y*tab->scale);
    if ( x<-4 || y<-4 || x>cv->width+4 || y>=cv->height+4 )
return;

    DrawPoint_SetupRectForSize( &r, x, y, 2 );
    /* r.x = x-2; */
    /* r.y = y-2; */
    /* r.width = r.height = 5; */
    if ( selected )
	GDrawSetLineWidth(pixmap,selectedpointwidth);

    float sizedelta = 3;
    if( prefs_cvEditHandleSize > prefs_cvEditHandleSize_default )
	sizedelta *= prefs_cvEditHandleSize / prefs_cvEditHandleSize_default;

    if ( ty == SPIRO_RIGHT ) {
	GDrawSetLineWidth(pixmap,2);
	gp[0].x = x-sizedelta; gp[0].y = y-sizedelta;
	gp[1].x = x;           gp[1].y = y-sizedelta;
	gp[2].x = x;           gp[2].y = y+sizedelta;
	gp[3].x = x-sizedelta; gp[3].y = y+sizedelta;
	GDrawDrawPoly(pixmap,gp,4,col);
    } else if ( ty == SPIRO_LEFT ) {
	GDrawSetLineWidth(pixmap,2);
	gp[0].x = x+sizedelta; gp[0].y = y-sizedelta;
	gp[1].x = x;           gp[1].y = y-sizedelta;
	gp[2].x = x;           gp[2].y = y+sizedelta;
	gp[3].x = x+sizedelta; gp[3].y = y+sizedelta;
	GDrawDrawPoly(pixmap,gp,4,col);
    } else if ( ty == SPIRO_G2 ) {
	GPoint gp[5];

	float sizedelta = 3;
	float offsetdelta = 1;
	if( prefs_cvEditHandleSize > prefs_cvEditHandleSize_default )
	{
	    sizedelta   *= prefs_cvEditHandleSize / prefs_cvEditHandleSize_default;
	    offsetdelta *= prefs_cvEditHandleSize / prefs_cvEditHandleSize_default;
	}

	float basex = r.x + 1 + offsetdelta;
	float basey = r.y + 1 + offsetdelta;
	gp[0].x = basex - sizedelta; gp[0].y = basey + 0;
	gp[1].x = basex + 0;         gp[1].y = basey + sizedelta;
	gp[2].x = basex + sizedelta; gp[2].y = basey + 0;
	gp[3].x = basex + 0;         gp[3].y = basey - sizedelta;
	gp[4] = gp[0];
	if ( selected )
	    GDrawDrawPoly(pixmap,gp,5,col);
	else
	    GDrawFillPoly(pixmap,gp,5,col);
    } else if ( ty==SPIRO_CORNER ) {
	if ( selected )
	    GDrawDrawRect(pixmap,&r,col);
	else
	    GDrawFillRect(pixmap,&r,col);
    } else {
	--r.x; --r.y; r.width +=2; r.height += 2;
	if ( selected )
	    GDrawDrawElipse(pixmap,&r,col);
	else
	    GDrawFillElipse(pixmap,&r,col);
    }
    GDrawSetLineWidth(pixmap,0);
}

static void DrawLine(CharView *cv, GWindow pixmap,
	real x1, real y1, real x2, real y2, Color fg) {
    CharViewTab* tab = CVGetActiveTab(cv);
    int ix1 = tab->xoff + rint(x1*tab->scale);
    int iy1 = -tab->yoff + cv->height - rint(y1*tab->scale);
    int ix2 = tab->xoff + rint(x2*tab->scale);
    int iy2 = -tab->yoff + cv->height - rint(y2*tab->scale);
    if ( iy1==iy2 ) {
	if ( iy1<0 || iy1>cv->height )
return;
	if ( ix1<0 ) ix1 = 0;
	if ( ix2>cv->width ) ix2 = cv->width;
    } else if ( ix1==ix2 ) {
	if ( ix1<0 || ix1>cv->width )
return;
	if ( iy1<0 ) iy1 = 0;
	if ( iy2<0 ) iy2 = 0;
	if ( iy1>cv->height ) iy1 = cv->height;
	if ( iy2>cv->height ) iy2 = cv->height;
    }
    GDrawDrawLine(pixmap, ix1,iy1, ix2,iy2, fg );
}

static void DrawDirection(CharView *cv,GWindow pixmap, SplinePoint *sp) {
    BasePoint dir, *other;
    double len;
    CharViewTab* tab = CVGetActiveTab(cv);
    int x,y,xe,ye;
    SplinePoint *test;

    if ( sp->next==NULL )
return;

    x = tab->xoff + rint(sp->me.x*tab->scale);
    y = -tab->yoff + cv->height - rint(sp->me.y*tab->scale);
    if ( x<0 || y<0 || x>cv->width || y>cv->width )
return;

    /* Werner complained when the first point and the second point were at */
    /*  the same location... */ /* Damn. They weren't at the same location */
    /*  the were off by a rounding error. I'm not going to fix for that */
    for ( test=sp; ; ) {
	if ( test->me.x!=sp->me.x || test->me.y!=sp->me.y ) {
	    other = &test->me;
    break;
	} else if ( !test->nonextcp ) {
	    other = &test->nextcp;
    break;
	}
	if ( test->next==NULL )
return;
	test = test->next->to;
	if ( test==sp )
return;
    }

    dir.x = other->x-sp->me.x;
    dir.y = sp->me.y-other->y;		/* screen coordinates are the mirror of user coords */
    len = sqrt(dir.x*dir.x + dir.y*dir.y);
    dir.x /= len; dir.y /= len;

    x += rint(5*dir.y);
    y -= rint(5*dir.x);
    xe = x + rint(7*dir.x);
    ye = y + rint(7*dir.y);
    GDrawDrawLine(pixmap,x,y,xe,ye,firstpointcol);
    GDrawDrawLine(pixmap,xe,ye,xe+rint(2*(dir.y-dir.x)),ye+rint(2*(-dir.y-dir.x)), firstpointcol);
    GDrawDrawLine(pixmap,xe,ye,xe+rint(2*(-dir.y-dir.x)),ye+rint(2*(dir.x-dir.y)), firstpointcol);
}

static void CVMarkInterestingLocations(CharView *cv, GWindow pixmap,
	SplinePointList *spl) {
    Spline *s, *first;
    extended interesting[6];
    CharViewTab* tab = CVGetActiveTab(cv);
    int i, ecnt, cnt;
    GRect r;

    for ( s=spl->first->next, first=NULL; s!=NULL && s!=first; s=s->to->next ) {
	if ( first==NULL ) first = s;
	cnt = ecnt = 0;
	if ( cv->markextrema )
	    ecnt = cnt = Spline2DFindExtrema(s,interesting);

	if ( cv->markpoi ) {
	    cnt += Spline2DFindPointsOfInflection(s,interesting+cnt);
	}
	r.width = r.height = 9;
	for ( i=0; i<cnt; ++i ) if ( interesting[i]>0 && interesting[i]<1.0 ) {
	    Color col = i<ecnt ? extremepointcol : pointofinflectioncol;
	    double x = ((s->splines[0].a*interesting[i]+s->splines[0].b)*interesting[i]+s->splines[0].c)*interesting[i]+s->splines[0].d;
	    double y = ((s->splines[1].a*interesting[i]+s->splines[1].b)*interesting[i]+s->splines[1].c)*interesting[i]+s->splines[1].d;
	    double sx =  tab->xoff + rint(x*tab->scale);
	    double sy = -tab->yoff + cv->height - rint(y*tab->scale);
	    if ( sx<-5 || sy<-5 || sx>10000 || sy>10000 )
	continue;
	    GDrawDrawLine(pixmap,sx-4,sy,sx+4,sy, col);
	    GDrawDrawLine(pixmap,sx,sy-4,sx,sy+4, col);
	    r.x = sx-4; r.y = sy-4;
	    GDrawDrawElipse(pixmap,&r,col);
	}
    }
}

static void CVMarkAlmostHV(CharView *cv, GWindow pixmap,
	SplinePointList *spl) {
    Spline *s, *first;
    double dx, dy;
    int x1,x2,y1,y2;
    CharViewTab* tab = CVGetActiveTab(cv);

    for ( s=spl->first->next, first=NULL; s!=NULL && s!=first; s=s->to->next ) {
	if ( first==NULL ) first = s;

	if ( s->islinear ) {
	    if ( !cv->showalmosthvlines )
    continue;
	    if ( (dx = s->from->me.x - s->to->me.x)<0 ) dx = -dx;
	    if ( (dy = s->from->me.y - s->to->me.y)<0 ) dy = -dy;
	    if ( dx<=cv->hvoffset && dy<=cv->hvoffset )
    continue;
	    if ( dx==0 || dy==0 )
    continue;
	    if ( dx<cv->hvoffset || dy<cv->hvoffset ) {
		x1 =  tab->xoff + rint(s->from->me.x*tab->scale);
		y1 = -tab->yoff + cv->height - rint(s->from->me.y*tab->scale);
		x2 =  tab->xoff + rint(s->to->me.x*tab->scale);
		y2 = -tab->yoff + cv->height - rint(s->to->me.y*tab->scale);
		GDrawDrawLine(pixmap,x1,y1,x2,y2,almosthvcol);
	    }
	} else {
	    if ( !cv->showalmosthvcurves )
    continue;
	    if ( (dx = s->from->me.x - s->from->nextcp.x)<0 ) dx = -dx;
	    if ( (dy = s->from->me.y - s->from->nextcp.y)<0 ) dy = -dy;
	    if ( dx<=cv->hvoffset && dy<=cv->hvoffset )
		/* Ignore */;
	    else if ( dx==0 || dy==0 )
		/* It's right */;
	    else if ( dx<cv->hvoffset || dy<cv->hvoffset ) {
		x2 = x1 =  tab->xoff + rint(s->from->me.x*tab->scale);
		y2 = y1 = -tab->yoff + cv->height - rint(s->from->me.y*tab->scale);
		if ( dx<cv->hvoffset ) {
		    if ( s->from->me.y<s->from->nextcp.y )
			y2 += 15;
		    else
			y2 -= 15;
		} else {
		    if ( s->from->me.x<s->from->nextcp.x )
			x2 += 15;
		    else
			x2 -= 15;
		}
		GDrawDrawLine(pixmap,x1,y1,x2,y2,almosthvcol);
	    }

	    if ( (dx = s->to->me.x - s->to->prevcp.x)<0 ) dx = -dx;
	    if ( (dy = s->to->me.y - s->to->prevcp.y)<0 ) dy = -dy;
	    if ( dx<=cv->hvoffset && dy<=cv->hvoffset )
		/* Ignore */;
	    else if ( dx==0 || dy==0 )
		/* It's right */;
	    else if ( dx<cv->hvoffset || dy<cv->hvoffset ) {
		x2 = x1 =  tab->xoff + rint(s->to->me.x*tab->scale);
		y2 = y1 = -tab->yoff + cv->height - rint(s->to->me.y*tab->scale);
		if ( dx<cv->hvoffset ) {
		    if ( s->to->me.y<s->to->prevcp.y )
			y2 += 15;
		    else
			y2 -= 15;
		} else {
		    if ( s->to->me.x<s->to->prevcp.x )
			x2 += 15;
		    else
			x2 -= 15;
		}
		GDrawDrawLine(pixmap,x1,y1,x2,y2,almosthvcol);
	    }
	}
    }
}

static void CVDrawPointName(CharView *cv, GWindow pixmap, SplinePoint *sp, Color fg)
{
    CharViewTab* tab = CVGetActiveTab(cv);
    if (sp->name && *sp->name) {
	int32_t theight;

	GDrawSetFont(pixmap, cv->normal);
	theight = GDrawGetText8Height(pixmap, sp->name, -1);
	GDrawDrawText8(pixmap,
		       tab->xoff + rint(sp->me.x*tab->scale),
		       cv->height-tab->yoff - rint(sp->me.y*tab->scale) + theight + 3,
		       sp->name,-1,fg);
	GDrawSetFont(pixmap,cv->small);	/* For point numbers */
    }
}

static void CVDrawContourName(CharView *cv, GWindow pixmap, SplinePointList *ss,
	Color fg ) {
    SplinePoint *sp, *topright;
    GPoint tr;
    CharViewTab* tab = CVGetActiveTab(cv);

    /* Find the top right point of the contour. This is where we will put the */
    /*  label */
    for ( sp = topright = ss->first; ; ) {
	if ( sp->me.y>topright->me.y || (sp->me.y==topright->me.y && sp->me.x>topright->me.x) )
	    topright = sp;
	if ( sp->next==NULL )
    break;
	sp = sp->next->to;
	if ( sp==ss->first )
    break;
    }
    tr.x = tab->xoff + rint(topright->me.x*tab->scale);
    tr.y = cv->height-tab->yoff - rint(topright->me.y*tab->scale);

    /* If the top edge of the contour is off the bottom of the screen */
    /*  then the contour won't show */
    if ( tr.y>cv->height )
return;

    GDrawSetFont(pixmap,cv->normal);

    if ( ss->first->prev==NULL && ss->first->next!=NULL &&
	    ss->first->next->to->next==NULL && ss->first->next->knownlinear &&
	    (tr.y < cv->nfh || tr.x>cv->width-cv->nfh) ) {
	/* It's a simple line */
	/* A special case because: It's common, and it's important to get it */
	/*  right and label lines even if the point to which we'd normally */
	/*  attach a label is offscreen */
	SplinePoint *sp1 = ss->first, *sp2 = ss->first->next->to;
	double dx, dy, slope, off, yinter, xinter;

	if ( (dx = sp1->me.x-sp2->me.x)<0 ) dx = -dx;
	if ( (dy = sp1->me.y-sp2->me.y)<0 ) dy = -dy;
	if ( dx==0 ) {
	    /* Vertical line */
	    tr.y = cv->nfh;
	    tr.x += 2;
	} else if ( dy==0 ) {
	    /* Horizontal line */
	    tr.x = cv->width - cv->nfh - GDrawGetText8Width(pixmap,ss->contour_name,-1);
	} else {
	    /* y = slope*x + off; */
	    slope = (sp1->me.y-sp2->me.y)/(sp1->me.x-sp2->me.x);
	    off = sp1->me.y - slope*sp1->me.x;
	    /* Now translate to screen coords */
	    off = (cv->height-tab->yoff)+slope*tab->xoff - tab->scale*off;
	    slope = -slope;
	    xinter = (0-off)/slope;
	    yinter = slope*cv->width + off;
	    if ( xinter>0 && xinter<cv->width ) {
		tr.x = xinter+2;
		tr.y = cv->nfh;
	    } else if ( yinter>0 && yinter<cv->height ) {
		tr.x = cv->width - cv->nfh - GDrawGetText8Width(pixmap,ss->contour_name,-1);
		tr.y = yinter;
	    }
	}
    } else {
	tr.y -= cv->nfh/2;
	tr.x -= GDrawGetText8Width(pixmap,ss->contour_name,-1)/2;
    }

    GDrawDrawText8(pixmap,tr.x,tr.y,ss->contour_name,-1,fg);
    GDrawSetFont(pixmap,cv->small);	/* For point numbers */
}

void CVDrawSplineSet(CharView *cv, GWindow pixmap, SplinePointList *set,
	Color fg, int dopoints, DRect *clip ) {
    CVDrawSplineSetSpecialized( cv, pixmap, set, fg, dopoints, clip,
				sfm_stroke, 0 );
}


void CVDrawSplineSetOutlineOnly(CharView *cv, GWindow pixmap, SplinePointList *set,
				Color fg, int dopoints, DRect *clip, enum outlinesfm_flags strokeFillMode ) {
    SplinePointList *spl;
    int currentSplineCounter = 0;
    int activelayer = CVLayer(&cv->b);
    CharViewTab* tab = CVGetActiveTab(cv);

    if( strokeFillMode == sfm_fill ) {
    	GDrawFillRuleSetWinding(pixmap);
    }

    for ( spl = set; spl!=NULL; spl = spl->next ) {

	Color fc  = spl->is_clip_path ? clippathcol : fg;
	/**
	 * Only make the outline red if this is not a grid layer
	 * and we want to highlight open paths
	 * and the activelayer is sane
	 * and the activelayer contains the given splinepointlist
	 * and the path is open
	 */
	if ( cv->b.drawmode!=dm_grid
	     && DrawOpenPathsWithHighlight
	     && activelayer < cv->b.sc->layer_cnt
	     && activelayer >= 0
	     && SplinePointListContains( cv->b.sc->layers[activelayer].splines, spl )
	     && spl->first
	     && spl->first->prev==NULL )
	{
            if ( GDrawGetLineWidth( pixmap ) <= 1 )
		fc = openpathcol | 0xff000000;
	    else
		fc = openpathcol;
	}

	if ( GDrawHasCairo(pixmap)&gc_buildpath ) {
	    Spline *first, *spline;
	    double x,y, cx1, cy1, cx2, cy2, dx,dy;
	    GDrawPathStartSubNew(pixmap);
	    x = rpt(cv,  tab->xoff + spl->first->me.x*tab->scale);
	    y = rpt(cv, -tab->yoff + cv->height - spl->first->me.y*tab->scale);
	    GDrawPathMoveTo(pixmap,x+.5,y+.5);
	    currentSplineCounter++;
	    for ( spline=spl->first->next, first=NULL; spline!=first && spline!=NULL; spline=spline->to->next ) {
		x = rpt(cv,  tab->xoff + spline->to->me.x*tab->scale);
		y = rpt(cv, -tab->yoff + cv->height - spline->to->me.y*tab->scale);
		if ( spline->knownlinear )
		    GDrawPathLineTo(pixmap,x+.5,y+.5);
		else if ( spline->order2 ) {
		    dx = rint(spline->from->me.x*tab->scale) - spline->from->me.x*tab->scale;
		    dy = rint(spline->from->me.y*tab->scale) - spline->from->me.y*tab->scale;
		    cx1 = spline->from->me.x + spline->splines[0].c/3;
		    cy1 = spline->from->me.y + spline->splines[1].c/3;
		    cx2 = cx1 + (spline->splines[0].b+spline->splines[0].c)/3;
		    cy2 = cy1 + (spline->splines[1].b+spline->splines[1].c)/3;
		    cx1 = tab->xoff + cx1*tab->scale + dx;
		    cy1 = -tab->yoff + cv->height - cy1*tab->scale - dy;
		    dx = rint(spline->to->me.x*tab->scale) - spline->to->me.x*tab->scale;
		    dy = rint(spline->to->me.y*tab->scale) - spline->to->me.y*tab->scale;
		    cx2 = tab->xoff + cx2*tab->scale + dx;
		    cy2 = -tab->yoff + cv->height - cy2*tab->scale - dy;
		    GDrawPathCurveTo(pixmap,cx1+.5,cy1+.5,cx2+.5,cy2+.5,x+.5,y+.5);
		} else {
		    dx = rint(spline->from->me.x*tab->scale) - spline->from->me.x*tab->scale;
		    dy = rint(spline->from->me.y*tab->scale) - spline->from->me.y*tab->scale;
		    cx1 = tab->xoff + spline->from->nextcp.x*tab->scale + dx;
		    cy1 = -tab->yoff + cv->height - spline->from->nextcp.y*tab->scale - dy;
		    dx = rint(spline->to->me.x*tab->scale) - spline->to->me.x*tab->scale;
		    dy = rint(spline->to->me.y*tab->scale) - spline->to->me.y*tab->scale;
		    cx2 = tab->xoff + spline->to->prevcp.x*tab->scale + dx;
		    cy2 = -tab->yoff + cv->height - spline->to->prevcp.y*tab->scale - dy;
		    GDrawPathCurveTo(pixmap,cx1+.5,cy1+.5,cx2+.5,cy2+.5,x+.5,y+.5);
		}
		if ( first==NULL )
		    first = spline;
	    }
	    if ( spline!=NULL )
		GDrawPathClose(pixmap);

	    switch( strokeFillMode ) {
            case sfm_stroke_trans:
                GDrawPathStroke( pixmap, fc );
                break;
            case sfm_stroke:
                GDrawPathStroke( pixmap, fc | 0xff000000 );
                break;
            case sfm_clip:
            case sfm_fill:
            case sfm_nothing:
                break;
	    }
	} else if (strokeFillMode != sfm_clip) {
	    GPointList *gpl = MakePoly(cv,spl), *cur;
	    for ( cur=gpl; cur!=NULL; cur=cur->next )
		GDrawDrawPoly(pixmap,cur->gp,cur->cnt,fc);
	    GPLFree(gpl);
	}
    }

    if (strokeFillMode == sfm_clip && (GDrawHasCairo(pixmap) & gc_buildpath)) {
        // Really only cairo_clip needs to be called
        // But then I'd have to change the GDraw interface, ew...
        GDrawClipPreserve( pixmap );
        GDrawPathStartNew( pixmap );
    } else if (strokeFillMode == sfm_fill) {
	if ( cv->inPreviewMode )
	    GDrawPathFill(pixmap, previewfillcol|0xff000000);
	else
	    GDrawPathFill(pixmap, fillcol);
    }
}



void CVDrawSplineSetSpecialized( CharView *cv, GWindow pixmap, SplinePointList *set,
				 Color fg, int dopoints, DRect *clip,
				 enum outlinesfm_flags strokeFillMode,
				 Color AlphaChannelOverride )
{
    Spline *spline, *first;
    SplinePointList *spl;
    int truetype_markup = set==cv->b.gridfit && cv->dv!=NULL;
    CharViewTab* tab = CVGetActiveTab(cv);

    if ( cv->inactive )
	dopoints = false;

    if( strokeFillMode == sfm_fill ) {
    	CVDrawSplineSetOutlineOnly( cv, pixmap, set,
    				    fg, dopoints, clip, strokeFillMode );
    }

    GDrawSetFont(pixmap,cv->small);		/* For point numbers */
    for ( spl = set; spl!=NULL; spl = spl->next ) {
	if ( spl->contour_name!=NULL )
	    CVDrawContourName(cv,pixmap,spl,fg);
	if ( dopoints>0 || (dopoints==-1 && cv->showpointnumbers) ) {
	    first = NULL;
	    if ( dopoints>0 )
		DrawDirection(cv,pixmap,spl->first);
	    if ( cv->b.sc->inspiro && hasspiro()) {
		if ( dopoints>=0 ) {
		    int i;
		    if ( spl->spiros==NULL ) {
			spl->spiros = SplineSet2SpiroCP(spl,&spl->spiro_cnt);
			spl->spiro_max = spl->spiro_cnt;
		    }
		    for ( i=0; i<spl->spiro_cnt-1; ++i )
			DrawSpiroPoint(cv,pixmap,&spl->spiros[i],spl,i, AlphaChannelOverride );
		}
	    } else {
		for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		    DrawPoint(cv,pixmap,spline->from,spl,dopoints<0,truetype_markup, AlphaChannelOverride );
		    CVDrawPointName(cv,pixmap,spline->from,fg);
		    if ( first==NULL ) first = spline;
		}
		if ( spline==NULL ) {
		    DrawPoint(cv,pixmap,spl->last,spl,dopoints<0,truetype_markup, AlphaChannelOverride );
		    CVDrawPointName(cv,pixmap,spl->last,fg);
		}
	    }
	}
    }

    if( strokeFillMode != sfm_nothing ) {
        /*
         * If we were filling, we have to stroke the outline again to properly show
         * clip path splines which will possibly have a different stroke color
         */
        Color thinfgcolor = fg;
        enum outlinesfm_flags fgstrokeFillMode = sfm_stroke;
        if( strokeFillMode==sfm_stroke_trans )
            fgstrokeFillMode = sfm_stroke_trans;
        if( shouldShowFilledUsingCairo(cv) ) {
            if (cv->inPreviewMode)
                thinfgcolor = (thinfgcolor | 0x01000000) & 0x01ffffff;
            fgstrokeFillMode = sfm_stroke_trans;
        }
        CVDrawSplineSetOutlineOnly( cv, pixmap, set,
                        thinfgcolor, dopoints, clip,
                        fgstrokeFillMode );

        if( prefs_cv_outline_thickness > 1 )
        {
            // we only draw the inner half, so we double the user's expected
            // thickness here.
            int strokeWidth = prefs_cv_outline_thickness * 2 * tab->scale;
            Color strokefg = foreoutthicklinecol;

            if( shouldShowFilledUsingCairo(cv) && cv->inPreviewMode ) {
                strokefg = (strokefg | 0x01000000) & 0x01ffffff;
            }

            int16_t oldwidth = GDrawGetLineWidth( pixmap );
            GDrawSetLineWidth( pixmap, strokeWidth );
            GDrawPushClipOnly( pixmap );

            CVDrawSplineSetOutlineOnly( cv, pixmap, set,
                                        strokefg, dopoints, clip,
                                        sfm_clip );
            CVDrawSplineSetOutlineOnly( cv, pixmap, set,
                                        strokefg, dopoints, clip,
                                        sfm_stroke_trans );

            GDrawPopClip( pixmap, NULL );
            GDrawSetLineWidth( pixmap, oldwidth );
        }
    }

    for ( spl = set; spl!=NULL; spl = spl->next ) {
	if (( cv->markextrema || cv->markpoi ) && dopoints && !cv->b.sc->inspiro )
	    CVMarkInterestingLocations(cv,pixmap,spl);
	if ( (cv->showalmosthvlines || cv->showalmosthvcurves ) && dopoints )
	    CVMarkAlmostHV(cv,pixmap,spl);
    }
}

static void CVDrawLayerSplineSet(CharView *cv, GWindow pixmap, Layer *layer,
	Color fg, int dopoints, DRect *clip, enum outlinesfm_flags strokeFillMode ) {
    CharViewTab* tab = CVGetActiveTab(cv);
    int active = cv->b.layerheads[cv->b.drawmode]==layer;
    int ml = cv->b.sc->parent->multilayer;

    if ( ml && layer->dostroke ) {
	if ( layer->stroke_pen.brush.col!=COLOR_INHERITED &&
		layer->stroke_pen.brush.col!=view_bgcol )
	    fg = layer->stroke_pen.brush.col;
    }
    if ( ml && layer->dofill ) {
	if ( layer->fill_brush.col!=COLOR_INHERITED &&
		layer->fill_brush.col!=view_bgcol )
	    fg = layer->fill_brush.col;
    }

    if ( ml && !active && layer!=&cv->b.sc->layers[ly_back] )
	GDrawSetDashedLine(pixmap,5,5,tab->xoff+cv->height-tab->yoff);
    
    CVDrawSplineSetSpecialized( cv, pixmap, layer->splines,
				fg, dopoints && active, clip,
				strokeFillMode, 0 );
    
    if ( ml && !active && layer!=&cv->b.sc->layers[ly_back] )
	GDrawSetDashedLine(pixmap,0,0,0);
}

static void CVDrawTemplates(CharView *cv,GWindow pixmap,SplineChar *template,DRect *clip) {
    RefChar *r;

    CVDrawSplineSet(cv,pixmap,template->layers[ly_fore].splines,templateoutlinecol,false,clip);
    for ( r=template->layers[ly_fore].refs; r!=NULL; r=r->next )
	CVDrawSplineSet(cv,pixmap,r->layers[0].splines,templateoutlinecol,false,clip);
}

static void CVShowDHintInstance(CharView *cv, GWindow pixmap, BasePoint *bp) {
    IPoint ip[40], ip2[40];
    GPoint clipped[13];
    int i,j, tot,last;
    CharViewTab* tab = CVGetActiveTab(cv);

    ip[0].x = tab->xoff + rint( bp[0].x*tab->scale );
    ip[0].y = -tab->yoff + cv->height - rint( bp[0].y*tab->scale );
    ip[1].x = tab->xoff + rint(bp[1].x*tab->scale);
    ip[1].y = -tab->yoff + cv->height - rint( bp[1].y*tab->scale );
    ip[2].x = tab->xoff + rint( bp[2].x*tab->scale );
    ip[2].y = -tab->yoff + cv->height - rint( bp[2].y*tab->scale );
    ip[3].x = tab->xoff + rint( bp[3].x*tab->scale );
    ip[3].y = -tab->yoff + cv->height - rint( bp[3].y*tab->scale );

    if (( ip[0].x<0 && ip[1].x<0 && ip[2].x<0 && ip[3].x<0 ) ||
	    ( ip[0].x>=cv->width && ip[1].x>=cv->width && ip[2].x>=cv->width && ip[3].x>=cv->width ) ||
	    ( ip[0].y<0 && ip[1].y<0 && ip[2].y<0 && ip[3].y<0 ) ||
	    ( ip[0].y>=cv->height && ip[1].y>=cv->height && ip[2].y>=cv->height && ip[3].y>=cv->height ))
return;		/* Offscreen */

    /* clip to left edge */
    tot = 4;
    for ( i=j=0; i<tot; ++i ) {
	last = i==0?tot-1:i-1;
	if ( ip[i].x>=0 && ip[last].x>=0) {
	    ip2[j++] = ip[i];
	} else if ( ip[i].x<0 && ip[last].x<0 ) {
	    if ( j==0 || ip2[j-1].x!=0 || ip2[j-1].y!=ip[i].y ) {
		ip2[j].x = 0;
		ip2[j++].y = ip[i].y;
	    }
	} else {
	    ip2[j].x = 0;
	    ip2[j++].y = ip[last].y - ip[last].x * ((real) (ip[i].y-ip[last].y))/(ip[i].x-ip[last].x);
	    if ( ip[i].x>0 )
		ip2[j++] = ip[i];
	    else {
		ip2[j].x = 0;
		ip2[j++].y = ip[i].y;
	    }
	}
    }
    /* clip to right edge */
    tot = j;
    for ( i=j=0; i<tot; ++i ) {
	last = i==0?tot-1:i-1;
	if ( ip2[i].x<cv->width && ip2[last].x<cv->width ) {
	    ip[j++] = ip2[i];
	} else if ( ip2[i].x>=cv->width && ip2[last].x>=cv->width ) {
	    if ( j==0 || ip[j-1].x!=cv->width-1 || ip[j-1].y!=ip2[i].y ) {
		ip[j].x = cv->width-1;
		ip[j++].y = ip2[i].y;
	    }
	} else {
	    ip[j].x = cv->width-1;
	    ip[j++].y = ip2[last].y + (cv->width-1- ip2[last].x) * ((real) (ip2[i].y-ip2[last].y))/(ip2[i].x-ip2[last].x);
	    if ( ip2[i].x<cv->width )
		ip[j++] = ip2[i];
	    else {
		ip[j].x = cv->width-1;
		ip[j++].y = ip2[i].y;
	    }
	}
    }
    /* clip to bottom edge */
    tot = j;
    for ( i=j=0; i<tot; ++i ) {
	last = i==0?tot-1:i-1;
	if ( ip[i].y>=0 && ip[last].y>=0) {
	    ip2[j++] = ip[i];
	} else if ( ip[i].y<0 && ip[last].y<0 ) {
	    ip2[j].y = 0;
	    ip2[j++].x = ip[i].x;
	} else {
	    ip2[j].y = 0;
	    ip2[j++].x = ip[last].x - ip[last].y * ((real) (ip[i].x-ip[last].x))/(ip[i].y-ip[last].y);
	    if ( ip[i].y>0 )
		ip2[j++] = ip[i];
	    else {
		ip2[j].y = 0;
		ip2[j++].x = ip[i].x;
	    }
	}
    }
    /* clip to top edge */
    tot = j;
    for ( i=j=0; i<tot; ++i ) {
	last = i==0?tot-1:i-1;
	if ( ip2[i].y<cv->height && ip2[last].y<cv->height ) {
	    ip[j++] = ip2[i];
	} else if ( ip2[i].y>=cv->height && ip2[last].y>=cv->height ) {
	    ip[j].y = cv->height-1;
	    ip[j++].x = ip2[i].x;
	} else {
	    ip[j].y = cv->height-1;
	    ip[j++].x = ip2[last].x + (cv->height-1- ip2[last].y) * ((real) (ip2[i].x-ip2[last].x))/(ip2[i].y-ip2[last].y);
	    if ( ip2[i].y<cv->height )
		ip[j++] = ip2[i];
	    else {
		ip[j].y = cv->height-1;
		ip[j++].x = ip2[i].x;
	    }
	}
    }

    tot=j;
    clipped[0].x = ip[0].x; clipped[0].y = ip[0].y;
    for ( i=j=1; i<tot; ++i ) {
	if ( ip[i].x!=ip[i-1].x || ip[i].y!=ip[i-1].y ) {
	    clipped[j].x = ip[i].x; clipped[j++].y = ip[i].y;
	}
    }
    clipped[j++] = clipped[0];
    GDrawFillPoly(pixmap,clipped,j,dhintcol);
}

static void CVShowDHint ( CharView *cv, GWindow pixmap, DStemInfo *dstem ) {
    BasePoint bp[4];
    HintInstance *hi;
    double roff;

    roff =  ( dstem->right.x - dstem->left.x ) * dstem->unit.x +
            ( dstem->right.y - dstem->left.y ) * dstem->unit.y;

    for ( hi=dstem->where; hi!=NULL; hi=hi->next ) {
        bp[0].x = dstem->left.x + dstem->unit.x * hi->begin;
        bp[0].y = dstem->left.y + dstem->unit.y * hi->begin;
        bp[1].x = dstem->right.x + dstem->unit.x * ( hi->begin - roff );
        bp[1].y = dstem->right.y + dstem->unit.y * ( hi->begin - roff );
        bp[2].x = dstem->right.x + dstem->unit.x * ( hi->end - roff );
        bp[2].y = dstem->right.y + dstem->unit.y * ( hi->end - roff );
        bp[3].x = dstem->left.x + dstem->unit.x * hi->end;
        bp[3].y = dstem->left.y + dstem->unit.y * hi->end;
        CVShowDHintInstance( cv, pixmap, bp );
    }
}

static void CVShowMinimumDistance(CharView *cv, GWindow pixmap,MinimumDistance *md) {
    CharViewTab* tab = CVGetActiveTab(cv);
    int x1,y1, x2,y2;
    int xa, ya;
    int off = tab->xoff+cv->height-tab->yoff;

    if (( md->x && !cv->showmdx ) || (!md->x && !cv->showmdy))
return;
    if ( md->sp1==NULL && md->sp2==NULL )
return;
    if ( md->sp1!=NULL ) {
	x1 = tab->xoff + rint( md->sp1->me.x*tab->scale );
	y1 = -tab->yoff + cv->height - rint(md->sp1->me.y*tab->scale);
    } else {
	x1 = tab->xoff + rint( cv->b.sc->width*tab->scale );
	y1 = 0x80000000;
    }
    if ( md->sp2!=NULL ) {
	x2 = tab->xoff + rint( md->sp2->me.x*tab->scale );
	y2 = -tab->yoff + cv->height - rint(md->sp2->me.y*tab->scale);
    } else {
	x2 = tab->xoff + rint( cv->b.sc->width*tab->scale );
	y2 = y1-8;
    }
    if ( y1==0x80000000 )
	y1 = y2-8;
    if ( md->x ) {
	ya = (y1+y2)/2;
	GDrawDrawArrow(pixmap, x1,ya, x2,ya, 2, mdhintcol);
	GDrawSetDashedLine(pixmap,5,5,off);
	GDrawDrawLine(pixmap, x1,ya, x1,y1, mdhintcol);
	GDrawDrawLine(pixmap, x2,ya, x2,y2, mdhintcol);
    } else {
	xa = (x1+x2)/2;
	GDrawDrawArrow(pixmap, xa,y1, xa,y2, 2, mdhintcol);
	GDrawSetDashedLine(pixmap,5,5,off);
	GDrawDrawLine(pixmap, xa,y1, x1,y1, mdhintcol);
	GDrawDrawLine(pixmap, xa,y2, x2,y2, mdhintcol);
    }
    GDrawSetDashedLine(pixmap,0,0,0);
}

static void dtos(char *buf,real val) {
    char *pt;

    sprintf( buf,"%.1f", (double) val);
    pt = buf+strlen(buf);
    if ( pt[-1]=='0' && pt[-2]=='.' ) pt[-2] = '\0';
}

static void CVDrawBlues(CharView *cv,GWindow pixmap,char *bluevals,char *others,
	Color col) {
    double blues[24];
    char *pt, *end;
    int i=0, bcnt=0;
    GRect r;
    char buf[20];
    int len,len2;
    CharViewTab* tab = CVGetActiveTab(cv);

    if ( bluevals!=NULL ) {
	for ( pt = bluevals; isspace( *pt ) || *pt=='['; ++pt);
	while ( i<14 && *pt!='\0' && *pt!=']' ) {
	    blues[i] = g_ascii_strtod(pt,&end);
	    if ( pt==end )
	break;
	    ++i;
	    pt = end;
	    while ( isspace( *pt )) ++pt;
	}
	if ( i&1 ) --i;
    }
    if ( others!=NULL ) {
	for ( pt = others; isspace( *pt ) || *pt=='['; ++pt);
	while ( i<24 && *pt!='\0' && *pt!=']' ) {
	    blues[i] = g_ascii_strtod(pt,&end);
	    if ( pt==end )
	break;
	    ++i;
	    pt = end;
	    while ( isspace( *pt )) ++pt;
	}
	if ( i&1 ) --i;
    }
    bcnt = i;
    if ( i==0 )
return;

    r.x = 0; r.width = cv->width;
    for ( i=0; i<bcnt; i += 2 ) {
	int first, other;
	first = -tab->yoff + cv->height - rint(blues[i]*tab->scale);
	other = -tab->yoff + cv->height - rint(blues[i+1]*tab->scale);
	r.y = first;
	if ( ( r.y<0 && other<0 ) || (r.y>cv->height && other>cv->height))
    continue;
	if ( r.y<0 ) r.y = 0;
	else if ( r.y>cv->height ) r.y = cv->height;
	if ( other<0 ) other = 0;
	else if ( other>cv->height ) other = cv->height;
	if ( other<r.y ) {
	    r.height = r.y-other;
	    r.y = other;
	} else
	    r.height = other-r.y;
	if ( r.height==0 ) r.height = 1;	/* show something */
	GDrawSetStippled(pixmap,2, 0,0);
	GDrawFillRect(pixmap,&r,col);
	GDrawSetStippled(pixmap,0, 0,0);

	if ( first>-20 && first<cv->height+20 ) {
	    dtos( buf, blues[i]);
	    len = GDrawGetText8Width(pixmap,buf,-1);
	    GDrawDrawText8(pixmap,cv->width-len-5,first-3,buf,-1,hintlabelcol);
	} else
	    len = 0;
	if ( other>-20 && other<cv->height+20 ) {
	    dtos( buf, blues[i+1]-blues[i]);
	    len2 = GDrawGetText8Width(pixmap,buf,-1);
	    GDrawDrawText8(pixmap,cv->width-len-5-len2-5,other+cv->sas-3,buf,-1,hintlabelcol);
	}
    }
}

static void CVShowHints(CharView *cv, GWindow pixmap) {
    StemInfo *hint;
    GRect r;
    HintInstance *hi;
    int end;
    Color col;
    DStemInfo *dstem;
    MinimumDistance *md;
    char *blues, *others;
    struct psdict *private = cv->b.sc->parent->private;
    char buf[20];
    int len, len2;
    SplinePoint *sp;
    SplineSet *spl;
    CharViewTab* tab = CVGetActiveTab(cv);

    GDrawSetFont(pixmap,cv->small);
    blues = PSDictHasEntry(private,"BlueValues"); others = PSDictHasEntry(private,"OtherBlues");
    if ( cv->showblues && (blues!=NULL || others!=NULL))
	CVDrawBlues(cv,pixmap,blues,others,bluevalstipplecol);
    blues = PSDictHasEntry(private,"FamilyBlues"); others = PSDictHasEntry(private,"FamilyOtherBlues");
    if ( cv->showfamilyblues && (blues!=NULL || others!=NULL))
	CVDrawBlues(cv,pixmap,blues,others,fambluestipplecol);

    if ( cv->showdhints ) for ( dstem = cv->b.sc->dstem; dstem!=NULL; dstem = dstem->next ) {
	CVShowDHint(cv,pixmap,dstem);
    }

    if ( cv->showhhints && cv->b.sc->hstem!=NULL ) {
	GDrawSetDashedLine(pixmap,5,5,tab->xoff);
	for ( hint = cv->b.sc->hstem; hint!=NULL; hint = hint->next ) {
	    if ( hint->width<0 ) {
		r.y = -tab->yoff + cv->height - rint(hint->start*tab->scale);
		r.height = rint(-hint->width*tab->scale)+1;
	    } else {
		r.y = -tab->yoff + cv->height - rint((hint->start+hint->width)*tab->scale);
		r.height = rint(hint->width*tab->scale)+1;
	    }
	    col = hint->active ? hhintactivecol : hhintcol;
	    /* XRectangles are shorts! */
	    if ( r.y<32767 && r.y+r.height>-32768 ) {
		if ( r.y<-32768 ) {
		    r.height -= (-32768-r.y);
		    r.y = -32768;
		}
		if ( r.y+r.height>32767 )
		    r.height = 32767-r.y;
		for ( hi=hint->where; hi!=NULL; hi=hi->next ) {
		    r.x = tab->xoff + rint(hi->begin*tab->scale);
		    end = tab->xoff + rint(hi->end*tab->scale);
		    if ( end>=0 && r.x<=cv->width ) {
			r.width = end-r.x+1;
			GDrawFillRect(pixmap,&r,col);
		    }
		}
	    }
	    col = (!hint->active && hint->hasconflicts) ? conflicthintcol : col;
	    if ( r.y>=0 && r.y<=cv->height )
		GDrawDrawLine(pixmap,0,r.y,cv->width,r.y,col);
	    if ( r.y+r.height>=0 && r.y+r.height<=cv->width )
		GDrawDrawLine(pixmap,0,r.y+r.height-1,cv->width,r.y+r.height-1,col);

	    r.y = -tab->yoff + cv->height - rint(hint->start*tab->scale);
	    r.y += ( hint->width>0 ) ? -3 : cv->sas+3;
	    if ( r.y>-20 && r.y<cv->height+20 ) {
		dtos( buf, hint->start);
		len = GDrawGetText8Width(pixmap,buf,-1);
		GDrawDrawText8(pixmap,cv->width-len-5,r.y,buf,-1,hintlabelcol);
	    } else
		len = 0;
	    r.y = -tab->yoff + cv->height - rint((hint->start+hint->width)*tab->scale);
	    r.y += ( hint->width>0 ) ? cv->sas+3 : -3;
	    if ( r.y>-20 && r.y<cv->height+20 ) {
		if ( hint->ghost ) {
		    buf[0] = 'G';
		    buf[1] = ' ';
		    dtos(buf+2, hint->width);
		} else
		    dtos( buf, hint->width);
		len2 = GDrawGetText8Width(pixmap,buf,-1);
		GDrawDrawText8(pixmap,cv->width-len-5-len2-5,r.y,buf,-1,hintlabelcol);
	    }
	}
    }
    if ( cv->showvhints && cv->b.sc->vstem!=NULL ) {
	GDrawSetDashedLine(pixmap,5,5,cv->height-tab->yoff);
	for ( hint = cv->b.sc->vstem; hint!=NULL; hint = hint->next ) {
	    if ( hint->width<0 ) {
		r.x = tab->xoff + rint( (hint->start+hint->width)*tab->scale );
		r.width = rint(-hint->width*tab->scale)+1;
	    } else {
		r.x = tab->xoff + rint(hint->start*tab->scale);
		r.width = rint(hint->width*tab->scale)+1;
	    }
	    col = hint->active ? vhintactivecol : vhintcol;
	    if ( r.x<32767 && r.x+r.width>-32768 ) {
		if ( r.x<-32768 ) {
		    r.width -= (-32768-r.x);
		    r.x = -32768;
		}
		if ( r.x+r.width>32767 )
		    r.width = 32767-r.x;
		for ( hi=hint->where; hi!=NULL; hi=hi->next ) {
		    r.y = -tab->yoff + cv->height - rint(hi->end*tab->scale);
		    end = -tab->yoff + cv->height - rint(hi->begin*tab->scale);
		    if ( end>=0 && r.y<=cv->height ) {
			r.height = end-r.y+1;
			GDrawFillRect(pixmap,&r,col);
		    }
		}
	    }
	    col = (!hint->active && hint->hasconflicts) ? conflicthintcol : col;
	    if ( r.x>=0 && r.x<=cv->width )
		GDrawDrawLine(pixmap,r.x,0,r.x,cv->height,col);
	    if ( r.x+r.width>=0 && r.x+r.width<=cv->width )
		GDrawDrawLine(pixmap,r.x+r.width-1,0,r.x+r.width-1,cv->height,col);

	    r.x = tab->xoff + rint(hint->start*tab->scale);
	    if ( r.x>-60 && r.x<cv->width+20 ) {
		dtos( buf, hint->start);
		len = GDrawGetText8Width(pixmap,buf,-1);
		r.x += ( hint->width>0 ) ? 3 : -len-3;
		GDrawDrawText8(pixmap,r.x,cv->sas+3,buf,-1,hintlabelcol);
	    }
	    r.x = tab->xoff + rint((hint->start+hint->width)*tab->scale);
	    if ( r.x>-60 && r.x<cv->width+20 ) {
		if ( hint->ghost ) {
		    buf[0] = 'G';
		    buf[1] = ' ';
		    dtos(buf+2, hint->width);
		} else
		    dtos( buf, hint->width);
		len = GDrawGetText8Width(pixmap,buf,-1);
		r.x += ( hint->width>0 ) ? -len-3 : 3;
		GDrawDrawText8(pixmap,r.x,cv->sas+cv->sfh+3,buf,-1,hintlabelcol);
	    }
	}
    }
    GDrawSetDashedLine(pixmap,0,0,0);

    for ( md=cv->b.sc->md; md!=NULL; md=md->next )
	CVShowMinimumDistance(cv, pixmap,md);

    if ( cv->showvhints || cv->showhhints ) {
	for ( spl=cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl=spl->next ) {
	    if ( spl->first->prev!=NULL ) for ( sp=spl->first ; ; ) {
		if ( cv->showhhints && sp->flexx ) {
		    double x,y,end;
		    x = tab->xoff + rint(sp->me.x*tab->scale);
		    y = -tab->yoff + cv->height - rint(sp->me.y*tab->scale);
		    end = tab->xoff + rint(sp->next->to->me.x*tab->scale);
		    if ( x>-4096 && x<32767 && y>-4096 && y<32767 ) {
			GDrawDrawLine(pixmap,x,y,end,y,hflexhintcol);
		    }
		}
		if ( cv->showvhints && sp->flexy ) {
		    double x,y,end;
		    x = tab->xoff + rint(sp->me.x*tab->scale);
		    y = -tab->yoff + cv->height - rint(sp->me.y*tab->scale);
		    end = -tab->yoff + cv->height - rint(sp->next->to->me.y*tab->scale);
		    if ( x>-4096 && x<32767 && y>-4096 && y<32767 ) {
			GDrawDrawLine(pixmap,x,y,x,end,vflexhintcol);
		    }
		}
		if ( sp->next==NULL )		/* This can happen if we get an internal error inside of RemoveOverlap when the pointlist is not in good shape */
	    break;
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	}
    }
}

static void CVDrawRefName(CharView *cv,GWindow pixmap,RefChar *ref,int fg) {
    CharViewTab* tab = CVGetActiveTab(cv);
    int x,y, len;
    GRect size;

    x = tab->xoff + rint(ref->top.x*tab->scale);
    y = -tab->yoff + cv->height - rint(ref->top.y*tab->scale);
    y -= 5;
    if ( x<-400 || y<-40 || x>cv->width+400 || y>cv->height )
return;

    GDrawLayoutInit(pixmap,ref->sc->name,-1,cv->small);
    GDrawLayoutExtents(pixmap,&size);
    GDrawLayoutDraw(pixmap,x-size.width/2,y,fg);
    len = size.width;
    if ( ref->use_my_metrics )
	GDrawDrawImage(pixmap,&GIcon_lock,NULL,x+len+3,y-cv->sas);
}

void DrawAnchorPoint(GWindow pixmap,int x, int y,int selected) {
    GPoint gp[9];
    Color col = anchorcol;

    gp[0].x = x-1; gp[0].y = y-1;
    gp[1].x = x;   gp[1].y = y-6;
    gp[2].x = x+1; gp[2].y = y-1;
    gp[3].x = x+6; gp[3].y = y;
    gp[4].x = x+1; gp[4].y = y+1;
    gp[5].x = x;   gp[5].y = y+6;
    gp[6].x = x-1; gp[6].y = y+1;
    gp[7].x = x-6; gp[7].y = y;
    gp[8] = gp[0];
    if ( selected )
	GDrawDrawPoly(pixmap,gp,9,col);
    else
	GDrawFillPoly(pixmap,gp,9,col);
}

static void CVDrawAnchorPoints(CharView *cv,GWindow pixmap) {
    CharViewTab* tab = CVGetActiveTab(cv);
    int x,y, len, sel;
    Color col = anchorcol;
    AnchorPoint *ap;
    char *name, ubuf[50];
    GRect r;

    if ( cv->b.drawmode!=dm_fore || cv->b.sc->anchor==NULL || !cv->showanchor )
return;
    GDrawSetFont(pixmap,cv->normal);

    for ( sel=0; sel<2; ++sel ) {
	for ( ap = cv->b.sc->anchor; ap!=NULL; ap=ap->next ) if ( ap->selected==sel ) {
	    x = tab->xoff + rint(ap->me.x*tab->scale);
	    y = -tab->yoff + cv->height - rint(ap->me.y*tab->scale);
	    if ( x<-400 || y<-40 || x>cv->width+400 || y>cv->height )
	continue;

	    DrawAnchorPoint(pixmap,x,y,ap->selected);
	    ubuf[30]=0;
	    if ( ap->anchor->type==act_mkmk ) {
		cc_strncpy(ubuf,ap->anchor->name,30);
		strcat(ubuf," ");
		strcat(ubuf,ap->type==at_basemark ? _("Base") : _("Mark") );
		name = ubuf;
	    } else if ( ap->type==at_basechar || ap->type==at_mark || ap->type==at_basemark ) {
		name = ap->anchor->name;
	    } else if ( ap->type==at_centry || ap->type==at_cexit ) {
		cc_strncpy(ubuf,ap->anchor->name,30);
		strcat(ubuf,ap->type==at_centry ? _("Entry") : _("Exit") );
		name = ubuf;
	    } else if ( ap->type==at_baselig ) {
		cc_strncpy(ubuf,ap->anchor->name,30);
		sprintf(ubuf+strlen(ubuf),"#%d", ap->lig_index);
		name = ubuf;
	    } else
		name = NULL;		/* Should never happen */

	    GRect size;
	    GDrawLayoutInit(pixmap,name,-1,NULL);
	    GDrawLayoutExtents(pixmap,&size);
	    len = size.width;

	    r.x = x-len/2; r.width = len;
	    r.y = y+7; r.height = cv->nfh;
	    GDrawFillRect(pixmap,&r,view_bgcol );
	    GDrawLayoutDraw(pixmap,x-len/2,y+7+cv->nas,col);
	}
    }
}

static void DrawImageList(CharView *cv,GWindow pixmap,ImageList *backimages) {
    CharViewTab* tab = CVGetActiveTab(cv);

    while ( backimages!=NULL ) {
	struct _GImage *base = backimages->image->list_len==0?
		backimages->image->u.image:backimages->image->u.images[0];

	GDrawDrawImageMagnified(pixmap, backimages->image, NULL,
		(int) (tab->xoff + rint(backimages->xoff * tab->scale)),
		(int) (-tab->yoff + cv->height - rint(backimages->yoff*tab->scale)),
		(int) rint((base->width*backimages->xscale*tab->scale)),
		(int) rint((base->height*backimages->yscale*tab->scale)));
	backimages = backimages->next;
    }
}

static void DrawSelImageList(CharView *cv,GWindow pixmap,ImageList *images) {
    while ( images!=NULL ) {
	if ( images->selected )
	    CVDrawBB(cv,pixmap,&images->bb);
	images = images->next;
    }
}

static void DrawOldState(CharView *cv, GWindow pixmap, Undoes *undo, DRect *clip) {
    RefChar *refs;

    if ( undo==NULL )
return;

    CVDrawSplineSet(cv,pixmap,undo->u.state.splines,oldoutlinecol,false,clip);
    for ( refs=undo->u.state.refs; refs!=NULL; refs=refs->next )
	if ( refs->layers[0].splines!=NULL )
	    CVDrawSplineSet(cv,pixmap,refs->layers[0].splines,oldoutlinecol,false,clip);
    /* Don't do images... */
}

static void DrawTransOrigin(CharView *cv, GWindow pixmap) {
    CharViewTab* tab = CVGetActiveTab(cv);
    int x = rint(cv->p.cx*tab->scale) + tab->xoff, y = cv->height-tab->yoff-rint(cv->p.cy*tab->scale);

    GDrawDrawLine(pixmap,x-4,y,x+4,y,transformorigincol);
    GDrawDrawLine(pixmap,x,y-4,x,y+4,transformorigincol);
}

static void DrawVLine(CharView *cv,GWindow pixmap,real pos,Color fg, int flags,
	GImage *lock, char *name) {
    CharViewTab* tab = CVGetActiveTab(cv);
    char buf[20];
    int x = tab->xoff + rint(pos*tab->scale);
    DrawLine(cv,pixmap,pos,-32768,pos,32767,fg);
    if ( x>-400 && x<cv->width+400 ) {
	if ( flags&1 ) {
	    dtos( buf, pos);
	    GDrawSetFont(pixmap,cv->small);
	    GDrawDrawText8(pixmap,x+5,cv->sas+3,buf,-1,metricslabelcol);
	    if ( lock!=NULL )
		GDrawDrawImage(pixmap,lock,NULL,x+5,3+cv->sfh);
	}
	if ( name!=NULL )
	    GDrawDrawText8(pixmap,x+5,cv->sas+cv->sfh*(1+lock!=NULL)+3,name,-1,metricslabelcol);
    }
    if ( ItalicConstrained && cv->b.sc->parent->italicangle!=0 ) {
	double t = tan(-cv->b.sc->parent->italicangle*FF_PI/180.);
	int xoff = rint(8096*t);
	DrawLine(cv,pixmap,pos-xoff,-8096,pos+xoff,8096,italiccoordcol);
    }
}

static void DrawMMGhosts(CharView *cv,GWindow pixmap,DRect *clip) {
    /* In an MM font, draw any selected alternate versions of the current char */
    MMSet *mm = cv->b.sc->parent->mm;
    int j;
    SplineFont *sub;
    SplineChar *sc;
    RefChar *rf;

    if ( mm==NULL )
return;
    for ( j = 0; j<mm->instance_count+1; ++j ) {
	if ( j==0 )
	    sub = mm->normal;
	else
	    sub = mm->instances[j-1];
	sc = NULL;
	if ( cv->b.sc->parent!=sub && (cv->mmvisible & (1<<j)) &&
		cv->b.sc->orig_pos<sub->glyphcnt )
	    sc = sub->glyphs[cv->b.sc->orig_pos];
	if ( sc!=NULL ) {
	    for ( rf=sc->layers[ly_fore].refs; rf!=NULL; rf = rf->next )
		CVDrawSplineSet(cv,pixmap,rf->layers[0].splines,backoutlinecol,false,clip);
	    CVDrawSplineSet(cv,pixmap,sc->layers[ly_fore].splines,backoutlinecol,false,clip);
	}
    }
}

static void CVDrawGridRaster(CharView *cv, GWindow pixmap, DRect *clip ) {
    CharViewTab* tab = CVGetActiveTab(cv);
    if ( cv->showgrids ) {
	/* Draw ppem grid, and the raster for truetype debugging, grid fit */
	GRect pixel;
	real ygrid_spacing = (cv->b.sc->parent->ascent+cv->b.sc->parent->descent) / (real) cv->ft_ppemy;
	real xgrid_spacing = (cv->b.sc->parent->ascent+cv->b.sc->parent->descent) / (real) cv->ft_ppemx;
	int max,jmax,ii,i,jj,j;
	int minx, maxx, miny, maxy, r,or=0;
	Color clut[256];

	pixel.width = xgrid_spacing*tab->scale+1;
	pixel.height = ygrid_spacing*tab->scale+1;
	if ( cv->raster!=NULL ) {
	    if ( cv->raster->num_greys>2 ) {
		int rb, gb, bb, rd, gd, bd;
		clut[0] = view_bgcol ;
		rb = COLOR_RED(clut[0]); gb = COLOR_GREEN(clut[0]); bb = COLOR_BLUE(clut[0]);
		rd = COLOR_RED(rasterdarkcol)-rb;
		gd = COLOR_GREEN(rasterdarkcol)-gb;
		bd = COLOR_BLUE(rasterdarkcol)-bb;
		for ( i=1; i<256; ++i ) {
		    clut[i] = ( (rb +rd*(i)/0xff)<<16 ) |
			    ( (gb+gd*(i)/0xff)<<8 ) |
			    ( (bb+bd*(i)/0xff) );
		}
	    }
	    minx = cv->raster->lb; maxx = minx+cv->raster->cols;
	    maxy = cv->raster->as; miny = maxy-cv->raster->rows;
	    if ( cv->oldraster!=NULL ) {
		if ( cv->oldraster->lb<minx ) minx = cv->oldraster->lb;
		if ( cv->oldraster->lb+cv->oldraster->cols>maxx ) maxx = cv->oldraster->lb+cv->oldraster->cols;
		if ( cv->oldraster->as>maxy ) maxy = cv->oldraster->as;
		if ( cv->oldraster->as-cv->oldraster->rows<miny ) miny = cv->oldraster->as-cv->oldraster->rows;
	    }
	    for ( ii=maxy; ii>miny; --ii ) {
		for ( jj=minx; jj<maxx; ++jj ) {
		    i = cv->raster->as-ii; j = jj-cv->raster->lb;
		    if ( i<0 || i>=cv->raster->rows || j<0 || j>=cv->raster->cols )
			r = 0;
		    else if ( cv->raster->num_greys<=2 )
			r = cv->raster->bitmap[i*cv->raster->bytes_per_row+(j>>3)] & (1<<(7-(j&7)));
		    else
			r = cv->raster->bitmap[i*cv->raster->bytes_per_row+j];
		    if ( cv->oldraster==NULL || cv->oldraster->num_greys!=cv->raster->num_greys)
			or = r;
		    else {
			i = cv->oldraster->as-ii; j = jj-cv->oldraster->lb;
			if ( i<0 || i>=cv->oldraster->rows || j<0 || j>=cv->oldraster->cols )
			    or = 0;
			else if ( cv->oldraster->num_greys<=2 )
			    or = cv->oldraster->bitmap[i*cv->oldraster->bytes_per_row+(j>>3)] & (1<<(7-(j&7)));
			else
			    or = cv->oldraster->bitmap[i*cv->oldraster->bytes_per_row+j];
		    }
		    if ( r || ( or && cv->showdebugchanges)) {
			pixel.x = jj*xgrid_spacing*tab->scale + tab->xoff+1;
			pixel.y = cv->height-tab->yoff - rint(ii*ygrid_spacing*tab->scale);
			if ( cv->showdebugchanges ) {
			    if ( cv->raster->num_greys<=2 )
				GDrawFillRect(pixmap,&pixel,(r && or) ? rastercol : r ? rasternewcol : rasteroldcol );
			    else
				GDrawFillRect(pixmap,&pixel,(r-or>-16 && r-or<16) ? clut[r] : (clut[r]&0x00ff00) );
			} else {
			    if ( cv->raster->num_greys<=2 )
				GDrawFillRect(pixmap,&pixel, rastercol );
			    else
				GDrawFillRect(pixmap,&pixel, clut[r] );
			}
		    }
		}
	    }
	}

	for ( i = floor( clip->x/xgrid_spacing ), max = ceil((clip->x+clip->width)/xgrid_spacing);
		i<=max; ++i )
	    DrawLine(cv,pixmap,i*xgrid_spacing,-32768,i*xgrid_spacing,32767,i==0?coordcol:rastergridcol);
	for ( i = floor( clip->y/ygrid_spacing ), max = ceil((clip->y+clip->height)/ygrid_spacing);
		i<=max; ++i )
	    DrawLine(cv,pixmap,-32768,i*ygrid_spacing,32767,i*ygrid_spacing,i==0?coordcol:rastergridcol);
	if ( xgrid_spacing*tab->scale>=7 && ygrid_spacing*tab->scale>=7) {
	    for ( i = floor( clip->x/xgrid_spacing ), max = ceil((clip->x+clip->width)/xgrid_spacing);
		    i<=max; ++i )
		for ( j = floor( clip->y/ygrid_spacing ), jmax = ceil((clip->y+clip->height)/ygrid_spacing);
			j<=jmax; ++j ) {
		    int x = (i+.5)*xgrid_spacing*tab->scale + tab->xoff;
		    int y = cv->height-tab->yoff - rint((j+.5)*ygrid_spacing*tab->scale);
		    GDrawDrawLine(pixmap,x-2,y,x+2,y,rastergridcol);
		    GDrawDrawLine(pixmap,x,y-2,x,y+2,rastergridcol);
		}
	}
	if ( cv->qg!=NULL ) {
	    pixel.x = cv->note_x*xgrid_spacing*tab->scale + tab->xoff;
	    pixel.y = cv->height-tab->yoff - rint(cv->note_y*ygrid_spacing*tab->scale)
		- pixel.height;
	    if ( pixel.height>=20 )
		GDrawSetLineWidth(pixmap,3);
	    else if ( pixel.height>10 )
		GDrawSetLineWidth(pixmap,2);
	    GDrawDrawRect(pixmap,&pixel,deltagridcol);
	    GDrawSetLineWidth(pixmap,0);
	    {
		int x = (cv->note_x+.5)*xgrid_spacing*tab->scale + tab->xoff;
		int y = cv->height-tab->yoff - rint((cv->note_y+.5)*ygrid_spacing*tab->scale);
		GDrawDrawLine(pixmap,x-2,y,x+2,y,deltagridcol);
		GDrawDrawLine(pixmap,x,y-2,x,y+2,deltagridcol);
	    }
	}
    }
    if ( cv->showback[0]&1 ) {
	CVDrawSplineSet(cv,pixmap,cv->b.gridfit,gridfitoutlinecol,
		cv->showpoints,clip);
    }
}

static int APinSC(AnchorPoint *ap,SplineChar *sc) {
    /* Anchor points can be deleted ... */
    AnchorPoint *test;

    for ( test=sc->anchor; test!=NULL && test!=ap; test = test->next );
return( test==ap );
}

static void DrawAPMatch(CharView *cv,GWindow pixmap,DRect *clip) {
    SplineChar *sc = cv->b.sc, *apsc = cv->apsc;
    SplineFont *sf = sc->parent;
    real trans[6];
    SplineSet *head, *tail, *temp;
    RefChar *ref;
    int layer = CVLayer((CharViewBase *) cv);

    if ( cv->b.drawmode==dm_grid )
return;

    /* The other glyph might have been removed from the font */
    /* Either anchor might have been deleted. Be prepared for that to happen */
    if ( (apsc->orig_pos>=sf->glyphcnt || apsc->orig_pos<0) ||
	    sf->glyphs[apsc->orig_pos]!=apsc ||
	    !APinSC(cv->apmine,sc) || !APinSC(cv->apmatch,apsc)) {
	cv->apmine = cv->apmatch = NULL;
	cv->apsc =NULL;
return;
    }

    /* Ok this isn't very accurate, but we are going to use the current glyph's*/
    /*  coordinate system (because we're showing the current glyph), we should */
    /*  always use the base character's coordinates, but that would screw up   */
    /*  editing of the current glyph if it happened to be the mark */
    trans[0] = trans[3] = 1;
    trans[1] = trans[2] = 0;
    trans[4] = cv->apmine->me.x - cv->apmatch->me.x;
    trans[5] = cv->apmine->me.y - cv->apmatch->me.y;

    head = tail = SplinePointListCopy(apsc->layers[layer].splines);
    for ( ref = apsc->layers[layer].refs; ref!=NULL; ref = ref->next ) {
	temp = SplinePointListCopy(ref->layers[0].splines);
	if ( head!=NULL ) {
	    for ( ; tail->next!=NULL; tail = tail->next );
	    tail->next = temp;
	} else
	    head = tail = temp;
    }
    head = SplinePointListTransform(head,trans,tpt_AllPoints);
    CVDrawSplineSet(cv,pixmap,head,anchoredoutlinecol,
	    false,clip);
    SplinePointListsFree(head);
    if ( cv->apmine->type==at_mark || cv->apmine->type==at_centry ) {
	DrawVLine(cv,pixmap,trans[4],anchoredoutlinecol,false,NULL,NULL);
	DrawLine(cv,pixmap,-8096,trans[5],8096,trans[5],anchoredoutlinecol);
    }
}

static void DrawPLine(CharView *cv,GWindow pixmap,int x1, int y1, int x2, int y2,Color col) {

    if ( x1==x2 || y1==y2 ) {
	if ( x1<0 ) x1=0;
	else if ( x1>cv->width ) x1 = cv->width;
	if ( x2<0 ) x2=0;
	else if ( x2>cv->width ) x2 = cv->width;
	if ( y1<0 ) y1=0;
	else if ( y1>cv->height ) y1 = cv->height;
	if ( y2<0 ) y2=0;
	else if ( y2>cv->height ) y2 = cv->height;
    } else if ( y1<-1000 || y2<-1000 || x1<-1000 || x2<-1000 ||
	    y1>cv->height+1000 || y2>cv->height+1000 ||
	    x1>cv->width+1000 || x2>cv->width+1000 )
return;
    GDrawDrawLine(pixmap,x1,y1,x2,y2,col);
}

static void FindQuickBounds(SplineSet *ss,BasePoint **bounds) {
    SplinePoint *sp;

    for ( ; ss!=NULL; ss=ss->next ) {
	sp = ss->first;
	if ( sp->next==NULL || sp->next->to==sp )	/* Ignore contours with one point. Often tt points for moving references or anchors */
    continue;
	for (;;) {
	    if ( bounds[0]==NULL )
		bounds[0] = bounds[1] = bounds[2] = bounds[3] = &sp->me;
	    else {
		if ( sp->me.x<bounds[0]->x ) bounds[0] = &sp->me;
		if ( sp->me.x>bounds[1]->x ) bounds[1] = &sp->me;
		if ( sp->me.y<bounds[2]->y ) bounds[2] = &sp->me;
		if ( sp->me.y>bounds[3]->y ) bounds[3] = &sp->me;
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
}

static void SSFindItalicBounds(SplineSet *ss,double t,SplinePoint **left, SplinePoint **right) {
    SplinePoint *sp;

    if ( t==0 )
return;

    for ( ; ss!=NULL; ss=ss->next ) {
	sp = ss->first;
	if ( sp->next==NULL || sp->next->to==sp )	/* Ignore contours with one point. Often tt points for moving references or anchors */
    continue;
	for (;;) {
	    if ( *left==NULL )
		*left = *right = sp;
	    else {
		double xoff = sp->me.y*t;
		if ( sp->me.x-xoff < (*left)->me.x - (*left)->me.y*t ) *left = sp;
		if ( sp->me.x-xoff > (*right)->me.x - (*right)->me.y*t ) *right = sp;
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
}

static void CVSideBearings(GWindow pixmap, CharView *cv) {
    CharViewTab* tab = CVGetActiveTab(cv);
    SplineChar *sc = cv->b.sc;
    RefChar *ref;
    BasePoint *bounds[4];
    int layer,last, first,l;
    int x,y, x2, y2;
    char buf[20];

    memset(bounds,0,sizeof(bounds));
    if ( sc->parent->multilayer ) {
	last = sc->layer_cnt-1;
	first = ly_fore;
    } else {
	first = last = CVLayer( (CharViewBase *) cv);
	if ( first==ly_grid )
	    first = last = ly_fore;
    }
    for ( layer = first ; layer<=last; ++layer ) {
	FindQuickBounds(sc->layers[layer].splines,bounds);
	for ( ref=sc->layers[layer].refs; ref!=NULL; ref=ref->next )
	    for ( l=0; l<ref->layer_cnt; ++l )
		FindQuickBounds(ref->layers[l].splines,bounds);
    }

    if ( bounds[0]==NULL )
return;				/* no points. no side bearings */

    GDrawSetFont(pixmap,cv->small);
    if ( cv->showhmetrics ) {
	if ( bounds[0]->x!=0 ) {
	    x = rint(bounds[0]->x*tab->scale) + tab->xoff;
	    y = cv->height-tab->yoff-rint(bounds[0]->y*tab->scale);
	    DrawPLine(cv,pixmap,tab->xoff,y,x,y,metricslabelcol);
	     /* arrow heads */
	     DrawPLine(cv,pixmap,tab->xoff,y,tab->xoff+4,y+4,metricslabelcol);
	     DrawPLine(cv,pixmap,tab->xoff,y,tab->xoff+4,y-4,metricslabelcol);
	     DrawPLine(cv,pixmap,x,y,x-4,y-4,metricslabelcol);
	     DrawPLine(cv,pixmap,x,y,x-4,y+4,metricslabelcol);
	    dtos( buf, bounds[0]->x);
	    x = tab->xoff + (x-tab->xoff-GDrawGetText8Width(pixmap,buf,-1))/2;
	    GDrawDrawText8(pixmap,x,y-4,buf,-1,metricslabelcol);
	}

	if ( sc->width != bounds[1]->x ) {
	    x = rint(bounds[1]->x*tab->scale) + tab->xoff;
	    y = cv->height-tab->yoff-rint(bounds[1]->y*tab->scale);
	    x2 = rint(sc->width*tab->scale) + tab->xoff;
	    DrawPLine(cv,pixmap,x,y,x2,y,metricslabelcol);
	     /* arrow heads */
	     DrawPLine(cv,pixmap,x,y,x+4,y+4,metricslabelcol);
	     DrawPLine(cv,pixmap,x,y,x+4,y-4,metricslabelcol);
	     DrawPLine(cv,pixmap,x2,y,x2-4,y-4,metricslabelcol);
	     DrawPLine(cv,pixmap,x2,y,x2-4,y+4,metricslabelcol);
	    dtos( buf, sc->width-bounds[1]->x);
	    x = x + (x2-x-GDrawGetText8Width(pixmap,buf,-1))/2;
	    GDrawDrawText8(pixmap,x,y-4,buf,-1,metricslabelcol);
	}
	if ( ItalicConstrained && cv->b.sc->parent->italicangle!=0 ) {
	    double t = tan(-cv->b.sc->parent->italicangle*FF_PI/180.);
	    if ( t!=0 ) {
		SplinePoint *leftmost=NULL, *rightmost=NULL;
		for ( layer=first; layer<=last; ++layer ) {
		    SSFindItalicBounds(sc->layers[layer].splines,t,&leftmost,&rightmost);
		    for ( ref=sc->layers[layer].refs; ref!=NULL; ref=ref->next )
			for ( l=0; l<ref->layer_cnt; ++l )
			    SSFindItalicBounds(ref->layers[l].splines,t,&leftmost,&rightmost);
		}
		if ( leftmost!=NULL ) {
		    x = rint(leftmost->me.y*t*tab->scale) + tab->xoff;
		    x2 = rint(leftmost->me.x*tab->scale) + tab->xoff;
		    y = cv->height-tab->yoff-rint(leftmost->me.y*tab->scale);
		    DrawPLine(cv,pixmap,x,y,x2,y,italiccoordcol);
		     /* arrow heads */
		     DrawPLine(cv,pixmap,x,y,x+4,y+4,italiccoordcol);
		     DrawPLine(cv,pixmap,x,y,x+4,y-4,italiccoordcol);
		     DrawPLine(cv,pixmap,x2,y,x2-4,y-4,italiccoordcol);
		     DrawPLine(cv,pixmap,x2,y,x2-4,y+4,italiccoordcol);
		    dtos( buf, leftmost->me.x-leftmost->me.y*t);
		    x = x + (x2-x-GDrawGetText8Width(pixmap,buf,-1))/2;
		    GDrawDrawText8(pixmap,x,y+12,buf,-1,italiccoordcol);
		}
		if ( rightmost!=NULL ) {
		    x = rint(rightmost->me.x*tab->scale) + tab->xoff;
		    y = cv->height-tab->yoff-rint(rightmost->me.y*tab->scale);
		    x2 = rint((sc->width + rightmost->me.y*t)*tab->scale) + tab->xoff;
		    DrawPLine(cv,pixmap,x,y,x2,y,italiccoordcol);
		     /* arrow heads */
		     DrawPLine(cv,pixmap,x,y,x+4,y+4,italiccoordcol);
		     DrawPLine(cv,pixmap,x,y,x+4,y-4,italiccoordcol);
		     DrawPLine(cv,pixmap,x2,y,x2-4,y-4,italiccoordcol);
		     DrawPLine(cv,pixmap,x2,y,x2-4,y+4,italiccoordcol);
		    dtos( buf, sc->width+rightmost->me.y*t-rightmost->me.x);
		    x = x + (x2-x-GDrawGetText8Width(pixmap,buf,-1))/2;
		    GDrawDrawText8(pixmap,x,y+12,buf,-1,italiccoordcol);
		}
	    }
	}
    }

    if ( cv->showvmetrics ) {
	x = rint(bounds[2]->x*tab->scale) + tab->xoff;
	y = cv->height-tab->yoff-rint(bounds[2]->y*tab->scale);
	y2 = cv->height-tab->yoff-rint(-sc->parent->descent*tab->scale);
	DrawPLine(cv,pixmap,x,y,x,y2,metricslabelcol);
	 /* arrow heads */
	 DrawPLine(cv,pixmap,x,y,x-4,y+4,metricslabelcol);
	 DrawPLine(cv,pixmap,x,y,x+4,y+4,metricslabelcol);
	 DrawPLine(cv,pixmap,x,y2,x+4,y2-4,metricslabelcol);
	 DrawPLine(cv,pixmap,x,y2,x-4,y2-4,metricslabelcol);
	dtos( buf, bounds[2]->y+sc->parent->descent);
	y = y - (y-y2-cv->sfh)/2;
	GDrawDrawText8(pixmap,x+4,y,buf,-1,metricslabelcol);

	x = rint(bounds[3]->x*tab->scale) + tab->xoff;
	y = cv->height-tab->yoff-rint(bounds[3]->y*tab->scale);
	y2 = cv->height-tab->yoff-rint(sc->parent->ascent*tab->scale);
	DrawPLine(cv,pixmap,x,y,x,y2,metricslabelcol);
	 /* arrow heads */
	 DrawPLine(cv,pixmap,x,y,x-4,y-4,metricslabelcol);
	 DrawPLine(cv,pixmap,x,y,x+4,y-4,metricslabelcol);
	 DrawPLine(cv,pixmap,x,y2,x+4,y2+4,metricslabelcol);
	 DrawPLine(cv,pixmap,x,y2,x-4,y2+4,metricslabelcol);
	dtos( buf, sc->parent->ascent-bounds[3]->y);
	x = x + (x2-x-GDrawGetText8Width(pixmap,buf,-1))/2;
	GDrawDrawText8(pixmap,x,y-4,buf,-1,metricslabelcol);
    }
}

static int CVExposeGlyphFill(CharView *cv, GWindow pixmap, GEvent *event, DRect* clip ) {
    CharViewTab* tab = CVGetActiveTab(cv);
    int layer, cvlayer = CVLayer((CharViewBase *) cv);
    int filled = 0;

    if( shouldShowFilledUsingCairo(cv) ) {
	layer = cvlayer;
	if ( layer>=0 ) {
	    CVDrawLayerSplineSet(cv,pixmap,&cv->b.sc->layers[layer],foreoutlinecol,
	    			 cv->showpoints, clip, sfm_fill );
	    filled = 1;
	}
    } else {
	if (( cv->showfore || cv->b.drawmode==dm_fore ) && cv->showfilled && 
	    cv->filled!=NULL ) {
	    GDrawDrawImage(pixmap, &cv->gi, NULL,
			   tab->xoff + cv->filled->xmin,
			   -tab->yoff + cv->height-cv->filled->ymax);
	    filled = 1;
	}
    }
    return(filled);
}

static void CVExposeReferences( CharView *cv, GWindow pixmap, SplineChar* sc, int layer, DRect* clip )
{
    RefChar *rf = 0;
    int rlayer = 0;
    
    for ( rf = sc->layers[layer].refs; rf!=NULL; rf = rf->next )
    {
	if ( cv->showrefnames )
	    CVDrawRefName(cv,pixmap,rf,0);
	enum outlinesfm_flags refsfm = sfm_stroke;
	if( shouldShowFilledUsingCairo(cv) ) {
	    refsfm = sfm_fill;
	}

	for ( rlayer=0; rlayer<rf->layer_cnt; ++rlayer )
	    CVDrawSplineSetSpecialized(cv,pixmap,rf->layers[rlayer].splines,foreoutlinecol,-1,clip, refsfm, 0);
	if ( rf->selected && cv->b.layerheads[cv->b.drawmode]==&sc->layers[layer])
	    CVDrawBB(cv,pixmap,&rf->bb);
    }
}


static void CVExpose(CharView *cv, GWindow pixmap, GEvent *event ) {
    CharViewTab* tab = CVGetActiveTab(cv);
    SplineFont *sf = cv->b.sc->parent;
    RefChar *rf;
    GRect old;
    DRect clip;
    char buf[20];
    PST *pst;
    int i, layer, rlayer, cvlayer = CVLayer((CharViewBase *) cv);
    enum outlinesfm_flags strokeFillMode = sfm_stroke;
    int GlyphHasBeenFilled = 0;

    GDrawPushClip(pixmap,&event->u.expose.rect,&old);

    if( shouldShowFilledUsingCairo(cv) ) {
    	strokeFillMode = sfm_fill;
    }

    clip.width = event->u.expose.rect.width/tab->scale;
    clip.height = event->u.expose.rect.height/tab->scale;
    clip.x = (event->u.expose.rect.x-tab->xoff)/tab->scale;
    clip.y = (cv->height-event->u.expose.rect.y-event->u.expose.rect.height-tab->yoff)/tab->scale;

    GDrawSetFont(pixmap,cv->small);
    GDrawSetLineWidth(pixmap,0);

    if ( !cv->show_ft_results && cv->dv==NULL ) {

	if ( cv->backimgs==NULL && !(GDrawHasCairo(cv->v)&gc_buildpath))
	    cv->backimgs = GDrawCreatePixmap(GDrawGetDisplayOfWindow(cv->v),cv->v,cv->width,cv->height);
	if ( GDrawHasCairo(cv->v)&gc_buildpath ) {
	    for ( layer = ly_back; layer<cv->b.sc->layer_cnt; ++layer ) if ( cv->b.sc->layers[layer].images!=NULL ) {
		if (( sf->multilayer && ((( cv->showback[0]&1 || cvlayer==layer) && layer==ly_back ) ||
			    ((cv->showfore || cvlayer==layer) && layer>ly_back)) ) ||
		    ( !sf->multilayer && (((cv->showfore && cvlayer==layer) && layer==ly_fore) ||
			    (((cv->showback[layer>>5]&(1<<(layer&31))) || cvlayer==layer) && layer!=ly_fore))) ) {
		    /* This really should be after the grids, but then it would completely*/
		    /*  hide them. */
		    DrawImageList(cv,pixmap,cv->b.sc->layers[layer].images);
		}
	    }
	    cv->back_img_out_of_date = false;
	    if ( cv->showhhints || cv->showvhints || cv->showdhints || cv->showblues || cv->showfamilyblues)
		CVShowHints(cv,pixmap);
	} else if ( cv->back_img_out_of_date ) {
	    GDrawFillRect(cv->backimgs,NULL,view_bgcol);
	    if ( cv->showhhints || cv->showvhints || cv->showdhints || cv->showblues || cv->showfamilyblues)
		CVShowHints(cv,cv->backimgs);
	    for ( layer = ly_back; layer<cv->b.sc->layer_cnt; ++layer ) if ( cv->b.sc->layers[layer].images!=NULL ) {
		if (( sf->multilayer && ((( cv->showback[0]&1 || cvlayer==layer) && layer==ly_back ) ||
			    ((cv->showfore || cvlayer==layer) && layer>ly_back)) ) ||
		    ( !sf->multilayer && (((cv->showfore && cvlayer==layer) && layer==ly_fore) ||
			    (((cv->showback[layer>>5]&(1<<(layer&31))) || cvlayer==layer) && layer!=ly_fore))) ) {
		    /* This really should be after the grids, but then it would completely*/
		    /*  hide them. */
		    if ( cv->back_img_out_of_date )
			DrawImageList(cv,cv->backimgs,cv->b.sc->layers[layer].images);
		}
	    }
	    cv->back_img_out_of_date = false;
	}
	if ( cv->backimgs!=NULL ) {
	    GRect r;
	    r.x = r.y = 0; r.width = cv->width; r.height = cv->height;
	    GDrawDrawPixmap(pixmap,cv->backimgs,&r,0,0);
	} else if ( !(GDrawHasCairo(cv->v)&gc_buildpath) &&
		( cv->showhhints || cv->showvhints || cv->showdhints || cv->showblues || cv->showfamilyblues)) {
	    /* if we've got bg images (and we're showing them) then the hints live in */
	    /*  the bg image pixmap (else they get overwritten by the pixmap) */
	    CVShowHints(cv,pixmap);
	}
	if ( cv->showgrids || cv->b.drawmode==dm_grid ) {
	    CVDrawSplineSet(cv,pixmap,cv->b.fv->sf->grid.splines,guideoutlinecol,
		    cv->showpoints && cv->b.drawmode==dm_grid,&clip);
	}
	if ( cv->showhmetrics ) {
	    Color lbcolor = (!cv->inactive && cv->lbearingsel) ? lbearingselcol : coordcol;
	    DrawVLine(cv,pixmap,0,lbcolor,false,NULL,NULL);
	    DrawLine(cv,pixmap,-8096,0,8096,0,coordcol);
	    DrawLine(cv,pixmap,-8096,sf->ascent,8096,sf->ascent,ascentdescentcol);
	    DrawLine(cv,pixmap,-8096,-sf->descent,8096,-sf->descent,ascentdescentcol);
	}
	if ( cv->showvmetrics ) {
	    /*DrawLine(cv,pixmap,(sf->ascent+sf->descent)/2,-8096,(sf->ascent+sf->descent)/2,8096,coordcol);
	    DrawLine(cv,pixmap,-8096,sf->vertical_origin,8096,sf->vertical_origin,coordcol);*/
	}

	DrawSelImageList(cv,pixmap,cv->b.layerheads[cv->b.drawmode]->images);

	/* Wrong order, I know. But it is useful to have the background */
	/*  visible on top of the fill... */
	GlyphHasBeenFilled = CVExposeGlyphFill(cv, pixmap, event, &clip );
    } else {
	/* Draw FreeType Results */
	CVDrawGridRaster(cv,pixmap,&clip);
    }

    if ( cv->b.layerheads[cv->b.drawmode]->undoes!=NULL &&
	    cv->b.layerheads[cv->b.drawmode]->undoes->undotype==ut_tstate )
	DrawOldState(cv,pixmap,cv->b.layerheads[cv->b.drawmode]->undoes, &clip);

    if ( cv->showfore )
	cv->showback[0] |= (1<<ly_fore);
    else
	cv->showback[0] &= ~(1<<ly_fore);

    for ( layer=ly_back; layer<cv->b.sc->layer_cnt; ++layer ) if ( layer!=cvlayer ) {
	if ( cv->showback[layer>>5]&(1<<(layer&31)) ) {
	    /* Used to draw the image list here, but that's too slow. Optimization*/
	    /*  is to draw to pixmap, dump pixmap a bit earlier */
	    /* Then when we moved the fill image around, we had to deal with the */
	    /*  images before the fill... */
	    int activelayer = CVLayer(&cv->b);

	    enum outlinesfm_flags strokeFillMode = sfm_stroke;
	    if( cv->inPreviewMode )
		strokeFillMode = sfm_nothing;
	    if( layer == activelayer && layer >= ly_back )
		strokeFillMode = sfm_fill;

	    CVDrawLayerSplineSet(cv,pixmap,&cv->b.sc->layers[layer],
	    			 !sf->multilayer || layer==ly_back ? backoutlinecol : foreoutlinecol,
	    			 false,&clip,strokeFillMode);
	    for ( rf=cv->b.sc->layers[layer].refs; rf!=NULL; rf = rf->next ) {
		if ( /* cv->b.drawmode==dm_back &&*/ cv->showrefnames )
		    CVDrawRefName(cv,pixmap,rf,0);
		for ( rlayer=0; rlayer<rf->layer_cnt; ++rlayer )
		    CVDrawSplineSet(cv,pixmap,rf->layers[rlayer].splines, backoutlinecol,false,&clip);
		if ( rf->selected && cv->b.layerheads[cv->b.drawmode]==&cv->b.sc->layers[layer])
		    CVDrawBB(cv,pixmap,&rf->bb);
	    }
	}
    }
    if ( cv->mmvisible!=0 )
	DrawMMGhosts(cv,pixmap,&clip);
    if ( cv->template1!=NULL )
	CVDrawTemplates(cv,pixmap,cv->template1,&clip);
    if ( cv->template2!=NULL )
	CVDrawTemplates(cv,pixmap,cv->template2,&clip);

    /* Draw the active layer last so its splines are on top. */
    /* Note that we don't check whether the layer is visible or not, we always*/
    /*  draw the current layer -- unless they've turned on grid fit. Then they*/
    /*  might want to hide the active layer. */
    layer = cvlayer;


    /*
     * If we have a pretransform_spl and the user wants to see it then show it to them
     */
     if(cv->p.pretransform_spl) {
         CVDrawSplineSetSpecialized(cv, pixmap, cv->p.pretransform_spl,
            DraggingComparisonOutlineColor, 1, &clip, sfm_stroke_trans,
            DraggingComparisonAlphaChannelOverride);
     }

    /* The call to CVExposeGlyphFill() above will have rendered a filled glyph already. */
    /* We draw the outline only at this stage so as to have it layered */
    /* over the control points if they are currently visible. */
    /* CVDrawLayerSplineSet() will draw both the control points, and the font outline over those */
    /* NB:
     *     Drawing the stroked outline may also use the color
     *     clippathcol for some splines, so we can't really avoid a
     *     restroke unless we are sure
     *     FOR-ALL(splines):spl->is_clip_path==0 */
    if( shouldShowFilledUsingCairo(cv) ) {
	strokeFillMode = sfm_stroke;
    }
    if( GlyphHasBeenFilled ) {
	strokeFillMode = sfm_stroke_trans;
    }

    if ( layer<0 ) /* Guide lines are special */
	CVDrawLayerSplineSet( cv,pixmap,cv->b.layerheads[cv->b.drawmode],foreoutlinecol,
			      cv->showpoints ,&clip, strokeFillMode );
    else if ( (cv->showback[layer>>5]&(1<<(layer&31))) ||
	    (!cv->show_ft_results && cv->dv==NULL ))
    {
	CVExposeReferences( cv, pixmap, cv->b.sc, layer, &clip );
    }
    if ( layer>=0 )
    {
	if( cv->dv && !(cv->showback[layer>>5]&(1<<(layer&31))))
	{
	    // MIQ 2013 feb: issue/168
	    // turn off the glyph outline if we are in debug mode and
	    // the layer is not visible
	}
	else
	{
	    CVDrawLayerSplineSet( cv,pixmap,&cv->b.sc->layers[layer],foreoutlinecol,
	    			  cv->showpoints ,&clip, strokeFillMode );

	    
	    int showpoints = 0;
	    enum outlinesfm_flags sm = sfm_stroke;
	    if( cv->inPreviewMode ) {
		sm = sfm_fill;
	    }

	    int ridx = cv->additionalCharsToShowActiveIndex+1;
//	    TRACE("expose(b) additionalCharsToShowActiveIndex:%d\n", cv->additionalCharsToShowActiveIndex );
	    if( cv->additionalCharsToShow[ ridx ] )
	    {
		int i = 1;
		int originalxoff = tab->xoff;
		int offset = tab->scale * cv->b.sc->width;
		for( i=ridx; i < additionalCharsToShowLimit; i++ )
		{
//		    TRACE("expose(right) loop:%d\n", i );
		    SplineChar* xc = cv->additionalCharsToShow[i];
		    if( !xc )
			break;

		    tab->xoff += offset;
		    CVExposeReferences(   cv, pixmap, xc, layer, &clip );
		    CVDrawLayerSplineSet( cv, pixmap, &xc->layers[layer], foreoutlinecol,
					  showpoints ,&clip, sm );
		    offset = tab->scale * xc->width;
		}
		tab->xoff = originalxoff;
	    }


	    
	    if( cv->additionalCharsToShowActiveIndex > 0 )
	    {
		int i = 1;
		int originalxoff = tab->xoff;
		int offset = 0;
		    
		for( i=cv->additionalCharsToShowActiveIndex-1; i >= 0; i-- )
		{
//		    TRACE("expose(left) loop:%d\n", i );
		    SplineChar* xc = cv->additionalCharsToShow[i];
		    if( !xc )
			break;

		    offset = tab->scale * xc->width;
		    tab->xoff -= offset;
		    CVExposeReferences(   cv, pixmap, xc, layer, &clip );
		    CVDrawLayerSplineSet( cv, pixmap, &xc->layers[layer], foreoutlinecol,
					  showpoints ,&clip, sm );
		}
		tab->xoff = originalxoff;
	    }
		
//	    TRACE("expose(e) ridx:%d\n", ridx );

	}
    }


    if ( cv->freehand.current_trace )
    	CVDrawSplineSet( cv,pixmap,cv->freehand.current_trace,tracecol,
    			 false,&clip);

    if ( cv->showhmetrics && (cv->b.container==NULL || cv->b.container->funcs->type==cvc_mathkern) ) {
	RefChar *lock = HasUseMyMetrics(cv->b.sc,cvlayer);
	if ( lock!=NULL ) cv->b.sc->width = lock->sc->width;
	DrawVLine(cv,pixmap,cv->b.sc->width,(!cv->inactive && cv->widthsel)?widthselcol:widthcol,true,
		lock!=NULL ? &GIcon_lock : NULL, NULL);
	if ( cv->b.sc->italic_correction!=TEX_UNDEF && cv->b.sc->italic_correction!=0 ) {
	    GDrawSetDashedLine(pixmap,2,2,0);
	    DrawVLine(cv,pixmap,cv->b.sc->width+cv->b.sc->italic_correction,(!cv->inactive && cv->icsel)?widthselcol:widthcol,
/* GT: Italic Correction */
		    false, NULL,_("ItalicCor."));
	    GDrawSetDashedLine(pixmap,0,0,0);
	}
    }
    if ( cv->showhmetrics && cv->b.container==NULL ) {
	for ( pst=cv->b.sc->possub; pst!=NULL && pst->type!=pst_lcaret; pst=pst->next );
	if ( pst!=NULL ) {
	    for ( i=0; i<pst->u.lcaret.cnt; ++i )
		DrawVLine(cv,pixmap,pst->u.lcaret.carets[i],lcaretcol,true,NULL,_("Lig.Caret"));
	}
	if ( cv->show_ft_results || cv->dv!=NULL )
	    DrawVLine(cv,pixmap,cv->b.ft_gridfitwidth,widthgridfitcol,true,NULL,NULL);
	if ( cv->b.sc->top_accent_horiz!=TEX_UNDEF )
	    DrawVLine(cv,pixmap,cv->b.sc->top_accent_horiz,(!cv->inactive && cv->tah_sel)?widthselcol:anchorcol,true,
		    NULL,_("TopAccent"));
    }
    if ( cv->showvmetrics ) {
	int vertical_height = -cv->b.sc->vwidth + cv->b.sc->parent->ascent;
	int len, y = -tab->yoff + cv->height - rint(vertical_height*tab->scale);
	DrawLine(cv,pixmap,-32768,vertical_height,32767,vertical_height,
		(!cv->inactive && cv->vwidthsel)?widthselcol:widthcol);
	if ( y>-40 && y<cv->height+40 ) {
	    dtos( buf, cv->b.sc->vwidth);
	    GDrawSetFont(pixmap,cv->small);
	    len = GDrawGetText8Width(pixmap,buf,-1);
	    GDrawDrawText8(pixmap,cv->width-len-5,y,buf,-1,metricslabelcol);
	}
    }
    if ( cv->showsidebearings && cv->showfore &&
	    (cv->showvmetrics || cv->showhmetrics))
	CVSideBearings(pixmap,cv);

    if ((( cv->active_tool >= cvt_scale && cv->active_tool <= cvt_perspective ) ||
		cv->active_shape!=NULL ) &&
	    cv->p.pressed )
	DrawTransOrigin(cv,pixmap);
    if ( cv->dv==NULL || (cv->showback[ly_fore>>5]&(1<<(ly_fore&31))) )
	CVDrawAnchorPoints(cv,pixmap);
    if ( cv->apmine!=NULL )
	DrawAPMatch(cv,pixmap,&clip);

    if ( cv->p.rubberbanding || cv->p.rubberlining ) {
	if ( cv->p.rubberbanding )
	    CVDrawRubberRect(pixmap,cv);
	if ( cv->p.rubberlining )
	    CVDrawRubberLine(pixmap,cv);
    }
    CVRulerExpose(pixmap,cv);

    if (cv->guide_pos != -1) {
        CVDrawGuideLine(cv, cv->guide_pos);
    }

    GDrawPopClip(pixmap,&old);
}

static void SC_UpdateAll(SplineChar *sc) {
    CharView *cv;
    struct splinecharlist *dlist;
    MetricsView *mv;
    FontView *fv;

    for ( cv=(CharView *) (sc->views); cv!=NULL; cv=(CharView *) (cv->b.next) )
	GDrawRequestExpose(cv->v,NULL,false);
    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next )
	SCUpdateAll(dlist->sc);
    if ( sc->parent!=NULL ) {
	for ( fv = (FontView *) (sc->parent->fv); fv!=NULL; fv=(FontView *) (fv->b.nextsame) )
	    FVRegenChar(fv,sc);
	for ( mv = sc->parent->metrics; mv!=NULL; mv=mv->next )
	    MVRegenChar(mv,sc);
    }
}

static void SC_OutOfDateBackground(SplineChar *sc) {
    CharView *cv;

    for ( cv=(CharView *) (sc->views); cv!=NULL; cv=(CharView *) (cv->b.next) )
	cv->back_img_out_of_date = true;
}

/* CVRegenFill() regenerates data used to show or not show paths as filled */
/* This is not static so that it can be called from the layers palette */
void CVRegenFill(CharView *cv) {
    CharViewTab* tab = CVGetActiveTab(cv);
    BDFCharFree(cv->filled);
    cv->filled = NULL;
    if ( cv->showfilled && !shouldShowFilledUsingCairo(cv) ) {
	extern int use_freetype_to_rasterize_fv;
	int layer = CVLayer((CharViewBase *) cv);
	int size = tab->scale*(cv->b.fv->sf->ascent+cv->b.fv->sf->descent);
	int clut_len= 2;

        if ( layer==ly_grid ) layer=ly_fore; /* otherwise crashes when using guides layer! */

	/* Generally I don't think there's much point in doing an anti-aliased*/
	/*  fill. But on the "M" (and "W") glyph of extravigant caps, ft won't*/
	/*  do a mono fill */
	if ( use_freetype_to_rasterize_fv && hasFreeType()) {
	    int depth = 1;
	    if( use_freetype_with_aa_fill_cv ) {
		depth = 4;
		clut_len = 16;
	    }
	    cv->filled = SplineCharFreeTypeRasterizeNoHints(cv->b.sc,layer,
		size,72, depth);
	    if ( cv->filled==NULL && size<2000 ) {
		/* There are some glyphs which freetype won't rasterize in */
		/* mono mode, but will in grey scale. Don't ask me why */
		cv->filled = SplineCharFreeTypeRasterizeNoHints(cv->b.sc,
		    layer, size, 72, 4);
		clut_len = 16;
	    }
	}
	if ( cv->filled==NULL )
	    cv->filled = SplineCharRasterize(cv->b.sc,layer,size+.1);
	if ( cv->filled==NULL )
return;
	cv->gi.u.image->image_type = clut_len==2 ? it_mono : it_index;
	cv->gi.u.image->data = cv->filled->bitmap;
	cv->gi.u.image->bytes_per_line = cv->filled->bytes_per_line;
	cv->gi.u.image->width = cv->filled->xmax-cv->filled->xmin+1;
	cv->gi.u.image->height = cv->filled->ymax-cv->filled->ymin+1;
	if ( clut_len!=cv->gi.u.image->clut->clut_len ) {
	    GClut *clut = cv->gi.u.image->clut;
	    int i;
	    Color bg = view_bgcol;
	    for ( i=0; i<clut_len; ++i ) {
		int r,g,b;
		r = ((bg>>16)&0xff)*(clut_len-1-i) + ((fillcol>>16)&0xff)*i;
		g = ((bg>>8 )&0xff)*(clut_len-1-i) + ((fillcol>>8 )&0xff)*i;
		b = ((bg    )&0xff)*(clut_len-1-i) + ((fillcol    )&0xff)*i;
		clut->clut[i] = COLOR_CREATE(r/(clut_len-1),g/(clut_len-1),b/(clut_len-1));
	    }
	    clut->clut_len = clut_len;
	}
	GDrawRequestExpose(cv->v,NULL,false);
    }
}


static void FVRedrawAllCharViewsSF(SplineFont *sf)
{
    int i;
    CharView *cv;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	for ( cv = (CharView *) (sf->glyphs[i]->views); cv!=NULL; cv=(CharView *) (cv->b.next) )
	    GDrawRequestExpose(cv->v,NULL,false);
}

void FVRedrawAllCharViews(FontView *fv)
{
    FVRedrawAllCharViewsSF( fv->b.sf );
}

static void SCRegenFills(SplineChar *sc) {
    struct splinecharlist *dlist;
    CharView *cv;

    for ( cv = (CharView *) (sc->views); cv!=NULL; cv=(CharView *) (cv->b.next) )
	CVRegenFill(cv);
    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next )
	SCRegenFills(dlist->sc);
}

static void SCRegenDependents(SplineChar *sc, int layer) {
    struct splinecharlist *dlist;
    FontView *fv;
    int first, last;

    first = last = layer;
    if ( layer==ly_all ) {
	first = 0; last = sc->layer_cnt-1;
    }
    for ( layer = first; layer<=last; ++layer ) {
	for ( fv = (FontView *) (sc->parent->fv); fv!=NULL; fv=(FontView *) (fv->b.nextsame) ) {
	    if ( fv->sv!=NULL && layer<=ly_fore ) {
		SCReinstanciateRef(&fv->sv->sd.sc_srch,sc,layer);
		SCReinstanciateRef(&fv->sv->sd.sc_rpl,sc,layer);
	    }
	}

	for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next ) {
	    SCReinstanciateRef(dlist->sc,sc,layer);
	    SCRegenDependents(dlist->sc,layer);
	}
    }
}

static void CVUpdateInfo(CharView *cv, GEvent *event) {
    CharViewTab* tab = CVGetActiveTab(cv);

    cv->info_within = true;
    cv->e.x = event->u.mouse.x; cv->e.y = event->u.mouse.y;
    cv->info.x = (event->u.mouse.x-tab->xoff)/tab->scale;
    cv->info.y = (cv->height-event->u.mouse.y-tab->yoff)/tab->scale;
    CVInfoDraw(cv,cv->gw);
}

static void CVNewScale(CharView *cv) {
    CharViewTab* tab = CVGetActiveTab(cv);
    GEvent e;

    CVRegenFill(cv);
    cv->back_img_out_of_date = true;

    GScrollBarSetBounds(cv->vsb,-20000*tab->scale,8000*tab->scale,cv->height);
    GScrollBarSetBounds(cv->hsb,-8000*tab->scale,32000*tab->scale,cv->width);
    GScrollBarSetPos(cv->vsb,tab->yoff-cv->height);
    GScrollBarSetPos(cv->hsb,-tab->xoff);

    GDrawRequestExpose(cv->v,NULL,false);
    if ( cv->showrulers )
	GDrawRequestExpose(cv->gw,NULL,false);
    GDrawGetPointerPosition(cv->v,&e);
    CVRulerLingerMove(cv);
    CVUpdateInfo(cv,&e);
}

static void _CVFit(CharView *cv,DBounds *b, int integral) {
    CharViewTab* tab = CVGetActiveTab(cv);
    real left, right, top, bottom, hsc, wsc;
    extern int palettes_docked;
    int offset = palettes_docked ? 90 : 0;
    int em = cv->b.sc->parent->ascent + cv->b.sc->parent->descent;
    int hsmall = true;

    if ( offset>cv->width ) offset = 0;

    bottom = b->miny;
    top = b->maxy;
    left = b->minx;
    right = b->maxx;

    if ( top<bottom ) IError("Bottom bigger than top!");
    if ( right<left ) IError("Left bigger than right!");
    top -= bottom;
    right -= left;
    if ( top==0 ) top = em;
    if ( right==0 ) right = em;
    wsc = (cv->width-offset) / right;
    hsc = cv->height / top;
    if ( wsc<hsc ) { hsc = wsc; hsmall = false ; }

    tab->scale = hsc;
    if ( integral ) {
	if ( tab->scale > 1.0 ) {
	    tab->scale = floor(tab->scale);
	} else {
	    tab->scale = 1/ceil(1/tab->scale);
	}
    } else {
	if ( tab->scale > 1.0 ) {
	    tab->scale = floor(2*tab->scale)/2;
	} else {
	    tab->scale = 2/ceil(2/tab->scale);
	}
    }

    /* Center glyph horizontally */
    tab->xoff = ( (cv->width-offset) - (right*tab->scale) )/2 + offset  - b->minx*tab->scale;
    if ( hsmall )
	tab->yoff = -bottom*tab->scale;
    else
	tab->yoff = -(bottom+top/2)*tab->scale + cv->height/2;

    CVNewScale(cv);
}

static void CVFit(CharView *cv) {
    DBounds b;
    double center;

    SplineCharFindBounds(cv->b.sc,&b);
    if ( b.miny==0 && b.maxy==0 ) {
	b.maxy =cv->b.sc->parent->ascent;
	b.miny = -cv->b.sc->parent->descent;
    }
    /* FF used to normalize bounding boxes making maxx positive. But this */
    /* may result into an incorrect positioning for combining marks, which */
    /* usually have both bearings negative. So keep negative values as they are. */
    /* As for the Y axis, we only ensure the minimum value doesn't exceed zero, */
    /* so that the baseline is always visible. */

    if ( b.miny>0 ) b.miny = 0;

    /* Now give some extra space around the interesting stuff */
    center = (b.maxx+b.minx)/2;
    b.minx = center - (center - b.minx)*1.2;
    b.maxx = center + (b.maxx - center)*1.2;
    center = (b.maxy+b.miny)/2;
    b.miny = center - (center - b.miny)*1.2;
    b.maxy = center + (b.maxy - center)*1.2;

    _CVFit(cv,&b,true);
}

void CVUnlinkView(CharView *cv ) {
    CharView *test;

    if ( cv->b.sc->views == (CharViewBase *) cv ) {
	cv->b.sc->views = cv->b.next;
    } else {
	for ( test=(CharView *) (cv->b.sc->views);
		test->b.next!=(CharViewBase *) cv && test->b.next!=NULL;
		test=(CharView *) (test->b.next) );
	if ( test->b.next==(CharViewBase *) cv )
	    test->b.next = cv->b.next;
    }
}

static GWindow CharIcon(CharView *cv, FontView *fv) {
    SplineChar *sc = cv->b.sc;
    BDFFont *bdf, *bdf2;
    BDFChar *bdfc;
    GWindow icon = cv->icon;
    GRect r;

    r.x = r.y = 0; r.width = r.height = fv->cbw-1;
    if ( icon == NULL )
	cv->icon = icon = GDrawCreatePixmap(NULL,NULL,r.width,r.width);
    GDrawFillRect(icon,&r,0x0);		/* for some reason icons seem to be color reversed by my defn */

    bdf = NULL; bdfc = NULL;
    if ( sc->layers[ly_fore].refs!=NULL || sc->layers[ly_fore].splines!=NULL ) {
	bdf = fv->show;
	if ( sc->orig_pos>=bdf->glyphcnt || bdf->glyphs[sc->orig_pos]==NULL )
	    bdf = fv->filled;
	if ( sc->orig_pos>=bdf->glyphcnt || bdf->glyphs[sc->orig_pos]==NULL ) {
	    bdf2 = NULL; bdfc = NULL;
	    for ( bdf=fv->b.sf->bitmaps; bdf!=NULL && bdf->pixelsize<24 ; bdf=bdf->next )
		bdf2 = bdf;
	    if ( bdf2!=NULL && bdf!=NULL ) {
		if ( 24-bdf2->pixelsize < bdf->pixelsize-24 )
		    bdf = bdf2;
	    } else if ( bdf==NULL )
		bdf = bdf2;
	}
	if ( bdf!=NULL && sc->orig_pos<bdf->glyphcnt )
	    bdfc = bdf->glyphs[sc->orig_pos];
    }

    // Leaving the hard-coded colors here because this stuff is window-system
    // dependent and not necessarily user-configurable (although it might or
    // might not be appropriate to make them theme-configurable)
    if ( bdfc!=NULL ) {
	GClut clut;
	struct _GImage base;
	GImage gi;
	/* if not empty, use the font's own shape, otherwise use a standard */
	/*  font */
	memset(&gi,'\0',sizeof(gi));
	memset(&base,'\0',sizeof(base));
	memset(&clut,'\0',sizeof(clut));
	gi.u.image = &base;
	base.trans = -1;
	base.clut = &clut;
	if ( bdfc->byte_data ) { int i;
	    base.image_type = it_index;
	    clut.clut_len = bdf->clut->clut_len;
	    for ( i=0; i<clut.clut_len; ++i ) {
		int v = 255-i*255/(clut.clut_len-1);
		clut.clut[i] = COLOR_CREATE(v,v,v);
	    }
	    clut.trans_index = -1;
	} else {
	    base.image_type = it_mono;
	    clut.clut_len = 2;
	    clut.clut[1] = 0xffffff;
	}
	base.data = bdfc->bitmap;
	base.bytes_per_line = bdfc->bytes_per_line;
	base.width = bdfc->xmax-bdfc->xmin+1;
	base.height = bdfc->ymax-bdfc->ymin+1;
	GDrawDrawImage(icon,&gi,NULL,(r.width-base.width)/2,(r.height-base.height)/2);
    } else if ( sc->unicodeenc!=-1 ) {
	if ( cv_iconfont.fi==NULL )
	    GResEditDoInit(&charviewpoints_ri);
	unichar_t text[2];
	int as, ds, ld, width;
	GDrawSetFont(icon,cv_iconfont.fi);
	text[0] = sc->unicodeenc; text[1] = 0;
	GDrawWindowFontMetrics(icon,cv_iconfont.fi,&as,&ds,&ld);
	width = GDrawGetTextWidth(icon,text,1);
	GDrawDrawText(icon,(r.width-width)/2,(r.height-as-ds)/2+as,text,1,0xffffff);
    }
return( icon );
}

static int CVCurEnc(CharView *cv)
{
    return( ((FontView *) (cv->b.fv))->b.map->backmap[cv->b.sc->orig_pos] );
}

static char *CVMakeTitles(CharView *cv,char *buf,size_t len) {
    char *title;
    SplineChar *sc = cv->b.sc;
    SplineFont *sf = sc->parent;
    char *uniname;
    size_t used;

/* GT: This is the title for a window showing an outline character */
/* GT: It will look something like: */
/* GT:  exclam at 33 from Arial */
/* GT: $1 is the name of the glyph */
/* GT: $2 is the glyph's encoding */
/* GT: $3 is the font name */
/* GT: $4 is the changed flag ('*' for the changed items)*/
    used = snprintf(buf,len,_("%1$.80s at %2$d from %3$.90s%4$s"),
		    sc->name, CVCurEnc(cv), sf->fontname,
		    sc->changed ? "*" : "");
    title = copy(buf);

    if (used < len) {
	/* Enhance 'buf' description with Nameslist.txt unicode name definition */
	if ( (uniname=uniname_name(sc->unicodeenc))!=NULL ) {
	    used += snprintf(buf+used, len-used, " %s", uniname);
	    free(uniname);
	}
    }

    if (used < len && ( cv->show_ft_results || cv->dv )) {
	snprintf(buf+used, len-used, " (%gpt, %ddpi)", (double) cv->ft_pointsizey, cv->ft_dpi );
    }

    return( title );
}

static void SC_RefreshTitles(SplineChar *sc) {
    /* Called if the user changes the unicode encoding or the character name */
    CharView *cv;
    char buf[300], *title;

    if ( (CharView *) (sc->views)==NULL )
return;
    for ( cv = (CharView *) (sc->views); cv!=NULL; cv=(CharView *) (cv->b.next) ) {
	title = CVMakeTitles(cv,buf,sizeof(buf));
	/* Could be different if one window is debugging and one is not */
	GDrawSetWindowTitles8(cv->gw,buf,title);
	free(title);
    }
}

static void CVChangeTabsVisibility(CharView *cv,int makevisible) {
    GRect gsize, pos, sbsize;

    if ( cv->tabs==NULL || GGadgetIsVisible(cv->tabs)==makevisible )
return;
    GGadgetGetSize(cv->tabs,&gsize);
    GGadgetGetSize(cv->vsb,&sbsize);
    if ( makevisible ) {
	cv->mbh += gsize.height;
	cv->height -= gsize.height;
	GGadgetMove(cv->vsb,sbsize.x,sbsize.y+gsize.height);
	GGadgetResize(cv->vsb,sbsize.width,sbsize.height-gsize.height);
	GGadgetMoveAddToY( cv->charselector, gsize.height );
	GGadgetMoveAddToY( cv->charselectorNext, gsize.height );
	GGadgetMoveAddToY( cv->charselectorPrev, gsize.height );
    } else {
	cv->mbh -= gsize.height;
	cv->height += gsize.height;
	GGadgetMove(cv->vsb,sbsize.x,sbsize.y-gsize.height);
	GGadgetResize(cv->vsb,sbsize.width,sbsize.height+gsize.height);
	GGadgetMoveAddToY( cv->charselector, -1*gsize.height );
	GGadgetMoveAddToY( cv->charselectorNext, -1*gsize.height );
	GGadgetMoveAddToY( cv->charselectorPrev, -1*gsize.height );
    }
    GGadgetSetVisible(cv->tabs,makevisible);
    cv->back_img_out_of_date = true;
    pos.x = 0; pos.y = cv->mbh+cv->charselectorh+cv->infoh;
    pos.width = cv->width; pos.height = cv->height;
    if ( cv->showrulers ) {
	pos.x += cv->rulerh;
	pos.y += cv->rulerh;
    }
    GDrawMoveResize(cv->v,pos.x,pos.y,pos.width,pos.height);
    GDrawSync(NULL);
    GDrawRequestExpose(cv->v,NULL,false);
    GDrawRequestExpose(cv->gw,NULL,false);
}

static void CVCheckPoints(CharView *cv) {
    if ( !SCPointsNumberedProperly(cv->b.sc,CVLayer((CharViewBase *) cv)) && cv->b.sc->ttf_instrs_len!=0 ) {
	char *buts[3];
	int answer;
	buts[0] = _("_Yes");
	buts[1] = _("_No");
	buts[2] = NULL;
	answer = ff_ask(_("Bad Point Numbering"),(const char **) buts,0,1,_("The points in %s are not numbered properly. This means that any instructions will probably move the wrong points and do the wrong thing.\nWould you like me to remove the instructions?"),cv->b.sc->name);
	if ( answer==0 ) {
	    free(cv->b.sc->ttf_instrs); cv->b.sc->ttf_instrs = NULL;
	    cv->b.sc->ttf_instrs_len = 0;
	}
    }
}

static void CVChangeSC_storeTab( CharView *cv, int tabnumber )
{
    TRACE("CVChangeSC_storeTab() %d\n", tabnumber );
    if( tabnumber < charview_cvtabssz )
    {
	CharViewTab* t = &cv->cvtabs[tabnumber];
	strncpy( t->charselected,
		 GGadgetGetTitle8(cv->charselector),
		 charviewtab_charselectedsz );
    }
}

static void CVChangeSC_fetchTab( CharView *cv, int tabnumber )
{
    if( tabnumber < charview_cvtabssz )
    {
	CharViewTab* t = &cv->cvtabs[tabnumber];
	GGadgetSetTitle8(cv->charselector, t->charselected );
    }
}

static void CVSetCharSelectorValueFromSC( CharView *cv, SplineChar *sc )
{
    const char* title = Wordlist_getSCName( sc );
    GGadgetSetTitle8(cv->charselector, title);
}
	    
// See comment in gtabset.c (fn GTabSetRemoveTabByPos)
static void CVMenuCloseTab(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e));
static void CVTabSetRemoveSync(GWindow gw, int pos) {
    CharView* cv = (CharView*) GDrawGetUserData(gw);
    // There's no other good way to pass an "argument" to this function as its definition
    // is required to be a certain way. The user_data of the GWindow is already our CharView
    // so we can't use that.
    cv->ctpos = pos;
    CVMenuCloseTab(gw, NULL, NULL);
}

static void CVTabSetSwapSync(GWindow gw, int pos_a, int pos_b) {
    CharView* cv = (CharView*) GDrawGetUserData(gw);
    CharViewTab tempt = cv->cvtabs[pos_a];
    char* fnt = cv->former_names[pos_a];

    cv->cvtabs[pos_a] = cv->cvtabs[pos_b];
    cv->former_names[pos_a] = cv->former_names[pos_b];
    cv->cvtabs[pos_b] = tempt;
    cv->former_names[pos_b] = fnt;

    int sel = GTabSetGetSel(cv->tabs);
    if (pos_a == sel)
    cv->oldtabnum = pos_b;
    if (pos_b == sel)
    cv->oldtabnum = pos_a;
}

void CVChangeSC( CharView *cv, SplineChar *sc )
{
    char *title;
    char buf[300];
    extern int updateflex;
    int i;
    int old_layer = CVLayer((CharViewBase *) cv), blayer;
    int was_fitted = cv->dv==NULL && cv->b.gridfit!=NULL;

    if ( old_layer>=sc->layer_cnt )
	old_layer = ly_fore;		/* Can happen in type3 fonts where each glyph has a different layer cnt */

    memset( cv->additionalCharsToShow, 0, sizeof(SplineChar*) * additionalCharsToShowLimit );
    cv->additionalCharsToShowActiveIndex = 0;
    cv->additionalCharsToShow[0] = sc;
    
    CVDebugFree(cv->dv);

    if ( cv->expandedge != ee_none ) {
	GDrawSetCursor(cv->v,ct_mypointer);
	cv->expandedge = ee_none;
    }

    SplinePointListsFree(cv->b.gridfit); cv->b.gridfit = NULL;
    FreeType_FreeRaster(cv->oldraster); cv->oldraster = NULL;
    FreeType_FreeRaster(cv->raster); cv->raster = NULL;

    SCLigCaretCheck(sc,false);

    CVUnlinkView(cv);
    cv->p.nextcp = cv->p.prevcp = cv->widthsel = cv->vwidthsel = false;
    if ( (CharView *) (sc->views)==NULL && updateflex )
	SplineCharIsFlexible(sc,old_layer!=ly_grid ? old_layer : ly_fore );
    cv->b.sc = sc;
    cv->b.next = sc->views;
    sc->views = &cv->b;
    cv->enc = ( ((FontView *) (cv->b.fv))->b.map->backmap[cv->b.sc->orig_pos] );
    cv->b.layerheads[dm_fore] = &sc->layers[ly_fore];
    blayer = old_layer;
    if ( old_layer==ly_grid || old_layer==ly_fore ||
	    sc->parent->multilayer || old_layer>=sc->layer_cnt )
	blayer = ly_back;
    cv->b.layerheads[dm_back] = &sc->layers[blayer];
    cv->b.layerheads[dm_grid] = &sc->parent->grid;
    cv->p.sp = cv->lastselpt = NULL;
    cv->p.spiro = cv->lastselcp = NULL;
    cv->apmine = cv->apmatch = NULL; cv->apsc = NULL;
    cv->template1 = cv->template2 = NULL;
#if HANYANG
    if ( cv->b.sc->parent->rules!=NULL && cv->b.sc->compositionunit )
	Disp_DefaultTemplate(cv);
#endif
    if ( cv->b.layerheads[cv->b.drawmode]->order2 )
	CVCheckPoints(cv);
    if ( cv->showpointnumbers || cv->show_ft_results )
	SCNumberPoints(sc,old_layer);
    if ( cv->show_ft_results )
	CVGridFitChar(cv);

    CVNewScale(cv);

    CharIcon(cv,(FontView *) (cv->b.fv));
    title = CVMakeTitles(cv,buf,sizeof(buf));
    GDrawSetWindowTitles8(cv->gw,buf,title);
    CVInfoDraw(cv,cv->gw);
    free(title);
    _CVPaletteActivate(cv,true,false);

    if ( cv->tabs!=NULL ) {
	for ( i=0; i<cv->former_cnt; ++i )
	    if ( strcmp(cv->former_names[i],sc->name)==0 )
	break;
	if ( i!=cv->former_cnt && cv->showtabs )
	{
	    CVChangeSC_storeTab( cv, cv->oldtabnum );
	    CVChangeSC_fetchTab( cv, i );
	    cv->oldtabnum = i;
	    GTabSetSetSel(cv->tabs,i);
	}
	else
	{
	    // Only need to store here, as we are about to make a new tab.
	    CVChangeSC_storeTab( cv, cv->oldtabnum );
	    cv->oldtabnum = 0;
	    // have to shuffle the cvtabs along to be in sync with cv->tabs
	    {
		int i = 0;
		for( i=charview_cvtabssz-1; i > 0; i-- )
		{
		    cv->cvtabs[i] = cv->cvtabs[i-1];
		}
	    }
	    CVSetCharSelectorValueFromSC( cv, sc );

	    if ( cv->former_cnt==CV_TABMAX )
		free(cv->former_names[CV_TABMAX-1]);
	    for ( i=cv->former_cnt<CV_TABMAX?cv->former_cnt-1:CV_TABMAX-2; i>=0; --i )
		cv->former_names[i+1] = cv->former_names[i];
	    cv->former_names[0] = copy(sc->name);
	    if ( cv->former_cnt<CV_TABMAX )
		++cv->former_cnt;
	    for ( i=0; i<cv->former_cnt; ++i )
            {
                if( i < charview_cvtabssz )
                {
                    CharViewTab* t = &cv->cvtabs[i];
                    GTabSetChangeTabName(cv->tabs, t->charselected, i);
                }
            }
            
	    GTabSetRemetric(cv->tabs);
	    GTabSetSetSel(cv->tabs,0);	/* This does a redraw */
	    if ( !GGadgetIsVisible(cv->tabs) && cv->showtabs )
		CVChangeTabsVisibility(cv,true);
	}
    }
    if( !strcmp(GGadgetGetTitle8(cv->charselector),""))
	CVSetCharSelectorValueFromSC( cv, sc );

    if ( sc->inspiro && !hasspiro() && !sc->parent->complained_about_spiros ) {
	sc->parent->complained_about_spiros = true;
#ifdef _NO_LIBSPIRO
	ff_post_error(_("You may not use spiros"),_("This glyph should display spiro points, but unfortunately this version of fontforge was not linked with the spiro library, so only normal bezier points will be displayed."));
#else
	ff_post_error(_("You may not use spiros"),_("This glyph should display spiro points, but unfortunately FontForge was unable to load libspiro, spiros are not available for use, and normal bezier points will be displayed instead."));
#endif
    }

    if ( was_fitted )
	CVGridFitChar(cv);

    // Force any extra chars to be setup and drawn
    GEvent e;
    e.type=et_controlevent;
    e.u.control.subtype = et_textchanged;
    e.u.control.u.tf_changed.from_pulldown = 0;
    CV_OnCharSelectorTextChanged( cv->charselector, &e );
}

static void CVChangeChar(CharView *cv, int i )
{
    SplineChar *sc;
    SplineFont *sf = cv->b.sc->parent;
    EncMap *map = ((FontView *) (cv->b.fv))->b.map;
    int gid = i<0 || i>= map->enccount ? -2 : map->map[i];

    if ( sf->cidmaster!=NULL && !map->enc->is_compact ) {
	SplineFont *cidmaster = sf->cidmaster;
	int k;
	for ( k=0; k<cidmaster->subfontcnt; ++k ) {
	    SplineFont *sf = cidmaster->subfonts[k];
	    if ( i<sf->glyphcnt && sf->glyphs[i]!=NULL )
	break;
	}
	if ( k!=cidmaster->subfontcnt ) {
	    if ( cidmaster->subfonts[k] != sf ) {
		sf = cidmaster->subfonts[k];
		gid = ( i>=sf->glyphcnt ) ? -2 : i;
		/* can't create a new glyph this way */
	    }
	}
    }
    if ( gid == -2 )
return;
    if ( gid==-1 || (sc = sf->glyphs[gid])==NULL ) {
	sc = SFMakeChar(sf,map,i);
	sc->inspiro = cv->b.sc->inspiro && hasspiro();
    }

    if ( sc==NULL || (cv->b.sc == sc && cv->enc==i ))
return;
    cv->map_of_enc = map;
    cv->enc = i;
    CVChangeSC(cv,sc);
}

/*
 * Unused
static void CVSwitchToTab(CharView *cv,int tnum ) {
    if( tnum >= cv->former_cnt )
	return;

    SplineFont *sf = cv->b.fv->sf;
    char* n = cv->former_names[tnum];
    int unienc = UniFromName(n,sf->uni_interp,cv->b.fv->map->enc);
    CVChangeChar(cv,unienc);
}

static void CVMenuShowTab(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVSwitchToTab(cv,mi->mid);
}
*/

static int CVChangeToFormer( GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	CharView *cv = GDrawGetUserData(GGadgetGetWindow(g));
	int new_aspect = GTabSetGetSel(g);
	SplineFont *sf = cv->b.sc->parent;
	int gid;

	for ( gid=sf->glyphcnt-1; gid>=0; --gid )
	    if ( sf->glyphs[gid]!=NULL &&
		    strcmp(sf->glyphs[gid]->name,cv->former_names[new_aspect])==0 )
	break;
	if ( gid<0 ) {
	    /* They changed the name? See if we can get a unicode value from it */
	    int unienc = UniFromName(cv->former_names[new_aspect],sf->uni_interp,cv->b.fv->map->enc);
	    if ( unienc>=0 ) {
		gid = SFFindGID(sf,unienc,cv->former_names[new_aspect]);
		if ( gid>=0 ) {
		    free(cv->former_names[new_aspect]);
		    cv->former_names[new_aspect] = copy(sf->glyphs[gid]->name);
		}
	    }
	}
	if ( gid<0 )
return( true );
	CVChangeSC(cv,sf->glyphs[gid]);
	TRACE("CVChangeToFormer: Changed SC to %s (GID %d)\n", sf->glyphs[gid]->name, gid);
	cv->enc = ((FontView *) (cv->b.fv))->b.map->backmap[cv->b.sc->orig_pos];
    }
return( true );
}

static void CVFakeMove(CharView *cv, GEvent *event) {
    GEvent e;

    memset(&e,0,sizeof(e));
    e.type = et_mousemove;
    e.w = cv->v;
    if ( event->w!=cv->v ) {
	GPoint p;
	p.x = event->u.chr.x; p.y = event->u.chr.y;
	GDrawTranslateCoordinates(event->w,cv->v,&p);
	event->u.chr.x = p.x; event->u.chr.y = p.y;
    }
    e.u.mouse.state = TrueCharState(event);
    e.u.mouse.x = event->u.chr.x;
    e.u.mouse.y = event->u.chr.y;
    e.u.mouse.device = NULL;
    CVMouseMove(cv,&e);
}

static void CVDoFindInFontView(CharView *cv) {
    FVChangeChar((FontView *) (cv->b.fv),CVCurEnc(cv));
    GDrawSetVisible(((FontView *) (cv->b.fv))->gw,true);
    GDrawRaise(((FontView *) (cv->b.fv))->gw);
}

static uint16_t HaveModifiers = 0;
static uint16_t PressingTilde = 0;
static uint16_t PrevCharEventWasCharUpOnControl = 0;


static void CVCharUp(CharView *cv, GEvent *event ) {

    if ( !event->u.chr.autorepeat && !HaveModifiers && event->u.chr.keysym==' ' ) {
	update_spacebar_hand_tool(cv);
    }

    int oldactiveModifierControl = cv->activeModifierControl;
    int oldactiveModifierAlt = cv->activeModifierAlt;
    cv->activeModifierControl = cv->activeModifierControl && !( event->u.chr.keysym == GK_Control_L || event->u.chr.keysym == GK_Control_R
				    || event->u.chr.keysym == GK_Meta_L || event->u.chr.keysym == GK_Meta_R );
    cv->activeModifierAlt = cv->activeModifierAlt && !( event->u.chr.keysym == GK_Alt_L || event->u.chr.keysym == GK_Alt_R
				    || event->u.chr.keysym == GK_Mode_switch );
    // helps with keys on the mac
    if( (event->u.chr.state&ksm_meta) )
        cv->activeModifierAlt = 0;
    if( oldactiveModifierControl != cv->activeModifierControl
	|| oldactiveModifierAlt != cv->activeModifierAlt )
    {
	CVInfoDraw(cv,cv->gw);
    }

    

//    TRACE("CVCharUp() ag:%d key:%d\n", cv_auto_goto, event->u.chr.keysym );
    if( !cv_auto_goto )
    {
	bool isImmediateKeyTogglePreview = isImmediateKey( cv->gw, "TogglePreview", event ) != NULL;
	if( isImmediateKeyTogglePreview ) {
	    PressingTilde = 1;
	}

	if( PrevCharEventWasCharUpOnControl && isImmediateKeyTogglePreview )
	{
	    HaveModifiers = 0;
	    PrevCharEventWasCharUpOnControl = 0;
	    return;
	}
	PrevCharEventWasCharUpOnControl = 0;

	if( !event->u.chr.autorepeat
	    && (event->u.chr.keysym == GK_Control_L
		|| event->u.chr.keysym == GK_Control_R ))
	{
	    PrevCharEventWasCharUpOnControl = 1;
	    if( !PressingTilde ) {
		HaveModifiers = 0;
	    }
	}

	if ( !event->u.chr.autorepeat && !HaveModifiers && isImmediateKeyTogglePreview ) {
	    PressingTilde = 0;
	    CVPreviewModeSet( cv->gw, false );
	    return;
	}

	if ( !event->u.chr.autorepeat && isImmediateKeyTogglePreview ) {
	    PressingTilde = 0;
	}
	if ( event->u.chr.autorepeat && HaveModifiers && isImmediateKeyTogglePreview ) {
	    return;
	}
    }


    if( event->u.chr.keysym == GK_Escape )
    {
	TRACE("escape char.......!\n");
	GGadget *active = GWindowGetFocusGadgetOfWindow(cv->gw);
	if( active == cv->charselector )
	{
	    TRACE("was on charselector\n");
	    GWidgetIndicateFocusGadget( cv->hsb );
	}
	else if ( cv->charselector != NULL )
	{
	    TRACE("was on NOT charselector\n");
	    GWidgetIndicateFocusGadget( cv->charselector );
	}
    }
    

#if _ModKeysAutoRepeat
    /* Under cygwin these keys auto repeat, they don't under normal X */
    if ( event->u.chr.keysym == GK_Shift_L || event->u.chr.keysym == GK_Shift_R ||
	    event->u.chr.keysym == GK_Control_L || event->u.chr.keysym == GK_Control_R ||
	    event->u.chr.keysym == GK_Meta_L || event->u.chr.keysym == GK_Meta_R ||
	    event->u.chr.keysym == GK_Alt_L || event->u.chr.keysym == GK_Alt_R ||
	    event->u.chr.keysym == GK_Super_L || event->u.chr.keysym == GK_Super_R ||
	    event->u.chr.keysym == GK_Hyper_L || event->u.chr.keysym == GK_Hyper_R ) {
	if ( cv->autorpt!=NULL ) {
	    GDrawCancelTimer(cv->autorpt);
	    CVToolsSetCursor(cv,cv->oldstate,NULL);
	}
	cv->keysym = event->u.chr.keysym;
	cv->oldkeyx = event->u.chr.x;
	cv->oldkeyy = event->u.chr.y;
	cv->oldkeyw = event->w;
	cv->oldstate = TrueCharState(event);
	cv->autorpt = GDrawRequestTimer(cv->v,100,0,NULL);
    } else {
	if ( cv->autorpt!=NULL ) {
	    GDrawCancelTimer(cv->autorpt); cv->autorpt=NULL;
	    CVToolsSetCursor(cv,cv->oldstate,NULL);
	}
	CVToolsSetCursor(cv,TrueCharState(event),NULL);
    }
#else
    CVToolsSetCursor(cv,TrueCharState(event),NULL);
    if ( event->u.chr.keysym == GK_Shift_L || event->u.chr.keysym == GK_Shift_R ||
	     event->u.chr.keysym == GK_Alt_L || event->u.chr.keysym == GK_Alt_R ||
	     event->u.chr.keysym == GK_Meta_L || event->u.chr.keysym == GK_Meta_R )
	CVFakeMove(cv, event);
#endif
}

void CVInfoDrawText(CharView *cv, GWindow pixmap ) {
    CharViewTab* tab = CVGetActiveTab(cv);
    GRect r;
    Color bg = GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(pixmap));
    Color fg = GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap));
    const int buffersz = 150;
    char buffer[buffersz+1];
    int ybase = cv->mbh+cv->charselectorh+(cv->infoh-cv->sfh)/2+cv->sas;
    real xdiff, ydiff;
    SplinePoint *sp, dummy;
    spiro_cp *cp;

    GDrawSetFont(pixmap,cv->small);
    r.x = RPT_DATA; r.width = SPT_BASE-RPT_DATA;
    r.y = cv->mbh+cv->charselectorh; r.height = cv->infoh-1;
    GDrawFillRect(pixmap,&r,bg);
    r.x = SPT_DATA; r.width = SOF_BASE-SPT_DATA;
    GDrawFillRect(pixmap,&r,bg);
    r.x = SOF_DATA; r.width = SDS_BASE-SOF_DATA;
    GDrawFillRect(pixmap,&r,bg);
    r.x = SDS_DATA; r.width = SAN_BASE-SDS_DATA;
    GDrawFillRect(pixmap,&r,bg);
    r.x = SAN_DATA; r.width = MAG_BASE-SAN_DATA;
    GDrawFillRect(pixmap,&r,bg);
    r.x = MAG_DATA; r.width = LAYER_DATA-MAG_DATA;
    GDrawFillRect(pixmap,&r,bg);
    r.x = LAYER_DATA; r.width = CODERANGE_DATA-LAYER_DATA;
    GDrawFillRect(pixmap,&r,bg);
    r.x = CODERANGE_DATA; r.width = FLAGS_DATA-CODERANGE_DATA;
    GDrawFillRect(pixmap,&r,bg);
    r.x = FLAGS_DATA; r.width = 200;
    GDrawFillRect(pixmap,&r,bg);

    if ( cv->info_within ) {
	if ( cv->info.x>=1000 || cv->info.x<=-1000 || cv->info.y>=1000 || cv->info.y<=-1000 )
	    sprintf(buffer,"%d%s%d", (int) cv->info.x, coord_sep, (int) cv->info.y );
	else
	    sprintf(buffer,"%.4g%s%.4g", (double) cv->info.x, coord_sep, (double) cv->info.y );
	buffer[11] = '\0';
	GDrawDrawText8(pixmap,RPT_DATA,ybase,buffer,-1,fg);
    }
    if ( tab->scale>=.25 )
	sprintf( buffer, "%d%%", (int) (100*tab->scale));
    else
	sprintf( buffer, "%.3g%%", (double) (100*tab->scale));
    GDrawDrawText8(pixmap,MAG_DATA,ybase,buffer,-1,fg);

    const int layernamesz = 100;
    char layername[layernamesz+1];
    strncpy(layername,_("Guide"),layernamesz);
    if(cv->b.drawmode!=dm_grid) {
	int idx = CVLayer((CharViewBase *) cv);
	if(idx >= 0 && idx < cv->b.sc->parent->layer_cnt) {
	    strncpy(layername,cv->b.sc->parent->layers[idx].name,layernamesz);
	}
    }
    snprintf( buffer, buffersz, _("Active Layer: %s (%s)"),
	      layername,
/* GT: Guide layer, make it short */
	      ( cv->b.drawmode==dm_grid ? _("Guide") :
/* GT: Background, make it short */
		cv->b.layerheads[cv->b.drawmode]->background ? _("Back") :
/* GT: Foreground, make it short */
		_("Fore") )
	      );
    GDrawDrawText8(pixmap,LAYER_DATA,ybase,buffer,-1,fg);
    GDrawDrawText8(pixmap,LAYER_DATA,ybase,buffer,-1,fg);

    if ( cv->coderange==cr_none )
    {
	snprintf( buffer, buffersz, _("Modes: "));
	if( CVShouldInterpolateCPsOnMotion(cv) )
	    strcat( buffer, "Interpolate" );
	GDrawDrawText8(pixmap,FLAGS_DATA,ybase,buffer,-1,fg);
    }
    
    
    if ( cv->coderange!=cr_none ) {
	GDrawDrawText8(pixmap,CODERANGE_DATA,ybase,
		cv->coderange==cr_fpgm ? _("'fpgm'") :
		cv->coderange==cr_prep ? _("'prep'") : _("Glyph"),
	    -1,fg);
	GDrawDrawText8(pixmap,CODERANGE_DATA+40,ybase,
		FreeTypeStringVersion(), -1,fg);
    }
    sp = NULL; cp = NULL;
    if ( cv->b.sc->inspiro && hasspiro())
	cp = cv->p.spiro!=NULL ? cv->p.spiro : cv->lastselcp;
    else
	sp = cv->p.sp!=NULL ? cv->p.sp : cv->lastselpt;
    if ( sp==NULL && cp==NULL )
	    if ( cv->active_tool==cvt_rect || cv->active_tool==cvt_elipse ||
		    cv->active_tool==cvt_poly || cv->active_tool==cvt_star ||
		    cv->active_tool==cvt_scale || cv->active_tool==cvt_skew ||
		    cv->active_tool==cvt_rotate || cv->active_tool==cvt_flip ) {
	dummy.me.x = cv->p.cx; dummy.me.y = cv->p.cy;
	sp = &dummy;
    }
    if ( sp || cp ) {
	real selx, sely;
	if ( sp ) {
	    if ( cv->pressed && sp==cv->p.sp ) {
		selx = cv->p.constrain.x;
		sely = cv->p.constrain.y;
	    } else {
		selx = sp->me.x;
		sely = sp->me.y;
	    }
	} else {
	    selx = cp->x;
	    sely = cp->y;
	}
	xdiff=cv->info.x-selx;
	ydiff = cv->info.y-sely;

	if ( selx>=1000 || selx<=-1000 || sely>=1000 || sely<=-1000 )
	    sprintf(buffer,"%d%s%d", (int) selx, coord_sep, (int) sely );
	else
	    sprintf(buffer,"%.4g%s%.4g", (double) selx, coord_sep, (double) sely );
	buffer[11] = '\0';
	GDrawDrawText8(pixmap,SPT_DATA,ybase,buffer,-1,fg);
    } else if ( cv->widthsel && cv->info_within ) {
	xdiff = cv->info.x-cv->p.cx;
	ydiff = 0;
    } else if ( cv->p.rubberbanding && cv->info_within ) {
	xdiff=cv->info.x-cv->p.cx;
	ydiff = cv->info.y-cv->p.cy;
    } else
return;
    if ( !cv->info_within )
return;

    if ( cv->active_tool==cvt_scale ) {
	xdiff = 100.0 + (cv->info.x-cv->p.cx)/(4*tab->scale);
	ydiff = 100.0 + (cv->info.y-cv->p.cy)/(4*tab->scale);
	if ( xdiff>=100 || xdiff<=-100 || ydiff>=100 || ydiff<=-100 )
	    sprintf(buffer,"%d%%%s%d%%", (int) xdiff, coord_sep, (int) ydiff );
	else
	    sprintf(buffer,"%.3g%%%s%.3g%%", (double) xdiff, coord_sep, (double) ydiff );
    } else if ( xdiff>=1000 || xdiff<=-1000 || ydiff>=1000 || ydiff<=-1000 )
	sprintf(buffer,"%d%s%d", (int) xdiff, coord_sep, (int) ydiff );
    else
	sprintf(buffer,"%.4g%s%.4g", (double) xdiff, coord_sep, (double) ydiff );
    buffer[11] = '\0';
    GDrawDrawText8(pixmap,SOF_DATA,ybase,buffer,-1,fg);

    sprintf( buffer, "%.1f", sqrt(xdiff*xdiff+ydiff*ydiff));
    GDrawDrawText8(pixmap,SDS_DATA,ybase,buffer,-1,fg);

	/* Utf-8 for degree sign */
    sprintf( buffer, "%d\302\260", (int) rint(180*atan2(ydiff,xdiff)/FF_PI));
    GDrawDrawText8(pixmap,SAN_DATA,ybase,buffer,-1,fg);
}

static void CVInfoDrawRulers(CharView *cv, GWindow pixmap ) {
    // Check if we have any rulers to draw over
    if (cv->hruler == NULL || cv->vruler == NULL) {
        return;
    }

    int rstart = cv->mbh+cv->charselectorh+cv->infoh;
    GRect rh, rv, oldrh, oldrv;
    rh.y = rstart; rh.height = cv->rulerh; rh.x = cv->rulerh; rh.width = cv->width;
    rv.x = 0; rv.width = cv->rulerh; rv.y = rstart + cv->rulerh; rv.height = cv->height;

    GDrawSetLineWidth(pixmap,0);
    // Draw the new rulers
    GDrawPushClip(pixmap, &rh, &oldrh);
    rh.x = cv->olde.x; rh.y = 0; rh.width = 1;
    GDrawDrawPixmap(pixmap, cv->hruler, &rh, cv->rulerh + cv->olde.x, rstart);
    GDrawDrawLine(pixmap,cv->e.x+cv->rulerh,rstart,cv->e.x+cv->rulerh,rstart+cv->rulerh,rulercurtickcol);
    GDrawPopClip(pixmap, &oldrh);

    GDrawPushClip(pixmap, &rv, &oldrv);
    rv.x = 0; rv.y = cv->olde.y; rv.height = 1;
    GDrawDrawPixmap(pixmap, cv->vruler, &rv, 0, cv->rulerh + rstart + cv->olde.y);
    GDrawDrawLine(pixmap,0,cv->e.y+rstart+cv->rulerh,cv->rulerh,cv->e.y+rstart+cv->rulerh,rulercurtickcol);
    GDrawPopClip(pixmap, &oldrv);

    cv->olde = cv->e;
}

void CVInfoDraw(CharView *cv, GWindow pixmap ) {
    GRect r;
    r.x = 0;
    r.y = cv->mbh+cv->charselectorh;
    r.width = cv->width;
    r.height = cv->infoh-1;
    GDrawRequestExpose(pixmap, &r, false);
    if (cv->showrulers) {
        int rstart = cv->mbh+cv->charselectorh+cv->infoh;
        r.x = cv->rulerh;
        r.y = rstart;
        r.height = cv->rulerh;
        r.width = cv->width;
        GDrawRequestExpose(pixmap, &r, false);

        r.x = 0;
        r.y = rstart + cv->rulerh;
        r.width = cv->rulerh;
        r.height = cv->height;
        GDrawRequestExpose(pixmap, &r, false);
    }
}

static void CVCrossing(CharView *cv, GEvent *event ) {
    CharViewTab* tab = CVGetActiveTab(cv);
    CVToolsSetCursor(cv,event->u.mouse.state,event->u.mouse.device);
    cv->info_within = event->u.crossing.entered;
    cv->info.x = (event->u.crossing.x-tab->xoff)/tab->scale;
    cv->info.y = (cv->height-event->u.crossing.y-tab->yoff)/tab->scale;
    CVInfoDraw(cv,cv->gw);
    CPEndInfo(cv);
}

static int CheckSpiroPoint(FindSel *fs, spiro_cp *cp, SplineSet *spl,int index) {

    if ( fs->xl<=cp->x && fs->xh>=cp->x &&
	    fs->yl<=cp->y && fs->yh >= cp->y ) {
	fs->p->spiro = cp;
	fs->p->spline = NULL;
	fs->p->anysel = true;
	fs->p->spl = spl;
	fs->p->spiro_index = index;
return( true );
    }
return( false );
}

static int CheckPoint(FindSel *fs, SplinePoint *sp, SplineSet *spl) {

    if ( fs->xl<=sp->me.x && fs->xh>=sp->me.x &&
	    fs->yl<=sp->me.y && fs->yh >= sp->me.y ) {
	fs->p->sp = sp;
	fs->p->spline = NULL;
	fs->p->anysel = true;
	fs->p->spl = spl;
	if ( !fs->seek_controls )
return( true );
    }
    if ( (sp->selected && fs->select_controls)
	 || fs->all_controls
	 || fs->alwaysshowcontrolpoints )
    {
	int seln=false, selp=false;
	if ( fs->c_xl<=sp->nextcp.x && fs->c_xh>=sp->nextcp.x &&
		fs->c_yl<=sp->nextcp.y && fs->c_yh >= sp->nextcp.y )
	    seln = true;
	if ( fs->c_xl<=sp->prevcp.x && fs->c_xh>=sp->prevcp.x &&
		fs->c_yl<=sp->prevcp.y && fs->c_yh >= sp->prevcp.y )
	    selp = true;
	if ( seln && selp ) {
	    /* Select the one with a spline attached. */
	    if ( sp->prev!=NULL && sp->next==NULL )
		seln = false;
	}
	if ( seln ) {
	    fs->p->sp = sp;
	    fs->p->spline = NULL;
	    fs->p->spl = spl;
	    fs->p->nextcp = true;
	    fs->p->anysel = true;
	    fs->p->cp = sp->nextcp;
	    if ( sp->nonextcp && (sp->pointtype==pt_curve || sp->pointtype==pt_hvcurve)) {
		fs->p->cp.x = sp->me.x + (sp->me.x-sp->prevcp.x);
		fs->p->cp.y = sp->me.y + (sp->me.y-sp->prevcp.y);
	    }
	    sp->selected = true;
	    sp->nextcpselected = true;
return( true );
	} else if ( selp ) {
	    fs->p->sp = sp;
	    fs->p->spline = NULL;
	    fs->p->spl = spl;
	    fs->p->prevcp = true;
	    fs->p->anysel = true;
	    fs->p->cp = sp->prevcp;
	    if ( sp->noprevcp && (sp->pointtype==pt_curve || sp->pointtype==pt_hvcurve)) {
		fs->p->cp.x = sp->me.x + (sp->me.x-sp->nextcp.x);
		fs->p->cp.y = sp->me.y + (sp->me.y-sp->nextcp.y);
	    }
	    sp->selected = true;
	    sp->prevcpselected = true;
return( true );
	}
    }
return( false );
}

static int CheckSpline(FindSel *fs, Spline *spline, SplineSet *spl) {

    /* Anything else is better than a spline */
    if ( fs->p->anysel )
return( false );

    if ( NearSpline(fs,spline)) {
	fs->p->spline = spline;
	fs->p->spl = spl;
	fs->p->anysel = true;
	fs->p->spiro_index = SplineT2SpiroIndex(spline,fs->p->t,spl);
return( false /*true*/ );	/* Check if there's a point where we are first */
	/* if there is use it, if not (because anysel is true) we'll fall back */
	/* here */
    }

return( false );
}

static int InImage( FindSel *fs, ImageList *img) {
    int x,y;

    x = floor((fs->p->cx-img->xoff)/img->xscale);
    y = floor((img->yoff-fs->p->cy)/img->yscale);
    if ( x<0 || y<0 || x>=GImageGetWidth(img->image) || y>=GImageGetHeight(img->image))
return ( false );
    if ( GImageGetPixelRGBA(img->image,x,y)<0x80000000 )	/* Transparent(ish) */
return( false );

return( true );
}

static int InSplineSet( FindSel *fs, SplinePointList *set,int inspiro) {
    SplinePointList *spl;
    Spline *spline, *first;
    int i;

    for ( spl = set; spl!=NULL; spl = spl->next ) {
	if ( inspiro ) {
	    for ( i=0; i<spl->spiro_cnt-1; ++i )
		if ( CheckSpiroPoint(fs,&spl->spiros[i],spl,i))
return( true );
	    first = NULL;
	    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		if ( CheckSpline(fs,spline,spl), fs->p->anysel )
return( true );
		if ( first==NULL ) first = spline;
	    }
	} else {
	    if ( CheckPoint(fs,spl->first,spl) && ( !fs->seek_controls || fs->p->nextcp || fs->p->prevcp )) {
return( true );
	    }
	    first = NULL;
	    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		if ( (CheckPoint(fs,spline->to,spl) && ( !fs->seek_controls || fs->p->nextcp || fs->p->prevcp )) ||
			( CheckSpline(fs,spline,spl) && !fs->seek_controls )) {
return( true );
		}
		if ( first==NULL ) first = spline;
	    }
	}
    }
return( fs->p->anysel );
}

static int NearSplineSetPoints( FindSel *fs, SplinePointList *set,int inspiro) {
    SplinePointList *spl;
    Spline *spline, *first;
    int i;

    for ( spl = set; spl!=NULL; spl = spl->next ) {
	if ( inspiro ) {
	    for ( i=0; i<spl->spiro_cnt; ++i )
		if ( CheckSpiroPoint(fs,&spl->spiros[i],spl,i))
return( true );
	} else {
	    if ( CheckPoint(fs,spl->first,spl)) {
return( true );
	    }
	    first = NULL;
	    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		if ( CheckPoint(fs,spline->to,spl) ) {
return( true );
		}
		if ( first==NULL ) first = spline;
	    }
	}
    }
return( fs->p->anysel );
}

static int16_t MouseToCX( CharView *cv, int16_t mx )
{
    CharViewTab* tab = CVGetActiveTab(cv);
    return( mx - tab->xoff ) / tab->scale;
}

    
static void SetFS( FindSel *fs, PressedOn *p, CharView *cv, GEvent *event) {
    CharViewTab* tab = CVGetActiveTab(cv);
    extern int snaptoint;

    memset(p,'\0',sizeof(PressedOn));
    p->pressed = true;

    memset(fs,'\0',sizeof(*fs));
    fs->p = p;
    fs->e = event;
    p->x = event->u.mouse.x;
    p->y = event->u.mouse.y;
    p->cx = (event->u.mouse.x-tab->xoff)/tab->scale;
    p->cy = (cv->height-event->u.mouse.y-tab->yoff)/tab->scale;

    fs->fudge = (cv->active_tool==cvt_ruler ? snapdistancemeasuretool : snapdistance)/tab->scale;

    /* If they have really large control points then expand
     * the selection range to allow them to still click on the
     * very edge of the control point to select it.
     */
    if( prefs_cvEditHandleSize > prefs_cvEditHandleSize_default )
    {
	float delta = (prefs_cvEditHandleSize - prefs_cvEditHandleSize_default) / tab->scale;
	delta *= 1.5;
	fs->fudge += delta;
    }

    fs->c_xl = fs->xl = p->cx - fs->fudge;
    fs->c_xh = fs->xh = p->cx + fs->fudge;
    fs->c_yl = fs->yl = p->cy - fs->fudge;
    fs->c_yh = fs->yh = p->cy + fs->fudge;
    if ( snaptoint ) {
	p->cx = rint(p->cx);
	p->cy = rint(p->cy);
	if ( fs->xl>p->cx - fs->fudge )
	    fs->xl = p->cx - fs->fudge;
	if ( fs->xh < p->cx + fs->fudge )
	    fs->xh = p->cx + fs->fudge;
	if ( fs->yl>p->cy - fs->fudge )
	    fs->yl = p->cy - fs->fudge;
	if ( fs->yh < p->cy + fs->fudge )
	    fs->yh = p->cy + fs->fudge;
    }
}

int CVMouseAtSpline(CharView *cv,GEvent *event) {
    FindSel fs;
    int pressed = cv->p.pressed;

    SetFS(&fs,&cv->p,cv,event);
    cv->p.pressed = pressed;
return( InSplineSet(&fs,cv->b.layerheads[cv->b.drawmode]->splines,cv->b.sc->inspiro && hasspiro()));
}

static GEvent *CVConstrainedMouseDown(CharView *cv,GEvent *event, GEvent *fake) {
    CharViewTab* tab = CVGetActiveTab(cv);
    SplinePoint *base;
    spiro_cp *basecp;
    int basex, basey, dx, dy;
    double basetruex, basetruey;
    int sign;

    if ( !CVAnySelPoint(cv,&base,&basecp))
return( event );

    if ( base!=NULL ) {
	basetruex = base->me.x;
	basetruey = base->me.y;
    } else {
	basetruex = basecp->x;
	basetruey = basecp->y;
    }
    basex =  tab->xoff + rint(basetruex*tab->scale);
    basey = -tab->yoff + cv->height - rint(basetruey*tab->scale);

    dx= event->u.mouse.x-basex, dy = event->u.mouse.y-basey;
    sign = dx*dy<0?-1:1;

    fake->u.mouse = event->u.mouse;
    if ( dx<0 ) dx = -dx; if ( dy<0 ) dy = -dy;
    if ( dy >= 2*dx ) {
	cv->p.x = fake->u.mouse.x = basex;
	cv->p.cx = basetruex ;
	if ( !(event->u.mouse.state&ksm_meta) &&
		ItalicConstrained && cv->b.sc->parent->italicangle!=0 ) {
	    double off = tan(cv->b.sc->parent->italicangle*FF_PI/180)*
		    (cv->p.cy-basetruey);
	    double aoff = off<0 ? -off : off;
	    if ( dx>=aoff*tab->scale/2 && (event->u.mouse.x-basex<0)!=(off<0) ) {
		cv->p.cx -= off;
		cv->p.x = fake->u.mouse.x = tab->xoff + rint(cv->p.cx*tab->scale);
	    }
	}
    } else if ( dx >= 2*dy ) {
	fake->u.mouse.y = basey;
	cv->p.cy = basetruey;
    } else if ( dx > dy ) {
	fake->u.mouse.x = basex + sign * (event->u.mouse.y-cv->p.y);
	cv->p.cx = basetruex - sign * (cv->p.cy-basetruey);
    } else {
	fake->u.mouse.y = basey + sign * (event->u.mouse.x-cv->p.x);
	cv->p.cy = basetruey - sign * (cv->p.cx-basetruex);
    }

return( fake );
}

static void CVSetConstrainPoint(CharView *cv, GEvent *event) {
    SplineSet *sel;

    if ( (sel = CVAnySelPointList(cv))!=NULL ) {
	if ( sel->first->selected ) cv->p.constrain = sel->first->me;
	else cv->p.constrain = sel->last->me;
    } else if ( cv->p.sp!=NULL ) {
	cv->p.constrain = cv->p.sp->me;
    } else {
	cv->p.constrain.x = cv->info.x;
	cv->p.constrain.y = cv->info.y;
    }
}

static void CVDoSnaps(CharView *cv, FindSel *fs) {
    PressedOn *p = fs->p;

    if ( cv->b.drawmode!=dm_grid && cv->b.layerheads[dm_grid]->splines!=NULL ) {
	PressedOn temp;
	int oldseek = fs->seek_controls;
	temp = *p;
	fs->p = &temp;
	fs->seek_controls = false;
	if ( InSplineSet( fs, cv->b.layerheads[dm_grid]->splines,cv->b.sc->inspiro && hasspiro())) {
	    if ( temp.spline!=NULL ) {
		p->cx = ((temp.spline->splines[0].a*temp.t+
			    temp.spline->splines[0].b)*temp.t+
			    temp.spline->splines[0].c)*temp.t+
			    temp.spline->splines[0].d;
		p->cy = ((temp.spline->splines[1].a*temp.t+
			    temp.spline->splines[1].b)*temp.t+
			    temp.spline->splines[1].c)*temp.t+
			    temp.spline->splines[1].d;
	    } else if ( temp.sp!=NULL ) {
		p->cx = temp.sp->me.x;
		p->cy = temp.sp->me.y;
	    }
	}
	fs->p = p;
	fs->seek_controls = oldseek;
    }
    if ( p->cx>-fs->fudge && p->cx<fs->fudge )
	p->cx = 0;
    else if ( p->cx>cv->b.sc->width-fs->fudge && p->cx<cv->b.sc->width+fs->fudge &&
	    !cv->widthsel)
	p->cx = cv->b.sc->width;
    else if ( cv->widthsel && p!=&cv->p &&
	    p->cx>cv->oldwidth-fs->fudge && p->cx<cv->oldwidth+fs->fudge )
	p->cx = cv->oldwidth;
    if ( p->cy>-fs->fudge && p->cy<fs->fudge )
	p->cy = 0;
}

static int _CVTestSelectFromEvent(CharView *cv,FindSel *fs) {
    PressedOn temp;
    ImageList *img;
    int found;

    found = InSplineSet(fs,cv->b.layerheads[cv->b.drawmode]->splines,cv->b.sc->inspiro && hasspiro());

    if ( !found ) {
	RefChar *rf;
	temp = cv->p;
	fs->p = &temp;
	fs->seek_controls = false;
	for ( rf=cv->b.layerheads[cv->b.drawmode]->refs; rf!=NULL; rf = rf->next ) {
	    if ( InSplineSet(fs,rf->layers[0].splines,cv->b.sc->inspiro && hasspiro())) {
		cv->p.ref = rf;
		cv->p.anysel = true;
	break;
	    }
	}
	if ( cv->b.drawmode==dm_fore && ( cv->showanchor && !cv->p.anysel )) {
	    AnchorPoint *ap, *found=NULL;
	    /* I do this pecular search because: */
	    /*	1) I expect there to be lots of times we get multiple */
	    /*     anchors at the same location */
	    /*  2) The anchor points are drawn so that the bottommost */
	    /*	   is displayed */
	    for ( ap=cv->b.sc->anchor; ap!=NULL; ap=ap->next )
		if ( fs->xl<=ap->me.x && fs->xh>=ap->me.x &&
			fs->yl<=ap->me.y && fs->yh >= ap->me.y )
		    found = ap;
	    if ( found!=NULL ) {
		cv->p.ap = found;
		cv->p.anysel = true;
	    }
	}
	for ( img = cv->b.layerheads[cv->b.drawmode]->images; img!=NULL; img=img->next ) {
	    if ( InImage(fs,img)) {
		cv->p.img = img;
		cv->p.anysel = true;
	break;
	    }
	}
    }
return( cv->p.anysel );
}

int CVTestSelectFromEvent(CharView *cv,GEvent *event) {
    FindSel fs;

    SetFS(&fs,&cv->p,cv,event);
return( _CVTestSelectFromEvent(cv,&fs));
}

/**
 * A cache for the selected spline point or spiro control point
 */
typedef struct lastselectedpoint
{
    SplinePoint *lastselpt;
    spiro_cp *lastselcp;
} lastSelectedPoint;

static void CVMaybeCreateDraggingComparisonOutline( CharView* cv )
{
    if (!prefs_create_dragging_comparison_outline)
        return;
    if (!cv)
        return;
    if (cv->p.pretransform_spl) {
        SplinePointListFree(cv->p.pretransform_spl);
        cv->p.pretransform_spl = NULL;
    }

    Layer* l = cv->b.layerheads[cv->b.drawmode];
    if (!l || !l->splines)
        return;

    if (cv->p.anysel) {
        SplinePointList *cur = NULL;
        for (SplinePointList *spl = l->splines; spl; spl = spl->next) {
            if (SplinePointListCheckSelected1(spl, cv->b.sc->inspiro && hasspiro(), NULL, false)) {
                if (cur == NULL) {
                    cv->p.pretransform_spl = cur = SplinePointListCopy1(spl);
                } else {
                    cur->next = SplinePointListCopy1(spl);
                    cur = cur->next;
                }
            }
        }
    }
}

static void CVSwitchActiveSC( CharView *cv, SplineChar* sc, int idx )
{
    CharViewTab* tab = CVGetActiveTab(cv);
    int i=0;
    FontViewBase *fv = cv->b.fv;
    char buf[300];

    TRACE("CVSwitchActiveSC() idx:%d active:%d\n", idx, cv->additionalCharsToShowActiveIndex );
    if( !sc )
    {
	sc = cv->additionalCharsToShow[i];
	if( !sc )
	    return;
    }
    else
    {
	// test for setting something twice
	if( !idx && cv->additionalCharsToShowActiveIndex == idx )
	{
	    if( cv->additionalCharsToShow[i] == sc )
		return;
	}
    }
    
    cv->changedActiveGlyph = 1;
    TRACE("CVSwitchActiveSC(b) activeidx:%d newidx:%d\n", cv->additionalCharsToShowActiveIndex, idx );
    for( i=0; i < additionalCharsToShowLimit; i++ )
	if( cv->additionalCharsToShow[i] )
	    TRACE("CVSwitchActiveSC(b) toshow.. i:%d char:%s\n", i, cv->additionalCharsToShow[i]
		   ? cv->additionalCharsToShow[i]->name : "N/A" );


    //
    // Work out how far we should scroll the panel to keep
    // the display from jumping around. First look right then
    // check left.
    int scroll_offset = 0;
    if( idx > cv->additionalCharsToShowActiveIndex )
    {
	SplineChar* xc = 0;
	int i = 0;
	scroll_offset = cv->b.sc->width;
	int ridx = cv->additionalCharsToShowActiveIndex+1;
	for( i=ridx; i < idx; i++ )
	{
	    if((xc = cv->additionalCharsToShow[i]))
		scroll_offset += xc->width;
	}
    }
    if( idx < cv->additionalCharsToShowActiveIndex )
    {
	SplineChar* xc = 0;
	int i = 0;
	scroll_offset = 0;
	int ridx = cv->additionalCharsToShowActiveIndex-1;
	for( i=ridx; i >= idx; i-- )
	{
	    if((xc = cv->additionalCharsToShow[i]))
		scroll_offset -= xc->width;
	}
    }
    
    


    
    CVUnlinkView( cv );
    cv->b.sc = sc;
    cv->b.next = sc->views;
    cv->b.layerheads[dm_fore] = &sc->layers[ly_fore];
    cv->b.layerheads[dm_back] = &sc->layers[ly_back];
    cv->b.layerheads[dm_grid] = &fv->sf->grid;
    if ( !sc->parent->multilayer && fv->active_layer!=ly_fore ) {
	cv->b.layerheads[dm_back] = &sc->layers[fv->active_layer];
	cv->b.drawmode = dm_back;
    }
    CharIcon(cv,(FontView *) (cv->b.fv));
    char* title = CVMakeTitles(cv,buf,sizeof(buf));
    GDrawSetWindowTitles8(cv->gw,buf,title);
    CVInfoDraw(cv,cv->gw);
    free(title);
    _CVPaletteActivate(cv,true,false);
    
    TRACE("CVSwitchActiveSC() idx:%d\n", idx );

    cv->additionalCharsToShowActiveIndex = idx;

    // update the select[i]on in the input text to reflect
    // the users currently selected char.
    {
	SplineFont* sf = cv->b.sc->parent;
	EncMap *map = ((FontView *) (cv->b.fv))->b.map;
	unichar_t *srctxt = GGadgetGetTitle( cv->charselector );
	int endsWithSlash = u_endswith( srctxt, c_to_u("/"));

	unichar_t* p = 0;
	p = Wordlist_selectionClear( sf, map, srctxt );	
	p = Wordlist_selectionAdd(   sf, map, p, idx );
	if( endsWithSlash )
	    uc_strcat( p, "/" );

	// only update when the selection has changed.
	// updating this string is a non reversible operation if the
	// user is part way through typing some text.
	if( !Wordlist_selectionsEqual( srctxt, p ))
	{
	    GGadgetSetTitle( cv->charselector, p );
	}
    }
    
    
    cv->b.next = sc->views;
    sc->views = &cv->b;

    // Move the scrollbar so that it appears the selection
    // box has moved rather than all the characters.
    if( scroll_offset )
	CVHScrollSetPos( cv, tab->xoff + scroll_offset * tab->scale );
    
//    if ( CVClearSel(cv))
//	SCUpdateAll(cv->b.sc);
    
}

static void CVMouseDown(CharView *cv, GEvent *event ) {
    FindSel fs;
    GEvent fake;
    lastSelectedPoint lastSel;
    memset( &lastSel, 0, sizeof(lastSelectedPoint));

    if ( event->u.mouse.button==2 && event->u.mouse.device!=NULL &&
	    strcmp(event->u.mouse.device,"stylus")==0 )
return;		/* I treat this more like a modifier key change than a button press */

    if ( cv->expandedge != ee_none )
	GDrawSetCursor(cv->v,ct_mypointer);
    if ( event->u.mouse.button==3 )
    {
	/* context menu */
	CVToolsPopup(cv,event);
	return;
    }

    TRACE("tool:%d pointer:%d ctl:%d alt:%d\n",
	   cv->showing_tool,
	   (cv->showing_tool == cvt_pointer),
	   cv->activeModifierControl, cv->activeModifierAlt );

    int8_t override_showing_tool = cvt_none;
    int8_t old_showing_tool = cv->showing_tool;
    if( cv->showing_tool == cvt_pointer
	&& cv->activeModifierControl
	&& cv->activeModifierAlt )
    {
	FindSel fs;
	SetFS(&fs,&cv->p,cv,event);
	int found = InSplineSet( &fs, cv->b.layerheads[cv->b.drawmode]->splines,
				 cv->b.sc->inspiro && hasspiro());
	TRACE("in spline set:%d cv->p.sp:%p\n", found, cv->p.sp );

	//
	// Only overwrite to create a point if the user has clicked a spline.
	//
	if( found && !cv->p.sp )
	    override_showing_tool = cvt_curve;
    }
    
    if( cv->charselector && cv->charselector == GWindowGetFocusGadgetOfWindow(cv->gw))
	GWindowClearFocusGadgetOfWindow(cv->gw);

    update_spacebar_hand_tool(cv);

    CVToolsSetCursor(cv,event->u.mouse.state|(1<<(7+event->u.mouse.button)), event->u.mouse.device );
    if( override_showing_tool != cvt_none )
	cv->showing_tool = override_showing_tool;
    cv->active_tool = cv->showing_tool;
    cv->needsrasterize = false;
    cv->recentchange = false;

    
    SetFS(&fs,&cv->p,cv,event);
    if ( event->u.mouse.state&ksm_shift )
	event = CVConstrainedMouseDown(cv,event,&fake);

    if ( cv->active_tool == cvt_pointer ) {
	fs.select_controls = true;
	if ( event->u.mouse.state&ksm_meta ) {
	    fs.seek_controls = true;
	    /* Allow more slop looking for control points if they asked for them */
	    fs.c_xl -= fs.fudge; fs.c_xh += fs.fudge;
	    fs.c_yl -= fs.fudge; fs.c_yh += fs.fudge;
	}
	if ( cv->showpointnumbers && cv->b.layerheads[cv->b.drawmode]->order2 )
	    fs.all_controls = true;
	fs.alwaysshowcontrolpoints = cv->alwaysshowcontrolpoints;
	lastSel.lastselpt = cv->lastselpt;
	lastSel.lastselcp = cv->lastselcp;
	cv->lastselpt = NULL;
	cv->lastselcp = NULL;
	_CVTestSelectFromEvent(cv,&fs);
	fs.p = &cv->p;

//	TRACE("cvmousedown tab->xoff:%d\n", tab->xoff );
//	TRACE("cvmousedown x:%d y:%d\n",   event->u.mouse.x, event->u.mouse.y );
	    
	if( !cv->p.anysel && cv->b.drawmode != dm_grid )
	{
	    // If we are in left-right arrow cursor mode to move
	    // those bearings then don't even think about changing
	    // the char right now.
	    if( !CVNearRBearingLine( cv, cv->p.cx, fs.fudge )
		&& !CVNearLBearingLine( cv, cv->p.cx, fs.fudge ))
	    {
		int i=0;
		FindSel fsadjusted = fs;
		fsadjusted.c_xl -= 2*fsadjusted.fudge;
		fsadjusted.c_xh += 2*fsadjusted.fudge;
		fsadjusted.xl -= 2*fsadjusted.fudge;
		fsadjusted.xh += 2*fsadjusted.fudge;
		SplineChar* xc = 0;
		int xcidx = -1;
		int borderFudge = 20;
	    
		{
		    int offset = cv->b.sc->width;
		    int cumulativeLeftSideBearing = 0;
//	        TRACE("first offset:%d original cx:%f \n", offset, fsadjusted.p->cx );
		    int ridx = cv->additionalCharsToShowActiveIndex+1;
		    for( i=ridx; i < additionalCharsToShowLimit; i++ )
		    {
			if( i == cv->additionalCharsToShowActiveIndex )
			    continue;
			xc = cv->additionalCharsToShow[i];
			if( !xc )
			    break;
			int OffsetForDoingCharNextToActive = 0;
			if( i == ridx )
			{
			    OffsetForDoingCharNextToActive = borderFudge;
			}
		    

			cumulativeLeftSideBearing += offset;
			/* TRACE("1 adj. x:%f %f\n",fsadjusted.xl,fsadjusted.xh); */
			/* TRACE("1 adj.cx:%f %f\n",fsadjusted.c_xl,fsadjusted.c_xh); */
			/* TRACE("1 p.  cx:%f %f\n",fsadjusted.p->cx,fsadjusted.p->cy ); */
			/* fsadjusted.c_xl -= offset; */
			/* fsadjusted.c_xh -= offset; */
			/* fsadjusted.xl   -= offset; */
			/* fsadjusted.xh   -= offset; */
			/* TRACE("2 adj. x:%f %f\n",fsadjusted.xl,fsadjusted.xh); */
			/* TRACE("2 adj.cx:%f %f\n",fsadjusted.c_xl,fsadjusted.c_xh); */
			/* TRACE("2 p.  cx:%f %f\n",fsadjusted.p->cx,fsadjusted.p->cy ); */
			/* int found = InSplineSet( &fsadjusted, */
			/* 			     xc->layers[cv->b.drawmode-1].splines, */
			/* 			     xc->inspiro && hasspiro()); */
			TRACE("A:%d\n", cumulativeLeftSideBearing );
			TRACE("B:%f\n", fsadjusted.p->cx );
			TRACE("C:%d\n", cumulativeLeftSideBearing+xc->width );
			int found = IS_IN_ORDER3(
			    cumulativeLeftSideBearing + OffsetForDoingCharNextToActive,
			    fsadjusted.p->cx,
			    cumulativeLeftSideBearing+xc->width );
			TRACE("CVMOUSEDOWN i:%d found:%d\n", i, found );
			if( found )
			{
			    TRACE("FOUND FOUND FOUND FOUND FOUND FOUND FOUND \n");
			
			    xcidx = i;
//		    CVChangeSC(cv,xc);
			    break;
			}

			offset = xc->width;
		    }
		}

		fsadjusted = fs;
		fsadjusted.c_xl -= 2*fsadjusted.fudge;
		fsadjusted.c_xh += 2*fsadjusted.fudge;
		fsadjusted.xl -= 2*fsadjusted.fudge;
		fsadjusted.xh += 2*fsadjusted.fudge;

		if( !xc && cv->additionalCharsToShowActiveIndex > 0 )
		{
		    xc = cv->additionalCharsToShow[cv->additionalCharsToShowActiveIndex-1];
		    int cumulativeLeftSideBearing = 0;
//	        TRACE("first offset:%d original cx:%f \n", offset, fsadjusted.p->cx );
		    int lidx = cv->additionalCharsToShowActiveIndex-1;
		    for( i=lidx; i>=0; i-- )
		    {
			if( i == cv->additionalCharsToShowActiveIndex )
			    continue;
			xc = cv->additionalCharsToShow[i];
			if( !xc )
			    break;
			cumulativeLeftSideBearing -= xc->width;
			int OffsetForDoingCharNextToActive = 0;
			if( i == lidx )
			{
			    OffsetForDoingCharNextToActive = borderFudge;
			}

			/* TRACE("1 adj. x:%f %f\n",fsadjusted.xl,fsadjusted.xh); */
			/* TRACE("1 adj.cx:%f %f\n",fsadjusted.c_xl,fsadjusted.c_xh); */
			/* TRACE("1 p.  cx:%f %f\n",fsadjusted.p->cx,fsadjusted.p->cy ); */
			/* fsadjusted.c_xl += offset; */
			/* fsadjusted.c_xh += offset; */
			/* fsadjusted.xl   += offset; */
			/* fsadjusted.xh   += offset; */
			/* TRACE("2 adj. x:%f %f\n",fsadjusted.xl,fsadjusted.xh); */
			/* TRACE("2 adj.cx:%f %f\n",fsadjusted.c_xl,fsadjusted.c_xh); */
			/* TRACE("2 p.  cx:%f %f\n",fsadjusted.p->cx,fsadjusted.p->cy ); */
			/* int found = InSplineSet( &fsadjusted, */
			/* 			     xc->layers[cv->b.drawmode-1].splines, */
			/* 			     xc->inspiro && hasspiro()); */
			TRACE("A:%d\n", cumulativeLeftSideBearing );
			TRACE("B:%f\n", fsadjusted.p->cx );
			TRACE("C:%d\n", cumulativeLeftSideBearing+xc->width );
			int found = IS_IN_ORDER3(
			    cumulativeLeftSideBearing,
			    fsadjusted.p->cx,
			    cumulativeLeftSideBearing + xc->width - OffsetForDoingCharNextToActive );
		    
			TRACE("cvmousedown i:%d found:%d\n", i, found );
			if( found )
			{
			    TRACE("FOUND FOUND FOUND FOUND FOUND FOUND FOUND i:%d\n", i);
			    xcidx = i;
			    break;
			}

		    }
		}

		TRACE("have xc:%p xcidx:%d\n", xc, xcidx );
		TRACE("    idx:%d active:%d\n", xcidx, cv->additionalCharsToShowActiveIndex );
		if( xc && xcidx >= 0 )
		{
		    CVSwitchActiveSC( cv, xc, xcidx );
		    GDrawRequestExpose(cv->v,NULL,false);
		    return;
		}
	    }
	}
	    
	    

    } else if ( cv->active_tool == cvt_curve || cv->active_tool == cvt_corner ||
	    cv->active_tool == cvt_tangent || cv->active_tool == cvt_hvcurve ||
	    cv->active_tool == cvt_pen || cv->active_tool == cvt_ruler )
    {
	/* Snap to points and splines */
	InSplineSet(&fs,cv->b.layerheads[cv->b.drawmode]->splines,cv->b.sc->inspiro && hasspiro());
	if ( fs.p->sp==NULL && fs.p->spline==NULL )
	    CVDoSnaps(cv,&fs);
    } else {
	/* Just snap to points */
	NearSplineSetPoints(&fs,cv->b.layerheads[cv->b.drawmode]->splines,cv->b.sc->inspiro && hasspiro());
	if ( fs.p->sp==NULL && fs.p->spline==NULL )
	    CVDoSnaps(cv,&fs);
    }

    cv->e.x = event->u.mouse.x; cv->e.y = event->u.mouse.y;
    if ( cv->p.sp!=NULL ) {
	BasePoint *p;
	if ( cv->p.nextcp )
	    p = &cv->p.sp->nextcp;
	else if ( cv->p.prevcp )
	    p = &cv->p.sp->prevcp;
	else
	    p = &cv->p.sp->me;
	cv->info.x = p->x;
	cv->info.y = p->y;
	cv->p.cx = p->x; cv->p.cy = p->y;
    } else if ( cv->p.spiro!=NULL ) {
	cv->info.x = cv->p.spiro->x;
	cv->info.y = cv->p.spiro->y;
	cv->p.cx = cv->p.spiro->x; cv->p.cy = cv->p.spiro->y;
    } else {
	cv->info.x = cv->p.cx;
	cv->info.y = cv->p.cy;
    }
    cv->info_within = true;
    CVInfoDraw(cv,cv->gw);
    CVSetConstrainPoint(cv,event);

    switch ( cv->active_tool ) {
      case cvt_pointer:
	CVMouseDownPointer(cv, &fs, event);
	CVMaybeCreateDraggingComparisonOutline( cv );
	cv->lastselpt = fs.p->sp;
	cv->lastselcp = fs.p->spiro;
      break;
      case cvt_magnify: case cvt_minify:
          //When scroll zooming, the old showing tool is the normal pointer.
          old_showing_tool = cv->active_tool;    
      break;
      case cvt_hand:
	CVMouseDownHand(cv);
      break;
      case cvt_freehand:
	CVMouseDownFreeHand(cv,event);
      break;
      case cvt_curve: case cvt_corner: case cvt_tangent: case cvt_pen:
      case cvt_hvcurve:
	CVMouseDownPoint(cv,event);
      break;
      case cvt_ruler:
	CVMouseDownRuler(cv,event);
      break;
      case cvt_rotate: case cvt_flip: case cvt_scale: case cvt_skew:
      case cvt_3d_rotate: case cvt_perspective:
	CVMouseDownTransform(cv);
      break;
      case cvt_knife:
	CVMouseDownKnife(cv);
      break;
      case cvt_rect: case cvt_elipse: case cvt_poly: case cvt_star:
	CVMouseDownShape(cv,event);
      break;
    }
    cv->showing_tool = old_showing_tool;
}

static void _SCHintsChanged(SplineChar *sc) {
    struct splinecharlist *dlist;

    if ( !sc->changedsincelasthinted ) {
	sc->changedsincelasthinted = true;
	FVMarkHintsOutOfDate(sc);
    }

    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next )
	_SCHintsChanged(dlist->sc);
}

static void SC_HintsChanged(SplineChar *sc) {
    struct splinecharlist *dlist;
    int was = sc->changedsincelasthinted;

    if ( sc->parent->onlybitmaps || sc->parent->multilayer || sc->parent->strokedfont )
return;
    sc->changedsincelasthinted = false;		/* We just applied a hinting change */
    if ( !sc->changed ) {
	sc->changed = true;
	FVToggleCharChanged(sc);
	SCRefreshTitles(sc);
	if ( !sc->parent->changed ) {
	    sc->parent->changed = true;
	    FVSetTitles(sc->parent);
	}
    }
    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next )
	_SCHintsChanged(dlist->sc);
    if ( was ) {
	FontView *fvs;
	for ( fvs = (FontView *) (sc->parent->fv); fvs!=NULL; fvs=(FontView *) (fvs->b.nextsame) )
	    GDrawRequestExpose(fvs->v,NULL,false);
    }
}

void CVSetCharChanged(CharView *cv,int changed) {
    SplineFont *sf = cv->b.fv->sf;
    SplineChar *sc = cv->b.sc;
    int oldchanged = sf->changed;
    /* A changed argument of 2 means the outline didn't change, but something */
    /*  else (width, anchorpoint) did */
    int cvlayer = CVLayer((CharViewBase *) cv);

    if ( changed )
	SFSetModTime(sf);
    if ( cv->b.drawmode==dm_grid ) {
	if ( changed ) {
	    sf->changed = true;
	    if ( sf->cidmaster!=NULL )
		sf->cidmaster->changed = true;
	}
    } else {
	if ( cv->b.drawmode==dm_fore && changed==1 ) {
	    sf->onlybitmaps = false;
	}
	SCTickValidationState(cv->b.sc,cvlayer);
	if ( (sc->changed==0) != (changed==0) ) {
	    sc->changed = (changed!=0);
	    FVToggleCharChanged(sc);
	    SCRefreshTitles(sc);
	    if ( changed ) {
		sf->changed = true;
		if ( sf->cidmaster!=NULL )
		    sf->cidmaster->changed = true;
	    }
	}
	if ( changed==1 ) {
	    instrcheck(sc,cvlayer);
	    /*SCDeGridFit(sc);*/
	    if ( sc->parent->onlybitmaps )
		/* Do nothing */;
	    else if ( sc->parent->multilayer || sc->parent->strokedfont || sc->layers[cvlayer].order2 )
		sc->changed_since_search = true;
	    else if ( cv->b.drawmode==dm_fore ) {
		sc->changed_since_search = true;
		_SCHintsChanged(cv->b.sc);
	    }
	    sc->changed_since_autosave = true;
	    sf->changed_since_autosave = true;
	    sf->changed_since_xuidchanged = true;
	    if ( sf->cidmaster!=NULL ) {
		sf->cidmaster->changed_since_autosave = true;
		sf->cidmaster->changed_since_xuidchanged = true;
	    }
	}
	if ( cv->b.drawmode!=dm_grid ) {
	    cv->needsrasterize = true;
	}
    }
    cv->recentchange = true;
    if ( !oldchanged )
	FVSetTitles(sf);
}

void SCClearSelPt(SplineChar *sc) {
    CharView *cv;

    for ( cv=(CharView *) (sc->views); cv!=NULL; cv=(CharView *) (cv->b.next) ) {
	cv->lastselpt = cv->p.sp = NULL;
	cv->p.spiro = cv->lastselcp = NULL;
    }
}

static void _SC_CharChangedUpdate(SplineChar *sc,int layer,int changed) {
    SplineFont *sf = sc->parent;
    extern int updateflex;
    /* layer might be ly_none or ly_all */

    if ( layer>=sc->layer_cnt ) {
	IError( "Bad layer in _SC_CharChangedUpdate");
	layer = ly_fore;
    }
    if ( layer>=0 && !sc->layers[layer].background )
	TTFPointMatches(sc,layer,true);
    if ( changed != -1 ) {
	sc->changed_since_autosave = true;
	SFSetModTime(sf);
	if ( (sc->changed==0) != (changed==0) ) {
	    sc->changed = (changed!=0);
	    if ( changed && layer>=ly_fore && (sc->layers[layer].splines!=NULL || sc->layers[layer].refs!=NULL))
		sc->parent->onlybitmaps = false;
	    FVToggleCharChanged(sc);
	    SCRefreshTitles(sc);
	}
	if ( !sf->changed ) {
	    sf->changed = true;
	    if ( sf->cidmaster )
		sf->cidmaster->changed = true;
	    FVSetTitles(sf);
	}
	if ( changed && layer>=0 && !sc->layers[layer].background && sc->layers[layer].order2 ) {
	    instrcheck(sc,layer);
	    SCReGridFit(sc,layer);
	}
	if ( !sc->parent->onlybitmaps && !sc->parent->multilayer &&
		changed==1 && !sc->parent->strokedfont &&
		layer>=0 &&
		!sc->layers[layer].background && !sc->layers[layer].order2 )
	    _SCHintsChanged(sc);
	sc->changed_since_search = true;
	sf->changed = true;
	sf->changed_since_autosave = true;
	sf->changed_since_xuidchanged = true;
	if ( layer>=0 )
	    SCTickValidationState(sc,layer);
    }
    if ( sf->cidmaster!=NULL )
	sf->cidmaster->changed = sf->cidmaster->changed_since_autosave =
		sf->cidmaster->changed_since_xuidchanged = true;
    SCRegenDependents(sc,ly_all);	/* All chars linked to this one need to get the new splines */
    if ( updateflex && (CharView *) (sc->views)!=NULL && layer>=ly_fore )
	SplineCharIsFlexible(sc,layer);
    SCUpdateAll(sc);
    SCLayersChange(sc);
    SCRegenFills(sc);
}

static void SC_CharChangedUpdate(SplineChar *sc,int layer) {
    _SC_CharChangedUpdate(sc,layer,true);
}

static void _CV_CharChangedUpdate(CharView *cv,int changed) {
    extern int updateflex;
    FontView *fv;
    int cvlayer = CVLayer((CharViewBase *) cv);

    CVSetCharChanged(cv,changed);
    CVLayerChange(cv);
    if ( cv->needsrasterize ) {
	TTFPointMatches(cv->b.sc,cvlayer,true);		/* Must precede regen dependents, as this can change references */
	SCRegenDependents(cv->b.sc,cvlayer);		/* All chars linked to this one need to get the new splines */
	if ( cv->b.layerheads[cv->b.drawmode]->order2 )
	    SCReGridFit(cv->b.sc,cvlayer);
	if ( updateflex && cvlayer!=ly_grid && !cv->b.layerheads[cv->b.drawmode]->background )
	    SplineCharIsFlexible(cv->b.sc,cvlayer);
	SCUpdateAll(cv->b.sc);
	SCRegenFills(cv->b.sc);
	for ( fv = (FontView *) (cv->b.sc->parent->fv); fv!=NULL; fv=(FontView *) (fv->b.nextsame) )
	    FVRegenChar(fv,cv->b.sc);
	cv->needsrasterize = false;
    } else if ( cv->b.drawmode!=dm_grid ) {
	/* If we changed the background then only views of this character */
	/*  need to know about it. No dependents needed, but why write */
	/*  another routine for a rare case... */
	SCUpdateAll(cv->b.sc);
    } else /* if ( cv->b.drawmode==dm_grid )*/ {
	/* If we changed the grid then any character needs to know it */
	FVRedrawAllCharViewsSF(cv->b.sc->parent);
    }
    if ( cv->showpointnumbers || cv->show_ft_results )
	SCNumberPoints(cv->b.sc, cvlayer);
    cv->recentchange = false;
    cv->p.sp = NULL;		/* Might have been deleted */
}

static void CV_CharChangedUpdate(CharView *cv) {
    _CV_CharChangedUpdate(cv,true);
}

static void CVMouseMove(CharView *cv, GEvent *event ) {
    CharViewTab* tab = CVGetActiveTab(cv);
    real cx, cy;
    PressedOn p;
    FindSel fs;
    GEvent fake;
    int stop_motion = false;
    int has_spiro = hasspiro();

		/* Debug wacom !!!! */
 /* TRACE( "dev=%s (%d,%d) 0x%x\n", event->u.mouse.device!=NULL?event->u.mouse.device:"<None>", */
 /*     event->u.mouse.x, event->u.mouse.y, event->u.mouse.state); */

    if ( event->u.mouse.device!=NULL )
	CVToolsSetCursor(cv,event->u.mouse.state,event->u.mouse.device);

    if ( !cv->p.pressed ) {
	CVUpdateInfo(cv, event);
	if ( cv->showing_tool==cvt_pointer ) {
	    CVCheckResizeCursors(cv);
	    if ( cv->dv!=NULL )
		CVDebugPointPopup(cv);
	} else if ( cv->showing_tool == cvt_ruler )
	    CVMouseMoveRuler(cv,event);
return;
    }

    //GDrawRequestExpose(cv->v,NULL,false);	/* TBD, hack to clear ruler */

    SetFS(&fs,&p,cv,event);
    if ( cv->active_tool == cvt_freehand )
	/* freehand does it's own kind of constraining */;
    else if ( (event->u.mouse.state&ksm_shift) && !cv->p.rubberbanding ) {
	/* Constrained */

	fake.u.mouse = event->u.mouse;
	if ( ((event->u.mouse.state&ksm_meta) ||
		    (!cv->cntrldown && (event->u.mouse.state&ksm_control))) &&
		(cv->p.nextcp || cv->p.prevcp)) {
	    real dot = (cv->p.cp.x-cv->p.constrain.x)*(p.cx-cv->p.constrain.x) +
		    (cv->p.cp.y-cv->p.constrain.y)*(p.cy-cv->p.constrain.y);
	    real len = (cv->p.cp.x-cv->p.constrain.x)*(cv->p.cp.x-cv->p.constrain.x)+
		    (cv->p.cp.y-cv->p.constrain.y)*(cv->p.cp.y-cv->p.constrain.y);
	    if ( len!=0 ) {
		dot /= len;
		/* constrain control point to same angle with respect to base point*/
		if ( dot<0 ) dot = 0;
		p.cx = cv->p.constrain.x + dot*(cv->p.cp.x-cv->p.constrain.x);
		p.cy = cv->p.constrain.y + dot*(cv->p.cp.y-cv->p.constrain.y);
		p.x = fake.u.mouse.x = tab->xoff + rint(p.cx*tab->scale);
		p.y = fake.u.mouse.y = -tab->yoff + cv->height - rint(p.cy*tab->scale);
	    }
	} else {
	    /* Constrain mouse to hor/vert/45 from base point */
	    int basex = cv->active_tool!=cvt_hand ? tab->xoff + rint(cv->p.constrain.x*tab->scale) : cv->p.x;
	    int basey = cv->active_tool!=cvt_hand ?-tab->yoff + cv->height - rint(cv->p.constrain.y*tab->scale) : cv->p.y;
	    int dx= event->u.mouse.x-basex, dy = event->u.mouse.y-basey;
	    int sign = dx*dy<0?-1:1;
	    double aspect = 1.0;

	    if ( dx<0 ) dx = -dx; if ( dy<0 ) dy = -dy;
	    if ( cv->p.img!=NULL && cv->p.img->bb.minx!=cv->p.img->bb.maxx )
		aspect = (cv->p.img->bb.maxy - cv->p.img->bb.miny) / (cv->p.img->bb.maxx - cv->p.img->bb.minx);
	    else if ( cv->p.ref!=NULL && cv->p.ref->bb.minx!=cv->p.ref->bb.maxx )
		aspect = (cv->p.ref->bb.maxy - cv->p.ref->bb.miny) / (cv->p.ref->bb.maxx - cv->p.ref->bb.minx);
	    if ( dy >= 2*dx ) {
		p.x = fake.u.mouse.x = basex;
		p.cx = cv->p.constrain.x;
		if ( ItalicConstrained && cv->b.sc->parent->italicangle!=0 ) {
		    double off = tan(cv->b.sc->parent->italicangle*FF_PI/180)*
			    (p.cy-cv->p.constrain.y);
		    double aoff = off<0 ? -off : off;
		    if ( dx>=aoff*tab->scale/2 && (event->u.mouse.x-basex<0)!=(off<0) ) {
			p.cx -= off;
			p.x = fake.u.mouse.x = tab->xoff + rint(p.cx*tab->scale);
		    }
		}
	    } else if ( dx >= 2*dy ) {
		p.y = fake.u.mouse.y = basey;
		p.cy = cv->p.constrain.y;
	    } else if ( dx > dy ) {
		p.x = fake.u.mouse.x = basex + sign * (event->u.mouse.y-basey)/aspect;
		p.cx = cv->p.constrain.x - sign * (p.cy-cv->p.constrain.y)/aspect;
	    } else {
		p.y = fake.u.mouse.y = basey + sign * (event->u.mouse.x-basex)*aspect;
		p.cy = cv->p.constrain.y - sign * (p.cx-cv->p.constrain.x)*aspect;
	    }
	}
	event = &fake;
    }

    /* If we've changed the character (recentchange is true) we want to */
    /*  snap to the original location, otherwise we'll keep snapping to the */
    /*  current point as it moves across the screen (jerkily) */
    if ( cv->active_tool == cvt_hand || cv->active_tool == cvt_freehand )
	/* Don't snap to points */;
    else if ( !cv->joinvalid ||
	    ((!cv->b.sc->inspiro || has_spiro) && !CheckPoint(&fs,&cv->joinpos,NULL)) ||
	    (  cv->b.sc->inspiro && has_spiro  && !CheckSpiroPoint(&fs,&cv->joincp,NULL,0))) {
	SplinePointList *spl;
	spl = cv->b.layerheads[cv->b.drawmode]->splines;
	if ( cv->recentchange && cv->active_tool==cvt_pointer &&
		cv->b.layerheads[cv->b.drawmode]->undoes!=NULL &&
		(cv->b.layerheads[cv->b.drawmode]->undoes->undotype==ut_state ||
		 cv->b.layerheads[cv->b.drawmode]->undoes->undotype==ut_tstate ))
	    spl = cv->b.layerheads[cv->b.drawmode]->undoes->u.state.splines;
	if ( cv->active_tool != cvt_knife && cv->active_tool != cvt_ruler ) {
	    if ( cv->active_tool == cvt_pointer && ( cv->p.nextcp || cv->p.prevcp ))
		fs.select_controls = true;
	    NearSplineSetPoints(&fs,spl,cv->b.sc->inspiro && has_spiro);
	} else
	    InSplineSet(&fs,spl,cv->b.sc->inspiro && has_spiro);
    }
    /* p.sp and cv->p.sp may correspond to different undo states, thus being */
    /* different objects even while describing essentially the same point. */
    /* So compare point coordinates rather than the points themselves */
    if ( (cv->p.nextcp || cv->p.prevcp) && p.nextcp &&
	    p.sp!=NULL && cv->p.sp != NULL &&
	    p.sp->me.x == cv->p.sp->me.x && p.sp->me.y == cv->p.sp->me.y ) {
	/* If either control point selected, then snap to it or its brother */
	/*  when close */
	p.cx = p.sp->nextcp.x;
	p.cy = p.sp->nextcp.y;
    } else if (( cv->p.nextcp || cv->p.prevcp) && p.prevcp &&
	    p.sp!=NULL && cv->p.sp != NULL &&
	    p.sp->me.x == cv->p.sp->me.x && p.sp->me.y == cv->p.sp->me.y ) {
	p.cx = p.sp->prevcp.x;
	p.cy = p.sp->prevcp.y;
    } else if ( p.sp!=NULL && p.sp!=cv->active_sp ) {		/* Snap to points */
	p.cx = p.sp->me.x;
	p.cy = p.sp->me.y;
    } else if ( p.spiro!=NULL && p.spiro!=cv->active_cp ) {
	p.cx = p.spiro->x;
	p.cy = p.spiro->y;
    } else {
	// Require some movement before snapping objects with the pointer tool
	if ( !RealNear(cv->info.x,cv->p.cx) || !RealNear(cv->info.y,cv->p.cy) )
	    CVDoSnaps(cv,&fs);
    }
    cx = (p.cx -cv->p.cx) / tab->scale;
    cy = (p.cy - cv->p.cy) / tab->scale;
    if ( cx<0 ) cx = -cx;
    if ( cy<0 ) cy = -cy;

	/* If they haven't moved far from the start point, then snap to it */
    if ( cx+cy < 4 ) {
	p.x = cv->p.x; p.y = cv->p.y;
    }

    cv->info.x = p.cx; cv->info.y = p.cy;
    cv->info_sp = p.sp;
    cv->info_spline = p.spline;
    cv->info_t = p.t;
    cv->e.x = event->u.mouse.x; cv->e.y = event->u.mouse.y;
    CVInfoDraw(cv,cv->gw);

    switch ( cv->active_tool ) {
      case cvt_pointer:
	stop_motion = CVMouseMovePointer(cv,event);
      break;
      case cvt_magnify: case cvt_minify:
	if ( !cv->p.rubberbanding ) {
	    cv->p.ex = cv->p.cx;
	    cv->p.ey = cv->p.cy;
	}
	cv->p.ex = cv->info.x;
	cv->p.ey = cv->info.y;
	cv->p.rubberbanding = true;
	GDrawRequestExpose(cv->v, NULL, false);
      break;
      case cvt_hand:
	CVMouseMoveHand(cv,event);
      break;
      case cvt_freehand:
	CVMouseMoveFreeHand(cv,event);
      break;
      case cvt_curve: case cvt_corner: case cvt_tangent: case cvt_hvcurve:
	CVMouseMovePoint(cv,&p);
      break;
      case cvt_pen:
	CVMouseMovePen(cv,&p,event);
      break;
      case cvt_ruler:
	CVMouseMoveRuler(cv,event);
      break;
      case cvt_rotate: case cvt_flip: case cvt_scale: case cvt_skew:
      case cvt_3d_rotate: case cvt_perspective:
	CVMouseMoveTransform(cv);
      break;
      case cvt_knife:
	CVMouseMoveKnife(cv,&p);
      break;
      case cvt_rect: case cvt_elipse: case cvt_poly: case cvt_star:
	CVMouseMoveShape(cv);
      break;
    }
    if ( stop_motion ) {
	event->type = et_mouseup;
	CVMouseUp(cv,event);
    }
}

static void CVMagnify(CharView *cv, real midx, real midy, int bigger, int LockPosition) {
    CharViewTab* tab = CVGetActiveTab(cv);
    static float scales[] = { 1, 2, 3, 4, 6, 8, 11, 16, 23, 32, 45, 64, 90, 128, 181, 256, 512, 1024, 0 };
    float oldscale;
    int i, j;

    oldscale = tab->scale;

    if ( bigger!=0 ) {
	if ( tab->scale>=1 ) {
	    for ( i=0; scales[i]!=0 && tab->scale>scales[i]; ++i );
	    if ( scales[i]==0 ) i=j= i-1;
	    else if ( RealNear(scales[i],tab->scale) ) j=i;
	    else if ( i!=0 && RealNear(scales[i-1],tab->scale) ) j= --i; /* Round errors! */
	    else j = i-1;
	} else { real sinv = 1/tab->scale; int t;
	    for ( i=0; scales[i]!=0 && sinv>scales[i]; ++i );
	    if ( scales[i]==0 ) i=j= i-1;
	    else if ( RealNear(scales[i],sinv) ) j=i;
	    else if ( i!=0 && RealNear(scales[i-1],sinv) ) j= --i; /* Round errors! */
	    else j = i-1;
	    t = j;
	    j = -i; i = -t;
	}
	if ( bigger==1 ) {
	    if ( i==j ) ++i;
	    if ( i>0 && scales[i]==0 ) --i;
	    if ( i>=0 )
		tab->scale = scales[i];
	    else
		tab->scale = 1/scales[-i];
	} else {
	    if ( i==j ) --j;
	    if ( j<0 && scales[-j]==0 ) ++j;
	    if ( j>=0 )
		tab->scale = scales[j];
	    else
		tab->scale = 1/scales[-j];
	}
    }

    if (LockPosition) {
	float mousex = rint(midx * oldscale + tab->xoff);
	float mousey = rint(midy * oldscale + tab->yoff - cv->height);
	tab->xoff = mousex - midx*tab->scale;
	tab->yoff = mousey - midy*tab->scale + cv->height;
    }
    else {
        tab->xoff = -(rint(midx*tab->scale) - cv->width/2);
        tab->yoff = -(rint(midy*tab->scale) - cv->height/2);
    }

    CVNewScale(cv);
}

static void CVMouseUp(CharView *cv, GEvent *event ) {
    CharViewTab* tab = CVGetActiveTab(cv);

    CVMouseMove(cv,event);
    if ( cv->pressed!=NULL ) {
	GDrawCancelTimer(cv->pressed);
	cv->pressed = NULL;
    }
    cv->p.pressed = false;
    SplinePointListFree(cv->p.pretransform_spl);
    cv->p.pretransform_spl = NULL;
    update_spacebar_hand_tool(cv);

    if ( cv->p.rubberbanding ) {
	cv->p.rubberbanding = false;
    } else if ( cv->p.rubberlining ) {
	cv->p.rubberlining = false;
    }

    // This is needed to allow characters to the left of the
    // active one to be picked with the mouse,
    // but outright it does mess with keyboard input changing BCP
    // so we only do it for mouse up to the left of the left side
    // bearing, because that click can not currently activate any BCP
    if ( cv->active_tool == cvt_pointer &&
	 MouseToCX( cv, event->u.mouse.x ) < -2 )
    {
	// Since we allow clicking anywhere now, instead of having
	// to check if you clicked on a spline of a prev char,
	// then we don't need this. It also causes an issue with the arrow
	// keys moving a BCP on a spline left of the lbearing line.
	// (comment included just in case this click on spline feature is desired in the future)
	/* FindSel fs; */
	/* SetFS(&fs,&cv->p,cv,event); */
	/* _CVTestSelectFromEvent(cv,&fs); */
	/* fs.p = &cv->p; */
    }
    
    
    switch ( cv->active_tool ) {
      case cvt_pointer:
	CVMouseUpPointer(cv);
      break;
      case cvt_ruler:
	CVMouseUpRuler(cv,event);
      break;
      case cvt_hand:
	CVMouseUpHand(cv);
      break;
      case cvt_freehand:
	CVMouseUpFreeHand(cv,event);
      break;
      case cvt_curve: case cvt_corner: case cvt_tangent: case cvt_hvcurve:
      case cvt_pen:
	CVMouseUpPoint(cv,event);
	CVGridHandlePossibleFitChar( cv );
      break;
      case cvt_magnify: case cvt_minify:
	if ( cv->p.x>=event->u.mouse.x-6 && cv->p.x<=event->u.mouse.x+6 &&
		 cv->p.y>=event->u.mouse.y-6 && cv->p.y<=event->u.mouse.y+6 ) {
	    real cx, cy;
	    cx = (event->u.mouse.x-tab->xoff)/tab->scale;
	    cy = (cv->height-event->u.mouse.y-tab->yoff)/tab->scale ;
	    CVMagnify(cv,cx,cy,cv->active_tool==cvt_minify?-1:1,event->u.mouse.button>3);
        } else {
	    DBounds b;
	    double oldscale = tab->scale;
	    if ( cv->p.cx>cv->info.x ) {
		b.minx = cv->info.x;
		b.maxx = cv->p.cx;
	    } else {
		b.minx = cv->p.cx;
		b.maxx = cv->info.x;
	    }
	    if ( cv->p.cy>cv->info.y ) {
		b.miny = cv->info.y;
		b.maxy = cv->p.cy;
	    } else {
		b.miny = cv->p.cy;
		b.maxy = cv->info.y;
	    }
	    _CVFit(cv,&b,false);
	    if ( oldscale==tab->scale ) {
		tab->scale += .5;
		CVNewScale(cv);
	    }
	}
      break;
      case cvt_rotate: case cvt_flip: case cvt_scale: case cvt_skew:
      case cvt_3d_rotate: case cvt_perspective:
	CVMouseUpTransform(cv);
      break;
      case cvt_knife:
	CVMouseUpKnife(cv,event);
      break;
      case cvt_rect: case cvt_elipse: case cvt_poly: case cvt_star:
	CVMouseUpShape(cv);
	CVGridHandlePossibleFitChar( cv );
      break;
    }
    cv->active_tool = cvt_none;
    CVToolsSetCursor(cv,event->u.mouse.state&~(1<<(7+event->u.mouse.button)),event->u.mouse.device);		/* X still has the buttons set in the state, even though we just released them. I don't want em */
    /* CharChangedUpdate is a rather blunt tool. When moving anchor points with */
    /*  the mouse we need finer control than it provides */
    /* If recentchange is set then change should also be set, don't think we */
    /*  need the full form of this call */
    if ( cv->needsrasterize || cv->recentchange )
	_CV_CharChangedUpdate(cv,2);

    dlist_foreach( &cv->pointInfoDialogs, (dlist_foreach_func_type)PIChangePoint );
}

static void CVTimer(CharView *cv,GEvent *event) {
    CharViewTab* tab = CVGetActiveTab(cv);

    if ( event->u.timer.timer==cv->pressed ) {
	GEvent e;
	GDrawGetPointerPosition(cv->v,&e);
	if ( e.u.mouse.x<0 || e.u.mouse.y<0 ||
		e.u.mouse.x>=cv->width || e.u.mouse.y >= cv->height ) {
	    real dx = 0, dy = 0;
	    if ( e.u.mouse.x<0 )
		dx = cv->width/8;
	    else if ( e.u.mouse.x>=cv->width )
		dx = -cv->width/8;
	    if ( e.u.mouse.y<0 )
		dy = -cv->height/8;
	    else if ( e.u.mouse.y>=cv->height )
		dy = cv->height/8;
	    tab->xoff += dx; tab->yoff += dy;
	    cv->back_img_out_of_date = true;
	    if ( dy!=0 )
		GScrollBarSetPos(cv->vsb,tab->yoff-cv->height);
	    if ( dx!=0 )
		GScrollBarSetPos(cv->hsb,-tab->xoff);
	    GDrawRequestExpose(cv->v,NULL,false);
	}
#if _ModKeysAutoRepeat
	/* Under cygwin the modifier keys auto repeat, they don't under normal X */
    } else if ( cv->autorpt==event->u.timer.timer ) {
	cv->autorpt = NULL;
	CVToolsSetCursor(cv,cv->oldstate,NULL);
	if ( cv->keysym == GK_Shift_L || cv->keysym == GK_Shift_R ||
		 cv->keysym == GK_Alt_L || cv->keysym == GK_Alt_R ||
		 cv->keysym == GK_Meta_L || cv->keysym == GK_Meta_R ) {
	    GEvent e;
	    e.w = cv->oldkeyw;
	    e.u.chr.keysym = cv->keysym;
	    e.u.chr.x = cv->oldkeyx;
	    e.u.chr.y = cv->oldkeyy;
	    CVFakeMove(cv,&e);
	}
#endif
    }
}

static void CVDrop(CharView *cv,GEvent *event) {
    /* We should get a list of character names. Add each as a RefChar */
    int32_t len;
    int ch, first = true;
    char *start, *pt, *cnames;
    SplineChar *rsc;
    RefChar *new;
    int layer = CVLayer((CharViewBase *) cv);

    if ( cv->b.drawmode==dm_grid ) {
	ff_post_error(_("Not Guides"),_("References may not be dragged into the guidelines layer"));
return;
    }
    if ( !GDrawSelectionHasType(cv->gw,sn_drag_and_drop,"STRING"))
return;
    cnames = GDrawRequestSelection(cv->gw,sn_drag_and_drop,"STRING",&len);
    if ( cnames==NULL )
return;

    start = cnames;
    while ( *start ) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
    break;
	for ( pt=start; *pt && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	if ( (rsc=SFGetChar(cv->b.sc->parent,-1,start))!=NULL && rsc!=cv->b.sc ) {
	    if ( first ) {
		CVPreserveState(&cv->b);
		first =false;
	    }
	    new = RefCharCreate();
	    new->transform[0] = new->transform[3] = 1.0;
	    new->layers[0].splines = NULL;
	    new->sc = rsc;
	    new->next = cv->b.sc->layers[layer].refs;
	    cv->b.sc->layers[layer].refs = new;
	    SCReinstanciateRefChar(cv->b.sc,new,layer);
	    SCMakeDependent(cv->b.sc,rsc);
	}
	*pt = ch;
	start = pt;
    }

    free(cnames);
    CVCharChangedUpdate(&cv->b);
}

static int v_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    GGadgetPopupExternalEvent(event);
    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7 ) ) {
	if ( !(event->u.mouse.state&(ksm_control)) )	/* bind control to magnify/minify */
	{
	    int ish = event->u.mouse.button>5;
	    if ( event->u.mouse.state&ksm_shift ) ish = !ish;
	    if ( ish ) /* bind shift to vertical scrolling */
return( GGadgetDispatchEvent(cv->hsb,event));
	    else
return( GGadgetDispatchEvent(cv->vsb,event));
	}
    }

    switch ( event->type ) {
      case et_expose:
	GDrawSetLineWidth(gw,0);
	CVExpose(cv,gw,event);
      break;
      case et_crossing:
	CVCrossing(cv,event);
      break;
      case et_mousedown:
	CVPaletteActivate(cv);
	if ( cv->gic!=NULL )
	    GDrawSetGIC(gw,cv->gic,0,20);
	if ( cv->inactive )
	    (cv->b.container->funcs->activateMe)(cv->b.container,&cv->b);
	CVMouseDown(cv,event);
      break;
      case et_mousemove:
	if ( cv->p.pressed || cv->spacebar_hold)
	    GDrawSkipMouseMoveEvents(cv->v,event);
	CVMouseMove(cv,event);
      break;
      case et_mouseup:
	CVMouseUp(cv,event);
      break;
      case et_char:
	if ( cv->b.container!=NULL )
	    (cv->b.container->funcs->charEvent)(cv->b.container,event);
	else
	    CVChar(cv,event);
      break;
      case et_charup:
	CVCharUp(cv,event);
      break;
      case et_timer:
	CVTimer(cv,event);
      break;
      case et_drop:
	CVDrop(cv,event);
      break;
      case et_focus:
	if ( event->u.focus.gained_focus ) {
	    if ( cv->gic!=NULL )
		GDrawSetGIC(gw,cv->gic,0,20);
	}
      break;
      default: break;
    }
return( true );
}

static void CVDrawNum(CharView *UNUSED(cv),GWindow pixmap,int x, int y, char *format,real val, int align) {
    char buffer[40];
    int len;

    if ( val==0 ) val=0;		/* avoid -0 */
    sprintf(buffer,format,(double)val); /* formats are given as for doubles */
    if ( align!=0 ) {
	len = GDrawGetText8Width(pixmap,buffer,-1);
	if ( align==1 )
	    x-=len/2;
	else
	    x-=len;
    }
    GDrawDrawText8(pixmap,x,y,buffer,-1,GDrawGetDefaultForeground(NULL));
}

static void CVDrawVNum(CharView *cv,GWindow pixmap,int x, int y, char *format,real val, int align) {
    char buffer[40], *pt;
    int len;

    if ( val==0 ) val=0;		/* avoid -0 */
    sprintf(buffer,format,(double)val); /* formats are given as for doubles */
    if ( align!=0 ) {
	len = strlen(buffer)*cv->sfh;
	if ( align==1 )
	    y-=len/2;
	else
	    y-=len;
    }
    for ( pt=buffer; *pt; ++pt ) {
	GDrawDrawText8(pixmap,x,y,pt,1,GDrawGetDefaultForeground(NULL));
	y += cv->sdh;
    }
}


static void CVExposeRulers(CharView *cv, GWindow pixmap ) {
    CharViewTab* tab = CVGetActiveTab(cv);
    real xmin, xmax, ymin, ymax;
    real onehundred, pos;
    real units, littleunits;
    int ybase = cv->mbh+cv->charselectorh+cv->infoh;
    int x,y;
    GRect rect;
    Color def_fg = GDrawGetDefaultForeground(NULL);

    xmin = -tab->xoff/tab->scale;
    xmax = (cv->width-tab->xoff)/tab->scale;
    ymin = -tab->yoff/tab->scale;
    ymax = (cv->height-tab->yoff)/tab->scale;
    onehundred = 100/tab->scale;

    if ( onehundred<5 ) {
	units = 1; littleunits=0;
    } else if ( onehundred<10 ) {
	units = 5; littleunits=1;
    } else if ( onehundred<50 ) {
	units = 10; littleunits=2;
    } else if ( onehundred<100 ) {
	units = 25; littleunits=5;
    } else if ( onehundred<500/2 ) {
	units = 100; littleunits=20;
    } else if ( onehundred<1000/2 ) {
	// The next numbers (1000 and up) take more space to display, so Frank has adjusted the thresholds.
	units = 250; littleunits=50;
    } else if ( onehundred<5000/2 ) {
	units = 1000; littleunits=200;
    } else if ( onehundred<10000/2 ) {
	units = 2500; littleunits=500;
    } else if ( onehundred<50000/2 ) {
	units = 10000; littleunits=2000;
    } else {
	for ( units=1 ; units<onehundred*2; units *= 10 );
	units/=10; littleunits = units/5;
    }

    // Create the pixmaps
    if (cv->hruler != NULL) {
        GDrawGetSize(cv->hruler, &rect);
        if (rect.width != cv->width || rect.height != cv->rulerh) {
            GDrawDestroyWindow(cv->hruler);
            cv->hruler = NULL;
        }
    }
    if (cv->vruler != NULL) {
        GDrawGetSize(cv->vruler, &rect);
        if (rect.height != cv->height || rect.width != cv->rulerh) {
            GDrawDestroyWindow(cv->vruler);
            cv->vruler = NULL;
        }
    }
    if (cv->hruler == NULL) {
        cv->hruler = GDrawCreatePixmap(GDrawGetDisplayOfWindow(cv->v), cv->v, cv->width, cv->rulerh);
    }
    if (cv->vruler == NULL) {
        cv->vruler = GDrawCreatePixmap(GDrawGetDisplayOfWindow(cv->v), cv->v, cv->rulerh, cv->height);
    }

    // Set background
    rect.x = 0; rect.width = cv->width; rect.y = 0; rect.height = cv->rulerh;
    GDrawFillRect(cv->hruler, &rect, GDrawGetDefaultBackground(NULL));
    rect.width = cv->rulerh; rect.height = cv->height;
    GDrawFillRect(cv->vruler, &rect, GDrawGetDefaultBackground(NULL));

    // Draw bottom line of rulers
    GDrawSetLineWidth(cv->hruler, 0);
    GDrawSetLineWidth(cv->vruler, 0);
    GDrawDrawLine(cv->hruler, 0, cv->rulerh - 1, cv->width, cv->rulerh - 1, def_fg);
    GDrawDrawLine(cv->vruler, cv->rulerh - 1, 0, cv->rulerh - 1, cv->height, def_fg);

    // Draw the ticks
    GDrawSetFont(cv->hruler, cv->small);
    GDrawSetFont(cv->vruler, cv->small);
    if ( xmax-xmin<1 && cv->width>100 ) {
	CVDrawNum(cv,cv->hruler,0,cv->sas,"%.3f",xmin,0);
	CVDrawNum(cv,cv->hruler,cv->width,cv->sas,"%.3f",xmax,2);
    }

    if ( ymax-ymin<1 && cv->height>100 ) {
	CVDrawVNum(cv,cv->vruler,1,cv->height+cv->sas,"%.3f",ymin,0);
	CVDrawVNum(cv,cv->vruler,1,cv->sas,"%.3f",ymax,2);
    }
    if ( fabs(xmin/units) < 1e5 && fabs(ymin/units)<1e5 && fabs(xmax/units)<1e5 && fabs(ymax/units)<1e5 ) {
	if ( littleunits!=0 ) {
	    for ( pos=littleunits*ceil(xmin/littleunits); pos<xmax; pos += littleunits ) {
		x = tab->xoff + rint(pos*tab->scale);
		GDrawDrawLine(cv->hruler,x,cv->rulerh-4,x,cv->rulerh, def_fg);
	    }
	    for ( pos=littleunits*ceil(ymin/littleunits); pos<ymax; pos += littleunits ) {
		y = -tab->yoff + cv->height - rint(pos*tab->scale);
		GDrawDrawLine(cv->vruler,cv->rulerh-4,y,cv->rulerh,y, def_fg);
	    }
	}
	for ( pos=units*ceil(xmin/units); pos<xmax; pos += units ) {
	    x = tab->xoff + rint(pos*tab->scale);
	    GDrawDrawLine(cv->hruler,x,0,x,cv->rulerh, rulerbigtickcol);
	    CVDrawNum(cv,cv->hruler,x+15,cv->sas,"%g",pos,1);
	}
	for ( pos=units*ceil(ymin/units); pos<ymax; pos += units ) {
	    y = -tab->yoff + cv->height - rint(pos*tab->scale);
	    GDrawDrawLine(cv->vruler,0,y,cv->rulerh,y, rulerbigtickcol);
	    CVDrawVNum(cv,cv->vruler,1,y+cv->sas+20,"%g",pos,1);
	}
    }

    // Draw the pixmaps to screen
    GDrawDrawPixmap(pixmap, cv->vruler, &rect, 0, cv->rulerh + ybase);
    rect.width = cv->width; rect.height = cv->rulerh;
    GDrawDrawPixmap(pixmap, cv->hruler, &rect, cv->rulerh, ybase);
}

static void InfoExpose(CharView *cv, GWindow pixmap, GEvent *expose) {
    GRect old1, old2;
    Color def_fg = GDrawGetDefaultForeground(NULL);

    if ( expose->u.expose.rect.y + expose->u.expose.rect.height < cv->mbh ||
	    (!cv->showrulers && expose->u.expose.rect.y >= cv->mbh+cv->charselectorh+cv->infoh ))
return;

    GDrawPushClip(pixmap,&expose->u.expose.rect,&old1);
    GDrawSetLineWidth(pixmap,0);
    if ( expose->u.expose.rect.y< cv->mbh+cv->charselectorh+cv->infoh ) {
	GDrawPushClip(pixmap,&expose->u.expose.rect,&old2);

	GDrawDrawLine(pixmap,0,cv->mbh+cv->charselectorh+cv->infoh-1,8096,cv->mbh+cv->charselectorh+cv->infoh-1,def_fg);
	GDrawDrawImage(pixmap,&GIcon_rightpointer,NULL,RPT_BASE,cv->mbh+cv->charselectorh+2);
	GDrawDrawImage(pixmap,&GIcon_selectedpoint,NULL,SPT_BASE,cv->mbh+cv->charselectorh+2);
	GDrawDrawImage(pixmap,&GIcon_sel2ptr,NULL,SOF_BASE,cv->mbh+cv->charselectorh+2);
	GDrawDrawImage(pixmap,&GIcon_distance,NULL,SDS_BASE,cv->mbh+cv->charselectorh+2);
	GDrawDrawImage(pixmap,&GIcon_angle,NULL,SAN_BASE,cv->mbh+cv->charselectorh+2);
	GDrawDrawImage(pixmap,&GIcon_mag,NULL,MAG_BASE,cv->mbh+cv->charselectorh+2);
	CVInfoDrawText(cv,pixmap);
	GDrawPopClip(pixmap,&old2);
    }
    if ( cv->showrulers ) {
	CVExposeRulers(cv,pixmap);
	cv->olde.x = -1;
	CVInfoDrawRulers(cv,pixmap);
    }
    GDrawPopClip(pixmap,&old1);
}

void CVResize(CharView *cv ) {
    int sbsize = GDrawPointsToPixels(cv->gw,_GScrollBar_Width);
    GRect size;
    GDrawGetSize(cv->gw,&size);
    {
	int newwidth = size.width-sbsize,
	    newheight = size.height-sbsize - cv->mbh-cv->charselectorh-cv->infoh;
	int sbwidth = newwidth, sbheight = newheight;

	if ( cv->dv!=NULL ) {
	    newwidth -= cv->dv->dwidth;
	    sbwidth -= cv->dv->dwidth;
	}
	if ( newwidth<30 || newheight<50 ) {
	    if ( newwidth<30 )
		newwidth = 30+sbsize+(cv->dv!=NULL ? cv->dv->dwidth : 0);
	    if ( newheight<50 )
		newheight = 50+sbsize+cv->mbh+cv->charselectorh+cv->infoh;
	    GDrawResize(cv->gw,newwidth,newheight);
return;
	}

	if ( cv->dv!=NULL ) {
	    int dvheight = size.height-(cv->mbh+cv->charselectorh+cv->infoh);
	    GDrawMove(cv->dv->dv,size.width-cv->dv->dwidth,cv->mbh+cv->charselectorh+cv->infoh);
	    GDrawResize(cv->dv->dv,cv->dv->dwidth,dvheight);
	    GDrawResize(cv->dv->ii.v,cv->dv->dwidth-sbsize,dvheight-cv->dv->toph);
	    GGadgetResize(cv->dv->ii.vsb,sbsize,dvheight-cv->dv->toph);
	    cv->dv->ii.vheight = dvheight-cv->dv->toph;
	    GDrawRequestExpose(cv->dv->dv,NULL,false);
	    GDrawRequestExpose(cv->dv->ii.v,NULL,false);
	    GScrollBarSetBounds(cv->dv->ii.vsb,0,cv->dv->ii.lheight+1,
		    cv->dv->ii.vheight/cv->dv->ii.fh);
	}

	if (cv->charselector != NULL && cv->charselectorPrev != NULL && cv->charselectorNext != NULL) {
	  GRect charselector_size;
	  GRect charselectorNext_size;
	  GRect charselectorPrev_size;
	  GGadgetGetSize(cv->charselector, &charselector_size);
	  GGadgetGetSize(cv->charselectorPrev, &charselectorPrev_size);
	  GGadgetGetSize(cv->charselectorNext, &charselectorNext_size);
	  int new_charselector_width = newwidth + sbsize - ( 4 * charselector_size.x ) - charselectorNext_size.width - charselectorPrev_size.width;
	  int new_charselectorPrev_x = newwidth + sbsize - ( 2 * charselector_size.x ) - charselectorNext_size.width - charselectorPrev_size.width;
	  int new_charselectorNext_x = newwidth + sbsize - ( 1 * charselector_size.x ) - charselectorNext_size.width;
	  GGadgetResize(cv->charselector, new_charselector_width, charselector_size.height);
	  GGadgetMove(cv->charselectorPrev, new_charselectorPrev_x, charselectorPrev_size.y);
	  GGadgetMove(cv->charselectorNext, new_charselectorNext_x, charselectorNext_size.y);
	  charselector_size.x = 0; charselector_size.y = cv->mbh; charselector_size.width = newwidth + sbsize; charselector_size.height = cv->charselectorh;
	  GDrawRequestExpose(cv->gw, &charselector_size, false);
	}

	if ( cv->showrulers ) {
	    newheight -= cv->rulerh;
	    newwidth -= cv->rulerh;
	}

	if ( newwidth == cv->width && newheight == cv->height )
return;
	if ( cv->backimgs!=NULL )
	    GDrawDestroyWindow(cv->backimgs);
	cv->backimgs = NULL;

	/* MenuBar takes care of itself */
	GDrawResize(cv->v,newwidth,newheight);
	GGadgetMove(cv->vsb,sbwidth, cv->mbh+cv->charselectorh+cv->infoh);
	GGadgetResize(cv->vsb,sbsize,sbheight);
	GGadgetMove(cv->hsb,0,size.height-sbsize);
	GGadgetResize(cv->hsb,sbwidth,sbsize);
	cv->width = newwidth; cv->height = newheight;
	/*CVFit(cv);*/ CVNewScale(cv);
	CVPalettesRaise(cv);
	if ( cv->b.container == NULL && ( cv_width!=size.width || cv_height!=size.height )) {
	    cv_width = size.width;
	    cv_height = size.height;
	    SavePrefs(true);
	}
    }
}

static void CVHScrollSetPos( CharView *cv, int newpos )
{
    CharViewTab* tab = CVGetActiveTab(cv);
    TRACE("CVHScrollSetPos(1) cvxoff:%f newpos:%d\n", tab->xoff, newpos );
    if ( newpos<-(32000*tab->scale-cv->width) )
        newpos = -(32000*tab->scale-cv->width);
    if ( newpos>8000*tab->scale ) newpos = 8000*tab->scale;
    TRACE("CVHScrollSetPos(2) cvxoff:%f newpos:%d\n", tab->xoff, newpos );
    if ( newpos!=tab->xoff ) {
	int diff = newpos-tab->xoff;
	tab->xoff = newpos;
	cv->back_img_out_of_date = true;
	GScrollBarSetPos(cv->hsb,-newpos);
	GDrawScroll(cv->v,NULL,diff,0);
	CVRulerLingerMove(cv);
	if (( cv->showhhints && cv->b.sc->hstem!=NULL ) || cv->showblues || cv->showvmetrics ) {
	    GRect r;
	    r.y = 0; r.height = cv->height;
	    r.width = 6*cv->sfh+10;
	    if ( diff>0 )
		r.x = cv->width-r.width;
	    else
		r.x = cv->width+diff-r.width;
	    GDrawRequestExpose(cv->v,&r,false);
	}
	if ( cv->showrulers ) {
	    GRect r;
	    r.y = cv->infoh+cv->mbh+cv->charselectorh; r.height = cv->rulerh; r.x = 0; r.width = cv->rulerh+cv->width;
	    GDrawRequestExpose(cv->gw,&r,false);
	}
    }
}

static void CVHScroll(CharView *cv, struct sbevent *sb) {
    CharViewTab* tab = CVGetActiveTab(cv);
    int newpos = tab->xoff;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos += 9*cv->width/10;
      break;
      case et_sb_up:
        newpos += cv->width/15;
      break;
      case et_sb_down:
        newpos -= cv->width/15;
      break;
      case et_sb_downpage:
        newpos -= 9*cv->width/10;
      break;
      case et_sb_bottom:
        newpos = 0;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = -sb->pos;
      break;
      case et_sb_halfup:
        newpos += cv->width/30;
      break;
      case et_sb_halfdown:
        newpos -= cv->width/30;
      break;
    }

    CVHScrollSetPos( cv, newpos );
}

static void CVVScroll(CharView *cv, struct sbevent *sb) {
    CharViewTab* tab = CVGetActiveTab(cv);
    int newpos = tab->yoff;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= 9*cv->height/10;
      break;
      case et_sb_up:
        newpos -= cv->height/15;
      break;
      case et_sb_down:
        newpos += cv->height/15;
      break;
      case et_sb_downpage:
        newpos += 9*cv->height/10;
      break;
      case et_sb_bottom:
        newpos = 0;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos+cv->height;
      break;
      case et_sb_halfup:
        newpos -= cv->height/30;
      break;
      case et_sb_halfdown:
        newpos += cv->height/30;
      break;
    }
    if ( newpos<-(20000*tab->scale-cv->height) )
        newpos = -(20000*tab->scale-cv->height);
    if ( newpos>8000*tab->scale ) newpos = 8000*tab->scale;
    if ( newpos!=tab->yoff ) {
	int diff = newpos-tab->yoff;
	tab->yoff = newpos;
	cv->back_img_out_of_date = true;
	GScrollBarSetPos(cv->vsb,newpos-cv->height);
	GDrawScroll(cv->v,NULL,0,diff);
	CVRulerLingerMove(cv);
	if (( cv->showvhints && cv->b.sc->vstem!=NULL) || cv->showhmetrics ) {
	    GRect r;
	    RefChar *lock = HasUseMyMetrics(cv->b.sc,CVLayer((CharViewBase *) cv));
	    r.x = 0; r.width = cv->width;
	    r.height = 2*cv->sfh+6;
	    if ( lock!=NULL )
		r.height = cv->sfh+3+GImageGetHeight(&GIcon_lock);
	    if ( diff>0 )
		r.y = 0;
	    else
		r.y = -diff;
	    GDrawRequestExpose(cv->v,&r,false);
	}
	if ( cv->showrulers ) {
	    GRect r;
	    r.x = 0; r.width = cv->rulerh; r.y = cv->infoh+cv->charselectorh+cv->mbh; r.height = cv->rulerh+cv->height;
	    GDrawRequestExpose(cv->gw,&r,false);
	}
    }
}

void LogoExpose(GWindow pixmap,GEvent *event, GRect *r,enum drawmode dm) {
    int sbsize = GDrawPointsToPixels(pixmap,_GScrollBar_Width);
    GRect old;

    r->width = r->height = sbsize;
    if ( event->u.expose.rect.x+event->u.expose.rect.width >= r->x &&
	    event->u.expose.rect.y+event->u.expose.rect.height >= r->y ) {
	/* Put something into the little box where the two scroll bars meet */
	int xoff, yoff;
	GImage *which = (dm==dm_fore) ? &GIcon_FontForgeLogo :
			(dm==dm_back) ? &GIcon_FontForgeBack :
			    &GIcon_FontForgeGuide;
	struct _GImage *base = which->u.image;
	xoff = (sbsize-base->width);
	yoff = (sbsize-base->height);
	GDrawPushClip(pixmap,r,&old);
	GDrawDrawImage(pixmap,which,NULL,
		r->x+(xoff-xoff/2),r->y+(yoff-yoff/2));
	GDrawPopClip(pixmap,&old);
	/*GDrawDrawLine(pixmap,r->x+sbsize-1,r->y,r->x+sbsize-1,r->y+sbsize,0x000000);*/
    }
}

static void CVLogoExpose(CharView *cv,GWindow pixmap,GEvent *event) {
    int rh = cv->showrulers ? cv->rulerh : 0;
    GRect r;

    r.x = cv->width+rh;
    r.y = cv->height+cv->mbh+cv->charselectorh+cv->infoh+rh;
    LogoExpose(pixmap,event,&r,cv->b.drawmode==dm_grid? dm_grid :
	    cv->b.layerheads[cv->b.drawmode]->background ? dm_back : dm_fore );
}

static void CVDrawGuideLine(CharView *cv, int guide_pos) {
    GWindow pixmap = cv->v;

    if ( guide_pos<0 )
return;
    GDrawSetDashedLine(pixmap,2,2,0);
    GDrawSetLineWidth(pixmap,0);

    if ( cv->ruler_pressedv ) {
	GDrawDrawLine(pixmap,guide_pos,0,guide_pos,cv->height,guidedragcol);
    } else {
	GDrawDrawLine(pixmap,0,guide_pos,cv->width,guide_pos,guidedragcol);
    }
    GDrawSetDashedLine(pixmap,0,0,0);
}

static void CVAddGuide(CharView *cv,int is_v,int guide_pos) {
    CharViewTab* tab = CVGetActiveTab(cv);
    SplinePoint *sp1, *sp2;
    SplineSet *ss;
    SplineFont *sf = cv->b.sc->parent;
    int emsize = sf->ascent+sf->descent;

    if ( is_v ) {
	/* Create a vertical guide line */
	double x = (guide_pos-tab->xoff)/tab->scale;
	sp1 = SplinePointCreate(x,sf->ascent+emsize/2);
	sp2 = SplinePointCreate(x,-sf->descent-emsize/2);
    } else {
	double y = (cv->height-guide_pos-tab->yoff)/tab->scale;
	sp1 = SplinePointCreate(-emsize,y);
	sp2 = SplinePointCreate(2*emsize,y);
    }
    SplineMake(sp1,sp2,sf->grid.order2);
    ss = chunkalloc(sizeof(SplineSet));
    ss->first = sp1; ss->last = sp2;
    ss->next = sf->grid.splines;
    sf->grid.splines = ss;
    ss->contour_name = gwwv_ask_string(_("Name this contour"),NULL,
		_("Name this guideline or cancel to create it without a name"));
    if ( ss->contour_name!=NULL && *ss->contour_name=='\0' ) {
	free(ss->contour_name);
	ss->contour_name = NULL;
    }

    FVRedrawAllCharViewsSF(sf);
    if ( !sf->changed ) {
	sf->changed = true;
	FVSetTitles(sf);
    }
}

static int cv_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
	int ish = event->u.mouse.button>5;
	if ( event->u.mouse.state&ksm_shift ) ish = !ish;
	if ( ish ) /* bind shift to vertical scrolling */
return( GGadgetDispatchEvent(cv->hsb,event));
	else
return( GGadgetDispatchEvent(cv->vsb,event));
    }

    switch ( event->type ) {
      case et_selclear:
	ClipboardClear();
      break;
      case et_expose:
	GDrawSetLineWidth(gw,0);
	InfoExpose(cv,gw,event);
	CVLogoExpose(cv,gw,event);
      break;
      case et_char:
	if ( cv->b.container!=NULL )
	    (cv->b.container->funcs->charEvent)(cv->b.container,event);
	else
	    CVChar(cv,event);
      break;
      case et_charup:
	CVCharUp(cv,event);
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    CVResize(cv);
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    if ( event->u.control.g == cv->hsb )
		CVHScroll(cv,&event->u.control.u.sb);
	    else
		CVVScroll(cv,&event->u.control.u.sb);
	  break;
	  default: break;
	}
      break;
      case et_map:
	if ( event->u.map.is_visible )
	    CVPaletteActivate(cv);
	else
	    CVPalettesHideIfMine(cv);
      break;
      case et_destroy:
	CVUnlinkView(cv);
	CVPalettesHideIfMine(cv);
	if ( cv->backimgs!=NULL ) {
	    GDrawDestroyWindow(cv->backimgs);
	    cv->backimgs = NULL;
	}
	if ( cv->icon!=NULL ) {
	    GDrawDestroyWindow(cv->icon);
	    cv->icon = NULL;
	}
    if (cv->hruler != NULL) {
        GDrawDestroyWindow(cv->hruler);
        cv->hruler = NULL;
    }
    if (cv->vruler != NULL) {
        GDrawDestroyWindow(cv->vruler);
        cv->vruler = NULL;
    }
	CharViewFree(cv);
      break;
      case et_close:
	  dlist_foreach( &cv->pointInfoDialogs, PI_Destroy );
	  GDrawDestroyWindow(gw);
      break;
      case et_mouseup: case et_mousedown:
	GGadgetEndPopup();
	CVPaletteActivate(cv);
	if ( cv->inactive )
	    (cv->b.container->funcs->activateMe)(cv->b.container,&cv->b);
	if ( cv->showrulers ) {
	    int is_h = event->u.mouse.y>cv->mbh+cv->charselectorh+cv->infoh && event->u.mouse.y<cv->mbh+cv->charselectorh+cv->infoh+cv->rulerh;
	    int is_v = event->u.mouse.x<cv->rulerh;
	    if ( cv->gwgic!=NULL && event->type==et_mousedown)
		GDrawSetGIC(gw,cv->gwgic,0,20);
	    if ( event->type == et_mousedown ) {
		if ( is_h && is_v )
		    /* Ambiguous, ignore */;
		else if ( is_h ) {
		    cv->ruler_pressed = true;
		    cv->ruler_pressedv = false;
		    GDrawSetCursor(cv->v,ct_updown);
		    GDrawSetCursor(cv->gw,ct_updown);
		} else if ( is_v ) {
		    cv->ruler_pressed = true;
		    cv->ruler_pressedv = true;
		    GDrawSetCursor(cv->v,ct_leftright);
		    GDrawSetCursor(cv->gw,ct_leftright);
		}
		cv->guide_pos = -1;
	    } else if ( event->type==et_mouseup && cv->ruler_pressed ) {
		cv->guide_pos = -1;
		cv->showing_tool = cvt_none;
		CVToolsSetCursor(cv,event->u.mouse.state&~(1<<(7+event->u.mouse.button)),event->u.mouse.device);		/* X still has the buttons set in the state, even though we just released them. I don't want em */
		GDrawSetCursor(cv->gw,ct_mypointer);
		cv->ruler_pressed = false;
		if ( is_h || is_v )
		    /* Do Nothing */;
		else if ( cv->ruler_pressedv )
		    CVAddGuide(cv,true,event->u.mouse.x-cv->rulerh);
		else
		    CVAddGuide(cv,false,event->u.mouse.y-(cv->mbh+cv->charselectorh+cv->infoh+cv->rulerh));
	    }
	}
      break;
      case et_mousemove:
	if ( cv->ruler_pressed ) {
        CharViewTab* tab = CVGetActiveTab(cv);
	    cv->e.x = event->u.mouse.x - cv->rulerh;
	    cv->e.y = event->u.mouse.y-(cv->mbh+cv->charselectorh+cv->infoh+cv->rulerh);
	    cv->info.x = (cv->e.x-tab->xoff)/tab->scale;
	    cv->info.y = (cv->height-cv->e.y-tab->yoff)/tab->scale;
	    if ( cv->ruler_pressedv )
		cv->guide_pos = cv->e.x;
	    else
		cv->guide_pos = cv->e.y;
	    GDrawRequestExpose(cv->v,NULL,false);
	    CVInfoDraw(cv,cv->gw);
	}
    else if ( event->u.mouse.y > cv->mbh )
    {
        if( GGadgetContainsEventLocation( cv->charselectorPrev, event ))
        {
            GGadgetPreparePopup(cv->gw,c_to_u("Show the previous word in the current word list\n"
                                              "Select the menu File / Load Word List... to load a wordlist."));
        }
        else if( GGadgetContainsEventLocation( cv->charselectorNext, event ))
        {
            GGadgetPreparePopup(cv->gw,c_to_u("Show the next word in the current word list\n"
                                              "Select the menu File / Load Word List... to load a wordlist."));
        }
        else if( GGadgetContainsEventLocation( cv->charselector, event ))
        {
            GGadgetPreparePopup(cv->gw,c_to_u("This is a word list that you can step through to quickly see your glyphs in context\n"
                                              "Select the menu File / Load Word List... to load a wordlist."));
        }
        else
        {
            int enc = CVCurEnc(cv);
            SCPreparePopup(cv->gw,cv->b.sc,cv->b.fv->map->remap,enc,
                           UniFromEnc(enc,cv->b.fv->map->enc));
        }
    }
      break;
      case et_drop:
	CVDrop(cv,event);
      break;
      case et_focus:
	if ( event->u.focus.gained_focus ) {
	    if ( cv->gic!=NULL )
		GDrawSetGIC(gw,cv->gic,0,20);

	    // X11 on Windows is broken, non-active windows
	    // receive this event on mouseover
#if !defined(_WIN32) || defined(FONTFORGE_CAN_USE_GDK)
	    CVPaletteActivate(cv);
#endif
	}
      break;
      default: break;
    }
return( true );
}

#define MID_Fit		2001
#define MID_ZoomIn	2002
#define MID_ZoomOut	2003
#define MID_HidePoints	2004
#define MID_HideControlPoints	2005
#define MID_Fill	2006
#define MID_Next	2007
#define MID_Prev	2008
#define MID_HideRulers	2009
#define MID_Preview     2010
#define MID_DraggingComparisonOutline 2011
#define MID_NextDef	2012
#define MID_PrevDef	2013
#define MID_DisplayCompositions	2014
#define MID_MarkExtrema	2015
#define MID_Goto	2016
#define MID_FindInFontView	2017
#define MID_KernPairs	2018
#define MID_AnchorPairs	2019
#define MID_ShowGridFit 2020
#define MID_PtsNone	2021
#define MID_PtsTrue	2022
#define MID_PtsPost	2023
#define MID_PtsSVG	2024
#define MID_Ligatures	2025
#define MID_Former	2026
#define MID_MarkPointsOfInflection	2027
#define MID_ShowCPInfo	2028
#define MID_ShowTabs	2029
#define MID_AnchorGlyph	2030
#define MID_AnchorControl 2031
#define MID_ShowSideBearings	2032
#define MID_Bigger	2033
#define MID_Smaller	2034
#define MID_GridFitAA	2035
#define MID_GridFitOff	2036
#define MID_ShowHHints	2037
#define MID_ShowVHints	2038
#define MID_ShowDHints	2039
#define MID_ShowBlueValues	2040
#define MID_ShowFamilyBlues	2041
#define MID_ShowAnchors		2042
#define MID_ShowHMetrics	2043
#define MID_ShowVMetrics	2044
#define MID_ShowRefNames	2045
#define MID_ShowAlmostHV	2046
#define MID_ShowAlmostHVCurves	2047
#define MID_DefineAlmost	2048
#define MID_SnapOutlines	2049
#define MID_ShowDebugChanges	2050
#define MID_Cut		2101
#define MID_Copy	2102
#define MID_Paste	2103
#define MID_Clear	2104
#define MID_Merge	2105
#define MID_SelAll	2106
#define MID_CopyRef	2107
#define MID_UnlinkRef	2108
#define MID_Undo	2109
#define MID_Redo	2110
#define MID_CopyWidth	2111
#define MID_RemoveUndoes	2112
#define MID_CopyFgToBg	2115
#define MID_NextPt	2116
#define MID_PrevPt	2117
#define MID_FirstPt	2118
#define MID_NextCP	2119
#define MID_PrevCP	2120
#define MID_SelNone	2121
#define MID_SelectWidth	2122
#define MID_SelectVWidth	2123
#define MID_CopyLBearing	2124
#define MID_CopyRBearing	2125
#define MID_CopyVWidth	2126
#define MID_Join	2127
#define MID_CopyGridFit	2128
/*#define MID_Elide	2129*/
#define MID_SelectAllPoints	2130
#define MID_SelectAnchors	2131
#define MID_FirstPtNextCont	2132
#define MID_Contours	2133
#define MID_SelectHM	2134
#define MID_SelInvert	2136
#define MID_CopyBgToFg	2137
#define MID_SelPointAt	2138
#define MID_CopyLookupData	2139
#define MID_SelectOpenContours	2140
#define MID_MergeToLine	2141
#define MID_Clockwise	2201
#define MID_Counter	2202
#define MID_GetInfo	2203
#define MID_Correct	2204
#define MID_AvailBitmaps	2210
#define MID_RegenBitmaps	2211
#define MID_Stroke	2205
#define MID_RmOverlap	2206
#define MID_Simplify	2207
#define MID_BuildAccent	2208
#define MID_Autotrace	2212
#define MID_Round	2213
#define MID_Embolden	2217
#define MID_Condense	2218
#define MID_Average	2219
#define MID_SpacePts	2220
#define MID_SpaceRegion	2221
#define MID_MakeParallel	2222
#define MID_ShowDependentRefs	2223
#define MID_AddExtrema	2224
#define MID_CleanupGlyph	2225
#define MID_TilePath	2226
#define MID_BuildComposite	2227
#define MID_Exclude	2228
#define MID_Intersection	2229
#define MID_FindInter	2230
#define MID_Styles	2231
#define MID_SimplifyMore	2232
#define MID_First	2233
#define MID_Earlier	2234
#define MID_Later	2235
#define MID_Last	2236
#define MID_CharInfo	2240
#define MID_ShowDependentSubs	2241
#define MID_CanonicalStart	2242
#define MID_CanonicalContours	2243
#define MID_RemoveBitmaps	2244
#define MID_RoundToCluster	2245
#define MID_Align		2246
#define MID_FontInfo		2247
#define MID_FindProblems	2248
#define MID_InsertText		2249
#define MID_Italic		2250
#define MID_ChangeXHeight	2251
#define MID_ChangeGlyph		2252
#define MID_CheckSelf		2253
#define MID_GlyphSelfIntersects	2254
#define MID_ReverseDir		2255
#define MID_AddInflections	2256
#define MID_Balance	2257
#define MID_Harmonize	2258
#define MID_Corner	2301
#define MID_Tangent	2302
#define MID_Curve	2303
#define MID_MakeFirst	2304
#define MID_MakeLine	2305
#define MID_CenterCP	2306
#define MID_ImplicitPt	2307
#define MID_NoImplicitPt	2308
#define MID_InsertPtOnSplineAt	2309
#define MID_AddAnchor	2310
#define MID_HVCurve	2311
#define MID_SpiroG4	2312
#define MID_SpiroG2	2313
#define MID_SpiroCorner	2314
#define MID_SpiroLeft	2315
#define MID_SpiroRight	2316
#define MID_SpiroMakeFirst 2317
#define MID_NamePoint	2318
#define MID_NameContour	2319
#define MID_AcceptableExtrema 2320
#define MID_MakeArc	2321
#define MID_ClipPath	2322

#define MID_AutoHint	2400
#define MID_ClearHStem	2401
#define MID_ClearVStem	2402
#define MID_ClearDStem	2403
#define MID_AddHHint	2404
#define MID_AddVHint	2405
#define MID_AddDHint	2406
#define MID_ReviewHints	2407
#define MID_CreateHHint	2408
#define MID_CreateVHint	2409
#define MID_MinimumDistance	2410
#define MID_AutoInstr	2411
#define MID_ClearInstr	2412
#define MID_EditInstructions 2413
#define MID_Debug	2414
#define MID_HintSubsPt	2415
#define MID_AutoCounter	2416
#define MID_DontAutoHint	2417
#define MID_Deltas	2418
#define MID_Tools	2501
#define MID_Layers	2502
#define MID_DockPalettes	2503
#define MID_Center	2600
#define MID_SetWidth	2601
#define MID_SetLBearing	2602
#define MID_SetRBearing	2603
#define MID_Thirds	2604
#define MID_RemoveKerns	2605
#define MID_SetVWidth	2606
#define MID_RemoveVKerns	2607
#define MID_KPCloseup	2608
#define MID_AnchorsAway	2609
#define MID_SetBearings	2610
#define MID_OpenBitmap	2700
#define MID_Revert	2702
#define MID_Recent	2703
#define MID_RevertGlyph	2707
#define MID_Open	2708
#define MID_New		2709
#define MID_Close	2710
#define MID_Quit	2711
#define MID_CloseTab	2712
#define MID_GenerateTTC	2713
#define MID_VKernClass  2715
#define MID_VKernFromHKern 2716

#define MID_ShowGridFitLiveUpdate 2720

#define MID_MMReblend	2800
#define MID_MMAll	2821
#define MID_MMNone	2822

#define MID_PtsPos      2823

#define MID_Warnings	3000

static void CVMenuClose(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    if ( cv->b.container )
	(cv->b.container->funcs->doClose)(cv->b.container);
    else
	GDrawDestroyWindow(gw);
}

// This can be triggered two ways: by the "X" buttons on tabs, and regularly in the menu.
static void CVMenuCloseTab(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int pos, i;

    if ( cv->b.container || cv->tabs==NULL || cv->former_cnt<=1 )
return;
    pos = cv->ctpos == -1 ? GTabSetGetSel(cv->tabs) : cv->ctpos;
    free(cv->former_names[pos]);
    for ( i=pos+1; i<cv->former_cnt; ++i ) {
	cv->former_names[i-1] = cv->former_names[i];
    cv->cvtabs[i-1] = cv->cvtabs[i];
    }
    --cv->former_cnt;
    if (cv->ctpos==-1){
        GTabSetRemoveTabByPos(cv->tabs,pos);	/* This should send an event that the selection has changed */
    }
    GTabSetRemetric(cv->tabs);
    if (GTabSetGetSel(cv->tabs) >= pos) {
    CVChangeSC_fetchTab(cv, pos);
    // Otherwise subsequent calls to CVChangeSC_storeTab which use this value will be off by one
    cv->oldtabnum = pos;
    }
    if (GTabSetGetTabCount(cv->tabs)<=1) CVChangeTabsVisibility(cv,false);
    // Reset this argument so in case it's called by the menu the right tab will be removed.
    cv->ctpos = -1;
}

static void CVMenuOpen(GWindow gw, struct gmenuitem *mi, GEvent *g) {
    CharView *d = (CharView*)GDrawGetUserData(gw);
    FontView *fv = NULL;
    if (d) {
        fv = (FontView*)d->b.fv;
    }
    _FVMenuOpen(fv);
}

static void CVMenuOpenBitmap(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    if ( cv->b.fv->sf->bitmaps==NULL )
return;
    BitmapViewCreatePick(CVCurEnc(cv),(FontView *) (cv->b.fv));
}

static void CVMenuOpenMetrics(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    MetricsViewCreate((FontView *) (cv->b.fv),cv->b.sc,NULL);
}

static void CVMenuSave(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _FVMenuSave((FontView *) (cv->b.fv));
}

static void CVMenuSaveAs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _FVMenuSaveAs((FontView *) (cv->b.fv));
}

static void CVMenuGenerate(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    FontViewBase *fv = cv->b.fv;
    SFGenerateFont(cv->b.sc->parent,CVLayer((CharViewBase *) cv),false,fv->normal==NULL?fv->map:fv->normal);
}

static void CVMenuGenerateFamily(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _FVMenuGenerate((FontView *) (cv->b.fv),gf_macfamily);
}

static void CVMenuGenerateTTC(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _FVMenuGenerate((FontView *) (cv->b.fv),gf_ttc);
}

static void CVMenuExport(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVExport(cv);
}

static void CVInkscapeAdjust(CharView *cv) {
    CharViewTab* tab = CVGetActiveTab(cv);
    /* Inkscape considers different coordinates useful. That is to say, */
    /*  Inkscape views the world as a blank sheet of paper and often */
    /*  put things outside the [0,1000] range (especially in Y) that */
    /*  FF uses. So after doing a Paste, or Import or something similar */
    /*  check and see if the image is completely out of view, and if so */
    /*  then adjust the view field */
    DBounds b;
    int layer = CVLayer((CharViewBase *) cv);

    if (layer != ly_grid) SplineCharLayerQuickBounds(cv->b.sc,layer,&b);
    else {
        b.minx = b.miny = 1e10;
        b.maxx = b.maxy = -1e10;
	SplineSetQuickBounds(cv->b.sc->parent->grid.splines,&b);
    }

    b.minx *= tab->scale; b.maxx *= tab->scale;
    b.miny *= tab->scale; b.maxy *= tab->scale;

    if ( b.minx + tab->xoff < 0 || b.miny + tab->yoff < 0 ||
	    b.maxx + tab->xoff > cv->width || b.maxy + tab->yoff > cv->height )
	CVFit(cv);
}

static void CVMenuImport(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVImport(cv);
    CVInkscapeAdjust(cv);
}

static void CVMenuRevert(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    FVDelay((FontView *) (cv->b.fv),(void (*)(FontView *)) FVRevert);
			    /* The revert command can potentially */
			    /* destroy our window (if the char weren't in the */
			    /* old font). If that happens before the menu finishes */
			    /* we get a crash. So delay till after the menu completes */
}

static void CVMenuRevertGlyph(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplineChar *sc, temp;
    Undoes **undoes;
    int layer, lc;
    CharView *cvs;
    int mylayer = CVLayer((CharViewBase *) cv);

    if ( cv->b.sc->parent->filename==NULL || cv->b.sc->namechanged || cv->b.sc->parent->mm!=NULL )
return;
    if ( cv->b.sc->parent->sfd_version<2 )
	ff_post_error(_("Old sfd file"),_("This font comes from an old format sfd file. Not all aspects of it can be reverted successfully."));

    sc = SFDReadOneChar(cv->b.sc->parent,cv->b.sc->name);
    if ( sc==NULL ) {
	ff_post_error(_("Can't Find Glyph"),_("The glyph, %.80s, can't be found in the sfd file"),cv->b.sc->name);
	cv->b.sc->namechanged = true;
    } else {
	SCPreserveState(cv->b.sc,true);
	SCPreserveBackground(cv->b.sc);
	temp = *cv->b.sc;
	cv->b.sc->dependents = NULL;
	lc = cv->b.sc->layer_cnt;
	undoes = malloc(lc*sizeof(Undoes *));
	for ( layer=0; layer<lc; ++layer ) {
	    undoes[layer] = cv->b.sc->layers[layer].undoes;
	    cv->b.sc->layers[layer].undoes = NULL;
	}
	SplineCharFreeContents(cv->b.sc);
	*cv->b.sc = *sc;
	chunkfree(sc,sizeof(SplineChar));
	cv->b.sc->parent = temp.parent;
	cv->b.sc->dependents = temp.dependents;
	for ( layer = 0; layer<lc && layer<cv->b.sc->layer_cnt; ++layer )
	    cv->b.sc->layers[layer].undoes = undoes[layer];
	for ( ; layer<lc; ++layer )
	    UndoesFree(undoes[layer]);
	free(undoes);
	cv->b.sc->views = temp.views;
	/* cv->b.sc->changed = temp.changed; */
	for ( cvs=(CharView *) (cv->b.sc->views); cvs!=NULL; cvs=(CharView *) (cvs->b.next) ) {
	    cvs->b.layerheads[dm_back] = &cv->b.sc->layers[ly_back];
	    cvs->b.layerheads[dm_fore] = &cv->b.sc->layers[ly_fore];
	    if ( cv->b.sc->parent->multilayer ) {
		if ( mylayer!=ly_back )
		    cvs->b.layerheads[dm_fore] = &cv->b.sc->layers[mylayer];
	    } else {
		if ( mylayer!=ly_fore )
		    cvs->b.layerheads[dm_back] = &cv->b.sc->layers[mylayer];
	    }
	}
	RevertedGlyphReferenceFixup(cv->b.sc, temp.parent);
	_CV_CharChangedUpdate(cv,false);
    }
}

static void CVAddWordList(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e))
{
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    WordlistLoadToGTextInfo( cv->charselector, &cv->charselectoridx );
}

static void CVMenuPrint(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    PrintFFDlg(NULL,cv->b.sc,NULL);
}

#if !defined(_NO_PYTHON)
static void CVMenuExecute(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    ScriptDlg((FontView *) (cv->b.fv),cv);
}
#endif		/* !_NO_PYTHON */

static void fllistcheck(GWindow gw, struct gmenuitem *mi,GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    FontView *fvs;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Open: case MID_New:
	    mi->ti.disabled = cv->b.container!=NULL && !(cv->b.container->funcs->canOpen)(cv->b.container);
	  break;
	  case MID_GenerateTTC:
	    for ( fvs=fv_list; fvs!=NULL; fvs=(FontView *) (fvs->b.next) ) {
		if ( fvs!=(FontView *) cv->b.fv )
	    break;
	    }
	    mi->ti.disabled = fvs==NULL || cv->b.container!=NULL;
	  break;
	  case MID_Revert:
	    mi->ti.disabled = cv->b.fv->sf->origname==NULL || cv->b.fv->sf->new || cv->b.container;
	  break;
	  case MID_RevertGlyph:
	    mi->ti.disabled = cv->b.fv->sf->filename==NULL ||
		    cv->b.fv->sf->sfd_version<2 ||
		    cv->b.sc->namechanged ||
		    cv->b.fv->sf->mm!=NULL ||
		    cv->b.container;
	  break;
	  case MID_Recent:
	    mi->ti.disabled = !RecentFilesAny() ||
		    (cv->b.container!=NULL && !(cv->b.container->funcs->canOpen)(cv->b.container));
	  break;
	  case MID_Close: case MID_Quit:
	    mi->ti.disabled = false;
	  break;
	  case MID_CloseTab:
	    mi->ti.disabled = cv->tabs==NULL || cv->former_cnt<=1;
	  break;
	  default:
	    mi->ti.disabled = cv->b.container!=NULL;
	  break;
	}
    }
}

static void CVMenuFontInfo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    FontInfo(cv->b.sc->parent,CVLayer((CharViewBase *) cv),-1,false);
}

static void CVMenuFindProblems(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    FindProblems(NULL,cv,NULL);
}

static void CVMenuEmbolden(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    EmboldenDlg(NULL,cv);
}

static void CVMenuItalic(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    ItalicDlg(NULL,cv);
}

static void CVMenuOblique(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    ObliqueDlg(NULL,cv);
}

static void CVMenuCondense(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CondenseExtendDlg(NULL,cv);
}

static void CVMenuChangeXHeight(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    ChangeXHeightDlg(NULL,cv);
}

static void CVMenuChangeGlyph(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    GlyphChangeDlg(NULL,cv,gc_generic);
}

static void CVMenuInline(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    OutlineDlg(NULL,cv,NULL,true);
}

static void CVMenuOutline(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    OutlineDlg(NULL,cv,NULL,false);
}

static void CVMenuShadow(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    ShadowDlg(NULL,cv,NULL,false);
}

static void CVMenuWireframe(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    ShadowDlg(NULL,cv,NULL,true);
}

static void _CVMenuScale(CharView *cv, int mid) {
    CharViewTab* tab = CVGetActiveTab(cv);
    if ( mid == MID_Fit ) {
	CVFit(cv);
    } else {
	BasePoint c;
	c.x = (cv->width/2-tab->xoff)/tab->scale;
	c.y = (cv->height/2-tab->yoff)/tab->scale;
	if ( CVAnySel(cv,NULL,NULL,NULL,NULL))
	    CVFindCenter(cv,&c,false);
	CVMagnify(cv,c.x,c.y, mid==MID_ZoomOut?-1:1,0);
    }
}

static void CVMenuScale(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuScale(cv,mi->mid);
}

static void CVMenuShowHide(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVShows.showpoints = cv->showpoints = !cv->showpoints;
    GDrawRequestExpose(cv->v,NULL,false);
}

static void CVMenuShowHideControlPoints(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVShows.alwaysshowcontrolpoints = cv->alwaysshowcontrolpoints = !cv->alwaysshowcontrolpoints;
    GDrawRequestExpose(cv->v,NULL,false);
}

static void CVMenuNumberPoints(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    switch ( mi->mid ) {
      case MID_PtsNone:
	cv->showpointnumbers = 0;
      break;
      case MID_PtsTrue:
	cv->showpointnumbers = 1;
	CVCheckPoints(cv);
      break;
      case MID_PtsPost:
	cv->showpointnumbers = 1;
	cv->b.sc->numberpointsbackards = true;
      break;
      case MID_PtsSVG:
	cv->showpointnumbers = 1;
	cv->b.sc->numberpointsbackards = false;
      break;
      case MID_PtsPos:
        cv->showpointnumbers = 2;
      break;
    }
    SCNumberPoints(cv->b.sc,CVLayer((CharViewBase *) cv));
    SCUpdateAll(cv->b.sc);
}

static void CVMenuMarkExtrema(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.markextrema = cv->markextrema = !cv->markextrema;
    SavePrefs(true);
    GDrawRequestExpose(cv->v,NULL,false);
}

static void CVMenuMarkPointsOfInflection(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.markpoi = cv->markpoi = !cv->markpoi;
    SavePrefs(true);
    GDrawRequestExpose(cv->v,NULL,false);
}

static void CVMenuShowAlmostHV(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.showalmosthvlines = cv->showalmosthvlines = !cv->showalmosthvlines;
    SavePrefs(true);
    GDrawRequestExpose(cv->v,NULL,false);
}

static void CVMenuShowAlmostHVCurves(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.showalmosthvcurves = cv->showalmosthvcurves = !cv->showalmosthvcurves;
    SavePrefs(true);
    GDrawRequestExpose(cv->v,NULL,false);
}

static void CVMenuDefineAlmost(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    char buf[20], *end;
    int val;
    char *ret;

    sprintf(buf,"%d",cv->hvoffset );

    ret = gwwv_ask_string(_("Define \"Almost Horizontal\""),buf,
	    _("A line is \"almost\" horizontal (or vertical)\nif the coordinates are within this many em-units"));
    if ( ret==NULL )
return;
    val = strtol(ret,&end,10);
    if ( val>100 || val<=0 || *end!='\0' ) {
	free(ret);
	ff_post_error(_("Bad number"),_("Bad number"));
    } else {
	free(ret);
	CVShows.hvoffset = cv->hvoffset = val;
	SavePrefs(true);
	GDrawRequestExpose(cv->v,NULL,false);
    }
}

static void CVMenuShowCPInfo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.showcpinfo = cv->showcpinfo = !cv->showcpinfo;
    SavePrefs(true);
    /* Nothing to update, only show this stuff in the user is moving a cp */
    /*  which s/he is currently not, s/he is manipulating the menu */
}

static void CVMenuShowTabs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.showtabs = cv->showtabs = !cv->showtabs;
    CVChangeTabsVisibility(cv,cv->showtabs);
    SavePrefs(true);
}

static void CVMenuShowSideBearings(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.showsidebearings = cv->showsidebearings = !cv->showsidebearings;
    SavePrefs(true);
    GDrawRequestExpose(cv->v,NULL,false);
}

static void CVMenuShowRefNames(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.showrefnames = cv->showrefnames = !cv->showrefnames;
    SavePrefs(true);
    GDrawRequestExpose(cv->v,NULL,false);
}

static void CVMenuSnapOutlines(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.snapoutlines = cv->snapoutlines = !cv->snapoutlines;
    SavePrefs(true);
    GDrawRequestExpose(cv->v,NULL,false);
}

static void CVMenuShowHints(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    switch ( mi->mid ) {
      case MID_ShowHHints:
	CVShows.showhhints = cv->showhhints = !cv->showhhints;
	cv->back_img_out_of_date = true;	/* only this cv */
      break;
      case MID_ShowVHints:
	CVShows.showvhints = cv->showvhints = !cv->showvhints;
	cv->back_img_out_of_date = true;	/* only this cv */
      break;
      case MID_ShowDHints:
	CVShows.showdhints = cv->showdhints = !cv->showdhints;
	cv->back_img_out_of_date = true;	/* only this cv */
      break;
      case MID_ShowBlueValues:
	CVShows.showblues = cv->showblues = !cv->showblues;
	cv->back_img_out_of_date = true;	/* only this cv */
      break;
      case MID_ShowFamilyBlues:
	CVShows.showfamilyblues = cv->showfamilyblues = !cv->showfamilyblues;
	cv->back_img_out_of_date = true;	/* only this cv */
      break;
      case MID_ShowAnchors:
	CVShows.showanchor = cv->showanchor = !cv->showanchor;
      break;
      case MID_ShowHMetrics:
	CVShows.showhmetrics = cv->showhmetrics = !cv->showhmetrics;
      break;
      case MID_ShowVMetrics:
	CVShows.showvmetrics = cv->showvmetrics = !cv->showvmetrics;
      break;
      case MID_ShowDebugChanges:
	CVShows.showdebugchanges = cv->showdebugchanges = !cv->showdebugchanges;
      break;
      default:
        IError("Unexpected call to CVMenuShowHints");
      break;
    }
    SavePrefs(true);
    GDrawRequestExpose(cv->v,NULL,false);
    /* !!!! In this interim version we should request an expose on cvlayers */
    /*  but that's private to cvpalettes, and later we won't need to */
}

static void _CVMenuShowHideRulers(CharView *cv) {
    GRect pos;

    CVShows.showrulers = cv->showrulers = !cv->showrulers;
    pos.y = cv->mbh+cv->charselectorh+cv->infoh;
    pos.x = 0;
    if ( cv->showrulers ) {
	cv->height -= cv->rulerh;
	cv->width -= cv->rulerh;
	pos.y += cv->rulerh;
	pos.x += cv->rulerh;
    } else {
	cv->height += cv->rulerh;
	cv->width += cv->rulerh;
    }
    cv->back_img_out_of_date = true;
    pos.width = cv->width; pos.height = cv->height;
    GDrawMoveResize(cv->v,pos.x,pos.y,pos.width,pos.height);
    GDrawSync(NULL);
    GDrawRequestExpose(cv->v,NULL,false);
    SavePrefs(true);
}

static void CVMenuShowHideRulers(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuShowHideRulers(cv);
}

static void CVMenuFill(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVShows.showfilled = cv->showfilled = !cv->showfilled;
    CVRegenFill(cv);
    GDrawRequestExpose(cv->v,NULL,false);
}

static struct cvshows* cvshowsCopyTo( struct cvshows* dst, CharView* src )
{
    dst->showfore = src->showfore;
    dst->showgrids = src->showgrids;
    dst->showhhints = src->showhhints;
    dst->showvhints = src->showvhints;
    dst->showdhints = src->showdhints;
    dst->showpoints = src->showpoints;
    dst->alwaysshowcontrolpoints = src->alwaysshowcontrolpoints;
    dst->showfilled = src->showfilled;
    dst->showrulers = src->showrulers;
    dst->showrounds = src->showrounds;
    dst->showmdx = src->showmdx;
    dst->showmdy = src->showmdy;
    dst->showhmetrics = src->showhmetrics;
    dst->showvmetrics = src->showvmetrics;
    dst->markextrema = src->markextrema;
    dst->markpoi = src->markpoi;
    dst->showblues = src->showblues;
    dst->showfamilyblues = src->showfamilyblues;
    dst->showanchor = src->showanchor;
    dst->showcpinfo = src->showcpinfo;
    dst->showtabs = src->showtabs;
    dst->showsidebearings = src->showsidebearings;
    dst->showrefnames = src->showrefnames;
    dst->snapoutlines = src->snapoutlines;
    dst->showalmosthvlines = src->showalmosthvlines;
    dst->showalmosthvcurves = src->showalmosthvcurves;
    dst->hvoffset = src->hvoffset;
    dst->checkselfintersects = src->checkselfintersects;
    dst->showdebugchanges = src->showdebugchanges;
    return dst;
}

static CharView* cvshowsCopyFrom( CharView* dst, struct cvshows* src )
{
    dst->showfore = src->showfore;
    dst->showgrids = src->showgrids;
    dst->showhhints = src->showhhints;
    dst->showvhints = src->showvhints;
    dst->showdhints = src->showdhints;
    dst->showpoints = src->showpoints;
    dst->alwaysshowcontrolpoints = src->alwaysshowcontrolpoints;
    dst->showfilled = src->showfilled;
    dst->showrulers = src->showrulers;
    dst->showrounds = src->showrounds;
    dst->showmdx = src->showmdx;
    dst->showmdy = src->showmdy;
    dst->showhmetrics = src->showhmetrics;
    dst->showvmetrics = src->showvmetrics;
    dst->markextrema = src->markextrema;
    dst->markpoi = src->markpoi;
    dst->showblues = src->showblues;
    dst->showfamilyblues = src->showfamilyblues;
    dst->showanchor = src->showanchor;
    dst->showcpinfo = src->showcpinfo;
    dst->showtabs = src->showtabs;
    dst->showsidebearings = src->showsidebearings;
    dst->showrefnames = src->showrefnames;
    dst->snapoutlines = src->snapoutlines;
    dst->showalmosthvlines = src->showalmosthvlines;
    dst->showalmosthvcurves = src->showalmosthvcurves;
    dst->hvoffset = src->hvoffset;
    dst->checkselfintersects = src->checkselfintersects;
    dst->showdebugchanges = src->showdebugchanges;
    return dst;
}

static void CVPreviewModeSet(GWindow gw, int checked ) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    if( checked == cv->inPreviewMode )
        return;

    cv->inPreviewMode = checked;
    if( checked ) {
        cvshowsCopyTo( &CVShowsPrevewToggleSavedState, cv );
        cv->showfore   = 1;
        cv->showgrids  = 0;
        cv->showhhints = 0;
        cv->showvhints = 0;
        cv->showdhints = 0;
        cv->showpoints = 0;
	cv->alwaysshowcontrolpoints = 0;
        cv->showfilled = 1;
        cv->showrounds = 0;
        cv->showanchor = 0;
        cv->showrefnames = 0;
        cv->showhmetrics = 0;
        cv->showvmetrics = 0;
    } else {
        cvshowsCopyFrom( cv, &CVShowsPrevewToggleSavedState );
    }
    CVRegenFill(cv);
    GDrawRequestExpose(cv->v,NULL,false);
}

static void CVMenuPreview(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    int checked = mi->ti.checked;

#if 0
    // This has the effect of breaking the button since cv_auto_goto and HaveModifiers tend to be false.
    // The purpose of these checks is unclear.
    if( !cv_auto_goto ) {
	if( !HaveModifiers )
	    return;
    }
#endif // 0

    CVPreviewModeSet(gw, checked);
}

static void CVMenuDraggingComparisonOutline(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e))
{
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    int checked = mi->ti.checked;
    SplinePointListFree(cv->p.pretransform_spl);
    cv->p.pretransform_spl = NULL;
    prefs_create_dragging_comparison_outline = checked;
    SavePrefs(true);
}


static void CVMenuShowGridFit(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( !hasFreeType() || cv->dv!=NULL )
	return;
    cv->show_ft_results_live_update = 0;
    CVFtPpemDlg(cv,false);
}

static void CVMenuShowGridFitLiveUpdate(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( !hasFreeType() || cv->dv!=NULL )
	return;
    cv->show_ft_results_live_update = 1;
    CVFtPpemDlg(cv,false);
}

static void CVMenuChangePointSize(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( !hasFreeType() || cv->dv!=NULL || !cv->show_ft_results )
return;

    if ( mi->mid==MID_GridFitOff ) {
	cv->show_ft_results = false;
	cv->show_ft_results_live_update = false;

	SplinePointListsFree(cv->b.gridfit); cv->b.gridfit = NULL;
	FreeType_FreeRaster(cv->raster); cv->raster = NULL;
	GDrawRequestExpose(cv->v,NULL,false);
    } else {
	if ( mi->mid==MID_Bigger ) {
	    ++cv->ft_pointsizex;
	    ++cv->ft_pointsizey;
	} else if ( mi->mid==MID_Smaller ) {
	    if ( cv->ft_pointsizex>1 )
		--cv->ft_pointsizex;
	    if ( cv->ft_pointsizey>1 )
		--cv->ft_pointsizey;
	}

	if ( mi->mid==MID_GridFitAA )
	    cv->ft_depth = cv->ft_depth==8 ? 1 : 8;
	cv->ft_ppemx = rint(cv->ft_pointsizex*cv->ft_dpi/72.0);
	cv->ft_ppemy = rint(cv->ft_pointsizey*cv->ft_dpi/72.0);
	CVGridFitChar(cv);
    }
    SCRefreshTitles(cv->b.sc);
}

static void CVMenuEditInstrs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SCEditInstructions(cv->b.sc);
}

static void CVMenuDebug(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( !hasFreeTypeDebugger())
return;
    CVFtPpemDlg(cv,true);
}

static void CVMenuDeltas(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( !hasFreeTypeDebugger())
return;
    DeltaSuggestionDlg(NULL,cv);
}

static void CVMenuClearInstrs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( cv->b.sc->ttf_instrs_len!=0 ) {
	free(cv->b.sc->ttf_instrs);
	cv->b.sc->ttf_instrs = NULL;
	cv->b.sc->ttf_instrs_len = 0;
	cv->b.sc->instructions_out_of_date = false;
	SCCharChangedUpdate(cv->b.sc,ly_none);
	SCMarkInstrDlgAsChanged(cv->b.sc);
	cv->b.sc->complained_about_ptnums = false;	/* Should be after CharChanged */
    }
}

static void CVMenuKernPairs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SFShowKernPairs(cv->b.sc->parent,cv->b.sc,NULL,CVLayer((CharViewBase *) cv));
}

static void CVMenuLigatures(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SFShowLigatures(cv->b.fv->sf,cv->b.sc);
}

static void CVMenuAnchorPairs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SFShowKernPairs(cv->b.sc->parent,cv->b.sc,(AnchorClass *) (-1),CVLayer((CharViewBase *) cv));
}

static void CVMenuAPDetach(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv->apmine = cv->apmatch = NULL;
    cv->apsc = NULL;
    GDrawRequestExpose(cv->v,NULL,false);
}

static void CVMenuAPAttachSC(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    enum anchor_type type;
    AnchorPoint *ap;
    AnchorClass *ac;

    for ( ap = cv->b.sc->anchor; ap!=NULL && !ap->selected; ap=ap->next );
    if ( ap==NULL )
	ap = cv->b.sc->anchor;
    if ( ap==NULL )
return;
    type = ap->type;
    cv->apmine = ap;
    ac = ap->anchor;
    cv->apsc = mi->ti.userdata;
    for ( ap = cv->apsc->anchor; ap!=NULL ; ap=ap->next ) {
	if ( ap->anchor==ac &&
		((type==at_centry && ap->type==at_cexit) ||
		 (type==at_cexit && ap->type==at_centry) ||
		 (type==at_mark && ap->type!=at_mark) ||
		 ((type==at_basechar || type==at_baselig || type==at_basemark) && ap->type==at_mark)) )
    break;
    }
    cv->apmatch = ap;
    GDrawRequestExpose(cv->v,NULL,false);
}

static SplineChar **GlyphsMatchingAP(SplineFont *sf, AnchorPoint *ap) {
    SplineChar *sc;
    AnchorClass *ac = ap->anchor;
    enum anchor_type type = ap->type;
    int i, k, gcnt;
    SplineChar **glyphs;

    glyphs = NULL;
    for ( k=0; k<2; ++k ) {
	gcnt = 0;
	for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    for ( ap = sc->anchor; ap!=NULL; ap=ap->next ) {
		if ( ap->anchor == ac &&
			((type==at_centry && ap->type==at_cexit) ||
			 (type==at_cexit && ap->type==at_centry) ||
			 (type==at_mark && ap->type!=at_mark) ||
			 ((type==at_basechar || type==at_baselig || type==at_basemark) && ap->type==at_mark)) )
	     break;
	     }
	     if ( ap!=NULL ) {
		 if ( k )
		     glyphs[gcnt] = sc;
		 ++gcnt;
	     }
	 }
	 if ( !k ) {
	     if ( gcnt==0 )
return( NULL );
	     glyphs = malloc((gcnt+1)*sizeof(SplineChar *));
	 } else
	     glyphs[gcnt] = NULL;
     }
return( glyphs );
}

static void _CVMenuChangeChar(CharView *cv, int mid) {
    SplineFont *sf = cv->b.sc->parent;
    int pos = -1;
    int gid;
    EncMap *map = cv->b.fv->map;
    Encoding *enc = map->enc;

    if ( cv->b.container!=NULL ) {
	if ( cv->b.container->funcs->doNavigate!=NULL && mid != MID_Former)
	    (cv->b.container->funcs->doNavigate)(cv->b.container,
		    mid==MID_Next ? nt_next :
		    mid==MID_Prev ? nt_prev :
		    mid==MID_NextDef ? nt_next :
		    /* mid==MID_PrevDef ?*/ nt_prev);
return;
    }

    int curenc = CVCurEnc(cv);
    
    if( cv->charselector )
    {
        char* txt = GGadgetGetTitle8( cv->charselector );
        if( txt && strlen(txt) > 1 )
        {
            int offset = 1;
            if ( mid == MID_Prev )
                offset = -1;

	    unichar_t *txtu = GGadgetGetTitle( cv->charselector );
            unichar_t* r = Wordlist_advanceSelectedCharsBy( cv->b.sc->parent,
                                                            ((FontView *) (cv->b.fv))->b.map,
                                                            txtu, offset );
            free( txtu );

	    GGadgetSetTitle( cv->charselector, r );
            // Force any extra chars to be setup and drawn
            GEvent e;
            e.type=et_controlevent;
            e.u.control.subtype = et_textchanged;
            e.u.control.u.tf_changed.from_pulldown = 0;
            CV_OnCharSelectorTextChanged( cv->charselector, &e );
            return;
        }
    }
    
    if ( mid == MID_Next ) {
	pos = curenc+1;
    } else if ( mid == MID_Prev ) {
	pos = curenc-1;
    } else if ( mid == MID_NextDef ) {
	for ( pos = CVCurEnc(cv)+1; pos<map->enccount &&
		((gid=map->map[pos])==-1 || !SCWorthOutputting(sf->glyphs[gid])); ++pos );
	if ( pos>=map->enccount ) {
	    if ( enc->is_tradchinese ) {
		if ( strstrmatch(enc->enc_name,"hkscs")!=NULL ) {
		    if ( CVCurEnc(cv)<0x8140 )
			pos = 0x8140;
		} else {
		    if ( CVCurEnc(cv)<0xa140 )
			pos = 0xa140;
		}
	    } else if ( CVCurEnc(cv)<0x8431 && strstrmatch(enc->enc_name,"johab")!=NULL )
		pos = 0x8431;
	    else if ( CVCurEnc(cv)<0xa1a1 && strstrmatch(enc->iconv_name?enc->iconv_name:enc->enc_name,"EUC")!=NULL )
		pos = 0xa1a1;
	    else if ( CVCurEnc(cv)<0x8140 && strstrmatch(enc->enc_name,"sjis")!=NULL )
		pos = 0x8140;
	    else if ( CVCurEnc(cv)<0xe040 && strstrmatch(enc->enc_name,"sjis")!=NULL )
		pos = 0xe040;
	    if ( pos>=map->enccount )
return;
	}
    } else if ( mid == MID_PrevDef ) {
	for ( pos = CVCurEnc(cv)-1; pos>=0 &&
		((gid=map->map[pos])==-1 || !SCWorthOutputting(sf->glyphs[gid])); --pos );
	if ( pos<0 )
return;
    } else if ( mid == MID_Former ) {
	if ( cv->former_cnt<=1 )
return;
	for ( gid=sf->glyphcnt-1; gid>=0; --gid )
	    if ( sf->glyphs[gid]!=NULL &&
		    strcmp(sf->glyphs[gid]->name,cv->former_names[1])==0 )
	break;
	if ( gid<0 )
return;
	pos = map->backmap[gid];
    }
    /* Werner doesn't think it should wrap */
    if ( pos<0 ) /* pos = map->enccount-1; */
return;
    else if ( pos>= map->enccount ) /* pos = 0; */
return;

    if ( pos>=0 && pos<map->enccount )
	CVChangeChar(cv,pos);
}


static void CVMenuChangeChar(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuChangeChar(cv,mi->mid);
}

static int CVMoveToNextInWordList(GGadget *g, GEvent *e)
{
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
        CharView *cv = GDrawGetUserData(GGadgetGetWindow(g));
        CVMoveInWordListByOffset( cv, 1 );
    }
    return 1;
}
static int CVMoveToPrevInWordList(GGadget *g, GEvent *e)
{
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
        CharView *cv = GDrawGetUserData(GGadgetGetWindow(g));
        CVMoveInWordListByOffset( cv, -1 );
    }
    return 1;
}

/**
 * If the prev/next BCP is selected than add those to the hash table
 * at "ret".
 */
static void getSelectedControlPointsVisitor(SplinePoint* splfirst, Spline* s, SplinePoint* sp, void* udata )
{
    GHashTable* ret = (GHashTable*)udata;
    if( sp->nextcpselected )
	g_hash_table_insert( ret, sp, 0 );
    if( sp->prevcpselected )
	g_hash_table_insert( ret, sp, 0 );
}
static void getAllControlPointsVisitor(SplinePoint* splfirst, Spline* s, SplinePoint* sp, void* udata )
{
    GHashTable* ret = (GHashTable*)udata;
    g_hash_table_insert( ret, sp, 0 );
    g_hash_table_insert( ret, sp, 0 );
}

/**
 * Get a hash table with all the selected BCP in it.
 *
 * The caller must call g_hash_table_destroy() on the return value.
 */
static GHashTable* getSelectedControlPoints( CharView *cv, PressedOn *p )
{
    Layer* l = cv->b.layerheads[cv->b.drawmode];
    if( !l || !l->splines )
	return 0;

    GHashTable* ret = g_hash_table_new( g_direct_hash, g_direct_equal );
    SplinePointList* spl = l->splines;
    for( ; spl; spl = spl->next )
    {
	SPLFirstVisitPoints( spl->first, getSelectedControlPointsVisitor, ret );
    }

    return ret;
}
static GHashTable* getAllControlPoints( CharView *cv, PressedOn *p )
{
    Layer* l = cv->b.layerheads[cv->b.drawmode];
    if( !l || !l->splines )
	return 0;

    GHashTable* ret = g_hash_table_new( g_direct_hash, g_direct_equal );
    SplinePointList* spl = l->splines;
    for( ; spl; spl = spl->next )
    {
	SPLFirstVisitPoints( spl->first, getAllControlPointsVisitor, ret );
    }
    return ret;
}

void FE_touchControlPoint( void* key,
			   void* value,
			   SplinePoint* sp,
			   BasePoint *which,
			   bool isnext,
			   void* udata )
{
    TRACE("FE_touchControlPoint() which:%p\n", which );
    SPTouchControl( sp, which, (int)(intptr_t)udata );
}

void FE_unselectBCP( void* key,
		     void* value,
		     SplinePoint* sp,
		     BasePoint *which,
		     bool isnext,
		     void* udata )
{
    sp->nextcpselected = 0;
    sp->prevcpselected = 0;
}

void FE_adjustBCPByDelta( void* key,
			  void* value,
			  SplinePoint* sp,
			  BasePoint *which,
			  bool isnext,
			  void* udata )
{
    FE_adjustBCPByDeltaData* data = (FE_adjustBCPByDeltaData*)udata;
    CharView *cv = data->cv;

    TRACE("FE_adjustBCPByDelta %p %d\n", which, isnext );
    BasePoint to;
    to.x = which->x + data->dx;
    to.y = which->y + data->dy;
    SPAdjustControl(sp,which,&to,cv->b.layerheads[cv->b.drawmode]->order2);
    CVSetCharChanged(cv,true);
}

void FE_adjustBCPByDeltaWhilePreservingBCPAngle( void* key,
						 void* value,
						 SplinePoint* sp,
						 BasePoint *which,
						 bool isnext,
						 void* udata )
{
    FE_adjustBCPByDeltaData* data = (FE_adjustBCPByDeltaData*)udata;
    CharView *cv = data->cv;

//    TRACE("FE_adjustBCPByDeltaWhilePreservingBCPAngle which:%p data:%p isnext:%d\n", which, data, isnext );
//    TRACE("FE_adjustBCPByDeltaWhilePreservingBCPAngle    delta %f %f\n", data->dx, data->dy );
    BasePoint to;
    to.x = which->x + data->dx;
    to.y = which->y + data->dy;

    // work out near and far BCP for this movement
    BasePoint* near = &sp->nextcp;
    BasePoint* far  = &sp->prevcp;
    if( !isnext )
    {
	near = &sp->prevcp;
	far  = &sp->nextcp;
    }

    real dx = near->x - far->x;
    if( !fabs(dx) )
    {
	// flatline protection
	to.x = which->x + data->dx;
	to.y = which->y + data->dy;
    }
    else
    {
	// we have a workable gradient
	real m = (near->y - far->y) / dx;

	// should we lean to x or y for the change?
	// all this is based on m = y2-y1/x2-x1
	// we use m from near and far and extrapolate to the new location
	// based on either x or y and calculate the other ordinate from the
	// gradiant function above.
	if( fabs(m) < 1.0 )
	{
	    real datadx = data->dx;
	    if( data->keyboarddx && !datadx )
	    {
		datadx = data->dy;
		if( m < 0 )
		    datadx *= -1;
	    }
	    
	    real dx = near->x - far->x;
	    real m = (near->y - far->y) / dx;
	    to.x = which->x + datadx;
	    to.y = m * (to.x - near->x) + near->y;
	}
	else
	{
	    real datady = data->dy;
	    if( data->keyboarddx && !datady )
	    {
		datady = data->dx;
		if( m < 0 )
		    datady *= -1;
	    }
	    real dx = near->x - far->x;
	    real m = (near->y - far->y) / dx;
	    to.y = which->y + datady;
	    to.x = (to.y - near->y + m*near->x) / m;
	}
    }

    // move the point and update
    SPAdjustControl(sp,which,&to,cv->b.layerheads[cv->b.drawmode]->order2);
    CVSetCharChanged(cv,true);
}

/**
 * Container for arguments to FE_visitSelectedControlPoints.
 */
typedef struct visitSelectedControlPoints_CallbackDataS
{
    int count;                              // number of times visitor is called.
    int sel;
    visitSelectedControlPointsVisitor func; // Visitor function to delegate to
    gpointer udata;                         // user data to use when calling above func()

} visitSelectedControlPoints_CallbackData;

/**
 * Visitor function: calls a delegate visitor function for any prev
 * and next BCP which are selected for each spline point. This is a
 * handy visitor when your BCP handling code for the most part doesn't
 * care if it will operate on the next or prev BCP, ie your visitor
 * simply wants to visit all the selected BCP.
 */
static void FE_visitSelectedControlPoints( gpointer key,
					   gpointer value,
					   gpointer udata )
{
    visitSelectedControlPoints_CallbackData* d = (visitSelectedControlPoints_CallbackData*)udata;
    SplinePoint* sp = (SplinePoint*)key;

    d->count++;
    if( sp->nextcpselected )
    {
	BasePoint *which = &sp->nextcp;
	d->func( key, value, sp, which, true, d->udata );
    }
    if( sp->prevcpselected )
    {
	BasePoint *which = &sp->prevcp;
	d->func( key, value, sp, which, false, d->udata );
    }
}
static void FE_visitAllControlPoints( gpointer key,
				      gpointer value,
				      gpointer udata )
{
    visitSelectedControlPoints_CallbackData* d = (visitSelectedControlPoints_CallbackData*)udata;
    SplinePoint* sp = (SplinePoint*)key;

    d->count++;
    {
	BasePoint *which = &sp->nextcp;
	d->func( key, value, sp, which, true, d->udata );
    }
    {
	BasePoint *which = &sp->prevcp;
	d->func( key, value, sp, which, false, d->udata );
    }
}
static void FE_visitAdjacentToSelectedControlPoints( gpointer key,
						     gpointer value,
						     gpointer udata )
{
    visitSelectedControlPoints_CallbackData* d = (visitSelectedControlPoints_CallbackData*)udata;
    SplinePoint* sp = (SplinePoint*)key;

    if( sp->selected )
	return;

    d->count++;
    if( sp->prev && sp->prev->from && sp->prev->from->selected )
    {
	d->func( key, value, sp, &sp->nextcp, true, d->udata );
	d->func( key, value, sp, &sp->prevcp, true, d->udata );
    }
    if( sp->next && sp->next->to && sp->next->to->selected )
    {
	d->func( key, value, sp, &sp->nextcp, true, d->udata );
	d->func( key, value, sp, &sp->prevcp, true, d->udata );
    }
}

void visitSelectedControlPoints( GHashTable *col, visitSelectedControlPointsVisitor f, gpointer udata )
{
    visitSelectedControlPoints_CallbackData d;
    d.func = f;
    d.udata = udata;
    d.count = 0;
    g_hash_table_foreach( col, FE_visitSelectedControlPoints, &d );
}

void visitAllControlPoints( GHashTable *col, visitSelectedControlPointsVisitor f, gpointer udata )
{
    visitSelectedControlPoints_CallbackData d;
    d.func = f;
    d.udata = udata;
    d.count = 0;
    g_hash_table_foreach( col, FE_visitAllControlPoints, &d );
}
static void visitAdjacentToSelectedControlPoints( GHashTable *col, visitSelectedControlPointsVisitor f, gpointer udata )
{
    visitSelectedControlPoints_CallbackData d;
    d.func = f;
    d.udata = udata;
    d.count = 0;
    g_hash_table_foreach( col, FE_visitAdjacentToSelectedControlPoints, &d );
}

void CVFindAndVisitSelectedControlPoints( CharView *cv, bool preserveState,
					  visitSelectedControlPointsVisitor f, void* udata )
{
//    TRACE("CVFindAndVisitSelectedControlPoints(top) cv->p.sp:%p\n", cv->p.sp );
    GHashTable* col = getSelectedControlPoints( cv, &cv->p );
    if(!col)
	return;
    
    if( g_hash_table_size( col ) )
    {
	if( preserveState )
	    CVPreserveState(&cv->b);
	visitSelectedControlPoints( col, f, udata );
    }
    g_hash_table_destroy(col);
}

void CVVisitAllControlPoints( CharView *cv, bool preserveState,
			      visitSelectedControlPointsVisitor f, void* udata )
{
    TRACE("CVVisitAllControlPoints(top) cv->p.spl:%p cv->p.sp:%p\n", cv->p.spl, cv->p.sp );
    if( !cv->p.spl || !cv->p.sp )
	return;

    GHashTable* col = getAllControlPoints( cv, &cv->p );
    if( g_hash_table_size( col ) )
    {
	if( preserveState )
	    CVPreserveState(&cv->b);
	visitAllControlPoints( col, f, udata );
    }
    g_hash_table_destroy(col);
}

void CVVisitAdjacentToSelectedControlPoints( CharView *cv, bool preserveState,
					     visitSelectedControlPointsVisitor f, void* udata )
{
//    TRACE("CVVisitAdjacentToSelectedControlPoints(top) cv->p.sp:%p\n", cv->p.sp );
    if( !cv->p.spl || !cv->p.sp )
	return;

    GHashTable* col = getAllControlPoints( cv, &cv->p );
    if( !col )
	return;

    if( g_hash_table_size( col ) )
    {
	if( preserveState )
	    CVPreserveState(&cv->b);
	visitAdjacentToSelectedControlPoints( col, f, udata );
    }
    g_hash_table_destroy(col);
}



void CVChar(CharView *cv, GEvent *event ) {
    extern float arrowAmount, arrowAccelFactor;
    extern int navigation_mask;

    if( !cv_auto_goto )
    {
	if( event->u.chr.keysym == GK_Control_L
	    || event->u.chr.keysym == GK_Control_R )
	{
	    HaveModifiers = 1;
	}
	bool isImmediateKeyTogglePreview = isImmediateKey( cv->gw, "TogglePreview", event ) != NULL;

	if( !HaveModifiers && isImmediateKeyTogglePreview ) {
	    PressingTilde = 1;
	    CVPreviewModeSet( cv->gw, true );
	    return;
	}
    }

    /* TRACE("GK_Control_L:%d\n", ( event->u.chr.keysym == GK_Control_L )); */
    /* TRACE("GK_Meta_L:%d\n", ( event->u.chr.keysym == GK_Meta_L )); */
    
    int oldactiveModifierControl = cv->activeModifierControl;
    int oldactiveModifierAlt = cv->activeModifierAlt;
    cv->activeModifierControl |= ( event->u.chr.keysym == GK_Control_L || event->u.chr.keysym == GK_Control_R
				   || event->u.chr.keysym == GK_Meta_L || event->u.chr.keysym == GK_Meta_R );
    cv->activeModifierAlt     |= ( event->u.chr.keysym == GK_Alt_L || event->u.chr.keysym == GK_Alt_R
				   || event->u.chr.keysym == GK_Mode_switch );

    if( oldactiveModifierControl != cv->activeModifierControl
	|| oldactiveModifierAlt != cv->activeModifierAlt )
    {
	CVInfoDraw(cv,cv->gw);
    }
    
    
#if _ModKeysAutoRepeat
	/* Under cygwin these keys auto repeat, they don't under normal X */
	if ( cv->autorpt!=NULL ) {
	    GDrawCancelTimer(cv->autorpt); cv->autorpt = NULL;
	    if ( cv->keysym == event->u.chr.keysym )	/* It's an autorepeat, ignore it */
return;
	    CVToolsSetCursor(cv,cv->oldstate,NULL);
	}
#endif

#if MyMemory
    if ( event->u.chr.keysym == GK_F2 ) {
	fprintf( stderr, "Malloc debug on\n" );
	__malloc_debug(5);
    } else if ( event->u.chr.keysym == GK_F3 ) {
	fprintf( stderr, "Malloc debug off\n" );
	__malloc_debug(0);
    }
#endif

    if ( !HaveModifiers && event->u.chr.keysym==' ' && cv->spacebar_hold==0 ) {
	cv->p.x = event->u.mouse.x;
	cv->p.y = event->u.mouse.y;
	update_spacebar_hand_tool(cv);
    }

    CVPaletteActivate(cv);
    CVToolsSetCursor(cv,TrueCharState(event),NULL);

    
	/* The window check is to prevent infinite loops since DVChar can */
	/*  call CVChar too */
    if ( cv->dv!=NULL && (event->w==cv->gw || event->w==cv->v) && DVChar(cv->dv,event))
    {
	/* All Done */;
    }
    else if ( event->u.chr.keysym=='s' &&
	    (event->u.chr.state&ksm_control) &&
	    (event->u.chr.state&ksm_meta) )
	MenuSaveAll(NULL,NULL,NULL);
    else if ( event->u.chr.keysym=='q' &&
	    (event->u.chr.state&ksm_control) &&
	    (event->u.chr.state&ksm_meta) )
	MenuExit(NULL,NULL,NULL);
    else if ( event->u.chr.keysym == GK_Shift_L || event->u.chr.keysym == GK_Shift_R ||
	     event->u.chr.keysym == GK_Alt_L || event->u.chr.keysym == GK_Alt_R ||
	     event->u.chr.keysym == GK_Meta_L || event->u.chr.keysym == GK_Meta_R ) {
	CVFakeMove(cv, event);
    } else if (( event->u.chr.keysym == GK_Tab || event->u.chr.keysym == GK_BackTab) &&
	    (event->u.chr.state&ksm_control) && cv->showtabs && cv->former_cnt>1 ) {
	GGadgetDispatchEvent(cv->tabs,event);
    } else if ( (event->u.chr.state&ksm_meta) &&
	    !(event->u.chr.state&(ksm_control|ksm_shift)) &&
	    event->u.chr.chars[0]!='\0' ) {
	CVPaletteMnemonicCheck(event);
    } else if ( !(event->u.chr.state&(ksm_control|ksm_meta)) &&
	    event->u.chr.keysym == GK_BackSpace ) {
	/* Menu does delete */
	CVClear(cv->gw,NULL,NULL);
    } else if ( event->u.chr.keysym == GK_Help ) {
	MenuHelp(NULL,NULL,NULL);	/* Menu does F1 */
    } else if ( event->u.chr.keysym=='<' && (event->u.chr.state&ksm_control) ) {
	/* European keyboards do not need shift to get < */
	CVDoFindInFontView(cv);
    } else if ( (event->u.chr.keysym=='[' || event->u.chr.keysym==']') &&
	    (event->u.chr.state&ksm_control) ) {
	/* European keyboards need a funky modifier to get [] */
	_CVMenuChangeChar(cv,event->u.chr.keysym=='[' ? MID_Prev : MID_Next );
    } else if ( (event->u.chr.keysym=='{' || event->u.chr.keysym=='}') &&
	    (event->u.chr.state&ksm_control) ) {
	/* European keyboards need a funky modifier to get {} */
	_CVMenuChangeChar(cv,event->u.chr.keysym=='{' ? MID_PrevDef : MID_NextDef );
    } else if ( event->u.chr.keysym=='\\' && (event->u.chr.state&ksm_control) ) {
	/* European keyboards need a funky modifier to get \ */
	CVDoTransform(cv,cvt_none);
    } else if ( (event->u.chr.keysym=='F' || event->u.chr.keysym=='B') &&
            !(event->u.chr.state&(ksm_control|ksm_meta)) ) {
        CVLSelectLayer(cv, event->u.chr.keysym=='F' ? 1 : 0);
    } else if ( (event->u.chr.state&ksm_control) && (event->u.chr.keysym=='-' || event->u.chr.keysym==0xffad/*XK_KP_Subtract*/) ){
	    _CVMenuScale(cv, MID_ZoomOut);
    } else if ( (event->u.chr.state&ksm_control) && (event->u.chr.keysym=='=' || event->u.chr.keysym==0xffab/*XK_KP_Add*/) ){
	    _CVMenuScale(cv, MID_ZoomIn);
    }
    else if ( event->u.chr.keysym == GK_Left ||
	    event->u.chr.keysym == GK_Up ||
	    event->u.chr.keysym == GK_Right ||
	    event->u.chr.keysym == GK_Down ||
	    event->u.chr.keysym == GK_KP_Left ||
	    event->u.chr.keysym == GK_KP_Up ||
	    event->u.chr.keysym == GK_KP_Right ||
	    event->u.chr.keysym == GK_KP_Down )
    {
	TRACE("key left/right/up/down...\n");
	
	GGadget *active = GWindowGetFocusGadgetOfWindow(cv->gw);
	if( active == cv->charselector )
	{
	    if ( event->u.chr.keysym == GK_Left ||event->u.chr.keysym == GK_Right )
	    {
		TRACE("left/right on the charselector!\n");
	    }
	    int dir = ( event->u.chr.keysym == GK_Up || event->u.chr.keysym==GK_KP_Up ) ? -1 : 1;
	    Wordlist_MoveByOffset( cv->charselector, &cv->charselectoridx, dir );

	    return;
	}
	
	
	real dx=0, dy=0; int anya;
	switch ( event->u.chr.keysym ) {
	  case GK_Left: case GK_KP_Left:
	    dx = -1;
	  break;
	  case GK_Right: case GK_KP_Right:
	    dx = 1;
	  break;
	  case GK_Up: case GK_KP_Up:
	    dy = 1;
	  break;
	  case GK_Down: case GK_KP_Down:
	    dy = -1;
	  break;
	}
	if ( event->u.chr.state & (ksm_control|ksm_capslock) ) {
	    struct sbevent sb;
	    sb.type = dy>0 || dx<0 ? et_sb_halfup : et_sb_halfdown;
	    if ( dx==0 )
		CVVScroll(cv,&sb);
	    else
		CVHScroll(cv,&sb);
	}
	else
	{
//	    TRACE("cvchar( moving points? ) shift:%d\n", ( event->u.chr.state & (ksm_shift) ));
	    FE_adjustBCPByDeltaData d;
	    memset( &d, 0, sizeof(FE_adjustBCPByDeltaData));
	    visitSelectedControlPointsVisitor func = FE_adjustBCPByDelta;
	    if ( event->u.chr.state & ksm_meta )
	    {
		// move the bcp 1 unit in the direction it already has
		func = FE_adjustBCPByDeltaWhilePreservingBCPAngle;
		// allow that func to work it's magic on any gradient
		d.keyboarddx = 1;
	    }
	    /* if ( event->u.chr.state & (ksm_shift) ) */
	    /* 	dx -= dy*tan((cv->b.sc->parent->italicangle)*(FF_PI/180) ); */
	    if ( event->u.chr.state & (ksm_shift) )
	    {
		dx *= arrowAccelFactor; dy *= arrowAccelFactor;
	    }

	    if ((  cv->p.sp!=NULL || cv->lastselpt!=NULL ) &&
		    (cv->p.nextcp || cv->p.prevcp) )
	    {
		// This code moves 1 or more BCP

		SplinePoint *old = cv->p.sp;
		d.cv = cv;
		d.dx = dx * arrowAmount;
		d.dy = dy * arrowAmount;
		CVFindAndVisitSelectedControlPoints( cv, true,
						     func, &d );
		cv->p.sp = old;
		SCUpdateAll(cv->b.sc);

	    }
	    else if ( CVAnySel(cv,NULL,NULL,NULL,&anya) || cv->widthsel || cv->vwidthsel )
	    {
		CVPreserveState(&cv->b);
		CVMoveSelection(cv,dx*arrowAmount,dy*arrowAmount, event->u.chr.state);
		if ( cv->widthsel )
		    SCSynchronizeWidth(cv->b.sc,cv->b.sc->width,cv->b.sc->width-dx,NULL);
		_CV_CharChangedUpdate(cv,2);
		CVInfoDraw(cv,cv->gw);
	    }
	    CVGridHandlePossibleFitChar( cv );
	}
    } else if ( event->u.chr.keysym == GK_Page_Up ||
	    event->u.chr.keysym == GK_KP_Page_Up ||
	    event->u.chr.keysym == GK_Prior ||
	    event->u.chr.keysym == GK_Page_Down ||
	    event->u.chr.keysym == GK_KP_Page_Down ||
	    event->u.chr.keysym == GK_Next ) {
	/* Um... how do we scroll horizontally??? */
	struct sbevent sb;
	sb.type = et_sb_uppage;
	if ( event->u.chr.keysym == GK_Page_Down ||
		event->u.chr.keysym == GK_KP_Page_Down ||
		event->u.chr.keysym == GK_Next )
	    sb.type = et_sb_downpage;
	CVVScroll(cv,&sb);
    } else if ( event->u.chr.keysym == GK_Home ) {
	CVFit(cv);
    } else if ( event->u.chr.keysym==' ' && cv->spacebar_hold ){
    } else if ( (event->u.chr.state&((GMenuMask()|navigation_mask)&~(ksm_shift|ksm_capslock)))==navigation_mask &&
	    event->type == et_char &&
	    event->u.chr.keysym!=0 &&
	    (event->u.chr.keysym<GK_Special /*|| event->u.chr.keysym>=0x10000*/)) {
	SplineFont *sf = cv->b.sc->parent;
	int i;
	EncMap *map = cv->b.fv->map;
	extern int cv_auto_goto;
	if ( cv_auto_goto ) {
	    i = SFFindSlot(sf,map,event->u.chr.keysym,NULL);
	    if ( i!=-1 )
		CVChangeChar(cv,i);
	}
    }
}

void CVShowPoint(CharView *cv, BasePoint *me) {
    CharViewTab* tab = CVGetActiveTab(cv);
    int x, y;
    int fudge = 30;

    if ( cv->width<60 )
	fudge = cv->width/3;
    if ( cv->height<60 && fudge>cv->height/3 )
	fudge = cv->height/3;

    /* Make sure the point is visible and has some context around it */
    x =  tab->xoff + rint(me->x*tab->scale);
    y = -tab->yoff + cv->height - rint(me->y*tab->scale);
    if ( x<fudge || y<fudge || x>cv->width-fudge || y>cv->height-fudge )
	CVMagnify(cv,me->x,me->y,0,0);
}

static void CVSelectContours(CharView *cv) {
    SplineSet *spl;
    SplinePoint *sp;
    int sel;
    int i;

    for ( spl=cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl=spl->next ) {
	sel = false;
	if ( cv->b.sc->inspiro && hasspiro()) {
	    for ( i=0; i<spl->spiro_cnt-1; ++i ) {
		if ( SPIRO_SELECTED(&spl->spiros[i]) ) {
		    sel = true;
	    break;
		}
	    }
	    if ( sel ) {
		for ( i=0; i<spl->spiro_cnt-1; ++i )
		    SPIRO_SELECT(&spl->spiros[i]);
	    }
	} else {
	    for ( sp=spl->first ; ; ) {
		if ( sp->selected ) {
		    sel = true;
	    break;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	    if ( sel ) {
		for ( sp=spl->first ; ; ) {
		    sp->selected = true;
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		    if ( sp==spl->first )
		break;
		}
	    }
	}
    }
    SCUpdateAll(cv->b.sc);
}

static void CVMenuSelectContours(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVSelectContours(cv);
}

static void CVMenuSelectPointAt(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVSelectPointAt(cv);
}

static void CVNextPrevSpiroPt(CharView *cv, struct gmenuitem *mi) {
    CharViewTab* tab = CVGetActiveTab(cv);
    RefChar *r; ImageList *il;
    SplineSet *spl, *ss;
    SplinePoint *junk;
    int x, y;
    spiro_cp *selcp = NULL, *other;
    int index;

    if ( mi->mid == MID_FirstPt ) {
	if ( cv->b.layerheads[cv->b.drawmode]->splines==NULL )
return;
	CVClearSel(cv);
	other = &cv->b.layerheads[cv->b.drawmode]->splines->spiros[0];
    } else {
	if ( !CVOneThingSel(cv,&junk,&spl,&r,&il,NULL,&selcp) || spl==NULL )
return;
	other = selcp;
	if ( spl==NULL )
return;
	index = selcp - spl->spiros;
	if ( mi->mid == MID_NextPt ) {
	    if ( index!=spl->spiro_cnt-2 )
		other = &spl->spiros[index+1];
	    else {
		if ( spl->next == NULL )
		    spl = cv->b.layerheads[cv->b.drawmode]->splines;
		else
		    spl = spl->next;
		other = &spl->spiros[0];
	    }
	} else if ( mi->mid == MID_PrevPt ) {
	    if ( index!=0 ) {
		other = &spl->spiros[index-1];
	    } else {
		if ( spl==cv->b.layerheads[cv->b.drawmode]->splines ) {
		    for ( ss = cv->b.layerheads[cv->b.drawmode]->splines; ss->next!=NULL; ss=ss->next );
		} else {
		    for ( ss = cv->b.layerheads[cv->b.drawmode]->splines; ss->next!=spl; ss=ss->next );
		}
		spl = ss;
		other = &ss->spiros[ss->spiro_cnt-2];
	    }
	} else if ( mi->mid == MID_FirstPtNextCont ) {
	    if ( spl->next!=NULL )
		other = &spl->next->spiros[0];
	    else
		other = NULL;
	}
    }
    if ( selcp!=NULL )
	SPIRO_DESELECT(selcp);
    if ( other!=NULL )
	SPIRO_SELECT(other);
    cv->p.sp = NULL;
    cv->lastselpt = NULL;
    cv->lastselcp = other;

    /* Make sure the point is visible and has some context around it */
    if ( other!=NULL ) {
	x =  tab->xoff + rint(other->x*tab->scale);
	y = -tab->yoff + cv->height - rint(other->y*tab->scale);
	if ( x<40 || y<40 || x>cv->width-40 || y>cv->height-40 )
	    CVMagnify(cv,other->x,other->y,0,0);
    }

    CVInfoDraw(cv,cv->gw);
    SCUpdateAll(cv->b.sc);
}

static void CVNextPrevPt(CharView *cv, struct gmenuitem *mi) {
    CharViewTab* tab = CVGetActiveTab(cv);
    SplinePoint *selpt=NULL, *other;
    RefChar *r; ImageList *il;
    SplineSet *spl, *ss;
    int x, y;
    spiro_cp *junk;

    if ( cv->b.sc->inspiro && hasspiro()) {
	CVNextPrevSpiroPt(cv,mi);
return;
    }

    if ( mi->mid == MID_FirstPt ) {
	if ( cv->b.layerheads[cv->b.drawmode]->splines==NULL )
return;
	other = (cv->b.layerheads[cv->b.drawmode]->splines)->first;
	CVClearSel(cv);
    } else {
	if ( !CVOneThingSel(cv,&selpt,&spl,&r,&il,NULL,&junk) || spl==NULL )
return;
	other = selpt;
	if ( spl==NULL )
return;
	else if ( mi->mid == MID_NextPt ) {
	    if ( other->next!=NULL && other->next->to!=spl->first )
		other = other->next->to;
	    else {
		if ( spl->next == NULL )
		    spl = cv->b.layerheads[cv->b.drawmode]->splines;
		else
		    spl = spl->next;
		other = spl->first;
	    }
	} else if ( mi->mid == MID_PrevPt ) {
	    if ( other!=spl->first ) {
		other = other->prev->from;
	    } else {
		if ( spl==cv->b.layerheads[cv->b.drawmode]->splines ) {
		    for ( ss = cv->b.layerheads[cv->b.drawmode]->splines; ss->next!=NULL; ss=ss->next );
		} else {
		    for ( ss = cv->b.layerheads[cv->b.drawmode]->splines; ss->next!=spl; ss=ss->next );
		}
		spl = ss;
		other = ss->last;
		if ( spl->last==spl->first && spl->last->prev!=NULL )
		    other = other->prev->from;
	    }
	} else if ( mi->mid == MID_FirstPtNextCont ) {
	    if ( spl->next!=NULL )
		other = spl->next->first;
	    else
		other = NULL;
	}
    }
    if ( selpt!=NULL )
	selpt->selected = false;
    if ( other!=NULL )
	other->selected = true;
    cv->p.sp = NULL;
    cv->lastselpt = other;
    cv->p.spiro = cv->lastselcp = NULL;

    /* Make sure the point is visible and has some context around it */
    if ( other!=NULL ) {
	x =  tab->xoff + rint(other->me.x*tab->scale);
	y = -tab->yoff + cv->height - rint(other->me.y*tab->scale);
	if ( x<40 || y<40 || x>cv->width-40 || y>cv->height-40 )
	    CVMagnify(cv,other->me.x,other->me.y,0,0);
    }

    CVInfoDraw(cv,cv->gw);
    SCUpdateAll(cv->b.sc);
}

static void CVMenuNextPrevPt(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVNextPrevPt(cv, mi);
}

static void CVNextPrevCPt(CharView *cv, struct gmenuitem *mi) {
    SplinePoint *selpt=NULL;
    RefChar *r; ImageList *il;
    SplineSet *spl;
    spiro_cp *junk;

    if ( !CVOneThingSel(cv,&selpt,&spl,&r,&il,NULL,&junk))
return;
    if ( selpt==NULL )
return;
    cv->p.nextcp = mi->mid==MID_NextCP;
    cv->p.prevcp = mi->mid==MID_PrevCP;
    SCUpdateAll(cv->b.sc);
}

static void CVMenuNextPrevCPt(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVNextPrevCPt(cv, mi);
}

static void CVMenuGotoChar(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int pos;

    if ( cv->b.container ) {
	(cv->b.container->funcs->doNavigate)(cv->b.container,nt_goto);
return;
    }

    pos = GotoChar(cv->b.fv->sf,cv->b.fv->map,NULL);
    if ( pos!=-1 )
	CVChangeChar(cv,pos);
}

static void CVMenuFindInFontView(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVDoFindInFontView(cv);
}

static void CVMenuPalettesDock(GWindow UNUSED(gw), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    PalettesChangeDocking();
}

static void CVMenuPaletteShow(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVPaletteSetVisible(cv, mi->mid==MID_Tools, !CVPaletteIsVisible(cv, mi->mid==MID_Tools));
}

static void cv_pllistcheck(CharView *cv, struct gmenuitem *mi) {
    extern int palettes_docked;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Tools:
	    mi->ti.checked = CVPaletteIsVisible(cv,1);
	  break;
	  case MID_Layers:
	    mi->ti.checked = CVPaletteIsVisible(cv,0);
	  break;
	  case MID_DockPalettes:
	    mi->ti.checked = palettes_docked;
	  break;
	}
    }
}

static void pllistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_pllistcheck(cv, mi);
}

/*
 * Unused
static void tablistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    GDrawGetUserData(gw);
}
*/

static void CVUndo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVDoUndo(&cv->b);
    cv->lastselpt = NULL;
}

static void CVRedo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    CVDoRedo(&cv->b);
    cv->lastselpt = NULL;
}

static void _CVCopy(CharView *cv) {
    int desel = false, anya;

    /* If nothing is selected, copy everything. Do that by temporarily selecting everything */
    if ( !CVAnySel(cv,NULL,NULL,NULL,&anya))
	if ( !(desel = CVSetSel(cv,-1)))
return;
    CopySelected(&cv->b,cv->showanchor);
    if ( desel )
	CVClearSel(cv);
}

static void CVCopy(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVCopy(cv);
}

static void CVCopyLookupData(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SCCopyLookupData(cv->b.sc);
}

static void CVCopyRef(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CopyReference(cv->b.sc);
}

static void CVMenuCopyGridFit(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVCopyGridFit(&cv->b);
}

static void CVCopyWidth(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    if ( mi->mid==MID_CopyVWidth && !cv->b.sc->parent->hasvmetrics )
return;
    CopyWidth(&cv->b,mi->mid==MID_CopyWidth?ut_width:
		     mi->mid==MID_CopyVWidth?ut_vwidth:
		     mi->mid==MID_CopyLBearing?ut_lbearing:
					     ut_rbearing);
}

static void CVDoClear(CharView *cv) {
    ImageList *prev, *imgs, *next;
    RefChar *refs, *rnext;
    int layer = CVLayer((CharViewBase *) cv);
    int anyimages;

    CVPreserveState(&cv->b);
    if ( cv->b.drawmode==dm_fore )
	SCRemoveSelectedMinimumDistances(cv->b.sc,2);
    cv->b.layerheads[cv->b.drawmode]->splines = SplinePointListRemoveSelected(cv->b.sc,
	    cv->b.layerheads[cv->b.drawmode]->splines);
    for ( refs=cv->b.layerheads[cv->b.drawmode]->refs; refs!=NULL; refs = rnext ) {
	rnext = refs->next;
	if ( refs->selected )
	    SCRemoveDependent(cv->b.sc,refs,layer);
    }
    if ( cv->b.drawmode==dm_fore ) {
	AnchorPoint *ap, *aprev=NULL, *anext;
	if ( cv->showanchor ) for ( ap=cv->b.sc->anchor; ap!=NULL; ap=anext ) {
	    anext = ap->next;
	    if ( ap->selected ) {
		if ( aprev!=NULL )
		    aprev->next = anext;
		else
		    cv->b.sc->anchor = anext;
		ap->next = NULL;
		AnchorPointsFree(ap);
	    } else
		aprev = ap;
	}
    }
    anyimages = false;
    for ( prev = NULL, imgs=cv->b.layerheads[cv->b.drawmode]->images; imgs!=NULL; imgs = next ) {
	next = imgs->next;
	if ( !imgs->selected )
	    prev = imgs;
	else {
	    if ( prev==NULL )
		cv->b.layerheads[cv->b.drawmode]->images = next;
	    else
		prev->next = next;
	    chunkfree(imgs,sizeof(ImageList));
	    /* garbage collection of images????!!!! */
	    anyimages = true;
	}
    }
    if ( anyimages )
	SCOutOfDateBackground(cv->b.sc);
    if ( cv->lastselpt!=NULL || cv->p.sp!=NULL || cv->p.spiro!=NULL || cv->lastselcp!=NULL ) {
	cv->lastselpt = NULL; cv->p.sp = NULL;
	cv->p.spiro = cv->lastselcp = NULL;
	CVInfoDraw(cv,cv->gw);
    }
}

static void CVClear(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int anyanchor;

    if ( !CVAnySel(cv,NULL,NULL,NULL,&anyanchor))
return;
    CVDoClear(cv);
    CVGridHandlePossibleFitChar( cv );
    CVCharChangedUpdate(&cv->b);
}

static void CVClearBackground(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SCClearBackground(cv->b.sc);
}

static void _CVPaste(CharView *cv) {
    enum undotype ut = CopyUndoType();
    int was_empty = cv->b.drawmode==dm_fore && cv->b.sc->hstem==NULL && cv->b.sc->vstem==NULL && cv->b.layerheads[cv->b.drawmode]->splines==NULL && cv->b.layerheads[cv->b.drawmode]->refs==NULL;
    if ( ut!=ut_lbearing )	/* The lbearing code does this itself */
	CVPreserveStateHints(&cv->b);
    if ( ut!=ut_width && ut!=ut_vwidth && ut!=ut_lbearing && ut!=ut_rbearing && ut!=ut_possub )
	CVClearSel(cv);
    PasteToCV(&cv->b);
    cv->lastselpt = NULL;
    CVCharChangedUpdate(&cv->b);
    if ( was_empty && (cv->b.sc->hstem != NULL || cv->b.sc->vstem!=NULL ))
	cv->b.sc->changedsincelasthinted = false;
}

static void CVPaste(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVPaste(cv);
    // Before July 2019, all pastes would reset the view such that all splines
    // were visible. This is wrong in most cases, e.g. tracing many letters
    // from one manuscript page, or copying and pasting parts of a glyph.
    //
    // A more complete fix would be to make it so only selected points were put
    // into view, but it was deemed not worth it at the time, and this comment
    // was left instead. See Github issue №3783 and PR №3813 for more details.
    // CVInkscapeAdjust(cv);
}

static void _CVMerge(CharView *cv, int elide) {
    int anyp = 0;

    if ( !CVAnySel(cv,&anyp,NULL,NULL,NULL) || !anyp)
return;
    CVPreserveState(&cv->b);
    SplineCharMerge(cv->b.sc,&cv->b.layerheads[cv->b.drawmode]->splines,!elide);
    SCClearSelPt(cv->b.sc);
    CVCharChangedUpdate(&cv->b);
}

void CVMerge(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMerge(cv,false);
}

static void _CVMergeToLine(CharView *cv, int elide) {
    int anyp = 0;

    if ( !CVAnySel(cv,&anyp,NULL,NULL,NULL) || !anyp)
	return;
    CVPreserveState(&cv->b);
    SplineCharMerge(cv->b.sc,&cv->b.layerheads[cv->b.drawmode]->splines,!elide);

    // Select the other side of the new curve
    if (!CVInSpiro(cv)) {
        GList_Glib* gl = CVGetSelectedPoints( cv );
        if( g_list_first(gl) )
        SPSelectPrevPoint( (SplinePoint*)g_list_first(gl)->data, 1 );
        g_list_free( gl );
    }

    // And make the curve between the two active points a line
    _CVMenuMakeLine( (CharViewBase*) cv, 0, 0 );
    SCClearSelPt(cv->b.sc);
    CVCharChangedUpdate(&cv->b);
}

void CVMergeToLine(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMergeToLine(cv,false);

}

static void _CVJoin(CharView *cv) {
    CharViewTab* tab = CVGetActiveTab(cv);
    int anyp = 0, changed;
    extern float joinsnap;

    CVAnySel(cv,&anyp,NULL,NULL,NULL);
    CVPreserveState(&cv->b);
    cv->b.layerheads[cv->b.drawmode]->splines = SplineSetJoin(cv->b.layerheads[cv->b.drawmode]->splines,!anyp,joinsnap/tab->scale,&changed,true);
    if ( changed )
	CVCharChangedUpdate(&cv->b);
}

static void CVJoin(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVJoin(cv);
}

static void _CVCut(CharView *cv) {
    int anya;

    if ( !CVAnySel(cv,NULL,NULL,NULL,&anya))
return;
    _CVCopy(cv);
    CVDoClear(cv);
    CVCharChangedUpdate(&cv->b);
}

static void CVCut(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVCut(cv);
}

static void CVCopyFgBg(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( cv->b.sc->layers[ly_fore].splines==NULL )
return;
    SCCopyLayerToLayer(cv->b.sc,ly_fore,ly_back,false);
}

static void CVMenuCopyL2L(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVCopyLayerToLayer(cv);
}

static void CVMenuCompareL2L(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVCompareLayerToLayer(cv);
}

static void CVSelectAll(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int mask = -1;

    if ( mi->mid==MID_SelectAllPoints )
	mask = 1;
    else if ( mi->mid==MID_SelectAnchors )
	mask = 2;
    else if ( mi->mid==MID_SelAll ) {
	mask = 1;
	if (cv->b.drawmode==dm_fore) mask+=2;
	/* TODO! Should we also check if this is the right foreground layer? */
    }

    if ( CVSetSel(cv,mask))
	SCUpdateAll(cv->b.sc);
}

static void CVSelectOpenContours(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplineSet *ss;
    int i;
    SplinePoint *sp;
    int changed = CVClearSel(cv);

    for ( ss=cv->b.layerheads[cv->b.drawmode]->splines; ss!=NULL; ss=ss->next ) {
	if ( ss->first->prev==NULL ) {
	    changed = true;
	    if ( cv->b.sc->inspiro && hasspiro()) {
		for ( i=0; i<ss->spiro_cnt; ++i )
		    SPIRO_SELECT(&ss->spiros[i]);
	    } else {
		for ( sp=ss->first ;; ) {
		    sp->selected = true;
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		}
	    }
	}
    }
    if ( changed )
	SCUpdateAll(cv->b.sc);
}

static void CVSelectNone(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    if ( CVClearSel(cv))
	SCUpdateAll(cv->b.sc);
}

static void CVSelectInvert(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVInvertSel(cv);
    SCUpdateAll(cv->b.sc);
}

static void CVSelectWidth(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    if ( HasUseMyMetrics(cv->b.sc,CVLayer((CharViewBase *) cv))!=NULL )
return;
    cv->widthsel = !cv->widthsel;
    cv->oldwidth = cv->b.sc->width;
    SCUpdateAll(cv->b.sc);
}

static void CVSelectVWidth(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    if ( !cv->showvmetrics || !cv->b.sc->parent->hasvmetrics )
return;
    if ( HasUseMyMetrics(cv->b.sc,CVLayer((CharViewBase *) cv))!=NULL )
return;
    cv->vwidthsel = !cv->widthsel;
    cv->oldvwidth = cv->b.sc->vwidth;
    SCUpdateAll(cv->b.sc);
}

static void CVSelectHM(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplinePoint *sp; SplineSet *spl; RefChar *r; ImageList *im;
    spiro_cp *junk;
    int exactlyone = CVOneThingSel(cv,&sp,&spl,&r,&im,NULL,&junk);

    if ( !exactlyone || sp==NULL || sp->hintmask == NULL || spl==NULL )
return;
    while ( sp!=NULL ) {
	if ( sp->prev==NULL )
    break;
	sp = sp->prev->from;
	if ( sp == spl->first )
    break;
	if ( sp->hintmask!=NULL )
 goto done;
	sp->selected = true;
    }
    for ( spl = spl->next; spl!=NULL; spl = spl->next ) {
	for ( sp=spl->first; sp!=NULL;  ) {
	    if ( sp->hintmask!=NULL )
 goto done;
	    sp->selected = true;
	    if ( sp->prev==NULL )
	break;
	    sp = sp->prev->from;
	    if ( sp == spl->first )
	break;
	}
    }
 done:
    SCUpdateAll(cv->b.sc);
}

static void _CVUnlinkRef(CharView *cv) {
    int anyrefs=0;
    RefChar *rf, *next;

    if ( cv->b.layerheads[cv->b.drawmode]->refs!=NULL ) {
	CVPreserveState(&cv->b);
	for ( rf=cv->b.layerheads[cv->b.drawmode]->refs; rf!=NULL && !anyrefs; rf=rf->next )
	    if ( rf->selected ) anyrefs = true;
	for ( rf=cv->b.layerheads[cv->b.drawmode]->refs; rf!=NULL ; rf=next ) {
	    next = rf->next;
	    if ( rf->selected || !anyrefs) {
		SCRefToSplines(cv->b.sc,rf,CVLayer((CharViewBase *) cv));
	    }
	}
	CVSetCharChanged(cv,true);
	SCUpdateAll(cv->b.sc);
	/* Don't need to update dependencies, their splines won't have changed*/
    }
}

static void CVUnlinkRef(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVUnlinkRef(cv);
}

typedef struct getValueDialogData
{
    int done;
    int cancelled;
    CharView *cv;
    GWindow gw;
    char* ret;
    GTextInfo label;
} GetValueDialogData;

static int getValueDialogData_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	GetValueDialogData *hd = GDrawGetUserData(gw);
	hd->done = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

static int getValueFromUser_OK(GGadget *g, GEvent *e)
{
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
        GetValueDialogData *hd = GDrawGetUserData(GGadgetGetWindow(g));
        strcpy( hd->ret, u_to_c(hd->label.text));
        strcpy( hd->ret, GGadgetGetTitle8(GWidgetGetControl(hd->gw,CID_getValueFromUser)));
        hd->done = true;
    }
    return( true );
}

static int getValueFromUser_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
        GetValueDialogData *hd = GDrawGetUserData(GGadgetGetWindow(g));
        hd->cancelled = true;
        hd->done = true;
    }
    return( true );
}

static char* getValueFromUser( CharView *cv, const char* windowTitle, const char* msg, const char* defaultValue )
{
    const int retsz = 4096;
    static char ret[4097];
    static GetValueDialogData DATA;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[9], *harray1[4], *harray2[9], *barray[7], *varray[5][2], boxes[5];
    GTextInfo label[9];

    DATA.cancelled = false;
    DATA.done = false;
    DATA.cv = cv;
    DATA.ret = ret;
    ret[0] = '\0';

    if ( DATA.gw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = windowTitle;
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,170));
	pos.height = GDrawPointsToPixels(NULL,90);
	DATA.gw = gw = GDrawCreateTopWindow(NULL,&pos,getValueDialogData_e_h,&DATA,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,  0,sizeof(gcd));
	memset(&boxes,0,sizeof(boxes));

	label[0].text = (unichar_t *) msg;
	label[0].text_is_1byte = true;
	label[0].text_in_resource = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5;
	gcd[0].gd.pos.y = 5;
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].creator = GLabelCreate;
	harray1[0] = GCD_Glue;
	harray1[1] = &gcd[0];
	harray1[2] = 0;

	label[1].text = (unichar_t *) defaultValue;
	label[1].text_is_1byte = true;
	DATA.label = label[1];
	gcd[1].gd.label = &label[1];
	gcd[1].gd.pos.x = 5;
	gcd[1].gd.pos.y = 17+5;
	gcd[1].gd.pos.width = 40;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].gd.cid = CID_getValueFromUser;
	gcd[1].creator = GTextFieldCreate;
	harray2[0] = &gcd[1];
	harray2[1] = 0;

	int idx = 2;
	gcd[idx].gd.pos.x = 20-3;
	gcd[idx].gd.pos.y = 17+37;
	gcd[idx].gd.pos.width = -1;
	gcd[idx].gd.pos.height = 0;
	gcd[idx].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[idx].text = (unichar_t *) _("_OK");
	label[idx].text_is_1byte = true;
	label[idx].text_in_resource = true;
	gcd[idx].gd.mnemonic = 'O';
	gcd[idx].gd.label = &label[idx];
	gcd[idx].gd.handle_controlevent = getValueFromUser_OK;
	gcd[idx].creator = GButtonCreate;
	barray[0] = GCD_Glue;
	barray[1] = &gcd[idx];
	barray[2] = GCD_Glue;

	++idx;
	gcd[idx].gd.pos.x = -20;
	gcd[idx].gd.pos.y = 17+37+3;
	gcd[idx].gd.pos.width = -1;
	gcd[idx].gd.pos.height = 0;
	gcd[idx].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[idx].text = (unichar_t *) _("_Cancel");
	label[idx].text_is_1byte = true;
	label[idx].text_in_resource = true;
	gcd[idx].gd.label = &label[idx];
	gcd[idx].gd.mnemonic = 'C';
	gcd[idx].gd.handle_controlevent = getValueFromUser_Cancel;
	gcd[idx].creator = GButtonCreate;
	barray[3] = GCD_Glue;
	barray[4] = &gcd[idx];
	barray[5] = GCD_Glue;
	barray[6] = NULL;

	gcd[7].gd.pos.x = 5;
	gcd[7].gd.pos.y = 17+31;
	gcd[7].gd.pos.width = 170-10;
	gcd[7].gd.flags = gg_enabled|gg_visible;
	gcd[7].creator = GLineCreate;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = harray1;
	boxes[2].creator = GHBoxCreate;

	boxes[3].gd.flags = gg_enabled|gg_visible;
	boxes[3].gd.u.boxelements = harray2;
	boxes[3].creator = GHBoxCreate;

	boxes[4].gd.flags = gg_enabled|gg_visible;
	boxes[4].gd.u.boxelements = barray;
	boxes[4].creator = GHBoxCreate;

	varray[0][0] = &boxes[2]; varray[0][1] = NULL;
	varray[1][0] = &boxes[3]; varray[1][1] = NULL;
	varray[2][0] = &gcd[7];   varray[2][1] = NULL;
	varray[3][0] = &boxes[4]; varray[3][1] = NULL;
	varray[4][0] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = varray[0];
	boxes[0].creator = GHVGroupCreate;

	GGadgetsCreate(gw,boxes);
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[4].ret,gb_expandgluesame);
	GHVBoxFitWindow(boxes[0].ret);
    } else {
	gw = DATA.gw;
	snprintf( ret, retsz, "%s", defaultValue );
	GGadgetSetTitle8(GWidgetGetControl(gw,CID_getValueFromUser),ret);
	GDrawSetTransientFor(gw,(GWindow) -1);
    }

    GWidgetIndicateFocusGadget(GWidgetGetControl(gw,CID_getValueFromUser));
    GTextFieldSelect(GWidgetGetControl(gw,CID_getValueFromUser),0,-1);

    GDrawSetVisible(gw,true);
    while ( !DATA.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(gw,false);

    if( DATA.cancelled )
        return 0;
    return ret;
}



static void CVRemoveUndoes(GWindow gw,struct gmenuitem *mi,GEvent *e)
{
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    static int lastValue = 10;
    int v = toint(getValueFromUser( cv,
				    _("Trimming Undo Information"),
				    _("How many most-recent Undos should be kept?"),
				    tostr(lastValue)));
    lastValue = v;
    UndoesFreeButRetainFirstN(&cv->b.layerheads[cv->b.drawmode]->undoes,v);
    UndoesFreeButRetainFirstN(&cv->b.layerheads[cv->b.drawmode]->redoes,v);
}


/* We can only paste if there's something in the copy buffer */
/* we can only copy if there's something selected to copy */
/* figure out what things are possible from the edit menu before the user */
/*  pulls it down */
static void cv_edlistcheck(CharView *cv, struct gmenuitem *mi) {
    int anypoints, anyrefs, anyimages, anyanchor;

    CVAnySel(cv,&anypoints,&anyrefs,&anyimages,&anyanchor);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Join:
	    mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->splines==NULL;
	  break;
	  case MID_Merge:
	    mi->ti.disabled = !anypoints;
	  break;
	  case MID_MergeToLine:
	    mi->ti.disabled = !anypoints;
	  break;
	  case MID_Clear: case MID_Cut: /*case MID_Copy:*/
	    /* If nothing is selected, copy copies everything */
	    /* In spiro mode copy will copy all contours with at least (spiro) one point selected */
	    mi->ti.disabled = !anypoints && !anyrefs && !anyimages && !anyanchor;
	  break;
	  case MID_CopyLBearing: case MID_CopyRBearing:
	    mi->ti.disabled = cv->b.drawmode!=dm_fore ||
		    (cv->b.layerheads[cv->b.drawmode]->splines==NULL && cv->b.layerheads[cv->b.drawmode]->refs==NULL);
	  break;
	  case MID_CopyFgToBg:
	    mi->ti.disabled = cv->b.sc->layers[ly_fore].splines==NULL;
	  break;
	  case MID_CopyGridFit:
	    mi->ti.disabled = cv->b.gridfit==NULL;
	  break;
	  case MID_Paste:
	    mi->ti.disabled = !CopyContainsSomething() && !SCClipboardHasPasteableContents();
	  break;
	  case MID_Undo:
	    mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->undoes==NULL;
	  break;
	  case MID_Redo:
	    mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->redoes==NULL;
	  break;
	  case MID_RemoveUndoes:
	    mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->undoes==NULL && cv->b.layerheads[cv->b.drawmode]->redoes==NULL;
	  break;
	  case MID_CopyRef:
	    mi->ti.disabled = cv->b.drawmode!=dm_fore || cv->b.container!=NULL;
	  break;
	  case MID_CopyLookupData:
	    mi->ti.disabled = (cv->b.sc->possub==NULL && cv->b.sc->kerns==NULL && cv->b.sc->vkerns==NULL) ||
		    cv->b.container!=NULL;
	  break;
	  case MID_UnlinkRef:
	    mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->refs==NULL;
	  break;
	}
    }
}

static void edlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_edlistcheck(cv, mi);
}

static void CVMenuAcceptableExtrema(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplineSet *ss;
    Spline *s, *first;

    for ( ss = cv->b.layerheads[cv->b.drawmode]->splines; ss!=NULL ; ss = ss->next ) {
	first = NULL;
	for ( s=ss->first->next; s!=NULL && s!=first; s = s->to->next ) {
	    if ( first == NULL )
		first = s;
	    if ( s->from->selected && s->to->selected )
		s->acceptableextrema = !s->acceptableextrema;
	}
    }
}

static void _CVMenuPointType(CharView *cv, struct gmenuitem *mi) {
    int pointtype = mi->mid==MID_Corner?pt_corner:mi->mid==MID_Tangent?pt_tangent:
	    mi->mid==MID_Curve?pt_curve:pt_hvcurve;
    SplinePointList *spl;
    Spline *spline, *first;

    CVPreserveState(&cv->b);	/* We should only get here if there's a selection */
    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL ; spl = spl->next ) {
	first = NULL;
	if ( spl->first->selected ) {
	    if ( spl->first->pointtype!=pointtype )
		SPChangePointType(spl->first,pointtype);
	}
	for ( spline=spl->first->next; spline!=NULL && spline!=first ; spline = spline->to->next ) {
	    if ( spline->to->selected ) {
	    if ( spline->to->pointtype!=pointtype )
		SPChangePointType(spline->to,pointtype);
	    }
	    if ( first == NULL ) first = spline;
	}
    }
    CVCharChangedUpdate(&cv->b);
}

static void _CVMenuSpiroPointType(CharView *cv, struct gmenuitem *mi) {
    int pointtype = mi->mid==MID_SpiroCorner?SPIRO_CORNER:
		    mi->mid==MID_SpiroG4?SPIRO_G4:
		    mi->mid==MID_SpiroG2?SPIRO_G2:
		    mi->mid==MID_SpiroLeft?SPIRO_LEFT:SPIRO_RIGHT;
    SplinePointList *spl;
    int i, changes;

    CVPreserveState(&cv->b);	/* We should only get here if there's a selection */
    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL ; spl = spl->next ) {
	changes = false;
	for ( i=0; i<spl->spiro_cnt-1; ++i ) {
	    if ( SPIRO_SELECTED(&spl->spiros[i]) ) {
		if ( (spl->spiros[i].ty&0x7f)!=SPIRO_OPEN_CONTOUR ) {
		    spl->spiros[i].ty = pointtype|0x80;
		    changes = true;
		}
	    }
	}
	if ( changes )
	    SSRegenerateFromSpiros(spl);
    }
    CVCharChangedUpdate(&cv->b);
}


void CVMenuPointType(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    if ( cv->b.sc->inspiro && hasspiro())
	_CVMenuSpiroPointType(cv, mi);
    else
	_CVMenuPointType(cv, mi);
}

static void _CVMenuImplicit(CharView *cv, struct gmenuitem *mi) {
    SplinePointList *spl;
    Spline *spline, *first;
    int dontinterpolate = mi->mid==MID_NoImplicitPt;

    if ( !cv->b.layerheads[cv->b.drawmode]->order2 )
return;
    CVPreserveState(&cv->b);	/* We should only get here if there's a selection */
    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL ; spl = spl->next ) {
	first = NULL;
	if ( spl->first->selected ) {
	    spl->first->dontinterpolate = dontinterpolate;
	}
	for ( spline=spl->first->next; spline!=NULL && spline!=first ; spline = spline->to->next ) {
	    if ( spline->to->selected ) {
		spline->to->dontinterpolate = dontinterpolate;
	    }
	    if ( first == NULL ) first = spline;
	}
    }
    CVCharChangedUpdate(&cv->b);
}

static void CVMenuImplicit(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuImplicit(cv, mi);
}

static GMenuItem2 spiroptlist[], ptlist[];
static void cv_ptlistcheck(CharView *cv, struct gmenuitem *mi) {
    int type = -2, cnt=0, ccp_cnt=0, spline_selected=0;
    int spirotype = -2, opencnt=0, spirocnt=0;
    SplinePointList *spl, *sel=NULL, *onlysel=NULL;
    Spline *spline, *first;
    SplinePoint *selpt=NULL;
    int notimplicit = -1;
    int acceptable = -1;
    uint16_t junk;
    int i;

    if ( cv->showing_spiro_pt_menu != (cv->b.sc->inspiro && hasspiro())) {
	GMenuItemArrayFree(mi->sub);
	mi->sub = GMenuItem2ArrayCopy(cv->b.sc->inspiro && hasspiro()?spiroptlist:ptlist,&junk);
	cv->showing_spiro_pt_menu = cv->b.sc->inspiro && hasspiro();
    }
    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next ) {
	first = NULL;
	if ( spl->first->selected ) {
	    sel = spl;
	    if ( onlysel==NULL || onlysel==spl ) onlysel = spl; else onlysel = (SplineSet *) (-1);
	    selpt = spl->first; ++cnt;
	    if ( type==-2 ) type = spl->first->pointtype;
	    else if ( type!=spl->first->pointtype ) type = -1;
	    if ( !spl->first->nonextcp && !spl->first->noprevcp && spl->first->prev!=NULL )
		++ccp_cnt;
	    if ( notimplicit==-1 ) notimplicit = spl->first->dontinterpolate;
	    else if ( notimplicit!=spl->first->dontinterpolate ) notimplicit = -2;
	}
	for ( spline=spl->first->next; spline!=NULL && spline!=first; spline = spline->to->next ) {
	    if ( spline->to->selected ) {
		if ( type==-2 ) type = spline->to->pointtype;
		else if ( type!=spline->to->pointtype ) type = -1;
		selpt = spline->to;
		if ( onlysel==NULL || onlysel==spl ) onlysel = spl; else onlysel = (SplineSet *) (-1);
		sel = spl; ++cnt;
		if ( !spline->to->nonextcp && !spline->to->noprevcp && spline->to->next!=NULL )
		    ++ccp_cnt;
		if ( notimplicit==-1 ) notimplicit = spline->to->dontinterpolate;
		else if ( notimplicit!=spline->to->dontinterpolate ) notimplicit = -2;
		if ( spline->from->selected )
		    ++spline_selected;
	    }
	    if ( spline->to->selected && spline->from->selected ) {
		if ( acceptable==-1 )
		    acceptable = spline->acceptableextrema;
		else if ( acceptable!=spline->acceptableextrema )
		    acceptable = -2;
	    }
	    if ( first == NULL ) first = spline;
	}
	for ( i=0; i<spl->spiro_cnt-1; ++i ) {
	    if ( SPIRO_SELECTED(&spl->spiros[i])) {
		int ty = spl->spiros[i].ty&0x7f;
		++spirocnt;
		if ( ty==SPIRO_OPEN_CONTOUR )
		    ++opencnt;
		else if ( spirotype==-2 )
		    spirotype = ty;
		else if ( spirotype!=ty )
		    spirotype = -1;
		if ( onlysel==NULL || onlysel==spl ) onlysel = spl; else onlysel = (SplineSet *) (-1);
	    }
	}
    }

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Corner:
	    mi->ti.disabled = type==-2;
	    mi->ti.checked = type==pt_corner;
	  break;
	  case MID_Tangent:
	    mi->ti.disabled = type==-2;
	    mi->ti.checked = type==pt_tangent;
	  break;
	  case MID_Curve:
	    mi->ti.disabled = type==-2;
	    mi->ti.checked = type==pt_curve;
	  break;
	  case MID_HVCurve:
	    mi->ti.disabled = type==-2;
	    mi->ti.checked = type==pt_hvcurve;
	  break;
	  case MID_SpiroG4:
	    mi->ti.disabled = spirotype==-2;
	    mi->ti.checked = spirotype==SPIRO_G4;
	  break;
	  case MID_SpiroG2:
	    mi->ti.disabled = spirotype==-2;
	    mi->ti.checked = spirotype==SPIRO_G2;
	  break;
	  case MID_SpiroCorner:
	    mi->ti.disabled = spirotype==-2;
	    mi->ti.checked = spirotype==SPIRO_CORNER;
	  break;
	  case MID_SpiroLeft:
	    mi->ti.disabled = spirotype==-2;
	    mi->ti.checked = spirotype==SPIRO_LEFT;
	  break;
	  case MID_SpiroRight:
	    mi->ti.disabled = spirotype==-2;
	    mi->ti.checked = spirotype==SPIRO_RIGHT;
	  break;
	  case MID_MakeFirst:
	    mi->ti.disabled = cnt!=1 || sel->first->prev==NULL || sel->first==selpt;
	  break;
	  case MID_SpiroMakeFirst:
	    mi->ti.disabled = opencnt!=0 || spirocnt!=1;
	  break;
	  case MID_MakeLine: case MID_MakeArc:
	    mi->ti.disabled = cnt<2;
	  break;
	  case MID_AcceptableExtrema:
	    mi->ti.disabled = acceptable<0;
	    mi->ti.checked = acceptable==1;
	  break;
	  case MID_NamePoint:
	    mi->ti.disabled = onlysel==NULL || onlysel == (SplineSet *) -1;
	  break;
	  case MID_NameContour:
	    mi->ti.disabled = onlysel==NULL || onlysel == (SplineSet *) -1;
	  break;
	  case MID_ClipPath:
	    mi->ti.disabled = !cv->b.sc->parent->multilayer;
	  break;
	  case MID_InsertPtOnSplineAt:
	    mi->ti.disabled = spline_selected!=1;
	  break;
	  case MID_CenterCP:
	    mi->ti.disabled = ccp_cnt==0;
	  break;
	  case MID_ImplicitPt:
	    mi->ti.disabled = !cv->b.layerheads[cv->b.drawmode]->order2;
	    mi->ti.checked = notimplicit==0;
	  break;
	  case MID_NoImplicitPt:
	    mi->ti.disabled = !cv->b.layerheads[cv->b.drawmode]->order2;
	    mi->ti.checked = notimplicit==1;
	  break;
	  case MID_AddAnchor:
	    mi->ti.disabled = cv->b.container!=NULL;
	  break;
	}
    }
}

static void ptlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_ptlistcheck(cv, mi);
}

static void _CVMenuDir(CharView *cv, struct gmenuitem *mi) {
    int splinepoints, dir;
    SplinePointList *spl;
    Spline *spline, *first;
    int needsrefresh = false;

    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next ) {
	first = NULL;
	splinepoints = 0;
	if ( cv->b.sc->inspiro  && hasspiro()) {
	    int i;
	    for ( i=0; i<spl->spiro_cnt-1; ++i )
		if ( SPIRO_SELECTED(&spl->spiros[i])) {
		    splinepoints = true;
	    break;
		}
	} else {
	    if ( spl->first->selected ) splinepoints = true;
	    for ( spline=spl->first->next; spline!=NULL && spline!=first && !splinepoints; spline = spline->to->next ) {
		if ( spline->to->selected ) splinepoints = true;
		if ( first == NULL ) first = spline;
	    }
	}
	if ( splinepoints && spl->first->prev!=NULL ) {
	    dir = SplinePointListIsClockwise(spl);
	    if ( (mi->mid==MID_Clockwise && dir==0) || (mi->mid==MID_Counter && dir==1)) {
		if ( !needsrefresh )
		    CVPreserveState(&cv->b);
		SplineSetReverse(spl);
		needsrefresh = true;
	    }
	}
    }
    if ( needsrefresh )
	CVCharChangedUpdate(&cv->b);
}

static void CVMenuDir(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuDir(cv, mi);
}

static void CVMenuCheckSelf(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVShows.checkselfintersects = cv->checkselfintersects = !cv->checkselfintersects;
}

static void CVMenuGlyphSelfIntersects(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    Spline *s=NULL, *s2=NULL;
    SplineSet *ss;
    DBounds b;
    double off;

    ss = LayerAllSplines(cv->b.layerheads[cv->b.drawmode]);
    SplineSetIntersect(ss,&s,&s2);
    LayerUnAllSplines(cv->b.layerheads[cv->b.drawmode]);

    if ( s!=NULL || s2!=NULL ) {
	memset(&b,0,sizeof(b));
	CVClearSel(cv);
	if ( s!=NULL ) {
	    b.minx = b.maxx = s->from->me.x;
	    b.miny = b.maxy = s->from->me.y;
	} else if ( s2!=NULL ) {
	    b.minx = b.maxx = s2->from->me.x;
	    b.miny = b.maxy = s2->from->me.y;
	}
	if ( s!=NULL ) {
	    s->from->selected = s->to->selected = true;
	    if ( s->to->me.x>b.maxx ) b.maxx = s->to->me.x;
	    if ( s->to->me.x<b.minx ) b.minx = s->to->me.x;
	    if ( s->to->me.y>b.maxy ) b.maxy = s->to->me.y;
	    if ( s->to->me.y<b.miny ) b.miny = s->to->me.y;
	}
	if ( s2!=NULL ) {
	    s2->from->selected = s2->to->selected = true;
	    if ( s2->from->me.x>b.maxx ) b.maxx = s2->from->me.x;
	    if ( s2->from->me.x<b.minx ) b.minx = s2->from->me.x;
	    if ( s2->from->me.y>b.maxy ) b.maxy = s2->from->me.y;
	    if ( s2->from->me.y<b.miny ) b.miny = s2->from->me.y;
	    if ( s2->to->me.x>b.maxx ) b.maxx = s2->to->me.x;
	    if ( s2->to->me.x<b.minx ) b.minx = s2->to->me.x;
	    if ( s2->to->me.y>b.maxy ) b.maxy = s2->to->me.y;
	    if ( s2->to->me.y<b.miny ) b.miny = s2->to->me.y;
	}
	off = (b.maxx-b.minx)/10;
	if ( off==0 ) off = 1;
	b.minx -= off; b.maxx += off;
	off = (b.maxy-b.miny)/10;
	if ( off==0 ) off = 1;
	b.miny -= off; b.maxy += off;
	_CVFit(cv,&b,false);
    } else
	ff_post_notice(_("No Intersections"),_("No Intersections"));
}

static int getorigin(void *d,BasePoint *base,int index) {
    CharView *cv = (CharView *) d;

    base->x = base->y = 0;
    switch ( index ) {
      case 0:		/* Character origin */
	/* all done */
      break;
      case 1:		/* Center of selection */
	CVFindCenter(cv,base,!CVAnySel(cv,NULL,NULL,NULL,NULL));
      break;
      case 2:		/* last press */
	base->x = cv->p.cx;
	base->y = cv->p.cy;
	/* I don't have any way of telling if a press has happened. if one */
	/*  hasn't they'll just get a 0,0 origin. oh well */
      break;
      default:
return( false );
    }
return( true );
}

static void TransRef(RefChar *ref,real transform[6], enum fvtrans_flags flags) {
    int j;
    real t[6];

    for ( j=0; j<ref->layer_cnt; ++j )
	SplinePointListTransform(ref->layers[j].splines,transform,tpt_AllPoints);
    t[0] = ref->transform[0]*transform[0] +
		ref->transform[1]*transform[2];
    t[1] = ref->transform[0]*transform[1] +
		ref->transform[1]*transform[3];
    t[2] = ref->transform[2]*transform[0] +
		ref->transform[3]*transform[2];
    t[3] = ref->transform[2]*transform[1] +
		ref->transform[3]*transform[3];
    t[4] = ref->transform[4]*transform[0] +
		ref->transform[5]*transform[2] +
		transform[4];
    t[5] = ref->transform[4]*transform[1] +
		ref->transform[5]*transform[3] +
		transform[5];
    if ( flags&fvt_round_to_int ) {
	t[4] = rint( t[4] );
	t[5] = rint( t[5] );
    }
    memcpy(ref->transform,t,sizeof(t));
    RefCharFindBounds(ref);
}

void CVTransFuncLayer(CharView *cv,Layer *ly,real transform[6], enum fvtrans_flags flags)
{
    int anysel = cv->p.transany;
    RefChar *refs;
    ImageList *img;
    AnchorPoint *ap;
    KernPair *kp;
    PST *pst;
    int l, cvlayer;
    enum transformPointMask tpmask = 0;

    if ( cv->b.sc->inspiro && hasspiro() )
	SplinePointListSpiroTransform(ly->splines,transform,!anysel);
    else
    {
	if( cv->active_tool==cvt_scale )
	    tpmask |= tpmask_operateOnSelectedBCP;

	SplinePointListTransformExtended(
	    ly->splines, transform,
	    !anysel?tpt_AllPoints: interpCPsOnMotion?tpt_OnlySelectedInterpCPs:tpt_OnlySelected,
	    tpmask );
    }

    if ( flags&fvt_round_to_int )
	SplineSetsRound2Int(ly->splines,1.0,cv->b.sc->inspiro && hasspiro(),!anysel);
    if ( ly->images!=NULL ) {
	ImageListTransform(ly->images,transform,!anysel);
	SCOutOfDateBackground(cv->b.sc);
    }
    for ( refs = ly->refs; refs!=NULL; refs=refs->next )
	if ( refs->selected || !anysel )
	    TransRef(refs,transform,flags);
    if ( cv->showanchor ) {
	for ( ap=cv->b.sc->anchor; ap!=NULL; ap=ap->next ) if ( ap->selected || !anysel )
	    ApTransform(ap,transform);
    }
    if ( !anysel ) {
	if ( flags & fvt_scalepstpos ) {
	    for ( kp=cv->b.sc->kerns; kp!=NULL; kp=kp->next )
		kp->off = rint(kp->off*transform[0]);
	    for ( kp=cv->b.sc->vkerns; kp!=NULL; kp=kp->next )
		kp->off = rint(kp->off*transform[3]);
	    for ( pst = cv->b.sc->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->type == pst_position )
		    VrTrans(&pst->u.pos,transform);
		else if ( pst->type==pst_pair ) {
		    VrTrans(&pst->u.pair.vr[0],transform);
		    VrTrans(&pst->u.pair.vr[1],transform);
		}
	    }
	}
	if ( transform[1]==0 && transform[2]==0 ) {
	    TransHints(cv->b.sc->hstem,transform[3],transform[5],transform[0],transform[4],flags&fvt_round_to_int);
	    TransHints(cv->b.sc->vstem,transform[0],transform[4],transform[3],transform[5],flags&fvt_round_to_int);
	    TransDStemHints(cv->b.sc->dstem,transform[0],transform[4],transform[3],transform[5],flags&fvt_round_to_int);
	}
	if ( transform[0]==1 && transform[3]==1 && transform[1]==0 &&
		transform[2]==0 && transform[5]==0 &&
		transform[4]!=0 && CVAllSelected(cv) &&
		cv->b.sc->unicodeenc!=-1 && isalpha(cv->b.sc->unicodeenc)) {
	    SCUndoSetLBearingChange(cv->b.sc,(int) rint(transform[4]));
	    SCSynchronizeLBearing(cv->b.sc,transform[4],CVLayer((CharViewBase *) cv));
	}
    }
    if ( !(flags&fvt_dontmovewidth) && (cv->widthsel || !anysel))
	if ( transform[0]>0 && transform[3]>0 && transform[1]==0 &&
		transform[2]==0 && transform[4]!=0 )
	    SCSynchronizeWidth(cv->b.sc,cv->b.sc->width*transform[0]+transform[4],cv->b.sc->width,NULL);
    if ( !(flags&fvt_dontmovewidth) && (cv->vwidthsel || !anysel))
	if ( transform[0]==1 && transform[3]==1 && transform[1]==0 &&
		transform[2]==0 && transform[5]!=0 )
	    cv->b.sc->vwidth+=transform[5];
    if ( (flags&fvt_alllayers) && !anysel ) {
	/* SCPreserveBackground(cv->b.sc); */ /* done by caller */
	cvlayer = CVLayer( (CharViewBase *) cv );
	for ( l=0; l<cv->b.sc->layer_cnt; ++l ) if ( l!=cvlayer ) {
	    for ( img = cv->b.sc->layers[l].images; img!=NULL; img=img->next )
		BackgroundImageTransform(cv->b.sc, img, transform);
	    SplinePointListTransform(cv->b.sc->layers[l].splines,
		    transform,tpt_AllPoints);
	    for ( refs=cv->b.sc->layers[l].refs; refs!=NULL; refs=refs->next )
		TransRef(refs,transform,flags);
	}
    }
}

void CVTransFunc(CharView *cv,real transform[6], enum fvtrans_flags flags)
{
    Layer *ly = cv->b.layerheads[cv->b.drawmode];
    CVTransFuncLayer( cv, ly, transform, flags );
}

void CVTransFuncAllLayers(CharView *cv,real transform[6], enum fvtrans_flags flags)
{
    int idx;
    for( idx = 0; idx < cv->b.sc->layer_cnt; ++idx )
    {
	Layer *ly = &cv->b.sc->layers[ idx ];
	CVTransFuncLayer( cv, ly, transform, flags );
    }
}

static void transfunc(void *d,real transform[6],int otype,BVTFunc *bvts,
	enum fvtrans_flags flags) {
    CharView *cv = (CharView *) d;
    int anya, l, cvlayer = CVLayer((CharViewBase *) cv);

    if ( cv->b.layerheads[cv->b.drawmode]->undoes!=NULL &&
	    cv->b.layerheads[cv->b.drawmode]->undoes->undotype==ut_tstate )
    {
	CVDoUndo(&cv->b);
    }

    if ( flags&fvt_revert )
	return;

    cv->p.transany = CVAnySel(cv,NULL,NULL,NULL,&anya);
    if ( flags&fvt_justapply )
	CVPreserveTState(cv);
    else {
	CVPreserveStateHints(&cv->b);
	if ( flags&fvt_alllayers )
	    for ( l=0; l<cv->b.sc->layer_cnt; ++l ) if ( l!=cvlayer )
		SCPreserveLayer(cv->b.sc,l,false);
    }

    CVPreserveMaybeState( cv, flags&fvt_justapply );
    CVTransFunc(cv,transform,flags);
    CVCharChangedUpdate(&cv->b);
}

void CVDoTransform(CharView *cv, enum cvtools cvt ) {
    int anysel = CVAnySel(cv,NULL,NULL,NULL,NULL);
    TransformDlgCreate(cv,transfunc,getorigin,!anysel?(tdf_enableback|tdf_addapply):tdf_addapply,
	    cvt);
}

static void CVMenuTransform(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVDoTransform(cv,cvt_none);
}

static void CVMenuPOV(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    struct pov_data pov_data;
    if ( PointOfViewDlg(&pov_data,cv->b.sc->parent,true)==-1 )
return;
    CVPointOfView(cv,&pov_data);
}

static void CVMenuNLTransform(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv->lastselpt = NULL; cv->lastselcp = NULL;
    NonLinearDlg(NULL,cv);
}

void CVMenuConstrain(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVConstrainSelection( cv,
                          mi->mid==MID_Average  ? constrainSelection_AveragePoints :
                          mi->mid==MID_SpacePts ? constrainSelection_SpacePoints   :
                          constrainSelection_SpaceSelectedRegions );
}

static void CVMenuMakeParallel(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVMakeParallel(cv);
}

static void _CVMenuRound2Int(CharView *cv, double factor) {
    int anysel = CVAnySel(cv,NULL,NULL,NULL,NULL);
    RefChar *r;
    AnchorPoint *ap;

    CVPreserveState(&cv->b);
    SplineSetsRound2Int(cv->b.layerheads[cv->b.drawmode]->splines,factor,
	    cv->b.sc->inspiro && hasspiro(), anysel);
    for ( r=cv->b.layerheads[cv->b.drawmode]->refs; r!=NULL; r=r->next ) {
	if ( r->selected || !anysel ) {
	    r->transform[4] = rint(r->transform[4]*factor)/factor;
	    r->transform[5] = rint(r->transform[5]*factor)/factor;
	}
    }
    if ( cv->b.drawmode==dm_fore ) {
	for ( ap=cv->b.sc->anchor; ap!=NULL; ap=ap->next ) {
	    if ( ap->selected || !anysel ) {
		ap->me.x = rint(ap->me.x*factor)/factor;
		ap->me.y = rint(ap->me.y*factor)/factor;
	    }
	}
    }
    CVCharChangedUpdate(&cv->b);
}

static void CVMenuRound2Int(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuRound2Int(cv,1.0);
}

static void CVMenuRound2Hundredths(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuRound2Int(cv,100.0);
}

static void CVMenuCluster(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int layer = cv->b.drawmode == dm_grid ? ly_grid :
		cv->b.drawmode == dm_back ? ly_back
					: cv->b.layerheads[dm_fore] - cv->b.sc->layers;
    SCRoundToCluster(cv->b.sc,layer,true,.1,.5);
}

static void CVMenuStroke(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVStroke(cv);
}

#ifdef FONTFORGE_CONFIG_TILEPATH
static void CVMenuTilePath(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVTile(cv);
}

static void CVMenuPatternTile(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVPatternTile(cv);
}
#endif

static void _CVMenuOverlap(CharView *cv,enum overlap_type ot) {
    /* We know it's more likely that we'll find a problem in the overlap code */
    /*  than anywhere else, so let's save the current state against a crash */

    DoAutoSaves();
    CVPreserveState(&cv->b);

    if ( cv->b.drawmode==dm_fore ) {
	MinimumDistancesFree(cv->b.sc->md);
	cv->b.sc->md = NULL;
    }
    cv->b.layerheads[cv->b.drawmode]->splines = SplineSetRemoveOverlap(cv->b.sc,cv->b.layerheads[cv->b.drawmode]->splines,ot);
    // Check for removal of last selected points.
    if ( cv->b.sc->inspiro && hasspiro()) {
	// Detection is not implemented for Spiro, so just clear them.
	// TODO: Detect point survival in Spiro mode.
	cv->p.sp = cv->lastselpt = NULL;
	cv->p.spiro = cv->lastselcp = NULL;
    } else {
	// Check whether the last selected point is still in the spline set.
	// If not, remove the reference to it.
	if (cv->lastselpt != NULL &&
	        !SplinePointListContainsPoint(cv->b.layerheads[cv->b.drawmode]->splines, cv->lastselpt))
	     cv->p.sp = cv->lastselpt = NULL;
	cv->p.spiro = cv->lastselcp = NULL;
    }
    CVCharChangedUpdate(&cv->b);
}

static void CVMenuOverlap(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int anysel;

    (void) CVAnySel(cv,&anysel,NULL,NULL,NULL);
    _CVMenuOverlap(cv,mi->mid==MID_RmOverlap ? (anysel ? over_rmselected: over_remove) :
		      mi->mid==MID_Intersection ? (anysel ? over_intersel : over_intersect ) :
		      mi->mid==MID_Exclude ? over_exclude :
			  (anysel ? over_fisel : over_findinter));
}

static void CVMenuOrder(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplinePointList *spl;
    RefChar *r;
    ImageList *im;
    int exactlyone = CVOneContourSel(cv,&spl,&r,&im);

    if ( !exactlyone )
return;

    CVPreserveState(&cv->b);
    if ( spl!=NULL ) {
	SplinePointList *p, *pp, *t;
	p = pp = NULL;
	for ( t=cv->b.layerheads[cv->b.drawmode]->splines; t!=NULL && t!=spl; t=t->next ) {
	    pp = p; p = t;
	}
	switch ( mi->mid ) {
	  case MID_First:
	    if ( p!=NULL ) {
		p->next = spl->next;
		spl->next = cv->b.layerheads[cv->b.drawmode]->splines;
		cv->b.layerheads[cv->b.drawmode]->splines = spl;
	    }
	  break;
	  case MID_Earlier:
	    if ( p!=NULL ) {
		p->next = spl->next;
		spl->next = p;
		if ( pp==NULL ) {
		    cv->b.layerheads[cv->b.drawmode]->splines = spl;
		} else {
		    pp->next = spl;
		}
	    }
	  break;
	  case MID_Last:
	    if ( spl->next!=NULL ) {
		for ( t=cv->b.layerheads[cv->b.drawmode]->splines; t->next!=NULL; t=t->next );
		t->next = spl;
		if ( p==NULL )
		    cv->b.layerheads[cv->b.drawmode]->splines = spl->next;
		else
		    p->next = spl->next;
		spl->next = NULL;
	    }
	  break;
	  case MID_Later:
	    if ( spl->next!=NULL ) {
		t = spl->next;
		spl->next = t->next;
		t->next = spl;
		if ( p==NULL )
		    cv->b.layerheads[cv->b.drawmode]->splines = t;
		else
		    p->next = t;
	    }
	  break;
	}
    } else if ( r!=NULL ) {
	RefChar *p, *pp, *t;
	p = pp = NULL;
	for ( t=cv->b.layerheads[cv->b.drawmode]->refs; t!=NULL && t!=r; t=t->next ) {
	    pp = p; p = t;
	}
	switch ( mi->mid ) {
	  case MID_First:
	    if ( p!=NULL ) {
		p->next = r->next;
		r->next = cv->b.layerheads[cv->b.drawmode]->refs;
		cv->b.layerheads[cv->b.drawmode]->refs = r;
	    }
	  break;
	  case MID_Earlier:
	    if ( p!=NULL ) {
		p->next = r->next;
		r->next = p;
		if ( pp==NULL ) {
		    cv->b.layerheads[cv->b.drawmode]->refs = r;
		} else {
		    pp->next = r;
		}
	    }
	  break;
	  case MID_Last:
	    if ( r->next!=NULL ) {
		for ( t=cv->b.layerheads[cv->b.drawmode]->refs; t->next!=NULL; t=t->next );
		t->next = r;
		if ( p==NULL )
		    cv->b.layerheads[cv->b.drawmode]->refs = r->next;
		else
		    p->next = r->next;
		r->next = NULL;
	    }
	  break;
	  case MID_Later:
	    if ( r->next!=NULL ) {
		t = r->next;
		r->next = t->next;
		t->next = r;
		if ( p==NULL )
		    cv->b.layerheads[cv->b.drawmode]->refs = t;
		else
		    p->next = t;
	    }
	  break;
	}
    } else if ( im!=NULL ) {
	ImageList *p, *pp, *t;
	p = pp = NULL;
	for ( t=cv->b.layerheads[cv->b.drawmode]->images; t!=NULL && t!=im; t=t->next ) {
	    pp = p; p = t;
	}
	switch ( mi->mid ) {
	  case MID_First:
	    if ( p!=NULL ) {
		p->next = im->next;
		im->next = cv->b.layerheads[cv->b.drawmode]->images;
		cv->b.layerheads[cv->b.drawmode]->images = im;
	    }
	  break;
	  case MID_Earlier:
	    if ( p!=NULL ) {
		p->next = im->next;
		im->next = p;
		if ( pp==NULL ) {
		    cv->b.layerheads[cv->b.drawmode]->images = im;
		} else {
		    pp->next = im;
		}
	    }
	  break;
	  case MID_Last:
	    if ( im->next!=NULL ) {
		for ( t=cv->b.layerheads[cv->b.drawmode]->images; t->next!=NULL; t=t->next );
		t->next = im;
		if ( p==NULL )
		    cv->b.layerheads[cv->b.drawmode]->images = im->next;
		else
		    p->next = im->next;
		im->next = NULL;
	    }
	  break;
	  case MID_Later:
	    if ( im->next!=NULL ) {
		t = im->next;
		im->next = t->next;
		t->next = im;
		if ( p==NULL )
		    cv->b.layerheads[cv->b.drawmode]->images = t;
		else
		    p->next = t;
	    }
	  break;
	}
    }
    CVCharChangedUpdate(&cv->b);
}

static void _CVMenuAddExtrema(CharView *cv) {
    int anysel;
    SplineFont *sf = cv->b.sc->parent;

    (void) CVAnySel(cv,&anysel,NULL,NULL,NULL);
    CVPreserveState(&cv->b);
    SplineCharAddExtrema(cv->b.sc,cv->b.layerheads[cv->b.drawmode]->splines,
	    anysel?ae_between_selected:ae_only_good,sf->ascent+sf->descent);
    CVCharChangedUpdate(&cv->b);
}

static void CVMenuAddExtrema(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuAddExtrema(cv);
}

static void _CVMenuAction(GWindow gw, void (*func)(SplineChar*, SplineSet*, int)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int anysel = CVAnySel(cv,NULL,NULL,NULL,NULL);
    CVPreserveState(&cv->b);
    func(cv->b.sc,cv->b.layerheads[cv->b.drawmode]->splines,anysel);
    CVCharChangedUpdate(&cv->b);
}

static void CVMenuAddInflections(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    _CVMenuAction(gw,&SplineCharAddInflections);
}

static void CVMenuBalance(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    _CVMenuAction(gw,&SplineCharBalance);
}

static void CVMenuHarmonize(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    _CVMenuAction(gw,&SplineCharHarmonize);
}

static void CVSimplify(CharView *cv,int type) {
    static struct simplifyinfo smpls[] = {
	    { sf_normal, 0, 0, 0, 0, 0, 0 },
	    { sf_normal,.75,.05,0,-1, 0, 0 },
	    { sf_normal,.75,.05,0,-1, 0, 0 }};
    struct simplifyinfo *smpl = &smpls[type+1];

    if ( smpl->linelenmax==-1 || (type==0 && !smpl->set_as_default)) {
	smpl->err = (cv->b.sc->parent->ascent+cv->b.sc->parent->descent)/1000.;
	smpl->linelenmax = (cv->b.sc->parent->ascent+cv->b.sc->parent->descent)/100.;
    }

    if ( type==1 ) {
	if ( !SimplifyDlg(cv->b.sc->parent,smpl))
return;
	if ( smpl->set_as_default )
	    smpls[1] = *smpl;
    }

    CVPreserveState(&cv->b);
    smpl->check_selected_contours = true;
    cv->b.layerheads[cv->b.drawmode]->splines = SplineCharSimplify(cv->b.sc,cv->b.layerheads[cv->b.drawmode]->splines,
	    smpl);
    CVCharChangedUpdate(&cv->b);
}

static void CVMenuSimplify(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVSimplify(cv,0);
}

static void CVMenuSimplifyMore(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVSimplify(cv,1);
}

static void CVMenuCleanupGlyph(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVSimplify(cv,-1);
}

static int SPLSelected(SplineSet *ss) {
    SplinePoint *sp;

    for ( sp=ss->first ;; ) {
	if ( sp->selected )
return( true );
	if ( sp->next==NULL )
return( false );
	sp = sp->next->to;
	if ( sp==ss->first )
return( false );
    }
}

static void CVCanonicalStart(CharView *cv) {
    SplineSet *ss;
    int changed = 0;

    for ( ss = cv->b.layerheads[cv->b.drawmode]->splines; ss!=NULL; ss=ss->next )
	if ( ss->first==ss->last && SPLSelected(ss)) {
	    SPLStartToLeftmost(cv->b.sc,ss,&changed);
	    /* The above clears the spiros if needed */
	}
}

static void CVMenuCanonicalStart(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVCanonicalStart(cv);
}

static void CVCanonicalContour(CharView *cv) {
    CanonicalContours(cv->b.sc,CVLayer((CharViewBase *) cv));
}

static void CVMenuCanonicalContours(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVCanonicalContour(cv);
}

static void _CVMenuMakeFirst(CharView *cv) {
    SplinePoint *selpt = NULL;
    int anypoints = 0, splinepoints;
    SplinePointList *spl, *sel;
    Spline *spline, *first;

    sel = NULL;
    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next ) {
	first = NULL;
	splinepoints = 0;
	if ( spl->first->selected ) { splinepoints = 1; sel = spl; selpt=spl->first; }
	for ( spline=spl->first->next; spline!=NULL && spline!=first && !splinepoints; spline = spline->to->next ) {
	    if ( spline->to->selected ) { ++splinepoints; sel = spl; selpt=spline->to; }
	    if ( first == NULL ) first = spline;
	}
	anypoints += splinepoints;
    }

    if ( anypoints!=1 || sel->first->prev==NULL || sel->first==selpt )
return;

    CVPreserveState(&cv->b);
    sel->first = sel->last = selpt;
    CVCharChangedUpdate(&cv->b);
}

static void CVMenuMakeFirst(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuMakeFirst(cv);
}

static void _CVMenuSpiroMakeFirst(CharView *cv) {
    int anypoints = 0, which;
    SplinePointList *spl, *sel;
    int i;
    spiro_cp *newspiros;

    sel = NULL;
    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next ) {
	for ( i=0; i<spl->spiro_cnt-1; ++i ) {
	    if ( SPIRO_SELECTED(&spl->spiros[i])) {
		if ( SPIRO_SPL_OPEN(spl))
return;
		++anypoints;
		sel = spl;
		which = i;
	    }
	}
    }

    if ( anypoints!=1 || sel==NULL )
return;

    CVPreserveState(&cv->b);
    newspiros = malloc((sel->spiro_max+1)*sizeof(spiro_cp));
    memcpy(newspiros,sel->spiros+which,(sel->spiro_cnt-1-which)*sizeof(spiro_cp));
    memcpy(newspiros+(sel->spiro_cnt-1-which),sel->spiros,which*sizeof(spiro_cp));
    memcpy(newspiros+sel->spiro_cnt-1,sel->spiros+sel->spiro_cnt-1,sizeof(spiro_cp));
    free(sel->spiros);
    sel->spiros = newspiros;
    SSRegenerateFromSpiros(sel);
    CVCharChangedUpdate(&cv->b);
}

static void CVMenuSpiroMakeFirst(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuSpiroMakeFirst(cv);
}

static void CVMenuMakeLine(GWindow gw, struct gmenuitem *mi, GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuMakeLine((CharViewBase *) cv,mi->mid==MID_MakeArc, e!=NULL && (e->u.mouse.state&ksm_meta));
}

void _CVMenuNamePoint(CharView *cv, SplinePoint *sp) {
    char *ret, *name, *oldname;

    oldname = (sp->name && *sp->name) ? sp->name : NULL;
    ret = gwwv_ask_string(_("Name this point"), oldname,
                  _("Please name this point"));
    if ( ret!=NULL ) {
        name = *ret ? ret : NULL;
        if (name != oldname || (name && oldname && strcmp(name,oldname))) {
            sp->name = name;
            CVCharChangedUpdate(&cv->b);
        }
        if (name != ret) { free(ret); ret = NULL; }
        if (name != oldname) { free(oldname); oldname = NULL; }
    }
}

static void CVMenuNamePoint(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplinePointList *spl;
    SplinePoint *sp;
    RefChar *r;
    ImageList *il;
    spiro_cp *junk;

    if ( CVOneThingSel( cv, &sp, &spl, &r, &il, NULL, &junk ) && sp) {
	_CVMenuNamePoint(cv, sp);
    }
}

void _CVMenuNameContour(CharView *cv) {
    SplinePointList *spl, *onlysel = NULL;
    SplinePoint *sp;
    char *ret;
    int i;

    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next ) {
	if ( !cv->b.sc->inspiro || !hasspiro()) {
	    for ( sp=spl->first; ; ) {
		if ( sp->selected ) {
		    if ( onlysel==NULL )
			onlysel = spl;
		    else if ( onlysel!=spl )
return;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	} else {
	    for ( i=0; i<spl->spiro_cnt; ++i ) {
		if ( SPIRO_SELECTED(&spl->spiros[i])) {
		    if ( onlysel==NULL )
			onlysel = spl;
		    else if ( onlysel!=spl )
return;
		}
	    }
	}
    }

    if ( onlysel!=NULL ) {
	ret = gwwv_ask_string(_("Name this contour"),onlysel->contour_name,
		_("Please name this contour"));
	if ( ret!=NULL ) {
	    free(onlysel->contour_name);
	    if ( *ret!='\0' )
		onlysel->contour_name = ret;
	    else {
		onlysel->contour_name = NULL;
		free(ret);
	    }
	    CVCharChangedUpdate(&cv->b);
	}
    }
}

static void CVMenuNameContour(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuNameContour(cv);
}

struct insertonsplineat {
    int done;
    GWindow gw;
    Spline *s;
    CharView *cv;
};

#define CID_X	1001
#define CID_Y	1002
#define CID_XR	1003
#define CID_YR	1004

static int IOSA_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	int err = false;
	struct insertonsplineat *iosa = GDrawGetUserData(GGadgetGetWindow(g));
	double val;
	extended ts[3];
	int which;
	SplinePoint *sp;

	if ( GGadgetIsChecked(GWidgetGetControl(iosa->gw,CID_XR)) ) {
	    val = GetReal8(iosa->gw,CID_X,"X",&err);
	    which = 0;
	} else {
	    val = GetReal8(iosa->gw,CID_Y,"Y",&err);
	    which = 1;
	}
	if ( err )
return(true);
	if ( CubicSolve(&iosa->s->splines[which],val,ts)==0 ) {
	    ff_post_error(_("Out of Range"),_("The spline does not reach %g"), (double) val );
return( true );
	}
	iosa->done = true;
	CVPreserveState(&iosa->cv->b);
	for (;;) {
	    sp = SplineBisect(iosa->s,ts[0]);
	    SplinePointCategorize(sp);
	    if ( which==0 ) {
		double off = val-sp->me.x;
		sp->me.x = val; sp->nextcp.x += off; sp->prevcp.x += off;
	    } else {
		double off = val-sp->me.y;
		sp->me.y = val; sp->nextcp.y += off; sp->prevcp.y += off;
	    }
	    SplineRefigure(sp->prev); SplineRefigure(sp->next);
	    if ( ts[1]==-1 ) {
		CVCharChangedUpdate(&iosa->cv->b);
return( true );
	    }
	    iosa->s = sp->next;
	    if ( CubicSolve(&iosa->s->splines[which],val,ts)==0 ) {
		/* Odd. We found one earlier */
		CVCharChangedUpdate(&iosa->cv->b);
return( true );
	    }
	}
    }
return( true );
}

static int IOSA_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct insertonsplineat *iosa = GDrawGetUserData(GGadgetGetWindow(g));
	iosa->done = true;
    }
return( true );
}

static int IOSA_FocusChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textfocuschanged ) {
	struct insertonsplineat *iosa = GDrawGetUserData(GGadgetGetWindow(g));
	int cid = (intptr_t) GGadgetGetUserData(g);
	GGadgetSetChecked(GWidgetGetControl(iosa->gw,cid),true);
    }
return( true );
}

static int IOSA_RadioChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct insertonsplineat *iosa = GDrawGetUserData(GGadgetGetWindow(g));
	int cid = (intptr_t) GGadgetGetUserData(g);
	GWidgetIndicateFocusGadget(GWidgetGetControl(iosa->gw,cid));
	GTextFieldSelect(GWidgetGetControl(iosa->gw,cid),0,-1);
    }
return( true );
}

static int iosa_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct insertonsplineat *iosa = GDrawGetUserData(gw);
	iosa->done = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

void _CVMenuInsertPt(CharView *cv) {
    SplineSet *spl;
    Spline *s, *found=NULL, *first;
    struct insertonsplineat iosa;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[11], boxes[2], topbox[2], *hvs[13], *varray[8], *buttons[6];
    GTextInfo label[11];

    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next ) {
	first = NULL;
	for ( s=spl->first->next; s!=NULL && s!=first ; s = s->to->next ) {
	    if ( first==NULL ) first=s;
	    if ( s->from->selected && s->to->selected ) {
		if ( found!=NULL )
return;		/* Can only work with one spline */
		found = s;
	    }
	}
    }
    if ( found==NULL )
return;		/* Need a spline */

    memset(&iosa,0,sizeof(iosa));
    iosa.s = found;
    iosa.cv = cv;
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Insert a point on the given spline at either...");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,210));
    pos.height = GDrawPointsToPixels(NULL,120);
    iosa.gw = GDrawCreateTopWindow(NULL,&pos,iosa_e_h,&iosa,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _("_X:");
    label[0].text_is_1byte = true;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5;
    gcd[0].gd.flags = gg_enabled|gg_visible|gg_cb_on;
    gcd[0].gd.cid = CID_XR;
    gcd[0].gd.handle_controlevent = IOSA_RadioChange;
    gcd[0].data = (void *) CID_X;
    gcd[0].creator = GRadioCreate;

    label[1].text = (unichar_t *) _("_Y:");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 5; gcd[1].gd.pos.y = 32;
    gcd[1].gd.flags = gg_enabled|gg_visible|gg_rad_continueold ;
    gcd[1].gd.cid = CID_YR;
    gcd[1].gd.handle_controlevent = IOSA_RadioChange;
    gcd[1].data = (void *) CID_Y;
    gcd[1].creator = GRadioCreate;

    gcd[2].gd.pos.x = 131; gcd[2].gd.pos.y = 5;  gcd[2].gd.pos.width = 60;
    gcd[2].gd.flags = gg_enabled|gg_visible;
    gcd[2].gd.cid = CID_X;
    gcd[2].gd.handle_controlevent = IOSA_FocusChange;
    gcd[2].data = (void *) CID_XR;
    gcd[2].creator = GTextFieldCreate;

    gcd[3].gd.pos.x = 131; gcd[3].gd.pos.y = 32;  gcd[3].gd.pos.width = 60;
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].gd.cid = CID_Y;
    gcd[3].gd.handle_controlevent = IOSA_FocusChange;
    gcd[3].data = (void *) CID_YR;
    gcd[3].creator = GTextFieldCreate;

    gcd[4].gd.pos.x = 20-3; gcd[4].gd.pos.y = 120-32-3;
    gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[4].text = (unichar_t *) _("_OK");
    label[4].text_is_1byte = true;
    label[4].text_in_resource = true;
    gcd[4].gd.mnemonic = 'O';
    gcd[4].gd.label = &label[4];
    gcd[4].gd.handle_controlevent = IOSA_OK;
    gcd[4].creator = GButtonCreate;

    gcd[5].gd.pos.x = -20; gcd[5].gd.pos.y = 120-32;
    gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[5].text = (unichar_t *) _("_Cancel");
    label[5].text_is_1byte = true;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.mnemonic = 'C';
    gcd[5].gd.handle_controlevent = IOSA_Cancel;
    gcd[5].creator = GButtonCreate;

    hvs[0] = &gcd[0]; hvs[1] = &gcd[2]; hvs[2] = NULL;
    hvs[3] = &gcd[1]; hvs[4] = &gcd[3]; hvs[5] = NULL;
    hvs[6] = NULL;

    buttons[0] = buttons[2] = buttons[4] = GCD_Glue; buttons[5] = NULL;
    buttons[1] = &gcd[4]; buttons[3] = &gcd[5];

    varray[0] = &boxes[1]; varray[1] = NULL;
    varray[2] = GCD_Glue; varray[3] = NULL;
    varray[4] = &boxes[0]; varray[5] = NULL;
    varray[6] = NULL;

    memset(boxes,0,sizeof(boxes));
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = buttons;
    boxes[0].creator = GHBoxCreate;

    boxes[1].gd.flags = gg_enabled|gg_visible;
    boxes[1].gd.u.boxelements = hvs;
    boxes[1].creator = GHVBoxCreate;

    memset(topbox,0,sizeof(topbox));
    topbox[0].gd.pos.x = topbox[0].gd.pos.y = 2;
    topbox[0].gd.pos.width = pos.width-4; topbox[0].gd.pos.height = pos.height-4;
    topbox[0].gd.flags = gg_enabled|gg_visible;
    topbox[0].gd.u.boxelements = varray;
    topbox[0].creator = GHVGroupCreate;


    GGadgetsCreate(iosa.gw,topbox);
    GHVBoxSetExpandableRow(topbox[0].ret,1);
    GHVBoxSetExpandableCol(boxes[0].ret,gb_expandgluesame);
    GHVBoxSetExpandableCol(boxes[1].ret,1);
    GWidgetIndicateFocusGadget(GWidgetGetControl(iosa.gw,CID_X));
    GTextFieldSelect(GWidgetGetControl(iosa.gw,CID_X),0,-1);
    GHVBoxFitWindow(topbox[0].ret);

    GDrawSetVisible(iosa.gw,true);
    while ( !iosa.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(iosa.gw);
}

static void CVMenuInsertPt(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVMenuInsertPt(cv);
}

static void _CVCenterCP(CharView *cv) {
    SplinePointList *spl;
    SplinePoint *sp;
    int changed = false;
    enum movething { mt_pt, mt_ncp, mt_pcp } movething = mt_pt;

    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next ) {
	for ( sp=spl->first; ; ) {
	    if ( sp->selected && sp->prev!=NULL && sp->next!=NULL &&
		    !sp->noprevcp && !sp->nonextcp ) {
		if ( sp->me.x != (sp->nextcp.x+sp->prevcp.x)/2 ||
			sp->me.y != (sp->nextcp.y+sp->prevcp.y)/2 ) {
		    if ( !changed ) {
			CVPreserveState(&cv->b);
			changed = true;
		    }
		    switch ( movething ) {
		      case mt_pt:
			sp->me.x = (sp->nextcp.x+sp->prevcp.x)/2;
			sp->me.y = (sp->nextcp.y+sp->prevcp.y)/2;
			SplineRefigure(sp->prev);
			SplineRefigure(sp->next);
		      break;
		      case mt_ncp:
			sp->nextcp.x = sp->me.x - (sp->prevcp.x-sp->me.x);
			sp->nextcp.y = sp->me.y - (sp->prevcp.y-sp->me.y);
			if ( sp->next->order2 ) {
			    sp->next->to->prevcp = sp->nextcp;
			    sp->next->to->noprevcp = false;
			}
			SplineRefigure(sp->prev);
			SplineRefigureFixup(sp->next);
		      break;
		      case mt_pcp:
			sp->prevcp.x = sp->me.x - (sp->nextcp.x-sp->me.x);
			sp->prevcp.y = sp->me.y - (sp->nextcp.y-sp->me.y);
			if ( sp->prev->order2 ) {
			    sp->prev->from->nextcp = sp->prevcp;
			    sp->prev->from->nonextcp = false;
			}
			SplineRefigureFixup(sp->prev);
			SplineRefigure(sp->next);
		      break;
		    }
		}
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }

    if ( changed )
	CVCharChangedUpdate(&cv->b);
}

static void CVMenuCenterCP(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    _CVCenterCP(cv);
}

void CVMakeClipPath(CharView *cv) {
    SplineSet *ss;
    SplinePoint *sp;
    int sel;
    int changed=false;

    for ( ss=cv->b.layerheads[cv->b.drawmode]->splines; ss!=NULL; ss=ss->next ) {
	sel = false;
	for ( sp=ss->first; ; ) {
	    if ( sp->selected ) {
		sel = true;
	break;
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
	if ( sel!=ss->is_clip_path ) {
	    if ( !changed )
		CVPreserveState((CharViewBase *) cv);
	    changed = true;
	    ss->is_clip_path = sel;
	}
    }
    if ( changed )
	CVCharChangedUpdate((CharViewBase *) cv);
}

static void CVMenuClipPath(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVMakeClipPath(cv);
}

void CVAddAnchor(CharView *cv) {
    int waslig;

    if ( AnchorClassUnused(cv->b.sc,&waslig)==NULL ) {
        SplineFont *sf = cv->b.sc->parent;
        char *name = gwwv_ask_string(_("Anchor Class Name"),"",_("Please enter the name of a Anchor point class to create"));
        if ( name==NULL )
return;
        SFFindOrAddAnchorClass(sf,name,NULL);
        free(name);
	if ( AnchorClassUnused(cv->b.sc,&waslig)==NULL )
return;
    }
    ApGetInfo(cv,NULL);
}

static void CVMenuAddAnchor(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVAddAnchor(cv);
}

static void CVMenuAutotrace(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    GCursor ct;

    ct = GDrawGetCursor(cv->v);
    GDrawSetCursor(cv->v,ct_watch);
    ff_progress_allow_events();
    SCAutoTrace(cv->b.sc,CVLayer((CharViewBase *) cv),e!=NULL && (e->u.mouse.state&ksm_shift));
    GDrawSetCursor(cv->v,ct);
}

static void CVMenuBuildAccent(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    extern int onlycopydisplayed;
    int layer = CVLayer((CharViewBase *) cv);

    if ( SFIsRotatable(cv->b.fv->sf,cv->b.sc))
	/* It's ok */;
    else if ( !SFIsSomethingBuildable(cv->b.fv->sf,cv->b.sc,layer,true) )
return;
    SCBuildComposit(cv->b.fv->sf,cv->b.sc,layer,NULL,onlycopydisplayed,true);
}

static void CVMenuBuildComposite(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    extern int onlycopydisplayed;
    int layer = CVLayer((CharViewBase *) cv);

    if ( SFIsRotatable(cv->b.fv->sf,cv->b.sc))
	/* It's ok */;
    else if ( !SFIsCompositBuildable(cv->b.fv->sf,cv->b.sc->unicodeenc,cv->b.sc,layer) )
return;
    SCBuildComposit(cv->b.fv->sf,cv->b.sc,layer,NULL,onlycopydisplayed,false);
}

static void CVMenuReverseDir(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int changed=false;
    SplineSet *ss;

    for ( ss = cv->b.layerheads[cv->b.drawmode]->splines; ss!=NULL; ss=ss->next )
	if ( PointListIsSelected(ss)) {
	    if ( !changed ) {
		CVPreserveState(&cv->b);
	        cv->lastselpt = NULL; cv->lastselcp = NULL;
		changed = true;
	    }
	    SplineSetReverse(ss);
	}

    if ( changed )
	CVCharChangedUpdate(&cv->b);
}

static void CVMenuCorrectDir(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int changed=false, refchanged=false;
    RefChar *ref;
    int asked=-1;
    int layer = CVLayer( (CharViewBase *) cv);

    for ( ref=cv->b.sc->layers[layer].refs; ref!=NULL; ref=ref->next ) {
	if ( ref->transform[0]*ref->transform[3]<0 ||
		(ref->transform[0]==0 && ref->transform[1]*ref->transform[2]>0)) {
	    if ( asked==-1 ) {
		char *buts[4];
		buts[0] = _("_Unlink");
		buts[1] = _("_No");
		buts[2] = _("_Cancel");
		buts[3] = NULL;
		asked = gwwv_ask(_("Flipped Reference"),(const char **) buts,0,2,_("%.50s contains a flipped reference. This cannot be corrected as is. Would you like me to unlink it and then correct it?"), cv->b.sc->name );
		if ( asked==2 )
return;
		else if ( asked==1 )
    break;
	    }
	    if ( asked==0 ) {
		if ( !refchanged ) {
		    refchanged = true;
		    CVPreserveState(&cv->b);
		}
		SCRefToSplines(cv->b.sc,ref,layer);
	    }
	}
    }

    if ( !refchanged )
	CVPreserveState(&cv->b);

    cv->b.layerheads[cv->b.drawmode]->splines = SplineSetsCorrect(cv->b.layerheads[cv->b.drawmode]->splines,&changed);
    if ( changed || refchanged )
	CVCharChangedUpdate(&cv->b);
}

static void CVMenuInsertText(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    InsertTextDlg(cv);
}

static void CVMenuGetInfo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVGetInfo(cv);
}

static void CVMenuCharInfo(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SCCharInfo(cv->b.sc,CVLayer((CharViewBase *) cv),cv->b.fv->map,CVCurEnc(cv));
}

static void CVMenuShowDependentRefs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SCRefBy(cv->b.sc);
}

static void CVMenuShowDependentSubs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SCSubBy(cv->b.sc);
}

static void CVMenuBitmaps(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    BitmapDlg((FontView *) (cv->b.fv),cv->b.sc,mi->mid==MID_RemoveBitmaps?-1: (mi->mid==MID_AvailBitmaps) );
}

static void cv_allistcheck(CharView *cv, struct gmenuitem *mi) {
    int selpoints = 0;
    SplinePointList *spl;
    SplinePoint *sp=NULL;

    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next ) {
	sp=spl->first;
	while ( 1 ) {
	    if ( sp->selected )
		++selpoints;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Average:
	    mi->ti.disabled = selpoints<2;
	  break;
	  case MID_SpacePts:
	    mi->ti.disabled = ((selpoints<3) && (selpoints!=1));
	  break;
	  case MID_SpaceRegion:
	    mi->ti.disabled = selpoints<3;
	  break;
	  case MID_MakeParallel:
	    mi->ti.disabled = selpoints!=4;
	  break;
        }
    }
}

static void allistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_allistcheck(cv, mi);
}

static void cv_balistcheck(CharView *cv, struct gmenuitem *mi) {
    int layer = CVLayer((CharViewBase *) cv);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_BuildAccent:
	    mi->ti.disabled = !SFIsSomethingBuildable(cv->b.fv->sf,cv->b.sc,layer,true);
	  break;
	  case MID_BuildComposite:
	    mi->ti.disabled = !SFIsSomethingBuildable(cv->b.fv->sf,cv->b.sc,layer,false);
	  break;
        }
    }
}

static void balistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_balistcheck(cv, mi);
}

static void delistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_ShowDependentRefs:
	    mi->ti.disabled = cv->b.sc->dependents==NULL;
	  break;
	  case MID_ShowDependentSubs:
	    mi->ti.disabled = !SCUsedBySubs(cv->b.sc);
	  break;
	}
    }
}

static void rndlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_RoundToCluster:
	    mi->ti.disabled = cv->b.sc->inspiro && hasspiro();
	  break;
        }
    }
}

static void cv_ellistcheck(CharView *cv, struct gmenuitem *mi) {
    int anypoints = 0, splinepoints, dir = -2;
    int self_intersects=-2;
    SplinePointList *spl;
    Spline *spline, *first;
    AnchorPoint *ap;
    spiro_cp *cp;
    int i;

#ifdef FONTFORGE_CONFIG_TILEPATH
    int badsel = false;
    RefChar *ref;
    ImageList *il;

    for ( ref=cv->b.layerheads[cv->b.drawmode]->refs; ref!=NULL; ref=ref->next )
	if ( ref->selected )
	    badsel = true;

    for ( il=cv->b.layerheads[cv->b.drawmode]->images; il!=NULL; il=il->next )
	if ( il->selected )
	    badsel = true;
#endif

    if ( cv->checkselfintersects ) {
	Spline *s, *s2;
	SplineSet *ss;
	ss = LayerAllSplines(cv->b.layerheads[cv->b.drawmode]);
	self_intersects = SplineSetIntersect(ss,&s,&s2);
	LayerUnAllSplines(cv->b.layerheads[cv->b.drawmode]);
    }

    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next ) {
	first = NULL;
	splinepoints = 0;
	if ( cv->b.sc->inspiro && hasspiro()) {
	    for ( i=0; i<spl->spiro_cnt-1; ++i ) {
		if ( SPIRO_SELECTED(&spl->spiros[i])) {
		    splinepoints = 1;
	    break;
		}
	    }
	} else {
	    if ( spl->first->selected ) { splinepoints = 1; }
	    for ( spline=spl->first->next; spline!=NULL && spline!=first && !splinepoints; spline = spline->to->next ) {
		if ( spline->to->selected ) { ++splinepoints; }
		if ( first == NULL ) first = spline;
	    }
	}
	if ( splinepoints ) {
	    anypoints += splinepoints;
	    if ( dir==-1 )
		/* Do nothing */;
	    else if ( spl->first!=spl->last || spl->first->next==NULL ) {
		if ( dir==-2 || dir==2 )
		    dir = 2;	/* Not a closed path, no direction */
		else
		    dir = -1;
	    } else if ( dir==-2 )
		dir = SplinePointListIsClockwise(spl);
		if ( dir==-1 )
		    self_intersects = 1;	/* Sometimes the clockwise test finds intersections the main routine can't */
	    else {
		int subdir = SplinePointListIsClockwise(spl);
		if ( subdir==-1 )
		    self_intersects = 1;
		if ( subdir!=dir )
		    dir = -1;
	    }
	}
    }

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_FontInfo: case MID_CharInfo: case MID_ShowDependentRefs:
	  case MID_FindProblems:
	  case MID_AvailBitmaps:
	    mi->ti.disabled = cv->b.container!=NULL;
	  break;
	  case MID_GetInfo:
	    {
		SplinePoint *sp; SplineSet *spl; RefChar *ref; ImageList *img;
		mi->ti.disabled = !CVOneThingSel(cv,&sp,&spl,&ref,&img,&ap,&cp);
	    }
	  break;
	  case MID_CheckSelf:
	    mi->ti.checked = cv->checkselfintersects;
	  break;
	  case MID_GlyphSelfIntersects:
	    mi->ti.disabled = !cv->checkselfintersects;
	    mi->ti.checked = self_intersects==1;
	  break;
	  case MID_Clockwise:
	    mi->ti.disabled = !anypoints || dir==2 || dir<0;
	    mi->ti.checked = dir==1;
	  break;
	  case MID_Counter:
	    mi->ti.disabled = !anypoints || dir==2 || dir<0;
	    mi->ti.checked = dir==0;
	  break;
	  case MID_Correct:
	    mi->ti.disabled = (cv->b.layerheads[cv->b.drawmode]->splines==NULL && cv->b.layerheads[cv->b.drawmode]->refs==NULL) ||
		    dir==2 || self_intersects==1;
	  break;
	  case MID_ReverseDir:
	    mi->ti.disabled = !anypoints;
	  break;
	  case MID_Stroke:
	  case MID_RmOverlap:
	  case MID_Styles:
	    mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->splines==NULL ||
				cv->b.container!=NULL;
	  break;
#ifdef FONTFORGE_CONFIG_TILEPATH
	  case MID_TilePath:
	    mi->ti.disabled = badsel;
	  break;
#endif
	  case MID_RegenBitmaps: case MID_RemoveBitmaps:
	    mi->ti.disabled = cv->b.fv->sf->bitmaps==NULL;
	  break;
	  case MID_AddExtrema: case MID_AddInflections: case MID_Harmonize:
	    mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->splines==NULL || (cv->b.sc->inspiro && hasspiro());
	  /* Like Simplify, always available, but may not do anything if */
	  /*  all extrema have points. I'm not going to check for that, too hard */
	  break;
	  case MID_Balance:
	    mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->splines==NULL || (cv->b.sc->inspiro && hasspiro())
	    || cv->b.layerheads[cv->b.drawmode]->order2;
	  break;
	  case MID_Simplify:
	    mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->splines==NULL || (cv->b.sc->inspiro && hasspiro());
	  /* Simplify is always available (it may not do anything though) */
	  /*  well, ok. Disable it if there is absolutely nothing to work on */
	  break;
	  case MID_BuildAccent:
	    mi->ti.disabled = !SFIsSomethingBuildable(cv->b.fv->sf,cv->b.sc,
		    CVLayer((CharViewBase *) cv),false);
	  break;
	  case MID_Autotrace:
	    mi->ti.disabled = FindAutoTraceName()==NULL || cv->b.sc->layers[ly_back].images==NULL;
	  break;
	  case MID_Align:
	    mi->ti.disabled = cv->b.sc->inspiro && hasspiro();
	  break;
	}
    }
}

static void ellistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_ellistcheck(cv, mi);
}

static void CVMenuAutoHint(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    /*int removeOverlap = e==NULL || !(e->u.mouse.state&ksm_shift);*/
    int was = cv->b.sc->changedsincelasthinted;

    /* Hint undoes are done in _SplineCharAutoHint */
    cv->b.sc->manualhints = false;
    SplineCharAutoHint(cv->b.sc,CVLayer((CharViewBase *) cv),NULL);
    SCUpdateAll(cv->b.sc);
    if ( was ) {
	FontView *fvs;
	for ( fvs=(FontView *) (cv->b.fv); fvs!=NULL; fvs=(FontView *) (fvs->b.nextsame) )
	    GDrawRequestExpose(fvs->v,NULL,false);	/* Clear any changedsincelasthinted marks */
    }
}

static void CVMenuAutoHintSubs(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SCFigureHintMasks(cv->b.sc,CVLayer((CharViewBase *) cv));
    SCUpdateAll(cv->b.sc);
}

static void CVMenuAutoCounter(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SCFigureCounterMasks(cv->b.sc);
}

static void CVMenuDontAutoHint(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv->b.sc->manualhints = !cv->b.sc->manualhints;
}

static void CVMenuNowakAutoInstr(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplineChar *sc = cv->b.sc;
    GlobalInstrCt gic;

    if ( cv->b.layerheads[cv->b.drawmode]->splines!=NULL && sc->hstem==NULL && sc->vstem==NULL
	    && sc->dstem==NULL && !no_windowing_ui )
	ff_post_notice(_("Things could be better..."), _("Glyph, %s, has no hints. FontForge will not produce many instructions."),
		sc->name );

    InitGlobalInstrCt(&gic, sc->parent, CVLayer((CharViewBase *) cv), NULL);
    NowakowskiSCAutoInstr(&gic, sc);
    FreeGlobalInstrCt(&gic);
    SCUpdateAll(sc);
}

static void CVMenuClearHints(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    SCPreserveHints(cv->b.sc,CVLayer((CharViewBase *) cv));
    SCHintsChanged(cv->b.sc);
    if ( mi->mid==MID_ClearHStem ) {
	StemInfosFree(cv->b.sc->hstem);
	cv->b.sc->hstem = NULL;
	cv->b.sc->hconflicts = false;
    } else if ( mi->mid==MID_ClearVStem ) {
	StemInfosFree(cv->b.sc->vstem);
	cv->b.sc->vstem = NULL;
	cv->b.sc->vconflicts = false;
    } else if ( mi->mid==MID_ClearDStem ) {
	DStemInfosFree(cv->b.sc->dstem);
	cv->b.sc->dstem = NULL;
    }
    cv->b.sc->manualhints = true;

    if ( mi->mid != MID_ClearDStem ) {
        SCClearHintMasks(cv->b.sc,CVLayer((CharViewBase *) cv),true);
    }
    SCOutOfDateBackground(cv->b.sc);
    SCUpdateAll(cv->b.sc);
}

/* This is an improved version of the older CVTwoForePointsSelected function. */
/* Unlike the former, it doesn't just check if there are exactly two points   */
/* selected, but rather returns the number of selected points (whatever this  */
/* number can be) and puts references to those points into an array. It is up */
/* to the calling code to see if the returned result is satisfiable (there    */
/* should be exactly two points selected for specifying a vertical or         */
/* horizontal stem and four points for a diagonal stem). */
static int CVNumForePointsSelected(CharView *cv, BasePoint **bp) {
    SplineSet *spl;
    SplinePoint *test, *first;
    BasePoint *bps[5];
    int i, cnt;

    if ( cv->b.drawmode!=dm_fore )
return( 0 ) ;
    cnt = 0;
    for ( spl = cv->b.sc->layers[ly_fore].splines; spl!=NULL; spl = spl->next ) {
	first = NULL;
	for ( test = spl->first; test!=first; test = test->next->to ) {
	    if ( test->selected ) {
		bps[cnt++] = &(test->me);
		if ( cnt>4 )
return( 0 );
	    }
	    if ( first == NULL ) first = test;
	    if ( test->next==NULL )
	break;
	}
    }
    for (i=0; i<cnt; i++) {
        bp[i] = bps[i];
    }
return( cnt );
}

static void CVMenuAddHint(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    BasePoint *bp[4], unit;
    StemInfo *h=NULL;
    DStemInfo *d;
    int num;
    int layer = CVLayer((CharViewBase *) cv);

    num = CVNumForePointsSelected( cv,bp );

    /* We need exactly 2 points to specify a horizontal or vertical stem */
    /* and exactly 4 points to specify a diagonal stem */
    if ( !(num == 2 && mi->mid != MID_AddDHint) &&
         !(num == 4 && mi->mid == MID_AddDHint))
return;

    SCPreserveHints(cv->b.sc,CVLayer((CharViewBase *) cv));
    SCHintsChanged(cv->b.sc);
    if ( mi->mid==MID_AddHHint ) {
	if ( bp[0]->y==bp[1]->y )
return;
	h = chunkalloc(sizeof(StemInfo));
	if ( bp[1]->y>bp[0]->y ) {
	    h->start = bp[0]->y;
	    h->width = bp[1]->y-bp[0]->y;
	} else {
	    h->start = bp[1]->y;
	    h->width = bp[0]->y-bp[1]->y;
	}
	SCGuessHHintInstancesAndAdd(cv->b.sc,layer,h,bp[0]->x,bp[1]->x);
	cv->b.sc->hconflicts = StemListAnyConflicts(cv->b.sc->hstem);
    } else if ( mi->mid==MID_AddVHint ) {
	if ( bp[0]->x==bp[1]->x )
return;
	h = chunkalloc(sizeof(StemInfo));
	if ( bp[1]->x>bp[0]->x ) {
	    h->start = bp[0]->x;
	    h->width = bp[1]->x-bp[0]->x;
	} else {
	    h->start = bp[1]->x;
	    h->width = bp[0]->x-bp[1]->x;
	}
	SCGuessVHintInstancesAndAdd(cv->b.sc,layer,h,bp[0]->y,bp[1]->y);
	cv->b.sc->vconflicts = StemListAnyConflicts(cv->b.sc->vstem);
    } else {
	if ( !PointsDiagonalable( cv->b.sc->parent,bp,&unit ))
return;
	/* No additional tests, as the points should have already been */
        /* reordered by PointsDiagonalable */
        d = chunkalloc(sizeof(DStemInfo));
        d->where = NULL;
        d->left = *bp[0];
        d->right = *bp[1];
        d->unit = unit;
        SCGuessDHintInstances( cv->b.sc,layer,d );
        if ( d->where == NULL )
            DStemInfoFree( d );
        else
            MergeDStemInfo( cv->b.sc->parent,&cv->b.sc->dstem,d );
    }
    cv->b.sc->manualhints = true;

    /* Hint Masks are not relevant for diagonal stems, so modifying */
    /* diagonal stems should not affect them */
    if ( (mi->mid==MID_AddVHint) || (mi->mid==MID_AddHHint) ) {
        if ( h!=NULL && cv->b.sc->parent->mm==NULL )
	    SCModifyHintMasksAdd(cv->b.sc,layer,h);
        else
	    SCClearHintMasks(cv->b.sc,layer,true);
    }
    SCOutOfDateBackground(cv->b.sc);
    SCUpdateAll(cv->b.sc);
}

static void CVMenuCreateHint(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVCreateHint(cv,mi->mid==MID_CreateHHint,true);
}

static void CVMenuReviewHints(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( cv->b.sc->hstem==NULL && cv->b.sc->vstem==NULL )
return;
    CVReviewHints(cv);
}

static void htlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int cvlayer = CVLayer((CharViewBase *) cv);
    BasePoint *bp[4], unit;
    int multilayer = cv->b.sc->parent->multilayer;
    int i=0, num = 0;

    for (i=0; i<4; i++) {bp[i]=NULL;}

    num = CVNumForePointsSelected(cv,bp);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_AutoHint:
	    mi->ti.disabled = cvlayer == ly_grid || multilayer;
	  break;
	  case MID_HintSubsPt:
	    mi->ti.disabled = multilayer ||
		              cv->b.layerheads[cv->b.drawmode]->order2 ||
		              cvlayer == ly_grid;
	  break;
	  case MID_AutoCounter:
	    mi->ti.disabled = multilayer;
	  break;
	  case MID_DontAutoHint:
	    mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->order2 || multilayer;
	    mi->ti.checked = cv->b.sc->manualhints;
	  break;
	  case MID_AutoInstr:
	  case MID_EditInstructions:
	    mi->ti.disabled = multilayer ||
		!cv->b.layerheads[cv->b.drawmode]->order2 ||
		cvlayer == ly_grid;
	  break;
	  case MID_Debug:
	    mi->ti.disabled = multilayer ||
		!cv->b.layerheads[cv->b.drawmode]->order2 ||
		!hasFreeTypeDebugger();
	  break;
	  case MID_Deltas:
	    mi->ti.disabled = multilayer ||
		!cv->b.layerheads[cv->b.drawmode]->order2 ||
		!hasFreeTypeDebugger();
	  break;
          case  MID_ClearHStem:
          case  MID_ClearVStem:
          case  MID_ClearDStem:
	    mi->ti.disabled = cvlayer == ly_grid;
	  break;
	  case MID_ClearInstr:
	    mi->ti.disabled = cv->b.sc->ttf_instrs_len==0;
	  break;
	  case MID_AddHHint:
	    mi->ti.disabled = num != 2 || bp[1]->y==bp[0]->y || multilayer;
	  break;
	  case MID_AddVHint:
	    mi->ti.disabled = num != 2 || bp[1]->x==bp[0]->x || multilayer;
	  break;
	  case MID_AddDHint:
	    mi->ti.disabled = num != 4 || !PointsDiagonalable( cv->b.sc->parent,bp,&unit ) || multilayer;
	  break;
          case  MID_CreateHHint:
          case  MID_CreateVHint:
	    mi->ti.disabled = cvlayer == ly_grid;
	  break;
	  case MID_ReviewHints:
	    mi->ti.disabled = (cv->b.sc->hstem==NULL && cv->b.sc->vstem==NULL ) || multilayer;
	  break;
	}
    }
}

static void mtlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    RefChar *r = HasUseMyMetrics(cv->b.sc,CVLayer((CharViewBase *) cv));

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_RemoveKerns:
	    mi->ti.disabled = cv->b.sc->kerns==NULL;
	  break;
	  case MID_RemoveVKerns:
	    mi->ti.disabled = cv->b.sc->vkerns==NULL;
	  break;
	  case MID_SetVWidth:
	    mi->ti.disabled = !cv->b.sc->parent->hasvmetrics || r!=NULL;
	  break;
	  case MID_AnchorsAway:
	    mi->ti.disabled = cv->b.sc->anchor==NULL;
	  break;
	  case MID_SetWidth: case MID_SetLBearing: case MID_SetRBearing: case MID_SetBearings:
	    mi->ti.disabled = r!=NULL;
	  break;
	}
    }
}

static void cv_sllistcheck(CharView *cv, struct gmenuitem *mi) {
    SplinePoint *sp; SplineSet *spl; RefChar *r; ImageList *im;
    spiro_cp *scp;
    SplineSet *test;
    int exactlyone = CVOneThingSel(cv,&sp,&spl,&r,&im,NULL,&scp);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_NextCP: case MID_PrevCP:
	    mi->ti.disabled = !exactlyone || sp==NULL || (cv->b.sc->inspiro && hasspiro());
	  break;
	  case MID_NextPt: case MID_PrevPt:
	  case MID_FirstPtNextCont:
	    mi->ti.disabled = !exactlyone || (sp==NULL && scp==NULL);
	  break;
	  case MID_FirstPt: case MID_SelPointAt:
	    mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->splines==NULL;
	  break;
	  case MID_Contours:
	    mi->ti.disabled = !CVAnySelPoints(cv);
	  break;
	  case MID_SelectOpenContours:
	    mi->ti.disabled = true;
	    for ( test=cv->b.layerheads[cv->b.drawmode]->splines; test!=NULL; test=test->next ) {
		if ( test->first->prev==NULL ) {
		    mi->ti.disabled = false;
	    break;
		}
	    }
	  break;
	  case MID_SelectWidth:
	    mi->ti.disabled = !cv->showhmetrics;
	    if ( HasUseMyMetrics(cv->b.sc,CVLayer((CharViewBase *) cv))!=NULL )
		mi->ti.disabled = true;
	    if ( !mi->ti.disabled ) {
		free(mi->ti.text);
		mi->ti.text = utf82u_copy(cv->widthsel?_("Deselect Width"):_("Width"));
	    }
	  break;
	  case MID_SelectVWidth:
	    mi->ti.disabled = !cv->showvmetrics || !cv->b.sc->parent->hasvmetrics;
	    if ( HasUseMyMetrics(cv->b.sc,CVLayer((CharViewBase *) cv))!=NULL )
		mi->ti.disabled = true;
	    if ( !mi->ti.disabled ) {
		free(mi->ti.text);
		mi->ti.text = utf82u_copy(cv->vwidthsel?_("Deselect VWidth"):_("VWidth"));
	    }
	  break;
	  case MID_SelectHM:
	    mi->ti.disabled = !exactlyone || sp==NULL || sp->hintmask==NULL;
	  break;
	}
    }
}

static void sllistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_sllistcheck(cv, mi);
}

static void cv_cblistcheck(CharView *cv, struct gmenuitem *mi) {
    int i;
    KernPair *kp;
    SplineChar *sc = cv->b.sc;
    SplineFont *sf = sc->parent;
    PST *pst;
    char *name;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_AnchorPairs:
	    mi->ti.disabled = sc->anchor==NULL;
	  break;
	  case MID_AnchorControl:
	    mi->ti.disabled = sc->anchor==NULL;
	  break;
	  case MID_AnchorGlyph:
	    if ( cv->apmine!=NULL )
		mi->ti.disabled = false;
	    else
		mi->ti.disabled = sc->anchor==NULL;
	  break;
	  case MID_KernPairs:
	    mi->ti.disabled = sc->kerns==NULL;
	    if ( sc->kerns==NULL ) {
		for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
		    for ( kp = sf->glyphs[i]->kerns; kp!=NULL; kp=kp->next ) {
			if ( kp->sc == sc ) {
			    mi->ti.disabled = false;
		goto out;
			}
		    }
		}
	      out:;
	    }
	  break;
	  case MID_Ligatures:
	    name = sc->name;
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
		for ( pst=sf->glyphs[i]->possub; pst!=NULL; pst=pst->next ) {
		    if ( pst->type==pst_ligature &&
			    PSTContains(pst->u.lig.components,name)) {
			mi->ti.disabled = false;
	  goto break_out_2;
		    }
		}
	    }
	    mi->ti.disabled = true;
	  break_out_2:;
	  break;
	}
    }
}

static void cv_nplistcheck(CharView *cv, struct gmenuitem *mi) {
    SplineChar *sc = cv->b.sc;
    int order2 = cv->b.layerheads[cv->b.drawmode]->order2;
    int is_grid_layer = cv->b.drawmode == dm_grid;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_PtsNone:
	    mi->ti.disabled = !order2 || is_grid_layer;
	    mi->ti.checked = (cv->showpointnumbers == 0);
	  break;
	  case MID_PtsTrue:
	    mi->ti.disabled = !order2 || is_grid_layer;
	    mi->ti.checked = cv->showpointnumbers && order2;
	  break;
	  case MID_PtsPost:
	    mi->ti.disabled = order2 || is_grid_layer;
	    mi->ti.checked = cv->showpointnumbers && !order2 && sc->numberpointsbackards;
	  break;
	  case MID_PtsSVG:
	    mi->ti.disabled = order2 || is_grid_layer;
	    mi->ti.checked = cv->showpointnumbers && !order2 && !sc->numberpointsbackards;
	  break;
          case MID_PtsPos:
	    mi->ti.disabled = is_grid_layer;
            mi->ti.checked = (cv->showpointnumbers == 2);
	}
    }
}

static void gflistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_ShowGridFit:
	    mi->ti.disabled = !hasFreeType() || cv->dv!=NULL;
	    mi->ti.checked = cv->show_ft_results;
	  break;
	  case MID_ShowGridFitLiveUpdate:
	    mi->ti.disabled = !hasFreeType() || cv->dv!=NULL;
	    mi->ti.checked = cv->show_ft_results_live_update;
	  break;
	  case MID_Bigger:
	    mi->ti.disabled = !cv->show_ft_results;
	  break;
	  case MID_Smaller:
	    mi->ti.disabled = !cv->show_ft_results || cv->ft_pointsizex<2 || cv->ft_pointsizey<2;
	  break;
	  case MID_GridFitAA:
	    mi->ti.disabled = !cv->show_ft_results;
	    mi->ti.checked = cv->ft_depth==8;
	  break;
	  case MID_GridFitOff:
	    mi->ti.disabled = !cv->show_ft_results;
	  break;
	}
    }
}

static void swlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplineFont *sf = cv->b.sc->parent;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_MarkExtrema:
	    mi->ti.checked = cv->markextrema;
	    mi->ti.disabled = cv->b.sc->inspiro && hasspiro();
	  break;
	  case MID_MarkPointsOfInflection:
	    mi->ti.checked = cv->markpoi;
	    mi->ti.disabled = cv->b.sc->inspiro && hasspiro();
	  break;
	  case MID_ShowAlmostHV:
	    mi->ti.checked = cv->showalmosthvlines;
	  break;
	  case MID_ShowAlmostHVCurves:
	    mi->ti.checked = cv->showalmosthvcurves;
	  break;
	  case MID_DefineAlmost:
	    mi->ti.disabled = !cv->showalmosthvlines && !cv->showalmosthvcurves;
	  break;
	  case MID_ShowCPInfo:
	    mi->ti.checked = cv->showcpinfo;
	  break;
          case MID_DraggingComparisonOutline:
	    mi->ti.checked = prefs_create_dragging_comparison_outline;
	    break;
	  case MID_ShowSideBearings:
	    mi->ti.checked = cv->showsidebearings;
	  break;
	  case MID_ShowRefNames:
	    mi->ti.checked = cv->showrefnames;
	  break;
	  case MID_ShowTabs:
	    mi->ti.checked = cv->showtabs;
	    mi->ti.disabled = cv->former_cnt<=1;
	  break;
	  case MID_HidePoints:
	    mi->ti.checked = cv->showpoints;
	  break;
	case MID_HideControlPoints:
	    mi->ti.checked = cv->alwaysshowcontrolpoints;
	    break;
	  case MID_HideRulers:
	    mi->ti.checked = cv->showrulers;
	  break;
	  case MID_Fill:
	    mi->ti.checked = cv->showfilled;
	  break;
	  case MID_ShowHHints:
	    mi->ti.checked = cv->showhhints;
	    mi->ti.disabled = sf->multilayer;
	  break;
	  case MID_ShowVHints:
	    mi->ti.checked = cv->showvhints;
	    mi->ti.disabled = sf->multilayer;
	  break;
	  case MID_ShowDHints:
	    mi->ti.checked = cv->showdhints;
	    mi->ti.disabled = sf->multilayer;
	  break;
	  case MID_ShowBlueValues:
	    mi->ti.checked = cv->showblues;
	    mi->ti.disabled = sf->multilayer;
	  break;
	  case MID_ShowFamilyBlues:
	    mi->ti.checked = cv->showfamilyblues;
	    mi->ti.disabled = sf->multilayer;
	  break;
	  case MID_ShowAnchors:
	    mi->ti.checked = cv->showanchor;
	    mi->ti.disabled = sf->multilayer;
	  break;
	  case MID_ShowHMetrics:
	    mi->ti.checked = cv->showhmetrics;
	  break;
	  case MID_ShowVMetrics:
	    mi->ti.checked = cv->showvmetrics;
	    mi->ti.disabled = !sf->hasvmetrics;
	  break;
	  case MID_ShowDebugChanges:
	    mi->ti.checked = cv->showdebugchanges;
	  break;
	  case MID_SnapOutlines:
#ifndef _NO_LIBCAIRO
	    if ( GDrawHasCairo(cv->v)&gc_alpha ) {
		mi->ti.checked = cv->snapoutlines;
		mi->ti.disabled = false;
	    } else
#endif
	    {
		mi->ti.checked = true;
		mi->ti.disabled = true;
	    }
	  break;
	}
    }
}

static void cv_vwlistcheck(CharView *cv, struct gmenuitem *mi) {
    int pos, gid;
    SplineFont *sf = cv->b.sc->parent;
    EncMap *map = cv->b.fv->map;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_NextDef:
	    if ( cv->b.container==NULL ) {
		for ( pos = CVCurEnc(cv)+1; pos<map->enccount && ((gid=map->map[pos])==-1 || !SCWorthOutputting(sf->glyphs[gid])); ++pos );
		mi->ti.disabled = pos==map->enccount;
	    } else
		mi->ti.disabled = !(cv->b.container->funcs->canNavigate)(cv->b.container,nt_nextdef);
	  break;
	  case MID_PrevDef:
	    if ( cv->b.container==NULL ) {
		for ( pos = CVCurEnc(cv)-1; pos>=0 && ((gid=map->map[pos])==-1 || !SCWorthOutputting(sf->glyphs[gid])); --pos );
		mi->ti.disabled = pos<0 || cv->b.container!=NULL;
	    } else
		mi->ti.disabled = !(cv->b.container->funcs->canNavigate)(cv->b.container,nt_nextdef);
	  break;
	  case MID_Next:
	    mi->ti.disabled = cv->b.container==NULL ? CVCurEnc(cv)==map->enccount-1 : !(cv->b.container->funcs->canNavigate)(cv->b.container,nt_nextdef);
	  break;
	  case MID_Prev:
	    mi->ti.disabled = cv->b.container==NULL ? CVCurEnc(cv)==0 : !(cv->b.container->funcs->canNavigate)(cv->b.container,nt_nextdef);
	  break;
	  case MID_Former:
	    if ( cv->former_cnt<=1 )
		pos = -1;
	    else for ( pos = sf->glyphcnt-1; pos>=0 ; --pos )
		if ( sf->glyphs[pos]!=NULL && strcmp(sf->glyphs[pos]->name,cv->former_names[1])==0 )
	    break;
	    mi->ti.disabled = pos==-1 || cv->b.container!=NULL;
	  break;
	  case MID_Goto:
	    mi->ti.disabled = cv->b.container!=NULL && !(cv->b.container->funcs->canNavigate)(cv->b.container,nt_goto);
	  break;
	  case MID_FindInFontView:
	    mi->ti.disabled = cv->b.container!=NULL;
	  break;
#if HANYANG
	  case MID_DisplayCompositions:
	    mi->ti.disabled = !cv->b.sc->compositionunit || cv->b.sc->parent->rules==NULL;
	  break;
#endif
	}
    }
}

static void cblistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_cblistcheck(cv, mi);
}

static void nplistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_nplistcheck(cv, mi);
}

static void vwlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv_vwlistcheck(cv, mi);
}

static void CVMenuCenter(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    DBounds bb;
    real transform[6];
    int drawmode = cv->b.drawmode;

    cv->b.drawmode = dm_fore;

    memset(transform,0,sizeof(transform));
    transform[0] = transform[3] = 1.0;
    transform[1] = transform[2] = transform[5] = 0.0;
    if ( cv->b.sc->parent->italicangle==0 )
	SplineCharFindBounds(cv->b.sc,&bb);
    else {
	SplineSet *base, *temp;
	base = LayerAllSplines(cv->b.layerheads[cv->b.drawmode]);
	transform[2] = tan( cv->b.sc->parent->italicangle * FF_PI/180.0 );
	temp = SplinePointListTransform(SplinePointListCopy(base),transform,tpt_AllPoints);
	transform[2] = 0;
	LayerUnAllSplines(cv->b.layerheads[cv->b.drawmode]);
	SplineSetFindBounds(temp,&bb);
	SplinePointListsFree(temp);
    }

    if ( mi->mid==MID_Center )
	transform[4] = (cv->b.sc->width-(bb.maxx-bb.minx))/2 - bb.minx;
    else
	transform[4] = (cv->b.sc->width-(bb.maxx-bb.minx))/3 - bb.minx;
    if ( transform[4]!=0 ) {
	cv->p.transany = false;
	CVPreserveState(&cv->b);
	CVTransFuncAllLayers(cv, transform, fvt_dontmovewidth );
	CVCharChangedUpdate(&cv->b);
    }
    cv->b.drawmode = drawmode;
}

static void CVMenuSetWidth(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( mi->mid == MID_SetVWidth && !cv->b.sc->parent->hasvmetrics )
return;
    CVSetWidth(cv,mi->mid==MID_SetWidth?wt_width:
		  mi->mid==MID_SetLBearing?wt_lbearing:
		  mi->mid==MID_SetRBearing?wt_rbearing:
		  mi->mid==MID_SetBearings?wt_bearings:
		  wt_vwidth);
}

static void CVMenuRemoveKern(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SCRemoveKern(cv->b.sc);
}

static void CVMenuRemoveVKern(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SCRemoveVKern(cv->b.sc);
}

static void CVMenuKPCloseup(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    KernPairD(cv->b.sc->parent,cv->b.sc,NULL,CVLayer((CharViewBase *) cv),false);
}

static void CVMenuAnchorsAway(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    AnchorPoint *ap;

    ap = mi->ti.userdata;
    if ( ap==NULL )
	for ( ap = cv->b.sc->anchor; ap!=NULL && !ap->selected; ap = ap->next );
    if ( ap==NULL ) ap= cv->b.sc->anchor;
    if ( ap==NULL )
return;

    GDrawSetCursor(cv->v,ct_watch);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    AnchorControl(cv->b.sc,ap,CVLayer((CharViewBase *) cv));
    GDrawSetCursor(cv->v,ct_pointer);
}

static GMenuItem2 wnmenu[] = {
    { { (unichar_t *) N_("New O_utline Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'u' }, H_("New Outline Window|No Shortcut"), NULL, NULL, /* No function, never avail */NULL, 0 },
    { { (unichar_t *) N_("New _Bitmap Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("New Bitmap Window|No Shortcut"), NULL, NULL, CVMenuOpenBitmap, MID_OpenBitmap },
    { { (unichar_t *) N_("New _Metrics Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("New Metrics Window|No Shortcut"), NULL, NULL, CVMenuOpenMetrics, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Warnings"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Warnings|No Shortcut"), NULL, NULL, _MenuWarnings, MID_Warnings },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    GMENUITEM2_EMPTY
};

static void CVWindowMenuBuild(GWindow gw, struct gmenuitem *mi, GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    struct gmenuitem *wmi;

    WindowMenuBuild(gw,mi,e);
    for ( wmi = mi->sub; wmi->ti.text!=NULL || wmi->ti.line ; ++wmi ) {
	switch ( wmi->mid ) {
	  case MID_OpenBitmap:
	    wmi->ti.disabled = cv->b.sc->parent->bitmaps==NULL;
	  break;
	  case MID_Warnings:
	    wmi->ti.disabled = ErrorWindowExists();
	  break;
	}
    }
    if ( cv->b.container!=NULL ) {
	int canopen = (cv->b.container->funcs->canOpen)(cv->b.container);
	if ( !canopen ) {
	    for ( wmi = mi->sub; wmi->ti.text!=NULL || wmi->ti.line ; ++wmi ) {
		wmi->ti.disabled = true;
	    }
	}
    }
}

static GMenuItem2 dummyitem[] = {
    { { (unichar_t *) N_("Font|_New"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'N' }, NULL, NULL, NULL, NULL, 0 },
    GMENUITEM2_EMPTY
};
static GMenuItem2 fllist[] = {
    { { (unichar_t *) N_("Font|_New"), (GImage *) "filenew.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'N' }, H_("New|No Shortcut"), NULL, NULL, MenuNew, MID_New },
    { { (unichar_t *) N_("_Open"), (GImage *) "fileopen.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'O' }, H_("Open|No Shortcut"), NULL, NULL, CVMenuOpen, MID_Open },
    { { (unichar_t *) N_("Recen_t"), (GImage *) "filerecent.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 't' }, NULL, dummyitem, MenuRecentBuild, NULL, MID_Recent },
    { { (unichar_t *) N_("_Close"), (GImage *) "fileclose.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Close|No Shortcut"), NULL, NULL, CVMenuClose, MID_Close },
    { { (unichar_t *) N_("C_lose Tab"), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Close Tab|No Shortcut"), NULL, NULL, CVMenuCloseTab, MID_CloseTab },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Save"), (GImage *) "filesave.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Save|No Shortcut"), NULL, NULL, CVMenuSave, 0 },
    { { (unichar_t *) N_("S_ave as..."), (GImage *) "filesaveas.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'a' }, H_("Save as...|No Shortcut"), NULL, NULL, CVMenuSaveAs, 0 },
    { { (unichar_t *) N_("_Generate Fonts..."), (GImage *) "filegenerate.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'G' }, H_("Generate Fonts...|No Shortcut"), NULL, NULL, CVMenuGenerate, 0 },
    { { (unichar_t *) N_("Generate Mac _Family..."), (GImage *) "filegeneratefamily.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Generate Mac Family...|No Shortcut"), NULL, NULL, CVMenuGenerateFamily, 0 },
    { { (unichar_t *) N_("Generate TTC..."), (GImage *) "filegeneratefamily.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Generate TTC...|No Shortcut"), NULL, NULL, CVMenuGenerateTTC, MID_GenerateTTC },
    { { (unichar_t *) N_("E_xport..."), (GImage *) "fileexport.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 't' }, H_("Export...|No Shortcut"), NULL, NULL, CVMenuExport, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Import..."), (GImage *) "fileimport.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Import...|No Shortcut"), NULL, NULL, CVMenuImport, 0 },
    { { (unichar_t *) N_("_Revert File"), (GImage *) "filerevert.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Revert File|No Shortcut"), NULL, NULL, CVMenuRevert, MID_Revert },
    { { (unichar_t *) N_("Revert Gl_yph"), (GImage *) "filerevertglyph.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Revert Glyph|No Shortcut"), NULL, NULL, CVMenuRevertGlyph, MID_RevertGlyph },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Load Word List..."), (GImage *) 0, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Load Word List...|No Shortcut"), NULL, NULL, CVAddWordList, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Print..."), (GImage *) "fileprint.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Print...|No Shortcut"), NULL, NULL, CVMenuPrint, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
#if !defined(_NO_PYTHON)
    { { (unichar_t *) N_("E_xecute Script..."), (GImage *) "python.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'x' }, H_("Execute Script...|No Shortcut"), NULL, NULL, CVMenuExecute, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
#endif
    { { (unichar_t *) N_("Pr_eferences..."), (GImage *) "fileprefs.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'e' }, H_("Preferences...|No Shortcut"), NULL, NULL, MenuPrefs, 0 },
    { { (unichar_t *) N_("Appea_rance Editor..."), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'e' }, H_("Appearance Editor...|No Shortcut"), NULL, NULL, MenuXRes, 0 },
#if !defined(_NO_PYTHON)
    { { (unichar_t *) N_("Config_ure Plugins..."), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'u' }, H_("Configure Plugins...|No Shortcut"), NULL, NULL, MenuPlug, 0 },
#endif
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Quit"), (GImage *) "filequit.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'Q' }, H_("Quit|No Shortcut"), NULL, NULL, MenuExit, MID_Quit },
    GMENUITEM2_EMPTY
};

static GMenuItem2 sllist[] = {
    { { (unichar_t *) N_("Select _All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'A' }, H_("Select All|No Shortcut"), NULL, NULL, CVSelectAll, MID_SelAll },
    { { (unichar_t *) N_("_Invert Selection"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Invert Selection|No Shortcut"), NULL, NULL, CVSelectInvert, MID_SelInvert },
    { { (unichar_t *) N_("_Deselect All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Deselect All|Escape"), NULL, NULL, CVSelectNone, MID_SelNone },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_First Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("First Point|No Shortcut"), NULL, NULL, CVMenuNextPrevPt, MID_FirstPt },
    { { (unichar_t *) N_("First P_oint, Next Contour"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("First Point, Next Contour|No Shortcut"), NULL, NULL, CVMenuNextPrevPt, MID_FirstPtNextCont },
    { { (unichar_t *) N_("_Next Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'N' }, H_("Next Point|No Shortcut"), NULL, NULL, CVMenuNextPrevPt, MID_NextPt },
    { { (unichar_t *) N_("_Prev Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Prev Point|No Shortcut"), NULL, NULL, CVMenuNextPrevPt, MID_PrevPt },
    { { (unichar_t *) N_("Ne_xt Control Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'x' }, H_("Next Control Point|No Shortcut"), NULL, NULL, CVMenuNextPrevCPt, MID_NextCP },
    { { (unichar_t *) N_("P_rev Control Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'r' }, H_("Prev Control Point|No Shortcut"), NULL, NULL, CVMenuNextPrevCPt, MID_PrevCP },
    { { (unichar_t *) N_("Points on Selected _Contours"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'r' }, H_("Points on Selected Contours|No Shortcut"), NULL, NULL, CVMenuSelectContours, MID_Contours },
    { { (unichar_t *) N_("Point A_t"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'r' }, H_("Point At|No Shortcut"), NULL, NULL, CVMenuSelectPointAt, MID_SelPointAt },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Select All _Points & Refs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Select All Points & Refs|No Shortcut"), NULL, NULL, CVSelectAll, MID_SelectAllPoints },
    { { (unichar_t *) N_("Select Open Contours"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Select Open Contours|No Shortcut"), NULL, NULL, CVSelectOpenContours, MID_SelectOpenContours },
    { { (unichar_t *) N_("Select Anc_hors"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'c' }, H_("Select Anchors|No Shortcut"), NULL, NULL, CVSelectAll, MID_SelectAnchors },
    { { (unichar_t *) N_("_Width"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Width|No Shortcut"), NULL, NULL, CVSelectWidth, MID_SelectWidth },
    { { (unichar_t *) N_("_VWidth"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("VWidth|No Shortcut"), NULL, NULL, CVSelectVWidth, MID_SelectVWidth },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Select Points Affected by HM"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("Select Points Affected by HM|No Shortcut"), NULL, NULL, CVSelectHM, MID_SelectHM },
    GMENUITEM2_EMPTY
};

static GMenuItem2 edlist[] = {
    { { (unichar_t *) N_("_Undo"), (GImage *) "editundo.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'U' }, H_("Undo|No Shortcut"), NULL, NULL, CVUndo, MID_Undo },
    { { (unichar_t *) N_("_Redo"), (GImage *) "editredo.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Redo|No Shortcut"), NULL, NULL, CVRedo, MID_Redo },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Cu_t"), (GImage *) "editcut.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 't' }, H_("Cut|No Shortcut"), NULL, NULL, CVCut, MID_Cut },
    { { (unichar_t *) N_("_Copy"), (GImage *) "editcopy.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Copy|No Shortcut"), NULL, NULL, CVCopy, MID_Copy },
    { { (unichar_t *) N_("C_opy Reference"), (GImage *) "editcopyref.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Copy Reference|No Shortcut"), NULL, NULL, CVCopyRef, MID_CopyRef },
    { { (unichar_t *) N_("Copy Loo_kup Data"), (GImage *) "editcopylookupdata.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Copy Lookup Data|No Shortcut"), NULL, NULL, CVCopyLookupData, MID_CopyLookupData },
    { { (unichar_t *) N_("Copy _Width"), (GImage *) "editcopywidth.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'W' }, H_("Copy Width|No Shortcut"), NULL, NULL, CVCopyWidth, MID_CopyWidth },
    { { (unichar_t *) N_("Co_py LBearing"), (GImage *) "editcopylbearing.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'p' }, H_("Copy LBearing|No Shortcut"), NULL, NULL, CVCopyWidth, MID_CopyLBearing },
    { { (unichar_t *) N_("Copy RBearin_g"), (GImage *) "editcopyrbearing.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'g' }, H_("Copy RBearing|No Shortcut"), NULL, NULL, CVCopyWidth, MID_CopyRBearing },
    { { (unichar_t *) N_("_Paste"), (GImage *) "editpaste.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Paste|No Shortcut"), NULL, NULL, CVPaste, MID_Paste },
    { { (unichar_t *) N_("C_hop"), (GImage *) "editclear.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'h' }, H_("Chop|Delete"), NULL, NULL, CVClear, MID_Clear },
    { { (unichar_t *) N_("Clear _Background"), (GImage *) "editclearback.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Clear Background|No Shortcut"), NULL, NULL, CVClearBackground, 0 },
    { { (unichar_t *) N_("points|_Merge"), (GImage *) "editmerge.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Merge|No Shortcut"), NULL, NULL, CVMerge, MID_Merge },
    { { (unichar_t *) N_("points|Merge to Line"), (GImage *) "editmergetoline.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Merge to Line|No Shortcut"), NULL, NULL, CVMergeToLine, MID_MergeToLine },
    /*{ { (unichar_t *) N_("_Elide"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Elide|No Shortcut"), NULL, NULL, CVElide, MID_Elide },*/
    { { (unichar_t *) N_("_Join"), (GImage *) "editjoin.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'J' }, H_("Join|No Shortcut"), NULL, NULL, CVJoin, MID_Join },
    { { (unichar_t *) N_("Copy _Fg To Bg"), (GImage *) "editcopyfg2bg.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Copy Fg To Bg|No Shortcut"), NULL, NULL, CVCopyFgBg, MID_CopyFgToBg },
    { { (unichar_t *) N_("Cop_y Layer To Layer..."), (GImage *) "editcopylayer2layer.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Copy Layer To Layer...|No Shortcut"), NULL, NULL, CVMenuCopyL2L, MID_CopyBgToFg },
    { { (unichar_t *) N_("Copy Gri_d Fit"), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Copy Grid Fit|No Shortcut"), NULL, NULL, CVMenuCopyGridFit, MID_CopyGridFit },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Select"), (GImage *) "editselect.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Select|No Shortcut"), sllist, sllistcheck, NULL, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("U_nlink Reference"), (GImage *) "editunlink.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'U' }, H_("Unlink Reference|No Shortcut"), NULL, NULL, CVUnlinkRef, MID_UnlinkRef },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Remo_ve Undoes..."), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'e' }, H_("Remove Undoes...|No Shortcut"), NULL, NULL, CVRemoveUndoes, MID_RemoveUndoes },
    GMENUITEM2_EMPTY
};

static GMenuItem2 ptlist[] = {
    { { (unichar_t *) N_("_Curve"), (GImage *) "pointscurve.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'C' }, H_("Curve|No Shortcut"), NULL, NULL, CVMenuPointType, MID_Curve },
    { { (unichar_t *) N_("_HVCurve"), (GImage *) "pointshvcurve.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'o' }, H_("HVCurve|No Shortcut"), NULL, NULL, CVMenuPointType, MID_HVCurve },
    { { (unichar_t *) N_("C_orner"), (GImage *) "pointscorner.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'o' }, H_("Corner|No Shortcut"), NULL, NULL, CVMenuPointType, MID_Corner },
    { { (unichar_t *) N_("_Tangent"), (GImage *) "pointstangent.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Tangent|No Shortcut"), NULL, NULL, CVMenuPointType, MID_Tangent },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
/* GT: Make this (selected) point the first point in the glyph */
    { { (unichar_t *) N_("_Make First"),  (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Make First|No Shortcut"), NULL, NULL, CVMenuMakeFirst, MID_MakeFirst },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Can Be _Interpolated"),  (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Can Be Interpolated|No Shortcut"), NULL, NULL, CVMenuImplicit, MID_ImplicitPt },
    { { (unichar_t *) N_("Can't _Be Interpolated"),  (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Can't Be Interpolated|No Shortcut"), NULL, NULL, CVMenuImplicit, MID_NoImplicitPt },
    { { (unichar_t *) N_("Center Bet_ween Control Points"),  (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Center Between Control Points|No Shortcut"), NULL, NULL, CVMenuCenterCP, MID_CenterCP },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Add Anchor"), (GImage *) "pointsaddanchor.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'A' }, H_("Add Anchor|No Shortcut"), NULL, NULL, CVMenuAddAnchor, MID_AddAnchor },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Acceptable _Extrema"), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'C' }, H_("Acceptable Extrema|No Shortcut"), NULL, NULL, CVMenuAcceptableExtrema, MID_AcceptableExtrema },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Make _Line"), (GImage *) "pointsmakeline.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Make Line|No Shortcut"), NULL, NULL, CVMenuMakeLine, MID_MakeLine },
    { { (unichar_t *) N_("Ma_ke Arc"), (GImage *) "pointsmakearc.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Make Arc|No Shortcut"), NULL, NULL, CVMenuMakeLine, MID_MakeArc },
    { { (unichar_t *) N_("Inse_rt Point On Spline At..."),  (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Insert Point On Spline At...|No Shortcut"), NULL, NULL, CVMenuInsertPt, MID_InsertPtOnSplineAt },
    { { (unichar_t *) N_("_Name Point"),  (GImage *) "pointsnamepoint.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Name Point|No Shortcut"), NULL, NULL, CVMenuNamePoint, MID_NamePoint },
    { { (unichar_t *) N_("_Name Contour"),  (GImage *) "pointsnamecontour.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Name Contour|No Shortcut"), NULL, NULL, CVMenuNameContour, MID_NameContour },
    { { (unichar_t *) N_("Make Clip _Path"), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Make Clip Path|No Shortcut"), NULL, NULL, CVMenuClipPath, MID_ClipPath },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Tool_s"),  (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Tools|No Shortcut"), cvtoollist, cvtoollist_check, NULL, MID_Tools },
    GMENUITEM2_EMPTY
};

static GMenuItem2 spiroptlist[] = {
    { { (unichar_t *) N_("G4 _Curve"), (GImage *) "pointscurve.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'C' }, H_("G4 Curve|No Shortcut"), NULL, NULL, CVMenuPointType, MID_SpiroG4 },
    { { (unichar_t *) N_("_G2 Curve"), (GImage *) "pointsG2curve.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'o' }, H_("G2 Curve|No Shortcut"), NULL, NULL, CVMenuPointType, MID_SpiroG2 },
    { { (unichar_t *) N_("C_orner"), (GImage *) "pointscorner.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'o' }, H_("Corner|No Shortcut"), NULL, NULL, CVMenuPointType, MID_SpiroCorner },
    { { (unichar_t *) N_("_Left Constraint"), (GImage *) "pointsspiroprev.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Left Constraint|No Shortcut"), NULL, NULL, CVMenuPointType, MID_SpiroLeft },
    { { (unichar_t *) N_("_Right Constraint"), (GImage *) "pointsspironext.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Right Constraint|No Shortcut"), NULL, NULL, CVMenuPointType, MID_SpiroRight },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
/* GT: Make this (selected) point the first point in the glyph */
    { { (unichar_t *) N_("_Make First"), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Make First|No Shortcut"), NULL, NULL, CVMenuSpiroMakeFirst, MID_SpiroMakeFirst },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Add Anchor"), (GImage *) "pointsaddanchor.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'A' }, H_("Add Anchor|No Shortcut"), NULL, NULL, CVMenuAddAnchor, MID_AddAnchor },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Name Point"), (GImage *) "pointsnamepoint.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Name Point|No Shortcut"), NULL, NULL, CVMenuNamePoint, MID_NamePoint },
    { { (unichar_t *) N_("_Name Contour"), (GImage *) "pointsnamecontour.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Name Contour|No Shortcut"), NULL, NULL, CVMenuNameContour, MID_NameContour },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Tool_s"),  (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, NULL, cvspirotoollist, cvtoollist_check, NULL, MID_Tools },
    GMENUITEM2_EMPTY
};

static GMenuItem2 allist[] = {
/* GT: Align these points to their average position */
    { { (unichar_t *) N_("_Align Points"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'A' }, H_("Align Points|No Shortcut"), NULL, NULL, CVMenuConstrain, MID_Average },
    { { (unichar_t *) N_("_Space Points"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Space Points|No Shortcut"), NULL, NULL, CVMenuConstrain, MID_SpacePts },
    { { (unichar_t *) N_("Space _Regions..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Space Regions...|No Shortcut"), NULL, NULL, CVMenuConstrain, MID_SpaceRegion },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Make _Parallel..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Make Parallel...|No Shortcut"), NULL, NULL, CVMenuMakeParallel, MID_MakeParallel },
    GMENUITEM2_EMPTY
};

static GMenuItem2 smlist[] = {
    { { (unichar_t *) N_("_Simplify"), (GImage *) "elementsimplify.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Simplify|No Shortcut"), NULL, NULL, CVMenuSimplify, MID_Simplify },
    { { (unichar_t *) N_("Simplify More..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Simplify More...|No Shortcut"), NULL, NULL, CVMenuSimplifyMore, MID_SimplifyMore },
    { { (unichar_t *) N_("Clea_nup Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'n' }, H_("Cleanup Glyph|No Shortcut"), NULL, NULL, CVMenuCleanupGlyph, MID_CleanupGlyph },
    { { (unichar_t *) N_("Canonical Start _Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'n' }, H_("Canonical Start Point|No Shortcut"), NULL, NULL, CVMenuCanonicalStart, MID_CanonicalStart },
    { { (unichar_t *) N_("Canonical _Contours"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'n' }, H_("Canonical Contours|No Shortcut"), NULL, NULL, CVMenuCanonicalContours, MID_CanonicalContours },
    GMENUITEM2_EMPTY
};

static void smlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Simplify:
	  case MID_CleanupGlyph:
	  case MID_SimplifyMore:
	    mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->splines==NULL;
	  break;
	  case MID_CanonicalStart:
	    mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->splines==NULL ||
		    (cv->b.sc->inspiro && hasspiro());
	  break;
	  case MID_CanonicalContours:
	    mi->ti.disabled = cv->b.layerheads[cv->b.drawmode]->splines==NULL ||
		cv->b.layerheads[cv->b.drawmode]->splines->next==NULL ||
		cv->b.drawmode!=dm_fore;
	  break;
	}
    }
}

static GMenuItem2 orlist[] = {
    { { (unichar_t *) N_("_First"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("First|No Shortcut"), NULL, NULL, CVMenuOrder, MID_First },
    { { (unichar_t *) N_("_Earlier"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Earlier|No Shortcut"), NULL, NULL, CVMenuOrder, MID_Earlier },
    { { (unichar_t *) N_("L_ater"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'n' }, H_("Later|No Shortcut"), NULL, NULL, CVMenuOrder, MID_Later },
    { { (unichar_t *) N_("_Last"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'n' }, H_("Last|No Shortcut"), NULL, NULL, CVMenuOrder, MID_Last },
    GMENUITEM2_EMPTY
};

static void orlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplinePointList *spl;
    RefChar *r;
    ImageList *im;
    int exactlyone = CVOneContourSel(cv,&spl,&r,&im);
    int isfirst, islast;

    isfirst = islast = false;
    if ( spl!=NULL ) {
	isfirst = cv->b.layerheads[cv->b.drawmode]->splines==spl;
	islast = spl->next==NULL;
    } else if ( r!=NULL ) {
	isfirst = cv->b.layerheads[cv->b.drawmode]->refs==r;
	islast = r->next==NULL;
    } else if ( im!=NULL ) {
	isfirst = cv->b.layerheads[cv->b.drawmode]->images==im;
	islast = im->next==NULL;
    }

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_First:
	  case MID_Earlier:
	    mi->ti.disabled = !exactlyone || isfirst;
	  break;
	  case MID_Last:
	  case MID_Later:
	    mi->ti.disabled = !exactlyone || islast;
	  break;
	}
    }
}

static GMenuItem2 rmlist[] = {
    { { (unichar_t *) N_("_Remove Overlap"), (GImage *) "overlaprm.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Remove Overlap|No Shortcut"), NULL, NULL, CVMenuOverlap, MID_RmOverlap },
    { { (unichar_t *) N_("_Intersect"), (GImage *) "overlapintersection.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Intersect|No Shortcut"), NULL, NULL, CVMenuOverlap, MID_Intersection },
    { { (unichar_t *) N_("_Exclude"), (GImage *) "overlapexclude.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'E' }, H_("Exclude|No Shortcut"), NULL, NULL, CVMenuOverlap, MID_Exclude },
    { { (unichar_t *) N_("_Find Intersections"), (GImage *) "overlapfindinter.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Find Intersections|No Shortcut"), NULL, NULL, CVMenuOverlap, MID_FindInter },
    GMENUITEM2_EMPTY
};

static GMenuItem2 eflist[] = {
    { { (unichar_t *) N_("Change _Weight..."), (GImage *) "styleschangeweight.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Change Weight...|No Shortcut"), NULL, NULL, CVMenuEmbolden, MID_Embolden },
    { { (unichar_t *) N_("_Italic..."), (GImage *) "stylesitalic.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Italic...|No Shortcut"), NULL, NULL, CVMenuItalic, MID_Italic },
    { { (unichar_t *) N_("Obli_que..."), (GImage *) "stylesoblique.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Oblique...|No Shortcut"), NULL, NULL, CVMenuOblique, 0 },
    { { (unichar_t *) N_("_Condense/Extend..."), (GImage *) "stylesextendcondense.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Condense/Extend...|No Shortcut"), NULL, NULL, CVMenuCondense, MID_Condense },
    { { (unichar_t *) N_("Change _X-Height..."), (GImage *) "styleschangexheight.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Change X-Height...|No Shortcut"), NULL, NULL, CVMenuChangeXHeight, MID_ChangeXHeight },
    { { (unichar_t *) N_("Change _Glyph..."), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Change Glyph...|No Shortcut"), NULL, NULL, CVMenuChangeGlyph, MID_ChangeGlyph },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("In_line..."), (GImage *) "stylesinline.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'O' }, H_("Inline...|No Shortcut"), NULL, NULL, CVMenuInline, 0 },
    { { (unichar_t *) N_("_Outline..."), (GImage *) "stylesoutline.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Outline...|No Shortcut"), NULL, NULL, CVMenuOutline, 0 },
    { { (unichar_t *) N_("S_hadow..."), (GImage *) "stylesshadow.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Shadow...|No Shortcut"), NULL, NULL, CVMenuShadow, 0 },
    { { (unichar_t *) N_("_Wireframe..."), (GImage *) "styleswireframe.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'W' }, H_("Wireframe...|No Shortcut"), NULL, NULL, CVMenuWireframe, 0 },
    GMENUITEM2_EMPTY
};

static GMenuItem2 balist[] = {
    { { (unichar_t *) N_("_Build Accented Glyph"), (GImage *) "elementbuildaccent.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'u' }, H_("Build Accented Glyph|No Shortcut"), NULL, NULL, CVMenuBuildAccent, MID_BuildAccent },
    { { (unichar_t *) N_("Build _Composite Glyph"), (GImage *) "elementbuildcomposite.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Build Composite Glyph|No Shortcut"), NULL, NULL, CVMenuBuildComposite, MID_BuildComposite },
    GMENUITEM2_EMPTY
};

static GMenuItem2 delist[] = {
    { { (unichar_t *) N_("_References..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'u' }, H_("References...|No Shortcut"), NULL, NULL, CVMenuShowDependentRefs, MID_ShowDependentRefs },
    { { (unichar_t *) N_("_Substitutions..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Substitutions...|No Shortcut"), NULL, NULL, CVMenuShowDependentSubs, MID_ShowDependentSubs },
    GMENUITEM2_EMPTY
};

static GMenuItem2 trlist[] = {
    { { (unichar_t *) N_("_Transform..."), (GImage *) "elementtransform.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Transform...|No Shortcut"), NULL, NULL, CVMenuTransform, 0 },
    { { (unichar_t *) N_("_Point of View Projection..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Point of View Projection...|No Shortcut"), NULL, NULL, CVMenuPOV, 0 },
    { { (unichar_t *) N_("_Non Linear Transform..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Non Linear Transform...|No Shortcut"), NULL, NULL, CVMenuNLTransform, 0 },
    GMENUITEM2_EMPTY
};

static GMenuItem2 rndlist[] = {
    { { (unichar_t *) N_("To _Int"), (GImage *) "elementround.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("To Int|No Shortcut"), NULL, NULL, CVMenuRound2Int, MID_Round },
    { { (unichar_t *) N_("To _Hundredths"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("To Hundredths|No Shortcut"), NULL, NULL, CVMenuRound2Hundredths, 0 },
    { { (unichar_t *) N_("_Cluster"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Cluster|No Shortcut"), NULL, NULL, CVMenuCluster, MID_RoundToCluster },
    GMENUITEM2_EMPTY
};

static GMenuItem2 ellist[] = {
    { { (unichar_t *) N_("_Font Info..."), (GImage *) "elementfontinfo.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Font Info...|No Shortcut"), NULL, NULL, CVMenuFontInfo, MID_FontInfo },
    { { (unichar_t *) N_("_Glyph Info..."), (GImage *) "elementglyphinfo.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Glyph Info...|No Shortcut"), NULL, NULL, CVMenuCharInfo, MID_CharInfo },
    { { (unichar_t *) N_("Get _Info..."), (GImage *) "elementgetinfo.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Get Info...|No Shortcut"), NULL, NULL, CVMenuGetInfo, MID_GetInfo },
    { { (unichar_t *) N_("S_how Dependent"), (GImage *) "elementshowdep.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'D' }, H_("Show Dependent|No Shortcut"), delist, delistcheck, NULL, MID_ShowDependentRefs },
    { { (unichar_t *) N_("Find Proble_ms..."), (GImage *) "elementfindprobs.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Find Problems...|No Shortcut"), NULL, NULL, CVMenuFindProblems, MID_FindProblems },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Bitm_ap strikes Available..."), (GImage *) "elementbitmapsavail.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'A' }, H_("Bitmap strikes Available...|No Shortcut"), NULL, NULL, CVMenuBitmaps, MID_AvailBitmaps },
    { { (unichar_t *) N_("Regenerate _Bitmap Glyphs..."), (GImage *) "elementregenbitmaps.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Regenerate Bitmap Glyphs...|No Shortcut"), NULL, NULL, CVMenuBitmaps, MID_RegenBitmaps },
    { { (unichar_t *) N_("Remove Bitmap Glyphs..."), (GImage *) "elementremovebitmaps.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Remove Bitmap Glyphs...|No Shortcut"), NULL, NULL, CVMenuBitmaps, MID_RemoveBitmaps },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("St_yles"), (GImage *) "elementstyles.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Styles|No Shortcut"), eflist, NULL, NULL, MID_Styles },
    { { (unichar_t *) N_("_Transformations"), (GImage *) "elementtransform.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Transformations|No Shortcut"), trlist, NULL, NULL, 0 },
    { { (unichar_t *) N_("_Expand Stroke..."), (GImage *) "elementexpandstroke.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'E' }, H_("Expand Stroke...|No Shortcut"), NULL, NULL, CVMenuStroke, MID_Stroke },
#ifdef FONTFORGE_CONFIG_TILEPATH
    { { (unichar_t *) N_("Tile _Path..."), (GImage *) "elementtilepath.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Tile Path...|No Shortcut"), NULL, NULL, CVMenuTilePath, MID_TilePath },
    { { (unichar_t *) N_("Tile Pattern..."), (GImage *) "elementtilepattern.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Tile Pattern...|No Shortcut"), NULL, NULL, CVMenuPatternTile, 0 },
#endif
    { { (unichar_t *) N_("O_verlap"), (GImage *) "overlaprm.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'v' }, H_("Overlap|No Shortcut"), rmlist, NULL, NULL, MID_RmOverlap },
    { { (unichar_t *) N_("_Simplify"), (GImage *) "elementsimplify.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Simplify|No Shortcut"), smlist, smlistcheck, NULL, MID_Simplify },
    { { (unichar_t *) N_("Add E_xtrema"), (GImage *) "elementaddextrema.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'x' }, H_("Add Extrema|No Shortcut"), NULL, NULL, CVMenuAddExtrema, MID_AddExtrema },
    { { (unichar_t *) N_("Add Points Of I_nflection"), (GImage *) "elementaddinflections.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'n' }, H_("Add Points Of Inflection|No Shortcut"), NULL, NULL, CVMenuAddInflections, MID_AddInflections },
    { { (unichar_t *) N_("_Balance"), (GImage *) "elementbalance.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'b' }, H_("Balance|No Shortcut"), NULL, NULL, CVMenuBalance, MID_Balance },
    { { (unichar_t *) N_("Harmoni_ze"), (GImage *) "elementharmonize.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'z' }, H_("Harmonize|No Shortcut"), NULL, NULL, CVMenuHarmonize, MID_Harmonize },
    { { (unichar_t *) N_("Autot_race"), (GImage *) "elementautotrace.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'r' }, H_("Autotrace|No Shortcut"), NULL, NULL, CVMenuAutotrace, MID_Autotrace },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("A_lign"), (GImage *) "elementalign.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'l' }, H_("Align|No Shortcut"), allist, allistcheck, NULL, MID_Align },
    { { (unichar_t *) N_("Roun_d"), (GImage *) "elementround.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Round|No Shortcut"), rndlist, rndlistcheck, NULL, MID_Round },
    { { (unichar_t *) N_("_Order"), (GImage *) "elementorder.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Order|No Shortcut"), orlist, orlistcheck, NULL, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Check Self-Intersection"), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'o' }, H_("Check Self-Intersection|No Shortcut"), NULL, NULL, CVMenuCheckSelf, MID_CheckSelf },
    { { (unichar_t *) N_("Glyph Self-Intersects"), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'o' }, H_("Glyph Self-Intersects|No Shortcut"), NULL, NULL, CVMenuGlyphSelfIntersects, MID_GlyphSelfIntersects },
    { { (unichar_t *) N_("Cloc_kwise"), (GImage *) "elementclockwise.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'o' }, H_("Clockwise|No Shortcut"), NULL, NULL, CVMenuDir, MID_Clockwise },
    { { (unichar_t *) N_("Cou_nter Clockwise"), (GImage *) "elementanticlock.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'n' }, H_("Counter Clockwise|No Shortcut"), NULL, NULL, CVMenuDir, MID_Counter },
    { { (unichar_t *) N_("_Correct Direction"), (GImage *) "elementcorrectdir.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'D' }, H_("Correct Direction|No Shortcut"), NULL, NULL, CVMenuCorrectDir, MID_Correct },
    { { (unichar_t *) N_("Reverse Direction"), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'D' }, H_("Reverse Direction|No Shortcut"), NULL, NULL, CVMenuReverseDir, MID_ReverseDir },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Insert Text Outlines..."), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'D' }, H_("Insert Text Outlines...|No Shortcut"), NULL, NULL, CVMenuInsertText, MID_InsertText },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("B_uild"), (GImage *) "elementbuildaccent.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'u' }, H_("Build|No Shortcut"), balist, balistcheck, NULL, MID_BuildAccent },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Compare Layers..."), (GImage *) "elementcomparelayers.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'u' }, H_("Compare Layers...|No Shortcut"), NULL, NULL, CVMenuCompareL2L, 0 },
    GMENUITEM2_EMPTY
};

static GMenuItem2 htlist[] = {
    { { (unichar_t *) N_("Auto_Hint"), (GImage *) "hintsautohint.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'H' }, H_("AutoHint|No Shortcut"), NULL, NULL, CVMenuAutoHint, MID_AutoHint },
    { { (unichar_t *) N_("Hint _Substitution Pts"), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'H' }, H_("Hint Substitution Pts|No Shortcut"), NULL, NULL, CVMenuAutoHintSubs, MID_HintSubsPt },
    { { (unichar_t *) N_("Auto _Counter Hint"), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'H' }, H_("Auto Counter Hint|No Shortcut"), NULL, NULL, CVMenuAutoCounter, MID_AutoCounter },
    { { (unichar_t *) N_("_Don't AutoHint"), (GImage *) "hintsdontautohint.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'H' }, H_("Don't AutoHint|No Shortcut"), NULL, NULL, CVMenuDontAutoHint, MID_DontAutoHint },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Auto_Instr"), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("AutoInstr|No Shortcut"), NULL, NULL, CVMenuNowakAutoInstr, MID_AutoInstr },
    { { (unichar_t *) N_("_Edit Instructions..."), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'l' }, H_("Edit Instructions...|No Shortcut"), NULL, NULL, CVMenuEditInstrs, MID_EditInstructions },
    { { (unichar_t *) N_("_Debug..."), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'l' }, H_("Debug...|No Shortcut"), NULL, NULL, CVMenuDebug, MID_Debug },
    { { (unichar_t *) N_("S_uggest Deltas..."), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'l' }, H_("Suggest Deltas...|No Shortcut"), NULL, NULL, CVMenuDeltas, MID_Deltas },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Clear HStem"), (GImage *) "hintsclearhstems.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Clear HStem|No Shortcut"), NULL, NULL, CVMenuClearHints, MID_ClearHStem },
    { { (unichar_t *) N_("Clear _VStem"), (GImage *) "hintsclearvstems.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("Clear VStem|No Shortcut"), NULL, NULL, CVMenuClearHints, MID_ClearVStem },
    { { (unichar_t *) N_("Clear DStem"), (GImage *) "hintscleardstems.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("Clear DStem|No Shortcut"), NULL, NULL, CVMenuClearHints, MID_ClearDStem },
    { { (unichar_t *) N_("Clear Instructions"),  (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Clear Instructions|No Shortcut"), NULL, NULL, CVMenuClearInstrs, MID_ClearInstr },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Add HHint"), (GImage *) "hintsaddhstem.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'A' }, H_("Add HHint|No Shortcut"), NULL, NULL, CVMenuAddHint, MID_AddHHint },
    { { (unichar_t *) N_("Add VHi_nt"), (GImage *) "hintsaddvstem.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 's' }, H_("Add VHint|No Shortcut"), NULL, NULL, CVMenuAddHint, MID_AddVHint },
    { { (unichar_t *) N_("Add DHint"), (GImage *) "hintsadddstem.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 't' }, H_("Add DHint|No Shortcut"), NULL, NULL, CVMenuAddHint, MID_AddDHint },
    { { (unichar_t *) N_("Crea_te HHint..."), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'r' }, H_("Create HHint...|No Shortcut"), NULL, NULL, CVMenuCreateHint, MID_CreateHHint },
    { { (unichar_t *) N_("Cr_eate VHint..."), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'e' }, H_("Create VHint...|No Shortcut"), NULL, NULL, CVMenuCreateHint, MID_CreateVHint },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Review Hints..."), (GImage *) "hintsreviewhints.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Review Hints...|No Shortcut"), NULL, NULL, CVMenuReviewHints, MID_ReviewHints },
    GMENUITEM2_EMPTY
};

static GMenuItem2 ap2list[] = {
    GMENUITEM2_EMPTY
};

static void ap2listbuild(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    char buf[300];
    GMenuItem *sub;
    int k, cnt;
    AnchorPoint *ap;

    if ( mi->sub!=NULL ) {
	GMenuItemArrayFree(mi->sub);
	mi->sub = NULL;
    }

    for ( k=0; k<2; ++k ) {
	cnt = 0;
	for ( ap=cv->b.sc->anchor; ap!=NULL; ap=ap->next ) {
	    if ( k ) {
		if ( ap->type==at_baselig )
/* GT: In the next few lines the "%s" is the name of an anchor class, and the */
/* GT: rest of the string identifies the type of the anchor */
		    snprintf(buf,sizeof(buf), _("%s at ligature pos %d"), ap->anchor->name, ap->lig_index );
		else
		    snprintf(buf,sizeof(buf),
			ap->type==at_cexit ? _("%s exit"):
			ap->type==at_centry ? _("%s entry"):
			ap->type==at_mark ? _("%s mark"):
			    _("%s base"),ap->anchor->name );
		sub[cnt].ti.text = utf82u_copy(buf);
		sub[cnt].ti.userdata = ap;
		sub[cnt].ti.bg = sub[cnt].ti.fg = COLOR_DEFAULT;
		sub[cnt].invoke = CVMenuAnchorsAway;
	    }
	    ++cnt;
	}
	if ( !k )
	    sub = calloc(cnt+1,sizeof(GMenuItem));
    }
    mi->sub = sub;
}

static void CVMenuKernByClasses(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    MetricsView *mv = 0;
    SplineFont *sf = cv->b.sc->parent;
    int cvlayer = CVLayer((CharViewBase *) cv);
    ShowKernClasses(sf, mv, cvlayer, false);
}

static void CVMenuVKernByClasses(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    MetricsView *mv = 0;
    SplineFont *sf = cv->b.sc->parent;
    int cvlayer = CVLayer((CharViewBase *) cv);
    ShowKernClasses(sf, mv, cvlayer, true);
}

static void CVMenuVKernFromHKern(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    FVVKernFromHKern((FontViewBase *) cv->b.fv);
}

static GMenuItem2 mtlist[] = {
    { { (unichar_t *) N_("New _Metrics Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("New Metrics Window|No Shortcut"), NULL, NULL, CVMenuOpenMetrics, 0 },
    GMENUITEM2_LINE,
    { { (unichar_t *) N_("_Center in Width"), (GImage *) "metricscenter.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Center in Width|No Shortcut"), NULL, NULL, CVMenuCenter, MID_Center },
    { { (unichar_t *) N_("_Thirds in Width"), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Thirds in Width|No Shortcut"), NULL, NULL, CVMenuCenter, MID_Thirds },
    { { (unichar_t *) N_("Set _Width..."), (GImage *) "metricssetwidth.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'W' }, H_("Set Width...|No Shortcut"), NULL, NULL, CVMenuSetWidth, MID_SetWidth },
    { { (unichar_t *) N_("Set _LBearing..."), (GImage *) "metricssetlbearing.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'L' }, H_("Set LBearing...|No Shortcut"), NULL, NULL, CVMenuSetWidth, MID_SetLBearing },
    { { (unichar_t *) N_("Set _RBearing..."), (GImage *) "metricssetrbearing.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Set RBearing...|No Shortcut"), NULL, NULL, CVMenuSetWidth, MID_SetRBearing },
    { { (unichar_t *) N_("Set Both Bearings..."), (GImage *) "menuempty.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Set Both Bearings...|No Shortcut"), NULL, NULL, CVMenuSetWidth, MID_SetBearings },
    GMENUITEM2_LINE,
    { { (unichar_t *) N_("Set _Vertical Advance..."), (GImage *) "metricssetvwidth.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("Set Vertical Advance...|No Shortcut"), NULL, NULL, CVMenuSetWidth, MID_SetVWidth },
    GMENUITEM2_LINE,
    { { (unichar_t *) N_("Ker_n By Classes..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Kern By Classes...|No Shortcut"), NULL, NULL, CVMenuKernByClasses, 0 },
    { { (unichar_t *) N_("VKern By Classes..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("VKern By Classes...|No Shortcut"), NULL, NULL, CVMenuVKernByClasses, MID_VKernClass },
    { { (unichar_t *) N_("VKern From HKern"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("VKern From HKern|No Shortcut"), NULL, NULL, CVMenuVKernFromHKern, MID_VKernFromHKern },
    { { (unichar_t *) N_("Remove Kern _Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Remove Kern Pairs|No Shortcut"), NULL, NULL, CVMenuRemoveKern, MID_RemoveKerns },
    { { (unichar_t *) N_("Remove VKern Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Remove VKern Pairs|No Shortcut"), NULL, NULL, CVMenuRemoveVKern, MID_RemoveVKerns },
    { { (unichar_t *) N_("Kern Pair Closeup..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Kern Pair Closeup...|No Shortcut"), NULL, NULL, CVMenuKPCloseup, MID_KPCloseup },
    GMENUITEM2_EMPTY
};

static GMenuItem2 pllist[] = {
    { { (unichar_t *) N_("_Tools"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Tools|No Shortcut"), NULL, NULL, CVMenuPaletteShow, MID_Tools },
    { { (unichar_t *) N_("_Layers"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'L' }, H_("Layers|No Shortcut"), NULL, NULL, CVMenuPaletteShow, MID_Layers },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Docked Palettes"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'D' }, H_("Docked Palettes|No Shortcut"), NULL, NULL, CVMenuPalettesDock, MID_DockPalettes },
    GMENUITEM2_EMPTY
};

static GMenuItem2 aplist[] = {
    { { (unichar_t *) N_("_Detach"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'K' }, H_("Detach|No Shortcut"), NULL, NULL, CVMenuAPDetach, 0 },
    GMENUITEM2_EMPTY
};

static void aplistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplineChar *sc = cv->b.sc, **glyphs;
    SplineFont *sf = sc->parent;
    AnchorPoint *ap, *found;
    GMenuItem2 *mit;
    int cnt;

    found = NULL;
    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
	if ( ap->selected ) {
	    if ( found==NULL )
		found = ap;
	    else {
		/* Can't deal with two selected anchors */
		found = NULL;
    break;
	    }
	}
    }

    GMenuItemArrayFree(mi->sub);
    if ( found==NULL )
	glyphs = NULL;
    else
	glyphs = GlyphsMatchingAP(sf,found);
    if ( glyphs==NULL ) {
	mi->sub = GMenuItem2ArrayCopy(aplist,NULL);
	mi->sub->ti.disabled = (cv->apmine==NULL);
return;
    }

    for ( cnt = 0; glyphs[cnt]!=NULL; ++cnt );
    mit = calloc(cnt+2,sizeof(GMenuItem2));
    mit[0] = aplist[0];
    mit[0].ti.text = (unichar_t *) copy( (char *) mit[0].ti.text );
    mit[0].ti.disabled = (cv->apmine==NULL);
    for ( cnt = 0; glyphs[cnt]!=NULL; ++cnt ) {
	mit[cnt+1].ti.text = (unichar_t *) copy(glyphs[cnt]->name);
	mit[cnt+1].ti.text_is_1byte = true;
	mit[cnt+1].ti.fg = mit[cnt+1].ti.bg = COLOR_DEFAULT;
	mit[cnt+1].ti.userdata = glyphs[cnt];
	mit[cnt+1].invoke = CVMenuAPAttachSC;
	if ( glyphs[cnt]==cv->apsc )
	    mit[cnt+1].ti.checked = mit[cnt+1].ti.checkable = true;
    }
    free(glyphs);
    mi->sub = GMenuItem2ArrayCopy(mit,NULL);
    GMenuItem2ArrayFree(mit);
}

static void CVMoveInWordListByOffset( CharView* cv, int offset )
{
    Wordlist_MoveByOffset( cv->charselector, &cv->charselectoridx, offset );
}

static void CVMenuNextLineInWordList(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView* cv = (CharView*) GDrawGetUserData(gw);
    CVMoveInWordListByOffset( cv, 1 );
}
static void CVMenuPrevLineInWordList(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView* cv = (CharView*) GDrawGetUserData(gw);
    CVMoveInWordListByOffset( cv, -1 );
}

static GMenuItem2 cblist[] = {
    { { (unichar_t *) N_("_Kern Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'K' }, H_("Kern Pairs|No Shortcut"), NULL, NULL, CVMenuKernPairs, MID_KernPairs },
    { { (unichar_t *) N_("_Anchored Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'A' }, H_("Anchored Pairs|No Shortcut"), NULL, NULL, CVMenuAnchorPairs, MID_AnchorPairs },
    { { (unichar_t *) N_("_Anchor Control..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("Anchor Control...|No Shortcut"), ap2list, ap2listbuild, NULL, MID_AnchorControl },
    { { (unichar_t *) N_("Anchor _Glyph at Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'A' }, H_("Anchor Glyph at Point|No Shortcut"), aplist, aplistcheck, NULL, MID_AnchorGlyph },
    { { (unichar_t *) N_("_Ligatures"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'L' }, H_("Ligatures|No Shortcut"), NULL, NULL, CVMenuLigatures, MID_Ligatures },
    GMENUITEM2_EMPTY
};

static GMenuItem2 nplist[] = {
    { { (unichar_t *) N_("PointNumbers|_None"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'K' }, H_("None|No Shortcut"), NULL, NULL, CVMenuNumberPoints, MID_PtsNone },
    { { (unichar_t *) N_("_TrueType"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'A' }, H_("TrueType|No Shortcut"), NULL, NULL, CVMenuNumberPoints, MID_PtsTrue },
    { { (unichar_t *) NU_("_PostScript®"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'L' }, H_("PostScript|No Shortcut"), NULL, NULL, CVMenuNumberPoints, MID_PtsPost },
    { { (unichar_t *) N_("_SVG"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'L' }, H_("SVG|No Shortcut"), NULL, NULL, CVMenuNumberPoints, MID_PtsSVG },
    { { (unichar_t *) N_("P_ositions"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'L' }, H_("Positions|No Shortcut"), NULL, NULL, CVMenuNumberPoints, MID_PtsPos },
    GMENUITEM2_EMPTY
};

static GMenuItem2 gflist[] = {
    { { (unichar_t *) N_("Show _Grid Fit..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'l' }, H_("Show Grid Fit...|No Shortcut"), NULL, NULL, CVMenuShowGridFit, MID_ShowGridFit },
    { { (unichar_t *) N_("Show _Grid Fit (Live Update)..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'l' }, H_("Show Grid Fit (Live Update)...|No Shortcut"), NULL, NULL, CVMenuShowGridFitLiveUpdate, MID_ShowGridFitLiveUpdate },
    { { (unichar_t *) N_("_Bigger Point Size"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Bigger Point Size|No Shortcut"), NULL, NULL, CVMenuChangePointSize, MID_Bigger },
    { { (unichar_t *) N_("_Smaller Point Size"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Smaller Point Size|No Shortcut"), NULL, NULL, CVMenuChangePointSize, MID_Smaller },
    { { (unichar_t *) N_("_Anti Alias"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'L' }, H_("Anti Alias|No Shortcut"), NULL, NULL, CVMenuChangePointSize, MID_GridFitAA },
    { { (unichar_t *) N_("_Off"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Off|No Shortcut"), NULL, NULL, CVMenuChangePointSize, MID_GridFitOff },
    GMENUITEM2_EMPTY
};

static GMenuItem2 swlist[] = {
    { { (unichar_t *) N_("_Points"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'o' }, H_("Points|No Shortcut"), NULL, NULL, CVMenuShowHide, MID_HidePoints },
    { { (unichar_t *) N_("Control Points (Always_)"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, ')' }, H_("Control Points (Always)|No Shortcut"), NULL, NULL, CVMenuShowHideControlPoints, MID_HideControlPoints },
    { { (unichar_t *) N_("_Control Point Info"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'M' }, H_("Control Point Info|No Shortcut"), NULL, NULL, CVMenuShowCPInfo, MID_ShowCPInfo },
    { { (unichar_t *) N_("_Extrema"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'M' }, H_("Extrema|No Shortcut"), NULL, NULL, CVMenuMarkExtrema, MID_MarkExtrema },
    { { (unichar_t *) N_("Points of _Inflection"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'M' }, H_("Points of Inflection|No Shortcut"), NULL, NULL, CVMenuMarkPointsOfInflection, MID_MarkPointsOfInflection },
    { { (unichar_t *) N_("Almost Horizontal/Vertical Lines"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'M' }, H_("Almost Horizontal/Vertical Lines|No Shortcut"), NULL, NULL, CVMenuShowAlmostHV, MID_ShowAlmostHV },
    { { (unichar_t *) N_("Almost Horizontal/Vertical Curves"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'M' }, H_("Almost Horizontal/Vertical Curves|No Shortcut"), NULL, NULL, CVMenuShowAlmostHVCurves, MID_ShowAlmostHVCurves },
    { { (unichar_t *) N_("(Define \"Almost\")"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("(Define \"Almost\")|No Shortcut"), NULL, NULL, CVMenuDefineAlmost, MID_DefineAlmost },
    { { (unichar_t *) N_("_Side Bearings"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'M' }, H_("Side Bearings|No Shortcut"), NULL, NULL, CVMenuShowSideBearings, MID_ShowSideBearings },
    { { (unichar_t *) N_("Reference Names"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'M' }, H_("Reference Names|No Shortcut"), NULL, NULL, CVMenuShowRefNames, MID_ShowRefNames },
    { { (unichar_t *) N_("_Fill"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'l' }, H_("Fill|No Shortcut"), NULL, NULL, CVMenuFill, MID_Fill },
    { { (unichar_t *) N_("Previe_w"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'l' }, H_("Preview|No Shortcut"), NULL, NULL, CVMenuPreview, MID_Preview },
    { { (unichar_t *) N_("Dragging Comparison Outline"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'l' }, H_("Dragging Comparison Outline|No Shortcut"), NULL, NULL, CVMenuDraggingComparisonOutline, MID_DraggingComparisonOutline },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Pale_ttes"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Palettes|No Shortcut"), pllist, pllistcheck, NULL, 0 },
    { { (unichar_t *) N_("_Glyph Tabs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("Glyph Tabs|No Shortcut"), NULL, NULL, CVMenuShowTabs, MID_ShowTabs },
    { { (unichar_t *) N_("_Rulers"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("Rulers|No Shortcut"), NULL, NULL, CVMenuShowHideRulers, MID_HideRulers },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Horizontal Hints"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("Horizontal Hints|No Shortcut"), NULL, NULL, CVMenuShowHints, MID_ShowHHints },
    { { (unichar_t *) N_("_Vertical Hints"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("Vertical Hints|No Shortcut"), NULL, NULL, CVMenuShowHints, MID_ShowVHints },
    { { (unichar_t *) N_("_Diagonal Hints"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("Diagonal Hints|No Shortcut"), NULL, NULL, CVMenuShowHints, MID_ShowDHints },
/* GT: You might not want to translate this, it's a keyword in PostScript font files */
    { { (unichar_t *) N_("_BlueValues"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("BlueValues|No Shortcut"), NULL, NULL, CVMenuShowHints, MID_ShowBlueValues },
/* GT: You might not want to translate this, it's a keyword in PostScript font files */
    { { (unichar_t *) N_("FamilyBl_ues"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("FamilyBlues|No Shortcut"), NULL, NULL, CVMenuShowHints, MID_ShowFamilyBlues },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Anchors"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("Anchors|No Shortcut"), NULL, NULL, CVMenuShowHints, MID_ShowAnchors },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Debug Raster Cha_nges"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("Debug Raster Changes|No Shortcut"), NULL, NULL, CVMenuShowHints, MID_ShowDebugChanges },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Hori_zontal Metric Lines"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("Horizontal Metric Lines|No Shortcut"), NULL, NULL, CVMenuShowHints, MID_ShowHMetrics },
    { { (unichar_t *) N_("Vertical _Metric Lines"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("Vertical Metric Lines|No Shortcut"), NULL, NULL, CVMenuShowHints, MID_ShowVMetrics },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Snap Outlines to Pi_xel Grid"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'R' }, H_("Snap Outlines to Pixel Grid|No Shortcut"), NULL, NULL, CVMenuSnapOutlines, MID_SnapOutlines },
    GMENUITEM2_EMPTY
};

static GMenuItem2 vwlist[] = {
    { { (unichar_t *) N_("_Fit"), (GImage *) "viewfit.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Fit|No Shortcut"), NULL, NULL, CVMenuScale, MID_Fit },
    { { (unichar_t *) N_("Z_oom out"), (GImage *) "viewzoomout.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Zoom out|No Shortcut"), NULL, NULL, CVMenuScale, MID_ZoomOut },
    { { (unichar_t *) N_("Zoom _in"), (GImage *) "viewzoomin.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'i' }, H_("Zoom in|No Shortcut"), NULL, NULL, CVMenuScale, MID_ZoomIn },
#if HANYANG
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Display Compositions..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'i' }, H_("Display Compositions...|No Shortcut"), NULL, NULL, CVDisplayCompositions, MID_DisplayCompositions },
#endif
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Next Glyph"), (GImage *) "viewnext.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'N' }, H_("Next Glyph|No Shortcut"), NULL, NULL, CVMenuChangeChar, MID_Next },
    { { (unichar_t *) N_("_Prev Glyph"), (GImage *) "viewprev.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Prev Glyph|No Shortcut"), NULL, NULL, CVMenuChangeChar, MID_Prev },
    { { (unichar_t *) N_("Next _Defined Glyph"), (GImage *) "viewnextdef.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'D' }, H_("Next Defined Glyph|No Shortcut"), NULL, NULL, CVMenuChangeChar, MID_NextDef },
    { { (unichar_t *) N_("Prev Defined Gl_yph"), (GImage *) "viewprevdef.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'a' }, H_("Prev Defined Glyph|No Shortcut"), NULL, NULL, CVMenuChangeChar, MID_PrevDef },
    { { (unichar_t *) N_("Form_er Glyph"), (GImage *) "viewformer.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'a' }, H_("Former Glyph|No Shortcut"), NULL, NULL, CVMenuChangeChar, MID_Former },
    { { (unichar_t *) N_("_Goto"), (GImage *) "viewgoto.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'G' }, H_("Goto|No Shortcut"), NULL, NULL, CVMenuGotoChar, MID_Goto },
    { { (unichar_t *) N_("Find In Font _View"), (GImage *) "viewfindinfont.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("Find In Font View|No Shortcut"), NULL, NULL, CVMenuFindInFontView, MID_FindInFontView },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("N_umber Points"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Number Points|No Shortcut"), nplist, nplistcheck, NULL, 0 },
    { { (unichar_t *) N_("Grid Fi_t"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'l' }, H_("Grid Fit|No Shortcut"), gflist, gflistcheck, NULL, MID_ShowGridFit },
    { { (unichar_t *) N_("Sho_w"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'l' }, H_("Show|No Shortcut"), swlist, swlistcheck, NULL, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Com_binations"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'b' }, H_("Combinations|No Shortcut"), cblist, cblistcheck, NULL, 0 },
    GMENUITEM2_LINE,
    { { (unichar_t *) N_("Next _Line in Word List"),     NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'L' }, H_("Next Line in Word List|No Shortcut"), NULL, NULL, CVMenuNextLineInWordList, MID_NextLineInWordList },
    { { (unichar_t *) N_("Previous Line in _Word List"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'W' }, H_("Previous Line in Word List|No Shortcut"), NULL, NULL, CVMenuPrevLineInWordList, MID_PrevLineInWordList },
    GMENUITEM2_EMPTY
};

static void CVMenuShowMMMask(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    uint32_t changemask = (uint32_t) (intptr_t) mi->ti.userdata;
    /* Change which mms get displayed in the "background" */

    if ( mi->mid==MID_MMAll ) {
	if ( (cv->mmvisible&changemask)==changemask ) cv->mmvisible = 0;
	else cv->mmvisible = changemask;
    } else if ( mi->mid == MID_MMNone ) {
	if ( cv->mmvisible==0 ) cv->mmvisible = (1<<(cv->b.sc->parent->mm->instance_count+1))-1;
	else cv->mmvisible = 0;
    } else
	cv->mmvisible ^= changemask;
    GDrawRequestExpose(cv->v,NULL,false);
}

static GMenuItem2 mvlist[] = {
    { { (unichar_t *) N_("SubFonts|_All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0xffffffff, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, '\0' }, H_("All|No Shortcut"), NULL, NULL, CVMenuShowMMMask, MID_MMAll },
    { { (unichar_t *) N_("SubFonts|_None"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, '\0' }, H_("None|No Shortcut"), NULL, NULL, CVMenuShowMMMask, MID_MMNone },
    GMENUITEM2_EMPTY
};

static void mvlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int i, base, j;
    MMSet *mm = cv->b.sc->parent->mm;
    uint32_t submask;
    SplineFont *sub;
    GMenuItem2 *mml;

    base = 3;
    if ( mm==NULL )
	mml = mvlist;
    else {
	mml = calloc(base+mm->instance_count+2,sizeof(GMenuItem2));
	memcpy(mml,mvlist,sizeof(mvlist));
	mml[base-1].ti.fg = mml[base-1].ti.bg = COLOR_DEFAULT;
	mml[base-1].ti.line = true;
	submask = 0;
	for ( j = 0, i=base; j<mm->instance_count+1; ++i, ++j ) {
	    if ( j==0 )
		sub = mm->normal;
	    else
		sub = mm->instances[j-1];
	    mml[i].ti.text = uc_copy(sub->fontname);
	    mml[i].ti.checkable = true;
	    mml[i].ti.checked = (cv->mmvisible & (1<<j))?1:0;
	    mml[i].ti.userdata = (void *) (intptr_t) (1<<j);
	    mml[i].invoke = CVMenuShowMMMask;
	    mml[i].ti.fg = mml[i].ti.bg = COLOR_DEFAULT;
	    if ( sub==cv->b.sc->parent )
		submask = (1<<j);
	}
	/* All */
	mml[0].ti.userdata = (void *) (intptr_t) ((1<<j)-1);
	mml[0].ti.checked = (cv->mmvisible == (uint32_t) (intptr_t) mml[0].ti.userdata);
	    /* None */
	mml[1].ti.checked = (cv->mmvisible == 0 || cv->mmvisible == submask);
    }
    GMenuItemArrayFree(mi->sub);
    mi->sub = GMenuItem2ArrayCopy(mml,NULL);
    if ( mml!=mvlist ) {
	for ( i=base; mml[i].ti.text!=NULL; ++i )
	    free( mml[i].ti.text);
	free(mml);
    }
}

static void CVMenuReblend(GWindow gw, struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    char *err;
    MMSet *mm = cv->b.sc->parent->mm;

    if ( mm==NULL || mm->apple || cv->b.sc->parent!=mm->normal )
return;
    err = MMBlendChar(mm,cv->b.sc->orig_pos);
    if ( mm->normal->glyphs[cv->b.sc->orig_pos]!=NULL )
	_SCCharChangedUpdate(mm->normal->glyphs[cv->b.sc->orig_pos],CVLayer((CharViewBase *)cv->b.sc),-1);
    if ( err!=0 )
	ff_post_error(_("Bad Multiple Master Font"),err);
}

static GMenuItem2 mmlist[] = {
/* GT: Here (and following) MM means "MultiMaster" */
    { { (unichar_t *) N_("MM _Reblend"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("MM Reblend|No Shortcut"), NULL, NULL, CVMenuReblend, MID_MMReblend },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_View"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, NULL, mvlist, mvlistcheck, NULL, 0 },
    GMENUITEM2_EMPTY
};

static void CVMenuShowSubChar(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    SplineFont *new = mi->ti.userdata;
    /* Change to the same char in a different instance font of the mm */

    CVChangeSC(cv,SFMakeChar(new,cv->b.fv->map,CVCurEnc(cv)));
    cv->b.layerheads[dm_grid] = &new->grid;
}

static void mmlistcheck(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e)) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int i, base, j;
    MMSet *mm = cv->b.sc->parent->mm;
    SplineFont *sub;
    GMenuItem2 *mml;

    base = sizeof(mmlist)/sizeof(mmlist[0]);
    if ( mm==NULL )
	mml = mmlist;
    else {
	mml = calloc(base+mm->instance_count+2,sizeof(GMenuItem2));
	memcpy(mml,mmlist,sizeof(mmlist));
	mml[base-1].ti.fg = mml[base-1].ti.bg = COLOR_DEFAULT;
	mml[base-1].ti.line = true;
	for ( j = 0, i=base; j<mm->instance_count+1; ++i, ++j ) {
	    if ( j==0 )
		sub = mm->normal;
	    else
		sub = mm->instances[j-1];
	    mml[i].ti.text = uc_copy(sub->fontname);
	    mml[i].ti.checkable = true;
	    mml[i].ti.checked = sub==cv->b.sc->parent;
	    mml[i].ti.userdata = sub;
	    mml[i].invoke = CVMenuShowSubChar;
	    mml[i].ti.fg = mml[i].ti.bg = COLOR_DEFAULT;
	}
    }
    mml[0].ti.disabled = (mm==NULL || cv->b.sc->parent!=mm->normal || mm->apple);
    GMenuItemArrayFree(mi->sub);
    mi->sub = GMenuItem2ArrayCopy(mml,NULL);
    if ( mml!=mmlist ) {
	for ( i=base; mml[i].ti.text!=NULL; ++i )
	    free( mml[i].ti.text);
	free(mml);
    }
}

static void CVMenuContextualHelp(GWindow UNUSED(gw), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    help("ui/mainviews/charview.html", NULL);
}

static GMenuItem2 mblist[] = {
    { { (unichar_t *) N_("_File"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("File|No Shortcut"), fllist, fllistcheck, NULL, 0 },
    { { (unichar_t *) N_("_Edit"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'E' }, H_("Edit|No Shortcut"), edlist, edlistcheck, NULL, 0 },
    { { (unichar_t *) N_("_Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Point|No Shortcut"), ptlist, ptlistcheck, NULL, 0 },
    { { (unichar_t *) N_("E_lement"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'l' }, H_("Element|No Shortcut"), ellist, ellistcheck, NULL, 0 },
#ifndef _NO_PYTHON
    { { (unichar_t *) N_("_Tools"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'l' }, H_("Tools|No Shortcut"), NULL, cvpy_tllistcheck, NULL, 0 },
#endif
    { { (unichar_t *) N_("H_ints"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'H' }, H_("Hints|No Shortcut"), htlist, htlistcheck, NULL, 0 },
    { { (unichar_t *) N_("_View"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("View|No Shortcut"), vwlist, vwlistcheck, NULL, 0 },
    { { (unichar_t *) N_("_Metrics"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Metrics|No Shortcut"), mtlist, mtlistcheck, NULL, 0 },
/* GT: Here (and following) MM means "MultiMaster" */
    { { (unichar_t *) N_("MM"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("MM|No Shortcut"), mmlist, mmlistcheck, NULL, 0 },
    { { (unichar_t *) N_("_Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'W' }, H_("Window|No Shortcut"), wnmenu, CVWindowMenuBuild, NULL, 0 },
    { { (unichar_t *) N_("_Help"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'H' }, H_("Help|No Shortcut"), helplist, NULL, NULL, 0 },
    GMENUITEM2_EMPTY
};

static GMenuItem2 mblist_nomm[] = {
    { { (unichar_t *) N_("_File"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("File|No Shortcut"), fllist, fllistcheck, NULL, 0 },
    { { (unichar_t *) N_("_Edit"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'E' }, H_("Edit|No Shortcut"), edlist, edlistcheck, NULL, 0 },
    { { (unichar_t *) N_("_Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Point|No Shortcut"), ptlist, ptlistcheck, NULL, 0 },
    { { (unichar_t *) N_("E_lement"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'l' }, H_("Element|No Shortcut"), ellist, ellistcheck, NULL, 0 },
#ifndef _NO_PYTHON
    { { (unichar_t *) N_("_Tools"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, H_("Tools|No Shortcut"), NULL, cvpy_tllistcheck, NULL, 0 },
#endif
    { { (unichar_t *) N_("H_ints"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'H' }, H_("Hints|No Shortcut"), htlist, htlistcheck, NULL, 0 },
    { { (unichar_t *) N_("_View"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("View|No Shortcut"), vwlist, vwlistcheck, NULL, 0 },
    { { (unichar_t *) N_("_Metrics"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Metrics|No Shortcut"), mtlist, mtlistcheck, NULL, 0 },
    { { (unichar_t *) N_("_Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'W' }, H_("Window|No Shortcut"), wnmenu, CVWindowMenuBuild, NULL, 0 },
    { { (unichar_t *) N_("_Help"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, 'H' }, H_("Help|No Shortcut"), helplist, NULL, NULL, 0 },
    GMENUITEM2_EMPTY
};

static void _CharViewCreate(CharView *cv, SplineChar *sc, FontView *fv,int enc,int show) {
    CharViewTab* tab = CVGetActiveTab(cv);
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetData gd;
    int sbsize;
    int as, ds, ld;
    extern int updateflex;
    GTextBounds textbounds;
    /* extern int cv_auto_goto; */
    extern enum cvtools cv_b1_tool, cv_cb1_tool, cv_b2_tool, cv_cb2_tool;

    CVColInit();

    static int firstCharView = 1;
    if( firstCharView )
    {
	firstCharView = 0;
	CVShows.alwaysshowcontrolpoints = prefs_cv_show_control_points_always_initially;
    }

    cv->b.sc = sc;
    tab->scale = .5;
    tab->xoff = tab->yoff = 20;
    cv->b.next = sc->views;
    sc->views = &cv->b;
    cv->b.fv = &fv->b;
    cv->map_of_enc = fv->b.map;
    cv->enc = enc;
    cv->p.pretransform_spl = NULL;
    cv->b.drawmode = dm_fore;

    memset(cv->showback,-1,sizeof(cv->showback));
    if ( !CVShows.showback )
	cv->showback[0] &= ~1;
    cv->showfore = CVShows.showfore;
    cv->showgrids = CVShows.showgrids;
    cv->showhhints = CVShows.showhhints;
    cv->showvhints = CVShows.showvhints;
    cv->showdhints = CVShows.showdhints;
    cv->showpoints = CVShows.showpoints;
    cv->alwaysshowcontrolpoints = CVShows.alwaysshowcontrolpoints;
    cv->showrulers = CVShows.showrulers;
    cv->showfilled = CVShows.showfilled;
    cv->showrounds = CVShows.showrounds;
    cv->showmdx = CVShows.showmdx;
    cv->showmdy = CVShows.showmdy;
    cv->showhmetrics = CVShows.showhmetrics;
    cv->showvmetrics = sc->parent->hasvmetrics ? CVShows.showvmetrics : 0;
    cv->markextrema = CVShows.markextrema;
    cv->showsidebearings = CVShows.showsidebearings;
    cv->showrefnames = CVShows.showrefnames;
    cv->snapoutlines = CVShows.snapoutlines;
    cv->markpoi = CVShows.markpoi;
    cv->showalmosthvlines = CVShows.showalmosthvlines;
    cv->showalmosthvcurves = CVShows.showalmosthvcurves;
    cv->hvoffset = CVShows.hvoffset;
    cv->showblues = CVShows.showblues;
    cv->showfamilyblues = CVShows.showfamilyblues;
    cv->showanchor = CVShows.showanchor;
    cv->showcpinfo = CVShows.showcpinfo;
    cv->showtabs = CVShows.showtabs;
    cv->inPreviewMode = 0;
    cv->checkselfintersects = CVShows.checkselfintersects;

    cv->showdebugchanges = CVShows.showdebugchanges;

    cv->infoh = 13;
#if defined(__MINGW32__)||defined(__CYGWIN__)
    cv->infoh = 26;
#endif
    cv->rulerh = 16;

    GDrawGetSize(cv->gw,&pos);
    memset(&gd,0,sizeof(gd));
    gd.pos.y = cv->mbh+cv->charselectorh+cv->infoh;
    gd.pos.width = sbsize = GDrawPointsToPixels(cv->gw,_GScrollBar_Width);
    gd.pos.height = pos.height-cv->mbh-cv->charselectorh-cv->infoh - sbsize;
    gd.pos.x = pos.width-sbsize;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    cv->vsb = GScrollBarCreate(cv->gw,&gd,cv);

    gd.pos.y = pos.height-sbsize; gd.pos.height = sbsize;
    gd.pos.width = pos.width - sbsize;
    gd.pos.x = 0;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_text_xim;
    cv->hsb = GScrollBarCreate(cv->gw,&gd,cv);

    GDrawGetSize(cv->gw,&pos);
    pos.y = cv->mbh+cv->charselectorh+cv->infoh; pos.height -= cv->mbh + cv->charselectorh + sbsize + cv->infoh;
    pos.x = 0; pos.width -= sbsize;
    if ( cv->showrulers ) {
	pos.y += cv->rulerh; pos.height -= cv->rulerh;
	pos.x += cv->rulerh; pos.width -= cv->rulerh;
    }
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_backcol;
    wattrs.background_color = view_bgcol;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    cv->v = GWidgetCreateSubWindow(cv->gw,&pos,v_e_h,cv,&wattrs);
    GDrawSetWindowTypeName(cv->v, "CharView");

    if ( GDrawRequestDeviceEvents(cv->v,input_em_cnt,input_em)>0 ) {
	/* Success! They've got a wacom tablet */
    }

    cv->small = cv_pointnumberfont.fi;
    GDrawWindowFontMetrics(cv->gw,cv->small,&as,&ds,&ld);
    cv->sfh = as+ds; cv->sas = as;
    GDrawSetFont(cv->gw,cv->small);
    GDrawGetText8Bounds(cv->gw,"0123456789",10,&textbounds);
    cv->sdh = textbounds.as+textbounds.ds+1;
    cv->normal = cv_labelfont.fi;
    GDrawWindowFontMetrics(cv->gw,cv->normal,&as,&ds,&ld);
    cv->nfh = as+ds; cv->nas = as;

    cv->height = pos.height; cv->width = pos.width;
    cv->gi.u.image = calloc(1,sizeof(struct _GImage));
    cv->gi.u.image->image_type = it_mono;
    cv->gi.u.image->clut = calloc(1,sizeof(GClut));
    cv->gi.u.image->clut->trans_index = cv->gi.u.image->trans = 0;
    cv->gi.u.image->clut->clut_len = 2;
    cv->gi.u.image->clut->clut[0] = view_bgcol;
    cv->gi.u.image->clut->clut[1] = fillcol;
    cv->b1_tool = cv_b1_tool; cv->cb1_tool = cv_cb1_tool;
    cv->b1_tool_old = cv->b1_tool;
    cv->b2_tool = cv_b2_tool; cv->cb2_tool = cv_cb2_tool;
    cv->s1_tool = cvt_freehand; cv->s2_tool = cvt_pen;
    cv->er_tool = cvt_knife;
    cv->showing_tool = cvt_pointer;
    cv->pressed_tool = cv->pressed_display = cv->active_tool = cvt_none;
    cv->spacebar_hold = 0;
    cv->b.layerheads[dm_fore] = &sc->layers[ly_fore];
    cv->b.layerheads[dm_back] = &sc->layers[ly_back];
    cv->b.layerheads[dm_grid] = &fv->b.sf->grid;
    if ( !sc->parent->multilayer && fv->b.active_layer!=ly_fore ) {
	cv->b.layerheads[dm_back] = &sc->layers[fv->b.active_layer];
	cv->b.drawmode = dm_back;
    }

#if HANYANG
    if ( sc->parent->rules!=NULL && sc->compositionunit )
	Disp_DefaultTemplate(cv);
#endif

    cv->olde.x = -1;

    cv->ft_dpi = 72; cv->ft_pointsizex = cv->ft_pointsizey = 12.0;
    cv->ft_ppemx = cv->ft_ppemy = 12;

    /*cv->tools = CVMakeTools(cv);*/
    /*cv->layers = CVMakeLayers(cv);*/

    CVFit(cv);
    GDrawSetVisible(cv->v,true);
    GWindowClearFocusGadgetOfWindow(cv->v);

    /*if ( cv_auto_goto )*/		/* Chinese input method steals hot key key-strokes */
	/* But if we don't do this, then people can't type menu short-cuts */
	cv->gic   = GDrawCreateInputContext(cv->v,gic_root|gic_orlesser);
	GDrawSetGIC(cv->v,cv->gic,0,20);
	cv->gwgic = GDrawCreateInputContext(cv->gw,gic_root|gic_orlesser);
	GDrawSetGIC(cv->gw,cv->gwgic,0,20);
	if( show )
	{
	    GDrawSetVisible(cv->gw,true);
	}

    if ( (CharView *) (sc->views)==NULL && updateflex )
	SplineCharIsFlexible(sc,CVLayer((CharViewBase *) cv));
    if ( sc->inspiro && !hasspiro() && !sc->parent->complained_about_spiros ) {
	sc->parent->complained_about_spiros = true;
#ifdef _NO_LIBSPIRO
	ff_post_error(_("You may not use spiros"),_("This glyph should display spiro points, but unfortunately this version of fontforge was not linked with the spiro library, so only normal bezier points will be displayed."));
#else
	ff_post_error(_("You may not use spiros"),_("This glyph should display spiro points, but unfortunately FontForge was unable to load libspiro, spiros are not available for use, and normal bezier points will be displayed instead."));
#endif
    }

}

void DefaultY(GRect *pos) {
    static int nexty=0;
    GRect size;

    GDrawGetSize(GDrawGetRoot(NULL),&size);
    if ( nexty!=0 ) {
	FontView *fv;
	int any=0, i;
	BDFFont *bdf;
	/* are there any open cv/bv windows? */
	for ( fv = fv_list; fv!=NULL && !any; fv = (FontView *) (fv->b.next) ) {
	    for ( i=0; i<fv->b.sf->glyphcnt; ++i ) if ( fv->b.sf->glyphs[i]!=NULL ) {
		if ( fv->b.sf->glyphs[i]->views!=NULL ) {
		    any = true;
	    break;
		}
	    }
	    for ( bdf = fv->b.sf->bitmaps; bdf!=NULL && !any; bdf=bdf->next ) {
		for ( i=0; i<bdf->glyphcnt; ++i ) if ( bdf->glyphs[i]!=NULL ) {
		    if ( bdf->glyphs[i]->views!=NULL ) {
			any = true;
		break;
		    }
		}
	    }
	}
	if ( !any ) nexty = 0;
    }
    pos->y = nexty;
    nexty += 200;
    if ( nexty+pos->height > size.height )
	nexty = 0;
}

static void CharViewInit(void);

static int CV_OnCharSelectorTextChanged( GGadget *g, GEvent *e )
{
    CharView* cv = GGadgetGetUserData(g);
    CharViewTab* tab = CVGetActiveTab(cv);
    SplineChar *sc = cv->b.sc;
    SplineFont* sf = sc->parent;

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged )
    {
	int pos = e->u.control.u.tf_changed.from_pulldown;

	if ( pos!=-1 )
	{
	    int32_t len;
	    GTextInfo **ti = GGadgetGetList(g,&len);
	    GTextInfo *cur = ti[pos];
	    int type = (intptr_t) cur->userdata;
	    if ( type < 0 )
	    {
		TRACE("load wordlist...! pos:%d\n",pos);

		WordlistLoadToGTextInfo( cv->charselector, &cv->charselectoridx );
		return 0;
	    }
	}
	
	cv->charselectoridx = pos;
	char* txt = GGadgetGetTitle8( cv->charselector );
	TRACE("char selector @%d changed to:%s\n", pos, txt );
	{
	    int tabnum = GTabSetGetSel(cv->tabs);
	    CharViewTab* t = &cv->cvtabs[tabnum];
	    strncpy( t->tablabeltxt, txt, charviewtab_charselectedsz );
	    TRACE("tab num: %d set to %s\n", tabnum, t->tablabeltxt);
	    GTabSetChangeTabName(cv->tabs,t->tablabeltxt,tabnum);
	    GTabSetRemetric(cv->tabs);
	    GTabSetSetSel(cv->tabs,tabnum);	/* This does a redraw */
	}
	
	memset( cv->additionalCharsToShow, 0, sizeof(SplineChar*) * additionalCharsToShowLimit );
	cv->additionalCharsToShowActiveIndex = 0;
	cv->additionalCharsToShow[0] = cv->b.sc;

	int hadSelection = 0;
	if( txt[0] == '\0' )
	{
	    CVSetCharSelectorValueFromSC( cv, cv->b.sc );
	}
	else if( strlen(txt) > 1 )
	{
	    int i=0;
	    unichar_t *ret = GGadgetGetTitle( cv->charselector );
	    WordListLine wll = WordlistEscapedInputStringToParsedData( sf, ret );
	    WordListLine pt = wll;
	    WordListLine ept = WordListLine_end(wll);
	    WordListLine tpt = 0;
	    for ( tpt=pt; tpt<ept; ++tpt )
	    {
		if( tpt == pt )
		{
		    // your own char at the leading of the text
		    cv->additionalCharsToShow[i] = tpt->sc;
		    i++;
		    continue;
		}
		
		cv->additionalCharsToShow[i] = tpt->sc;

		i++;
		if( i >= additionalCharsToShowLimit )
		    break;
	    }
	    free(ret);

	    if( wll->sc )
	    {
		if( wll->isSelected )
		{
		    // first char selected, nothing to do!
		}
		else
		{
		    while( wll->sc && !(wll->isSelected))
			wll++;
		    if( wll->sc && wll->isSelected )
		    {
			SplineChar* xc = wll->sc;
			if( xc )
			{
			    TRACE("selected v:%d xc:%s\n", wll->currentGlyphIndex, xc->name );
			    int xoff = tab->xoff;
			    CVSwitchActiveSC( cv, xc, wll->currentGlyphIndex );
			    CVHScrollSetPos( cv, xoff );
			    hadSelection = 1;
			}
			
		    }
		}
	    }
	}
	free(txt);

	int i=0;
	for( i=0; cv->additionalCharsToShow[i]; i++ )
	{
	    TRACE("i:%d %p .. ", i, cv->additionalCharsToShow[i] );
	    TRACE(" %s\n", cv->additionalCharsToShow[i]->name );
	}

	if( !hadSelection )
	    CVSwitchActiveSC( cv, 0, 0 );
	GDrawRequestExpose(cv->v,NULL,false);
    }
    return( true );
}

GTextInfo cv_charselector_init[] = {
    { (unichar_t *) "", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { NULL, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, 0, '\0'},
    { (unichar_t *) N_("Load Word List..."), NULL, 0, 0, (void *) -1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
//    { (unichar_t *) N_("Load Glyph Name List..."), NULL, 0, 0, (void *) -2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};

CharView *CharViewCreateExtended(SplineChar *sc, FontView *fv,int enc, int show )
{
    CharView *cv = calloc(1,sizeof(CharView));
    GWindowAttrs wattrs;
    GRect pos, zoom;
    GWindow gw;
    GGadgetData gd;
    GTabInfo aspects[2];
    GRect gsize;
    char buf[300];
    GTextInfo label[9];

    CharViewInit();

    cv->b.sc = sc;
    cv->b.fv = &fv->b;
    cv->enc = enc;
    cv->map_of_enc = fv->b.map;		/* I know this is done again in _CharViewCreate, but it needs to be done before creating the title */

    cv->infoh = 13;
#if defined(__MINGW32__)||defined(__CYGWIN__)
    cv->infoh = 26;
#endif
    cv->rulerh = 16;

    cv->ctpos = -1;


    SCLigCaretCheck(sc,false);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_utf8_ititle;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.utf8_icon_title = (const char*)CVMakeTitles(cv,buf,sizeof(buf));
    wattrs.utf8_window_title = buf;
    wattrs.icon = CharIcon(cv, fv);
    if ( wattrs.icon )
	wattrs.mask |= wam_icon;
    pos.x = GGadgetScale(104)+6;
    pos.width = (cv_width > 0) ? cv_width : default_cv_width;
    pos.height = (cv_height > 0) ? cv_height : default_cv_height;
    DefaultY(&pos);

    cv->gw = gw = GDrawCreateTopWindow(NULL,&pos,cv_e_h,cv,&wattrs);
    free( (unichar_t *) wattrs.icon_title );
    free((char*)wattrs.utf8_icon_title);
    GDrawSetWindowTypeName(cv->gw, "CharView");

    // FIXME: cant do this until gw is shown?
    GTextBounds textbounds;
    GDrawGetText8Bounds(cv->gw,"0123456789hl",10,&textbounds);
    TRACE("XXXXXX as:%d  ds:%d\n",  textbounds.as, textbounds.ds );
    cv->charselectorh = textbounds.as+textbounds.ds+1;
    TRACE("XXXXXX h:%d\n", GDrawGetText8Height( cv->gw, "0123456AZgplh", 10));
    cv->charselectorh = 35;


    GDrawGetSize(GDrawGetRoot(screen_display),&zoom);
    zoom.x = CVPalettesWidth(); zoom.width -= zoom.x-10;
    zoom.height -= 30;			/* Room for title bar & such */
    GDrawSetZoom(gw,&zoom,-1);

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    helplist[0].invoke = CVMenuContextualHelp;
    gd.u.menu2 = sc->parent->mm==NULL ? mblist_nomm : mblist;
    cv->mb = GMenu2BarCreate( gw, &gd, NULL);
    GGadgetGetSize(cv->mb,&gsize);
    cv->mbh = gsize.height;

//    TRACE("pos.x:%d pos.y:%d pos.w:%d pos.h:%d\n", pos.x, pos.y, pos.width, pos.height );
    GDrawGetSize(cv->gw,&pos);
    memset(&gd,0,sizeof(gd));
//    gd.pos.x = pos.x;
    gd.pos.x = 3;
    gd.pos.y = cv->mbh+2;
    gd.pos.height = cv->charselectorh-4;
    gd.pos.width  = cv_width / 2;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_text_xim;
    gd.handle_controlevent = CV_OnCharSelectorTextChanged;
    gd.u.list = cv_charselector_init;
    cv->charselector = GListFieldCreate(cv->gw,&gd,cv);
    CVSetCharSelectorValueFromSC( cv, sc );
    GGadgetSetSkipUnQualifiedHotkeyProcessing( cv->charselector, 1 );

    // Up and Down buttons for moving through the word list.
    {
        GGadgetData xgd = gd;
        gd.pos.width += 2 * xgd.pos.height + 4;
        memset(label, '\0', sizeof(GTextInfo));
        xgd.pos.x += xgd.pos.width + 2;
        xgd.pos.width = xgd.pos.height;
        xgd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
        xgd.handle_controlevent = CVMoveToPrevInWordList;
        xgd.label = &label[0];
        label[0].text = (unichar_t *) "⇞";
        label[0].text_is_1byte = true;
        cv->charselectorPrev = GButtonCreate(cv->gw,&xgd,cv);
        memset(label, '\0', sizeof(GTextInfo));
        xgd.pos.x += xgd.pos.width + 2;
        xgd.pos.width = xgd.pos.height;
        xgd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
        xgd.handle_controlevent = CVMoveToNextInWordList;
        xgd.label = &label[0];
        label[0].text = (unichar_t *) "⇟";
        label[0].text_is_1byte = true;
        cv->charselectorNext = GButtonCreate(cv->gw,&xgd,cv);
    }
    

    memset(aspects,0,sizeof(aspects));
    aspects[0].text = (unichar_t *) sc->name;
    aspects[0].text_is_1byte = true;
	/* NOT visible until we add a second glyph to the stack */
    gd.flags = gg_enabled|gg_tabset_nowindow|gg_tabset_scroll|gg_pos_in_pixels;
    gd.u.menu = NULL;
    gd.u.tabs = aspects;
    gd.pos.x = 0;
    gd.pos.y = cv->mbh;
    gd.handle_controlevent = CVChangeToFormer;
    cv->tabs = GTabSetCreate( gw, &gd, NULL );
    cv->former_cnt = 1;
    cv->former_names[0] = copy(sc->name);
    GTabSetSetClosable(cv->tabs, true);
    GTabSetSetMovable(cv->tabs, true);
    GTabSetSetRemoveSync(cv->tabs, CVTabSetRemoveSync);
    GTabSetSetSwapSync(cv->tabs, CVTabSetSwapSync);
    GGadgetTakesKeyboard(cv->tabs,false);

    _CharViewCreate( cv, sc, fv, enc, show );
    // Frank wants to avoid needing to implement scaling twice.
    CVResize(cv);

    return( cv );
}

CharView *CharViewCreate(SplineChar *sc, FontView *fv,int enc)
{
    return CharViewCreateExtended( sc, fv, enc, 1 );
}

void CharViewFree(CharView *cv) {
    int i;

    if ( cv->qg != NULL )
	QGRmCharView(cv->qg,cv);
    BDFCharFree(cv->filled);
    if ( cv->ruler_w ) {
	GDrawDestroyWindow(cv->ruler_w);
	cv->ruler_w = NULL;
    }
    if ( cv->ruler_linger_w ) {
	GDrawDestroyWindow(cv->ruler_linger_w);
	cv->ruler_linger_w = NULL;
    }
    free(cv->gi.u.image->clut);
    free(cv->gi.u.image);
#if HANYANG
    if ( cv->jamodisplay!=NULL )
	Disp_DoFinish(cv->jamodisplay,true);
#endif

    CVDebugFree(cv->dv);

    SplinePointListsFree(cv->b.gridfit);
    FreeType_FreeRaster(cv->oldraster);
    FreeType_FreeRaster(cv->raster);

    CVDebugFree(cv->dv);

    for ( i=0; i<cv->former_cnt; ++i )
	free(cv->former_names[i]);

    free(cv->ruler_intersections);
    free(cv);
}

int CVValid(SplineFont *sf, SplineChar *sc, CharView *cv) {
    /* A charview may have been closed. A splinechar may have been removed */
    /*  from a font */
    CharView *test;

    if ( cv->b.sc!=sc || sc->parent!=sf )
return( false );
    if ( sc->orig_pos<0 || sc->orig_pos>sf->glyphcnt )
return( false );
    if ( sf->glyphs[sc->orig_pos]!=sc )
return( false );
    for ( test=(CharView *) (sc->views); test!=NULL; test=(CharView *) (test->b.next) )
	if ( test==cv )
return( true );

return( false );
}

static int charview_ready = false;

static void CharViewFinish() {
  // The memory leak is limited and reachable.
  if ( !charview_ready ) return;
    charview_ready = false;
    mb2FreeGetText(mblist);
    mb2FreeGetText(spiroptlist);
    int i;
    for ( i=0; mblist_nomm[i].ti.text!=NULL; ++i ) {
      free(mblist_nomm[i].ti.text_untranslated); mblist_nomm[i].ti.text_untranslated = NULL;
    }
}

void CharViewFinishNonStatic() {
  CharViewFinish();
}

#ifndef _NO_PYTHON
void CVSetToolsSubmenu(GMenuItem2 *py_menu) {
    mblist[4].ti.disabled = mblist_nomm[4].ti.disabled = (py_menu == NULL);
    mblist[4].sub = mblist_nomm[4].sub = py_menu;
}
static GMenuItem2 *CVGetToolsSubmenu(void) {
    return mblist[4].sub;
}
#endif

static void CharViewInit(void) {
    int i;
    // static int done = false; // superseded by charview_ready.

    if ( charview_ready )
return;
    charview_ready = true;
//    TRACE("CharViewInit(top) mblist[0].text before translation: %s\n", mblist[0].ti.text );

// The tools menu handles its own translation. I would rather do this by testing
// whether ti.text_untranslated is already null but there are hundreds of missing
// initializers for that field in the menu layout code.
#ifndef _NO_PYTHON
    GMenuItem2 *t = CVGetToolsSubmenu();
    CVSetToolsSubmenu(NULL);
#endif
    mb2DoGetText(mblist);
#ifndef _NO_PYTHON
    CVSetToolsSubmenu(t);
#endif

//    TRACE("CharViewInit(2) mblist[0].text after    translation: %s\n", u_to_c(mblist[0].ti.text) );
//    TRACE("CharViewInit(2) mblist[0].text_untranslated notrans: %s\n", mblist[0].ti.text_untranslated );

    mb2DoGetText(spiroptlist);
    for ( i=0; mblist_nomm[i].ti.text!=NULL; ++i )
    {
	// Note that because we are doing this ourself we have to set
	// the text_untranslated ourself too.
 	if( mblist_nomm[i].shortcut )
	    mblist_nomm[i].ti.text_untranslated = copy(mblist_nomm[i].shortcut);
	else
	    mblist_nomm[i].ti.text_untranslated = copy((char*)mblist_nomm[i].ti.text);

	mblist_nomm[i].ti.text = (unichar_t *) _((char *) mblist_nomm[i].ti.text);
    }
    atexit(&CharViewFinishNonStatic);
}

static int nested_cv_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	InfoExpose(cv,gw,event);
	CVLogoExpose(cv,gw,event);
      break;
      case et_char:
	(cv->b.container->funcs->charEvent)(cv->b.container,event);
      break;
      case et_charup:
	CVCharUp(cv,event);
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    if ( event->u.control.g == cv->hsb )
		CVHScroll(cv,&event->u.control.u.sb);
	    else
		CVVScroll(cv,&event->u.control.u.sb);
	  break;
	  default: break;
	}
      break;
      case et_map:
	if ( event->u.map.is_visible )
	    CVPaletteActivate(cv);
	else
	    CVPalettesHideIfMine(cv);
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    CVResize(cv);
      break;
      case et_destroy:
	if ( cv->backimgs!=NULL ) {
	    GDrawDestroyWindow(cv->backimgs);
	    cv->backimgs = NULL;
	}
      break;
      case et_mouseup: case et_mousedown:
	GGadgetEndPopup();
	CVPaletteActivate(cv);
      break;
      default: break;
    }
return( true );
}

void SVCharViewInits(SearchView *sv) {
    GGadgetData gd;
    GWindowAttrs wattrs;
    GRect pos, gsize;

    CharViewInit();

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    helplist[0].invoke = CVMenuContextualHelp;
    gd.u.menu2 = mblist_nomm;
    sv->mb = GMenu2BarCreate( sv->gw, &gd, NULL);
    GGadgetGetSize(sv->mb,&gsize);
    sv->mbh = gsize.height;

    pos.y = sv->mbh+sv->fh+10; pos.height = 220;
    pos.width = pos.height; pos.x = 10+pos.width+20;	/* Do replace first so palettes appear properly */
    sv->rpl_x = pos.x; sv->cv_y = pos.y;
    sv->cv_height = pos.height; sv->cv_width = pos.width;
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    sv->cv_rpl.gw = GWidgetCreateSubWindow(sv->gw,&pos,nested_cv_e_h,&sv->cv_rpl,&wattrs);
    _CharViewCreate(&sv->cv_rpl, &sv->sd.sc_rpl, &sv->dummy_fv, 1, 1);

    pos.x = 10;
    sv->cv_srch.gw = GWidgetCreateSubWindow(sv->gw,&pos,nested_cv_e_h,&sv->cv_srch,&wattrs);
    _CharViewCreate(&sv->cv_srch, &sv->sd.sc_srch, &sv->dummy_fv, 0, 1);
}

/* Same for the MATH Kern dlg */

void MKDCharViewInits(MathKernDlg *mkd) {
    GGadgetData gd;
    GWindowAttrs wattrs;
    GRect pos, gsize;
    int i;

    CharViewInit();

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    helplist[0].invoke = CVMenuContextualHelp;
    gd.u.menu2 = mblist_nomm;
    mkd->mb = GMenu2BarCreate( mkd->gw, &gd, NULL);
    GGadgetGetSize(mkd->mb,&gsize);
    mkd->mbh = gsize.height;

    mkd->mid_space = 20;
    for ( i=3; i>=0; --i ) {	/* Create backwards so palettes get set in topright (last created) */
	pos.y = mkd->fh+10; pos.height = 220;	/* Size doesn't matter, adjusted later */
	pos.width = pos.height; pos.x = 10+i*(pos.width+20);
	mkd->cv_y = pos.y;
	mkd->cv_height = pos.height; mkd->cv_width = pos.width;
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor;
	wattrs.event_masks = -1;
	wattrs.cursor = ct_mypointer;
	(&mkd->cv_topright)[i].gw = GWidgetCreateSubWindow(mkd->cvparent_w,&pos,nested_cv_e_h,(&mkd->cv_topright)+i,&wattrs);
	_CharViewCreate((&mkd->cv_topright)+i, (&mkd->sc_topright)+i, &mkd->dummy_fv, i, 1);
    }
}

/* Same for the Tile Path dlg */

#ifdef FONTFORGE_CONFIG_TILEPATH

void TPDCharViewInits(TilePathDlg *tpd, int cid) {
    GGadgetData gd;
    GWindowAttrs wattrs;
    GRect pos, gsize;
    int i;

    CharViewInit();

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    helplist[0].invoke = CVMenuContextualHelp;
    gd.u.menu2 = mblist_nomm;
    tpd->mb = GMenu2BarCreate( tpd->gw, &gd, NULL);
    GGadgetGetSize(tpd->mb,&gsize);
    tpd->mbh = gsize.height;

    tpd->mid_space = 20;
    for ( i=3; i>=0; --i ) {	/* Create backwards so palettes get set in topright (last created) */
	pos.y = 0; pos.height = 220;	/* Size doesn't matter, adjusted later */
	pos.width = pos.height; pos.x = 0;
	tpd->cv_y = pos.y;
	tpd->cv_height = pos.height; tpd->cv_width = pos.width;
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor;
	wattrs.event_masks = -1;
	wattrs.cursor = ct_mypointer;
	(&tpd->cv_first)[i].gw = GWidgetCreateSubWindow(GDrawableGetWindow(GWidgetGetControl(tpd->gw,cid+i)),
		&pos,nested_cv_e_h,(&tpd->cv_first)+i,&wattrs);
	_CharViewCreate((&tpd->cv_first)+i, (&tpd->sc_first)+i, &tpd->dummy_fv, i, 1 );
    }
}

void PTDCharViewInits(TilePathDlg *tpd, int cid) {
    GGadgetData gd;
    GWindowAttrs wattrs;
    GRect pos, gsize;

    CharViewInit();

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    helplist[0].invoke = CVMenuContextualHelp;
    gd.u.menu2 = mblist_nomm;
    tpd->mb = GMenu2BarCreate( tpd->gw, &gd, NULL);
    GGadgetGetSize(tpd->mb,&gsize);
    tpd->mbh = gsize.height;

    tpd->mid_space = 20;
     {
	pos.y = 0; pos.height = 220;	/* Size doesn't matter, adjusted later */
	pos.width = pos.height; pos.x = 0;
	tpd->cv_y = pos.y;
	tpd->cv_height = pos.height; tpd->cv_width = pos.width;
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor;
	wattrs.event_masks = -1;
	wattrs.cursor = ct_mypointer;
	tpd->cv_first.gw = GWidgetCreateSubWindow(GDrawableGetWindow(GWidgetGetControl(tpd->gw,cid)),
		&pos,nested_cv_e_h,&tpd->cv_first,&wattrs);
	_CharViewCreate(&tpd->cv_first, &tpd->sc_first, &tpd->dummy_fv, 0, 1 );
    }
}
#endif		/* TilePath */

void GDDCharViewInits(GradientDlg *gdd, int cid) {
    GGadgetData gd;
    GWindowAttrs wattrs;
    GRect pos, gsize;

    CharViewInit();

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    helplist[0].invoke = CVMenuContextualHelp;
    gd.u.menu2 = mblist_nomm;
    gdd->mb = GMenu2BarCreate( gdd->gw, &gd, NULL);
    GGadgetGetSize(gdd->mb,&gsize);
    gdd->mbh = gsize.height;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;

    pos.y = 1; pos.height = 220;
    pos.width = pos.height; pos.x = 0;
    gdd->cv_grad.gw = GWidgetCreateSubWindow(
	    GDrawableGetWindow(GWidgetGetControl(gdd->gw,cid)),
	    &pos,nested_cv_e_h,&gdd->cv_grad,&wattrs);
    _CharViewCreate(&gdd->cv_grad, &gdd->sc_grad, &gdd->dummy_fv, 0, 1 );
}

void StrokeCharViewInits(StrokeDlg *sd, int cid) {
    GGadgetData gd;
    GWindowAttrs wattrs;
    GRect pos, gsize;

    CharViewInit();

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    helplist[0].invoke = CVMenuContextualHelp;
    gd.u.menu2 = mblist_nomm;
    sd->mb = GMenu2BarCreate( sd->gw, &gd, NULL);
    GGadgetGetSize(sd->mb,&gsize);
    sd->mbh = gsize.height;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;

    pos.y = 1; pos.height = 220;
    pos.width = pos.height; pos.x = 0;
    sd->cv_stroke.gw = GWidgetCreateSubWindow(
	    GDrawableGetWindow(GWidgetGetControl(sd->gw,cid)),
	    &pos,nested_cv_e_h,&sd->cv_stroke,&wattrs);
    _CharViewCreate(&sd->cv_stroke, &sd->sc_stroke, &sd->dummy_fv, 0, 1 );
}

static void SC_CloseAllWindows(SplineChar *sc) {
    CharViewBase *cv, *next;
    BitmapView *bv, *bvnext;
    BDFFont *bdf;
    BDFChar *bfc;
    FontView *fvs;

    if ( sc->views ) {
	for ( cv = sc->views; cv!=NULL; cv=next ) {
	    next = cv->next;
	    GDrawDestroyWindow(((CharView *) cv)->gw);
	}
	GDrawSync(NULL);
	GDrawProcessPendingEvents(NULL);
	GDrawSync(NULL);
	GDrawProcessPendingEvents(NULL);
    }

    for ( bdf=sc->parent->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	if ( sc->orig_pos<bdf->glyphcnt && (bfc = bdf->glyphs[sc->orig_pos])!= NULL ) {
	    if ( bfc->views!=NULL ) {
		for ( bv= bfc->views; bv!=NULL; bv=bvnext ) {
		    bvnext = bv->next;
		    GDrawDestroyWindow(bv->gw);
		}
		GDrawSync(NULL);
		GDrawProcessPendingEvents(NULL);
		GDrawSync(NULL);
		GDrawProcessPendingEvents(NULL);
	    }
	}
    }

    /* Turn any searcher references to this glyph into inline copies of it */
    for ( fvs=(FontView *) sc->parent->fv; fvs!=NULL; fvs=(FontView *) fvs->b.nextsame ) {
	if ( fvs->sv!=NULL ) {
	    RefChar *rf, *rnext;
	    for ( rf = fvs->sv->sd.sc_srch.layers[ly_fore].refs; rf!=NULL; rf=rnext ) {
		rnext = rf->next;
		if ( rf->sc==sc )
		    SCRefToSplines(&fvs->sv->sd.sc_srch,rf,ly_fore);
	    }
	    for ( rf = fvs->sv->sd.sc_rpl.layers[ly_fore].refs; rf!=NULL; rf=rnext ) {
		rnext = rf->next;
		if ( rf->sc==sc )
		    SCRefToSplines(&fvs->sv->sd.sc_rpl,rf,ly_fore);
	    }
	}
    }
}

struct sc_interface gdraw_sc_interface = {
    SC_UpdateAll,
    SC_OutOfDateBackground,
    SC_RefreshTitles,
    SC_HintsChanged,
    SC_CharChangedUpdate,
    _SC_CharChangedUpdate,
    SC_MarkInstrDlgAsChanged,
    SC_CloseAllWindows,
    SC_MoreLayers
};

static void UI_CVGlyphRenameFixup(SplineFont *sf, const char *oldname, const char *newname) {
    int gid, i;
    SplineChar *sc;
    CharView *cv;

    if ( no_windowing_ui )
return;

    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL) {
	for ( cv=(CharView *) (sc->views); cv!=NULL; cv = (CharView *) (cv->b.next)) {
	    for ( i=0; i<cv->former_cnt; ++i ) if ( strcmp(oldname,cv->former_names[i])==0 ) {
		free( cv->former_names[i] );
		cv->former_names[i] = copy( newname );
		if ( cv->tabs!=NULL ) {
		    GTabSetChangeTabName(cv->tabs,newname,i);
		    GTabSetRemetric(cv->tabs);
		    GGadgetRedraw(cv->tabs);
		}
	    }
	}
    }
}

struct cv_interface gdraw_cv_interface = {
    (void (*)(CharViewBase *)) CV_CharChangedUpdate,
    (void (*)(CharViewBase *,int)) _CV_CharChangedUpdate,
    UI_CVGlyphRenameFixup,
    CV_LayerPaletteCheck
};


int CVPalettesRIInit(GResInfo *ri) {
    extern GBox _ggadget_Default_Box;
    extern GResInfo ggadget_ri;
    if ( ri->is_initialized )
	return false;
    GResEditDoInit(&ggadget_ri);
    cvpalettefgcol = GDrawGetDefaultForeground(NULL);
    cvpalettebgcol = GDrawGetDefaultBackground(NULL);
    cvpaletteactborcol = _ggadget_Default_Box.active_border;
    return _GResEditInitialize(ri);
}

extern GResInfo bitmapview_ri;
GResInfo cvpalettes_ri = {
    &bitmapview_ri, NULL,NULL, NULL,
    NULL,
    NULL,
    NULL,
    cvpalettes_re,
    N_("Palettes"),
    N_("Colors, etc. of the Outline, Bitmap, and Layer palettes"),
    "CharView",
    "fontforge",
    false,
    false,
    0,
    GBOX_EMPTY,
    GBOX_EMPTY,
    NULL,
    CVPalettesRIInit,
    NULL
};
GResInfo charviewraster_ri = {
    &cvpalettes_ri, NULL,NULL, NULL,
    NULL,
    NULL,
    NULL,
    charviewraster_re,
    N_("Outline Raster"),
    N_("Colors related to images and font rasterizing"),
    "CharView",
    "fontforge",
    false,
    false,
    0,
    GBOX_EMPTY,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
GResInfo charviewhints_ri = {
    &charviewraster_ri, NULL,NULL, NULL,
    NULL,
    NULL,
    NULL,
    charviewhints_re,
    N_("Outline Hints"),
    N_("Colors related to CFF/PostScript hints"),
    "CharView",
    "fontforge",
    false,
    false,
    0,
    GBOX_EMPTY,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
GResInfo charviewtools_ri = {
    &charviewhints_ri, NULL,NULL, NULL,
    NULL,
    NULL,
    NULL,
    charviewtools_re,
    N_("Outline Tools"),
    N_("Colors, etc. related to tool use in outline window"),
    "CharView",
    "fontforge",
    false,
    false,
    0,
    GBOX_EMPTY,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
GResInfo charviewlinesfills_ri = {
    &charviewtools_ri, NULL,NULL, NULL,
    NULL,
    NULL,
    NULL,
    charviewlinesfills_re,
    N_("Outline Lines/Fills"),
    N_("Colors of lines and fills in outline window"),
    "CharView",
    "fontforge",
    false,
    false,
    0,
    GBOX_EMPTY,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
GResInfo charviewpoints_ri = {
    &charviewlinesfills_ri, NULL,NULL, NULL,
    NULL,
    NULL,
    NULL,
    charviewpoints_re,
    N_("Outline Points"),
    N_("Colors, etc of points in glyph outline window"),
    "CharView",
    "fontforge",
    false,
    false,
    0,
    GBOX_EMPTY,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};


void SPSelectNextPoint( SplinePoint *sp, int state )
{
    if( !sp )
	return;
    if( !sp->next )
	return;
    if( !sp->next->to )
	return;
    sp->next->to->selected = state;
}

void SPSelectPrevPoint( SplinePoint *sp, int state )
{
    if( !sp )
	return;
    if( !sp->prev )
	return;
    if( !sp->prev->from )
	return;
    sp->prev->from->selected = state;
}



bool SPIsNextCPSelectedSingle( SplinePoint *sp, CharView *cv )
{
    if( cv )
    {
	int iscurrent = sp == (cv->p.sp!=NULL?cv->p.sp:cv->lastselpt);
	if( iscurrent && cv->p.nextcp )
	    return true;
    }
    return false;
}


bool SPIsNextCPSelected( SplinePoint *sp, CharView *cv )
{
    if( SPIsNextCPSelectedSingle( sp, cv ))
	return true;
    return sp->nextcpselected;
}

bool SPIsPrevCPSelectedSingle( SplinePoint *sp, CharView *cv )
{
    if( cv )
    {
	int iscurrent = sp == (cv->p.sp!=NULL?cv->p.sp:cv->lastselpt);
	if( iscurrent && cv->p.prevcp )
	    return true;
    }
    return false;
}

bool SPIsPrevCPSelected( SplinePoint *sp, CharView *cv )
{
    if( SPIsPrevCPSelectedSingle( sp, cv ))
    {
	return true;
    }
    return sp->prevcpselected;
}

bool CVShouldInterpolateCPsOnMotion( CharView* cv )
{
    bool ret = interpCPsOnMotion;

    if( cv->activeModifierControl && cv->activeModifierAlt )
	ret = !ret;
    
    return ret;
}

