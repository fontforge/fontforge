/* Copyright (C) 2003-2009 by George Williams */
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
#include <gkeysym.h>
#include <string.h>
#include <ustring.h>
#include <utype.h>
#include <math.h>

typedef struct kernclassdlg {
    struct kernclasslistdlg *kcld;
    KernClass *orig;
    struct lookup_subtable *subtable;
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
    int layer;
    SplineChar *sc1, *sc2;
    int isv, iskernpair;
    SplineChar *scf, *scs;
    struct kernclassdlg *next;
} KernClassDlg;

typedef struct kernclasslistdlg {
    SplineFont *sf;
    int layer;
    GWindow gw;
    int isv;
} KernClassListDlg;

#define KCL_Width	200
#define KCL_Height	173
#define KC_Width	400
#define KC_Height	424
#define KC_CANCELDROP	33

#define CID_Subtable	1001

#define CID_List	1040
#define CID_New		1041
#define CID_Delete	1042
#define CID_Edit	1043

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

#define CID_Set		1020
#define CID_Select	1021
#define CID_GlyphList	1022
#define CID_Prev	1023
#define CID_Next	1024

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
#define CID_ClearDevice	1040

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

static GTextInfo **KCLookupSubtableArray(SplineFont *sf,int isv) {
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
	ti[cnt]->text = utf82u_copy(kc->subtable->subtable_name);
    }
    ti[cnt] = gcalloc(1,sizeof(GTextInfo));
return( ti );
}

static GTextInfo *KCLookupSubtableList(SplineFont *sf,int isv) {
    int cnt;
    KernClass *kc, *head = isv ? sf->vkerns : sf->kerns;
    GTextInfo *ti;

    if ( sf->cidmaster!=NULL ) sf=sf->cidmaster;
    else if ( sf->mm!=NULL ) sf = sf->mm->normal;

    for ( kc=head, cnt=0; kc!=NULL; kc=kc->next, ++cnt );
    ti = gcalloc(cnt+1,sizeof(GTextInfo));
    for ( kc=head, cnt=0; kc!=NULL; kc=kc->next, ++cnt )
	ti[cnt].text = utf82u_copy(kc->subtable->subtable_name);
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
	FontView *fv = (FontView *) sf->fv;
	const unichar_t *end;
	int pos, found=-1;
	char *nm;

	GDrawSetVisible(fv->gw,true);
	GDrawRaise(fv->gw);
	memset(fv->b.selected,0,fv->b.map->enccount);
	while ( *ret ) {
	    end = u_strchr(ret,' ');
	    if ( end==NULL ) end = ret+u_strlen(ret);
	    nm = cu_copybetween(ret,end);
	    for ( ret = end; isspace(*ret); ++ret);
	    if (( pos = SFFindSlot(sf,fv->b.map,-1,nm))!=-1 ) {
		if ( found==-1 ) found = pos;
		fv->b.selected[pos] = true;
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
	FontView *fv = (FontView *) sf->fv;
	unichar_t *vals, *pt;
	int i, len, max, gid;
	SplineChar *sc;
    
	for ( i=len=max=0; i<fv->b.map->enccount; ++i ) if ( fv->b.selected[i]) {
	    sc = SFMakeChar(sf,fv->b.map,i);
	    len += strlen(sc->name)+1;
	    if ( fv->b.selected[i]>max ) max = fv->b.selected[i];
	}
	pt = vals = galloc((len+1)*sizeof(unichar_t));
	*pt = '\0';
	/* in a class the order of selection is irrelevant */
	for ( i=0; i<fv->b.map->enccount; ++i ) if ( fv->b.selected[i] && (gid=fv->b.map->map[i])!=-1) {
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

static int KPD_FinishKP(KernClassDlg *);

static int KPD_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	if ( !KPD_FinishKP(kcd))
return( true );
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
	    ff_post_error( _("Bad Number"), _("Bad Number") );
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
	bg = GDrawGetDefaultBackground(NULL);
	fg = GDrawGetDefaultForeground(NULL);
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

static int KCD_RightToLeft(KernClassDlg *kcd) {
    if ( kcd->subtable!=NULL )
return( kcd->subtable->lookup->lookup_flags&pst_r2l );

    if ( kcd->scf!=NULL ) {
	uint32 script = SCScriptFromUnicode(kcd->scf);
	if ( script!=DEFAULT_SCRIPT )
return( ScriptIsRightToLeft( script ));
    }
    if ( kcd->scs!=NULL ) {
	uint32 script = SCScriptFromUnicode(kcd->scs);
	if ( script!=DEFAULT_SCRIPT )
return( ScriptIsRightToLeft( script ));
    }
return( false );
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

	if ( KCD_RightToLeft(kcd) ) {
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
	if ( KCD_RightToLeft(kcd) ) {
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
	freetypecontext = FreeTypeFontContext(sc->parent,sc,sc->parent->fv,kcd->layer);
    if ( freetypecontext ) {
	*scpos = SplineCharFreeTypeRasterize(freetypecontext,sc->orig_pos,kcd->pixelsize,72,8);
	FreeTypeFreeContext(freetypecontext);
    } else
	*scpos = SplineCharAntiAlias(sc,kcd->layer,kcd->pixelsize,4);
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
	    ff_post_error(_("Value out of range"),_("Value out of range"));
return( true );
	}

	DeviceTableSet(&kcd->active_adjust,kcd->pixelsize,correction);
	GDrawRequestExpose(kcd->kw,NULL,false);
	GGadgetSetEnabled(GWidgetGetControl(kcd->gw,CID_ClearDevice),
		kcd->active_adjust.corrections!=NULL);
    }
return( true );
}

static int KCD_ClearDevice(GGadget *g, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	free(kcd->active_adjust.corrections);
	kcd->active_adjust.corrections = NULL;
	kcd->active_adjust.first_pixel_size = kcd->active_adjust.last_pixel_size = 0;
	GGadgetSetTitle8(GWidgetGetControl(kcd->gw,CID_Correction),"0");
	GGadgetSetEnabled(g,false);
    }
return( true );
}
#endif

static void KPD_RestoreGlyphs(KernClassDlg *kcd) {
    if ( kcd->scf!=NULL )
	GGadgetSetTitle8(GWidgetGetControl(kcd->gw,CID_First),kcd->scf->name);
    if ( kcd->scs!=NULL )
	GGadgetSetTitle8(GWidgetGetControl(kcd->gw,CID_Second),kcd->scs->name);
}

static int KPD_FinishKP(KernClassDlg *kcd) {
    KernPair *kp;
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_KernOffset));
    int offset = u_strtol(ret,NULL,10);

    if ( kcd->scf!=NULL && kcd->scs!=NULL ) {
	for ( kp = kcd->isv?kcd->scf->vkerns:kcd->scf->kerns; kp!=NULL && kp->sc!=kcd->scs; kp=kp->next );
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( kp==NULL && offset==0 && kcd->active_adjust.corrections==NULL )
return(true);
#else
	if ( kp==NULL && offset==0 )
return(true);
#endif
	if ( kcd->subtable==NULL ) {
	    ff_post_error(_("No lookup selected"),_("You must select a lookup subtable to contain this kerning pair" ));
return(false);
	}
	if ( kp==NULL ) {
	    kp = chunkalloc(sizeof(KernPair));
	    kp->next = kcd->isv?kcd->scf->vkerns:kcd->scf->kerns;
	    kp->sc = kcd->scs;
	    if ( kcd->isv )
		kcd->scf->vkerns = kp;
	    else
		kcd->scf->kerns = kp;
	}
	kp->subtable = kcd->subtable;
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
return( true );
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
    GGadgetSetEnabled(GWidgetGetControl(kcd->gw,CID_ClearDevice),
	    kcd->active_adjust.corrections!=NULL);
}
#endif

static void KP_SelectSubtable(KernClassDlg *kcd,struct lookup_subtable *sub) {
    int32 len;
    GTextInfo **ti = GGadgetGetList(GWidgetGetControl(kcd->gw,CID_Subtable),&len);
    int i, new_pos = -1;

    for ( i=0; i<len; ++i ) if ( !ti[i]->line ) {
	if ( ti[i]->userdata == sub )
    break;
	else if ( ti[i]->userdata == NULL )
	    new_pos = i;
    }
    if ( i==len )
	i = new_pos;
    if ( i!=-1 )
	GGadgetSelectOneListItem(GWidgetGetControl(kcd->gw,CID_Subtable),i);
    if ( sub!=NULL )
	kcd->subtable = sub;
}

static int KP_Subtable(GGadget *g, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    GTextInfo *ti;
    struct lookup_subtable *sub;
    struct subtable_data sd;

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	ti = GGadgetGetListItemSelected(g);
	if ( ti!=NULL ) {
	    if ( ti->userdata!=NULL )
		kcd->subtable = ti->userdata;
	    else {
		memset(&sd,0,sizeof(sd));
		sd.flags = (kcd->isv ? sdf_verticalkern : sdf_horizontalkern ) |
			sdf_kernpair;
		sub = SFNewLookupSubtableOfType(kcd->sf,gpos_pair,&sd,kcd->layer);
		if ( sub!=NULL ) {
		    kcd->subtable = sub;
		    GGadgetSetList(g,SFSubtablesOfType(kcd->sf,gpos_pair,false,false),false);
		}
		KP_SelectSubtable(kcd,kcd->subtable);
	    }
	}
    }
return( true );
}

static void KPD_PairSearch(KernClassDlg *kcd) {
    int offset = 0;
    KernPair *kp=NULL;
    char buf[20];
    unichar_t ubuf[20];

#ifdef FONTFORGE_CONFIG_DEVICETABLES
    free(kcd->active_adjust.corrections); kcd->active_adjust.corrections = NULL;
#endif
    if ( kcd->scf!=NULL && kcd->scs!=NULL ) {
	for ( kp = kcd->isv?kcd->scf->vkerns:kcd->scf->kerns; kp!=NULL && kp->sc!=kcd->scs; kp=kp->next );
	if ( kp!=NULL ) {
	    offset = kp->off;
	    KP_SelectSubtable(kcd,kp->subtable);
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
    if ( kp==NULL && kcd->scf!=NULL ) {
	int32 len;
	GTextInfo **ti = GGadgetGetList(GWidgetGetControl(kcd->gw,CID_Subtable),&len);
	uint32 script = SCScriptFromUnicode(kcd->scf);
	int i;
	struct lookup_subtable *sub = NULL;

	for ( i=0; i<len; ++i ) {
	    struct lookup_subtable *test = ti[i]->userdata;
	    if ( test!=NULL && ScriptInFeatureScriptList(script,test->lookup->features)) {
		sub = test;
	break;
	    }
	}
	KP_SelectSubtable(kcd,sub);
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
}

static int KCD_GlyphSelected(GGadget *g, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    int which = GGadgetGetCid(g)==CID_Second;
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	KCD_UpdateGlyph(kcd,which);
	GDrawRequestExpose(kcd->kw,NULL,false);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	if ( !KPD_FinishKP(kcd)) {
	    KPD_RestoreGlyphs(kcd);
return( true );
	}
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
	ff_post_notice(_("Class 0"),_("The kerning values for class 0 (\"Everything Else\") should always be 0"));
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

static int KC_OK(GGadget *g, GEvent *e) {
    SplineFont *sf;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	KernClass *kc;
	int i;
	int32 len;
	GTextInfo **ti;

	sf = kcd->sf;
	if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
	else if ( sf->mm!=NULL ) sf = sf->mm->normal;

	if ( GDrawIsVisible(kcd->cw))
return( KCD_Next(g,e));
	else if ( GDrawIsVisible(kcd->kw))
return( KCD_Next2(g,e));

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
#ifdef FONTFORGE_CONFIG_DEVICETABLES
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
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		    memcpy(newd+i*kcd->second_cnt+(j+dir),kcd->adjusts+i*kcd->second_cnt+j,
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
	if ( off==0 || GGadgetGetFirstListSelectedItem(GWidgetGetControl(kcd->gw,CID_ClassList+off))>0 )
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

#define MID_Clear	1000
#define MID_ClearAll	1001
#define MID_ClearDevTab	1002
#define MID_ClearAllDevTab	1003

static void kernmenu_dispatch(GWindow gw, GMenuItem *mi, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(gw);
    int i;

    switch ( mi->mid ) {
      case MID_Clear:
	kcd->offsets[kcd->st_pos] = 0;
      break;
      case MID_ClearAll:
	for ( i=0; i<kcd->first_cnt*kcd->second_cnt; ++i )
	    kcd->offsets[i] = 0;
      break;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
      case MID_ClearDevTab: {
	DeviceTable *devtab = &kcd->adjusts[kcd->st_pos];
	free(devtab->corrections);
	devtab->corrections = NULL;
	devtab->first_pixel_size = devtab->last_pixel_size = 0;
      } break;
      case MID_ClearAllDevTab:
	for ( i=0; i<kcd->first_cnt*kcd->second_cnt; ++i ) {
	    DeviceTable *devtab = &kcd->adjusts[i];
	    free(devtab->corrections);
	    devtab->corrections = NULL;
	    devtab->first_pixel_size = devtab->last_pixel_size = 0;
	}
      break;
#endif
    }
    kcd->st_pos = -1;
}

static GMenuItem kernpopupmenu[] = {
    { { (unichar_t *) N_("Clear"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, '\0', ksm_control, NULL, NULL, kernmenu_dispatch, MID_Clear },
    { { (unichar_t *) N_("Clear All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, '\0', ksm_control, NULL, NULL, kernmenu_dispatch, MID_ClearAll },
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    { { (unichar_t *) N_("Clear Device Table"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, kernmenu_dispatch, MID_ClearDevTab },
    { { (unichar_t *) N_("Clear All Device Tables"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, kernmenu_dispatch, MID_ClearAllDevTab },
#endif
    { NULL }
};

static void KCD_PopupMenu(KernClassDlg *kcd,GEvent *event,int pos) {
    kcd->st_pos = pos;
    GMenuCreatePopupMenu(event->w,event, kernpopupmenu);
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
    else if ( event->type==et_mousedown && event->u.mouse.button==3 )
	KCD_PopupMenu(kcd,event,pos);
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
    int len, i, j, x, y;
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
	    len = GDrawGetText8Width(pixmap,buf,-1,NULL);
	    GDrawDrawBiText8(pixmap,kcd->xstart+(kcd->kernw-len)/2,kcd->ystart2+i*kcd->kernh+kcd->as+1,
		    buf,-1,NULL,0xff0000);
	}
    }
    for ( i=0 ; kcd->offleft+i<=kcd->second_cnt && (i-1)*kcd->kernw<kcd->fullwidth; ++i ) {
	GDrawDrawLine(pixmap,kcd->xstart2+i*kcd->kernw,kcd->ystart,kcd->xstart2+i*kcd->kernw,kcd->ystart+rect.height,
		0x808080);
	if ( i+kcd->offleft<kcd->second_cnt ) {
	    sprintf( buf, "%d", i+kcd->offleft );
	    len = GDrawGetText8Width(pixmap,buf,-1,NULL);
	    GDrawDrawBiText8(pixmap,kcd->xstart2+i*kcd->kernw+(kcd->kernw-len)/2,kcd->ystart+kcd->as+1,
		buf,-1,NULL,0xff0000);
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
	    len = GDrawGetText8Width(pixmap,buf,-1,NULL);
	    GDrawDrawBiText8(pixmap,x+kcd->kernw-3-len,y+kcd->as+1,
		buf,-1,NULL,0x000000);
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
	} else if ( GMenuIsCommand(event,H_("Quit|Ctl+Q") )) {
	    MenuExit(NULL,NULL,NULL);
return( true );
	} else if ( GMenuIsCommand(event,H_("Close|Ctl+Shft+Q") )) {
	    KC_DoCancel(kcd);
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
	} else if ( GMenuIsCommand(event,H_("Quit|Ctl+Q") )) {
	    MenuExit(NULL,NULL,NULL);
return( true );
	} else if ( GMenuIsCommand(event,H_("Close|Ctl+Shft+Q") )) {
	    KC_DoCancel(kcd);
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
	if ( kcd!=NULL ) {
	    SplineFont *sf = kcd->sf;
	    KernClassListDlg *kcld = kcd->isv ? sf->vkcld : sf->kcld;
	    KernClassDlg *prev, *test;
	    for ( prev=NULL, test=sf->kcd; test!=NULL && test!=kcd; prev=test, test=test->next );
	    if ( test==kcd ) {
		if ( prev==NULL )
		    sf->kcd = test->next;
		else
		    prev->next = test->next;
	    }
	    if ( kcld!=NULL ) {
		GGadgetSetList(GWidgetGetControl(kcld->gw,CID_List),
			KCLookupSubtableArray(sf,kcd->isv),false);
	    }
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

	GDrawGetSize(kcd->gw,&wsize);

	kcd->fullwidth = wsize.width;
	if ( kcd->cw!=NULL ) {
	    GDrawResize(kcd->cw,wsize.width,wsize.height);
	    GDrawResize(kcd->kw,wsize.width,wsize.height);
	    GGadgetGetSize(kcd->hsb,&csize);
	    kcd->width = csize.width;
	    kcd->xstart2 = csize.x;
	    GGadgetGetSize(kcd->vsb,&csize);
	    kcd->ystart2 = csize.y;
	    kcd->height = csize.height;
	    kcd->xstart = kcd->xstart2-kcd->kernw;
	    kcd->ystart = kcd->ystart2-kcd->fh-1;
	    KCD_SBReset(kcd);
	    GDrawRequestExpose(kcd->kw,NULL,false);
	    GDrawRequestExpose(kcd->cw,NULL,false);
	} else {
	    kcd->width = wsize.width-kcd->xstart2-5;
	    kcd->height = wsize.height-kcd->ystart2;
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
	int x, int y, int width, GGadgetCreateData **harray,
	GGadgetCreateData **varray ) {
    int blen = GIntGetResource(_NUM_Buttonsize);
    int space = 10;

    harray[0] = GCD_HPad10;
    label[k].text = (unichar_t *) (x<20?_("First Char"):_("Second Char"));
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = x+10; gcd[k].gd.pos.y = y;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_ClassLabel+off;
    gcd[k++].creator = GLabelCreate;
    harray[1] = &gcd[k-1]; harray[2] = GCD_Glue;

    label[k].image = &GIcon_up;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = x+100; gcd[k].gd.pos.y = y-3;
    gcd[k].gd.pos.height = 19;
    gcd[k].gd.flags = gg_visible|gg_utf8_popup;
    gcd[k].gd.handle_controlevent = KCD_Up;
    gcd[k].gd.popup_msg = (unichar_t *) _("Move selected class up");
    gcd[k].gd.cid = CID_ClassUp+off;
    gcd[k++].creator = GButtonCreate;
    harray[3] = &gcd[k-1];

    label[k].image = &GIcon_down;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = x+120; gcd[k].gd.pos.y = y-3;
    gcd[k].gd.pos.height = 19;
    gcd[k].gd.flags = gg_visible|gg_utf8_popup;
    gcd[k].gd.handle_controlevent = KCD_Down;
    gcd[k].gd.popup_msg = (unichar_t *) _("Move selected class down");
    gcd[k].gd.cid = CID_ClassDown+off;
    gcd[k++].creator = GButtonCreate;
    harray[4] = &gcd[k-1]; harray[5] = GCD_Glue; harray[6] = NULL;

    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.u.boxelements = harray;
    gcd[k++].creator = GHBoxCreate;
    varray[0] = &gcd[k-1];

    gcd[k].gd.pos.x = x; gcd[k].gd.pos.y = y+17;
    gcd[k].gd.pos.width = width;
    gcd[k].gd.pos.height = 8*12+10;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_list_multiplesel;
    gcd[k].gd.handle_controlevent = KCD_ClassSelected;
    gcd[k].gd.cid = CID_ClassList+off;
    gcd[k++].creator = GListCreate;
    varray[1] = &gcd[k-1];

    label[k].text = (unichar_t *) S_("Class|New...");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+gcd[k-1].gd.pos.height+10;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.handle_controlevent = KCD_New;
    gcd[k].gd.cid = CID_ClassNew+off;
    gcd[k++].creator = GButtonCreate;
    harray[7] = &gcd[k-1]; harray[8] = GCD_Glue;

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
    harray[9] = &gcd[k-1]; harray[10] = GCD_Glue;

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
    harray[11] = &gcd[k-1]; harray[12] = NULL;

    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.u.boxelements = harray+7;
    gcd[k++].creator = GHBoxCreate;
    varray[2] = &gcd[k-1];

/* GT: Select the class containing the glyph named in the following text field */
    label[k].text = (unichar_t *) _("Select Glyph Class:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-3].gd.pos.x+5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+26+4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("Select the class containing the named glyph");
    gcd[k++].creator = GLabelCreate;
    harray[13] = &gcd[k-1];

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+100; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 80;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("Select the class containing the named glyph");
    gcd[k].gd.handle_controlevent = KCD_TextSelect;
    gcd[k].gd.cid = CID_ClassSelect+off;
    gcd[k++].creator = GTextFieldCreate;
    harray[14] = &gcd[k-1]; harray[15] = NULL;

    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.u.boxelements = harray+13;
    gcd[k++].creator = GHBoxCreate;
    varray[3] = &gcd[k-1]; varray[4] = NULL;

    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.u.boxelements = varray;
    gcd[k++].creator = GVBoxCreate;

return( k );
}

static void FillShowKerningWindow(KernClassDlg *kcd, int for_class, SplineFont *sf) {
    GGadgetCreateData gcd[31], hbox, flagbox, hvbox, buttonbox, mainbox[2];
    GGadgetCreateData *harray[10], *hvarray[15], *flagarray[4], *buttonarray[9], *varray[12];
    GTextInfo label[31];
    int k,j;
    char buffer[20];
    GRect pos;

    kcd->top = GDrawPointsToPixels(kcd->gw,100);
    kcd->pixelsize = 150;
    kcd->magfactor = 1;

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    memset(&hbox,0,sizeof(hbox));
    memset(&flagbox,0,sizeof(flagbox));
    memset(&hvbox,0,sizeof(hvbox));
    memset(&buttonbox,0,sizeof(buttonbox));
    memset(&mainbox,0,sizeof(mainbox));
    k = j = 0;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = 120;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.handle_controlevent = KCD_GlyphSelected;
    gcd[k].gd.cid = CID_First;
    gcd[k++].creator = for_class ? GListButtonCreate : GTextFieldCreate;
    harray[0] = &gcd[k-1];

    gcd[k].gd.pos.x = 130; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = 120;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    if ( !for_class ) gcd[k].gd.flags |= gg_list_alphabetic;
    gcd[k].gd.handle_controlevent = KCD_GlyphSelected;
    gcd[k].gd.cid = CID_Second;
    gcd[k++].creator = for_class ? GListButtonCreate : GListFieldCreate;
    harray[1] = &gcd[k-1];

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
    harray[2] = GCD_Glue; harray[3] = &gcd[k-1];
    harray[4] = GCD_Glue; harray[5] = GCD_Glue;
    harray[6] = GCD_Glue; harray[7] = GCD_Glue; harray[8] = NULL;

    hbox.gd.flags = gg_enabled|gg_visible;
    hbox.gd.u.boxelements = harray;
    hbox.creator = GHBoxCreate;
    varray[j++] = &hbox; varray[j++] = NULL;

    label[k].text = (unichar_t *) _("Display Size:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k++].creator = GLabelCreate;
    hvarray[0] = &gcd[k-1];

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
    hvarray[1] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Magnification:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 185; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k++].creator = GLabelCreate;
    hvarray[2] = &gcd[k-1];

    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_Magnifications;
    gcd[k].gd.u.list = magnifications;
    gcd[k].gd.handle_controlevent = KCD_MagnificationChanged;
    gcd[k++].creator = GListButtonCreate;
    hvarray[3] = &gcd[k-1]; hvarray[4] = GCD_Glue; hvarray[5] = GCD_Glue;
    hvarray[6] = NULL;

    label[k].text = (unichar_t *) _("Kern Offset:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k++].creator = GLabelCreate;
    hvarray[7] = &gcd[k-1];

    gcd[k].gd.pos.x = 90; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_KernOffset;
    gcd[k].gd.handle_controlevent = KCD_KernOffChanged;
    gcd[k++].creator = GTextFieldCreate;
    hvarray[8] = &gcd[k-1];

#ifdef FONTFORGE_CONFIG_DEVICETABLES
    label[k].text = (unichar_t *) _("Device Table Correction:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 185; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k++].creator = GLabelCreate;
    hvarray[9] = &gcd[k-1];

    label[k].text = (unichar_t *) "0";
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 305; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_Correction;
    gcd[k].gd.handle_controlevent = KCD_CorrectionChanged;
    gcd[k++].creator = GTextFieldCreate;
    hvarray[10] = &gcd[k-1];

    label[k].text = (unichar_t *) "Clear";
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 305; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup ;
    gcd[k].gd.popup_msg = (unichar_t *) _("Clear all device table corrections associated with this combination");
    gcd[k].gd.cid = CID_ClearDevice;
    gcd[k].gd.handle_controlevent = KCD_ClearDevice;
    gcd[k++].creator = GButtonCreate;
    hvarray[11] = &gcd[k-1];
#else
    hvarray[9] = GCD_Glue;
    hvarray[10] = GCD_Glue;
    hvarray[11] = GCD_Glue;
#endif
    hvarray[12] = GCD_Glue;
    hvarray[13] = NULL; hvarray[14] = NULL;

    hvbox.gd.flags = gg_enabled|gg_visible;
    hvbox.gd.u.boxelements = hvarray;
    hvbox.creator = GHVBoxCreate;
    varray[j++] = &hvbox; varray[j++] = NULL;

    varray[j++] = GCD_Glue; varray[j++] = NULL;

    GDrawGetSize(kcd->kw,&pos);

    if ( !for_class ) {
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = -40;
	gcd[k].gd.flags = gg_enabled ;
	label[k].text = (unichar_t *) _("Lookup subtable:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k++].creator = GLabelCreate;
	flagarray[0] = &gcd[k-1];

	gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = KC_Height-KC_CANCELDROP-27;
	gcd[k].gd.pos.width = GDrawPixelsToPoints(kcd->kw,pos.width)-20;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Subtable;
	gcd[k].gd.handle_controlevent = KP_Subtable;
	gcd[k++].creator = GListButtonCreate;
	flagarray[1] = &gcd[k-1]; flagarray[2] = GCD_Glue; flagarray[3] = NULL;

	flagbox.gd.flags = gg_enabled|gg_visible;
	flagbox.gd.u.boxelements = flagarray;
	flagbox.creator = GHBoxCreate;
	varray[j++] = &flagbox; varray[j++] = NULL;
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

    buttonarray[0] = GCD_Glue; buttonarray[1] = &gcd[k-2]; buttonarray[2] = GCD_Glue;
    buttonarray[3] = GCD_Glue; buttonarray[4] = &gcd[k-1]; buttonarray[5] = GCD_Glue;
    buttonarray[6] = NULL;
    buttonbox.gd.flags = gg_enabled|gg_visible;
    buttonbox.gd.u.boxelements = buttonarray;
    buttonbox.creator = GHBoxCreate;
    varray[j++] = &buttonbox; varray[j++] = NULL; varray[j++] = NULL;

    mainbox[0].gd.pos.x = mainbox[0].gd.pos.y = 2;
    mainbox[0].gd.flags = gg_enabled|gg_visible;
    mainbox[0].gd.u.boxelements = varray;
    mainbox[0].creator = GHVGroupCreate;

    GGadgetsCreate(kcd->kw,mainbox);

    GHVBoxSetExpandableRow(mainbox[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(hbox.ret,gb_expandglue);
    GHVBoxSetExpandableCol(hvbox.ret,gb_expandglue);
    GHVBoxSetExpandableCol(buttonbox.ret,gb_expandgluesame);
    if ( !for_class ) {
	GHVBoxSetExpandableCol(flagbox.ret,gb_expandglue);
	GGadgetSetList(flagarray[1]->ret,SFSubtablesOfType(sf,gpos_pair,false,false),false);
    }
}

void KernClassD(KernClass *kc, SplineFont *sf, int layer, int isv) {
    GRect pos, subpos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[46], classbox, hvbox, buttonbox, mainbox[2];
    GGadgetCreateData selbox, *selarray[7], pane2[2];
    GGadgetCreateData *harray1[17], *harray2[17], *varray1[5], *varray2[5];
    GGadgetCreateData *hvarray[13], *buttonarray[8], *varray[15], *harrayclasses[6];
    GTextInfo label[46];
    KernClassDlg *kcd;
    int i, kc_width, vi, y, k;
    int as, ds, ld, sbsize;
    FontRequest rq;
    static unichar_t kernw[] = { '-', '1', '2', '3', '4', '5', 0 };
    GWindow gw;
    char titlebuf[300];
    static GFont *font;

    for ( kcd = sf->kcd; kcd!=NULL && kcd->orig!=kc; kcd = kcd->next );
    if ( kcd!=NULL ) {
	GDrawSetVisible(kcd->gw,true);
	GDrawRaise(kcd->gw);
return;
    }
    kcd = gcalloc(1,sizeof(KernClassDlg));
    kcd->orig = kc;
    kcd->subtable = kc->subtable;
    kcd->sf = sf;
    kcd->layer = layer;
    kcd->isv = isv;
    kcd->next = sf->kcd;
    sf->kcd = kcd;

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

    memset(&wattrs,0,sizeof(wattrs));
    memset(&gcd,0,sizeof(gcd));
    memset(&classbox,0,sizeof(classbox));
    memset(&hvbox,0,sizeof(hvbox));
    memset(&buttonbox,0,sizeof(buttonbox));
    memset(&mainbox,0,sizeof(mainbox));
    memset(&label,0,sizeof(label));

    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = false;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
/* GT: The %s is the name of the lookup subtable containing this kerning class */
    snprintf( titlebuf, sizeof(titlebuf), _("Kerning by Classes: %s"), kc->subtable->subtable_name );
    wattrs.utf8_window_title =  titlebuf ;
    wattrs.is_dlg = false;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,KC_Width));
    pos.height = GDrawPointsToPixels(NULL,KC_Height);
    kcd->gw = gw = GDrawCreateTopWindow(NULL,&pos,kcd_e_h,kcd,&wattrs);

    kc_width = GDrawPixelsToPoints(NULL,pos.width*100/GGadgetScale(100));

    if ( font==NULL ) {
	memset(&rq,'\0',sizeof(rq));
	rq.point_size = 12;
	rq.weight = 400;
	rq.utf8_family_name = MONO_UI_FAMILIES;
	font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
	font = GResourceFindFont("KernClass.Font",font);
    }
    kcd->font = font;
    GDrawFontMetrics(kcd->font,&as,&ds,&ld);
    kcd->fh = as+ds; kcd->as = as;
    GDrawSetFont(gw,kcd->font);

    kcd->kernh = kcd->fh+3;
    kcd->kernw = GDrawGetTextWidth(gw,kernw,-1,NULL)+3;

    i = 0;
    snprintf( titlebuf, sizeof(titlebuf), _("Lookup Subtable: %s"), kc->subtable->subtable_name );
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 5;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    label[i].text = (unichar_t *) titlebuf;
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i++].creator = GLabelCreate;
    varray[0] = &gcd[i-1]; varray[1] = NULL;

    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = GDrawPointsToPixels(gw,gcd[i-1].gd.pos.y+17);
    gcd[i].gd.pos.width = pos.width-20;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[i++].creator = GLineCreate;
    varray[2] = &gcd[i-1]; varray[3] = NULL;

    y = gcd[i-2].gd.pos.y+23;
    i = AddClassList(gcd,label,i,0,5,y,(kc_width-20)/2,harray1,varray1);
    harrayclasses[0] = &gcd[i-1];
    i = AddClassList(gcd,label,i,100,(kc_width-20)/2+10,y,(kc_width-20)/2,harray2,varray2);
    harrayclasses[2] = &gcd[i-1]; harrayclasses[3] = NULL;

    gcd[i].gd.pos.height = 20;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_line_vert;
    gcd[i++].creator = GLineCreate;
    harrayclasses[1] = &gcd[i-1];

    classbox.gd.flags = gg_enabled|gg_visible;
    classbox.gd.u.boxelements = harrayclasses;
    classbox.creator = GHBoxCreate;
    varray[4] = &classbox; varray[5] = NULL;

    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = GDrawPointsToPixels(gw,gcd[i-1].gd.pos.y+27);
    gcd[i].gd.pos.width = pos.width-20;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[i++].creator = GLineCreate;
    varray[6] = &gcd[i-1]; varray[7] = NULL;

    kcd->canceldrop = GDrawPointsToPixels(gw,KC_CANCELDROP);
    kcd->sbdrop = kcd->canceldrop+GDrawPointsToPixels(gw,7);
    gcd[i].gd.pos.width = kcd->kernw;
    gcd[i].gd.pos.height = kcd->kernh;
    gcd[i].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    gcd[i++].creator = GSpacerCreate;
    hvarray[0] = &gcd[i-1]; hvarray[1] = GCD_Glue; hvarray[2] = GCD_Glue; hvarray[3] = NULL;

    gcd[i].gd.pos.width = kcd->kernw;
    gcd[i].gd.pos.height = 4*kcd->kernh;
    gcd[i].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    gcd[i++].creator = GSpacerCreate;

    vi = i;
    gcd[i].gd.pos.width = sbsize = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gcd[i].gd.pos.x = pos.width-sbsize;
    gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+8;
    gcd[i].gd.pos.height = pos.height-gcd[i].gd.pos.y-sbsize-kcd->sbdrop;
    gcd[i].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    gcd[i++].creator = GScrollBarCreate;
    hvarray[4] = &gcd[i-2]; hvarray[5] = GCD_Glue; hvarray[6] = &gcd[i-1]; hvarray[7] = NULL;

    gcd[i].gd.pos.height = sbsize;
    gcd[i].gd.pos.y = pos.height-sbsize-8;
    gcd[i].gd.pos.x = 4;
    gcd[i].gd.pos.width = pos.width-sbsize;
    gcd[i].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    gcd[i++].creator = GScrollBarCreate;
    hvarray[8] = GCD_Glue; hvarray[9] = &gcd[i-1]; hvarray[10] = GCD_Glue; hvarray[11] = NULL;
    hvarray[12] = NULL;
    kcd->width = gcd[i-1].gd.pos.width;
    kcd->xstart = 5;

    hvbox.gd.flags = gg_enabled|gg_visible;
    hvbox.gd.u.boxelements = hvarray;
    hvbox.creator = GHVBoxCreate;
    varray[8] = &hvbox; varray[9] = NULL;

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

    buttonarray[0] = GCD_Glue; buttonarray[1] = &gcd[i-2]; buttonarray[2] = GCD_Glue;
    buttonarray[3] = GCD_Glue; buttonarray[4] = &gcd[i-1]; buttonarray[5] = GCD_Glue;
    buttonarray[6] = NULL;
    buttonbox.gd.flags = gg_enabled|gg_visible;
    buttonbox.gd.u.boxelements = buttonarray;
    buttonbox.creator = GHBoxCreate;
    varray[10] = &buttonbox; varray[11] = NULL; varray[12] = NULL;

    mainbox[0].gd.pos.x = mainbox[0].gd.pos.y = 2;
    mainbox[0].gd.flags = gg_enabled|gg_visible;
    mainbox[0].gd.u.boxelements = varray;
    mainbox[0].creator = GHVGroupCreate;

    GGadgetsCreate(kcd->gw,mainbox);
    kcd->vsb = gcd[vi].ret;
    kcd->hsb = gcd[vi+1].ret;

    GHVBoxSetExpandableRow(mainbox[0].ret,4);

    GHVBoxSetExpandableCol(buttonbox.ret,gb_expandgluesame);

    GHVBoxSetPadding(hvbox.ret,0,0);
    GHVBoxSetExpandableRow(hvbox.ret,1);
    GHVBoxSetExpandableCol(hvbox.ret,1);

    for ( i=0; i<2; ++i ) {
	GGadgetCreateData *box = harrayclasses[2*i], **boxarray = box->gd.u.boxelements;
	GHVBoxSetExpandableRow(box->ret,1);
	GHVBoxSetExpandableCol(boxarray[0]->ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxarray[2]->ret,gb_expandgluesame);
	GHVBoxSetExpandableCol(boxarray[3]->ret,1);
    }

    for ( i=0; i<2; ++i ) {
	GGadget *list = GWidgetGetControl(kcd->gw,CID_ClassList+i*100);
	if ( i==0 && kcd->orig!=NULL && kcd->orig->firsts!=NULL && kcd->orig->firsts[0]!=NULL ) {
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

    memset(&selbox,0,sizeof(selbox));
    memset(&buttonbox,0,sizeof(buttonbox));
    memset(pane2,0,sizeof(pane2));
    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    k = 0;

    label[k].text = (unichar_t *) _("Set From Font");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 5;
    gcd[k].gd.popup_msg = (unichar_t *) _("Set this glyph list to be the characters selected in the fontview");
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.handle_controlevent = KCD_FromSelection;
    gcd[k].gd.cid = CID_Set;
    gcd[k++].creator = GButtonCreate;
    selarray[0] = &gcd[k-1]; selarray[1] = GCD_Glue;

    label[k].text = (unichar_t *) _("Select In Font");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 110; gcd[k].gd.pos.y = 5;
    gcd[k].gd.popup_msg = (unichar_t *) _("Set the fontview's selection to be the characters named here");
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.handle_controlevent = KCD_ToSelection;
    gcd[k].gd.cid = CID_Select;
    gcd[k++].creator = GButtonCreate;
    selarray[2] = &gcd[k-1]; selarray[3] = GCD_Glue;
    selarray[4] = GCD_Glue; selarray[5] = GCD_Glue; selarray[6] = NULL;

    selbox.gd.flags = gg_enabled|gg_visible;
    selbox.gd.u.boxelements = selarray;
    selbox.creator = GHBoxCreate;
    varray[0] = &selbox; varray[1] = NULL;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 30;
    gcd[k].gd.pos.width = KC_Width-25; gcd[k].gd.pos.height = 8*13+4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
    gcd[k].gd.cid = CID_GlyphList;
    gcd[k++].creator = GTextAreaCreate;
    varray[2] = &gcd[k-1]; varray[3] = NULL;
    varray[4] = GCD_Glue; varray[5] = NULL;

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

    buttonarray[0] = GCD_Glue; buttonarray[1] = &gcd[k-2]; buttonarray[2] = GCD_Glue;
    buttonarray[3] = GCD_Glue; buttonarray[4] = &gcd[k-1]; buttonarray[5] = GCD_Glue;
    buttonarray[6] = NULL;
    buttonbox.gd.flags = gg_enabled|gg_visible;
    buttonbox.gd.u.boxelements = buttonarray;
    buttonbox.creator = GHBoxCreate;
    varray[6] = &buttonbox; varray[7] = NULL; varray[8] = NULL;

    pane2[0].gd.pos.x = pane2[0].gd.pos.y = 2;
    pane2[0].gd.flags = gg_enabled|gg_visible;
    pane2[0].gd.u.boxelements = varray;
    pane2[0].creator = GHVGroupCreate;

    GGadgetsCreate(kcd->cw,pane2);

    GHVBoxSetExpandableRow(pane2[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(selbox.ret,gb_expandglue);
    GHVBoxSetExpandableCol(buttonbox.ret,gb_expandgluesame);


    kcd->kw = GWidgetCreateSubWindow(kcd->gw,&subpos,subkern_e_h,kcd,&wattrs);
    FillShowKerningWindow(kcd, true, kcd->sf);

    GHVBoxFitWindow(mainbox[0].ret);
    GDrawSetVisible(kcd->gw,true);
}

static int KCL_New(GGadget *g, GEvent *e) {
    KernClassListDlg *kcld;
    struct subtable_data sd;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	kcld = GDrawGetUserData(GGadgetGetWindow(g));
	memset(&sd,0,sizeof(sd));
	sd.flags = (kcld->isv ? sdf_verticalkern : sdf_horizontalkern ) |
		sdf_kernclass;
	SFNewLookupSubtableOfType(kcld->sf,gpos_pair,&sd,kcld->layer);
    }
return( true );
}

static int KCL_Delete(GGadget *g, GEvent *e) {
    int32 len; int i,j;
    GTextInfo **old, **new;
    GGadget *list;
    KernClassListDlg *kcld;
    KernClassDlg *kcd;
    KernClass *p, *kc, *n;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	kcld = GDrawGetUserData(GGadgetGetWindow(g));
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
		for ( kcd=kcld->sf->kcd; kcd!=NULL && kcd->orig!=kc; kcd=kcd->next );
		if ( kcd!=NULL )
		    KC_DoCancel(kcd);
		KernClassListFree(kc);
	    }
	}
	new[j] = gcalloc(1,sizeof(GTextInfo));
	GGadgetSetList(list,new,false);
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
	if ( sel==-1 )
return( true );
	for ( kc=kcld->isv?kcld->sf->vkerns:kcld->sf->kerns, i=0; i<sel; kc=kc->next, ++i );
	KernClassD(kc,kcld->sf,kcld->layer,kcld->isv);
    }
return( true );
}

static int KCL_Done(GGadget *g, GEvent *e) {
    KernClassListDlg *kcld;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	kcld = GDrawGetUserData(GGadgetGetWindow(g));
	GDrawDestroyWindow(kcld->gw);
    }
return( true );
}

static int KCL_SelChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	KernClassListDlg *kcld = GDrawGetUserData(GGadgetGetWindow(g));
	int sel = GGadgetGetFirstListSelectedItem(g);
	GGadgetSetEnabled(GWidgetGetControl(kcld->gw,CID_Delete),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(kcld->gw,CID_Edit),sel!=-1);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	KernClassListDlg *kcld = GDrawGetUserData(GGadgetGetWindow(g));
	e->u.control.subtype = et_buttonactivate;
	e->u.control.g = GWidgetGetControl(kcld->gw,CID_Edit);
	KCL_Edit(e->u.control.g,e);
    }
return( true );
}

static int kcl_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	KernClassListDlg *kcld = GDrawGetUserData(gw);
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

void ShowKernClasses(SplineFont *sf,MetricsView *mv,int layer,int isv) {
    KernClassListDlg *kcld;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[7], boxes[4], *varray[10], *harray[9], *harray2[5];
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
    kcld->layer = layer;
    kcld->isv = isv;
    if ( isv )
	sf->vkcld = kcld;
    else
	sf->kcld = kcld;

    memset(&wattrs,0,sizeof(wattrs));
    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));
    memset(boxes,0,sizeof(boxes));

    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = false;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title =  isv?_("VKern By Classes"):_("Kern By Classes");
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
    gcd[0].gd.u.list = KCLookupSubtableList(sf,isv);
    gcd[0].gd.handle_controlevent = KCL_SelChanged;
    gcd[0].creator = GListCreate;
    varray[0] = &gcd[0]; varray[1] = NULL;

    gcd[1].gd.pos.x = 10; gcd[1].gd.pos.y = gcd[0].gd.pos.y+gcd[0].gd.pos.height+4;
    gcd[1].gd.flags = gg_visible | gg_enabled;
    label[1].text = (unichar_t *) S_("KernClass|_New Lookup...");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.cid = CID_New;
    gcd[1].gd.handle_controlevent = KCL_New;
    gcd[1].creator = GButtonCreate;
    harray[0] = GCD_Glue; harray[1] = &gcd[1];

    gcd[2].gd.pos.x = 20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor); gcd[2].gd.pos.y = gcd[1].gd.pos.y;
    gcd[2].gd.flags = gg_visible;
    label[2].text = (unichar_t *) _("_Delete");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.cid = CID_Delete;
    gcd[2].gd.handle_controlevent = KCL_Delete;
    gcd[2].creator = GButtonCreate;
    harray[2] = GCD_Glue; harray[3] = &gcd[2];

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
    harray[4] = GCD_Glue; harray[5] = &gcd[3]; harray[6] = GCD_Glue; harray[7] = NULL;

    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = harray;
    boxes[0].creator = GHBoxCreate;
    varray[2] = &boxes[0]; varray[3] = NULL;

    gcd[4].gd.pos.x = 10; gcd[4].gd.pos.y = gcd[1].gd.pos.y+28;
    gcd[4].gd.pos.width = kcl_width-20;
    gcd[4].gd.flags = gg_visible;
    gcd[4].creator = GLineCreate;
    varray[4] = &gcd[4]; varray[5] = NULL;

    gcd[5].gd.pos.x = (kcl_width-GIntGetResource(_NUM_Buttonsize))/2; gcd[5].gd.pos.y = gcd[4].gd.pos.y+7;
    gcd[5].gd.flags = gg_visible|gg_enabled|gg_but_default|gg_but_cancel;
    label[5].text = (unichar_t *) _("_Done");
    label[5].text_is_1byte = true;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.handle_controlevent = KCL_Done;
    gcd[5].creator = GButtonCreate;
    harray2[0] = GCD_Glue; harray2[1] = &gcd[5]; harray2[2] = GCD_Glue; harray2[3] = NULL;

    boxes[1].gd.flags = gg_enabled|gg_visible;
    boxes[1].gd.u.boxelements = harray2;
    boxes[1].creator = GHBoxCreate;
    varray[6] = &boxes[1]; varray[7] = NULL; varray[8] = NULL;

    boxes[2].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = varray;
    boxes[2].creator = GHVGroupCreate;

    GGadgetsCreate(kcld->gw,&boxes[2]);
    GHVBoxSetExpandableRow(boxes[2].ret,0);
    GHVBoxSetExpandableCol(boxes[0].ret,gb_expandgluesame);
    GHVBoxSetExpandableCol(boxes[1].ret,gb_expandgluesame);
    GDrawSetVisible(kcld->gw,true);
}

void KCLD_End(KernClassListDlg *kcld) {
    KernClassDlg *kcd, *kcdnext;
    for ( kcd= kcld->sf->kcd; kcd!=NULL; kcd=kcdnext ) {
	kcdnext = kcd->next;
	KC_DoCancel(kcd);
    }
    if ( kcld==NULL )
return;
    GDrawDestroyWindow(kcld->gw);
}

void KCLD_MvDetach(KernClassListDlg *kcld,MetricsView *mv) {
    if ( kcld==NULL )
return;
}

/* ************************************************************************** */
/* *************************** Kern Pair Dialog  **************************** */
/* ************************************************************************** */

void KernPairD(SplineFont *sf,SplineChar *sc1,SplineChar *sc2,int layer,int isv) {
    GRect pos;
    GWindowAttrs wattrs;
    KernClassDlg kcd;
    GWindow gw;
    int gid;

    if ( sc1==NULL ) {
	FontView *fv = (FontView *) sf->fv;
	int start = fv->rowoff*fv->colcnt, end = start+fv->rowcnt*fv->colcnt;
	int i;
	for ( i=start; i<end && i<fv->b.map->enccount; ++i )
	    if ( (gid=fv->b.map->map[i])!=-1 && sf->glyphs[gid]!=NULL &&
		    (isv ? sf->glyphs[gid]->vkerns : sf->glyphs[gid]->kerns)!=NULL )
	break;
	if ( i==end || i==fv->b.map->enccount ) {
	    for ( i=0; i<fv->b.map->enccount; ++i )
		if ( (gid=fv->b.map->map[i])!=-1 && sf->glyphs[gid]!=NULL &&
			(isv ? sf->glyphs[gid]->vkerns : sf->glyphs[gid]->kerns)!=NULL )
	    break;
	}
	if ( i==fv->b.map->enccount ) {
	    for ( i=start; i<end && i<fv->b.map->enccount; ++i )
		if ( (gid=fv->b.map->map[i])!=-1 && sf->glyphs[gid]!=NULL )
	    break;
	    if ( i==end || i==fv->b.map->enccount ) {
		for ( i=0; i<fv->b.map->enccount; ++i )
		    if ( (gid=fv->b.map->map[i])!=-1 && sf->glyphs[gid]!=NULL )
		break;
	    }
	}
	if ( i!=fv->b.map->enccount )
	    sc1 = sf->glyphs[gid];
    }
    if ( sc2==NULL && sc1!=NULL && (isv ? sc1->vkerns : sc1->kerns)!=NULL )
	sc2 = (isv ? sc1->vkerns : sc1->kerns)->sc;
    
    memset(&kcd,0,sizeof(kcd));
    kcd.sf = sf;
    kcd.layer = layer;
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
    wattrs.utf8_window_title = _("Kern Pair Closeup");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,KC_Width));
    pos.height = GDrawPointsToPixels(NULL,KC_Height);
    kcd.gw = gw = GDrawCreateTopWindow(NULL,&pos,kcd_e_h,&kcd,&wattrs);
    kcd.canceldrop = GDrawPointsToPixels(gw,KC_CANCELDROP);


    kcd.kw = gw;
    FillShowKerningWindow(&kcd, false, kcd.sf);

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

/* ************************************************************************** */
/* *******************************   kerning   ****************************** */
/* ************************************************************************** */

KernClass *SFFindKernClass(SplineFont *sf,SplineChar *first,SplineChar *last,
	int *index,int allow_zero) {
    int i,f,l;
    KernClass *kc;

    for ( i=0; i<=allow_zero; ++i ) {
	for ( kc=sf->kerns; kc!=NULL; kc=kc->next ) {
	    f = KCFindName(first->name,kc->firsts,kc->first_cnt,false);
	    l = KCFindName(last->name,kc->seconds,kc->second_cnt,true);
	    if ( f!=-1 && l!=-1 ) {
		if ( i || kc->offsets[f*kc->second_cnt+l]!=0 ) {
		    *index = f*kc->second_cnt+l;
return( kc );
		}
	    }
	}
    }
return( NULL );
}

KernClass *SFFindVKernClass(SplineFont *sf,SplineChar *first,SplineChar *last,
	int *index,int allow_zero) {
    int i,f,l;
    KernClass *kc;

    for ( i=0; i<=allow_zero; ++i ) {
	for ( kc=sf->vkerns; kc!=NULL; kc=kc->next ) {
	    f = KCFindName(first->name,kc->firsts,kc->first_cnt,false);
	    l = KCFindName(last->name,kc->seconds,kc->second_cnt,true);
	    if ( f!=-1 && l!=-1 ) {
		if ( i || kc->offsets[f*kc->second_cnt+l]!=0 ) {
		    *index = f*kc->second_cnt+l;
return( kc );
		}
	    }
	}
    }
return( NULL );
}
