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
    AnchorPoint *ap;
    SplinePoint *cursp;
    SplinePointList *curspl;
    SplinePointList *oldstate;
    AnchorPoint *oldaps;
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
#define CID_Script	1010
#define CID_LigTag	1011

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

#define CID_X		3001
#define CID_Y		3002
#define CID_NameList	3003
#define CID_Mark	3004
#define CID_BaseChar	3005
#define CID_BaseLig	3006
#define CID_BaseMark	3007
#define CID_LigIndex	3008
#define CID_Next	3009
#define CID_Prev	3010
#define CID_Delete	3011
#define CID_New		3012

#define CI_Width	218
#define CI_Height	242

#define RI_Width	215
#define RI_Height	180

#define II_Width	130
#define II_Height	70

#define PI_Width	218
#define PI_Height	200

#define AI_Width	160
#define AI_Height	220

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

static GTextInfo scripts[] = {
    { (unichar_t *) _STR_Arab, NULL, 0, 0, (void *) CHR('a','r','a','b'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Armn, NULL, 0, 0, (void *) CHR('a','r','m','n'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Beng, NULL, 0, 0, (void *) CHR('b','e','n','g'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Bopo, NULL, 0, 0, (void *) CHR('b','o','p','o'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Brai, NULL, 0, 0, (void *) CHR('b','r','a','i'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Byzm, NULL, 0, 0, (void *) CHR('b','y','z','m'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Cans, NULL, 0, 0, (void *) CHR('c','a','n','s'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Cher, NULL, 0, 0, (void *) CHR('c','h','e','r'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Hani, NULL, 0, 0, (void *) CHR('h','a','n','i'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Cyrl, NULL, 0, 0, (void *) CHR('c','y','r','l'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_DFLT, NULL, 0, 0, (void *) CHR('D','F','L','T'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Deva, NULL, 0, 0, (void *) CHR('d','e','v','a'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Ethi, NULL, 0, 0, (void *) CHR('e','t','h','i'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Geor, NULL, 0, 0, (void *) CHR('g','e','o','r'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Grek, NULL, 0, 0, (void *) CHR('g','r','e','k'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Gujr, NULL, 0, 0, (void *) CHR('g','u','j','r'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Guru, NULL, 0, 0, (void *) CHR('g','u','r','u'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Jamo, NULL, 0, 0, (void *) CHR('j','a','m','o'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Hang, NULL, 0, 0, (void *) CHR('h','a','n','g'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Hebr, NULL, 0, 0, (void *) CHR('h','e','b','r'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Kana, NULL, 0, 0, (void *) CHR('k','a','n','a'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Knda, NULL, 0, 0, (void *) CHR('k','n','d','a'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Khmr, NULL, 0, 0, (void *) CHR('k','h','m','r'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Lao , NULL, 0, 0, (void *) CHR('l','a','o',' '), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Latn, NULL, 0, 0, (void *) CHR('l','a','t','n'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Mlym, NULL, 0, 0, (void *) CHR('m','l','y','m'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Mong, NULL, 0, 0, (void *) CHR('m','o','n','g'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Mymr, NULL, 0, 0, (void *) CHR('m','y','m','r'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Ogam, NULL, 0, 0, (void *) CHR('o','g','a','m'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Orya, NULL, 0, 0, (void *) CHR('o','r','y','a'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Runr, NULL, 0, 0, (void *) CHR('r','u','n','r'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Sinh, NULL, 0, 0, (void *) CHR('s','i','n','h'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Syrc, NULL, 0, 0, (void *) CHR('s','y','r','c'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Taml, NULL, 0, 0, (void *) CHR('t','a','m','l'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Telu, NULL, 0, 0, (void *) CHR('t','e','l','u'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Thaa, NULL, 0, 0, (void *) CHR('t','h','a','a'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Thai, NULL, 0, 0, (void *) CHR('t','h','a','i'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Tibt, NULL, 0, 0, (void *) CHR('t','i','b','t'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Yi  , NULL, 0, 0, (void *) CHR('y','i',' ',' '), NULL, false, false, false, false, false, false, false, true },
    { NULL }
};

static GTextInfo ligature_tags[] = {
    { (unichar_t *) _STR_Discretionary, NULL, 0, 0, (void *) CHR('d','l','i','g'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Historic, NULL, 0, 0, (void *) CHR('h','l','i','g'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Required, NULL, 0, 0, (void *) CHR('r','l','i','g'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Standard, NULL, 0, 0, (void *) CHR('l','i','g','a'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Fraction, NULL, 0, 0, (void *) CHR('f','r','a','c'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_AltFrac, NULL, 0, 0, (void *) CHR('a','f','r','c'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_AboveBaseSubs, NULL, 0, 0, (void *) CHR('a','b','v','s'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_BelowBaseForms, NULL, 0, 0, (void *) CHR('b','l','w','f'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_BelowBaseSubs, NULL, 0, 0, (void *) CHR('b','l','w','s'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Akhand, NULL, 0, 0, (void *) CHR('a','k','h','n'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_GlyphCompDecomp, NULL, 0, 0, (void *) CHR('c','c','m','p'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_HalfForm, NULL, 0, 0, (void *) CHR('h','a','l','f'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_HalantForm, NULL, 0, 0, (void *) CHR('h','a','l','n'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_LeadingJamo, NULL, 0, 0, (void *) CHR('l','j','m','o'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_TrailingJamo, NULL, 0, 0, (void *) CHR('t','j','m','o'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_VowelJamo, NULL, 0, 0, (void *) CHR('v','j','m','o'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Nukta, NULL, 0, 0, (void *) CHR('n','u','k','f'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_PreBaseForms, NULL, 0, 0, (void *) CHR('p','r','e','f'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_PreBaseSubs, NULL, 0, 0, (void *) CHR('p','r','e','s'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_PostBaseForms, NULL, 0, 0, (void *) CHR('p','s','t','f'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_PostBaseSubs, NULL, 0, 0, (void *) CHR('p','s','t','s'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Reph, NULL, 0, 0, (void *) CHR('r','e','p','h'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_VattuVariants, NULL, 0, 0, (void *) CHR('v','a','t','u'), NULL, false, false, false, false, false, false, false, true },
    { NULL }
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
    const unichar_t *curname = GGadgetGetTitle(GWidgetGetControl(gw,cid));

    if ( val>=0 && val<psunicodenames_cnt && psunicodenames[val]!=NULL )
	temp = uc_copy(psunicodenames[val]);
/* If it a control char is already called ".notdef" then give it a uniXXXX style name */
    else if (( val>=32 && val<0x7f ) || val>=0xa1 ||
	    (uc_strcmp(curname,".notdef")==0 && val!=0)) {
	if ( val>= 0x10000 )
	    sprintf( buf,"u%04X", val );	/* u style names may contain 4,5 or 6 hex digits */
	else
	    sprintf( buf,"uni%04X", val );
	temp = uc_copy(buf);
    } else
	temp = uc_copy(".notdef");
    GGadgetSetTitle(GWidgetGetControl(gw,cid),temp);
    free(temp);
}

static void SetScriptFromTag(GWindow gw,int tag) {
    unichar_t ubuf[8];

    ubuf[0] = tag>>24;
    ubuf[1] = (tag>>16)&0xff;
    ubuf[2] = (tag>>8)&0xff;
    ubuf[3] = tag&0xff;
    ubuf[4] = 0;
    GGadgetSetTitle(GWidgetGetControl(gw,CID_Script),ubuf);
}

static void SetScriptFromUnicode(GWindow gw,int uni,SplineFont *sf) {
    SetScriptFromTag(gw,ScriptFromUnicode(uni,sf));
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
		
static void LigSet(SplineChar *sc,char *lig,uint32 ltag) {

    if ( lig==NULL || *lig=='\0' ) {
	LigatureFree(sc->lig);
	sc->lig=NULL;
    } else {
	LigatureFree(sc->lig);
	sc->lig = gcalloc(1,sizeof(Ligature));
	sc->lig->components = copy(lig);
	sc->lig->lig = sc;
	sc->lig->tag = ltag;
    }
}

int SCSetMetaData(SplineChar *sc,char *name,int unienc,char *lig,uint32 ltag) {
    SplineFont *sf = sc->parent;
    int i, mv=0;
    int isnotdef, samename=false;

    if ( !LigCheck(sf,sc,lig))
return( false );

    if ( sc->unicodeenc == unienc && strcmp(name,sc->name)==0 ) {
	samename = true;	/* No change, it must be good */
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
		sf->chars[i]->namechanged = true;
		sc->name = NULL;
	    break;
	    }
	}
    }
    if ( sc->unicodeenc!=unienc )
	sc->script = ScriptFromUnicode(unienc,sc->parent);
    sc->unicodeenc = unienc;
    if ( strcmp(name,sc->name)!=0 ) {
	free(sc->name);
	sc->name = copy(name);
	sc->namechanged = true;
    }
    sf->changed = true;
    if ( (sf->encoding_name==em_unicode || sf->encoding_name==em_unicode4) &&
	    unienc==sc->enc && unienc>=0xe000 && unienc<=0xf8ff )
	/* Ok to name things in the private use area */;
    else if ( samename )
	/* Ok to name it itself */;
    else if ( (sf->encoding_name<e_first2byte && sc->enc<256) ||
	    (sf->encoding_name>=em_big5 && sf->encoding_name<=em_unicode && sc->enc<65536 ) ||
	    (sf->encoding_name>=e_first2byte && sf->encoding_name<em_unicode && sc->enc<94*96 ) ||
	    sc->unicodeenc!=-1 )
	sf->encoding_name = em_none;
    LigSet(sc,lig,ltag);
    SCRefreshTitles(sc);
return( true );
}

static uint32 ParseTag(GWindow gw,int cid,int msg,int *err) {
    const unichar_t *utag = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    uint32 tag;
    unichar_t ubuf[8];

    if ( (ubuf[0] = utag[0])==0 ) {
return( 0 );
    } else {
	if ( (ubuf[1] = utag[1])==0 )
	    ubuf[1] = ubuf[2] = ubuf[3] = ' ';
	else if ( (ubuf[2] = utag[2])==0 )
	    ubuf[2] = ubuf[3] = ' ';
	else if ( (ubuf[3] = utag[3])==0 )
	    ubuf[3] = ' ';
	tag = (ubuf[0]<<24) | (ubuf[1]<<16) | (ubuf[2]<<8) | ubuf[3];
    }
    if ( u_strlen(utag)>4 || ubuf[0]>=0x7f || ubuf[1]>=0x7f || ubuf[2]>=0x7f || ubuf[3]>=0x7f ) {
	GWidgetErrorR(_STR_TagTooLong,msg);
	*err = true;
return( 0 );
    }
return( tag );
}

static int _CI_OK(GIData *ci) {
    int val;
    int ret, refresh_fvdi=0;
    uint32 tag, ltag;
    char *lig, *name;
    const unichar_t *comment;
    FontView *fvs;
    int err = false;

    val = ParseUValue(ci->gw,CID_UValue,true,ci->sc->parent);
    if ( val==-2 )
return( false );
    lig = cu_copy( _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_Ligature)) );
    name = cu_copy( _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UName)) );
    tag = ParseTag(ci->gw,CID_Script,_STR_ScriptTagTooLong,&err);
    ltag = ParseTag(ci->gw,CID_LigTag,_STR_FeatureTagTooLong,&err);
    if ( err )
return( false );
    if ( strcmp(name,ci->sc->name)!=0 || val!=ci->sc->unicodeenc )
	refresh_fvdi = 1;
    ret = SCSetMetaData(ci->sc,name,val,lig,ltag);
    if ( ret ) ci->sc->script = tag;
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
    unichar_t ubuf[8];
    uint32 tag;

    free(namtemp);
    if ( temp==NULL )
return;
    components = uc_copy(temp);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_Ligature),components);
    free(components);

    tag = LigTagFromUnicode(uni);
    if ( tag!=0 ) {
	ubuf[0] = tag>>24;
	ubuf[1] = (tag>>16)&0xff;
	ubuf[2] = (tag>>8)&0xff;
	ubuf[3] = tag&0xff;
	ubuf[4] = 0;
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_LigTag),ubuf);
    }
    if ( *_GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_Script))=='\0' ) {
	/* Use the script of the first component if we can */
	char *pt;
	uint32 script;
	if ( (pt=strchr(temp,' '))!=NULL ) *pt = '\0';
	script = ScriptFromUnicode(UniFromName(temp),ci->sc->parent);
	if ( script==0 ) {
	    int index = SFFindChar(ci->sc->parent,-1,temp);
	    if ( index!=-1 && ci->sc->parent->chars[index] )
		script = ci->sc->parent->chars[index]->script;
	}
	if ( script!=0 ) {
	    ubuf[0] = script>>24;
	    ubuf[1] = (script>>16)&0xff;
	    ubuf[2] = (script>>8)&0xff;
	    ubuf[3] = script&0xff;
	    ubuf[4] = 0;
	    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_LigTag),ubuf);
	}
    }
    free(temp);
}

static int psnamesinited=false;
#define HASH_SIZE	257
struct psbucket { const char *name; int uni; struct psbucket *prev; } *psbuckets[HASH_SIZE];

static int hashname(const char *name) {
    /* Name is assumed to be ascii */
    int hash=0;

    while ( *name ) {
	if ( *name<=' ' || *name>=0x7f )
    break;
	hash = (hash<<3)|((hash>>29)&0x7);
	hash ^= *name++-(' '+1);
    }
    hash ^= (hash>>16);
    hash &= 0xffff;
return( hash%HASH_SIZE );
}

static void psaddbucket(const char *name, int uni) {
    int hash = hashname(name);
    struct psbucket *buck = gcalloc(1,sizeof(struct psbucket));

    buck->name = name;
    buck->uni = uni;
    buck->prev = psbuckets[hash];
    psbuckets[hash] = buck;
}

static void psinitnames(void) {
    int i;

    for ( i=psaltuninames_cnt-1; i>=0 ; --i )
	psaddbucket(psaltuninames[i].name,psaltuninames[i].unicode);
    for ( i=psunicodenames_cnt-1; i>=0 ; --i )
	if ( psunicodenames[i]!=NULL )
	    psaddbucket(psunicodenames[i],i);
    psnamesinited = true;
}

int UniFromName(const char *name) {
    int i = -1;
    char *end;
    struct psbucket *buck;

    if ( strncmp(name,"uni",3)==0 ) {
	i = strtol(name+3,&end,16);
	if ( *end || end-name!=7 )	/* uniXXXXXXXX means a ligature of uniXXXX and uniXXXX */
	    i = -1;
    } else if ( name[0]=='u' && strlen(name)>=5 ) {
	i = strtol(name+1,&end,16);
	if ( *end )
	    i = -1;
    }
    if ( i==-1 ) {
	if ( !psnamesinited )
	    psinitnames();
	for ( buck = psbuckets[hashname(name)]; buck!=NULL; buck=buck->prev )
	    if ( strcmp(buck->name,name)==0 )
	break;
	if ( buck!=NULL )
	    i = buck->uni;
    }
#if 0
    if ( i==-1 ) {
	for ( i=65535; i>=0; --i )
	    if ( UnicodeCharacterNames[i>>8][i&0xff]!=NULL &&
		    uc_strcmp(UnicodeCharacterNames[i>>8][i&0xff],name)==0 )
	break;
    }
#endif
return( i );
}

int uUniFromName(const unichar_t *name) {
    int i = -1;
    unichar_t *end;

    if ( uc_strncmp(name,"uni",3)==0 ) {
	i = u_strtol(name+3,&end,16);
	if ( *end || end-name!=7 )	/* uniXXXXXXXX means a ligature of uniXXXX and uniXXXX */
	    i = -1;
    } else if ( name[0]=='u' && u_strlen(name)>=5 ) {
	i = u_strtol(name+1,&end,16);
	if ( *end )
	    i = -1;
    }
    if ( i==-1 ) for ( i=psunicodenames_cnt-1; i>=0 ; --i ) {
	if ( psunicodenames[i]!=NULL )
	    if ( uc_strcmp(name,psunicodenames[i])==0 )
    break;
    }
    if ( i==-1 ) for ( i=psaltuninames_cnt-1; i>=0 ; --i ) {
	if ( uc_strcmp(name,psaltuninames[i].name)==0 )
    break;
    }
#if 0
    if ( i==-1 ) {
	for ( i=65535; i>=0; --i )
	    if ( UnicodeCharacterNames[i>>8][i&0xff]!=NULL &&
		    u_strcmp(name,UnicodeCharacterNames[i>>8][i&0xff])==0 )
	break;
    }
#endif
return( i );
}

static int CI_SName(GGadget *g, GEvent *e) {	/* Set From Name */
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UName));
	int i;
	char buf[10]; unichar_t ubuf[2], *temp;
	i = uUniFromName(ret);
	for ( i=psunicodenames_cnt-1; i>=0; --i )
	    if ( psunicodenames[i]!=NULL && uc_strcmp(ret,psunicodenames[i])==0 )
	break;
	if ( i==-1 ) {
	    /* Adobe says names like uni00410042 represent a ligature (A&B) */
	    /*  (that is "uni" followed by two 4-digit codes). */
	    /* But that names outside of BMP should be uXXXX or uXXXXX or uXXXXXX */
	    if ( ret[0]=='u' && ret[1]!='n' && u_strlen(ret)<=1+6 ) {
		unichar_t *end;
		i = u_strtol(ret+1,&end,16);
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

	SetScriptFromUnicode(ci->gw,i,ci->sc->parent);

	ubuf[0] = i;
	if ( i==-1 || i>0xffff )
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
	SetScriptFromUnicode(ci->gw,val,ci->sc->parent);

	ubuf[0] = val;
	if ( val==-1 )
	    ubuf[0] = '\0';
	ubuf[1] = '\0';
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);
    }
return( true );
}

static GTextInfo *TIFromName(const char *name) {
    GTextInfo *ti = gcalloc(1,sizeof(GTextInfo));
    ti->text = uc_copy(name);
    ti->fg = COLOR_DEFAULT;
    ti->bg = COLOR_DEFAULT;
return( ti );
}

static void CI_SetNameList(GIData *ci,int val) {
    GGadget *g = GWidgetGetControl(ci->gw,CID_UName);
    int cnt, i;
    char buffer[20];

    if ( GGadgetGetUserData(g)==(void *) val )
return;		/* Didn't change */
    if ( val<0 || val>=0x1000000 ) {
	static GTextInfo notdef = { (unichar_t *) ".notdef", NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
			 empty = { NULL },
			 *list[] = { &notdef, &empty };
	GGadgetSetList(g,list,true);
    } else {
	GTextInfo **list = NULL;
	while ( 1 ) {
	    cnt=0;
	    if ( val<psunicodenames_cnt && psunicodenames[val]!=NULL ) {
		if ( list ) list[cnt] = TIFromName(psunicodenames[val]);
		++cnt;
	    } else if ( val<32 || (val>=0x7f && val<0xa0)) {
		if ( list ) list[cnt] = TIFromName(".notdef");
		++cnt;
	    }
	    for ( i=0; psaltuninames[i].name!=NULL; ++i )
		if ( psaltuninames[i].unicode==val ) {
		    if ( list ) list[cnt] = TIFromName(psaltuninames[i].name);
		    ++cnt;
		}
	    if ( val<0x10000 ) {
		if ( list ) {
		    sprintf( buffer, "uni%04X", val);
		    list[cnt] = TIFromName(buffer);
		}
		++cnt;
	    }
	    if ( list ) {
		sprintf( buffer, "u%04X", val);
		list[cnt] = TIFromName(buffer);
		list[cnt+1] = TIFromName(NULL);
	    }
	    ++cnt;
	    if ( list!=NULL )
	break;
	    list = galloc((cnt+1)*sizeof(GTextInfo*)); 
	}
	GGadgetSetList(g,list,true);
    }
    GGadgetSetUserData(g,(void *) val);
}

static int CI_UValChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UValue));
	unichar_t *end;
	int val;

	if (( *ret=='U' || *ret=='u' ) && ret[1]=='+' )
	    ret += 2;
	val = u_strtol(ret,&end,16);
	if ( *end=='\0' )
	    CI_SetNameList(ci,val);
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
	CI_SetNameList(ci,val);

	sprintf(buf,"U+%04x", val);
	temp = uc_copy(buf);
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UValue),temp);
	free(temp);
    }
return( true );
}

static int CI_ScriptChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged &&
	    e->u.control.u.tf_changed.from_pulldown!=-1 ) {
	uint32 tag = (uint32) scripts[e->u.control.u.tf_changed.from_pulldown].userdata;
	unichar_t ubuf[8];
	/* If they select something from the pulldown, don't show the human */
	/*  readable form, instead show the 4 character tag */
	ubuf[0] = tag>>24;
	ubuf[1] = (tag>>16)&0xff;
	ubuf[2] = (tag>>8)&0xff;
	ubuf[3] = tag&0xff;
	ubuf[4] = 0;
	GGadgetSetTitle(g,ubuf);
    }
return( true );
}

static int CI_LigTagChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged &&
	    e->u.control.u.tf_changed.from_pulldown!=-1 ) {
	uint32 tag = (uint32) ligature_tags[e->u.control.u.tf_changed.from_pulldown].userdata;
	unichar_t ubuf[8];
	/* If they select something from the pulldown, don't show the human */
	/*  readable form, instead show the 4 character tag */
	ubuf[0] = tag>>24;
	ubuf[1] = (tag>>16)&0xff;
	ubuf[2] = (tag>>8)&0xff;
	ubuf[3] = tag&0xff;
	ubuf[4] = 0;
	GGadgetSetTitle(g,ubuf);
    }
return( true );
}

static int CI_CommentChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	/* Let's give things with comments a white color. This may not be a good idea */
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
    unichar_t ubuf[8];
    const unichar_t *bits;
    int i;

    GGadgetSetEnabled(GWidgetGetControl(ci->gw,-1),sc->enc>0);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,1),sc->enc<sf->charcnt-1);

    temp = uc_copy(sc->name);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UName),temp);
    free(temp);
    CI_SetNameList(ci,sc->unicodeenc);

    sprintf(buffer,"U+%04x", sc->unicodeenc);
    temp = uc_copy(sc->unicodeenc==-1?"-1":buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UValue),temp);
    free(temp);

    ubuf[0] = sc->unicodeenc;
    if ( sc->unicodeenc==-1 )
	ubuf[0] = '\0';
    ubuf[1] = '\0';
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);

    SetScriptFromTag(ci->gw,sc->script);

    if ( sc->lig!=NULL ) {
	for ( i=0; ligature_tags[i].text!=NULL; ++i )
	    if ( (uint32) ligature_tags[i].userdata == sc->lig->tag )
	break;
	if ( ligature_tags[i].text!=NULL )
	    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_LigTag),i);
	temp = uc_copy(sc->lig->components);
	ubuf[0] = sc->lig->tag>>24;
	ubuf[1] = (sc->lig->tag>>16)&0xff;
	ubuf[2] = (sc->lig->tag>>8)&0xff;
	ubuf[3] = sc->lig->tag&0xff;
	ubuf[4] = 0;
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_LigTag),ubuf);
    } else
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
    GGadgetCreateData ugcd[12], cgcd[6], lgcd[8], mgcd[9];
    GTextInfo ulabel[12], clabel[6], llabel[8], mlabel[9];
    int i;
    GTabInfo aspects[5];

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

	memset(&ugcd,0,sizeof(ugcd));
	memset(&ulabel,0,sizeof(ulabel));

	ulabel[0].text = (unichar_t *) _STR_UnicodeName;
	ulabel[0].text_in_resource = true;
	ugcd[0].gd.label = &ulabel[0];
	ugcd[0].gd.pos.x = 5; ugcd[0].gd.pos.y = 5+4; 
	ugcd[0].gd.flags = gg_enabled|gg_visible;
	ugcd[0].gd.mnemonic = 'N';
	ugcd[0].creator = GLabelCreate;

	ugcd[1].gd.pos.x = 85; ugcd[1].gd.pos.y = 5;
	ugcd[1].gd.flags = gg_enabled|gg_visible;
	ugcd[1].gd.mnemonic = 'N';
	ugcd[1].gd.cid = CID_UName;
	ugcd[1].creator = GListFieldCreate;
	ugcd[1].data = (void *) (-2);

	ulabel[2].text = (unichar_t *) _STR_UnicodeValue;
	ulabel[2].text_in_resource = true;
	ugcd[2].gd.label = &ulabel[2];
	ugcd[2].gd.pos.x = 5; ugcd[2].gd.pos.y = 31+4; 
	ugcd[2].gd.flags = gg_enabled|gg_visible;
	ugcd[2].gd.mnemonic = 'V';
	ugcd[2].creator = GLabelCreate;

	ugcd[3].gd.pos.x = 85; ugcd[3].gd.pos.y = 31;
	ugcd[3].gd.flags = gg_enabled|gg_visible;
	ugcd[3].gd.mnemonic = 'V';
	ugcd[3].gd.cid = CID_UValue;
	ugcd[3].gd.handle_controlevent = CI_UValChanged;
	ugcd[3].creator = GTextFieldCreate;

	ulabel[4].text = (unichar_t *) _STR_UnicodeChar;
	ulabel[4].text_in_resource = true;
	ugcd[4].gd.label = &ulabel[4];
	ugcd[4].gd.pos.x = 5; ugcd[4].gd.pos.y = 57+4; 
	ugcd[4].gd.flags = gg_enabled|gg_visible;
	ugcd[4].gd.mnemonic = 'h';
	ugcd[4].creator = GLabelCreate;

	ugcd[5].gd.pos.x = 85; ugcd[5].gd.pos.y = 57;
	ugcd[5].gd.flags = gg_enabled|gg_visible|gg_text_xim;
	ugcd[5].gd.mnemonic = 'h';
	ugcd[5].gd.cid = CID_UChar;
	ugcd[5].gd.handle_controlevent = CI_CharChanged;
	ugcd[5].creator = GTextFieldCreate;

	ulabel[6].text = (unichar_t *) _STR_ScriptC;
	ulabel[6].text_in_resource = true;
	ugcd[6].gd.label = &ulabel[6];
	ugcd[6].gd.pos.x = 5; ugcd[6].gd.pos.y = 83+4; 
	ugcd[6].gd.flags = gg_enabled|gg_visible;
	ugcd[6].creator = GLabelCreate;

	ugcd[7].gd.pos.x = 85; ugcd[7].gd.pos.y = 83;
	ugcd[7].gd.flags = gg_enabled|gg_visible;
	ugcd[7].gd.mnemonic = 'h';
	ugcd[7].gd.cid = CID_Script;
	ugcd[7].gd.u.list = scripts;
	ugcd[7].gd.handle_controlevent = CI_ScriptChanged;
	ugcd[7].creator = GListFieldCreate;

	ugcd[8].gd.pos.x = 12; ugcd[8].gd.pos.y = 117;
	ugcd[8].gd.flags = gg_visible | gg_enabled;
	ulabel[8].text = (unichar_t *) _STR_SetFromName;
	ulabel[8].text_in_resource = true;
	ugcd[8].gd.mnemonic = 'a';
	ugcd[8].gd.label = &ulabel[8];
	ugcd[8].gd.handle_controlevent = CI_SName;
	ugcd[8].creator = GButtonCreate;

	ugcd[9].gd.pos.x = 107; ugcd[9].gd.pos.y = 117;
	ugcd[9].gd.flags = gg_visible | gg_enabled;
	ulabel[9].text = (unichar_t *) _STR_SetFromValue;
	ulabel[9].text_in_resource = true;
	ugcd[9].gd.mnemonic = 'l';
	ugcd[9].gd.label = &ulabel[9];
	ugcd[9].gd.handle_controlevent = CI_SValue;
	ugcd[9].creator = GButtonCreate;

	memset(&cgcd,0,sizeof(cgcd));
	memset(&clabel,0,sizeof(clabel));

	clabel[0].text = (unichar_t *) _STR_Comment;
	clabel[0].text_in_resource = true;
	cgcd[0].gd.label = &clabel[0];
	cgcd[0].gd.pos.x = 5; cgcd[0].gd.pos.y = 5; 
	cgcd[0].gd.flags = gg_enabled|gg_visible;
	cgcd[0].creator = GLabelCreate;

	cgcd[1].gd.pos.x = 5; cgcd[1].gd.pos.y = cgcd[0].gd.pos.y+13;
	cgcd[1].gd.pos.width = CI_Width-20;
	cgcd[1].gd.pos.height = 7*12+6;
	cgcd[1].gd.flags = gg_enabled|gg_visible|gg_textarea_wrap|gg_text_xim;
	cgcd[1].gd.cid = CID_Comment;
	cgcd[1].gd.handle_controlevent = CI_CommentChanged;
	cgcd[1].creator = GTextAreaCreate;

	clabel[2].text = (unichar_t *) _STR_Color;
	clabel[2].text_in_resource = true;
	cgcd[2].gd.label = &clabel[2];
	cgcd[2].gd.pos.x = 5; cgcd[2].gd.pos.y = cgcd[1].gd.pos.y+cgcd[1].gd.pos.height+5+6; 
	cgcd[2].gd.flags = gg_enabled|gg_visible;
	cgcd[2].creator = GLabelCreate;

	cgcd[3].gd.pos.x = cgcd[3].gd.pos.x; cgcd[3].gd.pos.y = cgcd[2].gd.pos.y-6;
	cgcd[3].gd.pos.width = cgcd[3].gd.pos.width;
	cgcd[3].gd.flags = gg_enabled|gg_visible;
	cgcd[3].gd.cid = CID_Color;
	cgcd[3].gd.u.list = std_colors;
	cgcd[3].creator = GListButtonCreate;

	memset(&lgcd,0,sizeof(lgcd));
	memset(&llabel,0,sizeof(llabel));

	llabel[0].text = (unichar_t *) _STR_Ligature;
	llabel[0].text_in_resource = true;
	lgcd[0].gd.label = &llabel[0];
	lgcd[0].gd.pos.x = 5; lgcd[0].gd.pos.y = 5+4; 
	lgcd[0].gd.flags = gg_enabled|gg_visible;
	lgcd[0].gd.mnemonic = 'L';
	lgcd[0].gd.popup_msg = GStringGetResource(_STR_Ligpop,NULL);
	lgcd[0].creator = GLabelCreate;

	lgcd[1].gd.pos.x = 85; lgcd[1].gd.pos.y = 5;
	lgcd[1].gd.flags = gg_enabled|gg_visible;
	lgcd[1].gd.mnemonic = 'L';
	lgcd[1].gd.cid = CID_Ligature;
	lgcd[1].gd.popup_msg = GStringGetResource(_STR_Ligpop,NULL);
	lgcd[1].creator = GTextFieldCreate;

	llabel[2].text = (unichar_t *) _STR_TagC;
	llabel[2].text_in_resource = true;
	lgcd[2].gd.label = &llabel[2];
	lgcd[2].gd.pos.x = 5; lgcd[2].gd.pos.y = lgcd[1].gd.pos.y+24+4; 
	lgcd[2].gd.flags = gg_enabled|gg_visible;
	lgcd[2].creator = GLabelCreate;

	lgcd[3].gd.pos.x = 85; lgcd[3].gd.pos.y = lgcd[2].gd.pos.y-4;
	lgcd[3].gd.flags = gg_enabled|gg_visible;
	lgcd[3].gd.cid = CID_LigTag;
	lgcd[3].gd.u.list = ligature_tags;
	lgcd[3].gd.handle_controlevent = CI_LigTagChanged;
	lgcd[3].creator = GListFieldCreate;

	lgcd[4].gd.pos.x = 5; lgcd[4].gd.pos.y = lgcd[3].gd.pos.y+26;
	lgcd[4].gd.pos.width = CI_Width-10;
	lgcd[4].gd.flags = gg_visible | gg_enabled;
	lgcd[4].creator = GLineCreate;

	llabel[5].text = (unichar_t *) _STR_AccentedComponents;
	llabel[5].text_in_resource = true;
	lgcd[5].gd.label = &llabel[5];
	lgcd[5].gd.pos.x = 5; lgcd[5].gd.pos.y = lgcd[4].gd.pos.y+5; 
	lgcd[5].gd.flags = gg_enabled|gg_visible;
	lgcd[5].gd.cid = CID_ComponentMsg;
	/*lgcd[5].gd.popup_msg = GStringGetResource(_STR_Ligpop,NULL);*/
	lgcd[5].creator = GLabelCreate;

	lgcd[6].gd.pos.x = 5; lgcd[6].gd.pos.y = lgcd[5].gd.pos.y+12;
	lgcd[6].gd.pos.width = CI_Width-20;
	lgcd[6].gd.flags = gg_enabled|gg_visible;
	lgcd[6].gd.cid = CID_Components;
	/*lgcd[6].gd.popup_msg = GStringGetResource(_STR_Ligpop,NULL);*/
	lgcd[6].creator = GLabelCreate;

	memset(&mgcd,0,sizeof(mgcd));
	memset(&mlabel,0,sizeof(mlabel));
	memset(&aspects,'\0',sizeof(aspects));

	i = 0;
	aspects[i].text = (unichar_t *) _STR_UnicodeL;
	aspects[i].text_in_resource = true;
	aspects[i].selected = true;
	aspects[i++].gcd = ugcd;

	aspects[i].text = (unichar_t *) _STR_Comment;
	aspects[i].text_in_resource = true;
	aspects[i++].gcd = cgcd;

	aspects[i].text = (unichar_t *) _STR_LigatureL;
	aspects[i].text_in_resource = true;
	aspects[i++].gcd = lgcd;

	mgcd[0].gd.pos.x = 4; mgcd[0].gd.pos.y = 6;
	mgcd[0].gd.pos.width = CI_Width-10;
	mgcd[0].gd.pos.height = CI_Height-70;
	mgcd[0].gd.u.tabs = aspects;
	mgcd[0].gd.flags = gg_visible | gg_enabled;
	mgcd[0].creator = GTabSetCreate;

	mgcd[1].gd.pos.x = 40; mgcd[1].gd.pos.y = mgcd[0].gd.pos.y+mgcd[0].gd.pos.height+3;
	mgcd[1].gd.pos.width = -1; mgcd[1].gd.pos.height = 0;
	mgcd[1].gd.flags = gg_visible | gg_enabled ;
	mlabel[1].text = (unichar_t *) _STR_PrevArrow;
	mlabel[1].text_in_resource = true;
	mgcd[1].gd.mnemonic = 'P';
	mgcd[1].gd.label = &mlabel[1];
	mgcd[1].gd.handle_controlevent = CI_NextPrev;
	mgcd[1].gd.cid = -1;
	mgcd[1].creator = GButtonCreate;

	mgcd[2].gd.pos.x = -40; mgcd[2].gd.pos.y = mgcd[1].gd.pos.y;
	mgcd[2].gd.pos.width = -1; mgcd[2].gd.pos.height = 0;
	mgcd[2].gd.flags = gg_visible | gg_enabled ;
	mlabel[2].text = (unichar_t *) _STR_NextArrow;
	mlabel[2].text_in_resource = true;
	mgcd[2].gd.label = &mlabel[2];
	mgcd[2].gd.mnemonic = 'N';
	mgcd[2].gd.handle_controlevent = CI_NextPrev;
	mgcd[2].gd.cid = 1;
	mgcd[2].creator = GButtonCreate;

	mgcd[3].gd.pos.x = 25-3; mgcd[3].gd.pos.y = CI_Height-31-3;
	mgcd[3].gd.pos.width = -1; mgcd[3].gd.pos.height = 0;
	mgcd[3].gd.flags = gg_visible | gg_enabled | gg_but_default;
	mlabel[3].text = (unichar_t *) _STR_OK;
	mlabel[3].text_in_resource = true;
	mgcd[3].gd.mnemonic = 'O';
	mgcd[3].gd.label = &mlabel[3];
	mgcd[3].gd.handle_controlevent = CI_OK;
	mgcd[3].creator = GButtonCreate;

	mgcd[4].gd.pos.x = -25; mgcd[4].gd.pos.y = mgcd[3].gd.pos.y+3;
	mgcd[4].gd.pos.width = -1; mgcd[4].gd.pos.height = 0;
	mgcd[4].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	mlabel[4].text = (unichar_t *) _STR_Cancel;
	mlabel[4].text_in_resource = true;
	mgcd[4].gd.label = &mlabel[4];
	mgcd[4].gd.mnemonic = 'C';
	mgcd[4].gd.handle_controlevent = CI_Cancel;
	mgcd[4].gd.cid = CID_Cancel;
	mgcd[4].creator = GButtonCreate;

	GGadgetsCreate(gi.gw,mgcd);
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
    gi.sc = cv->sc;
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
    gi.sc = cv->sc;
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

static int IsAnchorClassUsed(SplineChar *sc,AnchorClass *an) {
    AnchorPoint *ap;
    int waslig=0;

    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
	if ( ap->anchor==an ) {
	    if ( ap->type!=at_baselig )
return( -1 );
	    else if ( waslig<ap->lig_index+1 )
		waslig = ap->lig_index+1;
	}
    }
return( waslig );
}

AnchorClass *AnchorClassUnused(SplineChar *sc,int *waslig) {
    AnchorClass *an;
    int val;
    /* Are there any anchors with this name? If so can't reuse it */
    /*  unless they are ligature anchores */

    *waslig = false;
    for ( an=sc->parent->anchor; an!=NULL; an=an->next ) {
	val = IsAnchorClassUsed(sc,an);
	if ( val!=-1 ) {
	    if ( val>0 ) *waslig = val;
return( an );
	}
    }
return( NULL );
}

static AnchorPoint *AnchorPointNew(CharView *cv) {
    AnchorClass *an;
    AnchorPoint *ap;
    int waslig;
    SplineChar *sc = cv->sc;

    an = AnchorClassUnused(sc,&waslig);
    if ( an==NULL )
return(NULL);
    ap = chunkalloc(sizeof(AnchorPoint));
    ap->anchor = an;
    ap->me.x = cv->p.cx; /* cv->p.cx = 0; */
    ap->me.y = cv->p.cy; /* cv->p.cy = 0; */
    ap->type = at_basechar;
    if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 &&
	    iscombining(sc->unicodeenc))
	ap->type = at_mark;
    else if ( an->feature_tag==CHR('m','k','m','k') )
	ap->type = at_basemark;
    else if ( sc->lig!=NULL || waslig )
	ap->type = at_baselig;
    if (( ap->type==at_basechar || ap->type==at_baselig ) && an->feature_tag==CHR('m','k','m','k') )
	ap->type = at_basemark;
    ap->next = sc->anchor;
    ap->lig_index = waslig;
    sc->anchor = ap;
return( ap );
}

static void AI_SelectList(GIData *ci,AnchorPoint *ap) {
    int i;
    AnchorClass *an;

    for ( i=0, an=ci->sc->parent->anchor; an!=ap->anchor; ++i, an=an->next );
    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_NameList),i);
}

static void AI_DisplayClass(GIData *ci,AnchorPoint *ap) {
    AnchorClass *ac = ap->anchor;
    int enable_mkmk = ac->feature_tag!=CHR('m','a','r','k') && ac->feature_tag!=CHR('a','b','v','m') &&
	    ac->feature_tag!=CHR('b','l','w','m');
    int enable_mark = ac->feature_tag!=CHR('m','k','m','k');

    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_BaseChar),enable_mark);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_BaseLig),enable_mark);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_BaseMark),enable_mkmk);

    if ( !enable_mark && (ap->type==at_basechar || ap->type==at_baselig)) {
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_BaseMark),true);
	ap->type = at_basemark;
    } else if ( !enable_mark && ap->type==at_basemark ) {
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_BaseChar),true);
	ap->type = at_basechar;
    }
}

static void AI_Display(GIData *ci,AnchorPoint *ap) {
    char val[40];
    unichar_t uval[40];
    AnchorPoint *aps;

    ci->ap = ap;
    for ( aps=ci->sc->anchor; aps!=NULL; aps=aps->next )
	aps->selected = false;
    ap->selected = true;
    sprintf(val,"%g",ap->me.x);
    uc_strcpy(uval,val);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_X),uval);
    sprintf(val,"%g",ap->me.y);
    uc_strcpy(uval,val);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_Y),uval);
    sprintf(val,"%d",ap->type==at_baselig?ap->lig_index:0);
    uc_strcpy(uval,val);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_LigIndex),uval);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_LigIndex),ap->type==at_baselig);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Next),ap->next!=NULL);
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Prev),ci->sc->anchor!=ap);

    AI_DisplayClass(ci,ap);

    switch ( ap->type ) {
      case at_mark:
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_Mark),true);
      break;
      case at_basechar:
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_BaseChar),true);
      break;
      case at_baselig:
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_BaseLig),true);
      break;
      case at_basemark:
	GGadgetSetChecked(GWidgetGetControl(ci->gw,CID_BaseMark),true);
      break;
    }

    AI_SelectList(ci,ap);
    SCUpdateAll(ci->sc);
}

static int AI_Next(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));

	if ( ci->ap->next==NULL )
return( true );
	AI_Display(ci,ci->ap->next);
    }
return( true );
}

static int AI_Prev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	AnchorPoint *ap, *prev;

	prev = NULL;
	for ( ap=ci->sc->anchor; ap!=ci->ap; ap = ap->next )
	    prev = ap;
	if ( prev==NULL )
return( true );
	AI_Display(ci,prev);
    }
return( true );
}

static int AI_Ok(GGadget *g, GEvent *e);
static int AI_Delete(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	AnchorPoint *ap, *prev;

	prev=NULL;
	for ( ap=ci->sc->anchor; ap!=ci->ap; ap=ap->next )
	    prev = ap;
	if ( prev==NULL && ci->ap->next==NULL ) {
	    static int buts[] = { _STR_Yes, _STR_No, 0 };
	    if ( GWidgetAskR(_STR_LastAnchor,buts,0,1,_STR_RemoveLastAnchor)==1 ) {
		AI_Ok(g,e);
return( true );
	    }
	}
	ap = ci->ap->next;
	if ( prev==NULL )
	    ci->sc->anchor = ap;
	else
	    prev->next = ap;
	chunkfree(ci->ap,sizeof(AnchorPoint));
	AI_Display(ci,ap);
    }
return( true );
}

static int AI_New(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int waslig;
	AnchorPoint *ap;

	if ( AnchorClassUnused(ci->sc,&waslig)==NULL ) {
	    GWidgetPostNoticeR(_STR_MakeNewClass,_STR_MakeNewAnchorClass);
	    FontInfo(ci->sc->parent,8,true);		/* Anchor Class */
	    if ( AnchorClassUnused(ci->sc,&waslig)==NULL )
return(true);
	    GGadgetSetList(GWidgetGetControl(ci->gw,CID_NameList),
		    AnchorClassesLList(ci->sc->parent),false);
	}
	ap = AnchorPointNew(ci->cv);
	if ( ap==NULL )
return( true );
	AI_Display(ci,ap);
    }
return( true );
}

static int AI_TypeChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	AnchorPoint *ap = ci->ap;

	if ( GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_Mark)) )
	    ap->type = at_mark;
	else if ( GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_BaseChar)) )
	    ap->type = at_basechar;
	else if ( GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_BaseLig)) )
	    ap->type = at_baselig;
	else if ( GGadgetIsChecked(GWidgetGetControl(ci->gw,CID_BaseMark)) )
	    ap->type = at_basemark;
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_LigIndex),ap->type==at_baselig);
    }
return( true );
}

static void AI_TestOrdering(GIData *ci,real x) {
    AnchorPoint *aps, *ap=ci->ap;
    AnchorClass *ac = ap->anchor;
    int isr2l;

    isr2l = ci->sc->script==CHR('a','r','a','b') || ci->sc->script==CHR('h','e','b','r') ||
	    (ci->sc->unicodeenc!=-1 && ci->sc->unicodeenc<0x10000 && isrighttoleft(ci->sc->unicodeenc));
    for ( aps=ci->sc->anchor; aps!=NULL; aps=aps->next ) {
	if ( aps->anchor==ac && aps!=ci->ap ) {
	    if (( aps->lig_index<ap->lig_index &&
		    ((!isr2l && aps->me.x>x) ||
		     ( isr2l && aps->me.x<x))) ||
		( aps->lig_index>ap->lig_index &&
		    (( isr2l && aps->me.x>x) ||
		     (!isr2l && aps->me.x<x))) ) {
		GWidgetErrorR(_STR_OutOfOrder,_STR_IndexOutOfOrder,aps->lig_index);
return;
	    }
	}
    }
return;
}

static int AI_LigIndexChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int index, max;
	int err=false;
	AnchorPoint *ap = ci->ap, *aps;

	index = GetCalmRealR(ci->gw,CID_LigIndex,_STR_LigIndex,&err);
	if ( index<0 || err )
return( true );
	if ( *_GGadgetGetTitle(g)=='\0' )
return( true );
	max = 0;
	AI_TestOrdering(ci,ap->me.x);
	for ( aps=ci->sc->anchor; aps!=NULL; aps=aps->next ) {
	    if ( aps->anchor==ap->anchor && aps!=ap ) {
		if ( aps->lig_index==index ) {
		    GWidgetErrorR(_STR_IndexInUse,_STR_LigIndexInUse);
return( true );
		} else if ( aps->lig_index>max )
		    max = aps->lig_index;
	    }
	}
	if ( index>max+10 ) {
	    char buf[20]; unichar_t ubuf[20];
	    GWidgetErrorR(_STR_TooBig,_STR_IndexTooBig);
	    sprintf(buf,"%d", max+1);
	    uc_strcpy(ubuf,buf);
	    GGadgetSetTitle(g,ubuf);
	    index = max+1;
	}
	ap->lig_index = index;
    }
return( true );
}

static int AI_ANameChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	AnchorPoint *ap;
	GTextInfo *ti = GGadgetGetListItemSelected(g);
	AnchorClass *an = ti->userdata;
	int bad=0, max=0;

	for ( ap=ci->sc->anchor; ap!=NULL; ap = ap->next ) {
	    if ( ap!=ci->ap && ap->anchor==an ) {
		if ( ap->type!=at_baselig || ci->ap->type!=at_baselig )
	break;
		if ( ap->lig_index==ci->ap->lig_index )
		    bad = true;
		else if ( ap->lig_index>max )
		    max = ap->lig_index;
	    }
	}
	if ( ap!=NULL ) {
	    AI_SelectList(ci,ap);
	    GWidgetErrorR(_STR_ClassUsed,_STR_AnchorClassUsed);
	} else {
	    ci->ap->anchor = an;
	    if ( ci->ap->type!=at_baselig )
		ci->ap->lig_index = 0;
	    else if ( bad )
		ci->ap->lig_index = max+1;
	    AI_DisplayClass(ci,ci->ap);
	    AI_TestOrdering(ci,ci->ap->me.x);
	}
    }
return( true );
}

static int AI_PosChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	real dx=0, dy=0;
	int err=false;
	AnchorPoint *ap = ci->ap;

	if ( GGadgetGetCid(g)==CID_X ) {
	    dx = GetCalmRealR(ci->gw,CID_X,_STR_X,&err)-ap->me.x;
	    AI_TestOrdering(ci,ap->me.x+dx);
	} else
	    dy = GetCalmRealR(ci->gw,CID_Y,_STR_Y,&err)-ap->me.y;
	if ( (dx==0 && dy==0) || err )
return( true );
	ap->me.x += dx;
	ap->me.y += dy;
	CVCharChangedUpdate(ci->cv);
    }
return( true );
}

static void AI_DoCancel(GIData *ci) {
    CharView *cv = ci->cv;
    ci->done = true;
    AnchorPointsFree(cv->sc->anchor);
    cv->sc->anchor = ci->oldaps;
    ci->oldaps = NULL;
    CVRemoveTopUndo(cv);
    SCUpdateAll(cv->sc);
}

static int ai_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	AI_DoCancel( GDrawGetUserData(gw));
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

static int AI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	AI_DoCancel( GDrawGetUserData(GGadgetGetWindow(g)));
    }
return( true );
}

static int AI_Ok(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GIData *ci = GDrawGetUserData(GGadgetGetWindow(g));
	ci->done = true;
	/* All the work has been done as we've gone along */
    }
return( true );
}

void ApGetInfo(CharView *cv, AnchorPoint *ap) {
    static GIData gi;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[20];
    GTextInfo label[20];
    int j;

    memset(&gi,0,sizeof(gi));
    gi.cv = cv;
    gi.sc = cv->sc;
    gi.oldaps = AnchorPointsCopy(cv->sc->anchor);
    CVPreserveState(cv);
    if ( ap==NULL ) {
	ap = AnchorPointNew(cv);
	if ( ap==NULL )
return;
    }
	
    gi.ap = ap;
    gi.changed = false;
    gi.done = false;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource(_STR_AnchorPointInfo,NULL);
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,AI_Width));
	pos.height = GDrawPointsToPixels(NULL,AI_Height);
	gi.gw = GDrawCreateTopWindow(NULL,&pos,ai_e_h,&gi,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));

	j=0;
	label[j].text = ap->anchor->name;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = 5; 
	gcd[j].gd.pos.width = AI_Width-10; 
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_NameList;
	gcd[j].gd.u.list = AnchorClassesList(cv->sc->parent);
	gcd[j].gd.handle_controlevent = AI_ANameChanged;
	gcd[j].creator = GListButtonCreate;
	++j;

	label[j].text = (unichar_t *) _STR_XC;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+34;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	++j;

	gcd[j].gd.pos.x = 23; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-4;
	gcd[j].gd.pos.width = 50;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_X;
	gcd[j].gd.handle_controlevent = AI_PosChanged;
	gcd[j].creator = GTextFieldCreate;
	++j;

	label[j].text = (unichar_t *) _STR_YC;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 85; gcd[j].gd.pos.y = gcd[j-2].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	++j;

	gcd[j].gd.pos.x = 103; gcd[j].gd.pos.y = gcd[j-2].gd.pos.y;
	gcd[j].gd.pos.width = 50;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Y;
	gcd[j].gd.handle_controlevent = AI_PosChanged;
	gcd[j].creator = GTextFieldCreate;
	++j;

	label[j].text = (unichar_t *) _STR_Mark;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+28;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Mark;
	gcd[j].gd.handle_controlevent = AI_TypeChanged;
	gcd[j].creator = GRadioCreate;
	++j;

	label[j].text = (unichar_t *) _STR_BaseChar;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 70; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_BaseChar;
	gcd[j].gd.handle_controlevent = AI_TypeChanged;
	gcd[j].creator = GRadioCreate;
	++j;

	label[j].text = (unichar_t *) _STR_BaseLig;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[j-2].gd.pos.x; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+14;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_BaseLig;
	gcd[j].gd.handle_controlevent = AI_TypeChanged;
	gcd[j].creator = GRadioCreate;
	++j;

	label[j].text = (unichar_t *) _STR_BaseMark;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = gcd[j-2].gd.pos.x; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_BaseMark;
	gcd[j].gd.handle_controlevent = AI_TypeChanged;
	gcd[j].creator = GRadioCreate;
	++j;

	label[j].text = (unichar_t *) _STR_LigIndex;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+26;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLabelCreate;
	++j;

	gcd[j].gd.pos.x = 65; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y-4;
	gcd[j].gd.pos.width = 50;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_LigIndex;
	gcd[j].gd.handle_controlevent = AI_LigIndexChanged;
	gcd[j].creator = GTextFieldCreate;
	++j;

	label[j].text = (unichar_t *) _STR_New;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+30;
	gcd[j].gd.pos.width = -1;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_New;
	gcd[j].gd.handle_controlevent = AI_New;
	gcd[j].creator = GButtonCreate;
	++j;

	label[j].text = (unichar_t *) _STR_Delete;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = -5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.pos.width = -1;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Delete;
	gcd[j].gd.handle_controlevent = AI_Delete;
	gcd[j].creator = GButtonCreate;
	++j;

	label[j].text = (unichar_t *) _STR_PrevArrow;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 15; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+28;
	gcd[j].gd.pos.width = -1;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Next;	/* Yes, I really do mean the name to be reversed */
	gcd[j].gd.handle_controlevent = AI_Next;
	gcd[j].creator = GButtonCreate;
	++j;

	label[j].text = (unichar_t *) _STR_NextArrow;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = -15; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y;
	gcd[j].gd.pos.width = -1;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.cid = CID_Prev;
	gcd[j].gd.handle_controlevent = AI_Prev;
	gcd[j].creator = GButtonCreate;
	++j;

	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+26;
	gcd[j].gd.pos.width = AI_Width-10;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].creator = GLineCreate;
	++j;

	label[j].text = (unichar_t *) _STR_OK;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+8;
	gcd[j].gd.pos.width = -1;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.handle_controlevent = AI_Ok;
	gcd[j].creator = GButtonCreate;
	++j;

	label[j].text = (unichar_t *) _STR_Cancel;
	label[j].text_in_resource = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = -5; gcd[j].gd.pos.y = gcd[j-1].gd.pos.y+3;
	gcd[j].gd.pos.width = -1;
	gcd[j].gd.flags = gg_enabled|gg_visible;
	gcd[j].gd.handle_controlevent = AI_Cancel;
	gcd[j].creator = GButtonCreate;
	++j;

	gcd[j].gd.pos.x = 2; gcd[j].gd.pos.y = 2;
	gcd[j].gd.pos.width = pos.width-4; gcd[j].gd.pos.height = pos.height-4;
	gcd[j].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
	gcd[j].creator = GLineCreate;
	++j;

	GGadgetsCreate(gi.gw,gcd);
	AI_Display(&gi,ap);
	GWidgetIndicateFocusGadget(GWidgetGetControl(gi.gw,CID_X));

    GWidgetHidePalettes();
    GDrawSetVisible(gi.gw,true);
    while ( !gi.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gi.gw);
    AnchorPointsFree(gi.oldaps);
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
    gi.sc = cv->sc;
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
    AnchorPoint *ap;

    if ( !CVOneThingSel(cv,&sp,&spl,&ref,&img,&ap)) {
#if 0
	if ( cv->fv->cidmaster==NULL )
	    SCGetInfo(cv->sc,false);
#endif
    } else if ( ref!=NULL )
	RefGetInfo(cv,ref);
    else if ( img!=NULL )
	ImgGetInfo(cv,img);
    else if ( ap!=NULL )
	ApGetInfo(cv,ap);
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
