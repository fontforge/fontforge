/* Copyright (C) 2002 by George Williams */
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

#include "ttfmod.h"
#include <stdlib.h>
#include <math.h>

/* This file is designed to make a tiny ttf file containing just one character*/
/*  (well, and any others that it depends on) and only those tables needed to */
/*  make a real font.  Also need: head, hhea, hmtx, maxp, prep, fpgm, loca, */
/*  OS/2, post. Some of these are just direct copies, others (hmtx) are shrunk*/
/*  Freetype may require cmap,name too. post can be tiny */

struct tinytable {
    uint32 name;
    uint32 offset;
    uint32 checksum;
    FILE *source;
    uint32 len;
};
/* This lists the tables in the order they should appear in the font */
enum ttabs { tt_head, tt_hhea, tt_maxp, tt_OS2, tt_name, tt_cmap, tt_loca,
	tt_prep, tt_fpgm, tt_cvt, tt_hmtx, tt_glyf, tt_post, tt_max};
/* This lists the order of the pointers in the font header */
static enum ttabs alphabetic[] = { tt_OS2, tt_cmap, tt_cvt, tt_fpgm, tt_glyf,
	tt_head, tt_hhea, tt_hmtx, tt_loca, tt_maxp, tt_name, tt_prep, tt_post};
static uint32 tabnames[] = { CHR('h','e','a','d'), CHR('h','h','e','a'),
	CHR('m','a','x','p'), CHR('O','S','/','2'), CHR('n','a','m','e'),
	CHR('c','m','a','p'), CHR('l','o','c','a'), CHR('p','r','e','p'),
	CHR('f','p','g','m'), CHR('c','v','t',' '), CHR('h','m','t','x'), 
	CHR('g','l','y','f'), CHR('p','o','s','t')};

typedef struct tinytables {
    ConicChar *cc;
    ConicFont *cf;
    TtfFont *tfont;
    TtfFile *tfile;
    FILE *newf;
    uint8 *buf;
    FILE *glyf;
    int glyf_len;
    FILE *loca;
    int loca_len;
    FILE *hmtx;
    int hmtx_len;
    struct tinytable tabs[tt_max];
    int glyph_cnt;
} TT;

static void tiny_dumpinstrs(TT *tiny, ConicChar *cc) {
    int i;

    putshort(tiny->glyf,cc->instrdata.instr_cnt);
    for ( i=0; i<cc->instrdata.instr_cnt; ++i )
	putc(cc->instrdata.instrs[i],tiny->glyf);
}

#define _On_Curve	1
#define _X_Short	2
#define _Y_Short	4
#define _Repeat		8
#define _X_Same		0x10
#define _Y_Same		0x20

static void tiny_addpt(BasePoint *here,uint8 *flag,IPoint *pt,IPoint *last) {

    pt->x = rint(here->x)-last->x;
    pt->y = rint(here->y)-last->y;
    last->x = rint(here->x); last->y = rint(here->y);

    if ( pt->x==0 )
	*flag |= _X_Same;
    else if ( pt->x>=256 || pt->x<=-256 )
	/* 0 flag => signed 16 bit */;
    else if ( pt->x>0 )
	*flag |= _X_Short|_X_Same;
    else
	*flag |= _X_Short;

    if ( pt->y==0 )
	*flag |= _Y_Same;
    else if ( pt->y>=256 || pt->y<=-256 )
	/* 0 flag => signed 16 bit */;
    else if ( pt->y>0 )
	*flag |= _Y_Short|_Y_Same;
    else
	*flag |= _Y_Short;
}

static void tiny_dumpsimpleglyph(TT *tiny, ConicChar *cc) {
    ConicSet *cs;
    ConicPoint *cp;
    int pt;
    uint8 *flags;
    IPoint *pts, last;
    int i;

    /* number all our points */
    for ( cs = cc->conics, pt=0; cs!=NULL; cs=cs->next ) {
	for ( cp=cs->first; cp!=NULL; ) {
#if 0
	    if ( cp!=cs->first && cp->prevcp!=NULL && cp->nextcp!=NULL &&
		    cp->next!=NULL &&
		    (cp->me.x == (cp->prevcp->x+cp->nextcp->x)/2) &&
		    (cp->me.y == (cp->prevcp->y+cp->nextcp->y)/2))
		cp->me.pnum = -1;
	    else
		cp->me.pnum = pt++;
	    if ( cp->nextcp!=NULL )
		cp->nextcp->pnum = pt++;
#else
	    if ( cp->me.pnum!=-1 ) ++pt;
	    if ( cp->nextcp!=NULL ) ++pt;
#endif
	    if ( cp->next==NULL )
	break;
	    cp = cp->next->to;
	    if ( cp==cs->first )
	break;
	}
    }

    /* Array of contour end points */
    for ( cs = cc->conics; cs!=NULL; cs=cs->next ) {
	if ( cs->last!=cs->first )	/* Can't happen in a ttf file, but someone might have mucked with it... */
	    putshort(tiny->glyf,cs->last->me.pnum);
	else if ( cs->last->prevcp!=NULL )
	    putshort(tiny->glyf,cs->last->prevcp->pnum);
	else
	    putshort(tiny->glyf,cs->last->prev->from->me.pnum);
    }

    tiny_dumpinstrs(tiny,cc);

    flags = gcalloc(pt,sizeof(uint8));
    pts = gcalloc(pt,sizeof(IPoint));
    last.x = last.y = 0;
    for ( cs = cc->conics, pt=0; cs!=NULL; cs=cs->next ) {
	for ( cp=cs->first; cp!=NULL; ) {
	    if ( cp->me.pnum!=-1 ) {
		flags[pt] |= _On_Curve;
		tiny_addpt(&cp->me,&flags[pt],&pts[pt],&last);
		++pt;
	    }
	    if ( cp->nextcp!=NULL ) {
		tiny_addpt(cp->nextcp,&flags[pt],&pts[pt],&last);
		++pt;
	    }
	    if ( cp->next==NULL )
	break;
	    cp = cp->next->to;
	    if ( cp==cs->first )
	break;
	}
    }

    fwrite(flags,1,pt,tiny->glyf);
    for ( i=0; i<pt; ++i ) {
	if ( (flags[i]&_X_Short) && (flags[i]&_X_Same) )
	    putc(pts[i].x,tiny->glyf);
	else if ( (flags[i]&_X_Short) )
	    putc(-pts[i].x,tiny->glyf);
	else if ( !(flags[i]&_X_Same) )
	    putshort(tiny->glyf,pts[i].x);
    }
    for ( i=0; i<pt; ++i ) {
	if ( (flags[i]&_Y_Short) && (flags[i]&_Y_Same) )
	    putc(pts[i].y,tiny->glyf);
	else if ( (flags[i]&_Y_Short) )
	    putc(-pts[i].y,tiny->glyf);
	else if ( !(flags[i]&_Y_Same) )
	    putshort(tiny->glyf,pts[i].y);
    }
    free(flags);
    free(pts);
}

static void tiny_dumpglyph(TT *tiny,ConicChar *cc);

static void tiny_dumpcompositeglyph(TT *tiny, ConicChar *cc) {
    RefChar *ref;
    int flags;

    for ( ref=cc->refs; ref!=NULL; ref=ref->next ) {
	if ( ref->cc->tiny_glyph == -1 ) {
	    ref->cc->tiny_glyph = tiny->glyph_cnt++;
	    ref->tiny_output = false;
	} else
	    ref->tiny_output = true;
    }

    for ( ref=cc->refs; ref!=NULL; ref=ref->next ) {
	flags = (1<<1)|(1<<2)|(1<<12);
	if ( ref->next!=NULL )
	    flags |= (1<<5);		/* More components */
	else if ( ref->next==NULL && cc->instrdata.instr_cnt!=0 )
	    flags |= (1<<8);		/* Instructions appear after last ref */
	if ( ref->transform[1]!=0 || ref->transform[2]!=0 )
	    flags |= (1<<7);		/* Need a full matrix */
	else if ( ref->transform[0]!=ref->transform[3] )
	    flags |= (1<<6);		/* different xy scales */
	else if ( ref->transform[0]!=1. )
	    flags |= (1<<3);		/* xy scale is same */
	if ( ref->transform[4]<-128 || ref->transform[4]>127 ||
		ref->transform[5]<-128 || ref->transform[5]>127 )
	    flags |= (1<<0);
	putshort(tiny->glyf,flags);
	putshort(tiny->glyf,ref->cc->tiny_glyph);
	if ( flags&(1<<0) ) {
	    putshort(tiny->glyf,(short)ref->transform[4]);
	    putshort(tiny->glyf,(short)ref->transform[5]);
	} else {
	    putc((char) (ref->transform[4]),tiny->glyf);
	    putc((char) (ref->transform[5]),tiny->glyf);
	}
	if ( flags&(1<<7) ) {
	    put2d14(tiny->glyf,ref->transform[0]);
	    put2d14(tiny->glyf,ref->transform[1]);
	    put2d14(tiny->glyf,ref->transform[2]);
	    put2d14(tiny->glyf,ref->transform[3]);
	} else if ( flags&(1<<6) ) {
	    put2d14(tiny->glyf,ref->transform[0]);
	    put2d14(tiny->glyf,ref->transform[3]);
	} else if ( flags&(1<<3) ) {
	    put2d14(tiny->glyf,ref->transform[0]);
	}
    }
    if ( cc->instrdata.instr_cnt!=0 )
	tiny_dumpinstrs(tiny,cc);
    if ( ftell(tiny->glyf)&1 )		/* Pad the file so that the next glyph */
	putc('\0',tiny->glyf);		/* on a word boundary, can only happen if odd number of instrs */

    for ( ref=cc->refs; ref!=NULL; ref=ref->next ) if ( !ref->tiny_output )
	tiny_dumpglyph(tiny,ref->cc);
}

static void tiny_dumpglyph(TT *tiny,ConicChar *cc) {
    DBounds bb;
    ConicSet *cs;
    int cnt;

    ConicCharFindBounds(cc,&bb);
    putshort(tiny->loca,ftell(tiny->glyf)/2);
    putshort(tiny->hmtx,cc->width); putshort(tiny->hmtx,bb.minx);

    if ( !cc->changed && cc->conics!=NULL ) {
	tiny->buf = copyregion(tiny->glyf,tiny->tfile->file,
		cc->parent->glyphs->start+cc->glyph_offset,cc->glyph_len,
		tiny->buf);
return;
    }

    if ( cc->conics!=NULL || cc->refs!=NULL ) {
	if ( cc->conics==NULL )
	    putshort(tiny->glyf,-1);
	else {
	    for ( cs=cc->conics,cnt=0; cs!=NULL; cs = cs->next, ++cnt );
	    putshort(tiny->glyf,cnt);
	}
	putshort(tiny->glyf,bb.minx);
	putshort(tiny->glyf,bb.miny);
	putshort(tiny->glyf,bb.maxx);
	putshort(tiny->glyf,bb.maxy);
    }
    if ( cc->conics!=NULL )
	tiny_dumpsimpleglyph(tiny,cc);
    else if ( cc->refs!=NULL )
	tiny_dumpcompositeglyph(tiny,cc);
    else
	/* Nothing more needs to be done for space characters */;
}

static void tiny_dumpglyphs(TT *tiny) {
    ConicFont *cf = tiny->cf;
    int i;

    for ( i=0; i<cf->glyph_cnt; ++i )
	cf->chars[i]->tiny_glyph = -1;
    tiny->cc->tiny_glyph = 4;

    tiny->glyf = tmpfile();
    tiny->loca = tmpfile();
    tiny->hmtx = tmpfile();

    /* I'm just going to make the first four glyphs be spaces, even glyph 0 */
    putshort(tiny->loca,0);		/* Pointer to notdef */
    putshort(tiny->loca,0);		/* Pointer to .null */
    putshort(tiny->loca,0);		/* Pointer to cr */
    putshort(tiny->loca,0);		/* Pointer to space */

    putshort(tiny->hmtx,cf->chars[0]->width); putshort(tiny->hmtx,0);
    putshort(tiny->hmtx,cf->chars[1]->width); putshort(tiny->hmtx,0);
    putshort(tiny->hmtx,cf->chars[2]->width); putshort(tiny->hmtx,0);
    putshort(tiny->hmtx,cf->chars[3]->width); putshort(tiny->hmtx,0);

    tiny->glyph_cnt = 5;
    tiny_dumpglyph(tiny,tiny->cc);
    tiny->glyf_len = ftell(tiny->glyf);
    putshort(tiny->loca,tiny->glyf_len/2);	/* Pointer to end of glyphs */

    if ( tiny->glyf_len&2 )
	putshort(tiny->glyf,0);

    tiny->loca_len = ftell(tiny->loca);
    if ( tiny->loca_len&1 )
	putshort(tiny->loca,0);

    tiny->hmtx_len = ftell(tiny->hmtx);
    /* Always aligned */
}

static int32 tiny_cmap(FILE *cmap) {
    uint8 map[256];
    putshort(cmap,0);		/* version */
    putshort(cmap,1);		/* num tables */
    putshort(cmap,1);		/* mac platform */
    putshort(cmap,0);		/* plat specific enc, script=roman */
    putlong(cmap,12);		/* offset from tab start to sub tab start */
	/* Subtable */
    putshort(cmap,0);		/* format */
    putshort(cmap,262);		/* length = 256bytes + 6 header bytes */
    putshort(cmap,0);		/* language = english */
    memset(map,0,256);
    map[0] = 1;
    map[13] = 2;
    map[32] = 3;
    map[33] = 4;		/* The one character we care about */
    fwrite(map,1,256,cmap);

    putshort(cmap,0);		/* padding */
return( 12+6+256 );
}

static int32 tiny_post(TT *tiny, FILE *post) {
    Table *tab=TTFFindTable(tiny->tfont,tabnames[tt_post]);
    char buffer[16];

    putlong(post,0x00030000);	/* short table */
    if ( tab==NULL ) {
	putlong(post,0);
	putshort(post,-200);
	putshort(post,50);
	putlong(post,0);
    } else if ( tab->data!=NULL ) {
	fwrite(tab->data+4,1,12,post);
    } else {
	fseek(tiny->tfile->file,tab->start+4,SEEK_SET);
	fread(buffer,1,12,tiny->tfile->file);
	fwrite(buffer,1,12,post);
    }
    memset(buffer,0,16);
    fwrite(buffer,1,16,post);
    /* always aligned */
return( 32 );
}

static void tiny_dumpfontheader(FILE *newf, TT *tiny) {
    int bit, i, cnt;
    struct tinytable *tab;

    rewind(newf);
    for ( i=tt_head, cnt=0; i<tt_max; ++i )
	if ( tiny->tabs[i].name!=0 ) ++cnt;

    putlong(newf,tiny->tfont->version);
    putshort(newf,cnt);
    for ( i= -1, bit = 1; bit<cnt; bit<<=1, ++i );
    bit>>=1;
    putshort(newf,bit*16);
    putshort(newf,i);
    putshort(newf,(cnt-bit)*16);
    /* Dump out in alphabetic order... */
    for ( i=tt_head, cnt=0; i<tt_max; ++i ) if ( tiny->tabs[alphabetic[i]].name!=0 ) {
	tab = &tiny->tabs[alphabetic[i]];
	putlong(newf,tab->name);
	putlong(newf,tab->checksum);
	putlong(newf,tab->offset);
	putlong(newf,tab->len);
    }
}

static void tiny_preparetables(TT *tiny) {
    static int sametables[] = { tt_head, tt_maxp, tt_hhea,
	    tt_OS2, tt_name, tt_prep, tt_fpgm, tt_cvt, -1 };
	    /* head, hhea and maxp all need a fix */
    int i, cnt;
    uint32 checksum;
    Table *tab;
    struct tinytable *ttab;
    FILE *newf;

    tiny->tabs[tt_glyf].name = CHR('g','l','y','f');
    tiny->tabs[tt_glyf].source = tiny->glyf;
    tiny->tabs[tt_glyf].len = tiny->glyf_len; 
    tiny->tabs[tt_glyf].checksum = filecheck(tiny->glyf);

    tiny->tabs[tt_loca].name = CHR('l','o','c','a');
    tiny->tabs[tt_loca].source = tiny->loca;
    tiny->tabs[tt_loca].len = tiny->loca_len; 
    tiny->tabs[tt_loca].checksum = filecheck(tiny->loca);

    tiny->tabs[tt_hmtx].name = CHR('h','m','t','x');
    tiny->tabs[tt_hmtx].source = tiny->hmtx;
    tiny->tabs[tt_hmtx].len = tiny->hmtx_len; 
    tiny->tabs[tt_hmtx].checksum = filecheck(tiny->hmtx);

    tiny->tabs[tt_cmap].name = tabnames[tt_cmap];
    tiny->tabs[tt_cmap].source = tmpfile();;
    tiny->tabs[tt_cmap].len = tiny_cmap(tiny->tabs[tt_cmap].source); 
    tiny->tabs[tt_cmap].checksum = filecheck(tiny->tabs[tt_cmap].source);

    tiny->tabs[tt_post].name = tabnames[tt_post];
    tiny->tabs[tt_post].source = tmpfile();;
    tiny->tabs[tt_post].len = tiny_post(tiny,tiny->tabs[tt_post].source); 
    tiny->tabs[tt_post].checksum = filecheck(tiny->tabs[tt_post].source);

    for ( i=0; sametables[i]!=-1; ++i ) {
	if ( (tab=TTFFindTable(tiny->tfont,tabnames[sametables[i]]))!=NULL )
	    tiny->tabs[sametables[i]].name = tabnames[sametables[i]];
    }

    newf = tiny->newf;

    tiny_dumpfontheader(newf, tiny);	/* Create a placeholder */

    for ( i=tt_head, cnt=0; i<tt_max; ++i ) if ( tiny->tabs[i].name!=0 ) {
	ttab = &tiny->tabs[i];

	ttab->offset = ftell(newf);
	if ( ttab->source==NULL )
	    tab = TTFFindTable(tiny->tfont,tabnames[i]);
	if ( ttab->source!=NULL ) {
	    tiny->buf = copyregion(newf,ttab->source,0,ttab->len,tiny->buf);
	    fclose(ttab->source);
	} else if ( tab->td_changed ) {
	    (tab->write_tabledata)(newf,tab);
	} else if ( tab->data )
	    fwrite(tab->data,1,tab->newlen,newf);
	else {
	    tiny->buf = copyregion(newf,tiny->tfile->file,tab->start,tab->len,tiny->buf);
	    ttab->checksum = tab->oldchecksum;
	}
	ttab->len = ftell(newf)-ttab->offset;
	if ( ttab->len&1 )
	    putc('\0',newf);
	if ( (ttab->len+1)&2 )
	    putshort(newf,0);
    }

    fseek(newf,tiny->tabs[tt_head].offset+8,SEEK_SET);
    putlong(newf,0);
    fseek(newf,tiny->tabs[tt_maxp].offset+4,SEEK_SET);
    putshort(newf,tiny->glyph_cnt);
    fseek(newf,tiny->tabs[tt_hhea].offset+34,SEEK_SET);
    putshort(newf,tiny->glyph_cnt);
    tiny->tabs[tt_head].checksum = tiny->tabs[tt_maxp].checksum = tiny->tabs[tt_hhea].checksum = 0;

    for ( i=tt_head, cnt=0; i<tt_max; ++i )
	if ( tiny->tabs[i].name!=0 && tiny->tabs[i].checksum==0 )
	    tiny->tabs[i].checksum = figurecheck(newf,tiny->tabs[i].offset,(tiny->tabs[i].len+3)>>2);

    tiny_dumpfontheader(newf, tiny);	/* now store real data */

    checksum = 0xb1b0afba - filecheck(newf);
    fseek(newf,tiny->tabs[tt_head].offset+8,SEEK_SET);
    putlong(newf,checksum);

    free(tiny->buf);
}

FILE *Ttf_MakeTinyFont(ConicChar *cc) {
    TT tiny;

    memset(&tiny,0,sizeof(tiny));
    tiny.cc = cc;
    tiny.cf = cc->parent;
    tiny.tfont = tiny.cf->tfont;
    tiny.tfile = tiny.cf->tfile;

    tiny_dumpglyphs(&tiny);

    tiny.newf = tmpfile();
    tiny_preparetables(&tiny);

    if ( !ferror(tiny.newf))
return( tiny.newf );

    fclose(tiny.newf);
return( NULL );
}
