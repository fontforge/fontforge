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
/*  (or GX fonts)  */

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

int SLIHasDefault(SplineFont *sf,int sli) {
    struct script_record *sr = sf->script_lang[sli];
    int i, j;

    if ( sli==SLI_NESTED || sli==SLI_UNKNOWN )
return( false );

    for ( i=0; sr[i].script!=0; ++i )
	for ( j=0; sr[i].langs[j]!=0; ++j )
	    if ( sr[i].langs[j]==DEFAULT_LANG )
return( true );

return( false );
}

void ttf_dumpkerns(struct alltabs *at, SplineFont *sf) {
    int i, cnt, vcnt, j, k, m, kccnt=0, vkccnt=0, c, mh, mv;
    KernPair *kp;
    KernClass *kc;
    uint16 *glnum, *offsets;
    int version;
    int isv;

#if 0
    /* I'm told that at most 2048 kern pairs are allowed in a ttf font */
    /*  ... but can't find any data to back up that claim */
    threshold = KernThreshold(sf,2048);
#endif

    cnt = mh = vcnt = mv = 0;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	j = 0;
	for ( kp = sf->chars[i]->kerns; kp!=NULL; kp=kp->next )
	    if ( kp->off!=0 ) 
		++cnt, ++j;
	if ( j>mh ) mh=j;
	j=0;
	for ( kp = sf->chars[i]->vkerns; kp!=NULL; kp=kp->next )
	    if ( kp->off!=0 ) 
		++cnt, ++j;
	if ( j>mv ) mv=j;
    }
    kccnt = 0;
    for ( kc=sf->kerns; kc!=NULL; kc = kc->next ) if ( SLIHasDefault(sf,kc->sli) )
	++kccnt;
    for ( kc=sf->vkerns; kc!=NULL; kc = kc->next ) if ( SLIHasDefault(sf,kc->sli) )
	++vkccnt;
    if ( cnt==0 && kccnt==0 && vcnt==0 && vkccnt==0 )
return;

    /* Old kerning format (version 0) uses 16 bit quantities */
    /* Apple's new format (version 0x00010000) uses 32 bit quantities */
    at->kern = tmpfile();
    if ( kccnt==0 && vkccnt==0 && vcnt==0 ) {
	/* MS does not support format 2 kern sub-tables so if we have them */
	/*	(also MS docs are different from Apple's docs on this subtable) */
	/*  we might as well admit that this table is for apple only and use */
	/*  the new format apple recommends. Otherwise, use the old format */
	putshort(at->kern,0);		/* version */
	putshort(at->kern,1);		/* number of subtables */
	version = 0;
    } else {
	putlong(at->kern,0x00010000);
	putlong(at->kern,kccnt+vkccnt+(cnt!=0)+(vcnt!=0));
	version = 1;
    }
    for ( isv=0; isv<2; ++isv ) {
	c = isv ? vcnt : cnt;
	m = isv ? mv : mh;
	if ( c!=0 ) {
	    if ( version==0 ) {
		putshort(at->kern,0);		/* subtable version */
		putshort(at->kern,(7+3*c)*sizeof(uint16)); /* subtable length */
		putshort(at->kern,!isv);	/* coverage, flags=hor/vert&format=0 */
	    } else {
		putlong(at->kern,(8+3*c)*sizeof(uint16)); /* subtable length */
		/* Apple's new format has a completely different coverage format */
		putshort(at->kern,isv?0x8000:0);/* format 0, horizontal/vertical flags (coverage) */
		putshort(at->kern,0);		/* tuple index, whatever that means */
	    }
	    putshort(at->kern,c);
	    for ( i=1,j=0; i<=c; i<<=1, ++j );
	    i>>=1; --j;
	    putshort(at->kern,i*6);		/* binary search headers */
	    putshort(at->kern,j);
	    putshort(at->kern,6*(i-c));

	    glnum = galloc(m*sizeof(uint16));
	    offsets = galloc(m*sizeof(uint16));
	    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
		m = 0;
		for ( kp = isv ? sf->chars[i]->vkerns : sf->chars[i]->kerns; kp!=NULL; kp=kp->next ) {
		    if ( kp->off!=0 ) {
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
    }

    for ( isv=0; isv<2; ++isv ) {
	for ( kc=isv ? sf->vkerns : sf->kerns; kc!=NULL; kc=kc->next ) if ( SLIHasDefault(sf,kc->sli) ) {
	    /* If we are here, we must be using version 1 */
	    uint32 len_pos = ftell(at->kern), pos;
	    uint16 *class1, *class2;

	    putlong(at->kern,0); /* subtable length */
	    putshort(at->kern,isv?0x8002:2);	/* format 2, horizontal/vertical flags (coverage) */
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
	    putshort(temp,4);		/* Lookup table format 4 */
		/* Binary search header */
	    putshort(temp,6);		/* Entry size */
	    putshort(temp,seg_cnt);	/* Number of segments */
	    for ( j=0,l=1; l<=seg_cnt; l<<=1, ++j );
	    --j; l>>=1;
	    putshort(temp,6*l);
	    putshort(temp,j);
	    putshort(temp,6*(seg_cnt-l));
	    if ( seg_cnt==0 )
return;
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

    if ( lig->script_lang_index==SLI_NESTED )
return( false );

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
	    if ( pst->tag!=CHR('i','s','o','l') &&
		    pst->type!=pst_position &&
		    pst->type!=pst_pair &&
		    pst->type!=pst_lcaret &&
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
			     pst->type==pst_position || pst->type==pst_pair ||
			     pst->type==pst_lcaret); pst=pst->next );
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
		    if ( gcnt==0 )
	    break;
		    glyphs = galloc((gcnt+1)*sizeof(SplineChar *));
		    maps = galloc((gcnt+1)*sizeof(uint16));
		} else {
		    glyphs[gcnt] = NULL; maps[gcnt] = 0;
		}
	    }
	    if ( gcnt==0 )
	continue;
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

static int IsMarkChar( SplineChar *sc ) {
    AnchorPoint *ap;

    ap=sc->anchor;
    while ( ap!=NULL && (ap->type==at_centry || ap->type==at_cexit) )
	ap = ap->next;
    if ( ap!=NULL && (ap->type==at_mark || ap->type==at_basemark) )
return( true );

return( false );
}

struct transition { uint16 next_state, dontconsume, ismark, trans_ent; LigList *l; };
struct trans_entries { uint16 next_state, flags, act_index; LigList *l; };
static void morx_dumpLigaFeature(FILE *temp,SplineChar **glyphs,int gcnt,
	uint32 otf_tag, struct alltabs *at, SplineFont *sf,
	int ignoremarks) {
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
    int acnt, lcnt, charcnt;
    uint32 *actions;
    uint16 *components, *lig_glyphs;
    uint32 here;
    struct splinecharlist *scl;
    int anymarks;

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
    anymarks = false;
    charcnt = class;
    if ( ignoremarks ) {
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=-1 ) {
	    if ( IsMarkChar(sf->chars[i])) {
		anymarks = true;
		++charcnt;
		used[sf->chars[i]->ttf_glyph] = class;
	    }
	}
	if ( anymarks )
	    ++class;
    }
    cglyphs = galloc((charcnt+1)*sizeof(SplineChar *));
    map = galloc((charcnt+1)*sizeof(uint16));
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
    if ( anymarks ) {
	/* behavior for a mark is the same everywhere: stay in current state */
	/*  do no operations. consume the mark */
	for ( i=0; i<state_cnt; ++i ) {
	    states[i][class-1].next_state = i;
	    states[i][class-1].ismark = true;
	}
    }
    /* Ok, we've got the state machine now. Convert it into apple's wierd */
    /*  (space saving) format */
    trans = galloc(class*state_cnt*sizeof(struct trans_entries));
    trans_cnt = 0;
    for ( i=0; i<state_cnt; ++i ) for ( j=0; j<class; ++j ) {
	if ( states[i][j].ismark )
	    k = trans_cnt;
	else for ( k=0; k<trans_cnt; ++k ) {
	    if ( trans[k].next_state==states[i][j].next_state &&
		    (trans[k].flags&0x4000?1:0) == states[i][j].dontconsume &&
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
	    else if ( states[i][j].ismark )
		/* Do nothing */;
	    else if ( trans[k].next_state!=0 || trans[k].l!=NULL )
		trans[k].flags = 0x8000;
	    if ( trans[k].l!=NULL )
		trans[k].flags |= 0x2000;
	    trans[k].act_index = 0;
	    ++trans_cnt;
	}
    }
    /* Oops. Bug. */
    /* Suppose we have two ligatures f+l=>fl & s+t->st. */
    /* Suppose we get input "fst"			*/
    /* Now the state machine we've built so far will go to the f branch, see */
    /*  the "s" and go back to state 0 */
    /* Obviously that's wrong, we've lost the st. So either we go back to 0 */
    /*  but don't advance the glyph, or we take the transition from state 0 */
    /*  and copy it to here. The second is easier for me just now */
    for ( i=2; i<state_cnt; ++i ) for ( j=4; j<class; ++j ) {
	if ( states[i][j].trans_ent == 0 && states[0][j].trans_ent != 0 )
	    states[i][j].trans_ent = states[0][j].trans_ent;
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
			(l = LigListMatchOtfTag(sf,sc->ligofme,ligtags[j]))!=NULL ) {
	    uint32 script = SCScriptFromUnicode(sc);
	    int ignoremarks = l->lig->flags & pst_ignorecombiningmarks ? 1 : 0 ;
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
	    morx_dumpLigaFeature(temp,glyphs,gcnt,ligtags[j],at,sf,ignoremarks);
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

static SplineChar *SCFromTTFGlyph(SplineFont *sf,int ttf_glyph ) {
    int i;

    if ( ttf_glyph==-1 )
return( NULL );
    if ( (i=ttf_glyph-2)<0 ) i=0;
    for ( ; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	if ( sf->chars[i]->ttf_glyph==ttf_glyph )
return( sf->chars[i] );
	if ( sf->chars[i]->ttf_glyph>0 && sf->chars[i]->ttf_glyph>ttf_glyph )
return( NULL );
    }
return( NULL );
}

static void orderglyphs(SplineChar **cglyphs,uint16 *map,int max) {
    int i,j;
    uint16 temp;
    SplineChar *tempsc;

    for ( i=0; i<max; ++i ) for ( j=i+1; j<max; ++j ) {
	if ( cglyphs[i]->ttf_glyph > cglyphs[j]->ttf_glyph ) {
	    tempsc = cglyphs[i]; temp = map[i];
	    cglyphs[i] = cglyphs[j]; map[i] = map[j];
	    cglyphs[j] = tempsc; map[j] = temp;
	}
    }
}

static void morx_dumpContGlyphFormFeature(FILE *temp,SplineChar **glyphs,
	uint16 *forms[4], int gcnt,
	struct alltabs *at, SplineFont *sf, uint32 script, int ignoremarks ) {
    uint16 *used = gcalloc(at->maxp.numGlyphs,sizeof(uint16));
    SplineChar **cglyphs, *sc;
    uint16 *map;
    int i,j,k,f, anymarks;
    uint32 start, pos, here;
    int any[4];
    PST *pst;

    any[0] = any[1]= any[2]= any[3] = 0;
    /* figure out the classes, only one of any importance: Letter in this script */
    /* might already be formed. Might be a lig. Might be normal */
    /* Oh, if ignoremarks is true, then combining marks merit a class of their own */
    for ( i=0; i<gcnt; ++i ) for ( f=0; f<4; ++f) if ( forms[f][i]!=0 ) {
	used[ forms[f][i] ] = 4;
	any[f] = true;
    }
    anymarks = false;
    for ( i=0; i<sf->charcnt; ++i) if ( (sc=sf->chars[i])!=NULL && sc->ttf_glyph!=-1 ) {
	if ( SCScriptFromUnicode(sc)==script ) {
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
	if ( ignoremarks && IsMarkChar(sc)) {
	    used[sc->ttf_glyph] = 5;
	    anymarks = true;
	}
    }
    for ( i=0; i<gcnt; ++i )
	used[ glyphs[i]->ttf_glyph ] = 4;

    for ( i=k=0; i<at->maxp.numGlyphs; ++i )
	if ( used[i] ) ++k;
    cglyphs = galloc((2*k+1)*sizeof(SplineChar *));
    map = galloc((2*k+1)*sizeof(uint16));
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
    putlong(temp,anymarks?6:5);		/* # classes */
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
    /* State 2 means we have found one interesting letter, transformed current to 'init' and marked it (in case we need to make it isolated) */
    /* State 3 means we have found two interesting letters, transformed current to 'medi' and marked (in case we need to make it final) */
    putshort(temp,0); putshort(temp,0); putshort(temp,0); putshort(temp,0);
	putshort(temp,1); if ( anymarks ) putshort(temp,0);		/* 0 State */
    putshort(temp,0); putshort(temp,0); putshort(temp,0); putshort(temp,0);
	putshort(temp,1); if ( anymarks ) putshort(temp,0);		/* 1 State */
    putshort(temp,3); putshort(temp,3); putshort(temp,4); putshort(temp,3);
	putshort(temp,2); if ( anymarks ) putshort(temp,4);		/* 2 State */
    putshort(temp,6); putshort(temp,6); putshort(temp,7); putshort(temp,6);
	putshort(temp,5); if ( anymarks ) putshort(temp,7);		/* 3 State */

    /* Transition 0 goes to state 0, no substitutions, no marks */
    /* Transition 1 goes to state 2, current init subs, marks glyph */
    /* Transition 2 goes to state 3, current glyph=>medial, marks glyph */
    /* Transition 3 goes to state 0, marked glyph=>isolated, no marks */
    /* Transition 4 goes to state 2, no subs, no marks (for deleted/mark glyphs) */
    /* Transition 5 goes to state 3, current glyph=>medial, marks glyph */
    /* Transition 6 goes to state 0, marked glyph=>final, no marks */
    /* Transition 7 goes to state 3, no subs, no marks (for deleted) */
    putshort(temp,0); putshort(temp,0x0000); putshort(temp,0xffff); putshort(temp,0xffff);
    putshort(temp,2); putshort(temp,0x8000); putshort(temp,0xffff); putshort(temp,any[0]?0:0xffff);
    putshort(temp,3); putshort(temp,0x8000); putshort(temp,0xffff); putshort(temp,any[1]?any[0]:0xffff);
    putshort(temp,0); putshort(temp,0x0000); putshort(temp,any[3]?any[0]+any[1]+any[2]:0xffff); putshort(temp,0xffff);
    putshort(temp,2); putshort(temp,0x0000); putshort(temp,0xffff); putshort(temp,0xffff);
    putshort(temp,3); putshort(temp,0x8000); putshort(temp,0xffff); putshort(temp,any[1]?any[0]:0xffff);
    putshort(temp,0); putshort(temp,0x0000); putshort(temp,any[2]?any[0]+any[1]:0xffff); putshort(temp,0xffff);
    putshort(temp,3); putshort(temp,0x0000); putshort(temp,0xffff); putshort(temp,0xffff);

    /* Substitution 0 is initial */
    /* Substitution 1 is medial */
    /* Substitution 2 is final */	/* final must be prepared to convert both base chars and medial chars to isolated */
    /* Substitution 3 is isolated */	/* isolated must be prepared to convert both base chars and initial chars to isolated */
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
	if ( (f==2 && any[1]) || (f==3 && any[0])) {
	    int otherf = f==2 ? 1 : 0;
	    for ( i=0; i<gcnt; ++i ) {
		if ( forms[f][i]!=0 && forms[otherf][i]!=0 &&
			forms[otherf][i]!=glyphs[i]->ttf_glyph ) {
		    cglyphs[k] = SCFromTTFGlyph(sf,forms[otherf][i]);
		    map[k++] = forms[f][i];
		}
	    }
	    orderglyphs(cglyphs,map,k);
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
    int ignoremarks = 0;

    glyphs = galloc((at->maxp.numGlyphs+1)*sizeof(SplineChar *));
    for ( i=0; i<4; ++i )
	forms[i] = galloc((at->maxp.numGlyphs+1)*sizeof(uint16));

    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	sf->chars[i]->ticked = false;
    for ( i=0; i<sf->charcnt; ++i )
	    if ( (sc=sf->chars[i])!=NULL && !sc->ticked &&
		    HasAForm(sf,sc->possub)) {
	uint32 script = SCScriptFromUnicode(sc);
	ignoremarks = 0;
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
		if ( pst->flags & pst_ignorecombiningmarks )
		    ignoremarks = true;
		rsc = SFGetCharDup(sf,-1,pst->u.subs.variant);
		if ( rsc==NULL || rsc->ttf_glyph==-1 )
	    continue;
		forms[which][gcnt] = rsc->ttf_glyph;
		any = true;
	    }
	    if ( any ) {
		/* Currently I transform all isolated chars to initial first */
		/*  so if the isolated char, is just the char unchanged, I have to change it back */
		if ( forms[0][gcnt]!=0 && forms[3][gcnt]==0 ) forms[3][gcnt] = ssc->ttf_glyph;
		if ( forms[1][gcnt]!=0 && forms[2][gcnt]==0 ) forms[2][gcnt] = ssc->ttf_glyph;
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
	    morx_dumpContGlyphFormFeature(temp,glyphs,forms,gcnt,at,sf,
		    SCScriptFromUnicode(sc),ignoremarks);
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

static int ValidSubs(SplineFont *sf,uint32 tag ) {
    int i, any=false;
    PST *pst;

    for ( i = 0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	for ( pst=sf->chars[i]->possub; pst!=NULL && pst->tag!=tag; pst=pst->next );
	if ( pst!=NULL && pst->script_lang_index==SLI_NESTED ) {
	    if ( pst->type==pst_substitution )
		any = true;
	    else
return( false );
	}
    }
return( any );
}

static void TreeFree(struct contexttree *tree) {
    int i;
    for ( i=0; i<tree->branch_cnt; ++i )
	TreeFree(tree->branches[i].branch);

    free( tree->branches );
    free( tree->rules );
    chunkfree( tree,sizeof(*tree) );
}

static int TreeLabelState(struct contexttree *tree, int snum) {
    int i;

    if ( tree->branch_cnt==0 && tree->ends_here!=NULL ) {
	tree->state = 0;
return( snum );
    }

    tree->state = snum++;
    for ( i=0; i<tree->branch_cnt; ++i )
	snum = TreeLabelState(tree->branches[i].branch,snum);
    tree->next_state = snum;

return( snum );
}

static uint32 RuleHasSubsHere(struct fpst_rule *rule,int depth) {
    int i,j;

    if ( depth<rule->u.class.bcnt )
return( 0 );
    depth -= rule->u.class.bcnt;
    if ( depth>=rule->u.class.ncnt )
return( 0 );
    for ( i=0; i<rule->lookup_cnt; ++i ) {
	if ( rule->lookups[i].seq==depth ) {
	    /* It is possible to have two substitutions applied at the same */
	    /*  location. I can't deal with that here */
	    for ( j=i+1; j<rule->lookup_cnt; ++j ) {
		if ( rule->lookups[j].seq==depth )
return( 0xffffffff );
	    }
return( rule->lookups[i].lookup_tag );
	}
    }

return( 0 );
}

static uint32 RulesAllSameSubsAt(struct contexttree *me,int pos) {
    int i;
    uint32 tag=0x01, newtag;	/* Can't use 0 as an "unused" flag because it is perfectly valid for there to be no substititution. But then all rules must have no subs */

    for ( i=0; i<me->rule_cnt; ++i ) {
	newtag = RuleHasSubsHere(me->rules[i].rule,pos);
	if ( tag==0x01 )
	    tag=newtag;
	else if ( newtag!=tag )
return( 0xffffffff );
    }
return( tag );
}

static int TreeFollowBranches(SplineFont *sf,struct contexttree *me,int pending_pos) {
    int i, j;

    if ( me->ends_here!=NULL ) {
	/* If any rule ends here then we have to be able to apply all current */
	/*  and pending substitutions */
	if ( pending_pos!=-1 ) {
	    me->applymarkedsubs = RulesAllSameSubsAt(me,pending_pos);
	    if ( me->applymarkedsubs==0xffffffff )
return( false );
	    if ( !ValidSubs(sf,me->applymarkedsubs))
return( false );
	}
	me->applycursubs = RulesAllSameSubsAt(me,me->depth);
	if ( me->applycursubs==0xffffffff )
return( false );
	if ( me->applycursubs!=0 && !ValidSubs(sf,me->applycursubs))
return( false );
	for ( i=0; i<me->branch_cnt; ++i ) {
	    if ( !TreeFollowBranches(sf,me->branches[i].branch,-1))
return( false );
	}
    } else {
	for ( i=0; i<me->branch_cnt; ++i ) {
	    for ( j=0; j<me->rule_cnt; ++j )
		if ( me->rules[j].branch==me->branches[i].branch &&
			RuleHasSubsHere(me->rules[j].rule,me->depth))
	    break;
	    if ( j<me->rule_cnt ) {
		if ( pending_pos==-1 ) {
		    pending_pos = me->depth;
		    me->markme = true;
		} else
return( false );
	    }
	    if ( !TreeFollowBranches(sf,me->branches[i].branch,pending_pos))
return( false );
	}
    }

return( true );
}

static struct contexttree *_FPST2Tree(FPST *fpst,struct contexttree *parent,int class) {
    struct contexttree *me = chunkalloc(sizeof(struct contexttree));
    int i, rcnt, ccnt, k, thisclass;
    uint16 *classes;

    if ( fpst!=NULL ) {
	me->depth = -1;
	me->rule_cnt = fpst->rule_cnt;
	me->rules = gcalloc(me->rule_cnt,sizeof(struct ct_subs));
	for ( i=0; i<me->rule_cnt; ++i )
	    me->rules[i].rule = &fpst->rules[i];
	me->parent = NULL;
    } else {
	me->depth = parent->depth+1;
	for ( i=rcnt=0; i<parent->rule_cnt; ++i )
	    if ( parent->rules[i].rule->u.class.allclasses[me->depth] == class )
		++rcnt;
	me->rule_cnt = rcnt;
	me->rules = gcalloc(me->rule_cnt,sizeof(struct ct_subs));
	for ( i=rcnt=0; i<parent->rule_cnt; ++i )
	    if ( parent->rules[i].rule->u.class.allclasses[me->depth] == class )
		me->rules[rcnt++].rule = parent->rules[i].rule;
	me->parent = parent;
    }
    classes = galloc(me->rule_cnt*sizeof(uint16));
    for ( i=ccnt=0; i<me->rule_cnt; ++i ) {
	thisclass = me->rules[i].thisclassnum = me->rules[i].rule->u.class.allclasses[me->depth+1];
	if ( thisclass==0xffff ) {
	    if ( me->ends_here==NULL )
		me->ends_here = me->rules[i].rule;
	} else {
	    for ( k=0; k<ccnt; ++k )
		if ( classes[k] == thisclass )
	    break;
	    if ( k==ccnt )
		classes[ccnt++] = thisclass;
	}
    }
    me->branch_cnt = ccnt;
    me->branches = gcalloc(ccnt,sizeof(struct ct_branch));
    for ( i=0; i<ccnt; ++i )
	me->branches[i].classnum = classes[i];
    for ( i=0; i<ccnt; ++i ) {
	me->branches[i].branch = _FPST2Tree(NULL,me,classes[i]);
	for ( k=0; k<me->rule_cnt; ++k )
	    if ( classes[i]==me->rules[k].thisclassnum )
		me->rules[k].branch = me->branches[i].branch;
    }
    free(classes );
return( me );
}

static void FPSTBuildAllClasses(FPST *fpst) {
    int i, off,j;

    for ( i=0; i<fpst->rule_cnt; ++i ) {
	fpst->rules[i].u.class.allclasses = galloc((fpst->rules[i].u.class.bcnt+
						    fpst->rules[i].u.class.ncnt+
			                            fpst->rules[i].u.class.fcnt+
			                            1)*sizeof(uint16));
	off = fpst->rules[i].u.class.bcnt;
	for ( j=0; j<off; ++j )
	    fpst->rules[i].u.class.allclasses[j] = fpst->rules[i].u.class.bclasses[off-1-j];
	for ( j=0; j<fpst->rules[i].u.class.ncnt; ++j )
	    fpst->rules[i].u.class.allclasses[off+j] = fpst->rules[i].u.class.nclasses[j];
	off += j;
	for ( j=0; j<fpst->rules[i].u.class.fcnt; ++j )
	    fpst->rules[i].u.class.allclasses[off+j] = fpst->rules[i].u.class.fclasses[j];
	fpst->rules[i].u.class.allclasses[off+j] = 0xffff;	/* End of rule marker */
    }
}

static void FPSTFreeAllClasses(FPST *fpst) {
    int i;

    for ( i=0; i<fpst->rule_cnt; ++i ) {
	free( fpst->rules[i].u.class.allclasses );
	fpst->rules[i].u.class.allclasses = NULL;
    }
}

static struct contexttree *FPST2Tree(SplineFont *sf,FPST *fpst) {
    struct contexttree *ret;

    if ( fpst->format != pst_class )
return( NULL );

    /* I could check for subclasses rather than ClassesMatch, but then I'd have */
    /* to make sure that class 0 was used (if at all) consistently */
    if ( (fpst->bccnt!=0 && !ClassesMatch(fpst->bccnt,fpst->bclass,fpst->nccnt,fpst->nclass)) ||
	    (fpst->fccnt!=0 && !ClassesMatch(fpst->fccnt,fpst->fclass,fpst->nccnt,fpst->nclass)))
return( NULL );

    FPSTBuildAllClasses(fpst);
	
    ret = _FPST2Tree(fpst,NULL,0);

    if ( !TreeFollowBranches(sf,ret,-1) ) {
	TreeFree(ret);
	ret = NULL;
    }

    FPSTFreeAllClasses(fpst);

    TreeLabelState(ret,1);	/* actually, it's states 0&1, but this will do */

return( ret );
}

int FPSTisMacable(SplineFont *sf, FPST *fpst) {
    int i;
    int featureType, featureSetting;
    struct contexttree *ret;

    if ( fpst->type!=pst_contextsub && fpst->type!=pst_chainsub )
return( false );
    if ( !SLIHasDefault(sf,fpst->script_lang_index))
return( false );
    if ( !OTTagToMacFeature(fpst->tag,&featureType,&featureSetting) )
return( false );

    if ( fpst->format == pst_glyphs ) {
	FPST *tempfpst = FPSTGlyphToClass(fpst);
	ret = FPST2Tree(sf, tempfpst);
	FPSTFree(tempfpst);
	TreeFree(ret);
return( ret!=NULL );
    } else if ( fpst->format == pst_class ) {
	ret = FPST2Tree(sf, fpst);
	TreeFree(ret);
return( ret!=NULL );
    } else if ( fpst->format != pst_coverage )
return( false );

    for ( i=0; i<fpst->rule_cnt; ++i ) {
	if ( fpst->rules[i].u.coverage.ncnt+
		fpst->rules[i].u.coverage.bcnt+
		fpst->rules[i].u.coverage.fcnt>=10 )
return( false );			/* Let's not make a state machine this complicated */

	if ( fpst->rules[i].lookup_cnt==2 ) {
	    switch ( fpst->format ) {
	      case pst_coverage:
		/* Second substitution must be on the final glyph */
		if ( fpst->rules[i].u.coverage.fcnt!=0 ||
			fpst->rules[i].lookups[0].seq==fpst->rules[i].lookups[1].seq ||
			(fpst->rules[i].lookups[0].seq!=fpst->rules[i].u.coverage.ncnt-1 &&
			 fpst->rules[i].lookups[1].seq!=fpst->rules[i].u.coverage.ncnt-1) )
return( false );
	      break;
	      default:
return( false );
	    }
	    if ( !ValidSubs(sf,fpst->rules[i].lookups[1].lookup_tag) )
return( false );
		
	} else if ( fpst->rules[i].lookup_cnt!=1 )
return( false );
	if ( !ValidSubs(sf,fpst->rules[i].lookups[0].lookup_tag) )
return( false );
    }

return( fpst->rule_cnt>0 );
}

static void morx_dumpnestedsubs(FILE *temp,SplineFont *sf,uint32 tag) {
    int i, j, gcnt;
    PST *pst;
    SplineChar **glyphs, *sc;
    uint16 *map;

    for ( j=0; j<2; ++j ) {
	gcnt = 0;
	for ( i = 0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	    for ( pst=sf->chars[i]->possub;
		    pst!=NULL && (pst->tag!=tag || pst->script_lang_index!=SLI_NESTED);
		    pst=pst->next );
	    if ( pst!=NULL && pst->type==pst_substitution &&
		    (sc=SFGetCharDup(sf,-1,pst->u.subs.variant))!=NULL &&
		    sc->ttf_glyph!=-1 ) {
		if ( j ) {
		    glyphs[gcnt] = sf->chars[i];
		    map[gcnt] = sc->ttf_glyph;
		}
		++gcnt;
	    }
	}
	if ( !j ) {
	    glyphs = galloc((gcnt+1)*sizeof(SplineChar *));
	    map = galloc(gcnt*sizeof(uint16));
	    glyphs[gcnt] = NULL;
	}
    }
    morx_lookupmap(temp,glyphs,map,gcnt);
    free(glyphs);
    free(map);
}

static struct contexttree *TreeNext(struct contexttree *cur) {
    struct contexttree *p;
    int i;

    if ( cur->branch_cnt!=0 )
return( cur->branches[0].branch );
    else {
	forever {
	    p = cur->parent;
	    if ( p==NULL )
return( NULL );
	    for ( i=0; i<p->branch_cnt; ++i ) {
		if ( p->branches[i].branch==cur ) {
		    ++i;
	    break;
		}
	    }
	    if ( i<p->branch_cnt )
return( p->branches[i].branch );
	    cur = p;
	}
    }
}

struct transitions2 {
    uint16 next_state, markit, mark_index, cur_index;
};

static int Transition2(int state,int mark, int mindex,int cindex,
	struct transitions2 *transitions,int *tcur) {
    struct transitions2 *t;
    int i;

    for ( i= *tcur-1; i>=0 ; --i ) {
	struct transitions2 *t = &transitions[i];
	if ( t->next_state==state && t->markit==mark && t->mark_index==mindex &&
		t->cur_index == cindex )
return( i );
    }
    t = &transitions[*tcur];

    t->next_state = state;
    t->markit = mark;
    t->mark_index = mindex;
    t->cur_index = cindex;
return( (*tcur)++ );
}

static int Transition2Find(struct contexttree *cur,int classnum,
	struct transitions2 *transitions,int *tcur) {
    /* if this tree has a rule on what to do with the classnum then make a */
    /*  transition which follows that rule. Otherwise use transition 0 */
    /*  (return to start state) */
    int i;

    for ( i=0; i<cur->branch_cnt; ++i ) {
	if ( cur->branches[i].classnum==classnum ) {
return( Transition2(cur->branches[i].branch->state,
		cur->branches[i].branch->state!=0
		    ? cur->branches[i].branch->markme?0x8000:0x0000
		    : cur->branches[i].branch->markme?0xc000:0x4000,
		cur->branches[i].branch->marked_index,
		cur->branches[i].branch->cur_index,
		transitions, tcur));
	}
    }

    /* If we're at the end of a match apply any substitutions we found */
    if ( cur->ends_here!=NULL )
return( Transition2(0, 0x4000, cur->marked_index, cur->cur_index,
		transitions, tcur));

return( 0 );
}

static int AnyActiveSubstrings(struct contexttree *tree,
	struct contexttree *cur,int class, uint16 *tlist, int classcnt) {
    struct fpc *any = &cur->rules[0].rule->u.class;
    int i,rc,j, b;

    for ( i=1; i<=cur->depth; ++i ) {
	for ( rc=0; rc<tree->rule_cnt; ++rc ) {
	    struct fpc *r = &tree->rules[rc].rule->u.class;
	    int ok = true;
	    for ( j=0; j<=cur->depth-i; ++j ) {
		if ( any->allclasses[j+i]!=r->allclasses[j] ) {
		    ok = false;
	    break;
		}
	    }
	    if ( ok && r->allclasses[j]==class ) {
		struct contexttree *sub = tree;
		for ( j=0; j<=cur->depth-i; ++j ) {
		    for ( b=0; b<sub->branch_cnt; ++b ) {
			if ( sub->branches[b].classnum==r->allclasses[j] ) {
			    sub = sub->branches[b].branch;
		    break;
			}
		    }
		}
		if ( tlist[sub->state*classcnt+class+3]!=0 )
return( tlist[sub->state*classcnt+class+3] );
	    }
	}
    }
return( tlist[class+3] );		/* Else try state 0 */
}

static int morx_dumpContGlyphFeatureFromClass(FILE *temp,FPST *fpst,
	struct contexttree *tree,
	struct alltabs *at, SplineFont *sf ) {
    int i, class_cnt, gcnt, ch, first;
    char *pt, *end;
    uint16 *map, *tlist;
    SplineChar **glyphs, *sc;
    struct contexttree *cur;
    int tmax, tcur;
    struct transitions2 *transitions;
    int stcnt;
    uint32 *substags;
    uint32 start, here, substable_pos;

    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	sf->chars[i]->lsidebearing = 0;

    /* Figure classes. Just use those in the fpst->nclass */
    gcnt = 0;
    for ( i=1; i<fpst->nccnt; ++i ) {
	for ( pt = fpst->nclass[i]; ; pt=end ) {
	    while ( *pt==' ' ) ++pt;
	    if ( *pt=='\0' )
	break;
	    for ( end=pt; *end!='\0' && *end!=' '; ++end );
	    ch = *end; *end = '\0';
	    sc = SFGetCharDup(sf,-1,pt);
	    *end = ch;
	    if ( sc!=NULL ) {
		sc->lsidebearing = i+3;		/* there are four built in classes, but built in class 1 maps to our class 0, (so we don't count class 1) */
		++gcnt;
	    }
	}
    }
    class_cnt = fpst->nccnt-1+4;		/* -1 for class 0. +4 for the four built in classes */
    if ( fpst->flags & pst_ignorecombiningmarks ) {
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=-1) {
	    if ( sf->chars[i]->lsidebearing==0 && IsMarkChar(sf->chars[i])) {
		sf->chars[i]->lsidebearing = class_cnt;
		++gcnt;
	    }
	}
	++class_cnt;			/* Add a class for the marks so we can ignore them */
    }
    glyphs = galloc((gcnt+1)*sizeof(SplineChar *));
    map = galloc((gcnt+1)*sizeof(uint16));
    gcnt = 0;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->lsidebearing!=0 ) {
	glyphs[gcnt] = sf->chars[i];
	map[gcnt++] = sf->chars[i]->lsidebearing;
    }
    glyphs[gcnt] = NULL;


    /* Give each subs tab an index into the mac's substitution lookups */
    substags = galloc(2*tree->next_state*sizeof(uint32));
    stcnt = 0;
    for ( cur = tree; cur!=NULL; cur = TreeNext(cur)) {
	cur->marked_index = cur->cur_index = 0xffff;
	if ( cur->applymarkedsubs!=0 ) {
	    for ( i=0; i<stcnt; ++i )
		if ( substags[i]==cur->applymarkedsubs )
	    break;
	    if ( i==stcnt )
		substags[stcnt++] = cur->applymarkedsubs;
	    cur->marked_index = i;
	}
	if ( cur->applycursubs!=0 ) {
	    for ( i=0; i<stcnt; ++i )
		if ( substags[i]==cur->applycursubs )
	    break;
	    if ( i==stcnt )
		substags[stcnt++] = cur->applycursubs;
	    cur->cur_index = i;
	}
    }


    /* Output the header */
    start = ftell(temp);
    putlong(temp,class_cnt);		/* # my classes+ 4 standard ones */
    putlong(temp,5*sizeof(uint32));	/* class offset */
    putlong(temp,0);			/* state offset */
    putlong(temp,0);			/* transition entry offset */
    putlong(temp,0);			/* substitution table offset */
    morx_lookupmap(temp,glyphs,map,gcnt);/* dump the class lookup table */
    free(glyphs); free(map);


    here = ftell(temp);
    fseek(temp,start+2*sizeof(uint32),SEEK_SET);
    putlong(temp,here-start);			/* Point to start of state arrays */
    fseek(temp,0,SEEK_END);

    /* Now build the state machine */
    /* Except for initial state, each state has only two entry points:	*/
    /*		from its parent						*/
    /*		from itself (deleted glyph/mark)			*/
    /* => should need at most 2*#states transitions + 1 for error return to initial */
    tmax = 3*tree->next_state+4;		/* but give ourselves some slop */
    transitions = galloc(tmax*sizeof(struct transitions2));
    /* transition 0 is used on failure. Goes back to initial state */
    transitions[0].next_state = 1; transitions[0].markit = 0x0000;
	transitions[0].mark_index = transitions[0].cur_index = 0xffff;
    tcur = 1;
    first = true;
    tlist = galloc(class_cnt*tree->next_state*sizeof(uint16));
    for ( cur = tree; cur!=NULL; cur = TreeNext(cur)) if ( cur->state!=0 ) {
	int off = cur->state*class_cnt;
	tlist[off+0] = 0;					/* end of text */
	tlist[off+1] = Transition2Find(cur,0,transitions,&tcur);/* out of bounds */
	tlist[off+2] = Transition2(cur->state,0,0xffff,0xffff,transitions,&tcur);	/* stay here, deleted glyph */
	tlist[off+3] = 0;					/* End of line */
	for ( i=1; i<fpst->nccnt; ++i )
	    tlist[off+3+i] = Transition2Find(cur,i,transitions,&tcur);
	if ( fpst->flags & pst_ignorecombiningmarks )
	    tlist[off+3+i] = tlist[2];				/* marks get ignored like delete chars */
	if ( first ) {
	    /* classes 0 and 1 should look the same as far as I'm concerned */
	    for ( i=0; i<class_cnt; ++i )
		tlist[i] = tlist[off+i];
	    first = false;
	}
    }
    if ( tcur>tmax ) GDrawIError("Too many transitions in morx_dumpContGlyphFeatureFromClass" );
    /* Do a sort of transitive closure on states, so if we are looking for */
    /*  either "abcd" or "bce", don't lose the "bce" inside "abce" */
    FPSTBuildAllClasses(fpst);
    for ( cur = tree; cur!=NULL; cur = TreeNext(cur)) if ( cur->state>1 ) {
	int off = cur->state*class_cnt;
	for ( i=1; i<fpst->nccnt; ++i ) if ( tlist[off+3+i]==0 )
	    tlist[off+3+i] = AnyActiveSubstrings(tree,cur,i, tlist,class_cnt);
    }
    for ( i=0; i<class_cnt*tree->next_state; ++i )
	putshort(temp,tlist[i]);
    free(tlist);
    FPSTFreeAllClasses(fpst);


    here = ftell(temp);
    fseek(temp,start+3*sizeof(uint32),SEEK_SET);
    putlong(temp,here-start);			/* Point to start of state arrays */
    fseek(temp,0,SEEK_END);

    /* Now the transitions */
    for ( i=0; i<tcur; ++i ) {
	putshort(temp,transitions[i].next_state);
	putshort(temp,transitions[i].markit);
	putshort(temp,transitions[i].mark_index );
	putshort(temp,transitions[i].cur_index );
    }
    free(transitions);


    substable_pos = ftell(temp);
    fseek(temp,start+4*sizeof(uint32),SEEK_SET);
    putlong(temp,substable_pos-start);		/* Point to start of substitution lookup offsets */
    fseek(temp,0,SEEK_END);

    /* And finally the substitutions */
    for ( i=0; i<stcnt; ++i )
	putlong(temp,0);	/* offsets to the substitutions */
    for ( i=0; i<stcnt; ++i ) {
	here = ftell(temp);
	fseek(temp,substable_pos+i*sizeof(uint32),SEEK_SET);
	putlong(temp,here-substable_pos);
	fseek(temp,0,SEEK_END);
	morx_dumpnestedsubs(temp,sf,substags[i]);
    }
    free(substags);
return( true );
}

static SplineChar **morx_cg_FigureClasses(SplineChar ***tables,int match_len,
	struct alltabs *at, int ***classes, int *cc, uint16 **mp, int *gc,
	FPST *fpst,SplineFont *sf) {
    int i,j,k, mask, max, class_cnt, gcnt;
    SplineChar ***temp, *sc, **glyphs, **gall;
    uint16 *map;
    int *nc;
    
    int *next;
    /* For each glyph used, figure out what coverage tables it gets used in */
    /*  then all the glyphs which get used in the same set of coverage tables */
    /*  can form one class */

    if ( match_len>10 )		/* would need too much space to figure out */
return( NULL );

    max=0;
    for ( i=0; i<match_len; ++i ) {
	for ( k=0; tables[i][k]!=NULL; ++k );
	if ( k>max ) max=k;
    }
    next = gcalloc(1<<match_len,sizeof(int));
    temp = galloc((1<<match_len)*sizeof(SplineChar **));

    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	sf->chars[i]->lsidebearing = 0;
	sf->chars[i]->ticked = false;
    }
    for ( i=0; i<match_len; ++i ) {
	for ( j=0; tables[i][j]!=NULL ; ++j )
	    tables[i][j]->lsidebearing |= 1<<i;
    }

    for ( i=0; i<match_len; ++i ) {
	for ( j=0; (sc=tables[i][j])!=NULL ; ++j ) if ( !sc->ticked ) {
	    mask = sc->lsidebearing;
	    if ( next[mask]==0 )
		temp[mask] = galloc(max*sizeof(SplineChar *));
	    temp[mask][next[mask]++] = sc;
	    sc->ticked = true;
	}
    }

    gall = gcalloc(at->maxp.numGlyphs,sizeof(SplineChar *));
    class_cnt = gcnt = 0;
    for ( i=0; i<(1<<match_len); ++i ) {
	if ( next[i]!=0 ) {
	    for ( k=0; k<next[i]; ++k ) {
		gall[temp[i][k]->ttf_glyph] = temp[i][k];
		temp[i][k]->lsidebearing = class_cnt;
	    }
	    ++class_cnt;
	    gcnt += next[i];
	    free(temp[i]);
	}
    }
    if ( fpst->flags & pst_ignorecombiningmarks ) {
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=-1 ) {
	    if ( sf->chars[i]->lsidebearing==0 && IsMarkChar(sf->chars[i])) {
		sf->chars[i]->lsidebearing = class_cnt;
		++gcnt;
	    }
	}
	++class_cnt;			/* Add a class for the marks so we can ignore them */
    }
    *cc = class_cnt+4;
    glyphs = galloc((gcnt+1)*sizeof(SplineChar *));
    map = galloc((gcnt+1)*sizeof(uint16));
    gcnt = 0;
    for ( i=0; i<at->maxp.numGlyphs; ++i ) if ( gall[i]!=NULL ) {
	glyphs[gcnt] = gall[i];
	map[gcnt++] = gall[i]->lsidebearing+4;	/* there are 4 standard classes, so our first class starts at 4 */
    }
    free(gall);
    free(temp);
    *gc = gcnt;
    *mp = map;

    nc = gcalloc(match_len,sizeof(int));
    *classes = galloc((match_len+1)*sizeof(int *));
    for ( i=0; i<match_len; ++i )
	(*classes)[i] = galloc((class_cnt+1)*sizeof(int));
    (*classes)[i] = NULL;

    class_cnt = 0;
    for ( i=0; i<(1<<match_len); ++i ) {
	if ( next[i]!=0 ) {
	    for ( j=0; j<match_len; ++j ) if ( i&(1<<j)) {
		(*classes)[j][nc[j]++] = class_cnt+4;	/* there are 4 standard classes, so our first class starts at 4 */
	    }
	    ++class_cnt;
	}
    }
    for ( j=0; j<match_len; ++j )
	(*classes)[j][nc[j]] = 0xffff;		/* End marker */

    free(next);
    free(nc);
return( glyphs );
}

static int morx_dumpContGlyphFeatureFromCoverage(FILE *temp,FPST *fpst,
	struct alltabs *at, SplineFont *sf ) {
    SplineChar ***tables, **glyphs;
    int **classes, class_cnt, gcnt;
    int i, j, k, k2, match_len;
    struct fpst_rule *r = &fpst->rules[0];
    int subspos = r->u.coverage.bcnt+r->lookups[0].seq, hasfinal = false;
    int substag = r->lookups[0].lookup_tag, finaltag=-1;
    uint32 here, start, substable_pos;
    uint16 *map;

    /* In one very specific case we can support two substitutions */
    if ( r->lookup_cnt==2 ) {
	hasfinal = true;
	if ( r->lookups[0].seq==r->u.coverage.ncnt-1 ) {
	    finaltag = substag;
	    subspos = r->u.coverage.bcnt+r->lookups[1].seq;
	    substag = r->lookups[1].lookup_tag;
	} else
	    finaltag = r->lookups[1].lookup_tag;
    }

    tables = galloc((r->u.coverage.ncnt+r->u.coverage.bcnt+r->u.coverage.fcnt+1)*sizeof(SplineChar **));
    for ( j=0, i=r->u.coverage.bcnt-1; i>=0; --i, ++j )
	tables[j] = SFGlyphsFromNames(sf,r->u.coverage.bcovers[i]);
    for ( i=0; i<r->u.coverage.ncnt; ++i, ++j )
	tables[j] = SFGlyphsFromNames(sf,r->u.coverage.ncovers[i]);
    for ( i=0; i<r->u.coverage.fcnt; ++i, ++j )
	tables[j] = SFGlyphsFromNames(sf,r->u.coverage.fcovers[i]);
    tables[j] = NULL;
    match_len = j;

    for ( i=0; i<match_len; ++i )
	if ( tables[i]==NULL || tables[i][0]==NULL )
return( false );

    glyphs = morx_cg_FigureClasses(tables,match_len,at,
	    &classes,&class_cnt,&map,&gcnt,fpst,sf);
    if ( glyphs==NULL )
return( false );

    for ( i=0; i<match_len; ++i )
	free(tables[i]);
    free(tables);

    start = ftell(temp);
    putlong(temp,class_cnt);		/* # my classes+ 4 standard ones */
    putlong(temp,5*sizeof(uint32));	/* class offset */
    putlong(temp,0);			/* state offset */
    putlong(temp,0);			/* transition entry offset */
    putlong(temp,0);			/* substitution table offset */
    morx_lookupmap(temp,glyphs,map,gcnt);/* dump the class lookup table */
    free(glyphs); free(map);

    here = ftell(temp);
    fseek(temp,start+2*sizeof(uint32),SEEK_SET);
    putlong(temp,here-start);			/* Point to start of state arrays */
    fseek(temp,0,SEEK_END);

    /* Now build the state machine */
    /* we have match_len states, and there are 2 initial states */
    /*  we transition from the initial state to our first state when we get */
    /*  any class which makes up the first coverage table. From the first */
    /*  to the second on any class which makes up the second ... */
    /* transition 0 goes to state 0 */
    /* transition 1 goes to state 2 */
    /* transition n goes to state n+1 (and one of these will set the mark) */
    /* initial states */
    for ( j=0; j<2; ++j ) {
	for ( i=0; i<class_cnt; ++i ) {
	    for ( k=0; classes[0][k]!=0xffff && classes[0][k]!=i; ++k );
	    putshort(temp,classes[0][k]==i?1:0);
	}
    }
    if ( !(fpst->flags & pst_ignorecombiningmarks ) ) {
	for ( j=1; j<match_len; ++j ) {
	    for ( i=0; i<class_cnt; ++i ) {
		for ( k=0; classes[j][k]!=0xffff && classes[j][k]!=i; ++k );
		if ( classes[j][k]==0xffff )
		    for ( k2=0; classes[0][k2]!=0xffff && classes[0][k2]!=i; ++k2 );
		putshort(temp,classes[j][k]==i?j+1:classes[j][k2]==i?1:0);
	    }
	}
    } else {
	for ( j=1; j<match_len; ++j ) {
	    for ( i=0; i<class_cnt-1; ++i ) {
		for ( k=0; classes[j][k]!=0xffff && classes[j][k]!=i; ++k );
		if ( classes[j][k]==0xffff )
		    for ( k2=0; classes[0][k2]!=0xffff && classes[0][k2]!=i; ++k2 );
		putshort(temp,classes[j][k]==i?j+1:classes[j][k2]==i?1:0);
	    }
	    putshort(temp,subspos==j?match_len+1:j);	/* a mark leaves us in the same class */
	}
    }

    here = ftell(temp);
    fseek(temp,start+3*sizeof(uint32),SEEK_SET);
    putlong(temp,here-start);			/* Point to start of transition arrays */
    fseek(temp,0,SEEK_END);
    /* transitions */
    putshort(temp,0); putshort(temp,0x0000); putshort(temp,0xffff); putshort(temp,0xffff);
    for ( j=0; j<match_len-1; ++j ) {
	putshort(temp,j+2); putshort(temp,subspos==j?0x8000:0x0000); putshort(temp,0xffff); putshort(temp,0xffff);
    }
    if ( subspos==j ) {
	putshort(temp,0); putshort(temp,0x4000); putshort(temp,0xffff); putshort(temp,0);
    } else {
	putshort(temp,0); putshort(temp,0x4000); putshort(temp,0); putshort(temp,hasfinal?1:0xffff);
    }
    if ( fpst->flags & pst_ignorecombiningmarks ) {
	putshort(temp,subspos); putshort(temp,0x0000); putshort(temp,0xffff); putshort(temp,0xffff);
    }

    substable_pos = ftell(temp);
    fseek(temp,start+4*sizeof(uint32),SEEK_SET);
    putlong(temp,substable_pos-start);		/* Point to start of substitution lookup offsets */
    fseek(temp,0,SEEK_END);
    if ( !hasfinal )
	putlong(temp,4);			/* Offset to first (and only) substitution lookup */
    else {
	putlong(temp,8);
	putlong(temp,0);
    }
    morx_dumpnestedsubs(temp,sf,substag);
    if ( hasfinal ) {
	here = ftell(temp);
	fseek(temp,substable_pos+4*sizeof(uint32),SEEK_SET);
	putlong(temp,here-substable_pos);
	fseek(temp,0,SEEK_END);
	morx_dumpnestedsubs(temp,sf,finaltag);
    }
return( true );
}

static struct feature *aat_dumpmorx_contextchainsubs(struct alltabs *at, SplineFont *sf,
	FILE *temp, struct feature *features) {
    FPST *fpst, *tempfpst;
    struct contexttree *tree;
    struct feature *cur;
    int chrs;

    for ( fpst = sf->possub; fpst!=NULL; fpst=fpst->next ) {
	tempfpst = fpst;
	tree = NULL;
	if ( fpst->format==pst_glyphs )
	    tempfpst = FPSTGlyphToClass( fpst );
	if ( tempfpst->format==pst_class )
	    tree = FPST2Tree(sf, tempfpst);
	if ( (tree!=NULL || FPSTisMacable(sf,tempfpst)) &&
		(cur = featureFromTag(tempfpst->tag))!=NULL ) {
	    cur->r2l = tempfpst->flags&pst_r2l ? 1 : 0;
	    cur->subtable_type = 1;		/* contextual glyph subs */
	    cur->feature_start = ftell(temp);
	    if ( (tempfpst->format==pst_coverage && morx_dumpContGlyphFeatureFromCoverage(temp,tempfpst,at,sf)) ||
		    (tempfpst->format==pst_class && morx_dumpContGlyphFeatureFromClass(temp,tempfpst,tree,at,sf))) {
		cur->next = features;
		features = cur;
		if ( (ftell(temp)-cur->feature_start)&1 )
		    putc('\0',temp);
		if ( (ftell(temp)-cur->feature_start)&2 )
		    putshort(temp,0);
		cur->feature_len = ftell(temp)-cur->feature_start;
		chrs = fpst->rules[0].u.coverage.ncnt+
			fpst->rules[0].u.coverage.bcnt+
			fpst->rules[0].u.coverage.fcnt;
		if ( chrs>at->os2.maxContext )
		    at->os2.maxContext = chrs;
	    } else
		chunkfree(cur,sizeof(struct feature));
	}
	if ( tempfpst!=fpst )
	    FPSTFree(tempfpst);
	if ( tree!=NULL )
	    TreeFree(tree);
    }
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
    features = aat_dumpmorx_contextchainsubs(at,sf,temp,features);
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
		if ( i<sf->charcnt && sf->chars[l]!=NULL ) {
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
return( false );
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

