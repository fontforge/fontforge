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
#include "cvundoes.h"
#include "fontforgeui.h"
#include <math.h>
#include "splinefont.h"
#include "ustring.h"

static void SpAdjustTo(SplinePoint *sp,real newx, real newy) {
    sp->prevcp.x += newx-sp->me.x;
    sp->nextcp.x += newx-sp->me.x;
    sp->prevcp.y += newy-sp->me.y;
    sp->nextcp.y += newy-sp->me.y;
    sp->me.x = newx;
    sp->me.y = newy;
}

static void SpaceOne(CharView *cv,SplinePoint *sp) {
    SplinePoint *prev, *next;
    BasePoint v, new;
    real len, off;
    /* Rotate the coordinate system so that one axis is parallel to the */
    /*  line between sp->next->to and sp->prev->from. Position sp so that */
    /*  it is mid-way between the two on that axis while its distance from */
    /*  the axis (ie. the other coord) remains unchanged */
    /* Of course we do this with dot products rather than actual rotations */

    if ( sp->next==NULL || sp->prev==NULL )
return;

    prev = sp->prev->from; next = sp->next->to;
    if ( prev==next )
return;

    v.x = next->me.x - prev->me.x;
    v.y = next->me.y - prev->me.y;
    len = sqrt(v.x*v.x + v.y*v.y);
    if ( len==0 )
return;
    v.x /= len; v.y /= len;
    off = (sp->me.x-prev->me.x)*v.y - (sp->me.y-prev->me.y)*v.x;
    new.x = (next->me.x + prev->me.x)/2 + off*v.y;
    new.y = (next->me.y + prev->me.y)/2 - off*v.x;

    CVPreserveState((CharViewBase *) cv);
    SpAdjustTo(sp,new.x,new.y);
    SplineRefigure(sp->prev); SplineRefigure(sp->next);
    CVCharChangedUpdate(&cv->b);
}

static void SpaceMany(CharView *cv,DBounds *b, int dir, int region_size, int cnt) {
    SplinePoint *sp;
    SplineSet *spl;
    struct region { real begin, end, offset; } *regions;
    int rcnt,i,j;
    real range, rtot, space, rpos;

    if ( dir==-1 ) {
	if ( b->maxx - b->minx > b->maxy - b->miny )
	    dir = 0;		/* Space out along x axis */
	else
	    dir = 1;		/* Space out along y axis */
    }

    regions = malloc(cnt*sizeof(struct region));
    rcnt = 0;
    for ( spl= cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl=spl->next ) {
	sp=spl->first;
	while ( 1 ) {
	    if ( sp->selected ) {
		real coord = dir?sp->me.y:sp->me.x;
		for ( i=0; i<rcnt && coord>regions[i].end+region_size; ++i );
		if ( i==rcnt ) {
		    regions[i].begin = regions[i].end = coord;
		    ++rcnt;
		} else if ( coord<regions[i].begin-region_size ) {
		    for ( j=++rcnt; j>i; --j )
			regions[j] = regions[j-1];
		    regions[i].begin = regions[i].end = coord;
		} else {
		    if ( regions[i].begin>coord )
			regions[i].begin = coord;
		    else if ( regions[i].end < coord ) {
			regions[i].end = coord;
			if ( i<rcnt-1 && regions[i].end+region_size>=regions[i+1].begin ) {
			    /* Merge two regions */
			    regions[i].end = regions[i+1].end;
			    --rcnt;
			    for ( j=i+1; j<rcnt; ++j )
				regions[j] = regions[j+1];
			}
		    }
		}
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }

    /* we need at least three regions to space things out */
    if ( rcnt<3 )
return;

    /* Now should I allow equal spaces between regions, or spaces between */
    /*  region mid-points? I think spaces between regions */
    range = regions[rcnt-1].end-regions[0].begin;
    rtot = 0;
    for ( j=0; j<rcnt; ++j )
	rtot += regions[j].end-regions[j].begin;
    space = (range-rtot)/(rcnt-1);
    rpos = regions[0].begin;
    for ( j=0; j<rcnt-1; ++j ) {
	regions[j].offset = rpos-regions[j].begin;
	rpos += regions[j].end-regions[j].begin+space;
    }
    regions[rcnt-1].offset = 0;

    CVPreserveState((CharViewBase *) cv);
    for ( spl= cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl=spl->next ) {
	sp=spl->first;
	while ( 1 ) {
	    if ( sp->selected ) {
		real coord = dir?sp->me.y:sp->me.x;
		for ( i=0; i<rcnt && coord>regions[i].end; ++i );
		if ( i==rcnt )
		    IError( "Region list is screwed up");
		else {
		    if ( dir==0 ) {
			sp->me.x += regions[i].offset;
			sp->prevcp.x += regions[i].offset;
			sp->nextcp.x += regions[i].offset;
		    } else {
			sp->me.y += regions[i].offset;
			sp->prevcp.y += regions[i].offset;
			sp->nextcp.y += regions[i].offset;
		    }
		    if ( sp->prev!=NULL )
			SplineRefigure(sp->prev);
		    if ( sp->next!=NULL )
			SplineRefigure(sp->next);
		}
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }
    CVCharChangedUpdate(&cv->b);
    free(regions);
}


static void AlignTwoMaybeAsk(CharView *cv,SplinePoint *sp1, SplinePoint *sp2)
{
    real xoff, yoff, xpos, ypos;
    int HorizontalAlignment = 0;

    xoff = sp1->me.x - sp2->me.x;
    yoff = sp1->me.y - sp2->me.y;
//  printf("AlignTwo() %f %f, %f %f,   xoff:%f  yoff:%f\n",
//	   sp1->me.x, sp2->me.x,
//	   sp1->me.y, sp2->me.y,
//	   xoff, yoff );

    if ( fabs(yoff)<fabs(xoff)/2 ) {
//	printf("average y\n");
	/* average y */
	HorizontalAlignment = 0;
    } else if ( fabs(xoff)<fabs(yoff)/2 ) {
	/* average x */
//	printf("average x\n");
	HorizontalAlignment = 1;
    }
    else {
	char *buts[5];

	buts[0] = _("_Horizontal");
	buts[1] = _("_Vertical");
	buts[2] = _("_Cancel");
	buts[3] = NULL;

	int asked = gwwv_ask(_("Align Points"),(const char **) buts,0,3,_("How to align these points?"));
	HorizontalAlignment = asked;
	if( asked == 2 )
	    return;
    }

    CVPreserveState((CharViewBase *) cv);
    if ( HorizontalAlignment == 0 )
    {
//	printf("average y\n");
	/* average y */
	ypos = rint( (sp1->me.y+sp2->me.y)/2 );
	sp1->prevcp.y += ypos-sp1->me.y;
	sp1->nextcp.y += ypos-sp1->me.y;
	sp2->prevcp.y += ypos-sp2->me.y;
	sp2->nextcp.y += ypos-sp2->me.y;
	sp1->me.y = sp2->me.y = ypos;
    }
    else
    {
	/* average x */
//	printf("average x\n");
	xpos = rint( (sp1->me.x+sp2->me.x)/2 );
	sp1->prevcp.x += xpos-sp1->me.x;
	sp1->nextcp.x += xpos-sp1->me.x;
	sp2->prevcp.x += xpos-sp2->me.x;
	sp2->nextcp.x += xpos-sp2->me.x;
	sp1->me.x = sp2->me.x = xpos;
    }

    if ( sp1->prev ) SplineRefigure(sp1->prev);
    if ( sp1->next ) SplineRefigure(sp1->next);
    if ( sp2->prev ) SplineRefigure(sp2->prev);
    if ( sp2->next ) SplineRefigure(sp2->next);
    CVCharChangedUpdate(&cv->b);
}

static void AlignManyMaybeAsk(CharView *cv,DBounds *b) {
    real xoff, yoff, xpos, ypos;
    SplinePoint *sp;
    SplineSet *spl;
    int HorizontalAlignment = 0;

    xoff = b->maxx - b->minx;
    yoff = b->maxy - b->miny;

    if ( yoff<xoff )
    {
	/* average y */
	HorizontalAlignment = 0;
    }
    else if ( xoff<yoff/2 )
    {
	/* constrain x */
	HorizontalAlignment = 1;
    }
    else
    {
	char *buts[5];

	buts[0] = _("_Horizontal");
	buts[1] = _("_Vertical");
	buts[2] = _("_Cancel");
	buts[3] = NULL;

	int asked = gwwv_ask(_("Align Points"),(const char **) buts,0,3,_("How to align these points?"));
	HorizontalAlignment = asked;
	if( asked == 2 )
	    return;
    }

    CVPreserveState((CharViewBase *) cv);
    if ( HorizontalAlignment == 0 )
    {
	/* average y */
	ypos = rint( (b->maxy+b->miny)/2 );
	for ( spl= cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl=spl->next )
	{
	    sp=spl->first;
	    while ( 1 ) {
		if ( sp->selected ) {
		    sp->prevcp.y += ypos-sp->me.y;
		    sp->nextcp.y += ypos-sp->me.y;
		    sp->me.y = ypos;
		    if ( sp->prev ) SplineRefigure(sp->prev);
		    if ( sp->next ) SplineRefigure(sp->next);
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	}
    }
    else
    {
	/* constrain x */
	xpos = rint( (b->maxx+b->minx)/2 );
	for ( spl= cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl=spl->next )
	{
	    sp=spl->first;
	    while ( 1 ) {
		if ( sp->selected ) {
		    sp->prevcp.x += xpos-sp->me.x;
		    sp->nextcp.x += xpos-sp->me.x;
		    sp->me.x = xpos;
		    if ( sp->prev ) SplineRefigure(sp->prev);
		    if ( sp->next ) SplineRefigure(sp->next);
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	}
    }
    CVCharChangedUpdate(&cv->b);
}


struct rcd {
    CharView *cv;
    int done;
    DBounds *b;
    int cnt;
};
static double lastsize = 100;

#define CID_Y		1001
#define CID_X		1002
#define CID_Size	1003


static int RC_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct rcd *rcd = GDrawGetUserData(gw);
	int err=false;
	real size;
	int dir = GGadgetIsChecked(GWidgetGetControl(gw,CID_Y));
	size = GetReal8(gw,CID_Size,_("_Size:"),&err);
	if ( err )
return(true);
	SpaceMany(rcd->cv,rcd->b, dir, size, rcd->cnt);

	rcd->done = true;
    }
return( true );
}

static int RC_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct rcd *rcd = GDrawGetUserData(GGadgetGetWindow(g));
	rcd->done = true;
    }
return( true );
}

static int rcd_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct rcd *rcd = GDrawGetUserData(gw);
	rcd->done = true;
    }
return( event->type!=et_char );
}

static void RegionControl(CharView *cv,DBounds *b,int cnt) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[9], boxes[5], *rarray[5], *narray[5], *barray[10], *hvarray[7][2];
    GTextInfo label[9];
    struct rcd rcd;
    char buffer[20];

    rcd.cv = cv;
    rcd.b = b;
    rcd.cnt = cnt;
    rcd.done = false;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = _("Space Regions");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,225));
	pos.height = GDrawPointsToPixels(NULL,115);
	gw = GDrawCreateTopWindow(NULL,&pos,rcd_e_h,&rcd,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));
	memset(&boxes,0,sizeof(boxes));

	label[0].text = (unichar_t *) _("Coordinate along which to space");
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 6;
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].creator = GLabelCreate;
	hvarray[0][0] = &gcd[0]; hvarray[0][1] = NULL;

	label[1].text = (unichar_t *) _("_X");
	label[1].text_is_1byte = true;
	label[1].text_in_resource = true;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.pos.x = 25; gcd[1].gd.pos.y = gcd[0].gd.pos.y+13;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].creator = GRadioCreate;
	gcd[1].gd.cid = CID_X;
	rarray[0] = GCD_HPad10; rarray[1] = &gcd[1];

	label[2].text = (unichar_t *) _("_Y");
	label[2].text_is_1byte = true;
	label[2].text_in_resource = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.pos.x = 60; //FIXME: gcd[2].gd.pos.y = gcd[2].gd.pos.y;
	gcd[2].gd.flags = gg_enabled|gg_visible;
	gcd[2].gd.cid = CID_Y;
	gcd[2].creator = GRadioCreate;
	rarray[2] = &gcd[2];
	rarray[3] = GCD_Glue; rarray[4] = NULL;

	boxes[2].gd.flags = gg_enabled | gg_visible;
	boxes[2].gd.u.boxelements = rarray;
	boxes[2].creator = GHBoxCreate;
	hvarray[1][0] = &boxes[2]; hvarray[1][1] = NULL;

	if ( b->maxx-b->minx > b->maxy-b->miny )
	    gcd[1].gd.flags |= gg_cb_on;
	else
	    gcd[2].gd.flags |= gg_cb_on;

	label[3].text = (unichar_t *) _("_Maximum distance between points in a region");
	label[3].text_is_1byte = true;
	label[3].text_in_resource = true;
	gcd[3].gd.label = &label[3];
	gcd[3].gd.pos.x = 5; gcd[3].gd.pos.y = gcd[1].gd.pos.y+16;
	gcd[3].gd.flags = gg_enabled|gg_visible;
	gcd[3].creator = GLabelCreate;
	hvarray[2][0] = &gcd[3]; hvarray[2][1] = NULL;

	sprintf( buffer, "%g", lastsize );
	label[4].text = (unichar_t *) buffer;
	label[4].text_is_1byte = true;
	gcd[4].gd.label = &label[4];
	gcd[4].gd.pos.x = gcd[1].gd.pos.x; gcd[4].gd.pos.y = gcd[3].gd.pos.y+14;  gcd[4].gd.pos.width = 40;
	gcd[4].gd.flags = gg_enabled|gg_visible;
	gcd[4].gd.cid = CID_Size;
	gcd[4].creator = GNumericFieldCreate;
	narray[0] = GCD_HPad10; narray[1] = &gcd[4]; narray[2] = GCD_Glue; narray[3] = NULL;

	boxes[3].gd.flags = gg_enabled | gg_visible;
	boxes[3].gd.u.boxelements = narray;
	boxes[3].creator = GHBoxCreate;
	hvarray[3][0] = &boxes[3]; hvarray[3][1] = NULL;

	gcd[5].gd.pos.x = 20-3; gcd[5].gd.pos.y = gcd[4].gd.pos.y+35-3;
	gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[5].text = (unichar_t *) _("_OK");
	label[5].text_is_1byte = true;
	label[5].text_in_resource = true;
	gcd[5].gd.mnemonic = 'O';
	gcd[5].gd.label = &label[5];
	gcd[5].gd.handle_controlevent = RC_OK;
	gcd[5].creator = GButtonCreate;
	barray[0] = GCD_Glue; barray[1] = &gcd[5]; barray[2] = barray[3] = barray[4] = barray[5] = GCD_Glue;

	gcd[6].gd.pos.x = -20; gcd[6].gd.pos.y = gcd[5].gd.pos.y+3;
	gcd[6].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[6].text = (unichar_t *) _("_Cancel");
	label[6].text_is_1byte = true;
	label[6].text_in_resource = true;
	gcd[6].gd.label = &label[6];
	gcd[6].gd.mnemonic = 'C';
	gcd[6].gd.handle_controlevent = RC_Cancel;
	gcd[6].creator = GButtonCreate;
	barray[5] = &gcd[6]; barray[6] = GCD_Glue; barray[7] = NULL;

	boxes[4].gd.flags = gg_enabled | gg_visible;
	boxes[4].gd.u.boxelements = barray;
	boxes[4].creator = GHBoxCreate;
	hvarray[4][0] = GCD_Glue; hvarray[4][1] = NULL;
	hvarray[5][0] = &boxes[4]; hvarray[5][1] = NULL;
	hvarray[6][0] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled | gg_visible;
	boxes[0].gd.u.boxelements = hvarray[0];
	boxes[0].creator = GHVGroupCreate;

	GGadgetsCreate(gw,boxes);
	GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[4].ret,gb_expandgluesame);
	GHVBoxFitWindow(boxes[0].ret);

    GTextFieldSelect(GWidgetGetControl(gw,CID_Size),0,-1);

    GDrawSetVisible(gw,true);
    while ( !rcd.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

void CVConstrainSelection(CharView *cv,constrainSelection_t type) {
    DBounds b;
    SplinePoint *first=NULL, *second=NULL, *other=NULL, *sp;
    SplineSet *spl;
    int cnt=0;

    for ( spl= cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl=spl->next ) {
	sp=spl->first;
	while ( 1 ) {
	    if ( sp->selected ) {
		++cnt;
		if ( first == NULL ) {
		    first = sp;
		    b.minx = b.maxx = sp->me.x;
		    b.miny = b.maxy = sp->me.y;
		} else {
		    if ( second==NULL ) second = sp;
		    else other = sp;
		    if ( b.minx>sp->me.x ) b.minx = sp->me.x;
		    if ( b.maxx<sp->me.x ) b.maxx = sp->me.x;
		    if ( b.miny>sp->me.y ) b.miny = sp->me.y;
		    if ( b.maxy<sp->me.y ) b.maxy = sp->me.y;
		}
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }

    if ( type == constrainSelection_AveragePoints ) {
	/* Average points */
	if ( second==NULL )
	    /* Do Nothing */;
	else if ( !other )
	    AlignTwoMaybeAsk(cv,first,second);
	else
	    AlignManyMaybeAsk(cv,&b);
    } else if ( type == constrainSelection_SpacePoints ) {
	/* Space points */
	if ( other!=NULL )
	    SpaceMany(cv,&b,-1,0,cnt);
	else if ( second!=NULL )
	    /* Can't deal with it */;
	else if ( first!=NULL )
	    SpaceOne(cv,first);
	else {
	    /* Nothing selected */;
	}
    } else if ( type == constrainSelection_SpaceSelectedRegions ) {
	/* Space selected regions */
	if ( other==NULL )
return; 	/* Do nothing, need at least three regions */
	RegionControl(cv,&b,cnt);
    }
}

static void MakeParallel(Spline *e1, Spline *e2, SplinePoint *mobile) {
    /* Splines e1&e2 are to be made parallel */
    /* The spline containing mobile is the one which is to be moved */
    Spline *temp;
    double xdiff, ydiff, axdiff, aydiff;
    SplinePoint *other;

    if ( e1->to==mobile || e1->from==mobile ) {
	temp = e1;
	e1 = e2;
	e2 = temp;
    }
    /* Now the spline to be moved is e2 */
    other = e2->from==mobile ? e2->to : e2->from;

    if ( (axdiff = xdiff = e1->to->me.x-e1->from->me.x)<0 ) axdiff = -axdiff;
    if ( (aydiff = ydiff = e1->to->me.y-e1->from->me.y)<0 ) aydiff = -aydiff;
    if ( aydiff > axdiff ) {
	/* Hold the y coordinate fixed in e2 and move the x coord appropriately */
	int oldx = mobile->me.x;
	mobile->me.x = (mobile->me.y-other->me.y)*xdiff/ydiff + other->me.x;
	mobile->nextcp.x += mobile->me.x-oldx;
	mobile->prevcp.x += mobile->me.x-oldx;
    } else {
	int oldy = mobile->me.y;
	mobile->me.y = (mobile->me.x-other->me.x)*ydiff/xdiff + other->me.y;
	mobile->nextcp.y += mobile->me.y-oldy;
	mobile->prevcp.y += mobile->me.y-oldy;
    }
    if ( mobile->next!=NULL )
	SplineRefigure(mobile->prev);
    if ( mobile->prev!=NULL )
	SplineRefigure(mobile->next);
}

static void MakeParallelogram(Spline *e11, Spline *e12, Spline *e21, Spline *e22,
	SplinePoint *mobile) {
    /* Splines e11&e12 are to be made parallel, as are e21&e22 */
    /* The spline containing mobile is the one which is to be moved */
    Spline *temp;
    SplinePoint *other1, *other2, *unconnected;
    double denom;

    if ( e11->to==mobile || e11->from==mobile ) {
	temp = e11;
	e11 = e12;
	e12 = temp;
    }
    if ( e21->to==mobile || e21->from==mobile ) {
	temp = e21;
	e21 = e22;
	e22 = temp;
    }
    /* Now the splines to be moved are e?2, while e?1 is held fixed */
    other1 = e12->from==mobile ? e12->to : e12->from;
    other2 = e22->from==mobile ? e22->to : e22->from;
    unconnected = e11->from==other2 ? e11->to : e11->from;

    denom = (unconnected->me.y-other1->me.y)*(unconnected->me.x-other2->me.x) -
	    (unconnected->me.y-other2->me.y)*(unconnected->me.x-other1->me.x);
    if ( denom>-.0001 && denom<.0001 ) {
	/* The two splines e11 and e21 are essentially parallel, so we can't */
	/*  move mobile to the place where they intersect */
	mobile->me = unconnected->me;
    } else {
	mobile->me.y =
	    ((other2->me.x-other1->me.x)*(unconnected->me.y-other1->me.y)*
		    (unconnected->me.y-other2->me.y) -
		other2->me.y*(unconnected->me.y-other2->me.y)*(unconnected->me.x-other1->me.x) +
		other1->me.y*(unconnected->me.y-other1->me.y)*(unconnected->me.x-other2->me.x))/
	     denom;
	if ( unconnected->me.y-other1->me.y==0 )
	    mobile->me.x = other1->me.x + (unconnected->me.x-other2->me.x)/(unconnected->me.y-other2->me.y)*
		    (mobile->me.y-other1->me.y);
	else
	    mobile->me.x = other2->me.x + (unconnected->me.x-other1->me.x)/(unconnected->me.y-other1->me.y)*
		    (mobile->me.y-other2->me.y);
    }
    mobile->nextcp = mobile->prevcp = mobile->me;
    SplineRefigure(mobile->prev);
    SplineRefigure(mobile->next);
}

static int CommonEndPoint(Spline *s1, Spline *s2) {
return( s1->to==s2->to || s1->to==s2->from || s1->from==s2->to || s1->from==s2->from );
}

void CVMakeParallel(CharView *cv) {
    /* takes exactly four selected points and tries to find two lines between */
    /*  them which it then makes parallel */
    /* If the points are a quadralateral then we make it a parallelogram */
    /* If the points define 3 lines then throw out the middle one */
    /* If the points define 2 lines then good */
    /* Else complain */
    /* If possible, fix things by moving the lastselpt */
    SplinePoint *pts[4];
    Spline *edges[4];
    int cnt=0, mobilis;
    SplineSet *ss;
    SplinePoint *sp;

    for ( ss = cv->b.layerheads[cv->b.drawmode]->splines; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    if ( sp->selected ) {
		if ( cnt>=4 )
return;
		pts[cnt++] = sp;
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp == ss->first )
	break;
	}
    }
    if ( cnt!=4 )
return;

    for ( mobilis=0; mobilis<4; ++mobilis )
	if ( pts[mobilis]==cv->lastselpt )
    break;
    if ( mobilis==4 ) mobilis=3;		/* lastselpt didn't match, pick one */

    cnt=0;
    if ( pts[0]->next!=NULL && pts[0]->next->islinear &&
	    (pts[0]->next->to==pts[1] ||
	     pts[0]->next->to==pts[2] ||
	     pts[0]->next->to==pts[3]) &&
	     (pts[0]->me.x!=pts[0]->next->to->me.x || pts[0]->me.y!=pts[0]->next->to->me.y))
	 edges[cnt++] = pts[0]->next;
    if ( pts[1]->next!=NULL && pts[1]->next->islinear &&
	    (pts[1]->next->to==pts[0] ||
	     pts[1]->next->to==pts[2] ||
	     pts[1]->next->to==pts[3]) &&
	     (pts[1]->me.x!=pts[1]->next->to->me.x || pts[1]->me.y!=pts[1]->next->to->me.y))
	 edges[cnt++] = pts[1]->next;
    if ( pts[2]->next!=NULL && pts[2]->next->islinear &&
	    (pts[2]->next->to==pts[0] ||
	     pts[2]->next->to==pts[1] ||
	     pts[2]->next->to==pts[3]) &&
	     (pts[2]->me.x!=pts[2]->next->to->me.x || pts[2]->me.y!=pts[2]->next->to->me.y))
	 edges[cnt++] = pts[2]->next;
    if ( pts[3]->next!=NULL && pts[3]->next->islinear &&
	    (pts[3]->next->to==pts[0] ||
	     pts[3]->next->to==pts[1] ||
	     pts[3]->next->to==pts[2]) &&
	     (pts[3]->me.x!=pts[3]->next->to->me.x || pts[3]->me.y!=pts[3]->next->to->me.y))
	 edges[cnt++] = pts[3]->next;

    if ( cnt<2 ) {
	ff_post_error(_("Not enough lines"),_("Not enough lines"));
return;
    } else if ( cnt==2 && CommonEndPoint(edges[0],edges[1]) ) {
	ff_post_error(_("Can't Parallel"),_("These two lines share a common endpoint, I can't make them parallel"));
return;
    }

    CVPreserveState((CharViewBase *) cv);
    if ( cnt==4 ) {
	int second=3, third=1, fourth=2;
	if ( !CommonEndPoint(edges[0],edges[1])) {
	    second = 1; third = 3;
	} else if ( !CommonEndPoint(edges[0],edges[2])) {
	    second = 2;
	    fourth = 3;
	}
	MakeParallelogram(edges[0],edges[second], edges[third],edges[fourth],
		pts[mobilis]);
    } else if ( cnt==3 ) {
	if ( !CommonEndPoint(edges[0],edges[1]) )
	    MakeParallel(edges[0],edges[1],pts[mobilis]);
	else if ( !CommonEndPoint(edges[0],edges[2]) )
	    MakeParallel(edges[0],edges[2],pts[mobilis]);
	else
	    MakeParallel(edges[1],edges[2],pts[mobilis]);
    } else {
	MakeParallel(edges[0],edges[1],pts[mobilis]);
    }
    CVCharChangedUpdate(&cv->b);
}
