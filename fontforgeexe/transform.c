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

#include <fontforge-config.h>

#include "bvedit.h"
#include "fontforgeui.h"
#include "gkeysym.h"
#include "ustring.h"

#include <math.h>

#define TCnt	3

typedef struct transdata {
    void *userdata;
    void (*transfunc)(void *,real trans[6],int otype,BVTFunc *,enum fvtrans_flags);
    int  (*getorigin)(void *,BasePoint *bp,int otype);
    GWindow gw;
    int applied;
    int done;
} TransData;

#define CID_Origin		3001
#define CID_AllLayers		3002
#define CID_Round2Int		3003
#define CID_DoKerns		3004
#define CID_DoSimplePos		3005
#define CID_DoGrid		3006
#define CID_DoWidth		3007
#define CID_Apply		3008

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

#define CID_ClockBox	1021
#define CID_HVBox	1022
#define CID_HBox	1023

#define TBlock_Width	270
#define TBlock_Height	40
#define TBlock_Top	32
#define TBlock_XStart	130
#define TBlock_CIDOffset	100

static GTextInfo origin[] = {
    { (unichar_t *) N_("Glyph Origin"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Center of Selection"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
/* GT: The (x,y) position on the window where the user last pressed a mouse button */
    { (unichar_t *) N_("Last Press"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};

static int selcid[] = { 0, CID_XMove, CID_Angle, CID_Scale, CID_XScale, 0, CID_SkewAng, CID_XAxis };
static GTextInfo transformtypes[] = {
    { (unichar_t *) N_("Do Nothing"), NULL, 0, 0, (void *) 0x1000, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Move..."), NULL, 0, 0, (void *) 0x1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Rotate..."), NULL, 0, 0, (void *) 0x2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Scale Uniformly..."), NULL, 0, 0, (void *) 0x4, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Scale..."), NULL, 0, 0, (void *) 0x8, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Flip..."), NULL, 0, 0, (void *) 0x10, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Skew..."), NULL, 0, 0, (void *) 0x20, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Rotate 3D Around..."), NULL, 0, 0, (void *) 0x40, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Move by Ruler..."), NULL, 0, 0, (void *) 0x401, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Rotate by Ruler..."), NULL, 0, 0, (void *) 0x402, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Skew by Ruler..."), NULL, 0, 0, (void *) 0x420, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};

static int Trans_OK(GGadget *g, GEvent *e) {
    real transform[6], trans[6], t[6];
    bigreal angle, angle2;
    int i, index, err;
    int alllayers = false, round_2_int = false, dokerns = false, dokp=false;
    int dogrid = false, dowidth = false;
    BasePoint base;
    int origin, bvpos=0;
    BVTFunc bvts[TCnt+1];
    static int warned = false;
    int isapply = GGadgetGetCid(g) == CID_Apply;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	TransData *td = GDrawGetUserData(GGadgetGetWindow(g));

	transform[0] = transform[3] = 1.0;
	transform[1] = transform[2] = transform[4] = transform[5] = 0;
	base.x = base.y = 0;

	origin = GGadgetGetFirstListSelectedItem( GWidgetGetControl(td->gw,CID_Origin));
	if ( GWidgetGetControl(td->gw,CID_AllLayers)!=NULL )
	    alllayers = GGadgetIsChecked(GWidgetGetControl(td->gw,CID_AllLayers));
	if ( GWidgetGetControl(td->gw,CID_DoGrid)!=NULL )
	    dogrid = GGadgetIsChecked(GWidgetGetControl(td->gw,CID_DoGrid));
	if ( GWidgetGetControl(td->gw,CID_DoWidth)!=NULL )
	    dowidth = GGadgetIsChecked(GWidgetGetControl(td->gw,CID_DoWidth));
	else
	    dowidth = true;
	if ( GWidgetGetControl(td->gw,CID_DoSimplePos)!=NULL )
	    dokp = GGadgetIsChecked(GWidgetGetControl(td->gw,CID_DoSimplePos));
	if ( GWidgetGetControl(td->gw,CID_DoKerns)!=NULL )
	    dokerns = GGadgetIsChecked(GWidgetGetControl(td->gw,CID_DoKerns));
	round_2_int = GGadgetIsChecked(GWidgetGetControl(td->gw,CID_Round2Int));
	if ( isapply )
	    alllayers = dogrid = dokp = dokerns = false;
	if ( td->getorigin!=NULL ) {
	    (td->getorigin)(td->userdata,&base,origin );
	    transform[4] = -base.x;
	    transform[5] = -base.y;
	}
	for ( i=0; i<TCnt; ++i ) {
	    index =  GGadgetGetFirstListSelectedItem(
		    GWidgetGetControl(td->gw,CID_Type+i*TBlock_CIDOffset));
	    trans[0] = trans[3] = 1.0;
	    trans[1] = trans[2] = trans[4] = trans[5] = 0;
	    err = 0;
	    switch ( index ) {
	      case 0:		/* Do Nothing */
	      break;
	      case 1:		/* Move */
		trans[4] = GetReal8(td->gw,CID_XMove+i*TBlock_CIDOffset,_("X Movement"),&err);
		trans[5] = GetReal8(td->gw,CID_YMove+i*TBlock_CIDOffset,_("Y Movement"),&err);
		bvts[bvpos].x = trans[4]; bvts[bvpos].y = trans[5]; bvts[bvpos++].func = bvt_transmove;
	      break;
	      case 2:		/* Rotate */
		angle = GetReal8(td->gw,CID_Angle+i*TBlock_CIDOffset,_("Rotation Angle"),&err);
		if ( GGadgetIsChecked( GWidgetGetControl(td->gw,CID_Clockwise+i*TBlock_CIDOffset)) )
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
		angle *= FF_PI/180;
		trans[0] = trans[3] = cos(angle);
		trans[2] = -(trans[1] = sin(angle));
	      break;
	      case 3:		/* Scale Uniformly */
		trans[0] = trans[3] = GetReal8(td->gw,CID_Scale+i*TBlock_CIDOffset,_("Scale Factor"),&err)/100.0;
		bvts[0].func = bvt_none;		/* Bad trans=> No trans */
	      break;
	      case 4:		/* Scale */
		trans[0] = GetReal8(td->gw,CID_XScale+i*TBlock_CIDOffset,_("X Scale Factor"),&err)/100.0;
		trans[3] = GetReal8(td->gw,CID_YScale+i*TBlock_CIDOffset,_("Y Scale Factor"),&err)/100.0;
		bvts[0].func = bvt_none;		/* Bad trans=> No trans */
	      break;
	      case 5:		/* Flip */
		if ( GGadgetIsChecked( GWidgetGetControl(td->gw,CID_Horizontal+i*TBlock_CIDOffset)) ) {
		    trans[0] = -1;
		    bvts[bvpos++].func = bvt_fliph;
		} else {
		    trans[3] = -1;
		    bvts[bvpos++].func = bvt_flipv;
		}
	      break;
	      case 6:		/* Skew */
		angle = GetReal8(td->gw,CID_SkewAng+i*TBlock_CIDOffset,_("Skew Angle"),&err);
		if ( GGadgetIsChecked( GWidgetGetControl(td->gw,CID_CounterClockwise+i*TBlock_CIDOffset)) )
		    angle = -angle;
		angle *= FF_PI/180;
		trans[2] = tan(angle);
		skewselect(&bvts[bvpos],trans[2]); ++bvpos;
	      break;
	      case 7:		/* 3D rotate */
		angle =  GetReal8(td->gw,CID_XAxis+i*TBlock_CIDOffset,_("Rotation about X Axis"),&err) * FF_PI/180;
		angle2 = GetReal8(td->gw,CID_YAxis+i*TBlock_CIDOffset,_("Rotation about Y Axis"),&err) * FF_PI/180;
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
	}
	bvts[bvpos++].func = bvt_none;		/* Done */
	for ( i=0; i<6; ++i )
	    if ( RealNear(transform[i],0)) transform[i] = 0;
	transform[4] += base.x;
	transform[5] += base.y;

	if (( transform[1]!=0 || transform[2]!=0 ) && !warned ) {
	    ff_post_notice(_("Warning"),_("After rotating or skewing a glyph you should probably apply Element->Add Extrema"));
	    warned = true;
	}
	(td->transfunc)(td->userdata,transform,origin,bvts,
		(alllayers?fvt_alllayers:0)|
		(dogrid?fvt_dogrid:0)|
		 (dowidth?0:fvt_dontmovewidth)|
		 (round_2_int?fvt_round_to_int:0)|
		 (dokp?fvt_scalepstpos:0)|
		 (dokerns?fvt_scalekernclasses:0)|
		 (isapply?fvt_justapply:0));
	td->done = !isapply;
	td->applied = isapply;
    }
return( true );
}

static int Trans_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	TransData *td = GDrawGetUserData(GGadgetGetWindow(g));
	if ( td->applied )
	    (td->transfunc)(td->userdata,NULL,0,NULL,fvt_revert);
	td->done = true;
    }
return( true );
}

static int Trans_TypeChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	GWindow bw = GGadgetGetWindow(g);
	int offset = GGadgetGetCid(g)-CID_Type;
	int index = GGadgetGetFirstListSelectedItem(g);
	if ( index < 0 ) return( false );
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
		sprintf( buf, "%.1f", (double) xoff );
		uc_strcpy(ubuf,buf);
		GGadgetSetTitle(GWidgetGetControl(bw,CID_XMove+offset), ubuf );
		sprintf( buf, "%.1f", (double) yoff );
		uc_strcpy(ubuf,buf);
		GGadgetSetTitle(GWidgetGetControl(bw,CID_YMove+offset), ubuf );
	    } else {
		sprintf( buf, "%.0f", atan2(yoff,xoff)*180/FF_PI );
		uc_strcpy(ubuf,buf);
		GGadgetSetTitle(GWidgetGetControl(bw,((mask&0x2)?CID_Angle:CID_SkewAng)+offset), ubuf );
		GGadgetSetChecked(GWidgetGetControl(bw,CID_Clockwise+offset), false );
		GGadgetSetChecked(GWidgetGetControl(bw,CID_CounterClockwise+offset), true );
	    }
	}

	for ( i=CID_First; i<=CID_Last; ++i ) {
	    GGadget *sg;
	    sg = GWidgetGetControl(bw,i+offset);
	    GGadgetSetVisible(sg, ( ((intpt) GGadgetGetUserData(sg))&mask )?1:0);
	}
	if ( selcid[index]!=0 ) {
	    GGadget *tf = GWidgetGetControl(bw,selcid[index]+offset);
	    GWidgetIndicateFocusGadget(tf);
	    GTextFieldSelect(tf,0,-1);
	}
	GWidgetToDesiredSize(bw);
	GDrawRequestExpose(bw,NULL,false);
    }
return( true );
}

static int trans_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	TransData *td = GDrawGetUserData(gw);
	td->done = true;
    } else if ( event->type == et_map && event->u.map.is_visible ) {
	/* Above palettes */
	GDrawRaise(gw);
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/transform.html", NULL);
return( true );
	}
return( false );
    }
return( true );
}

static GGadgetCreateData *MakeTransBlock(TransData *td,int bnum,
	GGadgetCreateData *gcd, GTextInfo *label, GGadgetCreateData **array) {
    int offset = bnum*TBlock_CIDOffset;

    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 9;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].gd.label = &transformtypes[0];
    gcd[0].gd.u.list = transformtypes;
    gcd[0].gd.cid = CID_Type+offset;
    gcd[0].creator = GListButtonCreate;
    gcd[0].data = (void *) -1;
    gcd[0].gd.handle_controlevent = Trans_TypeChange;
    transformtypes[0].selected = true;

    label[1].text = (unichar_t *) _("_X");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = TBlock_XStart; gcd[1].gd.pos.y = 15;
    gcd[1].gd.flags = bnum==0? (gg_enabled|gg_visible) : gg_enabled;
    gcd[1].data = (void *) 0x49;
    gcd[1].gd.cid = CID_XLab+offset;
    gcd[1].creator = GLabelCreate;

    label[2].text = (unichar_t *) "0";
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = TBlock_XStart+10; gcd[2].gd.pos.y = 9;  gcd[2].gd.pos.width = 40;
    gcd[2].gd.flags = bnum==0? (gg_enabled|gg_visible) : gg_enabled;
    gcd[2].data = (void *) 0x1;
    gcd[2].gd.cid = CID_XMove+offset;
    gcd[2].creator = GTextFieldCreate;

    label[3].text = (unichar_t *) _("_Y");
    label[3].text_is_1byte = true;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = TBlock_XStart+70; gcd[3].gd.pos.y = 15;
    gcd[3].gd.flags = bnum==0? (gg_enabled|gg_visible) : gg_enabled;
    gcd[3].data = (void *) 0x49;
    gcd[3].gd.cid = CID_YLab+offset;
    gcd[3].creator = GLabelCreate;

    label[4].text = (unichar_t *) "0";
    label[4].text_is_1byte = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.pos.x = TBlock_XStart+80; gcd[4].gd.pos.y = 9;  gcd[4].gd.pos.width = 40;
    gcd[4].gd.flags = bnum==0? (gg_enabled|gg_visible) : gg_enabled;
    gcd[4].data = (void *) 0x1;
    gcd[4].gd.cid = CID_YMove+offset;
    gcd[4].creator = GTextFieldCreate;

    label[5].text = (unichar_t *) "100";
    label[5].text_is_1byte = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.pos.x = TBlock_XStart+10; gcd[5].gd.pos.y = 9;  gcd[5].gd.pos.width = 40;
    gcd[5].gd.flags = gg_enabled;
    gcd[5].data = (void *) 0x8;
    gcd[5].gd.cid = CID_XScale+offset;
    gcd[5].creator = GTextFieldCreate;

    label[6].text = (unichar_t *) "100";
    label[6].text_is_1byte = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.pos.x = TBlock_XStart+80; gcd[6].gd.pos.y = 9;  gcd[6].gd.pos.width = 40;
    gcd[6].gd.flags = gg_enabled;
    gcd[6].data = (void *) 0x8;
    gcd[6].gd.cid = CID_YScale+offset;
    gcd[6].creator = GTextFieldCreate;

    label[7].text = (unichar_t *) "100";
    label[7].text_is_1byte = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.pos.x = TBlock_XStart+10; gcd[7].gd.pos.y = 9;  gcd[7].gd.pos.width = 40;
    gcd[7].gd.flags = gg_enabled;
    gcd[7].data = (void *) 0x4;
    gcd[7].gd.cid = CID_Scale+offset;
    gcd[7].creator = GTextFieldCreate;

    label[8].text = (unichar_t *) U_("° Clockwise");
    label[8].text_is_1byte = true;
    gcd[8].gd.label = &label[8];
    gcd[8].gd.pos.x = TBlock_XStart+53; gcd[8].gd.pos.y = 2; gcd[8].gd.pos.height = 12;
    gcd[8].gd.flags = gg_enabled;
    gcd[8].data = (void *) 0x22;
    gcd[8].gd.cid = CID_Clockwise+offset;
    gcd[8].creator = GRadioCreate;

/* GT: Sometimes spelled Widdershins. An old word which means counter clockwise. */
/* GT: I used it because "counter clockwise" took too much space. */
    label[9].text = (unichar_t *) U_("° Withershins");	/* deiseal */
    label[9].text_is_1byte = true;
    gcd[9].gd.label = &label[9];
    gcd[9].gd.pos.x = TBlock_XStart+53; gcd[9].gd.pos.y = 17; gcd[9].gd.pos.height = 12;
    gcd[9].gd.flags = gg_enabled | gg_cb_on;
    gcd[9].data = (void *) 0x22;
    gcd[9].gd.cid = CID_CounterClockwise+offset;
    gcd[9].creator = GRadioCreate;

    label[10].text = (unichar_t *) "180";
    label[10].text_is_1byte = true;
    gcd[10].gd.label = &label[10];
    gcd[10].gd.pos.x = TBlock_XStart+10; gcd[10].gd.pos.y = 9;  gcd[10].gd.pos.width = 40;
    gcd[10].gd.flags = gg_enabled;
    gcd[10].data = (void *) 0x2;
    gcd[10].gd.cid = CID_Angle+offset;
    gcd[10].creator = GTextFieldCreate;

    label[11].text = (unichar_t *) "10";	/* -10 if we default clockwise */
    label[11].text_is_1byte = true;
    gcd[11].gd.label = &label[11];
    gcd[11].gd.pos.x = TBlock_XStart+10; gcd[11].gd.pos.y = 9;  gcd[11].gd.pos.width = 40;
    gcd[11].gd.flags = gg_enabled;
    gcd[11].data = (void *) 0x20;
    gcd[11].gd.cid = CID_SkewAng+offset;
    gcd[11].creator = GTextFieldCreate;

    label[12].text = (unichar_t *) _("Horizontal");
    label[12].text_is_1byte = true;
    gcd[12].gd.label = &label[12];
    gcd[12].gd.pos.x = TBlock_XStart; gcd[12].gd.pos.y = 2; gcd[12].gd.pos.height = 12;
    gcd[12].gd.flags = gg_enabled | gg_cb_on;
    gcd[12].data = (void *) 0x10;
    gcd[12].gd.cid = CID_Horizontal+offset;
    gcd[12].creator = GRadioCreate;

    label[13].text = (unichar_t *) _("Vertical");
    label[13].text_is_1byte = true;
    gcd[13].gd.label = &label[13];
    gcd[13].gd.pos.x = TBlock_XStart; gcd[13].gd.pos.y = 17; gcd[13].gd.pos.height = 12;
    gcd[13].gd.flags = gg_enabled;
    gcd[13].data = (void *) 0x10;
    gcd[13].gd.cid = CID_Vertical+offset;
    gcd[13].creator = GRadioCreate;

    label[14].text = (unichar_t *) _("%");
    label[14].text_is_1byte = true;
    gcd[14].gd.label = &label[14];
    gcd[14].gd.pos.x = TBlock_XStart+51; gcd[14].gd.pos.y = 15;
    gcd[14].gd.flags = gg_enabled;
    gcd[14].data = (void *) 0xc;
    gcd[14].gd.cid = CID_XPercent+offset;
    gcd[14].creator = GLabelCreate;

    label[15].text = (unichar_t *) _("%");
    label[15].text_is_1byte = true;
    gcd[15].gd.label = &label[15];
    gcd[15].gd.pos.x = TBlock_XStart+121; gcd[15].gd.pos.y = 15;
    gcd[15].gd.flags = gg_enabled;
    gcd[15].data = (void *) 0x8;
    gcd[15].gd.cid = CID_YPercent+offset;
    gcd[15].creator = GLabelCreate;

    label[16].text = (unichar_t *) U_("°");
    label[16].text_is_1byte = true;
    gcd[16].gd.label = &label[16];
    gcd[16].gd.pos.x = TBlock_XStart+51; gcd[16].gd.pos.y = 15;
    gcd[16].gd.flags = gg_enabled;
    gcd[16].data = (void *) 0x40;
    gcd[16].gd.cid = CID_XDegree+offset;
    gcd[16].creator = GLabelCreate;

    label[17].text = (unichar_t *) U_("°");
    label[17].text_is_1byte = true;
    gcd[17].gd.label = &label[17];
    gcd[17].gd.pos.x = TBlock_XStart+121; gcd[17].gd.pos.y = 15;
    gcd[17].gd.flags = gg_enabled;
    gcd[17].data = (void *) 0x40;
    gcd[17].gd.cid = CID_YDegree+offset;
    gcd[17].creator = GLabelCreate;

    label[18].text = (unichar_t *) "45";
    label[18].text_is_1byte = true;
    gcd[18].gd.label = &label[18];
    gcd[18].gd.pos.x = TBlock_XStart+10; gcd[18].gd.pos.y = 9;  gcd[18].gd.pos.width = 40;
    gcd[18].gd.flags = gg_enabled;
    gcd[18].data = (void *) 0x40;
    gcd[18].gd.cid = CID_XAxis+offset;
    gcd[18].creator = GTextFieldCreate;

    label[19].text = (unichar_t *) "0";
    label[19].text_is_1byte = true;
    gcd[19].gd.label = &label[19];
    gcd[19].gd.pos.x = TBlock_XStart+80; gcd[19].gd.pos.y = 9;  gcd[19].gd.pos.width = 40;
    gcd[19].gd.flags = gg_enabled;
    gcd[19].data = (void *) 0x40;
    gcd[19].gd.cid = CID_YAxis+offset;
    gcd[19].creator = GTextFieldCreate;


    array[0] = &gcd[12]; array[1] = &gcd[13]; array[2] = NULL;
    array[3] = &gcd[8]; array[4] = &gcd[9]; array[5] = NULL;

    gcd[20].gd.flags = gg_enabled|gg_visible;
    gcd[20].gd.u.boxelements = array;
    gcd[20].gd.cid = CID_HVBox+offset;
    gcd[20].creator = GVBoxCreate;

    gcd[21].gd.flags = gg_enabled|gg_visible;
    gcd[21].gd.u.boxelements = array+3;
    gcd[21].gd.cid = CID_ClockBox+offset;
    gcd[21].creator = GVBoxCreate;

    array[6] = &gcd[0];
    array[7] = &gcd[20];
    array[8] = &gcd[1];
    array[9] = &gcd[2];
    array[10] = &gcd[5];
    array[11] = &gcd[7];
    array[12] = &gcd[10];
    array[13] = &gcd[11];
    array[14] = &gcd[18];
    array[15] = &gcd[14];
    array[16] = &gcd[16];
    array[17] = &gcd[21];
    array[18] = &gcd[3];
    array[19] = &gcd[4];
    array[20] = &gcd[6];
    array[21] = &gcd[19];
    array[22] = &gcd[15];
    array[23] = &gcd[17];
    array[24] = GCD_Glue;
    array[25] = NULL;
    array[26] = NULL;

    gcd[22].gd.flags = gg_enabled|gg_visible;
    gcd[22].gd.u.boxelements = array+6;
    gcd[22].gd.cid = CID_HBox+offset;
    gcd[22].creator = GHVGroupCreate;
return( gcd+22 );
}

void TransformDlgCreate(void *data,void (*transfunc)(void *,real *,int,BVTFunc *,enum fvtrans_flags),
	int (*getorigin)(void *,BasePoint *,int), enum transdlg_flags flags,
	enum cvtools cvt) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[12+TCnt*25], boxes[4], *subarray[TCnt*27], *array[2*(TCnt+8)+4], *buttons[13], *origarray[4];
    GTextInfo label[9+TCnt*24];
    static TransData td;
    int i, y, gci, subai, ai;
    int32 len;
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
	wattrs.utf8_window_title = _("Transform");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,TBlock_Width));
	pos.height = GDrawPointsToPixels(NULL,TBlock_Top+TCnt*TBlock_Height+110);
	td.gw = gw = GDrawCreateTopWindow(NULL,&pos,trans_e_h,&td,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));
	memset(&boxes,0,sizeof(boxes));

	label[0].text = (unichar_t *) _("Origin:");
	label[0].text_is_1byte = true;
	label[0].text_in_resource = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 4;
	gcd[0].gd.flags = (getorigin==NULL) ? gg_visible : (gg_visible | gg_enabled);
	gcd[0].creator = GLabelCreate;

	gcd[1].gd.pos.x = 5; gcd[1].gd.pos.y = 4;
	gcd[1].gd.flags = (getorigin==NULL) ? gg_visible : (gg_visible | gg_enabled);
	gcd[1].gd.label = &origin[1];
	gcd[1].gd.u.list = origin;
	gcd[1].gd.cid = CID_Origin;
	gcd[1].creator = GListButtonCreate;
	origin[1].selected = true;

	origarray[0] = &gcd[0]; origarray[1] = &gcd[1]; origarray[2] = GCD_Glue; origarray[3] = NULL;

	boxes[3].gd.flags = gg_enabled|gg_visible;
	boxes[3].gd.u.boxelements = origarray;
	boxes[3].creator = GHBoxCreate;

	array[0] = &boxes[3]; array[1] = NULL;

	gci = 2; subai = 0; ai = 2;
	for ( i=0; i<TCnt; ++i ) {
	    array[ai++] = MakeTransBlock(&td,i,gcd+gci,label+gci,subarray+subai);
	    array[ai++] = NULL;
	    gci += 23; subai += 27;
	}

	y = TBlock_Top+TCnt*TBlock_Height+4;

	    gcd[gci].gd.pos.x = 10; gcd[gci].gd.pos.y = y;
	    gcd[gci].gd.flags = (flags&tdf_enableback) ? (gg_visible | gg_enabled) : gg_visible;
	    label[gci].text = (unichar_t *) _("Transform _All Layers");
	    label[gci].text_is_1byte = true;
	    label[gci].text_in_resource = true;
	    gcd[gci].gd.label = &label[gci];
	    gcd[gci].gd.cid = CID_AllLayers;
	    gcd[gci++].creator = GCheckBoxCreate;
	    array[ai++] = &gcd[gci-1]; array[ai++] = NULL;
	    y += 16;

	    gcd[gci].gd.pos.x = 10; gcd[gci].gd.pos.y = y;
	    gcd[gci].gd.flags = (flags&tdf_enableback) ? (gg_visible | gg_enabled) : gg_visible;
	    label[gci].text = (unichar_t *) _("Transform _Guide Layer Too");
	    label[gci].text_is_1byte = true;
	    label[gci].text_in_resource = true;
	    gcd[gci].gd.label = &label[gci];
	    gcd[gci].gd.cid = CID_DoGrid;
	    gcd[gci++].creator = GCheckBoxCreate;
	    array[ai++] = &gcd[gci-1]; array[ai++] = NULL;
	    y += 16;

	    gcd[gci].gd.pos.x = 10; gcd[gci].gd.pos.y = y;
	    gcd[gci].gd.flags = (flags&tdf_enableback) ? (gg_visible | gg_enabled | gg_cb_on) : gg_visible;
	    label[gci].text = (unichar_t *) _("Transform _Width Too");
	    label[gci].text_is_1byte = true;
	    label[gci].text_in_resource = true;
	    gcd[gci].gd.label = &label[gci];
	    gcd[gci].gd.cid = CID_DoWidth;
	    gcd[gci++].creator = GCheckBoxCreate;
	    array[ai++] = &gcd[gci-1]; array[ai++] = NULL;
	    y += 16;

	    gcd[gci].gd.pos.x = 10; gcd[gci].gd.pos.y = y;
	    gcd[gci].gd.flags = gg_visible | (flags&tdf_enablekerns ? gg_enabled : 0) |
		    (flags&tdf_defaultkerns ? gg_cb_on : 0);
	    label[gci].text = (unichar_t *) _("Transform kerning _classes too");
	    label[gci].text_is_1byte = true;
	    label[gci].text_in_resource = true;
	    gcd[gci].gd.label = &label[gci];
	    gcd[gci].gd.cid = CID_DoKerns;
	    gcd[gci++].creator = GCheckBoxCreate;
	    array[ai++] = &gcd[gci-1]; array[ai++] = NULL;
	    y += 16;

	    gcd[gci].gd.pos.x = 10; gcd[gci].gd.pos.y = y;
	    gcd[gci].gd.flags = gg_visible |
		    (flags&tdf_enableback ? gg_enabled : 0) |
		    (flags&tdf_enablekerns ? gg_cb_on : 0);
	    label[gci].text = (unichar_t *) _("Transform simple positioning features & _kern pairs");
	    label[gci].text_is_1byte = true;
	    label[gci].text_in_resource = true;
	    gcd[gci].gd.label = &label[gci];
	    gcd[gci].gd.cid = CID_DoSimplePos;
	    gcd[gci++].creator = GCheckBoxCreate;
	    array[ai++] = &gcd[gci-1]; array[ai++] = NULL;
	    y += 16;

	    gcd[gci].gd.pos.x = 10; gcd[gci].gd.pos.y = y;
	    gcd[gci].gd.flags = gg_visible | gg_enabled;
	    label[gci].text = (unichar_t *) _("Round To _Int");
	    label[gci].text_is_1byte = true;
	    label[gci].text_in_resource = true;
	    gcd[gci].gd.label = &label[gci];
	    gcd[gci].gd.cid = CID_Round2Int;
	    gcd[gci++].creator = GCheckBoxCreate;
	    array[ai++] = &gcd[gci-1]; array[ai++] = NULL;
	    y += 24;

	array[ai++] = GCD_Glue; array[ai++] = NULL;

	gcd[gci].gd.pos.x = 30-3; gcd[gci].gd.pos.y = y;
	gcd[gci].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[gci].text = (unichar_t *) _("_OK");
	label[gci].text_is_1byte = true;
	label[gci].text_in_resource = true;
	gcd[gci].gd.mnemonic = 'O';
	gcd[gci].gd.label = &label[gci];
	gcd[gci].gd.handle_controlevent = Trans_OK;
	gcd[gci++].creator = GButtonCreate;
	buttons[0] = GCD_Glue; buttons[1] = &gcd[gci-1]; buttons[2] = GCD_Glue; buttons[3] = GCD_Glue;

	gcd[gci].gd.flags = gg_visible | gg_enabled;
	label[gci].text = (unichar_t *) _("_Apply");
	label[gci].text_is_1byte = true;
	label[gci].text_in_resource = true;
	gcd[gci].gd.label = &label[gci];
	gcd[gci].gd.handle_controlevent = Trans_OK;
	gcd[gci].gd.cid = CID_Apply;
	gcd[gci++].creator = GButtonCreate;
	buttons[4] = GCD_Glue; buttons[5] = &gcd[gci-1]; buttons[6] = GCD_Glue; buttons[7] = GCD_Glue;

	gcd[gci].gd.pos.x = -30; gcd[gci].gd.pos.y = gcd[gci-1].gd.pos.y+3;
	gcd[gci].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[gci].text = (unichar_t *) _("_Cancel");
	label[gci].text_is_1byte = true;
	label[gci].text_in_resource = true;
	gcd[gci].gd.label = &label[gci];
	gcd[gci].gd.mnemonic = 'C';
	gcd[gci].gd.handle_controlevent = Trans_Cancel;
	gcd[gci++].creator = GButtonCreate;
	buttons[8] = GCD_Glue; buttons[9] = &gcd[gci-1]; buttons[10] = GCD_Glue;
	buttons[11] = NULL;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = buttons;
	boxes[2].creator = GHBoxCreate;

	array[ai++] = &boxes[2]; array[ai++] = NULL; array[ai++] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = array;
	boxes[0].creator = GHVGroupCreate;

	GGadgetsCreate(gw,boxes);
	GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
	GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
	for ( i=0; i<TCnt; ++i ) {
	    GHVBoxSetPadding( GWidgetGetControl(gw,CID_ClockBox+i*TBlock_CIDOffset),0,0);
	    GHVBoxSetPadding( GWidgetGetControl(gw,CID_HVBox+i*TBlock_CIDOffset),0,0);
	    GHVBoxSetExpandableCol( GWidgetGetControl(gw,CID_HBox+i*TBlock_CIDOffset),gb_expandglue);
	}
	GGadgetSelectOneListItem( GWidgetGetControl(gw,CID_Type), 1);
	GWidgetToDesiredSize(gw);
    } else
	GDrawSetTransientFor(td.gw,(GWindow) -1);
    gw = td.gw;

    GGadgetSetEnabled( GWidgetGetControl(gw,CID_AllLayers), flags&tdf_enableback);
    GGadgetSetEnabled( GWidgetGetControl(gw,CID_DoGrid), flags&tdf_enableback);
    GGadgetSetEnabled( GWidgetGetControl(gw,CID_DoSimplePos), flags&tdf_enableback);
    GGadgetSetEnabled( GWidgetGetControl(gw,CID_DoKerns), flags&tdf_enablekerns);
    GGadgetSetVisible( GWidgetGetControl(gw,CID_Apply), flags&tdf_addapply);
    if ( !(flags&tdf_enableback) ) {
	GGadgetSetChecked( GWidgetGetControl(gw,CID_AllLayers), false );
	GGadgetSetChecked( GWidgetGetControl(gw,CID_DoGrid), false );
    }
    GGadgetSetChecked( GWidgetGetControl(gw,CID_DoKerns),
	    !(flags&tdf_enablekerns)?false:(flags&tdf_defaultkerns)?true:false );
    /* Yes, this is set differently from the previous, that's intended */
    GGadgetSetChecked( GWidgetGetControl(gw,CID_DoSimplePos),
	    !(flags&tdf_enableback)?false:(flags&tdf_enablekerns)?true:false );
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
	GGadget *firstoption = GWidgetGetControl(td.gw,CID_Type);
	GEvent dummy;
	GGadgetSelectOneListItem( firstoption, index );
	memset(&dummy,0,sizeof(dummy));
	dummy.type = et_controlevent; dummy.u.control.subtype = et_listselected;
	Trans_TypeChange( firstoption, &dummy );
    }

    for ( i=0; i<TCnt; ++i ) {
	int index = GGadgetGetFirstListSelectedItem(GWidgetGetControl(td.gw,CID_Type+i*TBlock_CIDOffset));
	if ( selcid[index]>0 ) {
	    GGadget *tf = GWidgetGetControl(td.gw,selcid[index]+i*TBlock_CIDOffset);
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
