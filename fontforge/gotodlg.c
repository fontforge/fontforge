/* Copyright (C) 2000-2007 by George Williams */
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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include <ustring.h>
#include "unicoderange.h"

struct unicoderange specialnames[] = {
    { NULL }
};

const char *UnicodeRange(int unienc) {
    char *ret;
    struct unicoderange *best=NULL;
    int i;

    ret = "Unencoded Unicode";
    if ( unienc<0 )
return( ret );

    for ( i=0; unicoderange[i].name!=NULL; ++i ) {
	if ( unienc>=unicoderange[i].first && unienc<=unicoderange[i].last ) {
	    if ( best==NULL )
		best = &unicoderange[i];
	    else if (( unicoderange[i].first>best->first &&
			unicoderange[i].last<=best->last ) ||
		     ( unicoderange[i].first>=best->first &&
			unicoderange[i].last<best->last ))
		best = &unicoderange[i];
	}
    }
    if ( best!=NULL )
return(best->name);

return( ret );
}

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
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

int NameToEncoding(SplineFont *sf,EncMap *map,const char *name) {
    int enc, uni, i, ch;
    char *end, *dot=NULL, *freeme=NULL;
    const char *upt = name;

    ch = utf8_ildb(&upt);
    if ( *upt=='\0' ) {
	enc = SFFindSlot(sf,map,ch,NULL);
	if ( enc!=-1 )
return( enc );
    }

    enc = uni = -1;
	
    while ( 1 ) {
	enc = SFFindSlot(sf,map,-1,name);
	if ( enc!=-1 ) {
	    free(freeme);
return( enc );
	}
	if ( (*name=='U' || *name=='u') && name[1]=='+' ) {
	    uni = strtol(name+2,&end,16);
	    if ( *end!='\0' )
		uni = -1;
	} else if ( name[0]=='u' && name[1]=='n' && name[2]=='i' ) {
	    uni = strtol(name+3,&end,16);
	    if ( *end!='\0' )
		uni = -1;
	} else if ( name[0]=='g' && name[1]=='l' && name[2]=='y' && name[3]=='p' && name[4]=='h' ) {
	    int orig = strtol(name+5,&end,10);
	    if ( *end!='\0' )
		orig = -1;
	    if ( orig!=-1 )
		enc = map->backmap[orig];
	} else if ( isdigit(*name)) {
	    enc = strtoul(name,&end,0);
	    if ( *end!='\0' )
		enc = -1;
	    if ( map->remap!=NULL && enc!=-1 ) {
		struct remap *remap = map->remap;
		while ( remap->infont!=-1 ) {
		    if ( enc>=remap->firstenc && enc<=remap->lastenc ) {
			enc += remap->infont-remap->firstenc;
		break;
		    }
		    ++remap;
		}
	    }
	} else {
	    if ( enc==-1 ) {
		uni = UniFromName(name,sf->uni_interp,map->enc);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
		if ( uni<0 ) {
		    for ( i=0; specialnames[i].name!=NULL; ++i )
			if ( strcmp(name,specialnames[i].name)==0 ) {
			    uni = specialnames[i].first;
		    break;
			}
		}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
		if ( uni<0 && name[1]=='\0' )
		    uni = name[0];
	    }
	}
	if ( enc>=map->enccount || enc<0 )
	    enc = -1;
	if ( enc==-1 && uni!=-1 )
	    enc = SFFindSlot(sf,map,uni,NULL);
	if ( enc!=-1 && uni==-1 ) {
	    int gid = map->map[enc];
	    if ( gid!=-1 && sf->glyphs[gid]!=NULL )
		uni = sf->glyphs[gid]->unicodeenc;
	    else if ( map->enc->is_unicodebmp || map->enc->is_unicodefull )
		uni = enc;
	}
	if ( dot!=NULL ) {
	    free(freeme);
	    if ( uni==-1 )
return( -1 );
	    if ( uni<0x600 || uni>0x6ff )
return( -1 );
	    if ( strmatch(dot,".begin")==0 || strmatch(dot,".initial")==0 )
		uni = ArabicForms[uni-0x600].initial;
	    else if ( strmatch(dot,".end")==0 || strmatch(dot,".final")==0 )
		uni = ArabicForms[uni-0x600].final;
	    else if ( strmatch(dot,".medial")==0 )
		uni = ArabicForms[uni-0x600].medial;
	    else if ( strmatch(dot,".isolated")==0 )
		uni = ArabicForms[uni-0x600].isolated;
	    else
return( -1 );
return( SFFindSlot(sf,map,uni,NULL) );
	} else 	if ( enc!=-1 ) {
	    free(freeme);
return( enc );
	}
	dot = strrchr(name,'.');
	if ( dot==NULL )
return( -1 );
	free(freeme);
	name = freeme = copyn(name,dot-name);
    }
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
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
	for ( i=0; d->ranges[i].text!=NULL; ++i ) {
	    if ( strcmp(ret,(char *) d->ranges[i].text)==0 ) {
		d->ret = (intpt) d->ranges[i].userdata;
	break;
	    }
	}
	if ( d->ret==-1 ) {
	    d->ret = NameToEncoding(d->sf,d->map,ret);
	    if ( d->ret<0 || d->ret>=d->map->enccount )
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

int GotoChar(SplineFont *sf,EncMap *map) {
    int enc = -1;
    char *ret;

    if ( map->enc->only_1byte ) {
	/* In one byte encodings don't bother with the range list. It won't */
	/*  have enough entries to be useful */
	ret = gwwv_ask_string(_("Goto"),NULL,_("Enter the name of a glyph in the font"));
	if ( ret==NULL )
return(-1);
	enc = NameToEncoding(sf,map,ret);
	if ( enc<0 || enc>=map->enccount )
	    enc = -1;
	if ( enc==-1 )
	    ff_post_notice(_("Goto"),_("Could not find the glyph: %.70s"),ret);
	free(ret);
return( enc );
    } else {
	GRect pos;
	GWindow gw;
	GWindowAttrs wattrs;
	GGadgetCreateData gcd[9];
	GTextInfo label[9];
	static GotoData gd;
	GTextInfo *ranges = AvailableRanges(sf,map);
	int i, wid, temp;
	unichar_t ubuf[100];

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

	GDrawSetFont(gw,GGadgetGetFont(NULL));		/* Default gadget font */
	wid = GDrawGetText8Width(gw,_("Enter the name of a glyph in the font"),-1,NULL);
	for ( i=0; ranges[i].text!=NULL; ++i ) {
	    uc_strncpy(ubuf,(char *) (ranges[i].text),sizeof(ubuf)/sizeof(ubuf[0])-1);
	    temp = GDrawGetTextWidth(gw,ubuf,-1,NULL);
	    if ( temp>wid )
		wid = temp;
	}
	if ( pos.width<wid+20 ) {
	    pos.width = wid+20;
	    GDrawResize(gw,pos.width,pos.height);
	}

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	label[0].text = (unichar_t *) _("Enter the name of a glyph in the font");
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5; 
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].creator = GLabelCreate;

	gcd[1].gd.pos.x = 5; gcd[1].gd.pos.y = 17;  gcd[1].gd.pos.width = GDrawPixelsToPoints(NULL,wid);
	gcd[1].gd.flags = gg_enabled|gg_visible|gg_text_xim;
	gcd[1].gd.cid = CID_Name;
	gcd[1].gd.u.list = ranges;
	gcd[1].creator = GListFieldCreate;

	gcd[2].gd.pos.x = 20-3; gcd[2].gd.pos.y = 90-35-3;
	gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
	gcd[2].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[2].text = (unichar_t *) _("_OK");
	label[2].text_is_1byte = true;
	label[2].text_in_resource = true;
	gcd[2].gd.mnemonic = 'O';
	gcd[2].gd.label = &label[2];
	gcd[2].gd.handle_controlevent = Goto_OK;
	gcd[2].creator = GButtonCreate;

	gcd[3].gd.pos.x = -20; gcd[3].gd.pos.y = 90-35;
	gcd[3].gd.pos.width = -1; gcd[3].gd.pos.height = 0;
	gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[3].text = (unichar_t *) _("_Cancel");
	label[3].text_is_1byte = true;
	label[3].text_in_resource = true;
	gcd[3].gd.label = &label[3];
	gcd[3].gd.mnemonic = 'C';
	gcd[3].gd.handle_controlevent = Goto_Cancel;
	gcd[3].creator = GButtonCreate;

	gcd[4].gd.pos.x = 2; gcd[4].gd.pos.y = 2;
	gcd[4].gd.pos.width = pos.width-4; gcd[4].gd.pos.height = pos.height-2;
	gcd[4].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
	gcd[4].creator = GGroupCreate;

	GGadgetsCreate(gw,gcd);
	GDrawSetVisible(gw,true);
	while ( !gd.done )
	    GDrawProcessOneEvent(NULL);
	GDrawDestroyWindow(gw);
	free(ranges);
return( gd.ret );
    }
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
