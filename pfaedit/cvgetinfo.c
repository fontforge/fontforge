/* Copyright (C) 2000-2002 by George Williams */
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
#include <ustring.h>
#include <math.h>
#include <utype.h>
#include <gkeysym.h>

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
    int done, first, changed;
} GIData;

#define CID_UName	1001
#define CID_UValue	1002
#define CID_UChar	1003
#define CID_Ligature	1004
#define CID_Cancel	1005
#define CID_ComponentMsg	1006
#define CID_Components	1007
#define CID_Comment	1008
#define CID_Color	1009

#define CID_BaseX	2001
#define CID_BaseY	2002
#define CID_NextXOff	2003
#define CID_NextYOff	2004
#define CID_NextPos	2005
#define CID_PrevXOff	2006
#define CID_PrevYOff	2007
#define CID_PrevPos	2008
#define CID_NextDef	2009
#define CID_PrevDef	2010

#define CI_Width	200
#define CI_Height	375

#define RI_Width	215
#define RI_Height	180

#define II_Width	130
#define II_Height	70

#define PI_Width	218
#define PI_Height	200

static GTextInfo std_colors[] = {
    { (unichar_t *) _STR_Default, &def_image, 0, 0, (void *) COLOR_DEFAULT, NULL, false, true, false, false, false, false, false, true },
    { NULL, &white_image, 0, 0, (void *) 0xffffff, NULL, false, true },
    { NULL, &red_image, 0, 0, (void *) 0xff0000, NULL, false, true },
    { NULL, &green_image, 0, 0, (void *) 0x00ff00, NULL, false, true },
    { NULL, &blue_image, 0, 0, (void *) 0x0000ff, NULL, false, true },
    { NULL, &yellow_image, 0, 0, (void *) 0xffff00, NULL, false, true },
    { NULL, &cyan_image, 0, 0, (void *) 0x00ffff, NULL, false, true },
    { NULL, &magenta_image, 0, 0, (void *) 0xff00ff, NULL, false, true },
    { NULL, NULL }
};


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

static int ParseUValue(GWindow gw, int cid, int minusoneok, SplineFont *sf) {
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    unichar_t *end;
    int val;

    if (( *ret=='U' || *ret=='u' ) && ret[1]=='+' )
	val = u_strtoul(ret+2,&end,16);
    else if ( *ret=='#' )
	val = u_strtoul(ret+1,&end,16);
    else
	val = u_strtoul(ret,&end,16);
    if ( val==-1 && minusoneok )
return( -1 );
    if ( *end || val<0 || val>0x7fffffff ) {
	ProtestR( _STR_UnicodeValue );
return( -2 );
    } else if ( val>65535 && sf->encoding_name!=em_unicode4 &&
	    sf->encoding_name<em_unicodeplanes ) {
	static int buts[] = { _STR_Yes, _STR_No, 0 };
	if ( GWidgetAskR(_STR_PossiblyTooBig,buts,1,1,_STR_NotUnicodeBMP)==1 )
return(-2);
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

static int LigCheck(SplineFont *sf,SplineChar *sc,char *components) {
    int i;
    unichar_t ubuf[200]; char buffer[10];
    const unichar_t *buts[3]; unichar_t ocmn[2];
    char *pt, *spt, *start, ch;

    if ( components==NULL || *components=='\0' )
return( true );

    buts[2] = NULL;
    buts[0] = GStringGetResource( _STR_OK, &ocmn[0]);
    buts[1] = GStringGetResource( _STR_Cancel, &ocmn[1]);

    for ( i=0; i<sf->charcnt; ++i )
	if ( sf->chars[i]!=sc && sf->chars[i]!=NULL && sf->chars[i]->lig!=NULL ) {
	    if ( strcmp(components,sf->chars[i]->lig->components)==0 ) {
		u_strcpy(ubuf,GStringGetResource( _STR_Alreadyligpre,NULL ));
		uc_strncat(ubuf,sf->chars[i]->name,10);
		u_strcat(ubuf,GStringGetResource( _STR_Alreadyligmid,NULL ));
		sprintf( buffer, "%d", i );
		uc_strcat(ubuf,buffer);
		u_strcat(ubuf,GStringGetResource( _STR_Alreadyligpost,NULL ));
return( GWidgetAsk(GStringGetResource(_STR_Multiple,NULL),buts,ocmn,0,1,ubuf)==0 );
	    }
	}

    start = components;
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
	sc->lig->components = copy(lig);
	sc->lig->lig = sc;
    }
}

int SCSetMetaData(SplineChar *sc,char *name,int unienc,char *lig) {
    SplineFont *sf = sc->parent;
    int i, mv=0;
    int isnotdef;

    if ( !LigCheck(sf,sc,lig))
return( false );

    if ( sc->unicodeenc == unienc && strcmp(name,sc->name)==0 ) {
	/* No change, it must be good */
    } else {
	isnotdef = strcmp(name,".notdef")==0;
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]!=sc ) {
	    if ( unienc!=-1 && sf->chars[i]->unicodeenc==unienc ) {
		if ( !mv && !MultipleValues(sf->chars[i]->name,i)) {
return( false );
		}
		mv = 1;
	    } else if ( !isnotdef && strcmp(name,sf->chars[i]->name)==0 ) {
		if ( !MultipleNames()) {
return( false );
		}
		free(sf->chars[i]->name);
		sf->chars[i]->name = sc->name;
		sc->name = NULL;
	    break;
	    }
	}
    }
    sc->unicodeenc = unienc;
    free(sc->name);
    sc->name = copy(name);
    sf->changed = true;
    if ( (sf->encoding_name==em_unicode || sf->encoding_name==em_unicode4) &&
	    unienc==sc->enc && unienc>=0xe000 && unienc<=0xf8ff )
	/* Ok to name things in the private use area */;
    else if ( (sf->encoding_name<e_first2byte && sc->enc<256) ||
	    (sf->encoding_name>=em_big5 && sf->encoding_name<=em_unicode && sc->enc<65536 ) ||
	    (sf->encoding_name>=e_first2byte && sf->encoding_name<em_unicode && sc->enc<94*96 ) ||
	    sc->unicodeenc!=-1 )
	sf->encoding_name = em_none;
    LigSet(sc,lig);
    SCRefreshTitles(sc);
return( true );
}

static int _CI_OK(GIData *ci) {
    int val;
    int ret, refresh_fvdi=0;
    char *lig, *name;
    const unichar_t *comment;
    FontView *fvs;

    val = ParseUValue(ci->gw,CID_UValue,true,ci->sc->parent);
    if ( val==-2 )
return( false );
    lig = cu_copy( _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_Ligature)) );
    name = cu_copy( _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UName)) );
    if ( strcmp(name,ci->sc->name)!=0 || val!=ci->sc->unicodeenc )
	refresh_fvdi = 1;
    ret = SCSetMetaData(ci->sc,name,val,lig);
    free(name);
    free(lig);
    if ( refresh_fvdi ) {
	for ( fvs=ci->sc->parent->fv; fvs!=NULL; fvs=fvs->next )
	    GDrawRequestExpose(fvs->gw,NULL,false);	/* Redraw info area just in case this char is selected */
    }
    if ( ret ) {
	comment = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_Comment));
	free(ci->sc->comment); ci->sc->comment = NULL;
	if ( *comment!='\0' )
	    ci->sc->comment = u_copy(comment);
	val = GGadgetGetFirstListSelectedItem(GWidgetGetControl(ci->gw,CID_Color));
	if ( val!=-1 ) {
	    if ( ci->sc->color != (int) (std_colors[val].userdata) ) {
		ci->sc->color = (int) (std_colors[val].userdata);
		for ( fvs=ci->sc->parent->fv; fvs!=NULL; fvs=fvs->next )
		    GDrawRequestExpose(fvs->v,NULL,false);	/* Redraw info area just in case this char is selected */
	    }
	}
    }
return( ret );
}

static int CI_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	if ( _CI_OK(ci) )
	    ci->done = true;
    }
return( true );
}

static void LigatureNameCheck(GIData *ci, int uni, const unichar_t *name) {
    /* I'm not checking to see if the components are known names... */
    unichar_t *components=NULL;
    char *namtemp = cu_copy(name);
    char *temp = LigDefaultStr(uni,namtemp);

    free(namtemp);
    if ( temp==NULL )
return;
    components = uc_copy(temp);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_Ligature),components);
    free(components);
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
	    /* Adobe says names like uni00410042 represent a ligature (A&B) */
	    /*  (that is "uni" followed by two 4-digit codes). Adobe does not */
	    /*  name things outside BMP, but logically they'd be uniXXXXXX. */
	    /*  currently unicode doesn't go up to XXXXXXXX so there is no */
	    /*  ambiguity */
	    if ( ret[0]=='u' && ret[1]=='n' && ret[2]=='i' && u_strlen(ret)<3+8 ) {
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
	LigatureNameCheck(ci,i,ret);
    }
return( true );
}

static int CI_SValue(GGadget *g, GEvent *e) {	/* Set From Value */
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	unichar_t ubuf[2];
	int val;

	val = ParseUValue(ci->gw,CID_UValue,false,ci->sc->parent);
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
	} else if ( ret[0]=='\0' )
return( true );

	SetNameFromUnicode(ci->gw,CID_UName,val);

	sprintf(buf,"U+%04x", val);
	temp = uc_copy(buf);
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UValue),temp);
	free(temp);
    }
return( true );
}

static int CI_CommentChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	if ( ci->first && ci->sc->color==COLOR_DEFAULT &&
		0==GGadgetGetFirstListSelectedItem(GWidgetGetControl(ci->gw,CID_Color)) )
	    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_Color),1);
	ci->first = false;
    }
return( true );
}

static void CIFillup(GIData *ci) {
    SplineChar *sc = ci->sc;
    SplineFont *sf = sc->parent;
    unichar_t *temp;
    char buffer[10];
    unichar_t ubuf[2];
    const unichar_t *bits;
    int i;

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
	temp = uc_copy(sc->lig->components);
    else
	temp = uc_copy("");
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_Ligature),temp);
    free(temp);

    bits = SFGetAlternate(sc->parent,sc->unicodeenc,sc,true);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_ComponentMsg),GStringGetResource(
	bits==NULL ? _STR_NoComponents :
	hascomposing(sc->parent,sc->unicodeenc,sc) ? _STR_AccentedComponents :
	    _STR_CompositComponents, NULL));
    if ( bits==NULL ) {
	ubuf[0] = '\0';
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_Components),ubuf);
    } else {
	unichar_t *temp = galloc(11*u_strlen(bits)*sizeof(unichar_t));
	unichar_t *upt=temp;
	while ( *bits!='\0' ) {
	    sprintf(buffer, "U+%04x ", *bits );
	    uc_strcpy(upt,buffer);
	    upt += u_strlen(upt);
	    ++bits;
	}
	upt[-1] = '\0';
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_Components),temp);
	free(temp);
    }
    ubuf[0] = '\0';
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_Comment),
	    sc->comment?sc->comment:ubuf);
    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_Color),0);
    for ( i=0; std_colors[i].image!=NULL; ++i ) {
	if ( std_colors[i].userdata == (void *) sc->color )
	    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_Color),i);
    }
    ci->first = sc->comment==NULL;
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

static int CI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	ci->done = true;
    }
return( true );
}

static int CI_TransChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	ci->changed = true;
    }
return( true );
}

static int CI_ROK_Do(GIData *ci) {
    int errs=false,i;
    real trans[6];
    SplinePointList *spl, *new;
    RefChar *ref = ci->rf, *subref;

    for ( i=0; i<6; ++i ) {
	trans[i] = GetRealR(ci->gw,1000+i,_STR_TransformationMatrix,&errs);
	if ( !errs &&
		((i<4 && (trans[i]>30 || trans[i]<-30)) ||
		 (i>=4 && (trans[i]>16000 || trans[i]<-16000))) ) {
	    /* Don't want the user to insert an enormous scale factor or */
	    /*  it will move points outside the legal range. */
	    GTextFieldSelect(GWidgetGetControl(ci->gw,1000+i),0,-1);
	    GWidgetErrorR(_STR_OutOfRange,_STR_OutOfRange);
	    errs = true;
	}
	if ( errs )
return( false );
    }

    for ( i=0; i<6 && ref->transform[i]==trans[i]; ++i );
    if ( i==6 )		/* Didn't really change */
return( true );

    for ( i=0; i<6; ++i )
	ref->transform[i] = trans[i];
    SplinePointListFree(ref->splines);
    ref->splines = SplinePointListTransform(SplinePointListCopy(ref->sc->splines),trans,true);
    spl = NULL;
    if ( ref->splines!=NULL )
	for ( spl = ref->splines; spl->next!=NULL; spl = spl->next );
    for ( subref = ref->sc->refs; subref!=NULL; subref=subref->next ) {
	new = SplinePointListTransform(SplinePointListCopy(subref->splines),trans,true);
	if ( spl==NULL )
	    ref->splines = new;
	else
	    spl->next = new;
	if ( new!=NULL )
	    for ( spl = new; spl->next!=NULL; spl = spl->next );
    }

    SplineSetFindBounds(ref->splines,&ref->bb);
    CVCharChangedUpdate(ci->cv);
return( true );
}

static int CI_ROK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	if ( CI_ROK_Do(ci))
	    ci->done = true;
    }
return( true );
}

static int CI_Show(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	if ( ci->changed ) {
	    static int buts[] = { _STR_Change, _STR_Retain, _STR_Cancel, 0 };
	    int ans = GWidgetAskR(_STR_TransformChanged,buts,0,2,_STR_TransformChangedApply);
	    if ( ans==2 )
return( true );
	    else if ( ans==0 ) {
		if ( !CI_ROK_Do(ci))
return( true );
	    }
	}
	ci->done = true;
	CharViewCreate(ci->rf->sc,ci->cv->fv);
    }
return( true );
}

static int ci_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	GIData *ci = GDrawGetUserData(gw);
	ci->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("getinfo.html");
return( true );
	}
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
    GGadgetCreateData gcd[25];
    GTextInfo label[25];

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
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,CI_Width));
	pos.height = GDrawPointsToPixels(NULL,CI_Height);
	gi.gw = GDrawCreateTopWindow(NULL,&pos,ci_e_h,&gi,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));

	label[0].text = (unichar_t *) _STR_UnicodeName;
	label[0].text_in_resource = true;
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

	label[2].text = (unichar_t *) _STR_UnicodeValue;
	label[2].text_in_resource = true;
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

	label[4].text = (unichar_t *) _STR_UnicodeChar;
	label[4].text_in_resource = true;
	gcd[4].gd.label = &label[4];
	gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = 57+6; 
	gcd[4].gd.flags = gg_enabled|gg_visible;
	gcd[4].gd.mnemonic = 'h';
	gcd[4].creator = GLabelCreate;

	gcd[5].gd.pos.x = 85; gcd[5].gd.pos.y = 57;
	gcd[5].gd.flags = gg_enabled|gg_visible|gg_text_xim;
	gcd[5].gd.mnemonic = 'h';
	gcd[5].gd.cid = CID_UChar;
	gcd[5].gd.handle_controlevent = CI_CharChanged;
	gcd[5].creator = GTextFieldCreate;

	gcd[6].gd.pos.x = 12; gcd[6].gd.pos.y = 87;
	gcd[6].gd.flags = gg_visible | gg_enabled;
	label[6].text = (unichar_t *) _STR_SetFromName;
	label[6].text_in_resource = true;
	gcd[6].gd.mnemonic = 'a';
	gcd[6].gd.label = &label[6];
	gcd[6].gd.handle_controlevent = CI_SName;
	gcd[6].creator = GButtonCreate;

	gcd[7].gd.pos.x = 107; gcd[7].gd.pos.y = 87;
	gcd[7].gd.flags = gg_visible | gg_enabled;
	label[7].text = (unichar_t *) _STR_SetFromValue;
	label[7].text_in_resource = true;
	gcd[7].gd.mnemonic = 'l';
	gcd[7].gd.label = &label[7];
	gcd[7].gd.handle_controlevent = CI_SValue;
	gcd[7].creator = GButtonCreate;

	gcd[8].gd.pos.x = 40; gcd[8].gd.pos.y = CI_Height-32-32;
	gcd[8].gd.pos.width = -1; gcd[8].gd.pos.height = 0;
	gcd[8].gd.flags = gg_visible | gg_enabled ;
	label[8].text = (unichar_t *) _STR_PrevArrow;
	label[8].text_in_resource = true;
	gcd[8].gd.mnemonic = 'P';
	gcd[8].gd.label = &label[8];
	gcd[8].gd.handle_controlevent = CI_NextPrev;
	gcd[8].gd.cid = -1;
	gcd[8].creator = GButtonCreate;

	gcd[9].gd.pos.x = CI_Width-GIntGetResource(_NUM_Buttonsize)-40; gcd[9].gd.pos.y = CI_Height-32-32;
	gcd[9].gd.pos.width = -1; gcd[9].gd.pos.height = 0;
	gcd[9].gd.flags = gg_visible | gg_enabled ;
	label[9].text = (unichar_t *) _STR_NextArrow;
	label[9].text_in_resource = true;
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

	gcd[11].gd.pos.x = -25; gcd[11].gd.pos.y = CI_Height-32;
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

	gcd[13].gd.pos.x = 5; gcd[13].gd.pos.y = CI_Height-32-32-32-8-30;
	gcd[13].gd.pos.width = CI_Width-10;
	gcd[13].gd.flags = gg_visible | gg_enabled;
	gcd[13].creator = GLineCreate;

	gcd[14].gd.pos.x = 5; gcd[14].gd.pos.y = gcd[7].gd.pos.y+30;
	gcd[14].gd.pos.width = CI_Width-10;
	gcd[14].gd.flags = gg_visible | gg_enabled;
	gcd[14].creator = GLineCreate;

	label[15].text = (unichar_t *) _STR_Comment;
	label[15].text_in_resource = true;
	gcd[15].gd.label = &label[15];
	gcd[15].gd.pos.x = 5; gcd[15].gd.pos.y = gcd[14].gd.pos.y+5; 
	gcd[15].gd.flags = gg_enabled|gg_visible;
	gcd[15].creator = GLabelCreate;

	gcd[16].gd.pos.x = 5; gcd[16].gd.pos.y = gcd[15].gd.pos.y+13;
	gcd[16].gd.pos.width = CI_Width-10;
	gcd[16].gd.pos.height = GDrawPointsToPixels(NULL,4*12+6);
	gcd[16].gd.flags = gg_enabled|gg_visible|gg_textarea_wrap|gg_text_xim;
	gcd[16].gd.cid = CID_Comment;
	gcd[16].gd.handle_controlevent = CI_CommentChanged;
	gcd[16].creator = GTextAreaCreate;

	label[17].text = (unichar_t *) _STR_Color;
	label[17].text_in_resource = true;
	gcd[17].gd.label = &label[17];
	gcd[17].gd.pos.x = 5; gcd[17].gd.pos.y = gcd[16].gd.pos.y+gcd[16].gd.pos.height+5+6; 
	gcd[17].gd.flags = gg_enabled|gg_visible;
	gcd[17].creator = GLabelCreate;

	gcd[18].gd.pos.x = gcd[3].gd.pos.x; gcd[18].gd.pos.y = gcd[17].gd.pos.y-6;
	gcd[18].gd.pos.width = gcd[3].gd.pos.width;
	gcd[18].gd.flags = gg_enabled|gg_visible;
	gcd[18].gd.cid = CID_Color;
	gcd[18].gd.u.list = std_colors;
	gcd[18].creator = GListButtonCreate;

	label[19].text = (unichar_t *) _STR_Ligature;
	label[19].text_in_resource = true;
	gcd[19].gd.label = &label[19];
	gcd[19].gd.pos.x = 5; gcd[19].gd.pos.y = CI_Height-32-32-6-26+6-30; 
	gcd[19].gd.flags = gg_enabled|gg_visible;
	gcd[19].gd.mnemonic = 'L';
	gcd[19].gd.popup_msg = GStringGetResource(_STR_Ligpop,NULL);
	gcd[19].creator = GLabelCreate;

	gcd[20].gd.pos.x = 85; gcd[20].gd.pos.y = CI_Height-32-32-6-26-30;
	gcd[20].gd.flags = gg_enabled|gg_visible;
	gcd[20].gd.mnemonic = 'L';
	gcd[20].gd.cid = CID_Ligature;
	gcd[20].gd.popup_msg = GStringGetResource(_STR_Ligpop,NULL);
	gcd[20].creator = GTextFieldCreate;

	gcd[21].gd.pos.x = 5; gcd[21].gd.pos.y = CI_Height-32-32-8-27;
	gcd[21].gd.pos.width = CI_Width-10;
	gcd[21].gd.flags = gg_visible | gg_enabled;
	gcd[21].creator = GLineCreate;

	label[22].text = (unichar_t *) _STR_AccentedComponents;
	label[22].text_in_resource = true;
	gcd[22].gd.label = &label[22];
	gcd[22].gd.pos.x = 5; gcd[22].gd.pos.y = CI_Height-32-32-6-24; 
	gcd[22].gd.flags = gg_enabled|gg_visible;
	gcd[22].gd.cid = CID_ComponentMsg;
	/*gcd[22].gd.popup_msg = GStringGetResource(_STR_Ligpop,NULL);*/
	gcd[22].creator = GLabelCreate;

	gcd[23].gd.pos.x = 5; gcd[23].gd.pos.y = CI_Height-32-32-6-12;
	gcd[23].gd.pos.width = CI_Width-10;
	gcd[23].gd.flags = gg_enabled|gg_visible;
	gcd[23].gd.cid = CID_Components;
	/*gcd[23].gd.popup_msg = GStringGetResource(_STR_Ligpop,NULL);*/
	gcd[23].creator = GLabelCreate;

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
    GGadgetCreateData gcd[16];
    GTextInfo label[16];
    char namebuf[100], tbuf[6][40];
    char ubuf[40];
    int i,j;

    gi.cv = cv;
    gi.rf = ref;
    gi.changed = false;
    gi.done = false;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource(_STR_ReferenceInfo,NULL);
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,RI_Width));
	pos.height = GDrawPointsToPixels(NULL,
		ref->sc->unicodeenc!=-1?RI_Height+12:RI_Height);
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
	j = 1;

	if ( ref->sc->unicodeenc!=-1 ) {
	    sprintf( ubuf, " Unicode: U+%04x", ref->sc->unicodeenc );
	    label[1].text = (unichar_t *) ubuf;
	    label[1].text_is_1byte = true;
	    gcd[1].gd.label = &label[1];
	    gcd[1].gd.pos.x = 5; gcd[1].gd.pos.y = 17;
	    gcd[1].gd.flags = gg_enabled|gg_visible;
	    gcd[1].creator = GLabelCreate;
	    j=2;
	}

	label[j].text = (unichar_t *) _STR_TransformedBy;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+14;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.popup_msg = GStringGetResource(_STR_TransformPopup,NULL);
	gcd[j].creator = GLabelCreate;
	++j;

	for ( i=0; i<6; ++i ) {
	    sprintf(tbuf[i],"%g", ref->transform[i]);
	    label[i+j].text = (unichar_t *) tbuf[i];
	    label[i+j].text_is_1byte = true;
	    gcd[i+j].gd.label = &label[i+j];
	    gcd[i+j].gd.pos.x = 20+((i&1)?85:0); gcd[i+j].gd.pos.width=75;
	    gcd[i+j].gd.pos.y = gcd[j-1].gd.pos.y+14+(i/2)*26; 
	    gcd[i+j].gd.flags = gg_enabled|gg_visible;
	    gcd[i+j].gd.cid = i+1000;
	    gcd[i+j].gd.handle_controlevent = CI_TransChange;
	    gcd[i+j].creator = GTextFieldCreate;
	}

	gcd[6+j].gd.pos.x = (RI_Width-GIntGetResource(_NUM_Buttonsize))/2;
	gcd[6+j].gd.pos.y = RI_Height+(j==3?12:0)-64;
	gcd[6+j].gd.pos.width = -1; gcd[6+j].gd.pos.height = 0;
	gcd[6+j].gd.flags = gg_visible | gg_enabled ;
	label[6+j].text = (unichar_t *) _STR_Show;
	label[6+j].text_in_resource = true;
	gcd[6+j].gd.mnemonic = 'S';
	gcd[6+j].gd.label = &label[6+j];
	gcd[6+j].gd.handle_controlevent = CI_Show;
	gcd[6+j].creator = GButtonCreate;

	gcd[7+j].gd.pos.x = 30-3; gcd[7+j].gd.pos.y = RI_Height+(j==3?12:0)-32-3;
	gcd[7+j].gd.pos.width = -1; gcd[7+j].gd.pos.height = 0;
	gcd[7+j].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[7+j].text = (unichar_t *) _STR_OK;
	label[7+j].text_in_resource = true;
	gcd[7+j].gd.mnemonic = 'O';
	gcd[7+j].gd.label = &label[7+j];
	gcd[7+j].gd.handle_controlevent = CI_ROK;
	gcd[7+j].creator = GButtonCreate;

	gcd[8+j].gd.pos.x = -30; gcd[8+j].gd.pos.y = gcd[7+j].gd.pos.y+3;
	gcd[8+j].gd.pos.width = -1; gcd[8+j].gd.pos.height = 0;
	gcd[8+j].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[8+j].text = (unichar_t *) _STR_Cancel;
	label[8+j].text_in_resource = true;
	gcd[8+j].gd.mnemonic = 'C';
	gcd[8+j].gd.label = &label[8+j];
	gcd[8+j].gd.handle_controlevent = CI_Cancel;
	gcd[8+j].creator = GButtonCreate;

	gcd[9+j].gd.pos.x = 5; gcd[9+j].gd.pos.y = gcd[6+j].gd.pos.y-6;
	gcd[9+j].gd.pos.width = RI_Width-10;
	gcd[9+j].gd.flags = gg_visible | gg_enabled;
	gcd[9+j].creator = GLineCreate;

	gcd[10+j] = gcd[9+j];
	gcd[9+j].gd.pos.y = gcd[7+j].gd.pos.y-3;

	GGadgetsCreate(gi.gw,gcd);
	GWidgetIndicateFocusGadget(gcd[j].ret);

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
	wattrs.window_title = GStringGetResource(_STR_ImageInfo,NULL);
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,II_Width));
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

	gcd[2].gd.pos.x = (II_Width-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor)-6)/2; gcd[2].gd.pos.y = II_Height-32-3;
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

void MDReplace(MinimumDistance *md,SplineSet *old,SplineSet *rpl) {
    /* Replace all the old points with the ones in rpl in the minimu distance hints */
    SplinePoint *osp, *rsp;
    MinimumDistance *test;

    while ( old!=NULL && rpl!=NULL ) {
	osp = old->first; rsp = rpl->first;
	while ( 1 ) {
	    for ( test=md; test!=NULL ; test=test->next ) {
		if ( test->sp1==osp )
		    test->sp1 = rsp;
		if ( test->sp2==osp )
		    test->sp2 = rsp;
	    }
	    if ( osp->next==NULL )
	break;
	    osp = osp->next->to;
	    rsp = rsp->next->to;
	    if ( osp==old->first )
	break;
	}
	old = old->next;
	rpl = rpl->next;
    }
}

static void PI_DoCancel(GIData *ci) {
    CharView *cv = ci->cv;
    ci->done = true;
    if ( cv->drawmode==dm_fore )
	MDReplace(cv->sc->md,cv->sc->splines,ci->oldstate);
    SplinePointListsFree(*cv->heads[cv->drawmode]);
    *cv->heads[cv->drawmode] = ci->oldstate;
    CVRemoveTopUndo(cv);
    SCClearSelPt(cv->sc);
    SCUpdateAll(cv->sc);
}

static int pi_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	PI_DoCancel( GDrawGetUserData(gw));
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("getinfo.html");
return( true );
	}
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

static void mysprintf( char *buffer, real v) {
    char *pt;

    sprintf( buffer, "%.2f", v );
    pt = strrchr(buffer,'.');
    if ( pt[1]=='0' && pt[2]=='0' )
	*pt='\0';
    else if ( pt[2]=='0' )
	pt[2] = '\0';
}

static void mysprintf2( char *buffer, real v1, real v2) {
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

    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_NextDef), ci->cursp->nextcpdef );

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

    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_PrevDef), ci->cursp->prevcpdef );
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
	CVShowPoint(cv,ci->cursp);
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
	cv->p.nextcp = cv->p.prevcp = false;
	PIFillup(ci,0);
	CVShowPoint(cv,ci->cursp);
	SCUpdateAll(cv->sc);
    }
return( true );
}

static int PI_BaseChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	real dx=0, dy=0;
	int err=false;
	SplinePoint *cursp = ci->cursp;

	if ( GGadgetGetCid(g)==CID_BaseX )
	    dx = GetCalmRealR(ci->gw,CID_BaseX,_STR_BaseX,&err)-cursp->me.x;
	else
	    dy = GetCalmRealR(ci->gw,CID_BaseY,_STR_BaseY,&err)-cursp->me.y;
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
	real dx=0, dy=0;
	int err=false;
	SplinePoint *cursp = ci->cursp;

	if ( GGadgetGetCid(g)==CID_NextXOff )
	    dx = GetCalmRealR(ci->gw,CID_NextXOff,_STR_NextCPX,&err)-(cursp->nextcp.x-cursp->me.x);
	else
	    dy = GetCalmRealR(ci->gw,CID_NextYOff,_STR_NextCPY,&err)-(cursp->nextcp.y-cursp->me.y);
	if ( (dx==0 && dy==0) || err )
return( true );
	cursp->nextcp.x += dx;
	cursp->nextcp.y += dy;
	cursp->nonextcp = false;
	if (( dx>.1 || dx<-.1 || dy>.1 || dy<-.1 ) && cursp->nextcpdef ) {
	    cursp->nextcpdef = false;
	    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_NextDef), false );
	}
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
	real dx=0, dy=0;
	int err=false;
	SplinePoint *cursp = ci->cursp;

	if ( GGadgetGetCid(g)==CID_PrevXOff )
	    dx = GetCalmRealR(ci->gw,CID_PrevXOff,_STR_PrevCPX,&err)-(cursp->prevcp.x-cursp->me.x);
	else
	    dy = GetCalmRealR(ci->gw,CID_PrevYOff,_STR_PrevCPY,&err)-(cursp->prevcp.y-cursp->me.y);
	if ( (dx==0 && dy==0) || err )
return( true );
	cursp->prevcp.x += dx;
	cursp->prevcp.y += dy;
	cursp->noprevcp = false;
	if (( dx>.1 || dx<-.1 || dy>.1 || dy<-.1 ) && cursp->prevcpdef ) {
	    cursp->prevcpdef = false;
	    GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_PrevDef), false );
	}
	if ( cursp->prev!=NULL )
	    SplineRefigure(cursp->prev);
	CVCharChangedUpdate(ci->cv);
	PIFillup(ci,GGadgetGetCid(g));
    }
return( true );
}

static int PI_NextDefChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	SplinePoint *cursp = ci->cursp;

	cursp->nextcpdef = GGadgetIsChecked(g);
	/* If they turned def off, that's a noop, but if they turned it on... */
	/*  then set things to the default */
	if ( cursp->nextcpdef ) {
	    BasePoint temp = cursp->prevcp;
	    SplineCharDefaultNextCP(cursp,cursp->next==NULL?NULL:cursp->next->to);
	    if ( !cursp->prevcpdef )
		cursp->prevcp = temp;
	    CVCharChangedUpdate(ci->cv);
	    PIFillup(ci,GGadgetGetCid(g));
	}
    }
return( true );
}

static int PI_PrevDefChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	SplinePoint *cursp = ci->cursp;

	cursp->prevcpdef = GGadgetIsChecked(g);
	/* If they turned def off, that's a noop, but if they turned it on... */
	/*  then set things to the default */
	if ( cursp->prevcpdef ) {
	    BasePoint temp = cursp->nextcp;
	    SplineCharDefaultPrevCP(cursp,cursp->prev==NULL?NULL:cursp->prev->from);
	    if ( !cursp->nextcpdef )
		cursp->nextcp = temp;
	    CVCharChangedUpdate(ci->cv);
	    PIFillup(ci,GGadgetGetCid(g));
	}
    }
return( true );
}

static void PointGetInfo(CharView *cv, SplinePoint *sp, SplinePointList *spl) {
    static GIData gi;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[24];
    GTextInfo label[24];
    static GBox cur, nextcp, prevcp;
    extern Color nextcpcol, prevcpcol;
    GWindow root;
    GRect screensize;
    GPoint pt;

    cur.main_background = nextcp.main_background = prevcp.main_background = COLOR_DEFAULT;
    cur.main_foreground = 0xff0000;
    nextcp.main_foreground = nextcpcol;
    prevcp.main_foreground = prevcpcol;
    gi.cv = cv;
    gi.cursp = sp;
    gi.curspl = spl;
    gi.oldstate = SplinePointListCopy(*cv->heads[cv->drawmode]);
    gi.done = false;
    CVPreserveState(cv);

    root = GDrawGetRoot(NULL);
    GDrawGetSize(root,&screensize);

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_positioned|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.positioned = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource(_STR_PointInfo,NULL);
	wattrs.is_dlg = true;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,PI_Width));
	pos.height = GDrawPointsToPixels(NULL,PI_Height);
	pt.x = cv->xoff + rint(sp->me.x*cv->scale);
	pt.y = -cv->yoff + cv->height - rint(sp->me.y*cv->scale);
	GDrawTranslateCoordinates(cv->v,root,&pt);
	if ( pt.x+20+pos.width<=screensize.width )
	    pos.x = pt.x+20;
	else if ( (pos.x = pt.x-10-screensize.width)<0 )
	    pos.x = 0;
	pos.y = pt.y;
	if ( pos.y+pos.height+20 > screensize.height )
	    pos.y = screensize.height - pos.height - 20;
	if ( pos.y<0 ) pos.y = 0;
	gi.gw = GDrawCreateTopWindow(NULL,&pos,pi_e_h,&gi,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));

	label[0].text = (unichar_t *) _STR_Base;
	label[0].text_in_resource = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5+6; 
	gcd[0].gd.flags = gg_enabled|gg_visible|gg_dontcopybox;
	gcd[0].gd.box = &cur;
	gcd[0].creator = GLabelCreate;

	gcd[1].gd.pos.x = 60; gcd[1].gd.pos.y = 5; gcd[1].gd.pos.width = 70;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].gd.cid = CID_BaseX;
	gcd[1].gd.handle_controlevent = PI_BaseChanged;
	gcd[1].creator = GTextFieldCreate;

	gcd[2].gd.pos.x = 137; gcd[2].gd.pos.y = 5; gcd[2].gd.pos.width = 70;
	gcd[2].gd.flags = gg_enabled|gg_visible;
	gcd[2].gd.cid = CID_BaseY;
	gcd[2].gd.handle_controlevent = PI_BaseChanged;
	gcd[2].creator = GTextFieldCreate;

	label[3].text = (unichar_t *) _STR_PrevCP;
	label[3].text_in_resource = true;
	gcd[3].gd.label = &label[3];
	gcd[3].gd.pos.x = 9; gcd[3].gd.pos.y = 35; 
	gcd[3].gd.flags = gg_enabled|gg_visible|gg_dontcopybox;
	gcd[3].gd.box = &prevcp;
	gcd[3].creator = GLabelCreate;

	gcd[4].gd.pos.x = 60; gcd[4].gd.pos.y = 35; gcd[4].gd.pos.width = 60;
	gcd[4].gd.flags = gg_enabled|gg_visible;
	gcd[4].gd.cid = CID_PrevPos;
	gcd[4].creator = GLabelCreate;

	label[21].text = (unichar_t *) _STR_Default;
	label[21].text_in_resource = true;
	gcd[21].gd.label = &label[21];
	gcd[21].gd.pos.x = 125; gcd[21].gd.pos.y = gcd[4].gd.pos.y-3;
	gcd[21].gd.flags = gg_enabled|gg_visible;
	gcd[21].gd.cid = CID_PrevDef;
	gcd[21].gd.handle_controlevent = PI_PrevDefChanged;
	gcd[21].creator = GCheckBoxCreate;

	gcd[5].gd.pos.x = 60; gcd[5].gd.pos.y = 49; gcd[5].gd.pos.width = 70;
	gcd[5].gd.flags = gg_enabled|gg_visible;
	gcd[5].gd.cid = CID_PrevXOff;
	gcd[5].gd.handle_controlevent = PI_PrevChanged;
	gcd[5].creator = GTextFieldCreate;

	gcd[6].gd.pos.x = 137; gcd[6].gd.pos.y = 49; gcd[6].gd.pos.width = 70;
	gcd[6].gd.flags = gg_enabled|gg_visible;
	gcd[6].gd.cid = CID_PrevYOff;
	gcd[6].gd.handle_controlevent = PI_PrevChanged;
	gcd[6].creator = GTextFieldCreate;

	label[7].text = (unichar_t *) _STR_NextCP;
	label[7].text_in_resource = true;
	gcd[7].gd.label = &label[7];
	gcd[7].gd.pos.x = gcd[3].gd.pos.x; gcd[7].gd.pos.y = 82; 
	gcd[7].gd.flags = gg_enabled|gg_visible;
	gcd[7].gd.flags = gg_enabled|gg_visible|gg_dontcopybox;
	gcd[7].gd.box = &nextcp;
	gcd[7].creator = GLabelCreate;

	gcd[8].gd.pos.x = 60; gcd[8].gd.pos.y = 82;  gcd[8].gd.pos.width = 60;
	gcd[8].gd.flags = gg_enabled|gg_visible;
	gcd[8].gd.cid = CID_NextPos;
	gcd[8].creator = GLabelCreate;

	label[22].text = (unichar_t *) _STR_Default;
	label[22].text_in_resource = true;
	gcd[22].gd.label = &label[22];
	gcd[22].gd.pos.x = gcd[21].gd.pos.x; gcd[22].gd.pos.y = gcd[8].gd.pos.y-3;
	gcd[22].gd.flags = gg_enabled|gg_visible;
	gcd[22].gd.cid = CID_NextDef;
	gcd[22].gd.handle_controlevent = PI_NextDefChanged;
	gcd[22].creator = GCheckBoxCreate;

	gcd[9].gd.pos.x = 60; gcd[9].gd.pos.y = 96; gcd[9].gd.pos.width = 70;
	gcd[9].gd.flags = gg_enabled|gg_visible;
	gcd[9].gd.cid = CID_NextXOff;
	gcd[9].gd.handle_controlevent = PI_NextChanged;
	gcd[9].creator = GTextFieldCreate;

	gcd[10].gd.pos.x = 137; gcd[10].gd.pos.y = 96; gcd[10].gd.pos.width = 70;
	gcd[10].gd.flags = gg_enabled|gg_visible;
	gcd[10].gd.cid = CID_NextYOff;
	gcd[10].gd.handle_controlevent = PI_NextChanged;
	gcd[10].creator = GTextFieldCreate;

	gcd[11].gd.pos.x = (PI_Width-2*50-10)/2; gcd[11].gd.pos.y = 127;
	gcd[11].gd.pos.width = 53; gcd[11].gd.pos.height = 0;
	gcd[11].gd.flags = gg_visible | gg_enabled;
	label[11].text = (unichar_t *) _STR_PrevArrow;
	label[11].text_in_resource = true;
	gcd[11].gd.mnemonic = 'P';
	gcd[11].gd.label = &label[11];
	gcd[11].gd.handle_controlevent = PI_Prev;
	gcd[11].creator = GButtonCreate;

	gcd[12].gd.pos.x = PI_Width-50-(PI_Width-2*50-10)/2; gcd[12].gd.pos.y = gcd[11].gd.pos.y;
	gcd[12].gd.pos.width = 53; gcd[12].gd.pos.height = 0;
	gcd[12].gd.flags = gg_visible | gg_enabled;
	label[12].text = (unichar_t *) _STR_NextArrow;
	label[12].text_in_resource = true;
	gcd[12].gd.label = &label[12];
	gcd[12].gd.mnemonic = 'N';
	gcd[12].gd.handle_controlevent = PI_Next;
	gcd[12].creator = GButtonCreate;

	gcd[13].gd.pos.x = 5; gcd[13].gd.pos.y = 157;
	gcd[13].gd.pos.width = PI_Width-10;
	gcd[13].gd.flags = gg_enabled|gg_visible;
	gcd[13].creator = GLineCreate;

	gcd[14].gd.pos.x = 20-3; gcd[14].gd.pos.y = PI_Height-35-3;
	gcd[14].gd.pos.width = -1; gcd[14].gd.pos.height = 0;
	gcd[14].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[14].text = (unichar_t *) _STR_OK;
	label[14].text_in_resource = true;
	gcd[14].gd.mnemonic = 'O';
	gcd[14].gd.label = &label[14];
	gcd[14].gd.handle_controlevent = PI_Ok;
	gcd[14].creator = GButtonCreate;

	gcd[15].gd.pos.x = -20; gcd[15].gd.pos.y = PI_Height-35;
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

	label[19].text = (unichar_t *) _STR_Offset;
	label[19].text_in_resource = true;
	gcd[19].gd.label = &label[19];
	gcd[19].gd.pos.x = gcd[3].gd.pos.x+10; gcd[19].gd.pos.y = gcd[5].gd.pos.y+6; 
	gcd[19].gd.flags = gg_enabled|gg_visible;
	gcd[19].creator = GLabelCreate;

	label[20].text = (unichar_t *) _STR_Offset;
	label[20].text_in_resource = true;
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

    if ( !CVOneThingSel(cv,&sp,&spl,&ref,&img)) {
	if ( cv->fv->cidmaster==NULL )
	    SCGetInfo(cv->sc,false);
    } else if ( ref!=NULL )
	RefGetInfo(cv,ref);
    else if ( img!=NULL )
	ImgGetInfo(cv,img);
    else
	PointGetInfo(cv,sp,spl);
}

void SCRefBy(SplineChar *sc) {
    static int buts[] = { _STR_Show, _STR_Cancel };
    int cnt,i,tot=0;
    unichar_t **deps = NULL;
    struct splinecharlist *d;

    for ( i=0; i<2; ++i ) {
	cnt = 0;
	for ( d = sc->dependents; d!=NULL; d=d->next ) {
	    if ( deps!=NULL )
		deps[tot-cnt] = uc_copy(d->sc->name);
	    ++cnt;
	}
	if ( cnt==0 )
return;
	if ( i==0 )
	    deps = gcalloc(cnt+1,sizeof(unichar_t *));
	tot = cnt-1;
    }

    i = GWidgetChoicesBR(_STR_Dependents,(const unichar_t **) deps, cnt, 0, buts, _STR_Dependents );
    if ( i!=-1 ) {
	i = tot-i;
	for ( d = sc->dependents, cnt=0; d!=NULL && cnt<i; d=d->next, ++cnt );
	CharViewCreate(d->sc,sc->parent->fv);
    }
    for ( i=0; i<=tot; ++i )
	free( deps[i] );
    free(deps);
}
