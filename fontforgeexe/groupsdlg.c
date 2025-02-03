/* Copyright (C) 2005-2012 by George Williams */
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

#include "fontforgeui.h"
#include "fvfonts.h"
#include "gkeysym.h"
#include "gresedit.h"
#include "groups.h"
#include "namelist.h"
#include "splineutil.h"
#include "ustring.h"
#include "utype.h"
#include "gtk/font_view_shim.hpp"

#include <math.h>
#include <unistd.h>

GResFont groups_font = GRESFONT_INIT("400 12pt " SANS_UI_FAMILIES);

/******************************************************************************/
/******************************** Group Widget ********************************/
/******************************************************************************/

#define COLOR_CHOOSE	(-10)
static GTextInfo std_colors[] = {
    { (unichar_t *) N_("Select by Color"), NULL, 0, 0, (void *) COLOR_DEFAULT, NULL, 0, 1, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Color|Choose..."), NULL, 0, 0, (void *) COLOR_CHOOSE, NULL, 0, 1, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Color|Default"), &def_image, 0, 0, (void *) COLOR_DEFAULT, NULL, 0, 1, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { NULL, &white_image, 0, 0, (void *) 0xffffff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' },
    { NULL, &red_image, 0, 0, (void *) 0xff0000, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' },
    { NULL, &green_image, 0, 0, (void *) 0x00ff00, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' },
    { NULL, &blue_image, 0, 0, (void *) 0x0000ff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' },
    { NULL, &yellow_image, 0, 0, (void *) 0xffff00, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' },
    { NULL, &cyan_image, 0, 0, (void *) 0x00ffff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' },
    { NULL, &magenta_image, 0, 0, (void *) 0xff00ff, NULL, 0, 1, 0, 0, 0, 0, 0, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};

struct groupdlg {
    unsigned int oked: 1;
    unsigned int done: 1;
    unsigned int select_many: 1;
	/* define groups can only select one group at a time, select/restrict */
	/*  to groups can select multiple things */
    unsigned int select_kids_too: 1;
	/* When we select a parent group do we want to select all the kids? */
    Group *root;
    Group *oldsel;
    int open_cnt, lines_page, off_top, off_left, page_width, bmargin;
    int maxl;
    GWindow gw,v;
    GGadget *vsb, *hsb, *cancel, *ok, *compact;
    GGadget *newsub, *delete, *line1, *gpnamelab, *gpname, *glyphslab, *glyphs;
    GGadget *idlab, *idname, *iduni, *set, *select, *unique, *colour, *line2;
    int fh, as;
    GFont *font;
    FontView *fv;
    void (*select_callback)(struct groupdlg *);
    GTimer *showchange;
};

extern int _GScrollBar_Width;

static Group *GroupFindLPos(Group *group,int lpos,int *depth) {
    int i;

    for (;;) {
	if ( group->lpos==lpos )
return( group );
	if ( !group->open )
return( NULL );
	for ( i=0; i<group->kid_cnt-1; ++i ) {
	    if ( lpos<group->kids[i+1]->lpos )
	break;
	}

   // Return null if this is an empty node (prevents segfault on clicks under a group list)
   if(!group->kids) return NULL;

	group = group->kids[i];
	++*depth;
    }
}

static int GroupPosInParent(Group *group) {
    Group *parent = group->parent;
    int i;

    if ( parent==NULL )
return( 0 );
    for ( i=0; i<parent->kid_cnt; ++i )
	if ( parent->kids[i]==group )
return( i );

return( -1 );
}

static Group *GroupNext(Group *group,int *depth) {
    if ( group->open && group->kids ) {
	++*depth;
return( group->kids[0] );
    }
    for (;;) {
	int pos;
	if ( group->parent==NULL )
return( NULL );
	pos = GroupPosInParent(group);
	if ( pos+1<group->parent->kid_cnt )
return( group->parent->kids[pos+1] );
	group = group->parent;
	--*depth;
    }
}

static Group *GroupPrev(struct groupdlg *grp, Group *group,int *depth) {
    int pos;

    while ( group->parent!=NULL && group==group->parent->kids[0] ) {
	group = group->parent;
	--*depth;
    }
    if ( group->parent==NULL )
return( NULL );
    pos = GroupPosInParent(group);
    group = group->parent->kids[pos-1];
    while ( group->open ) {
	group = group->kids[group->kid_cnt-1];
	++*depth;
    }
return( group );
}

static int _GroupSBSizes(struct groupdlg *grp, Group *group, int lpos, int depth) {
    int i, len;

    group->lpos = lpos++;

    len = 5+8*depth+ grp->as + 5 + GDrawGetText8Width(grp->v,group->name,-1);
    if ( group->glyphs!=NULL )
	len += 5 + GDrawGetText8Width(grp->v,group->glyphs,-1);
    if ( len > grp->maxl )
	grp->maxl = len;

    if ( group->open ) {
	for ( i=0; i< group->kid_cnt; ++i )
	    lpos = _GroupSBSizes(grp,group->kids[i],lpos,depth+1);
    }
return( lpos );
}

static int GroupSBSizes(struct groupdlg *grp) {
    int lpos;

    grp->maxl = 0;
    GDrawSetFont(grp->v,grp->font);
    lpos = _GroupSBSizes(grp,grp->root,0,0);
    grp->maxl += 5;		/* margin */

    GScrollBarSetBounds(grp->vsb,0,lpos,grp->lines_page);
    GScrollBarSetBounds(grp->hsb,0,grp->maxl,grp->page_width);
    grp->open_cnt = lpos;
return( lpos );
}

static void GroupSelectKids(Group *group,int sel) {
    int i;

    group->selected = sel;
    for ( i=0; i<group->kid_cnt; ++i )
	GroupSelectKids(group->kids[i],sel);
}

static void GroupDeselectAllBut(Group *root,Group *group) {
    int i;

    if ( root!=group )
	root->selected = false;
    for ( i=0; i<root->kid_cnt; ++i )
	GroupDeselectAllBut(root->kids[i],group);
}

static Group *_GroupCurrentlySelected(Group *group) {
    int i;
    Group *sel;

    if ( group->selected )
return( group );
    for ( i=0; i<group->kid_cnt; ++i ) {
	sel = _GroupCurrentlySelected(group->kids[i]);
	if ( sel!=NULL )
return( sel );
    }
return( NULL );
}

static Group *GroupCurrentlySelected(struct groupdlg *grp) {

    if ( grp->select_many )
return( NULL );
return( _GroupCurrentlySelected(grp->root));
}

static void GroupWExpose(struct groupdlg *grp,GWindow pixmap,GRect *rect) {
    int depth, y, len;
    Group *group;
    GRect r;
    Color fg;

    GDrawFillRect(pixmap,rect,GDrawGetDefaultBackground(NULL));
    GDrawSetLineWidth(pixmap,0);

    r.height = r.width = grp->as;
    y = (rect->y/grp->fh) * grp->fh + grp->as;
    depth=0;
    group = GroupFindLPos(grp->root,rect->y/grp->fh+grp->off_top,&depth);
    GDrawSetFont(pixmap,grp->font);
    while ( group!=NULL ) {
	r.y = y-grp->as+1;
	r.x = 5+8*depth - grp->off_left;
	fg = group->selected ? GDrawGetWarningForeground(NULL) : GDrawGetDefaultForeground(NULL);
	if ( group->glyphs==NULL ) {
	    GDrawDrawRect(pixmap,&r,fg);
	    GDrawDrawLine(pixmap,r.x+2,r.y+grp->as/2,r.x+grp->as-2,r.y+grp->as/2,
		    fg);
	    if ( !group->open )
		GDrawDrawLine(pixmap,r.x+grp->as/2,r.y+2,r.x+grp->as/2,r.y+grp->as-2,
			fg);
	}
	len = GDrawDrawText8(pixmap,r.x+r.width+5,y,group->name,-1,fg);
	if ( group->glyphs )
	    GDrawDrawText8(pixmap,r.x+r.width+5+len+5,y,group->glyphs,-1,fg);
	group = GroupNext(group,&depth);
	y += grp->fh;
	if ( y-grp->fh>rect->y+rect->height )
    break;
    }
}

static void GroupWMouse(struct groupdlg *grp,GEvent *event) {
    int x;
    int depth=0;
    Group *group;

    group = GroupFindLPos(grp->root,event->u.mouse.y/grp->fh+grp->off_top,&depth);
    if ( group==NULL )
return;

    x = 5+8*depth - grp->off_left;
    if ( event->u.mouse.x<x )
return;
    if ( event->u.mouse.x<=x+grp->as ) {
	if ( group->glyphs != NULL )
return;
	group->open = !group->open;
	GroupSBSizes(grp);
    } else {
	group->selected = !group->selected;
	if ( grp->select_kids_too )
	    GroupSelectKids(group,group->selected);
	else if ( group->selected && !grp->select_many )
	    GroupDeselectAllBut(grp->root,group);
	if ( grp->select_callback!=NULL )
	    (grp->select_callback)(grp);
    }
    GDrawRequestExpose(grp->v,NULL,false);
}

static void GroupScroll(struct groupdlg *grp,struct sbevent *sb) {
    int newpos = grp->off_top;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= grp->lines_page;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += grp->lines_page;
      break;
      case et_sb_bottom:
        newpos = grp->open_cnt-grp->lines_page;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
      case et_sb_halfup: case et_sb_halfdown: break;
    }
    if ( newpos>grp->open_cnt-grp->lines_page )
        newpos = grp->open_cnt-grp->lines_page;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=grp->off_top ) {
	int diff = newpos-grp->off_top;
	grp->off_top = newpos;
	GScrollBarSetPos(grp->vsb,grp->off_top);
	GDrawScroll(grp->v,NULL,0,diff*grp->fh);
    }
}


static void GroupHScroll(struct groupdlg *grp,struct sbevent *sb) {
    int newpos = grp->off_left;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= grp->page_width;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += grp->page_width;
      break;
      case et_sb_bottom:
        newpos = grp->maxl-grp->page_width;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
      case et_sb_halfup: case et_sb_halfdown: break;
    }
    if ( newpos>grp->maxl-grp->page_width )
        newpos = grp->maxl-grp->page_width;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=grp->off_left ) {
	int diff = newpos-grp->off_left;
	grp->off_left = newpos;
	GScrollBarSetPos(grp->hsb,grp->off_left);
	GDrawScroll(grp->v,NULL,-diff,0);
    }
}

static void GroupResize(struct groupdlg *grp,GEvent *event) {
    GRect size, wsize;
    int lcnt, offy;
    int sbsize = GDrawPointsToPixels(grp->gw,_GScrollBar_Width);

    GDrawGetSize(grp->gw,&size);
    lcnt = (size.height-grp->bmargin)/grp->fh;
    GGadgetResize(grp->vsb,sbsize,lcnt*grp->fh);
    GGadgetMove(grp->vsb,size.width-sbsize,0);
    GGadgetResize(grp->hsb,size.width-sbsize,sbsize);
    GGadgetMove(grp->hsb,0,lcnt*grp->fh);
    GDrawResize(grp->v,size.width-sbsize,lcnt*grp->fh);
    grp->page_width = size.width-sbsize;
    grp->lines_page = lcnt;
    GScrollBarSetBounds(grp->vsb,0,grp->open_cnt,grp->lines_page);
    GScrollBarSetBounds(grp->hsb,0,grp->maxl,grp->page_width);

    GGadgetGetSize(grp->cancel,&wsize);
    offy = size.height-wsize.height-6 - wsize.y;
    GGadgetMove(grp->cancel,size.width-wsize.width-30,  wsize.y+offy);
    GGadgetMove(grp->ok    ,                       30-3,wsize.y+offy-3);
    if ( grp->newsub!=NULL ) {
	GGadgetGetSize(grp->newsub,&wsize);
	GGadgetMove(grp->newsub,wsize.x,wsize.y+offy);
	GGadgetGetSize(grp->delete,&wsize);
	GGadgetMove(grp->delete,wsize.x,wsize.y+offy);
	GGadgetGetSize(grp->line1,&wsize);
	GGadgetMove(grp->line1,wsize.x,wsize.y+offy);
	GGadgetGetSize(grp->gpnamelab,&wsize);
	GGadgetMove(grp->gpnamelab,wsize.x,wsize.y+offy);
	GGadgetGetSize(grp->gpname,&wsize);
	GGadgetMove(grp->gpname,wsize.x,wsize.y+offy);
	GGadgetGetSize(grp->glyphslab,&wsize);
	GGadgetMove(grp->glyphslab,wsize.x,wsize.y+offy);
	GGadgetGetSize(grp->glyphs,&wsize);
	GGadgetMove(grp->glyphs,wsize.x,wsize.y+offy);
	GGadgetGetSize(grp->idlab,&wsize);
	GGadgetMove(grp->idlab,wsize.x,wsize.y+offy);
	GGadgetGetSize(grp->idname,&wsize);
	GGadgetMove(grp->idname,wsize.x,wsize.y+offy);
	GGadgetGetSize(grp->iduni,&wsize);
	GGadgetMove(grp->iduni,wsize.x,wsize.y+offy);
	GGadgetGetSize(grp->set,&wsize);
	GGadgetMove(grp->set,wsize.x,wsize.y+offy);
	GGadgetGetSize(grp->select,&wsize);
	GGadgetMove(grp->select,wsize.x,wsize.y+offy);
	GGadgetGetSize(grp->unique,&wsize);
	GGadgetMove(grp->unique,wsize.x,wsize.y+offy);
	GGadgetGetSize(grp->colour,&wsize);
	GGadgetMove(grp->colour,wsize.x,wsize.y+offy);
	GGadgetGetSize(grp->line2,&wsize);
	GGadgetMove(grp->line2,wsize.x,wsize.y+offy);
    } else {
	GGadgetGetSize(grp->compact,&wsize);
	GGadgetMove(grp->compact,wsize.x,wsize.y+offy);
    }
    GDrawRequestExpose(grp->v,NULL,true);
    GDrawRequestExpose(grp->gw,NULL,true);
}

static void GroupWChangeCurrent(struct groupdlg *grp,Group *current,Group *next ) {
    if ( current!=NULL )
	current->selected = false;
    next->selected = true;
    if ( next->lpos<grp->off_top || next->lpos>=grp->off_top+grp->lines_page ) {
	if ( next->lpos>=grp->off_top+grp->lines_page )
	    grp->off_top = next->lpos;
	else {
	    grp->off_top = next->lpos-grp->lines_page-1;
	    if ( grp->off_top<0 ) grp->off_top = 0;
	}
	GScrollBarSetPos(grp->vsb,grp->off_top);
	GDrawRequestExpose(grp->v,NULL,false);
    }
}

static int GroupChar(struct groupdlg *grp,GEvent *event) {
    int depth = 0;
    int pos;
    Group *current = GroupCurrentlySelected(grp);

    switch (event->u.chr.keysym) {
      case GK_F1: case GK_Help:
	help("ui/dialogs/groups.html", NULL);
return( true );
      case GK_Return: case GK_KP_Enter:
	if ( current!=NULL ) {
	    current->open = !current->open;
	    GroupSBSizes(grp);
	    GDrawRequestExpose(grp->v,NULL,false);
	}
return( true );
      case GK_Page_Down: case GK_KP_Page_Down:
	pos = grp->off_top+(grp->lines_page<=1?1:grp->lines_page-1);
	if ( pos >= grp->open_cnt-grp->lines_page )
	    pos = grp->open_cnt-grp->lines_page;
	if ( pos<0 ) pos = 0;
	grp->off_top = pos;
	GScrollBarSetPos(grp->vsb,pos);
	GDrawRequestExpose(grp->v,NULL,false);
return( true );
      case GK_Down: case GK_KP_Down:
	if ( current==NULL || (event->u.chr.state&ksm_control)) {
	    if ( grp->off_top<grp->open_cnt-1 ) {
		++grp->off_top;
		GScrollBarSetPos(grp->vsb,grp->off_top);
		GDrawScroll(grp->v,NULL,0,grp->fh);
	    }
	} else
	    GroupWChangeCurrent(grp,current,GroupNext(current,&depth));
return( true );
      case GK_Up: case GK_KP_Up:
	if ( current==NULL || (event->u.chr.state&ksm_control)) {
	    if (grp->off_top!=0 ) {
		--grp->off_top;
		GScrollBarSetPos(grp->vsb,grp->off_top);
		GDrawScroll(grp->v,NULL,0,-grp->fh);
	    }
	} else
	    GroupWChangeCurrent(grp,current,GroupPrev(grp,current,&depth));
return( true );
      case GK_Page_Up: case GK_KP_Page_Up:
	pos = grp->off_top-(grp->lines_page<=1?1:grp->lines_page-1);
	if ( pos<0 ) pos = 0;
	grp->off_top = pos;
	GScrollBarSetPos(grp->vsb,pos);
	GDrawRequestExpose(grp->v,NULL,false);
return( true );
      case GK_Left: case GK_KP_Left:
	if ( !grp->select_many && current!=NULL )
	    GroupWChangeCurrent(grp,current,current->parent);
return( true );
      case GK_Right: case GK_KP_Right:
	if ( !grp->select_many && current != NULL && current->kid_cnt!=0 ) {
	    if ( !current->open ) {
		current->open = !current->open;
		GroupSBSizes(grp);
	    }
	    GroupWChangeCurrent(grp,current,current->kids[0]);
	}
return( true );
      case GK_Home: case GK_KP_Home:
	if ( grp->off_top!=0 ) {
	    grp->off_top = 0;
	    GScrollBarSetPos(grp->vsb,0);
	    GDrawRequestExpose(grp->v,NULL,false);
	}
	if ( !grp->select_many )
	    GroupWChangeCurrent(grp,current,grp->root);
return( true );
      case GK_End: case GK_KP_End:
	pos = grp->open_cnt-grp->lines_page;
	if ( pos<0 ) pos = 0;
	if ( pos!=grp->off_top ) {
	    grp->off_top = pos;
	    GScrollBarSetPos(grp->vsb,pos);
	    GDrawRequestExpose(grp->v,NULL,false);
	}
	if ( !grp->select_many )
	    GroupWChangeCurrent(grp,current,GroupFindLPos(grp->root,grp->open_cnt-1,&depth));
return( true );
    }
return( false );
}

static int grpv_e_h(GWindow gw, GEvent *event) {
    struct groupdlg *grp = (struct groupdlg *) GDrawGetUserData(gw);

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
return( GGadgetDispatchEvent(grp->vsb,event));
    }

    switch ( event->type ) {
      case et_expose:
	GroupWExpose(grp,gw,&event->u.expose.rect);
      break;
      case et_char:
return( GroupChar(grp,event));
      case et_mouseup:
	GroupWMouse(grp,event);
      break;
      default: break;
    }
return( true );
}

static void GroupWCreate(struct groupdlg *grp,GRect *pos) {
    int as, ds, ld;
    GGadgetCreateData gcd[5];
    GTextInfo label[4];
    int sbsize = GDrawPointsToPixels(NULL,_GScrollBar_Width);
    GWindowAttrs wattrs;

    grp->font = groups_font.fi;
    GDrawWindowFontMetrics(grp->gw,grp->font,&as,&ds,&ld);
    grp->fh = as+ds; grp->as = as;

    grp->lines_page = (pos->height-grp->bmargin)/grp->fh;
    grp->page_width = pos->width-sbsize;
    wattrs.mask = wam_events|wam_cursor/*|wam_bordwidth|wam_bordcol*/;
    wattrs.event_masks = ~0;
    wattrs.border_width = 1;
    wattrs.border_color = GDrawGetDefaultForeground(NULL);
    wattrs.cursor = ct_pointer;
    pos->x = 0; pos->y = 0;
    pos->width -= sbsize; pos->height = grp->lines_page*grp->fh;
    grp->v = GWidgetCreateSubWindow(grp->gw,pos,grpv_e_h,grp,&wattrs);
    GDrawSetVisible(grp->v,true);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    gcd[0].gd.pos.x = pos->width; gcd[0].gd.pos.y = 0;
    gcd[0].gd.pos.width = sbsize;
    gcd[0].gd.pos.height = pos->height;
    gcd[0].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_sb_vert;
    gcd[0].creator = GScrollBarCreate;

    gcd[1].gd.pos.x = 0; gcd[1].gd.pos.y = pos->height;
    gcd[1].gd.pos.height = sbsize;
    gcd[1].gd.pos.width = pos->width;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[1].creator = GScrollBarCreate;

    GGadgetsCreate(grp->gw,gcd);
    grp->vsb = gcd[0].ret;
    grp->hsb = gcd[1].ret;
}

/******************************************************************************/
/******************************** Group Dialogs *******************************/
/******************************************************************************/

static int FindDuplicateNumberInString(int seek,char *str) {
    char *start;

    if ( str==NULL )
return( false );

    while ( *str!='\0' ) {
	while ( *str==' ' ) ++str;
	start = str;
	while ( *str!=' ' && *str!='\0' ) ++str;
	if ( start==str )
    break;
	if ( (start[0]=='U' || start[0]=='u') && start[1]=='+' ) {
	    char *end;
	    int val = strtol(start+2,&end,16), val2=val;
	    if ( *end=='-' ) {
		if ( (end[1]=='u' || end[1]=='U') && end[2]=='+' )
		    end+=2;
		val2 = strtol(end+1,NULL,16);
	    }
	    if ( seek>=val && seek<=val2 )
return( true );
	}
    }
return( false );
}

static int FindDuplicateNameInString(char *name,char *str) {
    char *start;

    if ( str==NULL )
return( false );

    while ( *str!='\0' ) {
	while ( *str==' ' ) ++str;
	start = str;
	while ( *str!=' ' && *str!='\0' ) ++str;
	if ( start==str )
    break;
	if ( (start[0]=='U' || start[0]=='u') && start[1]=='+' )
	    /* Skip it */;
	else {
	    int ch = *str;
	    *str = '\0';
	    if ( strcmp(name,start)==0 ) {
		*str = ch;
return( true );
	    }
	    *str = ch;
	}
    }
return( false );
}

static Group *FindDuplicateNumber(Group *top,int val,Group *cur,char *str) {
    int i;
    Group *grp;

    if ( FindDuplicateNumberInString(val,str))
return( cur );
    if ( top==cur )
return( NULL );
    if ( FindDuplicateNumberInString(val,top->glyphs))
return( top );
    for ( i=0; i<top->kid_cnt; ++i )
	if ( (grp = FindDuplicateNumber(top->kids[i],val,cur,NULL))!=NULL )
return( grp );

return( NULL );
}

static Group *FindDuplicateName(Group *top,char *name,Group *cur,char *str) {
    int i;
    Group *grp;

    if ( FindDuplicateNameInString(name,str))
return( cur );
    if ( top==cur )
return( NULL );
    if ( FindDuplicateNameInString(name,top->glyphs))
return( top );
    for ( i=0; i<top->kid_cnt; ++i )
	if ( (grp = FindDuplicateName(top->kids[i],name,cur,NULL))!=NULL )
return( grp );

return( NULL );
}

static int GroupValidateGlyphs(Group *cur,char *g,const unichar_t *gu,int unique) {
    char *gpt, *start;
    Group *top, *grp;

    if ( gu!=NULL ) {
	for ( ; *gu!='\0'; ++gu ) {
	    if ( *gu<' ' || *gu>=0x7f || *gu=='(' || *gu==')' ||
		    *gu=='[' || *gu==']' || *gu=='{' || *gu=='}' ||
		    *gu=='<' || *gu=='>' || *gu=='%' || *gu=='/' ) {
		ff_post_error(_("Glyph names must be valid postscript names"),_("Glyph names must be valid postscript names"));
return( false );
	    }
	}
    }
    if ( unique ) {		/* Can't use cur->unique because it hasn't been set yet */
	top = cur;
	while ( top->parent!=NULL && top->parent->unique )
	    top = top->parent;
	for ( gpt=g; *gpt!='\0' ; ) {
	    while ( *gpt==' ' ) ++gpt;
	    start = gpt;
	    while ( *gpt!=' ' && *gpt!='\0' ) ++gpt;
	    if ( start==gpt )
	break;
	    if ( (start[0]=='U' || start[0]=='u') && start[1]=='+' ) {
		char *end;
		int val = strtol(start+2,&end,16), val2=val;
		if ( *end=='-' ) {
		    if ( (end[1]=='u' || end[1]=='U') && end[2]=='+' )
			end+=2;
		    val2 = strtol(end+1,NULL,16);
		}
		if ( val2<val ) {
		    ff_post_error(_("Bad Range"),_("Bad Range, start (%1$04X) is greater than end (%2$04X)"), val, val2 );
return( false );
		}
		for ( ; val<=val2; ++val )
		    if ( (grp=FindDuplicateNumber(top,val,cur,gpt))!=NULL ) {
			ff_post_error(_("Duplicate Name"),_("The code point U+%1$04X occurs in groups %2$.30s and %3$.30s"), val, cur->name, grp->name);
return( false );
		    }
	    } else {
		int ch = *gpt;
		*gpt = '\0';
		if ( (grp=FindDuplicateName(top,start,cur,ch!='\0'?gpt+1:NULL))!=NULL ) {
		    ff_post_error(_("Duplicate Name"),_("The glyph name \"%1$.30s\" occurs in groups %2$.30s and %3$.30s"), start, cur->name, grp->name);
		    *gpt = ch;
return( false );
		}
		*gpt = ch;
	    }
	}
    }
return( true );
}

static int GroupSetKidsUnique(Group *group) {
    int i;

    group->unique = true;
    for ( i=0; i<group->kid_cnt; ++i )
	if ( !GroupSetKidsUnique(group->kids[i]))
return( false );
    if ( group->glyphs!=NULL ) {
	if ( !GroupValidateGlyphs(group,group->glyphs,NULL,true))
return( false );
    }
return( true );
}

static int GroupFinishOld(struct groupdlg *grp) {
    if ( grp->oldsel!=NULL ) {
	const unichar_t *gu = _GGadgetGetTitle(grp->glyphs);
	char *g = cu_copy(gu);
	int oldunique = grp->oldsel->unique;

	if ( !GroupValidateGlyphs(grp->oldsel,g,gu,GGadgetIsChecked(grp->unique))) {
	    free(g);
return( false );
	}

	free(grp->oldsel->name);
	grp->oldsel->name = GGadgetGetTitle8(grp->gpname);
	free(grp->oldsel->glyphs);
	if ( *g=='\0' ) {
	    grp->oldsel->glyphs = NULL;
	    free(g);
	} else
	    grp->oldsel->glyphs = g;
	
	grp->oldsel->unique = GGadgetIsChecked(grp->unique);
	if ( grp->oldsel->unique && !oldunique ) {
	    /* The just set the unique bit. We must force it set in all */
	    /*  kids. We really should check for uniqueness too!!!!! */
	    if ( !GroupSetKidsUnique(grp->oldsel))
return( false );
	}
    }
return( true );
}

static void GroupSelected(struct groupdlg *grp) {
    Group *current = GroupCurrentlySelected(grp);

    if ( !GroupFinishOld(grp)) {
	if ( current!=NULL )
	    current->selected=false;
	if ( grp->oldsel!=NULL )
	    grp->oldsel->selected = true;
return;
    }
    grp->oldsel = current;
    if ( current == NULL ) {
	GGadgetSetEnabled(grp->newsub,false);
	GGadgetSetEnabled(grp->delete,false);
	GGadgetSetEnabled(grp->gpnamelab,false);
	GGadgetSetEnabled(grp->gpname,false);
	GGadgetSetEnabled(grp->glyphslab,false);
	GGadgetSetEnabled(grp->glyphs,false);
	GGadgetSetEnabled(grp->set,false);
	GGadgetSetEnabled(grp->select,false);
	GGadgetSetEnabled(grp->unique,false);
	GGadgetSetEnabled(grp->colour,false);
    } else {
	unichar_t *glyphs = uc_copy(current->glyphs);
	GGadgetSetTitle8(grp->gpname,current->name);
	if ( glyphs==NULL ) glyphs = uc_copy("");
	GGadgetSetTitle(grp->glyphs,glyphs);
	free(glyphs);
	GGadgetSetChecked(grp->unique,current->unique);
	GGadgetSetEnabled(grp->newsub,current->glyphs==NULL || *current->glyphs=='\0');
	GGadgetSetEnabled(grp->delete,current->parent!=NULL);
	GGadgetSetEnabled(grp->gpnamelab,true);
	GGadgetSetEnabled(grp->gpname,true);
	GGadgetSetEnabled(grp->glyphslab,current->kid_cnt==0);
	GGadgetSetEnabled(grp->glyphs,current->kid_cnt==0);
	GGadgetSetEnabled(grp->set,current->kid_cnt==0);
	GGadgetSetEnabled(grp->select,current->kid_cnt==0);
	GGadgetSetEnabled(grp->unique,current->parent==NULL || !current->parent->unique);
	GGadgetSetEnabled(grp->colour,current->kid_cnt==0);
    }
}

static void GroupShowChange(struct groupdlg *grp) {
    if ( GroupFinishOld(grp))
	GDrawRequestExpose(grp->v,NULL,false);
    grp->showchange = NULL;
}

static int Group_GlyphListChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	struct groupdlg *grp = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *glyphs = _GGadgetGetTitle(g);
	GGadgetSetEnabled(grp->newsub,*glyphs=='\0');
	if ( grp->showchange!=NULL )
	    GDrawCancelTimer(grp->showchange);
	grp->showchange = GDrawRequestTimer(grp->gw,500,0,NULL);
    }
return( true );
}

static int Group_ToSelection(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct groupdlg *grp = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(grp->glyphs);
	SplineFont *sf = grp->fv->b.sf;
	FontView *fv = grp->fv;
	const unichar_t *end;
	int pos, found=-1;
	char *nm;

	cg_raise_window(fv->cg_widget);
	memset(fv->b.selected,0,fv->b.map->enccount);
	while ( *ret ) {
	    end = u_strchr(ret,' ');
	    if ( end==NULL ) end = ret+u_strlen(ret);
	    nm = cu_copybetween(ret,end);
	    for ( ret = end; isspace(*ret); ++ret);
	    if ( (nm[0]=='U' || nm[0]=='u') && nm[1]=='+' ) {
		char *end;
		int val = strtol(nm+2,&end,16), val2=val;
		if ( *end=='-' ) {
		    if ( (end[1]=='u' || end[1]=='U') && end[2]=='+' )
			end+=2;
		    val2 = strtol(end+1,NULL,16);
		}
		for ( ; val<=val2; ++val ) {
		    if (( pos = SFFindSlot(sf,fv->b.map,val,NULL))!=-1 ) {
			if ( found==-1 ) found = pos;
			if ( pos!=-1 )
			    fv->b.selected[pos] = true;
		    }
		}
	    } else if ( strncasecmp(nm,"color=#",strlen("color=#"))==0 ) {
		Color col = strtoul(nm+strlen("color=#"),NULL,16);
		int gid; SplineChar *sc;

		for ( pos=0; pos<fv->b.map->enccount; ++pos )
		    if ( (gid=fv->b.map->map[pos])!=-1 && 
			    (sc = sf->glyphs[gid])!=NULL &&
			    sc->color == col )
			fv->b.selected[pos] = true;
	    } else {
		if (( pos = SFFindSlot(sf,fv->b.map,-1,nm))!=-1 ) {
		    if ( found==-1 ) found = pos;
		    if ( pos!=-1 )
			fv->b.selected[pos] = true;
		}
	    }
	    free(nm);
	}

	if ( found!=-1 )
	    FVScrollToChar(fv,found);
	GDrawRequestExpose(fv->v,NULL,false);
    }
return( true );
}

static int Group_FromSelection(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct groupdlg *grp = GDrawGetUserData(GGadgetGetWindow(g));
	SplineFont *sf = grp->fv->b.sf;
	FontView *fv = grp->fv;
	unichar_t *vals, *pt;
	int i, len, max, gid, k;
	SplineChar *sc, dummy;
	char buffer[20];

	if ( GGadgetIsChecked(grp->idname) ) {
	    for ( i=len=max=0; i<fv->b.map->enccount; ++i ) if ( fv->b.selected[i]) {
		gid = fv->b.map->map[i];
		if ( gid!=-1 && sf->glyphs[gid]!=NULL )
		    sc = sf->glyphs[gid];
		else
		    sc = SCBuildDummy(&dummy,sf,fv->b.map,i);
		len += strlen(sc->name)+1;
		if ( fv->b.selected[i]>max ) max = fv->b.selected[i];
	    }
	    pt = vals = malloc((len+1)*sizeof(unichar_t));
	    *pt = '\0';
	    for ( i=len=max=0; i<fv->b.map->enccount; ++i ) if ( fv->b.selected[i]) {
		gid = fv->b.map->map[i];
		if ( gid!=-1 && sf->glyphs[gid]!=NULL )
		    sc = sf->glyphs[gid];
		else
		    sc = SCBuildDummy(&dummy,sf,fv->b.map,i);
		uc_strcpy(pt,sc->name);
		pt += u_strlen(pt);
		*pt++ = ' ';
	    }
	    if ( pt>vals ) pt[-1]='\0';
	} else {
	    vals = NULL;
	    for ( k=0; k<2; ++k ) {
		int last=-2, start=-2;
		len = 0;
		for ( i=len=max=0; i<fv->b.map->enccount; ++i ) if ( fv->b.selected[i]) {
		    gid = fv->b.map->map[i];
		    if ( gid!=-1 && sf->glyphs[gid]!=NULL )
			sc = sf->glyphs[gid];
		    else
			sc = SCBuildDummy(&dummy,sf,fv->b.map,i);
		    if ( sc->unicodeenc==-1 )
		continue;
		    if ( sc->unicodeenc==last+1 )
			last = sc->unicodeenc;
		    else {
			if ( last!=-2 ) {
			    if ( start!=last )
				sprintf( buffer, "U+%04X-U+%04X ", start, last );
			    else
				sprintf( buffer, "U+%04X ", start );
			    if ( vals!=NULL )
				uc_strcpy(vals+len,buffer);
			    len += strlen(buffer);
			}
			start = last = sc->unicodeenc;
		    }
		}
		if ( last!=-2 ) {
		    if ( start!=last )
			sprintf( buffer, "U+%04X-U+%04X ", start, last );
		    else
			sprintf( buffer, "U+%04X ", start );
		    if ( vals!=NULL )
			uc_strcpy(vals+len,buffer);
		    len += strlen(buffer);
		}
		if ( !k )
		    vals = malloc((len+1)*sizeof(unichar_t));
		else if ( len!=0 )
		    vals[len-1] = '\0';
		else
		    *vals = '\0';
	    }
	}

	GGadgetSetTitle(grp->glyphs,vals);
	free(vals);
    }
return( true );
}

static int Group_AddColor(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct groupdlg *grp = GDrawGetUserData(GGadgetGetWindow(g));
	GTextInfo *ti = GGadgetGetListItemSelected(g);
	int set=false;
	Color xcol=0;

	if ( ti==NULL )
	    /* Can't happen */;
	else if ( ti->userdata == (void *) COLOR_CHOOSE ) {
	    struct hslrgb col, font_cols[6];
	    memset(&col,0,sizeof(col));
	    col = GWidgetColor(_("Pick a color"),&col,SFFontCols(grp->fv->b.sf,font_cols));
	    if ( col.rgb ) {
		xcol = (((int) rint(255.*col.r))<<16 ) |
			    (((int) rint(255.*col.g))<<8 ) |
			    (((int) rint(255.*col.b)) );
		set = true;
	    }
	} else {
	    xcol = (intptr_t) ti->userdata;
	    set = true;
	}

	if ( set ) {
	    char buffer[40]; unichar_t ubuf[40];
	    sprintf(buffer," color=#%06x", xcol );
	    uc_strcpy(ubuf,buffer);
	    GTextFieldReplace(grp->glyphs,ubuf);
	    if ( grp->showchange==NULL )
		GroupShowChange(grp);
	}
	GGadgetSelectOneListItem(g,0);
    }
return( true );
}

static int Group_NewSubGroup(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct groupdlg *grp = GDrawGetUserData(GGadgetGetWindow(g));
	Group *new_grp;
	if ( !GroupFinishOld(grp))
return( true );
	GDrawRequestExpose(grp->v,NULL,false);
	if ( grp->oldsel==NULL )
return( true );
	if ( grp->oldsel->glyphs!=NULL && grp->oldsel->glyphs[0]!='\0' ) {
	    GGadgetSetEnabled(grp->newsub,false);
return( true );
	}
	grp->oldsel->kids = realloc(grp->oldsel->kids,(++grp->oldsel->kid_cnt)*sizeof(Group *));
	grp->oldsel->kids[grp->oldsel->kid_cnt-1] = new_grp = chunkalloc(sizeof(Group));
	new_grp->parent = grp->oldsel;
	new_grp->unique = grp->oldsel->unique;
	new_grp->name = copy(_("UntitledGroup"));
	grp->oldsel->selected = false;
	grp->oldsel->open = true;
	new_grp->selected = true;
	GroupSBSizes(grp);
	GroupSelected(grp);
	GDrawRequestExpose(grp->v,NULL,false);
    }
return( true );
}

static int Group_Delete(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct groupdlg *grp = GDrawGetUserData(GGadgetGetWindow(g));
	Group *parent;
	int pos, i;
	if ( grp->oldsel==NULL || grp->oldsel->parent==NULL )
return( true );
	parent = grp->oldsel->parent;
	pos = GroupPosInParent(grp->oldsel);
	if ( pos==-1 )
return( true );
	for ( i=pos; i<parent->kid_cnt-1; ++i )
	    parent->kids[i] = parent->kids[i+1];
	--parent->kid_cnt;
	GroupFree(grp->oldsel);
	grp->oldsel = NULL;
	GroupSBSizes(grp);
	GroupSelected(grp);
	GDrawRequestExpose(grp->v,NULL,false);
    }
return( true );
}

static int displaygrp_e_h(GWindow gw, GEvent *event) {
    struct groupdlg *grp = (struct groupdlg *) GDrawGetUserData(gw);

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
return( GGadgetDispatchEvent(grp->vsb,event));
    }

    if ( grp==NULL )
return( true );

    switch ( event->type ) {
      case et_expose:
      break;
      case et_char:
return( GroupChar(grp,event));
      break;
      case et_timer:
	GroupShowChange(grp);
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    GroupResize(grp,event);
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    if ( event->u.control.g == grp->vsb )
		GroupScroll(grp,&event->u.control.u.sb);
	    else
		GroupHScroll(grp,&event->u.control.u.sb);
	  break;
	  case et_buttonactivate:
	    grp->done = true;
	    grp->oked = event->u.control.g == grp->ok;
	  break;
	  default: break;
	}
      break;
      case et_close:
	grp->done = true;
      break;
      case et_destroy:
	if ( grp->newsub!=NULL )
	    free(grp);
return( true );
      default: break;
    }
    if ( grp->done && grp->newsub!=NULL ) {
	if ( grp->oked ) {
	    if ( !GroupFinishOld(grp)) {
		grp->done = grp->oked = false;
return( true );
	    }
	    GroupFree(group_root);
	    if ( grp->root->kid_cnt==0 && grp->root->glyphs==NULL ) {
		group_root = NULL;
		GroupFree(grp->root);
	    } else
		group_root = grp->root;
	    SaveGroupList();
	} else
	    GroupFree(grp->root);
	GDrawDestroyWindow(grp->gw);
    }
return( true );
}

void DefineGroups(FontView *fv) {
    struct groupdlg *grp;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[20];
    GTextInfo label[19];
    int h, k,kk;

    grp = calloc(1,sizeof(*grp));
    grp->fv = fv;
    grp->select_many = grp->select_kids_too = false;
    grp->select_callback = GroupSelected;

    if ( group_root==NULL ) {
	grp->root = chunkalloc(sizeof(Group));
	grp->root->name = copy(_("Groups"));
    } else
	grp->root = GroupCopy(group_root);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = false;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Define Groups");
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(200));
    pos.height = h = GDrawPointsToPixels(NULL,482);
    grp->gw = GDrawCreateTopWindow(NULL,&pos,displaygrp_e_h,grp,&wattrs);

    grp->bmargin = GDrawPointsToPixels(NULL,248)+GDrawPointsToPixels(grp->gw,_GScrollBar_Width);

    GroupWCreate(grp,&pos);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    k = 0;

    gcd[k].gd.pos.x = 20;
    gcd[k].gd.pos.y = GDrawPixelsToPoints(NULL,h-grp->bmargin)+12;
    gcd[k].gd.flags = gg_visible;
    label[k].text = (unichar_t *) _("New Sub-Group");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = Group_NewSubGroup;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.width = -1;
    gcd[k].gd.pos.x = GDrawPixelsToPoints(NULL,(pos.width-30-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor)));
    gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.flags = gg_visible;
    label[k].text = (unichar_t *) _("_Delete");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = Group_Delete;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.width = GDrawPixelsToPoints(NULL,pos.width)-20;
    gcd[k].gd.pos.x = 10;
    gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+26;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLineCreate;

    gcd[k].gd.pos.x = 5;
    gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+8;
    gcd[k].gd.flags = gg_visible;
    label[k].text = (unichar_t *) _("Group Name:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k++].creator = GLabelCreate;

    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.width = 115;
    gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.flags = gg_visible;
    gcd[k++].creator = GTextFieldCreate;

    gcd[k].gd.pos.x = 5;
    gcd[k].gd.pos.y = gcd[k-2].gd.pos.y+16;
    gcd[k].gd.flags = gg_visible;
    label[k].text = (unichar_t *) _("Glyphs:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k++].creator = GLabelCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.pos.width = GDrawPixelsToPoints(NULL,pos.width)-10; gcd[k].gd.pos.height = 4*13+4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
    gcd[k].gd.handle_controlevent = Group_GlyphListChanged;
    gcd[k++].creator = GTextAreaCreate;

    gcd[k].gd.pos.x = 5;
    gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+gcd[k-1].gd.pos.height+5;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    label[k].text = (unichar_t *) _("Identify by");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.popup_msg = _("Glyphs may be either identified by name or by unicode code point.\nGenerally you control this by what you type in.\nTyping \"A\" would identify a glyph by name.\nTyping \"U+0041\" identifies a glyph by code point.\nWhen loading glyphs from the selection you must specify which format is desired.");
    gcd[k++].creator = GLabelCreate;

    gcd[k].gd.pos.x = 90; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-2;
    label[k].text = (unichar_t *) _("Name");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = (gg_visible | gg_enabled | gg_cb_on);
    gcd[k].gd.popup_msg = _("Glyphs may be either identified by name or by unicode code point.\nGenerally you control this by what you type in.\nTyping \"A\" would identify a glyph by name.\nTyping \"U+0041\" identifies a glyph by code point.\nWhen loading glyphs from the selection you must specify which format is desired.");
    gcd[k++].creator = GRadioCreate;

    gcd[k].gd.pos.x = 140; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    label[k].text = (unichar_t *) _("Unicode");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.popup_msg = _("Glyphs may be either identified by name or by unicode code point.\nGenerally you control this by what you type in.\nTyping \"A\" would identify a glyph by name.\nTyping \"U+0041\" identifies a glyph by code point.\nWhen loading glyphs from the selection you must specify which format is desired.");
    gcd[k++].creator = GRadioCreate;

    label[k].text = (unichar_t *) _("Set From Font");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+18;
    gcd[k].gd.popup_msg = _("Set this glyph list to be the glyphs selected in the fontview");
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = Group_FromSelection;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("Select In Font");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 110; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.popup_msg = _("Set the fontview's selection to be the glyphs named here");
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = Group_ToSelection;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+26;
    label[k].text = (unichar_t *) _("No Glyph Duplicates");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.popup_msg = _("Glyph names (or unicode code points) may occur at most once in this group and any of its sub-groups");
    gcd[k++].creator = GCheckBoxCreate;

    for ( kk=0; kk<3; ++kk )
	std_colors[kk].text = (unichar_t *) S_((char *) std_colors[kk].text);
    std_colors[1].image = GGadgetImageCache("colorwheel.png");
    std_colors[0].selected = true;
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+15;
    gcd[k].gd.label = &std_colors[0];
    gcd[k].gd.u.list = std_colors;
    gcd[k].gd.handle_controlevent = Group_AddColor;
    gcd[k].gd.flags = gg_visible;
    gcd[k++].creator = GListButtonCreate;

    gcd[k].gd.pos.width = GDrawPixelsToPoints(NULL,pos.width)-20;
    gcd[k].gd.pos.x = 10;
    gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+28;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLineCreate;

    gcd[k].gd.pos.width = -1;
    gcd[k].gd.pos.x = 30;
    gcd[k].gd.pos.y = h-GDrawPointsToPixels(NULL,32);
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default | gg_pos_in_pixels;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.width = -1;
    gcd[k].gd.pos.x = (pos.width-30-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor));
    gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel | gg_pos_in_pixels;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k++].creator = GButtonCreate;

    GGadgetsCreate(grp->gw,gcd);
    grp->newsub = gcd[0].ret;
    grp->delete = gcd[1].ret;
    grp->line1 = gcd[2].ret;
    grp->gpnamelab = gcd[3].ret;
    grp->gpname = gcd[4].ret;
    grp->glyphslab = gcd[5].ret;
    grp->glyphs = gcd[6].ret;
    grp->idlab = gcd[7].ret;
    grp->idname = gcd[8].ret;
    grp->iduni = gcd[9].ret;
    grp->set = gcd[10].ret;
    grp->select = gcd[11].ret;
    grp->unique = gcd[12].ret;
    grp->colour = gcd[13].ret;
    grp->line2 = gcd[14].ret;
    grp->ok = gcd[15].ret;
    grp->cancel = gcd[16].ret;

    GroupSBSizes(grp);
    GroupResize(grp,NULL);

    GDrawSetVisible(grp->gw,true);
}

static void MapEncAddGid(EncMap *map,SplineFont *sf, int compacted,
	int gid, int uni, char *name) {

    if ( compacted && gid==-1 )
return;

    if ( gid!=-1 && map->backmap[gid]==-1 )
	map->backmap[gid] = map->enccount;
    if ( map->enccount>=map->encmax )
	map->map = realloc(map->map,(map->encmax+=100)*sizeof(int));
    map->map[map->enccount++] = gid;
    if ( !compacted ) {
	Encoding *enc = map->enc;
	if ( enc->char_cnt>=enc->char_max ) {
	    enc->unicode = realloc(enc->unicode,(enc->char_max+=256)*sizeof(int));
	    enc->psnames = realloc(enc->psnames,enc->char_max*sizeof(char *));
	}
	if ( uni==-1 && name!=NULL ) {
	    if ( gid!=-1 && sf->glyphs[gid]!=NULL )
		uni = sf->glyphs[gid]->unicodeenc;
	    else
		uni = UniFromName(name,ui_none,&custom);
	}
	enc->unicode[enc->char_cnt] = uni;
	enc->psnames[enc->char_cnt++] = copy( name );
    }
}

static void MapAddGroupGlyph(EncMap *map,SplineFont *sf,char *name, int compacted) {
    int gid;

    if ( (name[0]=='u' || name[0]=='U') && name[1]=='+' && ishexdigit(name[2])) {
	char *end;
	int val = strtol(name+2,&end,16), val2=val;
	if ( *end=='-' ) {
	    if ( (end[1]=='u' || end[1]=='U') && end[2]=='+' )
		end+=2;
	    val2 = strtol(end+1,NULL,16);
	}
	for ( ; val<=val2; ++val ) {
	    gid = SFFindExistingSlot(sf,val,NULL);
	    MapEncAddGid(map,sf,compacted,gid,val,NULL);
	}
    } else if ( strncasecmp(name,"color=#",strlen("color=#"))==0 ) {
	Color col = strtoul(name+strlen("color=#"),NULL,16);
	int gid; SplineChar *sc;

	for ( gid=0; gid<sf->glyphcnt; ++gid )
	    if ( (sc = sf->glyphs[gid])!=NULL &&
		    sc->color == col )
		MapEncAddGid(map,sf,compacted,gid,sc->unicodeenc,NULL);
    } else {
	gid = SFFindExistingSlot(sf,-1,name);
	MapEncAddGid(map,sf,compacted,gid,-1,name);
    }
}

static int MapAddSelectedGroups(EncMap *map,SplineFont *sf,Group *group, int compacted) {
    int i, cnt=0;
    char *start, *pt;
    int ch;

    if ( group->glyphs==NULL ) {
	for ( i=0; i<group->kid_cnt; ++i )
	    cnt += MapAddSelectedGroups(map,sf,group->kids[i], compacted);
    } else if ( group->selected ) {
	for ( pt=group->glyphs; *pt!='\0'; ) {
	    while ( *pt==' ' ) ++pt;
	    start = pt;
	    while ( *pt!=' ' && *pt!='\0' ) ++pt;
	    ch = *pt; *pt='\0';
	    if ( *start!='\0' )
		MapAddGroupGlyph(map,sf,start, compacted);
	    *pt=ch;
	}
	++cnt;
    }
return( cnt );
}

static int GroupSelCnt(Group *group, Group **first, Group **second) {
    int cnt = 0, i;

    if ( group->glyphs==NULL ) {
	for ( i=0; i<group->kid_cnt; ++i )
	    cnt += GroupSelCnt(group->kids[i],first,second);
    } else if ( group->selected ) {
	if ( *first==NULL )
	    *first = group;
	else if ( *second==NULL )
	    *second = group;
	++cnt;
    }
return( cnt );
}

static char *EncNameFromGroups(Group *group) {
    Group *first = NULL, *second = NULL;
    int cnt = GroupSelCnt(group,&first,&second);
    char *prefix = P_("Group","Groups",cnt);
    char *ret;

    switch ( cnt ) {
      case 0:
return( copy( _("No Groups")) );
      case 1:
	ret = malloc(strlen(prefix) + strlen(first->name) + 3 );
	sprintf( ret, "%s: %s", prefix, first->name);
      break;
      case 2:
	ret = malloc(strlen(prefix) + strlen(first->name) + strlen(second->name) + 5 );
	sprintf( ret, "%s: %s, %s", prefix, first->name, second->name );
      break;
      default:
	ret = malloc(strlen(prefix) + strlen(first->name) + strlen(second->name) + 9 );
	sprintf( ret, "%s: %s, %s ...", prefix, first->name, second->name );
      break;
    }
return( ret );
}

static void EncodeToGroups(FontView *fv,Group *group, int compacted) {
    SplineFont *sf = fv->b.sf;
    EncMap *map;
    if ( compacted )
	map = EncMapNew(0,sf->glyphcnt,&custom);
    else {
	Encoding *enc = calloc(1,sizeof(Encoding));
	enc->enc_name = EncNameFromGroups(group);
	enc->is_temporary = true;
	enc->char_max = 256;
	enc->unicode = malloc(256*sizeof(int32_t));
	enc->psnames = malloc(256*sizeof(char *));
	map = EncMapNew(0,sf->glyphcnt,enc);
    }

    if ( MapAddSelectedGroups(map,sf,group,compacted)==0 ) {
	ff_post_error(_("Nothing Selected"),_("Nothing Selected"));
	EncMapFree(map);
    } else if ( map->enccount==0 ) {
	ff_post_error(_("Nothing Selected"),_("None of the glyphs in the current font match any names or code points in the selected groups"));
	EncMapFree(map);
    } else {
	fv->b.selected = realloc(fv->b.selected,map->enccount);
	memset(fv->b.selected,0,map->enccount);
	EncMapFree(fv->b.map);
	fv->b.map = map;
	FVSetTitle((FontViewBase *) fv);
	FontViewReformatOne((FontViewBase *) fv);
    }
}

void DisplayGroups(FontView *fv) {
    struct groupdlg grp;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[6];
    GTextInfo label[5];
    int h;

    memset( &grp,0,sizeof(grp));
    grp.fv = fv;
    grp.select_many = grp.select_kids_too = true;
    grp.root = group_root;

    if ( grp.root==NULL ) {
	grp.root = chunkalloc(sizeof(Group));
	grp.root->name = copy(_("Groups"));
    }

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Display By Groups");
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(200));
    pos.height = h = GDrawPointsToPixels(NULL,317);
    grp.gw = GDrawCreateTopWindow(NULL,&pos,displaygrp_e_h,&grp,&wattrs);

    grp.bmargin = GDrawPointsToPixels(NULL,50)+GDrawPointsToPixels(grp.gw,_GScrollBar_Width);

    GroupWCreate(&grp,&pos);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    gcd[0].gd.pos.width = -1;
    gcd[0].gd.pos.x = 30;
    gcd[0].gd.pos.y = h-GDrawPointsToPixels(NULL,30);
    gcd[0].gd.flags = gg_visible | gg_enabled | gg_but_default | gg_pos_in_pixels;
    label[0].text = (unichar_t *) _("_OK");
    label[0].text_is_1byte = true;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].creator = GButtonCreate;

    gcd[1].gd.pos.width = -1;
    gcd[1].gd.pos.x = (pos.width-30-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor));
    gcd[1].gd.pos.y = gcd[0].gd.pos.y+3;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_cancel | gg_pos_in_pixels;
    label[1].text = (unichar_t *) _("_Cancel");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].creator = GButtonCreate;

    gcd[2].gd.pos.width = -1;
    gcd[2].gd.pos.x = 10;
    gcd[2].gd.pos.y = gcd[0].gd.pos.y-GDrawPointsToPixels(NULL,17);
    gcd[2].gd.flags = gg_visible | gg_enabled | gg_cb_on | gg_pos_in_pixels;
    label[2].text = (unichar_t *) _("Compacted");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].creator = GCheckBoxCreate;    

    GGadgetsCreate(grp.gw,gcd);
    grp.ok = gcd[0].ret;
    grp.cancel = gcd[1].ret;
    grp.compact = gcd[2].ret;

    GroupSBSizes(&grp);

    GDrawSetVisible(grp.gw,true);

    while ( !grp.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetUserData(grp.gw,NULL);
    if ( grp.oked )
	EncodeToGroups(fv,grp.root, GGadgetIsChecked(gcd[2].ret));
    if ( grp.root!=group_root )
	GroupFree(grp.root);
    GDrawDestroyWindow(grp.gw);
}
