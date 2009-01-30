/* Copyright (C) 2003-2009 by George Williams */
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
#include "psfont.h"
#include <ustring.h>
#include <gkeysym.h>
#include <utype.h>
#include <math.h>
#include "psfont.h"

/* This operations are designed to work on a single font. NOT a CID collection*/
/*  A CID collection must be treated one sub-font at a time */

typedef struct histdata {
    int low, high;
    struct hentry {
	int cnt, sum;
	int char_cnt, max;
	SplineChar **chars;
    } *hist;			/* array of high-low+1 elements */
    int tot, max;
} HistData;

static void HistDataFree(HistData *h) {
    int i;

    for ( i=h->low; i<=h->high; ++i )
	free(h->hist[i-h->low].chars);
    free(h->hist);
    free(h);
}

static HistData *HistFindBlues(SplineFont *sf,int layer, uint8 *selected, EncMap *map) {
    int i, gid, low,high, top,bottom;
    SplineChar *sc;
    DBounds b;
    HistData *hist;
    struct hentry *h;

    hist = gcalloc(1,sizeof(HistData));
    hist->hist = gcalloc(sf->ascent+sf->descent+1,sizeof(struct hentry));
    hist->low = sf->ascent; hist->high = -sf->descent;
    low = -sf->descent; high = sf->ascent;

    for ( i=0; i<(selected==NULL?sf->glyphcnt:map->enccount); ++i ) {
	gid = selected==NULL ? i : map->map[i];
	if ( gid!=-1 && (sc = sf->glyphs[gid])!=NULL &&
		sc->layers[ly_fore].splines!=NULL &&
		sc->layers[ly_fore].refs==NULL &&
		(selected==NULL || selected[i])) {
	    SplineCharLayerFindBounds(sc,layer,&b);
	    bottom = rint(b.miny);
	    top = rint(b.maxy);
	    if ( top==bottom )
	continue;
	    if ( top>hist->high ) {
		hist->high = top;
		if ( top>high ) {
		    hist->hist = grealloc(hist->hist,(top+10-low)*sizeof(struct hentry));
		    memset(hist->hist + high-low+1,0,(top+10-high-1)*sizeof(struct hentry));
		    high = top+10 -1;
		}
	    }
	    ++ hist->hist[top-low].cnt;
	    if ( hist->hist[top-low].char_cnt >= hist->hist[top-low].max ) {
		if ( hist->hist[top-low].max==0 )
		    hist->hist[top-low].chars = galloc(10*sizeof(SplineChar *));
		else
		    hist->hist[top-low].chars = grealloc(hist->hist[top-low].chars,(hist->hist[top-low].max+10)*sizeof(SplineChar *));
		hist->hist[top-low].max += 10;
	    }
	    hist->hist[top-low].chars[hist->hist[top-low].char_cnt++] = sc;

	    if ( bottom<hist->low ) {
		hist->low = bottom;
		if ( bottom<low ) {
		    h = gcalloc((high-bottom+10),sizeof( struct hentry ));
		    memcpy(h+low-(bottom-10+1),hist->hist,(high+1-low)*sizeof(struct hentry));
		    low = bottom-10+1;
		    free( hist->hist );
		    hist->hist = h;
		}
	    }
	    ++ hist->hist[bottom-low].cnt;
	    if ( hist->hist[bottom-low].char_cnt >= hist->hist[bottom-low].max ) {
		if ( hist->hist[bottom-low].max==0 )
		    hist->hist[bottom-low].chars = galloc(10*sizeof(SplineChar *));
		else
		    hist->hist[bottom-low].chars = grealloc(hist->hist[bottom-low].chars,(hist->hist[bottom-low].max+10)*sizeof(SplineChar *));
		hist->hist[bottom-low].max += 10;
	    }
	    hist->hist[bottom-low].chars[hist->hist[bottom-low].char_cnt++] = sc;
	}
	hist->tot += 2;
    }
    if ( hist->low>hist->high ) {		/* Found nothing */
	hist->low = hist->high = 0;
    }
    if ( low!=hist->low || high!=hist->high ) {
	h = galloc((hist->high-hist->low+1)*sizeof(struct hentry));
	memcpy(h,hist->hist + hist->low-low,(hist->high-hist->low+1)*sizeof(struct hentry));
	free(hist->hist);
	hist->hist = h;
    }
return( hist );
}

static HistData *HistFindStemWidths(SplineFont *sf,int layer, uint8 *selected,EncMap *map,int hor) {
    int i, gid, low,high, val;
    SplineChar *sc;
    HistData *hist;
    struct hentry *h;
    StemInfo *stem;

    hist = gcalloc(1,sizeof(HistData));
    hist->hist = gcalloc(sf->ascent+sf->descent+1,sizeof(struct hentry));
    hist->low = sf->ascent+sf->descent;
    low = 0; high = sf->ascent+sf->descent;

    for ( i=0; i<(selected==NULL?sf->glyphcnt:map->enccount); ++i ) {
	gid = selected==NULL ? i : map->map[i];
	if ( gid!=-1 && (sc = sf->glyphs[gid])!=NULL &&
		sc->layers[ly_fore].splines!=NULL &&
		sc->layers[ly_fore].refs==NULL &&
		(selected==NULL || selected[i])) {
	    if ( autohint_before_generate && sc->changedsincelasthinted && !sc->manualhints )
		SplineCharAutoHint(sc,layer,NULL);
	    for ( stem = hor ? sc->hstem : sc->vstem ; stem!=NULL; stem = stem->next ) {
		if ( stem->ghost )
	    continue;
		val = rint(stem->width);
		if ( val<=0 )
		    val = -val;
		if ( val>hist->high ) {
		    hist->high = val;
		    if ( val>high ) {
			hist->hist = grealloc(hist->hist,(val+10-low)*sizeof(struct hentry));
			memset(hist->hist + high-low+1,0,(val+10-high-1)*sizeof(struct hentry));
			high = val+10 -1;
		    }
		}
		if ( val<hist->low )
		    hist->low = val;
		++ hist->hist[val-low].cnt;
		if ( hist->hist[val-low].char_cnt==0 ||
			hist->hist[val-low].chars[hist->hist[val-low].char_cnt-1]!=sc ) {
		    if ( hist->hist[val-low].char_cnt >= hist->hist[val-low].max ) {
			if ( hist->hist[val-low].max==0 )
			    hist->hist[val-low].chars = galloc(10*sizeof(SplineChar *));
			else
			    hist->hist[val-low].chars = grealloc(hist->hist[val-low].chars,(hist->hist[val-low].max+10)*sizeof(SplineChar *));
			hist->hist[val-low].max += 10;
		    }
		    hist->hist[val-low].chars[hist->hist[val-low].char_cnt++] = sc;
		}
		++ hist->tot;
	    }
	}
    }
    if ( hist->low>hist->high ) {		/* Found nothing */
	hist->low = hist->high = 0;
    }
    if ( low!=hist->low || high!=hist->high ) {
	h = galloc((hist->high-hist->low+1)*sizeof(struct hentry));
	memcpy(h,hist->hist + hist->low-low,(hist->high-hist->low+1)*sizeof(struct hentry));
	free(hist->hist);
	hist->hist = h;
    }
return( hist );
}

static HistData *HistFindHStemWidths(SplineFont *sf,int layer, uint8 *selected,EncMap *map) {
return( HistFindStemWidths(sf,layer,selected,map,true) );
}

static HistData *HistFindVStemWidths(SplineFont *sf,int layer, uint8 *selected,EncMap *map) {
return( HistFindStemWidths(sf,layer,selected,map,false) );
}

static void HistFindMax(HistData *h, int sum_around) {
    int i, j, m=1;
    int c;

    if ( sum_around<0 ) sum_around = 0;
    for ( i = h->low; i<=h->high; ++i ) {
	c = 0;
	for ( j=i-sum_around; j<=i+sum_around; ++j )
	    if ( j>=h->low && j<=h->high )
		c += h->hist[j-h->low].cnt;
	h->hist[i-h->low].sum = c;
	if ( c>m )
	    m = c;
    }
    h->max = m;
}

#define CID_ScrollBar		1000
#define CID_MainVal		1001
#define CID_SecondaryVal	1002

#define CID_SumAround		1003
#define CID_BarWidth		1004

#define CID_MainValL		2001
#define CID_SecondaryValL	2002
#define CID_SumAroundL		2003
#define CID_BarWidthL		2004
#define CID_Group		2005
#define CID_BlueMsg		2006

#define CID_OK			3001
#define CID_Cancel		3002

struct hist_dlg {
    enum hist_type which;
    SplineFont *sf;
    int layer;
    struct psdict *private;
    uint8 *selected;
    HistData *h;

    int pending_blue;
    int is_pending;

    int sum_around, barwidth;
    int hoff;

    int x,y;
    int hwidth, hheight;
    int yoff;

    GWindow gw;
    GFont *font;
    int fh, as;
    int done;
};

static void HistPopup(struct hist_dlg *hist,GEvent *e) {
    int x = e->u.mouse.x - (hist->x+1);
    struct hentry *h;
    static char buffer[300];
    char *end = buffer + sizeof(buffer)/sizeof(buffer[0]), *pt, *line;
    int i;

    if ( e->u.mouse.y<hist->y || e->u.mouse.y>hist->y+hist->hheight )
return;
    if ( x<0 || x>hist->hwidth-2 )
return;

    x /= hist->barwidth;
    if ( x + hist->hoff > hist->h->high || x + hist->hoff - hist->h->low<0 )
return;

    h = &hist->h->hist[x + hist->hoff - hist->h->low];
    if ( hist->sum_around==0 ) {
	if ( hist->which == hist_blues )
	    snprintf(buffer,end-buffer,
		    _("Position: %d\nCount: %d\n"),
		    x + hist->hoff,
		    h->sum);
	else
	    snprintf(buffer,end-buffer,
		    _("Width: %d\nCount: %d\nPercentage of Max: %d%%\n"),
		    x + hist->hoff,
		    h->sum, (int) rint(h->sum*100.0/hist->h->max));
    } else {
	if ( hist->which == hist_blues )
	    snprintf(buffer,end-buffer,
		    _("Position: %d-%d (%d)\nCount: %d (%d)\n"),
		    x+hist->hoff-hist->sum_around, x+hist->hoff+hist->sum_around, x + hist->hoff,
		    h->sum, h->cnt);
	else
	    snprintf(buffer,end-buffer,
		    _("Width: %d-%d (%d)\nCount: %d (%d)\nPercentage of Max: %d%%\n"),
		    x+hist->hoff-hist->sum_around, x+hist->hoff+hist->sum_around, x + hist->hoff,
		    h->sum, h->cnt, (int) rint(h->sum*100.0/hist->h->max));
    }
    pt = buffer+strlen(buffer);
    line = pt;
    for ( i = 0; i<h->char_cnt; ++i ) {
	if ( pt+strlen(h->chars[i]->name)+4>end ) {
	    strcpy(pt,"...");
    break;
	}
	strcpy(pt,h->chars[i]->name);
	pt += strlen(pt);
	if ( pt-line>70 ) {
	    *pt++ = '\n';
	    line = pt;
	} else
	    *pt++ = ' ';
	*pt = '\0';
    }
    GGadgetPreparePopup8(hist->gw,buffer);
}

static unichar_t *ArrayOrder(const unichar_t *old,int args,int val1,int val2) {
    unichar_t *end;
    double array[40];
    int i,j,k;
    static unichar_t format[] = { '%', 'g', '\0' };
    unichar_t *new, *pt;
    unichar_t ubuf[40];

    if ( *old=='[' ) ++old;

    for ( i=0; i<40 && *old!=']' && *old!='\0'; ++i ) {
	array[i] = u_strtod(old,&end);
	if ( old==end )
    break;
	old = end;
	while ( *old==' ' ) ++old;
    }
    array[i++] = val1;
    if ( args==2 )
	array[i++] = val2;
    for ( j=0; j<i; ++j ) for ( k=j+1; k<i; ++k ) if ( array[j]>array[k] ) {
	double temp = array[j];
	array[j] = array[k];
	array[k] = temp;
    }

    u_sprintf(ubuf,format,val1);
    new = galloc(2*(u_strlen(ubuf)+u_strlen(old)+10)*sizeof(unichar_t));

    pt = new;
    *pt++ = '[';
    for ( k=0; k<i; ++k ) {
	u_sprintf(pt,format,array[k]);
	pt += u_strlen(pt);
	if ( k==i-1 )
	    *pt++ = ']';
	else
	    *pt++ = ' ';
    }
    *pt = '\0';
return( new );
}

static void HistPress(struct hist_dlg *hist,GEvent *e) {
    int x = e->u.mouse.x - (hist->x+1);
    static unichar_t fullformat[] = { '[', '%', 'd', ']', '\0' };
    unichar_t ubuf[20];

    if ( e->u.mouse.y<hist->y || e->u.mouse.y>hist->y+hist->hheight )
return;
    if ( x<0 || x>hist->hwidth-2 )
return;

    x /= hist->barwidth;
    x += hist->hoff;
    if ( x > hist->h->high || x<hist->h->low )
return;

    if ( hist->which==hist_blues ) {
	if ( hist->is_pending ) {
	    if ( x<hist->pending_blue )
		ff_post_error(_("Bad Value"),_("The smaller number must be selected first in a pair of bluevalues"));
	    else if ( x<0 ) {	/* OtherBlues */
		const unichar_t *old = _GGadgetGetTitle(GWidgetGetControl(hist->gw,CID_SecondaryVal));
		unichar_t *new = ArrayOrder(old,2,hist->pending_blue,x);
		GGadgetSetTitle(GWidgetGetControl(hist->gw,CID_SecondaryVal),new);
		free(new);
	    } else {
		const unichar_t *old = _GGadgetGetTitle(GWidgetGetControl(hist->gw,CID_MainVal));
		unichar_t *new = ArrayOrder(old,2,hist->pending_blue,x);
		GGadgetSetTitle(GWidgetGetControl(hist->gw,CID_MainVal),new);
		free(new);
	    }
	    GDrawSetCursor(hist->gw,ct_pointer);
	    hist->is_pending = false;
	} else {
	    hist->is_pending = true;
	    hist->pending_blue = x;
	    GDrawSetCursor(hist->gw,ct_eyedropper);
	}
	GGadgetSetVisible(GWidgetGetControl(hist->gw,CID_MainVal),!hist->is_pending);
	GGadgetSetVisible(GWidgetGetControl(hist->gw,CID_MainValL),!hist->is_pending);
	GGadgetSetVisible(GWidgetGetControl(hist->gw,CID_BlueMsg),hist->is_pending);
    } else {
	if ( !( e->u.mouse.state&ksm_shift )) {
	    u_sprintf(ubuf,fullformat,x);
	    GGadgetSetTitle(GWidgetGetControl(hist->gw,CID_MainVal),ubuf);
	    GGadgetSetTitle(GWidgetGetControl(hist->gw,CID_SecondaryVal),ubuf);
	} else {
	    const unichar_t *old = _GGadgetGetTitle(GWidgetGetControl(hist->gw,CID_SecondaryVal));
	    unichar_t *new = ArrayOrder(old,1,x,0);
	    GGadgetSetTitle(GWidgetGetControl(hist->gw,CID_SecondaryVal),new);
	    free(new);
	}
    }
}

static void HistExpose(GWindow pixmap, struct hist_dlg *hist) {
    GRect r,old;
    int height = hist->hheight-hist->fh;
    double yscale = (4*height/5.0)/(hist->h->max-0);
    int i;
    char buf[20];

    GDrawSetLineWidth(pixmap,0);
    r.x = hist->x; r.y = hist->y;
    r.width = hist->hwidth; r.height = height;
    GDrawDrawRect(pixmap,&r,0x000000);

    ++r.x; r.width--;
    ++r.y; r.height--;
    GDrawPushClip(pixmap,&r,&old);

    for ( i=hist->hoff; (i-hist->hoff)*hist->barwidth<hist->hwidth-2 && i<=hist->h->high; ++i ) {
	r.x = (i-hist->hoff)*hist->barwidth+hist->x+1; r.width = hist->barwidth;
	r.height = rint(hist->h->hist[i-hist->h->low].sum * yscale);
	if ( r.height>=0 ) {
	    r.y = hist->y+height - r.height;
	    GDrawFillRect(pixmap,&r,0x2020ff);
	}
    }

    GDrawPopClip(pixmap,&old);

    GDrawSetFont(pixmap,hist->font);
    r.x = 10; r.y = hist->y+height+1; r.width = hist->hwidth+2*(hist->x-10); r.height = hist->fh;
    GDrawFillRect(pixmap,&r,GDrawGetDefaultBackground(NULL));
    sprintf(buf,"%d",hist->hoff);
    GDrawDrawBiText8(pixmap,hist->x-GDrawGetText8Width(pixmap,buf,-1,NULL)/2,hist->y+height+hist->as,
	    buf,-1,NULL,0x000000);
    sprintf(buf,"%d",hist->hoff+hist->hwidth/hist->barwidth);
    GDrawDrawBiText8(pixmap,hist->x+hist->hwidth-GDrawGetText8Width(pixmap,buf,-1,NULL)/2,hist->y+height+hist->as,
	    buf,-1,NULL,0x000000);

    sprintf(buf,"%d",hist->h->max);
    GDrawDrawBiText8(pixmap,hist->x-GDrawGetText8Width(pixmap,buf,-1,NULL)-2,hist->y+height-rint(hist->h->max*yscale),
	    buf,-1,NULL,0x000000);
    GDrawDrawBiText8(pixmap,hist->x+hist->hwidth+2,hist->y+height-rint(hist->h->max*yscale),
	    buf,-1,NULL,0x000000);
}

static void HistScroll(struct hist_dlg *hist,struct sbevent *sb) {
    int newpos = hist->hoff;
    int cols = (hist->hwidth-2)/hist->barwidth;
    GRect r;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= cols;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += cols;
      break;
      case et_sb_bottom:
        newpos = (hist->h->high+1-hist->h->low)-cols;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>(hist->h->high+1-hist->h->low)-cols + hist->h->low )
        newpos = (hist->h->high+1-hist->h->low)-cols + hist->h->low;
    if ( newpos<hist->h->low ) newpos = hist->h->low;
    if ( newpos!=hist->hoff ) {
	int diff = newpos-hist->hoff;
	hist->hoff = newpos;
	GScrollBarSetPos(GWidgetGetControl(hist->gw,CID_ScrollBar),hist->hoff);
	r.x = hist->x+1; r.y = hist->y+1;
	r.width = hist->hwidth-1; r.height = hist->hheight-1;
	GDrawScroll(hist->gw,&r,-diff*hist->barwidth,0);
	r.x = 10; r.y = hist->y+hist->hheight-hist->fh+1;
	r.width = hist->hwidth+2*(hist->x-10); r.height = hist->fh;
	GDrawRequestExpose(hist->gw,&r,false);
    }
}

static void HistRefigureSB(struct hist_dlg *hist) {
    GGadget *g = GWidgetGetControl(hist->gw,CID_ScrollBar);
    int width = hist->hwidth, hoff;

    GScrollBarSetBounds(g,hist->h->low,hist->h->high+1,(width-2)/hist->barwidth);
    if ( hist->hoff+width/hist->barwidth >hist->h->high ) {
	hoff = hist->h->high-width/hist->barwidth;
	if ( hoff<0 ) hoff = 0;
	if ( hoff!=hist->hoff ) {
	    hist->hoff = hoff;
	    GScrollBarSetPos(g,hoff);
	}
    }
}

static void HistResize(struct hist_dlg *hist) {
    GRect size;
    int width, height, changed=false;
    GGadget *g;

    GDrawGetSize(hist->gw,&size);
    width = size.width-2*hist->x;
    height = size.height - hist->yoff - 20;
    if ( width!=hist->hwidth || height!=hist->hheight ) {
	changed = true;
	g = GWidgetGetControl(hist->gw,CID_Group);
	GGadgetGetSize(g,&size);
	GGadgetResize(g,size.width+width-hist->hwidth,size.height+height-hist->hheight);
    }
    if ( width!=hist->hwidth ) {
	g = GWidgetGetControl(hist->gw,CID_Cancel);
	GGadgetGetSize(g,&size);
	GGadgetMove(g,size.x+width-hist->hwidth,size.y);
	g = GWidgetGetControl(hist->gw,CID_ScrollBar);
	GGadgetGetSize(g,&size);
	GGadgetResize(g,width+1,size.height);
	hist->hwidth = width;
	HistRefigureSB(hist);
    }
    if ( height!=hist->hheight ) {
	int off = height-hist->hheight;
	hist->hheight = height;
	g = GWidgetGetControl(hist->gw,CID_ScrollBar);
	GGadgetGetSize(g,&size);
	GGadgetMove(g,size.x,size.y+off);
	g = GWidgetGetControl(hist->gw,CID_SumAroundL);
	GGadgetGetSize(g,&size);
	GGadgetMove(g,size.x,size.y+off);
	g = GWidgetGetControl(hist->gw,CID_SumAround);
	GGadgetGetSize(g,&size);
	GGadgetMove(g,size.x,size.y+off);
	g = GWidgetGetControl(hist->gw,CID_BarWidthL);
	GGadgetGetSize(g,&size);
	GGadgetMove(g,size.x,size.y+off);
	g = GWidgetGetControl(hist->gw,CID_BarWidth);
	GGadgetGetSize(g,&size);
	GGadgetMove(g,size.x,size.y+off);
	g = GWidgetGetControl(hist->gw,CID_MainValL);
	GGadgetGetSize(g,&size);
	GGadgetMove(g,size.x,size.y+off);
	g = GWidgetGetControl(hist->gw,CID_MainVal);
	GGadgetGetSize(g,&size);
	GGadgetMove(g,size.x,size.y+off);
	g = GWidgetGetControl(hist->gw,CID_SecondaryValL);
	GGadgetGetSize(g,&size);
	GGadgetMove(g,size.x,size.y+off);
	g = GWidgetGetControl(hist->gw,CID_SecondaryVal);
	GGadgetGetSize(g,&size);
	GGadgetMove(g,size.x,size.y+off);
	g = GWidgetGetControl(hist->gw,CID_BlueMsg);
	GGadgetGetSize(g,&size);
	GGadgetMove(g,size.x,size.y+off);
	g = GWidgetGetControl(hist->gw,CID_OK);
	GGadgetGetSize(g,&size);
	GGadgetMove(g,size.x,size.y+off);
	g = GWidgetGetControl(hist->gw,CID_Cancel);
	GGadgetGetSize(g,&size);
	GGadgetMove(g,size.x,size.y+off);
    }
    if ( changed )
	GDrawRequestExpose(hist->gw,NULL,false);
}
	
static void HistSet(struct hist_dlg *hist) {
    char *primary, *secondary;
    char *temp;
    struct psdict *p = hist->private ? hist->private : hist->sf->private;
    const unichar_t *ret1, *ret2;

    switch ( hist->which ) {
      case hist_hstem:
	primary = "StdHW"; secondary = "StemSnapH";
      break;
      case hist_vstem:
	primary = "StdVW"; secondary = "StemSnapV";
      break;
      case hist_blues:
	primary = "BlueValues"; secondary = "OtherBlues";
      break;
    }
    ret1 = GGadgetGetTitle(GWidgetGetControl(hist->gw,CID_MainVal));
    ret2 = GGadgetGetTitle(GWidgetGetControl(hist->gw,CID_SecondaryVal));
    hist->done = true;
    if ( (*ret1=='\0' || uc_strcmp(ret1,"[]")==0 ) &&
	    (*ret2=='\0' || uc_strcmp(ret2,"[]")==0 ) && p==NULL )
return;
    if ( p==NULL ) {
	hist->sf->private = p = gcalloc(1,sizeof(struct psdict));
	p->cnt = 10;
	p->keys = gcalloc(10,sizeof(char *));
	p->values = gcalloc(10,sizeof(char *));
    }
    PSDictChangeEntry(p,primary,temp=cu_copy(ret1)); free(temp);
    PSDictChangeEntry(p,secondary,temp=cu_copy(ret2)); free(temp);
}

static int hist_e_h(GWindow gw, GEvent *event) {
    struct hist_dlg *hist = GDrawGetUserData(gw);
    int temp;
    const unichar_t *ret;
    unichar_t *end;

    if ( event->type==et_close ) {
	hist->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("histogram.html");
return( true );
	}
return( false );
    } else if ( event->type==et_resize ) {
	HistResize(hist);
    } else if ( event->type==et_expose ) {
	HistExpose(gw,hist);
    } else if ( event->type==et_mousemove ) {
	GGadgetEndPopup();
	HistPopup(hist,event);
    } else if ( event->type==et_mousedown ) {
	GGadgetEndPopup();
	HistPress(hist,event);
    } else if ( event->type==et_controlevent ) {
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    HistScroll(hist,&event->u.control.u.sb);
	  break;
	  case et_textchanged:
	    switch( GGadgetGetCid(event->u.control.g)) {
	      case CID_SumAround: case CID_BarWidth:
		ret = _GGadgetGetTitle(event->u.control.g);
		temp = u_strtol(ret,&end,10);
		if ( temp<0 || *end )
	      break;
		if ( GGadgetGetCid(event->u.control.g)==CID_SumAround ) {
		    hist->sum_around = temp;
		    HistFindMax(hist->h,temp);
		} else if ( temp==0 )
	      break;
		else {
		    hist->barwidth = temp;
		    HistRefigureSB(hist);
		}
		GDrawRequestExpose(gw,NULL,false);
	      break;
	    }
	  break;
	  case et_buttonactivate:
	    if ( GGadgetGetCid(event->u.control.g)==CID_OK ) {
		HistSet(hist);
	    } else
		hist->done = true;
	  break;
	}
    }
return( true );
}

static void CheckSmallSelection(uint8 *selected,EncMap *map,SplineFont *sf) {
    int i, cnt, tot;

    for ( i=cnt=tot=0; i<map->enccount; ++i ) {
	int gid = map->map[i];
	if ( gid!=-1 && sf->glyphs[gid]!=NULL ) {
	    ++tot;
	    if ( selected[i] )
		++cnt;
	}
    }
    if ( (cnt==1 && tot>1) || (cnt<8 && tot>30) )
	ff_post_notice(_("Tiny Selection"),_("There are so few glyphs selected that it seems unlikely to me that you will get a representative sample of this aspect of your font. If you deselect everything the command will apply to all glyphs in the font"));
}

void SFHistogram(SplineFont *sf,int layer, struct psdict *private, uint8 *selected,
	EncMap *map,enum hist_type which) {
    struct hist_dlg hist;
    GWindow gw;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[14];
    GTextInfo label[14];
    int i,j;
    char binsize[20], barwidth[20], *primary, *secondary;
    FontRequest rq;
    int as, ds, ld;
    static unichar_t n9999[] = { '9', '9', '9', '9', 0 };
    static GFont *font = NULL;

    memset(&hist,0,sizeof(hist));
    hist.sf = sf;
    hist.layer = layer;
    hist.private = private;
    if ( private==NULL ) private = sf->private;
    hist.selected = selected;
    hist.which = which;
    hist.barwidth = 6;
    hist.sum_around = 0;
    switch ( which ) {
      case hist_hstem:
	hist.h = HistFindHStemWidths(sf,layer,selected,map);
      break;
      case hist_vstem:
	hist.h = HistFindVStemWidths(sf,layer,selected,map);
      break;
      case hist_blues:
	hist.h = HistFindBlues(sf,layer,selected,map);
      break;
    }
    HistFindMax(hist.h,hist.sum_around);

    if ( selected!=NULL )
	CheckSmallSelection(selected,map,sf);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title =  which==hist_hstem?_("HStem") :
					      which==hist_vstem?_("VStem"):
							  _("Blues");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,210));
    hist.yoff = GDrawPointsToPixels(NULL,120);
    pos.height = pos.width + hist.yoff;
    hist.gw = gw = GDrawCreateTopWindow(NULL,&pos,hist_e_h,&hist,&wattrs);

    if ( font == NULL ) {
	memset(&rq,0,sizeof(rq));
	rq.utf8_family_name = SANS_UI_FAMILIES;
	rq.point_size = 10;
	rq.weight = 400;
	font = GDrawInstanciateFont(NULL,&rq);
	font = GResourceFindFont("Histogram.Font",font);
    }
    hist.font = font;
    GDrawFontMetrics(hist.font,&as,&ds,&ld);
    hist.fh = as+ds; hist.as = as;

    GDrawSetFont(gw,hist.font);
    hist.x = 10+GDrawGetTextWidth(gw,n9999,-1,NULL);
    hist.hwidth = pos.width - 2*hist.x;
    hist.y = 10; hist.hheight = pos.width-20;

    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));

    i=0;
    gcd[i].gd.pos.x = hist.x; gcd[i].gd.pos.y = hist.y+hist.hheight;
    gcd[i].gd.pos.width = hist.hwidth+1;
    gcd[i].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
    gcd[i].gd.cid = CID_ScrollBar;
    gcd[i++].creator = GScrollBarCreate;

    label[i].text = (unichar_t *) _("Sum Around:");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = GDrawPixelsToPoints(NULL,hist.y+hist.hheight)+12+10;
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.cid = CID_SumAroundL;
    gcd[i++].creator = GLabelCreate;

    sprintf(binsize,"%d", hist.sum_around);
    label[i].text = (unichar_t *) binsize;
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 64; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y-4;
    gcd[i].gd.pos.width = 30;
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.cid = CID_SumAround;
    gcd[i++].creator = GTextFieldCreate;

    label[i].text = (unichar_t *) _("Bar Width:");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 120; gcd[i].gd.pos.y = GDrawPixelsToPoints(NULL,hist.y+hist.hheight)+12+10;
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.cid = CID_BarWidthL;
    gcd[i++].creator = GLabelCreate;

    sprintf(barwidth,"%d", hist.barwidth);
    label[i].text = (unichar_t *) barwidth;
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 140+64-30-1; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y-4;
    gcd[i].gd.pos.width = 30;
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.cid = CID_BarWidth;
    gcd[i++].creator = GTextFieldCreate;

    label[i].text = (unichar_t *) _("BlueValues come in pairs. Select another.");
    label[i].text_is_1byte = true;
    label[i].fg = 0xff0000;
    label[i].bg = GDrawGetDefaultBackground(NULL);
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 8; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+28;
    gcd[i].gd.flags = gg_enabled;
    gcd[i].gd.cid = CID_BlueMsg;
    gcd[i++].creator = GLabelCreate;

    switch ( which ) {
      case hist_hstem:
	label[i].text = (unichar_t *) "StdHW:";
	label[i+2].text = (unichar_t *) "StemSnapH:";
	primary = "StdHW"; secondary = "StemSnapH";
      break;
      case hist_vstem:
	label[i].text = (unichar_t *) "StdVW:";
	label[i+2].text = (unichar_t *) "StemSnapV:";
	primary = "StdVW"; secondary = "StemSnapV";
      break;
      case hist_blues:
	label[i].text = (unichar_t *) "BlueValues:";
	label[i+2].text = (unichar_t *) "OtherBlues:";
	primary = "BlueValues"; secondary = "OtherBlues";
      break;
    }
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+28; 
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.cid = CID_MainValL;
    gcd[i++].creator = GLabelCreate;

    if ( private!=NULL && (j=PSDictFindEntry(private,primary))!=-1 ) {
	label[i].text = (unichar_t *) private->values[j];
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
    }
    gcd[i].gd.pos.x = 64; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y-4;
    gcd[i].gd.pos.width = 140;
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.cid = CID_MainVal;
    gcd[i++].creator = GTextFieldCreate;

    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+28; 
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.cid = CID_SecondaryValL;
    gcd[i++].creator = GLabelCreate;

    if ( private!=NULL && (j=PSDictFindEntry(private,secondary))!=-1 ) {
	label[i].text = (unichar_t *) private->values[j];
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
    }
    gcd[i].gd.pos.x = 64; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y-4;
    gcd[i].gd.pos.width = 140;
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.cid = CID_SecondaryVal;
    gcd[i++].creator = GTextFieldCreate;

    gcd[i].gd.pos.x = 15-3; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+26;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _("_OK");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_OK;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = -15; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _("_Cancel");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.mnemonic = 'C';
    gcd[i].gd.cid = CID_Cancel;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = 2; gcd[i].gd.pos.y = 2;
    gcd[i].gd.pos.width = pos.width-4; gcd[i].gd.pos.height = pos.height-4;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[i].gd.cid = CID_Group;
    gcd[i++].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);

    hist.hoff = hist.h->low;
    HistRefigureSB(&hist);

    GDrawSetVisible(gw,true);
    while ( !hist.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);

    HistDataFree(hist.h);
}
