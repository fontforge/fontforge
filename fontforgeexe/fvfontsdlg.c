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

#include "chardata.h"
#include "fontforgeui.h"
#include "fvfonts.h"
#include "gfile.h"
#include "splineutil.h"
#include "ustring.h"
#include "utype.h"

static void MergeAskFilename(FontView *fv,int preserveCrossFontKerning) {
    char *filename = GetPostScriptFontName(NULL,true,true);
    SplineFont *sf;
    char *eod, *fpt, *file, *full;

    if ( filename==NULL )
return;
    eod = strrchr(filename,'/');
    *eod = '\0';
    file = eod+1;
    do {
	fpt = strstr(file,"; ");
	if ( fpt!=NULL ) *fpt = '\0';
	full = malloc(strlen(filename)+1+strlen(file)+1);
	strcpy(full,filename); strcat(full,"/"); strcat(full,file);
	sf = LoadSplineFont(full,0);
	if ( sf!=NULL && sf->fv==NULL )
	    EncMapFree(sf->map);
	free(full);
	if ( sf==NULL )
	    /* Do Nothing */;
	else if ( sf->fv==(FontViewBase *) fv )
	    ff_post_error(_("Merging Problem"),_("Merging a font with itself achieves nothing"));
	else {
	    if ( preserveCrossFontKerning==-1 ) {
		char *buts[4];
		int ret;
		buts[0] = _("_Yes"); buts[1] = _("_No"); buts[2] = _("_Cancel"); buts[3]=NULL;
		ret = gwwv_ask(_("Kerning"),(const char **) buts,0,2,
			_("Do you want to retain kerning information from the selected font\nwhen one of the glyphs being kerned will come from the base font?"));
		if ( ret==2 ) {
		    free(filename);
return;
		}
		preserveCrossFontKerning = ret==0;
	    }
	    MergeFont((FontViewBase *) fv,sf,preserveCrossFontKerning);
	}
	file = fpt+2;
    } while ( fpt!=NULL );
    free(filename);
}

GTextInfo *BuildFontList(FontView *except) {
    FontView *fv;
    int cnt=0;
    GTextInfo *tf;

    for ( fv=fv_list; fv!=NULL; fv = (FontView *) (fv->b.next) )
	++cnt;
    tf = calloc(cnt+3,sizeof(GTextInfo));
    for ( fv=fv_list, cnt=0; fv!=NULL; fv = (FontView *) (fv->b.next) ) if ( fv!=except ) {
	tf[cnt].fg = tf[cnt].bg = COLOR_DEFAULT;
	tf[cnt].text = (unichar_t *) fv->b.sf->fontname;
	tf[cnt].text_is_1byte = true;
	++cnt;
    }
    tf[cnt++].line = true;
    tf[cnt].fg = tf[cnt].bg = COLOR_DEFAULT;
    tf[cnt].text_is_1byte = true;
    tf[cnt++].text = (unichar_t *) _("Other ...");
return( tf );
}

void TFFree(GTextInfo *tf) {
/*
    int i;

    for ( i=0; tf[i].text!=NULL || tf[i].line ; ++i )
	if ( !tf[i].text_in_resource )
	    free( tf[i].text );
*/
    free(tf);
}

struct mf_data {
    int done;
    FontView *fv;
    GGadget *other;
    GGadget *amount;
};
#define CID_Preserve	1001

static int MF_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct mf_data *d = GDrawGetUserData(gw);
	int i, index = GGadgetGetFirstListSelectedItem(d->other);
	int preserve = GGadgetIsChecked(GWidgetGetControl(gw,CID_Preserve));
	FontView *fv;
	for ( i=0, fv=fv_list; fv!=NULL; fv=(FontView *) (fv->b.next) ) {
	    if ( fv==d->fv )
	continue;
	    if ( i==index )
	break;
	    ++i;
	}
	if ( fv==NULL )
	    MergeAskFilename(d->fv,preserve);
	else
	    MergeFont((FontViewBase *) d->fv,fv->b.sf,preserve);
	d->done = true;
    }
return( true );
}

static int MF_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct mf_data *d = GDrawGetUserData(gw);
	d->done = true;
    }
return( true );
}

static int mv_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct mf_data *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

void FVMergeFonts(FontView *fv) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[7], *varray[9], *barray[8], boxes[4];
    GTextInfo label[7];
    struct mf_data d;
    char buffer[80];

    /* If there's only one font loaded, then it's the current one, and there's*/
    /*  no point asking the user if s/he wants to merge any of the loaded */
    /*  fonts, go directly to searching the disk */
    if ( fv_list==fv && fv_list->b.next==NULL )
	MergeAskFilename(fv,-1);
    else {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.is_dlg = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = _("Merge Fonts");
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,150));
	pos.height = GDrawPointsToPixels(NULL,88);
	gw = GDrawCreateTopWindow(NULL,&pos,mv_e_h,&d,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));
	memset(&boxes,0,sizeof(boxes));

	sprintf( buffer, _("Font to merge into %.20s"), fv->b.sf->fontname );
	label[0].text = (unichar_t *) buffer;
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; 
	gcd[0].gd.flags = gg_visible | gg_enabled;
	gcd[0].creator = GLabelCreate;
	varray[0] = &gcd[0]; varray[1] = NULL;

	gcd[1].gd.pos.x = 15; gcd[1].gd.pos.y = 21;
	gcd[1].gd.pos.width = 120;
	gcd[1].gd.flags = gg_visible | gg_enabled;
	gcd[1].gd.u.list = BuildFontList(fv);
	gcd[1].gd.label = &gcd[1].gd.u.list[0];
	gcd[1].gd.u.list[0].selected = true;
	gcd[1].creator = GListButtonCreate;
	varray[2] = &gcd[1]; varray[3] = NULL;

	label[2].text = (unichar_t *) _("Preserve cross-font kerning");
	label[2].text_is_1byte = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.pos.x = 12; gcd[2].gd.pos.y = 6; 
	gcd[2].gd.flags = gg_visible | gg_enabled | gg_cb_on;
	gcd[2].gd.cid = CID_Preserve;
	gcd[2].creator = GCheckBoxCreate;
	varray[4] = &gcd[2]; varray[5] = NULL;

	gcd[3].gd.pos.x = 15-3; gcd[3].gd.pos.y = 55-3;
	gcd[3].gd.pos.width = -1; gcd[3].gd.pos.height = 0;
	gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[3].text = (unichar_t *) _("_OK");
	label[3].text_is_1byte = true;
	label[3].text_in_resource = true;
	gcd[3].gd.mnemonic = 'O';
	gcd[3].gd.label = &label[3];
	gcd[3].gd.handle_controlevent = MF_OK;
	gcd[3].creator = GButtonCreate;

	gcd[4].gd.pos.x = -15; gcd[4].gd.pos.y = 55;
	gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
	gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[4].text = (unichar_t *) _("_Cancel");
	label[4].text_is_1byte = true;
	label[4].text_in_resource = true;
	gcd[4].gd.label = &label[4];
	gcd[4].gd.mnemonic = 'C';
	gcd[4].gd.handle_controlevent = MF_Cancel;
	gcd[4].creator = GButtonCreate;
	barray[0] = barray[2] = barray[3] = barray[4] = barray[6] = GCD_Glue; barray[7] = NULL;
	barray[1] = &gcd[3]; barray[5] = &gcd[4];

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = barray;
	boxes[2].creator = GHBoxCreate;
	varray[6] = &boxes[2]; varray[7] = NULL;
	varray[8] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = varray;
	boxes[0].creator = GHVGroupCreate;

	GGadgetsCreate(gw,boxes);
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
	GHVBoxFitWindow(boxes[0].ret);

	memset(&d,'\0',sizeof(d));
	d.other = gcd[1].ret;
	d.fv = fv;

	GWidgetHidePalettes();
	GDrawSetVisible(gw,true);
	while ( !d.done )
	    GDrawProcessOneEvent(NULL);
	GDrawDestroyWindow(gw);
	TFFree(gcd[1].gd.u.list);
    }
}

/******************************************************************************/
/* *************************** Font Interpolation *************************** */
/******************************************************************************/

static void InterAskFilename(FontView *fv, real amount) {
    char *filename = GetPostScriptFontName(NULL,false,true);
    SplineFont *sf;

    if ( filename==NULL )
return;
    sf = LoadSplineFont(filename,0);
    if ( sf!=NULL && sf->fv==NULL )
	EncMapFree(sf->map);
    free(filename);
    if ( sf==NULL )
return;
    FontViewCreate(InterpolateFont(fv->b.sf,sf,amount,fv->b.map->enc),false);
}

#define CID_Amount	1000
static double last_amount=50;

static int IF_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct mf_data *d = GDrawGetUserData(gw);
	int i, index = GGadgetGetFirstListSelectedItem(d->other);
	FontView *fv;
	int err=false;
	real amount;
	
	amount = GetReal8(gw,CID_Amount, _("Amount"),&err);
	if ( err )
return( true );
	last_amount = amount;
	for ( i=0, fv=fv_list; fv!=NULL; fv=(FontView *) (fv->b.next) ) {
	    if ( fv==d->fv )
	continue;
	    if ( i==index )
	break;
	    ++i;
	}
	if ( fv==NULL )
	    InterAskFilename(d->fv,last_amount/100.0);
	else
	    FontViewCreate(InterpolateFont(d->fv->b.sf,fv->b.sf,last_amount/100.0,d->fv->b.map->enc),false);
	d->done = true;
    }
return( true );
}

void FVInterpolateFonts(FontView *fv) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8];
    GTextInfo label[8];
    struct mf_data d;
    char buffer[80]; char buf2[30];

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Interpolate Fonts");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,200));
    pos.height = GDrawPointsToPixels(NULL,118);
    gw = GDrawCreateTopWindow(NULL,&pos,mv_e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    sprintf( buffer, _("Interpolating between %.20s and:"), fv->b.sf->fontname );
    label[0].text = (unichar_t *) buffer;
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; 
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].creator = GLabelCreate;

    gcd[1].gd.pos.x = 20; gcd[1].gd.pos.y = 21;
    gcd[1].gd.pos.width = 110;
    gcd[1].gd.flags = gg_visible | gg_enabled;
    gcd[1].gd.u.list = BuildFontList(fv);
    if ( gcd[1].gd.u.list[0].text!=NULL ) {
	gcd[1].gd.label = &gcd[1].gd.u.list[0];
	gcd[1].gd.u.list[0].selected = true;
    } else {
	gcd[1].gd.label = &gcd[1].gd.u.list[1];
	gcd[1].gd.u.list[1].selected = true;
	gcd[1].gd.flags = gg_visible;
    }
    gcd[1].creator = GListButtonCreate;

    sprintf( buf2, "%g", last_amount );
    label[2].text = (unichar_t *) buf2;
    label[2].text_is_1byte = true;
    gcd[2].gd.pos.x = 20; gcd[2].gd.pos.y = 51;
    gcd[2].gd.pos.width = 40;
    gcd[2].gd.flags = gg_visible | gg_enabled;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.cid = CID_Amount;
    gcd[2].creator = GTextFieldCreate;

    gcd[3].gd.pos.x = 5; gcd[3].gd.pos.y = 51+6;
    gcd[3].gd.flags = gg_visible | gg_enabled;
/* GT: The dialog looks like: */
/* GT:   Interpolating between <fontname> and: */
/* GT: <list of possible fonts> */
/* GT:   by  <50>% */
/* GT: So "by" means how much to interpolate. */
    label[3].text = (unichar_t *) _("by");
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].creator = GLabelCreate;

    gcd[4].gd.pos.x = 20+40+3; gcd[4].gd.pos.y = 51+6;
    gcd[4].gd.flags = gg_visible | gg_enabled;
    label[4].text = (unichar_t *) "%";
    label[4].text_is_1byte = true;
    gcd[4].gd.label = &label[4];
    gcd[4].creator = GLabelCreate;

    gcd[5].gd.pos.x = 15-3; gcd[5].gd.pos.y = 85-3;
    gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[5].text = (unichar_t *) _("_OK");
    label[5].text_is_1byte = true;
    label[5].text_in_resource = true;
    gcd[5].gd.mnemonic = 'O';
    gcd[5].gd.label = &label[5];
    gcd[5].gd.handle_controlevent = IF_OK;
    gcd[5].creator = GButtonCreate;

    gcd[6].gd.pos.x = -15; gcd[6].gd.pos.y = 85;
    gcd[6].gd.pos.width = -1; gcd[6].gd.pos.height = 0;
    gcd[6].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[6].text = (unichar_t *) _("_Cancel");
    label[6].text_is_1byte = true;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.mnemonic = 'C';
    gcd[6].gd.handle_controlevent = MF_Cancel;
    gcd[6].creator = GButtonCreate;

    GGadgetsCreate(gw,gcd);

    memset(&d,'\0',sizeof(d));
    d.other = gcd[1].ret;
    d.fv = fv;

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    TFFree(gcd[1].gd.u.list);
}
