/* Copyright (C) 2003-2012 by George Williams */
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

#include <fontforge-config.h>

#include "autowidth2.h"
#include "fontforgeui.h"
#include "fvfonts.h"
#include "gkeysym.h"
#include "gresedit.h"
#include "lookups.h"
#include "splinefill.h"
#include "splineutil.h"
#include "tottfgpos.h"
#include "ustring.h"
#include "utype.h"

#include <math.h>
#include <string.h>

extern GBox _ggadget_Default_Box;
#define ACTIVE_BORDER   (_ggadget_Default_Box.active_border)

GResFont kernclass_font = GRESFONT_INIT("400 12pt " MONO_UI_FAMILIES);
Color kernclass_classfgcol = 0x006080;

typedef struct kernclassdlg {
    struct kernclasslistdlg *kcld;
    KernClass *orig;
    struct lookup_subtable *subtable;
    int first_cnt, second_cnt;
    char **firsts_names;
    char **seconds_names;
    int *firsts_flags;
    int *seconds_flags;
    int16_t *offsets;
    int *offsets_flags;
    DeviceTable *adjusts;
    DeviceTable active_adjust;		/* The one that is currently active */
    DeviceTable orig_adjust;		/* Initial value for this the active adjust */
    GWindow gw, subw;
    GFont *font;
    int fh, as;
    int kernh, kernw;		/* Width of the box containing the kerning val */
    int xstart, ystart;		/* This is where the headers start */
    int xstart2, ystart2;	/* This is where the data start */
    int width, height, fullwidth, subwidth;
    int canceldrop, sbdrop;
    int offleft, offtop;
    GGadget *hsb, *vsb;
    int isedit, off;
    int st_pos, old_pos;
    BDFChar *fsc, *ssc;
    int pixelsize;
    int magfactor;
    int downpos, down, within, orig_kern;
    SplineFont *sf;
    int layer;
    int isv;
    int first_class_new, r2l, index;
    int orig_kern_offset;
/* For the kern pair dlg */
    int done;
    SplineChar *sc1, *sc2;
    int iskernpair;
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

#define CID_List	1040		/* List of kern class subtables */
#define CID_New		1041
#define CID_Delete	1042
#define CID_Edit	1043

#define CID_ClassList	1007		/* And 1107 for second char */
#define CID_ClassLabel	1011
#define CID_ClassSelect	1014

#define CID_OK		1015
#define CID_Cancel	1016

#define CID_First	1030
#define CID_Second	1031
#define CID_KernOffset	1032
#define CID_Prev2	1033
#define CID_Next2	1034
#define CID_DisplaySize	1036
#define CID_Correction	1037
#define CID_FreeType	1038
#define CID_Magnifications	1039
#define CID_ClearDevice	1040
#define CID_Display	1041

#define CID_Separation	2008
#define CID_MinKern	2009
#define CID_Touched	2010
#define CID_OnlyCloser	2011
#define CID_Autokern	2012

#define CID_SizeLabel	3000
#define CID_MagLabel	3001
#define CID_OffsetLabel	3002
#define CID_CorrectLabel	3003
#define CID_Revert	3004
#define CID_TopBox	3005
#define CID_ShowHideKern	3006

extern int _GScrollBar_Width;
int show_kerning_pane_in_class = true;

static GTextInfo magnifications[] = {
    { (unichar_t *) "100%", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "200%", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "300%", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "400%", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
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
    ti = calloc(cnt+1,sizeof(GTextInfo*));
    for ( kc=head, cnt=0; kc!=NULL; kc=kc->next, ++cnt ) {
	ti[cnt] = calloc(1,sizeof(GTextInfo));
	ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
	ti[cnt]->text = utf82u_copy(kc->subtable->subtable_name);
    }
    ti[cnt] = calloc(1,sizeof(GTextInfo));
return( ti );
}

static GTextInfo *KCLookupSubtableList(SplineFont *sf,int isv) {
    int cnt;
    KernClass *kc, *head = isv ? sf->vkerns : sf->kerns;
    GTextInfo *ti;

    if ( sf->cidmaster!=NULL ) sf=sf->cidmaster;
    else if ( sf->mm!=NULL ) sf = sf->mm->normal;

    for ( kc=head, cnt=0; kc!=NULL; kc=kc->next, ++cnt );
    ti = calloc(cnt+1,sizeof(GTextInfo));
    for ( kc=head, cnt=0; kc!=NULL; kc=kc->next, ++cnt )
	ti[cnt].text = utf82u_copy(kc->subtable->subtable_name);
return( ti );
}

static int isEverythingElse(char *text) {
    /* GT: The string "{Everything Else}" is used in the context of a list */
    /* GT: of classes (a set of kerning classes) where class 0 designates the */
    /* GT: default class containing all glyphs not specified in the other classes */
    int ret = strcmp(text,_("{Everything Else}") );
return( ret==0 );
}

static void KCD_AddOffset(void *data,int left_index,int right_index, int kern) {
    KernClassDlg *kcd = data;

    if ( kcd->first_class_new && !kcd->r2l ) {
	left_index = kcd->index;
	kcd->offsets[left_index*kcd->second_cnt+right_index] = kern;
    } else if ( kcd->first_class_new ) {
	right_index = kcd->index;
	kcd->offsets[right_index*kcd->second_cnt+left_index] = kern;
    } else if ( !kcd->r2l ) {
	right_index = kcd->index;
	kcd->offsets[left_index*kcd->second_cnt+right_index] = kern;
    } else {
	left_index = kcd->index;
	kcd->offsets[right_index*kcd->second_cnt+left_index] = kern;
    }
}

static void KCD_AddOffsetAsIs(void *data,int left_index,int right_index, int kern) {
    KernClassDlg *kcd = data;

    if ( !kcd->r2l ) {
	kcd->offsets[left_index*kcd->second_cnt+right_index] = kern;
    } else {
	kcd->offsets[right_index*kcd->second_cnt+left_index] = kern;
    }
}

static void KCD_AutoKernAClass(KernClassDlg *kcd,int index,int is_first) {
    char *space[1], **lefts, **rights, **others;
    int lcnt, rcnt; int ocnt, acnt;
    // CID_ClassList is a constant. It presumably provides a base for identifying the two list controls in the KernAClass dialogue, and we assign by which is active, not by which is the first list.
    // Empirical testing suggests that index indexes a unified collection of unique characters spread across the two list views. The empirical testing seems to reflect an incorrect calling sequence, as it turns out.
    // This function expects things to be indexed separately according to the items in the two visible lists.
    GGadget *activelist = GWidgetGetControl( kcd->gw, CID_ClassList+(is_first?0:100));
    GGadget *otherlist = GWidgetGetControl( kcd->gw, CID_ClassList+(is_first?100:0));
    // Each of these returns a GGadget. We then make the assumption that each gadget is nested in a GMatrixEdit and retrieve the GMatrixEdit object.
    struct matrix_data *otherdata = GMatrixEditGet(otherlist,&ocnt);
    struct matrix_data *activedata = GMatrixEditGet(activelist,&acnt);
    int err, touch=0, separation=0, minkern=0, onlyCloser;
    int r2l, i;

    if ( kcd->isv )
return;
    if( acnt <= index )
	return;

    err = false;
    touch = GGadgetIsChecked(GWidgetGetControl(kcd->gw,CID_Touched));
    separation = GetInt8(kcd->gw,CID_Separation,_("Separation"),&err);
    minkern = GetInt8(kcd->gw,CID_MinKern,_("Min Kern"),&err);
    onlyCloser = GGadgetIsChecked(GWidgetGetControl(kcd->gw,CID_OnlyCloser));
    if ( err )
return;

    // We next strdup from activedata[index].u.md_str, which causes a segmentation fault sometimes.
    space[0] = copy(activedata[index].u.md_str);
    // space keeps just the specified item from activelist. others stores the items from the opposite list.
    others = malloc((ocnt+1)*sizeof(char *));
    for ( i=0; i<ocnt; ++i ) {
	if ( i==0 && isEverythingElse(otherdata[0].u.md_str))
	    others[i] = copy("");
	else
	    others[i] = copy(otherdata[i].u.md_str);
    }
    kcd->first_class_new = is_first;
    kcd->r2l = r2l = (kcd->subtable->lookup->lookup_flags & pst_r2l)?1:0;
    kcd->index = index;

    if ( (is_first && !r2l) || (!is_first && r2l) ) {
	lefts = space; lcnt = 1;
	rights = others; rcnt = ocnt;
    } else {
	lefts = others; lcnt = ocnt;
	rights = space; rcnt=1;
    }
    AutoKern2NewClass(kcd->sf,kcd->layer, lefts, rights, lcnt, rcnt,
	    KCD_AddOffset, kcd, separation, minkern, touch, onlyCloser, 0);
    for ( i=0; i<ocnt; ++i )
	free(others[i]);
    free(others);
    free(space[0]);
}

static void KCD_AutoKernAll(KernClassDlg *kcd) {
    char **lefts, **rights, **firsts, **seconds;
    int lcnt, rcnt; int fcnt, scnt;
    GGadget *firstlist = GWidgetGetControl( kcd->gw, CID_ClassList+0);
    GGadget *secondlist = GWidgetGetControl( kcd->gw, CID_ClassList+100);
    struct matrix_data *seconddata = GMatrixEditGet(secondlist,&scnt);
    struct matrix_data *firstdata = GMatrixEditGet(firstlist,&fcnt);
    int err, touch=0, separation=0, minkern=0, onlyCloser;
    int r2l, i;

    if ( kcd->isv )
return;

    err = false;
    touch = GGadgetIsChecked(GWidgetGetControl(kcd->gw,CID_Touched));
    separation = GetInt8(kcd->gw,CID_Separation,_("Separation"),&err);
    minkern = GetInt8(kcd->gw,CID_MinKern,_("Min Kern"),&err);
    onlyCloser = GGadgetIsChecked(GWidgetGetControl(kcd->gw,CID_OnlyCloser));
    if ( err )
return;

    firsts = malloc((fcnt+1)*sizeof(char *));
    for ( i=0; i<fcnt; ++i ) {
	if ( i==0 && isEverythingElse(firstdata[0].u.md_str))
	    firsts[i] = copy("");
	else
	    firsts[i] = copy(firstdata[i].u.md_str);
    }
    seconds = malloc((scnt+1)*sizeof(char *));
    for ( i=0; i<scnt; ++i ) {
	if ( i==0 && isEverythingElse(seconddata[0].u.md_str))
	    seconds[i] = copy("");
	else
	    seconds[i] = copy(seconddata[i].u.md_str);
    }
    kcd->r2l = r2l = (kcd->subtable->lookup->lookup_flags & pst_r2l)?1:0;

    if ( !r2l ) {
	lefts = firsts; lcnt = fcnt;
	rights = seconds; rcnt = scnt;
    } else {
	lefts = seconds; lcnt = scnt;
	rights = firsts; rcnt=fcnt;
    }
    AutoKern2NewClass(kcd->sf,kcd->layer, lefts, rights, lcnt, rcnt,
	    KCD_AddOffsetAsIs, kcd, separation, minkern, touch, onlyCloser, 0);
    for ( i=0; i<fcnt; ++i )
	free(firsts[i]);
    free(firsts);
    for ( i=0; i<scnt; ++i )
	free(seconds[i]);
    free(seconds);
}

/* ************************************************************************** */
/* ************************** Kern Class Display **************************** */
/* ************************************************************************** */

static void KPD_DoCancel(KernClassDlg *kcd) {
    BDFCharFree(kcd->fsc); BDFCharFree(kcd->ssc);
    kcd->fsc = kcd->ssc = NULL;
    free(kcd->active_adjust.corrections); kcd->active_adjust.corrections = NULL;
    free(kcd->orig_adjust.corrections); kcd->orig_adjust.corrections = NULL;
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
	free(kcd->active_adjust.corrections); kcd->active_adjust.corrections = NULL;
	free(kcd->orig_adjust.corrections); kcd->orig_adjust.corrections = NULL;
	kcd->done = true;
    }
return( true );
}

static void KCD_Finalize(KernClassDlg *kcd) {
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_KernOffset));
    unichar_t *end;
    int val = u_strtol(ret,&end,10);

    if ( kcd->old_pos==-1 )
return;

    if ( val<-32768 || val>32767 || *end!='\0' ) {
	ff_post_error( _("Bad Number"), _("Bad Number") );
return;
    }
    kcd->offsets[kcd->old_pos] = val;
    free(kcd->adjusts[kcd->old_pos].corrections);
    kcd->adjusts[kcd->old_pos] = kcd->active_adjust;
    kcd->active_adjust.corrections = NULL;

    BDFCharFree(kcd->fsc); BDFCharFree(kcd->ssc);
    kcd->fsc = kcd->ssc = NULL;
    GDrawRequestExpose(kcd->gw,NULL,false);
    kcd->old_pos = -1;
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
	clut.clut[1] = GDrawGetDefaultBackground(NULL);
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
	uint32_t script = SCScriptFromUnicode(kcd->scf);
	if ( script!=DEFAULT_SCRIPT )
return( ScriptIsRightToLeft( script ));
    }
    if ( kcd->scs!=NULL ) {
	uint32_t script = SCScriptFromUnicode(kcd->scs);
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
//    printf("KCD_KernMouse()\n");

    scale = kcd->pixelsize/(double) (kcd->sf->ascent+kcd->sf->descent);
    kern = u_strtol(_GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_KernOffset)),NULL,10);
    pkern = kcd->magfactor*rint( kern*scale );	/* rounding can't include magnification */

    if ( !kcd->isv ) {
	/* Horizontal */
	width = kcd->magfactor*((kcd->fsc!=NULL?kcd->fsc->width:0)+(kcd->ssc!=NULL?kcd->ssc->width:0))+pkern;
	x = (kcd->subwidth - width)/2;

	if ( KCD_RightToLeft(kcd) ) {
	    if ( kcd->ssc!=NULL )
		width -= kcd->magfactor*kcd->ssc->width;
	} else {
	    if ( kcd->fsc!=NULL ) {
		x += kcd->magfactor*kcd->fsc->width + pkern;
		width -= kcd->magfactor*kcd->fsc->width + pkern;
	    }
	}

	if ( event->u.mouse.y>2*kcd->pixelsize*kcd->magfactor ||
		event->u.mouse.x<x || event->u.mouse.x>x+width ) {
	    if ( event->type == et_mousedown )
return;
	    if ( kcd->within ) {
		GDrawSetCursor(kcd->subw,ct_pointer);
		if ( kcd->down && kcd->orig_kern!=kern ) {
		    sprintf(buf, "%d", kcd->orig_kern);
		    uc_strcpy(ubuf,buf);
		    GGadgetSetTitle(GWidgetGetControl(kcd->gw,CID_KernOffset),ubuf);
		    GDrawRequestExpose(kcd->subw,NULL,false);
		}
		kcd->within = false;
	    }
	    if ( event->type==et_mouseup )
		kcd->down = false;
return;
	}

	if ( !kcd->within ) {
	    GDrawSetCursor(kcd->subw,ct_leftright);
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
		GGadgetSetTitle(GWidgetGetControl(kcd->gw,CID_KernOffset),ubuf);
		GDrawRequestExpose(kcd->subw,NULL,false);
	    }
	    if ( event->type==et_mouseup ) {
		kcd->down = false;
		if ( nkern!=kcd->orig_kern && kcd->active_adjust.corrections!=NULL ) {
		    free(kcd->active_adjust.corrections);
		    kcd->active_adjust.corrections = NULL;
		    ubuf[0] = '0'; ubuf[1] = '\0';
		    GGadgetSetTitle(GWidgetGetControl(kcd->gw,CID_Correction),ubuf);
		    GDrawRequestExpose(kcd->subw,NULL,false);
		}
	    }
	}
    } else {
	/* Vertical */
	y = kcd->pixelsize/3;
	width = (kcd->ssc!=NULL ? kcd->magfactor*rint(kcd->ssc->sc->vwidth * scale) : 0);
	if ( kcd->fsc!=NULL )
	    y += kcd->magfactor*rint(kcd->fsc->sc->vwidth * scale) + pkern;
	x = (kcd->subwidth/2 - kcd->pixelsize/2)*kcd->magfactor;

	if ( event->u.mouse.y<y || event->u.mouse.y>y+width ||
		event->u.mouse.x<x || event->u.mouse.x>x+kcd->pixelsize ) {
	    if ( event->type == et_mousedown )
return;
	    if ( kcd->within ) {
		GDrawSetCursor(kcd->subw,ct_pointer);
		if ( kcd->down && kcd->orig_kern!=kern ) {
		    sprintf(buf, "%d", kcd->orig_kern);
		    uc_strcpy(ubuf,buf);
		    GGadgetSetTitle(GWidgetGetControl(kcd->gw,CID_KernOffset),ubuf);
		    GDrawRequestExpose(kcd->subw,NULL,false);
		}
		kcd->within = false;
	    }
	    if ( event->type==et_mouseup )
		kcd->down = false;
return;
	}

	if ( !kcd->within ) {
	    GDrawSetCursor(kcd->subw,ct_updown);
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
		GGadgetSetTitle(GWidgetGetControl(kcd->gw,CID_KernOffset),ubuf);
		GDrawRequestExpose(kcd->subw,NULL,false);
	    }
	    if ( event->type==et_mouseup ) {
		kcd->down = false;
		if ( nkern!=kcd->orig_kern && kcd->active_adjust.corrections!=NULL ) {
		    free(kcd->active_adjust.corrections);
		    kcd->active_adjust.corrections = NULL;
		    ubuf[0] = '0'; ubuf[1] = '\0';
		    GGadgetSetTitle(GWidgetGetControl(kcd->gw,CID_Correction),ubuf);
		    GDrawRequestExpose(kcd->subw,NULL,false);
		}
	    }
	}
    }
}

static void KCD_KernExpose(KernClassDlg *kcd,GWindow pixmap,GEvent *event) {
    int x, y;
    SplineFont *sf = kcd->sf;
    int em = sf->ascent+sf->descent;
    int as = kcd->magfactor*rint(sf->ascent*kcd->pixelsize/(double) em);
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_KernOffset));
    int kern = u_strtol(ret,NULL,10);
    int baseline, xbaseline;

//    printf("KCD_KernExpose() ssc:%p fsc:%p\n", kcd->ssc, kcd->fsc );

    kern = kcd->magfactor*rint(kern*kcd->pixelsize/(double) em);

    { int correction;
	unichar_t *end;

	ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_Correction));
	correction = u_strtol(ret,&end,10);
	while ( *end==' ' ) ++end;
	if ( *end=='\0' && correction>=-128 && correction<=127 )
	    kern += correction*kcd->magfactor;
    }

    if ( !kcd->isv ) {
	x = (kcd->subwidth-( kcd->magfactor*(kcd->fsc!=NULL?kcd->fsc->width:0)+
		kcd->magfactor*(kcd->ssc!=NULL?kcd->ssc->width:0)+
		kern))/2;
	baseline = 0 + as + kcd->magfactor*kcd->pixelsize/2;
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
	y = kcd->magfactor*kcd->pixelsize/3 + as;
	xbaseline = kcd->subwidth/2;
	if ( kcd->fsc!=NULL ) {
	    KCD_DrawGlyph(pixmap,xbaseline-kcd->magfactor*kcd->pixelsize/2,y,kcd->fsc,kcd->magfactor);
	    y += kcd->magfactor*rint(kcd->fsc->sc->vwidth * kcd->pixelsize/(double) em) + kern;
	}
	if ( kcd->ssc!=NULL )
	    KCD_DrawGlyph(pixmap,xbaseline-kcd->magfactor*kcd->pixelsize/2,y,kcd->ssc,kcd->magfactor);
    }
}

static int KCD_KernOffChanged(GGadget *g, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged )
	GDrawRequestExpose(kcd->subw,NULL,false);
return( true );
}

static void KCD_UpdateGlyphFromName(KernClassDlg *kcd,int which,char* glyphname)
{
    BDFChar **scpos = which==0 ? &kcd->fsc : &kcd->ssc;
    SplineChar **possc = which==0 ? &kcd->scf : &kcd->scs;
    SplineChar *sc;
    void *freetypecontext=NULL;
//    printf("KCD_UpdateGlyphFromName() which:%d iskp:%d\n", which, kcd->iskernpair);

    char* localglyphname = copy( glyphname );
    char* p = 0;
    if((p = strstr( localglyphname, " " )))
	*p = '\0';

    BDFCharFree(*scpos);
    *scpos = NULL;

    *possc = sc = SFGetChar( kcd->sf, -1, localglyphname);
    free( localglyphname );

    if ( sc==NULL )
	return;

    if ( GGadgetIsChecked(GWidgetGetControl(kcd->gw,CID_FreeType)) )
	freetypecontext = FreeTypeFontContext(sc->parent,sc,sc->parent->fv,kcd->layer);
    if ( freetypecontext )
    {
	*scpos = SplineCharFreeTypeRasterize(freetypecontext,sc->orig_pos,kcd->pixelsize,72,8);
	FreeTypeFreeContext(freetypecontext);
    }
    else
    {
	*scpos = SplineCharAntiAlias(sc,kcd->layer,kcd->pixelsize,4);
    }

//    printf("KCD_UpdateGlyph() scpos:%p\n", *scpos );
}

static void KCD_UpdateGlyph(KernClassDlg *kcd,int which) {
    BDFChar **scpos = which==0 ? &kcd->fsc : &kcd->ssc;
    SplineChar **possc = which==0 ? &kcd->scf : &kcd->scs;
    SplineChar *sc;
    char *temp;
    void *freetypecontext=NULL;
//    printf("KCD_UpdateGlyph() which:%d iskp:%d\n", which, kcd->iskernpair);

    BDFCharFree(*scpos);
    *scpos = NULL;
    if ( kcd->iskernpair )
    {
	temp = cu_copy(_GGadgetGetTitle(GWidgetGetControl(kcd->gw,
		which==0 ? CID_First : CID_Second )));
    }
    else
    {
	GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(kcd->gw,
		which==0 ? CID_First : CID_Second ));
	if ( sel==NULL )
	{
//	    printf("KCD_UpdateGlyph() which:%d no selection...returning\n", which );
	    return;
	}
	else
	{
	    temp = cu_copy(sel->text);
	}

//	printf("KCD_UpdateGlyph() temp:%s\n", temp );
    }

    *possc = sc = SFGetChar(kcd->sf,-1,temp);
    free(temp);
    if ( sc==NULL )
	return;
    if ( GGadgetIsChecked(GWidgetGetControl(kcd->gw,CID_FreeType)) )
	freetypecontext = FreeTypeFontContext(sc->parent,sc,sc->parent->fv,kcd->layer);
    if ( freetypecontext )
    {
	*scpos = SplineCharFreeTypeRasterize(freetypecontext,sc->orig_pos,kcd->pixelsize,72,8);
	FreeTypeFreeContext(freetypecontext);
    }
    else
    {
	*scpos = SplineCharAntiAlias(sc,kcd->layer,kcd->pixelsize,4);
    }

//    printf("KCD_UpdateGlyph() scpos:%p\n", *scpos );
}

static void _KCD_DisplaySizeChanged(KernClassDlg *kcd) {
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(kcd->gw,CID_DisplaySize));
    unichar_t *end;
    int pixelsize = u_strtol(ret,&end,10);

    while ( *end==' ' ) ++end;
    if ( pixelsize>4 && pixelsize<400 && *end=='\0' ) {
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
	kcd->pixelsize = pixelsize;
	KCD_UpdateGlyph(kcd,0);
	KCD_UpdateGlyph(kcd,1);
	GDrawRequestExpose(kcd->subw,NULL,false);
    }
}

static int KCD_DisplaySizeChanged(GGadget *g, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	_KCD_DisplaySizeChanged(kcd);
    }
return( true );
}

static int KCD_MagnificationChanged(GGadget *g, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	int mag = GGadgetGetFirstListSelectedItem(GWidgetGetControl(kcd->gw,CID_Magnifications));

	if ( mag!=-1 && mag!=kcd->magfactor-1 ) {
	    kcd->magfactor = mag+1;
	    GDrawRequestExpose(kcd->subw,NULL,false);
	}
    }
return( true );
}

static int KCB_FreeTypeChanged(GGadget *g, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	KCD_UpdateGlyph(kcd,0);
	KCD_UpdateGlyph(kcd,1);
	GDrawRequestExpose(kcd->subw,NULL,false);
    }
return( true );
}

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
	GDrawRequestExpose(kcd->subw,NULL,false);
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

static int KCD_RevertKerning(GGadget *g, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	char buf[20];
	sprintf( buf, "%d", kcd->orig_kern_offset );
	GGadgetSetTitle8(GWidgetGetControl(kcd->gw,CID_KernOffset),buf);
	free(kcd->active_adjust.corrections);
	kcd->active_adjust = kcd->orig_adjust;
	if ( kcd->orig_adjust.corrections!=NULL ) {
	    int len = kcd->orig_adjust.last_pixel_size-kcd->orig_adjust.first_pixel_size+1;
	    kcd->active_adjust = kcd->orig_adjust;
	    kcd->active_adjust.corrections = malloc(len);
	    memcpy(kcd->active_adjust.corrections,kcd->orig_adjust.corrections,len);
	}
	_KCD_DisplaySizeChanged(kcd);
    }
return( true );
}

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
	if ( kp==NULL && offset==0 && kcd->active_adjust.corrections==NULL )
return(true);
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
    }
return( true );
}

static void KCD_SetDevTab(KernClassDlg *kcd) {
    unichar_t ubuf[20];

    ubuf[0] = '0'; ubuf[1] = '\0';
    GGadgetClearList(GWidgetGetControl(kcd->gw,CID_DisplaySize));
    if ( kcd->active_adjust.corrections!=NULL ) {
	int i;
	int len = kcd->active_adjust.last_pixel_size - kcd->active_adjust.first_pixel_size +1;
	char buffer[20];
	GTextInfo **ti = malloc((len+1)*sizeof(GTextInfo *));
	for ( i=0; i<len; ++i ) {
	    ti[i] = calloc(1,sizeof(GTextInfo));
	    sprintf( buffer, "%d", i+kcd->active_adjust.first_pixel_size);
	    ti[i]->text = uc_copy(buffer);
	    ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	}
	ti[i] = calloc(1,sizeof(GTextInfo));
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

static void KP_SelectSubtable(KernClassDlg *kcd,struct lookup_subtable *sub) {
    int32_t len;
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

    free(kcd->active_adjust.corrections); kcd->active_adjust.corrections = NULL;
    if ( kcd->scf!=NULL && kcd->scs!=NULL ) {
	for ( kp = kcd->isv?kcd->scf->vkerns:kcd->scf->kerns; kp!=NULL && kp->sc!=kcd->scs; kp=kp->next );
	if ( kp!=NULL ) {
	    offset = kp->off;
	    kcd->orig_kern_offset = offset;
	    KP_SelectSubtable(kcd,kp->subtable);
	    if ( kp->adjust!=NULL ) {
		int len = kp->adjust->last_pixel_size-kp->adjust->first_pixel_size+1;
		kcd->active_adjust = *kp->adjust;
		kcd->active_adjust.corrections = malloc(len);
		memcpy(kcd->active_adjust.corrections,kp->adjust->corrections,len);
		kcd->orig_adjust = *kp->adjust;
		kcd->orig_adjust.corrections = malloc(len);
		memcpy(kcd->orig_adjust.corrections,kp->adjust->corrections,len);
	    }
	}
    }
    if ( kp==NULL && kcd->scf!=NULL ) {
	int32_t len;
	GTextInfo **ti = GGadgetGetList(GWidgetGetControl(kcd->gw,CID_Subtable),&len);
	uint32_t script = SCScriptFromUnicode(kcd->scf);
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
    GGadgetSetTitle(GWidgetGetControl(kcd->gw,CID_KernOffset),ubuf);
    KCD_SetDevTab(kcd);
}

static void KPD_BuildKernList(KernClassDlg *kcd) {
    int len;
    KernPair *kp;
    GTextInfo **ti;

    len = 0;
    if ( kcd->scf!=NULL )
	for ( kp=kcd->isv?kcd->scf->vkerns:kcd->scf->kerns, len=0; kp!=NULL; kp=kp->next )
	    ++len;
    ti = calloc(len+1,sizeof(GTextInfo*));
    if ( kcd->scf!=NULL )
	for ( kp=kcd->isv?kcd->scf->vkerns:kcd->scf->kerns, len=0; kp!=NULL; kp=kp->next, ++len ) {
	    ti[len] = calloc(1,sizeof(GTextInfo));
	    ti[len]->fg = ti[len]->bg = COLOR_DEFAULT;
	    ti[len]->text = uc_copy(kp->sc->name);
	}
    ti[len] = calloc(1,sizeof(GTextInfo));
    GGadgetSetList(GWidgetGetControl(kcd->gw,CID_Second),ti,false);
}

static int KCD_GlyphSelected(GGadget *g, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    int which = GGadgetGetCid(g)==CID_Second;
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	KCD_UpdateGlyph(kcd,which);
	GDrawRequestExpose(kcd->subw,NULL,false);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	if ( !KPD_FinishKP(kcd)) {
	    KPD_RestoreGlyphs(kcd);
return( true );
	}
	KCD_UpdateGlyph(kcd,which);
	if ( which==0 )
	    KPD_BuildKernList(kcd);
	KPD_PairSearch(kcd);
	GDrawRequestExpose(kcd->subw,NULL,false);
    }
return( true );
}

static GTextInfo **TiNamesFromClass(GGadget *list,int class_index) {
    /* Return a list containing all the names in this class */
    char *pt, *end;
    GTextInfo **ti;
    int cnt;
    struct matrix_data *classes = GMatrixEditGet(list,&cnt);
    char *class_str = classes[class_index].u.md_str;
    int i, k;

    if ( class_str==NULL || isEverythingElse(class_str) ) {
	i=0;
	ti = malloc((i+1)*sizeof(GTextInfo*));
    } else {
	for ( k=0 ; k<2; ++k ) {
	    for ( i=0, pt=class_str; *pt; ) {
		while ( *pt==' ' ) ++pt;
		if ( *pt=='\0' )
	    break;
		for ( end = pt; *end!='\0' && *end!=' '; ++end );
		if ( k==1 ) {
		    ti[i] = calloc(1,sizeof(GTextInfo));
		    ti[i]->text = utf82u_copyn(pt,end-pt);
		    ti[i]->bg = ti[i]->fg = COLOR_DEFAULT;
		}
		++i;
		pt = end;
	    }
	    if ( k==0 )
		ti = malloc((i+1)*sizeof(GTextInfo*));
	}
    }
    if ( i>0 )
	ti[0]->selected = true;
    ti[i] = calloc(1,sizeof(GTextInfo));
return( ti );
}

static void KCD_EditOffset(KernClassDlg *kcd, int first, int second) {
    char buf[12];
    unichar_t ubuf[12];
    GTextInfo **ti;
    static unichar_t nullstr[] = { 0 };

    KCD_Finalize(kcd);
    if ( GMatrixEditGetActiveRow(GWidgetGetControl(kcd->gw,CID_ClassList))!=first )
	GMatrixEditActivateRowCol(GWidgetGetControl(kcd->gw,CID_ClassList),first,-1);
    if ( GMatrixEditGetActiveRow(GWidgetGetControl(kcd->gw,CID_ClassList+100))!=second )
	GMatrixEditActivateRowCol(GWidgetGetControl(kcd->gw,CID_ClassList+100),second,-1);
    if ( second==0 )
	ff_post_notice(_("Class 0"),_("The kerning values for class 0 (\"Everything Else\") should always be 0"));
    if ( first!=-1 && second!=-1 && first < kcd->first_cnt && second < kcd->second_cnt ) {
	kcd->st_pos = first*kcd->second_cnt+second;
	kcd->old_pos = kcd->st_pos;
	GGadgetSetList(GWidgetGetControl(kcd->gw,CID_First),
		ti = TiNamesFromClass(GWidgetGetControl(kcd->gw,CID_ClassList),first),false);
	GGadgetSetTitle(GWidgetGetControl(kcd->gw,CID_First),
		ti==NULL || ti[0]->text==NULL ? nullstr: ti[0]->text);
	GGadgetSetList(GWidgetGetControl(kcd->gw,CID_Second),
		ti = TiNamesFromClass(GWidgetGetControl(kcd->gw,CID_ClassList+100),second),false);
	GGadgetSetTitle(GWidgetGetControl(kcd->gw,CID_Second),
		ti==NULL || ti[0]->text==NULL ? nullstr: ti[0]->text);
	KCD_UpdateGlyph(kcd,0);
	KCD_UpdateGlyph(kcd,1);

	kcd->orig_kern_offset = kcd->offsets[kcd->st_pos];
	sprintf( buf, "%d", kcd->offsets[kcd->st_pos]);
	uc_strcpy(ubuf,buf);
	GGadgetSetTitle(GWidgetGetControl(kcd->gw,CID_KernOffset),ubuf);

	kcd->active_adjust = kcd->adjusts[kcd->st_pos];
	kcd->orig_adjust = kcd->adjusts[kcd->st_pos];
	if ( kcd->active_adjust.corrections!=NULL ) {
	    int len = kcd->active_adjust.last_pixel_size - kcd->active_adjust.first_pixel_size +1;
	    kcd->active_adjust.corrections = malloc(len);
	    memcpy(kcd->active_adjust.corrections,kcd->adjusts[kcd->st_pos].corrections,len);
	    kcd->orig_adjust.corrections = malloc(len);
	    memcpy(kcd->orig_adjust.corrections,kcd->adjusts[kcd->st_pos].corrections,len);
	}
	KCD_SetDevTab(kcd);
    }
    GDrawRequestExpose(kcd->subw,NULL,false);
    GDrawRequestExpose(kcd->gw,NULL,false);
}

/* ************************************************************************** */
/* *************************** Kern Class Dialog **************************** */
/* ************************************************************************** */

static void KC_DoResize(KernClassDlg *kcd) {
    GRect wsize, csize;

    GDrawGetSize(kcd->gw,&wsize);

    kcd->fullwidth = wsize.width;
    kcd->width = wsize.width-kcd->xstart2-5;
    kcd->height = wsize.height-kcd->ystart2;
    if ( kcd->hsb!=NULL ) {
	GGadgetGetSize(kcd->hsb,&csize);
	kcd->width = csize.width;
	kcd->xstart2 = csize.x;
	GGadgetGetSize(kcd->vsb,&csize);
	kcd->ystart2 = csize.y;
	kcd->height = csize.height;
	kcd->xstart = kcd->xstart2-kcd->kernw;
	kcd->ystart = kcd->ystart2-kcd->fh-1;
	KCD_SBReset(kcd);
    }
    GDrawRequestExpose(kcd->gw,NULL,false);
}

static int KC_ShowHideKernPane(GGadget *g, GEvent *e) {
    static int cidlist[] = { CID_First, CID_Second, CID_FreeType, CID_SizeLabel,
	    CID_DisplaySize, CID_MagLabel,CID_Magnifications, CID_OffsetLabel,
	    CID_KernOffset,
	    CID_CorrectLabel, CID_Correction, CID_Revert, CID_ClearDevice,
	    CID_Display, 0 };
    if ( e==NULL ||
	    (e->type==et_controlevent && e->u.control.subtype == et_radiochanged) ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	int i;

	show_kerning_pane_in_class = GGadgetIsChecked(g);

	for ( i=0; cidlist[i]!=0; ++i )
	    GGadgetSetVisible(GWidgetGetControl(kcd->gw,cidlist[i]),show_kerning_pane_in_class);
	GHVBoxReflow(GWidgetGetControl(kcd->gw,CID_TopBox));
	KC_DoResize(kcd);
	if ( e!=NULL )
	    SavePrefs(true);
    }
return( true );
}

static int KC_OK(GGadget *g, GEvent *e) {
    SplineFont *sf;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	KernClass *kc;
	int i;
	int len;
	struct matrix_data *classes;
	int err, touch=0, separation=0, minkern=0, onlyCloser, autokern;

	sf = kcd->sf;
	if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
	else if ( sf->mm!=NULL ) sf = sf->mm->normal;

	err = false;
	touch = GGadgetIsChecked(GWidgetGetControl(kcd->gw,CID_Touched));
	separation = GetInt8(kcd->gw,CID_Separation,_("Separation"),&err);
	minkern = GetInt8(kcd->gw,CID_MinKern,_("Min Kern"),&err);
	onlyCloser = GGadgetIsChecked(GWidgetGetControl(kcd->gw,CID_OnlyCloser));
	autokern = GGadgetIsChecked(GWidgetGetControl(kcd->gw,CID_Autokern));
	if ( err )
return( true );
	KCD_Finalize(kcd);

	kc = kcd->orig;
	for ( i=1; i<kc->first_cnt; ++i )
	    free( kc->firsts[i]);
	for ( i=1; i<kc->second_cnt; ++i )
	    free( kc->seconds[i]);
	free(kc->firsts);
	free(kc->seconds);
	free(kc->offsets);
	free(kc->adjusts);

	// Group kerning.
	if (kc->firsts_names)
	  for ( i=1; i<kc->first_cnt; ++i )
	    if (kc->firsts_names[i]) free(kc->firsts_names[i]);
	if (kc->seconds_names)
	  for ( i=1; i<kc->second_cnt; ++i )
	    if (kc->seconds_names[i]) free(kc->seconds_names[i]);
	if (kc->firsts_flags) free(kc->firsts_flags);
	if (kc->seconds_flags) free(kc->seconds_flags);
	if (kc->offsets_flags) free(kc->offsets_flags);
	if (kc->firsts_names) free(kc->firsts_names);
	if (kc->seconds_names) free(kc->seconds_names);

	kc->subtable->separation = separation;
	kc->subtable->minkern = minkern;
	kc->subtable->kerning_by_touch = touch;
	kc->subtable->onlyCloser = onlyCloser;
	kc->subtable->dontautokern = !autokern;

	kc->first_cnt = kcd->first_cnt;
	kc->second_cnt = kcd->second_cnt;
	kc->firsts = malloc(kc->first_cnt*sizeof(char *));
	kc->seconds = malloc(kc->second_cnt*sizeof(char *));
	kc->firsts[0] = kc->seconds[0] = NULL;
	classes = GMatrixEditGet(GWidgetGetControl(kcd->gw,CID_ClassList),&len);
	if ( !isEverythingElse(classes[0].u.md_str) )
	    kc->firsts[0] = GlyphNameListDeUnicode(classes[0].u.md_str);
	for ( i=1; i<kc->first_cnt; ++i )
	    kc->firsts[i] = GlyphNameListDeUnicode(classes[i].u.md_str);
	classes = GMatrixEditGet(GWidgetGetControl(kcd->gw,CID_ClassList+100),&len);
	for ( i=1; i<kc->second_cnt; ++i )
	    kc->seconds[i] = GlyphNameListDeUnicode(classes[i].u.md_str);
	kc->offsets = kcd->offsets;
	kc->adjusts = kcd->adjusts;

	// Group kerning.
	kc->firsts_flags = kcd->firsts_flags;
	kc->seconds_flags = kcd->seconds_flags;
	kc->offsets_flags = kcd->offsets_flags;
	kc->firsts_names = kcd->firsts_names;
	kc->seconds_names = kcd->seconds_names;

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
	{ int i;
	    for ( i=0; i<kcd->first_cnt*kcd->second_cnt; ++i )
		free(kcd->adjusts[i].corrections);
	}
	free(kcd->adjusts);

	// Group kerning.
	if (kcd->firsts_names) {
          int i;
	  for ( i=1; i<kcd->first_cnt; ++i )
	    if (kcd->firsts_names[i]) free(kcd->firsts_names[i]);
        }
	if (kcd->seconds_names) {
          int i;
	  for ( i=1; i<kcd->second_cnt; ++i )
	    if (kcd->seconds_names[i]) free(kcd->seconds_names[i]);
        }
	if (kcd->firsts_flags) free(kcd->firsts_flags);
	if (kcd->seconds_flags) free(kcd->seconds_flags);
	if (kcd->offsets_flags) free(kcd->offsets_flags);
	if (kcd->firsts_names) free(kcd->firsts_names);
	if (kcd->seconds_names) free(kcd->seconds_names);

	GDrawDestroyWindow(kcd->gw);
    }
}

static int KC_Cancel(GGadget *g, GEvent *e) {
    KernClassDlg *kcd;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	kcd = GDrawGetUserData(GGadgetGetWindow(g));

	KC_DoCancel(kcd);
    }
return( true );
}

static int KCD_TextSelect(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
	int off = GGadgetGetCid(g)-CID_ClassSelect;
	const unichar_t *uname = _GGadgetGetTitle(g), *upt;
	GGadget *list = GWidgetGetControl(kcd->gw,CID_ClassList+off);
	int rows;
	struct matrix_data *classes = GMatrixEditGet(list,&rows);
	int nlen;
	char *start, *pt, *name;
	int i;

        /* length of initial text contents up until blank, '(' or end-of-string */
        for ( upt=uname; *upt!='\0' && *upt!='(' && *upt!=' '; ++upt );
	name = u2utf8_copyn(uname,upt-uname);
        /* if string empty or invalid for any reason, quit processing text */
        if ( name==NULL )
            return( false );
	nlen = strlen(name);

	for ( i=0; i<rows; ++i ) {
	    for ( start = classes[i].u.md_str; start!=NULL && *start!='\0'; ) {
		while ( *start==' ' ) ++start;
                for ( pt=start; *pt!='\0' && *pt!=' ' && *pt!='('; ++pt );
		if ( pt-start == nlen && strncmp(name,start,nlen)==0 ) {
		    GMatrixEditScrollToRowCol(list,i,0);
		    GMatrixEditActivateRowCol(list,i,0);
		    if ( off==0 )
			KCD_VShow(kcd,i);
		    else
			KCD_HShow(kcd,i);
                    return( true );
		}
		if ( *pt=='(' ) {
		    while ( *pt!=')' && *pt!='\0' ) ++pt;
		    if ( *pt==')' ) ++pt;
		}
		start = pt;
	    }
	}

	/* Otherwise deselect everything */
	if ( nlen!=0 )
	    GMatrixEditActivateRowCol(list,-1,-1);
    }
return( true );
}

#define MID_Clear		1000
#define MID_ClearAll		1001
#define MID_ClearDevTab		1002
#define MID_ClearAllDevTab	1003
#define MID_AutoKernRow		1004
#define MID_AutoKernCol		1005
#define MID_AutoKernAll		1006

static void kernmenu_dispatch(GWindow gw, GMenuItem *mi, GEvent *e) {
    KernClassDlg *kcd = GDrawGetUserData(gw);
    int i;

    switch ( mi->mid ) {
      case MID_AutoKernRow:
	KCD_AutoKernAClass(kcd,kcd->st_pos/kcd->second_cnt,true);
      break;
      case MID_AutoKernCol:
	KCD_AutoKernAClass(kcd,kcd->st_pos%kcd->second_cnt,false);
      break;
      case MID_AutoKernAll:
	KCD_AutoKernAll(kcd);
      break;
      case MID_Clear:
	kcd->offsets[kcd->st_pos] = 0;
      break;
      case MID_ClearAll:
	for ( i=0; i<kcd->first_cnt*kcd->second_cnt; ++i )
	    kcd->offsets[i] = 0;
	if (kcd->offsets_flags != NULL)
	  for ( i=0; i<kcd->first_cnt*kcd->second_cnt; ++i )
	    kcd->offsets_flags[i] = 0;
      break;
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
    }
    kcd->st_pos = -1;
    GDrawRequestExpose(kcd->gw,NULL,false);
}

static GMenuItem kernpopupmenu[] = {
    { { (unichar_t *) N_("AutoKern Row"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, '\0', ksm_control, NULL, NULL, kernmenu_dispatch, MID_AutoKernRow },
    { { (unichar_t *) N_("AutoKern Column"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, '\0', ksm_control, NULL, NULL, kernmenu_dispatch, MID_AutoKernCol },
    { { (unichar_t *) N_("AutoKern All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, '\0', ksm_control, NULL, NULL, kernmenu_dispatch, MID_AutoKernAll },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, '\0', 0, NULL, NULL, NULL, 0 }, /* line */
#define Menu_VKern_Offset 4		/* No autokerning for vertical kerning */
    { { (unichar_t *) N_("Clear"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, '\0', ksm_control, NULL, NULL, kernmenu_dispatch, MID_Clear },
    { { (unichar_t *) N_("Clear All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, '\0', ksm_control, NULL, NULL, kernmenu_dispatch, MID_ClearAll },
    { { (unichar_t *) N_("Clear Device Table"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, kernmenu_dispatch, MID_ClearDevTab },
    { { (unichar_t *) N_("Clear All Device Tables"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, kernmenu_dispatch, MID_ClearAllDevTab },
    GMENUITEM_EMPTY
};

static void KCD_PopupMenu(KernClassDlg *kcd,GEvent *event,int pos) {
    kcd->st_pos = pos;
    if ( kcd->isv )
	GMenuCreatePopupMenu(event->w,event, kernpopupmenu+Menu_VKern_Offset);
    else
	GMenuCreatePopupMenu(event->w,event, kernpopupmenu);
}

static void KCD_Mouse(KernClassDlg *kcd,GEvent *event) {
    static char space[200];
    int len;
    struct matrix_data *classes;
    int pos = ((event->u.mouse.y-kcd->ystart2)/kcd->kernh + kcd->offtop) * kcd->second_cnt +
	    (event->u.mouse.x-kcd->xstart2)/kcd->kernw + kcd->offleft;

    GGadgetEndPopup();
//    printf("KCD_Mouse()\n");

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
	GGadgetDispatchEvent(kcd->vsb,event);
return;
    }

    if ( event->u.mouse.x<kcd->xstart || event->u.mouse.x>kcd->xstart2+kcd->fullwidth ||
	    event->u.mouse.y<kcd->ystart || event->u.mouse.y>kcd->ystart2+kcd->height )
return;

    if ( event->type==et_mousemove ) {
	int c = (event->u.mouse.x - kcd->xstart2)/kcd->kernw + kcd->offleft;
	int s = (event->u.mouse.y - kcd->ystart2)/kcd->kernh + kcd->offtop;
	char *str;
	//space[0] = '\0';
	memset(space,'\0',sizeof(space));
	if ( event->u.mouse.y>=kcd->ystart2 && s<kcd->first_cnt ) {
	    sprintf( space, _("First Class %d\n"), s );
	    classes = GMatrixEditGet(GWidgetGetControl(kcd->gw,CID_ClassList),&len);
	    str = classes[s].u.md_str!=NULL ? classes[s].u.md_str :
		    s==0 ? _("{Everything Else}") : "";
	    len = strlen(space);
	    strncpy(space+len,str,sizeof(space)/2-2 - len);
	    space[sizeof(space)/2-2] = '\0';
	    utf8_truncatevalid(space+len);
	    strcat(space+strlen(space),"\n");
	}
	if ( event->u.mouse.x>=kcd->xstart2 && c<kcd->second_cnt ) {
	    len = strlen(space);
	    sprintf( space+len, _("Second Class %d\n"), c );
	    classes = GMatrixEditGet(GWidgetGetControl(kcd->gw,CID_ClassList+100),&len);
	    str = classes[c].u.md_str!=NULL ? classes[c].u.md_str :
		    c==0 ? _("{Everything Else}") : "";
	    len = strlen(space);
	    strncpy(space+len,str,sizeof(space)-1 - len);
	    space[sizeof(space)-1] = '\0';
	    utf8_truncatevalid(space+len);
	}
	if ( space[0]=='\0' )
return;
	if ( space[strlen(space)-1]=='\n' )
	    space[strlen(space)-1]='\0';
	GGadgetPreparePopup8(kcd->gw,space);
    } else if ( event->u.mouse.x<kcd->xstart2 || event->u.mouse.y<kcd->ystart2 )
return;
    else if ( event->type==et_mousedown && event->u.mouse.button==3 )
	KCD_PopupMenu(kcd,event,pos);
    else if ( event->type==et_mousedown )
	kcd->st_pos = pos;
    else if ( event->type==et_mouseup ) {
//	printf("KCD_Mouse(up)\n");
	if ( pos==kcd->st_pos )
	    KCD_EditOffset(kcd, pos/kcd->second_cnt, pos%kcd->second_cnt);
    }
}

static int KCD_NameClass(SplineFont *sf,char *buf,int blen,char *class_str) {
    char *start, *pt, *bpt;
    int i, ch;
    SplineChar *sc;

    if ( class_str==NULL ) {
	utf8_idpb(buf,0x2205,0);	/* Empty set glyph */
return( true );
    }
    if ( isEverythingElse(class_str)) {
 /* GT: Short form of {Everything Else}, might use universal? U+2200 */
	strcpy(buf,_("{All}") );
return( true );
    }
    for ( start=class_str; *start==' '; ++start );
    bpt = buf;
    for ( i=0; i<2; ++i ) {
	for ( pt=start; *pt!='(' && *pt!=' ' && *pt!='\0'; ++pt );
	if ( *pt=='(' && (pt[2]==')' || pt[3]==')' || pt[4]==')' || pt[5]==')')) {
	    ++pt;
	    while ( *pt!=')' )
		*bpt++ = *pt++;
	    ++pt;
	} else if ( isalpha(*(unsigned char *) start) && pt-start==1 && *pt!='(' ) {
	    *bpt++ = *start;
	} else
    break;
	for ( ; *pt==' '; ++pt );
	start = pt;
	if ( *start=='\0' ) {
	    *bpt = '\0';
return( false );
	}
	*bpt++ = ' ';
    }
    if ( i!=0 ) {
	/* We parsed at least one glyph, and there's more stuff */
	bpt[-1] = '.'; *bpt++ = '.'; *bpt++ = '.';
	*bpt = '\0';
return( false );
    }

    ch = *pt; *pt='\0';
    sc = SFGetChar(sf,-1,start);
    if ( sc==NULL ) {
	snprintf( buf, blen, "!%s", start );
	*pt = ch;
return( true );
    } else if ( sc->unicodeenc==-1 || isprivateuse(sc->unicodeenc)
	       || issurrogate(sc->unicodeenc))	/* Pango complains that privateuse code points are "Invalid UTF8 strings" */
	snprintf( buf, blen, "%s", start );
    else {
	char *bpt = utf8_idpb(buf,sc->unicodeenc,0);
	*bpt = '\0';
    }
    *pt = ch;
return( false );
}

static void KCD_Expose(KernClassDlg *kcd,GWindow pixmap,GEvent *event) {
    GRect *area = &event->u.expose.rect;
    GRect rect, select,r;
    GRect clip,old1,old2,old3;
    Color fg = GDrawGetDefaultForeground(NULL);
    int len, i, j, x, y;
    char buf[100];
    int fcnt, scnt;
    GGadget *first = GWidgetGetControl(kcd->gw,CID_ClassList);
    GGadget *second = GWidgetGetControl(kcd->gw,CID_ClassList+100);
    struct matrix_data *fclasses = GMatrixEditGet(first,&fcnt);
    struct matrix_data *sclasses = GMatrixEditGet(second,&scnt);
    int factive = GMatrixEditGetActiveRow(first);
    int sactive = GMatrixEditGetActiveRow(second);

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
    for ( i=0 ; kcd->offtop+i<=kcd->first_cnt && (i-1)*kcd->kernh<kcd->height; ++i ) {
	if ( i+kcd->offtop<fcnt && i+kcd->offtop==factive ) {
	    select.x = kcd->xstart+1; select.y = kcd->ystart2+i*kcd->kernh+1;
	    select.width = rect.width-1; select.height = kcd->kernh-1;
	    GDrawFillRect(pixmap,&select,ACTIVE_BORDER);
	}
    }
    for ( i=0 ; kcd->offleft+i<=kcd->second_cnt && (i-1)*kcd->kernw<kcd->fullwidth; ++i ) {
	if ( i+kcd->offleft<scnt && i+kcd->offleft==sactive ) {
	    select.x = kcd->xstart2+i*kcd->kernw+1; select.y = kcd->ystart+1;
	    select.width = kcd->kernw-1; select.height = rect.height-1;
	    GDrawFillRect(pixmap,&select,ACTIVE_BORDER);
	}
    }

    for ( i=0 ; kcd->offtop+i<=kcd->first_cnt && (i-1)*kcd->kernh<kcd->height; ++i ) {
	GDrawDrawLine(pixmap,kcd->xstart,kcd->ystart2+i*kcd->kernh,kcd->xstart+rect.width,kcd->ystart2+i*kcd->kernh,fg);
	if ( i+kcd->offtop<kcd->first_cnt ) {
	    int err = KCD_NameClass(kcd->sf,buf,sizeof(buf),fclasses[i+kcd->offtop].u.md_str);
	    int fg = err ? GDrawGetWarningForeground(NULL) : kernclass_classfgcol;
	    len = GDrawGetText8Width(pixmap,buf,-1);
	    if ( len<=kcd->kernw )
		GDrawDrawText8(pixmap,kcd->xstart+(kcd->kernw-len)/2,kcd->ystart2+i*kcd->kernh+kcd->as+1,
			buf,-1,fg);
	    else {
		r.x = kcd->xstart; r.width = kcd->kernw;
		r.y = kcd->ystart2+i*kcd->kernh-1; r.height = kcd->kernh+1;
		GDrawPushClip(pixmap,&r,&old3);
		GDrawDrawText8(pixmap,r.x,r.y+kcd->as+1,
			buf,-1,fg);
		GDrawPopClip(pixmap,&old3);
	    }
	}
    }
    for ( i=0 ; kcd->offleft+i<=scnt && (i-1)*kcd->kernw<kcd->fullwidth; ++i ) {
	GDrawDrawLine(pixmap,kcd->xstart2+i*kcd->kernw,kcd->ystart,kcd->xstart2+i*kcd->kernw,kcd->ystart+rect.height,fg);
	if ( i+kcd->offleft<kcd->second_cnt ) {
	    int err = KCD_NameClass(kcd->sf,buf,sizeof(buf),sclasses[i+kcd->offleft].u.md_str);
	    int fg = err ? GDrawGetWarningForeground(NULL) : kernclass_classfgcol;
	    len = GDrawGetText8Width(pixmap,buf,-1);
	    if ( len<=kcd->kernw )
		GDrawDrawText8(pixmap,kcd->xstart2+i*kcd->kernw+(kcd->kernw-len)/2,kcd->ystart+kcd->as+1,
		    buf,-1,fg);
	    else {
		r.x = kcd->xstart2+i*kcd->kernw; r.width = kcd->kernw;
		r.y = kcd->ystart-1; r.height = kcd->kernh+1;
		GDrawPushClip(pixmap,&r,&old3);
		GDrawDrawText8(pixmap,r.x,r.y+kcd->as+1,
			buf,-1,fg);
		GDrawPopClip(pixmap,&old3);
	    }
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
	    len = GDrawGetText8Width(pixmap,buf,-1);
	    GDrawDrawText8(pixmap,x+kcd->kernw-3-len,y+kcd->as+1,
		buf,-1,fg);
	}
    }

    GDrawDrawLine(pixmap,kcd->xstart,kcd->ystart2,kcd->xstart+rect.width,kcd->ystart2,fg);
    GDrawDrawLine(pixmap,kcd->xstart2,kcd->ystart,kcd->xstart2,kcd->ystart+rect.height,fg);
    GDrawPopClip(pixmap,&old2);
    GDrawPopClip(pixmap,&old1);
    --rect.y; ++rect.height;		/* Makes accented letters show better */
    GDrawDrawRect(pixmap,&rect,fg);
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
      case et_sb_halfup: case et_sb_halfdown: break;
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
      case et_sb_halfup: case et_sb_halfdown: break;
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

static int kcd_sub_e_h(GWindow gw, GEvent *event) {
    KernClassDlg *kcd = GDrawGetUserData(gw);
    switch ( event->type ) {
      case et_expose:
	KCD_KernExpose(kcd,gw,event);
      break;
      case et_mouseup: case et_mousedown: case et_mousemove:
	KCD_KernMouse(kcd,event);
      break;
      case et_char:
return( false );
      case et_resize:
	kcd->subwidth = event->u.resize.size.width;
	GDrawRequestExpose(gw,NULL,false);
      break;
      default: break;
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
	    help("ui/mainviews/metricsview.html", kcd->iskernpair ?  "#metricsview-kernpair":
				    "#metricsview-kernclass");
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
	if ( !kcd->iskernpair )
	    KCD_Mouse(kcd,event);
      break;
      case et_expose:
	if ( !kcd->iskernpair )
	    KCD_Expose(kcd,gw,event);
      break;
      case et_resize:
	KC_DoResize(kcd);
      break;
      case et_controlevent:
	switch( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    if ( event->u.control.g == kcd->hsb )
		KCD_HScroll(kcd,&event->u.control.u.sb);
	    else
		KCD_VScroll(kcd,&event->u.control.u.sb);
	  break;
	  default: break;
	}
      break;
      default: break;
    }
return( true );
}

void ME_ListCheck(GGadget *g,int r, int c, SplineFont *sf) {
    /* Gadget g is a matrix edit and the column "c" contains a list of glyph */
    /*  lists. Glyphs may appear multiple times in the list, but glyph names */
    /*  should be in the font. */
    /* the entry at r,c has just changed. Check to validate the above */
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *classes = _GMatrixEditGet(g,&rows);
    char *start1, *pt1, *eow1;
    int ch1, off;
    int changed = false;

    /* Remove any leading spaces */
    for ( start1=classes[r*cols+c].u.md_str; *start1==' '; ++start1 );
    if ( start1!=classes[r*cols+c].u.md_str ) {
	off = start1-classes[r*cols+c].u.md_str;
	for ( pt1=start1; *pt1; ++pt1 )
	    pt1[-off] = *pt1;
	pt1[-off] = '\0';
	changed = true;
	pt1 -= off;
	start1 -= off;
    } else
	pt1 = start1+strlen(start1);
    while ( pt1>start1 && pt1[-1]==' ' ) --pt1;
    *pt1 = '\0';

    /* Check for duplicate names in this class */
    /*  also check for glyph names which aren't in the font */
    while ( *start1!='\0' ) {
	for ( pt1=start1; *pt1!=' ' && *pt1!='(' && *pt1!='{' && *pt1!='\0' ; ++pt1 );
	/* Preserve the {Everything Else} string from splitting */
	if ( *pt1=='{' ) {
	    while ( *pt1!='\0' && *pt1!='}' ) ++pt1;
	    if ( *pt1=='}' ) ++pt1;
	}
	eow1 = pt1;
	if ( *eow1=='(' ) {
	    while ( *eow1!='\0' && *eow1!=')' ) ++eow1;
	    if ( *eow1==')' ) ++eow1;
	}
	while ( *eow1==' ' ) ++eow1;
	ch1 = *pt1; *pt1='\0';
	if ( sf!=NULL && !isEverythingElse( start1 )) {
	    SplineChar *sc = SFGetChar(sf,-1,start1);
	    if ( sc==NULL )
		ff_post_notice(_("Missing glyph"),_("The font does not contain a glyph named %s."), start1 );
	}
	if ( *eow1=='\0' ) {
	    *pt1 = ch1;
    break;
	}
	*pt1 = ch1;
	start1 = eow1;
    }
    if ( changed ) {
	/* Remove trailing spaces too */
	start1=classes[r*cols+c].u.md_str;
	pt1 = start1+strlen(start1);
	while ( pt1>start1 && pt1[-1]==' ' )
	    --pt1;
	*pt1 = '\0';
	GGadgetRedraw(g);
    }
}

void ME_SetCheckUnique(GGadget *g,int r, int c, SplineFont *sf) {
    /* Gadget g is a matrix edit and the column "c" contains a list of glyph */
    /*  sets. No glyph may appear twice in a set, and glyph names */
    /*  should be in the font. */
    /* the entry at r,c has just changed. Check to validate the above */
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *classes = _GMatrixEditGet(g,&rows);
    char *start1, *start2, *pt1, *pt2, *eow1, *eow2;
    int ch1, ch2, off;
    int changed = false;

    /* Remove any leading spaces */
    for ( start1=classes[r*cols+c].u.md_str; *start1==' '; ++start1 );
    if ( start1!=classes[r*cols+c].u.md_str ) {
	off = start1-classes[r*cols+c].u.md_str;
	for ( pt1=start1; *pt1; ++pt1 )
	    pt1[-off] = *pt1;
	pt1[-off] = '\0';
	changed = true;
	pt1 -= off;
	start1 -= off;
    } else
	pt1 = start1+strlen(start1);
    while ( pt1>start1 && pt1[-1]==' ' ) --pt1;
    *pt1 = '\0';

    /* Check for duplicate names in this class */
    /*  also check for glyph names which aren't in the font */
    while ( *start1!='\0' ) {
	for ( pt1=start1; *pt1!=' ' && *pt1!='(' && *pt1!='{' && *pt1!='\0' ; ++pt1 );
	/* Preserve the {Everything Else} string from splitting */
	if ( *pt1=='{' ) {
	    while ( *pt1!='\0' && *pt1!='}' ) ++pt1;
	    if ( *pt1=='}' ) ++pt1;
	}
	eow1 = pt1;
	if ( *eow1=='(' ) {
	    while ( *eow1!='\0' && *eow1!=')' ) ++eow1;
	    if ( *eow1==')' ) ++eow1;
	}
	while ( *eow1==' ' ) ++eow1;
	ch1 = *pt1; *pt1='\0';
	if ( sf!=NULL && !isEverythingElse( start1 )) {
	    SplineChar *sc = SFGetChar(sf,-1,start1);
	    if ( sc==NULL )
		ff_post_notice(_("Missing glyph"),_("The font does not contain a glyph named %s."), start1 );
	}
	if ( *eow1=='\0' ) {
	    *pt1 = ch1;
    break;
	}
	for ( start2 = eow1; *start2!='\0'; ) {
	    for ( pt2=start2; *pt2!=' ' && *pt2!='(' && *pt2!='\0' ; ++pt2 );
	    eow2 = pt2;
	    if ( *eow2=='(' ) {
		while ( *eow2!='\0' && *eow2!=')' ) ++eow2;
		if ( *eow2==')' ) ++eow2;
	    }
	    while ( *eow2==' ' ) ++eow2;
	    ch2 = *pt2; *pt2='\0';
	    if ( strcmp(start1,start2)==0 ) {
		off = eow2-start2;
		if ( *eow2=='\0' && start2>classes[r*cols+c].u.md_str &&
			start2[-1]==' ' )
		    ++off;
		for ( pt2=eow2; *pt2; ++pt2 )
		    pt2[-off] = *pt2;
		pt2[-off] = '\0';
		changed = true;
	    } else {
		start2 = eow2;
		*pt2 = ch2;
	    }
	}
	*pt1 = ch1;
	start1 = eow1;
    }
    if ( changed ) {
	GGadgetRedraw(g);
	/* Remove trailing spaces too */
	start1=classes[r*cols+c].u.md_str;
	pt1 = start1+strlen(start1);
	while ( pt1>start1 && pt1[-1]==' ' )
	    --pt1;
	*pt1 = '\0';
    }
}

void ME_ClassCheckUnique(GGadget *g,int r, int c, SplineFont *sf) {
    /* Gadget g is a matrix edit and column "c" contains a list of glyph */
    /*  classes. No glyph may appear in more than one class. */
    /*  Also all checks in the above routine should be done. */
    /* the entry at r,c has just changed. Check to validate the above */
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *classes = _GMatrixEditGet(g,&rows);
    char *start1, *start2, *pt1, *pt2, *eow1, *eow2;
    int ch1, ch2, testr, off;
    int changed = false;
    char *buts[3];

    ME_SetCheckUnique(g,r,c,sf);

    buts[0] = _("_From this class"); buts[1] = _("From the _other class"); buts[2]=NULL;
    /* Now check for duplicates in other rows */
    for ( start1=classes[r*cols+c].u.md_str; *start1!='\0'; ) {
	for ( pt1=start1; *pt1!=' ' && *pt1!='(' && *pt1!='\0' ; ++pt1 );
	eow1 = pt1;
	if ( *eow1=='(' ) {
	    while ( *eow1!='\0' && *eow1!=')' ) ++eow1;
	    if ( *eow1==')' ) ++eow1;
	}
	while ( *eow1==' ' ) ++eow1;
	ch1 = *pt1; *pt1='\0';

	for ( testr=0; testr<rows; ++testr ) if ( testr!=r ) {
	    for ( start2 = classes[testr*cols+c].u.md_str; *start2!='\0'; ) {
		for ( pt2=start2; *pt2!=' ' && *pt2!='(' && *pt2!='\0' ; ++pt2 );
		eow2 = pt2;
		if ( *eow2=='(' ) {
		    while ( *eow2!='\0' && *eow2!=')' ) ++eow2;
		    if ( *eow2==')' ) ++eow2;
		}
		while ( *eow2==' ' ) ++eow2;
		ch2 = *pt2; *pt2='\0';
		if ( strcmp(start1,start2)==0 ) {
		    *pt2 = ch2;
		    if ( gwwv_ask(_("Glyph in two classes"),(const char **) buts,0,1,
			    _("The glyph named %s also occurs in the class on row %d which begins with %.20s...\nYou must remove it from one of them."),
			    start1, testr, classes[testr*cols+c].u.md_str )==0 ) {
			off = eow1-start1;
			for ( pt1=eow1; *pt1; ++pt1 )
			    pt1[-off] = *pt1;
			pt1[-off] = '\0';
			changed = true;
      goto end_of_outer_loop;
		    } else {
			off = eow2-start2;
			for ( pt2=eow2; *pt2; ++pt2 )
			    pt2[-off] = *pt2;
			pt2[-off] = '\0';
			changed = true;
		    }
		} else {
		    start2 = eow2;
		    *pt2 = ch2;
		}
	    }
	}
	*pt1 = ch1;
	start1 = eow1;
      end_of_outer_loop:;
    }
    if ( changed )
	GGadgetRedraw(g);
}

static void KCD_FinishEdit(GGadget *g,int r, int c, int wasnew) {
    // This function expands the cross-mapping structures and then calls KCD_AutoKernAClass in order to populate them.
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    // CID_ClassList is a macro denoting the identification number for the widget for the first character list.
    // If the CID differs, then we assume that we are using the second list.
    int is_first = GGadgetGetCid(g) == CID_ClassList;
    int i, autokern;

//    printf("KCD_FinishEdit()\n");
    ME_ClassCheckUnique(g, r, c, kcd->sf);

    if ( wasnew ) {
	autokern = GGadgetIsChecked(GWidgetGetControl(kcd->gw,CID_Autokern));
	if ( is_first ) {
            // offsets and adjusts are mappings between the characters in the first and second lists.
	    kcd->offsets = realloc(kcd->offsets,(kcd->first_cnt+1)*kcd->second_cnt*sizeof(int16_t));
	    memset(kcd->offsets+kcd->first_cnt*kcd->second_cnt,
		    0, kcd->second_cnt*sizeof(int16_t));
            // adjusts are resolution-specific.
	    kcd->adjusts = realloc(kcd->adjusts,(kcd->first_cnt+1)*kcd->second_cnt*sizeof(DeviceTable));
	    memset(kcd->adjusts+kcd->first_cnt*kcd->second_cnt,
		    0, kcd->second_cnt*sizeof(DeviceTable));
            // Group kerning.
	    if (kcd->firsts_names) {
	      kcd->firsts_names = realloc(kcd->firsts_names,(kcd->first_cnt+1)*sizeof(char*));
	      memset(kcd->firsts_names+kcd->first_cnt, 0, sizeof(char*));
	    }
	    if (kcd->firsts_flags) {
	      kcd->firsts_flags = realloc(kcd->firsts_flags,(kcd->first_cnt+1)*sizeof(int));
	      memset(kcd->firsts_flags+kcd->first_cnt, 0, sizeof(int));
	    }
	    if (kcd->offsets_flags) {
	      kcd->offsets_flags = realloc(kcd->offsets_flags,(kcd->first_cnt+1)*kcd->second_cnt*sizeof(int));
	      memset(kcd->offsets_flags+kcd->first_cnt*kcd->second_cnt,
		    0, kcd->second_cnt*sizeof(int));
	    }
	    ++kcd->first_cnt;
	    if ( autokern )
		KCD_AutoKernAClass(kcd,kcd->first_cnt-1,true);
	} else {
            // The procedure for expanding offsets varies here, adding a column, since it is necessary to leave a space on each row for the new column.
            {
	    int16_t *new = malloc(kcd->first_cnt*(kcd->second_cnt+1)*sizeof(int16_t));
	        for ( i=0; i<kcd->first_cnt; ++i ) {
		    memcpy(new+i*(kcd->second_cnt+1),kcd->offsets+i*kcd->second_cnt,
			    kcd->second_cnt*sizeof(int16_t));
		    new[i*(kcd->second_cnt+1)+kcd->second_cnt] = 0;
	        }
	        free( kcd->offsets );
	        kcd->offsets = new;
            }
            
	    {
		DeviceTable *new = malloc(kcd->first_cnt*(kcd->second_cnt+1)*sizeof(DeviceTable));
		for ( i=0; i<kcd->first_cnt; ++i ) {
		    memcpy(new+i*(kcd->second_cnt+1),kcd->adjusts+i*kcd->second_cnt,
			    kcd->second_cnt*sizeof(DeviceTable));
		    memset(&new[i*(kcd->second_cnt+1)+kcd->second_cnt],0,sizeof(DeviceTable));
		}
		free( kcd->adjusts );
		kcd->adjusts = new;
	    }

            // Group kerning.
	    if (kcd->seconds_names) {
	      kcd->seconds_names = realloc(kcd->seconds_names,(kcd->second_cnt+1)*sizeof(char*));
	      memset(kcd->seconds_names+kcd->second_cnt, 0, sizeof(char*));
	    }
	    if (kcd->seconds_flags) {
	      kcd->seconds_flags = realloc(kcd->seconds_flags,(kcd->second_cnt+1)*sizeof(int));
	      memset(kcd->seconds_flags+kcd->second_cnt, 0, sizeof(int));
	    }
            if (kcd->offsets_flags) {
	      int *new = malloc(kcd->first_cnt*(kcd->second_cnt+1)*sizeof(int));
	        for ( i=0; i<kcd->first_cnt; ++i ) {
		    memcpy(new+i*(kcd->second_cnt+1),kcd->offsets_flags+i*kcd->second_cnt,
			    kcd->second_cnt*sizeof(int));
		    new[i*(kcd->second_cnt+1)+kcd->second_cnt] = 0;
	        }
	        free( kcd->offsets_flags );
	        kcd->offsets_flags = new;
            }

	    ++kcd->second_cnt;
	    if ( autokern )
		KCD_AutoKernAClass(kcd,kcd->second_cnt-1,false);
	}
	KCD_SBReset(kcd);
	GDrawRequestExpose(kcd->gw,NULL,false);
    }
}

static int whichToWidgetID( int which )
{
    return which==0 ? CID_First : CID_Second;
}


static char *KCD_PickGlyphsForClass(GGadget *g,int r, int c) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *classes = _GMatrixEditGet(g,&rows);

    int which    = GWidgetGetControl(kcd->gw,CID_ClassList+100) == g;
    int widgetid = whichToWidgetID( which );
    char *new = GlyphSetFromSelection(kcd->sf,kcd->layer,classes[r*cols+c].u.md_str);
    if (new == NULL) new = copy("");
    if (new != NULL) {
      GGadgetSetTitle8(GWidgetGetControl(kcd->gw,widgetid),new );
      KCD_UpdateGlyphFromName(kcd,which,new);
    }
    char *other = GGadgetGetTitle8(GWidgetGetControl(kcd->gw,whichToWidgetID( !which )));
    if( other )
    {
	KCD_UpdateGlyphFromName(kcd,!which,other);
    }

    GDrawRequestExpose(kcd->subw,NULL,false);

return( new );
}

static enum gme_updown KCD_EnableUpDown(GGadget *g,int r) {
    int rows;
    enum gme_updown ret = 0;

    (void) GMatrixEditGet(g,&rows);
    if ( r>=2 )
	ret = ud_up_enabled;
    if ( r>=1 && r<rows-1 )
	ret |= ud_down_enabled;
return( ret );
}

static void KCD_RowMotion(GGadget *g,int oldr, int newr) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    int is_first = GGadgetGetCid(g) == CID_ClassList;
    int i;
    DeviceTable tempdt;

    if ( is_first ) {
	for ( i=0; i<kcd->second_cnt; ++i ) {
	    int16_t off = kcd->offsets[oldr*kcd->second_cnt + i];
	    kcd->offsets[oldr*kcd->second_cnt + i] = kcd->offsets[newr*kcd->second_cnt + i];
	    kcd->offsets[newr*kcd->second_cnt + i] = off;
	    tempdt = kcd->adjusts[oldr*kcd->second_cnt + i];
	    kcd->adjusts[oldr*kcd->second_cnt + i] = kcd->adjusts[newr*kcd->second_cnt + i];
	    kcd->adjusts[newr*kcd->second_cnt + i] = tempdt;
	    // Group kerning.
	    if (kcd->offsets_flags) {
	      kcd->offsets_flags[oldr*kcd->second_cnt + i] = kcd->offsets_flags[newr*kcd->second_cnt + i];
	      kcd->offsets_flags[newr*kcd->second_cnt + i] = off;
	    }
	}
	// Group kerning.
	if (kcd->firsts_names) {
	    char *name = kcd->firsts_names[oldr];
	    kcd->firsts_names[oldr] = kcd->firsts_names[newr];
	    kcd->firsts_names[newr] = name;
	}
	if (kcd->firsts_flags) {
	    int flags = kcd->firsts_flags[oldr];
	    kcd->firsts_flags[oldr] = kcd->firsts_flags[newr];
	    kcd->firsts_flags[newr] = flags;
	}
    } else {
	for ( i=0; i<kcd->first_cnt; ++i ) {
	    int16_t off = kcd->offsets[i*kcd->second_cnt + oldr];
	    kcd->offsets[i*kcd->second_cnt + oldr] = kcd->offsets[i*kcd->second_cnt + newr];
	    kcd->offsets[i*kcd->second_cnt + newr] = off;
	    tempdt = kcd->adjusts[i*kcd->second_cnt + oldr];
	    kcd->adjusts[i*kcd->second_cnt + oldr] = kcd->adjusts[i*kcd->second_cnt + newr];
	    kcd->adjusts[i*kcd->second_cnt + newr] = tempdt;
	    // Group kerning.
	    if (kcd->offsets_flags) {
	      kcd->offsets_flags[i*kcd->second_cnt + oldr] = kcd->offsets_flags[i*kcd->second_cnt + newr];
	      kcd->offsets_flags[i*kcd->second_cnt + newr] = off;
	    }
	}
	// Group kerning.
	if (kcd->seconds_names) {
	    char *name = kcd->seconds_names[oldr];
	    kcd->seconds_names[oldr] = kcd->seconds_names[newr];
	    kcd->seconds_names[newr] = name;
	}
	if (kcd->seconds_flags) {
	    int flags = kcd->seconds_flags[oldr];
	    kcd->seconds_flags[oldr] = kcd->seconds_flags[newr];
	    kcd->seconds_flags[newr] = flags;
	}
    }
    GDrawRequestExpose(kcd->gw,NULL,false);
}

static int KCD_EnableDeleteClass(GGadget *g,int whichclass) {
return( whichclass>0 );
}

static void KCD_DeleteClass(GGadget *g,int whichclass) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    int rows;
    int is_first = GGadgetGetCid(g) == CID_ClassList;
    int i,j;

    (void) GMatrixEditGet(g,&rows);
    if ( is_first ) {
	for ( i=0; i<kcd->second_cnt; ++i ) {
	    free(kcd->adjusts[whichclass*kcd->second_cnt+i].corrections);
	    kcd->adjusts[whichclass*kcd->second_cnt+i].corrections = NULL;
	}
	for ( i=whichclass+1; i<rows; ++i ) {
	    memmove(kcd->offsets+(i-1)*kcd->second_cnt,
		    kcd->offsets+i*kcd->second_cnt,
		    kcd->second_cnt*sizeof(int16_t));
	    memmove(kcd->adjusts+(i-1)*kcd->second_cnt,
		    kcd->adjusts+i*kcd->second_cnt,
		    kcd->second_cnt*sizeof(DeviceTable));
	    // Group kerning.
	    if (kcd->offsets_flags != NULL) {
	      memmove(kcd->offsets_flags+(i-1)*kcd->second_cnt,
		    kcd->offsets_flags+i*kcd->second_cnt,
		    kcd->second_cnt*sizeof(int));
	    }
	}
	// Group kerning.
	kcd->offsets = realloc(kcd->offsets, (kcd->first_cnt-1)*kcd->second_cnt*sizeof(int16_t));
	kcd->adjusts = realloc(kcd->adjusts, (kcd->first_cnt-1)*kcd->second_cnt*sizeof(DeviceTable));
	kcd->offsets_flags = realloc(kcd->offsets_flags, (kcd->first_cnt-1)*kcd->second_cnt*sizeof(int));
	if (kcd->firsts_names) {
	  memmove(kcd->firsts_names+whichclass, kcd->firsts_names+whichclass + 1, (kcd->first_cnt - whichclass - 1) * sizeof(char*));
	  kcd->firsts_names = realloc(kcd->firsts_names, (kcd->first_cnt - 1) * sizeof(char*));
	}
	if (kcd->firsts_flags) {
	  memmove(kcd->firsts_flags+whichclass, kcd->firsts_flags+whichclass + 1, (kcd->first_cnt - whichclass - 1) * sizeof(int));
	  kcd->firsts_flags = realloc(kcd->firsts_flags, (kcd->first_cnt - 1) * sizeof(int));
	}

	-- kcd->first_cnt;
    } else {
	int16_t *newoffs = malloc(kcd->first_cnt*(kcd->second_cnt-1)*sizeof(int16_t));
	DeviceTable *newadj = malloc(kcd->first_cnt*(kcd->second_cnt-1)*sizeof(DeviceTable));
	int *newoffflags = NULL;
	if (kcd->offsets_flags != NULL) newoffflags = malloc(kcd->first_cnt*(kcd->second_cnt-1)*sizeof(int));
	for ( i=0; i<kcd->first_cnt; ++i ) {
	    free(kcd->adjusts[i*kcd->second_cnt+whichclass].corrections);
	    kcd->adjusts[i*kcd->second_cnt+whichclass].corrections = NULL;
	}
	for ( i=0; i<rows; ++i ) if ( i!=whichclass ) {
	    int newi = i>whichclass ? i-1 : i;
	    for ( j=0; j<kcd->first_cnt; ++j ) {
		newoffs[j*(kcd->second_cnt-1)+newi] =
			kcd->offsets[j*kcd->second_cnt+i];
		newadj[j*(kcd->second_cnt-1)+newi] =
			kcd->adjusts[j*kcd->second_cnt+i];
		// Group kerning.
		if (newoffflags != NULL)
		  newoffflags[j*(kcd->second_cnt-1)+newi] =
			kcd->offsets_flags[j*kcd->second_cnt+i];
	    }
	}
	// Group kerning.
	if (kcd->seconds_names) {
	  memmove(kcd->seconds_names+whichclass, kcd->seconds_names+whichclass + 1, (kcd->second_cnt - whichclass - 1) * sizeof(char*));
	  kcd->seconds_names = realloc(kcd->seconds_names, (kcd->second_cnt - 1) * sizeof(char*));
	}
	if (kcd->seconds_flags) {
	  memmove(kcd->seconds_flags+whichclass, kcd->seconds_flags+whichclass + 1, (kcd->second_cnt - whichclass - 1) * sizeof(int));
	  kcd->seconds_flags = realloc(kcd->seconds_flags, (kcd->second_cnt - 1) * sizeof(int));
	}

	-- kcd->second_cnt;
	free(kcd->offsets);
	kcd->offsets = newoffs;
	free(kcd->adjusts);
	kcd->adjusts = newadj;
	// Group kerning.
	if (kcd->offsets_flags != NULL) free(kcd->offsets_flags);
	kcd->offsets_flags = newoffflags;
    }
}

static void KCD_ClassSelectionChanged(GGadget *g,int whichclass, int c) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(g));
    int is_first = GGadgetGetCid(g) == CID_ClassList;
    int first, second;

    if ( is_first )
	KCD_VShow(kcd,whichclass);
    else
	KCD_HShow(kcd,whichclass);
    first = GMatrixEditGetActiveRow(GWidgetGetControl(kcd->gw,CID_ClassList));
    second = GMatrixEditGetActiveRow(GWidgetGetControl(kcd->gw,CID_ClassList+100));
    if ( first!=-1 && second!=-1 )
	KCD_EditOffset(kcd, first, second);
}

static unichar_t **KCD_GlyphListCompletion(GGadget *t,int from_tab) {
    KernClassDlg *kcd = GDrawGetUserData(GDrawGetParentWindow(GGadgetGetWindow(t)));
    SplineFont *sf = kcd->sf;

return( SFGlyphNameCompletion(sf,t,from_tab,true));
}

static unichar_t **KCD_GlyphCompletion(GGadget *t,int from_tab) {
    KernClassDlg *kcd = GDrawGetUserData(GGadgetGetWindow(t));
    SplineFont *sf = kcd->sf;

return( SFGlyphNameCompletion(sf,t,from_tab,false));
}

static struct col_init class_ci[] = {
    { me_funcedit, KCD_PickGlyphsForClass, NULL, NULL, N_("Glyphs in the classes") },
    };
static int AddClassList(GGadgetCreateData *gcd, GTextInfo *label, int k, int off,
	struct matrixinit *mi, GGadgetCreateData **harray, GGadgetCreateData **varray,
	SplineFont *sf, char **classes, int cnt) {
    static char *empty[] = { NULL };
    static int initted = false;
    struct matrix_data *md;
    int i;

    if ( !initted ) {
	class_ci[0].title = S_(class_ci[0].title);
	initted = true;
    }

    label[k].text = (unichar_t *) (off==0?_("First Char"):_("Second Char"));
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_ClassLabel+off;
    gcd[k++].creator = GLabelCreate;
    varray[0] = &gcd[k-1];

    memset(mi,0,sizeof(*mi));
    mi->col_cnt = 1;
    mi->col_init = class_ci;

    if ( cnt==0 ) {
	cnt=1;
	classes = empty;
    }
    md = calloc(cnt+10,sizeof(struct matrix_data));
    for ( i=0; i<cnt; ++i ) {
	if ( i==0 && classes[i]==NULL ) {
	    md[i+0].u.md_str = copy( _("{Everything Else}") );
	    if ( off!=0 ) md[i+0].frozen = true;
	} else
	    md[i+0].u.md_str = SFNameList2NameUni(sf,classes[i]);
    }
    mi->matrix_data = md;
    mi->initial_row_cnt = cnt;
    mi->finishedit = KCD_FinishEdit;
    mi->candelete = KCD_EnableDeleteClass;

    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_ClassList+off;
    gcd[k].gd.u.matrix = mi;
    gcd[k++].creator = GMatrixEditCreate;
    varray[1] = &gcd[k-1];

/* GT: Select the class containing the glyph named in the following text field */
    label[k].text = (unichar_t *) _("Select Class Containing:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-3].gd.pos.x+5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+26+4;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.popup_msg = _("Select the class containing the named glyph");
    gcd[k++].creator = GLabelCreate;
    harray[0] = &gcd[k-1];

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+100; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 80;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.popup_msg = _("Select the class containing the named glyph");
    gcd[k].gd.handle_controlevent = KCD_TextSelect;
    gcd[k].gd.cid = CID_ClassSelect+off;
    gcd[k].gd.u.completion = KCD_GlyphCompletion;
    gcd[k++].creator = GTextCompletionCreate;
    harray[1] = &gcd[k-1]; harray[2] = NULL;

    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.u.boxelements = harray;
    gcd[k++].creator = GHBoxCreate;
    varray[2] = &gcd[k-1]; varray[3] = NULL;

    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.u.boxelements = varray;
    gcd[k++].creator = GVBoxCreate;

return( k );
}

static void FillShowKerningWindow(KernClassDlg *kcd, GGadgetCreateData *left,
	SplineFont *sf, GGadgetCreateData *topbox) {
    GGadgetCreateData gcd[31], hbox, flagbox, hvbox, buttonbox, mainbox[2];
    GGadgetCreateData *harray[10], *hvarray[20], *flagarray[4], *buttonarray[9], *varray[12];
    GGadgetCreateData *bigharray[6];
    GTextInfo label[31];
    int k,j;
    char buffer[20];
    int drawable_row;

    kcd->pixelsize = 150;
    kcd->magfactor = 1;

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    memset(&hbox,0,sizeof(hbox));
    memset(&flagbox,0,sizeof(flagbox));
    memset(&hvbox,0,sizeof(hvbox));
    memset(&buttonbox,0,sizeof(buttonbox));
    memset(&mainbox,0,sizeof(mainbox));
    if ( topbox!=NULL )
	memset(topbox,0,2*sizeof(*topbox));
    k = j = 0;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = 110;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.handle_controlevent = KCD_GlyphSelected;
    gcd[k].gd.cid = CID_First;
    gcd[k++].creator = left!=NULL ? GListButtonCreate : GTextFieldCreate;
    harray[0] = &gcd[k-1];

    gcd[k].gd.pos.x = 130; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = 110;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    if ( left==NULL ) gcd[k].gd.flags |= gg_list_alphabetic;
    gcd[k].gd.handle_controlevent = KCD_GlyphSelected;
    gcd[k].gd.cid = CID_Second;
    gcd[k++].creator = left!=NULL ? GListButtonCreate : GListFieldCreate;
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
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_SizeLabel;
    gcd[k++].creator = GLabelCreate;
    hvarray[0] = &gcd[k-1];

    sprintf( buffer, "%d", kcd->pixelsize );
    label[k].text = (unichar_t *) buffer;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.width = 80;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_DisplaySize;
    gcd[k].gd.handle_controlevent = KCD_DisplaySizeChanged;
    gcd[k++].creator = GListFieldCreate;
    hvarray[1] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Magnification:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_MagLabel;
    gcd[k++].creator = GLabelCreate;
    hvarray[2] = &gcd[k-1];

    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_Magnifications;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.u.list = magnifications;
    gcd[k].gd.handle_controlevent = KCD_MagnificationChanged;
    gcd[k++].creator = GListButtonCreate;
    hvarray[3] = &gcd[k-1]; hvarray[4] = GCD_Glue; hvarray[5] = NULL;

    label[k].text = (unichar_t *) _("Kern Offset:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_OffsetLabel;
    gcd[k++].creator = GLabelCreate;
    hvarray[6] = &gcd[k-1];

    gcd[k].gd.pos.x = 90; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_KernOffset;
    gcd[k].gd.handle_controlevent = KCD_KernOffChanged;
    gcd[k++].creator = GTextFieldCreate;
    hvarray[7] = &gcd[k-1];

    label[k].text = (unichar_t *) _("Device Table Correction:\n  (at display size)");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_CorrectLabel;
    gcd[k++].creator = GLabelCreate;
    hvarray[8] = &gcd[k-1];

    label[k].text = (unichar_t *) "0";
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_Correction;
    gcd[k].gd.handle_controlevent = KCD_CorrectionChanged;
    gcd[k++].creator = GTextFieldCreate;
    hvarray[9] = &gcd[k-1]; hvarray[10]=NULL;

    label[k].text = (unichar_t *) _("Revert Kerning");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible|gg_enabled;
    gcd[k].gd.popup_msg = _("Resets the kerning offset and device table corrections to what they were originally");
    gcd[k].gd.handle_controlevent = KCD_RevertKerning;
    gcd[k].gd.cid = CID_Revert;
    gcd[k++].creator = GButtonCreate;
    hvarray[11] = &gcd[k-1]; hvarray[12] = GCD_ColSpan;

    label[k].text = (unichar_t *) _("Clear Device Table");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible|gg_enabled;
    gcd[k].gd.popup_msg = _("Clear all device table corrections associated with this combination");
    gcd[k].gd.cid = CID_ClearDevice;
    gcd[k].gd.handle_controlevent = KCD_ClearDevice;
    gcd[k++].creator = GButtonCreate;
    hvarray[13] = &gcd[k-1]; hvarray[14] = GCD_ColSpan; hvarray[15] = NULL;
    hvarray[16] = NULL;

    hvbox.gd.flags = gg_enabled|gg_visible;
    hvbox.gd.u.boxelements = hvarray;
    hvbox.creator = GHVBoxCreate;
    varray[j++] = &hvbox; varray[j++] = NULL;

    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.pos.width = gcd[k].gd.pos.height = 100;
    gcd[k].gd.cid = CID_Display;
    gcd[k].gd.u.drawable_e_h = kcd_sub_e_h;
    gcd[k].data = kcd;
    gcd[k++].creator = GDrawableCreate;
    drawable_row = j/2;
    varray[j++] = &gcd[k-1]; varray[j++] = NULL;

    if ( left==NULL ) {
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = -40;
	gcd[k].gd.flags = gg_enabled ;
	label[k].text = (unichar_t *) _("Lookup subtable:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k++].creator = GLabelCreate;
	flagarray[0] = &gcd[k-1];

	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Subtable;
	gcd[k].gd.handle_controlevent = KP_Subtable;
	gcd[k++].creator = GListButtonCreate;
	flagarray[1] = &gcd[k-1]; flagarray[2] = GCD_Glue; flagarray[3] = NULL;

	flagbox.gd.flags = gg_enabled|gg_visible;
	flagbox.gd.u.boxelements = flagarray;
	flagbox.creator = GHBoxCreate;
	varray[j++] = &flagbox; varray[j++] = NULL;

	label[k].text = (unichar_t *) _("_OK");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 30; gcd[k].gd.pos.y = KC_Height-KC_CANCELDROP;
	gcd[k].gd.pos.width = -1;
	gcd[k].gd.flags = gg_visible|gg_enabled;
	if ( left==NULL ) gcd[k].gd.flags |= gg_but_default;
	gcd[k].gd.handle_controlevent = KPD_OK;
	gcd[k].gd.cid = CID_Prev2;
	gcd[k++].creator = GButtonCreate;

	label[k].text = (unichar_t *) _("_Cancel");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = -30+3; gcd[k].gd.pos.y = KC_Height-KC_CANCELDROP;
	gcd[k].gd.pos.width = -1;
	gcd[k].gd.flags = gg_visible|gg_enabled ;
	if ( left==NULL ) gcd[k].gd.flags |= gg_but_cancel;
	gcd[k].gd.handle_controlevent = KPD_Cancel;
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

	GGadgetsCreate(kcd->gw,mainbox);
	GHVBoxSetExpandableCol(buttonbox.ret,gb_expandgluesame);
    } else {
	varray[j++] = NULL;
	mainbox[0].gd.flags = gg_enabled|gg_visible;
	mainbox[0].gd.u.boxelements = varray;
	mainbox[0].creator = GHVBoxCreate;

	bigharray[0] = left;
	bigharray[1] = mainbox;
	bigharray[2] = bigharray[3] = NULL;

	topbox[0].gd.pos.x = mainbox[0].gd.pos.y = 2;
	topbox[0].gd.flags = gg_enabled|gg_visible;
	topbox[0].gd.u.boxelements = bigharray;
	topbox[0].gd.cid = CID_TopBox;
	topbox[0].creator = GHVGroupCreate;

	GGadgetsCreate(kcd->gw,topbox);
	GHVBoxSetExpandableCol(topbox[0].ret,0);
    }

    GHVBoxSetExpandableRow(mainbox[0].ret,drawable_row);
    GHVBoxSetExpandableCol(hbox.ret,gb_expandglue);
    /*GHVBoxSetExpandableCol(hvbox.ret,gb_expandglue);*/
    if ( left==NULL ) {
	GHVBoxSetExpandableCol(flagbox.ret,gb_expandglue);
	GGadgetSetList(flagarray[1]->ret,SFSubtablesOfType(sf,gpos_pair,false,false),false);
    }
    kcd->subw = GDrawableGetWindow(GWidgetGetControl(kcd->gw,CID_Display));
}

void KernClassD(KernClass *kc, SplineFont *sf, int layer, int isv) {
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[54], sepbox, classbox, hvbox, buttonbox, mainbox[2], topbox[2], titbox, hbox;
    GGadgetCreateData *harray1[17], *harray2[17], *varray1[5], *varray2[5];
    GGadgetCreateData *hvarray[13], *buttonarray[8], *varray[19], *h4array[8], *harrayclasses[6], *titlist[4], *h5array[3];
    GTextInfo label[54];
    KernClassDlg *kcd;
    int i, j, kc_width, vi;
    int as, ds, ld, sbsize;
    static unichar_t kernw[] = { '-', '1', '2', '3', '4', '5', 0 };
    GWindow gw;
    char titlebuf[300];
    char sepbuf[40], mkbuf[40];
    struct matrixinit firstmi, secondmi;

    for ( kcd = sf->kcd; kcd!=NULL && kcd->orig!=kc; kcd = kcd->next );
    if ( kcd!=NULL ) {
	GDrawSetVisible(kcd->gw,true);
	GDrawRaise(kcd->gw);
return;
    }
    kcd = calloc(1,sizeof(KernClassDlg));
    kcd->orig = kc;
    kcd->subtable = kc->subtable;
    kcd->sf = sf;
    kcd->layer = layer;
    kcd->isv = isv;
    kcd->old_pos = -1;
    kcd->next = sf->kcd;
    sf->kcd = kcd;

    kcd->first_cnt = kc->first_cnt;
    kcd->second_cnt = kc->second_cnt;
    kcd->offsets = malloc(kc->first_cnt*kc->second_cnt*sizeof(int16_t));
    memcpy(kcd->offsets,kc->offsets,kc->first_cnt*kc->second_cnt*sizeof(int16_t));
    kcd->adjusts = malloc(kc->first_cnt*kc->second_cnt*sizeof(DeviceTable));
    memcpy(kcd->adjusts,kc->adjusts,kc->first_cnt*kc->second_cnt*sizeof(DeviceTable));
    for ( i=0; i<kcd->first_cnt*kcd->second_cnt; ++i ) {
	if ( kcd->adjusts[i].corrections!=NULL ) {
	    int len = kcd->adjusts[i].last_pixel_size-kcd->adjusts[i].first_pixel_size+1;
	    kcd->adjusts[i].corrections = malloc(len);
	    memcpy(kcd->adjusts[i].corrections,kc->adjusts[i].corrections,len);
	}
    }

    // Group kerning.
    if (kc->firsts_names) {
      kcd->firsts_names = malloc(kc->first_cnt*sizeof(char*));
      int namepos;
      for (namepos = 0; namepos < kc->first_cnt; namepos ++)
        kcd->firsts_names[namepos] = copy(kc->firsts_names[namepos]);
    }
    if (kc->seconds_names) {
      kcd->seconds_names = malloc(kc->second_cnt*sizeof(char*));
      int namepos;
      for (namepos = 0; namepos < kc->second_cnt; namepos ++)
        kcd->seconds_names[namepos] = copy(kc->seconds_names[namepos]);
    }
    if (kc->firsts_flags) {
      kcd->firsts_flags = malloc(kc->first_cnt*sizeof(int));
      memcpy(kcd->firsts_flags,kc->firsts_flags,kc->first_cnt*sizeof(int));
    }
    if (kc->seconds_flags) {
      kcd->seconds_flags = malloc(kc->second_cnt*sizeof(int));
      memcpy(kcd->seconds_flags,kc->seconds_flags,kc->second_cnt*sizeof(int));
    }
    if (kcd->offsets_flags) {
      kcd->offsets_flags = malloc(kc->first_cnt*kc->second_cnt*sizeof(int));
      memcpy(kcd->offsets_flags,kc->offsets_flags,kc->first_cnt*kc->second_cnt*sizeof(int));
    }

    memset(&wattrs,0,sizeof(wattrs));
    memset(&gcd,0,sizeof(gcd));
    memset(&classbox,0,sizeof(classbox));
    memset(&hbox,0,sizeof(hbox));
    memset(&hvbox,0,sizeof(hvbox));
    memset(&buttonbox,0,sizeof(buttonbox));
    memset(&mainbox,0,sizeof(mainbox));
    memset(&titbox,0,sizeof(titbox));
    memset(&label,0,sizeof(label));

    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = true;
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

    kcd->font = kernclass_font.fi;
    GDrawWindowFontMetrics(gw,kcd->font,&as,&ds,&ld);
    kcd->fh = as+ds; kcd->as = as;
    GDrawSetFont(gw,kcd->font);

    kcd->kernh = kcd->fh+3;
    kcd->kernw = GDrawGetTextWidth(gw,kernw,-1)+3;

    if ( kc->subtable->separation==0 && !kc->subtable->kerning_by_touch ) {
	kc->subtable->separation = sf->width_separation;
	if ( sf->width_separation==0 )
	    kc->subtable->separation = 15*(sf->ascent+sf->descent)/100;
	kc->subtable->minkern = 0;
    }

    i = j = 0;
    snprintf( titlebuf, sizeof(titlebuf), _("Lookup Subtable: %s"), kc->subtable->subtable_name );
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 5;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    label[i].text = (unichar_t *) titlebuf;
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i++].creator = GLabelCreate;
    titlist[0] = &gcd[i-1]; titlist[1] = GCD_Glue;


    gcd[i].gd.flags = show_kerning_pane_in_class ? (gg_visible | gg_enabled | gg_cb_on) : (gg_visible | gg_enabled);
    label[i].text = (unichar_t *) _("Show Kerning");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = KC_ShowHideKernPane;
    gcd[i].gd.cid = CID_ShowHideKern;
    gcd[i++].creator = GCheckBoxCreate;
    titlist[2] = &gcd[i-1]; titlist[3] = NULL;

    memset(&titbox,0,sizeof(titbox));
    titbox.gd.flags = gg_enabled|gg_visible;
    titbox.gd.u.boxelements = titlist;
    titbox.creator = GHBoxCreate;

    varray[j++] = &titbox; varray[j++] = NULL;

	label[i].text = (unichar_t *) _("_Default Separation:");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 5+4;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.popup_msg = _(
	    "Add entries to the lookup trying to make the optical\n"
	    "separation between all pairs of glyphs equal to this\n"
	    "value." );
	gcd[i].creator = GLabelCreate;
	h4array[0] = &gcd[i++];

	sprintf( sepbuf, "%d", kc->subtable->separation );
	label[i].text = (unichar_t *) sepbuf;
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.width = 50;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.popup_msg = gcd[i-1].gd.popup_msg;
	gcd[i].gd.cid = CID_Separation;
	gcd[i].creator = GTextFieldCreate;
	h4array[1] = &gcd[i++]; h4array[2] = GCD_Glue;

	label[i].text = (unichar_t *) _("_Min Kern:");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 5+4;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.popup_msg = _(
	    "Any computed kerning change whose absolute value is less\n"
	    "that this will be ignored.\n" );
	gcd[i].creator = GLabelCreate;
	h4array[3] = &gcd[i++];

	sprintf( mkbuf, "%d", kc->subtable->minkern );
	label[i].text = (unichar_t *) mkbuf;
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.width = 50;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.popup_msg = gcd[i-1].gd.popup_msg;
	gcd[i].gd.cid = CID_MinKern;
	gcd[i].creator = GTextFieldCreate;
	h4array[4] = &gcd[i++];

	label[i].text = (unichar_t *) _("_Touching");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 5+4;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	if ( kc->subtable->kerning_by_touch )
	    gcd[i].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[i].gd.popup_msg = _(
	    "Normally kerning is based on achieving a constant (optical)\n"
	    "separation between glyphs, but occasionally it is desirable\n"
	    "to have a kerning table where the kerning is based on the\n"
	    "closest approach between two glyphs (So if the desired separ-\n"
	    "ation is 0 then the glyphs will actually be touching.");
	gcd[i].gd.cid = CID_Touched;
	gcd[i].creator = GCheckBoxCreate;
	h4array[5] = &gcd[i++];

	h4array[6] = GCD_Glue; h4array[7] = NULL;

	memset(&sepbox,0,sizeof(sepbox));
	sepbox.gd.flags = gg_enabled|gg_visible;
	sepbox.gd.u.boxelements = h4array;
	sepbox.creator = GHBoxCreate;
	varray[j++] = &sepbox; varray[j++] = NULL;

	label[i].text = (unichar_t *) _("Only kern glyphs closer");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.flags = gg_enabled|gg_visible;
	if ( kc->subtable->onlyCloser )
	    gcd[i].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[i].gd.popup_msg = _(
	    "When doing autokerning, only move glyphs closer together,\n"
	    "so the kerning offset will be negative.");
	gcd[i].gd.cid = CID_OnlyCloser;
	gcd[i].creator = GCheckBoxCreate;
	h5array[0] = &gcd[i++];

	label[i].text = (unichar_t *) _("Autokern new entries");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.flags = gg_enabled|gg_visible;
	if ( !kc->subtable->dontautokern )
	    gcd[i].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[i].gd.popup_msg = _(
	    "When adding a new class provide default kerning values\n"
	    "Between it and every class with which it interacts.");
	gcd[i].gd.cid = CID_Autokern;
	gcd[i].creator = GCheckBoxCreate;
	h5array[1] = &gcd[i++]; h5array[2] = NULL;

	memset(&hbox,0,sizeof(hbox));
	hbox.gd.flags = gg_enabled|gg_visible;
	hbox.gd.u.boxelements = h5array;
	hbox.creator = GHBoxCreate;

	varray[j++] = &hbox; varray[j++] = NULL;

    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = GDrawPointsToPixels(gw,gcd[i-1].gd.pos.y+17);
    gcd[i].gd.pos.width = pos.width-20;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[i++].creator = GLineCreate;
    varray[j++] = &gcd[i-1]; varray[j++] = NULL;

    i = AddClassList(gcd,label,i,0,&firstmi,harray1,varray1,
	    sf,kc->firsts,kc->first_cnt);
    harrayclasses[0] = &gcd[i-1];
    i = AddClassList(gcd,label,i,100,&secondmi,harray2,varray2,
	    sf,kc->seconds,kc->second_cnt);
    harrayclasses[2] = &gcd[i-1]; harrayclasses[3] = NULL;

    gcd[i].gd.pos.height = 20;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_line_vert;
    gcd[i++].creator = GLineCreate;
    harrayclasses[1] = &gcd[i-1];

    classbox.gd.flags = gg_enabled|gg_visible;
    classbox.gd.u.boxelements = harrayclasses;
    classbox.creator = GHBoxCreate;
    varray[j++] = &classbox; varray[j++] = NULL;

    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = GDrawPointsToPixels(gw,gcd[i-1].gd.pos.y+27);
    gcd[i].gd.pos.width = pos.width-20;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[i++].creator = GLineCreate;
    varray[j++] = &gcd[i-1]; varray[j++] = NULL;

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
    varray[j++] = &hvbox; varray[j++] = NULL;

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
    varray[j++] = &buttonbox; varray[j++] = NULL; varray[j++] = NULL;

    mainbox[0].gd.pos.x = mainbox[0].gd.pos.y = 2;
    mainbox[0].gd.flags = gg_enabled|gg_visible;
    mainbox[0].gd.u.boxelements = varray;
    mainbox[0].creator = GHVGroupCreate;

    FillShowKerningWindow(kcd, mainbox, kcd->sf, topbox);
    kcd->vsb = gcd[vi].ret;
    kcd->hsb = gcd[vi+1].ret;

    GHVBoxSetExpandableRow(mainbox[0].ret,6);

    GHVBoxSetExpandableCol(titbox.ret,gb_expandglue);

    GHVBoxSetExpandableCol(buttonbox.ret,gb_expandgluesame);
    GHVBoxSetExpandableCol(sepbox.ret,gb_expandglue);

    GHVBoxSetPadding(hvbox.ret,0,0);
    GHVBoxSetExpandableRow(hvbox.ret,1);
    GHVBoxSetExpandableCol(hvbox.ret,1);

    for ( i=0; i<2; ++i ) {
	GGadgetCreateData *box = harrayclasses[2*i], **boxarray = box->gd.u.boxelements;
	GHVBoxSetExpandableRow(box->ret,1);
	GHVBoxSetExpandableCol(boxarray[2]->ret,1);
    }

    for ( i=0; i<2; ++i ) {
	GGadget *list = GWidgetGetControl(kcd->gw,CID_ClassList+i*100);
	GRect size;
	memset(&size,0,sizeof(size));
	size.width = (kc_width-20)/2; size.height = 6;
	GGadgetSetDesiredSize(list,NULL,&size);
	GMatrixEditSetUpDownVisible(list, true);
	GMatrixEditSetCanUpDown(list, KCD_EnableUpDown);
	GMatrixEditSetRowMotionCallback(list, KCD_RowMotion);
	GMatrixEditSetBeforeDelete(list, KCD_DeleteClass);
    /* When the selection changes */
	GMatrixEditSetOtherButtonEnable(list, KCD_ClassSelectionChanged);
	GMatrixEditSetColumnCompletion(list,0,KCD_GlyphListCompletion);
    }

    KC_ShowHideKernPane(GWidgetGetControl(gw,CID_ShowHideKern),NULL);
    GHVBoxFitWindow(topbox[0].ret);
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
    int32_t len; int i,j;
    GTextInfo **old, **new;
    GGadget *list;
    KernClassListDlg *kcld;
    KernClassDlg *kcd;
    KernClass *p, *kc, *n;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	kcld = GDrawGetUserData(GGadgetGetWindow(g));
	list = GWidgetGetControl(kcld->gw,CID_List);
	old = GGadgetGetList(list,&len);
	new = calloc(len+1,sizeof(GTextInfo *));
	p = NULL; kc = kcld->isv ? kcld->sf->vkerns : kcld->sf->kerns;
	for ( i=j=0; i<len; ++i, kc = n ) {
	    n = kc->next;
	    if ( !old[i]->selected ) {
		new[j] = malloc(sizeof(GTextInfo));
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
	new[j] = calloc(1,sizeof(GTextInfo));
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
	//
	// Update any metrics views for the splinefont.
	//
	if( kcld && kcld->sf )
	{
	    SplineFont *sf = kcld->sf;

	    MVReFeatureAll( sf );
	    MVReKernAll( sf );

	    /* KernClass* kc = sf->kerns; */
	    /* int i = 0; */
	    /* for( ; kc; kc = kc->next ) */
	    /* 	i++; */
	    /* printf("kern count:%d\n", i ); */

	    MetricsView *mv;
	    for ( mv=sf->metrics; mv!=NULL; mv=mv->next )
		MVSelectFirstKerningTable( mv );

	}
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
	    help("ui/mainviews/metricsview.html", "#metricsview-kernclass");
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

    kcld = calloc(1,sizeof(KernClassListDlg));
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
    // temp = 40 + 300*GIntGetResource(_NUM_Buttonsize)/GGadgetScale(100); // The _NUM_Buttonsize value is obsolete.
    temp = 40 + 300*114/GGadgetScale(100);
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


    FillShowKerningWindow(&kcd, NULL, kcd.sf, NULL);

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
    int i,f,l,pcnt = 2;
    KernClass *kc;

    /* At the first pass we check only combinations between defined classes. */
    /* while at the second pass class 0 is also accepted. If zero kerning values are */
    /* allowed, then we may need two more passes (again, first checking only defined */
    /* classes, and then also class 0, but this time accepting also zero offsets) */
    if (allow_zero) pcnt *= 2;
    for ( i=0; i<=pcnt; ++i ) {
	for ( kc=sf->kerns; kc!=NULL; kc=kc->next ) {
	    uint8_t kspecd = kc->firsts[0] != NULL;
	    f = KCFindName(first->name,kc->firsts ,kc->first_cnt ,i % 2);
	    l = KCFindName(last->name ,kc->seconds,kc->second_cnt,i % 2);
	    if ( f!=-1 && l!=-1 && ( kspecd || f!=0 || l!=0 )  ) {
		if ( i > 1 || kc->offsets[f*kc->second_cnt+l]!=0 ) {
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
    int i,f,l,pcnt = 2;
    KernClass *kc;

    if (allow_zero) pcnt *= 2;
    for ( i=0; i<=pcnt; ++i ) {
	for ( kc=sf->vkerns; kc!=NULL; kc=kc->next ) {
	    uint8_t kspecd = kc->firsts[0] != NULL;
	    f = KCFindName(first->name,kc->firsts ,kc->first_cnt ,i % 2);
	    l = KCFindName(last->name ,kc->seconds,kc->second_cnt,i % 2);
	    if ( f!=-1 && l!=-1 && ( kspecd || f!=0 || l!=0 ) ) {
		if ( i > 1 || kc->offsets[f*kc->second_cnt+l]!=0 ) {
		    *index = f*kc->second_cnt+l;
return( kc );
		}
	    }
	}
    }
return( NULL );
}
