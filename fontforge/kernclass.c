/* Copyright (C) 2003,2004 by George Williams */
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
#include "nomen.h"
#include <gkeysym.h>
#include <string.h>
#include <ustring.h>
#include <utype.h>
#include <math.h>

typedef struct kernclassdlg {
    struct kernclasslistdlg *kcld;
    KernClass *orig;
    int first_cnt, second_cnt;
    int16 *offsets;
    GWindow gw, cw, kw;
    GFont *font;
    int fh, as;
    int kernh, kernw;		/* Width of the box containing the kerning val */
    int xstart, ystart;		/* This is where the headers start */
    int xstart2, ystart2;	/* This is where the data start */
    int width, height;
    int canceldrop, sbdrop;
    int offleft, offtop;
    GGadget *hsb, *vsb;
    int isedit, off;
    int st_pos;
    BDFChar *fsc, *ssc;
    int pixelsize;
    int top;
    int downpos, down, within, orig_kern;
} KernClassDlg;

typedef struct kernclasslistdlg {
    SplineFont *sf;
    KernClassDlg *kcd;
    GWindow gw;
    int isv;
} KernClassListDlg;

#define KCL_Width	200
#define KCL_Height	173
#define KC_Width	400
#define KC_Height	400
#define KC_CANCELDROP	32


#define CID_List	1040
#define CID_New		1041
#define CID_Delete	1042
#define CID_Edit	1043

#define CID_SLI		1000
#define CID_R2L		1001
#define CID_IgnBase	1002
#define CID_IgnLig	1003
#define CID_IgnMark	1004

#define CID_ClassList	1007		/* And 1107 for second char */
#define CID_ClassNew	1008
#define CID_ClassDel	1009
#define CID_ClassEdit	1010
#define CID_ClassLabel	1011

#define CID_OK		1012
#define CID_Cancel	1013
#define CID_Group	1014
#define CID_Line1	1015
#define CID_Line2	1016

#define CID_Set		1020
#define CID_Select	1021
#define CID_GlyphList	1022
#define CID_Prev	1023
#define CID_Next	1024
#define CID_Group2	1025

#define CID_First	1030
#define CID_Second	1031
#define CID_KernOffset	1032
#define CID_Prev2	1033
#define CID_Next2	1034
#define CID_Group3	1035

extern int _GScrollBar_Width;

static int  KCD_SBReset(KernClassDlg *);
static void KCD_HShow(KernClassDlg *, int pos);
static void KCD_VShow(KernClassDlg *, int pos);

static GTextInfo **KCSLIArray(SplineFont *sf,int isv) {
    int cnt;
    KernClass *kc, *head = isv ? sf->vkerns : sf->kerns;
    GTextInfo **ti;

    if ( sf->cidmaster!=NULL ) sf=sf->cidmaster;
    else if ( sf->mm!=NULL ) sf = sf->mm->normal;

    for ( kc=head, cnt=0; kc!=NULL; kc=kc->next, ++cnt );
    ti = gcalloc(cnt+1,sizeof(GTextInfo*));
    for ( kc=head, cnt=0; kc!=NULL; kc=kc->next, ++cnt ) {
	ti[cnt] = gcalloc(1,sizeof(GTextInfo));
	ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
	ti[cnt]->text = ScriptLangLine(sf->script_lang[kc->sli]);
    }
    ti[cnt] = gcalloc(1,sizeof(GTextInfo));
return( ti );
}

static GTextInfo *KCSLIList(SplineFont *sf,int isv) {
    int cnt;
    KernClass *kc, *head = isv ? sf->vkerns : sf->kerns;
    GTextInfo *ti;

    if ( sf->cidmaster!=NULL ) sf=sf->cidmaster;
    else if ( sf->mm!=NULL ) sf = sf->mm->normal;

    for ( kc=head, cnt=0; kc!=NULL; kc=kc->next, ++cnt );
    ti = gcalloc(cnt+1,sizeof(GTextInfo));
    for ( kc=head, cnt=0; kc!=NULL; kc=kc->next, ++cnt )
	ti[cnt].text = ScriptLangLine(sf->script_lang[kc->sli]);
return( ti );
}

/* ************************************************************************** */
/* **************************** Edit/Add a Class **************************** */
/* ************************************************************************** */

static int KCD_ToSelection(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_GlyphList));
	SplineFont *sf = kcd->kcld->sf;
	FontView *fv = sf->fv;
	const unichar_t *end;
	int pos, found=-1;
	char *nm;

	GDrawSetVisible(fv->gw,true);
	GDrawRaise(fv->gw);
	memset(fv->selected,0,sf->charcnt);
	while ( *ret ) {
	    end = u_strchr(ret,' ');
	    if ( end==NULL ) end = ret+u_strlen(ret);
	    nm = cu_copybetween(ret,end);
	    for ( ret = end; isspace(*ret); ++ret);
	    if (( pos = SFFindChar(sf,-1,nm))!=-1 ) {
		if ( found==-1 ) found = pos;
		fv->selected[pos] = true;
	    }
	    free(nm);
	}

	if ( found!=-1 )
	    FVScrollToChar(fv,found);
	GDrawRequestExpose(fv->v,NULL,false);
    }
return( true );
}

static int KCD_FromSelection(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	SplineFont *sf = kcd->kcld->sf;
	FontView *fv = sf->fv;
	unichar_t *vals, *pt;
	int i, len, max;
	SplineChar *sc;
    
	for ( i=len=max=0; i<sf->charcnt; ++i ) if ( fv->selected[i]) {
	    sc = SFMakeChar(sf,i);
	    len += strlen(sc->name)+1;
	    if ( fv->selected[i]>max ) max = fv->selected[i];
	}
	pt = vals = galloc((len+1)*sizeof(unichar_t));
	*pt = '\0';
	/* in a class the order of selection is irrelevant */
	for ( i=0; i<sf->charcnt; ++i ) if ( fv->selected[i] ) {
	    uc_strcpy(pt,sf->chars[i]->name);
	    pt += u_strlen(pt);
	    *pt++ = ' ';
	}
	if ( pt>vals ) pt[-1]='\0';
    
	GGadgetSetTitle(GWidgetGetControl(kcd->gw,CID_GlyphList),vals);
    }
return( true );
}

static int KCD_Prev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	GDrawSetVisible(kcd->cw,false);
    }
return( true );
}

static int KCD_Next(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_GlyphList));
	GGadget *list = GWidgetGetControl( kcd->gw, CID_ClassList+kcd->off );
	int i;

	if ( !CCD_NameListCheck(kcd->kcld->sf,ret,true,_STR_BadClass) ||
		CCD_InvalidClassList(ret,list,kcd->isedit))
return( true );

	if ( kcd->isedit ) {
	    GListChangeLine(list,GGadgetGetFirstListSelectedItem(list),ret);
	} else {
	    GListAppendLine(list,ret,false);
	    if ( kcd->off==0 ) {
		kcd->offsets = grealloc(kcd->offsets,(kcd->first_cnt+1)*kcd->second_cnt*sizeof(int16));
		memset(kcd->offsets+kcd->first_cnt*kcd->second_cnt,
			0, kcd->second_cnt*sizeof(int16));
		++kcd->first_cnt;
	    } else {
		int16 *new = galloc(kcd->first_cnt*(kcd->second_cnt+1)*sizeof(int16));
		for ( i=0; i<kcd->first_cnt; ++i ) {
		    memcpy(new+i*(kcd->second_cnt+1),kcd->offsets+i*kcd->second_cnt,
			    kcd->second_cnt*sizeof(int16));
		    new[i*(kcd->second_cnt+1)+kcd->second_cnt] = 0;
		}
		++kcd->second_cnt;
		free( kcd->offsets );
		kcd->offsets = new;
	    }
	    KCD_SBReset(kcd);
	}
	GDrawSetVisible(kcd->cw,false);		/* This will give us an expose so we needed ask for one */
    }
return( true );
}

static void _KCD_DoEditNew(KernClassDlg *kcd,int isedit,int off) {
    static unichar_t nullstr[] = { 0 };

    kcd->isedit = isedit;
    kcd->off = off;
    if ( isedit ) {
	GTextInfo *selected = GGadgetGetListItemSelected(GWidgetGetControl(
		kcd->gw, CID_ClassList+off));
	if ( selected==NULL )
return;
	GGadgetSetTitle(GWidgetGetControl(kcd->cw,CID_GlyphList),selected->text);
    } else {
	GGadgetSetTitle(GWidgetGetControl(kcd->cw,CID_GlyphList),nullstr);
    }
    GDrawSetVisible(kcd->cw,true);
}

/* ************************************************************************** */
/* ************************** Kern Class Display **************************** */
/* ************************************************************************** */

static int KCD_Prev2(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	BDFCharFree(kcd->fsc); BDFCharFree(kcd->ssc);
	kcd->fsc = kcd->ssc = NULL;
	GDrawSetVisible(kcd->kw,false);
    }
return( true );
}

static int KCD_Next2(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_KernOffset));
	unichar_t *end;
	int val = u_strtol(ret,&end,10);

	if ( val<-32768 || val>32767 || *end!='\0' ) {
	    GWidgetErrorR( _STR_BadNumber, _STR_BadNumber );
return( true );
	}
	kcd->offsets[kcd->st_pos] = val;

	BDFCharFree(kcd->fsc); BDFCharFree(kcd->ssc);
	kcd->fsc = kcd->ssc = NULL;
	GDrawSetVisible(kcd->kw,false);		/* This will give us an expose so we needed ask for one */
    }
return( true );
}

static void KCD_DrawGlyph(GWindow pixmap,int x,int baseline,BDFChar *bdfc) {
    struct _GImage base;
    GImage gi;
    GClut clut;

    memset(&gi,'\0',sizeof(gi));
    memset(&base,'\0',sizeof(base));
    memset(&clut,'\0',sizeof(clut));
    gi.u.image = &base;
    base.clut = &clut;
    if ( !bdfc->byte_data ) {
	base.image_type = it_mono;
	clut.clut_len = 2;
	clut.clut[0] = GDrawGetDefaultBackground(NULL);
	clut.clut[1] = 0x808080;
    } else {
	int scale = 4, l;
	Color fg, bg;
	base.image_type = it_index;
	clut.clut_len = 1<<scale;
	bg = GDrawGetDefaultBackground(NULL);
	fg = 0x808080;
	for ( l=0; l<(1<<scale); ++l )
	    clut.clut[l] =
		COLOR_CREATE(
		 COLOR_RED(bg) + (l*(COLOR_RED(fg)-COLOR_RED(bg)))/((1<<scale)-1),
		 COLOR_GREEN(bg) + (l*(COLOR_GREEN(fg)-COLOR_GREEN(bg)))/((1<<scale)-1),
		 COLOR_BLUE(bg) + (l*(COLOR_BLUE(fg)-COLOR_BLUE(bg)))/((1<<scale)-1) );
    }
    base.data = bdfc->bitmap;
    base.bytes_per_line = bdfc->bytes_per_line;
    base.width = bdfc->xmax-bdfc->xmin+1;
    base.height = bdfc->ymax-bdfc->ymin+1;
    GDrawDrawImage(pixmap,&gi,NULL,x,baseline-bdfc->ymax);
}

static void KCD_KernMouse(KernClassDlg *kcd,GEvent *event) {
    int x, y, width;
    char buf[20];
    unichar_t ubuf[20];
    int kern, pkern;
    double scale;

    scale = kcd->pixelsize/(double) (kcd->kcld->sf->ascent+kcd->kcld->sf->descent);
    kern = u_strtol(_GGadgetGetTitle(GWidgetGetControl(kcd->kw,CID_KernOffset)),NULL,10);
    pkern = rint( kern*scale );

    if ( !kcd->kcld->isv ) {
	/* Horizontal */
	width = (kcd->fsc!=NULL?kcd->fsc->width:0)+(kcd->ssc!=NULL?kcd->ssc->width:0)+pkern + 20;
	x = (kcd->width - width)/2;

	if ( GGadgetIsChecked(GWidgetGetControl(kcd->gw,CID_R2L)) ) {
	    if ( kcd->ssc!=NULL )
		width -= kcd->ssc->width;
	} else {
	    if ( kcd->fsc!=NULL ) {
		x += kcd->fsc->width + pkern;
		width -= kcd->fsc->width + pkern;
	    }
	}

	if ( event->u.mouse.y<kcd->top || event->u.mouse.y>kcd->top+2*kcd->pixelsize ||
		event->u.mouse.x<x || event->u.mouse.x>x+width ) {
	    if ( event->type == et_mousedown )
return;
	    if ( kcd->within ) {
		GDrawSetCursor(kcd->kw,ct_pointer);
		if ( kcd->down && kcd->orig_kern!=kern ) {
		    sprintf(buf, "%d", kcd->orig_kern);
		    uc_strcpy(ubuf,buf);
		    GGadgetSetTitle(GWidgetGetControl(kcd->kw,CID_KernOffset),ubuf);
		    GDrawRequestExpose(kcd->kw,NULL,false);
		}
		kcd->within = false;
	    }
	    if ( event->type==et_mouseup )
		kcd->down = false;
return;
	}

	if ( !kcd->within ) {
	    GDrawSetCursor(kcd->kw,ct_leftright);
	    kcd->within = true;
	}
	if ( event->type == et_mousedown ) {
	    kcd->orig_kern = kern;
	    kcd->down = true;
	    kcd->downpos = event->u.mouse.x;
	} else if ( kcd->down ) {
	    /* I multiply by 2 here because I center the glyphs, so the kerning */
	    /*  changes in both directions */
	    int nkern = kcd->orig_kern + rint(2*(event->u.mouse.x-kcd->downpos)/scale);
	    if ( kern!=nkern ) {
		sprintf(buf, "%d", nkern);
		uc_strcpy(ubuf,buf);
		GGadgetSetTitle(GWidgetGetControl(kcd->kw,CID_KernOffset),ubuf);
		GDrawRequestExpose(kcd->kw,NULL,false);
	    }
	    if ( event->type==et_mouseup )
		kcd->down = false;
	}
    } else {
	/* Vertical */
	y = kcd->top + kcd->pixelsize/3;
	width = (kcd->ssc!=NULL ? rint(kcd->ssc->sc->vwidth * scale) : 0);
	if ( kcd->fsc!=NULL )
	    y += rint(kcd->fsc->sc->vwidth * scale) + pkern;
	x = kcd->width/2 - kcd->pixelsize/2;

	if ( event->u.mouse.y<y || event->u.mouse.y>y+width ||
		event->u.mouse.x<x || event->u.mouse.x>x+kcd->pixelsize ) {
	    if ( event->type == et_mousedown )
return;
	    if ( kcd->within ) {
		GDrawSetCursor(kcd->kw,ct_pointer);
		if ( kcd->down && kcd->orig_kern!=kern ) {
		    sprintf(buf, "%d", kcd->orig_kern);
		    uc_strcpy(ubuf,buf);
		    GGadgetSetTitle(GWidgetGetControl(kcd->kw,CID_KernOffset),ubuf);
		    GDrawRequestExpose(kcd->kw,NULL,false);
		}
		kcd->within = false;
	    }
	    if ( event->type==et_mouseup )
		kcd->down = false;
return;
	}

	if ( !kcd->within ) {
	    GDrawSetCursor(kcd->kw,ct_updown);
	    kcd->within = true;
	}
	if ( event->type == et_mousedown ) {
	    kcd->orig_kern = kern;
	    kcd->down = true;
	    kcd->downpos = event->u.mouse.y;
	} else if ( kcd->down ) {
	    int nkern = kcd->orig_kern + rint((event->u.mouse.y-kcd->downpos)/scale);
	    if ( kern!=nkern ) {
		sprintf(buf, "%d", nkern);
		uc_strcpy(ubuf,buf);
		GGadgetSetTitle(GWidgetGetControl(kcd->kw,CID_KernOffset),ubuf);
		GDrawRequestExpose(kcd->kw,NULL,false);
	    }
	    if ( event->type==et_mouseup )
		kcd->down = false;
	}
    }
}

static void KCD_KernExpose(KernClassDlg *kcd,GWindow pixmap,GEvent *event) {
    GRect *area = &event->u.expose.rect;
    GRect rect;
    GRect old1;
    int x, y;
    SplineFont *sf = kcd->kcld->sf;
    int em = sf->ascent+sf->descent;
    int as = rint(sf->ascent*kcd->pixelsize/(double) em);
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_KernOffset));
    int kern = u_strtol(ret,NULL,10);
    int baseline, xbaseline;

    if ( area->y+area->height<kcd->top )
return;
    if ( area->y>kcd->top+3*kcd->pixelsize ||
	    (!kcd->kcld->isv && area->y>kcd->top+2*kcd->pixelsize ))
return;

    rect.x = 0; rect.y = kcd->top;
    rect.width = kcd->width; /* close enough */
    rect.height = kcd->kcld->isv ? 3*kcd->pixelsize : 2*kcd->pixelsize;
    GDrawPushClip(pixmap,&rect,&old1);

    kern = rint(kern*kcd->pixelsize/(double) em);

    if ( !kcd->kcld->isv ) {
	x = (kcd->width-( (kcd->fsc!=NULL?kcd->fsc->width:0)+(kcd->ssc!=NULL?kcd->ssc->width:0)+kern))/2;
	baseline = kcd->top + as + kcd->pixelsize/2;
	if ( GGadgetIsChecked(GWidgetGetControl(kcd->gw,CID_R2L)) ) {
	    if ( kcd->ssc!=NULL ) {
		KCD_DrawGlyph(pixmap,x,baseline,kcd->ssc);
		x += kcd->ssc->width + kern;
	    }
	    if ( kcd->fsc!=NULL )
		KCD_DrawGlyph(pixmap,x,baseline,kcd->fsc);
	} else {
	    if ( kcd->fsc!=NULL ) {
		KCD_DrawGlyph(pixmap,x,baseline,kcd->fsc);
		x += kcd->fsc->width + kern;
	    }
	    if ( kcd->ssc!=NULL )
		KCD_DrawGlyph(pixmap,x,baseline,kcd->ssc);
	}
    } else {
	/* I don't support top to bottom vertical */
	y = kcd->top + kcd->pixelsize/3 + as;
	xbaseline = kcd->width/2;
	if ( kcd->fsc!=NULL ) {
	    KCD_DrawGlyph(pixmap,xbaseline-kcd->pixelsize/2,y,kcd->fsc);
	    y += rint(kcd->fsc->sc->vwidth * kcd->pixelsize/(double) em) + kern;
	}
	if ( kcd->ssc!=NULL )
	    KCD_DrawGlyph(pixmap,xbaseline-kcd->pixelsize/2,y,kcd->ssc);
    }
    GDrawPopClip(pixmap,&old1);
}

static int KCD_KernOffChanged(GGadget *g, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged )
	GDrawRequestExpose(kcd->kw,NULL,false);
return( true );
}

static void KCD_UpdateGlyph(KernClassDlg *kcd,int which) {
    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(kcd->kw,
	    which==0 ? CID_First : CID_Second ));
    BDFChar **scpos = which==0 ? &kcd->fsc : &kcd->ssc;
    SplineChar *sc;
    char *temp;

    BDFCharFree(*scpos);
    *scpos = NULL;
    if ( sel==NULL )
return;
    temp = cu_copy(sel->text);
    sc = SFGetCharDup(kcd->kcld->sf,-1,temp);
    free(temp);
    if ( sc==NULL )
return;
    *scpos = SplineCharAntiAlias(sc,kcd->pixelsize,4);
}

static int KCD_GlyphSelected(GGadget *g, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    int which = GGadgetGetCid(g)==CID_Second;
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	KCD_UpdateGlyph(kcd,which);
	GDrawRequestExpose(kcd->kw,NULL,false);
    }
return( true );
}

static GTextInfo **TiNamesFromClass(GGadget *list,int class_index) {
    /* Return a list containing all the names in this class */
    unichar_t *upt, *end;
    unichar_t *line;
    GTextInfo **ti;
    int i, k;

    if ( class_index==0 || GGadgetGetListItem(list,class_index)==NULL ) {
	i=0;
	ti = galloc((i+1)*sizeof(GTextInfo*));
    } else {
	line = GGadgetGetListItem(list,class_index)->text;
	for ( k=0 ; k<2; ++k ) {
	    for ( i=0, upt=line; *upt; ) {
		while ( *upt==' ' ) ++upt;
		if ( *upt=='\0' )
	    break;
		for ( end = upt; *end!='\0' && *end!=' '; ++end );
		if ( k==1 ) {
		    ti[i] = gcalloc(1,sizeof(GTextInfo));
		    ti[i]->text = u_copyn(upt,end-upt);
		    ti[i]->bg = ti[i]->fg = COLOR_DEFAULT;
		}
		++i;
		upt = end;
	    }
	    if ( k==0 )
		ti = galloc((i+1)*sizeof(GTextInfo*));
	}
    }
    if ( i>0 )
	ti[0]->selected = true;
    ti[i] = gcalloc(1,sizeof(GTextInfo));
return( ti );
}

static void KCD_EditOffset(KernClassDlg *kcd) {
    int first = kcd->st_pos/kcd->second_cnt, second = kcd->st_pos%kcd->second_cnt;
    char buf[12];
    unichar_t ubuf[12];
    GTextInfo **ti;
    static unichar_t nullstr[] = { 0 };

    if ( first==0 || second==0 )
	GWidgetPostNoticeR(_STR_ClassZero,_STR_ClassZeroOffsets);
    GGadgetSetList(GWidgetGetControl(kcd->kw,CID_First),
	    ti = TiNamesFromClass(GWidgetGetControl(kcd->gw,CID_ClassList),first),false);
    GGadgetSetTitle(GWidgetGetControl(kcd->kw,CID_First),
	    ti==NULL || ti[0]->text==NULL ? nullstr: ti[0]->text);
    GGadgetSetList(GWidgetGetControl(kcd->kw,CID_Second),
	    ti = TiNamesFromClass(GWidgetGetControl(kcd->gw,CID_ClassList+100),second),false);
    GGadgetSetTitle(GWidgetGetControl(kcd->kw,CID_Second),
	    ti==NULL || ti[0]->text==NULL ? nullstr: ti[0]->text);
    KCD_UpdateGlyph(kcd,0);
    KCD_UpdateGlyph(kcd,1);

    sprintf( buf, "%d", kcd->offsets[kcd->st_pos]);
    uc_strcpy(ubuf,buf);
    GGadgetSetTitle(GWidgetGetControl(kcd->gw,CID_KernOffset),ubuf);

    GDrawSetVisible(kcd->kw,true);
}

/* ************************************************************************** */
/* *************************** Kern Class Dialog **************************** */
/* ************************************************************************** */

static int KC_Sli(GGadget *g, GEvent *e) {
    KernClassDlg *kcd;
    int sli;
    SplineFont *sf = kcd->kcld->sf;

    if ( sf->mm!=NULL ) sf = sf->mm->normal;
    else if ( sf->cidmaster!=NULL ) sf=sf->cidmaster;

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	kcd = GDrawGetUserData(GGadgetGetWindow(g));
	sli = GGadgetGetFirstListSelectedItem(g);
	if ( sf->script_lang==NULL ||
		sf->script_lang[sli]==NULL )
	    ScriptLangList(sf,g,kcd->orig!=NULL?kcd->orig->sli:0);
    }
return( true );
}

static int KC_OK(GGadget *g, GEvent *e) {
    static int flag_cid[] = { CID_R2L, CID_IgnBase, CID_IgnLig, CID_IgnMark, 0 };
    static int flag_bits[] = { pst_r2l, pst_ignorebaseglyphs, pst_ignoreligatures, pst_ignorecombiningmarks };
    SplineFont *sf;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	KernClassListDlg *kcld = kcd->kcld;
	KernClass *kc;
	int i;
	int sli = GGadgetGetFirstListSelectedItem(GWidgetGetControl(kcd->gw,CID_SLI));
	int32 len;
	GTextInfo **ti;

	sf = kcd->kcld->sf;
	if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
	else if ( sf->mm!=NULL ) sf = sf->mm->normal;

	if ( GDrawIsVisible(kcd->cw))
return( KCD_Next(g,e));
	else if ( GDrawIsVisible(kcd->kw))
return( KCD_Next2(g,e));

	if ( sf->script_lang==NULL || sli<0 ||
		sf->script_lang[sli]==NULL ) {
	    GWidgetErrorR(_STR_SelectAScript,_STR_SelectAScript);
return( true );
	}
	if ( kcd->orig==NULL ) {
	    kc = chunkalloc(sizeof(KernClass));
	    if ( kcld->isv ) {
		kc->next = kcld->sf->vkerns;
		kcld->sf->vkerns = kc;
	    } else {
		kc->next = kcld->sf->kerns;
		kcld->sf->kerns = kc;
	    }
	} else {
	    kc = kcd->orig;
	    for ( i=1; i<kc->first_cnt; ++i )
		free( kc->firsts[i]);
	    for ( i=1; i<kc->second_cnt; ++i )
		free( kc->seconds[i]);
	    free(kc->firsts);
	    free(kc->seconds);
	    free(kc->offsets);
	}
	kc->first_cnt = kcd->first_cnt;
	kc->second_cnt = kcd->second_cnt;
	kc->firsts = galloc(kc->first_cnt*sizeof(char *));
	kc->seconds = galloc(kc->second_cnt*sizeof(char *));
	kc->firsts[0] = kc->seconds[0] = NULL;
	ti = GGadgetGetList(GWidgetGetControl(kcd->gw,CID_ClassList),&len);
	for ( i=1; i<kc->first_cnt; ++i )
	    kc->firsts[i] = cu_copy(ti[i]->text);
	ti = GGadgetGetList(GWidgetGetControl(kcd->gw,CID_ClassList+100),&len);
	for ( i=1; i<kc->second_cnt; ++i )
	    kc->seconds[i] = cu_copy(ti[i]->text);
	kc->offsets = kcd->offsets;
	kc->sli = sli;
	kc->flags = 0;
	for ( i=0; flag_cid[i]!=0; ++i )
	    if ( GGadgetIsChecked(GWidgetGetControl(kcd->gw,flag_cid[i])))
		kc->flags |= flag_bits[i];
	kcd->kcld->sf->changed = true;
	sf->changed = true;

	GDrawDestroyWindow(kcd->gw);
    }
return( true );
}

static void KC_DoCancel(KernClassDlg *kcd) {
    free(kcd->offsets);
    GDrawDestroyWindow(kcd->gw);
}

static int KC_Cancel(GGadget *g, GEvent *e) {
    KernClassDlg *kcd;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	kcd = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GDrawIsVisible(kcd->cw))
return( KCD_Prev(g,e));
	else if ( GDrawIsVisible(kcd->kw))
return( KCD_Prev2(g,e));

	KC_DoCancel(kcd);
    }
return( true );
}

static void _KCD_EnableButtons(KernClassDlg *kcd,int off) {
    GGadget *list = GWidgetGetControl(kcd->gw,CID_ClassList+off);
    int32 i, len, j;
    GTextInfo **ti;

    ti = GGadgetGetList(list,&len);
    i = GGadgetGetFirstListSelectedItem(list);
    GGadgetSetEnabled(GWidgetGetControl(kcd->gw,CID_ClassDel+off),i>=1);
    for ( j=i+1; j<len; ++j )
	if ( ti[j]->selected )
    break;
    GGadgetSetEnabled(GWidgetGetControl(kcd->gw,CID_ClassEdit+off),i>=1 && j==len);
}

static void OffsetRemoveClasses(KernClassDlg *kcd, GTextInfo **removethese, int off ) {
    int16 *new;
    int i,j,k, remove_cnt;
    int old_cnt = off==0 ? kcd->first_cnt : kcd->second_cnt;

    removethese[0]->selected = false;
    for ( remove_cnt=i=0; i<old_cnt; ++i )
	if ( removethese[i]->selected )
	    ++remove_cnt;
    if ( remove_cnt==0 )
return;

    if ( off==0 ) {
	new = galloc((kcd->first_cnt-remove_cnt)*kcd->second_cnt*sizeof(int16));
	for ( i=0; i<kcd->second_cnt; ++i ) {
	    for ( j=k=0; j<kcd->first_cnt; ++j ) {
		if ( !removethese[j]->selected )
		    new[i*(kcd->first_cnt-remove_cnt)+k++] = kcd->offsets[i*kcd->first_cnt+j];
	    }
	}
	kcd->first_cnt -= remove_cnt;
    } else {
	new = galloc(kcd->first_cnt*(kcd->second_cnt-remove_cnt)*sizeof(int16));
	for ( i=k=0; i<kcd->second_cnt; ++i ) {
	    if ( !removethese[i]->selected )
		memcpy(new+(k++)*kcd->first_cnt,kcd->offsets+i*kcd->first_cnt,
			kcd->first_cnt*sizeof(int16));
	}
	kcd->second_cnt -= remove_cnt;
    }

    free(kcd->offsets);
    kcd->offsets = new;
}

static int KCD_Delete(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_ClassEdit;
	GGadget *list = GWidgetGetControl(kcd->gw,CID_ClassList+off);
	int32 len;

	OffsetRemoveClasses(kcd,GGadgetGetList(list,&len),off);
	GListDelSelected(list);
	_KCD_EnableButtons(kcd,off);
	KCD_SBReset(kcd);
	GDrawRequestExpose(kcd->gw,NULL,false);
    }
return( true );
}

static int KCD_Edit(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_ClassEdit;
	if ( GGadgetGetFirstListSelectedItem(GWidgetGetControl(kcd->gw,CID_ClassList+off))>0 )
	    _KCD_DoEditNew(kcd,true,off);
    }
return( true );
}

static int KCD_New(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_ClassNew;
	_KCD_DoEditNew(kcd,false,off);
    }
return( true );
}

static int KCD_ClassSelected(GGadget *g, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    int off = GGadgetGetCid(g)-CID_ClassList;
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	_KCD_EnableButtons(kcd,off);
	if ( off==0 )
	    KCD_VShow(kcd,GGadgetGetFirstListSelectedItem(g));
	else
	    KCD_HShow(kcd,GGadgetGetFirstListSelectedItem(g));
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	if ( GGadgetGetFirstListSelectedItem(g)>0 )
	    _KCD_DoEditNew(kcd,true,off);
    }
return( true );
}

static void KCD_Mouse(KernClassDlg *kcd,GEvent *event) {
    static unichar_t space[200];
    char buf[30];
    int32 len;
    GTextInfo **ti;
    int pos = ((event->u.mouse.y-kcd->ystart2)/kcd->kernh + kcd->offtop) * kcd->second_cnt +
	    (event->u.mouse.x-kcd->xstart2)/kcd->kernw + kcd->offleft;

    GGadgetEndPopup();

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button==4 || event->u.mouse.button==5) ) {
	GGadgetDispatchEvent(kcd->vsb,event);
return;
    }
    
    if ( event->u.mouse.x<kcd->xstart || event->u.mouse.x>kcd->xstart2+kcd->width ||
	    event->u.mouse.y<kcd->ystart || event->u.mouse.y>kcd->ystart2+kcd->height )
return;

    if ( event->type==et_mousemove ) {
	int c = (event->u.mouse.x - kcd->xstart2)/kcd->kernw + kcd->offleft;
	int s = (event->u.mouse.y - kcd->ystart2)/kcd->kernh + kcd->offtop;
	space[0] = '\0';
	if ( event->u.mouse.y>=kcd->ystart2 && s<kcd->first_cnt ) {
	    sprintf( buf, "First Class %d\n", s );
	    uc_strcpy(space,buf);
	    ti = GGadgetGetList(GWidgetGetControl(kcd->gw,CID_ClassList),&len);
	    len = u_strlen(space);
	    u_strncpy(space+len,ti[s]->text,(sizeof(space)/sizeof(space[0]))/2-2 - len);
	    uc_strcat(space,"\n");
	}
	if ( event->u.mouse.x>=kcd->xstart2 && c<kcd->second_cnt ) {
	    sprintf( buf, "Second Class %d\n", c );
	    uc_strcat(space,buf);
	    ti = GGadgetGetList(GWidgetGetControl(kcd->gw,CID_ClassList+100),&len);
	    len = u_strlen(space);
	    u_strncpy(space+len,ti[c]->text,(sizeof(space)/sizeof(space[0]))-1 - len);
	}
	if ( space[0]=='\0' )
return;
	if ( space[u_strlen(space)-1]=='\n' )
	    space[u_strlen(space)-1]='\0';
	GGadgetPreparePopup(kcd->gw,space);
    } else if ( event->u.mouse.x<kcd->xstart2 || event->u.mouse.y<kcd->ystart2 )
return;
    else if ( event->type==et_mousedown )
	kcd->st_pos = pos;
    else if ( event->type==et_mouseup ) {
	if ( pos==kcd->st_pos )
	    KCD_EditOffset(kcd);
    }
}

static void KCD_Expose(KernClassDlg *kcd,GWindow pixmap,GEvent *event) {
    GRect *area = &event->u.expose.rect;
    GRect rect;
    GRect clip,old1,old2;
    int len, off, i, j, x, y;
    unichar_t ubuf[8];
    char buf[100];

    if ( area->y+area->height<kcd->ystart )
return;
    if ( area->y>kcd->ystart2+kcd->height )
return;

    GDrawPushClip(pixmap,area,&old1);
    GDrawSetFont(pixmap,kcd->font);
    GDrawSetLineWidth(pixmap,0);
    rect.x = kcd->xstart; rect.y = kcd->ystart;
    rect.width = kcd->width+(kcd->xstart2-kcd->xstart);
    rect.height = kcd->height+(kcd->ystart2-kcd->ystart);
    clip = rect;
    GDrawPushClip(pixmap,&clip,&old2);
    for ( i=0 ; kcd->offtop+i<=kcd->first_cnt && (i-1)*kcd->kernh<kcd->height; ++i ) {
	GDrawDrawLine(pixmap,kcd->xstart,kcd->ystart2+i*kcd->kernh,kcd->xstart+rect.width,kcd->ystart2+i*kcd->kernh,
		0x808080);
	if ( i+kcd->offtop<kcd->first_cnt ) {
	    sprintf( buf, "%d", i+kcd->offtop );
	    uc_strcpy(ubuf,buf);
	    len = GDrawGetTextWidth(pixmap,ubuf,-1,NULL);
	    off = (kcd->kernh-len*kcd->fh)/2;
	    GDrawDrawText(pixmap,kcd->xstart+(kcd->kernw-len)/2,kcd->ystart2+i*kcd->kernh+kcd->as+1,
		    ubuf,-1,NULL,0xff0000);
	}
    }
    for ( i=0 ; kcd->offleft+i<=kcd->second_cnt && (i-1)*kcd->kernw<kcd->width; ++i ) {
	GDrawDrawLine(pixmap,kcd->xstart2+i*kcd->kernw,kcd->ystart,kcd->xstart2+i*kcd->kernw,kcd->ystart+rect.height,
		0x808080);
	if ( i+kcd->offleft<kcd->second_cnt ) {
	    sprintf( buf, "%d", i+kcd->offleft );
	    uc_strcpy(ubuf,buf);
	    len = GDrawGetTextWidth(pixmap,ubuf,-1,NULL);
	    GDrawDrawText(pixmap,kcd->xstart2+i*kcd->kernw+(kcd->kernw-len)/2,kcd->ystart+kcd->as+1,
		ubuf,-1,NULL,0xff0000);
	}
    }

    for ( i=0 ; kcd->offtop+i<kcd->first_cnt && (i-1)*kcd->kernh<kcd->height; ++i ) {
	y = kcd->ystart2+i*kcd->kernh;
	if ( y>area->y+area->height )
    break;
	if ( y+kcd->kernh<area->y )
    continue;
	for ( j=0 ; kcd->offleft+j<kcd->second_cnt && (j-1)*kcd->kernw<kcd->width; ++j ) {
	    x = kcd->xstart2+j*kcd->kernw;
	    if ( x>area->x+area->width )
	break;
	    if ( x+kcd->kernw<area->x )
	continue;

	    sprintf( buf, "%d", kcd->offsets[(i+kcd->offtop)*kcd->second_cnt+j+kcd->offleft] );
	    uc_strcpy(ubuf,buf);
	    len = GDrawGetTextWidth(pixmap,ubuf,-1,NULL);
	    GDrawDrawText(pixmap,x+kcd->kernw-3-len,y+kcd->as+1,
		ubuf,-1,NULL,0x000000);
	}
    }

    GDrawDrawLine(pixmap,kcd->xstart,kcd->ystart2,kcd->xstart+rect.width,kcd->ystart2,
	    0x000000);
    GDrawDrawLine(pixmap,kcd->xstart2,kcd->ystart,kcd->xstart2,kcd->ystart+rect.height,
	    0x000000);
    GDrawPopClip(pixmap,&old2);
    GDrawPopClip(pixmap,&old1);
    GDrawDrawRect(pixmap,&rect,0x000000);
    rect.y += rect.height;
    rect.x += rect.width;
    LogoExpose(pixmap,event,&rect,dm_fore);
}

static int KCD_SBReset(KernClassDlg *kcd) {
    int oldtop = kcd->offtop, oldleft = kcd->offleft;

    GScrollBarSetBounds(kcd->vsb,0,kcd->first_cnt, kcd->height/kcd->kernh);
    GScrollBarSetBounds(kcd->hsb,0,kcd->second_cnt, kcd->width/kcd->kernw);
    if ( kcd->offtop + (kcd->height/kcd->kernh) >= kcd->first_cnt )
	kcd->offtop = kcd->first_cnt - (kcd->height/kcd->kernh);
    if ( kcd->offtop < 0 ) kcd->offtop = 0;
    if ( kcd->offleft + (kcd->width/kcd->kernw) >= kcd->second_cnt )
	kcd->offleft = kcd->second_cnt - (kcd->width/kcd->kernw);
    if ( kcd->offleft < 0 ) kcd->offleft = 0;
    GScrollBarSetPos(kcd->vsb,kcd->offtop);
    GScrollBarSetPos(kcd->hsb,kcd->offleft);

return( oldtop!=kcd->offtop || oldleft!=kcd->offleft );
}

static void KCD_HShow(KernClassDlg *kcd, int pos) {
    if ( pos<0 || pos>=kcd->second_cnt )
return;
#if 0
    if ( pos>=kcd->offleft && pos<kcd->offleft+(kcd->width/kcd->kernw) )
return;		/* Already visible */
#endif
    --pos;	/* One line of context */
    if ( pos + (kcd->width/kcd->kernw) >= kcd->second_cnt )
	pos = kcd->second_cnt - (kcd->width/kcd->kernw);
    if ( pos < 0 ) pos = 0;
    kcd->offleft = pos;
    GScrollBarSetPos(kcd->hsb,pos);
    GDrawRequestExpose(kcd->gw,NULL,false);
}

static void KCD_HScroll(KernClassDlg *kcd,struct sbevent *sb) {
    int newpos = kcd->offleft;
    GRect rect;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
	if ( kcd->width/kcd->kernw == 1 )
	    --newpos;
	else
	    newpos -= kcd->width/kcd->kernw - 1;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
	if ( kcd->width/kcd->kernw == 1 )
	    ++newpos;
	else
	    newpos += kcd->width/kcd->kernw - 1;
      break;
      case et_sb_bottom:
        newpos = kcd->second_cnt - (kcd->width/kcd->kernw);
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos + (kcd->width/kcd->kernw) >= kcd->second_cnt )
	newpos = kcd->second_cnt - (kcd->width/kcd->kernw);
    if ( newpos < 0 ) newpos = 0;
    if ( newpos!=kcd->offleft ) {
	int diff = newpos-kcd->offleft;
	kcd->offleft = newpos;
	GScrollBarSetPos(kcd->hsb,newpos);
	rect.x = kcd->xstart2+1; rect.y = kcd->ystart;
	rect.width = kcd->width-1;
	rect.height = kcd->height+(kcd->ystart2-kcd->ystart);
	GDrawScroll(kcd->gw,&rect,-diff*kcd->kernw,0);
    }
}

static void KCD_VShow(KernClassDlg *kcd, int pos) {
    if ( pos<0 || pos>=kcd->first_cnt )
return;
#if 0
    if ( pos>=kcd->offtop && pos<kcd->offtop+(kcd->height/kcd->kernh) )
return;		/* Already visible */
#endif
    --pos;	/* One line of context */
    if ( pos + (kcd->height/kcd->kernh) >= kcd->first_cnt )
	pos = kcd->second_cnt - (kcd->height/kcd->kernh);
    if ( pos < 0 ) pos = 0;
    kcd->offtop = pos;
    GScrollBarSetPos(kcd->vsb,pos);
    GDrawRequestExpose(kcd->gw,NULL,false);
}

static void KCD_VScroll(KernClassDlg *kcd,struct sbevent *sb) {
    int newpos = kcd->offtop;
    GRect rect;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
	if ( kcd->height/kcd->kernh == 1 )
	    --newpos;
	else
	    newpos -= kcd->height/kcd->kernh - 1;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
	if ( kcd->height/kcd->kernh == 1 )
	    ++newpos;
	else
	    newpos += kcd->height/kcd->kernh - 1;
      break;
      case et_sb_bottom:
        newpos = kcd->first_cnt - (kcd->height/kcd->kernh);
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos + (kcd->height/kcd->kernh) >= kcd->first_cnt )
	newpos = kcd->first_cnt - (kcd->height/kcd->kernh);
    if ( newpos < 0 ) newpos = 0;
    if ( newpos!=kcd->offtop ) {
	int diff = newpos-kcd->offtop;
	kcd->offtop = newpos;
	GScrollBarSetPos(kcd->vsb,newpos);
	rect.x = kcd->xstart; rect.y = kcd->ystart2+1;
	rect.width = kcd->width+(kcd->xstart2-kcd->xstart);
	rect.height = kcd->height-1;
	GDrawScroll(kcd->gw,&rect,0,diff*kcd->kernh);
    }
}

static int subkern_e_h(GWindow gw, GEvent *event) {
    KernClassDlg *kcd = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_char:
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("metricsview.html#kernclass");
return( true );
	} else if ( event->u.chr.keysym=='q' && (event->u.chr.state&ksm_control)) {
	    if ( event->u.chr.state&ksm_shift )
		KC_DoCancel(kcd);
	    else
		MenuExit(NULL,NULL,NULL);
return( true );
	}
return( false );
      case et_mouseup: case et_mousedown: case et_mousemove:
	KCD_KernMouse(kcd,event);
      break;
      case et_expose:
	KCD_KernExpose(kcd,gw,event);
      break;
    }
return( true );
}

static int subkcd_e_h(GWindow gw, GEvent *event) {
    KernClassDlg *kcd = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_char:
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("metricsview.html#kernclass");
return( true );
	} else if ( event->u.chr.keysym=='q' && (event->u.chr.state&ksm_control)) {
	    if ( event->u.chr.state&ksm_shift )
		KC_DoCancel(kcd);
	    else
		MenuExit(NULL,NULL,NULL);
return( true );
	}
return( false );
    }
return( true );
}

static int kcd_e_h(GWindow gw, GEvent *event) {
    KernClassDlg *kcd = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_close:
	KC_DoCancel(kcd);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("metricsview.html#kernclass");
return( true );
	}
return( false );
      break;
      case et_destroy:
	kcd->kcld->kcd = NULL;
	GGadgetSetList(GWidgetGetControl(kcd->kcld->gw,CID_List),
		KCSLIArray(kcd->kcld->sf,kcd->kcld->isv),false);
	GGadgetSetEnabled(GWidgetGetControl(kcd->kcld->gw,CID_New),true);
	free(kcd);
      break;
      case et_mouseup: case et_mousemove: case et_mousedown:
	KCD_Mouse(kcd,event);
      break;
      case et_expose:
	KCD_Expose(kcd,gw,event);
      break;
      case et_resize: {
	GRect wsize, csize;
	int subwidth, offset;
	static int moveme[] = { CID_ClassList+100, CID_ClassNew+100,
		CID_ClassDel+100, CID_ClassEdit+100, CID_ClassLabel+100, 0};
	int i;

	GDrawGetSize(kcd->gw,&wsize);
	GDrawResize(kcd->cw,wsize.width,wsize.height);
	GGadgetResize(GWidgetGetControl(kcd->gw,CID_Group),wsize.width-4,wsize.height-4);
	GGadgetResize(GWidgetGetControl(kcd->gw,CID_Group2),wsize.width-4,wsize.height-4);
	GGadgetResize(GWidgetGetControl(kcd->gw,CID_Group3),wsize.width-4,wsize.height-4);
	GGadgetResize(GWidgetGetControl(kcd->gw,CID_Line2),wsize.width-20,3);
	GGadgetResize(GWidgetGetControl(kcd->gw,CID_Line1),wsize.width-20,3);

	GGadgetGetSize(GWidgetGetControl(kcd->gw,CID_OK),&csize);
	GGadgetMove(GWidgetGetControl(kcd->gw,CID_OK),15,wsize.height-kcd->canceldrop-3);
	GGadgetMove(GWidgetGetControl(kcd->gw,CID_Prev),15,wsize.height-kcd->canceldrop);
	GGadgetMove(GWidgetGetControl(kcd->gw,CID_Prev2),15,wsize.height-kcd->canceldrop);
	GGadgetGetSize(GWidgetGetControl(kcd->gw,CID_Cancel),&csize);
	GGadgetMove(GWidgetGetControl(kcd->gw,CID_Cancel),wsize.width-csize.width-15,wsize.height-kcd->canceldrop);
	GGadgetMove(GWidgetGetControl(kcd->gw,CID_Next),wsize.width-csize.width-15,wsize.height-kcd->canceldrop);
	GGadgetMove(GWidgetGetControl(kcd->gw,CID_Next2),wsize.width-csize.width-15,wsize.height-kcd->canceldrop);

	subwidth = (wsize.width-GDrawPointsToPixels(NULL,20))/2;
	GGadgetGetSize(GWidgetGetControl(kcd->gw,CID_ClassList),&csize);
	GGadgetResize(GWidgetGetControl(kcd->gw,CID_ClassList),subwidth,csize.height);
	GGadgetResize(GWidgetGetControl(kcd->gw,CID_ClassList+100),subwidth,csize.height);
	offset = subwidth - csize.width;
	for ( i=0; moveme[i]!=0; ++i ) {
	    GGadgetGetSize(GWidgetGetControl(kcd->gw,moveme[i]),&csize);
	    GGadgetMove(GWidgetGetControl(kcd->gw,moveme[i]),csize.x+offset,csize.y);
	}

	GGadgetGetSize(kcd->hsb,&csize);
	kcd->width = wsize.width-csize.height-kcd->xstart2-5;
	GGadgetResize(kcd->hsb,kcd->width,csize.height);
	GGadgetMove(kcd->hsb,kcd->xstart2,wsize.height-kcd->sbdrop-csize.height);
	GGadgetGetSize(kcd->vsb,&csize);
	kcd->height = wsize.height-kcd->sbdrop-kcd->ystart2-csize.width;
	GGadgetResize(kcd->vsb,csize.width,wsize.height-kcd->sbdrop-kcd->ystart2-csize.width);
	GGadgetMove(kcd->vsb,wsize.width-csize.width-5,kcd->ystart2);
	KCD_SBReset(kcd);
	GDrawRequestExpose(kcd->gw,NULL,false);

	GGadgetGetSize(GWidgetGetControl(kcd->cw,CID_GlyphList),&csize);
	GGadgetResize(GWidgetGetControl(kcd->gw,CID_GlyphList),wsize.width-20,csize.height);
      } break;
      case et_controlevent:
	switch( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    if ( event->u.control.g == kcd->hsb )
		KCD_HScroll(kcd,&event->u.control.u.sb);
	    else
		KCD_VScroll(kcd,&event->u.control.u.sb);
	  break;
	}
      break;
    }
return( true );
}

static int AddClassList(GGadgetCreateData *gcd, GTextInfo *label, int k, int off,
	int x, int y, int width ) {
    int blen = GIntGetResource(_NUM_Buttonsize);
    int space = 10;

    label[k].text = (unichar_t *) (x<20?_STR_FirstChar:_STR_SecondChar);
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = x+10; gcd[k].gd.pos.y = y;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_ClassLabel+off;
    gcd[k++].creator = GLabelCreate;

    gcd[k].gd.pos.x = x; gcd[k].gd.pos.y = y+13;
    gcd[k].gd.pos.width = width;
    gcd[k].gd.pos.height = 8*12+10;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_list_multiplesel;
    gcd[k].gd.handle_controlevent = KCD_ClassSelected;
    gcd[k].gd.cid = CID_ClassList+off;
    gcd[k++].creator = GListCreate;

    label[k].text = (unichar_t *) _STR_NewDDD;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+gcd[k-1].gd.pos.height+10;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.handle_controlevent = KCD_New;
    gcd[k].gd.cid = CID_ClassNew+off;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _STR_EditDDD;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = x+blen+space; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = KCD_Edit;
    gcd[k].gd.cid = CID_ClassEdit+off;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _STR_Delete;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+blen+space; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = KCD_Delete;
    gcd[k].gd.cid = CID_ClassDel+off;
    gcd[k++].creator = GButtonCreate;
return( k );
}

static void KernClassD(KernClassListDlg *kcld,KernClass *kc) {
    GRect pos, subpos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[30];
    GTextInfo label[30];
    KernClassDlg *kcd;
    int i,j, kc_width, vi, y, k;
    static unichar_t courier[] = { 'c', 'o', 'u', 'r', 'i', 'e', 'r', ',', 'm','o','n','o','s','p','a','c','e',',','c','l','e','a','r','l','y','u',',', 'u','n','i','f','o','n','t', '\0' };
    int as, ds, ld, sbsize;
    FontRequest rq;
    static unichar_t kernw[] = { '-', '1', '2', '3', '4', '5', 0 };
    GWindow gw;

    if ( kcld->kcd!=NULL ) {
	GDrawSetVisible(kcld->kcd->gw,true);
	GDrawRaise(kcld->kcd->gw);
return;
    }
    kcd = gcalloc(1,sizeof(KernClassDlg));
    kcd->kcld = kcld;
    kcld->kcd = kcd;
    kcd->orig = kc;
    kcd->pixelsize = 150;
    if ( kc==NULL ) {
	kcd->first_cnt = kcd->second_cnt = 1;
	kcd->offsets = gcalloc(1,sizeof(int16));
    } else {
	kcd->first_cnt = kc->first_cnt;
	kcd->second_cnt = kc->second_cnt;
	kcd->offsets = galloc(kc->first_cnt*kc->second_cnt*sizeof(int16));
	memcpy(kcd->offsets,kc->offsets,kc->first_cnt*kc->second_cnt*sizeof(int16));
    }

    memset(&wattrs,0,sizeof(wattrs));
    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));

    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = false;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource( _STR_KernClass,NULL );
    wattrs.is_dlg = false;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,KC_Width));
    pos.height = GDrawPointsToPixels(NULL,KC_Height);
    kcd->gw = gw = GDrawCreateTopWindow(NULL,&pos,kcd_e_h,kcd,&wattrs);

    kc_width = GDrawPixelsToPoints(NULL,pos.width*100/GGadgetScale(100));

    i = 0;
    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = 5;
    gcd[i].gd.pos.width = kc_width-20;
    gcd[i].gd.flags = gg_enabled|gg_visible;
    gcd[i].gd.u.list = SFLangList(kcld->sf,true,(SplineChar *) -1);
    if ( kc!=NULL ) {
	for ( j=0; gcd[i].gd.u.list[j].text!=NULL; ++j )
	    gcd[i].gd.u.list[j].selected = false;
	gcd[i].gd.u.list[kc->sli].selected = true;
    }
    gcd[i].gd.cid = CID_SLI;
    gcd[i].gd.handle_controlevent = KC_Sli;
    gcd[i++].creator = GListButtonCreate;

    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+28;
    gcd[i].gd.flags = gg_visible | gg_enabled | (kc!=NULL && (kc->flags&pst_r2l)?gg_cb_on:0);
    label[i].text = (unichar_t *) _STR_RightToLeft;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_R2L;
    gcd[i++].creator = GCheckBoxCreate;

    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15;
    gcd[i].gd.flags = gg_visible | gg_enabled | (kc!=NULL && (kc->flags&pst_ignorebaseglyphs)?gg_cb_on:0);
    label[i].text = (unichar_t *) _STR_IgnoreBaseGlyphs;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_IgnBase;
    gcd[i++].creator = GCheckBoxCreate;

    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15;
    gcd[i].gd.flags = gg_visible | gg_enabled | (kc!=NULL && (kc->flags&pst_ignoreligatures)?gg_cb_on:0);
    label[i].text = (unichar_t *) _STR_IgnoreLigatures;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_IgnLig;
    gcd[i++].creator = GCheckBoxCreate;

    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15;
    gcd[i].gd.flags = gg_visible | gg_enabled | (kc!=NULL && (kc->flags&pst_ignorecombiningmarks)?gg_cb_on:0);
    label[i].text = (unichar_t *) _STR_IgnoreCombiningMarks;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_IgnMark;
    gcd[i++].creator = GCheckBoxCreate;

    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = GDrawPointsToPixels(gw,gcd[i-1].gd.pos.y+17);
    gcd[i].gd.pos.width = pos.width-20;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[i].gd.cid = CID_Line1;
    gcd[i++].creator = GLineCreate;

    y = gcd[i-2].gd.pos.y+23;
    i = AddClassList(gcd,label,i,0,5,y,(kc_width-20)/2);
    i = AddClassList(gcd,label,i,100,(kc_width-20)/2+10,y,(kc_width-20)/2);

    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = GDrawPointsToPixels(gw,gcd[i-3].gd.pos.y+27);
    gcd[i].gd.pos.width = pos.width-20;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[i].gd.cid = CID_Line2;
    gcd[i++].creator = GLineCreate;

    kcd->canceldrop = GDrawPointsToPixels(gw,KC_CANCELDROP);
    kcd->sbdrop = kcd->canceldrop+GDrawPointsToPixels(gw,7);

    vi = i;
    gcd[i].gd.pos.width = sbsize = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gcd[i].gd.pos.x = pos.width-sbsize;
    gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+8;
    gcd[i].gd.pos.height = pos.height-gcd[i].gd.pos.y-sbsize-kcd->sbdrop;
    gcd[i].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    gcd[i++].creator = GScrollBarCreate;
    kcd->height = gcd[i-1].gd.pos.height;
    kcd->ystart = gcd[i-1].gd.pos.y;

    gcd[i].gd.pos.height = sbsize;
    gcd[i].gd.pos.y = pos.height-sbsize-8;
    gcd[i].gd.pos.x = 4;
    gcd[i].gd.pos.width = pos.width-sbsize;
    gcd[i].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    gcd[i++].creator = GScrollBarCreate;
    kcd->width = gcd[i-1].gd.pos.width;
    kcd->xstart = 5;

    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+24+3;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _STR_OK;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = KC_OK;
    gcd[i].gd.cid = CID_OK;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = -10; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _STR_Cancel;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = KC_Cancel;
    gcd[i].gd.cid = CID_Cancel;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = 2; gcd[i].gd.pos.y = 2;
    gcd[i].gd.pos.width = pos.width-4;
    gcd[i].gd.pos.height = pos.height-4;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[i].gd.cid = CID_Group;
    gcd[i++].creator = GGroupCreate;

    memset(&rq,'\0',sizeof(rq));
    rq.point_size = 12;
    rq.weight = 400;
    rq.family_name = courier;
    kcd->font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawFontMetrics(kcd->font,&as,&ds,&ld);
    kcd->fh = as+ds; kcd->as = as;
    GDrawSetFont(gw,kcd->font);

    kcd->kernh = kcd->fh+3;
    kcd->kernw = GDrawGetTextWidth(gw,kernw,-1,NULL)+3;
    kcd->xstart2 = kcd->xstart+kcd->kernw;
    kcd->ystart2 = kcd->ystart+kcd->fh+1;

    GGadgetsCreate(kcd->gw,gcd);
    kcd->vsb = gcd[vi].ret;
    kcd->hsb = gcd[vi+1].ret;

    for ( i=0; i<2; ++i ) {
	GGadget *list = GWidgetGetControl(kcd->gw,CID_ClassList+i*100);
	GListAppendLine(list,GStringGetResource(_STR_EverythingElse,NULL),false);
	if ( kcd->orig!=NULL ) {
	    for ( k=1; k<(&kcd->first_cnt)[i]; ++k ) {
		unichar_t *temp = uc_copy((&kcd->orig->firsts)[i][k]);
		GListAppendLine(list,temp,false);
		free(temp);
	    }
	}
    }

    wattrs.mask = wam_events;
    subpos = pos; subpos.x = subpos.y = 0;
    kcd->cw = GWidgetCreateSubWindow(kcd->gw,&subpos,subkcd_e_h,kcd,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    k = 0;

    label[k].text = (unichar_t *) _STR_Set;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_SetGlyphsFromSelectionPopup,NULL);
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.handle_controlevent = KCD_FromSelection;
    gcd[k].gd.cid = CID_Set;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _STR_Select_nom;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 70; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_SelectFromGlyphsPopup,NULL);
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.handle_controlevent = KCD_ToSelection;
    gcd[k].gd.cid = CID_Select;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 30;
    gcd[k].gd.pos.width = KC_Width-25; gcd[k].gd.pos.height = 8*13+4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
    gcd[k].gd.cid = CID_GlyphList;
    gcd[k++].creator = GTextAreaCreate;

    label[k].text = (unichar_t *) _STR_PrevArrow;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30; gcd[k].gd.pos.y = KC_Height-KC_CANCELDROP;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled /*| gg_but_cancel*/;
    gcd[k].gd.handle_controlevent = KCD_Prev;
    gcd[k].gd.cid = CID_Prev;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _STR_NextArrow;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = -30+3; gcd[k].gd.pos.y = KC_Height-KC_CANCELDROP;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled /*| gg_but_default*/;
    gcd[k].gd.handle_controlevent = KCD_Next;
    gcd[k].gd.cid = CID_Next;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width-4;
    gcd[k].gd.pos.height = pos.height-4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k].gd.cid = CID_Group2;
    gcd[k++].creator = GGroupCreate;

    GGadgetsCreate(kcd->cw,gcd);


    kcd->kw = GWidgetCreateSubWindow(kcd->gw,&subpos,subkern_e_h,kcd,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    k = 0;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = 120;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.handle_controlevent = KCD_GlyphSelected;
    gcd[k].gd.cid = CID_First;
    gcd[k++].creator = GListButtonCreate;

    gcd[k].gd.pos.x = 130; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = 120;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.handle_controlevent = KCD_GlyphSelected;
    gcd[k].gd.cid = CID_Second;
    gcd[k++].creator = GListButtonCreate;

    label[k].text = (unichar_t *) _STR_KernOffset;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k++].creator = GLabelCreate;

    gcd[k].gd.pos.x = 90; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_KernOffset;
    gcd[k].gd.handle_controlevent = KCD_KernOffChanged;
    gcd[k++].creator = GTextFieldCreate;

    label[k].text = (unichar_t *) _STR_PrevArrow;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30; gcd[k].gd.pos.y = KC_Height-KC_CANCELDROP;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled /*| gg_but_cancel*/;
    gcd[k].gd.handle_controlevent = KCD_Prev2;
    gcd[k].gd.cid = CID_Prev2;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _STR_NextArrow;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = -30+3; gcd[k].gd.pos.y = KC_Height-KC_CANCELDROP;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled /*| gg_but_default*/;
    gcd[k].gd.handle_controlevent = KCD_Next2;
    gcd[k].gd.cid = CID_Next2;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width-4;
    gcd[k].gd.pos.height = pos.height-4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k].gd.cid = CID_Group3;
    gcd[k++].creator = GGroupCreate;

    GGadgetsCreate(kcd->kw,gcd);
    kcd->top = GDrawPointsToPixels(kcd->gw,60);


    GDrawSetVisible(kcd->gw,true);

    GGadgetSetEnabled(GWidgetGetControl(kcld->gw,CID_Delete),false);
    GGadgetSetEnabled(GWidgetGetControl(kcld->gw,CID_Edit),false);
    GGadgetSetEnabled(GWidgetGetControl(kcld->gw,CID_New),false);
}

static int KCL_New(GGadget *g, GEvent *e) {
    KernClassListDlg *kcld;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	kcld = GDrawGetUserData(GGadgetGetWindow(g));
	if ( kcld->kcd==NULL )
	    KernClassD(kcld,NULL);
    }
return( true );
}

static int KCL_Delete(GGadget *g, GEvent *e) {
    int len, i,j;
    GTextInfo **old, **new;
    GGadget *list;
    KernClassListDlg *kcld;
    KernClass *p, *kc, *n;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	kcld = GDrawGetUserData(GGadgetGetWindow(g));
	if ( kcld->kcd==NULL ) {
	    list = GWidgetGetControl(kcld->gw,CID_List);
	    old = GGadgetGetList(list,&len);
	    new = gcalloc(len+1,sizeof(GTextInfo *));
	    p = NULL; kc = kcld->isv ? kcld->sf->vkerns : kcld->sf->kerns;
	    for ( i=j=0; i<len; ++i, kc = n ) {
		n = kc->next;
		if ( !old[i]->selected ) {
		    new[j] = galloc(sizeof(GTextInfo));
		    *new[j] = *old[i];
		    new[j]->text = u_copy(new[j]->text);
		    ++j;
		    p = kc;
		} else {
		    if ( p!=NULL )
			p->next = n;
		    else if ( kcld->isv )
			kcld->sf->vkerns = n;
		    else
			kcld->sf->kerns = n;
		    kc->next = NULL;
		    KernClassListFree(kc);
		}
	    }
	    new[j] = gcalloc(1,sizeof(GTextInfo));
	    GGadgetSetList(list,new,false);
	}
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Delete),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Edit),false);
    }
return( true );
}

static int KCL_Edit(GGadget *g, GEvent *e) {
    int sel, i;
    KernClassListDlg *kcld;
    KernClass *kc;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	kcld = GDrawGetUserData(GGadgetGetWindow(g));
	sel = GGadgetGetFirstListSelectedItem(GWidgetGetControl(GGadgetGetWindow(g),CID_List));
	if ( sel==-1 || kcld->kcd!=NULL )
return( true );
	for ( kc=kcld->isv?kcld->sf->vkerns:kcld->sf->kerns, i=0; i<sel; kc=kc->next, ++i );
	KernClassD(kcld,kc);
    }
return( true );
}

static int KCL_Done(GGadget *g, GEvent *e) {
    KernClassListDlg *kcld;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	kcld = GDrawGetUserData(GGadgetGetWindow(g));
	if ( kcld->kcd != NULL )
	    KC_DoCancel(kcld->kcd);
	GDrawDestroyWindow(kcld->gw);
    }
return( true );
}

static int KCL_SelChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	KernClassListDlg *kcld = GDrawGetUserData(GGadgetGetWindow(g));
	int sel = GGadgetGetFirstListSelectedItem(g);
	GGadgetSetEnabled(GWidgetGetControl(kcld->gw,CID_Delete),sel!=-1 && kcld->kcd==NULL);
	GGadgetSetEnabled(GWidgetGetControl(kcld->gw,CID_Edit),sel!=-1 && kcld->kcd==NULL);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	KernClassListDlg *kcld = GDrawGetUserData(GGadgetGetWindow(g));
	if ( kcld->kcd==NULL ) {
	    e->u.control.subtype = et_buttonactivate;
	    e->u.control.g = GWidgetGetControl(kcld->gw,CID_Edit);
	    KCL_Edit(e->u.control.g,e);
	} else {
	    GDrawSetVisible(kcld->kcd->gw,true);
	    GDrawRaise(kcld->kcd->gw);
	}
    }
return( true );
}

static int kcl_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	KernClassListDlg *kcld = GDrawGetUserData(gw);
	if ( kcld->kcd != NULL )
	    KC_DoCancel(kcld->kcd);
	GDrawDestroyWindow(kcld->gw);
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("metricsview.html#kernclass");
return( true );
	}
return( false );
    } else if ( event->type == et_destroy ) {
	KernClassListDlg *kcld = GDrawGetUserData(gw);
	if ( kcld->isv )
	    kcld->sf->vkcld = NULL;
	else
	    kcld->sf->kcld = NULL;
	free(kcld);
    }
return( true );
}

void ShowKernClasses(SplineFont *sf,MetricsView *mv,int isv) {
    KernClassListDlg *kcld;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[7];
    GTextInfo label[7];

    if ( sf->kcld && !isv ) {
	GDrawSetVisible(sf->kcld->gw,true);
	GDrawRaise(sf->kcld->gw);
return;
    } else if ( sf->vkcld && isv ) {
	GDrawSetVisible(sf->vkcld->gw,true);
	GDrawRaise(sf->vkcld->gw);
return;
    }

    kcld = gcalloc(1,sizeof(KernClassListDlg));
    kcld->sf = sf;
    kcld->isv = isv;
    if ( isv )
	sf->vkcld = kcld;
    else
	sf->kcld = kcld;

    memset(&wattrs,0,sizeof(wattrs));
    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));

    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = false;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource( isv?_STR_VKernByClasses:_STR_KernByClasses,NULL );
    wattrs.is_dlg = false;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,KCL_Width));
    pos.height = GDrawPointsToPixels(NULL,KCL_Height);
    kcld->gw = GDrawCreateTopWindow(NULL,&pos,kcl_e_h,kcld,&wattrs);

    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5;
    gcd[0].gd.pos.width = KCL_Width-28; gcd[0].gd.pos.height = 7*12+10;
    gcd[0].gd.flags = gg_visible | gg_enabled | gg_list_multiplesel;
    gcd[0].gd.cid = CID_List;
    gcd[0].gd.u.list = KCSLIList(sf,isv);
    gcd[0].gd.handle_controlevent = KCL_SelChanged;
    gcd[0].creator = GListCreate;

    gcd[1].gd.pos.x = 10; gcd[1].gd.pos.y = gcd[0].gd.pos.y+gcd[0].gd.pos.height+4;
    gcd[1].gd.pos.width = -1;
    gcd[1].gd.flags = gg_visible | gg_enabled;
    label[1].text = (unichar_t *) _STR_NewDDD;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.cid = CID_New;
    gcd[1].gd.handle_controlevent = KCL_New;
    gcd[1].creator = GButtonCreate;

    gcd[2].gd.pos.x = 20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor); gcd[2].gd.pos.y = gcd[1].gd.pos.y;
    gcd[2].gd.pos.width = -1;
    gcd[2].gd.flags = gg_visible;
    label[2].text = (unichar_t *) _STR_Delete;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.cid = CID_Delete;
    gcd[2].gd.handle_controlevent = KCL_Delete;
    gcd[2].creator = GButtonCreate;

    gcd[3].gd.pos.x = -10; gcd[3].gd.pos.y = gcd[1].gd.pos.y;
    gcd[3].gd.pos.width = -1;
    gcd[3].gd.flags = gg_visible;
    label[3].text = (unichar_t *) _STR_EditDDD;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.cid = CID_Edit;
    gcd[3].gd.handle_controlevent = KCL_Edit;
    gcd[3].creator = GButtonCreate;

    gcd[4].gd.pos.x = 10; gcd[4].gd.pos.y = gcd[1].gd.pos.y+28;
    gcd[4].gd.pos.width = KCL_Width-20;
    gcd[4].gd.flags = gg_visible;
    gcd[4].creator = GLineCreate;

    gcd[5].gd.pos.x = (KCL_Width-GIntGetResource(_NUM_Buttonsize))/2; gcd[5].gd.pos.y = gcd[4].gd.pos.y+7;
    gcd[5].gd.pos.width = -1;
    gcd[5].gd.flags = gg_visible|gg_enabled|gg_but_default|gg_but_cancel;
    label[5].text = (unichar_t *) _STR_Done;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.handle_controlevent = KCL_Done;
    gcd[5].creator = GButtonCreate;

    GGadgetsCreate(kcld->gw,gcd);
    GDrawSetVisible(kcld->gw,true);
}

void KCLD_End(KernClassListDlg *kcld) {
    if ( kcld==NULL )
return;
    if ( kcld->kcd != NULL )
	KC_DoCancel(kcld->kcd);
    GDrawDestroyWindow(kcld->gw);
}

void KCLD_MvDetach(KernClassListDlg *kcld,MetricsView *mv) {
    if ( kcld==NULL )
return;
}

int KernClassContains(KernClass *kc, char *name1, char *name2, int ordered ) {
    int infirst=0, insecond=0, scpos1, kwpos1, scpos2, kwpos2;
    int i;

    for ( i=1; i<kc->first_cnt; ++i ) {
	if ( PSTContains(kc->firsts[i],name1) ) {
	    scpos1 = i;
	    if ( ++infirst>=3 )		/* The name occurs twice??? */
    break;
	} else if ( PSTContains(kc->firsts[i],name2) ) {
	    kwpos1 = i;
	    if ( (infirst+=2)>=3 )
    break;
	}
    }
    if ( infirst==0 || infirst>3 )
return( 0 );
    for ( i=1; i<kc->second_cnt; ++i ) {
	if ( PSTContains(kc->seconds[i],name1) ) {
	    scpos2 = i;
	    if ( ++insecond>=3 )
    break;
	} else if ( PSTContains(kc->seconds[i],name2) ) {
	    kwpos2 = i;
	    if ( (insecond+=2)>=3 )
    break;
	}
    }
    if ( insecond==0 || insecond>3 )
return( 0 );
    if ( (infirst&1) && (insecond&2) ) {
	if ( kc->offsets[scpos1*kc->second_cnt+kwpos2]!=0 )
return( kc->offsets[scpos1*kc->second_cnt+kwpos2] );
    }
    if ( !ordered ) {
	if ( (infirst&2) && (insecond&1) ) {
	    if ( kc->offsets[kwpos1*kc->second_cnt+scpos2]!=0 )
return( kc->offsets[kwpos1*kc->second_cnt+scpos2] );
	}
    }
return( 0 );
}
