/* Copyright (C) 2000-2002 by George Williams */
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

void *FreeTypeFontContext(SplineFont *sf,SplineChar *sc,int doall) {
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
#define _FT_New_Face FT_New_Face
#define _FT_New_Memory_Face FT_New_Memory_Face
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
#include <unistd.h>
#include <sys/mman.h>

static void *libfreetype;
static FT_Error (*_FT_Init_FreeType)( FT_Library  * );
static FT_Error (*_FT_New_Face)( FT_Library, const char *, int, FT_Face * );
static FT_Error (*_FT_New_Memory_Face)( FT_Library, const FT_Byte *, int, int, FT_Face * );
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
    _FT_New_Memory_Face = (FT_Error (*)(FT_Library, const FT_Byte *, int, int, FT_Face * )) dlsym(libfreetype,"FT_New_Memory_Face");
    _FT_Set_Pixel_Sizes = (FT_Error (*)(FT_Face, int, int)) dlsym(libfreetype,"FT_Set_Pixel_Sizes");
    _FT_Done_Face = (FT_Error (*)(FT_Face)) dlsym(libfreetype,"FT_Done_Face");
    _FT_Load_Glyph = (FT_Error (*)(FT_Face, int, int)) dlsym(libfreetype,"FT_Load_Glyph");
    _FT_Render_Glyph = (FT_Error (*)(FT_GlyphSlot, int)) dlsym(libfreetype,"FT_Render_Glyph");
    _FT_Outline_Decompose = (FT_Error (*)(FT_Outline *, const FT_Outline_Funcs *,void *)) dlsym(libfreetype,"FT_Outline_Decompose");
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

typedef struct freetypecontext {
    SplineFont *sf;
    FILE *file;
    void *mappedfile;
    long len;
    int *glyph_indeces;
    FT_Face face;
} FTC;

static void TransitiveClosureAdd(SplineChar **new,SplineChar **old,SplineChar *sc) {
    RefChar *ref;

    if ( new[sc->enc]!=NULL )	/* already done */
return;
    new[sc->enc] = sc;
    for ( ref=sc->refs; ref!=NULL; ref = ref->next )
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
    if ( ftc->mappedfile )
	munmap(ftc->mappedfile,ftc->len);
    if ( ftc->file!=NULL )
	fclose(ftc->file);
    free(ftc->glyph_indeces);
    free(ftc);
}
    
void *FreeTypeFontContext(SplineFont *sf,SplineChar *sc,int doall) {
    /* build up a temporary font consisting of:
     *	sc!=NULL   => Just that character (and its references)
     *  doall	   => the entire font
     *  	   => only selected characters
     */
    FTC *ftc = gcalloc(1,sizeof(FTC));
    SplineChar **old, **new;
    char *selected = sf->fv->selected;
    int i,cnt;

    if ( !hasFreeType())
return( NULL );

    ftc->sf = sf;
    ftc->file = NULL;
    if ( sf->subfontcnt!=0 )
return( ftc );

    ftc->file = tmpfile();
    if ( ftc->file==NULL ) {
	free(ftc);
return( NULL );
    }

    old = sf->chars;
    if ( sc!=NULL || !doall ) {
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
	sf->chars = new;
    }
    if ( !_WritePSFont(ftc->file,sf,ff_pfb))
 goto fail;

    ftc->glyph_indeces = galloc(sf->charcnt*sizeof(int));
    memset(ftc->glyph_indeces,-1,sf->charcnt*sizeof(int));
    if ( SCWorthOutputting(sf->chars[0]))
	ftc->glyph_indeces[0] = 0;
    for ( i=cnt=1; i<sf->charcnt; ++i )
	if ( SCWorthOutputting(sf->chars[i]))
	    ftc->glyph_indeces[i] = cnt++;

    fseek(ftc->file,0,SEEK_END);
    ftc->len = ftell(ftc->file);
    ftc->mappedfile = mmap(NULL,ftc->len,PROT_READ,MAP_PRIVATE,fileno(ftc->file),0);
    if ( ftc->mappedfile==MAP_FAILED )
 goto fail;
    if ( _FT_New_Memory_Face(context,ftc->mappedfile,ftc->len,0,&ftc->face))
 goto fail;
    if ( sf->chars!=old ) {
	free(sf->chars);
	sf->chars = old;
    }
    
return( ftc );

 fail:
    FreeTypeFreeContext(ftc);
    if ( sf->chars!=old ) {
	free(sf->chars);
	sf->chars = old;
    }
return( NULL );
}

static void BCTruncateToDepth(BDFChar *bdfc,int depth) {
    int div = 255/((1<<(depth/2))-1);
    int i,j;

    for ( i=0; i<=bdfc->ymax-bdfc->ymin; ++i ) {
	for ( j=0; j<bdfc->bytes_per_line; ++j )
	    bdfc->bitmap[i*bdfc->bytes_per_line+j] /= div;
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
    bdfc = gcalloc(1,sizeof(BDFChar));
    bdfc->sc = sc;
    bdfc->xmin = slot->bitmap_left;
    bdfc->ymax = slot->bitmap_top;
    bdfc->ymin = slot->bitmap_top-slot->bitmap.rows+1;
    bdfc->xmax = slot->bitmap_left+slot->bitmap.width-1;
    bdfc->byte_data = (depth!=1);
    if ( sc!=NULL ) {
	bdfc->width = rint(sc->width*pixelsize / (real) (sc->parent->ascent+sc->parent->descent));
	bdfc->enc = enc;
    }
    bdfc->bytes_per_line = slot->bitmap.pitch;
    bdfc->bitmap = galloc(slot->bitmap.rows*bdfc->bytes_per_line);
    memcpy(bdfc->bitmap,slot->bitmap.buffer,slot->bitmap.rows*bdfc->bytes_per_line);
    if ( depth==1 )
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
	    subftc = FreeTypeFontContext(subsf,NULL,true);
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
		GProgressNext();
	    } else
		bdf->chars[i] = NULL;
	if ( subftc!=NULL && subftc!=ftc )
	    FreeTypeFreeContext(subftc);
	subftc = NULL;
	++k;
    } while ( k<sf->subfontcnt );
    GProgressEndIndicator();
return( bdf );
}
#endif
