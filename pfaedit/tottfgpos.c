/* Copyright (C) 2000-2003 by George Williams */
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
#include "pfaedit.h"
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>
#include <utype.h>
#include <ustring.h>
#include <chardata.h>
#include <gwidget.h>

#include "ttf.h"

/* This file contains routines to create the otf gpos and gsub tables and their */
/*  attendant subtables */

struct lookup {
    int script;
    int feature_tag;
    int lookup_type;
    int lookup_cnt;
    int feature_cnt;
    uint32 offset;
    struct lookup *next;
    struct lookup *script_next;
    struct lookup *feature_next;
    uint16 flags;
};

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
/* Hangul */	{ CHR('h','a','n','g'), 0x1100, 0x11ff, 0x3130, 0x319f, 0xffa0, 0xffdf },
 /* I'm not sure what the difference is between the 'hang' tag and the 'jamo' */
 /*  tag. 'Jamo' is said to be the precomposed forms, but what's 'hang'? */
/* CJKIdeogra */{ CHR('h','a','n','i'), 0x3300, 0x9fff, 0xf900, 0xfaff, 0x020000, 0x02ffff },
/* Hebrew */	{ CHR('h','e','b','r'), 0x0590, 0x05ff, 0xfb1e, 0xfb4ff },
#if 0	/* Hiragana used to have its own tag, but has since been merged with katakana */
/* Hiragana */	{ CHR('h','i','r','a'), 0x3040, 0x309f },
#endif
/* Hangul Jamo*/{ CHR('j','a','m','o'), 0xac00, 0xd7af },
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
	    enc == em_big5hkscs || enc==em_sjis )
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

static LigList *LigListMatchTag(LigList *ligs,uint32 tag,uint16 flags) {
    LigList *l;

    for ( l=ligs; l!=NULL; l=l->next )
	if ( l->lig->tag == tag && (l->lig->flags&~1)==flags )
return( l );
return( NULL );
}

static PST *PosSubMatchTag(PST *pst,uint32 tag,enum possub_type type,uint16 flags) {

    for ( ; pst!=NULL; pst=pst->next )
	if ( pst->tag == tag && pst->type==type && (pst->flags&~1)==flags )
return( pst );
return( NULL );
}

static void GlyphMapFree(SplineChar ***map) {
    int i;

    if ( map==NULL )
return;
    for ( i=0; map[i]!=NULL; ++i )
	free(map[i]);
    free(map);
}

static SplineChar **FindSubs(SplineChar *sc,uint32 tag, enum possub_type type) {
    SplineChar *space[30];
    int cnt=0;
    char *pt, *start;
    SplineChar *subssc, **ret;
    PST *pst;

    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->tag == tag && pst->type==type ) {
	    pt = pst->u.subs.variant;
	    while ( 1 ) {
		while ( *pt==' ' ) ++pt;
		start = pt;
		pt = strchr(start,' ');
		if ( pt!=NULL )
		    *pt = '\0';
		subssc = SFGetChar(sc->parent,-1,start);
		if ( subssc!=NULL && subssc->ttf_glyph!=-1 &&
			cnt<sizeof(space)/sizeof(space[0]) )
		    space[cnt++] = subssc;
		if ( pt==NULL )
	    break;
		*pt=' ';
	    }
	}
    }
    if ( cnt==0 )	/* Might happen if the substitution names weren't in the font */
return( NULL );
    ret = galloc((cnt+1)*sizeof(SplineChar *));
    memcpy(ret,space,cnt*sizeof(SplineChar *));
    ret[cnt] = NULL;
return( ret );
}

static SplineChar **generateGlyphList(SplineFont *sf, int iskern, uint32 script,
	uint32 tag, uint16 flags) {
    int cnt;
    SplineFont *sub;
    SplineChar *sc;
    int k,i,j;
    SplineChar **glyphs=NULL;

    for ( j=0; j<2; ++j ) {
	k = 0;
	cnt = 0;
	do {
	    sub = ( sf->subfontcnt==0 ) ? sf : sf->subfonts[k];
	    for ( i=0; i<sub->charcnt; ++i )
		    if ( SCWorthOutputting(sc=sub->chars[i]) &&
			    sc->script==script &&
			    ((iskern && sc->kerns!=NULL ) ||
			     (!iskern && LigListMatchTag(sc->ligofme,tag,flags)) )) {
		if ( glyphs!=NULL ) glyphs[cnt] = sc;
		++cnt;
		sc->ticked = true;
	    }
	    ++k;
	} while ( k<sf->subfontcnt );
	if ( glyphs==NULL ) {
	    if ( cnt==0 )
return( NULL );
	    glyphs = galloc((cnt+1)*sizeof(SplineChar *));
	}
    }
    glyphs[cnt] = NULL;
return( glyphs );
}

static SplineChar **generateGlyphTypeList(SplineFont *sf, enum possub_type type,
	uint32 script, uint32 tag, uint16 flags, SplineChar ****map) {
    int cnt;
    SplineFont *sub;
    SplineChar *sc;
    int k,i,j;
    SplineChar **glyphs=NULL, ***maps=NULL;

    for ( j=0; j<2; ++j ) {
	k = 0;
	cnt = 0;
	do {
	    sub = ( sf->subfontcnt==0 ) ? sf : sf->subfonts[k];
	    for ( i=0; i<sub->charcnt; ++i )
		    if ( SCWorthOutputting(sc=sub->chars[i]) &&
			    sc->script==script &&
			    PosSubMatchTag(sc->possub,tag,type,flags) ) {
		if ( glyphs!=NULL ) {
		    glyphs[cnt] = sc;
		    if ( type==pst_substitution || type==pst_alternate || type==pst_multiple ) {
			maps[cnt] = FindSubs(sc,tag,type);
			if ( maps[cnt]==NULL )
			    --cnt;		/* Couldn't find anything, toss it */
		    }
		}
		++cnt;
		sc->ticked = true;
	    }
	    ++k;
	} while ( k<sf->subfontcnt );
	if ( glyphs==NULL ) {
	    if ( cnt==0 )
return( NULL );
	    glyphs = galloc((cnt+1)*sizeof(SplineChar *));
	    if ( type==pst_substitution || type==pst_alternate || type==pst_multiple )
		maps = galloc((cnt+1)*sizeof(SplineChar **));
	}
    }
    glyphs[cnt] = NULL;
    if ( maps!=NULL )
	maps[cnt] = NULL;
    if ( cnt==0 ) {
	free(glyphs);
	GlyphMapFree(maps);
	glyphs = NULL; maps = NULL;
    }
    *map = maps;
return( glyphs );
}

static void AnchorClassDecompose(SplineFont *sf,AnchorClass *ac,
	SplineChar ***mark,SplineChar ***base,
	SplineChar ***lig,SplineChar ***mkmk) {
    /* Run through the font finding all characters with this anchor class */
    /*  and distributing in the four possible anchor types */
    int i,j;
    struct sclist { int cnt; SplineChar **glyphs; } heads[at_max];
    AnchorPoint *test;

    memset(heads,0,sizeof(heads));
    for ( j=0; j<2; ++j ) {
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	    for ( test=sf->chars[i]->anchor; test!=NULL && test->anchor!=ac; test=test->next );
	    if ( test!=NULL && test->type!=at_centry && test->type!=at_cexit ) {
		if ( heads[test->type].glyphs!=NULL )
		    heads[test->type].glyphs[heads[test->type].cnt] = sf->chars[i];
		++heads[test->type].cnt;
	    }
	}
	if ( j==1 )
    break;
	for ( i=0; i<4; ++i )
	    if ( heads[i].cnt!=0 ) {
		heads[i].glyphs = galloc((heads[i].cnt+1)*sizeof(SplineChar *));
		heads[i].glyphs[heads[i].cnt] = NULL;
		heads[i].cnt = 0;
	    }
    }

    *mark = heads[at_mark].glyphs;
    *base = heads[at_basechar].glyphs;
    *lig  = heads[at_baselig].glyphs;
    *mkmk = heads[at_basemark].glyphs;
}

static SplineChar **EntryExitDecompose(SplineFont *sf,AnchorClass *ac) {
    /* Run through the font finding all characters with this anchor class */
    int i,j, cnt;
    SplineChar **array;
    AnchorPoint *test;

    array=NULL;
    for ( j=0; j<2; ++j ) {
	cnt = 0;
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	    for ( test=sf->chars[i]->anchor; test!=NULL && test->anchor!=ac; test=test->next );
	    if ( test!=NULL && (test->type==at_centry || test->type==at_cexit )) {
		if ( array!=NULL )
		    array[cnt] = sf->chars[i];
		++cnt;
	    }
	}
	if ( cnt==0 )
return( NULL );
	if ( j==1 )
    break;
	array = galloc((cnt+1)*sizeof(SplineChar *));
	array[cnt] = NULL;
    }
return( array );
}

static SplineChar **GlyphsOfScript(SplineChar **base,int first) {
    int cnt, k;
    SplineChar **glyphs;

    glyphs = NULL;
    while ( 1 ) {
	cnt = 1;
	for ( k=first+1; base[k]!=NULL; ++k ) if ( base[first]->script==base[k]->script ) {
	    if ( glyphs!=NULL ) {
		glyphs[cnt] = base[k];
		base[k]->ticked = true;
	    }
	    ++cnt;
	}
	if ( glyphs!=NULL )
    break;
	glyphs = galloc((cnt+1)*sizeof(SplineChar *));
	glyphs[0] = base[first];
	glyphs[cnt] = NULL;
    }
return( glyphs );
}

static void AnchorGuessContext(SplineFont *sf,struct alltabs *at) {
    int i;
    int maxbase=0, maxmark=0, basec, markc;
    AnchorPoint *ap;
    int hascursive = 0;

    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i] ) {
	basec = markc = 0;
	for ( ap = sf->chars[i]->anchor; ap!=NULL; ap=ap->next )
	    if ( ap->type==at_basemark )
		++markc;
	    else if ( ap->type==at_basechar || ap->type==at_baselig )
		++basec;
	    else if ( ap->type==at_centry )
		hascursive = true;
	if ( basec>maxbase ) maxbase = basec;
	if ( markc>maxmark ) maxmark = markc;
    }

    if ( maxbase*(maxmark+1)>at->os2.maxContext )
	at->os2.maxContext = maxbase*(maxmark+1);
    if ( hascursive && at->os2.maxContext<2 )
	at->os2.maxContext=2;
}

static void dumpcoveragetable(FILE *gpos,SplineChar **glyphs) {
    int i, last = -2, range_cnt=0, start;
    /* the glyph list should already be sorted */
    /* figure out whether it is better (smaller) to use an array of glyph ids */
    /*  or a set of glyph id ranges */

    for ( i=0; glyphs[i]!=NULL; ++i ) {
	if ( glyphs[i]->ttf_glyph<=last )
	    GDrawIError("Glyphs must be ordered when creating coverage table");
	if ( glyphs[i]->ttf_glyph!=last+1 )
	    ++range_cnt;
	last = glyphs[i]->ttf_glyph;
    }
    if ( i<=3*range_cnt ) {
	/* We use less space with a list of glyphs than with a set of ranges */
	putshort(gpos,1);		/* Coverage format=1 => glyph list */
	putshort(gpos,i);		/* count of glyphs */
	for ( i=0; glyphs[i]!=NULL; ++i )
	    putshort(gpos,glyphs[i]->ttf_glyph);	/* array of glyph IDs */
    } else {
	putshort(gpos,2);		/* Coverage format=2 => range list */
	putshort(gpos,range_cnt);	/* count of ranges */
	last = -2; start = -2;		/* start is a index in our glyph array, last is ttf_glyph */
	for ( i=0; glyphs[i]!=NULL; ++i ) {
	    if ( glyphs[i]->ttf_glyph!=last+1 ) {
		if ( last!=-2 ) {
		    putshort(gpos,glyphs[start]->ttf_glyph);	/* start glyph ID */
		    putshort(gpos,last);			/* end glyph ID */
		    putshort(gpos,start);			/* coverage index of start glyph */
		}
		start = i;
	    }
	    last = glyphs[i]->ttf_glyph;
	}
	if ( last!=-2 ) {
	    putshort(gpos,glyphs[start]->ttf_glyph);	/* start glyph ID */
	    putshort(gpos,last);			/* end glyph ID */
	    putshort(gpos,start);			/* coverage index of start glyph */
	}
    }
}

static void dumpgposkerndata(FILE *gpos,SplineFont *sf,uint32 script,
	    struct alltabs *at) {
    int32 coverage_pos, next_val_pos, here;
    int cnt, i, pcnt, max=100, j,k;
    int *seconds = galloc(max*sizeof(int));
    int *changes = galloc(max*sizeof(int));
    int16 *offsets=NULL;
    SplineChar **glyphs;
    KernPair *kp;

    glyphs = generateGlyphList(sf,true,script,0,0);
    cnt=0;
    if ( glyphs!=NULL ) {
	for ( ; glyphs[cnt]!=NULL; ++cnt );
	at->os2.maxContext = 2;
    }

    putshort(gpos,1);		/* format 1 of the pair adjustment subtable */
    coverage_pos = ftell(gpos);
    putshort(gpos,0);		/* offset to coverage table */
    if ( script==CHR('a','r','a','b') || script==CHR('h','e','b','r') ) {
	/* Right to left kerns modify the second character's width */
	/*  this doesn't make sense to me, but who am I to argue */
	putshort(gpos,0x0000);	/* leave first char alone */
	putshort(gpos,0x0004);	/* Alter XAdvance of second character */
    } else {
	putshort(gpos,0x0004);	/* Alter XAdvance of first character */
	putshort(gpos,0x0000);	/* leave second char alone */
    }
    putshort(gpos,cnt);
    next_val_pos = ftell(gpos);
    if ( glyphs!=NULL )
	offsets = galloc(cnt*sizeof(int16));
    for ( i=0; i<cnt; ++i )
	putshort(gpos,0);
    for ( i=0; i<cnt; ++i ) {
	offsets[i] = ftell(gpos)-coverage_pos+2;
	for ( pcnt = 0, kp = glyphs[i]->kerns; kp!=NULL; kp=kp->next ) ++pcnt;
	putshort(gpos,pcnt);
	if ( pcnt>=max ) {
	    max = pcnt+100;
	    seconds = grealloc(seconds,max*sizeof(int));
	    changes = grealloc(changes,max*sizeof(int));
	}
	for ( pcnt = 0, kp = glyphs[i]->kerns; kp!=NULL; kp=kp->next ) {
	    seconds[pcnt] = kp->sc->ttf_glyph;
	    changes[pcnt++] = kp->off;
	}
	for ( j=0; j<pcnt-1; ++j ) for ( k=j+1; k<pcnt; ++k ) {
	    if ( seconds[k]<seconds[j] ) {
		int temp = seconds[k];
		seconds[k] = seconds[j];
		seconds[j] = temp;
		temp = changes[k];
		changes[k] = changes[j];
		changes[j] = temp;
	    }
	}
	for ( j=0; j<pcnt; ++j ) {
	    putshort(gpos,seconds[j]);
	    putshort(gpos,changes[j]);
	}
    }
    free(seconds);
    free(changes);
    if ( glyphs!=NULL ) {
	here = ftell(gpos);
	fseek(gpos,coverage_pos,SEEK_SET);
	putshort(gpos,here-coverage_pos+2);
	fseek(gpos,next_val_pos,SEEK_SET);
	for ( i=0; i<cnt; ++i )
	    putshort(gpos,offsets[i]);
	fseek(gpos,here,SEEK_SET);
	dumpcoveragetable(gpos,glyphs);
	free(glyphs);
	free(offsets);
    }
}

static struct lookup *dumpgposCursiveAttach(FILE *gpos,AnchorClass *ac,
	SplineFont *sf, struct lookup *lookups) {
    struct lookup *new;
    SplineChar **entryexit = EntryExitDecompose(sf,ac), **glyphs;
    int i,cnt, offset,j;
    AnchorPoint *ap, *entry, *exit;
    uint32 coverage_offset;

    if ( entryexit==NULL )
return( lookups );

    for ( i=0; entryexit[i]!=NULL; ++i )
	entryexit[i]->ticked = false;
    for ( i=0; entryexit[i]!=NULL; ++i ) if ( !entryexit[i]->ticked ) {
	glyphs = GlyphsOfScript(entryexit,i);
	for ( cnt=0; glyphs[cnt]!=NULL; ++cnt );
	new = chunkalloc(sizeof(struct lookup));
	new->script = glyphs[0]->script;
	new->feature_tag = ac->feature_tag;
	new->flags = ac->flags;
	new->lookup_type = 3;		/* Cursive positioning */
	new->offset = ftell(gpos);
	new->next = lookups;
	lookups = new;

	putshort(gpos,1);	/* format 1 for this subtable */
	putshort(gpos,0);	/* Fill in later, offset to coverage table */
	putshort(gpos,cnt);	/* number of glyphs */

	offset = 6+2*2*cnt;
	for ( j=0; j<cnt; ++j ) {
	    entry = exit = NULL;
	    for ( ap=glyphs[j]->anchor; ap!=NULL; ap=ap->next ) {
		if ( ap->anchor==ac && ap->type==at_centry ) entry = ap;
		if ( ap->anchor==ac && ap->type==at_cexit ) exit = ap;
	    }
	    if ( entry!=NULL ) {
		putshort(gpos,offset);
		offset += 6;
	    } else
		putshort(gpos,0);
	    if ( exit!=NULL ) {
		putshort(gpos,offset);
		offset += 6;
	    } else
		putshort(gpos,0);
	}
	for ( j=0; j<cnt; ++j ) {
	    entry = exit = NULL;
	    for ( ap=glyphs[j]->anchor; ap!=NULL; ap=ap->next ) {
		if ( ap->anchor==ac && ap->type==at_centry ) entry = ap;
		if ( ap->anchor==ac && ap->type==at_cexit ) exit = ap;
	    }
	    if ( entry!=NULL ) {
		putshort(gpos,1);		/* Anchor format 1 */
		putshort(gpos,entry->me.x);	/* X coord of attachment */
		putshort(gpos,entry->me.y);	/* Y coord of attachment */
	    }
	    if ( exit!=NULL ) {
		putshort(gpos,1);		/* Anchor format 1 */
		putshort(gpos,exit->me.x);	/* X coord of attachment */
		putshort(gpos,exit->me.y);	/* Y coord of attachment */
	    }
	}
	coverage_offset = ftell(gpos);
	dumpcoveragetable(gpos,glyphs);
	fseek(gpos,new->offset+2,SEEK_SET);
	putshort(gpos,coverage_offset-new->offset);
	fseek(gpos,0,SEEK_END);
    }
return( lookups );
}

static struct lookup *dumpgposAnchorData(FILE *gpos,AnchorClass *ac,
	enum anchor_type at,
	SplineChar **marks,SplineChar **base,struct lookup *lookups) {
    SplineChar **glyphs;
    struct lookup *new, *end=lookups, *l;
    int i,j,cnt,k, pos, offset, suboffset, tot, max, mask;
    uint32 coverage_offset, markarray_offset;
    AnchorPoint *ap, **aps;

    for ( i=0; base[i]!=NULL; ++i )
	base[i]->ticked = false;
    for ( i=0; base[i]!=NULL; ++i ) if ( !base[i]->ticked ) {
	glyphs = GlyphsOfScript(base,i);
	for ( cnt=0; glyphs[cnt]!=NULL; ++cnt );
	new = chunkalloc(sizeof(struct lookup));
	new->script = glyphs[0]->script;
	new->feature_tag = ac->feature_tag;
	new->flags = ac->flags;
	new->lookup_type = 3+at;	/* One of the "mark to *" types 4,5,6 */
	new->offset = ftell(gpos);
	new->next = lookups;
	lookups = new;

	putshort(gpos,1);	/* format 1 for this subtable */
	putshort(gpos,0);	/* Fill in later, offset to mark coverage table */
	putshort(gpos,0);	/* Fill in later, offset to base coverage table */
	putshort(gpos,1);	/* 1 class */
	putshort(gpos,0);	/* Fill in later, offset to mark array */
	putshort(gpos,12);	/* Offset to base array */
	    /* Base array */
	putshort(gpos,cnt);	/* Number of entries in array */
	if ( at==at_basechar || at==at_basemark ) {
	    offset = 2+2*cnt;
	    for ( j=0; j<cnt; ++j ) {
		putshort(gpos,offset);
		offset += 6;
	    }
	    for ( j=0; j<cnt; ++j ) {
		for ( ap=glyphs[j]->anchor; ap!=NULL && ap->anchor!=ac; ap=ap->next );
		putshort(gpos,1);		/* Anchor format 1 */
		putshort(gpos,ap->me.x);	/* X coord of attachment */
		putshort(gpos,ap->me.y);	/* Y coord of attachment */
	    }
	} else {
	    offset = 2+2*cnt;
	    suboffset = 0;
	    for ( j=0; j<cnt; ++j ) {
		putshort(gpos,offset);
		pos = tot = max = 0;
		mask = 0;
		for ( ap=glyphs[j]->anchor; ap!=NULL ; ap=ap->next )
		    if ( ap->anchor==ac ) {
			if ( pos<ap->lig_index ) pos = ap->lig_index;
			if ( !(mask&(1<<ap->lig_index) ) ) { ++tot; mask |= (1<<ap->lig_index); }
		    }
		if ( pos>max ) max = pos;
		offset += 2+(pos+1)*2+tot*(2+6);
		    /* 2 for component count, for each component an offset to an offset to an anchor record */
	    }
	    aps = galloc((max+1)*sizeof(AnchorPoint *));
	    for ( j=0; j<cnt; ++j ) {
		memset(aps,0,(max+1)*sizeof(AnchorPoint *));
		pos = 0;
		for ( ap=glyphs[j]->anchor; ap!=NULL ; ap=ap->next )
		    if ( ap->anchor==ac ) {
			if ( pos<ap->lig_index ) pos = ap->lig_index;
			aps[ap->lig_index] = ap;
		    }
		++pos;
		putshort(gpos,pos);
		offset = 2+2*pos;
		for ( k=0; k<pos; ++k ) {
		    if ( aps[k]==NULL )
			putshort(gpos,0);
		    else {
			putshort(gpos,offset);
			offset += 6;
		    }
		}
		for ( k=0; k<pos; ++k ) {
		    if ( aps[k]!=NULL ) {
			putshort(gpos,1);		/* Anchor format 1 */
			putshort(gpos,aps[k]->me.x);	/* X coord of attachment */
			putshort(gpos,aps[k]->me.y);	/* Y coord of attachment */
		    }
		}
	    }
	    free(aps);
	}
	coverage_offset = ftell(gpos);
	fseek(gpos,lookups->offset+4,SEEK_SET);
	putshort(gpos,coverage_offset-lookups->offset);
	fseek(gpos,0,SEEK_END);
	dumpcoveragetable(gpos,glyphs);
	free(glyphs);
    }
    /* All of the previous lookups share the same tables for their marks */
    coverage_offset = ftell(gpos);
    dumpcoveragetable(gpos,marks);
    markarray_offset = ftell(gpos);
	for ( cnt=0; marks[cnt]!=NULL; ++cnt );
	putshort(gpos,cnt);
	offset = 2+4*cnt;
	for ( j=0; j<cnt; ++j ) {
	    putshort(gpos,0);		/* Only one class */
	    putshort(gpos,offset);
	    offset += 6;
	}
	for ( j=0; j<cnt; ++j ) {
	    for ( ap=marks[j]->anchor; ap!=NULL && ap->anchor!=ac; ap=ap->next );
	    putshort(gpos,1);		/* Anchor format 1 */
	    putshort(gpos,ap->me.x);	/* X coord of attachment */
	    putshort(gpos,ap->me.y);	/* Y coord of attachment */
	}
    for ( l=lookups; l!=end; l=l->next ) {
	fseek(gpos,l->offset+2,SEEK_SET);	/* mark coverage table offset */
	putshort(gpos,coverage_offset-l->offset);
	fseek(gpos,4,SEEK_CUR);
	putshort(gpos,markarray_offset-l->offset);
    }
    fseek(gpos,0,SEEK_END);
return( lookups );
}

static void dumpGPOSsimplepos(FILE *gsub,SplineFont *sf,SplineChar **glyphs,uint32 tag) {
    int cnt;
    int32 coverage_pos, end;
    PST *pst, *first=NULL;
    int bits = 0, same=true;

    for ( cnt=0; glyphs[cnt]!=NULL; ++cnt) {
	for ( pst=glyphs[cnt]->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->tag==tag && pst->type==pst_position ) {
		if ( first==NULL ) first = pst;
		else if ( same ) {
		    if ( first->u.pos.xoff!=pst->u.pos.xoff ||
			    first->u.pos.yoff!=pst->u.pos.yoff ||
			    first->u.pos.h_adv_off!=pst->u.pos.h_adv_off ||
			    first->u.pos.v_adv_off!=pst->u.pos.v_adv_off )
			same = false;
		}
		if ( pst->u.pos.xoff!=0 ) bits |= 1;
		if ( pst->u.pos.yoff!=0 ) bits |= 2;
		if ( pst->u.pos.h_adv_off!=0 ) bits |= 4;
		if ( pst->u.pos.v_adv_off!=0 ) bits |= 8;
	break;
	    }
	}
    }
    if ( bits==0 ) bits=1;

    putshort(gsub,same?1:2);	/* 1 means all value records same */
    coverage_pos = ftell(gsub);
    putshort(gsub,0);		/* offset to coverage table */
    putshort(gsub,cnt);
    putshort(gsub,bits);
    if ( same ) {
	if ( bits&1 ) putshort(gsub,first->u.pos.xoff);
	if ( bits&2 ) putshort(gsub,first->u.pos.yoff);
	if ( bits&4 ) putshort(gsub,first->u.pos.h_adv_off);
	if ( bits&8 ) putshort(gsub,first->u.pos.v_adv_off);
    } else {
	for ( cnt = 0; glyphs[cnt]!=NULL; ++cnt ) {
	    for ( pst=glyphs[cnt]->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->tag==tag && pst->type==pst_position ) {
		    if ( bits&1 ) putshort(gsub,pst->u.pos.xoff);
		    if ( bits&2 ) putshort(gsub,pst->u.pos.yoff);
		    if ( bits&4 ) putshort(gsub,pst->u.pos.h_adv_off);
		    if ( bits&8 ) putshort(gsub,pst->u.pos.v_adv_off);
		}
	    }
	}
    }
    end = ftell(gsub);
    fseek(gsub,coverage_pos,SEEK_SET);
    putshort(gsub,end-coverage_pos+2);
    fseek(gsub,end,SEEK_SET);
    dumpcoveragetable(gsub,glyphs);
    free(glyphs);
}

static void dumpgsubligdata(FILE *gsub,SplineFont *sf,uint32 script, uint32 tag,
	uint16 flags, struct alltabs *at) {
    int32 coverage_pos, next_val_pos, here, lig_list_start;
    int cnt, i, pcnt, lcnt, max=100, j;
    uint16 *offsets=NULL, *ligoffsets=galloc(max*sizeof(uint16));
    SplineChar **glyphs;
    LigList *ll;
    struct splinecharlist *scl;

    glyphs = generateGlyphList(sf,false,script,tag,flags);
    cnt=0;
    if ( glyphs!=NULL ) for ( ; glyphs[cnt]!=NULL; ++cnt );

    putshort(gsub,1);		/* only one format for ligatures */
    coverage_pos = ftell(gsub);
    putshort(gsub,0);		/* offset to coverage table */
    putshort(gsub,cnt);
    next_val_pos = ftell(gsub);
    if ( glyphs!=NULL )
	offsets = galloc(cnt*sizeof(int16));
    for ( i=0; i<cnt; ++i )
	putshort(gsub,0);
    for ( i=0; i<cnt; ++i ) {
	offsets[i] = ftell(gsub)-coverage_pos+2;
	for ( pcnt = 0, ll = glyphs[i]->ligofme; ll!=NULL; ll=ll->next )
	    if ( ll->lig->tag==tag && (ll->lig->flags&~1) == flags)
		++pcnt;
	putshort(gsub,pcnt);
	if ( pcnt>=max ) {
	    max = pcnt+100;
	    ligoffsets = grealloc(ligoffsets,max*sizeof(int));
	}
	lig_list_start = ftell(gsub);
	for ( j=0; j<pcnt; ++j )
	    putshort(gsub,0);			/* Place holders */
	for ( pcnt=0, ll = glyphs[i]->ligofme; ll!=NULL; ll=ll->next ) {
	    if ( ll->lig->tag==tag && (ll->lig->flags&~1) == flags) {
		ligoffsets[pcnt] = ftell(gsub)-lig_list_start+2;
		putshort(gsub,ll->lig->u.lig.lig->ttf_glyph);
		for ( lcnt=0, scl=ll->components; scl!=NULL; scl=scl->next ) ++lcnt;
		putshort(gsub, lcnt+1);
		if ( lcnt+1>at->os2.maxContext )
		    at->os2.maxContext = lcnt+1;
		for ( scl=ll->components; scl!=NULL; scl=scl->next )
		    putshort(gsub, scl->sc->ttf_glyph );
		++pcnt;
	    }
	}
	fseek(gsub,lig_list_start,SEEK_SET);
	for ( j=0; j<pcnt; ++j )
	    putshort(gsub,ligoffsets[j]);
	fseek(gsub,0,SEEK_END);
    }
    free(ligoffsets);
    if ( glyphs!=NULL ) {
	here = ftell(gsub);
	fseek(gsub,coverage_pos,SEEK_SET);
	putshort(gsub,here-coverage_pos+2);
	fseek(gsub,next_val_pos,SEEK_SET);
	for ( i=0; i<cnt; ++i )
	    putshort(gsub,offsets[i]);
	fseek(gsub,here,SEEK_SET);
	dumpcoveragetable(gsub,glyphs);
	free(glyphs);
	free(offsets);
    }
}

static void dumpGSUBsimplesubs(FILE *gsub,SplineFont *sf,SplineChar **glyphs, SplineChar ***maps) {
    int cnt, diff, ok = true;
    int32 coverage_pos, end;

    diff = (*maps[0])->ttf_glyph - glyphs[0]->ttf_glyph;
    for ( cnt=0; glyphs[cnt]!=NULL; ++cnt)
	if ( diff!= maps[cnt][0]->ttf_glyph-glyphs[cnt]->ttf_glyph ) ok = false;

    if ( ok ) {
	putshort(gsub,1);	/* delta format */
	coverage_pos = ftell(gsub);
	putshort(gsub,0);		/* offset to coverage table */
	putshort(gsub,diff);
    } else {
	putshort(gsub,2);		/* glyph list format */
	coverage_pos = ftell(gsub);
	putshort(gsub,0);		/* offset to coverage table */
	putshort(gsub,cnt);
	for ( cnt = 0; glyphs[cnt]!=NULL; ++cnt )
	    putshort(gsub,(*maps[cnt])->ttf_glyph);
    }
    end = ftell(gsub);
    fseek(gsub,coverage_pos,SEEK_SET);
    putshort(gsub,end-coverage_pos+2);
    fseek(gsub,end,SEEK_SET);
    dumpcoveragetable(gsub,glyphs);
}

static void dumpGSUBmultiplesubs(FILE *gsub,SplineFont *sf,SplineChar **glyphs, SplineChar ***maps) {
    int cnt, offset;
    int32 coverage_pos, end;
    int gc;

    for ( cnt=0; glyphs[cnt]!=NULL; ++cnt);

    putshort(gsub,1);		/* glyph list format */
    coverage_pos = ftell(gsub);
    putshort(gsub,0);		/* offset to coverage table */
    putshort(gsub,cnt);
    offset = 6+2*cnt;
    for ( cnt = 0; glyphs[cnt]!=NULL; ++cnt ) {
	putshort(gsub,offset);
	for ( gc=0; maps[cnt][gc]!=NULL; ++gc );
	offset += 2+2*gc;
    }
    for ( cnt = 0; glyphs[cnt]!=NULL; ++cnt ) {
	for ( gc=0; maps[cnt][gc]!=NULL; ++gc );
	putshort(gsub,gc);
	for ( gc=0; maps[cnt][gc]!=NULL; ++gc )
	    putshort(gsub,maps[cnt][gc]->ttf_glyph);
    }
    end = ftell(gsub);
    fseek(gsub,coverage_pos,SEEK_SET);
    putshort(gsub,end-coverage_pos+2);
    fseek(gsub,end,SEEK_SET);
    dumpcoveragetable(gsub,glyphs);
}

static struct lookup *GPOSfigureLookups(FILE *lfile,SplineFont *sf,
	struct alltabs *at) {
    /* When we find a feature, we split it out into various scripts */
    /*  dumping one lookup per script into the file */
    struct lookup *lookups = NULL, *new;
    int i, j;
    SplineChar **marks, **base, **lig, **mkmk, ***map, **glyphs;
    SplineChar *sc;
    AnchorClass *ac;
    uint32 *ligtags;
    uint32 *flag_sets;
    int max, cnt, flags;
    enum possub_type type;
    PST *pst;

    max = 30; cnt = 0;
    ligtags = galloc(max*sizeof(uint32));
    flag_sets = galloc(max*sizeof(uint32));

    type = pst_position;
    {
	cnt = 0;
	for ( i=0; i<sf->charcnt; i++ ) if ( sf->chars[i]!=NULL ) {
	    for ( pst = sf->chars[i]->possub; pst!=NULL; pst=pst->next ) if ( pst->type==type ) {
		for ( j=0; j<cnt; ++j )
		    if ( ligtags[j]==pst->tag ) {
			flag_sets[j] |= 1<<(pst->flags>>1);
		break;
		    }
		if ( j==cnt ) {
		    if ( cnt>=max ) {
			max += 30;
			ligtags = grealloc(ligtags,max*sizeof(uint32));
			flag_sets = grealloc(flag_sets,max*sizeof(uint32));
		    }
		    flag_sets[cnt] |= 1<<(pst->flags>>1);
		    ligtags[cnt++] = pst->tag;
		}
	    }
	}

	/* Look for positions matching these tags */
	for ( j=0; j<cnt; ++j ) for ( flags=0; flags<8; ++flags ) if ( flag_sets[j]&(1<<flags) ) {
	    for ( i=0; i<sf->charcnt; i++ ) if ( sf->chars[i]!=NULL )
		sf->chars[i]->ticked = false;
	    for ( i=0; i<sf->charcnt; i++ ) 
		    if ( (sc=sf->chars[i])!=NULL && sc->possub!=NULL &&
			    sc->script!=0 && !sc->ticked &&
			    PosSubMatchTag(sc->possub,ligtags[j],type,flags<<1) ) {
		glyphs = generateGlyphTypeList(sf,type,sc->script,ligtags[j],flags<<1,&map);
		if ( glyphs!=NULL && glyphs[0]!=NULL ) {
		    new = chunkalloc(sizeof(struct lookup));
		    new->script = sc->script;
		    new->feature_tag = ligtags[j];
		    new->flags = flags<<1;
		    new->lookup_type = 1;
		    new->offset = ftell(lfile);
		    new->next = lookups;
		    lookups = new;
		    dumpGPOSsimplepos(lfile,sf,glyphs,ligtags[j]);
		}
	    }
	}
    }
    free(ligtags);
    free(flag_sets);

    /* Look for kerns */
    for ( i=0; i<sf->charcnt; i++ ) if ( sf->chars[i]!=NULL )
	sf->chars[i]->ticked = false;
    for ( i=0; i<sf->charcnt; i++ )
	    if ( (sc=sf->chars[i])!=NULL && sc->kerns!=NULL &&
		    sc->script!=0 && !sc->ticked ) {
	new = chunkalloc(sizeof(struct lookup));
	new->script = sc->script;
	new->feature_tag = CHR('k','e','r','n');
	new->flags = pst_ignorecombiningmarks;
	new->lookup_type = 2;		/* Pair adjustment subtable type */
	new->offset = ftell(lfile);
	new->next = lookups;
	lookups = new;
	dumpgposkerndata(lfile,sf,sc->script,at);
    }

    /* Every Anchor Class gets its own lookup (and may get several if it has */
    /*  different anchor types or scripts) */
    for ( ac=sf->anchor; ac!=NULL; ac = ac->next ) {
	if ( ac->feature_tag==CHR('c','u','r','s') )
	    lookups = dumpgposCursiveAttach(lfile,ac,sf,lookups);
	else {
	    AnchorClassDecompose(sf,ac,&marks,&base,&lig,&mkmk);
	    if ( marks!=NULL && base!=NULL )
		lookups = dumpgposAnchorData(lfile,ac,at_basechar,marks,base,lookups);
	    if ( marks!=NULL && lig!=NULL )
		lookups = dumpgposAnchorData(lfile,ac,at_baselig,marks,lig,lookups);
	    if ( marks!=NULL && mkmk!=NULL )
		lookups = dumpgposAnchorData(lfile,ac,at_basemark,marks,mkmk,lookups);
	    free(marks);
	    free(base);
	    free(lig);
	    free(mkmk);
	}
    }
    if ( sf->anchor )
	AnchorGuessContext(sf,at);

return( lookups );
}

static struct lookup *GSUBfigureLookups(FILE *lfile,SplineFont *sf,
	struct alltabs *at) {
    struct lookup *lookups = NULL, *new;
    uint32 *ligtags, *flag_sets;
    int i, j, max, cnt, flags;
    LigList *ll;
    PST *subs;
    SplineChar *sc;
    enum possub_type type;
    SplineChar **glyphs, ***map;

    /* Look for ligature tags used in the font */
    max = 30; cnt = 0;
    ligtags = galloc(max*sizeof(uint32));
    flag_sets = galloc(max*sizeof(uint32));
    for ( i=0; i<sf->charcnt; i++ ) if ( sf->chars[i]!=NULL ) {
	if ( sf->chars[i]->script==0 && sf->chars[i]->ligofme!=NULL )
	    fprintf( stderr, "Warning: Ligature '%s' beginning with glyph '%s' has no associated script.\n No GSUB entry will be generated for it.\n",
		    sf->chars[i]->ligofme->lig->u.lig.lig->name,
		    sf->chars[i]->name );
	for ( ll = sf->chars[i]->ligofme; ll!=NULL; ll=ll->next ) {
	    for ( j=0; j<cnt; ++j )
		if ( ligtags[j]==ll->lig->tag ) {
			flag_sets[j] |= 1<<(ll->lig->flags>>1);
	    break;
		}
	    if ( j==cnt ) {
		if ( cnt>=max ) {
		    max += 30;
		    ligtags = grealloc(ligtags,max*sizeof(uint32));
		    flag_sets = grealloc(flag_sets,max*sizeof(uint32));
		}
		flag_sets[cnt] = 1<<(ll->lig->flags>>1);
		ligtags[cnt++] = ll->lig->tag;
	    }
	}
    }

    /* Look for ligatures matching these tags */
    for ( j=0; j<cnt; ++j ) for ( flags=0; flags<8; ++flags ) if ( flag_sets[j]&(1<<flags) ) {
	for ( i=0; i<sf->charcnt; i++ ) if ( sf->chars[i]!=NULL )
	    sf->chars[i]->ticked = false;
	for ( i=0; i<sf->charcnt; i++ ) 
		if ( (sc=sf->chars[i])!=NULL && sc->ligofme!=NULL &&
			sc->script!=0 && !sc->ticked &&
			LigListMatchTag(sc->ligofme,ligtags[j],flags<<1) ) {
	    new = chunkalloc(sizeof(struct lookup));
	    new->script = sc->script;
	    new->feature_tag = ligtags[j];
	    new->lookup_type = 4;		/* Ligature */
	    new->flags = flags<<1;
	    new->offset = ftell(lfile);
	    new->next = lookups;
	    lookups = new;
	    dumpgsubligdata(lfile,sf,sc->script,ligtags[j],flags<<1,at);
	}
    }

    /* Now do something very similar for substitution simple, mult & alt tags */
    for ( type = pst_substitution; type<=pst_multiple; ++type ) {
	cnt = 0;
	for ( i=0; i<sf->charcnt; i++ ) if ( sf->chars[i]!=NULL ) {
	    for ( subs = sf->chars[i]->possub; subs!=NULL; subs=subs->next ) if ( subs->type==type ) {
		if ( sf->chars[i]->script==0 )
		    fprintf( stderr, "Warning: Substitution of '%s' has no associated script.\n No GSUB entry will be generated for it.\n",
			    sf->chars[i]->name );
		for ( j=0; j<cnt; ++j )
		    if ( ligtags[j]==subs->tag ) {
			flag_sets[j] |= 1<<(subs->flags>>1);
		break;
		    }
		if ( j==cnt ) {
		    if ( cnt>=max ) {
			max += 30;
			ligtags = grealloc(ligtags,max*sizeof(uint32));
			flag_sets = grealloc(flag_sets,max*sizeof(uint32));
		    }
		    flag_sets[cnt] |= 1<<(subs->flags>>1);
		    ligtags[cnt++] = subs->tag;
		}
	    }
	}

	/* Look for substitutions/decompositions matching these tags */
	for ( j=0; j<cnt; ++j ) for ( flags=0; flags<8; ++flags ) if ( flag_sets[j]&(1<<flags) ) {
	    for ( i=0; i<sf->charcnt; i++ ) if ( sf->chars[i]!=NULL )
		sf->chars[i]->ticked = false;
	    for ( i=0; i<sf->charcnt; i++ ) 
		    if ( (sc=sf->chars[i])!=NULL && sc->possub!=NULL &&
			    sc->script!=0 && !sc->ticked &&
			    PosSubMatchTag(sc->possub,ligtags[j],type,flags<<1) ) {
		glyphs = generateGlyphTypeList(sf,type,sc->script,ligtags[j],flags<<1,&map);
		if ( glyphs!=NULL && glyphs[0]!=NULL ) {
		    new = chunkalloc(sizeof(struct lookup));
		    new->script = sc->script;
		    new->feature_tag = ligtags[j];
		    new->flags = flags<<1;
		    new->lookup_type = type==pst_substitution?1:type==pst_multiple?2:3;
		    new->offset = ftell(lfile);
		    new->next = lookups;
		    lookups = new;
		    if ( type==pst_substitution )
			dumpGSUBsimplesubs(lfile,sf,glyphs,map);
		    else /* the format for multiple and alternate subs is the same. Only the semantics differ */
			dumpGSUBmultiplesubs(lfile,sf,glyphs,map);
		    free(glyphs);
		    GlyphMapFree(map);
		}
	    }
	}
    }
    free(ligtags);
    free(flag_sets);
return( lookups );
}

static int FeatureIndex( uint32 tag ) {
    /* This is the order in which features should be executed */

    switch ( tag ) {
/* GSUB ordering */
      case CHR('c','c','m','p'):	/* Must be first? */
return( -1 );
      case CHR('i','n','i','t'): case CHR('m','e','d','i'): case CHR('f','i','n','a'): case CHR('i','s','o','l'):
      case CHR('h','i','n','i'): case CHR('h','m','e','d'):
      case CHR('r','t','l','a'):
return( 0 );
      case CHR('s','m','c','p'): case CHR('c','2','s','c'):
return( 1 );
      case CHR('l','i','g','a'): case CHR('r','l','i','g'):
      case CHR('d','l','i','g'): case CHR('h','l','i','g'):
      case CHR('f','r','a','c'):
      case CHR('a','b','v','s'): case CHR('a','f','r','c'):
      case CHR('a','k','h','n'): case CHR('b','l','w','f'):
      case CHR('b','l','w','s'):
      case CHR('h','a','l','f'): case CHR('h','a','l','n'):
      case CHR('l','j','m','o'): case CHR('v','j','m','o'):
      case CHR('n','u','k','f'): case CHR('v','a','t','u'):
return( 2 );
      case CHR('f','a','l','t'): case CHR('j','a','l','t'):		/* must come after 'fina'/'isol' */
return( 3 );
      case CHR('v','r','t','2'): case CHR('v','e','r','t'):
return( 101 );		/* Documented to come last */
/* GPOS ordering */
      case CHR('c','u','r','s'):
return( 0 );
      case CHR('m','k','m','k'):
return( 1 );
      case CHR('m','a','r','k'): case CHR('a','b','v','m'): case CHR('b','l','w','m'):
return( 2 );
      case CHR('k','e','r','n'):
return( 3 );
/* Unknown things come after everything but vert/vrt2 */
      default:
return( 100 );
    }
}

static struct lookup *reverse_list(struct lookup *lookups) {
    struct lookup *next, *prev=NULL;

    next = lookups->next;
    while ( next!=NULL ) {
	lookups->next = prev;
	prev = lookups;
	lookups = next;
	next = lookups->next;
    }
    lookups->next = prev;
return( lookups );
}

static struct lookup *orderlookups(struct lookup *lookups,struct lookup **features) {
    int cnt,i,j;
    struct lookup **array, *l, *script_start, *temp;

    for ( l=lookups, cnt=0; l!=NULL; l=l->next )
	l->lookup_cnt = cnt++;
    array = galloc(cnt*sizeof(struct lookup *));
    for ( l=lookups, cnt=0; l!=NULL; l=l->next )
	array[cnt++] = l;

    /* sort by script */
    for ( i=0; i<cnt-1; ++i ) for ( j=i+1; j<cnt; ++j ) {
	if ( array[i]->script > array[j]->script ||
		(array[i]->script==array[j]->script &&
		    FeatureIndex(array[i]->feature_tag)>FeatureIndex(array[j]->feature_tag))) {
	    temp = array[i];
	    array[i] = array[j];
	    array[j] = temp;
	}
    }
    for ( i=0; i<cnt-1; ++i )
	array[i]->script_next = array[i+1];
    script_start = array[0];

    /* sort by feature */
    for ( i=0; i<cnt-1; ++i ) for ( j=i+1; j<cnt; ++j ) {
	if ( array[i]->feature_tag > array[j]->feature_tag ||
		(array[i]->feature_tag==array[j]->feature_tag &&
		    array[i]->script>array[j]->script )) {
	    temp = array[i];
	    array[i] = array[j];
	    array[j] = temp;
	}
    }
    for ( i=0; i<cnt-1; ++i ) {
	array[i]->feature_next = array[i+1];
	array[i]->feature_cnt = i;
    }
    array[i]->feature_cnt = i;
    *features = array[0];

    free(array);
return( script_start );
}

static struct lookup *dump_script_table(FILE *g___,struct lookup *scripts) {
    struct lookup *l;
    int cnt;

    /* Dump the script table */
    putshort(g___,4);		/* offset from start of script table to data */
				/* for default language */
    putshort(g___,0);		/* count of all non-default languages */
    /* Now the language system table */
    putshort(g___,0);		/* reserved, must be zero */
    putshort(g___,0xffff);	/* No required feature */
    for ( l=scripts, cnt=0; l!=NULL && l->script==scripts->script; l=l->script_next )
	++cnt;
    putshort(g___,cnt);		/* Number of features */
    for ( l=scripts; l!=NULL && l->script==scripts->script; l=l->script_next )
	putshort(g___,l->feature_cnt);	/* Index of each feature */
return( l );
}
    
static FILE *g___FigureExtensionSubTables(struct lookup *lookups,int is_gpos) {
    struct lookup *l;
    int cnt, estart, gotmore;
    FILE *efile;
    int i;

    if ( lookups==NULL )
return( NULL );
    for ( l=lookups, cnt=1; l->next!=NULL; l=l->next, ++cnt );
    if ( l->offset<65536 )
return( NULL );

    estart=cnt; gotmore = true;
    while ( gotmore ) {
	gotmore = false;
	for ( i=0, l=lookups; l!=NULL && i<estart; l=l->next, ++i ) {
	    if ( (cnt-i)*8+l->offset+(cnt-estart)*8 >=65536 ) {
		estart = i;
		gotmore = true;
	break;
	    }
	}
    }
    if ( estart==cnt ) {	/* Eh?, should never happen */
	fprintf( stderr, "Internal error in GPOS/SUB extension calculation\n" );
return( NULL );
    }

    efile = tmpfile();
    for ( i=0, l=lookups; l!=NULL && i<estart; l=l->next, ++i )
	l->offset += (cnt-estart)*8;
    while ( l!=NULL ) {
	putshort(efile,1);	/* Only one format for extensions */
	putshort(efile,l->lookup_type);
	putlong(efile,(cnt-i+estart)*8 + l->offset);
	l->lookup_type = is_gpos ? 9 : 7;
	l->offset = (i-estart)*8;
	l = l->next;
	++i;
    }
return( efile );
}

static FILE *dumpg___info(struct alltabs *at, SplineFont *sf,int is_gpos) {
    /* Dump out either a gpos or a gsub table. gpos handles kerns, gsub ligs */
    FILE *lfile, *g___, *efile;
    struct lookup *lookups=NULL, *script_ordered, *feature_ordered, *l, *prev, *next;
    int cnt, offset, i, flags, subcnt;
    char *buf;
    uint32 lookup_list_table_start, feature_list_table_start;

    lfile = tmpfile();
    if ( is_gpos ) {
	lookups = GPOSfigureLookups(lfile,sf,at);
    } else {
	lookups = GSUBfigureLookups(lfile,sf,at);
    }
    if ( lookups==NULL ) {
	fclose(lfile);
return( NULL );
    } else if ( lookups->offset>=65536 ) {
	fprintf( stderr, "The %s table generated for this font is too big and won't work.\n"
"There's just too much info to put in it, and the table is limited to 65k.\n",
		is_gpos ? "GPOS" : "GSUB" );
	fclose(lfile);
return( NULL );
    }

    lookups = reverse_list(lookups);
    script_ordered = orderlookups(lookups,&feature_ordered);

    cnt = 1;
    prev = script_ordered;
    for ( l=prev->script_next; l!=NULL; prev=l, l=l->script_next ) {
	if ( l->script != prev->script )
	    ++cnt;
    }

    g___ = tmpfile();
    putlong(g___,0x10000);		/* version number */
    putshort(g___,10);		/* offset to script table */
    putshort(g___,0);		/* offset to features. Come back for this */
    putshort(g___,0);		/* offset to lookups.  Come back for this */
/* Now the scripts */
    putshort(g___,cnt);
    offset = 2+6*cnt;		/* Offset to the first Script Table */
    putlong(g___,script_ordered->script);
    putshort(g___,offset);
    prev = script_ordered;
    subcnt=1;	/* one feature for the first script so far */
    for ( l=prev->script_next; l!=NULL; prev=l, l=l->script_next ) {
	if ( l->script==prev->script )
	    ++subcnt;
	else {
	    /* calculate size of previous script */
	    offset += 4/* Size of minimal script table */ +
			6+subcnt*2 /* Size of LangSys table */;
	    putlong(g___,l->script);
	    putshort(g___,offset);
	    subcnt = 1;
	}
    }

    /* Ok, that was the script_list_table which gives each script an offset */
    /* Now for each script we provide a Script table which contains an */
    /*  offset to a bunch of features for the default language, and a */
    /*  count of non-default languages which is always 0 for us */
    for ( l=dump_script_table(g___,script_ordered); l!=NULL;
	    l=dump_script_table(g___,l));
    /* And that should finish all the scripts */

    feature_list_table_start = ftell(g___);
    fseek(g___,6,SEEK_SET);
    putshort(g___,feature_list_table_start);
    fseek(g___,0,SEEK_END);
    for ( l=feature_ordered, cnt=0; l!=NULL; l=l->feature_next, ++cnt );
    putshort(g___,cnt);		/* Number of features */
    offset = 2+6*cnt;		/* Offset to start of first feature table from beginning of feature_list */
    for ( l=feature_ordered; l!=NULL; l=l->feature_next ) {
	putlong(g___,l->feature_tag);
	putshort(g___,offset);
	offset += 6;		/* Each of my feature tables will have one entry and be 6 bytes long */
    }
    /* for each feature, one feature table */
    for ( l=feature_ordered; l!=NULL; l=l->feature_next ) {
	putshort(g___,0);	/* No feature params */
	putshort(g___,1);	/* Only one lookup */
	putshort(g___,l->lookup_cnt);
    }
    /* And that should finish all the features */

    lookup_list_table_start = ftell(g___);
    fseek(g___,8,SEEK_SET);
    putshort(g___,lookup_list_table_start);
    fseek(g___,0,SEEK_END);
    putshort(g___,cnt);		/* same number of lookups as features */
    offset = 2+2*cnt;		/* Offset to start of first feature table from beginning of feature_list */
    for ( l=lookups; l!=NULL; l=l->next ) {
	putshort(g___,offset);
	offset += 8;		/* 8 bytes per lookup table */
    }
    /* now the lookup tables */
      /* do we need any extension sub-tables? */
    efile=g___FigureExtensionSubTables(lookups,is_gpos);
    for ( i=0, l=lookups; l!=NULL; l=l->next, ++i ) {
	putshort(g___,l->lookup_type);
	/* The right to left flag is not relevant for any of the tables I generate */
	/* but MS has it in tables where it is not relevant, so... */
	flags = l->script==CHR('a','r','a','b') || l->script==CHR('h','e','b','r');
	flags |= (l->flags&~1);
	putshort(g___,flags);
	putshort(g___,1);		/* Each table controls one lookup */
	putshort(g___,(cnt-i)*8+l->offset); /* Offset to lookup data which is in the temp file */
	    /* there are (cnt-i) lookup tables (of size 8) between here and */
	    /* the place where the temp file will start, and then we need to */
	    /* skip l->offset bytes in the temp file */
	    /* If it's a big GPOS/SUB table we may also need some extension */
	    /*  pointers, but FigureExtension will adjust for that */
    }

    buf = galloc(1024);
    if ( efile!=NULL ) {
	rewind(efile);
	while ( (i=fread(buf,1,1024,efile))>0 )
	    fwrite(buf,1,i,g___);
	fclose(efile);
    }
    rewind(lfile);
    while ( (i=fread(buf,1,1024,lfile))>0 )
	fwrite(buf,1,i,g___);
    fclose(lfile);
    free(buf);
    for ( l=lookups; l!=NULL; l=next ) {
	next = l->next;
	chunkfree(l,sizeof(*l));
    }
return( g___ );
}

void otf_dumpgpos(struct alltabs *at, SplineFont *sf) {
    /* Open Type, bless its annoying little heart, doesn't store kern info */
    /*  in the kern table. Of course not, how silly of me to think it might */
    /*  be consistent. It stores it in the much more complicated gpos table */
    at->gpos = dumpg___info(at, sf,true);
    if ( at->gpos!=NULL ) {
	at->gposlen = ftell(at->gpos);
	if ( at->gposlen&1 ) putc('\0',at->gpos);
	if ( (at->gposlen+1)&2 ) putshort(at->gpos,0);
    }
}

void otf_dumpgsub(struct alltabs *at, SplineFont *sf) {
    /* Ligatures, cjk vertical rotation replacement, arabic forms, small caps */
    SFLigaturePrepare(sf);
    at->gsub = dumpg___info(at, sf, false);
    if ( at->gsub!=NULL ) {
	at->gsublen = ftell(at->gsub);
	if ( at->gsublen&1 ) putc('\0',at->gsub);
	if ( (at->gsublen+1)&2 ) putshort(at->gsub,0);
    }
    SFLigatureCleanup(sf);
}

static int LigCaretCnt(SplineChar *sc) {
    PST *pst;

    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->type == pst_lcaret )
return( pst->u.lcaret.cnt );
    }
return( 0 );
}
    
static void DumpLigCarets(FILE *gdef,SplineChar *sc) {
    PST *pst;
    int i, j;

    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->type == pst_lcaret )
    break;
    }
    if ( pst==NULL )
return;

    if ( sc->script==CHR('a','r','a','b') || sc->script==CHR('h','e','b','r') ) {
	for ( i=0; i<pst->u.lcaret.cnt-1; ++i )
	    for ( j=i+1; j<pst->u.lcaret.cnt; ++j )
		if ( pst->u.lcaret.carets[i]<pst->u.lcaret.carets[j] ) {
		    int16 temp = pst->u.lcaret.carets[i];
		    pst->u.lcaret.carets[i] = pst->u.lcaret.carets[j];
		    pst->u.lcaret.carets[j] = temp;
		}
    } else {
	for ( i=0; i<pst->u.lcaret.cnt-1; ++i )
	    for ( j=i+1; j<pst->u.lcaret.cnt; ++j )
		if ( pst->u.lcaret.carets[i]>pst->u.lcaret.carets[j] ) {
		    int16 temp = pst->u.lcaret.carets[i];
		    pst->u.lcaret.carets[i] = pst->u.lcaret.carets[j];
		    pst->u.lcaret.carets[j] = temp;
		}
    }

    putshort(gdef,pst->u.lcaret.cnt);	/* this many carets */
    for ( i=0; i<pst->u.lcaret.cnt; ++i ) {
	putshort(gdef,1);		/* Format 1 */
	putshort(gdef,pst->u.lcaret.carets[i]);
    }
}

void otf_dumpgdef(struct alltabs *at, SplineFont *sf) {
    /* In spite of what the open type docs say, this table does appear to be */
    /*  required (at least the glyph class def table) if we do mark to base */
    /*  positioning */
    /* I was wondering at the apperant contradiction: something can be both a */
    /*  base glyph and a ligature component, but it appears that the component*/
    /*  class is unused and everything is a base unless it is a ligature or */
    /*  mark */
    /* All my example fonts ignore the attachment list subtable and the mark */
    /*  attach class def subtable, so I shall too */
    /* All my example fonts contain a ligature caret list subtable, which is */
    /*  empty. Odd, but perhaps important */
    AnchorClass *ac;
    AnchorPoint *ap;
    PST *pst;
    int i,j,k, lcnt;
    int pos, offset;
    SplineChar **glyphs;

    for ( ac = sf->anchor; ac!=NULL; ac=ac->next ) {
	if ( ac->feature_tag!=CHR('c','u','r','s'))
    break;
    }
    glyphs = NULL;
    for ( k=0; k<2; ++k ) {
	lcnt = 0;
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=-1 ) {
	    for ( pst=sf->chars[i]->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->type == pst_lcaret ) {
		    for ( j=pst->u.lcaret.cnt-1; j>=0; --j )
			if ( pst->u.lcaret.carets[j]!=0 )
		    break;
		    if ( j!=-1 )
	    break;
		}
	    }
	    if ( pst!=NULL ) {
		if ( glyphs!=NULL ) glyphs[lcnt] = sf->chars[i];
		++lcnt;
	    }
	}
	if ( lcnt==0 )
    break;
	if ( glyphs!=NULL )
    break;
	glyphs = galloc((lcnt+1)*sizeof(SplineChar *));
    }
    if ( ac==NULL && lcnt==0 )
return;					/* No anchor positioning, no ligature carets */
    glyphs[lcnt] = NULL;

    at->gdef = tmpfile();
    putlong(at->gdef,0x00010000);		/* Version */
    putshort(at->gdef, ac!=NULL ? 12 : 0 );	/* glyph class defn table */
    putshort(at->gdef, 0 );			/* attachment list table */
    putshort(at->gdef, 0 );			/* ligature caret table (come back and fix up later) */
    putshort(at->gdef, 0 );			/* mark attachment class table */

    if ( ac!=NULL ) {
	/* Mark shouldn't conflict with anything */
	/* Ligature is more important than Base */
	/* Component is not used */
	putshort(at->gdef,1);	/* class format 1 complete list of glyphs */
	putshort(at->gdef,0);	/* First glyph */
	putshort(at->gdef,at->maxp.numGlyphs );
	j=0;
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=-1 ) {
	    for ( ; j<sf->chars[i]->ttf_glyph; ++j )
		putshort(at->gdef,1);	/* Any hidden characters (like notdef) default to base */
	    for ( pst=sf->chars[i]->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->type == pst_ligature )
	    break;
	    }
	    ap=sf->chars[i]->anchor;
	    while ( ap!=NULL && (ap->type==at_centry || ap->type==at_cexit) )
		ap = ap->next;
	    if ( pst!=NULL )
		putshort(at->gdef,2);		/* ligature */
	    else if ( ap!=NULL && (ap->type==at_mark || ap->type==at_basemark) )
		putshort(at->gdef,3);		/* mark */
	    else
		putshort(at->gdef,1);		/* base */
	    ++j;
	}
    }
    pos = ftell(at->gdef);
    fseek(at->gdef,8,SEEK_SET);			/* location of lig caret table offset */
    putshort(at->gdef,pos);
    fseek(at->gdef,0,SEEK_END);
    if ( lcnt==0 ) {
	/* It always seems to be present, even if empty */
	putshort(at->gdef,4);			/* Offset to (empty) coverage table */
	putshort(at->gdef,0);			/* no ligatures */
	putshort(at->gdef,2);			/* coverage table format 2 */
	putshort(at->gdef,0);			/* no ranges in coverage table */
    } else {
	pos = ftell(at->gdef);	/* coverage location */
	putshort(at->gdef,0);			/* Offset to coverage table (fix up later) */
	putshort(at->gdef,lcnt);
	offset = 2*lcnt+4;
	for ( i=0; i<lcnt; ++i ) {
	    putshort(at->gdef,offset);
	    offset+=2+4*LigCaretCnt(glyphs[i]);
	}
	for ( i=0; i<lcnt; ++i )
	    DumpLigCarets(at->gdef,glyphs[i]);
	offset = ftell(at->gdef);
	fseek(at->gdef,pos,SEEK_SET);
	putshort(at->gdef,offset-pos);
	fseek(at->gdef,0,SEEK_END);
	dumpcoveragetable(at->gdef,glyphs);
    }

    at->gdeflen = ftell(at->gdef);
    if ( at->gdeflen&1 ) putc('\0',at->gdef);
    if ( (at->gdeflen+1)&2 ) putshort(at->gdef,0);
}
