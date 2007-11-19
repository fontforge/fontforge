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
#include <utype.h>
#include <ustring.h>

void SFUntickAll(SplineFont *sf) {
    int i;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	sf->glyphs[i]->ticked = false;
}

void SFOrderBitmapList(SplineFont *sf) {
    BDFFont *bdf, *prev, *next;
    BDFFont *bdf2, *prev2;

    for ( prev = NULL, bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	for ( prev2=NULL, bdf2=bdf->next; bdf2!=NULL; bdf2 = bdf2->next ) {
	    if ( bdf->pixelsize>bdf2->pixelsize ||
		    (bdf->pixelsize==bdf2->pixelsize && BDFDepth(bdf)>BDFDepth(bdf2)) ) {
		if ( prev==NULL )
		    sf->bitmaps = bdf2;
		else
		    prev->next = bdf2;
		if ( prev2==NULL ) {
		    bdf->next = bdf2->next;
		    bdf2->next = bdf;
		} else {
		    next = bdf->next;
		    bdf->next = bdf2->next;
		    bdf2->next = next;
		    prev2->next = bdf;
		}
		next = bdf;
		bdf = bdf2;
		bdf2 = next;
	    }
	    prev2 = bdf2;
	}
	prev = bdf;
    }
}

SplineChar *SCBuildDummy(SplineChar *dummy,SplineFont *sf,EncMap *map,int i) {
    static char namebuf[100];
#ifdef FONTFORGE_CONFIG_TYPE3
    static Layer layers[2];
#endif

    memset(dummy,'\0',sizeof(*dummy));
    dummy->color = COLOR_DEFAULT;
    dummy->layer_cnt = 2;
#ifdef FONTFORGE_CONFIG_TYPE3
    dummy->layers = layers;
#endif
    if ( sf->cidmaster!=NULL ) {
	/* CID fonts don't have encodings, instead we must look up the cid */
	if ( sf->cidmaster->loading_cid_map )
	    dummy->unicodeenc = -1;
	else
	    dummy->unicodeenc = CID2NameUni(FindCidMap(sf->cidmaster->cidregistry,sf->cidmaster->ordering,sf->cidmaster->supplement,sf->cidmaster),
		    i,namebuf,sizeof(namebuf));
    } else
	dummy->unicodeenc = UniFromEnc(i,map->enc);

    if ( sf->cidmaster!=NULL )
	dummy->name = namebuf;
    else if ( map->enc->psnames!=NULL && i<map->enc->char_cnt &&
	    map->enc->psnames[i]!=NULL )
	dummy->name = map->enc->psnames[i];
    else if ( dummy->unicodeenc==-1 )
	dummy->name = NULL;
    else
	dummy->name = (char *) StdGlyphName(namebuf,dummy->unicodeenc,sf->uni_interp,sf->for_new_glyphs);
    if ( dummy->name==NULL ) {
	/*if ( dummy->unicodeenc!=-1 || i<256 )
	    dummy->name = ".notdef";
	else*/ {
	    int j;
	    sprintf( namebuf, "NameMe.%d", i);
	    j=0;
	    while ( SFFindExistingSlot(sf,-1,namebuf)!=-1 )
		sprintf( namebuf, "NameMe.%d.%d", i, ++j);
	    dummy->name = namebuf;
	}
    }
    dummy->width = dummy->vwidth = sf->ascent+sf->descent;
    if ( dummy->unicodeenc>0 && dummy->unicodeenc<0x10000 &&
	    iscombining(dummy->unicodeenc)) {
	/* Mark characters should be 0 width */
	dummy->width = 0;
	/* Except in monospaced fonts on windows, where they should be the */
	/*  same width as everything else */
    }
    /* Actually, in a monospace font, all glyphs should be the same width */
    /*  whether mark or not */
    if ( sf->pfminfo.panose_set && sf->pfminfo.panose[3]==9 &&
	    sf->glyphcnt>0 ) {
	for ( i=sf->glyphcnt-1; i>=0; --i )
	    if ( SCWorthOutputting(sf->glyphs[i])) {
		dummy->width = sf->glyphs[i]->width;
	break;
	    }
    }
    dummy->parent = sf;
    dummy->orig_pos = 0xffff;
return( dummy );
}

static SplineChar *_SFMakeChar(SplineFont *sf,EncMap *map,int enc) {
    SplineChar dummy, *sc;
    SplineFont *ssf;
    int j, real_uni, gid;
    extern const int cns14pua[], amspua[];

    if ( enc>=map->enccount )
	gid = -1;
    else
	gid = map->map[enc];
    if ( sf->subfontcnt!=0 && gid!=-1 ) {
	ssf = NULL;
	for ( j=0; j<sf->subfontcnt; ++j )
	    if ( gid<sf->subfonts[j]->glyphcnt ) {
		ssf = sf->subfonts[j];
		if ( ssf->glyphs[gid]!=NULL ) {
return( ssf->glyphs[gid] );
		}
	    }
	sf = ssf;
    }

    if ( gid==-1 || (sc = sf->glyphs[gid])==NULL ) {
	if (( map->enc->is_unicodebmp || map->enc->is_unicodefull ) &&
		( enc>=0xe000 && enc<=0xf8ff ) &&
		( sf->uni_interp==ui_ams || sf->uni_interp==ui_trad_chinese ) &&
		( real_uni = (sf->uni_interp==ui_ams ? amspua : cns14pua)[enc-0xe000])!=0 ) {
	    if ( real_uni<map->enccount ) {
		SplineChar *sc;
		/* if necessary, create the real unicode code point */
		/*  and then make us be a duplicate of it */
		sc = _SFMakeChar(sf,map,real_uni);
		map->map[enc] = gid = sc->orig_pos;
		SCCharChangedUpdate(sc);
return( sc );
	    }
	}

	SCBuildDummy(&dummy,sf,map,enc);
	/* Let's say a user has a postscript encoding where the glyph ".notdef" */
	/*  is assigned to many slots. Once the user creates a .notdef glyph */
	/*  all those slots should fill in. If they don't they damn well better*/
	/*  when the user clicks on one to edit it */
	/* Used to do that with all encodings. It just confused people */
	if ( map->enc->psnames!=NULL &&
		(sc = SFGetChar(sf,dummy.unicodeenc,dummy.name))!=NULL ) {
	    map->map[enc] = sc->orig_pos;
	    AltUniAdd(sc,dummy.unicodeenc);
return( sc );
	}
	sc = SplineCharCreate();
	sc->unicodeenc = dummy.unicodeenc;
	sc->name = copy(dummy.name);
	sc->width = dummy.width;
	sc->parent = sf;
	sc->orig_pos = 0xffff;
	/*SCLigDefault(sc);*/
	SFAddGlyphAndEncode(sf,sc,map,enc);
    }
return( sc );
}

SplineChar *SFMakeChar(SplineFont *sf,EncMap *map, int enc) {
    int gid;

    if ( enc==-1 )
return( NULL );
    if ( enc>=map->enccount )
	gid = -1;
    else
	gid = map->map[enc];
    if ( sf->mm!=NULL && (gid==-1 || sf->glyphs[gid]==NULL) ) {
	int j;
	_SFMakeChar(sf->mm->normal,map,enc);
	for ( j=0; j<sf->mm->instance_count; ++j )
	    _SFMakeChar(sf->mm->instances[j],map,enc);
    }
return( _SFMakeChar(sf,map,enc));
}
