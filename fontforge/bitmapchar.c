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

BDFChar *BDFMakeGID(BDFFont *bdf,int gid) {
    SplineFont *sf=bdf->sf;
    SplineChar *sc;
    BDFChar *bc;
    int i;
    extern int use_freetype_to_rasterize_fv;

    if ( gid==-1 )
return( NULL );

    if ( sf->cidmaster!=NULL || sf->subfonts!=NULL ) {
	int j = SFHasCID(sf,gid);
	if ( sf->cidmaster ) sf = sf->cidmaster;
	if ( j==-1 ) {
	    for ( j=0; j<sf->subfontcnt; ++j )
		if ( gid<sf->subfonts[j]->glyphcnt )
	    break;
	    if ( j==sf->subfontcnt )
return( NULL );
	}
	sf = sf->subfonts[j];
    }
    if ( (sc = sf->glyphs[gid])==NULL )
return( NULL );

    if ( gid>=bdf->glyphcnt ) {
	if ( gid>=bdf->glyphmax )
	    bdf->glyphs = grealloc(bdf->glyphs,(bdf->glyphmax=sf->glyphmax)*sizeof(BDFChar *));
	for ( i=bdf->glyphcnt; i<=gid; ++i )
	    bdf->glyphs[i] = NULL;
	bdf->glyphcnt = sf->glyphcnt;
    }
    if ( (bc = bdf->glyphs[gid])==NULL ) {
	if ( use_freetype_to_rasterize_fv ) {
	    void *freetype_context = FreeTypeFontContext(sf,sc,NULL);
	    if ( freetype_context != NULL ) {
		bc = SplineCharFreeTypeRasterize(freetype_context,
			sc->orig_pos,bdf->pixelsize,bdf->clut?8:1);
		FreeTypeFreeContext(freetype_context);
	    }
	}
	if ( bc!=NULL )
	    /* Done */;
	else if ( bdf->clut==NULL )
	    bc = SplineCharRasterize(sc,bdf->pixelsize);
	else
	    bc = SplineCharAntiAlias(sc,bdf->pixelsize,BDFDepth(bdf));
	bdf->glyphs[gid] = bc;
	bc->orig_pos = gid;
	BCCharChangedUpdate(bc);
    }
return( bc );
}

BDFChar *BDFMakeChar(BDFFont *bdf,EncMap *map,int enc) {
    SplineFont *sf=bdf->sf;

    if ( enc==-1 )
return( NULL );

    if ( sf->cidmaster!=NULL ) {
	int j = SFHasCID(sf,enc);
	sf = sf->cidmaster;
	if ( j==-1 ) {
	    for ( j=0; j<sf->subfontcnt; ++j )
		if ( enc<sf->subfonts[j]->glyphcnt )
	    break;
	    if ( j==sf->subfontcnt )
return( NULL );
	}
	sf = sf->subfonts[j];
    }
    SFMakeChar(sf,map,enc);
return( BDFMakeGID(bdf,map->map[enc]));
}
