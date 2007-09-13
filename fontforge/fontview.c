/* Copyright (C) 2000-2007 by George Williams */
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
#include "groups.h"
#include "psfont.h"
#include <gfile.h>
#ifdef FONTFORGE_CONFIG_GTK
#include <gtkutype.h>
#else
#include <ustring.h>
#include <gkeysym.h>
#include <utype.h>
#include <chardata.h>
#include <gresource.h>
#endif
#include <math.h>
#include <unistd.h>

int onlycopydisplayed = 0;
int copymetadata = 0;
int copyttfinstr = 0;

struct compressors compressors[] = {
    { ".gz", "gunzip", "gzip" },
    { ".bz2", "bunzip2", "bzip2" },
    { ".Z", "gunzip", "compress" },
    NULL
};

#define XOR_COLOR	0x505050
#define	FV_LAB_HEIGHT	15

#ifdef FONTFORGE_CONFIG_GDRAW
/* GTK window icons are set by glade */
#ifdef BIGICONS
#define fontview_width 32
#define fontview_height 32
static unsigned char fontview_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xff, 0x02, 0x20, 0x80, 0x00,
   0x82, 0x20, 0x86, 0x08, 0x42, 0x21, 0x8a, 0x14, 0xc2, 0x21, 0x86, 0x04,
   0x42, 0x21, 0x8a, 0x14, 0x42, 0x21, 0x86, 0x08, 0x02, 0x20, 0x80, 0x00,
   0xaa, 0xaa, 0xaa, 0xaa, 0x02, 0x20, 0x80, 0x00, 0x82, 0xa0, 0x8f, 0x18,
   0x82, 0x20, 0x91, 0x24, 0x42, 0x21, 0x91, 0x02, 0x42, 0x21, 0x91, 0x02,
   0x22, 0x21, 0x8f, 0x02, 0xe2, 0x23, 0x91, 0x02, 0x12, 0x22, 0x91, 0x02,
   0x3a, 0x27, 0x91, 0x24, 0x02, 0xa0, 0x8f, 0x18, 0x02, 0x20, 0x80, 0x00,
   0xfe, 0xff, 0xff, 0xff, 0x02, 0x20, 0x80, 0x00, 0x42, 0x20, 0x86, 0x18,
   0xa2, 0x20, 0x8a, 0x04, 0xa2, 0x20, 0x86, 0x08, 0xa2, 0x20, 0x8a, 0x10,
   0x42, 0x20, 0x8a, 0x0c, 0x82, 0x20, 0x80, 0x00, 0x02, 0x20, 0x80, 0x00,
   0xaa, 0xaa, 0xaa, 0xaa, 0x02, 0x20, 0x80, 0x00};
#else
#define fontview2_width 16
#define fontview2_height 16
static unsigned char fontview2_bits[] = {
   0x00, 0x07, 0x80, 0x08, 0x40, 0x17, 0x40, 0x15, 0x60, 0x09, 0x10, 0x02,
   0xa0, 0x01, 0xa0, 0x00, 0xa0, 0x00, 0xa0, 0x00, 0x50, 0x00, 0x52, 0x00,
   0x55, 0x00, 0x5d, 0x00, 0x22, 0x00, 0x1c, 0x00};
#endif

extern int _GScrollBar_Width;

static int fv_fontsize = -13, fv_fs_init=0;
extern int use_freetype_to_rasterize_fv;
#endif

enum glyphlable { gl_glyph, gl_name, gl_unicode, gl_encoding };
int default_fv_font_size = 24, default_fv_antialias=true,
	default_fv_bbsized=true,
	default_fv_showhmetrics=false, default_fv_showvmetrics=false,
	default_fv_glyphlabel = gl_glyph;
#define METRICS_BASELINE 0x0000c0
#define METRICS_ORIGIN	 0xc00000
#define METRICS_ADVANCE	 0x008000
FontView *fv_list=NULL;

#if defined(FONTFORGE_CONFIG_GTK)
# define FV_From_MI(menuitem)	\
	(FontView *) g_object_get_data( \
		G_OBJECT( gtk_widget_get_toplevel( GTK_WIDGET( menuitem ))),\
		"data" )
#endif

void FVToggleCharChanged(SplineChar *sc) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    int i, j;
    int pos;
    FontView *fv;

    for ( fv = sc->parent->fv; fv!=NULL; fv=fv->nextsame ) {
	if ( fv->sf!=sc->parent )		/* Can happen in CID fonts if char's parent is not currently active */
    continue;
	if ( fv->v==NULL || fv->colcnt==0 )	/* Can happen in scripts */
    continue;
	for ( pos=0; pos<fv->map->enccount; ++pos ) if ( fv->map->map[pos]==sc->orig_pos ) {
	    if ( fv->mapping!=NULL ) {
		for ( i=0; i<fv->mapcnt; ++i )
		    if ( fv->mapping[i]==pos )
		break;
		if ( i==fv->mapcnt )		/* Not currently displayed */
	continue;
		pos = i;
	    }
	    i = pos / fv->colcnt;
	    j = pos - i*fv->colcnt;
	    i -= fv->rowoff;
 /* Normally we should be checking against fv->rowcnt (rather than <=rowcnt) */
 /*  but every now and then the WM forces us to use a window size which doesn't */
 /*  fit our expectations (maximized view) and we must be prepared for half */
 /*  lines */
	    if ( i>=0 && i<=fv->rowcnt ) {
# ifdef FONTFORGE_CONFIG_GDRAW
		GRect r;
#if 0
		Color bg;
		if ( sc->color!=COLOR_DEFAULT )
		    bg = sc->color;
		else if ( sc->layers[ly_back].splines!=NULL || sc->layers[ly_back].images!=NULL )
		    bg = 0x808080;
		else
		    bg = GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(fv->v));
#endif
		r.x = j*fv->cbw+1; r.width = fv->cbw-1;
		r.y = i*fv->cbh+1; r.height = fv->lab_height-1;
		GDrawRequestExpose(fv->v,&r,false);
#if 0
		GDrawSetXORBase(fv->v,bg);
		GDrawSetXORMode(fv->v);
		GDrawFillRect(fv->v,&r,0x000000);
		GDrawSetCopyMode(fv->v);
#endif
# elif defined(FONTFORGE_CONFIG_GTK)
		GdkGC *gc = fv->v->style->fg_gc[fv->v->state];
		GdkGCValues values;
		GdkColor bg;
		gdk_gc_get_values(gc,&values);
		bg.pixel = -1;
		if ( sc->color!=COLOR_DEFAULT ) {
		    bg.red   = ((sc->color>>16)&0xff)*0x101;
		    bg.green = ((sc->color>>8 )&0xff)*0x101;
		    bg.blue  = ((sc->color    )&0xff)*0x101;
		} else if ( sc->layers[ly_back].splines!=NULL || sc->layers[ly_back].images!=NULL )
		    bg.red = bg.green = bg.blue = 0x8000;
		else
		    bg = values.background;
		/* Bug!!! This only works on RealColor */
		bg.red ^= values.foreground.red;
		bg.green ^= values.foreground.green;
		bg.blue ^= values.foreground.blue;
		/* End bug */
		gdk_gc_set_function(gc,GDK_XOR);
		gdk_gc_set_foreground(gc, &bg);
		gdk_draw_rectangle(fv->v->window, gc, TRUE,
			j*fv->cbw+1, i*fv->cbh+1,  fv->cbw-1, fv->lab_height);
		gdk_gc_set_values(gc,&values,
			GDK_GC_FOREGROUND | GDK_GC_FUNCTION );
# endif
	    }
	}
    }
#endif
}

void FVMarkHintsOutOfDate(SplineChar *sc) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    int i, j;
    int pos;
    FontView *fv;

    if ( sc->parent->onlybitmaps || sc->parent->multilayer || sc->parent->strokedfont || sc->parent->order2 )
return;
    for ( fv = sc->parent->fv; fv!=NULL; fv=fv->nextsame ) {
	if ( fv->sf!=sc->parent )		/* Can happen in CID fonts if char's parent is not currently active */
    continue;
	if ( fv->v==NULL || fv->colcnt==0 )	/* Can happen in scripts */
    continue;
	for ( pos=0; pos<fv->map->enccount; ++pos ) if ( fv->map->map[pos]==sc->orig_pos ) {
	    if ( fv->mapping!=NULL ) {
		for ( i=0; i<fv->mapcnt; ++i )
		    if ( fv->mapping[i]==pos )
		break;
		if ( i==fv->mapcnt )		/* Not currently displayed */
	continue;
		pos = i;
	    }
	    i = pos / fv->colcnt;
	    j = pos - i*fv->colcnt;
	    i -= fv->rowoff;
 /* Normally we should be checking against fv->rowcnt (rather than <=rowcnt) */
 /*  but every now and then the WM forces us to use a window size which doesn't */
 /*  fit our expectations (maximized view) and we must be prepared for half */
 /*  lines */
	    if ( i>=0 && i<=fv->rowcnt ) {
# ifdef FONTFORGE_CONFIG_GDRAW
		GRect r;
		Color hintcol = 0x0000ff;
		r.x = j*fv->cbw+1; r.width = fv->cbw-1;
		r.y = i*fv->cbh+1; r.height = fv->lab_height-1;
		GDrawDrawLine(fv->v,r.x,r.y,r.x,r.y+r.height-1,hintcol);
		GDrawDrawLine(fv->v,r.x+1,r.y,r.x+1,r.y+r.height-1,hintcol);
		GDrawDrawLine(fv->v,r.x+r.width-1,r.y,r.x+r.width-1,r.y+r.height-1,hintcol);
		GDrawDrawLine(fv->v,r.x+r.width-2,r.y,r.x+r.width-2,r.y+r.height-1,hintcol);
# elif defined(FONTFORGE_CONFIG_GTK)
		GdkColor hintcol;
		GdkRectangle r;
		hintcol.red = hintcol.green = 0;
		hintcol.blue = 0xffff;
		gdk_gc_set_rgb_fg_color(fv->gc,&hintcol);
		r.x = j*fv->cbw+1; r.width = fv->cbw-1;
		r.y = i*fv->cbh+1; r.height = fv->lab_height-1;
		gdk_draw_line(GDK_DRAWABLE(fv->v),fv->gc,r.x,r.y,r.x,r.y+r.height-1);
		gdk_draw_line(GDK_DRAWABLE(fv->v),fv->gc,r.x+1,r.y,r.x+1,r.y+r.height-1);
		gdk_draw_line(GDK_DRAWABLE(fv->v),fv->gc,r.x+r.width-1,r.y,r.x+r.width-1,r.y+r.height-1);
		gdk_draw_line(GDK_DRAWABLE(fv->v),fv->gc,r.x+r.width-2,r.y,r.x+r.width-2,r.y+r.height-1);
# endif
	    }
	}
    }
#endif
}

static void FVToggleCharSelected(FontView *fv,int enc) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    int i, j;

    if ( fv->v==NULL || fv->colcnt==0 )	/* Can happen in scripts */
return;

    i = enc / fv->colcnt;
    j = enc - i*fv->colcnt;
    i -= fv->rowoff;
 /* Normally we should be checking against fv->rowcnt (rather than <=rowcnt) */
 /*  but every now and then the WM forces us to use a window size which doesn't */
 /*  fit our expectations (maximized view) and we must be prepared for half */
 /*  lines */
    if ( i>=0 && i<=fv->rowcnt ) {
# ifdef FONTFORGE_CONFIG_GDRAW
	GRect r;
	r.x = j*fv->cbw+1; r.width = fv->cbw-1;
	r.y = i*fv->cbh+fv->lab_height+1; r.height = fv->cbw;
	GDrawSetXORBase(fv->v,GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(fv->v)));
	GDrawSetXORMode(fv->v);
	GDrawFillRect(fv->v,&r,XOR_COLOR);
	GDrawSetCopyMode(fv->v);
# elif defined(FONTFORGE_CONFIG_GTK)
	GdkGC *gc = fv->v->style->fg_gc[fv->v->state];
	GdkGCValues values;
	gdk_gc_get_values(gc,&values);
	gdk_gc_set_function(gc,GDK_XOR);
	gdk_gc_set_foreground(gc, &values.background);
	gdk_draw_rectangle(fv->v->window, gc, TRUE,
		j*fv->cbw+1, i*fv->cbh+fv->lab_height+1,  fv->cbw-1, fv->cbw);
	gdk_gc_set_values(gc,&values,
		GDK_GC_FOREGROUND | GDK_GC_FUNCTION );
# endif
    }
#endif
}

void SFUntickAll(SplineFont *sf) {
    int i;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	sf->glyphs[i]->ticked = false;
}

void FVDeselectAll(FontView *fv) {
    int i;

    for ( i=0; i<fv->map->enccount; ++i ) {
	if ( fv->selected[i] ) {
	    fv->selected[i] = false;
	    FVToggleCharSelected(fv,i);
	}
    }
    fv->sel_index = 0;
}

static void FVInvertSelection(FontView *fv) {
    int i;

    for ( i=0; i<fv->map->enccount; ++i ) {
	fv->selected[i] = !fv->selected[i];
	FVToggleCharSelected(fv,i);
    }
    fv->sel_index = 1;
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void FVSelectAll(FontView *fv) {
    int i;

    for ( i=0; i<fv->map->enccount; ++i ) {
	if ( !fv->selected[i] ) {
	    fv->selected[i] = true;
	    FVToggleCharSelected(fv,i);
	}
    }
    fv->sel_index = 1;
}

static char *SubMatch(char *pattern, char *eop, char *name,int ignorecase) {
    char ch, *ppt, *npt, *ept, *eon;

    while ( pattern<eop && ( ch = *pattern)!='\0' ) {
	if ( ch=='*' ) {
	    if ( pattern[1]=='\0' )
return( name+strlen(name));
	    for ( npt=name; ; ++npt ) {
		if ( (eon = SubMatch(pattern+1,eop,npt,ignorecase))!= NULL )
return( eon );
		if ( *npt=='\0' )
return( NULL );
	    }
	} else if ( ch=='?' ) {
	    if ( *name=='\0' )
return( NULL );
	    ++name;
	} else if ( ch=='[' ) {
	    /* [<char>...] matches the chars
	    /* [<char>-<char>...] matches any char within the range (inclusive)
	    /* the above may be concattenated and the resultant pattern matches
	    /*		anything thing which matches any of them.
	    /* [^<char>...] matches any char which does not match the rest of
	    /*		the pattern
	    /* []...] as a special case a ']' immediately after the '[' matches
	    /*		itself and does not end the pattern */
	    int found = 0, not=0;
	    ++pattern;
	    if ( pattern[0]=='^' ) { not = 1; ++pattern; }
	    for ( ppt = pattern; (ppt!=pattern || *ppt!=']') && *ppt!='\0' ; ++ppt ) {
		ch = *ppt;
		if ( ppt[1]=='-' && ppt[2]!=']' && ppt[2]!='\0' ) {
		    char ch2 = ppt[2];
		    if ( (*name>=ch && *name<=ch2) ||
			    (ignorecase && islower(ch) && islower(ch2) &&
				    *name>=toupper(ch) && *name<=toupper(ch2)) ||
			    (ignorecase && isupper(ch) && isupper(ch2) &&
				    *name>=tolower(ch) && *name<=tolower(ch2))) {
			if ( !not ) {
			    found = 1;
	    break;
			}
		    } else {
			if ( not ) {
			    found = 1;
	    break;
			}
		    }
		    ppt += 2;
		} else if ( ch==*name || (ignorecase && tolower(ch)==tolower(*name)) ) {
		    if ( !not ) {
			found = 1;
	    break;
		    }
		} else {
		    if ( not ) {
			found = 1;
	    break;
		    }
		}
	    }
	    if ( !found )
return( NULL );
	    while ( *ppt!=']' && *ppt!='\0' ) ++ppt;
	    pattern = ppt;
	    ++name;
	} else if ( ch=='{' ) {
	    /* matches any of a comma seperated list of substrings */
	    for ( ppt = pattern+1; *ppt!='\0' ; ppt = ept ) {
		for ( ept=ppt; *ept!='}' && *ept!=',' && *ept!='\0'; ++ept );
		npt = SubMatch(ppt,ept,name,ignorecase);
		if ( npt!=NULL ) {
		    char *ecurly = ept;
		    while ( *ecurly!='}' && ecurly<eop && *ecurly!='\0' ) ++ecurly;
		    if ( (eon=SubMatch(ecurly+1,eop,npt,ignorecase))!=NULL )
return( eon );
		}
		if ( *ept=='}' )
return( NULL );
		if ( *ept==',' ) ++ept;
	    }
	} else if ( ch==*name ) {
	    ++name;
	} else if ( ignorecase && tolower(ch)==tolower(*name)) {
	    ++name;
	} else
return( NULL );
	++pattern;
    }
return( name );
}

/* Handles *?{}[] wildcards */
static int WildMatch(char *pattern, char *name,int ignorecase) {
    char *eop = pattern + strlen(pattern);

    if ( pattern==NULL )
return( true );

    name = SubMatch(pattern,eop,name,ignorecase);
    if ( name==NULL )
return( false );
    if ( *name=='\0' )
return( true );

return( false );
}

static int SS_ScriptChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype != et_textfocuschanged ) {
	char *txt = GGadgetGetTitle8(g);
	char buf[8];
	int i;
	extern GTextInfo scripts[];

	for ( i=0; scripts[i].text!=NULL; ++i ) {
	    if ( strcmp((char *) scripts[i].text,txt)==0 )
	break;
	}
	free(txt);
	if ( scripts[i].text==NULL )
return( true );
	buf[0] = ((intpt) scripts[i].userdata)>>24;
	buf[1] = ((intpt) scripts[i].userdata)>>16;
	buf[2] = ((intpt) scripts[i].userdata)>>8 ;
	buf[3] = ((intpt) scripts[i].userdata)    ;
	buf[4] = 0;
	GGadgetSetTitle8(g,buf);
    }
return( true );
}

static int SS_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	int *done = GDrawGetUserData(GGadgetGetWindow(g));
	*done = 2;
    }
return( true );
}

static int SS_Cancel(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	int *done = GDrawGetUserData(GGadgetGetWindow(g));
	*done = true;
    }
return( true );
}

static int ss_e_h(GWindow gw, GEvent *event) {
    int *done = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_char:
return( false );
      case et_close:
	*done = true;
      break;
    }
return( true );
}

static void FVSelectByScript(FontView *fv) {
    int j, gid;
    SplineChar *sc;
    EncMap *map = fv->map;
    SplineFont *sf = fv->sf;
    extern GTextInfo scripts[];
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[4], *hvarray[5][2], *barray[8], boxes[3];
    GTextInfo label[4];
    int i,k;
    int done = 0, merge;
    char tagbuf[4];
    uint32 tag;
    const unichar_t *ret;

    LookupUIInit();

    memset(&wattrs,0,sizeof(wattrs));
    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));
    memset(&boxes,0,sizeof(boxes));

    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = false;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Select by Script");
    wattrs.is_dlg = false;
    pos.x = pos.y = 0;
    pos.width = 100;
    pos.height = 100;
    gw = GDrawCreateTopWindow(NULL,&pos,ss_e_h,&done,&wattrs);

    k = i = 0;

    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.u.list = scripts;
    gcd[k].gd.handle_controlevent = SS_ScriptChanged;
    gcd[k++].creator = GListFieldCreate;
    hvarray[i][0] = &gcd[k-1]; hvarray[i++][1] = NULL;

    label[k].text = (unichar_t *) _("Merge into current selection");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray[i][0] = &gcd[k-1]; hvarray[i++][1] = NULL;

    hvarray[i][0] = GCD_Glue; hvarray[i++][1] = NULL;

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_default;
    gcd[k].gd.handle_controlevent = SS_OK;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_cancel;
    gcd[k].gd.handle_controlevent = SS_Cancel;
    gcd[k++].creator = GButtonCreate;

    barray[0] = barray[2] = barray[3] = barray[4] = barray[6] = GCD_Glue; barray[7] = NULL;
    barray[1] = &gcd[k-2]; barray[5] = &gcd[k-1];
    hvarray[i][0] = &boxes[2]; hvarray[i++][1] = NULL;
    hvarray[i][0] = NULL;

    memset(boxes,0,sizeof(boxes));
    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = hvarray[0];
    boxes[0].creator = GHVGroupCreate;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = barray;
    boxes[2].creator = GHBoxCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    

    GHVBoxFitWindow(boxes[0].ret);

    GDrawSetVisible(gw,true);
    ret = NULL;
    while ( !done ) {
	GDrawProcessOneEvent(NULL);
	if ( done==2 ) {
	    ret = _GGadgetGetTitle(gcd[0].ret);
	    if ( *ret=='\0' ) {
		gwwv_post_error(_("No Script"),_("Please specify a script"));
		done = 0;
	    } else if ( u_strlen(ret)>4 ) {
		gwwv_post_error(_("Bad Script"),_("Scripts are 4 letter tags"));
		done = 0;
	    }
	}
    }
    memset(tagbuf,' ',4);
    if ( done==2 && ret!=NULL ) {
	tagbuf[0] = *ret;
	if ( ret[1]!='\0' ) {
	    tagbuf[1] = ret[1];
	    if ( ret[2]!='\0' ) {
		tagbuf[2] = ret[2];
		if ( ret[3]!='\0' )
		    tagbuf[3] = ret[3];
	    }
	}
    }
    merge = GGadgetIsChecked(gcd[1].ret);

    GDrawDestroyWindow(gw);
    if ( done==1 )
return;
    tag = (tagbuf[0]<<24) | (tagbuf[1]<<16) | (tagbuf[2]<<8) | tagbuf[3];

    if ( !merge ) {
	FVDeselectAll(fv);
	fv->sel_index = 0;
    }
	
    for ( j=0; j<map->enccount; ++j ) if ( (gid=map->map[j])!=-1 && (sc=sf->glyphs[gid])!=NULL ) {
	if ( SCScriptFromUnicode(sc)==tag ) {
	    fv->selected[j] = fv->sel_index+1;
	    FVToggleCharSelected(fv,j);
	}
    }

    if ( fv->sel_index<254 )
	++fv->sel_index;
}

static void FVSelectByName(FontView *fv) {
    int j, gid;
    char *ret, *end;
    SplineChar *sc;
    EncMap *map = fv->map;
    SplineFont *sf = fv->sf;
    struct altuni *alt;

    ret = gwwv_ask_string(_("Select all instances of the wildcard pattern"),".notdef",_("Select all instances of the wildcard pattern"));
    if ( ret==NULL )
return;
    FVDeselectAll(fv);
    if (( *ret=='0' && ( ret[1]=='x' || ret[1]=='X' )) ||
	    ((*ret=='u' || *ret=='U') && ret[1]=='+' )) {
	int uni = strtol(ret+2,&end,16);
	if ( *end!='\0' || uni<0 || uni>=0x110000 ) {
	    free(ret);
	    gwwv_post_error( _("Bad Number"),_("Bad Number") );
return;
	}
	for ( j=0; j<map->enccount; ++j ) if ( (gid=map->map[j])!=-1 && (sc=sf->glyphs[gid])!=NULL ) {
	    for ( alt=sc->altuni; alt!=NULL && alt->unienc!=uni; alt=alt->next );
	    if ( sc->unicodeenc == uni || alt!=NULL ) {
		fv->selected[j] = true;
		FVToggleCharSelected(fv,j);
	    }
	}
    } else {
	for ( j=0; j<map->enccount; ++j ) if ( (gid=map->map[j])!=-1 && (sc=sf->glyphs[gid])!=NULL ) {
	    if ( WildMatch(ret,sc->name,false) ) {
		fv->selected[j] = true;
		FVToggleCharSelected(fv,j);
	    }
	}
    }
    free(ret);
    fv->sel_index = 1;
}

static void FVSelectColor(FontView *fv, uint32 col, int door) {
    int i, any=0;
    uint32 sccol;
    SplineChar **glyphs = fv->sf->glyphs;

    for ( i=0; i<fv->map->enccount; ++i ) {
	int gid = fv->map->map[i];
	sccol =  ( gid==-1 || glyphs[gid]==NULL ) ? COLOR_DEFAULT : glyphs[gid]->color;
	if ( (door && !fv->selected[i] && sccol==col) ||
		(!door && fv->selected[i]!=(sccol==col)) ) {
	    fv->selected[i] = !fv->selected[i];
	    if ( fv->selected[i] ) any = true;
	    FVToggleCharSelected(fv,i);
	}
    }
    fv->sel_index = any;
}

static void FVReselect(FontView *fv, int newpos) {
    int i;

    if ( newpos<0 ) newpos = 0;
    else if ( newpos>=fv->map->enccount ) newpos = fv->map->enccount-1;

    if ( fv->pressed_pos<fv->end_pos ) {
	if ( newpos>fv->end_pos ) {
	    for ( i=fv->end_pos+1; i<=newpos; ++i ) if ( !fv->selected[i] ) {
		fv->selected[i] = fv->sel_index;
		FVToggleCharSelected(fv,i);
	    }
	} else if ( newpos<fv->pressed_pos ) {
	    for ( i=fv->end_pos; i>fv->pressed_pos; --i ) if ( fv->selected[i] ) {
		fv->selected[i] = false;
		FVToggleCharSelected(fv,i);
	    }
	    for ( i=fv->pressed_pos-1; i>=newpos; --i ) if ( !fv->selected[i] ) {
		fv->selected[i] = fv->sel_index;
		FVToggleCharSelected(fv,i);
	    }
	} else {
	    for ( i=fv->end_pos; i>newpos; --i ) if ( fv->selected[i] ) {
		fv->selected[i] = false;
		FVToggleCharSelected(fv,i);
	    }
	}
    } else {
	if ( newpos<fv->end_pos ) {
	    for ( i=fv->end_pos-1; i>=newpos; --i ) if ( !fv->selected[i] ) {
		fv->selected[i] = fv->sel_index;
		FVToggleCharSelected(fv,i);
	    }
	} else if ( newpos>fv->pressed_pos ) {
	    for ( i=fv->end_pos; i<fv->pressed_pos; ++i ) if ( fv->selected[i] ) {
		fv->selected[i] = false;
		FVToggleCharSelected(fv,i);
	    }
	    for ( i=fv->pressed_pos+1; i<=newpos; ++i ) if ( !fv->selected[i] ) {
		fv->selected[i] = fv->sel_index;
		FVToggleCharSelected(fv,i);
	    }
	} else {
	    for ( i=fv->end_pos; i<newpos; ++i ) if ( fv->selected[i] ) {
		fv->selected[i] = false;
		FVToggleCharSelected(fv,i);
	    }
	}
    }
    fv->end_pos = newpos;
# ifdef FONTFORGE_CONFIG_GDRAW
    if ( newpos>=0 && newpos<fv->map->enccount && (i = fv->map->map[newpos])!=-1 &&
	    fv->sf->glyphs[i]!=NULL &&
	    fv->sf->glyphs[i]->unicodeenc>=0 && fv->sf->glyphs[i]->unicodeenc<0x10000 )
	GInsCharSetChar(fv->sf->glyphs[i]->unicodeenc);
# endif
}
#endif

static void _SplineFontSetUnChanged(SplineFont *sf) {
    int i;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    int was = sf->changed;
    FontView *fvs;
#endif
    BDFFont *bdf;

    sf->changed = false;
    SFClearAutoSave(sf);
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	if ( sf->glyphs[i]->changed ) {
	    sf->glyphs[i]->changed = false;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	    SCRefreshTitles(sf->glyphs[i]);
#endif
	}
    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	for ( i=0; i<bdf->glyphcnt; ++i ) if ( bdf->glyphs[i]!=NULL )
	    bdf->glyphs[i]->changed = false;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( was && sf->fv!=NULL && sf->fv->v!=NULL )
# ifdef FONTFORGE_CONFIG_GDRAW
	GDrawRequestExpose(sf->fv->v,NULL,false);
# elif defined(FONTFORGE_CONFIG_GTK)
	gtk_widget_queue_draw(sf->fv->v);
# endif
    if ( was )
	for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->next )
	    FVSetTitle(fvs);
#endif
    for ( i=0; i<sf->subfontcnt; ++i )
	_SplineFontSetUnChanged(sf->subfonts[i]);
}

void SplineFontSetUnChanged(SplineFont *sf) {
    int i;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
    if ( sf->mm!=NULL ) sf = sf->mm->normal;
    _SplineFontSetUnChanged(sf);
    if ( sf->mm!=NULL )
	for ( i=0; i<sf->mm->instance_count; ++i )
	    _SplineFontSetUnChanged(sf->mm->instances[i]);
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void FVFlattenAllBitmapSelections(FontView *fv) {
    BDFFont *bdf;
    int i;

    for ( bdf = fv->sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	for ( i=0; i<bdf->glyphcnt; ++i )
	    if ( bdf->glyphs[i]!=NULL && bdf->glyphs[i]->selection!=NULL )
		BCFlattenFloat(bdf->glyphs[i]);
    }
}

static int AskChanged(SplineFont *sf) {
    int ret;
    char *buts[4];
    char *filename, *fontname;

    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;

    filename = sf->filename;
    fontname = sf->fontname;

    if ( filename==NULL && sf->origname!=NULL &&
	    sf->onlybitmaps && sf->bitmaps!=NULL && sf->bitmaps->next==NULL )
	filename = sf->origname;
    if ( filename==NULL ) filename = "untitled.sfd";
    filename = GFileNameTail(filename);
#if defined(FONTFORGE_CONFIG_GDRAW)
    buts[0] = _("_Save");
    buts[2] = _("_Cancel");
#elif defined(FONTFORGE_CONFIG_GTK)
    buts[0] = GTK_STOCK_SAVE;
    buts[2] = GTK_STOCK_CANCEL;
#endif
    buts[1] = _("_Don't Save");
    buts[3] = NULL;
    ret = gwwv_ask( _("Font changed"),(const char **) buts,0,2,_("Font %1$.40s in file %2$.40s has been changed.\nDo you want to save it?"),fontname,filename);
return( ret );
}

static int RevertAskChanged(char *fontname,char *filename) {
    int ret;
    char *buts[3];

    if ( filename==NULL ) filename = "untitled.sfd";
    filename = GFileNameTail(filename);
#if defined(FONTFORGE_CONFIG_GDRAW)
    buts[0] = _("_Revert");
    buts[1] = _("_Cancel");
#elif defined(FONTFORGE_CONFIG_GTK)
    buts[0] = GTK_STOCK_REVERT_TO_SAVED;
    buts[1] = GTK_STOCK_CANCEL;
#endif
    buts[2] = NULL;
    ret = gwwv_ask( _("Font changed"),(const char **) buts,0,1,_("Font %1$.40s in file %2$.40s has been changed.\nReverting the file will lose those changes.\nIs that what you want?"),fontname,filename);
return( ret==0 );
}
#endif

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
int _FVMenuGenerate(FontView *fv,int family) {
    FVFlattenAllBitmapSelections(fv);
return( SFGenerateFont(fv->sf,family,fv->normal==NULL?fv->map:fv->normal) );
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuGenerate(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    _FVMenuGenerate(fv,false);
}

static void FVMenuGenerateFamily(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    _FVMenuGenerate(fv,true);
}
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Generate(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    _FVMenuGenerate(fv,false);
}

void FontViewMenu_GenerateFamily(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    _FVMenuGenerate(fv,true);
}
# endif

extern int save_to_dir;

static int SaveAs_FormatChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GGadget *fc = GWidgetGetControl(GGadgetGetWindow(g),1000);
	char *oldname = GGadgetGetTitle8(fc);
	int *_s2d = GGadgetGetUserData(g);
	int s2d = GGadgetIsChecked(g);
	char *pt, *newname = galloc(strlen(oldname)+8);
#ifdef VMS
	char *pt2;

	strcpy(newname,oldname);
	pt = strrchr(newname,'.');
	pt2 = strrchr(newname,'_');
	if ( pt==NULL )
	    pt = pt2;
	else if ( pt2!=NULL && pt<pt2 )
	    pt = pt2;
	if ( pt==NULL )
	    pt = newname+strlen(newname);
	strcpy(pt,s2d ? "_sfdir" : ".sfd" );
#else
	strcpy(newname,oldname);
	pt = strrchr(newname,'.');
	if ( pt==NULL )
	    pt = newname+strlen(newname);
	strcpy(pt,s2d ? ".sfdir" : ".sfd" );
#endif
	GGadgetSetTitle8(fc,newname);
	save_to_dir = *_s2d = s2d;
	SavePrefs();
    }
return( true );
}

int _FVMenuSaveAs(FontView *fv) {
    char *temp;
    char *ret;
    char *filename;
    int ok;
    int s2d = fv->cidmaster!=NULL ? fv->cidmaster->save_to_dir :
		fv->sf->mm!=NULL ? fv->sf->mm->normal->save_to_dir :
		fv->sf->save_to_dir;
    GGadgetCreateData gcd;
    GTextInfo label;

# ifdef FONTFORGE_CONFIG_GDRAW
    if ( fv->cidmaster!=NULL && fv->cidmaster->filename!=NULL )
	temp=def2utf8_copy(fv->cidmaster->filename);
    else if ( fv->sf->mm!=NULL && fv->sf->mm->normal->filename!=NULL )
	temp=def2utf8_copy(fv->sf->mm->normal->filename);
    else if ( fv->sf->filename!=NULL )
	temp=def2utf8_copy(fv->sf->filename);
# elif defined(FONTFORGE_CONFIG_GTK)
    gsize read, written;

    if ( fv->cidmaster!=NULL && fv->cidmaster->filename!=NULL )
	temp=g_filename_to_utf8(fv->cidmaster->filename,-1,&read,&written,NULL);
    else if ( fv->sf->mm!=NULL && fv->sf->mm->normal->filename!=NULL )
	temp=g_filename_to_utf8(fv->sf->mm->normal->filename,-1,&read,&written,NULL);
    else if ( fv->sf->filename!=NULL )
	temp=g_filename_to_utf8(fv->sf->filename,-1,&read,&written,NULL);
#endif
    else {
	SplineFont *sf = fv->cidmaster?fv->cidmaster:
		fv->sf->mm!=NULL?fv->sf->mm->normal:fv->sf;
	char *fn = sf->defbasefilename ? sf->defbasefilename : sf->fontname;
	temp = galloc((strlen(fn)+10));
	strcpy(temp,fn);
	if ( sf->defbasefilename!=NULL )
	    /* Don't add a default suffix, they've already told us what name to use */;
	else if ( fv->cidmaster!=NULL )
	    strcat(temp,"CID");
	else if ( sf->mm==NULL )
	    ;
	else if ( sf->mm->apple )
	    strcat(temp,"Var");
	else
	    strcat(temp,"MM");
#ifdef VMS
	strcat(temp,save_to_dir ? "_sfdir" : ".sfd");
#else
	strcat(temp,save_to_dir ? ".sfdir" : ".sfd");
#endif
	s2d = save_to_dir;
    }

    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));
    gcd.gd.flags = s2d ? (gg_visible | gg_enabled | gg_cb_on) : (gg_visible | gg_enabled);
    label.text = (unichar_t *) _("Save as _Directory");
    label.text_is_1byte = true;
    label.text_in_resource = true;
    gcd.gd.label = &label;
    gcd.gd.handle_controlevent = SaveAs_FormatChange;
    gcd.data = &s2d;
    gcd.creator = GCheckBoxCreate;

    ret = gwwv_save_filename_with_gadget(_("Save as..."),temp,NULL,&gcd);
    free(temp);
    if ( ret==NULL )
return( 0 );
# ifdef FONTFORGE_CONFIG_GDRAW
    filename = utf82def_copy(ret);
# elif defined(FONTFORGE_CONFIG_GTK)
    filename = g_filename_from_utf8(ret,-1,&read,&written,NULL);
#endif
    free(ret);
    FVFlattenAllBitmapSelections(fv);
    fv->sf->compression = 0;
    ok = SFDWrite(filename,fv->sf,fv->map,fv->normal,s2d);
    if ( ok ) {
	SplineFont *sf = fv->cidmaster?fv->cidmaster:fv->sf->mm!=NULL?fv->sf->mm->normal:fv->sf;
	free(sf->filename);
	sf->filename = filename;
	sf->save_to_dir = s2d;
	free(sf->origname);
	sf->origname = copy(filename);
	sf->new = false;
	if ( sf->mm!=NULL ) {
	    int i;
	    for ( i=0; i<sf->mm->instance_count; ++i ) {
		free(sf->mm->instances[i]->filename);
		sf->mm->instances[i]->filename = filename;
		free(sf->mm->instances[i]->origname);
		sf->mm->instances[i]->origname = copy(filename);
		sf->mm->instances[i]->new = false;
	    }
	}
	SplineFontSetUnChanged(sf);
	FVSetTitle(fv);
    } else
	free(filename);
return( ok );
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuSaveAs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    _FVMenuSaveAs(fv);
}
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_SaveAs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    _FVMenuSaveAs(fv);
}
# endif

int _FVMenuSave(FontView *fv) {
    int ret = 0;
    SplineFont *sf = fv->cidmaster?fv->cidmaster:
		    fv->sf->mm!=NULL?fv->sf->mm->normal:
			    fv->sf;

#if 0		/* This seems inconsistant with normal behavior. Removed 6 Feb '04 */
    if ( sf->filename==NULL && sf->origname!=NULL &&
	    sf->onlybitmaps && sf->bitmaps!=NULL && sf->bitmaps->next==NULL ) {
	/* If it's a single bitmap font then just save back to the bdf file */
	FVFlattenAllBitmapSelections(fv);
	ret = BDFFontDump(sf->origname,sf->bitmaps,EncodingName(sf->encoding_name),sf->bitmaps->res);
	if ( ret )
	    SplineFontSetUnChanged(sf);
    } else
#endif
    if ( sf->filename==NULL )
	ret = _FVMenuSaveAs(fv);
    else {
	FVFlattenAllBitmapSelections(fv);
	if ( !SFDWriteBak(sf,fv->map,fv->normal) )
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	    gwwv_post_error(_("Save Failed"),_("Save Failed"));
#endif
	else {
	    SplineFontSetUnChanged(sf);
	    ret = true;
	}
    }
return( ret );
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuSave(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    _FVMenuSave(fv);
}
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Save(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    _FVMenuSave(fv);
}
# endif
#endif

void _FVCloseWindows(FontView *fv) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    int i, j;
    BDFFont *bdf;
    MetricsView *mv, *mnext;
    SplineFont *sf = fv->cidmaster?fv->cidmaster:fv->sf->mm!=NULL?fv->sf->mm->normal : fv->sf;

    PrintWindowClose();
    if ( fv->nextsame==NULL && fv->sf->fv==fv && fv->sf->kcld!=NULL )
	KCLD_End(fv->sf->kcld);
    if ( fv->nextsame==NULL && fv->sf->fv==fv && fv->sf->vkcld!=NULL )
	KCLD_End(fv->sf->vkcld);

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	CharView *cv, *next;
	for ( cv = sf->glyphs[i]->views; cv!=NULL; cv = next ) {
	    next = cv->next;
# ifdef FONTFORGE_CONFIG_GDRAW
	    GDrawDestroyWindow(cv->gw);
# elif defined(FONTFORGE_CONFIG_GTK)
	    gtk_widget_destroy(cv->gw);
# endif
	}
	if ( sf->glyphs[i]->charinfo )
	    CharInfoDestroy(sf->glyphs[i]->charinfo);
    }
    if ( sf->mm!=NULL ) {
	MMSet *mm = sf->mm;
	for ( j=0; j<mm->instance_count; ++j ) {
	    SplineFont *sf = mm->instances[j];
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
		CharView *cv, *next;
		for ( cv = sf->glyphs[i]->views; cv!=NULL; cv = next ) {
		    next = cv->next;
# ifdef FONTFORGE_CONFIG_GDRAW
		    GDrawDestroyWindow(cv->gw);
# elif defined(FONTFORGE_CONFIG_GTK)
		    gtk_widget_destroy(cv->gw);
# endif
		}
		if ( sf->glyphs[i]->charinfo )
		    CharInfoDestroy(sf->glyphs[i]->charinfo);
	    }
	    for ( mv=sf->metrics; mv!=NULL; mv = mnext ) {
		mnext = mv->next;
# ifdef FONTFORGE_CONFIG_GDRAW
		GDrawDestroyWindow(mv->gw);
# elif defined(FONTFORGE_CONFIG_GTK)
		gtk_widget_destroy(mv->gw);
# endif
	    }
	}
    } else if ( sf->subfontcnt!=0 ) {
	for ( j=0; j<sf->subfontcnt; ++j ) {
	    for ( i=0; i<sf->subfonts[j]->glyphcnt; ++i ) if ( sf->subfonts[j]->glyphs[i]!=NULL ) {
		CharView *cv, *next;
		for ( cv = sf->subfonts[j]->glyphs[i]->views; cv!=NULL; cv = next ) {
		    next = cv->next;
# ifdef FONTFORGE_CONFIG_GDRAW
		    GDrawDestroyWindow(cv->gw);
# elif defined(FONTFORGE_CONFIG_GTK)
		    gtk_widget_destroy(cv->gw);
# endif
		if ( sf->subfonts[j]->glyphs[i]->charinfo )
		    CharInfoDestroy(sf->subfonts[j]->glyphs[i]->charinfo);
		}
	    }
	    for ( mv=sf->subfonts[j]->metrics; mv!=NULL; mv = mnext ) {
		mnext = mv->next;
# ifdef FONTFORGE_CONFIG_GDRAW
		GDrawDestroyWindow(mv->gw);
# elif defined(FONTFORGE_CONFIG_GTK)
		gtk_widget_destroy(mv->gw);
# endif
	    }
	}
    } else {
	for ( mv=sf->metrics; mv!=NULL; mv = mnext ) {
	    mnext = mv->next;
# ifdef FONTFORGE_CONFIG_GDRAW
	    GDrawDestroyWindow(mv->gw);
# elif defined(FONTFORGE_CONFIG_GTK)
	    gtk_widget_destroy(mv->gw);
# endif
	}
    }
    for ( bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	for ( i=0; i<bdf->glyphcnt; ++i ) if ( bdf->glyphs[i]!=NULL ) {
	    BitmapView *bv, *next;
	    for ( bv = bdf->glyphs[i]->views; bv!=NULL; bv = next ) {
		next = bv->next;
# ifdef FONTFORGE_CONFIG_GDRAW
		GDrawDestroyWindow(bv->gw);
# elif defined(FONTFORGE_CONFIG_GTK)
		gtk_widget_destroy(bv->gw);
# endif
	    }
	}
    }
    if ( fv->sf->fontinfo!=NULL )
	FontInfoDestroy(fv->sf);
    if ( fv->sf->valwin!=NULL )
	ValidationDestroy(fv->sf);
    SVDetachFV(fv);
#endif
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static int SFAnyChanged(SplineFont *sf) {
    if ( sf->mm!=NULL ) {
	MMSet *mm = sf->mm;
	int i;
	if ( mm->changed )
return( true );
	for ( i=0; i<mm->instance_count; ++i )
	    if ( sf->mm->instances[i]->changed )
return( true );
	/* Changes to the blended font aren't real (for adobe fonts) */
	if ( mm->apple && mm->normal->changed )
return( true );

return( false );
    } else
return( sf->changed );
}

static int _FVMenuClose(FontView *fv) {
    int i;
    SplineFont *sf = fv->cidmaster?fv->cidmaster:fv->sf;

    if ( !SFCloseAllInstrs(fv->sf) )
return( false );

    if ( fv->nextsame!=NULL || fv->sf->fv!=fv ) {
	/* There's another view, can close this one with no problems */
    } else if ( SFAnyChanged(sf) ) {
	i = AskChanged(fv->sf);
	if ( i==2 )	/* Cancel */
return( false );
	if ( i==0 && !_FVMenuSave(fv))		/* Save */
return(false);
	else
	    SFClearAutoSave(sf);		/* if they didn't save it, remove change record */
    }
    _FVCloseWindows(fv);
    if ( sf->filename!=NULL )
	RecentFilesRemember(sf->filename);
#ifdef FONTFORGE_CONFIG_GDRAW
    GDrawDestroyWindow(fv->gw);
#elif defined(FONTFORGE_CONFIG_GTK)
    gtk_widget_destroy(fv->gw);
#endif
return( true );
}

# ifdef FONTFORGE_CONFIG_GDRAW
void MenuNew(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontNew();
}

static void FVMenuClose(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    /*DelayEvent((void (*)(void *)) _FVMenuClose, fv);*/
    _FVMenuClose(fv);
}
# elif defined(FONTFORGE_CONFIG_GTK)
void Menu_New(GtkMenuItem *menuitem, gpointer user_data) {
    FontNew();
}

void FontViewMenu_Close(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    _FVMenuClose(fv);
}

gboolean FontView_RequestClose(GtkWidget *widget, GdkEvent *event,
	gpointer user_data) {
    FontView *fv = (FontView *) g_object_get_data(G_OBJECT(widget),"data");

    _FVMenuClose(fv);
}
# endif

static void FVReattachCVs(SplineFont *old,SplineFont *new) {
    int i, j, pos;
    CharView *cv, *cvnext;
    SplineFont *sub;

    for ( i=0; i<old->glyphcnt; ++i ) {
	if ( old->glyphs[i]!=NULL && old->glyphs[i]->views!=NULL ) {
	    if ( new->subfontcnt==0 ) {
		pos = SFFindExistingSlot(new,old->glyphs[i]->unicodeenc,old->glyphs[i]->name);
		sub = new;
	    } else {
		pos = -1;
		for ( j=0; j<new->subfontcnt && pos==-1 ; ++j ) {
		    sub = new->subfonts[j];
		    pos = SFFindExistingSlot(sub,old->glyphs[i]->unicodeenc,old->glyphs[i]->name);
		}
	    }
	    if ( pos==-1 ) {
		for ( cv=old->glyphs[i]->views; cv!=NULL; cv = cvnext ) {
		    cvnext = cv->next;
# ifdef FONTFORGE_CONFIG_GDRAW
		    GDrawDestroyWindow(cv->gw);
# elif defined(FONTFORGE_CONFIG_GTK)
		    gtk_widget_destroy(cv->gw);
# endif
		}
	    } else {
		for ( cv=old->glyphs[i]->views; cv!=NULL; cv = cvnext ) {
		    cvnext = cv->next;
		    CVChangeSC(cv,sub->glyphs[pos]);
		    cv->layerheads[dm_grid] = &new->grid;
		}
	    }
# ifdef FONTFORGE_CONFIG_GDRAW
	    GDrawProcessPendingEvents(NULL);		/* Don't want to many destroy_notify events clogging up the queue */
# endif
	}
    }
}

static char *Decompress(char *name, int compression) {
    char *dir = getenv("TMPDIR");
    char buf[1500];
    char *tmpfile;

    if ( dir==NULL ) dir = P_tmpdir;
    tmpfile = galloc(strlen(dir)+strlen(GFileNameTail(name))+2);
    strcpy(tmpfile,dir);
    strcat(tmpfile,"/");
    strcat(tmpfile,GFileNameTail(name));
    *strrchr(tmpfile,'.') = '\0';
#if defined( _NO_SNPRINTF ) || defined( __VMS )
    sprintf( buf, "%s < %s > %s", compressors[compression].decomp, name, tmpfile );
#else
    snprintf( buf, sizeof(buf), "%s < %s > %s", compressors[compression].decomp, name, tmpfile );
#endif
    if ( system(buf)==0 )
return( tmpfile );
    free(tmpfile);
return( NULL );
}

static void _FVRevert(FontView *fv,int tobackup) {
    SplineFont *temp, *old = fv->cidmaster?fv->cidmaster:fv->sf;
    BDFFont *tbdf, *fbdf;
    BDFChar *bc;
    BitmapView *bv, *bvnext;
    MetricsView *mv, *mvnext;
    int i;
    FontView *fvs;
    EncMap *map;

    if ( old->origname==NULL )
return;
    if ( old->changed )
	if ( !RevertAskChanged(old->fontname,old->origname))
return;
    if ( tobackup ) {
	/* we can only revert to backup if it's an sfd file. So we use filename*/
	/*  here. In the normal case we revert to whatever file we read it from*/
	/*  (sfd or not) so we use origname */
	char *buf = galloc(strlen(old->filename)+20);
	strcpy(buf,old->filename);
	if ( old->compression!=0 ) {
	    char *tmpfile;
	    strcat(buf,compressors[old->compression-1].ext);
	    strcat(buf,"~");
	    tmpfile = Decompress(buf,old->compression-1);
	    if ( tmpfile==NULL )
		temp = NULL;
	    else {
		temp = ReadSplineFont(tmpfile,0);
		unlink(tmpfile);
		free(tmpfile);
	    }
	} else {
	    strcat(buf,"~");
	    temp = ReadSplineFont(buf,0);
	}
	free(buf);
    } else {
	if ( old->compression!=0 ) {
	    char *tmpfile;
	    char *buf = galloc(strlen(old->filename)+20);
	    strcpy(buf,old->filename);
	    strcat(buf,compressors[old->compression-1].ext);
	    tmpfile = Decompress(buf,old->compression-1);
	    if ( tmpfile==NULL )
		temp = NULL;
	    else {
		temp = ReadSplineFont(tmpfile,0);
		unlink(tmpfile);
		free(tmpfile);
	    }
	} else
	    temp = ReadSplineFont(old->origname,0);
    }
    if ( temp==NULL ) {
return;
    }
    if ( temp->filename!=NULL ) {
	free(temp->filename);
	temp->filename = copy(old->filename);
    }
    if ( temp->origname!=NULL ) {
	free(temp->origname);
	temp->origname = copy(old->origname);
    }
    temp->compression = old->compression;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    FVReattachCVs(old,temp);
    for ( i=0; i<old->subfontcnt; ++i )
	FVReattachCVs(old->subfonts[i],temp);
    for ( fbdf=old->bitmaps; fbdf!=NULL; fbdf=fbdf->next ) {
	for ( tbdf=temp->bitmaps; tbdf!=NULL; tbdf=tbdf->next )
	    if ( tbdf->pixelsize==fbdf->pixelsize )
	break;
	for ( i=0; i<fv->sf->glyphcnt; ++i ) {
	    if ( i<fbdf->glyphcnt && fbdf->glyphs[i]!=NULL && fbdf->glyphs[i]->views!=NULL ) {
		int pos = SFFindExistingSlot(temp,fv->sf->glyphs[i]->unicodeenc,fv->sf->glyphs[i]->name);
		bc = pos==-1 || tbdf==NULL?NULL:tbdf->glyphs[pos];
		if ( bc==NULL ) {
		    for ( bv=fbdf->glyphs[i]->views; bv!=NULL; bv = bvnext ) {
			bvnext = bv->next;
# ifdef FONTFORGE_CONFIG_GDRAW
			GDrawDestroyWindow(bv->gw);
# elif defined(FONTFORGE_CONFIG_GTK)
			gtk_widget_destroy(bv->gw);
# endif
		    }
		} else {
		    for ( bv=fbdf->glyphs[i]->views; bv!=NULL; bv = bvnext ) {
			bvnext = bv->next;
			BVChangeBC(bv,bc,true);
		    }
		}
# ifdef FONTFORGE_CONFIG_GDRAW
		GDrawProcessPendingEvents(NULL);		/* Don't want to many destroy_notify events clogging up the queue */
# endif
	    }
	}
    }
# ifdef FONTFORGE_CONFIG_GDRAW
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
# endif
    if ( fv->sf->fontinfo )
	FontInfoDestroy(fv->sf);
    for ( fvs=fv->sf->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	for ( mv=fvs->sf->metrics; mv!=NULL; mv = mvnext ) {
	    /* Don't bother trying to fix up metrics views, just not worth it */
	    mvnext = mv->next;
# ifdef FONTFORGE_CONFIG_GDRAW
	    GDrawDestroyWindow(mv->gw);
# elif defined(FONTFORGE_CONFIG_GTK)
	    gtk_widget_destroy(mv->gw);
# endif
	}
	if ( fvs==fv )
	    map = temp->map;
	else
	    map = EncMapFromEncoding(fv->sf,fv->map->enc);
	if ( map->enccount>fvs->map->enccount ) {
	    fvs->selected = grealloc(fvs->selected,map->enccount);
	    memset(fvs->selected+fvs->map->enccount,0,map->enccount-fvs->map->enccount);
	}
	EncMapFree(fv->map);
	fv->map = map;
	if ( fvs->normal!=NULL ) {
	    EncMapFree(fvs->normal);
	    fvs->normal = EncMapCopy(fvs->map);
	    CompactEncMap(fvs->map,fv->sf);
	}
    }
# ifdef FONTFORGE_CONFIG_GDRAW
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
# endif
#endif
    SFClearAutoSave(old);
    temp->fv = fv->sf->fv;
    for ( fvs=fv->sf->fv; fvs!=NULL; fvs=fvs->nextsame )
	fvs->sf = temp;
    FontViewReformatAll(fv->sf);
    SplineFontFree(old);
}

void FVRevert(FontView *fv) {
    _FVRevert(fv,false);
}

void FVRevertBackup(FontView *fv) {
    _FVRevert(fv,true);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuRevert(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_RevertFile(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVRevert(fv);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuRevertBackup(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_RevertBackup(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVRevertBackup(fv);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuRevertGlyph(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_RevertGlyph(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    int i, gid;
    int nc_state = -1;
    SplineFont *sf = fv->sf;
    SplineChar *sc, *tsc;
    SplineChar temp;
#ifdef FONTFORGE_CONFIG_TYPE3
    Undoes **undoes;
    int layer, lc;
#endif
    EncMap *map = fv->map;
    CharView *cvs;

    if ( fv->sf->sfd_version<2 )
	gwwv_post_error(_("Old sfd file"),_("This font comes from an old format sfd file. Not all aspects of it can be reverted successfully."));

    for ( i=0; i<map->enccount; ++i ) if ( fv->selected[i] && (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL ) {
	tsc = sf->glyphs[gid];
	if ( tsc->namechanged ) {
	    if ( nc_state==-1 ) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
		gwwv_post_error(_("Glyph Name Changed"),_("The name of glyph %.40s has changed. This is what I use to find the glyph in the file, so I cannot revert this glyph.\n(You will not be warned for subsequent glyphs.)"),tsc->name);
#endif
		nc_state = 0;
	    }
	} else {
	    sc = SFDReadOneChar(sf,tsc->name);
	    if ( sc==NULL ) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
		gwwv_post_error(_("Can't Find Glyph"),_("The glyph, %.80s, can't be found in the sfd file"),tsc->name);
#endif
		tsc->namechanged = true;
	    } else {
		SCPreserveState(tsc,true);
		SCPreserveBackground(tsc);
		temp = *tsc;
		tsc->dependents = NULL;
#ifdef FONTFORGE_CONFIG_TYPE3
		lc = tsc->layer_cnt;
		undoes = galloc(lc*sizeof(Undoes *));
		for ( layer=0; layer<lc; ++layer ) {
		    undoes[layer] = tsc->layers[layer].undoes;
		    tsc->layers[layer].undoes = NULL;
		}
#else
		tsc->layers[ly_back].undoes = tsc->layers[ly_fore].undoes = NULL;
#endif
		SplineCharFreeContents(tsc);
		*tsc = *sc;
		chunkfree(sc,sizeof(SplineChar));
		tsc->parent = sf;
		tsc->dependents = temp.dependents;
		tsc->views = temp.views;
#ifdef FONTFORGE_CONFIG_TYPE3
		for ( layer = 0; layer<lc && layer<tsc->layer_cnt; ++layer )
		    tsc->layers[layer].undoes = undoes[layer];
		for ( ; layer<lc; ++layer )
		    UndoesFree(undoes[layer]);
		free(undoes);
#else
		tsc->layers[ly_fore].undoes = temp.layers[ly_fore].undoes;
		tsc->layers[ly_back].undoes = temp.layers[ly_back].undoes;
#endif
		/* tsc->changed = temp.changed; */
		/* tsc->orig_pos = temp.orig_pos; */
		for ( cvs=tsc->views; cvs!=NULL; cvs=cvs->next ) {
		    cvs->layerheads[dm_back] = &tsc->layers[ly_back];
		    cvs->layerheads[dm_fore] = &tsc->layers[ly_fore];
		}
		RevertedGlyphReferenceFixup(tsc, sf);
		_SCCharChangedUpdate(tsc,false);
	    }
	}
    }
}

# if defined(FONTFORGE_CONFIG_GDRAW)
void MenuPrefs(GWindow base,struct gmenuitem *mi,GEvent *e) {
# elif defined(FONTFORGE_CONFIG_GTK)
void Menu_Preferences(GtkMenuItem *menuitem, gpointer user_data) {
# endif
    DoPrefs();
}

# if defined(FONTFORGE_CONFIG_GDRAW)
void MenuSaveAll(GWindow base,struct gmenuitem *mi,GEvent *e) {
# elif defined(FONTFORGE_CONFIG_GTK)
void Menu_SaveAll(GtkMenuItem *menuitem, gpointer user_data) {
# endif
    FontView *fv;

    for ( fv = fv_list; fv!=NULL; fv = fv->next ) {
	if ( SFAnyChanged(fv->sf) && !_FVMenuSave(fv))
return;
    }
}
#endif

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void _MenuExit(void *junk) {
    FontView *fv, *next;

    for ( fv = fv_list; fv!=NULL; fv = next ) {
	next = fv->next;
	if ( !_FVMenuClose(fv))
return;
#ifdef FONTFORGE_CONFIG_GDRAW
	if ( fv->nextsame!=NULL || fv->sf->fv!=fv ) {
	    GDrawSync(NULL);
	    GDrawProcessPendingEvents(NULL);
	}
#endif
    }
    exit(0);
}

# ifdef FONTFORGE_CONFIG_GTK
static void Menu_Exit(GtkMenuItem *menuitem, gpointer user_data) {
    _MenuExit(NULL);
}

char *GetPostscriptFontName(char *dir, int mult) {
    /* Some people use pf3 as an extension for postscript type3 fonts */
    /* Any of these extensions */
    static char wild[] = "*.{"
	   "pfa,"
	   "pfb,"
	   "pt3,"
	   "t42,"
	   "sfd,"
	   "ttf,"
	   "bdf,"
	   "otf,"
	   "otb,"
	   "cff,"
	   "cef,"
	   "gai,"
#ifndef _NO_LIBXML
	   "svg,"
#endif
	   "pf3,"
	   "ttc,"
	   "gsf,"
	   "cid,"
	   "bin,"
	   "hqx,"
	   "dfont,"
	   "mf,"
	   "ik,"
	   "fon,"
	   "fnt,"
	   "pdb"
	   "}"
/* With any of these methods of compression */
	     "{.gz,.Z,.bz2,}";
    gsize read, written;
    char *u_dir, *ret, *temp;

    u_dir = g_filename_to_utf8(dir,-1,&read,&written,NULL);
    ret = FVOpenFont(_("Open Font"), u_dir,wild,mult);
    temp = g_filename_from_utf8(ret,-1,&read,&written,NULL);
    free(u_dir); free(ret);

return( temp );
}

void MergeKernInfo(SplineFont *sf,EncMap *map) {
#ifndef __Mac
    static char wild[] = "*.{afm,tfm,ofm,pfm,bin,hqx,dfont,fea}";
    static char wild2[] = "*.{afm,amfm,tfm,ofm,pfm,bin,hqx,dfont,fea}";
#else
    static char wild[] = "*";	/* Mac resource files generally don't have extensions */
    static char wild2[] = "*";
#endif
    char *ret = gwwv_open_filename(_("Merge Feature Info"),NULL,
	    sf->mm!=NULL?wild2:wild,NULL);

    if ( ret==NULL )		/* Cancelled */
return;

    if ( !LoadKerningDataFromMetricsFile(sf,ret,map))
	gwwv_post_error( _("Failed to load kern data from %s"), ret);
    free(ret);
}
# elif defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuExit(GWindow base,struct gmenuitem *mi,GEvent *e) {
    _MenuExit(NULL);
}

void MenuExit(GWindow base,struct gmenuitem *mi,GEvent *e) {
    if ( e==NULL )	/* Not from the menu directly, but a shortcut */
	_MenuExit(NULL);
    else
	DelayEvent(_MenuExit,NULL);
}

char *GetPostscriptFontName(char *dir, int mult) {
    /* Some people use pf3 as an extension for postscript type3 fonts */
    unichar_t *ret;
    char *u_dir;
    char *temp;

    u_dir = def2utf8_copy(dir);
    ret = FVOpenFont(_("Open Font"), u_dir,mult);
    temp = u2def_copy(ret);

    free(ret);
return( temp );
}

void MergeKernInfo(SplineFont *sf,EncMap *map) {
#ifndef __Mac
    static char wild[] = "*.{afm,tfm,ofm,pfm,bin,hqx,dfont,fea}";
    static char wild2[] = "*.{afm,amfm,tfm,ofm,pfm,bin,hqx,dfont,fea}";
#else
    static char wild[] = "*";	/* Mac resource files generally don't have extensions */
    static char wild2[] = "*";
#endif
#if defined(FONTFORGE_CONFIG_GTK)
    gsize read, written;
#endif
    char *ret = gwwv_open_filename(_("Merge Feature Info"),NULL,
	    sf->mm!=NULL?wild2:wild,NULL);
    char *temp;

    if ( ret==NULL )
return;				/* Cancelled */
# ifdef FONTFORGE_CONFIG_GDRAW
    temp = utf82def_copy(ret);
# elif defined(FONTFORGE_CONFIG_GTK)
    temp = g_utf8_to_filename(ret,-1,&read,&written,NULL);
#endif

    if ( !LoadKerningDataFromMetricsFile(sf,temp,map))
	gwwv_post_error(_("Load of Kerning Metrics Failed"),_("Failed to load kern data from %s"), temp);
    free(ret); free(temp);
}
# endif
#endif

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuMergeKern(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_MergeKern(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    MergeKernInfo(fv->sf,fv->map);
}

# ifdef FONTFORGE_CONFIG_GDRAW
void MenuOpen(GWindow base,struct gmenuitem *mi,GEvent *e) {
# elif defined(FONTFORGE_CONFIG_GTK)
void Menu_Open(GtkMenuItem *menuitem, gpointer user_data) {
# endif
    char *temp;
    char *eod, *fpt, *file, *full;
    FontView *test; int fvcnt, fvtest;

    for ( fvcnt=0, test=fv_list; test!=NULL; ++fvcnt, test=test->next );
    do {
	temp = GetPostscriptFontName(NULL,true);
	if ( temp==NULL )
return;
	eod = strrchr(temp,'/');
	*eod = '\0';
	file = eod+1;
	do {
	    fpt = strstr(file,"; ");
	    if ( fpt!=NULL ) *fpt = '\0';
	    full = galloc(strlen(temp)+1+strlen(file)+1);
	    strcpy(full,temp); strcat(full,"/"); strcat(full,file);
	    ViewPostscriptFont(full);
	    file = fpt+2;
	    free(full);
	} while ( fpt!=NULL );
	free(temp);
	for ( fvtest=0, test=fv_list; test!=NULL; ++fvtest, test=test->next );
    } while ( fvtest==fvcnt );	/* did the load fail for some reason? try again */
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuContextualHelp(GWindow base,struct gmenuitem *mi,GEvent *e) {
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_ContextualHelp(GtkMenuItem *menuitem, gpointer user_data) {
# endif
    help("fontview.html");
}

# ifdef FONTFORGE_CONFIG_GDRAW
void MenuHelp(GWindow base,struct gmenuitem *mi,GEvent *e) {
# elif defined(FONTFORGE_CONFIG_GTK)
void MenuHelp_Help(GtkMenuItem *menuitem, gpointer user_data) {
# endif
    help("overview.html");
}

# ifdef FONTFORGE_CONFIG_GDRAW
void MenuIndex(GWindow base,struct gmenuitem *mi,GEvent *e) {
# elif defined(FONTFORGE_CONFIG_GTK)
void MenuHelp_Index(GtkMenuItem *menuitem, gpointer user_data) {
# endif
    help("IndexFS.html");
}

# ifdef FONTFORGE_CONFIG_GDRAW
void MenuLicense(GWindow base,struct gmenuitem *mi,GEvent *e) {
# elif defined(FONTFORGE_CONFIG_GTK)
void MenuHelp_License(GtkMenuItem *menuitem, gpointer user_data) {
# endif
    help("license.html");
}

# ifdef FONTFORGE_CONFIG_GDRAW
void MenuAbout(GWindow base,struct gmenuitem *mi,GEvent *e) {
# elif defined(FONTFORGE_CONFIG_GTK)
void MenuHelp_About(GtkMenuItem *menuitem, gpointer user_data) {
# endif
    ShowAboutScreen();
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuImport(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Import(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    int empty = fv->sf->onlybitmaps && fv->sf->bitmaps==NULL;
    BDFFont *bdf;
    FVImport(fv);
    if ( empty && fv->sf->bitmaps!=NULL ) {
	for ( bdf= fv->sf->bitmaps; bdf->next!=NULL; bdf = bdf->next );
	FVChangeDisplayBitmap(fv,bdf);
    }
}

static int FVSelCount(FontView *fv) {
    int i, cnt=0;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] ) ++cnt;
    if ( cnt>10 ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	char *buts[3];
	buts[0] = _("_OK");
	buts[1] = _("_Cancel");
	buts[2] = NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
	static char *buts[] = { GTK_STOCK_OK, GTK_STOCK_CANCEL, NULL };
#endif
	if ( gwwv_ask(_("Many Windows"),(const char **) buts,0,1,_("This involves opening more than 10 windows.\nIs that really what you want?"))==1 )
return( false );
    }
return( true );
}
	
# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuOpenOutline(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_OpenOutline(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    int i;
    SplineChar *sc;

    if ( !FVSelCount(fv))
return;
    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] ) {
	    sc = FVMakeChar(fv,i);
	    CharViewCreate(sc,fv,i);
	}
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuOpenBitmap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_OpenBitmap(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    int i;
    SplineChar *sc;

    if ( fv->cidmaster==NULL ? (fv->sf->bitmaps==NULL) : (fv->cidmaster->bitmaps==NULL) )
return;
    if ( !FVSelCount(fv))
return;
    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] ) {
	    sc = FVMakeChar(fv,i);
	    if ( sc!=NULL )
		BitmapViewCreatePick(i,fv);
	}
}

# ifdef FONTFORGE_CONFIG_GDRAW
void _MenuWarnings(GWindow gw,struct gmenuitem *mi,GEvent *e) {
# elif defined(FONTFORGE_CONFIG_GTK)
void WindowMenu_Warnings(GtkMenuItem *menuitem, gpointer user_data) {
# endif
    ShowErrorWindow();
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuOpenMetrics(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_OpenMetrics(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    MetricsViewCreate(fv,NULL,fv->filled==fv->show?NULL:fv->show);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuPrint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Print(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    PrintDlg(fv,NULL,NULL);
}

#if !defined(_NO_FFSCRIPT) || !defined(_NO_PYTHON)
# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuExecute(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_ExecScript(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    ScriptDlg(fv,NULL);
}
#endif

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuFontInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_FontInfo(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FontMenuFontInfo(fv);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuMATHInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_MATHInfo(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    SFMathDlg(fv->sf);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuFindProblems(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_FindProbs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FindProblems(fv,NULL,NULL);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuValidate(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Validate(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    SFValidationWindow(fv->sf,ff_none);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuEmbolden(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Embolden(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    EmboldenDlg(fv,NULL);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuOblique(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Oblique(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    ObliqueDlg(fv,NULL);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuCondense(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Condense(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    CondenseExtendDlg(fv,NULL);
}

# ifdef FONTFORGE_CONFIG_GDRAW
#define MID_24	2001
#define MID_36	2002
#define MID_48	2004
#define MID_72	2014
#define MID_96	2015
#define MID_AntiAlias	2005
#define MID_Next	2006
#define MID_Prev	2007
#define MID_NextDef	2012
#define MID_PrevDef	2013
#define MID_ShowHMetrics 2016
#define MID_ShowVMetrics 2017
#define MID_Ligatures	2020
#define MID_KernPairs	2021
#define MID_AnchorPairs	2022
#define MID_FitToEm	2023
#define MID_DisplaySubs	2024
#define MID_32x8	2025
#define MID_16x4	2026
#define MID_8x2		2027
#define MID_BitmapMag	2028
#define MID_CharInfo	2201
#define MID_FindProblems 2216
#define MID_Embolden	2217
#define MID_Condense	2218
#define MID_Transform	2202
#define MID_Stroke	2203
#define MID_RmOverlap	2204
#define MID_Simplify	2205
#define MID_Correct	2206
#define MID_BuildAccent	2208
#define MID_AvailBitmaps	2210
#define MID_RegenBitmaps	2211
#define MID_Autotrace	2212
#define MID_Round	2213
#define MID_MergeFonts	2214
#define MID_InterpolateFonts	2215
#define MID_ShowDependentRefs	2222
#define MID_AddExtrema	2224
#define MID_CleanupGlyph	2225
#define MID_TilePath	2226
#define MID_BuildComposite	2227
#define MID_NLTransform	2228
#define MID_Intersection	2229
#define MID_FindInter	2230
#define MID_Styles	2231
#define MID_SimplifyMore	2233
#define MID_ShowDependentSubs	2234
#define MID_DefaultATT	2235
#define MID_POV		2236
#define MID_BuildDuplicates	2237
#define MID_StrikeInfo	2238
#define MID_FontCompare	2239
#define MID_CanonicalStart	2242
#define MID_CanonicalContours	2243
#define MID_RemoveBitmaps	2244
#define MID_Validate		2245
#define MID_MassRename		2246
#define MID_Center	2600
#define MID_Thirds	2601
#define MID_SetWidth	2602
#define MID_SetLBearing	2603
#define MID_SetRBearing	2604
#define MID_SetVWidth	2605
#define MID_RmHKern	2606
#define MID_RmVKern	2607
#define MID_VKernByClass	2608
#define MID_VKernFromH	2609
#define MID_AutoHint	2501
#define MID_ClearHints	2502
#define MID_ClearWidthMD	2503
#define MID_AutoInstr	2504
#define MID_EditInstructions	2505
#define MID_Editfpgm	2506
#define MID_Editprep	2507
#define MID_ClearInstrs	2508
#define MID_HStemHist	2509
#define MID_VStemHist	2510
#define MID_BlueValuesHist	2511
#define MID_Editcvt	2512
#define MID_HintSubsPt	2513
#define MID_AutoCounter	2514
#define MID_DontAutoHint	2515
#define MID_PrivateToCvt	2516
#define MID_Editmaxp	2517
#define MID_OpenBitmap	2700
#define MID_OpenOutline	2701
#define MID_Revert	2702
#define MID_Recent	2703
#define MID_Print	2704
#define MID_ScriptMenu	2705
#define MID_RevertGlyph	2707
#define MID_RevertToBackup 2708
#define MID_Cut		2101
#define MID_Copy	2102
#define MID_Paste	2103
#define MID_Clear	2104
#define MID_SelAll	2106
#define MID_CopyRef	2107
#define MID_UnlinkRef	2108
#define MID_Undo	2109
#define MID_Redo	2110
#define MID_CopyWidth	2111
#define MID_AllFonts		2122
#define MID_DisplayedFont	2123
#define	MID_CharName		2124
#define MID_RemoveUndoes	2114
#define MID_CopyFgToBg	2115
#define MID_ClearBackground	2116
#define MID_CopyLBearing	2125
#define MID_CopyRBearing	2126
#define MID_CopyVWidth	2127
#define MID_Join	2128
#define MID_PasteInto	2129
#define MID_SameGlyphAs	2130
#define MID_RplRef	2131
#define MID_PasteAfter	2132
#define	MID_TTFInstr	2134
#define	MID_CopyLookupData	2135
#define MID_Convert2CID	2800
#define MID_Flatten	2801
#define MID_InsertFont	2802
#define MID_InsertBlank	2803
#define MID_CIDFontInfo	2804
#define MID_RemoveFromCID 2805
#define MID_ConvertByCMap	2806
#define MID_FlattenByCMap	2807
#define MID_ChangeSupplement	2808
#define MID_Reencode		2830
#define MID_ForceReencode	2831
#define MID_AddUnencoded	2832
#define MID_RemoveUnused	2833
#define MID_DetachGlyphs	2834
#define MID_DetachAndRemoveGlyphs	2835
#define MID_LoadEncoding	2836
#define MID_MakeFromFont	2837
#define MID_RemoveEncoding	2838
#define MID_DisplayByGroups	2839
#define MID_Compact	2840
#define MID_SaveNamelist	2841
#define MID_RenameGlyphs	2842
#define MID_NameGlyphs		2843
#define MID_CreateMM	2900
#define MID_MMInfo	2901
#define MID_MMValid	2902
#define MID_ChangeMMBlend	2903
#define MID_BlendToNew	2904
#define MID_ModifyComposition	20902
#define MID_BuildSyllables	20903

#define MID_Warnings	3000
# endif
#endif

/* returns -1 if nothing selected, if exactly one char return it, -2 if more than one */
static int FVAnyCharSelected(FontView *fv) {
    int i, val=-1;

    for ( i=0; i<fv->map->enccount; ++i ) {
	if ( fv->selected[i]) {
	    if ( val==-1 )
		val = i;
	    else
return( -2 );
	}
    }
return( val );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static int FVAllSelected(FontView *fv) {
    int i, any = false;
    /* Is everything real selected? */

    for ( i=0; i<fv->sf->glyphcnt; ++i ) if ( SCWorthOutputting(fv->sf->glyphs[i])) {
	if ( !fv->selected[fv->map->backmap[i]] )
return( false );
	any = true;
    }
return( any );
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuCopyFrom(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    /*FontView *fv = (FontView *) GDrawGetUserData(gw);*/

    if ( mi->mid==MID_CharName )
	copymetadata = !copymetadata;
    else if ( mi->mid==MID_TTFInstr )
	copyttfinstr = !copyttfinstr;
    else
	onlycopydisplayed = (mi->mid==MID_DisplayedFont);
    SavePrefs();
}
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_CopyFromAll(GtkMenuItem *menuitem, gpointer user_data) {

    onlycopydisplayed = false;
    SavePrefs();
}

void FontViewMenu_CopyFromDisplayed(GtkMenuItem *menuitem, gpointer user_data) {

    onlycopydisplayed = true;
    SavePrefs();
}

void FontViewMenu_CopyFromMetadata(GtkMenuItem *menuitem, gpointer user_data) {

    copymetadata = !copymetadata;
    SavePrefs();
}
# endif

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuCopy(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Copy(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    if ( FVAnyCharSelected(fv)==-1 )
return;
    FVCopy(fv,ct_fullcopy);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuCopyLookupData(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_CopyLookupData(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    if ( FVAnyCharSelected(fv)==-1 )
return;
    FVCopy(fv,ct_lookups);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuCopyRef(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_CopyRef(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    if ( FVAnyCharSelected(fv)==-1 )
return;
    FVCopy(fv,ct_reference);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuCopyWidth(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    if ( FVAnyCharSelected(fv)==-1 )
return;
    if ( mi->mid==MID_CopyVWidth && !fv->sf->hasvmetrics )
return;
    FVCopyWidth(fv,mi->mid==MID_CopyWidth?ut_width:
		   mi->mid==MID_CopyVWidth?ut_vwidth:
		   mi->mid==MID_CopyLBearing?ut_lbearing:
					 ut_rbearing);
}
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_CopyWidth(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    if ( FVAnyCharSelected(fv)==-1 )
return;
    FVCopyWidth(fv,ut_width);
}

void FontViewMenu_CopyVWidth(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    if ( FVAnyCharSelected(fv)==-1 )
return;
    if ( !fv->sf->hasvmetrics )
return;
    FVCopyWidth(fv,ut_vwidth);
}

void FontViewMenu_CopyLBearing(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    if ( FVAnyCharSelected(fv)==-1 )
return;
    FVCopyWidth(fv,ut_lbearing);
}

void FontViewMenu_CopyRBearing(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    if ( FVAnyCharSelected(fv)==-1 )
return;
    FVCopyWidth(fv,ut_rbearing);
}
# endif

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuPaste(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Paste(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    if ( FVAnyCharSelected(fv)==-1 )
return;
    PasteIntoFV(fv,false,NULL);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuPasteInto(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_PasteInto(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    if ( FVAnyCharSelected(fv)==-1 )
return;
    PasteIntoFV(fv,true,NULL);
}

#ifdef FONTFORGE_CONFIG_PASTEAFTER
# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuPasteAfter(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_PasteAfter(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    int pos = FVAnyCharSelected(fv);
    if ( pos<0 )
return;
    PasteIntoFV(fv,2,NULL);
}
#elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_PasteAfter(GtkMenuItem *menuitem, gpointer user_data) {
}
#endif
#endif

void AltUniRemove(SplineChar *sc,int uni) {
    struct altuni *altuni, *prev;

    if ( sc==NULL || uni==-1 )
return;

    if ( sc->unicodeenc==uni && sc->altuni!=NULL ) {
	sc->unicodeenc = sc->altuni->unienc;
	sc->altuni->unienc = uni;
    }

    if ( sc->unicodeenc==uni )
return;
    for ( prev=NULL, altuni=sc->altuni; altuni!=NULL && altuni->unienc!=uni;
	    prev = altuni, altuni = altuni->next );
    if ( altuni ) {
	if ( prev==NULL )
	    sc->altuni = altuni->next;
	else
	    prev->next = altuni->next;
	altuni->next = NULL;
	AltUniFree(altuni);
    }
}

void AltUniAdd(SplineChar *sc,int uni) {
    struct altuni *altuni;

    if ( sc!=NULL && uni!=-1 && uni!=sc->unicodeenc ) {
	for ( altuni = sc->altuni; altuni!=NULL && altuni->unienc!=uni; altuni=altuni->next );
	if ( altuni==NULL ) {
	    altuni = chunkalloc(sizeof(struct altuni));
	    altuni->next = sc->altuni;
	    sc->altuni = altuni;
	    altuni->unienc = uni;
	}
    }
}

static void LinkEncToGid(FontView *fv,int enc, int gid) {
    EncMap *map = fv->map;
    int old_gid;
    int flags = -1;
    int j;

    if ( map->map[enc]!=-1 && map->map[enc]!=gid ) {
	SplineFont *sf = fv->sf;
	old_gid = map->map[enc];
	for ( j=0; j<map->enccount; ++j )
	    if ( j!=enc && map->map[j]==old_gid )
	break;
	/* If the glyph is used elsewhere in the encoding then reusing this */
	/* slot causes no problems */
	if ( j==map->enccount ) {
	    /* However if this is the only use and the glyph is interesting */
	    /*  then add it to the unencoded area. If it is uninteresting we*/
	    /*  can just get rid of it */
	    if ( SCWorthOutputting(sf->glyphs[old_gid]) )
		SFAddEncodingSlot(sf,old_gid);
	    else
		SFRemoveGlyph(sf,sf->glyphs[old_gid],&flags);
	}
    }
    map->map[enc] = gid;
    if ( map->backmap[gid]==-1 )
	map->backmap[gid] = enc;
    if ( map->enc!=&custom ) {
	int uni = UniFromEnc(enc,map->enc);
	AltUniAdd(fv->sf->glyphs[gid],uni);
    }    
}

static void FVSameGlyphAs(FontView *fv) {
    SplineFont *sf = fv->sf;
    RefChar *base = CopyContainsRef(sf);
    int i;
    EncMap *map = fv->map;

    if ( base==NULL || fv->cidmaster!=NULL )
return;
    for ( i=0; i<map->enccount; ++i ) if ( fv->selected[i] ) {
	LinkEncToGid(fv,i,base->orig_pos);
    }
# ifdef FONTFORGE_CONFIG_GDRAW
    GDrawRequestExpose(fv->v,NULL,false);
# elif defined(FONTFORGE_CONFIG_GTK)
    gtk_widget_queue_draw(fv->v);
#endif
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuSameGlyphAs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_SameGlyphAs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVSameGlyphAs(fv);
}
#endif

static void FVCopyFgtoBg(FontView *fv) {
    int i, gid;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
		fv->sf->glyphs[gid]!=NULL &&
		fv->sf->glyphs[gid]->layers[1].splines!=NULL )
	    SCCopyFgToBg(fv->sf->glyphs[gid],true);
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuCopyFgBg(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_CopyFg2Bg(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVCopyFgtoBg( fv );
}
#endif

void BCClearAll(BDFChar *bc) {
    if ( bc==NULL )
return;
    BCPreserveState(bc);
    BCFlattenFloat(bc);
    memset(bc->bitmap,'\0',bc->bytes_per_line*(bc->ymax-bc->ymin+1));
    BCCompressBitmap(bc);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    BCCharChangedUpdate(bc);
#endif
}

void SCClearContents(SplineChar *sc) {
    RefChar *refs, *next;
    int layer;

    if ( sc==NULL )
return;
    sc->widthset = false;
    sc->width = sc->parent->ascent+sc->parent->descent;
    for ( layer = ly_fore; layer<sc->layer_cnt; ++layer ) {
	SplinePointListsFree(sc->layers[layer].splines);
	sc->layers[layer].splines = NULL;
	for ( refs=sc->layers[layer].refs; refs!=NULL; refs = next ) {
	    next = refs->next;
	    SCRemoveDependent(sc,refs);
	}
	sc->layers[layer].refs = NULL;
    }
    AnchorPointsFree(sc->anchor);
    sc->anchor = NULL;
    StemInfosFree(sc->hstem); sc->hstem = NULL;
    StemInfosFree(sc->vstem); sc->vstem = NULL;
    DStemInfosFree(sc->dstem); sc->dstem = NULL;
    MinimumDistancesFree(sc->md); sc->md = NULL;
    free(sc->ttf_instrs);
    sc->ttf_instrs = NULL;
    sc->ttf_instrs_len = 0;
    SCOutOfDateBackground(sc);
}

void SCClearAll(SplineChar *sc) {

    if ( sc==NULL )
return;
    if ( sc->layers[1].splines==NULL && sc->layers[ly_fore].refs==NULL && !sc->widthset &&
	    sc->hstem==NULL && sc->vstem==NULL && sc->anchor==NULL &&
	    (!copymetadata ||
		(sc->unicodeenc==-1 && strcmp(sc->name,".notdef")==0)))
return;
    SCPreserveState(sc,2);
    if ( copymetadata ) {
	sc->unicodeenc = -1;
	free(sc->name);
	sc->name = copy(".notdef");
	PSTFree(sc->possub);
	sc->possub = NULL;
    }
    SCClearContents(sc);
    SCCharChangedUpdate(sc);
}

void SCClearBackground(SplineChar *sc) {

    if ( sc==NULL )
return;
    if ( sc->layers[0].splines==NULL && sc->layers[ly_back].images==NULL )
return;
    SCPreserveBackground(sc);
    SplinePointListsFree(sc->layers[0].splines);
    sc->layers[0].splines = NULL;
    ImageListsFree(sc->layers[ly_back].images);
    sc->layers[ly_back].images = NULL;
    SCOutOfDateBackground(sc);
    SCCharChangedUpdate(sc);
}

static int UnselectedDependents(FontView *fv, SplineChar *sc) {
    struct splinecharlist *dep;

    for ( dep=sc->dependents; dep!=NULL; dep=dep->next ) {
	if ( !fv->selected[fv->map->backmap[dep->sc->orig_pos]] )
return( true );
	if ( UnselectedDependents(fv,dep->sc))
return( true );
    }
return( false );
}

void UnlinkThisReference(FontView *fv,SplineChar *sc) {
    /* We are about to clear out sc. But somebody refers to it and that we */
    /*  aren't going to delete. So (if the user asked us to) instanciate sc */
    /*  into all characters which refer to it and which aren't about to be */
    /*  cleared out */
    struct splinecharlist *dep, *dnext;

    for ( dep=sc->dependents; dep!=NULL; dep=dnext ) {
	dnext = dep->next;
	if ( fv==NULL || !fv->selected[fv->map->backmap[dep->sc->orig_pos]]) {
	    SplineChar *dsc = dep->sc;
	    RefChar *rf, *rnext;
	    /* May be more than one reference to us, colon has two refs to period */
	    /*  but only one dlist entry */
	    for ( rf = dsc->layers[ly_fore].refs; rf!=NULL; rf=rnext ) {
		rnext = rf->next;
		if ( rf->sc == sc ) {
		    /* Even if we were to preserve the state there would be no */
		    /*  way to undo the operation until we undid the delete... */
		    SCRefToSplines(dsc,rf);
		    SCUpdateAll(dsc);
		}
	    }
	}
    }
}

static void FVClear(FontView *fv) {
    int i;
    BDFFont *bdf;
    int refstate = 0;
    char *buts[6];
    int yes, unsel, gid;
    /* refstate==0 => ask, refstate==1 => clearall, refstate==-1 => skip all */

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] && (gid=fv->map->map[i])!=-1 ) {
	if ( !onlycopydisplayed || fv->filled==fv->show ) {
	    /* If we are messing with the outline character, check for dependencies */
	    if ( refstate<=0 && fv->sf->glyphs[gid]!=NULL &&
		    fv->sf->glyphs[gid]->dependents!=NULL ) {
		unsel = UnselectedDependents(fv,fv->sf->glyphs[gid]);
		if ( refstate==-2 && unsel ) {
		    UnlinkThisReference(fv,fv->sf->glyphs[gid]);
		} else if ( unsel ) {
		    if ( refstate<0 )
    continue;
#if defined(FONTFORGE_CONFIG_GDRAW)
		    buts[0] = _("_Yes");
		    buts[4] = _("_No");
#elif defined(FONTFORGE_CONFIG_GTK)
		    buts[0] = GTK_STOCK_YES;
		    buts[4] = GTK_STOCK_NO;
#endif
		    buts[1] = _("Yes to _All");
		    buts[2] = _("_Unlink All");
		    buts[3] = _("No _to All");
		    buts[5] = NULL;
		    yes = gwwv_ask(_("Bad Reference"),(const char **) buts,2,4,_("You are attempting to clear %.30s which is referred to by\nanother character. Are you sure you want to clear it?"),fv->sf->glyphs[gid]->name);
		    if ( yes==1 )
			refstate = 1;
		    else if ( yes==2 ) {
			UnlinkThisReference(fv,fv->sf->glyphs[gid]);
			refstate = -2;
		    } else if ( yes==3 )
			refstate = -1;
		    if ( yes>=3 )
    continue;
		}
	    }
	}

	if ( onlycopydisplayed && fv->filled==fv->show ) {
	    SCClearAll(fv->sf->glyphs[gid]);
	} else if ( onlycopydisplayed ) {
	    BCClearAll(fv->show->glyphs[gid]);
	} else {
	    SCClearAll(fv->sf->glyphs[gid]);
	    for ( bdf=fv->sf->bitmaps; bdf!=NULL; bdf = bdf->next )
		BCClearAll(bdf->glyphs[gid]);
	}
    }
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuClear(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Clear(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVClear( fv );
}
#endif

static void FVClearBackground(FontView *fv) {
    SplineFont *sf = fv->sf;
    int i, gid;

    if ( onlycopydisplayed && fv->filled!=fv->show )
return;

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 && sf->glyphs[gid]!=NULL ) {
	SCClearBackground(sf->glyphs[gid]);
    }
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuClearBackground(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_ClearBackground(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVClearBackground( fv );
}
#endif

static void FVJoin(FontView *fv) {
    SplineFont *sf = fv->sf;
    int i,changed,gid;
    extern float joinsnap;

    if ( onlycopydisplayed && fv->filled!=fv->show )
return;

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 && sf->glyphs[gid]!=NULL ) {
	SCPreserveState(sf->glyphs[gid],false);
	sf->glyphs[gid]->layers[ly_fore].splines = SplineSetJoin(sf->glyphs[gid]->layers[ly_fore].splines,true,joinsnap,&changed);
	if ( changed )
	    SCCharChangedUpdate(sf->glyphs[gid]);
    }
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuJoin(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Join(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVJoin( fv );
}
#endif

static void FVUnlinkRef(FontView *fv) {
    int i,layer, gid;
    SplineChar *sc;
    RefChar *rf, *next;

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 && (sc = fv->sf->glyphs[gid])!=NULL &&
	     sc->layers[ly_fore].refs!=NULL ) {
	SCPreserveState(sc,false);
	for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
	    for ( rf=sc->layers[ly_fore].refs; rf!=NULL ; rf=next ) {
		next = rf->next;
		SCRefToSplines(sc,rf);
	    }
	}
	SCCharChangedUpdate(sc);
    }
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuUnlinkRef(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_UnlinkRef(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVUnlinkRef( fv );
}
#endif

void SFRemoveUndoes(SplineFont *sf,uint8 *selected, EncMap *map) {
    SplineFont *main = sf->cidmaster? sf->cidmaster : sf, *ssf;
    int i,k, max, layer, gid;
    SplineChar *sc;
    BDFFont *bdf;

    if ( selected!=NULL || main->subfontcnt==0 )
	max = sf->glyphcnt;
    else {
	max = 0;
	for ( k=0; k<main->subfontcnt; ++k )
	    if ( main->subfonts[k]->glyphcnt>max ) max = main->subfonts[k]->glyphcnt;
    }
    for ( i=0; ; ++i ) {
	if ( selected==NULL || main->subfontcnt!=0 ) {
	    if ( i>=max )
    break;
	    gid = i;
	} else {
	    if ( i>=map->enccount )
    break;
	    if ( !selected[i])
    continue;
	    gid = map->map[i];
	    if ( gid==-1 )
    continue;
	}
	for ( bdf=main->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( bdf->glyphs[gid]!=NULL ) {
		UndoesFree(bdf->glyphs[gid]->undoes); bdf->glyphs[gid]->undoes = NULL;
		UndoesFree(bdf->glyphs[gid]->redoes); bdf->glyphs[gid]->redoes = NULL;
	    }
	}
	k = 0;
	do {
	    ssf = main->subfontcnt==0? main: main->subfonts[k];
	    if ( gid<ssf->glyphcnt && ssf->glyphs[gid]!=NULL ) {
		sc = ssf->glyphs[gid];
		for ( layer = 0; layer<sc->layer_cnt; ++layer ) {
		    UndoesFree(sc->layers[layer].undoes); sc->layers[layer].undoes = NULL;
		    UndoesFree(sc->layers[layer].redoes); sc->layers[layer].redoes = NULL;
		}
	    }
	    ++k;
	} while ( k<main->subfontcnt );
    }
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuRemoveUndoes(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_RemoveUndoes(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    SFRemoveUndoes(fv->sf,fv->selected,fv->map);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuUndo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Undo(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    int i,j,layer, gid;
    MMSet *mm = fv->sf->mm;
    int was_blended = mm!=NULL && mm->normal==fv->sf;

    SFUntickAll(fv->sf);
    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
		fv->sf->glyphs[gid]!=NULL && !fv->sf->glyphs[gid]->ticked) {
	    SplineChar *sc = fv->sf->glyphs[gid];
	    for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
		if ( sc->layers[layer].undoes!=NULL ) {
		    SCDoUndo(sc,layer);
		    if ( was_blended ) {
			for ( j=0; j<mm->instance_count; ++j )
			    SCDoUndo(mm->instances[j]->glyphs[gid],layer);
		    }
		}
	    }
	    sc->ticked = true;
	}
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuRedo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Redo(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    int i,j,layer, gid;
    MMSet *mm = fv->sf->mm;
    int was_blended = mm!=NULL && mm->normal==fv->sf;

    SFUntickAll(fv->sf);
    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
		fv->sf->glyphs[gid]!=NULL && !fv->sf->glyphs[gid]->ticked) {
	    SplineChar *sc = fv->sf->glyphs[gid];
	    for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
		if ( sc->layers[layer].redoes!=NULL ) {
		    SCDoRedo(sc,layer);
		    if ( was_blended ) {
			for ( j=0; j<mm->instance_count; ++j )
			    SCDoRedo(mm->instances[j]->glyphs[gid],layer);
		    }
		}
	    }
	    sc->ticked = true;
	}
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuCut(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Cut(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVCopy(fv,ct_fullcopy);
    FVClear(fv);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuSelectAll(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_SelectAll(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    FVSelectAll(fv);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuInvertSelection(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_InvertSelection(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    FVInvertSelection(fv);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuDeselectAll(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_DeselectAll(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    FVDeselectAll(fv);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuSelectByName(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_SelectByName(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    FVSelectByName(fv);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuSelectByScript(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_SelectByScript(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    FVSelectByScript(fv);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuSelectWorthOutputting(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_SelectWorthOutputting(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    int i, gid;
    EncMap *map = fv->map;
    SplineFont *sf = fv->sf;

    for ( i=0; i< map->enccount; ++i )
	fv->selected[i] = ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL &&
		SCWorthOutputting(sf->glyphs[gid]) );
# ifdef FONTFORGE_CONFIG_GDRAW
    GDrawRequestExpose(fv->v,NULL,false);
# elif defined(FONTFORGE_CONFIG_GTK)
    gtk_widget_queue_draw(fv->v);
#endif
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuSelectChanged(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_SelectChangedGlyphs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    int i, gid;
    EncMap *map = fv->map;
    SplineFont *sf = fv->sf;

    for ( i=0; i< map->enccount; ++i )
	fv->selected[i] = ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL && sf->glyphs[gid]->changed );
# ifdef FONTFORGE_CONFIG_GDRAW
    GDrawRequestExpose(fv->v,NULL,false);
# elif defined(FONTFORGE_CONFIG_GTK)
    gtk_widget_queue_draw(fv->v);
#endif
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuSelectHintingNeeded(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_SelectUnhintedGlyphs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    int i, gid;
    EncMap *map = fv->map;
    SplineFont *sf = fv->sf;
    int order2 = sf->order2;

    for ( i=0; i< map->enccount; ++i )
	fv->selected[i] = ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL &&
		((!order2 && sf->glyphs[gid]->changedsincelasthinted ) ||
		 ( order2 && sf->glyphs[gid]->layers[ly_fore].splines!=NULL &&
		     sf->glyphs[gid]->ttf_instrs_len<=0 ) ||
		 ( order2 && sf->glyphs[gid]->instructions_out_of_date )) );
# ifdef FONTFORGE_CONFIG_GDRAW
    GDrawRequestExpose(fv->v,NULL,false);
# elif defined(FONTFORGE_CONFIG_GTK)
    gtk_widget_queue_draw(fv->v);
#endif
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuSelectColor(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    FVSelectColor(fv,(uint32) (intpt) (mi->ti.userdata),(e->u.chr.state&ksm_shift)?1:0);
}
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_SelectDefault(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVSelectColor(fv,COLOR_DEFAULT,false);
}

void FontViewMenu_SelectWhite(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVSelectColor(fv,0xffffff,false);
}

void FontViewMenu_SelectRed(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVSelectColor(fv,0xff0000,false);
}

void FontViewMenu_SelectGreen(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVSelectColor(fv,0x00ff00,false);
}

void FontViewMenu_SelectBlue(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVSelectColor(fv,0x0000ff,false);
}

void FontViewMenu_SelectYellow(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVSelectColor(fv,0xffff00,false);
}

void FontViewMenu_SelectCyan(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVSelectColor(fv,0x00ffff,false);
}

void FontViewMenu_SelectMagenta(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVSelectColor(fv,0xff00ff,false);
}

void FontViewMenu_SelectColorPicker(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    GtkWidget *color_picker = gtk_color_selection_dialog_new(_("Select Color"));
    GtkWidget *color_sel = GTK_COLOR_SELECTION_DIALOG( color_picker )->colorsel;
    static GdkColor last_col;

    gtk_color_selection_set_current_color(
	    GTK_COLOR_SELECTION( color_sel ), &last_col );
    if (gtk_dialog_run( GTK_DIALOG( color_picker )) == GTK_RESPONSE_ACCEPT) {
	gtk_color_selection_get_current_color(
		GTK_COLOR_SELECTION( color_sel ), &last_col );
	FVSelectColor(fv,((last_col.red>>8)<<16) |
			((last_col.green>>8)<<8) |
			 (last_col.blue>>8),
		false);
    }
    gtk_widget_destroy( color_picker );
}
# endif

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuSelectByPST(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_SelectByAtt(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    FVSelectByPST(fv);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuFindRpl(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_FindRpl(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    SVCreate(fv);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuReplaceWithRef(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_RplRef(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    FVReplaceOutlineWithReference(fv,.001);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuCharInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_CharInfo(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    int pos = FVAnyCharSelected(fv);
    if ( pos<0 )
return;
    if ( fv->cidmaster!=NULL &&
	    (fv->map->map[pos]==-1 || fv->sf->glyphs[fv->map->map[pos]]==NULL ))
return;
    SCCharInfo(SFMakeChar(fv->sf,fv->map,pos),fv->map,pos);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuBDFInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_BDFInfo(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    if ( fv->sf->bitmaps==NULL )
return;
    if ( fv->show!=fv->filled )
	SFBdfProperties(fv->sf,fv->map,fv->show);
    else
	SFBdfProperties(fv->sf,fv->map,NULL);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuMassRename(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_MassRename(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVMassGlyphRename(fv);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuShowDependentRefs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_ShowDependentRefs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    int pos = FVAnyCharSelected(fv);
    SplineChar *sc;

    if ( pos<0 || fv->map->map[pos]==-1 )
return;
    sc = fv->sf->glyphs[fv->map->map[pos]];
    if ( sc==NULL || sc->dependents==NULL )
return;
    SCRefBy(sc);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuShowDependentSubs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_ShowDependentSubs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    int pos = FVAnyCharSelected(fv);
    SplineChar *sc;

    if ( pos<0 || fv->map->map[pos]==-1 )
return;
    sc = fv->sf->glyphs[fv->map->map[pos]];
    if ( sc==NULL )
return;
    SCSubBy(sc);
}
#endif

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static int getorigin(void *d,BasePoint *base,int index) {
    /*FontView *fv = (FontView *) d;*/

    base->x = base->y = 0;
    switch ( index ) {
      case 0:		/* Character origin */
	/* all done */
      break;
      case 1:		/* Center of selection */
	/*CVFindCenter(cv,base,!CVAnySel(cv,NULL,NULL,NULL,NULL));*/
      break;
      default:
return( false );
    }
return( true );
}
#endif

void TransHints(StemInfo *stem,real mul1, real off1, real mul2, real off2, int round_to_int ) {
    HintInstance *hi;

    for ( ; stem!=NULL; stem=stem->next ) {
	stem->start = stem->start*mul1 + off1;
	stem->width *= mul1;
	if ( round_to_int ) {
	    stem->start = rint(stem->start);
	    stem->width = rint(stem->width);
	}
	if ( mul1<0 ) {
	    stem->start += stem->width;
	    stem->width = -stem->width;
	}
	for ( hi=stem->where; hi!=NULL; hi=hi->next ) {
	    hi->begin = hi->begin*mul2 + off2;
	    hi->end = hi->end*mul2 + off2;
	    if ( round_to_int ) {
		hi->begin = rint(hi->begin);
		hi->end = rint(hi->end);
	    }
	    if ( mul2<0 ) {
		double temp = hi->begin;
		hi->begin = hi->end;
		hi->end = temp;
	    }
	}
    }
}

void VrTrans(struct vr *vr,real transform[6]) {
    /* I'm interested in scaling and skewing. I think translation should */
    /*  not affect these guys (they are offsets, so offsets should be */
    /*  unchanged by translation */
    double x,y;

    x = vr->xoff; y=vr->yoff;
    vr->xoff = rint(transform[0]*x + transform[1]*y);
    vr->yoff = rint(transform[2]*x + transform[3]*y);
    x = vr->h_adv_off; y=vr->v_adv_off;
    vr->h_adv_off = rint(transform[0]*x + transform[1]*y);
    vr->v_adv_off = rint(transform[2]*x + transform[3]*y);
}

static void GV_Trans(struct glyphvariants *gv,real transform[6], int is_v) {
    int i;

    if ( gv==NULL )
return;
    gv->italic_correction = rint(gv->italic_correction*transform[0]);
    is_v = 3*is_v;
    for ( i=0; i<gv->part_cnt; ++i ) {
	gv->parts[i].startConnectorLength = rint( gv->parts[i].startConnectorLength*transform[is_v] );
	gv->parts[i].endConnectorLength = rint( gv->parts[i].endConnectorLength*transform[is_v] );
	gv->parts[i].fullAdvance = rint( gv->parts[i].fullAdvance*transform[is_v] );
    }
}

static void MKV_Trans(struct mathkernvertex *mkv,real transform[6]) {
    int i;

    for ( i=0; i<mkv->cnt; ++i ) {
	mkv->mkd[i].kern  = rint( mkv->mkd[i].kern  *transform[0]);
	mkv->mkd[i].height= rint( mkv->mkd[i].height*transform[0]);
    }
}

static void MK_Trans(struct mathkern *mk,real transform[6]) {
    if ( mk==NULL )
return;
    MKV_Trans(&mk->top_right,transform);
    MKV_Trans(&mk->top_left ,transform);
    MKV_Trans(&mk->bottom_right,transform);
    MKV_Trans(&mk->bottom_left ,transform);
}

static void MATH_Trans(struct MATH *math,real transform[6]) {
    if ( math==NULL )
return;
    math->DelimitedSubFormulaMinHeight = rint( math->DelimitedSubFormulaMinHeight*transform[3]);
    math->DisplayOperatorMinHeight = rint( math->DisplayOperatorMinHeight*transform[3]);
    math->MathLeading = rint( math->MathLeading*transform[3]);
    math->AxisHeight = rint( math->AxisHeight*transform[3]);
    math->AccentBaseHeight = rint( math->AccentBaseHeight*transform[3]);
    math->FlattenedAccentBaseHeight = rint( math->FlattenedAccentBaseHeight*transform[3]);
    math->SubscriptShiftDown = rint( math->SubscriptShiftDown*transform[3]);
    math->SubscriptTopMax = rint( math->SubscriptTopMax*transform[3]);
    math->SubscriptBaselineDropMin = rint( math->SubscriptBaselineDropMin*transform[3]);
    math->SuperscriptShiftUp = rint( math->SuperscriptShiftUp*transform[3]);
    math->SuperscriptShiftUpCramped = rint( math->SuperscriptShiftUpCramped*transform[3]);
    math->SuperscriptBottomMin = rint( math->SuperscriptBottomMin*transform[3]);
    math->SuperscriptBaselineDropMax = rint( math->SuperscriptBaselineDropMax*transform[3]);
    math->SubSuperscriptGapMin = rint( math->SubSuperscriptGapMin*transform[3]);
    math->SuperscriptBottomMaxWithSubscript = rint( math->SuperscriptBottomMaxWithSubscript*transform[3]);
    /* SpaceAfterScript is horizontal and is below */
    math->UpperLimitGapMin = rint( math->UpperLimitGapMin*transform[3]);
    math->UpperLimitBaselineRiseMin = rint( math->UpperLimitBaselineRiseMin*transform[3]);
    math->LowerLimitGapMin = rint( math->LowerLimitGapMin*transform[3]);
    math->LowerLimitBaselineDropMin = rint( math->LowerLimitBaselineDropMin*transform[3]);
    math->StackTopShiftUp = rint( math->StackTopShiftUp*transform[3]);
    math->StackTopDisplayStyleShiftUp = rint( math->StackTopDisplayStyleShiftUp*transform[3]);
    math->StackBottomShiftDown = rint( math->StackBottomShiftDown*transform[3]);
    math->StackBottomDisplayStyleShiftDown = rint( math->StackBottomDisplayStyleShiftDown*transform[3]);
    math->StackGapMin = rint( math->StackGapMin*transform[3]);
    math->StackDisplayStyleGapMin = rint( math->StackDisplayStyleGapMin*transform[3]);
    math->StretchStackTopShiftUp = rint( math->StretchStackTopShiftUp*transform[3]);
    math->StretchStackBottomShiftDown = rint( math->StretchStackBottomShiftDown*transform[3]);
    math->StretchStackGapAboveMin = rint( math->StretchStackGapAboveMin*transform[3]);
    math->StretchStackGapBelowMin = rint( math->StretchStackGapBelowMin*transform[3]);
    math->FractionNumeratorShiftUp = rint( math->FractionNumeratorShiftUp*transform[3]);
    math->FractionNumeratorDisplayStyleShiftUp = rint( math->FractionNumeratorDisplayStyleShiftUp*transform[3]);
    math->FractionDenominatorShiftDown = rint( math->FractionDenominatorShiftDown*transform[3]);
    math->FractionDenominatorDisplayStyleShiftDown = rint( math->FractionDenominatorDisplayStyleShiftDown*transform[3]);
    math->FractionNumeratorGapMin = rint( math->FractionNumeratorGapMin*transform[3]);
    math->FractionNumeratorDisplayStyleGapMin = rint( math->FractionNumeratorDisplayStyleGapMin*transform[3]);
    math->FractionRuleThickness = rint( math->FractionRuleThickness*transform[3]);
    math->FractionDenominatorGapMin = rint( math->FractionDenominatorGapMin*transform[3]);
    math->FractionDenominatorDisplayStyleGapMin = rint( math->FractionDenominatorDisplayStyleGapMin*transform[3]);
    /* SkewedFractionHorizontalGap is horizontal and is below */
    math->SkewedFractionVerticalGap = rint( math->SkewedFractionVerticalGap*transform[3]);
    math->OverbarVerticalGap = rint( math->OverbarVerticalGap*transform[3]);
    math->OverbarRuleThickness = rint( math->OverbarRuleThickness*transform[3]);
    math->OverbarExtraAscender = rint( math->OverbarExtraAscender*transform[3]);
    math->UnderbarVerticalGap = rint( math->UnderbarVerticalGap*transform[3]);
    math->UnderbarRuleThickness = rint( math->UnderbarRuleThickness*transform[3]);
    math->UnderbarExtraDescender = rint( math->UnderbarExtraDescender*transform[3]);
    math->RadicalVerticalGap = rint( math->RadicalVerticalGap*transform[3]);
    math->RadicalDisplayStyleVerticalGap = rint( math->RadicalDisplayStyleVerticalGap*transform[3]);
    math->RadicalRuleThickness = rint( math->RadicalRuleThickness*transform[3]);
    math->RadicalExtraAscender = rint( math->RadicalExtraAscender*transform[3]);
    math->RadicalDegreeBottomRaisePercent = rint( math->RadicalDegreeBottomRaisePercent*transform[3]);

    /* Horizontals */
    math->SpaceAfterScript = rint( math->SpaceAfterScript*transform[0]);
    math->SkewedFractionHorizontalGap = rint( math->SkewedFractionHorizontalGap*transform[0]);
    math->RadicalKernBeforeDegree = rint( math->RadicalKernBeforeDegree*transform[0]);
    math->RadicalKernAfterDegree = rint( math->RadicalKernAfterDegree*transform[0]);

    /* This number is the same for both horizontal and vertical connections */
    /*  Use the vertical amount as a) will probably be the same and */
    /*   b) most are vertical anyway */
    math->RadicalKernAfterDegree = rint( math->RadicalKernAfterDegree*transform[0]);
}

static void KCTrans(KernClass *kc,double scale) {
    /* Again these are offsets, so I don't apply translation */
    int i;

    for ( i=kc->first_cnt*kc->second_cnt-1; i>=0; --i )
	kc->offsets[i] = rint(scale*kc->offsets[i]);
}

/* If sel is specified then we decide how to transform references based on */
/*  whether the refered glyph is selected. (If we tranform a reference that */
/*  is selected we are, in effect, transforming it twice -- since the glyph */
/*  itself will be transformed -- so instead we just transform the offsets */
/*  of the reference */
/* If sel is NULL then we transform the reference */
/* if flags&fvt_partialreftrans then we always just transform the offsets */
void FVTrans(FontView *fv,SplineChar *sc,real transform[6], uint8 *sel,
	enum fvtrans_flags flags) {
    RefChar *refs;
    real t[6];
    AnchorPoint *ap;
    int i,j;
    KernPair *kp;
    PST *pst;

    if ( sc->blended ) {
	int j;
	MMSet *mm = sc->parent->mm;
	for ( j=0; j<mm->instance_count; ++j )
	    FVTrans(fv,mm->instances[j]->glyphs[sc->orig_pos],transform,sel,flags);
    }

    SCPreserveState(sc,true);
    if ( !(flags&fvt_dontmovewidth) )
	if ( transform[0]>0 && transform[3]>0 && transform[1]==0 && transform[2]==0 ) {
	    int widthset = sc->widthset;
	    SCSynchronizeWidth(sc,sc->width*transform[0]+transform[4],sc->width,fv);
	    if ( !(flags&fvt_dontsetwidth) ) sc->widthset = widthset;
	    sc->vwidth = sc->vwidth*transform[3]+transform[5];
	}
    if ( flags & fvt_scalepstpos ) {
	for ( kp=sc->kerns; kp!=NULL; kp=kp->next )
	    kp->off = rint(kp->off*transform[0]);
	for ( kp=sc->vkerns; kp!=NULL; kp=kp->next )
	    kp->off = rint(kp->off*transform[3]);
	for ( pst = sc->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->type == pst_position )
		VrTrans(&pst->u.pos,transform);
	    else if ( pst->type==pst_pair ) {
		VrTrans(&pst->u.pair.vr[0],transform);
		VrTrans(&pst->u.pair.vr[1],transform);
	    } else if ( pst->type == pst_lcaret ) {
		int j;
		for ( j=0; j<pst->u.lcaret.cnt; ++j )
		    pst->u.lcaret.carets[j] = rint(pst->u.lcaret.carets[j]*transform[0]+transform[4]);
	    }
	}
    }

    if ( sc->tex_height!=TEX_UNDEF )
	sc->tex_height = rint(sc->tex_height*transform[3]);
    if ( sc->tex_depth !=TEX_UNDEF )
	sc->tex_depth  = rint(sc->tex_depth *transform[3]);
    if ( sc->italic_correction!=TEX_UNDEF )
	sc->italic_correction = rint(sc->italic_correction *transform[0]);
    if ( sc->top_accent_horiz !=TEX_UNDEF )
	sc->top_accent_horiz  = rint(sc->top_accent_horiz *transform[0]);
    GV_Trans(sc->vert_variants ,transform, true);
    GV_Trans(sc->horiz_variants,transform, false);
    MK_Trans(sc->mathkern,transform);

    for ( ap=sc->anchor; ap!=NULL; ap=ap->next )
	ApTransform(ap,transform);
    for ( i=ly_fore; i<sc->layer_cnt; ++i ) {
	SplinePointListTransform(sc->layers[i].splines,transform,true);
	for ( refs = sc->layers[i].refs; refs!=NULL; refs=refs->next ) {
	    if ( (sel!=NULL && sel[fv->map->backmap[refs->sc->orig_pos]]) ||
		    (flags&fvt_partialreftrans)) {
		/* if the character referred to is selected then it's going to */
		/*  be scaled too (or will have been) so we don't want to scale */
		/*  it twice */
		t[4] = refs->transform[4]*transform[0] +
			    refs->transform[5]*transform[2] +
			    /*transform[4]*/0;
		t[5] = refs->transform[4]*transform[1] +
			    refs->transform[5]*transform[3] +
			    /*transform[5]*/0;
		t[0] = refs->transform[4]; t[1] = refs->transform[5];
		refs->transform[4] = t[4];
		refs->transform[5] = t[5];
		/* Now update the splines to match */
		t[4] -= t[0]; t[5] -= t[1];
		if ( t[4]!=0 || t[5]!=0 ) {
		    t[0] = t[3] = 1; t[1] = t[2] = 0;
		    for ( j=0; j<refs->layer_cnt; ++j )
			SplinePointListTransform(refs->layers[j].splines,t,true);
		}
	    } else {
		for ( j=0; j<refs->layer_cnt; ++j )
		    SplinePointListTransform(refs->layers[j].splines,transform,true);
		t[0] = refs->transform[0]*transform[0] +
			    refs->transform[1]*transform[2];
		t[1] = refs->transform[0]*transform[1] +
			    refs->transform[1]*transform[3];
		t[2] = refs->transform[2]*transform[0] +
			    refs->transform[3]*transform[2];
		t[3] = refs->transform[2]*transform[1] +
			    refs->transform[3]*transform[3];
		t[4] = refs->transform[4]*transform[0] +
			    refs->transform[5]*transform[2] +
			    transform[4];
		t[5] = refs->transform[4]*transform[1] +
			    refs->transform[5]*transform[3] +
			    transform[5];
		memcpy(refs->transform,t,sizeof(t));
	    }
	    RefCharFindBounds(refs);
	}
    }
    if ( transform[1]==0 && transform[2]==0 ) {
	if ( transform[0]==1 && transform[3]==1 &&
		transform[5]==0 && transform[4]!=0 && 
		sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 &&
		isalpha(sc->unicodeenc)) {
	    SCUndoSetLBearingChange(sc,(int) rint(transform[4]));
	    SCSynchronizeLBearing(sc,transform[4]);	/* this moves the hints */
	} else {
	    TransHints(sc->hstem,transform[3],transform[5],transform[0],transform[4],flags&fvt_round_to_int);
	    TransHints(sc->vstem,transform[0],transform[4],transform[3],transform[5],flags&fvt_round_to_int);
	}
    }
    if ( flags&fvt_round_to_int )
	SCRound2Int(sc,1.0);
    if ( flags&fvt_dobackground ) {
	ImageList *img;
	SCPreserveBackground(sc);
	SplinePointListTransform(sc->layers[ly_back].splines,transform,true);
	for ( img = sc->layers[ly_back].images; img!=NULL; img=img->next )
	    BackgroundImageTransform(sc, img, transform);
    }
    SCCharChangedUpdate(sc);
}

void FVTransFunc(void *_fv,real transform[6],int otype, BVTFunc *bvts,
	enum fvtrans_flags flags ) {
    FontView *fv = _fv;
    real transx = transform[4], transy=transform[5];
    DBounds bb;
    BasePoint base;
    int i, cnt=0, gid;
    BDFFont *bdf;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
		SCWorthOutputting(fv->sf->glyphs[gid]) )
	    ++cnt;

#if 0	/* Do this in scaletoem */
    if ( cnt>fv->sf->glyphcnt/2 ) {
	if ( transform[3]!=1.0 ) {
	    if ( transform[2]==0 ) {
		fv->sf->pfminfo.os2_typoascent = rint( fv->sf->pfminfo.os2_typoascent * transform[3]);
		fv->sf->pfminfo.os2_typodescent = rint( fv->sf->pfminfo.os2_typodescent * transform[3]);
		fv->sf->pfminfo.os2_typolinegap = rint( fv->sf->pfminfo.os2_typolinegap * transform[3]);
		fv->sf->pfminfo.os2_winascent = rint( fv->sf->pfminfo.os2_winascent * transform[3]);
		fv->sf->pfminfo.os2_windescent = rint( fv->sf->pfminfo.os2_windescent * transform[3]);
		fv->sf->pfminfo.hhead_ascent = rint( fv->sf->pfminfo.hhead_ascent * transform[3]);
		fv->sf->pfminfo.hhead_descent = rint( fv->sf->pfminfo.hhead_descent * transform[3]);
		fv->sf->pfminfo.linegap = rint( fv->sf->pfminfo.linegap * transform[3]);
		fv->sf->pfminfo.os2_subysize = rint( fv->sf->pfminfo.os2_subysize * transform[3]);
		fv->sf->pfminfo.os2_subyoff = rint( fv->sf->pfminfo.os2_subyoff * transform[3]);
		fv->sf->pfminfo.os2_supysize = rint( fv->sf->pfminfo.os2_supysize * transform[3]);
		fv->sf->pfminfo.os2_supyoff = rint( fv->sf->pfminfo.os2_supyoff * transform[3]);
		fv->sf->pfminfo.os2_strikeysize = rint( fv->sf->pfminfo.os2_strikeysize * transform[3]);
		fv->sf->pfminfo.os2_strikeypos = rint( fv->sf->pfminfo.os2_strikeypos * transform[3]);
		fv->sf->pfminfo.upos *= transform[3];
		fv->sf->pfminfo.uwidth *= transform[3];
	    }
	}
	if ( transform[1]==0 && transform[0]!=1.0 ) {
	    fv->sf->pfminfo.vlinegap = rint( fv->sf->pfminfo.vlinegap * transform[0]);
	    fv->sf->pfminfo.os2_subxsize = rint( fv->sf->pfminfo.os2_subxsize * transform[0]);
	    fv->sf->pfminfo.os2_subxoff = rint( fv->sf->pfminfo.os2_subxoff * transform[0]);
	    fv->sf->pfminfo.os2_supxsize = rint( fv->sf->pfminfo.os2_supxsize * transform[0]);
	    fv->sf->pfminfo.os2_supxoff = rint( fv->sf->pfminfo.os2_supxoff * transform[0]);
	}
    }
#endif

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_start_indicator(10,_("Transforming..."),_("Transforming..."),0,cnt,1);
# endif

    SFUntickAll(fv->sf);
    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 &&
	    SCWorthOutputting(fv->sf->glyphs[gid]) &&
	    !fv->sf->glyphs[gid]->ticked ) {
	SplineChar *sc = fv->sf->glyphs[gid];

	if ( onlycopydisplayed && fv->show!=fv->filled ) {
	    if ( fv->show->glyphs[gid]!=NULL )
		BCTrans(fv->show,fv->show->glyphs[gid],bvts,fv);
	} else {
	    if ( otype==1 ) {
		SplineCharFindBounds(sc,&bb);
		base.x = (bb.minx+bb.maxx)/2;
		base.y = (bb.miny+bb.maxy)/2;
		transform[4]=transx+base.x-
		    (transform[0]*base.x+transform[2]*base.y);
		transform[5]=transy+base.y-
		    (transform[1]*base.x+transform[3]*base.y);
	    }
	    FVTrans(fv,sc,transform,fv->selected,flags);
	    if ( !onlycopydisplayed ) {
		for ( bdf = fv->sf->bitmaps; bdf!=NULL; bdf=bdf->next )
		    if ( gid<bdf->glyphcnt && bdf->glyphs[gid]!=NULL )
			BCTrans(bdf,bdf->glyphs[gid],bvts,fv);
	    }
	}
	sc->ticked = true;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	if ( !gwwv_progress_next())
    break;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
    if ( flags&fvt_dogrid ) {
	SFPreserveGuide(fv->sf);
	SplinePointListTransform(fv->sf->grid.splines,transform,true);
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_end_indicator();
# endif

    if ( flags&fvt_scalekernclasses ) {
	KernClass *kc;
	SplineFont *sf = fv->cidmaster!=NULL ? fv->cidmaster : fv->sf;
	for ( kc=sf->kerns; kc!=NULL; kc=kc->next )
	    KCTrans(kc,transform[0]);
	for ( kc=sf->vkerns; kc!=NULL; kc=kc->next )
	    KCTrans(kc,transform[3]);
	if ( sf->MATH!=NULL )
	    MATH_Trans(sf->MATH,transform);
    }
}

static char *scaleString(char *string, double scale) {
    char *result;
    char *pt;
    char *end;
    double val;

    if ( string==NULL )
return( NULL );

    while ( *string==' ' ) ++string;
    result = galloc(10*strlen(string)+1);
    if ( *string!='[' ) {
	val = strtod(string,&end);
	if ( end==string ) {
	    free( result );
return( NULL );
	}
	sprintf( result, "%g", val*scale);
return( result );
    }

    pt = result;
    *pt++ = '[';
    ++string;
    while ( *string!='\0' && *string!=']' ) {
	val = strtod(string,&end);
	if ( end==string ) {
	    free(result);
return( NULL );
	}
	sprintf( pt, "%g ", val*scale);
	pt += strlen(pt);
	string = end;
    }
    if ( pt[-1] == ' ' ) pt[-1] = ']';
    else *pt++ = ']';
    *pt = '\0';
return( result );
}

static char *iscaleString(char *string, double scale) {
    char *result;
    char *pt;
    char *end;
    double val;

    if ( string==NULL )
return( NULL );

    while ( *string==' ' ) ++string;
    result = galloc(10*strlen(string)+1);
    if ( *string!='[' ) {
	val = strtod(string,&end);
	if ( end==string ) {
	    free( result );
return( NULL );
	}
	sprintf( result, "%g", rint(val*scale));
return( result );
    }

    pt = result;
    *pt++ = '[';
    ++string;
    while ( *string!='\0' && *string!=']' ) {
	val = strtod(string,&end);
	if ( end==string ) {
	    free(result);
return( NULL );
	}
	sprintf( pt, "%g ", rint(val*scale));
	pt += strlen(pt);
	string = end;
    }
    if ( pt[-1] == ' ' ) pt[-1] = ']';
    else *pt++ = ']';
    *pt = '\0';
return( result );
}

static void SFScalePrivate(SplineFont *sf,double scale) {
    static char *scalethese[] = {
	NULL
    };
    static char *integerscalethese[] = {
	"BlueValues", "OtherBlues",
	"FamilyBlues", "FamilyOtherBlues",
	"BlueShift", "BlueFuzz",
	"StdHW", "StdVW", "StemSnapH", "StemSnapV",
	NULL
    };
    int i;

    for ( i=0; integerscalethese[i]!=NULL; ++i ) {
	char *str = PSDictHasEntry(sf->private,integerscalethese[i]);
	char *new = iscaleString(str,scale);
	if ( new!=NULL )
	    PSDictChangeEntry(sf->private,integerscalethese[i],new);
	free(new);
    }
    for ( i=0; scalethese[i]!=NULL; ++i ) {
	char *str = PSDictHasEntry(sf->private,scalethese[i]);
	char *new = scaleString(str,scale);
	if ( new!=NULL )
	    PSDictChangeEntry(sf->private,scalethese[i],new);
	free(new);
    }
}

int SFScaleToEm(SplineFont *sf, int as, int des) {
    double scale;
    real transform[6];
    BVTFunc bvts;
    uint8 *oldselected = sf->fv->selected;

    scale = (as+des)/(double) (sf->ascent+sf->descent);
    sf->pfminfo.hhead_ascent = rint( sf->pfminfo.hhead_ascent * scale);
    sf->pfminfo.hhead_descent = rint( sf->pfminfo.hhead_descent * scale);
    sf->pfminfo.linegap = rint( sf->pfminfo.linegap * scale);
    sf->pfminfo.vlinegap = rint( sf->pfminfo.vlinegap * scale);
    sf->pfminfo.os2_winascent = rint( sf->pfminfo.os2_winascent * scale);
    sf->pfminfo.os2_windescent = rint( sf->pfminfo.os2_windescent * scale);
    sf->pfminfo.os2_typoascent = rint( sf->pfminfo.os2_typoascent * scale);
    sf->pfminfo.os2_typodescent = rint( sf->pfminfo.os2_typodescent * scale);
    sf->pfminfo.os2_typolinegap = rint( sf->pfminfo.os2_typolinegap * scale);

    sf->pfminfo.os2_subxsize = rint( sf->pfminfo.os2_subxsize * scale);
    sf->pfminfo.os2_subysize = rint( sf->pfminfo.os2_subysize * scale);
    sf->pfminfo.os2_subxoff = rint( sf->pfminfo.os2_subxoff * scale);
    sf->pfminfo.os2_subyoff = rint( sf->pfminfo.os2_subyoff * scale);
    sf->pfminfo.os2_supxsize = rint( sf->pfminfo.os2_supxsize * scale);
    sf->pfminfo.os2_supysize = rint(sf->pfminfo.os2_supysize *  scale);
    sf->pfminfo.os2_supxoff = rint( sf->pfminfo.os2_supxoff * scale);
    sf->pfminfo.os2_supyoff = rint( sf->pfminfo.os2_supyoff * scale);
    sf->pfminfo.os2_strikeysize = rint( sf->pfminfo.os2_strikeysize * scale);
    sf->pfminfo.os2_strikeypos = rint( sf->pfminfo.os2_strikeypos * scale);
    sf->upos *= scale;
    sf->uwidth *= scale;

    if ( sf->private!=NULL )
	SFScalePrivate(sf,scale);

    if ( as+des == sf->ascent+sf->descent ) {
	if ( as!=sf->ascent && des!=sf->descent ) {
	    sf->ascent = as; sf->descent = des;
	    sf->changed = true;
	}
return( false );
    }

    transform[0] = transform[3] = scale;
    transform[1] = transform[2] = transform[4] = transform[5] = 0;
    bvts.func = bvt_none;
    sf->fv->selected = galloc(sf->fv->map->enccount);
    memset(sf->fv->selected,1,sf->fv->map->enccount);

    sf->ascent = as; sf->descent = des;

    FVTransFunc(sf->fv,transform,0,&bvts,
	    fvt_dobackground|fvt_round_to_int|fvt_dontsetwidth|fvt_scalekernclasses|fvt_scalepstpos);
    free(sf->fv->selected);
    sf->fv->selected = oldselected;

    if ( !sf->changed ) {
	sf->changed = true;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	FVSetTitle(sf->fv);
#endif
    }
	
return( true );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void FVDoTransform(FontView *fv) {
    int flags=0x3;
    if ( FVAnyCharSelected(fv)==-1 )
return;
    if ( FVAllSelected(fv))
	flags = 0x7;
    TransformDlgCreate(fv,FVTransFunc,getorigin,flags,cvt_none);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuTransform(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Transform(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVDoTransform(fv);
}

#if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuPOV(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    struct pov_data pov_data;
    if ( FVAnyCharSelected(fv)==-1 || fv->sf->onlybitmaps )
return;
    if ( PointOfViewDlg(&pov_data,fv->sf,false)==-1 )
return;
    FVPointOfView(fv,&pov_data);
}

static void FVMenuNLTransform(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    if ( FVAnyCharSelected(fv)==-1 )
return;
    NonLinearDlg(fv,NULL);
}
#elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_PointOfView(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    struct pov_data pov_data;
    if ( FVAnyCharSelected(fv)==-1 || fv->sf->onlybitmaps )
return;
    if ( PointOfViewDlg(&pov_data,fv->sf,false)==-1 )
return;
    FVPointOfView(fv,&pov_data);
}

void FontViewMenu_NonLinearTransform(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    if ( FVAnyCharSelected(fv)==-1 || fv->sf->onlybitmaps )
return;
    NonLinearDlg(fv,NULL);
}
#endif

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuBitmaps(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    BitmapDlg(fv,NULL,mi->mid==MID_RemoveBitmaps?-1:(mi->mid==MID_AvailBitmaps) );
}
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_BitmapsAvail(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    BitmapDlg(fv,NULL,true );
}

void FontViewMenu_RegenBitmaps(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    BitmapDlg(fv,NULL,false );
}

void FontViewMenu_RemoveBitmaps(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    BitmapDlg(fv,NULL,-1 );
}
# endif

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuStroke(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_ExpandStroke(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVStroke(fv);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
#  ifdef FONTFORGE_CONFIG_TILEPATH
static void FVMenuTilePath(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    FVTile(fv);
}
#  endif
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_TilePath(GtkMenuItem *menuitem, gpointer user_data) {
#  ifdef FONTFORGE_CONFIG_TILEPATH
    FontView *fv = FV_From_MI(menuitem);
    FVTile(fv);
#  endif
}
# endif
#endif

static void FVOverlap(FontView *fv,enum overlap_type ot) {
    int i, cnt=0, layer, gid;
    SplineChar *sc;

    /* We know it's more likely that we'll find a problem in the overlap code */
    /*  than anywhere else, so let's save the current state against a crash */
    DoAutoSaves();

    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
		SCWorthOutputting(fv->sf->glyphs[gid]) )
	    ++cnt;

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_start_indicator(10,_("Removing overlaps..."),_("Removing overlaps..."),0,cnt,1);
# endif

    SFUntickAll(fv->sf);
    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 &&
	    SCWorthOutputting((sc=fv->sf->glyphs[gid])) &&
	    !sc->ticked ) {
	sc->ticked = true;
	if ( !SCRoundToCluster(sc,-2,false,.03,.12))
	    SCPreserveState(sc,false);
	MinimumDistancesFree(sc->md);
	sc->md = NULL;
	for ( layer = ly_fore; layer<sc->layer_cnt; ++layer )
	    sc->layers[layer].splines = SplineSetRemoveOverlap(sc,sc->layers[layer].splines,ot);
	SCCharChangedUpdate(sc);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	if ( !gwwv_progress_next())
    break;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_end_indicator();
# endif
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuOverlap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    if ( fv->sf->order2 || fv->sf->onlybitmaps )
return;

    /* We know it's more likely that we'll find a problem in the overlap code */
    /*  than anywhere else, so let's save the current state against a crash */
    DoAutoSaves();

    FVOverlap(fv,mi->mid==MID_RmOverlap ? over_remove :
		 mi->mid==MID_Intersection ? over_intersect :
		      over_findinter);
}
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_RemoveOverlap(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    /* We know it's more likely that we'll find a problem in the overlap code */
    /*  than anywhere else, so let's save the current state against a crash */
    DoAutoSaves();

    FVOverlap(fv,over_remove);
}

void FontViewMenu_Intersect(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    DoAutoSaves();
    FVOverlap(fv,over_intersect);
}

void FontViewMenu_FindInter(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    DoAutoSaves();
    FVOverlap(fv,over_findinter);
}
#endif

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuInline(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Inline(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    OutlineDlg(fv,NULL,NULL,true);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuOutline(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Outline(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    OutlineDlg(fv,NULL,NULL,false);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuShadow(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Shadow(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    ShadowDlg(fv,NULL,NULL,false);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuWireframe(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Wireframe(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    ShadowDlg(fv,NULL,NULL,true);
}
#endif

void _FVSimplify(FontView *fv,struct simplifyinfo *smpl) {
    int i, cnt=0, layer, gid;
    SplineChar *sc;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
		SCWorthOutputting(fv->sf->glyphs[gid]) )
	    ++cnt;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_start_indicator(10,_("Simplifying..."),_("Simplifying..."),0,cnt,1);
# endif

    SFUntickAll(fv->sf);
    for ( i=0; i<fv->map->enccount; ++i )
	if ( (gid=fv->map->map[i])!=-1 && SCWorthOutputting((sc=fv->sf->glyphs[gid])) &&
		fv->selected[i] && !sc->ticked ) {
	    sc->ticked = true;
	    SCPreserveState(sc,false);
	    for ( layer = ly_fore; layer<sc->layer_cnt; ++layer )
		sc->layers[layer].splines = SplineCharSimplify(sc,sc->layers[layer].splines,smpl);
	    SCCharChangedUpdate(sc);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	    if ( !gwwv_progress_next())
    break;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
	}
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_end_indicator();
# endif
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void FVSimplify(FontView *fv,int type) {
    static struct simplifyinfo smpls[] = {
	    { sf_normal },
	    { sf_normal,.75,.05,0,-1 },
	    { sf_normal,.75,.05,0,-1 }};
    struct simplifyinfo *smpl = &smpls[type+1];

    if ( smpl->linelenmax==-1 || (type==0 && !smpl->set_as_default)) {
	smpl->err = (fv->sf->ascent+fv->sf->descent)/1000.;
	smpl->linelenmax = (fv->sf->ascent+fv->sf->descent)/100.;
    }

    if ( type==1 ) {
	if ( !SimplifyDlg(fv->sf,smpl))
return;
	if ( smpl->set_as_default )
	    smpls[1] = *smpl;
    }
    _FVSimplify(fv,smpl);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuSimplify(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FVSimplify( (FontView *) GDrawGetUserData(gw),false );
}

static void FVMenuSimplifyMore(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FVSimplify( (FontView *) GDrawGetUserData(gw),true );
}

static void FVMenuCleanup(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FVSimplify( (FontView *) GDrawGetUserData(gw),-1 );
}
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Simplify(GtkMenuItem *menuitem, gpointer user_data) {
    FVSimplify( (FontView *) FV_From_MI(menuitem), false);
}

void FontViewMenu_SimplifyMore(GtkMenuItem *menuitem, gpointer user_data) {
    FVSimplify( (FontView *) FV_From_MI(menuitem), true);
}

void FontViewMenu_Cleanup(GtkMenuItem *menuitem, gpointer user_data) {
    FVSimplify( (FontView *) FV_From_MI(menuitem), -1);
}
#endif

static void FVCanonicalStart(FontView *fv) {
    int i, gid;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 )
	    SPLsStartToLeftmost(fv->sf->glyphs[gid]);
}

static void FVCanonicalContours(FontView *fv) {
    int i, gid;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 )
	    CanonicalContours(fv->sf->glyphs[gid]);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuCanonicalStart(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FVCanonicalStart( (FontView *) GDrawGetUserData(gw) );
}

static void FVMenuCanonicalContours(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FVCanonicalContours( (FontView *) GDrawGetUserData(gw) );
}
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_CanonicalStart(GtkMenuItem *menuitem, gpointer user_data) {
    FVCanonicalStart( (FontView *) FV_From_MI(menuitem));
}

void FontViewMenu_CanonicalContours(GtkMenuItem *menuitem, gpointer user_data) {
    FVCanonicalContours( (FontView *) FV_From_MI(menuitem));
}
#endif
#endif

static void FVAddExtrema(FontView *fv) {
    int i, cnt=0, layer, gid;
    SplineChar *sc;
    SplineFont *sf = fv->sf;
    int emsize = sf->ascent+sf->descent;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
		SCWorthOutputting(fv->sf->glyphs[gid]) )
	    ++cnt;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_start_indicator(10,_("Adding points at Extrema..."),_("Adding points at Extrema..."),0,cnt,1);
# endif

    SFUntickAll(fv->sf);
    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 &&
	    SCWorthOutputting((sc = fv->sf->glyphs[gid])) &&
	    !sc->ticked) {
	sc->ticked = true;
	SCPreserveState(sc,false);
	for ( layer = ly_fore; layer<sc->layer_cnt; ++layer )
	    SplineCharAddExtrema(sc,sc->layers[layer].splines,ae_only_good,emsize);
	SCCharChangedUpdate(sc);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	if ( !gwwv_progress_next())
    break;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_end_indicator();
# endif
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuAddExtrema(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FVAddExtrema( (FontView *) GDrawGetUserData(gw) );
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_AddExtrema(GtkMenuItem *menuitem, gpointer user_data) {
    FVAddExtrema( (FontView *) FV_From_MI(menuitem) );
# endif
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuCorrectDir(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_CorrectDir(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    int i, cnt=0, changed, refchanged, preserved, layer, gid;
    int askedall=-1, asked;
    RefChar *ref, *next;
    SplineChar *sc;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
		SCWorthOutputting(fv->sf->glyphs[gid]) )
	    ++cnt;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_start_indicator(10,_("Correcting Direction..."),_("Correcting Direction..."),0,cnt,1);
# endif

    SFUntickAll(fv->sf);
    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 && SCWorthOutputting((sc=fv->sf->glyphs[gid])) &&
	    !sc->ticked ) {
	sc->ticked = true;
	changed = refchanged = preserved = false;
	asked = askedall;
	for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
	    for ( ref=sc->layers[layer].refs; ref!=NULL; ref=next ) {
		next = ref->next;
		if ( ref->transform[0]*ref->transform[3]<0 ||
			(ref->transform[0]==0 && ref->transform[1]*ref->transform[2]>0)) {
		    if ( asked==-1 ) {
			char *buts[5];
#if defined(FONTFORGE_CONFIG_GDRAW)
			buts[2] = _("_Cancel");
#elif defined(FONTFORGE_CONFIG_GTK)
			buts[2] = GTK_STOCK_CANCEL;
#endif
			buts[0] = _("Unlink All");
			buts[1] = _("Unlink");
			buts[3] = NULL;
			asked = gwwv_ask(_("Flipped Reference"),(const char **) buts,0,2,_("%.50s contains a flipped reference. This cannot be corrected as is. Would you like me to unlink it and then correct it?"), sc->name );
			if ( asked==3 )
return;
			else if ( asked==2 )
	    break;
			else if ( asked==0 )
			    askedall = 0;
		    }
		    if ( asked==0 || asked==1 ) {
			if ( !preserved ) {
			    preserved = refchanged = true;
			    SCPreserveState(sc,false);
			}
			SCRefToSplines(sc,ref);
		    }
		}
	    }

	    if ( !preserved && sc->layers[layer].splines!=NULL ) {
		SCPreserveState(sc,false);
		preserved = true;
	    }
	    sc->layers[layer].splines = SplineSetsCorrect(sc->layers[layer].splines,&changed);
	}
	if ( changed || refchanged )
	    SCCharChangedUpdate(sc);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	if ( !gwwv_progress_next())
    break;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_end_indicator();
# endif
}
#endif

static void FVRound2Int(FontView *fv,real factor) {
    int i, cnt=0, gid;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
		SCWorthOutputting(fv->sf->glyphs[gid]) )
	    ++cnt;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_start_indicator(10,_("Rounding to integer..."),_("Rounding to integer..."),0,cnt,1);
# endif

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 && SCWorthOutputting(fv->sf->glyphs[gid]) ) {
	SplineChar *sc = fv->sf->glyphs[gid];

	SCPreserveState(sc,false);
	SCRound2Int( sc,factor);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	if ( !gwwv_progress_next())
    break;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_end_indicator();
# endif
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuRound2Int(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FVRound2Int( (FontView *) GDrawGetUserData(gw), 1.0 );
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_RoundToInt(GtkMenuItem *menuitem, gpointer user_data) {
    FVRound2Int( (FontView *) FV_From_MI(menuitem), 1.0);
# endif
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuRound2Hundredths(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FVRound2Int( (FontView *) GDrawGetUserData(gw),100.0 );
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_RoundToHundredths(GtkMenuItem *menuitem, gpointer user_data) {
    FVRound2Int( (FontView *) FV_From_MI(menuitem),100.0);
# endif
}

static void FVCluster(FontView *fv) {
    int i, cnt=0, gid;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
		SCWorthOutputting(fv->sf->glyphs[gid]) )
	    ++cnt;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_start_indicator(10,_("Rounding to integer..."),_("Rounding to integer..."),0,cnt,1);
# endif

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 && SCWorthOutputting(fv->sf->glyphs[gid]) ) {
	SCRoundToCluster(fv->sf->glyphs[gid],-2,false,.1,.5);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	if ( !gwwv_progress_next())
    break;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_end_indicator();
# endif
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuCluster(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FVCluster( (FontView *) GDrawGetUserData(gw));
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Cluster(GtkMenuItem *menuitem, gpointer user_data) {
    FVCluster( (FontView *) FV_From_MI(menuitem));
# endif
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuAutotrace(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Autotrace(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVAutoTrace(fv,e!=NULL && (e->u.mouse.state&ksm_shift));
}
#endif

void FVBuildAccent(FontView *fv,int onlyaccents) {
    int i, cnt=0, gid;
    SplineChar dummy;
    SplineChar *sc;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
		SCWorthOutputting(fv->sf->glyphs[gid]) )
	    ++cnt;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_start_indicator(10,_("Building accented glyphs"),
	    _("Building accented glyphs"),NULL,cnt,1);
# endif

    SFUntickAll(fv->sf);
    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] ) {
	gid = fv->map->map[i];
	sc = NULL;
	if ( gid!=-1 ) {
	    sc = fv->sf->glyphs[gid];
	    if ( sc!=NULL && sc->ticked )
    continue;
	}
	if ( sc==NULL )
	    sc = SCBuildDummy(&dummy,fv->sf,fv->map,i);
	else if ( !no_windowing_ui && sc->unicodeenc == 0x00c5 /* Aring */ &&
		sc->layers[ly_fore].splines!=NULL ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    char *buts[3];
	    buts[0] = _("_Yes");
	    buts[1] = _("_No");
	    buts[2] = NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
	    static char *buts[] = { GTK_STOCK_YES, GTK_STOCK_NO, NULL };
#endif
	    if ( gwwv_ask(U_("Replace Å"),(const char **) buts,0,1,U_("Are you sure you want to replace Å?\nThe ring will not join to the A."))==1 )
    continue;
	}
	if ( SFIsSomethingBuildable(fv->sf,sc,onlyaccents) ) {
	    sc = SFMakeChar(fv->sf,fv->map,i);
	    sc->ticked = true;
	    SCBuildComposit(fv->sf,sc,!onlycopydisplayed,fv);
	}
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	if ( !gwwv_progress_next())
    break;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_end_indicator();
# endif
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuBuildAccent(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FVBuildAccent( (FontView *) GDrawGetUserData(gw), true );
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_BuildAccent(GtkMenuItem *menuitem, gpointer user_data) {
    FVBuildAccent( (FontView *) FV_From_MI(menuitem), true );
# endif
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuBuildComposite(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FVBuildAccent( (FontView *) GDrawGetUserData(gw), false );
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_BuildComposite(GtkMenuItem *menuitem, gpointer user_data) {
    FVBuildAccent( (FontView *) FV_From_MI(menuitem), false );
# endif
}
#endif

static int SFIsDuplicatable(SplineFont *sf, SplineChar *sc) {
    extern const int cns14pua[], amspua[];
    const int *pua = sf->uni_interp==ui_trad_chinese ? cns14pua : sf->uni_interp==ui_ams ? amspua : NULL;
    int baseuni = 0;
    const unichar_t *pt;

    if ( pua!=NULL && sc->unicodeenc>=0xe000 && sc->unicodeenc<=0xf8ff )
	baseuni = pua[sc->unicodeenc-0xe000];
    if ( baseuni==0 && ( pt = SFGetAlternate(sf,sc->unicodeenc,sc,false))!=NULL &&
	    pt[0]!='\0' && pt[1]=='\0' )
	baseuni = pt[0];
    if ( baseuni!=0 && SFGetChar(sf,baseuni,NULL)!=NULL )
return( true );

return( false );
}

void FVBuildDuplicate(FontView *fv) {
    extern const int cns14pua[], amspua[];
    const int *pua = fv->sf->uni_interp==ui_trad_chinese ? cns14pua : fv->sf->uni_interp==ui_ams ? amspua : NULL;
    int i, cnt=0, gid;
    SplineChar dummy;
    const unichar_t *pt;

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] )
	++cnt;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_start_indicator(10,_("Building duplicate encodings"),
	_("Building duplicate encodings"),NULL,cnt,1);
# endif

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] ) {
	SplineChar *sc;
	int baseuni = 0;
	if ( (gid = fv->map->map[i])==-1 || (sc = fv->sf->glyphs[gid])==NULL )
	    sc = SCBuildDummy(&dummy,fv->sf,fv->map,i);
	if ( pua!=NULL && sc->unicodeenc>=0xe000 && sc->unicodeenc<=0xf8ff )
	    baseuni = pua[sc->unicodeenc-0xe000];
	if ( baseuni==0 && ( pt = SFGetAlternate(fv->sf,sc->unicodeenc,sc,false))!=NULL &&
		pt[0]!='\0' && pt[1]=='\0' )
	    baseuni = pt[0];
	
	if ( baseuni!=0 && (gid = SFFindExistingSlot(fv->sf,baseuni,NULL))!=-1 )
	    LinkEncToGid(fv,i,gid);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	if ( !gwwv_progress_next())
    break;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_end_indicator();
# endif
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuBuildDuplicate(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FVBuildDuplicate( (FontView *) GDrawGetUserData(gw));
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_BuildDuplicatem(GtkMenuItem *menuitem, gpointer user_data) {
    FVBuildDuplicate( (FontView *) FV_From_MI(menuitem));
# endif
}
#endif

#ifdef KOREAN
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuShowGroup(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    ShowGroup( ((FontView *) GDrawGetUserData(gw))->sf );
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_ShowGroup(GtkMenuItem *menuitem, gpointer user_data) {
    ShowGroup( ((FontView *) FV_From_MI(menuitem))->sf );
# endif
}
#endif
#endif

#if HANYANG
static void FVMenuModifyComposition(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    if ( fv->sf->rules!=NULL )
	SFModifyComposition(fv->sf);
}

static void FVMenuBuildSyllables(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    if ( fv->sf->rules!=NULL )
	SFBuildSyllables(fv->sf);
}
#endif

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuCompareFonts(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_CompareFonts(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FontCompareDlg(fv);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuMergeFonts(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_MergeFonts(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVMergeFonts(fv);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuInterpFonts(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Interpolate(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVInterpolateFonts(fv);
}

static void FVShowInfo(FontView *fv);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

void FVChangeChar(FontView *fv,int i) {

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( i!=-1 ) {
	FVDeselectAll(fv);
	fv->selected[i] = true;
	fv->sel_index = 1;
	fv->end_pos = fv->pressed_pos = i;
	FVToggleCharSelected(fv,i);
	FVScrollToChar(fv,i);
	FVShowInfo(fv);
    }
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
void FVScrollToChar(FontView *fv,int i) {

    if ( fv->v==NULL || fv->colcnt==0 )	/* Can happen in scripts */
return;

    if ( i!=-1 ) {
	if ( i/fv->colcnt<fv->rowoff || i/fv->colcnt >= fv->rowoff+fv->rowcnt ) {
	    fv->rowoff = i/fv->colcnt;
	    if ( fv->rowcnt>= 3 )
		--fv->rowoff;
	    if ( fv->rowoff+fv->rowcnt>=fv->rowltot )
		fv->rowoff = fv->rowltot-fv->rowcnt;
	    if ( fv->rowoff<0 ) fv->rowoff = 0;
	    GScrollBarSetPos(fv->vsb,fv->rowoff);
	    GDrawRequestExpose(fv->v,NULL,false);
	}
    }
}

static void _FVMenuChangeChar(FontView *fv,int mid ) {
    SplineFont *sf = fv->sf;
    EncMap *map = fv->map;
    int pos = FVAnyCharSelected(fv);

    if ( pos>=0 ) {
	if ( mid==MID_Next )
	    ++pos;
	else if ( mid==MID_Prev )
	    --pos;
	else if ( mid==MID_NextDef ) {
	    for ( ++pos; pos<map->enccount &&
		    (map->map[pos]==-1 || !SCWorthOutputting(sf->glyphs[map->map[pos]]) ||
			(fv->show!=fv->filled && fv->show->glyphs[map->map[pos]]==NULL ));
		    ++pos );
	    if ( pos>=map->enccount ) {
		int selpos = FVAnyCharSelected(fv);
		char *iconv_name = map->enc->iconv_name ? map->enc->iconv_name :
			map->enc->enc_name;
		if ( strstr(iconv_name,"2022")!=NULL && selpos<0x2121 )
		    pos = 0x2121;
		else if ( strstr(iconv_name,"EUC")!=NULL && selpos<0xa1a1 )
		    pos = 0xa1a1;
		else if ( map->enc->is_tradchinese ) {
		    if ( strstrmatch(map->enc->enc_name,"HK")!=NULL &&
			    selpos<0x8140 )
			pos = 0x8140;
		    else
			pos = 0xa140;
		} else if ( map->enc->is_japanese ) {
		    if ( strstrmatch(iconv_name,"SJIS")!=NULL ||
			    (strstrmatch(iconv_name,"JIS")!=NULL && strstrmatch(iconv_name,"SHIFT")!=NULL )) {
			if ( selpos<0x8100 )
			    pos = 0x8100;
			else if ( selpos<0xb000 )
			    pos = 0xb000;
		    }
		} else if ( map->enc->is_korean ) {
		    if ( strstrmatch(iconv_name,"JOHAB")!=NULL ) {
			if ( selpos<0x8431 )
			    pos = 0x8431;
		    } else {	/* Wansung, EUC-KR */
			if ( selpos<0xa1a1 )
			    pos = 0xa1a1;
		    }
		} else if ( map->enc->is_simplechinese ) {
		    if ( strmatch(iconv_name,"EUC-CN")==0 && selpos<0xa1a1 )
			pos = 0xa1a1;
		}
		if ( pos>=map->enccount )
return;
	    }
	} else if ( mid==MID_PrevDef ) {
	    for ( --pos; pos>=0 &&
		    (map->map[pos]==-1 || !SCWorthOutputting(sf->glyphs[map->map[pos]]) ||
			(fv->show!=fv->filled && fv->show->glyphs[map->map[pos]]==NULL ));
		    --pos );
	    if ( pos<0 )
return;
	}
    }
    if ( pos<0 ) pos = map->enccount-1;
    else if ( pos>= map->enccount ) pos = 0;
    if ( pos>=0 && pos<map->enccount )
	FVChangeChar(fv,pos);
}

static void FVMenuChangeChar(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    _FVMenuChangeChar(fv,mi->mid);
}

static void CIDSetEncMap(FontView *fv, SplineFont *new ) {
    int gcnt = new->glyphcnt;

    if ( fv->cidmaster!=NULL && gcnt!=fv->sf->glyphcnt ) {
	int i;
	if ( fv->map->encmax<gcnt ) {
	    fv->map->map = grealloc(fv->map->map,gcnt*sizeof(int));
	    fv->map->backmap = grealloc(fv->map->backmap,gcnt*sizeof(int));
	    fv->map->backmax = fv->map->encmax = gcnt;
	}
	for ( i=0; i<gcnt; ++i )
	    fv->map->map[i] = fv->map->backmap[i] = i;
	if ( gcnt<fv->map->enccount )
	    memset(fv->selected+gcnt,0,fv->map->enccount-gcnt);
	else {
	    free(fv->selected);
	    fv->selected = gcalloc(gcnt,sizeof(char));
	}
	fv->map->enccount = gcnt;
    }
    fv->sf = new;
    new->fv = fv;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    FVSetTitle(fv);
    FontViewReformatOne(fv);
#endif
}

static void FVShowSubFont(FontView *fv,SplineFont *new) {
    MetricsView *mv, *mvnext;
    BDFFont *newbdf;
    int wascompact = fv->normal!=NULL;

    for ( mv=fv->sf->metrics; mv!=NULL; mv = mvnext ) {
	/* Don't bother trying to fix up metrics views, just not worth it */
	mvnext = mv->next;
	GDrawDestroyWindow(mv->gw);
    }
    if ( wascompact ) {
	EncMapFree(fv->map);
	fv->map = fv->normal;
	fv->normal = NULL;
	fv->selected = grealloc(fv->selected,fv->map->enccount);
	memset(fv->selected,0,fv->map->enccount);
    }
    CIDSetEncMap(fv,new);
    if ( wascompact ) {
	fv->normal = EncMapCopy(fv->map);
	CompactEncMap(fv->map,fv->sf);
	FontViewReformatOne(fv);
	FVSetTitle(fv);
    }
    newbdf = SplineFontPieceMeal(fv->sf,fv->filled->pixelsize,
	    (fv->antialias?pf_antialias:0)|(fv->bbsized?pf_bbsized:0)|
		(use_freetype_to_rasterize_fv && !fv->sf->strokedfont && !fv->sf->multilayer?pf_ft_nohints:0),
	    NULL);
    BDFFontFree(fv->filled);
    if ( fv->filled == fv->show )
	fv->show = newbdf;
    fv->filled = newbdf;
    GDrawRequestExpose(fv->v,NULL,true);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuGotoChar(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Goto(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    int pos = GotoChar(fv->sf,fv->map);
    if ( fv->cidmaster!=NULL && pos!=-1 && !fv->map->enc->is_compact ) {
	SplineFont *cidmaster = fv->cidmaster;
	int k, hadk= cidmaster->subfontcnt;
	for ( k=0; k<cidmaster->subfontcnt; ++k ) {
	    SplineFont *sf = cidmaster->subfonts[k];
	    if ( pos<sf->glyphcnt && sf->glyphs[pos]!=NULL )
	break;
	    if ( pos<sf->glyphcnt )
		hadk = k;
	}
	if ( k==cidmaster->subfontcnt && pos>=fv->sf->glyphcnt )
	    k = hadk;
	if ( k!=cidmaster->subfontcnt && cidmaster->subfonts[k] != fv->sf )
	    FVShowSubFont(fv,cidmaster->subfonts[k]);
	if ( pos>=fv->sf->glyphcnt )
	    pos = -1;
    }
    FVChangeChar(fv,pos);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuLigatures(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Ligatures(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    SFShowLigatures(fv->sf,NULL);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuKernPairs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_KernPairs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    SFKernClassTempDecompose(fv->sf,false);
    SFShowKernPairs(fv->sf,NULL,NULL);
    SFKernCleanup(fv->sf,false);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuAnchorPairs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_AnchorPairs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    SFShowKernPairs(fv->sf,NULL,mi->ti.userdata);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuShowAtt(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_ShowAtt(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    ShowAtt(fv->sf);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuDisplaySubs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_DisplaySubstitutions(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    if ( fv->cur_subtable!=0 ) {
	fv->cur_subtable = NULL;
    } else {
	SplineFont *sf = fv->sf;
	OTLookup *otf;
	struct lookup_subtable *sub;
	int cnt, k;
	char **names = NULL;
	if ( sf->cidmaster ) sf=sf->cidmaster;
	for ( k=0; k<2; ++k ) {
	    cnt = 0;
	    for ( otf = sf->gsub_lookups; otf!=NULL; otf=otf->next ) {
		if ( otf->lookup_type==gsub_single ) {
		    for ( sub=otf->subtables; sub!=NULL; sub=sub->next ) {
			if ( names )
			    names[cnt] = sub->subtable_name;
			++cnt;
		    }
		}
	    }
	    if ( cnt==0 )
	break;
	    if ( names==NULL )
		names = galloc((cnt+3) * sizeof(char *));
	    else {
		names[cnt++] = "-";
		names[cnt++] = _("New Lookup Subtable...");
		names[cnt] = NULL;
	    }
	}
	sub = NULL;
	if ( names!=NULL ) {
	    int ret = gwwv_choose(_("Display Substitution..."), (const char **) names, cnt, 0,
		    _("Pick a substitution to display in the window."));
	    if ( ret!=-1 )
		sub = SFFindLookupSubtable(sf,names[ret]);
	    free(names);
	    if ( ret==-1 )
return;
	}
	if ( sub==NULL )
	    sub = SFNewLookupSubtableOfType(sf,gsub_single,NULL);
	if ( sub!=NULL )
	    fv->cur_subtable = sub;
    }
    GDrawRequestExpose(fv->v,NULL,false);
}

static void FVChangeDisplayFont(FontView *fv,BDFFont *bdf) {
    int samesize=0;
    int rcnt, ccnt;
    int oldr, oldc;
    int first_time = fv->show==NULL;

    if ( fv->v==NULL )			/* Can happen in scripts */
return;

    if ( fv->show!=bdf ) {
	oldc = fv->cbw*fv->colcnt;
	oldr = fv->cbh*fv->rowcnt;

	fv->show = bdf;
	if ( fv->user_requested_magnify!=-1 )
	    fv->magnify=fv->user_requested_magnify;
	else if ( bdf->pixelsize<20 ) {
	    if ( bdf->pixelsize<=9 )
		fv->magnify = 3;
	    else
		fv->magnify = 2;
	    samesize = ( fv->show && fv->cbw == (bdf->pixelsize*fv->magnify)+1 );
	} else
	    fv->magnify = 1;
	if ( !first_time && fv->cbw == fv->magnify*bdf->pixelsize+1 )
	    samesize = true;
	fv->cbw = (bdf->pixelsize*fv->magnify)+1;
	fv->cbh = (bdf->pixelsize*fv->magnify)+1+fv->lab_height+1;
	fv->resize_expected = !samesize;
	ccnt = fv->sf->desired_col_cnt;
	rcnt = fv->sf->desired_row_cnt;
	if ((( bdf->pixelsize<=fv->sf->display_size || bdf->pixelsize<=-fv->sf->display_size ) &&
		 fv->sf->top_enc!=-1 /* Not defaulting */ ) ||
		bdf->pixelsize<=48 ) {
	    /* use the desired sizes */
	} else {
	    if ( bdf->pixelsize>48 ) {
		ccnt = 8;
		rcnt = 2;
	    } else if ( bdf->pixelsize>=96 ) {
		ccnt = 4;
		rcnt = 1;
	    }
	    if ( !first_time ) {
		if ( ccnt < oldc/fv->cbw )
		    ccnt = oldc/fv->cbw;
		if ( rcnt < oldr/fv->cbh )
		    rcnt = oldr/fv->cbh;
	    }
	}
#if defined(FONTFORGE_CONFIG_GTK)
	if ( samesize ) {
	    gtk_widget_queue_draw(fv->v);
	} else {
	    gtk_widget_set_size_request(fv->v,ccnt*fv->cbw+1,rcnt*fv->cbh+1);
	    /* If we try to make the window smaller, we must force a resize */
	    /*  of the whole system, else the size will remain the same */
	    /* Also when we get a resize event we must set the widget size */
	    /*  to be minimal so that the user can resize */
	    if (( ccnt*fv->cbw < oldc || rcnt*fv->cbh < oldr ) && !first_time )
		gtk_window_resize( GTK_WINDOW(widget), 1,1 );
	}
#elif defined(FONTFORGE_CONFIG_GDRAW)
	if ( samesize ) {
	    GDrawRequestExpose(fv->v,NULL,false);
	} else {
	    GDrawResize(fv->gw,
		    ccnt*fv->cbw+1+GDrawPointsToPixels(fv->gw,_GScrollBar_Width),
		    rcnt*fv->cbh+1+fv->mbh+fv->infoh);
	}
#endif
    }
}

# if defined(FONTFORGE_CONFIG_GDRAW)
struct md_data {
    int done;
    int ish;
    FontView *fv;
};

static int md_e_h(GWindow gw, GEvent *e) {
    if ( e->type==et_close ) {
	struct md_data *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct md_data *d = GDrawGetUserData(gw);
	static int masks[] = { fvm_baseline, fvm_origin, fvm_advanceat, fvm_advanceto, -1 };
	int i, metrics;
	if ( GGadgetGetCid(e->u.control.g)==10 ) {
	    metrics = 0;
	    for ( i=0; masks[i]!=-1 ; ++i )
		if ( GGadgetIsChecked(GWidgetGetControl(gw,masks[i])))
		    metrics |= masks[i];
	    if ( d->ish )
		default_fv_showhmetrics = d->fv->showhmetrics = metrics;
	    else
		default_fv_showvmetrics = d->fv->showvmetrics = metrics;
	}
	d->done = true;
    } else if ( e->type==et_char ) {
#if 0
	if ( e->u.chr.keysym == GK_F1 || e->u.chr.keysym == GK_Help ) {
	    help("fontinfo.html");
return( true );
	}
#endif
return( false );
    }
return( true );
}

static void FVMenuShowMetrics(GWindow fvgw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(fvgw);
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    struct md_data d;
    GGadgetCreateData gcd[7];
    GTextInfo label[6];
    int metrics = mi->mid==MID_ShowHMetrics ? fv->showhmetrics : fv->showvmetrics;

    d.fv = fv;
    d.done = 0;
    d.ish = mi->mid==MID_ShowHMetrics;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = d.ish?_("Show H. Metrics..."):_("Show V. Metrics...");
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(170));
    pos.height = GDrawPointsToPixels(NULL,130);
    gw = GDrawCreateTopWindow(NULL,&pos,md_e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _("Baseline");
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 8; gcd[0].gd.pos.y = 8; 
    gcd[0].gd.flags = gg_enabled|gg_visible|(metrics&fvm_baseline?gg_cb_on:0);
    gcd[0].gd.cid = fvm_baseline;
    gcd[0].creator = GCheckBoxCreate;

    label[1].text = (unichar_t *) _("Origin");
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 8; gcd[1].gd.pos.y = gcd[0].gd.pos.y+16;
    gcd[1].gd.flags = gg_enabled|gg_visible|(metrics&fvm_origin?gg_cb_on:0);
    gcd[1].gd.cid = fvm_origin;
    gcd[1].creator = GCheckBoxCreate;

    label[2].text = (unichar_t *) _("Advance Width as a Line");
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 8; gcd[2].gd.pos.y = gcd[1].gd.pos.y+16;
    gcd[2].gd.flags = gg_enabled|gg_visible|gg_utf8_popup|(metrics&fvm_advanceat?gg_cb_on:0);
    gcd[2].gd.cid = fvm_advanceat;
    gcd[2].gd.popup_msg = (unichar_t *) _("Display the advance width as a line\nperpendicular to the advance direction");
    gcd[2].creator = GCheckBoxCreate;

    label[3].text = (unichar_t *) _("Advance Width as a Bar");
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = 8; gcd[3].gd.pos.y = gcd[2].gd.pos.y+16; 
    gcd[3].gd.flags = gg_enabled|gg_visible|gg_utf8_popup|(metrics&fvm_advanceto?gg_cb_on:0);
    gcd[3].gd.cid = fvm_advanceto;
    gcd[3].gd.popup_msg = (unichar_t *) _("Display the advance width as a bar under the glyph\nshowing the extent of the advance");
    gcd[3].creator = GCheckBoxCreate;

    label[4].text = (unichar_t *) _("_OK");
    label[4].text_is_1byte = true;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.pos.x = 20-3; gcd[4].gd.pos.y = GDrawPixelsToPoints(NULL,pos.height)-35-3;
    gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
    gcd[4].gd.cid = 10;
    gcd[4].creator = GButtonCreate;

    label[5].text = (unichar_t *) _("_Cancel");
    label[5].text_is_1byte = true;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.pos.x = -20; gcd[5].gd.pos.y = gcd[4].gd.pos.y+3;
    gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    gcd[5].creator = GButtonCreate;

    GGadgetsCreate(gw,gcd);
    
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);

    SavePrefs();
    GDrawRequestExpose(fv->v,NULL,false);
}
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_ShowMetrics(GtkMenuItem *menuitem, gpointer user_data) {
    GtkWidget *dlg = create_FontViewShowMetrics();
    int isv = strcmp(gtk_widget_get_name(menuitem),"show_v_metrics1")==0;
    int metrics = !isv ? fv->showhmetrics : fv->showvmetrics;

    if ( isv )
	gtk_window_set_title(GTK_WINDOW(dlg), _("Show Vertical Metrics"));
    gtk_toggle_button_set_active(lookup_widget(dlg,"baseline"),
	    (metrics&fvm_baseline)?true:false);
    gtk_toggle_button_set_active(lookup_widget(dlg,"origin"),
	    (metrics&fvm_origin)?true:false);
    gtk_toggle_button_set_active(lookup_widget(dlg,"AWBar"),
	    (metrics&fvm_advanceto)?true:false);
    gtk_toggle_button_set_active(lookup_widget(dlg,"AWLine"),
	    (metrics&fvm_advanceat)?true:false);
    if ( gtk_dialog_run( GTK_DIALOG (dlg)) == GTK_RESPONSE_ACCEPT ) {
	metrics = 0;
	if ( gtk_toggle_button_get_active(lookup_widget(dlg,"baseline")) )
	    metrics |= fvm_baseline;
	if ( gtk_toggle_button_get_active(lookup_widget(dlg,"origin")) )
	    metrics |= fvm_origin;
	if ( gtk_toggle_button_get_active(lookup_widget(dlg,"AWBar")) )
	    metrics |= fvm_advanceto;
	if ( gtk_toggle_button_get_active(lookup_widget(dlg,"AWLine")) )
	    metrics |= fvm_advanceat;
	if ( isv )
	    fv->showvmetrics = metrics;
	else
	    fv->showhmetrics = metrics;
    }
    gtk_widget_destroy( dlg );
}
#endif

void FVChangeDisplayBitmap(FontView *fv,BDFFont *bdf) {
    FVChangeDisplayFont(fv,bdf);
    fv->sf->display_size = fv->show->pixelsize;
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuSize(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw), *fvs, *fvss;
    int dspsize = fv->filled->pixelsize;
    int changedmodifier = false;

    fv->magnify = 1;
    fv->user_requested_magnify = -1;
    if ( mi->mid == MID_24 )
	default_fv_font_size = dspsize = 24;
    else if ( mi->mid == MID_36 )
	default_fv_font_size = dspsize = 36;
    else if ( mi->mid == MID_48 )
	default_fv_font_size = dspsize = 48;
    else if ( mi->mid == MID_72 )
	default_fv_font_size = dspsize = 72;
    else if ( mi->mid == MID_96 )
	default_fv_font_size = dspsize = 96;
    else if ( mi->mid == MID_FitToEm ) {
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_PixelSize(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int dspsize = fv->filled->pixelsize;
    int changedmodifier = false;
    G_CONST_RETURN gchar *name = gtk_widget_get_name(menuitem);

    fv->magnify = 1;
    fv->user_requested_magnify = -1;
    if ( strstr(name,"24")!=NULL )
	default_fv_font_size = dspsize = 24;
    else if ( strstr(name,"36")!=NULL )
	default_fv_font_size = dspsize = 36;
    else if ( strstr(name,"48")!=NULL )
	default_fv_font_size = dspsize = 48;
    else if ( strstr(name,"72")!=NULL )
	default_fv_font_size = dspsize = 72;
    else if ( strstr(name,"96")!=NULL )
	default_fv_font_size = dspsize = 96;
    else if ( strcmp(name,"fit_to_em1")==0 ) {
# endif
	default_fv_bbsized = fv->bbsized = !fv->bbsized;
	fv->sf->display_bbsized = fv->bbsized;
	changedmodifier = true;
    } else {
	default_fv_antialias = fv->antialias = !fv->antialias;
	fv->sf->display_antialias = fv->antialias;
	changedmodifier = true;
    }

    SavePrefs();
    if ( fv->filled!=fv->show || fv->filled->pixelsize != dspsize || changedmodifier ) {
	BDFFont *new, *old;
	for ( fvs=fv->sf->fv; fvs!=NULL; fvs=fvs->nextsame )
	    fvs->touched = false;
	while ( 1 ) {
	    for ( fvs=fv->sf->fv; fvs!=NULL; fvs=fvs->nextsame )
		if ( !fvs->touched )
	    break;
	    if ( fvs==NULL )
	break;
	    old = fvs->filled;
	    new = SplineFontPieceMeal(fvs->sf,dspsize,
		(fvs->antialias?pf_antialias:0)|(fvs->bbsized?pf_bbsized:0)|
		    (use_freetype_to_rasterize_fv && !fvs->sf->strokedfont && !fvs->sf->multilayer?pf_ft_nohints:0),
		NULL);
	    for ( fvss=fvs; fvss!=NULL; fvss = fvss->nextsame ) {
		if ( fvss->filled==old ) {
		    fvss->filled = new;
		    fvss->antialias = fvs->antialias;
		    fvss->bbsized = fvs->bbsized;
		    if ( fvss->show==old || fvss==fv )
			FVChangeDisplayFont(fvss,new);
		    fvss->touched = true;
		}
	    }
	    BDFFontFree(old);
	}
	fv->sf->display_size = -dspsize;
	if ( fv->cidmaster!=NULL ) {
	    int i;
	    for ( i=0; i<fv->cidmaster->subfontcnt; ++i )
		fv->cidmaster->subfonts[i]->display_size = -dspsize;
	}
    }
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuMagnify(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int magnify = fv->user_requested_magnify!=-1 ? fv->user_requested_magnify : fv->magnify;
    char def[20], *end, *ret;
    int val;
    BDFFont *show = fv->show;

    sprintf( def, "%d", magnify );
    ret = gwwv_ask_string(_("Bitmap Magnification..."),def,_("Please specify a bitmap magnification factor."));
    if ( ret==NULL )
return;
    val = strtol(ret,&end,10);
    if ( val<1 || val>5 || *end!='\0' )
	gwwv_post_error( _("Bad Number"),_("Bad Number") );
    else {
	fv->user_requested_magnify = val;
	fv->show = fv->filled;
	FVChangeDisplayFont(fv,show);
    }
    free(ret);
}
#endif

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuWSize(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int h,v;
    extern int default_fv_col_count, default_fv_row_count;

    if ( mi->mid == MID_32x8 ) {
	h = 32; v=8;
    } else if ( mi->mid == MID_16x4 ) {
	h = 16; v=4;
    } else {
	h = 8; v=2;
    }
    GDrawResize(fv->gw,
	    h*fv->cbw+1+GDrawPointsToPixels(fv->gw,_GScrollBar_Width),
	    v*fv->cbh+1+fv->mbh+fv->infoh);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_WindowSize(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    G_CONST_RETURN gchar *name = gtk_widget_get_name(menuitem);
    int h=8,v=2;
    extern int default_fv_col_count, default_fv_row_count;

    sscanf(name,"%dx%d", &h, &v );
    gtk_widget_set_size_request(fv->v,h*fv->cbw+1,v*fv->cbh+1);
# endif
    fv->sf->desired_col_cnt = default_fv_col_count = h;
    fv->sf->desired_row_cnt = default_fv_row_count = v;

    SavePrefs();
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuGlyphLabel(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    default_fv_glyphlabel = fv->glyphlabel = mi->mid;

    GDrawRequestExpose(fv->v,NULL,false);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_GlyphLabel(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    G_CONST_RETURN gchar *name = gtk_widget_get_name(menuitem);

    if ( strstr(name,"glyph_image")!=NULL )
	default_fv_glyphlabel = fv->glyphlabel = gl_glyph;
    else if ( strstr(name,"glyph_name")!=NULL )
	default_fv_glyphlabel = fv->glyphlabel = gl_name;
    else if ( strstr(name,"unicode")!=NULL )
	default_fv_glyphlabel = fv->glyphlabel = gl_unicode;
    else if ( strstr(name,"encoding")!=NULL )
	default_fv_glyphlabel = fv->glyphlabel = gl_encoding;

    gtk_widget_queue_draw(fv->v);
# endif

    SavePrefs();
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuShowBitmap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    BDFFont *bdf = mi->ti.userdata;
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_ShowBitmap(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    BDFFont *bdf = user_data;
# endif

    FVChangeDisplayBitmap(fv,bdf);		/* Let's not change any of the others */
}

void FVShowFilled(FontView *fv) {
    FontView *fvs;

    fv->magnify = 1;
    fv->user_requested_magnify = 1;
    for ( fvs=fv->sf->fv; fvs!=NULL; fvs=fvs->nextsame )
	if ( fvs->show!=fvs->filled )
	    FVChangeDisplayFont(fvs,fvs->filled);
    fv->sf->display_size = -fv->filled->pixelsize;
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

void FVMetricsCenter(FontView *fv,int docenter) {
    int i, gid;
    SplineChar *sc;
    DBounds bb;
    IBounds ib;
    real transform[6], itransform[6];
    BVTFunc bvts[2];
    BDFFont *bdf;

    memset(transform,0,sizeof(transform));
    memset(itransform,0,sizeof(itransform));
    transform[0] = transform[3] = 1.0;
    itransform[0] = itransform[3] = 1.0;
    itransform[2] = tan( fv->sf->italicangle * 3.1415926535897932/180.0 );
    bvts[1].func = bvt_none;
    bvts[0].func = bvt_transmove; bvts[0].y = 0;
    if ( !fv->sf->onlybitmaps ) {
	for ( i=0; i<fv->map->enccount; ++i ) {
	    if ( fv->selected[i] && (gid=fv->map->map[i])!=-1 &&
		    (sc = fv->sf->glyphs[gid])!=NULL ) {
		if ( itransform[2] == 0 )
		    SplineCharFindBounds(sc,&bb);
		else {
		    SplineSet *base, *temp;
		    base = LayerAllSplines(&sc->layers[ly_fore]);
		    temp = SplinePointListTransform(SplinePointListCopy(base),itransform,true);
		    LayerUnAllSplines(&sc->layers[ly_fore]);
		    SplineSetFindBounds(temp,&bb);
		    SplinePointListsFree(temp);
		}
		if ( docenter )
		    transform[4] = (sc->width-(bb.maxx-bb.minx))/2 - bb.minx;
		else
		    transform[4] = (sc->width-(bb.maxx-bb.minx))/3 - bb.minx;
		if ( transform[4]!=0 ) {
		    FVTrans(fv,sc,transform,NULL,fvt_dontmovewidth);
		    bvts[0].x = transform[4];
		    for ( bdf = fv->sf->bitmaps; bdf!=NULL; bdf=bdf->next )
			if ( gid<bdf->glyphcnt && bdf->glyphs[gid]!=NULL )
			    BCTrans(bdf,bdf->glyphs[gid],bvts,fv);
		}
	    }
	}
    } else {
	double scale = (fv->sf->ascent+fv->sf->descent)/(double) (fv->show->pixelsize);
	for ( i=0; i<fv->map->enccount; ++i ) {
	    if ( fv->selected[i] && (gid=fv->map->map[i])!=-1 &&
		    fv->sf->glyphs[gid]!=NULL ) {
		BDFChar *bc = fv->show->glyphs[gid];
		if ( bc==NULL ) bc = BDFMakeChar(fv->show,fv->map,i);
		BDFCharFindBounds(bc,&ib);
		if ( docenter )
		    transform[4] = scale * ((bc->width-(ib.maxx-ib.minx))/2 - ib.minx);
		else
		    transform[4] = scale * ((bc->width-(ib.maxx-ib.minx))/3 - ib.minx);
		if ( transform[4]!=0 ) {
		    FVTrans(fv,fv->sf->glyphs[gid],transform,NULL,fvt_dontmovewidth);
		    bvts[0].x = transform[4];
		    for ( bdf = fv->sf->bitmaps; bdf!=NULL; bdf=bdf->next )
			if ( gid<bdf->glyphcnt && bdf->glyphs[gid]!=NULL )
			    BCTrans(bdf,bdf->glyphs[gid],bvts,fv);
		}
	    }
	}
    }
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuCenter(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Center(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVMetricsCenter(fv,mi->mid==MID_Center);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuSetWidth(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    if ( FVAnyCharSelected(fv)==-1 )
return;
    if ( mi->mid == MID_SetVWidth && !fv->sf->hasvmetrics )
return;
    FVSetWidth(fv,mi->mid==MID_SetWidth   ?wt_width:
		  mi->mid==MID_SetLBearing?wt_lbearing:
		  mi->mid==MID_SetRBearing?wt_rbearing:
		  wt_vwidth);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Setwidth(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    G_CONST_RETURN gchar *name = gtk_widget_get_name(menuitem);

    if ( FVAnyCharSelected(fv)==-1 )
return;
    if ( strcmp(name,"set_vertical_advance1")==0 && !fv->sf->hasvmetrics )
return;
    FVSetWidth(fv,strcmp(name,"set_width1")==0   ?wt_width:
		  strcmp(name,"set_lbearing1")==0?wt_lbearing:
		  strcmp(name,"set_rbearing1")==0?wt_rbearing:
		  wt_vwidth);
# endif
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuAutoWidth(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_AutoWidth(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    FVAutoWidth(fv);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuAutoKern(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_AutoKern(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    FVAutoKern(fv);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuKernByClasses(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_KernClasses(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    ShowKernClasses(fv->sf,NULL,false);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuVKernByClasses(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_VKernClasses(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    ShowKernClasses(fv->sf,NULL,true);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuRemoveKern(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_RemoveKP(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    FVRemoveKerns(fv);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuRemoveVKern(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_RemoveVKP(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    FVRemoveVKerns(fv);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuKPCloseup(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_KernPairCloseup(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    int i;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] )
    break;
    KernPairD(fv->sf,i==fv->map->enccount?NULL:
		    fv->map->map[i]==-1?NULL:
		    fv->sf->glyphs[fv->map->map[i]],NULL,false);
}

# if defined(FONTFORGE_CONFIG_GDRAW)
static void FVMenuVKernFromHKern(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_VKernFromH(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

    FVVKernFromHKern(fv);
}
#endif

static void FVAutoHint(FontView *fv) {
    int i, cnt=0, gid;
    BlueData *bd = NULL, _bd;
    SplineChar *sc;

    if ( fv->sf->mm==NULL ) {
	QuickBlues(fv->sf,&_bd);
	bd = &_bd;
    }

    /* Tick the ones we don't want to AH, untick the ones that need AH */
    for ( gid = 0; gid<fv->sf->glyphcnt; ++gid ) if ( (sc = fv->sf->glyphs[gid])!=NULL )
	sc->ticked = true;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
		SCWorthOutputting(sc = fv->sf->glyphs[gid]) ) {
	    ++cnt;
	    sc->ticked = false;
	}
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_start_indicator(10,_("Auto Hinting Font..."),_("Auto Hinting Font..."),0,cnt,1);
# endif

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 && SCWorthOutputting(fv->sf->glyphs[gid]) ) {
	sc = fv->sf->glyphs[gid];
	sc->manualhints = false;
	/* Hint undoes are done in _SplineCharAutoHint */
	SFSCAutoHint(sc,bd);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	if ( !gwwv_progress_next())
    break;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_end_indicator();
# endif
    GDrawRequestExpose(fv->v,NULL,false);	/* Clear any changedsincelasthinted marks */
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuAutoHint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_AutoHint(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVAutoHint( fv );
}
#endif

static void FVAutoHintSubs(FontView *fv) {
    int i, cnt=0, gid;

    if ( fv->sf->mm!=NULL && fv->sf->mm->apple )
return;
    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
		SCWorthOutputting(fv->sf->glyphs[gid]) )
	    ++cnt;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_start_indicator(10,_("Finding Substitution Points..."),_("Finding Substitution Points..."),0,cnt,1);
# endif

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 && SCWorthOutputting(fv->sf->glyphs[gid]) ) {
	SplineChar *sc = fv->sf->glyphs[gid];
	SCFigureHintMasks(sc);
	SCUpdateAll(sc);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	if ( !gwwv_progress_next())
    break;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_end_indicator();
# endif
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuAutoHintSubs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_AutoHintSubs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVAutoHintSubs( fv );
}
#endif

static void FVAutoCounter(FontView *fv) {
    int i, cnt=0, gid;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
		SCWorthOutputting(fv->sf->glyphs[gid]) )
	    ++cnt;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_start_indicator(10,_("Finding Counter Masks..."),_("Finding Counter Masks..."),0,cnt,1);
# endif

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 && SCWorthOutputting(fv->sf->glyphs[gid]) ) {
	SplineChar *sc = fv->sf->glyphs[gid];
	SCFigureCounterMasks(sc);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	if ( !gwwv_progress_next())
    break;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_end_indicator();
# endif
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuAutoCounter(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_AutoCounter(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVAutoCounter( fv );
}
#endif

static void FVDontAutoHint(FontView *fv) {
    int i, gid;

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 && SCWorthOutputting(fv->sf->glyphs[gid]) ) {
	SplineChar *sc = fv->sf->glyphs[gid];
	sc->manualhints = true;
    }
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuDontAutoHint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_DontAutoHint(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVDontAutoHint( fv );
}
#endif

static void FVAutoInstr(FontView *fv,int usenowak) {
    BlueData bd;
    int i, cnt=0, gid;
    GlobalInstrCt gic;

    QuickBlues(fv->sf,&bd);

    if ( usenowak )
        InitGlobalInstrCt(&gic,fv->sf,&bd);

    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
		SCWorthOutputting(fv->sf->glyphs[gid]) )
	    ++cnt;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_start_indicator(10,_("Auto Instructing Font..."),_("Auto Instructing Font..."),0,cnt,1);
# endif

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 && SCWorthOutputting(fv->sf->glyphs[gid]) ) {
	SplineChar *sc = fv->sf->glyphs[gid];
	if ( usenowak )
	    NowakowskiSCAutoInstr(&gic,sc);
	else
	    SCAutoInstr(sc,&bd);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	if ( !gwwv_progress_next())
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    break;
    }
    
    if ( usenowak )
        FreeGlobalInstrCt(&gic);

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_end_indicator();
# endif
}

static void FVClearInstrs(FontView *fv) {
    SplineChar *sc;
    int i, gid;

    if ( !SFCloseAllInstrs(fv->sf))
return;

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 && SCWorthOutputting((sc = fv->sf->glyphs[gid])) ) {
	if ( sc->ttf_instrs_len!=0 ) {
	    free(sc->ttf_instrs);
	    sc->ttf_instrs = NULL;
	    sc->ttf_instrs_len = 0;
	    sc->instructions_out_of_date = false;
	    SCCharChangedUpdate(sc);
	    sc->complained_about_ptnums = false;
	}
    }
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuAutoInstr(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_AutoInstr(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVAutoInstr( fv, false );
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuNowakAutoInstr(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_NowakAutoInstr(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVAutoInstr( fv, true );
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuEditInstrs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_EditInstrs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    int index = FVAnyCharSelected(fv);
    SplineChar *sc;
    if ( index<0 )
return;
    sc = SFMakeChar(fv->sf,fv->map,index);
    SCEditInstructions(sc);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuEditTable(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    SFEditTable(fv->sf,
	    mi->mid==MID_Editprep?CHR('p','r','e','p'):
	    mi->mid==MID_Editfpgm?CHR('f','p','g','m'):
	    mi->mid==MID_Editmaxp?CHR('m','a','x','p'):
				  CHR('c','v','t',' '));
}
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_EditTable(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    G_CONST_RETURN gchar *name = gtk_widget_get_name(menuitem);
    SFEditTable(fv->sf,
	    strcmp(name,"edit_prep1")==0?CHR('p','r','e','p'):
	    strcmp(name,"edit_fpgm1")==0?CHR('f','p','g','m'):
				  CHR('c','v','t',' '));
}
# endif

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuPrivateToCvt(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_PrivateToCvt(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    CVT_ImportPrivate(fv->sf);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuClearInstrs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_ClearInstrs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVClearInstrs(fv);
}
#endif

static void FVClearHints(FontView *fv) {
    int i, gid;

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 && SCWorthOutputting(fv->sf->glyphs[gid]) ) {
	SplineChar *sc = fv->sf->glyphs[gid];
	sc->manualhints = true;
	SCPreserveHints(sc);
	SCClearHints(sc);
	SCUpdateAll(sc);
#if 0
	if ( !gwwv_progress_next())
    break;
#endif
    }
#if 0
    gwwv_progress_end_indicator();
#endif
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuClearHints(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_ClearHints(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    FVClearHints(fv);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuClearWidthMD(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_ClearWidthMD(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    int i, changed, gid;
    MinimumDistance *md, *prev, *next;

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 && SCWorthOutputting(fv->sf->glyphs[gid]) ) {
	SplineChar *sc = fv->sf->glyphs[gid];
	prev=NULL; changed = false;
	for ( md=sc->md; md!=NULL; md=next ) {
	    next = md->next;
	    if ( md->sp2==NULL ) {
		if ( prev==NULL )
		    sc->md = next;
		else
		    prev->next = next;
		chunkfree(md,sizeof(MinimumDistance));
		changed = true;
	    } else
		prev = md;
	}
	if ( changed ) {
	    sc->manualhints = true;
	    SCOutOfDateBackground(sc);
	    SCUpdateAll(sc);
	}
    }
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuHistograms(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    SFHistogram(fv->sf, NULL, FVAnyCharSelected(fv)!=-1?fv->selected:NULL,
			fv->map,
			mi->mid==MID_HStemHist ? hist_hstem :
			mi->mid==MID_VStemHist ? hist_vstem :
				hist_blues);
}
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Histograms(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    G_CONST_RETURN gchar *name = gtk_widget_get_name(menuitem);
    SFHistogram(fv->sf, NULL, FVAnyCharSelected(fv)!=-1?fv->selected:NULL,
			strcmp(name,"hstem1")==0 ? hist_hstem :
			strcmp(name,"vstem1")==0 ? hist_vstem :
				hist_blues);
# endif

void FVSetTitle(FontView *fv) {
# ifdef FONTFORGE_CONFIG_GDRAW
    unichar_t *title, *ititle, *temp;
    char *file=NULL;
    char *enc;
    int len;

    if ( fv->gw==NULL )		/* In scripting */
return;

    enc = SFEncodingName(fv->sf,fv->normal?fv->normal:fv->map);
    len = strlen(fv->sf->fontname)+1 + strlen(enc)+6;
    if ( fv->cidmaster!=NULL ) {
	if ( (file = fv->cidmaster->filename)==NULL )
	    file = fv->cidmaster->origname;
    } else {
	if ( (file = fv->sf->filename)==NULL )
	    file = fv->sf->origname;
    }
    if ( file!=NULL )
	len += 2+strlen(file);
    title = galloc((len+1)*sizeof(unichar_t));
    uc_strcpy(title,fv->sf->fontname);
    if ( fv->sf->changed )
	uc_strcat(title,"*");
    if ( file!=NULL ) {
	uc_strcat(title,"  ");
	temp = def2u_copy(GFileNameTail(file));
	u_strcat(title,temp);
	free(temp);
    }
    uc_strcat(title, " (" );
    if ( fv->normal ) { utf82u_strcat(title,_("Compact")); uc_strcat(title," "); }
    uc_strcat(title,enc);
    uc_strcat(title, ")" );
    free(enc);

    ititle = uc_copy(fv->sf->fontname);
    GDrawSetWindowTitles(fv->gw,title,ititle);
    free(title);
    free(ititle);
# elif defined(FONTFORGE_CONFIG_GTK)
    char *title, *ititle, *temp;
    char *file=NULL;
    char *enc;
    int len;
    gsize read, written;

    if ( fv->gw==NULL )		/* In scripting */
return;

    enc = SFEncodingName(fv->sf,fv->map);
    len = strlen(fv->sf->fontname)+1 + strlen(enc)+4;
    if ( fv->cidmaster!=NULL ) {
	if ( (file = fv->cidmaster->filename)==NULL )
	    file = fv->cidmaster->origname;
    } else {
	if ( (file = fv->sf->filename)==NULL )
	    file = fv->sf->origname;
    }
    if ( file!=NULL )
	len += 2+strlen(file);
    title = galloc((len+1));
    strcpy(title,fv->sf->fontname);
    if ( fv->sf->changed )
	strcat(title,"*");
    if ( file!=NULL ) {
	char *temp;
	strcat(title,"  ");
	temp = g_filename_to_utf8(GFileNameTail(file),-1,&read,&written,NULL);
	strcat(title,temp);
	free(temp);
    }
    strcat(title, " (" );
    strcat(title,enc);
    strcat(title, ")" );
    free(enc);

    gdk_window_set_icon_name( GTK_WIDGET(fv->gw)->window, fv->sf->fontname );
    gtk_window_set_title( GTK_WINDOW(fv->gw), title );
    free(title);
# endif
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuShowSubFont(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
static void FontViewMenu_ShowSubFont(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    SplineFont *new = mi->ti.userdata;
    FVShowSubFont(fv,new);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuConvert2CID(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Convert2CID(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    SplineFont *cidmaster = fv->cidmaster;

    if ( cidmaster!=NULL )
return;
    MakeCIDMaster(fv->sf,fv->map,false,NULL,NULL);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuConvertByCMap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_ConvertByCMap(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    SplineFont *cidmaster = fv->cidmaster;

    if ( cidmaster!=NULL )
return;
    MakeCIDMaster(fv->sf,fv->map,true,NULL,NULL);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuFlatten(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Flatten(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    SplineFont *cidmaster = fv->cidmaster;

    if ( cidmaster==NULL )
return;
    SFFlatten(cidmaster);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuFlattenByCMap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_FlattenByCMap(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    SplineFont *cidmaster = fv->cidmaster;

    if ( cidmaster==NULL )
return;
    SFFlattenByCMap(cidmaster,NULL);
}
#endif

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void FVInsertInCID(FontView *fv,SplineFont *sf) {
    SplineFont *cidmaster = fv->cidmaster;
    SplineFont **subs;
    int i;

    subs = galloc((cidmaster->subfontcnt+1)*sizeof(SplineFont *));
    for ( i=0; i<cidmaster->subfontcnt && cidmaster->subfonts[i]!=fv->sf; ++i )
	subs[i] = cidmaster->subfonts[i];
    subs[i] = sf;
    if ( sf->uni_interp == ui_none || sf->uni_interp == ui_unset )
	sf->uni_interp = cidmaster->uni_interp;
    for ( ; i<cidmaster->subfontcnt ; ++i )
	subs[i+1] = cidmaster->subfonts[i];
    ++cidmaster->subfontcnt;
    free(cidmaster->subfonts);
    cidmaster->subfonts = subs;
    cidmaster->changed = true;
    sf->cidmaster = cidmaster;

    CIDSetEncMap(fv,sf);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuInsertFont(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_InsertFont(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    SplineFont *cidmaster = fv->cidmaster;
    SplineFont *new;
    struct cidmap *map;
    char *filename;
    extern NameList *force_names_when_opening;

    if ( cidmaster==NULL || cidmaster->subfontcnt>=255 )	/* Open type allows 1 byte to specify the fdselect */
return;

    filename = GetPostscriptFontName(NULL,false);
    if ( filename==NULL )
return;
    new = LoadSplineFont(filename,0);
    free(filename);
    if ( new==NULL )
return;
    if ( new->fv == fv )		/* Already part of us */
return;
    if ( new->fv != NULL ) {
	if ( new->fv->gw!=NULL )
	    GDrawRaise(new->fv->gw);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	gwwv_post_error(_("Please close font"),_("Please close %s before inserting it into a CID font"),new->origname);
#endif
return;
    }
    EncMapFree(new->map);
    if ( force_names_when_opening!=NULL )
	SFRenameGlyphsToNamelist(new,force_names_when_opening );

    map = FindCidMap(cidmaster->cidregistry,cidmaster->ordering,cidmaster->supplement,cidmaster);
    SFEncodeToMap(new,map);
    if ( !PSDictHasEntry(new->private,"lenIV"))
	PSDictChangeEntry(new->private,"lenIV","1");		/* It's 4 by default, in CIDs the convention seems to be 1 */
    new->display_antialias = fv->sf->display_antialias;
    new->display_bbsized = fv->sf->display_bbsized;
    new->display_size = fv->sf->display_size;
    FVInsertInCID(fv,new);
    CIDMasterAsDes(new);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuInsertBlank(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_InsertBlank(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    SplineFont *cidmaster = fv->cidmaster, *sf;
    struct cidmap *map;

    if ( cidmaster==NULL || cidmaster->subfontcnt>=255 )	/* Open type allows 1 byte to specify the fdselect */
return;
    map = FindCidMap(cidmaster->cidregistry,cidmaster->ordering,cidmaster->supplement,cidmaster);
    sf = SplineFontBlank(MaxCID(map));
    sf->glyphcnt = sf->glyphmax;
    sf->cidmaster = cidmaster;
    sf->display_antialias = fv->sf->display_antialias;
    sf->display_bbsized = fv->sf->display_bbsized;
    sf->display_size = fv->sf->display_size;
    sf->private = gcalloc(1,sizeof(struct psdict));
    PSDictChangeEntry(sf->private,"lenIV","1");		/* It's 4 by default, in CIDs the convention seems to be 1 */
    FVInsertInCID(fv,sf);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuRemoveFontFromCID(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    char *buts[3];
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_RemoveFontFromCID(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    static char *buts[] = { GTK_STOCK_REMOVE, GTK_STOCK_CANCEL, NULL };
# endif
    SplineFont *cidmaster = fv->cidmaster, *sf = fv->sf, *replace;
    int i;
    MetricsView *mv, *mnext;
    FontView *fvs;

    if ( cidmaster==NULL || cidmaster->subfontcnt<=1 )	/* Can't remove last font */
return;
#if defined(FONTFORGE_CONFIG_GDRAW)
    buts[0] = _("_Remove"); buts[1] = _("_Cancel"); buts[2] = NULL;
#endif
    if ( gwwv_ask(_("_Remove Font"),(const char **) buts,0,1,_("Are you sure you wish to remove sub-font %1$.40s from the CID font %2$.40s"),
	    sf->fontname,cidmaster->fontname)==1 )
return;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	CharView *cv, *next;
	for ( cv = sf->glyphs[i]->views; cv!=NULL; cv = next ) {
	    next = cv->next;
	    GDrawDestroyWindow(cv->gw);
	}
    }
    GDrawProcessPendingEvents(NULL);
    for ( mv=fv->sf->metrics; mv!=NULL; mv = mnext ) {
	mnext = mv->next;
	GDrawDestroyWindow(mv->gw);
    }
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    /* Just in case... */
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);

    for ( i=0; i<cidmaster->subfontcnt; ++i )
	if ( cidmaster->subfonts[i]==sf )
    break;
    replace = i==0?cidmaster->subfonts[1]:cidmaster->subfonts[i-1];
    while ( i<cidmaster->subfontcnt-1 ) {
	cidmaster->subfonts[i] = cidmaster->subfonts[i+1];
	++i;
    }
    --cidmaster->subfontcnt;

    for ( fvs=fv->sf->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	if ( fvs->sf==sf )
	    CIDSetEncMap(fvs,replace);
    }
    FontViewReformatAll(fv->sf);
    SplineFontFree(sf);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuCIDFontInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_CIDFontInfo(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    SplineFont *cidmaster = fv->cidmaster;

    if ( cidmaster==NULL )
return;
    FontInfo(cidmaster,-1,false);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuChangeSupplement(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_ChangeSupplement(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    SplineFont *cidmaster = fv->cidmaster;
    struct cidmap *cidmap;
    char buffer[20];
    char *ret, *end;
    int supple;

    if ( cidmaster==NULL )
return;
    sprintf(buffer,"%d",cidmaster->supplement);
    ret = gwwv_ask_string(_("Change Supplement..."),buffer,_("Please specify a new supplement for %.20s-%.20s"),
	    cidmaster->cidregistry,cidmaster->ordering);
    if ( ret==NULL )
return;
    supple = strtol(ret,&end,10);
    if ( *end!='\0' || supple<=0 ) {
	free(ret);
	gwwv_post_error( _("Bad Number"),_("Bad Number") );
return;
    }
    free(ret);
    if ( supple!=cidmaster->supplement ) {
	    /* this will make noises if it can't find an appropriate cidmap */
	cidmap = FindCidMap(cidmaster->cidregistry,cidmaster->ordering,supple,cidmaster);
	cidmaster->supplement = supple;
	FVSetTitle(fv);
    }
}

# ifdef FONTFORGE_CONFIG_GDRAW
static SplineChar *FVFindACharInDisplay(FontView *fv) {
    int start, end, enc, gid;
    EncMap *map = fv->map;
    SplineFont *sf = fv->sf;
    SplineChar *sc;

    start = fv->rowoff*fv->colcnt;
    end = start + fv->rowcnt*fv->colcnt;
    for ( enc = start; enc<end && enc<map->enccount; ++enc ) {
	if ( (gid=map->map[enc])!=-1 && (sc=sf->glyphs[gid])!=NULL )
return( sc );
    }
return( NULL );
}

static void FVMenuReencode(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    Encoding *enc = NULL;
    EncMap *map;
    SplineChar *sc;

    sc = FVFindACharInDisplay(fv);
    enc = FindOrMakeEncoding(mi->ti.userdata);
    if ( enc==&custom )
	fv->map->enc = &custom;
    else {
	map = EncMapFromEncoding(fv->sf,enc);
	fv->selected = grealloc(fv->selected,map->enccount);
	memset(fv->selected,0,map->enccount);
	EncMapFree(fv->map);
	fv->map = map;
    }
    if ( fv->normal!=NULL ) {
	EncMapFree(fv->normal);
	fv->normal = NULL;
    }
    SFReplaceEncodingBDFProps(fv->sf,fv->map);
    FVSetTitle(fv);
    FontViewReformatOne(fv);
    if ( sc!=NULL ) {
	int enc = fv->map->backmap[sc->orig_pos];
	if ( enc!=-1 )
	    FVScrollToChar(fv,enc);
    }
}

static void FVMenuForceEncode(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    Encoding *enc = NULL;
    int oldcnt = fv->map->enccount;

    enc = FindOrMakeEncoding(mi->ti.userdata);
    SFForceEncoding(fv->sf,fv->map,enc);
    if ( oldcnt < fv->map->enccount ) {
	fv->selected = grealloc(fv->selected,fv->map->enccount);
	memset(fv->selected+oldcnt,0,fv->map->enccount-oldcnt);
    }
    if ( fv->normal!=NULL ) {
	EncMapFree(fv->normal);
	fv->normal = NULL;
    }
    SFReplaceEncodingBDFProps(fv->sf,fv->map);
    FVSetTitle(fv);
    FontViewReformatOne(fv);
}

static void FVMenuDisplayByGroups(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    DisplayGroups(fv);
}

static void FVMenuDefineGroups(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    DefineGroups(fv);
}
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Reencode(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
void FontViewMenu_ForceEncode(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuMMValid(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_MMValid(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    MMSet *mm = fv->sf->mm;

    if ( mm==NULL )
return;
    MMValid(mm,true);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuCreateMM(GWindow gw,struct gmenuitem *mi,GEvent *e) {
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_CreateMM(GtkMenuItem *menuitem, gpointer user_data) {
# endif
    MMWizard(NULL);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuMMInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_MMInfo(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    MMSet *mm = fv->sf->mm;

    if ( mm==NULL )
return;
    MMWizard(mm);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuChangeMMBlend(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_ChangeDefWeights(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    MMSet *mm = fv->sf->mm;

    if ( mm==NULL || mm->apple )
return;
    MMChangeBlend(mm,fv,false);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuBlendToNew(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_Blend(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    MMSet *mm = fv->sf->mm;

    if ( mm==NULL )
return;
    MMChangeBlend(mm,fv,true);
}
#endif

# ifdef FONTFORGE_CONFIG_GDRAW
static void cflistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    /*FontView *fv = (FontView *) GDrawGetUserData(gw);*/

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_AllFonts:
	    mi->ti.checked = !onlycopydisplayed;
	  break;
	  case MID_DisplayedFont:
	    mi->ti.checked = onlycopydisplayed;
	  break;
	  case MID_CharName:
	    mi->ti.checked = copymetadata;
	  break;
	  case MID_TTFInstr:
	    mi->ti.checked = copyttfinstr;
	  break;
	}
    }
}

static void sllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    fv = fv;
}

static void htlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int anychars = FVAnyCharSelected(fv);
    int multilayer = fv->sf->multilayer;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_AutoHint:
	    mi->ti.disabled = anychars==-1 || multilayer;
	  break;
	  case MID_HintSubsPt:
	    mi->ti.disabled = fv->sf->order2 || anychars==-1 || multilayer;
	    if ( fv->sf->mm!=NULL && fv->sf->mm->apple )
		mi->ti.disabled = true;
	  break;
	  case MID_AutoCounter: case MID_DontAutoHint:
	    mi->ti.disabled = fv->sf->order2 || anychars==-1 || multilayer;
	  break;
	  case MID_AutoInstr: case MID_EditInstructions:
	    mi->ti.disabled = !fv->sf->order2 || anychars==-1 || multilayer;
	  break;
	  case MID_PrivateToCvt:
	    mi->ti.disabled = !fv->sf->order2 || multilayer ||
		    fv->sf->private==NULL || fv->sf->cvt_dlg!=NULL;
	  break;
	  case MID_Editfpgm: case MID_Editprep: case MID_Editcvt: case MID_Editmaxp:
	    mi->ti.disabled = !fv->sf->order2 || multilayer;
	  break;
	  case MID_ClearHints: case MID_ClearWidthMD: case MID_ClearInstrs:
	    mi->ti.disabled = anychars==-1;
	  break;
	}
    }
}

static void fllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int anychars = FVAnyCharSelected(fv);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Revert:
	    mi->ti.disabled = fv->sf->origname==NULL || fv->sf->new;
	  break;
	  case MID_RevertToBackup:
	    /* We really do want to use filename here and origname above */
	    mi->ti.disabled = true;
	    if ( fv->sf->filename!=NULL ) {
		if ( fv->sf->backedup == bs_dontknow ) {
		    char *buf = galloc(strlen(fv->sf->filename)+20);
		    strcpy(buf,fv->sf->filename);
		    if ( fv->sf->compression!=0 )
			strcat(buf,compressors[fv->sf->compression-1].ext);
		    strcat(buf,"~");
		    if ( access(buf,F_OK)==0 )
			fv->sf->backedup = bs_backedup;
		    else
			fv->sf->backedup = bs_not;
		    free(buf);
		}
		if ( fv->sf->backedup == bs_backedup )
		    mi->ti.disabled = false;
	    }
	  break;
	  case MID_RevertGlyph:
	    mi->ti.disabled = fv->sf->origname==NULL || fv->sf->sfd_version<2 || anychars==-1 || fv->sf->compression!=0;
	  break;
	  case MID_Recent:
	    mi->ti.disabled = !RecentFilesAny();
	  break;
	  case MID_ScriptMenu:
	    mi->ti.disabled = script_menu_names[0]==NULL;
	  break;
	  case MID_Print:
	    mi->ti.disabled = fv->sf->onlybitmaps;
	  break;
	}
    }
}

static void edlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int pos = FVAnyCharSelected(fv), i, gid;
    int not_pasteable = pos==-1 ||
		    (!CopyContainsSomething() &&
#ifndef _NO_LIBPNG
		    !GDrawSelectionHasType(fv->gw,sn_clipboard,"image/png") &&
#endif
#ifndef _NO_LIBXML
		    !GDrawSelectionHasType(fv->gw,sn_clipboard,"image/svg") &&
#endif
		    !GDrawSelectionHasType(fv->gw,sn_clipboard,"image/bmp") &&
		    !GDrawSelectionHasType(fv->gw,sn_clipboard,"image/eps") &&
		    !GDrawSelectionHasType(fv->gw,sn_clipboard,"image/ps"));
    RefChar *base = CopyContainsRef(fv->sf);
    int base_enc = base!=NULL ? fv->map->backmap[base->orig_pos] : -1;


    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Paste: case MID_PasteInto:
	    mi->ti.disabled = not_pasteable;
	  break;
#ifdef FONTFORGE_CONFIG_PASTEAFTER
	  case MID_PasteAfter:
	    mi->ti.disabled = not_pasteable || pos<0;
	  break;
#endif
	  case MID_SameGlyphAs:
	    mi->ti.disabled = not_pasteable || base==NULL || fv->cidmaster!=NULL ||
		    base_enc==-1 ||
		    fv->selected[base_enc];	/* Can't be self-referential */
	  break;
	  case MID_Join:
	  case MID_Cut: case MID_Copy: case MID_Clear:
	  case MID_CopyWidth: case MID_CopyLBearing: case MID_CopyRBearing:
	  case MID_CopyRef: case MID_UnlinkRef:
	  case MID_RemoveUndoes: case MID_CopyFgToBg:
	  case MID_RplRef:
	    mi->ti.disabled = pos==-1;
	  break;
	  case MID_CopyLookupData:
	    mi->ti.disabled = pos==-1 || (fv->sf->gpos_lookups==NULL && fv->sf->gsub_lookups==NULL);
	  break;
	  case MID_CopyVWidth: 
	    mi->ti.disabled = pos==-1 || !fv->sf->hasvmetrics;
	  break;
	  case MID_ClearBackground:
	    mi->ti.disabled = true;
	    if ( pos!=-1 && !( onlycopydisplayed && fv->filled!=fv->show )) {
		for ( i=0; i<fv->map->enccount; ++i )
		    if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
			    fv->sf->glyphs[gid]!=NULL )
			if ( fv->sf->glyphs[gid]->layers[ly_back].images!=NULL ||
				fv->sf->glyphs[gid]->layers[ly_back].splines!=NULL ) {
			    mi->ti.disabled = false;
		break;
			}
	    }
	  break;
	  case MID_Undo:
	    for ( i=0; i<fv->map->enccount; ++i )
		if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
			fv->sf->glyphs[gid]!=NULL )
		    if ( fv->sf->glyphs[gid]->layers[ly_fore].undoes!=NULL )
	    break;
	    mi->ti.disabled = i==fv->map->enccount;
	  break;
	  case MID_Redo:
	    for ( i=0; i<fv->map->enccount; ++i )
		if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
			fv->sf->glyphs[gid]!=NULL )
		    if ( fv->sf->glyphs[gid]->layers[ly_fore].redoes!=NULL )
	    break;
	    mi->ti.disabled = i==fv->map->enccount;
	  break;
	}
    }
}

static void trlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int anychars = FVAnyCharSelected(fv);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Transform:
	    mi->ti.disabled = anychars==-1;
	  break;
	  case MID_NLTransform: case MID_POV:
	    mi->ti.disabled = anychars==-1 || fv->sf->onlybitmaps;
	  break;
	}
    }
}

static void ellistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int anychars = FVAnyCharSelected(fv), gid;
    int anybuildable, anytraceable;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_CharInfo:
	    mi->ti.disabled = anychars<0 || (gid = fv->map->map[anychars])==-1 ||
		    (fv->cidmaster!=NULL && fv->sf->glyphs[gid]==NULL);
	  break;
	  case MID_FindProblems:
	    mi->ti.disabled = anychars==-1;
	  break;
	  case MID_Validate:
	    mi->ti.disabled = fv->sf->strokedfont || fv->sf->multilayer;
	  break;
	  case MID_Transform:
	    mi->ti.disabled = anychars==-1;
	    /* some Transformations make sense on bitmaps now */
	  break;
	  case MID_AddExtrema:
	    mi->ti.disabled = anychars==-1 || fv->sf->onlybitmaps;
	  break;
	  case MID_Simplify:
	  case MID_Stroke: case MID_RmOverlap:
	    mi->ti.disabled = anychars==-1 || fv->sf->onlybitmaps;
	  break;
	  case MID_Styles:
	    mi->ti.disabled = anychars==-1 || fv->sf->onlybitmaps;
	  break;
	  case MID_Round: case MID_Correct:
	    mi->ti.disabled = anychars==-1 || fv->sf->onlybitmaps;
	  break;
#ifdef FONTFORGE_CONFIG_TILEPATH
	  case MID_TilePath:
	    mi->ti.disabled = anychars==-1 || fv->sf->onlybitmaps;
	  break;
#endif
	  case MID_AvailBitmaps:
	    mi->ti.disabled = fv->sf->mm!=NULL;
	  break;
	  case MID_RegenBitmaps: case MID_RemoveBitmaps:
	    mi->ti.disabled = fv->sf->bitmaps==NULL || fv->sf->onlybitmaps ||
		    fv->sf->mm!=NULL;
	  break;
	  case MID_BuildAccent:
	    anybuildable = false;
	    if ( anychars!=-1 ) {
		int i;
		for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] ) {
		    SplineChar *sc=NULL, dummy;
		    gid = fv->map->map[i];
		    if ( gid!=-1 )
			sc = fv->sf->glyphs[gid];
		    if ( sc==NULL )
			sc = SCBuildDummy(&dummy,fv->sf,fv->map,i);
		    if ( SFIsSomethingBuildable(fv->sf,sc,false) ||
			    SFIsDuplicatable(fv->sf,sc)) {
			anybuildable = true;
		break;
		    }
		}
	    }
	    mi->ti.disabled = !anybuildable;
	  break;
	  case MID_Autotrace:
	    anytraceable = false;
	    if ( FindAutoTraceName()!=NULL && anychars!=-1 ) {
		int i;
		for ( i=0; i<fv->map->enccount; ++i )
		    if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
			    fv->sf->glyphs[gid]!=NULL &&
			    fv->sf->glyphs[gid]->layers[ly_back].images!=NULL ) {
			anytraceable = true;
		break;
		    }
	    }
	    mi->ti.disabled = !anytraceable;
	  break;
	  case MID_MergeFonts:
	    mi->ti.disabled = fv->sf->bitmaps!=NULL && fv->sf->onlybitmaps;
	  break;
	  case MID_FontCompare:
	    mi->ti.disabled = fv_list->next==NULL;
	  break;
	  case MID_InterpolateFonts:
	    mi->ti.disabled = fv->sf->onlybitmaps;
	  break;
	}
    }
}

static void mtlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int anychars = FVAnyCharSelected(fv);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Center: case MID_Thirds: case MID_SetWidth: 
	  case MID_SetLBearing: case MID_SetRBearing:
	    mi->ti.disabled = anychars==-1;
	  break;
	  case MID_SetVWidth:
	    mi->ti.disabled = anychars==-1 || !fv->sf->hasvmetrics;
	  break;
	  case MID_VKernByClass:
	  case MID_VKernFromH:
	  case MID_RmVKern:
	    mi->ti.disabled = !fv->sf->hasvmetrics;
	  break;
	}
    }
}

#if HANYANG
static void hglistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
        if ( mi->mid==MID_BuildSyllables || mi->mid==MID_ModifyComposition )
	    mi->ti.disabled = fv->sf->rules==NULL;
    }
}

static GMenuItem2 hglist[] = {
    { { (unichar_t *) N_("_New Composition..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'N' }, H_("New Composition...|Ctl+Shft+N"), NULL, NULL, MenuNewComposition },
    { { (unichar_t *) N_("_Modify Composition..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Modify Composition...|No Shortcut"), NULL, NULL, FVMenuModifyComposition, MID_ModifyComposition },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Build Syllables"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Build Syllables|No Shortcut"), NULL, NULL, FVMenuBuildSyllables, MID_BuildSyllables },
    { NULL }
};
#endif

static void balistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
        if ( mi->mid==MID_BuildAccent || mi->mid==MID_BuildComposite ) {
	    int anybuildable = false;
	    int onlyaccents = mi->mid==MID_BuildAccent;
	    int i, gid;
	    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] ) {
		SplineChar *sc=NULL, dummy;
		if ( (gid=fv->map->map[i])!=-1 )
		    sc = fv->sf->glyphs[gid];
		if ( sc==NULL )
		    sc = SCBuildDummy(&dummy,fv->sf,fv->map,i);
		if ( SFIsSomethingBuildable(fv->sf,sc,onlyaccents)) {
		    anybuildable = true;
	    break;
		}
	    }
	    mi->ti.disabled = !anybuildable;
        } else if ( mi->mid==MID_BuildDuplicates ) {
	    int anybuildable = false;
	    int i, gid;
	    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] ) {
		SplineChar *sc=NULL, dummy;
		if ( (gid=fv->map->map[i])!=-1 )
		    sc = fv->sf->glyphs[gid];
		if ( sc==NULL )
		    sc = SCBuildDummy(&dummy,fv->sf,fv->map,i);
		if ( SFIsDuplicatable(fv->sf,sc)) {
		    anybuildable = true;
	    break;
		}
	    }
	    mi->ti.disabled = !anybuildable;
	}
    }
}

static void delistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int i = FVAnyCharSelected(fv);
    int gid = i<0 ? -1 : fv->map->map[i];

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_ShowDependentRefs:
	    mi->ti.disabled = gid<0 || fv->sf->glyphs[gid]==NULL ||
		    fv->sf->glyphs[gid]->dependents == NULL;
	  break;
	  case MID_ShowDependentSubs:
	    mi->ti.disabled = gid<0 || fv->sf->glyphs[gid]==NULL ||
		    !SCUsedBySubs(fv->sf->glyphs[gid]);
	  break;
	}
    }
}

static void infolistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int anychars = FVAnyCharSelected(fv);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_StrikeInfo:
	    mi->ti.disabled = fv->sf->bitmaps==NULL;
	  break;
	  case MID_MassRename:
	    mi->ti.disabled = anychars==-1;
	  break;
	}
    }
}

static GMenuItem2 dummyitem[] = { { (unichar_t *) N_("Font|_New"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'N' }, NULL };
static GMenuItem2 fllist[] = {
    { { (unichar_t *) N_("Font|_New"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'N' }, H_("New|Ctl+N"), NULL, NULL, MenuNew },
#if HANYANG
    { { (unichar_t *) N_("_Hangul"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, NULL, hglist, hglistcheck, NULL, 0 },
#endif
    { { (unichar_t *) N_("_Open"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'O' }, H_("Open|Ctl+O"), NULL, NULL, MenuOpen },
    { { (unichar_t *) N_("Recen_t"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, NULL, dummyitem, MenuRecentBuild, NULL, MID_Recent },
    { { (unichar_t *) N_("_Close"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Close|Ctl+Shft+Q"), NULL, NULL, FVMenuClose },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Save"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Save|Ctl+S"), NULL, NULL, FVMenuSave },
    { { (unichar_t *) N_("S_ave as..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'a' }, H_("Save as...|Ctl+Shft+S"), NULL, NULL, FVMenuSaveAs },
    { { (unichar_t *) N_("Save A_ll"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, H_("Save All|Alt+Ctl+S"), NULL, NULL, MenuSaveAll },
    { { (unichar_t *) N_("_Generate Fonts..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'G' }, H_("Generate Fonts...|Ctl+Shft+G"), NULL, NULL, FVMenuGenerate },
    { { (unichar_t *) N_("Generate Mac _Family..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Generate Mac Family...|Alt+Ctl+G"), NULL, NULL, FVMenuGenerateFamily },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Import..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Import...|Ctl+Shft+I"), NULL, NULL, FVMenuImport },
    { { (unichar_t *) N_("_Merge Feature Info..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Merge Kern Info...|Alt+Ctl+Shft+K"), NULL, NULL, FVMenuMergeKern },
    { { (unichar_t *) N_("_Revert File"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Revert File|Ctl+Shft+R"), NULL, NULL, FVMenuRevert, MID_Revert },
    { { (unichar_t *) N_("Revert To _Backup"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Revert To Backup|No Shortcut"), NULL, NULL, FVMenuRevertBackup, MID_RevertToBackup },
    { { (unichar_t *) N_("Revert Gl_yph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Revert Glyph|Alt+Ctl+R"), NULL, NULL, FVMenuRevertGlyph, MID_RevertGlyph },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Print..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Print...|Ctl+P"), NULL, NULL, FVMenuPrint, MID_Print },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
#if !defined(_NO_FFSCRIPT) || !defined(_NO_PYTHON)
    { { (unichar_t *) N_("E_xecute Script..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'x' }, H_("Execute Script...|Ctl+."), NULL, NULL, FVMenuExecute },
    { { (unichar_t *) N_("Script Menu"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'r' }, H_("Script Menu|No Shortcut"), dummyitem, MenuScriptsBuild, NULL, MID_ScriptMenu },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
#endif
    { { (unichar_t *) N_("Pr_eferences..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'e' }, H_("Preferences...|No Shortcut"), NULL, NULL, MenuPrefs },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Quit"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'Q' }, H_("Quit|Ctl+Q"), NULL, NULL, FVMenuExit },
    { NULL }
};

static GMenuItem2 cflist[] = {
    { { (unichar_t *) N_("_All Fonts"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'A' }, H_("All Fonts|No Shortcut"), NULL, NULL, FVMenuCopyFrom, MID_AllFonts },
    { { (unichar_t *) N_("_Displayed Font"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'D' }, H_("Displayed Font|No Shortcut"), NULL, NULL, FVMenuCopyFrom, MID_DisplayedFont },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Glyph _Metadata"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'N' }, H_("Glyph Metadata|No Shortcut"), NULL, NULL, FVMenuCopyFrom, MID_CharName },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_TrueType Instructions"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'N' }, H_("TrueType Instructions|No Shortcut"), NULL, NULL, FVMenuCopyFrom, MID_TTFInstr },
    { NULL }
};

static GMenuItem2 sclist[] = {
    { { (unichar_t *)  N_("Color|Default"), &def_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) COLOR_DEFAULT, NULL, 0, 1, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Default|No Shortcut"), NULL, NULL, FVMenuSelectColor },
    { { NULL, &white_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0xffffff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, FVMenuSelectColor },
    { { NULL, &red_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0xff0000, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, FVMenuSelectColor },
    { { NULL, &green_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0x00ff00, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, FVMenuSelectColor },
    { { NULL, &blue_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0x0000ff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, FVMenuSelectColor },
    { { NULL, &yellow_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0xffff00, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, FVMenuSelectColor },
    { { NULL, &cyan_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0x00ffff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, FVMenuSelectColor },
    { { NULL, &magenta_image, COLOR_DEFAULT, COLOR_DEFAULT, (void *) 0xff00ff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' }, NULL, NULL, NULL, FVMenuSelectColor },
    { NULL }
};

static GMenuItem2 sllist[] = {
    { { (unichar_t *) N_("Select _All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'A' }, H_("Select All|Ctl+A"), NULL, NULL, FVMenuSelectAll },
    { { (unichar_t *) N_("_Invert Selection"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Invert Selection|Ctl+Escape"), NULL, NULL, FVMenuInvertSelection },
    { { (unichar_t *) N_("_Deselect All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Deselect All|Escape"), NULL, NULL, FVMenuDeselectAll },
    { { (unichar_t *) N_("Select by _Color"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Select by Color|No Shortcut"), sclist },
    { { (unichar_t *) N_("Select by _Wildcard..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Select by Wildcard...|No Shortcut"), NULL, NULL, FVMenuSelectByName },
    { { (unichar_t *) N_("Select by _Script..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Select by Script...|No Shortcut"), NULL, NULL, FVMenuSelectByScript },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Glyphs Worth Outputting"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Glyphs Worth Outputting|No Shortcut"), NULL,NULL, FVMenuSelectWorthOutputting },
    { { (unichar_t *) N_("_Changed Glyphs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Changed Glyphs|No Shortcut"), NULL,NULL, FVMenuSelectChanged },
    { { (unichar_t *) N_("_Hinting Needed"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Hinting Needed|No Shortcut"), NULL,NULL, FVMenuSelectHintingNeeded },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Selec_t By Lookup Subtable..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Select By Lookup Subtable...|No Shortcut"), NULL, NULL, FVMenuSelectByPST },
    { NULL }
};

static GMenuItem2 edlist[] = {
    { { (unichar_t *) N_("_Undo"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'U' }, H_("Undo|Ctl+Z"), NULL, NULL, FVMenuUndo, MID_Undo },
    { { (unichar_t *) N_("_Redo"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Redo|Ctl+Y"), NULL, NULL, FVMenuRedo, MID_Redo},
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Cu_t"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, H_("Cut|Ctl+X"), NULL, NULL, FVMenuCut, MID_Cut },
    { { (unichar_t *) N_("_Copy"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Copy|Ctl+C"), NULL, NULL, FVMenuCopy, MID_Copy },
    { { (unichar_t *) N_("C_opy Reference"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Copy Reference|Ctl+G"), NULL, NULL, FVMenuCopyRef, MID_CopyRef },
    { { (unichar_t *) N_("Copy _Lookup Data"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Copy Lookup Data|Alt+Ctl+C"), NULL, NULL, FVMenuCopyLookupData, MID_CopyLookupData },
    { { (unichar_t *) N_("Copy _Width"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'W' }, H_("Copy Width|Ctl+W"), NULL, NULL, FVMenuCopyWidth, MID_CopyWidth },
    { { (unichar_t *) N_("Copy _VWidth"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("Copy VWidth|No Shortcut"), NULL, NULL, FVMenuCopyWidth, MID_CopyVWidth },
    { { (unichar_t *) N_("Co_py LBearing"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'p' }, H_("Copy LBearing|No Shortcut"), NULL, NULL, FVMenuCopyWidth, MID_CopyLBearing },
    { { (unichar_t *) N_("Copy RBearin_g"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'g' }, H_("Copy RBearing|No Shortcut"), NULL, NULL, FVMenuCopyWidth, MID_CopyRBearing },
    { { (unichar_t *) N_("_Paste"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Paste|Ctl+V"), NULL, NULL, FVMenuPaste, MID_Paste },
    { { (unichar_t *) N_("Paste Into"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Paste Into|Ctl+Shft+V"), NULL, NULL, FVMenuPasteInto, MID_PasteInto },
#ifdef FONTFORGE_CONFIG_PASTEAFTER
    { { (unichar_t *) N_("Paste After"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Paste After|Alt+Ctl+Shft+V"), NULL, NULL, FVMenuPasteAfter, MID_PasteAfter },
#endif
    { { (unichar_t *) N_("Sa_me Glyph As"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'm' }, H_("Same Glyph As|No Shortcut"), NULL, NULL, FVMenuSameGlyphAs, MID_SameGlyphAs },
    { { (unichar_t *) N_("C_lear"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, H_("Clear|No Shortcut"), NULL, NULL, FVMenuClear, MID_Clear },
    { { (unichar_t *) N_("Clear _Background"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Clear Background|No Shortcut"), NULL, NULL, FVMenuClearBackground, MID_ClearBackground },
    { { (unichar_t *) N_("Copy _Fg To Bg"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Copy Fg To Bg|Ctl+Shft+C"), NULL, NULL, FVMenuCopyFgBg, MID_CopyFgToBg },
    { { (unichar_t *) N_("_Join"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'J' }, H_("Join|Ctl+Shft+J"), NULL, NULL, FVMenuJoin, MID_Join },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Select"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, NULL, sllist, sllistcheck },
    { { (unichar_t *) N_("F_ind / Replace..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'i' }, H_("Find / Replace...|Alt+Ctl+F"), NULL, NULL, FVMenuFindRpl },
    { { (unichar_t *) N_("Replace with Reference"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'i' }, H_("Replace with Reference|Alt+Ctl+Shft+F"), NULL, NULL, FVMenuReplaceWithRef, MID_RplRef },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("U_nlink Reference"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'U' }, H_("Unlink Reference|Ctl+U"), NULL, NULL, FVMenuUnlinkRef, MID_UnlinkRef },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Copy _From"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, NULL, cflist, cflistcheck, NULL, 0 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Remo_ve Undoes"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'e' }, H_("Remove Undoes|No Shortcut"), NULL, NULL, FVMenuRemoveUndoes, MID_RemoveUndoes },
    { NULL }
};

static GMenuItem2 smlist[] = {
    { { (unichar_t *) N_("_Simplify"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Simplify|Ctl+Shft+M"), NULL, NULL, FVMenuSimplify, MID_Simplify },
    { { (unichar_t *) N_("Simplify More..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Simplify More...|Alt+Ctl+Shft+M"), NULL, NULL, FVMenuSimplifyMore, MID_SimplifyMore },
    { { (unichar_t *) N_("Clea_nup Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'n' }, H_("Cleanup Glyph|No Shortcut"), NULL, NULL, FVMenuCleanup, MID_CleanupGlyph },
    { { (unichar_t *) N_("Canonical Start _Point"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'n' }, H_("Canonical Start Point|No Shortcut"), NULL, NULL, FVMenuCanonicalStart, MID_CanonicalStart },
    { { (unichar_t *) N_("Canonical _Contours"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'n' }, H_("Canonical Contours|No Shortcut"), NULL, NULL, FVMenuCanonicalContours, MID_CanonicalContours },
    { NULL }
};

static GMenuItem2 rmlist[] = {
    { { (unichar_t *) N_("_Remove Overlap"), &GIcon_rmoverlap, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'O' }, H_("Remove Overlap|Ctl+Shft+O"), NULL, NULL, FVMenuOverlap, MID_RmOverlap },
    { { (unichar_t *) N_("_Intersect"), &GIcon_intersection, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Intersect|No Shortcut"), NULL, NULL, FVMenuOverlap, MID_Intersection },
    { { (unichar_t *) N_("_Find Intersections"), &GIcon_findinter, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'O' }, H_("Find Intersections|No Shortcut"), NULL, NULL, FVMenuOverlap, MID_FindInter },
    { NULL }
};

static GMenuItem2 eflist[] = {
    { { (unichar_t *) N_("Change _Weight..."), &GIcon_bold, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Change Weight...|Ctl+Shft+!"), NULL, NULL, FVMenuEmbolden, MID_Embolden },
    { { (unichar_t *) N_("Obl_ique..."), &GIcon_oblique, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Oblique...|No Shortcut"), NULL, NULL, FVMenuOblique },
    { { (unichar_t *) N_("_Condense/Extend..."), &GIcon_condense, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Condense...|No Shortcut"), NULL, NULL, FVMenuCondense, MID_Condense },
    { { (unichar_t *) N_("_Inline"), &GIcon_inline, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'O' }, H_("Inline|No Shortcut"), NULL, NULL, FVMenuInline },
    { { (unichar_t *) N_("_Outline"), &GIcon_outline, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Outline|No Shortcut"), NULL, NULL, FVMenuOutline },
    { { (unichar_t *) N_("_Shadow"), &GIcon_shadow, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Shadow|No Shortcut"), NULL, NULL, FVMenuShadow },
    { { (unichar_t *) N_("_Wireframe"), &GIcon_wireframe, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'W' }, H_("Wireframe|No Shortcut"), NULL, NULL, FVMenuWireframe },
    { NULL }
};

static GMenuItem2 balist[] = {
    { { (unichar_t *) N_("_Build Accented Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Build Accented Glyph|Ctl+Shft+A"), NULL, NULL, FVMenuBuildAccent, MID_BuildAccent },
    { { (unichar_t *) N_("Build _Composite Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Build Composite Glyph|No Shortcut"), NULL, NULL, FVMenuBuildComposite, MID_BuildComposite },
    { { (unichar_t *) N_("Buil_d Duplicate Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Build Duplicate Glyph|No Shortcut"), NULL, NULL, FVMenuBuildDuplicate, MID_BuildDuplicates },
#ifdef KOREAN
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_ShowGrp, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, NULL, NULL, NULL, FVMenuShowGroup },
#endif
    { NULL }
};

static GMenuItem2 delist[] = {
    { { (unichar_t *) N_("_References..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'u' }, H_("References...|Alt+Ctl+I"), NULL, NULL, FVMenuShowDependentRefs, MID_ShowDependentRefs },
    { { (unichar_t *) N_("_Substitutions..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Substitutions...|No Shortcut"), NULL, NULL, FVMenuShowDependentSubs, MID_ShowDependentSubs },
    { NULL }
};

static GMenuItem2 trlist[] = {
    { { (unichar_t *) N_("_Transform..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Transform...|No Shortcut"), NULL, NULL, FVMenuTransform, MID_Transform },
    { { (unichar_t *) N_("_Point of View Projection..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Point of View Projection...|Ctl+Shft+<"), NULL, NULL, FVMenuPOV, MID_POV },
    { { (unichar_t *) N_("_Non Linear Transform..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Non Linear Transform...|Ctl+Shft+|"), NULL, NULL, FVMenuNLTransform, MID_NLTransform },
    { NULL }
};

static GMenuItem2 rndlist[] = {
    { { (unichar_t *) N_("To _Int"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("To Int|Ctl+Shft+_"), NULL, NULL, FVMenuRound2Int, MID_Round },
    { { (unichar_t *) N_("To _Hundredths"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("To Hundredths|No Shortcut"), NULL, NULL, FVMenuRound2Hundredths, 0 },
    { { (unichar_t *) N_("_Cluster"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Cluster|No Shortcut"), NULL, NULL, FVMenuCluster },
    { NULL }
};

static GMenuItem2 infolist[] = {
    { { (unichar_t *) N_("_MATH Info..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("MATH Info...|No Shortcut"), NULL, NULL, FVMenuMATHInfo },
    { { (unichar_t *) N_("_BDF Info..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("BDF Info...|No Shortcut"), NULL, NULL, FVMenuBDFInfo, MID_StrikeInfo },
    { { (unichar_t *) N_("Show _Dependent"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, NULL, delist, delistcheck },
    { { (unichar_t *) N_("Mass Glyph _Rename..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Mass Glyph Rename...|No Shortcut"), NULL, NULL, FVMenuMassRename, MID_MassRename },
    { NULL }
};

static GMenuItem2 ellist[] = {
    { { (unichar_t *) N_("_Font Info..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Font Info...|Ctl+Shft+F"), NULL, NULL, FVMenuFontInfo },
    { { (unichar_t *) N_("Glyph _Info..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Glyph Info...|Ctl+I"), NULL, NULL, FVMenuCharInfo, MID_CharInfo },
    { { (unichar_t *) N_("Other Info"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, NULL, infolist, infolistcheck },
    { { (unichar_t *) N_("Find Pr_oblems..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Find Problems...|Ctl+E"), NULL, NULL, FVMenuFindProblems, MID_FindProblems },
    { { (unichar_t *) N_("_Validate..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Validate...|No Shortcut"), NULL, NULL, FVMenuValidate, MID_Validate },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Bitm_ap Strikes Available..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'A' }, H_("Bitmap Strikes Available...|Ctl+Shft+B"), NULL, NULL, FVMenuBitmaps, MID_AvailBitmaps },
    { { (unichar_t *) N_("Regenerate _Bitmap Glyphs..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Regenerate Bitmap Glyphs...|Ctl+B"), NULL, NULL, FVMenuBitmaps, MID_RegenBitmaps },
    { { (unichar_t *) N_("Remove Bitmap Glyphs..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Remove Bitmap Glyphs...|No Shortcut"), NULL, NULL, FVMenuBitmaps, MID_RemoveBitmaps },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Style"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, NULL, eflist, NULL, NULL, MID_Styles },
    { { (unichar_t *) N_("_Transformations"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, NULL, trlist, trlistcheck, NULL, MID_Transform },
    { { (unichar_t *) N_("_Expand Stroke..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'E' }, H_("Expand Stroke...|Ctl+Shft+E"), NULL, NULL, FVMenuStroke, MID_Stroke },
#ifdef FONTFORGE_CONFIG_TILEPATH
    { { (unichar_t *) N_("Tile _Path..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Tile Path...|No Shortcut"), NULL, NULL, FVMenuTilePath, MID_TilePath },
#endif
    { { (unichar_t *) N_("O_verlap"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'O' }, NULL, rmlist, NULL, NULL, MID_RmOverlap },
    { { (unichar_t *) N_("_Simplify"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, NULL, smlist, NULL, NULL, MID_Simplify },
    { { (unichar_t *) N_("Add E_xtrema"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'x' }, H_("Add Extrema|Ctl+Shft+X"), NULL, NULL, FVMenuAddExtrema, MID_AddExtrema },
    { { (unichar_t *) N_("Roun_d"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, NULL, rndlist, NULL, NULL, MID_Round },
    { { (unichar_t *) N_("Autot_race"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'r' }, H_("Autotrace|Ctl+Shft+T"), NULL, NULL, FVMenuAutotrace, MID_Autotrace },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Correct Direction"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'D' }, H_("Correct Direction|Ctl+Shft+D"), NULL, NULL, FVMenuCorrectDir, MID_Correct },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("B_uild"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, NULL, balist, balistcheck, NULL, MID_BuildAccent },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Merge Fonts..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Merge Fonts...|No Shortcut"), NULL, NULL, FVMenuMergeFonts, MID_MergeFonts },
    { { (unichar_t *) N_("Interpo_late Fonts..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'p' }, H_("Interpolate Fonts...|No Shortcut"), NULL, NULL, FVMenuInterpFonts, MID_InterpolateFonts },
    { { (unichar_t *) N_("Compare Fonts..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'p' }, H_("Compare Fonts...|No Shortcut"), NULL, NULL, FVMenuCompareFonts, MID_FontCompare },
    { NULL }
};

static GMenuItem2 dummyall[] = {
    { { (unichar_t *) N_("All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'K' }, H_("All|No Shortcut"), NULL, NULL, NULL },
    NULL
};

/* Builds up a menu containing all the anchor classes */
static void aplistbuild(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    extern void GMenuItemArrayFree(GMenuItem *mi);

    GMenuItemArrayFree(mi->sub);
    mi->sub = NULL;

    _aplistbuild(mi,fv->sf,FVMenuAnchorPairs);
}

static GMenuItem2 cblist[] = {
    { { (unichar_t *) N_("_Kern Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'K' }, H_("Kern Pairs|No Shortcut"), NULL, NULL, FVMenuKernPairs, MID_KernPairs },
    { { (unichar_t *) N_("_Anchored Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'K' }, H_("Anchored Pairs|No Shortcut"), dummyall, aplistbuild, NULL, MID_AnchorPairs },
    { { (unichar_t *) N_("_Ligatures"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'L' }, H_("Ligatures|No Shortcut"), NULL, NULL, FVMenuLigatures, MID_Ligatures },
    NULL
};

static void cblistcheck(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    SplineFont *sf = fv->sf;
    int i, anyligs=0, anykerns=0, gid;
    PST *pst;

    if ( sf->kerns ) anykerns=true;
    for ( i=0; i<fv->map->enccount; ++i ) if ( (gid = fv->map->map[i])!=-1 && sf->glyphs[gid]!=NULL ) {
	for ( pst=sf->glyphs[gid]->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->type==pst_ligature ) {
		anyligs = true;
		if ( anykerns )
    break;
	    }
	}
	if ( sf->glyphs[gid]->kerns!=NULL ) {
	    anykerns = true;
	    if ( anyligs )
    break;
	}
    }

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Ligatures:
	    mi->ti.disabled = !anyligs;
	  break;
	  case MID_KernPairs:
	    mi->ti.disabled = !anykerns;
	  break;
	  case MID_AnchorPairs:
	    mi->ti.disabled = sf->anchor==NULL;
	  break;
	}
    }
}


static GMenuItem2 gllist[] = {
    { { (unichar_t *) N_("_Glyph Image"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'K' }, H_("Glyph Image|No Shortcut"), NULL, NULL, FVMenuGlyphLabel, gl_glyph },
    { { (unichar_t *) N_("_Name"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'K' }, H_("Name|No Shortcut"), NULL, NULL, FVMenuGlyphLabel, gl_name },
    { { (unichar_t *) N_("_Unicode"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'L' }, H_("Unicode|No Shortcut"), NULL, NULL, FVMenuGlyphLabel, gl_unicode },
    { { (unichar_t *) N_("_Encoding Hex"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'L' }, H_("Encoding Hex|No Shortcut"), NULL, NULL, FVMenuGlyphLabel, gl_encoding },
    NULL
};

static void gllistcheck(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	mi->ti.checked = fv->glyphlabel == mi->mid;
    }
}

static GMenuItem2 emptymenu[] = {
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { NULL }
};

static void FVEncodingMenuBuild(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    extern void GMenuItemArrayFree(GMenuItem *mi);

    if ( mi->sub!=NULL ) {
	GMenuItemArrayFree(mi->sub);
	mi->sub = NULL;
    }
    mi->sub = GetEncodingMenu(FVMenuReencode,fv->map->enc);
}

static void FVForceEncodingMenuBuild(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    extern void GMenuItemArrayFree(GMenuItem *mi);

    if ( mi->sub!=NULL ) {
	GMenuItemArrayFree(mi->sub);
	mi->sub = NULL;
    }
    mi->sub = GetEncodingMenu(FVMenuForceEncode,fv->map->enc);
}

static void FVMenuAddUnencoded(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    char *ret, *end;
    int cnt, i;
    EncMap *map = fv->map;

    /* Add unused unencoded slots in the map */
    ret = gwwv_ask_string(_("Add Encoding Slots..."),"1",fv->cidmaster?_("How many CID slots do you wish to add?"):_("How many unencoded glyph slots do you wish to add?"));
    if ( ret==NULL )
return;
    cnt = strtol(ret,&end,10);
    if ( *end!='\0' || cnt<=0 ) {
	free(ret);
	gwwv_post_error( _("Bad Number"),_("Bad Number") );
return;
    }
    free(ret);
    if ( fv->normal!=NULL ) {
	/* If it's compacted, lose the base encoding and the fact that it's */
	/*  compact and make it be custom. That's what Alexey Kryukov asked */
	/*  for */
	EncMapFree(fv->normal);
	fv->normal = NULL;
	fv->map->enc = &custom;
	FVSetTitle(fv);
    }
    if ( fv->cidmaster ) {
	SplineFont *sf = fv->sf;
	FontView *fvs;
	if ( sf->glyphcnt+cnt<sf->glyphmax )
	    sf->glyphs = grealloc(sf->glyphs,(sf->glyphmax = sf->glyphcnt+cnt+10)*sizeof(SplineChar *));
	memset(sf->glyphs+sf->glyphcnt,0,cnt*sizeof(SplineChar *));
	for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	    EncMap *map = fvs->map;
	    if ( map->enccount+cnt>=map->encmax )
		map->map = grealloc(map->map,(map->encmax += cnt+10)*sizeof(int));
	    if ( sf->glyphcnt+cnt<map->backmax )
		map->backmap = grealloc(map->map,(map->backmax += cnt+10)*sizeof(int));
	    for ( i=map->enccount; i<map->enccount+cnt; ++i )
		map->map[i] = map->backmap[i] = i;
	    fvs->selected = grealloc(fvs->selected,(map->enccount+cnt));
	    memset(fvs->selected+map->enccount,0,cnt);
	    map->enccount += cnt;
	    if ( fv->filled!=NULL ) {
		if ( fv->filled->glyphmax<sf->glyphmax )
		    fv->filled->glyphs = grealloc(fv->filled->glyphs,(sf->glyphmax = sf->glyphcnt+cnt+10)*sizeof(BDFChar *));
		memset(fv->filled->glyphs+fv->filled->glyphcnt,0,cnt*sizeof(BDFChar *));
		fv->filled->glyphcnt = fv->filled->glyphmax = sf->glyphcnt+cnt;
	    }
	}
	sf->glyphcnt += cnt;
	FontViewReformatAll(fv->sf);
    } else {
	if ( map->enccount+cnt>=map->encmax )
	    map->map = grealloc(map->map,(map->encmax += cnt+10)*sizeof(int));
	for ( i=map->enccount; i<map->enccount+cnt; ++i )
	    map->map[i] = -1;
	fv->selected = grealloc(fv->selected,(map->enccount+cnt));
	memset(fv->selected+map->enccount,0,cnt);
	map->enccount += cnt;
	FontViewReformatOne(fv);
	FVScrollToChar(fv,map->enccount-cnt);
    }
}

static void FVMenuRemoveUnused(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    SplineFont *sf = fv->sf;
    EncMap *map = fv->map;
    int oldcount = map->enccount;
    int gid, i;
    int flags = -1;

    for ( i=map->enccount-1; i>=0 && ((gid=map->map[i])==-1 || !SCWorthOutputting(sf->glyphs[gid]));
	    --i ) {
	if ( gid!=-1 )
	    SFRemoveGlyph(sf,sf->glyphs[gid],&flags);
	map->enccount = i;
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    /* We reduced the encoding, so don't really need to reallocate the selection */
    /*  array. It's just bigger than it needs to be. */
    if ( oldcount!=map->enccount )
	FontViewReformatOne(fv);
#endif
}

static void FVMenuCompact(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int oldcount = fv->map->enccount;
    SplineChar *sc;

    sc = FVFindACharInDisplay(fv);
    if ( fv->normal!=NULL ) {
	EncMapFree(fv->map);
	fv->map = fv->normal;
	fv->normal = NULL;
	fv->selected = grealloc(fv->selected,fv->map->enccount);
	memset(fv->selected,0,fv->map->enccount);
    } else {
	/* We reduced the encoding, so don't really need to reallocate the selection */
	/*  array. It's just bigger than it needs to be. */
	fv->normal = EncMapCopy(fv->map);
	CompactEncMap(fv->map,fv->sf);
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( oldcount!=fv->map->enccount )
	FontViewReformatOne(fv);
    FVSetTitle(fv);
    if ( sc!=NULL ) {
	int enc = fv->map->backmap[sc->orig_pos];
	if ( enc!=-1 )
	    FVScrollToChar(fv,enc);
    }
#endif
}

static void FVMenuDetachGlyphs(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int i, j, gid;
    EncMap *map = fv->map;
    int altered = false;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    FontView *fvs;
#endif
    SplineFont *sf = fv->sf;

    for ( i=0; i<map->enccount; ++i ) if ( fv->selected[i] && (gid=map->map[i])!=-1 ) {
	altered = true;
	map->map[i] = -1;
	if ( map->backmap[gid]==i ) {
	    for ( j=map->enccount-1; j>=0 && map->map[j]!=gid; --j );
	    map->backmap[gid] = j;
	}
	if ( sf->glyphs[gid]!=NULL && sf->glyphs[gid]->altuni != NULL && map->enc!=&custom )
	    AltUniRemove(sf->glyphs[gid],UniFromEnc(i,map->enc));
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( altered )
	for ( fvs = fv->sf->fv; fvs!=NULL; fvs=fvs->nextsame )
	    GDrawRequestExpose(fvs->v,NULL,false);
#endif
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuDetachAndRemoveGlyphs(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    char *buts[3];
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_DetachAndRemoveGlyphs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    static char *buts[] = { GTK_STOCK_REMOVE, GTK_STOCK_CANCEL, NULL };
# endif
    int i, j, gid;
    EncMap *map = fv->map;
    SplineFont *sf = fv->sf;
    int flags = -1;
    int changed = false, altered = false;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    FontView *fvs;
#endif

#if defined(FONTFORGE_CONFIG_GDRAW)
    buts[0] = _("_Remove");
    buts[1] = _("_Cancel");
    buts[2] = NULL;
#endif
    
    if ( gwwv_ask(_("Detach & Remo_ve Glyphs..."),(const char **) buts,0,1,_("Are you sure you wish to remove these glyphs? This operation cannot be undone."))==1 )
return;

    for ( i=0; i<map->enccount; ++i ) if ( fv->selected[i] && (gid=map->map[i])!=-1 ) {
	altered = true;
	map->map[i] = -1;
	if ( map->backmap[gid]==i ) {
	    for ( j=map->enccount-1; j>=0 && map->map[j]!=gid; --j );
	    map->backmap[gid] = j;
	    if ( j==-1 ) {
		SFRemoveGlyph(sf,sf->glyphs[gid],&flags);
		changed = true;
	    } else if ( sf->glyphs[gid]!=NULL && sf->glyphs[gid]->altuni != NULL && map->enc!=&custom )
		AltUniRemove(sf->glyphs[gid],UniFromEnc(i,map->enc));
	}
    }
    if ( changed && !fv->sf->changed ) {
	fv->sf->changed = true;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	for ( fvs = sf->fv; fvs!=NULL; fvs=fvs->nextsame )
	    FVSetTitle(fvs);
#endif
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( altered )
	for ( fvs = sf->fv; fvs!=NULL; fvs=fvs->nextsame )
	    GDrawRequestExpose(fvs->v,NULL,false);
#endif
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuAddEncodingName(GWindow gw,struct gmenuitem *mi, GEvent *e) {
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_AddEncodingName(GtkMenuItem *menuitem, gpointer user_data) {
# endif
    char *ret;
    Encoding *enc;

    /* Search the iconv database for the named encoding */
    ret = gwwv_ask_string(_("Add Encoding Name..."),NULL,_("Please provide the name of an encoding in the iconv database which you want in the menu."));
    if ( ret==NULL )
return;
    enc = FindOrMakeEncoding(ret);
    if ( enc==NULL )
	gwwv_post_error(_("Invalid Encoding"),_("Invalid Encoding"));
    free(ret);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuLoadEncoding(GWindow gw,struct gmenuitem *mi, GEvent *e) {
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_LoadEncoding(GtkMenuItem *menuitem, gpointer user_data) {
# endif
    LoadEncodingFile();
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuMakeFromFont(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_MakeFromFont(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    (void) MakeEncoding(fv->sf,fv->map);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuRemoveEncoding(GWindow gw,struct gmenuitem *mi, GEvent *e) {
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_RemoveEncoding(GtkMenuItem *menuitem, gpointer user_data) {
# endif
    RemoveEncoding();
}

static int isuniname(char *name) {
    int i;
    if ( name[0]!='u' || name[1]!='n' || name[2]!='i' )
return( false );
    for ( i=3; i<7; ++i )
	if ( name[i]<'0' || (name[i]>'9' && name[i]<'A') || name[i]>'F' )
return( false );
    if ( name[7]!='\0' )
return( false );

return( true );
}

static int isuname(char *name) {
    int i;
    if ( name[0]!='u' )
return( false );
    for ( i=1; i<5; ++i )
	if ( name[i]<'0' || (name[i]>'9' && name[i]<'A') || name[i]>'F' )
return( false );
    if ( name[5]!='\0' )
return( false );

return( true );
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuMakeNamelist(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_MakeNamelist(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    gsize read, written;
# endif
    char buffer[1025];
    char *filename, *temp;
    FILE *file;
    SplineChar *sc;
    int i;

    snprintf(buffer, sizeof(buffer),"%s/%s.nam", getPfaEditDir(buffer), fv->sf->fontname );
# ifdef FONTFORGE_CONFIG_GDRAW
    temp = def2utf8_copy(buffer);
# elif defined(FONTFORGE_CONFIG_GTK)
    temp = g_filename_to_utf8(buffer,-1,&read,&written,NULL);
#endif
    filename = gwwv_save_filename(_("Make Namelist"), temp,"*.nam");
    free(temp);
    if ( filename==NULL )
return;
# ifdef FONTFORGE_CONFIG_GDRAW
    temp = utf82def_copy(filename);
# elif defined(FONTFORGE_CONFIG_GTK)
    temp = g_utf8_to_filename(filename,-1,&read,&written,NULL);
#endif
    file = fopen(temp,"w");
    free(temp);
    if ( file==NULL ) {
	gwwv_post_error(_("Namelist creation failed"),_("Could not write %s"), filename);
	free(filename);
return;
    }
    for ( i=0; i<fv->sf->glyphcnt; ++i ) {
	if ( (sc = fv->sf->glyphs[i])!=NULL && sc->unicodeenc!=-1 ) {
	    if ( !isuniname(sc->name) && !isuname(sc->name ) )
		fprintf( file, "0x%04X %s\n", sc->unicodeenc, sc->name );
	}
    }
    fclose(file);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuLoadNamelist(GWindow gw,struct gmenuitem *mi, GEvent *e) {
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_LoadNamelist(GtkMenuItem *menuitem, gpointer user_data) {
    gsize read, written;
# endif
    /* Read in a name list and copy it into the prefs dir so that we'll find */
    /*  it in the future */
    /* Be prepared to update what we've already got if names match */
    char buffer[1025];
    char *ret = gwwv_open_filename(_("Load Namelist"),NULL,
	    "*.nam",NULL);
    char *temp, *pt;
    char *buts[3];
    FILE *old, *new;
    int ch, ans;
    NameList *nl;

    if ( ret==NULL )
return;				/* Cancelled */
# ifdef FONTFORGE_CONFIG_GDRAW
    temp = utf82def_copy(ret);
# elif defined(FONTFORGE_CONFIG_GTK)
    temp = g_utf8_to_filename(ret,-1,&read,&written,NULL);
#endif
    pt = strrchr(temp,'/');
    if ( pt==NULL )
	pt = temp;
    else
	++pt;
    snprintf(buffer,sizeof(buffer),"%s/%s", getPfaEditDir(buffer), pt);
    if ( access(buffer,F_OK)==0 ) {
	buts[0] = _("_Replace");
#if defined(FONTFORGE_CONFIG_GDRAW)
	buts[1] = _("_Cancel");
#elif defined(FONTFORGE_CONFIG_GTK)
	buts[1] = GTK_STOCK_CANCEL;
#endif
	buts[2] = NULL;
	ans = gwwv_ask( _("Replace"),(const char **) buts,0,1,_("A name list with this name already exists. Replace it?"));
	if ( ans==1 ) {
	    free(temp);
	    free(ret);
return;
	}
    }

    old = fopen( temp,"r");
    if ( old==NULL ) {
	gwwv_post_error(_("No such file"),_("Could not read %s"), ret );
	free(ret); free(temp);
return;
    }
    if ( (nl = LoadNamelist(temp))==NULL ) {
	gwwv_post_error(_("Bad namelist file"),_("Could not parse %s"), ret );
	free(ret); free(temp);
return;
    }
    free(ret); free(temp);
    if ( nl->uses_unicode ) {
	if ( nl->a_utf8_name!=NULL )
	    ff_post_notice(_("Non-ASCII glyphnames"),_("This namelist contains at least one non-ASCII glyph name, namely: %s"), nl->a_utf8_name );
	else
	    ff_post_notice(_("Non-ASCII glyphnames"),_("This namelist is based on a namelist which contains non-ASCII glyph names"));
    }

    new = fopen( buffer,"w");
    if ( new==NULL ) {
	gwwv_post_error(_("Create failed"),_("Could not write %s"), buffer );
return;
    }

    while ( (ch=getc(old))!=EOF )
	putc(ch,new);
    fclose(old);
    fclose(new);
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuRenameByNamelist(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_RenameByNamelist(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
# endif
    char **namelists = AllNamelistNames();
    int i;
    int ret;
    NameList *nl;
    extern int allow_utf8_glyphnames;

    for ( i=0; namelists[i]!=NULL; ++i );
    ret = gwwv_choose(_("Rename by NameList"),(const char **) namelists,i,0,_("Rename the glyphs in this font to the names found in the selected namelist"));
    if ( ret==-1 )
return;
    nl = NameListByName(namelists[ret]);
    if ( nl==NULL ) {
	IError("Couldn't find namelist");
return;
    } else if ( nl!=NULL && nl->uses_unicode && !allow_utf8_glyphnames) {
	gwwv_post_error(_("Namelist contains non-ASCII names"),_("Glyph names should be limited to characters in the ASCII character set, but there are names in this namelist which use characters outside that range."));
return;
    }
    SFRenameGlyphsToNamelist(fv->sf,nl);
# if defined(FONTFORGE_CONFIG_GDRAW)
    GDrawRequestExpose(fv->v,NULL,false);
# elif defined(FONTFORGE_CONFIG_GTK)
    gtk_widget_queue_draw(fv->v);
# endif
}

# ifdef FONTFORGE_CONFIG_GDRAW
static void FVMenuNameGlyphs(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_NameGlyphs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    gsize read, written;
# endif
    /* Read a file containing a list of names, and add an unencoded glyph for */
    /*  each name */
    char buffer[33];
    char *ret = gwwv_open_filename(_("Load glyph names"),NULL, "*",NULL);
    char *temp, *pt;
    FILE *file;
    int ch;
    SplineChar *sc;
    FontView *fvs;

    if ( ret==NULL )
return;				/* Cancelled */
# ifdef FONTFORGE_CONFIG_GDRAW
    temp = utf82def_copy(ret);
# elif defined(FONTFORGE_CONFIG_GTK)
    temp = g_utf8_to_filename(ret,-1,&read,&written,NULL);
#endif

    file = fopen( temp,"r");
    if ( file==NULL ) {
	gwwv_post_error(_("No such file"),_("Could not read %s"), ret );
	free(ret); free(temp);
return;
    }
    pt = buffer;
    forever {
	ch = getc(file);
	if ( ch!=EOF && !isspace(ch)) {
	    if ( pt<buffer+sizeof(buffer)-1 )
		*pt++ = ch;
	} else {
	    if ( pt!=buffer ) {
		*pt = '\0';
		sc = NULL;
		for ( fvs=fv->sf->fv; fvs!=NULL; fvs=fvs->nextsame ) {
		    EncMap *map = fvs->map;
		    if ( map->enccount+1>=map->encmax )
			map->map = grealloc(map->map,(map->encmax += 20)*sizeof(int));
		    map->map[map->enccount] = -1;
		    fvs->selected = grealloc(fvs->selected,(map->enccount+1));
		    memset(fvs->selected+map->enccount,0,1);
		    ++map->enccount;
		    if ( sc==NULL ) {
			sc = SFMakeChar(fv->sf,map,map->enccount-1);
			free(sc->name);
			sc->name = copy(buffer);
			sc->comment = copy(".");	/* Mark as something for sfd file */
			/*SCLigDefault(sc);*/
		    }
		    map->map[map->enccount-1] = sc->orig_pos;
		    map->backmap[sc->orig_pos] = map->enccount-1;
		}
		pt = buffer;
	    }
	    if ( ch==EOF )
    break;
	}
    }
    fclose(file);
    free(ret); free(temp);
    FontViewReformatAll(fv->sf);
}

static GMenuItem2 enlist[] = {
    { { (unichar_t *) N_("_Reencode"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'E' }, H_("Reencode|No Shortcut"), emptymenu, FVEncodingMenuBuild, NULL, MID_Reencode },
    { { (unichar_t *) N_("_Compact"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'C' }, H_("Compact|No Shortcut"), NULL, NULL, FVMenuCompact, MID_Compact },
    { { (unichar_t *) N_("_Force Encoding"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Force Encoding|No Shortcut"), emptymenu, FVForceEncodingMenuBuild, NULL, MID_ForceReencode },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Add Encoding Slots..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Add Encoding Slots...|No Shortcut"), NULL, NULL, FVMenuAddUnencoded, MID_AddUnencoded },
    { { (unichar_t *) N_("Remove _Unused Slots"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Remove Unused Slots|No Shortcut"), NULL, NULL, FVMenuRemoveUnused, MID_RemoveUnused },
    { { (unichar_t *) N_("_Detach Glyphs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Detach Glyphs|No Shortcut"), NULL, NULL, FVMenuDetachGlyphs, MID_DetachGlyphs },
    { { (unichar_t *) N_("Detach & Remo_ve Glyphs..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Detach & Remove Glyphs...|No Shortcut"), NULL, NULL, FVMenuDetachAndRemoveGlyphs, MID_DetachAndRemoveGlyphs },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Add E_ncoding Name..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Add Encoding Name...|No Shortcut"), NULL, NULL, FVMenuAddEncodingName },
    { { (unichar_t *) N_("_Load Encoding..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Load Encoding...|No Shortcut"), NULL, NULL, FVMenuLoadEncoding, MID_LoadEncoding },
    { { (unichar_t *) N_("Ma_ke From Font..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Make From Font...|No Shortcut"), NULL, NULL, FVMenuMakeFromFont, MID_MakeFromFont },
    { { (unichar_t *) N_("Remove En_coding..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Remove Encoding...|No Shortcut"), NULL, NULL, FVMenuRemoveEncoding, MID_RemoveEncoding },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Display By _Groups..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Display By Groups...|No Shortcut"), NULL, NULL, FVMenuDisplayByGroups, MID_DisplayByGroups },
    { { (unichar_t *) N_("D_efine Groups..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Define Groups...|No Shortcut"), NULL, NULL, FVMenuDefineGroups },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Save Namelist of Font..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Save Namelist of Font...|No Shortcut"), NULL, NULL, FVMenuMakeNamelist, MID_SaveNamelist },
    { { (unichar_t *) N_("L_oad Namelist..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Load Namelist...|No Shortcut"), NULL, NULL, FVMenuLoadNamelist },
    { { (unichar_t *) N_("Rename Gl_yphs..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Rename Glyphs...|No Shortcut"), NULL, NULL, FVMenuRenameByNamelist, MID_RenameGlyphs },
    { { (unichar_t *) N_("Cre_ate Named Glyphs..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Create Named Glyphs...|No Shortcut"), NULL, NULL, FVMenuNameGlyphs, MID_NameGlyphs },
    { NULL }
};

static void enlistcheck(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int i, gid;
    SplineFont *sf = fv->sf;
    EncMap *map = fv->map;
    int anyglyphs = false;

    for ( i=map->enccount-1; i>=0 ; --i )
	if ( fv->selected[i] && (gid=map->map[i])!=-1 )
	    anyglyphs = true;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Compact:
	    mi->ti.checked = fv->normal!=NULL;
	  break;
	  case MID_Reencode: case MID_ForceReencode:
	    mi->ti.disabled = fv->cidmaster!=NULL;
	  break;
#if 0
	  case MID_AddUnencoded:
	    mi->ti.disabled = fv->normal!=NULL;
	  break;
#endif
	  case MID_DetachGlyphs: case MID_DetachAndRemoveGlyphs:
	    mi->ti.disabled = !anyglyphs;
	  break;
	  case MID_RemoveUnused:
	    gid = map->map[map->enccount-1];
	    mi->ti.disabled = gid!=-1 && SCWorthOutputting(sf->glyphs[gid]);
	  break;
	  case MID_MakeFromFont:
	    mi->ti.disabled = fv->cidmaster!=NULL || map->enccount>1024 || map->enc!=&custom;
	  break;
	  case MID_RemoveEncoding:
	  break;
	  case MID_DisplayByGroups:
	    mi->ti.disabled = fv->cidmaster!=NULL || group_root==NULL;
	  break;
	  case MID_NameGlyphs:
	    mi->ti.disabled = fv->normal!=NULL || fv->cidmaster!=NULL;
	  break;
	  case MID_RenameGlyphs: case MID_SaveNamelist:
	    mi->ti.disabled = fv->cidmaster!=NULL;
	  break;
	}
    }
}

static GMenuItem2 vwlist[] = {
    { { (unichar_t *) N_("_Next Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'N' }, H_("Next Glyph|Ctl+]"), NULL, NULL, FVMenuChangeChar, MID_Next },
    { { (unichar_t *) N_("_Prev Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Prev Glyph|Ctl+["), NULL, NULL, FVMenuChangeChar, MID_Prev },
    { { (unichar_t *) N_("Next _Defined Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'D' }, H_("Next Defined Glyph|Alt+Ctl+]"), NULL, NULL, FVMenuChangeChar, MID_NextDef },
    { { (unichar_t *) N_("Prev Defined Gl_yph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'a' }, H_("Prev Defined Glyph|Alt+Ctl+["), NULL, NULL, FVMenuChangeChar, MID_PrevDef },
    { { (unichar_t *) N_("_Goto"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'G' }, H_("Goto|Ctl+Shft+>"), NULL, NULL, FVMenuGotoChar },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Show ATT"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Show ATT|No Shortcut"), NULL, NULL, FVMenuShowAtt },
    { { (unichar_t *) N_("Display S_ubstitutions..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'u' }, H_("Display Substitutions...|No Shortcut"), NULL, NULL, FVMenuDisplaySubs, MID_DisplaySubs },
    { { (unichar_t *) N_("Com_binations"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'b' }, NULL, cblist, cblistcheck },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Label Glyph By"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'b' }, NULL, gllist, gllistcheck },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("S_how H. Metrics..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, H_("Show H. Metrics...|No Shortcut"), NULL, NULL, FVMenuShowMetrics, MID_ShowHMetrics },
    { { (unichar_t *) N_("Show _V. Metrics..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("Show V. Metrics...|No Shortcut"), NULL, NULL, FVMenuShowMetrics, MID_ShowVMetrics },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("32x8 cell window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, '2' }, H_("32x8 cell window|Ctl+Shft+%"), NULL, NULL, FVMenuWSize, MID_32x8 },
    { { (unichar_t *) N_("1_6x4 cell window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, '3' }, H_("16x4 cell window|Ctl+Shft+^"), NULL, NULL, FVMenuWSize, MID_16x4 },
    { { (unichar_t *) N_("_8x2  cell window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, '3' }, H_("8x2  cell window|Ctl+Shft+*"), NULL, NULL, FVMenuWSize, MID_8x2 },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_24 pixel outline"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, '2' }, H_("24 pixel outline|Ctl+2"), NULL, NULL, FVMenuSize, MID_24 },
    { { (unichar_t *) N_("_36 pixel outline"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, '3' }, H_("36 pixel outline|Ctl+3"), NULL, NULL, FVMenuSize, MID_36 },
    { { (unichar_t *) N_("_48 pixel outline"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, '4' }, H_("48 pixel outline|Ctl+4"), NULL, NULL, FVMenuSize, MID_48 },
    { { (unichar_t *) N_("_72 pixel outline"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, '4' }, H_("72 pixel outline|Ctl+7"), NULL, NULL, FVMenuSize, MID_72 },
    { { (unichar_t *) N_("_96 pixel outline"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, '4' }, H_("96 pixel outline|Ctl+9"), NULL, NULL, FVMenuSize, MID_96 },
    { { (unichar_t *) N_("_Anti Alias"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'A' }, H_("Anti Alias|Ctl+5"), NULL, NULL, FVMenuSize, MID_AntiAlias },
    { { (unichar_t *) N_("_Fit to em"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'F' }, H_("Fit to em|Ctl+6"), NULL, NULL, FVMenuSize, MID_FitToEm },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Bitmap _Magnification..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'F' }, H_("Bitmap Magnification...|No Shortcut"), NULL, NULL, FVMenuMagnify, MID_BitmapMag },
    { NULL },			/* Some extra room to show bitmaps */
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }
};

static void vwlistcheck(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int anychars = FVAnyCharSelected(fv);
    int i, base;
    BDFFont *bdf;
    char buffer[50];
    extern void GMenuItemArrayFree(GMenuItem *mi);
    extern GMenuItem *GMenuItem2ArrayCopy(GMenuItem2 *mi, uint16 *cnt);
    int pos;
    SplineFont *sf = fv->sf;
    EncMap *map = fv->map;
    OTLookup *otl;

    for ( i=0; vwlist[i].ti.text==NULL || strcmp((char *) vwlist[i].ti.text, _("Bitmap _Magnification..."))!=0; ++i );
    base = i+1;
    for ( i=base; vwlist[i].ti.text!=NULL; ++i ) {
	free( vwlist[i].ti.text);
	vwlist[i].ti.text = NULL;
    }

    vwlist[base-1].ti.disabled = true;
    if ( sf->bitmaps!=NULL ) {
	for ( bdf = sf->bitmaps, i=base;
		i<sizeof(vwlist)/sizeof(vwlist[0])-1 && bdf!=NULL;
		++i, bdf = bdf->next ) {
	    if ( BDFDepth(bdf)==1 )
		sprintf( buffer, _("%d pixel bitmap"), bdf->pixelsize );
	    else
		sprintf( buffer, _("%d@%d pixel bitmap"),
			bdf->pixelsize, BDFDepth(bdf) );
	    vwlist[i].ti.text = (unichar_t *) uc_copy(buffer);
	    vwlist[i].ti.checkable = true;
	    vwlist[i].ti.checked = bdf==fv->show;
	    vwlist[i].ti.userdata = bdf;
	    vwlist[i].invoke = FVMenuShowBitmap;
	    vwlist[i].ti.fg = vwlist[i].ti.bg = COLOR_DEFAULT;
	    if ( bdf==fv->show )
		vwlist[base-1].ti.disabled = false;
	}
    }
    GMenuItemArrayFree(mi->sub);
    mi->sub = GMenuItem2ArrayCopy(vwlist,NULL);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Next: case MID_Prev:
	    mi->ti.disabled = anychars<0;
	  break;
	  case MID_NextDef:
	    pos = anychars+1;
	    if ( anychars<0 ) pos = map->enccount;
	    for ( ; pos<map->enccount &&
		    (map->map[pos]==-1 || !SCWorthOutputting(sf->glyphs[map->map[pos]]));
		    ++pos );
	    mi->ti.disabled = pos==map->enccount;
	  break;
	  case MID_PrevDef:
	    for ( pos = anychars-1; pos>=0 &&
		    (map->map[pos]==-1 || !SCWorthOutputting(sf->glyphs[map->map[pos]]));
		    --pos );
	    mi->ti.disabled = pos<0;
	  break;
	  case MID_DisplaySubs: { SplineFont *_sf = sf;
	    mi->ti.checked = fv->cur_subtable!=NULL;
	    if ( _sf->cidmaster ) _sf = _sf->cidmaster;
	    for ( otl=_sf->gsub_lookups; otl!=NULL; otl=otl->next )
		if ( otl->lookup_type == gsub_single && otl->subtables!=NULL )
	    break;
	    mi->ti.disabled = otl==NULL;
	  } break;
	  case MID_ShowHMetrics:
	    /*mi->ti.checked = fv->showhmetrics;*/
	  break;
	  case MID_ShowVMetrics:
	    /*mi->ti.checked = fv->showvmetrics;*/
	    mi->ti.disabled = !sf->hasvmetrics;
	  break;
	  case MID_32x8:
	    mi->ti.checked = (fv->rowcnt==8 && fv->colcnt==32);
	  break;
	  case MID_16x4:
	    mi->ti.checked = (fv->rowcnt==4 && fv->colcnt==16);
	  break;
	  case MID_8x2:
	    mi->ti.checked = (fv->rowcnt==2 && fv->colcnt==8);
	  break;
	  case MID_24:
	    mi->ti.checked = (fv->show!=NULL && fv->show==fv->filled && fv->show->pixelsize==24);
	    mi->ti.disabled = sf->onlybitmaps && fv->show!=fv->filled;
	  break;
	  case MID_36:
	    mi->ti.checked = (fv->show!=NULL && fv->show==fv->filled && fv->show->pixelsize==36);
	    mi->ti.disabled = sf->onlybitmaps && fv->show!=fv->filled;
	  break;
	  case MID_48:
	    mi->ti.checked = (fv->show!=NULL && fv->show==fv->filled && fv->show->pixelsize==48);
	    mi->ti.disabled = sf->onlybitmaps && fv->show!=fv->filled;
	  break;
	  case MID_72:
	    mi->ti.checked = (fv->show!=NULL && fv->show==fv->filled && fv->show->pixelsize==72);
	    mi->ti.disabled = sf->onlybitmaps && fv->show!=fv->filled;
	  break;
	  case MID_96:
	    mi->ti.checked = (fv->show!=NULL && fv->show==fv->filled && fv->show->pixelsize==96);
	    mi->ti.disabled = sf->onlybitmaps && fv->show!=fv->filled;
	  break;
	  case MID_AntiAlias:
	    mi->ti.checked = (fv->show!=NULL && fv->show->clut!=NULL);
	    mi->ti.disabled = sf->onlybitmaps && fv->show!=fv->filled;
	  break;
	  case MID_FitToEm:
	    mi->ti.checked = (fv->show!=NULL && !fv->show->bbsized);
	    mi->ti.disabled = sf->onlybitmaps && fv->show!=fv->filled;
	  break;
	}
    }
}

static GMenuItem2 histlist[] = {
    { { (unichar_t *) N_("_HStem"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, H_("HStem|No Shortcut"), NULL, NULL, FVMenuHistograms, MID_HStemHist },
    { { (unichar_t *) N_("_VStem"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("VStem|No Shortcut"), NULL, NULL, FVMenuHistograms, MID_VStemHist },
    { { (unichar_t *) N_("BlueValues"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("BlueValues|No Shortcut"), NULL, NULL, FVMenuHistograms, MID_BlueValuesHist },
    { NULL }
};

static GMenuItem2 autoinstlist[] = {
    { { (unichar_t *) N_("_Former"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("AutoInstrFormer|No Shortcut"), NULL, NULL, FVMenuAutoInstr },
    { { (unichar_t *) N_("_Nowakowski"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("AutoInstrNowak|Ctl+T"), NULL, NULL, FVMenuNowakAutoInstr },
    { NULL }
};

static GMenuItem2 htlist[] = {
    { { (unichar_t *) N_("Auto_Hint"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, H_("AutoHint|Ctl+Shft+H"), NULL, NULL, FVMenuAutoHint, MID_AutoHint },
    { { (unichar_t *) N_("Hint _Substitution Pts"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, H_("Hint Substitution Pts|No Shortcut"), NULL, NULL, FVMenuAutoHintSubs, MID_HintSubsPt },
    { { (unichar_t *) N_("Auto _Counter Hint"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, H_("Auto Counter Hint|No Shortcut"), NULL, NULL, FVMenuAutoCounter, MID_AutoCounter },
    { { (unichar_t *) N_("_Don't AutoHint"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, H_("Don't AutoHint|No Shortcut"), NULL, NULL, FVMenuDontAutoHint, MID_DontAutoHint },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Auto_Instr"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, NULL, autoinstlist, NULL, NULL, MID_AutoInstr },
    { { (unichar_t *) N_("_Edit Instructions..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, H_("Edit Instructions...|No Shortcut"), NULL, NULL, FVMenuEditInstrs, MID_EditInstructions },
    { { (unichar_t *) N_("Edit 'fpgm'..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Edit 'fpgm'...|No Shortcut"), NULL, NULL, FVMenuEditTable, MID_Editfpgm },
    { { (unichar_t *) N_("Edit 'prep'..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Edit 'prep'...|No Shortcut"), NULL, NULL, FVMenuEditTable, MID_Editprep },
    { { (unichar_t *) N_("Edit 'maxp'..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Edit 'maxp'...|No Shortcut"), NULL, NULL, FVMenuEditTable, MID_Editmaxp },
    { { (unichar_t *) N_("Edit 'cvt '..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Edit 'cvt '...|No Shortcut"), NULL, NULL, FVMenuEditTable, MID_Editcvt },
    { { (unichar_t *) N_("Private to 'cvt'"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, H_("Private to 'cvt'|No Shortcut"), NULL, NULL, FVMenuPrivateToCvt, MID_PrivateToCvt },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Clear Hints"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Clear Hints|No Shortcut"), NULL, NULL, FVMenuClearHints, MID_ClearHints },
    { { (unichar_t *) N_("Clear _Width MD"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Clear Width MD|No Shortcut"), NULL, NULL, FVMenuClearWidthMD, MID_ClearWidthMD },
    { { (unichar_t *) N_("Clear Instructions"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Clear Instructions|No Shortcut"), NULL, NULL, FVMenuClearInstrs, MID_ClearInstrs },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Histograms"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, NULL, histlist },
    { NULL }
};

static GMenuItem2 mtlist[] = {
    { { (unichar_t *) N_("_Center in Width"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Center in Width|No Shortcut"), NULL, NULL, FVMenuCenter, MID_Center },
    { { (unichar_t *) N_("_Thirds in Width"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Thirds in Width|No Shortcut"), NULL, NULL, FVMenuCenter, MID_Thirds },
    { { (unichar_t *) N_("Set _Width..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'W' }, H_("Set Width...|Ctl+Shft+L"), NULL, NULL, FVMenuSetWidth, MID_SetWidth },
    { { (unichar_t *) N_("Set _LBearing..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'L' }, H_("Set LBearing...|Ctl+L"), NULL, NULL, FVMenuSetWidth, MID_SetLBearing },
    { { (unichar_t *) N_("Set _RBearing..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Set RBearing...|Ctl+R"), NULL, NULL, FVMenuSetWidth, MID_SetRBearing },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Set _Vertical Advance..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("Set Vertical Advance...|No Shortcut"), NULL, NULL, FVMenuSetWidth, MID_SetVWidth },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Auto Width..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'A' }, H_("Auto Width...|Ctl+Shft+W"), NULL, NULL, FVMenuAutoWidth },
    { { (unichar_t *) N_("Auto _Kern..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'K' }, H_("Auto Kern...|Ctl+Shft+K"), NULL, NULL, FVMenuAutoKern },
    { { (unichar_t *) N_("Ker_n By Classes..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'K' }, H_("Kern By Classes...|No Shortcut"), NULL, NULL, FVMenuKernByClasses },
    { { (unichar_t *) N_("Remove All Kern _Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Remove All Kern Pairs|No Shortcut"), NULL, NULL, FVMenuRemoveKern, MID_RmHKern },
    { { (unichar_t *) N_("Kern Pair Closeup..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Kern Pair Closeup...|No Shortcut"), NULL, NULL, FVMenuKPCloseup },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("VKern By Classes..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'K' }, H_("VKern By Classes...|No Shortcut"), NULL, NULL, FVMenuVKernByClasses, MID_VKernByClass },
    { { (unichar_t *) N_("VKern From HKern"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("VKern From HKern|No Shortcut"), NULL, NULL, FVMenuVKernFromHKern, MID_VKernFromH },
    { { (unichar_t *) N_("Remove All VKern Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Remove All VKern Pairs|No Shortcut"), NULL, NULL, FVMenuRemoveVKern, MID_RmVKern },
    { NULL }
};

static GMenuItem2 cdlist[] = {
    { { (unichar_t *) N_("_Convert to CID"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Convert to CID|No Shortcut"), NULL, NULL, FVMenuConvert2CID, MID_Convert2CID },
    { { (unichar_t *) N_("Convert By C_Map"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Convert By CMap|No Shortcut"), NULL, NULL, FVMenuConvertByCMap, MID_ConvertByCMap },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Flatten"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Flatten|No Shortcut"), NULL, NULL, FVMenuFlatten, MID_Flatten },
    { { (unichar_t *) N_("Fl_attenByCMap"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("FlattenByCMap|No Shortcut"), NULL, NULL, FVMenuFlattenByCMap, MID_FlattenByCMap },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Insert F_ont..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Insert Font...|No Shortcut"), NULL, NULL, FVMenuInsertFont, MID_InsertFont },
    { { (unichar_t *) N_("Insert _Blank"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Insert Blank|No Shortcut"), NULL, NULL, FVMenuInsertBlank, MID_InsertBlank },
    { { (unichar_t *) N_("_Remove Font"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Remove Font|No Shortcut"), NULL, NULL, FVMenuRemoveFontFromCID, MID_RemoveFromCID },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Change Supplement..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Change Supplement...|No Shortcut"), NULL, NULL, FVMenuChangeSupplement, MID_ChangeSupplement },
    { { (unichar_t *) N_("C_ID Font Info..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("CID Font Info...|No Shortcut"), NULL, NULL, FVMenuCIDFontInfo, MID_CIDFontInfo },
    { NULL },				/* Extra room to show sub-font names */
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }
};

static void cdlistcheck(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int i, base, j;
    extern void GMenuItemArrayFree(GMenuItem *mi);
    extern GMenuItem *GMenuItem2ArrayCopy(GMenuItem2 *mi, uint16 *cnt);
    SplineFont *sub, *cidmaster = fv->cidmaster;

    for ( i=0; cdlist[i].mid!=MID_CIDFontInfo; ++i );
    base = i+2;
    for ( i=base; cdlist[i].ti.text!=NULL; ++i ) {
	free( cdlist[i].ti.text);
	cdlist[i].ti.text = NULL;
    }

    cdlist[base-1].ti.fg = cdlist[base-1].ti.bg = COLOR_DEFAULT;
    if ( cidmaster==NULL ) {
	cdlist[base-1].ti.line = false;
    } else {
	cdlist[base-1].ti.line = true;
	for ( j = 0, i=base; 
		i<sizeof(cdlist)/sizeof(cdlist[0])-1 && j<cidmaster->subfontcnt;
		++i, ++j ) {
	    sub = cidmaster->subfonts[j];
	    cdlist[i].ti.text = uc_copy(sub->fontname);
	    cdlist[i].ti.checkable = true;
	    cdlist[i].ti.checked = sub==fv->sf;
	    cdlist[i].ti.userdata = sub;
	    cdlist[i].invoke = FVMenuShowSubFont;
	    cdlist[i].ti.fg = cdlist[i].ti.bg = COLOR_DEFAULT;
	}
    }
    GMenuItemArrayFree(mi->sub);
    mi->sub = GMenuItem2ArrayCopy(cdlist,NULL);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Convert2CID: case MID_ConvertByCMap:
	    mi->ti.disabled = cidmaster!=NULL || fv->sf->mm!=NULL;
	  break;
	  case MID_InsertFont: case MID_InsertBlank:
	    /* OpenType allows at most 255 subfonts (PS allows more, but why go to the effort to make safe font check that? */
	    mi->ti.disabled = cidmaster==NULL || cidmaster->subfontcnt>=255;
	  break;
	  case MID_RemoveFromCID:
	    mi->ti.disabled = cidmaster==NULL || cidmaster->subfontcnt<=1;
	  break;
	  case MID_Flatten: case MID_FlattenByCMap: case MID_CIDFontInfo:
	  case MID_ChangeSupplement:
	    mi->ti.disabled = cidmaster==NULL;
	  break;
	}
    }
}

static GMenuItem2 mmlist[] = {
/* GT: Here (and following) MM means "MultiMaster" */
    { { (unichar_t *) N_("_Create MM..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Create MM...|No Shortcut"), NULL, NULL, FVMenuCreateMM, MID_CreateMM },
    { { (unichar_t *) N_("MM _Validity Check"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("MM Validity Check|No Shortcut"), NULL, NULL, FVMenuMMValid, MID_MMValid },
    { { (unichar_t *) N_("MM _Info..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("MM Info...|No Shortcut"), NULL, NULL, FVMenuMMInfo, MID_MMInfo },
    { { (unichar_t *) N_("_Blend to New Font..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Blend to New Font...|No Shortcut"), NULL, NULL, FVMenuBlendToNew, MID_BlendToNew },
    { { (unichar_t *) N_("MM Change Default _Weights..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("MM Change Default Weights...|No Shortcut"), NULL, NULL, FVMenuChangeMMBlend, MID_ChangeMMBlend },
    { NULL },				/* Extra room to show sub-font names */
};

static void mmlistcheck(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int i, base, j;
    extern void GMenuItemArrayFree(GMenuItem *mi);
    extern GMenuItem *GMenuItem2ArrayCopy(GMenuItem2 *mi, uint16 *cnt);
    MMSet *mm = fv->sf->mm;
    SplineFont *sub;
    GMenuItem2 *mml;

    for ( i=0; mmlist[i].mid!=MID_ChangeMMBlend; ++i );
    base = i+2;
    if ( mm==NULL )
	mml = mmlist;
    else {
	mml = gcalloc(base+mm->instance_count+2,sizeof(GMenuItem2));
	memcpy(mml,mmlist,sizeof(mmlist));
	mml[base-1].ti.fg = mml[base-1].ti.bg = COLOR_DEFAULT;
	mml[base-1].ti.line = true;
	for ( j = 0, i=base; j<mm->instance_count+1; ++i, ++j ) {
	    if ( j==0 )
		sub = mm->normal;
	    else
		sub = mm->instances[j-1];
	    mml[i].ti.text = uc_copy(sub->fontname);
	    mml[i].ti.checkable = true;
	    mml[i].ti.checked = sub==fv->sf;
	    mml[i].ti.userdata = sub;
	    mml[i].invoke = FVMenuShowSubFont;
	    mml[i].ti.fg = mml[i].ti.bg = COLOR_DEFAULT;
	}
    }
    GMenuItemArrayFree(mi->sub);
    mi->sub = GMenuItem2ArrayCopy(mml,NULL);
    if ( mml!=mmlist ) {
	for ( i=base; mml[i].ti.text!=NULL; ++i )
	    free( mml[i].ti.text);
	free(mml);
    }

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_CreateMM:
	    mi->ti.disabled = false;
	  break;
	  case MID_MMInfo: case MID_MMValid: case MID_BlendToNew:
	    mi->ti.disabled = mm==NULL;
	  break;
	  case MID_ChangeMMBlend:
	    mi->ti.disabled = mm==NULL || mm->apple;
	  break;
	}
    }
}

static GMenuItem2 wnmenu[] = {
    { { (unichar_t *) N_("New O_utline Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'u' }, H_("New Outline Window|Ctl+H"), NULL, NULL, FVMenuOpenOutline, MID_OpenOutline },
    { { (unichar_t *) N_("New _Bitmap Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("New Bitmap Window|Ctl+J"), NULL, NULL, FVMenuOpenBitmap, MID_OpenBitmap },
    { { (unichar_t *) N_("New _Metrics Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("New Metrics Window|Ctl+K"), NULL, NULL, FVMenuOpenMetrics },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Warnings"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Warnings|No Shortcut"), NULL, NULL, _MenuWarnings, MID_Warnings },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { NULL }
};

static void FVWindowMenuBuild(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int anychars = FVAnyCharSelected(fv);
    struct gmenuitem *wmi;

    WindowMenuBuild(gw,mi,e);
    for ( wmi = mi->sub; wmi->ti.text!=NULL || wmi->ti.line ; ++wmi ) {
	switch ( wmi->mid ) {
	  case MID_OpenOutline:
	    wmi->ti.disabled = anychars==-1;
	  break;
	  case MID_OpenBitmap:
	    wmi->ti.disabled = anychars==-1 || fv->sf->bitmaps==NULL;
	  break;
	  case MID_Warnings:
	    wmi->ti.disabled = ErrorWindowExists();
	  break;
	}
    }
}

GMenuItem2 helplist[] = {
    { { (unichar_t *) N_("_Help"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, H_("Help|F1"), NULL, NULL, FVMenuContextualHelp },
    { { (unichar_t *) N_("_Overview"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Overview|Shft+F1"), NULL, NULL, MenuHelp },
    { { (unichar_t *) N_("_Index"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Index|Ctl+F1"), NULL, NULL, MenuIndex },
    { { (unichar_t *) N_("_About..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'A' }, H_("About...|No Shortcut"), NULL, NULL, MenuAbout },
    { { (unichar_t *) N_("_License..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'A' }, H_("License...|No Shortcut"), NULL, NULL, MenuLicense },
    { NULL }
};

GMenuItem fvpopupmenu[] = {
    { { (unichar_t *) N_("Cu_t"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, '\0', ksm_control, NULL, NULL, FVMenuCut, MID_Cut },
    { { (unichar_t *) N_("_Copy"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, '\0', ksm_control, NULL, NULL, FVMenuCopy, MID_Copy },
    { { (unichar_t *) N_("C_opy Reference"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, FVMenuCopyRef, MID_CopyRef },
    { { (unichar_t *) N_("Copy _Width"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'W' }, '\0', ksm_control, NULL, NULL, FVMenuCopyWidth, MID_CopyWidth },
    { { (unichar_t *) N_("_Paste"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, '\0', ksm_control, NULL, NULL, FVMenuPaste, MID_Paste },
    { { (unichar_t *) N_("C_lear"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, 0, 0, NULL, NULL, FVMenuClear, MID_Clear },
    { { (unichar_t *) N_("Copy _Fg To Bg"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, '\0', ksm_control|ksm_shift, NULL, NULL, FVMenuCopyFgBg, MID_CopyFgToBg },
    { { (unichar_t *) N_("U_nlink Reference"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'U' }, '\0', ksm_control, NULL, NULL, FVMenuUnlinkRef, MID_UnlinkRef },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Glyph _Info..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, '\0', ksm_control, NULL, NULL, FVMenuCharInfo, MID_CharInfo },
    { { (unichar_t *) N_("_Transform..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, '\0', ksm_control, NULL, NULL, FVMenuTransform, MID_Transform },
    { { (unichar_t *) N_("_Expand Stroke..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'E' }, '\0', ksm_control|ksm_shift, NULL, NULL, FVMenuStroke, MID_Stroke },
/*    { { (unichar_t *) N_("_Remove Overlap"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'O' }, '\0', ksm_control|ksm_shift, NULL, NULL, FVMenuOverlap, MID_RmOverlap },*/
/*    { { (unichar_t *) N_("_Simplify"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, '\0', ksm_control|ksm_shift, NULL, NULL, FVMenuSimplify, MID_Simplify },*/
/*    { { (unichar_t *) N_("Add E_xtrema"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'x' }, 'X', ksm_control|ksm_shift, NULL, NULL, FVMenuAddExtrema, MID_AddExtrema },*/
    { { (unichar_t *) N_("To _Int"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, '\0', ksm_control|ksm_shift, NULL, NULL, FVMenuRound2Int, MID_Round },
    { { (unichar_t *) N_("_Correct Direction"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'D' }, '\0', ksm_control|ksm_shift, NULL, NULL, FVMenuCorrectDir, MID_Correct },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Auto_Hint"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, '\0', ksm_control|ksm_shift, NULL, NULL, FVMenuAutoHint, MID_AutoHint },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Center in Width"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, '\0', ksm_control, NULL, NULL, FVMenuCenter, MID_Center },
    { { (unichar_t *) N_("Set _Width..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'W' }, '\0', ksm_control|ksm_shift, NULL, NULL, FVMenuSetWidth, MID_SetWidth },
    { { (unichar_t *) N_("Set _Vertical Advance..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, '\0', ksm_control|ksm_shift, NULL, NULL, FVMenuSetWidth, MID_SetVWidth },
    { NULL }
};

static GMenuItem2 mblist[] = {
    { { (unichar_t *) N_("_File"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, NULL, fllist, fllistcheck },
    { { (unichar_t *) N_("_Edit"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'E' }, NULL, edlist, edlistcheck },
    { { (unichar_t *) N_("E_lement"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, NULL, ellist, ellistcheck },
#ifndef _NO_PYTHON
    { { (unichar_t *) N_("_Tools"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, NULL, NULL, fvpy_tllistcheck},
#endif
    { { (unichar_t *) N_("H_ints"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'i' }, NULL, htlist, htlistcheck },
    { { (unichar_t *) N_("E_ncoding"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, NULL, enlist, enlistcheck },
    { { (unichar_t *) N_("_View"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, NULL, vwlist, vwlistcheck },
    { { (unichar_t *) N_("_Metrics"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, NULL, mtlist, mtlistcheck },
    { { (unichar_t *) N_("_CID"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, NULL, cdlist, cdlistcheck },
/* GT: Here (and following) MM means "MultiMaster" */
    { { (unichar_t *) N_("MM"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, NULL, mmlist, mmlistcheck },
    { { (unichar_t *) N_("_Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'W' }, NULL, wnmenu, FVWindowMenuBuild, NULL },
    { { (unichar_t *) N_("_Help"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, NULL, helplist, NULL },
    { NULL }
};
# elif defined(FONTFORGE_CONFIG_GTK)
void FontViewMenu_ActivateCopyFrom(GtkMenuItem *menuitem, gpointer user_data) {
    GtkWidget *w;

    w = lookup_widget( menuitem, "all_fonts1" );
    gtk_widget_set_sensitive(w,!onlycopydisplayed);

    w = lookup_widget( menuitem, "displayed_fonts1" );
    gtk_widget_set_sensitive(w,onlycopydisplayed);

    w = lookup_widget( menuitem, "char_metadata1" );
    gtk_widget_set_sensitive(w,copymetadata);
}

struct fv_any {
    FontView *fv;
    int anychars;
    void (*callback)(GtkWidget *w,gpointer user_data);
};

static void activate_file_items(GtkWidget *w, gpointer user_data) {
    struct fv_any *fv_any = (struct fv_any *) user_data;
    G_CONST_RETURN gchar *name = gtk_widget_get_name( GTK_WIDGET(w));

    if ( strcmp(name,"open_outline_window1")==0 )
	gtk_widget_set_sensitive(w,fv_any->anychars!=-1);
    else if ( strcmp(name,"open_bitmap_window1")==0 )
	gtk_widget_set_sensitive(w,fv_any->anychars!=-1 && fv_any->fv->sf->bitmaps!=NULL);
    else if ( strcmp(name,"revert_file1")==0 )
	gtk_widget_set_sensitive(w,fv_any->fv->sf->origname!=NULL);
    else if ( strcmp(name,"revert_glyph1")==0 )
	gtk_widget_set_sensitive(w,fv_any->anychars!=-1 && fv_any->fv->sf->origname!=NULL);
    else if ( strcmp(name,"recent1")==0 )
	gtk_widget_set_sensitive(w,RecentFilesAny());
    else if ( strcmp(name,"script_menu1")==0 )
	gtk_widget_set_sensitive(w,script_menu_names[0]!=NULL);
    else if ( strcmp(name,"print2")==0 )
	gtk_widget_set_sensitive(w,!fv_any->fv->onlybitmaps);
    else if ( strcmp(name,"display1")==0 )
	gtk_widget_set_sensitive(w,!((fv->sf->onlybitmaps && fv->sf->bitmaps==NULL) ||
			fv->sf->multilayer));
}

void FontViewMenu_ActivateFile(GtkMenuItem *menuitem, gpointer user_data) {
    struct fv_any data;

    data.fv = FV_From_MI(menuitem);
    data.anychars = FVAnyCharSelected(data.fv);

    gtk_container_foreach( GTK_CONTAINER( gtk_menu_item_get_submenu(menuitem )),
	    activate_file_items, &data );
}

void FontViewMenu_ActivateEdit(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int pos = FVAnyCharSelected(fv), i;
    /* I can only determine the contents of the clipboard asyncronously */
    /*  hence I can't check to see if it contains something that is pastable */
    /*  So all I can do with paste is check that something is selected */
#if 0
    int not_pasteable = pos==-1 ||
		    (!CopyContainsSomething() &&
		    !GDrawSelectionHasType(fv->gw,sn_clipboard,"image/png") &&
		    !GDrawSelectionHasType(fv->gw,sn_clipboard,"image/svg") &&
		    !GDrawSelectionHasType(fv->gw,sn_clipboard,"image/bmp") &&
		    !GDrawSelectionHasType(fv->gw,sn_clipboard,"image/eps"));
#endif
    RefChar *base = CopyContainsRef(fv->sf);
    GtkWidget *w;
    int gid;
    static char *poslist[] = { "cut2", "copy2", "copy_reference1", "copy_width1",
	    "copy_lbearing1", "copy_rbearing1", "paste2", "paste_into1",
	    "paste_after1", "same_glyph_as1", "clear2", "clear_background1",
	    "copy_fg_to_bg1", "join1", "replace_with_reference1",
	    "unlink_reference1", "remove_undoes1", NULL };

    w = lookup_widget( menuitem, "undo2" );
    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
		fv->sf->glyphs[gid]!=NULL ) {
	    if ( fv->sf->glyphs[gid]->layers[ly_fore].undoes!=NULL )
    break;
    gtk_widget_set_sensitive(w,i!=fv->map->enccount);

    w = lookup_widget( menuitem, "redo2" );
    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid = fv->map->map[i])!=-1 &&
		fv->sf->glyphs[gid]!=NULL ) {
	    if ( fv->sf->glyphs[gid]->layers[ly_fore].redoes!=NULL )
    break;
    gtk_widget_set_sensitive(w,i!=fv->map->enccount);

    for ( i=0; poslist[i]!=NULL; ++i ) {
	w = lookup_widget( menuitem, poslist[i] );
	if ( w!=NULL )
	    gtk_widget_set_sensitive(w,pos!=-1);
    }

    w = lookup_widget( menuitem, "copy_vwidth1" );
    if ( w!=NULL )
	gtk_widget_set_sensitive(w,pos!=-1 && fv->sf->hasvmetrics);

#  ifndef FONTFORGE_CONFIG_PASTEAFTER
    w = lookup_widget( menuitem, "paste_after1" );
    gtk_widget_hide(w);
#  endif
}

void FontViewMenu_ActivateDependents(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int anychars = FVAnyCharSelected(fv);
    int anygid = anychars<0 ? -1 : fv->map->map[anychars];

    gtk_widget_set_sensitive(lookup_widget( menuitem, "references1" ),
	    anygid>=0 &&
		    fv->sf->glyphs[anygid]!=NULL &&
		    fv->sf->glyphs[anygid]->dependents != NULL);
    gtk_widget_set_sensitive(lookup_widget( menuitem, "substitutions1" ),
	    anygid>=0 &&
		    fv->sf->glyphs[anygid]!=NULL &&
		    SCUsedBySubs(fv->sf->glyphs[anygid]));
}

void FontViewMenu_ActivateAAT(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FontView *ofv;
    int anychars = FVAnyCharSelected(fv);
    int anygid = anychars<0 ? -1 : fv->map->map[anychars];

    gtk_widget_set_sensitive(lookup_widget( menuitem, "default_att1" ),
	    anygid>=0 );
    for ( ofv=fv_list; ofv!=NULL && ofv->sf==fv->sf; ofv = ofv->next );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "copy_features_to_font1" ),
	    ofv!=NULL );
}

void FontViewMenu_ActivateBuild(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int anybuildable = false;
    int anybuildableaccents = false;
    int i, gid;

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] && (gid=fv->map->map[i])!=-1 ) {
	SplineChar *sc, dummy;
	sc = fv->sf->glyphs[gid];
	if ( sc==NULL )
	    sc = SCBuildDummy(&dummy,fv->sf,fv->map,i);
	if ( SFIsSomethingBuildable(fv->sf,sc,true)) {
	    anybuildableaccents = anybuildable = true;
    break;
	else if ( SFIsSomethingBuildable(fv->sf,sc,false))
	    anybuildable = true;
    }

    gtk_widget_set_sensitive(lookup_widget( menuitem, "build_accented_char1" ),
	    anybuildableaccents );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "build_composite_char1" ),
	    anybuildable );
}

void FontViewMenu_ActivateElement(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int anychars = FVAnyCharSelected(fv);
    int anygid = anychars<0 ? -1 : fv->map->map[anychars];
    int anybuildable, anytraceable;
    int order2 = fv->sf->order2;

    gtk_widget_set_sensitive(lookup_widget( menuitem, "char_info1" ),
	    anygid>=0 &&
		    (fv->cidmaster==NULL || fv->sf->glyphs[anygid]!=NULL));

    gtk_widget_set_sensitive(lookup_widget( menuitem, "find_problems1" ),
	    anygid!=-1);

    gtk_widget_set_sensitive(lookup_widget( menuitem, "transform1" ),
	    anygid!=-1);
    gtk_widget_set_sensitive(lookup_widget( menuitem, "non_linear_transform1" ),
	    anygid!=-1);

    gtk_widget_set_sensitive(lookup_widget( menuitem, "add_extrema1" ),
	    anygid!=-1 && !fv->sf->onlybitmaps );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "round_to_int1" ),
	    anygid!=-1 && !fv->sf->onlybitmaps );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "correct_direction1" ),
	    anygid!=-1 && !fv->sf->onlybitmaps );

    gtk_widget_set_sensitive(lookup_widget( menuitem, "simplify3" ),
	    anygid!=-1 && !fv->sf->onlybitmaps );

    gtk_widget_set_sensitive(lookup_widget( menuitem, "meta_font1" ),
	    anygid!=-1 && !fv->sf->onlybitmaps && !order2 );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "expand_stroke1" ),
	    anygid!=-1 && !fv->sf->onlybitmaps && !order2 );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "overlap1" ),
	    anygid!=-1 && !fv->sf->onlybitmaps && !order2 );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "effects1" ),
	    anygid!=-1 && !fv->sf->onlybitmaps && !order2 );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "tilepath1" ),
	    anygid!=-1 && !fv->sf->onlybitmaps && !order2 );

    gtk_widget_set_sensitive(lookup_widget( menuitem, "bitmaps_available1" ),
	    fv->mm==NULL );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "regenerate_bitmaps1" ),
	    fv->mm==NULL && fv->sf->bitmaps!=NULL && !fv->sf->onlybitmaps );

    anybuildable = false;
    if ( anygid!=-1 ) {
	int i, gid;
	for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] ) {
	    SplineChar *sc=NULL, dummy;
	    if ( (gid=fv->map->map[i])!=-1 )
		sc = fv->sf->glyphs[gid];
	    if ( sc==NULL )
		sc = SCBuildDummy(&dummy,fv->sf,fv->map,i);
	    if ( SFIsSomethingBuildable(fv->sf,sc,false)) {
		anybuildable = true;
	break;
	    }
	}
    }
    gtk_widget_set_sensitive(lookup_widget( menuitem, "build1" ),
	    anybuildable );

    anytraceable = false;
    if ( FindAutoTraceName()!=NULL && anychars!=-1 ) {
	int i;
	for ( i=0; i<fv->map->enccount; ++i )
	    if ( fv->selected[i] && (gid=fv->map->map[i])!=-1 &&
		    fv->sf->glyphs[gid]!=NULL &&
		    fv->sf->glyphs[gid]->layers[ly_back].images!=NULL ) {
		anytraceable = true;
	break;
	    }
    }
    gtk_widget_set_sensitive(lookup_widget( menuitem, "autotrace1" ),
	    anytraceable );

    gtk_widget_set_sensitive(lookup_widget( menuitem, "interpolate_fonts1" ),
	    !fv->sf->onlybitmaps );

#ifndef FONTFORGE_CONFIG_NONLINEAR
    w = lookup_widget( menuitem, "non_linear_transform1" );
    gtk_widget_hide(w);
#endif
#ifndef FONTFORGE_CONFIG_TILEPATH
    w = lookup_widget( menuitem, "tilepath1" );
    gtk_widget_hide(w);
#endif
}

void FontViewMenu_ActivateHints(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int anychars = FVAnyCharSelected(fv);
    int anygid = anychars<0 ? -1 : fv->map->map[anychars];
    int multilayer = fv->sf->multilayer;

    gtk_widget_set_sensitive(lookup_widget( menuitem, "autohint1" ),
	    anygid!=-1 && !multilayer );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "hint_subsitution_pts1" ),
	    !fv->sf->order2 && anygid!=-1 && !multilayer );
    if ( fv->sf->mm!=NULL && fv->sf->mm->apple )
	gtk_widget_set_sensitive(lookup_widget( menuitem, "hint_subsitution_pts1" ),
		false);
    gtk_widget_set_sensitive(lookup_widget( menuitem, "auto_counter_hint1" ),
	    !fv->sf->order2 && anygid!=-1 && !multilayer );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "dont_autohint1" ),
	    !fv->sf->order2 && anygid!=-1 && !multilayer );

    gtk_widget_set_sensitive(lookup_widget( menuitem, "autoinstr1" ),
	    fv->sf->order2 && anygid!=-1 && !multilayer );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "edit_instructions1" ),
	    fv->sf->order2 && anygid>=0 && !multilayer );

    gtk_widget_set_sensitive(lookup_widget( menuitem, "edit_fpgm1" ),
	    fv->sf->order2 && !multilayer );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "edit_prep1" ),
	    fv->sf->order2 && !multilayer );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "edit_cvt_1" ),
	    fv->sf->order2 && !multilayer );

    gtk_widget_set_sensitive(lookup_widget( menuitem, "clear_hints1" ),
	    anygid!=-1 );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "clear_width_md1" ),
	    anygid!=-1 );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "clear_instructions1" ),
	    fv->sf->order2 && anygid!=-1 );
}

/* Builds up a menu containing all the anchor classes */
void FontViewMenu_ActivateAnchoredPairs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    _aplistbuild(menuitem,fv->sf,FVMenuAnchorPairs);
}

void FontViewMenu_ActivateCombinations(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SplineFont *sf = fv->sf;
    int i, gid, anyligs=0, anykerns=0;
    PST *pst;

    if ( sf->kerns ) anykerns=true;
    for ( i=0; i<map->enccount; ++i ) if ( (gid=fv->map->map[i]!=-1 && sf->glyphs[gid]!=NULL ) {
	for ( pst=sf->glyphs[gid]->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->type==pst_ligature ) {
		anyligs = true;
		if ( anykerns )
    break;
	    }
	}
	if ( sf->glyphs[gid]->kerns!=NULL ) {
	    anykerns = true;
	    if ( anyligs )
    break;
	}
    }

    gtk_widget_set_sensitive(lookup_widget( menuitem, "ligatures2" ),
	    anyligs);
    gtk_widget_set_sensitive(lookup_widget( menuitem, "kern_pairs1" ),
	    anykerns);
    gtk_widget_set_sensitive(lookup_widget( menuitem, "anchored_pairs1" ),
	    sf->anchor!=NULL);
}

void FontViewMenu_ActivateView(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int anychars = FVAnyCharSelected(fv);
    int i,gid;
    BDFFont *bdf;
    int pos;
    SplineFont *sf = fv->sf;
    EncMap *map = fv->map;
    GtkWidget *menu = gtk_menu_item_get_submenu(menuitem);
    GList *kids, *next;
    static int sizes[] = { 24, 36, 48, 72, 96, 0 };

    /* First remove anything we might have added previously */
    for ( kids = GTK_MENU_SHELL(menu)->children; kids!=NULL; kids = next ) {
	GtkWidget *w = kids->data;
	next = kids->next;
	if ( strcmp(gtk_widget_get_name(w),"temporary")==0 )
	    gtk_container_remove(GTK_CONTAINER(menu),w);
    }

    /* Then add any pre-built bitmap sizes */
    if ( sf->bitmaps!=NULL ) {
	GtkWidget *w;

	w = gtk_separator_menu_item_new();
	gtk_widget_show(w);
	gtk_widget_set_name(w,"temporary");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),w);

	for ( bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( BDFDepth(bdf)==1 )
		sprintf( buffer, _("%d pixel bitmap"), bdf->pixelsize );
	    else
		sprintf( buffer, _("%d@%d pixel bitmap"),
			bdf->pixelsize, BDFDepth(bdf) );
	    w = gtk_check_menu_item_new_with_label( buffer );
	    gtk_widget_show(w);
	    gtk_widget_set_name(w,"temporary");
	    gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(w), bdf==fv->show );
	    g_signal_connect ( G_OBJECT( w ), "activate",
			    G_CALLBACK (FontViewMenu_ShowBitmap),
			    bdf);
	    gtk_menu_shell_append(GTK_MENU_SHELL(menu),w);
	}
    }

    gtk_widget_set_sensitive(lookup_widget( menuitem, "next_char1" ),
	    anychars>=0 );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "prev_char1" ),
	    anychars>=0 );
    if ( anychars<0 ) pos = map->enccount;
    else for ( pos = anychars+1; pos<map->enccount &&
	    (map->map[pos]==-1 || !SCWorthOutputting(sf->glyphs[map->map[pos]]));
	    ++pos );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "prev_char1" ),
	    pos!=map->enccount );
    for ( pos=anychars-1; pos>=0 &&
	    (map->map[pos]==-1 || !SCWorthOutputting(sf->glyphs[map->map[pos]]));
	    --pos );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "prev_char1" ),
	    pos>=0 );
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
		lookup_widget( menuitem, "encoded_view1" )),
	    !fv->sf->compacted);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
		lookup_widget( menuitem, "compacted_view1" )),
	    fv->sf->compacted);
    gtk_widget_set_sensitive(lookup_widget( menuitem, "show_v_metrics1" ),
	    fv->sf->hasvmetrics );

    for ( i=0; sizes[i]!=NULL; ++i ) {
	char buffer[80];
	sprintf( buffer, "%d_pixel_outline1", sizes[i]);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
		    lookup_widget( menuitem, buffer )),
		(fv->show!=NULL && fv->show==fv->filled && fv->show->pixelsize==sizes[i]));
	gtk_widget_set_sensitive(lookup_widget( menuitem, buffer ),
		!fv->sf->onlybitmaps || fv->show==fv->filled );
    }

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
		lookup_widget( menuitem, "anti_alias1" )),
	    (fv->show!=NULL && fv->show->clut!=NULL));
    gtk_widget_set_sensitive(lookup_widget( menuitem, "anti_alias1" ),
	    !fv->sf->onlybitmaps || fv->show==fv->filled );
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
		lookup_widget( menuitem, "fit_to_em1" )),
	    (fv->show!=NULL && !fv->show->bbsized));
    gtk_widget_set_sensitive(lookup_widget( menuitem, "fit_to_em1" ),
	    !fv->sf->onlybitmaps || fv->show==fv->filled );
}

void FontViewMenu_ActivateMetrics(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int anychars = FVAnyCharSelected(fv);
    char *argnames[] = { "center_in_width1", "thirds_in_width1", "set_width1",
	    "set_lbearing1", "set_rbearing1", "set_vertical_advance1", NULL };
    char *vnames[] = { "vkern_by_classes1", "vkern_from_hkern1",
	    "remove_all_vkern_pairs1", NULL };

    for ( i=0; argnames[i]!=NULL; ++i )
	gtk_widget_set_sensitive(lookup_widget( menuitem, argnames[i] ),
		anychars!=-1 );
    if ( !fv->sf->hasvmetrics )
	gtk_widget_set_sensitive(lookup_widget( menuitem, "set_vertical_advance1" ),
		false );
    for ( i=0; vnames[i]!=NULL; ++i )
	gtk_widget_set_sensitive(lookup_widget( menuitem, vnames[i] ),
		fv->sf->hasvmetrics );
}

void FontViewMenu_ActivateCID(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int i, j;
    SplineFont *sub, *cidmaster = fv->cidmaster;
    GtkWidget *menu = gtk_menu_item_get_submenu(menuitem);
    GList *kids, *next;

    /* First remove anything we might have added previously */
    for ( kids = GTK_MENU_SHELL(menu)->children; kids!=NULL; kids = next ) {
	GtkWidget *w = kids->data;
	next = kids->next;
	if ( strcmp(gtk_widget_get_name(w),"temporary")==0 )
	    gtk_container_remove(GTK_CONTAINER(menu),w);
    }

    /* Then add any sub-fonts */
    if ( cidmaster!=NULL ) {
	GtkWidget *w;

	w = gtk_separator_menu_item_new();
	gtk_widget_show(w);
	gtk_widget_set_name(w,"temporary");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),w);

	for ( j = 0; j<cidmaster->subfontcnt; ++j ) {
	    sub = cidmaster->subfonts[j];
	    w = gtk_check_menu_item_new_with_label( sub->fontname );
	    gtk_widget_show(w);
	    gtk_widget_set_name(w,"temporary");
	    gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(w), sub==fv->sf );
	    g_signal_connect ( G_OBJECT( w ), "activate",
			    G_CALLBACK (FontViewMenu_ShowSubFont),
			    sub);
	    gtk_menu_shell_append(GTK_MENU_SHELL(menu),w);
	}
    }

    gtk_widget_set_sensitive(lookup_widget( menuitem, "convert_to_cid1" ),
	    cidmaster==NULL && fv->sf->mm==NULL );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "convert_by_cmap1" ),
	    cidmaster==NULL && fv->sf->mm==NULL );

	    /* OpenType allows at most 255 subfonts (PS allows more, but why go to the effort to make safe font check that? */
    gtk_widget_set_sensitive(lookup_widget( menuitem, "insert_font1" ),
	    cidmaster!=NULL && cidmaster->subfontcnt<255 );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "insert_empty_font1" ),
	    cidmaster!=NULL && cidmaster->subfontcnt<255 );

    gtk_widget_set_sensitive(lookup_widget( menuitem, "remove_font1" ),
	    cidmaster!=NULL && cidmaster->subfontcnt>1 );

    gtk_widget_set_sensitive(lookup_widget( menuitem, "flatten1" ),
	    cidmaster!=NULL );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "flatten_by_cmap1" ),
	    cidmaster!=NULL );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "cid_font_info1" ),
	    cidmaster!=NULL );
}

void FontViewMenu_ActivateMM(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int i, base, j;
    MMSet *mm = fv->sf->mm;
    SplineFont *sub;
    GMenuItem *mml;
    GtkWidget *menu = gtk_menu_item_get_submenu(menuitem);
    GList *kids, *next;

    /* First remove anything we might have added previously */
    for ( kids = GTK_MENU_SHELL(menu)->children; kids!=NULL; kids = next ) {
	GtkWidget *w = kids->data;
	next = kids->next;
	if ( strcmp(gtk_widget_get_name(w),"temporary")==0 )
	    gtk_container_remove(GTK_CONTAINER(menu),w);
    }

    /* Then add any sub-fonts */
    if ( mm!=NULL ) {
	GtkWidget *w;

	w = gtk_separator_menu_item_new();
	gtk_widget_show(w);
	gtk_widget_set_name(w,"temporary");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),w);

	for ( j = 0; j<mm->instance_count+1; ++j ) {
	    if ( j==0 )
		sub = mm->normal;
	    else
		sub = mm->instances[j-1];
	    w = gtk_check_menu_item_new_with_label( sub->fontname );
	    gtk_widget_show(w);
	    gtk_widget_set_name(w,"temporary");
	    gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(w), sub==fv->sf );
	    g_signal_connect ( G_OBJECT( w ), "activate",
			    G_CALLBACK (FontViewMenu_ShowSubFont),
			    sub);
	    gtk_menu_shell_append(GTK_MENU_SHELL(menu),w);
	}
    }

    gtk_widget_set_sensitive(lookup_widget( menuitem, "mm_validity_check1" ),
	    mm!=NULL );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "mm_info1" ),
	    mm!=NULL );
    gtk_widget_set_sensitive(lookup_widget( menuitem, "blend_to_new_font1" ),
	    mm!=NULL );

    gtk_widget_set_sensitive(lookup_widget( menuitem, "mm_change_def_weights1" ),
	    mm!=NULL && !mm->apple );
}
#endif

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static int FeatureTrans(FontView *fv, int enc) {
    SplineChar *sc;
    PST *pst;
    char *pt;
    int gid;

    if ( enc<0 || enc>=fv->map->enccount || (gid = fv->map->map[enc])==-1 )
return( -1 );
    if ( fv->cur_subtable==NULL )
return( gid );

    sc = fv->sf->glyphs[gid];
    if ( sc==NULL )
return( -1 );
    for ( pst = sc->possub; pst!=NULL; pst=pst->next ) {
	if (( pst->type == pst_substitution || pst->type == pst_alternate ) &&
		pst->subtable == fv->cur_subtable )
    break;
    }
    if ( pst==NULL )
return( -1 );
    pt = strchr(pst->u.subs.variant,' ');
    if ( pt!=NULL )
	*pt = '\0';
    gid = SFFindExistingSlot(fv->sf, -1, pst->u.subs.variant );
    if ( pt!=NULL )
	*pt = ' ';
return( gid );
}

void FVRefreshChar(FontView *fv,int gid) {
    BDFChar *bdfc;
    int i, j, enc;
    MetricsView *mv;

    /* Can happen in scripts */ /* Can happen if we do an AutoHint when generating a tiny font for freetype context */
    if ( fv->v==NULL || fv->colcnt==0 || fv->sf->glyphs[gid]== NULL )
return;
#if 0
    /* What on earth was this code for? it breaks updates of things like "a.sc"*/
    if ( fv->cur_subtable==NULL && strchr(fv->sf->glyphs[gid]->name,'.')!=NULL ) {
	char *temp = copy(fv->sf->glyphs[gid]->name);
	SplineChar *sc2;
	*strchr(temp,'.') = '\0';
	sc2 = SFGetChar(fv->sf,-1,temp);
	if ( sc2!=NULL && sc2->orig_pos!=gid )
	    gid = sc2->orig_pos;
	free(temp);
    }
#endif

    for ( fv=fv->sf->fv; fv!=NULL; fv = fv->nextsame ) {
	for ( mv=fv->sf->metrics; mv!=NULL; mv=mv->next )
	    MVRefreshChar(mv,fv->sf->glyphs[gid]);
	bdfc = fv->show->glyphs[gid];
	/* A glyph may be encoded in several places, all need updating */
	for ( enc = 0; enc<fv->map->enccount; ++enc ) if ( fv->map->map[enc]==gid ) {
	    i = enc / fv->colcnt;
	    j = enc - i*fv->colcnt;
	    i -= fv->rowoff;
	    if ( i>=0 && i<fv->rowcnt ) {
		struct _GImage base;
		GImage gi;
		GClut clut;
		GRect old, box;

		if ( bdfc==NULL )
		    bdfc = BDFPieceMeal(fv->show,gid);
		if ( bdfc==NULL )
	continue;

		memset(&gi,'\0',sizeof(gi));
		memset(&base,'\0',sizeof(base));
		if ( bdfc->byte_data ) {
		    gi.u.image = &base;
		    base.image_type = it_index;
		    base.clut = fv->show->clut;
		    GDrawSetDither(NULL, false);	/* on 8 bit displays we don't want any dithering */
		    base.trans = -1;
		    /*base.clut->trans_index = 0;*/
		} else {
		    memset(&clut,'\0',sizeof(clut));
		    gi.u.image = &base;
		    base.image_type = it_mono;
		    base.clut = &clut;
		    clut.clut_len = 2;
		    clut.clut[0] = GDrawGetDefaultBackground(NULL);
		}

		base.data = bdfc->bitmap;
		base.bytes_per_line = bdfc->bytes_per_line;
		base.width = bdfc->xmax-bdfc->xmin+1;
		base.height = bdfc->ymax-bdfc->ymin+1;
		box.x = j*fv->cbw+1; box.width = fv->cbw-1;
		box.y = i*fv->cbh+fv->lab_height+1; box.height = fv->cbw;
		GDrawPushClip(fv->v,&box,&old);
		GDrawFillRect(fv->v,&box,GDrawGetDefaultBackground(NULL));
		if ( fv->magnify>1 ) {
		    GDrawDrawImageMagnified(fv->v,&gi,NULL,
			    j*fv->cbw+(fv->cbw-1-fv->magnify*base.width)/2,
			    i*fv->cbh+fv->lab_height+1+fv->magnify*(fv->show->ascent-bdfc->ymax),
			    fv->magnify*base.width,fv->magnify*base.height);
		} else
		    GDrawDrawImage(fv->v,&gi,NULL,
			    j*fv->cbw+(fv->cbw-1-base.width)/2,
			    i*fv->cbh+fv->lab_height+1+fv->show->ascent-bdfc->ymax);
		GDrawPopClip(fv->v,&old);
		if ( fv->selected[enc] ) {
		    GDrawSetXORBase(fv->v,GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(fv->v)));
		    GDrawSetXORMode(fv->v);
		    GDrawFillRect(fv->v,&box,XOR_COLOR);
		    GDrawSetCopyMode(fv->v);
		}
		GDrawSetDither(NULL, true);
	    }
	}
    }
}

void FVRegenChar(FontView *fv,SplineChar *sc) {
    struct splinecharlist *dlist;
    MetricsView *mv;

    if ( fv->v==NULL )			/* Can happen in scripts */
return;

    /* sc->changedsincelasthinted = true;*/	/* Why was this here? */
    if ( sc->orig_pos>=fv->filled->glyphcnt )
	IError("Character out of bounds in bitmap font %d>=%d", sc->orig_pos, fv->filled->glyphcnt );
    else
	BDFCharFree(fv->filled->glyphs[sc->orig_pos]);
    fv->filled->glyphs[sc->orig_pos] = NULL;
		/* FVRefreshChar does NOT do this for us */
    for ( mv=fv->sf->metrics; mv!=NULL; mv=mv->next )
	MVRegenChar(mv,sc);

    FVRefreshChar(fv,sc->orig_pos);
#if HANYANG
    if ( sc->compositionunit && fv->sf->rules!=NULL )
	Disp_RefreshChar(fv->sf,sc);
#endif

    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next )
	FVRegenChar(fv,dlist->sc);
}
#endif

int32 UniFromEnc(int enc, Encoding *encname) {
    char from[20];
    unichar_t to[20];
    ICONV_CONST char *fpt;
    char *tpt;
    size_t fromlen, tolen;

    if ( encname->is_custom || encname->is_original )
return( -1 );
    if ( enc>=encname->char_cnt )
return( -1 );
    if ( encname->is_unicodebmp || encname->is_unicodefull )
return( enc );
    if ( encname->unicode!=NULL )
return( encname->unicode[enc] );
    else if ( encname->tounicode ) {
	/* To my surprise, on RH9, doing a reset on conversion of CP1258->UCS2 */
	/* causes subsequent calls to return garbage */
	if ( encname->iso_2022_escape_len ) {
	    tolen = sizeof(to); fromlen = 0;
	    iconv(encname->tounicode,NULL,&fromlen,NULL,&tolen);	/* Reset state */
	}
	fpt = from; tpt = (char *) to; tolen = sizeof(to);
	if ( encname->has_1byte && enc<256 ) {
	    *(char *) fpt = enc;
	    fromlen = 1;
	} else if ( encname->has_2byte ) {
	    if ( encname->iso_2022_escape_len )
		strncpy(from,encname->iso_2022_escape,encname->iso_2022_escape_len );
	    fromlen = encname->iso_2022_escape_len;
	    from[fromlen++] = enc>>8;
	    from[fromlen++] = enc&0xff;
	}
	if ( iconv(encname->tounicode,&fpt,&fromlen,&tpt,&tolen)==(size_t) -1 )
return( -1 );
	if ( tpt-(char *) to == 0 ) {
	    /* This strange call appears to be what we need to make CP1258->UCS2 */
	    /*  work.  It's supposed to reset the state and give us the shift */
	    /*  out. As there is no state, and no shift out I have no idea why*/
	    /*  this works, but it does. */
	    if ( iconv(encname->tounicode,NULL,&fromlen,&tpt,&tolen)==(size_t) -1 )
return( -1 );
	}
	if ( tpt-(char *) to == sizeof(unichar_t) )
return( to[0] );
#ifdef UNICHAR_16
	else if ( tpt-(char *) to == 4 && to[0]>=0xd800 && to[0]<0xdc00 && to[1]>=0xdc00 )
return( ((to[0]-0xd800)<<10) + (to[1]-0xdc00) + 0x10000 );
#endif
    } else if ( encname->tounicode_func!=NULL ) {
return( (encname->tounicode_func)(enc) );
    }
return( -1 );
}

int32 EncFromUni(int32 uni, Encoding *enc) {
    unichar_t from[20];
    unsigned char to[20];
    ICONV_CONST char *fpt;
    char *tpt;
    size_t fromlen, tolen;
    int i;

    if ( enc->is_custom || enc->is_original || enc->is_compact || uni==-1 )
return( -1 );
    if ( enc->is_unicodebmp || enc->is_unicodefull )
return( uni<enc->char_cnt ? uni : -1 );

    if ( enc->unicode!=NULL ) {
	for ( i=0; i<enc->char_cnt; ++i ) {
	    if ( enc->unicode[i]==uni )
return( i );
	}
return( -1 );
    } else if ( enc->fromunicode!=NULL ) {
	/* I don't see how there can be any state to reset in this direction */
	/*  So I don't reset it */
#ifdef UNICHAR_16
	if ( uni<0x10000 ) {
	    from[0] = uni;
	    fromlen = sizeof(unichar_t);
	} else {
	    uni -= 0x10000;
	    from[0] = 0xd800 + (uni>>10);
	    from[1] = 0xdc00 + (uni&0x3ff);
	    fromlen = 2*sizeof(unichar_t);
	}
#else
	from[0] = uni;
	fromlen = sizeof(unichar_t);
#endif
	fpt = (char *) from; tpt = (char *) to; tolen = sizeof(to);
	iconv(enc->fromunicode,NULL,NULL,NULL,NULL);	/* reset shift in/out, etc. */
	if ( iconv(enc->fromunicode,&fpt,&fromlen,&tpt,&tolen)==(size_t) -1 )
return( -1 );
	if ( tpt-(char *) to == 1 )
return( to[0] );
	if ( enc->iso_2022_escape_len!=0 ) {
	    if ( tpt-(char *) to == enc->iso_2022_escape_len+2 &&
		    strncmp((char *) to,enc->iso_2022_escape,enc->iso_2022_escape_len)==0 )
return( (to[enc->iso_2022_escape_len]<<8) | to[enc->iso_2022_escape_len+1] );
	} else {
	    if ( tpt-(char *) to == sizeof(unichar_t) )
return( (to[0]<<8) | to[1] );
	}
    } else if ( enc->fromunicode_func!=NULL ) {
return( (enc->fromunicode_func)(uni) );
    }
return( -1 );
}

int32 EncFromName(const char *name,enum uni_interp interp,Encoding *encname) {
    int i;
    if ( encname->psnames!=NULL ) {
	for ( i=0; i<encname->char_cnt; ++i )
	    if ( encname->psnames[i]!=NULL && strcmp(name,encname->psnames[i])==0 )
return( i );
    }
    i = UniFromName(name,interp,encname);
    if ( i==-1 && strlen(name)==4 ) {
	/* MS says use this kind of name, Adobe says use the one above */
	char *end;
	i = strtol(name,&end,16);
	if ( i<0 || i>0xffff || *end!='\0' )
return( -1 );
    }
return( EncFromUni(i,encname));
}

SplineChar *SCBuildDummy(SplineChar *dummy,SplineFont *sf,EncMap *map,int i) {
    static char namebuf[100];
#ifdef FONTFORGE_CONFIG_TYPE3
    static Layer layers[2];
#endif

    memset(dummy,'\0',sizeof(*dummy));
    dummy->color = COLOR_DEFAULT;
    dummy->layer_cnt = 2;
#ifdef FONTFORGE_CONFIG_TYPE3
    dummy->layers = layers;
#endif
    if ( sf->cidmaster!=NULL ) {
	/* CID fonts don't have encodings, instead we must look up the cid */
	if ( sf->cidmaster->loading_cid_map )
	    dummy->unicodeenc = -1;
	else
	    dummy->unicodeenc = CID2NameUni(FindCidMap(sf->cidmaster->cidregistry,sf->cidmaster->ordering,sf->cidmaster->supplement,sf->cidmaster),
		    i,namebuf,sizeof(namebuf));
    } else
	dummy->unicodeenc = UniFromEnc(i,map->enc);

    if ( sf->cidmaster!=NULL )
	dummy->name = namebuf;
    else if ( map->enc->psnames!=NULL && i<map->enc->char_cnt &&
	    map->enc->psnames[i]!=NULL )
	dummy->name = map->enc->psnames[i];
    else if ( dummy->unicodeenc==-1 )
	dummy->name = NULL;
    else
	dummy->name = (char *) StdGlyphName(namebuf,dummy->unicodeenc,sf->uni_interp,sf->for_new_glyphs);
    if ( dummy->name==NULL ) {
	/*if ( dummy->unicodeenc!=-1 || i<256 )
	    dummy->name = ".notdef";
	else*/ {
	    int j;
	    sprintf( namebuf, "NameMe.%d", i);
	    j=0;
	    while ( SFFindExistingSlot(sf,-1,namebuf)!=-1 )
		sprintf( namebuf, "NameMe.%d.%d", i, ++j);
	    dummy->name = namebuf;
	}
    }
    dummy->width = dummy->vwidth = sf->ascent+sf->descent;
    if ( dummy->unicodeenc>0 && dummy->unicodeenc<0x10000 &&
	    iscombining(dummy->unicodeenc)) {
	/* Mark characters should be 0 width */
	dummy->width = 0;
	/* Except in monospaced fonts on windows, where they should be the */
	/*  same width as everything else */
    }
    /* Actually, in a monospace font, all glyphs should be the same width */
    /*  whether mark or not */
    if ( sf->pfminfo.panose_set && sf->pfminfo.panose[3]==9 &&
	    sf->glyphcnt>0 ) {
	for ( i=sf->glyphcnt-1; i>=0; --i )
	    if ( SCWorthOutputting(sf->glyphs[i])) {
		dummy->width = sf->glyphs[i]->width;
	break;
	    }
    }
    dummy->parent = sf;
    dummy->orig_pos = 0xffff;
return( dummy );
}

static SplineChar *_SFMakeChar(SplineFont *sf,EncMap *map,int enc) {
    SplineChar dummy, *sc;
    SplineFont *ssf;
    int j, real_uni, gid;
    extern const int cns14pua[], amspua[];

    if ( enc>=map->enccount )
	gid = -1;
    else
	gid = map->map[enc];
    if ( sf->subfontcnt!=0 && gid!=-1 ) {
	ssf = NULL;
	for ( j=0; j<sf->subfontcnt; ++j )
	    if ( gid<sf->subfonts[j]->glyphcnt ) {
		ssf = sf->subfonts[j];
		if ( ssf->glyphs[gid]!=NULL ) {
return( ssf->glyphs[gid] );
		}
	    }
	sf = ssf;
    }

    if ( gid==-1 || (sc = sf->glyphs[gid])==NULL ) {
	if (( map->enc->is_unicodebmp || map->enc->is_unicodefull ) &&
		( enc>=0xe000 && enc<=0xf8ff ) &&
		( sf->uni_interp==ui_ams || sf->uni_interp==ui_trad_chinese ) &&
		( real_uni = (sf->uni_interp==ui_ams ? amspua : cns14pua)[enc-0xe000])!=0 ) {
	    if ( real_uni<map->enccount ) {
		SplineChar *sc;
		/* if necessary, create the real unicode code point */
		/*  and then make us be a duplicate of it */
		sc = _SFMakeChar(sf,map,real_uni);
		map->map[enc] = gid = sc->orig_pos;
		SCCharChangedUpdate(sc);
return( sc );
	    }
	}

	SCBuildDummy(&dummy,sf,map,enc);
	if ((sc = SFGetChar(sf,dummy.unicodeenc,dummy.name))!=NULL ) {
	    map->map[enc] = sc->orig_pos;
return( sc );
	}
	sc = SplineCharCreate();
	sc->unicodeenc = dummy.unicodeenc;
	sc->name = copy(dummy.name);
	sc->width = dummy.width;
	sc->parent = sf;
	sc->orig_pos = 0xffff;
	/*SCLigDefault(sc);*/
	SFAddGlyphAndEncode(sf,sc,map,enc);
    }
return( sc );
}

SplineChar *SFMakeChar(SplineFont *sf,EncMap *map, int enc) {
    int gid;

    if ( enc==-1 )
return( NULL );
    if ( enc>=map->enccount )
	gid = -1;
    else
	gid = map->map[enc];
    if ( sf->mm!=NULL && (gid==-1 || sf->glyphs[gid]==NULL) ) {
	int j;
	_SFMakeChar(sf->mm->normal,map,enc);
	for ( j=0; j<sf->mm->instance_count; ++j )
	    _SFMakeChar(sf->mm->instances[j],map,enc);
    }
return( _SFMakeChar(sf,map,enc));
}

#if !defined(FONTFORGE_CONFIG_NO_WINDOWING_UI)
static void AddSubPST(SplineChar *sc,struct lookup_subtable *sub,char *variant) {
    PST *pst;

    pst = chunkalloc(sizeof(PST));
    pst->type = pst_substitution;
    pst->subtable = sub;
    pst->u.alt.components = copy(variant);
    pst->next = sc->possub;
    sc->possub = pst;
}

SplineChar *FVMakeChar(FontView *fv,int enc) {
    SplineFont *sf = fv->sf;
    SplineChar *base_sc = SFMakeChar(sf,fv->map,enc), *feat_sc = NULL;
    int feat_gid = FeatureTrans(fv,enc);

    if ( fv->cur_subtable==NULL )
return( base_sc );

    if ( feat_gid==-1 ) {
	int uni = -1;
	FeatureScriptLangList *fl = fv->cur_subtable->lookup->features;

	if ( base_sc->unicodeenc>=0x600 && base_sc->unicodeenc<=0x6ff &&
		fl!=NULL &&
		(fl->featuretag == CHR('i','n','i','t') ||
		 fl->featuretag == CHR('m','e','d','i') ||
		 fl->featuretag == CHR('f','i','n','a') ||
		 fl->featuretag == CHR('i','s','o','l')) ) {
	    uni = fl->featuretag == CHR('i','n','i','t') ? ArabicForms[base_sc->unicodeenc-0x600].initial  :
		  fl->featuretag == CHR('m','e','d','i') ? ArabicForms[base_sc->unicodeenc-0x600].medial   :
		  fl->featuretag == CHR('f','i','n','a') ? ArabicForms[base_sc->unicodeenc-0x600].final    :
		  fl->featuretag == CHR('i','s','o','l') ? ArabicForms[base_sc->unicodeenc-0x600].isolated :
		  -1;
	    feat_sc = SFGetChar(sf,uni,NULL);
	    if ( feat_sc!=NULL )
return( feat_sc );
	}
	feat_sc = SplineCharCreate();
	feat_sc->parent = sf;
	feat_sc->unicodeenc = uni;
	if ( uni!=-1 ) {
	    feat_sc->name = galloc(8);
	    feat_sc->unicodeenc = uni;
	    sprintf( feat_sc->name,"uni%04X", uni );
	} else if ( fv->cur_subtable->suffix!=NULL ) {
	    feat_sc->name = galloc(strlen(base_sc->name)+strlen(fv->cur_subtable->suffix)+2);
	    sprintf( feat_sc->name, "%s.%s", base_sc->name, fv->cur_subtable->suffix );
	} else if ( fl==NULL ) {
	    feat_sc->name = strconcat(base_sc->name,".unknown");
	} else if ( fl->ismac ) {
	    /* mac feature/setting */
	    feat_sc->name = galloc(strlen(base_sc->name)+14);
	    sprintf( feat_sc->name,"%s.m%d_%d", base_sc->name,
		    (int) (fl->featuretag>>16),
		    (int) ((fl->featuretag)&0xffff) );
	} else {
	    /* OpenType feature tag */
	    feat_sc->name = galloc(strlen(base_sc->name)+6);
	    sprintf( feat_sc->name,"%s.%c%c%c%c", base_sc->name,
		    (int) (fl->featuretag>>24),
		    (int) ((fl->featuretag>>16)&0xff),
		    (int) ((fl->featuretag>>8)&0xff),
		    (int) ((fl->featuretag)&0xff) );
	}
	SFAddGlyphAndEncode(sf,feat_sc,fv->map,fv->map->enccount);
	AddSubPST(base_sc,fv->cur_subtable,feat_sc->name);
return( feat_sc );
    } else
return( base_sc );
}
#else
SplineChar *FVMakeChar(FontView *fv,int enc) {
    SplineFont *sf = fv->sf;
return( SFMakeChar(sf,fv->map,enc) );
}
#endif

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static GImage *GImageCropAndRotate(GImage *unrot) {
    struct _GImage *unbase = unrot->u.image, *rbase;
    int xmin = unbase->width, xmax = -1, ymin = unbase->height, ymax = -1;
    int i,j, ii;
    GImage *rot;

    for ( i=0; i<unbase->height; ++i ) {
	for ( j=0; j<unbase->width; ++j ) {
	    if ( !(unbase->data[i*unbase->bytes_per_line+(j>>3)]&(0x80>>(j&7))) ) {
		if ( j<xmin ) xmin = j;
		if ( j>xmax ) xmax = j;
		if ( i>ymax ) ymax = i;
		if ( i<ymin ) ymin = i;
	    }
	}
    }
    if ( xmax==-1 )
return( NULL );

    rot = GImageCreate(it_mono,ymax-ymin+1,xmax-xmin+1);
    if ( rot==NULL )
return( NULL );
    rbase = rot->u.image;
    memset(rbase->data,-1,rbase->height*rbase->bytes_per_line);
    for ( i=ymin; i<=ymax; ++i ) {
	for ( j=xmin; j<=xmax; ++j ) {
	    if ( !(unbase->data[i*unbase->bytes_per_line+(j>>3)]&(0x80>>(j&7)) )) {
		ii = ymax-i;
		rbase->data[(j-xmin)*rbase->bytes_per_line+(ii>>3)] &= ~(0x80>>(ii&7));
	    }
	}
    }
    rbase->trans = 1;
return( rot );
}

static GImage *UniGetRotatedGlyph(FontView *fv, SplineChar *sc,int uni) {
    SplineFont *sf = fv->sf;
    int cid=-1;
    static GWindow pixmap=NULL;
    GRect r;
    unichar_t buf[2];
    GImage *unrot, *rot;
    SplineFont *cm = sf->cidmaster;

    if ( uni!=-1 )
	/* Do nothing */;
    else if ( sscanf(sc->name,"vertuni%x", (unsigned *) &uni)==1 )
	/* All done */;
    else if ( cm!=NULL &&
	    ((cid=CIDFromName(sc->name,cm))!=-1 ||
	     sscanf( sc->name, "cid-%d", &cid)==1 ||		/* Obsolete names */
	     sscanf( sc->name, "vertcid_%d", &cid)==1 ||
	     sscanf( sc->name, "cid_%d", &cid)==1 )) {
	uni = CID2Uni(FindCidMap(cm->cidregistry,cm->ordering,cm->supplement,cm),
		cid);
    }
    if ( uni&0x10000 ) uni -= 0x10000;		/* Bug in some old cidmap files */
    if ( uni<0 || uni>0xffff )
return( NULL );

    if ( pixmap==NULL ) {
	pixmap = GDrawCreateBitmap(NULL,2*fv->lab_height,2*fv->lab_height,NULL);
	if ( pixmap==NULL )
return( NULL );
	GDrawSetFont(pixmap,sf->fv->fontset[0]);
    }
    r.x = r.y = 0;
    r.width = r.height = 2*fv->lab_height;
    GDrawFillRect(pixmap,&r,1);
    buf[0] = uni; buf[1] = 0;
    GDrawDrawText(pixmap,2,fv->lab_height,buf,1,NULL,0);
    unrot = GDrawCopyScreenToImage(pixmap,&r);
    if ( unrot==NULL )
return( NULL );

    rot = GImageCropAndRotate(unrot);
    GImageDestroy(unrot);
return( rot );
}

#if 0
static int Use2ByteEnc(FontView *fv,SplineChar *sc, unichar_t *buf,FontMods *mods) {
    int ch1 = sc->enc>>8, ch2 = sc->enc&0xff, newch;
    Encoding *enc = fv->map->enc;
    unsigned short *subtable;

 retry:
    switch ( enc ) {
      case em_big5: case em_big5hkscs:
	if ( !GDrawFontHasCharset(fv->fontset[0],em_big5))
return( false);
	if ( ch1<0xa1 || ch1>0xf9 || ch2<0x40 || ch2>0xfe || sc->enc> 0xf9fe )
return( false );
	mods->has_charset = true; mods->charset = em_big5;
	buf[0] = sc->enc;
	buf[1] = 0;
return( true );
      break;
      case em_sjis:
	if ( !GDrawFontHasCharset(fv->fontset[0],em_jis208))
return( false);
	if ( ch1>=129 && ch1<=159 )
	    ch1-=112;
	else if ( ch1>=0xe0 && ch1<=0xef )
	    ch1-=176;
	else
return( false );
	ch1<<=1;
	if ( ch2 == 127 )
return( false );
	else if ( ch2>=159 )
	    ch2-=126;
	else if ( ch2>127 ) {
	    --ch1;
	    ch2 -= 32;
	} else {
	    -- ch1;
	    ch2 -= 31;
	}
	mods->has_charset = true; mods->charset = em_jis208;
	buf[0] = (ch1<<8) | ch2;
	buf[1] = 0;
return( true );
      break;
      case em_wansung:
	if ( !GDrawFontHasCharset(fv->fontset[0],em_ksc5601))
return( false);
	if ( ch1<0xa1 || ch1>0xfd || ch2<0xa1 || ch2>0xfe || sc->enc > 0xfdfe )
return( false );
	mods->has_charset = true; mods->charset = em_ksc5601;
	buf[0] = sc->enc-0x8080;
	buf[1] = 0;
return( true );
      break;
      case em_jisgb:
	if ( !GDrawFontHasCharset(fv->fontset[0],em_gb2312))
return( false);
	if ( ch1<0xa1 || ch1>0xfd || ch2<0xa1 || ch2>0xfe || sc->enc > 0xfdfe )
return( false );
	mods->has_charset = true; mods->charset = em_gb2312;
	buf[0] = sc->enc-0x8080;
	buf[1] = 0;
return( true );
      break;
      case em_ksc5601: case em_jis208: case em_jis212: case em_gb2312:
	if ( !GDrawFontHasCharset(fv->fontset[0],enc))
return( false);
	if ( ch1<0x21 || ch1>0x7e || ch2<0x21 || ch2>0x7e )
return( false );
	mods->has_charset = true; mods->charset = enc;
	buf[0] = (ch1<<8)|ch2;
	buf[1] = 0;
return( true );
      break;
      default:
    /* If possible, look at the unicode font using the appropriate glyphs */
    /*  for the CJ language for which the font was designed */
	ch1 = sc->unicodeenc>>8, ch2 = sc->unicodeenc&0xff;
	switch ( fv->sf->uni_interp ) {
	  case ui_japanese:
	    if ( ch1>=jis_from_unicode.first && ch1<=jis_from_unicode.last &&
		    (subtable = jis_from_unicode.table[ch1-jis_from_unicode.first])!=NULL &&
		    (newch = subtable[ch2])!=0 ) {
		if ( newch&0x8000 ) {
		    if ( GDrawFontHasCharset(fv->fontset[0],em_jis212)) {
			enc = em_jis212;
			newch &= ~0x8000;
			ch1 = newch>>8; ch2 = newch&0xff;
		    } else
return( false );
		} else {
		    if ( GDrawFontHasCharset(fv->fontset[0],em_jis208)) {
			enc = em_jis208;
			ch1 = newch>>8; ch2 = newch&0xff;
		    } else
return( false );
		}
	    } else
return( false );
	  break;
	  case ui_korean:
	    /* Don't know what to do about korean hanga chars */
	    /* No ambiguity for hangul */
return( false );
	  break;
	  case ui_trad_chinese:
	    if ( ch1>=big5hkscs_from_unicode.first && ch1<=big5hkscs_from_unicode.last &&
		    (subtable = big5hkscs_from_unicode.table[ch1-big5hkscs_from_unicode.first])!=NULL &&
		    (newch = subtable[ch2])!=0 &&
		    GDrawFontHasCharset(fv->fontset[0],em_big5)) {
		enc = em_big5hkscs;
		ch1 = newch>>8; ch2 = newch&0xff;
	    } else
return( false );
	  break;
	  case ui_simp_chinese:
	    if ( ch1>=gb2312_from_unicode.first && ch1<=gb2312_from_unicode.last &&
		    (subtable = gb2312_from_unicode.table[ch1-gb2312_from_unicode.first])!=NULL &&
		    (newch = subtable[ch2])!=0 &&
		    GDrawFontHasCharset(fv->fontset[0],em_gb2312)) {
		enc = em_gb2312;
		ch1 = newch>>8; ch2 = newch&0xff;
	    } else
return( false );
	  break;
	  default:
return( false );
	}
 goto retry;
    }
}
#endif

/* Mathmatical Alphanumeric Symbols in the 1d400-1d7ff range are styled */
/*  variants on latin, greek, and digits				*/
#define _uni_bold	0x1
#define _uni_italic	0x2
#define _uni_script	(1<<2)
#define _uni_fraktur	(2<<2)
#define _uni_doublestruck	(3<<2)
#define _uni_sans	(4<<2)
#define _uni_mono	(5<<2)
#define _uni_fontmax	(6<<2)
#define _uni_latin	0
#define _uni_greek	1
#define _uni_digit	2

static int latinmap[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '\0'
};
static int greekmap[] = {
    0x391, 0x392, 0x393, 0x394, 0x395, 0x396, 0x397, 0x398, 0x399, 0x39a,
    0x39b, 0x39c, 0x39d, 0x39e, 0x39f, 0x3a0, 0x3a1, 0x3f4, 0x3a3, 0x3a4,
    0x3a5, 0x3a6, 0x3a7, 0x3a8, 0x3a9, 0x2207,
    0x3b1, 0x3b2, 0x3b3, 0x3b4, 0x3b5, 0x3b6, 0x3b7, 0x3b8, 0x3b9, 0x3ba,
    0x3bb, 0x3bc, 0x3bd, 0x3be, 0x3bf, 0x3c0, 0x3c1, 0x3c2, 0x3c3, 0x3c4,
    0x3c5, 0x3c6, 0x3c7, 0x3c8, 0x3c9,
    0x2202, 0x3f5, 0x3d1, 0x3f0, 0x3d5, 0x3f1, 0x3d6,
    0
};
static int digitmap[] = { '0', '1', '2', '3', '4', '5', '6','7','8','9', '\0' };
static int *maps[] = { latinmap, greekmap, digitmap };

static struct { int start, last; int styles; int charset; } mathmap[] = {
    { 0x1d400, 0x1d433, _uni_bold,		_uni_latin },
    { 0x1d434, 0x1d467, _uni_italic,		_uni_latin },
    { 0x1d468, 0x1d49b, _uni_bold|_uni_italic,	_uni_latin },
    { 0x1d49c, 0x1d4cf, _uni_script,		_uni_latin },
    { 0x1d4d0, 0x1d503, _uni_script|_uni_bold,	_uni_latin },
    { 0x1d504, 0x1d537, _uni_fraktur,		_uni_latin },
    { 0x1d538, 0x1d56b, _uni_doublestruck,	_uni_latin },
    { 0x1d56c, 0x1d59f, _uni_fraktur|_uni_bold,	_uni_latin },
    { 0x1d5a0, 0x1d5d3, _uni_sans,		_uni_latin },
    { 0x1d5d4, 0x1d607, _uni_sans|_uni_bold,	_uni_latin },
    { 0x1d608, 0x1d63b, _uni_sans|_uni_italic,	_uni_latin },
    { 0x1d63c, 0x1d66f, _uni_sans|_uni_bold|_uni_italic,	_uni_latin },
    { 0x1d670, 0x1d6a3, _uni_mono,		_uni_latin },
    { 0x1d6a8, 0x1d6e1, _uni_bold,		_uni_greek },
    { 0x1d6e2, 0x1d71b, _uni_italic,		_uni_greek },
    { 0x1d71c, 0x1d755, _uni_bold|_uni_italic,	_uni_greek },
    { 0x1d756, 0x1d78f, _uni_sans|_uni_bold,	_uni_greek },
    { 0x1d790, 0x1d7c9, _uni_sans|_uni_bold|_uni_italic,	_uni_greek },
    { 0x1d7ce, 0x1d7d7, _uni_bold,		_uni_digit },
    { 0x1d7d8, 0x1d7e1, _uni_doublestruck,	_uni_digit },
    { 0x1d7e2, 0x1d7eb, _uni_sans,		_uni_digit },
    { 0x1d7ec, 0x1d7f5, _uni_sans|_uni_bold,	_uni_digit },
    { 0x1d7f6, 0x1d7ff, _uni_mono,		_uni_digit },
    { 0, 0 }
};
    
static GFont *FVCheckFont(FontView *fv,int type) {
    FontRequest rq;
    int family = type>>2;
    char *fontnames;
    unichar_t *ufontnames;

    static char *resourcenames[] = { "FontView.SerifFamily", "FontView.ScriptFamily",
	    "FontView.FrakturFamily", "FontView.DoubleStruckFamily",
	    "FontView.SansFamily", "FontView.MonoFamily", NULL };
    static char *defaultfontnames[] = {
	    "times,serif,caslon,clearlyu,unifont",
	    "script,formalscript,clearlyu,unifont",
	    "fraktur,clearlyu,unifont",
	    "doublestruck,clearlyu,unifont",
	    "helvetica,caliban,sansserif,sans,clearlyu,unifont",
	    "courier,monospace,caslon,clearlyu,unifont",
	    NULL
	};

    if ( fv->fontset[type]==NULL ) {
	fontnames = GResourceFindString(resourcenames[family]);
	if ( fontnames==NULL )
	    fontnames = defaultfontnames[family];
	ufontnames = uc_copy(fontnames);

	memset(&rq,0,sizeof(rq));
	rq.family_name = ufontnames;
	rq.point_size = -13;
	rq.weight = (type&_uni_bold) ? 700:400;
	rq.style = (type&_uni_italic) ? fs_italic : 0;
	fv->fontset[type] = GDrawInstanciateFont(GDrawGetDisplayOfWindow(fv->v),&rq);
    }
return( fv->fontset[type] );
}

extern unichar_t adobes_pua_alts[0x200][3];

static void do_Adobe_Pua(unichar_t *buf,int sob,int uni) {
    int i, j;

    for ( i=j=0; j<sob-1 && i<3; ++i ) {
	int ch = adobes_pua_alts[uni-0xf600][i];
	if ( ch==0 )
    break;
	if ( ch>=0xf600 && ch<=0xf7ff && adobes_pua_alts[ch-0xf600]!=0 ) {
	    do_Adobe_Pua(buf+j,sob-j,ch);
	    while ( buf[j]!=0 ) ++j;
	} else
	    buf[j++] = ch;
    }
    buf[j] = 0;
}

static void FVExpose(FontView *fv,GWindow pixmap,GEvent *event) {
    int i, j, width, gid;
    int changed;
    GRect old, old2, box, r;
    GClut clut;
    struct _GImage base;
    GImage gi;
    SplineChar dummy;
    int styles, laststyles=0;
    GImage *rotated=NULL;
    int em = fv->sf->ascent+fv->sf->descent;
    int yorg = fv->magnify*(fv->show->ascent-fv->sf->vertical_origin*fv->show->pixelsize/em);
    Color bg, def_fg;

    def_fg = GDrawGetDefaultForeground(NULL);
    memset(&gi,'\0',sizeof(gi));
    memset(&base,'\0',sizeof(base));
    if ( fv->show->clut!=NULL ) {
	gi.u.image = &base;
	base.image_type = it_index;
	base.clut = fv->show->clut;
	GDrawSetDither(NULL, false);
	base.trans = -1;
	/*base.clut->trans_index = 0;*/
    } else {
	memset(&clut,'\0',sizeof(clut));
	gi.u.image = &base;
	base.image_type = it_mono;
	base.clut = &clut;
	clut.clut_len = 2;
	clut.clut[0] = GDrawGetDefaultBackground(NULL);
    }

    GDrawSetFont(pixmap,fv->fontset[0]);
    GDrawSetLineWidth(pixmap,0);
    GDrawPushClip(pixmap,&event->u.expose.rect,&old);
    GDrawFillRect(pixmap,NULL,GDrawGetDefaultBackground(NULL));
    for ( i=0; i<=fv->rowcnt; ++i ) {
	GDrawDrawLine(pixmap,0,i*fv->cbh,fv->width,i*fv->cbh,def_fg);
	GDrawDrawLine(pixmap,0,i*fv->cbh+fv->lab_height,fv->width,i*fv->cbh+fv->lab_height,0x808080);
    }
    for ( i=0; i<=fv->colcnt; ++i )
	GDrawDrawLine(pixmap,i*fv->cbw,0,i*fv->cbw,fv->height,def_fg);
    for ( i=event->u.expose.rect.y/fv->cbh; i<=fv->rowcnt && 
	    (event->u.expose.rect.y+event->u.expose.rect.height+fv->cbh-1)/fv->cbh; ++i ) for ( j=0; j<fv->colcnt; ++j ) {
	int index = (i+fv->rowoff)*fv->colcnt+j;
	int feat_gid;
	styles = 0;
	SplineChar *sc;
	if ( fv->mapping!=NULL ) {
	    if ( index>=fv->mapcnt ) index = fv->map->enccount;
	    else
		index = fv->mapping[index];
	}
	if ( index < fv->map->enccount && index!=-1 ) {
	    unichar_t buf[60]; char cbuf[8];
	    char utf8_buf[8];
	    int use_utf8 = false;
	    Color fg;
	    FontMods *mods=NULL;
	    extern const int amspua[];
	    int uni;
	    struct cidmap *cidmap = NULL;
	    sc = (gid=fv->map->map[index])!=-1 ? fv->sf->glyphs[gid]: NULL;

	    if ( fv->cidmaster!=NULL )
		cidmap = FindCidMap(fv->cidmaster->cidregistry,fv->cidmaster->ordering,fv->cidmaster->supplement,fv->cidmaster);

	    if ( ( fv->map->enc==&custom && index<256 ) ||
		 ( fv->map->enc!=&custom && index<fv->map->enc->char_cnt ) ||
		 ( cidmap!=NULL && index<MaxCID(cidmap) ))
		fg = def_fg;
	    else
		fg = 0x505050;
	    if ( sc==NULL )
		sc = SCBuildDummy(&dummy,fv->sf,fv->map,index);
	    uni = sc->unicodeenc;
	    buf[0] = buf[1] = 0;
	    if ( fv->sf->uni_interp==ui_ams && uni>=0xe000 && uni<=0xf8ff &&
		    amspua[uni-0xe000]!=0 )
		uni = amspua[uni-0xe000];
	    switch ( fv->glyphlabel ) {
	      case gl_name:
		uc_strncpy(buf,sc->name,sizeof(buf)/sizeof(buf[0]));
		styles = _uni_sans;
	      break;
	      case gl_unicode:
		if ( sc->unicodeenc!=-1 ) {
		    sprintf(cbuf,"%04x",sc->unicodeenc);
		    uc_strcpy(buf,cbuf);
		} else
		    uc_strcpy(buf,"?");
		styles = _uni_sans;
	      break;
	      case gl_encoding:
		if ( fv->map->enc->only_1byte ||
			(fv->map->enc->has_1byte && index<256))
		    sprintf(cbuf,"%02x",index);
		else
		    sprintf(cbuf,"%04x",index);
		uc_strcpy(buf,cbuf);
		styles = _uni_sans;
	      break;
	      case gl_glyph:
		if ( uni==0xad )
		    buf[0] = '-';
		else if ( fv->sf->uni_interp==ui_adobe && uni>=0xf600 && uni<=0xf7ff &&
			adobes_pua_alts[uni-0xf600]!=0 ) {
		    use_utf8 = false;
		    do_Adobe_Pua(buf,sizeof(buf),uni);
		} else if ( uni>=0x1d400 && uni<=0x1d7ff ) {
		    int i;
		    for ( i=0; mathmap[i].start!=0; ++i ) {
			if ( uni<=mathmap[i].last ) {
			    buf[0] = maps[mathmap[i].charset][uni-mathmap[i].start];
			    styles = mathmap[i].styles;
		    break;
			}
		    }
		} else if ( uni>=0xe0020 && uni<=0xe007e ) {
		    buf[0] = uni-0xe0000;	/* A map of Ascii for language names */
#if HANYANG
		} else if ( sc->compositionunit ) {
		    if ( sc->jamo<19 )
			buf[0] = 0x1100+sc->jamo;
		    else if ( sc->jamo<19+21 )
			buf[0] = 0x1161 + sc->jamo-19;
		    else	/* Leave a hole for the blank char */
			buf[0] = 0x11a8 + sc->jamo-(19+21+1);
#endif
		} else if ( uni>0 && uni<unicode4_size ) {
		    char *pt = utf8_buf;
		    use_utf8 = true;
		    if ( uni<=0x7f )
			*pt ++ = uni;
		    else if ( uni<=0x7ff ) {
			*pt++ = 0xc0 | (uni>>6);
			*pt++ = 0x80 | (uni&0x3f);
		    } else if ( uni<=0xffff ) {
			*pt++ = 0xe0 | (uni>>12);
			*pt++ = 0x80 | ((uni>>6)&0x3f);
			*pt++ = 0x80 | (uni&0x3f);
		    } else {
			uint32 val = uni-0x10000;
			int u = ((val&0xf0000)>>16)+1, z=(val&0x0f000)>>12, y=(val&0x00fc0)>>6, x=val&0x0003f;
			*pt++ = 0xf0 | (u>>2);
			*pt++ = 0x80 | ((u&3)<<4) | z;
			*pt++ = 0x80 | y;
			*pt++ = 0x80 | x;
		    }
		    *pt = '\0';
		} else {
		    char *pt = strchr(sc->name,'.');
		    buf[0] = '?';
		    fg = 0xff0000;
		    if ( pt!=NULL ) {
			int i, n = pt-sc->name;
			char *end;
			SplineFont *cm = fv->sf->cidmaster;
			if ( n==7 && sc->name[0]=='u' && sc->name[1]=='n' && sc->name[2]=='i' &&
				(i=strtol(sc->name+3,&end,16), end-sc->name==7))
			    buf[0] = i;
			else if ( n>=5 && n<=7 && sc->name[0]=='u' &&
				(i=strtol(sc->name+1,&end,16), end-sc->name==n))
			    buf[0] = i;
			else if ( cm!=NULL && (i=CIDFromName(sc->name,cm))!=-1 ) {
			    int uni;
			    uni = CID2Uni(FindCidMap(cm->cidregistry,cm->ordering,cm->supplement,cm),
				    i);
			    if ( uni!=-1 )
				buf[0] = uni;
			} else {
			    int uni;
			    *pt = '\0';
			    uni = UniFromName(sc->name,fv->sf->uni_interp,fv->map->enc);
			    if ( uni!=-1 )
				buf[0] = uni;
			    *pt = '.';
			}
			if ( strstr(pt,".vert")!=NULL )
			    rotated = UniGetRotatedGlyph(fv,sc,buf[0]!='?'?buf[0]:-1);
			if ( buf[0]!='?' ) {
			    fg = def_fg;
			    if ( strstr(pt,".italic")!=NULL )
				styles = _uni_italic|_uni_mono;
			}
		    } else if ( strncmp(sc->name,"hwuni",5)==0 ) {
			int uni=-1;
			sscanf(sc->name,"hwuni%x", (unsigned *) &uni );
			if ( uni!=-1 ) buf[0] = uni;
		    } else if ( strncmp(sc->name,"italicuni",9)==0 ) {
			int uni=-1;
			sscanf(sc->name,"italicuni%x", (unsigned *) &uni );
			if ( uni!=-1 ) { buf[0] = uni; styles=_uni_italic|_uni_mono; }
			fg = def_fg;
		    } else if ( strncmp(sc->name,"vertcid_",8)==0 ||
			    strncmp(sc->name,"vertuni",7)==0 ) {
			rotated = UniGetRotatedGlyph(fv,sc,-1);
		    }
		}
	      break;
	    }
	    bg = COLOR_DEFAULT;
	    r.x = j*fv->cbw+1; r.width = fv->cbw-1;
	    r.y = i*fv->cbh+1; r.height = fv->lab_height-1;
	    if ( sc->layers[ly_back].splines!=NULL || sc->layers[ly_back].images!=NULL ||
		    sc->color!=COLOR_DEFAULT ) {
		bg = sc->color!=COLOR_DEFAULT?sc->color:0x808080;
		GDrawFillRect(pixmap,&r,bg);
	    }
	    if ( (!fv->sf->order2 && sc->changedsincelasthinted ) ||
		     ( fv->sf->order2 && sc->layers[ly_fore].splines!=NULL &&
			sc->ttf_instrs_len<=0 ) ||
		     ( fv->sf->order2 && sc->instructions_out_of_date ) ) {
		Color hintcol = 0x0000ff;
		if ( fv->sf->order2 && sc->instructions_out_of_date && sc->ttf_instrs_len>0 )
		    hintcol = 0xff0000;
		GDrawDrawLine(pixmap,r.x,r.y,r.x,r.y+r.height-1,hintcol);
		GDrawDrawLine(pixmap,r.x+1,r.y,r.x+1,r.y+r.height-1,hintcol);
		GDrawDrawLine(pixmap,r.x+2,r.y,r.x+2,r.y+r.height-1,hintcol);
		GDrawDrawLine(pixmap,r.x+r.width-1,r.y,r.x+r.width-1,r.y+r.height-1,hintcol);
		GDrawDrawLine(pixmap,r.x+r.width-2,r.y,r.x+r.width-2,r.y+r.height-1,hintcol);
		GDrawDrawLine(pixmap,r.x+r.width-3,r.y,r.x+r.width-3,r.y+r.height-1,hintcol);
	    }
	    if ( rotated!=NULL ) {
		GDrawPushClip(pixmap,&r,&old2);
		GDrawDrawImage(pixmap,rotated,NULL,j*fv->cbw+2,i*fv->cbh+2);
		GDrawPopClip(pixmap,&old2);
		GImageDestroy(rotated);
		rotated = NULL;
	    } else if ( use_utf8 ) {
		GTextBounds size;
		if ( styles!=laststyles ) GDrawSetFont(pixmap,FVCheckFont(fv,styles));
		width = GDrawGetText8Bounds(pixmap,utf8_buf,-1,mods,&size);
		if ( size.lbearing==0 && size.rbearing==0 ) {
		    utf8_buf[0] = 0xe0 | (0xfffd>>12);
		    utf8_buf[1] = 0x80 | ((0xfffd>>6)&0x3f);
		    utf8_buf[2] = 0x80 | (0xfffd&0x3f);
		    utf8_buf[3] = 0;
		    width = GDrawGetText8Bounds(pixmap,utf8_buf,-1,mods,&size);
		}
		width = size.rbearing - size.lbearing+1;
		if ( width >= fv->cbw-1 ) {
		    GDrawPushClip(pixmap,&r,&old2);
		    width = fv->cbw-1;
		}
		if ( sc->unicodeenc<0x80 || sc->unicodeenc>=0xa0 )
		    GDrawDrawText8(pixmap,j*fv->cbw+(fv->cbw-1-width)/2-size.lbearing,i*fv->cbh+fv->lab_as+1,utf8_buf,-1,mods,fg);
		if ( width >= fv->cbw-1 )
		    GDrawPopClip(pixmap,&old2);
		laststyles = styles;
	    } else {
		if ( styles!=laststyles ) GDrawSetFont(pixmap,FVCheckFont(fv,styles));
		width = GDrawGetTextWidth(pixmap,buf,-1,mods);
		if ( width >= fv->cbw-1 ) {
		    GDrawPushClip(pixmap,&r,&old2);
		    width = fv->cbw-1;
		}
		if ( sc->unicodeenc<0x80 || sc->unicodeenc>=0xa0 )
		    GDrawDrawText(pixmap,j*fv->cbw+(fv->cbw-1-width)/2,i*fv->cbh+fv->lab_as+1,buf,-1,mods,fg);
		if ( width >= fv->cbw-1 )
		    GDrawPopClip(pixmap,&old2);
		laststyles = styles;
	    }
	    changed = sc->changed;
	    if ( fv->sf->onlybitmaps )
		changed = gid==-1 || fv->show->glyphs[gid]==NULL? false : fv->show->glyphs[gid]->changed;
	    if ( changed ) {
		GRect r;
		r.x = j*fv->cbw+1; r.width = fv->cbw-1;
		r.y = i*fv->cbh+1; r.height = fv->lab_height-1;
		if ( bg == COLOR_DEFAULT )
		    bg = GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(fv->v));
		GDrawSetXORBase(pixmap,bg);
		GDrawSetXORMode(pixmap);
		GDrawFillRect(pixmap,&r,0x000000);
		GDrawSetCopyMode(pixmap);
	    }
	}

	feat_gid = FeatureTrans(fv,index);
	sc = feat_gid!=-1 ? fv->sf->glyphs[feat_gid]: NULL;
	if ( !SCWorthOutputting(sc) ) {
	    int x = j*fv->cbw+1, xend = x+fv->cbw-2;
	    int y = i*fv->cbh+fv->lab_height+1, yend = y+fv->cbw-1;
	    GDrawDrawLine(pixmap,x,y,xend,yend,0xd08080);
	    GDrawDrawLine(pixmap,x,yend,xend,y,0xd08080);
	}
	if ( sc!=NULL ) {
	    BDFChar *bdfc;

	    if ( fv->show!=NULL && fv->show->piecemeal &&
		    feat_gid!=-1 && fv->show->glyphs[feat_gid]==NULL &&
		    fv->sf->glyphs[feat_gid]!=NULL )
		BDFPieceMeal(fv->show,feat_gid);

	    if ( fv->show!=NULL && feat_gid!=-1 &&
		    feat_gid < fv->show->glyphcnt &&
		    fv->show->glyphs[feat_gid]==NULL &&
		    SCWorthOutputting(fv->sf->glyphs[feat_gid]) ) {
		/* If we have an outline but no bitmap for this slot */
		box.x = j*fv->cbw+1; box.width = fv->cbw-2;
		box.y = i*fv->cbh+fv->lab_height+2; box.height = box.width+1;
		GDrawDrawRect(pixmap,&box,0xff0000);
		++box.x; ++box.y; box.width -= 2; box.height -= 2;
		GDrawDrawRect(pixmap,&box,0xff0000);
/* When reencoding a font we can find times where index>=show->charcnt */
	    } else if ( fv->show!=NULL && feat_gid<fv->show->glyphcnt && feat_gid!=-1 &&
		    fv->show->glyphs[feat_gid]!=NULL ) {
		bdfc = fv->show->glyphs[feat_gid];
		base.data = bdfc->bitmap;
		base.bytes_per_line = bdfc->bytes_per_line;
		base.width = bdfc->xmax-bdfc->xmin+1;
		base.height = bdfc->ymax-bdfc->ymin+1;
		box.x = j*fv->cbw; box.width = fv->cbw;
		box.y = i*fv->cbh+fv->lab_height+1; box.height = box.width+1;
		GDrawPushClip(pixmap,&box,&old2);
		if ( !fv->sf->onlybitmaps && fv->show!=fv->filled &&
			sc->layers[ly_fore].splines==NULL && sc->layers[ly_fore].refs==NULL &&
			!sc->widthset &&
			!(bdfc->xmax<=0 && bdfc->xmin==0 && bdfc->ymax<=0 && bdfc->ymax==0) ) {
		    /* If we have a bitmap but no outline character... */
		    GRect b;
		    b.x = box.x+1; b.y = box.y+1; b.width = box.width-2; b.height = box.height-2;
		    GDrawDrawRect(pixmap,&b,0x008000);
		    ++b.x; ++b.y; b.width -= 2; b.height -= 2;
		    GDrawDrawRect(pixmap,&b,0x008000);
		}
		/* I assume that the bitmap image matches the bounding*/
		/*  box. In some bitmap fonts the bitmap has white space on the*/
		/*  right. This can throw off the centering algorithem */
		if ( fv->magnify>1 ) {
		    GDrawDrawImageMagnified(pixmap,&gi,NULL,
			    j*fv->cbw+(fv->cbw-1-fv->magnify*base.width)/2,
			    i*fv->cbh+fv->lab_height+1+fv->magnify*(fv->show->ascent-bdfc->ymax),
			    fv->magnify*base.width,fv->magnify*base.height);
		} else
		    GDrawDrawImage(pixmap,&gi,NULL,
			    j*fv->cbw+(fv->cbw-1-base.width)/2,
			    i*fv->cbh+fv->lab_height+1+fv->show->ascent-bdfc->ymax);
		if ( fv->showhmetrics ) {
		    int x1, x0 = j*fv->cbw+(fv->cbw-1-fv->magnify*base.width)/2- bdfc->xmin*fv->magnify;
		    /* Draw advance width & horizontal origin */
		    if ( fv->showhmetrics&fvm_origin )
			GDrawDrawLine(pixmap,x0,i*fv->cbh+fv->lab_height+yorg-3,x0,
				i*fv->cbh+fv->lab_height+yorg+2,METRICS_ORIGIN);
		    x1 = x0 + bdfc->width;
		    if ( fv->showhmetrics&fvm_advanceat )
			GDrawDrawLine(pixmap,x1,i*fv->cbh+fv->lab_height+1,x1,
				(i+1)*fv->cbh-1,METRICS_ADVANCE);
		    if ( fv->showhmetrics&fvm_advanceto )
			GDrawDrawLine(pixmap,x0,(i+1)*fv->cbh-2,x1,
				(i+1)*fv->cbh-2,METRICS_ADVANCE);
		}
		if ( fv->showvmetrics ) {
		    int x0 = j*fv->cbw+(fv->cbw-1-fv->magnify*base.width)/2- bdfc->xmin*fv->magnify
			    + fv->magnify*fv->show->pixelsize/2;
		    int y0 = i*fv->cbh+fv->lab_height+yorg;
		    int yvw = y0 + fv->magnify*sc->vwidth*fv->show->pixelsize/em;
		    if ( fv->showvmetrics&fvm_baseline )
			GDrawDrawLine(pixmap,x0,i*fv->cbh+fv->lab_height+1,x0,
				(i+1)*fv->cbh-1,METRICS_BASELINE);
		    if ( fv->showvmetrics&fvm_advanceat )
			GDrawDrawLine(pixmap,j*fv->cbw,yvw,(j+1)*fv->cbw,
				yvw,METRICS_ADVANCE);
		    if ( fv->showvmetrics&fvm_advanceto )
			GDrawDrawLine(pixmap,j*fv->cbw+2,y0,j*fv->cbw+2,
				yvw,METRICS_ADVANCE);
		    if ( fv->showvmetrics&fvm_origin )
			GDrawDrawLine(pixmap,x0-3,i*fv->cbh+fv->lab_height+yorg,x0+2,i*fv->cbh+fv->lab_height+yorg,METRICS_ORIGIN);
		}
		GDrawPopClip(pixmap,&old2);
	    }
	}
	if ( index<fv->map->enccount && fv->selected[index] ) {
	    box.x = j*fv->cbw+1; box.width = fv->cbw-1;
	    box.y = i*fv->cbh+fv->lab_height+1; box.height = fv->cbw;
	    GDrawSetXORMode(pixmap);
	    GDrawSetXORBase(pixmap,GDrawGetDefaultBackground(NULL));
	    GDrawFillRect(pixmap,&box,XOR_COLOR);
	    GDrawSetCopyMode(pixmap);
	}
    }
    if ( fv->showhmetrics&fvm_baseline ) {
	for ( i=0; i<=fv->rowcnt; ++i )
	    GDrawDrawLine(pixmap,0,i*fv->cbh+fv->lab_height+fv->show->ascent+1,fv->width,i*fv->cbh+fv->lab_height+fv->show->ascent+1,METRICS_BASELINE);
    }
    GDrawPopClip(pixmap,&old);
    GDrawSetDither(NULL, true);
}

static char *chosung[] = { "G", "GG", "N", "D", "DD", "L", "M", "B", "BB", "S", "SS", "", "J", "JJ", "C", "K", "T", "P", "H", NULL };
static char *jungsung[] = { "A", "AE", "YA", "YAE", "EO", "E", "YEO", "YE", "O", "WA", "WAE", "OE", "YO", "U", "WEO", "WE", "WI", "YU", "EU", "YI", "I", NULL };
static char *jongsung[] = { "", "G", "GG", "GS", "N", "NJ", "NH", "D", "L", "LG", "LM", "LB", "LS", "LT", "LP", "LH", "M", "B", "BS", "S", "SS", "NG", "J", "C", "K", "T", "P", "H", NULL };

static void FVDrawInfo(FontView *fv,GWindow pixmap,GEvent *event) {
    GRect old, r;
    char buffer[250], *pt;
    unichar_t ubuffer[250];
    Color bg = GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(pixmap));
    SplineChar *sc, dummy;
    SplineFont *sf = fv->sf;
    EncMap *map = fv->map;
    int gid;
    int uni;
    Color fg = 0xff0000;
    int ulen, tlen;

    if ( event->u.expose.rect.y+event->u.expose.rect.height<=fv->mbh )
return;

    GDrawSetFont(pixmap,fv->fontset[0]);
    GDrawPushClip(pixmap,&event->u.expose.rect,&old);

    r.x = 0; r.width = fv->width; r.y = fv->mbh; r.height = fv->infoh;
    GDrawFillRect(pixmap,&r,bg);
    if ( fv->end_pos>=map->enccount || fv->pressed_pos>=map->enccount ||
	    fv->end_pos<0 || fv->pressed_pos<0 )
	fv->end_pos = fv->pressed_pos = -1;	/* Can happen after reencoding */
    if ( fv->end_pos == -1 )
return;

    if ( map->remap!=NULL ) {
	int localenc = fv->end_pos;
	struct remap *remap = map->remap;
	while ( remap->infont!=-1 ) {
	    if ( localenc>=remap->infont && localenc<=remap->infont+(remap->lastenc-remap->firstenc) ) {
		localenc += remap->firstenc-remap->infont;
	break;
	    }
	    ++remap;
	}
	sprintf( buffer, "%-5d (0x%04x) ", localenc, localenc );
    } else if ( map->enc->only_1byte ||
	    (map->enc->has_1byte && fv->end_pos<256))
	sprintf( buffer, "%-3d (0x%02x) ", fv->end_pos, fv->end_pos );
    else
	sprintf( buffer, "%-5d (0x%04x) ", fv->end_pos, fv->end_pos );
    sc = (gid=fv->map->map[fv->end_pos])!=-1 ? sf->glyphs[gid] : NULL;
    SCBuildDummy(&dummy,sf,fv->map,fv->end_pos);
    if ( sc==NULL ) sc = &dummy;
    uni = dummy.unicodeenc!=-1 ? dummy.unicodeenc : sc->unicodeenc;
    if ( uni!=-1 )
	sprintf( buffer+strlen(buffer), "U+%04X", uni );
    else
	sprintf( buffer+strlen(buffer), "U+????" );
    sprintf( buffer+strlen(buffer), "  %.*s", (int) (sizeof(buffer)-strlen(buffer)-1),
	    sc->name );

    strcat(buffer,"  ");
    utf82u_strcpy(ubuffer,buffer);
    ulen = u_strlen(ubuffer);

    if ( uni==-1 && (pt=strchr(sc->name,'.'))!=NULL && pt-sc->name<30 ) {
	strncpy(buffer,sc->name,pt-sc->name);
	buffer[(pt-sc->name)] = '\0';
	uni = UniFromName(buffer,fv->sf->uni_interp,map->enc);
	if ( uni!=-1 ) {
	    sprintf( buffer, "U+%04X ", uni );
	    uc_strcat(ubuffer,buffer);
	}
	fg = 0x707070;
    }
    if ( uni!=-1 && uni<0x110000 && _UnicodeNameAnnot!=NULL &&
	    _UnicodeNameAnnot[uni>>16][(uni>>8)&0xff][uni&0xff].name!=NULL ) {
	uc_strncat(ubuffer, _UnicodeNameAnnot[uni>>16][(uni>>8)&0xff][uni&0xff].name, 80);
    } else if ( uni>=0xAC00 && uni<=0xD7A3 ) {
	sprintf( buffer, "Hangul Syllable %s%s%s",
		chosung[(uni-0xAC00)/(21*28)],
		jungsung[(uni-0xAC00)/28%21],
		jongsung[(uni-0xAC00)%28] );
	uc_strncat(ubuffer,buffer,80);
    } else if ( uni!=-1 ) {
	uc_strncat(ubuffer, UnicodeRange(uni),80);
    }

    tlen = GDrawDrawText(pixmap,10,fv->mbh+fv->lab_as,ubuffer,ulen,NULL,0xff0000);
    GDrawDrawText(pixmap,10+tlen,fv->mbh+fv->lab_as,ubuffer+ulen,-1,NULL,fg);
    GDrawPopClip(pixmap,&old);
}

static void FVShowInfo(FontView *fv) {
    GRect r;

    if ( fv->v==NULL )			/* Can happen in scripts */
return;

    r.x = 0; r.width = fv->width; r.y = fv->mbh; r.height = fv->infoh;
    GDrawRequestExpose(fv->gw,&r,false);
}

static void FVChar(FontView *fv,GEvent *event) {
    int i,pos, cnt, gid;

#if MyMemory
    if ( event->u.chr.keysym == GK_F2 ) {
	fprintf( stderr, "Malloc debug on\n" );
	__malloc_debug(5);
    } else if ( event->u.chr.keysym == GK_F3 ) {
	fprintf( stderr, "Malloc debug off\n" );
	__malloc_debug(0);
    }
#endif

    if ( event->u.chr.keysym=='s' &&
	    (event->u.chr.state&ksm_control) &&
	    (event->u.chr.state&ksm_meta) )
	MenuSaveAll(NULL,NULL,NULL);
    else if ( event->u.chr.keysym=='q' &&
	    (event->u.chr.state&ksm_control) &&
	    (event->u.chr.state&ksm_meta) )
	MenuExit(NULL,NULL,NULL);
    else if ( event->u.chr.keysym=='I' &&
	    (event->u.chr.state&ksm_shift) &&
	    (event->u.chr.state&ksm_meta) )
	FVMenuCharInfo(fv->gw,NULL,NULL);
    else if ( (event->u.chr.keysym=='[' || event->u.chr.keysym==']') &&
	    (event->u.chr.state&ksm_control) ) {
	_FVMenuChangeChar(fv,event->u.chr.keysym=='['?MID_Prev:MID_Next);
    } else if ( (event->u.chr.keysym=='{' || event->u.chr.keysym=='}') &&
	    (event->u.chr.state&ksm_control) ) {
	_FVMenuChangeChar(fv,event->u.chr.keysym=='{'?MID_PrevDef:MID_NextDef);
    } else if ( event->u.chr.keysym=='\\' && (event->u.chr.state&ksm_control) ) {
	/* European keyboards need a funky modifier to get \ */
	FVDoTransform(fv);
#if !defined(_NO_FFSCRIPT) || !defined(_NO_PYTHON)
    } else if ( isdigit(event->u.chr.keysym) && (event->u.chr.state&ksm_control) &&
	    (event->u.chr.state&ksm_meta) ) {
	/* The Script menu isn't always up to date, so we might get one of */
	/*  the shortcuts here */
	int index = event->u.chr.keysym-'1';
	if ( index<0 ) index = 9;
	if ( script_filenames[index]!=NULL )
	    ExecuteScriptFile(fv,NULL,script_filenames[index]);
#endif
    } else if ( event->u.chr.keysym == GK_Left ||
	    event->u.chr.keysym == GK_Tab ||
	    event->u.chr.keysym == GK_BackTab ||
	    event->u.chr.keysym == GK_Up ||
	    event->u.chr.keysym == GK_Right ||
	    event->u.chr.keysym == GK_Down ||
	    event->u.chr.keysym == GK_KP_Left ||
	    event->u.chr.keysym == GK_KP_Up ||
	    event->u.chr.keysym == GK_KP_Right ||
	    event->u.chr.keysym == GK_KP_Down ||
	    event->u.chr.keysym == GK_Home ||
	    event->u.chr.keysym == GK_KP_Home ||
	    event->u.chr.keysym == GK_End ||
	    event->u.chr.keysym == GK_KP_End ||
	    event->u.chr.keysym == GK_Page_Up ||
	    event->u.chr.keysym == GK_KP_Page_Up ||
	    event->u.chr.keysym == GK_Prior ||
	    event->u.chr.keysym == GK_Page_Down ||
	    event->u.chr.keysym == GK_KP_Page_Down ||
	    event->u.chr.keysym == GK_Next ) {
	int end_pos = fv->end_pos;
	/* We move the currently selected char. If there is none, then pick */
	/*  something on the screen */
	if ( end_pos==-1 )
	    end_pos = (fv->rowoff+fv->rowcnt/2)*fv->colcnt;
	switch ( event->u.chr.keysym ) {
	  case GK_Tab:
	    pos = end_pos;
	    do {
		if ( event->u.chr.state&ksm_shift )
		    --pos;
		else
		    ++pos;
		if ( pos>=fv->map->enccount ) pos = 0;
		else if ( pos<0 ) pos = fv->map->enccount-1;
	    } while ( pos!=end_pos &&
		    ((gid=fv->map->map[pos])==-1 || !SCWorthOutputting(fv->sf->glyphs[gid])));
	    if ( pos==end_pos ) ++pos;
	    if ( pos>=fv->map->enccount ) pos = 0;
	  break;
#if GK_Tab!=GK_BackTab
	  case GK_BackTab:
	    pos = end_pos;
	    do {
		--pos;
		if ( pos<0 ) pos = fv->map->enccount-1;
	    } while ( pos!=end_pos &&
		    ((gid=fv->map->map[pos])==-1 || !SCWorthOutputting(fv->sf->glyphs[gid])));
	    if ( pos==end_pos ) --pos;
	    if ( pos<0 ) pos = 0;
	  break;
#endif
	  case GK_Left: case GK_KP_Left:
	    pos = end_pos-1;
	  break;
	  case GK_Right: case GK_KP_Right:
	    pos = end_pos+1;
	  break;
	  case GK_Up: case GK_KP_Up:
	    pos = end_pos-fv->colcnt;
	  break;
	  case GK_Down: case GK_KP_Down:
	    pos = end_pos+fv->colcnt;
	  break;
	  case GK_End: case GK_KP_End:
	    pos = fv->map->enccount;
	  break;
	  case GK_Home: case GK_KP_Home:
	    pos = 0;
	    if ( fv->sf->top_enc!=-1 && fv->sf->top_enc<fv->map->enccount )
		pos = fv->sf->top_enc;
	    else {
		pos = SFFindSlot(fv->sf,fv->map,'A',NULL);
		if ( pos==-1 ) pos = 0;
	    }
	  break;
	  case GK_Page_Up: case GK_KP_Page_Up:
#if GK_Prior!=GK_Page_Up
	  case GK_Prior:
#endif
	    pos = (fv->rowoff-fv->rowcnt+1)*fv->colcnt;
	  break;
	  case GK_Page_Down: case GK_KP_Page_Down:
#if GK_Next!=GK_Page_Down
	  case GK_Next:
#endif
	    pos = (fv->rowoff+fv->rowcnt+1)*fv->colcnt;
	  break;
	}
	if ( pos<0 ) pos = 0;
	if ( pos>=fv->map->enccount ) pos = fv->map->enccount-1;
	if ( event->u.chr.state&ksm_shift && event->u.chr.keysym!=GK_Tab && event->u.chr.keysym!=GK_BackTab ) {
	    FVReselect(fv,pos);
	} else {
	    FVDeselectAll(fv);
	    fv->selected[pos] = true;
	    FVToggleCharSelected(fv,pos);
	    fv->pressed_pos = pos;
	    fv->sel_index = 1;
	}
	fv->end_pos = pos;
	FVShowInfo(fv);
	FVScrollToChar(fv,pos);
    } else if ( event->u.chr.keysym == GK_Help ) {
	MenuHelp(NULL,NULL,NULL);	/* Menu does F1 */
    } else if ( event->u.chr.keysym == GK_Escape ) {
	FVDeselectAll(fv);
    } else if ( event->u.chr.chars[0]=='\r' || event->u.chr.chars[0]=='\n' ) {
	for ( i=cnt=0; i<fv->map->enccount && cnt<10; ++i ) if ( fv->selected[i] ) {
	    SplineChar *sc = SFMakeChar(fv->sf,fv->map,i);
	    if ( fv->show==fv->filled ) {
		CharViewCreate(sc,fv,i);
	    } else {
		BDFFont *bdf = fv->show;
		BitmapViewCreate(BDFMakeGID(bdf,sc->orig_pos),bdf,fv,i);
	    }
	    ++cnt;
	}
    } else if ( event->u.chr.chars[0]<=' ' || event->u.chr.chars[1]!='\0' ) {
	/* Do Nothing */;
    } else {
	SplineFont *sf = fv->sf;
	for ( i=0; i<sf->glyphcnt; ++i ) {
	    if ( sf->glyphs[i]!=NULL )
		if ( sf->glyphs[i]->unicodeenc==event->u.chr.chars[0] )
	break;
	}
	if ( i==sf->glyphcnt ) {
	    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->map->map[i]==-1 ) {
		SplineChar dummy;
		SCBuildDummy(&dummy,sf,fv->map,i);
		if ( dummy.unicodeenc==event->u.chr.chars[0] )
	    break;
	    }
	} else
	    i = fv->map->backmap[i];
	if ( i!=fv->map->enccount && i!=-1 )
	    FVChangeChar(fv,i);
    }
}

static void uc_annot_strncat(unichar_t *to, const char *from, int len) {
    register unichar_t ch;

    to += u_strlen(to);
    while ( (ch = *(unsigned char *) from++) != '\0' && --len>=0 ) {
	if ( from[-2]=='\t' ) {
	    if ( ch=='*' ) ch = 0x2022;
	    else if ( ch=='x' ) ch = 0x2192;
	    else if ( ch==':' ) ch = 0x224d;
	    else if ( ch=='#' ) ch = 0x2245;
	} else if ( ch=='\t' ) {
	    *(to++) = ' ';
	    ch = ' ';
	}
	*(to++) = ch;
    }
    *to = 0;
}

void SCPreparePopup(GWindow gw,SplineChar *sc,struct remap *remap, int localenc,
	int actualuni) {
    static unichar_t space[810];
    char cspace[162];
    int upos=-1;
    int done = false;

    /* If a glyph is multiply mapped then the inbuild unicode enc may not be */
    /*  the actual one used to access the glyph */
    if ( remap!=NULL ) {
	while ( remap->infont!=-1 ) {
	    if ( localenc>=remap->infont && localenc<=remap->infont+(remap->lastenc-remap->firstenc) ) {
		localenc += remap->firstenc-remap->infont;
	break;
	    }
	    ++remap;
	}
    }

    if ( actualuni!=-1 )
	upos = actualuni;
    else if ( sc->unicodeenc!=-1 )
	upos = sc->unicodeenc;
#if HANYANG
    else if ( sc->compositionunit ) {
	if ( sc->jamo<19 )
	    upos = 0x1100+sc->jamo;
	else if ( sc->jamo<19+21 )
	    upos = 0x1161 + sc->jamo-19;
	else		/* Leave a hole for the blank char */
	    upos = 0x11a8 + sc->jamo-(19+21+1);
    }
#endif
    else {
#if defined( _NO_SNPRINTF ) || defined( __VMS )
	sprintf( cspace, "%u 0x%x U+???? \"%.25s\" ", localenc, localenc, sc->name==NULL?"":sc->name );
#else
	snprintf( cspace, sizeof(cspace), "%u 0x%x U+???? \"%.25s\" ", localenc, localenc, sc->name==NULL?"":sc->name );
#endif
	uc_strcpy(space,cspace);
	done = true;
    }
    if ( done )
	/* Do Nothing */;
    else if ( upos<0x110000 && _UnicodeNameAnnot!=NULL &&
	    _UnicodeNameAnnot[upos>>16][(upos>>8)&0xff][upos&0xff].name!=NULL ) {
#if defined( _NO_SNPRINTF ) || defined( __VMS )
	sprintf( cspace, "%u 0x%x U+%04x \"%.25s\" %.100s", localenc, localenc, upos, sc->name==NULL?"":sc->name,
		_UnicodeNameAnnot[upos>>16][(upos>>8)&0xff][upos&0xff].name);
#else
	snprintf( cspace, sizeof(cspace), "%u 0x%x U+%04x \"%.25s\" %.100s", localenc, localenc, upos, sc->name==NULL?"":sc->name,
		_UnicodeNameAnnot[upos>>16][(upos>>8)&0xff][upos&0xff].name);
#endif
	utf82u_strcpy(space,cspace);
    } else if ( upos>=0xAC00 && upos<=0xD7A3 ) {
#if defined( _NO_SNPRINTF ) || defined( __VMS )
	sprintf( cspace, "%u 0x%x U+%04x \"%.25s\" Hangul Syllable %s%s%s",
		localenc, localenc, upos, sc->name==NULL?"":sc->name,
		chosung[(upos-0xAC00)/(21*28)],
		jungsung[(upos-0xAC00)/28%21],
		jongsung[(upos-0xAC00)%28] );
#else
	snprintf( cspace, sizeof(cspace), "%u 0x%x U+%04x \"%.25s\" Hangul Syllable %s%s%s",
		localenc, localenc, upos, sc->name==NULL?"":sc->name,
		chosung[(upos-0xAC00)/(21*28)],
		jungsung[(upos-0xAC00)/28%21],
		jongsung[(upos-0xAC00)%28] );
#endif
	utf82u_strcpy(space,cspace);
    } else {
#if defined( _NO_SNPRINTF ) || defined( __VMS )
	sprintf( cspace, "%u 0x%x U+%04x \"%.25s\" %.50s", localenc, localenc, upos, sc->name==NULL?"":sc->name,
	    	UnicodeRange(upos));
#else
	snprintf( cspace, sizeof(cspace), "%u 0x%x U+%04x \"%.25s\" %.50s", localenc, localenc, upos, sc->name==NULL?"":sc->name,
	    	UnicodeRange(upos));
#endif
	utf82u_strcpy(space,cspace);
    }
    if ( upos>=0 && upos<0x110000 && _UnicodeNameAnnot!=NULL &&
	    _UnicodeNameAnnot[upos>>16][(upos>>8)&0xff][upos&0xff].annot!=NULL ) {
	int left = sizeof(space)/sizeof(space[0]) - u_strlen(space)-1;
	if ( left>4 ) {
	    uc_strcat(space,"\n");
	    uc_annot_strncat(space,_UnicodeNameAnnot[upos>>16][(upos>>8)&0xff][upos&0xff].annot,left-2);
	}
    }
    if ( sc->comment!=NULL ) {
	int left = sizeof(space)/sizeof(space[0]) - u_strlen(space)-1;
	if ( left>4 ) {
	    uc_strcat(space,"\n\n");
	    utf82u_strncpy(space+u_strlen(space),sc->comment,left-2);
	}
    }
    GGadgetPreparePopup(gw,space);
}

static void noop(void *_fv) {
}

static void *ddgencharlist(void *_fv,int32 *len) {
    int i,j,cnt, gid;
    FontView *fv = (FontView *) _fv;
    SplineFont *sf = fv->sf;
    EncMap *map = fv->map;
    char *data;

    for ( i=cnt=0; i<map->enccount; ++i ) if ( fv->selected[i] && (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL )
	cnt += strlen(sf->glyphs[gid]->name)+1;
    data = galloc(cnt+1); data[0] = '\0';
    for ( cnt=0, j=1 ; j<=fv->sel_index; ++j ) {
	for ( i=cnt=0; i<map->enccount; ++i )
	    if ( fv->selected[i] && (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL ) {
		strcpy(data+cnt,sf->glyphs[gid]->name);
		cnt += strlen(sf->glyphs[gid]->name);
		strcpy(data+cnt++," ");
	    }
    }
    if ( cnt>0 )
	data[--cnt] = '\0';
    *len = cnt;
return( data );
}

static void FVMouse(FontView *fv,GEvent *event) {
    int pos = (event->u.mouse.y/fv->cbh + fv->rowoff)*fv->colcnt + event->u.mouse.x/fv->cbw;
    int gid;
    int realpos = pos;
    SplineChar *sc, dummy;
    int dopopup = true;
    extern int OpenCharsInNewWindow;

    if ( event->type==et_mousedown )
	CVPaletteDeactivate();
    if ( pos<0 ) {
	pos = 0;
	dopopup = false;
    } else if ( pos>=fv->map->enccount ) {
	pos = fv->map->enccount-1;
	if ( pos<0 )		/* No glyph slots in font */
return;
	dopopup = false;
    }

    sc = (gid=fv->map->map[pos])!=-1 ? fv->sf->glyphs[gid] : NULL;
    if ( sc==NULL )
	sc = SCBuildDummy(&dummy,fv->sf,fv->map,pos);
    if ( event->type == et_mouseup && event->u.mouse.clicks==2 ) {
	if ( fv->cur_subtable!=NULL ) {
	    sc = FVMakeChar(fv,pos);
	    pos = fv->map->backmap[sc->orig_pos];
	}
	if ( sc==&dummy ) {
	    sc = SFMakeChar(fv->sf,fv->map,pos);
	    gid = fv->map->map[pos];
	}
	if ( fv->show==fv->filled ) {
	    SplineFont *sf = fv->sf;
	    gid = -1;
	    if ( !OpenCharsInNewWindow )
		for ( gid=sf->glyphcnt-1; gid>=0; --gid )
		    if ( sf->glyphs[gid]!=NULL && sf->glyphs[gid]->views!=NULL )
		break;
	    if ( gid!=-1 ) {
		CharView *cv = sf->glyphs[gid]->views;
		CVChangeSC(cv,sc);
		GDrawSetVisible(cv->gw,true);
		GDrawRaise(cv->gw);
	    } else
		CharViewCreate(sc,fv,pos);
	} else {
	    BDFFont *bdf = fv->show;
	    BDFChar *bc =BDFMakeGID(bdf,gid);
	    gid = -1;
	    if ( !OpenCharsInNewWindow )
		for ( gid=bdf->glyphcnt-1; gid>=0; --gid )
		    if ( bdf->glyphs[gid]!=NULL && bdf->glyphs[gid]->views!=NULL )
		break;
	    if ( gid!=-1 ) {
		BitmapView *bv = bdf->glyphs[gid]->views;
		BVChangeBC(bv,bc,true);
		GDrawSetVisible(bv->gw,true);
		GDrawRaise(bv->gw);
	    } else
		BitmapViewCreate(bc,bdf,fv,pos);
	}
    } else if ( event->type == et_mousemove ) {
	if ( dopopup )
	    SCPreparePopup(fv->v,sc,fv->map->remap,pos,sc==&dummy?dummy.unicodeenc: UniFromEnc(pos,fv->map->enc));
    }
    if ( event->type == et_mousedown ) {
	if ( fv->drag_and_drop ) {
	    GDrawSetCursor(fv->v,ct_mypointer);
	    fv->any_dd_events_sent = fv->drag_and_drop = false;
	}
	if ( !(event->u.mouse.state&ksm_shift) && event->u.mouse.clicks<=1 ) {
	    if ( !fv->selected[pos] )
		FVDeselectAll(fv);
	    else if ( event->u.mouse.button!=3 ) {
		fv->drag_and_drop = fv->has_dd_no_cursor = true;
		fv->any_dd_events_sent = false;
		GDrawSetCursor(fv->v,ct_prohibition);
		GDrawGrabSelection(fv->v,sn_drag_and_drop);
		GDrawAddSelectionType(fv->v,sn_drag_and_drop,"STRING",fv,0,sizeof(char),
			ddgencharlist,noop);
	    }
	}
	fv->pressed_pos = fv->end_pos = pos;
	FVShowInfo(fv);
	if ( !fv->drag_and_drop ) {
	    if ( !(event->u.mouse.state&ksm_shift))
		fv->sel_index = 1;
	    else if ( fv->sel_index<255 )
		++fv->sel_index;
	    if ( fv->pressed!=NULL )
		GDrawCancelTimer(fv->pressed);
	    else if ( event->u.mouse.state&ksm_shift ) {
		fv->selected[pos] = fv->selected[pos] ? 0 : fv->sel_index;
		FVToggleCharSelected(fv,pos);
	    } else if ( !fv->selected[pos] ) {
		fv->selected[pos] = fv->sel_index;
		FVToggleCharSelected(fv,pos);
	    }
	    if ( event->u.mouse.button==3 )
		GMenuCreatePopupMenu(fv->v,event, fvpopupmenu);
	    else
		fv->pressed = GDrawRequestTimer(fv->v,200,100,NULL);
	}
    } else if ( fv->drag_and_drop ) {
	if ( event->u.mouse.x>=0 && event->u.mouse.y>=-fv->mbh-fv->infoh-4 &&
		event->u.mouse.x<=fv->width+20 && event->u.mouse.y<fv->height ) {
	    if ( !fv->has_dd_no_cursor ) {
		fv->has_dd_no_cursor = true;
		GDrawSetCursor(fv->v,ct_prohibition);
	    }
	} else {
	    if ( fv->has_dd_no_cursor ) {
		fv->has_dd_no_cursor = false;
		GDrawSetCursor(fv->v,ct_ddcursor);
	    }
	    GDrawPostDragEvent(fv->v,event,event->type==et_mouseup?et_drop:et_drag);
	    fv->any_dd_events_sent = true;
	}
	if ( event->type==et_mouseup ) {
	    fv->drag_and_drop = fv->has_dd_no_cursor = false;
	    GDrawSetCursor(fv->v,ct_mypointer);
	    if ( !fv->any_dd_events_sent )
		FVDeselectAll(fv);
	    fv->any_dd_events_sent = false;
	}
    } else if ( fv->pressed!=NULL ) {
	int showit = realpos!=fv->end_pos;
	FVReselect(fv,realpos);
	if ( showit )
	    FVShowInfo(fv);
	if ( event->type==et_mouseup ) {
	    GDrawCancelTimer(fv->pressed);
	    fv->pressed = NULL;
	}
    }
    if ( event->type==et_mouseup && dopopup )
	SCPreparePopup(fv->v,sc,fv->map->remap,pos,sc==&dummy?dummy.unicodeenc: UniFromEnc(pos,fv->map->enc));
    if ( event->type==et_mouseup )
	SVAttachFV(fv,2);
}

#if defined(FONTFORGE_CONFIG_GTK)
gboolean FontView_ViewportResize(GtkWidget *widget, GdkEventConfigure *event,
	gpointer user_data) {
    extern int default_fv_row_count, default_fv_col_count;
    int cc, rc;
    FontView *fv = (FontView *) g_object_get_data( widget, "data" );

    cc = (event->width-1)/fv->cbw; rc = (event->height-1)/fv->cbh;
    if ( cc==0 ) cc = 1; if ( rc==0 ) rc = 1;

    if ( event->width==fv->width || event->height==fv->height )
	/* Ok, the window manager didn't change the size. */
    else if ( event->width!=cc*fv->cbw+1 || event->height!=rc*fv->cbh+1 ) {
	gtk_widget_set_size_request( widget, cc*fv->cbw+1, rc*fv->cbh+1);
return TRUE;
    }
    gtk_widget_set_size_request( lookup_widget(widget,"view"), 1, 1 );
    fv->width = width; fv->height = height;

    default_fv_row_count = rc;
    default_fv_col_count = cc;
    SavePrefs();

return TRUE;
}
#elif defined(FONTFORGE_CONFIG_GDRAW)
static void FVResize(FontView *fv,GEvent *event) {
    extern int default_fv_row_count, default_fv_col_count;
    GRect pos,screensize;
    int topchar;

    if ( fv->colcnt!=0 )
	topchar = fv->rowoff*fv->colcnt;
    else if ( fv->sf->top_enc!=-1 && fv->sf->top_enc<fv->map->enccount )
	topchar = fv->sf->top_enc;
    else {
	/* Position on 'A' if it exists */
	topchar = SFFindSlot(fv->sf,fv->map,'A',NULL);
	if ( topchar==-1 ) {
	    for ( topchar=0; topchar<fv->map->enccount; ++topchar )
		if ( fv->map->map[topchar]!=-1 && fv->sf->glyphs[fv->map->map[topchar]]!=NULL )
	    break;
	    if ( topchar==fv->map->enccount )
		topchar = 0;
	}
    }
    if ( !event->u.resize.sized )
	/* WM isn't responding to my resize requests, so no point in trying */;
    else if ( (event->u.resize.size.width-
		GDrawPointsToPixels(fv->gw,_GScrollBar_Width)-1)%fv->cbw!=0 ||
	    (event->u.resize.size.height-fv->mbh-fv->infoh-1)%fv->cbh!=0 ) {
	int cc = (event->u.resize.size.width+fv->cbw/2-
		GDrawPointsToPixels(fv->gw,_GScrollBar_Width)-1)/fv->cbw;
	int rc = (event->u.resize.size.height-fv->mbh-fv->infoh-1)/fv->cbh;
	if ( cc<=0 ) cc = 1;
	if ( rc<=0 ) rc = 1;
	GDrawGetSize(GDrawGetRoot(NULL),&screensize);
	if ( cc*fv->cbw+GDrawPointsToPixels(fv->gw,_GScrollBar_Width)+10>screensize.width )
	    --cc;
	if ( rc*fv->cbh+fv->mbh+fv->infoh+20>screensize.height )
	    --rc;
	GDrawResize(fv->gw,
		cc*fv->cbw+1+GDrawPointsToPixels(fv->gw,_GScrollBar_Width),
		rc*fv->cbh+1+fv->mbh+fv->infoh);
	/* somehow KDE loses this event of mine so to get even the vague effect */
	/*  we can't just return */
/*return;*/
    }

    pos.width = GDrawPointsToPixels(fv->gw,_GScrollBar_Width);
    pos.height = event->u.resize.size.height-fv->mbh-fv->infoh;
    pos.x = event->u.resize.size.width-pos.width; pos.y = fv->mbh+fv->infoh;
    GGadgetResize(fv->vsb,pos.width,pos.height);
    GGadgetMove(fv->vsb,pos.x,pos.y);
    pos.width = pos.x; pos.x = 0;
    GDrawResize(fv->v,pos.width,pos.height);

    fv->width = pos.width; fv->height = pos.height;
    fv->colcnt = (fv->width-1)/fv->cbw;
    if ( fv->colcnt<1 ) fv->colcnt = 1;
    fv->rowcnt = (fv->height-1)/fv->cbh;
    if ( fv->rowcnt<1 ) fv->rowcnt = 1;
    fv->rowltot = (fv->map->enccount+fv->colcnt-1)/fv->colcnt;

    GScrollBarSetBounds(fv->vsb,0,fv->rowltot,fv->rowcnt);
    fv->rowoff = topchar/fv->colcnt;
    if ( fv->rowoff>=fv->rowltot-fv->rowcnt )
        fv->rowoff = fv->rowltot-fv->rowcnt;
    if ( fv->rowoff<0 ) fv->rowoff =0;
    GScrollBarSetPos(fv->vsb,fv->rowoff);
    GDrawRequestExpose(fv->gw,NULL,true);
    GDrawRequestExpose(fv->v,NULL,true);

    default_fv_row_count = fv->rowcnt;
    default_fv_col_count = fv->colcnt;
    SavePrefs();
}
#endif

static void FVTimer(FontView *fv,GEvent *event) {

    if ( event->u.timer.timer==fv->pressed ) {
	GEvent e;
	GDrawGetPointerPosition(fv->v,&e);
	if ( e.u.mouse.y<0 || e.u.mouse.y >= fv->height ) {
	    real dy = 0;
	    if ( e.u.mouse.y<0 )
		dy = -1;
	    else if ( e.u.mouse.y>=fv->height )
		dy = 1;
	    if ( fv->rowoff+dy<0 )
		dy = 0;
	    else if ( fv->rowoff+dy+fv->rowcnt > fv->rowltot )
		dy = 0;
	    fv->rowoff += dy;
	    if ( dy!=0 ) {
		GScrollBarSetPos(fv->vsb,fv->rowoff);
		GDrawScroll(fv->v,NULL,0,dy*fv->cbh);
	    }
	}
    } else if ( event->u.timer.timer==fv->resize ) {
	/* It's a delayed resize event (for kde which sends continuous resizes) */
	fv->resize = NULL;
	FVResize(fv,(GEvent *) (event->u.timer.userdata));
    } else if ( event->u.timer.userdata!=NULL ) {
	/* It's a delayed function call */
	void (*func)(FontView *) = (void (*)(FontView *)) (event->u.timer.userdata);
	func(fv);
    }
}

void FVDelay(FontView *fv,void (*func)(FontView *)) {
    GDrawRequestTimer(fv->v,100,0,(void *) func);
}

static void FVScroll(FontView *fv,struct sbevent *sb) {
    int newpos = fv->rowoff;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= fv->rowcnt;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += fv->rowcnt;
      break;
      case et_sb_bottom:
        newpos = fv->rowltot-fv->rowcnt;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>fv->rowltot-fv->rowcnt )
        newpos = fv->rowltot-fv->rowcnt;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=fv->rowoff ) {
	int diff = newpos-fv->rowoff;
	fv->rowoff = newpos;
	GScrollBarSetPos(fv->vsb,fv->rowoff);
	GDrawScroll(fv->v,NULL,0,diff*fv->cbh);
    }
}

static int v_e_h(GWindow gw, GEvent *event) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button==4 || event->u.mouse.button==5) ) {
return( GGadgetDispatchEvent(fv->vsb,event));
    }

    GGadgetPopupExternalEvent(event);
    switch ( event->type ) {
      case et_expose:
	GDrawSetLineWidth(gw,0);
	FVExpose(fv,gw,event);
      break;
      case et_char:
	FVChar(fv,event);
      break;
      case et_mousemove: case et_mousedown: case et_mouseup:
	if ( event->type==et_mousedown )
	    GDrawSetGIC(gw,fv->gic,0,20);
	FVMouse(fv,event);
      break;
      case et_timer:
	FVTimer(fv,event);
      break;
      case et_focus:
	if ( event->u.focus.gained_focus ) {
	    GDrawSetGIC(gw,fv->gic,0,20);
#if 0
	    CVPaletteDeactivate();
#endif
	}
      break;
    }
return( true );
}

void FontViewReformatOne(FontView *fv) {
    FontView *fvs;

    if ( fv->v==NULL || fv->colcnt==0 )	/* Can happen in scripts */
return;

    GDrawSetCursor(fv->v,ct_watch);
    fv->rowltot = (fv->map->enccount+fv->colcnt-1)/fv->colcnt;
    GScrollBarSetBounds(fv->vsb,0,fv->rowltot,fv->rowcnt);
    if ( fv->rowoff>fv->rowltot-fv->rowcnt ) {
        fv->rowoff = fv->rowltot-fv->rowcnt;
	if ( fv->rowoff<0 ) fv->rowoff =0;
	GScrollBarSetPos(fv->vsb,fv->rowoff);
    }
    for ( fvs=fv->sf->fv; fvs!=NULL; fvs=fvs->nextsame )
	if ( fvs!=fv && fvs->sf==fv->sf )
    break;
    GDrawRequestExpose(fv->v,NULL,false);
    GDrawSetCursor(fv->v,ct_pointer);
}

void FontViewReformatAll(SplineFont *sf) {
    BDFFont *new, *old;
    FontView *fvs, *fv;
    MetricsView *mvs;

    if ( sf->fv->v==NULL || sf->fv->colcnt==0 )			/* Can happen in scripts */
return;

    for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame )
	fvs->touched = false;
    while ( 1 ) {
	for ( fv=sf->fv; fv!=NULL && fv->touched; fv=fv->nextsame );
	if ( fv==NULL )
    break;
	old = fv->filled;
				/* In CID fonts fv->sf may not be same as sf */
	new = SplineFontPieceMeal(fv->sf,fv->filled->pixelsize,
		(fv->antialias?pf_antialias:0)|(fv->bbsized?pf_bbsized:0)|
		    (use_freetype_to_rasterize_fv && !sf->strokedfont && !sf->multilayer?pf_ft_nohints:0),
		NULL);
	for ( fvs=fv; fvs!=NULL; fvs=fvs->nextsame )
	    if ( fvs->filled == old ) {
		fvs->filled = new;
		if ( fvs->show == old )
		    fvs->show = new;
		fvs->touched = true;
	    }
	BDFFontFree(old);
    }
    for ( fv=sf->fv; fv!=NULL; fv=fv->nextsame ) {
	GDrawSetCursor(fv->v,ct_watch);
	fv->rowltot = (fv->map->enccount+fv->colcnt-1)/fv->colcnt;
	GScrollBarSetBounds(fv->vsb,0,fv->rowltot,fv->rowcnt);
	if ( fv->rowoff>fv->rowltot-fv->rowcnt ) {
	    fv->rowoff = fv->rowltot-fv->rowcnt;
	    if ( fv->rowoff<0 ) fv->rowoff =0;
	    GScrollBarSetPos(fv->vsb,fv->rowoff);
	}
	GDrawRequestExpose(fv->v,NULL,false);
	GDrawSetCursor(fv->v,ct_pointer);
    }
    for ( mvs=sf->metrics; mvs!=NULL; mvs=mvs->next ) if ( mvs->bdf==NULL ) {
	BDFFontFree(mvs->show);
	mvs->show = SplineFontPieceMeal(sf,mvs->pixelsize,mvs->antialias?pf_antialias:0,NULL);
	GDrawRequestExpose(mvs->gw,NULL,false);
    }
}

static int fv_e_h(GWindow gw, GEvent *event) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button==4 || event->u.mouse.button==5) ) {
return( GGadgetDispatchEvent(fv->vsb,event));
    }

    switch ( event->type ) {
      case et_selclear:
	ClipboardClear();
      break;
      case et_expose:
	GDrawSetLineWidth(gw,0);
	FVDrawInfo(fv,gw,event);
      break;
      case et_resize:
	/* KDE sends a continuous stream of resize events, and gets very */
	/*  confused if I start resizing the window myself, try to wait for */
	/*  the user to finish before responding to resizes */
	if ( event->u.resize.sized || fv->resize_expected ) {
	    if ( fv->resize )
		GDrawCancelTimer(fv->resize);
	    fv->resize_event = *event;
	    fv->resize = GDrawRequestTimer(fv->v,300,0,(void *) &fv->resize_event);
	    fv->resize_expected = false;
	}
      break;
      case et_char:
	FVChar(fv,event);
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    FVScroll(fv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_close:
	FVMenuClose(gw,NULL,NULL);
      break;
      case et_create:
	fv->next = fv_list;
	fv_list = fv;
      break;
      case et_destroy:
	if ( fv_list==fv )
	    fv_list = fv->next;
	else {
	    FontView *n;
	    for ( n=fv_list; n->next!=fv; n=n->next );
	    n->next = fv->next;
	}
	if ( fv_list!=NULL )		/* Freeing a large font can take forever, and if we're just going to exit there's no real reason to do so... */
	    FontViewFree(fv);
      break;
    }
return( true );
}

static void FontViewOpenKids(FontView *fv) {
    int k, i;
    SplineFont *sf = fv->sf, *_sf;

    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;

    k=0;
    do {
	_sf = sf->subfontcnt==0 ? sf : sf->subfonts[k];
	for ( i=0; i<_sf->glyphcnt; ++i )
	    if ( _sf->glyphs[i]!=NULL && _sf->glyphs[i]->wasopen ) {
		_sf->glyphs[i]->wasopen = false;
		CharViewCreate(_sf->glyphs[i],fv,-1);
	    }
	++k;
    } while ( k<sf->subfontcnt );
}
#endif

FontView *_FontViewCreate(SplineFont *sf) {
    FontView *fv = gcalloc(1,sizeof(FontView));
    int i;
    int ps = sf->display_size<0 ? -sf->display_size :
	     sf->display_size==0 ? default_fv_font_size : sf->display_size;

    if ( ps>200 ) ps = 128;

    fv->nextsame = sf->fv;
    sf->fv = fv;
    if ( sf->mm!=NULL ) {
	sf->mm->normal->fv = fv;
	for ( i = 0; i<sf->mm->instance_count; ++i )
	    sf->mm->instances[i]->fv = fv;
    }
    if ( sf->subfontcnt==0 ) {
	fv->sf = sf;
	if ( fv->nextsame!=NULL ) {
	    fv->map = EncMapCopy(fv->nextsame->map);
	    fv->normal = fv->nextsame->normal==NULL ? NULL : EncMapCopy(fv->nextsame->normal);
	} else if ( sf->compacted ) {
	    fv->normal = sf->map;
	    fv->map = CompactEncMap(EncMapCopy(sf->map),sf);
	} else {
	    fv->map = sf->map;
	    fv->normal = NULL;
	}
    } else {
	fv->cidmaster = sf;
	for ( i=0; i<sf->subfontcnt; ++i )
	    sf->subfonts[i]->fv = fv;
	for ( i=0; i<sf->subfontcnt; ++i )	/* Search for a subfont that contains more than ".notdef" (most significant in .gai fonts) */
	    if ( sf->subfonts[i]->glyphcnt>1 ) {
		fv->sf = sf->subfonts[i];
	break;
	    }
	if ( fv->sf==NULL )
	    fv->sf = sf->subfonts[0];
	sf = fv->sf;
	if ( fv->nextsame==NULL ) EncMapFree(sf->map);
	fv->map = EncMap1to1(sf->glyphcnt);
    }
    fv->selected = gcalloc(fv->map->enccount,sizeof(char));
    fv->user_requested_magnify = -1;
    fv->magnify = (ps<=9)? 3 : (ps<20) ? 2 : 1;
    fv->cbw = (ps*fv->magnify)+1;
    fv->cbh = (ps*fv->magnify)+1+fv->lab_height+1;
    fv->antialias = sf->display_antialias;
    fv->bbsized = sf->display_bbsized;
    fv->glyphlabel = default_fv_glyphlabel;

    fv->end_pos = -1;
#ifndef _NO_PYTHON
    PyFF_InitFontHook(fv);
#endif
return( fv );
}

static void FontViewInit(void) {
    mb2DoGetText(mblist);
    mbDoGetText(fvpopupmenu);
}

FontView *FontViewCreate(SplineFont *sf) {
    FontView *fv = _FontViewCreate(sf);
    static int done = false;
#if defined(FONTFORGE_CONFIG_GTK)
    GtkWidget *status;
    PangoContext *context;
    PangoFont *font;
    PangoFontMetrics *fm;
    int as, ds;
    BDFFont *bdf;

    if ( !done ) { FontViewInit(); done=true;}

    fv->gw = create_FontView();
    g_object_set_data (G_OBJECT (fv->gw), "data", fv );
    fv->v = lookup_widget( fv->gw,"view" );
    status = lookup_widget( fv->gw,"status" );
    context = gtk_widget_get_pango_context( status );
    fv->gc = gdk_gc_new( fv->v );
    gdk_gc_copy(fv->gc,fv->v->style->fg_gc[fv->v->state]);
    font = pango_context_load_font( context, pango_context_get_font_description(context));
    fm = pango_font_get_metrics(font,NULL);
    as = pango_font_metrics_get_ascent(fm);
    ds = pango_font_metrics_get_descent(fm);
    fv->infoh = (as+ds)/1024;
    gtk_widget_set_size_request(status,0,fv->infoh);

    context = gtk_widget_get_pango_context( fv->v );
    font = pango_context_load_font( context, pango_context_get_font_description(context));
    fm = pango_font_get_metrics(font,NULL);
    as = pango_font_metrics_get_ascent(fm);
    ds = pango_font_metrics_get_descent(fm);
    fv->lab_height = (as+ds)/1024;
    fv->lab_as = as/1024;

    g_object_set_data( fv->gw, "data", fv );
    g_object_set_data( fv->v , "data", fv );

    {
	GdkGC *gc = fv->v->style->fg_gc[fv->v->state];
	GdkGCValues values;
	gdk_gc_get_values(gc,&values);
	default_background = COLOR_CREATE(
			     ((values.background.red  &0xff00)>>8) |
			     ((values.background.green&0xff00)>>8) |
			     ((values.background.blue &0xff00)>>8));
    }
#elif defined(FONTFORGE_CONFIG_GDRAW)
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetData gd;
    GRect gsize;
    FontRequest rq;
    /* sadly, clearlyu is too big for the space I've got */
    static unichar_t monospace[] = { 'f','o','n','t','v','i','e','w',',','c','o','u','r','i','e','r',',','m', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
    static unichar_t *fontnames=NULL;
    static GWindow icon = NULL;
    static int nexty=0;
    GRect size;
    BDFFont *bdf;
    int as,ds,ld;

    if ( !done ) { FontViewInit(); done=true;}
    if ( icon==NULL ) {
#ifdef BIGICONS
	icon = GDrawCreateBitmap(NULL,fontview_width,fontview_height,fontview_bits);
#else
	icon = GDrawCreateBitmap(NULL,fontview2_width,fontview2_height,fontview2_bits);
#endif
    }

    GDrawGetSize(GDrawGetRoot(NULL),&size);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.cursor = ct_pointer;
    wattrs.icon = icon;
    pos.width = sf->desired_col_cnt*fv->cbw+1;
    pos.height = sf->desired_row_cnt*fv->cbh+1;
    pos.x = size.width-pos.width-30; pos.y = nexty;
    nexty += 2*fv->cbh+50;
    if ( nexty+pos.height > size.height )
	nexty = 0;
    fv->gw = gw = GDrawCreateTopWindow(NULL,&pos,fv_e_h,fv,&wattrs);
    FVSetTitle(fv);

    if ( !fv_fs_init )
	fv_fontsize = -GResourceFindInt("FontView.FontSize",13);

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    helplist[0].invoke = FVMenuContextualHelp;
#ifndef _NO_PYTHON
    if ( fvpy_menu!=NULL )
	mblist[3].ti.disabled = false;
    mblist[3].sub = fvpy_menu;
#endif		/* _NO_PYTHON */
    gd.u.menu2 = mblist;
    fv->mb = GMenu2BarCreate( gw, &gd, NULL);
    GGadgetGetSize(fv->mb,&gsize);
    fv->mbh = gsize.height;
    fv->infoh = 1-fv_fontsize;
    fv->lab_height = FV_LAB_HEIGHT-13-fv_fontsize;

    gd.pos.y = fv->mbh+fv->infoh; gd.pos.height = pos.height;
    gd.pos.width = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gd.pos.x = pos.width;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    fv->vsb = GScrollBarCreate(gw,&gd,fv);

    wattrs.mask = wam_events|wam_cursor;
    pos.x = 0; pos.y = fv->mbh+fv->infoh;
    fv->v = GWidgetCreateSubWindow(gw,&pos,v_e_h,fv,&wattrs);
    GDrawSetVisible(fv->v,true);

    fv->gic = GDrawCreateInputContext(fv->v,gic_root|gic_orlesser);
    GDrawSetGIC(fv->v,fv->gic,0,20);

    if ( fontnames==NULL ) {
	fontnames = uc_copy(GResourceFindString("FontView.FontFamily"));
	if ( fontnames==NULL )
	    fontnames = monospace;
    }
    fv->fontset = gcalloc(_uni_fontmax,sizeof(GFont *));
    memset(&rq,0,sizeof(rq));
    rq.family_name = fontnames;
    rq.point_size = fv_fontsize;
    rq.weight = 400;
    fv->fontset[0] = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawSetFont(fv->v,fv->fontset[0]);
    GDrawFontMetrics(fv->fontset[0],&as,&ds,&ld);
    fv->lab_as = as;
#endif
    fv->showhmetrics = default_fv_showhmetrics;
    fv->showvmetrics = default_fv_showvmetrics && sf->hasvmetrics;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( fv->nextsame!=NULL ) {
	fv->filled = fv->nextsame->filled;
	bdf = fv->nextsame->show;
    } else {
	bdf = SplineFontPieceMeal(fv->sf,sf->display_size<0?-sf->display_size:default_fv_font_size,
		(fv->antialias?pf_antialias:0)|(fv->bbsized?pf_bbsized:0)|
		    (use_freetype_to_rasterize_fv && !sf->strokedfont && !sf->multilayer?pf_ft_nohints:0),
		NULL);
	fv->filled = bdf;
	if ( sf->display_size>0 ) {
	    for ( bdf=sf->bitmaps; bdf!=NULL && bdf->pixelsize!=sf->display_size ;
		    bdf=bdf->next );
	    if ( bdf==NULL )
		bdf = fv->filled;
	}
	if ( sf->onlybitmaps && bdf==fv->filled && sf->bitmaps!=NULL )
	    bdf = sf->bitmaps;
    }
    fv->cbw = -1;
    FVChangeDisplayFont(fv,bdf);
#endif

#if defined(FONTFORGE_CONFIG_GTK)
    gtk_widget_show(fv->gw);
#elif defined(FONTFORGE_CONFIG_GDRAW)
    /*GWidgetHidePalettes();*/
    GDrawSetVisible(gw,true);
    FontViewOpenKids(fv);
#endif
return( fv );
}

static SplineFont *SFReadPostscript(char *filename) {
    FontDict *fd=NULL;
    SplineFont *sf=NULL;

# ifdef FONTFORGE_CONFIG_GDRAW
    gwwv_progress_change_stages(2);
    fd = ReadPSFont(filename);
    gwwv_progress_next_stage();
    gwwv_progress_change_line2(_("Interpreting Glyphs"));
# elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_change_stages(2);
    fd = ReadPSFont(filename);
    gwwv_progress_next_stage();
    gwwv_progress_change_line2(_("Interpreting Glyphs"));
# else
    fd = ReadPSFont(filename);
# endif
    if ( fd!=NULL ) {
	sf = SplineFontFromPSFont(fd);
	PSFontFree(fd);
	if ( sf!=NULL )
	    CheckAfmOfPostscript(sf,filename,sf->map);
    }
return( sf );
}

/* This does not check currently existing fontviews, and should only be used */
/*  by LoadSplineFont (which does) and by RevertFile (which knows what it's doing) */
SplineFont *ReadSplineFont(char *filename,enum openflags openflags) {
    SplineFont *sf;
    char ubuf[250], *temp;
    int fromsfd = false;
    int i;
    char *pt, *strippedname, *oldstrippedname, *tmpfile=NULL, *paren=NULL, *fullname=filename;
    int len;
    FILE *foo;
    int checked;
    int compression=0;

    if ( filename==NULL )
return( NULL );

    strippedname = filename;
    pt = strrchr(filename,'/');
    if ( pt==NULL ) pt = filename;
    if ( (paren=strchr(pt,'('))!=NULL && strchr(paren,')')!=NULL ) {
	strippedname = copy(filename);
	strippedname[paren-filename] = '\0';
    }

    pt = strrchr(strippedname,'.');
    i = -1;
    if ( pt!=NULL ) for ( i=0; compressors[i].ext!=NULL; ++i )
	if ( strcmp(compressors[i].ext,pt)==0 )
    break;
    oldstrippedname = strippedname;
    if ( i==-1 || compressors[i].ext==NULL ) i=-1;
    else {
	tmpfile = Decompress(strippedname,i);
	if ( tmpfile!=NULL ) {
	    strippedname = tmpfile;
	} else {
	    gwwv_post_error(_("Decompress Failed!"),_("Decompress Failed!"));
return( NULL );
	}
	compression = i+1;
	if ( strippedname!=filename && paren!=NULL ) {
	    fullname = galloc(strlen(strippedname)+strlen(paren)+1);
	    strcpy(fullname,strippedname);
	    strcat(fullname,paren);
	} else
	    fullname = strippedname;
    }

    /* If there are no pfaedit windows, give them something to look at */
    /*  immediately. Otherwise delay a bit */
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    strcpy(ubuf,_("Loading font from "));
    len = strlen(ubuf);
# ifdef FONTFORGE_CONFIG_GDRAW
    strncat(ubuf,temp = def2utf8_copy(GFileNameTail(fullname)),100);
# elif defined(FONTFORGE_CONFIG_GTK)
    {
    gsize read, written;
    strncat(ubuf,temp = g_filename_to_utf8(GFileNameTail(fullname),-1,&read,&written,NULL),100);
    }
#endif
    free(temp);
    ubuf[100+len] = '\0';
    gwwv_progress_start_indicator(fv_list==NULL?0:10,_("Loading..."),ubuf,_("Reading Glyphs"),0,1);
    gwwv_progress_enable_stop(0);
    if ( fv_list==NULL && !no_windowing_ui ) { GDrawSync(NULL); GDrawProcessPendingEvents(NULL); }
#else
    len = 0;
#endif

    sf = NULL;
    foo = fopen(strippedname,"rb");
    checked = false;
/* checked == false => not checked */
/* checked == 'u'   => UFO */
/* checked == 't'   => TTF/OTF */
/* checked == 'p'   => pfb/general postscript */
/* checked == 'P'   => pdf */
/* checked == 'c'   => cff */
/* checked == 'S'   => svg */
/* checked == 'f'   => sfd */
/* checked == 'F'   => sfdir */
/* checked == 'b'   => bdf */
/* checked == 'i'   => ikarus */
    if ( GFileIsDir(strippedname) ) {
	char *temp = galloc(strlen(strippedname)+strlen("/glyphs/contents.plist")+1);
	strcpy(temp,strippedname);
	strcat(temp,"/glyphs/contents.plist");
	if ( GFileExists(temp)) {
	    sf = SFReadUFO(strippedname,0);
	    checked = 'u';
	} else {
	    strcpy(temp,strippedname);
	    strcat(temp,"/font.props");
	    if ( GFileExists(temp)) {
		sf = SFDirRead(strippedname);
		checked = 'F';
	    }
	}
	free(temp);
	if ( foo!=NULL )
	    fclose(foo);
    } else if ( foo!=NULL ) {
	/* Try to guess the file type from the first few characters... */
	int ch1 = getc(foo);
	int ch2 = getc(foo);
	int ch3 = getc(foo);
	int ch4 = getc(foo);
	int ch5 = getc(foo);
	int ch6 = getc(foo);
	int ch7 = getc(foo);
	int ch9, ch10;
	fseek(foo, 98, SEEK_SET);
	ch9 = getc(foo);
	ch10 = getc(foo);
	fclose(foo);
	if (( ch1==0 && ch2==1 && ch3==0 && ch4==0 ) ||
		(ch1=='O' && ch2=='T' && ch3=='T' && ch4=='O') ||
		(ch1=='t' && ch2=='r' && ch3=='u' && ch4=='e') ||
		(ch1=='t' && ch2=='t' && ch3=='c' && ch4=='f') ) {
	    sf = SFReadTTF(fullname,0,openflags);
	    checked = 't';
	} else if (( ch1=='%' && ch2=='!' ) ||
		    ( ch1==0x80 && ch2=='\01' ) ) {	/* PFB header */
	    sf = SFReadPostscript(fullname);
	    checked = 'p';
	} else if ( ch1=='%' && ch2=='P' && ch3=='D' && ch4=='F' ) {
	    sf = SFReadPdfFont(fullname,openflags);
	    checked = 'P';
	} else if ( ch1==1 && ch2==0 && ch3==4 ) {
	    sf = CFFParse(fullname);
	    checked = 'c';
	} else if ( ch1=='<' && ch2=='?' && (ch3=='x'||ch3=='X') && (ch4=='m'||ch4=='M') ) {
	    sf = SFReadSVG(fullname,0);
	    checked = 'S';
	} else if ( ch1==0xef && ch2==0xbb && ch3==0xbf &&
		ch4=='<' && ch5=='?' && (ch6=='x'||ch6=='X') && (ch7=='m'||ch7=='M') ) {
	    /* UTF-8 SVG with initial byte ordering mark */
	    sf = SFReadSVG(fullname,0);
	    checked = 'S';
#if 0		/* I'm not sure if this is a good test for mf files... */
	} else if ( ch1=='%' && ch2==' ' ) {
	    sf = SFFromMF(fullname);
#endif
	} else if ( ch1=='S' && ch2=='p' && ch3=='l' && ch4=='i' ) {
	    sf = SFDRead(fullname);
	    checked = 'f';
	    fromsfd = true;
	} else if ( ch1=='S' && ch2=='T' && ch3=='A' && ch4=='R' ) {
	    sf = SFFromBDF(fullname,0,false);
	    checked = 'b';
	} else if ( ch1=='\1' && ch2=='f' && ch3=='c' && ch4=='p' ) {
	    sf = SFFromBDF(fullname,2,false);
	} else if ( ch9=='I' && ch10=='K' && ch3==0 && ch4==55 ) {
	    /* Ikarus font type appears at word 50 (byte offset 98) */
	    /* Ikarus name section length (at word 2, byte offset 2) was 55 in the 80s at URW */
	    checked = 'i';
	    sf = SFReadIkarus(fullname);
	} /* Too hard to figure out a valid mark for a mac resource file */
    }

    if ( sf!=NULL )
	/* good */;
    else if (( strmatch(fullname+strlen(fullname)-4, ".sfd")==0 ||
	 strmatch(fullname+strlen(fullname)-5, ".sfd~")==0 ) && checked!='f' ) {
	sf = SFDRead(fullname);
	fromsfd = true;
    } else if (( strmatch(fullname+strlen(fullname)-4, ".ttf")==0 ||
		strmatch(fullname+strlen(strippedname)-4, ".ttc")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".gai")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".otf")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".otb")==0 ) && checked!='t') {
	sf = SFReadTTF(fullname,0,openflags);
    } else if ( strmatch(fullname+strlen(fullname)-4, ".svg")==0 && checked!='S' ) {
	sf = SFReadSVG(fullname,0);
    } else if ( strmatch(fullname+strlen(fullname)-4, ".ufo")==0 && checked!='u' ) {
	sf = SFReadUFO(fullname,0);
    } else if ( strmatch(fullname+strlen(fullname)-4, ".bdf")==0 && checked!='b' ) {
	sf = SFFromBDF(fullname,0,false);
    } else if ( strmatch(fullname+strlen(fullname)-2, "pk")==0 ) {
	sf = SFFromBDF(fullname,1,true);
    } else if ( strmatch(fullname+strlen(fullname)-2, "gf")==0 ) {
	sf = SFFromBDF(fullname,3,true);
    } else if ( strmatch(fullname+strlen(fullname)-4, ".pcf")==0 ||
		 strmatch(fullname+strlen(fullname)-4, ".pmf")==0 ) {
	/* Sun seems to use a variant of the pcf format which they call pmf */
	/*  the encoding actually starts at 0x2000 and the one I examined was */
	/*  for a pixel size of 200. Some sort of printer font? */
	sf = SFFromBDF(fullname,2,false);
    } else if ( strmatch(fullname+strlen(strippedname)-4, ".bin")==0 ||
		strmatch(fullname+strlen(strippedname)-4, ".hqx")==0 ||
		strmatch(fullname+strlen(strippedname)-6, ".dfont")==0 ) {
	sf = SFReadMacBinary(fullname,0,openflags);
    } else if ( strmatch(fullname+strlen(strippedname)-4, ".fon")==0 ||
		strmatch(fullname+strlen(strippedname)-4, ".fnt")==0 ) {
	sf = SFReadWinFON(fullname,0);
    } else if ( strmatch(fullname+strlen(strippedname)-4, ".pdb")==0 ) {
	sf = SFReadPalmPdb(fullname,0);
    } else if ( (strmatch(fullname+strlen(fullname)-4, ".pfa")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".pfb")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".pf3")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".cid")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".gsf")==0 ||
		strmatch(fullname+strlen(fullname)-4, ".pt3")==0 ||
		strmatch(fullname+strlen(fullname)-3, ".ps")==0 ) && checked!='p' ) {
	sf = SFReadPostscript(fullname);
    } else if ( strmatch(fullname+strlen(fullname)-4, ".cff")==0 && checked!='c' ) {
	sf = CFFParse(fullname);
    } else if ( strmatch(fullname+strlen(fullname)-3, ".mf")==0 ) {
	sf = SFFromMF(fullname);
    } else if ( strmatch(fullname+strlen(fullname)-4, ".pdf")==0 && checked!='P' ) {
	sf = SFReadPdfFont(fullname,openflags);
    } else if ( strmatch(fullname+strlen(fullname)-3, ".ik")==0 && checked!='i' ) {
	sf = SFReadIkarus(fullname);
    } else {
	sf = SFReadMacBinary(fullname,0,openflags);
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_end_indicator();
# endif

    if ( sf!=NULL ) {
	SplineFont *norm = sf->mm!=NULL ? sf->mm->normal : sf;
	if ( compression!=0 ) {
	    free(sf->filename);
	    *strrchr(oldstrippedname,'.') = '\0';
	    sf->filename = copy( oldstrippedname );
	}
	if ( fromsfd )
	    sf->compression = compression;
	free( norm->origname );
	if ( sf->chosenname!=NULL && strippedname==filename ) {
	    norm->origname = galloc(strlen(filename)+strlen(sf->chosenname)+8);
	    strcpy(norm->origname,filename);
	    strcat(norm->origname,"(");
	    strcat(norm->origname,sf->chosenname);
	    strcat(norm->origname,")");
	} else
	    norm->origname = copy(filename);
	free( sf->chosenname ); sf->chosenname = NULL;
	if ( sf->mm!=NULL ) {
	    int j;
	    for ( j=0; j<sf->mm->instance_count; ++j ) {
		free(sf->mm->instances[j]->origname);
		sf->mm->instances[j]->origname = copy(norm->origname);
	    }
	}
    } else if ( !GFileExists(filename) )
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Couldn't open font"),_("The requested file, %.100s, does not exist"),GFileNameTail(filename));
#else
	gwwv_post_error(_("Couldn't open font"),_("The requested file, %.100s, does not exist"),GFileNameTail(filename));
#endif
    else if ( !GFileReadable(filename) )
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Couldn't open font"),_("You do not have permission to read %.100s"),GFileNameTail(filename));
#else
	gwwv_post_error(_("Couldn't open font"),_("You do not have permission to read %.100s"),GFileNameTail(filename));
#endif
    else
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Couldn't open font"),_("%.100s is not in a known format (or is so badly corrupted as to be unreadable)"),GFileNameTail(filename));
#else
	gwwv_post_error(_("Couldn't open font"),_("%.100s is not in a known format (or is so badly corrupted as to be unreadable)"),GFileNameTail(filename));
#endif

    if ( oldstrippedname!=filename )
	free(oldstrippedname);
    if ( fullname!=filename && fullname!=strippedname )
	free(fullname);
    if ( tmpfile!=NULL ) {
	unlink(tmpfile);
	free(tmpfile);
    }
    if ( (openflags&of_fstypepermitted) && sf!=NULL && (sf->pfminfo.fstype&0xff)==0x0002 ) {
	/* Ok, they have told us from a script they have access to the font */
    } else if ( !fromsfd && sf!=NULL && (sf->pfminfo.fstype&0xff)==0x0002 ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	char *buts[3];
	buts[0] = _("_Yes"); buts[1] = _("_No"); buts[2] = NULL;
	if ( gwwv_ask(_("Restricted Font"),(const char **) buts,1,1,_("This font is marked with an FSType of 2 (Restricted\nLicense). That means it is not editable without the\npermission of the legal owner.\n\nDo you have such permission?"))==1 ) {
#elif defined(FONTFORGE_CONFIG_GTK)
	static char *buts[] = { GTK_STOCK_YES, GTK_STOCK_NO, NULL };
	if ( gwwv_ask(_("Restricted Font"),(const char **) buts,1,1,_("This font is marked with an FSType of 2 (Restricted\nLicense). That means it is not editable without the\npermission of the legal owner.\n\nDo you have such permission?"))==1 ) {
#else
	if ( true ) {		/* In a script, if they didn't explicitly say so, fail */
#endif
	    SplineFontFree(sf);
return( NULL );
	}
    }
return( sf );
}

static SplineFont *AbsoluteNameCheck(char *filename) {
    char buffer[1025];
    FontView *fv;

    GFileGetAbsoluteName(filename,buffer,sizeof(buffer)); 
    for ( fv=fv_list; fv!=NULL ; fv=fv->next ) {
	if ( fv->sf->filename!=NULL && strcmp(fv->sf->filename,buffer)==0 )
return( fv->sf );
	else if ( fv->sf->origname!=NULL && strcmp(fv->sf->origname,buffer)==0 )
return( fv->sf );
    }
return( NULL );
}

char *ToAbsolute(char *filename) {
    char buffer[1025];

    GFileGetAbsoluteName(filename,buffer,sizeof(buffer));
return( copy(buffer));
}

SplineFont *LoadSplineFont(char *filename,enum openflags openflags) {
    FontView *fv;
    SplineFont *sf;
    char *pt, *ept, *tobefreed1=NULL, *tobefreed2=NULL;
    static char *extens[] = { ".sfd", ".pfa", ".pfb", ".ttf", ".otf", ".ps", ".cid", ".bin", ".dfont", ".PFA", ".PFB", ".TTF", ".OTF", ".PS", ".CID", ".BIN", ".DFONT", NULL };
    int i;

    if ( filename==NULL )
return( NULL );

    if (( pt = strrchr(filename,'/'))==NULL ) pt = filename;
    if ( strchr(pt,'.')==NULL ) {
	/* They didn't give an extension. If there's a file with no extension */
	/*  see if it's a valid font file (and if so use the extensionless */
	/*  filename), otherwise guess at an extension */
	/* For some reason Adobe distributes CID keyed fonts (both OTF and */
	/*  postscript) as extensionless files */
	int ok = false;
	FILE *test = fopen(filename,"rb");
	if ( test!=NULL ) {
#if 0
	    int ch1 = getc(test);
	    int ch2 = getc(test);
	    int ch3 = getc(test);
	    int ch4 = getc(test);
	    if ( ch1=='%' ) ok = true;
	    else if (( ch1==0 && ch2==1 && ch3==0 && ch4==0 ) ||
		    (  ch1==0 && ch2==2 && ch3==0 && ch4==0 ) ||
		    /* Windows 3.1 Chinese version used this version for some arphic fonts */
		    /* See discussion on freetype list, july 2004 */
		    (ch1=='O' && ch2=='T' && ch3=='T' && ch4=='O') ||
		    (ch1=='t' && ch2=='r' && ch3=='u' && ch4=='e') ||
		    (ch1=='t' && ch2=='t' && ch3=='c' && ch4=='f') ) ok = true;
	    else if ( ch1=='S' && ch2=='p' && ch3=='l' && ch4=='i' ) ok = true;
#endif
	    ok = true;		/* Mac resource files are too hard to check for */
		    /* If file exists, assume good */
	    fclose(test);
	}
	if ( !ok ) {
	    tobefreed1 = galloc(strlen(filename)+8);
	    strcpy(tobefreed1,filename);
	    ept = tobefreed1+strlen(tobefreed1);
	    for ( i=0; extens[i]!=NULL; ++i ) {
		strcpy(ept,extens[i]);
		if ( GFileExists(tobefreed1))
	    break;
	    }
	    if ( extens[i]!=NULL )
		filename = tobefreed1;
	    else {
		free(tobefreed1);
		tobefreed1 = NULL;
	    }
	}
    } else
	tobefreed1 = NULL;

    sf = NULL;
    /* Only one view per font */
    for ( fv=fv_list; fv!=NULL && sf==NULL; fv=fv->next ) {
	if ( fv->sf->filename!=NULL && strcmp(fv->sf->filename,filename)==0 )
	    sf = fv->sf;
	else if ( fv->sf->origname!=NULL && strcmp(fv->sf->origname,filename)==0 )
	    sf = fv->sf;
    }
    if ( sf==NULL && *filename!='/' )
	sf = AbsoluteNameCheck(filename);
    if ( sf==NULL && *filename!='/' )
	filename = tobefreed2 = ToAbsolute(filename);

    if ( sf==NULL )
	sf = ReadSplineFont(filename,openflags);

    free(tobefreed1);
    free(tobefreed2);
return( sf );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
FontView *ViewPostscriptFont(char *filename) {
    SplineFont *sf = LoadSplineFont(filename,0);
    extern NameList *force_names_when_opening;
    if ( sf==NULL )
return( NULL );
    if ( sf->fv==NULL && force_names_when_opening!=NULL )
	SFRenameGlyphsToNamelist(sf,force_names_when_opening );
#if 0
    if ( sf->fv!=NULL ) {
	GDrawSetVisible(sf->fv->gw,true);
	GDrawRaise(sf->fv->gw);
return( sf->fv );
    }
#endif
return( FontViewCreate(sf));	/* Always make a new view now */
}

FontView *FontNew(void) {
return( FontViewCreate(SplineFontNew()));
}
#endif

void FontViewFree(FontView *fv) {
    int i;
    FontView *prev;
    FontView *fvs;

    if ( fv->sf == NULL )	/* Happens when usurping a font to put it into an MM */
	BDFFontFree(fv->filled);
    else if ( fv->nextsame==NULL && fv->sf->fv==fv ) {
	EncMapFree(fv->map);
	SplineFontFree(fv->cidmaster?fv->cidmaster:fv->sf);
	BDFFontFree(fv->filled);
    } else {
	EncMapFree(fv->map);
	for ( fvs=fv->sf->fv, i=0 ; fvs!=NULL; fvs = fvs->nextsame )
	    if ( fvs->filled==fv->filled ) ++i;
	if ( i==1 )
	    BDFFontFree(fv->filled);
	if ( fv->sf->fv==fv ) {
	    if ( fv->cidmaster==NULL )
		fv->sf->fv = fv->nextsame;
	    else {
		fv->cidmaster->fv = fv->nextsame;
		for ( i=0; i<fv->cidmaster->subfontcnt; ++i )
		    fv->cidmaster->subfonts[i]->fv = fv->nextsame;
	    }
	} else {
	    for ( prev = fv->sf->fv; prev->nextsame!=fv; prev=prev->nextsame );
	    prev->nextsame = fv->nextsame;
	}
    }
    DictionaryFree(fv->fontvars);
    free(fv->fontvars);
    free(fv->selected);
    free(fv->fontset);
#ifndef _NO_PYTHON
    PyFF_FreeFV(fv);
#endif
    free(fv);
}

void FVFakeMenus(FontView *fv,int cmd) {
    switch ( cmd ) {
      case 0:	/* Cut */
	FVCopy(fv,ct_fullcopy);
	FVClear(fv);
      break;
      case 1:
	FVCopy(fv,ct_fullcopy);
      break;
      case 2:	/* Copy reference */
	FVCopy(fv,ct_reference);
      break;
      case 3:
	FVCopyWidth(fv,ut_width);
      break;
      case 4:
	PasteIntoFV(fv,false,NULL);
      break;
      case 5:
	FVClear(fv);
      break;
      case 6:
	FVClearBackground(fv);
      break;
      case 7:
	FVCopyFgtoBg(fv);
      break;
      case 8:
	FVUnlinkRef(fv);
      break;
      case 9:
	PasteIntoFV(fv,true,NULL);
      break;
      case 10:
	FVCopyWidth(fv,ut_vwidth);
      break;
      case 11:
	FVCopyWidth(fv,ut_lbearing);
      break;
      case 12:
	FVCopyWidth(fv,ut_rbearing);
      break;
      case 13:
	FVJoin(fv);
      break;
      case 14:
	FVSameGlyphAs(fv);
      break;

      case 100:
	FVOverlap(fv,over_remove);
      break;
#if 0		/* Not used any more */
      case 101:
	FVSimplify(fv,false);
      break;
#endif
      case 102:
	FVAddExtrema(fv);
      break;
      case 103:
	FVRound2Int(fv,1.0);
      break;
      case 104:
	FVOverlap(fv,over_intersect);
      break;
      case 105:
	FVOverlap(fv,over_findinter);
      break;

      case 200:
	FVAutoHint(fv);
      break;
      case 201:
	FVClearHints(fv);
      break;
      case 202:
	FVAutoInstr(fv,true);
      break;
      case 203:
	FVAutoHintSubs(fv);
      break;
      case 204:
	FVAutoCounter(fv);
      break;
      case 205:
	FVDontAutoHint(fv);
      break;
      case 206:
	FVClearInstrs(fv);
      break;
    }
}

int FVWinInfo(FontView *fv, int *cc, int *rc) {
    if ( fv==NULL || fv->colcnt==0 || fv->rowcnt==0 ) {
	*cc = 16; *rc = 4;
return( -1 );
    }

    *cc = fv->colcnt;
    *rc = fv->rowcnt;

return( fv->rowoff*fv->colcnt );
}
