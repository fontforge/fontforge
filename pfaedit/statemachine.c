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

#define CID_Ok		307
#define CID_Cancel	308

#define CID_Line1	309
#define CID_Line2	310
#define CID_Group	311
#define CID_Group2	312

#define CID_Set		313
#define CID_Select	314
#define CID_GlyphList	315
#define CID_Next	316
#define CID_Prev	317

#define CID_RightToLeft	318
#define CID_VertOnly	319

#define SMD_WIDTH	350
#define SMD_HEIGHT	400
#define SMD_CANCELDROP	32

#define CID_NextState	400
#define CID_Flag4000	401
#define CID_Flag8000	402
#define CID_Flag2000	403
#define CID_Flag1000	404
#define CID_Flag0800	405
#define CID_Flag0400	406
#define CID_IndicVerb	407
#define CID_InsCur	408
#define CID_InsMark	409
#define CID_TagCur	410
#define CID_TagMark	411

#define SMDE_WIDTH	200
#define SMDE_HEIGHT	220

extern int _GScrollBar_Width;

typedef struct statemachinedlg {
    GWindow gw, cw, editgw;
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
    int isedit;
    int st_pos;
    int edit_done, edit_ok;
} SMD;

static int SMD_SBReset(SMD *smd);

static char *indicverbs[2][16] = {
    { "", "Ax", "xD", "AxD", "ABx", "ABx", "xCD", "xCD",
	"AxCD",  "AxCD", "ABxD", "ABxD", "ABxCD", "ABxCD", "ABxCD", "ABxCD" },
    { "", "xA", "Dx", "DxA", "xAB", "xBA", "CDx", "DCx",
	"CDxA",  "DCxA", "DxAB", "DxBA", "CDxAB", "CDxBA", "DCxAB", "DCxBA" }};

static void StatesFree(struct asm_state *old,int old_class_cnt,int old_state_cnt,
	enum asm_type type) {
    int i, j;

    if ( type==asm_insert ) {
	for ( i=0; i<old_state_cnt ; ++i ) {
	    for ( j=0; j<old_class_cnt; ++j ) {
		struct asm_state *this = &old[i*old_class_cnt+j];
		free(this->u.insert.mark_ins);
		free(this->u.insert.cur_ins);
	    }
	}
    }
    free(old);
}

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
    for ( ; i<new_state_cnt; ++i )
	new[i*new_class_cnt+2].next_state = i;		/* Deleted glyphs should be treated as noops */

    if ( freeold )
	StatesFree(old,old_class_cnt,old_state_cnt,type);

return( new );
}

static void StateRemoveClasses(SMD *smd, GTextInfo **removethese ) {
    struct asm_state *new;
    int i,j,k, remove_cnt;

    for ( remove_cnt=i=0; i<smd->class_cnt; ++i )
	if ( removethese[i]->selected )
	    ++remove_cnt;
    if ( remove_cnt==0 )
return;

    new = gcalloc((smd->class_cnt-remove_cnt)*smd->state_cnt,sizeof(struct asm_state));
    for ( i=0; i<smd->state_cnt ; ++i ) {
	for ( j=k=0; j<smd->class_cnt; ++j ) {
	    if ( !removethese[j]->selected )
		new[i*(smd->class_cnt-remove_cnt)+k++] = smd->states[i*smd->class_cnt+j];
	    else if ( smd->sm->type==asm_insert ) {
		free(smd->states[i*smd->class_cnt+j].u.insert.mark_ins);
		free(smd->states[i*smd->class_cnt+j].u.insert.cur_ins);
	    }
	}
    }

    free(smd->states);
    smd->states = new;
    smd->class_cnt -= remove_cnt;
}

static int FindMaxReachableStateCnt(SMD *smd) {
    int max_reachable = 1;
    int i, j, ns;

    for ( i=0; i<=max_reachable && i<smd->state_cnt; ++i ) {
	for ( j=0; j<smd->class_cnt; ++j ) {
	    ns = smd->states[i*smd->class_cnt+j].next_state;
	    if ( ns>max_reachable )
		max_reachable = ns;
	}
    }
return( max_reachable+1 );		/* The count is one more than the max */
}

/* ************************************************************************** */
/* ****************************** Edit a State ****************************** */
/* ************************************************************************** */
GTextInfo indicverbs_list[] = {
    { (unichar_t *) _STR_NoChange, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "Ax => xA", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "xD => Dx", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "AxD => DxA", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "ABx => xAB", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "ABx => xBA", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "xCD => CDx", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "xCD => DCx", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "AxCD => CDxA", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "AxCD => DCxA", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "ABxD => DxAB", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "ABxD => DxBA", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "ABxCD => CDxAB", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "ABxCD => CDxBA", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "ABxCD => DCxAB", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "ABxCD => DCxBA", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }
};

static char *copy_count(GWindow gw,int cid,int *cnt) {
    const unichar_t *u = _GGadgetGetTitle(GWidgetGetControl(gw,cid)), *upt;
    char *ret, *pt;
    int c;

    while ( *u==' ' ) ++u;
    if ( *u=='\0' ) {
	*cnt = 0;
return( NULL );
    }

    ret = pt = galloc(u_strlen(u)+1);
    c = 0;
    for ( upt=u; *upt; ) {
	if ( *upt==' ' ) {
	    while ( *upt==' ' ) ++upt;
	    if ( *upt!='\0' ) {
		++c;
		*pt++ = ' ';
	    }
	} else
	    *pt++ = *upt++;
    }
    *pt = '\0';
    *cnt = c+1;
return( ret );
}

static int smdedit_e_h(GWindow gw, GEvent *event) {
    SMD *smd = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_close:
	smd->edit_done = true;
	smd->edit_ok = false;
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("statemachine.html#EditTransition");
return( true );
	} else if ( event->u.chr.keysym == GK_Escape ) {
	    smd->edit_done = true;
return( true );
	} else if ( event->u.chr.chars[0] == '\r' ) {
	    smd->edit_done = true;
	    smd->edit_ok = true;
return( true );
	}
return( false );
      case et_controlevent:
	switch( event->u.control.subtype ) {
	  case et_buttonactivate:
	    smd->edit_done = true;
	    smd->edit_ok = GGadgetGetCid(event->u.control.g)==CID_Ok;
	  break;
	}
      break;
    }
return( true );
}

static void SMD_EditState(SMD *smd) {
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[20];
    GTextInfo label[20];
    GRect pos;
    int j, k, listk;
    struct asm_state *this = &smd->states[smd->st_pos];
    char buf[20];
    unichar_t utag1[8], utag2[8];

    smd->edit_done = smd->edit_ok = false;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = true;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(_STR_EditStateTransition,NULL);
    pos.x = pos.y = 0;
    pos.width = GDrawPointsToPixels(NULL,GGadgetScale(SMDE_WIDTH));
    pos.height = GDrawPointsToPixels(NULL,SMDE_HEIGHT);
    smd->editgw = gw = GDrawCreateTopWindow(NULL,&pos,smdedit_e_h,smd,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    k = 0; listk = -1;

    label[k].text = (unichar_t *) _STR_NextState;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 5+4;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;

    sprintf(buf,"%d", this->next_state );
    label[k].text = (unichar_t *) buf;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_NextState;
    gcd[k++].creator = GTextFieldCreate;

    label[k].text = (unichar_t *) _STR_AdvanceToNextGlyph;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+24;
    gcd[k].gd.flags = gg_enabled|gg_visible | (this->flags&0x4000?0:gg_cb_on);
    gcd[k].gd.cid = CID_Flag4000;
    gcd[k++].creator = GCheckBoxCreate;

    label[k].text = (unichar_t *) (smd->sm->type!=asm_indic?_STR_MarkCurrentGlyph:_STR_MarkCurrentGlyphAsFirst);
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
    gcd[k].gd.flags = gg_enabled|gg_visible | (this->flags&0x8000?gg_cb_on:0);
    gcd[k].gd.cid = CID_Flag8000;
    gcd[k++].creator = GCheckBoxCreate;

    if ( smd->sm->type==asm_indic ) {
	label[k].text = (unichar_t *) _STR_MarkCurrentGlyphAsLast;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
	gcd[k].gd.flags = gg_enabled|gg_visible | (this->flags&0x2000?gg_cb_on:0);
	gcd[k].gd.cid = CID_Flag2000;
	gcd[k++].creator = GCheckBoxCreate;

	for ( j=0; j<16; ++j ) indicverbs_list[j].selected = false;
	indicverbs_list[this->flags&0xf].selected = true;
	gcd[k].gd.label = &indicverbs_list[this->flags&0xf];
	gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+24;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.u.list = indicverbs_list;
	gcd[k].gd.cid = CID_IndicVerb;
	gcd[k++].creator = GListButtonCreate;
    } else if ( smd->sm->type==asm_insert ) {
	label[k].text = (unichar_t *) _STR_CurrentGlyphIsKashida;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
	gcd[k].gd.flags = gg_enabled|gg_visible | (this->flags&0x2000?gg_cb_on:0);
	gcd[k].gd.cid = CID_Flag2000;
	gcd[k++].creator = GCheckBoxCreate;

	label[k].text = (unichar_t *) _STR_MarkedGlyphIsKashida;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
	gcd[k].gd.flags = gg_enabled|gg_visible | (this->flags&0x1000?gg_cb_on:0);
	gcd[k].gd.cid = CID_Flag1000;
	gcd[k++].creator = GCheckBoxCreate;

	label[k].text = (unichar_t *) _STR_InsertBeforeCurrentGlyph;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
	gcd[k].gd.flags = gg_enabled|gg_visible | (this->flags&0x0800?gg_cb_on:0);
	gcd[k].gd.cid = CID_Flag0800;
	gcd[k++].creator = GCheckBoxCreate;

	label[k].text = (unichar_t *) _STR_InsertBeforeMarkedGlyph;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
	gcd[k].gd.flags = gg_enabled|gg_visible | (this->flags&0x0400?gg_cb_on:0);
	gcd[k].gd.cid = CID_Flag0400;
	gcd[k++].creator = GCheckBoxCreate;

	label[k].text = (unichar_t *) _STR_MarkInsert;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+26;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	label[k].text = (unichar_t *) (this->u.insert.mark_ins);
	label[k].text_is_1byte = true;
	gcd[k].gd.label = this->u.insert.mark_ins==NULL ? NULL : &label[k];
	gcd[k].gd.pos.x = gcd[1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_InsMark;
	gcd[k++].creator = GTextFieldCreate;

	label[k].text = (unichar_t *) _STR_CurrentInsert;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	label[k].text = (unichar_t *) (this->u.insert.cur_ins);
	label[k].text_is_1byte = true;
	gcd[k].gd.label = this->u.insert.cur_ins==NULL ? NULL : &label[k];
	gcd[k].gd.pos.x = gcd[1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_InsCur;
	gcd[k++].creator = GTextFieldCreate;
    } else {
	label[k].text = (unichar_t *) _STR_MarkSubs;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+26;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	utag1[0] = this->u.context.mark_tag>>24; utag1[1] = (this->u.context.mark_tag>>16)&0xff;
	utag1[2] = (this->u.context.mark_tag>>8)&0xff; utag1[3] = this->u.context.mark_tag&0xff;
	utag1[4] = 0;
	label[k].text = utag1;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	listk = k;
	gcd[k].gd.cid = CID_TagMark;
	gcd[k++].creator = GListFieldCreate;

	label[k].text = (unichar_t *) _STR_CurrentSubs;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	utag2[0] = this->u.context.cur_tag>>24; utag2[1] = (this->u.context.cur_tag>>16)&0xff;
	utag2[2] = (this->u.context.cur_tag>>8)&0xff; utag2[3] = this->u.context.cur_tag&0xff;
	utag2[4] = 0;
	label[k].text = utag2;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_TagCur;
	gcd[k].gd.u.list = gcd[k-2].gd.u.list;
	gcd[k++].creator = GListFieldCreate;
    }

    label[k].text = (unichar_t *) _STR_OK;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = SMDE_HEIGHT-SMD_CANCELDROP-3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_default;
    gcd[k].gd.cid = CID_Ok;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _STR_Cancel;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_cancel;
    gcd[k].gd.cid = CID_Cancel;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width-4;
    gcd[k].gd.pos.height = pos.height-4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k++].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);

    if ( listk!=-1 ) {
	GGadgetSetList(gcd[listk].ret,
		SFGenTagListFromType(&smd->sf->gentags,pst_substitution),false);
	GGadgetSetList(gcd[listk+2].ret,
		SFGenTagListFromType(&smd->sf->gentags,pst_substitution),false);
    }

    GDrawSetVisible(gw,true);
 retry:
    while ( !smd->edit_done )
	GDrawProcessOneEvent(NULL);

    if ( smd->edit_ok ) {
	int new_cnt, err=false, ns, flags, cnt;
	char *mins, *cins;
	uint32 mtag, ctag;
	const unichar_t *ret;

	smd->edit_ok = smd->edit_done = false;

	ns = GetIntR(gw,CID_NextState,_STR_NextState,&err);
	if ( err )
 goto retry;
	flags = 0;
	if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_Flag4000)) ) flags |= 0x4000;
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Flag8000)) ) flags |= 0x8000;
	if ( smd->sm->type==asm_indic ) {
	    if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Flag2000)) ) flags |= 0x2000;
	    flags |= GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,CID_IndicVerb));
	    this->next_state = ns;
	    this->flags = flags;
	} else if ( smd->sm->type==asm_context ) {
	    mtag = ctag = 0;
	    ret = _GGadgetGetTitle(GWidgetGetControl(gw,CID_TagMark));
	    if ( *ret=='\0' )
		/* That's ok */;
	    else if ( u_strlen(ret)!=4 ) {
		GWidgetErrorR(_STR_TagMustBe4,_STR_TagMustBe4);
 goto retry;
	    } else
		mtag = (ret[0]<<24)|(ret[1]<<16)|(ret[2]<<8)|ret[3];
	    ret = _GGadgetGetTitle(GWidgetGetControl(gw,CID_TagCur));
	    if ( *ret=='\0' )
		/* That's ok */;
	    else if ( u_strlen(ret)!=4 ) {
		GWidgetErrorR(_STR_TagMustBe4,_STR_TagMustBe4);
 goto retry;
	    } else
		ctag = (ret[0]<<24)|(ret[1]<<16)|(ret[2]<<8)|ret[3];
	    this->next_state = ns;
	    this->flags = flags;
	    this->u.context.mark_tag = mtag;
	    this->u.context.cur_tag = ctag;
	} else {
	    if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Flag2000)) ) flags |= 0x2000;
	    if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Flag1000)) ) flags |= 0x1000;
	    if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Flag0800)) ) flags |= 0x0800;
	    if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Flag0400)) ) flags |= 0x0400;
	    mins = copy_count(gw,CID_InsMark,&cnt);
	    if ( cnt>31 ) {
		GWidgetErrorR(_STR_TooManyGlyphs,_STR_AtMost31Glyphs);
		free(mins);
 goto retry;
	    }
	    flags |= cnt<<5;
	    cins = copy_count(gw,CID_InsCur,&cnt);
	    if ( cnt>31 ) {
		GWidgetErrorR(_STR_TooManyGlyphs,_STR_AtMost31Glyphs);
		free(mins);
		free(cins);
 goto retry;
	    }
	    flags |= cnt;
	    this->next_state = ns;
	    this->flags = flags;
	    free(this->u.insert.mark_ins);
	    free(this->u.insert.cur_ins);
	    this->u.insert.mark_ins = mins;
	    this->u.insert.cur_ins = cins;
	}

	new_cnt = FindMaxReachableStateCnt(smd);
	if ( new_cnt!=smd->state_cnt ) {
	    smd->states = StateCopy(smd->states,smd->class_cnt,smd->state_cnt,
		    smd->class_cnt,new_cnt,
		    smd->sm->type,true);
	    smd->state_cnt = new_cnt;
	    SMD_SBReset(smd);
	    GDrawRequestExpose(smd->gw,NULL,false);
	}
    }
    smd->st_pos = -1;
    GDrawDestroyWindow(gw);
return;
}

/* ************************************************************************** */
/* **************************** Edit/Add a Class **************************** */
/* ************************************************************************** */

static int SMD_ToSelection(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(smd->gw,CID_GlyphList));
	SplineFont *sf = smd->sf;
	FontView *fv = sf->fv;
	const unichar_t *end;
	int pos, found=-1;
	char *nm;

	GDrawSetVisible(fv->gw,true);
	GDrawRaise(fv->gw);
	memset(fv->selected,0,sf->charcnt);
	while ( *ret ) {
	    end = u_strchr(ret,' ');
	    if ( end==NULL ) end = ret+u_strlen(ret);
	    nm = cu_copybetween(ret,end);
	    for ( ret = end; isspace(*ret); ++ret);
	    if (( pos = SFFindChar(sf,-1,nm))!=-1 ) {
		if ( found==-1 ) found = pos;
		fv->selected[pos] = true;
	    }
	    free(nm);
	}

	if ( found!=-1 )
	    FVScrollToChar(fv,found);
	GDrawRequestExpose(fv->v,NULL,false);
    }
return( true );
}

static int SMD_FromSelection(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	SplineFont *sf = smd->sf;
	FontView *fv = sf->fv;
	unichar_t *vals, *pt;
	int i, len, max;
	SplineChar *sc;
    
	for ( i=len=max=0; i<sf->charcnt; ++i ) if ( fv->selected[i]) {
	    sc = SFMakeChar(sf,i);
	    len += strlen(sc->name)+1;
	    if ( fv->selected[i]>max ) max = fv->selected[i];
	}
	pt = vals = galloc((len+1)*sizeof(unichar_t));
	*pt = '\0';
	/* in a class the order of selection is irrelevant */
	for ( i=0; i<sf->charcnt; ++i ) if ( fv->selected[i] ) {
	    uc_strcpy(pt,sf->chars[i]->name);
	    pt += u_strlen(pt);
	    *pt++ = ' ';
	}
	if ( pt>vals ) pt[-1]='\0';
    
	GGadgetSetTitle(GWidgetGetControl(smd->gw,CID_GlyphList),vals);
    }
return( true );
}

static int SMD_Prev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	GDrawSetVisible(smd->cw,false);
    }
return( true );
}

static int SMD_Next(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(smd->gw,CID_GlyphList));
	GGadget *list = GWidgetGetControl( smd->gw, CID_Classes );

	if ( CCD_NameListCheck(smd->sf,ret,true,_STR_BadClass) ||
		CCD_InvalidClassList(ret,list,smd->isedit))
return( true );

	if ( smd->isedit ) {
	    GListChangeLine(list,GGadgetGetFirstListSelectedItem(list),ret);
	} else {
	    GListAppendLine(list,ret,false);
	    smd->states = StateCopy(smd->states,smd->class_cnt,smd->state_cnt,
		    smd->class_cnt+1,smd->state_cnt,
		    smd->sm->type,true);
	    ++smd->class_cnt;
	    SMD_SBReset(smd);
	}
	GDrawSetVisible(smd->cw,false);		/* This will give us an expose so we needed ask for one */
    }
return( true );
}

static void _SMD_DoEditNew(SMD *smd,int isedit) {
    static unichar_t nullstr[] = { 0 };

    smd->isedit = isedit;
    if ( isedit ) {
	GTextInfo *selected = GGadgetGetListItemSelected(GWidgetGetControl(
		smd->gw, CID_Classes));
	if ( selected==NULL )
return;
	GGadgetSetTitle(GWidgetGetControl(smd->cw,CID_GlyphList),selected->text);
    } else {
	GGadgetSetTitle(GWidgetGetControl(smd->cw,CID_GlyphList),nullstr);
    }
    GDrawSetVisible(smd->cw,true);
}

/* ************************************************************************** */
/* ****************************** Main Dialog ******************************* */
/* ************************************************************************** */

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
	GGadget *list = GWidgetGetControl(smd->gw,CID_Classes);
	int32 len;

	StateRemoveClasses(smd,GGadgetGetList(list,&len));
	GListDelSelected(list);
	_SMD_EnableButtons(smd);
	SMD_SBReset(smd);
	GDrawRequestExpose(smd->gw,NULL,false);
    }
return( true );
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

static void _SMD_Finish(SMD *smd, int success) {

    GDrawDestroyWindow(smd->gw);

    if ( smd->isnew )
	GFI_FinishSMNew(smd->d,smd->sm,success);
    GFI_SMDEnd(smd->d);

    GTextInfoListFree(smd->mactags);
    free(smd);
}

static void _SMD_Cancel(SMD *smd) {

    StatesFree(smd->states,smd->state_cnt,smd->class_cnt,smd->sm->type);
    _SMD_Finish(smd,false);
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
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(smd->gw,CID_FeatSet));
	unichar_t *end;
	int feat, set, i;
	int32 len;
	GTextInfo **ti = GGadgetGetList(GWidgetGetControl(smd->gw,CID_Classes),&len);
	ASM *sm = smd->sm;

	if ( *ret=='<' )
	    ++ret;
	feat = u_strtol(ret,&end,10);
	if ( *end==',' && ( set = u_strtol(end+1,&end,10), *end=='>' || *end=='\0')) {
	    sm->feature = feat;
	    sm->setting = set;
	} else {
	    GWidgetErrorR(_STR_BadFeatureSetting,_STR_BadFeatureSetting);
return( true );
	}
	
	for ( i=4; i<sm->class_cnt; ++i )
	    free(sm->classes[i]);
	free(sm->classes);
	sm->classes = galloc(smd->class_cnt*sizeof(char *));
	sm->classes[0] = sm->classes[1] = sm->classes[2] = sm->classes[3] = NULL;
	sm->class_cnt = smd->class_cnt;
	for ( i=4; i<sm->class_cnt; ++i )
	    sm->classes[i] = cu_copy(ti[i]->text);

	StatesFree(sm->state,sm->state_cnt,sm->class_cnt,
		sm->type);
	sm->state_cnt = smd->state_cnt;
	sm->state = smd->states;
	sm->flags = (sm->flags & ~0xc000) |
		(GGadgetIsChecked(GWidgetGetControl(smd->gw,CID_RightToLeft))?0x4000:0) |
		(GGadgetIsChecked(GWidgetGetControl(smd->gw,CID_VertOnly))?0x8000:0);
	_SMD_Finish(smd,true);
    }
return( true );
}

static void SMD_Mouse(SMD *smd,GEvent *event) {
    static unichar_t space[100];
    char buf[30];
    int32 len;
    GTextInfo **ti;
    int pos = (event->u.mouse.y-smd->ystart2)/smd->stateh * smd->class_cnt +
	    (event->u.mouse.x-smd->xstart2)/smd->statew;

    GGadgetEndPopup();

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button==4 || event->u.mouse.button==5) ) {
	GGadgetDispatchEvent(smd->vsb,event);
return;
    }
    
    if ( event->u.mouse.x<smd->xstart || event->u.mouse.x>smd->xstart2+smd->width ||
	    event->u.mouse.y<smd->ystart || event->u.mouse.y>smd->ystart2+smd->height )
return;

    if ( event->type==et_mousemove ) {
	int c = (event->u.mouse.x - smd->xstart2)/smd->statew + smd->offleft;
	int s = (event->u.mouse.y - smd->ystart2)/smd->stateh + smd->offtop;
	space[0] = '\0';
	if ( event->u.mouse.y>=smd->ystart2 && s<smd->state_cnt ) {
	    sprintf( buf, "State %d\n", s );
	    uc_strcpy(space,buf);
	}
	if ( event->u.mouse.x>=smd->xstart2 && c<smd->class_cnt ) {
	    sprintf( buf, "Class %d\n", c );
	    uc_strcat(space,buf);
	    ti = GGadgetGetList(GWidgetGetControl(smd->gw,CID_Classes),&len);
	    len = u_strlen(space);
	    u_strncpy(space+len,ti[c]->text,(sizeof(space)/sizeof(space[0]))-1 - len);
	} else if ( event->u.mouse.x<smd->xstart2 ) {
	    if ( s==0 )
		u_strcat(space,GStringGetResource(_STR_StartOfInput,NULL));
	    else if ( s==1 )
		u_strcat(space,GStringGetResource(_STR_StartOfLine,NULL));
	}
	if ( space[0]=='\0' )
return;
	if ( space[u_strlen(space)-1]=='\n' )
	    space[u_strlen(space)-1]='\0';
	GGadgetPreparePopup(smd->gw,space);
    } else if ( event->u.mouse.x<smd->xstart2 || event->u.mouse.y<smd->ystart2 )
return;
    else if ( event->type==et_mousedown )
	smd->st_pos = pos;
    else if ( event->type==et_mouseup ) {
	if ( pos==smd->st_pos )
	    SMD_EditState(smd);
    }
}

static void SMD_Expose(SMD *smd,GWindow pixmap,GEvent *event) {
    GRect *area = &event->u.expose.rect;
    GRect rect;
    GRect clip,old1,old2;
    int len, off, i, j, x, y;
    unichar_t ubuf[8];
    char buf[8];

    if ( area->y+area->height<smd->ystart )
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
	if ( i+smd->offtop<smd->state_cnt ) {
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
	if ( i+smd->offleft<smd->class_cnt ) {
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
    GDrawDrawRect(pixmap,&rect,0x000000);
    rect.y += rect.height;
    rect.x += rect.width;
    LogoExpose(pixmap,event,&rect,dm_fore);
}

static int SMD_SBReset(SMD *smd) {
    int oldtop = smd->offtop, oldleft = smd->offleft;

    GScrollBarSetBounds(smd->vsb,0,smd->state_cnt, smd->height/smd->stateh);
    GScrollBarSetBounds(smd->hsb,0,smd->class_cnt, smd->width/smd->statew);
    if ( smd->offtop + (smd->height/smd->stateh) >= smd->state_cnt )
	smd->offtop = smd->state_cnt - (smd->height/smd->stateh);
    if ( smd->offtop < 0 ) smd->offtop = 0;
    if ( smd->offleft + (smd->width/smd->statew) >= smd->class_cnt )
	smd->offleft = smd->class_cnt - (smd->width/smd->statew);
    if ( smd->offleft < 0 ) smd->offleft = 0;
    GScrollBarSetPos(smd->vsb,smd->offtop);
    GScrollBarSetPos(smd->hsb,smd->offleft);

return( oldtop!=smd->offtop || oldleft!=smd->offleft );
}

static void SMD_HScroll(SMD *smd,struct sbevent *sb) {
    int newpos = smd->offleft;
    GRect rect;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
	if ( smd->width/smd->statew == 1 )
	    --newpos;
	else
	    newpos -= smd->width/smd->statew - 1;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
	if ( smd->width/smd->statew == 1 )
	    ++newpos;
	else
	    newpos += smd->width/smd->statew - 1;
      break;
      case et_sb_bottom:
        newpos = smd->class_cnt - (smd->width/smd->statew);
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos + (smd->width/smd->statew) >= smd->class_cnt )
	newpos = smd->class_cnt - (smd->width/smd->statew);
    if ( newpos < 0 ) newpos = 0;
    if ( newpos!=smd->offleft ) {
	int diff = newpos-smd->offleft;
	smd->offleft = newpos;
	GScrollBarSetPos(smd->hsb,newpos);
	rect.x = smd->xstart2+1; rect.y = smd->ystart;
	rect.width = smd->width-1;
	rect.height = smd->height+(smd->ystart2-smd->ystart);
	GDrawScroll(smd->gw,&rect,-diff*smd->statew,0);
    }
}

static void SMD_VScroll(SMD *smd,struct sbevent *sb) {
    int newpos = smd->offtop;
    GRect rect;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
	if ( smd->height/smd->stateh == 1 )
	    --newpos;
	else
	    newpos -= smd->height/smd->stateh - 1;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
	if ( smd->height/smd->stateh == 1 )
	    ++newpos;
	else
	    newpos += smd->height/smd->stateh - 1;
      break;
      case et_sb_bottom:
        newpos = smd->state_cnt - (smd->height/smd->stateh);
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos + (smd->height/smd->stateh) >= smd->state_cnt )
	newpos = smd->state_cnt - (smd->height/smd->stateh);
    if ( newpos < 0 ) newpos = 0;
    if ( newpos!=smd->offtop ) {
	int diff = newpos-smd->offtop;
	smd->offtop = newpos;
	GScrollBarSetPos(smd->vsb,newpos);
	rect.x = smd->xstart; rect.y = smd->ystart2+1;
	rect.width = smd->width+(smd->xstart2-smd->xstart);
	rect.height = smd->height-1;
	GDrawScroll(smd->gw,&rect,0,diff*smd->stateh);
    }
}

static int subsmd_e_h(GWindow gw, GEvent *event) {
    SMD *smd = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_char:
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("statemachine.html#EditClass");
return( true );
	} else if ( event->u.chr.keysym=='q' && (event->u.chr.state&ksm_control)) {
	    if ( event->u.chr.state&ksm_shift )
		_SMD_Cancel(smd);
	    else
		MenuExit(NULL,NULL,NULL);
return( true );
	}
return( false );
    }
return( true );
}

static int smd_e_h(GWindow gw, GEvent *event) {
    SMD *smd = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_close:
	_SMD_Cancel(smd);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("statemachine.html");
return( true );
	} else if ( event->u.chr.keysym=='q' && (event->u.chr.state&ksm_control)) {
	    if ( event->u.chr.state&ksm_shift )
		_SMD_Cancel(smd);
	    else
		MenuExit(NULL,NULL,NULL);
return( true );
	}
return( false );
      case et_expose:
	SMD_Expose(smd,gw,event);
      break;
      case et_resize: {
	int blen = GDrawPointsToPixels(NULL,GIntGetResource(_NUM_Buttonsize));
	GRect wsize, csize;

	GDrawGetSize(smd->gw,&wsize);
	GDrawResize(smd->cw,wsize.width,wsize.height);
	GGadgetResize(GWidgetGetControl(smd->gw,CID_Group),wsize.width-4,wsize.height-4);
	GGadgetResize(GWidgetGetControl(smd->gw,CID_Group2),wsize.width-4,wsize.height-4);
	GGadgetResize(GWidgetGetControl(smd->gw,CID_Line1),wsize.width-20,1);
	GGadgetResize(GWidgetGetControl(smd->gw,CID_Line2),wsize.width-20,1);

	GGadgetGetSize(GWidgetGetControl(smd->gw,CID_Ok),&csize);
	GGadgetMove(GWidgetGetControl(smd->gw,CID_Ok),csize.x,wsize.height-smd->canceldrop-3);
	GGadgetMove(GWidgetGetControl(smd->cw,CID_Prev),csize.x+3,wsize.height-smd->canceldrop);
	GGadgetGetSize(GWidgetGetControl(smd->gw,CID_Cancel),&csize);
	GGadgetMove(GWidgetGetControl(smd->gw,CID_Cancel),wsize.width-blen-30,wsize.height-smd->canceldrop);
	GGadgetMove(GWidgetGetControl(smd->gw,CID_Next),wsize.width-blen-33,wsize.height-smd->canceldrop-3);

	GGadgetGetSize(GWidgetGetControl(smd->gw,CID_Classes),&csize);
	GGadgetResize(GWidgetGetControl(smd->gw,CID_Classes),wsize.width-GDrawPointsToPixels(NULL,10),csize.height);
	GGadgetResize(GWidgetGetControl(smd->gw,CID_GlyphList),wsize.width-GDrawPointsToPixels(NULL,10),csize.height);

	GGadgetGetSize(smd->hsb,&csize);
	smd->width = wsize.width-csize.height-smd->xstart2-5;
	GGadgetResize(smd->hsb,smd->width,csize.height);
	GGadgetMove(smd->hsb,smd->xstart2,wsize.height-smd->sbdrop-csize.height);
	GGadgetGetSize(smd->vsb,&csize);
	smd->height = wsize.height-smd->sbdrop-smd->ystart2-csize.width;
	GGadgetResize(smd->vsb,csize.width,wsize.height-smd->sbdrop-smd->ystart2-csize.width);
	GGadgetMove(smd->vsb,wsize.width-csize.width-5,smd->ystart2);
	SMD_SBReset(smd);

	GDrawRequestExpose(smd->gw,NULL,false);
      } break;
      case et_mouseup: case et_mousemove: case et_mousedown:
	SMD_Mouse(smd,event);
      break;
      case et_controlevent:
	switch( event->u.control.subtype ) {
	  case et_textchanged:
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
	  break;
	  case et_scrollbarchange:
	    if ( event->u.control.g == smd->hsb )
		SMD_HScroll(smd,&event->u.control.u.sb);
	    else
		SMD_VScroll(smd,&event->u.control.u.sb);
	  break;
	}
      break;
#if 0
      case et_drop:
	smd_Drop(GDrawGetUserData(gw),event);
      break;
#endif
    }
return( true );
}

SMD *StateMachineEdit(SplineFont *sf,ASM *sm,struct gfi_data *d) {
    static int titles[2][3] = {
	{ _STR_EditIndic, _STR_EditContextSub, _STR_EditInsert },
	{ _STR_NewIndic, _STR_NewContextSub, _STR_NewInsert }};
    SMD *smd = gcalloc(1,sizeof(SMD));
    GRect pos, subpos;
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
	smd->class_cnt = 4;			/* 4 built in classes */
	smd->state_cnt = 2;			/* 2 built in states */
	smd->states = gcalloc(smd->class_cnt*smd->state_cnt,sizeof(struct asm_state));
	smd->states[1*4+2].next_state = 1;	/* deleted glyph is a noop */
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

    label[k].text = (unichar_t *) _STR_RightToLeft;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 150; gcd[k].gd.pos.y = 5;
    gcd[k].gd.flags = gg_enabled|gg_visible | (sm->flags&0x4000?gg_cb_on:0);
    gcd[k].gd.cid = CID_RightToLeft;
    gcd[k++].creator = GCheckBoxCreate;

    label[k].text = (unichar_t *) _STR_VerticalOnly;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 150; gcd[k].gd.pos.y = 5+16;
    gcd[k].gd.flags = gg_enabled|gg_visible | (sm->flags&0x8000?gg_cb_on:0);
    gcd[k].gd.cid = CID_VertOnly;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = GDrawPointsToPixels(gw,gcd[k-3].gd.pos.y+27);
    gcd[k].gd.pos.width = pos.width-20;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k].gd.cid = CID_Line1;
    gcd[k++].creator = GLineCreate;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = GDrawPixelsToPoints(gw,gcd[k-1].gd.pos.y)+5;
    gcd[k].gd.pos.width = SMD_WIDTH-10;
    gcd[k].gd.pos.height = 8*12+10;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_list_multiplesel;
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

/* I don't really think Up/Down are meaningful in the class list, but I'll leave them here (invisible) in case I need them later. */
    label[k].text = (unichar_t *) _STR_Up;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+blen+space+5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = /*gg_visible*/0;
    gcd[k].gd.handle_controlevent = SMD_Up;
    gcd[k].gd.cid = CID_Up;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _STR_Down;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+blen+space; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = /*gg_visible*/0;
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
    gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
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

    wattrs.mask = wam_events;
    subpos = pos; subpos.x = subpos.y = 0;
    smd->cw = GWidgetCreateSubWindow(smd->gw,&subpos,subsmd_e_h,smd,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    k = 0;

    label[k].text = (unichar_t *) _STR_Set;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_SetGlyphsFromSelectionPopup,NULL);
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.handle_controlevent = SMD_FromSelection;
    gcd[k].gd.cid = CID_Set;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _STR_Select_nom;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 70; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_SelectFromGlyphsPopup,NULL);
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.handle_controlevent = SMD_ToSelection;
    gcd[k].gd.cid = CID_Select;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 30;
    gcd[k].gd.pos.width = SMD_HEIGHT-25; gcd[k].gd.pos.height = 8*13+4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
    gcd[k].gd.cid = CID_GlyphList;
    gcd[k++].creator = GTextAreaCreate;

    label[k].text = (unichar_t *) _STR_PrevArrow;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30; gcd[k].gd.pos.y = gcd[k].gd.pos.y+3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_cancel;
    gcd[k].gd.handle_controlevent = SMD_Prev;
    gcd[k].gd.cid = CID_Prev;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _STR_NextArrow;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = -30+3; gcd[k].gd.pos.y = SMD_HEIGHT-SMD_CANCELDROP-3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_default;
    gcd[k].gd.handle_controlevent = SMD_Next;
    gcd[k].gd.cid = CID_Next;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width-4;
    gcd[k].gd.pos.height = pos.height-4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k].gd.cid = CID_Group2;
    gcd[k++].creator = GGroupCreate;

    GGadgetsCreate(smd->cw,gcd);


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
