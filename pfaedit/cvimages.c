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
#include <math.h>
#include <ustring.h>
#include "utype.h"

static void ImportPS(CharView *cv,char *path) {
    FILE *ps = fopen(path,"r");
    SplinePointList *spl, *espl;

    if ( ps==NULL )
return;
    spl = SplinePointListInterpretPS(ps);
    fclose(ps);
    if ( spl==NULL )
return;
    for ( espl=spl; espl->next!=NULL; espl = espl->next );
    CVPreserveState(cv);
    espl->next = *cv->heads[cv->drawmode];
    *cv->heads[cv->drawmode] = spl;
    CVCharChangedUpdate(cv);
}

/**************************** Fig File Import *********************************/

static BasePoint *slurppoints(FILE *fig,SplineFont *sf,int cnt ) {
    BasePoint *bps = galloc((cnt+1)*sizeof(BasePoint));	/* spline code may want to add another point */
    int x, y, i, ch;
    double scale = sf->ascent/(8.5*1200.0);
    double ascent = 11*1200*sf->ascent/(sf->ascent+sf->descent);

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

static SplineSet * slurparc(FILE *fig,SplineChar *sc, SplineSet *sofar) {
    int ch;
    int sub, dir, fa, ba;
    float cx, cy;
    int sx,sy,ex,ey;
    fscanf(fig, "%d %*d %*d %*d %*d %*d %*d %*d %*f %*d %d %d %d %f %f %d %d %*d %*d %d %d",
	    &sub, &dir, &fa, &ba, &cx, &cy, &sx, &sy, &ex, &ey );
    while ((ch=getc(fig))!='\n' && ch!=EOF);
    /* I ignore arrow lines */
    if ( fa )
	while ((ch=getc(fig))!='\n' && ch!=EOF);
    if ( ba )
	while ((ch=getc(fig))!='\n' && ch!=EOF);
return( sofar );
}

static SplineSet * slurpelipse(FILE *fig,SplineChar *sc, SplineSet *sofar) {
    int ch;
    int sub, dir, cx, cy, rx, ry;
    float angle;
    SplinePointList *spl;
    SplinePoint *sp;
    double dcx,dcy,drx,dry;
    SplineFont *sf = sc->parent;
    double scale = sf->ascent/(8.5*1200.0);
    double ascent = 11*1200*sf->ascent/(sf->ascent+sf->descent);
    /* I ignore the angle */

    fscanf(fig, "%d %*d %*d %*d %*d %*d %*d %*d %*f %d %f %d %d %d %d %*d %*d %*d %*d",
	    &sub, &dir, &angle, &cx, &cy, &rx, &ry );
    while ((ch=getc(fig))!='\n' && ch!=EOF);

    dcx = cx*scale; dcy = (ascent-cy)*scale;
    drx = rx*scale; dry = ry*scale;

    spl = gcalloc(1,sizeof(SplinePointList));
    spl->next = sofar;
    spl->first = sp = gcalloc(1,sizeof(SplinePoint));
    sp->me.x = dcx; sp->me.y = dcy+dry;
	sp->nextcp.x = sp->me.x + .552*drx; sp->nextcp.y = sp->me.y;
	sp->prevcp.x = sp->me.x - .552*drx; sp->prevcp.y = sp->me.y;
    spl->last = sp = gcalloc(1,sizeof(SplinePoint));
    sp->me.x = dcx+drx; sp->me.y = dcy;
	sp->nextcp.x = sp->me.x; sp->nextcp.y = sp->me.y - .552*dry;
	sp->prevcp.x = sp->me.x; sp->prevcp.y = sp->me.y + .552*dry;
    SplineMake(spl->first,sp);
    sp = gcalloc(1,sizeof(SplinePoint));
    sp->me.x = dcx; sp->me.y = dcy-dry;
	sp->nextcp.x = sp->me.x - .552*drx; sp->nextcp.y = sp->me.y;
	sp->prevcp.x = sp->me.x + .552*drx; sp->prevcp.y = sp->me.y;
    SplineMake(spl->last,sp);
    spl->last = sp;
    sp = gcalloc(1,sizeof(SplinePoint));
    sp->me.x = dcx-drx; sp->me.y = dcy;
	sp->nextcp.x = sp->me.x; sp->nextcp.y = sp->me.y + .552*dry;
	sp->prevcp.x = sp->me.x; sp->prevcp.y = sp->me.y - .552*dry;
    SplineMake(spl->last,sp);
    SplineMake(sp,spl->first);
    spl->last = spl->first;
return( spl );
}

static SplineSet * slurppolyline(FILE *fig,SplineChar *sc, SplineSet *sofar) {
    int ch;
    int sub, cnt, fa, ba, radius;	/* radius of roundrects (sub==4) */
    BasePoint *bps;
    SplinePointList *spl=NULL;
    SplinePoint *sp;
    int i;

    fscanf(fig, "%d %*d %*d %*d %*d %*d %*d %*d %*f %*d %*d %d %d %d %d",
	    &sub, &radius, &fa, &ba, &cnt );
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
	spl = gcalloc(1,sizeof(SplinePointList));
	for ( i=0; i<cnt; ++i ) {
	    sp = gcalloc(1,sizeof(SplinePoint));
	    sp->me = sp->nextcp = sp->prevcp = bps[i];
	    sp->nonextcp = sp->noprevcp = true;
	    sp->pointtype = pt_corner;
	    if ( spl->first==NULL )
		spl->first = sp;
	    else
		SplineMake(spl->last,sp);
	    spl->last = sp;
	}
	if ( sub!=1 ) {
	    SplineMake(spl->last,spl->first);
	    spl->last = spl->first;
	}
	/* Note: I don't do round-rect boxes. Ah well */
	spl->next = sc->splines;
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
    double *s;		/* an array of n tension values */
    /* for a closed spline cp[0]==cp[n-1], but we may still need to wrap a bit*/
    unsigned int closed: 1;
};

static double g(double u, double q, double p) {
return( u * (q + u * (2*q + u *( 10-12*q+10*p + u * ( 2*p+14*q-15 + u*(6-5*q-p))))) );
}

static double h(double u, double q) {
    /* The paper says that h(-1)==0, but the definition of h they give */
    /*  doesn't do that. But if we negate the x^5 term it all works */
    /*  (works for the higher derivatives too) */
return( q*u * (1 + u * (2 - u * u * (u+2))) );
}

static void xsplineeval(BasePoint *ret,double t, struct xspline *xs) {
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
	double Tk0 = k+1 + (xs->s[k+1]>0?xs->s[k+1]:0);
	double Tk1 = k+2 + (xs->s[k+2]>0?xs->s[k+2]:0);
	double Tk2 = k+1 - (xs->s[k+1]>0?xs->s[k+1]:0);
	double Tk3 = k+2 - (xs->s[k+2]>0?xs->s[k+2]:0);
	/* Now each blending function has a "p" value that describes its shape*/
	double p0 = 2*(k-Tk0)*(k-Tk0);
	double p1 = 2*(k+1-Tk1)*(k+1-Tk1);
	double p2 = 2*(k+2-Tk2)*(k+2-Tk2);
	double p3 = 2*(k+3-Tk3)*(k+3-Tk3);
	/* and each negative tension blending function has a "q" value */
	double q0 = xs->s[k+1]<0?-xs->s[k+1]/2:0;
	double q1 = xs->s[k+2]<0?-xs->s[k+2]/2:0;
	double q2 = q0;
	double q3 = q1;
	/* the function f for positive s is the same as g if q==0 */
	double A0, A1, A2, A3;
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
    double len=0, sofar;
    double lens[8];
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

static SplineSet *ApproximateXSpline(struct xspline *xs) {
    int i, j;
    double t;
    TPoint mids[7];
    SplineSet *spl = gcalloc(1,sizeof(SplineSet));
    SplinePoint *sp;

    spl->first = spl->last = gcalloc(1,sizeof(SplinePoint));
    xsplineeval(&spl->first->me,0,xs);
    spl->first->pointtype = ( xs->s[0]==0 )?pt_corner:pt_curve;
    for ( i=0; i<xs->n-1; ++i ) {
	if ( i==xs->n-2 && xs->closed )
	    sp = spl->first;
	else {
	    sp = gcalloc(1,sizeof(SplinePoint));
	    sp->pointtype = ( xs->s[i+1]==0 )?pt_corner:pt_curve;
	    xsplineeval(&sp->me,i+1,xs);
	}
	for ( j=0, t=1./8; j<sizeof(mids)/sizeof(mids[0]); ++j, t+=1./8 ) {
	    xsplineeval((BasePoint *) &mids[j],i+t,xs);
	    mids[j].t = t;
	}
	AdjustTs(mids,spl->last,sp);
	ApproximateSplineFromPoints(spl->last,sp,mids,sizeof(mids)/sizeof(mids[0]));
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
    xs.s = galloc((cnt+1)*sizeof(double));
    xs.closed = (sub&1);
    for ( i=0; i<cnt; ++i )
	fscanf(fig,"%lf",&xs.s[i]);
    /* the spec says that the last point of a closed path will duplicate the */
    /* first, but it doesn't seem to */
    if ( xs.closed && ( !DoubleNear(xs.cp[cnt-1].x,xs.cp[0].x) ||
			!DoubleNear(xs.cp[cnt-1].y,xs.cp[0].y) )) {
	xs.n = ++cnt;
	xs.cp[cnt-1] = xs.cp[0];
	xs.s[cnt-1] = xs.s[0];
    }
    spl = ApproximateXSpline(&xs);

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

static void ImportFig(CharView *cv,char *path) {
    FILE *fig;
    char buffer[100];
    SplineSet *spl, *espl;
    int i;

    fig = fopen(path,"r");
    if ( fig==NULL ) {
	GWidgetErrorR(_STR_CantFindFile,_STR_CantFindFile);
return;
    }
    if ( fgets(buffer,sizeof(buffer),fig)==NULL || strcmp(buffer,"#FIG 3.2\n")!=0 ) {
	GWidgetErrorR(_STR_BadXFigFile,_STR_BadXFigFile);
	fclose(fig);
return;
    }
    /* skip the header, it isn't interesting */
    for ( i=0; i<8; ++i )
	fgets(buffer,sizeof(buffer),fig);
    spl = slurpcompoundguts(fig,cv->sc,NULL);
    if ( spl!=NULL ) {
	CVPreserveState(cv);
	for ( espl=spl; espl->next!=NULL; espl=espl->next );
	espl->next = *cv->heads[cv->drawmode];
	*cv->heads[cv->drawmode] = spl;
	CVCharChangedUpdate(cv);
    }
    fclose(fig);
}

/************************** Normal Image Import *******************************/

static void ImageAlterClut(GImage *image) {
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    GClut *clut;

    if ( base->image_type!=it_mono )
return;
    clut = base->clut;
    if ( clut==NULL ) {
	clut=base->clut = gcalloc(1,sizeof(GClut));
	clut->clut_len = 2;
	clut->clut[0] = 0x808080;
	clut->clut[1] = GDrawGetDefaultBackground(NULL);
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
}

static void ImportImage(CharView *cv,char *path) {
    GImage *image;
    ImageList *im;
    double scale;
    int dm = cv->drawmode;

    image = GImageRead(path);
    if ( image==NULL ) {
	GWidgetErrorR(_STR_BadImageFile,_STR_BadImageFile);
return;
    }
    cv->drawmode = dm_back;
    CVPreserveState(cv);
    cv->drawmode = dm;
    scale = (cv->sc->parent->ascent+cv->sc->parent->descent)/
	    (double) GImageGetHeight(image);
    im = galloc(sizeof(ImageList));
    im->image = image;
    im->xoff = 0;
    im->yoff = cv->sc->parent->ascent;
    im->xscale = im->yscale = scale;
    im->selected = true;
    im->next = cv->sc->backimages;
    im->bb.minx = im->xoff; im->bb.maxy = im->yoff;
    im->bb.maxx = im->xoff + GImageGetWidth(im->image)*im->xscale;
    im->bb.miny = im->yoff - GImageGetHeight(im->image)*im->yscale;
    cv->sc->backimages = im;
    SCOutOfDateBackground(cv->sc);
    ImageAlterClut(image);
    CVCharChangedUpdate(cv);
}

static int BCImportImage(BDFChar *bc,char *path) {
    GImage *image;
    struct _GImage *base;
    int tot;
    uint8 *pt, *end;

    image = GImageRead(path);
    if ( image==NULL ) {
	GWidgetErrorR(_STR_BadImageFile,_STR_BadImageFile);
return(false);
    }
    base = image->list_len==0?image->u.image:image->u.images[0];
    if ( base->image_type!=it_mono ) {
	GWidgetErrorR(_STR_BadImageFile,_STR_BadImageFileNotBitmap);
	GImageDestroy(image);
    }
    BCPreserveState(bc);
    BCFlattenFloat(bc);
    free(bc->bitmap);
    bc->bitmap = base->data;
    bc->xmin = bc->ymin = 0;
    bc->xmax = base->width-1; bc->ymax = base->height-1;
    bc->bytes_per_line = base->bytes_per_line;

    /* Sigh. Bitmaps use a different defn of set than images do. make it consistant */
    tot = bc->bytes_per_line*(bc->ymax-bc->ymin+1);
    for ( pt = bc->bitmap, end = pt+tot; pt<end; *pt++ ^= 0xff );

    base->data = NULL;
    GImageDestroy(image);
    BCCharChangedUpdate(bc,NULL);
return( true );
}

/****************************** Import picker *********************************/

static int last_format;
struct gfc_data {
    int done;
    int ret;
    GGadget *gfc;
    GGadget *format;
    CharView *cv;
    BDFChar *bc;
    FontView *fv;
};

static unichar_t wildimg[] = { '*', '.', '{',
#ifndef _NO_LIBUNGIF
'g','i','f',',',
#endif
#ifndef _NO_LIBPNG
'p','n','g',',',
#endif
#ifndef _NO_LIBTIFF
't','i','f','f',',',
#endif
'x','b','m',',', 'b','m','p', '}', '\0' };
static unichar_t wildps[] = { '*', '.', '{', 'p','s',',', 'e','p','s',',','}', '\0' };
static unichar_t wildfig[] = { '*', '.', '{', 'f','i','g',',','x','f','i','g','}',  '\0' };
static unichar_t wildbdf[] = { '*', '.', 'b', 'd','f',  '\0' };

static GTextInfo formats[] = {
    { (unichar_t *) _STR_Image, NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 1 },
    { (unichar_t *) "EPS", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "XFig", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { NULL }};

static GTextInfo fvformats[] = {
    { (unichar_t *) "BDF", NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 1, 0, 1 },
    { NULL }};

static int GFD_ImportOk(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	unichar_t *ret = GGadgetGetTitle(d->gfc);
	char *temp = cu_copy(ret);
	int format = GGadgetGetFirstListSelectedItem(d->format);

	GDrawSetCursor(GGadgetGetWindow(g),ct_watch);
	last_format = format;
	free(ret);
	d->done = true;
	if ( d->fv!=NULL )
	    d->done = FVImportBDF(d->fv,temp);
	else if ( d->bc!=NULL )
	    d->done = BCImportImage(d->bc,temp);
	else if ( format==0 )
	    ImportImage(d->cv,temp);
	else if ( format==1 )
	    ImportPS(d->cv,temp);
	else
	    ImportFig(d->cv,temp);
	free(temp);
    }
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
	GFileChooserSetFilterText(d->gfc,format==0?wildimg:format==1?wildps:wildfig);
	GFileChooserRefreshList(d->gfc);
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
    }
return( true );
}

static void _Import(CharView *cv,BDFChar *bc,FontView *fv) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8];
    GTextInfo label[7];
    struct gfc_data d;
    int i;
    int bs = GIntGetResource(_NUM_Buttonsize), bsbigger, totwid;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(_STR_Import,NULL);
    pos.x = pos.y = 0;
    bsbigger = 3*bs+4*14>223; totwid = bsbigger?3*bs+4*12:223;
    pos.width = GDrawPointsToPixels(NULL,totwid);
    pos.height = GDrawPointsToPixels(NULL,255);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; gcd[0].gd.pos.width = totwid-24; gcd[0].gd.pos.height = 182;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    if ( fv!=NULL )
	gcd[0].gd.flags |= gg_file_multiple;
    gcd[0].creator = GFileChooserCreate;

    gcd[1].gd.pos.x = 12; gcd[1].gd.pos.y = 224-3; gcd[1].gd.pos.width = -1; gcd[1].gd.pos.height = 0;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[1].text = (unichar_t *) _STR_Import;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.handle_controlevent = GFD_ImportOk;
    gcd[1].creator = GButtonCreate;

    gcd[2].gd.pos.x = (totwid-bs)/2; gcd[2].gd.pos.y = 224; gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
    gcd[2].gd.flags = gg_visible | gg_enabled;
    label[2].text = (unichar_t *) _STR_Filter;
    label[2].text_in_resource = true;
    gcd[2].gd.mnemonic = 'F';
    gcd[2].gd.label = &label[2];
    gcd[2].gd.handle_controlevent = GFileChooserFilterEh;
    gcd[2].creator = GButtonCreate;

    gcd[3].gd.pos.x = totwid-gcd[1].gd.pos.x-bs; gcd[3].gd.pos.y = 224; gcd[3].gd.pos.width = -1; gcd[3].gd.pos.height = 0;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[3].text = (unichar_t *) _STR_Cancel;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.handle_controlevent = GFD_Cancel;
    gcd[3].creator = GButtonCreate;

    gcd[4].gd.pos.x = 12; gcd[4].gd.pos.y = 200; gcd[4].gd.pos.width = 0; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled;
    label[4].text = (unichar_t *) _STR_Format;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].creator = GLabelCreate;

    gcd[5].gd.pos.x = 55; gcd[5].gd.pos.y = 194; 
    gcd[5].gd.flags = gg_visible | gg_enabled ;
    if ( bc!=NULL || fv!=NULL ) {
	gcd[5].gd.flags = gg_visible ;			/* No postscript in bitmap mode */
	last_format=0;
    }
    gcd[5].gd.u.list = fv==NULL?formats:fvformats;
    gcd[5].gd.label = &gcd[5].gd.u.list[last_format];
    gcd[5].gd.handle_controlevent = GFD_Format;
    gcd[5].creator = GListButtonCreate;
    for ( i=0; i<sizeof(formats)/sizeof(formats[0]); ++i )
	formats[i].selected = false;
    gcd[5].gd.u.list[last_format].selected = true;

    GGadgetsCreate(gw,gcd);
    GGadgetSetUserData(gcd[2].ret,gcd[0].ret);

    GFileChooserConnectButtons(gcd[0].ret,gcd[1].ret,gcd[2].ret);
    GFileChooserSetFilterText(gcd[0].ret,fv!=NULL?wildbdf:bc!=NULL?wildimg:
	    last_format==0?wildimg:last_format==1?wildps:wildfig);
    GFileChooserRefreshList(gcd[0].ret);
#if 0
    GFileChooserGetChildren(gcd[0].ret,&pulldown,&files,&tf);
    GWidgetIndicateFocusGadget(tf);
#endif

    memset(&d,'\0',sizeof(d));
    d.cv = cv;
    d.fv = fv;
    d.bc = bc;
    d.gfc = gcd[0].ret;
    d.format = gcd[5].ret;

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
    _Import(NULL,bv->bc,NULL);
}

void FVImport(FontView *fv) {
    _Import(NULL,NULL,fv);
}
