/* Copyright (C) 2003-2006 by George Williams */
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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
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
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    DeviceTable *adjusts;
    DeviceTable active_adjust;		/* The one that is currently active */
#endif
    GWindow gw, cw, kw;
    GFont *font;
    int fh, as;
    int kernh, kernw;		/* Width of the box containing the kerning val */
    int xstart, ystart;		/* This is where the headers start */
    int xstart2, ystart2;	/* This is where the data start */
    int width, height, fullwidth;
    int canceldrop, sbdrop;
    int offleft, offtop;
    GGadget *hsb, *vsb;
    int isedit, off;
    int st_pos;
    BDFChar *fsc, *ssc;
    int pixelsize;
    int magfactor;
    int top;
    int downpos, down, within, orig_kern;
/* For the kern pair dlg */
    int done;
    SplineFont *sf;
    SplineChar *sc1, *sc2;
    int isv, iskernpair;
    SplineChar *scf, *scs;
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
#define KC_Height	424
#define KC_CANCELDROP	33


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
#define CID_ClassUp	1012
#define CID_ClassDown	1013
#define CID_ClassSelect	1014

#define CID_OK		1015
#define CID_Cancel	1016
#define CID_Group	1017
#define CID_Line1	1018
#define CID_Line2	1019

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
#define CID_DisplaySize	1036
#define CID_Correction	1037
#define CID_FreeType	1038
#define CID_Magnifications	1039

extern int _GScrollBar_Width;

static GTextInfo magnifications[] = {
    { (unichar_t *) "100%", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 1, 0, 1},
    { (unichar_t *) "200%", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "300%", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "400%", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }
};

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
	SplineFont *sf = kcd->sf;
	FontView *fv = sf->fv;
	const unichar_t *end;
	int pos, found=-1;
	char *nm;

	GDrawSetVisible(fv->gw,true);
	GDrawRaise(fv->gw);
	memset(fv->selected,0,fv->map->enccount);
	while ( *ret ) {
	    end = u_strchr(ret,' ');
	    if ( end==NULL ) end = ret+u_strlen(ret);
	    nm = cu_copybetween(ret,end);
	    for ( ret = end; isspace(*ret); ++ret);
	    if (( pos = SFFindSlot(sf,fv->map,-1,nm))!=-1 ) {
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
	SplineFont *sf = kcd->sf;
	FontView *fv = sf->fv;
	unichar_t *vals, *pt;
	int i, len, max, gid;
	SplineChar *sc;
    
	for ( i=len=max=0; i<fv->map->enccount; ++i ) if ( fv->selected[i]) {
	    sc = SFMakeChar(sf,fv->map,i);
	    len += strlen(sc->name)+1;
	    if ( fv->selected[i]>max ) max = fv->selected[i];
	}
	pt = vals = galloc((len+1)*sizeof(unichar_t));
	*pt = '\0';
	/* in a class the order of selection is irrelevant */
	for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] && (gid=fv->map->map[i])!=-1) {
	    uc_strcpy(pt,sf->glyphs[gid]->name);
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

static int isEverythingElse(unichar_t *text) {
    /* GT: The string "{Everything Else}" is used in the context of a list */
    /* GT: of classes (a set of kerning classes) where class 0 designates the */
    /* GT: default class containing all glyphs not specified in the other classes */
    unichar_t *everything_else = utf82u_copy( _("{Everything Else}") );
    int ret = u_strcmp(text,everything_else);
    free(everything_else);
return( ret==0 );
}

static int KCD_Next(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	char *ret = GGadgetGetTitle8(GWidgetGetControl(kcd->gw,CID_GlyphList));
	GGadget *list = GWidgetGetControl( kcd->gw, CID_ClassList+kcd->off );
	int i;
	char *pt;
	int which = GGadgetGetFirstListSelectedItem(list);

	for ( pt=ret; *pt==' '; ++pt );

	if ( which==0 && *pt=='\0' )
	    /* Class 0 may contain no glyphs */;
	else if ( !CCD_NameListCheck(kcd->sf,ret,true,_("Bad Class")) ||
		CCD_InvalidClassList(ret,list,kcd->isedit)) {
	    free(ret);
return( true );
	}

	if ( kcd->isedit ) {
	    if ( which==0 && *pt=='\0' )
		GListChangeLine8(list,0,_("{Everything Else}"));
	    else
		GListChangeLine8(list,GGadgetGetFirstListSelectedItem(list),ret);
	} else {
	    GListAppendLine8(list,ret,false);
	    if ( kcd->off==0 ) {
		kcd->offsets = grealloc(kcd->offsets,(kcd->first_cnt+1)*kcd->second_cnt*sizeof(int16));
		memset(kcd->offsets+kcd->first_cnt*kcd->second_cnt,
			0, kcd->second_cnt*sizeof(int16));
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		kcd->adjusts = grealloc(kcd->adjusts,(kcd->first_cnt+1)*kcd->second_cnt*sizeof(DeviceTable));
		memset(kcd->adjusts+kcd->first_cnt*kcd->second_cnt,
			0, kcd->second_cnt*sizeof(DeviceTable));
#endif
		++kcd->first_cnt;
	    } else {
		int16 *new = galloc(kcd->first_cnt*(kcd->second_cnt+1)*sizeof(int16));
		for ( i=0; i<kcd->first_cnt; ++i ) {
		    memcpy(new+i*(kcd->second_cnt+1),kcd->offsets+i*kcd->second_cnt,
			    kcd->second_cnt*sizeof(int16));
		    new[i*(kcd->second_cnt+1)+kcd->second_cnt] = 0;
		}
		free( kcd->offsets );
		kcd->offsets = new;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		{
		    DeviceTable *new = galloc(kcd->first_cnt*(kcd->second_cnt+1)*sizeof(DeviceTable));
		    for ( i=0; i<kcd->first_cnt; ++i ) {
			memcpy(new+i*(kcd->second_cnt+1),kcd->adjusts+i*kcd->second_cnt,
				kcd->second_cnt*sizeof(DeviceTable));
			memset(&new[i*(kcd->second_cnt+1)+kcd->second_cnt],0,sizeof(DeviceTable));
		    }
		    free( kcd->adjusts );
		    kcd->adjusts = new;
		}
#endif
		++kcd->second_cnt;
	    }
	    KCD_SBReset(kcd);
	}
	GDrawSetVisible(kcd->cw,false);		/* This will give us an expose so we needed ask for one */
	free(ret);
    }
return( true );
}

static void _KCD_DoEditNew(KernClassDlg *kcd,int isedit,int off) {
    static unichar_t nullstr[] = { 0 };

    kcd->isedit = isedit;
    kcd->off = off;
    if ( isedit ) {
	GGadget *list = GWidgetGetControl( kcd->gw, CID_ClassList+off);
	GTextInfo *selected = GGadgetGetListItemSelected(list);
	int which = GGadgetGetFirstListSelectedItem(list);
	if ( selected==NULL )
return;
	if ( off==0 && which==0 && isEverythingElse(selected->text))
	    GGadgetSetTitle(GWidgetGetControl(kcd->cw,CID_GlyphList),nullstr);
	else
	    GGadgetSetTitle(GWidgetGetControl(kcd->cw,CID_GlyphList),selected->text);
    } else {
	GGadgetSetTitle(GWidgetGetControl(kcd->cw,CID_GlyphList),nullstr);
    }
    GWidgetIndicateFocusGadget(GWidgetGetControl(kcd->cw,CID_GlyphList));
    GDrawSetVisible(kcd->cw,true);
}

/* ************************************************************************** */
/* ************************** Kern Class Display **************************** */
/* ************************************************************************** */

static void KPD_DoCancel(KernClassDlg *kcd) {
    BDFCharFree(kcd->fsc); BDFCharFree(kcd->ssc);
    kcd->fsc = kcd->ssc = NULL;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    free(kcd->active_adjust.corrections); kcd->active_adjust.corrections = NULL;
#endif
    kcd->done = true;
}

static int KPD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	KPD_DoCancel(kcd);
    }
return( true );
}

static void KPD_FinishKP(KernClassDlg *);

static int KPD_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	KPD_FinishKP(kcd);
	BDFCharFree(kcd->fsc); BDFCharFree(kcd->ssc);
	kcd->fsc = kcd->ssc = NULL;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	free(kcd->active_adjust.corrections); kcd->active_adjust.corrections = NULL;
#endif
	kcd->done = true;
    }
return( true );
}

static int KCD_Prev2(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	BDFCharFree(kcd->fsc); BDFCharFree(kcd->ssc);
	kcd->fsc = kcd->ssc = NULL;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	free(kcd->active_adjust.corrections); kcd->active_adjust.corrections = NULL;
#endif
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
#if defined(FONTFORGE_CONFIG_GDRAW)
	    gwwv_post_error( _("Bad Number"), _("Bad Number") );
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error( _("Bad Number"), _("Bad Number") );
#endif
return( true );
	}
	kcd->offsets[kcd->st_pos] = val;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	free(kcd->adjusts[kcd->st_pos].corrections);
	kcd->adjusts[kcd->st_pos] = kcd->active_adjust;
	kcd->active_adjust.corrections = NULL;
#endif

	BDFCharFree(kcd->fsc); BDFCharFree(kcd->ssc);
	kcd->fsc = kcd->ssc = NULL;
	GDrawSetVisible(kcd->kw,false);		/* This will give us an expose so we needed ask for one */
    }
return( true );
}

void KCD_DrawGlyph(GWindow pixmap,int x,int baseline,BDFChar *bdfc,int mag) {
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
	clut.clut[1] = 0x000000;
    } else {
	int scale, l;
	Color fg, bg;
	scale = bdfc->depth == 8 ? 8 : 4;
	base.image_type = it_index;
	clut.clut_len = 1<<scale;
	bg = default_background;
	fg = 0x000000;
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
    x += mag*bdfc->xmin;
    if ( mag==1 )
	GDrawDrawImage(pixmap,&gi,NULL,x,baseline-bdfc->ymax);
    else
	GDrawDrawImageMagnified(pixmap, &gi, NULL,
		x,baseline-mag*bdfc->ymax,
		base.width*mag,base.height*mag);
}

static void KCD_KernMouse(KernClassDlg *kcd,GEvent *event) {
    int x, y, width;
    char buf[20];
    unichar_t ubuf[20];
    int kern, pkern;
    double scale;

    scale = kcd->pixelsize/(double) (kcd->sf->ascent+kcd->sf->descent);
    kern = u_strtol(_GGadgetGetTitle(GWidgetGetControl(kcd->kw,CID_KernOffset)),NULL,10);
    pkern = kcd->magfactor*rint( kern*scale );	/* rounding can't include magnification */

    if ( !kcd->isv ) {
	/* Horizontal */
	width = kcd->magfactor*((kcd->fsc!=NULL?kcd->fsc->width:0)+(kcd->ssc!=NULL?kcd->ssc->width:0))+pkern;
	x = (kcd->fullwidth - width)/2;

	if ( GGadgetIsChecked(GWidgetGetControl(kcd->gw,CID_R2L)) ) {
	    if ( kcd->ssc!=NULL )
		width -= kcd->magfactor*kcd->ssc->width;
	} else {
	    if ( kcd->fsc!=NULL ) {
		x += kcd->magfactor*kcd->fsc->width + pkern;
		width -= kcd->magfactor*kcd->fsc->width + pkern;
	    }
	}

	if ( event->u.mouse.y<kcd->top || event->u.mouse.y>kcd->top+2*kcd->pixelsize*kcd->magfactor ||
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
	    int nkern = kcd->orig_kern + rint(2*(event->u.mouse.x-kcd->downpos)/scale/kcd->magfactor);
	    if ( kern!=nkern ) {
		sprintf(buf, "%d", nkern);
		uc_strcpy(ubuf,buf);
		GGadgetSetTitle(GWidgetGetControl(kcd->kw,CID_KernOffset),ubuf);
		GDrawRequestExpose(kcd->kw,NULL,false);
	    }
	    if ( event->type==et_mouseup ) {
		kcd->down = false;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		if ( nkern!=kcd->orig_kern && kcd->active_adjust.corrections!=NULL ) {
		    free(kcd->active_adjust.corrections);
		    kcd->active_adjust.corrections = NULL;
		    ubuf[0] = '0'; ubuf[1] = '\0';
		    GGadgetSetTitle(GWidgetGetControl(kcd->kw,CID_Correction),ubuf);
		    GDrawRequestExpose(kcd->kw,NULL,false);
		}
#endif
	    }
	}
    } else {
	/* Vertical */
	y = kcd->top + kcd->pixelsize/3;
	width = (kcd->ssc!=NULL ? kcd->magfactor*rint(kcd->ssc->sc->vwidth * scale) : 0);
	if ( kcd->fsc!=NULL )
	    y += kcd->magfactor*rint(kcd->fsc->sc->vwidth * scale) + pkern;
	x = (kcd->fullwidth/2 - kcd->pixelsize/2)*kcd->magfactor;

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
	    int nkern = kcd->orig_kern + rint((event->u.mouse.y-kcd->downpos)/scale)/kcd->magfactor;
	    if ( kern!=nkern ) {
		sprintf(buf, "%d", nkern);
		uc_strcpy(ubuf,buf);
		GGadgetSetTitle(GWidgetGetControl(kcd->kw,CID_KernOffset),ubuf);
		GDrawRequestExpose(kcd->kw,NULL,false);
	    }
	    if ( event->type==et_mouseup ) {
		kcd->down = false;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		if ( nkern!=kcd->orig_kern && kcd->active_adjust.corrections!=NULL ) {
		    free(kcd->active_adjust.corrections);
		    kcd->active_adjust.corrections = NULL;
		    ubuf[0] = '0'; ubuf[1] = '\0';
		    GGadgetSetTitle(GWidgetGetControl(kcd->kw,CID_Correction),ubuf);
		    GDrawRequestExpose(kcd->kw,NULL,false);
		}
#endif
	    }
	}
    }
}

static void KCD_KernExpose(KernClassDlg *kcd,GWindow pixmap,GEvent *event) {
    GRect *area = &event->u.expose.rect;
    GRect rect;
    GRect old1;
    int x, y;
    SplineFont *sf = kcd->sf;
    int em = sf->ascent+sf->descent;
    int as = kcd->magfactor*rint(sf->ascent*kcd->pixelsize/(double) em);
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_KernOffset));
    int kern = u_strtol(ret,NULL,10);
    int baseline, xbaseline;

    if ( area->y+area->height<kcd->top )
return;
    if ( area->y>kcd->top+3*kcd->pixelsize ||
	    (!kcd->isv && area->y>kcd->top+2*kcd->pixelsize ))
return;

    rect.x = 0; rect.y = kcd->top;
    rect.width = kcd->fullwidth;
    rect.height = kcd->isv ? 3*kcd->pixelsize*kcd->magfactor : 2*kcd->pixelsize*kcd->magfactor;
    GDrawPushClip(pixmap,&rect,&old1);

    kern = kcd->magfactor*rint(kern*kcd->pixelsize/(double) em);

#ifdef FONTFORGE_CONFIG_DEVICETABLES
    { int correction;
	unichar_t *end;

	ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_Correction));
	correction = u_strtol(ret,&end,10);
	while ( *end==' ' ) ++end;
	if ( *end=='\0' && correction>=-128 && correction<=127 )
	    kern += correction*kcd->magfactor;
    }
#endif

    if ( !kcd->isv ) {
	x = (kcd->fullwidth-( kcd->magfactor*(kcd->fsc!=NULL?kcd->fsc->width:0)+
		kcd->magfactor*(kcd->ssc!=NULL?kcd->ssc->width:0)+
		kern))/2;
	baseline = kcd->top + as + kcd->magfactor*kcd->pixelsize/2;
	if ( GGadgetIsChecked(GWidgetGetControl(kcd->gw,CID_R2L)) ) {
	    if ( kcd->ssc!=NULL ) {
		KCD_DrawGlyph(pixmap,x,baseline,kcd->ssc,kcd->magfactor);
		x += kcd->magfactor*kcd->ssc->width + kern;
	    }
	    if ( kcd->fsc!=NULL )
		KCD_DrawGlyph(pixmap,x,baseline,kcd->fsc,kcd->magfactor);
	} else {
	    if ( kcd->fsc!=NULL ) {
		KCD_DrawGlyph(pixmap,x,baseline,kcd->fsc,kcd->magfactor);
		x += kcd->fsc->width*kcd->magfactor + kern;
	    }
	    if ( kcd->ssc!=NULL )
		KCD_DrawGlyph(pixmap,x,baseline,kcd->ssc,kcd->magfactor);
	}
    } else {
	/* I don't support top to bottom vertical */
	y = kcd->top + kcd->magfactor*kcd->pixelsize/3 + as;
	xbaseline = kcd->fullwidth/2;
	if ( kcd->fsc!=NULL ) {
	    KCD_DrawGlyph(pixmap,xbaseline-kcd->magfactor*kcd->pixelsize/2,y,kcd->fsc,kcd->magfactor);
	    y += kcd->magfactor*rint(kcd->fsc->sc->vwidth * kcd->pixelsize/(double) em) + kern;
	}
	if ( kcd->ssc!=NULL )
	    KCD_DrawGlyph(pixmap,xbaseline-kcd->magfactor*kcd->pixelsize/2,y,kcd->ssc,kcd->magfactor);
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
    BDFChar **scpos = which==0 ? &kcd->fsc : &kcd->ssc;
    SplineChar **possc = which==0 ? &kcd->scf : &kcd->scs;
    SplineChar *sc;
    char *temp;
    void *freetypecontext=NULL;

    BDFCharFree(*scpos);
    *scpos = NULL;
    if ( kcd->iskernpair ) {
	temp = cu_copy(_GGadgetGetTitle(GWidgetGetControl(kcd->kw,
		which==0 ? CID_First : CID_Second )));
    } else {
	GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(kcd->kw,
		which==0 ? CID_First : CID_Second ));
	if ( sel==NULL )
return;
	temp = cu_copy(sel->text);
    }

    *possc = sc = SFGetChar(kcd->sf,-1,temp);
    free(temp);
    if ( sc==NULL )
return;
    if ( GGadgetIsChecked(GWidgetGetControl(kcd->gw,CID_FreeType)) )
	freetypecontext = FreeTypeFontContext(sc->parent,sc,sc->parent->fv);
    if ( freetypecontext ) {
	*scpos = SplineCharFreeTypeRasterize(freetypecontext,sc->orig_pos,kcd->pixelsize,8);
	FreeTypeFreeContext(freetypecontext);
    } else
	*scpos = SplineCharAntiAlias(sc,kcd->pixelsize,4);
}

static int KCD_DisplaySizeChanged(GGadget *g, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_DisplaySize));
	unichar_t *end;
	int pixelsize = u_strtol(ret,&end,10);

	while ( *end==' ' ) ++end;
	if ( pixelsize>4 && pixelsize<400 && *end=='\0' ) {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    unichar_t ubuf[20]; char buffer[20];
	    ubuf[0] = '0'; ubuf[1] = '\0';
	    if ( kcd->active_adjust.corrections!=NULL &&
		    pixelsize>=kcd->active_adjust.first_pixel_size &&
		    pixelsize<=kcd->active_adjust.last_pixel_size ) {
		sprintf( buffer, "%d", kcd->active_adjust.corrections[
			pixelsize-kcd->active_adjust.first_pixel_size]);
		uc_strcpy(ubuf,buffer);
	    }
	    GGadgetSetTitle(GWidgetGetControl(kcd->gw,CID_Correction),ubuf);
#endif
	    kcd->pixelsize = pixelsize;
	    KCD_UpdateGlyph(kcd,0);
	    KCD_UpdateGlyph(kcd,1);
	    GDrawRequestExpose(kcd->kw,NULL,false);
	}
    }
return( true );
}

static int KCD_MagnificationChanged(GGadget *g, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	int mag = GGadgetGetFirstListSelectedItem(GWidgetGetControl(kcd->gw,CID_Magnifications));

	if ( mag!=-1 && mag!=kcd->magfactor-1 ) {
	    kcd->magfactor = mag+1;
	    GDrawRequestExpose(kcd->kw,NULL,false);
	}
    }
return( true );
}

static int KCB_FreeTypeChanged(GGadget *g, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	KCD_UpdateGlyph(kcd,0);
	KCD_UpdateGlyph(kcd,1);
	GDrawRequestExpose(kcd->kw,NULL,false);
    }
return( true );
}

#ifdef FONTFORGE_CONFIG_DEVICETABLES
static int KCD_CorrectionChanged(GGadget *g, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_Correction));
	unichar_t *end;
	int correction = u_strtol(ret,&end,10);

	while ( *end==' ' ) ++end;
	if ( *end!='\0' )
return( true );
	if ( correction<-128 || correction>127 ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    gwwv_post_error(_("Value out of range"),_("Value out of range"));
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error( _("Out of Range"), _("Corrections must be between -128 and 127 (and should be smaller)") );
#endif
return( true );
	}

	DeviceTableSet(&kcd->active_adjust,kcd->pixelsize,correction);
	GDrawRequestExpose(kcd->kw,NULL,false);
    }
return( true );
}
#endif

static void KPD_FinishKP(KernClassDlg *kcd) {
    KernPair *kp;
    int sli = GGadgetGetFirstListSelectedItem(GWidgetGetControl(kcd->gw,CID_SLI));
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_KernOffset));
    int offset = u_strtol(ret,NULL,10);

    if ( kcd->scf!=NULL && kcd->scs!=NULL ) {
	for ( kp = kcd->isv?kcd->scf->vkerns:kcd->scf->kerns; kp!=NULL && kp->sc!=kcd->scs; kp=kp->next );
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( kp==NULL && offset==0 && kcd->active_adjust.corrections==NULL )
return;
#else
	if ( kp==NULL && offset==0 )
return;
#endif
	if ( kp==NULL ) {
	    kp = chunkalloc(sizeof(KernPair));
	    kp->next = kcd->isv?kcd->scf->vkerns:kcd->scf->kerns;
	    kp->sc = kcd->scs;
	    if ( kcd->isv )
		kcd->scf->vkerns = kp;
	    else
		kcd->scf->kerns = kp;
	}
	kp->flags &= ~pst_r2l;
	if ( GGadgetIsChecked(GWidgetGetControl(kcd->gw,CID_R2L)))
	    kp->flags |= pst_r2l;
	kp->sli = sli;
	kp->off = offset;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( kp->adjust!=NULL && kcd->active_adjust.corrections!=NULL ) {
	    free(kp->adjust->corrections);
	    *kp->adjust = kcd->active_adjust;
	} else if ( kcd->active_adjust.corrections!=NULL ) {
	    kp->adjust = chunkalloc(sizeof(DeviceTable));
	    *kp->adjust = kcd->active_adjust;
	} else if ( kp->adjust!=NULL ) {
	    DeviceTableFree(kp->adjust);
	    kp->adjust = NULL;
	}
	memset(&kcd->active_adjust,0,sizeof(DeviceTable));
#endif
    }
}

#ifdef FONTFORGE_CONFIG_DEVICETABLES
static void KCD_SetDevTab(KernClassDlg *kcd) {
    unichar_t ubuf[20];

    ubuf[0] = '0'; ubuf[1] = '\0';
    GGadgetClearList(GWidgetGetControl(kcd->gw,CID_DisplaySize));
    if ( kcd->active_adjust.corrections!=NULL ) {
	int i;
	int len = kcd->active_adjust.last_pixel_size - kcd->active_adjust.first_pixel_size +1;
	char buffer[20];
	GTextInfo **ti = galloc((len+1)*sizeof(GTextInfo *));
	for ( i=0; i<len; ++i ) {
	    ti[i] = gcalloc(1,sizeof(GTextInfo));
	    sprintf( buffer, "%d", i+kcd->active_adjust.first_pixel_size);
	    ti[i]->text = uc_copy(buffer);
	    ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	}
	ti[i] = gcalloc(1,sizeof(GTextInfo));
	GGadgetSetList(GWidgetGetControl(kcd->gw,CID_DisplaySize),ti,false);
	if ( kcd->pixelsize>=kcd->active_adjust.first_pixel_size &&
		kcd->pixelsize<=kcd->active_adjust.last_pixel_size ) {
	    sprintf( buffer, "%d", kcd->active_adjust.corrections[
		    kcd->pixelsize-kcd->active_adjust.first_pixel_size]);
	    uc_strcpy(ubuf,buffer);
	}
    }
    GGadgetSetTitle(GWidgetGetControl(kcd->gw,CID_Correction),ubuf);
}
#endif

static void KPD_PairSearch(KernClassDlg *kcd) {
    int offset = 0;
    KernPair *kp=NULL;
    char buf[20];
    unichar_t ubuf[20];
    GGadget *slilist;

#ifdef FONTFORGE_CONFIG_DEVICETABLES
    free(kcd->active_adjust.corrections); kcd->active_adjust.corrections = NULL;
#endif
    if ( kcd->scf!=NULL && kcd->scs!=NULL ) {
	for ( kp = kcd->isv?kcd->scf->vkerns:kcd->scf->kerns; kp!=NULL && kp->sc!=kcd->scs; kp=kp->next );
	if ( kp!=NULL ) {
	    offset = kp->off;
	    GGadgetSelectOneListItem(GWidgetGetControl(kcd->gw,CID_SLI),kp->sli);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    if ( kp->adjust!=NULL ) {
		int len = kp->adjust->last_pixel_size-kp->adjust->first_pixel_size+1;
		kcd->active_adjust = *kp->adjust;
		kcd->active_adjust.corrections = galloc(len);
		memcpy(kcd->active_adjust.corrections,kp->adjust->corrections,len);
	    }
#endif
	}
    }
    slilist = GWidgetGetControl(kcd->gw,CID_SLI);
    if ( kp==NULL && kcd->scf!=NULL &&
	    !SCScriptInSLI(kcd->sf,kcd->scf,GGadgetGetFirstListSelectedItem(slilist)) ) {
	int sli_cnt = SLICount(kcd->sf);
	int sli = SCDefaultSLI(kcd->sf,kcd->scf);
	if ( SLICount(kcd->sf)!=sli_cnt )
	    GGadgetSetList(slilist,SFLangArray(kcd->sf,true),false);
	GGadgetSelectOneListItem(slilist,sli);
    }

    sprintf(buf, "%d", offset);
    uc_strcpy(ubuf,buf);
    GGadgetSetTitle(GWidgetGetControl(kcd->kw,CID_KernOffset),ubuf);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    KCD_SetDevTab(kcd);
#endif
}

static void KPD_BuildKernList(KernClassDlg *kcd) {
    int len;
    KernPair *kp;
    GTextInfo **ti;

    len = 0;
    if ( kcd->scf!=NULL )
	for ( kp=kcd->isv?kcd->scf->vkerns:kcd->scf->kerns, len=0; kp!=NULL; kp=kp->next )
	    ++len;
    ti = gcalloc(len+1,sizeof(GTextInfo*));
    if ( kcd->scf!=NULL )
	for ( kp=kcd->isv?kcd->scf->vkerns:kcd->scf->kerns, len=0; kp!=NULL; kp=kp->next, ++len ) {
	    ti[len] = gcalloc(1,sizeof(GTextInfo));
	    ti[len]->fg = ti[len]->bg = COLOR_DEFAULT;
	    ti[len]->text = uc_copy(kp->sc->name);
	}
    ti[len] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(GWidgetGetControl(kcd->gw,CID_Second),ti,false);

    if ( kcd->scf!=NULL )
	GGadgetSetChecked(GWidgetGetControl(kcd->gw,CID_R2L),SCRightToLeft(kcd->scf));
}

static int KCD_GlyphSelected(GGadget *g, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    int which = GGadgetGetCid(g)==CID_Second;
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	KCD_UpdateGlyph(kcd,which);
	GDrawRequestExpose(kcd->kw,NULL,false);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	KPD_FinishKP(kcd);
	KCD_UpdateGlyph(kcd,which);
	if ( which==0 )
	    KPD_BuildKernList(kcd);
	KPD_PairSearch(kcd);
	GDrawRequestExpose(kcd->kw,NULL,false);
    }
return( true );
}

static GTextInfo **TiNamesFromClass(GGadget *list,int class_index) {
    /* Return a list containing all the names in this class */
    unichar_t *upt, *end;
    unichar_t *line;
    GTextInfo *classti;
    GTextInfo **ti;
    int i, k;

    classti = GGadgetGetListItem(list,class_index);
    if ( classti==NULL || uc_strcmp(classti->text,_("{Everything Else}"))==0 ) {
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

    if ( second==0 )
	gwwv_post_notice(_("Class 0"),_("The kerning values for class 0 (\"Everything Else\") should always be 0"));
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

#ifdef FONTFORGE_CONFIG_DEVICETABLES
    kcd->active_adjust = kcd->adjusts[kcd->st_pos];
    if ( kcd->active_adjust.corrections!=NULL ) {
	int len = kcd->active_adjust.last_pixel_size - kcd->active_adjust.first_pixel_size +1;
	kcd->active_adjust.corrections = galloc(len);
	memcpy(kcd->active_adjust.corrections,kcd->adjusts[kcd->st_pos].corrections,len);
    }
    KCD_SetDevTab(kcd);
#endif

    GDrawSetVisible(kcd->kw,true);
}

/* ************************************************************************** */
/* *************************** Kern Class Dialog **************************** */
/* ************************************************************************** */

static int KC_Sli(GGadget *g, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    int sli;
    SplineFont *sf;

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	kcd = GDrawGetUserData(GGadgetGetWindow(g));

	sf = kcd->sf;
	if ( sf->mm!=NULL ) sf = sf->mm->normal;
	else if ( sf->cidmaster!=NULL ) sf=sf->cidmaster;

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

	sf = kcd->sf;
	if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
	else if ( sf->mm!=NULL ) sf = sf->mm->normal;

	if ( GDrawIsVisible(kcd->cw))
return( KCD_Next(g,e));
	else if ( GDrawIsVisible(kcd->kw))
return( KCD_Next2(g,e));

	if ( sf->script_lang==NULL || sli<0 ||
		sf->script_lang[sli]==NULL ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    gwwv_post_error(_("Please select a script"),_("Please select a script"));
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Please select a script"),_("Please select a script"));
#endif
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
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    free(kc->adjusts);
#endif
	}
	kc->first_cnt = kcd->first_cnt;
	kc->second_cnt = kcd->second_cnt;
	kc->firsts = galloc(kc->first_cnt*sizeof(char *));
	kc->seconds = galloc(kc->second_cnt*sizeof(char *));
	kc->firsts[0] = kc->seconds[0] = NULL;
	ti = GGadgetGetList(GWidgetGetControl(kcd->gw,CID_ClassList),&len);
	if ( uc_strcmp(ti[0]->text,_("{Everything Else}"))!=0 )
	    kc->firsts[0] = cu_copy(ti[0]->text);
	for ( i=1; i<kc->first_cnt; ++i )
	    kc->firsts[i] = cu_copy(ti[i]->text);
	ti = GGadgetGetList(GWidgetGetControl(kcd->gw,CID_ClassList+100),&len);
	for ( i=1; i<kc->second_cnt; ++i )
	    kc->seconds[i] = cu_copy(ti[i]->text);
	kc->offsets = kcd->offsets;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	kc->adjusts = kcd->adjusts;
#endif
	kc->sli = sli;
	kc->flags = 0;
	for ( i=0; flag_cid[i]!=0; ++i )
	    if ( GGadgetIsChecked(GWidgetGetControl(kcd->gw,flag_cid[i])))
		kc->flags |= flag_bits[i];
	kcd->sf->changed = true;
	sf->changed = true;

	GDrawDestroyWindow(kcd->gw);
    }
return( true );
}

static void KC_DoCancel(KernClassDlg *kcd) {
    if ( kcd->iskernpair )
	KPD_DoCancel(kcd);
    else {
	free(kcd->offsets);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	{ int i;
	    for ( i=0; i<kcd->first_cnt*kcd->second_cnt; ++i )
		free(kcd->adjusts[i].corrections);
	}
	free(kcd->adjusts);
#endif
	GDrawDestroyWindow(kcd->gw);
    }
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
    for ( j=len-1; j>i; --j )
	if ( ti[j]->selected )
    break;
    GGadgetSetEnabled(GWidgetGetControl(kcd->gw,CID_ClassEdit+off),(i>=1 || off==0) && j==i);
    GGadgetSetEnabled(GWidgetGetControl(kcd->gw,CID_ClassUp+off),i>=2);
    GGadgetSetEnabled(GWidgetGetControl(kcd->gw,CID_ClassDown+off),i>=1 && j<len-1);
}

static void OffsetMoveClasses(KernClassDlg *kcd, int dir, GTextInfo **movethese, int off ) {
    int16 *new;
    int i,j,k;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    DeviceTable *newd;
#endif

    movethese[0]->selected = false;

    new = galloc(kcd->first_cnt*kcd->second_cnt*sizeof(int16));
    memcpy(new,kcd->offsets,kcd->first_cnt*kcd->second_cnt*sizeof(int16));
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    newd = galloc(kcd->first_cnt*kcd->second_cnt*sizeof(DeviceTable));
    memcpy(newd,kcd->adjusts,kcd->first_cnt*kcd->second_cnt*sizeof(DeviceTable));
#endif
    if ( off==0 ) {
	for ( i=1; i<kcd->first_cnt; ++i ) {
	    if ( movethese[i]->selected ) {
		for ( k=i; k<kcd->first_cnt && movethese[k]->selected; ++k );
		memcpy(new+(i+dir)*kcd->second_cnt,kcd->offsets+i*kcd->second_cnt,
			(k-i)*kcd->second_cnt*sizeof(int16));
		if ( dir>0 )
		    memcpy(new+i*kcd->second_cnt,kcd->offsets+k*kcd->second_cnt,
			    kcd->second_cnt*sizeof(int16));
		else
		    memcpy(new+(k-1)*kcd->second_cnt,kcd->offsets+(i-1)*kcd->second_cnt,
			    kcd->second_cnt*sizeof(int16));
#ifndef FONTFORGE_CONFIG_DEVICETABLES
		memcpy(newd+(i+dir)*kcd->second_cnt,kcd->adjusts+i*kcd->second_cnt,
			(k-i)*kcd->second_cnt*sizeof(DeviceTable));
		if ( dir>0 )
		    memcpy(newd+i*kcd->second_cnt,kcd->adjusts+k*kcd->second_cnt,
			    kcd->second_cnt*sizeof(DeviceTable));
		else
		    memcpy(newd+(k-1)*kcd->second_cnt,kcd->adjusts+(i-1)*kcd->second_cnt,
			    kcd->second_cnt*sizeof(DeviceTable));
#endif
		i=k-1;
	    }
	}
    } else {
	for ( i=0; i<kcd->first_cnt; ++i ) {
	    for ( j=0; j<kcd->second_cnt; ++j ) {
		if ( movethese[j]->selected ) {
		    for ( k=j; k<kcd->second_cnt && movethese[k]->selected; ++k );
		    memcpy(new+i*kcd->second_cnt+(j+dir),kcd->offsets+i*kcd->second_cnt+j,
			    (k-j)*sizeof(int16));
		    if ( dir>0 )
			new[i*kcd->second_cnt+j] = kcd->offsets[i*kcd->second_cnt+k];
		    else
			new[i*kcd->second_cnt+k-1] = kcd->offsets[i*kcd->second_cnt+j-1];
#ifndef FONTFORGE_CONFIG_DEVICETABLES
		    memcpy(newd+i*kcd->second_cnt+(j+dir),kcd->offsets+i*kcd->second_cnt+j,
			    (k-j)*sizeof(DeviceTable));
		    if ( dir>0 )
			newd[i*kcd->second_cnt+j] = kcd->adjusts[i*kcd->second_cnt+k];
		    else
			newd[i*kcd->second_cnt+k-1] = kcd->adjusts[i*kcd->second_cnt+j-1];
#endif
		    j = k-1;
		}
	    }
	}
    }

    free(kcd->offsets);
    kcd->offsets = new;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    free(kcd->adjusts);
    kcd->adjusts = newd;
#endif
}

static int KCD_Up(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_ClassUp;
	GGadget *list = GWidgetGetControl(kcd->gw,CID_ClassList+off);
	int32 len;

	OffsetMoveClasses(kcd,-1,GGadgetGetList(list,&len),off);
	GListMoveSelected(list,-1);
	_KCD_EnableButtons(kcd,off);
	KCD_SBReset(kcd);
	GDrawRequestExpose(kcd->gw,NULL,false);
    }
return( true );
}

static int KCD_Down(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_ClassDown;
	GGadget *list = GWidgetGetControl(kcd->gw,CID_ClassList+off);
	int32 len;

	OffsetMoveClasses(kcd,1,GGadgetGetList(list,&len),off);
	GListMoveSelected(list,1);
	_KCD_EnableButtons(kcd,off);
	KCD_SBReset(kcd);
	GDrawRequestExpose(kcd->gw,NULL,false);
    }
return( true );
}

static void OffsetRemoveClasses(KernClassDlg *kcd, GTextInfo **removethese, int off ) {
    int16 *new;
    int i,j,k, remove_cnt;
    int old_cnt = off==0 ? kcd->first_cnt : kcd->second_cnt;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    DeviceTable *newd;
#endif

    removethese[0]->selected = false;
    for ( remove_cnt=i=0; i<old_cnt; ++i )
	if ( removethese[i]->selected )
	    ++remove_cnt;
    if ( remove_cnt==0 )
return;

    if ( off==0 ) {
	new = galloc((kcd->first_cnt-remove_cnt)*kcd->second_cnt*sizeof(int16));
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	newd = galloc((kcd->first_cnt-remove_cnt)*kcd->second_cnt*sizeof(DeviceTable));
#endif
	for ( i=k=0; i<kcd->first_cnt; ++i ) {
#ifndef FONTFORGE_CONFIG_DEVICETABLES
	    if ( !removethese[i]->selected )
		memcpy(new+(k++)*kcd->second_cnt,kcd->offsets+i*kcd->second_cnt,kcd->second_cnt*sizeof(int16));
#else
	    if ( !removethese[i]->selected ) {
		memcpy(new+k*kcd->second_cnt,kcd->offsets+i*kcd->second_cnt,kcd->second_cnt*sizeof(int16));
		memcpy(newd+(k++)*kcd->second_cnt,kcd->adjusts+i*kcd->second_cnt,kcd->second_cnt*sizeof(DeviceTable));
	    } else {
		for ( j=0; j<kcd->second_cnt; ++j )
		    free(kcd->adjusts[i*kcd->second_cnt+j].corrections);
	    }
#endif
	}
	kcd->first_cnt = k;
    } else {
	new = galloc(kcd->first_cnt*(kcd->second_cnt-remove_cnt)*sizeof(int16));
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	newd = galloc(kcd->first_cnt*(kcd->second_cnt-remove_cnt)*sizeof(DeviceTable));
#endif
	for ( i=0; i<kcd->first_cnt; ++i ) {
	    for ( j=k=0; j<kcd->second_cnt; ++j ) {
#ifndef FONTFORGE_CONFIG_DEVICETABLES
		if ( !removethese[j]->selected )
		    new[i*(kcd->second_cnt-remove_cnt)+(k++)] = kcd->offsets[i*kcd->second_cnt+j];
#else
		if ( !removethese[j]->selected ) {
		    new[i*(kcd->second_cnt-remove_cnt)+k] = kcd->offsets[i*kcd->second_cnt+j];
		    newd[i*(kcd->second_cnt-remove_cnt)+(k++)] = kcd->adjusts[i*kcd->second_cnt+j];
		} else
		    free(kcd->adjusts[i*kcd->second_cnt+j].corrections);
#endif
	    }
	}
	kcd->second_cnt -= remove_cnt;
    }

    free(kcd->offsets);
    kcd->offsets = new;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    free(kcd->adjusts);
    kcd->adjusts = newd;
#endif
}

static int KCD_Delete(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_ClassDel;
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
	/* Class 0 can contain something real on the first character but not the second */
	if ( off==0 || GGadgetGetFirstListSelectedItem(g)>0 )
	    _KCD_DoEditNew(kcd,true,off);
    }
return( true );
}

static int KCD_TextSelect(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_ClassSelect;
	const unichar_t *name = _GGadgetGetTitle(g);
	GGadget *list = GWidgetGetControl(kcd->gw,CID_ClassList+off);
	int32 len;
	GTextInfo **ti = GGadgetGetList(list,&len);
	int nlen=u_strlen(name);
	unichar_t *start, *pt;
	int i;

	for ( i=0; i<len; ++i ) {
	    for ( start = ti[i]->text; start!=NULL && *start!='\0'; ) {
		while ( *start==' ' ) ++start;
		for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
		if ( pt-start == nlen && u_strncmp(name,start,nlen)==0 ) {
		    GGadgetSelectOneListItem(list,i);
		    GGadgetScrollListToPos(list,i);
		    _KCD_EnableButtons(kcd,off);
		    if ( off==0 )
			KCD_VShow(kcd,i);
		    else
			KCD_HShow(kcd,i);
return( true );
		}
		start = pt;
	    }
	}

	/* Otherwise deselect everything */
	if ( nlen!=0 )
	    for ( i=0; i<len; ++i )
		GGadgetSelectListItem(list,i,false);
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
    
    if ( event->u.mouse.x<kcd->xstart || event->u.mouse.x>kcd->xstart2+kcd->fullwidth ||
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
    GRect rect, select;
    GRect clip,old1,old2;
    int len, off, i, j, x, y;
    unichar_t ubuf[8];
    char buf[100];
    int32 tilen;
    GTextInfo **ti;

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

    /* In the offsets list, show which classes are selected above in the class*/
    /*  lists */
    ti = GGadgetGetList(GWidgetGetControl(kcd->gw,CID_ClassList),&tilen);
    for ( i=0 ; kcd->offtop+i<=kcd->first_cnt && (i-1)*kcd->kernh<kcd->height; ++i ) {
	if ( i+kcd->offtop<tilen && ti[i+kcd->offtop]->selected ) {
	    select.x = kcd->xstart+1; select.y = kcd->ystart2+i*kcd->kernh+1;
	    select.width = rect.width-1; select.height = kcd->kernh-1;
	    GDrawFillRect(pixmap,&select,0xffff00);
	}
    }
    ti = GGadgetGetList(GWidgetGetControl(kcd->gw,CID_ClassList+100),&tilen);
    for ( i=0 ; kcd->offleft+i<=kcd->second_cnt && (i-1)*kcd->kernw<kcd->fullwidth; ++i ) {
	if ( i+kcd->offleft<tilen && ti[i+kcd->offleft]->selected ) {
	    select.x = kcd->xstart2+i*kcd->kernw+1; select.y = kcd->ystart+1;
	    select.width = kcd->kernw-1; select.height = rect.height-1;
	    GDrawFillRect(pixmap,&select,0xffff00);
	}
    }

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
    for ( i=0 ; kcd->offleft+i<=kcd->second_cnt && (i-1)*kcd->kernw<kcd->fullwidth; ++i ) {
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
	for ( j=0 ; kcd->offleft+j<kcd->second_cnt && (j-1)*kcd->kernw<kcd->fullwidth; ++j ) {
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

    if ( kcd->height>=kcd->kernh )
	GScrollBarSetBounds(kcd->vsb,0,kcd->first_cnt, kcd->height/kcd->kernh);
    if ( kcd->width>=kcd->kernw )
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
    if ( pos>=0 && pos<kcd->second_cnt ) {
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
    }
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
    if ( pos>=0 && pos<kcd->first_cnt ) {
#if 0
	if ( pos>=kcd->offtop && pos<kcd->offtop+(kcd->height/kcd->kernh) )
return;		/* Already visible */
#endif
	--pos;	/* One line of context */
	if ( pos + (kcd->height/kcd->kernh) >= kcd->first_cnt )
	    pos = kcd->first_cnt - (kcd->height/kcd->kernh);
	if ( pos < 0 ) pos = 0;
	kcd->offtop = pos;
	GScrollBarSetPos(kcd->vsb,pos);
    }
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

static void KCD_Drop(KernClassDlg *kcd, GEvent *event) {
    DropChars2Text(kcd->gw,GWidgetGetControl(kcd->cw,CID_GlyphList),event);
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
      case et_drop:
	KCD_Drop(kcd,event);
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
	    help(kcd->iskernpair ?  "metricsview.html#kernpair":
				    "metricsview.html#kernclass");
return( true );
	}
return( false );
      break;
      case et_destroy:
	if ( kcd!=NULL && kcd->kcld!=NULL ) {
	    kcd->kcld->kcd = NULL;
	    GGadgetSetList(GWidgetGetControl(kcd->kcld->gw,CID_List),
		    KCSLIArray(kcd->sf,kcd->isv),false);
	    GGadgetSetEnabled(GWidgetGetControl(kcd->kcld->gw,CID_New),true);
	    free(kcd);
	}
      break;
      case et_mouseup: case et_mousemove: case et_mousedown:
	if ( kcd->iskernpair )
	    KCD_KernMouse(kcd,event);
	else
	    KCD_Mouse(kcd,event);
      break;
      case et_expose:
	if ( kcd->iskernpair )
	    KCD_KernExpose(kcd,gw,event);
	else
	    KCD_Expose(kcd,gw,event);
      break;
      case et_resize: {
	GRect wsize, csize;
	int subwidth, offset;
	static int moveme[] = { CID_ClassList+100, CID_ClassNew+100,
		CID_ClassDel+100, CID_ClassEdit+100, CID_ClassLabel+100,
		CID_ClassUp+100, CID_ClassDown+100, 0};
	int i;

	GDrawGetSize(kcd->gw,&wsize);
	GGadgetGetSize(GWidgetGetControl(kcd->gw,CID_Next2),&csize);

	GGadgetMove(GWidgetGetControl(kcd->gw,CID_Prev2),15,wsize.height-kcd->canceldrop);
	GGadgetMove(GWidgetGetControl(kcd->gw,CID_Next2),wsize.width-csize.width-15,wsize.height-kcd->canceldrop);
	GGadgetResize(GWidgetGetControl(kcd->gw,CID_Group3),wsize.width-4,wsize.height-4);

	kcd->fullwidth = wsize.width;
	if ( kcd->cw!=NULL ) {
	    GDrawResize(kcd->cw,wsize.width,wsize.height);
	    GDrawResize(kcd->kw,wsize.width,wsize.height);
	    GGadgetResize(GWidgetGetControl(kcd->gw,CID_Group),wsize.width-4,wsize.height-4);
	    GGadgetResize(GWidgetGetControl(kcd->gw,CID_Group2),wsize.width-4,wsize.height-4);
	    GGadgetResize(GWidgetGetControl(kcd->gw,CID_Line2),wsize.width-20,3);
	    GGadgetResize(GWidgetGetControl(kcd->gw,CID_Line1),wsize.width-20,3);

	    GGadgetMove(GWidgetGetControl(kcd->gw,CID_OK),15,wsize.height-kcd->canceldrop-3);
	    GGadgetMove(GWidgetGetControl(kcd->gw,CID_Prev),15,wsize.height-kcd->canceldrop);
	    GGadgetGetSize(GWidgetGetControl(kcd->gw,CID_Cancel),&csize);
	    GGadgetMove(GWidgetGetControl(kcd->gw,CID_Cancel),wsize.width-csize.width-15,wsize.height-kcd->canceldrop);
	    GGadgetMove(GWidgetGetControl(kcd->gw,CID_Next),wsize.width-csize.width-15,wsize.height-kcd->canceldrop);

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
	    GDrawRequestExpose(kcd->kw,NULL,false);
	    GDrawRequestExpose(kcd->cw,NULL,false);

	    GGadgetGetSize(GWidgetGetControl(kcd->cw,CID_GlyphList),&csize);
	    GGadgetResize(GWidgetGetControl(kcd->gw,CID_GlyphList),wsize.width-20,csize.height);
	} else {
	    kcd->width = wsize.width-kcd->xstart2-5;
	    kcd->height = wsize.height-kcd->ystart2;
	    GGadgetGetSize(GWidgetGetControl(kcd->gw,CID_SLI),&csize);
	    GGadgetResize(GWidgetGetControl(kcd->gw,CID_SLI),wsize.width-24,csize.height);
	    GGadgetMove(GWidgetGetControl(kcd->gw,CID_SLI),12,wsize.height-kcd->canceldrop-csize.height-3);
	}
	GDrawRequestExpose(kcd->gw,NULL,false);
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

    label[k].text = (unichar_t *) (x<20?_("First Char"):_("Second Char"));
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = x+10; gcd[k].gd.pos.y = y;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_ClassLabel+off;
    gcd[k++].creator = GLabelCreate;

    label[k].image = &GIcon_up;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = x+100; gcd[k].gd.pos.y = y-3;
    gcd[k].gd.pos.height = 19;
    gcd[k].gd.flags = gg_visible|gg_utf8_popup;
    gcd[k].gd.handle_controlevent = KCD_Up;
    gcd[k].gd.popup_msg = (unichar_t *) _("Move selected class up");
    gcd[k].gd.cid = CID_ClassUp+off;
    gcd[k++].creator = GButtonCreate;

    label[k].image = &GIcon_down;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = x+120; gcd[k].gd.pos.y = y-3;
    gcd[k].gd.pos.height = 19;
    gcd[k].gd.flags = gg_visible|gg_utf8_popup;
    gcd[k].gd.handle_controlevent = KCD_Down;
    gcd[k].gd.popup_msg = (unichar_t *) _("Move selected class down");
    gcd[k].gd.cid = CID_ClassDown+off;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = x; gcd[k].gd.pos.y = y+17;
    gcd[k].gd.pos.width = width;
    gcd[k].gd.pos.height = 8*12+10;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_list_multiplesel;
    gcd[k].gd.handle_controlevent = KCD_ClassSelected;
    gcd[k].gd.cid = CID_ClassList+off;
    gcd[k++].creator = GListCreate;

    label[k].text = (unichar_t *) _("New...");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+gcd[k-1].gd.pos.height+10;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.handle_controlevent = KCD_New;
    gcd[k].gd.cid = CID_ClassNew+off;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("Edit...");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = x+blen+space; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = KCD_Edit;
    gcd[k].gd.cid = CID_ClassEdit+off;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("Delete");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+blen+space; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.handle_controlevent = KCD_Delete;
    gcd[k].gd.cid = CID_ClassDel+off;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("Select Glyph Class:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-3].gd.pos.x+5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+26+4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("Select the class containing the named glyph");
    gcd[k++].creator = GLabelCreate;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+100; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 80;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("Select the class containing the named glyph");
    gcd[k].gd.handle_controlevent = KCD_TextSelect;
    gcd[k].gd.cid = CID_ClassSelect+off;
    gcd[k++].creator = GTextFieldCreate;
return( k );
}

static void FillShowKerningWindow(KernClassDlg *kcd, int for_class) {
    GGadgetCreateData gcd[30];
    GTextInfo label[30];
    int k;
    char buffer[20];
    GRect pos;

    kcd->top = GDrawPointsToPixels(kcd->gw,100);
    kcd->pixelsize = 150;
    kcd->magfactor = 1;

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    k = 0;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = 120;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.handle_controlevent = KCD_GlyphSelected;
    gcd[k].gd.cid = CID_First;
    gcd[k++].creator = for_class ? GListButtonCreate : GTextFieldCreate;

    gcd[k].gd.pos.x = 130; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = 120;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    if ( !for_class ) gcd[k].gd.flags |= gg_list_alphabetic;
    gcd[k].gd.handle_controlevent = KCD_GlyphSelected;
    gcd[k].gd.cid = CID_Second;
    gcd[k++].creator = for_class ? GListButtonCreate : GListFieldCreate;

    label[k].text = (unichar_t *) _("Use FreeType");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 260; gcd[k].gd.pos.y = 7;
    if ( !hasFreeType() )
	gcd[k].gd.flags = gg_visible;
    else
	gcd[k].gd.flags = gg_enabled|gg_visible|gg_cb_on;
    gcd[k].gd.cid = CID_FreeType;
    gcd[k].gd.handle_controlevent = KCB_FreeTypeChanged;
    gcd[k++].creator = GCheckBoxCreate;

    label[k].text = (unichar_t *) _("Display Size:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k++].creator = GLabelCreate;

    sprintf( buffer, "%d", kcd->pixelsize );
    label[k].text = (unichar_t *) buffer;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 92; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 80;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_DisplaySize;
    gcd[k].gd.handle_controlevent = KCD_DisplaySizeChanged;
#ifndef FONTFORGE_CONFIG_DEVICETABLES
    gcd[k++].creator = GTextFieldCreate;
#else
    gcd[k++].creator = GListFieldCreate;
#endif

    label[k].text = (unichar_t *) _("Magnification:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 185; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k++].creator = GLabelCreate;

#ifndef FONTFORGE_CONFIG_DEVICETABLES
    gcd[k].gd.pos.x = 255; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
#else
    gcd[k].gd.pos.x = 305; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
#endif
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_Magnifications;
    gcd[k].gd.u.list = magnifications;
    gcd[k].gd.handle_controlevent = KCD_MagnificationChanged;
    gcd[k++].creator = GListButtonCreate;

    label[k].text = (unichar_t *) _("Kern Offset:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k++].creator = GLabelCreate;

    gcd[k].gd.pos.x = 90; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_KernOffset;
    gcd[k].gd.handle_controlevent = KCD_KernOffChanged;
    gcd[k++].creator = GTextFieldCreate;

#ifdef FONTFORGE_CONFIG_DEVICETABLES
    label[k].text = (unichar_t *) _("Device Table Correction:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 185; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) "0";
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 305; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_Correction;
    gcd[k].gd.handle_controlevent = KCD_CorrectionChanged;
    gcd[k++].creator = GTextFieldCreate;
#endif

    GDrawGetSize(kcd->kw,&pos);

    if ( !for_class ) {
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = -40;
	gcd[k].gd.flags = gg_enabled ;
	label[k].text = (unichar_t *) _("Right To Left");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.cid = CID_R2L;
	gcd[k++].creator = GCheckBoxCreate;

	gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = KC_Height-KC_CANCELDROP-27;
	gcd[k].gd.pos.width = GDrawPixelsToPoints(kcd->kw,pos.width)-20;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.u.list = SFLangList(kcd->sf,true,(SplineChar *) -1);
	gcd[k].gd.cid = CID_SLI;
	gcd[k].gd.handle_controlevent = KC_Sli;
	gcd[k++].creator = GListButtonCreate;
    }

    label[k].text = (unichar_t *) (for_class ? _("< _Prev") : _("_OK"));
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30; gcd[k].gd.pos.y = KC_Height-KC_CANCELDROP;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled;
    if ( !for_class ) gcd[k].gd.flags |= gg_but_default;
    gcd[k].gd.handle_controlevent = for_class ? KCD_Prev2 : KPD_OK;
    gcd[k].gd.cid = CID_Prev2;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) (for_class ? _("_Next >") : _("_Cancel"));
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = -30+3; gcd[k].gd.pos.y = KC_Height-KC_CANCELDROP;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    if ( !for_class ) gcd[k].gd.flags |= gg_but_cancel;
    gcd[k].gd.handle_controlevent = for_class ? KCD_Next2 : KPD_Cancel;
    gcd[k].gd.cid = CID_Next2;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width-4;
    gcd[k].gd.pos.height = pos.height-4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k].gd.cid = CID_Group3;
    gcd[k++].creator = GGroupCreate;

    GGadgetsCreate(kcd->kw,gcd);
}

static void KernClassD(KernClassListDlg *kcld,KernClass *kc) {
    GRect pos, subpos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[38];
    GTextInfo label[38];
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
    kcd->sf = kcld->sf;
    kcd->isv = kcld->isv;
    if ( kc==NULL ) {
	kcd->first_cnt = kcd->second_cnt = 1;
	kcd->offsets = gcalloc(1,sizeof(int16));
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	kcd->adjusts = gcalloc(1,sizeof(DeviceTable));
#endif
    } else {
	kcd->first_cnt = kc->first_cnt;
	kcd->second_cnt = kc->second_cnt;
	kcd->offsets = galloc(kc->first_cnt*kc->second_cnt*sizeof(int16));
	memcpy(kcd->offsets,kc->offsets,kc->first_cnt*kc->second_cnt*sizeof(int16));
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	kcd->adjusts = galloc(kc->first_cnt*kc->second_cnt*sizeof(DeviceTable));
	memcpy(kcd->adjusts,kc->adjusts,kc->first_cnt*kc->second_cnt*sizeof(DeviceTable));
	for ( i=0; i<kcd->first_cnt*kcd->second_cnt; ++i ) {
	    if ( kcd->adjusts[i].corrections!=NULL ) {
		int len = kcd->adjusts[i].last_pixel_size-kcd->adjusts[i].first_pixel_size+1;
		kcd->adjusts[i].corrections = galloc(len);
		memcpy(kcd->adjusts[i].corrections,kc->adjusts[i].corrections,len);
	    }
	}
#endif
    }

    memset(&wattrs,0,sizeof(wattrs));
    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));

    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = false;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title =  _("Kerning Class");
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
    label[i].text = (unichar_t *) _("Right To Left");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_R2L;
    gcd[i++].creator = GCheckBoxCreate;

    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15;
    gcd[i].gd.flags = gg_visible | gg_enabled | (kc!=NULL && (kc->flags&pst_ignorebaseglyphs)?gg_cb_on:0);
    label[i].text = (unichar_t *) _("Ignore Base Glyphs");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_IgnBase;
    gcd[i++].creator = GCheckBoxCreate;

    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15;
    gcd[i].gd.flags = gg_visible | gg_enabled | (kc!=NULL && (kc->flags&pst_ignoreligatures)?gg_cb_on:0);
    label[i].text = (unichar_t *) _("Ignore Ligatures");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.cid = CID_IgnLig;
    gcd[i++].creator = GCheckBoxCreate;

    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15;
    gcd[i].gd.flags = gg_visible | gg_enabled | (kc!=NULL && (kc->flags&pst_ignorecombiningmarks)?gg_cb_on:0);
    label[i].text = (unichar_t *) _("Ignore Combining Marks");
    label[i].text_is_1byte = true;
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

    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = GDrawPointsToPixels(gw,gcd[i-1].gd.pos.y+27);
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
    label[i].text = (unichar_t *) _("_OK");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = KC_OK;
    gcd[i].gd.cid = CID_OK;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = -10; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _("_Cancel");
    label[i].text_is_1byte = true;
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
	if ( i==0 && kcd->orig!=NULL && kcd->orig->firsts[0]!=NULL ) {
	    /* OpenType can set class 0 of the first classes by using a coverage*/
	    /*  table with more glyphs than are present in all the other classes */
	    unichar_t *temp = uc_copy(kcd->orig->firsts[0]);
	    GListAppendLine(list,temp,false);
	    free(temp);
	} else
	    GListAppendLine8(list,_("{Everything Else}"),false);
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

    label[k].text = (unichar_t *) _("Set");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.popup_msg = (unichar_t *) _("Set this glyph list to be the characters selected in the fontview");
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.handle_controlevent = KCD_FromSelection;
    gcd[k].gd.cid = CID_Set;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("Select");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 70; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.popup_msg = (unichar_t *) _("Set the fontview's selection to be the characters named here");
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.handle_controlevent = KCD_ToSelection;
    gcd[k].gd.cid = CID_Select;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 30;
    gcd[k].gd.pos.width = KC_Width-25; gcd[k].gd.pos.height = 8*13+4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
    gcd[k].gd.cid = CID_GlyphList;
    gcd[k++].creator = GTextAreaCreate;

    label[k].text = (unichar_t *) _("< _Prev");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30; gcd[k].gd.pos.y = KC_Height-KC_CANCELDROP;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled /*| gg_but_cancel*/;
    gcd[k].gd.handle_controlevent = KCD_Prev;
    gcd[k].gd.cid = CID_Prev;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Next >");
    label[k].text_is_1byte = true;
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
    FillShowKerningWindow(kcd, true);

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
    int kcl_width = KCL_Width, temp;

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

    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = false;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title =  isv?_("VKern By Classes..."):_("Kern By Classes...");
    wattrs.is_dlg = false;
    pos.x = pos.y = 0;
    temp = 40 + 300*GIntGetResource(_NUM_Buttonsize)/GGadgetScale(100);
    if ( kcl_width<temp ) kcl_width = temp;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,kcl_width));
    pos.height = GDrawPointsToPixels(NULL,KCL_Height);
    kcld->gw = GDrawCreateTopWindow(NULL,&pos,kcl_e_h,kcld,&wattrs);

    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5;
    gcd[0].gd.pos.width = kcl_width-10; gcd[0].gd.pos.height = 7*12+10;
    gcd[0].gd.flags = gg_visible | gg_enabled | gg_list_multiplesel;
    gcd[0].gd.cid = CID_List;
    gcd[0].gd.u.list = KCSLIList(sf,isv);
    gcd[0].gd.handle_controlevent = KCL_SelChanged;
    gcd[0].creator = GListCreate;

    gcd[1].gd.pos.x = 10; gcd[1].gd.pos.y = gcd[0].gd.pos.y+gcd[0].gd.pos.height+4;
    gcd[1].gd.pos.width = -1;
    gcd[1].gd.flags = gg_visible | gg_enabled;
    label[1].text = (unichar_t *) _("_New...");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.cid = CID_New;
    gcd[1].gd.handle_controlevent = KCL_New;
    gcd[1].creator = GButtonCreate;

    gcd[2].gd.pos.x = 20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor); gcd[2].gd.pos.y = gcd[1].gd.pos.y;
    gcd[2].gd.pos.width = -1;
    gcd[2].gd.flags = gg_visible;
    label[2].text = (unichar_t *) _("_Delete");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.cid = CID_Delete;
    gcd[2].gd.handle_controlevent = KCL_Delete;
    gcd[2].creator = GButtonCreate;

    gcd[3].gd.pos.x = -10; gcd[3].gd.pos.y = gcd[1].gd.pos.y;
    gcd[3].gd.pos.width = -1;
    gcd[3].gd.flags = gg_visible;
    label[3].text = (unichar_t *) _("_Edit...");
    label[3].text_is_1byte = true;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.cid = CID_Edit;
    gcd[3].gd.handle_controlevent = KCL_Edit;
    gcd[3].creator = GButtonCreate;

    gcd[4].gd.pos.x = 10; gcd[4].gd.pos.y = gcd[1].gd.pos.y+28;
    gcd[4].gd.pos.width = kcl_width-20;
    gcd[4].gd.flags = gg_visible;
    gcd[4].creator = GLineCreate;

    gcd[5].gd.pos.x = (kcl_width-GIntGetResource(_NUM_Buttonsize))/2; gcd[5].gd.pos.y = gcd[4].gd.pos.y+7;
    gcd[5].gd.pos.width = -1;
    gcd[5].gd.flags = gg_visible|gg_enabled|gg_but_default|gg_but_cancel;
    label[5].text = (unichar_t *) _("_Done");
    label[5].text_is_1byte = true;
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

/* ************************************************************************** */
/* *************************** Kern Pair Dialog  **************************** */
/* ************************************************************************** */

void KernPairD(SplineFont *sf,SplineChar *sc1,SplineChar *sc2,int isv) {
    GRect pos;
    GWindowAttrs wattrs;
    KernClassDlg kcd;
    GWindow gw;
    int gid;

    if ( sc1==NULL ) {
	FontView *fv = sf->fv;
	int start = fv->rowoff*fv->colcnt, end = start+fv->rowcnt*fv->colcnt;
	int i;
	for ( i=start; i<end && i<fv->map->enccount; ++i )
	    if ( (gid=fv->map->map[i])!=-1 && sf->glyphs[gid]!=NULL &&
		    (isv ? sf->glyphs[gid]->vkerns : sf->glyphs[gid]->kerns)!=NULL )
	break;
	if ( i==end || i==fv->map->enccount ) {
	    for ( i=0; i<fv->map->enccount; ++i )
		if ( (gid=fv->map->map[i])!=-1 && sf->glyphs[gid]!=NULL &&
			(isv ? sf->glyphs[gid]->vkerns : sf->glyphs[gid]->kerns)!=NULL )
	    break;
	}
	if ( i==fv->map->enccount ) {
	    for ( i=start; i<end && i<fv->map->enccount; ++i )
		if ( (gid=fv->map->map[i])!=-1 && sf->glyphs[gid]!=NULL )
	    break;
	    if ( i==end || i==fv->map->enccount ) {
		for ( i=0; i<fv->map->enccount; ++i )
		    if ( (gid=fv->map->map[i])!=-1 && sf->glyphs[gid]!=NULL )
		break;
	    }
	}
	if ( i!=fv->map->enccount )
	    sc1 = sf->glyphs[gid];
    }
    if ( sc2==NULL && sc1!=NULL && (isv ? sc1->vkerns : sc1->kerns)!=NULL )
	sc2 = (isv ? sc1->vkerns : sc1->kerns)->sc;
    
    memset(&kcd,0,sizeof(kcd));
    kcd.sf = sf;
    kcd.scf = sc1;
    kcd.scs = sc2;
    kcd.isv = isv;
    kcd.iskernpair = true;

    memset(&wattrs,0,sizeof(wattrs));

    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = true;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Kern Pair Closeup...");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,KC_Width));
    pos.height = GDrawPointsToPixels(NULL,KC_Height);
    kcd.gw = gw = GDrawCreateTopWindow(NULL,&pos,kcd_e_h,&kcd,&wattrs);
    kcd.canceldrop = GDrawPointsToPixels(gw,KC_CANCELDROP);


    kcd.kw = gw;
    FillShowKerningWindow(&kcd, false);

    if ( sc1!=NULL ) {
	unichar_t *utemp;
	GGadgetSetTitle(GWidgetGetControl(kcd.gw,CID_First),(utemp=uc_copy(sc1->name)));
	free(utemp);
	KPD_BuildKernList(&kcd);
	KCD_UpdateGlyph(&kcd,0);
    }
    if ( sc2!=NULL ) {
	unichar_t *utemp;
	GGadgetSetTitle(GWidgetGetControl(kcd.gw,CID_Second),(utemp=uc_copy(sc2->name)));
	free(utemp);
	KCD_UpdateGlyph(&kcd,1);
	KPD_PairSearch(&kcd);
    }

    GDrawSetVisible(kcd.gw,true);
    while ( !kcd.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetUserData(kcd.gw,NULL);
    GDrawDestroyWindow(kcd.gw);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

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
