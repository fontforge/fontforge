/* Copyright (C) 2001-2002 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derfved from this software without specific prior written permission.

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
#include "ttfmodui.h"
#include "ttfinstrs.h"

/* This is the interface module to freetype. You don't need it if you aren't */
/*  rasterizing glyphs for the fontview (and aren't able to use the bytecode */
/*  interpreter */
#if TT_RASTERIZE_FONTVIEW || TT_CONFIG_OPTION_BYTECODE_INTERPRETER

# ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
#  if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
#   define _TT_CONFIG_OPTION_BYTECODE_INTERPRETER
#  endif
#  undef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
# endif
#include <ft2build.h>
#include FT_FREETYPE_H
# if !defined(TT_CONFIG_OPTION_BYTECODE_INTERPRETER) && defined(_TT_CONFIG_OPTION_BYTECODE_INTERPRETER)
#  warning "You have compiled TtfMod to use FreeType's bytecode interpreter."
#  warning "But you didn't compile FreeType with it enabled. I suppose you"
#  warning "might want to examine their AutoHinter, so this is just a warning."
#  warning "But you should check and make sure that's what you want." 
# endif
# ifdef _TT_CONFIG_OPTION_BYTECODE_INTERPRETER
#  undef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
#  define TT_CONFIG_OPTION_BYTECODE_INTERPRETER 1
# else 
#  undef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
#  define TT_CONFIG_OPTION_BYTECODE_INTERPRETER 0
# endif

#if TT_CONFIG_OPTION_BYTECODE_DEBUG && TT_CONFIG_OPTION_FREETYPE_DEBUG && TT_CONFIG_OPTION_BYTECODE_INTERPRETER
# include "ttobjs.h"
# include "ttdriver.h"
# include "ttinterp.h"
#endif

static FT_Library context;

/* Ok, this complication is here because:				    */
/*	1) I want to be able to deal with static libraries on some systems  */
/*	2) If I've got a dynamic library and I compile up an executable     */
/*		I want it to run on systems without freetype		    */
/* So one case boils down to linking against the standard names, while the  */
/*  other does the link at run time if it's possible */

/*#define _STATIC_LIBFREETYPE 1		/* Debug */
# if defined(_STATIC_LIBFREETYPE) || defined(NODYNAMIC)

#define _FT_Init_FreeType FT_Init_FreeType
#define _FT_New_Face FT_New_Face
#define _FT_Set_Pixel_Sizes FT_Set_Pixel_Sizes
#define _FT_Done_Face FT_Done_Face
#define _FT_Load_Glyph FT_Load_Glyph
#define _FT_Render_Glyph FT_Render_Glyph
#define _FT_Outline_Decompose FT_Outline_Decompose

#define _FT_Get_Module FT_Get_Module
#define _FT_Set_Debug_Hook FT_Set_Debug_Hook
#define _TT_RunIns TT_RunIns

static int freetype_init_base() {
return( true );
}
# else
#include <dlfcn.h>

static void *libfreetype;
static FT_Error (*_FT_Init_FreeType)( FT_Library  * );
static FT_Error (*_FT_New_Face)( FT_Library, const char *, int, FT_Face * );
static FT_Error (*_FT_Done_Face)( FT_Face );
static FT_Error (*_FT_Set_Pixel_Sizes)( FT_Face, int, int);
static FT_Error (*_FT_Load_Glyph)( FT_Face, int, int);
static FT_Error (*_FT_Render_Glyph)( FT_GlyphSlot, int);
static FT_Error (*_FT_Outline_Decompose)(FT_Outline *, const FT_Outline_Funcs *,void *);

#if TT_CONFIG_OPTION_BYTECODE_DEBUG && TT_CONFIG_OPTION_FREETYPE_DEBUG && TT_CONFIG_OPTION_BYTECODE_INTERPRETER
static FT_Module (*_FT_Get_Module)(FT_Library, const char *);
static void (*_FT_Set_Debug_Hook)(FT_Library, FT_UInt, FT_DebugHook_Func);
static FT_Error (*_TT_RunIns)( TT_ExecContext );
#endif

static int freetype_init_base() {
    libfreetype = dlopen("libfreetype.so",RTLD_LAZY);
    if ( libfreetype==NULL )
return( false );

    _FT_Init_FreeType = (FT_Error (*)(FT_Library *)) dlsym(libfreetype,"FT_Init_FreeType");
    _FT_New_Face = (FT_Error (*)(FT_Library, const char *, int, FT_Face * )) dlsym(libfreetype,"FT_New_Face");
    _FT_Set_Pixel_Sizes = (FT_Error (*)(FT_Face, int, int)) dlsym(libfreetype,"FT_Set_Pixel_Sizes");
    _FT_Done_Face = (FT_Error (*)(FT_Face)) dlsym(libfreetype,"FT_Done_Face");
    _FT_Load_Glyph = (FT_Error (*)(FT_Face, int, int)) dlsym(libfreetype,"FT_Load_Glyph");
    _FT_Render_Glyph = (FT_Error (*)(FT_GlyphSlot, int)) dlsym(libfreetype,"FT_Render_Glyph");
    _FT_Outline_Decompose = (FT_Error (*)(FT_Outline *, const FT_Outline_Funcs *,void *)) dlsym(libfreetype,"FT_Outline_Decompose");

#if TT_CONFIG_OPTION_BYTECODE_DEBUG && TT_CONFIG_OPTION_FREETYPE_DEBUG && TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    _FT_Get_Module = (FT_Module (*)(FT_Library, const char* )) dlsym(libfreetype,"FT_Get_Module");
    _FT_Set_Debug_Hook = (void (*)(FT_Library, FT_UInt, FT_DebugHook_Func)) dlsym(libfreetype,"FT_Set_Debug_Hook");
    _TT_RunIns = (FT_Error (*)(TT_ExecContext)) dlsym(libfreetype,"TT_RunIns");
#endif
return( true );
}
# endif

static int freetype_init() {
    int done=false;
    int ok=false;

    if ( done )
return(ok);
    done = true;

    if ( !freetype_init_base())
return( false );
    if ( _FT_Init_FreeType( &context ))
return( false );

    ok = true;
return( true );
}

struct freetype_raster *FreeType_GetRaster(FontView *fv,int index) {
    FT_GlyphSlot slot;
    struct freetype_raster *ret;

    if ( fv->freetype_face==NULL ) {
	TtfFile *tfile = fv->cf->tfile;
	int i;

	if ( !freetype_init())
return( (void *) -1 );	/* Error */
	for ( i=0; i<tfile->font_cnt; ++i )
	    if ( tfile->fonts[i]==fv->cf->tfont )
	break;
	if ( _FT_New_Face(context,tfile->filename,i,(FT_Face *) &fv->freetype_face)) {
	    fv->freetype_face = (void *) -1;
return( (void *) -1 );	/* Error Return */
	}
	/* I want a 24 PIXEL (not point) bitmap */
	/* Er... let's try 22. A 24 pixel ARIAL does not fit in a 27 pixel grid */
	if ( _FT_Set_Pixel_Sizes(fv->freetype_face,0,22)) {
	    _FT_Done_Face(fv->freetype_face);
	    fv->freetype_face = (void *) -1;
return( (void *) -1 );	/* Error Return */
	}
    } else if ( fv->freetype_face == (void *) -1 )
return( (void *) -1 );

    if ( _FT_Load_Glyph(fv->freetype_face,index,FT_LOAD_RENDER))
return( (void *) -1 );

    slot = ((FT_Face) (fv->freetype_face))->glyph;
    if ( slot->bitmap.pixel_mode!=ft_pixel_mode_mono &&
	    slot->bitmap.pixel_mode!=ft_pixel_mode_grays )
return( (void *) -1 );
    ret = galloc(sizeof(struct freetype_raster));

    ret->rows = slot->bitmap.rows;
    ret->cols = slot->bitmap.width;
    ret->bytes_per_row = slot->bitmap.pitch;
    ret->as = slot->bitmap_top;
    ret->num_greys = slot->bitmap.num_grays;
	/* Can't find any description of freetype's bitendianness */
	/* These guys will probably be greyscale anyway... */
    ret->bitmap = galloc(ret->rows*ret->bytes_per_row);
    memcpy(ret->bitmap,slot->bitmap.buffer,ret->rows*ret->bytes_per_row);
return( ret );
}

void FreeType_ShowRaster(GWindow pixmap,int x,int y,
			struct freetype_raster *raster) {
    GClut clut;
    struct _GImage base;
    GImage gi;
    Color bg = GDrawGetDefaultBackground(NULL);
    int bgr=COLOR_RED(bg), bgg=COLOR_GREEN(bg), bgb=COLOR_BLUE(bg);

    memset(&gi,'\0',sizeof(gi));
    memset(&base,'\0',sizeof(base));
    memset(&clut,'\0',sizeof(clut));
    gi.u.image = &base;
    base.trans = 0;
    base.clut = &clut;
    if ( raster->num_greys>2 ) { int i;
	base.image_type = it_index;
	clut.clut_len = raster->num_greys;
	for ( i=0; i<clut.clut_len; ++i ) {
	    clut.clut[i] =
		    COLOR_CREATE( bgr- (i*(bgr))/(clut.clut_len-1),
				    bgg- (i*(bgg))/(clut.clut_len-1),
				    bgb- (i*(bgb))/(clut.clut_len-1));
	}
    } else {
	base.image_type = it_mono;
	clut.clut_len = 2;
	clut.clut[0] = 0xffffff;
    }
    clut.trans_index = 0;
    base.data = raster->bitmap;
    base.bytes_per_line = raster->bytes_per_row;
    base.width = raster->cols;
    base.height = raster->rows;
    GDrawDrawImage(pixmap,&gi,NULL,x,y);
}

void FreeType_FreeRaster(struct freetype_raster *raster) {
    if ( raster==NULL || raster==(void *) -1 )
return;
    free(raster->bitmap);
    free(raster);
}

# if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
struct ft_context {
    ConicPointList *hcpl, *lcpl, *cpl;
    ConicPoint *last;
    double scale;
    ConicPointList *orig_cpl;
    ConicPoint *orig_sp;
};

static void FT_ClosePath(struct ft_context *context) {
    if ( context->cpl!=NULL ) {
	if ( context->cpl->first->me.x != context->last->me.x ||
		context->cpl->first->me.y != context->last->me.y )
	    ConicMake(context->last,context->cpl->first);
	else {
	    context->cpl->first->prevcp = context->last->nextcp;
	    context->last->prev->to = context->cpl->first;
	    ConicPointFree(context->last);
	}
	context->cpl->last = context->cpl->first;
	context->last = NULL;
	if ( context->orig_cpl!=NULL )
	    context->orig_cpl = context->orig_cpl->next;
	context->orig_sp = NULL;
    }
}

static int FT_CubicTo(FT_Vector *cp1, FT_Vector *cp2,FT_Vector *to,void *user) {
    GDrawIError("Got a cubic in outline decomposition");
return( 1 );
}

static int FT_MoveTo(FT_Vector *to,void *user) {
    struct ft_context *context = user;

    FT_ClosePath(context);

    context->cpl = chunkalloc(sizeof(ConicPointList));
    if ( context->lcpl==NULL )
	context->hcpl = context->cpl;
    else
	context->lcpl->next = context->cpl;
    context->lcpl = context->cpl;

    if ( context->orig_cpl!=NULL )
	context->orig_sp = context->orig_cpl->first;

    context->last = context->cpl->first = chunkalloc(sizeof(ConicPoint));
    context->last->me.x = to->x*context->scale;
    context->last->me.y = to->y*context->scale;
    context->last->me.pnum = context->orig_sp?context->orig_sp->me.pnum: -2;
return( 0 );
}

static int FT_LineTo(FT_Vector *to,void *user) {
    struct ft_context *context = user;
    ConicPoint *sp;

    sp = chunkalloc(sizeof(ConicPoint));
    sp->me.x = to->x*context->scale;
    sp->me.y = to->y*context->scale;
    sp->me.pnum = -1;
    ConicMake(context->last,sp);
    context->last = sp;

    if ( context->orig_sp!=NULL ) {
	context->orig_sp = context->orig_sp->next->to;
	if ( context->orig_sp!=NULL )
	    sp->me.pnum = context->orig_sp->me.pnum;
    }
return( 0 );
}

static int FT_ConicTo(FT_Vector *_cp, FT_Vector *to,void *user) {
    struct ft_context *context = user;
    ConicPoint *sp;
    BasePoint *cp;

    sp = chunkalloc(sizeof(ConicPoint));
    sp->me.x = to->x*context->scale;
    sp->me.y = to->y*context->scale;
    sp->me.pnum = -1;
    cp = chunkalloc(sizeof(BasePoint));
    cp->x = _cp->x*context->scale;
    cp->y = _cp->y*context->scale;
    cp->pnum = -1;
    sp->prevcp = context->last->nextcp = cp;
    ConicMake(context->last,sp);
    context->last = sp;

    if ( context->orig_sp!=NULL ) {
	context->orig_sp = context->orig_sp->next->to;
	if ( context->orig_sp!=NULL ) {
	    sp->me.pnum = context->orig_sp->me.pnum;
	    if ( context->orig_sp->nextcp!=NULL )
		cp->pnum = context->orig_sp->nextcp->pnum;
	}
    }
return( 0 );
}

static FT_Outline_Funcs outlinefuncs = {
    FT_MoveTo,
    FT_LineTo,
    FT_ConicTo,
    FT_CubicTo,
    0,0		/* I don't understand shift and delta */
};

/* Eventually I want to create baby ttf fonts containing only the glyph I need*/
/*  and its componants. Also need: head, hhea, hmtx, maxp, prep, fpgm, loca, */
/*  OS/2, post. Some of these are just direct copies, others (hmtx) are shrunk */
static FT_Error FreeType_FaceForCV(CharView *cv, FT_Face *face) {
    TtfFile *tfile = cv->cc->parent->tfile;
    int i;

    /* Ttf_MakeTinyFont(cv->cc);*/	/* This makes the tiny font. Now figure out how to use it!!!! */

    for ( i=0; i<tfile->font_cnt; ++i )
	if ( tfile->fonts[i]==cv->cc->parent->tfont )
    break;
return( _FT_New_Face(context,tfile->filename,i,face) );
}

void FreeType_GridFitChar(CharView *cv) {
    FT_Face face;
    struct freetype_raster *ret;
    FT_GlyphSlot slot;
    struct ft_context outline_context;

    ConicPointListsFree(cv->gridfit); cv->gridfit = NULL;
    FreeType_FreeRaster(cv->raster); cv->raster = (void *) -1;	/* set error mark */

    if ( !freetype_init())
return;
    if ( FreeType_FaceForCV(cv,&face) )
return;
    if ( _FT_Set_Pixel_Sizes(face,0,cv->show.ppem)) {
	_FT_Done_Face(face);
return;
    }

    if ( _FT_Load_Glyph(face,cv->cc->glyph,FT_LOAD_NO_BITMAP)) {
	_FT_Done_Face(face);
return;
    }

    slot = face->glyph;
    memset(&outline_context,'\0',sizeof(outline_context));
    /* The outline's position is expressed in 24.6 fixed numbers representing */
    /*  pixels. I want to scale it back to the original coordinate system */
    outline_context.scale = cv->cc->parent->em/(64.0*cv->show.ppem);
    outline_context.orig_cpl = cv->cc->conics;
    outline_context.orig_sp = NULL;
    if ( !_FT_Outline_Decompose(&slot->outline,&outlinefuncs,&outline_context)) {
	FT_ClosePath(&outline_context);
	cv->gridfit = outline_context.hcpl;
    }
    cv->gridwidth = outline_context.scale*slot->advance.x;

#if 0		/* If I want to look at bitmaps... */
    _FT_Load_Glyph(face,cv->cc->glyph,FT_LOAD_DEFAULT);
    if ( slot->format!=ft_glyph_format_bitmap )
#endif
    if ( _FT_Render_Glyph(slot,ft_render_mode_mono)) {
	_FT_Done_Face(face);
return;
    }

    if ( slot->bitmap.pixel_mode!=ft_pixel_mode_mono ) {
	_FT_Done_Face(face);
return;
    }
    ret = galloc(sizeof(struct freetype_raster));

    ret->rows = slot->bitmap.rows;
    ret->cols = slot->bitmap.width;
    ret->bytes_per_row = slot->bitmap.pitch;
    ret->as = slot->bitmap_top;
    ret->lb = slot->bitmap_left;
    ret->num_greys = slot->bitmap.num_grays;
	/* Can't find any description of freetype's bitendianness */
	/* These guys will probably be greyscale anyway... */
    ret->bitmap = galloc(ret->rows*ret->bytes_per_row);
    memcpy(ret->bitmap,slot->bitmap.buffer,ret->rows*ret->bytes_per_row);
    cv->raster = ret;
    _FT_Done_Face(face);
}
# endif
#else
/* ANSI C says a file must define something. It might as well defined a noop */
/*  version of one of the things it is supposed to define */
void FreeType_FreeRaster(struct freetype_raster *raster) {
}
#endif

#if TT_CONFIG_OPTION_BYTECODE_DEBUG && TT_CONFIG_OPTION_FREETYPE_DEBUG && TT_CONFIG_OPTION_BYTECODE_INTERPRETER
/* This code is inspired by ttdebug.c in ft2demos and David Turner's kind */
/*  suggestions. */

#undef  PACK
#define PACK( x, y )  ( ( x << 4 ) | y )

/* And this table is taken directly from ttinterp.c */
static const FT_Byte  Pop_Push_Use_Count[256][2] = {
    /* opcodes are gathered in groups of 16 */
    /* please keep the spaces as they are   */

    /*  SVTCA  y  */  PACK( 0, 0 ), 0,
    /*  SVTCA  x  */  PACK( 0, 0 ), 0,
    /*  SPvTCA y  */  PACK( 0, 0 ), 0,
    /*  SPvTCA x  */  PACK( 0, 0 ), 0,
    /*  SFvTCA y  */  PACK( 0, 0 ), 0,
    /*  SFvTCA x  */  PACK( 0, 0 ), 0,
    /*  SPvTL //  */  PACK( 2, 0 ), 0,
    /*  SPvTL +   */  PACK( 2, 0 ), 0,
    /*  SFvTL //  */  PACK( 2, 0 ), 0,
    /*  SFvTL +   */  PACK( 2, 0 ), 0,
    /*  SPvFS     */  PACK( 2, 0 ), 0,
    /*  SFvFS     */  PACK( 2, 0 ), 0,
    /*  GPV       */  PACK( 0, 2 ), 0,
    /*  GFV       */  PACK( 0, 2 ), 0,
    /*  SFvTPv    */  PACK( 0, 0 ), 0,
    /*  ISECT     */  PACK( 5, 0 ), 0,

    /*  SRP0      */  PACK( 1, 0 ), 0,
    /*  SRP1      */  PACK( 1, 0 ), 0,
    /*  SRP2      */  PACK( 1, 0 ), 0,
    /*  SZP0      */  PACK( 1, 0 ), 0,
    /*  SZP1      */  PACK( 1, 0 ), 0,
    /*  SZP2      */  PACK( 1, 0 ), 0,
    /*  SZPS      */  PACK( 1, 0 ), 0,
    /*  SLOOP     */  PACK( 1, 0 ), 0,
    /*  RTG       */  PACK( 0, 0 ), 0,
    /*  RTHG      */  PACK( 0, 0 ), 0,
    /*  SMD       */  PACK( 1, 0 ), 0,
    /*  ELSE      */  PACK( 0, 0 ), 0,
    /*  JMPR      */  PACK( 1, 0 ), 0,
    /*  SCvTCi    */  PACK( 1, 0 ), 0,
    /*  SSwCi     */  PACK( 1, 0 ), 0,
    /*  SSW       */  PACK( 1, 0 ), 0,

    /*  DUP       */  PACK( 1, 2 ), 0,
    /*  POP       */  PACK( 1, 0 ), 0,
    /*  CLEAR     */  PACK( 0, 0 ), 0,
    /*  SWAP      */  PACK( 2, 2 ), 0,
    /*  DEPTH     */  PACK( 0, 1 ), 0,
    /*  CINDEX    */  PACK( 1, 1 ), 0,
    /*  MINDEX    */  PACK( 1, 0 ), 0,
    /*  AlignPTS  */  PACK( 2, 0 ), 0,
    /*  INS_$28   */  PACK( 0, 0 ), 0,
    /*  UTP       */  PACK( 1, 0 ), 0,
    /*  LOOPCALL  */  PACK( 2, 0 ), 0,
    /*  CALL      */  PACK( 1, 0 ), 0,
    /*  FDEF      */  PACK( 1, 0 ), 0,
    /*  ENDF      */  PACK( 0, 0 ), 0,
    /*  MDAP[0]   */  PACK( 1, 0 ), 0,
    /*  MDAP[1]   */  PACK( 1, 0 ), 0,

    /*  IUP[0]    */  PACK( 0, 0 ), 0,
    /*  IUP[1]    */  PACK( 0, 0 ), 0,
    /*  SHP[0]    */  PACK( 0, 0 ), ttf_rp2|ttf_inloop,
    /*  SHP[1]    */  PACK( 0, 0 ), ttf_rp1|ttf_inloop,
    /*  SHC[0]    */  PACK( 1, 0 ), ttf_rp1,
    /*  SHC[1]    */  PACK( 1, 0 ), ttf_rp1,
    /*  SHZ[0]    */  PACK( 1, 0 ), ttf_rp1,
    /*  SHZ[1]    */  PACK( 1, 0 ), ttf_rp1,
    /*  SHPIX     */  PACK( 1, 0 ), ttf_inloop,
    /*  IP        */  PACK( 0, 0 ), ttf_rp1|ttf_rp2|ttf_inloop,
    /*  MSIRP[0]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MSIRP[1]  */  PACK( 2, 0 ), ttf_rp0,
    /*  AlignRP   */  PACK( 0, 0 ), ttf_rp0|ttf_inloop,
    /*  RTDG      */  PACK( 0, 0 ), 0,
    /*  MIAP[0]   */  PACK( 2, 0 ), 0,
    /*  MIAP[1]   */  PACK( 2, 0 ), 0,

    /*  NPushB    */  PACK( 0, 0 ), 0,
    /*  NPushW    */  PACK( 0, 0 ), 0,
    /*  WS        */  PACK( 2, 0 ), 0,
    /*  RS        */  PACK( 1, 1 ), 0,
    /*  WCvtP     */  PACK( 2, 0 ), 0,
    /*  RCvt      */  PACK( 1, 1 ), 0,
    /*  GC[0]     */  PACK( 1, 1 ), 0,
    /*  GC[1]     */  PACK( 1, 1 ), 0,
    /*  SCFS      */  PACK( 2, 0 ), 0,
    /*  MD[0]     */  PACK( 2, 1 ), 0,
    /*  MD[1]     */  PACK( 2, 1 ), 0,
    /*  MPPEM     */  PACK( 0, 1 ), 0,
    /*  MPS       */  PACK( 0, 1 ), 0,
    /*  FlipON    */  PACK( 0, 0 ), 0,
    /*  FlipOFF   */  PACK( 0, 0 ), 0,
    /*  DEBUG     */  PACK( 1, 0 ), 0,

    /*  LT        */  PACK( 2, 1 ), 0,
    /*  LTEQ      */  PACK( 2, 1 ), 0,
    /*  GT        */  PACK( 2, 1 ), 0,
    /*  GTEQ      */  PACK( 2, 1 ), 0,
    /*  EQ        */  PACK( 2, 1 ), 0,
    /*  NEQ       */  PACK( 2, 1 ), 0,
    /*  ODD       */  PACK( 1, 1 ), 0,
    /*  EVEN      */  PACK( 1, 1 ), 0,
    /*  IF        */  PACK( 1, 0 ), 0,
    /*  EIF       */  PACK( 0, 0 ), 0,
    /*  AND       */  PACK( 2, 1 ), 0,
    /*  OR        */  PACK( 2, 1 ), 0,
    /*  NOT       */  PACK( 1, 1 ), 0,
    /*  DeltaP1   */  PACK( 1, 0 ), 0,
    /*  SDB       */  PACK( 1, 0 ), 0,
    /*  SDS       */  PACK( 1, 0 ), 0,

    /*  ADD       */  PACK( 2, 1 ), 0,
    /*  SUB       */  PACK( 2, 1 ), 0,
    /*  DIV       */  PACK( 2, 1 ), 0,
    /*  MUL       */  PACK( 2, 1 ), 0,
    /*  ABS       */  PACK( 1, 1 ), 0,
    /*  NEG       */  PACK( 1, 1 ), 0,
    /*  FLOOR     */  PACK( 1, 1 ), 0,
    /*  CEILING   */  PACK( 1, 1 ), 0,
    /*  ROUND[0]  */  PACK( 1, 1 ), 0,
    /*  ROUND[1]  */  PACK( 1, 1 ), 0,
    /*  ROUND[2]  */  PACK( 1, 1 ), 0,
    /*  ROUND[3]  */  PACK( 1, 1 ), 0,
    /*  NROUND[0] */  PACK( 1, 1 ), 0,
    /*  NROUND[1] */  PACK( 1, 1 ), 0,
    /*  NROUND[2] */  PACK( 1, 1 ), 0,
    /*  NROUND[3] */  PACK( 1, 1 ), 0,

    /*  WCvtF     */  PACK( 2, 0 ), 0,
    /*  DeltaP2   */  PACK( 1, 0 ), 0,
    /*  DeltaP3   */  PACK( 1, 0 ), 0,
    /*  DeltaCn[0] */ PACK( 1, 0 ), 0,
    /*  DeltaCn[1] */ PACK( 1, 0 ), 0,
    /*  DeltaCn[2] */ PACK( 1, 0 ), 0,
    /*  SROUND    */  PACK( 1, 0 ), 0,
    /*  S45Round  */  PACK( 1, 0 ), 0,
    /*  JROT      */  PACK( 2, 0 ), 0,
    /*  JROF      */  PACK( 2, 0 ), 0,
    /*  ROFF      */  PACK( 0, 0 ), 0,
    /*  INS_$7B   */  PACK( 0, 0 ), 0,
    /*  RUTG      */  PACK( 0, 0 ), 0,
    /*  RDTG      */  PACK( 0, 0 ), 0,
    /*  SANGW     */  PACK( 1, 0 ), 0,
    /*  AA        */  PACK( 1, 0 ), 0,

    /*  FlipPT    */  PACK( 0, 0 ), ttf_inloop,
    /*  FlipRgON  */  PACK( 2, 0 ), 0,
    /*  FlipRgOFF */  PACK( 2, 0 ), 0,
    /*  INS_$83   */  PACK( 0, 0 ), 0,
    /*  INS_$84   */  PACK( 0, 0 ), 0,
    /*  ScanCTRL  */  PACK( 1, 0 ), 0,
    /*  SDVPTL[0] */  PACK( 2, 0 ), 0,
    /*  SDVPTL[1] */  PACK( 2, 0 ), 0,
    /*  GetINFO   */  PACK( 1, 1 ), 0,
    /*  IDEF      */  PACK( 1, 0 ), 0,
    /*  ROLL      */  PACK( 3, 3 ), 0,
    /*  MAX       */  PACK( 2, 1 ), 0,
    /*  MIN       */  PACK( 2, 1 ), 0,
    /*  ScanTYPE  */  PACK( 1, 0 ), 0,
    /*  InstCTRL  */  PACK( 2, 0 ), 0,
    /*  INS_$8F   */  PACK( 0, 0 ), 0,

    /*  INS_$90  */   PACK( 0, 0 ), 0,
    /*  INS_$91  */   PACK( 0, 0 ), 0,
    /*  INS_$92  */   PACK( 0, 0 ), 0,
    /*  INS_$93  */   PACK( 0, 0 ), 0,
    /*  INS_$94  */   PACK( 0, 0 ), 0,
    /*  INS_$95  */   PACK( 0, 0 ), 0,
    /*  INS_$96  */   PACK( 0, 0 ), 0,
    /*  INS_$97  */   PACK( 0, 0 ), 0,
    /*  INS_$98  */   PACK( 0, 0 ), 0,
    /*  INS_$99  */   PACK( 0, 0 ), 0,
    /*  INS_$9A  */   PACK( 0, 0 ), 0,
    /*  INS_$9B  */   PACK( 0, 0 ), 0,
    /*  INS_$9C  */   PACK( 0, 0 ), 0,
    /*  INS_$9D  */   PACK( 0, 0 ), 0,
    /*  INS_$9E  */   PACK( 0, 0 ), 0,
    /*  INS_$9F  */   PACK( 0, 0 ), 0,

    /*  INS_$A0  */   PACK( 0, 0 ), 0,
    /*  INS_$A1  */   PACK( 0, 0 ), 0,
    /*  INS_$A2  */   PACK( 0, 0 ), 0,
    /*  INS_$A3  */   PACK( 0, 0 ), 0,
    /*  INS_$A4  */   PACK( 0, 0 ), 0,
    /*  INS_$A5  */   PACK( 0, 0 ), 0,
    /*  INS_$A6  */   PACK( 0, 0 ), 0,
    /*  INS_$A7  */   PACK( 0, 0 ), 0,
    /*  INS_$A8  */   PACK( 0, 0 ), 0,
    /*  INS_$A9  */   PACK( 0, 0 ), 0,
    /*  INS_$AA  */   PACK( 0, 0 ), 0,
    /*  INS_$AB  */   PACK( 0, 0 ), 0,
    /*  INS_$AC  */   PACK( 0, 0 ), 0,
    /*  INS_$AD  */   PACK( 0, 0 ), 0,
    /*  INS_$AE  */   PACK( 0, 0 ), 0,
    /*  INS_$AF  */   PACK( 0, 0 ), 0,

    /*  PushB[0]  */  PACK( 0, 1 ), 0,
    /*  PushB[1]  */  PACK( 0, 2 ), 0,
    /*  PushB[2]  */  PACK( 0, 3 ), 0,
    /*  PushB[3]  */  PACK( 0, 4 ), 0,
    /*  PushB[4]  */  PACK( 0, 5 ), 0,
    /*  PushB[5]  */  PACK( 0, 6 ), 0,
    /*  PushB[6]  */  PACK( 0, 7 ), 0,
    /*  PushB[7]  */  PACK( 0, 8 ), 0,
    /*  PushW[0]  */  PACK( 0, 1 ), 0,
    /*  PushW[1]  */  PACK( 0, 2 ), 0,
    /*  PushW[2]  */  PACK( 0, 3 ), 0,
    /*  PushW[3]  */  PACK( 0, 4 ), 0,
    /*  PushW[4]  */  PACK( 0, 5 ), 0,
    /*  PushW[5]  */  PACK( 0, 6 ), 0,
    /*  PushW[6]  */  PACK( 0, 7 ), 0,
    /*  PushW[7]  */  PACK( 0, 8 ), 0,

    /*  MDRP[00]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[01]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[02]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[03]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[04]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[05]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[06]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[07]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[08]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[09]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[10]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[11]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[12]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[13]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[14]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[15]  */  PACK( 1, 0 ), ttf_rp0,

    /*  MDRP[16]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[17]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[18]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[19]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[20]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[21]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[22]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[23]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[24]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[25]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[26]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[27]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[28]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[29]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[30]  */  PACK( 1, 0 ), ttf_rp0,
    /*  MDRP[31]  */  PACK( 1, 0 ), ttf_rp0,

    /*  MIRP[00]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[01]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[02]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[03]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[04]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[05]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[06]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[07]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[08]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[09]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[10]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[11]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[12]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[13]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[14]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[15]  */  PACK( 2, 0 ), ttf_rp0,

    /*  MIRP[16]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[17]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[18]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[19]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[20]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[21]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[22]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[23]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[24]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[25]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[26]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[27]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[28]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[29]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[30]  */  PACK( 2, 0 ), ttf_rp0,
    /*  MIRP[31]  */  PACK( 2, 0 ), ttf_rp0
  };

static CharView		*debugcv;

static void PointMoved( TT_ExecContext exc, FT_Vector *old, int pt,
	int val1, int val2, int rp0 ) {
    struct ttfactions *act = gcalloc(1,sizeof(struct ttfactions));
    struct ttfactions *test, *prev;
    int c;
    int old_opcode = exc->code[exc->IP-1];

    act->pnum = pt;
    act->interp = (old_opcode==ttf_ip || old_opcode==ttf_iup || old_opcode==ttf_iup+1)?
	    -2:-1;
    act->basedon = -1;
    act->distance = 0;
    act->cvt_entry = -1;
    act->min = 0;
    act->rounded = 0;
    if ( old_opcode==ttf_msirp || old_opcode==ttf_msirp+1 ) {
	act->basedon = rp0;
	act->distance = val2;
    } else if ( old_opcode==ttf_miap || old_opcode==ttf_miap+1 ) {
	act->cvt_entry = val2;
    } else if ( old_opcode==ttf_mdap || old_opcode==ttf_mdap+1 ) {
	act->rounded = (old_opcode&4)?1:0;
    } else if ( old_opcode>=ttf_mirp ) {
	act->basedon = rp0;
	act->cvt_entry = val2;
	act->min = (old_opcode&8)?1:0;
	act->rounded = (old_opcode&4)?1:0;
    } else if ( old_opcode>=ttf_mdrp ) {
	act->basedon = rp0;
	act->min = (old_opcode&8)?1:0;
	act->rounded = (old_opcode&4)?1:0;
    }
    act->infunc = -1;
    act->instr = debugcv->cc->instrdata.instrs + exc->IP-1;
    if ( exc->callTop!=0 )
	act->instr = debugcv->cc->instrdata.instrs + exc->callStack[0].Caller_IP;
    act->freedom.x = (double) (((int)exc->GS.freeVector.x<<16)>>(16+14)) + ((exc->GS.freeVector.x&0x3fff)/16384.0);
    act->freedom.y = (double) (((int)exc->GS.freeVector.y<<16)>>(16+14)) + ((exc->GS.freeVector.y&0x3fff)/16384.0);
    act->was.x = old[pt].x;
    act->was.y = old[pt].y;
    act->is.x = exc->pts.cur[pt].x;
    act->is.y = exc->pts.cur[pt].y;

    c = (act->freedom.x!=0?1:0) + (act->freedom.y!=0?2:0);
    for ( prev=NULL, test=debugcv->instrinfo.acts; test!=NULL && (pt>test->pnum ||
	    (pt==test->pnum && c>=(test->freedom.x!=0?1:0) + (test->freedom.y!=0?2:0)));
	    prev = test, test = test->acts );
    if ( prev==NULL )
	debugcv->instrinfo.acts = act;
    else
	prev->acts = act;
    act->acts = test;
    ++debugcv->instrinfo.act_cnt;
}

static FT_Error RunIns( TT_ExecContext exc ) {
    FT_Error ret=0;
    FT_Vector *old;
    struct ttfargs *args;
    int val1, val2, rp0, i, c;
    double scale = 64.0*debugcv->show.ppem/debugcv->cc->parent->em;

    exc->instruction_trap = 1;
    old = galloc(exc->pts.n_points*sizeof(FT_Vector));

    while ( true ) {
	if ( exc->curRange == (FT_Int)tt_coderange_glyph && exc->callTop==0 &&
/* How do I tell if I'm executing instructions in a subcomponant of a composit? */
		exc->IP < debugcv->cc->instrdata.instr_cnt &&
		debugcv->instrinfo.args!=NULL ) {
	    args = &debugcv->instrinfo.args[exc->IP];
	    if ( args->loopcnt==0 ) {
		args->rp0val = exc->GS.rp0;
		args->rp1val = exc->GS.rp1;
		args->rp2val = exc->GS.rp2;
		args->rp2val = exc->GS.rp2;
		if ( exc->GS.gep0 ) args->zs |= 1;
		if ( exc->GS.gep1 ) args->zs |= 2;
		if ( exc->GS.gep2 ) args->zs |= 4;
		args->used = Pop_Push_Use_Count[exc->code[exc->IP]][1];
		if ( exc->GS.loop<=1 )
		    args->used &= ~ttf_inloop;
		if ( (Pop_Push_Use_Count[exc->code[exc->IP]][0]>>4)>=1 ) {
		    args->used |= ttf_sp0;
		    args->spvals[0] = exc->stack[exc->top-1];
		    if ( (Pop_Push_Use_Count[exc->code[exc->IP]][0]>>4)>=2 ) {
			args->used |= ttf_sp1;
			args->spvals[1] = exc->stack[exc->top-3];
		    }
		}
	    }
	    ++args->loopcnt;
	}
	val2 = exc->stack[exc->top-1];
	val1 = exc->stack[exc->top-2];
	rp0 = exc->GS.rp0;
	memcpy(old,exc->pts.cur,exc->pts.n_points*sizeof(FT_Vector));
	ret = _TT_RunIns(exc);
/* How do I tell if I'm executing instructions in a subcomponant of a composit? */
	if ( exc->curRange == (FT_Int)tt_coderange_glyph && exc->callTop==0 &&
		exc->IP < debugcv->cc->instrdata.instr_cnt &&
		debugcv->instrinfo.args!=NULL ) {
	    args = &debugcv->instrinfo.args[exc->IP];
	    if ( args->loopcnt==1 && (Pop_Push_Use_Count[exc->code[exc->IP]][0]&0xf)>=1 ) {
		args->used |= ttf_pushed;
		args->pushed = exc->stack[exc->top-(Pop_Push_Use_Count[exc->code[exc->IP]][0]&0xf)];
		if ( (Pop_Push_Use_Count[exc->code[exc->IP]][0]&0xf)>=2 )
		    args->used |= ttf_pushedmore;
	    }
	}
	for ( i=0; i<exc->pts.n_points; ++i ) {
	    if ( old[i].x!=exc->pts.cur[i].x || old[i].y!=exc->pts.cur[i].y ) {
		PointMoved(exc,old,i,val1,val2,rp0);
	    }
	}
	if ( ret ||
		( exc->curRange == (FT_Int)tt_coderange_glyph && exc->IP >= exc->codeSize ) )
    break;
    }

    free(old);
    debugcv->twilight_cnt = exc->twilight.n_points;
    debugcv->twilight = galloc(debugcv->twilight_cnt*sizeof(BasePoint));
    for ( c=0; c<debugcv->twilight_cnt; ++c ) {
	debugcv->twilight[c].pnum = c;
	debugcv->twilight[c].x = exc->twilight.cur[c].x/scale;
	debugcv->twilight[c].y = exc->twilight.cur[c].y/scale;
    }
return( ret );
}

void CVGenerateGloss(CharView *cv) {
    /*struct ttfactions *acts;*/
    FT_Driver driver;	/* truetype driver */
    FT_Face face;

    free(cv->instrinfo.args);
    TtfActionsFree(cv->instrinfo.acts);
    free(cv->cvtvals);
    free(cv->twilight);
    cv->instrinfo.args = NULL;
    cv->instrinfo.acts = NULL;
    cv->instrinfo.act_cnt = 0;
    cv->twilight = NULL;
    cv->cvtvals = NULL;

    if ( cv->cc->refs!=NULL )	/* I can't figure out what character I'm in */
return;			/* (composite or which componant) so I get everything wrong */

    if ( !freetype_init())
return;
    driver = (FT_Driver)_FT_Get_Module( context, "truetype" );

    _FT_Set_Debug_Hook( context,
                       FT_DEBUG_HOOK_TRUETYPE,
                       (FT_DebugHook_Func)RunIns );

    if ( FreeType_FaceForCV(cv,&face) )
return;

    if ( ((TT_Face) face)->root.driver != driver ) {
	_FT_Done_Face(face);
return;
    }

    if ( _FT_Set_Pixel_Sizes(face,0,cv->show.ppem)) {
	_FT_Done_Face(face);
return;
    }

    debugcv = cv;
    cv->instrinfo.args = NULL;
    if ( cv->cc->refs==NULL )
	cv->instrinfo.args = gcalloc(cv->cc->instrdata.instr_cnt,sizeof(struct ttfargs));
	/* Don't know how to tell when I'm in the composit's instrs, and when */
	/*  in a componant's */
    _FT_Load_Glyph(face,cv->cc->glyph,FT_LOAD_NO_BITMAP);
    _FT_Done_Face(face);
}
#endif
