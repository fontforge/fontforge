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

/* This file contains routines to create some of the Apple Advanced Typography Tables */

/* ************************************************************************** */
/* *************************    The 'acnt' table    ************************* */
/* ************************************************************************** */

void aat_dumpacnt(struct alltabs *at, SplineFont *sf) {
}

/* ************************************************************************** */
/* *************************    The 'kern' table    ************************* */
/* ************************************************************************** */


void ttf_dumpkerns(struct alltabs *at, SplineFont *sf) {
    int i, cnt, j, k, m, threshold;
    KernPair *kp;
    uint16 *glnum, *offsets;

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
    if ( cnt==0 )
return;

    at->kern = tmpfile();
    putshort(at->kern,0);		/* version */
    putshort(at->kern,1);		/* number of subtables */
    putshort(at->kern,0);		/* subtable version */
    putshort(at->kern,(7+3*cnt)*sizeof(uint16)); /* subtable length */
    putshort(at->kern,1);		/* coverage, flags&format */
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

void aat_dumpmorx(struct alltabs *at, SplineFont *sf) {
}

/* ************************************************************************** */
/* *************************    The 'opbd' table    ************************* */
/* ************************************************************************** */


static int haslrbounds(SplineChar *sc, PST **left, PST **right) {
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

void aat_dumpopbd(struct alltabs *at, SplineFont *sf) {
    int i, j, k, l, seg_cnt, tot, last, offset;
    PST *left, *right;
    FILE *opbd=NULL;

    for ( k=0; k<4; ++k ) {
	for ( i=seg_cnt=tot=0; i<sf->charcnt; ++i )
		if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=-1 &&
		    haslrbounds(sf->chars[i],&left,&right) ) {
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
	    offset = 4+7*2 + seg_cnt*6 + 6;
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

static uint16 *props_array(SplineFont *sf,struct alltabs *at) {
    uint16 *props;
    int i;
    SplineChar *sc, *bsc;
    int dir, isfloat, isbracket, offset, doit=false;
    AnchorPoint *ap;
    PST *pst;

    props = gcalloc(at->maxp.numGlyphs,sizeof(uint16));
    for ( i=0; i<sf->charcnt; ++i ) if ( (sc=sf->chars[i])!=NULL && sc->ttf_glyph!=-1 ) {
	dir = 0;
	if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 &&
		isarabnumeric(sc->unicodeenc))
	    dir = 6;
	else if ( sc->script==CHR('a','r','a','b') )
	    dir = 2;
	else if ( sc->script==CHR('h','e','b','r') )
	    dir = 1;
	else if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 ) {
	    if ( iseuronumeric(sc->unicodeenc) )
		dir = 3;
	    else if ( iseuronumsep(sc->unicodeenc))
		dir = 4;
	    else if ( iseuronumterm(sc->unicodeenc))
		dir = 5;
	    else if ( iscommonsep(sc->unicodeenc))
		dir = 7;
	    else if ( isspace(sc->unicodeenc))
		dir = 10;
	    else if ( islefttoright(sc->unicodeenc) )
		dir = 0;
	    else if ( isrighttoleft(sc->unicodeenc) )
		dir = 1;
	    else
		dir = 11;		/* Other neutrals */
	    /* Not dealing with unicode 3 classes */
	    /* nor block seperator/ segment seperator */
	}
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
	    }
	    doit = true;
	}
	if ( sc->script==CHR('a','r','a','b') || sc->script==CHR('h','e','b','r') ) {
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

    if ( !doit ) {
	free(props);
return( NULL );
    }

return( props );
}

void aat_dumpprop(struct alltabs *at, SplineFont *sf) {
    uint16 *props = props_array(sf,at);
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
}
