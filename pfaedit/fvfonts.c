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
#include "ustring.h"
#include "utype.h"
#include "gfile.h"
#include "chardata.h"

static RefChar *RefCharsCopy(RefChar *ref) {
    RefChar *rhead=NULL, *last=NULL, *cur;

    while ( ref!=NULL ) {
	cur = chunkalloc(sizeof(RefChar));
	*cur = *ref;
	cur->splines = NULL;	/* Leave the old sc, we'll fix it later */
	cur->next = NULL;
	if ( rhead==NULL )
	    rhead = cur;
	else
	    last->next = cur;
	last = cur;
	ref = ref->next;
    }
return( rhead );
}

SplineChar *SplineCharCopy(SplineChar *sc) {
    SplineChar *nsc = chunkalloc(sizeof(SplineChar));

    *nsc = *sc;
    nsc->enc = -2;
    nsc->name = copy(sc->name);
    nsc->splines = SplinePointListCopy(nsc->splines);
    nsc->hstem = StemInfoCopy(nsc->hstem);
    nsc->vstem = StemInfoCopy(nsc->vstem);
    nsc->dstem = DStemInfoCopy(nsc->dstem);
    nsc->md = MinimumDistanceCopy(nsc->md);
    nsc->refs = RefCharsCopy(nsc->refs);
    nsc->views = NULL;
    nsc->parent = NULL;
    nsc->changed = true;
    nsc->dependents = NULL;		/* Fix up later when we know more */
    nsc->backgroundsplines = NULL;
    nsc->backimages = NULL;
    nsc->undoes[0] = nsc->undoes[1] = nsc->redoes[0] = nsc->redoes[1] = NULL;
    nsc->kerns = NULL;
    if ( nsc->lig!=NULL ) {
	nsc->lig = galloc(sizeof(Ligature));
	nsc->lig->lig = nsc;
	nsc->lig->components = copy(sc->lig->components);
    }
return(nsc);
}

BDFChar *BDFCharCopy(BDFChar *bc) {
    BDFChar *nbc = galloc(sizeof( BDFChar ));

    *nbc = *bc;
    nbc->views = NULL;
    nbc->selection = NULL;
    nbc->undoes = NULL;
    nbc->redoes = NULL;
    nbc->bitmap = galloc((nbc->ymax-nbc->ymin+1)*nbc->bytes_per_line);
    memcpy(nbc->bitmap,bc->bitmap,(nbc->ymax-nbc->ymin+1)*nbc->bytes_per_line);
return( nbc );
}

void BitmapsCopy(SplineFont *to, SplineFont *from, int to_index, int from_index ) {
    BDFFont *f_bdf, *t_bdf;

    /* We assume that the bitmaps are ordered. They should be */
    for ( t_bdf=to->bitmaps, f_bdf=from->bitmaps; t_bdf!=NULL && f_bdf!=NULL; ) {
	if ( t_bdf->pixelsize == f_bdf->pixelsize ) {
	    if ( f_bdf->chars[from_index]!=NULL ) {
		BDFCharFree(t_bdf->chars[to_index]);
		t_bdf->chars[to_index] = BDFCharCopy(f_bdf->chars[from_index]);
		t_bdf->chars[to_index]->sc = to->chars[to_index];
		t_bdf->chars[to_index]->enc = to_index;
	    }
	    t_bdf = t_bdf->next;
	    f_bdf = f_bdf->next;
	} else if ( t_bdf->pixelsize < f_bdf->pixelsize )
	    t_bdf = t_bdf->next;
	else
	    f_bdf = f_bdf->next;
    }
}

static int _SFFindChar(SplineFont *sf, int unienc, char *name ) {
    int index;

    if ( (sf->encoding_name==em_unicode || sf->encoding_name==em_unicode4) &&
	    unienc!=-1 ) {
	index = unienc;
	if ( index>=sf->charcnt || sf->chars[index]==NULL )
	    index = -1;
    } else if ( (sf->encoding_name>=em_unicodeplanes && sf->encoding_name<=em_unicodeplanesmax) &&
	    unienc!=-1 ) {
	index = unienc-((sf->encoding_name-em_unicodeplanes)<<16);
	if ( index<0 || index>=sf->charcnt || sf->chars[index]==NULL )
	    index = -1;
    } else if ( unienc!=-1 ) {
	if ( unienc<sf->charcnt && sf->chars[unienc]!=NULL &&
		sf->chars[unienc]->unicodeenc==unienc )
	    index = unienc;
	else for ( index = sf->charcnt-1; index>=0; --index ) if ( sf->chars[index]!=NULL ) {
	    if ( sf->chars[index]->unicodeenc==unienc )
	break;
	}
    } else {
	for ( index = sf->charcnt-1; index>=0; --index ) if ( sf->chars[index]!=NULL ) {
	    if ( strcmp(sf->chars[index]->name,name)==0 )
	break;
	}
    }
return( index );
}

int SFFindChar(SplineFont *sf, int unienc, char *name ) {
    int index;

    if ( (sf->encoding_name==em_unicode || sf->encoding_name==em_unicode4) &&
	    unienc!=-1 ) {
	index = unienc;
	if ( index>=sf->charcnt )
	    index = -1;
    } else if ( (sf->encoding_name>=em_unicodeplanes && sf->encoding_name<=em_unicodeplanesmax) &&
	    unienc!=-1 ) {
	index = unienc-((sf->encoding_name-em_unicodeplanes)<<16);
	if ( index<0 || index>=sf->charcnt || sf->chars[index]==NULL )
	    index = -1;
    } else if ( unienc!=-1 ) {
	if ( unienc<sf->charcnt && sf->chars[unienc]!=NULL &&
		sf->chars[unienc]->unicodeenc==unienc )
	    index = unienc;
	else for ( index = sf->charcnt-1; index>=0; --index ) if ( sf->chars[index]!=NULL ) {
	    if ( sf->chars[index]->unicodeenc==unienc )
	break;
	}
    } else {
	for ( index = sf->charcnt-1; index>=0; --index ) if ( sf->chars[index]!=NULL ) {
	    if ( strcmp(sf->chars[index]->name,name)==0 )
	break;
	}
	if ( index==-1 ) {
	    for ( index=psunicodenames_cnt-1; index>=0; --index )
		if ( psunicodenames[index]!=NULL &&
			strcmp(psunicodenames[index],name)==0 )
return( SFFindChar(sf,index,name));
	}
    }

 /* Ok. The character is not in the font, but that might be because the font */
 /*  has a hole. check to see if it is in the encoding */
    if ( index==-1 && unienc>=0 && unienc<=65535 && sf->encoding_name!=em_none ) {
	if ( sf->encoding_name>=em_base ) {
	    Encoding *item=NULL;
	    for ( item=enclist; item!=NULL && item->enc_num!=sf->encoding_name; item=item->next );
	    if ( item!=NULL ) {
		for ( index=item->char_cnt-1; index>=0; --index )
		    if ( item->unicode[index]==unienc )
		break;
	    }
	} else if ( sf->encoding_name==em_adobestandard ) {
	    for ( index=255; index>=0; --index )
		if ( unicode_from_adobestd[index]==unienc )
	    break;
	} else if ( sf->encoding_name<=em_first2byte ) {
	    unichar_t * table = unicode_from_alphabets[sf->encoding_name+3];
	    for ( index=255; index>=0; --index )
		if ( table[index]==unienc )
	    break;
	} else {
	    struct charmap2 *table2 = NULL;
	    unsigned short *plane2;
	    int highch, temp;

	    if ( sf->encoding_name==em_jis208 || sf->encoding_name==em_jis212 ||
		    sf->encoding_name==em_sjis )
		table2 = &jis_from_unicode;
	    else if ( sf->encoding_name==em_gb2312 )
		table2 = &gb2312_from_unicode;
	    else if ( sf->encoding_name==em_ksc5601 || sf->encoding_name==em_wansung )
		table2 = &ksc5601_from_unicode;
	    else if ( sf->encoding_name==em_big5 )
		table2 = &big5_from_unicode;
	    else if ( sf->encoding_name==em_johab )
		table2 = &johab_from_unicode;
	    if ( table2!=NULL ) {
		highch = unienc>>8;
		if ( highch>=table2->first && highch<=table2->last &&
			(plane2 = table2->table[highch-table2->first])!=NULL &&
			(temp=plane2[unienc&0xff])!=0 ) {
		    index = temp;
		    if ( sf->encoding_name==em_jis212 ) {
			if ( !(index&0x8000 ) )
			    index=-1;
			else
			    index &= 0x8000;
		    } else if ( (sf->encoding_name==em_jis208 || sf->encoding_name==em_sjis ) &&
			    (index&0x8000) )
			index = -1;
		} else if ( unienc<0x80 &&
			(sf->encoding_name==em_big5 || sf->encoding_name==em_johab ||
			 sf->encoding_name==em_sjis || sf->encoding_name==em_wansung ))
		    index = unienc;
		else if ( sf->encoding_name==em_sjis && unienc>=0xFF61 &&
			unienc<=0xFF9F )
		    index = unienc-0xFF61 + 0xa1;	/* katakana */
		if ( index>0x100 &&
			(sf->encoding_name==em_jis208 || sf->encoding_name==em_jis212 ||
			 sf->encoding_name==em_ksc5601 || sf->encoding_name==em_gb2312 )) {
		    index -= 0x2121;
		    index = (index>>8)*96 + (index&0xff)+1;
		} else if ( index>0x100 && sf->encoding_name==em_sjis ) {
		    int j1 = index>>8, j2 = index&0xff;
		    int ro = j1<95 ? 112 : 176;
		    int co = (j1&1) ? (j2>95?32:31) : 126;
		    index = ( (((j1+1)>>1) + ro )<<8 ) | (j2+co);
		}
	    }
	}
    }
return( index );
}

int SFCIDFindChar(SplineFont *sf, int unienc, char *name ) {
    int j,ret;

    if ( sf->subfonts==NULL && sf->cidmaster==NULL )
return( _SFFindChar(sf,unienc,name));
    if ( sf->cidmaster!=NULL )
	sf=sf->cidmaster;
    for ( j=0; j<sf->subfontcnt; ++j )
	if (( ret = _SFFindChar(sf->subfonts[j],unienc,name))!=-1 )
return( ret );
return( -1 );
}

int SFHasCID(SplineFont *sf,int cid) {
    int i;
    /* What subfont (if any) contains this cid? */
    if ( sf->cidmaster!=NULL )
	sf=sf->cidmaster;
    for ( i=0; i<sf->subfontcnt; ++i )
	if ( cid<sf->subfonts[i]->charcnt && sf->subfonts[i]->chars[cid]!=NULL )
return( i );

return( -1 );
}

SplineChar *SFGetChar(SplineFont *sf, int unienc, char *name ) {
    int ind = SFFindChar(sf,unienc,name);

    if ( ind==-1 )
return( NULL );

return( sf->chars[ind]);
}

int SFFindExistingChar(SplineFont *sf, int unienc, char *name ) {
    int i = _SFFindChar(sf,unienc,name);

    if ( i==-1 || sf->chars[i]==NULL )
return( -1 );
    if ( sf->chars[i]->splines!=NULL || sf->chars[i]->refs!=NULL ||
	    sf->chars[i]->widthset )
return( i );

return( -1 );
}

int SFCIDFindExistingChar(SplineFont *sf, int unienc, char *name ) {
    int j,ret;

    if ( sf->subfonts==NULL && sf->cidmaster==NULL )
return( SFFindExistingChar(sf,unienc,name));
    if ( sf->cidmaster!=NULL )
	sf=sf->cidmaster;
    for ( j=0; j<sf->subfontcnt; ++j )
	if (( ret = _SFFindChar(sf->subfonts[j],unienc,name))!=-1 &&
		sf->subfonts[j]->chars[ret]!=NULL )
return( ret );
return( -1 );
}

static void MFixupSC(SplineFont *sf, SplineChar *sc,int i) {
    RefChar *ref;

    sc->enc = i;
    sc->parent = sf;
    for ( ref=sc->refs; ref!=NULL; ref=ref->next ) {
	/* The sc in the ref is from the old font. It's got to be in the */
	/*  new font too (was either already there or just got copied) */
	ref->local_enc =  _SFFindChar(sf,ref->sc->unicodeenc,ref->sc->name);
	ref->sc = sf->chars[ref->local_enc];
	if ( ref->sc->enc==-2 )
	    MFixupSC(sf,ref->sc,ref->local_enc);
	SCReinstanciateRefChar(sc,ref);
	SCMakeDependent(sc,ref->sc);
    }
    /* I shan't automagically generate bitmaps for any bdf fonts */
    /*  but I have copied over the ones which match */
    sf->fv->filled->chars[i] = SplineCharRasterize(sc,sf->fv->filled->pixelsize);
}

static void MergeFixupRefChars(SplineFont *sf) {
    int i;

    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->enc==-2 ) {
	MFixupSC(sf,sf->chars[i], i);
    }
}

static int SFHasChar(SplineFont *sf, int unienc, char *name ) {
    int index;

    index = _SFFindChar(sf,unienc,name);
    if ( index>=0 && index<sf->charcnt && sf->chars[index]!=NULL ) {
	if ( sf->chars[index]->refs!=NULL ||
		sf->chars[index]->splines!=NULL ||
		sf->chars[index]->width!=sf->ascent+sf->descent ||
		sf->chars[index]->widthset ||
		sf->chars[index]->backimages != NULL ||
		sf->chars[index]->backgroundsplines != NULL )
return( true );
    }
return( false );
}

static int SFHasName(SplineFont *sf, char *name ) {
    int index;

    for ( index = sf->charcnt-1; index>=0; --index ) if ( sf->chars[index]!=NULL ) {
	if ( strcmp(sf->chars[index]->name,name)==0 )
    break;
    }
return( index );
}

static int SFEncodingCnt(SplineFont *sf) {
    Encoding *item=NULL;

    if ( sf->encoding_name == em_unicode4 )
return( unicode4_size );
    if ( sf->encoding_name == em_unicode || sf->encoding_name == em_big5 ||
	    sf->encoding_name == em_johab || sf->encoding_name == em_wansung ||
	    sf->encoding_name == em_sjis ||
	    (sf->encoding_name >= em_unicodeplanes && sf->encoding_name <= em_unicodeplanesmax ))
return( 65536 );
    else if ( sf->encoding_name == em_none )
return( sf->charcnt );
    else if ( sf->encoding_name < em_first2byte )
return( 256 );
    else if ( sf->encoding_name <= em_last94x94 )
return( 96*94 );
    else if ( sf->encoding_name >= em_base ) {
	for ( item=enclist; item!=NULL && item->enc_num!=sf->encoding_name;
		item=item->next );
	if ( item!=NULL )
return( item->char_cnt );
    }
return( sf->charcnt );
}

void MergeFont(FontView *fv,SplineFont *other) {
    int i,cnt, doit, emptypos, index, enc_cnt;
    BDFFont *bdf;

    if ( fv->sf==other ) {
	GWidgetErrorR(_STR_MergingProb,_STR_MergingFontSelf);
return;
    }

    enc_cnt = SFEncodingCnt(fv->sf);
    for ( i=fv->sf->charcnt-1; i>=enc_cnt && fv->sf->chars[i]==NULL; --i );
    emptypos = i+1;

    for ( doit=0; doit<2; ++doit ) {
	cnt = 0;
	for ( i=0; i<other->charcnt; ++i ) if ( other->chars[i]!=NULL ) {
	    if ( other->chars[i]->splines==NULL && other->chars[i]->refs==NULL &&
		    !other->chars[i]->widthset )
		/* Don't bother to copy it */;
	    else if ( !SFHasChar(fv->sf,other->chars[i]->unicodeenc,other->chars[i]->name)) {
		if ( other->chars[i]->unicodeenc==-1 ) {
		    if ( (index=SFHasName(fv->sf,other->chars[i]->name))== -1 )
			index = emptypos+cnt++;
		} else if ( (index = SFFindChar(fv->sf,other->chars[i]->unicodeenc,NULL))==-1 )
		    index = emptypos+cnt++;
		if ( doit ) {
		    /* Bug here. Suppose someone has a reference to our empty */
		    /*  char */
		    SplineCharFree(fv->sf->chars[index]);
		    fv->sf->chars[index] = SplineCharCopy(other->chars[i]);
		    if ( fv->sf->bitmaps!=NULL && other->bitmaps!=NULL )
			BitmapsCopy(fv->sf,other,index,i);
		}
	    }
	}
	if ( !doit ) {
	    if ( emptypos+cnt >= fv->sf->charcnt ) {
		fv->sf->chars = grealloc(fv->sf->chars,(emptypos+cnt)*sizeof(SplineChar *));
		fv->selected = grealloc(fv->selected,emptypos+cnt);
		for ( bdf = fv->sf->bitmaps; bdf!=NULL; bdf=bdf->next )
		    bdf->chars = grealloc(bdf->chars,(emptypos+cnt)*sizeof(SplineChar *));
		fv->filled->chars = grealloc(fv->filled->chars,(emptypos+cnt)*sizeof(SplineChar *));
		for ( i=fv->sf->charcnt; i<emptypos+cnt; ++i ) {
		    fv->sf->chars[i] = NULL;
		    fv->selected[i] = false;
		    for ( bdf = fv->sf->bitmaps; bdf!=NULL; bdf=bdf->next )
			bdf->chars[i] = NULL;
		    fv->filled->chars[i] = NULL;
		}
		fv->sf->charcnt = emptypos+cnt;
		fv->filled->charcnt = emptypos+cnt;
		for ( bdf = fv->sf->bitmaps; bdf!=NULL; bdf=bdf->next )
		    bdf->charcnt = emptypos+cnt;
	    }
	}
    }
    MergeFixupRefChars(fv->sf);
    if ( other->fv==NULL )
	SplineFontFree(other);
    fv->sf->changed = true;
    FontViewReformat(fv);
}

static void MergeAskFilename(FontView *fv) {
    char *filename = GetPostscriptFontName(true);
    SplineFont *sf;
    char *eod, *fpt, *file, *full;

    if ( filename==NULL )
return;
    eod = strrchr(filename,'/');
    *eod = '\0';
    file = eod+1;
    do {
	fpt = strstr(file,"; ");
	if ( fpt!=NULL ) *fpt = '\0';
	full = galloc(strlen(filename)+1+strlen(file)+1);
	strcpy(full,filename); strcat(full,"/"); strcat(full,file);
	sf = LoadSplineFont(full);
	free(full);
	if ( sf==NULL )
	    /* Do Nothing */;
	else if ( sf->fv==fv )
	    GWidgetErrorR(_STR_MergingProb,_STR_MergingFontSelf);
	else {
	    MergeFont(fv,sf);
	    if ( sf->fv==NULL )
		SplineFontFree(sf);
	}
	file = fpt+2;
    } while ( fpt!=NULL );
    free(filename);
}

static GTextInfo *BuildFontList(FontView *except) {
    FontView *fv;
    int cnt=0;
    GTextInfo *tf;

    for ( fv=fv_list; fv!=NULL; fv = fv->next )
	++cnt;
    tf = gcalloc(cnt+3,sizeof(GTextInfo));
    for ( fv=fv_list, cnt=0; fv!=NULL; fv = fv->next ) if ( fv!=except ) {
	tf[cnt].fg = tf[cnt].bg = COLOR_DEFAULT;
	tf[cnt].text = uc_copy(fv->sf->fontname);
	++cnt;
    }
    tf[cnt++].line = true;
    tf[cnt].fg = tf[cnt].bg = COLOR_DEFAULT;
    tf[cnt].text_in_resource = true;
    tf[cnt++].text = (unichar_t *) _STR_Other;
return( tf );
}

static void TFFree(GTextInfo *tf) {
    int i;

    for ( i=0; tf[i].text!=NULL || tf[i].line ; ++i )
	if ( !tf[i].text_in_resource )
	    free( tf[i].text );
    free(tf);
}

struct mf_data {
    int done;
    FontView *fv;
    GGadget *other;
    GGadget *amount;
};

static int MF_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct mf_data *d = GDrawGetUserData(gw);
	int i, index = GGadgetGetFirstListSelectedItem(d->other);
	FontView *fv;
	for ( i=0, fv=fv_list; fv!=NULL; fv=fv->next ) {
	    if ( fv==d->fv )
	continue;
	    if ( i==index )
	break;
	    ++i;
	}
	if ( fv==NULL )
	    MergeAskFilename(d->fv);
	else
	    MergeFont(d->fv,fv->sf);
	d->done = true;
    }
return( true );
}

static int MF_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct mf_data *d = GDrawGetUserData(gw);
	d->done = true;
    }
return( true );
}

static int mv_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct mf_data *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

void FVMergeFonts(FontView *fv) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[6];
    GTextInfo label[6];
    struct mf_data d;
    unichar_t buffer[80];

    /* If there's only one font loaded, then it's the current one, and there's*/
    /*  no point asking the user if s/he wants to merge any of the loaded */
    /*  fonts, go directly to searching the disk */
    if ( fv_list==fv && fv_list->next==NULL )
	MergeAskFilename(fv);
    else {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource(_STR_Mergefonts,NULL);
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,150));
	pos.height = GDrawPointsToPixels(NULL,88);
	gw = GDrawCreateTopWindow(NULL,&pos,mv_e_h,&d,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	u_sprintf( buffer, GStringGetResource(_STR_FontToMergeInto,NULL), fv->sf->fontname );
	label[0].text = buffer;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; 
	gcd[0].gd.flags = gg_visible | gg_enabled;
	gcd[0].creator = GLabelCreate;

	gcd[1].gd.pos.x = 20; gcd[1].gd.pos.y = 21;
	gcd[1].gd.pos.width = 110;
	gcd[1].gd.flags = gg_visible | gg_enabled;
	gcd[1].gd.u.list = BuildFontList(fv);
	gcd[1].gd.label = &gcd[1].gd.u.list[0];
	gcd[1].gd.u.list[0].selected = true;
	gcd[1].creator = GListButtonCreate;

	gcd[2].gd.pos.x = 15-3; gcd[2].gd.pos.y = 55-3;
	gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
	gcd[2].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[2].text = (unichar_t *) _STR_OK;
	label[2].text_in_resource = true;
	gcd[2].gd.mnemonic = 'O';
	gcd[2].gd.label = &label[2];
	gcd[2].gd.handle_controlevent = MF_OK;
	gcd[2].creator = GButtonCreate;

	gcd[3].gd.pos.x = -15; gcd[3].gd.pos.y = 55;
	gcd[3].gd.pos.width = -1; gcd[3].gd.pos.height = 0;
	gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[3].text = (unichar_t *) _STR_Cancel;
	label[3].text_in_resource = true;
	gcd[3].gd.label = &label[3];
	gcd[3].gd.mnemonic = 'C';
	gcd[3].gd.handle_controlevent = MF_Cancel;
	gcd[3].creator = GButtonCreate;

	GGadgetsCreate(gw,gcd);

	memset(&d,'\0',sizeof(d));
	d.other = gcd[1].ret;
	d.fv = fv;

	GWidgetHidePalettes();
	GDrawSetVisible(gw,true);
	while ( !d.done )
	    GDrawProcessOneEvent(NULL);
	GDrawDestroyWindow(gw);
	TFFree(gcd[1].gd.u.list);
    }
}

static RefChar *InterpRefs(RefChar *base, RefChar *other, real amount, SplineChar *sc) {
    RefChar *head=NULL, *last=NULL, *cur;
    RefChar *test;
    int i;

    for ( test = other; test!=NULL; test=test->next )
	test->checked = false;

    while ( base!=NULL ) {
	for ( test = other; test!=NULL; test=test->next ) {
	    if ( test->checked )
		/* Do nothing */;
	    else if ( test->unicode_enc==base->unicode_enc &&
		    (test->unicode_enc!=-1 || strcmp(test->sc->name,base->sc->name)==0 ) )
	break;
	}
	if ( test!=NULL ) {
	    test->checked = true;
	    cur = chunkalloc(sizeof(RefChar));
	    *cur = *base;
	    cur->local_enc = cur->sc->enc;
	    for ( i=0; i<6; ++i )
		cur->transform[i] = base->transform[i] + amount*(other->transform[i]-base->transform[i]);
	    cur->splines = NULL;
	    cur->checked = false;
	    if ( head==NULL )
		head = cur;
	    else
		last->next = cur;
	    last = cur;
	} else
	    fprintf( stderr, "In character %s, could not find reference to %s\n",
		    sc->name, base->sc->name );
	base = base->next;
	if ( test==other && other!=NULL )
	    other = other->next;
    }
return( head );
}

static void InterpPoint(SplineSet *cur, SplinePoint *base, SplinePoint *other, real amount ) {
    SplinePoint *p = chunkalloc(sizeof(SplinePoint));

    p->me.x = base->me.x + amount*(other->me.x-base->me.x);
    p->me.y = base->me.y + amount*(other->me.y-base->me.y);
    p->prevcp.x = base->prevcp.x + amount*(other->prevcp.x-base->prevcp.x);
    p->prevcp.y = base->prevcp.y + amount*(other->prevcp.y-base->prevcp.y);
    p->nextcp.x = base->nextcp.x + amount*(other->nextcp.x-base->nextcp.x);
    p->nextcp.y = base->nextcp.y + amount*(other->nextcp.y-base->nextcp.y);
    p->nonextcp = ( p->nextcp.x==p->me.x && p->nextcp.y==p->me.y );
    p->noprevcp = ( p->prevcp.x==p->me.x && p->prevcp.y==p->me.y );
    p->prevcpdef = base->prevcpdef && other->prevcpdef;
    p->nextcpdef = base->nextcpdef && other->nextcpdef;
    p->selected = false;
    p->pointtype = (base->pointtype==other->pointtype)?base->pointtype:pt_corner;
    /*p->flex = 0;*/
    if ( cur->first==NULL )
	cur->first = p;
    else
	SplineMake(cur->last,p);
    cur->last = p;
}
    
static SplineSet *InterpSplineSet(SplineSet *base, SplineSet *other, real amount, SplineChar *sc) {
    SplineSet *cur = chunkalloc(sizeof(SplineSet));
    SplinePoint *bp, *op;

    for ( bp=base->first, op = other->first; ; ) {
	InterpPoint(cur,bp,op,amount);
	if ( bp->next == NULL && op->next == NULL )
return( cur );
	if ( bp->next!=NULL && op->next!=NULL &&
		bp->next->to==base->first && op->next->to==other->first ) {
	    SplineMake(cur->last,cur->first);
	    cur->last = cur->first;
return( cur );
	}
	if ( bp->next == NULL || bp->next->to==base->first ) {
	    fprintf( stderr, "In character %s, there are too few points on a path in the base\n", sc->name);
	    if ( bp->next!=NULL ) {
		SplineMake(cur->last,cur->first);
		cur->last = cur->first;
	    }
return( cur );
	} else if ( op->next==NULL || op->next->to==other->first ) {
	    fprintf( stderr, "In character %s, there are too many points on a path in the base\n", sc->name);
	    while ( bp->next!=NULL && bp->next->to!=base->first ) {
		bp = bp->next->to;
		InterpPoint(cur,bp,op,amount);
	    }
	    if ( bp->next!=NULL ) {
		SplineMake(cur->last,cur->first);
		cur->last = cur->first;
	    }
return( cur );
	}
	bp = bp->next->to;
	op = op->next->to;
    }
}

static SplineSet *InterpSplineSets(SplineSet *base, SplineSet *other, real amount, SplineChar *sc) {
    SplineSet *head=NULL, *last=NULL, *cur;

    /* we could do something really complex to try and figure out which spline*/
    /*  set goes with which, but I'm not sure that it would really accomplish */
    /*  much, if things are off the computer probably won't figure it out */
    /* Could use path open/closed, direction, point count */
    while ( base!=NULL && other!=NULL ) {
	cur = InterpSplineSet(base,other,amount,sc);
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
	base = base->next;
	other = other->next;
    }
return( head );
}

static void InterpolateChar(SplineFont *new, int enc, SplineChar *base, SplineChar *other, real amount) {
    SplineChar *sc;

    if ( base==NULL || other==NULL )
return;
    sc = chunkalloc(sizeof(SplineChar));
    *sc = *base;
    sc->enc = enc;
    new->chars[enc] = sc;
    sc->views = NULL;
    sc->parent = new;
    sc->changed = true;
    sc->dependents = NULL;
    sc->backgroundsplines = NULL;
    sc->backimages = NULL;
    sc->undoes[0] = sc->undoes[1] = sc->redoes[0] = sc->redoes[1] = NULL;
    sc->kerns = NULL;
    sc->name = copy(sc->name);
    sc->width = base->width + amount*(other->width-base->width);
    sc->lsidebearing = base->lsidebearing + amount*(other->lsidebearing-base->lsidebearing);
    sc->splines = InterpSplineSets(base->splines,other->splines,amount,sc);
    sc->refs = InterpRefs(base->refs,other->refs,amount,sc);
    sc->changedsincelasthinted = true;
}

static void IFixupSC(SplineFont *sf, SplineChar *sc,int i) {
    RefChar *ref;

    for ( ref=sc->refs; ref!=NULL; ref=ref->next ) if ( !ref->checked ) {
	/* The sc in the ref is from the base font. It's got to be in the */
	/*  new font too (the ref only gets created if it's present in both fonts)*/
	ref->checked = true;
	ref->local_enc = _SFFindChar(sf,ref->sc->unicodeenc,ref->sc->name);
	ref->sc = sf->chars[ref->local_enc];
	IFixupSC(sf,ref->sc,ref->local_enc);
	SCReinstanciateRefChar(sc,ref);
	SCMakeDependent(sc,ref->sc);
    }
}

static void InterpFixupRefChars(SplineFont *sf) {
    int i;

    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	IFixupSC(sf,sf->chars[i], i);
    }
}

static void InterpolateFont(SplineFont *base, SplineFont *other, real amount) {
    SplineFont *new;
    int i, index;

    if ( base==other ) {
	GWidgetErrorR(_STR_InterpolatingProb,_STR_InterpolatingFontSelf);
return;
    }
    new = SplineFontBlank(base->encoding_name,base->charcnt);
    new->ascent = base->ascent + amount*(other->ascent-base->ascent);
    new->descent = base->descent + amount*(other->descent-base->descent);
    if ( base->encoding_name==other->encoding_name ) {
	for ( i=0; i<base->charcnt && i<other->charcnt; ++i )
	    InterpolateChar(new,i,base->chars[i],other->chars[i],amount);
    } else {
	for ( i=0; i<base->charcnt; ++i ) if ( base->chars[i]!=NULL ) {
	    index = _SFFindChar(other,base->chars[i]->unicodeenc,base->chars[i]->name);
	    if ( other->chars[index]!=NULL )
		InterpolateChar(new,i,base->chars[i],other->chars[index],amount);
	}
    }
    InterpFixupRefChars(new);
    new->changed = true;
    FontViewCreate(new);
}

static void InterAskFilename(FontView *fv, real amount) {
    char *filename = GetPostscriptFontName(false);
    SplineFont *sf;

    if ( filename==NULL )
return;
    sf = LoadSplineFont(filename);
    free(filename);
    if ( sf==NULL )
return;
    InterpolateFont(fv->sf,sf,amount);
}

#define CID_Amount	1000
static real last_amount=50;

static int IF_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct mf_data *d = GDrawGetUserData(gw);
	int i, index = GGadgetGetFirstListSelectedItem(d->other);
	FontView *fv;
	int err=false;
	real amount;
	
	amount = GetRealR(gw,CID_Amount, _STR_Amount,&err);
	if ( err )
return( true );
	last_amount = amount;
	for ( i=0, fv=fv_list; fv!=NULL; fv=fv->next ) {
	    if ( fv==d->fv )
	continue;
	    if ( i==index )
	break;
	    ++i;
	}
	if ( fv==NULL )
	    InterAskFilename(d->fv,last_amount/100.0);
	else
	    InterpolateFont(d->fv->sf,fv->sf,last_amount/100.0);
	d->done = true;
    }
return( true );
}

void FVInterpolateFonts(FontView *fv) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8];
    GTextInfo label[8];
    struct mf_data d;
    unichar_t buffer[80]; char buf2[30];

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(_STR_Interp,NULL);
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,200));
    pos.height = GDrawPointsToPixels(NULL,118);
    gw = GDrawCreateTopWindow(NULL,&pos,mv_e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    u_sprintf( buffer, GStringGetResource(_STR_InterpBetween,NULL), fv->sf->fontname );
    label[0].text = buffer;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; 
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].creator = GLabelCreate;

    gcd[1].gd.pos.x = 20; gcd[1].gd.pos.y = 21;
    gcd[1].gd.pos.width = 110;
    gcd[1].gd.flags = gg_visible | gg_enabled;
    gcd[1].gd.u.list = BuildFontList(fv);
    if ( gcd[1].gd.u.list[0].text!=NULL ) {
	gcd[1].gd.label = &gcd[1].gd.u.list[0];
	gcd[1].gd.u.list[0].selected = true;
    } else {
	gcd[1].gd.label = &gcd[1].gd.u.list[1];
	gcd[1].gd.u.list[1].selected = true;
	gcd[1].gd.flags = gg_visible;
    }
    gcd[1].creator = GListButtonCreate;

    sprintf( buf2, "%g", last_amount );
    label[2].text = (unichar_t *) buf2;
    label[2].text_is_1byte = true;
    gcd[2].gd.pos.x = 20; gcd[2].gd.pos.y = 51;
    gcd[2].gd.pos.width = 40;
    gcd[2].gd.flags = gg_visible | gg_enabled;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.cid = CID_Amount;
    gcd[2].creator = GTextFieldCreate;

    gcd[3].gd.pos.x = 5; gcd[3].gd.pos.y = 51+6;
    gcd[3].gd.flags = gg_visible | gg_enabled;
    label[3].text = (unichar_t *) _STR_By;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].creator = GLabelCreate;

    gcd[4].gd.pos.x = 20+40+3; gcd[4].gd.pos.y = 51+6;
    gcd[4].gd.flags = gg_visible | gg_enabled;
    label[4].text = (unichar_t *) "%";
    label[4].text_is_1byte = true;
    gcd[4].gd.label = &label[4];
    gcd[4].creator = GLabelCreate;

    gcd[5].gd.pos.x = 15-3; gcd[5].gd.pos.y = 85-3;
    gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[5].text = (unichar_t *) _STR_OK;
    label[5].text_in_resource = true;
    gcd[5].gd.mnemonic = 'O';
    gcd[5].gd.label = &label[5];
    gcd[5].gd.handle_controlevent = IF_OK;
    gcd[5].creator = GButtonCreate;

    gcd[6].gd.pos.x = -15; gcd[6].gd.pos.y = 85;
    gcd[6].gd.pos.width = -1; gcd[6].gd.pos.height = 0;
    gcd[6].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[6].text = (unichar_t *) _STR_Cancel;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.mnemonic = 'C';
    gcd[6].gd.handle_controlevent = MF_Cancel;
    gcd[6].creator = GButtonCreate;

    GGadgetsCreate(gw,gcd);

    memset(&d,'\0',sizeof(d));
    d.other = gcd[1].ret;
    d.fv = fv;

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    TFFree(gcd[1].gd.u.list);
}
