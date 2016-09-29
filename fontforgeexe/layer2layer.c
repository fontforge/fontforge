/* Copyright (C) 2008-2012 by George Williams */
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
#include <ustring.h>
#include <gkeysym.h>

enum l2l_type { l2l_copy, l2l_compare };

typedef struct l2ld {
    GWindow gw;
    SplineFont *sf;
    CharView *cv;
    FontView *fv;
    enum l2l_type l2l;
    int done;
} L2LDlg;

#define CID_FromLayer	1000
#define CID_ToLayer	1001
#define CID_ClearOld	1002
#define CID_ErrorBound	1003

static GTextInfo *SFLayerList(SplineFont *sf,int def_layer) {
    GTextInfo *ret = calloc(sf->layer_cnt+1,sizeof(GTextInfo));
    int l;

    for ( l=0; l<sf->layer_cnt; ++l ) {
	ret[l].text = (unichar_t *) copy(sf->layers[l].name);
	ret[l].text_is_1byte = true;
	ret[l].userdata = (void *) (intpt) l;
    }
    ret[def_layer].selected = true;
return( ret );
}

static void _DoCVCopy(CharView *cv,int from,int to,int clear) {
    SCCopyLayerToLayer(cv->b.sc,from,to,clear);
}

static void RedoRefs(SplineFont *sf,SplineChar *sc,int layer) {
    RefChar *refs;

    sc->ticked2 = true;
    for ( refs=sc->layers[layer].refs; refs!=NULL; refs=refs->next ) {
	if ( refs->sc->ticked && !refs->sc->ticked2 )
	    RedoRefs(sf,refs->sc,layer);
	SCReinstanciateRefChar(sc,refs,layer);
    }
}

static void _DoFVCopy(FontView *fv,int from,int to,int clear) {
    SplineFont *sf = fv->b.sf;
    int gid, enc;
    SplineChar *sc;

    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc = sf->glyphs[gid])!=NULL ) {
	sc->ticked = sc->ticked2 = false;
    }
    for ( enc=0 ; enc<fv->b.map->enccount; ++enc ) {
	if ( fv->b.selected[enc] && (gid=fv->b.map->map[enc])!=-1 &&
		(sc = sf->glyphs[gid])!=NULL && !sc->ticked ) {
	    SCCopyLayerToLayer(sc,from,to,clear);
	    sc->ticked = true;
	}
    }
    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc = sf->glyphs[gid])!=NULL ) {
	if ( sc->ticked && !sc->ticked2 )
	    RedoRefs(sf,sc,to);
    }
}

static void _DoCVCompare(CharView *cv,int from,int to,double errbound) {
    if ( LayersSimilar(&cv->b.sc->layers[from],&cv->b.sc->layers[to],errbound))
	ff_post_notice(_("Match"),_("No significant differences found"));
    else
	ff_post_notice(_("Differ"),_("The layers do not match"));
}

static void _DoFVCompare(FontView *fv,int from,int to,double errbound) {
    SplineFont *sf = fv->b.sf;
    int gid, enc;
    SplineChar *sc;
    int first=-1;

    memset(fv->b.selected,0,fv->b.map->enccount);

    for ( enc=0 ; enc<fv->b.map->enccount; ++enc ) {
	if ( /*fv->b.selected[enc] &&*/ (gid=fv->b.map->map[enc])!=-1 &&
		(sc = sf->glyphs[gid])!=NULL && !sc->ticked ) {
	    if ( !LayersSimilar(&sc->layers[from],&sc->layers[to],errbound)) {
		fv->b.selected[enc] = true;
		if ( first==-1 )
		    first = enc;
	    }
	}
    }
    GDrawRequestExpose(fv->v,NULL,true);
    if ( first==-1 )
	ff_post_notice(_("Match"),_("No significant differences found"));
    else {
	ff_post_notice(_("Differ"),_("The layers do not match"));
	FVScrollToChar(fv,first);
	fv->end_pos = fv->pressed_pos = first;
	/*FVShowInfo(fv);*/
    }
}


static int L2L_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	L2LDlg *d = GDrawGetUserData(GGadgetGetWindow(g));
	int from, to, clear;
	int err=0;
	double errbound;

	from = GGadgetGetFirstListSelectedItem(GWidgetGetControl(d->gw,CID_FromLayer));
	to   = GGadgetGetFirstListSelectedItem(GWidgetGetControl(d->gw,CID_ToLayer));
	if ( d->l2l==l2l_copy ) {
	    clear = GGadgetIsChecked(GWidgetGetControl(d->gw,CID_ClearOld));
	    if ( d->cv )
		_DoCVCopy(d->cv,from,to,clear);
	    else
		_DoFVCopy(d->fv,from,to,clear);
	} else {
	    errbound = GetReal8(d->gw,CID_ErrorBound,_("Error Bound"),&err);
	    if ( err )
return( true );
	    if ( d->cv )
		_DoCVCompare(d->cv,from,to,errbound);
	    else
		_DoFVCompare(d->fv,from,to,errbound);
	}
	d->done = true;
    }
return( true );
}

static int L2L_Cancel(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	L2LDlg *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = true;
    }
return( true );
}

static int l2l_e_h(GWindow gw, GEvent *event) {

    if ( event->type==et_close ) {
	L2LDlg *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

static void Layer2Layer(CharView *cv,FontView *fv,enum l2l_type l2l,int def_layer) {
    L2LDlg d;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[17], *hvarray[34], *harray[5], *harray2[8], *hvarray2[7], boxes[7];
    GTextInfo label[17];
    int k,j;

    memset(&d,0,sizeof(d));
    d.cv = cv; d.fv = fv; d.l2l = l2l;
    if ( cv!=NULL ) d.sf = cv->b.sc->parent;
    else d.sf = fv->b.sf;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = l2l==l2l_copy ? _("Copy Layers") : _("Compare Layers");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,170));
    pos.height = GDrawPointsToPixels(NULL,178);
    d.gw = gw = GDrawCreateTopWindow(NULL,&pos,l2l_e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    k=j=0;
    label[k].text = (unichar_t *) (l2l==l2l_copy ? _("Copy one layer to another") : _("Compare two layers"));
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[j++] = &gcd[k-1]; hvarray[j++] = NULL;

    label[k].text = (unichar_t *) (l2l==l2l_copy ? _("From:") : _("Base:"));
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray2[0] = &gcd[k-1];

    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_FromLayer;
    gcd[k].gd.u.list = SFLayerList(d.sf,def_layer);
    gcd[k++].creator = GListButtonCreate;
    hvarray2[1] = &gcd[k-1]; hvarray2[2] = NULL;

    label[k].text = (unichar_t *) (l2l==l2l_copy ? _("To:") : _("Other:"));
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray2[3] = &gcd[k-1];

    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_ToLayer;
    gcd[k].gd.u.list = SFLayerList(d.sf,def_layer==ly_fore ? ly_back : ly_fore );
    gcd[k++].creator = GListButtonCreate;
    hvarray2[4] = &gcd[k-1]; hvarray2[5] = NULL; hvarray2[6] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = hvarray2;
    boxes[3].creator = GHVBoxCreate;
    hvarray[j++] = &boxes[3]; hvarray[j++] = NULL;

    if ( l2l==l2l_copy ) {
	label[k].text = (unichar_t *) _("Clear destination layer before copy");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_ClearOld;
	gcd[k].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[k++].creator = GCheckBoxCreate;
	hvarray[j++] = &gcd[k-1]; hvarray[j++] = NULL;
    } else {
	label[k].text = (unichar_t *) _("Allow errors of:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;
	harray[0] = &gcd[k-1];
    
	label[k].text = (unichar_t *) "1";
	label[k].text_is_1byte = true;
	gcd[k].gd.pos.width = 50;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_ErrorBound;
	gcd[k++].creator = GTextFieldCreate;
	harray[1] = &gcd[k-1];

	label[k].text = (unichar_t *) _("em units");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;
	harray[2] = &gcd[k-1]; harray[3] = GCD_Glue; harray[4] = NULL;

	boxes[4].gd.flags = gg_enabled|gg_visible;
	boxes[4].gd.u.boxelements = harray;
	boxes[4].creator = GHBoxCreate;
	hvarray[j++] = &boxes[4]; hvarray[j++] = NULL;
    }

    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = L2L_OK;
    gcd[k++].creator = GButtonCreate;
    harray2[0] = GCD_Glue; harray2[1] = &gcd[k-1]; harray2[2] = GCD_Glue; harray2[3] = GCD_Glue;

    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = L2L_Cancel;
    gcd[k++].creator = GButtonCreate;
    harray2[4] = GCD_Glue; harray2[5] = &gcd[k-1]; harray2[6] = GCD_Glue; harray2[7] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray2;
    boxes[2].creator = GHBoxCreate;
    hvarray[j++] = &boxes[2]; hvarray[j++] = NULL;
    hvarray[j++] = GCD_Glue; hvarray[j++] = NULL; hvarray[j++] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = hvarray;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    if ( l2l==l2l_compare )
	GHVBoxSetExpandableRow(boxes[4].ret,gb_expandglue);

    GTextInfoListFree(gcd[2].gd.u.list);
    GTextInfoListFree(gcd[4].gd.u.list);
    
    GHVBoxFitWindow(boxes[0].ret);

    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);

    GDrawDestroyWindow(gw);
}

void CVCopyLayerToLayer(CharView *cv) {
    Layer2Layer(cv,NULL,l2l_copy,CVLayer((CharViewBase *) cv));
}

void FVCopyLayerToLayer(FontView *fv) {
    Layer2Layer(NULL,fv,l2l_copy,ly_fore);
}

void CVCompareLayerToLayer(CharView *cv) {
    Layer2Layer(cv,NULL,l2l_compare,CVLayer((CharViewBase *) cv));
}

void FVCompareLayerToLayer(FontView *fv) {
    Layer2Layer(NULL,fv,l2l_compare,ly_fore);
}
