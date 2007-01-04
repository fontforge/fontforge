/* Copyright (C) 2000-2007 by George Williams */
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
#include <math.h>
#include <sys/types.h>
#include <dirent.h>
#include "sd.h"
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include <gkeysym.h>
#include <ustring.h>
#include <utype.h>


static unichar_t wildimg[] = { '*', '.', '{',
#ifndef _NO_LIBUNGIF
'g','i','f',',',
#endif
#ifndef _NO_LIBPNG
'p','n','g',',',
#endif
#ifndef _NO_LIBTIFF
't','i','f','f',',',
't','i','f',',',
#endif
'x','p','m',',', 'x','b','m',',', 'b','m','p', '}', '\0' };
static unichar_t wildtemplate[] = { '{','u','n','i',',','u',',','c','i','d',',','e','n','c','}','[','0','-','9','a','-','f','A','-','F',']','*', '.', '{',
#ifndef _NO_LIBUNGIF
'g','i','f',',',
#endif
#ifndef _NO_LIBPNG
'p','n','g',',',
#endif
#ifndef _NO_LIBTIFF
't','i','f','f',',',
't','i','f',',',
#endif
'x','p','m',',', 'x','b','m',',', 'b','m','p', '}', '\0' };
/* Hmm. Mac seems to use the extension 'art' for eps files sometimes */
static unichar_t wildepstemplate[] = { '{','u','n','i',',','u',',','c','i','d',',','e','n','c','}','[','0','-','9','a','-','f','A','-','F',']','*', '.', '{', 'p','s',',', 'e','p','s',',','a','r','t','}',  0 };
static unichar_t wildsvgtemplate[] = { '{','u','n','i',',','u',',','c','i','d',',','e','n','c','}','[','0','-','9','a','-','f','A','-','F',']','*', '.', 's', 'v','g',  0 };
static unichar_t wildgliftemplate[] = { '{','u','n','i',',','u',',','c','i','d',',','e','n','c','}','[','0','-','9','a','-','f','A','-','F',']','*', '.', 'g', 'l','i','f',  0 };
static unichar_t wildps[] = { '*', '.', '{', 'p','s',',', 'e','p','s',',', 'a','r','t','}', '\0' };
static unichar_t wildsvg[] = { '*', '.', 's','v','g',  '\0' };
static unichar_t wildglif[] = { '*', '.', 'g','l','i','f',  '\0' };
static unichar_t wildfig[] = { '*', '.', '{', 'f','i','g',',','x','f','i','g','}',  '\0' };
static unichar_t wildbdf[] = { '*', '.', 'b', 'd','{', 'f', ',','f','.','g','z',',','f','.','Z',',','f','.','b','z','2','}',  '\0' };
static unichar_t wildpcf[] = { '*', '.', 'p', '{', 'c',',','m','}','{', 'f', ',','f','.','g','z',',','f','.','Z',',','f','.','b','z','2','}',  '\0' };
static unichar_t wildttf[] = { '*', '.', '{', 't', 't','f',',','o','t','f',',','o','t','b',',','t','t','c','}',  '\0' };
static unichar_t wildpk[] = { '*', '{', 'p', 'k', ',', 'g', 'f', '}',  '\0' };		/* pk fonts can have names like cmr10.300pk, not a normal extension */
static unichar_t wildmac[] = { '*', '{', 'b', 'i', 'n', ',', 'h', 'q', 'x', ',', 'd','f','o','n','t', '}',  '\0' };
static unichar_t wildwin[] = { '*', '{', 'f', 'o', 'n', ',', 'f', 'n', 't', '}',  '\0' };
static unichar_t wildpalm[] = { '*', 'p', 'd', 'b',  '\0' };
static unichar_t *wildchr[] = { wildimg, wildps,
#ifndef _NO_LIBXML
wildsvg,
wildglif,
#endif
wildfig };
static unichar_t *wildfnt[] = { wildbdf, wildttf, wildpk, wildpcf, wildmac,
wildwin, wildpalm,
wildimg, wildtemplate, wildps, wildepstemplate
#ifndef _NO_LIBXML
, wildsvg, wildsvgtemplate
, wildglif, wildgliftemplate
#endif
};

#define PSSF_Width 220
#define PSSF_Height 165

static int PSSF_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate )
	*(int *) GDrawGetUserData(GGadgetGetWindow(g)) = true;
return( true );
}

static int psstrokeflags_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	*(int *) GDrawGetUserData(gw) = true;
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("filemenu.html#type3-import");
return( true );
	}
return( false );
    }
return( true );
}

enum psstrokeflags PsStrokeFlagsDlg(void) {
    static enum psstrokeflags oldflags = sf_correctdir|sf_removeoverlap/*|sf_handle_eraser*/;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[11];
    GTextInfo label[11];
    int done = false;
    int k, rm_k, he_k, cd_k;

    if ( no_windowing_ui )
return( oldflags );

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("PS Interpretion");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,PSSF_Width));
    pos.height = GDrawPointsToPixels(NULL,PSSF_Height);
    gw = GDrawCreateTopWindow(NULL,&pos,psstrokeflags_e_h,&done,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    k = 0;
/* GT: The following strings should be concatenated together, the result */
/* GT: translated, and then broken into lines by hand. I'm sure it would */
/* GT: be better to specify this all as one string, but my widgets won't support */
/* GT: that */
    label[k].text = (unichar_t *) _("FontForge has some bugs in its remove overlap");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = 6;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) _("function which may cause you problems, so");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+13;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) _("I give you the option of turning it off.");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+13;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) _("Leave it on if possible though, it is useful.");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+13;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) "";
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+13;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;

    cd_k = k;
    label[k].text = (unichar_t *) _("_Correct Direction");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+15;
    gcd[k].gd.flags = gg_enabled | gg_visible | (oldflags&sf_correctdir?gg_cb_on:0);
    gcd[k++].creator = GCheckBoxCreate;

    rm_k = k;
    label[k].text = (unichar_t *) _("Cleanup Self Intersect");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+15;
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup |
	    (oldflags&sf_removeoverlap?gg_cb_on:0);
    gcd[k].gd.popup_msg = (unichar_t *) _("When FontForge detects that an expanded stroke will self-intersect,\nthen setting this option will cause it to try to make things nice\nby removing the intersections");
    gcd[k++].creator = GCheckBoxCreate;

    he_k = k;
    label[k].text = (unichar_t *) _("Handle Erasers");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+15;
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup |
	    (oldflags&sf_handle_eraser?gg_cb_on:0);
    gcd[k].gd.popup_msg = (unichar_t *) _("Certain programs use pens with white ink as erasers\nIf you select (blacken) this checkbox, FontForge will\nattempt to simulate that.");
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = (PSSF_Width-GIntGetResource(_NUM_Buttonsize))/2; gcd[k].gd.pos.y = PSSF_Height-34;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = PSSF_OK;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 1; gcd[k].gd.pos.y = 1;
    gcd[k].gd.pos.width = PSSF_Width-2; gcd[k].gd.pos.height = PSSF_Height-2;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    GDrawSetVisible(gw,true);

    while ( !done )
	GDrawProcessOneEvent(NULL);

    /* This dlg can't be cancelled */
    oldflags = 0;
    if ( GGadgetIsChecked(gcd[cd_k].ret) )
	oldflags |= sf_correctdir;
    if ( GGadgetIsChecked(gcd[rm_k].ret) )
	oldflags |= sf_removeoverlap;
    if ( GGadgetIsChecked(gcd[he_k].ret) )
	oldflags |= sf_handle_eraser;
    GDrawDestroyWindow(gw);
return( oldflags );
}
#else
enum psstrokeflags PsStrokeFlagsDlg(void) {
    static enum psstrokeflags oldflags = sf_correctdir|sf_removeoverlap/*|sf_handle_eraser*/;
return( oldflags );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

#ifdef FONTFORGE_CONFIG_TYPE3
void SCAppendEntityLayers(SplineChar *sc, Entity *ent) {
    int cnt, pos;
    Entity *e, *enext;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    Layer *old = sc->layers;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

    for ( e=ent, cnt=0; e!=NULL; e=e->next, ++cnt );
    pos = sc->layer_cnt;
    if ( cnt==0 )
return;
    EntityDefaultStrokeFill(ent);

    sc->layers = grealloc(sc->layers,(sc->layer_cnt+cnt)*sizeof(Layer));
    for ( pos = sc->layer_cnt, e=ent; e!=NULL ; e=enext, ++pos ) {
	enext = e->next;
	LayerDefault(&sc->layers[pos]);
	sc->layers[pos].splines = NULL;
	sc->layers[pos].refs = NULL;
	sc->layers[pos].images = NULL;
	if ( e->type == et_splines ) {
	    sc->layers[pos].dofill = e->u.splines.fill.col != 0xffffffff;
	    sc->layers[pos].dostroke = e->u.splines.stroke.col != 0xffffffff;
	    if ( !sc->layers[pos].dofill && !sc->layers[pos].dostroke )
		sc->layers[pos].dofill = true;		/* If unspecified, assume an implied fill in BuildGlyph */
	    sc->layers[pos].fill_brush.col = e->u.splines.fill.col==0xffffffff ?
		    COLOR_INHERITED : e->u.splines.fill.col;
	    sc->layers[pos].stroke_pen.brush.col = e->u.splines.stroke.col==0xffffffff ? COLOR_INHERITED : e->u.splines.stroke.col;
	    sc->layers[pos].stroke_pen.width = e->u.splines.stroke_width;
	    sc->layers[pos].stroke_pen.linejoin = e->u.splines.join;
	    sc->layers[pos].stroke_pen.linecap = e->u.splines.cap;
	    memcpy(sc->layers[pos].stroke_pen.trans, e->u.splines.transform,
		    4*sizeof(real));
	    sc->layers[pos].splines = e->u.splines.splines;
	} else if ( e->type == et_image ) {
	    ImageList *ilist = chunkalloc(sizeof(ImageList));
	    struct _GImage *base = e->u.image.image->list_len==0?
		    e->u.image.image->u.image:e->u.image.image->u.images[0];
	    sc->layers[pos].images = ilist;
	    sc->layers[pos].dofill = base->image_type==it_mono && base->trans!=-1;
	    sc->layers[pos].fill_brush.col = e->u.image.col==0xffffffff ?
		    COLOR_INHERITED : e->u.image.col;
	    ilist->image = e->u.image.image;
	    ilist->xscale = e->u.image.transform[0];
	    ilist->yscale = e->u.image.transform[3];
	    ilist->xoff = e->u.image.transform[4];
	    ilist->yoff = e->u.image.transform[5];
	    ilist->bb.minx = ilist->xoff;
	    ilist->bb.maxy = ilist->yoff;
	    ilist->bb.maxx = ilist->xoff + base->width*ilist->xscale;
	    ilist->bb.miny = ilist->yoff - base->height*ilist->yscale;
	}
	free(e);
    }
    sc->layer_cnt += cnt;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    SCMoreLayers(sc,old);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
}
#endif

void SCImportPSFile(SplineChar *sc,int layer,FILE *ps,int doclear,int flags) {
    SplinePointList *spl, *espl;
    SplineSet **head;

    if ( ps==NULL )
return;
#ifdef FONTFORGE_CONFIG_TYPE3
    if ( sc->parent->multilayer && layer>ly_back ) {
	SCAppendEntityLayers(sc, EntityInterpretPS(ps));
    } else
#endif
    {
	spl = SplinePointListInterpretPS(ps,flags,sc->parent->strokedfont);
	if ( spl==NULL ) {
	    gwwv_post_error( _("Too Complex or Bad"), _("I'm sorry this file is too complex for me to understand (or is erroneous, or is empty)") );
return;
	}
	if ( sc->parent->order2 )
	    spl = SplineSetsConvertOrder(spl,true);
	for ( espl=spl; espl->next!=NULL; espl = espl->next );
	if ( layer==ly_grid )
	    head = &sc->parent->grid.splines;
	else {
	    SCPreserveLayer(sc,layer,false);
	    head = &sc->layers[layer].splines;
	}
	if ( doclear ) {
	    SplinePointListsFree(*head);
	    *head = NULL;
	}
	espl->next = *head;
	*head = spl;
    }
    SCCharChangedUpdate(sc);
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void ImportPS(CharView *cv,char *path) {
    FILE *ps = fopen(path,"r");

    if ( ps==NULL )
return;
    SCImportPSFile(cv->sc,CVLayer(cv),ps,false,-1);
    fclose(ps);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static void SCImportPS(SplineChar *sc,int layer,char *path,int doclear, int flags) {
    FILE *ps = fopen(path,"r");

    if ( ps==NULL )
return;
    SCImportPSFile(sc,layer,ps,doclear,flags);
    fclose(ps);
}

#ifndef _NO_LIBXML
void SCImportSVG(SplineChar *sc,int layer,char *path,char *memory, int memlen, int doclear) {
    SplinePointList *spl, *espl, **head;

#ifdef FONTFORGE_CONFIG_TYPE3
    if ( sc->parent->multilayer && layer>ly_back ) {
	SCAppendEntityLayers(sc, EntityInterpretSVG(path,memory,memlen,sc->parent->ascent+sc->parent->descent,
		sc->parent->ascent));
    } else
#endif
    {
	spl = SplinePointListInterpretSVG(path,memory,memlen,sc->parent->ascent+sc->parent->descent,
		sc->parent->ascent,sc->parent->strokedfont);
	for ( espl = spl; espl!=NULL && espl->first->next==NULL; espl=espl->next );
	if ( espl!=NULL )
	    if ( espl->first->next->order2!=sc->parent->order2 )
		spl = SplineSetsConvertOrder(spl,sc->parent->order2);
	if ( spl==NULL ) {
	    gwwv_post_error(_("Too Complex or Bad"),_("I'm sorry this file is too complex for me to understand (or is erroneous)"));
return;
	}
	for ( espl=spl; espl->next!=NULL; espl = espl->next );
	if ( layer==ly_grid )
	    head = &sc->parent->grid.splines;
	else {
	    SCPreserveLayer(sc,layer,false);
	    head = &sc->layers[layer].splines;
	}
	if ( doclear ) {
	    SplinePointListsFree(*head);
	    *head = NULL;
	}
	espl->next = *head;
	*head = spl;
    }
    SCCharChangedUpdate(sc);
}

# ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void ImportSVG(CharView *cv,char *path) {
    SCImportSVG(cv->sc,CVLayer(cv),path,NULL,0,false);
}
# endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

void SCImportGlif(SplineChar *sc,int layer,char *path,char *memory, int memlen, int doclear) {
    SplinePointList *spl, *espl, **head;

    spl = SplinePointListInterpretGlif(path,memory,memlen,sc->parent->ascent+sc->parent->descent,
	    sc->parent->ascent,sc->parent->strokedfont);
    for ( espl = spl; espl!=NULL && espl->first->next==NULL; espl=espl->next );
    if ( espl!=NULL )
	if ( espl->first->next->order2!=sc->parent->order2 )
	    spl = SplineSetsConvertOrder(spl,sc->parent->order2);
    if ( spl==NULL ) {
	gwwv_post_error(_("Too Complex or Bad"),_("I'm sorry this file is too complex for me to understand (or is erroneous)"));
return;
    }
    for ( espl=spl; espl->next!=NULL; espl = espl->next );
    if ( layer==ly_grid )
	head = &sc->parent->grid.splines;
    else {
	SCPreserveLayer(sc,layer,false);
	head = &sc->layers[layer].splines;
    }
    if ( doclear ) {
	SplinePointListsFree(*head);
	*head = NULL;
    }
    espl->next = *head;
    *head = spl;

    SCCharChangedUpdate(sc);
}

# ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void ImportGlif(CharView *cv,char *path) {
    SCImportGlif(cv->sc,CVLayer(cv),path,NULL,0,false);
}
# endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
#endif

/**************************** Fig File Import *********************************/

static BasePoint *slurppoints(FILE *fig,SplineFont *sf,int cnt ) {
    BasePoint *bps = galloc((cnt+1)*sizeof(BasePoint));	/* spline code may want to add another point */
    int x, y, i, ch;
    real scale = sf->ascent/(8.5*1200.0);
    real ascent = 11*1200*sf->ascent/(sf->ascent+sf->descent);

    for ( i = 0; i<cnt; ++i ) {
	fscanf(fig,"%d %d", &x, &y );
	bps[i].x = x*scale;
	bps[i].y = (ascent-y)*scale;
    }
    while ((ch=getc(fig))!='\n' && ch!=EOF);
return( bps );
}

static SplineSet *slurpcolor(FILE *fig,SplineChar *sc, SplineSet *sofar) {
    int ch;
    while ((ch=getc(fig))!='\n' && ch!=EOF);
return( sofar );
}

static SplineSet *slurpcompoundguts(FILE *fig,SplineChar *sc, SplineSet *sofar);

static SplineSet * slurpcompound(FILE *fig,SplineChar *sc, SplineSet *sofar) {
    int ch;

    fscanf(fig, "%*d %*d %*d %*d" );
    while ((ch=getc(fig))!='\n' && ch!=EOF);
    sofar = slurpcompoundguts(fig,sc,sofar);
return( sofar );
}

static SplinePoint *ArcSpline(SplinePoint *sp,float sa,SplinePoint *ep,float ea,
	float cx, float cy, float r) {
    double len;
    double ss, sc, es, ec;

    ss = sin(sa); sc = cos(sa); es = sin(ea); ec = cos(ea);
    if ( ep==NULL )
	ep = SplinePointCreate(cx+r*ec, cy+r*es);
    len = ((ea-sa)/(3.1415926535897932/2)) * r * .552;

    sp->nextcp.x = sp->me.x - len*ss; sp->nextcp.y = sp->me.y + len*sc;
    ep->prevcp.x = ep->me.x + len*es; ep->prevcp.y = ep->me.y - len*ec;
    sp->nonextcp = ep->noprevcp = false;
    SplineMake3(sp,ep);
return( ep );
}

static SplineSet * slurparc(FILE *fig,SplineChar *sc, SplineSet *sofar) {
    int ch;
    int sub, dir, fa, ba;	/* 0 clockwise, 1 counter */
    float cx, cy, r, sa, ea, ma;
    float sx,sy,ex,ey;
    int _sx,_sy,_ex,_ey;
    SplinePoint *sp, *ep;
    SplinePointList *spl;
    real scale = sc->parent->ascent/(8.5*1200.0);
    real ascent = 11*1200*sc->parent->ascent/(sc->parent->ascent+sc->parent->descent);

    fscanf(fig, "%d %*d %*d %*d %*d %*d %*d %*d %*f %*d %d %d %d %f %f %d %d %*d %*d %d %d",
	    &sub, &dir, &fa, &ba, &cx, &cy, &_sx, &_sy, &_ex, &_ey );
    while ((ch=getc(fig))!='\n' && ch!=EOF);
    /* I ignore arrow lines */
    if ( fa )
	while ((ch=getc(fig))!='\n' && ch!=EOF);
    if ( ba )
	while ((ch=getc(fig))!='\n' && ch!=EOF);
    sx = _sx*scale; sy = (ascent-_sy)*scale; ex = _ex*scale; ey=(ascent-_ey)*scale; cx*=scale; cy=(ascent-cy)*scale;
    r = sqrt( (sx-cx)*(sx-cx) + (sy-cy)*(sy-cy) );
    sa = atan2(sy-cy,sx-cx);
    ea = atan2(ey-cy,ex-cx);

    spl = chunkalloc(sizeof(SplinePointList));
    spl->next = sofar;
    spl->first = sp = SplinePointCreate(sx,sy);
    spl->last = ep = SplinePointCreate(ex,ey);

    if ( dir==0 ) {	/* clockwise */
	if ( ea>sa ) ea -= 2*3.1415926535897932;
	ma=ceil(sa/(3.1415926535897932/2)-1)*(3.1415926535897932/2);
	if ( RealNearish( sa,ma )) ma -= (3.1415926535897932/2);
	while ( ma > ea ) {
	    sp = ArcSpline(sp,sa,NULL,ma,cx,cy,r);
	    sa = ma;
	    ma -= (3.1415926535897932/2);
	}
	sp = ArcSpline(sp,sa,ep,ea,cx,cy,r);
    } else {		/* counterclockwise */
	if ( ea<sa ) ea += 2*3.1415926535897932;
	ma=floor(sa/(3.1415926535897932/2)+1)*(3.1415926535897932/2);
	if ( RealNearish( sa,ma )) ma += (3.1415926535897932/2);
	while ( ma < ea ) {
	    sp = ArcSpline(sp,sa,NULL,ma,cx,cy,r);
	    sa = ma;
	    ma += (3.1415926535897932/2);
	}
	sp = ArcSpline(sp,sa,ep,ea,cx,cy,r);
    }
return( spl );
}

static SplineSet * slurpelipse(FILE *fig,SplineChar *sc, SplineSet *sofar) {
    int ch;
    int sub, dir, cx, cy, rx, ry;
    float angle;
    SplinePointList *spl;
    SplinePoint *sp;
    real dcx,dcy,drx,dry;
    SplineFont *sf = sc->parent;
    real scale = sf->ascent/(8.5*1200.0);
    real ascent = 11*1200*sf->ascent/(sf->ascent+sf->descent);
    /* I ignore the angle */

    fscanf(fig, "%d %*d %*d %*d %*d %*d %*d %*d %*f %d %f %d %d %d %d %*d %*d %*d %*d",
	    &sub, &dir, &angle, &cx, &cy, &rx, &ry );
    while ((ch=getc(fig))!='\n' && ch!=EOF);

    dcx = cx*scale; dcy = (ascent-cy)*scale;
    drx = rx*scale; dry = ry*scale;

    spl = chunkalloc(sizeof(SplinePointList));
    spl->next = sofar;
    spl->first = sp = chunkalloc(sizeof(SplinePoint));
    sp->me.x = dcx; sp->me.y = dcy+dry;
	sp->nextcp.x = sp->me.x + .552*drx; sp->nextcp.y = sp->me.y;
	sp->prevcp.x = sp->me.x - .552*drx; sp->prevcp.y = sp->me.y;
    spl->last = sp = chunkalloc(sizeof(SplinePoint));
    sp->me.x = dcx+drx; sp->me.y = dcy;
	sp->nextcp.x = sp->me.x; sp->nextcp.y = sp->me.y - .552*dry;
	sp->prevcp.x = sp->me.x; sp->prevcp.y = sp->me.y + .552*dry;
    SplineMake3(spl->first,sp);
    sp = chunkalloc(sizeof(SplinePoint));
    sp->me.x = dcx; sp->me.y = dcy-dry;
	sp->nextcp.x = sp->me.x - .552*drx; sp->nextcp.y = sp->me.y;
	sp->prevcp.x = sp->me.x + .552*drx; sp->prevcp.y = sp->me.y;
    SplineMake3(spl->last,sp);
    spl->last = sp;
    sp = chunkalloc(sizeof(SplinePoint));
    sp->me.x = dcx-drx; sp->me.y = dcy;
	sp->nextcp.x = sp->me.x; sp->nextcp.y = sp->me.y + .552*dry;
	sp->prevcp.x = sp->me.x; sp->prevcp.y = sp->me.y - .552*dry;
    SplineMake3(spl->last,sp);
    SplineMake3(sp,spl->first);
    spl->last = spl->first;
return( spl );
}

static SplineSet * slurppolyline(FILE *fig,SplineChar *sc, SplineSet *sofar) {
    int ch;
    int sub, cnt, fa, ba, radius;	/* radius of roundrects (sub==4) */
    BasePoint *bps;
    BasePoint topleft, bottomright;
    SplinePointList *spl=NULL;
    SplinePoint *sp;
    int i;

    fscanf(fig, "%d %*d %*d %*d %*d %*d %*d %*d %*f %*d %*d %d %d %d %d",
	    &sub, &radius, &fa, &ba, &cnt );
    /* sub==1 => polyline, 2=>box, 3=>polygon, 4=>arc-box, 5=>imported eps bb */
    while ((ch=getc(fig))!='\n' && ch!=EOF);
    /* I ignore arrow lines */
    if ( fa )
	while ((ch=getc(fig))!='\n' && ch!=EOF);
    if ( ba )
	while ((ch=getc(fig))!='\n' && ch!=EOF);
    bps = slurppoints(fig,sc->parent,cnt);
    if ( sub==5 )		/* skip picture line */
	while ((ch=getc(fig))!='\n' && ch!=EOF);
    else {
	if ( sub!=1 && bps[cnt-1].x==bps[0].x && bps[cnt-1].y==bps[0].y )
	    --cnt;
	spl = chunkalloc(sizeof(SplinePointList));
	if ( cnt==4 && sub==4/*arc-box*/ && radius!=0 ) {
	    SplineFont *sf = sc->parent;
	    real scale = sf->ascent/(8.5*80.0), r = radius*scale;	/* radii are scaled differently */
	    if ( bps[0].x>bps[2].x ) {
		topleft.x = bps[2].x;
		bottomright.x = bps[0].x;
	    } else {
		topleft.x = bps[0].x;
		bottomright.x = bps[2].x;
	    }
	    if ( bps[0].y<bps[2].y ) {
		topleft.y = bps[2].y;
		bottomright.y = bps[0].y;
	    } else {
		topleft.y = bps[0].y;
		bottomright.y = bps[2].y;
	    }
	    spl->first = SplinePointCreate(topleft.x,topleft.y-r); spl->first->pointtype = pt_tangent;
	    spl->first->nextcp.y += .552*r; spl->first->nonextcp = false;
	    spl->last = sp = SplinePointCreate(topleft.x+r,topleft.y); sp->pointtype = pt_tangent;
	    sp->prevcp.x -= .552*r; sp->noprevcp = false;
	    SplineMake3(spl->first,sp);
	    sp = SplinePointCreate(bottomright.x-r,topleft.y); sp->pointtype = pt_tangent;
	    sp->nextcp.x += .552*r; sp->nonextcp = false;
	    SplineMake3(spl->last,sp); spl->last = sp;
	    sp = SplinePointCreate(bottomright.x,topleft.y-r); sp->pointtype = pt_tangent;
	    sp->prevcp.y += .552*r; sp->noprevcp = false;
	    SplineMake3(spl->last,sp); spl->last = sp;
	    sp = SplinePointCreate(bottomright.x,bottomright.y+r); sp->pointtype = pt_tangent;
	    sp->nextcp.y -= .552*r; sp->nonextcp = false;
	    SplineMake3(spl->last,sp); spl->last = sp;
	    sp = SplinePointCreate(bottomright.x-r,bottomright.y); sp->pointtype = pt_tangent;
	    sp->prevcp.x += .552*r; sp->noprevcp = false;
	    SplineMake3(spl->last,sp); spl->last = sp;
	    sp = SplinePointCreate(topleft.x+r,bottomright.y); sp->pointtype = pt_tangent;
	    sp->nextcp.x -= .552*r; sp->nonextcp = false;
	    SplineMake3(spl->last,sp); spl->last = sp;
	    sp = SplinePointCreate(topleft.x,bottomright.y+r); sp->pointtype = pt_tangent;
	    sp->prevcp.y -= .552*r; sp->noprevcp = false;
	    SplineMake3(spl->last,sp); spl->last = sp;
	} else {
	    for ( i=0; i<cnt; ++i ) {
		sp = chunkalloc(sizeof(SplinePoint));
		sp->me = sp->nextcp = sp->prevcp = bps[i];
		sp->nonextcp = sp->noprevcp = true;
		sp->pointtype = pt_corner;
		if ( spl->first==NULL )
		    spl->first = sp;
		else
		    SplineMake3(spl->last,sp);
		spl->last = sp;
	    }
	}
	if ( sub!=1 ) {
	    SplineMake3(spl->last,spl->first);
	    spl->last = spl->first;
	}
	spl->next = sc->layers[ly_fore].splines;
	spl->next = sofar;
    }
    free(bps);
return( spl );
}

/* http://dev.acm.org/pubs/citations/proceedings/graph/218380/p377-blanc/ */
/*  X-Splines: a spline model designed for the end-user */
/*		by Carole Blanc & Christophe Schlick */
/* Also based on the helpful code fragment by Andreas Baerentzen */
/*  http://lin1.gk.dtu.dk/home/jab/software.html */

struct xspline {
    int n;		/* total number of control points */
    BasePoint *cp;	/* an array of n control points */
    real *s;		/* an array of n tension values */
    /* for a closed spline cp[0]==cp[n-1], but we may still need to wrap a bit*/
    unsigned int closed: 1;
};

static real g(real u, real q, real p) {
return( u * (q + u * (2*q + u *( 10-12*q+10*p + u * ( 2*p+14*q-15 + u*(6-5*q-p))))) );
}

static real h(real u, real q) {
    /* The paper says that h(-1)==0, but the definition of h they give */
    /*  doesn't do that. But if we negate the x^5 term it all works */
    /*  (works for the higher derivatives too) */
return( q*u * (1 + u * (2 - u * u * (u+2))) );
}

static void xsplineeval(BasePoint *ret,real t, struct xspline *xs) {
    /* By choosing t to range between [0,n-1] we set delta in the article to 1*/
    /*  and may therefore ignore it */
#if 0
    if ( t>=0 && t<=xs->n-1 ) {
#endif

	/* For any value of t there are four possible points that might be */
	/*  influencing things. These are cp[k], cp[k+1], cp[k+2], cp[k+3] */
	/*  where k+1<=t<k+2 */
	int k = floor(t-1);
	int k0, k1, k2, k3;
	/* now we need to find the points near us (on the + side of cp[k] & */
	/*  cp[k-1] and the - side of cp[k+2] & cp[k+3]) where the blending */
	/*  function becomes 0. This depends on the tension values */
	/* For negative tension values it doesn't happen, the curve itself */
	/*  is changed */
	real Tk0 = k+1 + (xs->s[k+1]>0?xs->s[k+1]:0);
	real Tk1 = k+2 + (xs->s[k+2]>0?xs->s[k+2]:0);
	real Tk2 = k+1 - (xs->s[k+1]>0?xs->s[k+1]:0);
	real Tk3 = k+2 - (xs->s[k+2]>0?xs->s[k+2]:0);
	/* Now each blending function has a "p" value that describes its shape*/
	real p0 = 2*(k-Tk0)*(k-Tk0);
	real p1 = 2*(k+1-Tk1)*(k+1-Tk1);
	real p2 = 2*(k+2-Tk2)*(k+2-Tk2);
	real p3 = 2*(k+3-Tk3)*(k+3-Tk3);
	/* and each negative tension blending function has a "q" value */
	real q0 = xs->s[k+1]<0?-xs->s[k+1]/2:0;
	real q1 = xs->s[k+2]<0?-xs->s[k+2]/2:0;
	real q2 = q0;
	real q3 = q1;
	/* the function f for positive s is the same as g if q==0 */
	real A0, A1, A2, A3;
	if ( t<=Tk0 )
	    A0 = g( (t-Tk0)/(k-Tk0), q0, p0);
	else if ( q0>0 )
	    A0 = h( (t-Tk0)/(k-Tk0), q0 );
	else
	    A0 = 0;
	A1 = g( (t-Tk1)/(k+1-Tk1), q1, p1);
	A2 = g( (t-Tk2)/(k+2-Tk2), q2, p2);
	if ( t>=Tk3 )
	    A3 = g( (t-Tk3)/(k+3-Tk3), q3, p3);
	else if ( q3>0 )
	    A3 = h( (t-Tk3)/(k+3-Tk3), q3 );
	else
	    A3 = 0;
	k0 = k; k1=k+1; k2=k+2; k3=k+3;
	if ( k<0 ) { k0=xs->n-2; if ( !xs->closed ) A0 = 0; }
	if ( k3>=xs->n ) { k3 -= xs->n; if ( !xs->closed ) A3 = 0; }
	if ( k2>=xs->n ) { k2 -= xs->n; if ( !xs->closed ) A2 = 0; }
	ret->x = A0*xs->cp[k0].x + A1*xs->cp[k1].x + A2*xs->cp[k2].x + A3*xs->cp[k3].x;
	ret->y = A0*xs->cp[k0].y + A1*xs->cp[k1].y + A2*xs->cp[k2].y + A3*xs->cp[k3].y;
	ret->x /= (A0+A1+A2+A3);
	ret->y /= (A0+A1+A2+A3);
#if 0
    } else
	ret->x = ret->y = 0;
#endif
}

static void AdjustTs(TPoint *mids,SplinePoint *from, SplinePoint *to) {
    real len=0, sofar;
    real lens[8];
    int i;

    lens[0] = sqrt((mids[0].x-from->me.x)*(mids[0].x-from->me.x) +
		    (mids[0].y-from->me.y)*(mids[0].y-from->me.y));
    lens[7] = sqrt((mids[6].x-to->me.x)*(mids[6].x-to->me.x) +
		    (mids[6].y-to->me.y)*(mids[6].y-to->me.y));
    for ( i=1; i<7; ++i )
	lens[i] = sqrt((mids[i].x-mids[i-1].x)*(mids[i].x-mids[i-1].x) +
			(mids[i].y-mids[i-1].y)*(mids[i].y-mids[i-1].y));
    for ( len=0, i=0; i<8; ++i )
	len += lens[i];
    for ( sofar=0, i=0; i<7; ++i ) {
	sofar += lens[i];
	mids[i].t = sofar/len;
    }
}

static SplineSet *ApproximateXSpline(struct xspline *xs,int order2) {
    int i, j;
    real t;
    TPoint mids[7];
    SplineSet *spl = chunkalloc(sizeof(SplineSet));
    SplinePoint *sp;

    spl->first = spl->last = chunkalloc(sizeof(SplinePoint));
    xsplineeval(&spl->first->me,0,xs);
    spl->first->pointtype = ( xs->s[0]==0 )?pt_corner:pt_curve;
    for ( i=0; i<xs->n-1; ++i ) {
	if ( i==xs->n-2 && xs->closed )
	    sp = spl->first;
	else {
	    sp = chunkalloc(sizeof(SplinePoint));
	    sp->pointtype = ( xs->s[i+1]==0 )?pt_corner:pt_curve;
	    xsplineeval(&sp->me,i+1,xs);
	}
	for ( j=0, t=1./8; j<sizeof(mids)/sizeof(mids[0]); ++j, t+=1./8 ) {
	    xsplineeval((BasePoint *) &mids[j],i+t,xs);
	    mids[j].t = t;
	}
	AdjustTs(mids,spl->last,sp);
	ApproximateSplineFromPoints(spl->last,sp,mids,sizeof(mids)/sizeof(mids[0]),order2);
	SPAverageCps(spl->last);
	spl->last = sp;
    }
    if ( !xs->closed ) {
	spl->first->noprevcp = spl->last->nonextcp = true;
	spl->first->prevcp = spl->first->me;
	spl->last->nextcp = spl->last->me;
    } else
	SPAverageCps(spl->first);
return( spl );
}

static SplineSet * slurpspline(FILE *fig,SplineChar *sc, SplineSet *sofar) {
    int ch;
    int sub, cnt, fa, ba;
    SplinePointList *spl;
    struct xspline xs;
    int i;

    fscanf(fig, "%d %*d %*d %*d %*d %*d %*d %*d %*f %*d %d %d %d",
	    &sub, &fa, &ba, &cnt );
    while ((ch=getc(fig))!='\n' && ch!=EOF);
    /* I ignore arrow lines */
    if ( fa )
	while ((ch=getc(fig))!='\n' && ch!=EOF);
    if ( ba )
	while ((ch=getc(fig))!='\n' && ch!=EOF);
    xs.n = cnt;
    xs.cp = slurppoints(fig,sc->parent,cnt);
    xs.s = galloc((cnt+1)*sizeof(real));
    xs.closed = (sub&1);
    for ( i=0; i<cnt; ++i )
#if defined( FONTFORGE_CONFIG_USE_LONGDOUBLE )
	fscanf(fig,"%Lf",&xs.s[i]);
#elif defined( FONTFORGE_CONFIG_USE_DOUBLE )
	fscanf(fig,"%lf",&xs.s[i]);
#else
	fscanf(fig,"%f",&xs.s[i]);
#endif
    /* the spec says that the last point of a closed path will duplicate the */
    /* first, but it doesn't seem to */
    if ( xs.closed && ( !RealNear(xs.cp[cnt-1].x,xs.cp[0].x) ||
			!RealNear(xs.cp[cnt-1].y,xs.cp[0].y) )) {
	xs.n = ++cnt;
	xs.cp[cnt-1] = xs.cp[0];
	xs.s[cnt-1] = xs.s[0];
    }
    spl = ApproximateXSpline(&xs,sc->parent->order2);

    free(xs.cp);
    free(xs.s);

    spl->next = sofar;
return( spl );
}

static SplineSet *slurpcompoundguts(FILE *fig,SplineChar *sc,SplineSet *sofar) {
    int oc;
    int ch;

    while ( 1 ) {
	fscanf(fig,"%d",&oc);
	if ( feof(fig) || oc==-6 )
return(sofar);
	switch ( oc ) {
	  case 6:
	    sofar = slurpcompound(fig,sc,sofar);
	  break;
	  case 0:
	    sofar = slurpcolor(fig,sc,sofar);
	  break;
	  case 1:
	    sofar = slurpelipse(fig,sc,sofar);
	  break;
	  case 5:
	    sofar = slurparc(fig,sc,sofar);
	  break;
	  case 2:
	    sofar = slurppolyline(fig,sc,sofar);
	  break;
	  case 3:
	    sofar = slurpspline(fig,sc,sofar);
	  break;
	  case 4:
	  default:
	    /* Text is also only one line */
	    while ( (ch=getc(fig))!='\n' && ch!=EOF );
	  break;
	}
    }
return( sofar );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void ImportFig(CharView *cv,char *path) {
    FILE *fig;
    char buffer[100];
    SplineSet *spl, *espl;
    int i;

    fig = fopen(path,"r");
    if ( fig==NULL ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	gwwv_post_error(_("Can't find the file"),_("Can't find the file"));
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Can't find the file"),_("Can't find the file"));
#endif
return;
    }
    if ( fgets(buffer,sizeof(buffer),fig)==NULL || strcmp(buffer,"#FIG 3.2\n")!=0 ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	gwwv_post_error(_("Bad xfig file"),_("Bad xfig file"));
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Bad xfig file"),_("Bad xfig file"));
#endif
	fclose(fig);
return;
    }
    /* skip the header, it isn't interesting */
    for ( i=0; i<8; ++i )
	fgets(buffer,sizeof(buffer),fig);
    spl = slurpcompoundguts(fig,cv->sc,NULL);
    if ( spl!=NULL ) {
	CVPreserveState(cv);
	if ( cv->sc->parent->order2 )
	    spl = SplineSetsConvertOrder(spl,true);
	for ( espl=spl; espl->next!=NULL; espl=espl->next );
	espl->next = cv->layerheads[cv->drawmode]->splines;
	cv->layerheads[cv->drawmode]->splines = spl;
	CVCharChangedUpdate(cv);
    }
    fclose(fig);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

/************************** Normal Image Import *******************************/

GImage *ImageAlterClut(GImage *image) {
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    GClut *clut;

    if ( base->image_type!=it_mono ) {
	/* png b&w images come through as indexed, not mono */
	if ( base->clut!=NULL && base->clut->clut_len==2 ) {
	    GImage *new = GImageCreate(it_mono,base->width,base->height);
	    struct _GImage *nbase = new->u.image;
	    int i,j;
	    memset(nbase->data,0,nbase->height*nbase->bytes_per_line);
	    for ( i=0; i<base->height; ++i ) for ( j=0; j<base->width; ++j )
		if ( base->data[i*base->bytes_per_line+j] )
		    nbase->data[i*nbase->bytes_per_line+(j>>3)] |= (0x80>>(j&7));
	    nbase->clut = base->clut;
	    base->clut = NULL;
	    nbase->trans = base->trans;
	    GImageDestroy(image);
	    image = new;
	    base = nbase;
	} else
return( image );
    }

    clut = base->clut;
    if ( clut==NULL ) {
	clut=base->clut = gcalloc(1,sizeof(GClut));
	clut->clut_len = 2;
	clut->clut[0] = 0x808080;
	if ( !no_windowing_ui )
	    clut->clut[1] = default_background;
	else
	    clut->clut[1] = 0xb0b0b0;
	clut->trans_index = 1;
	base->trans = 1;
    } else if ( base->trans!=-1 ) {
	clut->clut[!base->trans] = 0x808080;
    } else if ( clut->clut[0]<clut->clut[1] ) {
	clut->clut[0] = 0x808080;
	clut->trans_index = 1;
	base->trans = 1;
    } else {
	clut->clut[1] = 0x808080;
	clut->trans_index = 0;
	base->trans = 0;
    }
return( image );
}

void SCInsertImage(SplineChar *sc,GImage *image,real scale,real yoff,real xoff,
	int layer) {
    ImageList *im;

    SCPreserveLayer(sc,layer,false);
    im = galloc(sizeof(ImageList));
    im->image = image;
    im->xoff = xoff;
    im->yoff = yoff;
    im->xscale = im->yscale = scale;
    im->selected = true;
    im->next = sc->layers[layer].images;
    im->bb.minx = im->xoff; im->bb.maxy = im->yoff;
    im->bb.maxx = im->xoff + GImageGetWidth(im->image)*im->xscale;
    im->bb.miny = im->yoff - GImageGetHeight(im->image)*im->yscale;
    sc->layers[layer].images = im;
    sc->parent->onlybitmaps = false;
    SCOutOfDateBackground(sc);
    SCCharChangedUpdate(sc);
}

void SCAddScaleImage(SplineChar *sc,GImage *image,int doclear, int layer) {
    double scale;

    image = ImageAlterClut(image);
    scale = (sc->parent->ascent+sc->parent->descent)/(real) GImageGetHeight(image);
    if ( doclear )
	ImageListsFree(sc->layers[layer].images); sc->layers[layer].images = NULL;
    SCInsertImage(sc,image,scale,sc->parent->ascent,0,layer);
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void ImportImage(CharView *cv,char *path) {
    GImage *image;
    int layer;

    image = GImageRead(path);
    if ( image==NULL ) {
	gwwv_post_error(_("Bad image file"),_("Bad image file: %.100s"), path);
return;
    }
    layer = ly_back;
    if ( cv->sc->parent->multilayer && cv->drawmode!=dm_grid )
	layer = cv->drawmode-dm_back + ly_back;
    SCAddScaleImage(cv->sc,image,false,layer);
}

static int BVImportImage(BitmapView *bv,char *path) {
    GImage *image;
    struct _GImage *base;
    int tot;
    uint8 *pt, *end;
    BDFChar *bc = bv->bc;
    int i, j;

    image = GImageRead(path);
    if ( image==NULL ) {
	gwwv_post_error(_("Bad image file"),_("Bad image file: %.100s"), path);
return(false);
    }
    base = image->list_len==0?image->u.image:image->u.images[0];
    BCPreserveState(bc);
    BCFlattenFloat(bc);
    free(bc->bitmap);
    bc->xmin = bc->ymin = 0;
    bc->xmax = base->width-1; bc->ymax = base->height-1;
    if ( !bc->byte_data && base->image_type==it_mono ) {
	bc->bitmap = base->data;
	bc->bytes_per_line = base->bytes_per_line;

	/* Sigh. Bitmaps use a different defn of set than images do. make it consistant */
	tot = bc->bytes_per_line*(bc->ymax-bc->ymin+1);
	for ( pt = bc->bitmap, end = pt+tot; pt<end; *pt++ ^= 0xff );

	base->data = NULL;
    } else if ( base->image_type==it_mono /* && byte_data */) {
	int set = (1<<BDFDepth(bv->bdf))-1;
	bc->bytes_per_line = base->width;
	bc->bitmap = gcalloc(base->height,base->width);
	for ( i=0; i<base->height; ++i ) for ( j=0; j<base->width; ++j ) {
	    if ( !(base->data[i*base->bytes_per_line+(j>>3)]&(0x80>>(j&7))) )
		bc->bitmap[i*bc->bytes_per_line+j] = set;
	}
    } else if ( !bc->byte_data && base->image_type==it_true ) {
	bc->bytes_per_line = (base->width>>3)+1;
	bc->bitmap = gcalloc(base->height,bc->bytes_per_line);
	for ( i=0; i<base->height; ++i ) for ( j=0; j<base->width; ++j ) {
	    int col = ((Color *) (base->data+i*base->bytes_per_line))[j];
	    col = (3*COLOR_RED(col)+6*COLOR_GREEN(col)+COLOR_BLUE(col));
	    if ( col<=5*256 )
		bc->bitmap[i*bc->bytes_per_line+(j>>3)] |= (0x80>>(j&7));
	}
    } else if ( /* byte_data && */ base->image_type==it_true ) {
	int div = 255/((1<<BDFDepth(bv->bdf))-1);
	bc->bytes_per_line = base->width;
	bc->bitmap = gcalloc(base->height,base->width);
	for ( i=0; i<base->height; ++i ) for ( j=0; j<base->width; ++j ) {
	    int col = ((Color *) (base->data+i*base->bytes_per_line))[j];
	    col = 255-(3*COLOR_RED(col)+6*COLOR_GREEN(col)+COLOR_BLUE(col)+5)/10;
	    bc->bitmap[i*bc->bytes_per_line+j] = col/div;
	}
    } else if ( bc->byte_data /* && indexed */ ) {
	int div = 255/((1<<BDFDepth(bv->bdf))-1);
	bc->bitmap = base->data;
	bc->bytes_per_line = base->bytes_per_line;
	for ( i=0; i<base->height; ++i ) for ( j=0; j<base->width; ++j ) {
	    int col = base->clut->clut[base->data[i*base->bytes_per_line+j]];
	    col = 255-(3*COLOR_RED(col)+6*COLOR_GREEN(col)+COLOR_BLUE(col)+5)/10;
	    base->data[i*base->bytes_per_line+j] = col/div;
	}
	base->data = NULL;
    } else /* if ( mono && indexed ) */ {
	bc->bytes_per_line = (base->width>>3)+1;
	bc->bitmap = gcalloc(base->height,bc->bytes_per_line);
	for ( i=0; i<base->height; ++i ) for ( j=0; j<base->width; ++j ) {
	    int col = base->clut->clut[base->data[i*base->bytes_per_line+j]];
	    col = (3*COLOR_RED(col)+6*COLOR_GREEN(col)+COLOR_BLUE(col));
	    if ( col<=5*256 )
		bc->bitmap[i*bc->bytes_per_line+(j>>3)] |= (0x80>>(j&7));
	}
    }
    GImageDestroy(image);
    if ( bc->sc!=NULL )
	bc->sc->widthset = true;
    BCCharChangedUpdate(bc);
return( true );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

int FVImportImages(FontView *fv,char *path,int format,int toback, int flags) {
    GImage *image;
    /*struct _GImage *base;*/
    int tot;
    char *start = path, *endpath=path;
    int i;
    SplineChar *sc;

    tot = 0;
    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i]) {
	sc = SFMakeChar(fv->sf,fv->map,i);
	endpath = strchr(start,';');
	if ( endpath!=NULL ) *endpath = '\0';
	if ( format==fv_image ) {
	    image = GImageRead(start);
	    if ( image==NULL ) {
		gwwv_post_error(_("Bad image file"),_("Bad image file: %.100s"),start);
return(false);
	    }
	    ++tot;
#ifdef FONTFORGE_CONFIG_TYPE3
	    SCAddScaleImage(sc,image,true,toback?ly_back:ly_fore);
#else
	    SCAddScaleImage(sc,image,true,ly_back);
#endif
#ifndef _NO_LIBXML
	} else if ( format==fv_svg ) {
	    SCImportSVG(sc,toback?ly_back:ly_fore,start,NULL,0,flags&sf_clearbeforeinput);
	    ++tot;
	} else if ( format==fv_glif ) {
	    SCImportGlif(sc,toback?ly_back:ly_fore,start,NULL,0,flags&sf_clearbeforeinput);
	    ++tot;
#endif
	} else if ( format==fv_eps ) {
	    SCImportPS(sc,toback?ly_back:ly_fore,start,flags&sf_clearbeforeinput,flags&~sf_clearbeforeinput);
	    ++tot;
	}
	if ( endpath==NULL )
    break;
	start = endpath+1;
    }
    if ( tot==0 )
	gwwv_post_error(_("Nothing Selected"),_("You must select a glyph before you can import an image into it"));
    else if ( endpath!=NULL )
	gwwv_post_error(_("More Images Than Selected Glyphs"),_("More Images Than Selected Glyphs"));
return( true );
}

int FVImportImageTemplate(FontView *fv,char *path,int format,int toback, int flags) {
    GImage *image;
    struct _GImage *base;
    int tot;
    char *ext, *name, *dirname, *pt, *end;
    int i, val;
    int isu=false, ise=false, isc=false;
    DIR *dir;
    struct dirent *entry;
    SplineChar *sc;
    char start [1025];

    ext = strrchr(path,'.');
    name = strrchr(path,'/');
    if ( ext==NULL ) {
	gwwv_post_error(_("Bad Template"),_("Bad template, no extension"));
return( false );
    }
    if ( name==NULL ) name=path-1;
    if ( name[1]=='u' ) isu = true;
    else if ( name[1]=='c' ) isc = true;
    else if ( name[1]=='e' ) ise = true;
    else {
	gwwv_post_error(_("Bad Template"),_("Bad template, unrecognized format"));
return( false );
    }
    if ( name<path )
	dirname = ".";
    else {
	dirname = path;
	*name = '\0';
    }

    if ( (dir = opendir(dirname))==NULL ) {
	    gwwv_post_error(_("Nothing Loaded"),_("Nothing Loaded"));
return( false );
    }
    
    tot = 0;
    while ( (entry=readdir(dir))!=NULL ) {
	pt = strrchr(entry->d_name,'.');
	if ( pt==NULL )
    continue;
	if ( strmatch(pt,ext)!=0 )
    continue;
	if ( !(
		(isu && entry->d_name[0]=='u' && entry->d_name[1]=='n' && entry->d_name[2]=='i' && (val=strtol(entry->d_name+3,&end,16), end==pt)) ||
		(isu && entry->d_name[0]=='u' && (val=strtol(entry->d_name+1,&end,16), end==pt)) ||
		(isc && entry->d_name[0]=='c' && entry->d_name[1]=='i' && entry->d_name[2]=='d' && (val=strtol(entry->d_name+3,&end,10), end==pt)) ||
		(ise && entry->d_name[0]=='e' && entry->d_name[1]=='n' && entry->d_name[2]=='c' && (val=strtol(entry->d_name+3,&end,10), end==pt)) ))
    continue;
	sprintf (start, "%s/%s", dirname, entry->d_name);
	if ( isu ) {
	    i = SFFindSlot(fv->sf,fv->map,val,NULL);
	    if ( i==-1 ) {
		gwwv_post_error(_("Unicode value not in font"),_("Unicode value (%x) not in font, ignored"),val);
    continue;
	    }
	    sc = SFMakeChar(fv->sf,fv->map,i);
	} else {
	    if ( val<fv->map->enccount ) {
		/* It's there */;
	    } else {
		gwwv_post_error(_("Encoding value not in font"),_("Encoding value (%x) not in font, ignored"),val);
    continue;
	    }
	    sc = SFMakeChar(fv->sf,fv->map,val);
	}
	if ( format==fv_imgtemplate ) {
	    image = GImageRead(start);
	    if ( image==NULL ) {
		gwwv_post_error(_("Bad image file"),_("Bad image file: %.100s"),start);
    continue;
	    }
	    base = image->list_len==0?image->u.image:image->u.images[0];
	    if ( base->image_type!=it_mono ) {
		gwwv_post_error(_("Bad image file"),_("Bad image file, not a bitmap: %.100s"),start);
		GImageDestroy(image);
    continue;
	    }
	    ++tot;
#ifdef FONTFORGE_CONFIG_TYPE3
	    SCAddScaleImage(sc,image,true,toback?ly_back:ly_fore);
#else
	    SCAddScaleImage(sc,image,true,ly_back);
#endif
#ifndef _NO_LIBXML
	} else if ( format==fv_svgtemplate ) {
	    SCImportSVG(sc,toback?ly_back:ly_fore,start,NULL,0,flags&sf_clearbeforeinput);
	    ++tot;
	} else if ( format==fv_gliftemplate ) {
	    SCImportGlif(sc,toback?ly_back:ly_fore,start,NULL,0,flags&sf_clearbeforeinput);
	    ++tot;
#endif
	} else {
	    SCImportPS(sc,toback?ly_back:ly_fore,start,flags&sf_clearbeforeinput,flags&~sf_clearbeforeinput);
	    ++tot;
	}
    }
    if ( tot==0 )
	gwwv_post_error(_("Nothing Loaded"),_("Nothing Loaded"));
return( true );
}

/****************************** Import picker *********************************/
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI

static int last_format, flast_format;
struct gfc_data {
    int done;
    int ret;
    GGadget *gfc;
    GGadget *format;
    GGadget *background;
    CharView *cv;
    BitmapView *bv;
    FontView *fv;
};

static GTextInfo formats[] = {
    { (unichar_t *) N_("Image"), NULL, 0, 0, (void *) fv_image, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("EPS"), NULL, 0, 0, (void *) fv_eps, 0, 0, 0, 0, 0, 0, 0, 1 },
#ifndef _NO_LIBXML
    { (unichar_t *) N_("SVG"), NULL, 0, 0, (void *) fv_svg, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Glif"), NULL, 0, 0, (void *) fv_glif, 0, 0, 0, 0, 0, 0, 0, 1 },
#endif
    { (unichar_t *) N_("XFig"), NULL, 0, 0, (void *) fv_fig, 0, 0, 0, 0, 0, 0, 0, 1 },
    { NULL }};

static GTextInfo fvformats[] = {
    { (unichar_t *) N_("BDF"), NULL, 0, 0, (void *) fv_bdf, 0, 0, 0, 0, 0, 1, 0, 1 },
    { (unichar_t *) N_("TTF"), NULL, 0, 0, (void *) fv_ttf, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) NU_("ΤεΧ Bitmap Fonts"), NULL, 0, 0, (void *) fv_pk, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("PCF (pmf)"), NULL, 0, 0, (void *) fv_pcf, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Mac Bitmap"), NULL, 0, 0, (void *) fv_mac, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Win FON"), NULL, 0, 0, (void *) fv_win, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("palm"), NULL, 0, 0, (void *) fv_palm, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Image"), NULL, 0, 0, (void *) fv_image, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Image Template"), NULL, 0, 0, (void *) fv_imgtemplate, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("EPS"), NULL, 0, 0, (void *) fv_eps, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("EPS Template"), NULL, 0, 0, (void *) fv_epstemplate, 0, 0, 0, 0, 0, 0, 0, 1 },
#ifndef _NO_LIBXML
    { (unichar_t *) N_("SVG"), NULL, 0, 0, (void *) fv_svg, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("SVG Template"), NULL, 0, 0, (void *) fv_svgtemplate, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Glif"), NULL, 0, 0, (void *) fv_glif, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Glif Template"), NULL, 0, 0, (void *) fv_gliftemplate, 0, 0, 0, 0, 0, 0, 0, 1 },
#endif
    { NULL }};

static int GFD_ImportOk(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	unichar_t *ret = GGadgetGetTitle(d->gfc);
	char *temp = u2def_copy(ret);
	int pos = GGadgetGetFirstListSelectedItem(d->format);
	int format = (intpt) (GGadgetGetListItemSelected(d->format)->userdata);
	GGadget *tf;

	GFileChooserGetChildren(d->gfc,NULL,NULL,&tf);
	if ( *_GGadgetGetTitle(tf)=='\0' )
return( true );
	GDrawSetCursor(GGadgetGetWindow(g),ct_watch);
	if ( d->fv!=NULL )
	    flast_format = pos;
	else
	    last_format = pos;
	free(ret);
	if ( d->fv!=NULL ) {
	    int toback = GGadgetIsChecked(d->background);
	    if ( toback && strchr(temp,';')!=NULL && format<3 )
		gwwv_post_error(_("Only One Font"),_("Only one font may be imported into the background"));
	    else if ( format==fv_bdf )
		d->done = FVImportBDF(d->fv,temp,false, toback);
	    else if ( format==fv_ttf )
		d->done = FVImportMult(d->fv,temp,toback,bf_ttf);
	    else if ( format==fv_pk )		/* pk */
		d->done = FVImportBDF(d->fv,temp,true, toback);
	    else if ( format==fv_pcf )		/* pcf */
		d->done = FVImportBDF(d->fv,temp,2, toback);
	    else if ( format==fv_mac )
		d->done = FVImportMult(d->fv,temp,toback,bf_nfntmacbin);
	    else if ( format==fv_win )
		d->done = FVImportMult(d->fv,temp,toback,bf_fon);
	    else if ( format==fv_palm )
		d->done = FVImportMult(d->fv,temp,toback,bf_palm);
	    else if ( format==fv_image )
		d->done = FVImportImages(d->fv,temp,format,toback,-1);
	    else if ( format==fv_imgtemplate )
		d->done = FVImportImageTemplate(d->fv,temp,format,toback,-1);
	    else if ( format==fv_eps )
		d->done = FVImportImages(d->fv,temp,format,toback,-1);
	    else if ( format==fv_epstemplate )
		d->done = FVImportImageTemplate(d->fv,temp,format,toback,-1);
	    else if ( format==fv_svg )
		d->done = FVImportImages(d->fv,temp,format,toback,-1);
	    else if ( format==fv_svgtemplate )
		d->done = FVImportImageTemplate(d->fv,temp,format,toback,-1);
	    else if ( format==fv_glif )
		d->done = FVImportImages(d->fv,temp,format,toback,-1);
	    else if ( format==fv_gliftemplate )
		d->done = FVImportImageTemplate(d->fv,temp,format,toback,-1);
	} else if ( d->bv!=NULL )
	    d->done = BVImportImage(d->bv,temp);
	else {
	    d->done = true;
	    if ( format==fv_image )
		ImportImage(d->cv,temp);
	    else if ( format==fv_eps )
		ImportPS(d->cv,temp);
#ifndef _NO_LIBXML
	    else if ( format==fv_svg )
		ImportSVG(d->cv,temp);
	    else if ( format==fv_glif )
		ImportGlif(d->cv,temp);
#endif
	    else
		ImportFig(d->cv,temp);
	}
	free(temp);
    }
    GDrawSetCursor(GGadgetGetWindow(g),ct_pointer);
return( true );
}

static int GFD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = true;
	d->ret = false;
    }
return( true );
}

static int GFD_Format(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int format = GGadgetGetFirstListSelectedItem(g);
	GFileChooserSetFilterText(d->gfc,d->fv==NULL?wildchr[format]:wildfnt[format]);
	GFileChooserRefreshList(d->gfc);
	if ( d->fv!=NULL ) {
	    if ( format==fv_bdf || format==fv_ttf || format==fv_pcf ||
		    format==fv_mac || format==fv_win ) {
		GGadgetSetChecked(d->background,false);
		GGadgetSetEnabled(d->background,true);
	    } else if ( format==fv_pk ) {
		GGadgetSetChecked(d->background,true);
		GGadgetSetEnabled(d->background,true);
	    } else if ( format==fv_eps || format==fv_epstemplate ||
			format==fv_svg || format==fv_svgtemplate ||
			format==fv_glif || format==fv_gliftemplate ) {
		GGadgetSetChecked(d->background,false);
		GGadgetSetEnabled(d->background,true);
	    } else {			/* Images */
		GGadgetSetChecked(d->background,true);
#ifdef FONTFORGE_CONFIG_TYPE3
		GGadgetSetEnabled(d->background,true);
#else
		GGadgetSetEnabled(d->background,false);
#endif
	    }
	}
    }
return( true );
}

static int e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct gfc_data *d = GDrawGetUserData(gw);
	d->done = true;
	d->ret = false;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type==et_map ) {
	GDrawRaise(gw);
    } else if ( event->type == et_mousemove ||
	    (event->type==et_mousedown && event->u.mouse.button==3 )) {
	struct gfc_data *d = GDrawGetUserData(gw);
	GFileChooserPopupCheck(d->gfc,event);
    } else if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button==4 || event->u.mouse.button==5) ) {
	struct gfc_data *d = GDrawGetUserData(gw);
return( GGadgetDispatchEvent((GGadget *) (d->gfc),event));
    }
return( true );
}

static void _Import(CharView *cv,BitmapView *bv,FontView *fv) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[9], boxes[4], *varray[9], *harray[5], *buttons[10];
    GTextInfo label[9];
    struct gfc_data d;
    int i, format;
    int bs = GIntGetResource(_NUM_Buttonsize), bsbigger, totwid, scalewid;
    static int done= false;

    if ( !done ) {
	for ( i=0; formats[i].text!=NULL; ++i )
	    formats[i].text = (unichar_t *) _((char *) formats[i].text);
	for ( i=0; fvformats[i].text!=NULL; ++i )
	    fvformats[i].text = (unichar_t *) _((char *) fvformats[i].text);
	done = true;
    }

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Import...");
    pos.x = pos.y = 0;
    totwid = 223;
    if ( fv!=NULL ) totwid += 60;
    scalewid = GGadgetScale(totwid);
    bsbigger = 3*bs+4*14>scalewid; scalewid = bsbigger?3*bs+4*12:scalewid;
    pos.width = GDrawPointsToPixels(NULL,scalewid);
    pos.height = GDrawPointsToPixels(NULL,255);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; gcd[0].gd.pos.width = totwid-24; gcd[0].gd.pos.height = 182;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    if ( fv!=NULL )
	gcd[0].gd.flags |= gg_file_multiple;
    gcd[0].creator = GFileChooserCreate;
    varray[0] = &gcd[0]; varray[1] = NULL;

    gcd[1].gd.pos.x = 12; gcd[1].gd.pos.y = 224-3; gcd[1].gd.pos.width = -1; gcd[1].gd.pos.height = 0;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[1].text = (unichar_t *) _("_Import");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.handle_controlevent = GFD_ImportOk;
    gcd[1].creator = GButtonCreate;
    buttons[0] = GCD_Glue; buttons[1] = &gcd[1]; buttons[2] = GCD_Glue;

    gcd[2].gd.pos.x = (totwid-bs)*100/GIntGetResource(_NUM_ScaleFactor)/2; gcd[2].gd.pos.y = 224; gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
    gcd[2].gd.flags = gg_visible | gg_enabled;
    label[2].text = (unichar_t *) _("_Filter");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.mnemonic = 'F';
    gcd[2].gd.label = &label[2];
    gcd[2].gd.handle_controlevent = GFileChooserFilterEh;
    gcd[2].creator = GButtonCreate;
    buttons[3] = GCD_Glue; buttons[4] = &gcd[2]; buttons[5] = GCD_Glue;

    gcd[3].gd.pos.x = -gcd[1].gd.pos.x; gcd[3].gd.pos.y = 224; gcd[3].gd.pos.width = -1; gcd[3].gd.pos.height = 0;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[3].text = (unichar_t *) _("_Cancel");
    label[3].text_is_1byte = true;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.handle_controlevent = GFD_Cancel;
    gcd[3].creator = GButtonCreate;
    buttons[6] = GCD_Glue; buttons[7] = &gcd[3]; buttons[8] = GCD_Glue;
    buttons[9] = NULL;

    gcd[4].gd.pos.x = 12; gcd[4].gd.pos.y = 200; gcd[4].gd.pos.width = 0; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled;
    label[4].text = (unichar_t *) _("Format:");
    label[4].text_is_1byte = true;
    gcd[4].gd.label = &label[4];
    gcd[4].creator = GLabelCreate;
    harray[0] = &gcd[4];

    gcd[5].gd.pos.x = 55; gcd[5].gd.pos.y = 194; 
    gcd[5].gd.flags = gg_visible | gg_enabled ;
    if ( bv!=NULL ) {
	gcd[5].gd.flags = gg_visible ;			/* No postscript in bitmap mode */
	last_format=0;
    }
    format = fv==NULL?last_format:flast_format;
    gcd[5].gd.u.list = fv==NULL?formats:fvformats;
    gcd[5].gd.label = &gcd[5].gd.u.list[format];
    gcd[5].gd.handle_controlevent = GFD_Format;
    gcd[5].creator = GListButtonCreate;
    for ( i=0; i<sizeof(formats)/sizeof(formats[0]); ++i )
	gcd[5].gd.u.list[i].selected = false;
    gcd[5].gd.u.list[format].selected = true;
    harray[1] = &gcd[5];

    if ( fv!=NULL ) {
	gcd[6].gd.pos.x = 185; gcd[6].gd.pos.y = gcd[5].gd.pos.y+4;
	gcd[6].gd.flags = gg_visible | gg_enabled ;
#ifdef FONTFORGE_CONFIG_TYPE3
	if ( format==fv_pk || format==fv_image || format==fv_imgtemplate )
	    gcd[6].gd.flags = gg_visible | gg_enabled | gg_cb_on;
#else
	if ( format==fv_pk )
	    gcd[6].gd.flags = gg_visible | gg_enabled | gg_cb_on;
	else if ( format==fv_image || format==fv_imgtemplate )
	    gcd[6].gd.flags = gg_visible | gg_cb_on;
#endif
	label[6].text = (unichar_t *) _("As Background");
	label[6].text_is_1byte = true;
	gcd[6].gd.label = &label[6];
	gcd[6].creator = GCheckBoxCreate;
	harray[2] = &gcd[6]; harray[3] = GCD_Glue; harray[4] = NULL;
    } else {
	harray[2] = GCD_Glue; harray[3] = NULL;
    }

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray;
    boxes[2].creator = GHBoxCreate;
    varray[2] = &boxes[2]; varray[3] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = buttons;
    boxes[3].creator = GHBoxCreate;
    varray[4] = GCD_Glue; varray[5] = NULL;
    varray[6] = &boxes[3]; varray[7] = NULL;
    varray[8] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandgluesame);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
    GGadgetSetUserData(gcd[2].ret,gcd[0].ret);

    GFileChooserConnectButtons(gcd[0].ret,gcd[1].ret,gcd[2].ret);
    GFileChooserSetFilterText(gcd[0].ret,fv!=NULL?wildfnt[format]:wildchr[format]);
    GFileChooserRefreshList(gcd[0].ret);
    GHVBoxFitWindow(boxes[0].ret);
#if 0
    GFileChooserGetChildren(gcd[0].ret,&pulldown,&files,&tf);
    GWidgetIndicateFocusGadget(tf);
#endif

    memset(&d,'\0',sizeof(d));
    d.cv = cv;
    d.fv = fv;
    d.bv = bv;
    d.gfc = gcd[0].ret;
    d.format = gcd[5].ret;
    if ( fv!=NULL )
	d.background = gcd[6].ret;

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

void CVImport(CharView *cv) {
    _Import(cv,NULL,NULL);
}

void BVImport(BitmapView *bv) {
    _Import(NULL,bv,NULL);
}

void FVImport(FontView *fv) {
    _Import(NULL,NULL,fv);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
