/* A set of routines used in pfaedit that I don't want/need to bother with for*/
/*  sfddiff, but which get called by it */

#include "pfaeditui.h"
#include <stdarg.h>
#include <math.h>
#include <charset.h>
#include <ustring.h>

Encoding *enclist = NULL;
int local_encoding = e_iso8859_1;
char *iconv_local_encoding_name = NULL;

#if defined(FONTFORGE_CONFIG_GDRAW)
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
#elif defined(FONTFORGE_CONFIG_GTK)
void gwwv_progress_start_indicator(
    int delay,			/* in tenths of seconds */
    const unichar_t *win_title,	/* for the window decoration */
    const unichar_t *line1,	/* First line of description */
    const unichar_t *line2,	/* Second line */
    int tot,			/* Number of sub-entities in the operation */
    int stages			/* Number of stages, each processing tot sub-entities */
) {}
void gwwv_progress_enable_stop(int enabled) {}
void gwwv_progress_end_indicator(void) {}
int gwwv_progress_next_stage(void) { return(1); }
int gwwv_progress_next(void) { return( 1 ); }
void gwwv_progress_change_line2(int line2r) {}
void gwwv_progress_change_total(int tot) { }
void gwwv_progress_change_stages(int stages) { }
#endif

SplineFont *LoadSplineFont(char *filename, enum openflags of) { return NULL; }
int SFReencodeFont(SplineFont *sf,enum charset new_map) { return 0 ; }
void RefCharFree(RefChar *ref) {}
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
SplineChar *SFMakeChar(SplineFont *sf, int enc) { return NULL; }
GDisplay *screen_display=NULL;
int no_windowing_ui = true;
int FVWinInfo(struct fontview *sf,int *cc,int *rc) { return 0 ; }

/* ************************************************************************** */
/* And some routines we actually do need */

void IError(const char *format, ... ) {
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

void SplineRefigure3(Spline *spline) {
    SplinePoint *from = spline->from, *to = spline->to;
    Spline1D *xsp = &spline->splines[0], *ysp = &spline->splines[1];

#ifdef DEBUG
    if ( RealNear(from->me.x,to->me.x) && RealNear(from->me.y,to->me.y))
	IError("Zero length spline created");
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
	IError("NaN value in spline creation");
    LinearApproxFree(spline->approx);
    spline->approx = NULL;
    spline->knowncurved = false;
    spline->knownlinear = spline->islinear;
    SplineIsLinear(spline);
    spline->isquadratic = false;
    if ( !spline->islinear && xsp->a==0 && ysp->a==0 )
	spline->isquadratic = true;	/* Only likely if we read in a TTF */
}

void SplineRefigure2(Spline *spline) {
    SplinePoint *from = spline->from, *to = spline->to;
    Spline1D *xsp = &spline->splines[0], *ysp = &spline->splines[1];

#ifdef DEBUG
    if ( RealNear(from->me.x,to->me.x) && RealNear(from->me.y,to->me.y))
	IError("Zero length spline created");
#endif

    if ( from->nonextcp ) from->nextcp = from->me;
    else if ( from->nextcp.x==from->me.x && from->nextcp.y == from->me.y ) from->nonextcp = true;
    if ( to->noprevcp ) to->prevcp = to->me;
    else if ( to->prevcp.x==to->me.x && to->prevcp.y == to->me.y ) to->noprevcp = true;

    if ( from->nonextcp && to->noprevcp )
	/* Ok */;
    else if ( from->nonextcp || to->noprevcp || from->nextcp.x!=to->prevcp.x ||
	    from->nextcp.y!=to->prevcp.y )
	IError("Invalid 2nd order spline in SplineRefigure2" );

    xsp->d = from->me.x; ysp->d = from->me.y;
    if ( from->nonextcp && to->noprevcp ) {
	spline->islinear = true;
	xsp->c = to->me.x-from->me.x;
	ysp->c = to->me.y-from->me.y;
	xsp->a = xsp->b = 0;
	ysp->a = ysp->b = 0;
    } else {
	/* from p. 393 (Operator Details, curveto) Postscript Lang. Ref. Man. (Red book) */
	xsp->c = 2*(from->nextcp.x-from->me.x);
	ysp->c = 2*(from->nextcp.y-from->me.y);
	xsp->b = to->me.x-from->me.x-xsp->c;
	ysp->b = to->me.y-from->me.y-ysp->c;
	xsp->a = 0;
	ysp->a = 0;
	if ( RealNear(xsp->c,0)) xsp->c=0;
	if ( RealNear(ysp->c,0)) ysp->c=0;
	if ( RealNear(xsp->b,0)) xsp->b=0;
	if ( RealNear(ysp->b,0)) ysp->b=0;
	spline->islinear = false;
	if ( ysp->b==0 && xsp->b==0 )
	    spline->islinear = true;	/* This seems extremely unlikely... */
    }
    if ( isnan(ysp->b) || isnan(xsp->b) )
	IError("NaN value in spline creation");
    LinearApproxFree(spline->approx);
    spline->approx = NULL;
    spline->knowncurved = false;
    spline->knownlinear = spline->islinear;
    SplineIsLinear(spline);
    spline->isquadratic = !spline->knownlinear;
    spline->order2 = true;
}

Spline *SplineMake3(SplinePoint *from, SplinePoint *to) {
    Spline *spline = chunkalloc(sizeof(Spline));

    spline->from = from; spline->to = to;
    from->next = to->prev = spline;
    SplineRefigure3(spline);
return( spline );
}

Spline *SplineMake2(SplinePoint *from, SplinePoint *to) {
    Spline *spline = chunkalloc(sizeof(Spline));

    spline->from = from; spline->to = to;
    from->next = to->prev = spline;
    SplineRefigure2(spline);
return( spline );
}

Spline *SplineMake(SplinePoint *from, SplinePoint *to, int order2) {
    if ( order2 )
return( SplineMake2(from,to));
    else
return( SplineMake3(from,to));
}

int SCWorthOutputting(SplineChar *sc) {
return( sc!=NULL &&
	( sc->layers[ly_fore].splines!=NULL || sc->layers[ly_fore].refs!=NULL || sc->widthset ||
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

RefChar *RefCharCreate(void) {
    RefChar *ref = chunkalloc(sizeof(RefChar));
#ifdef FONTFORGE_CONFIG_TYPE3
    ref->layers = gcalloc(1,sizeof(struct reflayer));
    ref->layers[0].fill_brush.opacity = ref->layers[0].stroke_pen.brush.opacity = 1.0;
    ref->layers[0].fill_brush.col = ref->layers[0].stroke_pen.brush.col = COLOR_INHERITED;
    ref->layers[0].stroke_pen.width = WIDTH_INHERITED;
    ref->layers[0].stroke_pen.linecap = lc_inherited;
    ref->layers[0].stroke_pen.linejoin = lj_inherited;
    ref->layers[0].dofill = true;
#endif
    ref->layer_cnt = 1;
return( ref );
}

void *chunkalloc(int size) {
return( gcalloc(1,size));
}

void chunkfree(void *item,int size) {
    free(item);
}

void LayerDefault(Layer *layer) {
    memset(layer,0,sizeof(Layer));
#ifdef FONTFORGE_CONFIG_TYPE3
    layer->fill_brush.opacity = layer->stroke_pen.brush.opacity = 1.0;
    layer->fill_brush.col = layer->stroke_pen.brush.col = COLOR_INHERITED;
    layer->stroke_pen.width = WIDTH_INHERITED;
    layer->stroke_pen.linecap = lc_inherited;
    layer->stroke_pen.linejoin = lj_inherited;
    layer->dofill = true;
    layer->fillfirst = true;
    layer->stroke_pen.trans[0] = layer->stroke_pen.trans[3] = 1.0;
    layer->stroke_pen.trans[1] = layer->stroke_pen.trans[2] = 0.0;
#endif
}

SplineChar *SplineCharCreate(void) {
    SplineChar *sc = gcalloc(1,sizeof(SplineChar));
    sc->color = COLOR_DEFAULT;
    sc->orig_pos = 0xffff;
    sc->unicodeenc = -1;
    sc->layer_cnt = 2;
#ifdef FONTFORGE_CONFIG_TYPE3
    sc->layers = gcalloc(2,sizeof(Layer));
    LayerDefault(&sc->layers[0]);
    LayerDefault(&sc->layers[1]);
#endif
return( sc );    
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

void BDFClut(BDFFont *bdf, int linear_scale) {
    int scale = linear_scale*linear_scale, i;
    Color bg = 0xffffff;
    int bgr=COLOR_RED(bg), bgg=COLOR_GREEN(bg), bgb=COLOR_BLUE(bg);
    GClut *clut;

    bdf->clut = clut = gcalloc(1,sizeof(GClut));
    clut->clut_len = scale;
    clut->is_grey = (bgr==bgg && bgb==bgr);
    clut->trans_index = -1;
    for ( i=0; i<scale; ++i ) {
	clut->clut[i] =
		COLOR_CREATE( bgr- (i*(bgr))/(scale-1),
				bgg- (i*(bgg))/(scale-1),
				bgb- (i*(bgb))/(scale-1));
    }
    clut->clut[scale-1] = 0;	/* avoid rounding errors */
}

GImage *ImageAlterClut(GImage *image) { return NULL; }

int BDFDepth(BDFFont *bdf) {
    if ( bdf->clut==NULL )
return( 1 );

return( bdf->clut->clut_len==256 ? 8 :
	bdf->clut->clut_len==16 ? 4 : 2);
}

/* scripts (for opentype) that I understand */

static uint32 scripts[][11] = {
/* Arabic */	{ CHR('a','r','a','b'), 0x0600, 0x06ff, 0xfb50, 0xfdff, 0xfe70, 0xfeff },
/* Armenian */	{ CHR('a','r','m','n'), 0x0530, 0x058f, 0xfb13, 0xfb17 },
/* Bengali */	{ CHR('b','e','n','g'), 0x0980, 0x09ff },
/* Bopomofo */	{ CHR('b','o','p','o'), 0x3100, 0x312f },
/* Braille */	{ CHR('b','r','a','i'), 0x2800, 0x28ff },
/* Byzantine M*/{ CHR('b','y','z','m'), 0x1d000, 0x1d0ff },
/* Canadian Syl*/{CHR('c','a','n','s'), 0x1400, 0x167f },
/* Cherokee */	{ CHR('c','h','e','r'), 0x13a0, 0x13ff },
/* Cyrillic */	{ CHR('c','y','r','l'), 0x0500, 0x052f },
/* Devanagari */{ CHR('d','e','v','a'), 0x0900, 0x097f },
/* Ethiopic */	{ CHR('e','t','h','i'), 0x1300, 0x139f },
/* Georgian */	{ CHR('g','e','o','r'), 0x1080, 0x10ff },
/* Greek */	{ CHR('g','r','e','k'), 0x0370, 0x03ff, 0x1f00, 0x1fff },
/* Gujarati */	{ CHR('g','u','j','r'), 0x0a80, 0x0aff },
/* Gurmukhi */	{ CHR('g','u','r','u'), 0x0a00, 0x0a7f },
/* Hangul */	{ CHR('h','a','n','g'), 0xac00, 0xd7af, 0x3130, 0x319f, 0xffa0, 0xff9f },
 /* I'm not sure what the difference is between the 'hang' tag and the 'jamo' */
 /*  tag. 'Jamo' is said to be the precomposed forms, but what's 'hang'? */
/* CJKIdeogra */{ CHR('h','a','n','i'), 0x3300, 0x9fff, 0xf900, 0xfaff, 0x020000, 0x02ffff },
/* Hebrew */	{ CHR('h','e','b','r'), 0x0590, 0x05ff, 0xfb1e, 0xfb4ff },
#if 0	/* Hiragana used to have its own tag, but has since been merged with katakana */
/* Hiragana */	{ CHR('h','i','r','a'), 0x3040, 0x309f },
#endif
/* Hangul Jamo*/{ CHR('j','a','m','o'), 0x1100, 0x11ff, 0x3130, 0x319f, 0xffa0, 0xffdf },
/* Katakana */	{ CHR('k','a','n','a'), 0x3040, 0x30ff, 0xff60, 0xff9f },
/* Khmer */	{ CHR('k','h','m','r'), 0x1780, 0x17ff },
/* Kannada */	{ CHR('k','n','d','a'), 0x0c80, 0x0cff },
/* Latin */	{ CHR('l','a','t','n'), 0x0000, 0x02af, 0x1d00, 0x1eff, 0xfb00, 0xfb0f, 0xff00, 0xff5f, 0, 0 },
/* Lao */	{ CHR('l','a','o',' '), 0x0e80, 0x0eff },
/* Malayalam */	{ CHR('m','l','y','m'), 0x0d00, 0x0d7f },
/* Mongolian */	{ CHR('m','o','n','g'), 0x1800, 0x18af },
/* Myanmar */	{ CHR('m','y','m','r'), 0x1000, 0x107f },
/* Ogham */	{ CHR('o','g','a','m'), 0x1680, 0x169f },
/* Oriya */	{ CHR('o','r','y','a'), 0x0b00, 0x0b7f },
/* Runic */	{ CHR('r','u','n','r'), 0x16a0, 0x16ff },
/* Sinhala */	{ CHR('s','i','n','h'), 0x0d80, 0x0dff },
/* Syriac */	{ CHR('s','y','r','c'), 0x0700, 0x074f },
/* Tamil */	{ CHR('t','a','m','l'), 0x0b80, 0x0bff },
/* Telugu */	{ CHR('t','e','l','u'), 0x0c00, 0x0c7f },
/* Thaana */	{ CHR('t','h','a','a'), 0x0780, 0x07bf },
/* Thai */	{ CHR('t','h','a','i'), 0x0e00, 0x0e7f },
/* Tibetan */	{ CHR('t','i','b','t'), 0x0f00, 0x0fff },
/* Yi */	{ CHR('y','i',' ',' '), 0xa000, 0xa73f },
		{ 0 }
};

uint32 ScriptFromUnicode(int u,SplineFont *sf) {
    int s, k;
    int enc;

    if ( u!=-1 ) {
	for ( s=0; scripts[s][0]!=0; ++s ) {
	    for ( k=1; scripts[s][k+1]!=0; k += 2 )
		if ( u>=scripts[s][k] && u<=scripts[s][k+1] )
	    break;
	    if ( scripts[s][k+1]!=0 )
	break;
	}
	if ( scripts[s][0]!=0 )
return( scripts[s][0] );
    }

    if ( sf==NULL )
return( 0 );
    enc = sf->encoding_name;
    if ( sf->cidmaster!=NULL || sf->subfontcnt!=0 ) {
	if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
	if ( strmatch(sf->ordering,"Identity")==0 )
return( 0 );
	else if ( strmatch(sf->ordering,"Korean")==0 )
return( CHR('j','a','m','o'));
	else
return( CHR('h','a','n','i') );
    }

    if ( enc==em_jis208 || enc==em_jis212 || enc==em_gb2312 || enc==em_big5 ||
	    enc == em_big5hkscs || enc==em_sjis || enc==em_jisgb )
return( CHR('h','a','n','i') );
    else if ( enc==em_ksc5601 || enc==em_johab || enc==em_wansung )
return( CHR('j','a','m','o') );
    else if ( enc==em_iso8859_11 )
return( CHR('t','h','a','i'));
    else if ( enc==em_iso8859_8 )
return( CHR('h','e','b','r'));
    else if ( enc==em_iso8859_7 )
return( CHR('g','r','e','k'));
    else if ( enc==em_iso8859_6 )
return( CHR('a','r','a','b'));
    else if ( enc==em_iso8859_5 || enc==em_koi8_r )
return( CHR('c','y','r','l'));
    else if ( enc==em_jis201 )
return( CHR('k','a','n','a'));
    else if ( (enc>=em_iso8859_1 && enc<=em_iso8859_15 ) || enc==em_mac ||
	    enc==em_win || enc==em_adobestandard )
return( CHR('l','a','t','n'));

return( 0 );
}

uint32 SCScriptFromUnicode(SplineChar *sc) {
    PST *pst;
    SplineFont *sf;
    int i;

    if ( sc==NULL )
return( 0 );

    sf = sc->parent;
    if ( sc->unicodeenc!=-1 )
return( ScriptFromUnicode( sc->unicodeenc,sf ));
    if ( sf==NULL )
return( 0 );

    if ( sf->cidmaster ) sf=sf->cidmaster;
    for ( i=0; i<2; ++i ) {
	for ( pst=sc->possub; pst!=NULL; pst=pst->next ) if ( pst->script_lang_index!=0xffff ) {
	    if ( i==1 || sf->script_lang[pst->script_lang_index][1].script==0 )
return( sf->script_lang[pst->script_lang_index]->script );
	}
    }
return( ScriptFromUnicode( sc->unicodeenc,sf ));
}

int SCRightToLeft(SplineChar *sc) {
    uint32 script;

    if ( sc->unicodeenc>=0x10800 && sc->unicodeenc<=0x10fff )
return( true );		/* Supplemental Multilingual Plane, RTL scripts */

    script = SCScriptFromUnicode(sc);
return( script==CHR('a','r','a','b') || script==CHR('h','e','b','r') );
}

int SFAddScriptLangIndex(SplineFont *sf,uint32 script,uint32 lang) {
    int i;

    if ( sf->cidmaster ) sf = sf->cidmaster;
    if ( script==0 ) script=DEFAULT_SCRIPT;
    if ( lang==0 ) lang=DEFAULT_LANG;
    if ( sf->script_lang==NULL )
	sf->script_lang = gcalloc(2,sizeof(struct script_record *));
    for ( i=0; sf->script_lang[i]!=NULL; ++i ) {
	if ( sf->script_lang[i][0].script==script && sf->script_lang[i][1].script==0 &&
		sf->script_lang[i][0].langs[0]==lang &&
		sf->script_lang[i][0].langs[1]==0 )
return( i );
    }
    sf->script_lang = grealloc(sf->script_lang,(i+2)*sizeof(struct script_record *));
    sf->script_lang[i] = gcalloc(2,sizeof(struct script_record));
    sf->script_lang[i][0].script = script;
    sf->script_lang[i][0].langs = galloc(2*sizeof(uint32));
    sf->script_lang[i][0].langs[0] = lang;
    sf->script_lang[i][0].langs[1] = 0;
    sf->script_lang[i+1] = NULL;
return( i );
}

#if HANYANG
void SFDDumpCompositionRules(FILE *sfd,struct compositionrules *rules) {
}

struct compositionrules *SFDReadCompositionRules(FILE *sfd) {
    char buffer[200];
    while ( fgets(buffer,sizeof(buffer),sfd)!=NULL )
	if ( strstr(buffer, "EndCompositionRules")!=NULL )
    break;
return( NULL );
}
#endif

void SFConvertToOrder2(SplineFont *_sf) {}
void SFConvertToOrder3(SplineFont *_sf) {}

static int UnicodeContainsCombiners(int uni) {

    if ( uni<0 || uni>0xffff )
return( -1 );
return( false );
}

uint16 PSTDefaultFlags(enum possub_type type,SplineChar *sc ) {
    uint16 flags = 0;

    if ( sc!=NULL ) {
	if ( SCRightToLeft(sc))
	    flags = pst_r2l;
	if ( type==pst_ligature ) {
	    int script = SCScriptFromUnicode(sc);
	    if ( script==CHR('h','e','b','r') || script==CHR('a','r','a','b')) {
		if ( !UnicodeContainsCombiners(sc->unicodeenc))
		    flags |= pst_ignorecombiningmarks;
	    }
	}
    }
return( flags );
}
