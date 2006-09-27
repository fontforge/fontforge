/* Copyright (C) 2000-2006 by George Williams */
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

int coverageformatsallowed=3;

#include "ttf.h"

/* This file contains routines to create the otf gpos and gsub tables and their */
/*  attendant subtables */

struct lookup {		/* It used to be there was one lookup for every */
			/*  subtable, so I could use lookup and subtable */
			/*  interchangeably. But due to a bug in Word/  */
			/*  Uniscribe I can't do than now. These are actually */
			/*  subtables, not lookups. */
    /*int script;*/
    uint32 feature_tag;
    int lookup_type;
    int lookup_cnt;
    uint32 offset[2];	/* offset[0] is normal offset, offset[1] is extension offset */
			/* One subtable might be referenced by both depending */
			/* on what lookups it appears in (all subtables in a */
			/* lookup must be refed the same way so if one needs */
			/* an extension, then all do */
    uint32 len;
    struct lookup *next;
    struct lookup *feature_next;
    uint16 flags;
    uint8 used;
    uint8 already_extended;
    int script_lang_index;
};

/* Undocumented fact: ATM (which does kerning for otf fonts in Word) can't handle features with multiple lookups */
/*  So if we have multiple lookups with the same tag/flags they must be merged */
/*  one lookup pointing to many subtables */
/* But conversely, ttx can't handle this. So we only want to do it for kerns */
/* Alexej Kryukov suggests instead that we merge all sub-tables with the same */
/*  sli/tag/flags. Currently no one generates different kerning tables depend-*/
/*  ing on the language, so it will catch (current) kerns. */
struct clookup {		/* Coalesced lookups */
    uint8 duplicate;		/* This same set of lookups is used in an earlier feature. We don't need to output the lookup data */
    uint8 extension;		/* This set of lookups must be referenced throw an extension table */
    uint16 flags;
    int lookup_type;
    int lcnt;
    struct lookup **lookups;
    int lookup_cnt;		/* Index of lookup in table */
    struct clookup *next;	/* Next in table */
    struct clookup *fnext;	/* Next in feature */
    int ticked;
};

struct postponedlookup {
    uint32 feature_tag;
    int lookup_cnt;
    struct postponedlookup *next;
};

/* Undocumented fact: Only one feature with a given tag allowed per script/lang */
/*  So if we have multiple lookups with the same tag they must be merged into */
/*  one feature with many lookups */
struct feature {
    uint32 feature_tag;
    int lcnt;
    struct clookup *lookups;
    int baselcnt;
    struct lookup **baselookups;
    struct sl { uint32 script, lang; struct sl *next; } *sl;	/* More than one script/lang may use this feature */
    int feature_cnt;
    struct feature *feature_next;
    int is_size;		/* Must be treated separately, no lookups, needs params */
};

struct tagflaglang {
    uint32 tag;
    uint16 flags;
    int script_lang_index;
};

/* scripts (for opentype) that I understand */
    /* see also list in charinfo.c */

static uint32 scripts[][15] = {
/* Arabic */	{ CHR('a','r','a','b'), 0x0600, 0x06ff, 0xfb50, 0xfdff, 0xfe70, 0xfeff },
 /* Aramaic */	{ CHR('a','r','a','m'), 0x820, 0x83f },
/* Armenian */	{ CHR('a','r','m','n'), 0x0530, 0x058f, 0xfb13, 0xfb17 },
/* Balinese */	{ CHR('b','a','l','i'), 0x1b00, 0x1b7f },
/* Bengali */	{ CHR('b','e','n','g'), 0x0980, 0x09ff },
/* Bliss symb */{ CHR('b','l','i','s'), 0x12200, 0x124ff },
/* Bopomofo */	{ CHR('b','o','p','o'), 0x3100, 0x312f, 0x31a0, 0x31bf },
/* Braille */	{ CHR('b','r','a','i'), 0x2800, 0x28ff },
/* Buginese */	{ CHR('b','u','g','i'), 0x1a00, 0x1a1f },
/* Buhid */	{ CHR('b','u','h','d'), 0x1740, 0x1753 },
/* Byzantine M*/{ CHR('b','y','z','m'), 0x1d000, 0x1d0ff },
/* Canadian Syl*/{CHR('c','a','n','s'), 0x1400, 0x167f },
/* Cherokee */	{ CHR('c','h','e','r'), 0x13a0, 0x13ff },
/* Cirth */	{ CHR('c','i','r','t'), 0x12080, 0x120ff },
/* CJKIdeogra */{ CHR('h','a','n','i'), 0x3300, 0x9fff, 0xf900, 0xfaff, 0x020000, 0x02ffff },
/* Coptic */	{ CHR('c','o','p','t'), 0x2c80, 0x2cff },
/* Cypriot */	{ CHR('c','p','m','n'), 0x10800, 0x1083f },
/* Cyrillic */	{ CHR('c','y','r','l'), 0x0400, 0x052f },
/* Deseret */	{ CHR('d','s','r','t'), 0x10400, 0x1044f },
/* Devanagari */{ CHR('d','e','v','a'), 0x0900, 0x097f },
/* Ethiopic */	{ CHR('e','t','h','i'), 0x1200, 0x139f },
/* Georgian */	{ CHR('g','e','o','r'), 0x1080, 0x10ff },
/* Glagolitic */{ CHR('g','l','a','g'), 0x1080, 0x10ff },
/* Gothic */	{ CHR('g','o','t','h'), 0x10330, 0x1034a },
/* Greek */	{ CHR('g','r','e','k'), 0x0370, 0x03ff, 0x1f00, 0x1fff },
/* Gujarati */	{ CHR('g','u','j','r'), 0x0a80, 0x0aff },
/* Gurmukhi */	{ CHR('g','u','r','u'), 0x0a00, 0x0a7f },
/* Hangul */	{ CHR('h','a','n','g'), 0xac00, 0xd7af, 0x3130, 0x319f, 0xffa0, 0xff9f },
/* Hanunoo */	{ CHR('h','a','n','o'), 0x1720, 0x1734 },
 /* I'm not sure what the difference is between the 'hang' tag and the 'jamo' */
 /*  tag. 'Jamo' is said to be the precomposed forms, but what's 'hang'? */
/* Hebrew */	{ CHR('h','e','b','r'), 0x0590, 0x05ff, 0xfb1e, 0xfb4f },
#if 0	/* Hiragana used to have its own tag, but has since been merged with katakana */
/* Hiragana */	{ CHR('h','i','r','a'), 0x3040, 0x309f },
#endif
/* Hangul Jamo*/{ CHR('j','a','m','o'), 0x1100, 0x11ff, 0x3130, 0x319f, 0xffa0, 0xffdf },
/* Katakana */	{ CHR('k','a','n','a'), 0x3040, 0x30ff, 0xff60, 0xff9f },
/* Javanese */	{ CHR('j','a','v','a'), 0 },	/* MS has a tag, but there is no unicode range */
/* Khmer */	{ CHR('k','h','m','r'), 0x1780, 0x17ff },
/* Kannada */	{ CHR('k','n','d','a'), 0x0c80, 0x0cff },
/* Kharosthi */	{ CHR('k','h','a','r'), 0x10a00, 0x10a5f },
/* Latin */	{ CHR('l','a','t','n'), 0x0041, 0x005a, 0x0061, 0x007a,
	0x00c0, 0x02af, 0x1d00, 0x1eff, 0xfb00, 0xfb0f, 0xff00, 0xff5f, 0, 0 },
/* Lao */	{ CHR('l','a','o',' '), 0x0e80, 0x0eff },
/* Limbu */	{ CHR('l','i','m','b'), 0x1900, 0x194f },
/* Linear A */	{ CHR('l','i','n','a'), 0x10180, 0x102cf },
/* Linear B */	{ CHR('l','i','n','b'), 0x10000, 0x100fa },
/* Malayalam */	{ CHR('m','l','y','m'), 0x0d00, 0x0d7f },
/* Mathematical Alphanumeric Symbols */
		{ CHR('m','a','t','h'), 0x1d400, 0x1d7ff },
/* Mongolian */	{ CHR('m','o','n','g'), 0x1800, 0x18af },
/* Musical */	{ CHR('m','u','s','i'), 0x1d100, 0x1d1ff },
/* Myanmar */	{ CHR('m','y','m','r'), 0x1000, 0x107f },
/* N'Ko */	{ CHR('n','k','o',' '), 0x07c0, 0x07fa },
/* Ogham */	{ CHR('o','g','a','m'), 0x1680, 0x169f },
/* Old Italic */{ CHR('i','t','a','l'), 0x10300, 0x1031e },
/* Old Permic */{ CHR('p','e','r','m'), 0x10350, 0x1037f },
/* Old Persian cuneiform */
		{ CHR('x','p','e','o'), 0x103a0, 0x103df },
/* Oriya */	{ CHR('o','r','y','a'), 0x0b00, 0x0b7f },
/* Osmanya */	{ CHR('o','s','m','a'), 0x10480, 0x104a9 },
/* Phags-pa */	{ CHR('p','h','a','g'), 0xa840, 0xa87f },
/* Phoenician */{ CHR('p','h','n','x'), 0x10900, 0x1091f },
/* Pollard */	{ CHR('p','l','r','d'), 0x104b0, 0x104d9 },
/* Runic */	{ CHR('r','u','n','r'), 0x16a0, 0x16ff },
/* Shavian */	{ CHR('s','h','a','w'), 0x10450, 0x1047f },
/* Sinhala */	{ CHR('s','i','n','h'), 0x0d80, 0x0dff },
/* Sumero-Akkadian Cuneiform */
		{ CHR('x','s','u','x'), 0x12000, 0x1236e },
/* Syloti Nagri */
		{ CHR('s','y','l','o'), 0xa800, 0xa82f },
/* Syriac */	{ CHR('s','y','r','c'), 0x0700, 0x074f },
/* Tagalog */	{ CHR('t','a','g','l'), 0x1700, 0x1714 },
/* Tagbanwa */	{ CHR('t','a','g','b'), 0x1760, 0x1773 },
/* Tai Le */	{ CHR('t','a','l','e'), 0x1950, 0x1974 },
/* Tai Lu */	{ CHR('t','a','l','u'), 0x1980, 0x19df },
/* Tamil */	{ CHR('t','a','m','l'), 0x0b80, 0x0bff },
/* Telugu */	{ CHR('t','e','l','u'), 0x0c00, 0x0c7f },
/* Tengwar */	{ CHR('t','e','n','g'), 0x12000, 0x1207f },
/* Thaana */	{ CHR('t','h','a','a'), 0x0780, 0x07bf },
/* Thai */	{ CHR('t','h','a','i'), 0x0e00, 0x0e7f },
/* Tibetan */	{ CHR('t','i','b','t'), 0x0f00, 0x0fff },
/* Tifinagh */	{ CHR('t','f','n','g'), 0x2d30, 0x2d7f },
/* Ugaritic */	{ CHR('u','g','a','r'), 0x10380, 0x1039d },
/* Yi */	{ CHR('y','i',' ',' '), 0xa000, 0xa73f },
		{ 0 }
};

int ScriptIsRightToLeft(uint32 script) {
    if ( script==CHR('a','r','a','b') || script==CHR('h','e','b','r') ||
	    script==CHR('c','p','m','n') || script==CHR('k','h','a','r') ||
	    script==CHR('s','y','r','c') || script==CHR('t','h','a','a') ||
	    script==CHR('n','k','o',' '))
return( true );

return( false );
}

uint32 ScriptFromUnicode(int u,SplineFont *sf) {
    int s, k;

    if ( u!=-1 ) {
	for ( s=0; scripts[s][0]!=0; ++s ) {
	    for ( k=1; scripts[s][k+1]!=0; k += 2 )
		if ( u>=scripts[s][k] && u<=scripts[s][k+1] )
	    break;
	    if ( scripts[s][k+1]!=0 )
	break;
	}
	if ( scripts[s][0]!=0 ) {
	    extern int use_second_indic_scripts;
	    uint32 script = scripts[s][0];
	    if ( use_second_indic_scripts ) {
		/* MS has a parallel set of script tags for their new */
		/*  Indic font shaper */
		if ( script == CHR('b','e','n','g' )) script = CHR('b','n','g','2');
		else if ( script == CHR('d','e','v','a' )) script = CHR('d','e','v','2');
		else if ( script == CHR('g','u','j','r' )) script = CHR('g','j','r','2');
		else if ( script == CHR('g','u','r','u' )) script = CHR('g','u','r','2');
		else if ( script == CHR('k','n','d','a' )) script = CHR('k','n','d','2');
		else if ( script == CHR('m','l','y','m' )) script = CHR('m','l','y','2');
		else if ( script == CHR('o','r','y','a' )) script = CHR('o','r','y','2');
		else if ( script == CHR('t','a','m','l' )) script = CHR('t','m','l','2');
		else if ( script == CHR('t','e','l','u' )) script = CHR('t','e','l','2');
	    }
return( script );
	}
    }

    if ( sf==NULL )
return( 0 );
    if ( sf->cidmaster!=NULL || sf->subfontcnt!=0 ) {
	if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
	if ( strmatch(sf->ordering,"Identity")==0 )
return( 0 );
	else if ( strmatch(sf->ordering,"Korean")==0 )
return( CHR('h','a','n','g'));
	else
return( CHR('h','a','n','i') );
    }

return( DEFAULT_SCRIPT );
}

uint32 SCScriptFromUnicode(SplineChar *sc) {
    char *pt;
    PST *pst;
    SplineFont *sf;
    int i; unsigned uni;

    if ( sc==NULL )
return( DEFAULT_SCRIPT );

    sf = sc->parent;
    if ( sc->unicodeenc!=-1 )
return( ScriptFromUnicode( sc->unicodeenc,sf ));

    pt = sc->name;
    if ( *pt ) for ( ++pt; *pt!='\0' && *pt!='_' && *pt!='.'; ++pt );
    if ( *pt!='\0' ) {
	char *str = copyn(sc->name,pt-sc->name);
	int uni = sf==NULL || sf->fv==NULL ? UniFromName(str,ui_none,&custom) :
			    UniFromName(str,sf->uni_interp,sf->fv->map->enc);
	free(str);
	if ( uni!=-1 )
return( ScriptFromUnicode( uni,sf ));
    }
    /* Adobe ligature uniXXXXXXXX */
    if ( strncmp(sc->name,"uni",3)==0 && sscanf(sc->name+3,"%4x", &uni)==1 )
return( ScriptFromUnicode( uni,sf ));

    if ( sf==NULL )
return( DEFAULT_SCRIPT );

    if ( sf->cidmaster ) sf=sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;
    for ( i=0; i<2; ++i ) {
	for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	    if ( sf->script_lang==NULL || pst->script_lang_index>=sf->sli_cnt )
	continue;
	    if ( pst->script_lang_index!=SLI_UNKNOWN &&
		    pst->type!=pst_lcaret &&
		    pst->script_lang_index!=SLI_NESTED &&
		    (i==1 || sf->script_lang[pst->script_lang_index][1].script==0 ))
return( sf->script_lang[pst->script_lang_index]->script );
	}
    }
return( ScriptFromUnicode( sc->unicodeenc,sf ));
}

int SCRightToLeft(SplineChar *sc) {

    if ( sc->unicodeenc>=0x10800 && sc->unicodeenc<=0x10fff )
return( true );		/* Supplemental Multilingual Plane, RTL scripts */
    if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 )
return( isrighttoleft(sc->unicodeenc ));

return( ScriptIsRightToLeft(SCScriptFromUnicode(sc)));
}

int SLIContainsR2L(SplineFont *sf,int sli) {
    struct script_record *sr;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;
    sr = sf->script_lang[sli];
return( ScriptIsRightToLeft(sr[0].script) );
}

static KernPair *KernListMatch(KernPair *kerns,int sli) {
    KernPair *kp;

    for ( kp=kerns; kp!=NULL; kp=kp->next )
	if ( kp->sli == sli && SCWorthOutputting(kp->sc))
return( kp );
return( NULL );
}

static LigList *LigListMatchTag(LigList *ligs,struct tagflaglang *tfl) {
    LigList *l;

    for ( l=ligs; l!=NULL; l=l->next )
	if ( l->lig->tag == tfl->tag && l->lig->flags==tfl->flags &&
		l->lig->script_lang_index == tfl->script_lang_index )
return( l );
return( NULL );
}

static int AllToBeOutput(LigList *lig) {
    struct splinecharlist *cmp;

    if ( lig->lig->u.lig.lig->ttf_glyph==-1 ||
	    lig->first->ttf_glyph==-1 )
return( 0 );
    for ( cmp=lig->components; cmp!=NULL; cmp=cmp->next )
	if ( cmp->sc->ttf_glyph==-1 )
return( 0 );
return( true );
}

static PST *PosSubMatchTag(PST *pst,struct tagflaglang *tfl,enum possub_type type) {

    for ( ; pst!=NULL; pst=pst->next )
	if ( pst->tag == tfl->tag && pst->type==type && pst->flags==tfl->flags &&
		pst->script_lang_index == tfl->script_lang_index )
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

static SplineChar **FindSubs(SplineChar *sc,struct tagflaglang *tfl, enum possub_type type) {
    SplineChar *spc[30], **space = spc;
    int max = sizeof(space)/sizeof(space[0]);
    int cnt=0;
    char *pt, *start;
    SplineChar *subssc, **ret;
    PST *pst;

    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->tag == tfl->tag && pst->flags==tfl->flags &&
		pst->script_lang_index == tfl->script_lang_index && pst->type==type ) {
	    pt = pst->u.subs.variant;
	    while ( 1 ) {
		while ( *pt==' ' ) ++pt;
		start = pt;
		pt = strchr(start,' ');
		if ( pt!=NULL )
		    *pt = '\0';
		subssc = SFGetChar(sc->parent,-1,start);
		if ( subssc!=NULL && subssc->ttf_glyph!=-1 ) {
		    if ( cnt>=max ) {
			if ( spc==space ) {
			    space = galloc((max+=30)*sizeof(SplineChar *));
			    memcpy(space,spc,(max-30)*sizeof(SplineChar *));
			} else
			    space = grealloc(space,(max+=30)*sizeof(SplineChar *));
		    }
		    space[cnt++] = subssc;
		}
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
    if ( space!=spc )
	free(space);
return( ret );
}

static SplineChar **generateGlyphList(SplineFont *sf, int iskern, int sli,
	struct tagflaglang *ligtag,struct glyphinfo *gi) {
    int cnt;
    SplineChar *sc;
    int i,j;
    SplineChar **glyphs=NULL;

    for ( j=0; j<2; ++j ) {
	cnt = 0;
	for ( i=0; i<gi->gcnt; ++i ) {
	    if ( gi->bygid[i]!=-1 && SCWorthOutputting(sc=sf->glyphs[gi->bygid[i]]) &&
		    sc->ttf_glyph!=-1 &&
		    ((iskern==1 && KernListMatch(sc->kerns,sli)) ||
		     (iskern==2 && KernListMatch(sc->vkerns,sli)) ||
		     (!iskern && LigListMatchTag(sc->ligofme,ligtag) &&
			 AllToBeOutput(sc->ligofme)) )) {
		if ( glyphs!=NULL ) glyphs[cnt] = sc;
		++cnt;
	    }
	}
	if ( glyphs==NULL ) {
	    if ( cnt==0 )
return( NULL );
	    glyphs = galloc((cnt+1)*sizeof(SplineChar *));
	}
    }
    glyphs[cnt] = NULL;

    /* Make sure it is ordered by ttf_glyph */
    for ( i=0; i<cnt-1; ++i ) {
	for ( j=i+1; j<cnt; ++j )
	    if ( glyphs[i]->ttf_glyph > glyphs[j]->ttf_glyph ) {
		SplineChar *sc = glyphs[i];
		glyphs[i] = glyphs[j];
		glyphs[j] = sc;
	    }
    }
return( glyphs );
}

static SplineChar **generateGlyphTypeList(SplineFont *sf, enum possub_type type,
	struct tagflaglang *tfl, SplineChar ****map,struct glyphinfo *gi) {
    int cnt;
    SplineChar *sc;
    int i,j;
    SplineChar **glyphs=NULL, ***maps=NULL;

    for ( j=0; j<2; ++j ) {
	cnt = 0;
	for ( i=0; i<gi->gcnt; ++i ) {
	    if ( gi->bygid[i]!=-1 && SCWorthOutputting(sc=sf->glyphs[gi->bygid[i]]) &&
		    PosSubMatchTag(sc->possub,tfl,type) ) {
		if ( glyphs!=NULL ) {
		    glyphs[cnt] = sc;
		    if ( type==pst_substitution || type==pst_alternate || type==pst_multiple ) {
			maps[cnt] = FindSubs(sc,tfl,type);
			if ( maps[cnt]==NULL )
			    --cnt;		/* Couldn't find anything, toss it */
		    }
		}
		++cnt;
		sc->ticked = true;
	    }
	}
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

void AnchorClassDecompose(SplineFont *sf,AnchorClass *_ac, int classcnt, int *subcnts,
	SplineChar ***marks,SplineChar ***base,
	SplineChar ***lig,SplineChar ***mkmk,
	struct glyphinfo *gi) {
    /* Run through the font finding all characters with this anchor class */
    /*  (and the cnt-1 classes after it) */
    /*  and distributing in the four possible anchor types */
    int i,j,k,gid, gmax;
    struct sclist { int cnt; SplineChar **glyphs; } heads[at_max];
    AnchorPoint *test;
    AnchorClass *ac;

    memset(heads,0,sizeof(heads));
    memset(subcnts,0,classcnt*sizeof(int));
    memset(marks,0,classcnt*sizeof(SplineChar **));
    gmax = gi==NULL ? sf->glyphcnt : gi->gcnt;
    for ( j=0; j<2; ++j ) {
	for ( i=0; i<gmax; ++i ) if ( (gid = gi==NULL ? i : gi->bygid[i])!=-1 && sf->glyphs[gid]!=NULL ) {
	    for ( ac = _ac, k=0; k<classcnt; ac=ac->next ) if ( ac->matches ) {
		for ( test=sf->glyphs[gid]->anchor; test!=NULL ; test=test->next ) {
		    if ( test->anchor==ac ) {
			if ( test->type==at_mark ) {
			    if ( j )
				marks[k][subcnts[k]] = sf->glyphs[gid];
			    ++subcnts[k];
			    if ( ac->type!=act_mkmk )
		break;
			} else if ( test->type!=at_centry && test->type!=at_cexit ) {
			    if ( heads[test->type].glyphs!=NULL ) {
			    /* If we have multiple mark classes, we may use the same base glyph */
			    /* with more than one mark class. But it should only appear once in */
			    /* the output */
				if ( heads[test->type].cnt==0 ||
					 heads[test->type].glyphs[heads[test->type].cnt-1]!=sf->glyphs[gid] ) {
				    heads[test->type].glyphs[heads[test->type].cnt] = sf->glyphs[gid];
				    ++heads[test->type].cnt;
				}
			    } else
				++heads[test->type].cnt;
			    if ( ac->type!=act_mkmk )
		break;
			}
		    }
		}
		++k;
	    }
	}
	if ( j==1 )
    break;
	for ( i=0; i<4; ++i )
	    if ( heads[i].cnt!=0 ) {
		heads[i].glyphs = galloc((heads[i].cnt+1)*sizeof(SplineChar *));
		/* I used to set glyphs[cnt] to NULL here. But it turns out */
		/*  cnt may be an overestimate on the first pass. So we can */
		/*  only set it at the end of the second pass */
		heads[i].cnt = 0;
	    }
	for ( k=0; k<classcnt; ++k ) if ( subcnts[k]!=0 ) {
	    marks[k] = galloc((subcnts[k]+1)*sizeof(SplineChar *));
	    marks[k][subcnts[k]] = NULL;
	    subcnts[k] = 0;
	}
    }
    for ( i=0; i<4; ++i )
	if ( heads[i].glyphs!=NULL )
	    heads[i].glyphs[heads[i].cnt] = NULL;

    *base = heads[at_basechar].glyphs;
    *lig  = heads[at_baselig].glyphs;
    *mkmk = heads[at_basemark].glyphs;
}

SplineChar **EntryExitDecompose(SplineFont *sf,AnchorClass *ac,struct glyphinfo *gi) {
    /* Run through the font finding all characters with this anchor class */
    int i,j, cnt, gmax, gid;
    SplineChar **array;
    AnchorPoint *test;

    array=NULL;
    gmax = gi==NULL ? sf->glyphcnt : gi->gcnt;
    for ( j=0; j<2; ++j ) {
	cnt = 0;
	for ( i=0; i<gmax; ++i ) if ( (gid = gi==NULL ? i : gi->bygid[i])!=-1 && sf->glyphs[gid]!=NULL ) {
	    for ( test=sf->glyphs[gid]->anchor; test!=NULL && test->anchor!=ac; test=test->next );
	    if ( test!=NULL && (test->type==at_centry || test->type==at_cexit )) {
		if ( array!=NULL )
		    array[cnt] = sf->glyphs[gid];
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
    uint32 script = SCScriptFromUnicode(base[first]);

    glyphs = NULL;
    while ( 1 ) {
	cnt = 1;
	for ( k=first+1; base[k]!=NULL; ++k ) if ( script==SCScriptFromUnicode(base[k]) ) {
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

    /* the order in which we examine the glyphs does not matter here, so */
    /*  we needn't add the complexity running though in gid order */
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i] ) {
	basec = markc = 0;
	for ( ap = sf->glyphs[i]->anchor; ap!=NULL; ap=ap->next )
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
	    IError("Glyphs must be ordered when creating coverage table");
	if ( glyphs[i]->ttf_glyph!=last+1 )
	    ++range_cnt;
	last = glyphs[i]->ttf_glyph;
    }
    /* I think Windows will only accept format 2 coverage tables? */
    if ( !(coverageformatsallowed&2) || ((coverageformatsallowed&1) && i<=3*range_cnt )) {
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

SplineChar **SFGlyphsFromNames(SplineFont *sf,char *names) {
    int cnt, ch;
    char *pt, *end;
    SplineChar *sc, **glyphs;

    cnt = 0;
    for ( pt = names; *pt; pt = end+1 ) {
	++cnt;
	end = strchr(pt,' ');
	if ( end==NULL )
    break;
    }

    glyphs = galloc((cnt+1)*sizeof(SplineChar *));
    cnt = 0;
    for ( pt = names; *pt; pt = end+1 ) {
	end = strchr(pt,' ');
	if ( end==NULL )
	    end = pt+strlen(pt);
	ch = *end;
	*end = '\0';
	sc = SFGetChar(sf,-1,pt);
	if ( sc!=NULL && sc->ttf_glyph!=-1 )
	    glyphs[cnt++] = sc;
	*end = ch;
	if ( ch=='\0' )
    break;
    }
    glyphs[cnt] = NULL;
return( glyphs );
}

static SplineChar **OrderedGlyphsFromNames(SplineFont *sf,char *names) {
    SplineChar **glyphs = SFGlyphsFromNames(sf,names);
    int i,j, end;

    if ( glyphs==NULL || glyphs[0]==NULL )
return( glyphs );

    for ( i=0; glyphs[i+1]!=NULL; ++i ) for ( j=i+1; glyphs[j]!=NULL; ++j ) {
	if ( glyphs[i]->ttf_glyph > glyphs[j]->ttf_glyph ) {
	    SplineChar *sc = glyphs[i];
	    glyphs[i] = glyphs[j];
	    glyphs[j] = sc;
	}
    }
    end = i;
    if ( glyphs[0]!=NULL ) {		/* Glyphs should not appear twice in the name list, just just in case they do... */
	for ( i=0; glyphs[i+1]!=NULL; ++i ) {
	    if ( glyphs[i]==glyphs[i+1] ) {
		for ( j=i+1; glyphs[j]!=NULL; ++j )
		    glyphs[j] = glyphs[j+1];
	    }
	}
    }
return( glyphs );
}

#ifdef FONTFORGE_CONFIG_DEVICETABLES
static int devtaboffsetsize(DeviceTable *dt) {
    int type = 1, i;

    for ( i=dt->last_pixel_size-dt->first_pixel_size; i>=0; --i ) {
	if ( dt->corrections[i]>=8 || dt->corrections[i]<-8 )
return( 3 );
	else if ( dt->corrections[i]>=2 || dt->corrections[i]<-2 )
	    type = 2;
    }
return( type );
}

static void dumpgposdevicetable(FILE *gpos,DeviceTable *dt) {
    int type;
    int i,cnt,b;

    if ( dt->corrections==NULL )
return;
    type = devtaboffsetsize(dt);
    putshort(gpos,dt->first_pixel_size);
    putshort(gpos,dt->last_pixel_size );
    putshort(gpos,type);
    cnt = dt->last_pixel_size - dt->first_pixel_size + 1;
    if ( type==3 ) {
	for ( i=0; i<cnt; ++i )
	    putc(dt->corrections[i],gpos);
	if ( cnt&1 )
	    putc(0,gpos);
    } else if ( type==2 ) {
	for ( i=0; i<cnt; i+=4 ) {
	    int val = 0;
	    for ( b=0; b<4 && i+b<cnt; ++b )
		val |= (dt->corrections[i+b]&0x000f)<<(12-b*4);
	    putshort(gpos,val);
	}
    } else {
	for ( i=0; i<cnt; i+=8 ) {
	    int val = 0;
	    for ( b=0; b<8 && i+b<cnt; ++b )
		val |= (dt->corrections[i+b]&0x0003)<<(14-b*2);
	    putshort(gpos,val);
	}
    }
}

static int DevTabLen(DeviceTable *dt) {
    int type;
    int cnt;

    if ( dt->corrections==NULL )
return( 0 );
    cnt = dt->last_pixel_size - dt->first_pixel_size + 1;
    type = devtaboffsetsize(dt);
    if ( type==3 )
	cnt = (cnt+1)/2;
    else if ( type==2 )
	cnt = (cnt+3)/4;
    else
	cnt = (cnt+7)/8;
    cnt += 3;		/* first, last, type */
return( sizeof(uint16)*cnt );
}

static int gposdumpvaldevtab(FILE *gpos,ValDevTab *vdt,int bits,int next_dev_tab ) {

    if ( bits&0x10 ) {
	if ( vdt==NULL || vdt->xadjust.corrections==NULL )
	    putshort(gpos,0);
	else {
	    putshort(gpos,next_dev_tab);
	    next_dev_tab += DevTabLen(&vdt->xadjust);
	}
    }
    if ( bits&0x20 ) {
	if ( vdt==NULL || vdt->yadjust.corrections==NULL )
	    putshort(gpos,0);
	else {
	    putshort(gpos,next_dev_tab);
	    next_dev_tab += DevTabLen(&vdt->yadjust);
	}
    }
    if ( bits&0x40 ) {
	if ( vdt==NULL || vdt->xadv.corrections==NULL )
	    putshort(gpos,0);
	else {
	    putshort(gpos,next_dev_tab);
	    next_dev_tab += DevTabLen(&vdt->xadv);
	}
    }
    if ( bits&0x80 ) {
	if ( vdt==NULL || vdt->yadv.corrections==NULL )
	    putshort(gpos,0);
	else {
	    putshort(gpos,next_dev_tab);
	    next_dev_tab += DevTabLen(&vdt->yadv);
	}
    }
return( next_dev_tab );
}

static int DevTabsSame(DeviceTable *dt1, DeviceTable *dt2) {
    DeviceTable _dt;
    int i;

    if ( dt1==NULL && dt2==NULL )
return( false );
    if ( dt1==NULL ) {
	memset(&_dt,0,sizeof(_dt));
	dt1 = &_dt;
    }
    if ( dt2==NULL ) {
	memset(&_dt,0,sizeof(_dt));
	dt2 = &_dt;
    }
    if ( dt1->corrections==NULL && dt2->corrections==NULL )
return( true );
    if ( dt1->corrections==NULL || dt2->corrections==NULL )
return( false );
    if ( dt1->first_pixel_size!=dt2->first_pixel_size ||
	    dt1->last_pixel_size!=dt2->last_pixel_size )
return( false );
    for ( i=dt2->last_pixel_size-dt1->first_pixel_size; i>=0; --i )
	if ( dt1->corrections[i]!=dt2->corrections[i] )
return( false );

return( true );
}

static int ValDevTabsSame(ValDevTab *vdt1, ValDevTab *vdt2) {
    ValDevTab _vdt;

    if ( vdt1==NULL && vdt2==NULL )
return( false );
    if ( vdt1==NULL ) {
	memset(&_vdt,0,sizeof(_vdt));
	vdt1 = &_vdt;
    }
    if ( vdt2==NULL ) {
	memset(&_vdt,0,sizeof(_vdt));
	vdt2 = &_vdt;
    }
return( DevTabsSame(&vdt1->xadjust,&vdt2->xadjust) &&
	DevTabsSame(&vdt1->yadjust,&vdt2->yadjust) &&
	DevTabsSame(&vdt1->xadv,&vdt2->xadv) &&
	DevTabsSame(&vdt1->yadv,&vdt2->yadv) );
}
#endif

static void dumpgposkerndata(FILE *gpos,SplineFont *sf,int sli,
	    struct alltabs *at,int isv,
	    SplineChar **glyphs, int cnt ) {
    int32 coverage_pos, next_val_pos, here;
    int i, pcnt, max=100, j,k;
    int *seconds = galloc(max*sizeof(int));
    int *changes = galloc(max*sizeof(int));
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    int *devtabs = galloc(max*sizeof(int));
#endif
    int16 *offsets=NULL;
    KernPair *kp;
    uint32 script = (sf->cidmaster?sf->cidmaster:sf)->script_lang[sli][0].script;
    int isr2l = ScriptIsRightToLeft(script);
    int anydevtab = false;

    if ( glyphs!=NULL ) {
	if ( at->os2.maxContext<2 )
	    at->os2.maxContext = 2;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	for ( i=0; i<cnt && !anydevtab; ++i ) {
	    for ( kp = isv ? glyphs[i]->vkerns : glyphs[i]->kerns; kp!=NULL; kp=kp->next )
		if ( kp->sli==sli && kp->adjust!=NULL ) {
		    anydevtab = true;
	    break;
		}
	}
#endif
    }

    putshort(gpos,1);		/* format 1 of the pair adjustment subtable */
    coverage_pos = ftell(gpos);
    putshort(gpos,0);		/* offset to coverage table */
    if ( isv ) {
	/* As far as I know there is no "bottom to top" writing direction */
	putshort(gpos,anydevtab?0x0088:0x0008);	/* Alter YAdvance of first character */
	putshort(gpos,0x0000);			/* leave second char alone */
    } else if ( isr2l ) {
	/* Right to left kerns modify the second character's width */
	/*  this doesn't make sense to me, but who am I to argue */
	putshort(gpos,0x0000);			/* leave first char alone */
	putshort(gpos,anydevtab?0x0044:0x0004);	/* Alter XAdvance of second character */
    } else {
	putshort(gpos,anydevtab?0x0044:0x0004);	/* Alter XAdvance of first character */
	putshort(gpos,0x0000);			/* leave second char alone */
    }
    putshort(gpos,cnt);
    next_val_pos = ftell(gpos);
    if ( glyphs!=NULL )
	offsets = galloc(cnt*sizeof(int16));
    for ( i=0; i<cnt; ++i )
	putshort(gpos,0);
    for ( i=0; i<cnt; ++i ) {
	for ( pcnt = 0, kp = isv ? glyphs[i]->vkerns : glyphs[i]->kerns; kp!=NULL; kp=kp->next )
	    if ( kp->sli==sli ) ++pcnt;
	if ( pcnt>=max ) {
	    max = pcnt+100;
	    seconds = grealloc(seconds,max*sizeof(int));
	    changes = grealloc(changes,max*sizeof(int));
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    devtabs = grealloc(devtabs,max*sizeof(int));
#endif
	}
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	for ( pcnt=0,kp = isv ? glyphs[i]->vkerns : glyphs[i]->kerns; kp!=NULL; kp=kp->next )
	    if ( kp->sli==sli ) {
		if ( kp->adjust!=NULL ) {
		    devtabs[pcnt] = ftell(gpos)-coverage_pos+2;
		    dumpgposdevicetable(gpos,kp->adjust);
		} else
		    devtabs[pcnt] = 0;
		++pcnt;
	    }
#endif
	offsets[i] = ftell(gpos)-coverage_pos+2;
	putshort(gpos,pcnt);
	for ( pcnt = 0, kp = isv ? glyphs[i]->vkerns : glyphs[i]->kerns; kp!=NULL; kp=kp->next ) {
	    if ( kp->sli==sli ) {
		seconds[pcnt] = kp->sc->ttf_glyph;
		changes[pcnt++] = kp->off;
	    }
	}
	for ( j=0; j<pcnt-1; ++j ) for ( k=j+1; k<pcnt; ++k ) {
	    if ( seconds[k]<seconds[j] ) {
		int temp = seconds[k];
		seconds[k] = seconds[j];
		seconds[j] = temp;
		temp = changes[k];
		changes[k] = changes[j];
		changes[j] = temp;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		temp = devtabs[k];
		devtabs[k] = devtabs[j];
		devtabs[j] = temp;
#endif
	    }
	}
	for ( j=0; j<pcnt; ++j ) {
	    putshort(gpos,seconds[j]);
	    putshort(gpos,changes[j]);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    if ( anydevtab )
		putshort(gpos,devtabs[j]);
#endif
	}
    }
    free(seconds);
    free(changes);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    free(devtabs);
#endif
    if ( glyphs!=NULL ) {
	here = ftell(gpos);
	fseek(gpos,coverage_pos,SEEK_SET);
	putshort(gpos,here-coverage_pos+2);
	if ( here-coverage_pos+2>65535 )
	    IError( "Kerning table too big. Will not be useable.");
	fseek(gpos,next_val_pos,SEEK_SET);
	for ( i=0; i<cnt; ++i )
	    putshort(gpos,offsets[i]);
	fseek(gpos,here,SEEK_SET);
	dumpcoveragetable(gpos,glyphs);
	free(offsets);
    }
}

static struct lookup *dumpgposkerndatalookups(FILE *lfile,SplineFont *sf,int sli,
	    struct alltabs *at,int isv,struct lookup *lookups,
	    SplineChar **glyphs, int cnt ) {
    struct lookup *new;
    SplineChar *hold = glyphs[cnt];

    glyphs[cnt] = NULL;

    new = chunkalloc(sizeof(struct lookup));
    new->feature_tag = isv ? CHR('v','k','r','n') : CHR('k','e','r','n');
    /* new->flags = pst_ignorecombiningmarks; */	/* yudit doesn't like this flag to be set 2.7.2 */
    if ( SLIContainsR2L(sf,sli))
	new->flags = pst_r2l;
    new->script_lang_index = sli;
    new->lookup_type = 2;		/* Pair adjustment subtable type */
    new->offset[0] = ftell(lfile);
    new->next = lookups;
    dumpgposkerndata(lfile,sf,sli,at,isv,glyphs,cnt);
    new->len = ftell(lfile)-new->offset[0];

    glyphs[cnt] = hold;
return( new );
}

static struct lookup *dumpgposkernpairs(FILE *lfile,SplineFont *sf,int sli,
	    struct alltabs *at,int isv,struct lookup *lookups ) {
    SplineChar **glyphs;
    int cnt, tot, start, i;
    int anydevtab=0, devtabsize=0;
    KernPair *kp;
    /* Now it is perfectly possible to have more kern pairs than will fit */
    /*  into one 65536 sized kerning sub-table. So be prepared to create */
    /*  multiple sub-tables each with a small bit of data */

    glyphs = generateGlyphList(sf,isv+1,sli,NULL,&at->gi);
    cnt=tot=0;
    start = 0;
    if ( glyphs!=NULL ) {
	for ( ; glyphs[cnt]!=NULL; ++cnt );
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	for ( i=0; i<cnt && !anydevtab; ++i ) {
	    for ( kp = isv ? glyphs[i]->vkerns : glyphs[i]->kerns; kp!=NULL; kp=kp->next )
		if ( kp->sli==sli && kp->adjust!=NULL ) {
		    anydevtab = true;
	    break;
		}
	}
#endif
	for ( i=0; i<cnt ; ++i ) {
	    for ( kp = isv ? glyphs[i]->vkerns : glyphs[i]->kerns; kp!=NULL; kp=kp->next ) {
		if ( kp->sli==sli && SCWorthOutputting(kp->sc)) {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		    if ( kp->adjust!=NULL )
			devtabsize += DevTabLen(kp->adjust);
#endif
		    ++tot;
		}
	    }
	    if ( 5*sizeof(short)+		/* header */
		    (i-start)*sizeof(short)+	/* offset array */
		    (i-start)*sizeof(short)+	/* number of PairValueCounts */
		    tot*(2+anydevtab)*sizeof(short)+ /* One for each pair */
		    devtabsize +		/* size of any device tables */
		    100				/* Just in case I made a mistake */
		    > 65536 ) {
		if ( start==i )
		    IError("Kerning data for one glyph too big to fit in a single sub-table");
		else
		    lookups = dumpgposkerndatalookups(lfile,sf,sli,at,isv,lookups,glyphs+start,i-start);
		start = i;
		tot = devtabsize = 0;
		--i;		/* we didn't use it, so we must recount it */
	    }
	}
    }
    if ( cnt-start!=0 )
	lookups = dumpgposkerndatalookups(lfile,sf,sli,at,isv,lookups,glyphs+start,cnt-start);
    free(glyphs);
return( lookups );
}

uint16 *ClassesFromNames(SplineFont *sf,char **classnames,int class_cnt,
	int numGlyphs, SplineChar ***glyphs, int apple_kc) {
    uint16 *class;
    int i;
    char *pt, *end, ch;
    SplineChar *sc, **gs=NULL;
    int offset = (apple_kc && classnames[0]!=NULL);

    class = gcalloc(numGlyphs,sizeof(uint16));
    if ( glyphs ) *glyphs = gs = gcalloc(numGlyphs,sizeof(SplineChar *));
    for ( i=0; i<class_cnt; ++i ) {
	if ( i==0 && classnames[0]==NULL )
    continue;
	for ( pt = classnames[i]; *pt; pt = end+1 ) {
	    while ( *pt==' ' ) ++pt;
	    if ( *pt=='\0' )
	break;
	    end = strchr(pt,' ');
	    if ( end==NULL )
		end = pt+strlen(pt);
	    ch = *end;
	    *end = '\0';
	    sc = SFGetChar(sf,-1,pt);
	    if ( sc!=NULL && sc->ttf_glyph!=-1 ) {
		class[sc->ttf_glyph] = i+offset;
		if ( gs!=NULL )
		    gs[sc->ttf_glyph] = sc;
	    }
	    *end = ch;
	    if ( ch=='\0' )
	break;
	}
    }
return( class );
}

static SplineChar **GlyphsFromClasses(SplineChar **gs, int numGlyphs) {
    int i, cnt;
    SplineChar **glyphs;

    for ( i=cnt=0; i<numGlyphs; ++i )
	if ( gs[i]!=NULL ) ++cnt;
    glyphs = galloc((cnt+1)*sizeof(SplineChar *));
    for ( i=cnt=0; i<numGlyphs; ++i )
	if ( gs[i]!=NULL )
	    glyphs[cnt++] = gs[i];
    glyphs[cnt++] = NULL;
    free(gs);
return( glyphs );
}

static SplineChar **GlyphsFromInitialClasses(SplineChar **gs, int numGlyphs, uint16 *classes, uint16 *initial) {
    int i, j, cnt;
    SplineChar **glyphs;

    for ( i=cnt=0; i<numGlyphs; ++i ) {
	for ( j=0; initial[j]!=0xffff; ++j )
	    if ( initial[j]==classes[i])
	break;
	if ( initial[j]!=0xffff && gs[i]!=NULL ) ++cnt;
    }
    glyphs = galloc((cnt+1)*sizeof(SplineChar *));
    for ( i=cnt=0; i<numGlyphs; ++i ) {
	for ( j=0; initial[j]!=0xffff; ++j )
	    if ( initial[j]==classes[i])
	break;
	if ( initial[j]!=0xffff && gs[i]!=NULL )
	    glyphs[cnt++] = gs[i];
    }
    glyphs[cnt++] = NULL;
return( glyphs );
}

static void DumpClass(FILE *gpos,uint16 *class,int numGlyphs) {
    int ranges, i, cur, first= -1, last=-1, istart;

    for ( i=ranges=0; i<numGlyphs; ) {
	istart = i;
	cur = class[i];
	while ( i<numGlyphs && class[i]==cur )
	    ++i;
	if ( cur!=0 ) {
	    ++ranges;
	    if ( first==-1 ) first = istart;
	    last = i-1;
	}
    }
    if ( ranges*3+1>last-first+1+2 || first==-1 ) {
	if ( first==-1 ) first = last = 0;
	putshort(gpos,1);		/* Format 1, list of all posibilities */
	putshort(gpos,first);
	putshort(gpos,last-first+1);
	for ( i=first; i<=last ; ++i )
	    putshort(gpos,class[i]);
    } else {
	putshort(gpos,2);		/* Format 2, series of ranges */
	putshort(gpos,ranges);
	for ( i=0; i<numGlyphs; ) {
	    istart = i;
	    cur = class[i];
	    while ( i<numGlyphs && class[i]==cur )
		++i;
	    if ( cur!=0 ) {
		putshort(gpos,istart);
		putshort(gpos,i-1);
		putshort(gpos,cur);
	    }
	}
    }
}

static void dumpgposkernclass(FILE *gpos,SplineFont *sf,KernClass *kc,
	    struct alltabs *at,int isv) {
    uint32 begin_off = ftell(gpos), pos;
    uint32 script = sf->script_lang[kc->sli][0].script;
    uint16 *class1, *class2;
    SplineChar **glyphs;
    int i;
    int anydevtab = false;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    int next_devtab;
#endif

    putshort(gpos,2);		/* format 2 of the pair adjustment subtable */
    putshort(gpos,0);		/* offset to coverage table */
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    for ( i=0; i<kc->first_cnt*kc->second_cnt; ++i ) {
	if ( kc->adjusts[i].corrections!=NULL ) {
	    anydevtab = true;
    break;
	}
    }
#endif
    if ( isv ) {
	/* As far as I know there is no "bottom to top" writing direction */
	putshort(gpos,anydevtab?0x0088:0x0008);	/* Alter YAdvance of first character */
	putshort(gpos,0x0000);			/* leave second char alone */
    } else if ( ScriptIsRightToLeft(script) ) {
	/* Right to left kerns modify the second character's width */
	/*  this doesn't make sense to me, but who am I to argue */
	putshort(gpos,0x0000);			/* leave first char alone */
	putshort(gpos,anydevtab?0x0044:0x0004);	/* Alter XAdvance of second character */
    } else {
	putshort(gpos,anydevtab?0x0044:0x0004);	/* Alter XAdvance of first character */
	putshort(gpos,0x0000);			/* leave second char alone */
    }
    class1 = ClassesFromNames(sf,kc->firsts,kc->first_cnt,at->maxp.numGlyphs,&glyphs,false);
    glyphs = GlyphsFromClasses(glyphs,at->maxp.numGlyphs);
    class2 = ClassesFromNames(sf,kc->seconds,kc->second_cnt,at->maxp.numGlyphs,NULL,false);
    putshort(gpos,0);		/* offset to first glyph classes */
    putshort(gpos,0);		/* offset to second glyph classes */
    putshort(gpos,kc->first_cnt);
    putshort(gpos,kc->second_cnt);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    next_devtab = ftell(gpos)-begin_off + kc->first_cnt*kc->second_cnt*2*sizeof(uint16);
#endif
    for ( i=0; i<kc->first_cnt*kc->second_cnt; ++i ) {
	putshort(gpos,kc->offsets[i]);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( anydevtab && kc->adjusts[i].corrections!=NULL ) {
	    putshort(gpos,next_devtab);
	    next_devtab += DevTabLen(&kc->adjusts[i]);
	} else if ( anydevtab )
	    putshort(gpos,0);
#endif
    }
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    if ( anydevtab ) {
	for ( i=0; i<kc->first_cnt*kc->second_cnt; ++i ) {
	    if ( kc->adjusts[i].corrections!=NULL )
		dumpgposdevicetable(gpos,&kc->adjusts[i]);
	}
	if ( next_devtab!=ftell(gpos)-begin_off )
	    IError("Device table offsets screwed up in kerning class");
    }
#endif
    pos = ftell(gpos);
    fseek(gpos,begin_off+4*sizeof(uint16),SEEK_SET);
    putshort(gpos,pos-begin_off);
    fseek(gpos,pos,SEEK_SET);
    DumpClass(gpos,class1,at->maxp.numGlyphs);

    pos = ftell(gpos);
    fseek(gpos,begin_off+5*sizeof(uint16),SEEK_SET);
    putshort(gpos,pos-begin_off);
    fseek(gpos,pos,SEEK_SET);
    DumpClass(gpos,class2,at->maxp.numGlyphs);

    pos = ftell(gpos);
    fseek(gpos,begin_off+sizeof(uint16),SEEK_SET);
    putshort(gpos,pos-begin_off);
    fseek(gpos,pos,SEEK_SET);
    dumpcoveragetable(gpos,glyphs);

    free(glyphs);
    free(class1);
    free(class2);
}

static void dumpanchor(FILE *gpos,AnchorPoint *ap, int is_ttf ) {
    int base = ftell(gpos);

#ifdef FONTFORGE_CONFIG_DEVICETABLES
    if ( ap->xadjust.corrections!=NULL || ap->yadjust.corrections!=NULL )
	putshort(gpos,3);	/* format 3 w/ device tables */
    else
#endif
    if ( ap->has_ttf_pt && is_ttf )
	putshort(gpos,2);	/* format 2 w/ a matching ttf point index */
    else
	putshort(gpos,1);	/* Anchor format 1 just location*/
    putshort(gpos,ap->me.x);	/* X coord of attachment */
    putshort(gpos,ap->me.y);	/* Y coord of attachment */
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    if ( ap->xadjust.corrections!=NULL || ap->yadjust.corrections!=NULL ) {
	putshort(gpos,ap->xadjust.corrections==NULL?0:
		ftell(gpos)-base+4);
	putshort(gpos,ap->xadjust.corrections==NULL?0:
		ftell(gpos)-base+2+DevTabLen(&ap->xadjust));
	dumpgposdevicetable(gpos,&ap->xadjust);
	dumpgposdevicetable(gpos,&ap->yadjust);
    } else
#endif
    if ( ap->has_ttf_pt && is_ttf )
	putshort(gpos,ap->ttf_pt_index);
}

static struct lookup *dumpgposCursiveAttach(FILE *gpos,AnchorClass *ac,
	SplineFont *sf, struct lookup *lookups,struct glyphinfo *gi) {
    struct lookup *new;
    SplineChar **entryexit = EntryExitDecompose(sf,ac,gi), **glyphs;
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
	new->feature_tag = ac->feature_tag;
	new->flags = ac->flags;
	new->script_lang_index = ac->script_lang_index;
	new->lookup_type = 3;		/* Cursive positioning */
	new->offset[0] = ftell(gpos);
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
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		if ( entry->xadjust.corrections!=NULL || entry->yadjust.corrections!=NULL )
		    offset += 4 + DevTabLen(&entry->xadjust) + DevTabLen(&entry->yadjust);
#endif
	    } else
		putshort(gpos,0);
	    if ( exit!=NULL ) {
		putshort(gpos,offset);
		offset += 6;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		if ( exit->xadjust.corrections!=NULL || exit->yadjust.corrections!=NULL )
		    offset += 4 + DevTabLen(&exit->xadjust) + DevTabLen(&exit->yadjust);
		else
#endif
		if ( gi->is_ttf && ap->has_ttf_pt )
		    offset += 2;
	    } else
		putshort(gpos,0);
	}
	for ( j=0; j<cnt; ++j ) {
	    entry = exit = NULL;
	    for ( ap=glyphs[j]->anchor; ap!=NULL; ap=ap->next ) {
		if ( ap->anchor==ac && ap->type==at_centry ) entry = ap;
		if ( ap->anchor==ac && ap->type==at_cexit ) exit = ap;
	    }
	    if ( entry!=NULL )
		dumpanchor(gpos,entry,gi->is_ttf);
	    if ( exit!=NULL )
		dumpanchor(gpos,exit,gi->is_ttf);
	}
	coverage_offset = ftell(gpos);
	dumpcoveragetable(gpos,glyphs);
	fseek(gpos,new->offset[0]+2,SEEK_SET);
	putshort(gpos,coverage_offset-new->offset[0]);
	fseek(gpos,0,SEEK_END);
	new->len = ftell(gpos)-new->offset[0];
    }
return( lookups );
}

static int orderglyph(const void *_sc1,const void *_sc2) {
    SplineChar * const *sc1 = _sc1, * const *sc2 = _sc2;

return( (*sc1)->ttf_glyph - (*sc2)->ttf_glyph );
}

static SplineChar **allmarkglyphs(SplineChar ***glyphlist, int classcnt) {
    SplineChar **glyphs;
    int i, tot, k;

    if ( classcnt==1 )
return( glyphlist[0]);

    for ( i=tot=0; i<classcnt; ++i ) {
	for ( k=0; glyphlist[i][k]!=NULL; ++k );
	tot += k;
    }
    glyphs = galloc((tot+1)*sizeof(SplineChar *));
    for ( i=tot=0; i<classcnt; ++i ) {
	for ( k=0; glyphlist[i][k]!=NULL; ++k )
	    glyphs[tot++] = glyphlist[i][k];
    }
    qsort(glyphs,tot,sizeof(SplineChar *),orderglyph);
    for ( i=k=0; i<tot; ++i ) {
	while ( i+1<tot && glyphs[i]==glyphs[i+1]) ++i;
	glyphs[k++] = glyphs[i];
    }
    glyphs[k] = NULL;
return( glyphs );
}

static struct lookup *dumpgposAnchorData(FILE *gpos,AnchorClass *_ac,
	enum anchor_type at,
	SplineChar ***marks,SplineChar **base,struct lookup *lookups,
	int classcnt, struct glyphinfo *gi) {
    AnchorClass *ac=NULL;
    struct lookup *new;
    int j,cnt,k,l, pos, offset, suboffset, tot, max;
    uint32 coverage_offset, markarray_offset;
    AnchorPoint *ap, **aps;
    SplineChar **markglyphs;

    for ( cnt=0; base[cnt]!=NULL; ++cnt );
    new = chunkalloc(sizeof(struct lookup));
    new->feature_tag = _ac->feature_tag;
    new->flags = _ac->flags;
    new->script_lang_index = _ac->script_lang_index;
    new->lookup_type = 3+at;	/* One of the "mark to *" types 4,5,6 */
    new->offset[0] = ftell(gpos);
    new->next = lookups;
    lookups = new;

    putshort(gpos,1);	/* format 1 for this subtable */
    putshort(gpos,0);	/* Fill in later, offset to mark coverage table */
    putshort(gpos,0);	/* Fill in later, offset to base coverage table */
    putshort(gpos,classcnt);
    putshort(gpos,0);	/* Fill in later, offset to mark array */
    putshort(gpos,12);	/* Offset to base array */
	/* Base array */
    putshort(gpos,cnt);	/* Number of entries in array */
    if ( at==at_basechar || at==at_basemark ) {
	offset = 2;
	for ( l=0; l<3; ++l ) {
	    for ( j=0; j<cnt; ++j ) {
		for ( k=0, ac=_ac; k<classcnt; ++k, ac=ac->next ) {
		    for ( ap=base[j]->anchor; ap!=NULL && (ap->anchor!=ac || ap->type!=at);
			    ap=ap->next );
		    switch ( l ) {
		      case 0:
			offset += 2;
		      break;
		      case 1:
			if ( ap==NULL )
			    putshort(gpos,0);
			else {
			    putshort(gpos,offset);
			    offset += 6;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
			    if ( ap->xadjust.corrections!=NULL || ap->yadjust.corrections!=NULL )
				offset += 4 + DevTabLen(&ap->xadjust) + DevTabLen(&ap->yadjust);
			    else
#endif
			    if ( gi->is_ttf && ap->has_ttf_pt )
				offset += 2;
			}
		      break;
		      case 2:
			if ( ap!=NULL )
			    dumpanchor(gpos,ap,gi->is_ttf);
		      break;
		    }
		}
	    }
	}
    } else {
	offset = 2+2*cnt;
	suboffset = 0;
	max = 0;
	for ( j=0; j<cnt; ++j ) {
	    putshort(gpos,offset);
	    pos = tot = 0;
	    for ( ap=base[j]->anchor; ap!=NULL ; ap=ap->next )
		for ( k=0, ac=_ac; k<classcnt; ++k, ac=ac->next ) {
		    if ( ap->anchor==ac ) {
			if ( ap->lig_index>pos ) pos = ap->lig_index;
			++tot;
		    }
		}
	    if ( pos>max ) max = pos;
	    offset += 2+(pos+1)*classcnt*2+tot*6;
		/* 2 for component count, for each component an offset to an offset to an anchor record */
	}
	++max;
	aps = galloc((classcnt*max)*sizeof(AnchorPoint *));
	for ( j=0; j<cnt; ++j ) {
	    memset(aps,0,(classcnt*max)*sizeof(AnchorPoint *));
	    pos = 0;
	    for ( ap=base[j]->anchor; ap!=NULL ; ap=ap->next )
		for ( k=0, ac=_ac; k<classcnt; ++k, ac=ac->next ) {
		    if ( ap->anchor==ac ) {
			if ( ap->lig_index>pos ) pos = ap->lig_index;
			aps[k*max+ap->lig_index] = ap;
		    }
		}
	    ++pos;
	    putshort(gpos,pos);
	    offset = 2+2*pos*classcnt;
	    for ( l=0; l<pos; ++l ) {
		for ( k=0; k<classcnt; ++k ) {
		    if ( aps[k*max+l]==NULL )
			putshort(gpos,0);
		    else {
			putshort(gpos,offset);
			offset += 6;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
			if ( aps[k*max+l]->xadjust.corrections!=NULL || aps[k*max+l]->yadjust.corrections!=NULL )
			    offset += 4 + DevTabLen(&aps[k*max+l]->xadjust) +
					DevTabLen(&aps[k*max+l]->yadjust);
			else
#endif
			if ( gi->is_ttf && aps[k*max+l]->has_ttf_pt )
			    offset += 2;
		    }
		}
	    }
	    for ( l=0; l<pos; ++l ) {
		for ( k=0; k<classcnt; ++k ) {
		    if ( aps[k*max+l]!=NULL ) {
			/* !!! We could search through the character here to see */
			/*  if there is any point with this coord, and if so connect */
			/*  anchor to the point (anchor format 2) */
			dumpanchor(gpos,aps[k*max+l],gi->is_ttf);
		    }
		}
	    }
	}
	free(aps);
    }
    coverage_offset = ftell(gpos);
    fseek(gpos,lookups->offset[0]+4,SEEK_SET);
    putshort(gpos,coverage_offset-lookups->offset[0]);
    fseek(gpos,0,SEEK_END);
    dumpcoveragetable(gpos,base);

    /* We tried sharing the mark table, (among all these sub-tables) but */
    /*  that doesn't work because we need to be able to reorder the sub-tables */
    markglyphs = allmarkglyphs(marks,classcnt);
    coverage_offset = ftell(gpos);
    dumpcoveragetable(gpos,markglyphs);
    markarray_offset = ftell(gpos);
    for ( cnt=0; markglyphs[cnt]!=NULL; ++cnt );
    putshort(gpos,cnt);
    offset = 2+4*cnt;
    for ( j=0; j<cnt; ++j ) {
	if ( classcnt==0 ) {
	    putshort(gpos,0);		/* Only one class */
	    ap = NULL;
	} else {
	    for ( k=0, ac=_ac; k<classcnt; ac=ac->next ) {
		if ( ac->matches ) {
		    for ( ap = markglyphs[j]->anchor; ap!=NULL && (ap->anchor!=ac || ap->type!=at_mark);
			    ap=ap->next );
		    if ( ap!=NULL )
	    break;
		    ++k;
		}
	    }
	    putshort(gpos,k);
	}
	putshort(gpos,offset);
	offset += 6;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( ap!=NULL && (ap->xadjust.corrections!=NULL || ap->yadjust.corrections!=NULL ))
	    offset += 4 + DevTabLen(&ap->xadjust) + DevTabLen(&ap->yadjust);
	else
#endif
	if ( gi->is_ttf && ap->has_ttf_pt )
	    offset += 2;
    }
    for ( j=0; j<cnt; ++j ) {
	for ( k=0, ac=_ac; k<classcnt; ac=ac->next ) {
	    if ( ac->matches ) {
		for ( ap = markglyphs[j]->anchor; ap!=NULL && (ap->anchor!=ac || ap->type!=at_mark);
			ap=ap->next );
		if ( ap!=NULL )
	break;
		++k;
	    }
	}
	dumpanchor(gpos,ap,gi->is_ttf);
    }
    if ( markglyphs!=marks[0] )
	free(markglyphs);

    fseek(gpos,new->offset[0]+2,SEEK_SET);	/* mark coverage table offset */
    putshort(gpos,coverage_offset-new->offset[0]);
    fseek(gpos,4,SEEK_CUR);
    putshort(gpos,markarray_offset-new->offset[0]);

    fseek(gpos,0,SEEK_END);
    new->len = ftell(gpos)-new->offset[0];

return( lookups );
}

static void dumpGPOSsimplepos(FILE *gpos,SplineFont *sf,SplineChar **glyphs,
	struct tagflaglang *tfl) {
    int cnt, cnt2;
    int32 coverage_pos, end;
    PST *pst, *first=NULL;
    int bits = 0, same=true;

    for ( cnt=cnt2=0; glyphs[cnt]!=NULL; ++cnt) {
	for ( pst=glyphs[cnt]->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->tag==tfl->tag && pst->flags==tfl->flags &&
		    pst->script_lang_index == tfl->script_lang_index && pst->type==pst_position ) {
		if ( first==NULL ) first = pst;
		else if ( same ) {
		    if ( first->u.pos.xoff!=pst->u.pos.xoff ||
			    first->u.pos.yoff!=pst->u.pos.yoff ||
			    first->u.pos.h_adv_off!=pst->u.pos.h_adv_off ||
			    first->u.pos.v_adv_off!=pst->u.pos.v_adv_off )
			same = false;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		    if ( !ValDevTabsSame(pst->u.pos.adjust,first->u.pos.adjust))
			same = false;
#endif
		}
		if ( pst->u.pos.xoff!=0 ) bits |= 1;
		if ( pst->u.pos.yoff!=0 ) bits |= 2;
		if ( pst->u.pos.h_adv_off!=0 ) bits |= 4;
		if ( pst->u.pos.v_adv_off!=0 ) bits |= 8;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		if ( pst->u.pos.adjust!=NULL ) {
		    if ( pst->u.pos.adjust->xadjust.corrections!=NULL ) bits |= 0x10;
		    if ( pst->u.pos.adjust->yadjust.corrections!=NULL ) bits |= 0x20;
		    if ( pst->u.pos.adjust->xadv.corrections!=NULL ) bits |= 0x40;
		    if ( pst->u.pos.adjust->yadv.corrections!=NULL ) bits |= 0x80;
		}
#endif
		++cnt2;
	break;
	    }
	}
    }
    if ( bits==0 ) bits=1;
    if ( cnt!=cnt2 )
	IError( "Count mismatch in dumpGPOSsimplepos#1 %d vs %d\n", cnt, cnt2 );

    putshort(gpos,same?1:2);	/* 1 means all value records same */
    coverage_pos = ftell(gpos);
    putshort(gpos,0);		/* offset to coverage table */
    putshort(gpos,bits);
    if ( same ) {
	if ( bits&1 ) putshort(gpos,first->u.pos.xoff);
	if ( bits&2 ) putshort(gpos,first->u.pos.yoff);
	if ( bits&4 ) putshort(gpos,first->u.pos.h_adv_off);
	if ( bits&8 ) putshort(gpos,first->u.pos.v_adv_off);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( bits&0xf0 ) {
	    int next_dev_tab = ftell(gpos)-coverage_pos+2+
		sizeof(int16)*((bits&0x10?1:0) + (bits&0x20?1:0) + (bits&0x40?1:0) + (bits&0x80?1:0));
	    if ( bits&0x10 ) { putshort(gpos,next_dev_tab); next_dev_tab += DevTabLen(&first->u.pos.adjust->xadjust); }
	    if ( bits&0x20 ) { putshort(gpos,next_dev_tab); next_dev_tab += DevTabLen(&first->u.pos.adjust->yadjust); }
	    if ( bits&0x40 ) { putshort(gpos,next_dev_tab); next_dev_tab += DevTabLen(&first->u.pos.adjust->xadv); }
	    if ( bits&0x80 ) { putshort(gpos,next_dev_tab); next_dev_tab += DevTabLen(&first->u.pos.adjust->yadv); }
	    if ( bits&0x10 ) dumpgposdevicetable(gpos,&first->u.pos.adjust->xadjust);
	    if ( bits&0x20 ) dumpgposdevicetable(gpos,&first->u.pos.adjust->yadjust);
	    if ( bits&0x40 ) dumpgposdevicetable(gpos,&first->u.pos.adjust->xadv);
	    if ( bits&0x80 ) dumpgposdevicetable(gpos,&first->u.pos.adjust->yadv);
	    if ( next_dev_tab!=ftell(gpos)-coverage_pos+2 )
		IError( "Device Table offsets wrong in simple positioning 2");
	}
#endif
    } else {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	int vr_size = 
		sizeof(int16)*((bits&0x1?1:0) + (bits&0x2?1:0) + (bits&0x4?1:0) + (bits&0x8?1:0) +
		    (bits&0x10?1:0) + (bits&0x20?1:0) + (bits&0x40?1:0) + (bits&0x80?1:0));
	int next_dev_tab = ftell(gpos)-coverage_pos+2+2+vr_size*cnt;
#endif
	putshort(gpos,cnt);
	for ( cnt2 = 0; glyphs[cnt2]!=NULL; ++cnt2 ) {
	    for ( pst=glyphs[cnt2]->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->tag==tfl->tag && pst->flags==tfl->flags &&
			pst->script_lang_index == tfl->script_lang_index && pst->type==pst_position ) {
		    if ( bits&1 ) putshort(gpos,pst->u.pos.xoff);
		    if ( bits&2 ) putshort(gpos,pst->u.pos.yoff);
		    if ( bits&4 ) putshort(gpos,pst->u.pos.h_adv_off);
		    if ( bits&8 ) putshort(gpos,pst->u.pos.v_adv_off);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		    next_dev_tab = gposdumpvaldevtab(gpos,pst->u.pos.adjust,bits,
			    next_dev_tab);
#endif
	    break;
		}
	    }
	}
	if ( cnt!=cnt2 )
	    IError( "Count mismatch in dumpGPOSsimplepos#3 %d vs %d\n", cnt, cnt2 );
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( bits&0xf0 ) {
	    for ( cnt2 = 0; glyphs[cnt2]!=NULL; ++cnt2 ) {
		for ( pst=glyphs[cnt2]->possub; pst!=NULL; pst=pst->next ) {
		    if ( pst->tag==tfl->tag && pst->flags==tfl->flags &&
			    pst->script_lang_index == tfl->script_lang_index && pst->type==pst_position ) {
			if ( bits&0x10 ) dumpgposdevicetable(gpos,&first->u.pos.adjust->xadjust);
			if ( bits&0x20 ) dumpgposdevicetable(gpos,&first->u.pos.adjust->yadjust);
			if ( bits&0x40 ) dumpgposdevicetable(gpos,&first->u.pos.adjust->xadv);
			if ( bits&0x80 ) dumpgposdevicetable(gpos,&first->u.pos.adjust->yadv);
		    }
		}
	    }
	}
	if ( next_dev_tab!=ftell(gpos)-coverage_pos+2 )
	    IError( "Device Table offsets wrong in simple positioning 2");
#endif
    }
    end = ftell(gpos);
    fseek(gpos,coverage_pos,SEEK_SET);
    putshort(gpos,end-coverage_pos+2);
    fseek(gpos,end,SEEK_SET);
    dumpcoveragetable(gpos,glyphs);
    fseek(gpos,0,SEEK_END);
}


static void dumpGPOSpairpos(FILE *gpos,SplineFont *sf,SplineChar **glyphs,
	struct tagflaglang *tfl) {
    int cnt;
    int32 coverage_pos, offset_pos, end, start, pos;
    PST *pst;
    int vf1 = 0, vf2=0, i, subcnt;
    SplineChar *sc;

    for ( cnt=0; glyphs[cnt]!=NULL; ++cnt) {
	for ( pst=glyphs[cnt]->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->tag==tfl->tag && pst->flags==tfl->flags &&
		    pst->script_lang_index == tfl->script_lang_index &&
		    pst->type==pst_pair ) {
		if ( pst->u.pair.vr[0].xoff!=0 ) vf1 |= 1;
		if ( pst->u.pair.vr[0].yoff!=0 ) vf1 |= 2;
		if ( pst->u.pair.vr[0].h_adv_off!=0 ) vf1 |= 4;
		if ( pst->u.pair.vr[0].v_adv_off!=0 ) vf1 |= 8;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		if ( pst->u.pair.vr[0].adjust!=NULL ) {
		    if ( pst->u.pair.vr[0].adjust->xadjust.corrections!=NULL ) vf1 |= 0x10;
		    if ( pst->u.pair.vr[0].adjust->yadjust.corrections!=NULL ) vf1 |= 0x20;
		    if ( pst->u.pair.vr[0].adjust->xadv.corrections!=NULL ) vf1 |= 0x40;
		    if ( pst->u.pair.vr[0].adjust->yadv.corrections!=NULL ) vf1 |= 0x80;
		}
#endif
		if ( pst->u.pair.vr[1].xoff!=0 ) vf2 |= 1;
		if ( pst->u.pair.vr[1].yoff!=0 ) vf2 |= 2;
		if ( pst->u.pair.vr[1].h_adv_off!=0 ) vf2 |= 4;
		if ( pst->u.pair.vr[1].v_adv_off!=0 ) vf2 |= 8;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		if ( pst->u.pair.vr[1].adjust!=NULL ) {
		    if ( pst->u.pair.vr[1].adjust->xadjust.corrections!=NULL ) vf2 |= 0x10;
		    if ( pst->u.pair.vr[1].adjust->yadjust.corrections!=NULL ) vf2 |= 0x20;
		    if ( pst->u.pair.vr[1].adjust->xadv.corrections!=NULL ) vf2 |= 0x40;
		    if ( pst->u.pair.vr[1].adjust->yadv.corrections!=NULL ) vf2 |= 0x80;
		}
#endif
	    }
	}
    }
    if ( vf1==0 && vf2==0 ) vf1=1;

    start = ftell(gpos);
    putshort(gpos,1);		/* 1 means char pairs (ie. not classes) */
    coverage_pos = ftell(gpos);
    putshort(gpos,0);		/* offset to coverage table */
    putshort(gpos,vf1);
    putshort(gpos,vf2);
    putshort(gpos,cnt);
    offset_pos = ftell(gpos);
    for ( i=0; i<cnt; ++i )
	putshort(gpos,0);	/* Fill in later */
    for ( i=0; i<cnt; ++i ) {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	int next_dev_tab = ftell(gpos)-start;
	if ( (vf1&0xf0) || (vf2&0xf0) ) {
	    for ( pst=glyphs[i]->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->tag==tfl->tag && pst->flags==tfl->flags &&
			pst->script_lang_index == tfl->script_lang_index &&
			pst->type==pst_pair ) {
		    if ( pst->u.pair.vr[0].adjust!=NULL ) {
			dumpgposdevicetable(gpos,&pst->u.pair.vr[0].adjust->xadjust);
			dumpgposdevicetable(gpos,&pst->u.pair.vr[0].adjust->yadjust);
			dumpgposdevicetable(gpos,&pst->u.pair.vr[0].adjust->xadv);
			dumpgposdevicetable(gpos,&pst->u.pair.vr[0].adjust->yadv);
		    }
		    if ( pst->u.pair.vr[1].adjust!=NULL ) {
			dumpgposdevicetable(gpos,&pst->u.pair.vr[1].adjust->xadjust);
			dumpgposdevicetable(gpos,&pst->u.pair.vr[1].adjust->yadjust);
			dumpgposdevicetable(gpos,&pst->u.pair.vr[1].adjust->xadv);
			dumpgposdevicetable(gpos,&pst->u.pair.vr[1].adjust->yadv);
		    }
	    break;
		}
	    }
	}
#endif
	pos = ftell(gpos);
	fseek(gpos,offset_pos+i*sizeof(uint16),SEEK_SET);
	putshort(gpos,pos-start);
	fseek(gpos,pos,SEEK_SET);

	subcnt=0;
	for ( pst=glyphs[i]->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->tag==tfl->tag && pst->flags==tfl->flags &&
		    pst->script_lang_index == tfl->script_lang_index &&
		    pst->type==pst_pair )
		++subcnt;
	}
	putshort(gpos,subcnt);
	for ( pst=glyphs[i]->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->tag==tfl->tag && pst->flags==tfl->flags &&
		    pst->script_lang_index == tfl->script_lang_index &&
		    pst->type==pst_pair ) {
		sc = SFGetChar(sf,-1,pst->u.pair.paired);
		if ( sc==NULL || sc->ttf_glyph==-1 )
		    putshort(gpos,0);
		else
		    putshort(gpos,sc->ttf_glyph);
		if ( vf1&1 ) putshort(gpos,pst->u.pair.vr[0].xoff);
		if ( vf1&2 ) putshort(gpos,pst->u.pair.vr[0].yoff);
		if ( vf1&4 ) putshort(gpos,pst->u.pair.vr[0].h_adv_off);
		if ( vf1&8 ) putshort(gpos,pst->u.pair.vr[0].v_adv_off);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		next_dev_tab = gposdumpvaldevtab(gpos,pst->u.pair.vr[0].adjust,vf1,
			next_dev_tab);
#endif
		if ( vf2&1 ) putshort(gpos,pst->u.pair.vr[1].xoff);
		if ( vf2&2 ) putshort(gpos,pst->u.pair.vr[1].yoff);
		if ( vf2&4 ) putshort(gpos,pst->u.pair.vr[1].h_adv_off);
		if ( vf2&8 ) putshort(gpos,pst->u.pair.vr[1].v_adv_off);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		next_dev_tab = gposdumpvaldevtab(gpos,pst->u.pair.vr[1].adjust,vf2,
			next_dev_tab);
#endif
	    }
	}
    }
    end = ftell(gpos);
    fseek(gpos,coverage_pos,SEEK_SET);
    putshort(gpos,end-start);
    fseek(gpos,end,SEEK_SET);
    dumpcoveragetable(gpos,glyphs);
}

static void dumpgsubligdata(FILE *gsub,SplineFont *sf,
	struct tagflaglang *ligtag, struct alltabs *at) {
    int32 coverage_pos, next_val_pos, here, lig_list_start;
    int cnt, i, pcnt, lcnt, max=100, j;
    uint16 *offsets=NULL, *ligoffsets=galloc(max*sizeof(uint16));
    SplineChar **glyphs;
    LigList *ll;
    struct splinecharlist *scl;

    glyphs = generateGlyphList(sf,false,0,ligtag,&at->gi);
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
	    if ( ll->lig->tag==ligtag->tag && ll->lig->flags == ligtag->flags &&
		    ll->lig->script_lang_index == ligtag->script_lang_index &&
		    AllToBeOutput(ll))
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
	    if ( ll->lig->tag==ligtag->tag && ll->lig->flags == ligtag->flags &&
		    ll->lig->script_lang_index==ligtag->script_lang_index  &&
		    AllToBeOutput(ll)) {
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

static struct lookup *LookupFromTagFlagLang(struct tagflaglang *tfl ) {
    struct lookup *new;

    new = chunkalloc(sizeof(struct lookup));
    new->feature_tag = tfl->tag;
    new->flags = tfl->flags;
    new->script_lang_index = tfl->script_lang_index;
return( new );
}

static void TagFlagLangFromPST(struct tagflaglang *tfl,PST *pst) {
    tfl->script_lang_index = pst->script_lang_index;
    tfl->flags = pst->flags;
    tfl->tag = pst->tag;
}

void AnchorClassOrder(SplineFont *sf) {
    AnchorClass *ac, *p, *ac2, *n, *last;
    /* Order the AnchorClasses so that all those with the same:
	    tag
	    sli
	    merge field
	are together. These will be treated as one sub-table (well we might
	get 2 sub-tables if there are both ligature and normal base glyphs)
    */
    for ( ac=sf->anchor; ac!=NULL; ac = ac->next ) {
	last = ac;
	for ( p=ac, ac2 = ac->next; ac2!=NULL; ac2 = n ) {
	    n = ac2->next;
	    if ( ac2->feature_tag==ac->feature_tag &&
		    ac2->script_lang_index == ac->script_lang_index &&
		    ac2->merge_with == ac->merge_with &&
		    last!=p ) { 	/* if p==ac we don't need to do anything, already in place */
		ac2->next = last->next;
		last->next = ac2;
		p->next = n;
		last = ac2;
	    } else
		p = ac2;
	}
    }
}

static int AnchorHasMark(SplineFont *sf,AnchorClass *ac) {
    int i;
    AnchorPoint *ap;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	for ( ap=sf->glyphs[i]->anchor; ap!=NULL ; ap=ap->next ) {
	    if ( ap->anchor==ac && ap->type==at_mark )
return( true );
	}
    }
return( false );
}

static uint16 *FigureInitialClasses(FPST *fpst) {
    uint16 *initial = galloc((fpst->nccnt+1)*sizeof(uint16));
    int i, cnt, j;

    for ( i=cnt=0; i<fpst->rule_cnt; ++i ) {
	for ( j=0; j<cnt ; ++j )
	    if ( initial[j] == fpst->rules[i].u.class.nclasses[0] )
	break;
	if ( j==cnt )
	    initial[cnt++] = fpst->rules[i].u.class.nclasses[0];
    }
    initial[cnt] = 0xffff;
return( initial );
}

static SplineChar **OrderedInitialGlyphs(SplineFont *sf,FPST *fpst) {
    SplineChar **glyphs, *sc;
    int i, j, cnt, ch;
    char *pt, *names;

    glyphs = galloc((fpst->rule_cnt+1)*sizeof(SplineChar *));
    for ( i=cnt=0; i<fpst->rule_cnt; ++i ) {
	names = fpst->rules[i].u.glyph.names;
	pt = strchr(names,' ');
	if ( pt==NULL ) pt = names+strlen(names);
	ch = *pt; *pt = '\0';
	sc = SFGetChar(sf,-1,names);
	*pt = ch;
	for ( j=0; j<cnt; ++j )
	    if ( glyphs[j]==sc )
	break;
	if ( j==cnt && sc!=NULL )
	    glyphs[cnt++] = sc;
    }
    glyphs[cnt] = NULL;
    if ( cnt==0 )
return( glyphs );

    for ( i=0; glyphs[i+1]!=NULL; ++i ) for ( j=i+1; glyphs[j]!=NULL; ++j ) {
	if ( glyphs[i]->ttf_glyph > glyphs[j]->ttf_glyph ) {
	    sc = glyphs[i];
	    glyphs[i] = glyphs[j];
	    glyphs[j] = sc;
	}
    }
return( glyphs );
}

static int NamesStartWith(SplineChar *sc,char *names ) {
    char *pt;

    pt = strchr(names,' ');
    if ( pt==NULL ) pt = names+strlen(names);
    if ( pt-names!=strlen(sc->name))
return( false );

return( strncmp(sc->name,names,pt-names)==0 );
}

static int CntRulesStartingWith(FPST *fpst,SplineChar *sc) {
    int i, cnt;

    for ( i=cnt=0; i<fpst->rule_cnt; ++i ) {
	if ( NamesStartWith(sc,fpst->rules[i].u.glyph.names))
	    ++cnt;
    }
return( cnt );
}

static int CntRulesStartingWithClass(FPST *fpst,uint16 cval) {
    int i, cnt;

    for ( i=cnt=0; i<fpst->rule_cnt; ++i ) {
	if ( fpst->rules[i].u.class.nclasses[0]==cval )
	    ++cnt;
    }
return( cnt );
}

static int g___FigureNest(struct lookup **nested, struct postponedlookup **pp,
	struct alltabs *at, uint32 tag) {
    struct lookup *l;
    struct postponedlookup *p, *prev;

    for ( l=*nested; l!=NULL; l=l->next )
	if ( l->feature_tag == tag )
return( l->lookup_cnt );
    prev = NULL;
    for ( p = *pp; p!=NULL; p=p->next ) {
	if ( p->feature_tag == tag )
return( p->lookup_cnt );
	prev = p;
    }

    p = chunkalloc(sizeof(struct postponedlookup));
    p->feature_tag = tag;
    p->lookup_cnt = at->next_lookup++;
    if ( prev!=NULL )
	prev->next = p;
    else
	*pp = p;
return( p->lookup_cnt );
}

static void dumpg___ContextChainGlyphs(FILE *lfile,FPST *fpst,SplineFont *sf,
	struct lookup **nested, struct postponedlookup **pp, struct alltabs *at) {
    int iscontext = fpst->type==pst_contextpos || fpst->type==pst_contextsub;
    uint32 base = ftell(lfile);
    int i,cnt, subcnt, j,k,l, maxcontext,curcontext;
    SplineChar **glyphs, **subglyphs;
    uint8 *exists;
    int lc;
    int ispos = (fpst->type==pst_contextpos || fpst->type==pst_chainpos);

    glyphs = OrderedInitialGlyphs(sf,fpst);
    for ( cnt=0; glyphs[cnt]!=NULL; ++cnt );

    putshort(lfile,1);		/* Sub format 1 => glyph lists */
    putshort(lfile,(3+cnt)*sizeof(short));	/* offset to coverage */
    putshort(lfile,cnt);
    for ( i=0; i<cnt; ++i )
	putshort(lfile,0);	/* Offset to rule */
    dumpcoveragetable(lfile,glyphs);

    maxcontext = 0;

    for ( i=0; i<cnt; ++i ) {
	uint32 pos = ftell(lfile);
	fseek(lfile,base+(3+i)*sizeof(short),SEEK_SET);
	putshort(lfile,pos-base);
	fseek(lfile,pos,SEEK_SET);
	subcnt = CntRulesStartingWith(fpst,glyphs[i]);
	putshort(lfile,subcnt);
	for ( j=0; j<subcnt; ++j )
	    putshort(lfile,0);
	for ( j=k=0; k<fpst->rule_cnt; ++k ) if ( NamesStartWith(glyphs[i],fpst->rules[k].u.glyph.names )) {
	    uint32 subpos = ftell(lfile);
	    fseek(lfile,pos+(1+j)*sizeof(short),SEEK_SET);
	    putshort(lfile,subpos-pos);
	    fseek(lfile,subpos,SEEK_SET);

	    exists = galloc(fpst->rules[k].lookup_cnt*sizeof(uint8));
	    for ( l=lc=0; l<fpst->rules[k].lookup_cnt; ++l )
		lc += ( exists[l] = SFHasNestedLookupWithTag(sf,fpst->rules[k].lookups[l].lookup_tag,ispos));
	    if ( iscontext ) {
		subglyphs = SFGlyphsFromNames(sf,fpst->rules[k].u.glyph.names);
		for ( l=0; subglyphs[l]!=NULL; ++l );
		putshort(lfile,l);
		curcontext = l;
		putshort(lfile,lc);
		for ( l=1; subglyphs[l]!=NULL; ++l )
		    putshort(lfile,subglyphs[l]->ttf_glyph);
		free(subglyphs);
	    } else {
		subglyphs = SFGlyphsFromNames(sf,fpst->rules[k].u.glyph.back);
		for ( l=0; subglyphs[l]!=NULL; ++l );
		putshort(lfile,l);
		curcontext = l;
		for ( l=0; subglyphs[l]!=NULL; ++l )
		    putshort(lfile,subglyphs[l]->ttf_glyph);
		free(subglyphs);
		subglyphs = SFGlyphsFromNames(sf,fpst->rules[k].u.glyph.names);
		for ( l=0; subglyphs[l]!=NULL; ++l );
		putshort(lfile,l);
		curcontext += l;
		for ( l=1; subglyphs[l]!=NULL; ++l )
		    putshort(lfile,subglyphs[l]->ttf_glyph);
		free(subglyphs);
		subglyphs = SFGlyphsFromNames(sf,fpst->rules[k].u.glyph.fore);
		for ( l=0; subglyphs[l]!=NULL; ++l );
		putshort(lfile,l);
		curcontext += l;
		for ( l=0; subglyphs[l]!=NULL; ++l )
		    putshort(lfile,subglyphs[l]->ttf_glyph);
		free(subglyphs);
		putshort(lfile,lc);
	    }
	    for ( l=0; l<fpst->rules[k].lookup_cnt; ++l ) if ( exists[l] ) {
		putshort(lfile,fpst->rules[k].lookups[l].seq);
		putshort(lfile,g___FigureNest(nested,pp,at,fpst->rules[k].lookups[l].lookup_tag));
	    }
	    free(exists);
	    ++j;
	    if ( curcontext>maxcontext ) maxcontext = curcontext;
	}
    }
    free(glyphs);

    if ( maxcontext>at->os2.maxContext )
	at->os2.maxContext = maxcontext;
}

static void dumpg___ContextChainClass(FILE *lfile,FPST *fpst,SplineFont *sf,
	struct lookup **nested, struct postponedlookup **pp, struct alltabs *at) {
    int iscontext = fpst->type==pst_contextpos || fpst->type==pst_contextsub;
    uint32 base = ftell(lfile), rulebase, pos, subpos, npos;
    uint16 *initialclasses, *iclass, *bclass, *lclass;
    SplineChar **iglyphs, **bglyphs, **lglyphs, **glyphs;
    int i,ii,cnt, subcnt, j,k,l , maxcontext,curcontext;
    uint8 *exists;
    int lc;
    int ispos = (fpst->type==pst_contextpos || fpst->type==pst_chainpos);

    putshort(lfile,2);		/* Sub format 2 => class */
    putshort(lfile,0);		/* offset to coverage table */
    if ( iscontext )
	putshort(lfile,0);	/* offset to input classdef */
    else {
	putshort(lfile,0);	/* offset to backtrack classdef */
	putshort(lfile,0);	/* offset to input classdef */
	putshort(lfile,0);	/* offset to lookahead classdef */
    }
    initialclasses = FigureInitialClasses(fpst);
    putshort(lfile,fpst->nccnt);
    rulebase = ftell(lfile);
    for ( cnt=0; cnt<fpst->nccnt; ++cnt )
	putshort(lfile,0);

    iclass = ClassesFromNames(sf,fpst->nclass,fpst->nccnt,at->maxp.numGlyphs,&iglyphs,false);
    lglyphs = bglyphs = NULL; bclass = lclass = NULL;
    if ( !iscontext ) {
	bclass = ClassesFromNames(sf,fpst->bclass,fpst->bccnt,at->maxp.numGlyphs,&bglyphs,false);
	lclass = ClassesFromNames(sf,fpst->fclass,fpst->fccnt,at->maxp.numGlyphs,&lglyphs,false);
    }
    pos = ftell(lfile);
    fseek(lfile,base+sizeof(uint16),SEEK_SET);
    putshort(lfile,pos-base);
    fseek(lfile,pos,SEEK_SET);
    glyphs = GlyphsFromInitialClasses(iglyphs,at->maxp.numGlyphs,iclass,initialclasses);
    dumpcoveragetable(lfile,glyphs);
    free(glyphs);
    free(iglyphs); free(bglyphs); free(lglyphs);

    if ( iscontext ) {
	pos = ftell(lfile);
	fseek(lfile,base+2*sizeof(uint16),SEEK_SET);
	putshort(lfile,pos-base);
	fseek(lfile,pos,SEEK_SET);
	DumpClass(lfile,iclass,at->maxp.numGlyphs);
	free(iclass);
    } else {
	pos = ftell(lfile);
	fseek(lfile,base+2*sizeof(uint16),SEEK_SET);
	putshort(lfile,pos-base);
	fseek(lfile,pos,SEEK_SET);
	DumpClass(lfile,bclass,at->maxp.numGlyphs);
	if ( ClassesMatch(fpst->bccnt,fpst->bclass,fpst->nccnt,fpst->nclass)) {
	    npos = pos;
	    fseek(lfile,base+3*sizeof(uint16),SEEK_SET);
	    putshort(lfile,npos-base);
	    fseek(lfile,0,SEEK_END);
	} else {
	    npos = ftell(lfile);
	    fseek(lfile,base+3*sizeof(uint16),SEEK_SET);
	    putshort(lfile,npos-base);
	    fseek(lfile,npos,SEEK_SET);
	    DumpClass(lfile,iclass,at->maxp.numGlyphs);
	}
	if ( ClassesMatch(fpst->fccnt,fpst->fclass,fpst->bccnt,fpst->bclass)) {
	    fseek(lfile,base+4*sizeof(uint16),SEEK_SET);
	    putshort(lfile,pos-base);
	    fseek(lfile,0,SEEK_END);
	} else if ( ClassesMatch(fpst->fccnt,fpst->fclass,fpst->nccnt,fpst->nclass)) {
	    fseek(lfile,base+4*sizeof(uint16),SEEK_SET);
	    putshort(lfile,npos-base);
	    fseek(lfile,0,SEEK_END);
	} else {
	    pos = ftell(lfile);
	    fseek(lfile,base+4*sizeof(uint16),SEEK_SET);
	    putshort(lfile,pos-base);
	    fseek(lfile,pos,SEEK_SET);
	    DumpClass(lfile,lclass,at->maxp.numGlyphs);
	}
	free(iclass); free(bclass); free(lclass);
    }

    ii=0;
    for ( i=0; i<fpst->nccnt; ++i ) {
	if ( initialclasses[ii]!=i ) {
	    /* This class isn't an initial one, so leave it's rule pointer NULL */
	} else {
	    ++ii;
	    pos = ftell(lfile);
	    fseek(lfile,rulebase+i*sizeof(short),SEEK_SET);
	    putshort(lfile,pos-base);
	    fseek(lfile,pos,SEEK_SET);
	    subcnt = CntRulesStartingWithClass(fpst,i);
	    putshort(lfile,subcnt);
	    for ( j=0; j<subcnt; ++j )
		putshort(lfile,0);
	    for ( j=k=0; k<fpst->rule_cnt; ++k ) if ( i==fpst->rules[k].u.class.nclasses[0] ) {
		subpos = ftell(lfile);
		fseek(lfile,pos+(1+j)*sizeof(short),SEEK_SET);
		putshort(lfile,subpos-pos);
		fseek(lfile,subpos,SEEK_SET);

		exists = galloc(fpst->rules[k].lookup_cnt*sizeof(uint8));
		for ( l=lc=0; l<fpst->rules[k].lookup_cnt; ++l )
		    lc += ( exists[l] = SFHasNestedLookupWithTag(sf,fpst->rules[k].lookups[l].lookup_tag,ispos));
		if ( iscontext ) {
		    putshort(lfile,fpst->rules[k].u.class.ncnt);
		    putshort(lfile,lc);
		    for ( l=1; l<fpst->rules[k].u.class.ncnt; ++l )
			putshort(lfile,fpst->rules[k].u.class.nclasses[l]);
		} else {
		    putshort(lfile,fpst->rules[k].u.class.bcnt);
		    for ( l=0; l<fpst->rules[k].u.class.bcnt; ++l )
			putshort(lfile,fpst->rules[k].u.class.bclasses[l]);
		    putshort(lfile,fpst->rules[k].u.class.ncnt);
		    for ( l=1; l<fpst->rules[k].u.class.ncnt; ++l )
			putshort(lfile,fpst->rules[k].u.class.nclasses[l]);
		    putshort(lfile,fpst->rules[k].u.class.fcnt);
		    for ( l=0; l<fpst->rules[k].u.class.fcnt; ++l )
			putshort(lfile,fpst->rules[k].u.class.fclasses[l]);
		    putshort(lfile,lc);
		}
		for ( l=0; l<fpst->rules[k].lookup_cnt; ++l ) if ( exists[l] ) {
		    putshort(lfile,fpst->rules[k].lookups[l].seq);
		    putshort(lfile,g___FigureNest(nested,pp,at,fpst->rules[k].lookups[l].lookup_tag));
		}
		free(exists);
		++j;
	    }
	}
    }
    free(initialclasses);

    maxcontext = 0;
    for ( i=0; i<fpst->rule_cnt; ++i ) {
	curcontext = fpst->rules[i].u.class.ncnt+fpst->rules[i].u.class.bcnt+fpst->rules[i].u.class.fcnt;
	if ( curcontext>maxcontext ) maxcontext = curcontext;
    }
    if ( maxcontext>at->os2.maxContext )
	at->os2.maxContext = maxcontext;
}

static void dumpg___ContextChainCoverage(FILE *lfile,FPST *fpst,SplineFont *sf,
	struct lookup **nested, struct postponedlookup **pp, struct alltabs *at) {
    int iscontext = fpst->type==pst_contextpos || fpst->type==pst_contextsub;
    uint32 base = ftell(lfile), ibase, lbase, bbase;
    int i;
    SplineChar **glyphs;
    int curcontext;
    uint8 *exists;
    int lc;
    int ispos = (fpst->type==pst_contextpos || fpst->type==pst_chainpos);

    if ( fpst->rule_cnt!=1 )
	IError("Bad rule cnt in coverage context lookup");
    if ( fpst->format==pst_reversecoverage && fpst->rules[0].u.rcoverage.always1!=1 )
	IError("Bad input count in reverse coverage lookup" );

    putshort(lfile,3);		/* Sub format 3 => coverage */
    exists = galloc(fpst->rules[0].lookup_cnt*sizeof(uint8));
    for ( i=lc=0; i<fpst->rules[0].lookup_cnt; ++i )
	lc += ( exists[i] = SFHasNestedLookupWithTag(sf,fpst->rules[0].lookups[i].lookup_tag,ispos));
    if ( iscontext ) {
	putshort(lfile,fpst->rules[0].u.coverage.ncnt);
	putshort(lfile,lc);
	for ( i=0; i<fpst->rules[0].u.coverage.ncnt; ++i )
	    putshort(lfile,0);
	for ( i=0; i<fpst->rules[0].lookup_cnt; ++i ) if ( exists[i] ) {
	    putshort(lfile,fpst->rules[0].lookups[i].seq);
	    putshort(lfile,g___FigureNest(nested,pp,at,fpst->rules[0].lookups[i].lookup_tag));
	}
	for ( i=0; i<fpst->rules[0].u.coverage.ncnt; ++i ) {
	    uint32 pos = ftell(lfile);
	    fseek(lfile,base+6+2*i,SEEK_SET);
	    putshort(lfile,pos-base);
	    fseek(lfile,pos,SEEK_SET);
	    glyphs = OrderedGlyphsFromNames(sf,fpst->rules[0].u.coverage.ncovers[i]);
	    dumpcoveragetable(lfile,glyphs);
	    free(glyphs);
	}
    } else {
	if ( fpst->format==pst_reversecoverage ) {
	    ibase = ftell(lfile);
	    putshort(lfile,0);
	}
	putshort(lfile,fpst->rules[0].u.coverage.bcnt);
	bbase = ftell(lfile);
	for ( i=0; i<fpst->rules[0].u.coverage.bcnt; ++i )
	    putshort(lfile,0);
	if ( fpst->format==pst_coverage ) {
	    putshort(lfile,fpst->rules[0].u.coverage.ncnt);
	    ibase = ftell(lfile);
	    for ( i=0; i<fpst->rules[0].u.coverage.ncnt; ++i )
		putshort(lfile,0);
	}
	putshort(lfile,fpst->rules[0].u.coverage.fcnt);
	lbase = ftell(lfile);
	for ( i=0; i<fpst->rules[0].u.coverage.fcnt; ++i )
	    putshort(lfile,0);
	if ( fpst->format==pst_coverage ) {
	    putshort(lfile,lc);
	    for ( i=0; i<fpst->rules[0].lookup_cnt; ++i ) if ( exists[i] ) {
		putshort(lfile,fpst->rules[0].lookups[i].seq);
		putshort(lfile,g___FigureNest(nested,pp,at,fpst->rules[0].lookups[i].lookup_tag));
	    }
	} else {		/* reverse coverage */
	    glyphs = SFGlyphsFromNames(sf,fpst->rules[0].u.rcoverage.replacements);
	    for ( i=0; glyphs[i]!=0; ++i );
	    putshort(lfile,i);
	    for ( i=0; glyphs[i]!=0; ++i )
		putshort(lfile,glyphs[i]->ttf_glyph);
	}
	for ( i=0; i<fpst->rules[0].u.coverage.ncnt; ++i ) {
	    uint32 pos = ftell(lfile);
	    fseek(lfile,ibase+2*i,SEEK_SET);
	    putshort(lfile,pos-base);
	    fseek(lfile,pos,SEEK_SET);
	    glyphs = OrderedGlyphsFromNames(sf,fpst->rules[0].u.coverage.ncovers[i]);
	    dumpcoveragetable(lfile,glyphs);
	    free(glyphs);
	}
	for ( i=0; i<fpst->rules[0].u.coverage.bcnt; ++i ) {
	    uint32 pos = ftell(lfile);
	    fseek(lfile,bbase+2*i,SEEK_SET);
	    putshort(lfile,pos-base);
	    fseek(lfile,pos,SEEK_SET);
	    glyphs = OrderedGlyphsFromNames(sf,fpst->rules[0].u.coverage.bcovers[i]);
	    dumpcoveragetable(lfile,glyphs);
	    free(glyphs);
	}
	for ( i=0; i<fpst->rules[0].u.coverage.fcnt; ++i ) {
	    uint32 pos = ftell(lfile);
	    fseek(lfile,lbase+2*i,SEEK_SET);
	    putshort(lfile,pos-base);
	    fseek(lfile,pos,SEEK_SET);
	    glyphs = OrderedGlyphsFromNames(sf,fpst->rules[0].u.coverage.fcovers[i]);
	    dumpcoveragetable(lfile,glyphs);
	    free(glyphs);
	}
    }
    free(exists);

    curcontext = fpst->rules[0].u.coverage.ncnt+fpst->rules[0].u.coverage.bcnt+fpst->rules[0].u.coverage.fcnt;
    if ( curcontext>at->os2.maxContext )
	at->os2.maxContext = curcontext;
}

static void g___HandleNested(FILE *lfile,SplineFont *sf,int gpos,
	struct lookup **nested,struct postponedlookup *pp, struct alltabs *at);

static struct lookup *dumpg___ContextChain(FILE *lfile,FPST *fpst,SplineFont *sf,
	struct lookup *lookups,struct lookup **nested,struct alltabs *at) {
    struct lookup *new;
    struct postponedlookup *pp = NULL;

    new = chunkalloc(sizeof(struct lookup));
    new->feature_tag = fpst->tag;
    new->flags = fpst->flags;
    new->script_lang_index = fpst->script_lang_index;
    new->lookup_type =  fpst->type == pst_contextpos ? 7 :
			fpst->type == pst_chainpos ? 8 :
			fpst->type == pst_contextsub ? 5 :
			fpst->type == pst_chainsub ? 6 :
			/* fpst->type == pst_reversesub */ 8;
    new->offset[0] = ftell(lfile);
    new->next = lookups;
    lookups = new;

    switch ( fpst->format ) {
      case pst_glyphs:
	dumpg___ContextChainGlyphs(lfile,fpst,sf,nested,&pp,at);
      break;
      case pst_class:
	dumpg___ContextChainClass(lfile,fpst,sf,nested,&pp,at);
      break;
      case pst_coverage:
      case pst_reversecoverage:
	dumpg___ContextChainCoverage(lfile,fpst,sf,nested,&pp,at);
      break;
    }

    fseek(lfile,0,SEEK_END);
    new->len = ftell(lfile)-new->offset[0];

    g___HandleNested(lfile,sf,fpst->type==pst_contextpos || fpst->type==pst_chainpos,
	    nested,pp,at);
return( lookups );
}

static struct lookup *AnchorsAway(FILE *lfile,SplineFont *sf,struct lookup *lookups,
	uint32 tag, struct glyphinfo *gi ) {
    SplineChar **base, **lig, **mkmk;
    AnchorClass *ac, *ac2;
    SplineChar ***marks;
    int *subcnts;
    int cmax, classcnt;
    int i;

    marks = galloc((cmax=20)*sizeof(SplineChar **));
    subcnts = galloc(cmax*sizeof(int));

    for ( ac=sf->anchor; ac!=NULL; ac = ac->next ) {
	if ( !ac->processed &&
		((tag==0 && ac->script_lang_index!=SLI_NESTED) ||
		 (tag!=0 && ac->script_lang_index==SLI_NESTED && ac->feature_tag == tag))) {
	    if ( ac->type==act_curs )
		lookups = dumpgposCursiveAttach(lfile,ac,sf,lookups,gi);
	    else if ( ac->has_mark ) {
		ac->matches = ac->processed = true;
		for ( ac2 = ac->next, classcnt=1; ac2!=NULL ; ac2 = ac2->next ) {
		    if ( ac2->has_mark && !ac2->processed &&
			    ac2->feature_tag==ac->feature_tag &&
			    ac2->script_lang_index == ac->script_lang_index &&
			    ac2->merge_with == ac->merge_with ) {
			ac2->matches = true;
			ac2->processed = true;
			++classcnt;
		    } else
			ac2->matches = false;
		}
		if ( classcnt>cmax ) {
		    marks = grealloc(marks,(cmax=classcnt+10)*sizeof(SplineChar **));
		    subcnts = grealloc(subcnts,cmax*sizeof(int));
		}
		AnchorClassDecompose(sf,ac,classcnt,subcnts,marks,&base,&lig,&mkmk,gi);
		if ( marks[0]!=NULL && base!=NULL )
		    lookups = dumpgposAnchorData(lfile,ac,at_basechar,marks,base,lookups,classcnt,gi);
		if ( marks[0]!=NULL && lig!=NULL )
		    lookups = dumpgposAnchorData(lfile,ac,at_baselig,marks,lig,lookups,classcnt,gi);
		if ( marks[0]!=NULL && mkmk!=NULL )
		    lookups = dumpgposAnchorData(lfile,ac,at_basemark,marks,mkmk,lookups,classcnt,gi);
		for ( i=0; i<classcnt; ++i )
		    free(marks[i]);
		free(base);
		free(lig);
		free(mkmk);
	    }
	}
    }

    free(marks);
    free(subcnts);
return( lookups );
}

static void g___HandleNested(FILE *lfile,SplineFont *sf,int gpos,
	struct lookup **nested,struct postponedlookup *pp, struct alltabs *at) {
    /* for each pp, we need to find some opentype feature with the given tag */
    /*  name and SLI_NESTED, and we need to generate a lookup for it */
    FPST *fpst;
    AnchorClass *ac;
    struct postponedlookup *pnext;
    struct lookup *new;
    struct tagflaglang ligtags;
    LigList *ll;
    PST *pst;
    int i, j, start, end, type;
    unichar_t buf[6];
    SplineChar ***map, **glyphs;

    for ( ; pp!=NULL ; pp = pnext ) {
	pnext = pp->next;
	new = NULL;

	/* I don't think there is anything to be gained by nesting contexts */
	/*  but they are easy to support, so do so */
	for ( fpst=sf->possub; fpst!=NULL; fpst = fpst->next ) {
	    if ((( gpos && (fpst->type==pst_contextpos || fpst->type==pst_chainpos)) ||
		    (!gpos && (fpst->type==pst_contextsub || fpst->type==pst_chainsub || fpst->type==pst_reversesub ))) &&
		    fpst->script_lang_index==SLI_NESTED && fpst->tag==pp->feature_tag ) {
		new = dumpg___ContextChain(lfile,fpst,sf,NULL,nested,at);
	break;
	    }
	}
	/* I don't handle kerning, that could be done with pst_pair if needed */
	if ( new==NULL && gpos ) {
	    for ( ac=sf->anchor; ac!=NULL; ac=ac->next )
		if ( ac->feature_tag==pp->feature_tag && ac->script_lang_index==SLI_NESTED ) {
		    new = AnchorsAway(lfile,sf,NULL,pp->feature_tag,&at->gi);
	    break;
		}
	}
	if ( new==NULL && !gpos ) {
	    for ( i=0; i<sf->glyphcnt; i++ ) if ( sf->glyphs[i]!=NULL ) {
		for ( ll = sf->glyphs[i]->ligofme; ll!=NULL; ll=ll->next ) {
		    if ( ll->lig->script_lang_index==SLI_NESTED &&
			    ll->lig->tag == pp->feature_tag ) {
			ligtags.script_lang_index = SLI_NESTED;
			ligtags.flags = ll->lig->flags;
			ligtags.tag = pp->feature_tag;
			new = LookupFromTagFlagLang(&ligtags);
			new->lookup_type = 4;		/* Ligature */
			new->offset[0] = ftell(lfile);
			dumpgsubligdata(lfile,sf,&ligtags,at);
			new->len = ftell(lfile)-new->offset[0];
		break;
		    }
		}
		if ( new!=NULL )
	    break;
	    }
	}
	if ( new==NULL ) {
	    if ( gpos ) {
		start = pst_position;
		end = pst_pair;
	    } else {
		start = pst_substitution;
		end = pst_ligature;
	    }
	    for ( type=start; type<=end; ++type ) {
		for ( i=0; i<sf->glyphcnt; i++ ) if ( sf->glyphs[i]!=NULL ) {
		    for ( pst = sf->glyphs[i]->possub; pst!=NULL; pst=pst->next ) {
			if ( pst->type==type &&
				pst->script_lang_index == SLI_NESTED &&
			        pst->tag == pp->feature_tag ) {
			    ligtags.script_lang_index = SLI_NESTED;
			    ligtags.flags = pst->flags;
			    ligtags.tag = pp->feature_tag;
			    glyphs = generateGlyphTypeList(sf,type,&ligtags,&map,&at->gi);
			    if ( glyphs==NULL )
 goto failure;
			    new = LookupFromTagFlagLang(&ligtags);
			    new->offset[0] = ftell(lfile);
			    if ( type==pst_position ) {
				new->lookup_type = 1;
				dumpGPOSsimplepos(lfile,sf,glyphs,&ligtags);
			    } else if ( type==pst_pair ) {
				dumpGPOSpairpos(lfile,sf,glyphs,&ligtags);
				new->lookup_type = 2;
			    } else if ( type==pst_substitution ) {
				dumpGSUBsimplesubs(lfile,sf,glyphs,map);
				new->lookup_type = 1;
			    } else if ( type==pst_multiple ) {
				new->lookup_type = 2;
				dumpGSUBmultiplesubs(lfile,sf,glyphs,map);
			    } else if ( type==pst_alternate ) {
				new->lookup_type = 3;
				dumpGSUBmultiplesubs(lfile,sf,glyphs,map);
			    } else if ( type==pst_ligature ) {
				/* Already done */;
			    } else
				IError("Unknown PST type in GPOS/GSUB figure lookups" );
			    free(glyphs);
			    if ( map!=NULL ) for ( j=0; map[j]!=NULL ; ++j )
				free(map[j]);
			    free(map);
			    new->len = ftell(lfile)-new->offset[0];
		    break;
			}
		    }
		    if ( new!=NULL )
		break;
		}
		if ( new!=NULL )
	    break;
	    }
	}
 failure:
	buf[0] = pp->feature_tag>>24; buf[1] = (pp->feature_tag>>16)&0xff;
	buf[2] = (pp->feature_tag>>8)&0xff; buf[3] = pp->feature_tag&0xff;
	buf[4] = 0;
	if ( new!=NULL ) {
	    new->lookup_cnt = pp->lookup_cnt;
	    if ( new->next )
#if defined(FONTFORGE_CONFIG_GTK)
		gwwv_post_error(_("Multiple Lookups"),_("Multiple lookups were generated for nested anchor tag '%s', only one will be used"),buf);
#else
		gwwv_post_error(_("Multiple Lookups"),_("Multiple lookups were generated for nested anchor tag '%s', only one will be used"),buf);
#endif
	    new->next = *nested;
	    *nested = new;
	} else {
#if defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Missing Lookup"),_("A nested lookup with tag '%s' could not be found. The generated font will not be useable. Try Element->Find Problems"),buf);
#else
	    gwwv_post_error(_("Missing Lookup"),_("A nested lookup with tag '%s' could not be found. The generated font will not be useable. Try Element->Find Problems"),buf);
#endif
	}
	chunkfree(pp,sizeof(struct postponedlookup));
    }
}

static struct lookup *GPOSfigureLookups(FILE *lfile,SplineFont *sf,
	struct alltabs *at,struct lookup **nested) {
    /* When we find a feature, we split it out into various scripts */
    /*  dumping one lookup per script into the file */
    struct lookup *lookups = NULL, *new, *kchead, *kclast;
    int i, j, cnt, max, isv;
    SplineChar ***map, **glyphs;
    AnchorClass *ac;
    struct tagflaglang *ligtags;
    enum possub_type type;
    PST *pst;
    KernPair *kp;
    KernClass *kc;
    FPST *fpst;

    *nested = NULL;

    max = 30; cnt = 0;
    ligtags = galloc(max*sizeof(struct tagflaglang));

    for ( type = pst_position; type<=pst_pair; ++type ) {
	cnt = 0;
	for ( i=0; i<sf->glyphcnt; i++ ) if ( sf->glyphs[i]!=NULL ) {
	    for ( pst = sf->glyphs[i]->possub; pst!=NULL; pst=pst->next ) if ( pst->type==type && pst->script_lang_index!=SLI_NESTED ) {
		for ( j=0; j<cnt; ++j )
		    if ( ligtags[j].tag==pst->tag && pst->flags==ligtags[j].flags &&
			    pst->script_lang_index == ligtags[j].script_lang_index )
		break;
		if ( j==cnt ) {
		    if ( cnt>=max ) {
			max += 30;
			ligtags = grealloc(ligtags,max*sizeof(struct tagflaglang));
		    }
		    TagFlagLangFromPST(&ligtags[cnt++],pst);
		}
	    }
	}

	/* Look for positions matching these tags */
	for ( j=0; j<cnt; ++j ) {
	    glyphs = generateGlyphTypeList(sf,type,&ligtags[j],&map,&at->gi);
	    if ( glyphs!=NULL && glyphs[0]!=NULL ) {
		new = LookupFromTagFlagLang(&ligtags[j]);
		new->offset[0] = ftell(lfile);
		new->next = lookups;
		lookups = new;
		if ( type==pst_position ) {
		    new->lookup_type = 1;
		    dumpGPOSsimplepos(lfile,sf,glyphs,&ligtags[j]);
		} else if ( type==pst_pair ) {
		    dumpGPOSpairpos(lfile,sf,glyphs,&ligtags[j]);
		    new->lookup_type = 2;
		} else
		    IError("Unknown PST type in GPOS figure lookups" );
		new->len = ftell(lfile)-new->offset[0];
		free(glyphs);
	    }
	}
    }

    for ( isv=0; isv<2; ++isv ) {
	/* If we dump kern classes out first here, then kern pairs will end up*/
	/*  first in the lookup. Which is where we want them so they can over-*/
	/*  ride the class default */
	/* The order of the kern classes matters too */
	kchead = kclast = NULL;
	for ( kc=isv ? sf->vkerns : sf->kerns; kc!=NULL; kc=kc->next ) {
	    new = chunkalloc(sizeof(struct lookup));
	    new->feature_tag = isv ? CHR('v','k','r','n') : CHR('k','e','r','n');
	    new->flags = kc->flags;
	    new->script_lang_index = kc->sli;
	    new->lookup_type = 2;		/* Pair adjustment subtable type */
	    new->offset[0] = ftell(lfile);
	    if ( kchead==NULL )
		kchead = new;
	    else
		kclast->next = new;
	    kclast = new;
	    dumpgposkernclass(lfile,sf,kc,at,isv);
	    new->len = ftell(lfile)-new->offset[0];
	}
	if ( kclast!=NULL ) {
	    kclast->next = lookups;
	    lookups = kchead;
	}

	/* Look for kerns */ /* kerns now store langs but not flags */
	cnt = 0;
	for ( i=0; i<sf->glyphcnt; i++ ) if ( sf->glyphs[i]!=NULL ) {
	    for ( kp = isv ? sf->glyphs[i]->vkerns : sf->glyphs[i]->kerns; kp!=NULL; kp=kp->next ) {
		for ( j=0; j<cnt; ++j )
		    if ( kp->sli == ligtags[j].script_lang_index )
		break;
		if ( j==cnt ) {
		    if ( cnt>=max ) {
			max += 30;
			ligtags = grealloc(ligtags,max*sizeof(struct tagflaglang));
		    }
		    ligtags[cnt++].script_lang_index = kp->sli;
		}
	    }
	}
	for ( j=0; j<cnt; ++j ) {
	    lookups = dumpgposkernpairs(lfile,sf,ligtags[j].script_lang_index,at,isv,lookups);
	}
    }
    free(ligtags);

    if ( sf->anchor ) {
	AnchorClassOrder(sf);	/* Don't really need this any more */
	for ( ac=sf->anchor; ac!=NULL; ac = ac->next ) {
	    ac->processed = false;
	    if ( ac->type!=act_curs )
		ac->has_mark = AnchorHasMark(sf,ac);
	}
	lookups = AnchorsAway(lfile,sf,lookups,0,&at->gi);
	AnchorGuessContext(sf,at);
    }

    for ( fpst=sf->possub; fpst!=NULL; fpst=fpst->next ) if ( fpst->script_lang_index!=SLI_NESTED )
	if ( fpst->type==pst_contextpos || fpst->type==pst_chainpos )
	    lookups = dumpg___ContextChain(lfile,fpst,sf,lookups,nested,at);

return( lookups );
}

static struct lookup *GSUBfigureLookups(FILE *lfile,SplineFont *sf,
	struct alltabs *at, struct lookup **nested) {
    struct lookup *lookups = NULL, *new;
    struct tagflaglang *ligtags;
    int i, j, max, cnt;
    LigList *ll;
    PST *subs;
    enum possub_type type;
    SplineChar **glyphs, ***map;
    FPST *fpst;

    *nested = NULL;

    /* Look for ligature tags used in the font */
    max = 30; cnt = 0;
    ligtags = galloc(max*sizeof(struct tagflaglang));
    for ( i=0; i<sf->glyphcnt; i++ ) if ( SCWorthOutputting(sf->glyphs[i]) ) {
	for ( ll = sf->glyphs[i]->ligofme; ll!=NULL; ll=ll->next ) {
	    if ( !ll->lig->macfeature &&
		    ll->lig->script_lang_index!=SLI_NESTED &&
		    SCWorthOutputting(ll->lig->u.lig.lig) ) {
		for ( j=0; j<cnt; ++j )
		    if ( ligtags[j].tag==ll->lig->tag && ll->lig->flags==ligtags[j].flags &&
			    ll->lig->script_lang_index == ligtags[j].script_lang_index )
		break;
		if ( j==cnt ) {
		    if ( cnt>=max ) {
			max += 30;
			ligtags = grealloc(ligtags,max*sizeof(struct tagflaglang));
		    }
		    TagFlagLangFromPST(&ligtags[cnt++],ll->lig);
		}
	    }
	}
    }

    /* Look for ligatures matching these tags */
    for ( j=0; j<cnt; ++j ) {
	new = LookupFromTagFlagLang(&ligtags[j]);
	new->lookup_type = 4;		/* Ligature */
	new->offset[0] = ftell(lfile);
	new->next = lookups;
	lookups = new;
	dumpgsubligdata(lfile,sf,&ligtags[j],at);
	new->len = ftell(lfile)-new->offset[0];
    }

    /* Now do something very similar for substitution simple, mult & alt tags */
    for ( type = pst_substitution; type<=pst_multiple; ++type ) {
	cnt = 0;
	for ( i=0; i<sf->glyphcnt; i++ ) if ( sf->glyphs[i]!=NULL ) {
	    for ( subs = sf->glyphs[i]->possub; subs!=NULL; subs=subs->next ) if ( subs->type==type && !subs->macfeature && subs->script_lang_index!=SLI_NESTED ) {
		for ( j=0; j<cnt; ++j )
		    if ( ligtags[j].tag==subs->tag && subs->flags==ligtags[j].flags &&
			    subs->script_lang_index == ligtags[j].script_lang_index )
		break;
		if ( j==cnt ) {
		    if ( cnt>=max ) {
			max += 30;
			ligtags = grealloc(ligtags,max*sizeof(struct tagflaglang));
		    }
		    TagFlagLangFromPST(&ligtags[cnt++],subs);
		}
	    }
	}

	/* Look for substitutions/decompositions matching these tags */
	for ( j=0; j<cnt; ++j ) {
	    glyphs = generateGlyphTypeList(sf,type,&ligtags[j],&map,&at->gi);
	    if ( glyphs!=NULL && glyphs[0]!=NULL ) {
		new = LookupFromTagFlagLang(&ligtags[j]);
		new->lookup_type = type==pst_substitution?1:type==pst_multiple?2:3;
		new->offset[0] = ftell(lfile);
		new->next = lookups;
		lookups = new;
		if ( type==pst_substitution )
		    dumpGSUBsimplesubs(lfile,sf,glyphs,map);
		else /* the format for multiple and alternate subs is the same. Only the semantics differ */
		    dumpGSUBmultiplesubs(lfile,sf,glyphs,map);
		free(glyphs);
		GlyphMapFree(map);
		new->len = ftell(lfile)-new->offset[0];
	    }
	}
    }
    free(ligtags);

    for ( fpst=sf->possub; fpst!=NULL; fpst=fpst->next ) if ( fpst->script_lang_index!=SLI_NESTED )
	if ( fpst->type==pst_contextsub || fpst->type==pst_chainsub || fpst->type==pst_reversesub )
	    lookups = dumpg___ContextChain(lfile,fpst,sf,lookups,nested,at);

return( lookups );
}

int TTFFeatureIndex( uint32 tag, struct table_ordering *ord, int isgsub ) {
    /* This is the order in which features should be executed */

    if ( ord!=NULL ) {
	int i;
	for ( i=0; ord->ordered_features[i]!=0; ++i )
	    if ( ord->ordered_features[i]==tag )
	break;
return( i );
    }

    if ( isgsub ) switch ( tag ) {
/* GSUB ordering */
      case CHR('c','c','m','p'):	/* Must be first? */
return( -2 );
      case CHR('l','o','c','l'):	/* Language dependent letter forms (serbian uses some different glyphs than russian) */
return( -1 );
      case CHR('i','s','o','l'):
return( 0 );
      case CHR('j','a','l','t'):		/* must come after 'isol' */
return( 1 );
      case CHR('f','i','n','a'):
return( 2 );
      case CHR('f','i','n','2'):
      case CHR('f','a','l','t'):		/* must come after 'fina' */
return( 3 );
      case CHR('f','i','n','3'):
return( 4 );
      case CHR('m','e','d','i'):
return( 5 );
      case CHR('m','e','d','2'):
return( 6 );
      case CHR('i','n','i','t'):
return( 7 );

      case CHR('r','t','l','a'):
return( 100 );
      case CHR('s','m','c','p'): case CHR('c','2','s','c'):
return( 200 );

      case CHR('r','l','i','g'):
return( 300 );
      case CHR('c','a','l','t'):
return( 301 );
      case CHR('l','i','g','a'):
return( 302 );
      case CHR('d','l','i','g'): case CHR('h','l','i','g'):
return( 303 );
      case CHR('c','s','w','h'):
return( 304 );
      case CHR('m','s','e','t'):
return( 305 );

      case CHR('f','r','a','c'):
return( 306 );

/* Indic processing */
      case CHR('n','u','k','t'):
      case CHR('p','r','e','f'):
return( 301 );
      case CHR('a','k','h','n'):
return( 302 );
      case CHR('r','p','h','f'):
return( 303 );
      case CHR('b','l','w','f'):
return( 304 );
      case CHR('h','a','l','f'):
      case CHR('a','b','v','f'):
return( 305 );
      case CHR('p','s','t','f'):
return( 306 );
      case CHR('v','a','t','u'):
return( 307 );

      case CHR('p','r','e','s'):
return( 310 );
      case CHR('b','l','w','s'):
return( 311 );
      case CHR('a','b','v','s'):
return( 312 );
      case CHR('p','s','t','s'):
return( 313 );
      case CHR('c','l','i','g'):
return( 314 );
      
      case CHR('h','a','l','n'):
return( 320 );
/* end indic ordering */

      case CHR('a','f','r','c'):
      case CHR('l','j','m','o'):
      case CHR('v','j','m','o'):
return( 350 );
      case CHR('v','r','t','2'): case CHR('v','e','r','t'):
return( 1010 );		/* Documented to come last */

/* Unknown things come after everything but vert/vrt2 */
      default:
return( 1000 );

    } else switch ( tag ) {
/* GPOS ordering */
      case CHR('c','u','r','s'):
return( 0 );
      case CHR('d','i','s','t'):
return( 100 );
      case CHR('b','l','w','m'):
return( 201 );
      case CHR('a','b','v','m'):
return( 202 );
      case CHR('k','e','r','n'):
return( 300 );
      case CHR('m','a','r','k'):
return( 400 );
      case CHR('m','k','m','k'):
return( 500 );
/* Unknown things come after everything  */
      default:
return( 1000 );
    }
}

static struct lookup *reverse_list(struct lookup *lookups) {
    struct lookup *next, *prev=NULL;

    if ( lookups==NULL )
return( NULL );

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

static struct lookup *order_nested(struct lookup *nested ) {
    int cnt;
    struct lookup **array, *n;
    int i,j;

    if ( nested==NULL || nested->next==NULL )
return( nested );
    for ( n=nested, cnt=0; n!=NULL; n=n->next, ++cnt );
    array = galloc(cnt*sizeof(struct lookup *));
    for ( n=nested, cnt=0; n!=NULL; n=n->next)
	array[cnt++] = n;
    for ( i=0; i<cnt-1; ++i ) for ( j=i+1; j<cnt; ++j ) {
	if ( array[i]->lookup_cnt>array[j]->lookup_cnt ) {
	    n = array[i];
	    array[i] = array[j];
	    array[j] = n;
	}
    }
    for ( i=0; i<cnt-1; ++i )
	array[i]->next = array[i+1];
    array[cnt-1]->next = NULL;
    nested = array[0];
    free(array );
return( nested );
}

static struct lookup *orderlookups(struct lookup **_lookups,
	struct table_ordering *ord, FILE *ordered, FILE *disordered,
	struct lookup *nested,int isgsub) {
    int cnt,i,j;
    struct lookup **array, *l, *features, *temp;
    struct lookup *lookups = *_lookups;
    char *buf;
    int bsize, len, totlen;

    if ( lookups==NULL )
return( NULL );

    for ( l=lookups, cnt=0; l!=NULL; l=l->next ) cnt++;
    array = galloc(cnt*sizeof(struct lookup *));
    for ( l=lookups, cnt=0; l!=NULL; l=l->next )
	array[cnt++] = l;

    /* sort by feature execution order */
    for ( i=0; i<cnt-1; ++i ) for ( j=i+1; j<cnt; ++j ) {
	if ( TTFFeatureIndex(array[i]->feature_tag,ord,isgsub)>TTFFeatureIndex(array[j]->feature_tag,ord,isgsub)) {
	    temp = array[i];
	    array[i] = array[j];
	    array[j] = temp;
	}
    }
    for ( i=0; i<cnt-1; ++i ) {
	array[i]->next = array[i+1];
	array[i]->lookup_cnt = -1;
    }
    array[i]->next = NULL;
    array[i]->lookup_cnt = -1;
    *_lookups = array[0];
    /* Now reorder the lookup file so that it's also in execution order */
    bsize = 16*1024;
    buf = galloc(bsize);
    for ( i=0; i<cnt; ++i ) {
	fseek(disordered,array[i]->offset[0],SEEK_SET);
	array[i]->offset[0] = ftell(ordered);
	totlen = array[i]->len;
	while ( totlen>0 ) {
	    if ( (len = totlen)>bsize ) len = bsize;
	    len = fread(buf,1,len,disordered);
	    fwrite(buf,1,len,ordered);
	    totlen -= len;
	}
    }
    for ( l = nested; l!=NULL; l=l->next ) {
	fseek(disordered,l->offset[0],SEEK_SET);
	l->offset[0] = ftell(ordered);
	totlen = l->len;
	while ( totlen>0 ) {
	    if ( (len = totlen)>bsize ) len = bsize;
	    len = fread(buf,1,len,disordered);
	    fwrite(buf,1,len,ordered);
	    totlen -= len;
	}
    }
    fclose(disordered);
    free(buf);

    /* sort by feature */
    for ( i=0; i<cnt-1; ++i ) for ( j=i+1; j<cnt; ++j ) {
	if ( array[i]->feature_tag > array[j]->feature_tag ||
		(array[i]->feature_tag==array[j]->feature_tag )) {
	    temp = array[i];
	    array[i] = array[j];
	    array[j] = temp;
	}
    }
    for ( i=0; i<cnt-1; ++i ) {
	array[i]->feature_next = array[i+1];
	/*array[i]->feature_cnt = i;*/
    }
    /*array[i]->feature_cnt = i;*/
    features = array[0];

    free(array);
return( features );
}

static int FeatureScriptLangMatch(struct feature *l,uint32 script,uint32 lang) {
    struct sl *sl;

    if ( l->is_size )			/* Size feature should be referenced by each script/lang combo */
return( true );				/* I guess */

    for ( sl = l->sl; sl!=NULL; sl = sl->next )
	if ( sl->script == script && sl->lang == lang )
return( true );

return( false );
}

static void dump_script_table(FILE *g___,SplineFont *sf,
	struct feature *features,
	uint32 script, int sc, uint8 *used,
	uint32 *langs, int lc, uint8 *touched) {
    struct feature *f;
    int cnt, dflt_index= -1, dflt_equiv= -1, total, offset;
    int i,j,k,m;
    int req_pos;
    uint32 here,base;

    memset(touched,0,lc);
    if ( sf->cidmaster ) sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;
    for ( i=0; sf->script_lang[i]!=NULL; ++i ) if ( used[i] ) {
	for ( j=0; sf->script_lang[i][j].script!=0 &&
		sf->script_lang[i][j].script!=script; ++j );
	if ( sf->script_lang[i][j].script!=0 ) {
	    for ( k=0; sf->script_lang[i][j].langs[k]!=0; ++k ) {
		for ( m=0; langs[m]!=0 && langs[m]!=sf->script_lang[i][j].langs[k]; ++m );
		++touched[m];
	    }
	}
    }
    for ( k=total=i=0; i<lc; ++i ) {
	if ( touched[i] ) ++k;
	total += touched[i];
    }
    for ( dflt_index = lc-1; dflt_index>=0 && langs[dflt_index]!=DEFAULT_LANG; --dflt_index );
    if ( dflt_index!=-1 && !touched[dflt_index] ) dflt_index = -1;
    if ( dflt_index!=-1 ) {
	--k;
	total -= touched[dflt_index];
    }

/* Microsoft makes some weird restrictions on the format of GSUB as described */
/*  http://www.microsoft.com/typography/tt/internat/fettspec.doc	      */
/* (at least for vertical writing). Patch from KANOU.			      */
    /* search a equivalent langsys entry to a default one */
    if ( dflt_index!=-1 ) {
       for ( i=j=0; i<lc; ++i ) if ( touched[i] && i!=dflt_index ) {
	   for (f=features; 
		f!=NULL && FeatureScriptLangMatch(f,script,langs[i]) == 
		       FeatureScriptLangMatch(f,script,DEFAULT_LANG);
		f=f->feature_next );
	   if (f==NULL) {
	       dflt_equiv=j;
	       break;
	   }
	   j++;
       }
    }

    /* Dump the script table */
    base = ftell(g___);
    /* In the case of a dump G??? script entry (put in to make Uniscribe */
    /*  happy) Volt adds a dummy default language entry too */
    if ( dflt_index==-1 && k==0 )
	dflt_index = 0;
    putshort(g___,dflt_index==-1?0:4+k*6);/* offset from start of script table */
				/* to data for default language */
    putshort(g___,k);		/* count of all non-default languages */
    /* Now the lang sys records of non-default languages */
    for ( i=0; i<lc; ++i ) if ( touched[i] && i!=dflt_index ) {
	putlong(g___,langs[i]);
	putshort(g___,0);		/* Fixup later */
    }
    /* Now the language system table for default */
    if ( dflt_index!=-1 && dflt_equiv==-1 ) {
	putshort(g___,0);		/* reserved, must be zero */
	req_pos = 0xffff;
	for ( f=features, cnt=0; f!=NULL ; f=f->feature_next ) {
	    if ( FeatureScriptLangMatch(f,script,DEFAULT_LANG)) {
		if ( f->feature_tag==REQUIRED_FEATURE )
		    req_pos = f->feature_cnt;
		else
		    ++cnt;
	    }
	}
	putshort(g___,req_pos);		/* No required feature */
	putshort(g___,cnt);		/* Number of features */
	for ( f=features; f!=NULL ; f=f->feature_next ) {
	    if ( FeatureScriptLangMatch(f,script,DEFAULT_LANG))
		if ( f->feature_tag!=REQUIRED_FEATURE )
		    putshort(g___,f->feature_cnt);	/* Index of each feature */
	}
    }
    /* Now the language system table for each language */
    offset = 2*2+4;
    for ( i=0; i<lc; ++i ) if ( touched[i] && i!=dflt_index ) {
	here = ftell(g___);
	fseek(g___,base+offset,SEEK_SET);
	putshort(g___,here-base);
	if ( dflt_equiv==i ) {
	    fseek(g___,base,SEEK_SET);
	    putshort(g___,here-base);
	}
	fseek(g___,here,SEEK_SET);
	offset += 6;
	putshort(g___,0);		/* reserved, must be zero */
	req_pos = 0xffff;
	for ( f=features, cnt=0; f!=NULL ; f=f->feature_next ) {
	    if ( FeatureScriptLangMatch(f,script,langs[i])) {
		if ( f->feature_tag==REQUIRED_FEATURE )
		    req_pos = f->feature_cnt;
		else
		    ++cnt;
	    }
	}
	putshort(g___,req_pos);		/* No required feature */
	putshort(g___,cnt);		/* Number of features */
	for ( f=features; f!=NULL ; f=f->feature_next ) {
	    if ( FeatureScriptLangMatch(f,script,langs[i]))
		if ( f->feature_tag!=REQUIRED_FEATURE )
		    putshort(g___,f->feature_cnt);	/* Index of each feature */
	}
    }
}
    
static FILE *g___FigureExtensionSubTables(struct clookup *clookups,struct lookup *lookups,int startoffset,int is_gpos) {
    struct clookup *cl;
    struct lookup *l;
    int cnt, gotmore;
    FILE *efile;
    int i, offset;
    int any= false;

    if ( clookups==NULL )
return( NULL );
    gotmore = true; cnt=0;
    while ( gotmore ) {
	gotmore = false;
	offset = startoffset + 8*cnt;
	for ( cl=clookups; cl!=NULL; cl=cl->next ) if ( !cl->duplicate && !cl->extension ) {
	    for ( i=cl->lcnt-1; i>=0; --i )
		if ( offset+cl->lookups[i]->offset[0]>65535 )
	    break;
	    if ( i>=0 ) {
		if ( !any ) {
		    gwwv_post_notice(_("Lookup potentially too big"),
			    _("One of the lookups for the '%c%c%c%c' feature has an\noffset bigger than 65535 bytes. This means\nFontForge must use an extension lookup to output it.\nNot all applications support extension lookups."),
			    (cl->lookups[i]->feature_tag>>24)&0xff,
			    (cl->lookups[i]->feature_tag>>16)&0xff,
			    (cl->lookups[i]->feature_tag>>8 )&0xff,
			    (cl->lookups[i]->feature_tag    )&0xff );
		    any = true;
		}
		cl->extension = true;
		cl->lookup_type = is_gpos ? 9 : 7;
		for ( i=cl->lcnt-1; i>=0; --i ) if ( !cl->lookups[i]->already_extended ) {
		    ++cnt;
		    cl->lookups[i]->already_extended = true;
		    gotmore = true;
		}
	    }
	    offset -= 6+2*cl->lcnt;
	}
    }

    if ( cnt==0 )		/* No offset overflows */
return( NULL );

    /* Now we've worked out which lookups need extension tables and marked them*/
    /* Generate the extension tables, and update the offsets to reflect the size */
    /* of the extensions */
    efile = tmpfile();
    for ( i=0, l=lookups; l!=NULL; l=l->next ) {
	l->offset[0] += 8*cnt;
	if ( l->already_extended ) {
	    putshort(efile,1);	/* exten subtable format (there's only one) */
	    putshort(efile,l->lookup_type);
	    putlong(efile,l->offset[0]-i*8);
	    l->offset[1] = i*8;
	    ++i;
	}
    }
return( efile );
}

static int CountLangs(SplineFont *sf,int sli) {
    int lcnt, l, s;
    struct script_record *sr;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;

    sr = sf->script_lang[sli];
    lcnt = 0;
    for ( s=0; sr[s].script!=0; ++s )
	for ( l=0; sr[s].langs[l]!=0; ++l )
	    ++lcnt;
return( lcnt );
}

struct slusage {
    uint32 script;
    uint32 lang;
    int processed;
    union {
	uint32 used;
	uint32 *usedarr;
    } u;
};

static int FindSL(struct slusage *slu,int lcnt,uint32 script,uint32 lang) {
    int i;

    for ( i=lcnt-1; i>=0; --i )
	if ( slu[i].script==script && slu[i].lang == lang )
    break;
return( i );
}

/* Find all slus that have the same usage as that at this, and add their */
/*  script/langs to the feature */
static void AddScriptLangsToFeature(struct feature *f,struct slusage *slu,
	int this,int lcnt,int cnt ) {
    int pos, match, i;
    struct sl *sl;

    f->sl = NULL;
    for ( pos=0; pos<lcnt; ++pos ) {
	if ( cnt<=32 )
	    match = slu[this].u.used == slu[pos].u.used;
	else {
	    match = true;
	    for ( i=((cnt+31)>>5)-1 ; i>=0 ; --i )
		if ( slu[this].u.usedarr[i] != slu[pos].u.usedarr[i] ) {
		    match = false;
	    break;
		}
	}
	if ( match ) {
	    sl = chunkalloc(sizeof(struct sl));
	    sl->next = f->sl;
	    f->sl = sl;
	    sl->script = slu[pos].script;
	    sl->lang = slu[pos].lang;
	    slu[pos].processed = true;
	}
    }
    if ( f->sl==NULL )
	IError("Impossible script lang in AddScriptLangsToFeature" );
}

static void AddSliToFeature(struct feature *f,SplineFont *sf,int sli) {
    int s, l;
    struct sl *sl;
    struct script_record *sr;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;

    sr = sf->script_lang[sli];
    f->sl = NULL;
    for ( s=0; sr[s].script!=0; ++s ) for ( l=0; sr[s].langs[l]!=0; ++l ) {
	sl = chunkalloc(sizeof(struct sl));
	sl->next = f->sl;
	f->sl = sl;
	sl->script = sr[s].script;
	sl->lang = sr[s].langs[l];
    }
    if ( f->sl==NULL )
	IError("Impossible script lang in AddSliToFeature" );
}

static struct feature *CreateSizeFeature(SplineFont *sf,int fcnt) {
    /* The GPOS 'size' feature is like no other. */
    /* It has no lookups */
    /* it has parameter data */
    /* it is associated with every script/lang combo. */
    struct feature *f;

    f = chunkalloc(sizeof(struct feature));
    f->feature_tag = CHR('s','i','z','e');
    f->feature_cnt = fcnt++;
    f->is_size = true;
return( f );
}

/* Now we have a set of lookups. Each of which is used by some colection of */
/*  scripts and languages. We need to turn this around so that we know for */
/*  each script what lookups it uses, and we must also merge all lookups of */
/*  a given tag into one feature per script/lang. Of course some lookup */
/*  combinations may be shared by several script/lang settings and we should */
/*  use the same feature when possible */
/* It gets worse. ATM does not seem to deal with kern features containing */
/*  more than one lookup, so we need to merge multiple sub-tables into one */
/*  lookup if they have the same feature tag/lookup flags */
/* Alexej suggests requiring the same sli too */
static struct feature *CoalesceLookups(SplineFont *sf, struct lookup *lookups,
	struct lookup *nested, struct clookup **clookups, int is_gpos,
	struct table_ordering *ord) {
    struct lookup *l, *last;
    int cnt, lcnt, allsame;
    struct slusage *slu=NULL;
    int slumax=0;
    struct feature *fhead = NULL, *flast=NULL, *f;
    int lang, s, pos, which, tot;
    int fcnt = 0;
    int added_size = false;
    struct clookup *clhead, *cllast, *cl, *cl2, *clflast, *clendnest, *clstart, **array;
    struct lookup **temp;
    int clcnt, clfcnt, clcntnest;
    int lc_warned;
    int i,j;

    for ( ; lookups!=NULL; lookups = last->feature_next ) {
	cnt = lcnt = 0;
	allsame = true;
	if ( !added_size && is_gpos && CHR('s','i','z','e')<lookups->feature_tag &&
		sf->design_size!=0 ) {
	    f = CreateSizeFeature(sf,fcnt++);
	    if ( flast==NULL )
		fhead = f;
	    else
		flast->feature_next = f;
	    flast = f;
	    added_size = true;
	}
	for ( l=lookups; l!=NULL && l->feature_tag==lookups->feature_tag; l = l->feature_next ) {
	    last = l;
	    ++cnt;
	    lcnt += CountLangs(sf,l->script_lang_index);
	    if ( l->script_lang_index!=lookups->script_lang_index )
		allsame = false;
	}
	if ( allsame ) {
	    f = chunkalloc(sizeof(struct feature));
	    f->feature_tag = lookups->feature_tag;
	    f->baselcnt = cnt;
	    f->baselookups = galloc(cnt*sizeof(struct lookup *));
	    f->feature_cnt = fcnt++;
	    cnt = 0;
	    for ( l=lookups; l!=NULL && l->feature_tag==lookups->feature_tag; l = l->feature_next )
		f->baselookups[cnt++] = l;
	    AddSliToFeature(f,sf,lookups->script_lang_index);
	    if ( flast==NULL )
		fhead = f;
	    else
		flast->feature_next = f;
	    flast = f;
    continue;
	}
	if ( lcnt>slumax ) {
	    if ( slumax==0 )
		slu = galloc(lcnt*sizeof(struct slusage));
	    else
		slu = grealloc(slu,lcnt*sizeof(struct slusage));
	    slumax = lcnt;
	}
	lcnt = 0; which = 0;
	for ( l=lookups; l!=NULL && l->feature_tag==lookups->feature_tag; l = l->feature_next, ++which ) {
	    struct script_record *sr = (sf->cidmaster?sf->cidmaster:sf)->script_lang[l->script_lang_index];
	    for ( s=0; sr[s].script!=0; ++s ) for ( lang=0; sr[s].langs[lang]!=0; ++lang ) {
		pos = FindSL(slu,lcnt,sr[s].script,sr[s].langs[lang]);
		if ( pos==-1 ) {
		    slu[lcnt].script = sr[s].script;
		    slu[lcnt].lang = sr[s].langs[lang];
		    slu[lcnt].processed = false;
		    if ( cnt<32 )
			slu[lcnt].u.used = 0;
		    else
			slu[lcnt].u.usedarr = gcalloc( (cnt+31)>>5,sizeof(uint32) );
		    pos = lcnt++;
		}
		if ( cnt<=32 )
		    slu[pos].u.used |= (1<<which);
		else
		    slu[pos].u.usedarr[which>>5] |= (1<<(which&0x1f));
	    }
	}
	for ( pos=0; pos<lcnt; ++pos ) if ( !slu[pos].processed ) {
	    f = chunkalloc(sizeof(struct feature));
	    f->feature_tag = lookups->feature_tag;
	    for ( tot=which=0; which<cnt; ++which )
		if ( (cnt<=32 && (slu[pos].u.used&(1<<which))) ||
			(cnt>32 && (slu[pos].u.usedarr[which>>5]&(1<<(which&0x1f)))) )
		++tot;
	    f->baselcnt = tot;
	    f->baselookups = galloc(cnt*sizeof(struct lookup *));
	    f->feature_cnt = fcnt++;
	    tot = 0;
	    for ( which = 0, l=lookups; l!=NULL && l->feature_tag==lookups->feature_tag; l = l->feature_next, ++which )
		if ( (cnt<=32 && (slu[pos].u.used&(1<<which))) ||
			(cnt>32 && (slu[pos].u.usedarr[which>>5]&(1<<(which&0x1f)))) )
		    f->baselookups[tot++] = l;
	    AddScriptLangsToFeature(f,slu,pos,lcnt,cnt);
	    if ( flast==NULL )
		fhead = f;
	    else
		flast->feature_next = f;
	    flast = f;
	}
	if ( cnt>32 ) {
	    for ( pos=0; pos<lcnt; ++pos )
		free(slu[pos].u.usedarr);
	}
    }
    if ( !added_size && is_gpos && sf->design_size!=0 ) {
	f = CreateSizeFeature(sf,fcnt++);
	if ( flast==NULL )
	    fhead = f;
	else
	    flast->feature_next = f;
    }
    if ( slumax!=0 )
	free(slu);

/* Now coalesce sub-tables into lookups */
    lc_warned = false;
    clhead = cllast = NULL;
    clcnt = 0;
    for ( l = nested; l!=NULL ; l=l->next ) {
	cl = chunkalloc(sizeof(struct clookup));
	if ( clhead==NULL )
	    clhead = cl;
	else
	    cllast->next = cl;
	cllast = cl;
	cl->flags = l->flags;
	cl->lookup_type = l->lookup_type;
	cl->lcnt = 1;
	cl->lookups = galloc(sizeof(struct lookup *));
	cl->lookups[0] = l;
	if ( clcnt!=l->lookup_cnt && !lc_warned ) {
	    IError("GPOS/GSUB: Lookup ordering mismatch #2");
	    lc_warned = true;
	}
	cl->lookup_cnt = clcnt++;
    }
    clendnest = cllast;
    clcntnest = clcnt;

    for ( f=fhead; f!=NULL; f=f->feature_next ) {
	clfcnt = 0;
	clflast = NULL;
	for ( i=0; i<f->baselcnt; ++i )
	    f->baselookups[i]->used = false;
	for ( i=0; i<f->baselcnt; ++i ) if ( !f->baselookups[i]->used ) {
	    f->baselookups[i]->used = true;
	    cnt = 1;
	    /* ATM can't handle a kern feature with more than one lookup */
	    /* TTX can't handle a sub-table referenced by more than one lookup*/
	    /*  (in other words, can't handle what ATM forces us to do) */
	    /* Alexej suggests only making ATM happy in cases when TTX can */
	    /*  handle it (that is only coalescing subtables if they have the */
	    /*  same sli -- so won't appear in other lookups.) */
	    for ( j=i+1; j<f->baselcnt; ++j ) {
		if ( f->baselookups[i]->flags == f->baselookups[j]->flags &&
			f->baselookups[i]->lookup_type == f->baselookups[j]->lookup_type &&
                        f->baselookups[i]->script_lang_index == f->baselookups[j]->script_lang_index) {
		    f->baselookups[j]->used = true;
		    ++cnt;
		}
	    }
	    temp = galloc(cnt*sizeof(struct lookup *));
	    cnt = 0;
	    temp[cnt++] = f->baselookups[i];
	    for ( j=i+1; j<f->baselcnt; ++j ) {
		if ( f->baselookups[i]->flags == f->baselookups[j]->flags &&
			f->baselookups[i]->lookup_type == f->baselookups[j]->lookup_type &&
                        f->baselookups[i]->script_lang_index == f->baselookups[j]->script_lang_index )
		    temp[cnt++] = f->baselookups[j];
	    }
	    for ( cl2 = clhead; cl2!=NULL; cl2=cl2->next ) {
		if ( cl2->lcnt!=cnt )
	    continue;
		for ( j=0; j<cnt; ++j )
		    if ( cl2->lookups[j]!=temp[j] )
		break;
		if ( j==cnt )
	    break;
	    }

	    cl = chunkalloc(sizeof(struct clookup));
	    if ( clhead==NULL )
		clhead = cl;
	    else
		cllast->next = cl;
	    cllast = cl;
	    if ( clflast==NULL )
		f->lookups = cl;
	    else
		clflast->fnext = cl;
	    clflast = cl;
	    cl->flags = temp[0]->flags;
	    cl->lookup_type = temp[0]->lookup_type;
	    cl->lcnt = cnt;
	    cl->lookups = temp;
	    if ( cl2==NULL )
		cl->lookup_cnt = clcnt++;
	    else {
		cl->duplicate = true;
		cl->lookup_cnt = cl2->lookup_cnt;
	    }
	    clfcnt++;
	}
	f->lcnt = clfcnt;
    }

    if ( clendnest==NULL )
	clstart = clhead;
    else
	clstart = clendnest->next;
    for ( cnt=0, cl=clstart; cl!=NULL; cl=cl->next, ++cnt );
    array = galloc((cnt+1) *sizeof(struct clookup *));
    for ( cnt=0, cl=clstart; cl!=NULL; cl=cl->next, ++cnt )
	array[cnt] = cl;
    array[cnt] = NULL;
    for ( i=0; i<cnt-1; ++i ) for ( j=i+1; j<cnt; ++j ) {
	if ( TTFFeatureIndex(array[i]->lookups[0]->feature_tag,ord,!is_gpos)>TTFFeatureIndex(array[j]->lookups[0]->feature_tag,ord,!is_gpos)) {
	    struct clookup *temp = array[i];
	    array[i] = array[j];
	    array[j] = temp;
	}
    }
    clcnt = clcntnest;
    for ( i=0; i<cnt; ++i ) {
	array[i]->next = array[i+1];
	if ( !array[i]->duplicate ) {
	    for ( j=0; j<cnt; ++j ) if ( !array[j]->ticked && array[j]->duplicate && array[j]->lookup_cnt==array[i]->lookup_cnt ) {
		array[j]->ticked = true;
		array[j]->lookup_cnt = clcnt;
	    }
	    array[i]->lookup_cnt = clcnt++;
	}
    }
    if ( clendnest==NULL )
	clhead = array[0];
    else
	clendnest->next = array[0];
    free(array);
    
    *clookups = clhead;
	
return( fhead );
}

static uint32 *ScriptsFromFeatures(SplineFont *sf, uint8 *used,
	int *scnt, uint32 **_langs, int *lcnt) {
    int i,j,k,l, smax,lmax;
    uint32 *scripts, *langs;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;

    smax = lmax = 0;
    for ( i=0; sf->script_lang[i]!=NULL; ++i ) if ( used[i] ) {
	for ( j=0; sf->script_lang[i][j].script!=0; ++j ) {
	    for ( k=0; sf->script_lang[i][j].langs[k]!=0; ++k );
	    lmax += k;
	}
	smax += j;
    }

    scripts = galloc((smax+1)*sizeof(uint32));
    langs = galloc((lmax+1)*sizeof(uint32));
    smax = lmax = 0;
    for ( i=0; sf->script_lang[i]!=NULL; ++i ) if ( used[i] ) {
	for ( j=0; sf->script_lang[i][j].script!=0; ++j ) {
	    uint32 script = sf->script_lang[i][j].script;
	    for ( k=0; sf->script_lang[i][j].langs[k]!=0; ++k ) {
		uint32 lang = sf->script_lang[i][j].langs[k];
		for ( l=0; l<lmax && langs[l]!=lang; ++l );
		if ( l==lmax ) langs[lmax++] = lang;
	    }
	    for ( l=0; l<smax && scripts[l]!=script; ++l );
	    if ( l==smax ) scripts[smax++] = script;
	}
    }
    scripts[smax] = 0;
    langs[lmax] = 0;

    for ( i=0; i<smax-1; ++i ) for ( j=i+1; j<smax; ++j ) {
	if ( scripts[i]>scripts[j] ) {
	    uint32 temp = scripts[i];
	    scripts[i] = scripts[j];
	    scripts[j] = temp;
	}
    }

    for ( i=0; i<lmax-1; ++i ) for ( j=i+1; j<lmax; ++j ) {
	if ( langs[i]>langs[j] ) {
	    uint32 temp = langs[i];
	    langs[i] = langs[j];
	    langs[j] = temp;
	}
    }

    *_langs = langs;
    *scnt = smax;
    *lcnt = lmax;
return( scripts );
}

/* This routine is to work around what I believe is a Uniscribe bug */
/* If I have a mini-font with a hebrew base char and a hebrew mark char, and */
/*  a GPOS mark-to-base table, then the font only works IF there is also a */
/*  GSUB table with a (possibly empty) entry for script 'hebr' */
/* It appears that GSUB must contain script entries for all scripts in GPOS */
/* Actually it's more complex and is described in some detail in otf_orderlangs */
/*  (in hebrew both tables must be present, or MS rejects the font (for hebrew) */
/*  (in latin the font isn't rejected, but GPOS won't work unless GSUB is present */
static uint32 *MergeScripts(struct alltabs *at,uint32 *scripts,int *scnt) {
    int i,j,k;
    uint32 *s;

    if ( scripts==NULL ) {
	*scnt = at->script_cnt;
	s = galloc((at->script_cnt+1)*sizeof(uint32));
	memcpy(s,at->scripts,(at->script_cnt+1)*sizeof(uint32));
return( s );
    } else if ( at->scripts==NULL )
return( scripts );

    s = galloc((*scnt+at->script_cnt+1)*sizeof(uint32));
    for ( i=j=k=0; i<*scnt || j<at->script_cnt; ) {
	if ( i<*scnt && j<at->script_cnt ) {
	    if ( scripts[i] == at->scripts[j] ) {
		s[k++] = scripts[i];
		++i; ++j;
	    } else if ( scripts[i] < at->scripts[j] ) {
		s[k++] = scripts[i++];
	    } else {
		s[k++] = at->scripts[j++];
	    }
	} else if ( i<*scnt )
	    s[k++] = scripts[i++];
	else
	    s[k++] = at->scripts[j++];
    }
    s[k] = 0;
    free(scripts);
    *scnt = k;
return( s );
}

static FILE *dumpg___info(struct alltabs *at, SplineFont *sf,int is_gpos) {
    /* Dump out either a gpos or a gsub table. gpos handles kerns, gsub ligs */
    FILE *lfile, *lfile2, *g___, *efile;
    struct lookup *lookups=NULL, *features_ordered, *l, *next, *nested;
    struct feature *features, *f, *fnext;
    struct sl *sl, *slnext;
    int cnt, fcnt, offset, i;
    char *buf;
    uint32 *scripts, *langs;
    uint32 lookup_list_table_start, feature_list_table_start, here, scripts_start_offset;
    struct table_ordering *ord;
    uint8 *touched, *used;
    int lc;
    SplineFont *master;
    int lc_warned=false;
    int32 size_params_loc, size_params_ptr;
    struct clookup *clookups, *cl, *cnext;

    at->next_lookup = 0;
    lfile = tmpfile();
    if ( is_gpos ) {
	lookups = GPOSfigureLookups(lfile,sf,at,&nested);
	for ( ord = sf->orders; ord!=NULL && ord->table_tag!=CHR('G','P','O','S'); ord = ord->next );
    } else {
	lookups = GSUBfigureLookups(lfile,sf,at,&nested);
	for ( ord = sf->orders; ord!=NULL && ord->table_tag!=CHR('G','S','U','B'); ord = ord->next );
    }
    if ( lookups==NULL && at->script_cnt==0 ) {
	fclose(lfile);
return( NULL );
    }

    lookups = reverse_list(lookups);
    nested = order_nested(nested);
    lfile2 = tmpfile();
    features_ordered = orderlookups(&lookups,ord,lfile2,lfile,nested,!is_gpos);
    lfile = lfile2;
    features = CoalesceLookups(sf,features_ordered,nested,&clookups,is_gpos,ord);

    master = ( sf->cidmaster ) ? sf->cidmaster : ( sf->mm ) ? sf->mm->normal : sf;
    for ( i=0; master->script_lang[i]!=NULL; ++i );
    used = gcalloc(i,1);
    for ( l=lookups; l!=NULL; l=l->next )
	used[l->script_lang_index] = true;

    scripts = ScriptsFromFeatures(sf,used,&cnt,&langs,&lc);
    scripts = MergeScripts(at,scripts,&cnt);
    touched = galloc(lc);

    g___ = tmpfile();
    putlong(g___,0x10000);		/* version number */
    putshort(g___,10);		/* offset to script table */
    putshort(g___,0);		/* offset to features. Come back for this */
    putshort(g___,0);		/* offset to lookups.  Come back for this */
/* Now the scripts */
    scripts_start_offset = ftell(g___);
    putshort(g___,cnt);
    for ( i=0; i<cnt; ++i ) {
	putlong(g___,scripts[i]);
	putshort(g___,0);	/* fix up later */
    }

    /* Ok, that was the script_list_table which gives each script an offset */
    /* Now for each script we provide a Script table which contains an */
    /*  offset to a bunch of features for the default language, and a */
    /*  a more complex situation for non-default languages. */
    offset=2+4;
    for ( i=0; i<cnt; ++i ) {
	here = ftell(g___);
	fseek(g___,scripts_start_offset+offset,SEEK_SET);
	putshort(g___,here-scripts_start_offset);
	offset+=6;
	fseek(g___,here,SEEK_SET);
	dump_script_table(g___,sf,features,
		scripts[i],cnt,used,
		langs,lc,touched);
    }
    /* And that should finish all the scripts/languages */

    feature_list_table_start = ftell(g___);
    fseek(g___,6,SEEK_SET);
    putshort(g___,feature_list_table_start);
    fseek(g___,0,SEEK_END);
    for ( f=features, fcnt=0; f!=NULL; f=f->feature_next, ++fcnt );
    putshort(g___,fcnt);	/* Number of features */
    offset = 2+6*fcnt;		/* Offset to start of first feature table from beginning of feature_list */
    for ( f=features; f!=NULL; f=f->feature_next ) {
	putlong(g___,f->feature_tag);
	putshort(g___,offset);
	offset += 4+2*f->lcnt;
    }
    /* for each feature, one feature table */
    size_params_ptr = 0;
    for ( f=features; f!=NULL; f=f->feature_next ) {
	if ( f->is_size ) size_params_ptr = ftell(g___);
	putshort(g___,0);		/* No feature params */
	putshort(g___,f->lcnt);		/* this many lookups */
	for ( cl=f->lookups; cl!=NULL; cl=cl->fnext )
	    putshort(g___,cl->lookup_cnt);	/* index of each lookup */
    }
    if ( size_params_ptr!=0 ) {
	size_params_loc = ftell(g___);
	fseek(g___,size_params_ptr,SEEK_SET);
	/* When Adobe first released fonts containing the 'size' feature */
	/*  they did not follow the spec, and the offset to the size parameters */
	/*  was relative to the wrong location. They claim (Aug 2006) that */
	/*  this has been fixed. FF used to do what Adobe did. Many programs */
	/*  expect broken sizes tables now. Therefore allow the user to chose */
	/*  which kind to output */
	putshort(g___,size_params_loc-((at->gi.flags&ttf_flag_brokensize)?feature_list_table_start:size_params_ptr));
	fseek(g___,size_params_loc,SEEK_SET);
	putshort(g___,sf->design_size);
	if ( sf->fontstyle_id!=0 || sf->fontstyle_name!=NULL ||
		sf->design_range_bottom!=0 || sf->design_range_top!=0 ) {
	    putshort(g___,sf->fontstyle_id);
	    at->fontstyle_name_strid = at->next_strid++;
	    putshort(g___,at->fontstyle_name_strid);
	    putshort(g___,sf->design_range_bottom);
	    putshort(g___,sf->design_range_top);
	} else {
	    putshort(g___,0);
	    putshort(g___,0);
	    putshort(g___,0);
	    putshort(g___,0);
	}
    }
    /* And that should finish all the features */
    for ( f=features; f!=NULL; f=fnext ) {
	fnext = f->feature_next;
	free(f->baselookups);
	for ( sl=f->sl; sl!=NULL; sl=slnext ) {
	    slnext = sl->next;
	    chunkfree(sl,sizeof(struct sl));
	}
	chunkfree(f,sizeof(struct feature));
    }
    /* So free the feature list */

    for ( cnt=0, cl=clookups; cl!=NULL; cl=cl->next ) {
	if ( cl->duplicate )
    continue;
	if ( cl->lookup_cnt!=cnt && !lc_warned ) {
	    IError("GPOS/GSUB: Lookup ordering mismatch");
	    lc_warned = true;
	}
	++cnt;
    }
    lookup_list_table_start = ftell(g___);
    fseek(g___,8,SEEK_SET);
    putshort(g___,lookup_list_table_start);
    fseek(g___,0,SEEK_END);
    putshort(g___,cnt);
    offset = 2+2*cnt;		/* Offset to start of first feature table from beginning of feature_list */
    for ( cl=clookups; cl!=NULL; cl=cl->next ) if ( !cl->duplicate ) {
	putshort(g___,offset);
	offset += 6+2*cl->lcnt;		/* 6 bytes header +2 per lookup */
    }
    offset -= 2+2*cnt;
    /* now the lookup tables */
      /* do we need any extension sub-tables? */
    efile=g___FigureExtensionSubTables(clookups,lookups,offset,is_gpos);
    for ( cl=clookups; cl!=NULL; cl=cl->next ) if ( !cl->duplicate ) {
	putshort(g___,cl->lookup_type);
	putshort(g___,cl->flags);
	putshort(g___,cl->lcnt);
	for ( i=0; i<cl->lcnt; ++i ) {
	    putshort(g___,offset+cl->lookups[i]->offset[cl->extension]);
	    /* Offset to lookup data which is in the temp file */
	    /* we keep adjusting offset so it reflects the distance between */
	    /* here and the place where the temp file will start, and then */
	    /* we need to skip l->offset bytes in the temp file */
	    /* If it's a big GPOS/SUB table we may also need some extension */
	    /*  pointers, but FigureExtension will adjust for that */
	}
	offset -= 6+2*cl->lcnt;
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
    for ( l=nested; l!=NULL; l=next ) {
	next = l->next;
	chunkfree(l,sizeof(*l));
    }
    for ( cl=clookups; cl!=NULL; cl=cnext ) {
	cnext = cl->next;
	free( cl->lookups );
	chunkfree(cl,sizeof(*cl));
    }
    free(scripts);
    free(langs);
    free(touched);
    free(used);
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
    /* substitutions such as: Ligatures, cjk vertical rotation replacement, */
    /*  arabic forms, small caps, ... */
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
    int i, j, offset;

    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->type == pst_lcaret )
    break;
    }
    if ( pst==NULL )
return;

    if ( SCRightToLeft(sc) ) {
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
    offset = sizeof(uint16) + sizeof(uint16)*pst->u.lcaret.cnt;
    for ( i=0; i<pst->u.lcaret.cnt; ++i ) {
	putshort(gdef,offset);
	offset+=4;
    }
    for ( i=0; i<pst->u.lcaret.cnt; ++i ) {
	putshort(gdef,1);		/* Format 1 */
	putshort(gdef,pst->u.lcaret.carets[i]);
    }
}

int gdefclass(SplineChar *sc) {
    PST *pst;
    AnchorPoint *ap;

    if ( sc->glyph_class!=0 )
return( sc->glyph_class-1 );

    if ( strcmp(sc->name,".notdef")==0 )
return( 0 );

    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->type == pst_ligature )
return( 2 );			/* Ligature */
    }

    ap=sc->anchor;
    while ( ap!=NULL && (ap->type==at_centry || ap->type==at_cexit) )
	ap = ap->next;
    if ( ap!=NULL && (ap->type==at_mark || ap->type==at_basemark) )
return( 3 );
    else
return( 1 );
    /* I not quite sure what a componant glyph is. Probably something that */
    /*  is not in the cmap table and is referenced in other glyphs */
    /* Anyway I never return class 4 */ /* (I've never seen it used by others) */
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
    /* Ah. Some indic fonts need the mark attach class subtable for greater */
    /*  control of lookup flags */
    /* All my example fonts contain a ligature caret list subtable, which is */
    /*  empty. Odd, but perhaps important */
    PST *pst;
    int i,j,k, lcnt, needsclass;
    int pos, offset;
    int cnt, start, last, lastval;
    SplineChar **glyphs, *sc;

    /* Don't look in the cidmaster if we are only dumping one subfont */
    if ( sf->cidmaster && sf->cidmaster->glyphs!=NULL ) sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;

    glyphs = NULL;
    for ( k=0; k<2; ++k ) {
	lcnt = 0;
	needsclass = false;
	for ( i=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
	    SplineChar *sc = sf->glyphs[at->gi.bygid[i]];
	    if ( sc->glyph_class!=0 || gdefclass(sc)!=1 )
		needsclass = true;
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->type == pst_lcaret ) {
		    for ( j=pst->u.lcaret.cnt-1; j>=0; --j )
			if ( pst->u.lcaret.carets[j]!=0 )
		    break;
		    if ( j!=-1 )
	    break;
		}
	    }
	    if ( pst!=NULL ) {
		if ( glyphs!=NULL ) glyphs[lcnt] = sc;
		++lcnt;
	    }
	}
	if ( lcnt==0 )
    break;
	if ( glyphs!=NULL )
    break;
	glyphs = galloc((lcnt+1)*sizeof(SplineChar *));
	glyphs[lcnt] = NULL;
    }
    if ( !needsclass && lcnt==0 && sf->mark_class_cnt==0 )
return;					/* No anchor positioning, no ligature carets */

    at->gdef = tmpfile();
    putlong(at->gdef,0x00010000);		/* Version */
    putshort(at->gdef, needsclass ? 12 : 0 );	/* glyph class defn table */
    putshort(at->gdef, 0 );			/* attachment list table */
    putshort(at->gdef, 0 );			/* ligature caret table (come back and fix up later) */
    putshort(at->gdef, 0 );			/* mark attachment class table */

	/* Glyph class subtable */
    if ( needsclass ) {
	/* Mark shouldn't conflict with anything */
	/* Ligature is more important than Base */
	/* Component is not used */
#if 1		/* ttx can't seem to support class format type 1 so let's output type 2 */
	for ( j=0; j<2; ++j ) {
	    cnt = 0;
	    for ( i=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
		sc = sf->glyphs[at->gi.bygid[i]];
		if ( sc!=NULL && sc->ttf_glyph!=-1 ) {
		    lastval = gdefclass(sc);
		    start = last = i;
		    for ( ; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
			sc = sf->glyphs[at->gi.bygid[i]];
			if ( gdefclass(sc)!=lastval )
		    break;
			last = i;
		    }
		    --i;
		    if ( lastval!=0 ) {
			if ( j==1 ) {
			    putshort(at->gdef,start);
			    putshort(at->gdef,last);
			    putshort(at->gdef,lastval);
			}
			++cnt;
		    }
		}
	    }
	    if ( j==0 ) {
		putshort(at->gdef,2);	/* class format 2, range list by class */
		putshort(at->gdef,cnt);
	    }
	}
#else
	putshort(at->gdef,1);	/* class format 1 complete list of glyphs */
	putshort(at->gdef,0);	/* First glyph */
	putshort(at->gdef,at->maxp.numGlyphs );
	j=0;
	for ( i=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
	    for ( ; j<i; ++j )
		putshort(at->gdef,1);	/* Any hidden characters (like notdef) default to base */
	    putshort(at->gdef,gdefclass(sf->glyphs[at->gi.bygid[i]]));
	    ++j;
	}
#endif
    }

	/* Ligature caret subtable. Always include this if we have a GDEF */
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
	    offset+=2+6*LigCaretCnt(glyphs[i]);
	}
	for ( i=0; i<lcnt; ++i )
	    DumpLigCarets(at->gdef,glyphs[i]);
	offset = ftell(at->gdef);
	fseek(at->gdef,pos,SEEK_SET);
	putshort(at->gdef,offset-pos);
	fseek(at->gdef,0,SEEK_END);
	dumpcoveragetable(at->gdef,glyphs);
    }

	/* Mark Attachment Class Subtable */
    if ( sf->mark_class_cnt>0 ) {
	uint16 *mclasses = ClassesFromNames(sf,sf->mark_classes,sf->mark_class_cnt,at->maxp.numGlyphs,NULL,false);
	pos = ftell(at->gdef);
	fseek(at->gdef,10,SEEK_SET);		/* location of mark attach table offset */
	putshort(at->gdef,pos);
	fseek(at->gdef,0,SEEK_END);
	DumpClass(at->gdef,mclasses,at->maxp.numGlyphs);
	free(mclasses);
    }

    at->gdeflen = ftell(at->gdef);
    if ( at->gdeflen&1 ) putc('\0',at->gdef);
    if ( (at->gdeflen+1)&2 ) putshort(at->gdef,0);
}

/* ************************************************************************** */
/* *************************    utility routines    ************************* */
/* ************************************************************************** */

#include "pfaeditui.h"
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include <gkeysym.h>

struct order_dlg {
    uint32 table_tag;
    SplineFont *sf;
    struct table_ordering *ord;
    int done;
    GGadget *list;
    GGadget *up, *down;
    GGadget *top, *bottom;
    GGadget *new, *del;
    GGadget *ok, *cancel;
};

static uint32 *GetTags(SplineFont *sf, int isgsub) {
    /* !!!! Don't currently handle gpos. Needs to look for kern, marks, curs, etc. */
    int max, cnt, i,j;
    uint32 *tags;
    PST *pst;

    max = 30; cnt = 0;
    tags = galloc((max+1)*sizeof(uint32));
    for ( i=0; i<sf->glyphcnt; i++ ) if ( sf->glyphs[i]!=NULL ) {
	for ( pst = sf->glyphs[i]->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->type==pst_substitution || pst->type==pst_alternate ||
		    pst->type == pst_multiple || pst->type==pst_ligature ) {
		for ( j=0; j<cnt; ++j )
		    if ( tags[j]==pst->tag )
		break;
		if ( j==cnt ) {
		    if ( cnt>=max ) {
			max += 30;
			tags = grealloc(tags,(max+1)*sizeof(uint32));
		    }
		    tags[cnt++] = pst->tag;
		}
	    }
	}
    }
    tags[cnt] = 0;
    if ( cnt==0 ) {
	free(tags);
return( NULL );
    }
return( tags );
}

static int order_e_h(GWindow gw, GEvent *event) {
    struct order_dlg *d = GDrawGetUserData(gw);
    int i;
    int32 len;
    GTextInfo **ti;
    uint32 *new;

    if ( event->type==et_close ) {
	d->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("fontinfo.html#Order");
return( true );
	}
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	if ( event->u.control.g == d->cancel ) {
	    d->done = true;
	} else if ( event->u.control.g == d->top ||
		event->u.control.g == d->up ||
		event->u.control.g == d->down ||
		event->u.control.g == d->bottom ) {
	    GListMoveSelected(d->list,event->u.control.g == d->top ? 0x80000000 :
		    event->u.control.g == d->up ? -1 :
		    event->u.control.g == d->down ? 1 : 0x7fffffff );
	} else if ( event->u.control.g == d->del ) {
	    GListDelSelected(d->list);
	} else if ( event->u.control.g == d->ok ) {
	    ti = GGadgetGetList(d->list,&len);
	    new = galloc((len+1)*sizeof(uint32));
	    new[len] = 0;
	    for ( i=0; i<len; ++i )
		new[i] = (ti[i]->text[0]<<24)|(ti[i]->text[1]<<16)|(ti[i]->text[2]<<8)|ti[i]->text[3];
	    if ( d->ord==NULL ) {
		d->ord = gcalloc(1,sizeof(struct table_ordering));
		d->ord->table_tag = d->table_tag;
		d->ord->next = d->sf->orders;
		d->sf->orders = d->ord;
	    }
	    free(d->ord->ordered_features);
	    d->ord->ordered_features = new;
	    d->done = true;
	}
    }
return( true );
}

void OrderTable(SplineFont *sf,uint32 table_tag) {
    struct table_ordering *ord;
    uint32 *tags_used, *merged;
    int cnt1, cnt2;
    int i,j,temp;
    GTextInfo *ti;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[10];
    GTextInfo label[10];
    struct order_dlg d;
    static unichar_t monospace[] = { 'c','o','u','r','i','e','r',',','m', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
    FontRequest rq;
    GFont *font;
    int isgsub = table_tag==CHR('G','S','U','B');

    for ( ord = sf->orders; ord!=NULL && ord->table_tag!=table_tag; ord = ord->next );
    tags_used = GetTags(sf,isgsub);
    cnt1 = cnt2 = 0;
    if ( ord!=NULL )
	for ( cnt1=0; ord->ordered_features[cnt1]!=0; ++cnt1 );
    if ( tags_used!=NULL ) {
	for ( cnt2=0; tags_used[cnt2]!=0; ++cnt2 );
	/* order this list using our default ordering */
	for ( i=0; i<cnt2; ++i ) for ( j=i+1; j<cnt2; ++j ) {
	    if ( TTFFeatureIndex(tags_used[i],NULL,isgsub)>TTFFeatureIndex(tags_used[j],NULL,isgsub) ) {
		temp = tags_used[i];
		tags_used[i] = tags_used[j];
		tags_used[j] = temp;
	    }
	}
    }
    merged = galloc((cnt1+cnt2+1)*sizeof(uint32));
    if ( ord!=NULL )
	for ( cnt1=0; ord->ordered_features[cnt1]!=0; ++cnt1 )
	    merged[cnt1] = ord->ordered_features[cnt1];
    i = cnt1;
    if ( tags_used!=NULL ) {
	for ( cnt2=0; tags_used[cnt2]!=0; ++cnt2 ) {
	    for ( j=0; j<cnt1; ++j )
		if ( merged[j]==tags_used[cnt2] )
	    break;
	    if ( j==cnt1 )
		merged[i++] = tags_used[cnt2];
	}
    }
    merged[i] = 0;
    free(tags_used);

    ti = gcalloc((i+1),sizeof(GTextInfo));
    for ( i=0; merged[i]!=0; ++i ) {
	ti[i].text = galloc(5*sizeof(unichar_t));
	ti[i].text[0] = merged[i]>>24;
	ti[i].text[1] = (merged[i]>>16)&0xff;
	ti[i].text[2] = (merged[i]>>8)&0xff;
	ti[i].text[3] = merged[i]&0xff;
	ti[i].text[4] = '\0';
    }
    free(merged);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Set GSUB/morx Ordering");
    pos.x = pos.y = 0;
    pos.width = GDrawPointsToPixels(NULL,GGadgetScale(20)+2*GIntGetResource(_NUM_Buttonsize));
    pos.height = GDrawPointsToPixels(NULL,195);
    gw = GDrawCreateTopWindow(NULL,&pos,order_e_h,&d,&wattrs);

    memset(&d,0,sizeof(d));
    d.sf = sf;
    d.table_tag = table_tag;
    d.ord = ord;

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));

    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5;
    gcd[0].gd.pos.width = 60; gcd[0].gd.pos.height =12*12+10;
    gcd[0].gd.flags = (gg_visible | gg_enabled);
    gcd[0].gd.u.list = ti;
    gcd[0].creator = GListCreate;

    gcd[1].gd.pos.x = -5; gcd[1].gd.pos.y = 5;
    gcd[1].gd.pos.width = -1;
    label[1].text = (unichar_t *) _("_Top");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.flags = (gg_visible | gg_enabled);
    gcd[1].creator = GButtonCreate;

    gcd[2].gd.pos.x = -5; gcd[2].gd.pos.y = 35; gcd[2].gd.pos.width = -1;
    label[2].text = (unichar_t *) _("_Up");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.flags = (gg_visible | gg_enabled);
    gcd[2].creator = GButtonCreate;

    gcd[3].gd.pos.x = -5; gcd[3].gd.pos.y = 65;
    gcd[3].gd.pos.width = -1;
    label[3].text = (unichar_t *) _("_Down");
    label[3].text_is_1byte = true;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.flags = (gg_visible | gg_enabled);
    gcd[3].creator = GButtonCreate;

    gcd[4].gd.pos.x = -5; gcd[4].gd.pos.y = 95; gcd[4].gd.pos.width = -1;
    label[4].text = (unichar_t *) _("_Bottom");
    label[4].text_is_1byte = true;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.flags = (gg_visible | gg_enabled);
    gcd[4].creator = GButtonCreate;

    gcd[5].gd.pos.x = -5; gcd[5].gd.pos.y = gcd[4].gd.pos.y+28;
    gcd[5].gd.pos.width = GIntGetResource(_NUM_Buttonsize);
    gcd[5].gd.flags = (gg_visible | gg_enabled);
    gcd[5].creator = GLineCreate;

    gcd[6].gd.pos.x = -5; gcd[6].gd.pos.y = 130; gcd[6].gd.pos.width = -1;
    label[6].text = (unichar_t *) _("Re_move");
    label[6].text_is_1byte = true;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.flags = (gg_visible | gg_enabled);
    gcd[6].creator = GButtonCreate;

    gcd[7].gd.pos.x = 2; gcd[7].gd.pos.y = gcd[0].gd.pos.y+gcd[0].gd.pos.height+5;
    gcd[7].gd.pos.width = -1;
    label[7].text = (unichar_t *) _("_OK");
    label[7].text_is_1byte = true;
    label[7].text_in_resource = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.flags = (gg_visible | gg_enabled | gg_but_default);
    gcd[7].creator = GButtonCreate;

    gcd[8].gd.pos.x = -5; gcd[8].gd.pos.y = gcd[7].gd.pos.y+3; gcd[8].gd.pos.width = -1;
    label[8].text = (unichar_t *) _("_Cancel");
    label[8].text_is_1byte = true;
    label[8].text_in_resource = true;
    gcd[8].gd.label = &label[8];
    gcd[8].gd.flags = (gg_visible | gg_enabled | gg_but_cancel);
    gcd[8].creator = GButtonCreate;

    GGadgetsCreate(gw,gcd);
    memset(&rq,0,sizeof(rq));
    rq.family_name = monospace;
    rq.point_size = 12;
    rq.weight = 400;
    font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GGadgetSetFont(gcd[0].ret,font);

    d.list = gcd[0].ret;
    d.top = gcd[1].ret;
    d.up = gcd[2].ret;
    d.down = gcd[3].ret;
    d.bottom = gcd[4].ret;
    d.del = gcd[6].ret;
    d.ok = gcd[7].ret;
    d.cancel = gcd[8].ret;

    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

void otf_orderlangs(struct alltabs *at,SplineFont *sf) {
    int i,j,k,l, len,max;
    uint8 *used;
    PST *pst;
    KernPair *kp;
    KernClass *kc;
    AnchorClass *ac;
    FPST *fpst;
    uint32 *scripts;
    int smax;

    if ( sf->cidmaster!=NULL ) sf=sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;
    if ( sf->script_lang==NULL )
return;

    /* order the scripts in each scriptlist and then the languages attached */
    /*  to each script */
    max = 0;
    for ( i=0; sf->script_lang[i]!=NULL; ++i ) {
	struct script_record *sl = sf->script_lang[i];
	for ( len = 0; sl[len].script!=0; ++len );
	for ( k=0; k<len-1; ++k ) for ( l=k+1; l<len; ++l ) {
	    if ( sl[k].script > sl[l].script ) {
		struct script_record slr;
		slr = sl[k];
		sl[k] = sl[l];
		sl[l] = slr;
	    }
	}
	for ( j=0; sl[j].script!=0; ++j ) {
	    uint32 *langs = sl[j].langs;
	    for ( len=0; langs[len]!=0; ++len );
	    for ( k=0; k<len-1; ++k ) for ( l=k+1; l<len; ++l ) {
		if ( langs[k]>langs[l] ) {
		    uint32 temp = langs[k];
		    langs[k] = langs[l];
		    langs[l] = temp;
		}
	    }
	}
	if ( j>max ) max = j;
    }

    if ( at==NULL )
return;

/* Sergey Malkin from MicroSoft tells me:
    Each shaping engine in Uniscribe can decide on its requirements for
    layout tables - some of them require both GSUB and GPOS, in some cases
    any table present is enough, or it can work without any table. 

    Sometimes, purpose of the check is to determine if font is supporting
    particular script - if required tables are not there font is just
    rejected by this shaping engine. Sometimes, shaping engine can not just
    reject the font because there are fonts using older shaping technologies
    we still have to support, so it uses some logic when to fallback to
    legacy layout code. 

    In your case this is Hebrew, where both tables are required to use
    OpenType processing. Arabic requires both tables too, Latin requires
    GSUB to execute GPOS. But in general, if you have both tables you should
    be safe with any script to get fully featured OpenType shaping.

In other words, if we have a Hebrew font with just GPOS features they won't work,
and MS will not use the font at all. We must add a GSUB table. In the unlikely
event that we had a hebrew font with only GSUB it would not work either.

So if we want our lookups to have a chance of executing under Uniscribe we
better make sure that both tables have the same script set.

(Sergey says we could optimize a little: A Latin GSUB table will run without
a GPOS, but he says the GPOS won't work without a GSUB.)
*/
    /* Here we build the set of scripts used in either table... */
    /* I could just look at the script_lang table and use everything in it */
    /* But if I do that I may through in some extranious scripts that never */
    /* have a feature attached to them. */

    used = gcalloc(sf->sli_cnt,sizeof(uint8));
    for ( i=0; i<sf->glyphcnt; i++ ) if ( sf->glyphs[i]!=NULL ) {
	for ( pst = sf->glyphs[i]->possub; pst!=NULL; pst=pst->next )
	    if ( pst->type<=pst_ligature && /*pst->script_lang_index>=0 &&*/
		    pst->script_lang_index<sf->sli_cnt )
		used[pst->script_lang_index] = true;
	for ( kp = sf->glyphs[i]->kerns; kp!=NULL; kp = kp->next )
	    if ( /*kp->sli>=0 &&*/ kp->sli<sf->sli_cnt )
		used[kp->sli] = true;
	for ( kp = sf->glyphs[i]->vkerns; kp!=NULL; kp = kp->next )
	    if ( /*kp->sli>=0 &&*/ kp->sli<sf->sli_cnt )
		used[kp->sli] = true;
    }
    for ( kc = sf->kerns; kc!=NULL; kc = kc->next )
	if ( /*kc->sli>=0 &&*/ kc->sli<sf->sli_cnt )
	    used[kc->sli] = true;
    for ( kc = sf->vkerns; kc!=NULL; kc = kc->next )
	if ( /*kc->sli>=0 &&*/ kc->sli<sf->sli_cnt )
	    used[kc->sli] = true;
    for ( ac=sf->anchor; ac!=NULL; ac = ac->next )
	if ( /*ac->script_lang_index>=0 &&*/ ac->script_lang_index<sf->sli_cnt )
	    used[ac->script_lang_index] = true;
    for ( fpst=sf->possub; fpst!=NULL; fpst=fpst->next )
	if ( /*fpst->script_lang_index>=0 &&*/ fpst->script_lang_index<sf->sli_cnt )
	    used[fpst->script_lang_index] = true;

    smax = 0;
    for ( i=0; sf->script_lang[i]!=NULL; ++i ) if ( used[i] ) {
	for ( j=0; sf->script_lang[i][j].script!=0; ++j );
	smax += j;
    }

    scripts = galloc((smax+1)*sizeof(uint32));
    smax = 0;
    for ( i=0; sf->script_lang[i]!=NULL; ++i ) if ( used[i] ) {
	for ( j=0; sf->script_lang[i][j].script!=0; ++j )
	    scripts[smax++] = sf->script_lang[i][j].script;
    }
    scripts[smax] = 0;

    for ( i=0; i<smax-1; ++i ) for ( j=i+1; j<smax; ++j ) {
	if ( scripts[i]>scripts[j] ) {
	    uint32 temp = scripts[i];
	    scripts[i] = scripts[j];
	    scripts[j] = temp;
	}
    }
    for ( i=0; i<smax-1; ++i ) {
	for ( j=i+1; j<smax && scripts[i]==scripts[j]; ++j );
	if ( i+1!=j ) {
	    memcpy(scripts+i+1,scripts+j,(smax-j+1)*sizeof(uint32));
	    smax -= (j-i-1);
	}
    }
    at->scripts = scripts;
    at->script_cnt = smax;
    free(used);
}
