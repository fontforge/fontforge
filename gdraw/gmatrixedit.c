/* Copyright (C) 2006-2012 by George Williams */
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
#include "gdraw.h"
#include "../gdraw/gdrawP.h"
#include "gkeysym.h"
#include "ggadgetP.h"
#include "gwidget.h"
#include "gresource.h"
#include <string.h>
#include <ustring.h>
#include <ffglib.h>
#include <glib/gprintf.h>
#include "xvasprintf.h"

#define DEL_SPACE	6

static GBox gmatrixedit_box = GBOX_EMPTY; /* Don't initialize here */
static GBox gmatrixedit_button_box = GBOX_EMPTY; /* Don't initialize here */
static FontInstance *gmatrixedit_font = NULL, *gmatrixedit_titfont = NULL;
static Color gmatrixedit_title_bg = 0x808080, gmatrixedit_title_fg = 0x000000, gmatrixedit_title_divider = 0xffffff;
static Color gmatrixedit_rules = 0x000000;
static Color gmatrixedit_frozencol = 0xff0000,
	gmatrixedit_activecol = 0x0000ff, gmatrixedit_activebg=0xffffc0;
static int gmatrixedit_inited = false;

static struct resed gmatrixedit_re[] = {
    {N_("Title Background"), "TitleBG", rt_color, &gmatrixedit_title_bg, N_("Background color of column headers at the top of a matrix edit"), NULL, { 0 }, 0, 0 },
    {N_("Title Text Color"), "TitleFG", rt_color, &gmatrixedit_title_fg, N_("Text color of column headers at the top of a matrix edit"), NULL, { 0 }, 0, 0 },
    {N_("Title Divider Color"), "TitleDivider", rt_color, &gmatrixedit_title_divider, N_("Color of column dividers in the title section of a matrix edit"), NULL, { 0 }, 0, 0 },
    {N_("Rule Color"), "RuleCol", rt_color, &gmatrixedit_rules, N_("Color of column dividers in the main section of a matrix edit"), NULL, { 0 }, 0, 0 },
    {N_("Frozen Color"), "FrozenCol", rt_color, &gmatrixedit_frozencol, N_("Color of frozen (unchangeable) entries in the main section of a matrix edit"), NULL, { 0 }, 0, 0 },
    {N_("Active Color"), "ActiveCol", rt_color, &gmatrixedit_activecol, N_("Color of the active entry in the main section of a matrix edit"), NULL, { 0 }, 0, 0 },
    {N_("Active Background"), "ActiveBG", rt_color, &gmatrixedit_activebg, N_("Background color of the active entry in the main section of a matrix edit"), NULL, { 0 }, 0, 0 },
    {N_("Title Font"), "TitleFont", rt_font, &gmatrixedit_titfont, N_("Font used to draw titles of a matrix edit"), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};
static GResInfo gmatrixedit_ri = {
    NULL, &ggadget_ri, NULL,NULL,
    &gmatrixedit_box,
    &gmatrixedit_font,
    NULL,
    gmatrixedit_re,
    N_("Matrix Edit"),
    N_("Matrix Edit (sort of like a spreadsheet)"),
    "GMatrixEdit",
    "Gdraw",
    false,
    omf_border_type|omf_border_width|omf_border_shape|omf_padding|
	omf_main_background|omf_disabled_background,
    NULL,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
static GResInfo gmatrixedit2_ri = {
    NULL, &ggadget_ri, &gmatrixedit_ri,NULL,
    NULL,
    NULL,
    NULL,
    gmatrixedit_re,
    N_("Matrix Edit Continued"),
    N_("Matrix Edit (sort of like a spreadsheet)"),
    "GMatrixEdit",
    "Gdraw",
    false,
    0,
    NULL,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};

static void _GMatrixEdit_Init(void) {
    FontRequest rq;

    if ( gmatrixedit_inited )
return;
    _GGadgetCopyDefaultBox(&gmatrixedit_box);
    gmatrixedit_box.border_type = bt_none;
    gmatrixedit_box.border_width = 0;
    gmatrixedit_box.border_shape = bs_rect;
    gmatrixedit_box.padding = 0;
    /*gmatrixedit_box.flags = 0;*/
    gmatrixedit_box.main_background = COLOR_TRANSPARENT;
    gmatrixedit_box.disabled_background = COLOR_TRANSPARENT;
    GDrawDecomposeFont(_ggadget_default_font,&rq);
    gmatrixedit_font = GDrawInstanciateFont(NULL,&rq);
    gmatrixedit_font = _GGadgetInitDefaultBox("GMatrixEdit.",&gmatrixedit_box,gmatrixedit_font);
    GDrawDecomposeFont(gmatrixedit_font,&rq);
    rq.point_size = (rq.point_size>=12) ? rq.point_size-2 : rq.point_size>=10 ? rq.point_size-1 : rq.point_size;
    rq.weight = 700;
    gmatrixedit_titfont = GResourceFindFont("GMatrixEdit.TitleFont",GDrawInstanciateFont(NULL,&rq));
    gmatrixedit_title_bg = GResourceFindColor("GMatrixEdit.TitleBG",gmatrixedit_title_bg);
    gmatrixedit_title_fg = GResourceFindColor("GMatrixEdit.TitleFG",gmatrixedit_title_fg);
    gmatrixedit_title_divider = GResourceFindColor("GMatrixEdit.TitleDivider",gmatrixedit_title_divider);
    gmatrixedit_rules = GResourceFindColor("GMatrixEdit.RuleCol",gmatrixedit_rules);
    gmatrixedit_frozencol = GResourceFindColor("GMatrixEdit.FrozenCol",gmatrixedit_frozencol);
    gmatrixedit_activecol = GResourceFindColor("GMatrixEdit.ActiveCol",gmatrixedit_activecol);
    gmatrixedit_activebg = GResourceFindColor("GMatrixEdit.ActiveBG",gmatrixedit_activebg);
    gmatrixedit_inited = true;

    _GGadgetCopyDefaultBox(&gmatrixedit_button_box);
    gmatrixedit_button_box.border_width = 1;
    gmatrixedit_button_box.flags |= box_foreground_border_inner;
    gmatrixedit_button_box.main_background = gmatrixedit_box.main_background;
    gmatrixedit_button_box.disabled_background = gmatrixedit_box.disabled_background;
    _GGadgetInitDefaultBox("GMatrixEditButton.",&gmatrixedit_button_box,NULL);
}

static void MatrixDataFree(GMatrixEdit *gme) {
    int r,c;

    for ( r=0; r<gme->rows; ++r ) for ( c=0; c<gme->cols; ++c ) {
	if ( gme->col_data[c].me_type == me_string ||
		gme->col_data[c].me_type == me_bigstr ||
		gme->col_data[c].me_type == me_stringchoice ||
		gme->col_data[c].me_type == me_stringchoicetrans ||
		gme->col_data[c].me_type == me_stringchoicetag ||
		gme->col_data[c].me_type == me_funcedit ||
		gme->col_data[c].me_type == me_onlyfuncedit ||
		gme->col_data[c].me_type == me_button ||
		gme->col_data[c].me_type == me_func )
	    free( gme->data[r*gme->cols+c].u.md_str );
    }
    free( gme->data );
}

static void GMatrixEdit_destroy(GGadget *g) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
    int c, i;

    free(gme->newtext);
    /* The textfield gme->tf lives in the nested window and doesn't need to be destroyed */
    if ( gme->vsb!=NULL )
	GGadgetDestroy(gme->vsb);
    if ( gme->hsb!=NULL )
	GGadgetDestroy(gme->hsb);
    if ( gme->del!=NULL )
	GGadgetDestroy(gme->del);
    if ( gme->up!=NULL )
	GGadgetDestroy(gme->up);
    if ( gme->down!=NULL )
	GGadgetDestroy(gme->down);
    if ( gme->buttonlist!=NULL )
	for ( i=0; gme->buttonlist[i]!=NULL; ++i )
	    GGadgetDestroy(gme->buttonlist[i]);
    if ( gme->nested!=NULL ) {
	GDrawSetUserData(gme->nested,NULL);
	GDrawDestroyWindow(gme->nested);
    }

    MatrixDataFree(gme);	/* Uses col data */

    for ( c=0; c<gme->cols; ++c ) {
	if ( gme->col_data[c].enum_vals!=NULL )
	    GMenuItemArrayFree(gme->col_data[c].enum_vals);
	free( gme->col_data[c].title );
    }
    free( gme->col_data );

    _ggadget_destroy(g);
}

static void GME_FixScrollBars(GMatrixEdit *gme) {
    int width;
    int lastc;
    int pagesize = gme->vsb->r.height/(gme->fh+gme->vpad);
    if ( pagesize<=0 ) pagesize=1;

	/* Editable matrixedits need one extra line, for the <New> button */
    GScrollBarSetBounds(gme->vsb,0,gme->rows+!gme->no_edit,pagesize);
    for (lastc=gme->cols-1; lastc>=0 && gme->col_data[lastc].hidden; lastc--);
    width = gme->col_data[lastc].x + gme->col_data[lastc].width;
    GScrollBarSetBounds(gme->hsb,0,width,gme->hsb->r.width);
}

static void GMatrixEdit_Move(GGadget *g, int32 x, int32 y) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
    int i;

    /* I don't need to move the textfield because it is */
    /* nested within the window, and I'm moving the window */
    if ( gme->vsb!=NULL )
	_ggadget_move((GGadget *) (gme->vsb),x+(gme->vsb->r.x-g->r.x),
					     y+(gme->vsb->r.y-g->r.y));
    if ( gme->hsb!=NULL )
	_ggadget_move((GGadget *) (gme->hsb),x+(gme->hsb->r.x-g->r.x),
					     y+(gme->hsb->r.y-g->r.y));
    if ( gme->del!=NULL )
	_ggadget_move((GGadget *) (gme->del),x+(gme->del->r.x-g->r.x),
					     y+(gme->del->r.y-g->r.y));
    if ( gme->up!=NULL )
	_ggadget_move((GGadget *) (gme->up),x+(gme->up->r.x-g->r.x),
					     y+(gme->up->r.y-g->r.y));
    if ( gme->down!=NULL )
	_ggadget_move((GGadget *) (gme->down),x+(gme->down->r.x-g->r.x),
					     y+(gme->down->r.y-g->r.y));
    if ( gme->buttonlist!=NULL )
	for ( i=0; gme->buttonlist[i]!=NULL; ++i )
	    if ( gme->buttonlist[i]!=NULL )
		_ggadget_move((GGadget *) (gme->buttonlist[i]),x+(gme->buttonlist[i]->r.x-g->r.x),
						     y+(gme->buttonlist[i]->r.y-g->r.y));
    GDrawMove(gme->nested,x+(g->inner.x-g->r.x),y+(g->inner.y-g->r.y+
	    (gme->has_titles?gme->fh:0)));
    _ggadget_move(g,x,y);
}

static GMenuItem *FindMi(GMenuItem *mi, intpt val ) {
    int i;

    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.line; ++i ) {
	if ( mi[i].ti.userdata == (void *) (intpt) val && mi[i].ti.text!=NULL )
return( &mi[i] );
    }
return( NULL );
}

static int GME_ColWidth(GMatrixEdit *gme, int c) {
    int width=0, max, cur;
    FontInstance *old = GDrawSetFont(gme->g.base,gme->font);
    char *str, *pt;
    int r;
    GMenuItem *mi;

    if ( gme->col_data[c].hidden )
return( 0 );
    switch ( gme->col_data[c].me_type ) {
      case me_int:
	width = GDrawGetText8Width(gme->g.base,"1234", -1);
      break;
      case me_hex: case me_addr:
	width = GDrawGetText8Width(gme->g.base,"0xFFFF", -1);
      break;
      case me_uhex:
	width = GDrawGetText8Width(gme->g.base,"U+FFFF", -1);
      break;
      case me_real:
	width = GDrawGetText8Width(gme->g.base,"1.234567", -1);
      break;
      case me_enum:
	max = 0;
	for ( r=0; r<gme->rows; ++r ) {
	    mi = FindMi(gme->col_data[c].enum_vals,gme->data[r*gme->cols+c].u.md_ival);
	    if ( mi!=NULL ) {
		if ( mi->ti.text_is_1byte )
		    cur = GDrawGetText8Width(gme->g.base,(char *)mi->ti.text,-1);
		else
		    cur = GDrawGetTextWidth(gme->g.base,mi->ti.text,-1);
		if ( cur>max ) max = cur;
	    }
	}
	cur = 6 * GDrawGetText8Width(gme->g.base,"n", 1);
	if ( max<cur )
	    max = cur;
	width = max;
      break;
      case me_func:
      case me_button:
      case me_stringchoice: case me_stringchoicetrans: case me_stringchoicetag:
      case me_funcedit:
      case me_onlyfuncedit:
      case me_string: case me_bigstr:
	max = 0;
	for ( r=0; r<gme->rows; ++r ) {
	    char *freeme = NULL;
	    str = gme->data[r*gme->cols+c].u.md_str;
	    if ( str==NULL && gme->col_data[c].me_type==me_func )
		str = freeme = (gme->col_data[c].func)(&gme->g,r,c);
	    if ( str==NULL )
	continue;
	    /* use the maximum width of 40 characters to avoid insanely wide
	     * cells and horizontal scrollbars, the magic number 40 is the max
	     * length of characters after which we use GME_StrBigEdit below */
	    char buf[1024];
	    utf8_strncpy(buf, str, 40);
	    pt = strchr(buf,'\n');
	    cur = GDrawGetText8Width(gme->g.base,buf, pt==NULL ? -1: pt-buf);
	    if ( cur>max ) max = cur;
	    free(freeme);
	}
	if ( max < 10*GDrawGetText8Width(gme->g.base,"n", 1) )
	    width = 10*GDrawGetText8Width(gme->g.base,"n", 1);
	else
	    width = max;
	if ( gme->col_data[c].me_type==me_stringchoice ||
		gme->col_data[c].me_type==me_stringchoicetrans ||
		gme->col_data[c].me_type==me_stringchoicetag ||
		gme->col_data[c].me_type==me_onlyfuncedit ||
		gme->col_data[c].me_type==me_funcedit )
	    width += gme->mark_size + gme->mark_skip;
      break;
      default:
	width = 0;
      break;
    }
    if ( gme->col_data[c].title!=NULL ) {
	GDrawSetFont(gme->g.base,gme->titfont);
	cur = GDrawGetText8Width(gme->g.base,gme->col_data[c].title, -1);
	if ( cur>width ) width = cur;
    }
    GDrawSetFont(gme->g.base,old);
    if ( width>0x7fff )
	width = 0x7fff;
return( width );
}

static void GME_RedrawTitles(GMatrixEdit *gme);
static int GME_AdjustCol(GMatrixEdit *gme,int col) {
    int new_width, x,c, changed;
    int orig_width, min_width;
    int lastc;

    changed = false;
    if ( col==-1 ) {
	for ( c=0; c<gme->cols; ++c ) if ( !gme->col_data[c].fixed ) {
	    new_width = GME_ColWidth(gme,c);
	    if ( new_width!=gme->col_data[c].width ) {
		gme->col_data[c].width = new_width;
		changed = true;
	    }
	}
	col = 0;
    } else if ( !gme->col_data[col].fixed ) {
	new_width = GME_ColWidth(gme,col);
	if ( new_width!=gme->col_data[col].width ) {
	    gme->col_data[col].width = new_width;
	    changed = true;
	}
    }
    if ( changed ) {
	x = gme->col_data[col].x;
	for ( c=col; c<gme->cols; ++c ) {
	    gme->col_data[c].x = x;
	    if ( !gme->col_data[c].hidden )
		x += gme->col_data[c].width + gme->hpad;
	}
    }
    for ( lastc=gme->cols-1; lastc>0 && gme->col_data[lastc].hidden; --lastc );
    if ( !gme->col_data[lastc].fixed ) {
	orig_width = gme->col_data[lastc].width;
	min_width = GME_ColWidth(gme,lastc);
	gme->col_data[lastc].width = (gme->g.inner.width-gme->vsb->r.width-gme->hpad-gme->col_data[lastc].x);
	if ( gme->col_data[lastc].width<min_width )
	    gme->col_data[lastc].width = min_width;
	if ( gme->col_data[lastc].width != orig_width )
	    changed = true;
    }

    if ( changed ) {
	GME_FixScrollBars(gme);
	GDrawRequestExpose(gme->nested,NULL,false);
	GME_RedrawTitles(gme);
    }
return( changed );
}

static void GMatrixEdit_SetDesiredSize(GGadget *g,GRect *outer,GRect *inner) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
    int bp = GBoxBorderWidth(g->base,g->box);

    if ( outer!=NULL ) {
	g->desired_width = outer->width;
	g->desired_height = outer->height;
    } else if ( inner!=NULL ) {
	int extra = 2*bp+ gme->hsb->r.height+ (gme->has_titles?gme->fh:0);
	if ( gme->del )
	    extra += gme->del->r.height+DEL_SPACE;
	g->desired_width = inner->width<=0 ? -1 : inner->width+2*bp+gme->vsb->r.width;
	g->desired_height = inner->height<=0 ? -1 :
		inner->height<10 ? inner->height*(gme->fh + gme->vpad) + extra :
		    inner->height+extra;
    }
}

static void GMatrixEdit_GetDesiredSize(GGadget *g,GRect *outer,GRect *inner) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
    int width, height;
    int bp = GBoxBorderWidth(g->base,g->box);
    int c, rows, i;
    FontInstance *old = GDrawSetFont(gme->g.base,gme->font);
    int sbwidth = GDrawPointsToPixels(g->base,_GScrollBar_Width);
    int butwidth = 0;

    width = 1;
    for ( c=0; c<gme->cols; ++c ) {
	width += GME_ColWidth(gme,c);
	if ( c!=gme->cols-1 )
	    width += gme->hpad;
    }
    GDrawSetFont(gme->g.base,old);
    width += sbwidth;

    rows = (gme->rows<4) ? 4 : (gme->rows>20) ? 21 : gme->rows+1;
    height = rows * (gme->fh + gme->vpad);

    if ( gme->has_titles )
	height += gme->fh;
    height += sbwidth;
    if ( gme->del ) {
	height += gme->del->r.height+DEL_SPACE;
	butwidth += gme->del->r.width + 10;
    }
    if ( gme->up && gme->up->state!=gs_invisible )
	butwidth += gme->up->r.width+5;
    if ( gme->down && gme->down->state!=gs_invisible )
	butwidth += gme->down->r.width+5;
    if ( gme->buttonlist!=NULL )
	for ( i=0; gme->buttonlist[i]!=NULL; ++i )
	    if ( gme->buttonlist[i] && gme->buttonlist[i]->state!=gs_invisible )
		butwidth += gme->buttonlist[i]->r.width+5;
    if ( butwidth > width )
	width = butwidth;

    if ( g->desired_width>2*bp )
	width = g->desired_width-2*bp;
    if ( g->desired_height>2*bp )
	height = g->desired_height-2*bp;
    if ( inner!=NULL ) {
	inner->x = inner->y = 0;
	inner->width = width;
	inner->height = height;
    }
    if ( outer!=NULL ) {
	outer->x = outer->y = 0;
	outer->width = width + 2*bp;
	outer->height = height + 2*bp;
    }
}

static void GME_PositionEdit(GMatrixEdit *gme);
static void GMatrixEdit_Resize(GGadget *g, int32 width, int32 height ) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
    int bp = GBoxBorderWidth(g->base,g->box);
    int subheight, subwidth;
    /*int plus, extra,x,c;*/
    int bcnt, i, min_width;
    GRect wsize;

    width -= 2*bp; height -= 2*bp;

    subheight = height -
	    (gme->no_edit && gme->buttonlist==NULL?0:(gme->del->r.height+DEL_SPACE)) -
	    (gme->has_titles?gme->fh:0) -
	    gme->hsb->r.height;
    subwidth = width - gme->vsb->r.width;
    GDrawResize(gme->nested,subwidth, subheight);
    /* Make sure the dimensions of the internal window are properly set. */
    /*  We need this because GDrawResize() just notifies WM about the size */
    /*  changes, but doesn't update the data structures used by FF internally. */
    /*  This may result into list marks/functional buttons being incorrectly */
    /*  positioned in their matrix cells when the dialog is displayed. */
    GDrawGetSize(gme->nested, &wsize);
    wsize.width  = subwidth;
    wsize.height = subheight;
    gme->nested->pos = wsize;

    GGadgetResize(gme->vsb,gme->vsb->r.width,subheight);
    GGadgetMove(gme->vsb,gme->g.inner.x + width-2*bp-gme->vsb->r.width,
			    gme->vsb->r.y);
    GGadgetResize(gme->hsb,subwidth,gme->hsb->r.height);
    GGadgetMove(gme->hsb,gme->g.inner.x,
			 gme->g.inner.y + height - (gme->del->r.height+DEL_SPACE) - gme->hsb->r.height);
    GME_FixScrollBars(gme);

    bcnt = 1;	/* delete */
    if ( gme->up && gme->up->state!=gs_invisible )
	bcnt += 2;
    if ( gme->buttonlist!=NULL ) {
	for ( i=0; gme->buttonlist[i]!=NULL; ++i )
	    if ( gme->buttonlist[i]->state!=gs_invisible )
		++bcnt;
    }
    if ( bcnt==1 && gme->no_edit ) {
	/* No delete button to display */
    } else if ( bcnt==1 ) {
	GGadgetMove(gme->del,gme->g.inner.x + (width-gme->del->r.width)/2,
				 gme->g.inner.y + height - (gme->del->r.height+DEL_SPACE/2));
    } else {
	int y = gme->g.inner.y + height - (gme->del->r.height+DEL_SPACE/2);
	int x = gme->g.inner.x + width-5;
	GGadgetMove(gme->del,gme->g.inner.x + 5, y);
	if ( gme->up && gme->up->state!=gs_invisible ) {
	    x -= gme->down->r.width;
	    GGadgetMove(gme->down, x, y);
	    x -= 5 + gme->up->r.width;
	    GGadgetMove(gme->up, x, y);
	    x -= 10;
	}
	if ( gme->buttonlist!=NULL ) {
	    for ( i=0; gme->buttonlist[i]!=NULL; ++i )
		if ( gme->buttonlist[i]->state!=gs_invisible ) {
		    x -= gme->buttonlist[i]->r.width;
		    GGadgetMove(gme->buttonlist[i], x, y);
		    x -= 5;
		}
	}
    }

    /* I thought to leave the columns as they are, but that looks odd */
    /*  for the last column. Instead put all the extra space in the */
    /*  last column, but give it some minimal size */
    min_width = GME_ColWidth(gme,gme->cols-1);
    gme->col_data[gme->cols-1].width = (subwidth-gme->hpad-gme->col_data[gme->cols-1].x);
    if ( gme->col_data[gme->cols-1].width<min_width )
	gme->col_data[gme->cols-1].width = min_width;
    GME_FixScrollBars(gme);
    _ggadget_resize(g,width+2*bp, height+2*bp);
    GME_PositionEdit(gme);
    GDrawRequestExpose(gme->nested,NULL,false);
}

static int GMatrixEdit_Mouse(GGadget *g, GEvent *event) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
    int c, nw, i, x, ex = event->u.mouse.x + gme->off_left;

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7)) {
	    int isv = event->u.mouse.button<=5;
	if ( event->u.mouse.state&ksm_shift ) isv = !isv;
	if ( isv && gme->vsb!=NULL )
return( GGadgetDispatchEvent(gme->vsb,event));
	else if ( !isv && gme->hsb!=NULL )
return( GGadgetDispatchEvent(gme->hsb,event));
	else
return( true );
    }

    if ( gme->pressed_col>=0 && (event->type==et_mouseup || event->type==et_mousemove)) {
	c = gme->pressed_col;
	nw = (ex-gme->g.inner.x-gme->col_data[c].x-gme->hpad/2);
	if ( ex-gme->g.inner.x < gme->col_data[c].x ) {
	    if ( nw<=0 )
		nw = 1;
	}
	nw = (ex-gme->g.inner.x-gme->col_data[c].x-gme->hpad/2);
	x = gme->col_data[c].x;
	for ( i=c; i<gme->cols; ++i ) {
	    gme->col_data[i].x = x;
	    x += gme->col_data[i].width + gme->hpad;
	}
	gme->col_data[c].width = nw;
	if ( event->type==et_mouseup )
	    GME_FixScrollBars(gme);
	GME_RedrawTitles(gme);
	GME_PositionEdit(gme);
	GDrawRequestExpose(gme->nested,NULL,false);
	if ( event->type==et_mouseup ) {
	    GDrawSetCursor(g->base,ct_pointer);
	    gme->pressed_col = -1;
	}
return( true );
    }

    if ( !gme->has_titles ||
	    event->u.mouse.x< gme->hsb->r.x || event->u.mouse.x >= gme->hsb->r.x+gme->hsb->r.width ||
	    event->u.mouse.y< gme->g.inner.y || event->u.mouse.y>=gme->g.inner.y+gme->fh ) {
	if ( gme->lr_pointer ) {
	    gme->lr_pointer = false;
	    GDrawSetCursor(g->base,ct_pointer);
	}
return( false );
    }
    for ( c=0; c<gme->cols; ++c ) {
	if ( ex>= gme->g.inner.x + gme->col_data[c].x+gme->col_data[c].width+gme->hpad/2-4 &&
		ex<= gme->g.inner.x + gme->col_data[c].x+gme->col_data[c].width+gme->hpad/2+4 )
    break;
    }
    if ( c==gme->cols ) {
	if ( gme->lr_pointer ) {
	    gme->lr_pointer = false;
	    GDrawSetCursor(g->base,ct_pointer);
	}
    } else {
	if ( !gme->lr_pointer ) {
	    gme->lr_pointer = true;
	    GDrawSetCursor(g->base,ct_4way);
	}
	if ( event->type == et_mousedown )
	    gme->pressed_col = c;
    }
return( true );
}

static int GMatrixEdit_Expose(GWindow pixmap, GGadget *g, GEvent *event) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
    GRect r, old, older;
    int c, y, lastc;
    Color fg = gmatrixedit_title_fg;

    if ( gme->g.state!=gs_enabled )
	fg = gme->g.box->disabled_foreground;

    GBoxDrawBorder(pixmap,&g->r,g->box,g->state,false);
    if ( gme->has_titles ) {
	r = gme->g.inner;
	r.height = gme->font_fh;
	r.width = gme->hsb->r.width;
	GDrawPushClip(pixmap,&r,&older);
	GDrawFillRect(pixmap,&r,gmatrixedit_title_bg);
	y = r.y + gme->font_as;
	GDrawSetFont(pixmap,gme->titfont);
	for ( lastc = gme->cols-1; lastc>0 && gme->col_data[lastc].hidden; --lastc );
	for ( c=0; c<gme->cols; ++c ) {
	    if ( gme->col_data[c].title!=NULL &&
		    !gme->col_data[c].hidden ) {
		r.x = gme->col_data[c].x + gme->g.inner.x - gme->off_left;
		r.width = gme->col_data[c].width;
		GDrawPushClip(pixmap,&r,&old);
		GDrawDrawText8(pixmap,r.x,y,gme->col_data[c].title,-1,fg);
		GDrawPopClip(pixmap,&old);
	    }
	    if ( c!=lastc && !gme->col_data[c].hidden)
		GDrawDrawLine(pixmap,r.x+gme->col_data[c].width+gme->hpad/2,r.y,
				     r.x+gme->col_data[c].width+gme->hpad/2,r.y+r.height,
				     gmatrixedit_title_divider);
	}
	GDrawPopClip(pixmap,&older);
    }
return( true );
}

static void GME_RedrawTitles(GMatrixEdit *gme) {
    GMatrixEdit_Expose(gme->g.base,&gme->g,NULL);
}

static void GMatrixEdit_SetVisible(GGadget *g, int visible ) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
    int i;

    if ( gme->vsb!=NULL ) _ggadget_setvisible(gme->vsb,visible);
    if ( gme->hsb!=NULL ) _ggadget_setvisible(gme->hsb,visible);
    if ( gme->del!=NULL ) _ggadget_setvisible(gme->del,visible);
    if ( gme->up!=NULL )  _ggadget_setvisible(gme->up,visible);
    if ( gme->down!=NULL ) _ggadget_setvisible(gme->down,visible);
    if ( gme->buttonlist!=NULL )
	for ( i=0; gme->buttonlist[i]!=NULL; ++i )
	    _ggadget_setvisible(gme->buttonlist[i],visible);

    GDrawSetVisible(gme->nested,visible);
    _ggadget_setvisible(g,visible);
}

static void GMatrixEdit_Redraw(GGadget *g ) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
    GDrawRequestExpose(gme->nested,NULL,false);
    _ggadget_redraw(g);
}

static char *MD_Text(GMatrixEdit *gme,int r, int c ) {
    char buffer[20], *str= NULL;
    struct matrix_data *d = &gme->data[r*gme->cols+c];

    switch ( gme->col_data[c].me_type ) {
      case me_enum:
	/* Fall through into next case */
      case me_int:
	sprintf( buffer,"%d",(int) d->u.md_ival );
	str = buffer;
      break;
      case me_hex:
	sprintf( buffer,"0x%x",(int) d->u.md_ival );
	str = buffer;
      break;
      case me_uhex:
	sprintf( buffer,"U+%04X",(int) d->u.md_ival );
	str = buffer;
      break;
      case me_addr:
	sprintf( buffer,"%p", d->u.md_addr );
	str = buffer;
      break;
      case me_real:
	sprintf( buffer,"%g",d->u.md_real );
	str = buffer;
      break;
      case me_string: case me_bigstr:
      case me_funcedit:
      case me_onlyfuncedit:
      case me_button:
      case me_stringchoice: case me_stringchoicetrans: case me_stringchoicetag:
	str = d->u.md_str;
      break;
      case me_func:
	str = d->u.md_str;
	if ( str==NULL )
return( (gme->col_data[c].func)(&gme->g,r,c) );
      break;
    }
    if (!str) str="";
return( copy(str));
}

static int GME_RecalcFH(GMatrixEdit *gme) {
    int r,c, as, ds;
    int32 end = -1;
    char *str, *ept;
    GTextBounds bounds;
    GMenuItem *mi;

    GDrawSetFont(gme->nested,gme->font);
    as = gme->font_as; ds = gme->font_fh-as;
    for ( r=0; r<gme->rows; ++r ) for ( c=0; c<gme->cols; ++c ) {
	end = -1;
	switch ( gme->col_data[c].me_type ) {
	  case me_enum:
	    mi = FindMi(gme->col_data[c].enum_vals,gme->data[r*gme->cols+c].u.md_ival);
	    if ( mi==NULL )
    continue;
	    str = copy( (char *)mi->ti.text );
	break;
	  default:
	    str = MD_Text(gme,r,c);
	    if ( str == NULL )
    continue;
	    if ( ( ept = strchr(str,'\n') ) != NULL )
		end = ept - str;
	break;
	}
	GDrawGetText8Bounds(gme->nested, str, end, &bounds);
	free(str);
	if ( bounds.as>as )
	    as = bounds.as;
	if ( bounds.ds>ds )
	    ds = bounds.ds;
    }
    if ( as!=gme->as || as+ds!=gme->fh ) {
	gme->fh = as+ds;
	gme->as = as;
return( true );
    }
return( false );
}

static void GMatrixEdit_SetFont(GGadget *g,FontInstance *new) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
    int as, ds, ld;
    gme->font = new;
    GDrawWindowFontMetrics(g->base,gme->font,&as, &ds, &ld);
    gme->font_as = gme->as = as;
    gme->font_fh = gme->fh = as+ds;
    GME_RecalcFH(gme);
    GME_FixScrollBars(gme);
    GDrawRequestExpose(gme->nested,NULL,false);
}

static FontInstance *GMatrixEdit_GetFont(GGadget *g) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
return( gme->font );
}

static int32 GMatrixEdit_IsSelected(GGadget *g, int32 pos) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

return( pos==gme->active_row );
}

static int32 GMatrixEdit_GetFirst(GGadget *g) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

return( gme->active_row );
}

static int GMatrixEdit_FillsWindow(GGadget *g) {
return( true );
}

struct gfuncs gmatrixedit_funcs = {
    0,
    sizeof(struct gfuncs),

    GMatrixEdit_Expose,
    GMatrixEdit_Mouse,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    GMatrixEdit_Redraw,
    GMatrixEdit_Move,
    GMatrixEdit_Resize,
    GMatrixEdit_SetVisible,
    _ggadget_setenabled,
    _ggadget_getsize,
    _ggadget_getinnersize,

    GMatrixEdit_destroy,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    GMatrixEdit_SetFont,
    GMatrixEdit_GetFont,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    GMatrixEdit_IsSelected,
    GMatrixEdit_GetFirst,
    NULL,
    NULL,
    NULL,

    GMatrixEdit_GetDesiredSize,
    GMatrixEdit_SetDesiredSize,
    GMatrixEdit_FillsWindow,
    NULL
};

static void GME_PositionEdit(GMatrixEdit *gme) {
    int x,y,end;
    GRect wsize;
    int c = gme->active_col, r = gme->active_row, lastc;

    for ( lastc = gme->cols-1; lastc>0 && gme->col_data[lastc].hidden; --lastc );

    if ( gme->edit_active ) {
	x = gme->col_data[c].x - gme->off_left;
	y = (r-gme->off_top)*(gme->fh+gme->vpad);
	end = x + gme->col_data[c].width;

	if ( c == lastc ) {
	    GDrawGetSize(gme->nested,&wsize);
	    if ( end>wsize.width )
		end = wsize.width - x;
	    if ( gme->col_data[c].me_type==me_stringchoice ||
		    gme->col_data[c].me_type==me_stringchoicetrans ||
		    gme->col_data[c].me_type==me_stringchoicetag ||
		    gme->col_data[c].me_type==me_onlyfuncedit ||
		    gme->col_data[c].me_type==me_funcedit )
		end -= gme->mark_size+gme->mark_skip;
	}

	GGadgetResize(gme->tf,end-x,gme->fh);
	GGadgetMove(gme->tf,x,y);
    }
}

static void GME_StrSmallEdit(GMatrixEdit *gme,char *str, GEvent *event) {
    gme->edit_active = true;
    /* Shift so all of column is in window???? */
    GME_PositionEdit(gme);
    GGadgetSetTitle8(gme->tf,str);
    GGadgetSetVisible(gme->tf,true);
    GGadgetSetEnabled(gme->tf,true);
    GCompletionFieldSetCompletion(gme->tf,gme->col_data[gme->active_col].completer);
    ((GTextField *) (gme->tf))->accepts_tabs = false;
    ((GTextField *) (gme->tf))->was_completing = gme->col_data[gme->active_col].completer!=NULL;
    GWidgetIndicateFocusGadget(gme->tf);
    if ( event->type == et_mousedown )
	GGadgetDispatchEvent(gme->tf,event);
}

static int GME_SetValue(GMatrixEdit *gme,GGadget *g ) {
    int c = gme->active_col;
    int r = gme->active_row;
    intpt lval;
    double dval;
    char *end="";
    char *str = GGadgetGetTitle8(g), *pt;
    int kludge;

    switch ( gme->col_data[c].me_type ) {
      case me_enum:
	{
	    const unichar_t *ustr = _GGadgetGetTitle(g), *test;
	    int i;
	    for ( i=0; (test=gme->col_data[c].enum_vals[i].ti.text)!=NULL || gme->col_data[c].enum_vals[i].ti.line ; ++i ) {
		if ( u_strcmp(ustr,test)==0 ) {
		    if ( (intpt) gme->col_data[c].enum_vals[i].ti.userdata != GME_NoChange )
			gme->data[r*gme->cols+c].u.md_ival =
				(intpt) gme->col_data[c].enum_vals[i].ti.userdata;
		    free(str);
  goto good;
		}
	    }
	}
	/* Didn't match any of the enums try as a direct integer */
	/* Fall through */
      case me_int: case me_hex: case me_uhex: case me_addr:
	if ( gme->validatestr!=NULL )
	    end = (gme->validatestr)(&gme->g,gme->active_row,gme->active_col,gme->wasnew,str);
	if ( *end=='\0' ) {
	    if ( gme->col_data[c].me_type==me_hex || gme->col_data[c].me_type==me_uhex ) {
		pt = str;
		while ( *pt==' ' ) ++pt;
		if ( (*pt=='u' || *pt=='U') && pt[1]=='+' )
		    pt += 2;
		else if ( *pt=='0' && (pt[1]=='x' || pt[1]=='X'))
		    pt += 2;
		lval = strtoul(pt,&end,16);
	    } else
		lval = strtol(str,&end,10);
	}
	if ( *end!='\0' ) {
	    GTextFieldSelect(g,end-str,-1);
	    free(str);
	    GDrawBeep(NULL);
return( false );
	}
	if ( gme->col_data[c].me_type == me_addr )
	    gme->data[r*gme->cols+c].u.md_addr = (void *) lval;
	else
	    gme->data[r*gme->cols+c].u.md_ival = lval;
	free(str);
  goto good;
      case me_real:
	if ( gme->validatestr!=NULL )
	    end = (gme->validatestr)(&gme->g,gme->active_row,gme->active_col,gme->wasnew,str);
	if ( *end=='\0' )
	    dval = strtod(str,&end);
	if ( *end!='\0' ) {
	    GTextFieldSelect(g,end-str,-1);
	    free(str);
	    GDrawBeep(NULL);
return( false );
	}
	gme->data[r*gme->cols+c].u.md_real = dval;
	free(str);
  goto good;
      case me_stringchoice: case me_stringchoicetrans: case me_stringchoicetag:
      case me_funcedit: case me_onlyfuncedit:
      case me_string: case me_bigstr: case me_func: case me_button:
	if ( gme->validatestr!=NULL )
	    end = (gme->validatestr)(&gme->g,gme->active_row,gme->active_col,gme->wasnew,str);
	if ( *end!='\0' ) {
	    GTextFieldSelect(g,end-str,-1);
	    free(str);
	    GDrawBeep(NULL);
return( false );
	}

	free(gme->data[r*gme->cols+c].u.md_str);
	gme->data[r*gme->cols+c].u.md_str = str;
	/* Used to delete the row if this were a null string. seems extreme */
  goto good;
      default:
	/* Eh? Can't happen */
	GTextFieldSelect(g,0,-1);
	GDrawBeep(NULL);
	free(str);
return( false );
    }
  good:
    kludge = gme->edit_active; gme->edit_active = false;
    if ( gme->finishedit != NULL )
	(gme->finishedit)(&gme->g,r,c,gme->wasnew);
    gme->edit_active = kludge;
return( true );
}

static int GME_FinishEdit(GMatrixEdit *gme) {

    if ( !gme->edit_active ) {
        gme->wasnew = false;
        return( true );
    }
    if ( !GME_SetValue(gme,gme->tf)) {
        gme->wasnew = false;
        return( false );
    }
    gme->edit_active = false;
    GGadgetSetVisible(gme->tf,false);
    GME_AdjustCol(gme,gme->active_col);
    if ( GME_RecalcFH(gme) ) {
	GME_FixScrollBars(gme);
	GDrawRequestExpose(gme->nested,NULL,false);
    }

    gme->wasnew = false;
return( true );
}

/* Sometimes our data moves underneath us (if the validate function does */
/*  something weird). See if we can move the current row with the data */
static int GME_FinishEditPreserve(GMatrixEdit *gme,int r) {
    int i;

    if ( r<gme->rows ) {
	for ( i=0; i<gme->rows; ++i )
	    gme->data[i*gme->cols].current = 0;
	gme->data[r*gme->cols].current = 1;
    }
    if ( !GME_FinishEdit(gme))
return( -1 );
    if ( r==gme->rows )
return( r );
    for ( i=0; i<gme->rows; ++i )
	if ( gme->data[i*gme->cols].current )
return( i );

    /* Quite lost */
return( r );
}

static void GME_EnableDelete(GMatrixEdit *gme) {
    int enabled = false;

    if ( gme->setotherbuttons )
	(gme->setotherbuttons)(&gme->g,gme->active_row,gme->active_col);
    if ( gme->active_row>=0 && gme->active_row<gme->rows ) {
	enabled = true;
	if ( gme->candelete!=NULL && !(gme->candelete)(&gme->g,gme->active_row))
	    enabled = false;
    }
    GGadgetSetEnabled(gme->del,enabled);

    if ( gme->up!=NULL ) {
	enum gme_updown updown;
	if ( gme->canupdown != NULL )
	    updown = (gme->canupdown)((GGadget *) gme,gme->active_row);
	else {
	    updown = 0;
	    if ( gme->active_row>=1 && gme->active_row<gme->rows )
		updown = ud_up_enabled;
	    if ( gme->active_row>=0 && gme->active_row<gme->rows-1 )
		updown |= ud_down_enabled;
	}
	GGadgetSetEnabled(gme->up,updown & ud_up_enabled ? 1 : 0);
	GGadgetSetEnabled(gme->down,updown & ud_down_enabled ? 1 : 0);
    }
}

static void GME_DeleteActive(GMatrixEdit *gme) {
    int r, c;

    if ( gme->active_row==-1 || (gme->candelete && !(gme->candelete)(&gme->g,gme->active_row))) {
	GGadgetSetEnabled(gme->del,false);
	GDrawBeep(NULL);
return;
    }
    if ( gme->predelete!=NULL )
	(gme->predelete)((GGadget *) gme, gme->active_row );

    gme->edit_active = false;
    GGadgetSetVisible(gme->tf,false);
    for ( c=0; c<gme->cols; ++c ) {
	if ( gme->col_data[c].me_type == me_string || gme->col_data[c].me_type == me_bigstr ||
		gme->col_data[c].me_type == me_func || gme->col_data[c].me_type == me_funcedit ||
		gme->col_data[c].me_type == me_onlyfuncedit ||
		gme->col_data[c].me_type == me_button ||
		gme->col_data[c].me_type == me_stringchoice ||
		gme->col_data[c].me_type == me_stringchoicetag ||
		gme->col_data[c].me_type == me_stringchoicetrans ) {
	    free(gme->data[gme->active_row*gme->cols+c].u.md_str);
	    gme->data[gme->active_row*gme->cols+c].u.md_str = NULL;
	}
    }
    for ( r=gme->active_row+1; r<gme->rows; ++r )
	memcpy(gme->data+(r-1)*gme->cols,gme->data+r*gme->cols,
		gme->cols*sizeof(struct matrix_data));
    --gme->rows;
    gme->active_col = -1;
    if ( gme->active_row>=gme->rows ) gme->active_row = -1;
    GScrollBarSetBounds(gme->vsb,0,gme->rows,gme->vsb->inner.height/gme->fh);
    GDrawRequestExpose(gme->nested,NULL,false);
    GME_EnableDelete(gme);
}

void GMatrixEditUp(GGadget *g) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
    GRect r;
    int i;

    if ( gme->active_row<1 || gme->active_row>=gme->rows )
return;
    for ( i=0; i<gme->cols; ++i ) {
	struct matrix_data md = gme->data[gme->active_row*gme->cols+i];
	gme->data[gme->active_row*gme->cols+i] = gme->data[(gme->active_row-1)*gme->cols+i];
	gme->data[(gme->active_row-1)*gme->cols+i] = md;
    }
    --gme->active_row;;
    GGadgetGetSize(gme->tf,&r);
    GGadgetMove(gme->tf,r.x,r.y-(gme->fh+1));
    GME_EnableDelete(gme);
    if ( gme->rowmotion!=NULL )
	(gme->rowmotion)((GGadget *) gme, gme->active_row+1,gme->active_row);
    GMatrixEditScrollToRowCol(&gme->g,gme->active_row,gme->active_col);
}

static int _GME_Up(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GMatrixEditUp( g->data );
    }
return( true );
}

void GMatrixEditDown(GGadget *g) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
    GRect r;
    int i;

    if ( gme->active_row<0 || gme->active_row>=gme->rows-1 )
return;
    for ( i=0; i<gme->cols; ++i ) {
	struct matrix_data md = gme->data[gme->active_row*gme->cols+i];
	gme->data[gme->active_row*gme->cols+i] = gme->data[(gme->active_row+1)*gme->cols+i];
	gme->data[(gme->active_row+1)*gme->cols+i] = md;
    }
    ++gme->active_row;;
    GGadgetGetSize(gme->tf,&r);
    GGadgetMove(gme->tf,r.x,r.y-(gme->fh+1));
    GME_EnableDelete(gme);
    if ( gme->rowmotion!=NULL )
	(gme->rowmotion)((GGadget *) gme, gme->active_row-1,gme->active_row);
    GMatrixEditScrollToRowCol(&gme->g,gme->active_row,gme->active_col);
return;
}

static int _GME_Down(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GMatrixEditDown( g->data );
    }
return( true );
}

#define CID_OK		1001
#define CID_Cancel	1002
#define CID_EntryField	1011

static int big_e_h(GWindow gw, GEvent *event) {
    GMatrixEdit *gme = GDrawGetUserData(gw);

    if ( event->type==et_close ) {
	gme->big_done = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	gme->big_done = true;
	if ( GGadgetGetCid(event->u.control.g)==CID_OK ) {
	    gme->big_done = GME_SetValue(gme,GWidgetGetControl(gw,CID_EntryField) );
	    if ( gme->big_done )
		GME_AdjustCol(gme,gme->active_col);
	} else if ( gme->wasnew ) {
	    /* They canceled a click which produced a new row */
	    GME_DeleteActive(gme);
	    gme->wasnew = false;
	}
    }
return( true );
}

static void GME_StrBigEdit(GMatrixEdit *gme,char *str) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData mgcd[6], boxes[3], *varray[5], *harray[6];
    GTextInfo mlabel[6];
    char *title_str = NULL;

    if ( gme->bigedittitle!=NULL )
	title_str = (gme->bigedittitle)(&(gme->g),gme->active_row,gme->active_col);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = title_str==NULL ? "Editing..." : title_str;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(500));
    pos.height = GDrawPointsToPixels(NULL,400);
    gme->big_done = 0;
    gw = GDrawCreateTopWindow(NULL,&pos,big_e_h,gme,&wattrs);
    free(title_str);

    memset(&mgcd,0,sizeof(mgcd));
    memset(&boxes,0,sizeof(boxes));
    memset(&mlabel,0,sizeof(mlabel));
    mgcd[0].gd.pos.x = 4; mgcd[0].gd.pos.y = 6;
    mgcd[0].gd.pos.width = 492;
    mgcd[0].gd.pos.height = 260;
    mgcd[0].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap | gg_text_xim;
    mgcd[0].gd.cid = CID_EntryField;
    mgcd[0].creator = GTextAreaCreate;
    varray[0] = &mgcd[0]; varray[1] = NULL;
    varray[2] = &boxes[2]; varray[3] = NULL;
    varray[4] = NULL;

    mgcd[1].gd.pos.x = 30-3; mgcd[1].gd.pos.y = GDrawPixelsToPoints(NULL,pos.height)-35-3;
    mgcd[1].gd.pos.width = -1; mgcd[1].gd.pos.height = 0;
    mgcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    if ( _ggadget_use_gettext ) {
	mlabel[1].text = (unichar_t *) _("_OK");
	mlabel[1].text_is_1byte = true;
    } else
	mlabel[1].text = (unichar_t *) _STR_OK;
    mlabel[1].text_in_resource = true;
    mgcd[1].gd.label = &mlabel[1];
    mgcd[1].gd.cid = CID_OK;
    mgcd[1].creator = GButtonCreate;

    mgcd[2].gd.pos.x = -30; mgcd[2].gd.pos.y = mgcd[1].gd.pos.y+3;
    mgcd[2].gd.pos.width = -1; mgcd[2].gd.pos.height = 0;
    mgcd[2].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    if ( _ggadget_use_gettext ) {
	mlabel[2].text = (unichar_t *) _("_Cancel");
	mlabel[2].text_is_1byte = true;
    } else
	mlabel[2].text = (unichar_t *) _STR_Cancel;
    mlabel[2].text_in_resource = true;
    mgcd[2].gd.label = &mlabel[2];
    mgcd[2].gd.cid = CID_Cancel;
    mgcd[2].creator = GButtonCreate;
    harray[0] = GCD_Glue; harray[1] = &mgcd[1];
    harray[2] = GCD_Glue; harray[3] = &mgcd[2];
    harray[4] = GCD_Glue; harray[5] = NULL;

    boxes[2].gd.flags = gg_visible | gg_enabled;
    boxes[2].gd.u.boxelements = harray;
    boxes[2].creator = GHBoxCreate;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_visible | gg_enabled;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,0);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);
    GGadgetSetTitle8(mgcd[0].ret,str);
    GTextFieldSelect(mgcd[0].ret,0,0);
    GTextFieldShow(mgcd[0].ret,0);

    GDrawSetVisible(gw,true);
    while ( !gme->big_done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    GDrawRequestExpose(gme->nested,NULL,false);

    gme->wasnew = false;
}

static void GME_EnumDispatch(GWindow gw, GMenuItem *mi, GEvent *e) {
    GMatrixEdit *gme = GDrawGetUserData(gw);

    if ( (intpt) mi->ti.userdata == GME_NoChange )
return;

    gme->data[gme->active_row*gme->cols+gme->active_col].u.md_ival = (intpt) mi->ti.userdata;

    if ( gme->finishedit != NULL )
	(gme->finishedit)(&gme->g,gme->active_row,gme->active_col,gme->wasnew);
    GME_AdjustCol(gme,gme->active_col);
    gme->wasnew = false;
}

static void GME_FinishChoice(GWindow gw) {
    GMatrixEdit *gme = GDrawGetUserData(gw);

    /* If wasnew is still set then they didn't pick anything, so remove the row */
    if ( gme->wasnew && gme->active_col==0 )
	GME_DeleteActive(gme);
    gme->wasnew = false;
    GDrawRequestExpose(gme->nested,NULL,false);
}

static void GME_Choices(GMatrixEdit *gme,GEvent *event,int r,int c) {
    GMenuItem *mi = gme->col_data[c].enum_vals;
    int val = gme->data[r*gme->cols+c].u.md_ival;
    int i;

    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.line || mi[i].ti.image!=NULL; ++i )
	mi[i].ti.selected = mi[i].ti.checked = !gme->wasnew && (mi[i].ti.userdata == (void *) (intpt) val);
    if ( gme->col_data[c].enable_enum!=NULL )
	(gme->col_data[c].enable_enum)(&gme->g,mi,r,c);
    _GMenuCreatePopupMenu(gme->nested,event, mi, GME_FinishChoice);
}

static void GME_EnumStringDispatch(GWindow gw, GMenuItem *mi, GEvent *e) {
    GMatrixEdit *gme = GDrawGetUserData(gw);
    int r = gme->active_row, c = gme->active_col;

    if ( (intpt) mi->ti.userdata == GME_NoChange )
return;

    free(gme->data[r*gme->cols+c].u.md_str);
    if ( gme->col_data[c].me_type==me_stringchoicetrans )
	gme->data[r*gme->cols+c].u.md_str = copy( (char *) mi->ti.userdata );
    else if ( gme->col_data[c].me_type==me_stringchoicetag ) {
	char buf[8];
	buf[0] = ((intpt) mi->ti.userdata)>>24;
	buf[1] = ((intpt) mi->ti.userdata)>>16;
	buf[2] = ((intpt) mi->ti.userdata)>>8;
	buf[3] = ((intpt) mi->ti.userdata)&0xff;
	buf[4] = '\0';
	gme->data[r*gme->cols+c].u.md_str = copy( buf );
    } else
	gme->data[r*gme->cols+c].u.md_str = u2utf8_copy( mi->ti.text );

    if ( gme->finishedit != NULL )
	(gme->finishedit)(&gme->g,r,c,gme->wasnew);
    GME_AdjustCol(gme,c);
    gme->wasnew = false;
}

static void GME_StringChoices(GMatrixEdit *gme,GEvent *event,int r,int c) {
    GMenuItem *mi = gme->col_data[c].enum_vals;
    char *val = gme->data[r*gme->cols+c].u.md_str;
    int i;

    if ( gme->col_data[c].me_type==me_stringchoicetag ) {
	char buf[8];
	for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.line || mi[i].ti.image!=NULL; ++i ) {
	    buf[0] = ((intpt) mi[i].ti.userdata)>>24;
	    buf[1] = ((intpt) mi[i].ti.userdata)>>16;
	    buf[2] = ((intpt) mi[i].ti.userdata)>>8;
	    buf[3] = ((intpt) mi[i].ti.userdata)&0xff;
	    buf[4] = '\0';
	    mi[i].ti.selected = mi[i].ti.checked = !gme->wasnew && val!=NULL &&
		    strcmp(buf,val)==0;
	}
    } else if ( gme->col_data[c].me_type==me_stringchoicetrans ) {
	for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.line || mi[i].ti.image!=NULL; ++i )
	    mi[i].ti.selected = mi[i].ti.checked = !gme->wasnew && val!=NULL &&
		    strcmp((char *) mi[i].ti.userdata,val)==0;
    }
    if ( gme->col_data[c].enable_enum!=NULL )
	(gme->col_data[c].enable_enum)(&gme->g,mi,r,c);
    _GMenuCreatePopupMenu(gme->nested,event, mi, GME_FinishChoice);
}

static void GMatrixEdit_StartSubGadgets(GMatrixEdit *gme,int r, int c,GEvent *event) {
    int i, markpos, lastc;
    struct matrix_data *d;
    int old_off_left = gme->off_left;	/* We sometimes scroll */
    int oldr;
    GRect size;

    GDrawGetSize(gme->nested,&size);

    for ( lastc = gme->cols-1; lastc>0 && gme->col_data[lastc].hidden; --lastc );
    /* new row */
    if ( c==0 && r==gme->rows && event->type == et_mousedown &&
	    event->u.mouse.button==1 && !gme->no_edit ) {
	if ( gme->rows>=gme->row_max )
	    gme->data = realloc(gme->data,(gme->row_max+=10)*gme->cols*sizeof(struct matrix_data));
	++gme->rows;
	for ( i=0; i<gme->cols; ++i ) {
	    d = &gme->data[r*gme->cols+i];
	    memset(d,0,sizeof(*d));
	    switch ( gme->col_data[i].me_type ) {
	      case me_string: case me_bigstr:
	      case me_stringchoice: case me_stringchoicetrans: case me_stringchoicetag:
	      case me_funcedit:
	      case me_onlyfuncedit:
		d->u.md_str = copy("");
	      break;
	      case me_enum:
		d->u.md_ival = (int) (intptr_t) gme->col_data[i].enum_vals[0].ti.userdata;
	      break;
	    }
	}
	if ( gme->initrow!=NULL )
	    (gme->initrow)(&gme->g,r);
	GME_FixScrollBars(gme);
	GDrawRequestExpose(gme->nested,NULL,false);
	gme->wasnew = true;
    }

    if ( c==gme->cols || r>=gme->rows || gme->col_data[c].disabled )
return;
    oldr = gme->active_row;
    gme->active_col = c; gme->active_row = r;
    if ( r!=oldr && oldr!=-1 ) {
	GRect r;
	r.x = gme->col_data[c].x - gme->off_left; r.width = gme->col_data[c].width;
	r.y = (oldr-gme->off_top)*(gme->fh+gme->vpad); r.height = gme->fh+gme->vpad;
	if ( r.y+r.height>0 )
	    GDrawRequestExpose(gme->nested,&r,false);
    }
    GME_EnableDelete(gme);

    GMatrixEditScrollToRowCol(&gme->g,r,c);
    d = &gme->data[r*gme->cols+c];

    markpos = ( c == lastc && gme->col_data[c].x + gme->col_data[c].width > size.width ) ?
	    size.width - gme->col_data[c].x : gme->col_data[c].width;
    markpos -= (gme->mark_size+gme->mark_skip);
    if ( markpos < 0 ) markpos = 0;
    if ( event->type==et_mousedown && event->u.mouse.button==3 ) {
	if ( gme->popupmenu!=NULL )
	    (gme->popupmenu)(&gme->g,event,r,c);
    } else if ( d->frozen ) {
	GDrawBeep(NULL);
    } else if ( gme->no_edit ) {
	/* Twiddle toes */;
    } else if ( gme->col_data[c].me_type==me_enum ) {
	GME_Choices(gme,event,r,c);
    } else if ( gme->col_data[c].me_type==me_button ) {
	char *ret = (gme->col_data[c].func)(&gme->g,r,c);
	if ( ret!=NULL ) {
	    /* I don't bother validating it because I expect the function to */
	    /*  do that for me */
	    free(gme->data[r*gme->cols+c].u.md_str);
	    gme->data[r*gme->cols+c].u.md_str = ret;
	    GDrawRequestExpose(gme->nested,NULL,false);
	}
    } else if ( ((gme->col_data[c].me_type==me_funcedit ||
		    gme->col_data[c].me_type==me_onlyfuncedit ) &&
	    event->type==et_mousedown &&
	    event->u.mouse.x>gme->col_data[c].x + markpos - old_off_left ) ||
	(gme->col_data[c].me_type==me_onlyfuncedit &&
			event->type==et_mousedown &&
			(gme->wasnew || event->u.mouse.clicks==2)) ) {
	char *ret = (gme->col_data[c].func)(&gme->g,r,c);
	if ( ret!=NULL ) {
	    /* I don't bother validating it because I expect the function to */
	    /*  do that for me */
	    free(gme->data[r*gme->cols+c].u.md_str);
	    gme->data[r*gme->cols+c].u.md_str = ret;
	    if ( gme->finishedit != NULL )
                (gme->finishedit)(&gme->g,r,c,gme->wasnew);            
	    GDrawRequestExpose(gme->nested,NULL,false);
            gme->wasnew = false; // This is an attempted hack by somebody (Frank) who admittedly has no idea what is happening in all of this sparsely commented code.
	}
    } else if ( gme->col_data[c].me_type==me_onlyfuncedit ) {
	/* Don't allow other editing */
    } else if ( (gme->col_data[c].me_type==me_stringchoice ||
	    gme->col_data[c].me_type==me_stringchoicetrans ||
	    gme->col_data[c].me_type==me_stringchoicetag) &&
	    event->type==et_mousedown &&
	    event->u.mouse.x>gme->col_data[c].x + markpos - old_off_left ) {
	GME_StringChoices(gme,event,r,c);
    } else {
	char *str = MD_Text(gme,gme->active_row,gme->active_col);
	if ( str==NULL )
	    str = copy("");
	if ( str!=NULL &&
		(g_utf8_strlen(str, -1)>40 || strchr(str,'\n')!=NULL || gme->col_data[c].me_type == me_bigstr))
	    GME_StrBigEdit(gme,str);
	else
	    GME_StrSmallEdit(gme,str,event);
	free(str);
    }
}

static void GMatrixEdit_MouseEvent(GMatrixEdit *gme,GEvent *event) {
    int r = event->u.mouse.y/(gme->fh+gme->vpad) + gme->off_top;
    int x = event->u.mouse.x + gme->off_left;
    int c, i;

    if ( gme->edit_active && event->type==et_mousemove )
return;
    for ( c=0; c<gme->cols; ++c ) {
	if ( gme->col_data[c].hidden )
    continue;
	if ( x>=gme->col_data[c].x && x<=gme->col_data[c].x+gme->col_data[c].width )
    break;
    }
    if ( event->type==et_mousemove && gme->reportmousemove!=NULL ) {
	(gme->reportmousemove)(&gme->g,r,c);
return;
    }
    if ( gme->edit_active ) {
	if ( (r = GME_FinishEditPreserve(gme,r))== -1 )
return;
    }
    if ( event->type==et_mousedown )
	GMatrixEdit_StartSubGadgets(gme,r,c,event);
    else if ( event->type==et_mousemove ) {
	if ( c<gme->cols && gme->col_data[c].me_type==me_stringchoicetrans &&
		gme->col_data[c].enum_vals!=NULL &&
		r<gme->rows ) {
	    char *str = gme->data[r*gme->cols+c].u.md_str;
	    GMenuItem *enums = gme->col_data[c].enum_vals;
	    for ( i=0; enums[i].ti.text!=NULL || enums[i].ti.line ; ++i ) {
		if ( enums[i].ti.userdata!=NULL && strcmp(enums[i].ti.userdata,str)==0 ) {
		    if ( enums[i].ti.text_is_1byte )
			GGadgetPreparePopup8(gme->nested,(char *) enums[i].ti.text);
		    else
			GGadgetPreparePopup(gme->nested,enums[i].ti.text);
	    break;
		}
	    }
	} else if ( c<gme->cols && gme->col_data[c].me_type==me_stringchoicetag &&
		gme->col_data[c].enum_vals!=NULL &&
		r<gme->rows ) {
	    char *str = gme->data[r*gme->cols+c].u.md_str, buf[8];
	    GMenuItem *enums = gme->col_data[c].enum_vals;
	    for ( i=0; enums[i].ti.text!=NULL || enums[i].ti.line ; ++i ) {
		buf[0] = ((intpt) enums[i].ti.userdata)>>24;
		buf[1] = ((intpt) enums[i].ti.userdata)>>16;
		buf[2] = ((intpt) enums[i].ti.userdata)>>8;
		buf[3] = ((intpt) enums[i].ti.userdata)&0xff;
		buf[4] = '\0';
		if ( enums[i].ti.userdata!=NULL && strcmp(buf,str)==0 ) {
		    if ( enums[i].ti.text_is_1byte )
			GGadgetPreparePopup8(gme->nested,(char *) enums[i].ti.text);
		    else
			GGadgetPreparePopup(gme->nested,enums[i].ti.text);
	    break;
		}
	    }
	} else if ( gme->g.popup_msg!=NULL )
	    GGadgetPreparePopup(gme->nested,gme->g.popup_msg);
    }
}

static void GMatrixEdit_SubExpose(GMatrixEdit *gme,GWindow pixmap,GEvent *event) {
    int r,c, lastc, kludge;
    char *buf, *str, *pt;
    GRect size;
    GRect clip, old;
    Color fg, mkbg;
    struct matrix_data *data;
    GMenuItem *mi;

    GDrawGetSize(gme->nested,&size);
    if ( gme->g.state!=gs_enabled )
	GDrawFillRect(pixmap,&size,gme->g.box->disabled_background);

    GDrawDrawLine(pixmap,0,0,0,size.height,gmatrixedit_rules);
    /* Make sure the last visible column ends at (or after) the edge */
    for ( lastc = gme->cols-1; lastc>0 && gme->col_data[lastc].hidden; --lastc );
    if ( lastc>=0 && gme->col_data[lastc].x+gme->col_data[lastc].width < size.width ) {
	gme->col_data[lastc].width = size.width - gme->col_data[lastc].x;
	for ( c=lastc+1; c<gme->cols; ++c )
	    gme->col_data[c].x = size.width;
    }
    for ( c=0; c<gme->cols-1; ++c ) if ( !gme->col_data[c].hidden )
	GDrawDrawLine(pixmap,
		gme->col_data[c].x+gme->col_data[c].width+gme->hpad/2-gme->off_left,0,
		gme->col_data[c].x+gme->col_data[c].width+gme->hpad/2-gme->off_left,size.height,
		gmatrixedit_rules);
    GDrawDrawLine(pixmap,0,0,size.width,0,gmatrixedit_rules);
    for ( r=gme->off_top; r<=gme->rows && (r-gme->off_top)*(gme->fh+gme->vpad)<=gme->g.inner.height; ++r )
	GDrawDrawLine(pixmap,
		0,         (r-gme->off_top)*(gme->fh+gme->vpad)-1,
		size.width,(r-gme->off_top)*(gme->fh+gme->vpad)-1,
		gmatrixedit_rules);


    GDrawSetFont(pixmap,gme->font);
    for ( r=event->u.expose.rect.y/(gme->fh+gme->vpad);
	    r<=(event->u.expose.rect.y+event->u.expose.rect.height+gme->fh+gme->vpad-1)/gme->fh &&
	     r+gme->off_top<=gme->rows;
	    ++r ) {
	int y, lastc;
	clip.y = r*(gme->fh+gme->vpad);
	y = clip.y + gme->font_as;	/* I know this looks odd, but it seems to work when we grab a glyph from another font with cairo */
	clip.height = gme->fh;
	for ( lastc = gme->cols-1; lastc>0 && gme->col_data[lastc].hidden; --lastc );
	/* Compensate for the top border line */
	if ( clip.y <= 0 ) {
	    clip.y = 1; clip.height += ( clip.y - 1 );
	}

	for ( c=0; c<gme->cols; ++c ) {
	    if ( gme->col_data[c].hidden )
	continue;
	    if ( gme->col_data[c].x + gme->col_data[c].width < gme->off_left && gme->col_data[c].width > gme->hpad)
	continue;
	    clip.x = gme->col_data[c].x - gme->off_left;
	    clip.width = gme->col_data[c].width;
	    if ( gme->col_data[c].me_type==me_button ) {
		int temp;
		clip.height += 2;
		GBoxDrawBackground(pixmap,&clip,&gmatrixedit_button_box,
			gme->col_data[c].disabled ? gs_disabled : gs_enabled, false);
		GBoxDrawBorder(pixmap,&clip,&gmatrixedit_button_box,
			gme->col_data[c].disabled ? gs_disabled : gs_enabled, false);
		clip.height -= 2;
		temp = GBoxBorderWidth(pixmap,&gmatrixedit_button_box)+2;
		clip.x += temp;
		clip.width -= temp;
	    } else if ( gme->col_data[c].disabled && gme->g.box->disabled_background!=COLOR_TRANSPARENT )
		GDrawFillRect(pixmap,&clip,gme->g.box->disabled_background);
	    else if ( gme->active_row==r+gme->off_top )
		GDrawFillRect(pixmap,&clip,gmatrixedit_activebg);
	    if ( gme->col_data[c].me_type == me_stringchoice ||
		    gme->col_data[c].me_type == me_stringchoicetrans ||
		    gme->col_data[c].me_type == me_stringchoicetag ||
		    gme->col_data[c].me_type == me_onlyfuncedit ||
		    gme->col_data[c].me_type == me_funcedit ) {

		if ( c == lastc ) {
		    if ( clip.x < size.width && clip.x + clip.width > size.width )
			clip.width = size.width - clip.x;
		    else if ( clip.x >= size.width )
			clip.width = 0;
		}
		if ( clip.width >= (gme->mark_size+gme->mark_skip) )
		    clip.width -= (gme->mark_size+gme->mark_skip);
		else
		    clip.width = 0;
	    }
	    if ( clip.width>0 ) {
		GDrawPushClip(pixmap,&clip,&old);
		str = NULL;
		if ( r+gme->off_top==gme->rows ) {
		    if ( !gme->no_edit ) {
			if ( gme->newtext!=NULL )
			    buf = xasprintf( "<%s>", gme->newtext );
			else if ( _ggadget_use_gettext )
			    buf = xasprintf( "<%s>", S_("Row|New") );
			else {
			    gchar *tmp = g_ucs4_to_utf8( (const gunichar *) GStringGetResource( _STR_New, NULL ),
				   -1, NULL, NULL, NULL );
			    buf = xasprintf( "<%s>", tmp );
			    g_free( tmp ); tmp = NULL;
			}
			GDrawDrawText8( pixmap, gme->col_data[0].x - gme->off_left,y,
				(char *) buf, -1, gmatrixedit_activecol );
			free( buf ) ; buf = NULL ;
		    }
		} else {
		    data = &gme->data[(r+gme->off_top)*gme->cols+c];
		    fg = gme->g.state==gs_disabled?gme->g.box->disabled_foreground:
			    data->frozen ? gmatrixedit_frozencol:
			    gme->g.box->main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap)):
			    gme->g.box->main_foreground;
		    switch ( gme->col_data[c].me_type ) {
		      case me_enum:
			mi = FindMi(gme->col_data[c].enum_vals,data->u.md_ival);
			if ( mi!=NULL ) {
			    if ( mi->ti.text_is_1byte )
				GDrawDrawText8(pixmap,clip.x,y,(char *)mi->ti.text,-1,fg);
			    else
				GDrawDrawText(pixmap,clip.x,y,mi->ti.text,-1,fg);
		    break;
			}
			/* Fall through into next case */
		      default:
			kludge = gme->edit_active; gme->edit_active = false;
			str = MD_Text(gme,r+gme->off_top,c);
			if ( str==NULL && gme->col_data[c].me_type==me_button )
			    str = copy("...");
			gme->edit_active = kludge;
		      break;
		    }
		    if ( str!=NULL ) {
			pt = strchr(str,'\n');
			GDrawDrawText8(pixmap,clip.x,y,str,pt==NULL?-1:pt-str,fg);
			free(str);
		    }
		}
		GDrawPopClip(pixmap,&old);
	    }
	    if ( gme->col_data[c].me_type == me_stringchoice ||
		    gme->col_data[c].me_type == me_stringchoicetrans ||
		    gme->col_data[c].me_type == me_stringchoicetag ||
		    gme->col_data[c].me_type == me_onlyfuncedit ||
		    gme->col_data[c].me_type == me_funcedit ) {
		GRect mr;
		mr.x = clip.x + clip.width; mr.width = gme->mark_size+gme->mark_skip;
		mr.y = clip.y; mr.height = clip.height;

		mkbg = gme->active_row==r+gme->off_top ?
			gmatrixedit_activebg : GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(pixmap));
		GDrawFillRect(pixmap,&mr,mkbg);
		GListMarkDraw(pixmap,
			mr.x + gme->mark_skip + (gme->mark_size - gme->mark_length)/2,
			clip.y,
			clip.height,
			gme->g.state);
	    }
	    if ( r+gme->off_top==gme->rows )
return;
	}
    }
}

static int matrixeditsub_e_h(GWindow gw, GEvent *event) {
    GMatrixEdit *gme = (GMatrixEdit *) GDrawGetUserData(gw);
    int r,c;

    GGadgetPopupExternalEvent(event);
    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7)) {
	    int isv = event->u.mouse.button<=5;
	if ( event->u.mouse.state&ksm_shift ) isv = !isv;
	if ( isv && gme->vsb!=NULL )
return( GGadgetDispatchEvent(gme->vsb,event));
	else if ( !isv && gme->hsb!=NULL )
return( GGadgetDispatchEvent(gme->hsb,event));
	else
return( true );
    }

    switch ( event->type ) {
      case et_expose:
	GDrawSetLineWidth(gw,0);
	GMatrixEdit_SubExpose(gme,gw,event);
      break;
      case et_mousedown:
      case et_mouseup:
	if ( gme->g.state == gs_disabled )
return( false );
      case et_mousemove:
	GMatrixEdit_MouseEvent(gme,event);
      break;
      case et_char:
	if ( gme->g.state == gs_disabled )
return( false );
	r = gme->active_row;
	switch ( event->u.chr.keysym ) {
	  case GK_Up: case GK_KP_Up:
	    if ( (!gme->edit_active || GME_FinishEdit(gme)) &&
		    gme->active_row>0 )
		GMatrixEdit_StartSubGadgets(gme,gme->active_row-1,gme->active_col,event);
return( true );
	  break;
	  case GK_Down: case GK_KP_Down:
	    if ( (!gme->edit_active || GME_FinishEdit(gme)) &&
		    gme->active_row<gme->rows-(gme->active_col!=0) )
		GMatrixEdit_StartSubGadgets(gme,gme->active_row+1,gme->active_col,event);
return( true );
	  break;
	  case GK_Left: case GK_KP_Left:
	  case GK_BackTab:
	  backtab:
	    if ( (!gme->edit_active || (r=GME_FinishEditPreserve(gme,r))!=-1) &&
		    gme->active_col>0 ) {
		for ( c = gme->active_col-1; c>=0 && gme->col_data[c].hidden; --c );
		if ( c>=0 )
		    GMatrixEdit_StartSubGadgets(gme,r,c,event);
	    }
return( true );
	  break;
	  case GK_Tab:
	    if ( event->u.chr.state&ksm_shift )
	  goto backtab;
	    /* Else fall through */
	  case GK_Right: case GK_KP_Right:
	    if ( (!gme->edit_active || (r=GME_FinishEditPreserve(gme,r))!=-1) &&
		    gme->active_col<gme->cols-1 ) {
		for ( c = gme->active_col+1; c<gme->cols && gme->col_data[c].hidden; ++c );
		if ( c<gme->cols )
		    GMatrixEdit_StartSubGadgets(gme,r,c,event);
	    }
return( true );
	  break;
	  case GK_Return: case GK_KP_Enter:
	    if ( gme->edit_active && (r=GME_FinishEditPreserve(gme,r))!=-1 ) {
		GEvent dummy;
		memset(&dummy,0,sizeof(dummy));
		dummy.w = event->w;
		dummy.type = et_mousedown;
		dummy.u.mouse.state = event->u.chr.state;
		dummy.u.mouse.x = gme->off_left+gme->col_data[0].x+1;
		dummy.u.mouse.y = gme->off_top + (r+1)*(gme->fh+gme->vpad);
		dummy.u.mouse.button = 1;
		GMatrixEdit_StartSubGadgets(gme,r+1,0,&dummy);
	    }
return( true );
	}
	if ( gme->handle_key!=NULL )
return( (gme->handle_key)(&gme->g,event) );
return( false );
      break;
      case et_destroy:
	if ( gme!=NULL )
	    gme->nested = NULL;
      break;
      case et_controlevent:
	if ( gme->reporttextchanged!=NULL )
	    (gme->reporttextchanged)(&gme->g,gme->active_row,gme->active_col,gme->tf);
      break;
    }
return( true );
}

static void GME_HScroll(GMatrixEdit *gme,struct sbevent *sb) {
    int newpos = gme->off_left;
    GRect size;
    int hend = gme->col_data[gme->cols-1].x + gme->col_data[gme->cols-1].width;

    GDrawGetSize(gme->nested,&size);
    switch( sb->type ) {
      case et_sb_top:
	newpos = 0;
      break;
      case et_sb_uppage:
	newpos -= 9*size.width/10;
      break;
      case et_sb_up:
	newpos -= size.width/15;
      break;
      case et_sb_down:
	newpos += size.width/15;
      break;
      case et_sb_downpage:
	newpos += 9*size.width/10;
      break;
      case et_sb_bottom:
	newpos = hend;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
	newpos = sb->pos;
      break;
    }

    if ( newpos + size.width > hend )
	newpos = hend - size.width;
    if ( newpos<0 )
	newpos = 0;
    if ( newpos!=gme->off_left ) {
	int lastc, diff = gme->off_left-newpos;
	GRect clip;
	gme->off_left = newpos;
	GScrollBarSetPos(gme->hsb,newpos);

	clip.y = 1;
	clip.height = size.height - 1;
	for ( lastc = gme->cols-1; lastc>0 && gme->col_data[lastc].hidden; --lastc );

	gme->off_left = newpos;
	GScrollBarSetPos(gme->hsb,newpos);
	clip.x = 1; clip.y = 1; clip.width = size.width-1; clip.height = size.height-1;

	if (( gme->col_data[lastc].me_type == me_stringchoice ||
		gme->col_data[lastc].me_type == me_stringchoicetrans ||
		gme->col_data[lastc].me_type == me_stringchoicetag ||
		gme->col_data[lastc].me_type == me_onlyfuncedit ||
		gme->col_data[lastc].me_type == me_funcedit ) &&
		gme->col_data[lastc].x <= gme->off_left + size.width - (gme->mark_size + gme->mark_skip) ) {
	    int xdiff = gme->off_left + size.width - (gme->mark_size + gme->mark_skip) - gme->col_data[lastc].x;
	    /* Catch the moment when we should stop scrolling the list mark area */
	    if ( xdiff + diff < 0 ) {
		GDrawScroll( gme->nested,&clip,xdiff + diff,0 );
		diff = -xdiff;
	    }
	    clip.width -= (gme->mark_size + gme->mark_skip);
	}
	GDrawScroll( gme->nested,&clip,diff,0 );
	GME_PositionEdit(gme);
	GME_RedrawTitles(gme);
    }
}

static void GME_VScroll(GMatrixEdit *gme,struct sbevent *sb) {
    int newpos = gme->off_top;
    int page;
    GRect size;

    GDrawGetSize(gme->nested,&size);
    page = size.height/(gme->fh+gme->vpad);

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
	newpos = gme->rows+1;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
	newpos = sb->pos;
      break;
    }
    if ( newpos + page > gme->rows+1 )
	newpos = gme->rows+1 - page;
    if ( newpos<0 )
	newpos = 0;
    if ( newpos!=gme->off_top ) {
	int diff = (newpos-gme->off_top)*(gme->fh+gme->vpad);
	GRect r;
	gme->off_top = newpos;
	GScrollBarSetPos(gme->vsb,newpos);
	r.x = 1; r.y = 1; r.width = size.width-1; r.height = size.height-1;
	GDrawScroll(gme->nested,&r,0,diff);
	GME_PositionEdit(gme);
	GDrawRequestExpose(gme->nested,&size,false);
    }
}

static int _GME_HScroll(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_scrollbarchange ) {
	GMatrixEdit *gme = (GMatrixEdit *) g->data;
	GME_HScroll(gme,&e->u.control.u.sb);
    }
return( true );
}

static int _GME_VScroll(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_scrollbarchange ) {
	GMatrixEdit *gme = (GMatrixEdit *) g->data;
	GME_VScroll(gme,&e->u.control.u.sb);
    }
return( true );
}

static int _GME_DeleteActive(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GMatrixEdit *gme = (GMatrixEdit *) g->data;
	GME_DeleteActive(gme);
    }
return( true );
}

static GMenuItem *GMenuItemFromTI(GTextInfo *ti,int is_enum) {
    int cnt;
    GMenuItem *mi;

    for ( cnt=0; ti[cnt].text!=NULL || ti[cnt].line; ++cnt );
    mi = calloc((cnt+1),sizeof(GMenuItem));
    for ( cnt=0; ti[cnt].text!=NULL || ti[cnt].line; ++cnt ) {
	mi[cnt].ti = ti[cnt];
	if ( ti[cnt].bg == ti[cnt].fg )
	    mi[cnt].ti.bg = mi[cnt].ti.fg = COLOR_DEFAULT;
	if ( mi[cnt].ti.text!=NULL ) {
	    if ( ti[cnt].text_is_1byte )
		mi[cnt].ti.text = (unichar_t *) copy( (char *) mi[cnt].ti.text );
	    else
		mi[cnt].ti.text = u_copy( mi[cnt].ti.text );
	    mi[cnt].ti.checkable = true;
	    mi[cnt].invoke = is_enum ? GME_EnumDispatch : GME_EnumStringDispatch;
	}
    }
return( mi );
}

/* GMatrixElement: External interface *************************************** */
GGadget *GMatrixEditCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    struct matrixinit *matrix = gd->u.matrix;
    GMatrixEdit *gme = calloc(1,sizeof(GMatrixEdit));
    int r, c, bp;
    int x;
    GRect outer;
    GRect pos;
    GWindowAttrs wattrs;
    int sbwidth = GDrawPointsToPixels(base,_GScrollBar_Width);
    GGadgetData sub_gd;
    GTextInfo label;
    int as, ds, ld;

    if ( !gmatrixedit_inited )
	_GMatrixEdit_Init();

    gme->g.funcs = &gmatrixedit_funcs;
    _GGadget_Create(&gme->g,base,gd,data,&gmatrixedit_box);
    gme->g.takes_input = true; gme->g.takes_keyboard = false; gme->g.focusable = false;

    gme->font = gmatrixedit_font;
    gme->titfont = gmatrixedit_titfont;
    GDrawWindowFontMetrics(base,gme->font,&as, &ds, &ld);
    gme->font_as = gme->as = as;
    gme->font_fh = gme->fh = as+ds;

    gme->rows = matrix->initial_row_cnt; gme->cols = matrix->col_cnt;
    gme->row_max = gme->rows;
    gme->hpad = gme->vpad = GDrawPointsToPixels(base,2);

    gme->col_data = calloc(gme->cols,sizeof(struct col_data));
    for ( c=0; c<gme->cols; ++c ) {
	gme->col_data[c].me_type = matrix->col_init[c].me_type;
	gme->col_data[c].func = matrix->col_init[c].func;
	if ( matrix->col_init[c].enum_vals!=NULL )
	    gme->col_data[c].enum_vals = GMenuItemFromTI(matrix->col_init[c].enum_vals,
		    matrix->col_init[c].me_type==me_enum );
	else
	    gme->col_data[c].enum_vals = NULL;
	gme->col_data[c].enable_enum = matrix->col_init[c].enable_enum;
	gme->col_data[c].title = copy( matrix->col_init[c].title );
	if ( gme->col_data[c].title!=NULL ) gme->has_titles = true;
	gme->col_data[c].fixed = false;
    }

    gme->data = calloc(gme->rows*gme->cols,sizeof(struct matrix_data));
    memcpy(gme->data,matrix->matrix_data,gme->rows*gme->cols*sizeof(struct matrix_data));
    for ( c=0; c<gme->cols; ++c ) {
	enum me_type me_type = gme->col_data[c].me_type;
	if ( me_type==me_string || me_type==me_bigstr || me_type==me_func ||
		me_type==me_button || me_type==me_onlyfuncedit ||
		me_type==me_funcedit || me_type==me_stringchoice ||
		me_type==me_stringchoicetrans || me_type==me_stringchoicetag ) {
	    for ( r=0; r<gme->rows; ++r )
		gme->data[r*gme->cols+c].u.md_str = copy(gme->data[r*gme->cols+c].u.md_str);
	}
    }

    gme->mark_length = GDrawPointsToPixels(base,_GListMarkSize);
    gme->mark_size = gme->mark_length +
	    2*GBoxBorderWidth(base,&_GListMark_Box);
    gme->mark_skip = GDrawPointsToPixels(base,_GGadget_TextImageSkip);

    /* Can't do this earlier. It depends on matrix_data being set */
    x = 1;
    for ( c=0; c<gme->cols; ++c ) {
	gme->col_data[c].x = x;
	gme->col_data[c].width = GME_ColWidth(gme,c);
	x += gme->col_data[c].width + gme->hpad;
    }

    gme->pressed_col = -1;
    gme->active_col = gme->active_row = -1;
    gme->initrow = matrix->initrow;
    gme->finishedit = matrix->finishedit;
    gme->candelete = matrix->candelete;
    gme->popupmenu = matrix->popupmenu;
    gme->handle_key = matrix->handle_key;
    gme->bigedittitle = matrix->bigedittitle;

    GMatrixEdit_GetDesiredSize(&gme->g,&outer,NULL);
    if ( gme->g.r.width==0 )
	gme->g.r.width = outer.width;
    else
	gme->g.desired_width = gme->g.r.width;
    if ( gme->g.r.height==0 )
	gme->g.r.height = outer.height;
    else
	gme->g.desired_height = gme->g.r.height;
    bp = GBoxBorderWidth(gme->g.base,gme->g.box);
    gme->g.inner.x = gme->g.r.x + bp;
    gme->g.inner.y = gme->g.r.y + bp;
    gme->g.inner.width = gme->g.r.width -2*bp;
    gme->g.inner.height = gme->g.r.height -2*bp;

    memset(&sub_gd,0,sizeof(sub_gd));
    memset(&label,0,sizeof(label));
    sub_gd.pos.x = sub_gd.pos.y = 1; sub_gd.pos.width = sub_gd.pos.height = 0;
    label.text = (unichar_t *) _("Delete");
    label.text_is_1byte = true;
    sub_gd.flags = gg_visible | gg_pos_in_pixels;
    sub_gd.label = &label;
    sub_gd.handle_controlevent = _GME_DeleteActive;
    gme->del = GButtonCreate(base,&sub_gd,gme);
    gme->del->contained = true;

    if ( gme->g.r.height<10 ) {
	int extra = 2*bp+ sbwidth + (gme->has_titles?gme->fh:0) +
		    gme->del->r.height+DEL_SPACE;
	gme->g.r.height = extra + gme->g.r.height*(gme->fh + gme->vpad);
	gme->g.inner.height = gme->g.r.height - 2*bp;
    }

    memset(&wattrs,0,sizeof(wattrs));
    if ( gme->g.box->main_background!=COLOR_TRANSPARENT )
	wattrs.mask = wam_events|wam_cursor|wam_backcol;
    else
	wattrs.mask = wam_events|wam_cursor;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.cursor = ct_pointer;
    wattrs.background_color = gme->g.box->main_background;
    pos = gme->g.inner;
    pos.width -= sbwidth;
    pos.height -= sbwidth + gme->del->inner.height+DEL_SPACE;
    if ( gme->has_titles ) {
	pos.y += gme->fh;
	pos.height -= gme->fh;
    }
    gme->nested = GWidgetCreateSubWindow(base,&pos,matrixeditsub_e_h,gme,&wattrs);

    GGadgetMove(gme->del,
	    (gme->g.inner.width-gme->del->r.width)/2,
	    gme->g.inner.height-gme->del->r.height-DEL_SPACE/2);

    sub_gd.pos = pos;
    sub_gd.pos.x = pos.x+pos.width; sub_gd.pos.width = sbwidth;
    sub_gd.flags = (gd->flags & (gg_visible | gg_enabled)) | gg_sb_vert | gg_pos_in_pixels;
    sub_gd.handle_controlevent = _GME_VScroll;
    gme->vsb = GScrollBarCreate(base,&sub_gd,gme);
    gme->vsb->contained = true;

    sub_gd.pos = pos;
    sub_gd.pos.y = pos.y+pos.height; sub_gd.pos.height = sbwidth;
    sub_gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    sub_gd.handle_controlevent = _GME_HScroll;
    gme->hsb = GScrollBarCreate(base,&sub_gd,gme);
    gme->hsb->contained = true;

    GME_RecalcFH(gme);
    {
	static GBox small = GBOX_EMPTY;
	static unichar_t nullstr[1] = { 0 };

	small.main_background = gmatrixedit_activebg;
	small.main_foreground = gmatrixedit_activecol;
	memset(&sub_gd,'\0',sizeof(sub_gd));
	memset(&label,'\0',sizeof(label));

	label.text = nullstr;
	label.font = gme->font;
	sub_gd.pos.height = gme->fh;
	sub_gd.pos.width = 40;
	sub_gd.label = &label;
	sub_gd.box = &small;
	sub_gd.flags = gg_enabled | gg_pos_in_pixels | gg_dontcopybox | gg_text_xim;
	gme->tf = GTextCompletionCreate(gme->nested,&sub_gd,gme);
	((GTextField *) (gme->tf))->accepts_tabs = false;
    }

    if ( gme->g.state!=gs_invisible )
	GDrawSetVisible(gme->nested,true);
return( &gme->g );
}

void GMatrixEditSet(GGadget *g,struct matrix_data *data, int rows, int copy_it) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
    int r,c;

    if ( data==gme->data ) {
	if ( rows<gme->rows )
	    gme->rows = rows;
	GME_RecalcFH(gme);
    } else {
	MatrixDataFree(gme);

	gme->rows = gme->row_max = rows;
	if ( !copy_it ) {
	    gme->data = data;
	} else {
	    gme->data = calloc(rows*gme->cols,sizeof(struct matrix_data));
	    memcpy(gme->data,data,rows*gme->cols*sizeof(struct matrix_data));
	    for ( c=0; c<gme->cols; ++c ) {
		enum me_type me_type = gme->col_data[c].me_type;
		if ( me_type==me_string || me_type==me_bigstr || me_type==me_func ||
			me_type==me_button || me_type==me_onlyfuncedit ||
			me_type==me_funcedit || me_type==me_stringchoice ||
			me_type==me_stringchoicetrans || me_type==me_stringchoicetag ) {
		    for ( r=0; r<rows; ++r )
			gme->data[r*gme->cols+c].u.md_str = copy(gme->data[r*gme->cols+c].u.md_str);
		}
	    }
	}
	GME_RecalcFH(gme);

	gme->active_row = gme->active_col = -1;
	GME_EnableDelete(gme);
	if ( !GME_AdjustCol(gme,-1)) {
	    GME_FixScrollBars(gme);
	    GDrawRequestExpose(gme->nested,NULL,false);
	}
    }
}

void GMatrixEditDeleteRow(GGadget *g,int row) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

    if ( row!=-1 )
	gme->active_row = row;
    GME_DeleteActive(gme);
}

int GMatrixEditStringDlg(GGadget *g,int row,int col) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
    char *str;

    if ( gme->edit_active ) {
	if ( !GME_FinishEdit(gme) )
return(false);
    }
    if ( row!=-1 )
	gme->active_row = row;
    if ( col!=-1 )
	gme->active_col = col;
    str = MD_Text(gme,row,col);
    GME_StrBigEdit(gme,str);
    free(str);
return( true );
}

struct matrix_data *GMatrixEditGet(GGadget *g, int *rows) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

    if ( gme->edit_active && !GME_FinishEdit(gme) ) {
	*rows = 0;
return( NULL );
    }

    *rows = gme->rows;
return( gme->data );
}

struct matrix_data *_GMatrixEditGet(GGadget *g, int *rows) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

    /* Does not try to parse the active textfield, if any */
    *rows = gme->rows;
return( gme->data );
}

GGadget *_GMatrixEditGetActiveTextField(GGadget *g) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
    if ( gme->edit_active )
return( gme->tf );

return( NULL );
}

int GMatrixEditGetActiveRow(GGadget *g) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

return( gme->active_row );
}

int GMatrixEditGetActiveCol(GGadget *g) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

return( gme->active_col );
}

void GMatrixEditSetNewText(GGadget *g, char *text) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

    free(gme->newtext);
    gme->newtext = copy(text);
}

void GMatrixEditSetOtherButtonEnable(GGadget *g, void (*sob)(GGadget *g, int r, int c)) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

    gme->setotherbuttons = sob;
}

void GMatrixEditSetMouseMoveReporter(GGadget *g, void (*rmm)(GGadget *g, int r, int c)) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

    gme->reportmousemove = rmm;
}

void GMatrixEditSetTextChangeReporter(GGadget *g, void (*tcr)(GGadget *g, int r, int c, GGadget *text)) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

    gme->reporttextchanged = tcr;
}

void GMatrixEditSetValidateStr(GGadget *g, char *(*validate)(GGadget *g, int r, int c, int wasnew, char *str)) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

    gme->validatestr = validate;
}

void GMatrixEditSetUpDownVisible(GGadget *g, int visible) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
    GGadgetCreateData gcd[3];
    GTextInfo label[2];
    int i;

    if ( gme->up==NULL ) {
	if ( !visible )
return;

	memset(gcd,0,sizeof(gcd));
	memset(label,0,sizeof(label));
	i = 0;

/* I want the 2 pronged arrow, but gdraw can't find a nice one */
/*	label[i].text = (unichar_t *) "⇑";	*//* Up Arrow */
	label[i].text = (unichar_t *) "↑";	/* Up Arrow */
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.flags = gg_visible /*| gg_enabled*/ ;
	gcd[i].gd.handle_controlevent = _GME_Up;
	gcd[i].data = gme;
	gcd[i++].creator = GButtonCreate;

/* I want the 2 pronged arrow, but gdraw can't find a nice one */
/*	label[i].text = (unichar_t *) "⇓";	*//* Down Arrow */
	label[i].text = (unichar_t *) "↓";	/* Down Arrow */
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.flags = gg_visible /*| gg_enabled*/ ;
	gcd[i].gd.handle_controlevent = _GME_Down;
	gcd[i].data = gme;
	gcd[i++].creator = GButtonCreate;
	GGadgetsCreate(g->base,gcd);

	gme->up = gcd[0].ret;
	gme->down = gcd[1].ret;
	gme->up->contained = gme->down->contained = true;
    } else {
	GGadgetSetVisible(gme->up,visible);
	GGadgetSetVisible(gme->down,visible);
    }
}

void GMatrixEditAddButtons(GGadget *g, GGadgetCreateData *gcd) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
    int i, base=0;

    if ( gme->buttonlist!=NULL ) {
	for ( base=0; gme->buttonlist[base]!=NULL; ++base );
    }
    for ( i=0; gcd[i].creator!=NULL; ++i );
    gme->buttonlist = realloc(gme->buttonlist,(i+base+1)*sizeof(GGadget *));
    GGadgetsCreate(g->base,gcd);
    for ( i=0; gcd[i].creator!=NULL; ++i ) {
	gme->buttonlist[base+i] = gcd[i].ret;
	gcd[i].ret->contained = true;
    }
    gme->buttonlist[base+i] = NULL;
}

void GMatrixEditEnableColumn(GGadget *g, int col, int enabled) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
    /* User must do a refresh of the gadget. Don't want to do it always */
    /* because multiple calls might cause a flicker */

    if ( col<0 || col>=gme->cols )
return;
    gme->col_data[col].disabled = !enabled;
}

void GMatrixEditShowColumn(GGadget *g, int col, int visible) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

    if ( col<0 || col>=gme->cols )
return;
    gme->col_data[col].hidden = !visible;
    gme->col_data[col].fixed = false;
    GME_AdjustCol(gme,-1);
}

void GMatrixEditSetColumnChoices(GGadget *g, int col, GTextInfo *ti) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

    if ( gme->col_data[col].enum_vals!=NULL )
	GMenuItemArrayFree(gme->col_data[col].enum_vals);
    if ( ti!=NULL )
	gme->col_data[col].enum_vals = GMenuItemFromTI(ti,
		    gme->col_data[col].me_type==me_enum );
    else
	gme->col_data[col].enum_vals = NULL;
}

GMenuItem *GMatrixEditGetColumnChoices(GGadget *g, int col) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

return( gme->col_data[col].enum_vals );
}

int GMatrixEditGetColCnt(GGadget *g) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
return( gme->cols );
}

void GMatrixEditScrollToRowCol(GGadget *g,int r, int c) {
    GMatrixEdit *gme = (GMatrixEdit *) g;
    int rows_shown = gme->vsb->r.height/(gme->fh+gme->vpad);
    int context = rows_shown/3;
    int needs_expose = true;
    int width = gme->hsb->r.width;
    int i;
    GRect size;

    if ( r<0 ) r = 0; else if ( r>=gme->rows ) r = gme->rows-1;
    if ( r<gme->off_top || r>=gme->off_top+rows_shown ) {
	gme->off_top = r-context;
	if ( gme->off_top<0 )
	    gme->off_top = 0;
	needs_expose = true;
    }
    if ( c<0 ) c = 0; else if ( c>=gme->cols ) c = gme->cols-1;
    for ( i=0; i<gme->cols; ++i ) {
	if ( gme->col_data[i].x-gme->off_left>=0 )
    break;
    }
    if ( c<i ) {
	if ( c>0 && gme->col_data[c-1].width + gme->col_data[c].width<width )
	    gme->off_left = gme->col_data[c-1].x;
	else
	    gme->off_left = gme->col_data[c  ].x;
	needs_expose = true;
    } else {
	for ( ; i<gme->cols; ++i ) {
	    if ( gme->col_data[i].x+gme->col_data[i].width-gme->off_left>width )
	break;
	}
	if ( c>=i && gme->col_data[c].x!=gme->off_left ) {
	    gme->off_left = gme->col_data[c].x;
	    needs_expose = true;
	}
    }
    if ( needs_expose ) {
	int hend = gme->col_data[gme->cols-1].x + gme->col_data[gme->cols-1].width;

	GDrawGetSize(gme->nested,&size);
	if ( gme->off_left>hend-size.width )
	    gme->off_left = hend-size.width;
	if ( gme->off_left<0 )
	    gme->off_left = 0;
	GScrollBarSetPos(gme->hsb,gme->off_left);
	GScrollBarSetPos(gme->vsb,gme->off_top);
	GGadgetRedraw(&gme->g);
    /* Used to request expose only if the row or column we are scrolling to */
    /* was outside of the visible area. However we need expose anyway, because */
    /* otherwise it is impossible to properly highlight fields in the active row. */
    /* So the rectangle associated with the expose event is now the only thing which */
    /* makes some difference. */
    } else {
	GGadgetGetSize(gme->tf,&size);
	GDrawRequestExpose(gme->nested,&size,false);
    }
}

void GMatrixEditSetColumnCompletion(GGadget *g, int col,
	GTextCompletionHandler completion) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

    gme->col_data[col].completer = completion;
}

void GMatrixEditSetBeforeDelete(GGadget *g, void (*predelete)(GGadget *g, int r)) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

    gme->predelete = predelete;
}

void GMatrixEditSetRowMotionCallback(GGadget *g, void (*rowmotion)(GGadget *g, int oldr, int newr)) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

    gme->rowmotion = rowmotion;
}

void GMatrixEditSetCanUpDown(GGadget *g, enum gme_updown (*canupdown)(GGadget *g, int r)) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

    gme->canupdown = canupdown;
}

void GMatrixEditActivateRowCol(GGadget *g, int r, int c) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

    gme->active_row = r;
    gme->active_col = c;
    GME_EnableDelete(gme);
    GDrawRequestExpose(gme->nested,NULL,false);
}

void GMatrixEditSetEditable(GGadget *g, int editable ) {
    GMatrixEdit *gme = (GMatrixEdit *) g;

    gme->no_edit = !editable;
    GGadgetSetVisible(gme->del,editable);
    GMatrixEdit_Resize(&gme->g,gme->g.r.width,gme->g.r.height);
    GDrawRequestExpose(gme->nested,NULL,false);
}

GResInfo *_GMatrixEditRIHead(void) {
    /* GRect size; */

    _GMatrixEdit_Init();
    /* GDrawGetSize(GDrawGetRoot(NULL),&size);*/
    if ( true /* size.height<900*/ ) {
	gmatrixedit_ri.next = &gmatrixedit2_ri;
	gmatrixedit_ri.extras = NULL;
	gmatrixedit_ri.seealso1 = &gmatrixedit2_ri;
    }

return( &gmatrixedit_ri );
}
