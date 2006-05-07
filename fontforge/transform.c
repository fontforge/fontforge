/* Copyright (C) 2000-2006 by George Williams */
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
#include <math.h>
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include "ustring.h"
#include <gkeysym.h>

#define TCnt	3

typedef struct transdata {
    void *userdata;
    void (*transfunc)(void *,real trans[6],int otype,BVTFunc *,enum fvtrans_flags);
    int  (*getorigin)(void *,BasePoint *bp,int otype);
    GWindow tblock[TCnt];
    GWindow gw;
    int done;
} TransData;

#define CID_Origin		1101
#define CID_DoBackground	1102
#define CID_Round2Int		1103
#define CID_DoKerns		1104
#define CID_DoSimplePos		1105

#define CID_Type	1001
#define CID_XMove	1002
#define CID_YMove	1003
#define CID_Angle	1004
#define CID_Scale	1005
#define CID_XScale	1006
#define CID_YScale	1007
#define CID_Horizontal	1008
#define CID_Vertical	1009
#define CID_SkewAng	1010
#define CID_XLab	1011
#define CID_YLab	1012
#define CID_CounterClockwise	1013
#define CID_Clockwise	1014
#define CID_XPercent	1015
#define CID_YPercent	1016
#define CID_XDegree	1017
#define CID_YDegree	1018
#define CID_XAxis	1019
#define CID_YAxis	1020

#define CID_First	CID_Type
#define CID_Last	CID_YAxis

#define TBlock_Width	270
#define TBlock_Height	40
#define TBlock_Top	32
#define TBlock_XStart	130

static GTextInfo origin[] = {
    { (unichar_t *) N_("Glyph Origin"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Center of Selection"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
/* GT: The (x,y) position on the window where the user last pressed a mouse button */
    { (unichar_t *) N_("Last Press"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { NULL }};

static int selcid[] = { 0, CID_XMove, CID_Angle, CID_Scale, CID_XScale, 0, CID_SkewAng, CID_XAxis };
static GTextInfo transformtypes[] = {
    { (unichar_t *) N_("Do Nothing"), NULL, 0, 0, (void *) 0x1000, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Move..."), NULL, 0, 0, (void *) 0x1, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Rotate..."), NULL, 0, 0, (void *) 0x2, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Scale Uniformly..."), NULL, 0, 0, (void *) 0x4, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Scale..."), NULL, 0, 0, (void *) 0x8, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Flip..."), NULL, 0, 0, (void *) 0x10, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Skew..."), NULL, 0, 0, (void *) 0x20, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Rotate 3D Around..."), NULL, 0, 0, (void *) 0x40, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Move by Ruler..."), NULL, 0, 0, (void *) 0x401, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Rotate by Ruler..."), NULL, 0, 0, (void *) 0x402, 0, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) N_("Skew by Ruler..."), NULL, 0, 0, (void *) 0x420, 0, 0, 0, 0, 0, 0, 0, 1 },
    { NULL }};

#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
void skewselect(BVTFunc *bvtf,real t) {
    real off, bestoff;
    int i, best;

    bestoff = 10; best = 0;
    for ( i=1; i<=10; ++i ) {
	if (  (off = t*i-rint(t*i))<0 ) off = -off;
	if ( off<bestoff ) {
	    bestoff = off;
	    best = i;
	}
    }

    bvtf->func = bvt_skew;
    bvtf->x = rint(t*best);
    bvtf->y = best;
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static int Trans_OK(GGadget *g, GEvent *e) {
    GWindow tw;
    real transform[6], trans[6], t[6];
    double angle, angle2;
    int i, index, err;
    int dobackground = false, round_2_int = false, dokerns = false, dokp=false;
    BasePoint base;
    int origin, bvpos=0;
    BVTFunc bvts[TCnt+1];
    static int warned = false;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	TransData *td = GDrawGetUserData(GGadgetGetWindow(g));

	transform[0] = transform[3] = 1.0;
	transform[1] = transform[2] = transform[4] = transform[5] = 0;
	base.x = base.y = 0;

	origin = GGadgetGetFirstListSelectedItem( GWidgetGetControl(td->gw,CID_Origin));
	if ( GWidgetGetControl(td->gw,CID_DoBackground)!=NULL )
	    dobackground = GGadgetIsChecked(GWidgetGetControl(td->gw,CID_DoBackground));
	if ( GWidgetGetControl(td->gw,CID_DoSimplePos)!=NULL )
	    dokp = GGadgetIsChecked(GWidgetGetControl(td->gw,CID_DoSimplePos));
	if ( GWidgetGetControl(td->gw,CID_DoKerns)!=NULL )
	    dokerns = GGadgetIsChecked(GWidgetGetControl(td->gw,CID_DoKerns));
	round_2_int = GGadgetIsChecked(GWidgetGetControl(td->gw,CID_Round2Int));
	if ( td->getorigin!=NULL ) {
	    (td->getorigin)(td->userdata,&base,origin );
	    transform[4] = -base.x;
	    transform[5] = -base.y;
	}
	for ( i=0; i<TCnt; ++i ) {
	    tw = td->tblock[i];
	    index =  GGadgetGetFirstListSelectedItem( GWidgetGetControl(tw,CID_Type));
	    trans[0] = trans[3] = 1.0;
	    trans[1] = trans[2] = trans[4] = trans[5] = 0;
	    err = 0;
	    switch ( index ) {
	      case 0:		/* Do Nothing */
	      break;
	      case 1:		/* Move */
		trans[4] = GetReal8(tw,CID_XMove,_("X Movement"),&err);
		trans[5] = GetReal8(tw,CID_YMove,_("Y Movement"),&err);
		bvts[bvpos].x = trans[4]; bvts[bvpos].y = trans[5]; bvts[bvpos++].func = bvt_transmove;
	      break;
	      case 2:		/* Rotate */
		angle = GetReal8(tw,CID_Angle,_("Rotation Angle"),&err);
		if ( GGadgetIsChecked( GWidgetGetControl(tw,CID_Clockwise)) )
		    angle = -angle;
		if ( fmod(angle,90)!=0 )
		    bvts[0].func = bvt_none;		/* Bad trans=> No trans */
		else {
		    angle = fmod(angle,360);
		    if ( angle<0 ) angle+=360;
		    if ( angle==90 ) bvts[bvpos++].func = bvt_rotate90ccw;
		    else if ( angle==180 ) bvts[bvpos++].func = bvt_rotate180;
		    else if ( angle==270 ) bvts[bvpos++].func = bvt_rotate90cw;
		}
		angle *= 3.1415926535897932/180;
		trans[0] = trans[3] = cos(angle);
		trans[2] = -(trans[1] = sin(angle));
	      break;
	      case 3:		/* Scale Uniformly */
		trans[0] = trans[3] = GetReal8(tw,CID_Scale,_("Scale Factor"),&err)/100.0;
		bvts[0].func = bvt_none;		/* Bad trans=> No trans */
	      break;
	      case 4:		/* Scale */
		trans[0] = GetReal8(tw,CID_XScale,_("X Scale Factor"),&err)/100.0;
		trans[3] = GetReal8(tw,CID_YScale,_("Y Scale Factor"),&err)/100.0;
		bvts[0].func = bvt_none;		/* Bad trans=> No trans */
	      break;
	      case 5:		/* Flip */
		if ( GGadgetIsChecked( GWidgetGetControl(tw,CID_Horizontal)) ) {
		    trans[0] = -1;
		    bvts[bvpos++].func = bvt_fliph;
		} else {
		    trans[3] = -1;
		    bvts[bvpos++].func = bvt_flipv;
		}
	      break;
	      case 6:		/* Skew */
		angle = GetReal8(tw,CID_SkewAng,_("Skew Angle"),&err);
		if ( GGadgetIsChecked( GWidgetGetControl(tw,CID_Clockwise)) )
		    angle = -angle;
		angle *= 3.1415926535897932/180;
		trans[2] = tan(angle);
		skewselect(&bvts[bvpos],trans[2]); ++bvpos;
	      break;
	      case 7:		/* 3D rotate */
		angle =  GetReal8(tw,CID_XAxis,_("Rotation about X Axis"),&err) * 3.1415926535897932/180;
		angle2 = GetReal8(tw,CID_YAxis,_("Rotation about Y Axis"),&err) * 3.1415926535897932/180;
		trans[0] = cos(angle2);
		trans[3] = cos(angle );
		bvts[0].func = bvt_none;		/* Bad trans=> No trans */
	      break;
	      default:
		IError("Unexpected selection in Transform");
		err = 1;
	      break;
	    }
	    if ( err )
return(true);
#if 0
 printf( "(%g,%g,%g,%g,%g,%g)*(%g,%g,%g,%g,%g,%g) = ",
     trans[0], trans[1], trans[2], trans[3], trans[4], trans[5],
     transform[0], transform[1], transform[2], transform[3], transform[4], transform[5]);
#endif
	    t[0] = transform[0]*trans[0] +
			transform[1]*trans[2];
	    t[1] = transform[0]*trans[1] +
			transform[1]*trans[3];
	    t[2] = transform[2]*trans[0] +
			transform[3]*trans[2];
	    t[3] = transform[2]*trans[1] +
			transform[3]*trans[3];
	    t[4] = transform[4]*trans[0] +
			transform[5]*trans[2] +
			trans[4];
	    t[5] = transform[4]*trans[1] +
			transform[5]*trans[3] +
			trans[5];
	    memcpy(transform,t,sizeof(t));
#if 0
 printf( "(%g,%g,%g,%g,%g,%g)\n",
     transform[0], transform[1], transform[2], transform[3], transform[4], transform[5]);
#endif
	}
	bvts[bvpos++].func = bvt_none;		/* Done */
	for ( i=0; i<6; ++i )
	    if ( RealNear(transform[i],0)) transform[i] = 0;
	transform[4] += base.x;
	transform[5] += base.y;

	if (( transform[1]!=0 || transform[2]!=0 ) && !warned ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    gwwv_post_notice(_("Warning"),_("After rotating or skewing a glyph you should probably apply Element->Add Extrema"));
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_notice(_("Warning"),_("After rotating or skewing a character you should probably apply Element->Add Extrema"));
#endif
	    warned = true;
	}
	(td->transfunc)(td->userdata,transform,origin,bvts,
		(dobackground?fvt_dobackground:0)|
		 (round_2_int?fvt_round_to_int:0)|
		 (dokp?fvt_scalepstpos:0)|
		 (dokerns?fvt_scalekernclasses:0));
	td->done = true;
    }
return( true );
}

static int Trans_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	TransData *td = GDrawGetUserData(GGadgetGetWindow(g));
	td->done = true;
    }
return( true );
}

static int Trans_TypeChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	GWindow bw = GGadgetGetWindow(g);
	int index = GGadgetGetFirstListSelectedItem(g);
	int mask = (intpt) transformtypes[index].userdata;
	int i;

	if ( mask & 0x400 ) {
	    real xoff = last_ruler_offset[0].x, yoff = last_ruler_offset[0].y;
	    char buf[24]; unichar_t ubuf[24];
	    if ( mask & 0x20 )
		index -= 4;	/* skew */
	    else
		index -= 7;	/* move or rotate */
	    GGadgetSelectOneListItem( g,index );
	    mask &= ~0x400;
	    if ( mask&1 ) {		/* Move */
		sprintf( buf, "%.1f", xoff );
		uc_strcpy(ubuf,buf);
		GGadgetSetTitle(GWidgetGetControl(bw,CID_XMove), ubuf );
		sprintf( buf, "%.1f", yoff );
		uc_strcpy(ubuf,buf);
		GGadgetSetTitle(GWidgetGetControl(bw,CID_YMove), ubuf );
	    } else {
		sprintf( buf, "%.0f", atan2(yoff,xoff)*180/3.1415926535897932 );
		uc_strcpy(ubuf,buf);
		GGadgetSetTitle(GWidgetGetControl(bw,(mask&0x2)?CID_Angle:CID_SkewAng), ubuf );
		GGadgetSetChecked(GWidgetGetControl(bw,CID_Clockwise), false );
		GGadgetSetChecked(GWidgetGetControl(bw,CID_CounterClockwise), true );
	    }
	}

	for ( i=CID_First; i<=CID_Last; ++i ) {
	    GGadget *sg;
	    sg = GWidgetGetControl(bw,i);
	    GGadgetSetVisible(sg, ( ((intpt) GGadgetGetUserData(sg))&mask )?1:0);
	}
	if ( selcid[index]!=0 ) {
	    GGadget *tf = GWidgetGetControl(bw,selcid[index]);
	    GWidgetIndicateFocusGadget(tf);
	    GTextFieldSelect(tf,0,-1);
	}
	GDrawRequestExpose(bw,NULL,false);
    }
return( true );
}

static int trans_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	TransData *td = GDrawGetUserData(gw);
	td->done = true;
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("transform.html");
return( true );
	}
return( false );
    }
return( true );
}

static void MakeTransBlock(TransData *td,int bnum) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[23];
    GTextInfo label[23];

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.cursor = ct_pointer;
    pos.x = 0; pos.y = GDrawPointsToPixels(NULL,TBlock_Top+bnum*TBlock_Height);
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,TBlock_Width));
    pos.height = GDrawPointsToPixels(NULL,TBlock_Height);
    td->tblock[bnum] = gw = GDrawCreateSubWindow(td->gw,&pos,NULL,td,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 9;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].gd.label = &transformtypes[bnum==0?1:0];
    gcd[0].gd.u.list = transformtypes;
    gcd[0].gd.cid = CID_Type;
    gcd[0].creator = GListButtonCreate;
    gcd[0].data = (void *) -1;
    gcd[0].gd.handle_controlevent = Trans_TypeChange;
    transformtypes[bnum==0].selected = true;
    transformtypes[bnum!=0].selected = false;

    label[1].text = (unichar_t *) _("_X");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = TBlock_XStart; gcd[1].gd.pos.y = 15; 
    gcd[1].gd.flags = bnum==0? (gg_enabled|gg_visible) : gg_enabled;
    gcd[1].data = (void *) 0x49;
    gcd[1].gd.cid = CID_XLab;
    gcd[1].creator = GLabelCreate;

    label[2].text = (unichar_t *) "0";
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = TBlock_XStart+10; gcd[2].gd.pos.y = 9;  gcd[2].gd.pos.width = 40;
    gcd[2].gd.flags = bnum==0? (gg_enabled|gg_visible) : gg_enabled;
    gcd[2].data = (void *) 0x1;
    gcd[2].gd.cid = CID_XMove;
    gcd[2].creator = GTextFieldCreate;

    label[3].text = (unichar_t *) _("_Y");
    label[3].text_is_1byte = true;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = TBlock_XStart+70; gcd[3].gd.pos.y = 15; 
    gcd[3].gd.flags = bnum==0? (gg_enabled|gg_visible) : gg_enabled;
    gcd[3].data = (void *) 0x49;
    gcd[3].gd.cid = CID_YLab;
    gcd[3].creator = GLabelCreate;

    label[4].text = (unichar_t *) "0";
    label[4].text_is_1byte = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.pos.x = TBlock_XStart+80; gcd[4].gd.pos.y = 9;  gcd[4].gd.pos.width = 40;
    gcd[4].gd.flags = bnum==0? (gg_enabled|gg_visible) : gg_enabled;
    gcd[4].data = (void *) 0x1;
    gcd[4].gd.cid = CID_YMove;
    gcd[4].creator = GTextFieldCreate;

    label[5].text = (unichar_t *) "100";
    label[5].text_is_1byte = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.pos.x = TBlock_XStart+10; gcd[5].gd.pos.y = 9;  gcd[5].gd.pos.width = 40;
    gcd[5].gd.flags = gg_enabled;
    gcd[5].data = (void *) 0x8;
    gcd[5].gd.cid = CID_XScale;
    gcd[5].creator = GTextFieldCreate;

    label[6].text = (unichar_t *) "100";
    label[6].text_is_1byte = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.pos.x = TBlock_XStart+80; gcd[6].gd.pos.y = 9;  gcd[6].gd.pos.width = 40;
    gcd[6].gd.flags = gg_enabled;
    gcd[6].data = (void *) 0x8;
    gcd[6].gd.cid = CID_YScale;
    gcd[6].creator = GTextFieldCreate;

    label[7].text = (unichar_t *) "100";
    label[7].text_is_1byte = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.pos.x = TBlock_XStart+10; gcd[7].gd.pos.y = 9;  gcd[7].gd.pos.width = 40;
    gcd[7].gd.flags = gg_enabled;
    gcd[7].data = (void *) 0x4;
    gcd[7].gd.cid = CID_Scale;
    gcd[7].creator = GTextFieldCreate;

    label[8].text = (unichar_t *) U_("° Clockwise");
    label[8].text_is_1byte = true;
    gcd[8].gd.label = &label[8];
    gcd[8].gd.pos.x = TBlock_XStart+53; gcd[8].gd.pos.y = 2; gcd[8].gd.pos.height = 12;
    gcd[8].gd.flags = gg_enabled;
    gcd[8].data = (void *) 0x22;
    gcd[8].gd.cid = CID_Clockwise;
    gcd[8].creator = GRadioCreate;

/* GT: Sometimes spelled Widdershins. An old word which means counter clockwise. */
/* GT: I used it because "counter clockwise" took too much space. */
    label[9].text = (unichar_t *) U_("° Withershins");	/* deiseal */
    label[9].text_is_1byte = true;
    gcd[9].gd.label = &label[9];
    gcd[9].gd.pos.x = TBlock_XStart+53; gcd[9].gd.pos.y = 17; gcd[9].gd.pos.height = 12;
    gcd[9].gd.flags = gg_enabled | gg_cb_on;
    gcd[9].data = (void *) 0x22;
    gcd[9].gd.cid = CID_CounterClockwise;
    gcd[9].creator = GRadioCreate;

    label[10].text = (unichar_t *) "180";
    label[10].text_is_1byte = true;
    gcd[10].gd.label = &label[10];
    gcd[10].gd.pos.x = TBlock_XStart+10; gcd[10].gd.pos.y = 9;  gcd[10].gd.pos.width = 40;
    gcd[10].gd.flags = gg_enabled;
    gcd[10].data = (void *) 0x2;
    gcd[10].gd.cid = CID_Angle;
    gcd[10].creator = GTextFieldCreate;

    label[11].text = (unichar_t *) "10";	/* -10 if we default clockwise */
    label[11].text_is_1byte = true;
    gcd[11].gd.label = &label[11];
    gcd[11].gd.pos.x = TBlock_XStart+10; gcd[11].gd.pos.y = 9;  gcd[11].gd.pos.width = 40;
    gcd[11].gd.flags = gg_enabled;
    gcd[11].data = (void *) 0x20;
    gcd[11].gd.cid = CID_SkewAng;
    gcd[11].creator = GTextFieldCreate;

    label[12].text = (unichar_t *) _("Horizontal");
    label[12].text_is_1byte = true;
    gcd[12].gd.label = &label[12];
    gcd[12].gd.pos.x = TBlock_XStart; gcd[12].gd.pos.y = 2; gcd[12].gd.pos.height = 12;
    gcd[12].gd.flags = gg_enabled | gg_cb_on;
    gcd[12].data = (void *) 0x10;
    gcd[12].gd.cid = CID_Horizontal;
    gcd[12].creator = GRadioCreate;

    label[13].text = (unichar_t *) _("Vertical");
    label[13].text_is_1byte = true;
    gcd[13].gd.label = &label[13];
    gcd[13].gd.pos.x = TBlock_XStart; gcd[13].gd.pos.y = 17; gcd[13].gd.pos.height = 12;
    gcd[13].gd.flags = gg_enabled;
    gcd[13].data = (void *) 0x10;
    gcd[13].gd.cid = CID_Vertical;
    gcd[13].creator = GRadioCreate;

    label[14].text = (unichar_t *) _("%");
    label[14].text_is_1byte = true;
    gcd[14].gd.label = &label[14];
    gcd[14].gd.pos.x = TBlock_XStart+51; gcd[14].gd.pos.y = 15; 
    gcd[14].gd.flags = gg_enabled;
    gcd[14].data = (void *) 0xc;
    gcd[14].gd.cid = CID_XPercent;
    gcd[14].creator = GLabelCreate;

    label[15].text = (unichar_t *) _("%");
    label[15].text_is_1byte = true;
    gcd[15].gd.label = &label[15];
    gcd[15].gd.pos.x = TBlock_XStart+121; gcd[15].gd.pos.y = 15; 
    gcd[15].gd.flags = gg_enabled;
    gcd[15].data = (void *) 0x8;
    gcd[15].gd.cid = CID_YPercent;
    gcd[15].creator = GLabelCreate;

    label[16].text = (unichar_t *) U_("°");
    label[16].text_is_1byte = true;
    gcd[16].gd.label = &label[16];
    gcd[16].gd.pos.x = TBlock_XStart+51; gcd[16].gd.pos.y = 15; 
    gcd[16].gd.flags = gg_enabled;
    gcd[16].data = (void *) 0x40;
    gcd[16].gd.cid = CID_XDegree;
    gcd[16].creator = GLabelCreate;

    label[17].text = (unichar_t *) U_("°");
    label[17].text_is_1byte = true;
    gcd[17].gd.label = &label[17];
    gcd[17].gd.pos.x = TBlock_XStart+121; gcd[17].gd.pos.y = 15; 
    gcd[17].gd.flags = gg_enabled;
    gcd[17].data = (void *) 0x40;
    gcd[17].gd.cid = CID_YDegree;
    gcd[17].creator = GLabelCreate;

    label[18].text = (unichar_t *) "45";
    label[18].text_is_1byte = true;
    gcd[18].gd.label = &label[18];
    gcd[18].gd.pos.x = TBlock_XStart+10; gcd[18].gd.pos.y = 9;  gcd[18].gd.pos.width = 40;
    gcd[18].gd.flags = gg_enabled;
    gcd[18].data = (void *) 0x40;
    gcd[18].gd.cid = CID_XAxis;
    gcd[18].creator = GTextFieldCreate;

    label[19].text = (unichar_t *) "0";
    label[19].text_is_1byte = true;
    gcd[19].gd.label = &label[19];
    gcd[19].gd.pos.x = TBlock_XStart+80; gcd[19].gd.pos.y = 9;  gcd[19].gd.pos.width = 40;
    gcd[19].gd.flags = gg_enabled;
    gcd[19].data = (void *) 0x40;
    gcd[19].gd.cid = CID_YAxis;
    gcd[19].creator = GTextFieldCreate;

    gcd[20].gd.pos.x = 2; gcd[20].gd.pos.y = 1;
    gcd[20].gd.pos.width = TBlock_Width-3; gcd[20].gd.pos.height = TBlock_Height-2;
    gcd[20].gd.flags = gg_enabled | gg_visible;
    gcd[20].data = (void *) -1;
    gcd[20].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    GDrawSetVisible(gw,true);
}

void TransformDlgCreate(void *data,void (*transfunc)(void *,real *,int,BVTFunc *,enum fvtrans_flags),
	int (*getorigin)(void *,BasePoint *,int), int enableback,
	enum cvtools cvt) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[9];
    GTextInfo label[8];
    static TransData td;
    int i, len, y;
    GGadget *orig;
    BasePoint junk;
    GTextInfo **ti;
    static int done = false;

    if ( !done ) {
	int i;
	for ( i=0; transformtypes[i].text!=NULL; ++i )
	    transformtypes[i].text = (unichar_t *) _((char *) transformtypes[i].text);
	for ( i=0; origin[i].text!=NULL; ++i )
	    origin[i].text = (unichar_t *) _((char *) origin[i].text);
	done = true;
    }

    td.userdata = data;
    td.transfunc = transfunc;
    td.getorigin = getorigin;
    td.done = false;

    if ( td.gw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = _("Transform...");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,TBlock_Width));
	pos.height = GDrawPointsToPixels(NULL,TBlock_Top+TCnt*TBlock_Height+110);
	td.gw = gw = GDrawCreateTopWindow(NULL,&pos,trans_e_h,&td,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 4;
	gcd[0].gd.flags = (getorigin==NULL) ? gg_visible : (gg_visible | gg_enabled);
	gcd[0].gd.label = &origin[1];
	gcd[0].gd.u.list = origin;
	gcd[0].gd.cid = CID_Origin;
	gcd[0].creator = GListButtonCreate;
	origin[1].selected = true;

	i = 1; y = TBlock_Top+TCnt*TBlock_Height+4;

	    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = y;
	    gcd[i].gd.flags = (enableback&1) ? (gg_visible | gg_enabled) : gg_visible;
	    label[i].text = (unichar_t *) _("Transform _Background Too");
	    label[i].text_is_1byte = true;
	    label[i].text_in_resource = true;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.cid = CID_DoBackground;
	    gcd[i++].creator = GCheckBoxCreate;
	    y += 16;

	    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = y;
	    gcd[i].gd.flags = gg_visible | (enableback&2 ? gg_enabled : 0) |
		    (enableback&4 ? gg_cb_on : 0);
	    label[i].text = (unichar_t *) _("Transform kerning _classes too");
	    label[i].text_is_1byte = true;
	    label[i].text_in_resource = true;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.cid = CID_DoKerns;
	    gcd[i++].creator = GCheckBoxCreate;
	    y += 16;

	    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = y;
	    gcd[i].gd.flags = gg_visible |
		    (enableback&1 ? gg_enabled : 0) |
		    (enableback&2 ? gg_cb_on : 0);
	    label[i].text = (unichar_t *) _("Transform simple positioning features & _kern pairs");
	    label[i].text_is_1byte = true;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.cid = CID_DoSimplePos;
	    gcd[i++].creator = GCheckBoxCreate;
	    y += 16;

	    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = y;
	    gcd[i].gd.flags = gg_visible | gg_enabled;
	    label[i].text = (unichar_t *) _("Round To _Int");
	    label[i].text_is_1byte = true;
	    label[i].text_in_resource = true;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.cid = CID_Round2Int;
	    gcd[i++].creator = GCheckBoxCreate;
	    y += 24;

	gcd[i].gd.pos.x = 30-3; gcd[i].gd.pos.y = y;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[i].text = (unichar_t *) _("_OK");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.mnemonic = 'O';
	gcd[i].gd.label = &label[i];
	gcd[i].gd.handle_controlevent = Trans_OK;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.pos.x = -30; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[i].text = (unichar_t *) _("_Cancel");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.mnemonic = 'C';
	gcd[i].gd.handle_controlevent = Trans_Cancel;
	gcd[i].creator = GButtonCreate;

	GGadgetsCreate(gw,gcd);

	for ( i=0; i<TCnt; ++i )
	    MakeTransBlock(&td,i);
    }
    gw = td.gw;

    GGadgetSetEnabled( GWidgetGetControl(gw,CID_DoBackground), enableback&1);
    if ( !(enableback&1) )
	GGadgetSetChecked( GWidgetGetControl(gw,CID_DoBackground), false );
    orig = GWidgetGetControl(gw,CID_Origin);
    GGadgetSetEnabled( orig, getorigin!=NULL );
    ti = GGadgetGetList(orig,&len);
    for ( i=0; i<len; ++i ) {
	ti[i]->disabled = !getorigin(data,&junk,i);
	if ( ti[i]->disabled && ti[i]->selected ) {
	    ti[i]->selected = false;
	    ti[0]->selected = true;
	    GGadgetSetTitle(orig,ti[0]->text);
	}
    }

    if ( cvt!=cvt_none ) {
	int index = cvt == cvt_scale  ? 4 :
		    cvt == cvt_flip   ? 5 :
		    cvt == cvt_rotate ? 2 :
		    cvt == cvt_skew   ? 6 :
			   /* 3d rot*/  7 ;
	GGadget *firstoption = GWidgetGetControl(td.tblock[0],CID_Type);
	GEvent dummy;
	GGadgetSelectOneListItem( firstoption, index );
	memset(&dummy,0,sizeof(dummy));
	dummy.type = et_controlevent; dummy.u.control.subtype = et_listselected;
	Trans_TypeChange( firstoption, &dummy );
    }

    for ( i=0; i<TCnt; ++i ) {
	int index = GGadgetGetFirstListSelectedItem(GWidgetGetControl(td.tblock[i],CID_Type));
	if ( selcid[index]!=0 ) {
	    GGadget *tf = GWidgetGetControl(td.tblock[i],selcid[index]);
	    GWidgetIndicateFocusGadget(tf);
	    GTextFieldSelect(tf,0,-1);
    break;
	}
    }

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !td.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(gw,false);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
