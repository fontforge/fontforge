/* Copyright (C) 2000-2009 by George Williams */
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
#include <utype.h>
#include <ustring.h>
#include "unicoderange.h"

static int alpha(const void *_t1, const void *_t2) {
    const GTextInfo *t1 = _t1, *t2 = _t2;

return( strcmp((char *) (t1->text),(char *) (t2->text)));
}

static GTextInfo *AvailableRanges(SplineFont *sf,EncMap *map) {
    GTextInfo *ret = gcalloc(unicoderange_cnt+3,sizeof(GTextInfo));
    int i, cnt, ch, pos;

    for ( i=cnt=0; unicoderange[i].name!=NULL; ++i ) {
	ch = unicoderange[i].defined==-1 ? unicoderange[i].first : unicoderange[i].defined;
	pos = SFFindSlot(sf,map,ch,NULL);
	if ( pos!=-1 ) {
	    ret[cnt].text = (unichar_t *) _(unicoderange[i].name);
	    ret[cnt].text_is_1byte = true;
	    ret[cnt++].userdata = (void *) (intpt) pos;
	}
    }
    qsort(ret,cnt,sizeof(GTextInfo),alpha);
return( ret );
}

typedef struct gotodata {
    SplineFont *sf;
    EncMap *map;
    GWindow gw;
    int ret, done;
    GTextInfo *ranges;
} GotoData;

#define CID_Name 1000

static int Goto_Cancel(GGadget *g, GEvent *e) {
    GWindow gw;
    GotoData *d;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	d->done = true;
    }
return( true );
}

static int Goto_OK(GGadget *g, GEvent *e) {
    GWindow gw;
    GotoData *d;
    char *ret;
    int i;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	ret = GGadgetGetTitle8(GWidgetGetControl(gw,CID_Name));
	if ( d->ranges!=NULL ) {
	    for ( i=0; d->ranges[i].text!=NULL; ++i ) {
		if ( strcmp(ret,(char *) d->ranges[i].text)==0 ) {
		    d->ret = (intpt) d->ranges[i].userdata;
	    break;
		}
	    }
	}
	if ( d->ret==-1 ) {
	    d->ret = NameToEncoding(d->sf,d->map,ret);
	    if ( d->ret<0 || (d->ret>=d->map->enccount && d->sf->cidmaster==NULL ))
		d->ret = -1;
	    if ( d->ret==-1 ) {
		ff_post_notice(_("Goto"),_("Could not find the glyph: %.70s"),ret);
	    } else
		d->done = true;
	} else
	    d->done = true;
	free(ret);
    }
return( true );
}

static int goto_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	GotoData *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

static unichar_t **GotoCompletion(GGadget *t,int from_tab) {
    unichar_t *pt, *spt; unichar_t **ret;
    int gid, cnt, doit, match_len;
    SplineChar *sc;
    GotoData *d = GDrawGetUserData(GGadgetGetWindow(t));
    SplineFont *sf = d->sf;
    int do_wildcards;

    pt = spt = (unichar_t *) _GGadgetGetTitle(t);
    if ( pt==NULL )
return( NULL );
    while ( *pt && *pt!='*' && *pt!='?' && *pt!='[' && *pt!='{' )
	++pt;
    do_wildcards = *pt!='\0';
    if ( do_wildcards && !from_tab )
return( NULL );
    if ( do_wildcards ) {
	pt = spt;
	spt = galloc((u_strlen(spt)+2)*sizeof(unichar_t));
	u_strcpy(spt,pt);
	uc_strcat(spt,"*");
    }

    match_len = u_strlen(spt);
    ret = NULL;
    for ( doit=0; doit<2; ++doit ) {
	cnt=0;
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL ) {
	    int matched;
	    if ( do_wildcards ) {
		unichar_t *temp = utf82u_copy(sc->name);
		matched = GGadgetWildMatch((unichar_t *) spt,temp,false);
		free(temp);
	    } else
		matched = uc_strncmp(spt,sc->name,match_len)==0;
	    if ( matched ) {
		if ( doit )
		    ret[cnt] = utf82u_copy(sc->name);
		++cnt;
	    }
	}
	if ( doit )
	    ret[cnt] = NULL;
	else if ( cnt==0 )
    break;
	else
	    ret = galloc((cnt+1)*sizeof(unichar_t *));
    }
    if ( do_wildcards )
	free(spt);
return( ret );
}

int GotoChar(SplineFont *sf,EncMap *map) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[9], boxes[3], *hvarray[5][2], *barray[10];
    GTextInfo label[9];
    static GotoData gd;
    GTextInfo *ranges = NULL;

    if ( !map->enc->only_1byte )
	ranges = AvailableRanges(sf,map);
    memset(&gd,0,sizeof(gd));
    gd.sf = sf;
    gd.map = map;
    gd.ret = -1;
    gd.ranges = ranges;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Goto");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,170));
    pos.height = GDrawPointsToPixels(NULL,90);
    gd.gw = gw = GDrawCreateTopWindow(NULL,&pos,goto_e_h,&gd,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    label[0].text = (unichar_t *) _("Enter the name of a glyph in the font");
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.flags = gg_enabled|gg_visible;
    gcd[0].creator = GLabelCreate;
    hvarray[0][0] = &gcd[0]; hvarray[0][1] = NULL;

    gcd[1].gd.flags = gg_enabled|gg_visible|gg_text_xim;
    gcd[1].gd.cid = CID_Name;
    if ( ranges==NULL )
	gcd[1].creator = GTextCompletionCreate;
    else {
	gcd[1].gd.u.list = ranges;
	gcd[1].creator = GListFieldCreate;
    }
    hvarray[1][0] = &gcd[1]; hvarray[1][1] = NULL;
    hvarray[2][0] = GCD_Glue; hvarray[2][1] = NULL;

    gcd[2].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[2].text = (unichar_t *) _("_OK");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.handle_controlevent = Goto_OK;
    gcd[2].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[2]; barray[2] = GCD_Glue; barray[3] = GCD_Glue;

    gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[3].text = (unichar_t *) _("_Cancel");
    label[3].text_is_1byte = true;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.handle_controlevent = Goto_Cancel;
    gcd[3].creator = GButtonCreate;
    barray[4] = GCD_Glue; barray[5] = &gcd[3]; barray[6] = GCD_Glue; barray[7] = NULL;

    boxes[2].gd.flags = gg_visible | gg_enabled;
    boxes[2].gd.u.boxelements = barray;
    boxes[2].creator = GHBoxCreate;
    hvarray[3][0] = &boxes[2]; hvarray[3][1] = NULL;
    hvarray[4][0] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_visible | gg_enabled;
    boxes[0].gd.u.boxelements = hvarray[0];
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GCompletionFieldSetCompletion(gcd[1].ret,GotoCompletion);
    GCompletionFieldSetCompletionMode(gcd[1].ret,true);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);
    GDrawSetVisible(gw,true);
    while ( !gd.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    free(ranges);
return( gd.ret );
}
