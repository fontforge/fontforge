/* Copyright (C) 2000,2001 by George Williams */
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
#include "ustring.h"
#include <math.h>
#include "utype.h"

#define TCnt	3

typedef struct gidata {
    CharView *cv;
    SplineChar *sc;
    RefChar *rf;
    ImageList *img;
    SplinePoint *cursp;
    SplinePointList *curspl;
    SplinePointList *oldstate;
    GWindow gw;
    int done;
} GIData;

#define CID_UName	1001
#define CID_UValue	1002
#define CID_UChar	1003
#define CID_Ligature	1004
#define CID_Cancel	1005

#define CID_BaseX	2001
#define CID_BaseY	2002
#define CID_NextXOff	2003
#define CID_NextYOff	2004
#define CID_NextPos	2005
#define CID_PrevXOff	2006
#define CID_PrevYOff	2007
#define CID_PrevPos	2008

#define CI_Width	200
#define CI_Height	219

#define RI_Width	200
#define RI_Height	110

#define II_Width	130
#define II_Height	70

#define PI_Width	184
#define PI_Height	200

static int MultipleValues(char *name, int local) {
    unichar_t ubuf[200]; char buffer[10];
    const unichar_t *buts[3]; unichar_t ocmn[2];

    buts[2]=NULL;
    buts[0] = GStringGetResource( _STR_OK, &ocmn[0]);
    buts[1] = GStringGetResource( _STR_Cancel, &ocmn[1]);

    u_strcpy(ubuf,GStringGetResource( _STR_Alreadycharpre,NULL ));
    uc_strncat(ubuf,name,10);
    u_strcat(ubuf,GStringGetResource( _STR_Alreadycharmid,NULL ));
    sprintf( buffer, "%d", local );
    uc_strcat(ubuf,buffer);
    u_strcat(ubuf,GStringGetResource( _STR_Alreadycharpost,NULL ));
    if ( GWidgetAsk(GStringGetResource(_STR_Multiple,NULL),buts,ocmn,0,1,ubuf)==0 )
return( true );

return( false );
}

static int MultipleNames(void) {
    int buts[] = { _STR_OK, _STR_Cancel, 0 };

    if ( GWidgetAskR(_STR_Multiple,buts,0,1,_STR_Alreadycharnamed)==0 )
return( true );

return( false );
}

static int ParseUValue(GWindow gw, int cid, int minusoneok) {
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    unichar_t *end;
    int val;

    if (( *ret=='U' || *ret=='u' ) && ret[1]=='+' )
	val = u_strtol(ret+2,&end,16);
    else if ( *ret=='#' )
	val = u_strtol(ret+1,&end,16);
    else
	val = u_strtol(ret,&end,16);
    if ( val==-1 && minusoneok )
return( -1 );
    if ( *end || val<0 || val>65535 ) {
	Protest( "Unicode Value" );
return( -2 );
    }
return( val );
}

static void SetNameFromUnicode(GWindow gw,int cid,int val) {
    unichar_t *temp;
    char buf[10];

    if ( val>=0 && val<psunicodenames_cnt && psunicodenames[val]!=NULL )
	temp = uc_copy(psunicodenames[val]);
    else if ( val==0x2d )
	temp = uc_copy("hyphen-minus");
    else if ( val==0xa0 )
	temp = uc_copy("nonbreakingspace");
    else if (( val>=32 && val<0x7f ) || val>=0xa1 ) {
	sprintf( buf,"uni%04X", val );
	temp = uc_copy(buf);
    } else
	temp = uc_copy(".notdef");
    GGadgetSetTitle(GWidgetGetControl(gw,cid),temp);
    free(temp);
}

static int LigCheck(SplineFont *sf,SplineChar *sc,char *componants) {
    int i;
    unichar_t ubuf[200]; char buffer[10];
    const unichar_t *buts[3]; unichar_t ocmn[2];
    char *pt, *spt, *start, ch;

    if ( componants==NULL || *componants=='\0' )
return( true );

    buts[2]=NULL;
    buts[0] = GStringGetResource( _STR_OK, &ocmn[0]);
    buts[1] = GStringGetResource( _STR_Cancel, &ocmn[1]);

    for ( i=0; i<sf->charcnt; ++i )
	if ( sf->chars[i]!=sc && sf->chars[i]!=NULL && sf->chars[i]->lig!=NULL ) {
	    if ( strcmp(componants,sf->chars[i]->lig->componants)==0 ) {
		u_strcpy(ubuf,GStringGetResource( _STR_Alreadyligpre,NULL ));
		uc_strncat(ubuf,sf->chars[i]->name,10);
		u_strcat(ubuf,GStringGetResource( _STR_Alreadyligmid,NULL ));
		sprintf( buffer, "%d", i );
		uc_strcat(ubuf,buffer);
		u_strcat(ubuf,GStringGetResource( _STR_Alreadyligpost,NULL ));
return( GWidgetAsk(GStringGetResource(_STR_Multiple,NULL),buts,ocmn,0,1,ubuf)==0 );
	    }
	}

    start = componants;
    while ( *start!='\0' ) {
	pt = strchr(start,' ');
	spt = strchr(start,';');
	if ( pt==NULL ) pt = start+strlen(start);
	if ( spt==NULL ) spt = start+strlen(start);
	if ( pt>spt ) pt = spt;
	ch = *pt; *pt = '\0';
	if ( strcmp(start,sc->name)==0 ) {
	    GWidgetErrorR(_STR_Badligature,_STR_Badligature );
	    *pt = ch;
return( false );
	}
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	    if ( strcmp(start,sf->chars[i]->name)==0 )
	break;
	if ( i==sf->charcnt ) {
	    u_strcpy(ubuf,GStringGetResource( _STR_Missingcomponantpre,NULL ));
	    uc_strncat(ubuf,start,20);
	    u_strcat(ubuf,GStringGetResource( _STR_Missingcomponantpost,NULL ));
	    *pt = ch;
return( GWidgetAsk(GStringGetResource(_STR_Multiple,NULL),buts,ocmn,0,1,ubuf)==0 );
	}
	*pt = ch;
	if ( ch=='\0' )
    break;
	start = pt+1;
	while ( *start==' ' || *start==';' ) ++start;
    }
return( true );
}
		
static void LigSet(SplineChar *sc,char *lig) {

    if ( lig==NULL || *lig=='\0' ) {
	LigatureFree(sc->lig);
	sc->lig=NULL;
    } else {
	LigatureFree(sc->lig);
	sc->lig = gcalloc(1,sizeof(Ligature));
	sc->lig->componants = copy(lig);
	sc->lig->lig = sc;
    }
}

static int _CI_OK(GIData *ci) {
    const unichar_t *ret;
    int val, i, mv=0;
    SplineFont *sf = ci->sc->parent;
    int isnotdef;
    char *lig;

    val = ParseUValue(ci->gw,CID_UValue,true);
    if ( val==-2 )
return( false );
    lig = cu_copy( _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_Ligature)) );
    if ( !LigCheck(sf,ci->sc,lig)) {
	free(lig);
return( false );
    }
    ret = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UName));
    if ( ci->sc->unicodeenc == val && uc_strcmp(ret,ci->sc->name)==0 ) {
	/* No change, it must be good */
	LigSet(ci->sc,lig);
return( true );
    }
    isnotdef = uc_strcmp(ret,".notdef")==0;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]!=ci->sc ) {
	if ( val!=-1 && sf->chars[i]->unicodeenc==val ) {
	    if ( !mv && !MultipleValues(sf->chars[i]->name,i)) {
		free(lig);
return( false );
	    }
	    mv = 1;
	} else if ( !isnotdef && uc_strcmp(ret,sf->chars[i]->name)==0 ) {
	    if ( !MultipleNames()) {
		free(lig);
return( false );
	    }
	    free(sf->chars[i]->name);
	    sf->chars[i]->name = ci->sc->name;
	    ci->sc->name = NULL;
	break;
	}
    }
    ci->sc->unicodeenc = val;
    free(ci->sc->name);
    ci->sc->name = cu_copy(ret);
    sf->changed = true;
    if ( sf->encoding_name==em_unicode && val==ci->sc->enc &&
	    val>=0xe000 && val<=0xf8ff )
	/* Ok to name things in the private use area */;
    else if ( (sf->encoding_name<e_first2byte && ci->sc->enc<256) ||
	    (sf->encoding_name==em_unicode && ci->sc->enc<65536 ) ||
	    (sf->encoding_name>=e_first2byte && sf->encoding_name!=em_unicode && ci->sc->enc<94*96 ) ||
	    ci->sc->unicodeenc!=-1 )
	sf->encoding_name = em_none;
    LigSet(ci->sc,lig);
return( true );
}

static int CI_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	if ( _CI_OK(ci) )
	    ci->done = true;
    }
return( true );
}

static int CI_SName(GGadget *g, GEvent *e) {	/* Set From Name */
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UName));
	int i;
	char buf[10]; unichar_t ubuf[2], *temp;
	for ( i=psunicodenames_cnt-1; i>=0; --i )
	    if ( psunicodenames[i]!=NULL && uc_strcmp(ret,psunicodenames[i])==0 )
	break;
	if ( i==-1 ) {
	    if ( ret[0]=='u' && ret[1]=='n' && ret[2]=='i' ) {
		unichar_t *end;
		i = u_strtol(ret+3,&end,16);
		if ( *end )
		    i = -1;
		else		/* Make sure it is properly capitalized */
		    SetNameFromUnicode(ci->gw,CID_UName,i);
	    }
	}
	if ( i==-1 ) {
	    for ( i=65535; i>=0; --i )
		if ( UnicodeCharacterNames[i>>8][i&0xff]!=NULL &&
			u_strcmp(ret,UnicodeCharacterNames[i>>8][i&0xff])==0 )
	    break;
	}

	sprintf(buf,"U+%04x", i);
	temp = uc_copy(i==-1?"-1":buf);
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UValue),temp);
	free(temp);

	ubuf[0] = i;
	if ( i==-1 )
	    ubuf[0] = '\0';
	ubuf[1] = '\0';
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);
    }
return( true );
}

static int CI_SValue(GGadget *g, GEvent *e) {	/* Set From Value */
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	unichar_t ubuf[2];
	int val;

	val = ParseUValue(ci->gw,CID_UValue,false);
	if ( val<0 )
return( true );

	SetNameFromUnicode(ci->gw,CID_UName,val);

	ubuf[0] = val;
	if ( val==-1 )
	    ubuf[0] = '\0';
	ubuf[1] = '\0';
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);
    }
return( true );
}

static int CI_CharChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UChar));
	int val = *ret;
	unichar_t *temp, ubuf[1]; char buf[10];

	if ( ret[1]!='\0' ) {
	    temp = uc_copy("Only a single character allowed");
	    GWidgetPostNotice(temp,temp);
	    free(temp);
	    ubuf[0] = '\0';
	    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);
return( true );
	}

	SetNameFromUnicode(ci->gw,CID_UName,val);

	sprintf(buf,"U+%04x", val);
	temp = uc_copy(buf);
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UValue),temp);
	free(temp);
    }
return( true );
}

static void CIFillup(GIData *ci) {
    SplineChar *sc = ci->sc;
    SplineFont *sf = sc->parent;
    unichar_t *temp;
    char buffer[10];
    unichar_t ubuf[2];

    GGadgetSetEnabled(GWidgetGetControl(ci->gw,-1),sc->enc>0);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,1),sc->enc<sf->charcnt-1);

    temp = uc_copy(sc->name);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UName),temp);
    free(temp);

    sprintf(buffer,"U+%04x", sc->unicodeenc);
    temp = uc_copy(sc->unicodeenc==-1?"-1":buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UValue),temp);
    free(temp);

    ubuf[0] = sc->unicodeenc;
    if ( sc->unicodeenc==-1 )
	ubuf[0] = '\0';
    ubuf[1] = '\0';
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);

    if ( sc->lig!=NULL )
	temp = uc_copy(sc->lig->componants);
    else
	temp = uc_copy("");
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_Ligature),temp);
    free(temp);
}

static int CI_NextPrev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int enc = ci->sc->enc + GGadgetGetCid(g);	/* cid is 1 for next, -1 for prev */
	if ( enc<0 || enc>=ci->sc->parent->charcnt )
return( true );
	if ( !_CI_OK(ci))
return( true );
	ci->sc = SFMakeChar(ci->sc->parent,enc);
	CIFillup(ci);
    }
return( true );
}

static int CI_Show(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	ci->done = true;
	CharViewCreate(ci->rf->sc,ci->cv->fv);
    }
return( true );
}

static int CI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	ci->done = true;
    }
return( true );
}

static int ci_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	GIData *ci = GDrawGetUserData(gw);
	ci->done = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

void SCGetInfo(SplineChar *sc, int nextprev) {
    static GIData gi;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[18];
    GTextInfo label[18];

    gi.sc = sc;
    gi.done = false;

    if ( gi.gw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource( _STR_Charinfo,NULL );
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width =GDrawPointsToPixels(NULL,CI_Width);
	pos.height = GDrawPointsToPixels(NULL,CI_Height);
	gi.gw = GDrawCreateTopWindow(NULL,&pos,ci_e_h,&gi,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));

	label[0].text = (unichar_t *) "Unicode Name:";
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5+6; 
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].gd.mnemonic = 'N';
	gcd[0].creator = GLabelCreate;

	gcd[1].gd.pos.x = 85; gcd[1].gd.pos.y = 5;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].gd.mnemonic = 'N';
	gcd[1].gd.cid = CID_UName;
	gcd[1].creator = GTextFieldCreate;

	label[2].text = (unichar_t *) "Unicode Value:";
	label[2].text_is_1byte = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = 31+6; 
	gcd[2].gd.flags = gg_enabled|gg_visible;
	gcd[2].gd.mnemonic = 'V';
	gcd[2].creator = GLabelCreate;

	gcd[3].gd.pos.x = 85; gcd[3].gd.pos.y = 31;
	gcd[3].gd.flags = gg_enabled|gg_visible;
	gcd[3].gd.mnemonic = 'V';
	gcd[3].gd.cid = CID_UValue;
	gcd[3].creator = GTextFieldCreate;

	label[4].text = (unichar_t *) "Unicode Char:";
	label[4].text_is_1byte = true;
	gcd[4].gd.label = &label[4];
	gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = 57+6; 
	gcd[4].gd.flags = gg_enabled|gg_visible;
	gcd[4].gd.mnemonic = 'h';
	gcd[4].creator = GLabelCreate;

	gcd[5].gd.pos.x = 85; gcd[5].gd.pos.y = 57;
	gcd[5].gd.flags = gg_enabled|gg_visible;
	gcd[5].gd.mnemonic = 'h';
	gcd[5].gd.cid = CID_UChar;
	gcd[5].gd.handle_controlevent = CI_CharChanged;
	gcd[5].creator = GTextFieldCreate;

	gcd[6].gd.pos.x = 12; gcd[6].gd.pos.y = 87;
	gcd[6].gd.flags = gg_visible | gg_enabled;
	label[6].text = (unichar_t *) "Set From Name";
	label[6].text_is_1byte = true;
	gcd[6].gd.mnemonic = 'a';
	gcd[6].gd.label = &label[6];
	gcd[6].gd.handle_controlevent = CI_SName;
	gcd[6].creator = GButtonCreate;

	gcd[7].gd.pos.x = 107; gcd[7].gd.pos.y = 87;
	gcd[7].gd.flags = gg_visible | gg_enabled;
	label[7].text = (unichar_t *) "Set From Value";
	label[7].text_is_1byte = true;
	gcd[7].gd.mnemonic = 'l';
	gcd[7].gd.label = &label[7];
	gcd[7].gd.handle_controlevent = CI_SValue;
	gcd[7].creator = GButtonCreate;

	gcd[8].gd.pos.x = 40; gcd[8].gd.pos.y = CI_Height-32-32;
	gcd[8].gd.pos.width = -1; gcd[8].gd.pos.height = 0;
	gcd[8].gd.flags = gg_visible | gg_enabled ;
	label[8].text = (unichar_t *) "< Prev";
	label[8].text_is_1byte = true;
	gcd[8].gd.mnemonic = 'P';
	gcd[8].gd.label = &label[8];
	gcd[8].gd.handle_controlevent = CI_NextPrev;
	gcd[8].gd.cid = -1;
	gcd[8].creator = GButtonCreate;

	gcd[9].gd.pos.x = CI_Width-GIntGetResource(_NUM_Buttonsize)-40; gcd[9].gd.pos.y = CI_Height-32-32;
	gcd[9].gd.pos.width = -1; gcd[9].gd.pos.height = 0;
	gcd[9].gd.flags = gg_visible | gg_enabled ;
	label[9].text = (unichar_t *) "Next >";
	label[9].text_is_1byte = true;
	gcd[9].gd.label = &label[9];
	gcd[9].gd.mnemonic = 'N';
	gcd[9].gd.handle_controlevent = CI_NextPrev;
	gcd[9].gd.cid = 1;
	gcd[9].creator = GButtonCreate;

	gcd[10].gd.pos.x = 25-3; gcd[10].gd.pos.y = CI_Height-32-3;
	gcd[10].gd.pos.width = -1; gcd[10].gd.pos.height = 0;
	gcd[10].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[10].text = (unichar_t *) _STR_OK;
	label[10].text_in_resource = true;
	gcd[10].gd.mnemonic = 'O';
	gcd[10].gd.label = &label[10];
	gcd[10].gd.handle_controlevent = CI_OK;
	gcd[10].creator = GButtonCreate;

	gcd[11].gd.pos.x = CI_Width-GIntGetResource(_NUM_Buttonsize)-25; gcd[11].gd.pos.y = CI_Height-32;
	gcd[11].gd.pos.width = -1; gcd[11].gd.pos.height = 0;
	gcd[11].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[11].text = (unichar_t *) _STR_Cancel;
	label[11].text_in_resource = true;
	gcd[11].gd.label = &label[11];
	gcd[11].gd.mnemonic = 'C';
	gcd[11].gd.handle_controlevent = CI_Cancel;
	gcd[11].gd.cid = CID_Cancel;
	gcd[11].creator = GButtonCreate;

	gcd[12].gd.pos.x = 5; gcd[12].gd.pos.y = CI_Height-32-32-6;
	gcd[12].gd.pos.width = CI_Width-10;
	gcd[12].gd.flags = gg_visible | gg_enabled;
	gcd[12].creator = GLineCreate;

	gcd[13].gd.pos.x = 5; gcd[13].gd.pos.y = CI_Height-32-32-32-6;
	gcd[13].gd.pos.width = CI_Width-10;
	gcd[13].gd.flags = gg_visible | gg_enabled;
	gcd[13].creator = GLineCreate;

	label[14].text = (unichar_t *) "Ligature:";
	label[14].text_is_1byte = true;
	gcd[14].gd.label = &label[14];
	gcd[14].gd.pos.x = 5; gcd[14].gd.pos.y = CI_Height-32-32-6-26+6; 
	gcd[14].gd.flags = gg_enabled|gg_visible;
	gcd[14].gd.mnemonic = 'L';
	gcd[15].gd.popup_msg = GStringGetResource(_STR_Ligpop,NULL);
	gcd[14].creator = GLabelCreate;

	gcd[15].gd.pos.x = 85; gcd[15].gd.pos.y = CI_Height-32-32-6-26;
	gcd[15].gd.flags = gg_enabled|gg_visible;
	gcd[15].gd.mnemonic = 'L';
	gcd[15].gd.cid = CID_Ligature;
	gcd[15].gd.popup_msg = GStringGetResource(_STR_Ligpop,NULL);
	gcd[15].creator = GTextFieldCreate;

	GGadgetsCreate(gi.gw,gcd);
    }

    CIFillup(&gi);
    if ( !nextprev ) {
	GGadgetSetEnabled(GWidgetGetControl(gi.gw,1),false);
	GGadgetSetEnabled(GWidgetGetControl(gi.gw,-1),false);
	GGadgetSetTitle(GWidgetGetControl(gi.gw,CID_Cancel),GStringGetResource(_STR_Cancel,NULL));
    } else
	GGadgetSetTitle(GWidgetGetControl(gi.gw,CID_Cancel),GStringGetResource(_STR_Done,NULL));

    GWidgetHidePalettes();
    GDrawSetVisible(gi.gw,true);
    while ( !gi.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(gi.gw,false);
}

static void RefGetInfo(CharView *cv, RefChar *ref) {
    static GIData gi;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[12];
    GTextInfo label[12];
    static unichar_t title[] = { 'R','e','f','e','r','e','n','c','e',' ','I','n','f','o',  '\0' };
    char namebuf[100], tbuf[6][40];
    int i;

    gi.cv = cv;
    gi.rf = ref;
    gi.done = false;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = title;
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width =GDrawPointsToPixels(NULL,RI_Width);
	pos.height = GDrawPointsToPixels(NULL,RI_Height);
	gi.gw = GDrawCreateTopWindow(NULL,&pos,ci_e_h,&gi,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));

	sprintf( namebuf, "Reference to character %.20s at %d", ref->sc->name, ref->sc->enc);
	label[0].text = (unichar_t *) namebuf;
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5; 
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].creator = GLabelCreate;

	label[1].text = (unichar_t *) "Transformed by:";
	label[1].text_is_1byte = true;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.pos.x = 5; gcd[1].gd.pos.y = 19; 
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].creator = GLabelCreate;

	for ( i=0; i<6; ++i ) {
	    sprintf(tbuf[i],"%g", ref->transform[i]);
	    label[i+2].text = (unichar_t *) tbuf[i];
	    label[i+2].text_is_1byte = true;
	    gcd[i+2].gd.label = &label[i+2];
	    gcd[i+2].gd.pos.x = 20+((i&1)?60:0); gcd[i+2].gd.pos.y = 33+(i/2)*13; 
	    gcd[i+2].gd.flags = gg_enabled|gg_visible;
	    gcd[i+2].creator = GLabelCreate;
	}

	gcd[8].gd.pos.x = 30; gcd[8].gd.pos.y = RI_Height-32;
	gcd[8].gd.pos.width = -1; gcd[8].gd.pos.height = 0;
	gcd[8].gd.flags = gg_visible | gg_enabled ;
	label[8].text = (unichar_t *) _STR_Show;
	label[8].text_in_resource = true;
	gcd[8].gd.mnemonic = 'S';
	gcd[8].gd.label = &label[8];
	gcd[8].gd.handle_controlevent = CI_Show;
	gcd[8].creator = GButtonCreate;

	gcd[9].gd.pos.x = (RI_Width-GIntGetResource(_NUM_Buttonsize)-6-30); gcd[9].gd.pos.y = RI_Height-32-3;
	gcd[9].gd.pos.width = -1; gcd[9].gd.pos.height = 0;
	gcd[9].gd.flags = gg_visible | gg_enabled | gg_but_default | gg_but_cancel;
	label[9].text = (unichar_t *) _STR_OK;
	label[9].text_in_resource = true;
	gcd[9].gd.mnemonic = 'O';
	gcd[9].gd.label = &label[9];
	gcd[9].gd.handle_controlevent = CI_Cancel;
	gcd[9].creator = GButtonCreate;

	GGadgetsCreate(gi.gw,gcd);

    GWidgetHidePalettes();
    GDrawSetVisible(gi.gw,true);
    while ( !gi.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gi.gw);
}

static void ImgGetInfo(CharView *cv, ImageList *img) {
    static GIData gi;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[12];
    GTextInfo label[12];
    static unichar_t title[] = { 'I','m','a','g','e',' ','I','n','f','o',  '\0' };
    char posbuf[100], scalebuf[100];

    gi.cv = cv;
    gi.img = img;
    gi.done = false;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = title;
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width =GDrawPointsToPixels(NULL,II_Width);
	pos.height = GDrawPointsToPixels(NULL,II_Height);
	gi.gw = GDrawCreateTopWindow(NULL,&pos,ci_e_h,&gi,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));

	sprintf( posbuf, "Image at: (%.0f,%.0f)", img->xoff,
		img->yoff-GImageGetHeight(img->image)*img->yscale);
	label[0].text = (unichar_t *) posbuf;
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5; 
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].creator = GLabelCreate;

	sprintf( scalebuf, "Scaled by: (%.2f,%.2f)", img->xscale, img->yscale );
	label[1].text = (unichar_t *) scalebuf;
	label[1].text_is_1byte = true;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.pos.x = 5; gcd[1].gd.pos.y = 19; 
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].creator = GLabelCreate;

	gcd[2].gd.pos.x = (II_Width-GIntGetResource(_NUM_Buttonsize)-6)/2; gcd[2].gd.pos.y = II_Height-32-3;
	gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
	gcd[2].gd.flags = gg_visible | gg_enabled | gg_but_default | gg_but_cancel;
	label[2].text = (unichar_t *) _STR_OK;
	label[2].text_in_resource = true;
	gcd[2].gd.mnemonic = 'O';
	gcd[2].gd.label = &label[2];
	gcd[2].gd.handle_controlevent = CI_Cancel;
	gcd[2].creator = GButtonCreate;

	GGadgetsCreate(gi.gw,gcd);

    GWidgetHidePalettes();
    GDrawSetVisible(gi.gw,true);
    while ( !gi.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gi.gw);
}

static void PI_DoCancel(GIData *ci) {
    CharView *cv = ci->cv;
    ci->done = true;
    SplinePointListsFree(*cv->heads[cv->drawmode]);
    *cv->heads[cv->drawmode] = ci->oldstate;
    CVRemoveTopUndo(cv);
    SCUpdateAll(cv->sc);
}

static int pi_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	PI_DoCancel( GDrawGetUserData(gw));
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

static int PI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	PI_DoCancel( GDrawGetUserData(GGadgetGetWindow(g)));
    }
return( true );
}

static int PI_Ok(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	ci->done = true;
	/* All the work has been done as we've gone along */
    }
return( true );
}

static void mysprintf( char *buffer, double v) {
    char *pt;

    sprintf( buffer, "%.2f", v );
    pt = strrchr(buffer,'.');
    if ( pt[1]=='0' && pt[2]=='0' )
	*pt='\0';
    else if ( pt[2]=='0' )
	pt[2] = '\0';
}

static void mysprintf2( char *buffer, double v1, double v2) {
    char *pt;

    mysprintf(buffer,v1);
    pt = buffer+strlen(buffer);
    *pt++ = ',';
    mysprintf(pt,v2);
}

static void PIFillup(GIData *ci, int except_cid) {
    char buffer[50];
    unichar_t ubuf[50];

    mysprintf(buffer, ci->cursp->me.x );
    uc_strcpy(ubuf,buffer);
    if ( except_cid!=CID_BaseX )
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_BaseX),ubuf);

    mysprintf(buffer, ci->cursp->me.y );
    uc_strcpy(ubuf,buffer);
    if ( except_cid!=CID_BaseY )
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_BaseY),ubuf);

    mysprintf(buffer, ci->cursp->nextcp.x-ci->cursp->me.x );
    uc_strcpy(ubuf,buffer);
    if ( except_cid!=CID_NextXOff )
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_NextXOff),ubuf);

    mysprintf(buffer, ci->cursp->nextcp.y-ci->cursp->me.y );
    uc_strcpy(ubuf,buffer);
    if ( except_cid!=CID_NextYOff )
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_NextYOff),ubuf);

    mysprintf2(buffer, ci->cursp->nextcp.x,ci->cursp->nextcp.y );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_NextPos),ubuf);

    mysprintf(buffer, ci->cursp->prevcp.x-ci->cursp->me.x );
    uc_strcpy(ubuf,buffer);
    if ( except_cid!=CID_PrevXOff )
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_PrevXOff),ubuf);

    mysprintf(buffer, ci->cursp->prevcp.y-ci->cursp->me.y );
    uc_strcpy(ubuf,buffer);
    if ( except_cid!=CID_PrevYOff )
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_PrevYOff),ubuf);

    mysprintf2(buffer, ci->cursp->prevcp.x,ci->cursp->prevcp.y );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_PrevPos),ubuf);
}

static int PI_Next(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	CharView *cv = ci->cv;
	ci->cursp->selected = false;
	if ( ci->cursp->next!=NULL && ci->cursp->next->to!=ci->curspl->first )
	    ci->cursp = ci->cursp->next->to;
	else {
	    if ( ci->curspl->next == NULL )
		ci->curspl = *cv->heads[cv->drawmode];
	    else
		ci->curspl = ci->curspl->next;
	    ci->cursp = ci->curspl->first;
	}
	ci->cursp->selected = true;
	PIFillup(ci,0);
	SCUpdateAll(cv->sc);
    }
return( true );
}

static int PI_Prev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	CharView *cv = ci->cv;
	SplinePointList *spl;
	ci->cursp->selected = false;
	
	if ( ci->cursp!=ci->curspl->first ) {
	    ci->cursp = ci->cursp->prev->from;
	} else {
	    if ( ci->curspl==*cv->heads[cv->drawmode] ) {
		for ( spl = *cv->heads[cv->drawmode]; spl->next!=NULL; spl=spl->next );
	    } else {
		for ( spl = *cv->heads[cv->drawmode]; spl->next!=ci->curspl; spl=spl->next );
	    }
	    ci->curspl = spl;
	    ci->cursp = spl->last;
	    if ( spl->last==spl->first && spl->last->prev!=NULL )
		ci->cursp = ci->cursp->prev->from;
	}
	ci->cursp->selected = true;
	PIFillup(ci,0);
	SCUpdateAll(cv->sc);
    }
return( true );
}

static int PI_BaseChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	double dx=0, dy=0;
	int err=false;
	SplinePoint *cursp = ci->cursp;

	if ( GGadgetGetCid(g)==CID_BaseX )
	    dx = GetDouble(ci->gw,CID_BaseX,"Base X",&err)-cursp->me.x;
	else
	    dy = GetDouble(ci->gw,CID_BaseY,"Base Y",&err)-cursp->me.y;
	if ( (dx==0 && dy==0) || err )
return( true );
	cursp->me.x += dx;
	cursp->nextcp.x += dx;
	cursp->prevcp.x += dx;
	cursp->me.y += dy;
	cursp->nextcp.y += dy;
	cursp->prevcp.y += dy;
	if ( cursp->next!=NULL )
	    SplineRefigure(cursp->next);
	if ( cursp->prev!=NULL )
	    SplineRefigure(cursp->prev);
	CVCharChangedUpdate(ci->cv);
	PIFillup(ci,GGadgetGetCid(g));
    }
return( true );
}

static int PI_NextChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	double dx=0, dy=0;
	int err=false;
	SplinePoint *cursp = ci->cursp;

	if ( GGadgetGetCid(g)==CID_NextXOff )
	    dx = GetDouble(ci->gw,CID_NextXOff,"Next CP X",&err)-(cursp->nextcp.x-cursp->me.x);
	else
	    dy = GetDouble(ci->gw,CID_NextYOff,"Next CP Y",&err)-(cursp->nextcp.y-cursp->me.y);
	if ( (dx==0 && dy==0) || err )
return( true );
	cursp->nextcp.x += dx;
	cursp->nextcp.y += dy;
	cursp->nonextcp = false;
	if ( cursp->next!=NULL )
	    SplineRefigure(cursp->next);
	CVCharChangedUpdate(ci->cv);
	PIFillup(ci,GGadgetGetCid(g));
    }
return( true );
}

static int PI_PrevChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	double dx=0, dy=0;
	int err=false;
	SplinePoint *cursp = ci->cursp;

	if ( GGadgetGetCid(g)==CID_PrevXOff )
	    dx = GetDouble(ci->gw,CID_PrevXOff,"Prev CP X",&err)-(cursp->prevcp.x-cursp->me.x);
	else
	    dy = GetDouble(ci->gw,CID_PrevYOff,"Prev CP Y",&err)-(cursp->prevcp.y-cursp->me.y);
	if ( (dx==0 && dy==0) || err )
return( true );
	cursp->prevcp.x += dx;
	cursp->prevcp.y += dy;
	cursp->noprevcp = false;
	if ( cursp->prev!=NULL )
	    SplineRefigure(cursp->prev);
	CVCharChangedUpdate(ci->cv);
	PIFillup(ci,GGadgetGetCid(g));
    }
return( true );
}

static void PointGetInfo(CharView *cv, SplinePoint *sp, SplinePointList *spl) {
    static GIData gi;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[22];
    GTextInfo label[22];
    static unichar_t title[] = { 'P','o','i','n','t',' ','I','n','f','o',  '\0' };

    gi.cv = cv;
    gi.cursp = sp;
    gi.curspl = spl;
    gi.oldstate = SplinePointListCopy(*cv->heads[cv->drawmode]);
    gi.done = false;
    CVPreserveState(cv);

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = title;
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width =GDrawPointsToPixels(NULL,PI_Width);
	pos.height = GDrawPointsToPixels(NULL,PI_Height);
	gi.gw = GDrawCreateTopWindow(NULL,&pos,pi_e_h,&gi,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));

	label[0].text = (unichar_t *) "Base:";
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5+6; 
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].creator = GLabelCreate;

	gcd[1].gd.pos.x = 60; gcd[1].gd.pos.y = 5; gcd[1].gd.pos.width = 50;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].gd.cid = CID_BaseX;
	gcd[1].gd.handle_controlevent = PI_BaseChanged;
	gcd[1].creator = GTextFieldCreate;

	gcd[2].gd.pos.x = 120; gcd[2].gd.pos.y = 5; gcd[2].gd.pos.width = 50;
	gcd[2].gd.flags = gg_enabled|gg_visible;
	gcd[2].gd.cid = CID_BaseY;
	gcd[2].gd.handle_controlevent = PI_BaseChanged;
	gcd[2].creator = GTextFieldCreate;

	label[3].text = (unichar_t *) "Next CP:";
	label[3].text_is_1byte = true;
	gcd[3].gd.label = &label[3];
	gcd[3].gd.pos.x = 9; gcd[3].gd.pos.y = 35; 
	gcd[3].gd.flags = gg_enabled|gg_visible;
	gcd[3].creator = GLabelCreate;

	gcd[4].gd.pos.x = 60; gcd[4].gd.pos.y = 35; gcd[4].gd.pos.width = 60;
	gcd[4].gd.flags = gg_enabled|gg_visible;
	gcd[4].gd.cid = CID_NextPos;
	gcd[4].creator = GLabelCreate;

	gcd[5].gd.pos.x = 60; gcd[5].gd.pos.y = 49; gcd[5].gd.pos.width = 50;
	gcd[5].gd.flags = gg_enabled|gg_visible;
	gcd[5].gd.cid = CID_NextXOff;
	gcd[5].gd.handle_controlevent = PI_NextChanged;
	gcd[5].creator = GTextFieldCreate;

	gcd[6].gd.pos.x = 120; gcd[6].gd.pos.y = 49; gcd[6].gd.pos.width = 50;
	gcd[6].gd.flags = gg_enabled|gg_visible;
	gcd[6].gd.cid = CID_NextYOff;
	gcd[6].gd.handle_controlevent = PI_NextChanged;
	gcd[6].creator = GTextFieldCreate;

	label[7].text = (unichar_t *) "Prev CP:";
	label[7].text_is_1byte = true;
	gcd[7].gd.label = &label[7];
	gcd[7].gd.pos.x = gcd[3].gd.pos.x; gcd[7].gd.pos.y = 82; 
	gcd[7].gd.flags = gg_enabled|gg_visible;
	gcd[7].creator = GLabelCreate;

	gcd[8].gd.pos.x = 60; gcd[8].gd.pos.y = 82;  gcd[8].gd.pos.width = 60;
	gcd[8].gd.flags = gg_enabled|gg_visible;
	gcd[8].gd.cid = CID_PrevPos;
	gcd[8].creator = GLabelCreate;

	gcd[9].gd.pos.x = 60; gcd[9].gd.pos.y = 96; gcd[9].gd.pos.width = 50;
	gcd[9].gd.flags = gg_enabled|gg_visible;
	gcd[9].gd.cid = CID_PrevXOff;
	gcd[9].gd.handle_controlevent = PI_PrevChanged;
	gcd[9].creator = GTextFieldCreate;

	gcd[10].gd.pos.x = 120; gcd[10].gd.pos.y = 96; gcd[10].gd.pos.width = 50;
	gcd[10].gd.flags = gg_enabled|gg_visible;
	gcd[10].gd.cid = CID_PrevYOff;
	gcd[10].gd.handle_controlevent = PI_PrevChanged;
	gcd[10].creator = GTextFieldCreate;

	gcd[11].gd.pos.x = (PI_Width-2*50-10)/2; gcd[11].gd.pos.y = 127;
	gcd[11].gd.pos.width = 50; gcd[11].gd.pos.height = 0;
	gcd[11].gd.flags = gg_visible | gg_enabled;
	label[11].text = (unichar_t *) "< Prev";
	label[11].text_is_1byte = true;
	gcd[11].gd.mnemonic = 'P';
	gcd[11].gd.label = &label[11];
	gcd[11].gd.handle_controlevent = PI_Prev;
	gcd[11].creator = GButtonCreate;

	gcd[12].gd.pos.x = PI_Width-50-(PI_Width-2*50-10)/2; gcd[12].gd.pos.y = gcd[11].gd.pos.y;
	gcd[12].gd.pos.width = 50; gcd[12].gd.pos.height = 0;
	gcd[12].gd.flags = gg_visible | gg_enabled;
	label[12].text = (unichar_t *) "Next >";
	label[12].text_is_1byte = true;
	gcd[12].gd.label = &label[12];
	gcd[12].gd.mnemonic = 'N';
	gcd[12].gd.handle_controlevent = PI_Next;
	gcd[12].creator = GButtonCreate;

	gcd[13].gd.pos.x = 5; gcd[13].gd.pos.y = 157;
	gcd[13].gd.pos.width = PI_Width-10;
	gcd[13].gd.flags = gg_enabled|gg_visible;
	gcd[13].creator = GLineCreate;

	gcd[14].gd.pos.x = 20-3; gcd[14].gd.pos.y = PI_Height-35-3;
	gcd[14].gd.pos.width = GIntGetResource(_NUM_Buttonsize)+6; gcd[14].gd.pos.height = 0;
	gcd[14].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[14].text = (unichar_t *) _STR_OK;
	label[14].text_in_resource = true;
	gcd[14].gd.mnemonic = 'O';
	gcd[14].gd.label = &label[14];
	gcd[14].gd.handle_controlevent = PI_Ok;
	gcd[14].creator = GButtonCreate;

	gcd[15].gd.pos.x = PI_Width-GIntGetResource(_NUM_Buttonsize)-20; gcd[15].gd.pos.y = PI_Height-35;
	gcd[15].gd.pos.width = -1; gcd[15].gd.pos.height = 0;
	gcd[15].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[15].text = (unichar_t *) _STR_Cancel;
	label[15].text_in_resource = true;
	gcd[15].gd.label = &label[15];
	gcd[15].gd.mnemonic = 'C';
	gcd[15].gd.handle_controlevent = PI_Cancel;
	gcd[15].creator = GButtonCreate;

	gcd[16].gd.pos.x = 2; gcd[16].gd.pos.y = 2;
	gcd[16].gd.pos.width = pos.width-4; gcd[16].gd.pos.height = pos.height-4;
	gcd[16].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
	gcd[16].creator = GGroupCreate;

	gcd[17].gd.pos.x = 5; gcd[17].gd.pos.y = gcd[3].gd.pos.y-3;
	gcd[17].gd.pos.width = PI_Width-10; gcd[17].gd.pos.height = 43;
	gcd[17].gd.flags = gg_enabled | gg_visible;
	gcd[17].creator = GGroupCreate;

	gcd[18].gd.pos.x = 5; gcd[18].gd.pos.y = gcd[7].gd.pos.y-3;
	gcd[18].gd.pos.width = PI_Width-10; gcd[18].gd.pos.height = 43;
	gcd[18].gd.flags = gg_enabled | gg_visible;
	gcd[18].creator = GGroupCreate;

	label[19].text = (unichar_t *) "Offset";
	label[19].text_is_1byte = true;
	gcd[19].gd.label = &label[19];
	gcd[19].gd.pos.x = gcd[3].gd.pos.x+10; gcd[19].gd.pos.y = gcd[5].gd.pos.y+6; 
	gcd[19].gd.flags = gg_enabled|gg_visible;
	gcd[19].creator = GLabelCreate;

	label[20].text = (unichar_t *) "Offset";
	label[20].text_is_1byte = true;
	gcd[20].gd.label = &label[20];
	gcd[20].gd.pos.x = gcd[3].gd.pos.x+10; gcd[20].gd.pos.y = gcd[9].gd.pos.y+6; 
	gcd[20].gd.flags = gg_enabled|gg_visible;
	gcd[20].creator = GLabelCreate;

	GGadgetsCreate(gi.gw,gcd);

	PIFillup(&gi,0);

    GWidgetHidePalettes();
    GDrawSetVisible(gi.gw,true);
    while ( !gi.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gi.gw);
}

void CVGetInfo(CharView *cv) {
    SplinePoint *sp;
    SplinePointList *spl;
    RefChar *ref;
    ImageList *img;

    if ( !CVOneThingSel(cv,&sp,&spl,&ref,&img))
	SCGetInfo(cv->sc,false);
    else if ( ref!=NULL )
	RefGetInfo(cv,ref);
    else if ( img!=NULL )
	ImgGetInfo(cv,img);
    else
	PointGetInfo(cv,sp,spl);
}
