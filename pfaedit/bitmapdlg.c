/* Copyright (C) 2000,2001 by George Williams */
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
#include "gwidget.h"
#include "ustring.h"
#include <math.h>

typedef struct createbitmapdata {
    unsigned int done: 1;
    FontView *fv;
    SplineChar *sc;
    int isavail;
    int which;
    GWindow gw;
} CreateBitmapData;

enum { bd_all, bd_selected, bd_current };
static GTextInfo which[] = {
    { (unichar_t *) "All Characters", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "Selected Characters", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "Current Character", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { NULL }};
static int lastwhich = bd_selected;

static void RemoveBDFWindows(BDFFont *bdf) {
    int i;
    BitmapView *bv, *next;

    for ( i=0; i<bdf->charcnt; ++i ) if ( bdf->chars[i]!=NULL ) {
	for ( bv = bdf->chars[i]->views; bv!=NULL; bv=next ) {
	    next = bv->next;
	    GDrawDestroyWindow(bv->gw);
	}
    }
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    /* Just in case... */
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
}

void SFOrderBitmapList(SplineFont *sf) {
    BDFFont *bdf, *prev, *next;
    BDFFont *bdf2, *prev2;

    for ( prev = NULL, bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	for ( prev2=NULL, bdf2=bdf->next; bdf2!=NULL; bdf2 = bdf2->next ) {
	    if ( bdf->pixelsize>bdf2->pixelsize ) {
		if ( prev==NULL )
		    sf->bitmaps = bdf2;
		else
		    prev->next = bdf2;
		if ( prev2==NULL ) {
		    bdf->next = bdf2->next;
		    bdf2->next = bdf;
		} else {
		    next = bdf->next;
		    bdf->next = bdf2->next;
		    bdf2->next = next;
		    prev2->next = bdf;
		}
		next = bdf;
		bdf = bdf2;
		bdf2 = next;
	    }
	    prev2 = bdf2;
	}
	prev = bdf;
    }
}

static void FVRemoveUnwantedBitmaps(FontView *fv,double *sizes) {
    BDFFont *bdf, *prev, *next;
    int i;

    for ( prev = NULL, bdf=fv->sf->bitmaps; bdf!=NULL; bdf = next ) {
	next = bdf->next;
	for ( i=0; sizes[i]!=0 && sizes[i]!=bdf->pixelsize; ++i );
	if ( sizes[i]==0 ) {
	    /* To be deleted */
	    if ( prev==NULL )
		fv->sf->bitmaps = next;
	    else
		prev->next = next;
	    if ( fv->show==bdf )
		FVShowFilled(fv);
	    RemoveBDFWindows(bdf);
	    BDFFontFree(bdf);
	    fv->sf->changed = true;
	} else {
	    sizes[i] = -sizes[i];		/* don't need to create it */
	    prev = bdf;
	}
    }
}

static void FVFigureBitmaps(FontView *fv,double *sizes) {
    BDFFont *bdf;
    int i, first;

    FVRemoveUnwantedBitmaps(fv,sizes);

    first = true;
    for ( i=0; sizes[i]!=0 ; ++i ) if ( sizes[i]>0 ) {
	if ( first && autohint_before_rasterize )
	    SplineFontAutoHint(fv->sf);
	bdf = SplineFontRasterize(fv->sf,sizes[i],true);
	bdf->next = fv->sf->bitmaps;
	fv->sf->bitmaps = bdf;
	fv->sf->changed = true;
	first = false;
    }

    /* Order the list */
    SFOrderBitmapList(fv->sf);
}

static void FVScaleBitmaps(FontView *fv,double *sizes) {
    BDFFont *bdf;
    int i, cnt=0;
    static unichar_t sb[] = { 'S','c','a','l','i','n','g',' ','B','i','t','m','a','p','s',  '\0' };

    for ( i=0; sizes[i]!=0 ; ++i ) if ( sizes[i]>0 )
	++cnt;
    GProgressStartIndicator(10,sb,sb,NULL,cnt,1);

    FVRemoveUnwantedBitmaps(fv,sizes);

    for ( i=0; sizes[i]!=0 ; ++i ) if ( sizes[i]>0 ) {
	bdf = BitmapFontScaleTo(fv->show,sizes[i]);
	bdf->next = fv->sf->bitmaps;
	fv->sf->bitmaps = bdf;
	fv->sf->changed = true;
	if ( !GProgressNext())
    break;
    }
    GProgressEndIndicator();

    /* Order the list */
    SFOrderBitmapList(fv->sf);
}

static void ReplaceBDFC(SplineFont *sf,double *sizes,int enc) {
    BDFFont *bdf;
    BDFChar *bdfc, temp;
    int i;
    BitmapView *bv;
    int first=true;

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	for ( i=0; sizes[i]!=0 && sizes[i]!=bdf->pixelsize; ++i );
	if ( sizes[i]!=0 ) {
	    if ( first && autohint_before_rasterize && 
		    sf->chars[enc]->changedsincelasthinted &&
		    !sf->chars[enc]->manualhints )
		SplineCharAutoHint(sf->chars[enc],true);
	    first = false;
	    bdfc = SplineCharRasterize(sf->chars[enc],bdf->pixelsize);
	    if ( bdf->chars[enc]==NULL )
		bdf->chars[enc] = bdfc;
	    else {
		temp = *(bdf->chars[enc]);
		*bdf->chars[enc] = *bdfc;
		*bdfc = temp;
		bdf->chars[enc]->views = bdfc->views;
		bdfc->views = NULL;
		BDFCharFree(bdfc);
		for ( bv = bdf->chars[enc]->views; bv!=NULL; bv=bv->next ) {
		    GDrawRequestExpose(bv->v,NULL,false);
		    /* Mess with selection?????!!!!! */
		}
	    }
	}
    }
}

static int FVRegenBitmaps(CreateBitmapData *bd,double *sizes) {
    FontView *fv = bd->fv;
    SplineFont *sf = fv->sf;
    int i;
    BDFFont *bdf;

    for ( i=0; sizes[i]!=0; ++i ) {
	for ( bdf = sf->bitmaps; bdf!=NULL && bdf->pixelsize!=sizes[i]; bdf=bdf->next );
	if ( bdf==NULL ) {
	    unichar_t *temp;
	    char buffer[100];
	    sprintf(buffer,"Attempt to regenerate a pixel size that has not been created (%d)", (int) sizes[i]);
	    temp=uc_copy(buffer);
	    GWidgetPostNotice(temp,temp);
	    free(temp);
return( false );
	}
    }
    if ( bd->which==bd_current && bd->sc!=NULL )
	ReplaceBDFC(sf,sizes,bd->sc->enc);
    else {
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL &&
		    (fv->selected[i] || bd->which == bd_all)) {
	    ReplaceBDFC(sf,sizes,i);
	}
    }
    sf->changed = true;
    if ( fv->show!=fv->filled ) {
	for ( i=0; sizes[i]!=0 && sizes[i]!=fv->show->pixelsize; ++i );
	if ( sizes[i]!=0 )
	    GDrawRequestExpose(fv->v,NULL,false );
    }
return( true );
}

#define CID_Which	1001
#define CID_Pixel	1002
#define CID_75		1003
#define CID_100		1004

static double *ParseList(GWindow gw, int cid,int *err, int final) {
    const unichar_t *val = _GGadgetGetTitle(GWidgetGetControl(gw,cid)), *pt;
    unichar_t *end, *end2;
    int i;
    double *sizes;

    *err = false;
    end2 = NULL;
    for ( i=1, pt = val; (end = u_strchr(pt,',')) || (end2=u_strchr(pt,' ')); ++i ) {
	if ( end!=NULL && end2!=NULL ) {
	    if ( end2<end ) end = end2;
	} else if ( end2!=NULL )
	    end = end2;
	pt = end+1;
	end2 = NULL;
    }
    sizes = galloc((i+1)*sizeof(double));

    for ( i=0, pt = val; *pt!='\0' ; ) {
	sizes[i]=u_strtod(pt,&end);
	if ( sizes[i]>0 ) ++i;
	if ( *end!=' ' && *end!=',' && *end!='\0' ) {
	    free(sizes);
	    if ( final )
		Protest("Pixel List");
	    *err = true;
return( NULL );
	}
	while ( *end==' ' || *end==',' ) ++end;
	pt = end;
    }
    sizes[i] = 0;

    if ( cid==CID_75 )
	for ( i=0; sizes[i]!=0; ++i )
	    sizes[i] = rint(sizes[i]*75/72);
    else if ( cid==CID_100 )
	for ( i=0; sizes[i]!=0; ++i )
	    sizes[i] = rint(sizes[i]*100/72);
    else if ( final )
	for ( i=0; sizes[i]!=0; ++i )
	    sizes[i] = rint(sizes[i]);
return( sizes );
}

static int CB_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	int err;
	CreateBitmapData *bd = GDrawGetUserData(GGadgetGetWindow(g));
	double *sizes = ParseList(bd->gw, CID_Pixel,&err, true);
	if ( err )
return( true );
	bd->done = true;
	if ( bd->isavail && bd->fv->sf->onlybitmaps && bd->fv->sf->bitmaps!=NULL )
	    FVScaleBitmaps(bd->fv,sizes);
	else if ( bd->isavail )
	    FVFigureBitmaps(bd->fv,sizes);
	else {
	    bd->which = GGadgetGetFirstListSelectedItem(GWidgetGetControl(bd->gw,CID_Which));
	    if ( !FVRegenBitmaps(bd,sizes))
		bd->done = false;
	    else
		lastwhich = bd->which;
	}
	free(sizes);
    }
return( true );
}

static int CB_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CreateBitmapData *bd = GDrawGetUserData(GGadgetGetWindow(g));
	bd->done = true;
    }
return( true );
}

static unichar_t *GenText(double *sizes,double scale) {
    int i;
    char *cret, *pt;
    unichar_t *uret;

    for ( i=0; sizes[i]!=0; ++i );
    pt = cret = galloc(i*10);
    for ( i=0; sizes[i]!=0; ++i ) {
	if ( pt!=cret ) *pt++ = ',';
	sprintf(pt,"%.1f",sizes[i]*scale );
	pt += strlen(pt);
	if ( pt[-1]=='0' && pt[-2]=='.' ) {
	    pt -= 2;
	    *pt = '\0';
	}
    }
    *pt = '\0';
    uret = uc_copy(cret);
    free(cret);
return( uret );
}

static int CB_TextChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	CreateBitmapData *bd = GDrawGetUserData(GGadgetGetWindow(g));
	int cid = (int) GGadgetGetCid(g);
	unichar_t *val;
	int err=false;
	double *sizes = ParseList(bd->gw,cid,&err,false);
	int ncid;
	if ( err )
return( true );
	for ( ncid=CID_Pixel; ncid<=CID_100; ++ncid ) if ( ncid!=cid ) {
	    val = GenText(sizes,ncid==CID_Pixel?1.0:ncid==CID_75?72./75.:72./100.);
	    GGadgetSetTitle(GWidgetGetControl(bd->gw,ncid),val);
	    free(val);
	}
	free(sizes);
    }
return( true );
}

static int bd_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	CreateBitmapData *bd = GDrawGetUserData(gw);
	bd->done = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

void BitmapDlg(FontView *fv,SplineChar *sc, int isavail) {
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[13];
    GTextInfo label[13];
    CreateBitmapData bd;
    static unichar_t bitmapsavail[] = { 'B','i','t','m','a','p','s',' ','A','v','a','i','l','a','b','l','e','.', '.', '.',  '\0' };
    static unichar_t regenbitmaps[] = { 'R','e','g','e','r','a','t','e', ' ', 'B','i','t','m','a','p','s','.', '.', '.',  '\0' };
    int i,j,y;
    double *sizes;
    BDFFont *bdf;

    bd.fv = fv;
    bd.sc = sc;
    bd.isavail = isavail;
    bd.done = false;

    for ( bdf=fv->sf->bitmaps, i=0; bdf!=NULL; bdf=bdf->next, ++i );
    if ( i==0 && isavail )
	i = 2;
    sizes = galloc((i+1)*sizeof(double));
    for ( bdf=fv->sf->bitmaps, i=0; bdf!=NULL; bdf=bdf->next, ++i )
	sizes[i] = bdf->pixelsize;
/*
    if ( i==0 && isavail ) {
	sizes[i++] = 12.5;
	sizes[i++] = 17;
    }
*/
    sizes[i] = 0;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = isavail ? bitmapsavail : regenbitmaps;
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,190);
    pos.height = GDrawPointsToPixels(NULL,202);
    bd.gw = GDrawCreateTopWindow(NULL,&pos,bd_e_h,&bd,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    if ( isavail ) {
	label[0].text = (unichar_t *) "The list of current pixel bitmap sizes";
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5; 
	gcd[0].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[0].creator = GLabelCreate;

	label[1].text = (unichar_t *) " Removing a size will delete it.";
	label[1].text_is_1byte = true;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.pos.x = 5; gcd[1].gd.pos.y = 5+13;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].creator = GLabelCreate;

	if ( fv->sf->onlybitmaps && fv->sf->bitmaps!=NULL )
	    label[2].text = (unichar_t *) " Adding a size will create it by scaling.";
	else
	    label[2].text = (unichar_t *) " Adding a size will create it.";
	label[2].text_is_1byte = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = 5+26;
	gcd[2].gd.flags = gg_enabled|gg_visible;
	gcd[2].creator = GLabelCreate;
	j = 3; y = 5+39+3;
    } else {
	label[0].text = (unichar_t *) "Specify bitmap sizes to be regenerated";
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5; 
	gcd[0].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[0].creator = GLabelCreate;

	if ( lastwhich==bd_current && sc==NULL )
	    lastwhich = bd_selected;
	gcd[1].gd.label = &which[lastwhich];
	gcd[1].gd.u.list = which;
	gcd[1].gd.pos.x = 5; gcd[1].gd.pos.y = 5+13;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].gd.cid = CID_Which;
	gcd[1].creator = GListButtonCreate;
	which[bd_current].disabled = sc==NULL;
	which[lastwhich].selected = true;

	j=2; y = 5+13+28;
    }

    label[j].text = (unichar_t *) "Point sizes on a 75 dpi screen";
    label[j].text_is_1byte = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = y;
    gcd[j].gd.flags = gg_enabled|gg_visible|gg_cb_on;
    gcd[j++].creator = GLabelCreate;
    y += 13;

    label[j].text = GenText(sizes,72./75.);
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 15; gcd[j].gd.pos.y = y;
    gcd[j].gd.pos.width = 170;
    gcd[j].gd.flags = gg_enabled|gg_visible|gg_cb_on;
    gcd[j].gd.cid = CID_75;
    gcd[j].gd.handle_controlevent = CB_TextChange;
    gcd[j++].creator = GTextFieldCreate;
    y += 26;

    label[j].text = (unichar_t *) "Point sizes on a 100 dpi screen";
    label[j].text_is_1byte = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = y;
    gcd[j].gd.flags = gg_enabled|gg_visible|gg_cb_on;
    gcd[j++].creator = GLabelCreate;
    y += 13;

    label[j].text = GenText(sizes,72./100.);
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 15; gcd[j].gd.pos.y = y;
    gcd[j].gd.pos.width = 170;
    gcd[j].gd.flags = gg_enabled|gg_visible|gg_cb_on;
    gcd[j].gd.cid = CID_100;
    gcd[j].gd.handle_controlevent = CB_TextChange;
    gcd[j++].creator = GTextFieldCreate;
    y += 26;

    label[j].text = (unichar_t *) "Pixel sizes";
    label[j].text_is_1byte = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = y;
    gcd[j].gd.flags = gg_enabled|gg_visible|gg_cb_on;
    gcd[j++].creator = GLabelCreate;
    y += 13;

    label[j].text = GenText(sizes,1.);
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 15; gcd[j].gd.pos.y = y;
    gcd[j].gd.pos.width = 170;
    gcd[j].gd.flags = gg_enabled|gg_visible|gg_cb_on;
    gcd[j].gd.cid = CID_Pixel;
    gcd[j].gd.handle_controlevent = CB_TextChange;
    gcd[j++].creator = GTextFieldCreate;
    y += 26;

    gcd[j].gd.pos.x = 2; gcd[j].gd.pos.y = 2;
    gcd[j].gd.pos.width = pos.width-4;
    gcd[j].gd.pos.height = pos.height-4;
    gcd[j].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
    gcd[j++].creator = GGroupCreate;

    gcd[j].gd.pos.x = 20-3; gcd[j].gd.pos.y = 202-32-3;
    gcd[j].gd.pos.width = 55; gcd[j].gd.pos.height = 0;
    gcd[j].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[j].text = (unichar_t *) "OK";
    label[j].text_is_1byte = true;
    gcd[j].gd.mnemonic = 'O';
    gcd[j].gd.label = &label[j];
    gcd[j].gd.handle_controlevent = CB_OK;
    gcd[j++].creator = GButtonCreate;

    gcd[j].gd.pos.x = 190-55-20; gcd[j].gd.pos.y = 202-32;
    gcd[j].gd.pos.width = 55; gcd[j].gd.pos.height = 0;
    gcd[j].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[j].text = (unichar_t *) "Cancel";
    label[j].text_is_1byte = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.mnemonic = 'C';
    gcd[j].gd.handle_controlevent = CB_Cancel;
    gcd[j++].creator = GButtonCreate;

    GGadgetsCreate(bd.gw,gcd);
    which[lastwhich].selected = false;

    GWidgetHidePalettes();
    GDrawSetVisible(bd.gw,true);
    while ( !bd.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(bd.gw);
}
