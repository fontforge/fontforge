/* Copyright (C) 2000-2008 by George Williams */
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
#include "fontforgegtk.h"
#include <fontforge/groups.h>
#include <fontforge/psfont.h>
#include <gfile.h>
#include <utype.h>
#include <ustring.h>
#include <glib.h>
#include <math.h>
#include <unistd.h>
#include <gdk/gdkkeysyms.h>

int OpenCharsInNewWindow = 1;
char *RecentFiles[RECENT_MAX] = { NULL };
int save_to_dir = 0;			/* use sfdir rather than sfd */
char *script_menu_names[SCRIPT_MENU_MAX];
char *script_filenames[SCRIPT_MENU_MAX];
extern int onlycopydisplayed, copymetadata, copyttfinstr;
extern struct compressors compressors[];
int display_has_alpha;

enum glyphlable { gl_glyph, gl_name, gl_unicode, gl_encoding };
extern int default_fv_font_size, default_fv_antialias,
	default_fv_bbsized;
int default_fv_showhmetrics=false, default_fv_showvmetrics=false;
int default_fv_glyphlabel = gl_glyph;
#define METRICS_BASELINE 0x0000c0
#define METRICS_ORIGIN	 0xc00000
#define METRICS_ADVANCE	 0x008000
FontView *fv_list=NULL;

# define FV_From_Widget(menuitem)	\
	(FontView *) g_object_get_data( \
		G_OBJECT( gtk_widget_get_toplevel( GTK_WIDGET( menuitem ))),\
		"ffdata" )

# define FV_From_MI(menuitem)	\
	(FontView *) lookup_ffdata( GTK_WIDGET( menuitem ))


static void FVBDFCInitClut(FontView *fv,BDFFont *bdf, uint32 *clut) {
    int i;

    clut[0] = 0;		/* Transparent */
    if ( bdf->clut==NULL )
	clut[1] = 0xff000000;	/* Opaque black */
    else {
	GdkColor *fg = &fv->v->style->fg[fv->v->state];
	GdkColor *bg = &fv->v->style->bg[fv->v->state];
	for ( i=1; i<bdf->clut->clut_len; ++i ) {
	    int r,g,b;
	    r = ((fg->red  *i + bg->red  *(bdf->clut->clut_len-1-i))/bdf->clut->clut_len)>>8;
	    g = ((fg->green*i + bg->green*(bdf->clut->clut_len-1-i))/bdf->clut->clut_len)>>8;
	    b = ((fg->blue *i + bg->blue *(bdf->clut->clut_len-1-i))/bdf->clut->clut_len)>>8;
	    clut[i] = 0xff000000 | (r<<16) | (g<<8) | b;
	}
    }
}

static void FVBDFCToSlot(FontView *fv,BDFFont *bdf, BDFChar *bdfc, uint32 *clut) {
    /* I could use a grey-scale image as an alpha channel, and set the RGB to */
    /*  the foreground color. That only works if X supports alpha channels, */
    /*  which I don't really know. Or I can do the composing here, and create */
    /*  a transparent/opaque mask (no inbetween states). That'll always work */
    /*  and have the same effect */
    /* 0=>transparent, 0xff=>opaque (I think) */
    /* Two cases: GImage may be bitmap or grey scale */
    int i,j, hpos,vpos, ii, jj, pixel;
    uint32 rgba;
    int mag = fv->magnify>0 ? fv->magnify : 1;
    int width = gdk_pixbuf_get_width(fv->char_slot);
    int height = gdk_pixbuf_get_height(fv->char_slot);
    int bpl = gdk_pixbuf_get_rowstride(fv->char_slot);
    guchar *pixel_data = gdk_pixbuf_get_pixels(fv->char_slot), *pt;

    memset(pixel_data,0,(height-1)*bpl + width*4);	/* Last line is not guaranteed to be bpl long */
    for ( i=bdfc->ymin; i<=bdfc->ymax; ++i ) {
	vpos = fv->magnify*(bdf->ascent-i);
	if ( vpos<0 || vpos>=height )
    continue;
	for ( j=bdfc->xmax-bdfc->xmin; j>=0; --j ) {
	    hpos = fv->magnify*(j);
	    if ( hpos<0 )
	continue;
	    if ( bdf->clut!=NULL )
		pixel = bdfc->bitmap[(bdfc->ymax-i)*bdfc->bytes_per_line+j];
	    else
		pixel = bdfc->bitmap[(bdfc->ymax-i)*bdfc->bytes_per_line+(j>>3)]&(0x80>>(j&7)) ? 1 : 0;
	    rgba = clut[pixel];
	    pt = pixel_data+(vpos*bpl)+hpos*4;
	    for ( ii=0; ii<mag && vpos+ii<height; ++ii ) {
		for ( jj=0; jj<mag && hpos+jj<width; ++jj ) {
		    pt[0] = (rgba>>16)&0xff;
		    pt[1] = (rgba>>8 )&0xff;
		    pt[2] = (rgba    )&0xff;
		    pt[3] = (rgba>>24)&0xff;
		    pt+=4;
		}
		pt = pixel_data+((vpos+ii+1)*bpl)+hpos*4;
	    }
	}
    }
}

static void FV_ToggleCharChanged(SplineChar *sc) {
    int i, j;
    int pos;
    FontView *fv;

    for ( fv = (FontView *) sc->parent->fv; fv!=NULL; fv=(FontView *) fv->b.nextsame ) {
	if ( fv->b.sf!=sc->parent )		/* Can happen in CID fonts if char's parent is not currently active */
    continue;
	if ( fv->v==NULL || fv->colcnt==0 )	/* Can happen in scripts */
    continue;
	for ( pos=0; pos<fv->b.map->enccount; ++pos ) if ( fv->b.map->map[pos]==sc->orig_pos ) {
	    i = pos / fv->colcnt;
	    j = pos - i*fv->colcnt;
	    i -= fv->rowoff;
 /* Normally we should be checking against fv->rowcnt (rather than <=rowcnt) */
 /*  but every now and then the WM forces us to use a window size which doesn't */
 /*  fit our expectations (maximized view) and we must be prepared for half */
 /*  lines */
	    if ( i>=0 && i<=fv->rowcnt ) {
		GdkGC *gc = fv->v->style->fg_gc[fv->v->state];
		GdkGCValues values;
		GdkColor bg;
		gdk_gc_get_values(gc,&values);
		bg.pixel = -1;
		if ( sc->color!=COLOR_DEFAULT ) {
		    bg.red   = ((sc->color>>16)&0xff)*0x101;
		    bg.green = ((sc->color>>8 )&0xff)*0x101;
		    bg.blue  = ((sc->color    )&0xff)*0x101;
		} else if ( sc->layers[ly_back].splines!=NULL || sc->layers[ly_back].images!=NULL ) {
		    bg.red = bg.green = bg.blue = 0x8000; bg.pixel = 0x808080;
		} else
		    bg = values.background;
		/* Bug!!! This only works on RealColor */
		bg.red ^= values.foreground.red;
		bg.green ^= values.foreground.green;
		bg.blue ^= values.foreground.blue;
		bg.pixel ^= values.foreground.pixel;
		/* End bug */
		gdk_gc_set_function(gc,GDK_XOR);
		gdk_gc_set_foreground(gc, &bg);
		gdk_draw_rectangle(fv->v->window, gc, TRUE,
			j*fv->cbw+1, i*fv->cbh+1,  fv->cbw-1, fv->lab_height);
		gdk_gc_set_values(gc,&values,
			GDK_GC_FOREGROUND | GDK_GC_FUNCTION );
	    }
	}
    }
}

void FVMarkHintsOutOfDate(SplineChar *sc) {
    int i, j;
    int pos;
    FontView *fv;
    int l = sc->layer_cnt;

    if ( sc->parent->onlybitmaps || sc->parent->multilayer || sc->parent->strokedfont || sc->parent->layers[l].order2 )
return;
    for ( fv = (FontView *) sc->parent->fv; fv!=NULL; fv=(FontView *) fv->b.nextsame ) {
	if ( fv->b.sf!=sc->parent )		/* Can happen in CID fonts if char's parent is not currently active */
    continue;
	if ( fv->v==NULL || fv->colcnt==0 )	/* Can happen in scripts */
    continue;
	for ( pos=0; pos<fv->b.map->enccount; ++pos ) if ( fv->b.map->map[pos]==sc->orig_pos ) {
	    i = pos / fv->colcnt;
	    j = pos - i*fv->colcnt;
	    i -= fv->rowoff;
 /* Normally we should be checking against fv->rowcnt (rather than <=rowcnt) */
 /*  but every now and then the WM forces us to use a window size which doesn't */
 /*  fit our expectations (maximized view) and we must be prepared for half */
 /*  lines */
	    if ( i>=0 && i<=fv->rowcnt ) {
		GdkColor hintcol;
		GdkRectangle r;
		hintcol.red = hintcol.green = 0;
		hintcol.blue = 0xffff;
		gdk_gc_set_rgb_fg_color(fv->gc,&hintcol);
		r.x = j*fv->cbw+1; r.width = fv->cbw-1;
		r.y = i*fv->cbh+1; r.height = fv->lab_height-1;
		gdk_draw_line(GDK_DRAWABLE(fv->v->window),fv->gc,r.x,r.y,r.x,r.y+r.height-1);
		gdk_draw_line(GDK_DRAWABLE(fv->v->window),fv->gc,r.x+1,r.y,r.x+1,r.y+r.height-1);
		gdk_draw_line(GDK_DRAWABLE(fv->v->window),fv->gc,r.x+r.width-1,r.y,r.x+r.width-1,r.y+r.height-1);
		gdk_draw_line(GDK_DRAWABLE(fv->v->window),fv->gc,r.x+r.width-2,r.y,r.x+r.width-2,r.y+r.height-1);
	    }
	}
    }
}

static void FontViewRefreshAll(SplineFont *sf) {
    FontView *fv;
    for ( fv = (FontView *) (sf->fv); fv!=NULL; fv = (FontView *) (fv->b.nextsame) )
	if ( fv->v!=NULL )
	    gtk_widget_queue_draw(fv->v);
}

void FVDeselectAll(FontView *fv) {
    int i, redraw = false;

    for ( i=0; i<fv->b.map->enccount; ++i ) {
	if ( fv->b.selected[i] ) {
	    fv->b.selected[i] = false;
	    redraw = true;
	}
    }
    fv->sel_index = 0;
    if ( redraw )
	gtk_widget_queue_draw(fv->v);
}

static void FVInvertSelection(FontView *fv) {
    int i;

    for ( i=0; i<fv->b.map->enccount; ++i ) {
	fv->b.selected[i] = !fv->b.selected[i];
    }
    fv->sel_index = 1;
    gtk_widget_queue_draw(fv->v);
}

static void FVSelectAll(FontView *fv) {
    int i, redraw = false;

    for ( i=0; i<fv->b.map->enccount; ++i ) {
	if ( !fv->b.selected[i] ) {
	    fv->b.selected[i] = true;
	    redraw = true;
	}
    }
    fv->sel_index = 1;
    if ( redraw )
	gtk_widget_queue_draw(fv->v);
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
	    /* matches any of a comma separated list of substrings */
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

static void FVSelectByName(FontView *fv) {
    int j, gid;
    char *ret, *end;
    SplineChar *sc;
    EncMap *map = fv->b.map;
    SplineFont *sf = fv->b.sf;
    struct altuni *alt;

    ret = gwwv_ask_string(_("Select all instances of the wildcard pattern"),".notdef",_("Select all instances of the wildcard pattern"));
    if ( ret==NULL )
return;
    FVDeselectAll(fv);
    if (( *ret=='0' && ( ret[1]=='x' || ret[1]=='X' )) ||
	    ((*ret=='u' || *ret=='U') && ret[1]=='+' )) {
	int uni = (int) strtol(ret+2,&end,16);
	int vs= -2;
	if ( *end=='.' ) {
	    ++end;
	    if (( *end=='0' && ( end[1]=='x' || end[1]=='X' )) ||
		    ((*end=='u' || *end=='U') && end[1]=='+' ))
		end += 2;
	    vs = (int) strtoul(end,&end,16);
	}
	if ( *end!='\0' || uni<0 || uni>=0x110000 ) {
	    free(ret);
	    gwwv_post_error( _("Bad Number"),_("Bad Number") );
return;
	}
	for ( j=0; j<map->enccount; ++j ) if ( (gid=map->map[j])!=-1 && (sc=sf->glyphs[gid])!=NULL ) {
	    if ( vs==-2 ) {
		for ( alt=sc->altuni; alt!=NULL && (alt->unienc!=uni || alt->fid!=0); alt=alt->next );
	    } else {
		for ( alt=sc->altuni; alt!=NULL && (alt->unienc!=uni || alt->vs!=vs || alt->fid!=0); alt=alt->next );
	    }
	    if ( (sc->unicodeenc == uni && vs<0) || alt!=NULL ) {
		fv->b.selected[j] = true;
	    }
	}
    } else {
	for ( j=0; j<map->enccount; ++j ) if ( (gid=map->map[j])!=-1 && (sc=sf->glyphs[gid])!=NULL ) {
	    if ( WildMatch(ret,sc->name,false) ) {
		fv->b.selected[j] = true;
	    }
	}
    }
    free(ret);
    fv->sel_index = 1;
    gtk_widget_queue_draw(fv->v);
}

static void FVSelectByScript(FontView *fv) {
    gwwv_post_notice("NYI", "Select by Script not yet implemented!!!!" );
}

static void FVSelectColor(FontView *fv, uint32 col, int door) {
    int i, any=0;
    uint32 sccol;
    SplineChar **glyphs = fv->b.sf->glyphs;

    for ( i=0; i<fv->b.map->enccount; ++i ) {
	int gid = fv->b.map->map[i];
	sccol =  ( gid==-1 || glyphs[gid]==NULL ) ? COLOR_DEFAULT : glyphs[gid]->color;
	if ( (door && !fv->b.selected[i] && sccol==col) ||
		(!door && fv->b.selected[i]!=(sccol==col)) ) {
	    fv->b.selected[i] = !fv->b.selected[i];
	    if ( fv->b.selected[i] ) any = true;
	}
    }
    fv->sel_index = any;
    gtk_widget_queue_draw(fv->v);
}

static void FVReselect(FontView *fv, int newpos) {
    int i;

    if ( newpos<0 ) newpos = 0;
    else if ( newpos>=fv->b.map->enccount ) newpos = fv->b.map->enccount-1;

    if ( fv->pressed_pos<fv->end_pos ) {
	if ( newpos>fv->end_pos ) {
	    for ( i=fv->end_pos+1; i<=newpos; ++i ) if ( !fv->b.selected[i] ) {
		fv->b.selected[i] = fv->sel_index;
	    }
	} else if ( newpos<fv->pressed_pos ) {
	    for ( i=fv->end_pos; i>fv->pressed_pos; --i ) if ( fv->b.selected[i] ) {
		fv->b.selected[i] = false;
	    }
	    for ( i=fv->pressed_pos-1; i>=newpos; --i ) if ( !fv->b.selected[i] ) {
		fv->b.selected[i] = fv->sel_index;
	    }
	} else {
	    for ( i=fv->end_pos; i>newpos; --i ) if ( fv->b.selected[i] ) {
		fv->b.selected[i] = false;
	    }
	}
    } else {
	if ( newpos<fv->end_pos ) {
	    for ( i=fv->end_pos-1; i>=newpos; --i ) if ( !fv->b.selected[i] ) {
		fv->b.selected[i] = fv->sel_index;
	    }
	} else if ( newpos>fv->pressed_pos ) {
	    for ( i=fv->end_pos; i<fv->pressed_pos; ++i ) if ( fv->b.selected[i] ) {
		fv->b.selected[i] = false;
	    }
	    for ( i=fv->pressed_pos+1; i<=newpos; ++i ) if ( !fv->b.selected[i] ) {
		fv->b.selected[i] = fv->sel_index;
	    }
	} else {
	    for ( i=fv->end_pos; i<newpos; ++i ) if ( fv->b.selected[i] ) {
		fv->b.selected[i] = false;
	    }
	}
    }
    fv->end_pos = newpos;
    gtk_widget_queue_draw(fv->v);
}

static void FVFlattenAllBitmapSelections(FontView *fv) {
    BDFFont *bdf;
    int i;

    for ( bdf = fv->b.sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
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
    buts[0] = GTK_STOCK_SAVE;
    buts[1] = _("_Don't Save");
    buts[2] = GTK_STOCK_CANCEL;
    buts[3] = NULL;
    ret = gwwv_ask( _("Font changed"),(const char **) buts,0,2,_("Font %1$.40s in file %2$.40s has been changed.\nDo you want to save it?"),fontname,filename);
return( ret );
}

int _FVMenuGenerate(FontView *fv,int family) {
    FVFlattenAllBitmapSelections(fv);
return( SFGenerateFont(fv->b.sf,family,fv->b.normal==NULL?fv->b.map:fv->b.normal) );
}

void FontViewMenu_Generate(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    _FVMenuGenerate(fv,false);
}

void FontViewMenu_GenerateFamily(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    _FVMenuGenerate(fv,true);
}

extern int save_to_dir;

static int SaveAs_FormatChange(GtkWidget *g, gpointer data) {
    GtkWidget *dlg = gtk_widget_get_toplevel(g);		/* !!!! Is this what I need???? */
    char *oldname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
    int *_s2d = data;
    int s2d = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g));
    char *pt, *newname = malloc(strlen(oldname)+8);
    strcpy(newname,oldname);
    pt = strrchr(newname,'.');
    if ( pt==NULL )
	pt = newname+strlen(newname);
    strcpy(pt,s2d ? ".sfdir" : ".sfd" );
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg),newname);
    save_to_dir = *_s2d = s2d;
    SavePrefs(true);
return( true );
}

int _FVMenuSaveAs(FontView *fv) {
    char *temp;
    char *ret;
    char *filename;
    int ok;
    int s2d = fv->b.cidmaster!=NULL ? fv->b.cidmaster->save_to_dir :
		fv->b.sf->mm!=NULL ? fv->b.sf->mm->normal->save_to_dir :
		fv->b.sf->save_to_dir;
    gsize read, written;
    GtkWidget *as_dir;

    if ( fv->b.cidmaster!=NULL && fv->b.cidmaster->filename!=NULL )
	temp=g_filename_to_utf8(fv->b.cidmaster->filename,-1,&read,&written,NULL);
    else if ( fv->b.sf->mm!=NULL && fv->b.sf->mm->normal->filename!=NULL )
	temp=g_filename_to_utf8(fv->b.sf->mm->normal->filename,-1,&read,&written,NULL);
    else if ( fv->b.sf->filename!=NULL )
	temp=g_filename_to_utf8(fv->b.sf->filename,-1,&read,&written,NULL);
    else {
	SplineFont *sf = fv->b.cidmaster?fv->b.cidmaster:
		fv->b.sf->mm!=NULL?fv->b.sf->mm->normal:fv->b.sf;
	char *fn = sf->defbasefilename ? sf->defbasefilename : sf->fontname;
	temp = malloc((strlen(fn)+10));
	strcpy(temp,fn);
	if ( sf->defbasefilename!=NULL )
	    /* Don't add a default suffix, they've already told us what name to use */;
	else if ( fv->b.cidmaster!=NULL )
	    strcat(temp,"CID");
	else if ( sf->mm==NULL )
	    ;
	else if ( sf->mm->apple )
	    strcat(temp,"Var");
	else
	    strcat(temp,"MM");
	strcat(temp,save_to_dir ? ".sfdir" : ".sfd");
	s2d = save_to_dir;
    }

    as_dir = gtk_check_button_new_with_mnemonic(_("Save as _Directory"));
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( as_dir ), s2d );
    g_signal_connect (G_OBJECT(as_dir), "toggled",
                    G_CALLBACK (SaveAs_FormatChange),
                    &s2d);
    ret = gwwv_save_filename_with_gadget(_("Save as"),temp,NULL,as_dir);
    free(temp);
    if ( ret==NULL )
return( 0 );
    filename = g_filename_from_utf8(ret,-1,&read,&written,NULL);
    free(ret);
    FVFlattenAllBitmapSelections(fv);
    fv->b.sf->compression = 0;
    ok = SFDWrite(filename,fv->b.sf,fv->b.map,fv->b.normal,s2d);
    if ( ok ) {
	SplineFont *sf = fv->b.cidmaster?fv->b.cidmaster:fv->b.sf->mm!=NULL?fv->b.sf->mm->normal:fv->b.sf;
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
	FVSetTitle( (FontViewBase *) fv);
    } else
	free(filename);
return( ok );
}

void FontViewMenu_SaveAs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    _FVMenuSaveAs(fv);
}

int _FVMenuSave(FontView *fv) {
    int ret = 0;
    SplineFont *sf = fv->b.cidmaster?fv->b.cidmaster:
		    fv->b.sf->mm!=NULL?fv->b.sf->mm->normal:
			    fv->b.sf;

    if ( sf->filename==NULL )
	ret = _FVMenuSaveAs(fv);
    else {
	FVFlattenAllBitmapSelections(fv);
	if ( !SFDWriteBak(sf,fv->b.map,fv->b.normal) )
	    gwwv_post_error(_("Save Failed"),_("Save Failed"));
	else {
	    SplineFontSetUnChanged(sf);
	    ret = true;
	}
    }
return( ret );
}

void FontViewMenu_Save(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    _FVMenuSave(fv);
}

void _FVCloseWindows(FontView *fv) {
    int i, j;
    BDFFont *bdf;
    MetricsView *mv, *mnext;
    SplineFont *sf = fv->b.cidmaster?fv->b.cidmaster:fv->b.sf->mm!=NULL?fv->b.sf->mm->normal : fv->b.sf;

    PrintWindowClose();
    if ( fv->b.nextsame==NULL && fv->b.sf->fv==(FontViewBase *) fv && fv->b.sf->kcld!=NULL )
	KCLD_End(fv->b.sf->kcld);
    if ( fv->b.nextsame==NULL && fv->b.sf->fv==(FontViewBase *) fv && fv->b.sf->vkcld!=NULL )
	KCLD_End(fv->b.sf->vkcld);

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	CharView *cv, *next;
	for ( cv = (CharView *) sf->glyphs[i]->views; cv!=NULL; cv = next ) {
	    next = (CharView *) cv->b.next;
	    gtk_widget_destroy(cv->gw);
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
		for ( cv = (CharView *) sf->glyphs[i]->views; cv!=NULL; cv = next ) {
		    next = (CharView *) cv->b.next;
		    gtk_widget_destroy(cv->gw);
		}
		if ( sf->glyphs[i]->charinfo )
		    CharInfoDestroy(sf->glyphs[i]->charinfo);
	    }
	    for ( mv=sf->metrics; mv!=NULL; mv = mnext ) {
		mnext = mv->next;
		gtk_widget_destroy(mv->gw);
	    }
	}
    } else if ( sf->subfontcnt!=0 ) {
	for ( j=0; j<sf->subfontcnt; ++j ) {
	    for ( i=0; i<sf->subfonts[j]->glyphcnt; ++i ) if ( sf->subfonts[j]->glyphs[i]!=NULL ) {
		CharView *cv, *next;
		for ( cv = (CharView *) sf->subfonts[j]->glyphs[i]->views; cv!=NULL; cv = next ) {
		    next = (CharView *) cv->b.next;
		    gtk_widget_destroy(cv->gw);
		if ( sf->subfonts[j]->glyphs[i]->charinfo )
		    CharInfoDestroy(sf->subfonts[j]->glyphs[i]->charinfo);
		}
	    }
	    for ( mv=sf->subfonts[j]->metrics; mv!=NULL; mv = mnext ) {
		mnext = mv->next;
		gtk_widget_destroy(mv->gw);
	    }
	}
    } else {
	for ( mv=sf->metrics; mv!=NULL; mv = mnext ) {
	    mnext = mv->next;
	    gtk_widget_destroy(mv->gw);
	}
    }
    for ( bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	for ( i=0; i<bdf->glyphcnt; ++i ) if ( bdf->glyphs[i]!=NULL ) {
	    BitmapView *bv, *next;
	    for ( bv = bdf->glyphs[i]->views; bv!=NULL; bv = next ) {
		next = bv->next;
		gtk_widget_destroy(bv->gw);
	    }
	}
    }
    if ( fv->b.sf->fontinfo!=NULL )
	FontInfoDestroy(fv->b.sf);
    if ( fv->b.sf->valwin!=NULL )
	ValidationDestroy(fv->b.sf);
    SVDetachFV(fv);

    g_main_context_iteration(NULL,true);	/* Waits until all window destroy events are processed (I think) */
}

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
    SplineFont *sf = fv->b.cidmaster?fv->b.cidmaster:fv->b.sf;

    if ( !SFCloseAllInstrs(fv->b.sf) )
return( false );

    if ( fv->b.nextsame!=NULL || fv->b.sf->fv!=(FontViewBase *) fv ) {
	/* There's another view, can close this one with no problems */
    } else if ( SFAnyChanged(sf) ) {
	i = AskChanged(fv->b.sf);
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
    else if ( sf->origname!=NULL )
	RecentFilesRemember(sf->origname);
    if ( fv_list==fv && fv->b.next==NULL )
exit( 0 );
    gtk_widget_destroy(fv->gw);
return( true );
}

void Menu_New(GtkMenuItem *menuitem, gpointer user_data) {
    FontNew();
}

void FontViewMenu_Close(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    _FVMenuClose(fv);
}

gboolean FontView_RequestClose(GtkWidget *widget, GdkEvent *event,
	gpointer user_data) {
    FontView *fv = (FontView *) g_object_get_data(G_OBJECT(widget),"ffdata");

    _FVMenuClose(fv);
return( true );
}

gboolean FontView_DestroyWindow(GtkWidget *widget, GdkEvent *event,
	gpointer user_data) {
    FontView *fv = (FontView *) g_object_get_data(G_OBJECT(widget),"ffdata");

    if ( fv_list==fv )
	fv_list = (FontView *) (fv->b.next);
    else {
	FontView *n;
	for ( n=fv_list; n->b.next!=&fv->b; n=(FontView *) (n->b.next) );
	n->b.next = fv->b.next;
    }
    if ( fv_list!=NULL )		/* Freeing a large font can take forever, and if we're just going to exit there's no real reason to do so... */
	FontViewFree(&fv->b);
return( true );
}

gboolean FontView_ClearSelection(GtkWidget *widget, GdkEventSelection *event,
	gpointer user_data) {
    ClipboardClear();
return( true );
}

static void FV_ReattachCVs(SplineFont *old,SplineFont *new) {
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
		for ( cv=(CharView *) old->glyphs[i]->views; cv!=NULL; cv = cvnext ) {
		    cvnext = (CharView *) cv->b.next;
		    gtk_widget_destroy(cv->gw);
		}
	    } else {
		for ( cv=(CharView *) old->glyphs[i]->views; cv!=NULL; cv = cvnext ) {
		    cvnext = (CharView *) cv->b.next;
		    CVChangeSC(cv,sub->glyphs[pos]);
		    cv->b.layerheads[dm_grid] = &new->grid;
		}
	    }
	}
    }
}

void FontViewMenu_RevertFile(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVRevert((FontViewBase *) fv);
}

void FontViewMenu_RevertToBackup(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVRevertBackup((FontViewBase *) fv);
}

void FontViewMenu_RevertGlyph(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVRevertGlyph((FontViewBase *) fv);
}

void Menu_Preferences(GtkMenuItem *menuitem, gpointer user_data) {
    DoPrefs();
}

void Menu_SaveAll(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv;

    for ( fv = fv_list; fv!=NULL; fv = (FontView *) fv->b.next ) {
	if ( SFAnyChanged(fv->b.sf) && !_FVMenuSave(fv))
return;
    }
}

static void _MenuExit(void *junk) {
    FontView *fv, *next;

    for ( fv = fv_list; fv!=NULL; fv = next ) {
	next = (FontView *) fv->b.next;
	if ( !_FVMenuClose(fv))
return;
    }
    exit(0);
}

void Menu_Quit(GtkMenuItem *menuitem, gpointer user_data) {
    _MenuExit(NULL);
}

char *GetPostscriptFontName(char *dir, int mult) {
    gsize read, written;
    char *u_dir=NULL, *ret, *temp=NULL;

    if ( dir!=NULL )
	u_dir = g_filename_to_utf8(dir,-1,&read,&written,NULL);
    ret = FVOpenFont(_("Open Font"), u_dir,mult);
    if ( ret!=NULL )
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
	    sf->mm!=NULL?wild2:wild);
    char *temp;
    gsize read, written;

    if ( ret==NULL )		/* Cancelled */
return;
    temp = g_filename_from_utf8(ret,-1,&read,&written,NULL);

    if ( !LoadKerningDataFromMetricsFile(sf,temp,map))
	gwwv_post_error( _("Failed to load kern data from %s"), ret);
    free(ret);
    free(temp);
}

void FontViewMenu_MergeKern(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    MergeKernInfo(fv->b.sf,fv->b.map);
}

void Menu_Open(GtkMenuItem *menuitem, gpointer user_data) {
    char *temp;
    char *eod, *fpt, *file, *full;
    FontViewBase *test; int fvcnt, fvtest;

    for ( fvcnt=0, test=(FontViewBase *) fv_list; test!=NULL; ++fvcnt, test=test->next );
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
	    full = malloc(strlen(temp)+1+strlen(file)+1);
	    strcpy(full,temp); strcat(full,"/"); strcat(full,file);
	    ViewPostScriptFont(full,0);
	    file = fpt+2;
	    free(full);
	} while ( fpt!=NULL );
	free(temp);
	for ( fvtest=0, test=(FontViewBase *) fv_list; test!=NULL; ++fvtest, test=test->next );
    } while ( fvtest==fvcnt );	/* did the load fail for some reason? try again */
}

void FontViewMenu_ContextualHelp(GtkMenuItem *menuitem, gpointer user_data) {
    help("fontview.html");
}

void MenuHelp_Help(GtkMenuItem *menuitem, gpointer user_data) {
    help("overview.html");
}

void MenuHelp_Index(GtkMenuItem *menuitem, gpointer user_data) {
    help("IndexFS.html");
}

void MenuHelp_License(GtkMenuItem *menuitem, gpointer user_data) {
    help("license.html");
}

void MenuHelp_About(GtkMenuItem *menuitem, gpointer user_data) {
    ShowAboutScreen();
}

void FontViewMenu_Import(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int empty = fv->b.sf->onlybitmaps && fv->b.sf->bitmaps==NULL;
    BDFFont *bdf;
    FVImport(fv);
    if ( empty && fv->b.sf->bitmaps!=NULL ) {
	for ( bdf= fv->b.sf->bitmaps; bdf->next!=NULL; bdf = bdf->next );
	FVChangeDisplayBitmap((FontViewBase *) fv,bdf);
    }
}

static int FVSelCount(FontView *fv) {
    int i, cnt=0;

    for ( i=0; i<fv->b.map->enccount; ++i )
	if ( fv->b.selected[i] ) ++cnt;
    if ( cnt>10 ) {
	static char *buts[] = { GTK_STOCK_OK, GTK_STOCK_CANCEL, NULL };
	if ( gwwv_ask(_("Many Windows"),(const char **) buts,0,1,_("This involves opening more than 10 windows.\nIs that really what you want?"))==1 )
return( false );
    }
return( true );
}
	
void FontViewMenu_OpenOutline(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int i;
    SplineChar *sc;

    if ( !FVSelCount(fv))
return;
    for ( i=0; i<fv->b.map->enccount; ++i )
	if ( fv->b.selected[i] ) {
	    sc = FVMakeChar(fv,i);
	    CharViewCreate(sc,fv,i);
	}
}

void FontViewMenu_OpenBitmap(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int i;
    SplineChar *sc;

    if ( fv->b.cidmaster==NULL ? (fv->b.sf->bitmaps==NULL) : (fv->b.cidmaster->bitmaps==NULL) )
return;
    if ( !FVSelCount(fv))
return;
    for ( i=0; i<fv->b.map->enccount; ++i )
	if ( fv->b.selected[i] ) {
	    sc = FVMakeChar(fv,i);
	    if ( sc!=NULL )
		BitmapViewCreatePick(i,fv);
	}
}

void WindowMenu_Warnings(GtkMenuItem *menuitem, gpointer user_data) {
    ShowErrorWindow();
}

void FontViewMenu_OpenMetrics(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    MetricsViewCreate(fv,NULL,fv->filled==fv->show?NULL:fv->show);
}

void FontViewMenu_Print(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    PrintFFDlg(fv,NULL,NULL);
}

void FontViewMenu_ExecScript(GtkMenuItem *menuitem, gpointer user_data) {
#if !defined(_NO_FFSCRIPT) || !defined(_NO_PYTHON)
    FontView *fv = FV_From_MI(menuitem);

    ScriptDlg(fv,NULL);
#endif
}

void FontViewMenu_FontInfo(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FontMenuFontInfo(fv);
}

void FontViewMenu_MathInfo(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SFMathDlg(fv->b.sf);
}

void FontViewMenu_FindProbs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FindProblems(fv,NULL,NULL);
}

void FontViewMenu_Validate(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SFValidationWindow(fv->b.sf,ff_none);
}

void FontViewMenu_ChangeWeight(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    EmboldenDlg(fv,NULL);
}

void FontViewMenu_Oblique(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    ObliqueDlg(fv,NULL);
}

void FontViewMenu_Condense(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    CondenseExtendDlg(fv,NULL);
}

/* returns -1 if nothing selected, if exactly one char return it, -2 if more than one */
static int FVAnyCharSelected(FontView *fv) {
    int i, val=-1;

    for ( i=0; i<fv->b.map->enccount; ++i ) {
	if ( fv->b.selected[i]) {
	    if ( val==-1 )
		val = i;
	    else
return( -2 );
	}
    }
return( val );
}

static int FVAllSelected(FontView *fv) {
    int i, any = false;
    /* Is everything real selected? */

    for ( i=0; i<fv->b.sf->glyphcnt; ++i ) if ( SCWorthOutputting(fv->b.sf->glyphs[i])) {
	if ( !fv->b.selected[fv->b.map->backmap[i]] )
return( false );
	any = true;
    }
return( any );
}

void FontViewMenu_CopyFromAll(GtkMenuItem *menuitem, gpointer user_data) {

    onlycopydisplayed = false;
    SavePrefs(true);
}

void FontViewMenu_CopyFromDisplayed(GtkMenuItem *menuitem, gpointer user_data) {

    onlycopydisplayed = true;
    SavePrefs(true);
}

void FontViewMenu_CopyFromMetadata(GtkMenuItem *menuitem, gpointer user_data) {

    copymetadata = !copymetadata;
    SavePrefs(true);
}

void FontViewMenu_CopyFromTTInstrs(GtkMenuItem *menuitem, gpointer user_data) {

    copyttfinstr = !copyttfinstr;
    SavePrefs(true);
}

void FontViewMenu_Copy(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    if ( FVAnyCharSelected(fv)==-1 )
return;
    FVCopy((FontViewBase *) fv,ct_fullcopy);
}

void FontViewMenu_CopyLookupData(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    if ( FVAnyCharSelected(fv)==-1 )
return;
    FVCopy((FontViewBase *) fv,ct_lookups);
}

void FontViewMenu_CopyRef(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    if ( FVAnyCharSelected(fv)==-1 )
return;
    FVCopy((FontViewBase *) fv,ct_reference);
}

void FontViewMenu_CopyWidth(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    if ( FVAnyCharSelected(fv)==-1 )
return;
    FVCopyWidth((FontViewBase *) fv,ut_width);
}

void FontViewMenu_CopyVWidth(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    if ( FVAnyCharSelected(fv)==-1 )
return;
    if ( !fv->b.sf->hasvmetrics )
return;
    FVCopyWidth((FontViewBase *) fv,ut_vwidth);
}

void FontViewMenu_CopyLBearing(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    if ( FVAnyCharSelected(fv)==-1 )
return;
    FVCopyWidth((FontViewBase *) fv,ut_lbearing);
}

void FontViewMenu_CopyRBearing(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    if ( FVAnyCharSelected(fv)==-1 )
return;
    FVCopyWidth((FontViewBase *) fv,ut_rbearing);
}

void FontViewMenu_Paste(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    if ( FVAnyCharSelected(fv)==-1 )
return;
    PasteIntoFV((FontViewBase *) fv,false,NULL);
}

void FontViewMenu_PasteInto(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    if ( FVAnyCharSelected(fv)==-1 )
return;
    PasteIntoFV((FontViewBase *) fv,true,NULL);
}

void FontViewMenu_PasteAfter(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int pos = FVAnyCharSelected(fv);
    if ( pos<0 )
return;
    PasteIntoFV((FontViewBase *) fv,2,NULL);
}

void FontViewMenu_SameGlyphAs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVSameGlyphAs((FontViewBase *) fv);
}

void FontViewMenu_CopyFg2Bg(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVCopyFgtoBg( (FontViewBase *) fv );
}

void FontViewMenu_Clear(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVClear( (FontViewBase *) fv );
}

void FontViewMenu_ClearBackground(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVClearBackground( (FontViewBase *) fv );
}

void FontViewMenu_Join(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVJoin( (FontViewBase *) fv );
}

void FontViewMenu_UnlinkRef(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVUnlinkRef( (FontViewBase *) fv );
}

void FontViewMenu_RemoveUndoes(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SFRemoveUndoes(fv->b.sf,fv->b.selected,fv->b.map);
}

void FontViewMenu_Undo(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVUndo((FontViewBase *) fv);
}

void FontViewMenu_Redo(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVRedo((FontViewBase *) fv);
}

void FontViewMenu_Cut(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVCopy((FontViewBase *) fv,ct_fullcopy);
    FVClear((FontViewBase *) fv);
}

void FontViewMenu_SelectAll(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVSelectAll(fv);
}

void FontViewMenu_InvertSelection(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVInvertSelection(fv);
}

void FontViewMenu_DeselectAll(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVDeselectAll(fv);
}

void FontViewMenu_SelectByName(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVSelectByName(fv);
}

void FontViewMenu_SelectByScript(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVSelectByScript(fv);
}

void FontViewMenu_SelectWorthOutputting(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int i, gid;
    EncMap *map = fv->b.map;
    SplineFont *sf = fv->b.sf;

    for ( i=0; i< map->enccount; ++i )
	fv->b.selected[i] = ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL &&
		SCWorthOutputting(sf->glyphs[gid]) );
    gtk_widget_queue_draw(fv->v);
}

void FontViewMenu_SelectChangedGlyphs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int i, gid;
    EncMap *map = fv->b.map;
    SplineFont *sf = fv->b.sf;

    for ( i=0; i< map->enccount; ++i )
	fv->b.selected[i] = ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL && sf->glyphs[gid]->changed );
    gtk_widget_queue_draw(fv->v);
}

void FontViewMenu_SelectUnhintedGlyphs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int i, gid;
    EncMap *map = fv->b.map;
    SplineFont *sf = fv->b.sf;
    int l = sf->layer_cnt;
    int order2 = sf->layers[l].order2;

    for ( i=0; i< map->enccount; ++i )
	fv->b.selected[i] = ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL &&
		((!order2 && sf->glyphs[gid]->changedsincelasthinted ) ||
		 ( order2 && sf->glyphs[gid]->layers[ly_fore].splines!=NULL &&
		     sf->glyphs[gid]->ttf_instrs_len<=0 ) ||
		 ( order2 && sf->glyphs[gid]->instructions_out_of_date )) );
    gtk_widget_queue_draw(fv->v);
}

void FontViewMenu_SelectAutohintable(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int i, gid;
    EncMap *map = fv->b.map;
    SplineFont *sf = fv->b.sf;

    for ( i=0; i< map->enccount; ++i )
	fv->b.selected[i] = (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL &&
		!sf->glyphs[gid]->manualhints;
    gtk_widget_queue_draw(fv->v);
}

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

void FontViewMenu_SelectByLookupSubtable(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVSelectByPST(fv);
}

static void FVReplaceOutlineWithReference( FontView *fv, double fudge ) {

    if ( fv->v!=NULL )
	gdk_window_set_cursor(fv->v->window,ct_watch);

    FVBReplaceOutlineWithReference((FontViewBase *) fv, fudge);

    if ( fv->v!=NULL ) {
	gtk_widget_queue_draw(fv->v);
	gdk_window_set_cursor(fv->v->window,ct_pointer);
    }
}

void FontViewMenu_FindRpl(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SVCreate(fv);
}

void FontViewMenu_RplRef(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVReplaceOutlineWithReference(fv,.001);
}

void FontViewMenu_CharInfo(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int pos = FVAnyCharSelected(fv);
    if ( pos<0 )
return;
    if ( fv->b.cidmaster!=NULL &&
	    (fv->b.map->map[pos]==-1 || fv->b.sf->glyphs[fv->b.map->map[pos]]==NULL ))
return;
    SCCharInfo(SFMakeChar(fv->b.sf,fv->b.map,pos),fv->b.map,pos);
}

void FontViewMenu_BDFInfo(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    if ( fv->b.sf->bitmaps==NULL )
return;
    if ( fv->show!=fv->filled )
	SFBdfProperties(fv->b.sf,fv->b.map,fv->show);
    else
	SFBdfProperties(fv->b.sf,fv->b.map,NULL);
}

void FontViewMenu_GlyphRename(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVMassGlyphRename(fv);
}

void FontViewMenu_ShowDependentRefs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int pos = FVAnyCharSelected(fv);
    SplineChar *sc;

    if ( pos<0 || fv->b.map->map[pos]==-1 )
return;
    sc = fv->b.sf->glyphs[fv->b.map->map[pos]];
    if ( sc==NULL || sc->dependents==NULL )
return;
    SCRefBy(sc);
}

void FontViewMenu_ShowDependentSubs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int pos = FVAnyCharSelected(fv);
    SplineChar *sc;

    if ( pos<0 || fv->b.map->map[pos]==-1 )
return;
    sc = fv->b.sf->glyphs[fv->b.map->map[pos]];
    if ( sc==NULL )
return;
    SCSubBy(sc);
}

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

static void FVDoTransform(FontView *fv) {
    int flags=0x3;
    if ( FVAnyCharSelected(fv)==-1 )
return;
    if ( FVAllSelected(fv))
	flags = 0x7;
    TransformDlgCreate(fv,FVTransFunc,getorigin,flags,cvt_none);
}

void FontViewMenu_Transform(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVDoTransform(fv);
}

void FontViewMenu_PointOfView(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    struct pov_data pov_data;
    if ( FVAnyCharSelected(fv)==-1 || fv->b.sf->onlybitmaps )
return;
    if ( PointOfViewDlg(&pov_data,fv->b.sf,false)==-1 )
return;
    FVPointOfView((FontViewBase *) fv,&pov_data);
}

void FontViewMenu_NonLinearTransform(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    if ( FVAnyCharSelected(fv)==-1 || fv->b.sf->onlybitmaps )
return;
    NonLinearDlg(fv,NULL);
}

void FontViewMenu_BitmapsAvail(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    BitmapDlg(fv,NULL,true );
}

void FontViewMenu_RegenBitmaps(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    BitmapDlg(fv,NULL,false );
}

void FontViewMenu_RemoveBitmapGlyphs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    BitmapDlg(fv,NULL,-1 );
}

void FontViewMenu_ExpandStroke(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    FVStroke(fv);
}

void FontViewMenu_TilePath(GtkMenuItem *menuitem, gpointer user_data) {
#  ifdef FONTFORGE_CONFIG_TILEPATH
    FontView *fv = FV_From_MI(menuitem);
    FVTile(fv);
#  endif
}

void FontViewMenu_RemoveOverlap(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    /* We know it's more likely that we'll find a problem in the overlap code */
    /*  than anywhere else, so let's save the current state against a crash */
    DoAutoSaves();

    FVOverlap((FontViewBase *) fv,over_remove);
}

void FontViewMenu_Intersect(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    DoAutoSaves();
    FVOverlap((FontViewBase *) fv,over_intersect);
}

void FontViewMenu_FindInter(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVOverlap((FontViewBase *) fv,over_findinter);
}

void FontViewMenu_Inline(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    OutlineDlg(fv,NULL,NULL,true);
}

void FontViewMenu_Outline(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    OutlineDlg(fv,NULL,NULL,false);
}

void FontViewMenu_Shadow(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    ShadowDlg(fv,NULL,NULL,false);
}

void FontViewMenu_WireFrame(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    ShadowDlg(fv,NULL,NULL,true);
}

static void FVSimplify(FontView *fv,int type) {
    static struct simplifyinfo smpls[] = {
	    { sf_normal },
	    { sf_normal,.75,.05,0,-1 },
	    { sf_normal,.75,.05,0,-1 }};
    struct simplifyinfo *smpl = &smpls[type+1];

    if ( smpl->linelenmax==-1 || (type==0 && !smpl->set_as_default)) {
	smpl->err = (fv->b.sf->ascent+fv->b.sf->descent)/1000.;
	smpl->linelenmax = (fv->b.sf->ascent+fv->b.sf->descent)/100.;
    }

    if ( type==1 ) {
	if ( !SimplifyDlg(fv->b.sf,smpl))
return;
	if ( smpl->set_as_default )
	    smpls[1] = *smpl;
    }
    _FVSimplify((FontViewBase *) fv,smpl);
}

void FontViewMenu_Simplify(GtkMenuItem *menuitem, gpointer user_data) {
    FVSimplify( (FontView *) FV_From_MI(menuitem), false);
}

void FontViewMenu_SimplifyMore(GtkMenuItem *menuitem, gpointer user_data) {
    FVSimplify( (FontView *) FV_From_MI(menuitem), true);
}

void FontViewMenu_Cleanup(GtkMenuItem *menuitem, gpointer user_data) {
    FVSimplify( (FontView *) FV_From_MI(menuitem), -1);
}

void FontViewMenu_CanonicalStart(GtkMenuItem *menuitem, gpointer user_data) {
    FVCanonicalStart( (FontViewBase *) FV_From_MI(menuitem));
}

void FontViewMenu_CanonicalContours(GtkMenuItem *menuitem, gpointer user_data) {
    FVCanonicalContours( (FontViewBase *) FV_From_MI(menuitem));
}

void FontViewMenu_AddExtrema(GtkMenuItem *menuitem, gpointer user_data) {
    FVAddExtrema( (FontViewBase *) FV_From_MI(menuitem) , false);
}

void FontViewMenu_CorrectDir(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVCorrectDir((FontViewBase *) fv);
}

void FontViewMenu_RoundToInt(GtkMenuItem *menuitem, gpointer user_data) {
    FVRound2Int( (FontViewBase *) FV_From_MI(menuitem), 1.0);
}

void FontViewMenu_RoundToHundredths(GtkMenuItem *menuitem, gpointer user_data) {
    FVRound2Int( (FontViewBase *) FV_From_MI(menuitem),100.0);
}

void FontViewMenu_Cluster(GtkMenuItem *menuitem, gpointer user_data) {
    FVCluster( (FontViewBase *) FV_From_MI(menuitem));
}

void FontViewMenu_Autotrace(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    if ( fv->v!=NULL )
	gdk_window_set_cursor(fv->v->window,ct_watch);
    FVAutoTrace((FontViewBase *) fv,false);
    if ( fv->v!=NULL )
	gdk_window_set_cursor(fv->v->window,ct_pointer);
}

void FontViewMenu_BuildAccented(GtkMenuItem *menuitem, gpointer user_data) {
    FVBuildAccent( (FontViewBase *) FV_From_MI(menuitem), true );
}

void FontViewMenu_BuildComposite(GtkMenuItem *menuitem, gpointer user_data) {
    FVBuildAccent( (FontViewBase *) FV_From_MI(menuitem), false );
}

#ifdef KOREAN
void FontViewMenu_ShowGroup(GtkMenuItem *menuitem, gpointer user_data) {
    ShowGroup( ((FontView *) FV_From_MI(menuitem))->b.sf );
}
#endif

#if HANYANG
static void FontViewMenu_ModifyComposition(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    if ( fv->b.sf->rules!=NULL )
	SFModifyComposition(fv->b.sf);
}

static void FontViewMenu_BuildSyllables(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    if ( fv->b.sf->rules!=NULL )
	SFBuildSyllables(fv->b.sf);
}
#endif

void FontViewMenu_CompareFonts(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FontCompareDlg(fv);
}

void FontViewMenu_MergeFonts(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVMergeFonts(fv);
}

void FontViewMenu_Interpolate(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVInterpolateFonts(fv);
}

static void FVShowInfo(FontView *fv) {
    if ( fv->v==NULL )			/* Can happen in scripts */
return;

    gtk_widget_queue_draw(fv->status);
}

void FVChangeChar(FontView *fv,int i) {

    if ( i!=-1 ) {
	FVDeselectAll(fv);
	fv->b.selected[i] = true;
	fv->sel_index = 1;
	fv->end_pos = fv->pressed_pos = i;
	FVScrollToChar(fv,i);
	FVShowInfo(fv);
    }
}

void FVScrollToChar(FontView *fv,int i) {
    GtkAdjustment *sb;

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
	    sb = gtk_range_get_adjustment(GTK_RANGE(fv->vsb));
	    sb->value = fv->rowoff;
	    gtk_range_set_adjustment(GTK_RANGE(fv->vsb),sb);
	    gtk_widget_queue_draw(fv->v);
	}
    }
}

static void FVScrollToGID(FontView *fv,int gid) {
    FVScrollToChar(fv,fv->b.map->backmap[gid]);
}

static void FV_ChangeGID(FontView *fv,int gid) {
    FVChangeChar(fv,fv->b.map->backmap[gid]);
}

void FontViewMenu_ChangeChar(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SplineFont *sf = fv->b.sf;
    EncMap *map = fv->b.map;
    int pos = FVAnyCharSelected(fv);

    if ( pos>=0 ) {
	if ( strcmp(gtk_widget_get_name(GTK_WIDGET(menuitem)),"next_glyph1")==0 )
	    ++pos;
	if ( strcmp(gtk_widget_get_name(GTK_WIDGET(menuitem)),"prev_glyph1")==0 )
	    --pos;
	else if ( strcmp(gtk_widget_get_name(GTK_WIDGET(menuitem)),"next_defined_glyph1")==0 ) {
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
	} else if ( strcmp(gtk_widget_get_name(GTK_WIDGET(menuitem)),"prev_defined_glyph1")==0 ) {
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

static void FVShowSubFont(FontView *fv,SplineFont *new) {
    MetricsView *mv, *mvnext;
    BDFFont *newbdf;
    int wascompact = fv->b.normal!=NULL;
    extern int use_freetype_to_rasterize_fv;

    for ( mv=fv->b.sf->metrics; mv!=NULL; mv = mvnext ) {
	/* Don't bother trying to fix up metrics views, just not worth it */
	mvnext = mv->next;
	gtk_widget_destroy(mv->gw);
    }
    if ( wascompact ) {
	EncMapFree(fv->b.map);
	fv->b.map = fv->b.normal;
	fv->b.normal = NULL;
	fv->b.selected = realloc(fv->b.selected,fv->b.map->enccount);
	memset(fv->b.selected,0,fv->b.map->enccount);
    }
    CIDSetEncMap((FontViewBase *) fv,new);
    if ( wascompact ) {
	fv->b.normal = EncMapCopy(fv->b.map);
	CompactEncMap(fv->b.map,fv->b.sf);
	FontViewReformatOne(&fv->b);
	FVSetTitle( (FontViewBase *) &fv->b);
    }
    newbdf = SplineFontPieceMeal(fv->b.sf,fv->b.active_layer,fv->filled->pixelsize,72,
	    (fv->antialias?pf_antialias:0)|(fv->bbsized?pf_bbsized:0)|
		(use_freetype_to_rasterize_fv && !fv->b.sf->strokedfont && !fv->b.sf->multilayer?pf_ft_nohints:0),
	    NULL);
    BDFFontFree(fv->filled);
    if ( fv->filled == fv->show )
	fv->show = newbdf;
    fv->filled = newbdf;
    gtk_widget_queue_draw(fv->v);
}

void FontViewMenu_Goto(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int pos = GotoChar(fv->b.sf,fv->b.map);
    if ( fv->b.cidmaster!=NULL && pos!=-1 && !fv->b.map->enc->is_compact ) {
	SplineFont *cidmaster = fv->b.cidmaster;
	int k, hadk= cidmaster->subfontcnt;
	for ( k=0; k<cidmaster->subfontcnt; ++k ) {
	    SplineFont *sf = cidmaster->subfonts[k];
	    if ( pos<sf->glyphcnt && sf->glyphs[pos]!=NULL )
	break;
	    if ( pos<sf->glyphcnt )
		hadk = k;
	}
	if ( k==cidmaster->subfontcnt && pos>=fv->b.sf->glyphcnt )
	    k = hadk;
	if ( k!=cidmaster->subfontcnt && cidmaster->subfonts[k] != fv->b.sf )
	    FVShowSubFont(fv,cidmaster->subfonts[k]);
	if ( pos>=fv->b.sf->glyphcnt )
	    pos = -1;
    }
    FVChangeChar(fv,pos);
}

void FontViewMenu_Ligatures(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SFShowLigatures(fv->b.sf,NULL);
}

void FontViewMenu_KernPairs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SFKernClassTempDecompose(fv->b.sf,false);
    SFShowKernPairs(fv->b.sf,NULL,NULL);
    SFKernCleanup(fv->b.sf,false);
}

static void FontViewMenu_AnchoredPairs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SFShowKernPairs(fv->b.sf,NULL,user_data);
}

void FontViewMenu_ShowAtt(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    ShowAtt(fv->b.sf);
}

void FontViewMenu_DisplaySubstitutions(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    if ( fv->cur_subtable!=0 ) {
	fv->cur_subtable = NULL;
    } else {
	SplineFont *sf = fv->b.sf;
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
		names = malloc((cnt+3) * sizeof(char *));
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
    gtk_widget_queue_draw(fv->v);
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
	fv->b.active_bitmap = bdf==fv->filled ? NULL : bdf;
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
	ccnt = fv->b.sf->desired_col_cnt;
	rcnt = fv->b.sf->desired_row_cnt;
	if ((( bdf->pixelsize<=fv->b.sf->display_size || bdf->pixelsize<=-fv->b.sf->display_size ) &&
		 fv->b.sf->top_enc!=-1 /* Not defaulting */ ) ||
		bdf->pixelsize<=48 ) {
	    /* use the desired sizes */
	} else {
	    if ( bdf->pixelsize>48 ) {
		ccnt = 8;
		rcnt = 2;
	    }
	    if ( !first_time ) {
		if ( ccnt < oldc/fv->cbw )
		    ccnt = oldc/fv->cbw;
		if ( rcnt < oldr/fv->cbh )
		    rcnt = oldr/fv->cbh;
	    }
	}
	if ( samesize ) {
	    gtk_widget_queue_draw(fv->v);
	} else {
	    gtk_widget_set_size_request(fv->v,
		    ccnt*fv->cbw+1,
		    rcnt*fv->cbh+1);
	    /* If we try to make the window smaller, we must force a resize */
	    /*  of the whole system, else the size will remain the same */
	    /* Also when we get a resize event we must set the widget size */
	    /*  to be minimal so that the user can resize */
	    if (( ccnt*fv->cbw < oldc || rcnt*fv->cbh < oldr ) && !first_time )
		gtk_window_resize( GTK_WINDOW(fv->v), 1,1 );
	    if ( fv->char_slot!=NULL )
		g_object_unref(G_OBJECT(fv->char_slot));
	    fv->char_slot=NULL;
	}
    }
    if ( fv->char_slot==NULL )
	fv->char_slot = gdk_pixbuf_new(GDK_COLORSPACE_RGB,true,8,fv->cbw-1,fv->cbh-1);
}

void FontViewMenu_ShowMetrics(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    GtkWidget *dlg = create_FontViewShowMetrics();
    int isv = strcmp(gtk_widget_get_name(GTK_WIDGET(menuitem)),"show_v_metrics1")==0;
    int metrics = !isv ? fv->showhmetrics : fv->showvmetrics;

    if ( isv )
	gtk_window_set_title(GTK_WINDOW(dlg), _("Show Vertical Metrics"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(dlg,"baseline")),
	    (metrics&fvm_baseline)?true:false);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(dlg,"origin")),
	    (metrics&fvm_origin)?true:false);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(dlg,"AWBar")),
	    (metrics&fvm_advanceto)?true:false);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(dlg,"AWLine")),
	    (metrics&fvm_advanceat)?true:false);
    if ( gtk_dialog_run( GTK_DIALOG (dlg)) == GTK_RESPONSE_ACCEPT ) {
	metrics = 0;
	if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(dlg,"baseline"))) )
	    metrics |= fvm_baseline;
	if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(dlg,"origin"))) )
	    metrics |= fvm_origin;
	if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(dlg,"AWBar"))) )
	    metrics |= fvm_advanceto;
	if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(dlg,"AWLine"))) )
	    metrics |= fvm_advanceat;
	if ( isv )
	    fv->showvmetrics = metrics;
	else
	    fv->showhmetrics = metrics;
    }
    gtk_widget_destroy( dlg );
}

static void FV_ChangeDisplayBitmap(FontView *fv,BDFFont *bdf) {
    FVChangeDisplayFont(fv,bdf);
    fv->b.sf->display_size = fv->show->pixelsize;
}

void FontViewMenu_PixelSize(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem), *fvs, *fvss;
    int dspsize = fv->filled->pixelsize;
    int changedmodifier = false;
    G_CONST_RETURN gchar *name = gtk_widget_get_name(GTK_WIDGET(menuitem));
    extern int use_freetype_to_rasterize_fv;

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
	default_fv_bbsized = fv->bbsized = !fv->bbsized;
	fv->b.sf->display_bbsized = fv->bbsized;
	changedmodifier = true;
    } else {
	default_fv_antialias = fv->antialias = !fv->antialias;
	fv->b.sf->display_antialias = fv->antialias;
	changedmodifier = true;
    }

    SavePrefs(true);
    if ( fv->filled!=fv->show || fv->filled->pixelsize != dspsize || changedmodifier ) {
	BDFFont *new, *old;
	for ( fvs=(FontView *) fv->b.sf->fv; fvs!=NULL; fvs=(FontView *) fvs->b.nextsame )
	    fvs->touched = false;
	while ( 1 ) {
	    for ( fvs=(FontView *) fv->b.sf->fv; fvs!=NULL; fvs=(FontView *) fvs->b.nextsame )
		if ( !fvs->touched )
	    break;
	    if ( fvs==NULL )
	break;
	    old = fvs->filled;
	    new = SplineFontPieceMeal(fvs->b.sf,fv->b.active_layer,dspsize,72,
		(fvs->antialias?pf_antialias:0)|(fvs->bbsized?pf_bbsized:0)|
		    (use_freetype_to_rasterize_fv && !fvs->b.sf->strokedfont && !fvs->b.sf->multilayer?pf_ft_nohints:0),
		NULL);
	    for ( fvss=fvs; fvss!=NULL; fvss = (FontView *) fvss->b.nextsame ) {
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
	fv->b.sf->display_size = -dspsize;
	if ( fv->b.cidmaster!=NULL ) {
	    int i;
	    for ( i=0; i<fv->b.cidmaster->subfontcnt; ++i )
		fv->b.cidmaster->subfonts[i]->display_size = -dspsize;
	}
    }
}

static void FV_LayerChanged( FontView *fv ) {
    extern int use_freetype_to_rasterize_fv;
    BDFFont *new, *old;

    fv->magnify = 1;
    fv->user_requested_magnify = -1;

    old = fv->filled;
    new = SplineFontPieceMeal(fv->b.sf,fv->b.active_layer,fv->filled->pixelsize,72,
	(fv->antialias?pf_antialias:0)|(fv->bbsized?pf_bbsized:0)|
	    (use_freetype_to_rasterize_fv && !fv->b.sf->strokedfont && !fv->b.sf->multilayer?pf_ft_nohints:0),
	NULL);
    fv->filled = new;
    FVChangeDisplayFont(fv,new);
    fv->b.sf->display_size = -fv->filled->pixelsize;
    BDFFontFree(old);
}

void FontViewMenu_BitmapMagnification(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
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
	fv->b.active_bitmap = NULL;
	FVChangeDisplayFont(fv,show);
    }
    free(ret);
}

void FontViewMenu_WindowSize(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    G_CONST_RETURN gchar *name = gtk_widget_get_name(GTK_WIDGET(menuitem));
    int h=8,v=2;
    extern int default_fv_col_count, default_fv_row_count;

    sscanf(name,"%dx%d", &h, &v );
    gtk_widget_set_size_request(fv->v,h*fv->cbw+1,v*fv->cbh+1);
    fv->b.sf->desired_col_cnt = default_fv_col_count = h;
    fv->b.sf->desired_row_cnt = default_fv_row_count = v;

    SavePrefs(true);
}

void FontViewMenu_GlyphLabel(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    G_CONST_RETURN gchar *name = gtk_widget_get_name(GTK_WIDGET(menuitem));

    if ( strstr(name,"glyph_image")!=NULL )
	default_fv_glyphlabel = fv->glyphlabel = gl_glyph;
    else if ( strstr(name,"glyph_name")!=NULL )
	default_fv_glyphlabel = fv->glyphlabel = gl_name;
    else if ( strstr(name,"unicode")!=NULL )
	default_fv_glyphlabel = fv->glyphlabel = gl_unicode;
    else if ( strstr(name,"encoding")!=NULL )
	default_fv_glyphlabel = fv->glyphlabel = gl_encoding;

    gtk_widget_queue_draw(fv->v);

    SavePrefs(true);
}

static void FontViewMenu_ShowBitmap(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    BDFFont *bdf = user_data;

    FV_ChangeDisplayBitmap(fv,bdf);		/* Let's not change any of the others */
}

static void FV_ShowFilled(FontView *fv) {

    fv->magnify = 1;
    fv->user_requested_magnify = 1;
    if ( fv->show!=fv->filled )
	FVChangeDisplayFont(fv,fv->filled);
    fv->b.sf->display_size = -fv->filled->pixelsize;
    fv->b.active_bitmap = NULL;
}

void FontViewMenu_CenterWidth(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVMetricsCenter((FontViewBase *) fv,true);
}

void FontViewMenu_ThirdsWidth(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVMetricsCenter((FontViewBase *) fv,false);
}

void FontViewMenu_SetWidth(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    G_CONST_RETURN gchar *name = gtk_widget_get_name(GTK_WIDGET(menuitem));

    if ( FVAnyCharSelected(fv)==-1 )
return;
    if ( strcmp(name,"set_vertical_advance1")==0 && !fv->b.sf->hasvmetrics )
return;
    FVSetWidth(fv,strcmp(name,"set_width1")==0   ?wt_width:
		  strcmp(name,"set_lbearing1")==0?wt_lbearing:
		  strcmp(name,"set_rbearing1")==0?wt_rbearing:
		  wt_vwidth);
}

void FontViewMenu_AutoWidth(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVAutoWidth(fv);
}

void FontViewMenu_AutoKern(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVAutoKern(fv);
}

void FontViewMenu_KernClasses(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    ShowKernClasses(fv->b.sf,NULL,false);
}

void FontViewMenu_VKernClasses(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    ShowKernClasses(fv->b.sf,NULL,true);
}

void FontViewMenu_RemoveKP(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVRemoveKerns((FontViewBase *) fv);
}

void FontViewMenu_RemoveVKP(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVRemoveVKerns( (FontViewBase *) fv);
}

void FontViewMenu_KernPairCloseup(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int i;

    for ( i=0; i<fv->b.map->enccount; ++i )
	if ( fv->b.selected[i] )
    break;
    KernPairD(fv->b.sf,i==fv->b.map->enccount?NULL:
		    fv->b.map->map[i]==-1?NULL:
		    fv->b.sf->glyphs[fv->b.map->map[i]],NULL,false);
}

void FontViewMenu_VKernFromH(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVVKernFromHKern( (FontViewBase *) fv);
}

void FontViewMenu_AutoHint(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVAutoHint(  (FontViewBase *) fv );
}

void FontViewMenu_AutoHintSubsPts(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVAutoHintSubs( (FontViewBase *) fv );
}

void FontViewMenu_AutoCounter(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVAutoCounter( (FontViewBase *) fv );
}

void FontViewMenu_DontAutoHint(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVDontAutoHint( (FontViewBase *) fv );
}

void FontViewMenu_AutoInstr(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    FVAutoInstr( (FontViewBase *) fv );
}

void FontViewMenu_EditInstrs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int index = FVAnyCharSelected(fv);
    SplineChar *sc;
    if ( index<0 )
return;
    sc = SFMakeChar(fv->b.sf,fv->b.map,index);
    SCEditInstructions(sc);
}

void FontViewMenu_EditTable(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    G_CONST_RETURN gchar *name = gtk_widget_get_name(GTK_WIDGET(menuitem));
    SFEditTable(fv->b.sf,
	    strcmp(name,"edit_prep1")==0?CHR('p','r','e','p'):
	    strcmp(name,"edit_fpgm1")==0?CHR('f','p','g','m'):
	    strcmp(name,"edit_maxp1")==0?CHR('m','a','x','p'):
				  CHR('c','v','t',' '));
}

void FontViewMenu_ClearInstrs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVClearInstrs((FontViewBase *) fv);
}

void FontViewMenu_ClearHints(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVClearHints((FontViewBase *) fv);
}

void FontViewMenu_Histograms(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    G_CONST_RETURN gchar *name = gtk_widget_get_name(GTK_WIDGET(menuitem));
    SFHistogram(fv->b.sf, NULL, FVAnyCharSelected(fv)!=-1?fv->b.selected:NULL,
			fv->b.map,
			strcmp(name,"hstem1")==0 ? hist_hstem :
			strcmp(name,"vstem1")==0 ? hist_vstem :
				hist_blues);
}

static void FontViewSetTitle(FontView *fv) {
    char *title;
    char *file=NULL;
    char *enc;
    int len;
    gsize read, written;

    if ( fv->gw==NULL )		/* In scripting */
return;

    enc = SFEncodingName(fv->b.sf,fv->b.map);
    len = strlen(fv->b.sf->fontname)+1 + strlen(enc)+4;
    if ( fv->b.cidmaster!=NULL ) {
	if ( (file = fv->b.cidmaster->filename)==NULL )
	    file = fv->b.cidmaster->origname;
    } else {
	if ( (file = fv->b.sf->filename)==NULL )
	    file = fv->b.sf->origname;
    }
    if ( file!=NULL )
	len += 2+strlen(file);
    title = malloc((len+1));
    strcpy(title,fv->b.sf->fontname);
    if ( fv->b.sf->changed )
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

    if ( GTK_WIDGET(fv->gw)->window!=NULL )
	gdk_window_set_icon_name( GTK_WIDGET(fv->gw)->window, fv->b.sf->fontname );
    gtk_window_set_title( GTK_WINDOW(fv->gw), title );
    free(title);
}

static void FontViewSetTitles(SplineFont *sf) {
    FontView *fv;

    for ( fv = (FontView *) (sf->fv); fv!=NULL; fv=(FontView *) (fv->b.nextsame))
	FontViewSetTitle(fv);
}

static void FontViewMenu_ShowSubFont(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SplineFont *new = user_data;
    FVShowSubFont(fv,new);
}

void FontViewMenu_Convert2CID(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SplineFont *cidmaster = fv->b.cidmaster;
    struct cidmap *cidmap;

    if ( cidmaster!=NULL )
return;
    SFFindNearTop(fv->b.sf);
    cidmap = AskUserForCIDMap();
    if ( cidmap==NULL )
return;
    MakeCIDMaster(fv->b.sf,fv->b.map,false,NULL,cidmap);
    SFRestoreNearTop(fv->b.sf);
}

static gboolean CMapFilter(const GtkFileFilterInfo *filter_info,gpointer data) {
    const char *filename = filter_info->filename;
    char buf2[256];
    int ret = true;
    FILE *file;
    static char *cmapflag = "%!PS-Adobe-3.0 Resource-CMap";
    char *pt;

    pt = strrchr(filename,'.');
    if ( pt==NULL )
return( false );
    if ( strcasecmp(pt,".cmap")!=0 )
return( false );
    file = fopen(filename,"r");
    if ( file==NULL )
return( false );
    else {
	if ( fgets(buf2,sizeof(buf2),file)==NULL ||
		strncmp(buf2,cmapflag,strlen(cmapflag))!=0 )
	    ret = false;
	fclose(file);
    }
return( ret );
}

static struct gwwv_filter cmap_filters[] = {
    { N_("Adobe CMap files"), NULL, CMapFilter },
    { N_("All"), "*" },
    NULL
};

void FontViewMenu_ConvertByCMap(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SplineFont *cidmaster = fv->b.cidmaster;
    char *cmapfilename;

    if ( cidmaster!=NULL )
return;
    cmapfilename = gwwv_open_filename_mult(_("Find an adobe CMap file..."),NULL,cmap_filters,false);
    if ( cmapfilename==NULL )
return;
    MakeCIDMaster(fv->b.sf,fv->b.map,true,cmapfilename,NULL);
    free(cmapfilename);
}

void FontViewMenu_Flatten(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SplineFont *cidmaster = fv->b.cidmaster;

    if ( cidmaster==NULL )
return;
    SFFlatten(&cidmaster);
}

void FontViewMenu_FlattenByCMap(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SplineFont *cidmaster = fv->b.cidmaster;
    char *cmapname;

    if ( cidmaster==NULL )
return;
    cmapname = gwwv_open_filename_mult(_("Find an adobe CMap file..."),NULL,cmap_filters,false);
    if ( cmapname==NULL )
return;
    SFFindNearTop(fv->b.sf);
    SFFlattenByCMap(&cidmaster,cmapname);
    SFRestoreNearTop(fv->b.sf);
    free(cmapname);
}

void FontViewMenu_CIDInsertFont(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SplineFont *cidmaster = fv->b.cidmaster;
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
    if ( new->fv == (FontViewBase *) fv )		/* Already part of us */
return;
    if ( new->fv != NULL ) {
	if ( ((FontView *) new->fv)->gw!=NULL )
	    gdk_window_show(GDK_WINDOW( ((FontView *) new->fv)->gw->window ));
	gwwv_post_error(_("Please close font"),_("Please close %s before inserting it into a CID font"),new->origname);
return;
    }
    EncMapFree(new->map);
    if ( force_names_when_opening!=NULL )
	SFRenameGlyphsToNamelist(new,force_names_when_opening );

    map = FindCidMap(cidmaster->cidregistry,cidmaster->ordering,cidmaster->supplement,cidmaster);
    SFEncodeToMap(new,map);
    if ( !PSDictHasEntry(new->private,"lenIV"))
	PSDictChangeEntry(new->private,"lenIV","1");		/* It's 4 by default, in CIDs the convention seems to be 1 */
    new->display_antialias = fv->b.sf->display_antialias;
    new->display_bbsized = fv->b.sf->display_bbsized;
    new->display_size = fv->b.sf->display_size;
    FVInsertInCID((FontViewBase *) fv,new);
    CIDMasterAsDes(new);
}

void FontViewMenu_CIDInsertEmptyFont(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SplineFont *cidmaster = fv->b.cidmaster, *sf;
    struct cidmap *map;

    if ( cidmaster==NULL || cidmaster->subfontcnt>=255 )	/* Open type allows 1 byte to specify the fdselect */
return;
    map = FindCidMap(cidmaster->cidregistry,cidmaster->ordering,cidmaster->supplement,cidmaster);
    sf = SplineFontBlank(MaxCID(map));
    sf->glyphcnt = sf->glyphmax;
    sf->cidmaster = cidmaster;
    sf->display_antialias = fv->b.sf->display_antialias;
    sf->display_bbsized = fv->b.sf->display_bbsized;
    sf->display_size = fv->b.sf->display_size;
    sf->private = calloc(1,sizeof(struct psdict));
    PSDictChangeEntry(sf->private,"lenIV","1");		/* It's 4 by default, in CIDs the convention seems to be 1 */
    FVInsertInCID((FontViewBase *) fv,sf);
}

void FontViewMenu_RemoveFontFromCID(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    static char *buts[] = { GTK_STOCK_REMOVE, GTK_STOCK_CANCEL, NULL };
    SplineFont *cidmaster = fv->b.cidmaster, *sf = fv->b.sf, *replace;
    int i;
    MetricsView *mv, *mnext;
    FontView *fvs;

    if ( cidmaster==NULL || cidmaster->subfontcnt<=1 )	/* Can't remove last font */
return;
    if ( gwwv_ask(_("_Remove Font"),(const char **) buts,0,1,_("Are you sure you wish to remove sub-font %1$.40s from the CID font %2$.40s"),
	    sf->fontname,cidmaster->fontname)==1 )
return;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	CharView *cv, *next;
	for ( cv = (CharView *) sf->glyphs[i]->views; cv!=NULL; cv = next ) {
	    next = (CharView *) cv->b.next;
	    gtk_widget_destroy(cv->gw);
	}
    }
    for ( mv=fv->b.sf->metrics; mv!=NULL; mv = mnext ) {
	mnext = mv->next;
	gtk_widget_destroy(mv->gw);
    }
    g_main_context_iteration(NULL,TRUE);	/* Waits until all destroy windows are processed (I think) */

    for ( i=0; i<cidmaster->subfontcnt; ++i )
	if ( cidmaster->subfonts[i]==sf )
    break;
    replace = i==0?cidmaster->subfonts[1]:cidmaster->subfonts[i-1];
    while ( i<cidmaster->subfontcnt-1 ) {
	cidmaster->subfonts[i] = cidmaster->subfonts[i+1];
	++i;
    }
    --cidmaster->subfontcnt;

    for ( fvs=(FontView *) fv->b.sf->fv; fvs!=NULL; fvs=(FontView *) fvs->b.nextsame ) {
	if ( fvs->b.sf==sf )
	    CIDSetEncMap((FontViewBase *) fvs,replace);
    }
    FontViewReformatAll(fv->b.sf);
    SplineFontFree(sf);
}

void FontViewMenu_CIDFontInfo(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SplineFont *cidmaster = fv->b.cidmaster;

    if ( cidmaster==NULL )
return;
    FontInfo(cidmaster,-1,false);
}

void FontViewMenu_ChangeSupplement(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SplineFont *cidmaster = fv->b.cidmaster;
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
	FVSetTitle( (FontViewBase *) fv);
    }
}

static SplineChar *FVFindACharInDisplay(FontView *fv) {
    int start, end, enc, gid;
    EncMap *map = fv->b.map;
    SplineFont *sf = fv->b.sf;
    SplineChar *sc;

    start = fv->rowoff*fv->colcnt;
    end = start + fv->rowcnt*fv->colcnt;
    for ( enc = start; enc<end && enc<map->enccount; ++enc ) {
	if ( (gid=map->map[enc])!=-1 && (sc=sf->glyphs[gid])!=NULL )
return( sc );
    }
return( NULL );
}

static void FontViewMenu_Reencode(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    Encoding *enc = NULL;
    EncMap *map;
    SplineChar *sc;

    gwwv_post_notice("NYI", "Reencoding is not yet implemented");
    /* !!!!! */

    sc = FVFindACharInDisplay(fv);
    enc = FindOrMakeEncoding(user_data);
    if ( enc==&custom )
	fv->b.map->enc = &custom;
    else {
	map = EncMapFromEncoding(fv->b.sf,enc);
	fv->b.selected = realloc(fv->b.selected,map->enccount);
	memset(fv->b.selected,0,map->enccount);
	EncMapFree(fv->b.map);
	fv->b.map = map;
    }
    if ( fv->b.normal!=NULL ) {
	EncMapFree(fv->b.normal);
	fv->b.normal = NULL;
    }
    SFReplaceEncodingBDFProps(fv->b.sf,fv->b.map);
    FontViewSetTitle(fv);
    FontViewReformatOne(&fv->b);
    if ( sc!=NULL ) {
	int enc = fv->b.map->backmap[sc->orig_pos];
	if ( enc!=-1 )
	    FVScrollToChar(fv,enc);
    }
}

void FontViewMenu_BuildReencode(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    GetEncodingMenu(menuitem,FontViewMenu_Reencode,fv->b.map->enc);
}

static void FontViewMenu_ForceEncode(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    Encoding *enc = NULL;
    int oldcnt = fv->b.map->enccount;

    gwwv_post_notice("NYI", "Forced Reencoding is not yet implemented");
return;
    /*!!!!!!! */

    enc = FindOrMakeEncoding(user_data);
    SFForceEncoding(fv->b.sf,fv->b.map,enc);
    if ( oldcnt < fv->b.map->enccount ) {
	fv->b.selected = realloc(fv->b.selected,fv->b.map->enccount);
	memset(fv->b.selected+oldcnt,0,fv->b.map->enccount-oldcnt);
    }
    if ( fv->b.normal!=NULL ) {
	EncMapFree(fv->b.normal);
	fv->b.normal = NULL;
    }
    SFReplaceEncodingBDFProps(fv->b.sf,fv->b.map);
    FontViewSetTitle(fv);
    FontViewReformatOne(&fv->b);
}

void FontViewMenu_BuildForceEncoding(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    GetEncodingMenu(menuitem,FontViewMenu_ForceEncode,fv->b.map->enc);
}

void FontViewMenu_DisplayByGroups(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    DisplayGroups(fv);
}

void FontViewMenu_DefineGroups(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    DefineGroups(fv);
}

void FontViewMenu_MMValid(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    MMSet *mm = fv->b.sf->mm;

    if ( mm==NULL )
return;
    MMValid(mm,true);
}

void FontViewMenu_CreateMM(GtkMenuItem *menuitem, gpointer user_data) {
    MMWizard(NULL);
}

void FontViewMenu_MMInfo(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    MMSet *mm = fv->b.sf->mm;

    if ( mm==NULL )
return;
    MMWizard(mm);
}

void FontViewMenu_ChangeDefWeights(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    MMSet *mm = fv->b.sf->mm;

    if ( mm==NULL || mm->apple )
return;
    MMChangeBlend(mm,fv,false);
}

void FontViewMenu_Blend(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    MMSet *mm = fv->b.sf->mm;

    if ( mm==NULL )
return;
    MMChangeBlend(mm,fv,true);
}

void FontViewMenu_AddEncodingSlots(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    char *ret, *end;
    int cnt;

    ret = gwwv_ask_string(_("Add Encoding Slots"),"1",fv->b.cidmaster?_("How many CID slots do you wish to add?"):_("How many unencoded glyph slots do you wish to add?"));
    if ( ret==NULL )
return;
    cnt = strtol(ret,&end,10);
    if ( *end!='\0' || cnt<=0 ) {
	free(ret);
	ff_post_error( _("Bad Number"),_("Bad Number") );
return;
    }
    free(ret);
    FVAddUnencoded((FontViewBase *) fv, cnt);
}

void FontViewMenu_RemoveUnusedSlots(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVRemoveUnused((FontViewBase *) fv);
}

void FontViewMenu_Compact(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SplineChar *sc;

    sc = FVFindACharInDisplay(fv);
    FVCompact((FontViewBase *) fv);
    if ( sc!=NULL ) {
	int enc = fv->b.map->backmap[sc->orig_pos];
	if ( enc!=-1 )
	    FVScrollToChar(fv,enc);
    }
}

void FontViewMenu_DetachGlyphs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    FVDetachGlyphs((FontViewBase *) fv);
}

void FontViewMenu_DetachAndRemoveGlyphs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    static char *buts[] = { GTK_STOCK_REMOVE, GTK_STOCK_CANCEL, NULL };

    if ( gwwv_ask(_("Detach & Remove Glyphs"),(const char **) buts,0,1,_("Are you sure you wish to remove these glyphs? This operation cannot be undone."))==1 )
return;

    FVDetachAndRemoveGlyphs((FontViewBase *) fv);
}

void FontViewMenu_AddEncodingName(GtkMenuItem *menuitem, gpointer user_data) {
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

void FontViewMenu_LoadEncoding(GtkMenuItem *menuitem, gpointer user_data) {
    LoadEncodingFile();
}

void FontViewMenu_MakeFromFont(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    (void) MakeEncoding(fv->b.sf,fv->b.map);
}

void FontViewMenu_RemoveEncoding(GtkMenuItem *menuitem, gpointer user_data) {
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

void FontViewMenu_SaveNamelist(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    gsize read, written;
    char buffer[1025];
    char *filename, *temp;
    FILE *file;
    SplineChar *sc;
    int i;

    snprintf(buffer, sizeof(buffer),"%s/%s.nam", getFontForgeUserDir(Config), fv->b.sf->fontname );
    temp = g_filename_to_utf8(buffer,-1,&read,&written,NULL);
    filename = gwwv_saveas_filename(_("Save Namelist of font"), temp,"*.nam");
    free(temp);
    if ( filename==NULL )
return;
    temp = g_filename_from_utf8(filename,-1,&read,&written,NULL);
    file = fopen(temp,"w");
    free(temp);
    if ( file==NULL ) {
	gwwv_post_error(_("Namelist creation failed"),_("Could not write %s"), filename);
	free(filename);
return;
    }
    for ( i=0; i<fv->b.sf->glyphcnt; ++i ) {
	if ( (sc = fv->b.sf->glyphs[i])!=NULL && sc->unicodeenc!=-1 ) {
	    if ( !isuniname(sc->name) && !isuname(sc->name ) )
		fprintf( file, "0x%04X %s\n", sc->unicodeenc, sc->name );
	}
    }
    fclose(file);
}

void FontViewMenu_LoadNamelist(GtkMenuItem *menuitem, gpointer user_data) {
    gsize read, written;
    /* Read in a name list and copy it into the prefs dir so that we'll find */
    /*  it in the future */
    /* Be prepared to update what we've already got if names match */
    char buffer[1025];
    char *ret = gwwv_open_filename(_("Load Namelist"),NULL, "*.nam");
    char *temp, *pt;
    char *buts[3];
    FILE *old, *new;
    int ch, ans;
    NameList *nl;

    if ( ret==NULL )
return;				/* Cancelled */
    temp = g_filename_from_utf8(ret,-1,&read,&written,NULL);
    pt = strrchr(temp,'/');
    if ( pt==NULL )
	pt = temp;
    else
	++pt;
    snprintf(buffer,sizeof(buffer),"%s/%s", getFontForgeUserDir(Config), pt);
    if ( access(buffer,F_OK)==0 ) {
	buts[0] = _("_Replace");
	buts[1] = GTK_STOCK_CANCEL;
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
	free(ret); free(temp); fclose(old);
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
	fclose(old);
return;
    }

    while ( (ch=getc(old))!=EOF )
	putc(ch,new);
    fclose(old);
    fclose(new);
}

void FontViewMenu_RenameByNamelist(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
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
    SFRenameGlyphsToNamelist(fv->b.sf,nl);
    gtk_widget_queue_draw(fv->v);
}

void FontViewMenu_CreateNamedGlyphs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    gsize read, written;
    /* Read a file containing a list of names, and add an unencoded glyph for */
    /*  each name */
    char buffer[33];
    char *ret = gwwv_open_filename(_("Load glyph names"),NULL, "*");
    char *temp, *pt;
    FILE *file;
    int ch;
    SplineChar *sc;
    FontView *fvs;

    if ( ret==NULL )
return;				/* Cancelled */
    temp = g_filename_from_utf8(ret,-1,&read,&written,NULL);

    file = fopen( temp,"r");
    if ( file==NULL ) {
	gwwv_post_error(_("No such file"),_("Could not read %s"), ret );
	free(ret); free(temp);
return;
    }
    pt = buffer;
    for (;;) {
	ch = getc(file);
	if ( ch!=EOF && !isspace(ch)) {
	    if ( pt<buffer+sizeof(buffer)-1 )
		*pt++ = ch;
	} else {
	    if ( pt!=buffer ) {
		*pt = '\0';
		sc = NULL;
		for ( fvs=(FontView *) fv->b.sf->fv; fvs!=NULL; fvs=(FontView *) fvs->b.nextsame ) {
		    EncMap *map = fvs->b.map;
		    if ( map->enccount+1>=map->encmax )
			map->map = realloc(map->map,(map->encmax += 20)*sizeof(int));
		    map->map[map->enccount] = -1;
		    fvs->b.selected = realloc(fvs->b.selected,(map->enccount+1));
		    memset(fvs->b.selected+map->enccount,0,1);
		    ++map->enccount;
		    if ( sc==NULL ) {
			sc = SFMakeChar(fv->b.sf,map,map->enccount-1);
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
    FontViewReformatAll(fv->b.sf);
}

void FontViewMenu_ActivateCopyFrom(GtkMenuItem *menuitem, gpointer user_data) {
    GtkWidget *w;

    w = lookup_widget( GTK_WIDGET(menuitem), "all_fonts1" );
    gtk_widget_set_sensitive(w,!onlycopydisplayed);

    w = lookup_widget( GTK_WIDGET(menuitem), "displayed_fonts1" );
    gtk_widget_set_sensitive(w,onlycopydisplayed);

    w = lookup_widget( GTK_WIDGET(menuitem), "glyph_metadata1" );
    gtk_widget_set_sensitive(w,copymetadata);
}

void FontViewMenu_ActivateTransformations(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SplineFont *sf = fv->b.sf;
    int anychars = FVAnyCharSelected(fv);
    GtkWidget *w;

    w = lookup_widget( GTK_WIDGET(menuitem), "transform1" );
    gtk_widget_set_sensitive(w,anychars);

    w = lookup_widget( GTK_WIDGET(menuitem), "point_of_view_projection1" );
    gtk_widget_set_sensitive(w,anychars && !sf->onlybitmaps );

    w = lookup_widget( GTK_WIDGET(menuitem), "non_linear_transform1" );
    gtk_widget_set_sensitive(w,anychars && !sf->onlybitmaps);
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
	gtk_widget_set_sensitive(w,fv_any->anychars!=-1 && fv_any->fv->b.sf->bitmaps!=NULL);
    else if ( strcmp(name,"revert_file1")==0 )
	gtk_widget_set_sensitive(w,fv_any->fv->b.sf->origname!=NULL);
    else if ( strcmp(name,"revert_glyph1")==0 )
	gtk_widget_set_sensitive(w,fv_any->anychars!=-1 && fv_any->fv->b.sf->origname!=NULL);
    else if ( strcmp(name,"recent1")==0 )
	gtk_widget_set_sensitive(w,RecentFilesAny());
    else if ( strcmp(name,"script_menu1")==0 )
	gtk_widget_set_sensitive(w,script_menu_names[0]!=NULL);
    else if ( strcmp(name,"print2")==0 )
	gtk_widget_set_sensitive(w,!fv_any->fv->b.sf->onlybitmaps);
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
    GtkWidget *w;
    int gid;
    static char *poslist[] = { "cut2", "copy2", "copy_reference1", "copy_width1",
	    "copy_lbearing1", "copy_rbearing1", "paste2", "paste_into1",
	    "paste_after1", "same_glyph_as1", "clear2", "clear_background1",
	    "copy_fg_to_bg1", "join1", "replace_with_reference1",
	    "unlink_reference1", "remove_undoes1", NULL };

    w = lookup_widget( GTK_WIDGET(menuitem), "undo2" );
    for ( i=0; i<fv->b.map->enccount; ++i )
	if ( fv->b.selected[i] && (gid = fv->b.map->map[i])!=-1 &&
		fv->b.sf->glyphs[gid]!=NULL ) {
	    if ( fv->b.sf->glyphs[gid]->layers[ly_fore].undoes!=NULL )
    break;
	}
    gtk_widget_set_sensitive(w,i!=fv->b.map->enccount);

    w = lookup_widget( GTK_WIDGET(menuitem), "redo2" );
    for ( i=0; i<fv->b.map->enccount; ++i )
	if ( fv->b.selected[i] && (gid = fv->b.map->map[i])!=-1 &&
		fv->b.sf->glyphs[gid]!=NULL ) {
	    if ( fv->b.sf->glyphs[gid]->layers[ly_fore].redoes!=NULL )
    break;
	}
    gtk_widget_set_sensitive(w,i!=fv->b.map->enccount);

    for ( i=0; poslist[i]!=NULL; ++i ) {
	w = lookup_widget( GTK_WIDGET(menuitem), poslist[i] );
	if ( w!=NULL )
	    gtk_widget_set_sensitive(w,pos!=-1);
    }

    w = lookup_widget( GTK_WIDGET(menuitem), "copy_vwidth1" );
    if ( w!=NULL )
	gtk_widget_set_sensitive(w,pos!=-1 && fv->b.sf->hasvmetrics);
}

void FontViewMenu_ActivateDependents(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int anychars = FVAnyCharSelected(fv);
    int anygid = anychars<0 ? -1 : fv->b.map->map[anychars];

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "references1" ),
	    anygid>=0 &&
		    fv->b.sf->glyphs[anygid]!=NULL &&
		    fv->b.sf->glyphs[anygid]->dependents != NULL);
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "substitutions1" ),
	    anygid>=0 &&
		    fv->b.sf->glyphs[anygid]!=NULL &&
		    SCUsedBySubs(fv->b.sf->glyphs[anygid]));
}

void FontViewMenu_ActivateBuild(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int anybuildable = false;
    int anybuildableaccents = false;
    int i, gid;

    for ( i=0; i<fv->b.map->enccount; ++i ) if ( fv->b.selected[i] && (gid=fv->b.map->map[i])!=-1 ) {
	SplineChar *sc, dummy;
	sc = fv->b.sf->glyphs[gid];
	if ( sc==NULL )
	    sc = SCBuildDummy(&dummy,fv->b.sf,fv->b.map,i);
	if ( SFIsSomethingBuildable(fv->b.sf,sc,fv->b.active_layer,true)) {
	    anybuildableaccents = anybuildable = true;
    break;
	} else if ( SFIsSomethingBuildable(fv->b.sf,sc,fv->b.active_layer,false))
	    anybuildable = true;
    }

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "build_accented_char1" ),
	    anybuildableaccents );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "build_composite_char1" ),
	    anybuildable );
}

void FontViewMenu_ActivateElement(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int anychars = FVAnyCharSelected(fv);
    int anygid = anychars<0 ? -1 : fv->b.map->map[anychars];
    int anybuildable, anytraceable;
    int gid;
    GtkWidget *w;

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "char_info1" ),
	    anygid>=0 &&
		    (fv->b.cidmaster==NULL || fv->b.sf->glyphs[anygid]!=NULL));

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "find_problems1" ),
	    anygid!=-1);

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "transform1" ),
	    anygid!=-1);
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "non_linear_transform1" ),
	    anygid!=-1);

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "add_extrema1" ),
	    anygid!=-1 && !fv->b.sf->onlybitmaps );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "round_to_int1" ),
	    anygid!=-1 && !fv->b.sf->onlybitmaps );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "correct_direction1" ),
	    anygid!=-1 && !fv->b.sf->onlybitmaps );

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "simplify3" ),
	    anygid!=-1 && !fv->b.sf->onlybitmaps );

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "expand_stroke1" ),
	    anygid!=-1 && !fv->b.sf->onlybitmaps );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "overlap1" ),
	    anygid!=-1 && !fv->b.sf->onlybitmaps );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "style1" ),
	    anygid!=-1 && !fv->b.sf->onlybitmaps );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "tilepath1" ),
	    anygid!=-1 && !fv->b.sf->onlybitmaps );

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "bitmaps_available1" ),
	    fv->b.sf->mm==NULL );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "regenerate_bitmaps1" ),
	    fv->b.sf->mm==NULL && fv->b.sf->bitmaps!=NULL && !fv->b.sf->onlybitmaps );

    anybuildable = false;
    if ( anygid!=-1 ) {
	int i, gid;
	for ( i=0; i<fv->b.map->enccount; ++i ) if ( fv->b.selected[i] ) {
	    SplineChar *sc=NULL, dummy;
	    if ( (gid=fv->b.map->map[i])!=-1 )
		sc = fv->b.sf->glyphs[gid];
	    if ( sc==NULL )
		sc = SCBuildDummy(&dummy,fv->b.sf,fv->b.map,i);
	    if ( SFIsSomethingBuildable(fv->b.sf,sc,fv->b.active_layer,false)) {
		anybuildable = true;
	break;
	    }
	}
    }
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "build1" ),
	    anybuildable );

    anytraceable = false;
    if ( FindAutoTraceName()!=NULL && anychars!=-1 ) {
	int i;
	for ( i=0; i<fv->b.map->enccount; ++i )
	    if ( fv->b.selected[i] && (gid=fv->b.map->map[i])!=-1 &&
		    fv->b.sf->glyphs[gid]!=NULL &&
		    fv->b.sf->glyphs[gid]->layers[ly_back].images!=NULL ) {
		anytraceable = true;
	break;
	    }
    }
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "autotrace1" ),
	    anytraceable );

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "interpolate_fonts1" ),
	    !fv->b.sf->onlybitmaps );

#ifndef FONTFORGE_CONFIG_NONLINEAR
    w = lookup_widget( GTK_WIDGET(menuitem), "non_linear_transform1" );
    gtk_widget_hide(w);
#endif
#ifndef FONTFORGE_CONFIG_TILEPATH
    w = lookup_widget( GTK_WIDGET(menuitem), "tilepath1" );
    gtk_widget_hide(w);
#endif
}

void FontViewMenu_ActivateHints(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int anychars = FVAnyCharSelected(fv);
    int anygid = anychars<0 ? -1 : fv->b.map->map[anychars];
    int multilayer = fv->b.sf->multilayer;
    int l = fv->b.sf->layer_cnt;

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "autohint1" ),
	    anygid!=-1 && !multilayer );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "hint_subsitution_pts1" ),
	    !fv->b.sf->layers[l].order2 && anygid!=-1 && !multilayer );
    if ( fv->b.sf->mm!=NULL && fv->b.sf->mm->apple )
	gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "hint_subsitution_pts1" ),
		false);
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "auto_counter_hint1" ),
	    !fv->b.sf->layers[l].order2 && anygid!=-1 && !multilayer );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "dont_autohint1" ),
	    !fv->b.sf->layers[l].order2 && anygid!=-1 && !multilayer );

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "autoinstr1" ),
	    fv->b.sf->layers[l].order2 && anygid!=-1 && !multilayer );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "edit_instructions1" ),
	    fv->b.sf->layers[l].order2 && anygid>=0 && !multilayer );

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "edit_fpgm1" ),
	    fv->b.sf->layers[l].order2 && !multilayer );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "edit_prep1" ),
	    fv->b.sf->layers[l].order2 && !multilayer );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "edit_cvt_1" ),
	    fv->b.sf->layers[l].order2 && !multilayer );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "edit_maxp1" ),
	    fv->b.sf->layers[l].order2 && !multilayer );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "private_to_cvt_1" ),
	    fv->b.sf->layers[l].order2 && !multilayer );

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "clear_hints1" ),
	    anygid!=-1 );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "clear_instructions1" ),
	    fv->b.sf->layers[l].order2 && anygid!=-1 );
}

/* Builds up a menu containing all the anchor classes */
void FontViewMenu_ActivateAnchoredPairs(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);

    _aplistbuild(menuitem,fv->b.sf,FontViewMenu_AnchoredPairs);
}

void FontViewMenu_ActivateCombinations(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    SplineFont *sf = fv->b.sf;
    EncMap *map = fv->b.map;
    int i, gid, anyligs=0, anykerns=0;
    PST *pst;

    if ( sf->kerns ) anykerns=true;
    for ( i=0; i<map->enccount; ++i ) if ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL ) {
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

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "ligatures2" ),
	    anyligs);
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "kern_pairs1" ),
	    anykerns);
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "anchor_pairs1" ),
	    sf->anchor!=NULL);
}

void FontViewMenu_ActivateView(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int anychars = FVAnyCharSelected(fv);
    int i;
    BDFFont *bdf;
    int pos;
    SplineFont *sf = fv->b.sf;
    EncMap *map = fv->b.map;
    GtkWidget *menu = gtk_menu_item_get_submenu(menuitem);
    GList *kids, *next;
    static int sizes[] = { 24, 36, 48, 72, 96, 0 };
    char buffer[60];

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

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "next_glyph1" ),
	    anychars>=0 && anychars<map->enccount-1 );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "prev_glyph1" ),
	    anychars>0 );
    if ( anychars<0 ) pos = map->enccount;
    else for ( pos = anychars+1; pos<map->enccount &&
	    (map->map[pos]==-1 || !SCWorthOutputting(sf->glyphs[map->map[pos]]));
	    ++pos );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "next_defined_glyph1" ),
	    pos!=map->enccount );
    for ( pos=anychars-1; pos>=0 &&
	    (map->map[pos]==-1 || !SCWorthOutputting(sf->glyphs[map->map[pos]]));
	    --pos );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "prev_defined_glyph1" ),
	    pos>=0 );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "show_v_metrics1" ),
	    fv->b.sf->hasvmetrics );

    for ( i=0; sizes[i]!=0; ++i ) {
	char buffer[80];
	sprintf( buffer, "_%d_pixel_outline1", sizes[i]);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
		    lookup_widget( GTK_WIDGET(menuitem), buffer )),
		(fv->show!=NULL && fv->show==fv->filled && fv->show->pixelsize==sizes[i]));
	gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), buffer ),
		!fv->b.sf->onlybitmaps || fv->show==fv->filled );
    }

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
		lookup_widget( GTK_WIDGET(menuitem), "anti_alias1" )),
	    (fv->show!=NULL && fv->show->clut!=NULL));
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "anti_alias1" ),
	    !fv->b.sf->onlybitmaps || fv->show==fv->filled );
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(
		lookup_widget( GTK_WIDGET(menuitem), "fit_to_em1" )),
	    (fv->show!=NULL && !fv->show->bbsized));
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "fit_to_em1" ),
	    !fv->b.sf->onlybitmaps || fv->show==fv->filled );
}

void FontViewMenu_ActivateMetrics(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int i;
    int anychars = FVAnyCharSelected(fv);
    char *argnames[] = { "center_in_width1", "thirds_in_width1", "set_width1",
	    "set_lbearing1", "set_rbearing1", "set_vertical_advance1", NULL };
    char *vnames[] = { "vkern_by_classes1", "vkern_from_hkern1",
	    "remove_all_vkern_pairs1", NULL };

    for ( i=0; argnames[i]!=NULL; ++i )
	gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), argnames[i] ),
		anychars!=-1 );
    if ( !fv->b.sf->hasvmetrics )
	gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "set_vertical_advance1" ),
		false );
    for ( i=0; vnames[i]!=NULL; ++i )
	gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), vnames[i] ),
		fv->b.sf->hasvmetrics );
}

void FontViewMenu_ActivateCID(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int j;
    SplineFont *sub, *cidmaster = fv->b.cidmaster;
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
	    gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(w), sub==fv->b.sf );
	    g_signal_connect ( G_OBJECT( w ), "activate",
			    G_CALLBACK (FontViewMenu_ShowSubFont),
			    sub);
	    gtk_menu_shell_append(GTK_MENU_SHELL(menu),w);
	}
    }

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "convert_to_cid1" ),
	    cidmaster==NULL && fv->b.sf->mm==NULL );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "convert_by_cmap1" ),
	    cidmaster==NULL && fv->b.sf->mm==NULL );

	    /* OpenType allows at most 255 subfonts (PS allows more, but why go to the effort to make safe font check that? */
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "insert_font1" ),
	    cidmaster!=NULL && cidmaster->subfontcnt<255 );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "insert_empty_font1" ),
	    cidmaster!=NULL && cidmaster->subfontcnt<255 );

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "remove_font1" ),
	    cidmaster!=NULL && cidmaster->subfontcnt>1 );

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "flatten1" ),
	    cidmaster!=NULL );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "flatten_by_cmap1" ),
	    cidmaster!=NULL );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "cid_font_info1" ),
	    cidmaster!=NULL );
}

void FontViewMenu_ActivateMM(GtkMenuItem *menuitem, gpointer user_data) {
    FontView *fv = FV_From_MI(menuitem);
    int j;
    MMSet *mm = fv->b.sf->mm;
    SplineFont *sub;
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
	    gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(w), sub==fv->b.sf );
	    g_signal_connect ( G_OBJECT( w ), "activate",
			    G_CALLBACK (FontViewMenu_ShowSubFont),
			    sub);
	    gtk_menu_shell_append(GTK_MENU_SHELL(menu),w);
	}
    }

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "mm_validity_check1" ),
	    mm!=NULL );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "mm_info1" ),
	    mm!=NULL );
    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "blend_to_new_font1" ),
	    mm!=NULL );

    gtk_widget_set_sensitive(lookup_widget( GTK_WIDGET(menuitem), "mm_change_def_weights1" ),
	    mm!=NULL && !mm->apple );
}

static int FeatureTrans(FontView *fv, int enc) {
    SplineChar *sc;
    PST *pst;
    char *pt;
    int gid;

    if ( enc<0 || enc>=fv->b.map->enccount || (gid = fv->b.map->map[enc])==-1 )
return( -1 );
    if ( fv->cur_subtable==NULL )
return( gid );

    sc = fv->b.sf->glyphs[gid];
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
    gid = SFFindExistingSlot(fv->b.sf, -1, pst->u.subs.variant );
    if ( pt!=NULL )
	*pt = ' ';
return( gid );
}

void FVRefreshChar(FontView *fv,int gid) {
    BDFChar *bdfc;
    int i, j, enc;
    MetricsView *mv;
    uint32 clut[256];
    GdkGCValues values;
    GdkGC *gc = fv->v->style->bg_gc[fv->v->state];

    gdk_gc_get_values(gc,&values);

    /* Can happen in scripts */ /* Can happen if we do an AutoHint when generating a tiny font for freetype context */
    if ( fv->v==NULL || fv->colcnt==0 || fv->b.sf->glyphs[gid]== NULL )
return;

    FVBDFCInitClut(fv,fv->show, clut);

    for ( fv=(FontView *) fv->b.sf->fv; fv!=NULL; fv = (FontView *) fv->b.nextsame ) {
	for ( mv=fv->b.sf->metrics; mv!=NULL; mv=mv->next )
	    MVRefreshChar(mv,fv->b.sf->glyphs[gid]);
	if ( fv->show==fv->filled )
	    bdfc = BDFPieceMealCheck(fv->show,gid);
	else
	    bdfc = fv->show->glyphs[gid];
	/* A glyph may be encoded in several places, all need updating */
	for ( enc = 0; enc<fv->b.map->enccount; ++enc ) if ( fv->b.map->map[enc]==gid ) {
	    i = enc / fv->colcnt;
	    j = enc - i*fv->colcnt;
	    i -= fv->rowoff;
	    if ( i>=0 && i<fv->rowcnt ) {
		GdkRectangle box;
		if ( bdfc==NULL )
	continue;
		FVBDFCToSlot(fv,fv->show,bdfc,clut);

		box.x = j*fv->cbw+1; box.width = fv->cbw-1;
		box.y = i*fv->cbh+fv->lab_height+1; box.height = fv->cbw;
		gdk_draw_rectangle(GDK_DRAWABLE(fv->v->window),gc,TRUE,
		    box.x, box.y, box.width, box.height);
		gdk_pixbuf_render_to_drawable(fv->char_slot,GDK_DRAWABLE(fv->v->window),
			gc,0,0,
			box.x+(fv->cbw-2-fv->magnify*(bdfc->xmax-bdfc->xmin+1))/2,
			box.y,
			gdk_pixbuf_get_width(fv->char_slot),gdk_pixbuf_get_height(fv->char_slot),
			FALSE, 0,0 );
		if ( fv->b.selected[enc] ) {
		    gdk_gc_set_function(gc,GDK_XOR);
		    gdk_draw_rectangle(GDK_DRAWABLE(fv->v->window), gc, TRUE,
			    box.x, box.y,  box.width, box.height);
		    gdk_gc_set_values(gc,&values, GDK_GC_FUNCTION );
		}
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
    for ( mv=fv->b.sf->metrics; mv!=NULL; mv=mv->next )
	MVRegenChar(mv,sc);

    FVRefreshChar(fv,sc->orig_pos);
#if HANYANG
    if ( sc->compositionunit && fv->b.sf->rules!=NULL )
	Disp_RefreshChar(fv->b.sf,sc);
#endif

    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next )
	FVRegenChar(fv,dlist->sc);
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
    SplineFont *sf = fv->b.sf;
    SplineChar *base_sc = SFMakeChar(sf,fv->b.map,enc), *feat_sc = NULL;
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
	feat_sc = SFSplineCharCreate(sf);
	feat_sc->parent = sf;
	feat_sc->unicodeenc = uni;
	if ( uni!=-1 ) {
	    feat_sc->name = malloc(8);
	    feat_sc->unicodeenc = uni;
	    sprintf( feat_sc->name,"uni%04X", uni );
	} else if ( fv->cur_subtable->suffix!=NULL ) {
	    feat_sc->name = malloc(strlen(base_sc->name)+strlen(fv->cur_subtable->suffix)+2);
	    sprintf( feat_sc->name, "%s.%s", base_sc->name, fv->cur_subtable->suffix );
	} else if ( fl==NULL ) {
	    feat_sc->name = strconcat(base_sc->name,".unknown");
	} else if ( fl->ismac ) {
	    /* mac feature/setting */
	    feat_sc->name = malloc(strlen(base_sc->name)+14);
	    sprintf( feat_sc->name,"%s.m%d_%d", base_sc->name,
		    (int) (fl->featuretag>>16),
		    (int) ((fl->featuretag)&0xffff) );
	} else {
	    /* OpenType feature tag */
	    feat_sc->name = malloc(strlen(base_sc->name)+6);
	    sprintf( feat_sc->name,"%s.%c%c%c%c", base_sc->name,
		    (int) (fl->featuretag>>24),
		    (int) ((fl->featuretag>>16)&0xff),
		    (int) ((fl->featuretag>>8)&0xff),
		    (int) ((fl->featuretag)&0xff) );
	}
	SFAddGlyphAndEncode(sf,feat_sc,fv->b.map,fv->b.map->enccount);
	AddSubPST(base_sc,fv->cur_subtable,feat_sc->name);
return( feat_sc );
    } else
return( base_sc );
}
#else
SplineChar *FVMakeChar(FontView *fv,int enc) {
    SplineFont *sf = fv->b.sf;
return( SFMakeChar(sf,fv->b.map,enc) );
}
#endif

static GdkPixbuf *gdk_pixbuf_rotate(GdkPixbuf *unrot) {
    int orig_width = gdk_pixbuf_get_width(unrot);
    int orig_height = gdk_pixbuf_get_height(unrot);
    int orig_rowstride = gdk_pixbuf_get_rowstride(unrot);
    guchar *orig_pixels = gdk_pixbuf_get_pixels(unrot);
    GdkPixbuf *rot = gdk_pixbuf_new(GDK_COLORSPACE_RGB,false,8,orig_height,orig_width);
    int new_rowstride;
    guchar *new_pixels;
    int i,j;
    int n = gdk_pixbuf_get_n_channels(unrot);

    if ( rot==NULL )
return( NULL );

    new_rowstride = gdk_pixbuf_get_rowstride(rot);
    new_pixels = gdk_pixbuf_get_pixels(rot);
    for ( i=0; i<orig_height; ++i ) for ( j=0; j<orig_width; ++j ) {
	guchar *orig_p = &orig_pixels[(i*orig_rowstride+j)*n];
	guchar *new_p = &new_pixels[(j*new_rowstride+i)*3];
	new_p[0] = orig_p[0];
	new_p[1] = orig_p[1];
	new_p[2] = orig_p[2];
    }
return( rot );
}

static GdkPixbuf *UniGetRotatedGlyph(FontView *fv, SplineChar *sc,int uni) {
    SplineFont *sf = fv->b.sf;
    int cid=-1;
    static GdkPixmap *pixmap=NULL;
    char buf[20], *pt;
    GdkPixbuf *unrot, *rot;
    SplineFont *cm = sf->cidmaster;
    PangoRectangle pos;

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
    /*if ( uni&0x10000 ) uni -= 0x10000;		*//* Bug in some old cidmap files */
    if ( uni<0 || uni>0x10ffff )
return( NULL );

    if ( pixmap==NULL ) {
	pixmap = gdk_pixmap_new(fv->v->window,2*fv->lab_height,2*fv->lab_height,-1);
	if ( pixmap==NULL )
return( NULL );
    }
    gdk_draw_rectangle(GDK_DRAWABLE(pixmap),fv->v->style->bg_gc[fv->v->state],
	    true, 0,0, 2*fv->lab_height,2*fv->lab_height);
    pt = buf;
    pt = utf8_idpb(pt,uni);
    *pt = '\0';
    pango_layout_set_text( fv->vlayout, buf, -1);
    gdk_draw_layout( GDK_DRAWABLE(pixmap), fv->v->style->fg_gc[GTK_STATE_NORMAL],
	0, 0, fv->vlayout );
    pango_layout_index_to_pos(fv->vlayout,0,&pos);
    unrot = gdk_pixbuf_get_from_drawable(NULL,GDK_DRAWABLE(pixmap),NULL,
	    0,0, 0,0, pos.width/1000,pos.height/1000);
    if ( unrot==NULL )
return( NULL );

    rot = gdk_pixbuf_rotate(unrot);
    g_object_unref(G_OBJECT(unrot));
return( rot );
}

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
    
static PangoFontDescription *FVCheckFont(FontView *fv,int type) {
    PangoFontDescription *pfd;
    PangoContext *context;
    int family = type>>2;
    char *fontnames;

    static char *defaultfontnames[] = {
	    "times,serif,caslon,clearlyu,unifont",
	    "script,formalscript,clearlyu,unifont",
	    "fraktur,clearlyu,unifont",
	    "doublestruck,clearlyu,unifont",
	    "helvetica,caliban,sansserif,sans,clearlyu,unifont",
	    "courier,monospace,clearlyu,unifont",
	    NULL
	};

    if ( fv->fontset[type]==NULL ) {
        fontnames = defaultfontnames[family];

	pfd = pango_font_description_new();
	pango_font_description_set_family(pfd,fontnames);
	if ( type&_uni_bold )
	    pango_font_description_set_weight(pfd,700);
	if ( type&_uni_italic )
	    pango_font_description_set_style(pfd,PANGO_STYLE_ITALIC);
	context = gtk_widget_get_pango_context( fv->v );
	pango_font_description_merge(pfd,fv->fontset[0],false);
	fv->fontset[type] = pfd;
	/*g_object_ref( G_OBJECT( fv->fontset[type] ));*/	/* font descriptions aren't objects */
    }
return( fv->fontset[type] );
}

extern unichar_t adobes_pua_alts[0x200][3];

static void do_Adobe_Pua(char *buf,int sob,int uni) {
    int i, j;
    char *pt = buf;

    for ( i=j=0; pt-buf<sob-4 && i<3; ++i ) {
	int ch = adobes_pua_alts[uni-0xf600][i];
	if ( ch==0 )
    break;
	if ( ch>=0xf600 && ch<=0xf7ff && adobes_pua_alts[ch-0xf600]!=0 ) {
	    do_Adobe_Pua(pt,sob-(pt-buf),ch);
	    while ( *pt!='\0' ) ++pt;
	} else
	    pt = utf8_idpb(pt,ch);
    }
    *pt = 0;
}

static void gdk_gc_set_fg(GdkGC *gc,Color fg) {
    GdkColor col;

    col.red   = ((fg>>16)&0xff)*0x101;
    col.green = ((fg>>8 )&0xff)*0x101;
    col.blue  = ((fg    )&0xff)*0x101;
    col.pixel = fg;
    gdk_gc_set_foreground(gc,&col);
}

gboolean FontViewView_Expose(GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    FontView *fv = FV_From_Widget(widget);
    int i, j, width, gid;
    int changed;
    SplineChar dummy;
    int styles, laststyles=0;
    GdkPixbuf *rotated=NULL;
    int em = fv->b.sf->ascent+fv->b.sf->descent;
    int yorg = fv->magnify*(fv->show->ascent);
    GdkRectangle r, full, box;
    GdkGCValues values;
    GdkGC *gc = fv->gc;
    GdkColor col, *gdk_def_fg = &widget->style->fg[widget->state];
    Color bg, def_fg = COLOR_CREATE( (gdk_def_fg->red>>8), (gdk_def_fg->green>>8), (gdk_def_fg->blue>>8));
    uint32 clut[256];
    int l = fv->b.sf->layer_cnt;

    full.x = full.y = 0; full.width = full.height = 5000;
    gdk_gc_get_values(gc,&values);
    FVBDFCInitClut(fv,fv->show,clut);

    for ( i=0; i<=fv->rowcnt; ++i ) {
	gdk_draw_line(GDK_DRAWABLE(widget->window),widget->style->fg_gc[GTK_STATE_NORMAL],0,i*fv->cbh,fv->width,i*fv->cbh);
	gdk_draw_line(GDK_DRAWABLE(widget->window),widget->style->fg_gc[GTK_STATE_INSENSITIVE],0,i*fv->cbh+fv->lab_height,fv->width,i*fv->cbh+fv->lab_height);
    }
    for ( i=0; i<=fv->colcnt; ++i )
	gdk_draw_line(GDK_DRAWABLE(widget->window),widget->style->fg_gc[GTK_STATE_NORMAL],
	    i*fv->cbw,0,i*fv->cbw,fv->height);
    for ( i=event->area.y/fv->cbh; i<=fv->rowcnt && 
	    (event->area.y+event->area.height+fv->cbh-1)/fv->cbh; ++i ) for ( j=0; j<fv->colcnt; ++j ) {
	int index = (i+fv->rowoff)*fv->colcnt+j;
	int feat_gid;
	SplineChar *sc;
	styles = 0;
	if ( index < fv->b.map->enccount && index!=-1 ) {
	    char utf8_buf[60];
	    Color fg;
	    extern const int amspua[];
	    int uni;
	    struct cidmap *cidmap = NULL;
	    sc = (gid=fv->b.map->map[index])!=-1 ? fv->b.sf->glyphs[gid]: NULL;

	    if ( fv->b.cidmaster!=NULL )
		cidmap = FindCidMap(fv->b.cidmaster->cidregistry,fv->b.cidmaster->ordering,fv->b.cidmaster->supplement,fv->b.cidmaster);

	    if ( ( fv->b.map->enc==&custom && index<256 ) ||
		 ( fv->b.map->enc!=&custom && index<fv->b.map->enc->char_cnt ) ||
		 ( cidmap!=NULL && index<MaxCID(cidmap) ))
		fg = def_fg;
	    else
		fg = 0x505050;
	    if ( sc==NULL )
		sc = SCBuildDummy(&dummy,fv->b.sf,fv->b.map,index);
	    uni = sc->unicodeenc;
	    memset(utf8_buf,0,10);
	    if ( fv->b.sf->uni_interp==ui_ams && uni>=0xe000 && uni<=0xf8ff &&
		    amspua[uni-0xe000]!=0 )
		uni = amspua[uni-0xe000];
	    switch ( fv->glyphlabel ) {
	      case gl_name:
		strncpy(utf8_buf,sc->name,sizeof(utf8_buf)-1);
		styles = _uni_sans;
	      break;
	      case gl_unicode:
		if ( sc->unicodeenc!=-1 )
		    sprintf(utf8_buf,"%04x",sc->unicodeenc);
		else
		    strcpy(utf8_buf,"?");
		styles = _uni_sans;
	      break;
	      case gl_encoding:
		if ( fv->b.map->enc->only_1byte ||
			(fv->b.map->enc->has_1byte && index<256))
		    sprintf(utf8_buf,"%02x",index);
		else
		    sprintf(utf8_buf,"%04x",index);
		styles = _uni_sans;
	      break;
	      case gl_glyph:
		if ( uni==0xad )
		    utf8_buf[0] = '-';
		else if ( fv->b.sf->uni_interp==ui_adobe && uni>=0xf600 && uni<=0xf7ff &&
			adobes_pua_alts[uni-0xf600]!=0 ) {
		    do_Adobe_Pua(utf8_buf,sizeof(utf8_buf),uni);
		} else if ( uni>=0x1d400 && uni<=0x1d7ff ) {
		    int i;
		    for ( i=0; mathmap[i].start!=0; ++i ) {
			if ( uni<=mathmap[i].last ) {
			    char *pt = utf8_buf;
			    pt = utf8_idpb(pt,maps[mathmap[i].charset][uni-mathmap[i].start]);
			    *pt = '\0';
			    styles = mathmap[i].styles;
		    break;
			}
		    }
		} else if ( uni>=0xe0020 && uni<=0xe007e ) {
		    utf8_buf[0] = uni-0xe0000;	/* A map of Ascii for language names */
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
		    pt = utf8_idpb(pt,uni);
		    *pt = '\0';
		} else {
		    char *pt = strchr(sc->name,'.');
		    char *upt = utf8_buf;
		    utf8_buf[0] = '?';
		    fg = 0xff0000;
		    if ( pt!=NULL ) {
			int i, n = pt-sc->name;
			char *end;
			SplineFont *cm = fv->b.sf->cidmaster;
			if ( n==7 && sc->name[0]=='u' && sc->name[1]=='n' && sc->name[2]=='i' &&
				(i=strtol(sc->name+3,&end,16), end-sc->name==7))
			    upt = utf8_idpb(upt,i);
			else if ( n>=5 && n<=7 && sc->name[0]=='u' &&
				(i=strtol(sc->name+1,&end,16), end-sc->name==n))
			    upt = utf8_idpb(upt,i);
			else if ( cm!=NULL && (i=CIDFromName(sc->name,cm))!=-1 ) {
			    int uni;
			    uni = CID2Uni(FindCidMap(cm->cidregistry,cm->ordering,cm->supplement,cm),
				    i);
			    if ( uni!=-1 )
				upt = utf8_idpb(upt,i);
			} else {
			    int uni;
			    *pt = '\0';
			    uni = UniFromName(sc->name,fv->b.sf->uni_interp,fv->b.map->enc);
			    if ( uni!=-1 )
				upt = utf8_idpb(upt,uni);
			    *pt = '.';
			}
			if ( strstr(pt,".vert")!=NULL )
			    rotated = UniGetRotatedGlyph(fv,sc,utf8_buf[0]!='?'?utf8_buf[0]:-1);
			if ( pt!=utf8_buf ) {
			    *pt = '\0';
			    fg = def_fg;
			    if ( strstr(pt,".italic")!=NULL )
				styles = _uni_italic|_uni_mono;
			}
		    } else if ( strncmp(sc->name,"hwuni",5)==0 ) {
			int uni=-1;
			char *pt = utf8_buf;
			sscanf(sc->name,"hwuni%x", (unsigned *) &uni );
			if ( uni!=-1 ) {
			    pt = utf8_idpb(pt,uni);
			    *pt = '\0';
			}
		    } else if ( strncmp(sc->name,"italicuni",9)==0 ) {
			int uni=-1;
			char *pt = utf8_buf;
			sscanf(sc->name,"italicuni%x", (unsigned *) &uni );
			if ( uni!=-1 ) {
			    pt = utf8_idpb(pt,uni);
			    *pt = '\0';
			    styles=_uni_italic|_uni_mono;
			}
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
		col.pixel = bg;
		col.red   = ((bg>>16)&0xff)*0x101;
		col.green = ((bg>>8 )&0xff)*0x101;
		col.blue  = ((bg    )&0xff)*0x101;
		gdk_gc_set_foreground(gc, &col);
		gdk_draw_rectangle(fv->v->window,gc,TRUE,
			r.x,r.y, r.width,r.height);
		gdk_gc_set_values(gc,&values, GDK_GC_FOREGROUND );
	    }
	    if ( (!fv->b.sf->layers[l].order2 && sc->changedsincelasthinted ) ||
		     ( fv->b.sf->layers[l].order2 && sc->layers[ly_fore].splines!=NULL &&
			sc->ttf_instrs_len<=0 ) ||
		     ( fv->b.sf->layers[l].order2 && sc->instructions_out_of_date ) ) {
		GdkColor hintcol;
		memset(&hintcol,0,sizeof(hintcol));
		if ( fv->b.sf->layers[l].order2 && sc->instructions_out_of_date && sc->ttf_instrs_len>0 ) {
		    hintcol.red  = 0xffff;
		    hintcol.pixel = 0xff0000;
		} else {
		    hintcol.blue = 0xffff;
		    hintcol.pixel = 0x0000ff;
		}
		gdk_gc_set_foreground(gc, &hintcol);
		gdk_draw_line(fv->v->window,gc,r.x,r.y,r.x,r.y+r.height-1);
		gdk_draw_line(fv->v->window,gc,r.x+1,r.y,r.x+1,r.y+r.height-1);
		gdk_draw_line(fv->v->window,gc,r.x+2,r.y,r.x+2,r.y+r.height-1);
		gdk_draw_line(fv->v->window,gc,r.x+r.width-1,r.y,r.x+r.width-1,r.y+r.height-1);
		gdk_draw_line(fv->v->window,gc,r.x+r.width-2,r.y,r.x+r.width-2,r.y+r.height-1);
		gdk_draw_line(fv->v->window,gc,r.x+r.width-3,r.y,r.x+r.width-3,r.y+r.height-1);
		gdk_gc_set_values(gc,&values, GDK_GC_FOREGROUND );
	    }
	    if ( rotated!=NULL ) {
		gdk_pixbuf_render_to_drawable(rotated,GDK_DRAWABLE(fv->v->window),gc,0,0,
			j*fv->cbw+2,i*fv->cbh+2,
			gdk_pixbuf_get_width(rotated),gdk_pixbuf_get_height(rotated),
			FALSE, 0,0 );
		g_object_unref(G_OBJECT(rotated));
		rotated = NULL;
	    } else {
		int index;
		PangoRectangle pos;
		if ( styles!=laststyles )
		    pango_layout_set_font_description(fv->vlayout,FVCheckFont(fv,styles));
		laststyles = styles;
		pango_layout_set_text( fv->vlayout, utf8_buf, -1);
		index = utf8_strlen(utf8_buf)-1;
		pango_layout_index_to_pos(fv->vlayout,index,&pos);
		width = (pos.x+pos.width)/1000;
		if ( width >= fv->cbw-1 ) {
		    gdk_gc_set_clip_rectangle(gc,&r);
		    width = fv->cbw-1;
		}
		gdk_draw_layout( GDK_DRAWABLE(widget->window), gc,
			j*fv->cbw+(fv->cbw-1-width)/2, i*fv->cbh+1,
			fv->vlayout );
		if ( width >= fv->cbw-1 )
		    gdk_gc_set_clip_rectangle(gc,&full);
		laststyles = styles;
	    }
	    changed = sc->changed;
	    if ( fv->b.sf->onlybitmaps )
		changed = gid==-1 || fv->show->glyphs[gid]==NULL? false : fv->show->glyphs[gid]->changed;
	    if ( changed ) {
		GdkRectangle r;
		r.x = j*fv->cbw+1; r.width = fv->cbw-1;
		r.y = i*fv->cbh+1; r.height = fv->lab_height-1;
		if ( bg!=COLOR_DEFAULT ) {
		    col.red   = ((bg>>16)&0xff)*0x101;
		    col.green = ((bg>>8 )&0xff)*0x101;
		    col.blue  = ((bg    )&0xff)*0x101;
		    col.pixel = bg;
		} else
		    col = values.background;
		/* Bug!!! This only works on RealColor */
		col.red ^= values.foreground.red;
		col.green ^= values.foreground.green;
		col.blue ^= values.foreground.blue;
		col.pixel ^= values.foreground.pixel;
		/* End bug */
		gdk_gc_set_function(gc,GDK_XOR);
		gdk_gc_set_foreground(gc, &col);
		gdk_draw_rectangle(fv->v->window, gc, TRUE,
			r.x, r.y,  r.width, r.height);
		gdk_gc_set_values(gc,&values,
			GDK_GC_FOREGROUND | GDK_GC_FUNCTION );
	    }
	}

	feat_gid = FeatureTrans(fv,index);
	sc = feat_gid!=-1 ? fv->b.sf->glyphs[feat_gid]: NULL;
	if ( !SCWorthOutputting(sc) ) {
	    int x = j*fv->cbw+1, xend = x+fv->cbw-2;
	    int y = i*fv->cbh+fv->lab_height+1, yend = y+fv->cbw-1;
	    col.red = 0xdddd; col.green = col.blue = 0x8000; col.pixel = 0xdd8080;
	    gdk_gc_set_foreground(gc, &col);
	    gdk_draw_line(fv->v->window,gc,x,y,xend,yend);
	    gdk_draw_line(fv->v->window,gc,x,yend,xend,y);
	    gdk_gc_set_values(gc,&values, GDK_GC_FOREGROUND );
	}
	if ( sc!=NULL ) {
	    BDFChar *bdfc;

	    if ( fv->show!=NULL && fv->show->piecemeal &&
		    feat_gid!=-1 &&
		    (feat_gid>=fv->show->glyphcnt || fv->show->glyphs[feat_gid]==NULL) &&
		    fv->b.sf->glyphs[feat_gid]!=NULL )
		BDFPieceMeal(fv->show,feat_gid);

	    if ( fv->show!=NULL && feat_gid!=-1 &&
		    feat_gid < fv->show->glyphcnt &&
		    fv->show->glyphs[feat_gid]==NULL &&
		    SCWorthOutputting(fv->b.sf->glyphs[feat_gid]) ) {
		/* If we have an outline but no bitmap for this slot */
		box.x = j*fv->cbw+1; box.width = fv->cbw-2;
		box.y = i*fv->cbh+fv->lab_height+2; box.height = box.width+1;
		col.red = 0xffff; col.green = col.blue = 0x8000; col.pixel = 0xdd8080;
		gdk_gc_set_foreground(gc, &col);
		gdk_draw_rectangle(GDK_DRAWABLE(widget->window),gc,false,box.x,box.y,box.width,box.height);
		++box.x; ++box.y; box.width -= 2; box.height -= 2;
		gdk_draw_rectangle(GDK_DRAWABLE(widget->window),gc,false,box.x,box.y,box.width,box.height);
		gdk_gc_set_values(gc,&values, GDK_GC_FOREGROUND );
/* When reencoding a font we can find times where index>=show->charcnt */
	    } else if ( fv->show!=NULL && feat_gid<fv->show->glyphcnt && feat_gid!=-1 &&
		    fv->show->glyphs[feat_gid]!=NULL ) {
		bdfc = fv->show->glyphs[feat_gid];
		FVBDFCToSlot(fv,fv->show,bdfc,clut);
		gdk_pixbuf_render_to_drawable(fv->char_slot,GDK_DRAWABLE(fv->v->window),
			gc,0,0,
			j*fv->cbw+1+(fv->cbw-2-fv->magnify*(bdfc->xmax-bdfc->xmin+1))/2,
			i*fv->cbh+fv->lab_height+1,
			gdk_pixbuf_get_width(fv->char_slot),gdk_pixbuf_get_height(fv->char_slot),
			FALSE, 0,0 );
		if ( fv->showhmetrics ) {
		    int x1, x0 = j*fv->cbw+1+(fv->cbw-2-fv->magnify*(bdfc->xmax-bdfc->xmin+1))/2- bdfc->xmin*fv->magnify;
		    /* Draw advance width & horizontal origin */
		    if ( fv->showhmetrics&fvm_origin ) {
			gdk_gc_set_fg(gc,METRICS_ORIGIN);
			gdk_draw_line(widget->window,gc,x0,i*fv->cbh+fv->lab_height+yorg-3,x0,
				i*fv->cbh+fv->lab_height+yorg+2);
		    }
		    x1 = x0 + bdfc->width;
		    if ( fv->showhmetrics&fvm_advanceat ) {
			gdk_gc_set_fg(gc,METRICS_ADVANCE);
			gdk_draw_line(widget->window,gc,x1,i*fv->cbh+fv->lab_height+1,x1,
				(i+1)*fv->cbh-1);
		    }
		    if ( fv->showhmetrics&fvm_advanceto ) {
			gdk_gc_set_fg(gc,METRICS_ADVANCE);
			gdk_draw_line(widget->window,gc,x0,(i+1)*fv->cbh-2,x1,
				(i+1)*fv->cbh-2);
		    }
		    gdk_gc_set_values(gc,&values, GDK_GC_FOREGROUND );
		}
		if ( fv->showvmetrics ) {
		    int x0 = j*fv->cbw+1+(fv->cbw-2-fv->magnify*(bdfc->xmax-bdfc->xmin+1))/2
			    - bdfc->xmin*fv->magnify
			    + fv->magnify*fv->show->pixelsize/2;
		    int y0 = i*fv->cbh+fv->lab_height+yorg;
		    int yvw = y0 + fv->magnify*sc->vwidth*fv->show->pixelsize/em;
		    if ( fv->showvmetrics&fvm_baseline ) {
			gdk_gc_set_fg(gc,METRICS_BASELINE);
			gdk_draw_line(widget->window,gc,x0,i*fv->cbh+fv->lab_height+1,x0,
				(i+1)*fv->cbh-1);
		    }
		    if ( fv->showvmetrics&fvm_advanceat ) {
			gdk_gc_set_fg(gc,METRICS_ADVANCE);
			gdk_draw_line(widget->window,gc,j*fv->cbw,yvw,(j+1)*fv->cbw,
				yvw);
		    }
		    if ( fv->showvmetrics&fvm_advanceto ) {
			gdk_gc_set_fg(gc,METRICS_ADVANCE);
			gdk_draw_line(widget->window,gc,j*fv->cbw+2,y0,j*fv->cbw+2,
				yvw);
		    }
		    if ( fv->showvmetrics&fvm_origin ) {
			gdk_gc_set_fg(gc,METRICS_ORIGIN);
			gdk_draw_line(widget->window,gc,x0-3,i*fv->cbh+fv->lab_height+yorg,x0+2,i*fv->cbh+fv->lab_height+yorg);
		    }
		    gdk_gc_set_values(gc,&values, GDK_GC_FOREGROUND );
		}
	    }
	}
	if ( index<fv->b.map->enccount && fv->b.selected[index] ) {
	    box.x = j*fv->cbw+1; box.width = fv->cbw-1;
	    box.y = i*fv->cbh+fv->lab_height+1; box.height = fv->cbw;
	    gdk_gc_set_function(gc,GDK_XOR);
	    gdk_gc_set_fg(gc, bg);
	    gdk_draw_rectangle(widget->window, gc, TRUE,
		    box.x, box.y,  box.width, box.height);
	    gdk_gc_set_values(gc,&values,
		    GDK_GC_FOREGROUND | GDK_GC_FUNCTION );
	}
    }
    if ( fv->showhmetrics&fvm_baseline ) {
	for ( i=0; i<=fv->rowcnt; ++i ) {
	    gdk_gc_set_fg(gc,METRICS_BASELINE);
	    gdk_draw_line(widget->window,gc,0,i*fv->cbh+fv->lab_height+fv->show->ascent+1,fv->width,i*fv->cbh+fv->lab_height+fv->show->ascent+1);
	}
	gdk_gc_set_values(gc,&values, GDK_GC_FOREGROUND );
    }
return( true );
}

static char *chosung[] = { "G", "GG", "N", "D", "DD", "L", "M", "B", "BB", "S", "SS", "", "J", "JJ", "C", "K", "T", "P", "H", NULL };
static char *jungsung[] = { "A", "AE", "YA", "YAE", "EO", "E", "YEO", "YE", "O", "WA", "WAE", "OE", "YO", "U", "WEO", "WE", "WI", "YU", "EU", "YI", "I", NULL };
static char *jongsung[] = { "", "G", "GG", "GS", "N", "NJ", "NH", "D", "L", "LG", "LM", "LB", "LS", "LT", "LP", "LH", "M", "B", "BS", "S", "SS", "NG", "J", "C", "K", "T", "P", "H", NULL };

gboolean FontView_StatusExpose(GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    FontView *fv = FV_From_Widget(widget);
    char buffer[550], *pt, temp[200];
    SplineChar *sc, dummy;
    SplineFont *sf = fv->b.sf;
    EncMap *map = fv->b.map;
    int gid;
    int uni;
    int state = GTK_STATE_NORMAL;
    int len, tlen;
    PangoRectangle pos;

    if ( fv->end_pos>=map->enccount || fv->pressed_pos>=map->enccount ||
	    fv->end_pos<0 || fv->pressed_pos<0 )
	fv->end_pos = fv->pressed_pos = -1;	/* Can happen after reencoding */
    if ( fv->end_pos == -1 )
return( true );

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
    sc = (gid=fv->b.map->map[fv->end_pos])!=-1 ? sf->glyphs[gid] : NULL;
    SCBuildDummy(&dummy,sf,fv->b.map,fv->end_pos);
    if ( sc==NULL ) sc = &dummy;
    uni = dummy.unicodeenc!=-1 ? dummy.unicodeenc : sc->unicodeenc;
    if ( uni!=-1 )
	sprintf( buffer+strlen(buffer), "U+%04X", uni );
    else
	sprintf( buffer+strlen(buffer), "U+????" );
    sprintf( buffer+strlen(buffer), "  %.*s", (int) (sizeof(buffer)-strlen(buffer)-1),
	    sc->name );

    strcat(buffer,"  ");
    len = strlen(buffer);

    if ( uni==-1 && (pt=strchr(sc->name,'.'))!=NULL && pt-sc->name<30 ) {
	strncpy(temp,sc->name,pt-sc->name);
	temp[(pt-sc->name)] = '\0';
	uni = UniFromName(temp,fv->b.sf->uni_interp,map->enc);
	if ( uni!=-1 ) {
	    sprintf( buffer+strlen(buffer), "U+%04X ", uni );
	}
	state = GTK_STATE_INSENSITIVE;
    }
    if ( uni!=-1 && uni<0x110000 && _UnicodeNameAnnot!=NULL &&
	    _UnicodeNameAnnot[uni>>16][(uni>>8)&0xff][uni&0xff].name!=NULL ) {
	strncat(buffer, _UnicodeNameAnnot[uni>>16][(uni>>8)&0xff][uni&0xff].name, 80);
    } else if ( uni>=0xAC00 && uni<=0xD7A3 ) {
	sprintf( buffer+strlen(buffer), "Hangul Syllable %s%s%s",
		chosung[(uni-0xAC00)/(21*28)],
		jungsung[(uni-0xAC00)/28%21],
		jongsung[(uni-0xAC00)%28] );
    } else if ( uni!=-1 ) {
	strncat(buffer, UnicodeRange(uni),80);
    }

    pango_layout_set_text( fv->statuslayout, buffer, len);
    gdk_draw_layout( GDK_DRAWABLE(fv->status->window), fv->status->style->fg_gc[GTK_STATE_NORMAL],
	10, 0, fv->statuslayout );
    pango_layout_index_to_pos(fv->statuslayout,len-1,&pos);
    tlen = (pos.x+pos.width)/1000;
    pango_layout_set_text( fv->statuslayout, buffer+len, -1);
    gdk_draw_layout( GDK_DRAWABLE(fv->status->window), fv->status->style->fg_gc[state],
	10+tlen, 0, fv->statuslayout );
return( true );
}

static void FontView_TextEntry(GtkWidget *widget, gchar *text, FontView *fv) {
    SplineFont *sf = fv->b.sf;
    int enc, i;
    int ch = utf8_ildb((const char **) &text);

    enc = EncFromUni(ch,fv->b.map->enc);
    if ( enc==-1 ) {
	for ( i=0; i<sf->glyphcnt; ++i ) {
	    if ( sf->glyphs[i]!=NULL )
		if ( sf->glyphs[i]->unicodeenc==ch )
	break;
	}
	if ( i!=-1 )
	    enc = fv->b.map->backmap[i];
    }
    if ( enc<fv->b.map->enccount && enc!=-1 )
	FVChangeChar(fv,enc);
}

gboolean FontView_Char(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    int pos, gid;
    FontView *fv = FV_From_Widget(widget);

    if ( gtk_im_context_filter_keypress(fv->imc,event))
return( true );

#if !defined(_NO_FFSCRIPT) || !defined(_NO_PYTHON)
    if ( isdigit(event->keyval) && (event->state&GDK_CONTROL_MASK) &&
	    (event->state&GDK_MOD1_MASK) ) {
	/* The Script menu isn't always up to date, so we might get one of */
	/*  the shortcuts here */
	int index = event->keyval-'1';
	if ( index<0 ) index = 9;
	if ( script_filenames[index]!=NULL )
	    ExecuteScriptFile((FontViewBase *) fv,NULL,script_filenames[index]);
    } else
#endif
    if ( event->keyval == GDK_Left ||
	    event->keyval == GDK_Tab ||
	    /*event->keyval == GDK_BackTab ||*/
	    event->keyval == GDK_Up ||
	    event->keyval == GDK_Right ||
	    event->keyval == GDK_Down ||
	    event->keyval == GDK_KP_Left ||
	    event->keyval == GDK_KP_Up ||
	    event->keyval == GDK_KP_Right ||
	    event->keyval == GDK_KP_Down ||
	    event->keyval == GDK_Home ||
	    event->keyval == GDK_KP_Home ||
	    event->keyval == GDK_End ||
	    event->keyval == GDK_KP_End ||
	    event->keyval == GDK_Page_Up ||
	    event->keyval == GDK_KP_Page_Up ||
	    event->keyval == GDK_Prior ||
	    event->keyval == GDK_Page_Down ||
	    event->keyval == GDK_KP_Page_Down ||
	    event->keyval == GDK_Next ) {
	int end_pos = fv->end_pos;
	/* We move the currently selected char. If there is none, then pick */
	/*  something on the screen */
	if ( end_pos==-1 )
	    end_pos = (fv->rowoff+fv->rowcnt/2)*fv->colcnt;
	switch ( event->keyval ) {
	  case GDK_Tab:
	    pos = end_pos;
	    do {
		if ( event->state&GDK_SHIFT_MASK )
		    --pos;
		else
		    ++pos;
		if ( pos>=fv->b.map->enccount ) pos = 0;
		else if ( pos<0 ) pos = fv->b.map->enccount-1;
	    } while ( pos!=end_pos &&
		    ((gid=fv->b.map->map[pos])==-1 || !SCWorthOutputting(fv->b.sf->glyphs[gid])));
	    if ( pos==end_pos ) ++pos;
	    if ( pos>=fv->b.map->enccount ) pos = 0;
	  break;
	  case GDK_Left: case GDK_KP_Left:
	    pos = end_pos-1;
	  break;
	  case GDK_Right: case GDK_KP_Right:
	    pos = end_pos+1;
	  break;
	  case GDK_Up: case GDK_KP_Up:
	    pos = end_pos-fv->colcnt;
	  break;
	  case GDK_Down: case GDK_KP_Down:
	    pos = end_pos+fv->colcnt;
	  break;
	  case GDK_End: case GDK_KP_End:
	    pos = fv->b.map->enccount;
	  break;
	  case GDK_Home: case GDK_KP_Home:
	    pos = 0;
	    if ( fv->b.sf->top_enc!=-1 && fv->b.sf->top_enc<fv->b.map->enccount )
		pos = fv->b.sf->top_enc;
	    else {
		pos = SFFindSlot(fv->b.sf,fv->b.map,'A',NULL);
		if ( pos==-1 ) pos = 0;
	    }
	  break;
	  case GDK_Page_Up: case GDK_KP_Page_Up:
#if GDK_Prior!=GDK_Page_Up
	  case GDK_Prior:
#endif
	    pos = (fv->rowoff-fv->rowcnt+1)*fv->colcnt;
	  break;
	  case GDK_Page_Down: case GDK_KP_Page_Down:
#if GDK_Next!=GDK_Page_Down
	  case GDK_Next:
#endif
	    pos = (fv->rowoff+fv->rowcnt+1)*fv->colcnt;
	  break;
	}
	if ( pos<0 ) pos = 0;
	if ( pos>=fv->b.map->enccount ) pos = fv->b.map->enccount-1;
	if ( (event->state&GDK_SHIFT_MASK) && event->keyval!=GDK_Tab /*&& event->keyval!=GDK_BackTab*/ ) {
	    FVReselect(fv,pos);
	} else {
	    FVDeselectAll(fv);
	    fv->b.selected[pos] = true;
	    fv->pressed_pos = pos;
	    fv->sel_index = 1;
	    gtk_widget_queue_draw(fv->v);
	}
	fv->end_pos = pos;
	FVShowInfo(fv);
	FVScrollToChar(fv,pos);
    } else if ( event->keyval == GDK_Help ) {
	FontViewMenu_ContextualHelp(NULL,NULL);	/* Menu does F1 */
    } else if ( event->string[0]>=' ' && event->string[1]=='\0' ) {
	/* Should never happen. TextEntry should always be called by the IMC */
	FontView_TextEntry(GTK_WIDGET(fv->imc),event->string,fv);
    } else
return( false );

return( true );
}

static void annot_strncat(char *start, const char *from, int len) {
    char *to = start;
    register unichar_t ch;

    to += strlen(to);
    while ( (ch = *(unsigned char *) from++) != '\0' && to-start<len-4 ) {
	if ( from[-2]=='\t' ) {
	    if ( ch=='*' ) ch = 0x2022;
	    else if ( ch=='x' ) ch = 0x2192;
	    else if ( ch==':' ) ch = 0x224d;
	    else if ( ch=='#' ) ch = 0x2245;
	} else if ( ch=='\t' ) {
	    *(to++) = ' ';
	    ch = ' ';
	}
	to = utf8_idpb(to,ch);
    }
    *to = 0;
}

void SCPreparePopup(GtkTooltips *tip,GtkWidget *v,SplineChar *sc,struct remap *remap, int localenc,
	int actualuni) {
    char space[1610];
    int upos=-1;
    int done = false;

    /* If a glyph is multiply mapped then the built-in unicode enc may not be */
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
	snprintf( space, sizeof(space), "%u 0x%x U+???? \"%.25s\" ", localenc, localenc, sc->name==NULL?"":sc->name );
	done = true;
    }
    if ( done )
	/* Do Nothing */;
    else if ( upos<0x110000 && _UnicodeNameAnnot!=NULL &&
	    _UnicodeNameAnnot[upos>>16][(upos>>8)&0xff][upos&0xff].name!=NULL ) {
	snprintf( space, sizeof(space), "%u 0x%x U+%04x \"%.25s\" %.100s", localenc, localenc, upos, sc->name==NULL?"":sc->name,
		_UnicodeNameAnnot[upos>>16][(upos>>8)&0xff][upos&0xff].name);
    } else if ( upos>=0xAC00 && upos<=0xD7A3 ) {
	snprintf( space, sizeof(space), "%u 0x%x U+%04x \"%.25s\" Hangul Syllable %s%s%s",
		localenc, localenc, upos, sc->name==NULL?"":sc->name,
		chosung[(upos-0xAC00)/(21*28)],
		jungsung[(upos-0xAC00)/28%21],
		jongsung[(upos-0xAC00)%28] );
    } else {
	snprintf( space, sizeof(space), "%u 0x%x U+%04x \"%.25s\" %.50s", localenc, localenc, upos, sc->name==NULL?"":sc->name,
	    	UnicodeRange(upos));
    }
    if ( upos>=0 && upos<0x110000 && _UnicodeNameAnnot!=NULL &&
	    _UnicodeNameAnnot[upos>>16][(upos>>8)&0xff][upos&0xff].annot!=NULL ) {
	int left = sizeof(space)/sizeof(space[0]) - strlen(space)-1;
	if ( left>4 ) {
	    strcat(space,"\n");
	    annot_strncat(space,_UnicodeNameAnnot[upos>>16][(upos>>8)&0xff][upos&0xff].annot,left-2);
	}
    }
    if ( sc->comment!=NULL ) {
	int left = sizeof(space)/sizeof(space[0]) - strlen(space)-1;
	if ( left>4 ) {
	    strcat(space,"\n\n");
	    strncpy(space+strlen(space),sc->comment,left-2);
	}
    }
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tip),v,space,NULL);
}

static gboolean FV_PressedScrollCheck(void *_fv) {
    FontView *fv = _fv;
    gint x,y;
    GdkModifierType mask;

    (void) gdk_window_get_pointer(fv->v->window,&x,&y,&mask);

    if ( y<0 || y >= fv->height ) {
	real dy = 0;
	GtkAdjustment *sb;
	if ( y<0 )
	    dy = -1;
	else if ( y>=fv->height )
	    dy = 1;
	if ( fv->rowoff+dy<0 )
	    dy = 0;
	else if ( fv->rowoff+dy+fv->rowcnt > fv->rowltot )
	    dy = 0;
	fv->rowoff += dy;
	if ( dy!=0 ) {
	    sb = gtk_range_get_adjustment(GTK_RANGE(fv->vsb));
	    sb->value += dy;
	    gtk_range_set_adjustment(GTK_RANGE(fv->vsb),sb);
	    gdk_window_scroll(GDK_WINDOW(fv->v->window),0,dy*fv->cbh);
	}
    }
return( true );
}

gboolean FontViewView_Mouse(GtkWidget *widget,GdkEventButton *event, gpointer user_data) {
    /* I rely on the fact that the event structures for Button and Motion */
    /*  (the two events we might get here) are the same for x,y and state */
    FontView *fv = FV_From_Widget(widget);
    int pos = ((int) event->y/fv->cbh + fv->rowoff)*fv->colcnt + (int) event->x/fv->cbw;
    int gid;
    int realpos = pos;
    SplineChar *sc, dummy;
    int dopopup = true;
    extern int OpenCharsInNewWindow;

    if ( pos<0 ) {
	pos = 0;
	dopopup = false;
    } else if ( pos>=fv->b.map->enccount ) {
	pos = fv->b.map->enccount-1;
	if ( pos<0 )		/* No glyph slots in font */
return(true);
	dopopup = false;
    }

    sc = (gid=fv->b.map->map[pos])!=-1 ? fv->b.sf->glyphs[gid] : NULL;
    if ( sc==NULL )
	sc = SCBuildDummy(&dummy,fv->b.sf,fv->b.map,pos);
    if ( event->type == GDK_BUTTON_PRESS )
	fv->click_count = 1;
    else if ( event->type == GDK_2BUTTON_PRESS )
	fv->click_count = 2;
    else if ( event->type == GDK_3BUTTON_PRESS )
	fv->click_count = 3;
    if ( event->type == GDK_BUTTON_RELEASE && fv->click_count==2 ) {
	if ( fv->pressed_timer_src!=-1 ) {
	    g_source_destroy(g_main_context_find_source_by_id(g_main_context_default(), fv->pressed_timer_src));
	    fv->pressed_timer_src = -1;
	}
	if ( fv->cur_subtable!=NULL ) {
	    sc = FVMakeChar(fv,pos);
	    pos = fv->b.map->backmap[sc->orig_pos];
	}
	if ( sc==&dummy ) {
	    sc = SFMakeChar(fv->b.sf,fv->b.map,pos);
	    gid = fv->b.map->map[pos];
	}
	if ( fv->show==fv->filled ) {
	    SplineFont *sf = fv->b.sf;
	    gid = -1;
	    if ( !OpenCharsInNewWindow )
		for ( gid=sf->glyphcnt-1; gid>=0; --gid )
		    if ( sf->glyphs[gid]!=NULL && sf->glyphs[gid]->views!=NULL )
		break;
	    if ( gid!=-1 ) {
		CharView *cv = (CharView *) sf->glyphs[gid]->views;
		CVChangeSC(cv,sc);
		gdk_window_show(cv->gw->window);	/* Raises it too! */
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
		gdk_window_show(bv->gw->window);	/* Raises it too! */
	    } else
		BitmapViewCreate(bc,bdf,fv,pos);
	}
    }
    if ( event->type >= GDK_BUTTON_PRESS && event->type <= GDK_3BUTTON_PRESS ) {
	/* Drag & Drop end stuff!!!! */
	fv->drag_and_drop = false;
	if ( !(event->state&GDK_SHIFT_MASK) && event->type==GDK_BUTTON_PRESS ) {
	    if ( !fv->b.selected[pos] )
		FVDeselectAll(fv);
	    else if ( event->button!=3 ) {
		/* Drag & Drop start stuff!!! */
		fv->drag_and_drop = fv->has_dd_no_cursor = true;
		fv->any_dd_events_sent = false;
	    }
	}
	fv->pressed_pos = fv->end_pos = pos;
	FVShowInfo(fv);
	if ( !fv->drag_and_drop ) {
	    if ( !(event->state&GDK_SHIFT_MASK))
		fv->sel_index = 1;
	    else if ( fv->sel_index<255 )
		++fv->sel_index;
	    if ( fv->pressed_timer_src!=-1 ) {
		g_source_destroy(g_main_context_find_source_by_id(g_main_context_default(), fv->pressed_timer_src));
		fv->pressed_timer_src = -1;
	    } else if ( event->state&GDK_SHIFT_MASK ) {
		fv->b.selected[pos] = fv->b.selected[pos] ? 0 : fv->sel_index;
		gtk_widget_queue_draw(fv->v);
	    } else if ( !fv->b.selected[pos] ) {
		fv->b.selected[pos] = fv->sel_index;
		gtk_widget_queue_draw(fv->v);
	    }
	    if ( event->button==3 ) {
		static GtkWidget *popup;
		if ( popup == NULL )
		    popup = create_FontViewPopupMenu();
		g_object_set_data( G_OBJECT(popup), "ffdata", fv);
		gtk_menu_popup( GTK_MENU(popup), NULL,NULL,NULL,NULL,
			event->button, event->time);
	    } else
		fv->pressed_timer_src = g_timeout_add(100,FV_PressedScrollCheck,fv);
	}
    } else if ( fv->drag_and_drop ) {
	/* Drag and drop stuff */
    } else if ( fv->pressed_timer_src!=-1 ) {
	int showit = realpos!=fv->end_pos;
	FVReselect(fv,realpos);
	if ( showit )
	    FVShowInfo(fv);
	if ( event->type==GDK_BUTTON_RELEASE ) {
	    g_source_destroy(g_main_context_find_source_by_id(g_main_context_default(), fv->pressed_timer_src));
	    fv->pressed_timer_src = -1;
	}
    }
    if ( dopopup ) {
	int uni = sc==&dummy?dummy.unicodeenc: UniFromEnc(pos,fv->b.map->enc);
	if ( uni==-1 )
	    uni = sc->unicodeenc;	/* For custom encodings */
	if ( fv->last_popup_uni!=uni ) {
	    SCPreparePopup(fv->popupinfo,fv->v,sc,fv->b.map->remap,pos,uni);
	    fv->last_popup_uni = uni;
	}
    } else {
	/* If I disable a tooltip it stays off, even after a reenable, unless */
	/*  the user enters the window again. Which is annoying and none obvious*/
	/* gtk_tooltips_disable(GTK_TOOLTIPS(fv->popupinfo));*/
	gtk_tooltips_set_tip(fv->popupinfo,fv->v,_("Not in font"),NULL);
	
	fv->last_popup_uni = -1;
    }
    if ( event->type==GDK_BUTTON_RELEASE )
	SVAttachFV(fv,2);
return( true );
}

gboolean FontViewView_Resize(GtkWidget *widget, GdkEventConfigure *event,
	gpointer user_data) {
    FontView *fv = (FontView *) g_object_get_data(G_OBJECT(widget),"ffdata");
    extern int default_fv_row_count, default_fv_col_count;
    int topchar;
    int cc,rc;
    GtkAdjustment *sb;

    if ( fv->colcnt!=0 )
	topchar = fv->rowoff*fv->colcnt;
    else if ( fv->b.sf->top_enc!=-1 && fv->b.sf->top_enc<fv->b.map->enccount )
	topchar = fv->b.sf->top_enc;
    else {
	/* Position on 'A' if it exists */
	topchar = SFFindSlot(fv->b.sf,fv->b.map,'A',NULL);
	if ( topchar==-1 ) {
	    for ( topchar=0; topchar<fv->b.map->enccount; ++topchar )
		if ( fv->b.map->map[topchar]!=-1 && fv->b.sf->glyphs[fv->b.map->map[topchar]]!=NULL )
	    break;
	    if ( topchar==fv->b.map->enccount )
		topchar = 0;
	}
    }

    cc = (fv->v->allocation.width+fv->cbw/2)/fv->cbw;
    rc = (fv->v->allocation.height+fv->cbh/2)/fv->cbh;
    if ( cc<=0 ) cc = 1;
    if ( rc<=0 ) rc = 1;

    fv->width = fv->v->allocation.width; fv->height = fv->v->allocation.height;
    fv->colcnt = cc;
    fv->rowcnt = rc;
    fv->rowltot = (fv->b.map->enccount+fv->colcnt-1)/fv->colcnt;

    sb = gtk_range_get_adjustment(GTK_RANGE(fv->vsb));
    sb->lower = 0; sb->upper = fv->rowltot; sb->page_size = fv->rowcnt;
    sb->step_increment = 1; sb->page_increment = fv->rowcnt;

    fv->rowoff = topchar/fv->colcnt;
    if ( fv->rowoff>=fv->rowltot-fv->rowcnt )
        fv->rowoff = fv->rowltot-fv->rowcnt;
    if ( fv->rowoff<0 ) fv->rowoff =0;
    sb->value = fv->rowoff;
    gtk_range_set_adjustment(GTK_RANGE(fv->vsb),sb);

    default_fv_row_count = fv->rowcnt;
    default_fv_col_count = fv->colcnt;
    SavePrefs(true);
return( true );
}

static gboolean FV_ResizeCheck(void *_fv) {
    FontView *fv = _fv;
    int width_off, height_off;
    int r,c;

    fv->resize_timer_src = -1;
    width_off = fv->gw->allocation.width - (fv->v->allocation.width);
    height_off = fv->gw->allocation.height - (fv->v->allocation.height);
    if ( fv->v->allocation.width%fv->cbw!=1 || fv->v->allocation.height%fv->cbh!=1 ) {
	GtkRequisition desired;

	r = (fv->v->allocation.height+(fv->cbh/2))/fv->cbh;
	c = (fv->v->allocation.width+(fv->cbw/2))/fv->cbw;
	gtk_widget_size_request(fv->gw,&desired);
	if ( (desired.width-width_off-1)/fv->cbw > c )
	    c = (desired.width-width_off-1)/fv->cbw;
	if ( (desired.height-height_off-1)/fv->cbh > r )
	    r = (desired.height-height_off-1)/fv->cbh;
	gtk_widget_set_size_request(fv->v,
		c*fv->cbw+1,
		r*fv->cbh+1);
	/* The "set size request" will not force the window to shrink */
	/*  so do a resize */
	gtk_window_resize(GTK_WINDOW(fv->gw),1,1);
    } else {
	/* If we don't do this, then they can't make the window smaller */
	/*  (but, as noted above, this doesn't cause the window to get smaller)*/
	gtk_widget_set_size_request(fv->v, 0, 0);
    }
return 0;		/* No further instances of this timer */
}

gboolean FontView_Resize(GtkWidget *widget, GdkEventConfigure *event,
	gpointer user_data) {
    FontView *fv = (FontView *) g_object_get_data( G_OBJECT(widget), "ffdata" );
    /* When the WM does "smooth resizing" we should wait before doing our */
    /*  size adjustment until the user has stopped moving things aroung */
    /* So add a little delay, and if we get another resize before the delay */
    /*  expires, kill the old, and restart our wait. That seems to deal with */
    /*  smooth resizing */
    if ( fv->resize_timer_src!=-1 )
	g_source_destroy(g_main_context_find_source_by_id(g_main_context_default(), fv->resize_timer_src));
	
    fv->resize_timer_src = g_timeout_add(200,FV_ResizeCheck,fv);

    if ( fv->gc==NULL ) {	/* Can't create a gc until the window exists */
	fv->gc = gdk_gc_new( fv->v->window );
	gdk_gc_copy(fv->gc,fv->v->style->fg_gc[fv->v->state]);
    }

    /* If we return TRUE here, then gtk will NOT do it's own size adjusting */
return 0;
}

void FontView_Realize(GtkWidget *widget, gpointer user_data) {
    FontView *fv = (FontView *) g_object_get_data( G_OBJECT(widget), "ffdata" );

    if ( fv->gc==NULL ) {	/* Can't create a gc until the window exists */
	GdkGC *gc = fv->gw->style->fg_gc[fv->gw->state];
	GdkGCValues values;
	gdk_gc_get_values(gc,&values);
	default_background = COLOR_CREATE(
			     ((values.background.red  &0xff00)>>8),
			     ((values.background.green&0xff00)>>8),
			     ((values.background.blue &0xff00)>>8));

	/* This is a "best guess" which isn't always right */
	/* About 2005 X11 added visuals which supported alpha mode */
	/* They did this by redefining the behavior of a 32 bit visual */
	/* which used to mean 24bit colour and 8 bits padding, but now */
	/* means 24bit color and 8 bits alpha channel. No way for the  */
	/* client to distinquish that I know of. So this check may fail*/
	/* on an old SGI machine */
	display_has_alpha = gdk_visual_get_best_depth() == 32;

	fv->gc = gdk_gc_new( fv->gw->window );
	gdk_gc_copy(fv->gc,fv->gw->style->fg_gc[fv->gw->state]);
    }
}

void FVDelay(FontView *fv,int (*func)(FontView *)) {
    (void) g_timeout_add(200,(int (*)(gpointer))func,fv);
}

void FontView_VScroll(GtkRange *vsb, gpointer user_data) {
    GtkAdjustment *sb;
    FontView *fv = FV_From_Widget(vsb);
    int newpos;

    sb = gtk_range_get_adjustment(GTK_RANGE(vsb));
    newpos = rint(sb->value);
    if ( newpos!=fv->rowoff) {
	int diff = newpos-fv->rowoff;
	fv->rowoff = newpos;
	gdk_window_scroll(GDK_WINDOW(fv->v->window),0,-diff*fv->cbh);
    }
}

static void FontView_ReformatOne(FontView *fv) {
    GtkAdjustment *sb;

    if ( fv->v==NULL || fv->colcnt==0 )	/* Can happen in scripts */
return;

    gdk_window_set_cursor(fv->v->window,ct_watch);
    fv->rowltot = (fv->b.map->enccount+fv->colcnt-1)/fv->colcnt;
    if ( fv->rowoff>fv->rowltot-fv->rowcnt ) {
        fv->rowoff = fv->rowltot-fv->rowcnt;
	if ( fv->rowoff<0 ) fv->rowoff =0;
    }
    sb = gtk_range_get_adjustment(GTK_RANGE(fv->vsb));
    sb->lower = 0; sb->upper = fv->rowltot; sb->page_size = fv->rowcnt;
    sb->step_increment = 1; sb->page_increment = fv->rowcnt;
    sb->value = fv->rowoff;
    gtk_range_set_adjustment(GTK_RANGE(fv->vsb),sb);
    gtk_widget_queue_draw(fv->v);
    gdk_window_set_cursor(fv->v->window,ct_pointer);
}

static void FontView_ReformatAll(SplineFont *sf) {
    BDFFont *new, *old;
    FontView *fvs, *fv;
    MetricsView *mvs;
    extern int use_freetype_to_rasterize_fv;

    if ( ((FontView *) sf->fv)->v==NULL || ((FontView *) sf->fv)->colcnt==0 )			/* Can happen in scripts */
return;

    for ( fvs=(FontView *) sf->fv; fvs!=NULL; fvs=(FontView *) fvs->b.nextsame )
	fvs->touched = false;
    while ( 1 ) {
	for ( fv=(FontView *) sf->fv; fv!=NULL && fv->touched; fv=(FontView *) fv->b.nextsame );
	if ( fv==NULL )
    break;
	old = fv->filled;
				/* In CID fonts fv->b.sf may not be same as sf */
	new = SplineFontPieceMeal(fv->b.sf,fv->b.active_layer,fv->filled->pixelsize,72,
		(fv->antialias?pf_antialias:0)|(fv->bbsized?pf_bbsized:0)|
		    (use_freetype_to_rasterize_fv && !sf->strokedfont && !sf->multilayer?pf_ft_nohints:0),
		NULL);
	for ( fvs=fv; fvs!=NULL; fvs=(FontView *) fvs->b.nextsame )
	    if ( fvs->filled == old ) {
		fvs->filled = new;
		if ( fvs->show == old )
		    fvs->show = new;
		fvs->touched = true;
	    }
	BDFFontFree(old);
    }
    for ( fv=(FontView *) sf->fv; fv!=NULL; fv=(FontView *) fv->b.nextsame ) {
	GtkAdjustment *sb;
	gdk_window_set_cursor(fv->v->window,ct_watch);
	fv->rowltot = (fv->b.map->enccount+fv->colcnt-1)/fv->colcnt;
	if ( fv->rowoff>fv->rowltot-fv->rowcnt ) {
	    fv->rowoff = fv->rowltot-fv->rowcnt;
	    if ( fv->rowoff<0 ) fv->rowoff =0;
	}
	sb = gtk_range_get_adjustment(GTK_RANGE(fv->vsb));
	sb->lower = 0; sb->upper = fv->rowltot; sb->page_size = fv->rowcnt;
	sb->step_increment = 1; sb->page_increment = fv->rowcnt;
	sb->value = fv->rowoff;
	gtk_range_set_adjustment(GTK_RANGE(fv->vsb),sb);
	gtk_widget_queue_draw(fv->v);
	gdk_window_set_cursor(fv->v->window,ct_pointer);
    }
    for ( mvs=sf->metrics; mvs!=NULL; mvs=mvs->next ) if ( mvs->bdf==NULL ) {
	BDFFontFree(mvs->show);
	mvs->show = SplineFontPieceMeal(sf,mvs->layer,mvs->ptsize,mvs->dpi,
                mvs->antialias?(pf_antialias|pf_ft_recontext):pf_ft_recontext,NULL);
	gtk_widget_queue_draw(mvs->gw);
    }
}

static void FontViewOpenKids(FontView *fv) {
    int k, i;
    SplineFont *sf = fv->b.sf, *_sf;

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

static FontView *__FontViewCreate(SplineFont *sf) {
    FontView *fv = calloc(1,sizeof(FontView));
    int i;
    int ps = sf->display_size<0 ? -sf->display_size :
	     sf->display_size==0 ? default_fv_font_size : sf->display_size;

    if ( ps>200 ) ps = 128;

    fv->b.nextsame = sf->fv;
    fv->b.active_layer = sf->display_layer;
    sf->fv = (FontViewBase *) fv;
    fv->b.next = (FontViewBase *) fv_list;
    fv_list = fv;
    if ( sf->mm!=NULL ) {
	sf->mm->normal->fv = (FontViewBase *) fv;
	for ( i = 0; i<sf->mm->instance_count; ++i )
	    sf->mm->instances[i]->fv = (FontViewBase *) fv;
    }
    if ( sf->subfontcnt==0 ) {
	fv->b.sf = sf;
	if ( fv->b.nextsame!=NULL ) {
	    fv->b.map = EncMapCopy(fv->b.nextsame->map);
	    fv->b.normal = fv->b.nextsame->normal==NULL ? NULL : EncMapCopy(fv->b.nextsame->normal);
	} else if ( sf->compacted ) {
	    fv->b.normal = sf->map;
	    fv->b.map = CompactEncMap(EncMapCopy(sf->map),sf);
	} else {
	    fv->b.map = sf->map;
	    fv->b.normal = NULL;
	}
    } else {
	fv->b.cidmaster = sf;
	for ( i=0; i<sf->subfontcnt; ++i )
	    sf->subfonts[i]->fv = (FontViewBase *) fv;
	for ( i=0; i<sf->subfontcnt; ++i )	/* Search for a subfont that contains more than ".notdef" (most significant in .gai fonts) */
	    if ( sf->subfonts[i]->glyphcnt>1 ) {
		fv->b.sf = sf->subfonts[i];
	break;
	    }
	if ( fv->b.sf==NULL )
	    fv->b.sf = sf->subfonts[0];
	sf = fv->b.sf;
	if ( fv->b.nextsame==NULL ) EncMapFree(sf->map);
	fv->b.map = EncMap1to1(sf->glyphcnt);
    }
    fv->b.selected = calloc(fv->b.map->enccount,sizeof(char));
    fv->user_requested_magnify = -1;
    fv->magnify = (ps<=9)? 3 : (ps<20) ? 2 : 1;
    fv->cbw = (ps*fv->magnify)+1;
    fv->cbh = (ps*fv->magnify)+1+fv->lab_height+1;
    fv->antialias = sf->display_antialias;
    fv->bbsized = sf->display_bbsized;
    fv->glyphlabel = default_fv_glyphlabel;

    fv->resize_timer_src = -1;
    fv->pressed_timer_src = -1;

    fv->end_pos = -1;
#ifndef _NO_PYTHON
    PyFF_InitFontHook((FontViewBase *) fv);
#endif
return( fv );
}

static void FontViewInit(void) {
}

static FontView *FontView_Create(SplineFont *sf, int hide) {
    FontView *fv = __FontViewCreate(sf);
    static int done = false;
    PangoContext *context;
    PangoFont *font;
    PangoFontMetrics *fm;
    int as, ds;
    BDFFont *bdf;
    extern int use_freetype_to_rasterize_fv;

    if ( !done ) { FontViewInit(); done=true;}

    fv->gw = create_FontView();
    fv->fontset = calloc(_uni_fontmax,sizeof(PangoFont *));
    g_object_set_data (G_OBJECT (fv->gw), "ffdata", fv );
    fv->v = lookup_widget( fv->gw,"view" );
    fv->status = lookup_widget( fv->gw,"status" );
    fv->vsb    = lookup_widget( fv->gw,"vscrollbar2" );
    context = gtk_widget_get_pango_context( fv->status );
    font = pango_context_load_font( context, pango_context_get_font_description(context));
    fm = pango_font_get_metrics(font,NULL);
    as = pango_font_metrics_get_ascent(fm);
    ds = pango_font_metrics_get_descent(fm);
    fv->infoh = (as+ds)/1024;
    gtk_widget_set_size_request(fv->status,0,fv->infoh);
    gtk_widget_set_size_request(fv->v,sf->desired_col_cnt*fv->cbw+1,sf->desired_row_cnt*fv->cbh+1);

    context = gtk_widget_get_pango_context( fv->v );
    fv->fontset[0] = pango_context_get_font_description(context);
    /*g_object_ref(G_OBJECT(fv->fontset[0]));*/ /* Not an object, so can't incr its ref */
    font = pango_context_load_font( context, fv->fontset[0]);
    fm = pango_font_get_metrics(font,NULL);
    as = pango_font_metrics_get_ascent(fm);
    ds = pango_font_metrics_get_descent(fm);
    fv->lab_height = (as+ds)/1024;
    fv->lab_as = as/1024;

    g_object_set_data( G_OBJECT(fv->gw), "ffdata", fv );
    g_object_set_data( G_OBJECT(fv->v) , "ffdata", fv );

#ifndef _NO_PYTHON
    PyFF_BuildFVToolsMenu(fv,GTK_MENU_ITEM(lookup_widget(fv->gw,"tools3")));
#endif

    fv->showhmetrics = default_fv_showhmetrics;
    fv->showvmetrics = default_fv_showvmetrics && sf->hasvmetrics;
    if ( fv->b.nextsame!=NULL ) {
	fv->filled = ((FontView *) fv->b.nextsame)->filled;
	bdf = ((FontView *) fv->b.nextsame)->show;
    } else {
	bdf = SplineFontPieceMeal(fv->b.sf,fv->b.active_layer,sf->display_size<0?-sf->display_size:default_fv_font_size,72,
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

    fv->popupinfo = /*lookup_widget( fv->gw, "tooltips")*/ gtk_tooltips_new();
    gtk_tooltips_set_tip( GTK_TOOLTIPS(fv->popupinfo), fv->v, "", NULL );
    /*gtk_tooltips_disable( GTK_TOOLTIPS(fv->popupinfo));*/

    fv->imc = gtk_im_multicontext_new();
    gtk_im_context_set_use_preedit(fv->imc,FALSE);	/* Context should draw in its own window */
    gtk_im_context_set_client_window(fv->imc,fv->gw->window);
    g_signal_connect(G_OBJECT(fv->imc),"commit", G_CALLBACK(FontView_TextEntry), fv );

    fv->vlayout = gtk_widget_create_pango_layout( fv->v, NULL );
    fv->statuslayout = gtk_widget_create_pango_layout( fv->status, NULL );
#ifdef _NO_PYTHON
    /* Hide the tools menu if no python */
    gtk_widget_hide(lookup_widget( GTK_WIDGET(fv->gw), "tools3" ));
#endif
#if defined(_NO_PYTHON) && defined(_NO_FFSCRIPT)
    gtk_widget_hide(lookup_widget( GTK_WIDGET(fv->gw), "execute_script1" ));
#endif
#if defined(_NO_FFSCRIPT)
    gtk_widget_hide(lookup_widget( GTK_WIDGET(fv->gw), "script_menu1" ));
#endif
#ifndef FONTFORGE_CONFIG_TILEPATH
    gtk_widget_hide(lookup_widget( GTK_WIDGET(fv->gw), "tilepath1" ));
#endif

    FVSetTitle( (FontViewBase *) fv);

    gtk_widget_show(fv->gw);
    FontViewOpenKids(fv);
return( fv );
}

static FontView *FontView_Append(FontView *fv) {
    /* Normally fontviews get added to the fv list when their windows are */
    /*  created. but we don't create any windows here, so... */
    FontView *test;

    if ( fv_list==NULL ) fv_list = fv;
    else {
	for ( test = fv_list; test->b.next!=NULL; test=(FontView *) test->b.next );
	test->b.next = (FontViewBase *) fv;
    }
return( fv );
}

FontView *FontNew(void) {
return( FontView_Create(SplineFontNew(), false));
}

static void FontView_Free(FontView *fv) {
    int i;
    FontView *prev;
    FontView *fvs;

    if ( fv->b.sf == NULL )	/* Happens when usurping a font to put it into an MM */
	BDFFontFree(fv->filled);
    else if ( fv->b.nextsame==NULL && fv->b.sf->fv==(FontViewBase *) fv ) {
	EncMapFree(fv->b.map);
	SplineFontFree(fv->b.cidmaster?fv->b.cidmaster:fv->b.sf);
	BDFFontFree(fv->filled);
    } else {
	EncMapFree(fv->b.map);
	for ( fvs=(FontView *) fv->b.sf->fv, i=0 ; fvs!=NULL; fvs = (FontView *) fvs->b.nextsame )
	    if ( fvs->filled==fv->filled ) ++i;
	if ( i==1 )
	    BDFFontFree(fv->filled);
	if ( fv->b.sf->fv==(FontViewBase *) fv ) {
	    if ( fv->b.cidmaster==NULL )
		fv->b.sf->fv = fv->b.nextsame;
	    else {
		fv->b.cidmaster->fv = fv->b.nextsame;
		for ( i=0; i<fv->b.cidmaster->subfontcnt; ++i )
		    fv->b.cidmaster->subfonts[i]->fv = fv->b.nextsame;
	    }
	} else {
	    for ( prev = (FontView *) fv->b.sf->fv; prev->b.nextsame!=(FontViewBase *)fv;
		    prev=(FontView *) prev->b.nextsame );
	    prev->b.nextsame = fv->b.nextsame;
	}
    }
    free(fv->b.selected);
    for ( i=0; i<_uni_fontmax; ++i )
	if ( fv->fontset[i]!=NULL )
	    g_object_unref(G_OBJECT(fv->fontset[i]));
    free(fv->fontset);
    if ( fv->imc!=NULL ) {
	g_object_unref((GObject *) fv->vlayout );
	g_object_unref((GObject *) fv->statuslayout );
	g_object_unref((GObject *) fv->char_slot);
	g_object_unref((GObject *) fv->imc);
	g_object_unref((GObject *) fv->popupinfo);
    }
#ifndef _NO_FFSCRIPT
    DictionaryFree(fv->b.fontvars);
    free(fv->b.fontvars);
#endif
#ifndef _NO_PYTHON
    PyFF_FreeFV((FontViewBase *) fv);
#else
    { GtkWidget *w;
    w = lookup_widget(fv->gw,"tools1");
    if ( w!=NULL )
	gtk_widget_set_sensitive(w,false);
    }
#endif
    free(fv);
}

static int FontViewWinInfo(FontView *fv, int *cc, int *rc) {
    if ( fv==NULL || fv->colcnt==0 || fv->rowcnt==0 ) {
	*cc = 16; *rc = 4;
return( -1 );
    }

    *cc = fv->colcnt;
    *rc = fv->rowcnt;

return( fv->rowoff*fv->colcnt );
}

static FontViewBase *FVAny(void) { return (FontViewBase *) fv_list; }

static int  FontIsActive(SplineFont *sf) {
    FontView *fv;

    for ( fv=fv_list; fv!=NULL; fv=(FontView *) (fv->b.next) )
	if ( fv->b.sf == sf )
return( true );

return( false );
}

static SplineFont *FontOfFilename(const char *filename) {
    char buffer[1025];
    FontView *fv;

    GFileGetAbsoluteName((char *) filename,buffer,sizeof(buffer)); 
    for ( fv=fv_list; fv!=NULL ; fv=(FontView *) (fv->b.next) ) {
	if ( fv->b.sf->filename!=NULL && strcmp(fv->b.sf->filename,buffer)==0 )
return( fv->b.sf );
	else if ( fv->b.sf->origname!=NULL && strcmp(fv->b.sf->origname,buffer)==0 )
return( fv->b.sf );
    }
return( NULL );
}

static void FVExtraEncSlots(FontView *fv, int encmax) {
    if ( fv->colcnt!=0 ) {		/* Ie. scripting vs. UI */
	GtkAdjustment *sb;
	fv->rowltot = (encmax+1+fv->colcnt-1)/fv->colcnt;
	sb = gtk_range_get_adjustment(GTK_RANGE(fv->vsb));
	sb->upper = fv->rowltot;
	gtk_range_set_adjustment(GTK_RANGE(fv->vsb),sb);
    }
}

static void FV_BiggerGlyphCache(FontView *fv, int encmax) {
    if ( fv->filled!=NULL )
	BDFOrigFixup(fv->filled,encmax,fv->b.sf);
}

static void FontView_Close(FontView *fv) {
    if ( fv->gw!=NULL )
	gtk_widget_destroy(fv->gw);
    else {
	if ( fv_list==fv )
	    fv_list = (FontView *) fv->b.next;
	else {
	    FontViewBase *n;
	    for ( n=(FontViewBase *) fv_list; n->next!=(FontViewBase *) fv; n=n->next );
	    n->next = fv->b.next;
	}
	FontViewFree((FontViewBase *) fv);
    }
}


struct fv_interface gtk_fv_interface = {
    (FontViewBase *(*)(SplineFont *, int)) FontView_Create,
    (FontViewBase *(*)(SplineFont *)) __FontViewCreate,
    (void (*)(FontViewBase *)) FontView_Close,
    (void (*)(FontViewBase *)) FontView_Free,
    (void (*)(FontViewBase *)) FontViewSetTitle,
    FontViewSetTitles,
    FontViewRefreshAll,
    (void (*)(FontViewBase *)) FontView_ReformatOne,
    FontView_ReformatAll,
    (void (*)(FontViewBase *)) FV_LayerChanged,
    FV_ToggleCharChanged,
    (int  (*)(FontViewBase *, int *, int *)) FontViewWinInfo,
    FontIsActive,
    FVAny,
    (FontViewBase *(*)(FontViewBase *)) FontView_Append,
    FontOfFilename,
    (void (*)(FontViewBase *,int)) FVExtraEncSlots,
    (void (*)(FontViewBase *,int)) FV_BiggerGlyphCache,
    (void (*)(FontViewBase *,BDFFont *)) FV_ChangeDisplayBitmap,
    (void (*)(FontViewBase *)) FV_ShowFilled,
    FV_ReattachCVs,
    (void (*)(FontViewBase *)) FVDeselectAll,
    (void (*)(FontViewBase *,int )) FVScrollToGID,
    (void (*)(FontViewBase *,int )) FVScrollToChar,
    (void (*)(FontViewBase *,int )) FV_ChangeGID,
    SF_CloseAllInstrs
};
