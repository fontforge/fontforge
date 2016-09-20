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

/* Routines to handle bdf properties, and a dialog to set them */
#include "bitmapchar.h"
#include "fontforgeui.h"
#include "splinefont.h"
#include <string.h>
#include <ustring.h>
#include <utype.h>
#include <math.h>
#include <gkeysym.h>

extern GBox _ggadget_Default_Box;
#define ACTIVE_BORDER   (_ggadget_Default_Box.active_border)
#define MAIN_FOREGROUND (_ggadget_Default_Box.main_foreground)

struct bdf_dlg_font {
    int old_prop_cnt;
    BDFProperties *old_props;
    BDFFont *bdf;
    int top_prop;	/* for the scrollbar */
    int sel_prop;
};

struct bdf_dlg {
    int fcnt;
    struct bdf_dlg_font *fonts;
    struct bdf_dlg_font *cur;
    EncMap *map;
    SplineFont *sf;
    GWindow gw, v;
    GGadget *vsb;
    GGadget *tf;
    GFont *font;
    int value_x;
    int vheight;
    int vwidth;
    int height,width;
    int fh, as;
    int done;
    int active;
    int press_pos;
};

extern struct std_bdf_props StandardProps[];

static int NumericKey(char *key) {
    int i;

    for ( i=0; StandardProps[i].name!=NULL; ++i )
	if ( strcmp(key,StandardProps[i].name)==0 )
return(( StandardProps[i].type&~prt_property)==prt_uint || ( StandardProps[i].type&~prt_property)==prt_int );

return( false );
}

static int KnownProp(char *keyword) {	/* We can generate default values for these */
    int i;

    for ( i=0; StandardProps[i].name!=NULL; ++i )
	if ( strcmp(keyword,StandardProps[i].name)==0 )
return( StandardProps[i].defaultable );

return( false );
}

static int KeyType(char *keyword) {
    int i;

    for ( i=0; StandardProps[i].name!=NULL; ++i )
	if ( strcmp(keyword,StandardProps[i].name)==0 )
return( StandardProps[i].type );

return( -1 );
}

static int UnknownKey(char *keyword) {
    int i;

    for ( i=0; StandardProps[i].name!=NULL; ++i )
	if ( strcmp(keyword,StandardProps[i].name)==0 )
return( false );

return( true );
}

/* ************************************************************************** */
/* ************                    Dialog                    **************** */
/* ************************************************************************** */

#define CID_Delete	1001
#define CID_DefAll	1002
#define CID_DefCur	1003
#define CID_Up		1004
#define CID_Down	1005

#define CID_OK		1010
#define CID_Cancel	1011

static void BdfP_RefigureScrollbar(struct bdf_dlg *bd) {
    struct bdf_dlg_font *cur = bd->cur;
    int lines = bd->vheight/(bd->fh+1);

    /* allow for one extra line which will display "New" */
    GScrollBarSetBounds(bd->vsb,0,cur->bdf->prop_cnt+1,lines);
    if ( cur->top_prop+lines>cur->bdf->prop_cnt+1 )
	cur->top_prop = cur->bdf->prop_cnt+1-lines;
    if ( cur->top_prop < 0 )
	cur->top_prop = 0;
    GScrollBarSetPos(bd->vsb,cur->top_prop);
}

static void BdfP_EnableButtons(struct bdf_dlg *bd) {
    struct bdf_dlg_font *cur = bd->cur;

    if ( cur->sel_prop<0 || cur->sel_prop>=cur->bdf->prop_cnt ) {
	GGadgetSetEnabled(GWidgetGetControl(bd->gw,CID_Delete),false);
	GGadgetSetEnabled(GWidgetGetControl(bd->gw,CID_DefCur),false);
	GGadgetSetEnabled(GWidgetGetControl(bd->gw,CID_Up),false);
	GGadgetSetEnabled(GWidgetGetControl(bd->gw,CID_Down),false);
    } else {
	GGadgetSetEnabled(GWidgetGetControl(bd->gw,CID_Delete),true);
	GGadgetSetEnabled(GWidgetGetControl(bd->gw,CID_DefCur),KnownProp(cur->bdf->props[cur->sel_prop].name));
	GGadgetSetEnabled(GWidgetGetControl(bd->gw,CID_Up),cur->sel_prop>0);
	GGadgetSetEnabled(GWidgetGetControl(bd->gw,CID_Down),cur->sel_prop<cur->bdf->prop_cnt-1);
    }
}

static void BdfP_HideTextField(struct bdf_dlg *bd) {
    if ( bd->active ) {
	bd->active = false;
	GGadgetSetVisible(bd->tf,false);
    }
}

static int BdfP_FinishTextField(struct bdf_dlg *bd) {
    if ( bd->active ) {
	char *text = GGadgetGetTitle8(bd->tf);
	char *pt, *end;
	int val;
	struct bdf_dlg_font *cur = bd->cur;
	BDFFont *bdf = cur->bdf;

	for ( pt=text; *pt; ++pt )
	    if ( *pt&0x80 ) {
		ff_post_error(_("Not ASCII"),_("All characters in the value must be in ASCII"));
		free(text);
return( false );
	    }
	val = strtol(text,&end,10);
	if ( NumericKey(bdf->props[cur->sel_prop].name) ) {
	    if ( *end!='\0' ) {
		ff_post_error(_("Bad Number"),_("Must be a number"));
		free(text);
return( false );
	    }
	}
	if ( (bdf->props[cur->sel_prop].type&~prt_property)==prt_string ||
		(bdf->props[cur->sel_prop].type&~prt_property)==prt_atom )
	    free(bdf->props[cur->sel_prop].u.str);
	if ( UnknownKey(bdf->props[cur->sel_prop].name) ) {
	    if ( *end!='\0' ) {
		bdf->props[cur->sel_prop].type = prt_string | prt_property;
		bdf->props[cur->sel_prop].u.str = copy(text);
	    } else {
		if ( bdf->props[cur->sel_prop].type != (prt_uint | prt_property ))
		    bdf->props[cur->sel_prop].type = prt_int | prt_property;
		bdf->props[cur->sel_prop].u.val = val;
	    }
	} else if ( NumericKey(bdf->props[cur->sel_prop].name) ) {
	    bdf->props[cur->sel_prop].type = KeyType(bdf->props[cur->sel_prop].name);
	    bdf->props[cur->sel_prop].u.val = val;
	} else {
	    bdf->props[cur->sel_prop].type = KeyType(bdf->props[cur->sel_prop].name);
	    bdf->props[cur->sel_prop].u.str = copy(text);
	}
	free(text);	    
	bd->active = false;
	GGadgetSetVisible(bd->tf,false);
    }
return( true );
}

static int BdfP_DefaultAll(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct bdf_dlg *bd = GDrawGetUserData(GGadgetGetWindow(g));
	int res = BdfPropHasInt(bd->cur->bdf,"RESOLUTION_Y",-1);
	if ( res!=-1 )
	    bd->cur->bdf->res = res;
	BdfP_HideTextField(bd);
	BDFPropsFree(bd->cur->bdf);
	bd->cur->bdf->prop_cnt = bd->cur->bdf->prop_max = 0;
	bd->cur->bdf->props = NULL;
	BDFDefaultProps(bd->cur->bdf, bd->map, -1);
	bd->cur->top_prop = 0; bd->cur->sel_prop = -1;
	BdfP_RefigureScrollbar(bd);
	BdfP_EnableButtons(bd);
	GDrawRequestExpose(bd->v,NULL,false);
    }
return( true );
}

static void _BdfP_DefaultCurrent(struct bdf_dlg *bd) {
    struct bdf_dlg_font *cur = bd->cur;
    BDFFont *bdf = cur->bdf;
    if ( cur->sel_prop<0 || cur->sel_prop>=bdf->prop_cnt )
return;
    BdfP_HideTextField(bd);
    if ( strcmp(bdf->props[cur->sel_prop].name,"FONT")==0 ) {
	Default_XLFD(bdf,bd->map,-1);
    } else if ( strcmp(bdf->props[cur->sel_prop].name,"COMMENT")==0 )
return;
    else
	Default_Properties(bdf,bd->map,bdf->props[cur->sel_prop].name);
    GDrawRequestExpose(bd->v,NULL,false);
return;
}

static int BdfP_DefaultCurrent(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct bdf_dlg *bd = GDrawGetUserData(GGadgetGetWindow(g));
	_BdfP_DefaultCurrent(bd);
    }
return( true );
}

static int BdfP_DeleteCurrent(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct bdf_dlg *bd = GDrawGetUserData(GGadgetGetWindow(g));
	struct bdf_dlg_font *cur = bd->cur;
	BDFFont *bdf = cur->bdf;
	int i;
	if ( cur->sel_prop<0 || cur->sel_prop>=bdf->prop_cnt )
return( true );
	BdfP_HideTextField(bd);
	if ( (bdf->props[cur->sel_prop].type&~prt_property)==prt_string ||
		(bdf->props[cur->sel_prop].type&~prt_property)==prt_atom )
	    free(bdf->props[cur->sel_prop].u.str);
	free(bdf->props[cur->sel_prop].name);
	--bdf->prop_cnt;
	for ( i=cur->sel_prop; i<bdf->prop_cnt; ++i )
	    bdf->props[i] = bdf->props[i+1];
	if ( cur->sel_prop >= bdf->prop_cnt )
	    --cur->sel_prop;
	BdfP_RefigureScrollbar(bd);
	BdfP_EnableButtons(bd);
	GDrawRequestExpose(bd->v,NULL,false);
    }
return( true );
}

static void _BdfP_Up(struct bdf_dlg *bd) {
    struct bdf_dlg_font *cur = bd->cur;
    BDFFont *bdf = cur->bdf;
    GRect r;
    BDFProperties prop;
    if ( cur->sel_prop<1 || cur->sel_prop>=bdf->prop_cnt )
return;
    prop = bdf->props[cur->sel_prop];
    bdf->props[cur->sel_prop] = bdf->props[cur->sel_prop-1];
    bdf->props[cur->sel_prop-1] = prop;
    --cur->sel_prop;
    GGadgetGetSize(bd->tf,&r);
    GGadgetMove(bd->tf,r.x,r.y-(bd->fh+1));
    BdfP_EnableButtons(bd);
    GDrawRequestExpose(bd->v,NULL,false);
}

static int BdfP_Up(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	_BdfP_Up( GDrawGetUserData(GGadgetGetWindow(g)));
    }
return( true );
}

static void _BdfP_Down(struct bdf_dlg *bd) {
    struct bdf_dlg_font *cur = bd->cur;
    BDFFont *bdf = cur->bdf;
    GRect r;
    BDFProperties prop;
    if ( cur->sel_prop<0 || cur->sel_prop>=bdf->prop_cnt-1 )
return;
    prop = bdf->props[cur->sel_prop];
    bdf->props[cur->sel_prop] = bdf->props[cur->sel_prop+1];
    bdf->props[cur->sel_prop+1] = prop;
    ++cur->sel_prop;
    GGadgetGetSize(bd->tf,&r);
    GGadgetMove(bd->tf,r.x,r.y+(bd->fh+1));
    BdfP_EnableButtons(bd);
    GDrawRequestExpose(bd->v,NULL,false);
return;
}

static int BdfP_Down(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	_BdfP_Down((struct bdf_dlg *) GDrawGetUserData(GGadgetGetWindow(g)));
    }
return( true );
}

static int BdfP_ChangeBDF(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct bdf_dlg *bd = GDrawGetUserData(GGadgetGetWindow(g));
	int sel = GGadgetGetFirstListSelectedItem(g);
	if ( sel<0 || sel>=bd->fcnt )
return( true );
	if ( !BdfP_FinishTextField(bd)) {
	    sel = bd->cur-bd->fonts;
	    GGadgetSelectListItem(g,sel,true);
return( true );
	}
	bd->cur = &bd->fonts[sel];
	BdfP_RefigureScrollbar(bd);
	BdfP_EnableButtons(bd);
	GDrawRequestExpose(bd->v,NULL,false);
    }
return( true );
}

static void BdfP_DoCancel(struct bdf_dlg *bd) {
    int i;
    for ( i=0; i<bd->fcnt; ++i ) {
	BDFFont *bdf = bd->fonts[i].bdf;
	BDFPropsFree(bdf);
	bdf->props = bd->fonts[i].old_props;
	bdf->prop_cnt = bd->fonts[i].old_prop_cnt;
    }
    free(bd->fonts);
    bd->done = true;
}

static int BdfP_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct bdf_dlg *bd = GDrawGetUserData(GGadgetGetWindow(g));
	BdfP_DoCancel(bd);
    }
return( true );
}

static int BdfP_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct bdf_dlg *bd = GDrawGetUserData(GGadgetGetWindow(g));
	int i;
	struct xlfd_components components;
	const char *xlfd;

	if ( !BdfP_FinishTextField(bd))
return( true );
	for ( i=0; i<bd->fcnt; ++i ) {
	    BDFFont *bdf = bd->fonts[i].bdf;
	    BDFProperties *temp = bdf->props;
	    int pc = bdf->prop_cnt;
	    bdf->props = bd->fonts[i].old_props;
	    bdf->prop_cnt = bd->fonts[i].old_prop_cnt;
	    BDFPropsFree(bdf);
	    bdf->props = temp;
	    bdf->prop_cnt = pc;

	    xlfd = BdfPropHasString(bdf,"FONT",NULL);
	    if ( xlfd!=NULL )
		XLFD_GetComponents(xlfd,&components);
	    else
		XLFD_CreateComponents(bdf,bd->map, -1, &components);
	    bdf->res = components.res_y;
	}
	free(bd->fonts);
	bd->sf->changed = true;
	bd->done = true;
    }
return( true );
}

static void BdfP_VScroll(struct bdf_dlg *bd,struct sbevent *sb) {
    int newpos = bd->cur->top_prop;
    int page = bd->vheight/(bd->fh+1);

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= 9*page/10;
      break;
      case et_sb_up:
        newpos--;
      break;
      case et_sb_down:
        newpos++;
      break;
      case et_sb_downpage:
        newpos += 9*page/10;
      break;
      case et_sb_bottom:
        newpos = bd->cur->bdf->prop_cnt+1;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos + page > bd->cur->bdf->prop_cnt+1 )
	newpos = bd->cur->bdf->prop_cnt+1 - page;
    if ( newpos<0 )
	newpos = 0;
    if ( newpos!=bd->cur->top_prop ) {
	int diff = (newpos-bd->cur->top_prop)*(bd->fh+1);
	GRect r;
	bd->cur->top_prop = newpos;
	GScrollBarSetPos(bd->vsb,newpos);
	GGadgetGetSize(bd->tf,&r);
	GGadgetMove(bd->tf,r.x,r.y+diff);
	r.x = 0; r.y = 0; r.width = bd->vwidth; r.height = (bd->vheight/(bd->fh+1))*(bd->fh+1);
	GDrawScroll(bd->v,&r,0,diff);
    }
}

static int _BdfP_VScroll(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_scrollbarchange ) {
	struct bdf_dlg *bd = (struct bdf_dlg *) GDrawGetUserData(GGadgetGetWindow(g));
	BdfP_VScroll(bd,&e->u.control.u.sb);
    }
return( true );
}

static void BdfP_Resize(struct bdf_dlg *bd) {
    extern int _GScrollBar_Width;
    int sbwidth = GDrawPointsToPixels(bd->gw,_GScrollBar_Width);
    GRect size, pos;
    static int cids[] = { CID_Delete, CID_DefAll, CID_DefCur, CID_Up,
	    CID_Down, CID_OK, CID_Cancel, 0 };
    int i;

    GDrawGetSize(bd->gw,&size);
    GDrawGetSize(bd->v,&pos);
    if ( size.width!=bd->width || size.height!=bd->height ) {
	int yoff = size.height-bd->height;
	int xoff = size.width-bd->width;
	bd->width = size.width; bd->height = size.height;
	bd->vwidth += xoff; bd->vheight += yoff;
	GDrawResize(bd->v,bd->vwidth,bd->vheight);
	
	GGadgetMove(bd->vsb,size.width-sbwidth,pos.y-1);
	GGadgetResize(bd->vsb,sbwidth,bd->vheight+2);

	GGadgetGetSize(bd->tf,&pos);
	GGadgetResize(bd->tf,pos.width+xoff,pos.height);

	for ( i=0; cids[i]!=0; ++i ) {
	    GGadgetGetSize(GWidgetGetControl(bd->gw,cids[i]),&pos);
	    GGadgetMove(GWidgetGetControl(bd->gw,cids[i]),pos.x,pos.y+yoff);
	}
    }
    BdfP_RefigureScrollbar(bd);
    GDrawRequestExpose(bd->v,NULL,false);
    GDrawRequestExpose(bd->gw,NULL,true);
}

static void BdfP_Expose(struct bdf_dlg *bd, GWindow pixmap) {
    struct bdf_dlg_font *cur = bd->cur;
    BDFFont *bdf = cur->bdf;
    int i;
    int page = bd->vheight/(bd->fh+1);
    GRect clip, old, r;
    char buffer[40];
    extern GBox _ggadget_Default_Box;

    GDrawSetFont(pixmap,bd->font);
    clip.x = 4; clip.width = bd->value_x-4-2; clip.height = bd->fh;
    for ( i=0; i<page && i+cur->top_prop<bdf->prop_cnt; ++i ) {
	int sel = i+cur->top_prop==cur->sel_prop;
	clip.y = i*(bd->fh+1);
	if ( sel ) {
	    r.x = 0; r.width = bd->width;
	    r.y = clip.y; r.height = clip.height;
	    GDrawFillRect(pixmap,&r,ACTIVE_BORDER);
	}
	GDrawPushClip(pixmap,&clip,&old);
	GDrawDrawText8(pixmap,4,i*(bd->fh+1)+bd->as,bdf->props[i+cur->top_prop].name,-1,MAIN_FOREGROUND);
	GDrawPopClip(pixmap,&old);
	switch ( bdf->props[i+cur->top_prop].type&~prt_property ) {
	  case prt_string: case prt_atom:
	    GDrawDrawText8(pixmap,bd->value_x+2,i*(bd->fh+1)+bd->as,bdf->props[i+cur->top_prop].u.str,-1,MAIN_FOREGROUND);
	  break;
	  case prt_int:
	    sprintf( buffer, "%d", bdf->props[i+cur->top_prop].u.val );
	    GDrawDrawText8(pixmap,bd->value_x+2,i*(bd->fh+1)+bd->as,buffer,-1,MAIN_FOREGROUND);
	  break;
	  case prt_uint:
	    sprintf( buffer, "%u", (unsigned) bdf->props[i+cur->top_prop].u.val );
	    GDrawDrawText8(pixmap,bd->value_x+2,i*(bd->fh+1)+bd->as,buffer,-1,MAIN_FOREGROUND);
	  break;
	}
	GDrawDrawLine(pixmap,0,i*(bd->fh+1)+bd->fh,bd->vwidth,i*(bd->fh+1)+bd->fh,0x808080);
    }
    if ( i<page ) {
/* GT: I am told that the use of "|" to provide contextual information in a */
/* GT: gettext string is non-standard. However it is documented in section */
/* GT: 10.2.6 of http://www.gnu.org/software/gettext/manual/html_mono/gettext.html */
/* GT: */
/* GT: Anyway here the word "Property" is used to provide context for "New..." and */
/* GT: the translator should only translate "New...". This is necessary because in */
/* GT: French (or any language where adjectives agree in gender/number with their */
/* GT: nouns) there are several different forms of "New" and the one chose depends */
/* GT: on the noun in question. */
/* GT: A french translation might be either msgstr "Nouveau..." or msgstr "Nouvelle..." */
/* GT: */
/* GT: I expect there are more cases where one english word needs to be translated */
/* GT: by several different words in different languages (in Japanese a different */
/* GT: word is used for the latin script and the latin language) and that you, as */
/* GT: a translator may need to ask me to disambiguate more strings. Please do so: */
/* GT:      <pfaedit@users.sourceforge.net> */
	GDrawDrawText8(pixmap,4,i*(bd->fh+1)+bd->as,S_("Property|New..."),-1,0xff0000);
	GDrawDrawLine(pixmap,0,i*(bd->fh+1)+bd->fh,bd->vwidth,i*(bd->fh+1)+bd->fh, _ggadget_Default_Box.border_darker);
    }
    GDrawDrawLine(pixmap,bd->value_x,0,bd->value_x,bd->vheight, _ggadget_Default_Box.border_darker);
}

static void BdfP_Invoked(GWindow v, GMenuItem *mi, GEvent *e) {
    struct bdf_dlg *bd = (struct bdf_dlg *) GDrawGetUserData(v);
    BDFFont *bdf = bd->cur->bdf;
    char *prop_name = cu_copy(mi->ti.text);
    int sel = bd->cur->sel_prop;
    int i;

    if ( sel>=bdf->prop_cnt ) {
	/* Create a new one */
	if ( bdf->prop_cnt>=bdf->prop_max )
	    bdf->props = realloc(bdf->props,(bdf->prop_max+=10)*sizeof(BDFProperties));
	sel = bd->cur->sel_prop = bdf->prop_cnt++;
	bdf->props[sel].name = prop_name;
	for ( i=0; StandardProps[i].name!=NULL; ++i )
	    if ( strcmp(prop_name,StandardProps[i].name)==0 )
	break;
	if ( StandardProps[i].name!=NULL ) {
	    bdf->props[sel].type = StandardProps[i].type;
	    if ( (bdf->props[sel].type&~prt_property)==prt_string ||
		    (bdf->props[sel].type&~prt_property)==prt_atom)
		bdf->props[sel].u.str = NULL;
	    else
		bdf->props[sel].u.val = 0;
	    if ( StandardProps[i].defaultable )
		_BdfP_DefaultCurrent(bd);
	    else if ( (bdf->props[sel].type&~prt_property)==prt_string ||
		    (bdf->props[sel].type&~prt_property)==prt_atom)
		bdf->props[sel].u.str = copy("");
	} else {
	    bdf->props[sel].type = prt_property|prt_string;
	    bdf->props[sel].u.str = copy("");
	}
    } else {
	free(bdf->props[sel].name);
	bdf->props[sel].name = prop_name;
    }
    GDrawRequestExpose(bd->v,NULL,false);
}

static void BdfP_PopupMenuProps(struct bdf_dlg *bd, GEvent *e) {
    GMenuItem *mi;
    int i;

    for ( i=0 ; StandardProps[i].name!=NULL; ++i );
    mi = calloc(i+3,sizeof(GMenuItem));
    mi[0].ti.text = (unichar_t *) _("No Change");
    mi[0].ti.text_is_1byte = true;
    mi[0].ti.fg = COLOR_DEFAULT;
    mi[0].ti.bg = COLOR_DEFAULT;

    mi[1].ti.line = true;
    mi[1].ti.fg = COLOR_DEFAULT;
    mi[1].ti.bg = COLOR_DEFAULT;
    
    for ( i=0 ; StandardProps[i].name!=NULL; ++i ) {
	mi[i+2].ti.text = (unichar_t *) StandardProps[i].name;	/* These do not translate!!! */
	mi[i+2].ti.text_is_1byte = true;
	mi[i+2].ti.fg = COLOR_DEFAULT;
	mi[i+2].ti.bg = COLOR_DEFAULT;
	mi[i+2].invoke = BdfP_Invoked;
    }

    GMenuCreatePopupMenu(bd->v,e,mi);
    free(mi);
}

static void BdfP_Mouse(struct bdf_dlg *bd, GEvent *e) {
    int line = e->u.mouse.y/(bd->fh+1) + bd->cur->top_prop;

    if ( line<0 || line>bd->cur->bdf->prop_cnt )
return;			/* "New" happens when line==bd->cur->bdf->prop_cnt */
    if ( e->type == et_mousedown ) {
	if ( !BdfP_FinishTextField(bd) ) {
	    bd->press_pos = -1;
return;
	}
	if ( e->u.mouse.x>=4 && e->u.mouse.x<= bd->value_x ) {
	    bd->cur->sel_prop = line;
	    BdfP_PopupMenuProps(bd,e);
	    BdfP_EnableButtons(bd);
	} else if ( line>=bd->cur->bdf->prop_cnt )
return;
	else {
	    bd->press_pos = line;
	    bd->cur->sel_prop = -1;
	    GDrawRequestExpose(bd->v,NULL,false );
	}
    } else if ( e->type == et_mouseup ) {
	int pos = bd->press_pos;
	bd->press_pos = -1;
	if ( line>=bd->cur->bdf->prop_cnt || line!=pos )
return;
	if ( bd->active )		/* Should never happen */
return;
	bd->cur->sel_prop = line;
	if ( e->u.mouse.x > bd->value_x ) {
	    BDFFont *bdf = bd->cur->bdf;
	    GGadgetMove(bd->tf,bd->value_x+2,(line-bd->cur->top_prop)*(bd->fh+1));
	    if ( (bdf->props[line].type&~prt_property) == prt_int ||
		    (bdf->props[line].type&~prt_property) == prt_uint ) {
		char buffer[40];
		sprintf( buffer,"%d",bdf->props[line].u.val );
		GGadgetSetTitle8(bd->tf,buffer);
	    } else
		GGadgetSetTitle8(bd->tf,bdf->props[line].u.str );
	    GGadgetSetVisible(bd->tf,true);
	    bd->active = true;
	}
	GDrawRequestExpose(bd->v,NULL,false );
    }
}

static int BdfP_Char(struct bdf_dlg *bd, GEvent *e) {
    if ( bd->active || bd->cur->sel_prop==-1 )
return( false );
    switch ( e->u.chr.keysym ) {
      case GK_Up: case GK_KP_Up:
	_BdfP_Up(bd);
return( true );
      case GK_Down: case GK_KP_Down:
	_BdfP_Down(bd);
return( true );
      default:
return( false );
    }
}

static int bdfpv_e_h(GWindow gw, GEvent *event) {
    struct bdf_dlg *bd = GDrawGetUserData(gw);
    if ( event->type==et_expose ) {
	BdfP_Expose(bd,gw);
    } else if ( event->type == et_char ) {
return( BdfP_Char(bd,event));
    } else if ( event->type == et_mousedown || event->type == et_mouseup || event->type==et_mousemove ) {
	BdfP_Mouse(bd,event);
	if ( event->type == et_mouseup )
	    BdfP_EnableButtons(bd);
    }
return( true );
}

static int bdfp_e_h(GWindow gw, GEvent *event) {
    struct bdf_dlg *bd = GDrawGetUserData(gw);
    if ( event->type==et_close ) {
	BdfP_DoCancel(bd);
    } else if ( event->type == et_expose ) {
	GRect r;
	GDrawGetSize(bd->v,&r);
	GDrawDrawLine(gw,0,r.y-1,bd->width,r.y-1,0x808080);
	GDrawDrawLine(gw,0,r.y+r.height,bd->width,r.y+r.height,0x808080);
    } else if ( event->type == et_char ) {
return( BdfP_Char(bd,event));
    } else if ( event->type == et_resize ) {
	BdfP_Resize(bd);
    }
return( true );
}

void SFBdfProperties(SplineFont *sf, EncMap *map, BDFFont *thisone) {
    struct bdf_dlg bd;
    int i;
    BDFFont *bdf;
    GTextInfo *ti;
    char buffer[40];
    char title[130];
    GRect pos, subpos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[10];
    GTextInfo label[9];
    FontRequest rq;
    static GFont *font = NULL;
    extern int _GScrollBar_Width;
    int sbwidth;
    static unichar_t sans[] = { 'h','e','l','v','e','t','i','c','a',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
    static GBox small = GBOX_EMPTY;
    GGadgetData gd;
    /* I don't use a MatrixEdit here because I want to be able to display */
    /*  non-standard properties. And a MatrixEdit can only disply things in */
    /*  its pull-down list */

    memset(&bd,0,sizeof(bd));
    bd.map = map;
    bd.sf = sf;
    for ( bdf = sf->bitmaps, i=0; bdf!=NULL; bdf=bdf->next, ++i );
    if ( i==0 )
return;
    bd.fcnt = i;
    bd.fonts = calloc(i,sizeof(struct bdf_dlg_font));
    bd.cur = &bd.fonts[0];
    for ( bdf = sf->bitmaps, i=0; bdf!=NULL; bdf=bdf->next, ++i ) {
	bd.fonts[i].bdf = bdf;
	bd.fonts[i].old_prop_cnt = bdf->prop_cnt;
	bd.fonts[i].old_props = BdfPropsCopy(bdf->props,bdf->prop_cnt);
	bd.fonts[i].sel_prop = -1;
	bdf->prop_max = bdf->prop_cnt;
	if ( bdf==thisone )
	    bd.cur = &bd.fonts[i];
    }

    ti = calloc((i+1),sizeof(GTextInfo));
    for ( bdf = sf->bitmaps, i=0; bdf!=NULL; bdf=bdf->next, ++i ) {
	if ( bdf->clut==NULL )
	    sprintf( buffer, "%d", bdf->pixelsize );
	else
	    sprintf( buffer, "%d@%d", bdf->pixelsize, BDFDepth(bdf));
	ti[i].text = (unichar_t *) copy(buffer);
	ti[i].text_is_1byte = true;
    }
    ti[bd.cur-bd.fonts].selected = true;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    snprintf(title,sizeof(title),_("Strike Information for %.90s"),
	    sf->fontname);
    wattrs.utf8_window_title = title;
    pos.x = pos.y = 0;
    pos.width = GDrawPointsToPixels(NULL,GGadgetScale(268));
    pos.height = GDrawPointsToPixels(NULL,375);
    bd.gw = gw = GDrawCreateTopWindow(NULL,&pos,bdfp_e_h,&bd,&wattrs);

    sbwidth = GDrawPointsToPixels(bd.gw,_GScrollBar_Width);
    subpos.x = 0; subpos.y =  GDrawPointsToPixels(NULL,28);
    subpos.width = pos.width-sbwidth;
    subpos.height = pos.height - subpos.y - GDrawPointsToPixels(NULL,70);
    wattrs.mask = wam_events;
    bd.v = GWidgetCreateSubWindow(gw,&subpos,bdfpv_e_h,&bd,&wattrs);
    bd.vwidth = subpos.width; bd.vheight = subpos.height;
    bd.width = pos.width; bd.height = pos.height;
    bd.value_x = GDrawPointsToPixels(bd.gw,135);

    if ( font==NULL ) {
	memset(&rq,0,sizeof(rq));
	rq.family_name = sans;
	rq.point_size = 10;
	rq.weight = 400;
	font = GDrawInstanciateFont(gw,&rq);
	font = GResourceFindFont("BDFProperties.Font",font);
    }
    bd.font = font;
    {
	int as, ds, ld;
	GDrawWindowFontMetrics(gw,bd.font,&as,&ds,&ld);
	bd.as = as; bd.fh = as+ds;
    }

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));

    i=0;
    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = 3;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i].gd.u.list = ti;
    gcd[i].gd.handle_controlevent = BdfP_ChangeBDF;
    gcd[i++].creator = GListButtonCreate;

    gcd[i].gd.pos.x = bd.vwidth; gcd[i].gd.pos.y = subpos.y-1;
    gcd[i].gd.pos.width = sbwidth; gcd[i].gd.pos.height = subpos.height+2;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_sb_vert | gg_pos_in_pixels;
    gcd[i].gd.handle_controlevent = _BdfP_VScroll;
    gcd[i++].creator = GScrollBarCreate;

    label[i].text = (unichar_t *) _("Delete");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 4; gcd[i].gd.pos.y = GDrawPixelsToPoints(gw,subpos.y+subpos.height)+6;
    gcd[i].gd.flags = gg_visible | gg_enabled ;
    gcd[i].gd.handle_controlevent = BdfP_DeleteCurrent;
    gcd[i].gd.cid = CID_Delete;
    gcd[i++].creator = GButtonCreate;

    label[i].text = (unichar_t *) _("Default All");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 80; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.flags = gg_visible | gg_enabled ;
    gcd[i].gd.handle_controlevent = BdfP_DefaultAll;
    gcd[i].gd.cid = CID_DefAll;
    gcd[i++].creator = GButtonCreate;

    label[i].text = (unichar_t *) _("Default This");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.flags = gg_visible | gg_enabled ;
    gcd[i].gd.handle_controlevent = BdfP_DefaultCurrent;
    gcd[i].gd.cid = CID_DefCur;
    gcd[i++].creator = GButtonCreate;

/* I want the 2 pronged arrow, but gdraw can't find a nice one */
/*  label[i].text = (unichar_t *) "⇑";	*//* Up Arrow */
    label[i].text = (unichar_t *) "↑";	/* Up Arrow */
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.flags = gg_visible | gg_enabled ;
    gcd[i].gd.handle_controlevent = BdfP_Up;
    gcd[i].gd.cid = CID_Up;
    gcd[i++].creator = GButtonCreate;

/* I want the 2 pronged arrow, but gdraw can't find a nice one */
/*  label[i].text = (unichar_t *) "⇓";	*//* Down Arrow */
    label[i].text = (unichar_t *) "↓";	/* Down Arrow */
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.flags = gg_visible | gg_enabled ;
    gcd[i].gd.handle_controlevent = BdfP_Down;
    gcd[i].gd.cid = CID_Down;
    gcd[i++].creator = GButtonCreate;


    gcd[i].gd.pos.x = 30-3; gcd[i].gd.pos.y = GDrawPixelsToPoints(NULL,pos.height)-32-3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _("_OK");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = BdfP_OK;
    gcd[i].gd.cid = CID_OK;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = -30; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _("_Cancel");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = BdfP_Cancel;
    gcd[i].gd.cid = CID_Cancel;
    gcd[i++].creator = GButtonCreate;
    
    GGadgetsCreate(gw,gcd);
    GTextInfoListFree(gcd[0].gd.u.list);
    bd.vsb = gcd[1].ret;

    small.main_background = small.main_foreground = COLOR_DEFAULT;
    small.main_foreground = 0x0000ff;
    memset(&gd,'\0',sizeof(gd));
    memset(&label[0],'\0',sizeof(label[0]));

    label[0].text = (unichar_t *) "\0\0\0\0";
    label[0].font = bd.font;
    gd.pos.height = bd.fh;
    gd.pos.width = bd.vwidth-bd.value_x;
    gd.label = &label[0];
    gd.box = &small;
    gd.flags = gg_enabled | gg_pos_in_pixels | gg_dontcopybox | gg_text_xim;
    bd.tf = GTextFieldCreate(bd.v,&gd,&bd);

    bd.press_pos = -1;
    BdfP_EnableButtons(&bd);
    BdfP_RefigureScrollbar(&bd);
    
    GDrawSetVisible(bd.v,true);
    GDrawSetVisible(gw,true);
    while ( !bd.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}
