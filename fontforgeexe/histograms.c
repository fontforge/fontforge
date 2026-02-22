/* Copyright (C) 2003-2012 by George Williams */
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

#include <fontforge-config.h>

#include "autohint.h"
#include "dumppfa.h"
#include "ffglib.h"
#include "fontforgeui.h"
#include "gkeysym.h"
#include "gresedit.h"
#include "psfont.h"
#include "splineutil.h"
#include "ustring.h"
#include "utype.h"

#include <math.h>

/* This operations are designed to work on a single font. NOT a CID collection*/
/*  A CID collection must be treated one sub-font at a time */

GResFont histogram_font = GRESFONT_INIT("400 10pt " SANS_UI_FAMILIES);
Color histogram_graphcol = 0x2020ff;

struct hentry {
    int cnt, sum;
    int char_cnt, max;
    SplineChar **chars;
};

typedef struct histdata {
    int low, high;
    struct hentry *hist;	/* array of high-low+1 elements */
    int tot, max;
} HistData;

static void HistDataFree(HistData *h) {
    int i;

    for ( i=h->low; i<=h->high; ++i )
	free(h->hist[i-h->low].chars);
    free(h->hist);
    free(h);
}

static HistData *HistFindBlues(SplineFont *sf,int layer, uint8_t *selected, EncMap *map) {
    int i, gid, low,high, top,bottom;
    SplineChar *sc;
    DBounds b;
    HistData *hist;
    struct hentry *h;

    hist = calloc(1,sizeof(HistData));
    hist->hist = calloc(sf->ascent+sf->descent+1,sizeof(struct hentry));
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
		    hist->hist = realloc(hist->hist,(top+10-low)*sizeof(struct hentry));
		    memset(hist->hist + high-low+1,0,(top+10-high-1)*sizeof(struct hentry));
		    high = top+10 -1;
		}
	    }
	    ++ hist->hist[top-low].cnt;
	    if ( hist->hist[top-low].char_cnt >= hist->hist[top-low].max ) {
		if ( hist->hist[top-low].max==0 )
		    hist->hist[top-low].chars = malloc(10*sizeof(SplineChar *));
		else
		    hist->hist[top-low].chars = realloc(hist->hist[top-low].chars,(hist->hist[top-low].max+10)*sizeof(SplineChar *));
		hist->hist[top-low].max += 10;
	    }
	    hist->hist[top-low].chars[hist->hist[top-low].char_cnt++] = sc;

	    if ( bottom<hist->low ) {
		hist->low = bottom;
		if ( bottom<low ) {
		    h = calloc((high-bottom+10),sizeof( struct hentry ));
		    memcpy(h+low-(bottom-10+1),hist->hist,(high+1-low)*sizeof(struct hentry));
		    low = bottom-10+1;
		    free( hist->hist );
		    hist->hist = h;
		}
	    }
	    ++ hist->hist[bottom-low].cnt;
	    if ( hist->hist[bottom-low].char_cnt >= hist->hist[bottom-low].max ) {
		if ( hist->hist[bottom-low].max==0 )
		    hist->hist[bottom-low].chars = malloc(10*sizeof(SplineChar *));
		else
		    hist->hist[bottom-low].chars = realloc(hist->hist[bottom-low].chars,(hist->hist[bottom-low].max+10)*sizeof(SplineChar *));
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
	h = malloc((hist->high-hist->low+1)*sizeof(struct hentry));
	memcpy(h,hist->hist + hist->low-low,(hist->high-hist->low+1)*sizeof(struct hentry));
	free(hist->hist);
	hist->hist = h;
    }
return( hist );
}

static HistData *HistFindStemWidths(SplineFont *sf,int layer, uint8_t *selected,EncMap *map,int hor) {
    int i, gid, low,high, val;
    SplineChar *sc;
    HistData *hist;
    struct hentry *h;
    StemInfo *stem;

    hist = calloc(1,sizeof(HistData));
    hist->hist = calloc(sf->ascent+sf->descent+1,sizeof(struct hentry));
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
			hist->hist = realloc(hist->hist,(val+10-low)*sizeof(struct hentry));
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
			    hist->hist[val-low].chars = malloc(10*sizeof(SplineChar *));
			else
			    hist->hist[val-low].chars = realloc(hist->hist[val-low].chars,(hist->hist[val-low].max+10)*sizeof(SplineChar *));
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
	h = malloc((hist->high-hist->low+1)*sizeof(struct hentry));
	memcpy(h,hist->hist + hist->low-low,(hist->high-hist->low+1)*sizeof(struct hentry));
	free(hist->hist);
	hist->hist = h;
    }
return( hist );
}

static HistData *HistFindHStemWidths(SplineFont *sf,int layer, uint8_t *selected,EncMap *map) {
return( HistFindStemWidths(sf,layer,selected,map,true) );
}

static HistData *HistFindVStemWidths(SplineFont *sf,int layer, uint8_t *selected,EncMap *map) {
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

#define CID_LeftSide		4001
#define CID_Histogram		4002
#define CID_RightSide		4003

struct hist_dlg {
    enum hist_type which;
    SplineFont *sf;
    int layer;
    struct psdict *private;
    uint8_t *selected;
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
    int x = e->u.mouse.x;
    struct hentry *h;
    static char buffer[300];
    char *end = buffer + sizeof(buffer)/sizeof(buffer[0]), *pt, *line;
    int i;

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

static char *ArrayOrder(char *old,int args,int val1,int val2) {
    char *end;
    double array[40];
    int i,j,k;
    GString *new;

    if ( *old=='[' ) ++old;

    for ( i=0; i<40 && *old!=']' && *old!='\0'; ++i ) {
	array[i] = strtod(old,&end);
	if ( old==end )
    break;
	old = end;
	while ( *old==' ' ) ++old;
    }
    if (i<40)
        array[i++] = val1;
    if (i<40) {
        if ( args==2 )
	    array[i++] = val2;
    }
    for ( j=0; j<i; ++j ) for ( k=j+1; k<i; ++k ) if ( array[j]>array[k] ) {
	double temp = array[j];
	array[j] = array[k];
	array[k] = temp;
    }

    new = g_string_new( "[" );
    for ( k=0; k<i; ++k ) {
	if (k == i-1)
	    g_string_append_printf( new, "%g]", array[k] );
	else
	    g_string_append_printf( new, "%g ", array[k] );
    }

return( (char *) g_string_free( new, FALSE ) );
}

/* Handle clicks on histogram chart and update text fields below accordingly */
static void HistPress(struct hist_dlg *hist,GEvent *e) {
    char *old = NULL;
    char *new = NULL;
    int x = e->u.mouse.x;

    x /= hist->barwidth;
    x += hist->hoff;
    if ( x > hist->h->high || x<hist->h->low )
return;

    if ( hist->which==hist_blues ) {
	if ( hist->is_pending ) {
	    if ( x<hist->pending_blue )
		ff_post_error(_("Bad Value"),_("The smaller number must be selected first in a pair of bluevalues"));
	    else if ( x<0 ) {	/* OtherBlues */
		old = GGadgetGetTitle8( GWidgetGetControl( hist->gw, CID_SecondaryVal ));
		new = ArrayOrder( old, 2, hist->pending_blue, x );
		GGadgetSetTitle8( GWidgetGetControl( hist->gw, CID_SecondaryVal ), new );
	    } else {
		old = GGadgetGetTitle8( GWidgetGetControl( hist->gw, CID_MainVal ));
		new = ArrayOrder( old, 2, hist->pending_blue, x );
		GGadgetSetTitle8( GWidgetGetControl( hist->gw, CID_MainVal ), new );
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
    } else { /* HStem and VStem */
	if ( !( e->u.mouse.state&ksm_shift )) {
	    new = smprintf( "[%d]", x );
	    GGadgetSetTitle8( GWidgetGetControl( hist->gw, CID_MainVal ), new );
	    GGadgetSetTitle8( GWidgetGetControl( hist->gw, CID_SecondaryVal ), new );
	} else {
	    old = GGadgetGetTitle8( GWidgetGetControl( hist->gw, CID_SecondaryVal ));
	    new = ArrayOrder( old, 1, x, 0 );
	    GGadgetSetTitle8( GWidgetGetControl( hist->gw, CID_SecondaryVal ), new );
	}
    }
    free( old );
    free( new );
}

static void HistExpose(GWindow pixmap, struct hist_dlg *hist) {
    GRect r,old;
    int height;
    double yscale;
    int i;
    char buf[20];
    GRect size;
    GDrawGetSize(GDrawableGetWindow(GWidgetGetControl(hist->gw,CID_Histogram)),&size);

    height = size.height-hist->fh-2;
    yscale = (4*height/5.0)/(hist->h->max-0);

    Color fg = GDrawGetDefaultForeground(NULL);

    GDrawSetLineWidth(pixmap,0);
    r.x = 0; r.y = 0;
    r.width = size.width-1; r.height = height-1;
    GDrawDrawRect(pixmap,&r,fg);

    ++r.x; r.width--;
    ++r.y; r.height--;
    GDrawPushClip(pixmap,&r,&old);

    for ( i=hist->hoff; (i-hist->hoff)*hist->barwidth<size.width-2 && i<=hist->h->high; ++i ) {
	r.x = (i-hist->hoff)*hist->barwidth+1; r.width = hist->barwidth;
	r.height = rint(hist->h->hist[i-hist->h->low].sum * yscale);
	if ( r.height>=0 ) {
	    r.y = height - r.height;
	    GDrawFillRect(pixmap,&r,histogram_graphcol);
	}
    }

    GDrawPopClip(pixmap,&old);

    GDrawSetFont(pixmap,hist->font);
    sprintf(buf,"%d",hist->hoff);
    GDrawDrawText8(pixmap,0,height+2+hist->as, buf,-1,fg);
    sprintf(buf,"%d",hist->hoff+hist->hwidth/hist->barwidth);
    GDrawDrawText8(pixmap,size.width-GDrawGetText8Width(pixmap,buf,-1),height+2+hist->as,
	    buf,-1,fg);
}

static void HistRExpose(GWindow pixmap, struct hist_dlg *hist) {
    int height;
    double yscale;
    GRect size;
    char buf[20];

    GDrawGetSize(GDrawableGetWindow(GWidgetGetControl(hist->gw,CID_RightSide)),&size);
    height = size.height-hist->fh-2;
    yscale = (4*height/5.0)/(hist->h->max-0);

    sprintf(buf,"%d",hist->h->max);
    GDrawDrawText8(pixmap,1,height-rint(hist->h->max*yscale),
	    buf,-1,GDrawGetDefaultForeground(NULL));
}

static void HistLExpose(GWindow pixmap, struct hist_dlg *hist) {
    int height;
    double yscale;
    GRect size;
    char buf[20];

    GDrawGetSize(GDrawableGetWindow(GWidgetGetControl(hist->gw,CID_LeftSide)),&size);
    height = size.height-hist->fh-2;
    yscale = (4*height/5.0)/(hist->h->max-0);

    sprintf(buf,"%d",hist->h->max);
    GDrawDrawText8(pixmap,size.width-GDrawGetText8Width(pixmap,buf,-1)-1,height-rint(hist->h->max*yscale),
	    buf,-1,GDrawGetDefaultForeground(NULL));
}

static void HistScroll(struct hist_dlg *hist,struct sbevent *sb) {
    int newpos = hist->hoff;
    int cols;
    GRect size;
    GGadget *g = GWidgetGetControl(hist->gw,CID_ScrollBar);

    GGadgetGetSize(g,&size);
    cols = (size.width-2)/hist->barwidth;

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
      case et_sb_halfup: case et_sb_halfdown: break;
    }
    if ( newpos>(hist->h->high+1-hist->h->low)-cols + hist->h->low )
        newpos = (hist->h->high+1-hist->h->low)-cols + hist->h->low;
    if ( newpos<hist->h->low ) newpos = hist->h->low;
    if ( newpos!=hist->hoff ) {
	/*int diff = newpos-hist->hoff;*/
	hist->hoff = newpos;
	GScrollBarSetPos(g,hist->hoff);
	GDrawRequestExpose(GDrawableGetWindow(GWidgetGetControl(hist->gw,CID_Histogram)),NULL,false);
    }
}

static void HistRefigureSB(struct hist_dlg *hist) {
    GGadget *g = GWidgetGetControl(hist->gw,CID_ScrollBar);
    int width, hoff, cols;
    GRect size;

    GGadgetGetSize(g,&size);
    width = size.width-2;
    cols = width/hist->barwidth;

    GScrollBarSetBounds(g,hist->h->low,hist->h->high+1,cols);
    if ( hist->hoff+cols >hist->h->high ) {
	hoff = hist->h->high-cols;
	if ( hoff<0 ) hoff = 0;
	if ( hoff!=hist->hoff ) {
	    hist->hoff = hoff;
	    GScrollBarSetPos(g,hoff);
	}
    }
}

static void HistResize(struct hist_dlg *hist) {

    HistRefigureSB(hist);
    GDrawRequestExpose(hist->gw,NULL,false);
}
	
static void HistSet(struct hist_dlg *hist) {
    char *primary, *secondary;
    char *temp;
    struct psdict *p = hist->private ? hist->private : hist->sf->private_dict;
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
	hist->sf->private_dict = p = calloc(1,sizeof(struct psdict));
	p->cnt = 10;
	p->keys = calloc(10,sizeof(char *));
	p->values = calloc(10,sizeof(char *));
    }
    PSDictChangeEntry(p,primary,temp=cu_copy(ret1)); free(temp);
    PSDictChangeEntry(p,secondary,temp=cu_copy(ret2)); free(temp);
}

static int leftside_e_h(GWindow gw, GEvent *event) {
    struct hist_dlg *hist = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_char:
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/histogram.html", NULL);
return( true );
	}
return( false );
      break;
      case et_expose:
	HistLExpose(gw,hist);
      break;
      case et_mousemove:
      case et_mousedown:
	GGadgetEndPopup();
      break;
      default: break;
    }
return( true );
}

static int rightside_e_h(GWindow gw, GEvent *event) {
    struct hist_dlg *hist = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_char:
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/histogram.html", NULL);
return( true );
	}
return( false );
      break;
      case et_expose:
	HistRExpose(gw,hist);
      break;
      case et_mousemove:
      case et_mousedown:
	GGadgetEndPopup();
      break;
      default: break;
    }
return( true );
}

static int histogram_e_h(GWindow gw, GEvent *event) {
    struct hist_dlg *hist = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_char:
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/histogram.html", NULL);
return( true );
	}
return( false );
      break;
      case et_expose:
	HistExpose(gw,hist);
      break;
      case et_mousemove:
	GGadgetEndPopup();
	HistPopup(hist,event);
      break;
      case et_mousedown:
	GGadgetEndPopup();
	HistPress(hist,event);
      break;
      default: break;
    }
return( true );
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
	    help("ui/dialogs/histogram.html", NULL);
return( true );
	}
return( false );
    } else if ( event->type==et_resize ) {
	HistResize(hist);
    } else if ( event->type==et_mousemove ) {
	GGadgetEndPopup();
    } else if ( event->type==et_mousedown ) {
	GGadgetEndPopup();
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
		GDrawRequestExpose(GDrawableGetWindow(GWidgetGetControl(gw,CID_Histogram)),NULL,false);
		GDrawRequestExpose(GDrawableGetWindow(GWidgetGetControl(gw,CID_LeftSide)),NULL,false);
		GDrawRequestExpose(GDrawableGetWindow(GWidgetGetControl(gw,CID_RightSide)),NULL,false);
	      break;
	    }
	  break;
	  case et_buttonactivate:
	    if ( GGadgetGetCid(event->u.control.g)==CID_OK ) {
		HistSet(hist);
	    } else
		hist->done = true;
	  break;
	  default: break;
	}
    }
return( true );
}

static void CheckSmallSelection(uint8_t *selected,EncMap *map,SplineFont *sf) {
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

void SFHistogram(SplineFont *sf,int layer, struct psdict *private, uint8_t *selected,
	EncMap *map,enum hist_type which) {
    struct hist_dlg hist;
    GWindow gw;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[17], boxes[6], *hv[4][2], *butarray[9], *hvctls[5][5], *hvbody[3][4];
    GTextInfo label[17];
    int i,j;
    char binsize[20], barwidth[20], *primary, *secondary;
    int as, ds, ld;
    static unichar_t n9999[] = { '9', '9', '9', '9', 0 };

    memset(&hist,0,sizeof(hist));
    hist.sf = sf;
    hist.layer = layer;
    hist.private = private;
    if ( private==NULL ) private = sf->private_dict;
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

    hist.font = histogram_font.fi;
    GDrawWindowFontMetrics(gw,hist.font,&as,&ds,&ld);
    hist.fh = as+ds; hist.as = as;

    GDrawSetFont(gw,hist.font);
    hist.x = 10+GDrawGetTextWidth(gw,n9999,-1);
    hist.hwidth = pos.width - 2*hist.x;
    hist.y = 10; hist.hheight = pos.width-20;

    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));
    memset(&boxes,0,sizeof(boxes));

    i=0;
    gcd[i].gd.pos.width = hist.x; gcd[i].gd.pos.height = 200;
    gcd[i].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
    gcd[i].gd.cid = CID_LeftSide;
    gcd[i].gd.u.drawable_e_h = leftside_e_h;
    gcd[i++].creator = GDrawableCreate;
    hvbody[0][0] = &gcd[i-1];

    gcd[i].gd.pos.width = hist.hwidth+1; gcd[i].gd.pos.height = 200;
    gcd[i].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
    gcd[i].gd.cid = CID_Histogram;
    gcd[i].gd.u.drawable_e_h = histogram_e_h;
    gcd[i++].creator = GDrawableCreate;
    hvbody[0][1] = &gcd[i-1];

    gcd[i].gd.pos.width = hist.x; gcd[i].gd.pos.height = 200;
    gcd[i].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
    gcd[i].gd.cid = CID_RightSide;
    gcd[i].gd.u.drawable_e_h = rightside_e_h;
    gcd[i++].creator = GDrawableCreate;
    hvbody[0][2] = &gcd[i-1];
    hvbody[0][3] = NULL;

    hvbody[1][0] = GCD_Glue;
    gcd[i].gd.pos.width = hist.hwidth+1;
    gcd[i].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
    gcd[i].gd.cid = CID_ScrollBar;
    gcd[i++].creator = GScrollBarCreate;
    hvbody[1][1] = &gcd[i-1];
    hvbody[1][2] = GCD_Glue;
    hvbody[1][3] = NULL;
    hvbody[2][0] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = &hvbody[0][0];
    boxes[2].creator = GHVBoxCreate;

    label[i].text = (unichar_t *) _("Sum Around:");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.cid = CID_SumAroundL;
    gcd[i++].creator = GLabelCreate;
    hvctls[0][0] = &gcd[i-1];

    sprintf(binsize,"%d", hist.sum_around);
    label[i].text = (unichar_t *) binsize;
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.width = 30;
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.cid = CID_SumAround;
    gcd[i++].creator = GTextFieldCreate;
    hvctls[0][1] = &gcd[i-1];

    label[i].text = (unichar_t *) _("Bar Width:");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.cid = CID_BarWidthL;
    gcd[i++].creator = GLabelCreate;
    hvctls[0][2] = &gcd[i-1];

    sprintf(barwidth,"%d", hist.barwidth);
    label[i].text = (unichar_t *) barwidth;
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.width = 30;
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.cid = CID_BarWidth;
    gcd[i++].creator = GTextFieldCreate;
    hvctls[0][3] = &gcd[i-1];
    hvctls[0][4] = NULL;

    label[i].text = (unichar_t *) _("BlueValues come in pairs. Select another.");
    label[i].text_is_1byte = true;
    label[i].fg = GDrawGetWarningForeground(NULL);
    label[i].bg = GDrawGetDefaultBackground(NULL);
    gcd[i].gd.label = &label[i];
    gcd[i].gd.flags = gg_enabled;
    gcd[i].gd.cid = CID_BlueMsg;
    gcd[i++].creator = GLabelCreate;
    hvctls[1][0] = &gcd[i-1];
    hvctls[1][1] = hvctls[1][2] = hvctls[1][3] = GCD_ColSpan;
    hvctls[1][4] = NULL;

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
    hvctls[2][0] = &gcd[i-1];

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
    hvctls[2][1] = &gcd[i-1];
    hvctls[2][2] = hvctls[2][3] = GCD_ColSpan;
    hvctls[2][4] = NULL;

    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+28; 
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.cid = CID_SecondaryValL;
    gcd[i++].creator = GLabelCreate;
    hvctls[3][0] = &gcd[i-1];

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
    hvctls[3][1] = &gcd[i-1];
    hvctls[3][2] = hvctls[3][3] = GCD_ColSpan;
    hvctls[3][4] = NULL;
    hvctls[4][0] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = &hvctls[0][0];
    boxes[3].creator = GHVBoxCreate;

    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _("_OK");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_OK;
    gcd[i++].creator = GButtonCreate;
    butarray[0] = GCD_Glue; butarray[1] = &gcd[i-1]; butarray[2] = GCD_Glue; butarray[3] = GCD_Glue;

    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _("_Cancel");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.mnemonic = 'C';
    gcd[i].gd.cid = CID_Cancel;
    gcd[i++].creator = GButtonCreate;
    butarray[4] = GCD_Glue; butarray[5] = &gcd[i-1]; butarray[6] = GCD_Glue; butarray[7] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = &butarray[0];
    boxes[4].creator = GHBoxCreate;

    hv[0][0] = &boxes[2]; hv[0][1] = NULL;
    hv[1][0] = &boxes[3]; hv[1][1] = NULL;
    hv[2][0] = &boxes[4]; hv[2][1] = NULL; hv[3][0] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = &hv[0][0];
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);

    GHVBoxSetExpandableRow(boxes[0].ret,0);
    GHVBoxSetExpandableCol(boxes[2].ret,1);
    GHVBoxSetExpandableRow(boxes[2].ret,0);
    GHVBoxSetExpandableCol(boxes[3].ret,1);
    GHVBoxSetExpandableCol(boxes[4].ret,gb_expandglue);

    hist.hoff = 0;
    if ( hist.h->low>0 )
	hist.hoff = hist.h->low;
    GScrollBarSetPos(GWidgetGetControl(hist.gw,CID_ScrollBar),hist.hoff);
    HistRefigureSB(&hist);

    GHVBoxFitWindow(boxes[0].ret);
    GDrawSetVisible(gw,true);
    while ( !hist.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);

    HistDataFree(hist.h);
}
