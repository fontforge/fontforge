/* Copyright (C) 2001 by George Williams */
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
#include "ttfmodui.h"
#include <gkeysym.h>
#include <ustring.h>
#include <utype.h>
#include <locale.h>

extern struct lconv localeinfo;
extern int _GScrollBar_Width;

#define CID_Tabs		999

#define CID_Version		1000
#define CID_ItalicAngle		1001
#define CID_UnderlinePos	1002
#define CID_UnderlineHeight	1003
#define CID_IsFixed		1004
#define CID_minMemType42	1005
#define CID_maxMemType42	1006
#define CID_minMemType1		1007
#define CID_maxMemType1		1008

const char *ttfstandardnames[258] = {
".notdef",
".null",
"nonmarkingreturn",
"space",
"exclam",
"quotedbl",
"numbersign",
"dollar",
"percent",
"ampersand",
"quotesingle",
"parenleft",
"parenright",
"asterisk",
"plus",
"comma",
"hyphen",
"period",
"slash",
"zero",
"one",
"two",
"three",
"four",
"five",
"six",
"seven",
"eight",
"nine",
"colon",
"semicolon",
"less",
"equal",
"greater",
"question",
"at",
"A",
"B",
"C",
"D",
"E",
"F",
"G",
"H",
"I",
"J",
"K",
"L",
"M",
"N",
"O",
"P",
"Q",
"R",
"S",
"T",
"U",
"V",
"W",
"X",
"Y",
"Z",
"bracketleft",
"backslash",
"bracketright",
"asciicircum",
"underscore",
"grave",
"a",
"b",
"c",
"d",
"e",
"f",
"g",
"h",
"i",
"j",
"k",
"l",
"m",
"n",
"o",
"p",
"q",
"r",
"s",
"t",
"u",
"v",
"w",
"x",
"y",
"z",
"braceleft",
"bar",
"braceright",
"asciitilde",
"Adieresis",
"Aring",
"Ccedilla",
"Eacute",
"Ntilde",
"Odieresis",
"Udieresis",
"aacute",
"agrave",
"acircumflex",
"adieresis",
"atilde",
"aring",
"ccedilla",
"eacute",
"egrave",
"ecircumflex",
"edieresis",
"iacute",
"igrave",
"icircumflex",
"idieresis",
"ntilde",
"oacute",
"ograve",
"ocircumflex",
"odieresis",
"otilde",
"uacute",
"ugrave",
"ucircumflex",
"udieresis",
"dagger",
"degree",
"cent",
"sterling",
"section",
"bullet",
"paragraph",
"germandbls",
"registered",
"copyright",
"trademark",
"acute",
"dieresis",
"notequal",
"AE",
"Oslash",
"infinity",
"plusminus",
"lessequal",
"greaterequal",
"yen",
"mu",
"partialdiff",
"summation",
"product",
"pi",
"integral",
"ordfeminine",
"ordmasculine",
"Omega",
"ae",
"oslash",
"questiondown",
"exclamdown",
"logicalnot",
"radical",
"florin",
"approxequal",
"Delta",
"guillemotleft",
"guillemotright",
"ellipsis",
"nonbreakingspace",
"Agrave",
"Atilde",
"Otilde",
"OE",
"oe",
"endash",
"emdash",
"quotedblleft",
"quotedblright",
"quoteleft",
"quoteright",
"divide",
"lozenge",
"ydieresis",
"Ydieresis",
"fraction",
"currency",
"guilsinglleft",
"guilsinglright",
"fi",
"fl",
"daggerdbl",
"periodcentered",
"quotesinglbase",
"quotedblbase",
"perthousand",
"Acircumflex",
"Ecircumflex",
"Aacute",
"Edieresis",
"Egrave",
"Iacute",
"Icircumflex",
"Idieresis",
"Igrave",
"Oacute",
"Ocircumflex",
"apple",
"Ograve",
"Uacute",
"Ucircumflex",
"Ugrave",
"dotlessi",
"circumflex",
"tilde",
"macron",
"breve",
"dotaccent",
"ring",
"cedilla",
"hungarumlaut",
"ogonek",
"caron",
"Lslash",
"lslash",
"Scaron",
"scaron",
"Zcaron",
"zcaron",
"brokenbar",
"Eth",
"eth",
"Yacute",
"yacute",
"Thorn",
"thorn",
"minus",
"multiply",
"onesuperior",
"twosuperior",
"threesuperior",
"onehalf",
"onequarter",
"threequarters",
"franc",
"Gbreve",
"gbreve",
"Idotaccent",
"Scedilla",
"scedilla",
"Cacute",
"cacute",
"Ccaron",
"ccaron",
"dcroat"
};

static unichar_t one[] = { '1', '\0' };
static unichar_t two[] = { '2', '\0' };
static unichar_t two_five[] = { '2','.','5', '\0' };
static unichar_t three[] = { '3', '\0' };
static unichar_t four[] = { '4', '\0' };
static GTextInfo postversions[] = {
    { (unichar_t *) one, NULL, 0, 0, (void *) 10},
    { (unichar_t *) two, NULL, 0, 0, (void *) 20},
    { (unichar_t *) two_five, NULL, 0, 0, (void *) 25},
    { (unichar_t *) three, NULL, 0, 0, (void *) 30},
    { (unichar_t *) four, NULL, 0, 0, (void *) 40},
    { NULL }};


typedef struct postview /* : tableview */ {
    Table *table;
    GWindow gw, v;
    struct tableviewfuncs *virtuals;
    TtfFont *font;		/* for the encoding currently used */
    struct ttfview *owner;
    unsigned int destroyed: 1;		/* window has been destroyed */
/* post specials */
    int16 old_aspect;
    GWindow nv, ev;
    GGadget *nsb, *esb;
    GGadget *ntf, *etf;
    GGadget *tabs, *box;
    GGadget *ok, *cancel;
    GFont *gfont;
    int16 fh, as, sbw;
    int ntop, etop;
    int addrend;
    int16 vheight, vwidth;
    int16 nactive, eactive;
    double oldversion;
} postView;

struct postdata {
    int32 len;
    uint8 *data;
    int numnames;	/* could differ from numglyphs */
    char **names;	/* there are numnames entries */
    uint16 *enc;	/* there are numglyphs of these */
    char **editnames;
    uint16 *editenc;
};

#define EDGE_SPACER	2

static int efinishup(postView *postv) {
    const unichar_t *ret = _GGadgetGetTitle(postv->etf);
    unichar_t *end;
    struct postdata *pd = (postv->table->table_data);
    int val;

    if ( postv->eactive==-1 )
return( true );

    val = u_strtol(ret,&end,10);
    if ( *ret=='\0' || *end!='\0' ) {
	GWidgetErrorR(_STR_BadNumber,_STR_BadNumber);
return( false );
    } else if ( val<0 || val>postv->font->glyph_cnt ) {
	GWidgetErrorR(_STR_BadNumber,_STR_NumberOutOfRange,0,postv->font->glyph_cnt);
return( false );
    }
    if ( val != (pd->editenc==NULL?pd->enc[postv->eactive]:pd->editenc[postv->eactive]) ) {
	if ( pd->editenc==NULL ) {
	    pd->editenc = gcalloc(postv->font->glyph_cnt,sizeof(unichar_t));
	    memcpy(pd->editenc,pd->enc,postv->font->glyph_cnt*sizeof(unichar_t));
	}
	pd->editenc[postv->eactive] = val;
    }
    postv->eactive = -1;
    GGadgetMove(postv->etf,postv->addrend+EDGE_SPACER,-100);
return( true );
}

static int nfinishup(postView *postv) {
    const unichar_t *ret = _GGadgetGetTitle(postv->ntf), *pt;
    unichar_t *temp=NULL, *tpt, *end;
    struct postdata *pd = (postv->table->table_data);

    if ( postv->nactive==-1 )
return( true );

    if ( *ret=='\0' ) {
	GWidgetErrorR(_STR_BadPSName,_STR_BadPSNameEmpty);
	temp = uc_copy(".notdef");
    } else {
	for ( pt = ret; *pt; ++pt ) {
	    if ( *pt<' ' || *pt>=0x7f ||
		    *pt=='(' || *pt=='[' || *pt=='{' || *pt=='<' ||
		    *pt==')' || *pt==']' || *pt=='}' || *pt=='>' ||
		    *pt=='%' || *pt=='/' )
	break;
	}
	if ( *pt ) {
	    GWidgetErrorR(_STR_BadPSName,_STR_BadPSNameChar);
	    temp = u_copy(ret);
	    for ( pt=ret, tpt=temp; *pt ; ++pt ) {
		if ( *pt<' ' || *pt>=0x7f ||
			*pt=='(' || *pt=='[' || *pt=='{' || *pt=='<' ||
			*pt==')' || *pt==']' || *pt=='}' || *pt=='>' ||
			*pt=='%' || *pt=='/' )
		    /* Do Nothing */;
		else
		    *tpt++ = *pt;
	    }
	    *tpt='\0';
	    if ( *temp=='\0' ) {
		free( temp );
		temp = uc_copy(".notdef");
	    }
	} else {
	    u_strtod(ret,&end);
	    if ( *end=='\0' || (*end=='#' && *ret!='#')) {
		GWidgetErrorR(_STR_BadPSName,_STR_BadPSNameNotNumber);
		temp = uc_copy(".notdef");
	    }
	}
    }
    if ( temp!=NULL ) {
	GGadgetSetTitle(postv->ntf,temp);
	free(temp);
return( false );
    }
    if ( uc_strcmp(ret,pd->editnames==NULL || pd->editnames[postv->nactive]==NULL?
	    pd->names[postv->nactive]:pd->editnames[postv->nactive])!= 0 ) {
	if ( pd->editnames==NULL )
	    pd->editnames = gcalloc(pd->numnames,sizeof(char *));
	if ( pd->editnames[postv->nactive]!=NULL )
	    free( pd->editnames[postv->nactive] );
	pd->editnames[postv->nactive] = cu_copy(ret);
    }
    postv->nactive = -1;
    GGadgetMove(postv->ntf,postv->addrend+EDGE_SPACER,-100);
return( true );
}

static int post_processdata(TableView *tv) {
    postView *postv = (postView *) tv;
    struct postdata *pd = (postv->table->table_data);
    int err = false;
    real version, ia;
    int upos, usize, isfixed, min42, max42, min1, max1;
    int i;

    version = GetRealR(tv->gw,CID_Version,_STR_Version,&err);
    ia = GetRealR(tv->gw,CID_ItalicAngle,_STR_ItalicAngle,&err);
    upos = GetIntR(tv->gw,CID_UnderlinePos,_STR_UnderlinePos,&err);
    usize = GetIntR(tv->gw,CID_UnderlineHeight,_STR_UnderlineHeight,&err);
    isfixed = GetIntR(tv->gw,CID_IsFixed,_STR_IsFixed,&err);
    min42 = GetIntR(tv->gw,CID_minMemType42,_STR_MinMemType42,&err);
    max42 = GetIntR(tv->gw,CID_maxMemType42,_STR_MaxMemType42,&err);
    min1 = GetIntR(tv->gw,CID_minMemType1,_STR_MinMemType1,&err);
    max1 = GetIntR(tv->gw,CID_maxMemType1,_STR_MaxMemType1,&err);
    if ( err )
return( false );

    if ( !nfinishup(postv) || !efinishup(postv))
return( false );
    ptputvfixed(pd->data,version);
    ptputfixed(pd->data+4,ia);
    ptputushort(pd->data+8,upos);
    ptputushort(pd->data+10,usize);
    ptputlong(pd->data+12,isfixed);
    ptputlong(pd->data+16,min42);
    ptputlong(pd->data+20,max42);
    ptputlong(pd->data+24,min1);
    ptputlong(pd->data+28,max1);
    if ( pd->editnames!=NULL ) {
	if ( version>=3 ) {
	    for ( i=0; i<pd->numnames; ++i )
		free( pd->editnames[i]);
	    free(pd->editnames);
	} else if ( pd->names==NULL )
	    pd->names = pd->editnames;
	else {
	    for ( i=0; i<pd->numnames; ++i )
		if ( pd->editnames[i]!=NULL ) {
		    free(pd->names[i]);
		    pd->names[i] = pd->editnames[i];
		}
	    free(pd->editnames);
	}
	pd->editnames = NULL;
    }
    if ( pd->names!=NULL && version>=3 ) {
	for ( i=0; i<pd->numnames; ++i )
	    free( pd->names[i]);
	free(pd->names);
	pd->names = NULL;
	pd->numnames = 0;
    }
    if ( pd->editenc!=NULL && version==4 ) {
	free(pd->enc);
	pd->enc = pd->editenc;
    } else if ( version!=4 ) {
	free(pd->editenc);
	free(pd->enc);
	pd->enc = NULL;
    }
    pd->editenc = NULL;
    if ( !postv->table->changed ) {
	postv->table->changed = true;
	postv->table->td_changed = true;
	postv->table->container->changed = true;
	GDrawRequestExpose(tv->owner->v,NULL,false);
    }
return( true );
}

static int post_close(TableView *tv) {
    if ( post_processdata(tv)) {
	tv->destroyed = true;
	GDrawDestroyWindow(tv->gw);
return( true );
    }
return( false );
}

static struct tableviewfuncs postfuncs = { post_close, post_processdata };

static int JustMacNames(struct postdata *pd, int exact) {
    int i,j;
    char *name;

    if ( exact ) {
	for ( i=0; i<258 && i<pd->numnames; ++i ) {
	    name = pd->editnames!=NULL && pd->editnames[i]!=NULL ?
		    pd->editnames[i] : pd->names[i];
	    if ( strcmp(name,ttfstandardnames[i])!=0 )
return( false );
	}
	/* I'm going to allow extra glyphs which can't be named */
return( true );
    } else {
	for ( i=0; i<pd->numnames; ++i ) {
	    name = pd->editnames!=NULL && pd->editnames[i]!=NULL ?
		    pd->editnames[i] : pd->names[i];
	    for ( j=0; j<258; ++j )
		if ( strcmp(name,ttfstandardnames[j])==0 )
	    break;
	    if ( j==258 )
return( false );
	}
return( true );
    }
}

static int post_VersionChange(GGadget *g, GEvent *e) {
    GWindow gw = GDrawGetParentWindow(GGadgetGetWindow(g));
    const unichar_t *ret = _GGadgetGetTitle(g);
    double val = u_strtod(ret,NULL);
    postView *postv = GDrawGetUserData(gw);
    struct postdata *pd = (postv->table->table_data);
    int revert = false;
    struct enctab *enc;
    static int buts[] = { _STR_Yes, _STR_Cancel, 0 };
    char buf[100]; unichar_t ubuf[10];
    int i;

    if ( e->u.control.subtype!=et_textchanged )
return( true );
    if ( val==4.0 && pd->enc==NULL ) {
	if ( GWidgetAskR( _STR_PostNamesLost,buts,0,1,_STR_PostVersion4)==1 ) {
	    revert = 1;
	} else {
	    pd->editenc = gcalloc(postv->font->glyph_cnt,sizeof(uint16));
	    if ( (enc = postv->font->enc)!=NULL )
		for ( i=0; i<postv->font->glyph_cnt && i<enc->cnt; ++i )
		    pd->editenc[i] = enc->uenc[i]==0xffff?0:enc->uenc[i];
	    GScrollBarSetBounds(postv->esb,0,postv->font->glyph_cnt,postv->vheight/postv->fh);
	}
    } else if ( val!=4.0 && pd->names==NULL && pd->enc!=NULL ) {
	if ( GWidgetAskR( _STR_PostEncLost,buts,0,1,_STR_PostNotVersion4)==1 )
	    revert = 1;
	else if ( val==1.0 ) {
	    if ( GWidgetAskR( _STR_PostVersionFailure,buts,0,1,_STR_PostVersion1Len)==1 )
	    pd->numnames = postv->font->glyph_cnt<258 ? postv->font->glyph_cnt : 258;
	    pd->editnames = gcalloc(pd->numnames,sizeof(char *));
	    for ( i=0; i<pd->numnames; ++i )
		pd->editnames[i] = copy(ttfstandardnames[i]);
	} else if ( val==2.5 ) {
	    /* !!!! */
	} else if ( val==2.0 ) {
	    pd->numnames = postv->font->glyph_cnt;
	    pd->editnames = gcalloc(postv->font->glyph_cnt,sizeof(char *));
	    if ( (enc = postv->font->enc)!=NULL ) {
		for ( i=0; i<pd->numnames; ++i ) {
		    if ( enc->uenc[i]==0xffff )
			pd->editnames[i] = copy(".notdef");
		    else if ( psunicodenames[enc->uenc[i]]!=NULL )
			pd->editnames[i] = copy(psunicodenames[enc->uenc[i]]);
		    else {
			sprintf(buf,"uni%04X", enc->uenc[i]);
			pd->editnames[i] = copy(buf);
		    }
		}
	    } else {
		for ( i=0; i<pd->numnames; ++i )
		    pd->editnames[i] = copy(".notdef");
	    }
	}
	GScrollBarSetBounds(postv->nsb,0,pd->numnames,postv->vheight/postv->fh);
    } else if ( val==3.0 && pd->names!=NULL ) {
	if ( GWidgetAskR( _STR_PostNamesLost,buts,0,1,_STR_PostVersion3)==1 )
	    revert = 1;
    } else if ( val==1.0 && !JustMacNames(pd,true) ) {
	GWidgetErrorR( _STR_PostNamesLost,_STR_PostVersion1);
	revert = 1;
    } else if ( val==2.5 && !JustMacNames(pd,false) ) {
	GWidgetErrorR( _STR_PostNamesLost,_STR_PostVersion2_5);
	revert = 1;
    }
    if ( revert ) {
	sprintf( buf, "%g", postv->oldversion );
	uc_strcpy(ubuf,buf);
	GGadgetSetTitle(g,ubuf);
return( true );
    }
    if ( val!=0 )
	postv->oldversion = val;
    GTabSetSetEnabled(GWidgetGetControl(gw,CID_Tabs),2,val==4.0);
    GTabSetSetEnabled(GWidgetGetControl(gw,CID_Tabs),1,val<3.0);
return( true );
}

static int post_AspectChange(GGadget *g, GEvent *e) {
return( true );
}

static int post_Cancel(GGadget *g, GEvent *e) {
    GWindow gw;
    postView *postv;
    struct postdata *pd;
    int i;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	postv = GDrawGetUserData(gw);
	pd = (postv->table->table_data);
	if ( pd->editnames!=NULL ) {
	    for ( i=0;i<pd->numnames; ++i )
		free(pd->editnames[i]);
	    free(pd->editnames);
	    pd->editnames = NULL;
	}
	free(pd->editenc);
	pd->editenc = NULL;
	GDrawDestroyWindow(gw);
    }
return( true );
}

static int post_OK(GGadget *g, GEvent *e) {
    GWindow gw;
    postView *postv;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	postv = GDrawGetUserData(gw);
	post_close((TableView *) postv);
    }
return( true );
}

static int post_e_h(GWindow gw, GEvent *event) {
    postView *postv = GDrawGetUserData(gw);
    if ( event->type==et_close ) {
	GDrawDestroyWindow(postv->gw);
    } else if ( event->type == et_destroy ) {
	postv->table->tv = NULL;
	free(postv);
    } else if ( event->type == et_resize ) {
	GRect pos;
	GGadgetResize(postv->tabs,
		event->u.resize.size.width-GDrawPointsToPixels(NULL,352-343),
		event->u.resize.size.height-GDrawPointsToPixels(NULL,340-292));
	postv->vheight += event->u.resize.dheight;
	postv->vwidth += event->u.resize.dwidth;
	GGadgetGetSize(postv->ok,&pos);
	GGadgetMove(postv->ok,pos.x,pos.y+event->u.resize.dheight);
	GGadgetGetSize(postv->cancel,&pos);
	GGadgetMove(postv->cancel,pos.x+event->u.resize.dwidth,
		pos.y+event->u.resize.dheight);
	GGadgetGetSize(postv->box,&pos);
	GGadgetResize(postv->box,pos.width+event->u.resize.dwidth,
		pos.height+event->u.resize.dheight);
	GDrawRequestExpose(gw,NULL,false);
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 ) {
	    TableHelp(postv->table->name);
return( true );
	} else if (( event->u.chr.state&ksm_control ) &&
		(event->u.chr.keysym=='q' || event->u.chr.keysym=='Q')) {
	    MenuExit(NULL,NULL,NULL);
	}
return( false );
    }
return( true );
}

static void postv_expose(postView *postv,GWindow pixmap,GRect *rect, int is_n) {
    int low, high;
    int x,y;
    struct postdata *pd = (postv->table->table_data);
    char caddr[8], cval[20]; unichar_t uval[129], uaddr[8];
    int index, max = is_n? pd->numnames :
	    pd->enc==NULL && pd->editenc ==NULL ? 0 : postv->font->glyph_cnt;
    Color col;
    GRect old, size;

    if ( rect->y<0 ) {
	if ( rect->y+rect->height<0 )
return;
	rect->height += rect->y;
	rect->y = 0;
    }
    size = *rect;
    if ( rect->x+rect->width>=postv->vwidth-postv->sbw )
	size.width = postv->vwidth-postv->sbw-rect->x;
    GDrawPushClip(pixmap,&size,&old);
    GDrawSetFont(pixmap,postv->gfont);

    low = ( (rect->y-EDGE_SPACER)/postv->fh ) * postv->fh +2;
    high = ( (rect->y+rect->height+postv->fh-1-EDGE_SPACER)/postv->fh ) * postv->fh +EDGE_SPACER;
    if ( high>postv->vheight-EDGE_SPACER ) high = postv->vheight-EDGE_SPACER;

    GDrawDrawLine( pixmap, postv->addrend,0,postv->addrend,0x7fff,0x000000);

    index = ((is_n?postv->ntop:postv->etop)+(low-EDGE_SPACER)/postv->fh);
    y = low;
    for ( ; y<=high && index<max; ++index ) {
	sprintf( caddr, "%d", index );
	uc_strcpy(uaddr,caddr);
	col = 0x000000;
	if ( postv->font->enc!=NULL ) {
	    int p = strlen(caddr);
	    uaddr[p] = ' ';
	    if ( postv->font->enc->uenc[index]!=0xffff )
		uaddr[p+1] = postv->font->enc->uenc[index];
	    else {
		uaddr[p+1] = '?';
		col = 0xff0000;
	    }
	    uaddr[p+2] = '\0';
	}
	x = postv->addrend - EDGE_SPACER - GDrawGetTextWidth(pixmap,uaddr,-1,NULL);
	GDrawDrawText(pixmap,x,y+postv->as,uaddr,-1,NULL,col);

	if ( is_n ) {
	    if ( pd->names!=NULL ) {
		uc_strcpy(uval, pd->editnames!=NULL && pd->editnames[index]!=NULL?
			pd->editnames[index]: pd->names[index] );
		GDrawDrawText(pixmap,postv->addrend+EDGE_SPACER,y+postv->as,uval,-1,NULL,0x000000);
	    }
	} else {
	    if ( pd->enc!=NULL || pd->editenc ) {
		sprintf(cval, "%d", pd->editenc!=NULL ? pd->editenc[index]: pd->enc[index] );
		uc_strcpy(uval,cval);
	    }
	}
	GDrawDrawText(pixmap,postv->addrend+EDGE_SPACER,y+postv->as,uval,-1,NULL,0x000000);
	y += postv->fh;
    }
    GDrawPopClip(pixmap,&old);
}

static void post_scroll(postView *postv,struct sbevent *sb,int is_n) {
    int newpos = is_n?postv->ntop:postv->etop, oldpos = newpos;
    struct postdata *pd = postv->table->table_data;
    int lheight = is_n ? pd->numnames : postv->font->glyph_cnt;
    GRect size;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= postv->vheight/postv->fh;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += postv->vheight/postv->fh;
      break;
      case et_sb_bottom:
        newpos = lheight-postv->vheight/postv->fh;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>lheight-postv->vheight/postv->fh )
        newpos = lheight-postv->vheight/postv->fh;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=oldpos ) {
	int diff = newpos-oldpos;
	if ( is_n ) postv->ntop = newpos;
	else	    postv->etop = newpos;
	size.x = size.y = 0;
	size.width = postv->vwidth-postv->sbw; size.height = postv->vheight;
	GScrollBarSetPos(is_n ? postv->nsb : postv->esb,newpos);
	if ( (is_n && postv->nactive!=-1) || (!is_n && postv->eactive!=-1) ) {
	    GGadget *tf = is_n ? postv->ntf : postv->etf;
	    GRect pos;
	    GGadgetGetSize(tf,&pos);
	    GGadgetMove(tf,postv->addrend+EDGE_SPACER,pos.y+diff*postv->fh);
	}
	GDrawScroll(is_n ? postv->nv : postv->ev,&size,0,diff*postv->fh);
    }
}

static int ev_e_h(GWindow gw, GEvent *event) {
    postView *postv = GDrawGetUserData(gw);
    struct postdata *pd = postv->table->table_data;
    int width;

    if ( event->type == et_resize ) {
	struct postdata *pd = postv->table->table_data;
	int lheight = pd->enc==NULL && pd->editenc==NULL ? 0 : postv->font->glyph_cnt;

	GGadgetMove(postv->esb,event->u.resize.size.width-postv->sbw,0);
	GGadgetResize(postv->esb,postv->sbw,event->u.resize.size.height);
	GScrollBarSetBounds(postv->esb,0,lheight,postv->vheight/postv->fh);
	if ( postv->etop + postv->vheight/postv->fh > lheight )
	    postv->etop = lheight-postv->vheight/postv->fh;
	if ( postv->etop<0 ) postv->etop = 0;
	GScrollBarSetPos(postv->esb,postv->etop);
	width = event->u.resize.size.width-postv->addrend-EDGE_SPACER - postv->sbw;
	if ( width < 5 ) width = 5;
	GGadgetResize(postv->etf,width,postv->fh);
	GDrawRequestExpose(gw,NULL,false);
    } else if ( event->type == et_expose ) {
	postv_expose(postv,gw,&event->u.expose.rect,false);
    } else if ( event->type == et_char ) {
	post_e_h(postv->gw,event);
    } else if ( event->type == et_controlevent ) {
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    post_scroll(postv,&event->u.control.u.sb,false);
	  break;
	}
    } else if ( event->type == et_mousedown ) {
	int l = (event->u.mouse.y-EDGE_SPACER)/postv->fh + postv->etop;
	unichar_t ubuf[20]; char buf[20];
	if ( efinishup(postv) && event->u.mouse.x>postv->addrend+EDGE_SPACER &&
		l<postv->font->glyph_cnt && l!=postv->eactive ) {
	    postv->eactive = l;
	    GGadgetMove(postv->etf, postv->addrend+EDGE_SPACER,
				    (l-postv->etop)*postv->fh+EDGE_SPACER+1);
	    sprintf( buf, "%d", pd->editenc==NULL ? pd->enc[l] : pd->editenc[l]);
	    uc_strcpy(ubuf,buf);
	    GGadgetSetTitle(postv->etf,ubuf);
	    GDrawPostEvent(event);	/* And we hope the tf catches it this time */
	}
    }
return( true );
}

static int nv_e_h(GWindow gw, GEvent *event) {
    postView *postv = GDrawGetUserData(gw);
    struct postdata *pd = postv->table->table_data;
    int width;

    if ( event->type == et_resize ) {
	GGadgetMove(postv->nsb,event->u.resize.size.width-postv->sbw,0);
	GGadgetResize(postv->nsb,postv->sbw,event->u.resize.size.height);
	GScrollBarSetBounds(postv->nsb,0,pd->numnames,postv->vheight/postv->fh);
	if ( postv->ntop + postv->vheight/postv->fh > pd->numnames )
	    postv->ntop = pd->numnames-postv->vheight/postv->fh;
	if ( postv->ntop<0 ) postv->etop = 0;
	GScrollBarSetPos(postv->nsb,postv->ntop);
	width = event->u.resize.size.width-postv->addrend-EDGE_SPACER - postv->sbw;
	if ( width < 5 ) width = 5;
	GGadgetResize(postv->ntf,width,postv->fh);
	GDrawRequestExpose(gw,NULL,false);
    } else if ( event->type == et_expose ) {
	postv_expose(postv,gw,&event->u.expose.rect,true);
return( false );
    } else if ( event->type == et_char ) {
	post_e_h(postv->gw,event);
    } else if ( event->type == et_controlevent ) {
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    post_scroll(postv,&event->u.control.u.sb,true);
	  break;
	}
    } else if ( event->type == et_mousedown ) {
	int l = (event->u.mouse.y-EDGE_SPACER)/postv->fh + postv->ntop;
	unichar_t *temp;
	if ( nfinishup(postv) && event->u.mouse.x>postv->addrend+EDGE_SPACER &&
		l<pd->numnames && l!=postv->nactive ) {
	    postv->nactive = l;
	    GGadgetMove(postv->ntf, postv->addrend+EDGE_SPACER,
				    (l-postv->ntop)*postv->fh+EDGE_SPACER+1);
	    temp = uc_copy(pd->editnames!=NULL && pd->editnames[l]!=NULL ?
		    pd->editnames[l]:pd->names[l]);
	    GGadgetSetTitle(postv->ntf,temp);
	    free(temp);
	    GDrawPostEvent(event);	/* And we hope the tf catches it this time */
	}
    }
return( true );
}

static void free_posttabledata(void *_data) {
    struct postdata *pd = _data;
    int i;

    if ( pd==NULL )
return;

    free(pd->data);
    free(pd->enc);
    if ( pd->names!=NULL )
	for ( i=0; i<pd->numnames; ++i )
	    free( pd->names[i] );
    free( pd->names );
    free( pd );
}

static void write_posttabledata(FILE *tottf,Table *post) {
    /* !!!! */
}

static void postTableFillup(Table *tab,TtfFont *font) {
    int i, gcbig, len, j;
    struct postdata *pd;
    FILE *ttf = tab->container->file;
    uint16 *sids;
    char **names;

    if ( tab->table_data!=NULL )
return;		/* Already done */
    if ( tab->data!=NULL && tab->changed ) {
	GWidgetErrorR(_STR_BinaryEdit,_STR_BinaryEditSave);
return;
    }
    if ( tab->data ) {
	free(tab->data);
	tab->data = NULL;
    }

    fseek(ttf,tab->start,SEEK_SET);

    pd = gcalloc(1,sizeof(struct postdata));
    pd->len = 32;
    pd->data = galloc(32);
    fread(pd->data,1,32,ttf);

    if ( ptgetlong(pd->data)== 0x10000 ) {
	/* No name data. It defaults to standard ttf names */
	pd->numnames = font->glyph_cnt>258?258:font->glyph_cnt;
	pd->names = gcalloc(pd->numnames,sizeof(char *));
	for ( i=0; i<pd->numnames; ++i )
	    pd->names[i] = copy(ttfstandardnames[i]);
    } else if ( ptgetlong(pd->data)==0x20000 ) {
	/* Normal name tables with an array of pointers to strings */
	pd->numnames = getushort(ttf);
	pd->names = gcalloc(pd->numnames,sizeof(char *));
	sids = gcalloc(pd->numnames,sizeof(uint16));
	/* the index table is backwards from the way I want to use it */
	gcbig = 257;
	for ( i=0; i<pd->numnames; ++i ) {
	    sids[i] = getushort(ttf);
	    if ( sids[i]>gcbig ) gcbig = sids[i];
	}
	if ( gcbig>257 ) {
	    names = gcalloc(gcbig+1,sizeof(char *));
	    for ( i=258; i<=gcbig; ++i ) {
		char *nm;
		len = getc(ttf);
		nm = galloc(len+1);
		for ( j=0; j<len; ++j )
		    nm[j] = getc(ttf);
		nm[j] = '\0';
		names[i] = nm;
	    }
	}
	for ( i=0; i<pd->numnames; ++i ) {
	    if ( sids[i]<258 )
		pd->names[i] = copy(ttfstandardnames[sids[i]]);
	    else
		pd->names[i] = copy(names[sids[i]]);
	}
	free(sids);
	for ( i=258; i<=gcbig; ++i )
	    free(names[i]);
	free(names);
    } else if ( ptgetlong(pd->data)==0x25000 ) {
	/* Depreciated. Just a reordering of the standard names */
	pd->numnames = font->glyph_cnt;
	pd->names = gcalloc(pd->numnames,sizeof(char *));
	for ( i=0; i<pd->numnames; ++i )
	    pd->names[i] = copy(ttfstandardnames[getc(ttf)]);
    } else if ( ptgetlong(pd->data)==0x30000 ) {
	/* No data, no tables */
    } else if ( ptgetlong(pd->data)==0x40000 ) {
	/* Some sort of encoding table */
	pd->enc = galloc(font->glyph_cnt*sizeof(unichar_t));
	for ( i=0; i<font->glyph_cnt; ++i )
	    pd->enc[i] = getushort(ttf);
    }
    tab->table_data = pd;
    tab->free_tabledata = free_posttabledata;
    tab->write_tabledata = write_posttabledata;
}
    
void postCreateEditor(Table *tab,TtfView *tfv) {
    postView *postv = gcalloc(1,sizeof(postView));
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GTabInfo aspects[8];
    GGadgetCreateData mgcd[6], ggcd[20], ngcd[3], egcd[3];
    GTextInfo mlabel[6], glabel[20], tf;
    static unichar_t title[60];
    char version[12], ital[12], upos[12], usize[12], fixed[12], min42[12],
	max42[12], min1[12], max1[12];
    int i; real vnum;
    struct postdata *pd;
    FontRequest rq;
    static unichar_t monospace[] = { 'c','o','u','r','i','e','r',',','m', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',',','u','n','i','f','o','n','t', '\0' };
    static unichar_t num[] = { '0',  '\0' };
    int numlen;
    int as, ds, ld;
    static GBox tfbox;

    postv->table = tab;
    postv->virtuals = &postfuncs;
    postv->owner = tfv;
    postv->font = tfv->ttf->fonts[tfv->selectedfont];
    tab->tv = (TableView *) postv;

    postTableFillup(tab,postv->font);
    pd = tab->table_data;

    title[0] = (tab->name>>24)&0xff;
    title[1] = (tab->name>>16)&0xff;
    title[2] = (tab->name>>8 )&0xff;
    title[3] = (tab->name    )&0xff;
    title[4] = ' ';
    u_strncpy(title+5, postv->font->fontname, sizeof(title)/sizeof(title[0])-6);
    title[sizeof(title)/sizeof(title[0])-1] = '\0';

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = title;
    wattrs.icon = ttf_icon;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,252);
    pos.height = GDrawPointsToPixels(NULL,340);
    postv->gw = gw = GDrawCreateTopWindow(NULL,&pos,post_e_h,postv,&wattrs);

    memset(&rq,0,sizeof(rq));
    rq.family_name = monospace;
    rq.point_size = -12;
    rq.weight = 400;
    postv->gfont = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawSetFont(postv->gw,postv->gfont);
    GDrawFontMetrics(postv->gfont,&as,&ds,&ld);
    postv->as = as+1;
    postv->fh = postv->as+ds;

    numlen = GDrawGetTextWidth(postv->gw,num,1,NULL);
    postv->addrend = 6*numlen + 2*EDGE_SPACER;

/******************************************************************************/
    postv->oldversion = vnum = ptgetvfixed(pd->data);

    memset(&mlabel,0,sizeof(mlabel));
    memset(&mgcd,0,sizeof(mgcd));
    memset(&aspects,'\0',sizeof(aspects));

    i = 0;

    aspects[i].text = (unichar_t *) _STR_General;
    aspects[i].selected = true;
    postv->old_aspect = 0;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = ggcd;

    aspects[i].text = (unichar_t *) _STR_Names;
    aspects[i].text_in_resource = true;
    aspects[i].disabled = vnum>=3.0;
    aspects[i++].gcd = ngcd;

    aspects[i].text = (unichar_t *) _STR_Encoding;
    aspects[i].text_in_resource = true;
    aspects[i].disabled = vnum!=4.0;
    aspects[i++].gcd = egcd;

    mgcd[0].gd.pos.x = 5; mgcd[0].gd.pos.y = 5;
    mgcd[0].gd.pos.width = 242;
    mgcd[0].gd.pos.height = 292;
    mgcd[0].gd.u.tabs = aspects;
    mgcd[0].gd.flags = gg_visible | gg_enabled;
    mgcd[0].gd.cid = CID_Tabs;
    mgcd[0].gd.handle_controlevent = post_AspectChange;
    mgcd[0].creator = GTabSetCreate;

    mgcd[1].gd.pos.x = 30-3; mgcd[1].gd.pos.y = mgcd[0].gd.pos.y+mgcd[0].gd.pos.height+8-3;
    mgcd[1].gd.pos.width = -1; mgcd[1].gd.pos.height = 0;
    mgcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    mlabel[1].text = (unichar_t *) _STR_OK;
    mlabel[1].text_in_resource = true;
    mgcd[1].gd.label = &mlabel[1];
    mgcd[1].gd.handle_controlevent = post_OK;
    mgcd[1].creator = GButtonCreate;

    mgcd[2].gd.pos.x = 252-GIntGetResource(_NUM_Buttonsize)-30; mgcd[2].gd.pos.y = mgcd[1].gd.pos.y+3;
    mgcd[2].gd.pos.width = -1; mgcd[2].gd.pos.height = 0;
    mgcd[2].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    mlabel[2].text = (unichar_t *) _STR_Cancel;
    mlabel[2].text_in_resource = true;
    mgcd[2].gd.label = &mlabel[2];
    mgcd[2].gd.handle_controlevent = post_Cancel;
    mgcd[2].creator = GButtonCreate;

    mgcd[3].gd.pos.x = 2; mgcd[3].gd.pos.y = 2;
    mgcd[3].gd.pos.width = pos.width-4; mgcd[3].gd.pos.height = pos.height-4;
    mgcd[3].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    mgcd[3].creator = GGroupCreate;

/******************************************************************************/
    memset(&glabel,0,sizeof(glabel));
    memset(&ggcd,0,sizeof(ggcd));

    ggcd[0].gd.pos.x = 10; ggcd[0].gd.pos.y = 12;
    glabel[0].text = (unichar_t *) _STR_Version;
    glabel[0].text_in_resource = true;
    ggcd[0].gd.label = &glabel[0];
    ggcd[0].gd.flags = gg_visible | gg_enabled;
    ggcd[0].creator = GLabelCreate;

    sprintf( version, "%g", vnum );
    two_five[1] = localeinfo.decimal_point[0];
    glabel[1].text = (unichar_t *) version;
    glabel[1].text_is_1byte = true;
    ggcd[1].gd.label = &glabel[1];
    ggcd[1].gd.pos.x = 80; ggcd[1].gd.pos.y = ggcd[0].gd.pos.y-6;
    ggcd[1].gd.flags = gg_enabled|gg_visible;
    ggcd[1].gd.cid = CID_Version;
    ggcd[1].gd.u.list = postversions;
    ggcd[1].gd.handle_controlevent = post_VersionChange;
    ggcd[1].creator = GListFieldCreate;

    ggcd[2].gd.pos.x = 10; ggcd[2].gd.pos.y = ggcd[1].gd.pos.y+24+6;
    glabel[2].text = (unichar_t *) _STR_ItalicAngle;
    glabel[2].text_in_resource = true;
    ggcd[2].gd.label = &glabel[2];
    ggcd[2].gd.flags = gg_visible | gg_enabled;
    ggcd[2].creator = GLabelCreate;

    sprintf( ital, "%g", ptgetfixed(pd->data+4) );
    glabel[3].text = (unichar_t *) ital;
    glabel[3].text_is_1byte = true;
    ggcd[3].gd.label = &glabel[3];
    ggcd[3].gd.pos.x = 80; ggcd[3].gd.pos.y = ggcd[2].gd.pos.y-6; ggcd[3].gd.pos.width = 60;
    ggcd[3].gd.flags = gg_enabled|gg_visible;
    ggcd[3].gd.cid = CID_ItalicAngle;
    ggcd[3].creator = GTextFieldCreate;

    ggcd[4].gd.pos.x = 10; ggcd[4].gd.pos.y = ggcd[3].gd.pos.y+24+6;
    glabel[4].text = (unichar_t *) _STR_UnderlinePos;
    glabel[4].text_in_resource = true;
    ggcd[4].gd.label = &glabel[4];
    ggcd[4].gd.flags = gg_visible | gg_enabled;
    ggcd[4].creator = GLabelCreate;

    sprintf( upos, "%d", (short) ptgetushort(pd->data+8) );
    glabel[5].text = (unichar_t *) upos;
    glabel[5].text_is_1byte = true;
    ggcd[5].gd.label = &glabel[5];
    ggcd[5].gd.pos.x = 80; ggcd[5].gd.pos.y = ggcd[4].gd.pos.y-6; ggcd[5].gd.pos.width = 60;
    ggcd[5].gd.flags = gg_enabled|gg_visible;
    ggcd[5].gd.cid = CID_UnderlinePos;
    ggcd[5].creator = GTextFieldCreate;

    ggcd[6].gd.pos.x = 150; ggcd[6].gd.pos.y = ggcd[3].gd.pos.y+24+6;
    glabel[6].text = (unichar_t *) _STR_UnderlineHeight;
    glabel[6].text_in_resource = true;
    ggcd[6].gd.label = &glabel[6];
    ggcd[6].gd.flags = gg_visible | gg_enabled;
    ggcd[6].creator = GLabelCreate;

    sprintf( usize, "%d", ptgetushort(pd->data+10) );
    glabel[7].text = (unichar_t *) usize;
    glabel[7].text_is_1byte = true;
    ggcd[7].gd.label = &glabel[7];
    ggcd[7].gd.pos.x = 175; ggcd[7].gd.pos.y = ggcd[6].gd.pos.y-6; ggcd[7].gd.pos.width = 60;
    ggcd[7].gd.flags = gg_enabled|gg_visible;
    ggcd[7].gd.cid = CID_UnderlineHeight;
    ggcd[7].creator = GTextFieldCreate;

    ggcd[8].gd.pos.x = 10; ggcd[8].gd.pos.y = ggcd[6].gd.pos.y+24+6;
    glabel[8].text = (unichar_t *) _STR_IsFixed;
    glabel[8].text_in_resource = true;
    ggcd[8].gd.label = &glabel[8];
    ggcd[8].gd.flags = gg_visible | gg_enabled;
    ggcd[8].creator = GLabelCreate;

    sprintf( fixed, "%d", ptgetlong(pd->data+12) );
    glabel[9].text = (unichar_t *) fixed;
    glabel[9].text_is_1byte = true;
    ggcd[9].gd.label = &glabel[9];
    ggcd[9].gd.pos.x = 80; ggcd[9].gd.pos.y = ggcd[8].gd.pos.y-6; ggcd[9].gd.pos.width = 60;
    ggcd[9].gd.flags = gg_enabled|gg_visible;
    ggcd[9].gd.cid = CID_IsFixed;
    ggcd[9].creator = GTextFieldCreate;

    ggcd[10].gd.pos.x = 10; ggcd[10].gd.pos.y = ggcd[8].gd.pos.y+24+6;
    glabel[10].text = (unichar_t *) _STR_MinMemType42;
    glabel[10].text_in_resource = true;
    ggcd[10].gd.label = &glabel[10];
    ggcd[10].gd.flags = gg_visible | gg_enabled;
    ggcd[10].creator = GLabelCreate;

    sprintf( min42, "%d", ptgetlong(pd->data+16) );
    glabel[11].text = (unichar_t *) min42;
    glabel[11].text_is_1byte = true;
    ggcd[11].gd.label = &glabel[11];
    ggcd[11].gd.pos.x = 120; ggcd[11].gd.pos.y = ggcd[10].gd.pos.y-6; ggcd[11].gd.pos.width = 60;
    ggcd[11].gd.flags = gg_enabled|gg_visible;
    ggcd[11].gd.cid = CID_minMemType42;
    ggcd[11].creator = GTextFieldCreate;

    ggcd[12].gd.pos.x = 70; ggcd[12].gd.pos.y = ggcd[10].gd.pos.y+24;
    glabel[12].text = (unichar_t *) _STR_MaxMemType42;
    glabel[12].text_in_resource = true;
    ggcd[12].gd.label = &glabel[12];
    ggcd[12].gd.flags = gg_visible | gg_enabled;
    ggcd[12].creator = GLabelCreate;

    sprintf( max42, "%d", ptgetlong(pd->data+20) );
    glabel[13].text = (unichar_t *) max42;
    glabel[13].text_is_1byte = true;
    ggcd[13].gd.label = &glabel[13];
    ggcd[13].gd.pos.x = 100; ggcd[13].gd.pos.y = ggcd[12].gd.pos.y-6; ggcd[13].gd.pos.width = 60;
    ggcd[13].gd.flags = gg_enabled|gg_visible;
    ggcd[13].gd.cid = CID_maxMemType42;
    ggcd[13].creator = GTextFieldCreate;

    ggcd[14].gd.pos.x = 10; ggcd[14].gd.pos.y = ggcd[12].gd.pos.y+24;
    glabel[14].text = (unichar_t *) _STR_MinMemType1;
    glabel[14].text_in_resource = true;
    ggcd[14].gd.label = &glabel[14];
    ggcd[14].gd.flags = gg_visible | gg_enabled;
    ggcd[14].creator = GLabelCreate;

    sprintf( min1, "%d", ptgetlong(pd->data+24) );
    glabel[15].text = (unichar_t *) min1;
    glabel[15].text_is_1byte = true;
    ggcd[15].gd.label = &glabel[15];
    ggcd[15].gd.pos.x = ggcd[11].gd.pos.x; ggcd[15].gd.pos.y = ggcd[14].gd.pos.y-6; ggcd[15].gd.pos.width = 60;
    ggcd[15].gd.flags = gg_enabled|gg_visible;
    ggcd[15].gd.cid = CID_minMemType1;
    ggcd[15].creator = GTextFieldCreate;

    ggcd[16].gd.pos.x = ggcd[12].gd.pos.x; ggcd[16].gd.pos.y = ggcd[14].gd.pos.y+24;
    glabel[16].text = (unichar_t *) _STR_MaxMemType1;
    glabel[16].text_in_resource = true;
    ggcd[16].gd.label = &glabel[16];
    ggcd[16].gd.flags = gg_visible | gg_enabled;
    ggcd[16].creator = GLabelCreate;

    sprintf( max1, "%d", ptgetlong(pd->data+28) );
    glabel[17].text = (unichar_t *) max1;
    glabel[17].text_is_1byte = true;
    ggcd[17].gd.label = &glabel[17];
    ggcd[17].gd.pos.x = ggcd[13].gd.pos.x; ggcd[17].gd.pos.y = ggcd[16].gd.pos.y-6; ggcd[17].gd.pos.width = 60;
    ggcd[17].gd.flags = gg_enabled|gg_visible;
    ggcd[17].gd.cid = CID_maxMemType1;
    ggcd[17].creator = GTextFieldCreate;

/******************************************************************************/
    memset(&ngcd,0,sizeof(ngcd));
    memset(&tf,0,sizeof(tf));

    ngcd[0].gd.pos.y = 0; ngcd[0].gd.pos.height = mgcd[0].gd.pos.height;
    ngcd[0].gd.pos.width = _GScrollBar_Width;
    ngcd[0].gd.pos.x = mgcd[0].gd.pos.width-ngcd[0].gd.pos.width;
    ngcd[0].gd.flags = gg_visible|gg_enabled|gg_sb_vert;
    ngcd[0].creator = GScrollBarCreate;

    tfbox.main_background = tfbox.main_foreground = COLOR_DEFAULT;
    ngcd[1].gd.pos.y = -100; ngcd[0].gd.pos.height = postv->fh;
    ngcd[1].gd.pos.x = postv->addrend+EDGE_SPACER;
    tf.text = num+1;
    tf.font = postv->gfont;
    ngcd[1].gd.label = &tf;
    ngcd[1].gd.box = &tfbox;
    ngcd[1].gd.flags = gg_visible|gg_enabled|gg_sb_vert|gg_dontcopybox;
    ngcd[1].creator = GTextFieldCreate;
/******************************************************************************/
    memset(&egcd,0,sizeof(egcd));

    egcd[0].gd.pos.y = 0; egcd[0].gd.pos.height = mgcd[0].gd.pos.height;
    egcd[0].gd.pos.width = _GScrollBar_Width;
    egcd[0].gd.pos.x = mgcd[0].gd.pos.width-egcd[0].gd.pos.width;
    egcd[0].gd.flags = gg_visible|gg_enabled|gg_sb_vert;
    egcd[0].creator = GScrollBarCreate;

    egcd[1] = ngcd[1];
/******************************************************************************/

    GGadgetsCreate(gw,mgcd);
    postv->tabs = mgcd[0].ret;
    postv->ok = mgcd[1].ret;
    postv->cancel = mgcd[2].ret;
    postv->box = mgcd[3].ret;
    postv->nsb = ngcd[0].ret;
    postv->esb = egcd[0].ret;
    postv->ntf = ngcd[1].ret;
    postv->etf = egcd[1].ret;
    postv->nv = GTabSetGetSubwindow(postv->tabs,1);
    postv->ev = GTabSetGetSubwindow(postv->tabs,2);
    postv->sbw = GDrawPointsToPixels(postv->ev,_GScrollBar_Width);
    postv->nactive = postv->eactive = -1;

    GWidgetSetEH(postv->nv,nv_e_h);
    GWidgetSetEH(postv->ev,ev_e_h);
    GDrawGetSize(postv->ev,&pos);
    GGadgetMove(postv->nsb,pos.width-postv->sbw,0);
    GGadgetMove(postv->esb,pos.width-postv->sbw,0);
    GGadgetResize(postv->nsb,postv->sbw,pos.height);
    GGadgetResize(postv->esb,postv->sbw,pos.height);
    postv->vheight = pos.height;
    postv->vwidth = pos.width;

    GDrawSetVisible(gw,true);
}
