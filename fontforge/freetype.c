/* Copyright (C) 2000-2004 by George Williams */
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
#include <gwidget.h>
#include <ustring.h>
#include <math.h>

#if _NO_FREETYPE || _NO_MMAP
int hasFreeType(void) {
return( false );
}

int hasFreeTypeDebugger(void) {
return( false );
}

int hasFreeTypeByteCode(void) {
return( false );
}

void *_FreeTypeFontContext(SplineFont *sf,SplineChar *sc,FontView *fv,
	enum fontformat ff,int flags, void *share) {
return( NULL );
}

BDFFont *SplineFontFreeTypeRasterize(void *freetypecontext,int pixelsize,int depth) {
return( NULL );
}

BDFChar *SplineCharFreeTypeRasterize(void *freetypecontext,int enc,int pixelsize,int depth) {
return( NULL );
}

void FreeTypeFreeContext(void *freetypecontext) {
}

struct freetype_raster *FreeType_GetRaster(void *single_glyph_context,
	int enc, real ptsize, int dpi) {
return( NULL );
}

SplineSet *FreeType_GridFitChar(void *single_glyph_context,
	int enc, real ptsize, int dpi, int16 *width, SplineSet *splines) {
return( NULL );
}
#else
#include <ft2build.h>
#include FT_FREETYPE_H
#include <unistd.h>
#include <sys/mman.h>

static FT_Library context;

/* Ok, this complication is here because:				    */
/*	1) I want to be able to deal with static libraries on some systems  */
/*	2) If I've got a dynamic library and I compile up an executable     */
/*		I want it to run on systems without freetype		    */
/* So one case boils down to linking against the standard names, while the  */
/*  other does the link at run time if it's possible */

# if defined(_STATIC_LIBFREETYPE) || defined(NODYNAMIC)

#define _FT_Init_FreeType FT_Init_FreeType
#define _FT_New_Memory_Face FT_New_Memory_Face
#define _FT_Set_Pixel_Sizes FT_Set_Pixel_Sizes
#define _FT_Set_Char_Size FT_Set_Char_Size
#define _FT_Done_Face FT_Done_Face
#define _FT_Load_Glyph FT_Load_Glyph
#define _FT_Render_Glyph FT_Render_Glyph
#define _FT_Outline_Decompose FT_Outline_Decompose

# if FREETYPE_HAS_DEBUGGER
#  include "ttobjs.h"
#  include "ttdriver.h"
#  include "ttinterp.h"

#  define _FT_Set_Debug_Hook FT_Set_Debug_Hook
#  define _TT_RunIns TT_RunIns
#  define _FT_Done_FreeType FT_Done_FreeType
# endif

static int freetype_init_base() {
return( true );
}
# else
#  include <dynamic.h>
#  include <unistd.h>
#  include <sys/mman.h>

static DL_CONST void *libfreetype;
static FT_Error (*_FT_Init_FreeType)( FT_Library  * );
static FT_Error (*_FT_New_Memory_Face)( FT_Library, const FT_Byte *, int, int, FT_Face * );
static FT_Error (*_FT_Done_Face)( FT_Face );
static FT_Error (*_FT_Set_Pixel_Sizes)( FT_Face, int, int);
static FT_Error (*_FT_Set_Char_Size)( FT_Face, int wid/*=0*/, int height/* =ptsize*64*/, int hdpi, int vdpi);
static FT_Error (*_FT_Load_Glyph)( FT_Face, int, int);
static FT_Error (*_FT_Render_Glyph)( FT_GlyphSlot, int);
static FT_Error (*_FT_Outline_Decompose)(FT_Outline *, const FT_Outline_Funcs *,void *);

# if FREETYPE_HAS_DEBUGGER
#  include "ttobjs.h"
#  include "ttdriver.h"
#  include "ttinterp.h"

static void (*_FT_Set_Debug_Hook)(FT_Library, FT_UInt, FT_DebugHook_Func);
static FT_Error (*_TT_RunIns)( TT_ExecContext );
static FT_Error (*_FT_Done_FreeType)( FT_Library );
# endif

static int freetype_init_base() {
    libfreetype = dlopen("libfreetype" SO_EXT,RTLD_LAZY);
    if ( libfreetype==NULL )
return( false );

    _FT_Init_FreeType = (FT_Error (*)(FT_Library *)) dlsym(libfreetype,"FT_Init_FreeType");
    _FT_New_Memory_Face = (FT_Error (*)(FT_Library, const FT_Byte *, int, int, FT_Face * )) dlsym(libfreetype,"FT_New_Memory_Face");
    _FT_Set_Pixel_Sizes = (FT_Error (*)(FT_Face, int, int)) dlsym(libfreetype,"FT_Set_Pixel_Sizes");
    _FT_Set_Char_Size = (FT_Error (*)(FT_Face, int, int, int, int)) dlsym(libfreetype,"FT_Set_Char_Size");
    _FT_Done_Face = (FT_Error (*)(FT_Face)) dlsym(libfreetype,"FT_Done_Face");
    _FT_Load_Glyph = (FT_Error (*)(FT_Face, int, int)) dlsym(libfreetype,"FT_Load_Glyph");
    _FT_Render_Glyph = (FT_Error (*)(FT_GlyphSlot, int)) dlsym(libfreetype,"FT_Render_Glyph");
    _FT_Outline_Decompose = (FT_Error (*)(FT_Outline *, const FT_Outline_Funcs *,void *)) dlsym(libfreetype,"FT_Outline_Decompose");
#if FREETYPE_HAS_DEBUGGER
    _FT_Set_Debug_Hook = (void (*)(FT_Library, FT_UInt, FT_DebugHook_Func)) dlsym(libfreetype,"FT_Set_Debug_Hook");
    _TT_RunIns = (FT_Error (*)(TT_ExecContext)) dlsym(libfreetype,"TT_RunIns");
    _FT_Done_FreeType = (FT_Error (*)(FT_Library )) dlsym(libfreetype,"FT_Done_FreeType");
#endif
return( true );
}
# endif

int hasFreeType(void) {
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

int hasFreeTypeDebugger(void) {
    if ( !hasFreeType())
return( false );
#if FREETYPE_HAS_DEBUGGER
    if ( _FT_Set_Debug_Hook!=NULL && _TT_RunIns!=NULL )
return( true );
#endif

return( false );
}

int hasFreeTypeByteCode(void) {
    if ( !hasFreeType())
return( false );
#if defined(_STATIC_LIBFREETYPE) || defined(NODYNAMIC)
    /* In a static library, we can assume our headers are accurate */
# ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
return( true );
# else
return( false );
# endif
#elif FREETYPE_HAS_DEBUGGER
    /* Have we already checked for these data? */
    if ( _FT_Set_Debug_Hook!=NULL && _TT_RunIns!=NULL )
return( true );
    else
return( false );
#else
    {
    static int found = -1;
    if ( found==-1 )
	found = dlsym(libfreetype,"TT_RunIns")!=NULL;
return( found );
    }
#endif
}

typedef struct freetypecontext {
    SplineFont *sf;
    FILE *file;
    void *mappedfile;
    long len;
    int *glyph_indeces;
    FT_Face face;
    struct freetypecontext *shared_ftc;	/* file, mappedfile, glyph_indeces are shared with this ftc */
				/*  We have a new face, but that's it. This is so we can */
			        /*  have multiple pointsizes without loading the font many */
			        /*  times */
    int isttf;
    int em;
} FTC;

static void TransitiveClosureAdd(SplineChar **new,SplineChar **old,SplineChar *sc) {
    RefChar *ref;

    if ( new[sc->enc]!=NULL )	/* already done */
return;
    new[sc->enc] = sc;
    for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref = ref->next )
	TransitiveClosureAdd(new,old,ref->sc);
}

static void AddIf(SplineFont *sf,SplineChar **new,SplineChar **old,int unienc) {
    SplineChar *sc;

    sc = SFGetChar(sf,unienc,NULL);
    if ( sc!=NULL && SCWorthOutputting(sc))
	TransitiveClosureAdd(new,old,sc);
}

void FreeTypeFreeContext(void *freetypecontext) {
    FTC *ftc = freetypecontext;

    if ( ftc==NULL )
return;

    if ( ftc->face!=NULL )
	_FT_Done_Face(ftc->face);
    if ( ftc->shared_ftc )
return;
    if ( ftc->mappedfile )
	munmap(ftc->mappedfile,ftc->len);
    if ( ftc->file!=NULL )
	fclose(ftc->file);
    free(ftc->glyph_indeces);
    free(ftc);
}
    
static void *__FreeTypeFontContext(FT_Library context,
	SplineFont *sf,SplineChar *sc,FontView *fv,
	enum fontformat ff,int flags,void *shared_ftc) {
    /* build up a temporary font consisting of:
     *	sc!=NULL   => Just that character (and its references)
     *  fv!=NULL   => selected characters
     *  else	   => the entire font
     */
    FTC *ftc;
    SplineChar **old, **new;
    uint8 *selected = fv!=NULL ? fv->selected : NULL;
    int i,cnt;

    if ( !hasFreeType())
return( NULL );

    ftc = gcalloc(1,sizeof(FTC));
    if ( shared_ftc!=NULL ) {
	*ftc = *(FTC *) shared_ftc;
	ftc->face = NULL;
	ftc->shared_ftc = shared_ftc;
	ftc->em = ((FTC *) shared_ftc)->em;
    } else {
	ftc->sf = sf;
	ftc->em = sf->ascent+sf->descent;
	ftc->file = NULL;

	ftc->file = tmpfile();
	if ( ftc->file==NULL ) {
	    free(ftc);
return( NULL );
	}

	old = sf->chars;
	if ( sc!=NULL || selected!=NULL ) {
	    /* Build up a font consisting of those characters we actually use */
	    new = gcalloc(sf->charcnt,sizeof(SplineChar *));
	    if ( sc!=NULL )
		TransitiveClosureAdd(new,old,sc);
	    else for ( i=0; i<sf->charcnt; ++i )
		if ( selected[i] && SCWorthOutputting(old[i]))
		    TransitiveClosureAdd(new,old,old[i]);
	    /* Add these guys so we'll get reasonable blue values */
	    /* we won't rasterize them */
	    if ( PSDictHasEntry(sf->private,"BlueValues")==NULL ) {
		AddIf(sf,new,old,'I');
		AddIf(sf,new,old,'O');
		AddIf(sf,new,old,'x');
		AddIf(sf,new,old,'o');
	    }
	    AddIf(sf,new,old,0);	/* If there's a .notdef use it so that we don't generate our own .notdef (which can add cvt entries) */
	    sf->chars = new;
	}
	switch ( ff ) {
	  case ff_pfb: case ff_pfa:
	    if ( !_WritePSFont(ftc->file,sf,ff,0))
 goto fail;
	  break;
	  case ff_ttf: case ff_ttfsym:
	    ftc->isttf = true;
	    /* Fall through.... */
	  case ff_otf: case ff_otfcid:
	    if ( !_WriteTTFFont(ftc->file,sf,ff,NULL,bf_none,flags))
 goto fail;
	  break;
	  default:
 goto fail;
	}

	if ( sf->subfontcnt!=0 ) {
	    /* can only be an otfcid */
	    int k, max=0;
	    for ( k=0; k<sf->subfontcnt; ++k )
		if ( sf->subfonts[k]->charcnt>max )
		    max = sf->subfonts[k]->charcnt;
	    ftc->glyph_indeces = galloc(max*sizeof(int));
	    memset(ftc->glyph_indeces,-1,max*sizeof(int));
	    for ( i=0; i<max; ++i ) {
		for ( k=0; k<sf->subfontcnt; ++k ) {
		    if ( SCWorthOutputting(sf->subfonts[k]->chars[i]) ) {
			ftc->glyph_indeces[i] = sf->subfonts[k]->chars[i]->ttf_glyph;
		break;
		    }
		}
	    }
	} else {
	    ftc->glyph_indeces = galloc(sf->charcnt*sizeof(int));
	    memset(ftc->glyph_indeces,-1,sf->charcnt*sizeof(int));
	    cnt = 1; i = 0;
	    if ( SCIsNotdef(sf->chars[0], -1)) {
		ftc->glyph_indeces[0] = 0;
		i = 1;
	    }
	    for ( ; i<sf->charcnt; ++i ) {
		if ( SCWorthOutputting(sf->chars[i]) && SCDuplicate(sf->chars[i])==sf->chars[i] ) {
		    if ( ff==ff_pfa || ff==ff_pfb )
			ftc->glyph_indeces[i] = cnt++;
		    else
			ftc->glyph_indeces[i] = sf->chars[i]->ttf_glyph;
		}
	    }
	    for ( i=1 ; i<sf->charcnt; ++i ) {
		if ( SCWorthOutputting(sf->chars[i]) && SCDuplicate(sf->chars[i])!=sf->chars[i] ) {
		    SplineChar *sc = SCDuplicate(sf->chars[i]);
		    ftc->glyph_indeces[i] = ftc->glyph_indeces[sc->enc];
		}
	    }
	}

	fseek(ftc->file,0,SEEK_END);
	ftc->len = ftell(ftc->file);
	ftc->mappedfile = mmap(NULL,ftc->len,PROT_READ,MAP_PRIVATE,fileno(ftc->file),0);
	if ( ftc->mappedfile==MAP_FAILED )
 goto fail;
	if ( sf->chars!=old ) {
	    free(sf->chars);
	    sf->chars = old;
	}
    }

    if ( _FT_New_Memory_Face(context,ftc->mappedfile,ftc->len,0,&ftc->face))
 goto fail;
    
return( ftc );

 fail:
    FreeTypeFreeContext(ftc);
    if ( sf->chars!=old ) {
	free(sf->chars);
	sf->chars = old;
    }
return( NULL );
}

void *_FreeTypeFontContext(SplineFont *sf,SplineChar *sc,FontView *fv,
	enum fontformat ff,int flags,void *shared_ftc) {
return( __FreeTypeFontContext(context,sf,sc,fv,
	ff,flags,shared_ftc));
}

static void BCTruncateToDepth(BDFChar *bdfc,int depth) {
    int div = 255/((1<<depth)-1);
    int i,j;

    for ( i=0; i<=bdfc->ymax-bdfc->ymin; ++i ) {
	for ( j=0; j<bdfc->bytes_per_line; ++j )
	    bdfc->bitmap[i*bdfc->bytes_per_line+j] =
		    (bdfc->bitmap[i*bdfc->bytes_per_line+j]+div/2) / div;
    }
}

BDFChar *SplineCharFreeTypeRasterize(void *freetypecontext,int enc,
	int pixelsize,int depth) {
    FTC *ftc = freetypecontext;
    BDFChar *bdfc;
    SplineChar *sc;
    FT_GlyphSlot slot;

    if ( ftc->glyph_indeces[enc]==-1 )
 goto fail;
    if ( _FT_Set_Pixel_Sizes(ftc->face,0,pixelsize))
 goto fail;
    if ( _FT_Load_Glyph(ftc->face,ftc->glyph_indeces[enc],
	    depth==1?(FT_LOAD_RENDER|FT_LOAD_MONOCHROME):FT_LOAD_RENDER))
 goto fail;

    slot = ftc->face->glyph;
    sc = ftc->sf->chars[enc];
    bdfc = chunkalloc(sizeof(BDFChar));
    bdfc->sc = sc;
    bdfc->ymax = slot->bitmap_top-1;
    bdfc->ymin = slot->bitmap_top-slot->bitmap.rows;
    if ( slot->bitmap.rows==0 )
	bdfc->ymax = bdfc->ymin;
    bdfc->xmin = slot->bitmap_left;
    bdfc->xmax = slot->bitmap_left+slot->bitmap.width-1;
    if ( slot->bitmap.width==0 )
	bdfc->xmax = bdfc->xmin;
    bdfc->byte_data = (depth!=1);
    bdfc->depth = depth;
    if ( sc!=NULL ) {
	bdfc->width = rint(sc->width*pixelsize / (real) (sc->parent->ascent+sc->parent->descent));
	bdfc->enc = enc;
    }
    bdfc->bytes_per_line = slot->bitmap.pitch;
    if ( bdfc->bytes_per_line==0 ) bdfc->bytes_per_line = 1;
    bdfc->bitmap = galloc((bdfc->ymax-bdfc->ymin+1)*bdfc->bytes_per_line);
    if ( slot->bitmap.rows==0 || slot->bitmap.width==0 )
	memset(bdfc->bitmap,0,(bdfc->ymax-bdfc->ymin+1)*bdfc->bytes_per_line);
    else
	memcpy(bdfc->bitmap,slot->bitmap.buffer,slot->bitmap.rows*bdfc->bytes_per_line);
    BCCompressBitmap(bdfc);
    if ( depth!=1 && depth!=8 )
	BCTruncateToDepth(bdfc,depth);
return( bdfc );

 fail:
return( SplineCharRasterize(ftc->sf->chars[enc],pixelsize) );
}

BDFFont *SplineFontFreeTypeRasterize(void *freetypecontext,int pixelsize,int depth) {
    FTC *ftc = freetypecontext, *subftc=NULL;
    SplineFont *sf = ftc->sf, *subsf;
    int i,k;
    BDFFont *bdf = SplineFontToBDFHeader(sf,pixelsize,true);

    if ( depth!=1 )
	BDFClut(bdf, 1<<(depth/2) );

    k=0;
    do {
	if ( sf->subfontcnt==0 ) {
	    subsf = sf;
	    subftc = ftc;
	} else {
	    subsf = sf->subfonts[k];
	    subftc = FreeTypeFontContext(subsf,NULL,NULL);
	}
	for ( i=0; i<subsf->charcnt; ++i )
	    if ( SCWorthOutputting(subsf->chars[i] ) ) {
		/* If we could not allocate an ftc for this subfont, the revert to*/
		/*  our own rasterizer */
		if ( subftc!=NULL )
		    bdf->chars[i] = SplineCharFreeTypeRasterize(subftc,i,pixelsize,depth);
		else if ( depth==1 )
		    bdf->chars[i] = SplineCharRasterize(subsf->chars[i],pixelsize);
		else
		    bdf->chars[i] = SplineCharAntiAlias(subsf->chars[i],pixelsize,(1<<(depth/2)));
#if defined(FONTFORGE_CONFIG_GDRAW)
		GProgressNext();
#elif defined(FONTFORGE_CONFIG_GTK)
		gwwv_progress_next();
#endif
	    } else
		bdf->chars[i] = NULL;
	if ( subftc!=NULL && subftc!=ftc )
	    FreeTypeFreeContext(subftc);
	subftc = NULL;
	++k;
    } while ( k<sf->subfontcnt );
#if defined(FONTFORGE_CONFIG_GDRAW)
    GProgressEndIndicator();
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_end_indicator();
#endif
return( bdf );
}

/* ************************************************************************** */
struct ft_context {
    SplinePointList *hcpl, *lcpl, *cpl;
    SplinePoint *last;
    double scale;
    SplinePointList *orig_cpl;
    SplinePoint *orig_sp;
    int order2;
};

static void FT_ClosePath(struct ft_context *context) {
    if ( context->cpl!=NULL ) {
	if ( context->cpl->first->me.x != context->last->me.x ||
		context->cpl->first->me.y != context->last->me.y )
	    SplineMake(context->last,context->cpl->first,context->order2);
	else {
	    context->cpl->first->prevcp = context->last->prevcp;
	    context->last->prev->to = context->cpl->first;
	    context->cpl->first->prev = context->last->prev;
	    SplinePointFree(context->last);
	}
	context->cpl->last = context->cpl->first;
	context->last = NULL;
	if ( context->orig_cpl!=NULL )
	    context->orig_cpl = context->orig_cpl->next;
	context->orig_sp = NULL;
    }
}

static int FT_MoveTo(FT_Vector *to,void *user) {
    struct ft_context *context = user;

    FT_ClosePath(context);

    context->cpl = chunkalloc(sizeof(SplinePointList));
    if ( context->lcpl==NULL )
	context->hcpl = context->cpl;
    else
	context->lcpl->next = context->cpl;
    context->lcpl = context->cpl;

    if ( context->orig_cpl!=NULL )
	context->orig_sp = context->orig_cpl->first;

    context->last = context->cpl->first = chunkalloc(sizeof(SplinePoint));
    context->last->me.x = to->x*context->scale;
    context->last->me.y = to->y*context->scale;
    context->last->ttfindex = context->orig_sp?context->orig_sp->ttfindex: -2;
return( 0 );
}

static int FT_LineTo(FT_Vector *to,void *user) {
    struct ft_context *context = user;
    SplinePoint *sp;

    sp = SplinePointCreate( to->x*context->scale, to->y*context->scale );
    sp->ttfindex = -1;
    SplineMake(context->last,sp,context->order2);
    context->last = sp;

    if ( context->orig_sp!=NULL && context->orig_sp->next!=NULL ) {
	context->orig_sp = context->orig_sp->next->to;
	if ( context->orig_sp!=NULL )
	    sp->ttfindex = context->orig_sp->ttfindex;
    }
return( 0 );
}

static int FT_ConicTo(FT_Vector *_cp, FT_Vector *to,void *user) {
    struct ft_context *context = user;
    SplinePoint *sp;

    sp = SplinePointCreate( to->x*context->scale, to->y*context->scale );
    sp->noprevcp = false;
    sp->prevcp.x = _cp->x*context->scale;
    sp->prevcp.y = _cp->y*context->scale;
    context->last->nextcp = sp->prevcp;
    SplineMake2(context->last,sp);
    context->last = sp;

    if ( context->orig_sp!=NULL ) {
	context->orig_sp = context->orig_sp->next->to;
	if ( context->orig_sp!=NULL )
	    sp->ttfindex = context->orig_sp->ttfindex;
    }
return( 0 );
}

static int FT_CubicTo(FT_Vector *cp1, FT_Vector *cp2,FT_Vector *to,void *user) {
    struct ft_context *context = user;
    SplinePoint *sp;

    sp = SplinePointCreate( to->x*context->scale, to->y*context->scale );
    sp->noprevcp = false;
    sp->prevcp.x = cp2->x*context->scale;
    sp->prevcp.y = cp2->y*context->scale;
    context->last->nextcp.x = cp1->x*context->scale;
    context->last->nextcp.y = cp1->y*context->scale;
    SplineMake3(context->last,sp);
    context->last = sp;

    if ( context->orig_sp!=NULL ) {
	context->orig_sp = context->orig_sp->next->to;
	if ( context->orig_sp!=NULL )
	    sp->ttfindex = context->orig_sp->ttfindex;
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

SplineSet *FreeType_GridFitChar(void *single_glyph_context,
	int enc, real ptsize, int dpi, int16 *width, SplineSet *splines) {
    FT_GlyphSlot slot;
    FTC *ftc = (FTC *) single_glyph_context;
    struct ft_context outline_context;
    static int bc_checked = false;

    if ( ftc->face==(void *) -1 )
return( NULL );

    if ( !bc_checked && ftc->isttf ) {
	bc_checked = true;
	if ( !hasFreeTypeByteCode())
#if defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_notice(_("No ByteCode Interpreter"),_("These results are those of the freetype autohinter. They do not reflect the truetype instructions."));
#else
	    GWidgetPostNoticeR(_STR_NoByteCode,_STR_NoByteCodeMsg);
#endif
    }

    if ( _FT_Set_Char_Size(ftc->face,0,(int) (ptsize*64), dpi, dpi))
return( NULL );	/* Error Return */

    if ( _FT_Load_Glyph(ftc->face,ftc->glyph_indeces[enc],FT_LOAD_NO_BITMAP))
return( NULL );

    slot = ftc->face->glyph;
    memset(&outline_context,'\0',sizeof(outline_context));
    /* The outline's position is expressed in 24.6 fixed numbers representing */
    /*  pixels. I want to scale it back to the original coordinate system */
    outline_context.scale = ftc->em/(64.0*ptsize*dpi/72.0);
    outline_context.orig_cpl = splines;
    outline_context.orig_sp = NULL;
    outline_context.order2 = ftc->isttf;
    if ( !_FT_Outline_Decompose(&slot->outline,&outlinefuncs,&outline_context)) {
	FT_ClosePath(&outline_context);
	*width = outline_context.scale*slot->advance.x;
return( outline_context.hcpl );
    }
return( NULL );
}

struct freetype_raster *FreeType_GetRaster(void *single_glyph_context,
	int enc, real ptsize, int dpi) {
    FT_GlyphSlot slot;
    struct freetype_raster *ret;
    FTC *ftc = (FTC *) single_glyph_context;

    if ( ftc->face==(void *) -1 )
return( NULL );

    if ( _FT_Set_Char_Size(ftc->face,0,(int) (ptsize*64), dpi, dpi))
return( NULL );	/* Error Return */

    if ( _FT_Load_Glyph(ftc->face,ftc->glyph_indeces[enc],FT_LOAD_NO_BITMAP))
return( NULL );

    slot = ((FT_Face) (ftc->face))->glyph;
    if ( _FT_Render_Glyph(slot,ft_render_mode_mono))
return( NULL );

    if ( slot->bitmap.pixel_mode!=ft_pixel_mode_mono &&
	    slot->bitmap.pixel_mode!=ft_pixel_mode_grays )
return( NULL );
    ret = galloc(sizeof(struct freetype_raster));

    ret->rows = slot->bitmap.rows;
    ret->cols = slot->bitmap.width;
    ret->bytes_per_row = slot->bitmap.pitch;
    ret->as = slot->bitmap_top;
    ret->lb = slot->bitmap_left;
    ret->num_greys = slot->bitmap.num_grays;
	/* Can't find any description of freetype's bitendianness */
	/* But the obvious seems to work */
    ret->bitmap = galloc(ret->rows*ret->bytes_per_row);
    memcpy(ret->bitmap,slot->bitmap.buffer,ret->rows*ret->bytes_per_row);
return( ret );
}
#endif

void *FreeTypeFontContext(SplineFont *sf,SplineChar *sc,FontView *fv) {
return( _FreeTypeFontContext(sf,sc,fv,sf->subfontcnt!=0?ff_otfcid:ff_pfb,0,NULL) );
}

void FreeType_FreeRaster(struct freetype_raster *raster) {
    if ( raster==NULL || raster==(void *) -1 )
return;
    free(raster->bitmap);
    free(raster);
}

/******************************************************************************/
/* ***************************** Debugger Stuff ***************************** */
/******************************************************************************/

#if FREETYPE_HAS_DEBUGGER && !defined(FONTFORGE_CONFIG_NO_WINDOWING_UI)
#include <pthread.h>
#include <tterrors.h>

typedef struct bpdata {
    int range;	/* tt_coderange_glyph, tt_coderange_font, tt_coderange_cvt */
    int ip;
} BpData;

struct debugger_context {
    FT_Library context;
    FTC *ftc;
    /* I use a thread because freetype doesn't return, it just has a callback */
    /*  on each instruction. In actuallity only one thread should be executable*/
    /*  at a time (either main, or child) */
    pthread_t thread;
    pthread_mutex_t parent_mutex, child_mutex;
    pthread_cond_t parent_cond, child_cond;
    unsigned int terminate: 1;		/* The thread has been started simply to clean itself up and die */
    unsigned int has_mutexes: 1;
    unsigned int has_thread: 1;
    unsigned int has_finished: 1;
    unsigned int debug_fpgm: 1;
    unsigned int multi_step: 1;
    unsigned int found_wp: 1;
    unsigned int initted_pts: 1;
    int wp_ptindex;
    real ptsize;
    int dpi;
    TT_ExecContext exc;
    SplineChar *sc;
    BpData temp;
    BpData breaks[32];
    int bcnt;
    FT_Vector *oldpts;
    int n_points;
    uint8 *watch;
};

static int AtWp(struct debugger_context *dc, TT_ExecContext exc ) {
    int i, hit=false;

    dc->found_wp = false;
    if ( dc->watch==NULL )
return( false );

    for ( i=0; i<exc->pts.n_points; ++i ) {
	if ( dc->oldpts[i].x!=exc->pts.cur[i].x || dc->oldpts[i].y!=exc->pts.cur[i].y ) {
	    dc->oldpts[i] = exc->pts.cur[i];
	    if ( dc->watch[i] ) {
		hit = true;
		dc->wp_ptindex = i;
	    }
	}
    }
    dc->found_wp = hit;
return( hit );
}

static int AtBp(struct debugger_context *dc, TT_ExecContext exc ) {
    int i;

    if ( dc->temp.range==exc->curRange && dc->temp.ip==exc->IP ) {
	dc->temp.range = tt_coderange_none;
return( true );
    }

    for ( i=0; i<dc->bcnt; ++i ) {
	if ( dc->breaks[i].range==exc->curRange && dc->breaks[i].ip==exc->IP )
return( true );
    }
return( false );
}

static struct debugger_context *massive_kludge;

static FT_Error PauseIns( TT_ExecContext exc ) {
    int ret;
    struct debugger_context *dc = massive_kludge;

    if ( dc->terminate )
return( TT_Err_Execution_Too_Long );		/* Some random error code, says we're probably in a infinite loop */
    dc->exc = exc;

    if ( !dc->debug_fpgm && exc->curRange!=tt_coderange_glyph ) {
	exc->instruction_trap = 0;
return( _TT_RunIns(exc));	/* This should run to completion */
    }

    /* Set up for watch points */
    if ( dc->oldpts==NULL && exc->pts.n_points!=0 ) {
	dc->oldpts = gcalloc(exc->pts.n_points,sizeof(FT_Vector));
	dc->n_points = exc->pts.n_points;
    }
    if ( exc->pts.n_points!=0 && !dc->initted_pts ) {
	AtWp(dc,exc);
	dc->found_wp = false;
	dc->initted_pts = true;
    }

    pthread_mutex_lock(&dc->parent_mutex);
    pthread_cond_signal(&dc->parent_cond);
    pthread_mutex_unlock(&dc->parent_mutex);
    pthread_cond_wait(&dc->child_cond,&dc->child_mutex);
    if ( dc->terminate )
return( TT_Err_Execution_Too_Long );

    do {
	exc->instruction_trap = 1;
	ret = _TT_RunIns(exc);
	if ( ret )
    break;
	/* Signal the parent if we are single stepping, or if we've reached a break-point */
	if ( AtWp(dc,exc) || !dc->multi_step || AtBp(dc,exc) ||
		(exc->curRange==tt_coderange_glyph && exc->IP==exc->codeSize)) {
	    if ( dc->found_wp ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
		GWidgetPostNoticeR(_STR_HitWatchPoint,_STR_HitWatchPointn,dc->wp_ptindex);
#elif defined(FONTFORGE_CONFIG_GTK)
		gwwv_post_notice(_("Hit Watch Point"),_("Point %d was moved by the previous instruction"),dc->wp_ptindex);
#endif
		dc->found_wp = true;
	    }
	    pthread_mutex_lock(&dc->parent_mutex);
	    pthread_cond_signal(&dc->parent_cond);
	    pthread_mutex_unlock(&dc->parent_mutex);
	    pthread_cond_wait(&dc->child_cond,&dc->child_mutex);
	}
    } while ( !dc->terminate );

    if ( ret==TT_Err_Code_Overflow )
	ret = 0;

    massive_kludge = dc;	/* We set this again in case we are in a composite character where I think we get called several times (and some other thread might have set it) */
    if ( dc->terminate )
return( TT_Err_Execution_Too_Long );

return( ret );
}

static void *StartChar(void *_dc) {
    struct debugger_context *dc = _dc;

    pthread_mutex_lock(&dc->child_mutex);

    massive_kludge = dc;
    if ( (dc->ftc = __FreeTypeFontContext(dc->context,dc->sc->parent,dc->sc,NULL,
	    ff_ttf, 0, NULL))==NULL )
 goto finish;

    massive_kludge = dc;
    if ( _FT_Set_Char_Size(dc->ftc->face,0,(int) (dc->ptsize*64), dc->dpi, dc->dpi))
 goto finish;

    massive_kludge = dc;
    _FT_Load_Glyph(dc->ftc->face,dc->ftc->glyph_indeces[dc->sc->enc],FT_LOAD_NO_BITMAP);

 finish:
    dc->has_finished = true;
    dc->exc = NULL;
    pthread_mutex_lock(&dc->parent_mutex);
    pthread_cond_signal(&dc->parent_cond);	/* Wake up parent and get it to clean up after itself */
    pthread_mutex_unlock(&dc->parent_mutex);
    pthread_mutex_unlock(&dc->child_mutex);
return( NULL );
}

void DebuggerTerminate(struct debugger_context *dc) {
    if ( dc->has_thread ) {
	if ( !dc->has_finished ) {
	    dc->terminate = true;
	    pthread_mutex_lock(&dc->child_mutex);
	    pthread_cond_signal(&dc->child_cond);	/* Wake up child and get it to clean up after itself */
	    pthread_mutex_unlock(&dc->child_mutex);
	    pthread_mutex_unlock(&dc->parent_mutex);
	}
	pthread_join(dc->thread,NULL);
	dc->has_thread = false;
    }
    if ( dc->has_mutexes ) {
	pthread_cond_destroy(&dc->child_cond);
	pthread_cond_destroy(&dc->parent_cond);
	pthread_mutex_destroy(&dc->child_mutex);
	pthread_mutex_unlock(&dc->parent_mutex);
	pthread_mutex_destroy(&dc->parent_mutex);
    }
    if ( dc->ftc!=NULL )
	FreeTypeFreeContext(dc->ftc);
    if ( dc->context!=NULL )
	_FT_Done_FreeType( dc->context );
    free(dc->watch);
    free(dc->oldpts);
    free(dc);
}

void DebuggerReset(struct debugger_context *dc,real ptsize,int dpi,int dbg_fpgm) {
    /* Kill off the old thread, and start up a new one working on the given */
    /*  pointsize and resolution */ /* I'm not prepared for errors here */
    /* Note that if we don't want to look at the fpgm/prep code (and we */
    /*  usually don't) then we must turn off the debug hook when they get run */

    if ( dc->has_thread ) {
	dc->terminate = true;
	pthread_cond_signal(&dc->child_cond);	/* Wake up child and get it to clean up after itself */
	pthread_join(dc->thread,NULL);
	dc->has_thread = false;
    }
    if ( dc->ftc!=NULL )
	FreeTypeFreeContext(dc->ftc);

    dc->debug_fpgm = dbg_fpgm;
    dc->ptsize = ptsize;
    dc->dpi = dpi;
    dc->terminate = dc->has_finished = false;
    dc->initted_pts = false;

    pthread_create(&dc->thread,NULL,StartChar,(void *) dc);
    dc->has_thread = true;
    pthread_cond_wait(&dc->parent_cond,&dc->parent_mutex);
}

struct debugger_context *DebuggerCreate(SplineChar *sc,real ptsize,int dpi,int dbg_fpgm) {
    struct debugger_context *dc;

    if ( !hasFreeTypeDebugger())
return( NULL );

    dc = gcalloc(1,sizeof(struct debugger_context));
    dc->sc = sc;
    dc->debug_fpgm = dbg_fpgm;
    dc->ptsize = ptsize;
    dc->dpi = dpi;
    if ( _FT_Init_FreeType( &dc->context )) {
	free(dc);
return( NULL );
    }

    _FT_Set_Debug_Hook( dc->context,
		       FT_DEBUG_HOOK_TRUETYPE,
		       (FT_DebugHook_Func)PauseIns );

    pthread_mutex_init(&dc->parent_mutex,NULL); pthread_mutex_init(&dc->child_mutex,NULL);
    pthread_cond_init(&dc->parent_cond,NULL); pthread_cond_init(&dc->child_cond,NULL);
    dc->has_mutexes = true;

    pthread_mutex_lock(&dc->parent_mutex);
    if ( pthread_create(&dc->thread,NULL,StartChar,dc)!=0 ) {
	DebuggerTerminate( dc );
return( NULL );
    }
    dc->has_thread = true;
    pthread_cond_wait(&dc->parent_cond,&dc->parent_mutex);	/* Wait for the child to initialize itself (and stop) then we can look at its status */

return( dc );
}

void DebuggerGo(struct debugger_context *dc,enum debug_gotype dgt) {
    int opcode;

    if ( !dc->has_thread || dc->has_finished || dc->exc==NULL )
	DebuggerReset(dc,dc->ptsize,dc->dpi,dc->debug_fpgm);
    else {
	switch ( dgt ) {
	  case dgt_continue:
	    dc->multi_step = true;
	  break;
	  case dgt_stepout:
	    dc->multi_step = true;
	    if ( dc->exc->callTop>0 ) {
		dc->temp.range = dc->exc->callStack[dc->exc->callTop-1].Caller_Range;
		dc->temp.ip = dc->exc->callStack[dc->exc->callTop-1].Caller_IP;
	    }
	  break;
	  case dgt_next:
	    opcode = dc->exc->code[dc->exc->IP];
	    /* I've decided that IDEFs will get stepped into */
	    if ( opcode==0x2b /* call */ || opcode==0x2a /* loopcall */ ) {
		dc->temp.range = dc->exc->curRange;
		dc->temp.ip = dc->exc->IP+1;
		dc->multi_step = true;
	    } else
		dc->multi_step = false;
	  break;
	  default:
	  case dgt_step:
	    dc->multi_step = false;
	  break;
	}
	pthread_mutex_lock(&dc->child_mutex);
	pthread_cond_signal(&dc->child_cond);	/* Wake up child and get it to clean up after itself */
	pthread_mutex_unlock(&dc->child_mutex);
	pthread_cond_wait(&dc->parent_cond,&dc->parent_mutex);	/* Wait for the child to initialize itself (and stop) then we can look at its status */
    }
}

struct TT_ExecContextRec_ *DebuggerGetEContext(struct debugger_context *dc) {
return( dc->exc );
}

int DebuggerBpCheck(struct debugger_context *dc,int range,int ip) {
    int i;

    for ( i=0; i<dc->bcnt; ++i ) {
	if ( dc->breaks[i].range==range && dc->breaks[i].ip==ip )
return( true );
    }
return( false );
}

void DebuggerToggleBp(struct debugger_context *dc,int range,int ip) {
    int i;

    /* If the address has a bp, then remove it */
    for ( i=0; i<dc->bcnt; ++i ) {
	if ( dc->breaks[i].range==range && dc->breaks[i].ip==ip ) {
	    ++i;
	    while ( i<dc->bcnt ) {
		dc->breaks[i-1].range = dc->breaks[i].range;
		dc->breaks[i-1].ip = dc->breaks[i].ip;
		++i;
	    }
	    --dc->bcnt;
return;
	}
    }
    /* Else add it */
    if ( dc->bcnt>=sizeof(dc->breaks)/sizeof(dc->breaks[0]) ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	GWidgetErrorR(_STR_TooManyBreakpoints,_STR_TooManyBreakpoints);
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Too Many Breakpoints"),_("Too Many Breakpoints"));
#endif
return;
    }
    i = dc->bcnt++;
    dc->breaks[i].range = range;
    dc->breaks[i].ip = ip;
}

void DebuggerSetWatches(struct debugger_context *dc,int n, uint8 *w) {
    free(dc->watch); dc->watch=NULL;
    if ( n!=dc->n_points ) IError("Bad watchpoint count");
    else {
	dc->watch = w;
	if ( dc->exc ) {
	    AtWp(dc,dc->exc);
	    dc->found_wp = false;
	}
    }
}

uint8 *DebuggerGetWatches(struct debugger_context *dc, int *n) {
    *n = dc->n_points;
return( dc->watch );
}

int DebuggingFpgm(struct debugger_context *dc) {
return( dc->debug_fpgm );
}
#else
struct debugger_context;

void DebuggerTerminate(struct debugger_context *dc) {
}

void DebuggerReset(struct debugger_context *dc,real ptsize,int dpi,int dbg_fpgm) {
}

struct debugger_context *DebuggerCreate(SplineChar *sc,real pointsize,int dpi, int dbg_fpgm) {
return( NULL );
}

void DebuggerGo(struct debugger_context *dc,enum debug_gotype go) {
}

struct TT_ExecContextRec_ *DebuggerGetEContext(struct debugger_context *dc) {
return( NULL );
}

void DebuggerSetWatches(struct debugger_context *dc,int n, uint8 *w) {
}

uint8 *DebuggerGetWatches(struct debugger_context *dc, int *n) {
    *n = 0;
return( NULL );
}

int DebuggingFpgm(struct debugger_context *dc) {
return( false );
}
#endif	/* FREETYPE_HAS_DEBUGGER && !defined(FONTFORGE_CONFIG_NO_WINDOWING_UI)*/
