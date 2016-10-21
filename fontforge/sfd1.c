/* Copyright (C) 2000-2012 by George Williams */
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

#include "fontforge.h"
#include "fvfonts.h"
#include "lookups.h"
#include "sfd1.h"
#include <string.h>

/* This file contains the routines needed to process an old style sfd file and*/
/*  convert it into the new format */

static void SFGuessScriptList(SplineFont1 *sf) {
    uint32 scripts[32], script;
    int i, scnt=0, j;

    for ( i=0; i<sf->sf.glyphcnt; ++i ) if ( sf->sf.glyphs[i]!=NULL ) {
	script = SCScriptFromUnicode(sf->sf.glyphs[i]);
	if ( script!=0 && script!=DEFAULT_SCRIPT ) {
	    for ( j=scnt-1; j>=0 ; --j )
		if ( scripts[j]==script )
	    break;
	    if ( j<0 ) {
		scripts[scnt++] = script;
		if ( scnt>=32 )
    break;
	    }
	}
    }
    if ( scnt==0 )
	scripts[scnt++] = CHR('l','a','t','n');

    /* order scripts */
    for ( i=0; i<scnt-1; ++i ) for ( j=i+1; j<scnt; ++j ) {
	if ( scripts[i]>scripts[j] ) {
	    script = scripts[i];
	    scripts[i] = scripts[j];
	    scripts[j] = script;
	}
    }

    if ( sf->sf.cidmaster ) sf = (SplineFont1 *) sf->sf.cidmaster;
    else if ( sf->sf.mm!=NULL ) sf=(SplineFont1 *) sf->sf.mm->normal;
    if ( sf->script_lang!=NULL )
return;
    sf->script_lang = calloc(2,sizeof(struct script_record *));
    sf->script_lang[0] = calloc(scnt+1,sizeof(struct script_record));
    sf->sli_cnt = 1;
    for ( j=0; j<scnt; ++j ) {
	sf->script_lang[0][j].script = scripts[j];
	sf->script_lang[0][j].langs = malloc(2*sizeof(uint32));
	sf->script_lang[0][j].langs[0] = DEFAULT_LANG;
	sf->script_lang[0][j].langs[1] = 0;
    }
    sf->script_lang[1] = NULL;
}

static int SLContains(struct script_record *sr, uint32 script, uint32 lang) {
    int i, j;

    if ( script==DEFAULT_SCRIPT || script == 0 )
return( true );
    for ( i=0; sr[i].script!=0; ++i ) {
	if ( sr[i].script==script ) {
	    if ( lang==0 )
return( true );
	    for ( j=0; sr[i].langs[j]!=0; ++j )
		if ( sr[i].langs[j]==lang )
return( true );

return( false );	/* this script entry didn't contain the language. won't be any other scripts to check */
	}
    }
return( false );	/* Never found script */
}

int SFAddScriptIndex(SplineFont1 *sf,uint32 *scripts,int scnt) {
    int i,j;
    struct script_record *sr;

    if ( scnt==0 )
	scripts[scnt++] = CHR('l','a','t','n');		/* Need a default script preference */
    for ( i=0; i<scnt-1; ++i ) for ( j=i+1; j<scnt; ++j ) {
	if ( scripts[i]>scripts[j] ) {
	    uint32 temp = scripts[i];
	    scripts[i] = scripts[j];
	    scripts[j] = temp;
	}
    }

    if ( sf->sf.cidmaster ) sf = (SplineFont1 *) sf->sf.cidmaster;
    if ( sf->script_lang==NULL )	/* It's an old sfd file */
	sf->script_lang = calloc(1,sizeof(struct script_record *));
    for ( i=0; sf->script_lang[i]!=NULL; ++i ) {
	sr = sf->script_lang[i];
	for ( j=0; sr[j].script!=0 && j<scnt &&
		sr[j].script==scripts[j]; ++j );
	if ( sr[j].script==0 && j==scnt )
return( i );
    }

    sf->script_lang = realloc(sf->script_lang,(i+2)*sizeof(struct script_record *));
    sf->script_lang[i+1] = NULL;
    sr = sf->script_lang[i] = calloc(scnt+1,sizeof(struct script_record));
    for ( j = 0; j<scnt; ++j ) {
	sr[j].script = scripts[j];
	sr[j].langs = malloc(2*sizeof(uint32));
	sr[j].langs[0] = DEFAULT_LANG;
	sr[j].langs[1] = 0;
    }
return( i );
}

static int SFAddScriptLangIndex(SplineFont *_sf,uint32 script,uint32 lang) {
    int i;
    SplineFont1 *sf;

    if ( _sf->cidmaster ) _sf = _sf->cidmaster;
    else if ( _sf->mm!=NULL ) _sf=_sf->mm->normal;

    if ( _sf->sfd_version>=2 )
	IError( "SFFindBiggestScriptLangIndex called with bad version number.\n" );

    sf = (SplineFont1 *) _sf;

    if ( script==0 ) script=DEFAULT_SCRIPT;
    if ( lang==0 ) lang=DEFAULT_LANG;
    if ( sf->script_lang==NULL )
	sf->script_lang = calloc(2,sizeof(struct script_record *));
    for ( i=0; sf->script_lang[i]!=NULL; ++i ) {
	if ( sf->script_lang[i][0].script==script && sf->script_lang[i][1].script==0 &&
		sf->script_lang[i][0].langs[0]==lang &&
		sf->script_lang[i][0].langs[1]==0 )
return( i );
    }
    sf->script_lang = realloc(sf->script_lang,(i+2)*sizeof(struct script_record *));
    sf->script_lang[i] = calloc(2,sizeof(struct script_record));
    sf->script_lang[i][0].script = script;
    sf->script_lang[i][0].langs = malloc(2*sizeof(uint32));
    sf->script_lang[i][0].langs[0] = lang;
    sf->script_lang[i][0].langs[1] = 0;
    sf->script_lang[i+1] = NULL;
    sf->sli_cnt = i+1;
return( i );
}

static int SLCount(struct script_record *sr) {
    int sl_cnt = 0;
    int i,j;

    for ( i=0; sr[i].script!=0; ++i ) {
	for ( j=0; sr[i].langs[j]!=0; ++j )
	    ++sl_cnt;
    }
return( sl_cnt );
}

int SFFindBiggestScriptLangIndex(SplineFont *_sf,uint32 script,uint32 lang) {
    int i, best_sli= -1, best_cnt= -1, cnt;
    SplineFont1 *sf = (SplineFont1 *) _sf;

    if ( _sf->sfd_version>=2 )
	IError( "SFFindBiggestScriptLangIndex called with bad version number.\n" );

    if ( sf->script_lang==NULL )
	SFGuessScriptList(sf);
    for ( i=0; sf->script_lang[i]!=NULL; ++i ) {
	if ( SLContains(sf->script_lang[i],script,lang)) {
	    cnt = SLCount(sf->script_lang[i]);
	    if ( cnt>best_cnt ) {
		best_sli = i;
		best_cnt = cnt;
	    }
	}
    }
    if ( best_sli==-1 )
return( SFAddScriptLangIndex(_sf,script,lang) );

return( best_sli );
}

static FeatureScriptLangList *FeaturesFromTagSli(uint32 tag,int sli,SplineFont1 *sf) {
    FeatureScriptLangList *fl;
    struct script_record *sr;
    struct scriptlanglist *cur, *last;
    int i;

    fl = chunkalloc(sizeof(FeatureScriptLangList));
    fl->featuretag = tag;
    if ( sli==SLI_NESTED || sli<0 || sli>=sf->sli_cnt )
return( fl );
    last = NULL;
    for ( sr = sf->script_lang[sli]; sr->script!=0; ++sr ) {
	cur = chunkalloc(sizeof(struct scriptlanglist));
	cur->script = sr->script;
	for ( i=0; sr->langs[i]!=0; ++i );
	cur->lang_cnt = i;
	if ( i>MAX_LANG )
	    cur->morelangs = malloc((i-MAX_LANG) * sizeof(uint32));
	for ( i=0; sr->langs[i]!=0; ++i ) {
	    if ( i<MAX_LANG )
		cur->langs[i] = sr->langs[i];
	    else
		cur->morelangs[i-MAX_LANG] = sr->langs[i];
	}
	if ( last==NULL )
	    fl->scripts = cur;
	else
	    last->next = cur;
	last = cur;
    }
return( fl );
}

static OTLookup *CreateLookup(SplineFont1 *sf,uint32 tag, int sli,
	int flags,enum possub_type type) {
    OTLookup *otl = chunkalloc(sizeof(OTLookup));

    otl->lookup_type =
	    type == pst_position ? gpos_single :
	    type == pst_pair ? gpos_pair :
	    type == pst_contextpos ? gpos_context :
	    type == pst_chainpos ? gpos_contextchain :
	    type == pst_substitution ? gsub_single :
	    type == pst_alternate ? gsub_alternate :
	    type == pst_multiple ? gsub_multiple :
	    type == pst_ligature ? gsub_ligature :
	    type == pst_contextsub ? gsub_context :
	    type == pst_chainsub ? gsub_contextchain :
		ot_undef;
    if ( otl->lookup_type == ot_undef )
	IError("Unknown lookup type");
    if ( otl->lookup_type<gpos_single ) {
	otl->next = sf->sf.gsub_lookups;
	sf->sf.gsub_lookups = otl;
    } else {
	otl->next = sf->sf.gpos_lookups;
	sf->sf.gpos_lookups = otl;
    }
    otl->lookup_flags = flags;
    otl->features = FeaturesFromTagSli(tag,sli,sf);
    /* We will set the lookup_index after we've ordered the list */
    /* We will set the lookup_name after we've assigned the index */
    /* We will add subtables as we need them */
return( otl );
}

static OTLookup *CreateACLookup(SplineFont1 *sf,AnchorClass1 *ac) {
    OTLookup *otl = chunkalloc(sizeof(OTLookup));

    otl->lookup_type =
	    ac->ac.type == act_mark ? gpos_mark2base :
	    ac->ac.type == act_mkmk ? gpos_mark2mark :
	    ac->ac.type == act_curs ? gpos_cursive :
	    ac->ac.type == act_mklg ? gpos_mark2ligature :
		ot_undef;
    if ( otl->lookup_type == ot_undef )
	IError("Unknown AnchorClass type");
    otl->next = sf->sf.gpos_lookups;
    sf->sf.gpos_lookups = otl;
    otl->lookup_flags = ac->flags;
    otl->features = FeaturesFromTagSli(ac->feature_tag,ac->script_lang_index,sf);
    /* We will set the lookup_index after we've ordered the list */
    /* We will set the lookup_name after we've assigned the index */
    /* We will add one subtable soon */
return( otl );
}

static OTLookup *CreateMacLookup(SplineFont1 *sf,ASM1 *sm) {
    OTLookup *otl = chunkalloc(sizeof(OTLookup));
    int i, ch;
    char *pt, *start;
    SplineChar *sc;

    otl->features = chunkalloc(sizeof(FeatureScriptLangList));
    if ( sm->sm.type == asm_kern ) {
	otl->lookup_type = kern_statemachine;
	otl->next = sf->sf.gpos_lookups;
	sf->sf.gpos_lookups = otl;
	otl->features->featuretag = (sm->sm.flags&0x8000) ? CHR('v','k','r','n') : CHR('k','e','r','n');
    } else {
	otl->lookup_type = sm->sm.type==asm_indic ? morx_indic : sm->sm.type==asm_context ? morx_context : morx_insert;
	otl->next = sf->sf.gsub_lookups;
	sf->sf.gsub_lookups = otl;
	otl->features->featuretag = (sm->feature<<16) | (sm->setting);
	otl->features->ismac = true;
    }
    otl->lookup_flags = 0;

    for ( i=4; i<sm->sm.class_cnt; ++i ) {
	for ( start=sm->sm.classes[i]; ; start = pt ) {
	    while ( *start==' ' ) ++start;
	    if ( *start=='\0' )
	break;
	    for ( pt=start ; *pt!='\0' && *pt!=' '; ++pt );
	    ch = *pt; *pt = '\0';
	    sc = SFGetChar(&sf->sf,-1,start);
	    if ( sc!=NULL )
		FListAppendScriptLang(otl->features,SCScriptFromUnicode(sc),
			DEFAULT_LANG);
	    *pt = ch;
	}
    }
    
    /* We will set the lookup_index after we've ordered the list */
    /* We will set the lookup_name after we've assigned the index */
    /* We will add one subtable soon */
return( otl );
}

static struct lookup_subtable *CreateSubtable(OTLookup *otl,SplineFont1 *sf) {
    struct lookup_subtable *cur, *prev;

    cur = chunkalloc(sizeof(struct lookup_subtable));
    if ( otl->subtables==NULL )
	otl->subtables = cur;
    else {
	for ( prev=otl->subtables; prev->next!=NULL; prev=prev->next );
	prev->next = cur;
    }
    cur->lookup = otl;
    if ( otl->lookup_type == gsub_single ||
	    otl->lookup_type == gsub_multiple ||
	    otl->lookup_type == gsub_alternate ||
	    otl->lookup_type == gsub_ligature ||
	    otl->lookup_type == gpos_single ||
	    otl->lookup_type == gpos_pair )
	cur->per_glyph_pst_or_kern = true;
    else if ( otl->lookup_type == gpos_cursive ||
	    otl->lookup_type == gpos_mark2base ||
	    otl->lookup_type == gpos_mark2ligature ||
	    otl->lookup_type == gpos_mark2mark )
	cur->anchor_classes = true;
    if ( otl->lookup_type == gpos_pair ) {
	if ( otl->features!=NULL &&
		otl->features->featuretag==CHR('v','k','r','n'))
	    cur->vertical_kerning = true;
    }
return( cur );
}

static OTLookup *FindNestedLookupByTag(SplineFont1 *sf,uint32 tag) {
    int isgpos;
    OTLookup *otl;

    for ( isgpos=0; isgpos<2; ++isgpos ) {
	for ( otl = isgpos ? sf->sf.gpos_lookups : sf->sf.gsub_lookups; otl!=NULL; otl=otl->next ) {
	    if ( otl->features!=NULL && otl->features->scripts==NULL &&
		    otl->features->featuretag == tag )
return( otl );
	}
    }
return( NULL );
}

static void FPSTReplaceTagsWithLookups(FPST *fpst,SplineFont1 *sf) {
    int i,j,k;

    if ( fpst->type == pst_reversesub )
return;
    for ( i=0; i<fpst->rule_cnt; ++i ) {
	for ( j=0; j<fpst->rules[i].lookup_cnt; ++j ) {
	    OTLookup *otl = FindNestedLookupByTag(sf,(uint32) (intpt) (fpst->rules[i].lookups[j].lookup) );
	    if ( otl!=NULL )
		fpst->rules[i].lookups[j].lookup = otl;
	    else {
		for ( k=j+1; k<fpst->rules[i].lookup_cnt; ++k )
		    fpst->rules[i].lookups[k-1] = fpst->rules[i].lookups[k];
		--fpst->rules[i].lookup_cnt;
	    }
	}
    }
}

static void ASMReplaceTagsWithLookups(ASM *sm,SplineFont1 *sf) {
    int i;

    if ( sm->type != asm_context )
return;
    for ( i=0; i<sm->class_cnt*sm->state_cnt; ++i ) {
	if ( sm->state[i].u.context.mark_lookup!=NULL )
	    sm->state[i].u.context.mark_lookup = FindNestedLookupByTag(sf,(uint32) (intpt) (sm->state[i].u.context.mark_lookup) );
	if ( sm->state[i].u.context.cur_lookup!=NULL )
	    sm->state[i].u.context.cur_lookup = FindNestedLookupByTag(sf,(uint32) (intpt) (sm->state[i].u.context.cur_lookup) );
    }
}

static void ACHasBaseLig(SplineFont1 *sf,AnchorClass1 *ac) {
    int gid,k;
    SplineFont1 *subsf;
    SplineChar *sc;
    AnchorPoint *ap;

    ac->has_bases = ac->has_ligatures = false;
    if ( ac->ac.type==act_mkmk || ac->ac.type==act_curs )
return;
    k=0;
    do {
	subsf = sf->sf.subfontcnt==0 ? sf : (SplineFont1 *) (sf->sf.subfonts[k]);
	for ( gid=0; gid<subsf->sf.glyphcnt; ++gid ) if ( (sc=subsf->sf.glyphs[gid])!=NULL ) {
	    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
		if ( ap->anchor!=(AnchorClass *) ac )
	    continue;
		if ( ap->type==at_basechar ) {
		    ac->has_bases = true;
		    if ( ac->has_ligatures )
return;
		} else if ( ap->type==at_baselig ) {
		    ac->has_ligatures = true;
		    if ( ac->has_bases )
return;
		}
	    }
	}
	++k;
    } while ( k<sf->sf.subfontcnt );
}

static void ACDisassociateLigatures(SplineFont1 *sf,AnchorClass1 *ac) {
    int gid,k;
    SplineFont1 *subsf;
    SplineChar *sc;
    AnchorPoint *ap, *lap;
    AnchorClass1 *lac;
    char *format;

    lac = chunkalloc(sizeof(AnchorClass1));
    *lac = *ac;
    lac->ac.type = act_mklg;
    ac->ac.next = (AnchorClass *) lac;

  /* GT: Need to split some AnchorClasses into two classes, one for normal */
  /* GT:  base letters, and one for ligatures. So create a new AnchorClass */
  /* GT:  name for the ligature version */
    format = _("Ligature %s");
    lac->ac.name = malloc(strlen(ac->ac.name)+strlen(format)+1);
    sprintf( lac->ac.name, format, ac->ac.name );

    k=0;
    do {
	subsf = sf->sf.subfontcnt==0 ? sf : (SplineFont1 *) (sf->sf.subfonts[k]);
	for ( gid=0; gid<subsf->sf.glyphcnt; ++gid ) if ( (sc=subsf->sf.glyphs[gid])!=NULL ) {
	    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
		if ( ap->anchor!=(AnchorClass *) ac )
	    continue;
		if ( ap->type==at_mark ) {
		    lap = chunkalloc(sizeof(AnchorPoint));
		    *lap = *ap;
		    ap->next = lap;
		    lap->anchor = (AnchorClass *) lac;
		} else if ( ap->type==at_baselig ) {
		    ap->anchor = (AnchorClass *) lac;
		}
	    }
	}
	++k;
    } while ( k<sf->sf.subfontcnt );
}

static int TTFFeatureIndex( uint32 tag, struct table_ordering *ord ) {
    /* This is the order in which features should be executed */
    int cnt = 0;

    if ( ord!=NULL ) {
	for ( cnt=0; ord->ordered_features[cnt]!=0; ++cnt )
	    if ( ord->ordered_features[cnt]==tag )
	break;
return( cnt );
    }

    cnt+=2;

    switch ( tag ) {
/* GSUB ordering */
      case CHR('c','c','m','p'):	/* Must be first? */
return( cnt-2 );
      case CHR('l','o','c','l'):	/* Language dependent letter forms (serbian uses some different glyphs than russian) */
return( cnt-1 );
      case CHR('i','s','o','l'):
return( cnt );
      case CHR('j','a','l','t'):		/* must come after 'isol' */
return( cnt+1 );
      case CHR('f','i','n','a'):
return( cnt+2 );
      case CHR('f','i','n','2'):
      case CHR('f','a','l','t'):		/* must come after 'fina' */
return( cnt+3 );
      case CHR('f','i','n','3'):
return( cnt+4 );
      case CHR('m','e','d','i'):
return( cnt+5 );
      case CHR('m','e','d','2'):
return( cnt+6 );
      case CHR('i','n','i','t'):
return( cnt+7 );

      case CHR('r','t','l','a'):
return( cnt+100 );
      case CHR('s','m','c','p'): case CHR('c','2','s','c'):
return( cnt+200 );

      case CHR('r','l','i','g'):
return( cnt+300 );
      case CHR('c','a','l','t'):
return( cnt+301 );
      case CHR('l','i','g','a'):
return( cnt+302 );
      case CHR('d','l','i','g'): case CHR('h','l','i','g'):
return( cnt+303 );
      case CHR('c','s','w','h'):
return( cnt+304 );
      case CHR('m','s','e','t'):
return( cnt+305 );

      case CHR('f','r','a','c'):
return( cnt+306 );

/* Indic processing */
      case CHR('n','u','k','t'):
      case CHR('p','r','e','f'):
return( cnt+301 );
      case CHR('a','k','h','n'):
return( cnt+302 );
      case CHR('r','p','h','f'):
return( cnt+303 );
      case CHR('b','l','w','f'):
return( cnt+304 );
      case CHR('h','a','l','f'):
      case CHR('a','b','v','f'):
return( cnt+305 );
      case CHR('p','s','t','f'):
return( cnt+306 );
      case CHR('v','a','t','u'):
return( cnt+307 );

      case CHR('p','r','e','s'):
return( cnt+310 );
      case CHR('b','l','w','s'):
return( cnt+311 );
      case CHR('a','b','v','s'):
return( cnt+312 );
      case CHR('p','s','t','s'):
return( cnt+313 );
      case CHR('c','l','i','g'):
return( cnt+314 );
      
      case CHR('h','a','l','n'):
return( cnt+320 );
/* end indic ordering */

      case CHR('a','f','r','c'):
      case CHR('l','j','m','o'):
      case CHR('v','j','m','o'):
return( cnt+350 );
      case CHR('v','r','t','2'): case CHR('v','e','r','t'):
return( cnt+1010 );		/* Documented to come last */

/* Unknown things come after everything but vert/vrt2 */
      default:
return( cnt+1000 );

    }
}

static int GSubOrder(SplineFont1 *sf,FeatureScriptLangList *fl) {
    struct table_ordering *ord;
    int sofar = 30000, temp;

    for ( ord=sf->orders; ord!=NULL && ord->table_tag!=CHR('G','S','U','B');
	    ord = ord->next );
    for ( ; fl!=NULL; fl=fl->next ) {
	temp = TTFFeatureIndex(fl->featuretag,ord);
	if ( temp<sofar )
	    sofar = temp;
    }
return( sofar );
}

static int order_lookups(const void *_otl1, const void *_otl2) {
    const OTLookup *otl1 = *(const OTLookup **) _otl1, *otl2 = *(const OTLookup **) _otl2;
return( otl1->lookup_index - otl2->lookup_index );
}

static void SFDCleanupAnchorClasses(SplineFont *sf) {
    AnchorClass *ac;
    AnchorPoint *ap;
    int i, j, scnt;
#define S_MAX	100
    uint32 scripts[S_MAX];
    int merge=0;

    for ( ac = sf->anchor; ac!=NULL; ac=ac->next ) {
	if ( ((AnchorClass1 *) ac)->script_lang_index==0xffff ) {
	    scnt = 0;
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
		for ( ap = sf->glyphs[i]->anchor; ap!=NULL && ap->anchor!=ac; ap=ap->next );
		if ( ap!=NULL && scnt<S_MAX ) {
		    uint32 script = SCScriptFromUnicode(sf->glyphs[i]);
		    if ( script==0 )
	    continue;
		    for ( j=0; j<scnt; ++j )
			if ( scripts[j]==script )
		    break;
		    if ( j==scnt )
			scripts[scnt++] = script;
		}
	    }
	    ((AnchorClass1 *) ac)->script_lang_index = SFAddScriptIndex((SplineFont1 *) sf,scripts,scnt);
	}
	if ( ((AnchorClass1 *) ac)->merge_with == 0xffff )
	    ((AnchorClass1 *) ac)->merge_with = ++merge;
    }
#undef S_MAX
}

enum uni_interp interp_from_encoding(Encoding *enc,enum uni_interp interp) {

    if ( enc==NULL )
return( interp );

    if ( enc->is_japanese )
	interp = ui_japanese;
    else if ( enc->is_korean )
	interp = ui_korean;
    else if ( enc->is_tradchinese )
	interp = ui_trad_chinese;
    else if ( enc->is_simplechinese )
	interp = ui_simp_chinese;
return( interp );
}

void SFD_AssignLookups(SplineFont1 *sf) {
    PST1 *pst, *pst2;
    int isv;
    KernPair1 *kp, *kp2;
    KernClass1 *kc, *kc2;
    FPST1 *fpst;
    ASM1 *sm;
    AnchorClass1 *ac, *ac2;
    int gid, gid2, cnt, i, k, isgpos;
    SplineFont1 *subsf;
    SplineChar *sc, *sc2;
    OTLookup *otl, **all;
    struct lookup_subtable *sub;

    /* Fix up some gunk from really old versions of the sfd format */
    SFDCleanupAnchorClasses(&sf->sf);
    if ( sf->sf.uni_interp==ui_unset )
	sf->sf.uni_interp = interp_from_encoding(sf->sf.map->enc,ui_none);

    /* Fixup for an old bug */
    if ( sf->sf.pfminfo.os2_winascent < sf->sf.ascent/4 && !sf->sf.pfminfo.winascent_add ) {
	sf->sf.pfminfo.winascent_add = true;
	sf->sf.pfminfo.os2_winascent = 0;
	sf->sf.pfminfo.windescent_add = true;
	sf->sf.pfminfo.os2_windescent = 0;
    }

    /* First handle the PSTs, no complications here */
    k=0;
    do {
	subsf = sf->sf.subfontcnt==0 ? sf : (SplineFont1 *) (sf->sf.subfonts[k]);
	for ( gid=0; gid<subsf->sf.glyphcnt; ++gid ) if ( (sc=subsf->sf.glyphs[gid])!=NULL ) {
	    for ( pst = (PST1 *) (sc->possub); pst!=NULL; pst = (PST1*) (pst->pst.next) ) {
		if ( pst->pst.type == pst_lcaret || pst->pst.subtable!=NULL )
	    continue;		/* Nothing to do, or already done */
		otl = CreateLookup(sf,pst->tag,pst->script_lang_index,pst->flags,pst->pst.type);
		sub = CreateSubtable(otl,sf);
		/* There might be another PST with the same flags on this glyph */
		/* And we must fixup the current pst */
		for ( pst2=pst ; pst2!=NULL; pst2 = (PST1 *) (pst2->pst.next) ) {
		    if ( pst2->tag==pst->tag &&
			    pst2->script_lang_index==pst->script_lang_index &&
			    pst2->flags==pst->flags &&
			    pst2->pst.type==pst->pst.type )
			pst2->pst.subtable = sub;
		}
		for ( gid2=gid+1; gid2<subsf->sf.glyphcnt; ++gid2 ) if ( (sc2=subsf->sf.glyphs[gid2])!=NULL ) {
		    for ( pst2 = (PST1 *) (sc2->possub); pst2!=NULL; pst2 = (PST1 *) (pst2->pst.next) ) {
			if ( pst2->tag==pst->tag &&
				pst2->script_lang_index==pst->script_lang_index &&
				pst2->flags==pst->flags &&
				pst2->pst.type==pst->pst.type )
			    pst2->pst.subtable = sub;
		    }
		}
	    }
	}
	++k;
    } while ( k<sf->sf.subfontcnt );

	/* Now kerns. May need to merge kernclasses to kernpair lookups (different subtables, of course */
    for ( isv=0; isv<2; ++isv ) {
	k=0;
	do {
	    subsf = sf->sf.subfontcnt==0 ? sf : (SplineFont1 *) (sf->sf.subfonts[k]);
	    for ( gid=0; gid<subsf->sf.glyphcnt; ++gid ) if ( (sc=subsf->sf.glyphs[gid])!=NULL ) {
		for ( kp = (KernPair1 *) (isv ? sc->vkerns : sc->kerns); kp!=NULL; kp = (KernPair1 *) (kp->kp.next) ) {
		    if ( kp->kp.subtable!=NULL )
		continue;		/* already done */
		    otl = CreateLookup(sf,isv ? CHR('v','k','r','n') : CHR('k','e','r','n'),
			    kp->sli,kp->flags,pst_pair);
		    sub = CreateSubtable(otl,sf);
		    /* There might be another kp with the same flags on this glyph */
		    /* And we must fixup the current kp */
		    for ( kp2=kp ; kp2!=NULL; kp2 = (KernPair1 *) (kp2->kp.next) ) {
			if ( kp2->sli==kp->sli && kp2->flags==kp->flags )
			    kp2->kp.subtable = sub;
		    }
		    for ( gid2=gid+1; gid2<subsf->sf.glyphcnt; ++gid2 ) if ( (sc2=subsf->sf.glyphs[gid2])!=NULL ) {
			for ( kp2 = (KernPair1 *) (isv ? sc2->vkerns : sc2->kerns); kp2!=NULL; kp2 = (KernPair1 *) (kp2->kp.next) ) {
			    if ( kp2->sli==kp->sli && kp2->flags==kp->flags )
				kp2->kp.subtable = sub;
			}
		    }
		    /* And there might be a kerning class... */
		    for ( kc=(KernClass1 *) (isv ? sf->sf.vkerns : sf->sf.kerns); kc!=NULL;
			    kc = (KernClass1 *) (kc->kc.next) ) {
			if ( kc->sli == kp->sli && kc->flags == kp->flags && kc->kc.subtable==NULL) {
			    sub = CreateSubtable(otl,sf);
			    sub->per_glyph_pst_or_kern = false;
			    sub->kc = &kc->kc;
			    kc->kc.subtable = sub;
			}
		    }
		}
	    }
	    ++k;
	} while ( k<sf->sf.subfontcnt );
	/* Or there might be a kerning class all by its lonesome */
	for ( kc=(KernClass1 *) (isv ? sf->sf.vkerns : sf->sf.kerns); kc!=NULL;
		kc = (KernClass1 *) (kc->kc.next) ) {
	    if ( kc->kc.subtable==NULL) {
		otl = CreateLookup(sf,isv ? CHR('v','k','r','n') : CHR('k','e','r','n'),
			kc->sli,kc->flags,pst_pair);
		for ( kc2=kc; kc2!=NULL; kc2=(KernClass1 *) (kc2->kc.next) ) {
		    if ( kc->sli == kc2->sli && kc->flags == kc2->flags && kc2->kc.subtable==NULL) {
			sub = CreateSubtable(otl,sf);
			sub->per_glyph_pst_or_kern = false;
			sub->kc = &kc2->kc;
			kc2->kc.subtable = sub;
		    }
		}
	    }
	}
    }

    /* Every FPST and ASM lives in its own lookup with one subtable */
    /* But the old format refered to nested lookups by tag, and now we refer */
    /*  to the lookup itself, so fix that up */
    for ( fpst=(FPST1 *) sf->sf.possub; fpst!=NULL; fpst=((FPST1 *) fpst->fpst.next) ) {
	otl = CreateLookup(sf,fpst->tag, fpst->script_lang_index,
		fpst->flags,fpst->fpst.type);
	sub = CreateSubtable(otl,sf);
	sub->per_glyph_pst_or_kern = false;
	sub->fpst = &fpst->fpst;
	fpst->fpst.subtable = sub;
	FPSTReplaceTagsWithLookups(&fpst->fpst,sf);
    }
    for ( sm=(ASM1 *) sf->sf.sm; sm!=NULL; sm=((ASM1 *) sm->sm.next) ) {
	otl = CreateMacLookup(sf,sm);
	sub = CreateSubtable(otl,sf);
	sub->per_glyph_pst_or_kern = false;
	sub->sm = &sm->sm;
	sm->sm.subtable = sub;
	if ( sm->sm.type==asm_context )
	    ASMReplaceTagsWithLookups(&sm->sm,sf);
    }

    /* We retained the old nested feature tags so we could do the above conversion */
    /*  of tag to lookup. Get rid of them now */
    for ( isgpos=0; isgpos<2; ++isgpos ) {
	for ( otl = isgpos ? sf->sf.gpos_lookups : sf->sf.gsub_lookups ;
		otl != NULL; otl=otl->next ) {
	    if ( otl->features!=NULL && otl->features->scripts==NULL ) {
		chunkfree(otl->features,sizeof(FeatureScriptLangList));
		otl->features = NULL;
	    }
	}
    }

    /* Anchor classes are complicated, because I foolishly failed to distinguish */
    /*  between mark to base and mark to ligature classes. So one AC might have */
    /*  both. If so we need to turn it into two ACs, and have separate lookups */
    /*  for each */
    for ( ac=(AnchorClass1 *) (sf->sf.anchor); ac!=NULL; ac=(AnchorClass1 *) ac->ac.next ) {
	ACHasBaseLig(sf,ac);
	if ( ac->has_ligatures && !ac->has_bases )
	    ac->ac.type = act_mklg;
	else if ( ac->has_ligatures && ac->has_bases )
	    ACDisassociateLigatures(sf,ac);
    }
    for ( ac=(AnchorClass1 *) (sf->sf.anchor); ac!=NULL; ac=(AnchorClass1 *) ac->ac.next ) {
	if ( ac->ac.subtable==NULL ) {
	    otl = CreateACLookup(sf,ac);
	    sub = CreateSubtable(otl,sf);
	    for ( ac2=ac; ac2!=NULL; ac2 = (AnchorClass1 *) ac2->ac.next ) {
		if ( ac2->feature_tag == ac->feature_tag &&
			ac2->script_lang_index == ac->script_lang_index &&
			ac2->flags == ac->flags &&
			ac2->ac.type == ac->ac.type &&
			ac2->merge_with == ac->merge_with )
		    ac2->ac.subtable = sub;
	    }
	}
    }

    /* Now I want to order the gsub lookups. I shan't bother with the gpos */
    /*  lookups because I didn't before */
    for ( otl=sf->sf.gsub_lookups, cnt=0; otl!=NULL; otl=otl->next, ++cnt );
    if ( cnt!=0 ) {
	all = malloc(cnt*sizeof(OTLookup *));
	for ( otl=sf->sf.gsub_lookups, cnt=0; otl!=NULL; otl=otl->next, ++cnt ) {
	    all[cnt] = otl;
	    otl->lookup_index = GSubOrder(sf,otl->features);
	}
	qsort(all,cnt,sizeof(OTLookup *),order_lookups);
	sf->sf.gsub_lookups = all[0];
	for ( i=1; i<cnt; ++i )
	    all[i-1]->next = all[i];
	all[cnt-1]->next = NULL;
	free( all );
    }

    for ( isgpos=0; isgpos<2; ++isgpos ) {
	for ( otl = isgpos ? sf->sf.gpos_lookups : sf->sf.gsub_lookups , cnt=0;
		otl!=NULL; otl = otl->next ) {
	    otl->lookup_index = cnt++;
	    NameOTLookup(otl,&sf->sf);
	}
    }
}
