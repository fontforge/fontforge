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
#include "macenc.h"
#include "asmfpst.h"
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
    int i, first=-1, last=-1;

    for ( i=0; i<cnt; ++i ) {
	if ( class[i] ) last = i;
	if ( class[i] && first==-1 ) first = i;
    }
    putshort(file,first);
    putshort(file,last-first+1);
    for ( i=first; i<=last; ++i )
	putshort(file,class[i]*mul+add);
}

static int morx_dumpASM(FILE *temp,ASM *sm, struct alltabs *at, SplineFont *sf );

struct kerncounts {
    int cnt;
    int vcnt;
    int mh, mv;
    int kccnt;
    int vkccnt;
    int ksm;
    int hsubs;
    int *hbreaks;
    int vsubs;
    int *vbreaks;
};

static int CountKerns(struct alltabs *at, SplineFont *sf, struct kerncounts *kcnt) {
    int i, cnt, vcnt, j, kccnt=0, vkccnt=0, ksm=0, mh, mv;
    KernPair *kp;
    KernClass *kc;
    ASM *sm;

    cnt = mh = vcnt = mv = 0;
    for ( i=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
	j = 0;
	for ( kp = sf->glyphs[at->gi.bygid[i]]->kerns; kp!=NULL; kp=kp->next )
	    if ( kp->off!=0 && kp->sc->ttf_glyph!=-1 &&
		    LookupHasDefault(kp->subtable->lookup ))
		++cnt, ++j;
	if ( j>mh ) mh=j;
	j=0;
	for ( kp = sf->glyphs[at->gi.bygid[i]]->vkerns; kp!=NULL; kp=kp->next )
	    if ( kp->off!=0 && kp->sc->ttf_glyph!=-1 &&
		    LookupHasDefault(kp->subtable->lookup ))
		++vcnt, ++j;
	if ( j>mv ) mv=j;
    }
    kcnt->cnt = cnt;
    kcnt->vcnt = vcnt;
    kcnt->mh = mh;
    kcnt->mv = mv;
    kcnt->hbreaks = kcnt->vbreaks = NULL;
    if ( cnt>=10000 ) {
	/* the sub-table size is 6*cnt+14 or so and needs to be less 65535 */
	/*  so break it up into little bits */
	/* We might not need this when applemode is set because the subtable */
	/*  length is a long. BUT... there's a damn binsearch header with */
	/*  shorts in it still */
	int b=0;
	kcnt->hbreaks = malloc((at->gi.gcnt+1)*sizeof(int));
	cnt = 0;
	for ( i=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
	    j = 0;
	    for ( kp = sf->glyphs[at->gi.bygid[i]]->kerns; kp!=NULL; kp=kp->next )
		if ( kp->off!=0 && LookupHasDefault(kp->subtable->lookup ))
		    ++j;
	    if ( (cnt+j)*6>64000L && cnt!=0 ) {
		kcnt->hbreaks[b++] = cnt;
		cnt = 0;
	    }
	    cnt += j;
	}
	kcnt->hbreaks[b++] = cnt;
	kcnt->hsubs = b;
    } else if ( cnt!=0 )
	kcnt->hsubs = 1;
    else
	kcnt->hsubs = 0;
    if ( vcnt>=10000 ) {
	int b=0;
	kcnt->vbreaks = malloc((at->gi.gcnt+1)*sizeof(int));
	vcnt = 0;
	for ( i=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
	    j = 0;
	    for ( kp = sf->glyphs[at->gi.bygid[i]]->vkerns; kp!=NULL; kp=kp->next )
		if ( kp->off!=0 && LookupHasDefault(kp->subtable->lookup))
		    ++j;
	    if ( (vcnt+j)*6>64000L && vcnt!=0 ) {
		kcnt->vbreaks[b++] = vcnt;
		vcnt = 0;
	    }
	    vcnt += j;
	}
	kcnt->vbreaks[b++] = vcnt;
	kcnt->vsubs = b;
    } else if ( vcnt!=0 )
	kcnt->vsubs = 1;
    else
	kcnt->vsubs = 0;

    if ( at->applemode ) {	/* if we aren't outputting Apple's extensions to kerning (by classes, and by state machine) then don't check for those extensions */
	for ( kc=sf->kerns; kc!=NULL; kc = kc->next ) if ( LookupHasDefault(kc->subtable->lookup) )
	    ++kccnt;
	for ( kc=sf->vkerns; kc!=NULL; kc = kc->next ) if ( LookupHasDefault(kc->subtable->lookup) )
	    ++vkccnt;
	for ( sm=sf->sm; sm!=NULL; sm=sm->next )
	    if ( sm->type == asm_kern )
		++ksm;
    }
    kcnt->kccnt = kccnt;
    kcnt->vkccnt = vkccnt;
    kcnt->ksm = ksm;
return( kcnt->hsubs + kcnt->vsubs + kccnt + vkccnt + ksm );
}

static void ttf_dumpsfkerns(struct alltabs *at, SplineFont *sf, int tupleIndex, int version) {
    struct kerncounts kcnt;
    int i, j, k, m, c, gid, tot, km;
    KernPair *kp;
    KernClass *kc;
    ASM *sm;
    uint16 *glnum, *offsets;
    int isv;
    int tupleMask = tupleIndex==-1 ? 0 : 0x2000;
    int b, bmax;
    int *breaks;
    int winfail=0;
    int subtableBeginPos,subtableEndPos;

    if ( CountKerns(at,sf,&kcnt)==0 )
return;

    if ( tupleIndex==-1 ) tupleIndex = 0;
    
    for ( isv=0; isv<2; ++isv ) {
	c = isv ? kcnt.vcnt : kcnt.cnt;
	bmax = isv ? kcnt.vsubs : kcnt.hsubs;
	breaks = isv ? kcnt.vbreaks : kcnt.hbreaks;
	if ( c!=0 ) {
	    km = isv ? kcnt.mv : kcnt.mh;
	    glnum = malloc(km*sizeof(uint16));
	    offsets = malloc(km*sizeof(uint16));
	    gid = 0;
	    for ( b=0; b<bmax; ++b ) {
		c = bmax==1 ? c : breaks[b];

		// skip subtable header because we don't know the number of kern pairs yet
		subtableBeginPos=ftell(at->kern);
		if(version==0) fseek(at->kern,7*sizeof(uint16),SEEK_CUR);
		else fseek(at->kern,8*sizeof(uint16),SEEK_CUR);

		for ( tot = 0; gid<at->gi.gcnt && tot<c; ++gid ) if ( at->gi.bygid[gid]!=-1 ) {
		    SplineChar *sc = sf->glyphs[at->gi.bygid[gid]];
		    // if requested, omit kern pairs with unmapped glyphs
		    // (required for compatibility with non-OpenType-aware Windows applications)
		    if( (at->gi.flags&ttf_flag_oldkernmappedonly) && (unsigned)(sc->unicodeenc)>0xFFFF ) continue;
		    m = 0;
		    for ( kp = isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kp->next ) {
			// if requested, omit kern pairs with unmapped glyphs
			// (required for compatibility with non-OpenType-aware Windows applications)
			if( (at->gi.flags&ttf_flag_oldkernmappedonly) && (unsigned)(kp->sc->unicodeenc)>0xFFFF ) continue;
			if ( kp->off!=0 && kp->sc->ttf_glyph!=-1 &&
				LookupHasDefault(kp->subtable->lookup)) {
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
			    /* check if a pair will cause problems on Windows */
			    /* If the glyph is outside BMP, so either unicode >0xffff */
			    /*  or -1. Cast to unsigned catches both */
			    if( (unsigned)(sf->glyphs[at->gi.bygid[gid]]->unicodeenc)>0xFFFF ||
				    (unsigned)(sf->glyphs[at->gi.bygid[glnum[j]]]->unicodeenc)>0xFFFF )
				winfail++;
			}
		    }
		    for ( j=0; j<m; ++j ) {
			putshort(at->kern,gid);
			putshort(at->kern,glnum[j]);
			putshort(at->kern,offsets[j]);
		    }
		    tot += m;
		}

		// now we can fill the subtable header
		c=tot;
		subtableEndPos=ftell(at->kern);
		fseek(at->kern,subtableBeginPos,SEEK_SET);
		if ( version==0 ) {
		    putshort(at->kern,0);		/* subtable version */
		    if ( c>10920 )
			ff_post_error(_("Too many kern pairs"),_("The 'kern' table supports at most 10920 kern pairs in a subtable"));
		    putshort(at->kern,(7+3*c)*sizeof(uint16)); /* subtable length */
		    putshort(at->kern,!isv);	/* coverage, flags=hor/vert&format=0 */
		} else {
		    putlong(at->kern,(8+3*c)*sizeof(uint16)); /* subtable length */
		    /* Apple's new format has a completely different coverage format */
		    putshort(at->kern,(isv?0x8000:0)| /* format 0, horizontal/vertical flags (coverage) */
				    tupleMask);
		    putshort(at->kern,tupleIndex);
		}
		putshort(at->kern,c);
		for ( i=1,j=0; i<=c; i<<=1, ++j );
		i>>=1; --j;
		putshort(at->kern,i*6);		/* binary search headers */
		putshort(at->kern,j);
		putshort(at->kern,6*(c-i));
		fseek(at->kern,subtableEndPos,SEEK_SET);
	    }
	    free(offsets);
	    free(glnum);
	}
    }
    free(kcnt.hbreaks); free(kcnt.vbreaks);

    if( winfail > 0 )
	ff_post_error(_("Kerning is likely to fail on Windows"),_(
		"Note: On Windows many apps can have problems with this font's kerning, because %d of its glyph kern pairs cannot be mapped to unicode-BMP kern pairs (eg, they have a Unicode value of -1) To avoid this, go to Generate, Options, and check the \"Windows-compatible \'kern\'\" option."),
	    winfail);

    if ( at->applemode ) for ( isv=0; isv<2; ++isv ) {
	for ( kc=isv ? sf->vkerns : sf->kerns; kc!=NULL; kc=kc->next ) if ( LookupHasDefault(kc->subtable->lookup) ) {
	    /* If we are here, we must be using version 1 */
	    uint32 len_pos = ftell(at->kern), pos;
	    uint16 *class1, *class2;
	    int first_cnt = kc->first_cnt;

	    /* OpenType fonts can actually have a set of glyphs in class[0] of*/
	    /*  the first class. This happens when there are glyphs in the */
	    /*  coverage table which are not in any of the classes. Otherwise */
	    /*  class 0 is sort of useless in opentype */
	    if ( kc->firsts[0]!=NULL )
		++first_cnt;

	    putlong(at->kern,0); /* subtable length */
	    putshort(at->kern,(isv?0x8002:2)|	/* format 2, horizontal/vertical flags (coverage) */
			    tupleMask);
	    putshort(at->kern,tupleIndex);

	    putshort(at->kern,sizeof(uint16)*kc->second_cnt);
	    putshort(at->kern,0);		/* left classes */
	    putshort(at->kern,0);		/* right classes */
	    putshort(at->kern,16);		/* Offset to array, next byte */

	    if ( kc->firsts[0]!=NULL ) {
		/* Create a dummy class to correspond to the mac's class 0 */
		/*  all entries will be 0 */
		for ( i=0 ; i<kc->second_cnt; ++i )
		    putshort(at->kern,0);
	    }
	    for ( i=0; i<kc->first_cnt*kc->second_cnt; ++i )
		putshort(at->kern,kc->offsets[i]);

	    pos = ftell(at->kern);
	    fseek(at->kern,len_pos+10,SEEK_SET);
	    putshort(at->kern,pos-len_pos);
	    fseek(at->kern,pos,SEEK_SET);
	    class1 = ClassesFromNames(sf,kc->firsts,kc->first_cnt,at->maxp.numGlyphs,NULL,true);
	    DumpKernClass(at->kern,class1,at->maxp.numGlyphs,16,sizeof(uint16)*kc->second_cnt);
	    free(class1);

	    pos = ftell(at->kern);
	    fseek(at->kern,len_pos+12,SEEK_SET);
	    putshort(at->kern,pos-len_pos);
	    fseek(at->kern,pos,SEEK_SET);
	    class2 = ClassesFromNames(sf,kc->seconds,kc->second_cnt,at->maxp.numGlyphs,NULL,true);
	    DumpKernClass(at->kern,class2,at->maxp.numGlyphs,0,sizeof(uint16));
	    free(class2);

	    pos = ftell(at->kern);
	    fseek(at->kern,len_pos,SEEK_SET);
	    putlong(at->kern,pos-len_pos);
	    fseek(at->kern,pos,SEEK_SET);
	}
    }

    if ( at->applemode ) if ( kcnt.ksm!=0 ) {
	for ( sm=sf->sm; sm!=NULL; sm=sm->next ) if ( sm->type == asm_kern ) {
	    uint32 len_pos = ftell(at->kern), pos;

	    putlong(at->kern,0); 		/* subtable length */
	    putshort(at->kern,((sm->flags&0x8000)?0x8001:1)|	/* format 1, horizontal/vertical flags (coverage) */
			    tupleMask);
	    putshort(at->kern,tupleIndex);
	    morx_dumpASM(at->kern,sm,at,sf);

	    pos = ftell(at->kern);
	    fseek(at->kern,len_pos,SEEK_SET);
	    putlong(at->kern,pos-len_pos);
	    fseek(at->kern,pos,SEEK_SET);
	}
    }
}

void ttf_dumpkerns(struct alltabs *at, SplineFont *sf) {
    int i, mmcnt=0, sum;
    int version;
    MMSet *mm = at->dovariations ? sf->mm : NULL;
    struct kerncounts kcnt;
    int must_use_old_style = 0;

    if ( !at->applemode && (!at->opentypemode || (at->gi.flags&ttf_flag_oldkern)) ) {
	must_use_old_style = true;
	SFKernClassTempDecompose(sf,false);
	mm = NULL;
    } else {
	if ( mm!=NULL ) {
	    for ( i=0; i<mm->instance_count; ++i ) {
		mmcnt += CountKerns(at,mm->instances[i],&kcnt);
		free(kcnt.hbreaks); free(kcnt.vbreaks);
	    }
	    sf = mm->normal;
	}
    }

    sum = CountKerns(at,sf,&kcnt);
    free(kcnt.hbreaks); free(kcnt.vbreaks);
    if ( sum==0 && mmcnt==0 ) {
	if ( must_use_old_style )
	    SFKernCleanup(sf,false);
return;
    }

    /* Old kerning format (version 0) uses 16 bit quantities */
    /* Apple's new format (version 0x00010000) uses 32 bit quantities */
    at->kern = tmpfile();
    if ( must_use_old_style  ||
	    ( kcnt.kccnt==0 && kcnt.vkccnt==0 && kcnt.ksm==0 && mmcnt==0 )) {
	/* MS does not support format 1,2,3 kern sub-tables so if we have them */
	/*  we might as well admit that this table is for apple only and use */
	/*  the new format apple recommends. Otherwise, use the old format */
	/* If we might need to store tuple data, use the new format */
	putshort(at->kern,0);			/* version */
	putshort(at->kern,sum);			/* number of subtables */
	version = 0;
    } else {
	putlong(at->kern,0x00010000);		/* version */
	putlong(at->kern,sum+mmcnt);		/* number of subtables */
	version = 1;
    }

    ttf_dumpsfkerns(at, sf, -1, version);
    if ( mm!=NULL ) {
	for ( i=0; i<mm->instance_count; ++i )
	    ttf_dumpsfkerns(at, mm->instances[i], i, version);
    }
    if ( must_use_old_style )
	SFKernCleanup(sf,false);

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
	if ( !sc->lig_caret_cnt_fixed ) {
	    for ( j=pst->u.lcaret.cnt-1; j>=0 && pst->u.lcaret.carets[j]==0; --j );
	    if ( j==-1 )
		pst = NULL;
	} else {
	    if ( pst->u.lcaret.cnt==0 )
		pst = NULL;
	}
    }
return( pst );
}

void aat_dumplcar(struct alltabs *at, SplineFont *sf) {
    int i, j, k, l, seg_cnt, tot, last, offset;
    PST *pst;
    FILE *lcar=NULL;
    SplineChar *sc;
    /* We do four passes. The first just calculates how much space we will need */
    /*  the second provides the top-level lookup table structure */
    /*  the third provides the arrays of offsets needed for type 4 lookup tables */
    /*  the fourth provides the actual data on the ligature carets */

    for ( k=0; k<4; ++k ) {
	for ( i=seg_cnt=tot=0; i<at->gi.gcnt; ++i )
		if ( at->gi.bygid[i]!=-1 &&
		    (pst = haslcaret(sc = sf->glyphs[at->gi.bygid[i]]))!=NULL ) {
	    if ( k==1 )
		tot = 0;
	    else if ( k==2 ) {
		putshort(lcar,offset);
		offset += 2 + 2*LigCaretCnt(sc);
	    } else if ( k==3 ) {
		putshort(lcar,LigCaretCnt(sc));
		for ( l=0; l<pst->u.lcaret.cnt; ++l )
		    if ( pst->u.lcaret.carets[l]!=0 || sc->lig_caret_cnt_fixed )
			putshort(lcar,pst->u.lcaret.carets[l]);
	    }
	    last = i;
	    for ( j=i+1, ++tot; j<at->gi.gcnt && at->gi.bygid[j]!=-1; ++j ) {
		if ( (pst = haslcaret(sc = sf->glyphs[at->gi.bygid[j]]))== NULL )
	    break;
		++tot;
		last = j;
		if ( k==2 ) {
		    putshort(lcar,offset);
		    offset += 2 + 2*LigCaretCnt(sc);
		} else if ( k==3 ) {
		    putshort(lcar,LigCaretCnt(sc));
		    for ( l=0; l<pst->u.lcaret.cnt; ++l )
			if ( pst->u.lcaret.carets[l]!=0 || sc->lig_caret_cnt_fixed )
			    putshort(lcar,pst->u.lcaret.carets[l]);
		}
	    }
	    if ( k==1 ) {
		putshort(lcar,last);
		putshort(lcar,i);
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
	    offset = /*4+2+*/6*2 + seg_cnt*6 + 6 /* fake segment at end */;
		    /* Offset relative to lookup table, not to lcar_start */
		    /* Or, that's true while we build the lookup table. Once we */
		    /*  start working on the data offsets they are relative to */
		    /*  lcar_start */
	} else if ( k==1 ) {		/* flag entry */
	    putshort(lcar,0xffff);
	    putshort(lcar,0xffff);
	    putshort(lcar,0);

	    offset += 6;	/* Now offsets are relative to lcar_start */
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

/* Each lookup gets its own subtable, so there may be multiple subtables */
/*  with the same feature/setting. The subtables will be ordered the same */
/*  way the lookups are, which might lead to awkwardness if there are many */
/*  chains and the same feature occurs in several of them */
/* (only the default language will be used) */
struct feature {
    int16 featureType, featureSetting;
    MacFeat *mf, *smf;
    struct macsetting *ms, *sms;
    unsigned int vertOnly: 1;
    unsigned int r2l: 1;	/* I think this is the "descending" flag */
    unsigned int needsOff: 1;
    unsigned int singleMutex: 1;
    unsigned int dummyOff: 1;
    uint8 subtable_type;
    int chain;
    int32 flag, offFlags;
    uint32 feature_start;
    uint32 feature_len;		/* Does not include header yet */
    struct feature *next;	/* features in output order */
    struct feature *nexttype;	/* features in feature/setting order */
    struct feature *nextsame;	/* all features with the same feature/setting */
    int setting_cnt, setting_index, real_index;
};

static struct feature *featureFromSubtable(SplineFont *sf, struct lookup_subtable *sub );
static int PSTHasTag(PST *pst, uint32 tag);

static void morxfeaturesfree(struct feature *features) {
    struct feature *n;

    for ( ; features!=NULL; features=n ) {
	n = features->next;
	chunkfree( features,sizeof(*features) );
    }
}

static void mort_classes(FILE *temp,SplineFont *sf,struct glyphinfo *gi) {
    int first, last, i, cnt;
    /* Mort tables just have a trimmed byte array for the classes */

    for ( first=0; first<gi->gcnt; ++first )
	if ( gi->bygid[first]!=-1 && sf->glyphs[gi->bygid[first]]->lsidebearing!=1 )
    break;
    for ( last=gi->gcnt-1; last>first; --last )
	if ( gi->bygid[last]!=-1 && sf->glyphs[gi->bygid[last]]->lsidebearing!=1 )
    break;
    cnt = last-first+1;

    putshort(temp,first);
    putshort(temp,cnt);
    for ( i=first; i<=last; ++i )
	if ( gi->bygid[i]==-1 )
	    putc(1,temp);
	else
	    putc(sf->glyphs[gi->bygid[i]]->lsidebearing,temp);
    if ( cnt&1 )
	putc(1,temp);			/* Pad to a word boundary */
}

static void morx_lookupmap(FILE *temp,SplineChar **glyphs,uint16 *maps,int gcnt) {
    int i, j, k, l, seg_cnt, tot, last, offset;
    /* We do four passes. The first just calculates how much space we will need (if any) */
    /*  the second provides the top-level lookup table structure */
    /*  the third provides the arrays of offsets needed for type 4 lookup tables */

    for ( k=0; k<3; ++k ) {
	for ( i=seg_cnt=tot=0; i<gcnt; ++i ) {
	    if ( glyphs[i]==NULL )
	continue;
	    if ( k==1 )
		tot = 0;
	    else if ( k==2 ) {
		putshort(temp,maps[i]);
	    }
	    last = i;
	    for ( j=i+1, ++tot; j<gcnt && glyphs[j]!=NULL && glyphs[j]->ttf_glyph==glyphs[i]->ttf_glyph+j-i; ++j ) {
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

static struct feature *aat_dumpmorx_substitutions(struct alltabs *at, SplineFont *sf,
	FILE *temp, struct feature *features, struct lookup_subtable *sub) {
    int i, k, gcnt;
    SplineChar *sc, *msc, **glyphs;
    uint16 *maps;
    struct feature *cur;
    PST *pst;

    for ( k=0; k<2; ++k ) {
	gcnt = 0;
	for ( i=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
	    sc = sf->glyphs[at->gi.bygid[i]];
	    for ( pst=sc->possub; pst!=NULL && pst->subtable!=sub; pst=pst->next );
	    if ( pst!=NULL ) {
		if ( k==1 ) {
		    msc = SFGetChar(sf,-1,pst->u.subs.variant);
		    glyphs[gcnt] = sc;
		    if ( msc!=NULL && msc->ttf_glyph!=-1 ) {
			maps[gcnt++] = msc->ttf_glyph;
		    } else if ( msc==NULL &&
			    strcmp(pst->u.subs.variant,MAC_DELETED_GLYPH_NAME)==0 ) {
			maps[gcnt++] = 0xffff;
		    }
		} else
		    ++gcnt;
	    }
	}
	if ( k==0 ) {
	    if ( gcnt==0 )
return( features );
	    glyphs = malloc((gcnt+1)*sizeof(SplineChar *));
	    maps = malloc((gcnt+1)*sizeof(uint16));
	} else {
	    glyphs[gcnt] = NULL; maps[gcnt] = 0;
	}
    }

    cur = featureFromSubtable(sf,sub);
    cur->next = features;
    cur->r2l = sub->lookup->lookup_flags&pst_r2l ? true : false;
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
return( features);
}

static LigList *LigListMatchSubtable(SplineFont *sf,LigList *ligs,
	struct lookup_subtable *sub) {
    LigList *l;

    for ( l=ligs; l!=NULL; l=l->next )
	if ( l->lig->subtable==sub )
return( l );
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
	struct lookup_subtable *sub, struct alltabs *at, SplineFont *sf,
	int ignoremarks) {
    LigList *l;
    struct splinecharlist *comp;
    uint16 *used = calloc(at->maxp.numGlyphs,sizeof(uint16));
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
	for ( l=glyphs[i]->ligofme; l!=NULL; l=l->next ) if ( l->lig->subtable==sub ) {
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
	for ( i=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
	    if ( IsMarkChar(sf->glyphs[at->gi.bygid[i]])) {
		anymarks = true;
		++charcnt;
		used[i] = class;
	    }
	}
	if ( anymarks )
	    ++class;
    }
    cglyphs = malloc((charcnt+1)*sizeof(SplineChar *));
    map = malloc((charcnt+1)*sizeof(uint16));
    j=0;
    for ( i=k=0; i<at->maxp.numGlyphs; ++i ) if ( used[i] ) {
	j = at->gi.bygid[i];
	if ( j!=-1 ) {
	    cglyphs[k] = sf->glyphs[j];
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
    states = malloc(state_max*sizeof(struct transition *));
    states[0] = calloc(class,sizeof(struct transition));	/* Initial state */
    states[1] = calloc(class,sizeof(struct transition));	/* other Initial state */
    for ( i=0; i<gcnt; ++i ) {
	if ( state_cnt>=state_max )
	    states = realloc(states,(state_max += 40)*sizeof(struct transition *));
	base = state_cnt;
	states[0][used[glyphs[i]->ttf_glyph]].next_state = state_cnt;
	states[1][used[glyphs[i]->ttf_glyph]].next_state = state_cnt;
	states[state_cnt++] = calloc(class,sizeof(struct transition));
	for ( l=glyphs[i]->ligofme; l!=NULL; l=l->next ) if ( l->lig->subtable==sub ) {
	    if ( l->ccnt > maxccnt ) maxccnt = l->ccnt;
	    last = base;
	    for ( comp = l->components; comp!=NULL; comp=comp->next ) {
		if ( states[last][used[comp->sc->ttf_glyph]].next_state==0 ) {
		    if ( comp->next==NULL )
			states[last][used[comp->sc->ttf_glyph]].l = l;
		    else {
			states[last][used[comp->sc->ttf_glyph]].next_state = state_cnt;
			if ( state_cnt>=state_max )
			    states = realloc(states,(state_max += 40)*sizeof(struct transition *));
			last = state_cnt;
			states[state_cnt++] = calloc(class,sizeof(struct transition));
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
    trans = malloc(class*state_cnt*sizeof(struct trans_entries));
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
    actions = malloc(trans_cnt*maxccnt*sizeof(uint32));
    components = malloc(trans_cnt*maxccnt*sizeof(uint16));
    lig_glyphs = malloc(trans_cnt*sizeof(uint16));
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
	IError( "Offset wrong in morx ligature table\n" );
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
	FILE *temp, struct feature *features, struct lookup_subtable *sub) {
    int i, k, gcnt;
    SplineChar *sc, *ssc, **glyphs;
    struct feature *cur;
    LigList *l;

    glyphs = malloc((at->maxp.numGlyphs+1)*sizeof(SplineChar *));
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	sf->glyphs[i]->ticked = false;

    for ( i=0; i<at->gi.gcnt; ++i )
	    if ( at->gi.bygid[i]!=-1 && !(sc=sf->glyphs[at->gi.bygid[i]])->ticked &&
		    (l = LigListMatchSubtable(sf,sc->ligofme,sub))!=NULL ) {
	int ignoremarks = sub->lookup->lookup_flags & pst_ignorecombiningmarks ? 1 : 0 ;
	for ( k=i, gcnt=0; k<at->gi.gcnt; ++k )
		if ( at->gi.bygid[k]!=-1 &&
			(ssc=sf->glyphs[at->gi.bygid[k]])!=NULL && !ssc->ticked &&
			LigListMatchSubtable(sf,ssc->ligofme,sub)) {
	    glyphs[gcnt++] = ssc;
	    ssc->ticked = true;
	}
	glyphs[gcnt] = NULL;
	cur = featureFromSubtable(sf,sub);
	cur->next = features;
	features = cur;
	cur->subtable_type = 2;		/* ligature */
	cur->feature_start = ftell(temp);
	morx_dumpLigaFeature(temp,glyphs,gcnt,sub,at,sf,ignoremarks);
	if ( (ftell(temp)-cur->feature_start)&1 )
	    putc('\0',temp);
	if ( (ftell(temp)-cur->feature_start)&2 )
	    putshort(temp,0);
	cur->feature_len = ftell(temp)-cur->feature_start;
	cur->r2l = sub->lookup->lookup_flags&pst_r2l ? true : false;
    }

    free(glyphs);
return( features);
}

static void morx_dumpnestedsubs(FILE *temp,SplineFont *sf,OTLookup *otl,struct glyphinfo *gi) {
    int i, j, gcnt;
    PST *pst;
    SplineChar **glyphs, *sc;
    uint16 *map;
    struct lookup_subtable *sub = otl->subtables;	/* Mac can't have more than one subtable/lookup */

    for ( j=0; j<2; ++j ) {
	gcnt = 0;
	for ( i = 0; i<gi->gcnt; ++i ) if ( gi->bygid[i]!=-1 ) {
	    for ( pst=sf->glyphs[gi->bygid[i]]->possub;
		    pst!=NULL && pst->subtable!=sub;  pst=pst->next );
	    if ( pst!=NULL && pst->type==pst_substitution &&
		    (sc=SFGetChar(sf,-1,pst->u.subs.variant))!=NULL &&
		    sc->ttf_glyph!=-1 ) {
		if ( j ) {
		    glyphs[gcnt] = sf->glyphs[gi->bygid[i]];
		    map[gcnt] = sc->ttf_glyph;
		}
		++gcnt;
	    }
	}
	if ( !j ) {
	    glyphs = malloc((gcnt+1)*sizeof(SplineChar *));
	    map = malloc(gcnt*sizeof(uint16));
	    glyphs[gcnt] = NULL;
	}
    }
    morx_lookupmap(temp,glyphs,map,gcnt);
    free(glyphs);
    free(map);
}

static uint16 *NamesToGlyphs(SplineFont *sf,char *names,uint16 *cnt) {
    char *pt, *start;
    int c, ch;
    uint16 *ret;
    SplineChar *sc;

    for ( c=0, pt=names; *pt; ++pt )
	if ( *pt==' ' ) ++c;
    ret = malloc((c+1)*sizeof(uint16));

    for ( c=0, pt=names; *pt; ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	start = pt;
	while ( *pt!=' ' && *pt!='\0' ) ++pt;
	ch = *pt; *pt='\0';
	sc = SFGetChar(sf,-1,start);
	*pt = ch;
	if ( sc!=NULL && sc->ttf_glyph!=-1 )
	    ret[c++] = sc->ttf_glyph;
    }
    *cnt = c;
return( ret );
}

static int morx_dumpASM(FILE *temp,ASM *sm, struct alltabs *at, SplineFont *sf ) {
    int i, j, k, gcnt, ch;
    char *pt, *end;
    uint16 *map;
    SplineChar **glyphs, *sc;
    int stcnt, tcnt;
    struct ins { char *names; uint16 len,pos; uint16 *glyphs; } *subsins=NULL;
    OTLookup **subslookups=NULL;
    uint32 start, here, substable_pos, state_offset;
    struct transdata { uint16 transition, mark_index, cur_index; } *transdata;
    struct trans { uint16 ns, flags, mi, ci; } *trans;
    int ismort = sm->type == asm_kern;
    FILE *kernvalues;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	sf->glyphs[i]->lsidebearing = 1;

    gcnt = 0;
    for ( i=4; i<sm->class_cnt; ++i ) {
	for ( pt = sm->classes[i]; ; pt=end ) {
	    while ( *pt==' ' ) ++pt;
	    if ( *pt=='\0' )
	break;
	    for ( end=pt; *end!='\0' && *end!=' '; ++end );
	    ch = *end; *end = '\0';
	    sc = SFGetChar(sf,-1,pt);
	    *end = ch;
	    if ( sc!=NULL ) {
		sc->lsidebearing = i;
		++gcnt;
	    }
	}
    }
    glyphs = malloc((gcnt+1)*sizeof(SplineChar *));
    map = malloc((gcnt+1)*sizeof(uint16));
    gcnt = 0;
    for ( i=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 && sf->glyphs[at->gi.bygid[i]]->lsidebearing!=1 ) {
	glyphs[gcnt] = sf->glyphs[at->gi.bygid[i]];
	map[gcnt++] = sf->glyphs[at->gi.bygid[i]]->lsidebearing;
    }
    glyphs[gcnt] = NULL;

    /* Give each subs tab an index into the mac's substitution lookups */
    transdata = calloc(sm->state_cnt*sm->class_cnt,sizeof(struct transdata));
    stcnt = 0;
    subslookups = NULL; subsins = NULL;
    if ( sm->type==asm_context ) {
	subslookups = malloc(2*sm->state_cnt*sm->class_cnt*sizeof(OTLookup));
	for ( j=0; j<sm->state_cnt*sm->class_cnt; ++j ) {
	    struct asm_state *this = &sm->state[j];
	    transdata[j].mark_index = transdata[j].cur_index = 0xffff;
	    if ( this->u.context.mark_lookup!=NULL ) {
		for ( i=0; i<stcnt; ++i )
		    if ( subslookups[i]==this->u.context.mark_lookup )
		break;
		if ( i==stcnt )
		    subslookups[stcnt++] = this->u.context.mark_lookup;
		transdata[j].mark_index = i;
	    }
	    if ( this->u.context.cur_lookup!=NULL ) {
		for ( i=0; i<stcnt; ++i )
		    if ( subslookups[i]==this->u.context.cur_lookup )
		break;
		if ( i==stcnt )
		    subslookups[stcnt++] = this->u.context.cur_lookup;
		transdata[j].cur_index = i;
	    }
	}
    } else if ( sm->type==asm_insert ) {
	subsins = malloc(2*sm->state_cnt*sm->class_cnt*sizeof(struct ins));
	for ( j=0; j<sm->state_cnt*sm->class_cnt; ++j ) {
	    struct asm_state *this = &sm->state[j];
	    transdata[j].mark_index = transdata[j].cur_index = 0xffff;
	    if ( this->u.insert.mark_ins!=0 ) {
		for ( i=0; i<stcnt; ++i )
		    if ( strcmp(subsins[i].names,this->u.insert.mark_ins)==0 )
		break;
		if ( i==stcnt ) {
		    subsins[stcnt].pos = stcnt==0 ? 0 : subsins[stcnt-1].pos +
							subsins[stcnt-1].len;
		    subsins[stcnt].names = this->u.insert.mark_ins;
		    subsins[stcnt].glyphs = NamesToGlyphs(sf,subsins[stcnt].names,&subsins[stcnt].len);
		    ++stcnt;
		}
		transdata[j].mark_index = subsins[i].pos;
	    }
	    if ( this->u.insert.cur_ins!=0 ) {
		for ( i=0; i<stcnt; ++i )
		    if ( strcmp(subsins[i].names,this->u.insert.cur_ins)==0 )
		break;
		if ( i==stcnt ) {
		    subsins[stcnt].pos = stcnt==0 ? 0 : subsins[stcnt-1].pos +
							subsins[stcnt-1].len;
		    subsins[stcnt].names = this->u.insert.cur_ins;
		    subsins[stcnt].glyphs = NamesToGlyphs(sf,subsins[stcnt].names,&subsins[stcnt].len);
		    ++stcnt;
		}
		transdata[j].cur_index = subsins[i].pos;
	    }
	}
    } else if ( sm->type==asm_kern ) {
	int off=0;
	kernvalues = tmpfile();
	for ( j=0; j<sm->state_cnt*sm->class_cnt; ++j ) {
	    struct asm_state *this = &sm->state[j];
	    transdata[j].mark_index = 0xffff;
	    if ( this->u.kern.kcnt!=0 ) {
		for ( k=0; k<j; ++k )
		    if ( sm->state[k].u.kern.kcnt==this->u.kern.kcnt &&
			    memcmp(sm->state[k].u.kern.kerns,this->u.kern.kerns,
				    this->u.kern.kcnt*sizeof(int16))==0 )
		break;
		if ( k!=j )
		    transdata[j].mark_index = transdata[k].mark_index;
		else {
		    transdata[j].mark_index = off;
		    off += this->u.kern.kcnt*sizeof(int16);
		    /* kerning values must be output backwards */
		    for ( k=this->u.kern.kcnt-1; k>=1; --k )
			putshort(kernvalues,this->u.kern.kerns[k]&~1);
		    /* And the last one must be odd */
		    putshort(kernvalues,this->u.kern.kerns[0]|1);
		}
	    }
	}
    }

    trans = malloc(sm->state_cnt*sm->class_cnt*sizeof(struct trans));
    tcnt = 0;
    for ( j=0; j<sm->state_cnt*sm->class_cnt; ++j ) {
	struct asm_state *this = &sm->state[j];
	for ( i=0; i<tcnt; ++i )
	    if ( trans[i].ns==this->next_state && trans[i].flags==this->flags &&
		    trans[i].mi==transdata[j].mark_index &&
		    trans[i].ci==transdata[j].cur_index )
	break;
	if ( i==tcnt ) {
	    trans[tcnt].ns = this->next_state;
	    trans[tcnt].flags = this->flags;
	    trans[tcnt].mi = transdata[j].mark_index;
	    trans[tcnt++].ci = transdata[j].cur_index;
	}
	transdata[j].transition = i;
    }


    /* Output the header */
    start = ftell(temp);
    if ( ismort /* old format still used for kerning */ ) {
	putshort(temp,sm->class_cnt);
	putshort(temp,5*sizeof(uint16));	/* class offset */
	putshort(temp,0);			/* state offset */
	putshort(temp,0);			/* transition entry offset */
	putshort(temp,0);			/* kerning values offset */
	mort_classes(temp,sf,&at->gi);			/* dump the class table */
    } else {
	putlong(temp,sm->class_cnt);
	if ( sm->type==asm_indic ) {
	    putlong(temp,4*sizeof(uint32));	/* class offset */
	    putlong(temp,0);			/* state offset */
	    putlong(temp,0);			/* transition entry offset */
	} else {
	    putlong(temp,5*sizeof(uint32));	/* class offset */
	    putlong(temp,0);			/* state offset */
	    putlong(temp,0);			/* transition entry offset */
	    putlong(temp,0);			/* substitution/insertion table offset */
	}
	morx_lookupmap(temp,glyphs,map,gcnt);/* dump the class lookup table */
    }
    free(glyphs); free(map);


    state_offset = ftell(temp)-start;
    if ( ismort ) {
	fseek(temp,start+2*sizeof(uint16),SEEK_SET);
	putshort(temp,state_offset);		/* Point to start of state arrays */
    } else {
	fseek(temp,start+2*sizeof(uint32),SEEK_SET);
	putlong(temp,state_offset);		/* Point to start of state arrays */
    }
    fseek(temp,0,SEEK_END);

    if ( ismort ) {
	for ( j=0; j<sm->state_cnt*sm->class_cnt; ++j )
	    putc(transdata[j].transition,temp);
	if ( ftell(temp)&1 )
	    putc(0,temp);			/* Pad to a word boundry */
    } else {
	for ( j=0; j<sm->state_cnt*sm->class_cnt; ++j )
	    putshort(temp,transdata[j].transition);
    }
    free(transdata);

    here = ftell(temp);
    if ( ismort ) {
	fseek(temp,start+3*sizeof(uint16),SEEK_SET);
	putshort(temp,here-start);		/* Point to start of transition arrays */
    } else {
	fseek(temp,start+3*sizeof(uint32),SEEK_SET);
	putlong(temp,here-start);		/* Point to start of transition arrays */
    }
    fseek(temp,0,SEEK_END);

    /* Now the transitions */
    if ( sm->type==asm_kern ) {
	substable_pos = here+tcnt*2*sizeof(int16);
	for ( i=0; i<tcnt; ++i ) {
	    /* mort tables use an offset rather than the state number */
	    putshort(temp,trans[i].ns*sm->class_cnt+state_offset);
	    if ( trans[i].mi!=0xffff )
		trans[i].flags |= substable_pos-start+trans[i].mi;
	    putshort(temp,trans[i].flags);
	}
    } else {
	for ( i=0; i<tcnt; ++i ) {
	    putshort(temp,trans[i].ns);
	    putshort(temp,trans[i].flags);
	    if ( sm->type!=asm_indic && sm->type!=asm_kern ) {
		putshort(temp,trans[i].mi );
		putshort(temp,trans[i].ci );
	    }
	}
    }
    free(trans);

    if ( sm->type==asm_context ) {
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
	    morx_dumpnestedsubs(temp,sf,subslookups[i],&at->gi);
	}
	free(subslookups);
    } else if ( sm->type==asm_insert ) {
	substable_pos = ftell(temp);
	fseek(temp,start+4*sizeof(uint32),SEEK_SET);
	putlong(temp,substable_pos-start);		/* Point to start of insertions */
	fseek(temp,0,SEEK_END);

	for ( i=0; i<stcnt; ++i ) {
	    for ( j=0; j<subsins[i].len; ++j )
		putshort(temp,subsins[i].glyphs[j]);
	    free(subsins[i].glyphs);
	}
	free(subsins);
    } else if ( sm->type==asm_kern ) {
	if ( substable_pos!=ftell(temp) )
	    IError( "Kern Values table in wrong place.\n" );
	fseek(temp,start+4*sizeof(uint16),SEEK_SET);
	putshort(temp,substable_pos-start);		/* Point to start of insertions */
	fseek(temp,0,SEEK_END);
	if ( !ttfcopyfile(temp,kernvalues,substable_pos,"kern-subtable")) at->error = true;
    }
return( true );
}

static struct feature *aat_dumpmorx_asm(struct alltabs *at, SplineFont *sf,
	FILE *temp, struct feature *features, ASM *sm) {
    struct feature *cur;

    cur = featureFromSubtable(sf,sm->subtable);
    cur->vertOnly = sm->flags&0x8000?1:0;
    cur->r2l = sm->flags&0x4000?1:0;
    cur->subtable_type = sm->type;		/* contextual glyph subs */
    cur->feature_start = ftell(temp);
    if ( morx_dumpASM(temp,sm,at,sf)) {
	cur->next = features;
	features = cur;
	if ( (ftell(temp)-cur->feature_start)&1 )
	    putc('\0',temp);
	if ( (ftell(temp)-cur->feature_start)&2 )
	    putshort(temp,0);
	cur->feature_len = ftell(temp)-cur->feature_start;
    } else
	chunkfree(cur,sizeof(struct feature));
return( features);
}

static struct feature *aat_dumpmorx_cvtopentype(struct alltabs *at, SplineFont *sf,
	FILE *temp, struct feature *features, struct lookup_subtable *sub) {
    ASM *sm;

    if ( FPSTisMacable(sf,sub->fpst)) {
	sm = ASMFromFPST(sf,sub->fpst,true);
	if ( sm!=NULL ) {
	    features = aat_dumpmorx_asm(at,sf,temp,features,sm);
	    ASMFree(sm);
	}
    }
return( features );
}

static int IsOtfArabicFormFeature(OTLookup *otl) {
    FeatureScriptLangList *fl;

    for ( fl=otl->features; fl!=NULL; fl=fl->next ) {
	if (( fl->featuretag == CHR('i','n','i','t') ||
		fl->featuretag==CHR('m','e','d','i') ||
		fl->featuretag==CHR('f','i','n','a') ||
		fl->featuretag==CHR('i','s','o','l') ) &&
		    scriptsHaveDefault(fl->scripts))
return( true );
    }
return( false );
}

static int HasCursiveConnectionSM(SplineFont *sf) {
    int featureType, featureSetting;
    uint32 tag;
    ASM *sm;

    if ( OTTagToMacFeature(CHR('i','s','o','l'),&featureType,&featureSetting) ) {
	tag = (featureType<<16) | featureSetting;
	for ( sm = sf->sm; sm!=NULL; sm=sm->next ) {
	    if ( sm->subtable->lookup->features->featuretag==tag )
return( true );
	}
    }
    for ( sm = sf->sm; sm!=NULL; sm=sm->next ) {
	if ( sm->subtable->lookup->features->featuretag==CHR('i','s','o','l') )
return( true );
    }
return( false );
}

static uint32 *FormedScripts(SplineFont *sf) {
    OTLookup *otl;
    uint32 *ret = NULL;
    int scnt=0, smax=0;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    int i;

    for ( otl= sf->gsub_lookups; otl!=NULL; otl=otl->next ) {
	if ( otl->lookup_type == gsub_single ) {
	    for ( fl=otl->features; fl!=NULL; fl=fl->next ) {
		if ( fl->featuretag == CHR('i','n','i','t') ||
			fl->featuretag==CHR('m','e','d','i') ||
			fl->featuretag==CHR('f','i','n','a') ||
			fl->featuretag==CHR('i','s','o','l') ) {
		    for ( sl=fl->scripts; sl!=NULL; sl=sl->next ) {
			for ( i=0; i<sl->lang_cnt; ++i ) {
			    if ( (i<MAX_LANG ? sl->langs[i] : sl->morelangs[i-MAX_LANG])==DEFAULT_LANG ) {
				if ( scnt<=smax )
				    ret = realloc(ret,(smax+=5)*sizeof(uint32));
				ret[scnt++] = sl->script;
			    }
			}
		    }
		}
	    }
	}
    }
    if ( scnt==0 )
return( NULL );
    if ( scnt<=smax )
	ret = realloc(ret,(smax+=1)*sizeof(uint32));
    ret[scnt] = 0;
return( ret );
}
    
int Macable(SplineFont *sf, OTLookup *otl) {
    int ft, fs;
    FeatureScriptLangList *features;

    switch ( otl->lookup_type ) {
    /* These lookup types are mac only */
      case kern_statemachine: case morx_indic: case morx_context: case morx_insert:
return( true );
    /* These lookup types or OpenType only */
      case gsub_multiple: case gsub_alternate:
      case gpos_single: case gpos_cursive: case gpos_mark2base:
      case gpos_mark2ligature: case gpos_mark2mark:
return( false );
    /* These are OpenType only, but they might be convertable to a state */
    /*  machine */
      case gsub_context:
      case gsub_contextchain: case gsub_reversecchain:
      case gpos_context: case gpos_contextchain:
	if ( sf==NULL || sf->sm!=NULL )
return( false );
	/* Else fall through into the test on the feature tag */;
    /* These two can be expressed in both, and might be either */
      case gsub_single: case gsub_ligature: case gpos_pair:
	for ( features = otl->features; features!=NULL; features = features->next ) {
	    if ( features->ismac || OTTagToMacFeature(features->featuretag,&ft,&fs))
return( true );
	}
    }
return( false );
}

static struct feature *aat_dumpmorx_cvtopentypeforms(struct alltabs *at, SplineFont *sf,
	FILE *temp, struct feature *features) {
    ASM *sm;
    uint32 *scripts;
    int featureType, featureSetting;
    int i;
    OTLookup *otl;

    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;

    for ( otl=sf->gsub_lookups; otl!=NULL; otl=otl->next )
	if ( Macable(sf,otl) && otl->lookup_type==gsub_single && IsOtfArabicFormFeature(otl))
	    otl->ticked = true;

    if ( OTTagToMacFeature(CHR('i','s','o','l'),&featureType,&featureSetting) ) {
	scripts = FormedScripts(sf);
	for ( i=0; scripts[i]!=0; ++i ) {
	    sm = ASMFromOpenTypeForms(sf,scripts[i]);
	    if ( sm!=NULL ) {
		features = aat_dumpmorx_asm(at,sf,temp,features,sm);
		ASMFree(sf->sm);
	    }
	}
	free(scripts);
    }
return( features );
}

static struct feature *featuresReverse(struct feature *features) {
    struct feature *p, *n;

    p = NULL;
    while ( features!=NULL ) {
	n = features->next;
	features->next = p;
	p = features;
	features = n;
    }
return( p );
}

static struct feature *featuresOrderByType(struct feature *features) {
    struct feature *f, **all;
    int i, j, cnt/*, saw_default*/;

    for ( cnt=0, f=features; f!=NULL; f=f->next, ++cnt );
    if ( cnt==1 ) {
return( features );
    }
    all = malloc(cnt*sizeof(struct feature *));
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
    for ( i=0; i<cnt-1; ++i )
	all[i]->nexttype = all[i+1];
    all[cnt-1]->nexttype = NULL;
    features = all[0];
    free( all );
return( features );
}

static struct feature *AddExclusiveNoops(SplineFont *sf, struct feature *features) {
    struct feature *f, *n, *def, *p, *t;
    /* mutually exclusive features need to have a setting which does nothing */

    for ( f=features; f!=NULL; f=n ) {
	n= f->nexttype;
	if ( f->mf!=NULL && f->mf->ismutex ) {
	    def = NULL;
	    for ( n=f; n!=NULL && n->featureType==f->featureType; n=n->nexttype ) {
		if ( n->featureSetting==f->mf->default_setting )
		    def = n;
	    }
	    if ( def==NULL ) {
		t = chunkalloc(sizeof(struct feature));
		*t = *f;
		t->feature_start = 0; t->feature_len=0; t->next = NULL;
		t->featureSetting = f->mf->default_setting;
		t->ms = FindMacSetting(sf,t->featureType,f->mf->default_setting,&t->sms);
		t->flag = 0;
		t->dummyOff = true;
		if ( f==features )
		    p = NULL;
		else
		    for ( p=features; p->nexttype!=f; p=p->nexttype );
		n = f;
		while ( n!=NULL && n->featureType==t->featureType && n->featureSetting<t->featureSetting ) {
		    p = n;
		    n = n->nexttype;
		}
		t->nexttype = n;
		if ( p==NULL )
		    features = t;
		else
		    p->nexttype = t;
		while ( n!=NULL && n->featureType==t->featureType )
		    n=n->nexttype;
	    }
	}
    }
return( features );
}

static void SetExclusiveOffs(struct feature *features) {
    struct feature *f, *n;
    int offFlags;
    /* mutually exclusive features need to have a setting which does nothing */

    for ( f=features; f!=NULL; f=n ) {
	n= f->nexttype;
	if ( f->mf!=NULL && f->mf->ismutex ) {
	    offFlags=0;
	    for ( n=f; n!=NULL && n->featureType==f->featureType; n=n->nexttype ) {
		offFlags |= n->flag;
	    }
	    for ( n=f; n!=NULL && n->featureType==f->featureType; n=n->nexttype )
		n->offFlags = ~(offFlags&~n->flag);
	}
    }
return;
}

static void aat_dumpfeat(struct alltabs *at, SplineFont *sf, struct feature *feature) {
    int scnt, fcnt, cnt;
    struct feature *f, *n, *p;
    int k;
    uint32 offset;
    int strid = at->next_strid;
    int fn=0;
    MacFeat *mf, *smf;
    struct macsetting *ms, *sms;
    /* Dump the 'feat' table which is a connection between morx features and */
    /*  the name table */
    /* We do three passes. The first just calculates how much space we will need */
    /*  the second provides names for the feature types */
    /*  and the third provides names for the feature settings */
    /* As we fill up the feat table we also create an array of strings */
    /*  (strid, char *pointer) which will be used by the 'name' table to */
    /*  give names to the features and their settings */
    /* The mac documentation says that the features should be sorted by feature type */
    /*  This is a lie. Features should appear in the same order they appear */
    /*  in the morx table, otherwise WorldText goes blooie */
    /* WorldText doesn't exist any more. Perhaps the morx table needs to be */
    /*  sorted by feature id too? No, it can't be. Feature 0 must come last */

    if ( feature==NULL )
return;

    fcnt = scnt = 0;
    for ( k=0; k<3; ++k ) {
	if ( k==1 ) {
		/* FeatureName entry for All Typographics */
	    mf = FindMacFeature(sf,0,&smf);
	    if ( (mf!=NULL && mf->featname!=NULL) || (smf!=NULL && smf->featname!=NULL)) {
		at->feat_name[fn].mn = mf!=NULL ? mf->featname : NULL;
		at->feat_name[fn].smn = smf!=NULL ? smf->featname : NULL;
		at->feat_name[fn++].strid = strid;
	    }
	    putshort(at->feat,0);
	    putshort(at->feat,1);
	    putlong(at->feat,offset);
	    putshort(at->feat,0x0000);	/* non exclusive */
	    putshort(at->feat,strid++);
	    offset += 1*4;		/* (1 setting, 4 bytes) All Features */
	} else if ( k==2 ) {
		/* Setting Name Array for All Typographic Features */
	    ms = FindMacSetting(sf,0,0,&sms);
	    if ( (ms!=NULL && ms->setname!=NULL) || (sms!=NULL && sms->setname!=NULL)) {
		at->feat_name[fn].mn = ms!=NULL ? ms->setname: NULL;
		at->feat_name[fn].smn = sms!=NULL ? sms->setname: NULL;
		at->feat_name[fn++].strid = strid;
	    }
	    putshort(at->feat,0);
	    putshort(at->feat,strid++);
	}
	for ( f=feature; f!=NULL; f=n ) {
	    cnt=1;
	    if ( k!=2 ) {
		p = f;
		for ( n=f->nexttype; n!=NULL && n->featureType==f->featureType; n = n->nexttype ) {
		    if ( p->featureSetting!=n->featureSetting ) {
			++cnt;
			p = n;
		    }
		}
	    } else {
		p = f;
		for ( n=f; n!=NULL && n->featureType==f->featureType; n = n->nexttype ) {
		    if ( n==f || p->featureSetting!=n->featureSetting ) {
			if (( n->ms!=NULL && n->ms->setname!=NULL ) ||
				( n->sms!=NULL && n->sms->setname!=NULL)) {
			    at->feat_name[fn].mn = n->ms!=NULL ? n->ms->setname : NULL;
			    at->feat_name[fn].smn = n->sms!=NULL ? n->sms->setname : NULL;
			    at->feat_name[fn++].strid = strid;
			}
			putshort(at->feat,n->featureSetting);
			putshort(at->feat,strid++);
			p = n;
		    }
		}
	    }
	    if ( k==0 ) {
		++fcnt;
		scnt += cnt;
	    } else if ( k==1 ) {
		if ( (f->mf!=NULL && f->mf->featname!=NULL) || (f->smf!=NULL && f->smf->featname!=NULL) ) {
		    at->feat_name[fn].mn = f->mf!=NULL ? f->mf->featname : NULL;
		    at->feat_name[fn].smn = f->smf!=NULL ? f->smf->featname : NULL;
		    at->feat_name[fn++].strid = strid;
		}
		putshort(at->feat,f->featureType);
		putshort(at->feat,cnt);
		putlong(at->feat,offset);
		putshort(at->feat,f->mf!=NULL && f->mf->ismutex?(0xc000|f->mf->default_setting):
			0);
		putshort(at->feat,strid++);
		offset += 4*cnt;
	    }
	}
	if ( k==0 ) {
	    ++fcnt;		/* Add one for "All Typographic Features" */
	    ++scnt;		/* Add one for All Features */
	    at->feat = tmpfile();
	    at->feat_name = malloc((fcnt+scnt+1)*sizeof(struct feat_name));
	    putlong(at->feat,0x00010000);
	    putshort(at->feat,fcnt);
	    putshort(at->feat,0);
	    putlong(at->feat,0);
	    offset = 12 /* header */ + fcnt*12;
	}
    }
    memset( &at->feat_name[fn],0,sizeof(struct feat_name));
    at->next_strid = strid;

    at->featlen = ftell(at->feat);
    if ( at->featlen&2 )
	putshort(at->feat,0);
}

static int featuresAssignFlagsChains(struct feature *features, struct feature *feature_by_type) {
    int bit, cnt, chain, fcnt, i, mybit;
    struct feature *f, *n, *p;
    uint16 chains_features[32];
    uint32 chains_bitindex[32];		/* Index for bit of first setting of this feature */

    if ( features==NULL )
return( 0 );

    /* A feature may have several subtables which need not be contiguous in */
    /*  the feature list */
    /* Indeed we could have a feature in several different chains */
    /* Sigh */
    /* we figure out how many possible settings there are for each feature */
    /*  and reserve that many bits for the feature in all chains in which it */
    /*  occurs */
    /* Note that here we count dummy settings (they need turn off bits) */
    /*  so we use feature_by_type */
    for ( f=feature_by_type; f!=NULL; f=n ) {
	cnt=0;
	p = NULL;
	for ( n=f; n!=NULL && n->featureType==f->featureType; n=n->nexttype ) {
	    if ( p==NULL || n->featureSetting != p->featureSetting ) {
		++cnt;
		p = n;
	    }
	    n->setting_index = cnt-1;
	}
	for ( n=f; n!=NULL && n->featureType==f->featureType; n=n->nexttype )
	    n->setting_cnt = cnt;
    }
    /* When we counted flags we need to count the dummy features for turning  */
    /*  things off. Those features live in features_by_type. When we put      */
    /*  things in chains we want only the meaningful features, and we want    */
    /*  them to be properly ordered. That we get from the "features" list     */
    fcnt = 0; chain = 0; bit=0;
    for ( f=features; f!=NULL; f=f->next ) {
	for ( i=0; i<fcnt && chains_features[i]!=f->featureType; ++i );
	if ( i==fcnt ) {
	    if ( bit+f->setting_cnt>=32 ) {
		++chain;
		bit = 0;
		fcnt = 0;
	    }
	    chains_features[fcnt] = f->featureType;
	    chains_bitindex[fcnt++] = bit;
	    mybit = bit;
	    bit += f->setting_cnt;
	} else
	    mybit = chains_bitindex[i];
	f->real_index = mybit+f->setting_index;
	f->flag = 1<<f->real_index;
	if ( f->mf!=NULL && f->mf->ismutex ) {
	    int off = (~((~0)<<f->setting_cnt))<<mybit;
	    off &= !f->flag;
	    f->offFlags = off;
	} else {
	    if ( f->featureSetting&1 ) {
		for ( n=feature_by_type; n!=NULL &&
			(n->featureType!=f->featureType || n->featureSetting!=f->featureSetting+1);
			n=n->next );
	    } else {
		for ( n=feature_by_type; n!=NULL &&
			(n->featureType!=f->featureType || n->featureSetting!=f->featureSetting+1);
			n=n->next );
	    }
	    if ( n!=NULL )
		f->offFlags = 1<<(mybit+n->setting_index);
	    else
		f->offFlags = ~0;
	}
	f->chain = chain;
    }
return( chain+1 );
}

static void morxDumpChain(struct alltabs *at,struct feature *features,
	struct feature *features_by_type, int chain, FILE *temp) {
    uint32 def_flags=0;
    struct feature *f, *n;
    uint32 chain_start, end;
    char *buf;
    int len, tot, fs_cnt, sub_cnt;
    struct feature *all[32];
    int i,offFlags, last_ri=-1, last_f=-1, ri;

    memset(all,0,sizeof(all));
    for ( f=features, fs_cnt=sub_cnt=0; f!=NULL; f=f->next ) {
	if ( f->chain==chain ) {
	    if ( all[f->real_index]==NULL ) {
		int base = f->real_index-f->setting_index;
		/* Note we use features_by_type here. It will have the default*/
		/*  settings for features, and will be ordered nicely */
		for ( n=features_by_type; n!=NULL; n=n->nexttype ) {
		    if ( n->featureType==f->featureType && n->chain==chain ) {
			n->nextsame = all[base+n->setting_index];
			all[base+n->setting_index] = n;
			if ( n->ms!=NULL && n->ms->initially_enabled )
			    def_flags |= n->flag;
		    }
		}
	    }
	    ++sub_cnt;
	}
    }

    /* Chain header */
    chain_start = ftell(at->morx);
    putlong(at->morx,def_flags);
    putlong(at->morx,0);		/* Fix up length later */
    putlong(at->morx,0);		/* fix up feature count */
    putlong(at->morx,sub_cnt);		/* subtable cnt */

    /* Features */
    fs_cnt = 0;
    for ( i=0; i<32; ++i ) if ( all[i]!=NULL ) {
	putshort(at->morx,all[i]->featureType);
	putshort(at->morx,all[i]->featureSetting);
	if ( all[i]->dummyOff ) {
	    putlong(at->morx,0);
	    if ( last_f==all[i]->featureType )
		ri = last_ri;
	    else if ( i<31 && all[i+1]!=NULL && all[i+1]->featureType == all[i]->featureType )
		ri = i+1 - all[i+1]->real_index;
	    else
		ri = 0;		/* This can't happen */
	} else {
	    putlong(at->morx,1<<i);
	    ri = i-all[i]->real_index;
	    last_ri = ri; last_f = all[i]->featureType;
	}
	offFlags = all[i]->offFlags;
	if ( ri>0 )
	    offFlags<<=(ri);
	else if ( ri<0 )
	    offFlags>>=(-ri);
	putlong(at->morx,offFlags);
	++fs_cnt;

	if ( all[i]->needsOff && (i==31 || all[i+1]==NULL ||
		all[i+1]->featureType!=all[i]->featureType ||
		all[i+1]->featureSetting!=all[i]->featureSetting+1 )) {
	    putshort(at->morx,all[i]->featureType);
	    putshort(at->morx,all[i]->featureSetting+1);
	    putlong(at->morx,0);
	    putlong(at->morx,all[i]->offFlags & ~all[i]->flag );
	    ++fs_cnt;
	}
	/* I used to have code to output the default setting of a mutex */
	/*  but I should already have put that in the feature list */
    }
    /* The feature list of every chain must end with these two features */
    putshort(at->morx,0);		/* All Typo Features */
    putshort(at->morx,0);		/* All Features */
    putlong(at->morx,0xffffffff);	/* enable */
    putlong(at->morx,0xffffffff);	/* disable */
    putshort(at->morx,0);		/* All Typo Features */
    putshort(at->morx,1);		/* No Features */
    putlong(at->morx,0);		/* enable */
    putlong(at->morx,0);		/* disable */
    fs_cnt += 2;

    buf = malloc(16*1024);
    /* Subtables */
    for ( f=features; f!=NULL; f=f->next ) if ( f->chain==chain ) {
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
		IError( "Disk error\n" );
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
    putlong(at->morx,fs_cnt);
    fseek(at->morx,0,SEEK_END);
}

void aat_dumpmorx(struct alltabs *at, SplineFont *sf) {
    FILE *temp = tmpfile();
    struct feature *features = NULL, *features_by_type;
    int nchains, i;
    OTLookup *otl;
    struct lookup_subtable *sub;

    /* Arabic Form features all need to be merged together and formed into */
    /*  a cursive connection state machine. So the first time we see one of */
    /*  we handle all of them. After that we ignore all of them. Note: if */
    /*  OpenType has them happening in different orders, that information */
    /*  will be lost. All will be processed at once. */
    for ( otl = sf->gsub_lookups; otl!=NULL; otl=otl->next )
	otl->ticked = false;

    SFLigaturePrepare(sf);

    /* Retain the same lookup ordering */
    for ( otl = sf->gsub_lookups; otl!=NULL; otl=otl->next ) {
	if ( !Macable(sf,otl))
    continue;
	if ( otl->lookup_type==gsub_single && IsOtfArabicFormFeature(otl) ) {
	    if ( otl->ticked )
		/* Already processed */;
	    else if ( HasCursiveConnectionSM(sf) )
		/* Skip the OpenType conversion and use the native state machine */;
	    else
		features = aat_dumpmorx_cvtopentypeforms(at,sf,temp,features);
	} else {
	    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
		switch ( otl->lookup_type ) {
		  case gsub_single:
		    features = aat_dumpmorx_substitutions(at,sf,temp,features,sub);
		  break;
		  case gsub_ligature:
		    features = aat_dumpmorx_ligatures(at,sf,temp,features,sub);
		  break;
		  case morx_indic: case morx_context: case morx_insert:
		    features = aat_dumpmorx_asm(at,sf,temp,features,sub->sm);
		  break;
		  default:
		    if ( sf->sm==NULL )
			features = aat_dumpmorx_cvtopentype(at,sf,temp,features,sub);
		}
	    }
	}
    }

    SFLigatureCleanup(sf);

    if ( features==NULL ) {
	fclose(temp);
return;
    }
    /* The features are in reverse execution order */
    features = featuresReverse(features);
    /* But the feature table requires them in numeric order */
    features_by_type = featuresOrderByType(features);
    features_by_type = AddExclusiveNoops(sf,features_by_type);
    aat_dumpfeat(at, sf, features_by_type);
    nchains = featuresAssignFlagsChains(features,features_by_type);
    SetExclusiveOffs(features_by_type);

    at->morx = tmpfile();
    putlong(at->morx,0x00020000);
    putlong(at->morx,nchains);
    for ( i=0; i<nchains; ++i )
	morxDumpChain(at,features,features_by_type,i,temp);
    fclose(temp);
    morxfeaturesfree(features_by_type);
    
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
	    if ( PSTHasTag(pst,CHR('l','f','b','d')) ) {
		*left = pst;
		if ( *right )
return( true );
	    } else if ( PSTHasTag(pst,CHR('r','t','b','d')) ) {
		*right = pst;
		if ( *left )
return( true );
	    }
	}
    }
return( *left!=NULL || *right!=NULL );
}

void aat_dumpopbd(struct alltabs *at, SplineFont *_sf) {
    int i, j, k, l, seg_cnt, tot, last, offset;
    PST *left, *right;
    FILE *opbd=NULL;
    /* We do four passes. The first just calculates how much space we will need (if any) */
    /*  the second provides the top-level lookup table structure */
    /*  the third provides the arrays of offsets needed for type 4 lookup tables */
    /*  the fourth provides the actual data on the optical bounds */
    SplineChar *sc;

    for ( k=0; k<4; ++k ) {
	for ( i=seg_cnt=tot=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
	    l = 0;
	    sc = _sf->glyphs[at->gi.bygid[i]];
	    if ( haslrbounds(sc,&left,&right) ) {
		if ( k==1 )
		    tot = 0;
		else if ( k==2 ) {
		    putshort(opbd,offset);
		    offset += 8;
		} else if ( k==3 ) {
		    putshort(opbd,left!=NULL?-left->u.pos.xoff:0);
		    putshort(opbd,0);		/* top */
		    putshort(opbd,right!=NULL?-right->u.pos.h_adv_off:0);
		    putshort(opbd,0);		/* bottom */
		}
		last = i;
		for ( j=i+1, ++tot; j<at->gi.gcnt; ++j ) {
		    if ( at->gi.bygid[i]==-1 || !haslrbounds(_sf->glyphs[at->gi.bygid[j]],&left,&right) )
		break;
		    ++tot;
		    last = j;
		    if ( k==2 ) {
			putshort(opbd,offset);
			offset += 8;
		    } else if ( k==3 ) {
			putshort(opbd,left!=NULL?-left->u.pos.xoff:0);
			putshort(opbd,0);		/* top */
			putshort(opbd,right!=NULL?-right->u.pos.h_adv_off:0);
			putshort(opbd,0);		/* bottom */
		    }
		}
		if ( k==1 ) {
		    putshort(opbd,last);
		    putshort(opbd,i);
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
	    putshort(opbd,0);		/* data are distances (not control points) */

	    putshort(opbd,4);		/* Lookup table format 4 */
		/* Binary search header */
	    putshort(opbd,6);		/* Entry size */
	    putshort(opbd,seg_cnt);	/* Number of segments */
	    for ( j=0,l=1; l<=seg_cnt; l<<=1, ++j );
	    --j; l>>=1;
	    putshort(opbd,6*l);
	    putshort(opbd,j);
	    putshort(opbd,6*(seg_cnt-l));
	    /* offset from start of lookup, not table */
	    offset = 6*2/* format, binsearch*/ + seg_cnt*6 +6 /*flag entry */;
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

uint16 *props_array(SplineFont *sf,struct glyphinfo *gi) {
    uint16 *props;
    int i;
    SplineChar *sc, *bsc;
    int dir, isfloat, isbracket, offset, doit=false;
    AnchorPoint *ap;
    PST *pst;
    int p;

    props = calloc(gi->gcnt+1,sizeof(uint16));
    props[gi->gcnt] = -1;

    for ( i=0; i<gi->gcnt; ++i ) if ( (p = gi->bygid==NULL ? i : gi->bygid[i])!=-1 ) {
	sc = sf->glyphs[p];
	if ( sc!=NULL && (gi->bygid==NULL || sc->ttf_glyph!=-1 )) {
	    dir = 0;
	    if ( sc->unicodeenc>=0x10300 && sc->unicodeenc<=0x103ff )
		dir = 0;
	    else if ( sc->unicodeenc>=0x10800 && sc->unicodeenc<=0x10fff )
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
		bsc = SFGetChar(sf,tomirror(sc->unicodeenc),NULL);
		if ( bsc!=NULL && bsc->ttf_glyph-sc->ttf_glyph>-8 && bsc->ttf_glyph-sc->ttf_glyph<8 ) {
		    isbracket = true;
		    offset = bsc->ttf_glyph-sc->ttf_glyph;
		}
	    }
	    if ( !isbracket ) {
		for ( pst=sc->possub; pst!=NULL && PSTHasTag(pst,CHR('r','t','l','a')); pst=pst->next );
		if ( pst!=NULL && pst->type==pst_substitution &&
			(bsc=SFGetChar(sf,-1,pst->u.subs.variant))!=NULL &&
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
    uint16 *props = props_array(sf,&at->gi);
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
    for ( i=0; i<at->gi.gcnt; ++i ) {
	while ( i<at->gi.gcnt && props[i]==0 ) ++i;	/* skip default entries */
	if ( i>=at->gi.gcnt )
    break;
	for ( j=i+1; j<at->gi.gcnt && props[j]==props[i]; ++j );
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
/* *************************    The 'bsln' table    ************************* */
/* ************************************************************************** */

static int BslnFromTag(uint32 tag) {
    switch ( tag ) {
      case CHR('r','o','m','n'):
return( 0 );
    /* Apple has a centered ideographic baseline, while OT has a top ideo bsln*/
    /* no way to get Apple's baseline #1 */
      case CHR('i','d','e','o'):
return( 2 );
      case CHR('h','a','n','g'):
return( 3 );
      case CHR('m','a','t','h'):
return( 4 );
      default:
return( 0xffff );
    }
}

int16 *PerGlyphDefBaseline(SplineFont *sf,int *def_baseline) {
    int16 *baselines = malloc(sf->glyphcnt*sizeof(int16));
    int gid, bsln, i, any;
    SplineChar *sc;
    int counts[32];		/* Apple supports a max of 32 baselines, but only 5 are defined */
    struct Base *base = sf->horiz_base;
    struct basescript *bs;
    int bestbsln, bestcnt;

    memset(counts,0,sizeof(counts));

    for ( gid = 0; gid<sf->glyphcnt; ++gid ) if ( (sc = sf->glyphs[gid])!=NULL ) {
	uint32 script = SCScriptFromUnicode(sc);
	for ( bs= base->scripts; bs!=NULL; bs=bs->next )
	    if ( bs->script==script )
	break;
	if ( bs==NULL )
	    bsln = 0xffff;
	else
	    bsln = BslnFromTag( base->baseline_tags[bs->def_baseline] );
/* This if is duplicated (almost) in basedlg.c:Base_FinishEdit */
	if ( bsln==0xffff ) {
	    if ( script==CHR('k','a','n','a') || script==CHR('h','a','n','g') ||
		    script==CHR('h','a','n','i') || script==CHR('b','o','p','o') ||
		    script==CHR('j','a','m','o') || script==CHR('y','i',' ',' '))
		bsln = 2;
	    else if ( script==CHR('t','i','b','t' ) ||
		    script == CHR('b','e','n','g' ) || script == CHR('b','n','g','2') ||
		    script == CHR('d','e','v','a' ) || script == CHR('d','e','v','2') ||
		    script == CHR('g','u','j','r' ) || script == CHR('g','j','r','2') ||
		    script == CHR('g','u','r','u' ) || script == CHR('g','u','r','2') ||
		    script == CHR('k','n','d','a' ) || script == CHR('k','n','d','2') ||
		    script == CHR('m','l','y','m' ) || script == CHR('m','l','m','2') ||
		    script == CHR('o','r','y','a' ) || script == CHR('o','r','y','2') ||
		    script == CHR('t','a','m','l' ) || script == CHR('t','m','l','2') ||
		    script == CHR('t','e','l','u' ) || script == CHR('t','e','l','2'))
		bsln = 3;
	    else if ( script==CHR('m','a','t','h') )
		bsln = 4;
	    else
		bsln = 0;
	}
	baselines[gid] = bsln;
	if ( bsln!=0xffff )
	    ++counts[bsln];
    }
    
    bestbsln = 0;
    bestcnt = 0;
    any = 0;
    for ( i=0; i<32 ; ++i ) {
	if ( counts[i]>bestcnt ) {
	    bestbsln = i;
	    bestcnt = counts[i];
	    ++any;
	}
    }
    *def_baseline = bestbsln | (any<=1 ? 0x100 : 0 );
return( baselines );
}

void FigureBaseOffsets(SplineFont *sf,int def_bsln,int offsets[32]) {
    struct Base *base = sf->horiz_base;
    struct basescript *bs = base->scripts;
    int i;

    memset( offsets,0xff,32*sizeof(int));
    for ( i=0; i<base->baseline_cnt; ++i ) {
	int bsln = BslnFromTag(base->baseline_tags[i]);
	if ( bsln!=0xffff )
	    offsets[bsln] = bs->baseline_pos[i];
    }
    if ( offsets[def_bsln]!=-1 ) {
	for ( i=0; i<32; ++i ) {
	    if ( offsets[i]!=-1 )
		offsets[i] -= offsets[def_bsln];
	}
    }
    /* I suspect baseline 1 is the standard baseline for CJK glyphs on the mac*/
    /*  (because baseline 2 is often the same as baseline 1, which is wrong for 2) */
    /* OT doesn't have a centered ideographic baseline, so guestimate */
    /* And I don't want to base it on the actual ideo baseline (go up half an em?) */
    /*  because in my small sample of 'bsln' tables baseline 2 has been wrong */
    /*  most of the time, and it is wrong in the example in the docs. */
    /* (I know it is wrong because it has the same value as baseline 1, but */
    /*  is supposed to be below baseline 1 ) */
    if ( offsets[1]==-1 ) {
	if ( offsets[2]!=-1 )
	    offsets[1] = offsets[2]+(sf->ascent+sf->descent)/2;
	else
	    offsets[1] = (sf->ascent+sf->descent)/2 - sf->descent;
    }
    for ( i=0; i<32; ++i )
	if ( offsets[i]==-1 )
	    offsets[i] = 0;
}

void aat_dumpbsln(struct alltabs *at, SplineFont *sf) {
    int def_baseline;
    int offsets[32];
    int16 *baselines;
    int i, gid, j, bsln, cnt;

    if ( sf->horiz_base==NULL || sf->horiz_base->baseline_cnt==0 ||
	    sf->horiz_base->scripts==NULL )
return;

    baselines = PerGlyphDefBaseline(sf,&def_baseline);

    at->bsln = tmpfile();
    putlong(at->bsln,0x00010000);	/* Version */
    if ( def_baseline & 0x100 )		/* Only one baseline in the font */
	putshort(at->bsln,0);		/* distanced based (no control point), no per-glyph info */
    else
	putshort(at->bsln,1);		/* distanced based (no cp info) with per-glyph info */
    putshort(at->bsln,def_baseline&0x1f);/* Default baseline when no info specified for glyphs */

    /* table of 32 int16 (the docs say uint16, but that must be wrong) giving */
    /*  the offset of the nth baseline from the default baseline. */
    /* 0 => Roman, 1=> centered ideo, 2=>low ideo (same as OTF ideo) 3=>hang, 4=>Math */
    /* values 5-31 undefined, set to 0 */
    FigureBaseOffsets(sf,def_baseline&0x1f,offsets);

    for ( i=0; i<32; ++i )
	putshort(at->bsln,offsets[i]);

    if ( !(def_baseline&0x100) ) {
	def_baseline &= 0x1f;

	putshort(at->bsln,2);	/* Lookup format 2, segmented array w/ single value */

	cnt = 0;
	for ( i=0; i<at->gi.gcnt; ++i ) if ( (gid=at->gi.bygid[i])!=-1 ) {
	    if ( baselines[gid]!=-1 && baselines[gid]!=def_baseline ) {
		bsln = baselines[gid];
		for ( j=i; j<at->gi.gcnt && baselines[at->gi.bygid[i]]==bsln; ++j );
		i = j-1;
		++cnt;
	    }
	}

	/* Dump out a binary search header */
	putshort(at->bsln,6);		/* size of each item */
	putshort(at->bsln,cnt);		/* number of items */
	for ( j=1, i=0; cnt<=j; j<<=1, ++i );
	putshort(at->bsln,6*j/2);	/* j is a power of 2 too big */
	putshort(at->bsln,i-1);
	putshort(at->bsln,6*(cnt-(j>>1)) );

	for ( i=0; i<at->gi.gcnt; ++i ) if ( (gid=at->gi.bygid[i])!=-1 ) {
	    if ( baselines[gid]!=-1 && baselines[gid]!=def_baseline ) {
		bsln = baselines[gid];
		for ( j=i; j<at->gi.gcnt && baselines[at->gi.bygid[i]]==bsln; ++j );
		putshort(at->bsln,j-1);
		putshort(at->bsln,i);
		putshort(at->bsln,bsln);
		i = j-1;
	    }
	}

	putshort(at->bsln,0xffff);		/* Final eof marker */
	putshort(at->bsln,0xffff);
	putshort(at->bsln,0x0000);
    }

    at->bslnlen = ftell(at->bsln);
    /* Only contains 2 & 4 byte quantities, can't have an odd number of bytes */
    if ( at->bslnlen&2 )
	putshort(at->bsln,0);
    free(baselines);
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
    *featureType = (tag >> 16);
    *featureSetting = (tag & 0xFFFF);
	/* Ranges taken from Apple Font Registry. An OT tag without a 
    corresponding mac feature should fail this test.*/
    if (*featureType >= 0 && *featureType < 105 && *featureSetting < 16)
        return ( true );

    *featureType = 0;
    *featureSetting = 0;
return( false );
}

static struct feature *featureFromTag(SplineFont *sf, uint32 tag ) {
    int ft, fs;
    struct feature *feat;

    feat = chunkalloc(sizeof(struct feature));
    if (OTTagToMacFeature(tag, &ft, &fs)) {
        feat->featureType = ft;
        feat->featureSetting = fs;
        feat->mf = FindMacFeature(sf,feat->featureType,&feat->smf);
        feat->ms = FindMacSetting(sf,feat->featureType,feat->featureSetting,&feat->sms);
        feat->needsOff = feat->mf!=NULL && !feat->mf->ismutex;
        feat->vertOnly = tag==CHR('v','r','t','2') || tag==CHR('v','k','n','a');    
    }
    
    return( feat );
}

static struct feature *featureFromSubtable(SplineFont *sf, struct lookup_subtable *sub ) {
    FeatureScriptLangList *fl;
    int ft, fs;

    for ( fl=sub->lookup->features; fl!=NULL; fl=fl->next ) {
	if ( fl->ismac )
    break;
    }
    if ( fl==NULL ) {
	for ( fl=sub->lookup->features; fl!=NULL; fl=fl->next ) {
	    if ( OTTagToMacFeature(fl->featuretag,&ft,&fs) )
	break;
	}
	  if ( fl==NULL ) {
	    IError("Could not find a mac feature");
      return NULL;
    }
  }
  return( featureFromTag(sf,fl->featuretag));
}
    
static int PSTHasTag(PST *pst, uint32 tag) {
    FeatureScriptLangList *fl;

    if ( pst->subtable==NULL )
return( false );
    for ( fl=pst->subtable->lookup->features; fl!=NULL; fl=fl->next )
	if ( fl->featuretag == tag )
return( true );

return( false );
}

int scriptsHaveDefault(struct scriptlanglist *sl) {
    int i;

    for ( ; sl!=NULL; sl=sl->next ) {
	for ( i=0; i<sl->lang_cnt; ++i ) {
	    if ( (i<MAX_LANG && sl->langs[i]==DEFAULT_LANG) ||
		    (i>=MAX_LANG && sl->morelangs[i-MAX_LANG]==DEFAULT_LANG)) {
return( true );
	    }
	}
    }
return( false );
}

int LookupHasDefault(OTLookup *otl) {
    FeatureScriptLangList *feats;

    if ( otl->def_lang_checked )
return( otl->def_lang_found );

    otl->def_lang_checked = true;
    for ( feats=otl->features; feats!=NULL; feats = feats->next ) {
	if ( scriptsHaveDefault(feats->scripts) ) {
	    otl->def_lang_found = true;
return( true );
	}
    }
    otl->def_lang_found = false;
return( false );
}
