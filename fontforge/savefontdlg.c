/* Copyright (C) 2000-2005 by George Williams */
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
#include "gfile.h"
#include "gresource.h"
#include "utype.h"
#include "gio.h"
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "gicons.h"
#include <gkeysym.h>
#include "psfont.h"

static int nfnt_warned = false, post_warned = false;

#define CID_Family	2000

#define CID_OK		1001
#define CID_PS_AFM		1002
#define CID_PS_PFM		1003
#define CID_PS_TFM		1004
#define CID_PS_HintSubs		1005
#define CID_PS_Flex		1006
#define CID_PS_Hints		1007
#define CID_PS_Restrict256	1008
#define CID_PS_Round		1009
#define CID_TTF_Hints		1101
#define CID_TTF_FullPS		1102
#define CID_TTF_AppleMode	1103
#define CID_TTF_PfEdComments	1104
#define CID_TTF_PfEdColors	1105
#define CID_TTF_PfEd		1106
#define CID_TTF_TeXTable	1107
#define CID_TTF_OpenTypeMode	1108
#define CID_TTF_GlyphMap	1109


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
    int ps_flags;		/* The ordering of these flags fields is */
    int ttf_flags;		/*  important. We index into them */
    int otf_flags;		/*  don't reorder or put junk in between */
    int psotb_flags;
    uint8 optset[3];
    SplineFont *sf;
    EncMap *map;
};
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

#if __Mac
static char *extensions[] = { ".pfa", ".pfb", ".res", "%s.pfb", ".pfa", ".pfb", ".pt3", ".ps",
	".cid", ".cff", ".cid.cff",
	".t42", ".cid.t42",
	".ttf", ".ttf", ".suit", ".dfont", ".otf", ".otf.dfont", ".otf",
	".otf.dfont", ".svg", NULL };
# ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static char *bitmapextensions[] = { ".*bdf", ".ttf", ".dfont", ".bmap", ".dfont", ".*fnt", ".otb", ".pdb", ".none", NULL };
# endif
#else
static char *extensions[] = { ".pfa", ".pfb", ".bin", "%s.pfb", ".pfa", ".pfb", ".pt3", ".ps",
	".cid", ".cff", ".cid.cff",
	".t42", ".cid.t42",
	".ttf", ".ttf", ".ttf.bin", ".dfont", ".otf", ".otf.dfont", ".otf",
	".otf.dfont", ".svg", NULL };
# ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static char *bitmapextensions[] = { ".*bdf", ".ttf", ".dfont", ".bmap.bin", ".*fnt", ".otb", ".pdb", ".none", NULL };
# endif
#endif
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static GTextInfo formattypes[] = {
    { (unichar_t *) "PS Type 1 (Ascii)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "PS Type 1 (Binary)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
#if __Mac
    { (unichar_t *) "PS Type 1 (Resource)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
#else
    { (unichar_t *) "PS Type 1 (MacBin)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
#endif
    { (unichar_t *) "PS Type 1 (Multiple)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "PS Multiple Master(A)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "PS Multiple Master(B)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "PS Type 3", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "PS Type 0", NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "PS CID", NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "CFF (Bare)", NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "CFF CID (Bare)", NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "Type42", NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "Type42 CID", NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "True Type", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "True Type (Symbol)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
#if __Mac
    { (unichar_t *) "True Type (Resource)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
#else
    { (unichar_t *) "True Type (MacBin)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
#endif
    { (unichar_t *) "True Type (Mac dfont)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "Open Type (CFF)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "Open Type (Mac dfont)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "Open Type CID", NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "Open Type CID (dfont)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "SVG font", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) _STR_Nooutlinefont, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { NULL }
};
static GTextInfo bitmaptypes[] = {
    { (unichar_t *) "BDF", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "In TTF", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "sbits only (dfont)", NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 0, 1 },
#if __Mac
    { (unichar_t *) "NFNT (Resource)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
#else
    { (unichar_t *) "NFNT (MacBin)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
#endif
/* OS/X doesn't seem to support NFNTs, so there's no point in putting them in a dfont */
/*  { (unichar_t *) "NFNT (dfont)", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },*/
    { (unichar_t *) "Win FNT", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "OpenType Bitmap", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) "Palm OS Bitmap", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1 },
    { (unichar_t *) _STR_Nobitmapfonts, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1 },
    { NULL }
};
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

#if __Mac
int old_ttf_flags = ttf_flag_applemode;
int old_otf_flags = ttf_flag_applemode;
#else
int old_ttf_flags = ttf_flag_otmode;
int old_otf_flags = ttf_flag_otmode;
#endif
int old_ps_flags = ps_flag_afm|ps_flag_round;
int old_psotb_flags = ps_flag_afm;

int oldformatstate = ff_pfb;
int oldbitmapstate = 0;
extern int alwaysgenapple, alwaysgenopentype;

static const char *pfaeditflag = "SplineFontDB:";

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
int32 *ParseBitmapSizes(GGadget *g,int msg,int *err) {
    const unichar_t *val = _GGadgetGetTitle(g), *pt; unichar_t *end, *end2;
    int i;
    int32 *sizes;

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
	if ( msg!=_STR_PixelList )
	    /* No bit depth allowed */;
	else if ( *end!='@' )
	    sizes[i] |= 0x10000;
	else
	    sizes[i] |= (u_strtol(end+1,&end,10)<<16);
	if ( sizes[i]>0 ) ++i;
	if ( *end!=' ' && *end!=',' && *end!='\0' ) {
	    free(sizes);
	    ProtestR(msg);
	    *err = true;
return( NULL );
	}
	while ( *end==' ' || *end==',' ) ++end;
	pt = end;
    }
    sizes[i] = 0;
return( sizes );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static int WriteAfmFile(char *filename,SplineFont *sf, int formattype, EncMap *map) {
    char *buf = galloc(strlen(filename)+6), *pt, *pt2;
    FILE *afm;
    int ret;
#if defined(FONTFORGE_CONFIG_GDRAW)
    unichar_t *temp;
#endif
    int subtype = formattype;

    if ( (formattype==ff_mma || formattype==ff_mmb) && sf->mm!=NULL ) {
	sf = sf->mm->normal;
	subtype = ff_pfb;
    }

    strcpy(buf,filename);
    pt = strrchr(buf,'.');
    if ( pt!=NULL && (pt2=strrchr(buf,'/'))!=NULL && pt<pt2 )
	pt = NULL;
    if ( pt==NULL )
	strcat(buf,".afm");
    else
	strcpy(pt,".afm");
#if defined(FONTFORGE_CONFIG_GDRAW)
    GProgressChangeLine1R(_STR_SavingAFM);
    GProgressChangeLine2(temp=uc_copy(buf)); free(temp);
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_change_line1(_("Saving AFM File"));
    gwwv_progress_change_line2(buf);
#endif
    afm = fopen(buf,"w");
    free(buf);
    if ( afm==NULL )
return( false );
    ret = AfmSplineFont(afm,sf,subtype,map);
    if ( fclose(afm)==-1 )
return( false );
    if ( !ret )
return( false );

    if ( (formattype==ff_mma || formattype==ff_mmb) && sf->mm!=NULL ) {
	MMSet *mm = sf->mm;
	int i;
	for ( i=0; i<mm->instance_count; ++i ) {
	    sf = mm->instances[i];
	    buf = galloc(strlen(filename)+strlen(sf->fontname)+4+1);
	    strcpy(buf,filename);
	    pt = strrchr(buf,'/');
	    if ( pt==NULL ) pt = buf;
	    else ++pt;
	    strcpy(pt,sf->fontname);
	    strcat(pt,".afm");
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GProgressChangeLine2(temp=uc_copy(buf)); free(temp);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_progress_change_line2(buf);
#endif
	    afm = fopen(buf,"w");
	    free(buf);
	    if ( afm==NULL )
return( false );
	    ret = AfmSplineFont(afm,sf,subtype,map);
	    if ( fclose(afm)==-1 )
return( false );
	    if ( !ret )
return( false );
	}
	buf = galloc(strlen(filename)+8);

	strcpy(buf,filename);
	pt = strrchr(buf,'.');
	if ( pt!=NULL && (pt2=strrchr(buf,'/'))!=NULL && pt<pt2 )
	    pt = NULL;
	if ( pt==NULL )
	    strcat(buf,".amfm");
	else
	    strcpy(pt,".amfm");
#if defined(FONTFORGE_CONFIG_GDRAW)
	GProgressChangeLine2(temp=uc_copy(buf)); free(temp);
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_change_line2(buf);
#endif
	afm = fopen(buf,"w");
	free(buf);
	if ( afm==NULL )
return( false );
	ret = AmfmSplineFont(afm,mm,formattype,map);
	if ( fclose(afm)==-1 )
return( false );
    }
return( ret );
}

static int WriteTfmFile(char *filename,SplineFont *sf, int formattype, EncMap *map) {
    char *buf = galloc(strlen(filename)+6), *pt, *pt2;
    FILE *tfm, *enc;
    int ret;
#if defined(FONTFORGE_CONFIG_GDRAW)
    unichar_t *temp;
#endif
    int i;
    char *encname;

    strcpy(buf,filename);
    pt = strrchr(buf,'.');
    if ( pt!=NULL && (pt2=strrchr(buf,'/'))!=NULL && pt<pt2 )
	pt = NULL;
    if ( pt==NULL )
	strcat(buf,".tfm");
    else
	strcpy(pt,".tfm");
#if defined(FONTFORGE_CONFIG_GDRAW)
    GProgressChangeLine1R(_STR_SavingTFM);
    GProgressChangeLine2(temp=uc_copy(buf)); free(temp);
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_change_line1(_("Saving TFM File"));
    gwwv_progress_change_line2(buf);
#endif
    tfm = fopen(buf,"wb");
    if ( tfm==NULL )
return( false );
    ret = TfmSplineFont(tfm,sf,formattype,map);
    if ( fclose(tfm)==-1 )
	ret = 0;

    pt = strrchr(buf,'.');
    strcpy(pt,".enc");
    enc = fopen(buf,"wb");
    free(buf);
    if ( enc==NULL )
return( false );

    encname=NULL;
    if ( sf->subfontcnt==0 && map->enc!=&custom )
	encname = EncodingName(map->enc );
    if ( encname==NULL )
	fprintf( enc, "/%s-Enc [\n", sf->fontname );
    else
	fprintf( enc, "/%s [\n", encname );
    for ( i=0; i<map->enccount && i<256; ++i ) {
	if ( map->map[i]==-1 || !SCWorthOutputting(sf->glyphs[map->map[i]]) )
	    fprintf( enc, " /.notdef" );
	else
	    fprintf( enc, " /%s", sf->glyphs[map->map[i]]->name );
	if ( (i&0xf)==0 )
	    fprintf( enc, "\t\t%% 0x%02x", i );
	putc('\n',enc);
    }
    while ( i<256 ) {
	fprintf( enc, " /.notdef" );
	if ( (i&0xf0)==0 )
	    fprintf( enc, "\t\t%% 0x%02x", i );
	putc('\n',enc);
	++i;
    }
    fprintf( enc, "] def\n" );

    if ( fclose(enc)==-1 )
	ret = 0;
return( ret );
}

#ifndef FONTFORGE_CONFIG_WRITE_PFM
static
#endif
int WritePfmFile(char *filename,SplineFont *sf, int type0, EncMap *map) {
    char *buf = galloc(strlen(filename)+6), *pt, *pt2;
    FILE *pfm;
    int ret;
#if defined(FONTFORGE_CONFIG_GDRAW)
    unichar_t *temp;
#endif

    strcpy(buf,filename);
    pt = strrchr(buf,'.');
    if ( pt!=NULL && (pt2=strrchr(buf,'/'))!=NULL && pt<pt2 )
	pt = NULL;
    if ( pt==NULL )
	strcat(buf,".pfm");
    else
	strcpy(pt,".pfm");
#if defined(FONTFORGE_CONFIG_GDRAW)
    GProgressChangeLine2(temp=uc_copy(buf)); free(temp);
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_change_line2(buf);
#endif
    pfm = fopen(buf,"wb");
    free(buf);
    if ( pfm==NULL )
return( false );
    ret = PfmSplineFont(pfm,sf,type0,map);
    if ( fclose(pfm)==-1 )
return( 0 );
return( ret );
}
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI

static int OPT_PSHints(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GWindow gw = GGadgetGetWindow(g);
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetIsChecked(g)) {
	    int flags = (&d->ps_flags)[d->sod_which];
	    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_HintSubs),true);
	    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_Flex),true);
	    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_HintSubs),!(flags&ps_flag_nohintsubs));
	    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_Flex),!(flags&ps_flag_noflex));
	} else {
	    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_HintSubs),false);
	    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_Flex),false);
	    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_HintSubs),false);
	    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_Flex),false);
	}
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
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_PFM)) )
		    d->ps_flags |= ps_flag_pfm;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_TFM)) )
		    d->ps_flags |= ps_flag_tfm;
		if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_HintSubs)) )
		    d->ps_flags |= ps_flag_nohintsubs;
		if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_Flex)) )
		    d->ps_flags |= ps_flag_noflex;
		if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_Hints)) )
		    d->ps_flags |= ps_flag_nohints;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_Round)) )
		    d->ps_flags |= ps_flag_round;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_Restrict256)) )
		    d->ps_flags |= ps_flag_restrict256;
	    } else if ( d->sod_which==1 ) {	/* TrueType */
		d->ttf_flags = 0;
		if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_Hints)) )
		    d->ttf_flags |= ttf_flag_nohints;
		if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_FullPS)) )
		    d->ttf_flags |= ttf_flag_shortps;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_AppleMode)) )
		    d->ttf_flags |= ttf_flag_applemode;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_OpenTypeMode)) )
		    d->ttf_flags |= ttf_flag_otmode;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_PfEdComments)) )
		    d->ttf_flags |= ttf_flag_pfed_comments;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_PfEdColors)) )
		    d->ttf_flags |= ttf_flag_pfed_colors;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_TeXTable)) )
		    d->ttf_flags |= ttf_flag_TeXtable;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_GlyphMap)) )
		    d->ttf_flags |= ttf_flag_glyphmap;
	    } else if ( d->sod_which==2 ) {				/* OpenType */
		d->otf_flags = 0;
		if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_FullPS)) )
		    d->otf_flags |= ttf_flag_shortps;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_AppleMode)) )
		    d->otf_flags |= ttf_flag_applemode;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_OpenTypeMode)) )
		    d->otf_flags |= ttf_flag_otmode;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_PfEdComments)) )
		    d->otf_flags |= ttf_flag_pfed_comments;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_PfEdColors)) )
		    d->otf_flags |= ttf_flag_pfed_colors;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_TeXTable)) )
		    d->otf_flags |= ttf_flag_TeXtable;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_GlyphMap)) )
		    d->otf_flags |= ttf_flag_glyphmap;

		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_AFM)) )
		    d->otf_flags |= ps_flag_afm;
		if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_HintSubs)) )
		    d->otf_flags |= ps_flag_nohintsubs;
		if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_Flex)) )
		    d->otf_flags |= ps_flag_noflex;
		if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_Hints)) )
		    d->otf_flags |= ps_flag_nohints;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_Round)) )
		    d->otf_flags |= ps_flag_round;
	    } else {				/* PS + OpenType Bitmap */
		d->ps_flags = d->psotb_flags = 0;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_AFM)) )
		     d->psotb_flags = d->ps_flags |= ps_flag_afm;
		if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_PS_HintSubs)) )
		     d->psotb_flags = d->ps_flags |= ps_flag_nohintsubs;
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
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_TeXTable)) )
		    d->psotb_flags |= ttf_flag_TeXtable;
		if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TTF_GlyphMap)) )
		    d->psotb_flags |= ttf_flag_glyphmap;
	    }
	    d->sod_invoked = true;
	}
	d->sod_done = true;
    }
return( true );
}

static void OptSetDefaults(GWindow gw,struct gfc_data *d,int which,int iscid) {
    int flags = (&d->ps_flags)[which];
    int fs = GGadgetGetFirstListSelectedItem(d->pstype);
    int bf = GGadgetGetFirstListSelectedItem(d->bmptype);

    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_Hints),!(flags&ps_flag_nohints));
    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_HintSubs),!(flags&ps_flag_nohintsubs));
    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_Flex),!(flags&ps_flag_noflex));
    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_Restrict256),flags&ps_flag_restrict256);
    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_Round),flags&ps_flag_round);

    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_AFM),flags&ps_flag_afm);
    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_PFM),(flags&ps_flag_pfm) && !iscid);
    GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_TFM),flags&ps_flag_tfm);

    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_Hints),!(flags&ttf_flag_nohints));
    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_FullPS),!(flags&ttf_flag_shortps));
    if ( d->optset[which] )
	GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_AppleMode),flags&ttf_flag_applemode);
    else if ( which==0 || which==3 )	/* Postscript */
	GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_AppleMode),false);
    else if ( alwaysgenapple ||
	    fs==ff_ttfmacbin || fs==ff_ttfdfont || fs==ff_otfdfont ||
	    fs==ff_otfciddfont || d->family || (fs==ff_none && bf==bf_sfnt_dfont))
	GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_AppleMode),true);
    else
	GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_AppleMode),false);
    if ( d->optset[which] )
	GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_OpenTypeMode),flags&ttf_flag_otmode);
    else if ( which==0 )	/* Postscript */
	GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_OpenTypeMode),false);
    else if ( (fs==ff_ttfmacbin || fs==ff_ttfdfont || fs==ff_otfdfont ||
	     fs==ff_otfciddfont || d->family || (fs==ff_none && bf==bf_sfnt_dfont)))
	GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_OpenTypeMode),false);
    else
	GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_OpenTypeMode),alwaysgenopentype);

    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_PfEdComments),flags&ttf_flag_pfed_comments);
    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_PfEdColors),flags&ttf_flag_pfed_colors);
    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_TeXTable),flags&ttf_flag_TeXtable);
    GGadgetSetChecked(GWidgetGetControl(gw,CID_TTF_GlyphMap),flags&ttf_flag_glyphmap);

    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_Hints),which!=1);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_HintSubs),which!=1);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_Flex),which!=1);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_Round),which!=1);
    if ( which!=1 && (flags&ps_flag_nohints)) {
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_HintSubs),false);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_Flex),false);
	GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_HintSubs),false);
	GGadgetSetChecked(GWidgetGetControl(gw,CID_PS_Flex),false);
    }
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_Restrict256),(which==0 || which==3) && !iscid);

    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_AFM),which!=1);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_PFM),which==0 || which==3);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_PS_TFM),which==0 || which==3);

    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_Hints),which==1);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_FullPS),which!=0);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_AppleMode),which!=0 && which!=3);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_OpenTypeMode),which!=0 && which!=3);

    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_PfEd),which!=0);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_PfEdComments),which!=0);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_PfEdColors),which!=0);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_TeXTable),which!=0);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_TTF_GlyphMap),which!=0);

    d->optset[which] = true;
}

#define OPT_Width	230
#define OPT_Height	205

static void SaveOptionsDlg(struct gfc_data *d,int which,int iscid) {
    int flags;
    int k,group,group2;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[26];
    GTextInfo label[26];
    GRect pos;

    d->sod_done = false;
    d->sod_which = which;
    flags = (&d->ps_flags)[which];

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(_STR_Options,NULL);
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,OPT_Width));
    pos.height = GDrawPointsToPixels(NULL,OPT_Height);
    gw = GDrawCreateTopWindow(NULL,&pos,sod_e_h,d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    k = 0;
    gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width-4; gcd[k].gd.pos.height = pos.height-4;
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[k++].creator = GGroupCreate;

    label[k].text = (unichar_t *) _STR_PostScript;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 8; gcd[k].gd.pos.y = 5;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;

    group = k;
    gcd[k].gd.pos.x = 4; gcd[k].gd.pos.y = 9;
    gcd[k].gd.pos.width = OPT_Width-8; gcd[k].gd.pos.height = 58;
    gcd[k].gd.flags = gg_enabled | gg_visible ;
    gcd[k++].creator = GGroupCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = 16;
    gcd[k].gd.flags = gg_visible ;
    label[k].text = (unichar_t *) _STR_PSRound;
    label[k].text_in_resource = true;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_PSRoundPopup,NULL);
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_PS_Round;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible ;
    label[k].text = (unichar_t *) _STR_GenerateHints;
    label[k].text_in_resource = true;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_PSHintsPopup,NULL);
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = OPT_PSHints;
    gcd[k].gd.cid = CID_PS_Hints;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+4; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible ;
    label[k].text = (unichar_t *) _STR_FlexHints;
    label[k].text_in_resource = true;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_FlexHintsPopup,NULL);
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_PS_Flex;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = 0 ;
    label[k].text = (unichar_t *) _STR_HintSubs;
    label[k].text_in_resource = true;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_HintSubsPopup,NULL);
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_PS_HintSubs;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = 0 ;
    label[k].text = (unichar_t *) _STR_First256;
    label[k].text_in_resource = true;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_First256Popup,NULL);
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_PS_Restrict256;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = 110; gcd[k].gd.pos.y = gcd[k-5].gd.pos.y;
    gcd[k].gd.flags = gg_visible ;
    label[k].text = (unichar_t *) _STR_Outputafm;
    label[k].text_in_resource = true;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_OutputAfmPopup,NULL);
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_PS_AFM;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-5].gd.pos.y;
    gcd[k].gd.flags = gg_visible ;
    label[k].text = (unichar_t *) _STR_Outputpfm;
    label[k].text_in_resource = true;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_OutputPfmPopup,NULL);
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_PS_PFM;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-5].gd.pos.y;
    gcd[k].gd.flags = gg_visible ;
    label[k].text = (unichar_t *) _STR_Outputtfm;
    label[k].text_in_resource = true;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_OutputTfmPopup,NULL);
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_PS_TFM;
    gcd[k++].creator = GCheckBoxCreate;


    label[k].text = (unichar_t *) _STR_TrueType;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 8; gcd[k].gd.pos.y = gcd[group].gd.pos.y+gcd[group].gd.pos.height+6;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;

    group2 = k;
    gcd[k].gd.pos.x = 4; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+4;
    gcd[k].gd.pos.width = OPT_Width-8; gcd[k].gd.pos.height = 86;
    gcd[k].gd.flags = gg_enabled | gg_visible ;
    gcd[k++].creator = GGroupCreate;

    gcd[k].gd.pos.x = gcd[group+1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+8;
    gcd[k].gd.flags = gg_visible ;
    label[k].text = (unichar_t *) _STR_GenerateHints;
    label[k].text_in_resource = true;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_TTFHintsPopup,NULL);
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_Hints;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible ;
    label[k].text = (unichar_t *) _STR_PSNames;
    label[k].text_in_resource = true;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_PSNamesPopup,NULL);
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_FullPS;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible ;
    label[k].text = (unichar_t *) _STR_AppleMode;
    label[k].text_in_resource = true;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_AppleModePopup,NULL);
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_AppleMode;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible ;
    label[k].text = (unichar_t *) _STR_OpenTypeMode;
    label[k].text_in_resource = true;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_OpenTypeModePopup,NULL);
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_OpenTypeMode;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = gcd[group+6].gd.pos.x; gcd[k].gd.pos.y = gcd[k-4].gd.pos.y;
    gcd[k].gd.flags = gg_visible ;
    label[k].text = (unichar_t *) _STR_PfaEditTable;
    label[k].text_in_resource = true;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_PfaEditTablePopup,NULL);
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_PfEd;
    gcd[k++].creator = GLabelCreate;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+2; gcd[k].gd.pos.y = gcd[k-4].gd.pos.y-4;
    gcd[k].gd.flags = gg_visible ;
    label[k].text = (unichar_t *) _STR_PfEdComments;
    label[k].text_in_resource = true;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_PfEdCommentsPopup,NULL);
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_PfEdComments;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-4].gd.pos.y-4;
    gcd[k].gd.flags = gg_visible ;
    label[k].text = (unichar_t *) _STR_PfEdColors;
    label[k].text_in_resource = true;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_PfEdColorsPopup,NULL);
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_PfEdColors;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = gcd[k-3].gd.pos.x; gcd[k].gd.pos.y = gcd[k-4].gd.pos.y;
    gcd[k].gd.flags = gg_visible ;
    label[k].text = (unichar_t *) _STR_TeXTable;
    label[k].text_in_resource = true;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_TeXTablePopup,NULL);
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_TeXTable;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_visible ;
    label[k].text = (unichar_t *) _STR_OutputGlyphMap;
    label[k].text_in_resource = true;
    gcd[k].gd.popup_msg = GStringGetResource(_STR_PrefsPopupG2N,NULL);
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_TTF_GlyphMap;
    gcd[k++].creator = GCheckBoxCreate;

    gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = gcd[group2].gd.pos.y+gcd[group2].gd.pos.height+10-3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _STR_OK;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_OK;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _STR_Cancel;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k++].creator = GButtonCreate;

    GGadgetsCreate(gw,gcd);

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

static int AskResolution(int bf) {
    GRect pos;
    static GWindow bdf_gw, fon_gw;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[10];
    GTextInfo label[10];
    int done=-3;

    if ( no_windowing_ui )
return( -1 );

    gw = bf==bf_bdf ? bdf_gw : fon_gw;
    if ( gw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
#if defined(FONTFORGE_CONFIG_GDRAW)
	wattrs.window_title = GStringGetResource(_STR_BDFResolution,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
	wattrs.window_title = _("BDF Resolution");
#endif
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

	label[0].text = (unichar_t *) _STR_BDFResolution;
	label[0].text_in_resource = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 7; 
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].creator = GLabelCreate;

	label[1].text = (unichar_t *) (bf==bf_bdf ? "75" : "96");
	label[1].text_is_1byte = true;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.mnemonic = (bf==bf_bdf ? '7' : '9');
	gcd[1].gd.pos.x = 20; gcd[1].gd.pos.y = 13+7;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].gd.cid = 75;
	gcd[1].creator = GRadioCreate;

	label[2].text = (unichar_t *) (bf==bf_bdf ? "100" : "120");
	label[2].text_is_1byte = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.mnemonic = '1';
	gcd[2].gd.pos.x = 20; gcd[2].gd.pos.y = gcd[1].gd.pos.y+17; 
	gcd[2].gd.flags = gg_enabled|gg_visible;
	gcd[2].gd.cid = 100;
	gcd[2].creator = GRadioCreate;

	label[3].text = (unichar_t *) _STR_Guess;
	label[3].text_in_resource = true;
	gcd[3].gd.label = &label[3];
	gcd[3].gd.pos.x = 20; gcd[3].gd.pos.y = gcd[2].gd.pos.y+17;
	gcd[3].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[3].gd.cid = -1;
#if defined(FONTFORGE_CONFIG_GDRAW)
	gcd[3].gd.popup_msg = GStringGetResource(_STR_GuessResPopup,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
	gcd[3].gd.popup_msg = _("Guess each font's resolution based on its pixel size");
#endif
	gcd[3].creator = GRadioCreate;

	label[4].text = (unichar_t *) _STR_Other_;
	label[4].text_in_resource = true;
	gcd[4].gd.label = &label[4];
	gcd[4].gd.pos.x = 20; gcd[4].gd.pos.y = gcd[3].gd.pos.y+17;
	gcd[4].gd.flags = gg_enabled|gg_visible;
	gcd[4].gd.cid = 1004;
	gcd[4].creator = GRadioCreate;

	label[5].text = (unichar_t *) (bf == bf_bdf ? "96" : "72");
	label[5].text_is_1byte = true;
	gcd[5].gd.label = &label[5];
	gcd[5].gd.pos.x = 70; gcd[5].gd.pos.y = gcd[4].gd.pos.y-3; gcd[5].gd.pos.width = 40;
	gcd[5].gd.flags = gg_enabled|gg_visible;
	gcd[5].gd.cid = 1003;
	gcd[5].creator = GTextFieldCreate;

	gcd[6].gd.pos.x = 15-3; gcd[6].gd.pos.y = gcd[4].gd.pos.y+24;
	gcd[6].gd.pos.width = -1; gcd[6].gd.pos.height = 0;
	gcd[6].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[6].text = (unichar_t *) _STR_OK;
	label[6].text_in_resource = true;
	gcd[6].gd.mnemonic = 'O';
	gcd[6].gd.label = &label[6];
	gcd[6].gd.cid = 1001;
	/*gcd[6].gd.handle_controlevent = CH_OK;*/
	gcd[6].creator = GButtonCreate;

	gcd[7].gd.pos.x = -15; gcd[7].gd.pos.y = gcd[6].gd.pos.y+3;
	gcd[7].gd.pos.width = -1; gcd[7].gd.pos.height = 0;
	gcd[7].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[7].text = (unichar_t *) _STR_Cancel;
	label[7].text_in_resource = true;
	gcd[7].gd.label = &label[7];
	gcd[7].gd.mnemonic = 'C';
	/*gcd[7].gd.handle_controlevent = CH_Cancel;*/
	gcd[7].gd.cid = 1002;
	gcd[7].creator = GButtonCreate;

	gcd[8].gd.pos.x = 2; gcd[8].gd.pos.y = 2;
	gcd[8].gd.pos.width = pos.width-4; gcd[8].gd.pos.height = pos.height-2;
	gcd[8].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
	gcd[8].creator = GGroupCreate;

	GGadgetsCreate(gw,gcd);
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
	    int res = GetIntR(gw,1003,_STR_Other,&err);
	    if ( res<10 )
		ProtestR( _STR_Other_);
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
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static int WriteBitmaps(char *filename,SplineFont *sf, int32 *sizes,int res,
	int bf, EncMap *map) {
    char *buf = galloc(strlen(filename)+30), *pt, *pt2;
    int i;
    BDFFont *bdf;
#if defined(FONTFORGE_CONFIG_GDRAW)
    unichar_t *temp;
#endif
    char buffer[100], *ext;
    /* res = -1 => Guess depending on pixel size of font */
    extern int ask_user_for_resolution;

    if ( ask_user_for_resolution && res==0x80000000 ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	GProgressPauseTimer();
	res = AskResolution(bf);
	GProgressResumeTimer();
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_pause_timer();
	res = AskResolution(bf);
	gwwv_progress_resume_timer();
#endif
	if ( res==-2 )
return( false );
    }

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;

    for ( i=0; sizes[i]!=0; ++i );
#if defined(FONTFORGE_CONFIG_GDRAW)
    GProgressChangeStages(i);
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_change_stages(i);
#endif
    for ( i=0; sizes[i]!=0; ++i ) {
	buffer[0] = '\0';
	for ( bdf=sf->bitmaps; bdf!=NULL &&
		(bdf->pixelsize!=(sizes[i]&0xffff) || BDFDepth(bdf)!=(sizes[i]>>16));
		bdf=bdf->next );
	if ( bdf==NULL ) {
#if defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_notice(_("Missing Bitmap"),_("Attempt to save a pixel size that has not been created (%d@%d)",
			    sizes[i]&0xffff, sizes[i]>>16);
#else
	    GWidgetPostNoticeR(_STR_MissingBitmap,_STR_MissingBitmapLong,
		    sizes[i]&0xffff, sizes[i]>>16);
#endif
	    free(buf);
return( false );
	}
	strcpy(buf,filename);
	pt = strrchr(buf,'.');
	if ( pt!=NULL && (pt2=strrchr(buf,'/'))!=NULL && pt<pt2 )
	    pt = NULL;
	if ( pt==NULL )
	    pt = buf+strlen(buf);
	if ( strcmp(pt-4,".otf.dfont")==0 || strcmp(pt-4,".ttf.bin")==0 ) pt-=4;
	ext = bf==bf_bdf ? ".bdf" : ".fnt";
	if ( bdf->clut==NULL )
	    sprintf( pt, "-%d%s", bdf->pixelsize, ext );
	else
	    sprintf( pt, "-%d@%d%s", bdf->pixelsize, BDFDepth(bdf), ext );

#if defined(FONTFORGE_CONFIG_GDRAW)
	GProgressChangeLine2(temp=uc_copy(buf)); free(temp);
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_change_line2(buf);
#endif
	if ( bf==bf_bdf ) 
	    BDFFontDump(buf,bdf,map,res);
	else
	    FONFontDump(buf,bdf,map,res);
#if defined(FONTFORGE_CONFIG_GDRAW)
	GProgressNextStage();
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_next_stage();
#endif
    }
    free(buf);
return( true );
}

#ifdef FONTFORGE_CONFIG_TYPE3
static int CheckIfTransparent(SplineFont *sf) {
    /* Type3 doesn't support translucent fills */
    int i,j;
#if defined(FONTFORGE_CONFIG_GDRAW)
    static int buts[3] = { _STR_Yes, _STR_Cancel, 0 };
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[3] = { GTK_STOCK_YES, GTK_STOCK_CANCEL, NULL };
#endif

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	SplineChar *sc = sf->glyphs[i];
	for ( j=ly_fore; j<sc->layer_cnt; ++j ) {
	    if ( sc->layers[j].fill_brush.opacity!=1 || sc->layers[j].stroke_pen.brush.opacity!=1 ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
		if ( GWidgetAskR(_STR_BadDrawingOperation,buts,0,1,_STR_Type3NotTrans)==1 )
#elif defined(FONTFORGE_CONFIG_GTK)
		if ( gwwv_ask(_("Bad Drawing Operation"),buts,0,1,_("This font contains at least one translucent layer, but type3 does not support that (anything translucent or transparent is treated as opaque). Do you want to proceed anyway?"))==1 )
#endif
return( true );

return( false );
	    }
	}
    }
return( false );
}

static int CheckIfImages(SplineFont *sf) {
    /* SVG doesn't support images (that I can figure out anyway) */
    int i,j;
#if defined(FONTFORGE_CONFIG_GDRAW)
    static int buts[3] = { _STR_Yes, _STR_Cancel, 0 };
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[3] = { GTK_STOCK_YES, GTK_STOCK_CANCEL, NULL };
#endif

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	SplineChar *sc = sf->glyphs[i];
	for ( j=ly_fore; j<sc->layer_cnt; ++j ) {
	    if ( sc->layers[j].images!=NULL ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
		if ( GWidgetAskR(_STR_BadDrawingOperation,buts,0,1,_STR_SvgNoImages)==1 )
#elif defined(FONTFORGE_CONFIG_GTK)
		if ( gwwv_ask(_("Bad Drawing Operation"),buts,0,1,_("This font contains at least one foreground image, but svg does not support that. Do you want to proceed anyway?"))==1 )
#endif
return(true);

return( false );
	    }
	}
    }
return( false );
}
#endif

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

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
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
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static char *GetWernerSFDFile(SplineFont *sf,EncMap *map) {
    char *def=NULL, *ret;
    char buffer[100];
    unichar_t ubuf[100], *uret;
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
		ret = SearchNoLibsDirForWernerFile(GResourceProgramDir,def);
#ifdef SHAREDIR
	    if ( ret==NULL )
		ret = SearchDirForWernerFile(SHAREDIR,def);
#endif
	    if ( ret==NULL )
		ret = SearchDirForWernerFile(getPfaEditShareDir(),def);
	    if ( ret==NULL )
		ret = SearchDirForWernerFile("/usr/share/pfaedit",def);
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
    uc_strcpy(ubuf,def);
#if defined(FONTFORGE_CONFIG_GDRAW)
    uret = GWidgetOpenFile(GStringGetResource(_STR_FindMultipleMap,NULL),NULL,ubuf,NULL,GFileChooserFilterWernerSFDs);
#elif defined(FONTFORGE_CONFIG_GTK)
    uret = GWidgetOpenFile(_("Find Sub Font Definition file"),NULL,ubuf,NULL,GFileChooserFilterWernerSFDs);
#endif
    ret = u2def_copy(uret);
    free(uret);
return( ret );
}

static int32 *ParseWernerSFDFile(char *wernerfilename,SplineFont *sf,int *max,
	char ***_names, EncMap *map) {
    /* one entry for each char, >=1 => that subfont, 0=>not mapped, -1 => end of char mark */
    int cnt=0, subfilecnt=0, thusfar;
    int k, warned = false;
    uint32 r1,r2,i,modi;
    SplineFont *_sf;
    int32 *mapping;
    FILE *file;
    char buffer[200], *bpt;
    char *end, *pt;
    char *orig;
    struct remap *remap;
    char **names;
    int loop;

    file = fopen(wernerfilename,"r");
    if ( file==NULL ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	GWidgetErrorR(_STR_NoSubFontDefinitionFile,_STR_NoSubFontDefinitionFile);
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("No Sub Font Definition file"),_("No Sub Font Definition file"));
#endif
return( NULL );
    }

    k = 0;
    do {
	_sf = sf->subfontcnt==0 ? sf : sf->subfonts[k++];
	if ( _sf->glyphcnt>cnt ) cnt = _sf->glyphcnt;
    } while ( k<sf->subfontcnt );

    mapping = gcalloc(cnt+1,sizeof(int32));
    memset(mapping,-1,(cnt+1)*sizeof(int32));
    mapping[cnt] = -2;
    *max = 0;

    while ( fgets(buffer,sizeof(buffer),file)!=NULL )
	++subfilecnt;
    names = galloc((subfilecnt+1)*sizeof(char *));

    rewind(file);
    subfilecnt = 0;
    while ( fgets(buffer,sizeof(buffer),file)!=NULL ) {
	if ( strncmp(buffer,pfaeditflag,strlen(pfaeditflag))== 0 ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GWidgetErrorR(_STR_WrongSFDFile,_STR_BadSFDFile);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Wrong type of SFD file"),_("This looks like one of FontForge's SplineFont DataBase files.\nNot one of TeX's SubFont Definition files.\nAn unfortunate confusion of extensions."));
#endif
	    free(mapping);
return( NULL );
	}
	pt=buffer+strlen(buffer)-1;
	bpt = buffer;
	if (( *pt!='\n' && *pt!='\r') || (pt>buffer && pt[-1]=='\\') ||
		(pt>buffer+1 && pt[-2]=='\\' && isspace(pt[-1])) ) {
	    bpt = copy("");
	    forever {
		loop = false;
		if (( *pt!='\n' && *pt!='\r') || (pt>buffer && pt[-1]=='\\') ||
			(pt>buffer+1 && pt[-2]=='\\' && isspace(pt[-1])) )
		    loop = true;
		if ( *pt=='\n' || *pt=='\r') {
		    if ( pt[-1]=='\\' )
			pt[-1] = '\0';
		    else if ( pt[-2]=='\\' )
			pt[-2] = '\0';
		}
		bpt = grealloc(bpt,strlen(bpt)+strlen(buffer)+10);
		strcat(bpt,buffer);
		if ( !loop )
	    break;
		if ( fgets(buffer,sizeof(buffer),file)==NULL )
	    break;
		pt=buffer+strlen(buffer)-1;
	    }
	}
	if ( bpt[0]=='#' || bpt[0]=='\0' || isspace(bpt[0]))
    continue;
	for ( pt = bpt; !isspace(*pt) && *pt!='\0'; ++pt );
	if ( *pt=='\0' || *pt=='\r' || *pt=='\n' )
    continue;
	names[subfilecnt] = copyn(bpt,pt-bpt);
	if ( subfilecnt>*max ) *max = subfilecnt;
	end = pt;
	thusfar = 0;
	while ( *end!='\0' ) {
	    while ( isspace(*end)) ++end;
	    if ( *end=='\0' )
	break;
	    orig = end;
	    r1 = strtoul(end,&end,0);
	    if ( orig==end )
	break;
	    while ( isspace(*end)) ++end;
	    if ( *end==':' ) {
		if ( r1>=256 || r1<0)
		    fprintf( stderr, "Bad offset: %d for subfont %s\n", r1, names[subfilecnt]);
		else
		    thusfar = r1;
		r1 = strtoul(end+1,&end,0);
	    }
	    if ( *end=='_' || *end=='-' )
		r2 = strtoul(end+1,&end,0);
	    else
		r2 = r1;
	    for ( i=r1; i<=r2; ++i ) {
		modi = i;
		if ( map->remap!=NULL ) {
		    for ( remap = map->remap; remap->infont!=-1; ++remap ) {
			if ( i>=remap->firstenc && i<=remap->lastenc ) {
			    modi = i-remap->firstenc + remap->infont;
		    break;
			}
		    }
		}
		if ( modi<map->enccount )
		    modi = map->map[modi];
		else if ( sf->subfontcnt!=0 )
		    modi = modi;
		else
		    modi = -1;
		if ( modi<cnt && modi!=-1 ) {
		    if ( mapping[modi]!=-1 && !warned ) {
			if (( i==0xffff || i==0xfffe ) &&
				(map->enc->is_unicodebmp || map->enc->is_unicodefull))
			    /* Not a character anyway. just ignore it */;
			else {
			    fprintf( stderr, "Warning: Encoding %d (%x) is mapped to at least two locations (%s@0x%02x and %s@0x%02x)\n Only one will be used here.\n",
				    i, i, names[subfilecnt], thusfar, names[(mapping[modi]>>8)], mapping[modi]&0xff );
			    warned = true;
			}
		    }
		    mapping[modi] = (subfilecnt<<8) | thusfar;
		}
		thusfar++;
	    }
	}
	if ( thusfar>256 )
	    fprintf( stderr, "More that 256 entries in subfont %s\n", names[subfilecnt] );
	++subfilecnt;
	if ( bpt!=buffer )
	    free(bpt);
    }
    names[subfilecnt]=NULL;
    *_names = names;
    fclose(file);
return( mapping );
}

static int SaveSubFont(SplineFont *sf,char *newname,int32 *sizes,int res,
	int32 *mapping, int subfont, char **names,EncMap *map) {
    SplineFont temp;
    SplineChar *chars[256], **newchars;
    SplineFont *_sf;
    int k, i, used, base, extras;
    char *filename;
    char *spt, *pt, buf[8];
    RefChar *ref;
    int err = 0;
#if defined(FONTFORGE_CONFIG_GDRAW)
    unichar_t *ufile;
#endif
    enum fontformat subtype = strstr(newname,".pfa")!=NULL ? ff_pfa : ff_pfb ;
    EncMap encmap;
    int _mapping[256], _backmap[256];

    memset(&encmap,0,sizeof(encmap));
    encmap.enccount = encmap.encmax = encmap.backmax = 256;
    encmap.map = _mapping; encmap.backmap = _backmap;
    memset(_mapping,-1,sizeof(_mapping));
    memset(_backmap,-1,sizeof(_backmap));
    encmap.enc = &custom;

    temp = *sf;
    temp.glyphcnt = 0;
    temp.glyphmax = 256;
    temp.glyphs = chars;
    temp.bitmaps = NULL;
    temp.subfonts = NULL;
    temp.subfontcnt = 0;
    temp.uniqueid = 0;
    memset(chars,0,sizeof(chars));
    temp.glyphnames = NULL;
    used = 0;
    for ( i=0; mapping[i]!=-2; ++i ) if ( (mapping[i]>>8)==subfont ) {
	k = 0;
	do {
	    _sf = sf->subfontcnt==0 ? sf : sf->subfonts[k++];
	    if ( i<_sf->glyphcnt && _sf->glyphs[i]!=NULL )
	break;
	} while ( k<sf->subfontcnt );
	if ( temp.glyphcnt<256 ) {
	    if ( i<_sf->glyphcnt ) {
		if ( _sf->glyphs[i]!=NULL ) {
		    _sf->glyphs[i]->parent = &temp;
		    _sf->glyphs[i]->orig_pos = temp.glyphcnt;
		    chars[temp.glyphcnt] = _sf->glyphs[i];
		    _mapping[mapping[i]&0xff] = temp.glyphcnt;
		    _backmap[temp.glyphcnt] = mapping[i]&0xff;
		    ++temp.glyphcnt;
		    ++used;
		}
	    }
	}
    }
    if ( used==0 )
return( 0 );

    /* check for any references to things outside this subfont and add them */
    /*  as unencoded chars */
    /* We could just replace with splines, I suppose but that would make */
    /*  korean fonts huge */
    forever {
	extras = 0;
	for ( i=0; i<temp.glyphcnt; ++i ) if ( temp.glyphs[i]!=NULL ) {
	    for ( ref=temp.glyphs[i]->layers[ly_fore].refs; ref!=NULL; ref=ref->next )
		if ( ref->sc->parent!=&temp )
		    ++extras;
	}
	if ( extras == 0 )
    break;
	newchars = gcalloc(temp.glyphcnt+extras,sizeof(SplineChar *));
	memcpy(newchars,temp.glyphs,temp.glyphcnt*sizeof(SplineChar *));
	if ( temp.glyphs!=chars ) free(temp.glyphs );
	base = temp.glyphcnt;
	temp.glyphs = newchars;
	extras = 0;
	for ( i=0; i<base; ++i ) if ( temp.glyphs[i]!=NULL ) {
	    for ( ref=temp.glyphs[i]->layers[ly_fore].refs; ref!=NULL; ref=ref->next )
		if ( ref->sc->parent!=&temp ) {
		    temp.glyphs[base+extras] = ref->sc;
		    ref->sc->parent = &temp;
		    ref->sc->orig_pos = base+extras++;
		}
	}
	temp.glyphcnt += extras;	/* this might be a slightly different value from that found before if some references get reused. N'importe */
	temp.glyphmax = temp.glyphcnt;
    }

    filename = galloc(strlen(newname)+strlen(names[subfont])+10);
    strcpy(filename,newname);
    pt = strrchr(filename,'.');
    spt = strrchr(filename,'/');
    if ( spt==NULL ) spt = filename; else ++spt;
    if ( pt>spt )
	*pt = '\0';
    pt = strstr(spt,"%d");
    if ( pt==NULL )
	pt = strstr(spt,"%s");
    if ( pt==NULL )
	strcat(filename,names[subfont]);
    else {
	int len = strlen(names[subfont]);
	int l;
	if ( len>2 ) {
	    for ( l=strlen(pt); l>=2 ; --l )
		pt[l+len-2] = pt[l];
	} else if ( len<2 )
	    strcpy(pt+len,pt+2);
	memcpy(pt,names[subfont],len);
    }
    temp.fontname = copy(spt);
    temp.fullname = galloc(strlen(temp.fullname)+strlen(names[subfont])+3);
    strcpy(temp.fullname,sf->fullname);
    strcat(temp.fullname," ");
    strcat(temp.fullname,names[subfont]);
    strcat(spt,subtype==ff_pfb ? ".pfb" : ".pfa" );
#if defined(FONTFORGE_CONFIG_GDRAW)
    GProgressChangeLine2(ufile=uc_copy(filename)); free(ufile);
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_change_line2(filename);
#endif

    if ( sf->xuid!=NULL ) {
	sprintf( buf, "%d", subfont );
	temp.xuid = galloc(strlen(sf->xuid)+strlen(buf)+5);
	strcpy(temp.xuid,sf->xuid);
	pt = temp.xuid + strlen( temp.xuid )-1;
	while ( pt>temp.xuid && *pt==' ' ) --pt;
	if ( *pt==']' ) --pt;
	*pt = ' ';
	strcpy(pt+1,buf);
	strcat(pt,"]");
    }

    err = !WritePSFont(filename,&temp,subtype,old_ps_flags,&encmap);
    if ( err )
#if defined(FONTFORGE_CONFIG_GDRAW)
	GWidgetErrorR(_STR_Savefailedtitle,_STR_Savefailedtitle);
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Save Failed"),_("Save Failed"));
#endif
#if defined(FONTFORGE_CONFIG_GDRAW)
    if ( !err && (old_ps_flags&ps_flag_afm) && GProgressNextStage()) {
#elif defined(FONTFORGE_CONFIG_GTK)
    if ( !err && (old_ps_flags&ps_flag_afm) && gwwv_progress_next_stage()) {
#else
    if ( !err && (old_ps_flags&ps_flag_afm)) {
#endif
	if ( !WriteAfmFile(filename,&temp,oldformatstate,&encmap)) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GWidgetErrorR(_STR_Afmfailedtitle,_STR_Afmfailedtitle);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Afm Save Failed"),_("Afm Save Failed"));
#endif
	    err = true;
	}
    }
    if ( !err && (old_ps_flags&ps_flag_tfm) ) {
	if ( !WriteTfmFile(filename,&temp,oldformatstate,&encmap)) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GWidgetErrorR(_STR_Tfmfailedtitle,_STR_Tfmfailedtitle);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Tfm Save Failed"),_("Tfm Save Failed"));
#endif
	    err = true;
	}
    }
    /* ??? Bitmaps */
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#if defined(FONTFORGE_CONFIG_GDRAW)
    if ( !GProgressNextStage())
#elif defined(FONTFORGE_CONFIG_GTK)
    if ( !gwwv_progress_next_stage())
#endif
	err = -1;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

    if ( temp.glyphs!=chars )
	free(temp.glyphs);
    GlyphHashFree( &temp );
    free( temp.xuid );
    free( temp.fontname );
    free( temp.fullname );
    free( filename );

    /* SaveSubFont messes up the parent and orig_pos fields. Fix 'em up */
    /* Do this after every save, else afm,tfm files might produce extraneous kerns */
    k = 0;
    do {
	_sf = sf->subfontcnt==0 ? sf : sf->subfonts[k++];
	for ( i=0; i<_sf->glyphcnt; ++i ) if ( _sf->glyphs[i]!=NULL ) {
	    _sf->glyphs[i]->parent = _sf;
	    _sf->glyphs[i]->orig_pos = i;
	}
    } while ( k<sf->subfontcnt );

return( err );
}

/* ttf2tfm supports multiple sfd files. I do not. */
static int WriteMultiplePSFont(SplineFont *sf,char *newname,int32 *sizes,
	int res, char *wernerfilename,EncMap *map) {
    int err=0, tofree=false, max, filecnt;
    int32 *mapping;
    unichar_t *path;
    int i;
    char **names;
    char *pt;

    pt = strrchr(newname,'.');
    if ( pt==NULL ||
	    (strcmp(pt,".pfa")!=0 && strcmp(pt,".pfb")!=0 && strcmp(pt,".mult")!=0)) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	GWidgetErrorR(_STR_BadExtension,_STR_MustBeType1);
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Bad Extension"),_("You must specify a standard type1 extension (.pfb or .pfa)"));
#endif
return( 0 );
    }

    if ( wernerfilename==NULL ) {
	wernerfilename = GetWernerSFDFile(sf,map);
	tofree = true;
    }
    if ( wernerfilename==NULL )
return( 0 );
    mapping = ParseWernerSFDFile(wernerfilename,sf,&max,&names,map);
    if ( tofree ) free(wernerfilename);
    if ( mapping==NULL )
return( 1 );

    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;

    filecnt = 1;
    if ( (old_ps_flags&ps_flag_afm) )
	filecnt = 2;
#if 0
    if ( oldbitmapstate==bf_bdf )
	++filecnt;
#endif
    path = def2u_copy(newname);
#if defined(FONTFORGE_CONFIG_GDRAW)
    GProgressStartIndicator(10,GStringGetResource(_STR_SavingFont,NULL),
	    GStringGetResource(_STR_SavingMultiplePSFonts,NULL),
	    path,256,(max+1)*filecnt );
    /*GProgressEnableStop(false);*/
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_start_indicator(10,_("Saving font"),
	    _("Saving Multiple Postscript Fonts"),
	    path,256,(max+1)*filecnt );
    /*gwwv_progress_enable_stop(false);*/
#endif
    free(path);

    for ( i=0; i<=max && !err; ++i )
	err = SaveSubFont(sf,newname,sizes,res,mapping,i,names,map);

    free(mapping);
    for ( i=0; names[i]!=NULL; ++i ) free(names[i]);
    free(names);
    free( sizes );
#if defined(FONTFORGE_CONFIG_GDRAW)
    GProgressEndIndicator();
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_end_indicator();
#endif
    if ( !err )
	SavePrefs();
return( err );
}

static int _DoSave(SplineFont *sf,char *newname,int32 *sizes,int res,EncMap *map) {
    unichar_t *path;
    int err=false;
    int iscid = oldformatstate==ff_cid || oldformatstate==ff_cffcid ||
	    oldformatstate==ff_otfcid || oldformatstate==ff_otfciddfont;
    int flags = 0;

    if ( oldformatstate == ff_multiple )
return( WriteMultiplePSFont(sf,newname,sizes,res,NULL,map));

    if ( oldformatstate<=ff_cffcid )
	flags = old_ps_flags;
    else if ( oldformatstate<=ff_ttfdfont )
	flags = old_ttf_flags;
    else if ( oldformatstate!=ff_none )
	flags = old_otf_flags;
    else
	flags = old_ttf_flags;
    if ( oldformatstate<=ff_cffcid && oldbitmapstate==bf_otb )
	flags = old_psotb_flags;

    path = def2u_copy(newname);
#if defined(FONTFORGE_CONFIG_GDRAW)
    GProgressStartIndicator(10,GStringGetResource(_STR_SavingFont,NULL),
		GStringGetResource(oldformatstate==ff_ttf || oldformatstate==ff_ttfsym ||
		     oldformatstate==ff_ttfmacbin ?_STR_SavingTTFont:
		 oldformatstate==ff_otf || oldformatstate==ff_otfdfont ?_STR_SavingOpenTypeFont:
		 oldformatstate==ff_cid || oldformatstate==ff_cffcid ||
		  oldformatstate==ff_otfcid || oldformatstate==ff_otfciddfont ?_STR_SavingCIDFont:
		 _STR_SavingPSFont,NULL),
	    path,sf->glyphcnt,1);
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_start_indicator(10,_("Saving font"),
		GStringGetResource(oldformatstate==ff_ttf || oldformatstate==ff_ttfsym ||
		     oldformatstate==ff_ttfmacbin ?_("Saving TrueType Font") :
		 oldformatstate==ff_otf || oldformatstate==ff_otfdfont ?_("Saving OpenType Font"):
		 oldformatstate==ff_cid || oldformatstate==ff_cffcid ||
		  oldformatstate==ff_otfcid || oldformatstate==ff_otfciddfont ?_("Saving CID keyed font") :
		 _("Saving PostScript Font"),NULL),
	    path,sf->glyphcnt,1);
#endif
    free(path);
    if ( oldformatstate!=ff_none ||
	    oldbitmapstate==bf_sfnt_dfont ||
	    oldbitmapstate==bf_ttf ) {
	int oerr = 0;
	int bmap = oldbitmapstate;
	if ( bmap==bf_otb ) bmap = bt_none;
	switch ( oldformatstate ) {
	  case ff_mma: case ff_mmb:
	    sf = sf->mm->instances[0];
	  case ff_pfa: case ff_pfb: case ff_ptype3: case ff_ptype0:
	  case ff_cid:
	  case ff_type42: case ff_type42cid:
#ifdef FONTFORGE_CONFIG_TYPE3
	    if ( sf->multilayer && CheckIfTransparent(sf))
return( true );
#endif
	    oerr = !WritePSFont(newname,sf,oldformatstate,flags,map);
	  break;
	  case ff_ttf: case ff_ttfsym: case ff_otf: case ff_otfcid:
	  case ff_cff: case ff_cffcid:
	    oerr = !WriteTTFFont(newname,sf,oldformatstate,sizes,bmap,
		flags,map);
	  break;
	  case ff_pfbmacbin:
	    oerr = !WriteMacPSFont(newname,sf,oldformatstate,flags,map);
	  break;
	  case ff_ttfmacbin: case ff_ttfdfont: case ff_otfdfont: case ff_otfciddfont:
	    oerr = !WriteMacTTFFont(newname,sf,oldformatstate,sizes,
		    bmap,flags,map);
	  break;
	  case ff_svg:
#ifdef FONTFORGE_CONFIG_TYPE3
	    if ( sf->multilayer && CheckIfImages(sf))
return( true );
#endif
	    oerr = !WriteSVGFont(newname,sf,oldformatstate,flags,map);
	  break;
	  case ff_none:		/* only if bitmaps, an sfnt wrapper for bitmaps */
	    if ( bmap==bf_sfnt_dfont )
		oerr = !WriteMacTTFFont(newname,sf,oldformatstate,sizes,
			bmap,flags,map);
	    else if ( bmap==bf_ttf )
		oerr = !WriteTTFFont(newname,sf,oldformatstate,sizes,bmap,
		    flags,map);
	  break;
	}
	if ( oerr ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GWidgetErrorR(_STR_Savefailedtitle,_STR_Savefailedtitle);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Save Failed"),_("Save Failed"));
#endif
	    err = true;
	}
    }
    if ( !err && (flags&ps_flag_tfm) ) {
	if ( !WriteTfmFile(newname,sf,oldformatstate,map)) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GWidgetErrorR(_STR_Tfmfailedtitle,_STR_Tfmfailedtitle);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Tfm Save Failed"),_("Tfm Save Failed"));
#endif
	    err = true;
	}
    }
    if ( !err && (flags&ps_flag_afm) ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	GProgressIncrementBy(-sf->glyphcnt);
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_increment(-sf->glyphcnt);
#endif
	if ( !WriteAfmFile(newname,sf,oldformatstate,map)) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GWidgetErrorR(_STR_Afmfailedtitle,_STR_Afmfailedtitle);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Afm Save Failed"),_("Afm Save Failed"));
#endif
	    err = true;
	}
    }
    if ( !err && (flags&ps_flag_pfm) && !iscid ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	GProgressChangeLine1R(_STR_SavingPFM);
	GProgressIncrementBy(-sf->glyphcnt);
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_change_line1(_("Saving PFM File"));
	gwwv_progress_increment(-sf->glyphcnt);
#endif
	if ( !WritePfmFile(newname,sf,oldformatstate==ff_ptype0,map)) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GWidgetErrorR(_STR_Pfmfailedtitle,_STR_Pfmfailedtitle);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Pfm Save Failed"),_("Pfm Save Failed"));
#endif
	    err = true;
	}
    }
    if ( oldbitmapstate==bf_otb ) {
	char *temp = newname;
	if ( newname[strlen(newname)-1]=='.' ) {
	    temp = galloc(strlen(newname)+8);
	    strcpy(temp,newname);
	    strcat(temp,"otb");
	}
	if ( !WriteTTFFont(temp,sf,ff_none,sizes,bf_otb,flags,map) )
	    err = true;
	if ( temp!=newname )
	    free(temp);
    } else if ( (oldbitmapstate==bf_bdf || oldbitmapstate==bf_fon) && !err ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	GProgressChangeLine1R(_STR_SavingBitmapFonts);
	GProgressIncrementBy(-sf->glyphcnt);
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_change_line1(_("Saving Bitmap Font(s)"));
	gwwv_progress_increment(-sf->glyphcnt);
#endif
	if ( !WriteBitmaps(newname,sf,sizes,res,oldbitmapstate,map))
	    err = true;
    } else if ( oldbitmapstate==bf_palm && !err ) {
	if ( !WritePalmBitmaps(newname,sf,sizes,map))
	    err = true;
    } else if ( (oldbitmapstate==bf_nfntmacbin /*|| oldbitmapstate==bf_nfntdfont*/) &&
	    !err ) {
	if ( !WriteMacBitmaps(newname,sf,sizes,false/*oldbitmapstate==bf_nfntdfont*/,map))
	    err = true;
    }
    free( sizes );
#if defined(FONTFORGE_CONFIG_GDRAW)
    GProgressEndIndicator();
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_end_indicator();
#endif
    if ( !err )
	SavePrefs();
return( err );
}

static int32 *AllBitmapSizes(SplineFont *sf) {
    int32 *sizes=NULL;
    BDFFont *bdf;
    int i,cnt;

    for ( i=0; i<2; ++i ) {
	cnt = 0;
	for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( sizes!=NULL )
		sizes[cnt] = bdf->pixelsize | (BDFDepth(bdf)<<16);
	    ++cnt;
	}
	if ( i==1 )
    break;
	sizes = galloc((cnt+1)*sizeof(int32));
    }
    sizes[cnt] = 0;
return( sizes );
}

int GenerateScript(SplineFont *sf,char *filename,char *bitmaptype, int fmflags,
	int res, char *subfontdefinition, struct sflist *sfs,EncMap *map) {
    int i;
    static char *bitmaps[] = {"bdf", "ttf", "sbit", "bin", /*"dfont", */"fnt", "otb", "pdb", NULL };
    int32 *sizes=NULL;
    char *end = filename+strlen(filename);
    struct sflist *sfi;
    char *freeme = NULL;
    int ret;

    if ( sf->bitmaps==NULL ) i = bf_none;
    else if ( strmatch(bitmaptype,"otf")==0 ) i = bf_ttf;
    else if ( strmatch(bitmaptype,"ms")==0 ) i = bf_ttf;
    else if ( strmatch(bitmaptype,"apple")==0 ) i = bf_ttf;
    else if ( strmatch(bitmaptype,"nfnt")==0 ) i = bf_nfntmacbin;
    else for ( i=0; bitmaps[i]!=NULL; ++i ) {
	if ( strmatch(bitmaptype,bitmaps[i])==0 )
    break;
    }
    oldbitmapstate = i;

    for ( i=0; extensions[i]!=NULL; ++i ) {
	if ( strlen( extensions[i])>0 &&
		end-filename>=strlen(extensions[i]) &&
		strmatch(end-strlen(extensions[i]),extensions[i])==0 )
    break;
    }
    if ( end-filename>8 && strmatch(end-strlen(".ttf.bin"),".ttf.bin")==0 )
	i = ff_ttfmacbin;
    else if ( end-filename>5 && strmatch(end-strlen(".suit"),".suit")==0 )	/* Different extensions for Mac/non Mac, support both always */
	i = ff_ttfmacbin;
    else if ( end-filename>4 && strmatch(end-strlen(".bin"),".bin")==0 )
	i = ff_pfbmacbin;
    else if ( end-filename>4 && strmatch(end-strlen(".res"),".res")==0 )
	i = ff_pfbmacbin;
    else if ( end-filename>8 && strmatch(end-strlen(".sym.ttf"),".sym.ttf")==0 )
	i = ff_ttfsym;
    else if ( end-filename>8 && strmatch(end-strlen(".cid.cff"),".cid.cff")==0 )
	i = ff_cffcid;
    else if ( end-filename>8 && strmatch(end-strlen(".cid.t42"),".cid.t42")==0 )
	i = ff_type42cid;
    else if ( end-filename>7 && strmatch(end-strlen(".mm.pfa"),".mm.pfa")==0 )
	i = ff_mma;
    else if ( end-filename>7 && strmatch(end-strlen(".mm.pfb"),".mm.pfb")==0 )
	i = ff_mmb;
    else if ( end-filename>7 && strmatch(end-strlen(".mult"),".mult")==0 )
	i = ff_multiple;
    else if (( i==ff_pfa || i==ff_pfb ) && strstr(filename,"%s")!=NULL )
	i = ff_multiple;
    if ( extensions[i]==NULL ) {
	for ( i=0; bitmaps[i]!=NULL; ++i ) {
	    if ( end-filename>strlen(bitmaps[i]) &&
		    strmatch(end-strlen(bitmaps[i]),bitmaps[i])==0 )
	break;
	}
	if ( *filename=='\0' || end[-1]=='.' )
	    i = ff_none;
	else if ( bitmaps[i]==NULL )
	    i = ff_pfb;
	else {
	    oldbitmapstate = i;
	    i = ff_none;
	}
    }
    if ( i==ff_ptype3 && map->enc->has_2byte )
	i = ff_ptype0;
    else if ( i==ff_ttfdfont && strmatch(end-strlen(".otf.dfont"),".otf.dfont")==0 )
	i = ff_otfdfont;
    if ( sf->cidmaster!=NULL ) {
	if ( i==ff_otf ) i = ff_otfcid;
	else if ( i==ff_otfdfont ) i = ff_otfciddfont;
    }
    oldformatstate = i;

    if ( oldformatstate==ff_none && end[-1]=='.' &&
	    (oldbitmapstate==bf_ttf || oldbitmapstate==bf_sfnt_dfont || oldbitmapstate==bf_otb)) {
	freeme = galloc(strlen(filename)+8);
	strcpy(freeme,filename);
	if ( strmatch(bitmaptype,"otf")==0 )
	    strcat(freeme,"otf");
	else if ( oldbitmapstate==bf_otb )
	    strcat(freeme,"otf");
	else if ( oldbitmapstate==bf_sfnt_dfont )
	    strcat(freeme,"dfont");
	else
	    strcat(freeme,"ttf");
	filename = freeme;
    } else if ( sf->onlybitmaps && sf->bitmaps!=NULL &&
	    (oldformatstate==ff_ttf || oldformatstate==ff_otf) &&
	    (oldbitmapstate == bf_none || oldbitmapstate==bf_ttf ||
	     oldbitmapstate==bf_sfnt_dfont || oldbitmapstate==bf_otb)) {
	if ( oldbitmapstate==ff_ttf )
	    oldbitmapstate = bf_ttf;
	oldformatstate = ff_none;
    }

    if ( oldbitmapstate==bf_sfnt_dfont )
	oldformatstate = ff_none;

    if ( fmflags==-1 ) {
	if ( alwaysgenapple && alwaysgenopentype ) {
	    old_otf_flags |= ttf_flag_applemode | ttf_flag_otmode;
	    old_ttf_flags |= ttf_flag_applemode | ttf_flag_otmode;
	} else if ( alwaysgenapple ) {
	    old_otf_flags |= ttf_flag_applemode;
	    old_ttf_flags |= ttf_flag_applemode;
	    old_otf_flags &=~ttf_flag_otmode;
	    old_ttf_flags &=~ttf_flag_otmode;
	} else if ( alwaysgenopentype ) {
	    old_otf_flags |= ttf_flag_otmode;
	    old_ttf_flags |= ttf_flag_otmode;
	    old_otf_flags &=~ttf_flag_applemode;
	    old_ttf_flags &=~ttf_flag_applemode;
	}
	if ( strmatch(bitmaptype,"apple")==0 || strmatch(bitmaptype,"sbit")==0 ||
		oldformatstate==ff_ttfmacbin || oldformatstate==ff_ttfdfont ||
		oldformatstate==ff_otfdfont || oldformatstate==ff_otfciddfont ) {
	    old_otf_flags |= ttf_flag_applemode;
	    old_ttf_flags |= ttf_flag_applemode;
	    old_otf_flags &=~ttf_flag_otmode;
	    old_ttf_flags &=~ttf_flag_otmode;
	} else if ( strmatch(bitmaptype,"ms")==0 ) {
	    old_otf_flags |= ttf_flag_otmode;
	    old_ttf_flags |= ttf_flag_otmode;
	    old_otf_flags &=~ttf_flag_applemode;
	    old_ttf_flags &=~ttf_flag_applemode;
	}
    } else {
	if ( oldformatstate<=ff_cffcid ) {
	    old_ps_flags = 0;
	    if ( fmflags&1 ) old_ps_flags |= ps_flag_afm;
	    if ( fmflags&2 ) old_ps_flags |= ps_flag_pfm;
	    if ( fmflags&0x10000 ) old_ps_flags |= ps_flag_tfm;
	    if ( fmflags&0x20000 ) old_ps_flags |= ps_flag_nohintsubs;
	    if ( fmflags&0x40000 ) old_ps_flags |= ps_flag_noflex;
	    if ( fmflags&0x80000 ) old_ps_flags |= ps_flag_nohints;
	    if ( fmflags&0x100000 ) old_ps_flags |= ps_flag_restrict256;
	    if ( fmflags&0x200000 ) old_ps_flags |= ps_flag_round;
	    if ( i==bf_otb ) {
		if ( fmflags&4 ) old_ttf_flags |= ttf_flag_shortps;
		if ( fmflags&0x20 ) old_ttf_flags |= ttf_flag_pfed_comments;
		if ( fmflags&0x40 ) old_ttf_flags |= ttf_flag_pfed_colors;
		if ( fmflags&0x200 ) old_ttf_flags |= ttf_flag_TeXtable;
	    }
	} else if ( oldformatstate<=ff_ttfdfont || oldformatstate==ff_none ) {
	    old_ttf_flags = 0;
	    if ( fmflags&4 ) old_ttf_flags |= ttf_flag_shortps;
	    if ( fmflags&8 ) old_ttf_flags |= ttf_flag_nohints;
	    if ( fmflags&0x10 ) old_ttf_flags |= ttf_flag_applemode;
	    else old_ttf_flags |= ttf_flag_otmode;
	    if ( fmflags&0x20 ) old_ttf_flags |= ttf_flag_pfed_comments;
	    if ( fmflags&0x40 ) old_ttf_flags |= ttf_flag_pfed_colors;
	    if ( fmflags&0x80 ) old_ttf_flags |= ttf_flag_applemode|ttf_flag_otmode;
	    if ( fmflags&0x100 ) old_ttf_flags |= ttf_flag_glyphmap;
	} else {
	    old_otf_flags = 0;
		/* Applicable postscript flags */
	    if ( fmflags&1 ) old_otf_flags |= ps_flag_afm;
	    if ( fmflags&2 ) old_otf_flags |= ps_flag_pfm;
	    if ( fmflags&0x20000 ) old_otf_flags |= ps_flag_nohintsubs;
	    if ( fmflags&0x40000 ) old_otf_flags |= ps_flag_noflex;
	    if ( fmflags&0x80000 ) old_otf_flags |= ps_flag_nohints;
	    if ( fmflags&0x200000 ) old_otf_flags |= ps_flag_round;
		/* Applicable truetype flags */
	    if ( fmflags&4 ) old_otf_flags |= ttf_flag_shortps;
	    if ( fmflags&0x10 ) old_otf_flags |= ttf_flag_applemode;
	    else old_otf_flags |= ttf_flag_otmode;
	    if ( fmflags&0x20 ) old_otf_flags |= ttf_flag_pfed_comments;
	    if ( fmflags&0x40 ) old_otf_flags |= ttf_flag_pfed_colors;
	    if ( fmflags&0x80 ) old_otf_flags |= ttf_flag_applemode|ttf_flag_otmode;
	    if ( fmflags&0x100 ) old_otf_flags |= ttf_flag_glyphmap;
	}
    }

    if ( oldbitmapstate!=bf_none ) {
	if ( sfs!=NULL ) {
	    for ( sfi=sfs; sfi!=NULL; sfi=sfi->next )
		sfi->sizes = AllBitmapSizes(sfi->sf);
	} else
	    sizes = AllBitmapSizes(sf);
    }

    if ( sfs!=NULL ) {
	int flags = 0;
	if ( oldformatstate<=ff_cffcid )
	    flags = old_ps_flags;
	else if ( oldformatstate<=ff_ttfdfont )
	    flags = old_ttf_flags;
	else
	    flags = old_otf_flags;
	ret = WriteMacFamily(filename,sfs,oldformatstate,oldbitmapstate,flags,map);
    } else if ( oldformatstate == ff_multiple ) {
	ret = !WriteMultiplePSFont(sf,filename,sizes,res,subfontdefinition,map);
    } else {
	ret = !_DoSave(sf,filename,sizes,res,map);
    }
    free(freeme);
return( ret );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
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

static void DoSave(struct gfc_data *d,unichar_t *path) {
    int err=false;
    char *temp;
    int32 *sizes=NULL;
    int iscid, i;
    struct sflist *sfs=NULL, *cur, *last=NULL;
#if defined(FONTFORGE_CONFIG_GDRAW)
    static int buts[] = { _STR_Yes, _STR_No, 0 };
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[] = { GTK_STOCK_YES, GTK_STOCK_NO, NULL };
#endif
    static int psscalewarned=0, ttfscalewarned=0;
    int flags;

    for ( i=d->sf->glyphcnt-1; i>=1; --i )
	if ( d->sf->glyphs[i]!=NULL && strcmp(d->sf->glyphs[i]->name,".notdef")==0 &&
		(d->sf->glyphs[i]->layers[ly_fore].splines!=NULL || AnyRefs(d->sf->glyphs[i]->layers[ly_fore].refs )))
    break;
    if ( i!=0 ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	if ( GWidgetAskR(_STR_NotdefName,buts,0,1,_STR_NotdefGlyph,i)==1 )
#elif defined(FONTFORGE_CONFIG_GTK)
	if ( gwwv_ask(_("Notdef name"),buts,0,1,_("The glyph at encoding %d is named \".notdef\" but contains an outline. Because it is called \".notdef\" it will not be included in the generated font. You may give it a new name using Element->Char Info. Do you wish to continue font generation (and omit this character)?"),i)==1 )
#endif
return;
    }

    temp = u2def_copy(path);
    oldformatstate = GGadgetGetFirstListSelectedItem(d->pstype);
    iscid = oldformatstate==ff_cid || oldformatstate==ff_cffcid ||
	    oldformatstate==ff_otfcid || oldformatstate==ff_otfciddfont;
    if ( !iscid && (d->sf->cidmaster!=NULL || d->sf->subfontcnt>1)) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	if ( GWidgetAskR(_STR_NotCID,buts,0,1,_STR_NotCIDOk)==1 )
#elif defined(FONTFORGE_CONFIG_GTK)
	if ( gwwv_ask(_("Not a CID format"),buts,0,1,_("You are attempting to save a CID font in a non-CID format. This is ok, but it means that only the current sub-font will be saved.\nIs that what you want?"))==1 )
#endif
return;
    }

    if ( oldformatstate<=ff_cffcid || (oldformatstate>=ff_otf && oldformatstate<=ff_otfciddfont)) {
	if ( d->sf->ascent+d->sf->descent!=1000 && !psscalewarned ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    if ( GWidgetAskR(_STR_EmSizeBad,buts,0,1,_STR_PSEmSize1000,
#elif defined(FONTFORGE_CONFIG_GTK)
	    if ( gwwv_ask(_("Non-standard Em-Size"),buts,0,1,_("The convention is that PostScript fonts should have an Em-Size of 1000. But this font has a size of %d. This is not an error, but you might consider altering the Em-Size with the Element->Font Info->PS General dialog.\nDo you wish to continue to generate your font in spite of this?"),
#endif
		    d->sf->ascent+d->sf->descent)==1 )
return;
	    psscalewarned = true;
	}
    } else if ( oldformatstate!=ff_none && oldformatstate!=ff_svg ) {
	int val = d->sf->ascent+d->sf->descent;
	int bit;
	for ( bit=0x800000; bit!=0; bit>>=1 )
	    if ( bit==val )
	break;
	if ( bit==0 && !ttfscalewarned ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    if ( GWidgetAskR(_STR_EmSizeBad,buts,0,1,_STR_TTFEmSize2,val)==1 )
#elif defined(FONTFORGE_CONFIG_GTK)
	    if ( gwwv_ask(_("Non-standard Em-Size"),buts,0,1,_("The convention is that TrueType fonts should have an Em-Size which is a power of 2. But this font has a size of %d. This is not an error, but you might consider altering the Em-Size with the Element->Font Info->PS General dialog.\nDo you wish to continue to generate your font in spite of this?"),val)==1 )
#endif
return;
	    ttfscalewarned = true;
	}
    }

    if ( ((oldformatstate<ff_ptype0 && oldformatstate!=ff_multiple) ||
		oldformatstate==ff_ttfsym || oldformatstate==ff_cff ) &&
		d->map->enc->has_2byte ) {
	static int buts[3] = { _STR_Yes, _STR_Cancel, 0 };
#if defined(FONTFORGE_CONFIG_GDRAW)
	if ( GWidgetAskR(_STR_EncodingTooLarge,buts,0,1,_STR_TwoBEncIn1BFont)==1 )
#elif defined(FONTFORGE_CONFIG_GTK)
	if ( gwwv_ask(_("Encoding Too Large"),buts,0,1,_("Your font has a 2 byte encoding, but you are attempting to save it in a format that only supports one byte encodings. This means that you won't be able to access anything after the first 256 characters without reencoding the font.\n\nDo you want to procede anyway?"))==1 )
#endif
return;
    }

    oldbitmapstate = GGadgetGetFirstListSelectedItem(d->bmptype);
    if ( oldbitmapstate!=bf_none )
	sizes = ParseBitmapSizes(d->bmpsizes,_STR_PixelList,&err);
    if ( err )
return;
    if ( oldbitmapstate==bf_nfntmacbin && oldformatstate!=ff_pfbmacbin && !nfnt_warned ) {
	nfnt_warned = true;
#if defined(FONTFORGE_CONFIG_GDRAW)
	GWidgetPostNoticeR(_STR_NFNTObsoleteTit,_STR_NFNTObsolete);
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_notice(_("The 'NFNT' bitmap format is obsolete"),_("The 'NFNT' bitmap format is not used under OS/X (though you still need to create a (useless) bitmap font if you are saving a type1 PostScript resource)"));
#endif
    } else if ( oldformatstate==ff_pfbmacbin &&
	    (oldbitmapstate!=bf_nfntmacbin || sizes[0]==0)) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	GWidgetErrorR(_STR_Type1NeedsNFNTTit,_STR_Type1NeedsNFNT);
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Needs bitmap font"),_("When generating a Mac Type1 resource font, you MUST generate at least one NFNT bitmap font to go with it. If you have not created any bitmaps for this font, cancel this dlg and use the Element->Bitmaps Available command to create one"));
#endif
return;
    } else if ( oldformatstate==ff_pfbmacbin && !post_warned) {
	post_warned = true;
#if defined(FONTFORGE_CONFIG_GDRAW)
	GWidgetPostNoticeR(_STR_POSTDepreciatedTit,_STR_POSTDepreciated);
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_notice(_("The 'POST' type1 format is probably depreciated"),_("The 'POST' type1 format is probably depreciated and may not work in future version of the mac."));
#endif
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
		    cur->sizes = ParseBitmapSizes(GWidgetGetControl(d->gw,CID_Family+10*i+1),_STR_PixelList,&err);
		if ( err ) {
		    SfListFree(sfs);
return;
		}
		cur->map = EncMapFromEncoding(cur->sf,d->map->enc);
	    }
	}
    }

    switch ( d->sod_which ) {
      case 0:	/* PostScript */
	flags = old_ps_flags = d->ps_flags;
      break;
      case 1:	/* TrueType */
	flags = old_ttf_flags = d->ttf_flags;
	if ( !d->sod_invoked ) {
	    if ( oldformatstate==ff_ttfmacbin || oldformatstate==ff_ttfdfont || d->family ||
		    (oldformatstate==ff_none && oldbitmapstate==bf_sfnt_dfont))
		old_ttf_flags |= ttf_flag_applemode;
	}
      break;
      case 2:	/* OpenType */
	flags = old_otf_flags = d->otf_flags;
	if ( !d->sod_invoked ) {
	    if ( oldformatstate==ff_otfdfont || oldformatstate==ff_otfciddfont || d->family ||
		    (oldformatstate==ff_none && oldbitmapstate==bf_sfnt_dfont))
		old_ttf_flags |= ttf_flag_applemode;
	}
      break;
      case 3:	/* PostScript & OpenType bitmaps */
	old_ps_flags = d->ps_flags;
	flags = old_psotb_flags = d->psotb_flags;
      break;
    }

    if ( !d->family )
	err = _DoSave(d->sf,temp,sizes,0x80000000,d->map);
    else
	err = !WriteMacFamily(temp,sfs,oldformatstate,oldbitmapstate,flags,d->map);

    free(temp);
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
    unichar_t buffer[200];
    const unichar_t *rcb[3]; unichar_t rcmn[2];

    rcb[2]=NULL;
#if defined(FONTFORGE_CONFIG_GDRAW)
    rcb[0] = GStringGetResource( _STR_Replace, &rcmn[0]);
#elif defined(FONTFORGE_CONFIG_GTK)
    rcb[0] =  _("Replace");
#endif
#if defined(FONTFORGE_CONFIG_GDRAW)
    rcb[1] = GStringGetResource( _STR_Cancel, &rcmn[1]);
#elif defined(FONTFORGE_CONFIG_GTK)
    rcb[1] =  _("Cancel");
#endif

#if defined(FONTFORGE_CONFIG_GDRAW)
    u_strcpy(buffer, GStringGetResource(_STR_Fileexistspre,NULL));
#elif defined(FONTFORGE_CONFIG_GTK)
    u_strcpy(buffer, _("File, "));
#endif
    u_strcat(buffer, u_GFileNameTail(gio->path));
#if defined(FONTFORGE_CONFIG_GDRAW)
    u_strcat(buffer, GStringGetResource(_STR_Fileexistspost,NULL));
#elif defined(FONTFORGE_CONFIG_GTK)
    u_strcat(buffer, _(", exists. Replace it?"));
#endif
#if defined(FONTFORGE_CONFIG_GDRAW)
    if ( GWidgetAsk(GStringGetResource(_STR_Fileexists,NULL),rcb,rcmn,0,1,buffer)==0 ) {
#elif defined(FONTFORGE_CONFIG_GTK)
    if ( GWidgetAsk(_("File Exists"),rcb,rcmn,0,1,buffer)==0 ) {
#endif
	DoSave(d,gio->path);
    }
    GFileChooserReplaceIO(d->gfc,NULL);
}

static void _GFD_SaveOk(struct gfc_data *d) {
    GGadget *tf;
    unichar_t *ret;
    int formatstate = GGadgetGetFirstListSelectedItem(d->pstype);

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

static int GFD_Options(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int fs = GGadgetGetFirstListSelectedItem(d->pstype);
	int bf = GGadgetGetFirstListSelectedItem(d->bmptype);
	int iscid = fs==ff_cid || fs==ff_cffcid || fs==ff_otfcid ||
		fs==ff_otfciddfont || fs==ff_type42cid;
	int which;
	if ( fs==ff_none )
	    which = 1;		/* Some bitmaps get stuffed int ttf files */
	else if ( fs<=ff_cffcid )
	    which = 0;		/* Postscript options */
	else if ( fs<=ff_ttfdfont )
	    which = 1;		/* truetype options */ /* type42 also */
	else
	    which = 2;		/* opentype options */
	if ( bf == bf_otb && which==0 )
	    which = 3;		/* postscript options with opentype bitmap options */
	SaveOptionsDlg(d,which,iscid);
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
    unichar_t buffer[500];

#if defined(FONTFORGE_CONFIG_GDRAW)
    u_strcpy(buffer, GStringGetResource(_STR_Couldntcreatedir,NULL));
#elif defined(FONTFORGE_CONFIG_GTK)
    u_strcpy(buffer, _("Couldn't create directory"));
#endif
    uc_strcat(buffer,": ");
    u_strcat(buffer, u_GFileNameTail(gio->path));
    uc_strcat(buffer, ".\n");
    if ( gio->error!=NULL ) {
	u_strcat(buffer,gio->error);
	uc_strcat(buffer, "\n");
    }
    if ( gio->status[0]!='\0' )
	u_strcat(buffer,gio->status);
#if defined(FONTFORGE_CONFIG_GDRAW)
    GWidgetPostNotice(GStringGetResource(_STR_Couldntcreatedir,NULL),buffer);
#elif defined(FONTFORGE_CONFIG_GTK)
    GWidgetPostNotice(_("Couldn't create directory"),buffer);
#endif
    GFileChooserReplaceIO(d->gfc,NULL);
}

static int GFD_NewDir(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	unichar_t *newdir;
	newdir = GWidgetAskStringR(_STR_Createdir,NULL,_STR_Dirname);
	if ( newdir==NULL )
return( true );
	if ( !u_GFileIsAbsolute(newdir)) {
	    unichar_t *temp = u_GFileAppendFile(GFileChooserGetDir(d->gfc),newdir,false);
	    free(newdir);
	    newdir = temp;
	}
	GIOmkDir(GFileChooserReplaceIO(d->gfc,
		GIOCreate(newdir,d,GFD_dircreated,GFD_dircreatefailed)));
	free(newdir);
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
    uc_strcpy(pt,bitmapextensions[bf]);
    GGadgetSetTitle(d->gfc,dup);
    free(dup);
}

static int GFD_Format(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	unichar_t *pt, *dup, *tpt, *ret;
	int format = GGadgetGetFirstListSelectedItem(d->pstype);
	int len, bf;
	static unichar_t nullstr[] = { 0 };
	GTextInfo **list;
	SplineFont *temp;

	list = GGadgetGetList(d->bmptype,&len);
	temp = d->sf->cidmaster ? d->sf->cidmaster : d->sf;
	if ( format==ff_none ) {
	    if ( temp->bitmaps!=NULL ) {
		list[bf_sfnt_dfont]->disabled = false;
		list[bf_ttf]->disabled = false;
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
	uc_strcpy(pt,extensions[format]);
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
	    if ( bf==bf_ttf ||
		    bf==bf_nfntmacbin /*|| bf==bf_nfntdfont*/ )
		GGadgetSelectOneListItem(d->bmptype,bf_bdf);
	    if ( format==ff_pfbmacbin )
		GGadgetSelectOneListItem(d->bmptype,bf_nfntmacbin);
	    list[bf_ttf]->disabled = true;
	    bf = GGadgetGetFirstListSelectedItem(d->bmptype);
	    GGadgetSetEnabled(d->bmpsizes, format!=ff_multiple && bf!=bf_none );	/* We know there are bitmaps */
	} else {
	    list[bf_ttf]->disabled = false;
	    if ( bf==bf_none )
		/* Do nothing, always appropriate */;
	    else if ( format==ff_ttf || format==ff_ttfsym || format==ff_otf ||
		    format==ff_otfcid )
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

static int e_h(GWindow gw, GEvent *event) {
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
	GFileChooserPopupCheck(d->gfc,event);
    } else if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button==4 || event->u.mouse.button==5) ) {
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

typedef SplineFont *SFArray[48];

int SFGenerateFont(SplineFont *sf,int family,EncMap *map) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[16+2*48+2];
    GTextInfo label[16+2*48+2];
    struct gfc_data d;
    GGadget *pulldown, *files, *tf;
    int i, j, k, old, ofs, y, fc, dupfc, dupstyle;
    int bs = GIntGetResource(_NUM_Buttonsize), bsbigger, totwid, spacing;
    SplineFont *temp;
    int familycnt=0;
    int fondcnt = 0, fondmax = 10;
    SFArray *familysfs=NULL;
    uint16 psstyle;

    if ( (alwaysgenapple || family ) && alwaysgenopentype ) {
	old_otf_flags |= ttf_flag_applemode | ttf_flag_otmode;
	old_ttf_flags |= ttf_flag_applemode | ttf_flag_otmode;
    } else if ( alwaysgenapple || family ) {
	old_otf_flags |= ttf_flag_applemode;
	old_ttf_flags |= ttf_flag_applemode;
	old_otf_flags &=~ttf_flag_otmode;
	old_ttf_flags &=~ttf_flag_otmode;
    } else if ( alwaysgenopentype ) {
	old_otf_flags |= ttf_flag_otmode;
	old_ttf_flags |= ttf_flag_otmode;
	old_otf_flags &=~ttf_flag_applemode;
	old_ttf_flags &=~ttf_flag_applemode;
    }

    if ( family ) {
	/* I could just disable the menu item, but I think it's a bit confusing*/
	/*  and I want people to know why they can't generate a family */
	FontView *fv;
	SplineFont *dup=NULL/*, *badenc=NULL*/;
	familysfs = galloc((fondmax=10)*sizeof(SFArray));
	memset(familysfs[0],0,sizeof(familysfs[0]));
	familysfs[0][0] = sf;
	fondcnt = 1;
	for ( fv=fv_list; fv!=NULL; fv=fv->next )
	    if ( fv->sf!=sf && strcmp(fv->sf->familyname,sf->familyname)==0 ) {
		MacStyleCode(fv->sf,&psstyle);
		if ( fv->sf->fondname==NULL ) {
		    fc = 0;
		    if ( familysfs[0][0]->fondname==NULL &&
			    (familysfs[0][psstyle]==NULL || familysfs[0][psstyle]==fv->sf))
			familysfs[0][psstyle] = fv->sf;
		    else {
			for ( fc=0; fc<fondcnt; ++fc ) {
			    for ( i=0; i<48; ++i )
				if ( familysfs[fc][i]!=NULL )
			    break;
			    if ( i<48 && familysfs[fc][i]->fondname==NULL &&
				    familysfs[fc][psstyle]==fv->sf )
			break;
			}
		    }
		} else {
		    for ( fc=0; fc<fondcnt; ++fc ) {
			for ( i=0; i<48; ++i )
			    if ( familysfs[fc][i]!=NULL )
			break;
			if ( i<48 && familysfs[fc][i]->fondname!=NULL &&
				strcmp(familysfs[fc][i]->fondname,fv->sf->fondname)==0 ) {
			    if ( familysfs[fc][psstyle]==fv->sf )
				/* several windows may point to same font */; 
			    else if ( familysfs[fc][psstyle]!=NULL ) {
				dup = fv->sf;
			        dupstyle = psstyle;
			        dupfc = fc;
			    } else
				familysfs[fc][psstyle] = fv->sf;
		    break;
			}
		    }
		}
		if ( fc==fondcnt ) {
		    /* Create a new fond containing just this font */
		    if ( fondcnt>=fondmax )
			familysfs = grealloc(familysfs,(fondmax+=10)*sizeof(SFArray));
		    memset(familysfs[fondcnt],0,sizeof(SFArray));
		    familysfs[fondcnt++][psstyle] = fv->sf;
		}
	    }
	for ( fc=0; fc<fondcnt; ++fc ) for ( i=0; i<48; ++i ) {
	    if ( familysfs[fc][i]!=NULL ) {
		++familycnt;
#if 0
		if ( familysfs[fc][i]->encoding_name!=map->enc )
		    badenc = fv->sf;
#endif
	    }
	}
	if ( MacStyleCode(sf,NULL)!=0 || familycnt==0 || sf->multilayer ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GWidgetErrorR(_STR_BadFamilyForMac,_STR_BadMacFamily);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Bad Mac Family"),_("To generate a Mac family file, the current font must have plain (Normal, Regular, etc.) style, and there must be other open fonts with the same family name."));
#endif
return( 0 );
	} else if ( dup ) {
	    MacStyleCode(dup,&psstyle);
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GWidgetErrorR(_STR_BadFamilyForMac,_STR_TwoFontsSameStyle,
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Bad Mac Family"),_("There are two open fonts with the current family name and the same style. %.30s and %.30s"),
#endif
		dup->fontname, familysfs[dupfc][dupstyle]->fontname);
return( 0 );
#if 0
	} else if ( badenc ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GWidgetErrorR(_STR_BadFamilyForMac,_STR_DifferentEncodings,
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Bad Mac Family"),_("The font %1$.30s has a different encoding than that of %2$.30s"),
#endif
		badenc->fontname, sf->fontname );
return( 0 );
#endif
	}
    }

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
#if defined(FONTFORGE_CONFIG_GDRAW)
    wattrs.window_title = GStringGetResource(family?_STR_GenerateMac:_STR_Generate,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
    wattrs.window_title = family?_("Generate Mac Family...");
#endif
    pos.x = pos.y = 0;
    totwid = GGadgetScale(295);
    bsbigger = 4*bs+4*14>totwid; totwid = bsbigger?4*bs+4*12:totwid;
    spacing = (totwid-4*bs-2*12)/3;
    pos.width = GDrawPointsToPixels(NULL,totwid);
    if ( family )
	pos.height = GDrawPointsToPixels(NULL,285+13+26*familycnt);
    else
	pos.height = GDrawPointsToPixels(NULL,285);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; gcd[0].gd.pos.width = 100*totwid/GIntGetResource(_NUM_ScaleFactor)-24; gcd[0].gd.pos.height = 182;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].creator = GFileChooserCreate;

    y = 252;
    if ( family )
	y += 13 + 26*familycnt;

    gcd[1].gd.pos.x = 12; gcd[1].gd.pos.y = y-3;
    gcd[1].gd.pos.width = -1;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[1].text = (unichar_t *) _STR_Save;
    label[1].text_in_resource = true;
    gcd[1].gd.mnemonic = 'S';
    gcd[1].gd.label = &label[1];
    gcd[1].gd.handle_controlevent = GFD_SaveOk;
    gcd[1].creator = GButtonCreate;

    gcd[2].gd.pos.x = -(spacing+bs)*100/GIntGetResource(_NUM_ScaleFactor)-12; gcd[2].gd.pos.y = y;
    gcd[2].gd.pos.width = -1;
    gcd[2].gd.flags = gg_visible | gg_enabled;
    label[2].text = (unichar_t *) _STR_Filter;
    label[2].text_in_resource = true;
    gcd[2].gd.mnemonic = 'F';
    gcd[2].gd.label = &label[2];
    gcd[2].gd.handle_controlevent = GFileChooserFilterEh;
    gcd[2].creator = GButtonCreate;

    gcd[3].gd.pos.x = -12; gcd[3].gd.pos.y = y; gcd[3].gd.pos.width = -1; gcd[3].gd.pos.height = 0;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[3].text = (unichar_t *) _STR_Cancel;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.mnemonic = 'C';
    gcd[3].gd.handle_controlevent = GFD_Cancel;
    gcd[3].creator = GButtonCreate;

    gcd[4].gd.pos.x = (spacing+bs)*100/GIntGetResource(_NUM_ScaleFactor)+12; gcd[4].gd.pos.y = y; gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled;
    label[4].text = (unichar_t *) _STR_New;
    label[4].text_in_resource = true;
    label[4].image = &_GIcon_dir;
    label[4].image_precedes = false;
    gcd[4].gd.mnemonic = 'N';
    gcd[4].gd.label = &label[4];
    gcd[4].gd.handle_controlevent = GFD_NewDir;
    gcd[4].creator = GButtonCreate;

    gcd[5].gd.pos.x = 12; gcd[5].gd.pos.y = 218; gcd[5].gd.pos.width = 0; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled;
    label[5].text = (unichar_t *) _STR_Options;
    label[5].text_in_resource = true;
#if defined(FONTFORGE_CONFIG_GDRAW)
    gcd[5].gd.popup_msg = GStringGetResource(_STR_OptionsPopup,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
    gcd[5].gd.popup_msg = _("Allows you to select optional behavior when generating the font");
#endif
    gcd[5].gd.label = &label[5];
    gcd[5].gd.handle_controlevent = GFD_Options;
    gcd[5].creator = GButtonCreate;

    gcd[6].gd.pos.x = 12; gcd[6].gd.pos.y = 190; gcd[6].gd.pos.width = 0; gcd[6].gd.pos.height = 0;
    gcd[6].gd.flags = gg_visible | gg_enabled ;
    gcd[6].gd.u.list = formattypes;
    gcd[6].creator = GListButtonCreate;
    for ( i=0; i<sizeof(formattypes)/sizeof(formattypes[0])-1; ++i )
	formattypes[i].disabled = sf->onlybitmaps;
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
	formattypes[ff_cid].disabled = true;
	formattypes[ff_ttf].disabled = true;
	formattypes[ff_ttfsym].disabled = true;
	formattypes[ff_ttfmacbin].disabled = true;
	formattypes[ff_ttfdfont].disabled = true;
	formattypes[ff_otfdfont].disabled = true;
	formattypes[ff_otf].disabled = true;
	formattypes[ff_otfcid].disabled = true;
	formattypes[ff_cffcid].disabled = true;
	if ( ofs!=ff_svg )
	    ofs = ff_ptype3;
    } else if ( sf->strokedfont ) {
	formattypes[ff_ttf].disabled = true;
	formattypes[ff_ttfsym].disabled = true;
	formattypes[ff_ttfmacbin].disabled = true;
	formattypes[ff_ttfdfont].disabled = true;
	if ( ofs==ff_ttf || ofs==ff_ttfsym || ofs==ff_ttfmacbin || ofs==ff_ttfdfont )
	    ofs = ff_otf;
    }
    if ( family ) {
	if ( ofs==ff_pfa || ofs==ff_pfb || ofs==ff_multiple || ofs==ff_ptype3 ||
		ofs==ff_ptype0 || ofs==ff_mma || ofs==ff_mmb )
	    ofs = ff_pfbmacbin;
	else if ( ofs==ff_cid || ofs==ff_otfcid || ofs==ff_cffcid )
	    ofs = ff_otfciddfont;
	else if ( ofs==ff_ttf || ofs==ff_ttfsym )
	    ofs = ff_ttfmacbin;
	else if ( ofs==ff_otf || ofs==ff_cff )
	    ofs = ff_otfdfont;
	formattypes[ff_pfa].disabled = true;
	formattypes[ff_pfb].disabled = true;
	formattypes[ff_mma].disabled = true;
	formattypes[ff_mmb].disabled = true;
	formattypes[ff_multiple].disabled = true;
	formattypes[ff_ptype3].disabled = true;
	formattypes[ff_ptype0].disabled = true;
	formattypes[ff_ttf].disabled = true;
	formattypes[ff_ttfsym].disabled = true;
	formattypes[ff_otf].disabled = true;
	formattypes[ff_otfcid].disabled = true;
	formattypes[ff_cff].disabled = true;
	formattypes[ff_cffcid].disabled = true;
	formattypes[ff_svg].disabled = true;
    }
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
    old = oldbitmapstate;
    if ( family ) {
	if ( old==bf_bdf || old==bf_fon ) {
	    if ( ofs==ff_otfdfont || ofs==ff_otfciddfont || ofs==ff_ttfdfont )
		old = bf_ttf;
	    else
		old = bf_nfntmacbin;
	} else if ( old==bf_nfntmacbin &&
		    ( ofs==ff_otfdfont || ofs==ff_otfciddfont || ofs==ff_ttfdfont ))
	    old = bf_ttf;
#if 0
	else if ( old==bf_nfntdfont &&
		    ( ofs!=ff_otfdfont && ofs!=ff_otfciddfont && ofs!=ff_ttfdfont ))
	    old = bf_nfntmacbin;
#endif
	bitmaptypes[bf_bdf].disabled = true;
	bitmaptypes[bf_fon].disabled = true;
    }
    temp = sf->cidmaster ? sf->cidmaster : sf;
    if ( temp->bitmaps==NULL ) {
	old = bf_none;
	bitmaptypes[bf_bdf].disabled = true;
	bitmaptypes[bf_ttf].disabled = true;
	bitmaptypes[bf_sfnt_dfont].disabled = true;
	bitmaptypes[bf_nfntmacbin].disabled = true;
	/*bitmaptypes[bf_nfntdfont].disabled = true;*/
	bitmaptypes[bf_fon].disabled = true;
	bitmaptypes[bf_otb].disabled = true;
    } else if ( ofs!=ff_none )
	bitmaptypes[bf_sfnt_dfont].disabled = true;
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

    k = 10;
    if ( family ) {
	y = 250;

	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = y;
	gcd[k].gd.pos.width = totwid-5-5;
	gcd[k].gd.flags = gg_visible | gg_enabled ;
	gcd[k++].creator = GLineCreate;
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
	    y+=26;
	    ++j;
	}

	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = y;
	gcd[k].gd.pos.width = totwid-5-5;
	gcd[k].gd.flags = gg_visible | gg_enabled ;
	gcd[k++].creator = GLineCreate;

	free(familysfs);
    }

    GGadgetsCreate(gw,gcd);
    GGadgetSetUserData(gcd[2].ret,gcd[0].ret);
    free(label[9].text);
    for ( i=11; i<k; ++i ) if ( gcd[i].gd.popup_msg!=NULL )
	free((unichar_t *) gcd[i].gd.popup_msg);

    GFileChooserConnectButtons(gcd[0].ret,gcd[1].ret,gcd[2].ret);
    {
	char *fn = sf->cidmaster==NULL? sf->fontname:sf->cidmaster->fontname;
	unichar_t *temp = galloc(sizeof(unichar_t)*(strlen(fn)+30));
	uc_strcpy(temp,fn);
	uc_strcat(temp,extensions[ofs]!=NULL?extensions[ofs]:bitmapextensions[old]);
	GGadgetSetTitle(gcd[0].ret,temp);
	free(temp);
    }
    GFileChooserGetChildren(gcd[0].ret,&pulldown,&files,&tf);
    GWidgetIndicateFocusGadget(tf);
#if __Mac
	/* The name of the postscript file is fixed and depends solely on */
	/*  the font name. If the user tried to change it, the font would */
	/*  not be found */
	GGadgetSetVisible(tf,ofs!=ff_pfbmacbin);
#endif

    memset(&d,'\0',sizeof(d));
    d.sf = sf;
    d.map = map;
    d.family = family;
    d.familycnt = familycnt-1;		/* Don't include the "normal" instance */
    d.gfc = gcd[0].ret;
    d.pstype = gcd[6].ret;
    d.bmptype = gcd[8].ret;
    d.bmpsizes = gcd[9].ret;
    d.gw = gw;

    d.ps_flags = old_ps_flags;
    d.ttf_flags = old_ttf_flags;
    d.otf_flags = old_otf_flags;
    d.psotb_flags = old_ps_flags | (old_psotb_flags&~ps_flag_mask);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
return(d.ret);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
