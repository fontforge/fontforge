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
#include "cvundoes.h"
#include "fontforgeui.h"
#include "spiro.h"
#include "splinefill.h"
#include "splineorder2.h"
#include "splineutil.h"
#include "splineutil2.h"
#include "collabclientui.h"

int palettes_docked=1;
int rectelipse=0, polystar=0, regular_star=1;
int center_out[2] = { false, true };
float rr_radius=0;
int ps_pointcnt=6;
float star_percent=1.7320508;	/* Regular 6 pointed star */
extern int interpCPsOnMotion;

#include <gkeysym.h>
#include <math.h>
#include "splinefont.h"
#include <ustring.h>
#include <utype.h>
#include <gresource.h>
#include "charview_private.h"
#include "gdraw/hotkeys.h"

static void CVLCheckLayerCount(CharView *cv, int resize);

extern void CVDebugFree(DebugView *dv);

extern GBox _ggadget_Default_Box;
#define ACTIVE_BORDER   (_ggadget_Default_Box.active_border)
#define MAIN_FOREGROUND (_ggadget_Default_Box.main_foreground)

extern GDevEventMask input_em[];
extern const int input_em_cnt;

int cvvisible[2] = { 1, 1}, bvvisible[3]= { 1,1,1 };
static GWindow cvlayers, cvtools, bvlayers, bvtools, bvshades;
static GWindow cvlayers2=NULL;

#define LSHOW_CUBIC   1
#define LSHOW_FG      2
#define LSHOW_PREVIEW 4
static int layerscols = LSHOW_CUBIC|LSHOW_FG|LSHOW_PREVIEW; /* which columns to show in layers1 palette */
static int layer_height = 0;        /* height of each layer row in layers1 palette */
static int layer_header_height = 0; /* height of initial stuff in layers1 palette  */
static int layer_footer_height = 0; /* height of +/- buttons at bottom of layers1 palette */
static int layers_max = 2; /* Maximum number of layers for which widgets have been allocated in layers1 palette */
struct l2 {
    int active;           /* index of the active layer */
    int offtop;           /* first layer to show at the top line in layers palette */
    int visible_layers;   /* number of layers apart from the guides layer to show before using a scrollbar      */
    int current_layers;   /* number of layers for the current character, and the number used in l2.layers       */
    int max_layers;       /* maximum number of layers for which layer controls and previews have been allocated */
    BDFChar **layers;     /* layer thumbnail previews              */
    int sb_start;         /* x pixel position of the scrollbar     */
    int column_width;     /* width of various indicator columns    */
    int header_height;    /* height of the header in pixels before the first layer */
    int mo_col, mo_layer; /* mouse over column and layer           */
    int rename_active;    /* If >=2, layer number for which the edit box for layer names is active */
    GClut *clut;
    GFont *font;          /* font to draw text in the palette with */
} layerinfo = {           /* info about the current layers in the layers palette */
    2, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 0, 0, NULL, NULL
};

struct l2 layer2 = { 2, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 0, 0, NULL, NULL };
static int layers2_active = -1;
static GPoint cvtoolsoff = { -9999, -9999 }, cvlayersoff = { -9999, -9999 }, bvlayersoff = { -9999, -9999 }, bvtoolsoff = { -9999, -9999 }, bvshadesoff = { -9999, -9999 };
int palettes_fixed=1;
static GCursor tools[cvt_max+1] = { ct_pointer }, spirotools[cvt_max+1];

enum cvtools cv_b1_tool = cvt_pointer, cv_cb1_tool = cvt_pointer,
	     cv_b2_tool = cvt_magnify, cv_cb2_tool = cvt_ruler;

static GFont *toolsfont=NULL, *layersfont=NULL;

#define CV_LAYERS_WIDTH		104
#define CV_LAYERS_HEIGHT	100
#define CV_LAYERS_INITIALCNT	6
#define CV_LAYERS_LINE_HEIGHT	25
#define CV_LAYERS2_WIDTH	185
#define CV_LAYERS2_HEIGHT	126
#define CV_LAYERS2_LINE_HEIGHT	25
#define CV_LAYERS2_HEADER_HEIGHT	20
#define BV_TOOLS_WIDTH		53
#define BV_TOOLS_HEIGHT		80
#define BV_LAYERS_HEIGHT	73
#define BV_LAYERS_WIDTH		73
#define BV_SHADES_HEIGHT	(8+9*16)

/* These are control ids for the layers palette controls */
#define CID_VBase	1000
#define CID_VGrid	(CID_VBase+ly_grid)
#define CID_VBack	(CID_VBase+ly_back)
#define CID_VFore	(CID_VBase+ly_fore)

#define CID_EBase	3000
#define CID_EGrid	(CID_EBase+ly_grid)
#define CID_EBack	(CID_EBase+ly_back)
#define CID_EFore	(CID_EBase+ly_fore)

#define CID_QBase	5000
#define CID_QGrid	(CID_QBase+ly_grid)
#define CID_QBack	(CID_QBase+ly_back)
#define CID_QFore	(CID_QBase+ly_fore)

#define CID_FBase	7000

#define CID_SB		8000
#define CID_Edit	8001

#define CID_AddLayer    9000
#define CID_RemoveLayer 9001
#define CID_RenameLayer 9002
#define CID_LayersMenu  9003
#define CID_LayerLabel  9004

void onCollabSessionStateChanged( gpointer instance, FontViewBase* fv, gpointer user_data )
{
    bool inCollab = collabclient_inSessionFV( fv );

    if (cvlayers != NULL) {
      GGadgetSetEnabled(GWidgetGetControl(cvlayers,CID_AddLayer),    !inCollab );
      GGadgetSetEnabled(GWidgetGetControl(cvlayers,CID_RemoveLayer), !inCollab );
      GGadgetSetEnabled(GWidgetGetControl(cvlayers,CID_RenameLayer), !inCollab );
    } else if (cvlayers2 != NULL && 0) {
      // These controls seem not to exist in cvlayers2. We can look deeper into this later.
      GGadgetSetEnabled(GWidgetGetControl(cvlayers2,CID_AddLayer),    !inCollab );
      GGadgetSetEnabled(GWidgetGetControl(cvlayers2,CID_RemoveLayer), !inCollab );
      GGadgetSetEnabled(GWidgetGetControl(cvlayers2,CID_RenameLayer), !inCollab );
    }
}

/* Initialize a window that is to be used for a palette. Specific widgets and other functionality are added elsewhere. */
static GWindow CreatePalette(GWindow w, GRect *pos, int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *wattrs, GWindow v) {
    GWindow gw;
    GPoint pt, base;
    GRect newpos;
    GWindow root;
    GRect ownerpos, screensize;

    pt.x = pos->x; pt.y = pos->y;
    if ( !palettes_fixed ) {
	root = GDrawGetRoot(NULL);
	GDrawGetSize(w,&ownerpos);
	GDrawGetSize(root,&screensize);
	GDrawTranslateCoordinates(w,root,&pt);
	base.x = base.y = 0;
	GDrawTranslateCoordinates(w,root,&base);
	if ( pt.x<0 ) {
	    if ( base.x+ownerpos.width+20+pos->width+20 > screensize.width )
		pt.x=0;
	    else
		pt.x = base.x+ownerpos.width+20;
	}
	if ( pt.y<0 ) pt.y=0;
	if ( pt.x+pos->width>screensize.width )
	    pt.x = screensize.width-pos->width;
	if ( pt.y+pos->height>screensize.height )
	    pt.y = screensize.height-pos->height;
    }
    wattrs->mask |= wam_bordcol|wam_bordwidth;
    wattrs->border_width = 1;
    wattrs->border_color = GDrawGetDefaultForeground(NULL);

    newpos.x = pt.x; newpos.y = pt.y; newpos.width = pos->width; newpos.height = pos->height;
    wattrs->mask|= wam_positioned;
    wattrs->positioned = true;
    if (palettes_docked) {
        pos->x = 0;
        gw = GDrawCreateSubWindow(v, pos, eh, user_data, wattrs);
    } else {
        wattrs->mask |= wam_palette;
        gw = GDrawCreateTopWindow(NULL,&newpos,eh,user_data,wattrs);
    }

    collabclient_addSessionJoiningCallback( onCollabSessionStateChanged );
    collabclient_addSessionLeavingCallback( onCollabSessionStateChanged );
    
return( gw );
}



/* Return screen coordinates of the palette in off, relative to the root window origin. */
static void SaveOffsets(GWindow main, GWindow palette, GPoint *off) {
    if ( !palettes_docked && !palettes_fixed && GDrawIsVisible(palette)) {
	GRect mr, pr;
	GWindow root, temp;
	root = GDrawGetRoot(NULL);
	while ( (temp=GDrawGetParentWindow(main))!=root )
	    main = temp;
	GDrawGetSize(main,&mr);
	GDrawGetSize(palette,&pr);
	off->x = pr.x-mr.x;
	off->y = pr.y-mr.y;
	if ( off->x<0 ) off->x = 0;
	if ( off->y<0 ) off->y = 0;
    }
}

/* Set the palette window position to off, a point in the root window space. */
static void RestoreOffsets(GWindow main, GWindow palette, GPoint *off) {
    GPoint pt;
    GWindow root,temp;
    GRect screensize, pos;

    if ( palettes_fixed )
return;
    pt = *off;
    root = GDrawGetRoot(NULL);
    GDrawGetSize(root,&screensize);
    GDrawGetSize(palette,&pos);
    while ( (temp=GDrawGetParentWindow(main))!=root )
	main = temp;
    GDrawTranslateCoordinates(main,root,&pt);
    if ( pt.x<0 ) pt.x=0;
    if ( pt.y<0 ) pt.y=0;
    if ( pt.x+pos.width>screensize.width )
	pt.x = screensize.width-pos.width;
    if ( pt.y+pos.height>screensize.height )
	pt.y = screensize.height-pos.height;
    GDrawTrueMove(palette,pt.x,pt.y);
    GDrawRaise(palette);
}

static void CVMenuTool(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    cv->b1_tool = mi->mid;
    if ( cvtools!=NULL )
	GDrawRequestExpose(cvtools,NULL,false);
    CVToolsSetCursor(cv,0,NULL);
}

static void CVChangeSpiroMode(CharView *cv);
static void CVMenuSpiroSet(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    CVChangeSpiroMode(cv);
}

void cvtoollist_check(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    int order2 = cv->b.layerheads[cv->b.drawmode]->order2;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	mi->ti.checked = mi->mid==cv->b1_tool;
	switch ( mi->mid ) {
	  case cvt_freehand:
	    mi->ti.disabled = order2;
	  break;
	  case cvt_spiro:
	    mi->ti.disabled = !hasspiro();
	  break;
        }
    }
}

/* Note: If you change this ordering, change enum cvtools */
static char *popupsres[] = {
    N_("Pointer"),               N_("Magnify (Minify with alt)"),
    N_("Draw a freehand curve"), N_("Scroll by hand"),
    N_("Cut splines in two"),    N_("Measure distance, angle between points"),
    N_("Add a point, then drag out its control points"), N_("Change whether spiro is active or not"),
    N_("Add a curve point"),     N_("Add a curve point always either horizontal or vertical"),
    N_("Add a corner point"),    N_("Add a tangent point"),
    N_("Scale the selection"),   N_("Rotate the selection"),
    N_("Flip the selection"),  N_("Skew the selection"),
    N_("Rotate the selection in 3D and project back to plain"), N_("Perform a perspective transformation on the selection"),
    N_("Rectangle or Ellipse"),  N_("Polygon or Star"),
    N_("Rectangle or Ellipse"),  N_("Polygon or Star")};
GMenuItem2 cvtoollist[] = {
    { { (unichar_t *) N_("_Pointer"), (GImage *) "toolspointer.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'C' }, H_("Pointer|No Shortcut"), NULL, NULL, CVMenuTool, cvt_pointer },
    { { (unichar_t *) N_("_Magnify"), (GImage *) "toolsmagnify.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'C' }, H_("Magnify|No Shortcut"), NULL, NULL, CVMenuTool, cvt_magnify },
    { { (unichar_t *) N_("_Freehand"), (GImage *) "toolsfreehand.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'C' }, H_("Freehand|No Shortcut"), NULL, NULL, CVMenuTool, cvt_freehand },
    { { (unichar_t *) N_("_Scroll"), (GImage *) "toolsscroll.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'C' }, H_("Scroll|No Shortcut"), NULL, NULL, CVMenuTool, cvt_hand },
    { { (unichar_t *) N_("_Knife"), (GImage *) "toolsknife.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Knife|No Shortcut"), NULL, NULL, CVMenuTool, cvt_knife },
    { { (unichar_t *) N_("_Ruler"), (GImage *) "toolsruler.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Ruler|No Shortcut"), NULL, NULL, CVMenuTool, cvt_ruler },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("P_en"), (GImage *) "toolspen.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Pen|No Shortcut"), NULL, NULL, CVMenuTool, cvt_pen },
    { { (unichar_t *) N_("_Activate Spiro"), (GImage *) "toolsspiro.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Activate Spiro|No Shortcut"), NULL, NULL, CVMenuSpiroSet, cvt_spiro },
    { { (unichar_t *) N_("_Curve"), (GImage *) "pointscurve.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'C' }, H_("Curve Tool|No Shortcut"), NULL, NULL, CVMenuTool, cvt_curve },
    { { (unichar_t *) N_("_HVCurve"), (GImage *) "pointshvcurve.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'o' }, H_("HVCurve Tool|No Shortcut"), NULL, NULL, CVMenuTool, cvt_hvcurve },
    { { (unichar_t *) N_("C_orner"), (GImage *) "pointscorner.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'o' }, H_("Corner Tool|No Shortcut"), NULL, NULL, CVMenuTool, cvt_corner },
    { { (unichar_t *) N_("_Tangent"), (GImage *) "pointstangent.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Tangent Tool|No Shortcut"), NULL, NULL, CVMenuTool, cvt_tangent },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Sca_le"), (GImage *) "toolsscale.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Scale|No Shortcut"), NULL, NULL, CVMenuTool, cvt_scale },
    { { (unichar_t *) N_("Rotate"), (GImage *) "toolsrotate.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Rotate|No Shortcut"), NULL, NULL, CVMenuTool, cvt_rotate },
    { { (unichar_t *) N_("Flip"), (GImage *) "toolsflip.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Flip|No Shortcut"), NULL, NULL, CVMenuTool, cvt_flip },
    { { (unichar_t *) N_("Ske_w"), (GImage *) "toolsskew.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Skew|No Shortcut"), NULL, NULL, CVMenuTool, cvt_skew },
    { { (unichar_t *) N_("_3D Rotate"), (GImage *) "tools3drotate.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("3D Rotate|No Shortcut"), NULL, NULL, CVMenuTool, cvt_3d_rotate },
    { { (unichar_t *) N_("Perspecti_ve"), (GImage *) "toolsperspective.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Perspective|No Shortcut"), NULL, NULL, CVMenuTool, cvt_perspective },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Rectan_gle"), (GImage *) "toolsrect.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Rectangle|No Shortcut"), NULL, NULL, CVMenuTool, cvt_rect },
    { { (unichar_t *) N_("Pol_ygon"), (GImage *) "toolspolygon.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Polygon|No Shortcut"), NULL, NULL, CVMenuTool, cvt_poly },
    { { (unichar_t *) N_("Ellipse"), (GImage *) "toolselipse.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Ellipse|No Shortcut"), NULL, NULL, CVMenuTool, cvt_elipse },
    { { (unichar_t *) N_("Star"), (GImage *) "toolsstar.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Star|No Shortcut"), NULL, NULL, CVMenuTool, cvt_star },
    GMENUITEM2_EMPTY
};
GMenuItem2 cvspirotoollist[] = {
    { { (unichar_t *) N_("_Pointer"), (GImage *) "toolspointer.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'C' }, H_("Pointer|No Shortcut"), NULL, NULL, CVMenuTool, cvt_pointer },
    { { (unichar_t *) N_("_Magnify"), (GImage *) "toolsmagnify.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'C' }, H_("Magnify|No Shortcut"), NULL, NULL, CVMenuTool, cvt_magnify },
    { { (unichar_t *) N_("_Freehand"), (GImage *) "toolsfreehand.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'C' }, H_("Freehand|No Shortcut"), NULL, NULL, CVMenuTool, cvt_freehand },
    { { (unichar_t *) N_("_Scroll"), (GImage *) "toolsscroll.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'C' }, H_("Scroll|No Shortcut"), NULL, NULL, CVMenuTool, cvt_hand },
    { { (unichar_t *) N_("_Knife"), (GImage *) "toolsknife.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Knife|No Shortcut"), NULL, NULL, CVMenuTool, cvt_knife },
    { { (unichar_t *) N_("_Ruler"), (GImage *) "toolsruler.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Ruler|No Shortcut"), NULL, NULL, CVMenuTool, cvt_ruler },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Knife"), (GImage *) "toolsknife.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Knife|No Shortcut"), NULL, NULL, CVMenuTool, cvt_knife },
    { { (unichar_t *) N_("_Ruler"), (GImage *) "toolsruler.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Ruler|No Shortcut"), NULL, NULL, CVMenuTool, cvt_ruler },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("De_activate Spiro"), (GImage *) "toolsspiro.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Activate Spiro|No Shortcut"), NULL, NULL, CVMenuSpiroSet, cvt_spiro },
    { { (unichar_t *) N_("C_orner"), (GImage *) "pointscorner.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'o' }, H_("Corner Tool|No Shortcut"), NULL, NULL, CVMenuTool, cvt_spirocorner },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("G_4"), (GImage *) "pointscurve.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'C' }, H_("G4 Tool|No Shortcut"), NULL, NULL, CVMenuTool, cvt_spirog4 },
    { { (unichar_t *) N_("G_2"), (GImage *) "pointsG2curve.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'o' }, H_("G2 Tool|No Shortcut"), NULL, NULL, CVMenuTool, cvt_spirog2 },
    { { (unichar_t *) N_("Lef_t"), (GImage *) "pointsspiroprev.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Left Tool|No Shortcut"), NULL, NULL, CVMenuTool, cvt_spiroleft },
    { { (unichar_t *) N_("Rig_ht"), (GImage *) "pointsspironext.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Right Tool|No Shortcut"), NULL, NULL, CVMenuTool, cvt_spiroright },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Sca_le"), (GImage *) "toolsscale.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Scale|No Shortcut"), NULL, NULL, CVMenuTool, cvt_scale },
    { { (unichar_t *) N_("Rotate"), (GImage *) "toolsrotate.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Rotate|No Shortcut"), NULL, NULL, CVMenuTool, cvt_rotate },
    { { (unichar_t *) N_("Flip"), (GImage *) "toolsflip.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Flip|No Shortcut"), NULL, NULL, CVMenuTool, cvt_flip },
    { { (unichar_t *) N_("Ske_w"), (GImage *) "toolsskew.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Skew|No Shortcut"), NULL, NULL, CVMenuTool, cvt_skew },
    { { (unichar_t *) N_("_3D Rotate"), (GImage *) "tools3drotate.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("3D Rotate|No Shortcut"), NULL, NULL, CVMenuTool, cvt_3d_rotate },
    { { (unichar_t *) N_("Perspecti_ve"), (GImage *) "toolsperspective.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Perspective|No Shortcut"), NULL, NULL, CVMenuTool, cvt_perspective },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Rectan_gle"), (GImage *) "toolsrect.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Rectangle|No Shortcut"), NULL, NULL, CVMenuTool, cvt_rect },
    { { (unichar_t *) N_("Pol_ygon"), (GImage *) "toolspolygon.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Polygon|No Shortcut"), NULL, NULL, CVMenuTool, cvt_poly },
    { { (unichar_t *) N_("Ellipse"), (GImage *) "toolselipse.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Ellipse|No Shortcut"), NULL, NULL, CVMenuTool, cvt_elipse },
    { { (unichar_t *) N_("Star"), (GImage *) "toolsstar.png", COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 1, 0, 0, 0, 1, 1, 0, 'T' }, H_("Star|No Shortcut"), NULL, NULL, CVMenuTool, cvt_star },
    GMENUITEM2_EMPTY
};

static char *editablelayers[] = {
/* GT: Foreground, make it short */
    N_("F_ore"),
/* GT: Background, make it short */
    N_("_Back"),
/* GT: Guide layer, make it short */
    N_("_Guide")
};
static real raddiam_x = 20, raddiam_y = 20, rotate_by=0;
static StrokeInfo expand = {
    25, lj_round, lc_butt, si_centerline,
    false, /* removeexternal */
    false, /* removeinternal */
    false, /* leave users */
    3.1415926535897932/4, 25, NULL, 50,
    0.0, 0, 0, NULL, NULL
};

real CVRoundRectRadius(void) {
return( rr_radius );
}

int CVRectElipseCenter(void) {
return( center_out[rectelipse] );
}

int CVPolyStarPoints(void) {
return( ps_pointcnt );
}

real CVStarRatio(void) {
    if ( regular_star )
return( sin(3.1415926535897932/ps_pointcnt)*tan(2*3.1415926535897932/ps_pointcnt)+cos(3.1415926535897932/ps_pointcnt) );
	
return( star_percent );
}

StrokeInfo *CVFreeHandInfo(void) {
return( &expand );
}
    
struct ask_info {
    GWindow gw;
    int done;
    int ret;
    float *val;
    int *co;
    GGadget *rb1;
    GGadget *reg;
    GGadget *pts;
    int ispolystar;
    int haspos;
    char *lab;
    CharView *cv;
};
#define CID_ValText		1001
#define CID_PointPercent	1002
#define CID_CentCornLab		1003
#define CID_CentCornX		1004
#define CID_CentCornY		1005
#define CID_RadDiamLab		1006
#define CID_RadDiamX		1007
#define CID_RadDiamY		1008
#define CID_Angle		1009

static void FakeShapeEvents(CharView *cv) {
    GEvent event;
    real trans[6];

    cv->active_tool = rectelipse ? cvt_elipse : cvt_rect;
    if ( cv->b.sc->inspiro && hasspiro() ) {
	GDrawSetCursor(cv->v,spirotools[cv->active_tool]);
	GDrawSetCursor(cvtools,spirotools[cv->active_tool]);
    } else {
	GDrawSetCursor(cv->v,tools[cv->active_tool]);
	GDrawSetCursor(cvtools,tools[cv->active_tool]);
    }
    cv->showing_tool = cv->active_tool;

    memset(&event,0,sizeof(event));
    event.type = et_mousedown;
    CVMouseDownShape(cv,&event);
    cv->info.x += raddiam_x;
    cv->info.y += raddiam_y;
    CVMouseMoveShape(cv);
    CVMouseUpShape(cv);
    if ( raddiam_x!=0 && raddiam_y!=0 && rotate_by!=0 ) {
	trans[0] = trans[3] = cos ( rotate_by*3.1415926535897932/180. );
	trans[1] = sin( rotate_by*3.1415926535897932/180. );
	trans[2] = -trans[1];
	trans[4] = -cv->p.x*trans[0] - cv->p.y*trans[2] + cv->p.x;
	trans[5] = -cv->p.x*trans[1] - cv->p.y*trans[3] + cv->p.y;
	SplinePointListTransform(cv->b.layerheads[cv->b.drawmode]->splines,trans,
		interpCPsOnMotion?tpt_OnlySelectedInterpCPs:tpt_OnlySelected);
	SCUpdateAll(cv->b.sc);
    }
    cv->active_tool = cvt_none;
}

static int TA_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct ask_info *d = GDrawGetUserData(GGadgetGetWindow(g));
	real val, val2=0;
	int err=0;
	int re = !GGadgetIsChecked(d->rb1);
	if ( d->ispolystar ) {
	    val = GetInt8(d->gw,CID_ValText,d->lab,&err);
	    if ( !(regular_star = GGadgetIsChecked(d->reg)))
		val2 = GetReal8(d->gw,CID_PointPercent,_("Size of Points"),&err);
	} else {
	    val = GetReal8(d->gw,CID_ValText,d->lab,&err);
	    d->co[re] = !GGadgetIsChecked(d->reg);
	}
	if ( err )
return( true );
	if ( d->haspos ) {
	    real x,y, radx,rady, ang;
	    x = GetInt8(d->gw,CID_CentCornX,_("_X"),&err);
	    y = GetInt8(d->gw,CID_CentCornY,_("_Y"),&err);
	    radx = GetInt8(d->gw,CID_RadDiamX,_("Radius:   "),&err);
	    rady = GetInt8(d->gw,CID_RadDiamY,_("Radius:   "),&err);
	    ang = GetInt8(d->gw,CID_Angle,_("Angle:"),&err);
	    if ( err )
return( true );
	    d->cv->p.x = d->cv->info.x = x;
	    d->cv->p.y = d->cv->info.y = y;
	    raddiam_x = radx; raddiam_y = rady;
	    rotate_by = ang;
	    rectelipse = re;
	    *d->val = val;
	    FakeShapeEvents(d->cv);
	}
	*d->val = val;
	d->ret = re;
	d->done = true;
	if ( !regular_star && d->ispolystar )
	    star_percent = val2/100;
	SavePrefs(true);
    }
return( true );
}

static int TA_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct ask_info *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = true;
    }
return( true );
}

static int TA_CenRadChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct ask_info *d = GDrawGetUserData(GGadgetGetWindow(g));
	int is_bb = GGadgetIsChecked(d->reg);
	GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_CentCornLab),
		is_bb ? _("Corner") : _("C_enter"));
	GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_RadDiamLab),
		is_bb ? _("Diameter:") : _("Radius:   "));
    }
return( true );
}

static int TA_RadChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct ask_info *d = GDrawGetUserData(GGadgetGetWindow(g));
	int is_ellipse = !GGadgetIsChecked(d->rb1);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ValText), !is_ellipse );
	GGadgetSetChecked(d->reg,!center_out[is_ellipse]);
	GGadgetSetChecked(d->pts,center_out[is_ellipse]);
	if ( d->haspos )
	    TA_CenRadChange(g,e);
    }
return( true );
}

static int toolask_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct ask_info *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( event->type!=et_char );
}

static int Ask(char *rb1, char *rb2, int rb, char *lab, float *val, int *co,
	int ispolystar, CharView *cv ) {
    struct ask_info d;
    char buffer[20], buf[20];
    char cenx[20], ceny[20], radx[20], rady[20], angle[20];
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[19];
    GTextInfo label[19];
    int off = ((ispolystar&1)?15:0) + ((ispolystar&2)?84:0);
    int haspos = (ispolystar&2)?1:0;

    ispolystar &= 1;

    d.done = false;
    d.ret = rb;
    d.val = val;
    d.co = co;
    d.ispolystar = ispolystar;
    d.haspos = haspos;
    d.lab = lab;
    d.cv = cv;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = _("Shape Type");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,190));
	pos.height = GDrawPointsToPixels(NULL,120+off);
	d.gw = GDrawCreateTopWindow(NULL,&pos,toolask_e_h,&d,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	label[0].text = (unichar_t *) rb1;
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5; 
	gcd[0].gd.flags = gg_enabled|gg_visible | (rb==0?gg_cb_on:0);
	gcd[0].creator = GRadioCreate;

	label[1].text = (unichar_t *) rb2;
	label[1].text_is_1byte = true;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.pos.x = ispolystar?65:75; gcd[1].gd.pos.y = 5; 
	gcd[1].gd.flags = gg_enabled|gg_visible | (rb==1?gg_cb_on:0);
	gcd[1].creator = GRadioCreate;

	label[2].text = (unichar_t *) lab;
	label[2].text_is_1byte = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = 25; 
	gcd[2].gd.flags = gg_enabled|gg_visible ;
	gcd[2].creator = GLabelCreate;

	sprintf( buffer, "%g", *val );
	label[3].text = (unichar_t *) buffer;
	label[3].text_is_1byte = true;
	gcd[3].gd.label = &label[3];
	gcd[3].gd.pos.x = 5; gcd[3].gd.pos.y = 40; 
	gcd[3].gd.flags = gg_enabled|gg_visible ;
	gcd[3].gd.cid = CID_ValText;
	gcd[3].creator = GTextFieldCreate;

	gcd[4].gd.pos.x = 20-3; gcd[4].gd.pos.y = 85+off;
	gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
	gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[4].text = (unichar_t *) _("_OK");
	label[4].text_is_1byte = true;
	label[4].text_in_resource = true;
	gcd[4].gd.mnemonic = 'O';
	gcd[4].gd.label = &label[4];
	gcd[4].gd.handle_controlevent = TA_OK;
	gcd[4].creator = GButtonCreate;

	gcd[5].gd.pos.x = -20; gcd[5].gd.pos.y = 85+3+off;
	gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
	gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[5].text = (unichar_t *) _("_Cancel");
	label[5].text_is_1byte = true;
	label[5].text_in_resource = true;
	gcd[5].gd.label = &label[5];
	gcd[5].gd.mnemonic = 'C';
	gcd[5].gd.handle_controlevent = TA_Cancel;
	gcd[5].creator = GButtonCreate;

	if ( ispolystar ) {
	    label[6].text = (unichar_t *) _("Regular");
	    label[6].text_is_1byte = true;
	    gcd[6].gd.label = &label[6];
	    gcd[6].gd.pos.x = 5; gcd[6].gd.pos.y = 70; 
	    gcd[6].gd.flags = gg_enabled|gg_visible | (rb==0?gg_cb_on:0);
	    gcd[6].creator = GRadioCreate;

	    label[7].text = (unichar_t *) _("Points:");
	    label[7].text_is_1byte = true;
	    gcd[7].gd.label = &label[7];
	    gcd[7].gd.pos.x = 65; gcd[7].gd.pos.y = 70; 
	    gcd[7].gd.flags = gg_enabled|gg_visible | (rb==1?gg_cb_on:0);
	    gcd[7].creator = GRadioCreate;

	    sprintf( buf, "%4g", star_percent*100 );
	    label[8].text = (unichar_t *) buf;
	    label[8].text_is_1byte = true;
	    gcd[8].gd.label = &label[8];
	    gcd[8].gd.pos.x = 125; gcd[8].gd.pos.y = 70;  gcd[8].gd.pos.width=50;
	    gcd[8].gd.flags = gg_enabled|gg_visible ;
	    gcd[8].gd.cid = CID_PointPercent;
	    gcd[8].creator = GTextFieldCreate;

	    label[9].text = (unichar_t *) "%";
	    label[9].text_is_1byte = true;
	    gcd[9].gd.label = &label[9];
	    gcd[9].gd.pos.x = 180; gcd[9].gd.pos.y = 70; 
	    gcd[9].gd.flags = gg_enabled|gg_visible ;
	    gcd[9].creator = GLabelCreate;
	} else {
	    label[6].text = (unichar_t *) _("Bounding Box");
	    label[6].text_is_1byte = true;
	    gcd[6].gd.label = &label[6];
	    gcd[6].gd.pos.x = 5; gcd[6].gd.pos.y = 65; 
	    gcd[6].gd.flags = gg_enabled|gg_visible | (co[rb]==0?gg_cb_on:0);
	    gcd[6].creator = GRadioCreate;

	    label[7].text = (unichar_t *) _("Center Out");
	    label[7].text_is_1byte = true;
	    gcd[7].gd.label = &label[7];
	    gcd[7].gd.pos.x = 90; gcd[7].gd.pos.y = 65; 
	    gcd[7].gd.flags = gg_enabled|gg_visible | (co[rb]==1?gg_cb_on:0);
	    gcd[7].creator = GRadioCreate;

	    if ( rb )
		gcd[3].gd.flags = gg_visible;
	    gcd[0].gd.handle_controlevent = TA_RadChange;
	    gcd[1].gd.handle_controlevent = TA_RadChange;

	    if ( haspos ) {
		gcd[6].gd.handle_controlevent = TA_CenRadChange;
		gcd[7].gd.handle_controlevent = TA_CenRadChange;

		label[8].text = (unichar_t *) _("_X");
		label[8].text_is_1byte = true;
		label[8].text_in_resource = true;
		gcd[8].gd.label = &label[8];
		gcd[8].gd.pos.x = 70; gcd[8].gd.pos.y = gcd[7].gd.pos.y+15;
		gcd[8].gd.flags = gg_enabled|gg_visible;
		gcd[8].creator = GLabelCreate;

		label[9].text = (unichar_t *) _("_Y");
		label[9].text_is_1byte = true;
		label[9].text_in_resource = true;
		gcd[9].gd.label = &label[9];
		gcd[9].gd.pos.x = 120; gcd[9].gd.pos.y = gcd[8].gd.pos.y;
		gcd[9].gd.flags = gg_enabled|gg_visible;
		gcd[9].creator = GLabelCreate;

		label[10].text = (unichar_t *) (co[rb] ? _("C_enter") : _("C_orner") );
		label[10].text_is_1byte = true;
		label[10].text_in_resource = true;
		gcd[10].gd.label = &label[10];
		gcd[10].gd.pos.x = 5; gcd[10].gd.pos.y = gcd[8].gd.pos.y+17;
		gcd[10].gd.flags = gg_enabled|gg_visible;
		gcd[10].gd.cid = CID_CentCornLab;
		gcd[10].creator = GLabelCreate;

		sprintf( cenx, "%g", (double) cv->info.x );
		label[11].text = (unichar_t *) cenx;
		label[11].text_is_1byte = true;
		gcd[11].gd.label = &label[11];
		gcd[11].gd.pos.x = 60; gcd[11].gd.pos.y = gcd[10].gd.pos.y-4;
		gcd[11].gd.pos.width = 40;
		gcd[11].gd.flags = gg_enabled|gg_visible;
		gcd[11].gd.cid = CID_CentCornX;
		gcd[11].creator = GTextFieldCreate;

		sprintf( ceny, "%g", (double) cv->info.y );
		label[12].text = (unichar_t *) ceny;
		label[12].text_is_1byte = true;
		gcd[12].gd.label = &label[12];
		gcd[12].gd.pos.x = 110; gcd[12].gd.pos.y = gcd[11].gd.pos.y;
		gcd[12].gd.pos.width = gcd[11].gd.pos.width;
		gcd[12].gd.flags = gg_enabled|gg_visible;
		gcd[12].gd.cid = CID_CentCornY;
		gcd[12].creator = GTextFieldCreate;

		label[13].text = (unichar_t *) (co[rb] ? _("Radius:   ") : _("Diameter:") );
		label[13].text_is_1byte = true;
		gcd[13].gd.label = &label[13];
		gcd[13].gd.pos.x = 5; gcd[13].gd.pos.y = gcd[10].gd.pos.y+24;
		gcd[13].gd.flags = gg_enabled|gg_visible;
		gcd[13].gd.cid = CID_RadDiamLab;
		gcd[13].creator = GLabelCreate;

		sprintf( radx, "%g", (double) raddiam_x );
		label[14].text = (unichar_t *) radx;
		label[14].text_is_1byte = true;
		gcd[14].gd.label = &label[14];
		gcd[14].gd.pos.x = gcd[11].gd.pos.x; gcd[14].gd.pos.y = gcd[13].gd.pos.y-4;
		gcd[14].gd.pos.width = gcd[11].gd.pos.width;
		gcd[14].gd.flags = gg_enabled|gg_visible;
		gcd[14].gd.cid = CID_RadDiamX;
		gcd[14].creator = GTextFieldCreate;

		sprintf( rady, "%g", (double) raddiam_y );
		label[15].text = (unichar_t *) rady;
		label[15].text_is_1byte = true;
		gcd[15].gd.label = &label[15];
		gcd[15].gd.pos.x = gcd[12].gd.pos.x; gcd[15].gd.pos.y = gcd[14].gd.pos.y;
		gcd[15].gd.pos.width = gcd[11].gd.pos.width;
		gcd[15].gd.flags = gg_enabled|gg_visible;
		gcd[15].gd.cid = CID_RadDiamY;
		gcd[15].creator = GTextFieldCreate;

		label[16].text = (unichar_t *) _("Angle:");
		label[16].text_is_1byte = true;
		gcd[16].gd.label = &label[16];
		gcd[16].gd.pos.x = 5; gcd[16].gd.pos.y = gcd[13].gd.pos.y+24;
		gcd[16].gd.flags = gg_enabled|gg_visible;
		gcd[16].creator = GLabelCreate;

		sprintf( angle, "%g", (double) rotate_by );
		label[17].text = (unichar_t *) angle;
		label[17].text_is_1byte = true;
		gcd[17].gd.label = &label[17];
		gcd[17].gd.pos.x = 60; gcd[17].gd.pos.y = gcd[16].gd.pos.y-4;
		gcd[17].gd.pos.width = gcd[11].gd.pos.width;
		gcd[17].gd.flags = gg_enabled|gg_visible;
		gcd[17].gd.cid = CID_Angle;
		gcd[17].creator = GTextFieldCreate;
	    }
	}
	GGadgetsCreate(d.gw,gcd);
    d.rb1 = gcd[0].ret;
    d.reg = gcd[6].ret;
    d.pts = gcd[7].ret;

    GWidgetHidePalettes();
    GDrawSetVisible(d.gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(d.gw);
return( d.ret );
}

static void CVRectElipse(CharView *cv) {
    rectelipse = Ask(_("Rectangle"),_("Ellipse"),rectelipse,
	    _("Round Rectangle Radius"),&rr_radius,center_out,false, cv);
    GDrawRequestExpose(cvtools,NULL,false);
}

void CVRectEllipsePosDlg(CharView *cv) {
    rectelipse = Ask(_("Rectangle"),_("Ellipse"),rectelipse,
	    _("Round Rectangle Radius"),&rr_radius,center_out,2, cv);
    GDrawRequestExpose(cvtools,NULL,false);
}

static void CVPolyStar(CharView *cv) {
    float temp = ps_pointcnt;
    int foo[2];
    polystar = Ask(_("Polygon"),_("Star"),polystar,
	    _("Number of star points/Polygon vertices"),&temp,foo,true, cv);
    ps_pointcnt = temp;
}

/* Note: If you change this ordering, change enum cvtools */
// index0 is non selected / selected image
// index1 is the row    in the toolbar
// index2 is the column in the toolbar
//
// This is two null terminated collections. The second collection
// contains the alternate icons for circle and star at the bottom of
// the toolbar.
static GImage *normbuttons[2][14][2] = {
    {
	{ &GIcon_pointer,  &GIcon_magnify },
	{ &GIcon_freehand, &GIcon_hand },
	{ &GIcon_knife,    &GIcon_ruler },
	{ &GIcon_pen,      &GIcon_spirodisabled },
	{ &GIcon_curve,    &GIcon_hvcurve },
	{ &GIcon_corner,   &GIcon_tangent},
	{ &GIcon_scale,    &GIcon_rotate },
	{ &GIcon_flip,     &GIcon_skew },
	{ &GIcon_3drotate, &GIcon_perspective },
	{ &GIcon_rect,     &GIcon_poly},
	{ 0, 0 },
	{ &GIcon_elipse,   &GIcon_star},
	{ 0, 0 },
	{ 0, 0 }
    }
    , {
	{ &GIcon_pointer_selected,  &GIcon_magnify_selected },
	{ &GIcon_freehand_selected, &GIcon_hand_selected },
	{ &GIcon_knife_selected,    &GIcon_ruler_selected },
	{ &GIcon_pen_selected,      &GIcon_spiroup_selected },
	{ &GIcon_curve_selected,    &GIcon_hvcurve_selected },
	{ &GIcon_corner_selected,   &GIcon_tangent_selected},
	{ &GIcon_scale_selected,    &GIcon_rotate_selected },
	{ &GIcon_flip_selected,     &GIcon_skew_selected },
	{ &GIcon_3drotate_selected, &GIcon_perspective_selected },
	{ &GIcon_rect_selected,     &GIcon_poly_selected},
	{ 0, 0 },
	{ &GIcon_elipse_selected,   &GIcon_star_selected},
	{ 0, 0 },
	{ 0, 0 }
    }};
static GImage *spirobuttons[2][14][2] = {
    {
	{ &GIcon_pointer,      &GIcon_magnify },
	{ &GIcon_freehand,     &GIcon_hand },
	{ &GIcon_knife,        &GIcon_ruler },
	{ &GIcon_spiroright,   &GIcon_spirodown },
	{ &GIcon_spirocurve,   &GIcon_spirog2curve },
	{ &GIcon_spirocorner,  &GIcon_spiroleft },
	{ &GIcon_scale,        &GIcon_rotate },
	{ &GIcon_flip,         &GIcon_skew },
	{ &GIcon_3drotate,     &GIcon_perspective },
	{ &GIcon_rect,         &GIcon_poly},
	{ 0, 0 },
	{ &GIcon_elipse,       &GIcon_star},
	{ 0, 0 },
	{ 0, 0 }
    }
    , {
	{ &GIcon_pointer_selected,     &GIcon_magnify_selected },
	{ &GIcon_freehand_selected,    &GIcon_hand_selected },
	{ &GIcon_knife_selected,       &GIcon_ruler_selected },
	{ &GIcon_spiroright_selected,  &GIcon_spirodown_selected },
	{ &GIcon_spirocurve_selected,  &GIcon_spirog2curve_selected },
	{ &GIcon_spirocorner_selected, &GIcon_spiroleft_selected },
	{ &GIcon_scale_selected,       &GIcon_rotate_selected },
	{ &GIcon_flip_selected,        &GIcon_skew_selected },
	{ &GIcon_3drotate_selected,    &GIcon_perspective_selected },
	{ &GIcon_rect_selected,        &GIcon_poly_selected},
	{ 0, 0 },
	{ &GIcon_elipse_selected,      &GIcon_star_selected},
	{ 0, 0 },
	{ 0, 0 }
    }};
static GImage *normsmalls[] = { &GIcon_smallpointer,  &GIcon_smallmag,
				&GIcon_smallpencil,   &GIcon_smallhand,
				&GIcon_smallknife,    &GIcon_smallruler,
				&GIcon_smallpen,      NULL,
				&GIcon_smallcurve,    &GIcon_smallhvcurve,
				&GIcon_smallcorner,   &GIcon_smalltangent,
				&GIcon_smallscale,    &GIcon_smallrotate,
				&GIcon_smallflip,     &GIcon_smallskew,
				&GIcon_small3drotate, &GIcon_smallperspective,
				&GIcon_smallrect,     &GIcon_smallpoly,
				&GIcon_smallelipse,   &GIcon_smallstar };
static GImage *spirosmalls[] = { &GIcon_smallpointer, &GIcon_smallmag,
				 &GIcon_smallpencil,  &GIcon_smallhand,
				 &GIcon_smallknife,   &GIcon_smallruler,
				 &GIcon_smallspiroright,  NULL,
				 &GIcon_smallspirocurve,  &GIcon_smallspirog2curve,
				 &GIcon_smallspirocorner, &GIcon_smallspiroleft,
				 &GIcon_smallscale,       &GIcon_smallrotate,
				 &GIcon_smallflip,        &GIcon_smallskew,
				 &GIcon_small3drotate,    &GIcon_smallperspective,
				 &GIcon_smallrect,        &GIcon_smallpoly,
				 &GIcon_smallelipse,      &GIcon_smallstar };

static int getSmallIconsHeight()
{
    return GIcon_smallpointer.u.image->height;
}

static int getToolbarWidth( CharView *cv ) 
{
    int cache = 0;
    if( !cache ) {
	GImage* (*buttons)[14][2] = (CVInSpiro(cv) ? spirobuttons : normbuttons);
	int i = 0;
	
	for ( i=0; buttons[0][i][0]; ++i ) {
	    cache = MAX( cache, buttons[0][i][0]->u.image->width + buttons[0][i][1]->u.image->width );
	}
    }
    return cache;
}

static int getToolbarHeight( CharView *cv ) 
{
    int cache = 0;
    if( !cache ) {
	GImage* (*buttons)[14][2] = (CVInSpiro(cv) ? spirobuttons : normbuttons);
	int i = 0;
	
	for ( i=0; buttons[0][i][0]; ++i ) {
	    cache += MAX( buttons[0][i][0]->u.image->height, buttons[0][i][1]->u.image->height );
	}
    }
    cache += getSmallIconsHeight() * 4;
    cache += 6;
    return cache;
}


typedef struct _IJ 
{
    int i;
    int j;
} IJ;

typedef void (*visitButtonsVisitor) ( CharView *cv,             // CharView passed to visitButtons()
				      GImage* gimage, int mi,   // button we are visiting and adjusted 'i'
				      int i, int j,             // i and j position of button (j is across 0,1) (i is down 0...11)
				      int iconx, int icony,     // pixel where this button starts
				      int selected,             // is this button the active tool
				      void* udata );            // user data


/**
 * Visit every button in the toolbar calling the function 'v' with a
 * collection of interesting state data.
 */
static void visitButtons( CharView* cv, visitButtonsVisitor v, void* udata ) 
{
    GImage* (*buttons)[14][2] = (CVInSpiro(cv) ? spirobuttons : normbuttons);
    int i,j,sel,norm, mi;
    int tool = cv->cntrldown?cv->cb1_tool:cv->b1_tool;
    int icony = 1;
    for ( i=0; buttons[0][i][0]; ++i ) {
	int iconx = 1;
	for ( j=0; j<2 && buttons[0][i][j]; ++j ) {

	    mi = i;
	    sel = (tool == mi*2+j );
	    if( buttons[0][mi][j] == &GIcon_rect && rectelipse
		|| buttons[0][mi][j] == &GIcon_poly && polystar )
	    {
		sel = (tool == (mi+1)*2+j );
		mi+=2;
	    }
	    
	    v( cv, buttons[sel][mi][j], mi, i, j, iconx, icony, sel, udata );
	    iconx += buttons[sel][mi][j]->u.image->width;
	}
	icony += MAX( buttons[0][i][0]->u.image->width, buttons[0][i][1]->u.image->width );
    }
}

typedef struct _getIJFromMouseVisitorData 
{
    int mx, my;
    IJ ret;
} getIJFromMouseVisitorData;

static void getIJFromMouseVisitor( CharView *cv, GImage* gimage,
				   int mi, int i, int j,
				   int iconx, int icony, int selected, void* udata )
{
    getIJFromMouseVisitorData* d = (getIJFromMouseVisitorData*)udata;
    if( IS_IN_ORDER3( iconx, d->mx, iconx + gimage->u.image->width )
	&& IS_IN_ORDER3( icony, d->my, icony + gimage->u.image->height ))
	{
	    d->ret.i = i;
	    d->ret.j = j;
	}
}

/**
 * Get the i,j coordinates of the toolbar button that is under the
 * mouse at mx,my or return -1,-1 if nothing is under the mouse.
 */ 
static IJ getIJFromMouse( CharView* cv, int mx, int my ) 
{
    getIJFromMouseVisitorData d;
    d.mx = mx;
    d.my = my;
    d.ret.i = -1;
    d.ret.j = -1;
    visitButtons( cv, getIJFromMouseVisitor, &d );
    return d.ret;
}

/*
 * This draws the three-dimensional relief on a button. It gets the dimensions from the button image.
 */

void cvp_draw_relief(GWindow pixmap, GImage *iconimg, int iconx, int icony, int selected) {
	extern Color cvbutton3dedgelightcol; // Default 0xe0e0e0.		
	extern Color cvbutton3dedgedarkcol; // Default 0x707070.
	int iconw = iconimg->u.image->width;
	int iconh = iconimg->u.image->height;
	int norm = !selected;
	// Note: The original code placed the right and bottom fake relief
	// outside of the button image area (offset by 25 instead of 24),
	// but we are staying just inside the image area now.
	GDrawDrawLine(pixmap,iconx,icony,iconx+iconw,icony,norm?cvbutton3dedgelightcol:cvbutton3dedgedarkcol);		
	GDrawDrawLine(pixmap,iconx,icony,iconx,icony+iconh,norm?cvbutton3dedgelightcol:cvbutton3dedgedarkcol);		
	GDrawDrawLine(pixmap,iconx,icony+iconh,iconx+iconw,icony+iconh,norm?cvbutton3dedgedarkcol:cvbutton3dedgelightcol);		
	GDrawDrawLine(pixmap,iconx+iconw,icony,iconx+iconw,icony+iconh,norm?cvbutton3dedgedarkcol:cvbutton3dedgelightcol);
}


/**
 * This visitor draws the actual image for each toolbar button
 * The drawing is done to d->pixmap.
 * icony is the max 
 */
typedef struct _ToolsExposeVisitorData 
{
    GWindow pixmap;
    int     maxicony;       //< largest icony that the visitor saw
    int     lastIconHeight; //< height of the last icon visited
} ToolsExposeVisitorData;
static void ToolsExposeVisitor( CharView *cv, GImage* gimage,
				int mi, int i, int j,
				int iconx, int icony, int selected, void* udata )
{
		extern int cvbutton3d; // This comes from elsewhere.
    ToolsExposeVisitorData* d = (ToolsExposeVisitorData*)udata;
    GImage* (*buttons)[14][2] = (CVInSpiro(cv) ? spirobuttons : normbuttons);

		// We draw the button image onto the toolbar image.
		GDrawDrawImage(d->pixmap,buttons[selected][mi][j],NULL,iconx,icony);

		// We may need to draw relief, too.
		if (cvbutton3d > 0) cvp_draw_relief(d->pixmap, buttons[0][mi][j], iconx, icony, selected);

    d->maxicony = MAX( d->maxicony, icony );
    d->lastIconHeight = buttons[selected][mi][j]->u.image->height;
}


static void ToolsExpose(GWindow pixmap, CharView *cv, GRect *r) {
    GRect old;
    static const unichar_t _Mouse[][9] = {
	    { 'M', 's', 'e', '1',  '\0' },
	    { '^', 'M', 's', 'e', '1',  '\0' },
	    { 'M', 's', 'e', '2',  '\0' },
	    { '^', 'M', 's', 'e', '2',  '\0' }};
    int i,j,sel,norm, mi;
    int tool = cv->cntrldown?cv->cb1_tool:cv->b1_tool;
    int dither = GDrawSetDither(NULL,false);
    GRect temp;
    int canspiro = hasspiro(), inspiro = canspiro && cv->b.sc->inspiro;
    GImage* (*buttons)[14][2] = (inspiro ? spirobuttons : normbuttons);
    GImage **smalls = inspiro ? spirosmalls : normsmalls;

    normbuttons[0][3][1] = canspiro ? &GIcon_spiroup : &GIcon_spirodisabled;

    GDrawPushClip(pixmap,r,&old);
    GDrawFillRect(pixmap,r,GDrawGetDefaultBackground(NULL));
    GDrawSetLineWidth(pixmap,0);

    ToolsExposeVisitorData d;
    d.pixmap = pixmap;
    d.maxicony = 0;
    visitButtons( cv, ToolsExposeVisitor, &d );
    int bottomOfMainIconsY = d.maxicony + d.lastIconHeight;
    
    
    GDrawSetFont(pixmap,toolsfont);
    temp.x = 52-16;
    temp.y = bottomOfMainIconsY;
    temp.width = 16;
    temp.height = 4*12;
    GDrawFillRect(pixmap,&temp,GDrawGetDefaultBackground(NULL));
    for ( j=0; j<4; ++j ) {
	GDrawDrawText(pixmap,2,bottomOfMainIconsY+j*getSmallIconsHeight()+10,
		      (unichar_t *) _Mouse[j],-1,GDrawGetDefaultForeground(NULL));
	if ( (&cv->b1_tool)[j]!=cvt_none && smalls[(&cv->b1_tool)[j]])
	    GDrawDrawImage(pixmap,smalls[(&cv->b1_tool)[j]],NULL,52-16,bottomOfMainIconsY+j*getSmallIconsHeight());
    }
    GDrawPopClip(pixmap,&old);
    GDrawSetDither(NULL,dither);
}

int TrueCharState(GEvent *event) {
    int bit = 0;
    /* X doesn't set the state until after the event. I want the state to */
    /*  reflect whatever key just got depressed/released */
    int keysym = event->u.chr.keysym;

    if ( keysym == GK_Caps_Lock || keysym == GK_Shift_Lock ) {
	static int set_on_last_down = false;
	/* caps lock is sticky and doesn't work like the other modifiers */
	/* but it is even worse. the bit seems to be set on key down, but */
	/* unset on key up. In other words on key up, the bit will always */
	/* set and we have no idea which way it will go. So we guess, and */
	/* if they haven't messed with the key outside ff we should be right */
	if ( event->type == et_char ) {
	    set_on_last_down = (event->u.chr.state ^ ksm_capslock)& ksm_capslock;
return( event->u.chr.state ^ ksm_capslock );
	} else if ( !(event->u.chr.state & ksm_capslock) || set_on_last_down )
return( event->u.chr.state );
	else
return( event->u.chr.state & ~ksm_capslock );
    }

    if ( keysym == GK_Meta_L || keysym == GK_Meta_R ||
	    keysym == GK_Alt_L || keysym == GK_Alt_R )
	bit = ksm_meta;
    else if ( keysym == GK_Shift_L || keysym == GK_Shift_R )
	bit = ksm_shift;
    else if ( keysym == GK_Control_L || keysym == GK_Control_R )
	bit = ksm_control;
    else if ( keysym == GK_Super_L || keysym == GK_Super_R )
	bit = ksm_super;
    else if ( keysym == GK_Hyper_L || keysym == GK_Hyper_R )
	bit = ksm_hyper;
    else
return( event->u.chr.state );

    if ( event->type == et_char )
return( event->u.chr.state | bit );
    else
return( event->u.chr.state & ~bit );
}

void CVToolsSetCursor(CharView *cv, int state, char *device) {
    int shouldshow;
    int cntrl;

    if ( tools[0] == ct_pointer ) {
	tools[cvt_pointer] = ct_mypointer;
	tools[cvt_magnify] = ct_magplus;
	tools[cvt_freehand] = ct_pencil;
	tools[cvt_hand] = ct_myhand;
	tools[cvt_curve] = ct_circle;
	tools[cvt_hvcurve] = ct_hvcircle;
	tools[cvt_corner] = ct_square;
	tools[cvt_tangent] = ct_triangle;
	tools[cvt_pen] = ct_pen;
	tools[cvt_knife] = ct_knife;
	tools[cvt_ruler] = ct_ruler;
	tools[cvt_scale] = ct_scale;
	tools[cvt_flip] = ct_flip;
	tools[cvt_rotate] = ct_rotate;
	tools[cvt_skew] = ct_skew;
	tools[cvt_3d_rotate] = ct_3drotate;
	tools[cvt_perspective] = ct_perspective;
	tools[cvt_rect] = ct_rect;
	tools[cvt_poly] = ct_poly;
	tools[cvt_elipse] = ct_elipse;
	tools[cvt_star] = ct_star;
	tools[cvt_minify] = ct_magminus;
	memcpy(spirotools,tools,sizeof(tools));
	spirotools[cvt_spirog2] = ct_g2circle;
	spirotools[cvt_spiroleft] = ct_spiroleft;
	spirotools[cvt_spiroright] = ct_spiroright;
    }

    shouldshow = cvt_none;
    if ( cv->active_tool!=cvt_none )
	shouldshow = cv->active_tool;
    else if ( cv->pressed_display!=cvt_none )
	shouldshow = cv->pressed_display;
    else if ( device==NULL || strcmp(device,"Mouse1")==0 ) {
	if ( (state&(ksm_shift|ksm_control)) && (state&ksm_button4))
	    shouldshow = cvt_magnify;
	else if ( (state&(ksm_shift|ksm_control)) && (state&ksm_button5))
	    shouldshow = cvt_minify;
	else if ( (state&ksm_control) && (state&(ksm_button2|ksm_super)) )
	    shouldshow = cv->cb2_tool;
	else if ( (state&(ksm_button2|ksm_super)) )
	    shouldshow = cv->b2_tool;
	else if ( (state&ksm_control) )
	    shouldshow = cv->cb1_tool;
	else
	    shouldshow = cv->b1_tool;
    } else if ( strcmp(device,"eraser")==0 )
	shouldshow = cv->er_tool;
    else if ( strcmp(device,"stylus")==0 ) {
	if ( (state&(ksm_button2|ksm_control|ksm_super)) )
	    shouldshow = cv->s2_tool;
	else
	    shouldshow = cv->s1_tool;
    }
    if ( shouldshow==cvt_magnify && (state&ksm_meta))
	shouldshow = cvt_minify;
    if ( shouldshow!=cv->showing_tool ) {
	CPEndInfo(cv);
	if ( cv->b.sc->inspiro && hasspiro()) {
	    GDrawSetCursor(cv->v,spirotools[shouldshow]);
	    if ( cvtools!=NULL )	/* Might happen if window owning docked palette destroyed */
		GDrawSetCursor(cvtools,spirotools[shouldshow]);
	} else {
	    GDrawSetCursor(cv->v,tools[shouldshow]);
	    if ( cvtools!=NULL )	/* Might happen if window owning docked palette destroyed */
		GDrawSetCursor(cvtools,tools[shouldshow]);
	}
	cv->showing_tool = shouldshow;
    }

    if ( device==NULL || strcmp(device,"stylus")==0 ) {
	cntrl = (state&ksm_control)?1:0;
	if ( device!=NULL && (state&ksm_button2))
	    cntrl = true;
	if ( cntrl != cv->cntrldown ) {
	    cv->cntrldown = cntrl;
	    GDrawRequestExpose(cvtools,NULL,false);
	}
    }
}

static int CVCurrentTool(CharView *cv, GEvent *event) {
    if ( event->u.mouse.device!=NULL && strcmp(event->u.mouse.device,"eraser")==0 )
return( cv->er_tool );
    else if ( event->u.mouse.device!=NULL && strcmp(event->u.mouse.device,"stylus")==0 ) {
	if ( event->u.mouse.button==2 )
	    /* Only thing that matters is touch which maps to button 1 */;
	else if ( cv->had_control )
return( cv->s2_tool );
	else
return( cv->s1_tool );
    }
    if ( cv->had_control && event->u.mouse.button==2 )
return( cv->cb2_tool );
    else if ( event->u.mouse.button==2 )
return( cv->b2_tool );
    else if ( cv->had_control ) {
return( cv->cb1_tool );
    } else {
return( cv->b1_tool );
    }
}

static void SCCheckForSSToOptimize(SplineChar *sc, SplineSet *ss,int order2) {

    for ( ; ss!=NULL ; ss = ss->next ) {
	if ( ss->beziers_need_optimizer ) {
	    SplineSetAddExtrema(sc,ss,ae_only_good,sc->parent->ascent+sc->parent->descent);
	    ss->beziers_need_optimizer = false;
	}
	if ( order2 && ss->first->next!=NULL && !ss->first->next->order2 ) {
	    SplineSet *temp = SSttfApprox(ss), foo;
	    foo = *ss;
	    ss->first = temp->first; ss->last = temp->last;
	    temp->first = foo.first; temp->last = foo.last;
	    SplinePointListFree(temp);
	}
    }
}

static void CVChangeSpiroMode(CharView *cv) {
    if ( hasspiro() ) {
	cv->b.sc->inspiro = !cv->b.sc->inspiro;
	cv->showing_tool = cvt_none;
	CVClearSel(cv);
	if ( !cv->b.sc->inspiro )
	    SCCheckForSSToOptimize(cv->b.sc,cv->b.layerheads[cv->b.drawmode]->splines,
		    cv->b.layerheads[cv->b.drawmode]->order2);
	GDrawRequestExpose(cvtools,NULL,false);
	SCUpdateAll(cv->b.sc);
    } else
#ifdef _NO_LIBSPIRO
	ff_post_error(_("You may not use spiros"),_("This version of fontforge was not linked with the spiro library, so you may not use them."));
#else
	ff_post_error(_("You may not use spiros"),_("FontForge was unable to load libspiro, spiros are not available for use."));
#endif
}

char* HKTextInfoToUntranslatedTextFromTextInfo( GTextInfo* ti ); // From ../gdraw/gmenu.c.


static void ToolsMouse(CharView *cv, GEvent *event) {
    IJ ij = getIJFromMouse( cv, event->u.mouse.x, event->u.mouse.y );
    int i = ij.i;
    int j = ij.j;
    int mi = i;
    int pos;
    int isstylus = event->u.mouse.device!=NULL && strcmp(event->u.mouse.device,"stylus")==0;
    int styluscntl = isstylus && (event->u.mouse.state&0x200);
    static int settings[2];

    if( j==-1 || i==-1 )
	return;
    
    if(j >= 2)
	return;			/* If the wm gave me a window the wrong size */


    if ( i==(cvt_rect)/2 ) {
	int current = CVCurrentTool(cv,event);
	int changed = false;
	if ( event->type == et_mousedown && event->u.mouse.clicks>=2 ) {
	    rectelipse = settings[0];
	    polystar = settings[1];
	} else if ( event->type == et_mousedown ) {
	    settings[0] = rectelipse; settings[1] = polystar;
	    /* A double click will change the type twice, which leaves it where it was, which is desired */
	    if ( j==0 && ((!rectelipse && current==cvt_rect) || (rectelipse && current==cvt_elipse)) ) {
		rectelipse = !rectelipse;
		changed = true;
	    } else if (j==1 && ((!polystar && current==cvt_poly) || (polystar && current==cvt_star)) ) {
		polystar = !polystar;
		changed = true;
	    }
	    if ( changed ) {
		SavePrefs(true);
		GDrawRequestExpose(cvtools,NULL,false);
	    }
	}
	if ( (j==0 && rectelipse) || (j==1 && polystar) )
	    ++mi;
    }
    pos = mi*2 + j;
    GGadgetEndPopup();
    /* we have two fewer buttons than commands as two bottons each control two commands */
    if ( pos<0 || pos>=cvt_max )
	pos = cvt_none;
    if ( event->type == et_mousedown ) {
	if ( isstylus && event->u.mouse.button==2 )
	    /* Not a real button press, only touch counts. This is a modifier */;
	else if ( pos==cvt_spiro ) {
	    CVChangeSpiroMode(cv);
	    /* This is just a button that indicates a state */
	} else {
	    cv->pressed_tool = cv->pressed_display = pos;
	    cv->had_control = ((event->u.mouse.state&ksm_control) || styluscntl)?1:0;
	    event->u.mouse.state |= (1<<(7+event->u.mouse.button));
	}
	if ( event->u.mouse.clicks>=2 &&
		(pos/2 == cvt_scale/2 || pos/2 == cvt_rotate/2 || pos == cvt_3d_rotate ))
	    CVDoTransform(cv,pos);
    } else if ( event->type == et_mousemove ) {
	if ( cv->pressed_tool==cvt_none && pos!=cvt_none ) {
	    /* Not pressed */
	    char *msg = _(popupsres[pos]);
	    if ( cv->b.sc->inspiro && hasspiro()) {
		if ( pos==cvt_spirog2 )
		    msg = _("Add a g2 curve point");
		else if ( pos==cvt_spiroleft )
		    msg = _("Add a prev constraint point (sometimes like a tangent)");
		else if ( pos==cvt_spiroright )
		    msg = _("Add a next constraint point (sometimes like a tangent)");
	    }
	    // We want to display the hotkey for the key in question if possible.
	    char * mininame = HKTextInfoToUntranslatedTextFromTextInfo(&cvtoollist[pos].ti);
	    char * menuname = NULL;
	    Hotkey* toolhotkey = NULL;
            if (mininame != NULL) {
              if (asprintf(&menuname, "%s%s", "Point.Tools.", mininame) != -1) {
	        toolhotkey = hotkeyFindByMenuPath(cv->gw, menuname);
	        free(menuname); menuname = NULL;
              }
              free(mininame); mininame = NULL;
            }
	    char * finalmsg = NULL;
	    if (toolhotkey != NULL && asprintf(&finalmsg, "%s (%s)", msg, toolhotkey->text) != -1) {
	      GGadgetPreparePopup8(cvtools, finalmsg);
	      free(finalmsg); finalmsg = NULL;
	    } else GGadgetPreparePopup8(cvtools, msg); // That's what we were doing before. Much simpler.
	} else if ( pos!=cv->pressed_tool || cv->had_control != (((event->u.mouse.state&ksm_control) || styluscntl)?1:0) )
	    cv->pressed_display = cvt_none;
	else
	    cv->pressed_display = cv->pressed_tool;
    } else if ( event->type == et_mouseup ) {
	if ( pos==cvt_freehand && event->u.mouse.clicks==2 ) {
	    FreeHandStrokeDlg(&expand);
	} else if ( pos==cvt_pointer && event->u.mouse.clicks==2 ) {
	    PointerDlg(cv);
	} else if ( pos==cvt_ruler && event->u.mouse.clicks==2 ) {
	    RulerDlg(cv);
	} else if ( i==cvt_rect/2 && event->u.mouse.clicks==2 ) {
	    ((j==0)?CVRectElipse:CVPolyStar)(cv);
	    mi = i;
	    if ( (j==0 && rectelipse) || (j==1 && polystar) )
		++mi;
	    pos = mi*2 + j;
	    cv->pressed_tool = cv->pressed_display = pos;
	}
	if ( pos!=cv->pressed_tool || cv->had_control != (((event->u.mouse.state&ksm_control)||styluscntl)?1:0) )
	    cv->pressed_tool = cv->pressed_display = cvt_none;
	else {
	    if ( event->u.mouse.device!=NULL && strcmp(event->u.mouse.device,"eraser")==0 )
		cv->er_tool = pos;
	    else if ( isstylus ) {
	        if ( event->u.mouse.button==2 )
		    /* Only thing that matters is touch which maps to button 1 */;
		else if ( cv->had_control )
		    cv->s2_tool = pos;
		else
		    cv->s1_tool = pos;
	    } else if ( cv->had_control && event->u.mouse.button==2 )
		cv->cb2_tool = cv_cb2_tool = pos;
	    else if ( event->u.mouse.button==2 )
		cv->b2_tool = cv_b2_tool = pos;
	    else if ( cv->had_control ) {
		cv->cb1_tool = cv_cb1_tool = pos;
	    } else {
		cv->b1_tool = cv_b1_tool = pos;
	    }
	    SavePrefs(true);
	    cv->pressed_tool = cv->pressed_display = cvt_none;
	}
	GDrawRequestExpose(cvtools,NULL,false);
	event->u.chr.state &= ~(1<<(7+event->u.mouse.button));
    }
    CVToolsSetCursor(cv,event->u.mouse.state,event->u.mouse.device);
}

static void PostCharToWindow(GWindow to, GEvent *e) {
    GPoint p;

    p.x = e->u.chr.x; p.y = e->u.chr.y;
    GDrawTranslateCoordinates(e->w,to,&p);
    e->u.chr.x = p.x; e->u.chr.y = p.y;
    e->w = to;
    GDrawPostEvent(e);
}

static int cvtools_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( event->type==et_destroy && cvtools == gw ) {
	cvtools = NULL;
return( true );
    }

    if ( cv==NULL )
return( true );

    GGadgetPopupExternalEvent(event);
    switch ( event->type ) {
      case et_expose:
	ToolsExpose(gw,cv,&event->u.expose.rect);
      break;
      case et_mousedown:
	ToolsMouse(cv,event);
      break;
      case et_mousemove:
	ToolsMouse(cv,event);
      break;
      case et_mouseup:
	ToolsMouse(cv,event);
      break;
      case et_crossing:
	cv->pressed_display = cvt_none;
	CVToolsSetCursor(cv,event->u.mouse.state,NULL);
      break;
      case et_char: case et_charup:
	if ( cv->had_control != ((event->u.chr.state&ksm_control)?1:0) )
	    cv->pressed_display = cvt_none;
	PostCharToWindow(cv->gw,event);
      break;
      case et_close:
	GDrawSetVisible(gw,false);
      break;
    }
return( true );
}

GWindow CVMakeTools(CharView *cv) {
    GRect r;
    GWindowAttrs wattrs;
    FontRequest rq;

    if ( cvtools!=NULL )
return( cvtools );

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_positioned|wam_isdlg;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.positioned = true;
    wattrs.is_dlg = true;
    wattrs.utf8_window_title = _("Tools");

    r.width = getToolbarWidth(cv); r.height = getToolbarHeight(cv);
    if ( cvtoolsoff.x==-9999 ) {
	cvtoolsoff.x = -r.width-6; cvtoolsoff.y = cv->mbh+20;
    }
    r.x = cvtoolsoff.x; r.y = cvtoolsoff.y;
    if ( palettes_docked )
	r.x = r.y = 0;
    cvtools = CreatePalette( cv->gw, &r, cvtools_e_h, NULL, &wattrs, cv->v );

    if ( GDrawRequestDeviceEvents(cvtools,input_em_cnt,input_em)>0 ) {
	/* Success! They've got a wacom tablet */
    }

    if ( toolsfont==NULL ) {
	memset(&rq,0,sizeof(rq));
	rq.utf8_family_name = SANS_UI_FAMILIES;
	rq.point_size = -10;
	rq.weight = 400;
	toolsfont = GDrawInstanciateFont(NULL,&rq);
	toolsfont = GResourceFindFont("ToolsPalette.Font",toolsfont);
    }

    if ( cvvisible[1])
	GDrawSetVisible(cvtools,true);
return( cvtools );
}


    /* ********************************************************* */
    /* ******************  Layers Palette  ********************* */
    /* ********************************************************* */



/* Create a layer thumbnail */
static BDFChar *BDFCharFromLayer(SplineChar *sc,int layer) {
    SplineChar dummy;
    memset(&dummy,0,sizeof(dummy));
    dummy.layer_cnt = 2;
    dummy.layers = sc->layers+layer-1;
    dummy.parent = sc->parent;
return( SplineCharAntiAlias(&dummy,ly_fore,24,4));
}

/**
 *  \brief Recalculate the number of visible layers,
 *         reposition the visibility checkboxes, and
 *         if requested, reposition/resize the scrollbar.
 *
 *  \param [in] cv The charview
 */
static void CVLayers2Reflow(CharView *cv, bool resize) {
    extern int _GScrollBar_Width;
    GGadget *scrollbar;
    GRect cvl2size;

    GDrawGetSize(cvlayers2, &cvl2size);
    // Minus 2 because we always have the 'Guide' and 'Back' entries
    layer2.visible_layers = (cvl2size.height - layer2.header_height - 2 * CV_LAYERS2_LINE_HEIGHT) / CV_LAYERS2_LINE_HEIGHT;
    if (layer2.visible_layers < 0) {
        layer2.visible_layers = 0;
    }

    // Reposition the visibility checkboxes
    int num_potentially_visible = 2 + layer2.visible_layers + 1;
    if (num_potentially_visible > layer2.current_layers){
        num_potentially_visible = layer2.current_layers;
    }
    for (int i = 2, first_visible = 2 + layer2.offtop; i < layer2.current_layers; i++) {
        GGadget *vis = GWidgetGetControl(cvlayers2, CID_VBase + i - 1);
        if (vis == NULL) {
            break;
        }
        if (i >= first_visible && (i - layer2.offtop) < num_potentially_visible) {
            GGadgetMove(vis, 5, layer2.header_height + (i - layer2.offtop) * CV_LAYERS2_LINE_HEIGHT);
            GGadgetSetVisible(vis, true);
        } else {
            GGadgetSetVisible(vis, false);
        }
    }

    if (!resize) {
        return;
    }

    scrollbar = GWidgetGetControl(cvlayers2, CID_SB);
    // Check if we need the scrollbar or not.
    if (layer2.current_layers - 2 <= layer2.visible_layers || layer2.visible_layers <= 0) {
        GGadgetSetVisible(scrollbar, false);
        layer2.sb_start = cvl2size.width;
        layer2.offtop = 0;
    } else {
        layer2.sb_start = cvl2size.width - GDrawPointsToPixels(cv->gw, _GScrollBar_Width);
        GGadgetMove(scrollbar, layer2.sb_start, 0);
        GGadgetResize(scrollbar, GDrawPointsToPixels(cv->gw, _GScrollBar_Width), cvl2size.height);
        GGadgetSetVisible(scrollbar, true);

        GScrollBarSetBounds(scrollbar, 0, layer2.current_layers - 2, layer2.visible_layers);
        GScrollBarSetPos(scrollbar, layer2.offtop);
    }

    GDrawRequestExpose(cvlayers2, NULL, false);
}

/* Update the type3 layers palette to the given character view */
static void CVLayers2Set(CharView *cv) {
    int i, top;

    GGadgetSetChecked(GWidgetGetControl(cvlayers2,CID_VFore),cv->showfore);
    GGadgetSetChecked(GWidgetGetControl(cvlayers2,CID_VBack),cv->showback[0]&1);
    GGadgetSetChecked(GWidgetGetControl(cvlayers2,CID_VGrid),cv->showgrids);

    int ly = 0;
    // We want to look at the unhandled layers.
    if (ly <= ly_back) ly = ly_back + 1;
    if (ly <= ly_fore) ly = ly_fore + 1;
    if (ly <= ly_grid) ly = ly_grid + 1;
    while (ly < cv->b.sc->parent->layer_cnt) {
      GGadget *tmpgadget = GWidgetGetControl(cvlayers2, CID_VBase + ly);
      if (tmpgadget != NULL) {
        // We set a low cap on the number of layers provisioned with check boxes for safety.
        // So it is important to check that this exists.
        GGadgetSetChecked(tmpgadget, cv->showback[ly>>5]&(1<<(ly&31)));
      }
      ly ++;
    }

	 /* set old to NULL */
    layer2.offtop = 0;
    for ( i=2; i<layer2.current_layers; ++i ) {
	BDFCharFree(layer2.layers[i]);
	layer2.layers[i]=NULL;
    }

	 /* reallocate enough space if necessary */
    if ( cv->b.sc->layer_cnt+1>=layer2.max_layers ) {
	top = cv->b.sc->layer_cnt+10;
	if ( layer2.layers==NULL )
	    layer2.layers = calloc(top,sizeof(BDFChar *));
	else {
	    layer2.layers = realloc(layer2.layers,top*sizeof(BDFChar *));
	    for ( i=layer2.current_layers; i<top; ++i )
		layer2.layers[i] = NULL;
	}
	layer2.max_layers = top;
    }
    layer2.current_layers = cv->b.sc->layer_cnt+1;
    for ( i=ly_fore; i<cv->b.sc->layer_cnt; ++i )
	layer2.layers[i+1] = BDFCharFromLayer(cv->b.sc,i);
    layer2.active = CVLayer(&cv->b)+1;

    CVLayers2Reflow(cv, true);
}

static void Layers2Expose(CharView *cv,GWindow pixmap,GEvent *event) {
    int i, ll;
    const char *str;
    GRect r, oldclip;
    struct _GImage base;
    GImage gi;
    int as = (24*cv->b.sc->parent->ascent)/(cv->b.sc->parent->ascent+cv->b.sc->parent->descent);
    int leftOffset, layerCount;

    if ( event->u.expose.rect.y+event->u.expose.rect.height<layer2.header_height )
return;

    // Calculate the left offset (from the checkboxes)
    GGadgetGetSize(GWidgetGetControl(cvlayers2, CID_VGrid), &r);
    leftOffset = r.x + r.width;
    // Compute the drawable area and clip to it.
    GDrawGetSize(cvlayers2, &r);
    r.x = leftOffset;
    r.width = layer2.sb_start - r.x;
    r.y = layer2.header_height;
    r.height = r.height - layer2.header_height;
    GDrawPushClip(pixmap, &r, &oldclip);
    GDrawFillRect(pixmap,&r,GDrawGetDefaultBackground(NULL));

    GDrawSetDither(NULL, false);	/* on 8 bit displays we don't want any dithering */

    memset(&gi,0,sizeof(gi));
    memset(&base,0,sizeof(base));
    gi.u.image = &base;
    base.image_type = it_index;
    base.clut = layer2.clut;
    base.trans = -1;
    GDrawSetFont(pixmap,layer2.font);

    // +2 for the defaults, +1 to show one extra (could be partially visible)
    layerCount = layer2.visible_layers + 2 + 1;
    if (layerCount > layer2.current_layers) {
        layerCount = layer2.current_layers;
    }
    for (i = 0; i < layerCount; ++i) {
	ll = i<2 ? i : i+layer2.offtop;
	if ( ll==layer2.active ) {
            r.x = leftOffset;
            r.width = layer2.sb_start - r.x;
            r.y = layer2.header_height + i * CV_LAYERS2_LINE_HEIGHT;
	    r.height = CV_LAYERS2_LINE_HEIGHT;
	    GDrawFillRect(pixmap,&r,GDrawGetDefaultForeground(NULL));
	}
        GDrawDrawLine(pixmap, r.x, layer2.header_height + i * CV_LAYERS2_LINE_HEIGHT,
                      r.x + r.width, layer2.header_height + i * CV_LAYERS2_LINE_HEIGHT,
		0x808080);
	if ( i==0 || i==1 ) {
	    str = i==0?_("Guide") : _("Back");
            GDrawDrawText8(pixmap, r.x + 2, layer2.header_height + i * CV_LAYERS2_LINE_HEIGHT + (CV_LAYERS2_LINE_HEIGHT - 12) / 2 + 12,
		    (char *) str,-1,ll==layer2.active?0xffffff:GDrawGetDefaultForeground(NULL));
	} else if ( layer2.offtop+i>=layer2.current_layers ) {
    break;
	} else if ( layer2.layers[layer2.offtop+i]!=NULL ) {
#if 0
	    // This is currently broken, and we do not have time to fix it.
	    BDFChar *bdfc = layer2.layers[layer2.offtop+i];
	    base.data = bdfc->bitmap;
	    base.bytes_per_line = bdfc->bytes_per_line;
	    base.width = bdfc->xmax-bdfc->xmin+1;
	    base.height = bdfc->ymax-bdfc->ymin+1;
	    GDrawDrawImage(pixmap,&gi,NULL,
		    r.x+2+bdfc->xmin,
                           layer2.header_height + i * CV_LAYERS2_LINE_HEIGHT + as - bdfc->ymax);
#else
	    // This logic comes from CVInfoDrawText.
	    const int layernamesz = 100;
	    char layername[layernamesz+1];
	    strncpy(layername,_("Guide"),layernamesz);
	    int idx = layer2.offtop+i-1;
	    if(idx >= 0 && idx < cv->b.sc->parent->layer_cnt) {
	      strncpy(layername,cv->b.sc->parent->layers[idx].name,layernamesz);
	    } else {
	      fprintf(stderr, "Invalid layer!\n");
	    }
	    // And this comes from above.
            GDrawDrawText8(pixmap, r.x + 2, layer2.header_height + i * CV_LAYERS2_LINE_HEIGHT + (CV_LAYERS2_LINE_HEIGHT - 12) / 2 + 12,
		    (char *) layername,-1,ll==layer2.active?0xffffff:GDrawGetDefaultForeground(NULL));
#endif // 0
	}
    }
    GDrawPopClip(pixmap, &oldclip);
}

// Frank changed the prefix from MID to MIDL in order to avert conflicts with values set in charview_private.h.
#define MIDL_LayerInfo	1
#define MIDL_NewLayer	2
#define MIDL_DelLayer	3
#define MIDL_First	4
#define MIDL_Earlier	5
#define MIDL_Later	6
#define MIDL_Last	7
#define MIDL_MakeLine 100
#define MIDL_MakeArc  200
#define MIDL_InsertPtOnSplineAt  2309
#define MIDL_NamePoint  2318
#define MIDL_NameContour  2319

static void CVLayer2Invoked(GWindow v, GMenuItem *mi, GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(v);
    Layer temp;
    int layer = CVLayer(&cv->b);
    SplineChar *sc = cv->b.sc;
    int i;
    char *buts[3];
    buts[0] = _("_Yes"); buts[1]=_("_No"); buts[2] = NULL;

    switch ( mi->mid ) {
      case MIDL_LayerInfo:
	if ( !LayerDialog(cv->b.layerheads[cv->b.drawmode],cv->b.sc->parent))
return;
      break;
      case MIDL_NewLayer:
	LayerDefault(&temp);
	if ( !LayerDialog(&temp,cv->b.sc->parent))
return;
	sc->layers = realloc(sc->layers,(sc->layer_cnt+1)*sizeof(Layer));
	sc->layers[sc->layer_cnt] = temp;
	cv->b.layerheads[dm_fore] = &sc->layers[sc->layer_cnt];
	cv->b.layerheads[dm_back] = &sc->layers[ly_back];
	++sc->layer_cnt;
      break;
      case MIDL_DelLayer:
	if ( sc->layer_cnt==2 )		/* May not delete the last foreground layer */
return;
	if ( gwwv_ask(_("Cannot Be Undone"),(const char **) buts,0,1,_("This operation cannot be undone, do it anyway?"))==1 )
return;
	SplinePointListsFree(sc->layers[layer].splines);
	RefCharsFree(sc->layers[layer].refs);
	ImageListsFree(sc->layers[layer].images);
	UndoesFree(sc->layers[layer].undoes);
	UndoesFree(sc->layers[layer].redoes);
	for ( i=layer+1; i<sc->layer_cnt; ++i )
	    sc->layers[i-1] = sc->layers[i];
	--sc->layer_cnt;
	if ( layer==sc->layer_cnt )
	    cv->b.layerheads[dm_fore] = &sc->layers[layer-1];
      break;
      case MIDL_First:
	if ( layer==ly_fore )
return;
	temp = sc->layers[layer];
	for ( i=layer-1; i>=ly_fore; --i )
	    sc->layers[i+1] = sc->layers[i];
	sc->layers[i+1] = temp;
	cv->b.layerheads[dm_fore] = &sc->layers[ly_fore];
      break;
      case MIDL_Earlier:
	if ( layer==ly_fore )
return;
	temp = sc->layers[layer];
	sc->layers[layer] = sc->layers[layer-1];
	sc->layers[layer-1] = temp;
	cv->b.layerheads[dm_fore] = &sc->layers[layer-1];
      break;
      case MIDL_Later:
	if ( layer==sc->layer_cnt-1 )
return;
	temp = sc->layers[layer];
	sc->layers[layer] = sc->layers[layer+1];
	sc->layers[layer+1] = temp;
	cv->b.layerheads[dm_fore] = &sc->layers[layer+1];
      break;
      case MIDL_Last:
	if ( layer==sc->layer_cnt-1 )
return;
	temp = sc->layers[layer];
	for ( i=layer+1; i<sc->layer_cnt; ++i )
	    sc->layers[i-1] = sc->layers[i];
	sc->layers[i-1] = temp;
	cv->b.layerheads[dm_fore] = &sc->layers[i-1];
      break;
    }
    CVLayers2Set(cv);
    CVCharChangedUpdate(&cv->b);
}

static void Layer2Menu(CharView *cv,GEvent *event, int nolayer) {
    GMenuItem mi[20];
    int i;
    static char *names[] = { N_("Layer Info..."), N_("New Layer..."), N_("Del Layer"), (char *) -1,
	    N_("_First"), N_("_Earlier"), N_("L_ater"), N_("_Last"), NULL };
    static int mids[] = { MIDL_LayerInfo, MIDL_NewLayer, MIDL_DelLayer, -1,
	    MIDL_First, MIDL_Earlier, MIDL_Later, MIDL_Last, 0 };
    int layer = CVLayer(&cv->b);

    memset(mi,'\0',sizeof(mi));
    for ( i=0; names[i]!=0; ++i ) {
	if ( names[i]!=(char *) -1 ) {
	    mi[i].ti.text = (unichar_t *) _(names[i]);
	    mi[i].ti.text_is_1byte = true;
	} else
	    mi[i].ti.line = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = mids[i];
	mi[i].invoke = CVLayer2Invoked;
	if ( mids[i]!=MIDL_NewLayer && nolayer )
	    mi[i].ti.disabled = true;
	if (( mids[i]==MIDL_First || mids[i]==MIDL_Earlier ) && layer==ly_fore )
	    mi[i].ti.disabled = true;
	if (( mids[i]==MIDL_Last || mids[i]==MIDL_Later ) && layer==cv->b.sc->layer_cnt-1 )
	    mi[i].ti.disabled = true;
	if ( mids[i]==MIDL_DelLayer && cv->b.sc->layer_cnt==2 )
	    mi[i].ti.disabled = true;
    }
    GMenuCreatePopupMenu(cvlayers2,event, mi);
}

static void Layer2Scroll(CharView *cv, GEvent *event) {
    int off = 0;
    enum sb sbt = event->u.control.u.sb.type;

    if ( sbt==et_sb_top )
	off = 0;
    else if ( sbt==et_sb_bottom )
	off = cv->b.sc->layer_cnt-1-layer2.visible_layers;
    else if ( sbt==et_sb_up ) {
	off = layer2.offtop-1;
    } else if ( sbt==et_sb_down ) {
	off = layer2.offtop+1;
    } else if ( sbt==et_sb_uppage ) {
	off = layer2.offtop-layer2.visible_layers+1;
    } else if ( sbt==et_sb_downpage ) {
	off = layer2.offtop+layer2.visible_layers-1;
    } else /* if ( sbt==et_sb_thumb || sbt==et_sb_thumbrelease ) */ {
	off = event->u.control.u.sb.pos;
    }
    if ( off>cv->b.sc->layer_cnt-1-layer2.visible_layers )
	off = cv->b.sc->layer_cnt-1-layer2.visible_layers;
    if ( off<0 ) off=0;
    if ( off==layer2.offtop )
return;
    layer2.offtop = off;
    GScrollBarSetPos(GWidgetGetControl(cvlayers2,CID_SB),off);
    CVLayers2Reflow(cv, false);
    GDrawRequestExpose(cvlayers2,NULL,false);
}

static int cvlayers2_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( event->type==et_destroy && cvlayers2 == gw ) {
	cvlayers2 = NULL;
return( true );
    }

    if ( cv==NULL )
return( true );

    switch ( event->type ) {
      case et_close:
	GDrawSetVisible(gw,false);
      break;
      case et_char: case et_charup:
	PostCharToWindow(cv->gw,event);
      break;
      case et_resize:
        CVLayers2Reflow(cv, true);
      break;
      case et_expose:
	Layers2Expose(cv,gw,event);
      break;
      case et_mousedown: {
	if ( event->u.mouse.y>CV_LAYERS2_HEADER_HEIGHT ) {
        if (event->u.mouse.button >= 4) {
            // Scroll the list
            event->u.control.u.sb.type = (event->u.mouse.button == 4 || event->u.mouse.button == 6) ? et_sb_up : et_sb_down;
            Layer2Scroll(cv, event);
            return true;
        }
	int layer = (event->u.mouse.y-CV_LAYERS2_HEADER_HEIGHT)/CV_LAYERS2_LINE_HEIGHT;
	    if ( layer<2 ) {
		cv->b.drawmode = layer==0 ? dm_grid : dm_back;
		layer2.active = layer;
	    } else if ( layer-1+layer2.offtop >= cv->b.sc->layer_cnt ) {
		if ( event->u.mouse.button==3 )
		    Layer2Menu(cv,event,true);
		else
		    GDrawBeep(NULL);
return(true);
	    } else {
		layer2.active = layer+layer2.offtop;
		cv->b.drawmode = dm_fore;
		cv->b.layerheads[dm_fore] = &cv->b.sc->layers[layer-1+layer2.offtop];
	    }
	    GDrawRequestExpose(cvlayers2,NULL,false);
	    GDrawRequestExpose(cv->v,NULL,false);
	    GDrawRequestExpose(cv->gw,NULL,false);	/* the logo (where the scrollbars join) shows what layer we are in */
	    if ( event->u.mouse.button==3 )
		Layer2Menu(cv,event,cv->b.drawmode!=dm_fore);
	    else if ( event->u.mouse.clicks==2 && cv->b.drawmode==dm_fore ) {
		if ( LayerDialog(cv->b.layerheads[cv->b.drawmode],cv->b.sc->parent))
		    CVCharChangedUpdate(&cv->b);
	    }
	}
      } break;
      case et_controlevent:
	if ( event->u.control.subtype == et_radiochanged ) {
	    enum drawmode dm = cv->b.drawmode;
	    int tmpcid = -1;
	    int tmplayer = -1;
	    switch(GGadgetGetCid(event->u.control.g)) {
	      case CID_VFore:
		CVShows.showfore = cv->showfore = GGadgetIsChecked(event->u.control.g);
		if ( CVShows.showback )
		    cv->showback[0] |= 2;
		else
		    cv->showback[0] &= ~2;
	      break;
	      case CID_VBack:
		CVShows.showback = GGadgetIsChecked(event->u.control.g);
		if ( CVShows.showback )
		    cv->showback[0] |= 1;
		else
		    cv->showback[0] &= ~1;
		cv->back_img_out_of_date = true;
	      break;
	      case CID_VGrid:
		CVShows.showgrids = cv->showgrids = GGadgetIsChecked(event->u.control.g);
	      break;
	      default:
		tmpcid = GGadgetGetCid(event->u.control.g);
		tmplayer = tmpcid - CID_VBase;
		if (tmpcid < 0 || tmplayer < 0) break;
		// We check that the layer is valid (since the code does not presently, as far as Frank knows, handle layer deletion).
		// We also check that the CID is within the allocated range (although this may not be necessary since the checkbox would not exist otherwise).
		if (tmplayer > 0 && tmplayer < 999 && tmplayer < cv->b.sc->parent->layer_cnt) {
		  if (GGadgetIsChecked(event->u.control.g)) {
		    cv->showback[tmplayer>>5]|=(1<<(tmplayer&31));
		  } else {
		    cv->showback[tmplayer>>5]&=~(1<<(tmplayer&31));
		  }
		}
		break;
	    }
	    GDrawRequestExpose(cv->v,NULL,false);
	    if ( dm!=cv->b.drawmode )
		GDrawRequestExpose(cv->gw,NULL,false);	/* the logo (where the scrollbars join) shows what layer we are in */
	} else
	    Layer2Scroll(cv,event);
      break;
    }
return( true );
}

/* This is used for Type 3 fonts. CVMakeLayers is used for other fonts. */
static void CVMakeLayers2(CharView *cv) {
    GRect r;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[25];
    GTextInfo label[25];
    static GBox radio_box = { bt_none, bs_rect, 0, 0, 0, 0, 0, 0, 0, 0, COLOR_DEFAULT, COLOR_DEFAULT, 0, 0, 0, 0, 0, 0, 0 };
    FontRequest rq;
    int i;
    extern int _GScrollBar_Width;

    if ( layer2.clut==NULL )
	layer2.clut = _BDFClut(4);
    if ( cvlayers2!=NULL )
return;
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_positioned|wam_isdlg;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.positioned = true;
    wattrs.is_dlg = true;
    wattrs.utf8_window_title = _("Layers");

    r.width = GGadgetScale(CV_LAYERS2_WIDTH); r.height = CV_LAYERS2_HEIGHT;
    if ( cvlayersoff.x==-9999 ) {
	cvlayersoff.x = -r.width-6;
	cvlayersoff.y = cv->mbh+getToolbarHeight(cv)+45/*25*/;	/* 45 is right if there's decor, 25 when none. twm gives none, kde gives decor */
    }
    r.x = cvlayersoff.x; r.y = cvlayersoff.y;
    if ( palettes_docked ) { r.x = 0; r.y=getToolbarHeight(cv)+2; }
    cvlayers2 = CreatePalette( cv->gw, &r, cvlayers2_e_h, NULL, &wattrs, cv->v );

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    if ( layersfont==NULL ) {
	memset(&rq,'\0',sizeof(rq));
	rq.utf8_family_name = SANS_UI_FAMILIES;
	rq.point_size = -12;
	rq.weight = 400;
	layersfont = GDrawInstanciateFont(cvlayers2,&rq);
	layersfont = GResourceFindFont("LayersPalette.Font",layersfont);
    }

    for ( i=0; i<sizeof(label)/sizeof(label[0]); ++i )
	label[i].font = layersfont;
    layer2.font = layersfont;

    gcd[0].gd.pos.width = GDrawPointsToPixels(cv->gw,_GScrollBar_Width);
    gcd[0].gd.pos.x = CV_LAYERS2_WIDTH-gcd[0].gd.pos.width;
    gcd[0].gd.pos.y = CV_LAYERS2_HEADER_HEIGHT+2*CV_LAYERS2_LINE_HEIGHT;
    gcd[0].gd.pos.height = CV_LAYERS2_HEIGHT-gcd[0].gd.pos.y;
    gcd[0].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels|gg_sb_vert;
    gcd[0].gd.cid = CID_SB;
    gcd[0].creator = GScrollBarCreate;
    layer2.sb_start = gcd[0].gd.pos.x;

/* GT: Abbreviation for "Visible" */
    label[1].text = (unichar_t *) _("V");
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 7; gcd[1].gd.pos.y = 5; 
    gcd[1].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels|gg_utf8_popup;
    gcd[1].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[1].creator = GLabelCreate;

    label[2].text = (unichar_t *) _("Layer");
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 30; gcd[2].gd.pos.y = 5; 
    gcd[2].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels|gg_utf8_popup;
    gcd[2].gd.cid = CID_LayerLabel;
    gcd[2].gd.popup_msg = (unichar_t *) _("Is Layer Editable?");
    gcd[2].creator = GLabelCreate;

    gcd[3].gd.pos.x = 5; gcd[3].gd.pos.y = CV_LAYERS2_HEADER_HEIGHT+(CV_LAYERS2_LINE_HEIGHT-12)/2; 
    gcd[3].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[3].gd.cid = CID_VGrid;
    gcd[3].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[3].gd.box = &radio_box;
    gcd[3].creator = GCheckBoxCreate;

    gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = gcd[3].gd.pos.y+CV_LAYERS2_LINE_HEIGHT; 
    gcd[4].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[4].gd.cid = CID_VBack;
    gcd[4].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[4].gd.box = &radio_box;
    gcd[4].creator = GCheckBoxCreate;

    gcd[5].gd.pos.x = 5; gcd[5].gd.pos.y = gcd[4].gd.pos.y+CV_LAYERS2_LINE_HEIGHT; 
    gcd[5].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[5].gd.cid = CID_VFore;
    gcd[5].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[5].gd.box = &radio_box;
    gcd[5].creator = GCheckBoxCreate;

    int wi = 6; // Widget index.
    int ly = 0;
    // We want to look at the unhandled layers.
    if (ly <= ly_back) ly = ly_back + 1;
    if (ly <= ly_fore) ly = ly_fore + 1;
    if (ly <= ly_grid) ly = ly_grid + 1;
    while (ly < cv->b.sc->parent->layer_cnt && wi < 24) {
      gcd[wi].gd.pos.x = 5; gcd[wi].gd.pos.y = gcd[wi-1].gd.pos.y+CV_LAYERS2_LINE_HEIGHT; 
      gcd[wi].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
      gcd[wi].gd.cid = CID_VBase + ly; // There are plenty of CID values available for these above CID_VBase.
      gcd[wi].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
      gcd[wi].gd.box = &radio_box;
      gcd[wi].creator = GCheckBoxCreate;
      ly++;
      wi++;
    }

    if ( cv->showgrids ) gcd[3].gd.flags |= gg_cb_on;
    if ( cv->showback[0]&1 ) gcd[4].gd.flags |= gg_cb_on;
    if ( cv->showfore ) gcd[5].gd.flags |= gg_cb_on;

    GGadgetsCreate(cvlayers2, gcd);
    // Calculate the header height. The '-1' is magic! Needed to obtain symmetry
    layer2.header_height = GGadgetGetY(GWidgetGetControl(cvlayers2, CID_VGrid)) - 1;
    // Move the "Layer" label into the correct position based on the width of the checkbox
    GGadgetGetSize(GWidgetGetControl(cvlayers2, CID_VGrid), &r);
    GGadgetMove(GWidgetGetControl(cvlayers2, CID_LayerLabel), r.x + r.width, 5);
    if ( cvvisible[0] )
	GDrawSetVisible(cvlayers2,true);
}

static void LayersSwitch(CharView *cv) {
}

void SC_MoreLayers(SplineChar *sc, Layer *old) { /* We've added more layers */
    CharView *curcv, *cv;
    if ( sc->parent==NULL || !sc->parent->multilayer )
return;
    for ( cv=(CharView *) (sc->views); cv!=NULL ; cv=(CharView *) (cv->b.next) ) {
	cv->b.layerheads[dm_fore] = &cv->b.sc->layers[cv->b.layerheads[dm_fore]-old];
	cv->b.layerheads[dm_back] = &cv->b.sc->layers[ly_back];
    }
    if ( cvtools==NULL )
return;
    curcv = GDrawGetUserData(cvtools);
    if ( curcv==NULL || curcv->b.sc!=sc )
return;
    CVLayers2Set(curcv);
}

void SCLayersChange(SplineChar *sc) { /* many of the foreground layers need to be redrawn */
    CharView *curcv;
    if ( cvtools==NULL || !sc->parent->multilayer )
return;
    curcv = GDrawGetUserData(cvtools);
    if ( curcv==NULL || curcv->b.sc!=sc )
return;
    CVLayers2Set(curcv);
}

void CVLayerChange(CharView *cv) { /* Current layer needs to be redrawn */
    CharView *curcv;
    int layer;

    if ( cvtools==NULL  || !cv->b.sc->parent->multilayer )
return;
    curcv = GDrawGetUserData(cvtools);
    if ( curcv!=cv )
return;
    if ( cv->b.drawmode==dm_grid || cv->b.drawmode==dm_back )
return;
    layer = CVLayer(&cv->b);
    BDFCharFree(layer2.layers[layer+1]);
    layer2.layers[layer+1] = BDFCharFromLayer(cv->b.sc,layer);
    GDrawRequestExpose(cvlayers2,NULL,false);
}

/* Update the state of the controls of the non-type3 layers palette to the given character view */
/* New widgets are not allocated here. For that, see CVLCheckLayerCount(). */
static void CVLayers1Set(CharView *cv) {
    int i, top;

    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VFore),cv->showfore);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VBack),cv->showback[0]&1);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VGrid),cv->showgrids);

     /* clear old layer previews */
    layerinfo.offtop = 0;
    for ( i=2; i<layerinfo.current_layers; ++i ) {
	BDFCharFree(layerinfo.layers[i]);
	layerinfo.layers[i]=NULL;
    }

     /* reallocate enough space if necessary */
    if ( cv->b.sc->layer_cnt+1>=layerinfo.max_layers ) {
	top = cv->b.sc->layer_cnt+10;
	if ( layerinfo.layers==NULL )
	    layerinfo.layers = calloc(top,sizeof(BDFChar *));
	else {
	    layerinfo.layers = realloc(layerinfo.layers,top*sizeof(BDFChar *));
	    for ( i=layerinfo.current_layers; i<top; ++i )
		layerinfo.layers[i] = NULL;
	}
	layerinfo.max_layers = top;
    }
    layerinfo.current_layers = cv->b.sc->layer_cnt+1;
    for ( i=ly_fore; i<cv->b.sc->layer_cnt; ++i )
	layerinfo.layers[i+1] = BDFCharFromLayer(cv->b.sc,i);
    layerinfo.active = CVLayer(&cv->b)+1;

    if ( layerinfo.visible_layers==0 ) {
        GRect size;
        GDrawGetSize(cvlayers,&size);
        layerinfo.visible_layers=(size.height-layer_header_height)/layer_height;
    }
    GScrollBarSetBounds(GWidgetGetControl(cvlayers,CID_SB),0,cv->b.sc->layer_cnt+1-2, layerinfo.visible_layers);
    if ( layerinfo.offtop>cv->b.sc->layer_cnt-1-layerinfo.visible_layers )
        layerinfo.offtop = cv->b.sc->layer_cnt-1-layerinfo.visible_layers;
    if ( layerinfo.offtop<0 ) layerinfo.offtop = 0;
    GScrollBarSetPos(GWidgetGetControl(cvlayers,CID_SB),layerinfo.offtop);

    for ( i=0; i<cv->b.sc->layer_cnt; i++ ) {
        GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VBase+i),cv->showback[i>>5]&(1<<(i&31)));
    }

    layerinfo.active = CVLayer(&cv->b); /* the index of the active layer */
    GDrawRequestExpose(cvlayers,NULL,false);
}

/* Update the layers palette to reflect the given character view. No new gadgets
 * are created or hid here, only the state of existing gadgets is changed.
 * New layer gadgets are created in CVLCheckLayerCount(). */
void CVLayersSet(CharView *cv) {
    if( cv )
	onCollabSessionStateChanged( NULL, cv->b.fv, NULL );
    
    if ( cv->b.sc->parent->multilayer ) {
	CVLayers2Set(cv);
return;
    }
     /* This is for the non-type3 layers palette: */
    CVLayers1Set(cv);
}

/**
 * Get the offset at the right hand size of the eyeball to show/hide
 * a layer. This is the x offset where the Q/C indicators might be drawn.
 */
static int32 Layers_getOffsetAtRightOfViewLayer(CharView *cv)
{
    int32 ret = 64;
    GGadget *v = GWidgetGetControl(cvlayers,CID_VBack);
    if( v )
    {
	GRect size;
	GGadgetGetSize(v,&size);
	ret = 7 + size.width;
    }
    return ret;
}


 /* Draw the fg/bg, cubic/quadratic columns, plus layer preview and label name */
static void LayersExpose(CharView *cv,GWindow pixmap,GEvent *event) {
    int i, ll, y;
    const char *str;
    GRect r;
    struct _GImage base;
    GImage gi;
    Color mocolor = ACTIVE_BORDER; /* mouse over color */
    int ww;

    int yt = .7*layer_height; /* vertical spacer to add when drawing text in the row */
    int column_width;
    int quadcol, fgcol, editcol;

    if ( event->u.expose.rect.y+event->u.expose.rect.height<layer_header_height )
return;

    int offsetAtRightOfViewLayer = Layers_getOffsetAtRightOfViewLayer(cv);
    column_width = layerinfo.column_width;
    
    GDrawSetDither(NULL, false);	/* on 8 bit displays we don't want any dithering */
    ww=layerinfo.sb_start;

    memset(&gi,0,sizeof(gi));
    memset(&base,0,sizeof(base));
    gi.u.image = &base;
    base.image_type = it_index;
    base.clut = layer2.clut;
    base.trans = -1;
    GDrawSetFont(pixmap,layerinfo.font);

    quadcol=fgcol=offsetAtRightOfViewLayer;
    if ( layerscols & LSHOW_CUBIC )
    {
	/* show quad col */
	quadcol = offsetAtRightOfViewLayer;
	fgcol   = offsetAtRightOfViewLayer+column_width;
    } 
    if ( layerscols & LSHOW_FG )
    {
	/* show fg col */
	fgcol = quadcol+column_width;
    }
    // editcol is the X offset where the layer name label should be drawn
    editcol = fgcol+column_width;
    int bottomOfLast = 0;
    
     /* loop once per layer, where 0==guides, 1=back, 2=fore, etc */
    for ( i=(event->u.expose.rect.y-layer_header_height)/layer_height;
	    i<(event->u.expose.rect.y+event->u.expose.rect.height+layer_height-layer_header_height)/layer_height;
	    ++i ) {
	ll = i-1+layerinfo.offtop;
        if ( ll>=cv->b.sc->layer_cnt || ll<-1 ) continue;

        y = layer_header_height + i*layer_height;
	bottomOfLast = y + layer_height;
        if ( y<layer_header_height ) continue;

         /* draw quadratic/cubic toggle */
        if ( layerscols & LSHOW_CUBIC ) {
            if ( layerinfo.mo_layer==ll && layerinfo.mo_col==CID_QBase ) {
                r.x = quadcol; r.width = column_width;
                r.y = y;
                r.height = layer_height;
                GDrawFillRect(pixmap,&r,mocolor);
            }
            str = ( ll>=0 && ll<cv->b.sc->layer_cnt ? (cv->b.sc->layers[ll].order2? "Q" : "C") : " ");
	    GDrawDrawText8(pixmap, quadcol, y + yt,
		    (char *) str,-1,GDrawGetDefaultForeground(NULL));
        }

         /* draw fg/bg toggle */
        if ( layerscols & LSHOW_FG ) {
            if ( layerinfo.mo_layer==ll && layerinfo.mo_col==CID_FBase ) {
                r.x = fgcol; r.width = column_width;
                r.y = y;
                r.height = layer_height;
                GDrawFillRect(pixmap,&r,mocolor);
            }
            str = ( ll>=0 && ll<cv->b.sc->layer_cnt ? (cv->b.sc->layers[ll].background? "B" : "F") : "#");
	    GDrawDrawText8(pixmap, fgcol, y + yt,
		    (char *) str,-1,GDrawGetDefaultForeground(NULL));
        }

         /* draw layer thumbnail and label */
	if ( ll==layerinfo.active ) {
            r.x = editcol; r.width = ww-r.x;
	    r.y = y;
	    r.height = layer_height;
	    GDrawFillRect(pixmap,&r,GDrawGetDefaultForeground(NULL));
	} else if ( layerinfo.mo_layer==ll && layerinfo.mo_col==CID_EBase ) {
            r.x = editcol; r.width = ww-r.x;
            r.y = y;
            r.height = layer_height;
            GDrawFillRect(pixmap,&r,mocolor);
        }
        r.x=editcol;
	if ( ll==-1 || ll==0 || ll==1) {
	    str = ll==-1 ? _("Guide") : (ll==0 ?_("Back") : _("Fore")) ;
	    GDrawDrawText8(pixmap,r.x+2,y + yt,
		    (char *) str,-1,ll==layerinfo.active?0xffffff:GDrawGetDefaultForeground(NULL));
	} else if ( ll>=layerinfo.current_layers ) {
             break; /* no more layers to draw! */
	} else if ( ll>=0 && layerinfo.layers[ll]!=NULL ) {
	    BDFChar *bdfc = layerinfo.layers[ll];
	    base.data = bdfc->bitmap;
	    base.bytes_per_line = bdfc->bytes_per_line;
	    base.width = bdfc->xmax-bdfc->xmin+1;
	    base.height = bdfc->ymax-bdfc->ymin+1;
//	    GDrawDrawImage(pixmap,&gi,NULL,
//		    r.x+2+bdfc->xmin,
//		    y+as-bdfc->ymax);
            str = cv->b.sc->parent->layers[ll].name;
            if ( !str || !*str ) str="-";
	    GDrawDrawText8(pixmap, r.x+2, y + yt,
		        (char *) str,-1,ll==layerinfo.active?0xffffff:GDrawGetDefaultForeground(NULL));
	}
    }

    if( bottomOfLast )
    {
	GGadgetSetY(GWidgetGetControl(cvlayers,CID_AddLayer),    bottomOfLast + 2 );
	GGadgetSetY(GWidgetGetControl(cvlayers,CID_RemoveLayer), bottomOfLast + 2 );
	GGadgetSetY(GWidgetGetControl(cvlayers,CID_LayersMenu),  bottomOfLast + 2 );
    }
    
}

/* Remove the layer rename edit box. If save!=0, then record the text as the new layer name. */
static void CVLRemoveEdit(CharView *cv, int save) {
    if ( layerinfo.rename_active ) {
	GGadget *g = GWidgetGetControl(cvlayers,CID_Edit);
	const unichar_t *str = GGadgetGetTitle(g);
	int l = layerinfo.active;

	if ( save
		&& layerinfo.active>=0 && str!=NULL && str[0]!='\0' 
		&& uc_strcmp( str,cv->b.sc->parent->layers[l].name) ) {
	    free( cv->b.sc->parent->layers[l].name );
	    cv->b.sc->parent->layers[l].name = cu_copy( str );

	    CVLCheckLayerCount(cv,true);
	    CVLayersSet(cv);
	}
	GGadgetSetVisible(g,false);
	GDrawRequestExpose(cvlayers,NULL,false);

	layerinfo.rename_active = 0;
	CVInfoDrawText(cv,cv->gw);
    }
}

 /* Make sure we've got the right number of gadgets in the layers palette, and that
  * they are positioned properly. Their state are updated in CVLayers1Set().
  * If resize, then make the palette fit the layers up to a max number of layers. */
static void CVLCheckLayerCount(CharView *cv, int resize) {

    SplineChar *sc = cv->b.sc;
    int i;
    GGadgetCreateData gcd[4];
    GTextInfo label[3];
    GRect size;
    int width;
    int maxwidth=0;
    int togsize=0;
    int x, y;
    int column_width = layerinfo.column_width;
    char namebuf[40];
    int viscol=0, quadcol, fgcol, editcol;
    extern int _GScrollBar_Width;
    int offsetAtRightOfViewLayer = Layers_getOffsetAtRightOfViewLayer(cv);

    if (layerinfo.rename_active) CVLRemoveEdit(cv,true);

    quadcol=fgcol=offsetAtRightOfViewLayer;
    if ( layerscols & LSHOW_CUBIC )
    {
	quadcol = offsetAtRightOfViewLayer;
	fgcol   = offsetAtRightOfViewLayer+column_width;
    }
    if ( layerscols & LSHOW_FG )
    {
	fgcol = quadcol+column_width;
    }
    // editcol is the X offset where the layer name label should be drawn
    editcol = fgcol+column_width;

    /* First figure out if we need to create any new widgets. If we have more */
    /* widgets than we need, we just set them to be invisible.                */
    if ( sc->layer_cnt > layers_max ) {
	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));
	for ( i=layers_max; i<sc->layer_cnt; ++i ) {
	     /* for each new layer, create new widgets */

	     /* Visibility toggle */
	    gcd[0].gd.flags = gg_enabled|gg_utf8_popup;
	    gcd[0].gd.cid = CID_VBase+i;
	    gcd[0].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
	    gcd[0].creator = GVisibilityBoxCreate;

	    GGadgetsCreate(cvlayers,gcd);
	    GVisibilityBoxSetToMinWH(GWidgetGetControl(cvlayers,CID_VBase+i));
	}
	layers_max = sc->layer_cnt;
    }

    /* Then position everything, and name it properly */

    GDrawSetFont(cvlayers,layerinfo.font); /* for width finding code, need this */

     /* First need to position the add, remove, and layers gadgets */
    GGadgetGetSize(GWidgetGetControl(cvlayers,CID_RemoveLayer),&size);
    x = 7+size.width;
    y = layer_header_height;
    GGadgetMove(GWidgetGetControl(cvlayers,CID_AddLayer), x, 5);
    GGadgetSetSize(GWidgetGetControl(cvlayers,CID_AddLayer),&size);
    GGadgetGetSize(GWidgetGetControl(cvlayers,CID_AddLayer),&size);
    x += size.width;
    GGadgetGetSize(GWidgetGetControl(cvlayers,CID_LayersMenu),&size);
    GGadgetMove(GWidgetGetControl(cvlayers,CID_LayersMenu), x+5, 5+(y-8-size.height)/2);
    maxwidth=x+5+size.width;

    if ( !resize )
    {
         /* adjust the number of layers that can be visible in the palette */
        GDrawGetSize(cvlayers,&size);
        layerinfo.visible_layers=(size.height-layer_header_height)/layer_height;
        if ( layerinfo.offtop+layerinfo.visible_layers>=sc->layer_cnt )
            layerinfo.offtop = sc->layer_cnt-layerinfo.visible_layers;
        if ( layerinfo.offtop<0 ) layerinfo.offtop=0;
    }
    if ( layerinfo.visible_layers<2 ) layerinfo.visible_layers=2;

     /* Now position each layer row */
    for ( i=-1; i<layers_max; ++i ) {
	GGadget *v = GWidgetGetControl(cvlayers,CID_VBase+i);

        width=0;
	togsize = editcol;
	
	if ( i>=0 && i<sc->layer_cnt ) {
	    char *hasmn = strchr(sc->parent->layers[i].name,'_');
	    if ( hasmn==NULL && i>=2 && i<9 && strlen(sc->parent->layers[i].name)<30 ) {
		 /* For the first 10 or so layers, add a mnemonic like "(_3)" to the name label */
		 /* if it does not already have a mnemonic.                                     */
		/* sprintf(namebuf, "%s (_%d)", sc->parent->layers[i].name, i+1); */
		sprintf(namebuf, "%s", sc->parent->layers[i].name);
	    } else if ( hasmn==NULL ) {
                sprintf(namebuf,"%s", i==-1 ? _("Guide") : (i==0 ?_("Back") : _("Fore")) );
            }
            width = GDrawGetText8Width(cvlayers, namebuf, -1);
	    width += 10; // padding takes up some space.
	    if ( width+togsize>maxwidth ) maxwidth = width + togsize;
	} else if ( i==-1 ) {
	    if ( width+togsize>maxwidth ) maxwidth = width + togsize;
        }

	if ( i+1<layerinfo.offtop || i>=layerinfo.offtop+layerinfo.visible_layers ||
		(sc->layer_cnt<=layerinfo.visible_layers && i>=sc->layer_cnt)) {
             /* layer is currently scrolled out of palette */
	    GGadgetSetVisible(v,false);
	} else {
	    GGadgetMove(v,viscol ,y);
	    GGadgetSetVisible(v,true);
	    y += layer_height;
	}
    }

     /* Update the scroll bar */
    if ( sc->layer_cnt+1<=layerinfo.visible_layers ) {
         /* don't need the scroll bar, so turn it off */
	GGadgetSetVisible(GWidgetGetControl(cvlayers,CID_SB),false);
    } else {
	if( !resize )
	{
	    GGadget *sb = GWidgetGetControl(cvlayers,CID_SB);
	    maxwidth += 2 + GDrawPointsToPixels(cv->gw,_GScrollBar_Width);
	    GScrollBarSetBounds(sb,0,sc->layer_cnt,layerinfo.visible_layers);
	    GScrollBarSetPos(sb,cv->layers_off_top);
	    GGadgetSetVisible(sb,true);
	}
    }

     /* Resize the palette to fit */
    if ( resize )
    {
        y += GDrawPointsToPixels(NULL,3);
        GDrawGetSize(cvlayers,&size);
	GDrawResize(cvlayers,maxwidth,y+layer_footer_height);
    }

    GDrawGetSize(cvlayers,&size);
    layerinfo.sb_start = size.width
                - (sc->layer_cnt+1<=layerinfo.visible_layers ? 0 : GDrawPointsToPixels(cv->gw,_GScrollBar_Width));
    GGadget *sb = GWidgetGetControl(cvlayers,CID_SB);
    GGadgetResize(sb, GDrawPointsToPixels(cv->gw,_GScrollBar_Width), size.height-layer_header_height);
    GGadgetMove(sb,layerinfo.sb_start,layer_header_height);

    GDrawRequestExpose(cvlayers,NULL,false);
}

/* Respond to scroll events from cvlayers scrollbar. */
static void LayerScroll(CharView *cv, GEvent *event) {
    int off = 0;
    enum sb sbt = event->u.control.u.sb.type;

    if ( sbt==et_sb_top )
	off = 0;
    else if ( sbt==et_sb_bottom )
	off = cv->b.sc->layer_cnt-layerinfo.visible_layers;
    else if ( sbt==et_sb_up ) {
	off = cv->layers_off_top-1;
    } else if ( sbt==et_sb_down ) {
	off = cv->layers_off_top+1;
    } else if ( sbt==et_sb_uppage ) {
	off = cv->layers_off_top-layerinfo.visible_layers+1;
    } else if ( sbt==et_sb_downpage ) {
	off = cv->layers_off_top+layerinfo.visible_layers-1;
    } else /* if ( sbt==et_sb_thumb || sbt==et_sb_thumbrelease ) */ {
	off = event->u.control.u.sb.pos;
    }
    if ( off>cv->b.sc->layer_cnt-layerinfo.visible_layers )
	off = cv->b.sc->layer_cnt-layerinfo.visible_layers;
    if ( off<0 ) off=0;
    if ( off==cv->layers_off_top )
return;
    cv->layers_off_top = off;
    layerinfo.offtop = off;
    CVLCheckLayerCount(cv, false);
    GScrollBarSetPos(GWidgetGetControl(cvlayers,CID_SB),off);
    GDrawRequestExpose(cvlayers,NULL,false);
}

/* Layers palette menu ids */
#define LMID_LayerInfo	1
#define LMID_NewLayer	2
#define LMID_DelLayer	3
#define LMID_Fill       4
#define LMID_First	5
#define LMID_Up     	6
#define LMID_Down	7
#define LMID_Last	8
#define LMID_Foreground 9
#define LMID_Background 10
#define LMID_Cubic      11
#define LMID_Quadratic  12
#define LMID_ShowCubic  13
#define LMID_ShowFore   14

/* Return a unique layer name based on base.
 * This just appends a number to base until the name is not found. */
static char *UniqueLayerName(SplineChar *sc, const char *base)
{
    static char buffer[100];
    const char *basestr=base;
    int i=1, c;
    
    if ( basestr==NULL || basestr[0]=='\0' ) basestr=_("Layer");

    while (1) {
        if (i==1) sprintf( buffer,"%s",basestr );
        else sprintf( buffer,"%s %d",basestr, i );
        
        for (c=0; c<sc->layer_cnt; c++) {
            if (!strcmp(sc->parent->layers[c].name,buffer)) break;
        }

        if ( c==sc->layer_cnt ) break;
        i++;
    }

    return buffer;
}

/* Layers palette menu selection */
static void CVLayerInvoked(GWindow v, GMenuItem *mi, GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(v);
    int layer = CVLayer(&cv->b);
    SplineChar *sc = cv->b.sc;
    Layer temp;
    int i;
    char *buts[3];
    buts[0] = _("_Yes"); buts[1]=_("_No"); buts[2] = NULL;

    switch ( mi->mid ) {
      case LMID_Fill:
        cv->showfilled = !cv->showfilled;
        CVRegenFill(cv);
        GDrawRequestExpose(cv->v,NULL,false);
      break;

      case LMID_ShowCubic:
        layerscols=(layerscols&~LSHOW_CUBIC)|((layerscols&LSHOW_CUBIC)?0:1);
        CVLCheckLayerCount(cv, true);
      break;

      case LMID_ShowFore:
        layerscols=(layerscols&~LSHOW_FG)|((layerscols&LSHOW_FG)?0:2);
        CVLCheckLayerCount(cv, true);
      break;

      case LMID_Foreground:
        if ( layer>ly_fore && cv->b.sc->parent->layers[layer].background==1) {
            SFLayerSetBackground(cv->b.sc->parent,layer,0);
	    GDrawRequestExpose(cvlayers,NULL,false);
        }
      break;

      case LMID_Background:
        if ( layer>=ly_fore && cv->b.sc->parent->layers[layer].background==0) {
            SFLayerSetBackground(cv->b.sc->parent,layer,1);
	    GDrawRequestExpose(cvlayers,NULL,false);
        }
      break;

      case LMID_Cubic:
        if ( layer!=ly_grid && cv->b.sc->layers[layer].order2 ) {
            SFConvertLayerToOrder3(cv->b.sc->parent, layer);
	    GDrawRequestExpose(cvlayers,NULL,false);
            cv->back_img_out_of_date = true;
        }
      break;

      case LMID_Quadratic:
        if ( layer!=ly_grid && !cv->b.sc->layers[layer].order2 ) {
            SFConvertLayerToOrder2(cv->b.sc->parent, layer);
	    GDrawRequestExpose(cvlayers,NULL,false);
            cv->back_img_out_of_date = true;
        }
      break;

      case LMID_NewLayer:
        SFAddLayer(cv->b.sc->parent, /* font of the glyph in the charview */
                        UniqueLayerName(sc,_("Back")),     /* Name */
                        0,         /* 0=cubic, 1=quad */
                        1);       /* 1=back,  0=fore */

        layer=cv->b.sc->parent->layer_cnt-1;
        
        cv->showback[layer>>5] |=  (1<<(layer&31)); /* make it visible */
        CVLCheckLayerCount(cv, true); /* update widget existence */
        CVLayersSet(cv);       /* update widget state     */
      break;
      case LMID_DelLayer:
        layer = CVLayer((CharViewBase *) cv); /* the index of the active layer */
        if (layer==ly_fore || layer==ly_back || layer==ly_grid)
return;
        if ( gwwv_ask(_("Cannot Be Undone"),(const char **) buts,0,1,_("This operation cannot be undone, do it anyway?"))==1 )
return;
        SFRemoveLayer(cv->b.sc->parent, layer);
        CVLCheckLayerCount(cv, true); /* update widget existence */
        CVLayersSet(cv);       /* update widget state     */
      break;

      case LMID_First: /* move layer contents to top */
	if ( layer==ly_fore )
return;
	temp = sc->layers[layer];
	for ( i=layer-1; i>=ly_fore; --i )
	    sc->layers[i+1] = sc->layers[i];
	sc->layers[i+1] = temp;
	cv->b.layerheads[dm_fore] = &sc->layers[ly_fore];
      break;
      case LMID_Up: /* move layer contents one up */
	if ( layer==ly_fore )
return;
	temp = sc->layers[layer];
	sc->layers[layer] = sc->layers[layer-1];
	sc->layers[layer-1] = temp;
	cv->b.layerheads[dm_fore] = &sc->layers[layer-1];
      break;
      case LMID_Down: /* move layer contents one down */
	if ( layer==sc->layer_cnt-1 )
return;
	temp = sc->layers[layer];
	sc->layers[layer] = sc->layers[layer+1];
	sc->layers[layer+1] = temp;
	cv->b.layerheads[dm_fore] = &sc->layers[layer+1];
      break;
      case LMID_Last:
	if ( layer==sc->layer_cnt-1 )
return;
	temp = sc->layers[layer]; /* move layer contents to bottom */
	for ( i=layer+1; i<sc->layer_cnt; ++i )
	    sc->layers[i-1] = sc->layers[i];
	sc->layers[i-1] = temp;
	cv->b.layerheads[dm_fore] = &sc->layers[i-1];
      break;
    }
    CVLayersSet(cv);
    CVCharChangedUpdate(&cv->b);
}

/* Pop up the layers palette context menu */
static void LayerMenu(CharView *cv,GEvent *event, int nolayer) {
    GMenuItem mi[20];
    int i;
    static char *names[] = {
                     /*N_("Rename Layer..."),*/
                     N_("New Layer"),
                     N_("Del Layer"),
                     (char *) -1,
                     N_("Shift Contents To _First"),
                     N_("Shift Contents _Up"),
                     N_("Shift Contents _Down"),
                     N_("Shift Contents To _Last"),
                     (char *) -1,
                     N_("Make Foreground"),/* or N_("Make Background"), */
                     N_("Make Cubic"),     /* or N_("Make Quadratic"),  */
                     (char *) -1,
                     N_("Fill"),
                     (char *) -1,
                     N_("Show Cubic Column"),
                     N_("Show Fore/Back Column"),
                     NULL,
                    };
    static int mids[] = {
                     /*LMID_RenameLayer,*/
                     LMID_NewLayer,
                     LMID_DelLayer,
                     -1,
                     LMID_First,
                     LMID_Up,
                     LMID_Down,
                     LMID_Last,
                     -1,
                     LMID_Foreground, /* or LMID_Background, */
                     LMID_Cubic,      /* or LMID_Quadratic,  */
                     -1,
                     LMID_Fill,
                     -1,
                     LMID_ShowCubic,
                     LMID_ShowFore,
                     0
                    };
    int layer = CVLayer(&cv->b);

    memset(mi,'\0',sizeof(mi));
    for ( i=0; names[i]!=0; ++i ) {
	if ( names[i]!=(char *) -1 ) {
	    mi[i].ti.text = (unichar_t *) _(names[i]);
	    mi[i].ti.text_is_1byte = true;
	    mi[i].ti.text_in_resource = true;
	} else
	    mi[i].ti.line = true;

	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = mids[i];
	mi[i].invoke = CVLayerInvoked;

	/*if ( mids[i]!=LMID_NewLayer && nolayer )
	    mi[i].ti.disabled = true;*/

	if ( ( mids[i]==LMID_First || mids[i]==LMID_Up ) && ( layer==-1 || layer==0) )
	    mi[i].ti.disabled = true;

        else if ( ( mids[i]==LMID_Last || mids[i]==LMID_Down ) && (layer==ly_grid || layer==cv->b.sc->layer_cnt-1) )
	    mi[i].ti.disabled = true;

        else if ( mids[i]==LMID_DelLayer && ( layer<2 || cv->b.sc->layer_cnt==2) )
	    mi[i].ti.disabled = true;

        else if ( mids[i]==LMID_Fill ) {
            mi[i].ti.checkable = 1;
            mi[i].ti.checked = cv->showfilled;

        } else if ( mids[i]==LMID_Foreground ) {
            if ( layer>=0 ) {
                if ( ! cv->b.sc->layers[layer].background ) {
                    mi[i].mid = LMID_Background;
                    mi[i].ti.text = (unichar_t *) _("Make Background");
                }
            } else {
	        mi[i].ti.disabled = true;
            }
        } else if ( mids[i]==LMID_Cubic ) {
            if ( ! cv->b.sc->layers[layer].order2 ) {
                mi[i].mid = LMID_Quadratic;
                mi[i].ti.text = (unichar_t *) _("Make Quadratic");
            }

        } else if ( mids[i]==LMID_ShowCubic ) {
            mi[i].ti.checkable = 1;
            mi[i].ti.checked = (layerscols & LSHOW_CUBIC)?1:0;

        } else if ( mids[i]==LMID_ShowFore ) {
            mi[i].ti.checkable = 1;
            mi[i].ti.checked = (layerscols & LSHOW_FG)?1:0;
        }
    }
    GMenuCreatePopupMenu(cvlayers, event, mi);
}

/* Scan for which layer and column one clicks on in the layers 1 palette                */
/* -1 is the guides layer, 0 is default back, 1 default fore, etc.                      */
/* col will be set to either -1 for none, CID_VBase, CID_QBase, CID_FBase, or CID_EBase */
static int CVLScanForItem(int x, int y, int *col) {
    int l=(y-layer_header_height)/layer_height + layerinfo.offtop - 1;
    int viscol=0, quadcol, fgcol, editcol;
    int cw=layerinfo.column_width;

    quadcol=fgcol=viscol;
    if ( layerscols & LSHOW_CUBIC ) { quadcol = viscol+cw; fgcol=viscol+cw; }
    if ( layerscols & LSHOW_FG    ) { fgcol = quadcol+cw; }
    editcol=fgcol+cw;

    *col=-1;
    if ( x>0 && x<viscol+cw ) *col=CID_VBase;
    /**
     * The two below options, CID_QBase and CID_FBase allow the curve
     * type and foreground/background to be changed simply by clicking
     * on them. The cubic/quadratic and background/foreground
     * attributes should NOT be buttons that can change these
     * attributes, they should only SHOW the attribute. Changing the
     * attributes can be done in Font Info, Layers, and is done
     * infrequently and has a lot of implications so shouldn't be
     * easily done by mistake.
     */
//    else if ( (layerscols & LSHOW_CUBIC) && x>=quadcol && x<quadcol+cw ) *col=CID_QBase;
//    else if ( (layerscols & LSHOW_FG) && x>=fgcol && x<fgcol+cw ) *col=CID_FBase;
    else if ( x>=editcol ) *col=CID_EBase;

    return l;
}

/* Called in response to some event where we want to change the current layer. */
void CVLSelectLayer(CharView *cv, int layer) {
    enum drawmode dm = cv->b.drawmode;

    if ( layer<-1 || layer>=cv->b.sc->layer_cnt )
	return;

    if ( layer==-1 ) {
        cv->b.drawmode = dm_grid;
        cv->lastselpt = NULL;
    } else {
        if ( layer==1 ) {
            cv->b.drawmode = dm_fore;
            cv->lastselpt = NULL;
        } else {
            cv->b.drawmode = dm_back;
            cv->b.layerheads[dm_back] = &cv->b.sc->layers[layer];
            cv->lastselpt = NULL;
        }

        CVDebugFree(cv->dv);
        SplinePointListsFree(cv->b.gridfit); cv->b.gridfit = NULL;
        FreeType_FreeRaster(cv->oldraster); cv->oldraster = NULL;
        FreeType_FreeRaster(cv->raster); cv->raster = NULL;
        cv->show_ft_results = false;
    }
    layerinfo.active = CVLayer(&cv->b); /* the index of the active layer */

    CVRegenFill(cv);
    GDrawRequestExpose(cv->v,NULL,false);
    if (cvlayers2) GDrawRequestExpose(cvlayers2,NULL,false);
    if (cvlayers)  GDrawRequestExpose(cvlayers,NULL,false);
    if ( dm!=cv->b.drawmode )
        GDrawRequestExpose(cv->gw,NULL,false); /* the logo (where the scrollbars join) shows what layer we are in */
    CVInfoDrawText(cv,cv->gw);
}

static int cvlayers_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);
    char *buts[3];
    buts[0] = _("_Yes"); buts[1]=_("_No"); buts[2] = NULL;

    if ( event->type==et_destroy && cvlayers == gw )
    {
	cvlayers = NULL;
	return( true );
    }

    if ( cv==NULL )
	return( true );

    switch ( event->type ) {
      case et_close:
	GDrawSetVisible(gw,false);
      break;
      case et_char: case et_charup:
	if ( event->u.chr.keysym == GK_Return) {
	    CVLRemoveEdit(cv, true);
	} else if ( event->u.chr.keysym == GK_Escape) {
	    CVLRemoveEdit(cv, false);
	} else PostCharToWindow(cv->gw,event);
      break;
      case et_mousemove: {
        int l, col;

        l = CVLScanForItem(event->u.mouse.x,event->u.mouse.y, &col);
        if ( l!=layerinfo.mo_layer || col!=layerinfo.mo_col ) {
            layerinfo.mo_layer = l;
            layerinfo.mo_col = col;
            GDrawRequestExpose(cvlayers,NULL,false);
        }

return( true );

      } break;
      case et_mousedown: {
        if ( layerinfo.rename_active ) CVLRemoveEdit(cv, true);
      } break;
      case et_mouseup: {
        int l, x, cid, h;
        GGadget *g;
        l = CVLScanForItem(event->u.mouse.x,event->u.mouse.y, &cid);

        if ( cid==CID_EBase && l>=-1 && l<cv->b.sc->layer_cnt )
	{
	    /* Need to check for this BEFORE checking for right click! */
            if ( event->u.mouse.button==1 && event->u.mouse.clicks==2 )
	    {
		 /* bring up edit box for layer name */

                if ( l<2 )
		    return ( true );

                x = 7+(1+((layerscols&LSHOW_CUBIC)?1:0)+((layerscols&LSHOW_FG)?1:0))*layerinfo.column_width;
                g = GWidgetGetControl(cvlayers,CID_Edit);
		h = 1.5*layer_height;

                GGadgetResize(g, layerinfo.sb_start-x, h);
                GGadgetMove(g, x,layer_header_height+(l+1.5+layerinfo.offtop)*layer_height-h/2);
                GGadgetSetVisible(g,true);
                /* GGadgetSetTitle8((GTextField*)g, cv->b.sc->parent->layers[l].name); */
                GGadgetSetTitle8(g, cv->b.sc->parent->layers[l].name);

		layerinfo.active=l;
                layerinfo.rename_active=1;
                return ( true );
            }

            CVLSelectLayer(cv, l);
        }

         /* right click to pop up menu */
	if ( event->u.mouse.button==3 ) {
	    LayerMenu(cv,event,true);
	    return(true);
        }

         /* otherwise, deal with clicking up on the various controls */
        if ( l<-1 || l>=cv->b.sc->layer_cnt)
	    return (true);

        if ( cid==CID_QBase) {
            if (l>=0) { /* don't try to adjust if calling for guides layer */
                if (cv->b.sc->layers[l].order2)
                    SFConvertLayerToOrder3(cv->b.sc->parent, l);
                else
                    SFConvertLayerToOrder2(cv->b.sc->parent, l);
                cv->back_img_out_of_date = true;
	        GDrawRequestExpose(cvlayers,NULL,false);
                GDrawRequestExpose(cv->v,NULL,false);
            }
        } else if ( cid==CID_FBase) {
            if (l>1) { /* don't try to adjust if calling guides, default fore or back layer */
                if (cv->b.sc->layers[l].background)
                    SFLayerSetBackground(cv->b.sc->parent,l,0);
                else
                    SFLayerSetBackground(cv->b.sc->parent,l,1);
	        GDrawRequestExpose(cvlayers,NULL,false);
                GDrawRequestExpose(cv->v,NULL,false);
            }
        }
      } break; /* case et_mouseup */
      case et_expose:
	LayersExpose(cv,gw,event);
      break;
      case et_resize:
        if ( event->u.resize.sized ) {
            CVLCheckLayerCount(cv,false); /* update widget existence, but do not resize */
        }
      break;
      case et_controlevent:
	if ( event->u.control.subtype == et_buttonactivate ) {
	    int cid = GGadgetGetCid(event->u.control.g);
	    int layer;

	    switch( cid ) {
	      case CID_AddLayer: {
                SplineChar *sc = cv->b.sc;

		 /* This adds a new layer to the end of the current layers list.
		  * Somehow it is created as an invisible layer. */
		SFAddLayer(cv->b.sc->parent, /* font of the glyph in the charview */
				UniqueLayerName(sc,_("Back")),     /* Name */
				0,         /* 0=cubic, 1=quad */
				1);       /* 1=back,  0=fore */

		layer=cv->b.sc->parent->layer_cnt-1;
		
		cv->showback[layer>>5] |=  (1<<(layer&31)); /* make it visible */
		CVLCheckLayerCount(cv,true); /* update widget existence */
		CVLayersSet(cv);       /* update widget state     */
              } break;
	      case CID_RemoveLayer:
		layer = CVLayer((CharViewBase *) cv); /* the index of the active layer */
                if (layer==ly_fore || layer==ly_back || layer==ly_grid)
return ( true );
	        if ( gwwv_ask(_("Cannot Be Undone"),(const char **) buts,0,1,_("This operation cannot be undone, do it anyway?"))==1 )
return ( true );
                SFRemoveLayer(cv->b.sc->parent, layer);
                CVLCheckLayerCount(cv,true); /* update widget existence */
                CVLayersSet(cv);       /* update widget state     */
	      break;
	      case CID_RenameLayer: {
		/* *** */
                int x = 7+(1+((layerscols&LSHOW_CUBIC)?1:0)+((layerscols&LSHOW_FG)?1:0))*layerinfo.column_width;
                GGadget *g = GWidgetGetControl(cvlayers,CID_Edit);
		layer = CVLayer((CharViewBase *) cv); /* the index of the active layer */
		/* layer = layerinfo.active */ /* the index of the active layer */

                GGadgetResize(g, layerinfo.sb_start-x,1.5*layer_height);
                GGadgetMove(g, x,layer_header_height+(layer+1+layerinfo.offtop)*layer_height);
                GGadgetSetVisible(g,true);
                GGadgetSetTitle8(g, cv->b.sc->parent->layers[layer].name);

                layerinfo.rename_active=1;

		CVLCheckLayerCount(cv,true); /* update widget existence */
		CVLayersSet(cv);       /* update widget state     */
		GDrawRequestExpose(cvtools,NULL,false);
		
              } break;
	    }
        } else if ( event->u.control.subtype == et_radiochanged ) {
	    enum drawmode dm = cv->b.drawmode;
	    int cid = GGadgetGetCid(event->u.control.g);

	    switch( cid ) {
	      case CID_VFore:
		CVShows.showfore = cv->showfore = GGadgetIsChecked(event->u.control.g);
                GDrawRequestExpose(cv->v,NULL,false);
	      break;
	      case CID_VBack:
		CVShows.showback = GGadgetIsChecked(event->u.control.g);
		if ( CVShows.showback )
		    cv->showback[0] |= 1;
		else
		    cv->showback[0] &= ~1;
		cv->back_img_out_of_date = true;
                GDrawRequestExpose(cv->v,NULL,false);
	      break;
	      case CID_VGrid:
		CVShows.showgrids = cv->showgrids = GGadgetIsChecked(event->u.control.g);
                GDrawRequestExpose(cv->v,NULL,false);
	      break;
	      case CID_EFore:
		cv->b.drawmode = dm_fore;
		cv->lastselpt = NULL;

		CVDebugFree(cv->dv);
		SplinePointListsFree(cv->b.gridfit); cv->b.gridfit = NULL;
		FreeType_FreeRaster(cv->oldraster); cv->oldraster = NULL;
		FreeType_FreeRaster(cv->raster); cv->raster = NULL;
		cv->show_ft_results = false;
	      break;
	      case CID_EBack:
		cv->b.drawmode = dm_back;
		cv->b.layerheads[dm_back] = &cv->b.sc->layers[ly_back];
		cv->lastselpt = NULL;

		CVDebugFree(cv->dv);
		SplinePointListsFree(cv->b.gridfit); cv->b.gridfit = NULL;
		FreeType_FreeRaster(cv->oldraster); cv->oldraster = NULL;
		FreeType_FreeRaster(cv->raster); cv->raster = NULL;
		cv->show_ft_results = false;
	      break;
	      case CID_EGrid:
		cv->b.drawmode = dm_grid;
		cv->lastselpt = NULL;
	      break;
	      default:
                if ( cid>=CID_VBase-1 && cid<CID_VBase+999) {
                    cid -= CID_VBase;
                    if ( GGadgetIsChecked(event->u.control.g))
                        cv->showback[cid>>5] |=  (1<<(cid&31));
                    else
                        cv->showback[cid>>5] &= ~(1<<(cid&31));
                    cv->back_img_out_of_date = true;

                    GDrawRequestExpose(cv->v,NULL,false);
                    if ( dm!=cv->b.drawmode )
                        GDrawRequestExpose(cv->gw,NULL,false);	/* the logo (where the scrollbars join) shows what layer we are in */
                }
            }

        } else if ( event->u.control.subtype == et_scrollbarchange ) {
	    LayerScroll(cv,event);

	}
      break; /* case et_controlevent */
      default: {
      } break;
    } /* switch ( event->type ) */
return( true );
}

/* Set to true the editable field for the current layer, and false for the other layers. */
void CVSetLayer(CharView *cv,int layer) {

     /* Update the drawmode of cv */
    if ( layer == ly_grid )
	cv->b.drawmode = dm_grid;
    else if (layer == ly_fore )
	cv->b.drawmode = dm_fore;
    else {
	cv->b.drawmode = dm_back;
	cv->b.layerheads[dm_back] = &cv->b.sc->layers[layer];
    }
    if ( cvlayers!=NULL && GDrawGetUserData(cvlayers)==cv ) 
	GDrawRequestExpose(cvlayers,NULL,false);
}

/* Check if a key press corresponds to a mnemonic the palette knows about. */
int CVPaletteMnemonicCheck(GEvent *event) {
    static struct strmatch { char *str; int cid; } strmatch[] = {
/* GT: Foreground, make it short */
	{ N_("F_ore"), CID_EFore },
/* GT: Background, make it short */
	{ N_("_Back"), CID_EBack },
/* GT: Guide layer, make it short */
	{ N_("_Guide"), CID_EGrid },
	{ NULL, 0 }
    };
    unichar_t mn, mnc;
    int j, i, ch;
    char *foo;
    GEvent fake;
    GGadget *g;
    CharView *cv;
    SplineFont *parent;
    int curlayer;

    if ( cvtools==NULL )
return( false );
    cv = GDrawGetUserData(cvtools);
    parent = cv->b.sc->parent;
    curlayer = CVLayer(&cv->b); /* the index of the active layer */

    if ( isdigit(event->u.chr.keysym) ) {
	int off = event->u.chr.keysym - '0';

	g = GWidgetGetControl(cvlayers, CID_EBase+off-1);
	if ( off-1<parent->layer_cnt && off!=curlayer ) {
            CVLSelectLayer(cv, off);
	    if ( cv->b.sc->parent->multilayer )
	    	GDrawRequestExpose(cvlayers2,NULL,false);
	    else
return( true );
	}
    }

     /* mnemonic is encoded in the layer name */
    for ( j=0; j<2; ++j ) {
	for ( i=0; j==0 ? i<parent->layer_cnt : strmatch[i].str!=NULL; ++i ) {
	    for ( foo = j==0 ? parent->layers[i].name : _(strmatch[i].str);
		    (ch=utf8_ildb((const char **) &foo))!=0; )
		if ( ch=='_' )
	    break;
	    if ( ch=='_' )
		mnc = utf8_ildb((const char **) &foo);
	    else
		mnc = 0;
	    mn = mnc;
	    if ( islower(mn)) mnc = toupper(mn);
	    else if ( isupper(mn)) mnc = tolower(mn);
	    if ( event->u.chr.chars[0]==mn || event->u.chr.chars[0]==mnc ) {
		if ( cv->b.sc->parent->multilayer ) {
		    fake.type = et_mousedown;
		    fake.w = cvlayers;
		    fake.u.mouse.x = 40;
		    if ( strmatch[i].cid==CID_EGrid ) {
			fake.u.mouse.y = layer2.header_height+12;
		    } else if ( strmatch[i].cid==CID_EBack ) {
			fake.u.mouse.y = layer2.header_height+12+CV_LAYERS2_LINE_HEIGHT;
		    } else {
			fake.u.mouse.y = layer2.header_height+12+2*CV_LAYERS2_LINE_HEIGHT;
		    }
		    cvlayers2_e_h(cvlayers2,&fake);
		} else {
            	    CVLSelectLayer(cv, i);
	    	    GDrawRequestExpose(cvlayers,NULL,false);
		}
    return( true );
	    }
	}
    }
return( false );
}

/* This is used for fonts other than Type 3 fonts. CVMakeLayers2() is used for Type 3.
 * Only the basics of the palette are set up here, with the widgets for the default fore, back,
 * and guides layers. The palette is updated to actual character views in CVLCheckLayerCount(). */
GWindow CVMakeLayers(CharView *cv) {
    GRect r,size;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[25];
    GTextInfo label[25];
    GGadget *gadget;
    FontRequest rq;
    extern int _GScrollBar_Width;
    int i=0;
    int viscol=0;

    if ( cvlayers!=NULL )
return( cvlayers );

     /* Initialize layerinfo */
    if ( layerinfo.clut==NULL )
	layerinfo.clut = _BDFClut(4);
    if ( layersfont==NULL ) {
	memset(&rq,'\0',sizeof(rq));
	rq.utf8_family_name = SANS_UI_FAMILIES;
	rq.point_size = -12;
	rq.weight = 400;
	layersfont = GDrawInstanciateFont(cvlayers2,&rq);
	layersfont = GResourceFindFont("LayersPalette.Font",layersfont);
    }
    layerinfo.font = layersfont;

     /* Initialize palette window */
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_positioned|wam_isdlg;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.positioned = true;
    wattrs.is_dlg = true;
    wattrs.utf8_window_title = _("Layers");

    r.width = GGadgetScale(104); r.height = CV_LAYERS_HEIGHT;
    if ( cvlayersoff.x==-9999 ) {
	 /* Offset of window on screen, by default make it sit just below the tools palette */
	cvlayersoff.x = -r.width-6;
	cvlayersoff.y = cv->mbh+getToolbarHeight(cv)+45/*25*/; /* 45 is right if there's decor, 25 when none. twm gives none, kde gives decor */
    }
    r.x = cvlayersoff.x; r.y = cvlayersoff.y;
    if ( palettes_docked ) { r.x = 0; r.y=getToolbarHeight(cv)+2; }
    cvlayers = CreatePalette( cv->gw, &r, cvlayers_e_h, NULL, &wattrs, cv->v );

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    int32 plusw = GDrawGetText8Width(cv->gw,  _("+"), -1);
    int32 plush = GDrawGetText8Height(cv->gw, _("+"), -1);
    plusw = GDrawPointsToPixels(NULL,plusw+4);
    plush = GDrawPointsToPixels(NULL,plush+4);
    plush = MAX( plush, plusw ); // make it square.
    
     /* Remove Layer button */
    label[0].text = (unichar_t *) _("-");
    label[0].text_is_1byte = true;
    gcd[i].gd.label = &label[0];
    gcd[i].gd.pos.x = 7; gcd[i].gd.pos.y = 5; 
    gcd[i].gd.pos.width  = plusw; gcd[i].gd.pos.height = plush;
    gcd[i].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels|gg_utf8_popup;
    gcd[i].gd.cid = CID_RemoveLayer;
    gcd[i].gd.popup_msg = (unichar_t *) _("Delete the current layer");
    gcd[i].creator = GButtonCreate;
    ++i;

     /* Add Layer button */
    label[1].text = (unichar_t *) _("+");
    label[1].text_is_1byte = true;
    gcd[i].gd.label = &label[1];
    gcd[i].gd.pos.x = 30; gcd[i].gd.pos.y = 5; 
    gcd[i].gd.pos.width  = plusw; gcd[i].gd.pos.height = plush;
    gcd[i].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels|gg_utf8_popup;
    gcd[i].gd.cid = CID_AddLayer;
    gcd[i].gd.popup_msg = (unichar_t *) _("Add a new layer");
    gcd[i].creator = GButtonCreate;
    ++i;

     /* "Layers" label next to the add and remove buttons */
    label[2].text = (unichar_t *) "";
    label[2].text_is_1byte = true;
    gcd[i].gd.label = &label[2];
    gcd[i].gd.pos.x = 47; gcd[i].gd.pos.y = 5; 
    gcd[i].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels|gg_utf8_popup;
    gcd[i].gd.cid = CID_LayersMenu;
    /* gcd[i].gd.popup_msg = (unichar_t *) _("Rename the current layer"); */
    gcd[i].creator = GLabelCreate;
    ++i;

     /* Default visibility toggles for Fore, Back, and Guides */
    gcd[i].gd.pos.x = viscol; gcd[i].gd.pos.y = 38; 
    gcd[i].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    if ( cv->showgrids ) gcd[i].gd.flags |= gg_cb_on;
    gcd[i].gd.cid = CID_VGrid;
    gcd[i].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[i].creator = GVisibilityBoxCreate;
    ++i;

    gcd[i].gd.pos.x = viscol; gcd[i].gd.pos.y = 38; 
    gcd[i].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    if ( cv->showback[0]&1 ) gcd[i].gd.flags |= gg_cb_on;
    gcd[i].gd.cid = CID_VBack;
    gcd[i].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[i].creator = GVisibilityBoxCreate;
    ++i;

    gcd[i].gd.pos.x = viscol; gcd[i].gd.pos.y = 21; 
    gcd[i].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    if ( cv->showfore ) gcd[i].gd.flags |= gg_cb_on;
    gcd[i].gd.cid = CID_VFore;
    gcd[i].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[i].creator = GVisibilityBoxCreate;
    ++i;


     /* Scroll bar */
    gcd[i].gd.pos.width = GDrawPointsToPixels(cv->gw,_GScrollBar_Width);
    gcd[i].gd.pos.x = 0; /* <- these get updated to real values later */
    gcd[i].gd.pos.y = 0;
    gcd[i].gd.pos.height = 50;
    gcd[i].gd.flags = gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    gcd[i].gd.cid = CID_SB;
    gcd[i].creator = GScrollBarCreate;
    layerinfo.sb_start = gcd[i].gd.pos.x;
    ++i;

     /* Edit box for in place layer rename */
    gcd[i].gd.pos.width=gcd[i].gd.pos.height=1;
    gcd[i].gd.flags = gg_enabled|gg_pos_in_pixels|gg_utf8_popup;
    gcd[i].gd.cid = CID_Edit;
    gcd[i].gd.popup_msg = (unichar_t *) _("Type in new layer name");
    gcd[i].creator = GTextFieldCreate;
    ++i;

    GGadgetsCreate(cvlayers,gcd);
    if ( cvvisible[0] )
	GDrawSetVisible(cvlayers,true);
    layers_max=2;

    gadget=GWidgetGetControl(cvlayers,CID_AddLayer);
    GGadgetGetSize(gadget,&size);
    layer_header_height = 0;
    layer_footer_height = size.y + size.height;

    GGadgetGetSize(GWidgetGetControl(cvlayers,CID_VGrid),&size);
    layer_height = size.height;
    int32 w = GDrawGetText8Width(cvlayers, "W", -1);
    layerinfo.column_width = w+6;

    layerinfo.active = CVLayer(&cv->b); /* the index of the active layer */
    layerinfo.mo_col   = -2; /* -2 forces this variable to be updated. afterwords it will be -1 for nothing, or >=0 */
    layerinfo.mo_layer = -2;
    layerinfo.offtop   = 0;
    layerinfo.rename_active = 0;

    GVisibilityBoxSetToMinWH(GWidgetGetControl(cvlayers,CID_VGrid));
    GVisibilityBoxSetToMinWH(GWidgetGetControl(cvlayers,CID_VBack));
    GVisibilityBoxSetToMinWH(GWidgetGetControl(cvlayers,CID_VFore));

return( cvlayers );
}


/* ***************** CVTools and other common palette functions follow ************ */

static void CVPopupInvoked(GWindow v, GMenuItem *mi, GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(v);
    int pos;

    pos = mi->mid;
    if ( pos==cvt_spiro ) {
	CVChangeSpiroMode(cv);
    } else if ( cv->had_control ) {
	if ( cv->cb1_tool!=pos ) {
	    cv->cb1_tool = cv_cb1_tool = pos;
	    GDrawRequestExpose(cvtools,NULL,false);
	}
    } else {
	if ( cv->b1_tool!=pos ) {
	    cv->b1_tool = cv_b1_tool = pos;
	    GDrawRequestExpose(cvtools,NULL,false);
	}
    }
    CVToolsSetCursor(cv,cv->had_control?ksm_control:0,NULL);
}

static void CVPopupLayerInvoked(GWindow v, GMenuItem *mi, GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(v);
    int layer = mi->mid==0 ? 1 : mi->mid==1 ? 0 : -1;

    if ( layerinfo.active!=layer )
        CVLSelectLayer(cv, layer);
}

static void CVPopupSelectInvoked(GWindow v, GMenuItem *mi, GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(v);

    switch ( mi->mid ) {
      case 0:
	CVPGetInfo(cv);
      break;
      case 1:
	if ( cv->p.ref!=NULL )
	    CharViewCreate(cv->p.ref->sc,(FontView *) (cv->b.fv),-1);
      break;
      case 2:
	CVAddAnchor(cv);
      break;
      case 3:
	CVMakeClipPath(cv);
      break;
    case MIDL_MakeLine: {
	_CVMenuMakeLine((CharViewBase *) cv,mi->mid==MIDL_MakeArc, e!=NULL && (e->u.mouse.state&ksm_meta));
	break;
    }
    case MIDL_MakeArc: {
	_CVMenuMakeLine((CharViewBase *) cv,mi->mid==MIDL_MakeArc, e!=NULL && (e->u.mouse.state&ksm_meta));
	break;
    }
    case MIDL_InsertPtOnSplineAt: {
	_CVMenuInsertPt( cv );
	break;
    }
    case MIDL_NamePoint: {
	if ( cv->p.sp )
	    _CVMenuNamePoint( cv, cv->p.sp );
	break;
    }
    case MIDL_NameContour: {
	_CVMenuNameContour( cv );
	break;
    }
      
    }
}

void CVToolsPopup(CharView *cv, GEvent *event) {
    GMenuItem mi[125];
    int i=0;
    int j=0;
    int anysel=0;
    static char *selectables[] = { N_("Get Info..."), N_("Open Reference"), N_("Add Anchor"), NULL };

    memset(mi,'\0',sizeof(mi));
    anysel = CVTestSelectFromEvent(cv,event);

    if( anysel )
    {
	    mi[i].ti.text = (unichar_t *) _("Curve");
	    mi[i].ti.text_is_1byte = true;
	    mi[i].ti.fg = COLOR_DEFAULT;
	    mi[i].ti.bg = COLOR_DEFAULT;
	    mi[i].mid = MID_Curve;
	    mi[i].invoke = CVMenuPointType;
	    i++;
	    mi[i].ti.text = (unichar_t *) _("HVCurve");
	    mi[i].ti.text_is_1byte = true;
	    mi[i].ti.fg = COLOR_DEFAULT;
	    mi[i].ti.bg = COLOR_DEFAULT;
	    mi[i].mid = MID_HVCurve;
	    mi[i].invoke = CVMenuPointType;
	    i++;
	    mi[i].ti.text = (unichar_t *) _("Corner");
	    mi[i].ti.text_is_1byte = true;
	    mi[i].ti.fg = COLOR_DEFAULT;
	    mi[i].ti.bg = COLOR_DEFAULT;
	    mi[i].mid = MID_Corner;
	    mi[i].invoke = CVMenuPointType;
	    i++;
	    mi[i].ti.text = (unichar_t *) _("Tangent");
	    mi[i].ti.text_is_1byte = true;
	    mi[i].ti.fg = COLOR_DEFAULT;
	    mi[i].ti.bg = COLOR_DEFAULT;
	    mi[i].mid = MID_Tangent;
	    mi[i].invoke = CVMenuPointType;
	    i++;

	    mi[i].ti.line = true;
	    mi[i].ti.fg = COLOR_DEFAULT;
	    mi[i].ti.bg = COLOR_DEFAULT;
	    i++;

	    mi[i].ti.text = (unichar_t *) _("Merge");
	    mi[i].ti.text_is_1byte = true;
	    mi[i].ti.fg = COLOR_DEFAULT;
	    mi[i].ti.bg = COLOR_DEFAULT;
	    mi[i].mid = MID_Merge;
	    mi[i].invoke = CVMerge;
	    i++;
	    mi[i].ti.text = (unichar_t *) _("Merge to Line");
	    mi[i].ti.text_is_1byte = true;
	    mi[i].ti.fg = COLOR_DEFAULT;
	    mi[i].ti.bg = COLOR_DEFAULT;
	    mi[i].mid = MID_MergeToLine;
	    mi[i].invoke = CVMergeToLine;
	    i++;

	    mi[i].ti.text = (unichar_t *) _("Align Points");
	    mi[i].ti.text_is_1byte = true;
	    mi[i].ti.fg = COLOR_DEFAULT;
	    mi[i].ti.bg = COLOR_DEFAULT;
	    mi[i].mid = MID_Average;
	    mi[i].invoke = CVMenuConstrain;
	    i++;
        
    }
    
    
    if( !anysel )
    {
	for ( i=0;i<=cvt_skew; ++i ) {
	    char *msg = _(popupsres[i]);
	    if ( cv->b.sc->inspiro && hasspiro()) {
		if ( i==cvt_spirog2 )
		    msg = _("Add a g2 curve point");
		else if ( i==cvt_spiroleft )
		    msg = _("Add a left \"tangent\" point");
		else if ( i==cvt_spiroright )
		    msg = _("Add a right \"tangent\" point");
	    }
	    mi[i].ti.text = (unichar_t *) msg;
	    mi[i].ti.text_is_1byte = true;
	    mi[i].ti.fg = COLOR_DEFAULT;
	    mi[i].ti.bg = COLOR_DEFAULT;
	    mi[i].mid = i;
	    mi[i].invoke = CVPopupInvoked;
	}
    }
    
    if( !anysel )
    {
	if ( cvlayers!=NULL && !cv->b.sc->parent->multilayer ) {
	    mi[i].ti.line = true;
	    mi[i].ti.fg = COLOR_DEFAULT;
	    mi[i++].ti.bg = COLOR_DEFAULT;
	    for ( j=0;j<3; ++j, ++i ) {
		mi[i].ti.text = (unichar_t *) _(editablelayers[j]);
		mi[i].ti.text_in_resource = true;
		mi[i].ti.text_is_1byte = true;
		mi[i].ti.fg = COLOR_DEFAULT;
		mi[i].ti.bg = COLOR_DEFAULT;
		mi[i].mid = j;
		mi[i].invoke = CVPopupLayerInvoked;
	    }
	}
    }

    if( i > 0 ) {
	mi[i].ti.line = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i++].ti.bg = COLOR_DEFAULT;
    }
    
    for ( j=0; selectables[j]!=0; ++j )
    {
	if ( (!anysel && j!=2 ) ||
		( j==0 && cv->p.spline ) ||
		( j==1 && !cv->p.ref ))
	{
	    // don't show them a disabled item
	    continue;
	    
	    // or, if the above "continue;" is commented then keep the entry
	    // but don't let them select it 
	    mi[i].ti.disabled = true; 
	}
	mi[i].ti.text = (unichar_t *) _(selectables[j]);
	mi[i].ti.text_is_1byte = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = j;
	mi[i].invoke = CVPopupSelectInvoked;
	i++;
    }

    if ( anysel ) {
	mi[i].ti.text = (unichar_t *)_("Name Point...");
	mi[i].ti.text_is_1byte = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = MIDL_NamePoint;
	mi[i].invoke = CVPopupSelectInvoked;
	i++;
    }

    if ( cv->b.sc->parent->multilayer ) {
	mi[i].ti.text = (unichar_t *) _("Make Clip Path");
	mi[i].ti.text_is_1byte = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = j;
	mi[i].invoke = CVPopupSelectInvoked;
	i++; 
    }

    int cnt = CVCountSelectedPoints(cv);
//    printf(".... count:%d\n", cnt );
    if( cnt > 1 ) {
	mi[i].ti.text = (unichar_t *) _("Make Line");
	mi[i].ti.text_is_1byte = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = MIDL_MakeLine;
	mi[i].invoke = CVPopupSelectInvoked;
	i++;

	mi[i].ti.text = (unichar_t *) _("Make Arc");
	mi[i].ti.text_is_1byte = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = MIDL_MakeArc;
	mi[i].invoke = CVPopupSelectInvoked;
	i++;

	mi[i].ti.text = (unichar_t *) _("Insert Point On Spline At...");
	mi[i].ti.text_is_1byte = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = MIDL_InsertPtOnSplineAt;
	mi[i].invoke = CVPopupSelectInvoked;
	i++;

	mi[i].ti.text = (unichar_t *) _("Name Point");
	mi[i].ti.text_is_1byte = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = MIDL_NamePoint;
	mi[i].invoke = CVPopupSelectInvoked;
	i++;

	mi[i].ti.text = (unichar_t *) _("Name Contour");
	mi[i].ti.text_is_1byte = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = MIDL_NameContour;
	mi[i].invoke = CVPopupSelectInvoked;
	i++;
    }

    cv->had_control = (event->u.mouse.state&ksm_control)?1:0;
    GMenuCreatePopupMenuWithName(cv->v,event, "Popup", mi);
}

static void CVPaletteCheck(CharView *cv) {
    if ( cvtools==NULL ) {
	if ( palettes_fixed ) {
	    cvtoolsoff.x = 0; cvtoolsoff.y = 0;
	}
	CVMakeTools(cv);
    }
    if ( cv->b.sc->parent->multilayer && cvlayers2==NULL ) {
	if ( palettes_fixed ) {
	    cvlayersoff.x = 0; cvlayersoff.y = getToolbarHeight(cv)+45/*25*/;	/* 45 is right if there's decor, 25 when none. twm gives none, kde gives decor */
	}
	CVMakeLayers2(cv);
    } else if ( !cv->b.sc->parent->multilayer && cvlayers==NULL ) {
	if ( palettes_fixed ) {
	    cvlayersoff.x = 0; cvlayersoff.y = getToolbarHeight(cv)+45/*25*/;	/* 45 is right if there's decor, 25 when none. twm gives none, kde gives decor */
	}
	CVMakeLayers(cv);
    }
}

int CVPaletteIsVisible(CharView *cv,int which) {
    CVPaletteCheck(cv);
    if ( which==1 )
return( cvtools!=NULL && GDrawIsVisible(cvtools) );

    if ( cv->b.sc->parent->multilayer )
return( cvlayers2!=NULL && GDrawIsVisible(cvlayers2));

return( cvlayers!=NULL && GDrawIsVisible(cvlayers) );
}

void CVPaletteSetVisible(CharView *cv,int which,int visible) {
    CVPaletteCheck(cv);
    if ( which==1 && cvtools!=NULL)
	GDrawSetVisible(cvtools,visible );
    else if ( which==0 && cv->b.sc->parent->multilayer && cvlayers2!=NULL )
	GDrawSetVisible(cvlayers2,visible );
    else if ( which==0 && cvlayers!=NULL )
	GDrawSetVisible(cvlayers,visible );
    cvvisible[which] = visible;
    SavePrefs(true);
}

void CVPalettesRaise(CharView *cv) {
    if ( cvtools!=NULL && GDrawIsVisible(cvtools))
	GDrawRaise(cvtools);
    if ( cvlayers!=NULL && GDrawIsVisible(cvlayers))
	GDrawRaise(cvlayers);
    if ( cvlayers2!=NULL && GDrawIsVisible(cvlayers2))
	GDrawRaise(cvlayers2);
}

void _CVPaletteActivate(CharView *cv, int force, int docking_changed) {
    CharView *old;

    CVPaletteCheck(cv);
    if ( layers2_active!=-1 && layers2_active!=cv->b.sc->parent->multilayer ) {
	if ( !cvvisible[0] ) {
	    if ( cvlayers2!=NULL ) GDrawSetVisible(cvlayers2,false);
	    if ( cvlayers !=NULL ) GDrawSetVisible(cvlayers,false);
	} else if ( layers2_active && cvlayers!=NULL ) {
	    if ( cvlayers2!=NULL ) GDrawSetVisible(cvlayers2,false);
	    GDrawSetVisible(cvlayers,true);
	} else if ( !layers2_active && cvlayers2!=NULL ) {
	    if ( cvlayers !=NULL ) GDrawSetVisible(cvlayers,false);
	    GDrawSetVisible(cvlayers2,true);
	}
    }
    layers2_active = cv->b.sc->parent->multilayer;
    if ( (old = GDrawGetUserData(cvtools))!=cv || force) {
	if ( old!=NULL ) {
	    SaveOffsets(old->gw,cvtools,&cvtoolsoff);
	    if ( old->b.sc->parent->multilayer )
		SaveOffsets(old->gw,cvlayers2,&cvlayersoff);
	    else
		SaveOffsets(old->gw,cvlayers,&cvlayersoff);
	}

    if (palettes_docked || docking_changed) {
        // When docked, we need to recreate the palettes
        // Because they were created as a child of the old charview
        GDrawDestroyWindow(cvtools);

        if (cvlayers != NULL)
            GDrawDestroyWindow(cvlayers);
        if (cvlayers2 != NULL)
            GDrawDestroyWindow(cvlayers2);

        cvtools = cvlayers = cvlayers2 = NULL;
        CVPaletteCheck(cv);
    }

    GDrawSetUserData(cvtools,cv);

	if ( cv->b.sc->parent->multilayer ) {
	    LayersSwitch(cv);
	    GDrawSetUserData(cvlayers2,cv);
	} else {
	    GDrawSetUserData(cvlayers,cv);
            CVLCheckLayerCount(cv,true);
	}
    if (palettes_docked) {
        if (cvvisible[1])
            GDrawRequestExpose(cvtools, NULL, false);
        if (cvvisible[0]) {
            if (cv->b.sc->parent->multilayer)
                GDrawRequestExpose(cvlayers2, NULL, false);
            else
                GDrawRequestExpose(cvlayers, NULL, false);
        }
    } else {
	    if ( cvvisible[0]) {
		if ( cv->b.sc->parent->multilayer )
		    RestoreOffsets(cv->gw,cvlayers2,&cvlayersoff);
		else
		    RestoreOffsets(cv->gw,cvlayers,&cvlayersoff);
	    }
	    if ( cvvisible[1])
		RestoreOffsets(cv->gw,cvtools,&cvtoolsoff);
	}
	GDrawSetVisible(cvtools,cvvisible[1]);
	if ( cv->b.sc->parent->multilayer )
	    GDrawSetVisible(cvlayers2,cvvisible[0]);
	else
	    GDrawSetVisible(cvlayers,cvvisible[0]);
	if ( cvvisible[1]) {
	    cv->showing_tool = cvt_none;
	    CVToolsSetCursor(cv,0,NULL);
	    GDrawRequestExpose(cvtools,NULL,false);
	}
	if ( cvvisible[0])
	    CVLayersSet(cv);
    }
    if ( bvtools!=NULL ) {
	BitmapView *bv = GDrawGetUserData(bvtools);
	if ( bv!=NULL ) {
	    SaveOffsets(bv->gw,bvtools,&bvtoolsoff);
	    SaveOffsets(bv->gw,bvlayers,&bvlayersoff);
	    if ( !bv->shades_hidden )
		SaveOffsets(bv->gw,bvshades,&bvshadesoff);
	    GDrawSetUserData(bvtools,NULL);
	    GDrawSetUserData(bvlayers,NULL);
	    GDrawSetUserData(bvshades,NULL);
	}
	GDrawSetVisible(bvtools,false);
	GDrawSetVisible(bvlayers,false);
	GDrawSetVisible(bvshades,false);
    }
}

void CVPaletteActivate(CharView *cv) {
    _CVPaletteActivate(cv,false,false);
}

void CV_LayerPaletteCheck(SplineFont *sf) {
    CharView *old;

    if ( cvlayers!=NULL ) {
	if ( (old = GDrawGetUserData(cvlayers))!=NULL ) {
	    if ( old->b.sc->parent==sf )
		_CVPaletteActivate(old,true,false);
	}
    }
}

/* make the charview point to the correct layer heads for the specified glyph */
void SFLayerChange(SplineFont *sf) {
    CharView *old, *cv;
    int i;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	SplineChar *sc = sf->glyphs[i];
	for ( cv=(CharView *) (sc->views); cv!=NULL; cv=(CharView *) (cv->b.next) ) {
	    cv->b.layerheads[dm_back] = &sc->layers[ly_back];
	    cv->b.layerheads[dm_fore] = &sc->layers[ly_fore];
	    cv->b.layerheads[dm_grid] = &sf->grid;
	}
    }

    if ( cvtools==NULL )
return;					/* No charviews open */
    old = GDrawGetUserData(cvtools);
    if ( old==NULL || old->b.sc->parent!=sf )	/* Irrelevant */
return;
    _CVPaletteActivate(old,true,false);
}

void CVPalettesHideIfMine(CharView *cv) {
    if ( cvtools==NULL )
return;
    if ( GDrawGetUserData(cvtools)==cv ) {
	SaveOffsets(cv->gw,cvtools,&cvtoolsoff);
	GDrawSetVisible(cvtools,false);
	GDrawSetUserData(cvtools,NULL);
	if ( cv->b.sc->parent->multilayer && cvlayers2!=NULL ) {
	    SaveOffsets(cv->gw,cvlayers2,&cvlayersoff);
	    GDrawSetVisible(cvlayers2,false);
	    GDrawSetUserData(cvlayers2,NULL);
	} else {
	    SaveOffsets(cv->gw,cvlayers,&cvlayersoff);
	    GDrawSetVisible(cvlayers,false);
	    GDrawSetUserData(cvlayers,NULL);
	}
    }
}

int CVPalettesWidth(void) {
return( GGadgetScale(CV_LAYERS2_WIDTH));
}

/* ************************************************************************** */
/* **************************** Bitmap Palettes ***************************** */
/* ************************************************************************** */

static void BVLayersSet(BitmapView *bv) {
    GGadgetSetChecked(GWidgetGetControl(bvlayers,CID_VFore),bv->showfore);
    GGadgetSetChecked(GWidgetGetControl(bvlayers,CID_VBack),bv->showoutline);
    GGadgetSetChecked(GWidgetGetControl(bvlayers,CID_VGrid),bv->showgrid);
}

static int bvlayers_e_h(GWindow gw, GEvent *event) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    if ( event->type==et_destroy && bvlayers == gw ) {
	bvlayers = NULL;
return( true );
    }

    if ( bv==NULL )
return( true );

    switch ( event->type ) {
      case et_close:
	GDrawSetVisible(gw,false);
      break;
      case et_char: case et_charup:
	PostCharToWindow(bv->gw,event);
      break;
      case et_controlevent:
	if ( event->u.control.subtype == et_radiochanged ) {
	    switch(GGadgetGetCid(event->u.control.g)) {
	      case CID_VFore:
		BVShows.showfore = bv->showfore = GGadgetIsChecked(event->u.control.g);
	      break;
	      case CID_VBack:
		BVShows.showoutline = bv->showoutline = GGadgetIsChecked(event->u.control.g);
	      break;
	      case CID_VGrid:
		BVShows.showgrid = bv->showgrid = GGadgetIsChecked(event->u.control.g);
	      break;
	    }
	    GDrawRequestExpose(bv->v,NULL,false);
	}
      break;
    }
return( true );
}

GWindow BVMakeLayers(BitmapView *bv) {
    GRect r;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8], boxes[2], *hvarray[5][3];
    GTextInfo label[8];
    static GBox radio_box = { bt_none, bs_rect, 0, 0, 0, 0, 0, 0, 0, 0, COLOR_DEFAULT, COLOR_DEFAULT, 0, 0, 0, 0, 0, 0, 0 };
    FontRequest rq;
    int i;

    if ( bvlayers!=NULL )
return(bvlayers);
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_positioned|wam_isdlg;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.positioned = true;
    wattrs.is_dlg = true;
    wattrs.utf8_window_title = _("Layers");

    r.width = GGadgetScale(BV_LAYERS_WIDTH); r.height = BV_LAYERS_HEIGHT;
    r.x = -r.width-6; r.y = bv->mbh+BV_TOOLS_HEIGHT+45/*25*/;	/* 45 is right if there's decor, is in kde, not in twm. Sigh */
    if ( palettes_docked ) {
	r.x = 0; r.y = BV_TOOLS_HEIGHT+4;
    } else if ( palettes_fixed ) {
	r.x = 0; r.y = BV_TOOLS_HEIGHT+45;
    }
    bvlayers = CreatePalette( bv->gw, &r, bvlayers_e_h, bv, &wattrs, bv->v );

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    if ( layersfont==NULL ) {
	memset(&rq,'\0',sizeof(rq));
	rq.utf8_family_name = SANS_UI_FAMILIES;
	rq.point_size = -12;
	rq.weight = 400;
	layersfont = GDrawInstanciateFont(cvlayers2,&rq);
	layersfont = GResourceFindFont("LayersPalette.Font",layersfont);
    }
    for ( i=0; i<sizeof(label)/sizeof(label[0]); ++i )
	label[i].font = layersfont;

/* GT: Abbreviation for "Visible" */
    label[0].text = (unichar_t *) _("V");
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 7; gcd[0].gd.pos.y = 5; 
    gcd[0].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels|gg_utf8_popup;
    gcd[0].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[0].creator = GLabelCreate;

    label[1].text = (unichar_t *) "Layer";
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 23; gcd[1].gd.pos.y = 5; 
    gcd[1].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels|gg_utf8_popup;
    gcd[1].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[1].creator = GLabelCreate;
    hvarray[0][0] = &gcd[0]; hvarray[0][1] = &gcd[1]; hvarray[0][2] = NULL;

    gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = 21; 
    gcd[2].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[2].gd.cid = CID_VFore;
    gcd[2].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[2].gd.box = &radio_box;
    gcd[2].creator = GCheckBoxCreate;
    label[2].text = (unichar_t *) _("Bitmap");
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    hvarray[1][0] = &gcd[2]; hvarray[1][1] = GCD_ColSpan; hvarray[1][2] = NULL;

    gcd[3].gd.pos.x = 5; gcd[3].gd.pos.y = 37; 
    gcd[3].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[3].gd.cid = CID_VBack;
    gcd[3].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[3].gd.box = &radio_box;
    gcd[3].creator = GCheckBoxCreate;
    label[3].text = (unichar_t *) _("Outline");
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    hvarray[2][0] = &gcd[3]; hvarray[2][1] = GCD_ColSpan; hvarray[2][2] = NULL;

    gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = 53; 
    gcd[4].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels|gg_utf8_popup;
    gcd[4].gd.cid = CID_VGrid;
    gcd[4].gd.popup_msg = (unichar_t *) _("Is Layer Visible?");
    gcd[4].gd.box = &radio_box;
    gcd[4].creator = GCheckBoxCreate;
    label[4].text = (unichar_t *) _("_Guide");
    label[4].text_is_1byte = true;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    hvarray[3][0] = &gcd[4]; hvarray[3][1] = GCD_ColSpan; hvarray[3][2] = NULL;
    hvarray[4][0] = NULL;

    if ( bv->showfore ) gcd[2].gd.flags |= gg_cb_on;
    if ( bv->showoutline ) gcd[3].gd.flags |= gg_cb_on;
    if ( bv->showgrid ) gcd[4].gd.flags |= gg_cb_on;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = hvarray[0];
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(bvlayers,boxes);
    GHVBoxFitWindow(boxes[0].ret);

    if ( bvvisible[0] )
	GDrawSetVisible(bvlayers,true);
return( bvlayers );
}

struct shades_layout {
    int depth;
    int div;
    int cnt;		/* linear number of squares */
    int size;
};

static void BVShadesDecompose(BitmapView *bv, struct shades_layout *lay) {
    GRect r;
    int temp;

    GDrawGetSize(bvshades,&r);
    lay->depth = BDFDepth(bv->bdf);
    lay->div = 255/((1<<lay->depth)-1);
    lay->cnt = lay->depth==8 ? 16 : lay->depth;
    temp = r.width>r.height ? r.height : r.width;
    lay->size = (temp-8+1)/lay->cnt - 1;
}

static void BVShadesExpose(GWindow pixmap, BitmapView *bv, GRect *r) {
    struct shades_layout lay;
    GRect old;
    int i,j,index;
    GRect block;
    Color bg = default_background;
    int greybg = (3*COLOR_RED(bg)+6*COLOR_GREEN(bg)+COLOR_BLUE(bg))/10;

    GDrawSetLineWidth(pixmap,0);
    BVShadesDecompose(bv,&lay);
    GDrawPushClip(pixmap,r,&old);
    for ( i=0; i<=lay.cnt; ++i ) {
	int p = 3+i*(lay.size+1);
	int m = 8+lay.cnt*(lay.size+1);
	GDrawDrawLine(pixmap,p,0,p,m,bg);
	GDrawDrawLine(pixmap,0,p,m,p,bg);
    }
    block.width = block.height = lay.size;
    for ( i=0; i<lay.cnt; ++i ) {
	block.y = 4 + i*(lay.size+1);
	for ( j=0; j<lay.cnt; ++j ) {
	    block.x = 4 + j*(lay.size+1);
	    index = (i*lay.cnt+j)*lay.div;
	    if (( bv->color >= index - lay.div/2 &&
			bv->color <= index + lay.div/2 ) ||
		 ( bv->color_under_cursor >= index - lay.div/2 &&
		    bv->color_under_cursor <= index + lay.div/2 )) {
		GRect outline;
		outline.x = block.x-1; outline.y = block.y-1;
		outline.width = block.width+1; outline.height = block.height+1;
		GDrawDrawRect(pixmap,&outline,
		    ( bv->color >= index - lay.div/2 &&
			bv->color <= index + lay.div/2 )?0x00ff00:0xffffff);
	    }
	    index = (255-index) * greybg / 255;
	    GDrawFillRect(pixmap,&block,0x010101*index);
	}
    }
}

static void BVShadesMouse(BitmapView *bv, GEvent *event) {
    struct shades_layout lay;
    int i, j;

    GGadgetEndPopup();
    if ( event->type == et_mousemove && !bv->shades_down )
return;
    BVShadesDecompose(bv,&lay);
    if ( event->u.mouse.x<4 || event->u.mouse.y<4 ||
	    event->u.mouse.x>=4+lay.cnt*(lay.size+1) ||
	    event->u.mouse.y>=4+lay.cnt*(lay.size+1) )
return;
    i = (event->u.mouse.y-4)/(lay.size+1);
    j = (event->u.mouse.x-4)/(lay.size+1);
    if ( bv->color != (i*lay.cnt + j)*lay.div ) {
	bv->color = (i*lay.cnt + j)*lay.div;
	GDrawRequestExpose(bvshades,NULL,false);
    }
    if ( event->type == et_mousedown ) bv->shades_down = true;
    else if ( event->type == et_mouseup ) bv->shades_down = false;
    if ( event->type == et_mouseup )
	GDrawRequestExpose(bv->gw,NULL,false);
}

static int bvshades_e_h(GWindow gw, GEvent *event) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    if ( event->type==et_destroy && bvshades == gw ) {
	bvshades = NULL;
return( true );
    }

    if ( bv==NULL )
return( true );

    switch ( event->type ) {
      case et_expose:
	BVShadesExpose(gw,bv,&event->u.expose.rect);
      break;
      case et_mousemove:
      case et_mouseup:
      case et_mousedown:
	BVShadesMouse(bv,event);
      break;
      case et_char: case et_charup:
	PostCharToWindow(bv->gw,event);
      break;
      case et_destroy:
      break;
      case et_close:
	GDrawSetVisible(gw,false);
      break;
    }
return( true );
}

static GWindow BVMakeShades(BitmapView *bv) {
    GRect r;
    GWindowAttrs wattrs;

    if ( bvshades!=NULL )
return( bvshades );
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_positioned|wam_isdlg/*|wam_backcol*/;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_eyedropper;
    wattrs.positioned = true;
    wattrs.is_dlg = true;
    wattrs.background_color = 0xffffff;
    wattrs.utf8_window_title = _("Shades");

    r.width = BV_SHADES_HEIGHT; r.height = r.width;
    r.x = -r.width-6; r.y = bv->mbh+225;
    if ( palettes_docked ) {
	r.x = 0; r.y = BV_TOOLS_HEIGHT+BV_LAYERS_HEIGHT+4;
    } else if ( palettes_fixed ) {
	r.x = 0; r.y = BV_TOOLS_HEIGHT+BV_LAYERS_HEIGHT+90;
    }
    bvshades = CreatePalette( bv->gw, &r, bvshades_e_h, bv, &wattrs, bv->v );
    bv->shades_hidden = BDFDepth(bv->bdf)==1;
    if ( bvvisible[2] && !bv->shades_hidden )
	GDrawSetVisible(bvshades,true);
return( bvshades );
}

static char *bvpopups[] = { N_("Pointer"), N_("Magnify (Minify with alt)"),
				    N_("Set/Clear Pixels"), N_("Draw a Line"),
			            N_("Shift Entire Bitmap"), N_("Scroll Bitmap") };

static void BVToolsExpose(GWindow pixmap, BitmapView *bv, GRect *r) {
    GRect old;
    /* Note: If you change this ordering, change enum bvtools */
    static GImage *buttons[][2] = { { &GIcon_pointer, &GIcon_magnify },
				    { &GIcon_pencil, &GIcon_line },
			            { &GIcon_shift, &GIcon_hand }};
    int i,j,norm;
    int tool = bv->cntrldown?bv->cb1_tool:bv->b1_tool;
    int dither = GDrawSetDither(NULL,false);

    GDrawPushClip(pixmap,r,&old);
    GDrawFillRect(pixmap,r,GDrawGetDefaultBackground(NULL));
    GDrawSetLineWidth(pixmap,0);
    for ( i=0; i<sizeof(buttons)/sizeof(buttons[0]); ++i ) for ( j=0; j<2; ++j ) {
	GDrawDrawImage(pixmap,buttons[i][j],NULL,j*27+1,i*27+1);
	norm = (i*2+j!=tool);
	GDrawDrawLine(pixmap,j*27,i*27,j*27+25,i*27,norm?0xe0e0e0:0x707070);
	GDrawDrawLine(pixmap,j*27,i*27,j*27,i*27+25,norm?0xe0e0e0:0x707070);
	GDrawDrawLine(pixmap,j*27,i*27+25,j*27+25,i*27+25,norm?0x707070:0xe0e0e0);
	GDrawDrawLine(pixmap,j*27+25,i*27,j*27+25,i*27+25,norm?0x707070:0xe0e0e0);
    }
    GDrawPopClip(pixmap,&old);
    GDrawSetDither(NULL,dither);
}

void BVToolsSetCursor(BitmapView *bv, int state,char *device) {
    int shouldshow;
    static enum bvtools tools[bvt_max2+1] = { bvt_none };
    int cntrl;

    if ( tools[0] == bvt_none ) {
	tools[bvt_pointer] = ct_mypointer;
	tools[bvt_magnify] = ct_magplus;
	tools[bvt_pencil] = ct_pencil;
	tools[bvt_line] = ct_line;
	tools[bvt_shift] = ct_shift;
	tools[bvt_hand] = ct_myhand;
	tools[bvt_minify] = ct_magminus;
	tools[bvt_eyedropper] = ct_eyedropper;
	tools[bvt_setwidth] = ct_setwidth;
	tools[bvt_setvwidth] = ct_updown;
	tools[bvt_rect] = ct_rect;
	tools[bvt_filledrect] = ct_filledrect;
	tools[bvt_elipse] = ct_elipse;
	tools[bvt_filledelipse] = ct_filledelipse;
    }

    shouldshow = bvt_none;
    if ( bv->active_tool!=bvt_none )
	shouldshow = bv->active_tool;
    else if ( bv->pressed_display!=bvt_none )
	shouldshow = bv->pressed_display;
    else if ( device==NULL || strcmp(device,"Mouse1")==0 ) {
	if ( (state&(ksm_shift|ksm_control)) && (state&ksm_button4))
	    shouldshow = bvt_magnify;
	else if ( (state&(ksm_shift|ksm_control)) && (state&ksm_button5))
	    shouldshow = bvt_minify;
	else if ( (state&ksm_control) && (state&(ksm_button2|ksm_super)) )
	    shouldshow = bv->cb2_tool;
	else if ( (state&(ksm_button2|ksm_super)) )
	    shouldshow = bv->b2_tool;
	else if ( (state&ksm_control) )
	    shouldshow = bv->cb1_tool;
	else
	    shouldshow = bv->b1_tool;
    } else if ( strcmp(device,"eraser")==0 )
	shouldshow = bv->er_tool;
    else if ( strcmp(device,"stylus")==0 ) {
	if ( (state&(ksm_button2|ksm_control|ksm_super)) )
	    shouldshow = bv->s2_tool;
	else
	    shouldshow = bv->s1_tool;
    }
    
    if ( shouldshow==bvt_magnify && (state&ksm_meta))
	shouldshow = bvt_minify;
    if ( (shouldshow==bvt_pencil || shouldshow==bvt_line) && (state&ksm_meta) && bv->bdf->clut!=NULL )
	shouldshow = bvt_eyedropper;
    if ( shouldshow!=bvt_none && shouldshow!=bv->showing_tool ) {
	GDrawSetCursor(bv->v,tools[shouldshow]);
	if ( bvtools != NULL )
	    GDrawSetCursor(bvtools,tools[shouldshow]);
	bv->showing_tool = shouldshow;
    }

    if ( device==NULL || strcmp(device,"stylus")==0 ) {
	cntrl = (state&ksm_control)?1:0;
	if ( device!=NULL && (state&ksm_button2))
	    cntrl = true;
	if ( cntrl != bv->cntrldown ) {
	    bv->cntrldown = cntrl;
	    GDrawRequestExpose(bvtools,NULL,false);
	}
    }
}

static void BVToolsMouse(BitmapView *bv, GEvent *event) {
    int i = (event->u.mouse.y/27), j = (event->u.mouse.x/27);
    int pos;
    int isstylus = event->u.mouse.device!=NULL && strcmp(event->u.mouse.device,"stylus")==0;
    int styluscntl = isstylus && (event->u.mouse.state&0x200);

    if(j >= 2)
return;			/* If the wm gave me a window the wrong size */

    pos = i*2 + j;
    GGadgetEndPopup();
    if ( pos<0 || pos>=bvt_max )
	pos = bvt_none;
    if ( event->type == et_mousedown ) {
        if ( isstylus && event->u.mouse.button==2 )
            /* Not a real button press, only touch counts. This is a modifier */;
	else {
	    bv->pressed_tool = bv->pressed_display = pos;
	    bv->had_control = ((event->u.mouse.state&ksm_control) || styluscntl)?1:0;
	    event->u.chr.state |= (1<<(7+event->u.mouse.button));
	}
    } else if ( event->type == et_mousemove ) {
	if ( bv->pressed_tool==bvt_none && pos!=bvt_none ) {
	    /* Not pressed */
	    if ( !bv->shades_hidden && strcmp(bvpopups[pos],"Set/Clear Pixels")==0 )
		GGadgetPreparePopup8(bvtools,_("Set/Clear Pixels\n(Eyedropper with alt)"));
	    else
		GGadgetPreparePopup8(bvtools,_(bvpopups[pos]));
	} else if ( pos!=bv->pressed_tool || bv->had_control != (((event->u.mouse.state&ksm_control)||styluscntl)?1:0) )
	    bv->pressed_display = bvt_none;
	else
	    bv->pressed_display = bv->pressed_tool;
    } else if ( event->type == et_mouseup ) {
	if ( pos!=bv->pressed_tool || bv->had_control != (((event->u.mouse.state&ksm_control)||styluscntl)?1:0) )
	    bv->pressed_tool = bv->pressed_display = bvt_none;
	else {
	    if ( event->u.mouse.device!=NULL && strcmp(event->u.mouse.device,"eraser")==0 )
		bv->er_tool = pos;
	    else if ( isstylus ) {
	        if ( event->u.mouse.button==2 )
		    /* Only thing that matters is touch which maps to button 1 */;
		else if ( bv->had_control )
		    bv->s2_tool = pos;
		else
		    bv->s1_tool = pos;
	    } else if ( bv->had_control && event->u.mouse.button==2 )
		bv->cb2_tool = pos;
	    else if ( event->u.mouse.button==2 )
		bv->b2_tool = pos;
	    else if ( bv->had_control ) {
		if ( bv->cb1_tool!=pos ) {
		    bv->cb1_tool = pos;
		    GDrawRequestExpose(bvtools,NULL,false);
		}
	    } else {
		if ( bv->b1_tool!=pos ) {
		    bv->b1_tool = pos;
		    GDrawRequestExpose(bvtools,NULL,false);
		}
	    }
	    bv->pressed_tool = bv->pressed_display = bvt_none;
	}
	event->u.mouse.state &= ~(1<<(7+event->u.mouse.button));
    }
    BVToolsSetCursor(bv,event->u.mouse.state,event->u.mouse.device);
}

static int bvtools_e_h(GWindow gw, GEvent *event) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    if ( event->type==et_destroy && bvtools == gw ) {
	bvtools = NULL;
return( true );
    }

    if ( bv==NULL )
return( true );

    switch ( event->type ) {
      case et_expose:
	BVToolsExpose(gw,bv,&event->u.expose.rect);
      break;
      case et_mousedown:
	BVToolsMouse(bv,event);
      break;
      case et_mousemove:
	BVToolsMouse(bv,event);
      break;
      case et_mouseup:
	BVToolsMouse(bv,event);
      break;
      case et_crossing:
	bv->pressed_display = bvt_none;
	BVToolsSetCursor(bv,event->u.mouse.state,event->u.mouse.device);
      break;
      case et_char: case et_charup:
	if ( bv->had_control != ((event->u.chr.state&ksm_control)?1:0) )
	    bv->pressed_display = bvt_none;
	PostCharToWindow(bv->gw,event);
      break;
      case et_close:
	GDrawSetVisible(gw,false);
      break;
    }
return( true );
}

GWindow BVMakeTools(BitmapView *bv) {
    GRect r;
    GWindowAttrs wattrs;

    if ( bvtools!=NULL )
return( bvtools );
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_positioned|wam_isdlg;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.positioned = true;
    wattrs.is_dlg = true;
    wattrs.utf8_window_title = _("Tools");

    r.width = BV_TOOLS_WIDTH; r.height = BV_TOOLS_HEIGHT;
    r.x = -r.width-6; r.y = bv->mbh+20;
    if ( palettes_fixed || palettes_docked ) {
	r.x = 0; r.y = 0;
    }
    bvtools = CreatePalette( bv->gw, &r, bvtools_e_h, bv, &wattrs, bv->v );
    if ( bvvisible[1] )
	GDrawSetVisible(bvtools,true);
return( bvtools );
}

static void BVPopupInvoked(GWindow v, GMenuItem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(v);
    int pos;

    pos = mi->mid;
    if ( bv->had_control ) {
	if ( bv->cb1_tool!=pos ) {
	    bv->cb1_tool = pos;
	    GDrawRequestExpose(bvtools,NULL,false);
	}
    } else {
	if ( bv->b1_tool!=pos ) {
	    bv->b1_tool = pos;
	    GDrawRequestExpose(bvtools,NULL,false);
	}
    }
    BVToolsSetCursor(bv,bv->had_control?ksm_control:0,NULL);
}

void BVToolsPopup(BitmapView *bv, GEvent *event) {
    GMenuItem mi[21];
    int i, j;

    memset(mi,'\0',sizeof(mi));
    for ( i=0;i<6; ++i ) {
	mi[i].ti.text = (unichar_t *) _(bvpopups[i]);
	mi[i].ti.text_is_1byte = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = i;
	mi[i].invoke = BVPopupInvoked;
    }

    mi[i].ti.text = (unichar_t *) _("Rectangle");
    mi[i].ti.text_is_1byte = true;
    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i].mid = bvt_rect;
    mi[i++].invoke = BVPopupInvoked;
    mi[i].ti.text = (unichar_t *) _("Filled Rectangle"); mi[i].ti.text_is_1byte = true;
    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i].mid = bvt_filledrect;
    mi[i++].invoke = BVPopupInvoked;
    mi[i].ti.text = (unichar_t *) _("Ellipse"); mi[i].ti.text_is_1byte = true;
    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i].mid = bvt_elipse;
    mi[i++].invoke = BVPopupInvoked;
    mi[i].ti.text = (unichar_t *) _("Filled Ellipse"); mi[i].ti.text_is_1byte = true;
    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i].mid = bvt_filledelipse;
    mi[i++].invoke = BVPopupInvoked;

    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i++].ti.line = true;
    for ( j=0; j<6; ++j, ++i ) {
	mi[i].ti.text = (unichar_t *) BVFlipNames[j];
	mi[i].ti.text_is_1byte = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = j;
	mi[i].invoke = BVMenuRotateInvoked;
    }
    if ( bv->fv->b.sf->onlybitmaps ) {
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i++].ti.line = true;
	mi[i].ti.text = (unichar_t *) _("Set _Width...");
	mi[i].ti.text_is_1byte = true;
	mi[i].ti.text_in_resource = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = bvt_setwidth;
	mi[i].invoke = BVPopupInvoked;
    }
    bv->had_control = (event->u.mouse.state&ksm_control)?1:0;
    GMenuCreatePopupMenu(bv->v,event, mi);
}

static void BVPaletteCheck(BitmapView *bv) {
    if ( bvtools==NULL ) {
	BVMakeTools(bv);
	BVMakeLayers(bv);
	BVMakeShades(bv);
    }
}

int BVPaletteIsVisible(BitmapView *bv,int which) {
    BVPaletteCheck(bv);
    if ( which==1 )
return( bvtools!=NULL && GDrawIsVisible(bvtools) );
    if ( which==2 )
return( bvshades!=NULL && GDrawIsVisible(bvshades) );

return( bvlayers!=NULL && GDrawIsVisible(bvlayers) );
}

void BVPaletteSetVisible(BitmapView *bv,int which,int visible) {
    BVPaletteCheck(bv);
    if ( which==1 && bvtools!=NULL)
	GDrawSetVisible(bvtools,visible );
    else if ( which==2 && bvshades!=NULL)
	GDrawSetVisible(bvshades,visible );
    else if ( which==0 && bvlayers!=NULL )
	GDrawSetVisible(bvlayers,visible );
    bvvisible[which] = visible;
    SavePrefs(true);
}

static void _BVPaletteActivate(BitmapView *bv, int force, int docking_changed) {
    BitmapView *old;

    BVPaletteCheck(bv);
    if ( ((old = GDrawGetUserData(bvtools)) != bv) || force ) {
	if ( old!=NULL ) {
	    SaveOffsets(old->gw,bvtools,&bvtoolsoff);
	    SaveOffsets(old->gw,bvlayers,&bvlayersoff);
	    SaveOffsets(old->gw,bvshades,&bvshadesoff);
	}

    if (palettes_docked || docking_changed) {
        // Recreate the bvtools if docked, similar to cvtools
        GDrawDestroyWindow(bvtools);
        GDrawDestroyWindow(bvlayers);
        GDrawDestroyWindow(bvshades);
        bvtools = bvlayers = bvshades = NULL;
        BVPaletteCheck(bv);
    }

	GDrawSetUserData(bvtools,bv);
	GDrawSetUserData(bvlayers,bv);
	GDrawSetUserData(bvshades,bv);

    if (palettes_docked) {
        if (bvvisible[0])
            GDrawRequestExpose(bvlayers, NULL, false);
        if (bvvisible[1])
            GDrawRequestExpose(bvtools, NULL, false);
        if (bvvisible[2])
            GDrawRequestExpose(bvshades, NULL, false);
    } else {
	    if ( bvvisible[0])
		RestoreOffsets(bv->gw,bvlayers,&bvlayersoff);
	    if ( bvvisible[1])
		RestoreOffsets(bv->gw,bvtools,&bvtoolsoff);
	    if ( bvvisible[2] && !bv->shades_hidden )
		RestoreOffsets(bv->gw,bvshades,&bvshadesoff);
	}
	GDrawSetVisible(bvtools,bvvisible[1]);
	GDrawSetVisible(bvlayers,bvvisible[0]);
	GDrawSetVisible(bvshades,bvvisible[2] && bv->bdf->clut!=NULL);
	if ( bvvisible[1]) {
	    bv->showing_tool = bvt_none;
	    BVToolsSetCursor(bv,0,NULL);
	    GDrawRequestExpose(bvtools,NULL,false);
	}
	if ( bvvisible[0])
	    BVLayersSet(bv);
	if ( bvvisible[2] && !bv->shades_hidden )
	    GDrawRequestExpose(bvtools,NULL,false);
    }
    if ( cvtools!=NULL ) {
	CharView *cv = GDrawGetUserData(cvtools);
	if ( cv!=NULL ) {
	    SaveOffsets(cv->gw,cvtools,&cvtoolsoff);
	    SaveOffsets(cv->gw,cvlayers,&cvlayersoff);
	    GDrawSetUserData(cvtools,NULL);
	    if ( cvlayers!=NULL )
		GDrawSetUserData(cvlayers,NULL);
	    if ( cvlayers2!=NULL )
		GDrawSetUserData(cvlayers2,NULL);
	}
	GDrawSetVisible(cvtools,false);
	if ( cvlayers!=NULL )
	    GDrawSetVisible(cvlayers,false);
	if ( cvlayers2!=NULL )
	    GDrawSetVisible(cvlayers2,false);
    }
}

void BVPaletteActivate(BitmapView *bv) {
    _BVPaletteActivate(bv, false, false);
}

void BVPalettesHideIfMine(BitmapView *bv) {
    if ( bvtools==NULL )
return;
    if ( GDrawGetUserData(bvtools)==bv ) {
	SaveOffsets(bv->gw,bvtools,&bvtoolsoff);
	SaveOffsets(bv->gw,bvlayers,&bvlayersoff);
	SaveOffsets(bv->gw,bvshades,&bvshadesoff);
	GDrawSetVisible(bvtools,false);
	GDrawSetVisible(bvlayers,false);
	GDrawSetVisible(bvshades,false);
	GDrawSetUserData(bvtools,NULL);
	GDrawSetUserData(bvlayers,NULL);
	GDrawSetUserData(bvshades,NULL);
    }
}

void CVPaletteDeactivate(void) {
    if ( cvtools!=NULL ) {
	CharView *cv = GDrawGetUserData(cvtools);
	if ( cv!=NULL ) {
	    SaveOffsets(cv->gw,cvtools,&cvtoolsoff);
	    GDrawSetUserData(cvtools,NULL);
	    if ( cv->b.sc->parent->multilayer && cvlayers2!=NULL ) {
		SaveOffsets(cv->gw,cvlayers2,&cvlayersoff);
		GDrawSetUserData(cvlayers2,NULL);
	    } else if ( cvlayers!=NULL ) {
		SaveOffsets(cv->gw,cvlayers,&cvlayersoff);
		GDrawSetUserData(cvlayers,NULL);
	    }
	}
	GDrawSetVisible(cvtools,false);
	if ( cvlayers!=NULL )
	    GDrawSetVisible(cvlayers,false);
	if ( cvlayers2!=NULL )
	    GDrawSetVisible(cvlayers2,false);
    }
    if ( bvtools!=NULL ) {
	BitmapView *bv = GDrawGetUserData(bvtools);
	if ( bv!=NULL ) {
	    SaveOffsets(bv->gw,bvtools,&bvtoolsoff);
	    SaveOffsets(bv->gw,bvlayers,&bvlayersoff);
	    SaveOffsets(bv->gw,bvshades,&bvshadesoff);
	    GDrawSetUserData(bvtools,NULL);
	    GDrawSetUserData(bvlayers,NULL);
	    GDrawSetUserData(bvshades,NULL);
	}
	GDrawSetVisible(bvtools,false);
	GDrawSetVisible(bvlayers,false);
	GDrawSetVisible(bvshades,false);
    }
}

void BVPaletteColorChange(BitmapView *bv) {
    if ( bvshades!=NULL )
	GDrawRequestExpose(bvshades,NULL,false);
    GDrawRequestExpose(bv->gw,NULL,false);
}

void BVPaletteColorUnderChange(BitmapView *bv,int color_under) {
    if ( bvshades!=NULL && color_under!=bv->color_under_cursor ) {
	bv->color_under_cursor = color_under;
	GDrawRequestExpose(bvshades,NULL,false);
    }
}

void BVPaletteChangedChar(BitmapView *bv) {
    if ( bvshades!=NULL && bvvisible[2]) {
	int hidden = bv->bdf->clut==NULL;
	if ( hidden!=bv->shades_hidden ) {
	    GDrawSetVisible(bvshades,!hidden);
	    bv->shades_hidden = hidden;
	    GDrawRequestExpose(bv->gw,NULL,false);
	} else
	    GDrawRequestExpose(bvshades,NULL,false);
    }
}

void PalettesChangeDocking() {
    palettes_docked = !palettes_docked;

    if (cvtools != NULL)
        _CVPaletteActivate((CharView*)GDrawGetUserData(cvtools), true, true);
    if (bvtools != NULL)
        _BVPaletteActivate((BitmapView*)GDrawGetUserData(bvtools), true, true);

    SavePrefs(true);
}

int BVPalettesWidth(void) {
return( GGadgetScale(BV_LAYERS_WIDTH));
}
