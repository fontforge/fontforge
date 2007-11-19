/* Copyright (C) 2007 by George Williams */
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
#include <chardata.h>
#include <utype.h>
#include <ustring.h>
#include <math.h>
#include <locale.h>
#include <stdlib.h>
#include "ttf.h"
#include "lookups.h"

static int uint32_cmp(const void *_ui1, const void *_ui2) {
    if ( *(uint32 *) _ui1 > *(uint32 *)_ui2 )
return( 1 );
    if ( *(uint32 *) _ui1 < *(uint32 *)_ui2 )
return( -1 );

return( 0 );
}

static int lang_cmp(const void *_ui1, const void *_ui2) {
    /* The default language is magic, and should come first in the list even */
    /*  if that is not true alphabetical order */
    if ( *(uint32 *) _ui1 == DEFAULT_LANG )
return( -1 );
    if ( *(uint32 *) _ui2 == DEFAULT_LANG )
return( 1 );

    if ( *(uint32 *) _ui1 > *(uint32 *)_ui2 )
return( 1 );
    if ( *(uint32 *) _ui1 < *(uint32 *)_ui2 )
return( -1 );

return( 0 );
}

int FeatureTagInFeatureScriptList(uint32 tag, FeatureScriptLangList *fl) {

    while ( fl!=NULL ) {
	if ( fl->featuretag==tag )
return( true );
	fl = fl->next;
    }
return( false );
}

int ScriptInFeatureScriptList(uint32 script, FeatureScriptLangList *fl) {
    struct scriptlanglist *sl;

    while ( fl!=NULL ) {
	for ( sl=fl->scripts; sl!=NULL; sl=sl->next ) {
	    if ( sl->script == script )
return( true );
	}
	fl = fl->next;
    }
return( false );
}

int FeatureScriptTagInFeatureScriptList(uint32 feature, uint32 script, FeatureScriptLangList *fl) {
    struct scriptlanglist *sl;

    while ( fl!=NULL ) {
	if ( fl->featuretag == feature ) {
	    for ( sl=fl->scripts; sl!=NULL; sl=sl->next ) {
		if ( sl->script == script )
return( true );
	    }
	}
	fl = fl->next;
    }
return( false );
}

int DefaultLangTagInOneScriptList(struct scriptlanglist *sl) {
    int l;

    for ( l=0; l<sl->lang_cnt; ++l ) {
	uint32 lang = l<MAX_LANG ? sl->langs[l] : sl->morelangs[l-MAX_LANG];
	if ( lang==DEFAULT_LANG )
return( true );
    }
return( false );
}

struct scriptlanglist *DefaultLangTagInScriptList(struct scriptlanglist *sl, int DFLT_ok) {

    while ( sl!=NULL ) {
	if ( DFLT_ok || sl->script!=DEFAULT_SCRIPT ) {
	    if ( DefaultLangTagInOneScriptList(sl))
return( sl );
	}
	sl = sl->next;
    }
return( NULL );
}

uint32 *SFScriptsInLookups(SplineFont *sf,int gpos) {
    /* Presumes that either SFFindUnusedLookups or SFFindClearUnusedLookupBits */
    /*  has been called first */
    /* Since MS will sometimes ignore a script if it isn't found in both */
    /*  GPOS and GSUB we want to return the same script list no matter */
    /*  what the setting of gpos ... so we totally ignore that argument */
    /*  and always look at both sets of lookups */

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
    int cnt=0, tot=0, i;
    uint32 *scripts = NULL;
    OTLookup *test;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;

    /* So here always give scripts for both (see comment above) no */
    /*  matter what they asked for */
    for ( gpos=0; gpos<2; ++gpos ) {
	for ( test = gpos ? sf->gpos_lookups : sf->gsub_lookups; test!=NULL; test = test->next ) {
	    if ( test->unused )
	continue;
	    for ( fl=test->features; fl!=NULL; fl=fl->next ) {
		if ( fl->ismac )
	    continue;
		for ( sl=fl->scripts ; sl!=NULL; sl=sl->next ) {
		    for ( i=0; i<cnt; ++i ) {
			if ( sl->script==scripts[i] )
		    break;
		    }
		    if ( i==cnt ) {
			if ( cnt>=tot )
			    scripts = grealloc(scripts,(tot+=10)*sizeof(uint32));
			scripts[cnt++] = sl->script;
		    }
		}
	    }
	}
    }

    if ( cnt==0 )
return( NULL );

    /* We want our scripts in alphabetic order */
    qsort(scripts,cnt,sizeof(uint32),uint32_cmp);
    /* add a 0 entry to mark the end of the list */
    if ( cnt>=tot )
	scripts = grealloc(scripts,(tot+1)*sizeof(uint32));
    scripts[cnt] = 0;
return( scripts );
}

uint32 *SFLangsInScript(SplineFont *sf,int gpos,uint32 script) {
    /* However, the language lists (I think) are distinct */
    /* But giving a value of -1 for gpos will give us the set of languages in */
    /*  both tables (for this script) */
    int cnt=0, tot=0, i, g, l;
    uint32 *langs = NULL;
    OTLookup *test;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;

    for ( g=0; g<2; ++g ) {
	if (( gpos==0 && g==1 ) || ( gpos==1 && g==0 ))
    continue;
	for ( test = gpos ? sf->gpos_lookups : sf->gsub_lookups; test!=NULL; test = test->next ) {
	    if ( test->unused )
	continue;
	    for ( fl=test->features; fl!=NULL; fl=fl->next ) {
		for ( sl=fl->scripts ; sl!=NULL; sl=sl->next ) {
		    if ( sl->script==script ) {
			for ( l=0; l<sl->lang_cnt; ++l ) {
			    int lang;
			    if ( l<MAX_LANG )
				lang = sl->langs[l];
			    else
				lang = sl->morelangs[l-MAX_LANG];
			    for ( i=0; i<cnt; ++i ) {
				if ( lang==langs[i] )
			    break;
			    }
			    if ( i==cnt ) {
				if ( cnt>=tot )
				    langs = grealloc(langs,(tot+=10)*sizeof(uint32));
				langs[cnt++] = lang;
			    }
			}
		    }
		}
	    }
	}
    }

    if ( cnt==0 ) {
	/* We add dummy script entries. Because Uniscribe will refuse to */
	/*  process some scripts if they don't have an entry in both GPOS */
	/*  an GSUB. So if a script appears in either table, force it to */
	/*  appear in both. That means we can get scripts with no lookups */
	/*  and hence no languages. It seems that Uniscribe doesn't like */
	/*  that either. So give each such script a dummy default language */
	/*  entry. This is what VOLT does */
	langs = gcalloc(2,sizeof(uint32));
	langs[0] = DEFAULT_LANG;
return( langs );
    }

    /* We want our languages in alphabetic order */
    qsort(langs,cnt,sizeof(uint32),lang_cmp);
    /* add a 0 entry to mark the end of the list */
    if ( cnt>=tot )
	langs = grealloc(langs,(tot+1)*sizeof(uint32));
    langs[cnt] = 0;
return( langs );
}

uint32 *SFFeaturesInScriptLang(SplineFont *sf,int gpos,uint32 script,uint32 lang) {
    int cnt=0, tot=0, i, l, isg;
    uint32 *features = NULL;
    OTLookup *test;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    /* gpos==0 => GSUB, gpos==1 => GPOS, gpos==-1 => both, gpos==-2 => Both & morx & kern */

    if ( sf->cidmaster ) sf=sf->cidmaster;
    for ( isg = 0; isg<2; ++isg ) {
	if ( gpos>=0 && isg!=gpos )
    continue;
	for ( test = isg ? sf->gpos_lookups : sf->gsub_lookups; test!=NULL; test = test->next ) {
	    if ( test->unused )
	continue;
	    for ( fl=test->features; fl!=NULL; fl=fl->next ) {
		if ( fl->ismac && gpos!=-2 )
	    continue;
		if ( script==0xffffffff ) {
		    for ( i=0; i<cnt; ++i ) {
			if ( fl->featuretag==features[i] )
		    break;
		    }
		    if ( i==cnt ) {
			if ( cnt>=tot )
			    features = grealloc(features,(tot+=10)*sizeof(uint32));
			features[cnt++] = fl->featuretag;
		    }
		} else for ( sl=fl->scripts ; sl!=NULL; sl=sl->next ) {
		    if ( sl->script==script ) {
			int matched = false;
			if ( fl->ismac && gpos==-2 )
			    matched = true;
			else for ( l=0; l<sl->lang_cnt; ++l ) {
			    int testlang;
			    if ( l<MAX_LANG )
				testlang = sl->langs[l];
			    else
				testlang = sl->morelangs[l-MAX_LANG];
			    if ( testlang==lang ) {
				matched = true;
			break;
			    }
			}
			if ( matched ) {
			    for ( i=0; i<cnt; ++i ) {
				if ( fl->featuretag==features[i] )
			    break;
			    }
			    if ( i==cnt ) {
				if ( cnt>=tot )
				    features = grealloc(features,(tot+=10)*sizeof(uint32));
				features[cnt++] = fl->featuretag;
			    }
			}
		    }
		}
	    }
	}
    }

    if ( sf->design_size!=0 && gpos ) {
	/* The 'size' feature is like no other. It has no lookups and so */
	/*  we will never find it in the normal course of events. If the */
	/*  user has specified a design size, then every script/lang combo */
	/*  gets a 'size' feature which contains no lookups but feature */
	/*  params */
	if ( cnt>=tot )
	    features = grealloc(features,(tot+=2)*sizeof(uint32));
	features[cnt++] = CHR('s','i','z','e');
    }

    if ( cnt==0 )
return( gcalloc(1,sizeof(uint32)) );

    /* We don't care if our features are in alphabetical order here */
    /*  all that matters is whether the complete list of features is */
    /*  ordering here would be irrelevant */
    /* qsort(features,cnt,sizeof(uint32),uint32_cmp); */

    /* add a 0 entry to mark the end of the list */
    if ( cnt>=tot )
	features = grealloc(features,(tot+1)*sizeof(uint32));
    features[cnt] = 0;
return( features );
}

OTLookup **SFLookupsInScriptLangFeature(SplineFont *sf,int gpos,uint32 script,uint32 lang, uint32 feature) {
    int cnt=0, tot=0, l;
    OTLookup **lookups = NULL;
    OTLookup *test;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;

    for ( test = gpos ? sf->gpos_lookups : sf->gsub_lookups; test!=NULL; test = test->next ) {
	if ( test->unused )
    continue;
	for ( fl=test->features; fl!=NULL; fl=fl->next ) {
	    if ( fl->featuretag==feature ) {
		for ( sl=fl->scripts ; sl!=NULL; sl=sl->next ) {
		    if ( sl->script==script ) {
			for ( l=0; l<sl->lang_cnt; ++l ) {
			    int testlang;
			    if ( l<MAX_LANG )
				testlang = sl->langs[l];
			    else
				testlang = sl->morelangs[l-MAX_LANG];
			    if ( testlang==lang ) {
				if ( cnt>=tot )
				    lookups = grealloc(lookups,(tot+=10)*sizeof(OTLookup *));
				lookups[cnt++] = test;
	goto found;
			    }
			}
		    }
		}
	    }
	}
	found:;
    }

    if ( cnt==0 )
return( NULL );

    /* lookup order is irrelevant here. might as well leave it in invocation order */
    /* add a 0 entry to mark the end of the list */
    if ( cnt>=tot )
	lookups = grealloc(lookups,(tot+1)*sizeof(OTLookup *));
    lookups[cnt] = 0;
return( lookups );
}

static int LigaturesFirstComponentGID(SplineFont *sf,char *components) {
    int gid, ch;
    char *pt;

    for ( pt = components; *pt!='\0' && *pt!=' '; ++pt );
    ch = *pt; *pt = '\0';
    gid = SFFindExistingSlot(sf,-1,components);
    *pt = ch;
return( gid );
}

static int PSTValid(SplineFont *sf,PST *pst) {
    char *start, *pt, ch;
    int ret;

    switch ( pst->type ) {
      case pst_position:
return( true );
      case pst_pair:
return( SCWorthOutputting(SFGetChar(sf,-1,pst->u.pair.paired)) );
      case pst_substitution: case pst_alternate: case pst_multiple:
      case pst_ligature:
	for ( start = pst->u.mult.components; *start ; ) {
	    for ( pt=start; *pt && *pt!=' '; ++pt );
	    ch = *pt; *pt = '\0';
	    ret = SCWorthOutputting(SFGetChar(sf,-1,start));
	    *pt = ch;
	    if ( !ret )
return( false );
	    if ( ch==0 )
		start = pt;
	    else
		start = pt+1;
	}
    }
return( true );
}

SplineChar **SFGlyphsWithPSTinSubtable(SplineFont *sf,struct lookup_subtable *subtable) {
    uint8 *used = gcalloc(sf->glyphcnt,sizeof(uint8));
    SplineChar **glyphs, *sc;
    int i, k, gid, cnt;
    KernPair *kp;
    PST *pst;
    int ispair = subtable->lookup->lookup_type == gpos_pair;
    int isliga = subtable->lookup->lookup_type == gsub_ligature;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sc = sf->glyphs[i]) ) {
	if ( ispair ) {
	    for ( k=0; k<2; ++k ) {
		for ( kp= k ? sc->kerns : sc->vkerns; kp!=NULL ; kp=kp->next ) {
		    if ( !SCWorthOutputting(kp->sc))
		continue;
		    if ( kp->subtable == subtable ) {
			used[i] = true;
    goto continue_;
		    }
		}
	    }
	}
	for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->subtable == subtable && PSTValid(sf,pst)) {
		if ( !isliga ) {
		    used[i] = true;
    goto continue_;
		} else {
		    gid = LigaturesFirstComponentGID(sf,pst->u.lig.components);
		    pst->u.lig.lig = sc;
		    if ( gid!=-1 )
			used[gid] = true;
		    /* can't continue here. ffi might be "f+f+i" and "ff+i" */
		    /*  and we need to mark both "f" and "ff" as used */
		}
	    }
	}
    continue_: ;
    }

    for ( i=cnt=0 ; i<sf->glyphcnt; ++i )
	if ( used[i] )
	    ++cnt;

    if ( cnt==0 ) {
	free(used);
return( NULL );
    }
    glyphs = galloc((cnt+1)*sizeof(SplineChar *));
    for ( i=cnt=0 ; i<sf->glyphcnt; ++i ) {
	if ( used[i] )
	    glyphs[cnt++] = sf->glyphs[i];
    }
    glyphs[cnt] = NULL;
    free(used);
return( glyphs );
}

SplineChar **SFGlyphsWithLigatureinLookup(SplineFont *sf,struct lookup_subtable *subtable) {
    uint8 *used = gcalloc(sf->glyphcnt,sizeof(uint8));
    SplineChar **glyphs, *sc;
    int i, cnt;
    PST *pst;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sc = sf->glyphs[i]) ) {
	for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->subtable == subtable ) {
		used[i] = true;
    goto continue_;
	    }
	}
    continue_: ;
    }

    for ( i=cnt=0 ; i<sf->glyphcnt; ++i )
	if ( used[i] )
	    ++cnt;

    if ( cnt==0 ) {
	free(used);
return( NULL );
    }

    glyphs = galloc((cnt+1)*sizeof(SplineChar *));
    for ( i=cnt=0 ; i<sf->glyphcnt; ++i ) {
	if ( used[i] )
	    glyphs[cnt++] = sf->glyphs[i];
    }
    glyphs[cnt] = NULL;
    free(used);
return( glyphs );
}

void SFFindUnusedLookups(SplineFont *sf) {
    OTLookup *test;
    struct lookup_subtable *sub;
    int gpos;
    AnchorClass *ac;
    AnchorPoint *ap;
    SplineChar *sc;
    KernPair *kp;
    PST *pst;
    int k,gid,isv;
    SplineFont *_sf = sf;

    if ( _sf->cidmaster ) _sf = _sf->cidmaster;

    /* Some things are obvious. If a subtable consists of a kernclass or some */
    /*  such, then obviously it is used. But more distributed info takes more */
    /*  work. So mark anything easy as used, and anything difficult as unused */
    /* We'll work on the difficult things later */
    for ( gpos=0; gpos<2; ++gpos ) {
	for ( test = gpos ? _sf->gpos_lookups : _sf->gsub_lookups; test!=NULL; test = test->next ) {
	    for ( sub = test->subtables; sub!=NULL; sub=sub->next ) {
		if ( sub->kc!=NULL || sub->fpst!=NULL || sub->sm!=NULL ) {
		    sub->unused = false;
	    continue;
		}
		sub->unused = true;
		/* We'll turn the following bit back on if there turns out */
		/*  to be an anchor class attached to it -- that is subtly */
		/*  different than being unused -- unused will be set if all */
		/*  acs are unused, this bit will be on if there are unused */
		/*  classes that still refer to us. */
		sub->anchor_classes = false;
	    }
	}
    }

    /* To be useful an anchor class must have both at least one base and one mark */
    /*  (for cursive anchors that means at least one entry and at least one exit) */
    /* Start by assuming the worst */
    for ( ac = _sf->anchor; ac!=NULL; ac=ac->next )
	ac->has_mark = ac->has_base = false;

    /* Ok, for each glyph, look at all lookups (or anchor classes) it affects */
    /*  and mark the appropriate parts of them as used */
    k = 0;
    do {
	sf = _sf->subfontcnt==0 ? _sf : _sf->subfonts[k];
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( SCWorthOutputting(sc = sf->glyphs[gid]) ) {
	    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
		switch ( ap->type ) {
		  case at_mark: case at_centry:
		    ap->anchor->has_mark = true;
		  break;
		  case at_basechar: case at_baselig: case at_basemark:
		  case at_cexit:
		    ap->anchor->has_base = true;
		  break;
		}
	    }
	    for ( isv=0; isv<2; ++isv ) {
		for ( kp= isv ? sc->kerns : sc->vkerns ; kp!=NULL; kp=kp->next ) {
		    if ( SCWorthOutputting(kp->sc))
			kp->subtable->unused = false;
		}
	    }
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->subtable==NULL )
	    continue;
		if ( !PSTValid(sf,pst))
	    continue;
		pst->subtable->unused = false;
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );

    /* Finally for any anchor class that has both a mark and a base then it is */
    /*  used, and its lookup is also used */
    /* Also, even if unused, as long as the anchor class exists we must keep */
    /*  the subtable around */
    for ( ac = _sf->anchor; ac!=NULL; ac=ac->next ) {
	ac->subtable->anchor_classes = true;
	if ( ac->has_mark && ac->has_base )
	    ac->subtable->unused = false;
    }

    /* Now for each lookup, a lookup is unused if ALL subtables are unused */
    for ( gpos=0; gpos<2; ++gpos ) {
	for ( test = gpos ? _sf->gpos_lookups : _sf->gsub_lookups; test!=NULL; test = test->next ) {
	    test->unused = test->empty = true;
	    for ( sub=test->subtables; sub!=NULL; sub=sub->next ) {
		if ( !sub->unused )
		    test->unused = false;
		if ( !sub->unused && !sub->anchor_classes ) {
		    test->empty = false;
	    break;
		}
	    }
	}
    }
}

void SFFindClearUnusedLookupBits(SplineFont *sf) {
    OTLookup *test;
    int gpos;

    for ( gpos=0; gpos<2; ++gpos ) {
	for ( test = gpos ? sf->gpos_lookups : sf->gsub_lookups; test!=NULL; test = test->next ) {
	    test->unused = false;
	    test->empty = false;
	    test->def_lang_checked = false;
	}
    }
}

static void SFRemoveAnchorPointsOfAC(SplineFont *sf,AnchorClass *ac) {
    int gid;
    SplineChar *sc;
    AnchorPoint *ap, *prev, *next;

    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc = sf->glyphs[gid])!=NULL ) {
	for ( prev=NULL, ap=sc->anchor; ap!=NULL; ap=next ) {
	    next = ap->next;
	    if ( ap->anchor!=ac )
		prev = ap;
	    else {
		if ( prev==NULL )
		    sc->anchor = next;
		else
		    prev->next = next;
		ap->next = NULL;
		AnchorPointsFree(ap);
	    }
	}
    }
}

static void RemoveNestedReferences(SplineFont *sf,int isgpos,OTLookup *dying) {
    OTLookup *otl;
    struct lookup_subtable *sub;
    int i,j,k;

    for ( otl = isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl = otl->next ) {
	if ( otl->lookup_type==morx_context ) {
	    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
		ASM *sm = sub->sm;
		if ( sm->type==asm_context ) {
		    for ( i=0; i<sm->state_cnt*sm->class_cnt; ++i ) {
			struct asm_state *state = &sm->state[i];
			if ( state->u.context.mark_lookup == otl )
			    state->u.context.mark_lookup = NULL;
			if ( state->u.context.cur_lookup == otl )
			    state->u.context.cur_lookup = NULL;
		    }
		}
	    }
	/* Reverse chaining tables do not reference lookups. The match pattern*/
	/*  is a (exactly one) coverage table, and each glyph in that table   */
	/*  as an inline replacement. There is no lookup to do the replacement*/
	/* (so we ignore it here) */
	} else if ( otl->lookup_type==gsub_context || otl->lookup_type==gsub_contextchain ||
		otl->lookup_type==gpos_context || otl->lookup_type==gpos_contextchain ) {
	    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
		FPST *fpst = sub->fpst;
		for ( i=0; i<fpst->rule_cnt; ++i ) {
		    for ( j=0; j<fpst->rules[i].lookup_cnt; ++j ) {
			if ( fpst->rules[i].lookups[j].lookup == otl ) {
			    for ( k=j+1; k<fpst->rules[i].lookup_cnt; ++k )
				fpst->rules[i].lookups[k-1] = fpst->rules[i].lookups[k];
			    --fpst->rules[i].lookup_cnt;
			    --j;
			}
		    }
		}
	    }
	}
    }
}

void SFRemoveUnusedLookupSubTables(SplineFont *sf,
	int remove_incomplete_anchorclasses,
	int remove_unused_lookups) {
    int gpos;
    struct lookup_subtable *sub, *subnext, *prev;
    AnchorClass *ac, *acprev, *acnext;
    OTLookup *otl, *otlprev, *otlnext;

    /* Presumes someone has called SFFindUnusedLookups first */

    if ( remove_incomplete_anchorclasses ) {
	for ( acprev=NULL, ac=sf->anchor; ac!=NULL; ac=acnext ) {
	    acnext = ac->next;
	    if ( ac->has_mark && ac->has_base )
		acprev = ac;
	    else {
		SFRemoveAnchorPointsOfAC(sf,ac);
		ac->next = NULL;
		AnchorClassesFree(ac);
		if ( acprev==NULL )
		    sf->anchor = acnext;
		else
		    acprev = acnext;
	    }
	}
    }

    for ( gpos=0; gpos<2; ++gpos ) {
	for ( otl = gpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl = otlnext ) {
	    otlnext = otl->next;
	    if ( remove_unused_lookups && (otl->empty ||
		    (otl->unused && remove_incomplete_anchorclasses)) ) {
		if ( otlprev!=NULL )
		    otlprev->next = otlnext;
		else if ( gpos )
		    sf->gpos_lookups = otlnext;
		else
		    sf->gsub_lookups = otlnext;
		RemoveNestedReferences(sf,gpos,otl);
		OTLookupFree(otl);
	    } else {
		for ( prev=NULL, sub=otl->subtables; sub!=NULL; sub=subnext ) {
		    subnext = sub->next;
		    if ( sub->unused &&
			    (!sub->anchor_classes ||
			      remove_incomplete_anchorclasses )) {
			if ( prev==NULL )
			    otl->subtables = subnext;
			else
			    prev->next = subnext;
			free(sub->subtable_name);
			chunkfree(sub,sizeof(*sub));
		    } else
			prev = sub;
		}
	    }
	}
    }
}

void SFRemoveLookupSubTable(SplineFont *sf,struct lookup_subtable *sub) {
    OTLookup *otl = sub->lookup;
    struct lookup_subtable *subprev, *subtest;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;

    if ( sub->sm!=NULL ) {
	ASM *prev = NULL, *test;
	for ( test=sf->sm; test!=NULL && test!=sub->sm; prev=test, test=test->next );
	if ( prev==NULL )
	    sf->sm = sub->sm->next;
	else
	    prev->next = sub->sm->next;
	sub->sm->next = NULL;
	ASMFree(sub->sm);
	sub->sm = NULL;
    } else if ( sub->fpst!=NULL ) {
	FPST *prev = NULL, *test;
	for ( test=sf->possub; test!=NULL && test!=sub->fpst; prev=test, test=test->next );
	if ( prev==NULL )
	    sf->possub = sub->fpst->next;
	else
	    prev->next = sub->fpst->next;
	sub->fpst->next = NULL;
	FPSTFree(sub->fpst);
	sub->fpst = NULL;
    } else if ( sub->kc!=NULL ) {
	KernClass *prev = NULL, *test;
	for ( test=sf->kerns; test!=NULL && test!=sub->kc; prev=test, test=test->next );
	if ( test!=NULL ) {
	    if ( prev==NULL )
		sf->kerns = sub->kc->next;
	    else
		prev->next = sub->kc->next;
	} else {
	    for ( prev=NULL,test=sf->vkerns; test!=NULL && test!=sub->kc; prev=test, test=test->next );
	    if ( prev==NULL )
		sf->vkerns = sub->kc->next;
	    else
		prev->next = sub->kc->next;
	}
	sub->kc->next = NULL;
	KernClassListFree(sub->kc);
	sub->kc = NULL;
    } else if ( otl->lookup_type==gpos_cursive || otl->lookup_type==gpos_mark2base ||
	    otl->lookup_type==gpos_mark2ligature || otl->lookup_type==gpos_mark2mark ) {
	AnchorClass *ac, *acnext;
	for ( ac=sf->anchor; ac!=NULL; ac=acnext ) {
	    acnext = ac->next;
	    if ( ac->subtable==sub )
		SFRemoveAnchorClass(sf,ac);
	}
    } else {
	int i,k,v;
	SplineChar *sc;
	SplineFont *_sf;
	PST *pst, *prev, *next;
	KernPair *kp, *kpprev, *kpnext;
	k=0;
	do {
	    _sf = sf->subfontcnt==0 ? sf : sf->subfonts[i];
	    for ( i=0; i<_sf->glyphcnt; ++i ) if ( (sc=_sf->glyphs[i])!=NULL ) {
		for ( pst=sc->possub, prev=NULL ; pst!=NULL; pst=next ) {
		    next = pst->next;
		    if ( pst->subtable==sub ) {
			if ( prev==NULL )
			    sc->possub = next;
			else
			    prev->next = next;
			pst->next = NULL;
			PSTFree(pst);
		    } else
			prev = pst;
		}
		for ( v=0; v<2; ++v ) {
		    for ( kp=v ? sc->vkerns : sc->kerns, kpprev=NULL ; kp!=NULL; kp=kpnext ) {
			kpnext = kp->next;
			if ( kp->subtable==sub ) {
			    if ( kpprev!=NULL )
				kpprev->next = kpnext;
			    else if ( v )
				sc->vkerns = kpnext;
			    else
				sc->kerns = kpnext;
			    kp->next = NULL;
			    KernPairsFree(kp);
			} else
			    kpprev = kp;
		    }
		}
	    }
	    ++k;
	} while ( k<sf->subfontcnt );
    }

    subprev = NULL;
    for ( subtest = otl->subtables; subtest!=NULL && subtest!=sub; subprev = subtest, subtest=subtest->next );
    if ( subprev==NULL )
	otl->subtables = sub->next;
    else
	subprev->next = sub->next;
    free(sub->subtable_name);
    free(sub->suffix);
    chunkfree(sub,sizeof(struct lookup_subtable));
}
	
void SFRemoveLookup(SplineFont *sf,OTLookup *otl) {
    OTLookup *test, *prev;
    int isgpos;
    struct lookup_subtable *sub, *subnext;

    if ( sf->cidmaster ) sf = sf->cidmaster;

    for ( sub = otl->subtables; sub!=NULL; sub=subnext ) {
	subnext = sub->next;
	SFRemoveLookupSubTable(sf,sub);
    }

    for ( prev=NULL, test=sf->gpos_lookups; test!=NULL && test!=otl; prev=test, test=test->next );
    if ( test==NULL ) {
	isgpos = false;
	for ( prev=NULL, test=sf->gsub_lookups; test!=NULL && test!=otl; prev=test, test=test->next );
    } else
	isgpos = true;
    if ( prev!=NULL )
	prev->next = otl->next;
    else if ( isgpos )
	sf->gpos_lookups = otl->next;
    else
	sf->gsub_lookups = otl->next;

    RemoveNestedReferences(sf,isgpos,otl);

    otl->next = NULL;
    OTLookupFree(otl);
}

struct lookup_subtable *SFFindLookupSubtable(SplineFont *sf,char *name) {
    int isgpos;
    OTLookup *otl;
    struct lookup_subtable *sub;

    if ( sf->cidmaster ) sf = sf->cidmaster;

    if ( name==NULL )
return( NULL );

    for ( isgpos=0; isgpos<2; ++isgpos ) {
	for ( otl = isgpos ? sf->gpos_lookups : sf->gsub_lookups ; otl!=NULL; otl=otl->next ) {
	    for ( sub = otl->subtables; sub!=NULL; sub=sub->next ) {
		if ( strcmp(name,sub->subtable_name)==0 )
return( sub );
	    }
	}
    }
return( NULL );
}

struct lookup_subtable *SFFindLookupSubtableAndFreeName(SplineFont *sf,char *name) {
    struct lookup_subtable *sub = SFFindLookupSubtable(sf,name);
    free(name);
return( sub );
}

OTLookup *SFFindLookup(SplineFont *sf,char *name) {
    int isgpos;
    OTLookup *otl;

    if ( sf->cidmaster ) sf = sf->cidmaster;

    if ( name==NULL )
return( NULL );

    for ( isgpos=0; isgpos<2; ++isgpos ) {
	for ( otl = isgpos ? sf->gpos_lookups : sf->gsub_lookups ; otl!=NULL; otl=otl->next ) {
	    if ( strcmp(name,otl->lookup_name)==0 )
return( otl );
	}
    }
return( NULL );
}

void FListAppendScriptLang(FeatureScriptLangList *fl,uint32 script_tag,uint32 lang_tag) {
    struct scriptlanglist *sl;
    int l;

    for ( sl = fl->scripts; sl!=NULL && sl->script!=script_tag; sl=sl->next );
    if ( sl==NULL ) {
	sl = chunkalloc(sizeof(struct scriptlanglist));
	sl->script = script_tag;
	sl->next = fl->scripts;
	fl->scripts = sl;
    }
    for ( l=0; l<MAX_LANG && l<sl->lang_cnt && sl->langs[l]!=lang_tag; ++l );
    if ( l>=MAX_LANG && l<sl->lang_cnt ) {
	while ( l<sl->lang_cnt && sl->morelangs[l-MAX_LANG]!=lang_tag )
	    ++l;
    }
    if ( l>=sl->lang_cnt ) {
	if ( l<MAX_LANG )
	    sl->langs[l] = lang_tag;
	else {
	    if ( l%MAX_LANG == 0 )
		sl->morelangs = grealloc(sl->morelangs,l*sizeof(uint32));
		/* We've just allocated MAX_LANG-1 more than we need */
		/*  so we don't do quite some many allocations */
	    sl->morelangs[l-MAX_LANG] = lang_tag;
	}
	++sl->lang_cnt;
    }
}

void FListsAppendScriptLang(FeatureScriptLangList *fl,uint32 script_tag,uint32 lang_tag) {
    while ( fl!=NULL ) {
	FListAppendScriptLang(fl,script_tag,lang_tag);
	fl=fl->next;
    }
}

char *SuffixFromTags(FeatureScriptLangList *fl) {
    static struct { uint32 tag; char *suffix; } tags2suffix[] = {
	{ CHR('v','r','t','2'), "vert" },	/* Will check for vrt2 later */
	{ CHR('o','n','u','m'), "oldstyle" },
	{ CHR('s','u','p','s'), "superior" },
	{ CHR('s','u','b','s'), "inferior" },
	{ CHR('s','w','s','h'), "swash" },
	{ CHR('f','w','i','d'), "full" },
	{ CHR('h','w','i','d'), "hw" },
	{ 0 }
	};
    int i;

    while ( fl!=NULL ) {
	for ( i=0; tags2suffix[i].tag!=0; ++i )
	    if ( tags2suffix[i].tag==fl->featuretag )
return( copy( tags2suffix[i].suffix ));
	fl = fl->next;
    }
return( NULL );
}

char *lookup_type_names[2][10] =
	    { { N_("Undefined substitution"), N_("Single Substitution"), N_("Multiple Substitution"),
		N_("Alternate Substitution"), N_("Ligature Substitution"), N_("Contextual Substitution"),
		N_("Contextual Chaining Substitution"), N_("Extension"),
		N_("Reverse Contextual Chaining Substitution") },
	      { N_("Undefined positioning"), N_("Single Positioning"), N_("Pairwise Positioning (kerning)"),
		N_("Cursive attachment"), N_("Mark to base attachment"),
		N_("Mark to Ligature attachment"), N_("Mark to Mark attachment"),
		N_("Contextual Positioning"), N_("Contextual Chaining Positioning"),
		N_("Extension") }};

void NameOTLookup(OTLookup *otl,SplineFont *sf) {
    char *userfriendly = NULL, *script;
    FeatureScriptLangList *fl;
    char *lookuptype;
    extern GTextInfo scripts[];
    char *format;
    struct lookup_subtable *subtable;
    int k;

    if ( otl->lookup_name==NULL ) {
	for ( k=0; k<2; ++k ) {
	    for ( fl=otl->features; fl!=NULL ; fl=fl->next ) {
		/* look first for a feature attached to a default language */
		if ( k==1 || DefaultLangTagInScriptList(fl->scripts,false)!=NULL ) {
		    userfriendly = TagFullName(sf,fl->featuretag, fl->ismac, true);
		    if ( userfriendly!=NULL )
	    break;
		}
	    }
	    if ( userfriendly!=NULL )
	break;
	}
	if ( userfriendly==NULL ) {
	    if ( (otl->lookup_type&0xff)>= 0xf0 )
		lookuptype = _("State Machine");
	    else if ( (otl->lookup_type>>8)<2 && (otl->lookup_type&0xff)<10 )
		lookuptype = _(lookup_type_names[otl->lookup_type>>8][otl->lookup_type&0xff]);
	    else
		lookuptype = S_("LookupType|Unknown");
	    for ( fl=otl->features; fl!=NULL && !fl->ismac; fl=fl->next );
	    if ( fl==NULL )
		userfriendly = copy(lookuptype);
	    else {
		userfriendly = galloc( strlen(lookuptype) + 10);
		sprintf( userfriendly, "%s '%c%c%c%c'", lookuptype,
		    fl->featuretag>>24,
		    fl->featuretag>>16,
		    fl->featuretag>>8 ,
		    fl->featuretag );
	    }
	}
	script = NULL;
	if ( fl==NULL ) fl = otl->features;
	if ( fl!=NULL && fl->scripts!=NULL ) {
	    char buf[8];
	    int j;
	    struct scriptlanglist *sl, *found, *found2;
	    uint32 script_tag = fl->scripts->script;
	    found = found2 = NULL;
	    for ( sl = fl->scripts; sl!=NULL; sl=sl->next ) {
		if ( sl->script == DEFAULT_SCRIPT )
		    /* Ignore it */;
		else if ( DefaultLangTagInOneScriptList(sl)) {
		    if ( found==NULL )
			found = sl;
		    else {
			found = found2 = NULL;
	    break;
		    }
		} else if ( found2 == NULL )
		    found2 = sl;
		else
		    found2 = (struct scriptlanglist *) -1;
	    }
	    if ( found==NULL && found2!=NULL && found2 != (struct scriptlanglist *) -1 )
		found = found2;
	    if ( found!=NULL ) {
		script_tag = found->script;
		for ( j=0; scripts[j].text!=NULL && script_tag!=(uint32) (intpt) scripts[j].userdata; ++j );
		if ( scripts[j].text!=NULL )
		    script = copy( S_((char *) scripts[j].text) );
		else {
		    buf[0] = '\'';
		    buf[1] = fl->scripts->script>>24;
		    buf[2] = (fl->scripts->script>>16)&0xff;
		    buf[3] = (fl->scripts->script>>8)&0xff;
		    buf[4] = fl->scripts->script&0xff;
		    buf[5] = '\'';
		    buf[6] = 0;
		    script = copy(buf);
		}
	    }
	}
/* GT: This string is used to generate a name for each OpenType lookup. */
/* GT: The %s will be filled with the user friendly name of the feature used to invoke the lookup */
/* GT: The second %s (if present) is the script */
/* GT: While the %d is the index into the lookup list and is used to disambiguate it */
/* GT: In case that is needed */
	if ( script!=NULL ) {
	    format = _("%s in %s lookup %d");
	    otl->lookup_name = galloc( strlen(userfriendly)+strlen(format)+strlen(script)+10 );
	    sprintf( otl->lookup_name, format, userfriendly, script, otl->lookup_index );
	} else {
	    format = _("%s lookup %d");
	    otl->lookup_name = galloc( strlen(userfriendly)+strlen(format)+10 );
	    sprintf( otl->lookup_name, format, userfriendly, otl->lookup_index );
	}
	free(script);
	free(userfriendly);
    }

    if ( otl->subtables==NULL )
	/* IError( _("Lookup with no subtables"))*/;
    else {
	int cnt = 0;
	for ( subtable = otl->subtables; subtable!=NULL; subtable=subtable->next, ++cnt )
		if ( subtable->subtable_name==NULL ) {
	    if ( subtable==otl->subtables && subtable->next==NULL )
/* GT: This string is used to generate a name for an OpenType lookup subtable. */
/* GT:  %s is the lookup name */
		format = _("%s subtable");
	    else if ( subtable->per_glyph_pst_or_kern )
/* GT: This string is used to generate a name for an OpenType lookup subtable. */
/* GT:  %s is the lookup name, %d is the index of the subtable in the lookup */
		format = _("%s per glyph data %d");
	    else if ( subtable->kc!=NULL )
		format = _("%s kerning class %d");
	    else if ( subtable->fpst!=NULL )
		format = _("%s contextual %d");
	    else if ( subtable->anchor_classes )
		format = _("%s anchor %d");
	    else {
		IError("Subtable status not filled in for %dth subtable of %s", cnt, otl->lookup_name );
		format = "%s !!!!!!!! %d";
	    }
	    subtable->subtable_name = galloc( strlen(otl->lookup_name)+strlen(format)+10 );
	    sprintf( subtable->subtable_name, format, otl->lookup_name, cnt );
	}
    }
    if ( otl->lookup_type==gsub_ligature ) {
	for ( fl=otl->features; fl!=NULL; fl=fl->next )
	    if ( fl->featuretag==CHR('l','i','g','a') || fl->featuretag==CHR('r','l','i','g'))
		otl->store_in_afm = true;
    }

    if ( otl->lookup_type==gsub_single )
	for ( subtable = otl->subtables; subtable!=NULL; subtable=subtable->next )
	    subtable->suffix = SuffixFromTags(otl->features);
}

static void LangOrder(struct scriptlanglist *sl) {
    int i,j;
    uint32 lang, lang2;

    for ( i=0; i<sl->lang_cnt; ++i ) {
	lang = i<MAX_LANG ? sl->langs[i] : sl->morelangs[i-MAX_LANG];
	for ( j=i+1; j<sl->lang_cnt; ++j ) {
	    lang2 = j<MAX_LANG ? sl->langs[j] : sl->morelangs[j-MAX_LANG];
	    if ( lang>lang2 ) {
		if ( i<MAX_LANG )
		    sl->langs[i] = lang2;
		else
		    sl->morelangs[i-MAX_LANG] = lang2;
		if ( j<MAX_LANG )
		    sl->langs[j] = lang;
		else
		    sl->morelangs[j-MAX_LANG] = lang;
		lang = lang2;
	    }
	}
    }
}
	
static struct scriptlanglist *SLOrder(struct scriptlanglist *sl) {
    int i,j, cnt;
    struct scriptlanglist *sl2, *space[30], **allocked=NULL, **test = space;

    for ( sl2=sl, cnt=0; sl2!=NULL; sl2=sl2->next, ++cnt )
	LangOrder(sl2);
    if ( cnt<=1 )
return( sl );
    if ( cnt>30 )
	test = allocked = galloc(cnt*sizeof(struct scriptlanglist *));
    for ( sl2=sl, cnt=0; sl2!=NULL; sl2=sl2->next, ++cnt )
	test[cnt] = sl2;
    for ( i=0; i<cnt; ++i ) for ( j=i+1; j<cnt; ++j ) {
	if ( test[i]->script > test[j]->script ) {
	    struct scriptlanglist *temp;
	    temp = test[i];
	    test[i] = test[j];
	    test[j] = temp;
	}
    }
    sl = test[0];
    for ( i=1; i<cnt; ++i )
	test[i-1]->next = test[i];
    test[i-1]->next = NULL;
    free( allocked );
return( sl );
}
    
FeatureScriptLangList *FLOrder(FeatureScriptLangList *fl) {
    int i,j, cnt;
    FeatureScriptLangList *fl2, *space[30], **allocked=NULL, **test = space;

    for ( fl2=fl, cnt=0; fl2!=NULL; fl2=fl2->next, ++cnt )
	fl2->scripts = SLOrder(fl2->scripts);
    if ( cnt<=1 )
return( fl );
    if ( cnt>30 )
	test = allocked = galloc(cnt*sizeof(FeatureScriptLangList *));
    for ( fl2=fl, cnt=0; fl2!=NULL; fl2=fl2->next, ++cnt )
	test[cnt] = fl2;
    for ( i=0; i<cnt; ++i ) for ( j=i+1; j<cnt; ++j ) {
	if ( test[i]->featuretag > test[j]->featuretag ) {
	    FeatureScriptLangList *temp;
	    temp = test[i];
	    test[i] = test[j];
	    test[j] = temp;
	}
    }
    fl = test[0];
    for ( i=1; i<cnt; ++i )
	test[i-1]->next = test[i];
    test[i-1]->next = NULL;
    free( allocked );
return( fl );
}

struct scriptlanglist *SLCopy(struct scriptlanglist *sl) {
    struct scriptlanglist *newsl;

    newsl = chunkalloc(sizeof(struct scriptlanglist));
    *newsl = *sl;
    newsl->next = NULL;

    if ( sl->lang_cnt>MAX_LANG ) {
	newsl->morelangs = galloc((newsl->lang_cnt-MAX_LANG)*sizeof(uint32));
	memcpy(newsl->morelangs,sl->morelangs,(newsl->lang_cnt-MAX_LANG)*sizeof(uint32));
    }
return( newsl );
}

FeatureScriptLangList *FeatureListCopy(FeatureScriptLangList *fl) {
    FeatureScriptLangList *newfl;
    struct scriptlanglist *sl, *prev;

    if ( fl==NULL )
return( NULL );

    newfl = chunkalloc(sizeof(FeatureScriptLangList));
    *newfl = *fl;
    newfl->next = NULL;

    prev = NULL;
    for ( sl=fl->scripts; sl!=NULL; sl=sl->next ) {
	if ( prev==NULL )
	    newfl->scripts = prev = SLCopy(sl);
	else {
	    prev->next = SLCopy(sl);
	    prev = prev->next;
	}
    }
return( newfl );
}
    
static void LangMerge(struct scriptlanglist *into, struct scriptlanglist *from) {
    int i,j;
    uint32 flang, tlang;

    for ( i=0 ; i<from->lang_cnt; ++i ) {
	flang = i<MAX_LANG ? from->langs[i] : from->morelangs[i-MAX_LANG];
	for ( j=0; j<into->lang_cnt; ++j ) {
	    tlang = j<MAX_LANG ? into->langs[j] : into->morelangs[j-MAX_LANG];
	    if ( tlang==flang )
	break;
	}
	if ( j==into->lang_cnt ) {
	    if ( into->lang_cnt<MAX_LANG )
		into->langs[into->lang_cnt++] = flang;
	    else {
		into->morelangs = grealloc(into->morelangs,(into->lang_cnt+1-MAX_LANG)*sizeof(uint32));
		into->morelangs[into->lang_cnt++-MAX_LANG] = flang;
	    }
	}
    }
}

void SLMerge(FeatureScriptLangList *into, struct scriptlanglist *fsl) {
    struct scriptlanglist *isl;

    for ( ; fsl!=NULL; fsl = fsl->next ) {
	for ( isl=into->scripts; isl!=NULL; isl=isl->next ) {
	    if ( fsl->script==isl->script )
	break;
	}
	if ( isl!=NULL )
	    LangMerge(fsl,isl);
	else {
	    isl = SLCopy(fsl);
	    isl->next = into->scripts;
	    into->scripts = isl;
	}
    }
}

void FLMerge(OTLookup *into, OTLookup *from) {
    /* Merge the feature list from "from" into "into" */
    FeatureScriptLangList *ifl, *ffl;

    /* first check for common featuretags and merge the scripts of each */
    for ( ffl = from->features; ffl!=NULL; ffl = ffl->next ) {
	for ( ifl=into->features; ifl!=NULL; ifl=ifl->next ) {
	    if ( ffl->featuretag==ifl->featuretag )
	break;
	}
	if ( ifl!=NULL )
	    SLMerge(ffl,ifl->scripts);
	else {
	    ifl = FeatureListCopy(ffl);
	    ifl->next = into->features;
	    into->features = ifl;
	}
    }
    into->features = FLOrder(into->features);
}

void SFSubTablesMerge(SplineFont *_sf,struct lookup_subtable *subfirst,
	struct lookup_subtable *subsecond) {
    int lookup_type = subfirst->lookup->lookup_type;
    int gid,k,isv;
    SplineChar *sc;
    SplineFont *sf = _sf;
    PST *pst, *fpst, *spst, *pstprev, *pstnext;
    KernPair *fkp, *skp, *kpprev, *kpnext;
    AnchorClass *ac;

    if ( lookup_type != subsecond->lookup->lookup_type ) {
	IError("Attempt to merge lookup subtables with mismatch types");
return;
    }
    if ( lookup_type != gsub_single &&
	    lookup_type != gsub_multiple &&
	    lookup_type != gsub_alternate &&
	    lookup_type != gsub_ligature &&
	    lookup_type != gpos_single &&
	    lookup_type != gpos_pair &&
	    lookup_type != gpos_cursive &&
	    lookup_type != gpos_mark2base &&
	    lookup_type != gpos_mark2ligature &&
	    lookup_type != gpos_mark2mark ) {
	IError("Attempt to merge lookup subtables with bad types");
return;
    } else if ( subfirst->kc!=NULL || subsecond->kc != NULL ) {
	IError("Attempt to merge lookup subtables with kerning classes");
return;
    }

    if ( lookup_type==gpos_cursive || lookup_type==gpos_mark2base ||
	    lookup_type==gpos_mark2ligature || lookup_type==gpos_mark2mark ) {
	for ( ac = sf->anchor; ac!=NULL ; ac=ac->next )
	    if ( ac->subtable == subsecond )
		ac->subtable = subfirst;
    } else {
	k=0;
	do {
	    sf = _sf->subfontcnt==0 ? _sf : _sf->subfonts[k];
	    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL ) {
		if ( lookup_type==gsub_single || lookup_type==gsub_multiple ||
			lookup_type==gsub_alternate || lookup_type==gpos_single ) {
		    fpst = spst = NULL;
		    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
			if ( pst->subtable == subfirst ) {
			    fpst = pst;
			    if ( spst!=NULL )
		    break;
			} else if ( pst->subtable == subsecond ) {
			    spst = pst;
			    if ( fpst!=NULL )
		    break;
			}
		    }
		    if ( fpst==NULL && spst!=NULL )
			spst->subtable = subfirst;
		    else if ( spst!=NULL ) {
			LogError(_("The glyph, %s, contains a %s from %s and one from %s.\nThe one from %s will be removed.\n"),
				sc->name,
			        lookup_type==gpos_single ? _("positioning") : _("substitution"),
			        subfirst->subtable_name, subsecond->subtable_name,
			        subsecond->subtable_name );
			pstprev = NULL;
			for ( pst=sc->possub; pst!=NULL && pst!=spst; pstprev=pst, pst=pst->next );
			if ( pstprev==NULL )
			    sc->possub = spst->next;
			else
			    pstprev = spst->next;
			spst->next = NULL;
			PSTFree(spst);
		    }
		} else if ( lookup_type==gsub_ligature || lookup_type==gpos_pair ) {
		    pstprev = NULL;
		    for ( spst=sc->possub; spst!=NULL ; spst = pstnext ) {
			pstnext = spst->next;
			if ( spst->subtable == subsecond ) {
			    for ( fpst=sc->possub; fpst!=NULL; fpst=fpst->next ) {
				if ( fpst->subtable == subfirst &&
					strcmp(fpst->u.lig.components,spst->u.lig.components)==0 )
			    break;
			    }
			    if ( fpst==NULL )
				spst->subtable = subfirst;
			    else {
				LogError(_("The glyph, %s, contains the same %s from %s and from %s.\nThe one from %s will be removed.\n"),
					sc->name,
			                lookup_type==gsub_ligature ? _("ligature") : _("kern pair"),
					subfirst->subtable_name, subsecond->subtable_name,
					subsecond->subtable_name );
				if ( pstprev==NULL )
				    sc->possub = pstnext;
				else
				    pstprev->next = pstnext;
				spst->next = NULL;
			        PSTFree(spst);
			        spst = pstprev;
			    }
			}
			pstprev = spst;
		    }
		    for ( isv=0; isv<2; ++isv ) {
			kpprev = NULL;
			for ( skp=isv ? sc->vkerns : sc->kerns; skp!=NULL ; skp = kpnext ) {
			    kpnext = skp->next;
			    if ( skp->subtable == subsecond ) {
				for ( fkp=isv ? sc->vkerns : sc->kerns; fkp!=NULL; fkp=fkp->next ) {
				    if ( fkp->subtable == subfirst && fkp->sc==skp->sc )
				break;
				}
				if ( fkp==NULL )
				    skp->subtable = subfirst;
				else {
				    LogError(_("The glyph, %s, contains the same kern pair from %s and from %s.\nThe one from %s will be removed.\n"),
					    sc->name,
					    subfirst->subtable_name, subsecond->subtable_name,
					    subsecond->subtable_name );
				    if ( kpprev!=NULL )
					kpprev->next = kpnext;
				    else if ( isv )
					sc->vkerns = kpnext;
				    else
					sc->kerns = kpnext;
				    skp->next = NULL;
				    KernPairsFree(skp);
				    skp = kpprev;
				}
			    }
			    kpprev = skp;
			}
		    }
		}
	    }
	    ++k;
	} while ( k<_sf->subfontcnt );
    }
}

/* ************************************************************************** */
/* ******************************* copy lookup ****************************** */
/* ************************************************************************** */

static char **ClassCopy(int class_cnt,char **classes) {
    char **newclasses;
    int i;

    if ( classes==NULL || class_cnt==0 )
return( NULL );
    newclasses = galloc(class_cnt*sizeof(char *));
    for ( i=0; i<class_cnt; ++i )
	newclasses[i] = copy(classes[i]);
return( newclasses );
}

static OTLookup *OTLookupCopyNested(SplineFont *to_sf,SplineFont *from_sf, char *prefix,
	OTLookup *from_otl) {
    char *newname;
    OTLookup *to_nested_otl;

    if ( from_otl==NULL )
return( NULL );
    newname = strconcat(prefix,from_otl->lookup_name);
    to_nested_otl = SFFindLookup(to_sf,newname);
    free(newname);
    if ( to_nested_otl==NULL )
	to_nested_otl = OTLookupCopyInto(to_sf,from_sf, from_otl);
return( to_nested_otl );
}

static KernClass *SF_AddKernClass(SplineFont *to_sf,KernClass *kc, struct lookup_subtable *sub,
	SplineFont *from_sf, char *prefix) {
    KernClass *newkc;

    newkc = chunkalloc(sizeof(KernClass));
    *newkc = *kc;
    newkc->subtable = sub;
    if ( sub->vertical_kerning ) {
	newkc->next = to_sf->vkerns;
	to_sf->vkerns = newkc;
    } else {
	newkc->next = to_sf->kerns;
	to_sf->kerns = newkc;
    }

    newkc->firsts = ClassCopy(newkc->first_cnt,newkc->firsts);
    newkc->seconds = ClassCopy(newkc->second_cnt,newkc->seconds);
    newkc->offsets = galloc(newkc->first_cnt*newkc->first_cnt*sizeof(int16));
    memcpy(newkc->offsets,kc->offsets,newkc->first_cnt*newkc->first_cnt*sizeof(int16));
return( newkc );
}

static FPST *SF_AddFPST(SplineFont *to_sf,FPST *fpst, struct lookup_subtable *sub,
	SplineFont *from_sf, char *prefix) {
    FPST *newfpst;
    int i, k, cur;

    newfpst = chunkalloc(sizeof(FPST));
    *newfpst = *fpst;
    newfpst->subtable = sub;
    newfpst->next = to_sf->possub;
    to_sf->possub = newfpst;

    newfpst->nclass = ClassCopy(newfpst->nccnt,newfpst->nclass);
    newfpst->bclass = ClassCopy(newfpst->bccnt,newfpst->bclass);
    newfpst->fclass = ClassCopy(newfpst->fccnt,newfpst->fclass);

    newfpst->rules = galloc(newfpst->rule_cnt*sizeof(struct fpst_rule));
    memcpy(newfpst->rules,fpst->rules,newfpst->rule_cnt*sizeof(struct fpst_rule));

    cur = 0;
    for ( i=0; i<newfpst->rule_cnt; ++i ) {
	struct fpst_rule *r = &newfpst->rules[i], *oldr = &fpst->rules[i];

	r->lookups = galloc(r->lookup_cnt*sizeof(struct seqlookup));
	memcpy(r->lookups,oldr->lookups,r->lookup_cnt*sizeof(struct seqlookup));
	for ( k=0; k<r->lookup_cnt; ++k ) {
	    r->lookups[k].lookup = OTLookupCopyNested(to_sf,from_sf, prefix,
		r->lookups[k].lookup);
	}

	switch ( newfpst->format ) {
	  case pst_glyphs:
	    r->u.glyph.names = copy( r->u.glyph.names );
	    r->u.glyph.back = copy( r->u.glyph.back );
	    r->u.glyph.fore = copy( r->u.glyph.fore );
	  break;
	  case pst_class:
	    r->u.class.nclasses = galloc( r->u.class.ncnt*sizeof(uint16));
	    memcpy(r->u.class.nclasses,oldr->u.class.nclasses, r->u.class.ncnt*sizeof(uint16));
	    r->u.class.bclasses = galloc( r->u.class.bcnt*sizeof(uint16));
	    memcpy(r->u.class.bclasses,oldr->u.class.bclasses, r->u.class.ncnt*sizeof(uint16));
	    r->u.class.fclasses = galloc( r->u.class.fcnt*sizeof(uint16));
	    memcpy(r->u.class.fclasses,oldr->u.class.fclasses, r->u.class.fcnt*sizeof(uint16));
	  break;
	  case pst_coverage:
	    r->u.coverage.ncovers = ClassCopy( r->u.coverage.ncnt, r->u.coverage.ncovers );
	    r->u.coverage.bcovers = ClassCopy( r->u.coverage.bcnt, r->u.coverage.bcovers );
	    r->u.coverage.fcovers = ClassCopy( r->u.coverage.fcnt, r->u.coverage.fcovers );
	  break;
	  case pst_reversecoverage:
	    r->u.rcoverage.ncovers = ClassCopy( r->u.rcoverage.always1, r->u.rcoverage.ncovers );
	    r->u.rcoverage.bcovers = ClassCopy( r->u.rcoverage.bcnt, r->u.rcoverage.bcovers );
	    r->u.rcoverage.fcovers = ClassCopy( r->u.rcoverage.fcnt, r->u.rcoverage.fcovers );
	    r->u.rcoverage.replacements = copy( r->u.rcoverage.replacements );
	  break;
	}
    }
return( newfpst );
}

static ASM *SF_AddASM(SplineFont *to_sf,ASM *sm, struct lookup_subtable *sub,
	SplineFont *from_sf, char *prefix) {
    ASM *newsm;
    int i;

    newsm = chunkalloc(sizeof(ASM));
    *newsm = *sm;
    newsm->subtable = sub;
    newsm->next = to_sf->sm;
    to_sf->sm = newsm;
    to_sf->changed = true;
    newsm->classes = ClassCopy(newsm->class_cnt, newsm->classes);
    newsm->state = galloc(newsm->class_cnt*newsm->state_cnt*sizeof(struct asm_state));
    memcpy(newsm->state,sm->state,
	    newsm->class_cnt*newsm->state_cnt*sizeof(struct asm_state));
    if ( newsm->type == asm_kern ) {
	for ( i=newsm->class_cnt*newsm->state_cnt-1; i>=0; --i ) {
	    newsm->state[i].u.kern.kerns = galloc(newsm->state[i].u.kern.kcnt*sizeof(int16));
	    memcpy(newsm->state[i].u.kern.kerns,sm->state[i].u.kern.kerns,newsm->state[i].u.kern.kcnt*sizeof(int16));
	}
    } else if ( newsm->type == asm_insert ) {
	for ( i=0; i<newsm->class_cnt*newsm->state_cnt; ++i ) {
	    struct asm_state *this = &newsm->state[i];
	    this->u.insert.mark_ins = copy(this->u.insert.mark_ins);
	    this->u.insert.cur_ins = copy(this->u.insert.cur_ins);
	}
    } else if ( newsm->type == asm_context ) {
	for ( i=0; i<newsm->class_cnt*newsm->state_cnt; ++i ) {
	    newsm->state[i].u.context.mark_lookup = OTLookupCopyNested(to_sf,from_sf, prefix,
		    newsm->state[i].u.context.mark_lookup);
	    newsm->state[i].u.context.cur_lookup = OTLookupCopyNested(to_sf,from_sf, prefix,
		    newsm->state[i].u.context.cur_lookup);
	}
    }
return( newsm );
}

static SplineChar *SCFindOrMake(SplineFont *into,SplineChar *fromsc) {
    int to_index;

    if ( into->cidmaster==NULL && into->fv!=NULL ) {
	to_index = SFFindSlot(into,into->fv->map,fromsc->unicodeenc,fromsc->name);
	if ( to_index==-1 )
return( NULL );
return( SFMakeChar(into,into->fv->map,to_index));
    }
return( SFGetChar(into,fromsc->unicodeenc,fromsc->name));
}

static void SF_SCAddAP(SplineChar *tosc,AnchorPoint *ap, AnchorClass *newac) {
    AnchorPoint *newap;

    newap = chunkalloc(sizeof(AnchorPoint));
    *newap = *ap;
    newap->anchor = newac;
    newap->next = tosc->anchor;
    tosc->anchor = newap;
}

static void SF_AddAnchorClasses(SplineFont *to_sf,struct lookup_subtable *from_sub, struct lookup_subtable *sub,
	SplineFont *from_sf, char *prefix) {
    AnchorClass *ac, *nac;
    int k, gid;
    SplineFont *fsf;
    AnchorPoint *ap;
    SplineChar *fsc, *tsc;

    for ( ac=from_sf->anchor; ac!=NULL; ac=ac->next ) if ( ac->subtable==from_sub ) {
	nac = chunkalloc(sizeof(AnchorClass));
	*nac = *ac;
	nac->subtable = sub;
	nac->name = strconcat(prefix,nac->name);
	nac->next = to_sf->anchor;
	to_sf->anchor = nac;

	k=0;
	do {
	    fsf = from_sf->subfontcnt==0 ? from_sf : from_sf->subfonts[k];
	    for ( gid = 0; gid<fsf->glyphcnt; ++gid ) if ( (fsc = fsf->glyphs[gid])!=NULL ) {
		for ( ap=fsc->anchor; ap!=NULL; ap=ap->next ) {
		    if ( ap->anchor==ac ) {
			tsc = SCFindOrMake(to_sf,fsc);
			if ( tsc==NULL )
		break;
			SF_SCAddAP(tsc,ap,nac);
		    }
		}
	    }
	    ++k;
	} while ( k<from_sf->subfontcnt );
    }
}

static int SF_SCAddPST(SplineChar *tosc,PST *pst,struct lookup_subtable *sub) {
    PST *newpst;

    newpst = chunkalloc(sizeof(PST));
    *newpst = *pst;
    newpst->subtable = sub;
    newpst->next = tosc->possub;
    tosc->possub = newpst;

    switch( newpst->type ) {
      case pst_pair:
	newpst->u.pair.paired = copy(pst->u.pair.paired);
	newpst->u.pair.vr = chunkalloc(sizeof(struct vr [2]));
	memcpy(newpst->u.pair.vr,pst->u.pair.vr,sizeof(struct vr [2]));
      break;
      case pst_ligature:
	newpst->u.lig.lig = tosc;
	/* Fall through */
      case pst_substitution:
      case pst_alternate:
      case pst_multiple:
	newpst->u.subs.variant = copy(pst->u.subs.variant);
      break;
    }
return( true );
}

static int SF_SCAddKP(SplineChar *tosc,KernPair *kp,struct lookup_subtable *sub,
	int isvkern, SplineFont *to_sf ) {
    SplineChar *tosecond;
    KernPair *newkp;

    tosecond = SFGetChar(to_sf,kp->sc->unicodeenc,kp->sc->name);
    if ( tosecond==NULL )
return( false );

    newkp = chunkalloc(sizeof(KernPair));
    *newkp = *kp;
    newkp->subtable = sub;
    newkp->sc = tosecond;
    if ( isvkern ) {
	newkp->next = tosc->vkerns;
	tosc->vkerns = newkp;
    } else {
	newkp->next = tosc->kerns;
	tosc->kerns = newkp;
    }
return(true);
}

static void SF_AddPSTKern(SplineFont *to_sf,struct lookup_subtable *from_sub, struct lookup_subtable *sub,
	SplineFont *from_sf, char *prefix) {
    int k, gid, isv;
    SplineFont *fsf;
    SplineChar *fsc, *tsc;
    PST *pst;
    KernPair *kp;
    int iskern = sub->lookup->lookup_type==gpos_pair;

    k=0;
    do {
	fsf = from_sf->subfontcnt==0 ? from_sf : from_sf->subfonts[k];
	for ( gid = 0; gid<fsf->glyphcnt; ++gid ) if ( (fsc = fsf->glyphs[gid])!=NULL ) {
	    tsc = (SplineChar *) -1;
	    for ( pst = fsc->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->subtable==from_sub ) {
		    if ( tsc==(SplineChar *) -1 ) {
			tsc = SCFindOrMake(to_sf,fsc);
			if ( tsc==NULL )
	    break;
		    }
		    SF_SCAddPST(tsc,pst,sub);
		}
	    }
	    if ( tsc!=NULL && iskern ) {
		for ( isv=0; isv<2 && tsc!=NULL; ++isv ) {
		    for ( kp= isv ? fsc->vkerns : fsc->kerns; kp!=NULL; kp=kp->next ) {
			if ( kp->subtable==sub ) {
			    /* Kerning data tend to be individualistic. Only copy if */
			    /*  glyphs exist */
			    if ( tsc==(SplineChar *) -1 ) {
				tsc = SFGetChar(to_sf,fsc->unicodeenc,fsc->name);
				if ( tsc==NULL )
		    break;
			    }
			    SF_SCAddKP(tsc,kp,sub,isv,to_sf);
			}
		    }
		}
	    }
	}
	++k;
    } while ( k<from_sf->subfontcnt );
}

void SortInsertLookup(SplineFont *sf, OTLookup *newotl) {
    int isgpos = newotl->lookup_type>=gpos_start;
    int pos, i, k;
    OTLookup *prev, *otl;

    pos = FeatureOrderId(isgpos,newotl->features);
    for ( prev=NULL, otl= isgpos ? sf->gpos_lookups : sf->gsub_lookups ;
	    otl!=NULL && FeatureOrderId(isgpos,newotl->features)<pos;
	    prev = otl, otl=otl->next );
    newotl->next = otl;
    if ( prev!=NULL )
	prev->next = newotl;
    else if ( isgpos )
	sf->gpos_lookups = newotl;
    else
	sf->gsub_lookups = newotl;

    if ( sf->fontinfo ) {
	struct lkdata *lk = &sf->fontinfo->tables[isgpos];
	if ( lk->cnt>=lk->max )
	    lk->all = grealloc(lk->all,(lk->max+=10)*sizeof(struct lkinfo));
	for ( i=0; i<lk->cnt && FeatureOrderId(isgpos,lk->all[i].lookup->features)<pos; ++i );
	for ( k=lk->cnt; k>i+1; --k )
	    lk->all[k] = lk->all[k-1];
	memset(&lk->all[k],0,sizeof(struct lkinfo));
	lk->all[k].lookup = newotl;
	++lk->cnt;
	GFI_LookupScrollbars(sf->fontinfo,isgpos, true);
	GFI_LookupEnableButtons(sf->fontinfo,isgpos);
    }
}

OTLookup *OTLookupCopyInto(SplineFont *into_sf,SplineFont *from_sf, OTLookup *from_otl) {
    OTLookup *otl = chunkalloc(sizeof(OTLookup));
    struct lookup_subtable *sub, *last, *from_sub;
    char *prefix = strconcat(from_sf->fontname,"-");
    int scnt;

    into_sf->changed = true;

    *otl = *from_otl;
    otl->lookup_name = strconcat(prefix,from_otl->lookup_name);
    otl->features = FeatureListCopy(from_otl->features);
    otl->next = NULL; otl->subtables = NULL;
    SortInsertLookup(into_sf,otl);

    last = NULL;
    scnt = 0;
    for ( from_sub = from_otl->subtables; from_sub!=NULL; from_sub=from_sub->next ) {
	sub = chunkalloc(sizeof(struct lookup_subtable));
	*sub = *from_sub;
	sub->lookup = otl;
	sub->subtable_name = strconcat(prefix,from_sub->subtable_name);
	sub->suffix = copy(sub->suffix);
	if ( last==NULL )
	    otl->subtables = sub;
	else
	    last->next = sub;
	last = sub;
	if ( from_sub->kc!=NULL )
	    sub->kc = SF_AddKernClass(into_sf, from_sub->kc, sub, from_sf, prefix);
	else if ( from_sub->fpst!=NULL )
	    sub->fpst = SF_AddFPST(into_sf, from_sub->fpst, sub, from_sf, prefix);
	else if ( from_sub->sm!=NULL )
	    sub->sm = SF_AddASM(into_sf, from_sub->sm, sub, from_sf, prefix);
	else if ( from_sub->anchor_classes )
	    SF_AddAnchorClasses(into_sf, from_sub, sub, from_sf, prefix);
	else
	    SF_AddPSTKern(into_sf, from_sub, sub, from_sf, prefix);
	++scnt;
    }
    if ( into_sf->fontinfo ) {
	int isgpos = otl->lookup_type>=gpos_start;
	struct lkdata *lk = &into_sf->fontinfo->tables[isgpos];
	int i;
	for ( i=0; i<lk->cnt; ++i )
	    if ( lk->all[i].lookup==otl )
	break;
	lk->all[i].subtable_cnt = lk->all[i].subtable_max = scnt;
	lk->all[i].subtables = gcalloc(scnt,sizeof(struct lksubinfo));
	for ( sub=otl->subtables, scnt=0; sub!=NULL; sub=sub->next, ++scnt )
	    lk->all[i].subtables[scnt].subtable = sub;
    }
return( otl );
}

/* ************************************************************************** */
/* ****************************** Apply lookups ***************************** */
/* ************************************************************************** */

struct lookup_data {
    struct opentype_str *str;
    int cnt, max;

    uint32 script;
    SplineFont *sf;

    struct lookup_subtable *lig_owner;
    int lcnt, lmax;
    SplineChar ***ligs;		/* For each ligature we have an array of SplineChars that are its components preceded by the ligature glyph itself */
				/*  NULL terminated */
    int pixelsize;
    double scale;
};

static int ApplyLookupAtPos(uint32 tag, OTLookup *otl,struct lookup_data *data,int pos);

static int GlyphNameInClass(char *name,char *class ) {
    char *pt;
    int len = strlen(name);

    if ( class==NULL )
return( false );

    pt = class;
    while ( (pt=strstr(pt,name))!=NULL ) {
	if ( pt==NULL )
return( false );
	if ( (pt==class || pt[-1]==' ') && (pt[len]=='\0' || pt[len]==' '))
return( true );
	pt+=len;
    }

return( false );
}

/* ************************************************************************** */
/* ************************ Apply Apple State Machines ********************** */
/* ************************************************************************** */

static void ApplyMacIndicRearrangement(struct lookup_data *data,int verb,
	int first_pos,int last_pos) {
    struct opentype_str temp, temp2, temp3, temp4;
    int i;

    if ( first_pos==-1 || last_pos==-1 || last_pos <= first_pos )
return;
    switch ( verb ) {
      case 1: /* Ax => xA */
	temp = data->str[first_pos];
	for ( i= first_pos; i<last_pos; ++i )
	    data->str[i] = data->str[i+1];
	data->str[last_pos] = temp;
      break;
      case 2: /* xD => Dx */
	temp = data->str[last_pos];
	for ( i= last_pos; i>first_pos; --i )
	    data->str[i] = data->str[i-1];
	data->str[first_pos] = temp;
      break;
      case 3: /* AxD => DxA */
	temp = data->str[last_pos];
	data->str[last_pos] = data->str[first_pos];
	data->str[first_pos] = temp;
      break;
      case 4: /* ABx => xAB */
	temp = data->str[first_pos];
	temp2 = data->str[first_pos+1];
	for ( i= first_pos; i<last_pos-1; ++i )
	    data->str[i] = data->str[i+21];
	data->str[last_pos-1] = temp;
	data->str[last_pos] = temp2;
      break;
      case 5: /* ABx => xBA */
	temp = data->str[first_pos];
	temp2 = data->str[first_pos+1];
	for ( i= first_pos; i<last_pos-1; ++i )
	    data->str[i] = data->str[i+21];
	data->str[last_pos-1] = temp2;
	data->str[last_pos] = temp;
      break;
      case 6: /* xCD => CDx */
	temp = data->str[last_pos];
	temp2 = data->str[last_pos-1];
	for ( i= last_pos; i>first_pos+1; --i )
	    data->str[i] = data->str[i-2];
	data->str[first_pos+1] = temp;
	data->str[first_pos] = temp2;
      break;
      case 7: /* xCD => DCx */
	temp = data->str[last_pos];
	temp2 = data->str[last_pos-1];
	for ( i= last_pos; i>first_pos+1; --i )
	    data->str[i] = data->str[i-2];
	data->str[first_pos+1] = temp2;
	data->str[first_pos] = temp;
      break;
      case 8: /* AxCD => CDxA */
	temp = data->str[first_pos];
	temp2 = data->str[last_pos-1];
	temp3 = data->str[last_pos];
	for ( i= last_pos-1; i>first_pos; --i )
	    data->str[i] = data->str[i-1];
	data->str[first_pos+1] = temp2;
	data->str[first_pos] = temp3;
	data->str[last_pos] = temp;
      break;
      case 9: /* AxCD => DCxA */
	temp = data->str[first_pos];
	temp2 = data->str[last_pos-1];
	temp3 = data->str[last_pos];
	for ( i= last_pos-1; i>first_pos; --i )
	    data->str[i] = data->str[i-1];
	data->str[first_pos+1] = temp3;
	data->str[first_pos] = temp2;
	data->str[last_pos] = temp;
      break;
      case 10: /* ABxD => DxAB */
	temp = data->str[first_pos];
	temp2 = data->str[first_pos+1];
	temp3 = data->str[last_pos];
	for ( i= first_pos+1; i<last_pos; ++i )
	    data->str[i] = data->str[i+1];
	data->str[first_pos] = temp3;
	data->str[last_pos-1] = temp;
	data->str[last_pos] = temp2;
      break;
      case 11: /* ABxD => DxBA */
	temp = data->str[first_pos];
	temp2 = data->str[first_pos+1];
	temp3 = data->str[last_pos];
	for ( i= first_pos+1; i<last_pos; ++i )
	    data->str[i] = data->str[i+1];
	data->str[first_pos] = temp3;
	data->str[last_pos-1] = temp2;
	data->str[last_pos] = temp;
      break;
      case 12: /* ABxCD => CDxAB */
	temp = data->str[first_pos];
	temp2 = data->str[first_pos+1];
	temp3 = data->str[last_pos-1];
	temp4 = data->str[last_pos];
	data->str[last_pos] = temp2;
	data->str[last_pos-1] = temp;
	data->str[first_pos+1] = temp4;
	data->str[first_pos] = temp3;
      break;
      case 13: /* ABxCD => CDxBA */
	temp = data->str[first_pos];
	temp2 = data->str[first_pos+1];
	temp3 = data->str[last_pos-1];
	temp4 = data->str[last_pos];
	data->str[last_pos] = temp;
	data->str[last_pos-1] = temp2;
	data->str[first_pos+1] = temp4;
	data->str[first_pos] = temp3;
      break;
      case 14: /* ABxCD => DCxAB */
	temp = data->str[first_pos];
	temp2 = data->str[first_pos+1];
	temp3 = data->str[last_pos-1];
	temp4 = data->str[last_pos];
	data->str[last_pos] = temp2;
	data->str[last_pos-1] = temp;
	data->str[first_pos+1] = temp3;
	data->str[first_pos] = temp4;
      break;
      case 15: /* ABxCD => DCxBA */
	temp = data->str[first_pos];
	temp2 = data->str[first_pos+1];
	temp3 = data->str[last_pos-1];
	temp4 = data->str[last_pos];
	data->str[last_pos] = temp;
	data->str[last_pos-1] = temp2;
	data->str[first_pos+1] = temp3;
	data->str[first_pos] = temp4;
      break;
    }
}

static int ApplyMacInsert(struct lookup_data *data,int ipos,int cnt,
	char *glyphnames, int orig_index) {
    SplineChar *inserts[32];
    char *start, *pt;
    int i, ch;

    if ( cnt==0 || glyphnames==NULL || ipos == -1 )
return( 0 );

    for ( i=0, start = glyphnames; i<cnt; ) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
    break;
	for ( pt = start; *pt!=' ' && *pt!='\0'; ++pt );
	ch = *pt; *pt = '\0';
	inserts[i] = SFGetChar(data->sf,-1,start);
	*pt = ch;
	if ( inserts[i]!=NULL )
	    ++i;
    }
    cnt = i;
    if ( i==0 )
return( 0 );
    for ( i= data->cnt; i>=ipos; --i )
	data->str[i+cnt] = data->str[i];
    memset(data->str+ipos,0,cnt*sizeof(struct opentype_str));
    for ( i=0; i<cnt; ++i ) {
	data->str[ipos+i].sc = inserts[i];
	data->str[ipos+i].orig_index = orig_index;
    }
return( cnt );
}

static void ApplyAppleStateMachine(uint32 tag, OTLookup *otl,struct lookup_data *data) {
    struct lookup_subtable *sub;
    int state, class, pos, mark_pos, markend_pos, i;
    ASM *sm;
    int cnt_cur, cnt_mark;
    struct asm_state *entry;
    int kern_stack[8], kcnt;		/* Kerning state machines handle at most 8 glyphs */
    /* Flaws: Line processing has not been done yet, so we are never in the */
    /*  start of line state and we never get an end of line token. We never */
    /*  get deleted tokens either, those glyphs are just gone */
    /* Class 0: End of text */
    /* Class 1: Glyph not in any classes */
    /* Class 2: Deleted (we never see) */
    /* Class 3: End of line (we never see) */

    /* Mac doesn't have the concept of subtables, but a user could create one */
    /*  it will get flattened out into its own "lookup" when written to a file*/
    /*  So if there are multiple subtables, just process them all */
    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
	sm = sub->sm;

	state = 0;
	mark_pos = markend_pos = -1;
	for ( pos = 0; pos<=data->cnt; ) {
	    if ( pos==data->cnt )
		class = 0;
	    else {
		for ( class = sm->class_cnt-1; class>3; --class )
		    if ( GlyphNameInClass(data->str[i].sc->name,sm->classes[class]) )
		break;
		if ( class==3 )
		    class = 1;		/* Glyph not in any class */;
	    }
	    entry = &sm->state[state*sm->class_cnt+class];
	    switch ( otl->lookup_type ) {
	      case morx_context:
		if ( entry->u.context.cur_lookup!=NULL )
		    ApplyLookupAtPos(0,entry->u.context.cur_lookup,data,pos);
		if ( entry->u.context.mark_lookup!=NULL && mark_pos!=-1 ) {
		    ApplyLookupAtPos(0,entry->u.context.mark_lookup,data,mark_pos);
		    mark_pos = -1;
		}
	      break;
	      case morx_indic:
		if ( entry->flags & 0x2000 )
		    markend_pos = pos;
		if ( (entry->flags&0xf)!=0 ) {
		    ApplyMacIndicRearrangement(data,entry->flags&0xf,mark_pos,markend_pos);
		    mark_pos = markend_pos = -1;
		}
	      break;
	      case morx_insert:
		/* I live in total ignorance of what I should do if the glyph */
		/*  "is Kashida like"... so I ignore those flags. */
		cnt_cur = (entry->flags>>5)&0x1f;
		cnt_mark = (entry->flags&0x1f);
		if ( data->cnt + cnt_cur + cnt_mark >= data->max )
		    data->str = grealloc(data->str,(data->max = data->cnt + cnt_cur + cnt_mark +20)*sizeof(struct opentype_str));
		if ( cnt_cur!=0 )
		    cnt_cur = ApplyMacInsert(data,(entry->flags& 0x0800)? pos : pos+1,
			    cnt_cur,entry->u.insert.cur_ins,data->str[pos].orig_index);
		if ( cnt_mark!=0 && mark_pos!=-1 ) {
		    cnt_mark = ApplyMacInsert(data,(entry->flags& 0x0800)? mark_pos : mark_pos+1,
			    cnt_mark,entry->u.insert.mark_ins,data->str[mark_pos].orig_index);
		    mark_pos = -1;
		} else
		    cnt_mark = 0;
		pos += cnt_cur+cnt_mark;
	      break;
	      case kern_statemachine:
		if ( entry->u.kern.kcnt!=0 ) {
		    for ( i=0; i<kcnt && i<entry->u.kern.kcnt; ++i )
			data->str[kern_stack[i]].vr.h_adv_off +=
				entry->u.kern.kerns[i];
		    kcnt = 0;
		}
		if ( entry->flags & 0x8000 ) {
		    for ( i=6; i>=0; --i )
			kern_stack[i+1] = kern_stack[i];
		    kern_stack[0] = pos;
		    if ( ++kcnt>8 ) kcnt = 8;
		}
	      break;
	    }
	    if ( entry->flags & 0x8000 )
		mark_pos = pos;		/* The docs do not state whether this happens before or after substitutions are applied at the mark */
					/* after is more useful. So assume that */
	    if ( !(entry->flags & 0x4000) )
		++pos;
	    state = entry->next_state;
	}
    }
}

/* ************************************************************************** */
/* ************************* Apply OpenType Lookups ************************* */
/* ************************************************************************** */

static void LigatureFree(struct lookup_data *data) {
    int i;

    if ( data->ligs==NULL )
return;
    for ( i=0; data->ligs[i]!=NULL; ++i )
	free(data->ligs[i]);
}

static void LigatureSearch(struct lookup_subtable *sub, struct lookup_data *data) {
    SplineFont *sf = data->sf;
    int gid, ccnt, cnt, ch, err;
    SplineChar *sc;
    PST *pst;
    char *pt, *start;

    LigatureFree(data);
    cnt = 0;
    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL ) {
	for ( pst=sc->possub; pst!=NULL; pst=pst->next ) if ( pst->subtable==sub ) {
	    for ( pt = pst->u.lig.components, ccnt=0; *pt; ++pt )
		if ( *pt==' ' )
		    ++ccnt;
	    if ( cnt>=data->lmax )
		data->ligs = grealloc(data->ligs,(data->lmax+=100)*sizeof(SplineChar **));
	    data->ligs[cnt] = galloc((ccnt+3)*sizeof(SplineChar *));
	    data->ligs[cnt][0] = sc;
	    ccnt = 1;
	    err = 0;
	    for ( pt = pst->u.lig.components; *pt; ) {
		while ( *pt==' ' ) ++pt;
		if ( *pt=='\0' )
	    break;
		for ( start=pt; *pt!='\0' && *pt!=' '; ++pt );
		ch = *pt; *pt = '\0';
		data->ligs[cnt][ccnt++] = SFGetChar(sf,-1,start);
		*pt = ch;
		if ( data->ligs[cnt][ccnt-1]==NULL )
		    err = 1;
	    }
	    if ( !err )
		data->ligs[cnt++][ccnt] = NULL;
	}
    }
    if ( cnt>=data->lmax )
	data->ligs = grealloc(data->ligs,(data->lmax+=1)*sizeof(SplineChar **));
    data->ligs[cnt] = NULL;
    data->lcnt = cnt;
}

static int skipglyphs(int lookup_flags, struct lookup_data *data, int pos) {
    int mc, glyph_class;
    /* The lookup flags tell us what glyphs to ignore. Skip over anything we */
    /*  should ignore */

    if ( lookup_flags==0 )
return( pos );
    mc = (lookup_flags>>8);
    if ( mc<0 || mc>=data->sf->mark_class_cnt )
	mc = 0;
    while ( pos<data->cnt ) {
	glyph_class = gdefclass(data->str[pos].sc);
	/* 1=>base, 2=>ligature, 3=>mark, 4=>component?, 0=>.notdef */
	if ( (glyph_class==1 && (lookup_flags&pst_ignorebaseglyphs)) ||
		(glyph_class==2 && (lookup_flags&pst_ignoreligatures)) ||
		(glyph_class==3 && (lookup_flags&pst_ignorecombiningmarks)) ||
		(glyph_class==3 && mc!=0 &&
			!GlyphNameInClass(data->str[pos].sc->name,data->sf->mark_classes[mc])) ) {
	    ++pos;
	} else
    break;
    }
return( pos );
}

static int bskipglyphs(int lookup_flags, struct lookup_data *data, int pos) {
    int mc, glyph_class;
    /* The lookup flags tell us what glyphs to ignore. Skip over anything we */
    /*  should ignore. Here we skip backward */

    if ( lookup_flags==0 )
return( pos );
    mc = (lookup_flags>>8);
    if ( mc<0 || mc>=data->sf->mark_class_cnt )
	mc = 0;
    while ( pos>=0 ) {
	glyph_class = gdefclass(data->str[pos].sc);
	/* 1=>base, 2=>ligature, 3=>mark, 4=>component?, 0=>.notdef */
	if ( (glyph_class==1 && (lookup_flags&pst_ignorebaseglyphs)) ||
		(glyph_class==2 && (lookup_flags&pst_ignoreligatures)) ||
		(glyph_class==3 && (lookup_flags&pst_ignorecombiningmarks)) ||
		(glyph_class==3 && mc!=0 &&
			!GlyphNameInClass(data->str[pos].sc->name,data->sf->mark_classes[mc])) ) {
	    --pos;
	} else
    break;
    }
return( pos );
}

static int ContextualMatch(struct lookup_subtable *sub,struct lookup_data *data,
	int pos, struct fpst_rule **_rule) {
    int i, cpos, retpos, r;
    FPST *fpst = sub->fpst;
    int lookup_flags = sub->lookup->lookup_flags;
    char *pt, *start;

    /* If we should skip the current glyph then don't try for a match here */
    cpos = skipglyphs(lookup_flags,data,pos);
    if ( cpos!=pos )
return( 0 );

    for ( r=0; r<fpst->rule_cnt; ++r ) {
	struct fpst_rule *rule = &fpst->rules[r];
	for ( i=pos; i<data->cnt; ++i )
	    data->str[i].context_pos = -1;

/* Handle backtrack */
	if ( fpst->type == pst_chainpos || fpst->type == pst_chainsub ) {
	    if ( fpst->format==pst_glyphs ) {
		pt = rule->u.glyph.back;
		start = strrchr(pt,' ');
		if ( start==NULL ) start = pt-1;
		for ( i=bskipglyphs(lookup_flags,data,pos-1), cpos=0; i>=0; i = bskipglyphs(lookup_flags,data,i-1)) {
		    char *name = data->str[i].sc->name;
		    int len = strlen( name );
		    if ( strncmp(name,start,len)!=0 || (start[len]!='\0' && start[len]!=' '))
		break;
		    while ( start>=pt && *start==' ' ) --start;
		    if ( start<pt ) {
			pt = NULL;
		break;
		    }
		    while ( start>=pt && *start!=' ' ) --start;
		}
		if ( *pt!='\0' )
    continue;		/* didn't match */
	    } else if ( fpst->format==pst_class ) {
		for ( i=bskipglyphs(lookup_flags,data,pos-1), cpos=rule->u.class.bcnt-1; i>=0 && cpos>=0; i = bskipglyphs(lookup_flags,data,i-1)) {
		    if ( !GlyphNameInClass(data->str[i].sc->name,fpst->bclass[rule->u.class.bclasses[cpos]]) )
		break;
		    --cpos;
		}
		if ( cpos>=0 )
    continue;		/* didn't match */
	    } else if ( fpst->format==pst_coverage ) {
		for ( i=bskipglyphs(lookup_flags,data,pos-1), cpos=rule->u.coverage.bcnt-1; i>=0 && cpos>=0; i = bskipglyphs(lookup_flags,data,i-1)) {
		    if ( !GlyphNameInClass(data->str[i].sc->name,rule->u.coverage.bcovers[cpos]) )
		break;
		    --cpos;
		}
		if ( cpos>=0 )
    continue;		/* didn't match */
	    }
	}
/* Handle Match */
	if ( fpst->format==pst_glyphs ) {
	    pt = rule->u.glyph.names;
	    for ( i=pos, cpos=0; i<data->cnt && *pt!='\0'; i = skipglyphs(lookup_flags,data,i+1)) {
		char *name = data->str[i].sc->name;
		int len = strlen( name );
		if ( strncmp(name,pt,len)!=0 || (pt[len]!='\0' && pt[len]!=' '))
	    break;
		data->str[i].context_pos = cpos++;
		pt += len;
		while ( *pt==' ' ) ++pt;
	    }
	    if ( *pt!='\0' )
    continue;		/* didn't match */
	} else if ( fpst->format==pst_class ) {
	    for ( i=pos, cpos=0; i<data->cnt && cpos<rule->u.class.ncnt; i = skipglyphs(lookup_flags,data,i+1)) {
		int class = rule->u.class.nclasses[cpos];
		if ( class!=0 ) {
		    if ( !GlyphNameInClass(data->str[i].sc->name,fpst->nclass[class]) )
	    break;
		} else {
		    int c;
		    /* Ok, to match class 0 we must fail to match all other classes */
		    for ( c=1; c<fpst->nccnt; ++c )
			if ( !GlyphNameInClass(data->str[i].sc->name,fpst->nclass[c]) )
		    break;
		    if ( c!=fpst->nccnt )
	    break;		/* It matched another class => not in class 0 */
		}
		data->str[i].context_pos = cpos++;
	    }
	    if ( cpos<rule->u.class.ncnt )
    continue;		/* didn't match */
	} else if ( fpst->format==pst_coverage ) {
	    for ( i=pos, cpos=0; i<data->cnt && cpos<rule->u.coverage.ncnt; i = skipglyphs(lookup_flags,data,i+1)) {
		if ( !GlyphNameInClass(data->str[i].sc->name,rule->u.coverage.ncovers[cpos]) )
	    break;
		data->str[i].context_pos = cpos++;
	    }
	    if ( cpos<rule->u.coverage.ncnt )
    continue;		/* didn't match */
	} else
return( 0 );		/* Not ready to deal with reverse chainging */

	retpos = i;
/* Handle lookahead */
	if ( fpst->type == pst_chainpos || fpst->type == pst_chainsub ) {
	    if ( fpst->format==pst_glyphs ) {
		pt = rule->u.glyph.fore;
		for ( i=retpos; i<data->cnt && *pt!='\0'; i = skipglyphs(lookup_flags,data,i+1)) {
		    char *name = data->str[i].sc->name;
		    int len = strlen( name );
		    if ( strncmp(name,pt,len)!=0 || (pt[len]!='\0' && pt[len]!=' '))
		break;
		    pt += len;
		    while ( *pt==' ' ) ++pt;
		}
		if ( *pt!='\0' )
    continue;		/* didn't match */
	    } else if ( fpst->format==pst_class ) {
		for ( i=retpos, cpos=0; i<data->cnt && cpos<rule->u.class.fcnt; i = skipglyphs(lookup_flags,data,i+1)) {
		    if ( !GlyphNameInClass(data->str[i].sc->name,fpst->fclass[rule->u.class.fclasses[cpos]]) )
		break;
		    cpos++;
		}
		if ( cpos<rule->u.class.fcnt )
    continue;		/* didn't match */
	    } else if ( fpst->format==pst_coverage ) {
		for ( i=retpos, cpos=0; i<data->cnt && cpos<rule->u.coverage.fcnt; i = skipglyphs(lookup_flags,data,i+1)) {
		    if ( !GlyphNameInClass(data->str[i].sc->name,rule->u.coverage.fcovers[cpos]) )
		break;
		    cpos++;
		}
		if ( cpos<rule->u.coverage.fcnt )
    continue;		/* didn't match */
	    }
	}
	*_rule = rule;
return( retpos );
    }
return( 0 );
}

static int ApplySingleSubsAtPos(struct lookup_subtable *sub,struct lookup_data *data,int pos) {
    PST *pst;
    SplineChar *sc;

    for ( pst=data->str[pos].sc->possub; pst!=NULL && pst->subtable!=sub; pst=pst->next );
    if ( pst==NULL )
return( 0 );

    sc = SFGetChar(data->sf,-1,pst->u.subs.variant);
    if ( sc!=NULL ) {
	data->str[pos].sc = sc;
return( pos+1 );
    } else if ( strcmp(pst->u.subs.variant,MAC_DELETED_GLYPH_NAME)==0 ) {
	/* Under AAT we delete the glyph. But OpenType doesn't have that concept */
	int i;
	for ( i=pos+1; i<data->cnt; ++i )
	    data->str[pos-1] = data->str[pos];
	--data->cnt;
return( pos );
    } else {
return( 0 );
    }
}

static int ApplyMultSubsAtPos(struct lookup_subtable *sub,struct lookup_data *data,int pos) {
    PST *pst;
    SplineChar *sc;
    char *start, *pt;
    int mcnt, ch, i;
    SplineChar *mults[20];

    for ( pst=data->str[pos].sc->possub; pst!=NULL && pst->subtable!=sub; pst=pst->next );
    if ( pst==NULL )
return( 0 );

    mcnt = 0;
    for ( start = pst->u.alt.components; *start==' '; ++start);
    for ( ; *start; ) {
	for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	sc = SFGetChar(data->sf,-1,start);
	*pt = ch;
	if ( sc==NULL )
return( 0 );
	if ( mcnt<20 ) mults[mcnt++] = sc;
	while ( *pt==' ' ) ++pt;
	start = pt;
    }

    if ( mcnt==0 ) {
	/* Is this legal? that is can we remove a glyph with an empty multiple? */
	for ( i=pos+1; i<data->cnt; ++i )
	    data->str[i-1] = data->str[i];
	--data->cnt;
return( pos );
    } else if ( mcnt==1 ) {
	data->str[pos].sc = mults[0];
return( pos+1 );
    } else {
	if ( data->cnt+mcnt-1 >= data->max )
	    data->str = grealloc(data->str,(data->max+=mcnt) * sizeof( struct opentype_str ));
	for ( i=data->cnt-1; i>pos; --i )
	    data->str[i+mcnt-1] = data->str[i];
	memset(data->str+pos,0,mcnt*sizeof(struct opentype_str));
	for ( i=0; i<mcnt; ++i ) {
	    data->str[pos+i].sc = mults[i];
	    data->str[pos+i].orig_index = data->str[pos].orig_index;
	}
	data->cnt += (mcnt-1);
return( pos+mcnt );
    }
}

static int ApplyAltSubsAtPos(struct lookup_subtable *sub,struct lookup_data *data,int pos) {
    PST *pst;
    SplineChar *sc;
    char *start, *pt, ch;

    for ( pst=data->str[pos].sc->possub; pst!=NULL && pst->subtable!=sub; pst=pst->next );
    if ( pst==NULL )
return( 0 );

    for ( start = pst->u.alt.components; *start==' '; ++start);
    for ( ; *start; ) {
	for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	sc = SFGetChar(data->sf,-1,start);
	*pt = ch;
	if ( sc!=NULL ) {
	    data->str[pos].sc = sc;
return( pos+1 );
	}
	while ( *pt==' ' ) ++pt;
	start = pt;
    }
return( 0 );
}

static int ApplyLigatureSubsAtPos(struct lookup_subtable *sub,struct lookup_data *data,int pos) {
    int i,k, lpos, npos;
    int lookup_flags = sub->lookup->lookup_flags;
    int match_found = -1, match_len=0;

    if ( data->lig_owner!=sub )
	LigatureSearch(sub,data);
    for ( i=0; i<data->lcnt; ++i ) {
	if ( data->ligs[i][1]==data->str[pos].sc ) {
	    lpos = 0;
	    npos = pos+1;
	    for ( k=2; data->ligs[i][k]!=NULL; ++k ) {
		npos = skipglyphs(lookup_flags,data,npos);
		if ( npos>=data->cnt || data->str[npos].sc != data->ligs[i][k] )
	    break;
		++npos;
	    }
	    if ( data->ligs[i][k]==NULL ) {
		if ( match_found==-1 || k>match_len ) {
		    match_found = i;
		    match_len = k;
		}
	    }
	}
    }
    if ( match_found!=-1 ) {
	/* Matched. Remove the component glyphs, and note which component */
	/*  any intervening marks should be attached to */
	data->str[pos].sc = data->ligs[match_found][0];
	npos = pos+1;
	for ( k=2; data->ligs[match_found][k]!=NULL; ++k ) {
	    lpos = skipglyphs(lookup_flags,data,npos);
	    for ( ; npos<lpos; ++npos )
		data->str[npos].lig_pos = k-2;
	    /* Remove this glyph (copy the final NUL too) */
	    for ( ++lpos; lpos<=data->cnt; ++lpos )
		data->str[lpos-1] = data->str[lpos];
	    --data->cnt;
	}
	/* Any marks after the last component (which should be attached */
	/*  to it) will not have been tagged, so do that now */
	lpos = skipglyphs(lookup_flags,data,npos);
	for ( ; npos<lpos; ++npos )
	    data->str[npos].lig_pos = k-2;
return( pos+1 );
    }

return( 0 );
}
		    
static int ApplyContextual(struct lookup_subtable *sub,struct lookup_data *data,int pos) {
    /* On this level there is no difference between GPOS/GSUB contextuals */
    /*  If the contextual matches, then we apply the lookups, otherwise we */
    /*  don't. Now the lookups will be different, but we don't care here */
    struct fpst_rule *rule;
    int retpos, i,j;

    retpos = ContextualMatch(sub,data,pos,&rule);
    if ( retpos==0 )
return( 0 );
    for ( i=0; i<rule->lookup_cnt; ++i ) {
	for ( j=pos; j<data->cnt; ++j ) {
	    if ( data->str[j].context_pos == rule->lookups[i].seq ) {
		ApplyLookupAtPos(0,rule->lookups[i].lookup,data,j);
	break;
	    }
	}
    }
return( retpos );
}

static int FigureDeviceTable(DeviceTable *dt,int pixelsize) {

    if ( dt==NULL || dt->corrections==NULL || pixelsize<dt->first_pixel_size ||
	    pixelsize>dt->last_pixel_size )
return( 0 );

return( dt->corrections[pixelsize - dt->last_pixel_size] );
}

static int ApplySinglePosAtPos(struct lookup_subtable *sub,struct lookup_data *data,int pos) {
    PST *pst;

    for ( pst=data->str[pos].sc->possub; pst!=NULL && pst->subtable!=sub; pst=pst->next );
    if ( pst==NULL )
return( 0 );

    data->str[pos].vr.xoff += rint( pst->u.pos.xoff * data->scale );
    data->str[pos].vr.yoff += rint( pst->u.pos.yoff * data->scale );
    data->str[pos].vr.h_adv_off += rint( pst->u.pos.h_adv_off * data->scale );
    data->str[pos].vr.v_adv_off += rint( pst->u.pos.v_adv_off * data->scale );
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    if ( pst->u.pos.adjust!=NULL ) {
	data->str[pos].vr.xoff += FigureDeviceTable(&pst->u.pos.adjust->xadjust,data->pixelsize);
	data->str[pos].vr.yoff += FigureDeviceTable(&pst->u.pos.adjust->yadjust,data->pixelsize);
	data->str[pos].vr.h_adv_off += FigureDeviceTable(&pst->u.pos.adjust->xadv,data->pixelsize);
	data->str[pos].vr.v_adv_off += FigureDeviceTable(&pst->u.pos.adjust->yadv,data->pixelsize);
    }
#endif
return( pos+1 );
}

static int ApplyPairPosAtPos(struct lookup_subtable *sub,struct lookup_data *data,int pos) {
    PST *pst;
    int npos, isv, within;
    KernPair *kp;

    npos = skipglyphs(sub->lookup->lookup_flags,data,pos+1);
    if ( npos>=data->cnt )
return( 0 );
    if ( sub->kc!=NULL ) {
	within = KCFindIndex(sub->kc,data->str[pos].sc->name,data->str[npos].sc->name);
	if ( within==-1 )
return( 0 );
	data->str[pos].kc_index = within;
	data->str[pos].kc = sub->kc;
	if ( sub->vertical_kerning ) {
	    data->str[pos].vr.v_adv_off += rint( sub->kc->offsets[within] * data->scale );
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    data->str[pos].vr.v_adv_off += FigureDeviceTable(&sub->kc->adjusts[within],data->pixelsize);
#endif
	} else if ( sub->lookup->lookup_flags & pst_r2l ) {
	    data->str[npos].vr.h_adv_off += rint( sub->kc->offsets[within] * data->scale );
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    data->str[npos].vr.h_adv_off += FigureDeviceTable(&sub->kc->adjusts[within],data->pixelsize);
#endif
	} else {
	    data->str[pos].vr.h_adv_off += rint( sub->kc->offsets[within] * data->scale );
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    data->str[pos].vr.h_adv_off += FigureDeviceTable(&sub->kc->adjusts[within],data->pixelsize);
#endif
	}
return( pos+1 );
    } else {
	for ( pst=data->str[pos].sc->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->subtable==sub && strcmp(pst->u.pair.paired,data->str[npos].sc->name)==0 ) {
		data->str[pos].vr.xoff += rint( pst->u.pair.vr[0].xoff * data->scale);
		data->str[pos].vr.yoff += rint( pst->u.pair.vr[0].yoff * data->scale);
		data->str[pos].vr.h_adv_off += rint( pst->u.pair.vr[0].h_adv_off * data->scale);
		data->str[pos].vr.v_adv_off += rint( pst->u.pair.vr[0].v_adv_off * data->scale);
		data->str[npos].vr.xoff += rint( pst->u.pair.vr[1].xoff * data->scale);
		data->str[npos].vr.yoff += rint( pst->u.pair.vr[1].yoff * data->scale);
		data->str[npos].vr.h_adv_off += rint( pst->u.pair.vr[1].h_adv_off * data->scale);
		data->str[npos].vr.v_adv_off += rint( pst->u.pair.vr[1].v_adv_off * data->scale);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		/* I got bored. I should do all of them */
		if ( pst->u.pair.vr[0].adjust!=NULL ) {
		    data->str[pos].vr.h_adv_off += FigureDeviceTable(&pst->u.pair.vr[0].adjust->xadv,data->pixelsize);
		}
#endif
return( pos+1 );	/* We do NOT want to return npos+1 */
	    }
	}
	for ( isv = 0; isv<2; ++isv ) {
	    for ( kp = isv ? data->str[pos].sc->vkerns : data->str[pos].sc->kerns; kp!=NULL; kp=kp->next ) {
		if ( kp->subtable == sub && kp->sc == data->str[npos].sc ) {
		    data->str[pos].kp = kp;
		    if ( isv ) {
			data->str[pos].vr.v_adv_off += rint( kp->off * data->scale);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
			data->str[pos].vr.v_adv_off += FigureDeviceTable(kp->adjust,data->pixelsize);
#endif
		    } else if ( sub->lookup->lookup_flags & pst_r2l ) {
			data->str[npos].vr.h_adv_off += rint( kp->off * data->scale);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
			data->str[npos].vr.h_adv_off += FigureDeviceTable(kp->adjust,data->pixelsize);
#endif
		    } else {
			data->str[pos].vr.h_adv_off += rint( kp->off * data->scale);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
			data->str[pos].vr.h_adv_off += FigureDeviceTable(kp->adjust,data->pixelsize);
#endif
		    }
return( pos+1 );
		}
	    }
	}
    }
	    
return( 0 );
}

static int ApplyAnchorPosAtPos(struct lookup_subtable *sub,struct lookup_data *data,int pos) {
    AnchorPoint *ap1, *ap2;
    int npos;
    int any = 0;

    for ( ap1=data->str[pos].sc->anchor; ap1!=NULL ; ap1=ap1->next )
	ap1->ticked = false;

    npos = skipglyphs(sub->lookup->lookup_flags,data,pos+1);
    /* there may be several marks which attach at different anchor classes */
    /* in the same subtable. Ligatures may get several marks in the same */
    /* class. Cursives have only a single match */

    for ( ; npos<data->cnt &&
		(sub->lookup->lookup_type != gpos_mark2ligature ||
		 data->str[npos].lig_pos!=-1); ) {
	if ( sub->lookup->lookup_type == gpos_cursive ) {
	    for ( ap1=data->str[pos].sc->anchor; ap1!=NULL ; ap1=ap1->next ) {
		if ( ap1->anchor->subtable==sub && ap1->type==at_cexit && !ap1->ticked ) {
		    for ( ap2 = data->str[npos].sc->anchor; ap2!=NULL; ap2=ap2->next ) {
			if ( ap2->anchor==ap1->anchor && ap2->type==at_centry )
		    break;
		    }
		    if ( ap2!=NULL )
	    break;
		}
	    }
	} else if ( sub->lookup->lookup_type == gpos_mark2ligature ) {
	    for ( ap1=data->str[pos].sc->anchor; ap1!=NULL ; ap1=ap1->next ) {
		if ( ap1->anchor->subtable==sub && ap1->type==at_baselig &&
			ap1->lig_index == data->str[npos].lig_pos && !ap1->ticked ) {
		    for ( ap2 = data->str[npos].sc->anchor; ap2!=NULL; ap2=ap2->next ) {
			if ( ap2->anchor==ap1->anchor && ap2->type==at_mark )
		    break;
		    }
		    if ( ap2!=NULL )
	    break;
		}
	    }
	} else {
	    for ( ap1=data->str[pos].sc->anchor; ap1!=NULL ; ap1=ap1->next ) {
		if ( ap1->anchor->subtable==sub &&
			(ap1->type==at_basechar || ap1->type==at_basemark) &&
			!ap1->ticked ) {
		    for ( ap2 = data->str[npos].sc->anchor; ap2!=NULL; ap2=ap2->next ) {
			if ( ap2->anchor==ap1->anchor && ap2->type==at_mark )
		    break;
		    }
		    if ( ap2!=NULL )
	    break;
		}
	    }
	}
	if ( ap1==NULL )
return( any ? pos+1 : 0 );
	ap1->ticked = true;

	data->str[npos].vr.yoff = data->str[pos].vr.yoff +
		rint((ap1->me.y - ap2->me.y) * data->scale);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	data->str[npos].vr.yoff += FigureDeviceTable(&ap1->yadjust,data->pixelsize)-
		    FigureDeviceTable(&ap2->yadjust,data->pixelsize);
#endif
	if ( sub->lookup->lookup_flags&pst_r2l ) {
	    data->str[npos].vr.xoff = data->str[pos].vr.xoff +
		    rint( -(ap1->me.x - ap2->me.x)*data->scale );
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    data->str[npos].vr.xoff -= FigureDeviceTable(&ap1->xadjust,data->pixelsize)-
			FigureDeviceTable(&ap2->xadjust,data->pixelsize);
#endif
	} else {
	    data->str[npos].vr.xoff = data->str[pos].vr.xoff +
		    rint( (ap1->me.x - ap2->me.x - data->str[pos].sc->width)*data->scale );
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    data->str[npos].vr.xoff += FigureDeviceTable(&ap1->xadjust,data->pixelsize)-
			FigureDeviceTable(&ap2->xadjust,data->pixelsize);
#endif
	}

	++any;
	if ( sub->lookup->lookup_type == gpos_cursive )
return( pos+1 );
	npos = skipglyphs(sub->lookup->lookup_flags,data,npos+1);
    }
return( any ? pos+1 : 0 );
}

static int ConditionalTagOk(uint32 tag, OTLookup *otl,struct lookup_data *data,int pos) {
    int npos, bpos;
    uint32 script;
    int before_in_script, after_in_script;

    if ( tag==CHR('i','n','i','t') || tag==CHR('i','s','o','l') ||
	    tag==CHR('f','i','n','a') || tag==CHR('m','e','d','i') ) {
	npos = skipglyphs(otl->lookup_flags,data,pos+1);
	bpos = bskipglyphs(otl->lookup_flags,data,pos-1);
	script = SCScriptFromUnicode(data->str[pos].sc);
	before_in_script = (bpos>=0 && SCScriptFromUnicode(data->str[bpos].sc)==script);
	after_in_script = (npos<data->cnt && SCScriptFromUnicode(data->str[npos].sc)==script);
	if ( tag==CHR('i','n','i','t') )
return( !before_in_script && after_in_script );
	else if ( tag==CHR('i','s','o','l') )
return( !before_in_script && !after_in_script );
	else if ( tag==CHR('f','i','n','a') )
return( before_in_script && !after_in_script );
	else
return( before_in_script && after_in_script );
    }

return( true );
}

static int ApplyLookupAtPos(uint32 tag, OTLookup *otl,struct lookup_data *data,int pos) {
    struct lookup_subtable *sub;
    int newpos;

    /* Some tags imply a conditional check. Do that now */
    if ( !ConditionalTagOk(tag,otl,data,pos))
return( 0 );

    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
	switch ( otl->lookup_type ) {
	  case gsub_single:
	    newpos = ApplySingleSubsAtPos(sub,data,pos);
	  break;
	  case gsub_multiple:
	    newpos = ApplyMultSubsAtPos(sub,data,pos);
	  break;
	  case gsub_alternate:
	    newpos = ApplyAltSubsAtPos(sub,data,pos);
	  break;
	  case gsub_ligature:
	    newpos = ApplyLigatureSubsAtPos(sub,data,pos);
	  break;
	  case gsub_context:
	    newpos = ApplyContextual(sub,data,pos);
	  break;
	  case gsub_contextchain:
	    newpos = ApplyContextual(sub,data,pos);
	  break;
	  case gsub_reversecchain:
	    newpos = ApplySingleSubsAtPos(sub,data,pos);
	  break;

	  case gpos_single:
	    newpos = ApplySinglePosAtPos(sub,data,pos);
	  break;
	  case gpos_pair:
	    newpos = ApplyPairPosAtPos(sub,data,pos);
	  break;
	  case gpos_cursive:
	    newpos = ApplyAnchorPosAtPos(sub,data,pos);
	  break;
	  case gpos_mark2base:
	    newpos = ApplyAnchorPosAtPos(sub,data,pos);
	  break;
	  case gpos_mark2ligature:
	    newpos = ApplyAnchorPosAtPos(sub,data,pos);
	  break;
	  case gpos_mark2mark:
	    newpos = ApplyAnchorPosAtPos(sub,data,pos);
	  break;
	  case gpos_context:
	    newpos = ApplyContextual(sub,data,pos);
	  break;
	  case gpos_contextchain:
	    newpos = ApplyContextual(sub,data,pos);
	  break;
	  default:
	    /* apple state machines */
	    newpos = 0;
	  break;
	}
	/* if a subtable worked, we don't try to apply the next one */
	if ( newpos!=0 )
return( newpos );
    }
return( 0 );
}

static void ApplyLookup(uint32 tag, OTLookup *otl,struct lookup_data *data) {
    int pos, npos;
    int lt = otl->lookup_type;

    if ( lt == morx_indic || lt == morx_context || lt == morx_insert ||
	    lt == kern_statemachine )
	ApplyAppleStateMachine(tag,otl,data);
    else {
	/* OpenType */
	for ( pos = 0; pos<data->cnt; ) {
	    npos = ApplyLookupAtPos(tag,otl,data,pos);
	    if ( npos==0 )
		npos = pos+1;
	    pos = npos;
	}
    }
}

static uint32 FSLLMatches(FeatureScriptLangList *fl,uint32 *flist,uint32 script,uint32 lang) {
    int i,l;
    struct scriptlanglist *sl;

    if ( flist==NULL )
return( 0 );

    while ( fl!=NULL ) {
	for ( i=0; flist[i]!=0; ++i ) {
	    if ( fl->featuretag==flist[i] )
	break;
	}
	if ( flist[i]!=0 ) {
	    for ( sl=fl->scripts; sl!=NULL; sl=sl->next ) {
		if ( sl->script == script ) {
		    if ( fl->ismac )	/* Language irrelevant on macs (scripts too, but we pretend they matter) */
return( fl->featuretag );
		    for ( l=0; l<sl->lang_cnt; ++l )
			if ( (l<MAX_LANG && sl->langs[l]==lang) ||
				(l>=MAX_LANG && sl->morelangs[l-MAX_LANG]==lang))
return( fl->featuretag );
		}
	    }
	}
	fl = fl->next;
    }
return( 0 );
}

/* This routine takes a string of glyphs and applies the opentype transformations */
/*  indicated by the features (and script and language) we are passed, it returns */
/*  a transformed string with substitutions applied and containing positioning */
/*  info */
struct opentype_str *ApplyTickedFeatures(SplineFont *sf,uint32 *flist, uint32 script, uint32 lang,
	int pixelsize, SplineChar **glyphs) {
    int isgpos, cnt;
    OTLookup *otl;
    struct lookup_data data;
    uint32 *langs, templang;
    int i;

    memset(&data,0,sizeof(data));
    for ( cnt=0; glyphs[cnt]!=NULL; ++cnt );
    data.str = gcalloc(cnt+1,sizeof(struct opentype_str));
    data.cnt = data.max = cnt;
    for ( cnt=0; glyphs[cnt]!=NULL; ++cnt ) {
	data.str[cnt].sc = glyphs[cnt];
	data.str[cnt].orig_index = cnt;
	data.str[cnt].lig_pos = data.str[cnt].context_pos = -1;
    }
    if ( sf->cidmaster!=NULL ) sf=sf->cidmaster;
    data.sf = sf;
    data.pixelsize = pixelsize;
    data.scale = pixelsize/(double) (sf->ascent+sf->descent);

    /* Indic glyph reordering???? */
    for ( isgpos=0; isgpos<2; ++isgpos ) {
	/* Check that this table has an entry for this language */
	/*  if it doesn't use the default language */
	/* GPOS/GSUB may have different language sets, so we must be prepared */
	templang = lang;
	langs = SFLangsInScript(sf,isgpos,script);
	for ( i=0; langs[i]!=0 && langs[i]!=lang; ++i );
	if ( langs[i]==0 )
	    templang = DEFAULT_LANG;
	free(langs);

	for ( otl = isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL ; otl = otl->next ) {
	    uint32 tag;
	    if ( (tag=FSLLMatches(otl->features,flist,script,templang))!=0 )
		ApplyLookup(tag,otl,&data);
	}
    }
    LigatureFree(&data);
    free(data.ligs);

    data.str = grealloc(data.str,(data.cnt+1)*sizeof(struct opentype_str));
    memset(&data.str[data.cnt],0,sizeof(struct opentype_str));
return( data.str );
}

static void doreplace(char **haystack,char *start,char *search,char *rpl,int slen) {
    int rlen;
    char *pt = start+slen;

    rlen = strlen(rpl);
    if ( slen>=rlen ) {
	memcpy(start,rpl,rlen);
	if ( slen>rlen ) {
	    int diff = slen-rlen;
	    for ( ; *pt ; ++pt )
		pt[-diff] = *pt;
	    pt[-diff] = '\0';
	}
    } else {
	char *base = *haystack;
	char *new = galloc(pt-base+strlen(pt)+rlen-slen+1);
	memcpy(new,base,start-base);
	memcpy(new+(start-base),rpl,rlen);
	strcpy(new+(start-base)+rlen,pt);
	free( base );
	*haystack = new;
    }
}

static int rplstr(char **haystack,char *search, char *rpl,int multipleoccurances) {
    char *start, *pt, *base = *haystack;
    int ch, match, slen = strlen(search);
    int any = 0;

    if ( base==NULL )
return( false );

    for ( pt=base ; ; ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
return( any );
	start=pt;
	while ( *pt!=' ' && *pt!='\0' ) ++pt;
	if ( pt-start!=slen )
	    match = -1;
	else {
	    ch = *pt; *pt='\0';
	    match = strcmp(start,search);
	    *pt = ch;
	}
	if ( match==0 ) {
	    doreplace(haystack,start,search,rpl,slen);
	    if ( !multipleoccurances )
return( true );
	    any = true;
	    if ( base!=*haystack ) {
		pt = *haystack + (start-base)+strlen(rpl);
		base = *haystack;
	    } else
		pt = start+strlen(rpl);
	}
    }
}

static int rplglyphname(char **haystack,char *search, char *rpl) {
    /* If we change "f" to "uni0066" then we should also change "f.sc" to */
    /*  "uni0066.sc" and "f_f_l" to "uni0066_uni0066_l" */
    char *start, *pt, *base = *haystack;
    int ch, match, slen = strlen(search);
    int any = 0;

    if ( slen>=strlen( base ))
return( false );

    for ( pt=base ; ; ) {
	while ( *pt=='_' ) ++pt;
	if ( *pt=='\0' || *pt=='.' )
return( any );
	start=pt;
	while ( *pt!='_' && *pt!='\0' && *pt!='.' ) ++pt;
	if ( *pt=='\0' && start==base )	/* Don't change any unsegmented names */
return( false );			/* In particular don't rename ourselves*/
	if ( pt-start!=slen )
	    match = -1;
	else {
	    ch = *pt; *pt='\0';
	    match = strcmp(start,search);
	    *pt = ch;
	}
	if ( match==0 ) {
	    doreplace(haystack,start,search,rpl,slen);
	    any = true;
	    if ( base!=*haystack ) {
		pt = *haystack + (start-base) + strlen(rpl);
		base = *haystack;
	    } else
		pt = start+strlen(rpl);
	}
    }
}

static int glyphnameIsComponent(char *haystack,char *search) {
    /* Check for a glyph name in ligature names and dotted names */
    char *start, *pt;
    int slen = strlen(search);

    if ( slen>=strlen( haystack ))
return( false );

    for ( pt=haystack ; ; ) {
	while ( *pt=='_' ) ++pt;
	if ( *pt=='\0' || *pt=='.' )
return( false );
	start=pt;
	while ( *pt!='_' && *pt!='\0' && *pt!='.' ) ++pt;
	if ( *pt=='\0' && start==haystack )/* Don't change any unsegmented names */
return( false );			/* In particular don't rename ourselves*/
	if ( pt-start==slen && strncmp(start,search,slen)==0 )
return( true );
    }
}

static int gvfixup(struct glyphvariants *gv,char *old, char *new) {
    int i;
    int ret=0;

    if ( gv==NULL )
return( false );
    ret = rplstr(&gv->variants,old,new,false);
    for ( i=0; i<gv->part_cnt; ++i ) {
	if ( strcmp(gv->parts[i].component,old)==0 ) {
	    free( gv->parts[i].component);
	    gv->parts[i].component = copy(new);
	    ret = true;
	}
    }
return( ret );
}

void SFGlyphRenameFixup(SplineFont *sf, char *old, char *new) {
    int k, gid, isv;
    int i,r;
    SplineFont *master = sf;
    SplineChar *sc;
    PST *pst;
    FPST *fpst;
    KernClass *kc;
    ASM *sm;

    if ( sf->cidmaster!=NULL )
	master = sf->cidmaster;

    /* Look through all substitutions (and pairwise psts) stored on the glyphs*/
    /*  and change any occurances of the name */
    /* (KernPairs have a reference to the SC rather than the name, and need no fixup) */
    /* Also if the name is "f" then look for glyph names like "f.sc" or "f_f_l"*/
    /*  and be ready to change them too */
    k = 0;
    do {
	sf = k<master->subfontcnt ? master->subfonts[k] : master;
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL ) {
	    if ( glyphnameIsComponent(sc->name,old)) {
		char *newer = copy(sc->name);
		rplglyphname(&newer,old,new);
		SFGlyphRenameFixup(master,sc->name,newer);
		free(sc->name);
		sc->name = newer;
		sc->namechanged = sc->changed = true;
	    }
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->type==pst_substitution || pst->type==pst_alternate ||
			pst->type==pst_multiple || pst->type==pst_pair ||
			pst->type==pst_ligature ) {
		    if ( rplstr(&pst->u.mult.components,old,new,pst->type==pst_ligature))
			sc->changed = true;
		}
	    }
	    /* For once I don't want a short circuit eval of "or", so I use */
	    /*  bitwise rather than boolean intentionally */
	    if ( gvfixup(sc->vert_variants,old,new) |
		    gvfixup(sc->horiz_variants,old,new))
		sc->changed = true;
	}
	++k;
    } while ( k<master->subfontcnt );

    /* Now look for contextual fpsts which might use the name */
    for ( fpst=master->possub; fpst!=NULL; fpst=fpst->next ) {
	if ( fpst->format==pst_class ) {
	    for ( i=0; i<fpst->nccnt; ++i ) if ( fpst->nclass[i]!=NULL ) {
		if ( rplstr(&fpst->nclass[i],old,new,false))
	    break;
	    }
	    for ( i=0; i<fpst->bccnt; ++i ) if ( fpst->bclass[i]!=NULL ) {
		if ( rplstr(&fpst->bclass[i],old,new,false))
	    break;
	    }
	    for ( i=0; i<fpst->fccnt; ++i ) if ( fpst->fclass[i]!=NULL ) {
		if ( rplstr(&fpst->fclass[i],old,new,false))
	    break;
	    }
	}
	for ( r=0; r<fpst->rule_cnt; ++r ) {
	    struct fpst_rule *rule = &fpst->rules[r];
	    if ( fpst->type==pst_glyphs ) {
		rplstr(&rule->u.glyph.names,old,new,true);
		rplstr(&rule->u.glyph.back,old,new,true);
		rplstr(&rule->u.glyph.fore,old,new,true);
	    } else if ( fpst->type==pst_coverage ||
		    fpst->type==pst_reversecoverage ) {
		for ( i=0; i<rule->u.coverage.ncnt ; ++i )
		    rplstr(&rule->u.coverage.ncovers[i],old,new,false);
		for ( i=0; i<rule->u.coverage.bcnt ; ++i )
		    rplstr(&rule->u.coverage.bcovers[i],old,new,false);
		for ( i=0; i<rule->u.coverage.fcnt ; ++i )
		    rplstr(&rule->u.coverage.fcovers[i],old,new,false);
		if ( fpst->type==pst_reversecoverage )
		    rplstr(&rule->u.rcoverage.replacements,old,new,true);
	    }
	}
    }

    /* Now look for contextual apple state machines which might use the name */
    for ( sm = master->sm; sm!=NULL; sm=sm->next ) {
	for ( i=0; i<sm->class_cnt; ++i ) if ( sm->classes[i]!=NULL ) {
	    if ( rplstr(&sm->classes[i],old,new,false))
	break;
	}
    }

    /* Now look for contextual kerning classes which might use the name */
    for ( isv=0; isv<2; ++isv ) {
	for ( kc=isv ? master->vkerns : master->kerns; kc!=NULL; kc=kc->next ) {
	    for ( i=0; i<kc->first_cnt; ++i ) if ( kc->firsts[i]!=NULL ) {
		if ( rplstr(&kc->firsts[i],old,new,false))
	    break;
	    }
	    for ( i=0; i<kc->second_cnt; ++i ) if ( kc->seconds[i]!=NULL ) {
		if ( rplstr(&kc->seconds[i],old,new,false))
	    break;
	    }
	}
    }
}
