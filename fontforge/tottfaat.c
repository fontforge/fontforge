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

int SLIHasDefault(SplineFont *sf,int sli) {
    struct script_record *sr;
    int i, j;

    if ( sli==SLI_NESTED || sli==SLI_UNKNOWN )
return( false );

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
    if ( sf->mm!=NULL ) sf = sf->mm->normal;

    sr = sf->script_lang[sli];

    for ( i=0; sr[i].script!=0; ++i )
	for ( j=0; sr[i].langs[j]!=0; ++j )
	    if ( sr[i].langs[j]==DEFAULT_LANG )
return( true );

return( false );
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
	    if ( kp->off!=0 )
		++cnt, ++j;
	if ( j>mh ) mh=j;
	j=0;
	for ( kp = sf->glyphs[at->gi.bygid[i]]->vkerns; kp!=NULL; kp=kp->next )
	    if ( kp->off!=0 )
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
	kcnt->hbreaks = galloc((at->gi.gcnt+1)*sizeof(int));
	cnt = 0;
	for ( i=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
	    j = 0;
	    for ( kp = sf->glyphs[at->gi.bygid[i]]->kerns; kp!=NULL; kp=kp->next )
		if ( kp->off!=0 )
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
	kcnt->vbreaks = galloc((at->gi.gcnt+1)*sizeof(int));
	vcnt = 0;
	for ( i=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
	    j = 0;
	    for ( kp = sf->glyphs[at->gi.bygid[i]]->vkerns; kp!=NULL; kp=kp->next )
		if ( kp->off!=0 )
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
	for ( kc=sf->kerns; kc!=NULL; kc = kc->next ) if ( SLIHasDefault(sf,kc->sli) )
	    ++kccnt;
	for ( kc=sf->vkerns; kc!=NULL; kc = kc->next ) if ( SLIHasDefault(sf,kc->sli) )
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

    if ( CountKerns(at,sf,&kcnt)==0 )
return;

    if ( tupleIndex==-1 ) tupleIndex = 0;
    
    for ( isv=0; isv<2; ++isv ) {
	c = isv ? kcnt.vcnt : kcnt.cnt;
	bmax = isv ? kcnt.vsubs : kcnt.hsubs;
	breaks = isv ? kcnt.vbreaks : kcnt.hbreaks;
	if ( c!=0 ) {
	    km = isv ? kcnt.mv : kcnt.mh;
	    glnum = galloc(km*sizeof(uint16));
	    offsets = galloc(km*sizeof(uint16));
	    gid = 0;
	    for ( b=0; b<bmax; ++b ) {
		c = bmax==1 ? c : breaks[b];
		if ( version==0 ) {
		    putshort(at->kern,0);		/* subtable version */
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

		for ( tot = 0; gid<at->gi.gcnt && tot<c; ++gid ) if ( at->gi.bygid[gid]!=-1 ) {
		    SplineChar *sc = sf->glyphs[at->gi.bygid[gid]];
		    m = 0;
		    for ( kp = isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kp->next ) {
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
			putshort(at->kern,gid);
			putshort(at->kern,glnum[j]);
			putshort(at->kern,offsets[j]);
		    }
		    tot += m;
		}
	    }
	    free(offsets);
	    free(glnum);
	}
    }
    free(kcnt.hbreaks); free(kcnt.vbreaks);

    if ( at->applemode ) for ( isv=0; isv<2; ++isv ) {
	for ( kc=isv ? sf->vkerns : sf->kerns; kc!=NULL; kc=kc->next ) if ( SLIHasDefault(sf,kc->sli) ) {
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
	SFKernPrepare(sf,false);
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
	for ( i=seg_cnt=tot=0; i<at->gi.gcnt; ++i )
		if ( at->gi.bygid[i]!=-1 &&
		    (pst = haslcaret(sf->glyphs[at->gi.bygid[i]]))!=NULL ) {
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
	    for ( j=i+1, ++tot; j<at->gi.gcnt && at->gi.bygid[j]!=-1; ++j ) {
		if ( (pst = haslcaret(sf->glyphs[at->gi.bygid[j]]))== NULL )
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

/* I'm also going to break ligatures up into seperate sub-tables depending on */
/*  script, so again there may be multiple tags */
/* (only the default language will be used) */
struct feature {
    uint32 otftag;
    int16 featureType, featureSetting;
    MacFeat *mf, *smf;
    struct macsetting *ms, *sms;
    unsigned int vertOnly: 1;
    unsigned int r2l: 1;	/* I think this is the "descending" flag */
    unsigned int needsOff: 1;
    unsigned int singleMutex: 1;
    uint8 subtable_type;
    int chain;
    int32 flag, offFlags;
    uint32 feature_start;
    uint32 feature_len;		/* Does not include header yet */
    struct feature *next;
};

static struct feature *featureFromTag(SplineFont *sf, uint32 tag);

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

static int HasDefaultLang(SplineFont *sf,PST *lig,uint32 script) {
    int i, j;
    struct script_record *sr;

    if ( lig->script_lang_index==SLI_NESTED )
return( false );

    if ( sf->cidmaster ) sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;
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

struct tagflag {
    uint32 tag;
    uint32 r2l;
};

static struct feature *aat_dumpmorx_substitutions(struct alltabs *at, SplineFont *sf,
	FILE *temp, struct feature *features) {
    int i, max, cnt, j, k, gcnt;
    struct tagflag *subtags;
    int ft, fs;
    SplineChar *sc, *msc, **glyphs;
    uint16 *maps;
    struct feature *cur;
    PST *pst;

    max = 30; cnt = 0;
    subtags = galloc(max*sizeof(sizeof(struct tagflag)));

    for ( i=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
	sc = sf->glyphs[at->gi.bygid[i]];
	for ( pst=sc->possub; pst!=NULL; pst=pst->next ) if ( pst->type == pst_substitution ) {
	    /* Arabic forms (marked by 'isol') are done with a contextual glyph */
	    /*  substitution subtable (cursive connection) */
	    if ( !HasDefaultLang(sf,pst,0) )
	continue;
	    if ( pst->tag!=CHR('i','s','o','l') &&
		    pst->type!=pst_position &&
		    pst->type!=pst_pair &&
		    pst->type!=pst_lcaret &&
		    (pst->macfeature || OTTagToMacFeature(pst->tag,&ft,&fs))) {
		for ( j=cnt-1; j>=0 &&
			(subtags[j].tag!=pst->tag || subtags[j].r2l!=(pst->flags&pst_r2l)); --j );
		if ( j<0 ) {
		    if ( cnt>=max )
			subtags = grealloc(subtags,(max+=30)*sizeof(uint32));
		    subtags[cnt].tag = pst->tag;
		    subtags[cnt++].r2l = pst->flags & pst_r2l;
		}
	    }
	}
    }

    if ( cnt!=0 ) {
	for ( j=0; j<cnt; ++j ) {
	    for ( k=0; k<2; ++k ) {
		gcnt = 0;
		for ( i=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
		    sc = sf->glyphs[at->gi.bygid[i]];
		    for ( pst=sc->possub; pst!=NULL &&
			    (pst->tag!=subtags[j].tag ||
			     (pst->flags&pst_r2l) != subtags[j].r2l ||
			     pst->type==pst_position || pst->type==pst_pair ||
			     pst->type==pst_lcaret ||
			     !HasDefaultLang(sf,pst,0)); pst=pst->next );
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
	    break;
		    glyphs = galloc((gcnt+1)*sizeof(SplineChar *));
		    maps = galloc((gcnt+1)*sizeof(uint16));
		} else {
		    glyphs[gcnt] = NULL; maps[gcnt] = 0;
		}
	    }
	    if ( gcnt==0 )
	continue;
	    cur = featureFromTag(sf,subtags[j].tag);
	    cur->next = features;
	    cur->r2l = subtags[j].r2l ? true : false;
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

static LigList *LigListMatchOtfTag(SplineFont *sf,LigList *ligs,uint32 tag,
	int r2l) {
    LigList *l;

    for ( l=ligs; l!=NULL; l=l->next )
	if ( l->lig->tag == tag && r2l == (l->lig->flags & pst_r2l) &&
		HasDefaultLang(sf,l->lig,0))
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
    cglyphs = galloc((charcnt+1)*sizeof(SplineChar *));
    map = galloc((charcnt+1)*sizeof(uint16));
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
	FILE *temp, struct feature *features) {
    int i, max, cnt, j, k, gcnt;
    struct tagflag *ligtags;
    int ft, fs;
    SplineChar *sc, *ssc, **glyphs;
    struct feature *cur;
    LigList *l;

    SFLigaturePrepare(sf);

    max = 30; cnt = 0;
    ligtags = galloc(max*sizeof(struct tagflag));

    for ( i=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
	sc = sf->glyphs[at->gi.bygid[i]];
	for ( l=sc->ligofme; l!=NULL; l=l->next ) {
	    if ( HasDefaultLang(sf,l->lig,0) &&
		    (l->lig->macfeature || OTTagToMacFeature(l->lig->tag,&ft,&fs))) {
		for ( j=cnt-1; j>=0 &&
			(ligtags[j].tag!=l->lig->tag || ligtags[j].r2l!=(l->lig->flags&pst_r2l));
			--j );
		if ( j<0 ) {
		    if ( cnt>=max )
			ligtags = grealloc(ligtags,(max+=30)*sizeof(struct tagflag));
		    ligtags[cnt].tag = l->lig->tag;
		    ligtags[cnt++].r2l = l->lig->flags & pst_r2l;
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
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	    sf->glyphs[i]->ticked = false;
	for ( i=0; i<at->gi.gcnt; ++i )
		if ( at->gi.bygid[i]!=-1 && !(sc=sf->glyphs[at->gi.bygid[i]])->ticked &&
			(l = LigListMatchOtfTag(sf,sc->ligofme,ligtags[j].tag,ligtags[j].r2l))!=NULL ) {
	    uint32 script = SCScriptFromUnicode(sc);
	    int ignoremarks = l->lig->flags & pst_ignorecombiningmarks ? 1 : 0 ;
	    for ( k=i, gcnt=0; k<at->gi.gcnt; ++k )
		    if ( at->gi.bygid[k]!=-1 &&
			    (ssc=sf->glyphs[at->gi.bygid[k]])!=NULL && !ssc->ticked &&
			    SCScriptFromUnicode(ssc) == script &&
			    LigListMatchOtfTag(sf,ssc->ligofme,ligtags[j].tag,ligtags[j].r2l)) {
		glyphs[gcnt++] = ssc;
		ssc->ticked = true;
	    }
	    glyphs[gcnt] = NULL;
	    cur = featureFromTag(sf,ligtags[j].tag);
	    cur->next = features;
	    features = cur;
	    cur->subtable_type = 2;		/* ligature */
	    cur->feature_start = ftell(temp);
	    morx_dumpLigaFeature(temp,glyphs,gcnt,ligtags[j].tag,at,sf,ignoremarks);
	    if ( (ftell(temp)-cur->feature_start)&1 )
		putc('\0',temp);
	    if ( (ftell(temp)-cur->feature_start)&2 )
		putshort(temp,0);
	    cur->feature_len = ftell(temp)-cur->feature_start;
	    cur->r2l = ligtags[j].r2l ? true : false;
	}
    }

    free(glyphs);
    free(ligtags);
    SFLigatureCleanup(sf);
return( features);
}

static void morx_dumpnestedsubs(FILE *temp,SplineFont *sf,uint32 tag,struct glyphinfo *gi) {
    int i, j, gcnt;
    PST *pst;
    SplineChar **glyphs, *sc;
    uint16 *map;

    for ( j=0; j<2; ++j ) {
	gcnt = 0;
	for ( i = 0; i<gi->gcnt; ++i ) if ( gi->bygid[i]!=-1 ) {
	    for ( pst=sf->glyphs[gi->bygid[i]]->possub;
		    pst!=NULL && (pst->tag!=tag || pst->script_lang_index!=SLI_NESTED);
		    pst=pst->next );
	    if ( pst!=NULL && pst->type==pst_substitution &&
		    (sc=SFGetChar(sf,-1,pst->u.subs.variant))!=NULL &&
		    sc->ttf_glyph!=-1 ) {
		if ( j ) {
		    glyphs[gcnt] = sf->glyphs[i];
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

static uint16 *NamesToGlyphs(SplineFont *sf,char *names,uint16 *cnt) {
    char *pt, *start;
    int c, ch;
    uint16 *ret;
    SplineChar *sc;

    for ( c=0, pt=names; *pt; ++pt )
	if ( *pt==' ' ) ++c;
    ret = galloc((c+1)*sizeof(uint16));

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
    uint32 *substags=NULL;
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
    glyphs = galloc((gcnt+1)*sizeof(SplineChar *));
    map = galloc((gcnt+1)*sizeof(uint16));
    gcnt = 0;
    for ( i=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 && sf->glyphs[at->gi.bygid[i]]->lsidebearing!=1 ) {
	glyphs[gcnt] = sf->glyphs[at->gi.bygid[i]];
	map[gcnt++] = sf->glyphs[at->gi.bygid[i]]->lsidebearing;
    }
    glyphs[gcnt] = NULL;

    /* Give each subs tab an index into the mac's substitution lookups */
    transdata = gcalloc(sm->state_cnt*sm->class_cnt,sizeof(struct transdata));
    stcnt = 0;
    substags = NULL; subsins = NULL;
    if ( sm->type==asm_context ) {
	substags = galloc(2*sm->state_cnt*sm->class_cnt*sizeof(uint32));
	for ( j=0; j<sm->state_cnt*sm->class_cnt; ++j ) {
	    struct asm_state *this = &sm->state[j];
	    transdata[j].mark_index = transdata[j].cur_index = 0xffff;
	    if ( this->u.context.mark_tag!=0 ) {
		for ( i=0; i<stcnt; ++i )
		    if ( substags[i]==this->u.context.mark_tag )
		break;
		if ( i==stcnt )
		    substags[stcnt++] = this->u.context.mark_tag;
		transdata[j].mark_index = i;
	    }
	    if ( this->u.context.cur_tag!=0 ) {
		for ( i=0; i<stcnt; ++i )
		    if ( substags[i]==this->u.context.cur_tag )
		break;
		if ( i==stcnt )
		    substags[stcnt++] = this->u.context.cur_tag;
		transdata[j].cur_index = i;
	    }
	}
    } else if ( sm->type==asm_insert ) {
	subsins = galloc(2*sm->state_cnt*sm->class_cnt*sizeof(struct ins));
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

    trans = galloc(sm->state_cnt*sm->class_cnt*sizeof(struct trans));
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
	    morx_dumpnestedsubs(temp,sf,substags[i],&at->gi);
	}
	free(substags);
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
	FILE *temp, struct feature *features) {
    ASM *sm;
    struct feature *cur;

    for ( sm = sf->sm; sm!=NULL; sm=sm->next ) if ( sm->type!=asm_kern ) {
	cur = chunkalloc(sizeof(struct feature));
	if ( sm->opentype_tag==0 )
	    cur->otftag = (sm->feature<<16)|sm->setting;
	else
	    cur->otftag = sm->opentype_tag;
	cur->featureType = sm->feature;
	cur->featureSetting = sm->setting;
	cur->mf = FindMacFeature(sf,cur->featureType,&cur->smf);
	cur->ms = FindMacSetting(sf,cur->featureType,cur->featureSetting,&cur->sms);
	cur->needsOff = cur->mf!=NULL && !cur->mf->ismutex;
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
    }
return( features);
}

static struct feature *aat_dumpmorx_cvtopentype(struct alltabs *at, SplineFont *sf,
	FILE *temp, struct feature *features) {
    FPST *fpst;
    ASM *sm, *last=NULL;
    int genbase;
    struct sliflag *sliflags;
    int featureType, featureSetting;
    int i;

    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;
    genbase = sf->gentags.tt_cur;

    for ( fpst=sf->possub; fpst!=NULL; fpst=fpst->next ) {
	if ( FPSTisMacable(sf,fpst,true)) {
	    sm = ASMFromFPST(sf,fpst,true);
	    if ( sm!=NULL ) {
		OTTagToMacFeature(fpst->tag,&featureType,&featureSetting);
		sm->feature = featureType; sm->setting = featureSetting;
		if ( last==NULL )
		    sf->sm = sm;
		else
		    last->next = sm;
		last = sm;
	    }
	}
    }

    sliflags = NULL;
    if ( OTTagToMacFeature(CHR('i','s','o','l'),&featureType,&featureSetting) ) {
	sliflags = SFGetFormsList(sf,true);
	if ( sliflags!=NULL ) {
	    for ( i=0; sliflags[i].sli!=0xffff; ++i ) {
		sm = ASMFromOpenTypeForms(sf,sliflags[i].sli,sliflags[i].flags);
		if ( sm!=NULL ) {
		    sm->feature = featureType; sm->setting = featureSetting;
		    if ( last==NULL )
			sf->sm = sm;
		    else
			last->next = sm;
		    last = sm;
		}
	    }
	}
    }

    features = aat_dumpmorx_asm(at,sf,temp,features);
    ASMFree(sf->sm); sf->sm = NULL;
    RemoveGeneratedTagsAbove(sf,genbase);
    free(sliflags);
return( features );
}

static struct feature *featuresOrderByType(struct feature *features) {
    struct feature *f, **all;
    int i, j, cnt, saw_default;

    for ( cnt=0, f=features; f!=NULL; f=f->next, ++cnt );
    if ( cnt==1 ) {
	if ( !features->needsOff )
	    features->singleMutex = true;
return( features );
    }
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
	if ( all[i]->mf!=NULL && all[i]->mf->ismutex && !all[i]->needsOff ) {
	    /* mutexes with just 2 choices don't always follow the rule that 0 is off */
	    if (i==cnt-1 || all[i]->featureType!=all[i+1]->featureType )
		all[i]->singleMutex = true;
	    saw_default = false;
	    j=i;
	    while ( i<cnt-1 && all[i]->featureType==all[j]->featureType ) {
		if ( all[i]->featureSetting==0 ) saw_default = true;
		++i;
	    }
	    if ( i!=j ) --i;
	    if ( !saw_default )
		all[i]->singleMutex = true;
	} else {
	    j=i;
	    while ( i<cnt-1 && all[i]->featureType==all[j]->featureType )
		++i;
	    if ( i!=j ) --i;
	}
    }
    for ( i=0; i<cnt-1; ++i )
	all[i]->next = all[i+1];
    all[cnt-1]->next = NULL;
    features = all[0];
    free( all );
return( features );
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
    /*feature = reversefeatures(feature);*/

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
		for ( n=f->next; n!=NULL && n->featureType==f->featureType; n = n->next ) {
		    if ( p->featureSetting!=n->featureSetting ) {
			++cnt;
			p = n;
		    }
		}
	    } else {
		p = f;
		for ( n=f; n!=NULL && n->featureType==f->featureType; n = n->next ) {
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
	    at->feat_name = galloc((fcnt+scnt+1)*sizeof(struct feat_name));
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

static int featuresAssignFlagsChains(struct feature *feature) {
    int bit=0, cnt, chain = 0;
    struct feature *f, *n, *p;

    if ( feature==NULL )
return( 0 );

    for ( f=feature; f!=NULL; f=n ) {
	cnt=0;
	p = f;
	for ( n=f; n!=NULL && n->featureType==f->featureType; n=n->next ) {
	    if ( n->featureSetting != p->featureSetting ) {
		++cnt;
		p = n;
	    }
	}
	if ( bit+cnt>=32 ) {
	    ++chain;
	    bit = 0;
	}
	for ( n=f; n!=NULL && n->featureType==f->featureType; ) {
	    p = n;
	    while ( n!=NULL && n->featureType==p->featureType &&
		    n->featureSetting == p->featureSetting ) {
		n->flag = 1<<bit;
		n->chain = chain;
		n = n->next;
	    }
	    ++bit;
	}
    }
    for ( f=feature; f!=NULL; f=n ) {
	if ( f->mf!=NULL && f->mf->ismutex ) {
	    int offFlags = 0;
	    for ( n=f; n!=NULL && f->featureType==n->featureType; n=n->next )
		offFlags |= n->flag;
	    for ( n=f; n!=NULL && f->featureType==n->featureType; n=n->next )
		n->offFlags = offFlags;
	} else if ( f->next!=NULL && (f->featureSetting&1)==0 &&
		f->next->featureType==f->featureType &&
		f->next->featureSetting==f->featureSetting+1 ) {
	    n = f->next;
	    f->offFlags = n->flag;
	    n->offFlags = f->flag;
	    n = n->next;
	} else {
	    f->offFlags = 0;
	    n = f->next;
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
		if ( n->ms!=NULL && n->ms->initially_enabled )
		    def_flags |= n->flag;
		++scnt;
		if ( n->needsOff ) {
		    ++scnt;
		    if ( n->next!=NULL && n->next->featureSetting==n->featureSetting+1 )
			n = n->next;
		} else if ( n->singleMutex )
		    ++scnt;
	    }
	} else
	    n = f->next;
    }

    if ( cnt>32 ) IError("Too many features in chain");
    for ( i=0; i<cnt; ++i ) for ( j=i+1; j<cnt; ++j ) {
	if ( TTFFeatureIndex(all[i]->otftag,ord,true)>TTFFeatureIndex(all[j]->otftag,ord,true)) {
	    ftemp = all[i];
	    all[i] = all[j];
	    all[j] = ftemp;
	}
    }

    /* Chain header */
    chain_start = ftell(at->morx);
    putlong(at->morx,def_flags);
    putlong(at->morx,0);		/* Fix up length later */
    putlong(at->morx,scnt+2);		/* extra for All Typo features */
    putlong(at->morx,subcnt);		/* subtable cnt */

    /* Features */
    for ( f=features; f!=NULL; f=n ) {
	if ( f->chain==chain ) {
	    for ( n=f; n!=NULL && n->featureType==f->featureType; n=n->next ) {
		putshort(at->morx,n->featureType);
		putshort(at->morx,n->featureSetting);
		putlong(at->morx,n->flag);
		putlong(at->morx,n->flag | ~n->offFlags);
		if ( n->needsOff ) {
		    putshort(at->morx,n->featureType);
		    putshort(at->morx,n->featureSetting+1);
		} else if ( n->singleMutex ) {		    
		    putshort(at->morx,n->featureType);
		    putshort(at->morx,n->featureSetting==0?1:0);
		}
		putlong(at->morx,0);
		putlong(at->morx,~n->offFlags & ~n->flag );
		if ( n->needsOff && n->next!=NULL && n->featureSetting+1==n->next->featureSetting )
		    n = n->next;
	    }
	} else
	    n = f->next;
    }
    putshort(at->morx,0);		/* All Typo Features */
    putshort(at->morx,0);		/* All Features */
    putlong(at->morx,0xffffffff);	/* enable */
    putlong(at->morx,0xffffffff);	/* disable */
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
    fseek(at->morx,0,SEEK_END);
}

void aat_dumpmorx(struct alltabs *at, SplineFont *sf) {
    FILE *temp = tmpfile();
    struct feature *features = NULL;
    int nchains, i;
    struct table_ordering *ord;

    features = aat_dumpmorx_substitutions(at,sf,temp,features);
    features = aat_dumpmorx_ligatures(at,sf,temp,features);
    if ( sf->sm!=NULL )
	features = aat_dumpmorx_asm(at,sf,temp,features);
    else
	features = aat_dumpmorx_cvtopentype(at,sf,temp,features);
    if ( features==NULL ) {
	fclose(temp);
return;
    }
    features = featuresOrderByType(features);
    aat_dumpfeat(at, sf, features);
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
		    putshort(opbd,left!=NULL?left->u.pos.xoff:0);
		    putshort(opbd,0);		/* top */
		    putshort(opbd,right!=NULL?-right->u.pos.h_adv_off:0);
		    putshort(opbd,0);		/* bottom */
		}
		for ( j=i+1, ++tot; j<at->gi.gcnt; ++j ) {
		    if ( at->gi.bygid[i]==-1 || !haslrbounds(_sf->glyphs[at->gi.bygid[j]],&left,&right) )
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

uint16 *props_array(SplineFont *sf,struct glyphinfo *gi) {
    uint16 *props;
    int i;
    SplineChar *sc, *bsc;
    int dir, isfloat, isbracket, offset, doit=false;
    AnchorPoint *ap;
    PST *pst;
    int l, p;

    props = gcalloc(gi->gcnt+1,sizeof(uint16));
    props[gi->gcnt] = -1;

    for ( i=0; i<gi->gcnt; ++i ) if ( (p = gi->bygid==NULL ? i : gi->bygid[i])!=-1 ) {
	l = 0;
	sc = sf->glyphs[p];
	if ( sc!=NULL && (gi->bygid==NULL || sc->ttf_glyph!=-1 )) {
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
		bsc = SFGetChar(sf,tomirror(sc->unicodeenc),NULL);
		if ( bsc!=NULL && bsc->ttf_glyph-sc->ttf_glyph>-8 && bsc->ttf_glyph-sc->ttf_glyph<8 ) {
		    isbracket = true;
		    offset = bsc->ttf_glyph-sc->ttf_glyph;
		}
	    }
	    if ( !isbracket ) {
		for ( pst=sc->possub; pst!=NULL && pst->tag!=CHR('r','t','l','a'); pst=pst->next );
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

static struct feature *featureFromTag(SplineFont *sf, uint32 tag ) {
    int i;
    struct feature *feat;
    struct macsettingname *msn = user_macfeat_otftag ? user_macfeat_otftag : macfeat_otftag;

    for ( i=0; msn[i].otf_tag!=0; ++i )
	if ( msn[i].otf_tag == tag )
    break;

    feat = chunkalloc(sizeof(struct feature));
    feat->otftag = tag;
    if ( msn[i].otf_tag!=0 ) {
	feat->featureType = msn[i].mac_feature_type;
	feat->featureSetting = msn[i].mac_feature_setting;
    } else {
	feat->featureType = tag>>16;
	feat->featureSetting = tag & 0xffff;
    }
    feat->mf = FindMacFeature(sf,feat->featureType,&feat->smf);
    feat->ms = FindMacSetting(sf,feat->featureType,feat->featureSetting,&feat->sms);
    feat->needsOff = feat->mf!=NULL && !feat->mf->ismutex;
    feat->vertOnly = tag==CHR('v','r','t','2') || tag==CHR('v','k','n','a');
return( feat );
}
