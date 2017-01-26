/* -*- coding: utf-8 -*- */
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
#include "fontforgeui.h"
#include "fvcomposite.h"
#include "fvfonts.h"
#include "lookups.h"
#include "psfont.h"
#include "splinefill.h"
#include "splineutil.h"
#include <ustring.h>
#include <gkeysym.h>
#include <utype.h>
#include <unistd.h>


GTextInfo sizes[] = {
    { (unichar_t *) "24", NULL, 0, 0, (void *) 24, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "36", NULL, 0, 0, (void *) 36, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "48", NULL, 0, 0, (void *) 48, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "72", NULL, 0, 0, (void *) 72, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "96", NULL, 0, 0, (void *) 96, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "200", NULL, 0, 0, (void *) 200, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
enum sortby { sb_first, sb_second, sb_kern };
GTextInfo sortby[] = {
    { (unichar_t *) N_("First Char"), NULL, 0, 0, (void *) sb_first, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Second Char"), NULL, 0, 0, (void *) sb_second, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Kern Size"), NULL, 0, 0, (void *) sb_kern, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};

void SFShowLigatures(SplineFont *sf,SplineChar *searchfor) {
    int i, cnt;
    char **choices=NULL;
    int *where=NULL;
    SplineChar *sc, *sc2;
    char *pt, *line;
    char *start, *end, ch;
    PST *pst;

    while ( 1 ) {
	for ( i=cnt=0; i<sf->glyphcnt; ++i ) {
	    if ( (sc=sf->glyphs[i])!=NULL && SCDrawsSomething(sc) ) {
		for ( pst=sc->possub; pst!=NULL; pst=pst->next )
			if ( pst->type==pst_ligature &&
				(searchfor==NULL || PSTContains(pst->u.lig.components,searchfor->name))) {
		    if ( choices!=NULL ) {
			line = pt = malloc((strlen(sc->name)+13+3*strlen(pst->u.lig.components)));
			strcpy(pt,sc->name);
			pt += strlen(pt);
			if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 ) {
			    *pt++='(';
			    pt = utf8_idpb(pt,sc->unicodeenc,0);
			    *pt++=')';
			}
			/* *pt++ = 0x21d0;*/ /* left arrow */
			strcpy(pt," ⇐ "); pt += strlen(pt);
			for ( start= pst->u.lig.components; ; start=end ) {
			    while ( *start==' ' ) ++start;
			    if ( *start=='\0' )
			break;
			    for ( end=start+1; *end!='\0' && *end!=' '; ++end );
			    ch = *end;
			    *end = '\0';
			    strcpy( pt,start );
			    pt += strlen(pt);
			    sc2 = SFGetChar(sf,-1,start);
			    *end = ch;
			    if ( sc2!=NULL && sc2->unicodeenc!=-1 && sc2->unicodeenc<0x10000 ) {
				*pt++='(';
				*pt++ = sc2->unicodeenc;
				*pt++=')';
			    }
			    *pt++ = ' ';
			}
			pt[-1] = '\0';
			choices[cnt] = line;
			where[cnt] = i;
		    }
		    ++cnt;
		}
	    }
	}
	if ( choices!=NULL )
    break;
	choices = malloc((cnt+2)*sizeof(unichar_t *));
	where = malloc((cnt+1)*sizeof(int));
	if ( cnt==0 ) {
	    choices[0] = copy("<No Ligatures>");
	    where[0] = -1;
	    choices[1] = NULL;
    break;
	}
    }
    choices[cnt] = NULL;
    i = gwwv_choose(_("Ligatures"),(const char **) choices,cnt,0,_("Select a ligature to view"));
    if ( i!=-1 && where[i]!=-1 )
	CharViewCreate(sf->glyphs[where[i]],(FontView *) sf->fv,-1);
    free(where);
    for ( i=0; i<cnt; ++i )
	free(choices[i]);
    free(choices);
}

struct kerns {
    SplineChar *first;
    SplineChar *second;
    int newoff, newyoff;
    unsigned int r2l: 1;
    KernPair *kp;
    AnchorClass *ac;
};

typedef struct kpdata {
    GWindow gw,v;
    int vwidth;
    SplineFont *sf;
    SplineChar *sc;		/* If set then restrict to kerns of this char */
				/*  in either position */
    AnchorClass *ac;		/* If set then don't do kerns, but look for */
				/*  anchor combos with this class. If -1 */
			        /*  then all anchor combos with any class */
    struct kerns *kerns;	/* All the kerns we care about */
    int layer;
    int kcnt, firstcnt;
    BDFFont *bdf;
    int header_height;
    int sb_width;
    GFont *font;
    int fh, as;
    int uh, wh, off_top, selected, last_index, vpad;
    int pressed_x, old_val;
    unsigned int done:1;
    unsigned int first:1;
    unsigned int pressed:1;
    unsigned int movecursor:1;
} KPData;

#define CID_Size	1001
#define CID_SortBy	1002
#define CID_ScrollBar	1003
#define CID_OK		1004
#define CID_Cancel	1005

static int firstcmpr(const void *_k1, const void *_k2) {
    const struct kerns *k1 = _k1, *k2 = _k2;

    if ( k1->first==k2->first )		/* If same first char, use second char as tie breaker */
return( k1->second->unicodeenc-k2->second->unicodeenc );

return( k1->first->unicodeenc-k2->first->unicodeenc );
}

static int secondcmpr(const void *_k1, const void *_k2) {
    const struct kerns *k1 = _k1, *k2 = _k2;

    if ( k1->second==k2->second )		/* If same second char, use first char as tie breaker */
return( k1->first->unicodeenc-k2->first->unicodeenc );

return( k1->second->unicodeenc-k2->second->unicodeenc );
}

static int offcmpr(const void *_k1, const void *_k2) {
    const struct kerns *k1 = _k1, *k2 = _k2;
    int off1, off2;

    if ( (off1=k1->newoff)<0 ) off1 = -off1;
    if ( (off2=k2->newoff)<0 ) off2 = -off2;

    if ( off1!=off2 )		/* If same offset, use first char as tie breaker */
return( off1-off2 );

    if ( k1->first!=k2->first )		/* If same first char, use second char as tie breaker */
return( k1->first->unicodeenc-k2->first->unicodeenc );

return( k1->second->unicodeenc-k2->second->unicodeenc );
}

static void KPSortEm(KPData *kpd,enum sortby sort_func) {
    int oldenc;

    if ( sort_func==sb_first || sort_func==sb_second ) {
	if ( kpd->sc!=NULL ) {
	    oldenc = kpd->sc->unicodeenc;
	    kpd->sc->unicodeenc = -1;
	}
	qsort(kpd->kerns,kpd->kcnt,sizeof(struct kerns),
		sort_func==sb_first ? firstcmpr : secondcmpr );
	if ( kpd->sc!=NULL )
	    kpd->sc->unicodeenc = oldenc;
    } else
	qsort(kpd->kerns,kpd->kcnt,sizeof(struct kerns), offcmpr );

    if ( sort_func==sb_first ) {
	int cnt=1, i;
	for ( i=1; i<kpd->kcnt; ++i ) {
	    if ( kpd->kerns[i].first!=kpd->kerns[i-1].first )
		++cnt;
	}
	kpd->firstcnt = cnt;
    }
}

static void CheckLeftRight(struct kerns *k) {
    /* flag any hebrew/arabic entries */

    /* Figure that there won't be any mixed orientation kerns (no latin "A" with hebrew "Alef" kern) */
    /*  but there might be some hebrew/arabic ligatures or something that */
    /*  we don't recognize as right-to-left (ie. not in unicode) */
    if ( SCRightToLeft(k->first) || SCRightToLeft(k->second) )
	k->r2l = true;
    else 
        k->r2l = false;
}

static void KPBuildKernList(KPData *kpd) {
    int i, cnt;
    KernPair *kp;

    if ( kpd->sc!=NULL ) {
	while ( 1 ) {
	    for ( cnt=0, kp=kpd->sc->kerns; kp!=NULL; kp=kp->next ) {
		if ( kpd->kerns!=NULL ) {
		    kpd->kerns[cnt].first = kpd->sc;
		    kpd->kerns[cnt].second = kp->sc;
		    kpd->kerns[cnt].newoff = kp->off;
		    kpd->kerns[cnt].newyoff = 0;
		    kpd->kerns[cnt].kp = kp;
		    kpd->kerns[cnt].ac = NULL;
		    CheckLeftRight(&kpd->kerns[cnt]);
		}
		++cnt;
	    }
	    for ( i=0; i<kpd->sf->glyphcnt; ++i ) if ( kpd->sf->glyphs[i]!=NULL ) {
		for ( kp = kpd->sf->glyphs[i]->kerns; kp!=NULL; kp=kp->next ) {
		    if ( kp->sc == kpd->sc ) {
			if ( kpd->kerns!=NULL ) {
			    kpd->kerns[cnt].first = kpd->sf->glyphs[i];
			    kpd->kerns[cnt].second = kp->sc;
			    kpd->kerns[cnt].newoff = kp->off;
			    kpd->kerns[cnt].newyoff = 0;
			    kpd->kerns[cnt].kp = kp;
			    kpd->kerns[cnt].ac = NULL;
			    CheckLeftRight(&kpd->kerns[cnt]);
			}
			++cnt;
		break;
		    }
		}
	    }
	    if ( kpd->kerns!=NULL )
	break;
	    if ( cnt==0 )
return;
	    kpd->kerns = calloc(cnt+1, sizeof(struct kerns));
	    kpd->kcnt = cnt;
	}
    } else {
	while ( 1 ) {
	    for ( cnt=i=0; i<kpd->sf->glyphcnt; ++i ) if ( kpd->sf->glyphs[i]!=NULL ) {
		for ( kp = kpd->sf->glyphs[i]->kerns; kp!=NULL; kp=kp->next ) {
		    if ( kpd->kerns!=NULL ) {
			kpd->kerns[cnt].first = kpd->sf->glyphs[i];
			kpd->kerns[cnt].second = kp->sc;
			kpd->kerns[cnt].newoff = kp->off;
			kpd->kerns[cnt].newyoff = 0;
			kpd->kerns[cnt].kp = kp;
			kpd->kerns[cnt].ac = NULL;
			CheckLeftRight(&kpd->kerns[cnt]);
		    }
		    ++cnt;
		}
	    }
	    if ( kpd->kerns!=NULL )
	break;
	    if ( cnt==0 )
return;
	    kpd->kerns = calloc(cnt+1, sizeof(struct kerns));
	    kpd->kcnt = cnt;
	}
    }
    KPSortEm(kpd,sb_first);
}

static void AnchorRefigure(KPData *kpd) {
    AnchorPoint *ap1, *ap2;
    DBounds bb;
    int i;

    for ( i=0; i<kpd->kcnt; ++i ) {
	struct kerns *k= &kpd->kerns[i];
	for ( ap1=k->first->anchor; ap1!=NULL && ap1->anchor!=k->ac; ap1=ap1->next );
	for ( ap2=k->second->anchor; ap2!=NULL && ap2->anchor!=k->ac; ap2=ap2->next );
	if ( ap1!=NULL && ap2!=NULL ) {
	    if ( k->r2l ) {
		SplineCharQuickBounds(k->second,&bb);
		k->newoff = k->second->width-ap1->me.x + ap2->me.x;
	    } else
		k->newoff = -k->first->width+ap1->me.x-ap2->me.x;
	    k->newyoff = ap1->me.y-ap2->me.y;
	}
    }
}

static void KPBuildAnchorList(KPData *kpd) {
    int i, j, cnt;
    AnchorClass *ac;
    AnchorPoint *ap1, *ap2, *temp;
    SplineFont *sf = kpd->sf;
    DBounds bb;

    if ( kpd->sc!=NULL ) {
	while ( 1 ) {
	    cnt = 0;
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
		if ( (ac = AnchorClassMatch(kpd->sc,sf->glyphs[i],kpd->ac,&ap1,&ap2)) ||
			(ac = AnchorClassMatch(sf->glyphs[i],kpd->sc,kpd->ac,&ap1,&ap2)) ) {
		    if ( kpd->kerns!=NULL ) {
			struct kerns *k = &kpd->kerns[cnt];
			switch ( ap1->type ) {
			  case at_cexit: case at_basechar: case at_baselig: case at_basemark:
			    k->first = kpd->sc;
			    k->second = sf->glyphs[i];
			  break;
			  case at_centry: case at_mark:
			    k->first = sf->glyphs[i];
			    k->second = kpd->sc;
			    temp = ap1; ap1=ap2; ap2=temp;
			  break;
			}
			CheckLeftRight(k);
			if ( k->r2l ) {
			    SplineCharQuickBounds(k->second,&bb);
			    k->newoff = k->second->width-ap1->me.x + ap2->me.x;
			} else
			    k->newoff = -k->first->width+ap1->me.x-ap2->me.x;
			k->newyoff = ap1->me.y-ap2->me.y;
			k->ac = ac;
			k->kp = NULL;
		    }
		    ++cnt;
		}
	    }
	    if ( kpd->kerns!=NULL )
	break;
	    if ( cnt==0 )
return;
	    kpd->kerns = malloc((cnt+1)*sizeof(struct kerns));
	    kpd->kcnt = cnt;
	}
    } else {
	while ( 1 ) {
	    cnt = 0;
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL && sf->glyphs[i]->anchor ) {
		if ( kpd->ac!=(AnchorClass *) -1 ) {
		    for ( temp = sf->glyphs[i]->anchor; temp!=NULL && temp->anchor!=kpd->ac; temp=temp->next );
		    if ( temp==NULL )
	    continue;
		}
		for ( j=0; j<sf->glyphcnt; ++j ) if ( sf->glyphs[j]!=NULL ) {
		    if ( (ac = AnchorClassMatch(sf->glyphs[i],sf->glyphs[j],kpd->ac,&ap1,&ap2)) ) {
			if ( kpd->kerns!=NULL ) {
			    struct kerns *k = &kpd->kerns[cnt];
			    k->first = sf->glyphs[i];
			    k->second = sf->glyphs[j];
			    CheckLeftRight(k);
			    if ( k->r2l ) {
				SplineCharQuickBounds(k->second,&bb);
				k->newoff = k->second->width-ap1->me.x + ap2->me.x;
			    } else
				k->newoff = -k->first->width+ap1->me.x-ap2->me.x;
			    k->newyoff = ap1->me.y-ap2->me.y;
			    k->ac = ac;
			    k->kp = NULL;
			}
			++cnt;
		    }
		}
	    }
	    if ( kpd->kerns!=NULL )
	break;
	    if ( cnt==0 )
return;
	    kpd->kerns = malloc((cnt+1)*sizeof(struct kerns));
	    kpd->kcnt = cnt;
	}
    }
    KPSortEm(kpd,sb_first);
    AnchorRefigure(kpd);
}

static void KPScrollTo(KPData *kpd, unichar_t uch, enum sortby sort) {
    int i;

    if ( sort==sb_first ) {
	for ( i=0; i<kpd->kcnt && kpd->kerns[i].first->unicodeenc<uch; ++i );
    } else {
	for ( i=0; i<kpd->kcnt && kpd->kerns[i].second->unicodeenc<uch; ++i );
    }
    if ( kpd->wh<=2 )
	/* As is */;
    else if ( kpd->wh<5 )
	--i;
    else
	i -= kpd->wh/5;

    if ( i>kpd->kcnt-kpd->wh )
	i = kpd->kcnt-kpd->wh;
    if ( i<0 )
	i = 0;
    if ( i!=kpd->off_top ) {
	int off = i-kpd->off_top;
	kpd->off_top = i;
	GScrollBarSetPos(GWidgetGetControl(kpd->gw,CID_ScrollBar),kpd->off_top);
	GDrawScroll(kpd->v,NULL,0,off*kpd->uh);
    }
}

static void BaseFillFromBDFC(struct _GImage *base,BDFChar *bdfc) {
    base->data = bdfc->bitmap;
    base->bytes_per_line = bdfc->bytes_per_line;
    base->width = bdfc->xmax-bdfc->xmin+1;
    base->height = bdfc->ymax-bdfc->ymin+1;
}

static void KP_ExposeKerns(KPData *kpd,GWindow pixmap,GRect *rect) {
    GRect old, subclip, subold, sel;
    struct _GImage base;
    GImage gi;
    int index1, index2;
    BDFChar *bdfc1, *bdfc2;
    int i, as, x, em = kpd->sf->ascent+kpd->sf->descent, yoff;
    int first, last;
    struct kerns *kern;
    char buffer[140];

    first = rect->y/kpd->uh;
    last = (rect->y+rect->height+kpd->uh-1)/kpd->uh;

    for ( i=first; i<=last && i+kpd->off_top<kpd->kcnt; ++i ) {
	kern = &kpd->kerns[i+kpd->off_top];
	index1 = kern->first->orig_pos;
	if ( kpd->bdf->glyphs[index1]==NULL )
	    BDFPieceMeal(kpd->bdf,index1);
	index2 = kern->second->orig_pos;
	if ( kpd->bdf->glyphs[index2]==NULL )
	    BDFPieceMeal(kpd->bdf,index2);
    }

    as = kpd->vpad + kpd->sf->ascent * kpd->bdf->pixelsize / em;

    memset(&gi,'\0',sizeof(gi));
    memset(&base,'\0',sizeof(base));
    gi.u.image = &base;
    base.image_type = it_index;
    base.clut = kpd->bdf->clut;
    GDrawSetDither(NULL, false);

    GDrawPushClip(pixmap,rect,&old);
    GDrawSetFont(pixmap,kpd->font);
    GDrawSetLineWidth(pixmap,0);
    GDrawFillRect(pixmap,rect,GDrawGetDefaultBackground(NULL));
    subclip = *rect;
    for ( i=first; i<=last && i+kpd->off_top<kpd->kcnt; ++i ) {
	subclip.y = i*kpd->uh; subclip.height = kpd->uh;
	GDrawPushClip(pixmap,&subclip,&subold);

	kern = &kpd->kerns[i+kpd->off_top];
	index1 = kern->first->orig_pos;
	index2 = kern->second->orig_pos;
	bdfc1 = kpd->bdf->glyphs[index1];
	bdfc2 = kpd->bdf->glyphs[index2];

	BaseFillFromBDFC(&base,bdfc1);
	base.trans = base.clut->trans_index = -1;
	/* the peculiar behavior concerning xmin/xmax is because the bitmaps */
	/*  don't contain the side-bearings, we have to add that spacing manually */
	if ( !kern->r2l ) {
	    GDrawDrawImage(pixmap,&gi,NULL, 10,subclip.y+as-bdfc1->ymax);
	    x = 10 + (bdfc1->width-bdfc1->xmin) + bdfc2->xmin +
		    (kern->newoff*kpd->bdf->pixelsize/em);
	} else {
	    x = kpd->vwidth-10-(bdfc1->xmax-bdfc1->xmin);
	    GDrawDrawImage(pixmap,&gi,NULL, x,subclip.y+as-bdfc1->ymax);
	    x -= bdfc1->xmin + (bdfc2->width-bdfc2->xmin) +
		    (kern->newoff*kpd->bdf->pixelsize/em);
	}
	BaseFillFromBDFC(&base,bdfc2);
#ifndef _BrokenBitmapImages
	base.trans = base.clut->trans_index = 0;
#endif
	yoff = (kern->newyoff*kpd->bdf->pixelsize/em);
	GDrawDrawImage(pixmap,&gi,NULL, x,subclip.y+as-bdfc2->ymax-yoff);
	GDrawDrawLine(pixmap,0,subclip.y+kpd->uh-1,
		subclip.x+subclip.width,subclip.y+kpd->uh-1,0x000000);
	if ( kern->kp!=NULL )
	    sprintf( buffer, "%d ", kern->newoff);
	else
	    sprintf( buffer, "%d,%d ", kern->newoff, kern->newyoff );
	if ( kern->ac!=NULL )
	    strncat(buffer,kern->ac->name,sizeof(buffer)-strlen(buffer)-1);
	GDrawDrawText8(pixmap,15,subclip.y+kpd->uh-kpd->fh+kpd->as,buffer,-1,
		kern->kp!=NULL && kern->newoff!=kern->kp->off ? 0xff0000 : 0x000000 );
	if ( i+kpd->off_top==kpd->selected ) {
	    sel.x = 0; sel.width = kpd->vwidth-1;
	    sel.y = subclip.y; sel.height = kpd->uh-2;
	    GDrawDrawRect(pixmap,&sel,0x000000);
	}
	GDrawPopClip(pixmap,&subold);
    }
#ifndef _BrokenBitmapImages
    base.clut->trans_index = -1;
#endif
    GDrawPopClip(pixmap,&old);
    GDrawSetDither(NULL, true);
}

static void KP_RefreshSel(KPData *kpd,int index) {
    Color col = index==kpd->selected ? 0x000000 : GDrawGetDefaultBackground(NULL);
    GRect sel;

    if ( index==-1 )
return;
    sel.x = 0; sel.width = kpd->vwidth-1;
    sel.y = (index-kpd->off_top)*kpd->uh; sel.height = kpd->uh-2;
    GDrawSetLineWidth(kpd->v,0);
    GDrawDrawRect(kpd->v,&sel,col);
}

static void KP_RefreshKP(KPData *kpd,int index) {
    GRect sel;

    if ( index<kpd->off_top || index>kpd->off_top+kpd->wh )
return;
    sel.x = 0; sel.width = kpd->vwidth;
    sel.y = (index-kpd->off_top)*kpd->uh; sel.height = kpd->uh;
    GDrawRequestExpose(kpd->v,&sel,false);
}

static void KP_KernClassAlter(KPData *kpd,int index) {
    KernPair *kp = kpd->kerns[index].kp;
    int kc_pos, kc_pos2;
    KernClass *kc = SFFindKernClass(kpd->sf,kpd->kerns[index].first,kpd->kerns[index].second,
	    &kc_pos,false);
    int i;

    if ( kc==NULL )
return;

    for ( i=0; i<kpd->kcnt; ++i ) if ( i!=index &&
	    kpd->kerns[i].kp->kcid==kp->kcid &&
	    kpd->kerns[i].kp->off==kp->off ) {
	if ( SFFindKernClass(kpd->sf,kpd->kerns[i].first,kpd->kerns[i].second,
		&kc_pos2,false)==kc && kc_pos==kc_pos2 ) {
	    kpd->kerns[i].newoff = kpd->kerns[index].newoff;
	    KP_RefreshKP(kpd,i);
	}
    }
}

static BDFChar *KP_Inside(KPData *kpd, GEvent *e) {
    struct kerns *kern;
    int index1, index2, i;
    int baseline, x, em = kpd->sf->ascent+kpd->sf->descent;
    BDFChar *bdfc1, *bdfc2;

    i = e->u.mouse.y/kpd->uh + kpd->off_top;
    if ( i>=kpd->kcnt )
return( NULL );

    kern = &kpd->kerns[i];
    index1 = kern->first->orig_pos;
    index2 = kern->second->orig_pos;
    bdfc1 = kpd->bdf->glyphs[index1];
    bdfc2 = kpd->bdf->glyphs[index2];
    if ( bdfc1 ==NULL || bdfc2==NULL )
return( NULL );
    if ( !kern->r2l )
	x = 10 + (bdfc1->width-bdfc1->xmin) + bdfc2->xmin +
		(kpd->kerns[i].newoff*kpd->bdf->pixelsize/em);
    else
	x = kpd->vwidth-10- (bdfc1->xmax-bdfc1->xmin) - bdfc1->xmin -
		(bdfc2->width-bdfc2->xmin) -
		(kern->newoff*kpd->bdf->pixelsize/em);
    if ( e->u.mouse.x < x || e->u.mouse.x>= x+bdfc2->xmax-bdfc2->xmin )
return( NULL );

    baseline = (i-kpd->off_top)*kpd->uh + kpd->sf->ascent * kpd->bdf->pixelsize / em + kpd->vpad;
    if ( e->u.mouse.y < baseline-bdfc2->ymax || e->u.mouse.y >= baseline-bdfc2->ymin )
return( NULL );

return( bdfc2 );
}

static void KP_SetCursor(KPData *kpd, int ismove ) {

    if ( kpd->movecursor!=ismove ) {
	GDrawSetCursor(kpd->v,ismove ? ct_leftright : ct_mypointer );
	kpd->movecursor = ismove;
    }
}

static BDFChar *KP_Cursor(KPData *kpd, GEvent *e) {
    if ( kpd->ac==NULL ) {
	BDFChar *bdfc2 = KP_Inside(kpd,e);

	KP_SetCursor(kpd,bdfc2!=NULL );
return( bdfc2 );
    }
return( NULL );
}

static void KP_ScrollTo(KPData *kpd,int where) {
    /* Make sure the line "where" is visible */

    if ( where<kpd->off_top || where>=kpd->off_top+kpd->wh ) {
	where -= kpd->wh/4;
	if ( where>kpd->kcnt-kpd->wh )
	    where = kpd->kcnt-kpd->wh;
	if ( where<0 ) where = 0;
	kpd->off_top = where;
	GScrollBarSetPos(GWidgetGetControl(kpd->gw,CID_ScrollBar),where);
	GDrawRequestExpose(kpd->v,NULL,false);
    }
}

static void KPRemove(KPData *kpd) {
    if ( kpd->selected!=-1 ) {
	kpd->kerns[kpd->selected].newoff = 0;
	GDrawRequestExpose(kpd->v,NULL,false);
    }
}

static void KP_Commands(KPData *kpd, GEvent *e) {
    int old_sel, amount;

    switch( e->u.chr.keysym ) {
      case '\177':
	KPRemove(kpd);
      break;
      case 'z': case 'Z':
	if ( e->u.chr.state&ksm_control ) {
	    if ( kpd->last_index!=-1 ) {
		kpd->kerns[kpd->last_index].newoff = kpd->old_val;
		KP_RefreshKP(kpd,kpd->last_index);
		if ( kpd->kerns[kpd->last_index].kp->kcid!=0 )
		    KP_KernClassAlter(kpd,kpd->last_index);
		kpd->last_index = -1;
	    }
	} else if ( e->u.chr.state&ksm_meta ) {
	    if ( kpd->selected!=-1 ) {
		kpd->kerns[kpd->selected].newoff = kpd->kerns[kpd->selected].kp->off;
		KP_RefreshKP(kpd,kpd->selected);
		if ( kpd->kerns[kpd->selected].kp->kcid!=0 )
		    KP_KernClassAlter(kpd,kpd->selected);
		kpd->last_index = -1;
	    }
	}
      break;
      case GK_Up: case GK_KP_Up:
	old_sel = kpd->selected;
	if ( kpd->selected<=0 )
	    kpd->selected = kpd->kcnt-1;
	else
	    --kpd->selected;
	KP_RefreshSel(kpd,old_sel);
	KP_RefreshSel(kpd,kpd->selected);
	KP_ScrollTo(kpd,kpd->selected);
      break;
      case GK_Down: case GK_KP_Down:
	old_sel = kpd->selected;
	if ( kpd->selected==-1 || kpd->selected==kpd->kcnt-1 )
	    kpd->selected = 0;
	else
	    ++kpd->selected;
	KP_RefreshSel(kpd,old_sel);
	KP_RefreshSel(kpd,kpd->selected);
	KP_ScrollTo(kpd,kpd->selected);
      break;
      case GK_Left: case GK_KP_Left: case GK_Right: case GK_KP_Right:
	amount = e->u.chr.keysym==GK_Left || e->u.chr.keysym==GK_KP_Left? -1 : 1;
	if ( e->u.chr.state&(ksm_shift|ksm_control|ksm_meta) ) amount *= 10;
	if ( kpd->selected!=-1 ) {
	    KP_ScrollTo(kpd,kpd->selected);
	    kpd->last_index = kpd->selected;
	    kpd->old_val = kpd->kerns[kpd->selected].newoff;
	    kpd->kerns[kpd->selected].newoff += amount;
	    KP_RefreshKP(kpd,kpd->selected);
	    if ( kpd->kerns[kpd->selected].kp->kcid!=0 )
		KP_KernClassAlter(kpd,kpd->selected);
	}
      break;
    }
}

static void KPV_Resize(KPData *kpd) {
    GRect size;
    GGadget *sb;

    GDrawGetSize(kpd->v,&size);
    kpd->wh = size.height/kpd->uh;

    sb = GWidgetGetControl(kpd->gw,CID_ScrollBar);
    GScrollBarSetBounds(sb,0,kpd->kcnt,kpd->wh);
    if ( kpd->off_top>kpd->kcnt-kpd->wh )
	kpd->off_top = kpd->kcnt-kpd->wh;
    if ( kpd->off_top<0 )
	kpd->off_top = 0;
    GScrollBarSetPos(sb,kpd->off_top);
    kpd->vwidth = size.width;
    GDrawRequestExpose(kpd->v,NULL,false);
    GDrawRequestExpose(kpd->gw,NULL,false);
}

static void KP_Resize(KPData *kpd) {

    kpd->uh = (4*kpd->bdf->pixelsize/3)+kpd->fh+6;
    kpd->vpad = kpd->bdf->pixelsize/5 + 3;
}

static int KP_ChangeSize(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	KPData *kpd = GDrawGetUserData(GGadgetGetWindow(g));
	int newsize = (intpt) (GGadgetGetListItemSelected(g)->userdata);
	BDFFont *temp;
	if ( newsize==kpd->bdf->pixelsize )
return( true );
	temp = SplineFontPieceMeal(kpd->sf,kpd->layer,newsize,72,true,NULL);
	BDFFontFree(kpd->bdf);
	kpd->bdf = temp;
	KP_Resize(kpd);
	KPV_Resize(kpd);
    }
return( true );
}

static int KP_ChangeSort(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	KPData *kpd = GDrawGetUserData(GGadgetGetWindow(g));
	KernPair *old = kpd->selected==-1 ? NULL : kpd->kerns[kpd->selected].kp;
	int i;

	KPSortEm(kpd,GGadgetGetFirstListSelectedItem(g));
	for ( i=0 ; i<kpd->kcnt; ++i )
	    if ( kpd->kerns[i].kp==old ) {
		kpd->selected = i;
		KP_ScrollTo(kpd,i);
	break;
	    }
	GDrawRequestExpose(kpd->v,NULL,false);
    }
return( true );
}

static int KP_Scrolled(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_scrollbarchange ) {
	KPData *kpd = GDrawGetUserData(GGadgetGetWindow(g));
	int newpos = kpd->off_top;

	switch( e->u.control.u.sb.type ) {
	  case et_sb_top:
	    newpos = 0;
	  break;
	  case et_sb_halfup:
	  case et_sb_uppage:
	    newpos -= kpd->wh==1?1:kpd->wh-1;
	  break;
	  case et_sb_up:
	    newpos -= 1;
	  break;
	  case et_sb_halfdown:
	  case et_sb_down:
	    newpos += 1;
	  break;
	  case et_sb_downpage:
	    newpos += kpd->wh==1?1:kpd->wh-1;
	  break;
	  case et_sb_bottom:
	    newpos = kpd->kcnt-kpd->wh;
	  break;
	  case et_sb_thumb:
	  case et_sb_thumbrelease:
	    newpos = e->u.control.u.sb.pos;
	  break;
	}
	if ( newpos>kpd->kcnt-kpd->wh )
	    newpos = kpd->kcnt-kpd->wh;
	if ( newpos<0 )
	    newpos = 0;
	if ( newpos!=kpd->off_top ) {
	    int off = newpos-kpd->off_top;
	    kpd->off_top = newpos;
	    GScrollBarSetPos(g,kpd->off_top);
	    GDrawScroll(kpd->v,NULL,0,off*kpd->uh);
	}
    }
return( true );
}

static int KP_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KPData *kpd = GDrawGetUserData(GGadgetGetWindow(g));
	int i;
	FontView *fv; MetricsView *mv;

	for ( i=0; i<kpd->kcnt; ++i ) if ( kpd->kerns[i].kp!=NULL )
	    if ( kpd->kerns[i].newoff != kpd->kerns[i].kp->off ) {
		kpd->kerns[i].kp->off = kpd->kerns[i].newoff;
		kpd->sf->changed = true;
		for ( fv=(FontView *) kpd->sf->fv; fv!=NULL; fv = (FontView *) (fv->b.nextsame) ) {
		    for ( mv=fv->b.sf->metrics; mv!=NULL; mv=mv->next )
			MVRefreshChar(mv,kpd->kerns[i].first);
		}
	    }
	kpd->done = true;
    }
return( true );
}

static int KP_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KPData *kpd = GDrawGetUserData(GGadgetGetWindow(g));
	kpd->done = true;
    }
return( true );
}

static void KPMenuRemove(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    KPData *kpd = GDrawGetUserData(gw);
    KPRemove(kpd);
}

static void KPKPCloseup(KPData *kpd) {
    if ( kpd->selected!=-1 ) {
	struct kerns *k = &kpd->kerns[kpd->selected];
	int oldoff = k->kp->off;
	k->kp->off = k->newoff;
	KernPairD(k->first->parent,k->first,k->second,kpd->layer,false);
	k->newoff = k->kp->off;
	k->kp->off = oldoff;
	GDrawRequestExpose(kpd->v,NULL,false);
	kpd->selected = -1;
    }
}

static void KPAC(KPData *kpd, int base) {
    if ( kpd->selected!=-1 ) {
	struct kerns *k = &kpd->kerns[kpd->selected];
	SplineChar *sc = base ? k->first : k->second;
	AnchorPoint *ap;
	for ( ap=sc->anchor; ap!=NULL && ap->anchor!=k->ac; ap=ap->next );
	if ( ap!=NULL ) {
	    /* There is currently no way to modify anchors in this dlg */
	    /* so the anchor will be right. On the other hand we might */
	    /* need to reinit all other combinations which use this point */
	    AnchorControl(sc,ap,kpd->layer);
	    AnchorRefigure(kpd);
	    GDrawRequestExpose(kpd->v,NULL,false);
	}
    }
}

static void KPMenuKPCloseup(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    KPData *kpd = GDrawGetUserData(gw);
    KPKPCloseup(kpd);
}

static void KPMenuACB(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    KPData *kpd = GDrawGetUserData(gw);
    KPAC(kpd,true);
}

static void KPMenuACM(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    KPData *kpd = GDrawGetUserData(gw);
    KPAC(kpd,false);
}

static GMenuItem kernmenu[] = {
    { { (unichar_t *) N_("C_lear"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, 'N' }, '\177', 0, NULL, NULL, KPMenuRemove, 0 },
    { { (unichar_t *) N_("Kern Pair Closeup"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, 'N' }, '\0', 0, NULL, NULL, KPMenuKPCloseup, 0 },
    GMENUITEM_EMPTY
};

static GMenuItem acmenu[] = {
    { { (unichar_t *) N_("Anchor Control for Base"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, 'N' }, '\0', 0, NULL, NULL, KPMenuACB, 0 },
    { { (unichar_t *) N_("Anchor Control for Mark"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, 'N' }, '\0', 0, NULL, NULL, KPMenuACM, 0 },
    GMENUITEM_EMPTY
};

static unichar_t upopupbuf[100];

static int kpdv_e_h(GWindow gw, GEvent *event) {
    KPData *kpd = GDrawGetUserData(gw);
    int index, old_sel, temp;
    char buffer[100];
    static int done=false;

    switch ( event->type ) {
      case et_expose:
	KP_ExposeKerns(kpd,gw,&event->u.expose.rect);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("kernpairs.html");
return( true );
	}
	KP_Commands(kpd,event);
      break;
      case et_mousedown:
	GGadgetEndPopup();
	kpd->pressed = true;
	index = kpd->off_top + event->u.mouse.y/kpd->uh;
	if ( index>=kpd->kcnt )
	    index = -1;
	if ( index!=kpd->selected ) {
	    old_sel = kpd->selected;
	    kpd->selected = index;
	    KP_RefreshSel(kpd,old_sel);
	    KP_RefreshSel(kpd,index);
	}
	if ( event->u.mouse.button==3 && index>=0 ) {
	    if ( !done ) {
		int i;
		for ( i=0; kernmenu[i].ti.text!=NULL || kernmenu[i].ti.line; ++i )
		    if ( kernmenu[i].ti.text!=NULL )
			kernmenu[i].ti.text = (unichar_t *) _((char *) kernmenu[i].ti.text);
		for ( i=0; acmenu[i].ti.text!=NULL || acmenu[i].ti.line; ++i )
		    if ( acmenu[i].ti.text!=NULL )
			acmenu[i].ti.text = (unichar_t *) _((char *) acmenu[i].ti.text);
		done = true;
	    }
	    if ( kpd->ac==NULL )
		GMenuCreatePopupMenu(gw,event, kernmenu);
	    else
		GMenuCreatePopupMenu(gw,event, acmenu);
	} else if ( KP_Cursor(kpd,event)!=NULL ) {
	    kpd->pressed_x = event->u.mouse.x;
	    kpd->old_val = kpd->kerns[index].newoff;
	} else
	    kpd->pressed_x = -1;
      break;
      case et_mouseup:
	if ( kpd->pressed_x!=-1 )
	    kpd->last_index = kpd->selected;
	else
	    kpd->last_index = -1;
	if ( kpd->selected>=0 && event->u.mouse.clicks>1 ) {
	    if ( kpd->ac==NULL )
		KPKPCloseup(kpd);
	    else
		KPAC(kpd,true);
return( true );
	}
      /* Fall through... */
      case et_mousemove:
	GGadgetEndPopup();
	index = kpd->off_top + event->u.mouse.y/kpd->uh;
	if ( !kpd->pressed && index<kpd->kcnt ) {
	    sprintf( buffer, "%.20s %d U+%04x",
		    kpd->kerns[index].first->name,
		    kpd->kerns[index].first->orig_pos,
		    kpd->kerns[index].first->unicodeenc );
	    if ( kpd->kerns[index].first->unicodeenc==-1 )
		strcpy(buffer+strlen(buffer)-4, "????");
	    sprintf( buffer+strlen(buffer), " + %.20s %d U+%04x",
		    kpd->kerns[index].second->name,
		    kpd->kerns[index].second->orig_pos,
		    kpd->kerns[index].second->unicodeenc );
	    if ( kpd->kerns[index].second->unicodeenc==-1 )
		strcpy(buffer+strlen(buffer)-4, "????");
	    uc_strcpy(upopupbuf,buffer);
	    GGadgetPreparePopup(gw,upopupbuf);
	    KP_Cursor(kpd,event);
	} else if ( kpd->pressed && kpd->pressed_x!=-1 ) {
	    if ( kpd->ac!=NULL ) {
		/* Nothing to be done. That's what I find so wonderful. Happy Days */
	    } else if ( index==kpd->selected ) {
		KP_SetCursor(kpd,true);
		temp = kpd->old_val + (event->u.mouse.x-kpd->pressed_x)*(kpd->sf->ascent+kpd->sf->descent)/kpd->bdf->pixelsize;
		if ( temp!=kpd->kerns[index].newoff ) {
		    kpd->kerns[index].newoff = temp;
		    KP_RefreshKP(kpd,index);
		}
	    } else {
		if ( kpd->movecursor ) {
		    kpd->kerns[kpd->selected].newoff = kpd->old_val;
		    KP_SetCursor(kpd,false);
		    KP_RefreshKP(kpd,kpd->selected);
		}
	    }
	    if ( kpd->ac==NULL && kpd->kerns[index].kp->kcid!=0 && event->type==et_mouseup )
		KP_KernClassAlter(kpd,index);
	}
	if ( event->type==et_mouseup )
	    kpd->pressed = false;
      break;
      case et_resize:
	KPV_Resize(kpd);
      break;
    }
return( true );
}

static void kpdpopup(KPData *kpd) {
    char buffer[100];

    if ( kpd->ac==NULL ) {
	sprintf( buffer, "total kern pairs=%d\nchars starting kerns=%d",
		kpd->kcnt, kpd->firstcnt );
    } else {
	sprintf( buffer, "total anchored pairs=%d\nbase char cnt=%d",
		kpd->kcnt, kpd->firstcnt );
    }
    uc_strcpy(upopupbuf,buffer);
    GGadgetPreparePopup(kpd->gw,upopupbuf);
}

static int kpd_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	KPData *kpd = GDrawGetUserData(gw);
	kpd->done = true;
    } else if ( event->type == et_mousemove ) {
	kpdpopup(GDrawGetUserData(gw));
    } else if ( event->type == et_expose ) {
	KPData *kpd = GDrawGetUserData(gw);
	GRect size, sbsize;
	GDrawGetSize(kpd->v,&size);
	GGadgetGetSize(GWidgetGetControl(kpd->gw,CID_ScrollBar),&sbsize);
	GDrawSetLineWidth(gw,0);
	GDrawDrawLine(gw,size.x,size.y-1,sbsize.x+sbsize.width-1,size.y-1,0x000000);
	GDrawDrawLine(gw,size.x,size.y+size.height,sbsize.x+sbsize.width-1,size.y+size.height,0x000000);
	GDrawDrawLine(gw,size.x-1,size.y-1,size.x-1,size.y+size.height,0x000000);
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("kernpairs.html");
return( true );
	}
	if ( event->u.chr.chars[0]!='\0' && event->u.chr.chars[1]=='\0' ) {
	    enum sortby sort = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,CID_SortBy));
	    KPData *kpd = GDrawGetUserData(gw);
	    if ( sort!=sb_kern ) {
		KPScrollTo(kpd,event->u.chr.chars[0],sort);
return( true );
	    } else
		GDrawBeep(NULL);
	}
return( false );
    } else if ( event->type == et_resize && event->u.resize.sized ) {
	KP_Resize((KPData *) GDrawGetUserData(gw) );
    }
return( true );
}

void SFShowKernPairs(SplineFont *sf,SplineChar *sc,AnchorClass *ac,int layer) {
    KPData kpd;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[9], boxes[6], *hvarray[3][3], *harray[3], *barray[10], *varray[5];
    GTextInfo label[9];
    FontRequest rq;
    int as, ds, ld,i;
    static int done=false;
    static GFont *font=NULL;

    memset(&kpd,0,sizeof(kpd));
    kpd.sf = sf;
    kpd.sc = sc;
    kpd.ac = ac;
    kpd.layer = layer;
    kpd.first = true;
    kpd.last_index = kpd.selected = -1;
    if ( ac==NULL )
	KPBuildKernList(&kpd);
    else
	KPBuildAnchorList(&kpd);
    if ( kpd.kcnt==0 )
return;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = ac==NULL?_("Kern Pairs"):_("Anchored Pairs");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(200);
    pos.height = GDrawPointsToPixels(NULL,500);
    kpd.gw = gw = GDrawCreateTopWindow(NULL,&pos,kpd_e_h,&kpd,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    label[0].text = (unichar_t *) _("_Size:");
    label[0].text_is_1byte = true;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5+6;
    gcd[0].gd.flags = gg_enabled|gg_visible;
    gcd[0].creator = GLabelCreate;
    hvarray[0][0] = &gcd[0];

    gcd[1].gd.label = &sizes[1];  gcd[1].gd.label->selected = true;
    gcd[1].gd.pos.x = 50; gcd[1].gd.pos.y = 5;
    gcd[1].gd.flags = gg_enabled|gg_visible;
    gcd[1].gd.cid = CID_Size;
    gcd[1].gd.u.list = sizes;
    gcd[1].gd.handle_controlevent = KP_ChangeSize;
    gcd[1].creator = GListButtonCreate;
    hvarray[0][1] = &gcd[1]; hvarray[0][2] = NULL;

    label[2].text = (unichar_t *) _("Sort By:");
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = gcd[0].gd.pos.x; gcd[2].gd.pos.y = gcd[0].gd.pos.y+25;
    gcd[2].gd.flags = gg_enabled|gg_visible;
    gcd[2].creator = GLabelCreate;
    hvarray[1][0] = &gcd[2];

    if ( !done ) {
	done = true;
	for ( i=0; sortby[i].text!=NULL; ++i )
	    sortby[i].text = (unichar_t *) _((char *) sortby[i].text);
    }

    gcd[3].gd.label = &sortby[0]; gcd[3].gd.label->selected = true;
    gcd[3].gd.pos.x = 50; gcd[3].gd.pos.y = gcd[1].gd.pos.y+25;
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].gd.cid = CID_SortBy;
    gcd[3].gd.u.list = sortby;
    gcd[3].gd.handle_controlevent = KP_ChangeSort;
    gcd[3].creator = GListButtonCreate;
    hvarray[1][1] = &gcd[3]; hvarray[1][2] = NULL; hvarray[2][0] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = hvarray[0];
    boxes[2].creator = GHVBoxCreate;
    varray[0] = &boxes[2];

    gcd[4].gd.pos.width = 40;
    gcd[4].gd.pos.height = 250;
    gcd[4].gd.flags = gg_visible | gg_enabled;
    gcd[4].gd.u.drawable_e_h = kpdv_e_h;
    gcd[4].creator = GDrawableCreate;

    gcd[5].gd.flags = gg_enabled|gg_visible|gg_sb_vert;
    gcd[5].gd.cid = CID_ScrollBar;
    gcd[5].gd.handle_controlevent = KP_Scrolled;
    gcd[5].creator = GScrollBarCreate;
    harray[0] = &gcd[4]; harray[1] = &gcd[5]; harray[2] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = harray;
    boxes[3].creator = GHBoxCreate;
    varray[1] = &boxes[3];

    gcd[6].gd.pos.x = 20-3; gcd[6].gd.pos.y = 17+37;
    gcd[6].gd.pos.width = -1; gcd[6].gd.pos.height = 0;
    gcd[6].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[6].text = (unichar_t *) _("_OK");
    label[6].text_is_1byte = true;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.cid = CID_OK;
    gcd[6].gd.handle_controlevent = KP_OK;
    gcd[6].creator = GButtonCreate;

    gcd[7].gd.pos.x = -20; gcd[7].gd.pos.y = gcd[6].gd.pos.y+3;
    gcd[7].gd.pos.width = -1; gcd[7].gd.pos.height = 0;
    gcd[7].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[7].text = (unichar_t *) _("_Cancel");
    label[7].text_is_1byte = true;
    label[7].text_in_resource = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.cid = CID_Cancel;
    gcd[7].gd.handle_controlevent = KP_Cancel;
    gcd[7].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[6]; barray[2] = GCD_Glue;
    barray[3] = GCD_Glue; barray[4] = &gcd[7]; barray[5] = GCD_Glue; barray[6] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = barray;
    boxes[4].creator = GHBoxCreate;
    varray[2] = &boxes[4];
    varray[3] = NULL;

    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GVBoxCreate;


    GGadgetsCreate(gw,boxes);

    GHVBoxSetExpandableRow(boxes[0].ret,1);
    GHVBoxSetExpandableCol(boxes[3].ret,0);
    GHVBoxSetExpandableCol(boxes[4].ret,gb_expandgluesame);
    GHVBoxSetPadding(boxes[0].ret,0,2);
    GHVBoxSetPadding(boxes[3].ret,0,0);
    kpd.v = GDrawableGetWindow(gcd[4].ret);;

    GGadgetGetSize(gcd[4].ret,&pos);
    kpd.sb_width = pos.width;
    GGadgetGetSize(gcd[3].ret,&pos);
    kpd.header_height = pos.y+pos.height+4;

    kpd.bdf = SplineFontPieceMeal(kpd.sf,kpd.layer,(intpt) (gcd[1].gd.label->userdata),72,true,NULL);

    if ( font==NULL ) {
	memset(&rq,'\0',sizeof(rq));
	rq.utf8_family_name = SANS_UI_FAMILIES;
	rq.point_size = -12;
	rq.weight = 400;
	font = GDrawInstanciateFont(gw,&rq);
	font = GResourceFindFont("Combinations.Font",font);
    }
    kpd.font = font;
    GDrawWindowFontMetrics(gw,kpd.font,&as,&ds,&ld);
    kpd.fh = as+ds; kpd.as = as;

    kpd.uh = (4*kpd.bdf->pixelsize/3)+kpd.fh+6;
    kpd.vpad = kpd.bdf->pixelsize/5 + 3;

    GHVBoxFitWindow(boxes[0].ret);

    GDrawSetVisible(kpd.v,true);
    GDrawSetVisible(kpd.gw,true);
    while ( !kpd.done )
	GDrawProcessOneEvent(NULL);
    free( kpd.kerns );
    GDrawDestroyWindow(gw);
}
