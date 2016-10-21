/* -*- coding: utf-8 -*- */
/* Copyright (C) 2003-2012 by George Williams */
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
#include "lookups.h"
#include "ttf.h"
#include <chardata.h>
#include <utype.h>
#include <ustring.h>
#include <gkeysym.h>

#define CID_Classes	305


#define CID_Ok		307
#define CID_Cancel	308

#define CID_Line1	309
#define CID_Line2	310
#define CID_Group	311

#define CID_Set		313
#define CID_Select	314

#define CID_RightToLeft	318
#define CID_VertOnly	319

#define SMD_WIDTH	350
#define SMD_HEIGHT	400
#define SMD_CANCELDROP	32
#define SMD_DIRDROP	88

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
#define CID_Kerns	412
#define CID_StateClass	413

#define CID_Up		420
#define CID_Down	421
#define CID_Left	422
#define CID_Right	423

#define SMDE_WIDTH	200
#define SMDE_HEIGHT	(SMD_DIRDROP+200)

extern int _GScrollBar_Width;

typedef struct statemachinedlg {
    GWindow gw, editgw;
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
    int done;
} SMD;

static int  SMD_SBReset(SMD *smd);
static void SMD_HShow(SMD *smd,int pos);

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
    } else if ( type==asm_kern ) {
	for ( i=0; i<old_state_cnt ; ++i ) {
	    for ( j=0; j<old_class_cnt; ++j ) {
		struct asm_state *this = &old[i*old_class_cnt+j];
		free(this->u.kern.kerns);
	    }
	}
    }
    free(old);
}

static struct asm_state *StateCopy(struct asm_state *old,int old_class_cnt,int old_state_cnt,
	int new_class_cnt,int new_state_cnt, enum asm_type type,int freeold) {
    struct asm_state *new = calloc(new_class_cnt*new_state_cnt,sizeof(struct asm_state));
    int i,j;
    int minclass = new_class_cnt<old_class_cnt ? new_class_cnt : old_class_cnt;

    for ( i=0; i<old_state_cnt && i<new_state_cnt; ++i ) {
	memcpy(new+i*new_class_cnt, old+i*old_class_cnt, minclass*sizeof(struct asm_state));
	if ( type==asm_insert ) {
	    for ( j=0; j<minclass; ++j ) {
		struct asm_state *this = &new[i*new_class_cnt+j];
		this->u.insert.mark_ins = copy(this->u.insert.mark_ins);
		this->u.insert.cur_ins = copy(this->u.insert.cur_ins);
	    }
	} else if ( type==asm_kern ) {
	    for ( j=0; j<minclass; ++j ) {
		struct asm_state *this = &new[i*new_class_cnt+j];
		int16 *temp;
		temp = malloc(this->u.kern.kcnt*sizeof(int16));
		memcpy(temp,this->u.kern.kerns,this->u.kern.kcnt*sizeof(int16));
		this->u.kern.kerns = temp;
	    }
	}
    }
    for ( ; i<new_state_cnt; ++i )
	new[i*new_class_cnt+2].next_state = i;		/* Deleted glyphs should be treated as noops */

    if ( freeold )
	StatesFree(old,old_class_cnt,old_state_cnt,type);

return( new );
}

static void StateRemoveClasses(SMD *smd, int removeme ) {
    struct asm_state *new;
    int i,j,k;

    if ( removeme<4 )
return;

    new = calloc((smd->class_cnt-1)*smd->state_cnt,sizeof(struct asm_state));
    for ( i=0; i<smd->state_cnt ; ++i ) {
	for ( j=k=0; j<smd->class_cnt; ++j ) {
	    if ( j!=removeme )
		new[i*(smd->class_cnt-1)+k++] = smd->states[i*smd->class_cnt+j];
	    else if ( smd->sm->type==asm_insert ) {
		free(smd->states[i*smd->class_cnt+j].u.insert.mark_ins);
		free(smd->states[i*smd->class_cnt+j].u.insert.cur_ins);
	    } else if ( smd->sm->type==asm_kern ) {
		free(smd->states[i*smd->class_cnt+j].u.kern.kerns);
	    }
	}
    }

    free(smd->states);
    smd->states = new;
    --smd->class_cnt;
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
    { (unichar_t *) N_("No Change"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Ax => xA"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("xD => Dx"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("AxD => DxA"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("ABx => xAB"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("ABx => xBA"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("xCD => CDx"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("xCD => DCx"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("AxCD => CDxA"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("AxCD => DCxA"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("ABxD => DxAB"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("ABxD => DxBA"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("ABxCD => CDxAB"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("ABxCD => CDxBA"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("ABxCD => DCxAB"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("ABxCD => DCxBA"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
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

    ret = pt = malloc(u_strlen(u)+1);
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

static void SMD_Fillup(SMD *smd) {
    int state = smd->st_pos/smd->class_cnt;
    int class = smd->st_pos%smd->class_cnt;
    struct asm_state *this = &smd->states[smd->st_pos];
    char buffer[100];
    char buf[100], *temp;
    int j;
    GGadget *list = GWidgetGetControl( smd->gw, CID_Classes );
    int rows;
    struct matrix_data *classes = GMatrixEditGet(list,&rows);

    snprintf(buffer,sizeof(buffer)/sizeof(buffer[0]),
	    _("State %d,  %.40s"), state, classes[class*1+0].u.md_str );
    GGadgetSetTitle8(GWidgetGetControl(smd->editgw,CID_StateClass),buffer);
    sprintf(buf,"%d", this->next_state );
    GGadgetSetTitle8(GWidgetGetControl(smd->editgw,CID_NextState),buf);

    GGadgetSetChecked(GWidgetGetControl(smd->editgw,CID_Flag4000),
	    this->flags&0x4000?0:1);
    GGadgetSetChecked(GWidgetGetControl(smd->editgw,CID_Flag8000),
	    this->flags&0x8000?1:0);
    if ( smd->sm->type==asm_indic ) {
	GGadgetSetChecked(GWidgetGetControl(smd->editgw,CID_Flag2000),
		this->flags&0x2000?1:0);
	GGadgetSelectOneListItem(GWidgetGetControl(smd->editgw,CID_IndicVerb),
		this->flags&0xf);
    } else if ( smd->sm->type==asm_insert ) {
	GGadgetSetChecked(GWidgetGetControl(smd->editgw,CID_Flag2000),
		this->flags&0x2000?1:0);
	GGadgetSetChecked(GWidgetGetControl(smd->editgw,CID_Flag1000),
		this->flags&0x1000?1:0);
	GGadgetSetChecked(GWidgetGetControl(smd->editgw,CID_Flag0800),
		this->flags&0x0800?1:0);
	GGadgetSetChecked(GWidgetGetControl(smd->editgw,CID_Flag0400),
		this->flags&0x0400?1:0);
	temp = this->u.insert.mark_ins;
	buffer[0]='\0';
	GGadgetSetTitle8(GWidgetGetControl(smd->editgw,CID_InsMark),temp==NULL?buffer:temp);
	temp = this->u.insert.cur_ins;
	GGadgetSetTitle8(GWidgetGetControl(smd->editgw,CID_InsCur),temp==NULL?buffer:temp);
    } else if ( smd->sm->type==asm_kern ) {
	buf[0] = '\0';
	for ( j=0; j<this->u.kern.kcnt; ++j )
	    sprintf( buf+strlen(buf), "%d ", this->u.kern.kerns[j]);
	if ( buf[0]!='\0' && buf[strlen(buf)-1]==' ' )
	    buf[strlen(buf)-1] = '\0';
	GGadgetSetTitle8(GWidgetGetControl(smd->editgw,CID_Kerns),buf);
    } else {
	if ( this->u.context.mark_lookup != NULL )
	GGadgetSetTitle8(GWidgetGetControl(smd->editgw,CID_TagMark),this->u.context.mark_lookup->lookup_name);
	if ( this->u.context.cur_lookup != NULL )
	    GGadgetSetTitle8(GWidgetGetControl(smd->editgw,CID_TagCur),this->u.context.cur_lookup->lookup_name);
    }

    GGadgetSetEnabled(GWidgetGetControl(smd->editgw,CID_Up), state!=0 );
    GGadgetSetEnabled(GWidgetGetControl(smd->editgw,CID_Left), class!=0 );
    GGadgetSetEnabled(GWidgetGetControl(smd->editgw,CID_Right), class<smd->class_cnt-1 );
    GGadgetSetEnabled(GWidgetGetControl(smd->editgw,CID_Down), state<smd->state_cnt-1 );
}

static int SMD_DoChange(SMD *smd) {
    struct asm_state *this = &smd->states[smd->st_pos];
    int err=false, ns, flags, cnt;
    char *mins, *cins;
    OTLookup *mlook, *clook;
    const unichar_t *ret;
    unichar_t *end;
    char *ret8;
    int16 kbuf[9];
    int kerns;
    int oddcomplain=false;

    ns = GetInt8(smd->editgw,CID_NextState,_("Next State:"),&err);
    if ( err )
return( false );
    flags = 0;
    if ( !GGadgetIsChecked(GWidgetGetControl(smd->editgw,CID_Flag4000)) ) flags |= 0x4000;
    if ( GGadgetIsChecked(GWidgetGetControl(smd->editgw,CID_Flag8000)) ) flags |= 0x8000;
    if ( smd->sm->type==asm_indic ) {
	if ( GGadgetIsChecked(GWidgetGetControl(smd->editgw,CID_Flag2000)) ) flags |= 0x2000;
	flags |= GGadgetGetFirstListSelectedItem(GWidgetGetControl(smd->editgw,CID_IndicVerb));
	this->next_state = ns;
	this->flags = flags;
    } else if ( smd->sm->type==asm_kern ) {
	ret = _GGadgetGetTitle(GWidgetGetControl(smd->editgw,CID_Kerns));
	kerns=0;
	while ( *ret!='\0' ) {
	    while ( *ret==' ' ) ++ret;
	    if ( *ret=='\0' )
	break;
	    kbuf[kerns] = u_strtol(ret,&end,10);
	    if ( end==ret ) {
		GGadgetProtest8(_("Kern Values:"));
return( false );
	    } else if ( kerns>=8 ) {
		ff_post_error(_("Too Many Kerns"),_("At most 8 kerning values may be specified here"));
return( false );
	    } else if ( kbuf[kerns]&1 ) {
		kbuf[kerns] &= ~1;
		if ( !oddcomplain )
		    ff_post_notice(_("Kerning values must be even"),_("Kerning values must be even"));
		oddcomplain = true;
	    }
	    ++kerns;
	    ret = end;
	}
	this->next_state = ns;
	this->flags = flags;
	free(this->u.kern.kerns);
	this->u.kern.kcnt = kerns;
	if ( kerns==0 )
	    this->u.kern.kerns = NULL;
	else {
	    this->u.kern.kerns = malloc(kerns*sizeof(int16));
	    memcpy(this->u.kern.kerns,kbuf,kerns*sizeof(int16));
	}
    } else if ( smd->sm->type==asm_context ) {
	ret8 = GGadgetGetTitle8(GWidgetGetControl(smd->editgw,CID_TagMark));
	if ( *ret8=='\0' )
	    mlook = NULL; 		/* That's ok */
	else if ( (mlook=SFFindLookup(smd->sf,ret8))==NULL ) {
	    ff_post_error(_("Unknown lookup"),_("Lookup, %s, does not exist"), ret8 );
	    free(ret8);
return( false );
	} else if ( mlook->lookup_type!=gsub_single ) {
	    ff_post_error(_("Bad lookup type"),_("Lookups in contextual state machines must be simple substitutions,\n, but %s is not"), ret8 );
	    free(ret8);
return( false );
	}
	free(ret8);
	ret8 = GGadgetGetTitle8(GWidgetGetControl(smd->editgw,CID_TagCur));
	if ( *ret8=='\0' )
	    clook = NULL; 		/* That's ok */
	else if ( (clook=SFFindLookup(smd->sf,ret8))==NULL ) {
	    ff_post_error(_("Unknown lookup"),_("Lookup, %s, does not exist"), ret8 );
	    free(ret8);
return( false );
	} else if ( clook->lookup_type!=gsub_single ) {
	    ff_post_error(_("Bad lookup type"),_("Lookups in contextual state machines must be simple substitutions,\n, but %s is not"), ret8 );
	    free(ret8);
return( false );
	}
	this->next_state = ns;
	this->flags = flags;
	this->u.context.mark_lookup = mlook;
	this->u.context.cur_lookup = clook;
    } else {

	if ( GGadgetIsChecked(GWidgetGetControl(smd->editgw,CID_Flag2000)) ) flags |= 0x2000;
	if ( GGadgetIsChecked(GWidgetGetControl(smd->editgw,CID_Flag1000)) ) flags |= 0x1000;
	if ( GGadgetIsChecked(GWidgetGetControl(smd->editgw,CID_Flag0800)) ) flags |= 0x0800;
	if ( GGadgetIsChecked(GWidgetGetControl(smd->editgw,CID_Flag0400)) ) flags |= 0x0400;

	mins = copy_count(smd->editgw,CID_InsMark,&cnt);
	if ( cnt>31 ) {
	    ff_post_error(_("Too Many Glyphs"),_("At most 31 glyphs may be specified in an insert list"));
	    free(mins);
return( false );
	}
	flags |= cnt<<5;

	cins = copy_count(smd->editgw,CID_InsCur,&cnt);
	if ( cnt>31 ) {
	    ff_post_error(_("Too Many Glyphs"),_("At most 31 glyphs may be specified in an insert list"));
	    free(mins);
	    free(cins);
return( false );
	}
	flags |= cnt;
	this->next_state = ns;
	this->flags = flags;
	free(this->u.insert.mark_ins);
	free(this->u.insert.cur_ins);
	this->u.insert.mark_ins = mins;
	this->u.insert.cur_ins = cins;
    }

    /* Show changes in main window */
    GDrawRequestExpose(smd->gw,NULL,false);
return( true );
}

static int SMDE_Arrow(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
	int state = smd->st_pos/smd->class_cnt;
	int class = smd->st_pos%smd->class_cnt;
	switch( GGadgetGetCid(g)) {
	  case CID_Up:
	    if ( state!=0 ) --state;
	  break;
	  case CID_Left:
	    if ( class!=0 ) --class;
	  break;
	  case CID_Right:
	    if ( class<smd->class_cnt-1 ) ++class;
	  break;
	  case CID_Down:
	    if ( state<smd->state_cnt-1 ) ++state;
	  break;
	}
	if ( state!=smd->st_pos/smd->class_cnt || class!=smd->st_pos%smd->class_cnt ) {
	    if ( SMD_DoChange(smd)) {
		smd->st_pos = state*smd->class_cnt+class;
		SMD_Fillup(smd);
	    }
	}
    }
return( true );
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
	    smd->edit_done = SMD_DoChange(smd);
return( true );
	}
return( false );
      case et_controlevent:
	switch( event->u.control.subtype ) {
	  case et_buttonactivate:
	    if ( GGadgetGetCid(event->u.control.g)==CID_Ok )
		smd->edit_done = SMD_DoChange(smd);
	    else
		smd->edit_done = true;
	  break;
	}
      break;
    }
return( true );
}

static void SMD_EditState(SMD *smd) {
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[23];
    GTextInfo label[23];
    GRect pos;
    int k, listk, new_cnt;
    char stateclass[100];
    static int indicv_done = false;

    smd->edit_done = false;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = true;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Edit State Transition");
    pos.x = pos.y = 0;
    pos.width = GDrawPointsToPixels(NULL,GGadgetScale(SMDE_WIDTH));
    pos.height = GDrawPointsToPixels(NULL,SMDE_HEIGHT);
    smd->editgw = gw = GDrawCreateTopWindow(NULL,&pos,smdedit_e_h,smd,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    k = 0; listk = -1;

    snprintf(stateclass,sizeof(stateclass), _("State %d,  %.40s"),
	    999, _("Class 1: {Everything Else}" ));
    label[k].text = (unichar_t *) stateclass;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 5;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_StateClass;
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) _("Next State:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+13+4;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;

    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_NextState;
    gcd[k++].creator = GTextFieldCreate;

    label[k].text = (unichar_t *) _("Advance To Next Glyph");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+24;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_Flag4000;
    gcd[k++].creator = GCheckBoxCreate;

    label[k].text = (unichar_t *) (smd->sm->type==asm_kern?_("Push Current Glyph"):
				   smd->sm->type!=asm_indic?_("Mark Current Glyph"):
					    _("Mark Current Glyph As First"));
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_Flag8000;
    gcd[k++].creator = GCheckBoxCreate;

    if ( smd->sm->type==asm_indic ) {
	if ( !indicv_done ) {
	    int i;
	    for ( i=0; indicverbs_list[i].text!=NULL; ++i )
		indicverbs_list[i].text = (unichar_t *) _((char *) indicverbs_list[i].text );
	    indicv_done = true;
	}
	label[k].text = (unichar_t *) _("Mark Current Glyph As Last");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Flag2000;
	gcd[k++].creator = GCheckBoxCreate;

	gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+24;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.u.list = indicverbs_list;
	gcd[k].gd.cid = CID_IndicVerb;
	gcd[k++].creator = GListButtonCreate;
    } else if ( smd->sm->type==asm_insert ) {
	label[k].text = (unichar_t *) _("Current Glyph Is Kashida Like");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Flag2000;
	gcd[k++].creator = GCheckBoxCreate;

	label[k].text = (unichar_t *) _("Marked Glyph Is Kashida Like");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Flag1000;
	gcd[k++].creator = GCheckBoxCreate;

	label[k].text = (unichar_t *) _("Insert Before Current Glyph");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Flag0800;
	gcd[k++].creator = GCheckBoxCreate;

	label[k].text = (unichar_t *) _("Insert Before Marked Glyph");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Flag0400;
	gcd[k++].creator = GCheckBoxCreate;

	label[k].text = (unichar_t *) _("Mark Insert:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+26;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	gcd[k].gd.pos.x = gcd[2].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_InsMark;
	gcd[k++].creator = GTextFieldCreate;

	label[k].text = (unichar_t *) _("Current Insert:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	gcd[k].gd.pos.x = gcd[2].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_InsCur;
	gcd[k++].creator = GTextFieldCreate;
    } else if ( smd->sm->type==asm_kern ) {
	label[k].text = (unichar_t *) _("Kern Values:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+26;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	gcd[k].gd.pos.x = gcd[2].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Kerns;
	gcd[k++].creator = GTextFieldCreate;
    } else {
	label[k].text = (unichar_t *) _("Mark Subs:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+26;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	listk = k;
	gcd[k].gd.pos.x = gcd[2].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_TagMark;
	gcd[k++].creator = GListFieldCreate;

	label[k].text = (unichar_t *) _("Current Subs:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	gcd[k].gd.pos.x = gcd[2].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_TagCur;
	gcd[k].gd.u.list = gcd[k-2].gd.u.list;
	gcd[k++].creator = GListFieldCreate;
    }

    label[k].text = (unichar_t *) U_("_Up↑");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = (SMDE_WIDTH-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor))/2;
    gcd[k].gd.pos.y = SMDE_HEIGHT-SMD_DIRDROP;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled;
    gcd[k].gd.cid = CID_Up;
    gcd[k].gd.handle_controlevent = SMDE_Arrow;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) U_("←_Left");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = SMDE_HEIGHT-SMD_DIRDROP+13;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled;
    gcd[k].gd.cid = CID_Left;
    gcd[k].gd.handle_controlevent = SMDE_Arrow;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) U_("_Right→");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = -10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled;
    gcd[k].gd.cid = CID_Right;
    gcd[k].gd.handle_controlevent = SMDE_Arrow;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) U_("↓_Down");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-3].gd.pos.x; gcd[k].gd.pos.y = SMDE_HEIGHT-SMD_DIRDROP+26;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled;
    gcd[k].gd.cid = CID_Down;
    gcd[k].gd.handle_controlevent = SMDE_Arrow;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = SMDE_HEIGHT-SMD_CANCELDROP-3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_default;
    gcd[k].gd.cid = CID_Ok;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
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
		SFLookupListFromType(smd->sf,gsub_single),false);
	GGadgetSetList(gcd[listk+2].ret,
		SFLookupListFromType(smd->sf,gsub_single),false);
    }

    SMD_Fillup(smd);

    GDrawSetVisible(gw,true);

    smd->edit_done = false;
    while ( !smd->edit_done )
	GDrawProcessOneEvent(NULL);

    new_cnt = FindMaxReachableStateCnt(smd);
    if ( new_cnt!=smd->state_cnt ) {
	smd->states = StateCopy(smd->states,smd->class_cnt,smd->state_cnt,
		smd->class_cnt,new_cnt,
		smd->sm->type,true);
	smd->state_cnt = new_cnt;
	SMD_SBReset(smd);
	GDrawRequestExpose(smd->gw,NULL,false);
    }
    smd->st_pos = -1;
    GDrawDestroyWindow(gw);
return;
}

/* ************************************************************************** */
/* ****************************** Main Dialog ******************************* */
/* ************************************************************************** */

static void _SMD_Finish(SMD *smd, int success) {

    GDrawDestroyWindow(smd->gw);

    GFI_FinishSMNew(smd->d,smd->sm,success,smd->isnew);

    GTextInfoListFree(smd->mactags);
    smd->done = true;
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
	int i;
  struct ggadget * classControl = GWidgetGetControl(smd->gw,CID_Classes);
  int rows;
  struct matrix_data *classes = GMatrixEditGet(classControl,&rows);

	ASM *sm = smd->sm;
	char *upt;

	for ( i=4; i<sm->class_cnt; ++i )
	    free(sm->classes[i]);
	free(sm->classes);
	sm->classes = malloc(smd->class_cnt*sizeof(char *));
	sm->classes[0] = sm->classes[1] = sm->classes[2] = sm->classes[3] = NULL;
	sm->class_cnt = smd->class_cnt;
    for ( i=4; i<sm->class_cnt; ++i ) {
        upt = strstr(classes[i].u.md_str,": ");
        if ( upt==NULL ) upt = classes[i].u.md_str; else upt += 2;
        sm->classes[i] = copy(GlyphNameListDeUnicode(upt));
    }

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
    char *pt;
    char buf[30];
    int len;
    struct matrix_data *classes;
    int pos = ((event->u.mouse.y-smd->ystart2)/smd->stateh+smd->offtop) * smd->class_cnt +
	    (event->u.mouse.x-smd->xstart2)/smd->statew + smd->offleft;

    GGadgetEndPopup();

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
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
	    classes = GMatrixEditGet(GWidgetGetControl(smd->gw,CID_Classes),&len);
	    len = u_strlen(space);
	    pt = strstr(classes[c].u.md_str,": ");
	    if ( pt==NULL ) pt = classes[c].u.md_str;
	    else pt += 2;
	    utf82u_strncpy(space+len,pt,(sizeof(space)/sizeof(space[0]))-1 - len);
	} else if ( event->u.mouse.x<smd->xstart2 ) {
	    if ( s==0 )
		utf82u_strcat(space,_("{Start of Input}"));
	    else if ( s==1 )
		utf82u_strcat(space,_("{Start of Line}"));
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
    int len, off, i, j, x, y, kddd=false;
    unichar_t ubuf[8];
    char buf[101];

    if ( area->y+area->height<smd->ystart )
return;
    if ( area->y>smd->ystart2+smd->height )
return;

    GDrawPushClip(pixmap,area,&old1);
    GDrawSetFont(pixmap,smd->font);
    GDrawSetLineWidth(pixmap,0);
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
		    ubuf,1,0xff0000);
	    }
	}
    }
    for ( i=0 ; smd->offleft+i<=smd->class_cnt && (i-1)*smd->statew<smd->width; ++i ) {
	GDrawDrawLine(pixmap,smd->xstart2+i*smd->statew,smd->ystart,smd->xstart2+i*smd->statew,smd->ystart+rect.height,
		0x808080);
	if ( i+smd->offleft<smd->class_cnt ) {
	    GDrawDrawText8(pixmap,smd->xstart2+i*smd->statew+1,smd->ystart+smd->as+1,
		"Class",-1,0xff0000);
	    sprintf( buf, "%d", i+smd->offleft );
	    len = GDrawGetText8Width(pixmap,buf,-1);
	    GDrawDrawText8(pixmap,smd->xstart2+i*smd->statew+(smd->statew-len)/2,smd->ystart+smd->fh+smd->as+1,
		buf,-1,0xff0000);
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
	    len = GDrawGetText8Width(pixmap,buf,-1);
	    GDrawDrawText8(pixmap,x+(smd->statew-len)/2,y+smd->as+1,
		buf,-1,0x000000);

	    ubuf[0] = (this->flags&0x8000)? 'M' : ' ';
	    if ( smd->sm->type==asm_kern && (this->flags&0x8000))
		ubuf[0] = 'P';
	    ubuf[1] = (this->flags&0x4000)? ' ' : 'A';
	    ubuf[2] = '\0';
	    if ( smd->sm->type==asm_indic ) {
		ubuf[2] = (this->flags&0x2000) ? 'L' : ' ';
		ubuf[3] = '\0';
	    }
	    len = GDrawGetTextWidth(pixmap,ubuf,-1);
	    GDrawDrawText(pixmap,x+(smd->statew-len)/2,y+smd->fh+smd->as+1,
		ubuf,-1,0x000000);

	    buf[0]='\0';
	    if ( smd->sm->type==asm_indic ) {
		strcpy(buf,indicverbs[0][this->flags&0xf]);
	    } else if ( smd->sm->type==asm_context ) {
		if ( this->u.context.mark_lookup!=NULL ) {
		    strncpy(buf,this->u.context.mark_lookup->lookup_name,6);
		    buf[6] = '\0';
		}
	    } else if ( smd->sm->type==asm_insert ) {
		if ( this->u.insert.mark_ins!=NULL )
		    strncpy(buf,this->u.insert.mark_ins,5);
	    } else { /* kern */
		if ( this->u.kern.kerns!=NULL ) {
		    int j;
		    buf[0] = '\0';
		    for ( j=0; j<this->u.kern.kcnt; ++j )
			sprintf(buf+strlen(buf),"%d ", this->u.kern.kerns[j]);
		    kddd = ( strlen(buf)>5 );
		    buf[5] = '\0';
		} else
		    kddd = false;
	    }
	    len = GDrawGetText8Width(pixmap,buf,-1);
	    GDrawDrawText8(pixmap,x+(smd->statew-len)/2,y+2*smd->fh+smd->as+1,
		buf,-1,0x000000);

	    buf[0] = '\0';
	    if ( smd->sm->type==asm_indic ) {
		strncpy(buf,indicverbs[1][this->flags&0xf],sizeof(buf)-1);
	    } else if ( smd->sm->type==asm_context ) {
		if ( this->u.context.cur_lookup!=NULL ) {
		    strncpy(buf,this->u.context.cur_lookup->lookup_name,6);
		    buf[6] = '\0';
		}
	    } else if ( smd->sm->type==asm_insert ) {
		if ( this->u.insert.cur_ins!=NULL )
		    strncpy(buf,this->u.insert.cur_ins,5);
	    } else { /* kern */
		if ( kddd ) strcpy(buf,"...");
		else buf[0] = '\0';
	    }
	    len = GDrawGetText8Width(pixmap,buf,-1);
	    GDrawDrawText8(pixmap,x+(smd->statew-len)/2,y+3*smd->fh+smd->as+1,
		buf,-1,0x000000);
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

static void SMD_HShow(SMD *smd, int pos) {
    if ( pos<0 || pos>=smd->class_cnt )
return;
    --pos;	/* One line of context */
    if ( pos + (smd->width/smd->statew) >= smd->class_cnt )
	pos = smd->class_cnt - (smd->width/smd->statew);
    if ( pos < 0 ) pos = 0;
    smd->offleft = pos;
    GScrollBarSetPos(smd->hsb,pos);
    GDrawRequestExpose(smd->gw,NULL,false);
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
	SMD_SBReset(smd);

	GDrawRequestExpose(smd->gw,NULL,false);
      } break;
      case et_mouseup: case et_mousemove: case et_mousedown:
	SMD_Mouse(smd,event);
      break;
      case et_controlevent:
	switch( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    if ( event->u.control.g == smd->hsb )
		SMD_HScroll(smd,&event->u.control.u.sb);
	    else
		SMD_VScroll(smd,&event->u.control.u.sb);
	  break;
	}
      break;
    }
return( true );
}

static char *SMD_PickGlyphsForClass(GGadget *g,int r, int c) {
    SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *classes = _GMatrixEditGet(g,&rows);
    char *new = GlyphSetFromSelection(smd->sf,ly_fore,classes[r*cols+c].u.md_str);
return( new );
}

static void SMD_NewClassRow(GGadget *g,int r) {
    SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));

    smd->states = StateCopy(smd->states,smd->class_cnt,smd->state_cnt,
	    smd->class_cnt+1,smd->state_cnt,
	    smd->sm->type,true);
    ++smd->class_cnt;
    SMD_SBReset(smd);
}

static void SMD_FinishEdit(GGadget *g,int r, int c, int wasnew) {
    SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));

    ME_ClassCheckUnique(g, r, c, smd->sf);
}

static int SMD_EnableDeleteClass(GGadget *g,int whichclass) {
return( whichclass>=4 );
}

static void SMD_DeleteClass(GGadget *g,int whichclass) {
    SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));

    StateRemoveClasses(smd,whichclass);
    SMD_SBReset(smd);
    GDrawRequestExpose(smd->gw,NULL,false);
}

static void SMD_ClassSelectionChanged(GGadget *g,int whichclass, int c) {
    SMD *smd = GDrawGetUserData(GGadgetGetWindow(g));
    SMD_HShow(smd,whichclass);
}

static unichar_t **SMD_GlyphListCompletion(GGadget *t,int from_tab) {
    SMD *smd = GDrawGetUserData(GDrawGetParentWindow(GGadgetGetWindow(t)));
    SplineFont *sf = smd->sf;

return( SFGlyphNameCompletion(sf,t,from_tab,true));
}

static struct col_init class_ci[] = {
    { me_funcedit, SMD_PickGlyphsForClass, NULL, NULL, N_("Glyphs in the classes") },
    };
void StateMachineEdit(SplineFont *sf,ASM *sm,struct gfi_data *d) {
    static char *titles[2][4] = {
	{ N_("Edit Indic Rearrangement"), N_("Edit Contextual Substitution"), N_("Edit Contextual Glyph Insertion"), N_("Edit Contextual Kerning") },
	{ N_("New Indic Rearrangement"), N_("New Contextual Substitution"), N_("New Contextual Glyph Insertion"), N_("New Contextual Kerning") }};
    SMD smd;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[20];
    GTextInfo label[20];
    int i, k, vk;
    int as, ds, ld, sbsize;
    FontRequest rq;
    static unichar_t statew[] = { '1', '2', '3', '4', '5', 0 };
    static GFont *font = NULL;
    struct matrix_data *md;
    struct matrixinit mi;
    static char *specialclasses[4] = { N_("{End of Text}"),
	    N_("{Everything Else}"),
	    N_("{Deleted Glyph}"),
	    N_("{End of Line}") };

    memset(&smd,0,sizeof(smd));
    smd.sf = sf;
    smd.sm = sm;
    smd.d = d;
    smd.isnew = (sm->class_cnt==0);
    if ( smd.isnew ) {
	smd.class_cnt = 4;			/* 4 built in classes */
	smd.state_cnt = 2;			/* 2 built in states */
	smd.states = calloc(smd.class_cnt*smd.state_cnt,sizeof(struct asm_state));
	smd.states[1*4+2].next_state = 1;	/* deleted glyph is a noop */
    } else {
	smd.class_cnt = sm->class_cnt;
	smd.state_cnt = sm->state_cnt;
	smd.states = StateCopy(sm->state,sm->class_cnt,sm->state_cnt,
		smd.class_cnt,smd.state_cnt,sm->type,false);
    }
    smd.index = sm->type==asm_indic ? 0 : sm->type==asm_context ? 1 : sm->type==asm_insert ? 2 : 3;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = true;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _(titles[smd.isnew][smd.index]);
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(SMD_WIDTH));
    pos.height = GDrawPointsToPixels(NULL,SMD_HEIGHT);
    smd.gw = gw = GDrawCreateTopWindow(NULL,&pos,smd_e_h,&smd,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    k = 0;

    label[k].text = (unichar_t *) _("Right To Left");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 150; gcd[k].gd.pos.y = 5;
    gcd[k].gd.flags = gg_enabled|gg_visible | (sm->flags&0x4000?gg_cb_on:0);
    gcd[k].gd.cid = CID_RightToLeft;
    gcd[k++].creator = GCheckBoxCreate;
    if ( smd.sm->type == asm_kern ) {
	gcd[k-1].gd.flags = gg_enabled;		/* I'm not sure why kerning doesn't have an r2l bit */
    }

    label[k].text = (unichar_t *) _("Vertical Only");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 150; gcd[k].gd.pos.y = 5+16;
    gcd[k].gd.flags = gg_enabled|gg_visible | (sm->flags&0x8000?gg_cb_on:0);
    gcd[k].gd.cid = CID_VertOnly;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = GDrawPointsToPixels(gw,gcd[k-1].gd.pos.y+15);
    gcd[k].gd.pos.width = pos.width-20;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k].gd.cid = CID_Line1;
    gcd[k++].creator = GLineCreate;

    memset(&mi,0,sizeof(mi));
    mi.col_cnt = 1;
    mi.col_init = class_ci;

    if ( sm->class_cnt<4 ) sm->class_cnt=4;
    md = calloc(sm->class_cnt+1,sizeof(struct matrix_data));
    for ( i=0; i<sm->class_cnt; ++i ) {
	if ( i<4 ) {
	    md[i+0].u.md_str = copy( _(specialclasses[i]) );
	    md[i+0].frozen = true;
	} else
      if (sm->classes[i])
        md[i+0].u.md_str = SFNameList2NameUni(sf,sm->classes[i]);
    }
    mi.matrix_data = md;
    mi.initial_row_cnt = i;
    mi.initrow = SMD_NewClassRow;
    mi.finishedit = SMD_FinishEdit;
    mi.candelete = SMD_EnableDeleteClass;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = GDrawPixelsToPoints(gw,gcd[k-1].gd.pos.y)+5;
    gcd[k].gd.pos.width = SMD_WIDTH-10;
    gcd[k].gd.pos.height = 8*12+34;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_Classes;
    gcd[k].gd.u.matrix = &mi;
    gcd[k++].creator = GMatrixEditCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = GDrawPointsToPixels(gw,gcd[k-1].gd.pos.y+gcd[k-1].gd.pos.height+5);
    gcd[k].gd.pos.width = pos.width-20;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k].gd.cid = CID_Line2;
    gcd[k++].creator = GLineCreate;

    smd.canceldrop = GDrawPointsToPixels(gw,SMD_CANCELDROP);
    smd.sbdrop = smd.canceldrop+GDrawPointsToPixels(gw,7);

    vk = k;
    gcd[k].gd.pos.width = sbsize = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gcd[k].gd.pos.x = pos.width-sbsize;
    gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+8;
    gcd[k].gd.pos.height = pos.height-gcd[k].gd.pos.y-sbsize-smd.sbdrop;
    gcd[k].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    gcd[k++].creator = GScrollBarCreate;
    smd.height = gcd[k-1].gd.pos.height;
    smd.ystart = gcd[k-1].gd.pos.y;

    gcd[k].gd.pos.height = sbsize;
    gcd[k].gd.pos.y = pos.height-sbsize-8;
    gcd[k].gd.pos.x = 4;
    gcd[k].gd.pos.width = pos.width-sbsize;
    gcd[k].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    gcd[k++].creator = GScrollBarCreate;
    smd.width = gcd[k-1].gd.pos.width;
    smd.xstart = 5;

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = SMD_HEIGHT-SMD_CANCELDROP-3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_default;
    gcd[k].gd.handle_controlevent = SMD_Ok;
    gcd[k].gd.cid = CID_Ok;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
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
    smd.vsb = gcd[vk].ret;
    smd.hsb = gcd[vk+1].ret;

    {
	GGadget *list = GWidgetGetControl(smd.gw,CID_Classes);
	GMatrixEditSetBeforeDelete(list, SMD_DeleteClass);
    /* When the selection changes */
	GMatrixEditSetOtherButtonEnable(list, SMD_ClassSelectionChanged);
	GMatrixEditSetColumnCompletion(list,0,SMD_GlyphListCompletion);
    }

    if ( font==NULL ) {
	memset(&rq,'\0',sizeof(rq));
	rq.point_size = 12;
	rq.weight = 400;
	rq.utf8_family_name = MONO_UI_FAMILIES;
	font = GDrawInstanciateFont(gw,&rq);
	font = GResourceFindFont("StateMachine.Font",font);
    }
    smd.font = font;
    GDrawWindowFontMetrics(gw,smd.font,&as,&ds,&ld);
    smd.fh = as+ds; smd.as = as;
    GDrawSetFont(gw,smd.font);

    smd.stateh = 4*smd.fh+3;
    smd.statew = GDrawGetTextWidth(gw,statew,-1)+3;
    smd.xstart2 = smd.xstart+smd.statew/2;
    smd.ystart2 = smd.ystart+2*smd.fh+1;

    GDrawSetVisible(gw,true);
    while ( !smd.done )
	GDrawProcessOneEvent(NULL);
}
