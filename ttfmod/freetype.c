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
/*  and its componants. Also need: head, hhea, hmtx, maxp, prep, fpgm, loca,
/*  OS/2, post. Some of these are just direct copies, others (hmtx) are shrunk */
void FreeType_GridFitChar(CharView *cv) {
    TtfFile *tfile = cv->cc->parent->tfile;
    int i;
    FT_Face face;
    struct freetype_raster *ret;
    FT_GlyphSlot slot;
    struct ft_context outline_context;

    ConicPointListsFree(cv->gridfit); cv->gridfit = NULL;
    FreeType_FreeRaster(cv->raster); cv->raster = (void *) -1;	/* set error mark */

    if ( !freetype_init())
return;
    for ( i=0; i<tfile->font_cnt; ++i )
	if ( tfile->fonts[i]==cv->cc->parent->tfont )
    break;
    if ( _FT_New_Face(context,tfile->filename,i,&face) )
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
