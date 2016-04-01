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
#include <math.h>
#include <time.h>
#include <utype.h>
#include <ustring.h>
#include <gimage.h>		/* For COLOR_DEFAULT */

#include "ttf.h"

/* This file contains routines to generate non-standard true/opentype tables */
/* The first is the 'PfEd' table containing PfaEdit specific information */
/*	glyph comments & colours ... perhaps other info later */

/* ************************************************************************** */
/* *************************    The 'PfEd' table    ************************* */
/* *************************         Output         ************************* */
/* ************************************************************************** */

#include "PfEd.h"	/* This describes the format of the 'PfEd' table */
			/*  and its many subtables. */

#define MAX_SUBTABLE_TYPES	20

struct PfEd_subtabs {
    int next;
    struct {
	FILE *data;
	uint32 tag;
	uint32 offset;
    } subtabs[MAX_SUBTABLE_TYPES];
};

static void PfEd_FontComment(SplineFont *sf, struct PfEd_subtabs *pfed, uint32 tag ) {
    FILE *fcmt;
    char *pt;
    char *text = tag==fcmt_TAG ? sf->comments : sf->fontlog;

    if ( text==NULL || *text=='\0' )
return;
    pfed->subtabs[pfed->next].tag = tag;
    pfed->subtabs[pfed->next++].data = fcmt = tmpfile();

    putshort(fcmt,1);			/* sub-table version number */
    putshort(fcmt,strlen(text));
    for ( pt = text; *pt; ++pt )
	putc(*pt,fcmt);
    putshort(fcmt,0);
    if ( ftell(fcmt)&1 ) putc(0,fcmt);
    if ( ftell(fcmt)&2 ) putshort(fcmt,0);
}

static void PfEd_GlyphComments(SplineFont *sf, struct PfEd_subtabs *pfed,
	struct glyphinfo *gi ) {
    int i, j, k, any, cnt, last, skipped;
    uint32 offset;
    SplineChar *sc, *sc2;
    FILE *cmnt;

    any = 0;
    /* We don't need to check in bygid order. We just want to know existance */
    for ( i=0; i<sf->glyphcnt; ++i ) {
	if ( sf->glyphs[i]!=NULL && sf->glyphs[i]->ttf_glyph!=-1 &&
		sf->glyphs[i]->comment!=NULL ) {
	    any = true;
    break;
	}
    }

    if ( !any )
return;

    pfed->subtabs[pfed->next].tag = cmnt_TAG;
    pfed->subtabs[pfed->next++].data = cmnt = tmpfile();

    putshort(cmnt,1);			/* sub-table version number */
	    /* Version 0 used ucs2, version 1 uses utf8 */

    offset = 0;
    for ( j=0; j<4; ++j ) {
	cnt = 0;
	for ( i=0; i<gi->gcnt; ++i ) if ( gi->bygid[i]!=-1 ) {
	    sc=sf->glyphs[gi->bygid[i]];
	    if ( sc!=NULL && sc->comment!=NULL ) {
		last = i; skipped = false;
		for ( k=i+1; k<gi->gcnt; ++k ) {
		    if ( gi->bygid[k]!=-1 )
			sc2 = sf->glyphs[gi->bygid[k]];
		    if ( (gi->bygid[k]==-1 || sc2->comment==NULL) && skipped )
		break;
		    if ( gi->bygid[k]!=-1 && sc2->comment!=NULL ) {
			last = k;
			skipped = false;
		    } else
			skipped = true;
		}
		++cnt;
		if ( j==1 ) {
		    putshort(cmnt,i);
		    putshort(cmnt,last);
		    putlong(cmnt,offset);
		    offset += sizeof(uint32)*(last-i+2);
		} else if ( j==2 ) {
		    for ( ; i<=last; ++i ) {
			if ( gi->bygid[i]==-1 || (sc=sf->glyphs[gi->bygid[i]])->comment==NULL )
			    putlong(cmnt,0);
			else {
			    putlong(cmnt,offset);
			    offset += strlen(sc->comment)+1;
			}
		    }
		    putlong(cmnt,offset);	/* Guard data, to let us calculate the string lengths */
		} else if ( j==3 ) {
		    for ( ; i<=last; ++i ) {
			if ( gi->bygid[i]==-1 || (sc=sf->glyphs[gi->bygid[i]])->comment==NULL )
		    continue;
			fputs(sc->comment,cmnt);
			putc('\0',cmnt);
		    }
		}
		i = last;
	    }
	}
	if ( j==0 ) {
	    putshort(cmnt,cnt);
	    offset = 2*sizeof(short) + cnt*(2*sizeof(short)+sizeof(uint32));
	}
    }
    if ( ftell(cmnt) & 1 )
	putc('\0',cmnt);
    if ( ftell(cmnt) & 2 )
	putshort(cmnt,0);
}

static void PfEd_CvtComments(SplineFont *sf, struct PfEd_subtabs *pfed ) {
    FILE *cvtcmt;
    int i, offset;

    if ( sf->cvt_names==NULL )
return;
    pfed->subtabs[pfed->next].tag = cvtc_TAG;
    pfed->subtabs[pfed->next++].data = cvtcmt = tmpfile();

    for ( i=0; sf->cvt_names[i]!=END_CVT_NAMES; ++i);

    putshort(cvtcmt,0);			/* sub-table version number */
    putshort(cvtcmt,i);
    offset = 2*2 + i*2;
    for ( i=0; sf->cvt_names[i]!=END_CVT_NAMES; ++i) {
	if ( sf->cvt_names[i]==NULL )
	    putshort(cvtcmt,0);
	else {
	    putshort(cvtcmt,offset);
	    offset += strlen(sf->cvt_names[i])+1;
	}
    }
    for ( i=0; sf->cvt_names[i]!=END_CVT_NAMES; ++i) {
	if ( sf->cvt_names[i]!=NULL ) {
	    fputs(sf->cvt_names[i],cvtcmt);
	    putc('\0',cvtcmt);
	}
    }
    if ( ftell(cvtcmt)&1 ) putc(0,cvtcmt);
    if ( ftell(cvtcmt)&2 ) putshort(cvtcmt,0);
}

static void PfEd_Colours(SplineFont *sf, struct PfEd_subtabs *pfed, struct glyphinfo *gi ) {
    int i, j, k, any, cnt, last;
    SplineChar *sc, *sc2;
    FILE *colr;

    any = 0;
    for ( i=0; i<sf->glyphcnt; ++i ) {
	if ( sf->glyphs[i]!=NULL && sf->glyphs[i]->ttf_glyph!=-1 &&
		sf->glyphs[i]->color!=COLOR_DEFAULT ) {
	    any = true;
    break;
	}
    }

    if ( !any )
return;

    pfed->subtabs[pfed->next].tag = colr_TAG;
    pfed->subtabs[pfed->next++].data = colr = tmpfile();

    putshort(colr,0);			/* sub-table version number */
    for ( j=0; j<2; ++j ) {
	cnt = 0;
	for ( i=0; i<gi->gcnt; ++i ) if ( gi->bygid[i]!=-1 ) {
	    sc = sf->glyphs[gi->bygid[i]];
	    if ( sc!=NULL && sc->color!=COLOR_DEFAULT ) {
		last = i;
		for ( k=i+1; k<gi->gcnt; ++k ) {
		    if ( gi->bygid[k]==-1 )
		break;
		    sc2 = sf->glyphs[gi->bygid[k]];
		    if ( sc2->color != sc->color )
		break;
		    last = k;
		}
		++cnt;
		if ( j==1 ) {
		    putshort(colr,i);
		    putshort(colr,last);
		    putlong(colr,sc->color);
		}
		i = last;
	    }
	}
	if ( j==0 )
	    putshort(colr,cnt);
    }
    if ( ftell(colr) & 2 )
	putshort(colr,0);
}

static void PfEd_Lookups(SplineFont *sf, struct PfEd_subtabs *pfed,
	OTLookup *lookups, uint32 tag) {
    OTLookup *otl;
    int lcnt, scnt, ascnt, acnt, s, a;
    FILE *lkf;
    struct lookup_subtable *subs;
    AnchorClass *ac;
    int sub_info, ac_info, name_info;

    if ( lookups==NULL )
return;
    for ( otl=lookups, lcnt=scnt=acnt=ascnt=0; otl!=NULL; otl=otl->next ) if ( !otl->unused ) {
	++lcnt;
	for ( subs = otl->subtables; subs!=NULL; subs=subs->next ) if ( !subs->unused ) {
	    ++scnt;
	    if ( subs->anchor_classes ) {
		++ascnt;
		for ( ac=sf->anchor; ac!=NULL; ac=ac->next )
		    if ( ac->subtable==subs && ac->has_base && ac->has_mark )
			++acnt;
	    }
	}
    }

    pfed->subtabs[pfed->next].tag = tag;
    pfed->subtabs[pfed->next++].data = lkf = tmpfile();

    putshort(lkf,0);			/* Subtable version */
    putshort(lkf,lcnt);

    sub_info = 4 + 4*lcnt;
    ac_info =  sub_info + 2*lcnt + 4*scnt;
    name_info = ac_info + 2*ascnt + 2*acnt;
    for ( otl=lookups; otl!=NULL; otl=otl->next ) if ( !otl->unused ) {
	putshort(lkf,name_info);
	name_info += strlen(otl->lookup_name)+1;
	putshort(lkf,sub_info);
	for ( subs = otl->subtables, s=0; subs!=NULL; subs=subs->next ) if ( !subs->unused ) ++s;
	sub_info += 2 + 4*s;
    }
    if ( sub_info!=ac_info )
	IError("Lookup name data didn't behave as expected");
    for ( otl=lookups; otl!=NULL; otl=otl->next ) if ( !otl->unused ) {
	for ( subs = otl->subtables, s=0; subs!=NULL; subs=subs->next ) if ( !subs->unused ) ++s;
	putshort(lkf,s);			/* Subtable count */
	for ( subs = otl->subtables, s=0; subs!=NULL; subs=subs->next ) if ( !subs->unused ) {
	    putshort(lkf,name_info);
	    name_info += strlen(subs->subtable_name)+1;
	    if ( subs->anchor_classes ) {
		putshort(lkf,ac_info);
		for ( ac=sf->anchor, a=0; ac!=NULL; ac=ac->next )
		    if ( ac->subtable==subs && ac->has_base && ac->has_mark )
			++a;
		ac_info += 2 + 2*a;
	    } else
		putshort(lkf,0);
	}
    }
    for ( otl=lookups; otl!=NULL; otl=otl->next ) if ( !otl->unused ) {
	for ( subs = otl->subtables, s=0; subs!=NULL; subs=subs->next ) if ( !subs->unused ) {
	    if ( subs->anchor_classes ) {
		for ( ac=sf->anchor, a=0; ac!=NULL; ac=ac->next )
		    if ( ac->subtable==subs && ac->has_base && ac->has_mark )
			++a;
		putshort(lkf,a);
		for ( ac=sf->anchor, a=0; ac!=NULL; ac=ac->next )
		    if ( ac->subtable==subs && ac->has_base && ac->has_mark ) {
			putshort(lkf,name_info);
			name_info += strlen(ac->name)+1;
		    }
	    }
	}
    }
    for ( otl=lookups; otl!=NULL; otl=otl->next ) if ( !otl->unused ) {
	fputs(otl->lookup_name,lkf);
	putc('\0',lkf);
    }
    for ( otl=lookups; otl!=NULL; otl=otl->next ) if ( !otl->unused ) {
	for ( subs = otl->subtables, s=0; subs!=NULL; subs=subs->next ) if ( !subs->unused ) {
	    fputs(subs->subtable_name,lkf);
	    putc('\0',lkf);
	}
    }
    for ( otl=lookups; otl!=NULL; otl=otl->next ) if ( !otl->unused ) {
	for ( subs = otl->subtables, s=0; subs!=NULL; subs=subs->next ) if ( !subs->unused ) {
	    for ( ac=sf->anchor, a=0; ac!=NULL; ac=ac->next )
		if ( ac->subtable==subs && ac->has_base && ac->has_mark ) {
		    fputs(ac->name,lkf);
		    putc('\0',lkf);
		}
	}
    }
    if ( ftell(lkf) & 1 )
	putc('\0',lkf);
    if ( ftell(lkf) & 2 )
	putshort(lkf,0);
}

static int pfed_mod_type(float val,int last_mod) {
    float ival;

    if ( last_mod==V_F )
return( V_F );
    ival = rint(val);
    if ( ival!=val || ival<-32768 || ival>32767 )
return( V_F );
    if ( last_mod==V_S || ival<-128 || ival>127 )
return( V_S );

return( V_B );
}

static void pfed_write_data(FILE *ttf, float val, int mod) {
    if ( mod==V_F )
	putlong(ttf,(int) rint(val*256.0f));
    else if ( mod==V_S )
	putshort(ttf,(int) rint(val));
    else
	putc(((int) rint(val)), ttf);
}

static void pfed_glyph_layer(FILE *layr,Layer *layer, int do_spiro) {
    int contour_cnt, image_cnt, ref_cnt, name_off, i,j;
    SplineSet *ss;
    SplinePoint *sp;
    uint32 base;
    int mod, was_implicit;
    RefChar *ref;

    contour_cnt = 0;
    for ( ss=layer->splines; ss!=NULL; ss=ss->next )
	++contour_cnt;
    image_cnt = 0;
    /* I'm not doing images yet (if ever) but I leave space for them */
    ref_cnt = 0;
    for ( ref=layer->refs; ref!=NULL; ref=ref->next )
	++ref_cnt;

    base = ftell(layr);
    putshort(layr,contour_cnt);
    putshort(layr,ref_cnt);
    putshort(layr,image_cnt);

    name_off = 2*3 + 4 * contour_cnt + (4*7+2)* ref_cnt;
    for ( ss=layer->splines; ss!=NULL; ss=ss->next ) {
	putshort(layr,0);			/* fill in later */
	if ( ss->contour_name!=NULL ) {
	    putshort(layr,name_off);
	    name_off += strlen(ss->contour_name)+1;
	} else {
	    putshort(layr,0);
	}
    }
    for ( ref=layer->refs; ref!=NULL; ref=ref->next ) {
	for ( j=0; j<6; ++j )
	    putlong(layr, (int) rint(ref->transform[j]*32768));
	putshort(layr,ref->sc->ttf_glyph);
    }
    for ( ss=layer->splines; ss!=NULL; ss=ss->next ) {
	if ( ss->contour_name!=NULL ) {
	    fputs(ss->contour_name,layr);
	    putc('\0',layr);
	}
    }

    contour_cnt=0;
    for ( ss=layer->splines; ss!=NULL; ss=ss->next, ++contour_cnt ) {
	uint32 pos = ftell(layr);
	fseek( layr, base + 6 + 4*contour_cnt, SEEK_SET);
	putshort( layr, pos-base);
	fseek( layr, pos, SEEK_SET );

	if ( !do_spiro ) {
	    sp = ss->first;
	    mod = pfed_mod_type(sp->me.x, pfed_mod_type(sp->me.y,V_B));
	    putc( (V_MoveTo|mod),layr);
	    pfed_write_data(layr,sp->me.x,mod);
	    pfed_write_data(layr,sp->me.y,mod);
	    was_implicit = false;

	    while ( sp->next!=NULL ) {
		SplinePoint *nsp = sp->next->to;
		float offx = nsp->me.x - sp->me.x;
		float offy = nsp->me.y - sp->me.y;
		if ( offx==0 && offy==0 )
		    /* Do Nothing */;
		else if ( sp->next->knownlinear ) {
		    mod = pfed_mod_type(offx, pfed_mod_type(offy,V_B));
		    if ( offx==0 ) {
			putc( (V_VLineTo|mod), layr);
			pfed_write_data(layr,offy,mod);
		    } else if ( offy==0 ) {
			putc( (V_HLineTo|mod), layr);
			pfed_write_data(layr,offx,mod);
		    } else {
			putc( (V_LineTo|mod), layr);
			pfed_write_data(layr,offx,mod);
			pfed_write_data(layr,offy,mod);
		    }
		} else if ( sp->next->order2 ) {
		    float offx1, offx2, offy1, offy2;
		    BasePoint *base = was_implicit ? &sp->prevcp : &sp->me;
		    offx1 = sp->nextcp.x - base->x;
		    offy1 = sp->nextcp.y - base->y;
		    mod = pfed_mod_type(offx1, pfed_mod_type(offy1,V_B));
		    if ( SPInterpolate(nsp) && nsp!=ss->first ) {
			was_implicit = true;
			if ( offx1==0 ) {
			    putc( (V_QVImplicit|mod), layr);
			    pfed_write_data(layr,offy1,mod);
			} else if ( offy1==0 ) {
			    putc( (V_QHImplicit|mod), layr);
			    pfed_write_data(layr,offx1,mod);
			} else {
			    putc( (V_QImplicit|mod), layr);
			    pfed_write_data(layr,offx1,mod);
			    pfed_write_data(layr,offy1,mod);
			}
		    } else {
			offx2 = nsp->me.x - sp->nextcp.x;
			offy2 = nsp->me.y - sp->nextcp.y;
			mod = pfed_mod_type(offx2, pfed_mod_type(offy2,mod));
			was_implicit = false;
			putc( (V_QCurveTo|mod), layr);
			pfed_write_data(layr,offx1,mod);
			pfed_write_data(layr,offy1,mod);
			pfed_write_data(layr,offx2,mod);
			pfed_write_data(layr,offy2,mod);
		    }
		} else {
		    float offx1 = sp->nextcp.x - sp->me.x;
		    float offy1 = sp->nextcp.y - sp->me.y;
		    float offx2 = nsp->prevcp.x - sp->nextcp.x;
		    float offy2 = nsp->prevcp.y - sp->nextcp.y;
		    float offx3 = nsp->me.x - nsp->prevcp.x;
		    float offy3 = nsp->me.y - nsp->prevcp.y;
		    mod = pfed_mod_type(offx1, pfed_mod_type(offy1,V_B));
		    mod = pfed_mod_type(offx2, pfed_mod_type(offy2,mod));
		    mod = pfed_mod_type(offx3, pfed_mod_type(offy3,mod));
		    if ( offx1==0 && offy3==0 ) {
			putc((V_VHCurveTo|mod),layr);
			pfed_write_data(layr,offy1,mod);
			pfed_write_data(layr,offx2,mod);
			pfed_write_data(layr,offy2,mod);
			pfed_write_data(layr,offx3,mod);
		    } else if ( offy1==0 && offx3==0 ) {
			putc((V_HVCurveTo|mod),layr);
			pfed_write_data(layr,offx1,mod);
			pfed_write_data(layr,offx2,mod);
			pfed_write_data(layr,offy2,mod);
			pfed_write_data(layr,offy3,mod);
		    } else {
			putc((V_CurveTo|mod),layr);
			pfed_write_data(layr,offx1,mod);
			pfed_write_data(layr,offy1,mod);
			pfed_write_data(layr,offx2,mod);
			pfed_write_data(layr,offy2,mod);
			pfed_write_data(layr,offx3,mod);
			pfed_write_data(layr,offy3,mod);
		    }
		}
		if ( nsp == ss->first )
	    break;
		if ( nsp->next!=NULL && nsp->next->to==ss->first && nsp->next->knownlinear )
	    break;
		sp = nsp;
	    }
	    if ( sp->next==NULL )
		putc(V_End,layr);
	    else
		putc(V_Close,layr);
	} else if ( ss->spiro_cnt==0 )
	    putc(SPIRO_CLOSE_CONTOUR,layr);		/* Mark for an empty spiro contour */
	else {
	    for ( i=0; i<ss->spiro_cnt; ++i ) {
		if ( i==ss->spiro_cnt-1 && ss->first->prev==NULL )
		    putc(SPIRO_CLOSE_CONTOUR,layr);
		else if ( i==0 && ss->first->prev==NULL )	/* Open */
		    putc(SPIRO_OPEN_CONTOUR,layr);
		else
		    putc(ss->spiros[i].ty&0x7f,layr);
		putlong(layr,rint(ss->spiros[i].x*256.0));
		putlong(layr,rint(ss->spiros[i].y*256.0));
	    }
	    putc(SPIRO_END,layr);		/* Add the z whether open or not. Might as well */
	}
    }
}

struct pos_name {
    real pos;
    char *name;
};

static int pfed_guide_real_comp(const void *_r1, const void *_r2) {
    const struct pos_name *r1 = _r1, *r2 = _r2;

    if ( r1->pos>r2->pos )
return( 1 );
    else if ( r1->pos<r2->pos )
return( -1 );
    else
return( 0 );
}

static int pfed_guide_sortuniq( struct pos_name *array, int cnt) {
    int i,j;

    qsort(array,cnt,sizeof(struct pos_name),pfed_guide_real_comp);
    for ( i=j=0; i<cnt; ++i ) {
	if ( array[i].pos<-32768 || array[i].pos>32767 )
	    /* Out of bounds, ignore it */;
	else if ( i!=0 && array[i].pos == array[i-1].pos )
	    /* Duplicate, ignore it */;
	else
	    array[j++] = array[i];
    }
return( j );
}

static int pfed_guide_dump_pos_name(FILE *guid, struct pos_name *pn, int namestart ) {
    putshort(guid,(short) rint(pn->pos));
    if ( pn->name!=NULL ) {
	putshort(guid,namestart);
	namestart += strlen(pn->name)+1;
    } else {
	putshort(guid,0);
    }
return( namestart );
}

static void PfEd_Guides(SplineFont *sf, struct PfEd_subtabs *pfed ) {
    int h,v, i;
    SplineSet *ss;
    Spline *s, *first;
    FILE *guid;
    struct pos_name hs[100], vs[100];
    int nameoff, namelen;

    if ( sf->grid.splines==NULL )
return;

    h=v=0;
    for ( ss=sf->grid.splines; ss!=NULL; ss=ss->next ) {
	first = NULL;
	for ( s=ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
	    if ( first==NULL ) first = s;
	    if ( s->from->me.x==s->to->me.x ) {
		if ( s->from->me.y!=s->to->me.y && v<100 ) {
		    vs[v].name = ss->contour_name;
		    vs[v++].pos = s->from->me.x;
		}
	    } else if ( s->from->me.y==s->to->me.y ) {
		if ( h<100 ) {
		    hs[h].name = ss->contour_name;
		    hs[h++].pos = s->from->me.y;
		}
	    }
	}
    }

    v = pfed_guide_sortuniq(vs,v);
    h = pfed_guide_sortuniq(hs,h);

    pfed->subtabs[pfed->next].tag = guid_TAG;
    pfed->subtabs[pfed->next++].data = guid = tmpfile();

    nameoff   = 5*2 + (h+v) * 4;
    namelen   = 0;
    for ( i=0; i<v; ++i ) if ( vs[i].name!=NULL )
	namelen += strlen( vs[i].name )+1;
    for ( i=0; i<h; ++i ) if ( hs[i].name!=NULL )
	namelen += strlen( hs[i].name )+1;

    putshort(guid,1);			/* sub-table version number */
    putshort(guid,v);
    putshort(guid,h);
    putshort(guid,0);			/* Diagonal lines someday? nothing for now */
    putshort(guid,nameoff+namelen);	/* full spline output */
    for ( i=0; i<v; ++i )
	nameoff = pfed_guide_dump_pos_name(guid, &vs[i], nameoff );
    for ( i=0; i<h; ++i )
	nameoff = pfed_guide_dump_pos_name(guid, &hs[i], nameoff );

    for ( i=0; i<v; ++i ) if ( vs[i].name!=NULL ) {
	fputs(vs[i].name,guid);
	putc('\0',guid);
    }
    for ( i=0; i<h; ++i ) if ( hs[i].name!=NULL ) {
	fputs(hs[i].name,guid);
	putc('\0',guid);
    }

    pfed_glyph_layer(guid,&sf->grid,false);

    if ( ftell(guid) & 1 )
	putc('\0',guid);
    if ( ftell(guid) & 2 )
	putshort(guid,0);
}

static int pfed_has_spiros(Layer *layer) {
    SplineSet *ss;

    for ( ss=layer->splines; ss!=NULL; ss=ss->next ) {
	if ( ss->spiro_cnt>1 )
return( true );
    }
return( false );
}

static void PfEd_Layer(SplineFont *sf, struct glyphinfo *gi, int layer, int dospiro,
	FILE *layr) {
    int i, j, k, gid, cnt, last, skipped;
    SplineChar *sc, *sc2;
    uint32 offset;
    uint32 *glyph_data_offset_location;

    for ( i=0; i<gi->gcnt; ++i ) if ( gi->bygid[i]!=-1 )
	if ( (sc=sf->glyphs[gi->bygid[i]])!=NULL ) {
	    sc->ticked = false;
	    if ( (!dospiro && (sc->layers[layer].splines!=NULL || sc->layers[layer].refs!=NULL) ) ||
		    (dospiro && pfed_has_spiros(&sc->layers[layer])) )
		sc->ticked=true;
	}

    offset = ftell(layr);
    glyph_data_offset_location = calloc(gi->gcnt,sizeof(uint32));
    for ( j=0; j<4; ++j ) {
	cnt = 0;
	for ( i=0; i<gi->gcnt; ++i ) if ( (gid=gi->bygid[i])!=-1 && (sc=sf->glyphs[gid])!=NULL ) {
	    if ( sc->ticked ) {
		last = i; skipped = false;
		for ( k=i+1; k<gi->gcnt; ++k ) {
		    sc2 = NULL;
		    if ( gi->bygid[k]!=-1 )
			sc2 = sf->glyphs[gi->bygid[k]];
		    if ( skipped && (sc2==NULL || !sc2->ticked))
		break;
		    if ( sc2!=NULL && sc2->ticked ) {
			last = k;
			skipped = false;
		    } else
			skipped = true;
		}
		++cnt;
		if ( j==1 ) {
		    putshort(layr,i);
		    putshort(layr,last);
		    putlong(layr,offset);
		    offset += sizeof(uint32)*(last-i+1);
		} else if ( j==2 ) {
		    for ( ; i<=last; ++i ) {
			if ( gi->bygid[i]==-1 || !sf->glyphs[gi->bygid[i]]->ticked )
			    putlong(layr,0);
			else {
			    glyph_data_offset_location[i] = ftell(layr);
			    putlong(layr,0);
			}
		    }
		} else if ( j==3 ) {
		    for ( ; i<=last; ++i ) {
			if ( gi->bygid[i]!=-1 && (sc=sf->glyphs[gi->bygid[i]])->ticked ) {
			    uint32 pos = ftell(layr);
			    fseek(layr,glyph_data_offset_location[i],SEEK_SET);
			    putlong(layr,pos);	/* Offset relative to start of subtable==start of file */
			    fseek(layr,pos,SEEK_SET);
			    pfed_glyph_layer(layr,&sc->layers[layer],dospiro);
			}
		    }
		}
		i = last;
	    }
	}
	if ( j==0 ) {
	    offset += sizeof(short) + cnt*(2*sizeof(short)+sizeof(uint32));
	    putshort(layr,cnt);
	}
    }
    free(glyph_data_offset_location);
}

static void PfEd_Layers(SplineFont *sf, struct PfEd_subtabs *pfed,
	struct glyphinfo *gi ) {
    /* currently we output the following:              */
    /*  The background layer                           */
    /*  And the spiro representation of the foreground */
    /*  if the foreground is cubic and output is quad then the foreground */
    /*  Any other layers      		               */
    /* Check if any of these data exist                */
    uint8 has_spiro=0;
    uint8 *otherlayers;
    int i, name_off, l, cnt, sofar;
    SplineChar *sc;
    FILE *layr;

    otherlayers = calloc(sf->layer_cnt,sizeof(uint8));

    /* We don't need to check in bygid order. We just want to know existance */
    /* We don't check for refs because a reference to an empty glyph is empty too */
    for ( i=0; i<sf->glyphcnt; ++i ) {
	if ( (sc=sf->glyphs[i])!=NULL && sc->ttf_glyph!=-1 ) {
	    if ( pfed_has_spiros(&sc->layers[ly_fore]))
		has_spiro = true;
	    for ( l=ly_back ; l<sf->layer_cnt; ++l )
		if ( sc->layers[l].splines!=NULL )
		    otherlayers[l] = true;
	}
    }
    otherlayers[gi->layer] = (!sf->layers[gi->layer].order2 &&  gi->is_ttf) ||
			     ( sf->layers[gi->layer].order2 && !gi->is_ttf);

    for ( l=cnt=0; l<sf->layer_cnt; ++l )
	if ( otherlayers[l] )
	    ++cnt;
    cnt += has_spiro;
    if ( cnt==0 ) {
    free(otherlayers);
return;
    }

    pfed->subtabs[pfed->next].tag = layr_TAG;
    pfed->subtabs[pfed->next++].data = layr = tmpfile();

    putshort(layr,1);			/* sub-table version */
    putshort(layr,cnt);			/* layer count */

    name_off = 4 + 8 * cnt;
    if ( has_spiro ) {
	putshort(layr,1);		/* spiros */
	putshort(layr,name_off);
	name_off += strlen("Spiro")+1;
	putlong(layr,0);		/* Fill in later */
    }
    for ( l=0; l<sf->layer_cnt; ++l ) if ( otherlayers[l]) {
	putshort(layr,(sf->layers[l].order2?2:3) |		/* Quadratic/cubic */
		      (sf->layers[l].background?0:0x100));	/* Fore/Back */
	putshort(layr,name_off);
	if ( l==ly_fore ) name_off += strlen("Old_");
	name_off += strlen(sf->layers[l].name)+1;
	putlong(layr,0);		/* Fill in later */
    }
    if ( has_spiro ) {
	fputs("Spiro",layr);
	putc('\0',layr);
    }
    for ( l=0; l<sf->layer_cnt; ++l ) if ( otherlayers[l]) {
	if ( l==ly_fore ) fputs("Old_",layr);
	fputs(sf->layers[l].name,layr);
	putc('\0',layr);
    }

    sofar = 0;
    if ( has_spiro ) {
	uint32 pos = ftell(layr);
	fseek(layr, 4 + 0*8 + 4, SEEK_SET);
	putlong(layr,pos);
	fseek(layr, 0, SEEK_END);
	PfEd_Layer(sf, gi, ly_fore, true, layr);
	++sofar;
    }
    for ( l=0; l<sf->layer_cnt; ++l ) if ( otherlayers[l]) {
	uint32 pos = ftell(layr);
	fseek(layr, 4 + sofar*8 + 4, SEEK_SET);
	putlong(layr,pos);
	fseek(layr, 0, SEEK_END);
	PfEd_Layer(sf, gi, l, false, layr);
	++sofar;
    }

    if ( ftell(layr) & 1 )
	putc('\0',layr);
    if ( ftell(layr) & 2 )
	putshort(layr,0);
    free(otherlayers);
}

void pfed_dump(struct alltabs *at, SplineFont *sf) {
    struct PfEd_subtabs pfed;
    FILE *file;
    int i;
    uint32 offset;

    memset(&pfed,0,sizeof(pfed));
    if ( at->gi.flags & ttf_flag_pfed_comments ) {
	PfEd_FontComment(sf, &pfed, fcmt_TAG );
	PfEd_FontComment(sf, &pfed, flog_TAG );
	PfEd_GlyphComments(sf, &pfed, &at->gi );
	PfEd_CvtComments(sf, &pfed );
    }
    if ( at->gi.flags & ttf_flag_pfed_colors )
	PfEd_Colours(sf, &pfed, &at->gi );
    if ( (at->gi.flags & ttf_flag_pfed_lookupnames) && at->opentypemode ) {
	PfEd_Lookups(sf, &pfed, sf->gsub_lookups, GSUB_TAG );
	PfEd_Lookups(sf, &pfed, sf->gpos_lookups, GPOS_TAG );
    }
    if ( at->gi.flags & ttf_flag_pfed_guides )
	PfEd_Guides(sf, &pfed);
    if ( at->gi.flags & ttf_flag_pfed_layers )
	PfEd_Layers(sf, &pfed, &at->gi);

    if ( pfed.next==0 )
return;		/* No subtables */

    at->pfed = file = tmpfile();
    putlong(file, 0x00010000);		/* Version number */
    putlong(file, pfed.next);		/* sub-table count */
    offset = 2*sizeof(uint32) + 2*pfed.next*sizeof(uint32);
    for ( i=0; i<pfed.next; ++i ) {
	putlong(file,pfed.subtabs[i].tag);
	putlong(file,offset);
	fseek(pfed.subtabs[i].data,0,SEEK_END);
	pfed.subtabs[i].offset = offset;
	offset += ftell(pfed.subtabs[i].data);
    }
    for ( i=0; i<pfed.next; ++i ) {
	fseek(pfed.subtabs[i].data,0,SEEK_SET);
	ttfcopyfile(file,pfed.subtabs[i].data,pfed.subtabs[i].offset,"PfEd-subtable");
    }
    if ( ftell(file)&3 )
	IError("'PfEd' table not properly aligned");
    at->pfedlen = ftell(file);
}

/* *************************    The 'PfEd' table    ************************* */
/* *************************          Input         ************************* */

static void pfed_readfontcomment(FILE *ttf,struct ttfinfo *info,uint32 base,
	uint32 tag) {
    int len;
    char *start, *pt, *end;
    int use_utf8;

    fseek(ttf,base,SEEK_SET);
    use_utf8 = getushort(ttf);
    if ( use_utf8!=0 && use_utf8!=1 )
return;			/* Bad version number */
    len = getushort(ttf);
    start = pt = malloc(len+1);

    end = pt+len;
    if ( use_utf8 ) {
	while ( pt<end )
	    *pt++ = getc(ttf);
    } else {
	while ( pt<end )
	    *pt++ = getushort(ttf);
    }
    *pt = '\0';
    if ( !use_utf8 ) {
	pt = latin1_2_utf8_copy(info->fontcomments);
	free(start);
	start = pt;
    }
    if ( tag==flog_TAG )
	info->fontlog = start;
    else
	info->fontcomments = start;
}

static char *pfed_read_utf8(FILE *ttf, uint32 start) {
    int ch, len;
    char *str, *pt;

    fseek( ttf, start, SEEK_SET);
    len = 0;
    while ( (ch=getc(ttf))!='\0' && ch!=EOF )
	++len;
    fseek( ttf, start, SEEK_SET);
    str = pt = malloc(len+1);
    while ( (ch=getc(ttf))!='\0' && ch!=EOF )
	*pt++ = ch;
    *pt = '\0';
return( str );
}

static char *pfed_read_ucs2_len(FILE *ttf,uint32 offset,int len) {
    char *pt, *str;
    uint32 uch, uch2;
    int i;

    if ( len<0 )
return( NULL );

    len>>=1;
    if ( (pt=str=malloc(len>0 ? 3*len:1))==NULL )
	return( NULL );
    fseek(ttf,offset,SEEK_SET);
    for ( i=0; i<len; ++i ) {
	uch = getushort(ttf);
	if ( uch>=0xd800 && uch<0xdc00 ) {
	    /* Is this a possible utf16 surrogate value? */
	    uch2 = getushort(ttf);
	    if ( uch2>=0xdc00 && uch2<0xe000 )
		uch = ((uch-0xd800)<<10) | (uch2&0x3ff);
	    else {
		pt = utf8_idpb(pt,uch,0);
		uch = uch2;
	    }
	}
	pt = utf8_idpb(pt,uch,0);
    }
    *pt++ = 0;
return( realloc(str,pt-str) );
}

static char *pfed_read_utf8_len(FILE *ttf,uint32 offset,int len) {
    char *pt, *str;
    int i;

    if ( len<0 )
return( NULL );

    pt = str = malloc(len+1);
    fseek(ttf,offset,SEEK_SET);
    for ( i=0; i<len; ++i )
	*pt++ = getc(ttf);
    *pt = '\0';
return( str );
}

static void pfed_readcvtcomments(FILE *ttf,struct ttfinfo *info,uint32 base ) {
    int count, i;
    uint16 *offsets;

    fseek(ttf,base,SEEK_SET);
    if ( getushort(ttf)!=0 )
return;			/* Bad version number */
    count = getushort(ttf);

    offsets = malloc(count*sizeof(uint16));
    info->cvt_names = malloc((count+1)*sizeof(char *));
    for ( i=0; i<count; ++i )
	offsets[i] = getushort(ttf);
    for ( i=0; i<count; ++i ) {
	if ( offsets[i]==0 )
	    info->cvt_names[i] = NULL;
	else
	    info->cvt_names[i] = pfed_read_utf8(ttf,base+offsets[i]);
    }
    free(offsets);
}

static void pfed_readglyphcomments(FILE *ttf,struct ttfinfo *info,uint32 base) {
    int n, i, j;
    struct grange { int start, end; uint32 offset; } *grange;
    uint32 offset, next;
    int use_utf8;

    fseek(ttf,base,SEEK_SET);
    use_utf8 = getushort(ttf);
    if ( use_utf8!=0 && use_utf8!=1 )
return;			/* Bad version number */
    n = getushort(ttf);
    grange = malloc(n*sizeof(struct grange));
    for ( i=0; i<n; ++i ) {
	grange[i].start = getushort(ttf);
	grange[i].end = getushort(ttf);
	grange[i].offset = getlong(ttf);
	if ( grange[i].start>grange[i].end || grange[i].end>info->glyph_cnt ) {
	    LogError( _("Bad glyph range specified in glyph comment subtable of PfEd table\n") );
	    grange[i].start = 1; grange[i].end = 0;
	}
    }
    for ( i=0; i<n; ++i ) {
	for ( j=grange[i].start; j<=grange[i].end; ++j ) {
	    fseek( ttf,base+grange[i].offset+(j-grange[i].start)*sizeof(uint32),SEEK_SET);
	    offset = getlong(ttf);
	    next = getlong(ttf);
	    if ( use_utf8 )
		info->chars[j]->comment = pfed_read_utf8_len(ttf,base+offset,next-offset);
	    else
		info->chars[j]->comment = pfed_read_ucs2_len(ttf,base+offset,next-offset);
	    if ( info->chars[j]->comment == NULL )
		LogError(_("Invalid comment string (negative length?) in 'PfEd' table for glyph %s."),
			info->chars[j]->name );
	}
    }
    free(grange);
}

static void pfed_readcolours(FILE *ttf,struct ttfinfo *info,uint32 base) {
    int n, i, j, start, end;
    uint32 col;

    fseek(ttf,base,SEEK_SET);
    if ( getushort(ttf)!=0 )
return;			/* Bad version number */
    n = getushort(ttf);
    for ( i=0; i<n; ++i ) {
	start = getushort(ttf);
	end = getushort(ttf);
	col = getlong(ttf);
	if ( start>end || end>info->glyph_cnt )
	    LogError( _("Bad glyph range specified in color subtable of PfEd table\n") );
	else {
	    for ( j=start; j<=end; ++j )
		info->chars[j]->color = col;
	}
    }
}

static void pfed_readlookupnames(FILE *ttf,struct ttfinfo *info,uint32 base,
	OTLookup *lookups) {
    OTLookup *otl;
    struct lookup_subtable *sub;
    AnchorClass *ac;
    int i, j, k, n, s, a;
    struct lstruct { int name_off, subs_off; } *ls, *ss, *as;

    fseek(ttf,base,SEEK_SET);
    if ( getushort(ttf)!=0 )
return;			/* Bad version number */
    n = getushort(ttf);
    ls = malloc(n*sizeof(struct lstruct));
    for ( i=0; i<n; ++i ) {
	ls[i].name_off = getushort(ttf);
	ls[i].subs_off = getushort(ttf);
    }
    for ( i=0, otl=lookups; i<n && otl!=NULL; ++i, otl=otl->next ) {
	if ( ls[i].name_off!=0 ) {
	    free( otl->lookup_name );
	    otl->lookup_name = pfed_read_utf8(ttf,base+ls[i].name_off);
	}
	if ( ls[i].subs_off!=0 ) {
	    fseek(ttf,base+ls[i].subs_off,SEEK_SET);
	    s = getushort(ttf);
	    ss = malloc(s*sizeof(struct lstruct));
	    for ( j=0; j<s; ++j ) {
		ss[j].name_off = getushort(ttf);
		ss[j].subs_off = getushort(ttf);
	    }
	    for ( j=0, sub=otl->subtables; j<s && sub!=NULL; ++j, sub=sub->next ) {
		if ( ss[j].name_off!=0 ) {
		    free( sub->subtable_name );
		    sub->subtable_name = pfed_read_utf8(ttf,base+ss[j].name_off);
		}
		if ( ss[j].subs_off!=0 ) {
		    if ( !sub->anchor_classes )
			LogError(_("Whoops, attempt to name anchors in a subtable which doesn't contain any\n"));
		    else {
			fseek(ttf,base+ss[j].subs_off,SEEK_SET);
			a = getushort(ttf);
			as = malloc(a*sizeof(struct lstruct));
			for ( k=0; k<a; ++k ) {
			    as[k].name_off = getushort(ttf);
			}
			k=0;
			for ( ac=info->ahead; ac!=NULL; ac=ac->next ) {
			    if ( ac->subtable==sub ) {
				if ( as[k].name_off!=0 ) {
				    free( ac->name );
				    ac->name = pfed_read_utf8(ttf,base+as[k].name_off);
				}
			        ++k;
			    }
			}
			free(as);
		    }
		}
	    }
	    /* I guess it's ok for some subtables to be unnamed, so no check for sub!=NULL */
	    if ( j<s )
		LogError(_("Whoops, more names than subtables of lookup %s\n"), otl->lookup_name );
	    free(ss);
	}
    }
    /* I guess it's ok for some lookups to be unnamed, so no check for otf!=NULL */
    if ( i<n )
	LogError(_("Whoops, more names than lookups\n") );
    free(ls);
}

static float pfed_get_coord(FILE *ttf,int mod) {
    if ( mod==V_B )
return( (float) (signed char) getc(ttf) );
    else if ( mod==V_S )
return( (float) (short) getushort(ttf));
    else if ( mod==V_F )
return( getlong(ttf)/256.0 );
    else {
	LogError(_("Bad data type in contour verb in 'PfEd'\n"));
return( 0 );
    }
}

static void pfed_read_normal_contour(FILE *ttf,SplineSet *ss,
	uint32 base, int type) {
    SplinePoint *sp, *current;
    int verb, v, m;
    float offx, offy, offx1, offy1, offx2, offy2;
    int was_implicit=false;

    fseek(ttf,base,SEEK_SET);

    verb = getc(ttf);
    if ( COM_VERB(verb)!=V_MoveTo ) {
	LogError(_("Whoops, contours must begin with a move to\n") );
	ss->first = ss->last = SplinePointCreate(0,0);
	ss->start_offset = 0;
return;
    }
    offx = pfed_get_coord(ttf,COM_MOD(verb));
    offy = pfed_get_coord(ttf,COM_MOD(verb));
    ss->first = current = SplinePointCreate(offx,offy);
    ss->start_offset = 0;
    for (;;) {
	verb = getc(ttf);
	v = COM_VERB(verb); m = COM_MOD(verb);
	if ( m==3 ) {
	    LogError(_("Bad data modifier in contour command in 'PfEd'\n") );
    break;
	}
	if ( verb==V_Close || verb==V_End )
    break;
	else if ( v>=V_LineTo && v<=V_VLineTo ) {
	    offx = offy = 0;
	    if ( v==V_LineTo ) {
		offx = pfed_get_coord(ttf,m);
		offy = pfed_get_coord(ttf,m);
	    } else if ( v==V_HLineTo )
		offx = pfed_get_coord(ttf,m);
	    else if ( v==V_VLineTo )
		offy = pfed_get_coord(ttf,m);
	    sp = SplinePointCreate(current->me.x+offx,current->me.y+offy);
	} else if ( v>=V_QCurveTo && v<=V_QVImplicit ) {
	    int will_be_implicit = true;
	    offx = offy = 0; offx1 = offy1 = 1;	/* else implicit points become straight lines too soon */
	    if ( v==V_QCurveTo ) {
		offx = pfed_get_coord(ttf,m);
		offy = pfed_get_coord(ttf,m);
		offx1 = pfed_get_coord(ttf,m);
		offy1 = pfed_get_coord(ttf,m);
		will_be_implicit = false;
	    } else if ( v==V_QImplicit ) {
		offx = pfed_get_coord(ttf,m);
		offy = pfed_get_coord(ttf,m);
	    } else if ( v==V_QHImplicit ) {
		offx = pfed_get_coord(ttf,m);
	    } else if ( v==V_QVImplicit ) {
		offy = pfed_get_coord(ttf,m);
	    }

	    current->nextcp.x = current->me.x+offx;
	    current->nextcp.y = current->me.y+offy;
	    current->nonextcp = false;
	    sp = SplinePointCreate(current->nextcp.x+offx1,current->nextcp.y+offy1);
	    sp->prevcp = current->nextcp;
	    sp->noprevcp = false;
	    if ( was_implicit ) {
		current->me.x = (current->prevcp.x + current->nextcp.x)/2;
		current->me.y = (current->prevcp.y + current->nextcp.y)/2;
		SplineRefigure(current->prev);
	    }
	    was_implicit = will_be_implicit;
	} else if ( v>=V_CurveTo && v<=V_HVCurveTo ) {
	    offx=offy=offx2=offy2=0;
	    if ( v==V_CurveTo ) {
		offx = pfed_get_coord(ttf,m);
		offy = pfed_get_coord(ttf,m);
		offx1 = pfed_get_coord(ttf,m);
		offy1 = pfed_get_coord(ttf,m);
		offx2 = pfed_get_coord(ttf,m);
		offy2 = pfed_get_coord(ttf,m);
	    } else if ( v==V_VHCurveTo ) {
		offy = pfed_get_coord(ttf,m);
		offx1 = pfed_get_coord(ttf,m);
		offy1 = pfed_get_coord(ttf,m);
		offx2 = pfed_get_coord(ttf,m);
	    } else if ( v==V_HVCurveTo ) {
		offx = pfed_get_coord(ttf,m);
		offx1 = pfed_get_coord(ttf,m);
		offy1 = pfed_get_coord(ttf,m);
		offy2 = pfed_get_coord(ttf,m);
	    }
	    current->nextcp.x = current->me.x+offx;
	    current->nextcp.y = current->me.y+offy;
	    current->nonextcp = false;
	    sp = SplinePointCreate(current->nextcp.x+offx1+offx2,current->nextcp.y+offy1+offy2);
	    sp->prevcp.x = current->nextcp.x+offx1;
	    sp->prevcp.y = current->nextcp.y+offy1;
	    sp->noprevcp = false;
	} else {
	    LogError(_("Whoops, unexpected verb in contour %d.%d\n"), v, m );
    break;
	}
	SplineMake(current,sp,type==2);
	current = sp;
    }
    if ( verb==V_Close ) {
	if ( was_implicit ) {
	    current->me.x = (current->prevcp.x + ss->first->nextcp.x)/2;
	    current->me.y = (current->prevcp.y + ss->first->nextcp.y)/2;
	}
	if ( current->me.x==ss->first->me.x && current->me.y==ss->first->me.y ) {
	    current->prev->to = ss->first;
	    ss->first->prev = current->prev;
	    ss->first->prevcp = current->prevcp;
	    ss->first->noprevcp = current->noprevcp;
	    SplinePointFree(current);
	} else
	    SplineMake(current,ss->first,type==2);
	ss->last = ss->first;
    } else {
	ss->last = current;
    }
    SPLCategorizePoints(ss);
}

static void pfed_read_spiro_contour(FILE *ttf,SplineSet *ss,
	uint32 base, int type) {
    int i, ch;

    fseek(ttf,base,SEEK_SET);

    for ( i=0; ; ) {
	ch = getc(ttf);
	if ( ch!=SPIRO_OPEN_CONTOUR && ch!=SPIRO_CORNER && ch!=SPIRO_G4 &&
		ch!=SPIRO_G2 && ch!=SPIRO_LEFT && ch!=SPIRO_RIGHT &&
		ch!=SPIRO_END && ch!=SPIRO_CLOSE_CONTOUR ) {
	    LogError(_("Whoops, bad spiro command %d\n"), ch);
    break;
	}
	if ( ss->spiro_cnt>=ss->spiro_max )
	    ss->spiros = realloc(ss->spiros,(ss->spiro_max+=10)*sizeof(spiro_cp));
	ss->spiros[ss->spiro_cnt].ty = ch;
	if ( ch!=SPIRO_END ) {
	    ss->spiros[ss->spiro_cnt].x = getlong(ttf)/256.0;
	    ss->spiros[ss->spiro_cnt].y = getlong(ttf)/256.0;
	} else {
	    ss->spiros[ss->spiro_cnt].x = 0;
	    ss->spiros[ss->spiro_cnt].y = 0;
	}
	++(ss->spiro_cnt);
	if ( ch==SPIRO_END || ch=='}' )
    break;
    }
    if ( ss->spiro_cnt!=0 && ss->spiros[ss->spiro_cnt-1].ty!= SPIRO_END ) {
	if ( ss->spiros[ss->spiro_cnt-1].ty==SPIRO_CLOSE_CONTOUR )
	    ss->spiros[ss->spiro_cnt-1].ty = SPIRO_G4;
	if ( ss->spiro_cnt>=ss->spiro_max )
	    ss->spiros = realloc(ss->spiros,(ss->spiro_max+=2)*sizeof(spiro_cp));
	ss->spiros[ss->spiro_cnt].ty = SPIRO_END;
	ss->spiros[ss->spiro_cnt].x = 0;
	ss->spiros[ss->spiro_cnt].y = 0;
    }
}

static void pfed_read_glyph_layer(FILE *ttf,struct ttfinfo *info,Layer *ly,
	uint32 base, int type, int version) {
    int cc, ic, rc, i, j;
    SplineSet *ss;
    struct contours { int data_off, name_off; SplineSet *ss; } *contours;
    int gid;
    RefChar *last, *cur;

    fseek(ttf,base,SEEK_SET);
    cc = getushort(ttf);		/* Contours */
    rc = 0;
    if ( version==1 )
	rc = getushort(ttf);		/* References */
    ic = getushort(ttf);		/* Images */
    contours = malloc(cc*sizeof(struct contours));
    for ( i=0; i<cc; ++i ) {
	contours[i].data_off = getushort(ttf);
	contours[i].name_off = getushort(ttf);
    }
    last = NULL;
    for ( i=0; i<rc; ++i ) {
	cur = RefCharCreate();
	for ( j=0; j<6; ++j )
	    cur->transform[j] = getlong(ttf)/32768.0;
	gid = getushort(ttf);
	if ( gid>=info->glyph_cnt ) {
	    LogError(_("Bad glyph reference in layer info.\n"));
    break;
	}
	cur->sc = info->chars[gid];
	cur->orig_pos = gid;
	cur->unicode_enc = cur->sc->unicodeenc;
	if ( last==NULL )
	    ly->refs = cur;
	else
	    last->next = cur;
	last = cur;
    }

    ss = ly->splines;			/* Only relevant for spiros where they live in someone else's layer */
    for ( i=0; i<cc; ++i ) {
	if ( type!=1 ) {		/* Not spiros */
	    contours[i].ss = chunkalloc(sizeof(SplineSet));
	    if ( i==0 )
		ly->splines = contours[i].ss;
	    else
		contours[i-1].ss->next = contours[i].ss;
	    if ( contours[i].name_off!=0 )
		contours[i].ss->contour_name = pfed_read_utf8(ttf,base+contours[i].name_off);
	    pfed_read_normal_contour(ttf,contours[i].ss,base+contours[i].data_off,type);
	} else {			/* Spiros are actually bound to an already existing layer and don't have an independent existance yet */
	    contours[i].ss = ss;
	    if ( ss!=NULL ) {
		pfed_read_spiro_contour(ttf,ss,base+contours[i].data_off,type);
		ss = ss->next;
	    } else
		LogError(_("Whoops, Ran out of spiros\n"));
	}
    }
    free(contours);
}

static void pfed_readguidelines(FILE *ttf,struct ttfinfo *info,uint32 base) {
    int i,v,h,off;
    int version;
    SplinePoint *sp, *nsp;
    SplineSet *ss;

    fseek(ttf,base,SEEK_SET);
    version = getushort(ttf);
    if ( version>1 )
return;			/* Bad version number */
    v = getushort(ttf);
    h = getushort(ttf);
    (void) getushort(ttf);
    off = getushort(ttf);

    if ( off!=0 ) {
	pfed_read_glyph_layer(ttf,info,&info->guidelines,base+off,info->to_order2?2:3,version);
    } else {
	struct npos { int pos; int offset; } *vs, *hs;
	vs = malloc(v*sizeof(struct npos));
	hs = malloc(h*sizeof(struct npos));
	for ( i=0; i<v; ++i ) {
	    vs[i].pos = (short) getushort(ttf);
	    vs[i].offset = getushort(ttf);
	}
	for ( i=0; i<h; ++i ) {
	    hs[i].pos = (short) getushort(ttf);
	    hs[i].offset = getushort(ttf);
	}
	for ( i=0; i<v; ++i ) {
	    sp = SplinePointCreate(vs[i].pos,-info->emsize);
	    nsp = SplinePointCreate(vs[i].pos,2*info->emsize);
	    SplineMake(sp,nsp,info->to_order2);
	    ss = chunkalloc(sizeof(SplineSet));
	    ss->first = sp; ss->last = nsp;
	    if ( vs[i].offset!=0 )
		ss->contour_name = pfed_read_utf8(ttf,base+vs[i].offset);
	    ss->next = info->guidelines.splines;
	    info->guidelines.splines = ss;
	}
	for ( i=0; i<h; ++i ) {
	    sp = SplinePointCreate(-info->emsize,hs[i].pos);
	    nsp = SplinePointCreate(2*info->emsize,hs[i].pos);
	    SplineMake(sp,nsp,info->to_order2);
	    ss = chunkalloc(sizeof(SplineSet));
	    ss->first = sp; ss->last = nsp;
	    if ( hs[i].offset!=0 )
		ss->contour_name = pfed_read_utf8(ttf,base+hs[i].offset);
	    ss->next = info->guidelines.splines;
	    info->guidelines.splines = ss;
	}
	SPLCategorizePoints(info->guidelines.splines);
	free(vs); free(hs);
    }
}

static void pfed_redo_refs(SplineChar *sc,int layer) {
    RefChar *refs;

    sc->ticked = true;
    for ( refs=sc->layers[layer].refs; refs!=NULL; refs=refs->next ) {
	if ( layer==1 && refs->sc==NULL )	/* If main layer has spiros attached, then we'll get here. Any refs will come from the main ttf reading routines and won't be fixed up yet */
    continue;
	if ( !refs->sc->ticked )
	    pfed_redo_refs(refs->sc,layer);
	SCReinstanciateRefChar(sc,refs,layer);
    }
}

static void pfed_read_layer(FILE *ttf,struct ttfinfo *info,int layer,int type, uint32 base,
	uint32 start,int version) {
    uint32 *loca = calloc(info->glyph_cnt,sizeof(uint32));
    int i,j;
    SplineChar *sc;
    int rcnt;
    struct range { int start, last; uint32 offset; } *ranges;

    fseek(ttf,start,SEEK_SET);
    rcnt = getushort(ttf);
    ranges = malloc(rcnt*sizeof(struct range));
    for ( i=0; i<rcnt; ++i ) {
	ranges[i].start  = getushort(ttf);
	ranges[i].last   = getushort(ttf);
	ranges[i].offset = getlong(ttf);
    }
    for ( i=0; i<rcnt; ++i ) {
	fseek(ttf,base+ranges[i].offset,SEEK_SET);
	for ( j=ranges[i].start; j<=ranges[i].last; ++j )
	    loca[j] = getlong(ttf);
	for ( j=ranges[i].start; j<=ranges[i].last; ++j ) {
	    Layer *ly;
	    sc = info->chars[j];
	    ly = &sc->layers[layer];
	    if ( loca[j]!=0 )
		pfed_read_glyph_layer(ttf,info,ly,base+loca[j],type,version);
	}
    }
    free(ranges); free(loca);

    for ( i=0; i<info->glyph_cnt; ++i ) if ( info->chars[i]!=NULL )
	info->chars[i]->ticked = false;
    for ( i=0; i<info->glyph_cnt; ++i ) if ( info->chars[i]!=NULL )
	pfed_redo_refs(info->chars[i],layer);
}

static void pfed_readotherlayers(FILE *ttf,struct ttfinfo *info,uint32 base) {
    int i, l, lcnt, spiro_index, gid;
    int version;
    struct layer_info { int type, name_off, data_off, sf_layer; char *name; } *layers;
    int non_spiro_cnt=0;
    SplineChar *sc;

    fseek(ttf,base,SEEK_SET);
    version = getushort(ttf);
    if ( version>1 )
return;			/* Bad version number */
    lcnt = getushort(ttf);
    layers = malloc(lcnt*sizeof(struct layer_info));
    for ( i=0; i<lcnt; ++i ) {
	layers[i].type     = getushort(ttf);
	layers[i].name_off = getushort(ttf);
	layers[i].data_off = getlong(ttf);
	layers[i].sf_layer = -1;
    }
    spiro_index = -1;
    non_spiro_cnt = 0;
    for ( i=0; i<lcnt; ++i ) {
	if ( layers[i].name_off==0 )
	    layers[i].name = copy("Unnamed");
	else {
	    layers[i].name = pfed_read_utf8(ttf,base+layers[i].name_off);
	    if ( layers[i].type==1 && strcmp(layers[i].name,"Spiro")==0 )
		spiro_index = i;
	}
	if ( layers[i].type==2 || layers[i].type==3 || layers[i].type==0x102 || layers[i].type==0x103 )
	    ++non_spiro_cnt;
    }
    if ( spiro_index==-1 ) {
	for ( i=0; i<lcnt; ++i )
	    if ( layers[i].type==1 ) {
		spiro_index=i;
	break;
	    }
    }

    if ( non_spiro_cnt!=0 ) {
	info->layer_cnt = non_spiro_cnt+1;
	info->layers = calloc(info->layer_cnt+1,sizeof(LayerInfo));
	info->layers[ly_back].background = true;
	info->layers[ly_fore].order2 = info->to_order2;
	info->layers[ly_fore].background = false;
	l = i = 0;
	if ( (layers[i].type&0xff)==1 )
	    ++i;
	if ( layers[i].type&0x100 ) {
	    /* first layer output is foreground, so it can't replace the background layer */
	    ++info->layer_cnt;
	    l = 2;
	    info->layers[ly_back].order2 = info->to_order2;
	}
	for ( ; i<lcnt; ++i ) if ( (layers[i].type&0xff)==2 || (layers[i].type&0xff)==3 ) {
	    info->layers[l].name   = layers[i].name;
	    layers[i].name = NULL;
	    layers[i].sf_layer = l;
	    info->layers[l].order2 = (layers[i].type&0xff)==2;
	    info->layers[l].background = (layers[i].type&0x100)?0:1;
	    if ( l==0 ) l=2; else ++l;
	}
	if ( info->layer_cnt!=2 ) {
	    for ( gid = 0; gid<info->glyph_cnt; ++gid ) if ((sc=info->chars[gid])!=NULL ) {
		sc->layers = realloc(sc->layers,info->layer_cnt*sizeof(Layer));
		memset(sc->layers+2,0,(info->layer_cnt-2)*sizeof(Layer));
		sc->layer_cnt = info->layer_cnt;
	    }
	}
    }
    if ( spiro_index!=-1 )
	pfed_read_layer(ttf,info,ly_fore,layers[spiro_index].type,base,base+layers[spiro_index].data_off,version);
    for ( i=0; i<lcnt; ++i ) if ( layers[i].sf_layer!=-1 ) {
	pfed_read_layer(ttf,info,layers[i].sf_layer,layers[i].type&0xff,
		base,base+layers[i].data_off,version);
    }
    for ( i=0; i<lcnt; ++i )
	free( layers[i].name );
    free( layers );
}

void pfed_read(FILE *ttf,struct ttfinfo *info) {
    int n,i;
    struct tagoff { uint32 tag, offset; } tagoff[MAX_SUBTABLE_TYPES+30];

    fseek(ttf,info->pfed_start,SEEK_SET);

    if ( getlong(ttf)!=0x00010000 )
return;
    n = getlong(ttf);
    if ( n>=MAX_SUBTABLE_TYPES+30 )
	n = MAX_SUBTABLE_TYPES+30;
    for ( i=0; i<n; ++i ) {
	tagoff[i].tag = getlong(ttf);
	tagoff[i].offset = getlong(ttf);
    }
    for ( i=0; i<n; ++i ) switch ( tagoff[i].tag ) {
      case fcmt_TAG: case flog_TAG:
	pfed_readfontcomment(ttf,info,info->pfed_start+tagoff[i].offset, tagoff[i].tag);
      break;
      case cvtc_TAG:
	pfed_readcvtcomments(ttf,info,info->pfed_start+tagoff[i].offset);
      break;
      case cmnt_TAG:
	pfed_readglyphcomments(ttf,info,info->pfed_start+tagoff[i].offset);
      break;
      case colr_TAG:
	pfed_readcolours(ttf,info,info->pfed_start+tagoff[i].offset);
      break;
      case GPOS_TAG:
	pfed_readlookupnames(ttf,info,info->pfed_start+tagoff[i].offset,info->gpos_lookups);
      break;
      case GSUB_TAG:
	pfed_readlookupnames(ttf,info,info->pfed_start+tagoff[i].offset,info->gsub_lookups);
      break;
      case layr_TAG:
	pfed_readotherlayers(ttf,info,info->pfed_start+tagoff[i].offset);
      break;
      case guid_TAG:
	pfed_readguidelines(ttf,info,info->pfed_start+tagoff[i].offset);
      break;
      default:
	LogError( _("Unknown subtable '%c%c%c%c' in 'PfEd' table, ignored\n"),
		tagoff[i].tag>>24, (tagoff[i].tag>>16)&0xff, (tagoff[i].tag>>8)&0xff, tagoff[i].tag&0xff );
      break;
    }
}

/* 'TeX ' table format is as follows...				 */
/* uint32  version number 0x00010000				 */
/* uint32  subtable count					 */
/* struct { uint32 tab, offset } tag/offset for first subtable	 */
/* struct { uint32 tab, offset } tag/offset for second subtable	 */
/* ...								 */

/* 'TeX ' 'ftpm' font parameter subtable format			 */
/*  short version number 0					 */
/*  parameter count						 */
/*  array of { 4chr tag, value }				 */

/* 'TeX ' 'htdp' per-glyph height/depth subtable format		 */
/*  short  version number 0					 */
/*  short  glyph-count						 */
/*  array[glyph-count] of { int16 height,depth }		 */

/* 'TeX ' 'itlc' per-glyph italic correction subtable		 */
/*  short  version number 0					 */
/*  short  glyph-count						 */
/*  array[glyph-count] of int16 italic_correction		 */

/* !!!!!!!!!!! OBSOLETE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/* 'TeX ' 'sbsp' per-glyph sub/super script positioning subtable */
/*  short  version number 0					 */
/*  short  glyph-count						 */
/*  array[glyph-count] of { int16 sub,super }			 */

#undef MAX_SUBTABLE_TYPES
#define MAX_SUBTABLE_TYPES	4

struct TeX_subtabs {
    int next;
    struct {
	FILE *data;
	uint32 tag;
	uint32 offset;
    } subtabs[MAX_SUBTABLE_TYPES];
};

static uint32 tex_text_params[] = {
    TeX_Slant,
    TeX_Space,
    TeX_Stretch,
    TeX_Shrink,
    TeX_XHeight,
    TeX_Quad,
    TeX_ExtraSp,
    0
};
static uint32 tex_math_params[] = {
    TeX_Slant,
    TeX_Space,
    TeX_Stretch,
    TeX_Shrink,
    TeX_XHeight,
    TeX_Quad,
    TeX_MathSp,
    TeX_Num1,
    TeX_Num2,
    TeX_Num3,
    TeX_Denom1,
    TeX_Denom2,
    TeX_Sup1,
    TeX_Sup2,
    TeX_Sup3,
    TeX_Sub1,
    TeX_Sub2,
    TeX_SupDrop,
    TeX_SubDrop,
    TeX_Delim1,
    TeX_Delim2,
    TeX_AxisHeight,
    0};
static uint32 tex_mathext_params[] = {
    TeX_Slant,
    TeX_Space,
    TeX_Stretch,
    TeX_Shrink,
    TeX_XHeight,
    TeX_Quad,
    TeX_MathSp,
    TeX_DefRuleThick,
    TeX_BigOpSpace1,
    TeX_BigOpSpace2,
    TeX_BigOpSpace3,
    TeX_BigOpSpace4,
    TeX_BigOpSpace5,
    0};

/* ************************************************************************** */
/* *************************    The 'TeX ' table    ************************* */
/* *************************         Output         ************************* */
/* ************************************************************************** */

static void TeX_dumpFontParams(SplineFont *sf, struct TeX_subtabs *tex, struct alltabs *at ) {
    FILE *fprm;
    int i,pcnt;
    uint32 *tags;

    if ( sf->texdata.type==tex_unset )
return;
    tex->subtabs[tex->next].tag = CHR('f','t','p','m');
    tex->subtabs[tex->next++].data = fprm = tmpfile();

    putshort(fprm,0);			/* sub-table version number */
    pcnt = sf->texdata.type==tex_math ? 22 : sf->texdata.type==tex_mathext ? 13 : 7;
    tags = sf->texdata.type==tex_math ? tex_math_params :
	    sf->texdata.type==tex_mathext ? tex_mathext_params :
					    tex_text_params;
    putshort(fprm,pcnt);
    for ( i=0; i<pcnt; ++i ) {
	putlong(fprm,tags[i]);
	putlong(fprm,sf->texdata.params[i]);
    }
    /* always aligned */
}

static void TeX_dumpHeightDepth(SplineFont *sf, struct TeX_subtabs *tex, struct alltabs *at ) {
    FILE *htdp;
    int i,j,k,last_g, gid;
    DBounds b;

    for ( i=at->gi.gcnt-1; i>=0; --i ) {
	gid = at->gi.bygid[i];
	if ( gid!=-1 && sf->glyphs[gid]!=NULL &&
		(sf->glyphs[gid]->tex_height!=TEX_UNDEF || sf->glyphs[gid]->tex_depth!=TEX_UNDEF))
    break;
    }
    if ( i<0 )		/* No height/depth info */
return;

    tex->subtabs[tex->next].tag = CHR('h','t','d','p');
    tex->subtabs[tex->next++].data = htdp = tmpfile();

    putshort(htdp,0);				/* sub-table version number */
    putshort(htdp,sf->glyphs[gid]->ttf_glyph+1);/* data for this many glyphs */

    last_g = -1;
    for ( j=0; j<=i; ++j ) {
	gid = at->gi.bygid[j];
	if ( gid!=-1 && sf->glyphs[gid]!=NULL ) {
	    SplineChar *sc = sf->glyphs[gid];
	    for ( k=last_g+1; k<sc->ttf_glyph; ++k ) {
		putshort(htdp,0);
		putshort(htdp,0);
	    }
	    if ( sc->tex_depth==TEX_UNDEF || sc->tex_height==TEX_UNDEF )
		SplineCharFindBounds(sc,&b);
	    putshort( htdp, sc->tex_height==TEX_UNDEF ? b.maxy : sc->tex_height );
	    putshort( htdp, sc->tex_depth==TEX_UNDEF ? -b.miny : sc->tex_depth );
	    last_g = sc->ttf_glyph;
	}
    }
    /* always aligned */
}

static void TeX_dumpItalicCorr(SplineFont *sf, struct TeX_subtabs *tex, struct alltabs *at ) {
    FILE *itlc;
    int i,j,k,last_g, gid;

    for ( i=at->gi.gcnt-1; i>=0; --i ) {
	gid = at->gi.bygid[i];
	if ( gid!=-1 && sf->glyphs[gid]!=NULL &&
		sf->glyphs[gid]->italic_correction!=TEX_UNDEF )
    break;
    }
    if ( i<0 )		/* No italic_correction info */
return;

    tex->subtabs[tex->next].tag = CHR('i','t','l','c');
    tex->subtabs[tex->next++].data = itlc = tmpfile();

    putshort(itlc,0);				/* sub-table version number */
    putshort(itlc,sf->glyphs[gid]->ttf_glyph+1);/* data for this many glyphs */

    last_g = -1;
    for ( j=0; j<=i; ++j ) {
	gid = at->gi.bygid[j];
	if ( gid!=-1 && sf->glyphs[gid]!=NULL ) {
	    SplineChar *sc = sf->glyphs[gid];
	    for ( k=last_g+1; k<sc->ttf_glyph; ++k ) {
		putshort(itlc,0);
		putshort(itlc,0);
	    }
	    putshort( itlc, sc->italic_correction!=TEX_UNDEF ? sc->italic_correction :
				    0 );
	    last_g = sc->ttf_glyph;
	}
    }
    /* always aligned */
}

void tex_dump(struct alltabs *at, SplineFont *sf) {
    struct TeX_subtabs tex;
    FILE *file;
    int i;
    uint32 offset;

    if ( !(at->gi.flags & ttf_flag_TeXtable ))
return;

    memset(&tex,0,sizeof(tex));
    TeX_dumpFontParams(sf,&tex,at);
    TeX_dumpHeightDepth(sf,&tex,at);
    TeX_dumpItalicCorr(sf,&tex,at);

    if ( tex.next==0 )
return;		/* No subtables */

    at->tex = file = tmpfile();
    putlong(file, 0x00010000);		/* Version number */
    putlong(file, tex.next);		/* sub-table count */
    offset = 2*sizeof(uint32) + 2*tex.next*sizeof(uint32);
    for ( i=0; i<tex.next; ++i ) {
	putlong(file,tex.subtabs[i].tag);
	putlong(file,offset);
	fseek(tex.subtabs[i].data,0,SEEK_END);
	tex.subtabs[i].offset = offset;
	offset += ftell(tex.subtabs[i].data);
    }
    for ( i=0; i<tex.next; ++i ) {
	fseek(tex.subtabs[i].data,0,SEEK_SET);
	ttfcopyfile(file,tex.subtabs[i].data,tex.subtabs[i].offset,"TeX-subtable");
    }
    if ( ftell(file)&2 )
	putshort(file,0);
    if ( ftell(file)&3 )
	IError("'TeX ' table not properly aligned");
    at->texlen = ftell(file);
}

/* *************************    The 'TeX ' table    ************************* */
/* *************************          Input         ************************* */

static void TeX_readFontParams(FILE *ttf,struct ttfinfo *info,uint32 base) {
    int i,pcnt;
    static uint32 *alltags[] = { tex_text_params, tex_math_params, tex_mathext_params };
    int j,k;
    uint32 tag;
    int32 val;

    fseek(ttf,base,SEEK_SET);
    if ( getushort(ttf)!=0 )	/* Don't know how to read this version of the subtable */
return;
    pcnt = getushort(ttf);
    if ( pcnt==22 ) info->texdata.type = tex_math;
    else if ( pcnt==13 ) info->texdata.type = tex_mathext;
    else if ( pcnt>=7 ) info->texdata.type = tex_text;
    for ( i=0; i<pcnt; ++i ) {
	tag = getlong(ttf);
	val = getlong(ttf);
	for ( j=0; j<3; ++j ) {
	    for ( k=0; alltags[j][k]!=0; ++k )
		if ( alltags[j][k]==tag )
	    break;
	    if ( alltags[j][k]==tag )
	break;
	}
	if ( j<3 )
	    info->texdata.params[k] = val;
    }
}

static void TeX_readHeightDepth(FILE *ttf,struct ttfinfo *info,uint32 base) {
    int i,gcnt;

    fseek(ttf,base,SEEK_SET);
    if ( getushort(ttf)!=0 )	/* Don't know how to read this version of the subtable */
return;
    gcnt = getushort(ttf);
    for ( i=0; i<gcnt && i<info->glyph_cnt; ++i ) {
	int h, d;
	h = getushort(ttf);
	d = getushort(ttf);
	if ( info->chars[i]!=NULL ) {
	    info->chars[i]->tex_height = h;
	    info->chars[i]->tex_depth = d;
	}
    }
}

static void TeX_readItalicCorr(FILE *ttf,struct ttfinfo *info,uint32 base) {
    int i,gcnt;

    fseek(ttf,base,SEEK_SET);
    if ( getushort(ttf)!=0 )	/* Don't know how to read this version of the subtable */
return;
    gcnt = getushort(ttf);
    for ( i=0; i<gcnt && i<info->glyph_cnt; ++i ) {
	int ital;
	ital = getushort(ttf);
	if ( info->chars[i]!=NULL ) {
	    info->chars[i]->italic_correction = ital;
	}
    }
}

void tex_read(FILE *ttf,struct ttfinfo *info) {
    int n,i;
    struct tagoff { uint32 tag, offset; } tagoff[MAX_SUBTABLE_TYPES+30];

    fseek(ttf,info->tex_start,SEEK_SET);

    if ( getlong(ttf)!=0x00010000 )
return;
    n = getlong(ttf);
    if ( n>=MAX_SUBTABLE_TYPES+30 )
	n = MAX_SUBTABLE_TYPES+30;
    for ( i=0; i<n; ++i ) {
	tagoff[i].tag = getlong(ttf);
	tagoff[i].offset = getlong(ttf);
    }
    for ( i=0; i<n; ++i ) switch ( tagoff[i].tag ) {
      case CHR('f','t','p','m'):
	TeX_readFontParams(ttf,info,info->tex_start+tagoff[i].offset);
      break;
      case CHR('h','t','d','p'):
	TeX_readHeightDepth(ttf,info,info->tex_start+tagoff[i].offset);
      break;
      case CHR('i','t','l','c'):
	TeX_readItalicCorr(ttf,info,info->tex_start+tagoff[i].offset);
      break;
      default:
	LogError( _("Unknown subtable '%c%c%c%c' in 'TeX ' table, ignored\n"),
		tagoff[i].tag>>24, (tagoff[i].tag>>16)&0xff, (tagoff[i].tag>>8)&0xff, tagoff[i].tag&0xff );
      break;
    }
}

/* ************************************************************************** */
/* *************************    The 'BDF ' table    ************************* */
/* *************************         Output         ************************* */
/* ************************************************************************** */

/* the BDF table is used to store BDF properties so that we can do round trip */
/* conversion from BDF->otb->BDF without losing anything. */
/* Format:
	USHORT	version		: 'BDF' table version number, must be 0x0001
	USHORT	strikeCount	: number of strikes in table
	ULONG	stringTable	: offset (from start of BDF table) to string table

followed by an array of 'strikeCount' descriptors that look like:
	USHORT	ppem		: vertical pixels-per-EM for this strike
	USHORT	num_items	: number of items (properties and atoms), max is 255

this array is followed by 'strikeCount' value sets. Each "value set" is
an array of (num_items) items that look like:
	ULONG	item_name	: offset in string table to item name
	USHORT	item_type	: item type: 0 => non-property string (e.g. COMMENT)
					     1 => non-property atom (e.g. FONT)
					     2 => non-property int32
					     3 => non-property uint32
					  0x10 => flag for a property, ored
						  with above value types)
	ULONG	item_value	: item value.
				strings	 => an offset into the string table
					  to the corresponding string,
					  without the surrending double-quotes

				atoms	 => an offset into the string table

				integers => the corresponding 32-bit value
Then the string table of null terminated strings. These strings should be in
ASCII.
*/

/* Internally FF stores BDF comments as one psuedo property per line. As you */
/*  might expect. But FreeType merges them into one large lump with newlines */
/*  between lines. Which means that BDF tables created by FreeType will be in*/
/*  that format. So we might as well be compatible. We will pack & unpack    */
/*  comment lines */

static int BDFPropCntMergedComments(BDFFont *bdf) {
    int i, cnt;

    cnt = 0;
    for ( i=0; i<bdf->prop_cnt; ++i ) {
	++cnt;
	if ( strmatch(bdf->props[i].name,"COMMENT")==0 )
    break;
    }
    for ( ; i<bdf->prop_cnt; ++i ) {
	if ( strmatch(bdf->props[i].name,"COMMENT")==0 )
    continue;
	++cnt;
    }
return( cnt );
}

static char *MergeComments(BDFFont *bdf) {
    int len, i;
    char *str;

    len = 0;
    for ( i=0; i<bdf->prop_cnt; ++i ) {
	if ( strmatch(bdf->props[i].name,"COMMENT")==0 &&
		((bdf->props[i].type & ~prt_property)==prt_string ||
		 (bdf->props[i].type & ~prt_property)==prt_atom ))
	    len += strlen( bdf->props[i].u.atom ) + 1;
    }
    if ( len==0 )
return( copy( "" ));

    str = malloc( len+1 );
    len = 0;
    for ( i=0; i<bdf->prop_cnt; ++i ) {
	if ( strmatch(bdf->props[i].name,"COMMENT")==0 &&
		((bdf->props[i].type & ~prt_property)==prt_string ||
		 (bdf->props[i].type & ~prt_property)==prt_atom )) {
	    strcpy(str+len,bdf->props[i].u.atom );
	    len += strlen( bdf->props[i].u.atom ) + 1;
	    str[len-1] = '\n';
	}
    }
    str[len-1] = '\0';
return( str );
}

#define AMAX	50

int ttf_bdf_dump(SplineFont *sf,struct alltabs *at,int32 *sizes) {
    FILE *strings;
    struct atomoff { char *name; int pos; } atomoff[AMAX];
    int acnt=0;
    int spcnt = 0;
    int i,j,k;
    BDFFont *bdf;
    long pos;

    for ( i=0; sizes[i]!=0; ++i ) {
	for ( bdf=sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=(sizes[i]&0xffff) || BDFDepth(bdf)!=(sizes[i]>>16)); bdf=bdf->next );
	if ( bdf!=NULL && bdf->prop_cnt!=0 )
	    ++spcnt;
    }
    if ( spcnt==0 )	/* No strikes with properties */
return(true);

    at->bdf = tmpfile();
    strings = tmpfile();

    putshort(at->bdf,0x0001);
    putshort(at->bdf,spcnt);
    putlong(at->bdf,0);		/* offset to string table */

    for ( i=0; sizes[i]!=0; ++i ) {
	for ( bdf=sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=(sizes[i]&0xffff) || BDFDepth(bdf)!=(sizes[i]>>16)); bdf=bdf->next );
	if ( bdf!=NULL && bdf->prop_cnt!=0 ) {
	    putshort(at->bdf,bdf->pixelsize);
	    putshort(at->bdf,BDFPropCntMergedComments(bdf));
	}
    }

    for ( i=0; sizes[i]!=0; ++i ) {
	for ( bdf=sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=(sizes[i]&0xffff) || BDFDepth(bdf)!=(sizes[i]>>16)); bdf=bdf->next );
	if ( bdf!=NULL && bdf->prop_cnt!=0 ) {
	    int saw_comment=0;
	    char *str;
	    for ( j=0; j<bdf->prop_cnt; ++j ) {
		if ( strmatch(bdf->props[j].name,"COMMENT")==0 && saw_comment )
	    continue;
		/* Try to reuse keyword names in string space */
		for ( k=0 ; k<acnt; ++k ) {
		    if ( strcmp(atomoff[k].name,bdf->props[j].name)==0 )
		break;
		}
		if ( k>=acnt && k<AMAX ) {
		    atomoff[k].name = bdf->props[j].name;
		    atomoff[k].pos = ftell(strings);
		    ++acnt;
		    fwrite(atomoff[k].name,1,strlen(atomoff[k].name)+1,strings);
		}
		if ( k<acnt )
		    putlong(at->bdf,atomoff[k].pos);
		else {
		    putlong(at->bdf,ftell(strings));
		    fwrite(bdf->props[j].name,1,strlen(bdf->props[j].name)+1,strings);
		}
		str = bdf->props[j].u.str;
		if ( strmatch(bdf->props[j].name,"COMMENT")==0 ) {
		    str = MergeComments(bdf);
		    saw_comment = true;
		}
		putshort(at->bdf,bdf->props[j].type);
		if ( (bdf->props[j].type & ~prt_property)==prt_string ||
			(bdf->props[j].type & ~prt_property)==prt_atom ) {
		    putlong(at->bdf,ftell(strings));
		    fwrite(str,1,strlen(str)+1,strings);
		    if ( str!=bdf->props[j].u.str )
			free(str);
		} else {
		    putlong(at->bdf,bdf->props[j].u.val);
		}
	    }
	}
    }

    pos = ftell(at->bdf);
    fseek(at->bdf,4,SEEK_SET);
    putlong(at->bdf,pos);
    fseek(at->bdf,0,SEEK_END);

    if ( !ttfcopyfile(at->bdf,strings,pos,"BDF string table"))
return( false );

    at->bdflen = ftell( at->bdf );

    /* Align table */
    if ( at->bdflen&1 )
	putc('\0',at->bdf);
    if ( (at->bdflen+1)&2 )
	putshort(at->bdf,0);

return( true );
}

/* *************************    The 'BDF ' table    ************************* */
/* *************************          Input         ************************* */

static char *getstring(FILE *ttf,long start) {
    long here = ftell(ttf);
    int len, ch;
    char *str, *pt;

    if ( here<0 ) return( NULL );
    fseek(ttf,start,SEEK_SET);
    for ( len=1; (ch=getc(ttf))>0 ; ++len );
    fseek(ttf,start,SEEK_SET);
    pt = str = malloc(len);
    while ( (ch=getc(ttf))>0 )
	*pt++ = ch;
    *pt = '\0';
    fseek(ttf,here,SEEK_SET);
return( str );
}

/* COMMENTS get stored all in one lump by freetype. De-lump them */
static int CheckForNewlines(BDFFont *bdf,int k) {
    char *pt, *start;
    int cnt, i;

    for ( cnt=0, pt = bdf->props[k].u.atom; *pt; ++pt )
	if ( *pt=='\n' )
	    ++cnt;
    if ( cnt==0 )
return( k );

    bdf->prop_cnt += cnt;
    bdf->props = realloc(bdf->props, bdf->prop_cnt*sizeof( BDFProperties ));

    pt = strchr(bdf->props[k].u.atom,'\n');
    *pt = '\0'; ++pt;
    for ( i=1; i<=cnt; ++i ) {
	start = pt;
	while ( *pt!='\n' && *pt!='\0' ) ++pt;
	bdf->props[k+i].name = copy(bdf->props[k].name);
	bdf->props[k+i].type = bdf->props[k].type;
	bdf->props[k+i].u.atom = copyn(start,pt-start);
	if ( *pt=='\n' ) ++pt;
    }
    pt = copy( bdf->props[k].u.atom );
    free( bdf->props[k].u.atom );
    bdf->props[k].u.atom = pt;
return( k+cnt );
}

void ttf_bdf_read(FILE *ttf,struct ttfinfo *info) {
    int strike_cnt, i,j,k;
    long string_start;
    struct bdfinfo { BDFFont *bdf; int cnt; } *bdfinfo;
    BDFFont *bdf;

    if ( info->bdf_start==0 )
return;
    fseek(ttf,info->bdf_start,SEEK_SET);
    if ( getushort(ttf)!=1 )
return;
    strike_cnt = getushort(ttf);
    string_start = getlong(ttf) + info->bdf_start;

    bdfinfo = malloc(strike_cnt*sizeof(struct bdfinfo));
    for ( i=0; i<strike_cnt; ++i ) {
	int ppem, num_items;
	ppem = getushort(ttf);
	num_items = getushort(ttf);
	for ( bdf=info->bitmaps; bdf!=NULL; bdf=bdf->next )
	    if ( bdf->pixelsize==ppem )
	break;
	bdfinfo[i].bdf = bdf;
	bdfinfo[i].cnt = num_items;
    }

    for ( i=0; i<strike_cnt; ++i ) {
	if ( (bdf = bdfinfo[i].bdf) ==NULL )
	    fseek(ttf,10*bdfinfo[i].cnt,SEEK_CUR);
	else {
	    bdf->prop_cnt = bdfinfo[i].cnt;
	    bdf->props = malloc(bdf->prop_cnt*sizeof(BDFProperties));
	    for ( j=k=0; j<bdfinfo[i].cnt; ++j, ++k ) {
		long name = getlong(ttf);
		int type = getushort(ttf);
		long value = getlong(ttf);
		bdf->props[k].type = type;
		bdf->props[k].name = getstring(ttf,string_start+name);
		switch ( type&~prt_property ) {
		  case prt_int: case prt_uint:
		    bdf->props[k].u.val = value;
		    if ( strcmp(bdf->props[k].name,"FONT_ASCENT")==0 &&
			    value<=bdf->pixelsize ) {
			bdf->ascent = value;
			bdf->descent = bdf->pixelsize-value;
		    }
		  break;
		  case prt_string: case prt_atom:
		    bdf->props[k].u.str = getstring(ttf,string_start+value);
		    k = CheckForNewlines(bdf,k);
		  break;
		}
	    }
	}
    }
}


/* ************************************************************************** */
/* *************************    The 'FFTM' table    ************************* */
/* *************************         Output         ************************* */
/* ************************************************************************** */

/* FontForge timestamp table */
/* Contains: */
/*  date of fontforge sources */
/*  date of font's (not file's) creation */
/*  date of font's modification */
int ttf_fftm_dump(SplineFont *sf,struct alltabs *at) {
    int32 results[2];

    at->fftmf = tmpfile();

    putlong(at->fftmf,0x00000001);	/* Version */

    cvt_unix_to_1904(LibFF_ModTime,results);
    putlong(at->fftmf,results[1]);
    putlong(at->fftmf,results[0]);

    cvt_unix_to_1904(sf->creationtime,results);
    putlong(at->fftmf,results[1]);
    putlong(at->fftmf,results[0]);

    cvt_unix_to_1904(sf->modificationtime,results);
    putlong(at->fftmf,results[1]);
    putlong(at->fftmf,results[0]);

    at->fftmlen = ftell(at->fftmf);	/* had better be 7*4 */
	    /* It will never be misaligned */
    if ( (at->fftmlen&1)!=0 )
	putc(0,at->fftmf);
    if ( ((at->fftmlen+1)&2)!=0 )
	putshort(at->fftmf,0);
return( true );
}
