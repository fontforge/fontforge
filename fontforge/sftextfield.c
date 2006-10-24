/* Copyright (C) 2002-2006 by George Williams */
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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include <gkeysym.h>

#include "../gdraw/ggadgetP.h"
#include "ustring.h"
#include "utype.h"

typedef struct fontdata {
    SplineFont *sf;
    enum sftf_fonttype fonttype;
    int pixelsize;
    int antialias;
    BDFFont *bdf;
    struct fontdata *next;
    struct fontdata *depends_on;	/* We use much of the ftc allocated for depends_on */
					/* Can't free depends_on until after we get freeed */
    struct _GImage base;
    GImage gi;
    GClut clut;
    struct sfmaps *sfmap;
} FontData;

typedef struct sftextarea {
    GGadget g;
    unsigned int cursor_on: 1;
    unsigned int wordsel: 1;
    unsigned int linesel: 1;
    unsigned int listfield: 1;
    unsigned int drag_and_drop: 1;
    unsigned int has_dd_cursor: 1;
    unsigned int hidden_cursor: 1;
    unsigned int multi_line: 1;
    unsigned int accepts_tabs: 1;
    unsigned int accepts_returns: 1;
    unsigned int wrap: 1;
    unsigned int dobitext: 1;	/* has at least one right to left character */
    unsigned int password: 1;
    unsigned int dontdraw: 1;	/* Used when the tf is part of a larger control, and the control determines when to draw the tf */
    uint8 fh;
    uint8 as;
    uint8 nw;			/* Width of one character (an "n") */
    int16 xoff_left, loff_top;
    int16 sel_start, sel_end, sel_base;
    int16 sel_oldstart, sel_oldend, sel_oldbase;
    int16 dd_cursor_pos;
    unichar_t *text, *oldtext;
    FontInstance *font;
    GTimer *pressed;
    GTimer *cursor;
    GCursor old_cursor;
    GScrollBar *hsb, *vsb;
    int16 lcnt, lmax;
    int32 *lines;		/* offsets in text to the start of the nth line */
    GBiText bidata;
    int32 bilen;		/* allocated size of bidata */
    int16 xmax;
    GIC *gic;
    struct lineheights { int32 y; int16 as, fh; } *lineheights;
    struct fontlist {
	int start, end;		/* starting and ending characters [start,end) */
	FontData *fd;
	struct fontlist *next;
    } *fontlist, *oldfontlist;
    struct sfmaps {
	SplineFont *sf;
	EncMap *map;
	struct sfmaps *next;
    } *sfmaps;
    FontData *generated;
    void *cbcontext;
    void (*changefontcallback)(void *,SplineFont *,enum sftf_fonttype,int size,int aa);
    FontData *last_fd;
} SFTextArea;

void SFTextAreaShow(GGadget *g,int pos);
void SFTextAreaSelect(GGadget *g,int start, int end);
void SFTextAreaReplace(GGadget *g,const unichar_t *txt);

static GBox sftextarea_box = { /* Don't initialize here */ 0 };
static int sftextarea_inited = false;
static FontInstance *sftextarea_font;

static unichar_t nullstr[] = { 0 }, 
	newlinestr[] = { '\n', 0 }, tabstr[] = { '\t', 0 };

static int SFTextArea_Show(SFTextArea *st, int pos);
static void GTPositionGIC(SFTextArea *st);

static void SFTextAreaChanged(SFTextArea *st,int src) {
    GEvent e;

    e.type = et_controlevent;
    e.w = st->g.base;
    e.u.control.subtype = et_textchanged;
    e.u.control.g = &st->g;
    e.u.control.u.tf_changed.from_pulldown = src;
    if ( st->g.handle_controlevent != NULL )
	(st->g.handle_controlevent)(&st->g,&e);
    else
	GDrawPostEvent(&e);
}

static void SFTextAreaFocusChanged(SFTextArea *st,int gained) {
    GEvent e;

    e.type = et_controlevent;
    e.w = st->g.base;
    e.u.control.subtype = et_textfocuschanged;
    e.u.control.g = &st->g;
    e.u.control.u.tf_focus.gained_focus = gained;
    if ( st->g.handle_controlevent != NULL )
	(st->g.handle_controlevent)(&st->g,&e);
    else
	GDrawPostEvent(&e);
}

static void SFTextAreaProcessBi(SFTextArea *st, int start_of_change) {
    int i, pos;
    unichar_t *pt, *end;
    GBiText bi;

    if ( !st->dobitext )
	i = GDrawIsAllLeftToRight(st->text+start_of_change,-1);
    else
	i = GDrawIsAllLeftToRight(st->text,-1);
    st->dobitext = (i!=1);
    if ( st->dobitext ) {
	int cnt = u_strlen(st->text);
	if ( cnt>= st->bilen ) {
	    st->bilen = cnt + 50;
	    free(st->bidata.text); free(st->bidata.level);
	    free(st->bidata.override); free(st->bidata.type);
	    free(st->bidata.original);
	    ++st->bilen;
	    st->bidata.text = galloc(st->bilen*sizeof(unichar_t));
	    st->bidata.level = galloc(st->bilen*sizeof(uint8));
	    st->bidata.override = galloc(st->bilen*sizeof(uint8));
	    st->bidata.type = galloc(st->bilen*sizeof(uint16));
	    st->bidata.original = galloc(st->bilen*sizeof(unichar_t *));
	    --st->bilen;
	}
	bi = st->bidata;
	pt = st->text;
	pos = 0;
	st->bidata.interpret_arabic = false;
	do {
	    end = u_strchr(pt,'\n');
	    if ( end==NULL || !st->multi_line ) end = pt+u_strlen(pt);
	    else ++end;
	    bi.text = st->bidata.text+pos;
	    bi.level = st->bidata.level+pos;
	    bi.override = st->bidata.override+pos;
	    bi.type = st->bidata.type+pos;
	    bi.original = st->bidata.original+pos;
	    bi.base_right_to_left = GDrawIsAllLeftToRight(pt,end-pt)==-1;
	    GDrawBiText1(&bi,pt,end-pt);
	    if ( bi.interpret_arabic ) st->bidata.interpret_arabic = true;
	    pos += end-pt;
	    pt = end;
	} while ( *pt!='\0' );
	st->bidata.len = cnt;
	if ( !st->multi_line ) {
	    st->bidata.base_right_to_left = bi.base_right_to_left;
	    GDrawBiText2(&st->bidata,0,-1);
	}
    }
}

static int FDMap(FontData *fd,int uenc) {
    /* given a unicode code point, find the encoding in this font */
    int gid;

    if ( uenc>=fd->sfmap->map->enccount )
return( -1 );
    gid = fd->sfmap->map->map[uenc];
    if ( gid==-1 || fd->sf->glyphs[gid]==NULL )
return( -1 );

return( gid );
}

static int FDDrawChar(GWindow pixmap,FontData *fd,int gid,int x,int y,Color col) {
    BDFChar *bdfc;

    if ( gid!=-1 && fd->bdf->glyphs[gid]==NULL )
	BDFPieceMeal(fd->bdf,gid);
    if ( gid==-1 || (bdfc=fd->bdf->glyphs[gid])==NULL ) {
	if ( col!=-1 ) {
	    GRect r;
	    r.x = x+1; r.width= fd->bdf->ascent/2-2;
	    r.height = (2*fd->bdf->ascent/3); r.y = y-r.height;
	    GDrawDrawRect(pixmap,&r,col);
	}
	x += fd->bdf->ascent/2;
    } else {
	if ( col!=-1 ) {
	    fd->clut.clut[1] = col;		/* Only works for bitmaps */
	    fd->base.clut->trans_index = 0;
	    fd->base.data = bdfc->bitmap;
	    fd->base.bytes_per_line = bdfc->bytes_per_line;
	    fd->base.width = bdfc->xmax-bdfc->xmin+1;
	    fd->base.height = bdfc->ymax-bdfc->ymin+1;
	    GDrawDrawImage(pixmap,&fd->gi,NULL,x+bdfc->xmin, y-bdfc->ymax);
	    fd->base.clut->trans_index = -1;
	}
	x += bdfc->width;
    }
return( x );
}

static int STDrawText(SFTextArea *st,GWindow pixmap,int x,int y,
	int pos, int len, Color col ) {
    struct fontlist *fl, *sub;
    int gid, npos;

    if ( len==-1 )
	len = u_strlen(st->text)-pos;
    for ( fl=st->fontlist; fl!=NULL; fl=fl->next )
	if ( fl->start<=pos && fl->end>pos )
    break;
    if ( fl==NULL )
return(x);

    if ( !st->dobitext ) {
	while ( len>0 ) {
	    if ( pos>=fl->end )
		fl = fl->next;
	    if ( fl==NULL )
	break;
	    if ( st->text[pos]!='\n' ) {
		gid = FDMap(fl->fd,st->text[pos]);
		x = FDDrawChar(pixmap,fl->fd,gid,x,y,col);
	    }
	    ++pos;
	    --len;
	}
    } else {
	while ( len>0 ) {
	    npos = st->bidata.original[pos]-st->text;
	    for ( sub=fl; sub!=NULL; sub=sub->next )
		if ( sub->start<=pos && sub->end>pos )
	    break;
	    if ( sub!=NULL && st->bidata.text[pos]!='\n') {
		gid = FDMap(sub->fd,st->bidata.text[pos]);
		x = FDDrawChar(pixmap,sub->fd,gid,x,y,col);
	    }
	    ++pos;
	    --len;
	}
    }
return( x );
}

static int STGetTextWidth(SFTextArea *st, int pos, int len ) {
return( STDrawText(st,NULL,0,-1,pos,len,-1));
}

static void STGetTextPtAfterPos(SFTextArea *st, unichar_t *pt, int cnt, int max, unichar_t **end ) {
    unichar_t *last = pt+cnt;
    int len;

    while ( pt<last ) {
	len = STGetTextWidth(st,pt-st->text,1);
	max -= len;
	if ( max<0 ) {
	    *end = pt-1;
return;
	}
	++pt;
    }
    *end = pt;
}

static void STGetTextPtBeforePos(SFTextArea *st, unichar_t *pt, int cnt, int max, unichar_t **end ) {
    unichar_t *last = pt+cnt;
    int len;

    while ( pt<last ) {
	len = STGetTextWidth(st,pt-st->text,1);
	max -= len;
	if ( max<0 ) {
	    *end = pt;
return;
	}
	++pt;
    }
    *end = pt;
}

static void SFFigureLineHeight(SFTextArea *st, int i) {
    int as=0, ds=0;
    int start, end;
    struct fontlist *fl;

    if ( i<0 )
return;
    start = st->lines[i]; end = st->lines[i+1];
    for ( fl=st->fontlist; fl!=NULL && fl->end<start; fl=fl->next );
    for ( ; fl!=NULL; fl=fl->next ) {
	if ( fl->fd->bdf->ascent>as )
	    as = fl->fd->bdf->ascent;
	if ( fl->fd->bdf->descent>ds )
	    ds = fl->fd->bdf->descent;
	if ( fl->end>=end )
    break;
    }
    if ( as+ds==0 ) { as = st->as; ds = st->fh-as; }
    st->lineheights[i].fh = as+ds;
    st->lineheights[i].as = as;
    if ( i==0 )
	st->lineheights[i].y = 0;
    else
	st->lineheights[i].y = st->lineheights[i-1].y+st->lineheights[i-1].fh;
}

static void SFTextAreaRefigureLines(SFTextArea *st, int start_of_change) {
    int i, dobitext;
    unichar_t *pt, *ept, *end, *temp;

    GDrawSetFont(st->g.base,st->font);
    if ( st->lines==NULL ) {
	st->lines = galloc(10*sizeof(int32));
	st->lineheights = galloc(10*sizeof(struct lineheights));
	st->lines[0] = 0;
	st->lines[1] = -1;
	st->lmax = 10;
	st->lcnt = 1;
	if ( st->vsb!=NULL )
	    GScrollBarSetBounds(&st->vsb->g,0,st->lcnt-1,st->g.inner.height);
    }

    SFTextAreaProcessBi(st,start_of_change);

    if ( !st->multi_line ) {
	st->xmax = STGetTextWidth(st,0,-1);
return;
    }

    for ( i=0; i<st->lcnt && st->lines[i]<start_of_change; ++i );
    if ( !st->wrap || st->fontlist==NULL ) {
	if ( --i<0 ) i = 0;
	pt = st->text+st->lines[i];
	while ( ( ept = u_strchr(pt,'\n'))!=NULL ) {
	    if ( i>=st->lmax ) {
		st->lines = grealloc(st->lines,(st->lmax+=10)*sizeof(int32));
		st->lineheights = grealloc(st->lineheights,st->lmax*sizeof(struct lineheights));
	    }
	    st->lines[i] = pt-st->text;
	    SFFigureLineHeight(st,i-1);
	    ++i;
	    pt = ept+1;
	}
	if ( i>=st->lmax ) {
	    st->lines = grealloc(st->lines,(st->lmax+=10)*sizeof(int32));
	    st->lineheights = grealloc(st->lineheights,st->lmax*sizeof(struct lineheights));
	}
	st->lines[i] = pt-st->text;
	SFFigureLineHeight(st,i-1);
	++i;
    } else {
	if (( i -= 2 )<0 ) i = 0;
	pt = st->text+st->lines[i];
	dobitext = st->dobitext; st->dobitext = false;	/* Turn bitext processing of for eol computation */
	do {
	    if ( ( ept = u_strchr(pt,'\n'))==NULL )
		ept = pt+u_strlen(pt);
	    while ( pt<=ept ) {
		STGetTextPtAfterPos(st,pt, ept-pt, st->g.inner.width, &end);
		if ( end!=ept && !isbreakbetweenok(*end,end[1]) ) {
		    for ( temp=end; temp>pt && !isbreakbetweenok(*temp,temp[1]); --temp );
		    if ( temp==pt )
			for ( temp=end; temp<ept && !isbreakbetweenok(*temp,temp[1]); ++temp );
		    end = temp;
		}
		if ( i>=st->lmax ) {
		    st->lines = grealloc(st->lines,(st->lmax+=10)*sizeof(int32));
		    st->lineheights = grealloc(st->lineheights,st->lmax*sizeof(struct lineheights));
		}
		st->lines[i] = pt-st->text;
		SFFigureLineHeight(st,i-1);
		++i;
		if ( *end=='\0' )
       goto break_2_loops;
		pt = end+1;
	    }
	} while ( *ept!='\0' );
       break_2_loops:
	st->dobitext = dobitext;
    }
    if ( i>=2 )
	st->lineheights[i-1].y = st->lineheights[i-2].y+st->lineheights[i-2].fh;
    else
	st->lineheights[i-1].y = 0;
    if ( st->lcnt!=i ) { int page;
	st->lcnt = i;
	if ( st->vsb!=NULL )
	    GScrollBarSetBounds(&st->vsb->g,0,st->lineheights[i-1].y,st->g.inner.height);
	for ( page=1; st->lcnt-page>=0 && st->lineheights[st->lcnt-1].y-st->lineheights[st->lcnt-page].y<=st->g.inner.height;
		++page );
	if ( st->loff_top > st->lcnt-page ) {
	    st->loff_top = st->lcnt-page;
	    if ( st->loff_top<0 ) st->loff_top = 0;
	    if ( st->vsb!=NULL )
		GScrollBarSetPos(&st->vsb->g,st->lineheights[st->loff_top].y);
	}
    }
    if ( i>=st->lmax ) {
	st->lines = grealloc(st->lines,(st->lmax+=10)*sizeof(int32));
	st->lineheights = grealloc(st->lineheights,st->lmax*sizeof(struct lineheights));
    }
    st->lines[i++] = -1;

    st->xmax = 0;
    for ( i=0; i<st->lcnt; ++i ) {
	int eol = st->lines[i+1]==-1?-1:st->lines[i+1]-st->lines[i]-1;
	int wid = STGetTextWidth(st,st->lines[i],eol);
	if ( wid>st->xmax )
	    st->xmax = wid;
    }
    if ( st->hsb!=NULL ) {
	GScrollBarSetBounds(&st->hsb->g,0,st->xmax,st->g.inner.width);
    }

    if ( st->dobitext ) {
	int end = -1, off;
	for ( i=0; i<st->lcnt; ++i ) {
	    if ( st->lines[i]>end ) {
		unichar_t *ept = u_strchr(st->text+end+1,'\n');
		if ( ept==NULL ) ept = st->text+u_strlen(st->text);
		st->bidata.base_right_to_left = GDrawIsAllLeftToRight(st->text+end+1,ept-st->text)==-1;
		end = ept - st->text;
	    }
	    off = 0;
	    if ( st->text[st->lines[i+1]-1]=='\n' ) off=1;
	    GDrawBiText2(&st->bidata,st->lines[i],st->lines[i+1]-off);
	}
    }
}

static void fontlistfree(struct fontlist *fl ) {
    struct fontlist *nfl;

    for ( ; fl!=NULL; fl=nfl ) {
	nfl = fl->next;
	chunkfree(fl,sizeof(struct fontlist));
    }
}

static struct fontlist *fontlistcopy(struct fontlist *fl ) {
    struct fontlist *nfl, *nhead=NULL, *last=NULL;

    for ( ; fl!=NULL; fl=fl->next ) {
	nfl = chunkalloc(sizeof(struct fontlist));
	*nfl = *fl;
	if ( nhead == NULL )
	    nhead = nfl;
	else
	    last->next = nfl;
	last = nfl;
    }
return( nhead );
}

static void fontlistcheck(SFTextArea *st) {
    struct fontlist *fl, *next;

    if ( st->fontlist==NULL )
return;
    for ( fl = st->fontlist; fl!=NULL; fl=next ) {
	next = fl->next;
	if ( next==NULL )
    break;
	if ( fl->start>fl->end || fl->end!=next->start || next==fl || next->next==fl ) {
	    IError("FontList is corrupted" );
	    fl->next = NULL;
return;
	}
    }
}

static void fontlistmergecheck(SFTextArea *st) {
    struct fontlist *fl, *next;

    if ( st->fontlist==NULL )
return;
    fontlistcheck(st);
    for ( fl = st->fontlist; fl!=NULL; fl=next ) {
	for ( next=fl->next; next!=NULL && next->fd==fl->fd ; next = fl->next ) {
	    fl->next = next->next;
	    fl->end = next->end;
	    chunkfree(next,sizeof(struct fontlist));
	}
    }
    fontlistcheck(st);
}

static void SFTextAreaChangeFontList(SFTextArea *st,int rpllen) {
    /* we are removing a chunk starting at st->sel_start going to st->sel_end */
    /*  and replacing it with a chunk that is rpllen long */
    /* So we remove any chunks wholy within sel_start,sel_end and extend the */
    /*  chunk at sel_start by rpllen */
    struct fontlist *fl, *next, *test;
    int diff, checkmerge=true;

    fontlistfree(st->oldfontlist);
    st->oldfontlist = fontlistcopy(st->fontlist);

    for ( fl=st->fontlist; fl!=NULL && st->sel_start>fl->end; fl=fl->next );
    if ( fl==NULL )
return;
    diff = rpllen - (st->sel_end-st->sel_start);
    if ( fl->end>=st->sel_end ) {
	fl->end += diff;
	fl = fl->next;
    } else {
	fl->end = st->sel_start + rpllen;
	for ( test=fl->next; test!=NULL && st->sel_end>=test->end; test=next ) {
	    next = test->next;
	    chunkfree(test,sizeof(struct fontlist));
	    checkmerge=true;
	}
	fl->next = test;
	if ( test!=NULL ) {
	    test->start = fl->end;
	    test->end -= diff;
	    fl = test->next;
	} else
	    fl = NULL;
    }
    while ( fl!=NULL ) {
	fl->start += diff;
	fl->end += diff;
	fl = fl->next;
    }
    if ( checkmerge )
	fontlistmergecheck(st);
}

static void _SFTextAreaReplace(SFTextArea *st, const unichar_t *str) {
    unichar_t *old = st->oldtext;
    int rpllen = u_strlen(str);
    unichar_t *new = galloc((u_strlen(st->text)-(st->sel_end-st->sel_start) + rpllen+1)*sizeof(unichar_t));

    st->oldtext = st->text;
    st->sel_oldstart = st->sel_start;
    st->sel_oldend = st->sel_end;
    st->sel_oldbase = st->sel_base;
    SFTextAreaChangeFontList(st,rpllen);

    u_strncpy(new,st->text,st->sel_start);
    u_strcpy(new+st->sel_start,str);
    st->sel_start += rpllen;
    u_strcpy(new+st->sel_start,st->text+st->sel_end);
    st->text = new;
    st->sel_end = st->sel_base = st->sel_start;
    free(old);

    SFTextAreaRefigureLines(st,st->sel_oldstart);
}

static void SFTextArea_Replace(SFTextArea *st, const unichar_t *str) {
    _SFTextAreaReplace(st,str);
    SFTextArea_Show(st,st->sel_start);
}

static int SFTextAreaFindLine(SFTextArea *st, int pos) {
    int i;
    for ( i=0; st->lines[i+1]!=-1; ++i )
	if ( pos<st->lines[i+1])
    break;
return( i );
}

static unichar_t *SFTextAreaGetPtFromPos(SFTextArea *st,int i,int xpos) {
    int ll, offset, len;

    ll = st->lines[i+1];
    if ( ll==-1 ) ll = st->lines[i]+u_strlen(st->text+st->lines[i]);
    xpos -= st->g.inner.x-st->xoff_left;
    for ( offset=st->lines[i]; offset<ll; ++offset ) {
	len = STGetTextWidth(st,offset,1);
	xpos -= len;
	if ( xpos<0 )
return( st->text+offset );
    }
return( st->text+offset );
}

static int SFTextAreaBiPosFromPos(SFTextArea *st,int i,int pos) {
    int ll,j;
    unichar_t *pt = st->text+pos;

    if ( !st->dobitext )
return( pos );
    ll = st->lines[i+1]==-1?-1:st->lines[i+1]-st->lines[i]-1;
    for ( j=st->lines[i]; j<ll; ++j )
	if ( st->bidata.original[j] == pt )
return( j );

return( pos );
}

static int SFTextAreaGetOffsetFromOffset(SFTextArea *st,int l, int sel) {
    int i;
    unichar_t *spt = st->text+sel;
    int llen = st->lines[l+1]!=-1 ? st->lines[l+1]: u_strlen(st->text+st->lines[l])+st->lines[l];

    if ( !st->dobitext )
return( sel );
    for ( i=st->lines[l]; i<llen && st->bidata.original[i]!=spt; ++i );
return( i );
}

static int SFTextArea_Show(SFTextArea *st, int pos) {
    int i, ll, m, xoff, loff;
    unichar_t *bitext = st->dobitext ? st->bidata.text:st->text;
    int refresh=false, page;

    if ( pos < 0 ) pos = 0;
    if ( pos > u_strlen(st->text)) pos = u_strlen(st->text);
    i = SFTextAreaFindLine(st,pos);

    loff = st->loff_top;
    if ( st->lineheights[st->lcnt-1].y<st->g.inner.height )
	loff = 0;
    if ( i<loff )
	loff = i;
    for ( page=1; st->lcnt-page>=0 && st->lineheights[st->lcnt-1].y-st->lineheights[st->lcnt-page].y<=st->g.inner.height;
	    ++page );
    if ( loff > st->lcnt-page )
	loff = st->lcnt-page;
    if ( loff<0 ) loff = 0;

    xoff = st->xoff_left;
    if ( st->lines[i+1]==-1 ) ll = -1; else ll = st->lines[i+1]-st->lines[i]-1;
    if ( STGetTextWidth(st,st->lines[i],ll)< st->g.inner.width )
	xoff = 0;
    else {
	if ( st->dobitext ) {
	    bitext = st->bidata.text;
	    pos = SFTextAreaBiPosFromPos(st,i,pos);
	} else
	    bitext = st->text;
	m = STGetTextWidth(st,st->lines[i],pos-st->lines[i]);
	if ( m < xoff )
	    xoff = st->nw*(m/st->nw);
	if ( m - xoff >= st->g.inner.width )
	    xoff = st->nw * ((m-2*st->g.inner.width/3)/st->nw);
    }

    if ( xoff!=st->xoff_left ) {
	st->xoff_left = xoff;
	if ( st->hsb!=NULL )
	    GScrollBarSetPos(&st->hsb->g,xoff);
	refresh = true;
    }
    if ( loff!=st->loff_top ) {
	st->loff_top = loff;
	if ( st->vsb!=NULL )
	    GScrollBarSetPos(&st->vsb->g,st->lineheights[loff].y);
	refresh = true;
    }
    GTPositionGIC(st);
return( refresh );
}

static void *genunicodedata(void *_gt,int32 *len) {
    SFTextArea *st = _gt;
    *len = st->sel_end-st->sel_start;
return( u_copyn(st->text+st->sel_start,st->sel_end-st->sel_start));
}

static void *genutf8data(void *_gt,int32 *len) {
    GTextField *gt = _gt;
    unichar_t *temp =u_copyn(gt->text+gt->sel_start,gt->sel_end-gt->sel_start);
    char *ret = u2utf8_copy(temp);
    free(temp);
    *len = strlen(ret);
return( ret );
}

static void *ddgenunicodedata(void *_gt,int32 *len) {
    void *temp = genunicodedata(_gt,len);
    SFTextArea *st = _gt;
    _SFTextAreaReplace(st,nullstr);
    _ggadget_redraw(&st->g);
return( temp );
}

static void *genlocaldata(void *_gt,int32 *len) {
    SFTextArea *st = _gt;
    unichar_t *temp =u_copyn(st->text+st->sel_start,st->sel_end-st->sel_start);
    char *ret = u2def_copy(temp);
    free(temp);
    *len = strlen(ret);
return( ret );
}

static void *ddgenlocaldata(void *_gt,int32 *len) {
    void *temp = genlocaldata(_gt,len);
    SFTextArea *st = _gt;
    _SFTextAreaReplace(st,nullstr);
    _ggadget_redraw(&st->g);
return( temp );
}

static void noop(void *_st) {
}

static void SFTextAreaGrabPrimarySelection(SFTextArea *st) {
    int ss = st->sel_start, se = st->sel_end;

    GDrawGrabSelection(st->g.base,sn_primary);
    st->sel_start = ss; st->sel_end = se;
    GDrawAddSelectionType(st->g.base,sn_primary,"text/plain;charset=ISO-10646-UCS-2",st,st->sel_end-st->sel_start,
	    sizeof(unichar_t),
	    genunicodedata,noop);
    GDrawAddSelectionType(st->g.base,sn_primary,"UTF8_STRING",st,3*(st->sel_end-st->sel_start),
	    sizeof(unichar_t),
	    genutf8data,noop);
    GDrawAddSelectionType(st->g.base,sn_primary,"STRING",st,st->sel_end-st->sel_start,sizeof(char),
	    genlocaldata,noop);
}

static void SFTextAreaGrabDDSelection(SFTextArea *st) {

    GDrawGrabSelection(st->g.base,sn_drag_and_drop);
    GDrawAddSelectionType(st->g.base,sn_drag_and_drop,"text/plain;charset=ISO-10646-UCS-2",st,st->sel_end-st->sel_start,
	    sizeof(unichar_t),
	    ddgenunicodedata,noop);
    GDrawAddSelectionType(st->g.base,sn_drag_and_drop,"STRING",st,st->sel_end-st->sel_start,sizeof(char),
	    ddgenlocaldata,noop);
}

static void SFTextAreaGrabSelection(SFTextArea *st, enum selnames sel ) {

    if ( st->sel_start!=st->sel_end ) {
	unichar_t *temp;
	char *ctemp;
	GDrawGrabSelection(st->g.base,sel);
	temp = u_copyn(st->text+st->sel_start,st->sel_end-st->sel_start);
	ctemp = u2utf8_copy(temp);
	GDrawAddSelectionType(st->g.base,sel,"text/plain;charset=ISO-10646-UCS-2",temp,u_strlen(temp),sizeof(unichar_t),
		NULL,NULL);
	GDrawAddSelectionType(st->g.base,sel,"UTF8_STRING",ctemp,strlen(ctemp),sizeof(char),
		NULL,NULL);
	GDrawAddSelectionType(st->g.base,sel,"STRING",u2def_copy(temp),u_strlen(temp),sizeof(char),
		NULL,NULL);
    }
}

static int SFTextAreaSelBackword(unichar_t *text,int start) {
    unichar_t ch = text[start-1];

    if ( start==0 )
	/* Can't go back */;
    else if ( isalnum(ch) || ch=='_' ) {
	int i;
	for ( i=start-1; i>=0 && (isalnum(text[i]) || text[i]=='_') ; --i );
	start = i+1;
    } else {
	int i;
	for ( i=start-1; i>=0 && !isalnum(text[i]) && text[i]!='_' ; --i );
	start = i+1;
    }
return( start );
}

static int SFTextAreaSelForeword(unichar_t *text,int end) {
    unichar_t ch = text[end];

    if ( ch=='\0' )
	/* Nothing */;
    else if ( isalnum(ch) || ch=='_' ) {
	int i;
	for ( i=end; isalnum(text[i]) || text[i]=='_' ; ++i );
	end = i;
    } else {
	int i;
	for ( i=end; !isalnum(text[i]) && text[i]!='_' && text[i]!='\0' ; ++i );
	end = i;
    }
return( end );
}

static void SFTextAreaSelectWord(SFTextArea *st,int mid, int16 *start, int16 *end) {
    unichar_t *text;
    unichar_t ch = st->text[mid];

    if ( st->dobitext ) {
	text = st->bidata.text;
	mid = SFTextAreaGetOffsetFromOffset(st,SFTextAreaFindLine(st,mid),mid);
    } else
	text = st->text;
    ch = text[mid];

    if ( ch=='\0' )
	*start = *end = mid;
    else if ( isspace(ch) ) {
	int i;
	for ( i=mid; isspace(text[i]); ++i );
	*end = i;
	for ( i=mid-1; i>=0 && isspace(text[i]) ; --i );
	*start = i+1;
    } else if ( isalnum(ch) || ch=='_' ) {
	int i;
	for ( i=mid; isalnum(text[i]) || text[i]=='_' ; ++i );
	*end = i;
	for ( i=mid-1; i>=0 && (isalnum(text[i]) || text[i]=='_') ; --i );
	*start = i+1;
    } else {
	int i;
	for ( i=mid; !isalnum(text[i]) && text[i]!='_' && text[i]!='\0' ; ++i );
	*end = i;
	for ( i=mid-1; i>=0 && !isalnum(text[i]) && text[i]!='_' ; --i );
	*start = i+1;
    }

    if ( st->dobitext ) {
	*start = st->bidata.original[*start]-st->text;
	*end = st->bidata.original[*end]-st->text;
    }
}

static void SFTextAreaSelectWords(SFTextArea *st,int last) {
    int16 ss, se;
    SFTextAreaSelectWord(st,st->sel_base,&st->sel_start,&st->sel_end);
    if ( last!=st->sel_base ) {
	SFTextAreaSelectWord(st,last,&ss,&se);
	if ( ss<st->sel_start ) st->sel_start = ss;
	if ( se>st->sel_end ) st->sel_end = se;
    }
}

static void SFTextAreaPaste(SFTextArea *st,enum selnames sel) {
    if ( GDrawSelectionHasType(st->g.base,sel,"Unicode") ||
	    GDrawSelectionHasType(st->g.base,sel,"text/plain;charset=ISO-10646-UCS-2")) {
	unichar_t *temp;
	int32 len;
	if ( GDrawSelectionHasType(st->g.base,sel,"Unicode") )
	    temp = GDrawRequestSelection(st->g.base,sel,"Unicode",&len);
	else
	    temp = GDrawRequestSelection(st->g.base,sel,"text/plain;charset=ISO-10646-UCS-2",&len);
	if ( temp!=NULL ) 
	    SFTextArea_Replace(st,temp);
	free(temp);
    } else if ( GDrawSelectionHasType(st->g.base,sel,"UTF8_STRING") ||
	    GDrawSelectionHasType(st->g.base,sel,"text/plain;charset=UTF-8")) {
	unichar_t *temp; char *ctemp;
	int32 len;
	if ( GDrawSelectionHasType(st->g.base,sel,"UTF8_STRING") )
	    ctemp = GDrawRequestSelection(st->g.base,sel,"UTF8_STRING",&len);
	else
	    ctemp = GDrawRequestSelection(st->g.base,sel,"text/plain;charset=UTF-8",&len);
	if ( ctemp!=NULL ) {
	    temp = utf82u_copyn(ctemp,strlen(ctemp));
	    SFTextArea_Replace(st,temp);
	    free(ctemp); free(temp);
	}
    } else if ( GDrawSelectionHasType(st->g.base,sel,"STRING")) {
	unichar_t *temp; char *ctemp;
	int32 len;
	ctemp = GDrawRequestSelection(st->g.base,sel,"STRING",&len);
	if ( ctemp!=NULL ) {
	    temp = def2u_copy(ctemp);
	    SFTextArea_Replace(st,temp);
	    free(ctemp); free(temp);
	}
    }
}

static int sftextarea_editcmd(GGadget *g,enum editor_commands cmd) {
    SFTextArea *st = (SFTextArea *) g;

    switch ( cmd ) {
      case ec_selectall:
	st->sel_start = 0;
	st->sel_end = u_strlen(st->text);
return( true );
      case ec_clear:
	SFTextArea_Replace(st,nullstr);
return( true );
      case ec_cut:
	SFTextAreaGrabSelection(st,sn_clipboard);
	SFTextArea_Replace(st,nullstr);
return( true );
      case ec_copy:
	SFTextAreaGrabSelection(st,sn_clipboard);
return( true );
      case ec_paste:
	SFTextAreaPaste(st,sn_clipboard);
	SFTextArea_Show(st,st->sel_start);
return( true );
      case ec_undo:
	if ( st->oldtext!=NULL ) {
	    unichar_t *temp = st->text;
	    struct fontlist *ofl = st->fontlist;
	    int16 s;
	    st->text = st->oldtext; st->oldtext = temp;
	    st->fontlist = st->oldfontlist; st->oldfontlist = ofl;
	    s = st->sel_start; st->sel_start = st->sel_oldstart; st->sel_oldstart = s;
	    s = st->sel_end; st->sel_end = st->sel_oldend; st->sel_oldend = s;
	    s = st->sel_base; st->sel_base = st->sel_oldbase; st->sel_oldbase = s;
	    SFTextAreaRefigureLines(st, 0);
	    SFTextArea_Show(st,st->sel_end);
	}
return( true );
      case ec_redo:		/* Hmm. not sure */ /* we don't do anything */
return( true );			/* but probably best to return success */
      case ec_backword:
        if ( st->sel_start==st->sel_end && st->sel_start!=0 ) {
	    if ( st->dobitext ) {
		int sel = SFTextAreaGetOffsetFromOffset(st,SFTextAreaFindLine(st,st->sel_start),st->sel_start);
		sel = SFTextAreaSelBackword(st->bidata.text,sel);
		st->sel_start = st->bidata.original[sel]-st->text;
	    } else
		st->sel_start = SFTextAreaSelBackword(st->text,st->sel_start);
	}
	SFTextArea_Replace(st,nullstr);
return( true );
      case ec_deleteword:
        if ( st->sel_start==st->sel_end && st->sel_start!=0 )
	    SFTextAreaSelectWord(st,st->sel_start,&st->sel_start,&st->sel_end);
	SFTextArea_Replace(st,nullstr);
return( true );
    }
return( false );
}

static int _sftextarea_editcmd(GGadget *g,enum editor_commands cmd) {
    if ( sftextarea_editcmd(g,cmd)) {
	_ggadget_redraw(g);
	GTPositionGIC((SFTextArea *) g);
return( true );
    }
return( false );
}

static int GTBackPos(SFTextArea *st,int pos, int ismeta) {
    int newpos,sel;

    if ( ismeta && !st->dobitext )
	newpos = SFTextAreaSelBackword(st->text,pos);
    else if ( ismeta ) {
	sel = SFTextAreaGetOffsetFromOffset(st,SFTextAreaFindLine(st,pos),pos);
	newpos = SFTextAreaSelBackword(st->bidata.text,sel);
	newpos = st->bidata.original[newpos]-st->text;
    } else if ( !st->dobitext )
	newpos = pos-1;
    else {
	sel = SFTextAreaGetOffsetFromOffset(st,SFTextAreaFindLine(st,pos),pos);
	if ( sel!=0 ) --sel;
	newpos = st->bidata.original[sel]-st->text;
    }
    if ( newpos==-1 ) newpos = pos;
return( newpos );
}

static int GTForePos(SFTextArea *st,int pos, int ismeta) {
    int newpos=pos,sel;

    if ( ismeta && !st->dobitext )
	newpos = SFTextAreaSelForeword(st->text,pos);
    else if ( ismeta ) {
	sel = SFTextAreaGetOffsetFromOffset(st,SFTextAreaFindLine(st,pos),pos);
	newpos = SFTextAreaSelForeword(st->bidata.text,sel);
	newpos = st->bidata.original[newpos]-st->text;
    } else if ( !st->dobitext ) {
	if ( st->text[pos]!=0 )
	    newpos = pos+1;
    } else {
	sel = SFTextAreaGetOffsetFromOffset(st,SFTextAreaFindLine(st,pos),pos);
	if ( st->text[sel]!=0 )
	    ++sel;
	newpos = st->bidata.original[sel]-st->text;
    }
return( newpos );
}

static void SFTextAreaImport(SFTextArea *st) {
    char *cret = gwwv_open_filename(_("Open"),NULL,
	    "*.txt",NULL);
    unichar_t *str;

    if ( cret==NULL )
return;
    str = _GGadgetFileToUString(cret,65536);
    if ( str==NULL ) {
	gwwv_post_error(_("Could not open"),_("Could not open %.100s"),cret);
	free(cret);
return;
    }
    free(cret);
    SFTextArea_Replace(st,str);
    free(str);
}

static void SFTextAreaSave(SFTextArea *st) {
    char *cret = gwwv_save_filename(_("Save"),NULL, "*.txt");
    FILE *file;
    unichar_t *pt;

    if ( cret==NULL )
return;
    file = fopen(cret,"w");
    if ( file==NULL ) {
	gwwv_post_error(_("Could not open"),_("Could not open %.100s"),cret);
	free(cret);
return;
    }
    free(cret);

    putc(0xfeff>>8,file);		/* Zero width something or other. Marks this as unicode */
    putc(0xfeff&0xff,file);
    for ( pt = st->text ; *pt; ++pt ) {
	putc(*pt>>8,file);
	putc(*pt&0xff,file);
    }
    fclose(file);
}

#define MID_Cut		1
#define MID_Copy	2
#define MID_Paste	3

#define MID_SelectAll	4

#define MID_Save	5
#define MID_Import	6

#define MID_Undo	7

static SFTextArea *popup_kludge;

static void SFTFPopupInvoked(GWindow v, GMenuItem *mi,GEvent *e) {
    SFTextArea *st;
    if ( popup_kludge==NULL )
return;
    st = popup_kludge;
    popup_kludge = NULL;
    switch ( mi->mid ) {
      case MID_Undo:
	sftextarea_editcmd(&st->g,ec_undo);
      break;
      case MID_Cut:
	sftextarea_editcmd(&st->g,ec_cut);
      break;
      case MID_Copy:
	sftextarea_editcmd(&st->g,ec_copy);
      break;
      case MID_Paste:
	sftextarea_editcmd(&st->g,ec_paste);
      break;
      case MID_SelectAll:
	sftextarea_editcmd(&st->g,ec_selectall);
      break;
      case MID_Save:
	SFTextAreaSave(st);
      break;
      case MID_Import:
	SFTextAreaImport(st);
      break;
    }
}

static GMenuItem sftf_popuplist[] = {
    { { (unichar_t *) N_("_Undo"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'U' }, 'Z', ksm_control, NULL, NULL, SFTFPopupInvoked, MID_Undo },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Cu_t"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, 'X', ksm_control, NULL, NULL, SFTFPopupInvoked, MID_Cut },
    { { (unichar_t *) N_("_Copy"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, 'C', ksm_control, NULL, NULL, SFTFPopupInvoked, MID_Copy },
    { { (unichar_t *) N_("_Paste"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, 'V', ksm_control, NULL, NULL, SFTFPopupInvoked, MID_Paste },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Save"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, 'S', ksm_control, NULL, NULL, SFTFPopupInvoked, MID_Save },
    { { (unichar_t *) N_("_Import..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, 'I', ksm_control, NULL, NULL, SFTFPopupInvoked, MID_Import },
    { NULL }
};

static void SFTFPopupMenu(SFTextArea *st, GEvent *event) {
    int no_sel = st->sel_start==st->sel_end;
    static int done = false;

    if ( !done ) {
	int i;
	for ( i=0; sftf_popuplist[i].ti.text!=NULL || sftf_popuplist[i].ti.line; ++i )
	    if ( sftf_popuplist[i].ti.text!=NULL )
		sftf_popuplist[i].ti.text = (unichar_t *) _( (char *) sftf_popuplist[i].ti.text);
	done = true;
    }

    sftf_popuplist[0].ti.disabled = st->oldtext==NULL;	/* Undo */
    sftf_popuplist[2].ti.disabled = no_sel;		/* Cut */
    sftf_popuplist[3].ti.disabled = no_sel;		/* Copy */
    sftf_popuplist[4].ti.disabled = !GDrawSelectionHasType(st->g.base,sn_clipboard,"text/plain;charset=ISO-10646-UCS-2") &&
	    !GDrawSelectionHasType(st->g.base,sn_clipboard,"UTF8_STRING") &&
	    !GDrawSelectionHasType(st->g.base,sn_clipboard,"STRING");
    popup_kludge = st;
    GMenuCreatePopupMenu(st->g.base,event, sftf_popuplist);
}

static int SFTextAreaDoChange(SFTextArea *st, GEvent *event) {
    int ss = st->sel_start, se = st->sel_end;
    int pos, l, xpos, sel;
    unichar_t *upt;

    if ( ( event->u.chr.state&(ksm_control|ksm_meta)) ||
	    event->u.chr.chars[0]<' ' || event->u.chr.chars[0]==0x7f ) {
	switch ( event->u.chr.keysym ) {
	  case GK_BackSpace:
	    if ( st->sel_start==st->sel_end ) {
		if ( st->sel_start==0 )
return( 2 );
		--st->sel_start;
	    }
	    SFTextArea_Replace(st,nullstr);
return( true );
	  break;
	  case GK_Delete:
	    if ( st->sel_start==st->sel_end ) {
		if ( st->text[st->sel_start]==0 )
return( 2 );
		++st->sel_end;
	    }
	    SFTextArea_Replace(st,nullstr);
return( true );
	  break;
	  case GK_Left: case GK_KP_Left:
	    if ( st->sel_start==st->sel_end ) {
		st->sel_start = GTBackPos(st,st->sel_start,event->u.chr.state&ksm_meta);
		if ( !(event->u.chr.state&ksm_shift ))
		    st->sel_end = st->sel_start;
	    } else if ( event->u.chr.state&ksm_shift ) {
		if ( st->sel_end==st->sel_base ) {
		    st->sel_start = GTBackPos(st,st->sel_start,event->u.chr.state&ksm_meta);
		} else {
		    st->sel_end = GTBackPos(st,st->sel_end,event->u.chr.state&ksm_meta);
		}
	    } else {
		st->sel_end = st->sel_base = st->sel_start;
	    }
	    SFTextArea_Show(st,st->sel_start);
return( 2 );
	  break;
	  case GK_Right: case GK_KP_Right:
	    if ( st->sel_start==st->sel_end ) {
		st->sel_end = GTForePos(st,st->sel_start,event->u.chr.state&ksm_meta);
		if ( !(event->u.chr.state&ksm_shift ))
		    st->sel_start = st->sel_end;
	    } else if ( event->u.chr.state&ksm_shift ) {
		if ( st->sel_end==st->sel_base ) {
		    st->sel_start = GTForePos(st,st->sel_start,event->u.chr.state&ksm_meta);
		} else {
		    st->sel_end = GTForePos(st,st->sel_end,event->u.chr.state&ksm_meta);
		}
	    } else {
		st->sel_start = st->sel_base = st->sel_end;
	    }
	    SFTextArea_Show(st,st->sel_start);
return( 2 );
	  break;
	  case GK_Up: case GK_KP_Up:
	    if ( !st->multi_line )
	  break;
	    if ( !( event->u.chr.state&ksm_shift ) && st->sel_start!=st->sel_end )
		st->sel_end = st->sel_base = st->sel_start;
	    else {
		pos = st->sel_start;
		if ( ( event->u.chr.state&ksm_shift ) && st->sel_start==st->sel_base )
		    pos = st->sel_end;
		l = SFTextAreaFindLine(st,st->sel_start);
		sel = SFTextAreaGetOffsetFromOffset(st,l,st->sel_start);
		xpos = STGetTextWidth(st,st->lines[l],sel-st->lines[l]);
		if ( l!=0 )
		    pos = SFTextAreaGetPtFromPos(st,l-1,xpos) - st->text;
		if ( event->u.chr.state&ksm_shift ) {
		    if ( pos<st->sel_base ) {
			st->sel_start = pos;
			st->sel_end = st->sel_base;
		    } else {
			st->sel_start = st->sel_base;
			st->sel_end = pos;
		    }
		} else {
		    st->sel_start = st->sel_end = st->sel_base = pos;
		}
	    }
	    SFTextArea_Show(st,st->sel_start);
return( 2 );
	  break;
	  case GK_Down: case GK_KP_Down:
	    if ( !st->multi_line )
	  break;
	    if ( !( event->u.chr.state&ksm_shift ) && st->sel_start!=st->sel_end )
		st->sel_end = st->sel_base = st->sel_end;
	    else {
		pos = st->sel_start;
		if ( ( event->u.chr.state&ksm_shift ) && st->sel_start==st->sel_base )
		    pos = st->sel_end;
		l = SFTextAreaFindLine(st,st->sel_start);
		sel = SFTextAreaGetOffsetFromOffset(st,l,st->sel_start);
		xpos = STGetTextWidth(st,st->lines[l],sel-st->lines[l]);
		if ( l<st->lcnt-1 )
		    pos = SFTextAreaGetPtFromPos(st,l+1,xpos) - st->text;
		if ( event->u.chr.state&ksm_shift ) {
		    if ( pos<st->sel_base ) {
			st->sel_start = pos;
			st->sel_end = st->sel_base;
		    } else {
			st->sel_start = st->sel_base;
			st->sel_end = pos;
		    }
		} else {
		    st->sel_start = st->sel_end = st->sel_base = pos;
		}
	    }
	    SFTextArea_Show(st,st->sel_start);
return( 2 );
	  break;
	  case GK_Home: case GK_Begin: case GK_KP_Home: case GK_KP_Begin:
	    if ( !(event->u.chr.state&ksm_shift) ) {
		st->sel_start = st->sel_base = st->sel_end = 0;
	    } else {
		st->sel_start = 0; st->sel_end = st->sel_base;
	    }
	    SFTextArea_Show(st,st->sel_start);
return( 2 );
	  break;
	  /* Move to eol. (if already at eol, move to next eol) */
	  case 'E': case 'e':
	    if ( !( event->u.chr.state&ksm_control ) )
return( false );
	    upt = st->text+st->sel_base;
	    if ( *upt=='\n' )
		++upt;
	    upt = u_strchr(upt,'\n');
	    if ( upt==NULL ) upt=st->text+u_strlen(st->text);
	    if ( !(event->u.chr.state&ksm_shift) ) {
		st->sel_start = st->sel_base = st->sel_end =upt-st->text;
	    } else {
		st->sel_start = st->sel_base; st->sel_end = upt-st->text;
	    }
	    SFTextArea_Show(st,st->sel_start);
return( 2 );
	  break;
	  case GK_End: case GK_KP_End:
	    if ( !(event->u.chr.state&ksm_shift) ) {
		st->sel_start = st->sel_base = st->sel_end = u_strlen(st->text);
	    } else {
		st->sel_start = st->sel_base; st->sel_end = u_strlen(st->text);
	    }
	    SFTextArea_Show(st,st->sel_start);
return( 2 );
	  break;
	  case 'A': case 'a':
	    if ( event->u.chr.state&ksm_control ) {	/* Select All */
		sftextarea_editcmd(&st->g,ec_selectall);
return( 2 );
	    }
	  break;
	  case 'C': case 'c':
	    if ( event->u.chr.state&ksm_control ) {	/* Copy */
		sftextarea_editcmd(&st->g,ec_copy);
	    }
	  break;
	  case 'V': case 'v':
	    if ( event->u.chr.state&ksm_control ) {	/* Paste */
		sftextarea_editcmd(&st->g,ec_paste);
		SFTextArea_Show(st,st->sel_start);
return( true );
	    }
	  break;
	  case 'X': case 'x':
	    if ( event->u.chr.state&ksm_control ) {	/* Cut */
		sftextarea_editcmd(&st->g,ec_cut);
		SFTextArea_Show(st,st->sel_start);
return( true );
	    }
	  break;
	  case 'Z': case 'z':				/* Undo */
	    if ( event->u.chr.state&ksm_control ) {
		sftextarea_editcmd(&st->g,ec_undo);
		SFTextArea_Show(st,st->sel_start);
return( true );
	    }
	  break;
	  case 'D': case 'd':
	    if ( event->u.chr.state&ksm_control ) {	/* delete word */
		sftextarea_editcmd(&st->g,ec_deleteword);
		SFTextArea_Show(st,st->sel_start);
return( true );
	    }
	  break;
	  case 'W': case 'w':
	    if ( event->u.chr.state&ksm_control ) {	/* backword */
		sftextarea_editcmd(&st->g,ec_backword);
		SFTextArea_Show(st,st->sel_start);
return( true );
	    }
	  break;
	  case 'M': case 'm': case 'J': case 'j':
	    if ( !( event->u.chr.state&ksm_control ) )
return( false );
	    /* fall through into return case */
	  case GK_Return: case GK_Linefeed:
	    if ( st->accepts_returns ) {
		SFTextArea_Replace(st,newlinestr);
return( true );
	    }
	  break;
	  case GK_Tab:
	    if ( st->accepts_tabs ) {
		SFTextArea_Replace(st,tabstr);
return( true );
	    }
	  break;
	  case 's': case 'S':
	    if ( !( event->u.chr.state&ksm_control ) )
return( false );
	    SFTextAreaSave(st);
return( 2 );
	  break;
	  case 'I': case 'i':
	    if ( !( event->u.chr.state&ksm_control ) )
return( false );
	    SFTextAreaImport(st);
return( true );
	}
    } else {
	SFTextArea_Replace(st,event->u.chr.chars);
return( true );
    }

    if ( st->sel_start == st->sel_end )
	st->sel_base = st->sel_start;
    if ( ss!=st->sel_start || se!=st->sel_end )
	SFTextAreaGrabPrimarySelection(st);
return( false );
}

static void gt_cursor_pos(SFTextArea *st, int *x, int *y, int *fh) {
    int l, sel, ty;

    *x = -1; *y= -1;
    GDrawSetFont(st->g.base,st->font);
    l = SFTextAreaFindLine(st,st->sel_start);
    ty = st->lineheights[l].y - st->lineheights[st->loff_top].y;
    if ( ty<0 || ty>st->g.inner.height )
return;
    *y = ty;
    *fh = st->lineheights[l].fh;
    sel = SFTextAreaGetOffsetFromOffset(st,l,st->sel_start);
    *x = STGetTextWidth(st,st->lines[l],sel-st->lines[l])-
	    st->xoff_left;
}

static void GTPositionGIC(SFTextArea *st) {
    int x,y,fh;

    if ( !st->g.has_focus || st->gic==NULL )
return;
    gt_cursor_pos(st,&x,&y,&fh);
    if ( x<0 )
return;
    GDrawSetGIC(st->g.base,st->gic,st->g.inner.x+x,st->g.inner.y+y+st->as);
}

static void gt_draw_cursor(GWindow pixmap, SFTextArea *st) {
    GRect old;
    int x, y, fh;

    if ( !st->cursor_on || st->sel_start != st->sel_end )
return;
    gt_cursor_pos(st,&x,&y,&fh);

    if ( x<0 || x>=st->g.inner.width )
return;
    GDrawPushClip(pixmap,&st->g.inner,&old);
    GDrawSetXORMode(pixmap);
    GDrawSetXORBase(pixmap,st->g.box->main_background!=COLOR_DEFAULT?st->g.box->main_background:
	    GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(pixmap)) );
    GDrawSetFont(pixmap,st->font);
    GDrawSetLineWidth(pixmap,0);
    GDrawDrawLine(pixmap,st->g.inner.x+x,st->g.inner.y+y,
	    st->g.inner.x+x,st->g.inner.y+y+fh,
	    st->g.box->main_foreground!=COLOR_DEFAULT?st->g.box->main_foreground:
	    GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap)) );
    GDrawSetCopyMode(pixmap);
    GDrawPopClip(pixmap,&old);
}

static void SFTextAreaDrawDDCursor(SFTextArea *st, int pos) {
    GRect old;
    int x, y, l;

    l = SFTextAreaFindLine(st,pos);
    y = st->lineheights[l].y - st->lineheights[st->loff_top].y;
    if ( y<0 || y>st->g.inner.height )
return;
    pos = SFTextAreaGetOffsetFromOffset(st,l,pos);
    x = STGetTextWidth(st,st->lines[l],pos-st->lines[l])-
	    st->xoff_left;
    if ( x<0 || x>=st->g.inner.width )
return;

    GDrawPushClip(st->g.base,&st->g.inner,&old);
    GDrawSetXORMode(st->g.base);
    GDrawSetXORBase(st->g.base,st->g.box->main_background!=COLOR_DEFAULT?st->g.box->main_background:
	    GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(st->g.base)) );
    GDrawSetFont(st->g.base,st->font);
    GDrawSetLineWidth(st->g.base,0);
    GDrawSetDashedLine(st->g.base,2,2,0);
    GDrawDrawLine(st->g.base,st->g.inner.x+x,st->g.inner.y+y,
	    st->g.inner.x+x,st->g.inner.y+y+st->lineheights[l].fh,
	    st->g.box->main_foreground!=COLOR_DEFAULT?st->g.box->main_foreground:
	    GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(st->g.base)) );
    GDrawSetCopyMode(st->g.base);
    GDrawPopClip(st->g.base,&old);
    GDrawSetDashedLine(st->g.base,0,0,0);
    st->has_dd_cursor = !st->has_dd_cursor;
    st->dd_cursor_pos = pos;
}


static void SFTextAreaDrawLineSel(GWindow pixmap, SFTextArea *st, int line, Color fg, Color sel ) {
    GRect selr;
    int s,e, y,llen,i,j, fh;

    y = st->g.inner.y+ st->lineheights[line].y-st->lineheights[st->loff_top].y;
    fh = st->lineheights[line].fh;
    selr = st->g.inner; selr.y = y; selr.height = fh;
    if ( !st->g.has_focus ) --selr.height;
    llen = st->lines[line+1]==-1?
	    u_strlen(st->text+st->lines[line])+st->lines[line]:
	    st->lines[line+1];
    s = st->sel_start<st->lines[line]?st->lines[line]:st->sel_start;
    e = st->sel_end>st->lines[line+1] && st->lines[line+1]!=-1?st->lines[line+1]-1:
	    st->sel_end;

    GDrawSetLineWidth(pixmap,0);
    if ( !st->dobitext ) {
	if ( st->sel_start>st->lines[line] )
	    selr.x += STGetTextWidth(st,st->lines[line],st->sel_start-st->lines[line])-
		    st->xoff_left;
	if ( st->sel_end <= st->lines[line+1] || st->lines[line+1]==-1 )
	    selr.width = STGetTextWidth(st,st->lines[line],st->sel_end-st->lines[line])-
		    st->xoff_left - (selr.x-st->g.inner.x);
	if ( st->g.has_focus )
	    GDrawFillRect(pixmap,&selr,st->g.box->active_border);
	else
	    GDrawDrawRect(pixmap,&selr,st->g.box->active_border);
	if ( sel!=fg ) {
	    STDrawText(st,pixmap,selr.x,y+st->as, s,e-s, sel );
	}
    } else {
	/* in bidirectional text the selection can be all over the */
	/*  place, so look for contiguous regions of text within the*/
	/*  selection and draw them */
	for ( i=st->lines[line]; i<llen; ++i ) {
	    if ( st->bidata.original[i]-st->text >= s &&
		    st->bidata.original[i]-st->text < e ) {
		for ( j=i+1 ; j<llen &&
			st->bidata.original[j]-st->text >= s &&
			st->bidata.original[j]-st->text < e; ++j );
		selr.x = STGetTextWidth(st,st->lines[line],i-st->lines[line])+
			st->g.inner.x - st->xoff_left;
		selr.width = STGetTextWidth(st,i,j-i);
		if ( st->g.has_focus )
		    GDrawFillRect(pixmap,&selr,st->g.box->active_border);
		else
		    GDrawDrawRect(pixmap,&selr,st->g.box->active_border);
		if ( sel!=fg )
		    STDrawText(st,pixmap,selr.x,y+st->as, i,j-i, sel );
		i = j-1;
	    }
	}
    }
}

static int sftextarea_expose(GWindow pixmap, GGadget *g, GEvent *event) {
    SFTextArea *st = (SFTextArea *) g;
    GRect old1, old2, *r = &g->r;
    Color fg,sel;
    int y,ll,i;
    unichar_t *bitext = st->dobitext ?st->bidata.text:st->text;

    if ( g->state == gs_invisible || st->dontdraw )
return( false );

    GDrawPushClip(pixmap,r,&old1);

    GBoxDrawBackground(pixmap,r,g->box,
	    g->state==gs_enabled? gs_pressedactive: g->state,false);
    GBoxDrawBorder(pixmap,r,g->box,g->state,false);

    GDrawPushClip(pixmap,&g->inner,&old2);
    GDrawSetFont(pixmap,st->font);
    GDrawSetDither(NULL, false);	/* on 8 bit displays we don't want any dithering */
    GDrawSetLineWidth(pixmap,0);

    fg = g->state==gs_disabled?g->box->disabled_foreground:
		    g->box->main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap)):
		    g->box->main_foreground;
    for ( i=st->loff_top; st->lines[i]!=-1; ++i ) {
	/* there is an odd complication in drawing each line. */
	/* normally we draw the selection rectangle(s) and then draw the text */
	/*  on top of that all in one go. But that doesn't work if the select */
	/*  color is the same as the foreground color (bw displays). In that */
	/*  case we draw the text first, draw the rectangles, and draw the text*/
	/*  within the rectangles */
	y = g->inner.y+ st->lineheights[i].y-st->lineheights[st->loff_top].y;
	if ( y>g->inner.y+g->inner.height || y>event->u.expose.rect.y+event->u.expose.rect.height )
    break;
	if ( y+st->lineheights[i].fh<=event->u.expose.rect.y )
    continue;
	sel = fg;
	ll = st->lines[i+1]==-1?-1:st->lines[i+1]-st->lines[i];
	if ( st->sel_start != st->sel_end && st->sel_end>st->lines[i] &&
		(st->lines[i+1]==-1 || st->sel_start<st->lines[i+1])) {
	    if ( g->box->active_border==fg ) {
		sel = g->state==gs_disabled?g->box->disabled_background:
				g->box->main_background==COLOR_DEFAULT?GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(pixmap)):
				g->box->main_background;
		STDrawText(st,pixmap,g->inner.x-st->xoff_left,y+st->lineheights[i].as,
			st->lines[i],ll, fg );
	    }
	    SFTextAreaDrawLineSel(pixmap,st,i,fg,sel);
	}
	if ( sel==fg ) {
	    if ( i!=0 && (*(bitext+st->lines[i]+ll-1)=='\n' || *(bitext+st->lines[i]+ll-1)=='\r' ))
		--ll;
	    STDrawText(st,pixmap,g->inner.x-st->xoff_left,y+st->lineheights[i].as,
		    st->lines[i],ll, fg );
	}
    }

    GDrawSetDither(NULL, true);
    GDrawPopClip(pixmap,&old2);
    GDrawPopClip(pixmap,&old1);
    gt_draw_cursor(pixmap, st);
return( true );
}

static int SFTextAreaDoDrop(SFTextArea *st,GEvent *event,int endpos) {

    if ( st->has_dd_cursor )
	SFTextAreaDrawDDCursor(st,st->dd_cursor_pos);

    if ( event->type == et_mousemove ) {
	if ( GGadgetInnerWithin(&st->g,event->u.mouse.x,event->u.mouse.y) ) {
	    if ( endpos<st->sel_start || endpos>=st->sel_end )
		SFTextAreaDrawDDCursor(st,endpos);
	} else if ( !GGadgetWithin(&st->g,event->u.mouse.x,event->u.mouse.y) ) {
	    GDrawPostDragEvent(st->g.base,event,et_drag);
	}
    } else {
	if ( GGadgetInnerWithin(&st->g,event->u.mouse.x,event->u.mouse.y) ) {
	    if ( endpos>=st->sel_start && endpos<st->sel_end ) {
		st->sel_start = st->sel_end = endpos;
	    } else {
		unichar_t *old=st->oldtext, *temp;
		int pos=0;
		if ( event->u.mouse.state&ksm_control ) {
		    temp = galloc((u_strlen(st->text)+st->sel_end-st->sel_start+1)*sizeof(unichar_t));
		    memcpy(temp,st->text,endpos*sizeof(unichar_t));
		    memcpy(temp+endpos,st->text+st->sel_start,
			    (st->sel_end-st->sel_start)*sizeof(unichar_t));
		    u_strcpy(temp+endpos+st->sel_end-st->sel_start,st->text+endpos);
		} else if ( endpos>=st->sel_end ) {
		    temp = u_copy(st->text);
		    memcpy(temp+st->sel_start,temp+st->sel_end,
			    (endpos-st->sel_end)*sizeof(unichar_t));
		    memcpy(temp+endpos-(st->sel_end-st->sel_start),
			    st->text+st->sel_start,(st->sel_end-st->sel_start)*sizeof(unichar_t));
		    pos = endpos;
		} else /*if ( endpos<st->sel_start )*/ {
		    temp = u_copy(st->text);
		    memcpy(temp+endpos,st->text+st->sel_start,
			    (st->sel_end-st->sel_start)*sizeof(unichar_t));
		    memcpy(temp+endpos+st->sel_end-st->sel_start,st->text+endpos,
			    (st->sel_start-endpos)*sizeof(unichar_t));
		    pos = endpos+st->sel_end-st->sel_start;
		}
		st->oldtext = st->text;
		st->sel_oldstart = st->sel_start;
		st->sel_oldend = st->sel_end;
		st->sel_oldbase = st->sel_base;
		st->sel_start = st->sel_end = st->sel_end = pos;
		st->text = temp;
		free(old);
		SFTextAreaRefigureLines(st, endpos<st->sel_oldstart?endpos:st->sel_oldstart);
	    }
	} else if ( !GGadgetWithin(&st->g,event->u.mouse.x,event->u.mouse.y) ) {
	    /* Don't delete the selection until someone actually accepts the drop */
	    /* Don't delete at all (copy not move) if control key is down */
	    if ( ( event->u.mouse.state&ksm_control ) )
		SFTextAreaGrabSelection(st,sn_drag_and_drop);
	    else
		SFTextAreaGrabDDSelection(st);
	    GDrawPostDragEvent(st->g.base,event,et_drop);
	}
	st->drag_and_drop = false;
	GDrawSetCursor(st->g.base,st->old_cursor);
	_ggadget_redraw(&st->g);
    }
return( false );
}

static void STChangeCheck(SFTextArea *st) {
    struct fontlist *fl;

    for ( fl=st->fontlist; fl!=NULL && fl->end<st->sel_end; fl=fl->next );
    if ( fl==NULL || fl->fd==st->last_fd || st->changefontcallback==NULL )
return;
    (st->changefontcallback)(st->cbcontext,fl->fd->sf,fl->fd->fonttype,fl->fd->pixelsize,fl->fd->antialias);
    st->last_fd = fl->fd;
}
    
static int sftextarea_mouse(GGadget *g, GEvent *event) {
    SFTextArea *st = (SFTextArea *) g;
    unichar_t *end=NULL, *end1, *end2;
    int i=0,ll;
    unichar_t *bitext = st->dobitext ?st->bidata.text:st->text;

    if ( st->hidden_cursor ) {
	GDrawSetCursor(st->g.base,st->old_cursor);
	st->hidden_cursor = false;
	_GWidget_ClearGrabGadget(g);
    }
    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active && g->state!=gs_focused ))
return( false );
    if ( event->type == et_crossing )
return( false );
    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button==4 || event->u.mouse.button==5) &&
	    st->vsb!=NULL )
return( GGadgetDispatchEvent(&st->vsb->g,event));

    if ( st->pressed==NULL && event->type == et_mousemove && g->popup_msg!=NULL &&
	    GGadgetWithin(g,event->u.mouse.x,event->u.mouse.y))
	GGadgetPreparePopup(g->base,g->popup_msg);

    if ( event->type == et_mousedown && event->u.mouse.button==3 &&
	    GGadgetWithin(g,event->u.mouse.x,event->u.mouse.y)) {
	SFTFPopupMenu(st,event);
return( true );
    }

    if ( event->type == et_mousedown || st->pressed ) {
	for ( i=st->loff_top; i<st->lcnt-1 &&
		event->u.mouse.y-g->inner.y>=st->lineheights[i+1].y-st->lineheights[st->loff_top].y;
		++i );
	if ( i<0 ) i = 0;
	if ( !st->multi_line ) i = 0;
	if ( i>=st->lcnt )
	    end = st->text+u_strlen(st->text);
	else
	    end = SFTextAreaGetPtFromPos(st,i,event->u.mouse.x);
    }

    if ( event->type == et_mousedown ) {
	if ( i>=st->lcnt )
	    end1 = end2 = end;
	else {
	    ll = st->lines[i+1]==-1?-1:st->lines[i+1]-st->lines[i]-1;
	    STGetTextPtBeforePos(st,bitext+st->lines[i], ll, 
		    event->u.mouse.x-g->inner.x+st->xoff_left, &end1);
	    STGetTextPtAfterPos(st,bitext+st->lines[i], ll, 
		    event->u.mouse.x-g->inner.x+st->xoff_left, &end2);
	    if ( st->dobitext ) {
		end1 = st->bidata.original[end1-st->bidata.text];
		end2 = st->bidata.original[end2-st->bidata.text];
	    }
	}
	st->wordsel = st->linesel = false;
	if ( event->u.mouse.button==1 && event->u.mouse.clicks>=3 ) {
	    st->sel_start = st->lines[i]; st->sel_end = st->lines[i+1];
	    if ( st->sel_end==-1 ) st->sel_end = u_strlen(st->text);
	    st->wordsel = false; st->linesel = true;
	} else if ( event->u.mouse.button==1 && event->u.mouse.clicks==2 ) {
	    st->sel_start = st->sel_end = st->sel_base = end-st->text;
	    st->wordsel = true;
	    SFTextAreaSelectWords(st,st->sel_base);
	} else if ( end1-st->text>=st->sel_start && end2-st->text<st->sel_end &&
		st->sel_start!=st->sel_end &&
		event->u.mouse.button==1 ) {
	    st->drag_and_drop = true;
	    if ( !st->hidden_cursor )
		st->old_cursor = GDrawGetCursor(st->g.base);
	    GDrawSetCursor(st->g.base,ct_draganddrop);
	} else if ( event->u.mouse.button!=3 && !(event->u.mouse.state&ksm_shift) ) {
	    if ( event->u.mouse.button==1 )
		SFTextAreaGrabPrimarySelection(st);
	    st->sel_start = st->sel_end = st->sel_base = end-st->text;
	} else if ( end-st->text>st->sel_base ) {
	    st->sel_start = st->sel_base;
	    st->sel_end = end-st->text;
	} else {
	    st->sel_start = end-st->text;
	    st->sel_end = st->sel_base;
	}
	if ( st->pressed==NULL )
	    st->pressed = GDrawRequestTimer(st->g.base,200,100,NULL);
	if ( st->sel_start > u_strlen( st->text ))	/* Ok to have selection at end, but beyond is an error */
	    fprintf( stderr, "About to crash\n" );
	_ggadget_redraw(g);
	if ( st->changefontcallback )
	    STChangeCheck(st);
return( true );
    } else if ( st->pressed && (event->type == et_mousemove || event->type == et_mouseup )) {
	int refresh = true;

	if ( st->drag_and_drop ) {
	    refresh = SFTextAreaDoDrop(st,event,end-st->text);
	} else if ( st->linesel ) {
	    int j, e;
	    st->sel_start = st->lines[i]; st->sel_end = st->lines[i+1];
	    if ( st->sel_end==-1 ) st->sel_end = u_strlen(st->text);
	    for ( j=0; st->lines[i+1]!=-1 && st->sel_base>=st->lines[i+1]; ++j );
	    if ( st->sel_start<st->lines[i] ) st->sel_start = st->lines[i];
	    e = st->lines[j+1]==-1 ? u_strlen(st->text): st->lines[j+1];
	    if ( e>st->sel_end ) st->sel_end = e;
	} else if ( st->wordsel )
	    SFTextAreaSelectWords(st,end-st->text);
	else if ( event->u.mouse.button!=2 ) {
	    int e = end-st->text;
	    if ( e>st->sel_base ) {
		st->sel_start = st->sel_base; st->sel_end = e;
	    } else {
		st->sel_start = e; st->sel_end = st->sel_base;
	    }
	}
	if ( event->type==et_mouseup ) {
	    GDrawCancelTimer(st->pressed); st->pressed = NULL;
	    if ( event->u.mouse.button==2 )
		SFTextAreaPaste(st,sn_primary);
	    if ( st->sel_start==st->sel_end )
		SFTextArea_Show(st,st->sel_start);
	}
	if ( st->sel_end > u_strlen( st->text ))
	    fprintf( stderr, "About to crash\n" );
	if ( refresh )
	    _ggadget_redraw(g);
	if ( event->type==et_mouseup && st->changefontcallback )
	    STChangeCheck(st);
return( true );
    }
return( false );
}

static int sftextarea_key(GGadget *g, GEvent *event) {
    SFTextArea *st = (SFTextArea *) g;
    int ret;

    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active && g->state!=gs_focused ))
return( false );

    if ( event->type == et_charup )
return( false );
    if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ||
	    (event->u.chr.keysym == GK_Return && !st->accepts_returns ) ||
	    ( event->u.chr.keysym == GK_Tab && !st->accepts_tabs ) ||
	    event->u.chr.keysym == GK_BackTab || event->u.chr.keysym == GK_Escape )
return( false );

    if ( !st->hidden_cursor ) {	/* hide the mouse pointer */
	if ( !st->drag_and_drop )
	    st->old_cursor = GDrawGetCursor(st->g.base);
	GDrawSetCursor(g->base,ct_invisible);
	st->hidden_cursor = true;
	_GWidget_SetGrabGadget(g);	/* so that we get the next mouse movement to turn the cursor on */
    }
    if( st->cursor_on ) {	/* undraw the blinky text cursor if it is drawn */
	gt_draw_cursor(g->base, st);
	st->cursor_on = false;
    }

    ret = SFTextAreaDoChange(st,event);
    if ( st->changefontcallback )
	STChangeCheck(st);
    switch ( ret ) {
      case 2:
      break;
      case true:
	SFTextAreaChanged(st,-1);
      break;
      case false:
return( false );
    }
    _ggadget_redraw(g);
return( true );
}

static int sftextarea_focus(GGadget *g, GEvent *event) {
    SFTextArea *st = (SFTextArea *) g;
    if ( st->cursor!=NULL ) {
	GDrawCancelTimer(st->cursor);
	st->cursor = NULL;
	st->cursor_on = false;
    }
    if ( st->hidden_cursor && !event->u.focus.gained_focus ) {
	GDrawSetCursor(st->g.base,st->old_cursor);
	st->hidden_cursor = false;
    }
    st->g.has_focus = event->u.focus.gained_focus;
    if ( event->u.focus.gained_focus ) {
	st->cursor = GDrawRequestTimer(st->g.base,400,400,NULL);
	st->cursor_on = true;
	if ( event->u.focus.mnemonic_focus != mf_normal )
	    SFTextAreaSelect(&st->g,0,-1);
	if ( st->gic!=NULL )
	    GTPositionGIC(st);
    }
    _ggadget_redraw(g);
    SFTextAreaFocusChanged(st,event->u.focus.gained_focus);
return( true );
}

static int sftextarea_timer(GGadget *g, GEvent *event) {
    SFTextArea *st = (SFTextArea *) g;

    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active && g->state!=gs_focused ))
return(false);
    if ( st->cursor == event->u.timer.timer ) {
	if ( st->cursor_on ) {
	    gt_draw_cursor(g->base, st);
	    st->cursor_on = false;
	} else {
	    st->cursor_on = true;
	    gt_draw_cursor(g->base, st);
	}
return( true );
    }
    if ( st->pressed == event->u.timer.timer ) {
	GEvent e;
	GDrawSetFont(g->base,st->font);
	GDrawGetPointerPosition(g->base,&e);
	if ( (e.u.mouse.x<g->r.x && st->xoff_left>0 ) ||
		(st->multi_line && e.u.mouse.y<g->r.y && st->loff_top>0 ) ||
		( e.u.mouse.x >= g->r.x + g->r.width &&
			st->xmax-st->xoff_left>g->inner.width ) ||
		( e.u.mouse.y >= g->r.y + g->r.height &&
			st->lineheights[st->lcnt-1].y-st->lineheights[st->loff_top].y >= g->inner.height )) {
	    int l;
	    int xpos; unichar_t *end;

	    for ( l=st->loff_top; l<st->lcnt-1 && e.u.mouse.y-g->inner.y>st->lineheights[l+1].y-st->lineheights[st->loff_top].y;
		    ++l );
	    if ( e.u.mouse.y<g->r.y && st->loff_top>0 )
		l = --st->loff_top;
	    else if ( e.u.mouse.y >= g->r.y + g->r.height &&
			    st->lineheights[st->lcnt-1].y-st->lineheights[st->loff_top].y > g->inner.height ) {
		++st->loff_top;
		++l;
	    } else if ( l<st->loff_top )
		l = st->loff_top; 
	    else if ( st->lineheights[l].y>=st->lineheights[st->loff_top].y + g->inner.height ) {
		for ( l = st->loff_top+1; st->lineheights[l].y<st->lineheights[st->loff_top].y+g->inner.height; ++l );
		--l;
		if ( l==st->loff_top ) ++l;
	    }
	    if ( l>=st->lcnt ) l = st->lcnt-1;

	    xpos = e.u.mouse.x+st->xoff_left;
	    if ( e.u.mouse.x<g->r.x && st->xoff_left>0 ) {
		st->xoff_left -= st->nw;
		xpos = g->inner.x + st->xoff_left;
	    } else if ( e.u.mouse.x >= g->r.x + g->r.width &&
			    st->xmax-st->xoff_left>g->inner.width ) {
		st->xoff_left += st->nw;
		xpos = g->inner.x + st->xoff_left + g->inner.width;
	    }

	    end = SFTextAreaGetPtFromPos(st,l,xpos);
	    if ( end-st->text > st->sel_base ) {
		st->sel_start = st->sel_base;
		st->sel_end = end-st->text;
	    } else {
		st->sel_start = end-st->text;
		st->sel_end = st->sel_base;
	    }
	    _ggadget_redraw(g);
	    if ( st->vsb!=NULL )
		GScrollBarSetPos(&st->vsb->g,st->lineheights[st->loff_top].y);
	    if ( st->hsb!=NULL )
		GScrollBarSetPos(&st->hsb->g,st->xoff_left);
	}
return( true );
    }
return( false );
}

static int sftextarea_sel(GGadget *g, GEvent *event) {
    SFTextArea *st = (SFTextArea *) g;
    unichar_t *end;
    int i;

    if ( event->type == et_selclear ) {
	if ( event->u.selclear.sel==sn_primary && st->sel_start!=st->sel_end ) {
#if 0		/* Retain the drawn selection even if X says we don't own */
		/*  the selection property. Otherwise we can't change the */
		/*  fontsize (ie. must select the fontsize field) */
	    st->sel_start = st->sel_end = st->sel_base;
	    _ggadget_redraw(g);
#endif
return( true );
	}
return( false );
    }

    if ( st->has_dd_cursor )
	SFTextAreaDrawDDCursor(st,st->dd_cursor_pos);
    GDrawSetFont(g->base,st->font);
    for ( i=st->loff_top ; i<st->lcnt-1 && st->lineheights[i+1].y-st->lineheights[st->loff_top].y<
	    event->u.drag_drop.y-g->inner.y; ++i );
    if ( !st->multi_line ) i = 0;
    if ( i>=st->lcnt )
	end = st->text+u_strlen(st->text);
    else
	end = SFTextAreaGetPtFromPos(st,i,event->u.drag_drop.x);
    if ( event->type == et_drag ) {
	SFTextAreaDrawDDCursor(st,end-st->text);
    } else if ( event->type == et_dragout ) {
	/* this event exists simply to clear the dd cursor line. We've done */
	/*  that already */ 
    } else if ( event->type == et_drop ) {
	st->sel_start = st->sel_end = st->sel_base = end-st->text;
	SFTextAreaPaste(st,sn_drag_and_drop);
	SFTextArea_Show(st,st->sel_start);
	_ggadget_redraw(&st->g);
    } else
return( false );

return( true );
}

static void sftextarea_destroy(GGadget *g) {
    SFTextArea *st = (SFTextArea *) g;
    struct sfmaps *m, *n;
    FontData *fd, *nfd;

    if ( st==NULL )
return;

    if ( st->vsb!=NULL )
	(st->vsb->g.funcs->destroy)(&st->vsb->g);
    if ( st->hsb!=NULL )
	(st->hsb->g.funcs->destroy)(&st->hsb->g);
    GDrawCancelTimer(st->pressed);
    GDrawCancelTimer(st->cursor);
    free(st->lines);
    free(st->oldtext);
    free(st->text);
    free(st->bidata.text);
    free(st->bidata.level);
    free(st->bidata.override);
    free(st->bidata.type);
    free(st->bidata.original);
    fontlistfree(st->fontlist);
    fontlistfree(st->oldfontlist);
    for ( m=st->sfmaps; m!=NULL; m=n ) {
	n = m->next;
	EncMapFree(m->map);
	chunkfree(m,sizeof(struct sfmaps));
    }
    for ( fd=st->generated ; fd!=NULL; fd = nfd ) {
	nfd = fd->next;
	if ( fd->depends_on )
	    fd->bdf->freetype_context = NULL;
	if ( fd->fonttype!=sftf_bitmap )	/* If it's a bitmap font, we didn't create it (lives in sf) so we can't destroy it */
	    BDFFontFree(fd->bdf);
	free(fd);
    }
    _ggadget_destroy(g);
}

static void SFTextAreaSetTitle(GGadget *g,const unichar_t *tit) {
    SFTextArea *st = (SFTextArea *) g;
    unichar_t *old = st->oldtext;
    if ( u_strcmp(tit,st->text)==0 )	/* If it doesn't change anything, then don't trash undoes or selection */
return;
    st->oldtext = st->text;
    st->sel_oldstart = st->sel_start; st->sel_oldend = st->sel_end; st->sel_oldbase = st->sel_base;
    st->text = u_copy(tit);		/* tit might be oldtext, so must copy before freeing */
    free(old);
    st->sel_start = st->sel_end = st->sel_base = u_strlen(tit);
    SFTextAreaRefigureLines(st,0);
    SFTextArea_Show(st,st->sel_start);
    _ggadget_redraw(g);
}

static const unichar_t *_SFTextAreaGetTitle(GGadget *g) {
    SFTextArea *st = (SFTextArea *) g;
return( st->text );
}

static void SFTextAreaSetFont(GGadget *g,FontInstance *new) {
    SFTextArea *st = (SFTextArea *) g;
    st->font = new;
    SFTextAreaRefigureLines(st,0);
}

static FontInstance *SFTextAreaGetFont(GGadget *g) {
    SFTextArea *st = (SFTextArea *) g;
return( st->font );
}

void SFTextAreaShow(GGadget *g,int pos) {
    SFTextArea *st = (SFTextArea *) g;

    SFTextArea_Show(st,pos);
    _ggadget_redraw(g);
}

void SFTextAreaSelect(GGadget *g,int start, int end) {
    SFTextArea *st = (SFTextArea *) g;

    SFTextAreaGrabPrimarySelection(st);
    if ( end<0 ) {
	end = u_strlen(st->text);
	if ( start<0 ) start = end;
    }
    if ( start>end ) { int temp = start; start = end; end = temp; }
    if ( end>u_strlen(st->text)) end = u_strlen(st->text);
    if ( start>u_strlen(st->text)) start = end;
    else if ( start<0 ) start=0;
    st->sel_start = st->sel_base = start;
    st->sel_end = end;
    _ggadget_redraw(g);			/* Should be safe just to draw the textfield gadget, sbs won't have changed */
}

void SFTextAreaReplace(GGadget *g,const unichar_t *txt) {
    SFTextArea *st = (SFTextArea *) g;

    SFTextArea_Replace(st,txt);
    _ggadget_redraw(g);
}

static void sftextarea_redraw(GGadget *g) {
    SFTextArea *st = (SFTextArea *) g;
    if ( st->vsb!=NULL )
	_ggadget_redraw((GGadget *) (st->vsb));
    if ( st->hsb!=NULL )
	_ggadget_redraw((GGadget *) (st->hsb));
    _ggadget_redraw(g);
}

static void sftextarea_move(GGadget *g, int32 x, int32 y ) {
    SFTextArea *st = (SFTextArea *) g;
    if ( st->vsb!=NULL )
	_ggadget_move((GGadget *) (st->vsb),x+(st->vsb->g.r.x-g->r.x),y);
    if ( st->hsb!=NULL )
	_ggadget_move((GGadget *) (st->hsb),x,y+(st->hsb->g.r.y-g->r.y));
    _ggadget_move(g,x,y);
}

static void sftextarea_resize(GGadget *g, int32 width, int32 height ) {
    SFTextArea *st = (SFTextArea *) g;
    int gtwidth=width, gtheight=height, oldheight=0;
    int l;

    if ( st->hsb!=NULL ) {
	oldheight = st->hsb->g.r.y+st->hsb->g.r.height-g->r.y;
	gtheight = height - (oldheight-g->r.height);
    }
    if ( st->vsb!=NULL ) {
	int oldwidth = st->vsb->g.r.x+st->vsb->g.r.width-g->r.x;
	gtwidth = width - (oldwidth-g->r.width);
	_ggadget_move((GGadget *) (st->vsb),st->vsb->g.r.x+width-oldwidth,st->vsb->g.r.y);
	_ggadget_resize((GGadget *) (st->vsb),st->vsb->g.r.width,gtheight);
    }
    if ( st->hsb!=NULL ) {
	_ggadget_move((GGadget *) (st->hsb),st->hsb->g.r.y,st->hsb->g.r.y+height-oldheight);
	_ggadget_resize((GGadget *) (st->hsb),gtwidth,st->hsb->g.r.height);
    }
    _ggadget_resize(g,gtwidth, gtheight);
    SFTextAreaRefigureLines(st,0);
    if ( st->vsb!=NULL ) {
	GScrollBarSetBounds(&st->vsb->g,0,st->lineheights[st->lcnt-1].y,st->g.inner.height);
	if ( st->loff_top>=st->lcnt )
	    st->loff_top = st->lcnt-1;
	for ( l = st->loff_top; l>=0 && st->lineheights[st->lcnt-1].y-st->lineheights[l].y<=st->g.inner.height; --l );
	++l;
	if ( l<0 ) l = 0;
	if ( l!=st->loff_top ) {
	    st->loff_top = l;
	    GScrollBarSetPos(&st->vsb->g,st->lineheights[l].y);
	    _ggadget_redraw(&st->g);
	}
    }
}

static GRect *sftextarea_getsize(GGadget *g, GRect *r ) {
    SFTextArea *st = (SFTextArea *) g;
    _ggadget_getsize(g,r);
    if ( st->vsb!=NULL )
	r->width =  st->vsb->g.r.x+st->vsb->g.r.width-g->r.x;
    if ( st->hsb!=NULL )
	r->height =  st->hsb->g.r.y+st->hsb->g.r.height-g->r.y;
return( r );
}

static void sftextarea_setvisible(GGadget *g, int visible ) {
    SFTextArea *st = (SFTextArea *) g;
    if ( st->vsb!=NULL ) _ggadget_setvisible(&st->vsb->g,visible);
    if ( st->hsb!=NULL ) _ggadget_setvisible(&st->hsb->g,visible);
    _ggadget_setvisible(g,visible);
}

static void sftextarea_setenabled(GGadget *g, int enabled ) {
    SFTextArea *st = (SFTextArea *) g;
    if ( st->vsb!=NULL ) _ggadget_setenabled(&st->vsb->g,enabled);
    if ( st->hsb!=NULL ) _ggadget_setenabled(&st->hsb->g,enabled);
    _ggadget_setenabled(g,enabled);
}

static int sftextarea_vscroll(GGadget *g, GEvent *event) {
    enum sb sbt = event->u.control.u.sb.type;
    SFTextArea *st = (SFTextArea *) (g->data);
    int loff = st->loff_top;
    int page;

    g = (GGadget *) st;

    if ( sbt==et_sb_top )
	loff = 0;
    else if ( sbt==et_sb_bottom ) {
	loff = st->lcnt;
    } else if ( sbt==et_sb_up ) {
	if ( st->loff_top!=0 ) loff = st->loff_top-1; else loff = 0;
    } else if ( sbt==et_sb_down ) {
	++loff;
    } else if ( sbt==et_sb_uppage ) {
	for ( page=0; st->loff_top-page>=0 && st->lineheights[st->loff_top].y-st->lineheights[st->loff_top-page].y<=g->inner.height;
		++page );
	if ( page==0 ) page=1;
	else if ( page>2 ) page-=1;
	loff = st->loff_top - page;
    } else if ( sbt==et_sb_downpage ) {
	for ( page=0; st->loff_top+page<st->lcnt && st->lineheights[st->loff_top+page].y-st->lineheights[st->loff_top].y<=g->inner.height;
		++page );
	if ( page==0 ) page=1;
	else if ( page>2 ) page-=1;
	loff = st->loff_top + page;
    } else /* if ( sbt==et_sb_thumb || sbt==et_sb_thumbrelease ) */ {
	for ( loff = 0; loff<st->lcnt && st->lineheights[loff].y<event->u.control.u.sb.pos; ++loff );
    }
    for ( page=1; st->lcnt-page>=0 && st->lineheights[st->lcnt-1].y-st->lineheights[st->lcnt-page].y<=g->inner.height;
	    ++page );
    --page;
    if ( loff > st->lcnt-page )
	loff = st->lcnt - page;
    if ( loff<0 ) loff = 0;
    if ( loff!=st->loff_top ) {
	st->loff_top = loff;
	GScrollBarSetPos(&st->vsb->g,st->lineheights[loff].y);
	_ggadget_redraw(&st->g);
    }
return( true );
}

static int sftextarea_hscroll(GGadget *g, GEvent *event) {
    enum sb sbt = event->u.control.u.sb.type;
    SFTextArea *st = (SFTextArea *) (g->data);
    int xoff = st->xoff_left;

    g = (GGadget *) st;

    if ( sbt==et_sb_top )
	xoff = 0;
    else if ( sbt==et_sb_bottom ) {
	xoff = st->xmax - st->g.inner.width;
	if ( xoff<0 ) xoff = 0;
    } else if ( sbt==et_sb_up ) {
	if ( st->xoff_left>st->nw ) xoff = st->xoff_left-st->nw; else xoff = 0;
    } else if ( sbt==et_sb_down ) {
	if ( st->xoff_left + st->nw + st->g.inner.width >= st->xmax )
	    xoff = st->xmax - st->g.inner.width;
	else
	    xoff += st->nw;
    } else if ( sbt==et_sb_uppage ) {
	int page = (3*g->inner.width)/4;
	xoff = st->xoff_left - page;
	if ( xoff<0 ) xoff=0;
    } else if ( sbt==et_sb_downpage ) {
	int page = (3*g->inner.width)/4;
	xoff = st->xoff_left + page;
	if ( xoff + st->g.inner.width >= st->xmax )
	    xoff = st->xmax - st->g.inner.width;
    } else /* if ( sbt==et_sb_thumb || sbt==et_sb_thumbrelease ) */ {
	xoff = event->u.control.u.sb.pos;
    }
    if ( xoff + st->g.inner.width >= st->xmax )
	xoff = st->xmax - st->g.inner.width;
    if ( xoff<0 ) xoff = 0;
    if ( st->xoff_left!=xoff ) {
	st->xoff_left = xoff;
	GScrollBarSetPos(&st->hsb->g,xoff);
	_ggadget_redraw(&st->g);
    }
return( true );
}

struct gfuncs sftextarea_funcs = {
    0,
    sizeof(struct gfuncs),

    sftextarea_expose,
    sftextarea_mouse,
    sftextarea_key,
    _sftextarea_editcmd,
    sftextarea_focus,
    sftextarea_timer,
    sftextarea_sel,

    sftextarea_redraw,
    sftextarea_move,
    sftextarea_resize,
    sftextarea_setvisible,
    sftextarea_setenabled,
    sftextarea_getsize,
    _ggadget_getinnersize,

    sftextarea_destroy,

    SFTextAreaSetTitle,
    _SFTextAreaGetTitle,
    NULL,
    NULL,
    NULL,
    SFTextAreaSetFont,
    SFTextAreaGetFont
};

static void SFTextAreaInit() {
    static unichar_t courier[] = { 'c', 'o', 'u', 'r', 'i', 'e', 'r', ',', 'm','o','n','o','s','p','a','c','e',',','c','l','e','a','r','l','y','u',',', 'u','n','i','f','o','n','t', '\0' };
    FontRequest rq;

    GGadgetInit();
    GDrawDecomposeFont(_ggadget_default_font,&rq);
    rq.family_name = courier;
    sftextarea_font = GDrawInstanciateFont(screen_display,&rq);
    _GGadgetCopyDefaultBox(&sftextarea_box);
    sftextarea_box.padding = 3;
    sftextarea_box.flags = box_active_border_inner;
    sftextarea_font = _GGadgetInitDefaultBox("SFTextArea.",&sftextarea_box,sftextarea_font);
    sftextarea_inited = true;
}

static void SFTextAreaAddVSb(SFTextArea *st) {
    GGadgetData gd;

    memset(&gd,'\0',sizeof(gd));
    gd.pos.y = st->g.r.y; gd.pos.height = st->g.r.height;
    gd.pos.width = GDrawPointsToPixels(st->g.base,_GScrollBar_Width);
    gd.pos.x = st->g.r.x+st->g.r.width - gd.pos.width;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    gd.handle_controlevent = sftextarea_vscroll;
    st->vsb = (GScrollBar *) GScrollBarCreate(st->g.base,&gd,st);
    st->vsb->g.contained = true;

    gd.pos.width += GDrawPointsToPixels(st->g.base,1);
    st->g.r.width -= gd.pos.width;
    st->g.inner.width -= gd.pos.width;
}

static void SFTextAreaAddHSb(SFTextArea *st) {
    GGadgetData gd;

    memset(&gd,'\0',sizeof(gd));
    gd.pos.x = st->g.r.x; gd.pos.width = st->g.r.width;
    gd.pos.height = GDrawPointsToPixels(st->g.base,_GScrollBar_Width);
    gd.pos.y = st->g.r.y+st->g.r.height - gd.pos.height;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    gd.handle_controlevent = sftextarea_hscroll;
    st->hsb = (GScrollBar *) GScrollBarCreate(st->g.base,&gd,st);
    st->hsb->g.contained = true;

    gd.pos.height += GDrawPointsToPixels(st->g.base,1);
    st->g.r.height -= gd.pos.height;
    st->g.inner.height -= gd.pos.height;
    if ( st->vsb!=NULL ) {
	st->vsb->g.r.height -= gd.pos.height;
	st->vsb->g.inner.height -= gd.pos.height;
    }
}

static void SFTextAreaFit(SFTextArea *st) {
    GTextBounds bounds;
    int as=0, ds, ld, fh=0, width=0, temp;
    GRect needed;
    int extra=0;

    needed.x = needed.y = 0;
    needed.width = needed.height = 1;

    {
	FontInstance *old = GDrawSetFont(st->g.base,st->font);
	width = GDrawGetTextBounds(st->g.base,st->text, -1, NULL, &bounds);
	GDrawFontMetrics(st->font,&as, &ds, &ld);
	if ( as<bounds.as ) as = bounds.as;
	if ( ds<bounds.ds ) ds = bounds.ds;
	st->fh = fh = as+ds;
	st->as = as;
	st->nw = 6;
	GDrawSetFont(st->g.base,old);
    }

    temp = GGadgetScale(GDrawPointsToPixels(st->g.base,80))+extra;

    if ( st->g.r.width==0 || st->g.r.height==0 ) {
	int bp = GBoxBorderWidth(st->g.base,st->g.box);
	needed.x = needed.y = 0;
	needed.width = temp;
	needed.height = st->multi_line? 4*fh:fh;
	_ggadgetFigureSize(st->g.base,st->g.box,&needed,false);
	if ( st->g.r.width==0 ) {
	    st->g.r.width = needed.width;
	    st->g.inner.width = temp-extra;
	    st->g.inner.x = st->g.r.x + (needed.width-temp)/2;
	} else {
	    st->g.inner.x = st->g.r.x + bp;
	    st->g.inner.width = st->g.r.width - 2*bp;
	}
	if ( st->g.r.height==0 ) {
	    st->g.r.height = needed.height;
	    st->g.inner.height = st->multi_line? 4*fh:fh;
	    st->g.inner.y = st->g.r.y + (needed.height-st->g.inner.height)/2;
	} else {
	    st->g.inner.y = st->g.r.y + bp;
	    st->g.inner.height = st->g.r.height - 2*bp;
	}
	if ( st->multi_line ) {
	    int sbadd = GDrawPointsToPixels(st->g.base,_GScrollBar_Width) +
		    GDrawPointsToPixels(st->g.base,1);
	    {
		st->g.r.width += sbadd;
		st->g.inner.width += sbadd;
	    }
	    if ( !st->wrap ) {
		st->g.r.height += sbadd;
		st->g.inner.height += sbadd;
	    }
	}
    } else {
	int bp = GBoxBorderWidth(st->g.base,st->g.box);
	st->g.inner = st->g.r;
	st->g.inner.x += bp; st->g.inner.y += bp;
	st->g.inner.width -= 2*bp-extra; st->g.inner.height -= 2*bp;
    }
    if ( st->multi_line ) {
	SFTextAreaAddVSb(st);
	if ( !st->wrap )
	    SFTextAreaAddHSb(st);
    }
}

static SFTextArea *_SFTextAreaCreate(SFTextArea *st, struct gwindow *base, GGadgetData *gd,void *data, GBox *def) {

    if ( !sftextarea_inited )
	SFTextAreaInit();
    st->g.funcs = &sftextarea_funcs;
    _GGadget_Create(&st->g,base,gd,data,def);

    st->g.takes_input = true; st->g.takes_keyboard = true; st->g.focusable = true;
    if ( gd->label!=NULL ) {
	if ( gd->label->text_in_resource )	/* This one use of GStringGetResource is ligit */
	    st->text = u_copy((unichar_t *) GStringGetResource((intpt) gd->label->text,&st->g.mnemonic));
	else if ( gd->label->text_is_1byte )
	    st->text = utf82u_copy((char *) gd->label->text);
	else
	    st->text = u_copy(gd->label->text);
	st->sel_start = st->sel_end = st->sel_base = u_strlen(st->text);
    }
    if ( st->text==NULL )
	st->text = gcalloc(1,sizeof(unichar_t));
    st->font = sftextarea_font;
    if ( gd->label!=NULL && gd->label->font!=NULL )
	st->font = gd->label->font;
    SFTextAreaFit(st);
    _GGadget_FinalPosition(&st->g,base,gd);
    SFTextAreaRefigureLines(st,0);

    if ( gd->flags & gg_group_end )
	_GGadgetCloseGroup(&st->g);
    GWidgetIndicateFocusGadget(&st->g);
    if ( gd->flags & gg_text_xim )
	st->gic = GDrawCreateInputContext(base,gic_overspot|gic_orlesser);
return( st );
}

GGadget *SFTextAreaCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    SFTextArea *st = gcalloc(1,sizeof(SFTextArea));
    st->multi_line = true;
    st->accepts_returns = true;
    st->wrap = true;
    _SFTextAreaCreate(st,base,gd,data,&sftextarea_box);

return( &st->g );
}

static struct sfmaps *SFMapOfSF(SFTextArea *st,SplineFont *sf) {
    struct sfmaps *sfmaps;

    for ( sfmaps=st->sfmaps; sfmaps!=NULL; sfmaps=sfmaps->next )
	if ( sfmaps->sf==sf )
return( sfmaps );

    sfmaps = chunkalloc(sizeof(struct sfmaps));
    sfmaps->sf = sf;
    sfmaps->next = st->sfmaps;
    st->sfmaps = sfmaps;
    sfmaps->map = EncMapFromEncoding(sf,FindOrMakeEncoding("Unicode"));
return( sfmaps );
}

static FontData *FindFontData(SFTextArea *st, SplineFont *sf,
	enum sftf_fonttype fonttype, int size, int antialias) {
    FontData *test, *ret;
    BDFFont *bdf, *ok;
    void *ftc;

    for ( test=st->generated; test!=NULL; test=test->next )
	if ( test->sf == sf && test->fonttype == fonttype &&
		test->pixelsize==size && test->antialias==antialias )
return( test );

    ret = gcalloc(1,sizeof(FontData));
    ret->sf = sf;
    ret->fonttype = fonttype;
    ret->pixelsize = size;
    ret->antialias = antialias;
    if ( fonttype==sftf_bitmap ) {
	ok = NULL;
	for ( bdf= sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( bdf->pixelsize==size ) {
		if (( !antialias && bdf->clut==NULL ) ||
			(antialias && bdf->clut!=NULL && bdf->clut->clut_len==256) ) {
		    ok = bdf;
	break;
		}
		if ( antialias && bdf->clut!=NULL &&
			(ok==NULL || bdf->clut->clut_len>ok->clut->clut_len))
		    ok = bdf;
	    }
	}
	if ( ok==NULL ) {
	    free(ret);
return( NULL );
	}
	ret->bdf = ok;
    } else if ( fonttype==sftf_pfaedit ) {
	ret->bdf = SplineFontPieceMeal(sf,size,antialias,NULL);
    } else if ( !hasFreeType() )
return( NULL );
    else {
	for ( test=st->generated; test!=NULL; test=test->next )
	    if ( test->sf == sf && test->fonttype == fonttype )
	break;
	ret->depends_on = test;
	ftc = NULL;
	if ( test )
	    ftc = test->bdf->freetype_context;
	if ( ftc==NULL ) {
	    int flags = 0;
	    int ff = fonttype==sftf_pfb ? ff_pfb :
		     fonttype==sftf_ttf ? ff_ttf :
		     ff_otf;
	    ftc = _FreeTypeFontContext(sf,NULL,NULL,ff,flags,NULL);
	}
	if ( ftc==NULL ) {
	    free(ret);
return( NULL );
	}
	ret->bdf = SplineFontPieceMeal(sf,size,antialias,ftc);
    }
    ret->sfmap = SFMapOfSF(st,sf);
    if ( ret->bdf->clut ) {
	ret->gi.u.image = &ret->base;
	ret->base.image_type = it_index;
	ret->base.clut = ret->bdf->clut;
	ret->base.trans = 0;
    } else {
	memset(&ret->clut,'\0',sizeof(ret->clut));
	ret->gi.u.image = &ret->base;
	ret->base.image_type = it_mono;
	ret->base.clut = &ret->clut;
	ret->clut.clut_len = 2;
	ret->clut.clut[0] = GDrawGetDefaultBackground(NULL);
	ret->base.trans = 0;
    }
    ret->next = st->generated;
    st->generated = ret;
return( ret );
}

int SFTFSetFont(GGadget *g, int start, int end, SplineFont *sf,
	enum sftf_fonttype fonttype, int size, int antialias) {
    /* Sets the font for the region between start and end. If start==-1 it */
    /*  means use the current selection (and ignore end). If end==-1 it means */
    /*  strlen(g->text) */
    /* I'm not going to mess with making this undoable. Nor am I going to clear*/
    /*  out the undoes. So if someone does an undo after this it will undo */
    /*  two things. Tough. */
    SFTextArea *st = (SFTextArea *) g;
    FontData *cur;
    int len = u_strlen(st->text);
    struct fontlist *new, *fl, *prev, *next;

    if ( st->generated==NULL ) {
	start = 0;
	end = len;
    } else if ( start==-1 ) {
	start = st->sel_start;
	end = st->sel_end;
    } else if ( end==-1 )
	end = len;
    if ( end>len ) end = len;
    if ( start<0 ) start = 0;
    if ( start>end ) start = end;

    cur = FindFontData(st, sf, fonttype, size, antialias);
    if ( cur==NULL )
return( false );

    new = chunkalloc(sizeof(struct fontlist));
    new->start = start;
    new->end = end;
    new->fd = cur;

    prev = next = NULL;
    for ( fl=st->fontlist; fl!=NULL; fl=next ) {
	next = fl->next;
	if ( new->end>=fl->start && new->end<fl->end ) {
	    if ( new->end==fl->start )
    break;
	    next = chunkalloc(sizeof(struct fontlist));
	    *next = *fl;
	    next->start = new->end;
	    fl->end = new->end;
	}
	if ( fl->start>=new->start && fl->end<=new->end ) {
	    chunkfree(fl,sizeof(struct fontlist));
	    if ( prev==NULL )
		st->fontlist = next;
	    else
		prev->next = next;
	} else if ( new->start>fl->start && new->start<fl->end ) {
	    fl->end = new->start;
	    prev = fl;
	} else if ( fl->end<=new->start ) {
	    prev = fl;
	} else if ( fl->start>=new->end )
    break;
    }
    new->next = fl;
    if ( prev==NULL )
	st->fontlist = new;
    else
	prev->next = new;
    fontlistmergecheck(st);
    SFTextAreaRefigureLines(st, start);
    GDrawRequestExpose(g->base,&g->inner,false);
    if ( st->changefontcallback != NULL )
	STChangeCheck(st);
return( true );
}

void SFTFRegisterCallback(GGadget *g, void *cbcontext,
	void (*changefontcallback)(void *,SplineFont *,enum sftf_fonttype,int size,int aa)) {
    SFTextArea *st = (SFTextArea *) g;

    st->cbcontext = cbcontext;
    st->changefontcallback = changefontcallback;
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
