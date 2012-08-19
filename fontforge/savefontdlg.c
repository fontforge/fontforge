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
#include "fontforgeui.h"
#include <ustring.h>
#include <locale.h>
#include <gfile.h>
#include <gresource.h>
#include <utype.h>
#include <gio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <gicons.h>
#include <gkeysym.h>
#include "psfont.h"
#include "savefont.h"
#include <time.h>

int ask_user_for_resolution = true;
char *oflib_username = NULL;
char *oflib_password = NULL;
int old_fontlog=false;

static int nfnt_warned = false, post_warned = false;

#define CID_MergeTables	100
#define CID_TTC_CFF	101
#define CID_Family	2000

#define CID_OFLibUpload	300
#define CID_OFLibName	301
#define CID_OFLibTags	302
#define CID_OFLibDescription	303
#define CID_OFLibArtists	304
#define CID_OFLibNotSafe	305
#define CID_OFLibUsername	306
#define CID_OFLibPassword	307
#define CID_OFLibRememberMe	308
#define CID_OFLibOFL		309
#define CID_OFLibPD		310
#define CID_OFLibLine1		312
#define CID_OFLibLine2		313
#define CID_OFLibGenPreview	314
#define CID_OFLibNoPreview	315
#define CID_OFLibDiskPreview	316
#define CID_OFLibPreviewText	317
#define CID_OFLibPreviewBrowse	318
#define CID_UploadLicense	319
#define CID_UploadFontLog	320
#define CID_OFLibLabOffset	30
#define CID_AppendFontLog	400
#define CID_FontLogBit		401
#define CID_Layers		402
#define CID_PrependTimestamp    403

#define CID_OK		        1001
#define CID_PS_AFM		1002
#define CID_PS_PFM		1003
#define CID_PS_TFM		1004
#define CID_PS_HintSubs		1005
#define CID_PS_Flex		1006
#define CID_PS_Hints		1007
#define CID_PS_Restrict256	1008
#define CID_PS_Round		1009
#define CID_PS_OFM		1010
#define CID_PS_AFMmarks		1011
#define CID_TTF_Hints		1101
#define CID_TTF_FullPS		1102
#define CID_TTF_AppleMode	1103
#define CID_TTF_PfEdComments	1104
#define CID_TTF_PfEdColors	1105
#define CID_TTF_PfEd		1106
#define CID_TTF_TeXTable	1107
#define CID_TTF_OpenTypeMode	1108
#define CID_TTF_OldKern		1109
#define CID_TTF_GlyphMap	1110
#define CID_TTF_OFM		1111
/*#define CID_TTF_BrokenSize	1112*/
#define CID_TTF_PfEdLookups	1113
#define CID_TTF_PfEdGuides	1114
#define CID_TTF_PfEdLayers	1115
#define CID_FontLog		1116
#define CID_TTF_DummyDSIG	1117

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

struct gfc_data {
    int done;
    int sod_done;
    int sod_which;
    int sod_invoked;
    int ret;
    int family, familycnt;
    GWindow gw;
    GGadget *gfc;
    GGadget *pstype;
    GGadget *bmptype;
    GGadget *bmpsizes;
    GGadget *options;
    GGadget *rename;
    GGadget *validate;
    int ps_flags;		/* The ordering of these flags fields is */
    int sfnt_flags;		/*  important. We index into them */
  /* WAS otf_flags */
    int psotb_flags;		/*  don't reorder or put junk in between */
    uint8 optset[3];
    SplineFont *sf;
    EncMap *map;
    int layer;
#ifdef HAVE_PTHREAD_H
    uint8 please_die_thread;
    uint8 thread_active;
    pthread_t validate_thread;
#endif
};

static GTextInfo formattypes[] = {
    { (unichar_t *) N_("PS Type 1 (Ascii)"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("PS Type 1 (Binary)"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
#if __Mac
    { (unichar_t *) N_("PS Type 1 (Resource)"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
#else
    { (unichar_t *) N_("PS Type 1 (MacBin)"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
#endif
    { (unichar_t *) N_("PS Type 1 (Multiple)"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("PS Multiple Master(A)"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("PS Multiple Master(B)"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("PS Type 3"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("PS Type 0"), NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("PS CID"), NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
/* GT: "CFF (Bare)" means a CFF font without the normal OpenType wrapper */
/* GT: CFF is a font format that normally lives inside an OpenType font */
/* GT: but it is perfectly meaningful to remove all the OpenType complexity */
/* GT: and just leave a bare CFF font */
    { (unichar_t *) N_("CFF (Bare)"), NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("CFF CID (Bare)"), NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Type42"), NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Type11 (CID 2)"), NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("TrueType"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("TrueType (Symbol)"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
#if __Mac
    { (unichar_t *) N_("TrueType (Resource)"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
#else
    { (unichar_t *) N_("TrueType (MacBin)"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
#endif
    { (unichar_t *) N_("TrueType (TTC)"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("TrueType (Mac dfont)"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("OpenType (CFF)"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("OpenType (Mac dfont)"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("OpenType CID"), NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("OpenType CID (dfont)"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("SVG font"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Unified Font Object"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Web Open Font"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("No Outline Font"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};
static GTextInfo bitmaptypes[] = {
    { (unichar_t *) N_("BDF"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("In TTF/OTF"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Apple bitmap only sfnt (dfont)"), NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("(faked) MS bitmap only sfnt (ttf)"), NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("X11 bitmap only sfnt (otb)"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
#if __Mac
    { (unichar_t *) N_("NFNT (Resource)"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
#else
    { (unichar_t *) N_("NFNT (MacBin)"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
#endif
/* OS/X doesn't seem to support NFNTs, so there's no point in putting them in a dfont */
/*  { (unichar_t *) "NFNT (dfont)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },*/
    { (unichar_t *) N_("Win FON"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Win FNT"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Palm OS Bitmap"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("PS Type3 Bitmap"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("No Bitmap Fonts"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};

int old_validate = true;

extern int old_sfnt_flags;
extern int old_ps_flags;
extern int old_psotb_flags;

extern int oldformatstate;
extern int oldbitmapstate;

static const char *pfaeditflag = "SplineFontDB:";

int32 *ParseBitmapSizes(GGadget *g,char *msg,int *err) {
    const unichar_t *val = _GGadgetGetTitle(g), *pt; unichar_t *end, *end2;
    int i;
    int32 *sizes;
    char oldloc[24];

    strcpy( oldloc,setlocale(LC_NUMERIC,NULL) );
    setlocale(LC_NUMERIC,"C");

    *err = false;
    end2 = NULL;
    for ( i=1, pt = val; (end = u_strchr(pt,',')) || (end2=u_strchr(pt,' ')); ++i ) {
	if ( end!=NULL && end2!=NULL ) {
	    if ( end2<end ) end = end2;
	} else if ( end2!=NULL )
	    end = end2;
	pt = end+1;
	end2 = NULL;
    }
    sizes = galloc((i+1)*sizeof(int32));

    for ( i=0, pt = val; *pt!='\0' ; ) {
	sizes[i]=rint(u_strtod(pt,&end));
	if ( msg!=_("Pixel List") )
	    /* No bit depth allowed */;
	else if ( *end!='@' )
	    sizes[i] |= 0x10000;
	else
	    sizes[i] |= (u_strtol(end+1,&end,10)<<16);
	if ( sizes[i]>0 ) ++i;
	if ( *end!=' ' && *end!=',' && *end!='\0' ) {
	    free(sizes);
	    GGadgetProtest8(msg);
	    *err = true;
    break;
	}
	while ( *end==' ' || *end==',' ) ++end;
	pt = end;
    }
    setlocale(LC_NUMERIC,oldloc);
    if ( *err )
return( NULL );
    sizes[i] = 0;
return( sizes );
}

static int OPT_PSHints(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GWindow gw = GGadgetGetWindow(g);
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetIsChecked(g)) {
	    int which = d->sod_which>1? d->sod_which-1 : d->sod_which;
	    int flags = (&d->ps_flags)[which];
	    /*GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_HintSubs),true);*/
	    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_Flex),true);
	    /*GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_HintSubs),!(flags&ps_flag_nohintsubs));*/
	    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_Flex),!(flags&ps_flag_noflex));
	} else {
	    /*GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_HintSubs),false);*/
	    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_Flex),false);
	    /*GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_HintSubs),false);*/
	    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_Flex),false);
	}
    }
return( true );
}

static int OPT_Applemode(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GWindow gw = GGadgetGetWindow(g);
	if ( GGadgetIsChecked(g))
	    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_OldKern),false);
    }
return( true );
}

static int OPT_OldKern(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GWindow gw = GGadgetGetWindow(g);
	if ( GGadgetIsChecked(g))
	    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_AppleMode),false);
    }
return( true );
}

static int sod_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct gfc_data *d = GDrawGetUserData(gw);
	d->sod_done = true;
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("generate.html#Options");
return( true );
	}
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(gw);
	if ( GGadgetGetCid(event->u.control.g)==CID_OK ) {
	    if ( d->sod_which==0 ) {		/* PostScript */
		d->ps_flags = 0;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_AFM)) )
		    d->ps_flags |= ps_flag_afm;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_AFMmarks)) )
		    d->ps_flags |= ps_flag_afmwithmarks;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_PFM)) )
		    d->ps_flags |= ps_flag_pfm;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_TFM)) )
		    d->ps_flags |= ps_flag_tfm;
		/*if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_HintSubs)) )*/
		    /*d->ps_flags |= ps_flag_nohintsubs;*/
		if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_Flex)) )
		    d->ps_flags |= ps_flag_noflex;
		if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_Hints)) )
		    d->ps_flags |= ps_flag_nohints;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_Round)) )
		    d->ps_flags |= ps_flag_round;
		/*if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_Restrict256)) )*/
		    /*d->ps_flags |= ps_flag_restrict256;*/
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_FontLog)) )
		    d->ps_flags |= ps_flag_outputfontlog;
	    } else if ( d->sod_which==1 || d->sod_which==2 ) {	/* Open/TrueType */
		d->sfnt_flags = 0;
		if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_Hints)) )
		    d->sfnt_flags |= ttf_flag_nohints;
		if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_FullPS)) )
		    d->sfnt_flags |= ttf_flag_shortps;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_AppleMode)) )
		    d->sfnt_flags |= ttf_flag_applemode;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_OpenTypeMode)) )
		    d->sfnt_flags |= ttf_flag_otmode;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_OldKern)) &&
			!(d->sfnt_flags&ttf_flag_applemode) )
		    d->sfnt_flags |= ttf_flag_oldkern;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_DummyDSIG)) )
		    d->sfnt_flags |= ttf_flag_dummyDSIG;
#if 0
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_BrokenSize)) )
		    d->sfnt_flags |= ttf_flag_brokensize;
#endif
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_PfEdComments)) )
		    d->sfnt_flags |= ttf_flag_pfed_comments;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_PfEdColors)) )
		    d->sfnt_flags |= ttf_flag_pfed_colors;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_PfEdLookups)) )
		    d->sfnt_flags |= ttf_flag_pfed_lookupnames;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_PfEdGuides)) )
		    d->sfnt_flags |= ttf_flag_pfed_guides;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_PfEdLayers)) )
		    d->sfnt_flags |= ttf_flag_pfed_layers;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_TeXTable)) )
		    d->sfnt_flags |= ttf_flag_TeXtable;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_GlyphMap)) )
		    d->sfnt_flags |= ttf_flag_glyphmap;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_OFM)) )
		    d->sfnt_flags |= ttf_flag_ofm;

		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_AFM)) )
		    d->sfnt_flags |= ps_flag_afm;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_AFMmarks)) )
		    d->sfnt_flags |= ps_flag_afmwithmarks;
		/*if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_HintSubs)) )*/
		    /*d->sfnt_flags |= ps_flag_nohintsubs;*/
		if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_Flex)) )
		    d->sfnt_flags |= ps_flag_noflex;
		if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_Hints)) )
		    d->sfnt_flags |= ps_flag_nohints;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_Round)) )
		    d->sfnt_flags |= ps_flag_round;

		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_FontLog)) )
		    d->sfnt_flags |= ps_flag_outputfontlog;
	    } else {				/* PS + OpenType Bitmap */
		d->ps_flags = d->psotb_flags = 0;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_AFMmarks)) )
		     d->psotb_flags = d->ps_flags |= ps_flag_afmwithmarks;
		/*if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_HintSubs)) )*/
		     /*d->psotb_flags = d->ps_flags |= ps_flag_nohintsubs;*/
		if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_Flex)) )
		     d->psotb_flags = d->ps_flags |= ps_flag_noflex;
		if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_Hints)) )
		     d->psotb_flags = d->ps_flags |= ps_flag_nohints;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_Round)) )
		     d->psotb_flags = d->ps_flags |= ps_flag_round;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_PFM)) )
		     d->psotb_flags = d->ps_flags |= ps_flag_pfm;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_TFM)) )
		     d->psotb_flags = d->ps_flags |= ps_flag_tfm;

		if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_FullPS)) )
		    d->psotb_flags |= ttf_flag_shortps;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_PfEdComments)) )
		    d->psotb_flags |= ttf_flag_pfed_comments;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_PfEdColors)) )
		    d->psotb_flags |= ttf_flag_pfed_colors;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_PfEdLookups)) )
		    d->psotb_flags |= ttf_flag_pfed_lookupnames;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_PfEdGuides)) )
		    d->psotb_flags |= ttf_flag_pfed_guides;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_PfEdLayers)) )
		    d->psotb_flags |= ttf_flag_pfed_layers;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_TeXTable)) )
		    d->psotb_flags |= ttf_flag_TeXtable;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_GlyphMap)) )
		    d->psotb_flags |= ttf_flag_glyphmap;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_OFM)) )
		    d->psotb_flags |= ttf_flag_ofm;
	    }
	    d->sod_invoked = true;
	}
	d->sod_done = true;
    }
return( true );
}

static void OptSetDefaults(GWindow gw,struct gfc_data *d,int which,int iscid) {
    int w = which>1 ? which-1 : which;
    int flags = (&d->ps_flags)[w];
    int fs = GGadgetGetFirstListSelectedItem(d->pstype);
    int bf = GGadgetGetFirstListSelectedItem(d->bmptype);
    /* which==0 => pure postscript */
    /* which==1 => truetype or bitmap in sfnt wrapper */
    /* which==2 => opentype (ps) */
    /* which==3 => generating an sfnt based bitmap only font with a postscript outline font */

    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_Hints),!(flags&ps_flag_nohints));
    /*GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_HintSubs),!(flags&ps_flag_nohintsubs));*/
    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_Flex),!(flags&ps_flag_noflex));
    /*GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_Restrict256),flags&ps_flag_restrict256);*/
    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_Round),flags&ps_flag_round);

    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_AFM),flags&ps_flag_afm);
    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_AFMmarks),flags&ps_flag_afmwithmarks);
    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_PFM),(flags&ps_flag_pfm) && !iscid);
    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_TFM),flags&ps_flag_tfm);

    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_Hints),!(flags&ttf_flag_nohints));
    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_FullPS),!(flags&ttf_flag_shortps));
    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_OFM),flags&ttf_flag_ofm);
    if ( d->optset[which] )
	GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_AppleMode),flags&ttf_flag_applemode);
    else if ( which==0 || which==3 )	/* PostScript */
	GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_AppleMode),false);
    else
	GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_AppleMode),(flags&ttf_flag_applemode));
    if ( d->optset[which] )
	GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_OpenTypeMode),flags&ttf_flag_otmode);
    else if ( which==0 )	/* PostScript */
	GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_OpenTypeMode),false);
    else if ( (fs==ff_ttfmacbin || fs==ff_ttfdfont || fs==ff_otfdfont ||
	     fs==ff_otfciddfont || d->family==gf_macfamily || (fs==ff_none && bf==bf_sfnt_dfont)))
	GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_OpenTypeMode),false);
    else
	GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_OpenTypeMode),(flags&ttf_flag_otmode));

    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_PfEdComments),flags&ttf_flag_pfed_comments);
    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_PfEdColors),flags&ttf_flag_pfed_colors);
    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_PfEdLookups),flags&ttf_flag_pfed_lookupnames);
    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_PfEdGuides),flags&ttf_flag_pfed_guides);
    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_PfEdLayers),flags&ttf_flag_pfed_layers);
    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_TeXTable),flags&ttf_flag_TeXtable);
    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_GlyphMap),flags&ttf_flag_glyphmap);
#if 0 
    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_BrokenSize),flags&ttf_flag_brokensize);
#endif
    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_OldKern),
	    (flags&ttf_flag_oldkern) && !GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_AppleMode)));
    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_DummyDSIG),flags&ttf_flag_dummyDSIG);
    GGadgetSetChecked(GWidgetGetControl(gw,CID_FontLog),flags&ps_flag_outputfontlog);

    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_Hints),which!=1);
    /*GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_HintSubs),which!=1);*/
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_Flex),which!=1);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_Round),which!=1);
    if ( which!=1 && (flags&ps_flag_nohints)) {
	/*GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_HintSubs),false);*/
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_Flex),false);
	/*GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_HintSubs),false);*/
	GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_Flex),false);
    }
    /*GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_Restrict256),(which==0 || which==3) && !iscid);*/

    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_AFM),which!=1);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_AFMmarks),which!=1);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_PFM),which==0 || which==3);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_TFM),which==0 || which==3);

    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_Hints),which==1);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_FullPS),which!=0);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_AppleMode),which!=0 && which!=3);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_OpenTypeMode),which!=0 && which!=3);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_OldKern),which!=0 );
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_DummyDSIG),which!=0 );
#if 0
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_BrokenSize),which!=0 );
#endif

    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_PfEd),which!=0);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_PfEdComments),which!=0);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_PfEdColors),which!=0);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_PfEdLookups),which!=0);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_PfEdGuides),which!=0);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_PfEdColors),which!=0);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_PfEdLayers),which!=0);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_TeXTable),which!=0);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_GlyphMap),which!=0);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_OFM),which!=0);

    d->optset[which] = true;
}

#define OPT_Width	230
#define OPT_Height	233

static void SaveOptionsDlg(struct gfc_data *d,int which,int iscid) {
    int k,fontlog_k,group,group2;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[32];
    GTextInfo label[32];
    GRect pos;
    GGadgetCreateData *hvarray1[21], *hvarray2[42], *harray[7], *varray[11];
    GGadgetCreateData boxes[5];

    d->sod_done = false;
    d->sod_which = which;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Options");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,OPT_Width));
    pos.height = GDrawPointsToPixels(NULL,OPT_Height);
    gw = GDrawCreateTopWindow(NULL,&pos,sod_e_h,d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(boxes,0,sizeof(boxes));

    k = 0;
    gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width-4; gcd[k].gd.pos.height = pos.height-4;
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[k++].creator = GGroupCreate;

    label[k].text = (unichar_t *) U_("PostScript®");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 8; gcd[k].gd.pos.y = 5;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;

    group = k;
    gcd[k].gd.pos.x = 4; gcd[k].gd.pos.y = 9;
    gcd[k].gd.pos.width = OPT_Width-8; gcd[k].gd.pos.height = 72;
    gcd[k].gd.flags = gg_enabled | gg_visible ;
    gcd[k++].creator = GGroupCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = 16;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Round");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _("Do you want to round coordinates to integers (this saves space)?");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_PS_Round;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray1[0] = &gcd[k-1]; hvarray1[1] = GCD_ColSpan;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Hints");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _("Do you want the font file to contain PostScript hints?");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = OPT_PSHints;
    gcd[k].gd.cid = CID_PS_Hints;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray1[5] = &gcd[k-1]; hvarray1[6] = GCD_ColSpan;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+4; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Flex Hints");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _("Do you want the font file to contain PostScript flex hints?");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_PS_Flex;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray1[10] = GCD_HPad10; hvarray1[11] = &gcd[k-1];
    hvarray1[15] = GCD_Glue; hvarray1[16] = GCD_Glue;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_utf8_popup ;
    label[k].text = (unichar_t *) _("Hint Substitution");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _("Do you want the font file to do hint substitution?");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_PS_HintSubs;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_utf8_popup ;
    label[k].text = (unichar_t *) _("First 256");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _("Limit the font so that only the glyphs referenced in the first 256 encodings\nwill be included in the file");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_PS_Restrict256;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = 110; gcd[k].gd.pos.y = gcd[k-5].gd.pos.y;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Output AFM");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) U_("The AFM file contains metrics information that many word-processors will read when using a PostScript® font.");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_PS_AFM;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray1[2] = &gcd[k-1]; hvarray1[3] = GCD_ColSpan; hvarray1[4] = NULL;

    gcd[k].gd.pos.x = 112; gcd[k].gd.pos.y = gcd[k-5].gd.pos.y;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Composites in AFM");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) U_("The AFM format allows some information about composites\n(roughly the same as mark to base anchor classes) to be\nincluded. However it tends to make AFM files huge as it\nis not stored in an efficient manner.");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_PS_AFMmarks;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray1[7] = GCD_HPad10; hvarray1[8] = &gcd[k-1]; hvarray1[9] = NULL;

    gcd[k].gd.pos.x = gcd[k-2].gd.pos.x; gcd[k].gd.pos.y = gcd[k-5].gd.pos.y;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Output PFM");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) U_("The PFM file contains information Windows needs to install a PostScript® font.");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_PS_PFM;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray1[12] = &gcd[k-1]; hvarray1[13] = GCD_ColSpan; hvarray1[14] = NULL;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible |gg_utf8_popup;
    label[k].text = (unichar_t *) _("Output TFM & ENC");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) U_("The tfm and enc files contain information TeX needs to install a PostScript® font.");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_PS_TFM;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray1[17] = &gcd[k-1]; hvarray1[18] = GCD_ColSpan; hvarray1[19] = NULL;
    hvarray1[20] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = hvarray1;
    boxes[2].gd.label = (GTextInfo *) &gcd[1];
    boxes[2].creator = GHVGroupCreate;


    label[k].text = (unichar_t *) _("SFNT");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 8; gcd[k].gd.pos.y = gcd[group].gd.pos.y+gcd[group].gd.pos.height+6;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;

    group2 = k;
    gcd[k].gd.pos.x = 4; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+4;
    gcd[k].gd.pos.width = OPT_Width-8; gcd[k].gd.pos.height = 100;
    gcd[k].gd.flags = gg_enabled | gg_visible ;
    gcd[k++].creator = GGroupCreate;

    gcd[k].gd.pos.x = gcd[group+1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+8;
    gcd[k].gd.flags = gg_visible |gg_utf8_popup;
    label[k].text = (unichar_t *) _("TrueType Hints");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _("Do you want the font file to contain truetype hints? This will not\ngenerate new instructions, it will just make use of whatever is associated\nwith each character.");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_Hints;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray2[0] = &gcd[k-1]; hvarray2[1] = GCD_ColSpan;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("PS Glyph Names");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _("Do you want the font file to contain the names of each glyph in the font?");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_FullPS;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray2[5] = &gcd[k-1]; hvarray2[6] = GCD_ColSpan;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Apple");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _("Apple and MS/Adobe differ about the format of truetype and opentype files\nThis allows you to select which standard to follow for your font.\nThe main differences are:\n The requirements for the 'postscript' name in the name table conflict\n Bitmap data are stored in different tables\n Scaled composite characters are treated differently\n Use of GSUB rather than morx(t)/feat\n Use of GPOS rather than kern/opbd\n Use of GDEF rather than lcar/prop");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_AppleMode;
    gcd[k].gd.handle_controlevent = OPT_Applemode;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray2[10] = &gcd[k-1]; hvarray2[11] = GCD_ColSpan;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("OpenType");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _("Apple and MS/Adobe differ about the format of truetype and opentype files\nThis allows you to select which standard to follow for your font.\nThe main differences are:\n The requirements for the 'postscript' name in the name table conflict\n Bitmap data are stored in different tables\n Scaled composite glyphs are treated differently\n Use of GSUB rather than morx(t)/feat\n Use of GPOS rather than kern/opbd\n Use of GDEF rather than lcar/prop");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_OpenTypeMode;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray2[15] = &gcd[k-1]; hvarray2[16] = GCD_ColSpan;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+4; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Old style 'kern'");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _("Many applications still don't support 'GPOS' kerning.\nIf you want to include both 'GPOS' and old-style 'kern'\ntables set this check box.\nIt may not be set in conjunction with the Apple checkbox.\nThis may confuse other applications though.");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_OldKern;
    gcd[k].gd.handle_controlevent = OPT_OldKern;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray2[20] = GCD_HPad10; hvarray2[21] = &gcd[k-1];

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+4; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Dummy 'DSIG'");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _(
	"MS uses the presence of a 'DSIG' table to determine whether to use an OpenType\n"
	"icon for the tt font. FontForge can't generate a useful 'DSIG' table, but it can\n"
	"generate an empty one with no signature info. A pointless table.");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_DummyDSIG;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray2[25] = GCD_HPad10; hvarray2[26] = &gcd[k-1];

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Output Glyph Map");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _("When generating a truetype or opentype font it is occasionally\nuseful to know the mapping between truetype glyph ids and\nglyph names. Setting this option will cause FontForge to\nproduce a file (with extension .g2n) containing those data.");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_GlyphMap;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray2[30] = &gcd[k-1]; hvarray2[31] = GCD_ColSpan;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Output OFM & CFG");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _("The ofm and cfg files contain information Omega needs to process a font.");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_OFM;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray2[35] = &gcd[k-1]; hvarray2[36] = GCD_ColSpan;

    gcd[k].gd.pos.x = gcd[group+6].gd.pos.x; gcd[k].gd.pos.y = gcd[k-5].gd.pos.y;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("PfaEdit Table");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _("The PfaEdit table is an extension to the TrueType format\nand contains various data used by FontForge\n(It should be called the FontForge table,\nbut isn't for historical reasons)");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_PfEd;
    gcd[k++].creator = GLabelCreate;
    hvarray2[2] = &gcd[k-1]; hvarray2[3] = GCD_ColSpan; hvarray2[4] = NULL;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+2; gcd[k].gd.pos.y = gcd[k-5].gd.pos.y-4;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Save Comments");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _("Save glyph comments in the PfEd table");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_PfEdComments;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray2[7] = GCD_HPad10; hvarray2[8] = &gcd[k-1]; hvarray2[9] = NULL;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-5].gd.pos.y-4;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Save Colors");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _("Save glyph colors in the PfEd table");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_PfEdColors;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray2[12] = GCD_HPad10; hvarray2[13] = &gcd[k-1]; hvarray2[14] = NULL;

    gcd[k].gd.pos.x = gcd[k-3].gd.pos.x; gcd[k].gd.pos.y = gcd[k-5].gd.pos.y;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Lookup Names");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _("Preserve the names of the GPOS/GSUB lookups and subtables");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_PfEdLookups;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray2[17] = GCD_HPad10; hvarray2[18] = &gcd[k-1]; hvarray2[19] = NULL;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Save Guides");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _("Save the guidelines in the Guide layer.");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_PfEdGuides;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray2[22] = GCD_HPad10; hvarray2[23] = &gcd[k-1]; hvarray2[24] = NULL;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Save Layers");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _(
	    "Preserve any background and spiro layers.\n"
	    "Also if we output a truetype font from a\n"
	    "cubic database, save the cubic splines.");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_PfEdLayers;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray2[27] = GCD_HPad10; hvarray2[28] = &gcd[k-1]; hvarray2[29] = NULL;

    gcd[k].gd.pos.x = gcd[k-3].gd.pos.x; gcd[k].gd.pos.y = gcd[k-5].gd.pos.y;
    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
    label[k].text = (unichar_t *) _("TeX Table");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _("The TeX table is an extension to the TrueType format\nand the various data you would expect to find in\na tfm file (that isn't already stored elsewhere\nin the ttf file)\n");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_TeXTable;
    gcd[k++].creator = GCheckBoxCreate;
    hvarray2[32] = &gcd[k-1]; hvarray2[33] = GCD_ColSpan; hvarray2[34] = NULL;
    hvarray2[37] = GCD_Glue; hvarray2[38] = GCD_Glue; hvarray2[39] = NULL;
    hvarray2[40] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = hvarray2;
    boxes[3].gd.label = (GTextInfo *) &gcd[group2-1];
    boxes[3].creator = GHVGroupCreate;

    fontlog_k = k;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    label[k].text = (unichar_t *) _("Output Font Log");
    label[k].text_is_1byte = true;
    gcd[k].gd.popup_msg = (unichar_t *) _(
	"The FONTLOG is a text file containing pertinent information\n"
	"about the font including such things as its change history.\n"
	"The SIL Open Font License highly recommends its use.\n\n"
	"If your font contains a FONTLOG (see the Element->Font Info)\n"
	"and you check this box, then the internal FONTLOG will be\n"
	"written to the file \"FONTLOG.txt\" in the same directory\n"
	"as the font itself.");
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_FontLog;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = gcd[group2].gd.pos.y+gcd[group2].gd.pos.height+10-3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_OK;
    gcd[k++].creator = GButtonCreate;
    harray[0] = GCD_Glue; harray[1] = &gcd[k-1]; harray[2] = GCD_Glue;

    gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k++].creator = GButtonCreate;
    harray[3] = GCD_Glue; harray[4] = &gcd[k-1]; harray[5] = GCD_Glue;
    harray[6] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = harray;
    boxes[4].creator = GHBoxCreate;

    varray[0] = &boxes[2]; varray[1] = NULL;
    varray[2] = &boxes[3]; varray[3] = NULL;
    varray[4] = &gcd[fontlog_k];; varray[5] = NULL;
    varray[6] = GCD_Glue; varray[7] = NULL;
    varray[8] = &boxes[4]; varray[9] = NULL;
    varray[10] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[4].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);

    OptSetDefaults(gw,d,which,iscid);

    GDrawSetVisible(gw,true);
    while ( !d->sod_done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

static int br_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	int *done = GDrawGetUserData(gw);
	*done = -1;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	int *done = GDrawGetUserData(gw);
	if ( GGadgetGetCid(event->u.control.g)==1001 )
	    *done = true;
	else
	    *done = -1;
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_textchanged &&
	    GGadgetGetCid(event->u.control.g)==1003 ) {
	GGadgetSetChecked(GWidgetGetControl(gw,1004),true);
    }
return( true );
}

static int AskResolution(int bf,BDFFont *bdf) {
    GRect pos;
    static GWindow bdf_gw, fon_gw;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[10], *varray[22], *harray[7], *harray2[4], boxes[4];
    GTextInfo label[10];
    int done=-3;
    int def_res = -1;
    char buf[20];

    if ( bdf!=NULL )
	def_res = bdf->res;

    if ( no_windowing_ui )
return( -1 );

    gw = bf==bf_bdf ? bdf_gw : fon_gw;
    if ( gw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = _("BDF Resolution");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,150));
	pos.height = GDrawPointsToPixels(NULL,130);
	gw = GDrawCreateTopWindow(NULL,&pos,br_e_h,&done,&wattrs);
	if ( bf==bf_bdf )
	    bdf_gw = gw;
	else
	    fon_gw = gw;

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));
	memset(&boxes,0,sizeof(boxes));

	label[0].text = (unichar_t *) _("BDF Resolution");
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 7; 
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].creator = GLabelCreate;
	harray2[0] = GCD_Glue; harray2[1] = &gcd[0]; harray2[2] = GCD_Glue; harray2[3] = NULL;

	boxes[3].gd.flags = gg_enabled|gg_visible;
	boxes[3].gd.u.boxelements = harray2;
	boxes[3].creator = GHBoxCreate;
	varray[0] = &boxes[3]; varray[1] = GCD_ColSpan; varray[2] = NULL;

	label[1].text = (unichar_t *) (bf==bf_bdf ? "75" : "96");
	label[1].text_is_1byte = true;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.mnemonic = (bf==bf_bdf ? '7' : '9');
	gcd[1].gd.pos.x = 20; gcd[1].gd.pos.y = 13+7;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	if ( (bf==bf_bdf ? 75 : 96)==def_res )
	    gcd[1].gd.flags |= gg_cb_on;
	gcd[1].gd.cid = 75;
	gcd[1].creator = GRadioCreate;
	varray[3] = &gcd[1]; varray[4] = GCD_Glue; varray[5] = NULL;

	label[2].text = (unichar_t *) (bf==bf_bdf ? "100" : "120");
	label[2].text_is_1byte = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.mnemonic = '1';
	gcd[2].gd.pos.x = 20; gcd[2].gd.pos.y = gcd[1].gd.pos.y+17; 
	gcd[2].gd.flags = gg_enabled|gg_visible;
	if ( (bf==bf_bdf ? 100 : 120)==def_res )
	    gcd[2].gd.flags |= gg_cb_on;
	gcd[2].gd.cid = 100;
	gcd[2].creator = GRadioCreate;
	varray[6] = &gcd[2]; varray[7] = GCD_Glue; varray[8] = NULL;

	label[3].text = (unichar_t *) _("_Guess");
	label[3].text_is_1byte = true;
	label[3].text_in_resource = true;
	gcd[3].gd.label = &label[3];
	gcd[3].gd.pos.x = 20; gcd[3].gd.pos.y = gcd[2].gd.pos.y+17;
	gcd[3].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	if ( !((gcd[1].gd.flags|gcd[2].gd.flags)&gg_cb_on) )
	    gcd[3].gd.flags |= gg_cb_on;
	gcd[3].gd.cid = -1;
	gcd[3].gd.popup_msg = (unichar_t *) _("Guess each font's resolution based on its pixel size");
	gcd[3].creator = GRadioCreate;
	varray[9] = &gcd[3]; varray[10] = GCD_Glue; varray[11] = NULL;

	label[4].text = (unichar_t *) _("_Other");
	label[4].text_is_1byte = true;
	label[4].text_in_resource = true;
	gcd[4].gd.label = &label[4];
	gcd[4].gd.pos.x = 20; gcd[4].gd.pos.y = gcd[3].gd.pos.y+17;
	gcd[4].gd.flags = gg_enabled|gg_visible;
	gcd[4].gd.cid = 1004;
	gcd[4].creator = GRadioCreate;

	label[5].text = (unichar_t *) (bf == bf_bdf ? "96" : "72");
	if ( def_res>0 ) {
	    sprintf( buf, "%d", def_res );
	    label[5].text = (unichar_t *) buf;
	}
	label[5].text_is_1byte = true;
	gcd[5].gd.label = &label[5];
	gcd[5].gd.pos.x = 70; gcd[5].gd.pos.y = gcd[4].gd.pos.y-3; gcd[5].gd.pos.width = 40;
	gcd[5].gd.flags = gg_enabled|gg_visible;
	gcd[5].gd.cid = 1003;
	gcd[5].creator = GTextFieldCreate;
	varray[12] = &gcd[4]; varray[13] = &gcd[5]; varray[14] = NULL;
	varray[15] = GCD_Glue; varray[16] = GCD_Glue; varray[17] = NULL;

	gcd[6].gd.pos.x = 15-3; gcd[6].gd.pos.y = gcd[4].gd.pos.y+24;
	gcd[6].gd.pos.width = -1; gcd[6].gd.pos.height = 0;
	gcd[6].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[6].text = (unichar_t *) _("_OK");
	label[6].text_is_1byte = true;
	label[6].text_in_resource = true;
	gcd[6].gd.mnemonic = 'O';
	gcd[6].gd.label = &label[6];
	gcd[6].gd.cid = 1001;
	gcd[6].creator = GButtonCreate;
	harray[0] = GCD_Glue; harray[1] = &gcd[6]; harray[2] = GCD_Glue;

	gcd[7].gd.pos.x = -15; gcd[7].gd.pos.y = gcd[6].gd.pos.y+3;
	gcd[7].gd.pos.width = -1; gcd[7].gd.pos.height = 0;
	gcd[7].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[7].text = (unichar_t *) _("_Cancel");
	label[7].text_is_1byte = true;
	label[7].text_in_resource = true;
	gcd[7].gd.label = &label[7];
	gcd[7].gd.mnemonic = 'C';
	/*gcd[7].gd.handle_controlevent = CH_Cancel;*/
	gcd[7].gd.cid = 1002;
	gcd[7].creator = GButtonCreate;
	harray[3] = GCD_Glue; harray[4] = &gcd[7]; harray[5] = GCD_Glue;
	harray[6] = NULL;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = harray;
	boxes[2].creator = GHBoxCreate;
	varray[18] = &boxes[2]; varray[19] = GCD_ColSpan; varray[20] = NULL;
	varray[21] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = varray;
	boxes[0].creator = GHVGroupCreate;

	gcd[8].gd.pos.x = 2; gcd[8].gd.pos.y = 2;
	gcd[8].gd.pos.width = pos.width-4; gcd[8].gd.pos.height = pos.height-2;
	gcd[8].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
	gcd[8].creator = GGroupCreate;

	GGadgetsCreate(gw,boxes);
	GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    } else
	GDrawSetUserData(gw,&done);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( true ) {
	done = 0;
	while ( !done )
	    GDrawProcessOneEvent(NULL);
	if ( GGadgetIsChecked(GWidgetGetControl(gw,1004)) ) {
	    int err = false;
	    int res = GetInt8(gw,1003,_("Other ..."),&err);
	    if ( res<10 )
		GGadgetProtest8( _("_Other"));
	    else if ( !err ) {
		GDrawSetVisible(gw,false);
return( res );
	    }
    continue;
	}
	GDrawSetVisible(gw,false);
	if ( done==-1 )
return( -2 );
	if ( GGadgetIsChecked(GWidgetGetControl(gw,75)) )
return( bf == bf_bdf ? 75 : 96 );
	if ( GGadgetIsChecked(GWidgetGetControl(gw,100)) )
return( bf == bf_bdf ? 100 : 120 );
	/*if ( GGadgetIsChecked(GWidgetGetControl(gw,-1)) )*/
return( -1 );
    }
}

static char *SearchDirForWernerFile(char *dir,char *filename) {
    char buffer[1025], buf2[200];
    FILE *file;
    int good = false;

    if ( dir==NULL )
return( NULL );

    strcpy(buffer,dir);
    strcat(buffer,"/");
    strcat(buffer,filename);
    file = fopen(buffer,"r");
    if ( file==NULL )
return( NULL );
    if ( fgets(buf2,sizeof(buf2),file)!=NULL &&
	    strncmp(buf2,pfaeditflag,strlen(pfaeditflag))==0 )
	good = true;
    fclose( file );
    if ( good )
return( copy(buffer));

return( NULL );
}

static char *SearchNoLibsDirForWernerFile(char *dir,char *filename) {
    char *ret;

    if ( dir==NULL || strstr(dir,"/.libs")==NULL )
return( NULL );

    dir = copy(dir);
    *strstr(dir,"/.libs") = '\0';

    ret = SearchDirForWernerFile(dir,filename);
    free(dir);
return( ret );
}

static enum fchooserret GFileChooserFilterWernerSFDs(GGadget *g,GDirEntry *ent,
	const unichar_t *dir) {
    enum fchooserret ret = GFileChooserDefFilter(g,ent,dir);
    char buf2[200];
    FILE *file;

    if ( ret==fc_show && !ent->isdir ) {
	char *filename = galloc(u_strlen(dir)+u_strlen(ent->name)+5);
	cu_strcpy(filename,dir);
	strcat(filename,"/");
	cu_strcat(filename,ent->name);
	file = fopen(filename,"r");
	if ( file==NULL )
	    ret = fc_hide;
	else {
	    if ( fgets(buf2,sizeof(buf2),file)==NULL ||
		    strncmp(buf2,pfaeditflag,strlen(pfaeditflag))==0 )
		ret = fc_hide;
	    fclose(file);
	}
	free(filename);
    }
return( ret );
}

static char *GetWernerSFDFile(SplineFont *sf,EncMap *map) {
    char *def=NULL, *ret;
    char buffer[100];
    int supl = sf->supplement;

    for ( supl = sf->supplement; supl<sf->supplement+10 ; ++supl ) {
	if ( no_windowing_ui ) {
	    if ( sf->subfontcnt!=0 ) {
		sprintf(buffer,"%.40s-%.40s-%d.sfd", sf->cidregistry,sf->ordering,supl);
		def = buffer;
	    } else if ( strstrmatch(map->enc->enc_name,"big")!=NULL &&
		    strchr(map->enc->enc_name,'5')!=NULL ) {
		if (strstrmatch(map->enc->enc_name,"hkscs")!=NULL )
		    def = "Big5HKSCS.sfd";
		else
		    def = "Big5.sfd";
	    } else if ( strstrmatch(map->enc->enc_name,"sjis")!=NULL )
		def = "Sjis.sfd";
	    else if ( strstrmatch(map->enc->enc_name,"Wansung")!=NULL ||
		    strstrmatch(map->enc->enc_name,"EUC-KR")!=NULL )
		def = "Wansung.sfd";
	    else if ( strstrmatch(map->enc->enc_name,"johab")!=NULL )
		def = "Johab.sfd";
	    else if ( map->enc->is_unicodebmp || map->enc->is_unicodefull )
		def = "Unicode.sfd";
	}
	if ( def!=NULL ) {
	    ret = SearchDirForWernerFile(".",def);
	    if ( ret==NULL )
		ret = SearchDirForWernerFile(GResourceProgramDir,def);
	    if ( ret==NULL )
		ret = SearchDirForWernerFile(getFontForgeShareDir(),def);
	    if ( ret==NULL )
		ret = SearchNoLibsDirForWernerFile(GResourceProgramDir,def);
	    if ( ret!=NULL )
return( ret );
	}
	if ( sf->subfontcnt!=0 )
    break;
    }

    if ( no_windowing_ui )
return( NULL );

    /*if ( def==NULL )*/
	def = "*.sfd";
    ret = gwwv_open_filename(_("Find Sub Font Definition file"),NULL,"*.sfd",GFileChooserFilterWernerSFDs);
return( ret );
}

static int AnyRefs(RefChar *refs) {
    int j;
    while ( refs!=NULL ) {
	for ( j=0; j<refs->layer_cnt; ++j )
	    if ( refs->layers[j].splines!=NULL )
return( true );
	refs = refs->next;
    }
return( false );
}

static int OFLibUploadGather(struct gfc_data *d,unichar_t *path) {
    OFLibData oflib;
    int ret;
    char *pt;
    int gendefaultimage=GGadgetIsChecked(GWidgetGetControl(d->gw,CID_OFLibGenPreview));

    memset(&oflib,0,sizeof(oflib));
    oflib.sf = d->sf;
    oflib.pathspec = u2utf8_copy(path);
    oflib.username = GGadgetGetTitle8(GWidgetGetControl(d->gw,CID_OFLibUsername));
    oflib.password = GGadgetGetTitle8(GWidgetGetControl(d->gw,CID_OFLibPassword));
    oflib.name     = GGadgetGetTitle8(GWidgetGetControl(d->gw,CID_OFLibName));
    oflib.description = GGadgetGetTitle8(GWidgetGetControl(d->gw,CID_OFLibDescription));
    oflib.tags     = GGadgetGetTitle8(GWidgetGetControl(d->gw,CID_OFLibTags));
    oflib.artists  = GGadgetGetTitle8(GWidgetGetControl(d->gw,CID_OFLibArtists));
    oflib.notsafeforwork = GGadgetIsChecked(GWidgetGetControl(d->gw,CID_OFLibNotSafe));
    oflib.oflicense = GGadgetIsChecked(GWidgetGetControl(d->gw,CID_OFLibOFL));
    oflib.upload_license = GGadgetIsChecked(GWidgetGetControl(d->gw,CID_UploadLicense));
    oflib.upload_fontlog = GGadgetIsChecked(GWidgetGetControl(d->gw,CID_UploadFontLog));
    if ( gendefaultimage )
	oflib.previewimage = SFDefaultImage(d->sf,NULL);
    else if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_OFLibNoPreview)) )
	oflib.previewimage = NULL;
    else {
	oflib.previewimage = GGadgetGetTitle8(GWidgetGetControl(d->gw,CID_OFLibPreviewText));
	if ( *oflib.previewimage=='\0' ) {
	    free(oflib.previewimage);
	    oflib.previewimage = NULL;
	}
    }

    ret = OFLibUploadFont( &oflib );
    if ( oflib.upload_id!=NULL ) {
	char *baseurl = "http://openfontlibrary.org/media/files/";
	char *uploadcontrol = galloc(strlen(baseurl) + strlen(oflib.upload_id) + 1 );
	strcpy(uploadcontrol,baseurl);
	strcat(uploadcontrol,oflib.upload_id);
	help(uploadcontrol);
	free(uploadcontrol);
    }
    
    if ( ret && GGadgetIsChecked(GWidgetGetControl(d->gw,CID_OFLibRememberMe))) {
	free(oflib_username); free(oflib_password);
	oflib_username = copy( oflib.username );
	oflib_password = copy( oflib.password );
	for ( pt=oflib_password; *pt!='\0' ; ++pt )
	    *pt ^= 0xf;		/* Simple encryption */
    }

    free( oflib.pathspec );
    free( oflib.username );
    free( oflib.password );
    free( oflib.name );
    free( oflib.description );
    free( oflib.tags );
    free( oflib.artists );
    if ( gendefaultimage && oflib.previewimage!=NULL )
	unlink(oflib.previewimage);
    free(oflib.previewimage);
return ( ret );
}

int will_prepend_timestamp = false;

static void prepend_timestamp(struct gfc_data *d){

  if (d->sf->familyname_with_timestamp)
    free(d->sf->familyname_with_timestamp);
  d->sf->familyname_with_timestamp = NULL;
  
  if (will_prepend_timestamp){
    //prepend "YYMMDDHHMM-"

    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char timestamp[11];
    strftime(timestamp, 11, "%y%m%d%H%M", timeinfo);

    char* new_name;
    char* original_name = (d->sf->familyname) ? d->sf->familyname : d->sf->fontname;
    int i;

    if (original_name!=NULL){
    //if we already have a timestamp prepended, we should remove it before appending a new one
      int timestamp_found = (original_name[10]=='-');
      for (i=0;i<10;i++)
        if (original_name[i]<'0' || original_name[i]>'9')
          timestamp_found = false;

      if (timestamp_found)
        original_name = original_name + 11; //skip previous timestamp

      new_name = malloc (sizeof(char) * (strlen(original_name) + 12));
      sprintf(new_name, "%s-%s", timestamp, original_name);

      d->sf->familyname_with_timestamp = new_name;
    }
  }
}

static void DoSave(struct gfc_data *d,unichar_t *path) {
    int err=false;
    char *temp;
    int32 *sizes=NULL;
    int iscid, i;
    struct sflist *sfs=NULL, *cur, *last=NULL;
    static int psscalewarned=0, ttfscalewarned=0, psfnlenwarned=0;
    int flags;
    struct sflist *sfl;
    char **former;
    NameList *rename_to;
    GTextInfo *ti = GGadgetGetListItemSelected(d->rename);
    char *nlname = u2utf8_copy(ti->text);
    extern NameList *force_names_when_saving;
    int notdef_pos = SFFindNotdef(d->sf,-1);
    int layer = ly_fore;
    int likecff;
    char *buts[3];

    prepend_timestamp(d);

    buts[0] = _("_Yes");
    buts[1] = _("_No");
    buts[2] = NULL;

    rename_to = NameListByName(nlname);
    free(nlname);
    if ( rename_to!=NULL && rename_to->uses_unicode ) {
	/* I'll let someone generate a font with utf8 names, but I won't let */
	/*  them take a font and force it to unicode here. */
	ff_post_error(_("Namelist contains non-ASCII names"),_("Glyph names should be limited to characters in the ASCII character set, but there are names in this namelist which use characters outside that range."));
return;
    }
    
    for ( i=d->sf->glyphcnt-1; i>=1; --i ) if ( i!=notdef_pos )
	if ( d->sf->glyphs[i]!=NULL && strcmp(d->sf->glyphs[i]->name,".notdef")==0 &&
		(d->sf->glyphs[i]->layers[ly_fore].splines!=NULL || AnyRefs(d->sf->glyphs[i]->layers[ly_fore].refs )))
    break;
    if ( i>0 ) {
	if ( gwwv_ask(_("Notdef name"),(const char **) buts,0,1,_("The glyph at encoding %d is named \".notdef\" but contains an outline. Because it is called \".notdef\" it will not be included in the generated font. You may give it a new name using Element->Glyph Info. Do you wish to continue font generation (and omit this character)?"),d->map->backmap[i])==1 )
return;
    }

    if ( d->family==gf_none )
	layer = (intpt) GGadgetGetListItemSelected(GWidgetGetControl(d->gw,CID_Layers))->userdata;

    temp = u2def_copy(path);
    oldformatstate = GGadgetGetFirstListSelectedItem(d->pstype);
    iscid = oldformatstate==ff_cid || oldformatstate==ff_cffcid ||
	    oldformatstate==ff_otfcid || oldformatstate==ff_otfciddfont;
    if ( !iscid && (d->sf->cidmaster!=NULL || d->sf->subfontcnt>1)) {
	if ( gwwv_ask(_("Not a CID format"),(const char **) buts,0,1,_("You are attempting to save a CID font in a non-CID format. This is ok, but it means that only the current sub-font will be saved.\nIs that what you want?"))==1 )
return;
    }
    if ( oldformatstate == ff_ttf || oldformatstate==ff_ttfsym ||
	    oldformatstate==ff_ttfmacbin || oldformatstate==ff_ttfdfont ) {
	SplineChar *sc; RefChar *ref;
	for ( i=0; i<d->sf->glyphcnt; ++i ) if ( (sc = d->sf->glyphs[i])!=NULL ) {
	    if ( sc->ttf_instrs_len!=0 && sc->instructions_out_of_date ) {
		if ( gwwv_ask(_("Instructions out of date"),(const char **) buts,0,1,_("The truetype instructions on glyph %s are out of date.\nDo you want to proceed anyway?"), sc->name)==1 ) {
		    CharViewCreate(sc,(FontView *) d->sf->fv,-1);
return;
		} else
	break;
	    }
	    for ( ref=sc->layers[ly_fore].refs; ref!=NULL ; ref=ref->next )
		if ( ref->point_match_out_of_date && ref->point_match ) {
		    if ( gwwv_ask(_("Reference point match out of date"),(const char **) buts,0,1,_("In glyph %s the reference to %s is positioned by point matching, and the point numbers may no longer reflect the original intent.\nDo you want to proceed anyway?"), sc->name, ref->sc->name)==1 ) {
			CharViewCreate(sc,(FontView *) d->sf->fv,-1);
return;
		    } else
	goto end_of_loop;
		}
	}
	end_of_loop:;
    }

    if ( d->sf->os2_version==1 && (oldformatstate>=ff_otf && oldformatstate<=ff_otfciddfont)) {
	ff_post_error(_("Bad OS/2 version"), _("OpenType fonts must have a version greater than 1\nUse Element->Font Info->OS/2->Misc to change this."));
return;
    }

    likecff = 0;
    if ( oldformatstate==ff_ttc ) {
	likecff = GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TTC_CFF));
    }

    if ( likecff || oldformatstate<=ff_cffcid ||
	    (oldformatstate>=ff_otf && oldformatstate<=ff_otfciddfont)) {
	if ( d->sf->ascent+d->sf->descent!=1000 && !psscalewarned ) {
	    if ( gwwv_ask(_("Non-standard Em-Size"),(const char **) buts,0,1,_("The convention is that PostScript fonts should have an Em-Size of 1000. But this font has a size of %d. This is not an error, but you might consider altering the Em-Size with the Element->Font Info->General dialog.\nDo you wish to continue to generate your font in spite of this?"),
		    d->sf->ascent+d->sf->descent)==1 )
return;
	    psscalewarned = true;
	}
	if ( (strlen(d->sf->fontname)>31 || (d->sf->familyname!=NULL && strlen(d->sf->familyname)>31)) && !psfnlenwarned ) {
	    if ( gwwv_ask(_("Bad Font Name"),(const char **) buts,0,1,_("Some versions of Windows will refuse to install postscript fonts if the fontname is longer than 31 characters. Do you want to continue anyway?"))==1 )
return;
	    psfnlenwarned = true;
	}
    } else if ( oldformatstate!=ff_none && oldformatstate!=ff_svg &&
	    oldformatstate!=ff_ufo ) {
	int val = d->sf->ascent+d->sf->descent;
	int bit;
	for ( bit=0x800000; bit!=0; bit>>=1 )
	    if ( bit==val )
	break;
	if ( bit==0 && !ttfscalewarned ) {
	    if ( gwwv_ask(_("Non-standard Em-Size"),(const char **) buts,0,1,_("The convention is that TrueType fonts should have an Em-Size which is a power of 2. But this font has a size of %d. This is not an error, but you might consider altering the Em-Size with the Element->Font Info->General dialog.\nDo you wish to continue to generate your font in spite of this?"),val)==1 )
return;
	    ttfscalewarned = true;
	}
    }

    if ( oldformatstate==ff_ttfsym && AlreadyMSSymbolArea(d->sf,d->map))
	/* Don't complain */;
    else if ( ((oldformatstate<ff_ptype0 && oldformatstate!=ff_multiple) ||
		oldformatstate==ff_ttfsym || oldformatstate==ff_cff ) &&
		d->map->enc->has_2byte ) {
	char *buts[3];
	buts[0] = _("_Yes");
	buts[1] = _("_Cancel");
	buts[2] = NULL;
	if ( gwwv_ask(_("Encoding Too Large"),(const char **) buts,0,1,_("Your font has a 2 byte encoding, but you are attempting to save it in a format that only supports one byte encodings. This means that you won't be able to access anything after the first 256 characters without reencoding the font.\n\nDo you want to proceed anyway?"))==1 )
return;
    }

    oldbitmapstate = GGadgetGetFirstListSelectedItem(d->bmptype);
    if ( oldbitmapstate!=bf_none )
	sizes = ParseBitmapSizes(d->bmpsizes,_("Pixel List"),&err);
    if ( err )
return;
    if ( oldbitmapstate==bf_nfntmacbin && oldformatstate!=ff_pfbmacbin && !nfnt_warned ) {
	nfnt_warned = true;
	ff_post_notice(_("The 'NFNT' bitmap format is obsolete"),_("The 'NFNT' bitmap format is not used under OS/X (though you still need to create a (useless) bitmap font if you are saving a type1 PostScript resource)"));
    } else if ( oldformatstate==ff_pfbmacbin &&
	    (oldbitmapstate!=bf_nfntmacbin || sizes[0]==0)) {
	ff_post_error(_("Needs bitmap font"),_("When generating a Mac Type1 resource font, you MUST generate at least one NFNT bitmap font to go with it. If you have not created any bitmaps for this font, cancel this dlg and use the Element->Bitmaps Available command to create one"));
return;
    } else if ( oldformatstate==ff_pfbmacbin && !post_warned) {
	post_warned = true;
	ff_post_notice(_("The 'POST' type1 format is probably deprecated"),_("The 'POST' type1 format is probably depreciated and may not work in future version of the mac."));
    }

    if ( d->family ) {
	cur = chunkalloc(sizeof(struct sflist));
	cur->next = NULL;
	sfs = last = cur;
	cur->sf = d->sf;
	cur->map = EncMapCopy(d->map);
	cur->sizes = sizes;
	for ( i=0; i<d->familycnt; ++i ) {
	    if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_Family+10*i)) ) {
		cur = chunkalloc(sizeof(struct sflist));
		last->next = cur;
		last = cur;
		cur->sf = GGadgetGetUserData(GWidgetGetControl(d->gw,CID_Family+10*i));
		if ( oldbitmapstate!=bf_none )
		    cur->sizes = ParseBitmapSizes(GWidgetGetControl(d->gw,CID_Family+10*i+1),_("Pixel List"),&err);
		if ( err ) {
		    SfListFree(sfs);
return;
		}
		cur->map = EncMapFromEncoding(cur->sf,d->map->enc);
	    }
	}
    } else if ( !d->sf->multilayer && !d->sf->strokedfont && !d->sf->onlybitmaps ) {
#ifdef HAVE_PTHREAD_H
	if ( d->thread_active ) {
	    d->please_die_thread = true;
	    pthread_join(d->validate_thread,NULL);
	    d->thread_active = false;
	}
#endif
	if ( oldformatstate == ff_ptype3 || oldformatstate == ff_none )
	    /* No point in validating type3 fonts */;
	else if ( (old_validate = GGadgetIsChecked(d->validate))) {
	    int vs = SFValidate(d->sf,layer,false);
	    int mask = VSMaskFromFormat(d->sf,layer,oldformatstate);
	    int blues = ValidatePrivate(d->sf)& ~pds_missingblue;
	    if ( (vs&mask) || blues!=0 ) {
		const char *rsb[3];
		char *errs;
		int ret;

		rsb[0] =  _("_Review");
		rsb[1] =  _("_Save");
		rsb[2]=NULL;
		errs = VSErrorsFromMask(vs&mask,blues);
		ret = gwwv_ask(_("Errors detected"),rsb,0,0,_("The font contains errors.\n%sWould you like to review the errors or save the font anyway?"),errs);
		free(errs);
		if ( ret==0 ) {
		    d->done = true;
		    d->ret = false;
		    SFValidationWindow(d->sf,layer,oldformatstate);
return;
		}
		/* Ok... they want to proceed */
	    }
	}
    }

    switch ( d->sod_which ) {
      case 0:	/* PostScript */
	flags = old_ps_flags = d->ps_flags;
      break;
      case 1:	/* TrueType */
	flags = old_sfnt_flags = d->sfnt_flags;
      break;
      case 2:	/* OpenType */
	flags = old_sfnt_flags = d->sfnt_flags;
      break;
      case 3:	/* PostScript & OpenType bitmaps */
	old_ps_flags = d->ps_flags;
	flags = old_psotb_flags = d->psotb_flags;
      break;
    }

    former = NULL;
    if ( d->family && sfs!=NULL ) {
	for ( sfl=sfs; sfl!=NULL; sfl=sfl->next ) {
	    PrepareUnlinkRmOvrlp(sfl->sf,temp,layer);
	    if ( rename_to!=NULL && !iscid )
		sfl->former_names = SFTemporaryRenameGlyphsToNamelist(sfl->sf,rename_to);
	}
    } else {
	PrepareUnlinkRmOvrlp(d->sf,temp,layer);
	if ( rename_to!=NULL && !iscid )
	    former = SFTemporaryRenameGlyphsToNamelist(d->sf,rename_to);
    }

    if ( d->family==gf_none ) {
	char *wernersfdname = NULL;
	char *old_fontlog_contents;
	int res = -1;
	if ( oldformatstate == ff_multiple )
	    wernersfdname = GetWernerSFDFile(d->sf,d->map);	
	if (( oldbitmapstate==bf_bdf || oldbitmapstate==bf_fnt ||
		oldbitmapstate==bf_fon ) && ask_user_for_resolution ) {
	    ff_progress_pause_timer();
	    res = AskResolution(oldbitmapstate,d->sf->bitmaps);
	    ff_progress_resume_timer();
	}
	old_fontlog = GGadgetIsChecked(GWidgetGetControl(d->gw,CID_AppendFontLog));
	old_fontlog_contents = NULL;
	if ( old_fontlog ) {
	    char *new = GGadgetGetTitle8(GWidgetGetControl(d->gw,CID_FontLogBit));
	    if ( new!=NULL && *new!='\0' ) {
		old_fontlog_contents=d->sf->fontlog;
		if ( d->sf->fontlog==NULL )
		    d->sf->fontlog = new;
		else {
		    d->sf->fontlog = strconcat3(d->sf->fontlog,"\n\n",new);
		    free(new);
		}
		d->sf->changed = true;
	    }
	}
	if ( res!=-2 )
	    err = _DoSave(d->sf,temp,sizes,res,d->map, wernersfdname, layer );
	if ( err && old_fontlog ) {
	    free(d->sf->fontlog);
	    d->sf->fontlog = old_fontlog_contents;
	} else
	    free( old_fontlog_contents );
	free(wernersfdname);
    } else if ( d->family == gf_macfamily )
	err = !WriteMacFamily(temp,sfs,oldformatstate,oldbitmapstate,flags,layer);
    else {
	int ttcflags =
	    (GGadgetIsChecked(GWidgetGetControl(d->gw,CID_MergeTables))?ttc_flag_trymerge:0) |
	    (GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TTC_CFF))?ttc_flag_cff:0);
	err = !WriteTTC(temp,sfs,oldformatstate,oldbitmapstate,flags,layer,ttcflags);
    }

    if ( d->family && sfs!=NULL ) {
	for ( sfl=sfs; sfl!=NULL; sfl=sfl->next ) {
	    RestoreUnlinkRmOvrlp(sfl->sf,temp,layer);
	    if ( rename_to!=NULL && !iscid )
		SFTemporaryRestoreGlyphNames(sfl->sf,sfl->former_names);
	}
    } else {
	RestoreUnlinkRmOvrlp(d->sf,temp,layer);
	if ( rename_to!=NULL && !iscid )
	    SFTemporaryRestoreGlyphNames(d->sf,former);
    }
    if ( !iscid )
	force_names_when_saving = rename_to;

    free(temp);
    if ( !err && !d->family &&
	    GGadgetIsChecked(GWidgetGetControl(d->gw,CID_OFLibUpload)) )
	err = !OFLibUploadGather(d,path);
    d->done = !err;
    d->ret = !err;
}

static void GFD_doesnt(GIOControl *gio) {
    /* The filename the user chose doesn't exist, so everything is happy */
    struct gfc_data *d = gio->userdata;
    DoSave(d,gio->path);
    GFileChooserReplaceIO(d->gfc,NULL);
}

static void GFD_exists(GIOControl *gio) {
    /* The filename the user chose exists, ask user if s/he wants to overwrite */
    struct gfc_data *d = gio->userdata;
    char *temp;
    const char *rcb[3];

    rcb[2]=NULL;
    rcb[0] =  _("_Replace");
    rcb[1] =  _("_Cancel");

    if ( gwwv_ask(_("File Exists"),rcb,0,1,_("File, %s, exists. Replace it?"),
	    temp = u2utf8_copy(u_GFileNameTail(gio->path)))==0 ) {
	DoSave(d,gio->path);
    }
    free(temp);
    GFileChooserReplaceIO(d->gfc,NULL);
}

static void _GFD_SaveOk(struct gfc_data *d) {
    GGadget *tf;
    unichar_t *ret;
    int formatstate = GGadgetGetFirstListSelectedItem(d->pstype);

    if ( !d->family && GGadgetIsChecked(GWidgetGetControl(d->gw, CID_OFLibUpload))) {
	/* Check that we've got all the data we need for an oflib upload */
	if ( *_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_OFLibName))=='\0' ||
		/* Can they get by without keywords? */
		*_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_OFLibTags))=='\0' ||
		*_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_OFLibDescription))=='\0' ||
		/* Artists is definitely optional */
		*_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_OFLibUsername))=='\0' ||
		*_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_OFLibPassword))=='\0' ) {
	    ff_post_error(_("Bad OFLib upload"),
		    *_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_OFLibPassword))=='\0' ?
			    _("Missing OFLib password") :
		    *_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_OFLibUsername))=='\0' ?
			    _("Missing OFLib username") :
		    *_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_OFLibDescription))=='\0' ?
			    _("Missing OFLib description") :
		    *_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_OFLibName))=='\0' ?
			    _("Missing OFLib name") :
			    _("Missing OFLib keywords"));
return;
	}
    }
    GFileChooserGetChildren(d->gfc,NULL,NULL,&tf);
    if ( *_GGadgetGetTitle(tf)!='\0' ) {
	ret = GGadgetGetTitle(d->gfc);
	if ( formatstate!=ff_none )	/* are we actually generating an outline font? */
	    GIOfileExists(GFileChooserReplaceIO(d->gfc,
		    GIOCreate(ret,d,GFD_exists,GFD_doesnt)));
	else
	    GFD_doesnt(GIOCreate(ret,d,GFD_exists,GFD_doesnt));	/* No point in bugging the user if we aren't doing anything */
	free(ret);
    }
}

static int GFD_SaveOk(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	_GFD_SaveOk(d);
    }
return( true );
}

static int GFD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = true;
	d->ret = false;
    }
return( true );
}

static void GFD_FigureWhich(struct gfc_data *d) {
    int fs = GGadgetGetFirstListSelectedItem(d->pstype);
    int bf = GGadgetGetFirstListSelectedItem(d->bmptype);
    int which;
    if ( fs==ff_none )
	which = 1;		/* Some bitmaps get stuffed int ttf files */
    else if ( fs<=ff_cffcid )
	which = 0;		/* PostScript options */
    else if ( fs<=ff_ttfdfont )
	which = 1;		/* truetype options */ /* type42 also */
    else
	which = 2;		/* opentype options */
    if ( fs==ff_woff ) {
	SplineFont *sf = d->sf;
	int layer = d->layer;
	if ( sf->layers[layer].order2 )
	    which = 1;			/* truetype */
	else
	    which = 2;			/* opentype */
    }
    if ( bf == bf_otb && which==0 )
	which = 3;		/* postscript options with opentype bitmap options */
    d->sod_which = which;
}

static int GFD_Options(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int fs = GGadgetGetFirstListSelectedItem(d->pstype);
	int iscid = fs==ff_cid || fs==ff_cffcid || fs==ff_otfcid ||
		fs==ff_otfciddfont || fs==ff_type42cid;
	GFD_FigureWhich(d);
	SaveOptionsDlg(d,d->sod_which,iscid);
    }
return( true );
}

static void GFD_dircreated(GIOControl *gio) {
    struct gfc_data *d = gio->userdata;
    unichar_t *dir = u_copy(gio->path);

    GFileChooserReplaceIO(d->gfc,NULL);
    GFileChooserSetDir(d->gfc,dir);
    free(dir);
}

static void GFD_dircreatefailed(GIOControl *gio) {
    /* We couldn't create the directory */
    struct gfc_data *d = gio->userdata;
    char *temp;

    ff_post_notice(_("Couldn't create directory"),_("Couldn't create directory: %s"),
		temp = u2utf8_copy(u_GFileNameTail(gio->path)));
    free(temp);
    GFileChooserReplaceIO(d->gfc,NULL);
}

static int GFD_NewDir(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	char *newdir;
	unichar_t *temp;
	newdir = gwwv_ask_string(_("Create directory..."),NULL,_("Directory name?"));
	if ( newdir==NULL )
return( true );
	if ( !GFileIsAbsolute(newdir)) {
	    char *olddir = u2utf8_copy(GFileChooserGetDir(d->gfc));
	    char *temp = GFileAppendFile(olddir,newdir,false);
	    free(newdir); free(olddir);
	    newdir = temp;
	}
	temp = utf82u_copy(newdir);
	GIOmkDir(GFileChooserReplaceIO(d->gfc,
		GIOCreate(temp,d,GFD_dircreated,GFD_dircreatefailed)));
	free(newdir); free(temp);
    }
return( true );
}

static void BitmapName(struct gfc_data *d) {
    int bf = GGadgetGetFirstListSelectedItem(d->bmptype);
    unichar_t *ret = GGadgetGetTitle(d->gfc);
    unichar_t *dup, *pt, *tpt;
    int format = GGadgetGetFirstListSelectedItem(d->pstype);

    if ( format!=ff_none )
return;

    dup = galloc((u_strlen(ret)+30)*sizeof(unichar_t));
    u_strcpy(dup,ret);
    free(ret);
    pt = u_strrchr(dup,'.');
    tpt = u_strrchr(dup,'/');
    if ( pt<tpt )
	pt = NULL;
    if ( pt==NULL ) pt = dup+u_strlen(dup);
    if ( uc_strcmp(pt-5, ".bmap.bin" )==0 ) pt -= 5;
    if ( uc_strcmp(pt-4, ".ttf.bin" )==0 ) pt -= 4;
    if ( uc_strcmp(pt-4, ".otf.dfont" )==0 ) pt -= 4;
    if ( uc_strncmp(pt-2, "%s", 2 )==0 ) pt -= 2;
    if ( uc_strncmp(pt-2, "-*", 2 )==0 ) pt -= 2;
    uc_strcpy(pt,bitmapextensions[bf]);
    GGadgetSetTitle(d->gfc,dup);
    free(dup);
}

static int GFD_Format(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	unichar_t *pt, *dup, *tpt, *ret;
	int format = GGadgetGetFirstListSelectedItem(d->pstype);
	int32 len; int bf;
	static unichar_t nullstr[] = { 0 };
	GTextInfo **list;
	SplineFont *temp;

	list = GGadgetGetList(d->bmptype,&len);
	temp = d->sf->cidmaster ? d->sf->cidmaster : d->sf;
	if ( format==ff_none ) {
	    if ( temp->bitmaps!=NULL ) {
		list[bf_sfnt_dfont]->disabled = false;
		list[bf_sfnt_ms]->disabled = false;
		list[bf_otb]->disabled = false;
		list[bf_ttf]->disabled = true;
	    }
	    BitmapName(d);
return( true );
	}

	ret = GGadgetGetTitle(d->gfc);
	dup = galloc((u_strlen(ret)+30)*sizeof(unichar_t));
	u_strcpy(dup,ret);
	free(ret);
	pt = u_strrchr(dup,'.');
	tpt = u_strrchr(dup,'/');
	if ( pt<tpt )
	    pt = NULL;
	if ( pt==NULL ) pt = dup+u_strlen(dup);
	if ( uc_strcmp(pt-5, ".bmap.bin" )==0 ) pt -= 5;
	if ( uc_strcmp(pt-4, ".ttf.bin" )==0 ) pt -= 4;
	if ( uc_strcmp(pt-4, ".otf.dfont" )==0 ) pt -= 4;
	if ( uc_strcmp(pt-4, ".cid.t42" )==0 ) pt -= 4;
	if ( uc_strncmp(pt-2, "%s", 2 )==0 ) pt -= 2;
	if ( uc_strncmp(pt-2, "-*", 2 )==0 ) pt -= 2;
	uc_strcpy(pt,savefont_extensions[format]);
	GGadgetSetTitle(d->gfc,dup);
	free(dup);

	if ( d->sf->cidmaster!=NULL ) {
	    if ( format!=ff_none && format != ff_cid && format != ff_cffcid &&
		    format != ff_otfcid && format!=ff_otfciddfont ) {
		GGadgetSetTitle(d->bmpsizes,nullstr);
	    }
	}

	bf = GGadgetGetFirstListSelectedItem(d->bmptype);
	list[bf_sfnt_dfont]->disabled = true;
	if ( temp->bitmaps==NULL )
	    /* Don't worry about what formats are possible, they're disabled */;
	else if ( format!=ff_ttf && format!=ff_ttfsym && format!=ff_otf &&
		format!=ff_ttfdfont && format!=ff_otfdfont && format!=ff_otfciddfont &&
		format!=ff_otfcid && format!=ff_ttfmacbin && format!=ff_none ) {
	    /* If we're not in a ttf format then we can't output ttf bitmaps */
	    list[bf_ttf]->disabled = true;
	    list[bf_sfnt_dfont]->disabled = false;
	    list[bf_sfnt_ms]->disabled = false;
	    list[bf_otb]->disabled = false;
	    if ( bf==bf_ttf )
		GGadgetSelectOneListItem(d->bmptype,bf_otb);
	    if ( format==ff_pfbmacbin )
		GGadgetSelectOneListItem(d->bmptype,bf_nfntmacbin);
	    bf = GGadgetGetFirstListSelectedItem(d->bmptype);
	    GGadgetSetEnabled(d->bmpsizes, format!=ff_multiple && bf!=bf_none );	/* We know there are bitmaps */
	} else {
	    list[bf_ttf]->disabled = false;
	    list[bf_sfnt_dfont]->disabled = true;
	    list[bf_sfnt_ms]->disabled = true;
	    list[bf_otb]->disabled = true;
	    if ( bf==bf_none )
		/* Do nothing, always appropriate */;
	    else if ( format==ff_ttf || format==ff_ttfsym || format==ff_otf ||
		    format==ff_otfcid ||
		    bf==bf_sfnt_dfont || bf == bf_sfnt_ms || bf == bf_otb )
		GGadgetSelectOneListItem(d->bmptype,bf_ttf);
	}
#if __Mac
	{ GGadget *pulldown, *list, *tf;
	    /* The name of the postscript file is fixed and depends solely on */
	    /*  the font name. If the user tried to change it, the font would */
	    /*  not be found */
	    /* See MakeMacPSName for a full description */
	    GFileChooserGetChildren(d->gfc,&pulldown,&list,&tf);
	    GGadgetSetVisible(tf,format!=ff_pfbmacbin);
	}
#endif
	GGadgetSetEnabled(d->bmptype, format!=ff_multiple );
    }
return( true );
}

static int GFD_BitmapFormat(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	/*int format = GGadgetGetFirstListSelectedItem(d->pstype);*/
	int bf = GGadgetGetFirstListSelectedItem(d->bmptype);
	int i;

	GGadgetSetEnabled(d->bmpsizes,bf!=bf_none);
	if ( d->family ) {
	    for ( i=0; i<d->familycnt; ++i )
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Family+10*i+1),
			bf!=bf_none);
	}
	BitmapName(d);
    }
return( true );
}

static int GFD_TogglePrependTimestamp(GGadget *g, GEvent *e) {
  if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
    will_prepend_timestamp = GGadgetIsChecked(g);
  }
  return( true );
}

static int GFD_ToggleFontLog(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	static int cids[] = {
	    CID_FontLogBit,
	    0 };
	int i, visible = GGadgetIsChecked(g);

	for ( i=0; cids[i]!=0; ++i ) {
	    GGadgetSetVisible(GWidgetGetControl(d->gw,cids[i]),visible);
	    g = GWidgetGetControl(d->gw,cids[i]+CID_OFLibLabOffset);
	    if ( g!=NULL )
		GGadgetSetVisible(g,visible);
	}
	    
	GWidgetToDesiredSize(d->gw);
    }
return( true );
}

static int GFD_ToggleOFLib(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	static int cids[] = {
	    CID_OFLibUsername, CID_OFLibPassword, CID_OFLibRememberMe, 
	    CID_OFLibOFL, CID_OFLibPD,
	    CID_OFLibName, CID_OFLibTags, CID_OFLibDescription, CID_OFLibArtists,
	    CID_OFLibNotSafe,
	    CID_OFLibLine1, CID_OFLibLine2,
	    CID_OFLibGenPreview, CID_OFLibNoPreview, CID_OFLibDiskPreview,
	    CID_OFLibPreviewText, CID_OFLibPreviewBrowse,
	    CID_UploadLicense, CID_UploadFontLog,
	    0 };
	int i, visible = GGadgetIsChecked(g);

	for ( i=0; cids[i]!=0; ++i ) {
	    GGadgetSetVisible(GWidgetGetControl(d->gw,cids[i]),visible);
	    g = GWidgetGetControl(d->gw,cids[i]+CID_OFLibLabOffset);
	    if ( g!=NULL )
		GGadgetSetVisible(g,visible);
	}
	    
	GWidgetToDesiredSize(d->gw);
    }
return( true );
}

static int GFD_OFLibHelp(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	help("http://www.openfontlibrary.org/");
    }
return( true );
}

static int GFD_OFLibRegister(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	help("http://www.openfontlibrary.org/media/register");
    }
return( true );
}

static int GFD_OFLibPreviewBrowse(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	char *filename = GGadgetGetTitle8(GWidgetGetControl(d->gw,CID_OFLibPreviewText));
	char *newname;

	GGadgetSetChecked(GWidgetGetControl(d->gw,CID_OFLibDiskPreview), true);

	if ( *filename=='\0' ) {
	    free(filename);
	    filename = NULL;
	}
	newname = gwwv_open_filename(_("Browse for a preview image"), filename, "*.{png,gif,jpeg}",NULL);
	if ( newname!=NULL )
	    GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_OFLibPreviewText), newname );
	free(newname);
	free(filename);
    }
return( true );
}

static int e_h(GWindow gw, GEvent *event) {
    extern int GGadgetWithin(GGadget *g, int x, int y);

    if ( event->type==et_close ) {
	struct gfc_data *d = GDrawGetUserData(gw);
	d->done = true;
	d->ret = false;
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("generate.html");
return( true );
	} else if ( (event->u.chr.keysym=='s' || event->u.chr.keysym=='g' ||
		event->u.chr.keysym=='G' ) &&
		(event->u.chr.state&ksm_control) ) {
	    _GFD_SaveOk(GDrawGetUserData(gw));
return( true );
	}
return( false );
    } else if ( event->type == et_mousemove ||
	    (event->type==et_mousedown && event->u.mouse.button==3 )) {
	struct gfc_data *d = GDrawGetUserData(gw);
	if ( !GGadgetWithin(d->gfc,event->u.mouse.x,event->u.mouse.y))
return( false );
	GFileChooserPopupCheck(d->gfc,event);
    } else if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
	struct gfc_data *d = GDrawGetUserData(gw);
return( GGadgetDispatchEvent((GGadget *) (d->gfc),event));
    }
return( true );
}

static unichar_t *BitmapList(SplineFont *sf) {
    BDFFont *bdf;
    int i;
    char *cret, *pt;
    unichar_t *uret;

    for ( bdf=sf->bitmaps, i=0; bdf!=NULL; bdf=bdf->next, ++i );
    pt = cret = galloc((i+1)*20);
    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	if ( pt!=cret ) *pt++ = ',';
	if ( bdf->clut==NULL )
	    sprintf( pt, "%d", bdf->pixelsize );
	else
	    sprintf( pt, "%d@%d", bdf->pixelsize, BDFDepth(bdf) );
	pt += strlen(pt);
    }
    *pt = '\0';
    uret = uc_copy(cret);
    free(cret);
return( uret );
}

static unichar_t *uStyleName(SplineFont *sf) {
    int stylecode = MacStyleCode(sf,NULL);
    char buffer[200];

    buffer[0]='\0';
    if ( stylecode&sf_bold )
	strcpy(buffer," Bold");
    if ( stylecode&sf_italic )
	strcat(buffer," Italic");
    if ( stylecode&sf_outline )
	strcat(buffer," Outline");
    if ( stylecode&sf_shadow )
	strcat(buffer," Shadow");
    if ( stylecode&sf_condense )
	strcat(buffer," Condensed");
    if ( stylecode&sf_extend )
	strcat(buffer," Extended");
    if ( buffer[0]=='\0' )
	strcpy(buffer," Plain");
return( uc_copy(buffer+1));
}

#if 0 && defined( HAVE_PTHREAD_H )
static void *BackgroundValidate(void *_d) {
    struct gfc_data *d = _d;
    SplineFont *sf = d->sf, *ssf;
    int k, gid;
    SplineChar *sc;

    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;
    k=0;
    do {
	ssf = sf->subfontcnt==0 ? sf : sf->subfonts[k];
	for ( gid = 0; gid<ssf->glyphcnt; ++gid ) if ( (sc=ssf->glyphs[gid])!=NULL ) {
	    if ( d->please_die_thread )
pthread_exit(NULL);
	    if ( !(sc->validation_state&vs_known) )
		SCValidate(sc,true);
	}
	++k;
    } while ( k<sf->subfontcnt );

return( NULL );
}
#endif

static GTextInfo *SFUsableLayerNames(SplineFont *sf,int def_layer) {
    int gid, layer, cnt = 1, k, known;
    SplineFont *_sf;
    SplineChar *sc;
    GTextInfo *ti;

    for ( layer=0; layer<sf->layer_cnt; ++layer )
	sf->layers[layer].ticked = false;
    sf->layers[ly_fore].ticked = true;
    for ( layer=0; layer<sf->layer_cnt; ++layer ) if ( !sf->layers[layer].background ) {
	known = -1;
	k = 0;
	do {
	    _sf = sf->subfontcnt==0 ? sf : sf->subfonts[k];
	    for ( gid = 0; gid<_sf->glyphcnt; ++gid ) if ( (sc=_sf->glyphs[gid])!=NULL ) {
		if ( sc->layers[layer].images!=NULL ) {
		    known = 0;
	    break;
		}
		if ( sc->layers[layer].splines!=NULL )
		    known = 1;
	    }
	    ++k;
	} while ( known!=0 && k<sf->subfontcnt );
	if ( known == 1 ) {
	    sf->layers[layer].ticked = true;
	    ++cnt;
	} else if ( layer==def_layer )
	    def_layer = ly_fore;
    }

    ti = gcalloc(cnt+1,sizeof(GTextInfo));
    cnt=0;
    for ( layer=0; layer<sf->layer_cnt; ++layer ) if ( sf->layers[layer].ticked ) {
	ti[cnt].text = (unichar_t *) sf->layers[layer].name;
	ti[cnt].text_is_1byte = true;
	ti[cnt].selected = layer==def_layer;
	ti[cnt++].userdata = (void *) (intpt) layer;
    }
return( ti );
}

typedef SplineFont *SFArray[48];

int SFGenerateFont(SplineFont *sf,int layer,int family,EncMap *map) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[20+2*48+5+1], *varray[13], *hvarray[57], *famarray[3*52+1],
	    *harray[10], boxes[9], *oflarray[11][5], *oflibinfo[8], *parray[3][4], *p2array[5];
    GTextInfo label[20+2*48+4+1];
    struct gfc_data d;
    GGadget *pulldown, *files, *tf;
    int hvi, i, j, k, f, old, ofs, y, fc, dupfc, dupstyle, rk, vk;
    int bs = GIntGetResource(_NUM_Buttonsize), bsbigger, totwid, spacing;
    SplineFont *temp;
    int familycnt=0;
    int fondcnt = 0, fondmax = 10;
    SFArray *familysfs=NULL;
    uint16 psstyle;
    static int done=false;
    extern NameList *force_names_when_saving;
    char **nlnames;
    int cnt, any;
    GTextInfo *namelistnames, *lynames=NULL;
    static GBox small_blue_box;
    extern GBox _GGadget_button_box;
    char *oflpwd;

    memset(&d,'\0',sizeof(d));
    d.sf = sf;
    d.layer = layer;
#if 0 && defined(HAVE_PTHREAD_H)
    /* Drat. my allocation routines are not thread safe */
    /* If I can't create the thread, that's not a real problem. The font just*/
    /*  won't be prevalidated */
    if ( !sf->multilayer && !sf->strokedfont && !sf->onlybitmaps )
	if ( pthread_create(&d.validate_thread,NULL,BackgroundValidate,(void *) &d)==0 )
	    d.thread_active = true;
#endif

    if ( !done ) {
	done = true;
	for ( i=0; formattypes[i].text; ++i )
	    formattypes[i].text = (unichar_t *) _((char *) formattypes[i].text);
	for ( i=0; bitmaptypes[i].text; ++i )
	    bitmaptypes[i].text = (unichar_t *) _((char *) bitmaptypes[i].text);
    }

    if ( family==gf_macfamily ) {
	old_sfnt_flags |=  ttf_flag_applemode;
	old_sfnt_flags &= ~ttf_flag_otmode;
    }

    /* Let's not support broken size any more */
    old_sfnt_flags &= ~ttf_flag_brokensize;
    old_psotb_flags &= ~ttf_flag_brokensize;

    if ( family ) {
	/* I could just disable the menu item, but I think it's a bit confusing*/
	/*  and I want people to know why they can't generate a family */
	FontView *fv;
	SplineFont *dup=NULL/*, *badenc=NULL*/;
	familysfs = galloc((fondmax=10)*sizeof(SFArray));
	memset(familysfs[0],0,sizeof(familysfs[0]));
	familysfs[0][0] = sf;
	fondcnt = 1;
	for ( fv=fv_list; fv!=NULL; fv=(FontView *) (fv->b.next) ) {
	    if ( fv->b.sf==sf )
	continue;
	    if ( family==gf_ttc ) {
		fc = fondcnt;
		psstyle = 0;
	    } else if ( family==gf_macfamily && strcmp(fv->b.sf->familyname,sf->familyname)==0 ) {
		MacStyleCode(fv->b.sf,&psstyle);
		if ( fv->b.sf->fondname==NULL ) {
		    fc = 0;
		    if ( familysfs[0][0]->fondname==NULL &&
			    (familysfs[0][psstyle]==NULL || familysfs[0][psstyle]==fv->b.sf))
			familysfs[0][psstyle] = fv->b.sf;
		    else {
			for ( fc=0; fc<fondcnt; ++fc ) {
			    for ( i=0; i<48; ++i )
				if ( familysfs[fc][i]!=NULL )
			    break;
			    if ( i<48 && familysfs[fc][i]->fondname==NULL &&
				    familysfs[fc][psstyle]==fv->b.sf )
			break;
			}
		    }
		} else {
		    for ( fc=0; fc<fondcnt; ++fc ) {
			for ( i=0; i<48; ++i )
			    if ( familysfs[fc][i]!=NULL )
			break;
			if ( i<48 && familysfs[fc][i]->fondname!=NULL &&
				strcmp(familysfs[fc][i]->fondname,fv->b.sf->fondname)==0 ) {
			    if ( familysfs[fc][psstyle]==fv->b.sf )
				/* several windows may point to same font */; 
			    else if ( familysfs[fc][psstyle]!=NULL ) {
				dup = fv->b.sf;
			        dupstyle = psstyle;
			        dupfc = fc;
			    } else
				familysfs[fc][psstyle] = fv->b.sf;
		    break;
			}
		    }
		}
	    }
	    if ( fc==fondcnt ) {
		/* Create a new fond containing just this font */
		if ( fondcnt>=fondmax )
		    familysfs = grealloc(familysfs,(fondmax+=10)*sizeof(SFArray));
		memset(familysfs[fondcnt],0,sizeof(SFArray));
		familysfs[fondcnt++][psstyle] = fv->b.sf;
	    }
	}
	if ( family==gf_macfamily ) {
	    for ( fc=0; fc<fondcnt; ++fc ) for ( i=0; i<48; ++i ) {
		if ( familysfs[fc][i]!=NULL ) {
		    ++familycnt;
		}
	    }
	    if ( MacStyleCode(sf,NULL)!=0 || familycnt<=1 || sf->multilayer ) {
		ff_post_error(_("Bad Mac Family"),_("To generate a Mac family file, the current font must have plain (Normal, Regular, etc.) style, and there must be other open fonts with the same family name."));
return( 0 );
	    } else if ( dup ) {
		MacStyleCode(dup,&psstyle);
		ff_post_error(_("Bad Mac Family"),_("There are two open fonts with the current family name and the same style. %.30s and %.30s"),
		    dup->fontname, familysfs[dupfc][dupstyle]->fontname);
    return( 0 );
	    }
	} else {
	    familycnt = fondcnt;
	}
    }

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = family?_("Generate Mac Family"):_("Generate Fonts");
    pos.x = pos.y = 0;
    totwid = GGadgetScale(295);
    bsbigger = 4*bs+4*14>totwid; totwid = bsbigger?4*bs+4*12:totwid;
    spacing = (totwid-4*bs-2*12)/3;
    pos.width = GDrawPointsToPixels(NULL,totwid);
    if ( family )
	pos.height = GDrawPointsToPixels(NULL,310+13+26*(familycnt-1));
    else
	pos.height = GDrawPointsToPixels(NULL,310);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; gcd[0].gd.pos.width = 100*totwid/GIntGetResource(_NUM_ScaleFactor)-24; gcd[0].gd.pos.height = 182;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].creator = GFileChooserCreate;
    varray[0] = &gcd[0]; varray[1] = NULL;

    y = 276;
    if ( family )
	y += 13 + 26*(familycnt-1);

    gcd[1].gd.pos.x = 12; gcd[1].gd.pos.y = y-3;
    gcd[1].gd.pos.width = -1;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[1].text = (unichar_t *) _("_Save");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.mnemonic = 'S';
    gcd[1].gd.label = &label[1];
    gcd[1].gd.handle_controlevent = GFD_SaveOk;
    gcd[1].creator = GButtonCreate;
    harray[0] = GCD_Glue; harray[1] = &gcd[1]; 

    gcd[2].gd.pos.x = -(spacing+bs)*100/GIntGetResource(_NUM_ScaleFactor)-12; gcd[2].gd.pos.y = y;
    gcd[2].gd.pos.width = -1;
    gcd[2].gd.flags = gg_visible | gg_enabled;
    label[2].text = (unichar_t *) _("_Filter");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.mnemonic = 'F';
    gcd[2].gd.label = &label[2];
    gcd[2].gd.handle_controlevent = GFileChooserFilterEh;
    gcd[2].creator = GButtonCreate;
    harray[2] = GCD_Glue; harray[3] = &gcd[2]; 

    gcd[3].gd.pos.x = -12; gcd[3].gd.pos.y = y; gcd[3].gd.pos.width = -1; gcd[3].gd.pos.height = 0;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[3].text = (unichar_t *) _("_Cancel");
    label[3].text_is_1byte = true;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.mnemonic = 'C';
    gcd[3].gd.handle_controlevent = GFD_Cancel;
    gcd[3].creator = GButtonCreate;
    harray[6] = GCD_Glue; harray[7] = &gcd[3]; harray[8] = GCD_Glue; harray[9] = NULL;

    gcd[4].gd.pos.x = (spacing+bs)*100/GIntGetResource(_NUM_ScaleFactor)+12; gcd[4].gd.pos.y = y; gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled;
    label[4].text = (unichar_t *) S_("Directory|_New");
    label[4].text_is_1byte = true;
    label[4].text_in_resource = true;
    label[4].image = &_GIcon_dir;
    label[4].image_precedes = false;
    gcd[4].gd.mnemonic = 'N';
    gcd[4].gd.label = &label[4];
    gcd[4].gd.handle_controlevent = GFD_NewDir;
    gcd[4].creator = GButtonCreate;
    harray[4] = GCD_Glue; harray[5] = &gcd[4]; 

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray;
    boxes[2].creator = GHBoxCreate;

    gcd[5].gd.pos.x = 12; gcd[5].gd.pos.y = 218; gcd[5].gd.pos.width = 0; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    label[5].text = (unichar_t *) _("Options");
    label[5].text_is_1byte = true;
    gcd[5].gd.popup_msg = (unichar_t *) _("Allows you to select optional behavior when generating the font");
    gcd[5].gd.label = &label[5];
    gcd[5].gd.handle_controlevent = GFD_Options;
    gcd[5].creator = GButtonCreate;
    hvarray[4] = &gcd[5]; hvarray[5] = GCD_Glue;

    gcd[6].gd.pos.x = 12; gcd[6].gd.pos.y = 190; gcd[6].gd.pos.width = 0; gcd[6].gd.pos.height = 0;
    gcd[6].gd.flags = gg_visible | gg_enabled ;
    gcd[6].gd.u.list = formattypes;
    gcd[6].creator = GListButtonCreate;
    hvarray[0] = &gcd[6]; hvarray[1] = GCD_ColSpan;

    any = false;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sf->glyphs[i])) {
	any = true;
    break;
    }
    for ( i=0; i<sizeof(formattypes)/sizeof(formattypes[0])-1; ++i )
	formattypes[i].disabled = !any;
    formattypes[ff_ptype0].disabled = sf->onlybitmaps || map->enc->only_1byte;
    formattypes[ff_mma].disabled = formattypes[ff_mmb].disabled =
	     sf->mm==NULL || sf->mm->apple || !MMValid(sf->mm,false);
    formattypes[ff_cffcid].disabled = sf->cidmaster==NULL;
    formattypes[ff_cid].disabled = sf->cidmaster==NULL;
    formattypes[ff_otfcid].disabled = sf->cidmaster==NULL;
    formattypes[ff_otfciddfont].disabled = sf->cidmaster==NULL;
    if ( map->enc->is_unicodefull )
	formattypes[ff_type42cid].disabled = true;	/* Identity CMap only handles BMP */
    ofs = oldformatstate;
    if (( ofs==ff_ptype0 && formattypes[ff_ptype0].disabled ) ||
	    ((ofs==ff_mma || ofs==ff_mmb) && sf->mm==NULL) ||
	    ((ofs==ff_cid || ofs==ff_cffcid || ofs==ff_otfcid || ofs==ff_otfciddfont) && formattypes[ff_cid].disabled))
	ofs = ff_pfb;
    else if ( (ofs!=ff_cid && ofs!=ff_cffcid && ofs!=ff_otfcid && ofs!=ff_otfciddfont) && sf->cidmaster!=NULL )
	ofs = ff_otfcid;
    else if ( !formattypes[ff_mmb].disabled && ofs!=ff_mma )
	ofs = ff_mmb;
    if ( ofs==ff_ttc )
	ofs = ff_ttf;
    if ( sf->onlybitmaps )
	ofs = ff_none;
    if ( sf->multilayer ) {
	formattypes[ff_pfa].disabled = true;
	formattypes[ff_pfb].disabled = true;
	formattypes[ff_pfbmacbin].disabled = true;
	formattypes[ff_mma].disabled = true;
	formattypes[ff_mmb].disabled = true;
	formattypes[ff_multiple].disabled = true;
	formattypes[ff_ptype0].disabled = true;
	formattypes[ff_cff].disabled = true;
	formattypes[ff_cffcid].disabled = true;
	formattypes[ff_cid].disabled = true;
	formattypes[ff_ttf].disabled = true;
	formattypes[ff_type42].disabled = true;
	formattypes[ff_type42cid].disabled = true;
	formattypes[ff_ttfsym].disabled = true;
	formattypes[ff_ttfmacbin].disabled = true;
	formattypes[ff_ttfdfont].disabled = true;
	formattypes[ff_otfdfont].disabled = true;
	formattypes[ff_otf].disabled = true;
	formattypes[ff_otfcid].disabled = true;
	formattypes[ff_cffcid].disabled = true;
	formattypes[ff_ufo].disabled = true;
	if ( ofs!=ff_svg )
	    ofs = ff_ptype3;
    } else if ( sf->strokedfont ) {
	formattypes[ff_ttf].disabled = true;
	formattypes[ff_ttfsym].disabled = true;
	formattypes[ff_ttfmacbin].disabled = true;
	formattypes[ff_ttfdfont].disabled = true;
	formattypes[ff_ufo].disabled = true;
	if ( ofs==ff_ttf || ofs==ff_ttfsym || ofs==ff_ttfmacbin || ofs==ff_ttfdfont )
	    ofs = ff_otf;
    }
    formattypes[ff_ttc].disabled = true;
    if ( family == gf_macfamily ) {
	if ( ofs==ff_pfa || ofs==ff_pfb || ofs==ff_multiple || ofs==ff_ptype3 ||
		ofs==ff_ptype0 || ofs==ff_mma || ofs==ff_mmb )
	    ofs = ff_pfbmacbin;
	else if ( ofs==ff_cid || ofs==ff_otfcid || ofs==ff_cffcid )
	    ofs = ff_otfciddfont;
	else if ( ofs==ff_ttf || ofs==ff_ttfsym )
	    ofs = ff_ttfmacbin;
	else if ( ofs==ff_otf || ofs==ff_cff )
	    ofs = ff_otfdfont;
	else if ( ofs==ff_ufo || ofs==ff_ttc )
	    ofs = ff_ttfdfont;
	formattypes[ff_pfa].disabled = true;
	formattypes[ff_pfb].disabled = true;
	formattypes[ff_mma].disabled = true;
	formattypes[ff_mmb].disabled = true;
	formattypes[ff_multiple].disabled = true;
	formattypes[ff_ptype3].disabled = true;
	formattypes[ff_ptype0].disabled = true;
	formattypes[ff_type42].disabled = true;
	formattypes[ff_type42cid].disabled = true;
	formattypes[ff_ttf].disabled = true;
	formattypes[ff_ttfsym].disabled = true;
	formattypes[ff_otf].disabled = true;
	formattypes[ff_otfcid].disabled = true;
	formattypes[ff_cff].disabled = true;
	formattypes[ff_cffcid].disabled = true;
	formattypes[ff_svg].disabled = true;
	formattypes[ff_ufo].disabled = true;
    } else if ( family == gf_ttc ) {
	for ( i=0; i<=ff_none; ++i )
	    formattypes[i].disabled = true;
	formattypes[ff_ttc].disabled = false;
	ofs = ff_ttc;
    }
    if ( !CanWoff())
	formattypes[ff_woff].disabled = true;
    for ( i=0; i<sizeof(formattypes)/sizeof(formattypes[0]); ++i )
	formattypes[i].selected = false;
    formattypes[ofs].selected = true;
    gcd[6].gd.handle_controlevent = GFD_Format;
    gcd[6].gd.label = &formattypes[ofs];

    gcd[7].gd.pos.x = 2; gcd[7].gd.pos.y = 2;
    gcd[7].gd.pos.width = pos.width-4; gcd[7].gd.pos.height = pos.height-4;
    gcd[7].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[7].creator = GGroupCreate;

    gcd[8].gd.pos.x = 155; gcd[8].gd.pos.y = 190; gcd[8].gd.pos.width = 126;
    gcd[8].gd.flags = gg_visible | gg_enabled;
    gcd[8].gd.u.list = bitmaptypes;
    gcd[8].creator = GListButtonCreate;
    for ( i=0; i<sizeof(bitmaptypes)/sizeof(bitmaptypes[0]); ++i ) {
	bitmaptypes[i].selected = false;
	bitmaptypes[i].disabled = false;
    }
    hvarray[2] = &gcd[8]; hvarray[3] = NULL;
    old = oldbitmapstate;
    if ( family==gf_macfamily ) {
	if ( old==bf_bdf || old==bf_fon || old==bf_fnt || old==bf_sfnt_ms ||
		old==bf_otb || old==bf_palm || old==bf_ptype3 ) {
	    if ( ofs==ff_otfdfont || ofs==ff_otfciddfont || ofs==ff_ttfdfont )
		old = bf_ttf;
	    else
		old = bf_sfnt_dfont;
	} else if ( old==bf_nfntmacbin &&
		    ( ofs==ff_otfdfont || ofs==ff_otfciddfont || ofs==ff_ttfdfont ))
	    old = bf_ttf;
	bitmaptypes[bf_bdf].disabled = true;
	bitmaptypes[bf_fon].disabled = true;
	bitmaptypes[bf_fnt].disabled = true;
    } else if ( family==gf_ttc ) {
	for ( i=0; i<bf_none; ++i )
	    bitmaptypes[i].disabled = true;
	bitmaptypes[bf_ttf].disabled = false;
	if ( old!=bf_none )
	    old = bf_ttf;
    }
    temp = sf->cidmaster ? sf->cidmaster : sf;
    if ( temp->bitmaps==NULL ) {
	old = bf_none;
	bitmaptypes[bf_bdf].disabled = true;
	bitmaptypes[bf_ttf].disabled = true;
	bitmaptypes[bf_sfnt_dfont].disabled = true;
	bitmaptypes[bf_sfnt_ms].disabled = true;
	bitmaptypes[bf_otb].disabled = true;
	bitmaptypes[bf_nfntmacbin].disabled = true;
	bitmaptypes[bf_fon].disabled = true;
	bitmaptypes[bf_fnt].disabled = true;
	bitmaptypes[bf_palm].disabled = true;
	bitmaptypes[bf_ptype3].disabled = true;
    } else if ( ofs==ff_ttf || ofs==ff_ttfsym || ofs==ff_ttfmacbin ||
	    ofs==ff_ttfdfont || ofs==ff_otf || ofs==ff_otfdfont || ofs==ff_otfcid ||
	    ofs==ff_otfciddfont ) {
	bitmaptypes[bf_ttf].disabled = false;
	bitmaptypes[bf_sfnt_dfont].disabled = true;
	bitmaptypes[bf_sfnt_ms].disabled = true;
	bitmaptypes[bf_otb].disabled = true;
    } else {
	bitmaptypes[bf_ttf].disabled = true;
	bitmaptypes[bf_sfnt_dfont].disabled = false;
	bitmaptypes[bf_sfnt_ms].disabled = false;
	bitmaptypes[bf_otb].disabled = false;
    }
    bitmaptypes[old].selected = true;
    gcd[8].gd.label = &bitmaptypes[old];
    gcd[8].gd.handle_controlevent = GFD_BitmapFormat;

    gcd[9].gd.pos.x = gcd[8].gd.pos.x; gcd[9].gd.pos.y = 219; gcd[9].gd.pos.width = gcd[8].gd.pos.width;
    gcd[9].gd.flags = gg_visible | gg_enabled;
    if ( old==bf_none )
	gcd[9].gd.flags &= ~gg_enabled;
    gcd[9].creator = GTextFieldCreate;
    label[9].text = BitmapList(temp);
    gcd[9].gd.label = &label[9];
    hvarray[6] = &gcd[9]; hvarray[7] = NULL;

    k = 10;
    label[k].text = (unichar_t *) _("Force glyph names to:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 8; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+24+6;
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("In the saved font, force all glyph names to match those in the specified namelist");
    gcd[k++].creator = GLabelCreate;
    hvarray[8] = &gcd[k-1]; hvarray[9] = GCD_ColSpan;

    rk = k;
    gcd[k].gd.pos.x = gcd[k-2].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-6;
    gcd[k].gd.pos.width = gcd[k-2].gd.pos.width;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("In the saved font, force all glyph names to match those in the specified namelist");
    gcd[k].creator = GListButtonCreate;
    nlnames = AllNamelistNames();
    for ( cnt=0; nlnames[cnt]!=NULL; ++cnt);
    namelistnames = gcalloc(cnt+3,sizeof(GTextInfo));
    namelistnames[0].text = (unichar_t *) _("No Rename");
    namelistnames[0].text_is_1byte = true;
    if ( force_names_when_saving==NULL ) {
	namelistnames[0].selected = true;
	gcd[k].gd.label = &namelistnames[0];
    }
    namelistnames[1].line = true;
    for ( cnt=0; nlnames[cnt]!=NULL; ++cnt) {
	namelistnames[cnt+2].text = (unichar_t *) nlnames[cnt];
	namelistnames[cnt+2].text_is_1byte = true;
	if ( force_names_when_saving!=NULL &&
		strcmp(_(force_names_when_saving->title),nlnames[cnt])==0 ) {
	    namelistnames[cnt+2].selected = true;
	    gcd[k].gd.label = &namelistnames[cnt+2];
	}
    }
    gcd[k++].gd.u.list = namelistnames;
    free(nlnames);
    hvarray[10] = &gcd[k-1]; hvarray[11] = NULL;

    if ( !family ) {
	/* Too annoying to check if all fonts in a family have the same set of*/
	/*  useful layers. So only do this if not family */
	label[k].text = (unichar_t *) _("Layer:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 8; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+24+6;
	gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
	gcd[k].gd.popup_msg = (unichar_t *) _("In the saved font, force all glyph names to match those in the specified namelist");
	gcd[k++].creator = GLabelCreate;

	gcd[k].gd.pos.x = gcd[k-2].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-6;
	gcd[k].gd.pos.width = gcd[k-2].gd.pos.width;
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	gcd[k].gd.popup_msg = (unichar_t *) _("Save a font based on the specified layer");
	gcd[k].creator = GListButtonCreate;
	gcd[k].gd.cid = CID_Layers;
	gcd[k++].gd.u.list = lynames = SFUsableLayerNames(sf,layer);
	if ( lynames[1].text==NULL ) {
	    gcd[k-2].gd.flags &= ~gg_visible;
	    gcd[k-1].gd.flags &= ~gg_visible;
	}
	hvi=12;
	hvarray[hvi++] = &gcd[k-2]; hvarray[hvi++] = GCD_ColSpan; hvarray[hvi++] = &gcd[k-1];
	hvarray[hvi++] = NULL;

	/* Too time consuming to validate lots of fonts, and what UI would I use? */
	/*  so only do this if not family */
	vk = k;
	label[k].text = (unichar_t *) _("Validate Before Saving");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 8; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+24+6;
	if ( sf->multilayer || sf->strokedfont || sf->onlybitmaps )
	    gcd[k].gd.flags = gg_visible | gg_utf8_popup;
	else if ( old_validate )
	    gcd[k].gd.flags = (gg_enabled | gg_visible | gg_cb_on | gg_utf8_popup);
	else
	    gcd[k].gd.flags = (gg_enabled | gg_visible | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("Check the glyph outlines for standard errors before saving\nThis can be slow.");
	gcd[k++].creator = GCheckBoxCreate;
	hvarray[hvi++] = &gcd[k-1]; hvarray[hvi++] = GCD_ColSpan; hvarray[hvi++] = GCD_ColSpan;
	hvarray[hvi++] = NULL;

	label[k].text = (unichar_t *) _("Append a FONTLOG entry");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 8; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+24+6;
	gcd[k].gd.flags = (gg_enabled | gg_visible | gg_utf8_popup);
	if ( old_fontlog )
	    gcd[k].gd.flags |= gg_cb_on;
	gcd[k].gd.popup_msg = (unichar_t *) _("The FONTLOG allows you to keep a log of changes made to your font.");
	gcd[k].gd.cid = CID_AppendFontLog;
	gcd[k].gd.handle_controlevent = GFD_ToggleFontLog;
	gcd[k++].creator = GCheckBoxCreate;
	hvarray[hvi++] = &gcd[k-1]; hvarray[hvi++] = GCD_ColSpan; hvarray[hvi++] = GCD_ColSpan;
	hvarray[hvi++] = NULL;

	gcd[k].gd.flags = old_fontlog ? (gg_visible | gg_enabled | gg_textarea_wrap) : (gg_enabled|gg_textarea_wrap);
	gcd[k].gd.cid = CID_FontLogBit;
	gcd[k++].creator = GTextAreaCreate;
	hvarray[hvi++] = &gcd[k-1];
	hvarray[hvi++] = GCD_ColSpan; hvarray[hvi++] = GCD_ColSpan; hvarray[hvi++] = NULL;

	label[k].text = (unichar_t *) _("Prepend timestamp");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 8; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+24+6;
	gcd[k].gd.flags = (gg_enabled | gg_visible | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("This option prepends a timestamp in the format YYMMDDHHMM to the filename and font-family name metadata.");
	gcd[k].gd.cid = CID_PrependTimestamp;
	gcd[k].gd.handle_controlevent = GFD_TogglePrependTimestamp;
	gcd[k++].creator = GCheckBoxCreate; //???
	hvarray[hvi++] = &gcd[k-1]; hvarray[hvi++] = GCD_ColSpan; hvarray[hvi++] = GCD_ColSpan;
	hvarray[hvi++] = NULL;

	/* And OFLib uploads of families won't work because they don't accept dfonts */
	label[k].text = (unichar_t *) _("Upload to the");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 8; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+24+6;
	gcd[k].gd.flags = (gg_enabled | gg_visible | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("Once you have a final version of your font\nand if your font is licensed under the Open\nFont License, then you might consider uploading\nit to the Open Font Library. This is a website\nof Free/Libre fonts.\n\nYou must have previously registered with OFLib\nand have a valid username/password there.\n\nObviously you must be connected to the internet\nfor this to work.");
	gcd[k].gd.cid = CID_OFLibUpload;
	gcd[k].gd.handle_controlevent = GFD_ToggleOFLib;
	gcd[k++].creator = GCheckBoxCreate;
	oflibinfo[0] = &gcd[k-1];

	if ( small_blue_box.main_foreground==0 ) {
	    extern void _GButtonInit(void);
	    _GButtonInit();
	    small_blue_box = _GGadget_button_box;
	    small_blue_box.border_type = bt_box;
	    small_blue_box.border_shape = bs_rect;
	    small_blue_box.border_width = 0;
	    small_blue_box.flags = box_foreground_shadow_outer;
	    small_blue_box.padding = 0;
	    small_blue_box.main_foreground = 0x0000ff;
	    small_blue_box.border_darker = small_blue_box.main_foreground;
	    small_blue_box.border_darkest = small_blue_box.border_brighter =
		    small_blue_box.border_brightest =
		    small_blue_box.main_background = GDrawGetDefaultBackground(NULL);
	}

	label[k].text = (unichar_t *) _("Open Font Library");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.box = &small_blue_box;
	gcd[k].gd.flags = gg_enabled | gg_visible | gg_dontcopybox | gg_utf8_popup;
	gcd[k].gd.handle_controlevent = GFD_OFLibHelp;
	gcd[k].gd.popup_msg = (unichar_t *) "http://openfontlibrary.org/";
	gcd[k++].creator = GButtonCreate;
	oflibinfo[1] = &gcd[k-1];
	oflibinfo[2] = GCD_HPad10;

	label[k].text = (unichar_t *) _("Register");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled | gg_visible | gg_dontcopybox | gg_utf8_popup;
	gcd[k].gd.box = &small_blue_box;
	gcd[k].gd.handle_controlevent = GFD_OFLibRegister;
	gcd[k].gd.popup_msg = (unichar_t *) "http://openfontlibrary.org/media/register";
	gcd[k++].creator = GButtonCreate;
	oflibinfo[3] = &gcd[k-1];
	oflibinfo[4] = GCD_Glue;
	oflibinfo[5] = NULL;

	boxes[7].gd.flags = gg_enabled|gg_visible;
	boxes[7].gd.u.boxelements = oflibinfo;
	boxes[7].creator = GHBoxCreate;

	hvarray[hvi++] = &boxes[7]; hvarray[hvi++] = GCD_ColSpan; hvarray[hvi++] = GCD_ColSpan;
	hvarray[hvi++] = NULL;

	gcd[k].gd.flags = gg_enabled;
	gcd[k].gd.cid = CID_OFLibLine1;
	gcd[k++].creator = GLineCreate;
	hvarray[hvi++] = &gcd[k-1];
	hvarray[hvi++] = GCD_ColSpan; hvarray[hvi++] = GCD_ColSpan; hvarray[hvi++] = NULL;

	label[k].text = (unichar_t *) _("Username:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("Username on OFLib");
	gcd[k].gd.cid = CID_OFLibUsername + CID_OFLibLabOffset;
	gcd[k++].creator = GLabelCreate;
	oflarray[0][0] = &gcd[k-1];

	label[k].text = (unichar_t *) oflib_username;
	label[k].text_is_1byte = true;
	if ( oflib_username!=NULL )
	    gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("Username on OFLib");
	gcd[k].gd.cid = CID_OFLibUsername;
	gcd[k++].creator = GTextFieldCreate;
	oflarray[0][1] = &gcd[k-1];

	label[k].text = (unichar_t *) _("Password:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("Password on OFLib");
	gcd[k].gd.cid = CID_OFLibPassword + CID_OFLibLabOffset;
	gcd[k++].creator = GLabelCreate;
	oflarray[0][2] = &gcd[k-1];

	label[k].text = (unichar_t *) copy(oflib_password);
	if ( label[k].text!=NULL )
	    for ( oflpwd = (char *) label[k].text; *oflpwd!='\0'; ++oflpwd )
		*oflpwd ^= 0xf;		/* Simple encryption */
	oflpwd = (char *) label[k].text;
	label[k].text_is_1byte = true;
	if ( oflib_password!=NULL )
	    gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("Password on OFLib");
	gcd[k].gd.cid = CID_OFLibPassword;
	gcd[k++].creator = GPasswordCreate;
	oflarray[0][3] = &gcd[k-1];
	oflarray[0][4] = NULL;

	label[k].text = (unichar_t *) _("Remember Me");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = (oflib_username!=NULL && oflib_password!=NULL) ?
		(gg_enabled  | gg_cb_on | gg_utf8_popup) :
		(gg_enabled  | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("FontForge will remember your username/password\nby storing them as plain text (insecurely) in your preference file");
	gcd[k].gd.cid = CID_OFLibRememberMe;
	gcd[k++].creator = GCheckBoxCreate;
	oflarray[1][0] = GCD_Glue; oflarray[1][1] = &gcd[k-1];
	oflarray[1][2] = oflarray[1][3] = GCD_ColSpan;
	oflarray[1][4] = NULL;
	

	label[k].text = (unichar_t *) _("OFLib Name:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("Font name to display on OFLib");
	gcd[k].gd.cid = CID_OFLibName + CID_OFLibLabOffset;
	gcd[k++].creator = GLabelCreate;
	oflarray[2][0] = &gcd[k-1];

	label[k].text = (unichar_t *) (sf->fullname!=NULL ? sf->fullname : sf->fontname);
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("Font name to display on OFLib");
	gcd[k].gd.cid = CID_OFLibName;
	gcd[k++].creator = GTextFieldCreate;
	oflarray[2][1] = &gcd[k-1];

	label[k].text = (unichar_t *) _("Artists:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("Other artists involved in the design of this font.\nCollaborators, etc.");
	gcd[k].gd.cid = CID_OFLibArtists + CID_OFLibLabOffset;
	gcd[k++].creator = GLabelCreate;
	oflarray[2][2] = &gcd[k-1];

	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("Other artists involved in the design of this font.\nCollaborators, etc.");
	gcd[k].gd.cid = CID_OFLibArtists;
	gcd[k++].creator = GTextFieldCreate;
	oflarray[2][3] = &gcd[k-1];
	oflarray[2][4] = NULL;

	label[k].text = (unichar_t *) _("Keyword Tags:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("A comma separated list of keywords that describe\nthe font to help others search for it.\nYou may use whatever keyword tags you desire.\nSuggestions: serif, sans_serif, bold, italic, oblique,\nextended, compressed, thin, demibold, black, outline, regular,\ndisplay, etc.");
	gcd[k].gd.cid = CID_OFLibTags + CID_OFLibLabOffset;
	gcd[k++].creator = GLabelCreate;
	oflarray[3][0] = &gcd[k-1];

	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("A comma separated list of keywords that describe\nthe font to help others search for it.\nYou may use whatever keyword tags you desire.\nSuggestions: serif, sans_serif, bold, italic, oblique,\nextended, compressed, thin, demibold, black, outline,\nregular, display, etc.");
	gcd[k].gd.cid = CID_OFLibTags;
	gcd[k++].creator = GTextFieldCreate;
	oflarray[3][1] = &gcd[k-1]; oflarray[3][2] = oflarray[3][3] = GCD_ColSpan;
	oflarray[3][4] = NULL;

	label[k].text = (unichar_t *) _("Description:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("A description of the font");
	gcd[k].gd.cid = CID_OFLibDescription + CID_OFLibLabOffset;
	gcd[k++].creator = GLabelCreate;
	oflarray[4][0] = &gcd[k-1];

	gcd[k].gd.pos.height = 4*12;
	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup | gg_textarea_wrap);
	gcd[k].gd.popup_msg = (unichar_t *) _("A description of the font");
	gcd[k].gd.cid = CID_OFLibDescription;
	gcd[k++].creator = GTextAreaCreate;
	oflarray[4][1] = &gcd[k-1]; oflarray[4][2] = oflarray[4][3] = GCD_ColSpan;
	oflarray[4][4] = NULL;

	label[k].text = (unichar_t *) _("License:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("A description of the font");
	gcd[k].gd.cid = CID_OFLibOFL + CID_OFLibLabOffset;
	gcd[k++].creator = GLabelCreate;
	oflarray[5][0] = &gcd[k-1];

	label[k].text = (unichar_t *) _("SIL");
	label[k].image_precedes = false;
	label[k].image = &OFL_logo;
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup | gg_cb_on);
	gcd[k].gd.popup_msg = (unichar_t *) _("The SIL Open Font License\nPlease see http://scripts.sil.org/OFL");
	gcd[k].gd.cid = CID_OFLibOFL;
	gcd[k++].creator = GRadioCreate;
	oflarray[5][1] = &gcd[k-1]; oflarray[5][2] = GCD_ColSpan;

	label[k].text = (unichar_t *) _("Public Domain");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup );
	gcd[k].gd.popup_msg = (unichar_t *) _("Public Domain");
	gcd[k].gd.cid = CID_OFLibPD;
	gcd[k++].creator = GRadioCreate;
	oflarray[5][3] = &gcd[k-1]; oflarray[5][4] = NULL;

	label[k].text = (unichar_t *) _("Upload License");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled  | gg_utf8_popup;
	if ( HasLicense(sf,NULL) )
	    gcd[k].gd.flags |= gg_cb_on;
	gcd[k].gd.popup_msg = (unichar_t *) _("Upload the license (as found in the list of truetype names)");
	gcd[k].gd.cid = CID_UploadLicense;
	gcd[k++].creator = GCheckBoxCreate;
	oflarray[6][0] = GCD_Glue; oflarray[6][1] = &gcd[k-1];
	oflarray[6][2] = GCD_ColSpan; oflarray[6][3] = GCD_Glue;
	oflarray[6][4] = NULL;

	label[k].text = (unichar_t *) _("Upload FONTLOG");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled ;
	if ( sf->fontlog )
	    gcd[k].gd.flags |= gg_cb_on;
	gcd[k].gd.cid = CID_UploadFontLog;
	gcd[k++].creator = GCheckBoxCreate;
	oflarray[7][0] = GCD_Glue; oflarray[7][1] = &gcd[k-1];
	oflarray[7][2] = GCD_ColSpan; oflarray[7][3] = GCD_Glue;
	oflarray[7][4] = NULL;

	label[k].text = (unichar_t *) _("Preview:");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("An image of the font in use");
	gcd[k].gd.cid = CID_OFLibGenPreview + CID_OFLibLabOffset;
	gcd[k++].creator = GLabelCreate;
	oflarray[8][0] = &gcd[k-1];

	label[k].text = (unichar_t *) _("Generated Image");
	label[k].image_precedes = false;
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup | gg_cb_on);
	gcd[k].gd.popup_msg = (unichar_t *) _("FontForge will generate a sample image for you\nbefore uploading.");
	gcd[k].gd.cid = CID_OFLibGenPreview;
	gcd[k++].creator = GRadioCreate;
	parray[0][0] = &gcd[k-1]; parray[0][1] = GCD_ColSpan;

	label[k].text = (unichar_t *) _("No Preview");
	label[k].image_precedes = false;
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("No image will be uploaded.");
	gcd[k].gd.cid = CID_OFLibNoPreview;
	gcd[k++].creator = GRadioCreate;
	parray[0][2] = &gcd[k-1]; parray[0][3] = NULL;

	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup | gg_rad_continueold);
	gcd[k].gd.popup_msg = (unichar_t *) _("If you have already created a preview image of the font\nthen provide the pathspec here.");
	gcd[k].gd.cid = CID_OFLibDiskPreview;
	gcd[k++].creator = GRadioCreate;
	p2array[0] = &gcd[k-1];

	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("If you have already created a preview image of the font\nthen provide the pathspec here.");
	gcd[k].gd.cid = CID_OFLibPreviewText;
	gcd[k++].creator = GTextFieldCreate;
	p2array[1] = &gcd[k-1];

	label[k].text = (unichar_t *) _("...");
	label[k].image_precedes = false;
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = (gg_enabled  | gg_utf8_popup);
	gcd[k].gd.popup_msg = (unichar_t *) _("If you have already created a preview image of the font\nthen provide the pathspec here.");
	gcd[k].gd.cid = CID_OFLibPreviewBrowse;
	gcd[k].gd.handle_controlevent = GFD_OFLibPreviewBrowse;
	gcd[k++].creator = GButtonCreate;
	p2array[2] = &gcd[k-1]; p2array[3] = NULL;
	boxes[8].gd.flags = gg_enabled|gg_visible;
	boxes[8].gd.u.boxelements = p2array;
	boxes[8].creator = GHBoxCreate;

	parray[1][0] = &boxes[8];
	parray[1][1] = parray[1][2] = GCD_ColSpan;
	parray[1][3] = NULL;
	parray[2][0] = NULL;

	boxes[5].gd.flags = gg_enabled|gg_visible;
	boxes[5].gd.u.boxelements = parray[0];
	boxes[5].creator = GHVBoxCreate;
	oflarray[8][1] = &boxes[5]; oflarray[8][2] = oflarray[8][3] = GCD_ColSpan;
	oflarray[8][4] = NULL;

	label[k].text = (unichar_t *) _("Not Safe for Work");
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_enabled  | gg_utf8_popup;
	gcd[k].gd.popup_msg = (unichar_t *) _("If for some reason the font is deemed inappropriate\nfor a work environment.");
	gcd[k].gd.cid = CID_OFLibNotSafe;
	gcd[k++].creator = GCheckBoxCreate;
	oflarray[9][0] = GCD_Glue; oflarray[9][1] = &gcd[k-1];
	oflarray[9][2] = GCD_ColSpan; oflarray[9][3] = GCD_Glue;
	oflarray[9][4] = NULL;
	oflarray[10][0] = NULL;

	boxes[6].gd.flags = gg_enabled|gg_visible;
	boxes[6].gd.u.boxelements = oflarray[0];
	boxes[6].creator = GHVBoxCreate;
	hvarray[hvi++] = &boxes[6]; hvarray[hvi++] = GCD_ColSpan; hvarray[hvi++] = GCD_ColSpan;
	hvarray[hvi++] = NULL;

	gcd[k].gd.flags = gg_enabled;
	gcd[k].gd.cid = CID_OFLibLine2;
	gcd[k++].creator = GLineCreate;
	hvarray[hvi++] = &gcd[k-1];
	hvarray[hvi++] = GCD_ColSpan; hvarray[hvi++] = GCD_ColSpan; hvarray[hvi++] = NULL; hvarray[hvi++] = NULL;
    } else {
	hvarray[12] = NULL;
	oflpwd = NULL;
    }

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = hvarray;
    boxes[3].creator = GHVBoxCreate;
    varray[2] = GCD_Glue; varray[3] = NULL;
    varray[4] = &boxes[3]; varray[5] = NULL;
    varray[6] = GCD_Glue; varray[7] = NULL;
    varray[8] = GCD_Glue; varray[9] = NULL;
    varray[10] = &boxes[2]; varray[11] = NULL;
    varray[12] = NULL;

    if ( family ) {
	y = 276;

	f = 0;
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = y;
	gcd[k].gd.pos.width = totwid-5-5;
	gcd[k].gd.flags = gg_visible | gg_enabled ;
	gcd[k++].creator = GLineCreate;
	famarray[f++] = &gcd[k-1]; famarray[f++] = GCD_ColSpan; famarray[f++] = NULL;
	y += 7;

	for ( i=0, fc=0, j=1; i<familycnt && j<48 ; ++i ) {
	    while ( fc<fondcnt ) {
		while ( j<48 && familysfs[fc][j]==NULL ) ++j;
		if ( j!=48 )
	    break;
		++fc;
		j=0;
	    }
	    if ( fc==fondcnt )
	break;
	    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = y;
	    gcd[k].gd.pos.width = gcd[8].gd.pos.x-gcd[k].gd.pos.x-5;
	    gcd[k].gd.flags = gg_visible | gg_enabled | gg_cb_on ;
	    label[k].text = (unichar_t *) (familysfs[fc][j]->fontname);
	    label[k].text_is_1byte = true;
	    gcd[k].gd.label = &label[k];
	    gcd[k].gd.cid = CID_Family+i*10;
	    gcd[k].data = familysfs[fc][j];
	    gcd[k].gd.popup_msg = uStyleName(familysfs[fc][j]);
	    gcd[k++].creator = GCheckBoxCreate;
	    famarray[f++] = &gcd[k-1];

	    gcd[k].gd.pos.x = gcd[8].gd.pos.x; gcd[k].gd.pos.y = y; gcd[k].gd.pos.width = gcd[8].gd.pos.width;
	    gcd[k].gd.flags = gg_visible | gg_enabled;
	    if ( old==bf_none )
		gcd[k].gd.flags &= ~gg_enabled;
	    temp = familysfs[fc][j]->cidmaster ? familysfs[fc][j]->cidmaster : familysfs[fc][j];
	    label[k].text = BitmapList(temp);
	    gcd[k].gd.label = &label[k];
	    gcd[k].gd.cid = CID_Family+i*10+1;
	    gcd[k].data = familysfs[fc][j];
	    gcd[k++].creator = GTextFieldCreate;
	    famarray[f++] = &gcd[k-1]; famarray[f++] = NULL;
	    y+=26;
	    ++j;
	}

	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = y;
	gcd[k].gd.pos.width = totwid-5-5;
	gcd[k].gd.flags = gg_visible | gg_enabled ;
	gcd[k++].creator = GLineCreate;
	famarray[f++] = &gcd[k-1]; famarray[f++] = GCD_ColSpan; famarray[f++] = NULL;
	if ( family == gf_ttc ) {
	    gcd[k].gd.flags = gg_visible | gg_enabled | gg_cb_on | gg_utf8_popup ;
	    label[k].text = (unichar_t *) _("Merge tables across fonts");
	    label[k].text_is_1byte = true;
	    gcd[k].gd.label = &label[k];
	    gcd[k].gd.cid = CID_MergeTables;
	    gcd[k].gd.popup_msg = (unichar_t *) copy(_(
		"FontForge can generate two styles of ttc file.\n"
		"In the first each font is a separate entity\n"
		"with no connection to other fonts. In the second\n"
		"FontForge will attempt to use the same glyph table\n"
		"for all fonts, merging duplicate glyphs. It will\n"
		"also attempt to use the same space for tables in\n"
		"different fonts which are bit by bit the same.\n\n"
		"FontForge isn't always able to perform a merge, in\n"
		"which case it falls back on generating independent\n"
		"fonts within the ttc.\n"
		" FontForge cannot merge if:\n"
		"  * The fonts have different em-sizes\n"
		"  * Bitmaps are involved\n"
		"  * The merged glyf table has more than 65534 glyphs\n\n"
		"(Merging will take longer)" ));
	    gcd[k++].creator = GCheckBoxCreate;
	    famarray[f++] = &gcd[k-1]; famarray[f++] = GCD_ColSpan; famarray[f++] = NULL;

	    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup ;
	    label[k].text = (unichar_t *) _("As CFF fonts");
	    label[k].text_is_1byte = true;
	    gcd[k].gd.label = &label[k];
	    gcd[k].gd.cid = CID_TTC_CFF;
	    gcd[k].gd.popup_msg = (unichar_t *) copy(_(
		"Put CFF fonts into the ttc rather than TTF.\n These seem to work on the mac and linux\n but are documented not to work on Windows." ));
	    gcd[k++].creator = GCheckBoxCreate;
	    famarray[f++] = &gcd[k-1]; famarray[f++] = GCD_ColSpan; famarray[f++] = NULL;
	}
	famarray[f++] = NULL;

	free(familysfs);

	boxes[4].gd.flags = gg_enabled|gg_visible;
	boxes[4].gd.u.boxelements = famarray;
	boxes[4].creator = GHVBoxCreate;
	varray[6] = &boxes[4];
    }

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);
    
    GGadgetSetUserData(gcd[2].ret,gcd[0].ret);
    free(namelistnames);
    free(lynames);
    free(label[9].text);
    if ( family ) {
	for ( i=13; i<k; ++i ) if ( gcd[i].gd.popup_msg!=NULL )
	    free((unichar_t *) gcd[i].gd.popup_msg);
    } else {
	GHVBoxSetExpandableCol(boxes[6].ret,1);
	GHVBoxSetExpandableCol(boxes[7].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[8].ret,1);
    }

    GFileChooserConnectButtons(gcd[0].ret,gcd[1].ret,gcd[2].ret);
    {
	SplineFont *master = sf->cidmaster ? sf->cidmaster : sf;
	char *fn = master->defbasefilename!=NULL ? master->defbasefilename :
		master->fontname;
	unichar_t *temp = galloc(sizeof(unichar_t)*(strlen(fn)+30));
	uc_strcpy(temp,fn);
	uc_strcat(temp,savefont_extensions[ofs]!=NULL?savefont_extensions[ofs]:bitmapextensions[old]);
	GGadgetSetTitle(gcd[0].ret,temp);
	free(temp);
    }
    free(oflpwd);
    GFileChooserGetChildren(gcd[0].ret,&pulldown,&files,&tf);
    GWidgetIndicateFocusGadget(tf);
#if __Mac
	/* The name of the postscript file is fixed and depends solely on */
	/*  the font name. If the user tried to change it, the font would */
	/*  not be found */
	GGadgetSetVisible(tf,ofs!=ff_pfbmacbin);
#endif

    d.sf = sf;
    d.map = map;
    d.family = family;
    d.familycnt = familycnt-1;		/* Don't include the "normal" instance */
    d.gfc = gcd[0].ret;
    d.pstype = gcd[6].ret;
    d.bmptype = gcd[8].ret;
    d.bmpsizes = gcd[9].ret;
    d.rename = gcd[rk].ret;
    d.validate = family ? NULL : gcd[vk].ret;
    d.gw = gw;

    d.ps_flags = old_ps_flags;
    d.sfnt_flags = old_sfnt_flags;
    d.psotb_flags = old_ps_flags | (old_psotb_flags&~ps_flag_mask);

    GFD_FigureWhich(&d);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
#ifdef HAVE_PTHREAD_H
    if ( d.thread_active ) {
	d.please_die_thread = true;
	pthread_join(d.validate_thread,NULL);
	d.thread_active = false;
    }
#endif
return(d.ret);
}
