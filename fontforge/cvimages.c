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

#include "cvimages.h"

#include "cvundoes.h"
#include "fontforgevw.h"
#include "fvfonts.h"
#include "parsepdf.h"
#include <math.h>
#include <sys/types.h>
#include <dirent.h>
#include "sd.h"
#include <ustring.h>
#include <utype.h>

void SCAppendEntityLayers(SplineChar *sc, Entity *ent) {
    int cnt, pos;
    Entity *e, *enext;
    Layer *old = sc->layers;
    SplineSet *ss;

    for ( e=ent, cnt=0; e!=NULL; e=e->next, ++cnt );
    pos = sc->layer_cnt;
    if ( cnt==0 )
return;
    EntityDefaultStrokeFill(ent);

    sc->layers = realloc(sc->layers,(sc->layer_cnt+cnt)*sizeof(Layer));
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
	    sc->layers[pos].fill_brush.gradient = e->u.splines.fill.grad;
	    /*!!!!!! pattern? */
	    sc->layers[pos].stroke_pen.brush.col = e->u.splines.stroke.col==0xffffffff ? COLOR_INHERITED : e->u.splines.stroke.col;
	    sc->layers[pos].stroke_pen.brush.gradient = e->u.splines.stroke.grad;
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
	    sc->layers[pos].dofill = base->image_type==it_mono && base->trans!=(Color)-1;
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
	if ( e->clippath ) {
	    for ( ss=e->clippath; ss->next!=NULL; ss=ss->next )
		ss->is_clip_path = true;
	    ss->is_clip_path = true;
	    ss->next = sc->layers[pos].splines;
	    sc->layers[pos].splines = e->clippath;
	}
	free(e);
    }
    sc->layer_cnt += cnt;
    SCMoreLayers(sc,old);
}

void SCImportPSFile(SplineChar *sc,int layer,FILE *ps,int doclear,int flags) {
    SplinePointList *spl, *espl;
    SplineSet **head;
    int empty, width;

    if ( ps==NULL )
return;
    width = UNDEFINED_WIDTH;
    empty = sc->layers[layer].splines==NULL && sc->layers[layer].refs==NULL;
    if ( sc->parent->multilayer && layer>ly_back ) {
	SCAppendEntityLayers(sc, EntityInterpretPS(ps,&width));
    } else {
	spl = SplinePointListInterpretPS(ps,flags,sc->parent->strokedfont,&width);
	if ( spl==NULL ) {
	    ff_post_error( _("Too Complex or Bad"), _("I'm sorry this file is too complex for me to understand (or is erroneous, or is empty)") );
return;
	}
	if ( sc->layers[layer].order2 )
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
    if ( (empty || doclear) && width!=UNDEFINED_WIDTH )
	SCSynchronizeWidth(sc,width,sc->width,NULL);
    SCCharChangedUpdate(sc,layer);
}

void SCImportPS(SplineChar *sc,int layer,char *path,int doclear, int flags) {
    FILE *ps = fopen(path,"r");

    if ( ps==NULL )
return;
    SCImportPSFile(sc,layer,ps,doclear,flags);
    fclose(ps);
}

void SCImportPDFFile(SplineChar *sc,int layer,FILE *pdf,int doclear,int flags) {
    SplinePointList *spl, *espl;
    SplineSet **head;

    if ( pdf==NULL )
return;

    if ( sc->parent->multilayer && layer>ly_back ) {
	SCAppendEntityLayers(sc, EntityInterpretPDFPage(pdf,-1));
    } else {
	spl = SplinesFromEntities(EntityInterpretPDFPage(pdf,-1),&flags,sc->parent->strokedfont);
	if ( spl==NULL ) {
	    ff_post_error( _("Too Complex or Bad"), _("I'm sorry this file is too complex for me to understand (or is erroneous, or is empty)") );
return;
	}
	if ( sc->layers[layer].order2 )
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
    SCCharChangedUpdate(sc,layer);
}

void SCImportPDF(SplineChar *sc,int layer,char *path,int doclear, int flags) {
    FILE *pdf = fopen(path,"r");

    if ( pdf==NULL )
return;
    SCImportPDFFile(sc,layer,pdf,doclear,flags);
    fclose(pdf);
}

void SCImportPlateFile(SplineChar *sc,int layer,FILE *plate,int doclear) {
    SplineSet **ly_head, *head, *cur, *last;
    spiro_cp *spiros=NULL;
    int cnt=0, max=0, ch;
    char buffer[80];
    real transform[6];

    if ( plate==NULL )
return;

    head = last = NULL;
    fgets(buffer,sizeof(buffer),plate);
    if ( strncmp(buffer,"(plate",strlen("plate("))!=0 ) {
	ff_post_error( _("Not a plate file"), _("This does not seem to be a plate file\nFirst line wrong"));
return;
    }
    while ( !feof(plate)) {
	while ( isspace( (ch=getc(plate)) ) );
	if ( ch==')' || ch==EOF )
    break;
	if ( ch!='(' ) {
	    ff_post_error( _("Not a plate file"), _("This does not seem to be a plate file\nExpected left paren"));
return;
	}
	ch = getc(plate);
	if ( ch!='v' && ch!='o' && ch!='c' && ch!='[' && ch!=']' && ch!='z' ) {
	    ff_post_error( _("Not a plate file"), _("This does not seem to be a plate file\nExpected one of 'voc[]z'"));
return;
	}
	if ( cnt>=max )
	    spiros = realloc(spiros,(max+=30)*sizeof(spiro_cp));
	spiros[cnt].x = spiros[cnt].y = 0;
	spiros[cnt].ty = ch;
	if ( ch=='z' ) {
	    cur = SpiroCP2SplineSet(spiros);
	    cur->spiros = SpiroCPCopy(spiros,&cur->spiro_cnt);
	    cur->spiro_max = cur->spiro_cnt;
	    SplineSetAddExtrema(sc,cur,ae_only_good,sc->parent->ascent+sc->parent->descent);
	    if ( cur==NULL )
		/* Do Nothing */;
	    else if ( last!=NULL ) {
		last->next = cur;
		last = cur;
	    } else
		head = last = cur;
	    cnt = 0;
	    ch = getc(plate);		/* Must be ')' */
	} else {
	    if ( fscanf(plate,"%lg %lg )", &spiros[cnt].x, &spiros[cnt].y)!=2 ) {
		ff_post_error( _("Not a plate file"), _("This does not seem to be a plate file\nExpected two real numbers"));
return;
	    }
	    ++cnt;
	}
    }
    if ( cnt!=0 ) {
	/* This happens when we've got an open contour */
	if ( cnt>=max )
	    spiros = realloc(spiros,(max+=30)*sizeof(spiro_cp));
	spiros[cnt].x = spiros[cnt].y = 0;
	spiros[cnt].ty = 'z';
	spiros[0].ty = '{';		/* Open contour mark */
	cur = SpiroCP2SplineSet(spiros);
	cur->spiros = SpiroCPCopy(spiros,&cur->spiro_cnt);
	cur->spiro_max = cur->spiro_cnt;
	SplineSetAddExtrema(sc,cur,ae_only_good,sc->parent->ascent+sc->parent->descent);
	if ( cur==NULL )
	    /* Do Nothing */;
	else if ( last!=NULL ) {
	    last->next = cur;
	    last = cur;
	} else
	    head = last = cur;
    }
    free(spiros);

    /* Raph's plate files seem to have the base line at 800, and glyphs grow */
    /*  downwards */ /* At least for Inconsola */
    memset(transform,0,sizeof(transform));
    transform[0] = 1; transform[3] = -1;
    transform[5] = 800;
    head = SplinePointListTransform(head,transform,tpt_AllPoints);
    /* After doing the above flip, the contours appear oriented acording to my*/
    /*  conventions */

    if ( sc->layers[layer].order2 ) {
	head = SplineSetsConvertOrder(head,true);
	for ( last=head; last->next!=NULL; last = last->next );
    }
    if ( layer==ly_grid )
	ly_head = &sc->parent->grid.splines;
    else {
	SCPreserveLayer(sc,layer,false);
	ly_head = &sc->layers[layer].splines;
    }
    if ( doclear ) {
	SplinePointListsFree(*ly_head);
	*ly_head = NULL;
    }
    last->next = *ly_head;
    *ly_head = head;
    SCCharChangedUpdate(sc,layer);
}

void SCImportSVG(SplineChar *sc,int layer,char *path,char *memory, int memlen, int doclear) {
    SplinePointList *spl, *espl, **head;

    if ( sc->parent->multilayer && layer>ly_back ) {
	SCAppendEntityLayers(sc, EntityInterpretSVG(path,memory,memlen,sc->parent->ascent+sc->parent->descent,
		sc->parent->ascent));
    } else {
	spl = SplinePointListInterpretSVG(path,memory,memlen,sc->parent->ascent+sc->parent->descent,
		sc->parent->ascent,sc->parent->strokedfont);
	for ( espl = spl; espl!=NULL && espl->first->next==NULL; espl=espl->next );
	if ( espl!=NULL )
	    if ( espl->first->next->order2!=sc->layers[layer].order2 )
		spl = SplineSetsConvertOrder(spl,sc->layers[layer].order2);
	if ( spl==NULL ) {
	    ff_post_error(_("Too Complex or Bad"),_("I'm sorry this file is too complex for me to understand (or is erroneous)"));
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
    SCCharChangedUpdate(sc,layer);
}

void SCImportGlif(SplineChar *sc,int layer,char *path,char *memory, int memlen, int doclear) {
    SplinePointList *spl, *espl, **head;

    spl = SplinePointListInterpretGlif(sc->parent,path,memory,memlen,sc->parent->ascent+sc->parent->descent,
	    sc->parent->ascent,sc->parent->strokedfont);
    for ( espl = spl; espl!=NULL && espl->first->next==NULL; espl=espl->next );
    if ( espl!=NULL )
	if ( espl->first->next->order2!=sc->layers[layer].order2 )
	    spl = SplineSetsConvertOrder(spl,sc->layers[layer].order2);
    if ( spl==NULL ) {
	ff_post_error(_("Too Complex or Bad"),_("I'm sorry this file is too complex for me to understand (or is erroneous)"));
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

    SCCharChangedUpdate(sc,layer);
}

/**************************** Fig File Import *********************************/

static BasePoint *slurppoints(FILE *fig,SplineFont *sf,int cnt ) {
    BasePoint *bps = malloc((cnt+1)*sizeof(BasePoint));	/* spline code may want to add another point */
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

static SplineSet *slurpcolor(FILE *fig,SplineSet *sofar) {
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
	ep = SplinePointCreate((double)(cx+r)*ec, (double)(cy+r)*es);
    len = ((double)(ea-sa)/(3.1415926535897932/2)) * (double)r * .552;

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
    sx = _sx*scale; sy = (ascent-_sy)*scale; ex = _ex*scale; ey=(ascent-_ey)*scale; cx=(double)cx*scale; cy=(ascent-(double)cy)*scale;
    r = sqrt( (sx-cx)*(sx-cx) + (sy-cy)*(sy-cy) );
    sa = atan2(sy-cy,sx-cx);
    ea = atan2(ey-cy,ex-cx);

    spl = chunkalloc(sizeof(SplinePointList));
    spl->next = sofar;
    spl->first = sp = SplinePointCreate(sx,sy);
    spl->last = ep = SplinePointCreate(ex,ey);

    if ( dir==0 ) {	/* clockwise */
	if ( ea>sa ) ea = (double)ea - 2*3.1415926535897932;
	ma=ceil((double)sa/(3.1415926535897932/2)-1)*(3.1415926535897932/2);
	if ( RealNearish( sa,ma )) ma = (double)ma - (3.1415926535897932/2);
	while ( ma > ea ) {
	    sp = ArcSpline(sp,sa,NULL,ma,cx,cy,r);
	    sa = ma;
	    ma = (double)ma - (3.1415926535897932/2);
	}
	sp = ArcSpline(sp,sa,ep,ea,cx,cy,r);
    } else {		/* counterclockwise */
	if ( ea<sa ) ea = (double)ea + 2*3.1415926535897932;
	ma=floor((double)sa/(3.1415926535897932/2)+1)*(3.1415926535897932/2);
	if ( RealNearish( sa,ma )) ma = (double)ma + (3.1415926535897932/2);
	while ( ma < ea ) {
	    sp = ArcSpline(sp,sa,NULL,ma,cx,cy,r);
	    sa = ma;
	    ma = (double)ma + (3.1415926535897932/2);
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
    size_t i, j;
    real t;
    TPoint mids[7];
    SplineSet *spl = chunkalloc(sizeof(SplineSet));
    SplinePoint *sp;

    spl->first = spl->last = chunkalloc(sizeof(SplinePoint));
    xsplineeval(&spl->first->me,0,xs);
    spl->first->pointtype = ( xs->s[0]==0 )?pt_corner:pt_curve;
    for ( i=0; i<(size_t)(xs->n-1); ++i ) {
	if ( i==(size_t)(xs->n-2) && xs->closed )
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
    xs.s = malloc((cnt+1)*sizeof(real));
    xs.closed = (sub&1);
    for ( i=0; i<cnt; ++i )
#ifdef FONTFORGE_CONFIG_USE_DOUBLE
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
    spl = ApproximateXSpline(&xs,sc->layers[ly_fore].order2);

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
	    sofar = slurpcolor(fig,sofar);
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

void SCImportFig(SplineChar *sc,int layer,char *path,int doclear) {
    FILE *fig;
    char buffer[100];
    SplineSet *spl, *espl, **head;
    int i;

    fig = fopen(path,"r");
    if ( fig==NULL ) {
	ff_post_error(_("Can't find the file"),_("Can't find the file"));
return;
    }
    if ( fgets(buffer,sizeof(buffer),fig)==NULL || strcmp(buffer,"#FIG 3.2\n")!=0 ) {
	ff_post_error(_("Bad xfig file"),_("Bad xfig file"));
	fclose(fig);
return;
    }
    /* skip the header, it isn't interesting */
    for ( i=0; i<8; ++i )
	fgets(buffer,sizeof(buffer),fig);
    spl = slurpcompoundguts(fig,sc,NULL);
    if ( spl!=NULL ) {
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
	if ( sc->layers[ly_fore].order2 )
	    spl = SplineSetsConvertOrder(spl,true);
	for ( espl=spl; espl->next!=NULL; espl=espl->next );
	espl->next = *head;
	*head = spl;
	SCCharChangedUpdate(sc,layer);
    }
    fclose(fig);
}

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
	clut=base->clut = calloc(1,sizeof(GClut));
	clut->clut_len = 2;
	clut->clut[0] = 0x808080;
	if ( !no_windowing_ui )
	    clut->clut[1] = default_background;
	else
	    clut->clut[1] = 0xb0b0b0;
	clut->trans_index = 1;
	base->trans = 1;
    } else if ( base->trans!=(Color)-1 ) {
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
    im = malloc(sizeof(ImageList));
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
    SCCharChangedUpdate(sc,layer);
}

void SCAddScaleImage(SplineChar *sc,GImage *image,int doclear, int layer) {
    double scale;

    image = ImageAlterClut(image);
    scale = (sc->parent->ascent+sc->parent->descent)/(real) GImageGetHeight(image);
    if ( doclear ) {
	ImageListsFree(sc->layers[layer].images);
	sc->layers[layer].images = NULL;
    }
    SCInsertImage(sc,image,scale,sc->parent->ascent,0,layer);
}

int FVImportImages(FontViewBase *fv,char *path,int format,int toback, int flags) {
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
		ff_post_error(_("Bad image file"),_("Bad image file: %.100s"),start);
return(false);
	    }
	    ++tot;
	    SCAddScaleImage(sc,image,true,toback?ly_back:ly_fore);
	} else if ( format==fv_svg ) {
	    SCImportSVG(sc,toback?ly_back:fv->active_layer,start,NULL,0,flags&sf_clearbeforeinput);
	    ++tot;
	} else if ( format==fv_glif ) {
	    SCImportGlif(sc,toback?ly_back:fv->active_layer,start,NULL,0,flags&sf_clearbeforeinput);
	    ++tot;
	} else if ( format==fv_eps ) {
	    SCImportPS(sc,toback?ly_back:fv->active_layer,start,flags&sf_clearbeforeinput,flags&~sf_clearbeforeinput);
	    ++tot;
	} else if ( format==fv_pdf ) {
	    SCImportPDF(sc,toback?ly_back:fv->active_layer,start,flags&sf_clearbeforeinput,flags&~sf_clearbeforeinput);
	    ++tot;
#ifndef _NO_PYTHON
	} else if ( format>=fv_pythonbase ) {
	    PyFF_SCImport(sc,format-fv_pythonbase,start, toback?ly_back:fv->active_layer,flags&sf_clearbeforeinput);
	    ++tot;
#endif
	}
	if ( endpath==NULL )
    break;
	start = endpath+1;
    }
    if ( tot==0 )
	ff_post_error(_("Nothing Selected"),_("You must select a glyph before you can import an image into it"));
    else if ( endpath!=NULL )
	ff_post_error(_("More Images Than Selected Glyphs"),_("More Images Than Selected Glyphs"));
return( true );
}

int FVImportImageTemplate(FontViewBase *fv,char *path,int format,int toback, int flags) {
    GImage *image;
    struct _GImage *base;
    int tot;
    char *ext, *name, *pt, *end;
    const char *dirname;
    int i, val;
    int isu=false, ise=false, isc=false;
    DIR *dir;
    struct dirent *entry;
    SplineChar *sc;
    char start [1025];

    ext = strrchr(path,'.');
    name = strrchr(path,'/');
    if ( ext==NULL ) {
	ff_post_error(_("Bad Template"),_("Bad template, no extension"));
return( false );
    }
    if ( name==NULL ) name=path-1;
    if ( name[1]=='u' ) isu = true;
    else if ( name[1]=='c' ) isc = true;
    else if ( name[1]=='e' ) ise = true;
    else {
	ff_post_error(_("Bad Template"),_("Bad template, unrecognized format"));
return( false );
    }
    if ( name<path )
	dirname = ".";
    else {
	dirname = path;
	*name = '\0';
    }

    if ( (dir = opendir(dirname))==NULL ) {
	    ff_post_error(_("Nothing Loaded"),_("Nothing Loaded"));
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
		ff_post_error(_("Unicode value not in font"),_("Unicode value (%x) not in font, ignored"),val);
    continue;
	    }
	    sc = SFMakeChar(fv->sf,fv->map,i);
	} else {
	    if ( val<fv->map->enccount ) {
		/* It's there */;
	    } else {
		ff_post_error(_("Encoding value not in font"),_("Encoding value (%x) not in font, ignored"),val);
    continue;
	    }
	    sc = SFMakeChar(fv->sf,fv->map,val);
	}
	if ( format==fv_imgtemplate ) {
	    image = GImageRead(start);
	    if ( image==NULL ) {
		ff_post_error(_("Bad image file"),_("Bad image file: %.100s"),start);
    continue;
	    }
	    base = image->list_len==0?image->u.image:image->u.images[0];
	    if ( base->image_type!=it_mono ) {
		ff_post_error(_("Bad image file"),_("Bad image file, not a bitmap: %.100s"),start);
		GImageDestroy(image);
    continue;
	    }
	    ++tot;
	    SCAddScaleImage(sc,image,true,toback?ly_back:ly_fore);
	} else if ( format==fv_svgtemplate ) {
	    SCImportSVG(sc,toback?ly_back:fv->active_layer,start,NULL,0,flags&sf_clearbeforeinput);
	    ++tot;
	} else if ( format==fv_gliftemplate ) {
	    SCImportGlif(sc,toback?ly_back:fv->active_layer,start,NULL,0,flags&sf_clearbeforeinput);
	    ++tot;
	} else if ( format==fv_pdftemplate ) {
	    SCImportPDF(sc,toback?ly_back:fv->active_layer,start,flags&sf_clearbeforeinput,flags&~sf_clearbeforeinput);
	    ++tot;
	} else {
	    SCImportPS(sc,toback?ly_back:fv->active_layer,start,flags&sf_clearbeforeinput,flags&~sf_clearbeforeinput);
	    ++tot;
	}
    }
    closedir(dir);
    if ( tot==0 )
	ff_post_error(_("Nothing Loaded"),_("Nothing Loaded"));
return( true );
}
