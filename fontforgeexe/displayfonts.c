/* Copyright (C) 2000-2012 by George Williams */
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

#include "autotrace.h"
#include "cvundoes.h"
#include "ffglib.h"
#include "fontforgeui.h"
#include "gfile.h"
#include "gkeysym.h"
#include "lookups.h"
#include "print.h"
#include "sftextfieldP.h"
#include "splinesaveafm.h"
#include "splineutil2.h"
#include "tottfgpos.h"
#include "ustring.h"
#include "utype.h"

#include <math.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#if !defined(__MINGW32__)
#include <sys/wait.h>
#endif

typedef struct printffdlg {
    struct printinfo pi;
    GWindow gw;
    GWindow setup;
    GTimer *sizechanged;
    GTimer *dpichanged;
    GTimer *widthchanged;
    GTimer *resized;
    GTextInfo *scriptlangs;
    FontView *fv;
    CharView *cv;
    SplineSet *fit_to_path;
    uint8_t script_unknown;
    uint8_t ready;
    int *done;
} PD;

static int lastdpi=0;
static unichar_t *old_bind_text = NULL;

/* ************************************************************************** */
/* *********************** Code for Page Setup dialog *********************** */
/* ************************************************************************** */

#define CID_lp		1001
#define CID_lpr		1002
#define	CID_ghostview	1003
#define CID_File	1004
#define CID_Other	1005
#define CID_OtherCmd	1006
#define	CID_Pagesize	1007
#define CID_CopiesLab	1008
#define CID_Copies	1009
#define CID_PrinterLab	1010
#define CID_Printer	1011
#define CID_PDFFile	1012


/* ************************************************************************** */
/* ************************* Code for Print dialog ************************** */
/* ************************************************************************** */

    /* CIDs for Print */
#define CID_TabSet	1000
#define CID_Display	1001
#define CID_Chars	1002
#define	CID_MultiSize	1003
#define CID_PSLab	1005
#define	CID_PointSize	1006
#define CID_OK		1009
#define CID_Cancel	1010
#define CID_Setup	1010

    /* CIDs for display */
#define CID_Font	2001
#define CID_AA		2002
#define CID_SizeLab	2003
#define CID_Size	2004
#define CID_pfb		2005
#define CID_ttf		2006
#define CID_otf		2007
#define CID_nohints	2008
#define CID_bitmap	2009
#define CID_pfaedit	2010
#define CID_SampleText	2011
#define CID_ScriptLang	2022
#define CID_Features	2023
#define CID_DPI		2024
#define CID_TopBox	2025

    /* CIDs for Insert Text */
#define CID_Bind	3001
#define CID_Scale	3002
#define CID_Start	3003
#define CID_Center	3004
#define CID_End		3005
#define CID_TextWidth	3006
#define CID_YOffset	3007
#define CID_GlyphAsUnit	3008
#define CID_ActualWidth	3009

static int PRT_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	PD *pi = GDrawGetUserData(GGadgetGetWindow(g));
	int err = false;

	    SplineSet *ss, *end;
	    int bound = GGadgetIsChecked(GWidgetGetControl(pi->gw,CID_Bind));
	    int scale = GGadgetIsChecked(GWidgetGetControl(pi->gw,CID_Scale));
	    int gunit = GGadgetIsChecked(GWidgetGetControl(pi->gw,CID_GlyphAsUnit));
	    int align = GGadgetIsChecked(GWidgetGetControl(pi->gw,CID_Start ))? 0 :
			GGadgetIsChecked(GWidgetGetControl(pi->gw,CID_Center))? 1 : 2;
	    int width = GetInt8(pi->gw,CID_TextWidth,_("Width"),&err);
	    real offset = GetReal8(pi->gw,CID_YOffset,_("Offset"),&err);
	    CharView *cv = pi->cv;
	    LayoutInfo *sample;
	    /* int dpi  =*/ GetInt8(pi->gw,CID_DPI,_("DPI"),&err);
	    /* int size =*/ GetInt8(pi->gw,CID_Size,_("Size"),&err);
	    if ( err )
return(true);
	    free(old_bind_text);
	    old_bind_text = GGadgetGetTitle(GWidgetGetControl(pi->gw,CID_SampleText));
	    sample = LIConvertToPrint(
			&((SFTextArea *) GWidgetGetControl(pi->gw,CID_SampleText))->li,
			width, 50000, 72 );
	    ss = LIConvertToSplines(sample, 72,cv->b.layerheads[cv->b.drawmode]->order2);
	    LayoutInfo_Destroy(sample);
	    free(sample);
	    if ( bound && pi->fit_to_path )
		SplineSetBindToPath(ss,scale,gunit,align,offset,pi->fit_to_path);
	    if ( ss ) {
		SplineSet *test;
		CVPreserveState((CharViewBase *) cv);
		end = NULL;
		for ( test=ss; test!=NULL; test=test->next ) {
		    SplinePoint *sp;
		    end = test;
		    for ( sp=test->first; ; ) {
			sp->selected = true;
			if ( sp->next==NULL )
		    break;
			sp = sp->next->to;
			if ( sp==test->first )
		    break;
		    }
		}
		end->next = cv->b.layerheads[cv->b.drawmode]->splines;
		cv->b.layerheads[cv->b.drawmode]->splines = ss;
		CVCharChangedUpdate((CharViewBase *) cv);
	    }

	if ( pi->done!=NULL )
	    *(pi->done) = true;
	GDrawDestroyWindow(pi->gw);
    }
return( true );
}

/* ************************************************************************** */
/* ************************ Code for Display dialog ************************* */
/* ************************************************************************** */

static void TextInfoDataFree(GTextInfo *ti) {
    int i;

    if ( ti==NULL )
return;
    for ( i=0; ti[i].text!=NULL || ti[i].line ; ++i )
	free(ti[i].userdata);
    GTextInfoListFree(ti);
}

static GTextInfo *FontNames(SplineFont *cur_sf) {
    int cnt;
    FontView *fv;
    SplineFont *sf;
    GTextInfo *ti;
    int selected = false, any_other=-1;

    for ( fv=fv_list, cnt=0; fv!=NULL; fv=(FontView *) (fv->b.next) )
	if ( (FontView *) (fv->b.nextsame)==NULL )
	    ++cnt;
    ti = calloc(cnt+1,sizeof(GTextInfo));
    for ( fv=fv_list, cnt=0; fv!=NULL; fv=(FontView *) (fv->b.next) ) {
	if ( (FontView *) (fv->b.nextsame)==NULL ) {
	    sf = fv->b.sf;
	    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
	    ti[cnt].text = uc_copy(sf->fontname);
	    ti[cnt].userdata = sf;
	    /* In the insert_text dlg, use anything other than the current font */
	    if ( cur_sf!=sf && !selected ) {
		if ( cur_sf->isnew )
		    any_other = cnt;
		else {
		    ti[cnt].selected = true;
		    selected = true;
		}
	    }
	    ++cnt;
	}
    }
    if ( !selected && any_other!=-1 )
	ti[any_other].selected = true;
    else if ( !selected )
	ti[0].selected = true;
return( ti );
}

static BDFFont *DSP_BestMatch(SplineFont *sf,int aa,int size) {
    BDFFont *bdf, *sizem=NULL;
    int a;

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	if ( bdf->clut==NULL && !aa )
	    a = 4;
	else if ( bdf->clut!=NULL && aa ) {
	    if ( bdf->clut->clut_len==256 )
		a = 4;
	    else if ( bdf->clut->clut_len==16 )
		a = 3;
	    else
		a = 2;
	} else
	    a = 1;
	if ( bdf->pixelsize==size && a==4 )
return( bdf );
	if ( sizem==NULL )
	    sizem = bdf;
	else {
	    int sdnew = bdf->pixelsize-size, sdold = sizem->pixelsize-size;
	    if ( sdnew<0 ) sdnew = -sdnew;
	    if ( sdold<0 ) sdold = -sdold;
	    if ( sdnew<sdold )
		sizem = bdf;
	    else if ( sdnew==sdold ) {
		int olda;
		if ( sizem->clut==NULL && !aa )
		    olda = 4;
		else if ( sizem->clut!=NULL && aa ) {
		    if ( sizem->clut->clut_len==256 )
			olda = 4;
		    else if ( sizem->clut->clut_len==16 )
			olda = 3;
		    else
			olda = 2;
		} else
		    olda = 1;
		if ( a>olda )
		    sizem = bdf;
	    }
	}
    }
return( sizem );	
}

static BDFFont *DSP_BestMatchDlg(PD *di) {
    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
    SplineFont *sf;
    int val;
    unichar_t *end;

    if ( sel==NULL )
return( NULL );
    sf = sel->userdata;
    val = u_strtol(_GGadgetGetTitle(GWidgetGetControl(di->gw,CID_Size)),&end,10);
    if ( *end!='\0' || val<4 )
return( NULL );

return( DSP_BestMatch(sf,GGadgetIsChecked(GWidgetGetControl(di->gw,CID_AA)),val) );
}

static enum sftf_fonttype DSP_FontType(PD *di) {
    int type;
    type = GGadgetIsChecked(GWidgetGetControl(di->gw,CID_pfb))? sftf_pfb :
	   GGadgetIsChecked(GWidgetGetControl(di->gw,CID_ttf))? sftf_ttf :
	   GGadgetIsChecked(GWidgetGetControl(di->gw,CID_otf))? sftf_otf :
	   GGadgetIsChecked(GWidgetGetControl(di->gw,CID_nohints))? sftf_nohints :
	   GGadgetIsChecked(GWidgetGetControl(di->gw,CID_pfaedit))? sftf_pfaedit :
	   sftf_bitmap;
return( type );
}

static void DSP_SetFont(PD *di,int doall) {
    unichar_t *end;
    int size = u_strtol(_GGadgetGetTitle(GWidgetGetControl(di->gw,CID_Size)),&end,10);
    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
    SplineFont *sf;
    int aa = GGadgetIsChecked(GWidgetGetControl(di->gw,CID_AA));
    int type;
    SFTextArea *g;
    int layer;

    if ( sel==NULL || *end )
return;
    sf = sel->userdata;

    type = DSP_FontType(di);

    g = (SFTextArea *) GWidgetGetControl(di->gw,CID_SampleText);
    layer = di->fv!=NULL && di->fv->b.sf==sf ? di->fv->b.active_layer : ly_fore;
    if ( !LI_SetFontData( &g->li, doall?0:-1,-1,
	    sf,layer,type,size,aa,g->g.inner.width))
	ff_post_error(_("Bad Font"),_("Bad Font"));
}

static void DSP_ChangeFontCallback(void *context,SplineFont *sf,enum sftf_fonttype type,
	int size, int aa, uint32_t script, uint32_t lang, uint32_t *feats) {
    PD *di = context;
    char buf[16];
    int i,j,cnt;
    uint32_t *tags;
    GTextInfo **ti;

    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_AA),aa);

    sprintf(buf,"%d",size);
    GGadgetSetTitle8(GWidgetGetControl(di->gw,CID_Size),buf);

    {
	GTextInfo **ti;
	int i,set; int32_t len;
	ti = GGadgetGetList(GWidgetGetControl(di->gw,CID_Font),&len);
	for ( i=0; i<len; ++i )
	    if ( ti[i]->userdata == sf )
	break;
	if ( i<len ) {
	    GGadgetSelectOneListItem(GWidgetGetControl(di->gw,CID_Font),i);
	    /*GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Font),ti[i]->text);*/
	}
	set = hasFreeType() && !sf->onlybitmaps && sf->subfontcnt==0 &&
		!sf->strokedfont && !sf->multilayer;
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_pfb),set);
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_ttf),set);
	set = hasFreeType() && !sf->onlybitmaps &&
		!sf->strokedfont && !sf->multilayer;
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_otf),set);
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_nohints),hasFreeType());
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_bitmap),sf->bitmaps!=NULL);
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_pfaedit),!sf->onlybitmaps);
    }

    if ( type==sftf_pfb )
	GGadgetSetChecked(GWidgetGetControl(di->gw,CID_pfb),true);
    else if ( type==sftf_ttf )
	GGadgetSetChecked(GWidgetGetControl(di->gw,CID_ttf),true);
    else if ( type==sftf_otf )
	GGadgetSetChecked(GWidgetGetControl(di->gw,CID_otf),true);
    else if ( type==sftf_nohints )
	GGadgetSetChecked(GWidgetGetControl(di->gw,CID_nohints),true);
    else if ( type==sftf_pfaedit )
	GGadgetSetChecked(GWidgetGetControl(di->gw,CID_pfaedit),true);
    else
	GGadgetSetChecked(GWidgetGetControl(di->gw,CID_bitmap),true);

    if ( script==0 ) script = DEFAULT_SCRIPT;
    if ( lang  ==0 ) lang   = DEFAULT_LANG;
    buf[0] = script>>24; buf[1] = script>>16; buf[2] = script>>8; buf[3] = script;
    buf[4] = '{';
    buf[5] = lang>>24; buf[6] = lang>>16; buf[7] = lang>>8; buf[8] = lang;
    buf[9] = '}';
    buf[10] = '\0';
    GGadgetSetTitle8(GWidgetGetControl(di->gw,CID_ScriptLang),buf);

    tags = SFFeaturesInScriptLang(sf,-2,script,lang);
    if ( tags[0]==0 ) {
	free(tags);
	tags = SFFeaturesInScriptLang(sf,-2,script,DEFAULT_LANG);
    }
    for ( cnt=0; tags[cnt]!=0; ++cnt );
    if ( feats!=NULL )
	for ( i=0; feats[i]!=0; ++i ) {
	    for ( j=0; tags[j]!=0; ++j ) {
		if ( feats[i]==tags[j] )
	    break;
	    }
	    if ( tags[j]==0 )
		++cnt;
	}
    ti = malloc((cnt+2)*sizeof(GTextInfo *));
    for ( i=0; tags[i]!=0; ++i ) {
	ti[i] = calloc( 1,sizeof(GTextInfo));
	ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	if ( (tags[i]>>24)<' ' || (tags[i]>>24)>0x7e )
	    sprintf( buf, "<%d,%d>", tags[i]>>16, tags[i]&0xffff );
	else {
	    buf[0] = tags[i]>>24; buf[1] = tags[i]>>16; buf[2] = tags[i]>>8; buf[3] = tags[i]; buf[4] = 0;
	}
	ti[i]->text = uc_copy(buf);
	ti[i]->userdata = (void *) (intptr_t) tags[i];
	if ( feats!=NULL ) {
	    for ( j=0; feats[j]!=0; ++j ) {
		if ( feats[j] == tags[i] ) {
		    ti[i]->selected = true;
	    break;
		}
	    }
	}
    }
    cnt = i;
    if ( feats!=NULL )
	for ( i=0; feats[i]!=0; ++i ) {
	    for ( j=0; tags[j]!=0; ++j ) {
		if ( feats[i]==tags[j] )
	    break;
	    }
	    if ( tags[j]==0 ) {
		ti[cnt] = calloc( 1,sizeof(GTextInfo));
		ti[cnt]->bg = COLOR_DEFAULT;
		ti[cnt]->fg = COLOR_CREATE(0x70,0x70,0x70);
		ti[cnt]->selected = true;
		buf[0] = feats[i]>>24; buf[1] = feats[i]>>16; buf[2] = feats[i]>>8; buf[3] = feats[i]; buf[4] = 0;
		ti[cnt]->text = uc_copy(buf);
		ti[cnt++]->userdata = (void *) (intptr_t) feats[i];
	    }
	}
    ti[cnt] = calloc(1,sizeof(GTextInfo));
    /* These will become ordered because the list widget will do that */
    GGadgetSetList(GWidgetGetControl(di->gw,CID_Features),ti,false);
    free(tags);
}

static int DSP_AAState(SplineFont *sf,BDFFont *bestbdf) {
    /* What should AntiAlias look like given the current set of bitmaps */
    int anyaa=0, anybit=0;
    BDFFont *bdf;

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	if ( bdf->clut==NULL )
	    anybit = true;
	else
	    anyaa = true;
    if ( anybit && anyaa )
return( gg_visible | gg_enabled | (bestbdf!=NULL && bestbdf->clut!=NULL ? gg_cb_on : 0) );
    else if ( anybit )
return( gg_visible );
    else if ( anyaa )
return( gg_visible | gg_cb_on );

return( gg_visible );
}

static int DSP_FontChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	GTextInfo *sel = GGadgetGetListItemSelected(g);
	SplineFont *sf;
	BDFFont *best;
	int flags, pick = 0, i;
	char size[12]; unichar_t usize[12];
	uint16_t cnt;

	if ( sel==NULL )
return( true );
	sf = sel->userdata;

	TextInfoDataFree(di->scriptlangs);
	di->scriptlangs = SLOfFont(sf);
	GGadgetSetList(GWidgetGetControl(di->gw,CID_ScriptLang),
		GTextInfoArrayFromList(di->scriptlangs,&cnt),false);

	if ( GGadgetIsChecked(GWidgetGetControl(di->gw,CID_bitmap)) && sf->bitmaps==NULL )
	    pick = true;
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_bitmap),sf->bitmaps!=NULL);
	if ( !hasFreeType() || sf->onlybitmaps ) {
	    best = DSP_BestMatchDlg(di);
	    flags = DSP_AAState(sf,best);
	    GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_AA),flags&gg_enabled);
	    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_AA),flags&gg_cb_on);
	    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_bitmap),true);
	    for ( i=CID_pfb; i<=CID_nohints; ++i )
		GGadgetSetEnabled(GWidgetGetControl(di->gw,i),false);
	    if ( best!=NULL ) {
		sprintf( size, "%d", best->pixelsize );
		uc_strcpy(usize,size);
		GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Size),usize);
	    }
	    pick = true;
	} else if ( sf->subfontcnt!=0 ) {
	    for ( i=CID_pfb; i<CID_otf; ++i ) {
		GGadgetSetEnabled(GWidgetGetControl(di->gw,i),false);
		if ( GGadgetIsChecked(GWidgetGetControl(di->gw,i)) )
		    pick = true;
	    }
	    GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_otf),true);
	    if ( pick )
		GGadgetSetChecked(GWidgetGetControl(di->gw,CID_otf),true);
	} else {
	    for ( i=CID_pfb; i<=CID_otf; ++i )
		GGadgetSetEnabled(GWidgetGetControl(di->gw,i),true);
	    if ( pick )
		GGadgetSetChecked(GWidgetGetControl(di->gw,CID_pfb),true);
	}
	if ( pick )
	    DSP_SetFont(di,false);
	else
	    SFTFSetFont(GWidgetGetControl(di->gw,CID_SampleText),-1,-1,sf);
    }
return( true );
}

static int DSP_AAChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetIsChecked(GWidgetGetControl(di->gw,CID_bitmap)) ) {
	    int val = u_strtol(_GGadgetGetTitle(GWidgetGetControl(di->gw,CID_Size)),NULL,10);
	    int bestdiff = 8000, bdfdiff;
	    BDFFont *bdf, *best=NULL;
	    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
	    SplineFont *sf;
	    int aa = GGadgetIsChecked(GWidgetGetControl(di->gw,CID_AA));
	    if ( sel==NULL )
return( true );
	    sf = sel->userdata;
	    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
		if ( (aa && bdf->clut) || (!aa && bdf->clut==NULL)) {
		    if ((bdfdiff = bdf->pixelsize-val)<0 ) bdfdiff = -bdfdiff;
		    if ( bdfdiff<bestdiff ) {
			best = bdf;
			bestdiff = bdfdiff;
			if ( bdfdiff==0 )
	    break;
		    }
		}
	    }
	    if ( best!=NULL ) {
		char size[12]; unichar_t usize[12];
		sprintf( size, "%d", best->pixelsize );
		uc_strcpy(usize,size);
		GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Size),usize);
	    }
	    DSP_SetFont(di,false);
	} else
	    SFTFSetAntiAlias(GWidgetGetControl(di->gw,CID_SampleText),-1,-1,
		    GGadgetIsChecked(GWidgetGetControl(di->gw,CID_AA)));
    }
return( true );
}

static int DSP_SizeChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textfocuschanged &&
	    !e->u.control.u.tf_focus.gained_focus ) {
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	int err=false;
	int size = GetInt8(di->gw,CID_Size,_("_Size:"),&err);
	if ( err || size<4 )
return( true );
	if ( GWidgetGetControl(di->gw,CID_SampleText)==NULL )
return( true );		/* Happens during startup */
	if ( GGadgetIsChecked(GWidgetGetControl(di->gw,CID_bitmap)) ) {
	    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
	    SplineFont *sf;
	    BDFFont *bdf, *best=NULL;
	    int aa = GGadgetIsChecked(GWidgetGetControl(di->gw,CID_AA));
	    if ( sel==NULL )
return( true );
	    sf = sel->userdata;
	    for ( bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
		if ( bdf->pixelsize==size ) {
		    if (( bdf->clut && aa ) || ( bdf->clut==NULL && !aa )) {
			best = bdf;
	    break;
		    }
		    best = bdf;
		}
	    }
	    if ( best==NULL ) {
		char buf[100], *pt=buf, *end=buf+sizeof(buf)-10;
		unichar_t ubuf[12];
		int lastsize = 0;
		for ( bdf=sf->bitmaps; bdf!=NULL && pt<end; bdf=bdf->next ) {
		    if ( bdf->pixelsize!=lastsize ) {
			sprintf( pt, "%d,", bdf->pixelsize );
			lastsize = bdf->pixelsize;
			pt += strlen(pt);
		    }
		}
		if ( pt!=buf )
		    pt[-1] = '\0';
		ff_post_error(_("Bad Size"),_("Requested bitmap size not available in font. Font supports %s"),buf);
		best = DSP_BestMatchDlg(di);
		if ( best!=NULL ) {
		    sprintf( buf, "%d", best->pixelsize);
		    uc_strcpy(ubuf,buf);
		    GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Size),ubuf);
		    size = best->pixelsize;
		}
	    }
	    if ( best==NULL )
return(true);
	    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_AA),best->clut!=NULL );
	    DSP_SetFont(di,false);
	} else
	    SFTFSetSize(GWidgetGetControl(di->gw,CID_SampleText),-1,-1,size);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	/* Don't change the font on each change to the text field, that might */
	/*  look rather odd. But wait until we think they may have finished */
	/*  typing. Either when they change the focus (above) or when they */
	/*  just don't do anything for a while */
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	if ( di->sizechanged!=NULL )
	    GDrawCancelTimer(di->sizechanged);
	di->sizechanged = GDrawRequestTimer(di->gw,600,0,NULL);
    }
return( true );
}

static void DSP_SizeChangedTimer(PD *di) {
    GEvent e;

    di->sizechanged = NULL;
    memset(&e,0,sizeof(e));
    e.type = et_controlevent;
    e.u.control.g = GWidgetGetControl(di->gw,CID_Size);
    e.u.control.subtype = et_textfocuschanged;
    e.u.control.u.tf_focus.gained_focus = false;
    DSP_SizeChanged(e.u.control.g,&e);
}


static int DSP_WidthChanged(GGadget *g, GEvent *e) {
    if ( e==NULL ||
	    (e->type==et_controlevent && e->u.control.subtype == et_textfocuschanged &&
	     !e->u.control.u.tf_focus.gained_focus )) {
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	int err=false;
	int width = GetInt8(di->gw,CID_TextWidth,_("Width"),&err);
	GGadget *sample = GWidgetGetControl(di->gw,CID_SampleText);
	GRect outer;
	int bp;
	if ( err || width<20 || width>2000 )
return( true );
	if ( !di->ready )
return( true );
	bp = GBoxBorderWidth(di->gw,sample->box);
	GGadgetGetSize(sample,&outer);
	outer.width = rint(width*lastdpi/72.0)+2*bp;
	GGadgetSetDesiredSize(sample,&outer,NULL);
	GHVBoxFitWindow(GWidgetGetControl(di->gw,CID_TopBox));
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	/* Don't change the font on each change to the text field, that might */
	/*  look rather odd. But wait until we think they may have finished */
	/*  typing. Either when they change the focus (above) or when they */
	/*  just don't do anything for a while */
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	if ( di->widthchanged!=NULL )
	    GDrawCancelTimer(di->widthchanged);
	di->widthchanged = GDrawRequestTimer(di->gw,600,0,NULL);
    }
return( true );
}

static void DSP_WidthChangedTimer(PD *di) {

    di->widthchanged = NULL;
    DSP_WidthChanged(GWidgetGetControl(di->gw,CID_TextWidth),NULL);
}

static void DSP_JustResized(PD *di) {
    GGadget *sample = GWidgetGetControl(di->gw,CID_SampleText);
    GGadget *widthfield = GWidgetGetControl(di->gw,CID_TextWidth);
    char buffer[20];

    di->resized = NULL;
    if ( lastdpi!=0 && widthfield!=NULL ) {
	sprintf( buffer, "%d", (int) rint( sample->inner.width*72/lastdpi ));
	GGadgetSetTitle8(widthfield,buffer);
    }
}

static int DSP_DpiChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textfocuschanged &&
	    !e->u.control.u.tf_focus.gained_focus ) {
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	int err=false;
	int dpi = GetInt8(di->gw,CID_DPI,_("DPI"),&err);
	GGadget *sample = GWidgetGetControl(di->gw,CID_SampleText);
	GGadget *widthfield = GWidgetGetControl(di->gw,CID_TextWidth);
	if ( err || dpi<20 || dpi>300 )
return( true );
	if ( !di->ready )
return( true );		/* Happens during startup */
	if ( lastdpi==dpi )
return( true );
	SFTFSetDPI(sample,dpi);
	lastdpi = dpi;
	if ( widthfield!=NULL ) {
	    DSP_WidthChanged(widthfield,NULL);
	} else {
	    GGadgetRedraw(sample);
	}
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	/* Don't change the font on each change to the text field, that might */
	/*  look rather odd. But wait until we think they may have finished */
	/*  typing. Either when they change the focus (above) or when they */
	/*  just don't do anything for a while */
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	if ( di->dpichanged!=NULL )
	    GDrawCancelTimer(di->dpichanged);
	di->dpichanged = GDrawRequestTimer(di->gw,600,0,NULL);
    }
return( true );
}

static void DSP_DpiChangedTimer(PD *di) {
    GEvent e;

    di->dpichanged = NULL;
    memset(&e,0,sizeof(e));
    e.type = et_controlevent;
    e.u.control.g = GWidgetGetControl(di->gw,CID_Size);
    e.u.control.subtype = et_textfocuschanged;
    e.u.control.u.tf_focus.gained_focus = false;
    DSP_DpiChanged(e.u.control.g,&e);
}

static int DSP_RadioSet(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetIsChecked(GWidgetGetControl(di->gw,CID_bitmap)) ) {
	    BDFFont *best = DSP_BestMatchDlg(di);
	    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
	    SplineFont *sf;
	    int flags;
	    char size[14]; unichar_t usize[14];

	    if ( sel!=NULL ) {
		sf = sel->userdata;
		flags = DSP_AAState(sf,best);
		GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_AA),flags&gg_enabled);
		GGadgetSetChecked(GWidgetGetControl(di->gw,CID_AA),flags&gg_cb_on);
		if ( best!=NULL ) {
		    sprintf( size, "%g",
			    rint(best->pixelsize*72.0/SFTFGetDPI(GWidgetGetControl(di->gw,CID_SampleText))) );
		    uc_strcpy(usize,size);
		    GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Size),usize);
		}
	    }
	    DSP_SetFont(di,false);
	} else {
	    /*GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_AA),true);*/
	    SFTFSetFontType(GWidgetGetControl(di->gw,CID_SampleText),-1,-1,
		    DSP_FontType(di));
	}
    }
return( true );
}

static int DSP_ScriptLangChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	const unichar_t *sstr = _GGadgetGetTitle(g);
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	uint32_t script, lang;

	if ( e->u.control.u.tf_changed.from_pulldown!=-1 ) {
	    GGadgetSetTitle8(g,di->scriptlangs[e->u.control.u.tf_changed.from_pulldown].userdata );
	    sstr = _GGadgetGetTitle(g);
	} else {
	    if ( u_strlen(sstr)<4 || !isalpha(sstr[0]) || !isalnum(sstr[1]) /*|| !isalnum(sstr[2]) || !isalnum(sstr[3])*/ )
return( true );
	    if ( u_strlen(sstr)==4 )
		/* No language, we'll use default */;
	    else if ( u_strlen(sstr)!=10 || sstr[4]!='{' || sstr[9]!='}' ||
		    !isalpha(sstr[5]) || !isalpha(sstr[6]) || !isalpha(sstr[7])  )
return( true );
	}
	script = DEFAULT_SCRIPT;
	lang = DEFAULT_LANG;
	if ( u_strlen(sstr)>=4 )
	    script = (sstr[0]<<24) | (sstr[1]<<16) | (sstr[2]<<8) | sstr[3];
	if ( sstr[4]=='{' && u_strlen(sstr)>=9 )
	    lang = (sstr[5]<<24) | (sstr[6]<<16) | (sstr[7]<<8) | sstr[8];
	SFTFSetScriptLang(GWidgetGetControl(di->gw,CID_SampleText),-1,-1,script,lang);
    }
return( true );
}

static int DSP_Menu(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonpress ) {
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *st = GWidgetGetControl(di->gw,CID_SampleText);
	GEvent fudge;
	GPoint p;
	memset(&fudge,0,sizeof(fudge));
	fudge.type = et_mousedown;
	fudge.w = st->base;
	p.x = g->inner.x+g->inner.width/2;
	p.y = g->inner.y+g->inner.height;
	GDrawTranslateCoordinates(g->base,st->base,&p);
	fudge.u.mouse.x = p.x; fudge.u.mouse.y = p.y;
	SFTFPopupMenu((SFTextArea *) st,&fudge);
    }
return( true );
}

static int DSP_FeaturesChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	uint32_t *feats;
	int32_t len;
	GTextInfo **ti = GGadgetGetList(g,&len);
	int i,cnt;

	for ( i=cnt=0; i<len; ++i )
	    if ( ti[i]->selected ) ++cnt;
	feats = malloc((cnt+1)*sizeof(uint32_t));
	for ( i=cnt=0; i<len; ++i )
	    if ( ti[i]->selected )
		feats[cnt++] = (intptr_t) ti[i]->userdata;
	feats[cnt] = 0;
	/* These will be ordered because the list widget will do that */
	SFTFSetFeatures(GWidgetGetControl(di->gw,CID_SampleText),-1,-1,feats);
    }
return( true );
}

static int PRT_Bind(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	PD *pi = GDrawGetUserData(GGadgetGetWindow(g));
	int on = GGadgetIsChecked(g);
	GGadgetSetEnabled(GWidgetGetControl(pi->gw,CID_Scale),on);
	GGadgetSetEnabled(GWidgetGetControl(pi->gw,CID_Start),on);
	GGadgetSetEnabled(GWidgetGetControl(pi->gw,CID_Center),on);
	GGadgetSetEnabled(GWidgetGetControl(pi->gw,CID_End),on);
    }
return( true );
}

static int DSP_TextChanged(GGadget *g, GEvent *e) {
    if ( e==NULL ||
	    (e->type==et_controlevent && e->u.control.subtype == et_textchanged )) {
	PD *di = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *txt = _GGadgetGetTitle(g);
	const unichar_t *pt;
	SFTextArea *ta = (SFTextArea *) g;
	LayoutInfo *li = &ta->li;
	char buffer[200];

	for ( pt=txt; *pt!='\0' && ScriptFromUnicode(*pt,NULL)==DEFAULT_SCRIPT; ++pt);
	if ( *pt=='\0' ) {
	    if ( !di->script_unknown ) {
		di->script_unknown = true;
		if ( li->fontlist!=NULL ) {
		    li->fontlist->script = DEFAULT_SCRIPT;
		    li->fontlist->lang   = DEFAULT_LANG;
		}
		GGadgetSetTitle8(GWidgetGetControl(di->gw,CID_ScriptLang),"DFLT{dflt}");
	    }
	} else if ( di->script_unknown ) {
	    uint32_t script = ScriptFromUnicode(*pt,NULL);
	    struct fontlist *fl;
	    unichar_t buf[20];
	    for ( fl=li->fontlist; fl!=NULL && ta->sel_start>fl->end; fl=fl->next );
	    if ( fl!=NULL && (fl->script==DEFAULT_SCRIPT || fl->script==0 )) {
		for ( fl=li->fontlist; fl!=NULL; fl=fl->next ) {
		    if ( fl->script==DEFAULT_SCRIPT || fl->script == 0 ) {
			fl->script = script;
			fl->lang = DEFAULT_LANG;
		    }
		}
		buf[0] = (script>>24)&0xff;
		buf[1] = (script>>16)&0xff;
		buf[2] = (script>>8 )&0xff;
		buf[3] = (script    )&0xff;
		uc_strcpy(buf+4,"{dflt}");
		GGadgetSetTitle(GWidgetGetControl(di->gw,CID_ScriptLang),buf);
	    }
	    di->script_unknown = false;
	}

	if ( lastdpi!=0) {
	    sprintf( buffer,_("Text Width:%4d"), (int) rint(li->xmax*72.0/lastdpi));
	    GGadgetSetTitle8(GWidgetGetControl(di->gw,CID_ActualWidth),buffer);
	}
    }
return( true );
}

static int DSP_Done(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	PD *di = GDrawGetUserData(gw);
	if ( di->done!=NULL )
	    *(di->done) = true;
	GDrawDestroyWindow(di->gw);
    }
return( true );
}

static int dsp_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	PD *di = GDrawGetUserData(gw);
	if ( di->done!=NULL )
	    *(di->done) = true;
	GDrawDestroyWindow(di->gw);
    } else if ( event->type==et_destroy ) {
	PD *di = GDrawGetUserData(gw);
	TextInfoDataFree(di->scriptlangs);
	free(di);
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/display.html", NULL);
return( true );
	} else if ( GMenuIsCommand(event,H_("Quit|Ctl+Q") )) {
	    MenuExit(NULL,NULL,NULL);
    return( false );
	} else if ( GMenuIsCommand(event,H_("Close|Ctl+Shft+Q") )) {
	    PD *di = GDrawGetUserData(gw);
	    di->pi.done = true;
	}
return( false );
    } else if ( event->type==et_timer ) {
	PD *di = GDrawGetUserData(gw);
	if ( event->u.timer.timer==di->sizechanged )
	    DSP_SizeChangedTimer(di);
	else if ( event->u.timer.timer==di->dpichanged )
	    DSP_DpiChangedTimer(di);
	else if ( event->u.timer.timer==di->widthchanged )
	    DSP_WidthChangedTimer(di);
	else if ( event->u.timer.timer==di->resized )
	    DSP_JustResized(di);
    } else if ( event->type==et_resize ) {
	PD *di = GDrawGetUserData(gw);
	if ( di->resized!=NULL )
	    GDrawCancelTimer(di->resized);
	di->resized = GDrawRequestTimer(di->gw,300,0,NULL);
    }
return( true );
}
    
static void _PrintFFDlg(SplineChar *sc,
	CharView *cv,SplineSet *fit_to_path) {
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[20], boxes[15], mgcd[5], pgcd[8], vbox, tgcd[14],
	    *harray[8], *farray[8],
	    *barray2[8], *varray[11], *varray2[9], *harray2[5],
	    *regenarray[9], *varray6[3], *tarray[18], *alarray[6],
	    *patharray[5], *oarray[4];
    GTextInfo label[20], mlabel[5], plabel[8], tlabel[14];
    int dpi;
    char buf[12], dpibuf[12], widthbuf[12], pathlen[32];
    SplineFont *sf = sc->parent;
    int hasfreetype = hasFreeType();
    BDFFont *bestbdf=DSP_BestMatch(sf,true,12);
    PD *active;
    int done = false;
    int width = 300;
    extern int _GScrollBar_Width;

    if ( sf->cidmaster )
	sf = sf->cidmaster;

    active = calloc(1,sizeof(PD));

    PI_Init(&active->pi,NULL,sc);
    active->cv = cv;
    active->fit_to_path = fit_to_path;
    active->done = &done;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = false;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.is_dlg = true;
    wattrs.utf8_window_title = _("Insert Text Outlines");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,410));
    pos.height = GDrawPointsToPixels(NULL,330);
    active->gw = GDrawCreateTopWindow(NULL,&pos,dsp_e_h,active,&wattrs);

    memset(&vbox,0,sizeof(vbox));
    memset(&label,0,sizeof(label));
    memset(&mlabel,0,sizeof(mlabel));
    memset(&plabel,0,sizeof(plabel));
    memset(&tlabel,0,sizeof(tlabel));
    memset(&gcd,0,sizeof(gcd));
    memset(&mgcd,0,sizeof(mgcd));
    memset(&pgcd,0,sizeof(pgcd));
    memset(&tgcd,0,sizeof(tgcd));
    memset(&boxes,0,sizeof(boxes));

    label[0].text = (unichar_t *) sf->fontname;
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.popup_msg = _("Select some text, then use this list to change the\nfont in which those characters are displayed.");
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; 
    gcd[0].gd.pos.width = 150;
    gcd[0].gd.cid = CID_Font;
    gcd[0].gd.u.list = FontNames(sf);
    gcd[0].gd.flags = (fv_list==NULL || gcd[0].gd.u.list[1].text==NULL ) ?
	    (gg_visible):
	    (gg_visible | gg_enabled);
    gcd[0].gd.handle_controlevent = DSP_FontChanged;
    gcd[0].creator = GListButtonCreate;
    varray[0] = &gcd[0]; varray[1] = NULL;

    label[2].text = (unichar_t *) _("_Size:");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 210; gcd[2].gd.pos.y = gcd[0].gd.pos.y+6; 
    gcd[2].gd.flags = gg_visible | gg_enabled;
    gcd[2].gd.popup_msg = _("Select some text, this specifies the vertical\nsize of those characters in em-units");
    gcd[2].gd.cid = CID_SizeLab;
    gcd[2].creator = GLabelCreate;
    harray[0] = &gcd[2];

    if ( bestbdf !=NULL && ( !hasfreetype || sf->onlybitmaps ))
	sprintf( buf, "%d", bestbdf->pixelsize );
    else
	strcpy(buf,"12");
    label[3].text = (unichar_t *) buf;
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = 240; gcd[3].gd.pos.y = gcd[0].gd.pos.y+3; 
    gcd[3].gd.pos.width = 40;
    gcd[3].gd.flags = gg_visible | gg_enabled;
    gcd[3].gd.cid = CID_Size;
    gcd[3].gd.handle_controlevent = DSP_SizeChanged;
    gcd[3].gd.popup_msg = _("Select some text, this specifies the pixel\nsize of those characters");
    gcd[3].creator = GNumericFieldCreate;
    harray[1] = &gcd[3]; harray[2] = GCD_HPad10;

    label[1].text = (unichar_t *) _("_AA");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 170; gcd[1].gd.pos.y = gcd[0].gd.pos.y+3; 
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_cb_on;
    if ( sf->bitmaps!=NULL && ( !hasfreetype || sf->onlybitmaps ))
	gcd[1].gd.flags = DSP_AAState(sf,bestbdf);
    gcd[1].gd.flags = gg_enabled | gg_cb_on;
    gcd[1].gd.popup_msg = _("Select some text, this controls whether those characters will be\nAntiAlias (greymap) characters, or bitmap characters");
    gcd[1].gd.handle_controlevent = DSP_AAChange;
    gcd[1].gd.cid = CID_AA;
    gcd[1].creator = GCheckBoxCreate;
    harray[3] = &gcd[1]; harray[4] = GCD_Glue; harray[5] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray;
    boxes[2].creator = GHBoxCreate;
    varray[2] = &boxes[2]; varray[3] = NULL;

    label[4].text = (unichar_t *) "pfb";
    label[4].text_is_1byte = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.pos.x = gcd[0].gd.pos.x; gcd[4].gd.pos.y = 24+gcd[3].gd.pos.y; 
    gcd[4].gd.flags = gg_visible | gg_enabled;
    gcd[4].gd.cid = CID_pfb;
    gcd[4].gd.handle_controlevent = DSP_RadioSet;
    gcd[4].gd.popup_msg = _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  nohints -- freetype rasterizes without hints\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[4].creator = GRadioCreate;
    if ( sf->subfontcnt!=0 || !hasfreetype || sf->onlybitmaps || sf->strokedfont || sf->multilayer ) gcd[4].gd.flags = gg_visible;
    farray[0] = &gcd[4];

    label[5].text = (unichar_t *) "ttf";
    label[5].text_is_1byte = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.pos.x = 46; gcd[5].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[5].gd.flags = gg_visible | gg_enabled;
    gcd[5].gd.cid = CID_ttf;
    gcd[5].gd.handle_controlevent = DSP_RadioSet;
    gcd[5].gd.popup_msg = _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  nohints -- freetype rasterizes without hints\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[5].creator = GRadioCreate;
    if ( sf->subfontcnt!=0 || !hasfreetype || sf->onlybitmaps || sf->strokedfont || sf->multilayer ) gcd[5].gd.flags = gg_visible;
    else if ( sf->layers[ly_fore].order2 ) gcd[5].gd.flags |= gg_cb_on;
    farray[1] = &gcd[5];

    label[6].text = (unichar_t *) "otf";
    label[6].text_is_1byte = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.pos.x = 114; gcd[6].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[6].gd.flags = gg_visible | gg_enabled;
    gcd[6].gd.cid = CID_otf;
    gcd[6].gd.handle_controlevent = DSP_RadioSet;
    gcd[6].gd.popup_msg = _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  nohints -- freetype rasterizes without hints\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[6].creator = GRadioCreate;
    if ( !hasfreetype || sf->onlybitmaps || sf->strokedfont || sf->multilayer ) gcd[6].gd.flags = gg_visible;
    else if ( sf->subfontcnt!=0 || !sf->layers[ly_fore].order2 ) gcd[6].gd.flags |= gg_cb_on;
    farray[2] = &gcd[6];

    label[7].text = (unichar_t *) _("nohints");
    label[7].text_is_1byte = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.pos.x = 114; gcd[7].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[7].gd.flags = gg_visible | gg_enabled;
    gcd[7].gd.cid = CID_nohints;
    gcd[7].gd.handle_controlevent = DSP_RadioSet;
    gcd[7].gd.popup_msg = _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  nohints -- freetype rasterizes without hints\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[7].creator = GRadioCreate;
    if ( !hasfreetype || sf->onlybitmaps ) gcd[7].gd.flags = gg_visible;
    else if ( sf->strokedfont || sf->multilayer ) gcd[7].gd.flags |= gg_cb_on;
    farray[3] = &gcd[7];
    

    label[8].text = (unichar_t *) "bitmap";
    label[8].text_is_1byte = true;
    gcd[8].gd.label = &label[8];
    gcd[8].gd.pos.x = 148; gcd[8].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[8].gd.flags = gg_visible | gg_enabled;
    gcd[8].gd.cid = CID_bitmap;
    gcd[8].gd.handle_controlevent = DSP_RadioSet;
    gcd[8].gd.popup_msg = _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  nohints -- freetype rasterizes without hints\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[8].creator = GRadioCreate;
    if ( sf->bitmaps==NULL ) gcd[8].gd.flags = gg_visible;
    else if ( sf->onlybitmaps ) gcd[8].gd.flags |= gg_cb_on;
    farray[4] = &gcd[8];

    label[9].text = (unichar_t *) "FontForge";
    label[9].text_is_1byte = true;
    gcd[9].gd.label = &label[9];
    gcd[9].gd.pos.x = 200; gcd[9].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[9].gd.flags = gg_visible | gg_enabled;
    gcd[9].gd.cid = CID_pfaedit;
    gcd[9].gd.handle_controlevent = DSP_RadioSet;
    gcd[9].gd.popup_msg = _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  nohints -- freetype rasterizes without hints\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[9].creator = GRadioCreate;
    if ( sf->onlybitmaps ) gcd[9].gd.flags = gg_visible;
    if ( !hasfreetype && sf->bitmaps==NULL ) gcd[9].gd.flags |= gg_cb_on;
    else if ( sf->onlybitmaps ) gcd[9].gd.flags = gg_visible;
    farray[5] = &gcd[9]; farray[6] = GCD_Glue; farray[7] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = farray;
    boxes[3].creator = GHBoxCreate;
    varray[4] = &boxes[3]; varray[5] = NULL;
    for ( int k=4; k<=9; ++k )
        gcd[k].gd.flags &= ~gg_visible;
    boxes[3].gd.flags = gg_enabled;

    label[10].text = (unichar_t *) "DFLT{dflt}";
    label[10].text_is_1byte = true;
    gcd[10].gd.label = &label[10];
    gcd[10].gd.popup_msg = _("Select some text, then use this list to specify\nthe current script & language.");
    gcd[10].gd.pos.x = 12; gcd[10].gd.pos.y = 6; 
    gcd[10].gd.pos.width = 150;
    gcd[10].gd.flags = gg_visible | gg_enabled;
    gcd[10].gd.cid = CID_ScriptLang;
    gcd[10].gd.u.list = active->scriptlangs = SLOfFont(sf);
    gcd[10].gd.handle_controlevent = DSP_ScriptLangChanged;
    gcd[10].creator = GListFieldCreate;
    varray[6] = GCD_Glue; varray[7] = NULL;
    varray[8] = &gcd[10]; varray[9] = NULL; varray[10] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = varray;
    boxes[4].creator = GHVBoxCreate;
    harray2[1] = &boxes[4];

    gcd[11].gd.popup_msg = _("Select some text, then use this list to specify\nactive features.");
    gcd[11].gd.pos.width = 50;
    gcd[11].gd.flags = gg_visible | gg_enabled| gg_list_alphabetic | gg_list_multiplesel;
    gcd[11].gd.cid = CID_Features;
    gcd[11].gd.handle_controlevent = DSP_FeaturesChanged;
    gcd[11].creator = GListCreate;
    harray2[0] = &gcd[11];

    label[12].image = &GIcon_menudelta;
    gcd[12].gd.label = &label[12];
    gcd[12].gd.popup_msg = _("Menu");
    gcd[12].gd.flags = gg_visible | gg_enabled;
    gcd[12].gd.handle_controlevent = DSP_Menu;
    gcd[12].creator = GButtonCreate;

    varray6[0] = GCD_Glue; varray6[1] = &gcd[12]; varray6[2] = NULL;
    vbox.gd.flags = gg_enabled|gg_visible;
    vbox.gd.u.boxelements = varray6;
    vbox.creator = GVBoxCreate;

    harray2[2] = &vbox; harray2[3] = GCD_Glue; harray2[4] = NULL;

    boxes[5].gd.flags = gg_enabled|gg_visible;
    boxes[5].gd.u.boxelements = harray2;
    boxes[5].creator = GHBoxCreate;
    varray2[0] = &boxes[5]; varray2[1] = NULL;


    gcd[13].gd.pos.x = 5; gcd[13].gd.pos.y = 20+gcd[13].gd.pos.y; 
    gcd[13].gd.pos.width = 400; gcd[13].gd.pos.height = 236; 
    gcd[13].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap | gg_text_xim;
    gcd[13].gd.handle_controlevent = DSP_TextChanged;
    gcd[13].gd.cid = CID_SampleText;
    gcd[13].creator = SFTextAreaCreate;
    varray2[2] = &gcd[13]; varray2[3] = NULL;

    gcd[14].gd.flags = gg_visible | gg_enabled ;
    gcd[14].creator = GLineCreate;
    varray2[4] = &gcd[14]; varray2[5] = NULL;

    label[15].text = (unichar_t *) _("DPI:");
    label[15].text_is_1byte = true;
    gcd[15].gd.label = &label[15];
    gcd[15].gd.flags = gg_visible | gg_enabled;
    gcd[15].gd.popup_msg = _("Specifies screen dots per inch");
    gcd[15].creator = GLabelCreate;

    if ( lastdpi==0 )
	lastdpi = GDrawPointsToPixels(NULL,72);
    dpi = lastdpi;
    sprintf( dpibuf, "%d", dpi );
    label[16].text = (unichar_t *) dpibuf;
    label[16].text_is_1byte = true;
    gcd[16].gd.label = &label[16];
    gcd[16].gd.pos.width = 50;
    gcd[16].gd.flags = gg_visible | gg_enabled;
    gcd[16].gd.popup_msg = _("Specifies screen dots per inch");
    gcd[16].gd.cid = CID_DPI;
    gcd[16].gd.handle_controlevent = DSP_DpiChanged;
    gcd[16].creator = GNumericFieldCreate;

    regenarray[0] = &gcd[15]; regenarray[1] = &gcd[16]; regenarray[2] = GCD_Glue;
	label[17].text = (unichar_t *) _("Text Width:    0");
	label[17].text_is_1byte = true;
	gcd[17].gd.label = &label[17];
	gcd[17].gd.flags = gg_visible | gg_enabled;
	gcd[17].gd.cid = CID_ActualWidth;
	gcd[17].creator = GLabelCreate;

	label[18].text = (unichar_t *) _("Wrap Pos:");
	label[18].text_is_1byte = true;
	gcd[18].gd.label = &label[18];
	gcd[18].gd.flags = gg_visible | gg_enabled;
	gcd[18].gd.popup_msg = _("The text will wrap to a new line after this many em-units");
	gcd[18].creator = GLabelCreate;

	sprintf( widthbuf, "%d", width );
	label[19].text = (unichar_t *) widthbuf;
	label[19].text_is_1byte = true;
	gcd[19].gd.label = &label[19];
	gcd[19].gd.pos.width = 50;
	gcd[19].gd.flags = gg_visible | gg_enabled;
	gcd[19].gd.popup_msg = _("The text will wrap to a new line after this many em-units");
	gcd[19].gd.cid = CID_TextWidth;
	gcd[19].gd.handle_controlevent = DSP_WidthChanged;
	gcd[19].creator = GNumericFieldCreate;
	regenarray[3] = &gcd[17]; regenarray[4] = GCD_Glue;
	regenarray[5] = &gcd[18]; regenarray[6] = &gcd[19]; regenarray[7] = NULL;
	gcd[13].gd.pos.width = GDrawPixelsToPoints(NULL,width*dpi/72) + _GScrollBar_Width+4;

    boxes[12].gd.flags = gg_enabled|gg_visible;
    boxes[12].gd.u.boxelements = regenarray;
    boxes[12].creator = GHBoxCreate;
    varray2[6] = &boxes[12]; varray2[7] = NULL; varray2[8] = NULL;

    boxes[6].gd.flags = gg_enabled|gg_visible;
    boxes[6].gd.u.boxelements = varray2;
    boxes[6].creator = GHVBoxCreate;

	tarray[0] = &boxes[6]; tarray[1] = NULL;

	tgcd[0].gd.flags = fit_to_path==NULL ? gg_visible : (gg_visible | gg_enabled | gg_cb_on);
	tlabel[0].text = (unichar_t *) _("Bind to Path");
	tlabel[0].text_is_1byte = true;
	tlabel[0].text_in_resource = true;
	tgcd[0].gd.label = &tlabel[0];
	tgcd[0].gd.handle_controlevent = PRT_Bind;
	tgcd[0].gd.cid = CID_Bind;
	tgcd[0].creator = GCheckBoxCreate;
	patharray[0] = &tgcd[0]; patharray[1] = GCD_Glue;

	if ( fit_to_path==NULL ) {
	    tgcd[1].gd.flags = 0;
	    strcpy(pathlen,"0");
	} else {
	    tgcd[1].gd.flags = gg_visible | gg_enabled;
	    sprintf(pathlen, _("Path Length: %g"), PathLength(fit_to_path));
	}
	tlabel[1].text = (unichar_t *) pathlen;
	tlabel[1].text_is_1byte = true;
	tlabel[1].text_in_resource = true;
	tgcd[1].gd.label = &tlabel[1];
	tgcd[1].creator = GLabelCreate;
	patharray[2] = &tgcd[1]; patharray[3] = NULL;

	boxes[9].gd.flags = gg_enabled|gg_visible;
	boxes[9].gd.u.boxelements = patharray;
	boxes[9].creator = GHBoxCreate;
	tarray[2] = &boxes[9]; tarray[3] = NULL;

	tgcd[2].gd.flags = fit_to_path==NULL ? 0 : (gg_visible | gg_enabled | gg_cb_on);
	tlabel[2].text = (unichar_t *) _("Scale so text width matches path length");
	tlabel[2].text_is_1byte = true;
	tlabel[2].text_in_resource = true;
	tgcd[2].gd.label = &tlabel[2];
	tgcd[2].gd.cid = CID_Scale;
	tgcd[2].creator = GCheckBoxCreate;
	tarray[4] = &tgcd[2]; tarray[5] = NULL;

	tgcd[3].gd.flags = fit_to_path==NULL ? 0 : (gg_visible | gg_enabled);
	tlabel[3].text = (unichar_t *) _("Rotate each glyph as a unit");
	tlabel[3].text_is_1byte = true;
	tlabel[3].text_in_resource = true;
	tgcd[3].gd.label = &tlabel[3];
	tgcd[3].gd.cid = CID_GlyphAsUnit;
	tgcd[3].creator = GCheckBoxCreate;
	tarray[6] = &tgcd[3]; tarray[7] = NULL;

	tgcd[4].gd.flags = fit_to_path==NULL ? 0 : (gg_visible | gg_enabled);
	tlabel[4].text = (unichar_t *) _("Align:");
	tlabel[4].text_is_1byte = true;
	tlabel[4].text_in_resource = true;
	tgcd[4].gd.label = &tlabel[4];
	tgcd[4].creator = GLabelCreate;
	alarray[0] = &tgcd[4];

	tgcd[5].gd.flags = fit_to_path==NULL ? 0 : (gg_visible | gg_enabled | gg_cb_on);
	tlabel[5].text = (unichar_t *) _("At Start");
	tlabel[5].text_is_1byte = true;
	tlabel[5].text_in_resource = true;
	tgcd[5].gd.label = &tlabel[5];
	tgcd[5].gd.cid = CID_Start;
	tgcd[5].creator = GRadioCreate;
	alarray[1] = &tgcd[5];

	tgcd[6].gd.flags = fit_to_path==NULL ? 0 : (gg_visible | gg_enabled);
	tlabel[6].text = (unichar_t *) _("Centered");
	tlabel[6].text_is_1byte = true;
	tlabel[6].text_in_resource = true;
	tgcd[6].gd.label = &tlabel[6];
	tgcd[6].gd.cid = CID_Center;
	tgcd[6].creator = GRadioCreate;
	alarray[2] = &tgcd[6];

	tgcd[7].gd.flags = fit_to_path==NULL ? 0 : (gg_visible | gg_enabled);
	tlabel[7].text = (unichar_t *) _("At End");
	tlabel[7].text_is_1byte = true;
	tlabel[7].text_in_resource = true;
	tgcd[7].gd.label = &tlabel[7];
	tgcd[7].gd.cid = CID_End;
	tgcd[7].creator = GRadioCreate;
	alarray[3] = &tgcd[7];
	alarray[4] = GCD_Glue; alarray[5] = NULL;

	boxes[8].gd.flags = gg_enabled|gg_visible;
	boxes[8].gd.u.boxelements = alarray;
	boxes[8].creator = GHBoxCreate;
	tarray[8] = &boxes[8]; tarray[9] = NULL;

	tgcd[8].gd.flags = fit_to_path==NULL ? 0 : (gg_visible | gg_enabled);
	tlabel[8].text = (unichar_t *) _("Offset text from path by:");
	tlabel[8].text_is_1byte = true;
	tlabel[8].text_in_resource = true;
	tgcd[8].gd.label = &tlabel[8];
	tgcd[8].creator = GLabelCreate;
	oarray[0] = &tgcd[8];

	tgcd[9].gd.flags = fit_to_path==NULL ? 0 : (gg_visible | gg_enabled);
	tgcd[9].gd.pos.width = 60;
	tlabel[9].text = (unichar_t *) "0";
	tlabel[9].text_is_1byte = true;
	tlabel[9].text_in_resource = true;
	tgcd[9].gd.label = &tlabel[9];
	tgcd[9].gd.cid = CID_YOffset;
	tgcd[9].creator = GNumericFieldCreate;
	oarray[1] = &tgcd[9]; oarray[2] = GCD_Glue; oarray[3] = NULL;

	boxes[13].gd.flags = gg_enabled|gg_visible;
	boxes[13].gd.u.boxelements = oarray;
	boxes[13].creator = GHBoxCreate;
	tarray[10] = &boxes[13]; tarray[11] = NULL;

	tgcd[10].gd.flags = gg_visible | gg_enabled ;
	tgcd[10].creator = GLineCreate;
	tarray[12] = &tgcd[10]; tarray[13] = NULL;

	tgcd[11].gd.pos.width = -1; tgcd[11].gd.pos.height = 0;
	tgcd[11].gd.flags = gg_visible | gg_enabled | gg_but_default;
	tlabel[11].text = (unichar_t *) _("_Insert");
	tlabel[11].text_is_1byte = true;
	tlabel[11].text_in_resource = true;
	tgcd[11].gd.mnemonic = 'O';
	tgcd[11].gd.label = &tlabel[11];
	tgcd[11].gd.handle_controlevent = PRT_OK;
	tgcd[11].gd.cid = CID_OK;
	tgcd[11].creator = GButtonCreate;

	tgcd[12].gd.pos.width = -1; tgcd[12].gd.pos.height = 0;
	tgcd[12].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	tlabel[12].text = (unichar_t *) _("_Cancel");
	tlabel[12].text_is_1byte = true;
	tlabel[12].text_in_resource = true;
	tgcd[12].gd.label = &tlabel[12];
	tgcd[12].gd.cid = CID_Cancel;
	tgcd[12].gd.handle_controlevent = DSP_Done;
	tgcd[12].creator = GButtonCreate;
	barray2[0] = GCD_Glue; barray2[1] = &tgcd[11]; barray2[2] = GCD_Glue;
	barray2[3] = GCD_Glue; barray2[4] = &tgcd[12]; barray2[5] = GCD_Glue; barray2[6] = NULL;

	boxes[11].gd.flags = gg_enabled|gg_visible;
	boxes[11].gd.u.boxelements = barray2;
	boxes[11].creator = GHBoxCreate;
	tarray[14] = &boxes[11]; tarray[15] = NULL; tarray[16] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = tarray;
	boxes[0].gd.cid = CID_TopBox;
	boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(active->gw,boxes);
    GListSetSBAlwaysVisible(gcd[11].ret,true);
    GListSetPopupCallback(gcd[11].ret,MV_FriendlyFeatures);

    GTextInfoListFree(gcd[0].gd.u.list);
    DSP_SetFont(active,true);
	active->script_unknown = true;
	if ( old_bind_text ) {
	    SFTextAreaReplace(gcd[13].ret,old_bind_text);
	    DSP_TextChanged(gcd[13].ret,NULL);
	}
    SFTFRegisterCallback(gcd[13].ret,active,DSP_ChangeFontCallback);

    GHVBoxSetExpandableRow(boxes[0].ret,0);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxSetExpandableRow(boxes[4].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[4].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[5].ret,gb_expandglue);
    GHVBoxSetExpandableRow(boxes[6].ret,1);
    GHVBoxSetExpandableCol(boxes[12].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[11].ret,gb_expandglue);
    GHVBoxSetExpandableRow(vbox.ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[8].ret,gb_expandglue);
    GHVBoxSetExpandableRow(boxes[9].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[13].ret,gb_expandglue);
    GHVBoxFitWindow(boxes[0].ret);

    SFTextAreaSelect(gcd[13].ret,0,0);
    SFTextAreaShow(gcd[13].ret,0);
    SFTFProvokeCallback(gcd[13].ret);
    GWidgetIndicateFocusGadget(gcd[13].ret);

    GDrawSetVisible(active->gw,true);
    active->ready = true;
    while ( !done )
	GDrawProcessOneEvent(NULL);
}

static SplineSet *OnePathSelected(CharView *cv) {
    RefChar *rf;
    SplineSet *ss, *sel=NULL;
    SplinePoint *sp;

    for ( ss = cv->b.layerheads[cv->b.drawmode]->splines; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    if ( sp->selected ) {
		if ( sel==NULL )
		    sel = ss;
		else if ( sel!=ss )
return( NULL );
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
    if ( sel==NULL )
return( NULL );

    for ( rf = cv->b.layerheads[cv->b.drawmode]->refs; rf!=NULL; rf=rf->next ) {
	if ( rf->selected )
return( NULL );
    }
return( sel );
}

void InsertTextDlg(CharView *cv) {
    _PrintFFDlg(cv->b.sc,cv,OnePathSelected(cv));
}
