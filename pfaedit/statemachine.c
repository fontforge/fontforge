/* Copyright (C) 2003 by George Williams */
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
#include <chardata.h>
#include <utype.h>
#include <ustring.h>
#include <gkeysym.h>

#define CID_New		300
#define CID_Edit	301
#define CID_Delete	302
#define CID_Up		303
#define CID_Down	304
#define CID_Classes	305

#define CID_FeatSet	306

#define CID_Ok		306
#define CID_Cancel	307

#define CID_Line1	308
#define CID_Line2	309
#define CID_Group	310

#define SMD_WIDTH	350
#define SMD_HEIGHT	400
#define SMD_CANCELDROP	32

extern int _GScrollBar_Width;

typedef struct statemachinedlg {
    GWindow gw;
    int state_cnt, class_cnt, index;
    struct asm_state *states;
    GGadget *hsb, *vsb;
    ASM *sm;
    SplineFont *sf;
    struct gfi_data *d;
    int isnew;
    GFont *font;
    int fh, as;
    int stateh, statew;
    int xstart, ystart;		/* This is where the headers start */
    int xstart2, ystart2;	/* This is where the data start */
    int width, height;
    int offleft, offtop;
    int canceldrop, sbdrop;
    GTextInfo *mactags;
} SMD;

static char *indicverbs[2][16] = {
    { "", "Ax", "xD", "AxD", "ABx", "ABx", "xCD", "xCD",
	"AxCD",  "AxCD", "ABxD", "ABxD", "ABxCD", "ABxCD", "ABxCD", "ABxCD" },
    { "", "xA", "Dx", "DxA", "xAB", "xBA", "CDx", "DCx",
	"CDxA",  "DCxA", "DxAB", "DxBA", "CDxAB", "CDxBA", "DCxAB", "DCxBA" }};

static struct asm_state *StateCopy(struct asm_state *old,int old_class_cnt,int old_state_cnt,
	int new_class_cnt,int new_state_cnt, enum asm_type type,int freeold) {
    struct asm_state *new = gcalloc(new_class_cnt*new_state_cnt,sizeof(struct asm_state));
    int i,j;
    int minclass = new_class_cnt<old_class_cnt ? new_class_cnt : old_class_cnt;

    for ( i=0; i<old_state_cnt && i<new_state_cnt; ++i ) {
	memcpy(new+i*new_class_cnt, old+i*old_class_cnt, minclass*sizeof(struct asm_state));
	if ( type==asm_insert )
	    for ( j=0; j<minclass; ++j ) {
		struct asm_state *this = &new[i*new_class_cnt+j];
		this->u.insert.mark_ins = copy(this->u.insert.mark_ins);
		this->u.insert.cur_ins = copy(this->u.insert.cur_ins);
	    }
    }
    if ( freeold ) {
	if ( type==asm_insert ) {
	    for ( i=0; i<old_state_cnt ; ++i ) {
		for ( j=0; j<old_class_cnt; ++j ) {
		    struct asm_state *this = &new[i*new_class_cnt+j];
		    free(this->u.insert.mark_ins);
		    free(this->u.insert.cur_ins);
		}
	    }
	}
	free(old);
    }
return( new );
}

static void _SMD_EnableButtons(SMD *smd) {
    GGadget *list = GWidgetGetControl(smd->gw,CID_Classes);
    int32 i, len;

    (void) GGadgetGetList(list,&len);
    i = GGadgetGetFirstListSelectedItem(list);
    GGadgetSetEnabled(GWidgetGetControl(smd->gw,CID_Up),i>4);
    GGadgetSetEnabled(GWidgetGetControl(smd->gw,CID_Down),i!=len-1 && i>4);
    GGadgetSetEnabled(GWidgetGetControl(smd->gw,CID_Delete),i>=4);
    GGadgetSetEnabled(GWidgetGetControl(smd->gw,CID_Edit),i>=4);
}

static int SMD_Up(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *list = GWidgetGetControl(smd->gw,CID_Classes);

	GListMoveSelected(list,-1);
	_SMD_EnableButtons(smd);
    }
return( true );
}

static int SMD_Down(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *list = GWidgetGetControl(smd->gw,CID_Classes);

	GListMoveSelected(list,1);
	_SMD_EnableButtons(smd);
    }
return( true );
}

static int SMD_Delete(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	GListDelSelected(GWidgetGetControl(smd->gw,CID_Classes));
	_SMD_EnableButtons(smd);
    }
return( true );
}

static void _SMD_DoEditNew(SMD *smd,int isedit) {
}

static int SMD_Edit(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	_SMD_DoEditNew(smd,true);
    }
return( true );
}

static int SMD_New(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	_SMD_DoEditNew(smd,false);
    }
return( true );
}

static int SMD_ClassSelected(GGadget *g, GEvent *e) {
    SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	_SMD_EnableButtons(smd);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	_SMD_DoEditNew(smd,true);
    }
return( true );
}

static void _SMD_Cancel(SMD *smd) {
    int i,j;

    GDrawDestroyWindow(smd->gw);

    if ( smd->isnew )
	GFI_FinishSMNew(smd->d,smd->sm,false);
    GFI_SMDEnd(smd->d);

    if ( smd->sm->type==asm_insert ) {
	for ( i=0; i<smd->state_cnt ; ++i ) {
	    for ( j=0; j<smd->class_cnt; ++j ) {
		struct asm_state *this = &smd->states[i*smd->class_cnt+j];
		free(this->u.insert.mark_ins);
		free(this->u.insert.cur_ins);
	    }
	}
    }
    free(smd->states);
    GTextInfoListFree(smd->mactags);
    free(smd);
}

static int SMD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	_SMD_Cancel(smd);
    }
return( true );
}

static int SMD_Ok(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	_SMD_Cancel(smd);
    }
return( true );
}

static void SMD_Expose(SMD *smd,GWindow pixmap,GRect *area) {
    GRect rect;
    GRect clip,old1,old2;
    int len, off, i, j, x, y;
    unichar_t ubuf[8];
    char buf[8];

    if ( area->y+area->width<smd->ystart )
return;
    if ( area->y>smd->ystart2+smd->height )
return;

    GDrawPushClip(pixmap,area,&old1);
    GDrawSetFont(pixmap,smd->font);
    rect.x = smd->xstart; rect.y = smd->ystart;
    rect.width = smd->width+(smd->xstart2-smd->xstart);
    rect.height = smd->height+(smd->ystart2-smd->ystart);
    clip = rect;
    GDrawPushClip(pixmap,&clip,&old2);
    for ( i=0 ; smd->offtop+i<=smd->state_cnt && (i-1)*smd->stateh<smd->height; ++i ) {
	GDrawDrawLine(pixmap,smd->xstart,smd->ystart2+i*smd->stateh,smd->xstart+rect.width,smd->ystart2+i*smd->stateh,
		0x808080);
	if ( i<smd->state_cnt ) {
	    sprintf( buf, i+smd->offtop<100 ? "St%d" : "%d", i+smd->offtop );
	    len = strlen( buf );
	    off = (smd->stateh-len*smd->fh)/2;
	    for ( j=0; j<len; ++j ) {
		ubuf[0] = buf[j];
		GDrawDrawText(pixmap,smd->xstart+3,smd->ystart2+i*smd->stateh+off+j*smd->fh+smd->as,
		    ubuf,1,NULL,0xff0000);
	    }
	}
    }
    for ( i=0 ; smd->offleft+i<=smd->class_cnt && (i-1)*smd->statew<smd->width; ++i ) {
	GDrawDrawLine(pixmap,smd->xstart2+i*smd->statew,smd->ystart,smd->xstart2+i*smd->statew,smd->ystart+rect.height,
		0x808080);
	if ( i<smd->class_cnt ) {
	    uc_strcpy(ubuf,"Class");
	    GDrawDrawText(pixmap,smd->xstart2+i*smd->statew+1,smd->ystart+smd->as+1,
		ubuf,-1,NULL,0xff0000);
	    sprintf( buf, "%d", i+smd->offleft );
	    uc_strcpy(ubuf,buf);
	    len = GDrawGetTextWidth(pixmap,ubuf,-1,NULL);
	    GDrawDrawText(pixmap,smd->xstart2+i*smd->statew+(smd->statew-len)/2,smd->ystart+smd->fh+smd->as+1,
		ubuf,-1,NULL,0xff0000);
	}
    }

    for ( i=0 ; smd->offtop+i<smd->state_cnt && (i-1)*smd->stateh<smd->height; ++i ) {
	y = smd->ystart2+i*smd->stateh;
	if ( y>area->y+area->height )
    break;
	if ( y+smd->stateh<area->y )
    continue;
	for ( j=0 ; smd->offleft+j<smd->class_cnt && (j-1)*smd->statew<smd->width; ++j ) {
	    struct asm_state *this = &smd->states[(i+smd->offtop)*smd->class_cnt+j+smd->offleft];
	    x = smd->xstart2+j*smd->statew;
	    if ( x>area->x+area->width )
	break;
	    if ( x+smd->statew<area->x )
	continue;

	    sprintf( buf, "%d", this->next_state );
	    uc_strcpy(ubuf,buf);
	    len = GDrawGetTextWidth(pixmap,ubuf,-1,NULL);
	    GDrawDrawText(pixmap,x+(smd->statew-len)/2,y+smd->as+1,
		ubuf,-1,NULL,0x000000);

	    ubuf[0] = (this->flags&0x8000)? 'M' : ' ';
	    ubuf[1] = (this->flags&0x4000)? ' ' : 'A';
	    ubuf[2] = '\0';
	    if ( smd->sm->type==asm_indic ) {
		ubuf[2] = (this->flags&0x2000) ? 'L' : ' ';
		ubuf[3] = '\0';
	    }
	    len = GDrawGetTextWidth(pixmap,ubuf,-1,NULL);
	    GDrawDrawText(pixmap,x+(smd->statew-len)/2,y+smd->fh+smd->as+1,
		ubuf,-1,NULL,0x000000);

	    if ( smd->sm->type==asm_indic ) {
		uc_strcpy(ubuf,indicverbs[0][this->flags&0xf]);
	    } else if ( smd->sm->type==asm_context ) {
		ubuf[0] = this->u.context.mark_tag>>24;
		ubuf[1] = (this->u.context.mark_tag>>16)&0xff;
		ubuf[2] = (this->u.context.mark_tag>>8)&0xff;
		ubuf[3] = this->u.context.mark_tag&0xff;
		ubuf[4] = 0;
	    } else {
		ubuf[0] = '\0';
		if ( this->u.insert.mark_ins!=NULL )
		    uc_strncpy(ubuf,this->u.insert.mark_ins,5);
	    }
	    len = GDrawGetTextWidth(pixmap,ubuf,-1,NULL);
	    GDrawDrawText(pixmap,x+(smd->statew-len)/2,y+2*smd->fh+smd->as+1,
		ubuf,-1,NULL,0x000000);

	    if ( smd->sm->type==asm_indic ) {
		uc_strcpy(ubuf,indicverbs[1][this->flags&0xf]);
	    } else if ( smd->sm->type==asm_context ) {
		ubuf[0] = this->u.context.cur_tag>>24;
		ubuf[1] = (this->u.context.cur_tag>>16)&0xff;
		ubuf[2] = (this->u.context.cur_tag>>8)&0xff;
		ubuf[3] = this->u.context.cur_tag&0xff;
		ubuf[4] = 0;
	    } else {
		ubuf[0] = '\0';
		if ( this->u.insert.cur_ins!=NULL )
		    uc_strncpy(ubuf,this->u.insert.cur_ins,5);
	    }
	    len = GDrawGetTextWidth(pixmap,ubuf,-1,NULL);
	    GDrawDrawText(pixmap,x+(smd->statew-len)/2,y+3*smd->fh+smd->as+1,
		ubuf,-1,NULL,0x000000);
	}
    }

    GDrawDrawLine(pixmap,smd->xstart,smd->ystart2,smd->xstart+rect.width,smd->ystart2,
	    0x000000);
    GDrawDrawLine(pixmap,smd->xstart2,smd->ystart,smd->xstart2,smd->ystart+rect.height,
	    0x000000);
    GDrawPopClip(pixmap,&old2);
    GDrawPopClip(pixmap,&old1);
    --rect.width; --rect.height;
    GDrawDrawRect(pixmap,&rect,0x000000);
}

static int smd_e_h(GWindow gw, GEvent *event) {
    SMD *smd = GDrawGetUserData(gw);

    if ( event->type==et_close ) {
	_SMD_Cancel(smd);
    } else if ( event->type==et_destroy ) {
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("statemachine.html");
return( true );
	} else if ( event->u.chr.keysym=='q' && (event->u.chr.state&ksm_control)) {
	    if ( event->u.chr.state&ksm_shift )
		_SMD_Cancel(GDrawGetUserData(gw));
	    else
		MenuExit(NULL,NULL,NULL);
return( true );
	}
return( false );
    } else if ( event->type==et_expose ) {
	SMD_Expose(smd,gw,&event->u.expose.rect);
return( false );
    } else if ( event->type==et_resize ) {
	int blen = GDrawPointsToPixels(NULL,GIntGetResource(_NUM_Buttonsize));
	GRect wsize, csize;

	GDrawGetSize(smd->gw,&wsize);
	GGadgetResize(GWidgetGetControl(smd->gw,CID_Group),wsize.width-4,wsize.height-4);
	GGadgetResize(GWidgetGetControl(smd->gw,CID_Line1),wsize.width-20,1);
	GGadgetResize(GWidgetGetControl(smd->gw,CID_Line2),wsize.width-20,1);

	GGadgetGetSize(GWidgetGetControl(smd->gw,CID_Ok),&csize);
	GGadgetMove(GWidgetGetControl(smd->gw,CID_Ok),csize.x,wsize.height-smd->canceldrop-3);
	GGadgetGetSize(GWidgetGetControl(smd->gw,CID_Cancel),&csize);
	GGadgetMove(GWidgetGetControl(smd->gw,CID_Cancel),wsize.width-blen-30,wsize.height-smd->canceldrop);

	GGadgetGetSize(GWidgetGetControl(smd->gw,CID_Classes),&csize);
	GGadgetResize(GWidgetGetControl(smd->gw,CID_Classes),wsize.width-GDrawPointsToPixels(NULL,10),csize.height);

	GGadgetGetSize(smd->hsb,&csize);
	smd->width = wsize.width-csize.height-smd->xstart2-5;
	GGadgetResize(smd->hsb,smd->width,csize.height);
	GGadgetMove(smd->hsb,smd->xstart2,wsize.height-smd->sbdrop-csize.height);
	GGadgetGetSize(smd->vsb,&csize);
	smd->height = wsize.height-smd->sbdrop-smd->ystart2-csize.width;
	GGadgetResize(smd->vsb,csize.width,wsize.height-smd->sbdrop-smd->ystart2-csize.width);
	GGadgetMove(smd->vsb,wsize.width-csize.width-5,smd->ystart2);

	GDrawRequestExpose(smd->gw,NULL,false);
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_textchanged ) {
	if ( event->u.control.u.tf_changed.from_pulldown!=-1 ) {
	    uint32 tag = (uint32) smd->mactags[event->u.control.u.tf_changed.from_pulldown].userdata;
	    unichar_t ubuf[20];
	    char buf[20];
	    /* If they select something from the pulldown, don't show the human */
	    /*  readable form, instead show the numeric feature/setting */
	    sprintf( buf,"<%d,%d>", tag>>16, tag&0xffff );
	    uc_strcpy(ubuf,buf);
	    GGadgetSetTitle(event->u.control.g,ubuf);
	}
#if 0
    } else if ( event->type == et_drop ) {
	smd_Drop(GDrawGetUserData(gw),event);
#endif
    }
return( true );
}

SMD *StateMachineEdit(SplineFont *sf,ASM *sm,struct gfi_data *d) {
    static int titles[2][3] = {
	{ _STR_EditIndic, _STR_EditContextSub, _STR_EditInsert },
	{ _STR_NewIndic, _STR_NewContextSub, _STR_NewInsert }};
    SMD *smd = gcalloc(1,sizeof(SMD));
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[20];
    GTextInfo label[20];
    int k, vk;
    int blen = GIntGetResource(_NUM_Buttonsize);
    const int space = 7;
    static unichar_t courier[] = { 'c', 'o', 'u', 'r', 'i', 'e', 'r', ',', 'm','o','n','o','s','p','a','c','e',',','c','l','e','a','r','l','y','u',',', 'u','n','i','f','o','n','t', '\0' };
    int as, ds, ld, sbsize;
    FontRequest rq;
    static unichar_t statew[] = { '1', '2', '3', '4', '5', 0 };
    char buf[20];
    unichar_t ubuf[20];

    smd->sf = sf;
    smd->sm = sm;
    smd->d = d;
    smd->isnew = (sm->class_cnt==0);
    if ( smd->isnew ) {
	smd->class_cnt = 4;		/* 4 built in types */
	smd->state_cnt = 2;		/* 2 built in states */
	smd->states = gcalloc(smd->class_cnt*smd->state_cnt,sizeof(struct asm_state));
    } else {
	smd->class_cnt = sm->class_cnt;
	smd->state_cnt = sm->state_cnt;
	smd->states = StateCopy(sm->state,sm->class_cnt,sm->state_cnt,
		smd->class_cnt,smd->state_cnt,sm->type,false);
    }
    smd->index = sm->type==asm_indic ? 0 : sm->type==asm_context ? 1 : 2;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = false;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(titles[smd->isnew][smd->index],NULL);
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(SMD_WIDTH));
    pos.height = GDrawPointsToPixels(NULL,SMD_HEIGHT);
    smd->gw = gw = GDrawCreateTopWindow(NULL,&pos,smd_e_h,smd,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    k = 0;

    label[k].text = (unichar_t *) _STR_FeatureSetting;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 5;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;

    if ( !smd->isnew ) {
	sprintf(buf,"<%d,%d>", sm->feature, sm->setting );
	uc_strcpy(ubuf,buf);
	label[k].text = ubuf;
	gcd[k].gd.label = &label[k];
    }
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.u.list = smd->mactags = AddMacFeatures(NULL,pst_max,sf);
    gcd[k].gd.cid = CID_FeatSet;
    gcd[k++].creator = GListFieldCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = GDrawPointsToPixels(gw,gcd[k-1].gd.pos.y+27);
    gcd[k].gd.pos.width = pos.width-20;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k].gd.cid = CID_Line1;
    gcd[k++].creator = GLineCreate;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = GDrawPixelsToPoints(gw,gcd[k-1].gd.pos.y)+5;
    gcd[k].gd.pos.width = SMD_WIDTH-10;
    gcd[k].gd.pos.height = 8*12+10;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.handle_controlevent = SMD_ClassSelected;
    gcd[k].gd.cid = CID_Classes;
    gcd[k++].creator = GListCreate;

    label[k].text = (unichar_t *) _STR_New;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+gcd[k-1].gd.pos.height+10;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.handle_controlevent = SMD_New;
    gcd[k].gd.cid = CID_New;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _STR_Edit;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5+blen+space; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = SMD_Edit;
    gcd[k].gd.cid = CID_Edit;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _STR_Delete;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+blen+space; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = SMD_Delete;
    gcd[k].gd.cid = CID_Delete;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _STR_Up;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+blen+space+5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = SMD_Up;
    gcd[k].gd.cid = CID_Up;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _STR_Down;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+blen+space; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = SMD_Down;
    gcd[k].gd.cid = CID_Down;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = GDrawPointsToPixels(gw,gcd[k-1].gd.pos.y+28);
    gcd[k].gd.pos.width = pos.width-20;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k].gd.cid = CID_Line2;
    gcd[k++].creator = GLineCreate;

    smd->canceldrop = GDrawPointsToPixels(gw,SMD_CANCELDROP);
    smd->sbdrop = smd->canceldrop+GDrawPointsToPixels(gw,7);

    vk = k;
    gcd[k].gd.pos.width = sbsize = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gcd[k].gd.pos.x = pos.width-sbsize;
    gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+8;
    gcd[k].gd.pos.height = pos.height-gcd[k].gd.pos.y-sbsize-smd->sbdrop;
    gcd[k].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    gcd[k++].creator = GScrollBarCreate;
    smd->height = gcd[k-1].gd.pos.height;
    smd->ystart = gcd[k-1].gd.pos.y;

    gcd[k].gd.pos.height = sbsize;
    gcd[k].gd.pos.y = pos.height-sbsize-8;
    gcd[k].gd.pos.x = 4;
    gcd[k].gd.pos.width = pos.width-sbsize;
    gcd[k].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    gcd[k++].creator = GScrollBarCreate;
    smd->width = gcd[k-1].gd.pos.width;
    smd->xstart = 5;

    label[k].text = (unichar_t *) _STR_OK;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = SMD_HEIGHT-SMD_CANCELDROP-3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_default;
    gcd[k].gd.handle_controlevent = SMD_Ok;
    gcd[k].gd.cid = CID_Ok;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _STR_Cancel;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k].gd.pos.y+3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_cancel;
    gcd[k].gd.handle_controlevent = SMD_Cancel;
    gcd[k].gd.cid = CID_Cancel;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width-4;
    gcd[k].gd.pos.height = pos.height-4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k].gd.cid = CID_Group;
    gcd[k++].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    smd->vsb = gcd[vk].ret;
    smd->hsb = gcd[vk+1].ret;

    {
	GGadget *list = GWidgetGetControl(smd->gw,CID_Classes);
	GListAppendLine(list,GStringGetResource(_STR_EndOfText,NULL),false);
	GListAppendLine(list,GStringGetResource(_STR_EverythingElse,NULL),false);
	GListAppendLine(list,GStringGetResource(_STR_DeletedGlyph,NULL),false);
	GListAppendLine(list,GStringGetResource(_STR_EndOfLine,NULL),false);
	for ( k=4; k<sm->class_cnt; ++k ) {
	    unichar_t *temp = uc_copy(sm->classes[k]);
	    GListAppendLine(list,temp,false);
	    free(temp);
	}
    }

    memset(&rq,'\0',sizeof(rq));
    rq.point_size = 12;
    rq.weight = 400;
    rq.family_name = courier;
    smd->font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawFontMetrics(smd->font,&as,&ds,&ld);
    smd->fh = as+ds; smd->as = as;
    GDrawSetFont(gw,smd->font);

    smd->stateh = 4*smd->fh+3;
    smd->statew = GDrawGetTextWidth(gw,statew,-1,NULL)+3;
    smd->xstart2 = smd->xstart+smd->statew/2;
    smd->ystart2 = smd->ystart+2*smd->fh+1;

    GDrawSetVisible(gw,true);

return( smd );
}

void SMD_Close(SMD *smd) {
    _SMD_Cancel(smd);
}
