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

#include <fontforge-config.h>

#include "gdraw.h"
#include "ggadgetP.h"

/* If the locale is hebrew or arabic should we lay out boxes right to left???*/

#define GG_Glue		((GGadget *) -1)	/* Special entries */
#define GG_ColSpan	((GGadget *) -2)	/* for box elements */
#define GG_RowSpan	((GGadget *) -3)	/* Must match those in ggadget.h */
#define GG_HPad10	((GGadget *) -4)

static GBox hvgroup_box = GBOX_EMPTY; /* Don't initialize here */
static GBox hvbox_box = GBOX_EMPTY; /* Don't initialize here */

extern GResInfo gflowbox_ri;
GResInfo ghvgroupbox_ri = {
    &gflowbox_ri, &ggadget_ri, NULL, NULL,
    &hvgroup_box,
    NULL,
    NULL,
    NULL,
    N_("HV Group Box"),
    N_("A box drawn around other gadgets"),
    "GGroup",
    "Gdraw",
    false,
    false,
    omf_border_type|omf_border_shape|omf_padding|omf_main_background|omf_disabled_background,
    { bt_engraved, bs_rect, 0, 2, 0, 0, 0, 0, 0, 0, COLOR_TRANSPARENT, 0, COLOR_TRANSPARENT, 0, 0, 0, 0, 0, 0 },
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
GResInfo ghvbox_ri = {
    &ghvgroupbox_ri, &ggadget_ri, NULL, NULL,
    &hvbox_box,
    NULL,
    NULL,
    NULL,
    N_("HV Box"),
    N_("A container for other gadgets"),
    "GHVBox",
    "Gdraw",
    false,
    false,
    omf_border_type|omf_border_shape|omf_padding,
    { bt_none, bs_rect, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};

static void _GHVBox_Init(void) {
    GResEditDoInit(&ghvgroupbox_ri);
    GResEditDoInit(&ghvbox_ri);
}

static void GHVBox_destroy(GGadget *g) {
    GHVBox *gb = (GHVBox *) g;
    int i;

    if ( gb->label != NULL )
	GGadgetDestroy( gb->label );
    for ( i=0; i<gb->rows*gb->cols; ++i )
	if ( gb->children[i]!=GG_Glue && gb->children[i]!=GG_ColSpan &&
		gb->children[i]!=GG_RowSpan && gb->children[i]!=GG_HPad10 )
	    GGadgetDestroy(gb->children[i]);
    free(gb->children);
    _ggadget_destroy(g);
}

static void GHVBoxMove(GGadget *g, int32_t x, int32_t y) {
    GHVBox *gb = (GHVBox *) g;
    int offx = x-g->r.x, offy = y-g->r.y;
    int i;

    if ( gb->label!=NULL )
	GGadgetMove(gb->label,gb->label->inner.x+offx,gb->label->inner.y+offy);
    for ( i=0; i<gb->rows*gb->cols; ++i )
	if ( gb->children[i]!=GG_Glue && gb->children[i]!=GG_ColSpan &&
		gb->children[i]!=GG_RowSpan && gb->children[i]!=GG_HPad10 )
	    GGadgetMove(gb->children[i],
		    gb->children[i]->r.x+offx, gb->children[i]->r.y+offy);
    _ggadget_move(g,x,y);
}

struct sizedata {
    int extra_space;		/* a default button has "extra_space" */
    int min;			/* This value includes padding */
    int sized;
    int allocated;
    int allglue;
};

struct sizeinfo {
    struct sizedata *cols;
    struct sizedata *rows;
    int label_height, label_width;
    int width, height;
    int minwidth, minheight;
};

static void GHVBoxGatherSizeInfo(GHVBox *gb,struct sizeinfo *si) {
    int i,c,r,spanc, spanr, totc,totr, max, plus, extra, es;
    GRect outer;
    int ten = GDrawPointsToPixels(gb->g.base,10);

    memset(si,0,sizeof(*si));
    si->cols = calloc(gb->cols,sizeof(struct sizedata));
    si->rows = calloc(gb->rows,sizeof(struct sizedata));
    for ( c=0; c<gb->cols; ++c ) si->cols[c].allglue = true;
    for ( r=0; r<gb->rows; ++r ) si->rows[r].allglue = true;

    for ( r=0; r<gb->rows; ++r ) {
	for ( c=0; c<gb->cols; ++c ) {
	    GGadget *g = gb->children[r*gb->cols+c];
	    if ( g==GG_Glue ) {
		if ( c+1!=gb->cols && si->cols[c].min<gb->hpad ) si->cols[c].min = gb->hpad;
		if ( r+1!=gb->rows && si->rows[r].min<gb->vpad ) si->rows[r].min = gb->vpad;
	    } else if ( g==GG_HPad10 ) {
		if ( c+1!=gb->cols && si->cols[c].min<gb->hpad+ten ) si->cols[c].min = gb->hpad+ten;
		if ( r+1!=gb->rows && si->rows[r].min<gb->vpad ) si->rows[r].min = gb->vpad+ten/2;
		si->cols[c].allglue = false;
	    } else if ( g==GG_ColSpan || g==GG_RowSpan )
		/* Skip it */;
	    else if ( (r+1<gb->rows && gb->children[(r+1)*gb->cols+c]==GG_RowSpan) ||
		      (c+1<gb->cols && gb->children[r*gb->cols+c+1]==GG_ColSpan))
		/* This gadget spans some columns or rows. Come back later */;
	    else {
		GGadgetGetDesiredSize(g,&outer,NULL);
		es = GBoxExtraSpace(g);
		if ( c+1!=gb->cols && g->state!=gs_invisible )
		    outer.width += gb->hpad;
		if ( r+1!=gb->rows && g->state!=gs_invisible )
		    outer.height += gb->vpad;
		si->cols[c].allglue = false;
		if ( si->cols[c].extra_space<es ) si->cols[c].extra_space=es;
		if ( si->cols[c].min<outer.width ) si->cols[c].min=outer.width;
		si->rows[r].allglue = false;
		if ( si->rows[r].min<outer.height ) si->rows[r].min=outer.height;
		if ( si->rows[r].extra_space<es ) si->rows[r].extra_space=es;
	    }
	}
    }

    for ( r=0; r<gb->rows; ++r ) {
	for ( c=0; c<gb->cols; ++c ) {
	    GGadget *g = gb->children[r*gb->cols+c];
	    if ( g==GG_Glue || g==GG_ColSpan || g==GG_RowSpan || g==GG_HPad10 )
		/* Skip it */;
	    else if ( (r+1<gb->rows && gb->children[(r+1)*gb->cols+c]==GG_RowSpan) ||
		      (c+1<gb->cols && gb->children[r*gb->cols+c+1]==GG_ColSpan)) {
		si->rows[r].allglue = false;
		totr = si->rows[r].min;
		for ( spanr=1; r+spanr<gb->rows &&
			gb->children[(r+spanr)*gb->cols+c]==GG_RowSpan; ++spanr ) {
		    si->rows[r+spanr].allglue = false;
		    totr += si->rows[r+spanr].min;
		}
		si->cols[c].allglue = false;
		totc = si->cols[c].min;
		for ( spanc=1; c+spanc<gb->cols &&
			gb->children[r*gb->cols+c+spanc]==GG_ColSpan; ++spanc ) {
		    si->cols[c+spanc].allglue = false;
		    totc += si->cols[c+spanc].min;
		}
		GGadgetGetDesiredSize(g,&outer,NULL);
		es = GBoxExtraSpace(g);
		if ( c+spanc!=gb->cols && g->state!=gs_invisible )
		    outer.width += gb->hpad;
		if ( r+spanr!=gb->rows && g->state!=gs_invisible )
		    outer.height += gb->vpad;
		if ( outer.width>totc ) {
		    plus = (outer.width-totc)/spanc;
		    extra = (outer.width-totc-spanc*plus);
		    for ( i=0; i<spanc; ++i ) {
			si->cols[c+i].min += plus + (extra>0);
			--extra;
		    }
		}
		if ( outer.height>totr ) {
		    plus = (outer.height-totr)/spanr;
		    extra = (outer.height-totr-spanr*plus);
		    for ( i=0; i<spanr; ++i ) {
			si->rows[r+i].min += plus + (extra>0);
			--extra;
		    }
		}
		if ( es!=0 ) {
		    for ( i=0; i<spanc; ++i ) {
			if ( es>si->cols[c+i].extra_space )
			    si->cols[c+i].extra_space = es;
		    }
		    for ( i=0; i<spanr; ++i ) {
			if ( es>si->rows[r+i].extra_space )
			    si->rows[r+i].extra_space = es;
		    }
		}
	    }
	}
    }

    if ( gb->label!=NULL ) {
	GGadgetGetDesiredSize(gb->label,&outer,NULL);
	totc = 0;
	for ( c=0; c<gb->cols ; ++c )
	    totc += si->cols[c].min;
	outer.width += 20;		/* Set back on each side */
	if ( outer.width>totc ) {
	    plus = (outer.width-totc)/gb->cols;
	    extra = (outer.width-totc-gb->cols*plus);
	    for ( i=0; i<gb->cols; ++i ) {
		si->cols[i].min += plus + (extra>0);
		--extra;
	    }
	}
	si->label_height = outer.height;
	si->label_width = outer.width;
    }

    for ( max=c=0; c<gb->cols; ++c )
	si->cols[c].sized = si->cols[c].min;
    for ( max=r=0; r<gb->rows; ++r )
	si->rows[r].sized = si->rows[r].min;

    if ( gb->grow_col==gb_samesize ) {
	for ( max=c=0; c<gb->cols; ++c )
	    if ( max<si->cols[c].sized ) max = si->cols[c].sized;
	for ( c=0; c<gb->cols; ++c )
	    if ( si->cols[c].min!=0 || si->cols[c].allglue )
		si->cols[c].sized = max;
    } else if ( gb->grow_col==gb_expandgluesame ) {
	for ( max=c=0; c<gb->cols; ++c )
	    if ( max<si->cols[c].sized && !si->cols[c].allglue ) max = si->cols[c].sized;
	for ( c=0; c<gb->cols; ++c )
	    if ( !si->cols[c].allglue && si->cols[c].min!=0 )	/* Must have at least one visible element */
		si->cols[c].sized = max;
    }

    if ( gb->grow_row==gb_samesize ) {
	for ( max=r=0; r<gb->rows; ++r )
	    if ( max<si->rows[r].sized ) max = si->rows[r].sized;
	for ( r=0; r<gb->rows; ++r )
	    if ( si->rows[r].min!=0 || si->rows[r].allglue )
		si->rows[r].sized = max;
    } else if ( gb->grow_row==gb_expandgluesame ) {
	for ( max=r=0; r<gb->rows; ++r )
	    if ( max<si->rows[r].sized && !si->rows[r].allglue ) max = si->rows[r].sized;
	for ( r=0; r<gb->rows; ++r )
	    if ( !si->rows[r].allglue && si->rows[r].min>0 )
		si->rows[r].sized = max;
    }

    for ( i=si->width = si->minwidth = 0; i<gb->cols; ++i ) {
	si->width += si->cols[i].sized;
	si->minwidth += si->cols[i].min;
    }
    for ( i=0, si->height=si->minheight = si->label_height; i<gb->rows; ++i ) {
	si->height += si->rows[i].sized;
	si->minheight += si->rows[i].min;
    }
}

static void GHVBoxResize(GGadget *g, int32_t width, int32_t height) {
    struct sizeinfo si;
    GHVBox *gb = (GHVBox *) g;
    int bp = GBoxBorderWidth(g->base,g->box);
    int i,c,r,spanc, spanr, totc,totr, glue_cnt, plus, extra;
    int x,y;
    int old_enabled = GDrawEnableExposeRequests(g->base,false);

    GHVBoxGatherSizeInfo(gb,&si);
    width -= 2*bp; height -= 2*bp;

    if(width < si.minwidth) width = si.minwidth;
    if(height < si.minheight) height = si.minheight;

    gb->g.inner.x = gb->g.r.x + bp; gb->g.inner.y = gb->g.r.y + bp;
    if ( gb->label!=NULL ) {
        gb->label_height = si.label_height;
        g->inner.y += si.label_height;
    }

    if ( si.width!=width ) {
        int vcols=0;
        for ( i=0; i<gb->cols-1; ++i )
            if ( si.cols[i].sized>0 )
                ++vcols;
        int vcols1 = vcols;
        if(si.cols[gb->cols-1].sized > 0)
            ++ vcols1;
        if ( width<si.width ) {
            for ( i=0; i<gb->cols; ++i )
                si.cols[i].sized = si.cols[i].min;
            si.width = si.minwidth;
            if ( width<si.width && gb->hpad>1 && vcols>0 ) {
                int reduce_pad = (si.width-width)/vcols + 1;
                if ( reduce_pad>gb->hpad-1 ) reduce_pad = gb->hpad-1;
                for ( i=0; i<gb->cols-1; ++i )
                    if ( si.cols[i].sized > 0 )
                        si.cols[i].sized -= reduce_pad;
                si.width -= vcols*reduce_pad;
            }
        }
        if((width > si.width) && (gb->grow_col==gb_expandglue || gb->grow_col==gb_expandgluesame )) {
            for ( i=glue_cnt=0; i<gb->cols; ++i )
                if ( si.cols[i].allglue )
                    ++glue_cnt;
            if ( glue_cnt!=0 ) {
                plus = (width-si.width)/glue_cnt;
                extra = (width-si.width-glue_cnt*plus);
                for ( i=0; i<gb->cols; ++i ) if ( si.cols[i].allglue ) {
                    si.cols[i].sized += plus + (extra>0);
                    si.width += plus + (extra>0);
                    --extra;
                }
            }
        } 
        if ((width != si.width) && gb->grow_col>=0 ) {
            int * ss = &(si.cols[gb->grow_col].sized);
            int w = si.width - *ss;
            *ss += (width-si.width);
            if(*ss < gb->hpad + 3)
                *ss = gb->hpad + 3;
            si.width = w + *ss;
        } 
        if ((width > si.width) && (vcols1!=0)) {
            plus = (width-si.width)/vcols1;
            extra = (width-si.width-vcols1*plus);
            for ( i=0; i<gb->cols; ++i ) {
                if ( si.cols[i].sized>0 ) {
                    si.cols[i].sized += plus + (extra>0);
                    si.width += plus + (extra>0);
                    --extra;
                }
            }
        }
        width = si.width;
    }

    if ( si.height!=height ) {
        int vrows=0;
        for ( i=0; i<gb->rows-1; ++i )
            if ( si.rows[i].sized>0 )
                ++vrows;
        int vrows1 = vrows;
        if(si.rows[gb->rows-1].sized > 0)
            ++ vrows1;
        if ( height<si.height ) {
            for ( i=0; i<gb->rows; ++i )
                si.rows[i].sized = si.rows[i].min;
            si.height = si.minheight;
            if ( height<si.height && gb->vpad>1 && vrows>0 ) {
                int reduce_pad = (si.height-height)/vrows + 1;
                if ( reduce_pad>gb->vpad-1 ) reduce_pad = gb->vpad-1;
                for ( i=0; i<gb->rows-1; ++i )
                    if ( si.rows[i].sized > 0 )
                        si.rows[i].sized -= reduce_pad;
                si.height -= vrows*reduce_pad;
            }
        }
        if((height > si.height) && (gb->grow_row==gb_expandglue || gb->grow_row==gb_expandgluesame )) {
            for ( i=glue_cnt=0; i<gb->rows; ++i )
                if ( si.rows[i].allglue )
                    ++glue_cnt;
            if ( glue_cnt!=0 ){
                plus = (height-si.height)/glue_cnt;
                extra = (height-si.height-glue_cnt*plus);
                for ( i=0; i<gb->rows; ++i ) if ( si.rows[i].allglue ) {
                    si.rows[i].sized += plus + (extra>0);
                    si.height += plus + (extra>0);
                    --extra;
                }
            }
        } 
        if ((height != si.height) && gb->grow_row>=0 ) {
            int * ss = &(si.rows[gb->grow_row].sized);
            int h = si.height - *ss;
            *ss += (height-si.height);
            if(*ss < gb->vpad + 3)
                *ss = gb->vpad + 3;
            si.height = h + *ss;
        } 
        if ((height > si.height) && (vrows1!=0)) {
            plus = (height-si.height)/vrows1;
            extra = (height-si.height-vrows1*plus);
            for ( i=0; i<gb->rows; ++i ) {
                if ( si.rows[i].sized>0 ) {
                    si.rows[i].sized += plus + (extra>0);
                    si.height += plus + (extra>0);
                    --extra;
                }
            }
        }
        height = si.height;
    }

    y = gb->g.inner.y;
    if ( gb->label!=NULL ) {
        if ( gb->label->state!=gs_invisible )
            GGadgetResize( gb->label, si.label_width, si.label_height);
        GGadgetMove( gb->label, gb->g.inner.x+10, y-si.label_height-bp/2);
    }
    for ( r=0; r<gb->rows; ++r ) {
        x = gb->g.inner.x;
        for ( c=0; c<gb->cols; ++c ) {
            GGadget *g = gb->children[r*gb->cols+c];
            if ( g==GG_Glue || g==GG_ColSpan || g==GG_RowSpan || g==GG_HPad10 )
                /* Skip it */;
            else {
                int xes, yes, es;
                totr = si.rows[r].sized;
                for ( spanr=1; r+spanr<gb->rows &&
                        gb->children[(r+spanr)*gb->cols+c]==GG_RowSpan; ++spanr )
                    totr += si.rows[r+spanr].sized;
                totc = si.cols[c].sized;
                for ( spanc=1; c+spanc<gb->cols &&
                        gb->children[r*gb->cols+c+spanc]==GG_ColSpan; ++spanc )
                    totc += si.cols[c+spanc].sized;
                if ( r+spanr!=gb->rows ) totr -= gb->vpad;
                if ( c+spanc!=gb->cols ) totc -= gb->hpad;
                es = GBoxExtraSpace(g);
                xes = si.cols[c].extra_space - es;
                yes = si.rows[r].extra_space - es;
                if ( g->state!=gs_invisible )
                    GGadgetResize(g,totc-2*xes,totr-2*yes);
                GGadgetMove(g,x+xes,y+yes);
            }
            x += si.cols[c].sized;
        }
        y += si.rows[r].sized;
    }

    free(si.cols); free(si.rows);

    gb->g.inner.width = width; gb->g.inner.height = height;
    gb->g.r.width = width + 2*bp; gb->g.r.height = height + 2*bp;
    GDrawEnableExposeRequests(g->base,old_enabled);
    GDrawRequestExpose(g->base,&g->r,false);
}

static void GHVBoxGetDesiredSize(GGadget *g, GRect *outer, GRect *inner) {
    struct sizeinfo si;
    GHVBox *gb = (GHVBox *) g;
    int bp = GBoxBorderWidth(g->base,g->box);

    GHVBoxGatherSizeInfo(gb,&si);
    if ( g->desired_width>0 ) si.width = g->desired_width;
    if ( g->desired_height>0 ) si.height = g->desired_height;

    if ( inner!=NULL ) {
	inner->x = inner->y = 0;
	inner->width = si.width; inner->height = si.height;
    }
    if ( outer!=NULL ) {
	outer->x = outer->y = 0;
	outer->width = si.width+2*bp; outer->height = si.height+2*bp;
    }
    free(si.cols); free(si.rows);
}

static int GHVBoxFillsWindow(GGadget *g) {
return( true );
}

static int expose_nothing(GWindow pixmap, GGadget *g, GEvent *event) {
    GHVBox *gb = (GHVBox *) g;
    GRect r;

    if ( g->state==gs_invisible )
return( true );

    if ( gb->label==NULL )
	GBoxDrawBorder(pixmap,&g->r,g->box,g->state,false);
    else {
	r = g->r;
	r.y += gb->label_height/2;
	r.height -= gb->label_height/2;
	GBoxDrawBorder(pixmap,&r,g->box,g->state,false);
	/* Background is transparent */
	(gb->label->funcs->handle_expose)(pixmap,gb->label,event);
    }
return( true );
}

struct gfuncs ghvbox_funcs = {
    0,
    sizeof(struct gfuncs),

    expose_nothing,	/* Expose */
    _ggadget_noop,	/* Mouse */
    _ggadget_noop,	/* Key */
    NULL,
    NULL,		/* Focus */
    NULL,
    NULL,

    _ggadget_redraw,
    GHVBoxMove,
    GHVBoxResize,
    _ggadget_setvisible,
    _ggadget_setenabled,
    _ggadget_getsize,
    _ggadget_getinnersize,

    GHVBox_destroy,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    GHVBoxGetDesiredSize,
    _ggadget_setDesiredSize,
    GHVBoxFillsWindow,
    NULL
};

void GHVBoxSetExpandableCol(GGadget *g,int col) {
    GHVBox *gb = (GHVBox *) g;
    if ( col<gb->cols )
	gb->grow_col = col;
}

void GHVBoxSetExpandableRow(GGadget *g,int row) {
    GHVBox *gb = (GHVBox *) g;
    if ( row < gb->rows )
	gb->grow_row = row;
}

void GHVBoxSetPadding(GGadget *g,int hpad, int vpad) {
    GHVBox *gb = (GHVBox *) g;
    gb->hpad = hpad;
    gb->vpad = vpad;
}

static GHVBox *_GHVBoxCreate(struct gwindow *base, GGadgetData *gd,void *data,
	int hcnt, int vcnt, GBox *def_box) {
    GHVBox *gb = calloc(1,sizeof(GHVBox));
    int i, h, v;
    GGadgetCreateData *label = (GGadgetCreateData *) (gd->label);

    _GHVBox_Init();

    gd->label = NULL;
    gb->g.funcs = &ghvbox_funcs;
    _GGadget_Create(&gb->g,base,gd,data,def_box);
    gb->rows = vcnt; gb->cols = hcnt;
    gb->grow_col = gb->grow_row = gb_expandall;
    gb->hpad = gb->vpad = GDrawPointsToPixels(base,2);

    gb->g.takes_input = false; gb->g.takes_keyboard = false; gb->g.focusable = false;

    if ( label != NULL ) {
	gb->label = label->ret =
		(label->creator)(base,&label->gd,label->data);
	gb->label->contained = true;
    }

    gb->children = malloc(vcnt*hcnt*sizeof(GGadget *));
    for ( i=v=0; v<vcnt; ++v ) {
	for ( h=0; h<hcnt && gd->u.boxelements[i]!=NULL; ++h, ++i ) {
	    GGadgetCreateData *gcd = gd->u.boxelements[i];
	    if ( gcd==GCD_Glue ||
		    gcd==GCD_ColSpan ||
		    gcd==GCD_RowSpan ||
		    gcd==GCD_HPad10 )
		gb->children[v*hcnt+h] = (GGadget *) gcd;
	    else {
		gcd->gd.pos.x = gcd->gd.pos.y = 1;
		gb->children[v*hcnt+h] = gcd->ret =
			(gcd->creator)(base,&gcd->gd,gcd->data);
		gcd->ret->contained = true;
	    }
	}
	while ( h<hcnt )
	    gb->children[v*hcnt+h++] = GG_Glue;
	if ( gd->u.boxelements[i]==NULL )
	    ++i;
    }
return( gb );
}

GGadget *GHBoxCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GHVBox *gb;
    int hcnt;

    for ( hcnt=0; gd->u.boxelements[hcnt]!=NULL; ++hcnt );
    gb = _GHVBoxCreate(base,gd,data,hcnt,1,&hvbox_box);

return( &gb->g );
}

GGadget *GVBoxCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GHVBox *gb;
    int vcnt;

    for ( vcnt=0; gd->u.boxelements[vcnt]!=NULL; ++vcnt );
    gb = _GHVBoxCreate(base,gd,data,1,vcnt,&hvbox_box);

return( &gb->g );
}

GGadget *GHVBoxCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GHVBox *gb;
    int hcnt, vcnt, i;

    for ( hcnt=0; gd->u.boxelements[hcnt]!=NULL; ++hcnt );
    for ( i=0, vcnt=1; gd->u.boxelements[i]!=NULL || gd->u.boxelements[i+1]!=NULL ; ++i )
	if ( gd->u.boxelements[i]==NULL )
	    ++vcnt;
    gb = _GHVBoxCreate(base,gd,data,hcnt,vcnt,&hvbox_box);

return( &gb->g );
}

GGadget *GHVGroupCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GHVBox *gb;
    int hcnt, vcnt, i;

    for ( hcnt=0; gd->u.boxelements[hcnt]!=NULL; ++hcnt );
    for ( i=0, vcnt=1; gd->u.boxelements[i]!=NULL || gd->u.boxelements[i+1]!=NULL ; ++i )
	if ( gd->u.boxelements[i]==NULL )
	    ++vcnt;
    gb = _GHVBoxCreate(base,gd,data,hcnt,vcnt,&hvgroup_box);

return( &gb->g );
}

static void _GHVBoxFitWindow(GGadget *g, int center) {
    GRect outer, cur, screen;

    if ( !GGadgetFillsWindow(g)) {
	fprintf( stderr, "Call to GHVBoxFitWindow in something not an HVBox\n" );
return;
    }
    GHVBoxGetDesiredSize(g,&outer, NULL );
    GDrawGetSize(GDrawGetRoot(NULL),&screen);
    if ( outer.width > screen.width-20 ) outer.width = screen.width-20;
    if ( outer.height > screen.height-40 ) outer.height = screen.height-40;
    GDrawGetSize(g->base,&cur);
    /* Make any offset symmetrical */
    outer.width += 2*g->r.x;
    outer.height += 2*g->r.y;
    if ( cur.width!=outer.width || cur.height!=outer.height ) {
	GDrawResize(g->base, outer.width, outer.height );
	/* We want to get the resize before we set the window visible */
	/*  and window managers make synchronizing an issue... */
	GDrawSync(GDrawGetDisplayOfWindow(g->base));
	GDrawProcessPendingEvents(GDrawGetDisplayOfWindow(g->base));
	GDrawSync(GDrawGetDisplayOfWindow(g->base));
	GDrawProcessPendingEvents(GDrawGetDisplayOfWindow(g->base));
    } else
	GGadgetResize(g, outer.width-2*g->r.x, outer.height-2*g->r.y );
    if ( center ) {
	GDrawMove(g->base,(screen.width-outer.width)/2,(screen.height-outer.height)/2);
	/* I don't think this one matters as much, but try a little */
	GDrawSync(GDrawGetDisplayOfWindow(g->base));
	GDrawProcessPendingEvents(GDrawGetDisplayOfWindow(g->base));
    }
}

void GHVBoxFitWindow(GGadget *g) {
    _GHVBoxFitWindow(g,false);
}

void GHVBoxFitWindowCentered(GGadget *g) {
    _GHVBoxFitWindow(g,true);
}

void GHVBoxReflow(GGadget *g) {
    GHVBoxResize(g, g->r.width, g->r.height);
}
