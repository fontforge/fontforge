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
#include <utype.h>

#include "ttf.h"

/* This file contains routines to create some of the Apple Advanced Typography Tables */

/* ************************************************************************** */
/* *************************    The 'acnt' table    ************************* */
/* ************************************************************************** */

void aat_dumpacnt(struct alltabs *at, SplineFont *sf) {
}

/* ************************************************************************** */
/* *************************    The 'kern' table    ************************* */
/* ************************************************************************** */


/* Apple's docs imply that kerning info is always provided left to right, even*/
/*  for right to left scripts. If that be so then we need code in here to reverse */
/*  the order of the characters for right to left since pfaedit's convention */
/*  is to follow writing order rather than to go left to right */


static void DumpKernClass(FILE *file, uint16 *class,int cnt,int add,int mul) {
    int i, first=-1, last;

    for ( i=0; i<cnt; ++i ) {
	if ( class[i] ) last = i;
	if ( class[i] && first==-1 ) first = i;
    }
    putshort(file,first);
    putshort(file,last-first+1);
    for ( i=first; i<=last; ++i )
	putshort(file,class[i]*mul+add);
}

static int SLIHasDefault(SplineFont *sf,int sli) {
    struct script_record *sr = sf->script_lang[sli];
    int i, j;

    for ( i=0; sr[i].script!=0; ++i )
	for ( j=0; sr[i].langs[j]!=0; ++j )
	    if ( sr[i].langs[j]==DEFAULT_LANG )
return( true );

return( false );
}

void ttf_dumpkerns(struct alltabs *at, SplineFont *sf) {
    int i, cnt, j, k, m, threshold=0, kccnt=0;
    KernPair *kp;
    KernClass *kc;
    uint16 *glnum, *offsets;
    int version;

#if 0
    /* I'm told that at most 2048 kern pairs are allowed in a ttf font */
    /*  ... but can't find any data to back up that claim */
    threshold = KernThreshold(sf,2048);
#endif

    cnt = m = 0;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	j = 0;
	for ( kp = sf->chars[i]->kerns; kp!=NULL; kp=kp->next )
	    if ( kp->off>=threshold || kp->off<=-threshold ) 
		++cnt, ++j;
	if ( j>m ) m=j;
    }
    kccnt = 0;
    for ( kc=sf->kerns; kc!=NULL; kc = kc->next ) if ( SLIHasDefault(sf,kc->sli) )
	++kccnt;
    if ( cnt==0 && kccnt==0 )
return;

    /* Old kerning format (version 0) uses 16 bit quantities */
    /* Apple's new format (version 0x00010000) uses 32 bit quantities */
    at->kern = tmpfile();
    if ( kccnt==0 ) {
	/* MS does not support format 2 kern sub-tables so if we have them */
	/*	(also MS docs are different from Apple's docs on this subtable) */
	/*  we might as well admit that this table is for apple only and use */
	/*  the new format apple recommends. Otherwise, use the old format */
	putshort(at->kern,0);		/* version */
	putshort(at->kern,1);		/* number of subtables */
	version = 0;
    } else {
	putlong(at->kern,0x00010000);
	putlong(at->kern,kccnt+(cnt!=0));
	version = 1;
    }
    if ( cnt!=0 ) {
	if ( version==0 ) {
	    putshort(at->kern,0);		/* subtable version */
	    putshort(at->kern,(7+3*cnt)*sizeof(uint16)); /* subtable length */
	    putshort(at->kern,1);		/* coverage, flags&format */
	} else {
	    putlong(at->kern,(8+3*cnt)*sizeof(uint16)); /* subtable length */
	    /* Apple's new format has a completely different coverage format */
	    putshort(at->kern,0);		/* format 0, no flags (coverage)*/
	    putshort(at->kern,0);		/* tuple index, whatever that means */
	}
	putshort(at->kern,cnt);
	for ( i=1,j=0; i<=cnt; i<<=1, ++j );
	i>>=1; --j;
	putshort(at->kern,i*6);		/* binary search headers */
	putshort(at->kern,j);
	putshort(at->kern,6*(i-cnt));

	glnum = galloc(m*sizeof(uint16));
	offsets = galloc(m*sizeof(uint16));
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	    m = 0;
	    for ( kp = sf->chars[i]->kerns; kp!=NULL; kp=kp->next ) {
		if ( kp->off>=threshold || kp->off<=-threshold ) {
		    /* order the pairs */
		    for ( j=0; j<m; ++j )
			if ( kp->sc->ttf_glyph<glnum[j] )
		    break;
		    for ( k=m; k>j; --k ) {
			glnum[k] = glnum[k-1];
			offsets[k] = offsets[k-1];
		    }
		    glnum[j] = kp->sc->ttf_glyph;
		    offsets[j] = kp->off;
		    ++m;
		}
	    }
	    for ( j=0; j<m; ++j ) {
		putshort(at->kern,sf->chars[i]->ttf_glyph);
		putshort(at->kern,glnum[j]);
		putshort(at->kern,offsets[j]);
	    }
	}
	free(offsets);
	free(glnum);
    }
    for ( kc=sf->kerns; kc!=NULL; kc=kc->next ) if ( SLIHasDefault(sf,kc->sli) ) {
	/* If we are here, we must be using version 1 */
	uint32 len_pos = ftell(at->kern), pos;
	uint16 *class1, *class2;

	putlong(at->kern,0); /* subtable length */
	putshort(at->kern,2);		/* format 2, no flags (coverage)*/
	putshort(at->kern,0);		/* tuple index, whatever that means */

	putshort(at->kern,sizeof(uint16)*kc->second_cnt);
	putshort(at->kern,0);		/* left classes */
	putshort(at->kern,0);		/* right classes */
	putshort(at->kern,16);		/* Offset to array, next byte */
	for ( i=0; i<kc->first_cnt*kc->second_cnt; ++i )
	    putshort(at->kern,kc->offsets[i]);

	pos = ftell(at->kern);
	fseek(at->kern,len_pos+10,SEEK_SET);
	putshort(at->kern,pos-len_pos);
	fseek(at->kern,pos,SEEK_SET);
	class1 = ClassesFromNames(sf,kc->firsts,kc->first_cnt,at->maxp.numGlyphs,NULL);
	DumpKernClass(at->kern,class1,at->maxp.numGlyphs,16,sizeof(uint16)*kc->second_cnt);
	free(class1);

	pos = ftell(at->kern);
	fseek(at->kern,len_pos+12,SEEK_SET);
	putshort(at->kern,pos-len_pos);
	fseek(at->kern,pos,SEEK_SET);
	class2 = ClassesFromNames(sf,kc->seconds,kc->second_cnt,at->maxp.numGlyphs,NULL);
	DumpKernClass(at->kern,class2,at->maxp.numGlyphs,0,sizeof(uint16));
	free(class2);

	pos = ftell(at->kern);
	fseek(at->kern,len_pos,SEEK_SET);
	putlong(at->kern,pos-len_pos);
	fseek(at->kern,pos,SEEK_SET);
    }

    at->kernlen = ftell(at->kern);
    if ( at->kernlen&2 )
	putshort(at->kern,0);		/* pad it */
}

/* ************************************************************************** */
/* *************************    The 'lcar' table    ************************* */
/* ************************************************************************** */

static PST *haslcaret(SplineChar *sc) {
    PST *pst; int j;

    for ( pst=sc->possub; pst!=NULL && pst->type!=pst_lcaret; pst=pst->next );
    if ( pst!=NULL ) {
	for ( j=pst->u.lcaret.cnt-1; j>=0 && pst->u.lcaret.carets[j]==0; --j );
	if ( j==-1 )
	    pst = NULL;
    }
return( pst );
}

void aat_dumplcar(struct alltabs *at, SplineFont *sf) {
    int i, j, k, l, seg_cnt, tot, last, offset;
    PST *pst;
    FILE *lcar=NULL;
    /* We do four passes. The first just calculates how much space we will need */
    /*  the second provides the top-level lookup table structure */
    /*  the third provides the arrays of offsets needed for type 4 lookup tables */
    /*  the fourth provides the actual data on the ligature carets */

    for ( k=0; k<4; ++k ) {
	for ( i=seg_cnt=tot=0; i<sf->charcnt; ++i )
		if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=-1 &&
		    (pst = haslcaret(sf->chars[i]))!=NULL ) {
	    if ( k==1 )
		tot = 0;
	    else if ( k==2 ) {
		putshort(lcar,offset);
		offset += 2 + 2*pst->u.lcaret.cnt;
	    } else if ( k==3 ) {
		putshort(lcar,pst->u.lcaret.cnt);
		for ( l=0; l<pst->u.lcaret.cnt; ++l )
		    putshort(lcar,pst->u.lcaret.carets[l]);
	    }
	    last = i;
	    for ( j=i+1, ++tot; j<sf->charcnt; ++j ) if ( sf->chars[j]!=NULL && sf->chars[j]->ttf_glyph!=-1 ) {
		if ( (pst = haslcaret(sf->chars[j]))== NULL )
	    break;
		++tot;
		last = j;
		if ( k==2 ) {
		    putshort(lcar,offset);
		    offset += 2 + 2*pst->u.lcaret.cnt;
		} else if ( k==3 ) {
		    putshort(lcar,pst->u.lcaret.cnt);
		    for ( l=0; l<pst->u.lcaret.cnt; ++l )
			putshort(lcar,pst->u.lcaret.carets[l]);
		}
	    }
	    if ( k==1 ) {
		putshort(lcar,sf->chars[last]->ttf_glyph);
		putshort(lcar,sf->chars[i]->ttf_glyph);
		putshort(lcar,offset);
		offset += 2*tot;
	    }
	    ++seg_cnt;
	    i = j-1;
	}
	if ( k==0 ) {
	    if ( seg_cnt==0 )
return;
	    lcar = tmpfile();
	    putlong(lcar, 0x00010000);	/* version */
	    putshort(lcar,0);		/* data are distances (not points) */

	    putshort(lcar,4);		/* Lookup table format 4 */
		/* Binary search header */
	    putshort(lcar,6);		/* Entry size */
	    putshort(lcar,seg_cnt);	/* Number of segments */
	    for ( j=0,l=1; l<=seg_cnt; l<<=1, ++j );
	    --j; l>>=1;
	    putshort(lcar,6*l);
	    putshort(lcar,j);
	    putshort(lcar,6*(seg_cnt-l));
	    offset = 4+7*2 + seg_cnt*6 + 6;
	} else if ( k==1 ) {		/* flag entry */
	    putshort(lcar,0xffff);
	    putshort(lcar,0xffff);
	    putshort(lcar,0);
	}
    }
    at->lcar = lcar;
    at->lcarlen = ftell(at->lcar);
    if ( at->lcarlen&2 )
	putshort(at->lcar,0);
}

/* ************************************************************************** */
/* *************************    The 'morx' table    ************************* */
/* *************************      (and 'feat')      ************************* */
/* ************************************************************************** */

/* Some otf tags may map to more than one type,setting ('fwid' maps to several) */
/*  I think I shall ignore this complication. 'fwid' will just map to the general Full Width Text and ignore the more specific options */
/* Some type,settings may control more than one otf_tag */
/* I'm also going to break ligatures up into seperate sub-tables depending on */
/*  script, so again there may be multiple tags */
/* (only the default language will be used) */
struct feature {
    uint32 otftag;
    int16 featureType, featureSetting, offSetting;
    char *name, *offname;
    unsigned int ismutex: 1;
    unsigned int defaultOn: 1;
    unsigned int vertOnly: 1;
    unsigned int r2l: 1;	/* I think this is the "descending" flag */
    uint8 subtable_type;
    int chain;
    int32 flag;
    uint32 feature_start;
    uint32 feature_len;		/* Does not include header yet */
    struct feature *next;
};

static struct feature *featureFromTag(uint32 tag);

static void morxfeaturesfree(struct feature *features) {
    struct feature *n;

    for ( ; features!=NULL; features=n ) {
	n = features->next;
	chunkfree( features,sizeof(*features) );
    }
}

static void morx_lookupmap(FILE *temp,SplineChar **glyphs,uint16 *maps,int gcnt) {
    int i, j, k, l, seg_cnt, tot, last, offset;
    /* We do four passes. The first just calculates how much space we will need (if any) */
    /*  the second provides the top-level lookup table structure */
    /*  the third provides the arrays of offsets needed for type 4 lookup tables */

    for ( k=0; k<3; ++k ) {
	for ( i=seg_cnt=tot=0; i<gcnt; ++i ) {
	    if ( k==1 )
		tot = 0;
	    else if ( k==2 ) {
		putshort(temp,maps[i]);
	    }
	    last = i;
	    for ( j=i+1, ++tot; j<gcnt && glyphs[j]->ttf_glyph==glyphs[i]->ttf_glyph+j-i; ++j ) {
		++tot;
		last = j;
		if ( k==2 ) {
		    putshort(temp,maps[j]);
		}
	    }
	    if ( k==1 ) {
		putshort(temp,glyphs[last]->ttf_glyph);
		putshort(temp,glyphs[i]->ttf_glyph);
		putshort(temp,offset);
		offset += 2*tot;
	    }
	    ++seg_cnt;
	    i = j-1;
	}
	if ( k==0 ) {
	    if ( seg_cnt==0 )
return;
	    putshort(temp,4);		/* Lookup table format 4 */
		/* Binary search header */
	    putshort(temp,6);		/* Entry size */
	    putshort(temp,seg_cnt);	/* Number of segments */
	    for ( j=0,l=1; l<=seg_cnt; l<<=1, ++j );
	    --j; l>>=1;
	    putshort(temp,6*l);
	    putshort(temp,j);
	    putshort(temp,6*(seg_cnt-l));
	    offset = 6*2 + seg_cnt*6 + 6;
	} else if ( k==1 ) {		/* flag entry */
	    putshort(temp,0xffff);
	    putshort(temp,0xffff);
	    putshort(temp,0);
	}
    }
}

static void morx_dumpSubsFeature(FILE *temp,SplineChar **glyphs,uint16 *maps,int gcnt) {
    morx_lookupmap(temp,glyphs,maps,gcnt);
}

static int HasDefaultLang(SplineFont *sf,PST *lig,uint32 script) {
    int i, j;
    struct script_record *sr;

    if ( sf->cidmaster ) sf = sf->cidmaster;
    sr = sf->script_lang[lig->script_lang_index];
    for ( i=0; sr[i].script!=0; ++i ) {
	if ( script==0 || script==sr[i].script ) {
	    for ( j=0; sr[i].langs[j]!=0; ++j )
		if ( sr[i].langs[j]==DEFAULT_LANG )
return( true );
	    if ( script!=0 )
return( false );
	}
    }
return( false );
}

static struct feature *aat_dumpmorx_substitutions(struct alltabs *at, SplineFont *sf,
	FILE *temp, struct feature *features) {
    int i, max, cnt, j, k, gcnt;
    uint32 *subtags;
    int ft, fs;
    SplineChar *sc, *msc, **glyphs;
    uint16 *maps;
    struct feature *cur;
    PST *pst;

    max = 30; cnt = 0;
    subtags = galloc(max*sizeof(uint32));

    for ( i=0; i<sf->charcnt; ++i ) if ( (sc = sf->chars[i])!=NULL && sc->ttf_glyph!=-1) {
	for ( pst=sc->possub; pst!=NULL; pst=pst->next ) if ( pst->type == pst_substitution ) {
	    /* Arabic forms (marked by 'isol') are done with a contextual glyph */
	    /*  substitution subtable (cursive connection) */
	    if ( !HasDefaultLang(sf,pst,0) )
	continue;
	    if ( pst->tag!=CHR('i','s','o','l') && pst->type!=pst_position &&
		    OTTagToMacFeature(pst->tag,&ft,&fs)) {
		for ( j=cnt-1; j>=0 && subtags[j]!=pst->tag; --j );
		if ( j<0 ) {
		    if ( cnt>=max )
			subtags = grealloc(subtags,(max+=30)*sizeof(uint32));
		    subtags[cnt++] = pst->tag;
		}
	    }
	}
    }

    if ( cnt!=0 ) {
	for ( j=0; j<cnt; ++j ) {
	    for ( k=0; k<2; ++k ) {
		gcnt = 0;
		for ( i=0; i<sf->charcnt; ++i ) if ( (sc = sf->chars[i])!=NULL && sc->ttf_glyph!=-1) {
		    for ( pst=sc->possub; pst!=NULL &&
			    (pst->tag!=subtags[j] || !HasDefaultLang(sf,pst,0) ||
			     pst->type==pst_position); pst=pst->next );
		    if ( pst!=NULL ) {
			if ( k==1 ) {
			    msc = SFGetCharDup(sf,-1,pst->u.subs.variant);
			    if ( msc!=NULL && msc->ttf_glyph!=-1 ) {
				glyphs[gcnt] = sc;
			        maps[gcnt++] = msc->ttf_glyph;
			    }
			} else
			    ++gcnt;
		    }
		}
		if ( k==0 ) {
		    glyphs = galloc((gcnt+1)*sizeof(SplineChar *));
		    maps = galloc((gcnt+1)*sizeof(uint16));
		} else {
		    glyphs[gcnt] = NULL; maps[gcnt] = 0;
		}
	    }
	    cur = featureFromTag(subtags[j]);
	    cur->next = features;
	    features = cur;
	    cur->subtable_type = 4;
	    cur->feature_start = ftell(temp);
	    morx_dumpSubsFeature(temp,glyphs,maps,gcnt);
	    if ( (ftell(temp)-cur->feature_start)&1 )
		putc('\0',temp);
	    if ( (ftell(temp)-cur->feature_start)&2 )
		putshort(temp,0);
	    cur->feature_len = ftell(temp)-cur->feature_start;
	    free(glyphs); free(maps);
	}
    }
    free(subtags);
return( features);
}

static LigList *LigListMatchOtfTag(SplineFont *sf,LigList *ligs,uint32 tag) {
    LigList *l;

    for ( l=ligs; l!=NULL; l=l->next )
	if ( l->lig->tag == tag && HasDefaultLang(sf,l->lig,0))
return( l );
return( NULL );
}

static PST *HasAForm(SplineFont *sf,PST *pst) {
    for ( ; pst!=NULL; pst=pst->next ) if ( pst->type==pst_substitution ) {
	if ( !HasDefaultLang(sf,pst,0))
    continue;
	if ( pst->tag==CHR('i','n','i','t') ||
		pst->tag==CHR('m','e','d','i') ||
		pst->tag==CHR('f','i','n','a') ||
		pst->tag==CHR('i','s','o','l'))
return( pst );
    }

return( NULL );
}

struct transition { uint16 next_state, dontconsume, trans_ent; LigList *l; };
struct trans_entries { uint16 next_state, flags, act_index; LigList *l; };
static void morx_dumpLigaFeature(FILE *temp,SplineChar **glyphs,int gcnt,
	uint32 otf_tag, struct alltabs *at, SplineFont *sf ) {
    LigList *l;
    struct splinecharlist *comp;
    uint16 *used = gcalloc(at->maxp.numGlyphs,sizeof(uint16));
    SplineChar **cglyphs;
    uint16 *map;
    int i,j,k,class, state_max, state_cnt, base, last;
    uint32 start;
    struct transition **states;
    struct trans_entries *trans;
    int trans_cnt;
    int maxccnt=0;
    int acnt, lcnt;
    uint32 *actions;
    uint16 *components, *lig_glyphs;
    uint32 here;
    struct splinecharlist *scl;

    /* figure out the classes (one for each character used to make a lig) */
    for ( i=0; i<gcnt; ++i ) {
	used[glyphs[i]->ttf_glyph] = true;
	for ( l=glyphs[i]->ligofme; l!=NULL; l=l->next ) if ( l->lig->tag==otf_tag ) {
	    for ( comp = l->components; comp!=NULL; comp=comp->next )
		used[comp->sc->ttf_glyph] = true;
	}
    }
    class = 4;
    for ( i=0; i<at->maxp.numGlyphs; ++i ) if ( used[i] )
	used[i] = class++;
    cglyphs = galloc((class+1)*sizeof(SplineChar *));
    map = galloc((class+1)*sizeof(uint16));
    j=0;
    for ( i=k=0; i<at->maxp.numGlyphs; ++i ) if ( used[i] ) {
	for ( ; j<sf->charcnt; ++j )
	    if ( sf->chars[j]!=NULL && sf->chars[j]->ttf_glyph==i )
	break;
	if ( j<sf->charcnt ) {
	    cglyphs[k] = sf->chars[j];
	    map[k++] = used[i];
	}
    }
    cglyphs[k] = NULL;

    start = ftell(temp);
    putlong(temp,class);
    putlong(temp,7*sizeof(uint32));
    putlong(temp,0);		/* Fill in later */
    putlong(temp,0);
    putlong(temp,0);
    putlong(temp,0);
    putlong(temp,0);
    morx_lookupmap(temp,cglyphs,map,k);	/* dump the class lookup table */
    free( cglyphs ); free( map );
    here = ftell(temp);
    fseek(temp,start+2*sizeof(uint32),SEEK_SET);
    putlong(temp,here-start);			/* Point to start of state arrays */
    fseek(temp,0,SEEK_END);

    /* Now build the state machine */
    /* Note: the ligofme list is so ordered that the longest ligatures come first */
    /*  we will depend on that in the case of "ffl", "ffi", "ff" */
    state_max = 40; state_cnt = 2;
    states = galloc(state_max*sizeof(struct transition *));
    states[0] = gcalloc(class,sizeof(struct transition));	/* Initial state */
    states[1] = gcalloc(class,sizeof(struct transition));	/* other Initial state */
    for ( i=0; i<gcnt; ++i ) {
	if ( state_cnt>=state_max )
	    states = grealloc(states,(state_max += 40)*sizeof(struct transition *));
	base = state_cnt;
	states[0][used[glyphs[i]->ttf_glyph]].next_state = state_cnt;
	states[1][used[glyphs[i]->ttf_glyph]].next_state = state_cnt;
	states[state_cnt++] = gcalloc(class,sizeof(struct transition));
	for ( l=glyphs[i]->ligofme; l!=NULL; l=l->next ) if ( l->lig->tag==otf_tag ) {
	    if ( l->ccnt > maxccnt ) maxccnt = l->ccnt;
	    last = base;
	    for ( comp = l->components; comp!=NULL; comp=comp->next ) {
		if ( states[last][used[comp->sc->ttf_glyph]].next_state==0 ) {
		    if ( comp->next==NULL )
			states[last][used[comp->sc->ttf_glyph]].l = l;
		    else {
			states[last][used[comp->sc->ttf_glyph]].next_state = state_cnt;
			if ( state_cnt>=state_max )
			    states = grealloc(states,(state_max += 40)*sizeof(struct transition *));
			last = state_cnt;
			states[state_cnt++] = gcalloc(class,sizeof(struct transition));
		    }
		} else {
		    last = states[last][used[comp->sc->ttf_glyph]].next_state;
		    if ( comp->next==NULL ) {
			/* this is where we depend on the ordering */
			for ( j=0; j<class; ++j )
			    if ( states[last][j].next_state==0 && states[last][j].l==NULL ) {
				states[last][j].l = l;
				states[last][j].dontconsume = true;
				/* the next state should continue to be 0 (initial) */
			    }
		    }
		}
	    }
	}
    }
    /* Ok, we've got the state machine now. Convert it into apple's wierd */
    /*  (space saving) format */
    trans = galloc(class*state_cnt*sizeof(struct trans_entries));
    trans_cnt = 0;
    for ( i=0; i<state_cnt; ++i ) for ( j=0; j<class; ++j ) {
	for ( k=0; k<trans_cnt; ++k ) {
	    if ( trans[k].next_state==states[i][j].next_state &&
		    trans[k].l ==states[i][j].l )
	break;
	}
	states[i][j].trans_ent = k;
	if ( k==trans_cnt ) {
	    trans[k].next_state = states[i][j].next_state;
	    trans[k].l = states[i][j].l;
	    trans[k].flags = 0;
	    if ( states[i][j].dontconsume )
		trans[k].flags = 0x4000;
	    else if ( trans[k].next_state!=0 || trans[k].l!=NULL )
		trans[k].flags = 0x8000;
	    if ( trans[k].l!=NULL )
		trans[k].flags |= 0x2000;
	    trans[k].act_index = 0;
	    ++trans_cnt;
	}
    }
    /* Dump out the state machine */
    for ( i=0; i<state_cnt; ++i ) for ( j=0; j<class; ++j )
	putshort( temp, states[i][j].trans_ent );

    /* Now figure out the ligature actions (and all the other tables) */
    actions = galloc(trans_cnt*maxccnt*sizeof(uint32));
    components = galloc(trans_cnt*maxccnt*sizeof(uint16));
    lig_glyphs = galloc(trans_cnt*sizeof(uint16));
    acnt = lcnt = 0;
    for ( i=0; i<trans_cnt; ++i ) if ( trans[i].l!=NULL ) {
	lig_glyphs[lcnt] = trans[i].l->lig->u.lig.lig->ttf_glyph;
	/* component Glyphs get popped off the stack in the reverse order */
	/*  so we must built our tables backwards */
	components[acnt+trans[i].l->ccnt-1] = lcnt;
	actions[acnt+trans[i].l->ccnt-1] = 0x80000000 |
		((acnt+trans[i].l->ccnt-1 - trans[i].l->first->ttf_glyph)&0x3fffffff);
	for ( scl=trans[i].l->components,j=trans[i].l->ccnt-2; scl!=NULL; scl=scl->next, --j ) {
	    components[acnt+j] = 0;
	    actions[acnt+j] = (acnt+j - scl->sc->ttf_glyph)&0x3fffffff;
	}
	trans[i].act_index = acnt;
	++lcnt;
	acnt += trans[i].l->ccnt;
    }

    /* Now we know how big all the tables will be. Dump out their locations */
    here = ftell(temp);
    fseek(temp,start+3*sizeof(uint32),SEEK_SET);
    putlong(temp,here-start);			/* Point to start of entry array */
    putlong(temp,here-start+6*trans_cnt);	/* Point to start of actions */
    putlong(temp,here-start+6*trans_cnt+4*acnt);/* Point to start of components */
    putlong(temp,here-start+6*trans_cnt+6*acnt);/* Point to start of ligatures */
    fseek(temp,0,SEEK_END);

    /* Now dump the transitions */
    for ( i=0; i<trans_cnt; ++i ) {
	putshort(temp,trans[i].next_state);
	putshort(temp,trans[i].flags);
	putshort(temp,trans[i].act_index);
    }
    /* And the actions */
    for ( i=0; i<acnt; ++i )
	putlong(temp,actions[i]);
    /* And the components */
    for ( i=0; i<acnt; ++i )
	putshort(temp,components[i]);
    /* Do A simple check on the validity of what we've done */
    if ( here+6*trans_cnt+6*acnt != ftell(temp) )
	fprintf( stderr, "Offset wrong in morx ligature table\n" );
    /* And finally the ligature glyph indeces */
    for ( i=0; i<lcnt; ++i )
	putshort(temp,lig_glyphs[i]);

    /* clean up */
    free(actions); free(components); free(lig_glyphs);
    free(trans);
    for ( i=0; i<state_cnt; ++i )
	free(states[i]);
    free(states);
    free(used);
}

static struct feature *aat_dumpmorx_ligatures(struct alltabs *at, SplineFont *sf,
	FILE *temp, struct feature *features) {
    int i, max, cnt, j, k, gcnt;
    uint32 *ligtags;
    int ft, fs;
    SplineChar *sc, *ssc, **glyphs;
    struct feature *cur;
    LigList *l;

    SFLigaturePrepare(sf);

    max = 30; cnt = 0;
    ligtags = galloc(max*sizeof(uint32));

    for ( i=0; i<sf->charcnt; ++i ) if ( (sc = sf->chars[i])!=NULL && sc->ttf_glyph!=-1) {
	for ( l=sc->ligofme; l!=NULL; l=l->next ) {
	    if ( HasDefaultLang(sf,l->lig,0) && OTTagToMacFeature(l->lig->tag,&ft,&fs)) {
		for ( j=cnt-1; j>=0 && ligtags[j]!=l->lig->tag; --j );
		if ( j<0 ) {
		    if ( cnt>=max )
			ligtags = grealloc(ligtags,(max+=30)*sizeof(uint32));
		    ligtags[cnt++] = l->lig->tag;
		}
	    }
	}
    }
    if ( cnt==0 ) {
	free( ligtags );
	SFLigatureCleanup(sf);
return( features);
    }

    glyphs = galloc((at->maxp.numGlyphs+1)*sizeof(SplineChar *));
    for ( j=0; j<cnt; ++j ) {
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	    sf->chars[i]->ticked = false;
	for ( i=0; i<sf->charcnt; ++i )
		if ( (sc=sf->chars[i])!=NULL && !sc->ticked &&
			LigListMatchOtfTag(sf,sc->ligofme,ligtags[j])) {
	    uint32 script = SCScriptFromUnicode(sc);
	    for ( k=i, gcnt=0; k<sf->charcnt; ++k )
		    if ( (ssc=sf->chars[k])!=NULL && !ssc->ticked &&
			    SCScriptFromUnicode(ssc) == script &&
			    LigListMatchOtfTag(sf,ssc->ligofme,ligtags[j])) {
		glyphs[gcnt++] = ssc;
		ssc->ticked = true;
	    }
	    glyphs[gcnt] = NULL;
	    cur = featureFromTag(ligtags[j]);
	    cur->next = features;
	    features = cur;
	    cur->subtable_type = 2;		/* ligature */
	    cur->feature_start = ftell(temp);
	    morx_dumpLigaFeature(temp,glyphs,gcnt,ligtags[j],at,sf);
	    if ( (ftell(temp)-cur->feature_start)&1 )
		putc('\0',temp);
	    if ( (ftell(temp)-cur->feature_start)&2 )
		putshort(temp,0);
	    cur->feature_len = ftell(temp)-cur->feature_start;
	    cur->r2l = SCRightToLeft(sc);
	}
    }

    free(glyphs);
    free(ligtags);
    SFLigatureCleanup(sf);
return( features);
}

static void morx_dumpContGlyphFeature(FILE *temp,SplineChar **glyphs,uint16 *forms[4], int gcnt,
	struct alltabs *at, SplineFont *sf, uint32 script ) {
    uint16 *used = gcalloc(at->maxp.numGlyphs,sizeof(uint16));
    SplineChar **cglyphs, *sc;
    uint16 *map;
    int i,j,k,f;
    uint32 start, pos, here;
    int any[4];
    PST *pst;

    any[0] = any[1]= any[2]= any[3] = 0;
    /* figure out the classes, only one of any importance: Letter in this script */
    /* might already be formed. Might be a lig. Might be normal */
    for ( i=0; i<gcnt; ++i ) for ( f=0; f<4; ++f) if ( forms[f][i]!=0 ) {
	used[ forms[f][i] ] = 4;
	any[f] = true;
    }
    for ( i=0; i<sf->charcnt; ++i) if ( (sc=sf->chars[i])!=NULL && SCScriptFromUnicode(sc)==script && sc->ttf_glyph!=-1) {
	if ( sc->unicodeenc>=0 && sc->unicodeenc<0x10000 && isalpha(sc->unicodeenc))
	    used[sc->ttf_glyph] = 4;
	else {
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next )
		if ( pst->type==pst_ligature &&
			(pst->tag==CHR('l','i','g','a') || pst->tag==CHR('r','l','i','g') || pst->tag==CHR('d','l','i','g'))) {
		    used[sc->ttf_glyph] = 4;
	    break;
		}	/* T.alt? etc? Too hard */
	}
    }
    for ( i=0; i<gcnt; ++i )
	used[ glyphs[i]->ttf_glyph ] = 4;

    for ( i=k=0; i<at->maxp.numGlyphs; ++i )
	if ( used[i] ) ++k;
    cglyphs = galloc((k+1)*sizeof(SplineChar *));
    map = galloc((k+1)*sizeof(uint16));
    j=0;
    for ( i=k=0; i<at->maxp.numGlyphs; ++i ) if ( used[i] ) {
	for ( ; j<sf->charcnt; ++j )
	    if ( sf->chars[j]!=NULL && sf->chars[j]->ttf_glyph==i )
	break;
	if ( j<sf->charcnt ) {
	    cglyphs[k] = sf->chars[j];
	    map[k++] = used[i];
	}
    }
    cglyphs[k] = NULL;

    start = ftell(temp);
    putlong(temp,5);			/* # classes */
    putlong(temp,5*sizeof(uint32));	/* class offset */
    putlong(temp,0);			/* state offset */
    putlong(temp,0);			/* transition entry offset */
    putlong(temp,0);			/* substitution table offset */
    morx_lookupmap(temp,cglyphs,map,k);	/* dump the class lookup table */
    here = ftell(temp);
    fseek(temp,start+2*sizeof(uint32),SEEK_SET);
    putlong(temp,here-start);			/* Point to start of state arrays */
    putlong(temp,here-start+5*4*sizeof(uint16));/* Point to end of state arrays */
    putlong(temp,here-start+5*4*sizeof(uint16)+8*4*sizeof(uint16));/* Point to end of transitions arrays */
    fseek(temp,0,SEEK_END);

    /* State 0,1 are start states */
    /* State 2 means we have found one interesting letter, and marked it (so it will become either initial or isolated) */
    /* State 3 means we have found two interesting letters, and marked (so the last will be either medial or final) */
    putshort(temp,0); putshort(temp,0); putshort(temp,0); putshort(temp,0); putshort(temp,1);		/* State 0 */
    putshort(temp,0); putshort(temp,0); putshort(temp,0); putshort(temp,0); putshort(temp,1);		/* State 1 */
    putshort(temp,3); putshort(temp,3); putshort(temp,4); putshort(temp,3); putshort(temp,3);		/* State 2 */
    putshort(temp,5); putshort(temp,5); putshort(temp,7); putshort(temp,5); putshort(temp,6);		/* State 3 */

    /* Transition 0 goes to state 0, no substitutions, no marks */
    /* Transition 1 goes to state 2, no subs, marks glyph */
    /* Transition 2 goes to state 3, marked glyph=>initial, marks glyph */
    /* Transition 3 goes to state 0, marked glyph=>isolated, no marks */
    /* Transition 4 goes to state 2, no subs, no marks (for deleted) */
    /* Transition 5 goes to state 3, marked glyph=>medial, marks glyph */
    /* Transition 6 goes to state 0, marked glyph=>final, no marks */
    /* Transition 7 goes to state 3, no subs, no marks (for deleted) */
    putshort(temp,0); putshort(temp,0x0000); putshort(temp,0xffff); putshort(temp,0xffff);
    putshort(temp,2); putshort(temp,0x8000); putshort(temp,0xffff); putshort(temp,0xffff);
    putshort(temp,3); putshort(temp,0x8000); putshort(temp,any[0]?0:0xffff); putshort(temp,0xffff);
    putshort(temp,0); putshort(temp,0x0000); putshort(temp,any[1]?any[0]:0xffff); putshort(temp,0xffff);
    putshort(temp,2); putshort(temp,0x0000); putshort(temp,0xffff); putshort(temp,0xffff);
    putshort(temp,3); putshort(temp,0x8000); putshort(temp,any[2]?any[0]+any[1]:0xffff); putshort(temp,0xffff);
    putshort(temp,0); putshort(temp,0x0000); putshort(temp,any[3]?any[0]+any[1]+any[2]:0xffff); putshort(temp,0xffff);
    putshort(temp,3); putshort(temp,0x0000); putshort(temp,0xffff); putshort(temp,0xffff);

    /* Substitution 0 is initial */
    /* Substitution 1 is isolated */
    /* Substitution 2 is medial */
    /* Substitution 3 is final */
    pos = ftell(temp);
    for ( i=0; i<any[0]+any[1]+any[2]+any[3]; ++i )
	putlong(temp,0);		/* Fixup later */

    j = 0;
    for ( f=0; f<4; ++f ) if ( any[f] ) {
	here = ftell(temp);
	fseek(temp,pos+j++*sizeof(uint32),SEEK_SET);
	putlong(temp,here-pos);
	fseek(temp,0,SEEK_END);
	k=0;
	for ( i=0; i<gcnt; ++i ) if ( forms[f][i]!=0 ) {
	    cglyphs[k] = glyphs[i];
	    map[k++] = forms[f][i];
	}
	cglyphs[k] = NULL;
	morx_lookupmap(temp,cglyphs,map,k);	/* dump this substitution lookup table */
    }

    /* clean up */
    free(used);
    free(cglyphs); free(map);
}

/* take care of arabic "forms" like initial, medial, final, isolated */
/*  could happen in other languages (long-s is initial, medial) */
static struct feature *aat_dumpmorx_glyphforms(struct alltabs *at, SplineFont *sf,
	FILE *temp, struct feature *features) {
    int i, k, gcnt, f;
    SplineChar *sc, *ssc, *rsc, **glyphs;
    uint16 *forms[4];
    struct feature *cur;
    PST *pst;

    glyphs = galloc((at->maxp.numGlyphs+1)*sizeof(SplineChar *));
    for ( i=0; i<4; ++i )
	forms[i] = galloc((at->maxp.numGlyphs+1)*sizeof(uint16));

    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	sf->chars[i]->ticked = false;
    for ( i=0; i<sf->charcnt; ++i )
	    if ( (sc=sf->chars[i])!=NULL && !sc->ticked &&
		    HasAForm(sf,sc->possub)) {
	uint32 script = SCScriptFromUnicode(sc);
	for ( f=0; f<4; ++f) forms[f][0] = 0;
	for ( k=i, gcnt=0; k<sf->charcnt; ++k )
		if ( (ssc=sf->chars[k])!=NULL && !ssc->ticked &&
			SCScriptFromUnicode(ssc) == script ) {
	    int any = false, which;
	    for ( pst=ssc->possub; pst!=NULL; pst=pst->next ) {
		if ( !HasDefaultLang(sf,pst,script))
	    continue;
		if ( pst->tag==CHR('i','n','i','t')) which = 0;
		else if ( pst->tag==CHR('m','e','d','i')) which = 1;
		else if ( pst->tag==CHR('f','i','n','a')) which = 2;
		else if ( pst->tag==CHR('i','s','o','l')) which = 3;
		else
	    continue;
		rsc = SFGetCharDup(sf,-1,pst->u.subs.variant);
		if ( rsc==NULL || rsc->ttf_glyph==-1 )
	    continue;
		forms[which][gcnt] = rsc->ttf_glyph;
		any = true;
	    }
	    if ( any ) {
		glyphs[gcnt++] = ssc;
		for ( f=0; f<4; ++f) forms[f][gcnt] = 0;
	    }
	    ssc->ticked = true;
	}
	glyphs[gcnt] = NULL;
	if ( gcnt!=0 && (cur = featureFromTag(CHR('i','s','o','l')))!=NULL ) {
	    cur->r2l = SCRightToLeft(sc);
	    cur->next = features;
	    features = cur;
	    cur->subtable_type = 1;		/* contextual glyph subs */
	    cur->feature_start = ftell(temp);
	    morx_dumpContGlyphFeature(temp,glyphs,forms,gcnt,at,sf,SCScriptFromUnicode(sc));
	    if ( (ftell(temp)-cur->feature_start)&1 )
		putc('\0',temp);
	    if ( (ftell(temp)-cur->feature_start)&2 )
		putshort(temp,0);
	    cur->feature_len = ftell(temp)-cur->feature_start;
	}
    }
    free(glyphs);
    for ( i=0; i<4; ++i )
	free(forms[i]);

return( features);
}

static struct feature *featuresOrderByType(struct feature *features) {
    struct feature *f, **all;
    int i, j, cnt;

    for ( cnt=0, f=features; f!=NULL; f=f->next, ++cnt );
    if ( cnt==1 )
return( features );
    all = galloc(cnt*sizeof(struct feature *));
    for ( i=0, f=features; f!=NULL; f=f->next, ++i )
	all[i] = f;
    for ( i=0; i<cnt-1; ++i ) for ( j=i+1; j<cnt; ++j ) {
	if ( all[i]->featureType>all[j]->featureType ||
		(all[i]->featureType==all[j]->featureType && all[i]->featureSetting>all[j]->featureSetting )) {
	    f = all[i];
	    all[i] = all[j];
	    all[j] = f;
	}
    }
    for ( i=0; i<cnt; ++i ) {
	if ( all[i]->ismutex && (i==cnt-1 || all[i]->featureType!=all[i+1]->featureType))
	    all[i]->ismutex = false;
	else {
	    while ( i<cnt-1 && all[i]->featureType==all[i+1]->featureType )
		++i;
	}
    }
    for ( i=0; i<cnt-1; ++i )
	all[i]->next = all[i+1];
    all[cnt-1]->next = NULL;
    features = all[0];
    free( all );
return( features );
}

static void aat_dumpfeat(struct alltabs *at, struct feature *feature) {
    int scnt, fcnt, cnt;
    struct feature *f, *n, *p;
    int k;
    uint32 offset;
    int strid = 256;
    int fn=0;
    /* Dump the 'feat' table which is a connection between morx features and */
    /*  the name table */
    /* We do three passes. The first just calculates how much space we will need */
    /*  the second provides names for the feature types */
    /*  and the third provides names for the feature settings */
    /* As we fill up the feat table we also create an array of strings */
    /*  (strid, char *pointer) which will be used by the 'name' table to */
    /*  give names to the features and their settings */

    if ( feature==NULL )
return;

    fcnt = scnt = 0;
    for ( k=0; k<3; ++k ) {
	for ( f=feature; f!=NULL; f=n ) {
	    cnt=1;
	    if ( !f->ismutex && f->offname!=NULL ) cnt = 2;
	    if ( k!=2 ) {
		for ( p=f, n=p->next; n!=NULL && n->featureType==f->featureType; p=n, n=n->next ) {
		    ++cnt;
		    if ( !n->ismutex && n->offname!=NULL ) ++cnt;
		}
	    } else {
		for ( p=f; p!=NULL && p->featureType==f->featureType; p=p->next ) {
		    putshort(at->feat,p->featureSetting);
		    putshort(at->feat,strid);
		    at->feat_name[fn].name = p->name;
		    at->feat_name[fn++].strid = strid++;
		    if ( !p->ismutex ) {
			putshort(at->feat,p->offSetting);
			putshort(at->feat,strid);
			at->feat_name[fn].name = p->offname;
			at->feat_name[fn++].strid = strid++;
		    }
		}
		n = p;
	    }
	    if ( k==0 ) {
		++fcnt;
		scnt += cnt;
	    } else if ( k==1 ) {
		putshort(at->feat,f->featureType);
		putshort(at->feat,cnt);
		putlong(at->feat,offset);
		putshort(at->feat,f->ismutex?0xc000:0);
		putshort(at->feat,strid);
		at->feat_name[fn].name = FeatureNameFromType(f->featureType);
		at->feat_name[fn++].strid = strid++;
		offset += 4*cnt;
	    }
	}
	if ( k==0 ) {
	    ++fcnt;		/* Add one for "All Typographic Features" */
	    scnt += 2;		/* Add one each for All/No Features */
	    at->feat = tmpfile();
	    at->feat_name = galloc((fcnt+scnt+1)*sizeof(struct feat_name));
	    putlong(at->feat,0x00010000);
	    putshort(at->feat,fcnt);
	    putshort(at->feat,0);
	    putlong(at->feat,0);
	    offset = 12 /* header */ + fcnt*12;
		/* FeatureName entry for All Typographics */
	    putshort(at->feat,0);
	    putshort(at->feat,1);
	    putlong(at->feat,offset);
	    putshort(at->feat,0x0000);	/* non exclusive */
	    putshort(at->feat,strid);
	    at->feat_name[fn].name = "All Typographic Features";
	    at->feat_name[fn++].strid = strid++;
	    offset += 2*4;		/* All Features/No Features */
	} else if ( k==1 ) {
		/* Setting Name Array for All Typographic Features */
	    putshort(at->feat,0);
	    putshort(at->feat,strid);
	    at->feat_name[fn].name = "All Features";
	    at->feat_name[fn++].strid = strid++;
	    putshort(at->feat,1);
	    putshort(at->feat,strid);
	    at->feat_name[fn].name = "No Features";
	    at->feat_name[fn++].strid = strid++;
	}
    }
    memset( &at->feat_name[fn],0,sizeof(struct feat_name));
    at->featlen = ftell(at->feat);
    if ( at->featlen&2 )
	putshort(at->feat,0);
}

static int featuresAssignFlagsChains(struct feature *feature) {
    int bit=0, cnt, chain = 0;
    struct feature *f, *n;

    if ( feature==NULL )
return( 0 );

    for ( f=feature; f!=NULL; f=n ) {
	cnt=0;
	for ( n=f; n!=NULL && n->featureType==f->featureType; n=n->next ) ++cnt;
	if ( bit+cnt>=32 ) {
	    ++chain;
	    bit = 0;
	}
	for ( n=f; n!=NULL && n->featureType==f->featureType; n=n->next ) {
	    n->flag = 1<<(bit++);
	    n->chain = chain;
	}
    }
return( chain+1 );
}

static void morxDumpChain(struct alltabs *at,struct feature *features,int chain,
	FILE *temp,struct table_ordering *ord) {
    uint32 def_flags=0;
    struct feature *f, *n;
    int subcnt=0, scnt=0;
    uint32 chain_start, end;
    char *buf;
    int len, tot, cnt;
    struct feature *all[32], *ftemp;
    int i,j;

    for ( f=features, cnt=0; f!=NULL; f=n ) {
	if ( f->chain==chain ) {
	    for ( n=f; n!=NULL && n->featureType==f->featureType; n=n->next ) {
		all[cnt++] = n;
		++subcnt;
		if ( n->defaultOn )
		    def_flags |= n->flag;
		++scnt;
		if ( !n->ismutex && n->offname!=NULL ) ++scnt;
	    }
	} else
	    n = f->next;
    }

    if ( cnt>32 ) GDrawIError("Too many features in chain");
    for ( i=0; i<cnt; ++i ) for ( j=i+1; j<cnt; ++j ) {
	if ( TTFFeatureIndex(all[i]->otftag,ord)>TTFFeatureIndex(all[j]->otftag,ord)) {
	    ftemp = all[i];
	    all[i] = all[j];
	    all[j] = ftemp;
	}
    }
	

    /* Chain header */
    chain_start = ftell(at->morx);
    putlong(at->morx,def_flags);
    putlong(at->morx,0);		/* Fix up length later */
    putlong(at->morx,scnt+1);		/* extra for All Typo features */
    putlong(at->morx,subcnt);		/* subtable cnt */

    /* Features */
    for ( f=features; f!=NULL; f=n ) {
	if ( f->chain==chain ) {
	    for ( n=f; n!=NULL && n->featureType==f->featureType; n=n->next ) {
		putshort(at->morx,n->featureType);
		putshort(at->morx,n->featureSetting);
		putlong(at->morx,n->flag);
		putlong(at->morx,0xffffffff);
		if ( !n->ismutex && n->offname!=NULL ) {
		    putshort(at->morx,n->featureType);
		    putshort(at->morx,n->offSetting);
		    putlong(at->morx,0);
		    putlong(at->morx,~n->flag);
		}
	    }
	} else
	    n = f->next;
    }
    putshort(at->morx,0);		/* All Typo Features */
    putshort(at->morx,1);		/* No Features */
    putlong(at->morx,0);		/* enable */
    putlong(at->morx,0);		/* disable */

    buf = galloc(16*1024);
    /* Subtables */
    /* Ordered by tag */
    for ( i=0; i<cnt; ++i ) if ( (f=all[i])->chain==chain ) {
	putlong(at->morx,f->feature_len+12);		/* Size of header needs to be added */
	putlong(at->morx,(f->vertOnly?0x80000000:f->r2l?0x40000000:0) | f->subtable_type);
	putlong(at->morx,f->flag);
	tot = f->feature_len;
	fseek(temp, f->feature_start, SEEK_SET);
	while ( tot!=0 ) {
	    len = tot;
	    if ( len>16*1024 ) len = 16*1024;
	    len = fread(buf,1,len,temp);
	    len = fwrite(buf,1,len,at->morx);
	    if ( len<=0 ) {
		fprintf( stderr, "Disk error\n" );
	break;
	    }
	    tot -= len;
	}
    }
    free(buf);

    /* Pad chain to a multiple of four */
    if ( (ftell(at->morx)-chain_start)&1 )
	putc('\0',at->morx);
    if ( (ftell(at->morx)-chain_start)&2 )
	putshort(at->morx,0);
    end = ftell(at->morx);
    fseek(at->morx,chain_start+4,SEEK_SET);
    putlong(at->morx,end-chain_start);
    fseek(at->morx,0,SEEK_END);
}

void aat_dumpmorx(struct alltabs *at, SplineFont *sf) {
    FILE *temp = tmpfile();
    struct feature *features = NULL;
    int nchains, i;
    struct table_ordering *ord;

    features = aat_dumpmorx_substitutions(at,sf,temp,features);
    features = aat_dumpmorx_glyphforms(at,sf,temp,features);
    features = aat_dumpmorx_ligatures(at,sf,temp,features);
    if ( features==NULL ) {
	fclose(temp);
return;
    }
    features = featuresOrderByType(features);
    aat_dumpfeat(at, features);
    nchains = featuresAssignFlagsChains(features);

    at->morx = tmpfile();
    putlong(at->morx,0x00020000);
    putlong(at->morx,nchains);
    for ( ord = sf->orders; ord!=NULL && ord->table_tag!=CHR('G','S','U','B'); ord = ord->next );
    for ( i=0; i<nchains; ++i )
	morxDumpChain(at,features,i,temp,ord);
    fclose(temp);
    morxfeaturesfree(features);
    
    at->morxlen = ftell(at->morx);
    if ( at->morxlen&1 )
	putc('\0',at->morx);
    if ( (at->morxlen+1)&2 )
	putshort(at->morx,0);
}

/* ************************************************************************** */
/* *************************    The 'opbd' table    ************************* */
/* ************************************************************************** */


int haslrbounds(SplineChar *sc, PST **left, PST **right) {
    PST *pst;

    *left = *right = NULL;
    for ( pst=sc->possub; pst!=NULL ; pst=pst->next ) {
	if ( pst->type == pst_position ) {
	    if ( pst->tag==CHR('l','f','b','d') ) {
		*left = pst;
		if ( *right )
return( true );
	    } else if ( pst->tag==CHR('r','t','b','d') ) {
		*right = pst;
		if ( *left )
return( true );
	    }
	}
    }
return( *left!=NULL || *right!=NULL );
}

void aat_dumpopbd(struct alltabs *at, SplineFont *_sf) {
    int i, j, k, l, cmax, seg_cnt, tot, last, offset;
    PST *left, *right;
    FILE *opbd=NULL;
    /* We do four passes. The first just calculates how much space we will need (if any) */
    /*  the second provides the top-level lookup table structure */
    /*  the third provides the arrays of offsets needed for type 4 lookup tables */
    /*  the fourth provides the actual data on the optical bounds */
    SplineFont *sf;
    SplineChar *sc;

    cmax = 0;
    l = 0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[l];
	if ( cmax<sf->charcnt ) cmax = sf->charcnt;
	++l;
    } while ( l<_sf->subfontcnt );

    for ( k=0; k<4; ++k ) {
	for ( i=seg_cnt=tot=0; i<cmax; ++i ) {
	    l = 0;
	    sc = NULL;
	    do {
		sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[l];
		if ( l<sf->charcnt && sf->chars[l]!=NULL ) {
		    sc = sf->chars[i];
	    break;
		}
		++l;
	    } while ( l<_sf->subfontcnt );
	    if ( sc!=NULL && sc->ttf_glyph!=-1 && haslrbounds(sc,&left,&right) ) {
		if ( k==1 )
		    tot = 0;
		else if ( k==2 ) {
		    putshort(opbd,offset);
		    offset += 8;
		} else if ( k==3 ) {
		    putshort(opbd,left!=NULL?left->u.pos.xoff:0);
		    putshort(opbd,0);		/* top */
		    putshort(opbd,right!=NULL?-right->u.pos.h_adv_off:0);
		    putshort(opbd,0);		/* bottom */
		}
		for ( j=i+1, ++tot; j<sf->charcnt; ++j ) if ( sf->chars[j]!=NULL && sf->chars[j]->ttf_glyph!=-1 ) {
		    if ( !haslrbounds(sf->chars[j],&left,&right) )
		break;
		    ++tot;
		    last = j;
		    if ( k==2 ) {
			putshort(opbd,offset);
			offset += 8;
		    } else if ( k==3 ) {
			putshort(opbd,left!=NULL?left->u.pos.xoff:0);
			putshort(opbd,0);		/* top */
			putshort(opbd,right!=NULL?-right->u.pos.h_adv_off:0);
			putshort(opbd,0);		/* bottom */
		    }
		}
		if ( k==1 ) {
		    putshort(opbd,sf->chars[last]->ttf_glyph);
		    putshort(opbd,sf->chars[i]->ttf_glyph);
		    putshort(opbd,offset);
		    offset += 2*tot;
		}
		++seg_cnt;
		i = j-1;
	    }
	}
	if ( k==0 ) {
	    if ( seg_cnt==0 )
return;
	    opbd = tmpfile();
	    putlong(opbd, 0x00010000);	/* version */
	    putshort(opbd,0);		/* data are distances (not points) */

	    putshort(opbd,4);		/* Lookup table format 4 */
		/* Binary search header */
	    putshort(opbd,6);		/* Entry size */
	    putshort(opbd,seg_cnt);	/* Number of segments */
	    for ( j=0,l=1; l<=seg_cnt; l<<=1, ++j );
	    --j; l>>=1;
	    putshort(opbd,6*l);
	    putshort(opbd,j);
	    putshort(opbd,6*(seg_cnt-l));
	    offset = 6*2 + seg_cnt*6 + 6;
	} else if ( k==1 ) {		/* flag entry */
	    putshort(opbd,0xffff);
	    putshort(opbd,0xffff);
	    putshort(opbd,0);
	}
    }
    at->opbd = opbd;
    at->opbdlen = ftell(at->opbd);
    if ( at->opbdlen&2 )
	putshort(at->opbd,0);
}

/* ************************************************************************** */
/* *************************    The 'prop' table    ************************* */
/* ************************************************************************** */

uint16 *props_array(SplineFont *_sf,int numGlyphs) {
    uint16 *props;
    int i;
    SplineChar *sc, *bsc;
    int dir, isfloat, isbracket, offset, doit=false;
    AnchorPoint *ap;
    PST *pst;
    SplineFont *sf;
    int l,cmax;

    props = gcalloc(numGlyphs+1,sizeof(uint16));
    props[numGlyphs] = -1;

    cmax = 0;
    l = 0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[l];
	if ( cmax<sf->charcnt ) cmax = sf->charcnt;
	++l;
    } while ( l<_sf->subfontcnt );

    for ( i=0; i<cmax; ++i ) {
	l = 0;
	sc = NULL;
	do {
	    sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[l];
	    if ( i<sf->charcnt && sf->chars[i]!=NULL ) {
		sc = sf->chars[i];
	break;
	    }
	    ++l;
	} while ( l<_sf->subfontcnt );
	if ( sc!=NULL && sc->ttf_glyph!=-1 ) {
	    dir = 0;
	    if ( sc->unicodeenc>=0x10300 && sc->unicodeenc<=0x103ff )
		dir = 0;
	    else if ( sc->unicodeenc>=0x10800 && sc->unicodeenc<=0x103ff )
		dir = 1;
	    else if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10fff ) {
		if ( iseuronumeric(sc->unicodeenc) )
		    dir = 3;
		else if ( iseuronumsep(sc->unicodeenc))
		    dir = 4;
		else if ( iseuronumterm(sc->unicodeenc))
		    dir = 5;
		else if ( isarabnumeric(sc->unicodeenc))
		    dir = 6;
		else if ( iscommonsep(sc->unicodeenc))
		    dir = 7;
		else if ( isspace(sc->unicodeenc))
		    dir = 10;
		else if ( islefttoright(sc->unicodeenc) )
		    dir = 0;
		else if ( isrighttoleft(sc->unicodeenc) )
		    dir = 1;
		else if ( SCScriptFromUnicode(sc)==CHR('a','r','a','b') )
		    dir = 2;
		else if ( SCScriptFromUnicode(sc)==CHR('h','e','b','r') )
		    dir = 1;
		else
		    dir = 11;		/* Other neutrals */
		/* Not dealing with unicode 3 classes */
		/* nor block seperator/ segment seperator */
	    } else if ( SCScriptFromUnicode(sc)==CHR('a','r','a','b') )
		dir = 2;
	    else if ( SCScriptFromUnicode(sc)==CHR('h','e','b','r') )
		dir = 1;

	    if ( dir==1 || dir==2 ) doit = true;
	    isfloat = false;
	    if ( sc->width==0 &&
		    ((sc->anchor!=NULL && sc->anchor->type==at_mark) ||
		     (sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 && iscombining(sc->unicodeenc))))
		isfloat = doit = true;
	    isbracket = offset = 0;
	    if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 && tomirror(sc->unicodeenc)!=0 ) {
		bsc = SFGetCharDup(sf,tomirror(sc->unicodeenc),NULL);
		if ( bsc!=NULL && bsc->ttf_glyph-sc->ttf_glyph>-8 && bsc->ttf_glyph-sc->ttf_glyph<8 ) {
		    isbracket = true;
		    offset = bsc->ttf_glyph-sc->ttf_glyph;
		}
	    }
	    if ( !isbracket ) {
		for ( pst=sc->possub; pst!=NULL && pst->tag!=CHR('r','t','l','a'); pst=pst->next );
		if ( pst!=NULL && pst->type==pst_substitution &&
			(bsc=SFGetCharDup(sf,-1,pst->u.subs.variant))!=NULL &&
			bsc->ttf_glyph!=-1 && bsc->ttf_glyph-sc->ttf_glyph>-8 && bsc->ttf_glyph-sc->ttf_glyph<8 ) {
		    isbracket = true;
		    offset = bsc->ttf_glyph-sc->ttf_glyph;
		    doit = true;
		}
	    }
	    if ( SCRightToLeft(sc) ) {
		/* Apple docs say attached right. So for r2l scripts we look for */
		/*  a cursive entry, and for l2r a cursive exit */
		for ( ap=sc->anchor; ap!=NULL && ap->type!=at_centry; ap=ap->next );
	    } else {
		for ( ap=sc->anchor; ap!=NULL && ap->type!=at_cexit; ap=ap->next );
	    }
	    props[sc->ttf_glyph] = dir |
		    (isfloat ? 0x8000 : 0 ) |
		    (isbracket ? 0x1000 : 0 ) |
		    (ap!=NULL ? 0x80 : 0 ) |
		    ((offset&0xf)<<8);
	    /* not dealing with */
	    /*	hang left 0x4000 */
	    /*	hang right 0x2000 */
	}
    }

    if ( !doit ) {
	free(props);
return( NULL );
    }

return( props );
}

void aat_dumpprop(struct alltabs *at, SplineFont *sf) {
    uint16 *props = props_array(sf,at->maxp.numGlyphs);
    uint32 bin_srch_header;
    int i, j, cnt;

    if ( props==NULL )
return;

    at->prop = tmpfile();
    putlong(at->prop,0x00020000);
    putshort(at->prop,1);		/* Lookup data */
    putshort(at->prop,0);		/* default property is simple l2r */
    putshort(at->prop,2);		/* lookup format 2 => segment single value */
	/* Binsearch header */
    bin_srch_header = ftell(at->prop);
    putshort(at->prop,6);		/* Entry size */
    putshort(at->prop,0);		/* fill in later */
    putshort(at->prop,0);
    putshort(at->prop,0);
    putshort(at->prop,0);

    cnt = 0;
    for ( i=0; i<at->maxp.numGlyphs; ++i ) {
	while ( i<at->maxp.numGlyphs && props[i]==0 ) ++i;	/* skip default entries */
	if ( i>=at->maxp.numGlyphs )
    break;
	for ( j=i+1; j<at->maxp.numGlyphs && props[j]==props[i]; ++j );
	putshort(at->prop,j-1);
	putshort(at->prop,i);
	putshort(at->prop,props[i]);
	i = j-1;
	++cnt;
    }
    putshort(at->prop,0xffff);		/* Final eof marker */
    putshort(at->prop,0xffff);
    putshort(at->prop,0x0000);

    fseek(at->prop,bin_srch_header,SEEK_SET);
    putshort(at->prop,6);		/* Entry size */
    putshort(at->prop,cnt);		/* Number of segments */
    for ( j=0,i=1; i<=cnt; i<<=1, ++j );
    --j; i>>=1;
    putshort(at->prop,6*i);
    putshort(at->prop,j);
    putshort(at->prop,6*(cnt-i));

    fseek(at->prop,0,SEEK_END);
    at->proplen = ftell(at->prop);
    if ( at->proplen&2 )
	putshort(at->prop,0);
    free(props);
}

/* ************************************************************************** */
/* *************************    utility routines    ************************* */
/* ************************************************************************** */

uint32 MacFeatureToOTTag(int featureType,int featureSetting) {
    int i;
    struct macsettingname *msn = user_macfeat_otftag ? user_macfeat_otftag : macfeat_otftag;

    for ( i=0; msn[i].otf_tag!=0; ++i )
	if ( msn[i].mac_feature_type == featureType &&
		msn[i].mac_feature_setting == featureSetting )
return( msn[i].otf_tag );

return( 0 );
}

int OTTagToMacFeature(uint32 tag, int *featureType,int *featureSetting) {
    int i;
    struct macsettingname *msn = user_macfeat_otftag ? user_macfeat_otftag : macfeat_otftag;

    for ( i=0; msn[i].otf_tag!=0; ++i )
	if ( msn[i].otf_tag == tag ) {
	    *featureType = msn[i].mac_feature_type;
	    *featureSetting = msn[i].mac_feature_setting;
return( true );
	}

    *featureType = 0;
    *featureSetting = 0;
return( 0 );
}

static struct feature *featureFromTag(uint32 tag ) {
    int i;
    struct feature *feat;
    struct macsettingname *msn = user_macfeat_otftag ? user_macfeat_otftag : macfeat_otftag;

    for ( i=0; msn[i].otf_tag!=0; ++i )
	if ( msn[i].otf_tag == tag ) {
	    feat = chunkalloc(sizeof(struct feature));
	    feat->otftag = tag;
	    feat->featureType = msn[i].mac_feature_type;
	    feat->featureSetting = msn[i].mac_feature_setting;
	    feat->offSetting = msn[i].off_setting;
	    feat->name = msn[i].on_name;
	    feat->offname = msn[i].off_name;
	    feat->ismutex = msn[i].ismutex;
	    feat->defaultOn = msn[i].defaultOn;
	    feat->vertOnly = tag==CHR('v','r','t','2') || tag==CHR('v','k','n','a');
return( feat );
	}

return( NULL );
}

