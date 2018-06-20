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
#include "namelist.h"
#include "scripting.h"

#include <stdlib.h>
#include <string.h>
#include <ustring.h>
#include <gdraw.h>
#include <gwidget.h>
#include <ggadget.h>

extern NameList *force_names_when_opening;
int default_font_filter_index=0;
struct openfilefilters *user_font_filters = NULL;

#ifdef FONTFORGE_CAN_USE_WOFF2
/* GGadgetWildMatch is buggy... it must be woff2 then woff */
#  define WOFF_EXT ",woff2,woff"
#else
#  define WOFF_EXT ",woff"
#endif

struct openfilefilters def_font_filters[] = {
    {
        N_("All Fonts"),
        /* any these files... */
        "*.{"
	   "pfa,"
	   "pfb,"
	   "pt3,"
	   "t42,"
	   "sfd,"
	   "ufo,"
	   "ttf,"
	   "bdf,"
	   "otf,"
	   "otb,"
	   "cff,"
	   "cef,"
	   "gai,"
	   "svg,"
	   "ufo,"
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
	   "pcf,"
	   "pmf,"
	   "[0-9]*pk,"
/* I used to say "*gf" but that also matched xgf (xgridfit) files -- which ff can't open */
	   "[0-9]*gf,"
	   "pdb"
	   WOFF_EXT
        "}"
        /* With any of these methods of compression */
	"{.gz,.Z,.bz2,.lzma,}"
    },
    {
	N_("Outline Fonts"),
	"*.{"
	   "pfa,"
	   "pfb,"
	   "pt3,"
	   "t42,"
	   "sfd,"
	   "ufo,"
	   "ttf,"
	   "otf,"
	   "cff,"
	   "cef,"
	   "gai,"
	   "svg,"
	   "ufo,"
	   "pf3,"
	   "ttc,"
	   "gsf,"
	   "cid,"
	   "bin,"
	   "hqx,"
	   "dfont,"
	   "mf,"
	   "ik"
	   WOFF_EXT
	"}"
	"{.gz,.Z,.bz2,.lzma,}"
    },
    {
	N_("Bitmap Fonts"),
	"*.{"
	   "bdf,"
	   "otb,"
	   "bin,"
	   "hqx,"
	   "fon,"
	   "fnt,"
	   "pcf,"
	   "pmf,"
	   "*pk,"
	   "*gf,"
	   "pdb"
	"}"
	"{.gz,.Z,.bz2,.lzma,}"
    },
    { NU_("ΤεΧ Bitmap Fonts"), "*{pk,gf}" },
    { N_("PostScript"), "*.{pfa,pfb,t42,otf,cef,cff,gai,pf3,pt3,gsf,cid}{.gz,.Z,.bz,.bz2,.lzma,}" },
    { N_("TrueType"), "*.{ttf,t42,ttc}{.gz,.Z,.bz,.bz2,.lzma,}" },
    { N_("OpenType"), "*.{ttf,otf" WOFF_EXT "}{.gz,.Z,.bz,.bz2,.lzma,}" },
    { N_("Type1"), "*.{pfa,pfb,gsf,cid}{.gz,.Z,.bz2,.lzma,}" },
    { N_("Type2"), "*.{otf,cef,cff,gai}{.gz,.Z,.bz2,.lzma,}" },
    { N_("Type3"), "*.{pf3,pt3}{.gz,.Z,.bz2,.lzma,}" },
    { N_("SVG"), "*.svg{.gz,.Z,.bz2,.lzma,}" },
    { N_("Unified Font Object"), "*.ufo" },
    { N_("FontForge's SFD"), "*.sfd{.gz,.Z,.bz2,.lzma,}" },
    { N_("Backup SFD"), "*.sfd~" },
    { N_("Extract from PDF"), "*.pdf{.gz,.Z,.bz2,.lzma,}" },
    { "-", NULL },
    { N_("Archives"), "*.{zip,tgz,tbz,tbz2,tar.gz,tar.bz,tar.bz2,tar}" },
    { N_("All Files"), "*" },
    { NULL, NULL }
};

static GTextInfo **StandardFilters(void) {
    int k, cnt, i;
    GTextInfo **ti;

    /* Make two passes thru outer loop. The first pass determines the
     * interim count of how many entries will be generated and then
     * allocates an array with 3 entries more than that. The second 
     * pass ( if(k) ) accumulates the data values, then adds a trailer,
     * to return in the GTextInfo** structure 'ti'.
     */
    for ( k=0; k<2; ++k ) {
	cnt = 0;
	for ( i=0; def_font_filters[i].name!=NULL; ++i ) {
	    if ( k ) {
		ti[cnt] = calloc(1,sizeof(GTextInfo));
		ti[cnt]->userdata = def_font_filters[i].filter;
		ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
		if ( *(char *) def_font_filters[i].name == '-' )
		    ti[cnt]->line = true;
		else
		    ti[cnt]->text = utf82u_copy(_(def_font_filters[i].name));
	    }
	    ++cnt;
	}
	if ( user_font_filters!=NULL ) {
	    if ( k ) {
		ti[cnt] = calloc(1,sizeof(GTextInfo));
		ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
		/* Don't translate this name */
		ti[cnt]->line = true;
	    }
	    ++cnt;
	    for ( i=0; user_font_filters[i].name!=NULL; ++i ) {
		if ( k ) {
		    ti[cnt] = calloc(1,sizeof(GTextInfo));
		    ti[cnt]->userdata = user_font_filters[i].filter;
		    ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
		    /* Don't translate this name */
		    if ( *(char *) user_font_filters[i].name == '-' )
			ti[cnt]->line = true;
		    else
			ti[cnt]->text = utf82u_copy(user_font_filters[i].name);
		}
		++cnt;
	    }
	}
	if ( k ) {
	    ti[cnt] = calloc(1,sizeof(GTextInfo));
	    ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
	    ti[cnt++]->line = true;
	    ti[cnt] = calloc(1,sizeof(GTextInfo));
	    ti[cnt]->userdata = (void *) -1;
	    ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
	    ti[cnt++]->text = utf82u_copy(_("Edit Filter List"));
	    ti[cnt] = calloc(1,sizeof(GTextInfo));
	} else
	    ti = malloc((cnt+3)*sizeof(GTextInfo *));
    }
    ti[default_font_filter_index]->selected = true;
return( ti );
}

struct filter_d {
    int done;
    GGadget *gme;
};

static int Filter_Cancel(GGadget *g, GEvent *e) {
    GWindow gw;
    struct filter_d *d;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	d->done = true;
    }
return( true );
}

static int Filter_OK(GGadget *g, GEvent *e) {
    struct filter_d *d;
    struct matrix_data *md;
    int rows,i,cnt;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	if ( user_font_filters!=NULL ) {
	    for ( i=0; user_font_filters[i].name!=NULL; ++i ) {
		free(user_font_filters[i].name);
		free(user_font_filters[i].filter);
	    }
	    free(user_font_filters);
	    user_font_filters = NULL;
	}
	d = GDrawGetUserData(GGadgetGetWindow(g));
	md = GMatrixEditGet(d->gme,&rows);
	for ( i=cnt=0; i<rows; ++i )
	    if ( !md[2*i].frozen )
		++cnt;
	if ( cnt!=0 ) {
	    user_font_filters = malloc((cnt+1)*sizeof(struct openfilefilters));
	    for ( i=cnt=0; i<rows; ++i ) if ( !md[2*i].frozen ) {
		user_font_filters[cnt].name = copy(md[2*i].u.md_str);
		user_font_filters[cnt].filter = copy(md[2*i+1].u.md_str);
		++cnt;
	    }
	    user_font_filters[cnt].name = user_font_filters[cnt].filter = NULL;
	}
	SavePrefs(true);
	d->done = true;
    }
return( true );
}

static int filter_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct filter_d *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

static int filter_candelete(GGadget *g, int r) {
    struct matrix_data *md;
    int rows;

    md = GMatrixEditGet(g,&rows);
    if ( r>=rows )
return( false );

return( !md[2*r].frozen );
}

static void FilterDlg(void) {
    static struct col_init cols[] = {
	{ me_string, NULL, NULL, NULL, N_("Name") },
	{ me_string, NULL, NULL, NULL, N_("Filter") }
    };
    static int inited = false;
    static struct matrixinit mi = {
	2, cols,
	0, NULL,
	NULL,
	filter_candelete,
	NULL,
	NULL,
	NULL,
	NULL
    };
    struct matrix_data *md;
    int k, cnt, i, ptwidth;
    GGadgetCreateData gcd[3], boxes[3], *varray[7], *harray[7];
    GTextInfo label[3];
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    struct filter_d d;

    if ( !inited ) {
	inited = true;
	cols[0].title = _(cols[0].title);
	cols[1].title = _(cols[1].title);
    }
    
    for ( k=0; k<2; ++k ) {
	cnt = 0;
	for ( i=0; def_font_filters[i].name!=NULL; ++i ) {
	    if ( *(char *) def_font_filters[i].name != '-' ) {
		if ( k ) {
		    md[2*cnt].u.md_str = copy(_(def_font_filters[i].name));
		    md[2*cnt].frozen = true;
		    md[2*cnt+1].u.md_str = copy(def_font_filters[i].filter);
		    md[2*cnt+1].frozen = true;
		}
		++cnt;
	    }
	}
	if ( user_font_filters!=NULL ) {
	    for ( i=0; user_font_filters[i].name!=NULL; ++i ) {
		if ( *(char *) user_font_filters[i].name != '-' ) {
		    if ( k ) {
			md[2*cnt].u.md_str = copy(user_font_filters[i].name);
			md[2*cnt].frozen = false;
			md[2*cnt+1].u.md_str = copy(user_font_filters[i].filter);
			md[2*cnt+1].frozen = false;
		    }
		    ++cnt;
		}
	    }
	}
	if ( !k )
	    md = calloc(2*cnt,sizeof(struct matrix_data));
    }
    mi.initial_row_cnt = cnt;
    mi.matrix_data = md;


    memset(&d,'\0',sizeof(d));

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = true;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Edit Font Filters");
    pos.x = pos.y = 0;
    ptwidth = 2*GIntGetResource(_NUM_Buttonsize)+GGadgetScale(60);
    pos.width =GDrawPointsToPixels(NULL,ptwidth);
    pos.height = GDrawPointsToPixels(NULL,90);
    gw = GDrawCreateTopWindow(NULL,&pos,filter_e_h,&d,&wattrs);


    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    gcd[0].gd.pos.x = 10; gcd[0].gd.pos.y = 6;
    gcd[0].gd.pos.width = 300; gcd[0].gd.pos.height = 200;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].creator = GMatrixEditCreate;
    gcd[0].gd.u.matrix = &mi;
    varray[0] = &gcd[0]; varray[1] = NULL;

    gcd[1].gd.pos.x = 20-3; gcd[1].gd.pos.y = 90-35-3;
    gcd[1].gd.pos.width = -1; gcd[1].gd.pos.height = 0;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[1].text = (unichar_t *) _("_OK");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.handle_controlevent = Filter_OK;
    gcd[1].creator = GButtonCreate;
    harray[0] = GCD_Glue; harray[1] = &gcd[1]; harray[2] = GCD_Glue;

    gcd[2].gd.pos.x = -20; gcd[2].gd.pos.y = 90-35;
    gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
    gcd[2].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[2].text = (unichar_t *) _("_Cancel");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.handle_controlevent = Filter_Cancel;
    gcd[2].creator = GButtonCreate;
    harray[3] = GCD_Glue; harray[4] = &gcd[2]; harray[5] = GCD_Glue;
    harray[6] = NULL;
    varray[2] = &boxes[2]; varray[3] = NULL;
    varray[4] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GHVGroupCreate;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray;
    boxes[2].creator = GHBoxCreate;


    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,0);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);
    GMatrixEditSetNewText(gcd[0].ret,S_("Filter|New"));
    d.gme = gcd[0].ret;
    
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);

    for ( i=0; i<cnt; ++i ) {
	free(md[2*i].u.md_str);
	free(md[2*i+1].u.md_str);
    }
    free(md);
}

struct gfc_data {
    int done;
    unichar_t *ret;
    GGadget *gfc;
    GGadget *rename;
    int filename_popup_pos;
    unichar_t *lastpopupfontname;
};

static int GFD_Ok(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *tf;
	GFileChooserGetChildren(d->gfc,NULL,NULL,&tf);
	if ( *_GGadgetGetTitle(tf)!='\0' ) {
	    extern int allow_utf8_glyphnames;
	    GTextInfo *ti = GGadgetGetListItemSelected(d->rename);
	    char *nlname = u2utf8_copy(ti->text);
	    force_names_when_opening = NameListByName(nlname);
	    free(nlname);
	    if ( force_names_when_opening!=NULL && force_names_when_opening->uses_unicode &&
		    !allow_utf8_glyphnames) {
		ff_post_error(_("Namelist contains non-ASCII names"),_("Glyph names should be limited to characters in the ASCII character set, but there are names in this namelist which use characters outside that range."));
return(true);
	    }
	    d->done = true;
	    d->ret = GGadgetGetTitle(d->gfc);

	    // Trim trailing '/' if its there and put that string back as
	    // the d->gfc string.
	    int tmplen = u_strlen( d->ret );
	    if( tmplen > 0 ) {
		if( d->ret[ tmplen-1 ] == '/' ) {
		    unichar_t* tmp = u_copy( d->ret );
		    tmp[ tmplen-1 ] = '\0';
		    GGadgetSetTitle(d->gfc, tmp);
		    free(tmp);
		    d->ret = GGadgetGetTitle(d->gfc);
		}
	    }
	}
    }
return( true );
}

static int GFD_New(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = true;
	GDrawSetVisible(GGadgetGetWindow(g),false);
	FontNew();
    }
return( true );
}

static int GFD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = true;
    }
return( true );
}

static int GFD_FilterSelected(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GTextInfo *ti = GGadgetGetListItemSelected(g);
	if ( ti->userdata==NULL )
	    /* They selected a line. Dull */;
	else if ( ti->userdata == (void *) -1 ) {
	    FilterDlg();
	    GGadgetSetList(g,StandardFilters(),true);
	} else {
	    unichar_t *temp = utf82u_copy(ti->userdata);
	    GFileChooserSetFilterText(d->gfc,temp);
	    free(temp);
	    temp = GFileChooserGetDir(d->gfc);
	    GFileChooserSetDir(d->gfc,temp);
	    free(temp);
	    default_font_filter_index = GGadgetGetFirstListSelectedItem(g);
	    SavePrefs(true);
	}
    }
return( true );
}

static int WithinList(struct gfc_data *d,GEvent *event) {
    GRect size;
    GGadget *list;
    int32 pos;
    unichar_t *ufile;
    char *file, **fontnames;
    int cnt, len;
    unichar_t *msg;

    if ( event->type!=et_mousemove )
return( false );

    GFileChooserGetChildren(d->gfc,NULL, &list, NULL);
    if ( list==NULL )
return( false );
    if ( !GGadgetWithin(list,event->u.mouse.x,event->u.mouse.y) )
        return( false );
    pos = GListIndexFromY(list,event->u.mouse.y);
    if ( pos == d->filename_popup_pos )
return( pos!=-1 );
    if ( pos==-1 || GFileChooserPosIsDir(d->gfc,pos)) {
	d->filename_popup_pos = -1;
return( pos!=-1 );
    }
    ufile = GFileChooserFileNameOfPos(d->gfc,pos);
    if ( ufile==NULL )
return( true );
    file = u2def_copy(ufile);
    free(ufile);

    fontnames = GetFontNames(file, 0);
    if ( fontnames==NULL || fontnames[0]==NULL )
	msg = uc_copy( "???" );
    else {
	len = 0;
	for ( cnt=0; fontnames[cnt]!=NULL; ++cnt )
	    len += strlen(fontnames[cnt])+1;
	msg = malloc((len+2)*sizeof(unichar_t));
	len = 0;
	for ( cnt=0; fontnames[cnt]!=NULL; ++cnt ) {
	    uc_strcpy(msg+len,fontnames[cnt]);
	    len += strlen(fontnames[cnt]);
	    msg[len++] = '\n';
	}
	msg[len-1] = '\0';
    }
    GGadgetPreparePopup(GGadgetGetWindow(d->gfc),msg);
    if ( fontnames!=NULL ) {
        for ( cnt=0; fontnames[cnt]!=NULL; ++cnt ) {
            free(fontnames[cnt]);
        }
        free(fontnames);
    }
    free(file);
    free(d->lastpopupfontname);
    d->lastpopupfontname = msg;
return( true );
}

static int e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct gfc_data *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_mousemove ||
	    (event->type==et_mousedown && event->u.mouse.button==3 )) {
	struct gfc_data *d = GDrawGetUserData(gw);
	if ( !WithinList(d,event) )
	    GFileChooserPopupCheck(d->gfc,event);
    } else if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
	struct gfc_data *d = GDrawGetUserData(gw);
return( GGadgetDispatchEvent((GGadget *) (d->gfc),event));
    } else if ( event->type == et_resize ) {
	GRect r, size;
	struct gfc_data *d = GDrawGetUserData(gw);
	if ( d->gfc!=NULL ) {
	    GDrawGetSize(gw,&size);
	    GGadgetGetSize(d->gfc,&r);
	    GGadgetResize(d->gfc,size.width-2*r.x,r.height);
	}
    }
return( event->type!=et_char );
}

unichar_t *FVOpenFont(char *title, const char *defaultfile, int mult) {
    GRect pos;
    int i, filter, renamei;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[11], boxes[5], *varray[9], *harray1[7], *harray2[4], *harray3[9];
    GTextInfo label[10];
    struct gfc_data d;
    int bs = GIntGetResource(_NUM_Buttonsize), bsbigger, totwid, spacing;
    GGadget *tf;
    unichar_t *temp;
    char **nlnames;
    GTextInfo *namelistnames, **filts;
    int cnt;

    memset(&d,'\0',sizeof(d));

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = true;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = title;
    pos.x = pos.y = 0;

    totwid = GGadgetScale(295);
    bsbigger = 4*bs+4*14>totwid; totwid = bsbigger?4*bs+4*12:totwid;
    spacing = (totwid-4*bs-2*12)/3;

    pos.width = GDrawPointsToPixels(NULL,totwid);
    pos.height = GDrawPointsToPixels(NULL,247);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));
    i=0;
    gcd[i].gd.pos.x = 12; gcd[i].gd.pos.y = 6; gcd[i].gd.pos.width = totwid*100/GIntGetResource(_NUM_ScaleFactor)-24; gcd[i].gd.pos.height = 180;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    if ( RecentFiles[0]!=NULL )
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_file_pulldown;
    if ( mult )
	gcd[i].gd.flags |= gg_file_multiple;
    varray[0] = &gcd[i]; varray[1] = NULL;
    gcd[i++].creator = GFileChooserCreate;

    label[i].text = (unichar_t *) _("Filter:");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 8; gcd[i].gd.pos.y = 188+6;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[i].gd.popup_msg = (unichar_t *) _("Display files of this type" );
    harray1[0] = GCD_Glue; harray1[1] = &gcd[i];
    gcd[i++].creator = GLabelCreate;

    gcd[i].gd.pos.x = 0; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y-6;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[i].gd.popup_msg = (unichar_t *) _("Display files of this type");
    gcd[i].gd.handle_controlevent = GFD_FilterSelected;
    harray1[2] = &gcd[i]; harray1[3] = GCD_Glue; harray1[4] = GCD_Glue; harray1[5] = GCD_Glue; harray1[6] = NULL;
    gcd[i++].creator = GListButtonCreate;

    boxes[2].gd.flags = gg_visible | gg_enabled;
    boxes[2].gd.u.boxelements = harray1;
    boxes[2].creator = GHBoxCreate;
    varray[2] = &boxes[2]; varray[3] = NULL;

    label[i].text = (unichar_t *) _("Force glyph names to:");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 8; gcd[i].gd.pos.y = 188+6;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[i].gd.popup_msg = (unichar_t *) _("In the saved font, force all glyph names to match those in the specified namelist");
    harray2[0] = &gcd[i];
    gcd[i++].creator = GLabelCreate;

    renamei = i;
    gcd[i].gd.pos.x = 0; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y-6;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[i].gd.popup_msg = (unichar_t *) _("In the saved font, force all glyph names to match those in the specified namelist");
    gcd[i].creator = GListButtonCreate;
    nlnames = AllNamelistNames();
    for ( cnt=0; nlnames[cnt]!=NULL; ++cnt);
    namelistnames = calloc(cnt+3,sizeof(GTextInfo));
    namelistnames[0].text = (unichar_t *) _("No Rename");
    namelistnames[0].text_is_1byte = true;
    if ( force_names_when_opening==NULL ) {
	namelistnames[0].selected = true;
	gcd[i].gd.label = &namelistnames[0];
    }
    namelistnames[1].line = true;
    for ( cnt=0; nlnames[cnt]!=NULL; ++cnt) {
	namelistnames[cnt+2].text = (unichar_t *) nlnames[cnt];
	namelistnames[cnt+2].text_is_1byte = true;
	if ( force_names_when_opening!=NULL &&
		strcmp(_(force_names_when_opening->title),nlnames[cnt])==0 ) {
	    namelistnames[cnt+2].selected = true;
	    gcd[i].gd.label = &namelistnames[cnt+2];
	}
    }
    harray2[1] = &gcd[i]; harray2[2] = GCD_Glue; harray2[3] = NULL;
    gcd[i++].gd.u.list = namelistnames;

    boxes[3].gd.flags = gg_visible | gg_enabled;
    boxes[3].gd.u.boxelements = harray2;
    boxes[3].creator = GHBoxCreate;
    varray[4] = &boxes[3]; varray[5] = NULL;

    gcd[i].gd.pos.x = 12; gcd[i].gd.pos.y = 216-3;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _("_OK");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.mnemonic = 'O';
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = GFD_Ok;
    harray3[0] = GCD_Glue; harray3[1] = &gcd[i];
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = -(spacing+bs)*100/GIntGetResource(_NUM_ScaleFactor)-12; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    label[i].text = (unichar_t *) S_("Font|_New");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.mnemonic = 'N';
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = GFD_New;
    harray3[2] = GCD_Glue; harray3[3] = &gcd[i];
    gcd[i++].creator = GButtonCreate;

    filter = i;
    gcd[i].gd.pos.x = (spacing+bs)*100/GIntGetResource(_NUM_ScaleFactor)+12; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = /* gg_visible |*/ gg_enabled;
    label[i].text = (unichar_t *) _("_Filter");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.mnemonic = 'F';
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = GFileChooserFilterEh;
    harray3[4] = &gcd[i];
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = -12; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _("_Cancel");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.mnemonic = 'C';
    gcd[i].gd.handle_controlevent = GFD_Cancel;
    harray3[5] = GCD_Glue; harray3[6] = &gcd[i]; harray3[7] = GCD_Glue; harray3[8] = NULL;
    gcd[i++].creator = GButtonCreate;

    boxes[4].gd.flags = gg_visible | gg_enabled;
    boxes[4].gd.u.boxelements = harray3;
    boxes[4].creator = GHBoxCreate;
    varray[6] = &boxes[4]; varray[7] = NULL;
    varray[8] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_visible | gg_enabled;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GHVGroupCreate;

    gcd[i].gd.pos.x = 2; gcd[i].gd.pos.y = 2;
    gcd[i].gd.pos.width = pos.width-4; gcd[i].gd.pos.height = pos.height-4;
    gcd[i].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[i++].creator = GGroupCreate;

    GGadgetsCreate(gw,boxes);

    d.gfc = gcd[0].ret;
    d.rename = gcd[renamei].ret;

    filts = StandardFilters();
    GGadgetSetList(harray1[2]->ret,filts,true);
    GHVBoxSetExpandableRow(boxes[0].ret,0);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[4].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);
    free(namelistnames);
    GGadgetSetUserData(gcd[filter].ret,gcd[0].ret);

    GFileChooserConnectButtons(gcd[0].ret,harray3[1]->ret,gcd[filter].ret);
    temp = utf82u_copy(filts[default_font_filter_index]->userdata);
    GFileChooserSetFilterText(gcd[0].ret,temp);
    free(temp);
    GFileChooserGetChildren(gcd[0].ret,NULL, NULL, &tf);
    if ( RecentFiles[0]!=NULL ) {
	GGadgetSetList(tf,GTextInfoFromChars(RecentFiles,RECENT_MAX),false);
    }
    GGadgetSetTitle8(gcd[0].ret,defaultfile);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    GDrawProcessPendingEvents(NULL);		/* Give the window a chance to vanish... */
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);		/* Give the window a chance to vanish... */
    free( d.lastpopupfontname );
    GTextInfoArrayFree(filts);
    for ( cnt=0; nlnames[cnt]!=NULL; ++cnt) {
	free(nlnames[cnt]);
    }
    free(nlnames);
return(d.ret);
}
