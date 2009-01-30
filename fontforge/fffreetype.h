/* Copyright (C) 2000-2009 by George Williams */
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

#if !defined(_NO_FREETYPE) && !defined(_NO_MMAP)
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#if defined(FREETYPE_HAS_DEBUGGER) && FREETYPE_MINOR>=2
# include <internal/internal.h>
#endif
#include <unistd.h>
#include <sys/mman.h>

extern FT_Library ff_ft_context;

# if defined(_STATIC_LIBFREETYPE) || defined(NODYNAMIC)

#define _FT_Init_FreeType FT_Init_FreeType
#define _FT_New_Memory_Face FT_New_Memory_Face
#define _FT_Set_Pixel_Sizes FT_Set_Pixel_Sizes
#define _FT_Set_Char_Size FT_Set_Char_Size
#define _FT_Done_Face FT_Done_Face
#define _FT_Load_Glyph FT_Load_Glyph
#define _FT_Render_Glyph FT_Render_Glyph
#define _FT_Outline_Decompose FT_Outline_Decompose
#define _FT_Library_Version FT_Library_Version
#define _FT_Outline_Get_Bitmap FT_Outline_Get_Bitmap
#define _FT_Done_FreeType FT_Done_FreeType

# if FREETYPE_HAS_DEBUGGER
#  include "ttobjs.h"
#  include "ttdriver.h"
#  include "ttinterp.h"

#  define _FT_Set_Debug_Hook FT_Set_Debug_Hook
#  define _TT_RunIns TT_RunIns
# endif

#else		/* Do runtime linking */

extern FT_Error (*_FT_Init_FreeType)( FT_Library  * );
extern FT_Error (*_FT_Done_FreeType)( FT_Library  );
extern FT_Error (*_FT_Load_Glyph)( FT_Face, int, int);
extern FT_Error (*_FT_Set_Char_Size)( FT_Face, int wid/*=0*/, int height/* =ptsize*64*/, int hdpi, int vdpi);
extern FT_Error (*_FT_Outline_Get_Bitmap)(FT_Library, FT_Outline *,FT_Bitmap *);

# if FREETYPE_HAS_DEBUGGER
#  include "ttobjs.h"
#  include "ttdriver.h"
#  include "ttinterp.h"

extern void (*_FT_Set_Debug_Hook)(FT_Library, FT_UInt, FT_DebugHook_Func);
extern FT_Error (*_TT_RunIns)( TT_ExecContext );
# endif
#endif

typedef struct freetypecontext {
    SplineFont *sf;
    int layer;
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
    int em;			/* Em size in the spline font, not ppem */
} FTC;

extern void *__FreeTypeFontContext(FT_Library context,
	SplineFont *sf,SplineChar *sc,FontViewBase *fv,
	int layer,
	enum fontformat ff,int flags,void *shared_ftc);
#endif
