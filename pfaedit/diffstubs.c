/* A set of routines used in pfaedit that I don't want/need to bother with for*/
/*  sfddiff, but which get called by it */

#include "pfaeditui.h"
#include <stdarg.h>
#include <math.h>
#include <charset.h>

Encoding *enclist = NULL;
int local_encoding = e_iso8859_1;

void GProgressStartIndicator(
    int delay,			/* in tenths of seconds */
    const unichar_t *win_title,	/* for the window decoration */
    const unichar_t *line1,	/* First line of description */
    const unichar_t *line2,	/* Second line */
    int tot,			/* Number of sub-entities in the operation */
    int stages			/* Number of stages, each processing tot sub-entities */
) {}
void GProgressStartIndicatorR(int delay, int win_titler, int line1r,
	int line2r, int tot, int stages) {}
void GProgressEnableStop(int enabled) {}
void GProgressEndIndicator(void) {}
int GProgressNextStage(void) { return(1); }
int GProgressNext(void) { return( 1 ); }
void GProgressChangeLine2R(int line2r) {}
void GProgressChangeTotal(int tot) { }
void GProgressChangeStages(int stages) { }
int GWidgetAskR(int title, int *answers, int def, int cancel,int question,...) { return cancel; }
SplineFont *LoadSplineFont(char *filename) { return NULL; }
int SFReencodeFont(SplineFont *sf,enum charset new_map) { return 0 ; }
void LinearApproxFree(LinearApprox *la) {}
SplineFont *SplineFontNew(void) {return NULL; }
void SplineFontFree(SplineFont *sf) { }
void BDFCharFree(BDFChar *bc) { }
void SplineCharFree(SplineChar *sc) { }
void SCMakeDependent(SplineChar *dependent,SplineChar *base) {}
void SCReinstanciateRefChar(SplineChar *sc,RefChar *rf) {}
void SCGuessHHintInstancesList(SplineChar *sc) { }
void SCGuessVHintInstancesList(SplineChar *sc) { }
int StemListAnyConflicts(StemInfo *stems) { return 0 ; }
int getAdobeEnc(char *name) { return -1; }

/* ************************************************************************** */
/* And some routines we actually do need */

void GDrawIError(const char *format, ... ) {
    va_list ap;
    va_start(ap,format);
    vfprintf(stderr,format,ap);
    va_end(ap);
}

int RealNear(real a,real b) {
    real d;

#ifdef USE_DOUBLE
    if ( a==0 )
return( b>-1e-8 && b<1e-8 );
    if ( b==0 )
return( a>-1e-8 && a<1e-8 );

    d = a/(1024*1024.);
    if ( d<0 ) d = -d;
return( b>a-d && b<a+d );
#else		/* For floats */
    if ( a==0 )
return( b>-1e-5 && b<1e-5 );
    if ( b==0 )
return( a>-1e-5 && a<1e-5 );

    d = a/(1024*64.);
    if ( d<0 ) d = -d;
return( b>a-d && b<a+d );
#endif
}

int RealApprox(real a,real b) {

    if ( a==0 ) {
	if ( b<.0001 && b>-.0001 )
return( true );
    } else if ( b==0 ) {
	if ( a<.0001 && a>-.0001 )
return( true );
    } else {
	a /= b;
	if ( a>=.95 && a<=1.05 )
return( true );
    }
return( false );
}

int SplineIsLinear(Spline *spline) {
    real t1,t2;
    int ret;

    if ( spline->knownlinear )
return( true );
    if ( spline->knowncurved )
return( false );

    if ( spline->splines[0].a==0 && spline->splines[0].b==0 &&
	    spline->splines[1].a==0 && spline->splines[1].b==0 )
return( true );

    /* Something is linear if the control points lie on the line between the */
    /*  two base points */

    /* Vertical lines */
    if ( RealNear(spline->from->me.x,spline->to->me.x) ) {
	ret = RealNear(spline->from->me.x,spline->from->nextcp.x) &&
	    RealNear(spline->from->me.x,spline->to->prevcp.x) &&
	    ((spline->from->nextcp.y >= spline->from->me.y &&
	      spline->from->nextcp.y <= spline->to->prevcp.y &&
	      spline->to->prevcp.y <= spline->to->me.y ) ||
	     (spline->from->nextcp.y <= spline->from->me.y &&
	      spline->from->nextcp.y >= spline->to->prevcp.y &&
	      spline->to->prevcp.y >= spline->to->me.y ));
    /* Horizontal lines */
    } else if ( RealNear(spline->from->me.y,spline->to->me.y) ) {
	ret = RealNear(spline->from->me.y,spline->from->nextcp.y) &&
	    RealNear(spline->from->me.y,spline->to->prevcp.y) &&
	    ((spline->from->nextcp.x >= spline->from->me.x &&
	      spline->from->nextcp.x <= spline->to->prevcp.x &&
	      spline->to->prevcp.x <= spline->to->me.x) ||
	     (spline->from->nextcp.x <= spline->from->me.x &&
	      spline->from->nextcp.x >= spline->to->prevcp.x &&
	      spline->to->prevcp.x >= spline->to->me.x));
    } else {
	ret = true;
	t1 = (spline->from->nextcp.y-spline->from->me.y)/(spline->to->me.y-spline->from->me.y);
	if ( t1<0 || t1>1.0 )
	    ret = false;
	else {
	    t2 = (spline->from->nextcp.x-spline->from->me.x)/(spline->to->me.x-spline->from->me.x);
	    if ( t2<0 || t2>1.0 )
		ret = false;
	    ret = RealApprox(t1,t2);
	}
	if ( ret ) {
	    t1 = (spline->to->me.y-spline->to->prevcp.y)/(spline->to->me.y-spline->from->me.y);
	    if ( t1<0 || t1>1.0 )
		ret = false;
	    else {
		t2 = (spline->to->me.x-spline->to->prevcp.x)/(spline->to->me.x-spline->from->me.x);
		if ( t2<0 || t2>1.0 )
		    ret = false;
		else
		    ret = RealApprox(t1,t2);
	    }
	}
    }
    spline->knowncurved = !ret;
    spline->knownlinear = ret;
return( ret );
}

void SplineRefigure(Spline *spline) {
    SplinePoint *from = spline->from, *to = spline->to;
    Spline1D *xsp = &spline->splines[0], *ysp = &spline->splines[1];

#ifdef DEBUG
    if ( RealNear(from->me.x,to->me.x) && RealNear(from->me.y,to->me.y))
	GDrawIError("Zero length spline created");
#endif
    xsp->d = from->me.x; ysp->d = from->me.y;
    if ( from->nonextcp ) from->nextcp = from->me;
    else if ( from->nextcp.x==from->me.x && from->nextcp.y == from->me.y ) from->nonextcp = true;
    if ( to->noprevcp ) to->prevcp = to->me;
    else if ( to->prevcp.x==to->me.x && to->prevcp.y == to->me.y ) to->noprevcp = true;
    if ( from->nonextcp && to->noprevcp ) {
	spline->islinear = true;
	xsp->c = to->me.x-from->me.x;
	ysp->c = to->me.y-from->me.y;
	xsp->a = xsp->b = 0;
	ysp->a = ysp->b = 0;
    } else {
	/* from p. 393 (Operator Details, curveto) Postscript Lang. Ref. Man. (Red book) */
	xsp->c = 3*(from->nextcp.x-from->me.x);
	ysp->c = 3*(from->nextcp.y-from->me.y);
	xsp->b = 3*(to->prevcp.x-from->nextcp.x)-xsp->c;
	ysp->b = 3*(to->prevcp.y-from->nextcp.y)-ysp->c;
	xsp->a = to->me.x-from->me.x-xsp->c-xsp->b;
	ysp->a = to->me.y-from->me.y-ysp->c-ysp->b;
	if ( RealNear(xsp->c,0)) xsp->c=0;
	if ( RealNear(ysp->c,0)) ysp->c=0;
	if ( RealNear(xsp->b,0)) xsp->b=0;
	if ( RealNear(ysp->b,0)) ysp->b=0;
	if ( RealNear(xsp->a,0)) xsp->a=0;
	if ( RealNear(ysp->a,0)) ysp->a=0;
	spline->islinear = false;
	if ( ysp->a==0 && xsp->a==0 && ysp->b==0 && xsp->b==0 )
	    spline->islinear = true;	/* This seems extremely unlikely... */
    }
    if ( isnan(ysp->a) || isnan(xsp->a) )
	GDrawIError("NaN value in spline creation");
    LinearApproxFree(spline->approx);
    spline->approx = NULL;
    spline->knowncurved = false;
    spline->knownlinear = spline->islinear;
    SplineIsLinear(spline);
    spline->isquadratic = false;
    if ( !spline->islinear && xsp->a==0 && ysp->a==0 )
	spline->isquadratic = true;	/* Only likely if we read in a TTF */
}

Spline *SplineMake(SplinePoint *from, SplinePoint *to) {
    Spline *spline = chunkalloc(sizeof(Spline));

    spline->from = from; spline->to = to;
    from->next = to->prev = spline;
    SplineRefigure(spline);
return( spline );
}

int SCWorthOutputting(SplineChar *sc) {
return( sc!=NULL &&
	( sc->splines!=NULL || sc->refs!=NULL || sc->widthset ||
#if HANYANG
	    sc->compositionunit ||
#endif
	    sc->dependents!=NULL ||
	    sc->width!=sc->parent->ascent+sc->parent->descent ) &&
	( strcmp(sc->name,".notdef")!=0 || sc->enc==0) );
}

SplineFont *SplineFontEmpty(void) {
    SplineFont *sf;
    sf = gcalloc(1,sizeof(SplineFont));
    sf->pfminfo.fstype = -1;
    sf->encoding_name = em_none;
return( sf );
}

void *chunkalloc(int size) {
return( gcalloc(1,size));
}

void chunkfree(void *item,int size) {
    free(item);
}

GImage *GImageCreate(enum image_type type, int32 width, int32 height) {
    GImage *gi;
    struct _GImage *base;

    if ( type<it_mono || type>it_true )
return( NULL );

    gi = gcalloc(1,sizeof(GImage));
    base = galloc(sizeof(struct _GImage));
    if ( gi==NULL || base==NULL ) {
	free(gi); free(base);
return( NULL );
    }
    gi->u.image = base;
    base->image_type = type;
    base->width = width;
    base->height = height;
    base->bytes_per_line = type==it_true?4*width:type==it_index?width:(width+7)/8;
    base->data = NULL;
    base->clut = NULL;
    base->trans = COLOR_UNKNOWN;
    base->data = galloc(height*base->bytes_per_line);
    if ( base->data==NULL ) {
	free(base);
	free(gi);
return( NULL );
    }
    if ( type==it_index ) {
	base->clut = gcalloc(1,sizeof(GClut));
	base->clut->trans_index = COLOR_UNKNOWN;
    }
return( gi );
}

int GImageGetWidth(GImage *img) {
    if ( img->list_len==0 ) {
return( img->u.image->width );
    } else {
return( img->u.images[0]->width );
    }
}

int GImageGetHeight(GImage *img) {
    if ( img->list_len==0 ) {
return( img->u.image->height );
    } else {
return( img->u.images[0]->height );
    }
}
