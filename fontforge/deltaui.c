/* Copyright (C) 2009 by George Williams */
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
#include "pfaeditui.h"
#include <ustring.h>
#include <chardata.h>
#include <utype.h>
#include <gkeysym.h>
#include "delta.h"

/* Suggestions for where delta instructions might be wanted */

#define CID_Sizes	100
#define CID_DPI		101
#define CID_BW		102
#define CID_Within	103
#define CID_Msg		104
#define CID_Ok		105
#define CID_Cancel	106
#define CID_Top		107

static double delta_within = .05;
static char *delta_sizes=NULL;
static int delta_dpi = 100;
static int delta_depth = 1;

static void StartDeltaDisplay(QGData *qg);

static int Delta_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	QGData *qg = GDrawGetUserData(GGadgetGetWindow(g));
	int err=false;
	int dpi, depth;
	double within;
	char *sizes;

	within = GetReal8(qg->gw,CID_Within,_("Proximity"),&err);
	dpi = GetInt8(qg->gw,CID_DPI,_("DPI"),&err);
	if ( err )
return(true);
	if ( within<=0 || within>=.5 ) {
	    ff_post_error(_("Bad Number"),_("The \"Proximity\" field must be more than 0 and less than a half."));
return( true );
	}
	if ( dpi<10 || dpi>5000 ) {
	    ff_post_error(_("Unreasonable DPI"),_("The \"DPI\" field must be more than 10 and less than 5000."));
return( true );
	}
	depth = GGadgetIsChecked(GWidgetGetControl(qg->gw,CID_BW)) ? 1 : 8;
	sizes = GGadgetGetTitle8(GWidgetGetControl(qg->gw,CID_Sizes));

	GGadgetSetVisible(GWidgetGetControl(qg->gw,CID_Msg),true);
	GGadgetSetVisible(GWidgetGetControl(qg->gw,CID_Ok),false);
	GGadgetSetVisible(GWidgetGetControl(qg->gw,CID_Cancel),false);
	GDrawSetCursor(qg->gw,ct_watch);
	GDrawProcessPendingEvents(NULL);

	qg->within = within;
	qg->dpi = dpi;
	qg->pixelsizes = sizes;
	qg->depth = depth;
	TopFindQuestionablePoints(qg);

	GGadgetSetVisible(GWidgetGetControl(qg->gw,CID_Msg),false);
	GGadgetSetVisible(GWidgetGetControl(qg->gw,CID_Ok),true);
	GGadgetSetVisible(GWidgetGetControl(qg->gw,CID_Cancel),true);
	GDrawSetCursor(qg->gw,ct_pointer);
	GDrawProcessPendingEvents(NULL);

	if ( qg->error!=qg_ok ) {
	    switch ( qg->error ) {
	      case qg_notnumber:
		ff_post_error(_("Bad Number"),_("An entry in the \"Sizes\" field is not a number."));
	      break;
	      case qg_badnumber:
		ff_post_error(_("Bad Number"),_("An entry in the \"Sizes\" field is unreasonable."));
	      break;
	      case qg_badrange:
		ff_post_error(_("Bad Number"),_("An range in the \"Sizes\" field is incorrectly ordered."));
	      break;
	      case qg_nofont:
		ff_post_error(_("FreeType unavailable"),_("FreeType unavailable."));
	      break;
	      default:
		IError(_("Unexpected error"));
	      break;
	    }
	    free(sizes);
	    qg->cur = 0;
return( true );
	}

	free(delta_sizes);
	delta_within = within;
	delta_dpi    = dpi;
	delta_depth  = depth;
	delta_sizes  = sizes;

	if ( qg->cur==0 ) {
	    ff_post_error(_("Nothing found"),_("Nothng found."));
	    qg->done = true;
return( true );
	}

	if ( qg->cur >= qg->max )
	    qg->qg = grealloc(qg->qg,(qg->max += 1) * sizeof(QuestionableGrid));
	memset(&qg->qg+qg->cur,0,sizeof(QuestionableGrid));
	GDrawSetVisible(qg->gw,false);
	StartDeltaDisplay(qg);
	qg->done = true;
    }
return( true );
}

static int Delta_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	QGData *qg = GDrawGetUserData(GGadgetGetWindow(g));
	qg->done = true;
    }
return( true );
}

static int delta_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	QGData *qg = GDrawGetUserData(gw);
	qg->done = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    } else if ( event->type == et_destroy ) {
	QGData *qg = GDrawGetUserData(gw);
	free(qg->qg);
	free(qg);
    }
return( true );
}

void DeltaSuggestionDlg(FontView *fv,CharView *cv) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[13], boxes[4];
    GTextInfo label[13];
    GGadgetCreateData *varray[7][5], *barray[9];
    char dpi_buffer[40], within_buffer[40];
    QGData *data;
    int failed = false;
    int k, r;

    if ( !hasFreeType() ) {
	ff_post_error(_("No FreeType"),_("You must install the freetype library before using this command."));
return;
    }
    if ( !hasFreeTypeByteCode() ) {
	ff_post_error(_("No FreeType"),_("Your version of the freetype library does not contain the bytecode interpreter."));
return;
    }
    
    if ( delta_sizes==NULL )
	delta_sizes = copy("7-40,72,80,88,96");

    data = gcalloc(1,sizeof(QGData));
    data->fv = (FontViewBase *) fv;
    data->cv = cv;
    if ( cv!=NULL ) {
	data->sc = cv->b.sc;
	data->layer = CVLayer((CharViewBase *) cv);
	if ( !data->sc->parent->layers[data->layer].order2 )
	    failed = true;
	if ( !failed && data->sc->ttf_instrs_len==0 )
	    ff_post_notice(_("No Instructions"),_("This glyph has no instructions. Adding instructions (a DELTA) may change its rasterization significantly."));
    } else {
	data->layer = fv->b.active_layer;
	if ( !fv->b.sf->layers[data->layer].order2 )
	    failed = true;
    }
    if ( failed ) {
	ff_post_error(_("Not quadratic"),_("This must be a truetype layer."));
	free(data);
return;
    }

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("DELTA suggestions");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,190));
    pos.height = GDrawPointsToPixels(NULL,106);
    data->gw = gw = GDrawCreateTopWindow(NULL,&pos,delta_e_h,data,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    k=r=0;

    label[k].text = (unichar_t *) _(
	    "When a curve passes very close to the center of a\n"
	    "pixel you might want to check that the curve is on\n"
	    "the intended side of that pixel.\n"
	    "If it's on the wrong side, consider using a DELTA\n"
	    "instruction to adjust the closest point at the\n"
	    "current pixelsize."
	);
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;
    varray[r][0] = &gcd[k-1]; varray[r][1] = GCD_ColSpan; varray[r][2] = GCD_ColSpan; varray[r][3] = GCD_ColSpan; varray[r++][4] = NULL;

    label[k].text = (unichar_t *) _("Rasterize at sizes:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) delta_sizes;
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_Sizes;
    gcd[k++].creator = GTextFieldCreate;
    varray[r][0] = &gcd[k-2]; varray[r][1] = &gcd[k-1]; varray[r][2] = GCD_ColSpan; varray[r][3] = GCD_ColSpan; varray[r++][4] = NULL;

    label[k].text = (unichar_t *) _("DPI:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;

    sprintf( dpi_buffer, "%d", delta_dpi );
    label[k].text = (unichar_t *) dpi_buffer;
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.width = 40;
    gcd[k].gd.cid = CID_DPI;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GTextFieldCreate;

    label[k].text = (unichar_t *) _("_Mono");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = delta_depth==1 ? (gg_enabled|gg_visible|gg_cb_on) : (gg_enabled|gg_visible);
    gcd[k].gd.cid = CID_BW;
    gcd[k++].creator = GRadioCreate;
    varray[r][0] = &gcd[k-1]; varray[r][1] = GCD_HPad10;

    label[k].text = (unichar_t *) _("_Anti-Aliased");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = delta_depth!=1 ? (gg_enabled|gg_visible|gg_cb_on) : (gg_enabled|gg_visible);
    gcd[k++].creator = GRadioCreate;
    varray[r][0] = &gcd[k-4]; varray[r][1] = &gcd[k-3]; varray[r][2] = &gcd[k-2]; varray[r][3] = &gcd[k-1]; varray[r++][4] = NULL;

    label[k].text = (unichar_t *) _("Proximity:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;

    sprintf( within_buffer, "%g", delta_within );
    label[k].text = (unichar_t *) within_buffer;
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.pos.width = 40;
    gcd[k].gd.cid = CID_Within;
    gcd[k++].creator = GTextFieldCreate;

    label[k].text = (unichar_t *) _("pixels");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;
    varray[r][0] = &gcd[k-3]; varray[r][1] = &gcd[k-2]; varray[r][2] = &gcd[k-1]; varray[r][3] = GCD_ColSpan; varray[r++][4] = NULL;

    label[k].text = (unichar_t *) _( "This may take a while. Please be patient..." );
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled;
    gcd[k].gd.cid = CID_Msg;
    gcd[k++].creator = GLabelCreate;
    varray[r][0] = &gcd[k-1]; varray[r][1] = GCD_ColSpan; varray[r][2] = GCD_ColSpan; varray[r][3] = GCD_ColSpan; varray[r++][4] = NULL;

    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = Delta_OK;
    gcd[k].gd.cid = CID_Ok;
    gcd[k++].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[k-1]; barray[2] = GCD_Glue;

    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = Delta_Cancel;
    gcd[k].gd.cid = CID_Cancel;
    gcd[k++].creator = GButtonCreate;
    barray[3] = GCD_Glue; barray[4] = &gcd[k-1]; barray[5] = GCD_Glue; barray[6] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = barray;
    boxes[2].creator = GHBoxCreate;
    varray[r][0] = &boxes[2]; varray[r][1] = GCD_ColSpan; varray[r][2] = GCD_ColSpan; varray[r][3] = GCD_ColSpan; varray[r++][4] = NULL;
    varray[r][0] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray[0];
    boxes[0].gd.cid = CID_Top;
    boxes[0].creator = GHVGroupCreate;


    GGadgetsCreate(gw,boxes);
    GHVBoxFitWindow(boxes[0].ret);

    GDrawSetVisible(gw,true);
    while ( !data->done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

/* ************************************************************************** */
/* ************************************************************************** */

#define CID_Sort	200
#define CID_GlyphSort	201

static GTextInfo sorts[] = {
    { (unichar_t *) N_("Glyph, Size, Point"), NULL, 0, 0, (void *) is_glyph_size_pt, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Glyph, Point, Size"), NULL, 0, 0, (void *) is_glyph_pt_size, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Size, Glyph, Point"), NULL, 0, 0, (void *) is_size_glyph_pt, NULL, 0, 0, 0, 0, 0, 0, 1},
    NULL
};
static GTextInfo glyphsorts[] = {
    { (unichar_t *) N_("Unicode"), NULL, 0, 0, (void *) gs_unicode, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Alphabetic"), NULL, 0, 0, (void *) gs_alpha, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Glyph Order"), NULL, 0, 0, (void *) gs_gid, NULL, 0, 0, 0, 0, 0, 0, 1},
    NULL
};

static const QGData *kludge;

static int qg_sorter(const void *pt1, const void *pt2) {
    const QuestionableGrid *q1 = pt1, *q2 = pt2;
    const QGData *qg = kludge;
    int pt_cmp = q1->nearestpt - q2->nearestpt;
    int size_cmp = q1->size - q2->size;
    int g_cmp;
    int t1, t2, t3;

    switch ( qg->glyph_sort ) {
      case gs_unicode:
	g_cmp = q1->sc->unicodeenc - q2->sc->unicodeenc;
      break;
      case gs_gid:
	g_cmp = q1->sc->orig_pos - q2->sc->orig_pos;
      break;
      case gs_alpha:
	g_cmp = strcmp(q1->sc->name,q2->sc->name);
      break;
    }
    switch ( qg->info_sort ) {
      case is_glyph_size_pt:
	t1 = g_cmp; t2 = size_cmp; t3 = pt_cmp;
      break;
      case is_glyph_pt_size:
	t1 = g_cmp; t2 = pt_cmp; t3 = size_cmp;
      break;
      case is_size_glyph_pt:
	t1 = size_cmp; t2 = g_cmp; t3 = pt_cmp;
      break;
    }
    if ( t1!=0 )
return( t1 );
    if ( t2!=0 )
return( t2 );

return( t3 );
}

static void QGDoSort(QGData *qg) {
    int pos;

    pos = GGadgetGetFirstListSelectedItem(GWidgetGetControl(qg->gw,CID_Sort));
    qg->info_sort = (intpt) sorts[pos].userdata;

    pos = GGadgetGetFirstListSelectedItem(GWidgetGetControl(qg->gw,CID_GlyphSort));
    qg->glyph_sort = (intpt) glyphsorts[pos].userdata;

    kludge = qg;
    qsort(qg->qg,qg->cur,sizeof(QuestionableGrid),qg_sorter);
}

static int QGSorter(GGadget *g, GEvent *e) {
    if ( e->u.control.subtype == et_listselected ) {
	QGData *qg = GDrawGetUserData(GGadgetGetWindow(g));
	QGDoSort(qg);
	GDrawRequestExpose(qg->v,NULL,false);
    }
return( true );
}

static void QGDrawWindow(GWindow pixmap, QGData *qg, GEvent *e) {
    int l, y;
    char buffer[200];
    GRect old;

    GDrawPushClip(pixmap,&e->u.expose.rect,&old);
    y = qg->as;
    for ( l=0; l<qg->vlcnt && l+qg->loff_top<qg->cur; ++l ) {
	QuestionableGrid *q = &qg->qg[l+qg->loff_top];
	sprintf( buffer, "\"%.40s\" size=%d point=%d (%d,%d) distance=%g",
		q->sc->name, q->size, q->nearestpt, q->x, q->y, q->distance );
	GDrawDrawBiText8(pixmap,2,y,buffer,-1,NULL, 0x000000);
	y += qg->fh;
    }
}

static void QGMouse( QGData *qg, GEvent *e) {
}

static int QG_VScroll(GGadget *g, GEvent *e) {
    QGData *qg = GDrawGetUserData(GGadgetGetWindow(g));
    int newpos = qg->loff_top;

    switch( e->u.control.u.sb.type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= 9*qg->vlcnt/10;
      break;
      case et_sb_up:
        newpos -= qg->vlcnt/15;
      break;
      case et_sb_down:
        newpos += qg->vlcnt/15;
      break;
      case et_sb_downpage:
        newpos += 9*qg->vlcnt/10;
      break;
      case et_sb_bottom:
        newpos = 0;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = e->u.control.u.sb.pos;
      break;
      case et_sb_halfup:
        newpos -= qg->vlcnt/30;
      break;
      case et_sb_halfdown:
        newpos += qg->vlcnt/30;
      break;
    }
    if ( newpos + qg->vlcnt > qg->cur )
	newpos = qg->cur-qg->vlcnt;
    if ( newpos<0 )
	newpos = 0;
    if ( qg->loff_top!=newpos ) {
	qg->loff_top = newpos;
	GScrollBarSetPos(qg->vsb,newpos);
	GDrawRequestExpose(qg->v,NULL,false);
    }
return( true );
}

static void QG_SetSb(QGData *qg) {
    if ( qg->loff_top + qg->vlcnt > qg->cur )
	qg->loff_top = qg->cur-qg->vlcnt;
    if ( qg->loff_top<0 )
	qg->loff_top = 0;
    GScrollBarSetBounds(qg->vsb,0,qg->cur,qg->vlcnt);
    GScrollBarSetPos(qg->vsb,qg->loff_top);
}
	
static int qgv_e_h(GWindow gw, GEvent *event) {
    QGData *qg = (QGData *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	QGDrawWindow(gw,qg,event);
      break;
      case et_mouseup:
      case et_mousedown:
      case et_mousemove:
	QGMouse(qg,event);
      break;
      case et_char:
return( false );
      break;
      case et_resize: {
	int vlcnt = event->u.resize.size.height/qg->fh;
	qg->vlcnt = vlcnt;
	QG_SetSb(qg);
	GDrawRequestExpose(qg->v,NULL,false);
      } break;
    }
return( true );
}

static int QG_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	QGData *qg = GDrawGetUserData(GGadgetGetWindow(g));
	qg->done = true;
    }
return( true );
}

static int qg_e_h(GWindow gw, GEvent *event) {

    if ( event->type==et_close ) {
	QGData *qg = (QGData *) GDrawGetUserData(gw);
	qg->done = true;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

static void StartDeltaDisplay(QGData *qg) {
    GWindowAttrs wattrs;
    GRect pos;
    GWindow gw, oldgw = qg->gw;
    GGadgetCreateData gcd[8], boxes[5], *harray[4], *harray2[5], *butarray[8],
	    *varray[4];
    GTextInfo label[8];
    int k;
    FontRequest rq;
    int as, ds, ld;
    static GFont *valfont=NULL;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.utf8_window_title = _("Potential spots for Delta instructions");
    wattrs.is_dlg = true;
    wattrs.undercursor = 1;
    pos.x = pos.y = 0;
    pos.width = GDrawPointsToPixels(NULL,200);
    pos.height = GDrawPointsToPixels(NULL,300);
    qg->gw = gw = GDrawCreateTopWindow(NULL,&pos,qg_e_h,qg,&wattrs);
    qg->done = false;

    if ( valfont==NULL ) {
	memset(&rq,0,sizeof(rq));
	rq.utf8_family_name = "Helvetica";
	rq.point_size = 11;
	rq.weight = 400;
	valfont = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
	valfont = GResourceFindFont("Validate.Font",valfont);
    }
    qg->font = valfont;
    GDrawFontMetrics(qg->font,&as,&ds,&ld);
    qg->fh = as+ds;
    qg->as = as;

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    k = 0;
    label[k].text = (unichar_t *) _("Sort:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;

    sorts[0].selected = true; sorts[1].selected = sorts[2].selected = false;
    sorts[2].disabled = qg->sc!=NULL;
    gcd[k].gd.u.list = sorts;
    gcd[k].gd.cid = CID_Sort;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.handle_controlevent = QGSorter;
    gcd[k++].creator = GListButtonCreate;

    label[k].text = (unichar_t *) _("Glyph:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;

    gcd[k].gd.u.list = glyphsorts;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_GlyphSort;
    gcd[k].gd.handle_controlevent = QGSorter;
    gcd[k++].creator = GListButtonCreate;
    if ( qg->sc!=NULL )
	gcd[k-1].gd.flags = gcd[k-2].gd.flags = gg_enabled;
    harray2[0] = &gcd[k-4]; harray2[1] = &gcd[k-3]; harray2[2] = &gcd[k-2]; harray2[3] = &gcd[k-1]; harray2[4] = NULL;


    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.u.drawable_e_h = qgv_e_h;
    gcd[k++].creator = GDrawableCreate;

    gcd[k].gd.flags = gg_visible | gg_enabled | gg_sb_vert;
    gcd[k].gd.handle_controlevent = QG_VScroll;
    gcd[k++].creator = GScrollBarCreate;
    harray[0] = &gcd[k-2]; harray[1] = &gcd[k-1]; harray[2] = NULL; harray[3] = NULL;

    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = QG_OK;
    gcd[k++].creator = GButtonCreate;
    butarray[0] = GCD_Glue; butarray[1] = &gcd[k-1]; butarray[2] = GCD_Glue; butarray[3] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray;
    boxes[2].creator = GHVGroupCreate;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = butarray;
    boxes[3].creator = GHBoxCreate;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = harray2;
    boxes[4].creator = GHBoxCreate;
    varray[0] = &boxes[4]; varray[1] = &boxes[2]; varray[2] = &boxes[3]; varray[3] = NULL;

    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GVBoxCreate;

    GGadgetsCreate(gw,boxes);
    qg->vsb = gcd[5].ret;
    qg->v = GDrawableGetWindow(gcd[4].ret);
    GHVBoxSetExpandableRow(boxes[0].ret,0);
    GHVBoxSetExpandableCol(boxes[2].ret,0);
    GHVBoxSetPadding(boxes[2].ret,0,0);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxFitWindow(boxes[0].ret);

    QGDoSort(qg);

    GDrawSetVisible(gw,true);
    while ( !qg->done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);

    qg->gw = oldgw;
}
