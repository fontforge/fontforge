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
#include "chardata.h"
#include "utype.h"

struct gfi_data {
    int done;
    SplineFont *sf;
    GWindow gw;
};

GTextInfo encodingtypes[] = {
    { (unichar_t *) _STR_Custom, NULL, 0, 0, (void *) em_none, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_Isolatin1, NULL, 0, 0, (void *) em_iso8859_1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin0, NULL, 0, 0, (void *) em_iso8859_15, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin2, NULL, 0, 0, (void *) em_iso8859_2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin3, NULL, 0, 0, (void *) em_iso8859_3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin4, NULL, 0, 0, (void *) em_iso8859_4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin5, NULL, 0, 0, (void *) em_iso8859_9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin6, NULL, 0, 0, (void *) em_iso8859_10, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin7, NULL, 0, 0, (void *) em_iso8859_13, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin8, NULL, 0, 0, (void *) em_iso8859_14, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_Isocyrillic, NULL, 0, 0, (void *) em_iso8859_5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Koi8cyrillic, NULL, 0, 0, (void *) em_koi8_r, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isoarabic, NULL, 0, 0, (void *) em_iso8859_6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isogreek, NULL, 0, 0, (void *) em_iso8859_7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isohebrew, NULL, 0, 0, (void *) em_iso8859_8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_Mac, NULL, 0, 0, (void *) em_mac, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Win, NULL, 0, 0, (void *) em_win, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Adobestd, NULL, 0, 0, (void *) em_adobestandard, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Symbol, NULL, 0, 0, (void *) em_symbol, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Texbase, NULL, 0, 0, (void *) em_base, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_Unicode, NULL, 0, 0, (void *) em_unicode, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_Jis208, NULL, 0, 0, (void *) em_jis208, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Jis212, NULL, 0, 0, (void *) em_jis212, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Korean, NULL, 0, 0, (void *) em_ksc5601, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Chinese, NULL, 0, 0, (void *) em_gb2312, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};

#define CID_Encoding	1001
#define CID_Family	1002
#define CID_Modifiers	1003
#define CID_ItalicAngle	1004
#define CID_UPos	1005
#define CID_UWidth	1006
#define CID_Ascent	1007
#define CID_Descent	1008
#define CID_NChars	1009
#define CID_Notice	1010
#define CID_Make	1011
#define CID_Delete	1012
#define CID_XUID	1013
#define CID_Human	1014

static int infont(SplineChar *sc, const unsigned short *table, int tlen,
	Encoding *item, uint8 *used) {
    int i;
    /* for some reason, some encodings have multiple entries for the same */
    /*  glyph. One obvious one is space: 0x20 and 0xA0. The used bit array */
    /*  is designed to handle that unpleasantness */

    if ( table==NULL ) {
	if ( sc->unicodeenc==-1 ) {
	    if ( sc->enc!=0 || (used[0]&1) ) {
		sc->enc = -1;
return( false );
	    }
	    used[0] |= 1;
return( true );
	}
	if ( used[sc->unicodeenc>>3] & (1<<(sc->unicodeenc&7)) ) {
	    sc->enc = -1;
return( false );
	}
	used[sc->unicodeenc>>3] |= (1<<(sc->unicodeenc&7));
	sc->enc = sc->unicodeenc;
return( true );
    }
    if ( sc->unicodeenc==-1 ) {
	if ( sc->enc==0 && strcmp(sc->name,".notdef")==0 && table[0]==0 && !(used[0]&1)) {
	    used[0] |= 1;
return( true );			/* .notdef goes to encoding 0 */
	} else if ( item!=NULL && item->psnames!=NULL ) {
	    for ( i=0; i<tlen ; ++i ) {
		if ( item->psnames[i]!=NULL && strcmp(item->psnames[i],sc->name)==0 &&
			!(used[i>>3]&(1<<(i&7))) ) {
		    used[i>>3] |= (1<<(i&7));
		    sc->enc = i;
return( true );
		}
	    }
	} else {
	    sc->enc = -1;
return( false );
	}
    }

    for ( i=0; i<tlen && (sc->unicodeenc!=table[i] || (used[i>>3]&(1<<(i&7)))); ++i );
    if ( i==tlen ) {
	sc->enc = -1;
return( false );
    } else {
	used[i>>3] |= (1<<(i&7));
	if ( tlen==94*94 ) {
	    sc->enc = (i/94)*96+(i%94)+1;
return( true );
	} else {
	    sc->enc = i;
return( true );
	}
    }
}

static void RemoveSplineChar(SplineFont *sf, int enc) {
    CharView *cv, *next;
    struct splinecharlist *dep, *dnext;
    BDFFont *bdf;
    BDFChar *bfc;
    SplineChar *sc = sf->chars[enc];
    BitmapView *bv, *bvnext;
    RefChar *refs, *rnext;

    if ( sc!=NULL ) {
	if ( sc->views ) {
	    for ( cv = sc->views; cv!=NULL; cv=next ) {
		next = cv->next;
		GDrawDestroyWindow(cv->gw);
	    }
	    GDrawSync(NULL);
	    GDrawProcessPendingEvents(NULL);
	    GDrawSync(NULL);
	    GDrawProcessPendingEvents(NULL);
	}
	for ( dep=sc->dependents; dep!=NULL; dep=dnext ) {
	    SplineChar *dsc = dep->sc;
	    RefChar *rf, *rnext;
	    dnext = dep->next;
	    /* May be more than one reference to us, colon has two refs to period */
	    /*  but only one dlist entry */
	    for ( rf = dsc->refs; rf!=NULL; rf=rnext ) {
		rnext = rf->next;
		SCRefToSplines(dsc,rf);
	    }
	}
	sf->chars[enc] = NULL;

	for ( refs=sc->refs; refs!=NULL; refs = rnext ) {
	    rnext = refs->next;
	    SCRemoveDependent(sc,refs);
	}
	SplineCharFree(sc);
    }

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	if ( (bfc = bdf->chars[enc])!= NULL ) {
	    bdf->chars[enc] = NULL;
	    if ( bfc->views!=NULL ) {
		for ( bv= bfc->views; bv!=NULL; bv=bvnext ) {
		    bvnext = bv->next;
		    GDrawDestroyWindow(bv->gw);
		}
		GDrawSync(NULL);
		GDrawProcessPendingEvents(NULL);
		GDrawSync(NULL);
		GDrawProcessPendingEvents(NULL);
	    }
	    BDFCharFree(bfc);
	}
    }
}

/* see also SplineFontNew in splineutil2.c */
int SFReencodeFont(SplineFont *sf,enum charset new_map) {
    const unsigned short *table;
    int i, extras, epos;
    SplineChar **chars;
    int enc_cnt;
    BDFFont *bdf;
    int tlen = 256;
    Encoding *item=NULL;
    uint8 *used;

    if ( sf->encoding_name==new_map )
return(false);
    if ( new_map==em_none ) {
	sf->encoding_name=em_none;	/* Custom, it's whatever's there */
return(false);
    }
    if ( new_map==em_adobestandard ) {
	table = unicode_from_adobestd;
    } else if ( new_map>=em_base ) {
	for ( item=enclist; item!=NULL && item->enc_num!=new_map; item=item->next );
	if ( item!=NULL ) {
	    tlen = item->char_cnt;
	    table = item->unicode;
	} else {
	    GDrawError("Invalid encoding");
return( false );
	}
    } else if ( new_map==em_iso8859_1 )
	table = unicode_from_i8859_1;
    else if ( new_map==em_unicode ) {
	table = NULL;
	tlen = 65536;
    } else if ( new_map==em_jis208 ) {
	table = unicode_from_jis208;
	tlen = 94*94;
    } else if ( new_map==em_jis212 ) {
	table = unicode_from_jis212;
	tlen = 94*94;
    } else if ( new_map==em_ksc5601 ) {
	table = unicode_from_ksc5601;
	tlen = 94*94;
    } else if ( new_map==em_gb2312 ) {
	table = unicode_from_gb2312;
	tlen = 94*94;
    } else
	table = unicode_from_alphabets[new_map+3];

    extras = 0;
    used = gcalloc((tlen+7)/8,sizeof(uint8));
    for ( i=0; i<sf->charcnt; ++i ) {
	if ( sf->chars[i]==NULL )
	    /* skip */;
	else if ( !infont(sf->chars[i],table,tlen,item,used)) {
	    SplineChar *sc = sf->chars[i];
	    if ( sc->splines==NULL && sc->refs==NULL && sc->dependents==NULL
		    /*&& sc->width==sf->ascent+sf->descent*/ ) {
		RemoveSplineChar(sf,i);
	    } else
		++extras;
	}
    }
    free(used);
    enc_cnt=tlen;
    if ( new_map==em_unicode )
	enc_cnt = 65536;
    else if ( tlen == 94*94 )
	enc_cnt = 94*96;
    chars = calloc(enc_cnt+extras,sizeof(SplineChar *));
    for ( bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next )
	bdf->temp = calloc(enc_cnt+extras,sizeof(BDFChar *));
    for ( i=0, epos=enc_cnt; i<sf->charcnt; ++i ) {
	if ( sf->chars[i]==NULL )
	    /* skip */;
	else {
	    if ( sf->chars[i]->enc==-1 )
		sf->chars[i]->enc = epos++;
	    chars[sf->chars[i]->enc] = sf->chars[i];
	    for ( bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
		if ( bdf->chars[i]!=NULL ) {
		    if ( sf->chars[i]!=NULL )
			bdf->chars[i]->enc = sf->chars[i]->enc;
		    bdf->temp[sf->chars[i]->enc] = bdf->chars[i];
		}
	    }
	}
    }
    if ( epos!=enc_cnt+extras ) GDrawIError( "Bad count in ReencodeFont");
    for ( i=0; i<256; ++i ) {
	if ( chars[i]==NULL ) {
	    SplineChar *sc = chars[i] = calloc(1,sizeof(SplineChar));
	    sc->enc = i;
	    if ( table==NULL )
		sc->unicodeenc = i;
	    else if ( new_map>=em_jis208 && new_map<em_base ) {
		if ( i%96==0 || i%96==95 )
		    sc->unicodeenc = -1;
		else
		    sc->unicodeenc = table[(i/96)*94+(i%96-1)];
		if ( sc->unicodeenc==0 ) sc->unicodeenc = -1;
	    } else if ( ( sc->unicodeenc = table[i])==0xfffd )
		sc->unicodeenc = -1;
	    if ( sc->unicodeenc == 0 && i!=0 )
		sc->unicodeenc = -1;
	    if ( sc->unicodeenc!=-1 && sc->unicodeenc<psunicodenames_cnt )
		sc->name = copy(psunicodenames[sc->unicodeenc]);
	    if ( sc->name==NULL ) {
		if ( sc->unicodeenc==0xa0 )
		    sc->name = copy("nonbreakingspace");
		else if ( sc->unicodeenc==0x2d )
		    sc->name = copy("hyphen-minus");
		else if ( sc->unicodeenc==0xad )
		    sc->name = copy("softhyphen");
		else if ( sc->unicodeenc==0x00 )
		    sc->name = copy(".notdef");
		else if ( sc->unicodeenc!=-1 ) {
		    char buf[10];
		    sprintf(buf,"uni%04X", sc->unicodeenc );
		    sc->name = copy(buf);
		} else if ( item!=NULL && item->psnames!=NULL && item->psnames[i]!=NULL ) {
		    sc->name = copy(item->psnames[i]);
		} else
		    sc->name = copy(".notdef");
	    }
	    sc->width = sf->ascent+sf->descent;
	    sc->parent = sf;
	    sc->lig = SCLigDefault(sc);
	}
    }
    free(sf->chars);
    sf->chars = chars;
    sf->charcnt = enc_cnt+extras;
    sf->encoding_name = new_map;
    for ( bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	free(bdf->chars);
	bdf->chars = bdf->temp;
	bdf->temp = NULL;
	bdf->charcnt = enc_cnt+extras;
	bdf->encoding_name = new_map;
    }
return( true );
}

static int AddDelChars(SplineFont *sf, int nchars) {
    int i;
    BDFFont *bdf;
    MetricsView *mv, *mnext;

    if ( nchars==sf->charcnt )
return( false );
    if ( nchars>sf->charcnt ) {
	int is_unicode = 1;
	for ( i=0; i<sf->charcnt && is_unicode; ++i )
	    if ( sf->chars[i]==NULL || sf->chars[i]->unicodeenc!=i )
		is_unicode = false;
	sf->chars = realloc(sf->chars,nchars*sizeof(SplineChar *));
	for ( i=sf->charcnt; i<nchars; ++i )
	    sf->chars[i] = NULL;
#if 0
	for ( i=sf->charcnt; i<nchars; ++i ) {
	    SplineChar *sc = sf->chars[i] = calloc(1,sizeof(SplineChar));
	    sc->enc = i;
	    if ( is_unicode ) {
		sc->unicodeenc = i;
		if ( i<psunicodenames_cnt && psunicodenames[i]!=NULL )
		    sc->name = copy(psunicodenames[i]);
		else { char buf[10];
		    sprintf( buf, "uni%04X", i );
		    sc->name = copy(buf);
		}
	    } else
		sc->unicodeenc = -1;
	    sc->width = sf->ascent+sf->descent;
	    sc->parent = sf;
	}
#endif
	sf->charcnt = nchars;
	for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    bdf->chars = realloc(bdf->chars,nchars*sizeof(BDFChar *));
	    for ( i=bdf->charcnt; i<nchars; ++i )
		bdf->chars[i] = NULL;
	    bdf->charcnt = nchars;
	}
    } else {
	if ( sf->fv->metrics!=NULL ) {
	    for ( mv=sf->fv->metrics; mv!=NULL; mv = mnext ) {
		mnext = mv->next;
		GDrawDestroyWindow(mv->gw);
	    }
	    GDrawSync(NULL);
	    GDrawProcessPendingEvents(NULL);
	    GDrawSync(NULL);
	    GDrawProcessPendingEvents(NULL);
	}
	for ( i=nchars; i<sf->charcnt; ++i ) {
	    RemoveSplineChar(sf,i);
	}
	sf->charcnt = nchars;
	if ( nchars<256 ) sf->encoding_name = em_none;
	for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    bdf->charcnt = nchars;
	    bdf->encoding_name = sf->encoding_name;
	}
    }
return( true );
}

static void RegenerateEncList(struct gfi_data *d) {
    Encoding *item;
    GTextInfo **ti;
    int i, any;
    unichar_t *title=NULL;

    i = 0;
    for ( item=enclist; item!=NULL ; item=item->next )
	if ( !item->builtin )
	    ++i;
    any = i!=0;
    i += sizeof(encodingtypes)/sizeof(encodingtypes[0]);
    ti = gcalloc(i+1,sizeof(GTextInfo *));
    for ( i=0; i<sizeof(encodingtypes)/sizeof(encodingtypes[0])-1; ++i ) {
	ti[i] = galloc(sizeof(GTextInfo));
	memcpy(ti[i],&encodingtypes[i],sizeof(GTextInfo));
	if ( encodingtypes[i].text_is_1byte ) {
	    ti[i]->text = uc_copy((char *) ti[i]->text);
	    ti[i]->text_is_1byte = false;
	} else {
	    ti[i]->text = u_copy(ti[i]->text);
	}
	ti[i]->bg = ti[i]->fg = COLOR_DEFAULT;
	ti[i]->selected = ( (int) (ti[i]->userdata)==d->sf->encoding_name) &&
		!ti[i]->line;
	if ( ti[i]->selected )
	    title = ti[i]->text;
    }
    if ( any ) {
	ti[i] = gcalloc(1,sizeof(GTextInfo));
	ti[i]->bg = ti[i]->fg = COLOR_DEFAULT;
	ti[i++]->line = true;
	for ( item=enclist; item!=NULL ; item=item->next )
	    if ( !item->builtin ) {
		ti[i] = gcalloc(1,sizeof(GTextInfo));
		ti[i]->bg = ti[i]->fg = COLOR_DEFAULT;
		ti[i]->text = uc_copy(item->enc_name);
		ti[i]->userdata = (void *) item->enc_num;
		ti[i]->selected = ( item->enc_num==d->sf->encoding_name);
		if ( ti[i++]->selected )
		    title = ti[i-1]->text;
	    }
    }
    ti[i] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(GWidgetGetControl(d->gw,CID_Encoding),ti,false);
    if ( title!=NULL )
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_Encoding),title);

    for ( item=enclist; item!=NULL && item->builtin; item=item->next );
    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Delete),item!=NULL);
}

static int GFI_Delete(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	RemoveEncoding();
	RegenerateEncList(d);
    }
return( true );
}

static int GFI_Load(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	LoadEncodingFile();
	RegenerateEncList(d);
    }
return( true );
}

static int GFI_Make(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	Encoding *item = MakeEncoding(d->sf);
	if ( item!=NULL ) {
	    d->sf->encoding_name = item->enc_num;
	    RegenerateEncList(d);
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Make),false);
	}
    }
return( true );
}

static int GFI_NameChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GWindow gw = GGadgetGetWindow(g);
	const unichar_t *ufamily = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Family));
	const unichar_t *umods = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Modifiers));
	unichar_t *uhum = galloc((u_strlen(ufamily)+u_strlen(umods)+1)*sizeof(unichar_t));
	u_strcpy(uhum,ufamily);
	u_strcat(uhum,umods);
	GGadgetSetTitle(GWidgetGetControl(gw,CID_Human),uhum);
	free(uhum);
    }
return( true );
}

static int GFI_GuessItalic(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	double val = SFGuessItalicAngle(d->sf);
	char buf[30]; unichar_t ubuf[30];
	sprintf( buf, "%.1f", val);
	uc_strcpy(ubuf,buf);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_ItalicAngle),ubuf);
    }
return( true );
}

static int GFI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = true;
    }
return( true );
}

static int AskTooFew() {
    static int buts[] = { _STR_OK, _STR_Cancel, 0 };
return( GWidgetAskR(_STR_Toofew,_STR_Reducing,buts,0,1) );
}

static void BadFamily() {
    GWidgetPostNoticeR(_STR_Badfamily,_STR_Badfamilyn);
}

static char *GetModifiers(char *fontname) {
    char *ipt, *bpt, *ept, *dpt;

    dpt = strchr(fontname,'-');
    if ((ipt = strstrmatch(fontname,"Italic"))==NULL )
	if ((ipt = strstrmatch(fontname,"Oblique"))==NULL )
	    if ((ipt = strstrmatch(fontname,"Cursive"))==NULL )
		;
    if ((bpt = strstrmatch(fontname,"Demi"))==NULL )
	if ((bpt = strstrmatch(fontname,"Light"))==NULL )
	    if ((bpt = strstrmatch(fontname,"Black"))==NULL )
		if ((bpt = strstrmatch(fontname,"Bold"))==NULL )
		    if ((bpt = strstrmatch(fontname,"Medium"))==NULL )
			if ((bpt = strstrmatch(fontname,"Roman"))==NULL )
			    if ((bpt = strstrmatch(fontname,"Heavy"))==NULL )
				;
    if ((ept = strstrmatch(fontname,"Expanded"))==NULL )
	if ((ept = strstrmatch(fontname,"Condensed"))==NULL )
	    if ((ept = strstrmatch(fontname,"Outline"))==NULL )
		if ((ept = strstrmatch(fontname,"SmallCaps"))==NULL )
		    ;
    if ( ept==NULL ) ept = (bpt!=NULL)?bpt:(dpt==NULL)?ipt:dpt;
    if ( bpt==NULL ) bpt = ept;
    if ( ipt==NULL ) ipt = ept;
    if ( dpt==NULL ) dpt = ept;
    if ( ipt==NULL )
return( "" );
    if ( ipt<=ept && ipt<=bpt && ipt<=dpt )
return( ipt );
    if ( bpt<=ept && bpt<=dpt )
return( bpt );
    if ( dpt<=ept )
return( dpt );

return( ept );
}

void SFSetFontName(SplineFont *sf, char *family, char *mods,char *full) {
    char *n;
    unichar_t *temp; char *pt, *tpt;
    int i;

    n = malloc(strlen(family)+strlen(mods)+1);
    strcpy(n,family); strcat(n,mods);
    if ( full==NULL || *full == '\0' )
	full = copy(n);
    for ( pt=tpt=n; *pt; ) {
	if ( !isspace(*pt))
	    *tpt++ = *pt++;
	else
	    ++pt;
    }
    *tpt = '\0';
    for ( pt=tpt=family; *pt; ) {
	if ( !isspace(*pt))
	    *tpt++ = *pt++;
	else
	    ++pt;
    }
    *tpt = '\0';
    free(sf->fontname); sf->fontname = n;
    free(sf->fullname); sf->fullname = full;
    free(sf->familyname); sf->familyname = copy(family);
    free(sf->weight); sf->weight = NULL;
    if ( strstrmatch(mods,"extralight")!=NULL || strstrmatch(mods,"extra-light")!=NULL )
	sf->weight = copy("ExtraLight");
    else if ( strstrmatch(mods,"demilight")!=NULL || strstrmatch(mods,"demi-light")!=NULL )
	sf->weight = copy("DemiLight");
    else if ( strstrmatch(mods,"demibold")!=NULL || strstrmatch(mods,"demi-bold")!=NULL )
	sf->weight = copy("DemiBold");
    else if ( strstrmatch(mods,"demiblack")!=NULL || strstrmatch(mods,"demi-black")!=NULL )
	sf->weight = copy("DemiBlack");
    else if ( strstrmatch(mods,"extrabold")!=NULL || strstrmatch(mods,"extra-bold")!=NULL )
	sf->weight = copy("ExtraBold");
    else if ( strstrmatch(mods,"extrablack")!=NULL || strstrmatch(mods,"extra-black")!=NULL )
	sf->weight = copy("ExtraBlack");
    else if ( strstrmatch(mods,"book")!=NULL )
	sf->weight = copy("Book");
    else if ( strstrmatch(mods,"regular")!=NULL )
	sf->weight = copy("Regular");
    else if ( strstrmatch(mods,"roman")!=NULL )
	sf->weight = copy("Roman");
    else if ( strstrmatch(mods,"normal")!=NULL )
	sf->weight = copy("Normal");
    else if ( strstrmatch(mods,"demi")!=NULL )
	sf->weight = copy("Demi");
    else if ( strstrmatch(mods,"medium")!=NULL )
	sf->weight = copy("Medium");
    else if ( strstrmatch(mods,"bold")!=NULL )
	sf->weight = copy("Bold");
    else if ( strstrmatch(mods,"heavy")!=NULL )
	sf->weight = copy("Heavy");
    else if ( strstrmatch(mods,"black")!=NULL )
	sf->weight = copy("Black");
    else
	sf->weight = copy("Medium");

    if ( sf->fv!=NULL ) {
	GDrawSetWindowTitles(sf->fv->gw,temp = uc_copy(sf->fontname),NULL);
	free(temp);
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->views!=NULL ) {
	    char buffer[300]; unichar_t ubuf[300]; CharView *cv;
	    sprintf( buffer, "%.90s from %.90s", sf->chars[i]->name, sf->fontname );
	    uc_strcpy(ubuf,buffer);
	    for ( cv = sf->chars[i]->views; cv!=NULL; cv=cv->next )
		GDrawSetWindowTitles(cv->gw,ubuf,NULL);
	}
    }
}

static void SetFontName(GWindow gw, SplineFont *sf) {
    const unichar_t *ufamily = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Family));
    const unichar_t *umods = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Modifiers));
    const unichar_t *uhum = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Human));
    char *family, *mods, *human;

    if ( uc_strcmp(ufamily,sf->familyname)==0 && uc_strcmp(uhum,sf->fullname)==0 &&
	    uc_strcmp(umods,GetModifiers(sf->fontname))==0 )
return;			/* Unchanged */
    family = cu_copy(ufamily); mods = cu_copy(umods); human = cu_copy(uhum);
    SFSetFontName(sf,family,mods,human);
    free(mods); free(family); free(human);
}

static int GFI_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct gfi_data *d = GDrawGetUserData(gw);
	SplineFont *sf = d->sf;
	int enc;
	int reformat_fv=0;
	int upos, uwid, as, des, nchar, oldcnt=sf->charcnt, err = false;
	double ia;
	const unichar_t *txt; unichar_t *end;

	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Family));
	if ( !isalpha(*txt)) {
	    BadFamily();
return( true );
	}
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_ItalicAngle));
	ia = u_strtod(txt,&end);
	if ( *end!='\0' ) {
	    ProtestR(_STR_Italicangle);
return(true);
	}
	upos = GetIntR(gw,CID_UPos, _STR_Upos,&err);
	uwid = GetIntR(gw,CID_UWidth,_STR_Uheight,&err);
	as = GetIntR(gw,CID_Ascent,_STR_Ascent,&err);
	des = GetIntR(gw,CID_Descent,_STR_Descent,&err);
	nchar = GetIntR(gw,CID_NChars,_STR_Numchars,&err);
	if ( err )
return(true);
	if ( as+des>16384 || des<0 || as<0 ) {
	    GWidgetPostNoticeR(_STR_Badascentdescent,_STR_Badascentdescentn);
return( true );
	}
	if ( nchar<sf->charcnt && AskTooFew())
return(true);
	GDrawSetCursor(gw,ct_watch);
	SetFontName(gw,sf);
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_XUID));
	free(sf->xuid); sf->xuid = *txt=='\0'?NULL:cu_copy(txt);
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Notice));
	free(sf->copyright); sf->copyright = cu_copy(txt);
	enc = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,CID_Encoding));
	if ( enc!=-1 ) {
	    enc = (int) (GGadgetGetListItem(GWidgetGetControl(gw,CID_Encoding),enc)->userdata);
	    reformat_fv = SFReencodeFont(sf,enc);
	    if ( reformat_fv && nchar==oldcnt )
		nchar = sf->charcnt;
	}
	if ( nchar!=sf->charcnt )
	    reformat_fv = AddDelChars(sf,nchar);
	if ( as!=sf->ascent || des!=sf->descent ) {
	    sf->ascent = as;
	    sf->descent = des;
	    reformat_fv = true;
	}
	sf->italicangle = ia;
	sf->upos = upos;
	sf->uwidth = uwid;
	if ( reformat_fv )
	    FontViewReformat(sf->fv);
	sf->changed = true;
	sf->changed_since_autosave = true;
	d->done = true;
    }
return( true );
}

GTextInfo *EncodingTypesFindEnc(GTextInfo *encodingtypes, int enc) {
    int i;

    for ( i=0; encodingtypes[i].text!=NULL || encodingtypes[i].line; ++i ) {
	if ( encodingtypes[i].text==NULL )
	    ;
	else if ( encodingtypes[i].userdata == (void *) enc )
return( &encodingtypes[i] );
    }
return( NULL );
}

GTextInfo *GetEncodingTypes(void) {
    GTextInfo *ti;
    int i;
    Encoding *item;

    i = 0;
    for ( item=enclist; item!=NULL ; item=item->next )
	if ( !item->builtin )
	    ++i;
    if ( i==0 )
return( encodingtypes );
    i += sizeof(encodingtypes)/sizeof(encodingtypes[0]);
    ti = gcalloc(i+1,sizeof(GTextInfo));
    memcpy(ti,encodingtypes,sizeof(encodingtypes)-sizeof(encodingtypes[0]));
    for ( i=0; i<sizeof(encodingtypes)/sizeof(encodingtypes[0])-1; ++i ) {
	if ( ti[i].text_is_1byte ) {
	    ti[i].text = uc_copy((char *) ti[i].text);
	    ti[i].text_is_1byte = false;
	} else if ( ti[i].text_in_resource ) {
	    ti[i].text = u_copy(GStringGetResource( (int) ti[i].text,NULL));
	    ti[i].text_in_resource = false;
	} else
	    ti[i].text = u_copy(ti[i].text);
    }
    ti[i++].line = true;
    for ( item=enclist; item!=NULL ; item=item->next )
	if ( !item->builtin ) {
	    ti[i].text = uc_copy(item->enc_name);
	    ti[i++].userdata = (void *) item->enc_num;
	}
return( ti );
}

static int e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct gfi_data *d = GDrawGetUserData(gw);
	d->done = true;
    }
return( event->type!=et_char );
}

void FontMenuFontInfo(void *_fv) {
    FontView *fv = (FontView *) _fv;
    SplineFont *sf = fv->sf;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[33];
    GTextInfo label[33], *list;
    struct gfi_data d;
    char iabuf[20], upbuf[20], uwbuf[20], asbuf[20], dsbuf[20], ncbuf[20];
    int i;
    int oldcnt = sf->charcnt;
    Encoding *item;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(_STR_Fontinformation,NULL);
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,260);
    pos.height = GDrawPointsToPixels(NULL,335);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _STR_Familyname;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.mnemonic = 'F';
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; 
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].creator = GLabelCreate;

    gcd[1].gd.pos.x = 12; gcd[1].gd.pos.y = gcd[0].gd.pos.y+15; gcd[1].gd.pos.width = 120;
    gcd[1].gd.flags = gg_visible | gg_enabled;
    label[1].text = (unichar_t *) (sf->familyname);
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.cid = CID_Family;
    gcd[1].gd.handle_controlevent = GFI_NameChange;
    gcd[1].creator = GTextFieldCreate;

    label[2].text = (unichar_t *) _STR_Fontmodifiers;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.mnemonic = 'M';
    gcd[2].gd.pos.x = 133; gcd[2].gd.pos.y = gcd[0].gd.pos.y; 
    gcd[2].gd.flags = gg_visible | gg_enabled;
    gcd[2].creator = GLabelCreate;

    gcd[3].gd.pos.x = 133; gcd[3].gd.pos.y = gcd[1].gd.pos.y; gcd[3].gd.pos.width = 120;
    gcd[3].gd.flags = gg_visible | gg_enabled;
    label[3].text = (unichar_t *) GetModifiers(sf->fontname);
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.cid = CID_Modifiers;
    gcd[3].gd.handle_controlevent = GFI_NameChange;
    gcd[3].creator = GTextFieldCreate;

    gcd[30].gd.pos.x = 12; gcd[30].gd.pos.y = gcd[1].gd.pos.y+26+6;
    label[30].text = (unichar_t *) _STR_Humanname;
    label[30].text_in_resource = true;
    gcd[30].gd.label = &label[30];
    gcd[30].gd.mnemonic = 'H';
    gcd[30].gd.flags = gg_visible | gg_enabled;
    gcd[30].creator = GLabelCreate;

    gcd[31].gd.pos.x = 105; gcd[31].gd.pos.y = gcd[30].gd.pos.y-6; gcd[31].gd.pos.width = 147;
    gcd[31].gd.flags = gg_visible | gg_enabled;
    label[31].text = (unichar_t *) sf->fullname;
    label[31].text_is_1byte = true;
    gcd[31].gd.label = &label[31];
    gcd[31].gd.cid = CID_Human;
    gcd[31].creator = GTextFieldCreate;

    gcd[4].gd.pos.x = 12; gcd[4].gd.pos.y = gcd[31].gd.pos.y+29+6;
    gcd[4].gd.flags = gg_visible | gg_enabled;
    gcd[4].gd.mnemonic = 'E';
    label[4].text = (unichar_t *) _STR_Encoding;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].creator = GLabelCreate;

    gcd[5].gd.pos.x = 80; gcd[5].gd.pos.y = gcd[4].gd.pos.y-6;
    gcd[5].gd.flags = gg_visible | gg_enabled;
    gcd[5].gd.u.list = list = GetEncodingTypes();
    gcd[5].gd.label = EncodingTypesFindEnc(list,sf->encoding_name);
    if ( gcd[5].gd.label==NULL ) gcd[5].gd.label = &list[0];
    gcd[5].gd.cid = CID_Encoding;
    gcd[5].creator = GListButtonCreate;
    for ( i=0; list[i].text!=NULL || list[i].line; ++i ) {
	if ( (void *) (sf->encoding_name)==list[i].userdata &&
		list[i].text!=NULL )
	    list[i].selected = true;
	else
	    list[i].selected = false;
    }

    gcd[27].gd.pos.x = 8; gcd[27].gd.pos.y = GDrawPointsToPixels(NULL,gcd[4].gd.pos.y+6);
    gcd[27].gd.pos.width = pos.width-16; gcd[27].gd.pos.height = GDrawPointsToPixels(NULL,43);
    gcd[27].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[27].creator = GGroupCreate;

    gcd[28].gd.pos.x = 12; gcd[28].gd.pos.y = gcd[5].gd.pos.y+27;
    gcd[28].gd.pos.width = -1; gcd[28].gd.pos.height = 0;
    gcd[28].gd.flags = gg_visible | gg_enabled;
    label[28].text = (unichar_t *) _STR_Load;
    label[28].text_in_resource = true;
    gcd[28].gd.mnemonic = 'L';
    gcd[28].gd.label = &label[28];
    gcd[28].gd.handle_controlevent = GFI_Load;
    gcd[28].creator = GButtonCreate;

    gcd[25].gd.pos.x = (260-100)/2; gcd[25].gd.pos.y = gcd[28].gd.pos.y;
    gcd[25].gd.pos.width = 100; gcd[25].gd.pos.height = 0;
    gcd[25].gd.flags = gg_visible;
    if ( sf->encoding_name==em_none ) gcd[25].gd.flags |= gg_enabled;
    label[25].text = (unichar_t *) _STR_Makefromfont;
    label[25].text_in_resource = true;
    gcd[25].gd.mnemonic = 'k';
    gcd[25].gd.label = &label[25];
    gcd[25].gd.handle_controlevent = GFI_Make;
    gcd[25].gd.cid = CID_Make;
    gcd[25].creator = GButtonCreate;

    gcd[26].gd.pos.x = 260-12-GIntGetResource(_NUM_Buttonsize); gcd[26].gd.pos.y = gcd[28].gd.pos.y;
    gcd[26].gd.pos.width = -1; gcd[26].gd.pos.height = 0;
    gcd[26].gd.flags = gg_visible;
    for ( item=enclist; item!=NULL && item->builtin; item=item->next );
    if ( item!=NULL ) gcd[26].gd.flags |= gg_enabled;
    label[26].text = (unichar_t *) _STR_Remove;
    label[26].text_in_resource = true;
    gcd[26].gd.mnemonic = 'R';
    gcd[26].gd.label = &label[26];
    gcd[26].gd.handle_controlevent = GFI_Delete;
    gcd[26].gd.cid = CID_Delete;
    gcd[26].creator = GButtonCreate;

    gcd[6].gd.pos.x = 12; gcd[6].gd.pos.y = gcd[28].gd.pos.y+32+6;
    gcd[6].gd.flags = gg_visible | gg_enabled;
    gcd[6].gd.mnemonic = 'I';
    label[6].text = (unichar_t *) _STR_Italicangle;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    gcd[6].creator = GLabelCreate;

    gcd[7].gd.pos.x = 103; gcd[7].gd.pos.y = gcd[6].gd.pos.y-6;
    gcd[7].gd.pos.width = 45;
    gcd[7].gd.flags = gg_visible | gg_enabled;
    sprintf( iabuf, "%g", sf->italicangle );
    label[7].text = (unichar_t *) iabuf;
    label[7].text_is_1byte = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.cid = CID_ItalicAngle;
    gcd[7].creator = GTextFieldCreate;

    gcd[8].gd.pos.x = 12; gcd[8].gd.pos.y = gcd[7].gd.pos.y+26+6;
    gcd[8].gd.flags = gg_visible | gg_enabled;
    gcd[8].gd.mnemonic = 'P';
    label[8].text = (unichar_t *) _STR_Upos;
    label[8].text_in_resource = true;
    gcd[8].gd.label = &label[8];
    gcd[8].creator = GLabelCreate;

    gcd[9].gd.pos.x = 103; gcd[9].gd.pos.y = gcd[8].gd.pos.y-6; gcd[9].gd.pos.width = 45;
    gcd[9].gd.flags = gg_visible | gg_enabled;
    sprintf( upbuf, "%g", sf->upos );
    label[9].text = (unichar_t *) upbuf;
    label[9].text_is_1byte = true;
    gcd[9].gd.label = &label[9];
    gcd[9].gd.cid = CID_UPos;
    gcd[9].creator = GTextFieldCreate;

    gcd[10].gd.pos.x = 155; gcd[10].gd.pos.y = gcd[8].gd.pos.y;
    gcd[10].gd.flags = gg_visible | gg_enabled;
    gcd[10].gd.mnemonic = 'H';
    label[10].text = (unichar_t *) _STR_Uheight;
    label[10].text_in_resource = true;
    gcd[10].gd.label = &label[10];
    gcd[10].creator = GLabelCreate;

    gcd[11].gd.pos.x = 200; gcd[11].gd.pos.y = gcd[9].gd.pos.y; gcd[11].gd.pos.width = 45;
    gcd[11].gd.flags = gg_visible | gg_enabled;
    sprintf( uwbuf, "%g", sf->uwidth );
    label[11].text = (unichar_t *) uwbuf;
    label[11].text_is_1byte = true;
    gcd[11].gd.label = &label[11];
    gcd[11].gd.cid = CID_UWidth;
    gcd[11].creator = GTextFieldCreate;

    gcd[12].gd.pos.x = 12; gcd[12].gd.pos.y = gcd[9].gd.pos.y+26+6;
    gcd[12].gd.flags = gg_visible | gg_enabled;
    gcd[12].gd.mnemonic = 'A';
    label[12].text = (unichar_t *) _STR_Ascent;
    label[12].text_in_resource = true;
    gcd[12].gd.label = &label[12];
    gcd[12].creator = GLabelCreate;

    gcd[13].gd.pos.x = 103; gcd[13].gd.pos.y = gcd[12].gd.pos.y-6; gcd[13].gd.pos.width = 45;
    gcd[13].gd.flags = gg_visible | gg_enabled;
    sprintf( asbuf, "%d", sf->ascent );
    label[13].text = (unichar_t *) asbuf;
    label[13].text_is_1byte = true;
    gcd[13].gd.label = &label[13];
    gcd[13].gd.cid = CID_Ascent;
    gcd[13].creator = GTextFieldCreate;

    gcd[14].gd.pos.x = 155; gcd[14].gd.pos.y = gcd[12].gd.pos.y;
    gcd[14].gd.flags = gg_visible | gg_enabled;
    gcd[14].gd.mnemonic = 'D';
    label[14].text = (unichar_t *) _STR_Descent;
    label[14].text_in_resource = true;
    gcd[14].gd.label = &label[14];
    gcd[14].creator = GLabelCreate;

    gcd[15].gd.pos.x = 200; gcd[15].gd.pos.y = gcd[13].gd.pos.y; gcd[15].gd.pos.width = 45;
    gcd[15].gd.flags = gg_visible | gg_enabled;
    sprintf( dsbuf, "%d", sf->descent );
    label[15].text = (unichar_t *) dsbuf;
    label[15].text_is_1byte = true;
    gcd[15].gd.label = &label[15];
    gcd[15].gd.cid = CID_Descent;
    gcd[15].creator = GTextFieldCreate;

    gcd[16].gd.pos.x = 12; gcd[16].gd.pos.y = gcd[13].gd.pos.y+26+6;
    gcd[16].gd.flags = gg_visible | gg_enabled;
    gcd[16].gd.mnemonic = 'r';
    label[16].text = (unichar_t *) _STR_Copyright;
    label[16].text_in_resource = true;
    gcd[16].gd.label = &label[16];
    gcd[16].creator = GLabelCreate;

    gcd[17].gd.pos.x = 103; gcd[17].gd.pos.y = gcd[16].gd.pos.y-6; gcd[17].gd.pos.width = 142;
    gcd[17].gd.flags = gg_visible | gg_enabled;
    if ( sf->copyright!=NULL ) {
	label[17].text = (unichar_t *) sf->copyright;
	label[17].text_is_1byte = true;
	gcd[17].gd.label = &label[17];
    }
    gcd[17].gd.cid = CID_Notice;
    gcd[17].creator = GTextFieldCreate;

    gcd[18].gd.pos.x = 12; gcd[18].gd.pos.y = gcd[17].gd.pos.y+26+6;
    gcd[18].gd.flags = gg_visible | gg_enabled;
    gcd[18].gd.mnemonic = 'x';
    label[18].text = (unichar_t *) _STR_Xuid;
    label[18].text_in_resource = true;
    gcd[18].gd.label = &label[18];
    gcd[18].creator = GLabelCreate;

    gcd[19].gd.pos.x = 103; gcd[19].gd.pos.y = gcd[18].gd.pos.y-6; gcd[19].gd.pos.width = 142;
    gcd[19].gd.flags = gg_visible | gg_enabled;
    if ( sf->xuid!=NULL ) {
	label[19].text = (unichar_t *) sf->xuid;
	label[19].text_is_1byte = true;
	gcd[19].gd.label = &label[19];
    }
    gcd[19].gd.cid = CID_XUID;
    gcd[19].creator = GTextFieldCreate;

    gcd[20].gd.pos.x = 12; gcd[20].gd.pos.y = gcd[19].gd.pos.y+26+6;
    gcd[20].gd.flags = gg_visible | gg_enabled;
    gcd[20].gd.mnemonic = 'N';
    label[20].text = (unichar_t *) _STR_Numchars;
    label[20].text_in_resource = true;
    gcd[20].gd.label = &label[20];
    gcd[20].creator = GLabelCreate;

    gcd[21].gd.pos.x = 123; gcd[21].gd.pos.y = gcd[20].gd.pos.y-6; gcd[21].gd.pos.width = 60;
    gcd[21].gd.flags = gg_visible | gg_enabled;
    sprintf( ncbuf, "%d", sf->charcnt );
    label[21].text = (unichar_t *) ncbuf;
    label[21].text_is_1byte = true;
    gcd[21].gd.label = &label[21];
    gcd[21].gd.cid = CID_NChars;
    gcd[21].creator = GTextFieldCreate;

    gcd[22].gd.pos.x = 30-3; gcd[22].gd.pos.y = 336-35-3;
    gcd[22].gd.pos.width = -1; gcd[22].gd.pos.height = 0;
    gcd[22].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[22].text = (unichar_t *) _STR_OK;
    label[22].text_in_resource = true;
    gcd[22].gd.label = &label[22];
    gcd[22].gd.handle_controlevent = GFI_OK;
    gcd[22].creator = GButtonCreate;

    gcd[23].gd.pos.x = 250-GIntGetResource(_NUM_Buttonsize)-30; gcd[23].gd.pos.y = gcd[22].gd.pos.y+3;
    gcd[23].gd.pos.width = -1; gcd[23].gd.pos.height = 0;
    gcd[23].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[23].text = (unichar_t *) _STR_Cancel;
    label[23].text_in_resource = true;
    gcd[23].gd.label = &label[23];
    gcd[23].gd.handle_controlevent = GFI_Cancel;
    gcd[23].creator = GButtonCreate;

    gcd[24].gd.pos.x = 2; gcd[24].gd.pos.y = 2;
    gcd[24].gd.pos.width = pos.width-4; gcd[24].gd.pos.height = pos.height-2;
    gcd[24].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[24].creator = GGroupCreate;

    gcd[29].gd.pos.y = gcd[7].gd.pos.y;
    gcd[29].gd.pos.width = GIntGetResource(_NUM_Buttonsize); gcd[29].gd.pos.height = 0;
    gcd[29].gd.pos.x = gcd[11].gd.pos.x+gcd[11].gd.pos.width-gcd[29].gd.pos.width;
    /*if ( strstrmatch(sf->fontname,"Italic")!=NULL ||strstrmatch(sf->fontname,"Oblique")!=NULL )*/
	gcd[29].gd.flags = gg_visible | gg_enabled;
    label[29].text = (unichar_t *) _STR_Guess;
    label[29].text_in_resource = true;
    gcd[29].gd.label = &label[29];
    gcd[29].gd.mnemonic = 'G';
    gcd[29].gd.handle_controlevent = GFI_GuessItalic;
    gcd[29].creator = GButtonCreate;

    GGadgetsCreate(gw,gcd);
    if ( list!=encodingtypes )
	GTextInfoListFree(list);
    GWidgetIndicateFocusGadget(gcd[1].ret);
    /*GTextFieldSelect(gcd[1].ret,0,-1);*/

    memset(&d,'\0',sizeof(d));
    d.sf = sf;
    d.gw = gw;

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);

    if ( oldcnt!=sf->charcnt && fv!=NULL ) {
	free(fv->selected);
	fv->selected = gcalloc(sf->charcnt,sizeof(char));
    }
    if ( fv!=NULL )
	GDrawRequestExpose(fv->v,NULL,false);
}
