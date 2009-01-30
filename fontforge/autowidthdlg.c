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
#include <math.h>
#include <ustring.h>
#include <utype.h>
#include <gkeysym.h>

#include "autowidth.h"

static GTextInfo widthlist[] = {
    { (unichar_t *) N_("A-Za-z0-9"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
/* gettext gets very unhappy with things outside of ASCII and removes them */
    { (unichar_t *) NU_("Α-ΡΣ-Ωα-ω"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) NU_("ЂЄ-ІЈ-ЋЏ-ИК-Яа-ик-яђє-іј-ћџ"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("All"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Selected"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    NULL };
static GTextInfo kernllist[] = {
    { (unichar_t *) N_("A-Za-z"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) NU_("Α-ΡΣ-Ωα-ω"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) NU_("ЂЄ-ІЈ-ЋЏ-ИК-Яа-ик-яђє-іј-ћџ"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("All"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Selected"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    NULL };
static GTextInfo kernrlist[] = {
    { (unichar_t *) N_("a-z.,:;-"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("A-Za-z.,:;-"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) NU_("α-ω.,:;-"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) NU_("Α-ΡΣ-Ωα-ω.,:;-"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) NU_("а-ик-яђє-іј-ћџ.,:;-"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) NU_("ЂЄ-ІЈ-ЋЏ-ИК-Яа-ик-яђє-іј-ћџ.,:;-"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("All"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Selected"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    NULL };

static void AutoWidthInit(void) {
    static int done=false;
    int i;

    if ( done )
return;
    for ( i=0; widthlist[i].text!=NULL; ++i )
	widthlist[i].text = (unichar_t *) _((char *) widthlist[i].text);
    for ( i=0; kernllist[i].text!=NULL; ++i )
	kernllist[i].text = (unichar_t *) _((char *) kernllist[i].text);
    for ( i=0; kernrlist[i].text!=NULL; ++i )
	kernrlist[i].text = (unichar_t *) _((char *) kernrlist[i].text);
    done = true;
}

#define CID_Spacing	1001
#define CID_Total	1002
#define CID_Threshold	1003
#define CID_OnlyNeg	1004
#define CID_Left	1010
#define CID_Right	1020
#define CID_Subtable	1030
#define CID_Browse	2001
#define CID_OK		2002

static void ReplaceGlyphWith(SplineFont *sf, struct charone **ret, int cnt, int ch1, int ch2 ) {
    int s,e,j;

    for ( s=0; s<cnt; ++s )
	if ( ret[s]->sc->unicodeenc==ch1 )
    break;
    if ( s!=cnt && ( j=SFFindExistingSlot(sf,ch2,NULL))!=-1 &&
	    sf->glyphs[j]->width==ret[s]->sc->width &&	/* without this, they won't sync up */
	    ret[s]->sc->layers[ly_fore].refs!=NULL &&
		    (ret[s]->sc->layers[ly_fore].refs->sc->unicodeenc==ch2 ||
		     (ret[s]->sc->layers[ly_fore].refs->next!=NULL &&
			 ret[s]->sc->layers[ly_fore].refs->next->sc->unicodeenc==ch2)) ) {
	for ( e=0; e<cnt; ++e )
	    if ( ret[e]->sc->unicodeenc==ch2 )
	break;
	if ( e==cnt )
	    ret[s]->sc = sf->glyphs[j];
    }
}

static struct charone **BuildCharList(FontView *fv, SplineFont *sf,GWindow gw,
	int base, int *tot, int *rtot, int *ipos, int iswidth) {
    int i, cnt, rcnt=0, doit, s, e;
    struct charone **ret=NULL;
    int all, sel;
    const unichar_t *str, *pt;
    int gid;

    str = _GGadgetGetTitle(GWidgetGetControl(gw,base));
    all = uc_strcmp(str,_("All"))==0;
    sel = uc_strcmp(str,_("Selected"))==0;

    for ( doit=0; doit<2; ++doit ) {
	if ( all ) {
	    for ( i=cnt=0; i<sf->glyphcnt && cnt<300; ++i ) {
		if ( SCWorthOutputting(sf->glyphs[i]) ) {
		    if ( doit )
			ret[cnt++] = AW_MakeCharOne(sf->glyphs[i]);
		    else
			++cnt;
		}
	    }
	} else if ( sel ) {
	    EncMap *map = fv->b.map;
	    for ( i=cnt=0; i<map->enccount && cnt<300; ++i ) {
		if ( fv->b.selected[i] && (gid=map->map[i])!=-1 && SCWorthOutputting(sf->glyphs[gid]) ) {
		    if ( doit )
			ret[cnt++] = AW_MakeCharOne(sf->glyphs[i]);
		    else
			++cnt;
		}
	    }
	} else {
	    for ( pt=str, cnt=0; *pt && cnt<300 ; ) {
		if ( pt[1]=='-' && pt[2]!='\0' ) {
		    s = pt[0]; e = pt[2];
		    pt += 3;
		} else {
		    s = e = pt[0];
		    ++pt;
		}
		for ( ; s<=e && cnt<300 ; ++s ) {
		    i = SFFindExistingSlot(sf,s,NULL);
		    if ( i!=-1 && sf->glyphs[i]!=NULL &&
			    (sf->glyphs[i]->layers[ly_fore].splines!=NULL || sf->glyphs[i]->layers[ly_fore].refs!=NULL )) {
			if ( doit )
			    ret[cnt++] = AW_MakeCharOne(sf->glyphs[i]);
			else
			    ++cnt;
		    }
		}
	    }
	}
	if ( cnt==0 )
    break;
	if ( !doit )
	    ret = galloc((cnt+2)*sizeof(struct charone *));
	else {
	    rcnt = cnt;
	    /* If lower case i is used, and it's a composite, then use */
	    /*  dotlessi instead */ /* could do the same for dotless j */
	    if ( iswidth ) {
		ReplaceGlyphWith(sf,ret,cnt,'i',0x131);
		ReplaceGlyphWith(sf,ret,cnt,'j',0x237);
	    }

	    if ( iswidth &&		/* I always want 'I' in the character list when doing widths */
					/*  or at least when doing widths of LGC alphabets where */
			                /*  concepts like serifs make sense */
		    (( ret[0]->sc->unicodeenc>='A' && ret[0]->sc->unicodeenc<0x530) ||
		     ( ret[0]->sc->unicodeenc>=0x1d00 && ret[0]->sc->unicodeenc<0x2000)) ) {

		for ( s=0; s<cnt; ++s )
		    if ( ret[s]->sc->unicodeenc=='I' )
		break;
		if ( s==cnt ) {
		    i = SFFindExistingSlot(sf,'I',NULL);
		    if ( i!=-1 && sf->glyphs[i]!=NULL &&
			    (sf->glyphs[i]->layers[ly_fore].splines!=NULL || sf->glyphs[i]->layers[ly_fore].refs!=NULL ))
			ret[cnt++] = AW_MakeCharOne(sf->glyphs[i]);
		    else
			s = -1;
		}
		*ipos = s;
	    }
	    ret[cnt] = NULL;
	}
    }
    *tot = cnt;
    *rtot = rcnt;
return( ret );
}

static int AW_Subtable(GGadget *g, GEvent *e) {
    WidthInfo *wi = GDrawGetUserData(GGadgetGetWindow(g));
    GTextInfo *ti;
    struct lookup_subtable *sub;
    struct subtable_data sd;

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	ti = GGadgetGetListItemSelected(g);
	if ( ti!=NULL ) {
	    if ( ti->userdata!=NULL )
		wi->subtable = ti->userdata;
	    else {
		memset(&sd,0,sizeof(sd));
		sd.flags = sdf_horizontalkern | sdf_kernpair;
		sub = SFNewLookupSubtableOfType(wi->sf,gpos_pair,&sd,wi->layer);
		if ( sub!=NULL ) {
		    wi->subtable = sub;
		    GGadgetSetList(g,SFSubtablesOfType(wi->sf,gpos_pair,false,false),false);
		}
	    }
	}
    }
return( true );
}

static int AW_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	WidthInfo *wi = GDrawGetUserData(gw);
	int err = false;
	int tot;

	wi->spacing = GetReal8(gw,CID_Spacing, _("Spacing"),&err);
	if ( wi->autokern ) {
	    wi->threshold = GetInt8(gw,CID_Threshold, _("Threshold:"), &err);
	    wi->onlynegkerns = GGadgetIsChecked(GWidgetGetControl(gw,CID_OnlyNeg));
	    tot = GetInt8(gw,CID_Total, _("Total Kerns:"), &err);
	    if ( tot<0 ) tot = 0;
	}
	if ( err )
return( true );
	if ( wi->autokern && wi->subtable==NULL ) {
	    ff_post_error(_("Select a subtable"),_("You must select a lookup subtable in which to store the kerning pairs"));
return( true );
	}

	aw_old_sf = wi->sf;
	aw_old_spaceguess = wi->spacing;

	wi->done = true;
	GDrawSetVisible(gw,false);
	GDrawSync(NULL);
	GDrawProcessPendingEvents(NULL);

	if ( GGadgetGetCid(g)==CID_OK ) {
	    wi->left = BuildCharList((FontView *) wi->fv, wi->sf,gw,CID_Left, &wi->lcnt, &wi->real_lcnt, &wi->l_Ipos, !wi->autokern );
	    wi->right = BuildCharList((FontView *) wi->fv, wi->sf,gw,CID_Right, &wi->rcnt, &wi->real_rcnt, &wi->r_Ipos, !wi->autokern );
	    if ( wi->real_lcnt==0 || wi->real_rcnt==0 ) {
		AW_FreeCharList(wi->left);
		AW_FreeCharList(wi->right);
		ff_post_error(_("No glyphs selected."),_("No glyphs selected."));
return( true );
	    }
	    AW_ScriptSerifChecker(wi);
	    AW_InitCharPairs(wi);
	} else {
	    char *fn = gwwv_open_filename(_("Load Kern Pairs"), NULL, "*.txt", NULL);
	    if ( fn==NULL ) {
		GDrawSetVisible(gw,true);
		wi->done = false;
return( true );
	    }
	    if ( !AW_ReadKernPairFile(fn,wi)) {
		GDrawSetVisible(gw,true);
		wi->done = false;
return( true );
	    }
	}
	AW_BuildCharPairs(wi);
	if ( wi->autokern ) {
	    AW_AutoKern(wi);
	    AW_KernRemoveBelowThreshold(wi->sf,KernThreshold(wi->sf,tot));
	} else
	    AW_AutoWidth(wi);
	AW_FreeCharList(wi->left);
	AW_FreeCharList(wi->right);
	AW_FreeCharPairs(wi->pairs,wi->pcnt);
    }
return( true );
}

static int AW_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	WidthInfo *wi = GDrawGetUserData(gw);
	wi->done = true;
    }
return( true );
}

static int AW_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	WidthInfo *wi = GDrawGetUserData(gw);
	wi->done = true;
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    WidthInfo *wi = GDrawGetUserData(gw);
	    help(wi->autokern?"autowidth.html#AutoKern":"autowidth.html#AutoWidth");
return( true );
	}
return( false );
    }
return( true );
}

#define SelHeight	34
static int MakeSelGadgets(GGadgetCreateData *gcd, GTextInfo *label,
	int i, int base, char *labr, int y, int toomany, int autokern,
	GGadgetCreateData **hvarray) {
    char *std = !autokern ? _("A-Za-z0-9") :
		base==CID_Left ? _("A-Za-z") :
		_("a-z.,:;-");
    int epos;

    label[i].text = (unichar_t *) labr;
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 12; gcd[i].gd.pos.y = y; 
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i++].creator = GLabelCreate;
    hvarray[0] = &gcd[i-1];

    label[i].text = (unichar_t *) std;
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 12; gcd[i].gd.pos.y = y+14; 
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i].gd.u.list = !autokern ? widthlist : base==CID_Left ? kernllist : kernrlist;
    gcd[i].gd.cid = base;
    gcd[i++].creator = GListFieldCreate;
    hvarray[1] = &gcd[i-1]; hvarray[2] = NULL;

    for ( epos = 0; gcd[i-1].gd.u.list[epos].text!=NULL; ++epos );
    gcd[i-1].gd.u.list[epos-2].disabled = (toomany&1);
    gcd[i-1].gd.u.list[epos-1].disabled = (toomany&2)?1:0;
return( i );
}

static int SFCount(SplineFont *sf) {
    int i, cnt;

    for ( i=cnt=0; i<sf->glyphcnt; ++i )
	if ( SCWorthOutputting(sf->glyphs[i]) )
	    ++cnt;
return( cnt );
}

static int SFCountSel(FontView *fv, SplineFont *sf) {
    int i, cnt;
    uint8 *sel = fv->b.selected;
    EncMap *map = fv->b.map;

    for ( i=cnt=0; i<map->enccount; ++i ) if ( sel[i] ) {
	int gid = map->map[i];
	if ( gid!=-1 && SCWorthOutputting(sf->glyphs[gid]))
	    ++cnt;
    }
return( cnt );
}

static void AutoWKDlg(FontView *fv,int autokern) {
    WidthInfo wi;
    GWindow gw;
    GWindowAttrs wattrs;
    GRect pos;
    GGadgetCreateData *hvarray[24], *varray[10], *h3array[8];
    GGadgetCreateData gcd[29];
    GTextInfo label[29];
    int i, y, selfield, v;
    char buffer[30], buffer2[30];
    SplineFont *sf = fv->b.sf;
    int selcnt = SFCountSel(fv,sf);
    int toomany = ((SFCount(sf)>=300)?1:0) | ((selcnt==0 || selcnt>=300)?2:0);

    memset(&wi,'\0',sizeof(wi));
    wi.autokern = autokern;
    wi.layer = fv->b.active_layer;
    wi.sf = sf;
    wi.fv = (FontViewBase *) fv;
    AW_FindFontParameters(&wi);
    AutoWidthInit();

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = autokern?_("Auto Kern"):_("Auto Width");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,200));
    pos.height = GDrawPointsToPixels(NULL,autokern?270:180);
    gw = GDrawCreateTopWindow(NULL,&pos,AW_e_h,&wi,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    i = 0;

    label[i].text = (unichar_t *) _("Enter two glyph ranges to be adjusted.");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 6;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i++].creator = GLabelCreate;
    varray[0] = &gcd[i-1]; varray[1] = NULL;

    i = MakeSelGadgets(gcd, label, i, CID_Left, _("Glyphs on Left"), 21,
	    toomany, autokern, hvarray );
    varray[2] = &gcd[i-1]; varray[3] = NULL;
    selfield = i-1;
    i = MakeSelGadgets(gcd, label, i, CID_Right, _("Glyphs on Right"), 21+SelHeight+9,
	    toomany, autokern, hvarray+3 );
    y = 32+2*(SelHeight+9);

    label[i].text = (unichar_t *) _("Spacing");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = y+7;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i++].creator = GLabelCreate;
    hvarray[6] = &gcd[i-1];

    sprintf( buffer, "%d", wi.space_guess );
    label[i].text = (unichar_t *) buffer;
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 65; gcd[i].gd.pos.y = y+3;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i].gd.cid = CID_Spacing;
    gcd[i++].creator = GTextFieldCreate;
    hvarray[7] = &gcd[i-1]; hvarray[8] = NULL;
    y += 32;

    if ( autokern ) {
	y -= 4;

	label[i].text = (unichar_t *) _("Total Kerns:");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = y+7;
	gcd[i].gd.flags = gg_visible | gg_enabled;
	gcd[i++].creator = GLabelCreate;
	hvarray[9] = &gcd[i-1];

	label[i].text = (unichar_t *) "2048";
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 65; gcd[i].gd.pos.y = y+3;
	gcd[i].gd.flags = gg_visible | gg_enabled;
	gcd[i].gd.cid = CID_Total;
	gcd[i++].creator = GTextFieldCreate;
	hvarray[10] = &gcd[i-1]; hvarray[11] = NULL;
	y += 28;

	label[i].text = (unichar_t *) _("Threshold:");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = y+7;
	gcd[i].gd.flags = gg_visible | gg_enabled;
	gcd[i++].creator = GLabelCreate;
	hvarray[12] = &gcd[i-1];

	sprintf( buffer2, "%d", (sf->ascent+sf->descent)/25 );
	label[i].text = (unichar_t *) buffer2;
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 65; gcd[i].gd.pos.y = y+3;
	gcd[i].gd.flags = gg_visible | gg_enabled;
	gcd[i].gd.cid = CID_Threshold;
	gcd[i++].creator = GTextFieldCreate;
	hvarray[13] = &gcd[i-1]; hvarray[14] = NULL;
	y += 32;

	gcd[i].gd.flags = gg_enabled | gg_visible | gg_cb_on;
	label[i].text = (unichar_t *) _("Only kern glyphs closer");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.cid = CID_OnlyNeg;
	gcd[i++].creator = GCheckBoxCreate;
	hvarray[15] = GCD_Glue; hvarray[16] = &gcd[i-1]; hvarray[17] = NULL;

	gcd[i].gd.flags = gg_enabled | gg_visible;
	label[i].text = (unichar_t *) _("Lookup subtable:");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i++].creator = GLabelCreate;
	hvarray[18] = &gcd[i-1];

	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.cid = CID_Subtable;
	gcd[i].gd.handle_controlevent = AW_Subtable;
	gcd[i++].creator = GListButtonCreate;
	hvarray[19] = &gcd[i-1]; hvarray[20] = NULL; hvarray[21] = NULL;
    } else
	hvarray[9] = NULL;

    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.u.boxelements = hvarray;
    gcd[i++].creator = GHVBoxCreate;
    varray[2] = &gcd[i-1]; varray[3] = NULL;

    v = 4;
    varray[v++] = GCD_Glue; varray[v++] = NULL;

    gcd[i].gd.pos.x = 30-3; gcd[i].gd.pos.y = y-3;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _("_OK");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.mnemonic = 'O';
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = AW_OK;
    gcd[i].gd.cid = CID_OK;
    gcd[i++].creator = GButtonCreate;
    h3array[0] = GCD_Glue; h3array[1] = &gcd[i-1]; h3array[2] = GCD_Glue;

    if ( autokern ) {
	gcd[i].gd.pos.x = (200-gcd[i].gd.pos.width)/2; gcd[i].gd.pos.y = y;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	label[i].text = (unichar_t *) _("_Browse...");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.handle_controlevent = AW_OK;	/* Yes, really */
	gcd[i].gd.popup_msg = (unichar_t *) _("Browse for a file containing a list of kerning pairs\ntwo characters per line. FontForge will only check\nthose pairs for kerning info.");
	gcd[i].gd.cid = CID_Browse;
	gcd[i++].creator = GButtonCreate;
	h3array[3] = &gcd[i-1]; h3array[4] = GCD_Glue;
    } else
	h3array[3] = h3array[4] = GCD_Glue;

    gcd[i].gd.pos.x = -30; gcd[i].gd.pos.y = y;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _("_Cancel");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.mnemonic = 'C';
    gcd[i].gd.handle_controlevent = AW_Cancel;
    gcd[i++].creator = GButtonCreate;
    h3array[5] = &gcd[i-1]; h3array[6] = GCD_Glue; h3array[7] = NULL;

    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.u.boxelements = h3array;
    gcd[i++].creator = GHBoxCreate;
    varray[v++] = &gcd[i-1]; varray[v++] = NULL; varray[v++] = NULL;

    gcd[i].gd.pos.x = gcd[i].gd.pos.y = 2;
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.u.boxelements = varray;
    gcd[i].creator = GHVGroupCreate;

    GGadgetsCreate(gw,gcd+i);
    GHVBoxSetExpandableRow(gcd[i].ret,gb_expandglue);
    GHVBoxSetExpandableCol(gcd[i-1].ret,gb_expandgluesame);
    GHVBoxSetExpandableCol(varray[2]->ret,1);
    if ( autokern ) {
	GTextInfo *ti;
	GGadgetSetList(hvarray[19]->ret,SFSubtablesOfType(sf,gpos_pair,false,false),false);
	ti = GGadgetGetListItemSelected(hvarray[19]->ret);
	if ( ti==NULL ) {
	    int32 len,j;
	    GTextInfo **list = GGadgetGetList(hvarray[19]->ret,&len);
	    for ( j=0; j<len; ++j )
		if ( !list[j]->disabled && !list[j]->line ) {
		    ti = list[j];
		    GGadgetSelectOneListItem(hvarray[19]->ret,j);
	    break;
		}
	}
	if ( ti!=NULL )
	    wi.subtable = ti->userdata;
    }
    GHVBoxFitWindow(gcd[i].ret);

    GWidgetIndicateFocusGadget(gcd[selfield].ret);
    GTextFieldSelect(gcd[selfield].ret,0,-1);
    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !wi.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

void FVAutoKern(FontView *fv) {
    AutoWKDlg(fv,true);
}

void FVAutoWidth(FontView *fv) {
    AutoWKDlg(fv,false);
}
