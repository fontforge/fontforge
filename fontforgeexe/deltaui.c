/* Copyright (C) 2009-2012 by George Williams */
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

#include "cvundoes.h"
#include "delta.h"
#include "fontforgeui.h"
#include "gimage.h"
#include "gkeysym.h"
#include "gresedit.h"
#include "ustring.h"
#include "utype.h"

#include <math.h>

/* Suggestions for where delta instructions might be wanted */

#define CID_Sizes	100
#define CID_DPI		101
#define CID_BW		102
#define CID_Within	103
#define CID_Msg		104
#define CID_Ok		105
#define CID_Cancel	106
#define CID_Top		107

extern GBox _ggadget_Default_Box;
#define MAIN_FOREGROUND (_ggadget_Default_Box.main_foreground)

extern GResFont validate_font;

static double delta_within = .02;
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
	    qg->qg = realloc(qg->qg,(qg->max += 1) * sizeof(QuestionableGrid));
	memset(qg->qg+qg->cur,0,sizeof(QuestionableGrid));
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
    FontView *savefv;

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

    data = calloc(1,sizeof(QGData));
    data->fv = (FontViewBase *) fv;
    data->cv = cv;
    if ( cv!=NULL ) {
	data->sc = cv->b.sc;
	savefv = (FontView *) cv->b.fv;
	data->layer = CVLayer((CharViewBase *) cv);
	if ( !data->sc->parent->layers[data->layer].order2 )
	    failed = true;
	if ( !failed && data->sc->ttf_instrs_len==0 )
	    ff_post_notice(_("No Instructions"),_("This glyph has no instructions. Adding instructions (a DELTA) may change its rasterization significantly."));
	cv->qg = data;
	cv->note_x = cv->note_y = 32766;
    } else {
	savefv = fv;
	data->layer = fv->b.active_layer;
	if ( !fv->b.sf->layers[data->layer].order2 )
	    failed = true;
    }
    if ( failed ) {
	ff_post_error(_("Not quadratic"),_("This must be a truetype layer."));
	free(data);
	if ( cv!=NULL )
	    cv->qg = NULL;
return;
    }
    savefv->qg = data;

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
    if ( data->cv!=NULL )
	data->cv->qg = NULL;
    if ( savefv->qg == data )
	savefv->qg = NULL;
}

void QGRmCharView(QGData *qg,CharView *cv) {
    if ( qg->cv==cv )
	qg->cv = NULL;
}

void QGRmFontView(QGData *qg,FontView *fv) {
    qg->done = true;
    fv->qg = NULL;
}
/* ************************************************************************** */
/* ************************************************************************** */

#define CID_Sort	200
#define CID_GlyphSort	201

static GTextInfo sorts[] = {
    { (unichar_t *) N_("Glyph, Size, Point"), NULL, 0, 0, (void *) is_glyph_size_pt, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Glyph, Point, Size"), NULL, 0, 0, (void *) is_glyph_pt_size, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Size, Glyph, Point"), NULL, 0, 0, (void *) is_size_glyph_pt, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};
static GTextInfo glyphsorts[] = {
    { (unichar_t *) N_("Unicode"), NULL, 0, 0, (void *) gs_unicode, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Sort|Alphabetic"), NULL, 0, 0, (void *) gs_alpha, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Glyph Order"), NULL, 0, 0, (void *) gs_gid, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};

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
    if ( newpos + qg->vlcnt > qg->lcnt )
	newpos = qg->lcnt-qg->vlcnt;
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
    if ( qg->loff_top + qg->vlcnt > qg->lcnt )
	qg->loff_top = qg->lcnt-qg->vlcnt;
    if ( qg->loff_top<0 )
	qg->loff_top = 0;
    GScrollBarSetBounds(qg->vsb,0,qg->lcnt,qg->vlcnt);
    GScrollBarSetPos(qg->vsb,qg->loff_top);
}

static int QG_Count(struct qgnode *parent) {
    int l, cnt=0;

    if ( !parent->open )
return( 0 );
    if ( parent->kids==NULL )
return( parent->qg_cnt );

    for ( l=0; l<parent->kid_cnt; ++l ) {
	parent->kids[l].tot_under = QG_Count(&parent->kids[l]);
	cnt += 1 + parent->kids[l].tot_under;
    }
return( cnt );
}

static void QG_Remetric(QGData *qg) {
    GRect size;

    GDrawGetSize(qg->v,&size);
    qg->vlcnt = size.height/qg->fh;
    qg->lcnt = QG_Count(&qg->list);
    QG_SetSb(qg);
}

struct navigate {
    struct qgnode *parent;
    int offset;			/* offset of -1 means the qgnode */
};

static void QG_FindLine(struct qgnode *parent, int l,struct navigate *where) {
    int k;

    if ( parent->kids == NULL ) {
	if ( l<parent->qg_cnt ) {
	    where->parent = parent;
	    where->offset = l;
	} else {
	    where->parent = NULL;
	    where->offset = -1;
	}
return;
    }
    for ( k=0; k<parent->kid_cnt; ++k ) {
	if ( l==0 ) {
	    where->parent = &parent->kids[k];
	    where->offset = -1;
return;
	}
	--l;
	if ( parent->kids[k].open ) {
	    if ( l<parent->kids[k].tot_under ) {
		QG_FindLine(&parent->kids[k],l,where);
return;
	    } else
		l -= parent->kids[k].tot_under;
	}
    }
    where->parent = NULL;
    where->offset = -1;
}

static void QG_NextLine(struct navigate *where) {
    int k;

    if ( where->parent==NULL )
return;
    if ( where->offset!=-1 && where->offset+1<where->parent->qg_cnt ) {
	++where->offset;
return;
    }
    if ( where->parent->open && where->offset==-1 ) {
	if ( where->parent->kids==NULL ) {
	    where->offset = 0;
return;
	} else {
	    where->parent = &where->parent->kids[0];
	    where->offset = -1;
return;
	}
    }
    for (;;) {
	if ( where->parent->parent==NULL ) {
	    where->parent = NULL;
	    where->offset = -1;
return;
	}
	k = where->parent - where->parent->parent->kids;
	if ( k+1< where->parent->parent->kid_cnt ) {
	    ++where->parent;
	    where->offset = -1;
return;
	}
	where->parent = where->parent->parent;
    }
}

static void qgnodeFree(struct qgnode *parent) {
    int i;

    for ( i=0; i<parent->kid_cnt; ++i )
	qgnodeFree(&parent->kids[i]);
    free(parent->kids);
    free(parent->name);
}

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

static void QGSecondLevel(QGData *qg, struct qgnode *parent) {
    int cnt, l, lstart;
    int size;
    SplineChar *sc;
    int pt;
    char buffer[200];

    switch ( qg->info_sort ) {
      case is_glyph_size_pt:	/* size */
	size = -1;
	cnt = 0;
	for ( l=0; l<parent->qg_cnt; ++l ) {
	    if ( size!=parent->first[l].size ) {
		++cnt;
		size = parent->first[l].size;
	    }
	}
	parent->kid_cnt = cnt;
	parent->kids = calloc(cnt,sizeof(struct qgnode));
	cnt = 0;
	lstart = 0; size=-1;
	for ( l=0; l<parent->qg_cnt; ++l ) {
	    if ( size!=parent->first[l].size && size!=-1 ) {
		sprintf( buffer, _("Size: %d (%d)"), size, l-lstart );
		parent->kids[cnt].name = copy(buffer);
		parent->kids[cnt].parent = parent;
		parent->kids[cnt].first = &parent->first[lstart];
		parent->kids[cnt].qg_cnt = l-lstart;
		++cnt;
		lstart = l;
		size = parent->first[l].size;
	    } else if ( size!=parent->first[l].size )
		size = parent->first[l].size;
	}
	if ( size!=-1 ) {
	    sprintf( buffer, _("Size: %d (%d)"), size, l-lstart );
	    parent->kids[cnt].name = copy(buffer);
	    parent->kids[cnt].parent = parent;
	    parent->kids[cnt].first = &parent->first[lstart];
	    parent->kids[cnt].qg_cnt = l-lstart;
	}
      break;
      case is_glyph_pt_size:	/* pt */
	pt = -1;
	cnt = 0;
	for ( l=0; l<parent->qg_cnt; ++l ) {
	    if ( pt!=parent->first[l].nearestpt ) {
		++cnt;
		pt = parent->first[l].nearestpt;
	    }
	}
	parent->kid_cnt = cnt;
	parent->kids = calloc(cnt,sizeof(struct qgnode));
	cnt = 0;
	lstart = 0; pt=-1;
	for ( l=0; l<parent->qg_cnt; ++l ) {
	    if ( pt!=parent->first[l].nearestpt && pt!=-1 ) {
		sprintf( buffer, _("Point: %d (%d)"), pt, l-lstart );
		parent->kids[cnt].name = copy(buffer);
		parent->kids[cnt].parent = parent;
		parent->kids[cnt].first = &parent->first[lstart];
		parent->kids[cnt].qg_cnt = l-lstart;
		++cnt;
		lstart = l;
		pt = parent->first[l].nearestpt;
	    } else if ( pt!=parent->first[l].nearestpt )
		pt = parent->first[l].nearestpt;
	}
	if ( pt!=-1 ) {
	    sprintf( buffer, _("Point: %d (%d)"), pt, l-lstart );
	    parent->kids[cnt].name = copy(buffer);
	    parent->kids[cnt].parent = parent;
	    parent->kids[cnt].first = &parent->first[lstart];
	    parent->kids[cnt].qg_cnt = l-lstart;
	}
      break;
      case is_size_glyph_pt:	/* glyph */
	sc = NULL;
	cnt = 0;
	for ( l=0; l<parent->qg_cnt; ++l ) {
	    if ( sc!=parent->first[l].sc ) {
		++cnt;
		sc = parent->first[l].sc;
	    }
	}
	parent->kid_cnt = cnt;
	parent->kids = calloc(cnt,sizeof(struct qgnode));
	cnt = 0;
	lstart = 0;
	sc = NULL;
	for ( l=0; l<parent->qg_cnt; ++l ) {
	    if ( sc!=parent->first[l].sc && sc!=NULL ) {
		sprintf( buffer, "\"%.40s\" (%d)", sc->name, l-lstart );
		parent->kids[cnt].name = copy(buffer);
		parent->kids[cnt].parent = parent;
		parent->kids[cnt].first = &parent->first[lstart];
		parent->kids[cnt].qg_cnt = l-lstart;
		++cnt;
		lstart = l;
		sc = parent->first[l].sc;
	    } else if ( sc!=parent->first[l].sc )
		sc = parent->first[l].sc;
	}
	if ( sc!=NULL ) {
	    sprintf( buffer, "\"%.40s\" (%d)", sc->name, l-lstart );
	    parent->kids[cnt].name = copy(buffer);
	    parent->kids[cnt].parent = parent;
	    parent->kids[cnt].first = &parent->first[lstart];
	    parent->kids[cnt].qg_cnt = l-lstart;
	}
      break;
    }
}

static void QGDoSort(QGData *qg) {
    int pos, l, k, cnt, lstart;
    char buffer[200];

    pos = GGadgetGetFirstListSelectedItem(GWidgetGetControl(qg->gw,CID_Sort));
    qg->info_sort = (intptr_t) sorts[pos].userdata;

    pos = GGadgetGetFirstListSelectedItem(GWidgetGetControl(qg->gw,CID_GlyphSort));
    qg->glyph_sort = (intptr_t) glyphsorts[pos].userdata;

    kludge = qg;
    qsort(qg->qg,qg->cur,sizeof(QuestionableGrid),qg_sorter);

    qgnodeFree(&qg->list);
    memset(&qg->list,0,sizeof(struct qgnode));
    qg->list.open = true;
    qg->list.first = qg->qg;
    if ( qg->info_sort == is_glyph_size_pt || qg->info_sort == is_glyph_pt_size ) {
	SplineChar *sc = NULL;
	cnt = 0;
	for ( l=0; l<qg->cur; ++l ) {
	    if ( sc!=qg->qg[l].sc ) {
		++cnt;
		sc = qg->qg[l].sc;
	    }
	}
	qg->list.kid_cnt = cnt;
	qg->list.kids = calloc(cnt,sizeof(struct qgnode));
	cnt = 0;
	lstart = 0; sc=NULL;
	for ( l=0; l<qg->cur; ++l ) {
	    if ( sc!=qg->qg[l].sc && sc!=NULL ) {
		sprintf( buffer, "\"%.40s\" (%d)", sc->name, l-lstart );
		qg->list.kids[cnt].name = copy(buffer);
		qg->list.kids[cnt].parent = &qg->list;
		qg->list.kids[cnt].first = &qg->qg[lstart];
		qg->list.kids[cnt].qg_cnt = l-lstart;
		++cnt;
		lstart = l;
		sc = qg->qg[l].sc;
	    } else if ( sc!=qg->qg[l].sc )
		sc = qg->qg[l].sc;
	}
	if ( sc!=NULL ) {
	    sprintf( buffer, "\"%.40s\" (%d)", sc->name, l-lstart );
	    qg->list.kids[cnt].name = copy(buffer);
	    qg->list.kids[cnt].parent = &qg->list;
	    qg->list.kids[cnt].first = &qg->qg[lstart];
	    qg->list.kids[cnt].qg_cnt = l-lstart;
	}
    } else {
	int size = -1;
	cnt = 0;
	for ( l=0; l<qg->cur; ++l ) {
	    if ( size!=qg->qg[l].size ) {
		++cnt;
		size = qg->qg[l].size;
	    }
	}
	qg->list.kid_cnt = cnt;
	qg->list.kids = calloc(cnt,sizeof(struct qgnode));
	cnt = 0;
	lstart = 0; size=-1;
	for ( l=0; l<qg->cur; ++l ) {
	    if ( size!=qg->qg[l].size && size!=-1 ) {
		sprintf( buffer, _("Size: %d (%d)"), size, l-lstart );
		qg->list.kids[cnt].name = copy(buffer);
		qg->list.kids[cnt].parent = &qg->list;
		qg->list.kids[cnt].first = &qg->qg[lstart];
		qg->list.kids[cnt].qg_cnt = l-lstart;
		++cnt;
		lstart = l;
		size = qg->qg[l].size;
	    } else if ( size!=qg->qg[l].size )
		size = qg->qg[l].size;
	}
	if ( size!=-1 ) {
	    sprintf( buffer, _("Size: %d (%d)"), size, l-lstart );
	    qg->list.kids[cnt].name = copy(buffer);
	    qg->list.kids[cnt].parent = &qg->list;
	    qg->list.kids[cnt].first = &qg->qg[lstart];
	    qg->list.kids[cnt].qg_cnt = l-lstart;
	}
    }
    if ( qg->list.kid_cnt==1 )
	qg->list.kids[0].open = true;
    for ( k=0; k<qg->list.kid_cnt; ++k )
	QGSecondLevel(qg,&qg->list.kids[k]);
    QG_Remetric(qg);
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
    int l, y, depth;
    char buffer[200];
    GRect old, r;
    struct navigate where;
    struct qgnode *parent;

    GDrawPushClip(pixmap,&e->u.expose.rect,&old);
    r.width = r.height = qg->as;
    y = qg->as;
    memset(&where,0,sizeof(where));
    QG_FindLine(&qg->list,qg->loff_top,&where);
    GDrawSetFont(pixmap,qg->font);

    for ( l=0; l<qg->vlcnt && where.parent!=NULL; ++l ) {
	for ( parent=where.parent, depth= -2; parent!=NULL; parent=parent->parent, ++depth );
	if ( where.offset==-1 ) {
	    r.x = 2+depth*qg->fh;   r.y = y-qg->as+1;
	    GDrawDrawRect(pixmap,&r,MAIN_FOREGROUND);
	    GDrawDrawLine(pixmap,r.x+2,r.y+qg->as/2,r.x+qg->as-2,r.y+qg->as/2,
		    MAIN_FOREGROUND);
	    if ( !where.parent->open )
		GDrawDrawLine(pixmap,r.x+qg->as/2,r.y+2,r.x+qg->as/2,r.y+qg->as-2,
			MAIN_FOREGROUND);
	    GDrawDrawText8(pixmap,r.x+qg->fh,y,where.parent->name,-1, MAIN_FOREGROUND);
	} else {
	    QuestionableGrid *q = &where.parent->first[where.offset];
	    sprintf( buffer, _("\"%.40s\" size=%d point=%d (%d,%d) distance=%g"),
		    q->sc->name, q->size, q->nearestpt, q->x, q->y, q->distance );
	    GDrawDrawText8(pixmap,2+(depth+1)*qg->fh,y,buffer,-1, MAIN_FOREGROUND);
	}
	y += qg->fh;
	QG_NextLine(&where);
    }
    GDrawPopClip(pixmap,&old);
}

static void QGMouse( QGData *qg, GEvent *e) {
    int l = qg->loff_top + e->u.mouse.y/qg->fh;
    struct navigate where;

    if ( (e->type == et_mousedown) && (e->u.mouse.button==1)) {
	memset(&where,0,sizeof(where));
	QG_FindLine(&qg->list,l,&where);
	if ( where.parent==NULL )
return;
	if ( where.offset==-1 ) {
	    where.parent->open = !where.parent->open;
	    QG_Remetric(qg);
	    GDrawRequestExpose(qg->v,NULL,false);
return;
	} else {
	    QuestionableGrid *q = &where.parent->first[where.offset];
	    CharView *cv;
	    if ( qg->inprocess )
return;
	    cv = qg->cv;
	    if ( cv==NULL && qg->fv!=NULL ) {
		qg->inprocess = true;
		cv = qg->cv = CharViewCreate(q->sc,(FontView *) (qg->fv),qg->fv->map->backmap[q->sc->orig_pos]);
		if ( qg->layer == ly_fore ) {
		    cv->b.drawmode = dm_fore;
		} else {
		    cv->b.layerheads[dm_back] = &qg->sc->layers[qg->layer];
		    cv->b.drawmode = dm_back;
		}
		cv->qg = qg;
		qg->inprocess = false;
	    } else if ( qg->cv==NULL )
return;
	    else if ( qg->cv->b.sc != q->sc ) {
		CVChangeSC(qg->cv,q->sc);
	    }
	    cv->ft_pointsizex = cv->ft_pointsizey = q->size;
	    cv->ft_ppemy = cv->ft_ppemx = rint(q->size*qg->dpi/72.0);
	    cv->ft_dpi = qg->dpi;
	    cv->ft_depth = qg->depth;
	    cv->note_x = q->x; cv->note_y = q->y;
	    cv->show_ft_results = true; cv->showgrids = true;
	    CVGridFitChar(cv);
	}
    }
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
      default: break;
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
    int i, k;
    int as, ds, ld;
    static int sorts_translated = 0;

    if (!sorts_translated)
    {
        for (i=0; i<sizeof(sorts)/sizeof(sorts[0]); i++)
            sorts[i].text = (unichar_t *) _((char *) sorts[i].text);
        for (i=0; i<sizeof(glyphsorts)/sizeof(glyphsorts[0]); i++)
            glyphsorts[i].text = (unichar_t *) _((char *) glyphsorts[i].text);
        sorts_translated=1;
    }

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

    qg->font = validate_font.fi;
    GDrawWindowFontMetrics(gw,qg->font,&as,&ds,&ld);
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
    sorts[2].disabled = qg->fv==NULL;
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
    if ( qg->fv==NULL )
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
    GHVBoxSetExpandableRow(boxes[0].ret,1);
    GHVBoxSetExpandableCol(boxes[2].ret,0);
    GHVBoxSetPadding(boxes[2].ret,0,0);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxFitWindow(boxes[0].ret);

    QGDoSort(qg);

    GDrawSetVisible(gw,true);
    while ( !qg->done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);

    qgnodeFree(&qg->list);
    qg->gw = oldgw;
}
