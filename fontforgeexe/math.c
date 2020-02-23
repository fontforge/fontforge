/* Copyright (C) 2007-2012 by George Williams */
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

#include "fontforgeui.h"
#include "fvfonts.h"
#include "gkeysym.h"
#include "mathconstants.h"
#include "splineutil.h"
#include "ustring.h"
#include "utype.h"

#include <math.h>
#include <stddef.h>

extern struct math_constants_descriptor math_constants_descriptor[];

static char *aspectnames[] = {
    N_("Constants"),
    N_("Sub/Superscript"),
    N_("Limits"),
    N_("Stacks"),
    N_("Fractions"),
    N_("Over/Underbars"),
    N_("Radicals"),
    N_("Connectors"),
    NULL
};

static char *GlyphConstruction_Dlg(GGadget *g, int r, int c);
static char *MKChange_Dlg(GGadget *g, int r, int c);
static void extpart_finishedit(GGadget *g, int r, int c, int wasnew);
static void italic_finishedit(GGadget *g, int r, int c, int wasnew);
static void topaccent_finishedit(GGadget *g, int r, int c, int wasnew);
static void mathkern_initrow(GGadget *g, int r);

static GTextInfo truefalse[] = {
    { (unichar_t *) N_("false"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("true"),  NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};

static struct col_init exten_shape_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Glyph") },
    { me_enum, NULL, truefalse, NULL, N_("Is Extended Shape") },
    COL_INIT_EMPTY
};

static struct col_init italic_cor_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Glyph") },
    { me_int, NULL, NULL, NULL, N_("Italic Correction") },
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    COL_INIT_EMPTY
};

static struct col_init top_accent_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Glyph") },
    { me_int, NULL, NULL, NULL, N_("Top Accent Horiz. Pos") },
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    COL_INIT_EMPTY
};

static struct col_init glyph_variants_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Glyph") },
    { me_string, NULL, NULL, NULL, N_("Pre-Built Larger Variants") },
    COL_INIT_EMPTY
};

static struct col_init glyph_construction_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Glyph") },
/* GT: Italic correction */
    { me_int, NULL, NULL, NULL, N_("I.C.") },
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_funcedit, GlyphConstruction_Dlg, NULL, NULL, N_("Parts List") },
    COL_INIT_EMPTY
};

static struct col_init math_kern_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Glyph") },
    { me_button, MKChange_Dlg, NULL, NULL, N_("Height/Kern Data") },
    COL_INIT_EMPTY
};

struct matrixinit mis[] = {
    { sizeof(exten_shape_ci)/sizeof(struct col_init)-1, exten_shape_ci, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    { sizeof(italic_cor_ci)/sizeof(struct col_init)-1, italic_cor_ci, 0, NULL, NULL, NULL, italic_finishedit, NULL, NULL, NULL },
    { sizeof(top_accent_ci)/sizeof(struct col_init)-1, top_accent_ci, 0, NULL, NULL, NULL, topaccent_finishedit, NULL, NULL, NULL },
    { sizeof(math_kern_ci)/sizeof(struct col_init)-1, math_kern_ci, 0, NULL, mathkern_initrow, NULL, NULL, NULL, NULL, NULL },
    { sizeof(glyph_variants_ci)/sizeof(struct col_init)-1, glyph_variants_ci, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    { sizeof(glyph_construction_ci)/sizeof(struct col_init)-1, glyph_construction_ci, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    { sizeof(glyph_variants_ci)/sizeof(struct col_init)-1, glyph_variants_ci, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    { sizeof(glyph_construction_ci)/sizeof(struct col_init)-1, glyph_construction_ci, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    MATRIXINIT_EMPTY
};

static struct col_init extensionpart[] = {
    { me_string , NULL, NULL, NULL, N_("Glyph") },
    { me_enum, NULL, truefalse, NULL, N_("Extender") },
/* GT: "Len" is an abreviation for "Length" */
    { me_int, NULL, NULL, NULL, N_("StartLen") },
    { me_int, NULL, NULL, NULL, N_("EndLen") },
    { me_int, NULL, NULL, NULL, N_("FullLen") },
    COL_INIT_EMPTY
};
static struct matrixinit mi_extensionpart =
    { sizeof(extensionpart)/sizeof(struct col_init)-1, extensionpart, 0, NULL, NULL, NULL, extpart_finishedit, NULL, NULL, NULL };

static struct col_init mathkern[] = {
    { me_int , NULL, NULL, NULL, N_("Height") },
    { me_int, NULL, NULL, NULL, N_("Kern") },
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Height Adjusts") },
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Kern Adjusts") },
    COL_INIT_EMPTY
};
static struct matrixinit mi_mathkern =
    { sizeof(mathkern)/sizeof(struct col_init)-1, mathkern, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL };


#define CID_Exten	1000
#define CID_Italic	1001
#define CID_TopAccent	1002
#define CID_MathKern	1003
#define CID_VGlyphVar	1004
#define CID_VGlyphConst	1005
#define CID_HGlyphVar	1006
#define CID_HGlyphConst	1007

static char *gi_aspectnames[] = {
    N_("Exten Shapes"),
    N_("Italic Correction"),
    N_("Top Accent"),
    N_("Math Kern"),
    N_("Vert. Variants"),
    N_("Vert. Construction"),
    N_("Hor. Variants"),
    N_("Hor. Construction"),
    NULL
};

static char *cornernames[] = {
    N_("Top Right"),
    N_("Top Left"),
    N_("Bottom Right"),
    N_("Bottom Left"),
    NULL
};

void MathInit(void) {
    int i, j;
    static int inited = false;
    static struct col_init *ci[] = { exten_shape_ci, italic_cor_ci,
	    top_accent_ci, glyph_variants_ci, glyph_construction_ci,
	    extensionpart, math_kern_ci, mathkern, NULL };
    static GTextInfo *tis[] = { truefalse, NULL };
    static char **chars[] = { aspectnames, gi_aspectnames, cornernames, NULL };

    if ( inited )
return;

    for ( j=0; chars[j]!=NULL; ++j )
	for ( i=0; chars[j][i]!=NULL; ++i )
	    chars[j][i] = _(chars[j][i]);
    for ( i=0; math_constants_descriptor[i].ui_name!=NULL; ++i ) {
	math_constants_descriptor[i].ui_name=_(math_constants_descriptor[i].ui_name);
	if ( math_constants_descriptor[i].message != NULL )
	    math_constants_descriptor[i].message=_(math_constants_descriptor[i].message);
    }
    
    for ( j=0; tis[j]!=NULL; ++j )
	for ( i=0; tis[j][i].text!=NULL; ++i )
	    tis[j][i].text = (unichar_t *) _((char *) tis[j][i].text);
    for ( j=0; ci[j]!=NULL; ++j )
	for ( i=0; ci[j][i].title!=NULL; ++i )
	    ci[j][i].title = _(ci[j][i].title);
	    
    inited = true;
}

static int GV_StringCheck(SplineFont *sf,char *str) {
    char *start, *pt;
    int scnt, pcnt, ch;
    SplineChar *sc;

    pcnt = 0;
    for ( start = str ; ; ) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
return( pcnt );
	for ( pt=start; *pt!=':' && *pt!=' ' && *pt!='\0' ; ++pt );
	ch = *pt;
	if ( ch==' ' || ch=='\0' )
return( -1 );
	if ( sf!=NULL ) {
	    *pt = '\0';
	    sc = SFGetChar(sf,-1,start);
	    *pt = ch;
	    if ( sc==NULL )
return( -1 );
	}
	scnt = 0;
	while ( *pt!=' ' && *pt!='\0' ) {
	    if ( *pt==':' ) ++scnt;
	    else if ( !isdigit( *pt ))
return( -1 );
	    ++pt;
	}
	if ( scnt!=4 )
return( -1 );
	++pcnt;
	start = pt;
    }
}

static struct glyphvariants *GV_FromString(struct glyphvariants *gv,char *str) {
    int pcnt = GV_StringCheck(NULL,str);
    char *start, *pt;
    int ch, temp;

    if ( pcnt<=0 )
return( gv );
    if ( gv==NULL )
	gv = chunkalloc(sizeof(struct glyphvariants));
    gv->part_cnt = pcnt;
    gv->parts = calloc(pcnt,sizeof(struct gv_part));
    pcnt = 0;
    for ( start = str ; ; ) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
return( gv );
	for ( pt=start; *pt!=':' ; ++pt );
	ch = *pt; *pt = '\0';
	gv->parts[pcnt].component = copy(start);
	*pt = ch;
	sscanf(pt,":%d:%hd:%hd:%hd", &temp,
		&gv->parts[pcnt].startConnectorLength,
		&gv->parts[pcnt].endConnectorLength,
		&gv->parts[pcnt].fullAdvance);
	gv->parts[pcnt].is_extender = temp;
	while ( *pt!=' ' && *pt!='\0' ) ++pt;
	++pcnt;
	start = pt;
    }
}

static char *GV_ToString(struct glyphvariants *gv) {
    int i, len;
    char buffer[80], *str;

    if ( gv==NULL || gv->part_cnt==0 )
return( NULL );
    for ( i=len=0; i<gv->part_cnt; ++i ) {
	len += strlen(gv->parts[i].component);
	sprintf( buffer, ":%d:%d:%d:%d ", gv->parts[i].is_extender,
		gv->parts[i].startConnectorLength,
		gv->parts[i].endConnectorLength,
		gv->parts[i].fullAdvance);
	len += strlen( buffer );
    }
    str = malloc(len+1);
    for ( i=len=0; i<gv->part_cnt; ++i ) {
	strcpy(str+len,gv->parts[i].component);
	len += strlen(gv->parts[i].component);
	sprintf( buffer, ":%d:%d:%d:%d ", gv->parts[i].is_extender,
		gv->parts[i].startConnectorLength,
		gv->parts[i].endConnectorLength,
		gv->parts[i].fullAdvance);
	strcpy(str+len,buffer);
	len += strlen( buffer );
    }
    if ( len!=0 )
	str[len-1] = '\0';
    else
	*str = '\0';
return( str );
}

static int SF_NameListCheck(SplineFont *sf,char *list) {
    char *start, *pt;
    int ch;
    SplineChar *sc;

    if ( list==NULL )
return( true );

    for ( start = list ; ; ) {
	while ( *start== ' ' ) ++start;
	if ( *start=='\0' )
return( true );
	for ( pt=start ; *pt!=' ' && *pt!='\0' && *pt!='('; ++pt );
	ch = *pt; *pt = '\0';
	sc = SFGetChar(sf,-1,start);
	*pt = ch;
	if ( ch=='(' ) {
	    while ( *pt!=')' && *pt!='\0' ) ++pt;
	    if ( *pt==')' ) ++pt;
	}
	start = pt;
	if ( sc==NULL )
return( false );
    }
}

typedef struct mathdlg {
    GWindow gw;
    SplineFont *sf;
    int def_layer;
    struct MATH *math;
    uint8 done;
    uint8 ok;
    uint16 popup_r;
    GGadget *popup_g;
    /* Used by glyphconstruction_dlg */
    SplineChar *sc;
    int is_horiz;
} MathDlg;

static unichar_t **MATH_GlyphNameCompletion(GGadget *t,int from_tab) {
    MathDlg *math = GDrawGetUserData(GDrawGetParentWindow(GGadgetGetWindow(t)));
    SplineFont *sf = math->sf;

return( SFGlyphNameCompletion(sf,t,from_tab,false));
}

static unichar_t **MATH_GlyphListCompletion(GGadget *t,int from_tab) {
    MathDlg *math = GDrawGetUserData(GDrawGetParentWindow(GGadgetGetWindow(t)));
    SplineFont *sf = math->sf;

return( SFGlyphNameCompletion(sf,t,from_tab,true));
}

static void MATH_Init(MathDlg *math) {
    int i, cnt, ccnt, ta, h;
    char buffer[20];
    GGadget *g;
    struct matrix_data *mds;
    SplineFont *sf = math->sf;
    SplineChar *sc;
    int cols;
    char *change = _("Change");

    for ( i=0; math_constants_descriptor[i].ui_name!=NULL; ++i ) {
	GGadget *tf = GWidgetGetControl(math->gw,2*i+1);
	int16 *pos = (int16 *) (((char *) (math->math)) + math_constants_descriptor[i].offset );

	sprintf( buffer, "%d", *pos );
	GGadgetSetTitle8(tf,buffer);
	if ( math_constants_descriptor[i].devtab_offset >= 0 ) {
	    GGadget *tf2 = GWidgetGetControl(math->gw,2*i+2);
	    DeviceTable **devtab = (DeviceTable **) (((char *) (math->math)) + math_constants_descriptor[i].devtab_offset );
	    char *str;

	    DevTabToString(&str,*devtab);
	    if ( str!=NULL )
		GGadgetSetTitle8(tf2,str);
	    free(str);
	}
    }

    /* Extension Shapes */
    for ( i=cnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL )
	if ( sc->is_extended_shape )
	    ++cnt;
    mds = calloc(cnt*2,sizeof(struct matrix_data));
    for ( i=cnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL )
	if ( sc->is_extended_shape ) {
	    mds[2*cnt+0].u.md_str = copy(sc->name);
	    mds[2*cnt+1].u.md_ival = true;
	    ++cnt;
	}
    GMatrixEditSet(GWidgetGetControl(math->gw,CID_Exten), mds,cnt,false);

    /* Italic Correction && Top Angle Horizontal Position */
    for ( ta=0; ta<2; ++ta ) {
	g = GWidgetGetControl(math->gw,ta?CID_TopAccent:CID_Italic );
	cols = GMatrixEditGetColCnt(g);
	for ( i=cnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    if ( (ta==0 && sc->italic_correction!=TEX_UNDEF ) ||
		    (ta==1 && sc->top_accent_horiz!=TEX_UNDEF))
		++cnt;
	}
	mds = calloc(cnt*cols,sizeof(struct matrix_data));
	for ( i=cnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    if ( ta==0 && sc->italic_correction!=TEX_UNDEF ) {
		mds[cols*cnt+0].u.md_str = copy(sc->name);
		mds[cols*cnt+1].u.md_ival = sc->italic_correction;
		DevTabToString(&mds[cols*cnt+2].u.md_str,sc->italic_adjusts);
		++cnt;
	    } else if ( ta==1 &&sc->top_accent_horiz!=TEX_UNDEF ) {
		mds[cols*cnt+0].u.md_str = copy(sc->name);
		mds[cols*cnt+1].u.md_ival = sc->top_accent_horiz;
		DevTabToString(&mds[cols*cnt+2].u.md_str,sc->top_accent_adjusts);
		++cnt;
	    }
	}
	GMatrixEditSet(g, mds,cnt,false);
    }

    /* Math Kern */
    g = GWidgetGetControl(math->gw,CID_MathKern);
    cols = GMatrixEditGetColCnt(g);
    for ( i=cnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL )
	if ( sc->mathkern!=NULL )
	    ++cnt;
    mds = calloc(cnt*cols,sizeof(struct matrix_data));
    for ( i=cnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL )
	if ( sc->mathkern!=NULL ) {
	    mds[cols*cnt+0].u.md_str = copy(sc->name);
	    mds[cols*cnt+1].u.md_str = copy(change);
	    ++cnt;
	}
    GMatrixEditSet(g, mds,cnt,false);

    /* Horizontal/Vertical Glyph Variants */
    for ( h=0; h<2; ++h ) {
	g = GWidgetGetControl(math->gw,CID_VGlyphVar+2*h);
	cols = GMatrixEditGetColCnt(g);
	for ( i=cnt=ccnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    struct glyphvariants *gv = h ? sc->horiz_variants : sc->vert_variants;
	    if ( gv!=NULL && gv->variants!=NULL )
		++cnt;
	    if ( gv!=NULL && gv->part_cnt!=0 )
		++ccnt;
	}
	mds = calloc(cnt*cols,sizeof(struct matrix_data));
	for ( i=cnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    struct glyphvariants *gv = h ? sc->horiz_variants : sc->vert_variants;
	    if ( gv!=NULL && gv->variants!=NULL ) {
		mds[cols*cnt+0].u.md_str = SCNameUniStr(sc);
		mds[cols*cnt+1].u.md_str = SFNameList2NameUni(sf,gv->variants);
		++cnt;
	    }
	}
	GMatrixEditSet(g, mds,cnt,false);

	/* Glyph Construction */
	g = GWidgetGetControl(math->gw,CID_VGlyphConst+2*h);
	cols = GMatrixEditGetColCnt(g);
	mds = calloc(ccnt*cols,sizeof(struct matrix_data));
	for ( i=cnt=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    struct glyphvariants *gv = h ? sc->horiz_variants : sc->vert_variants;
	    if ( gv!=NULL && gv->part_cnt!=0 ) {
		mds[cols*cnt+0].u.md_str = copy(sc->name);
		mds[cols*cnt+1].u.md_ival = gv->italic_correction;
		DevTabToString(&mds[cols*cnt+2].u.md_str,gv->italic_adjusts);
		mds[cols*cnt+cols-1].u.md_str = GV_ToString(gv);
		++cnt;
	    }
	}
	GMatrixEditSet(g, mds,cnt,false);
    }
}

static void MATH_FreeImage(const void *_math, GImage *img) {
    GImageDestroy(img);
}

static GImage *_MATHVar_GetImage(const void *_math) {
    MathDlg *math = (MathDlg *) _math;
    GGadget *varlist = math->popup_g;
    int rows, cols = GMatrixEditGetColCnt(varlist);
    struct matrix_data *old = GMatrixEditGet(varlist,&rows);
    SplineChar *sc = SFGetChar(math->sf,-1, old[cols*math->popup_r].u.md_str);
    static OTLookup dummyl = OTLOOKUP_EMPTY;
    static struct lookup_subtable dummys = LOOKUP_SUBTABLE_EMPTY;

    dummyl.lookup_type = gsub_multiple;
    dummys.lookup = &dummyl;
return( PST_GetImage(varlist,math->sf,math->def_layer,&dummys,math->popup_r,sc) );
}

static void MATHVar_PopupPrepare(GGadget *g, int r, int c) {
    MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *old = GMatrixEditGet(g,&rows);

    if ( c<0 || c>=cols || r<0 || r>=rows || old[cols*r].u.md_str==NULL ||
	SFGetChar(math->sf,-1, old[cols*r+0].u.md_str)==NULL )
return;
    math->popup_r = r;
    math->popup_g = g;
    GGadgetPreparePopupImage(GGadgetGetWindow(g),NULL,math,_MATHVar_GetImage,MATH_FreeImage);
}

static GImage *_MATHConst_GetImage(const void *_math) {
    MathDlg *math = (MathDlg *) _math;
    GGadget *varlist = math->popup_g;
    int rows, cols = GMatrixEditGetColCnt(varlist);
    struct matrix_data *old = GMatrixEditGet(varlist,&rows);
    SplineChar *sc = SFGetChar(math->sf,-1, old[cols*math->popup_r].u.md_str);
    struct glyphvariants *gv = GV_FromString(NULL,old[cols*math->popup_r+cols-1].u.md_str);
    GImage *ret;

    ret = GV_GetConstructedImage(sc,math->def_layer,gv,GGadgetGetCid(varlist)==CID_HGlyphConst);
    GlyphVariantsFree(gv);
return( ret );
}

static void MATHConst_PopupPrepare(GGadget *g, int r, int c) {
    MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *old = GMatrixEditGet(g,&rows);

    if ( c<0 || c>=cols || r<0 || r>=rows || old[cols*r].u.md_str==NULL ||
	SFGetChar(math->sf,-1, old[cols*r+0].u.md_str)==NULL )
return;
    math->popup_r = r;
    math->popup_g = g;
    GGadgetPreparePopupImage(GGadgetGetWindow(g),NULL,math,_MATHConst_GetImage,MATH_FreeImage);
}

static GImage *_MATHLine_GetImage(const void *_math) {
    MathDlg *math = (MathDlg *) _math;
    GGadget *varlist = math->popup_g;
    int rows, cols = GMatrixEditGetColCnt(varlist);
    struct matrix_data *old = GMatrixEditGet(varlist,&rows);
    SplineChar *sc = SFGetChar(math->sf,-1, old[cols*math->popup_r].u.md_str);

return( SC_GetLinedImage(sc,math->def_layer,old[cols*math->popup_r+1].u.md_ival,GGadgetGetCid(varlist)==CID_Italic));
}

static void MATHLine_PopupPrepare(GGadget *g, int r, int c) {
    MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *old = GMatrixEditGet(g,&rows);

    if ( c<0 || c>=cols || r<0 || r>=rows || old[cols*r].u.md_str==NULL ||
	SFGetChar(math->sf,-1, old[cols*r+0].u.md_str)==NULL )
return;
    math->popup_r = r;
    math->popup_g = g;
    GGadgetPreparePopupImage(GGadgetGetWindow(g),NULL,math,_MATHLine_GetImage,MATH_FreeImage);
}

static GImage *_GVC_GetImage(const void *_math) {
    MathDlg *math = (MathDlg *) _math;
    GGadget *varlist = math->popup_g;
    int rows, cols = GMatrixEditGetColCnt(varlist);
    struct matrix_data *old = GMatrixEditGet(varlist,&rows);
    GImage *ret;
    struct glyphvariants *gv;

    gv = GV_ParseConstruction(NULL,old,rows,cols);
    ret = GV_GetConstructedImage(math->sc,math->def_layer,gv,math->is_horiz);
    GlyphVariantsFree(gv);
return( ret );
}

static void italic_finishedit(GGadget *g, int r, int c, int wasnew) {
    int rows;
    struct matrix_data *stuff;
    MathDlg *math;
    int cols;
    DBounds b;
    SplineChar *sc;

    if ( c!=0 )
return;
    if ( !wasnew )
return;
    /* If they added a new glyph to the sequence then set some defaults for it. */
    /*  only the full advance has any likelyhood of being correct */
    math = GDrawGetUserData(GGadgetGetWindow(g));
    stuff = GMatrixEditGet(g, &rows);
    cols = GMatrixEditGetColCnt(g);
    if ( stuff[r*cols+0].u.md_str==NULL )
return;
    sc = SFGetChar(math->sf,-1,stuff[r*cols+0].u.md_str);
    if ( sc==NULL )
return;
    SplineCharFindBounds(sc,&b);
    if ( b.maxx>sc->width ) {
	stuff[r*cols+1].u.md_ival = rint((b.maxx-sc->width) +
			(math->sf->ascent+math->sf->descent)/16.0);
	GGadgetRedraw(g);
    }
}

static void topaccent_finishedit(GGadget *g, int r, int c, int wasnew) {
    int rows;
    struct matrix_data *stuff;
    MathDlg *math;
    int cols;
    DBounds b;
    SplineChar *sc;
    double italic_off;

    if ( c!=0 )
return;
    if ( !wasnew )
return;
    /* If they added a new glyph to the sequence then set some defaults for it. */
    /*  only the full advance has any likelyhood of being correct */
    math = GDrawGetUserData(GGadgetGetWindow(g));
    stuff = GMatrixEditGet(g, &rows);
    cols = GMatrixEditGetColCnt(g);
    if ( stuff[r*cols+0].u.md_str==NULL )
return;
    sc = SFGetChar(math->sf,-1,stuff[r*cols+0].u.md_str);
    if ( sc==NULL )
return;
    SplineCharFindBounds(sc,&b);
    italic_off = (b.maxy-b.miny)*tan(-math->sf->italicangle);
    if ( b.maxx-b.minx-italic_off < 0 )
	stuff[r*cols+1].u.md_ival = rint(b.minx + (b.maxx-b.minx)/2);
    else
	stuff[r*cols+1].u.md_ival = rint(b.minx + italic_off + (b.maxx - b.minx - italic_off)/2);
    GGadgetRedraw(g);
}

static void mathkern_initrow(GGadget *g, int r) {
    int rows;
    struct matrix_data *stuff;
    int cols;

    cols = GMatrixEditGetColCnt(g);
    stuff = GMatrixEditGet(g, &rows);
    stuff[r*cols+1].u.md_str = copy(_("Change"));
};

static void extpart_finishedit(GGadget *g, int r, int c, int wasnew) {
    int rows;
    struct matrix_data *stuff;
    MathDlg *math;
    int cols;
    DBounds b;
    double full_advance;
    SplineChar *sc;

    if ( c!=0 )
return;
    if ( !wasnew )
return;
    /* If they added a new glyph to the sequence then set some defaults for it. */
    /*  only the full advance has any likelyhood of being correct */
    math = GDrawGetUserData(GGadgetGetWindow(g));
    stuff = GMatrixEditGet(g, &rows);
    cols = GMatrixEditGetColCnt(g);
    if ( stuff[r*cols+0].u.md_str==NULL )
return;
    sc = SFGetChar(math->sf,-1,stuff[r*cols+0].u.md_str);
    if ( sc==NULL )
return;
    SplineCharFindBounds(sc,&b);
    if ( math->is_horiz )
	full_advance = b.maxx - b.minx;
    else
	full_advance = b.maxy - b.miny;
    stuff[r*cols+2].u.md_ival = stuff[r*cols+3].u.md_ival = rint(full_advance/3);
    stuff[r*cols+4].u.md_ival = rint(full_advance);
    GGadgetRedraw(g);
}

static void GVC_PopupPrepare(GGadget *g, int r, int c) {
    MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));

    math->popup_g = g;
    if ( math->sc==NULL )
return;
    GGadgetPreparePopupImage(GGadgetGetWindow(g),NULL,math,_GVC_GetImage,MATH_FreeImage);
}

static int GVC_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));
	math->done = true;
	math->ok = true;
    }
return( true );
}

static int MATH_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));
	math->done = true;
    }
return( true );
}

static int gc_e_h(GWindow gw, GEvent *event) {
    MathDlg *math = GDrawGetUserData(gw);

    if ( event->type==et_close ) {
	math->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/math.html", "#math-glyphconstruction");
return( true );
	} else if ( GMenuIsCommand(event,H_("Quit|Ctl+Q") )) {
	    MenuExit(NULL,NULL,NULL);
	} else if ( GMenuIsCommand(event,H_("Close|Ctl+Shft+Q") )) {
	    math->done = true;
	}
return( false );
    }
return( true );
}

static char *GlyphConstruction_Dlg(GGadget *g, int r, int c) {
    MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));
    MathDlg md;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *old = GMatrixEditGet(g,&rows);
    GGadgetCreateData *harray[7], mgcd[4], *varray[6], mboxes[3];
    GTextInfo mlabel[3];
    struct glyphvariants *gv;
    char *ret;

    memset(&md,0,sizeof(md));
    md.sf = math->sf;
    md.is_horiz = GGadgetGetCid(g)==CID_HGlyphConst;
    md.sc = SFGetChar(md.sf,-1,old[r*cols+0].u.md_str);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Glyph Construction");
    pos.x = pos.y = 0;
    pos.width = 100;
    pos.height = 100;
    md.gw = gw = GDrawCreateTopWindow(NULL,&pos,gc_e_h,&md,&wattrs);

    memset(mgcd,0,sizeof(mgcd));
    memset(mlabel,0,sizeof(mlabel));
    memset(mboxes,0,sizeof(mboxes));

    mgcd[0].gd.flags = gg_visible | gg_enabled;
    mgcd[0].gd.u.matrix = &mi_extensionpart;
    mgcd[0].gd.cid = CID_VGlyphConst;
    mgcd[0].creator = GMatrixEditCreate;

    mgcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    mlabel[1].text = (unichar_t *) _("_OK");
    mlabel[1].text_is_1byte = true;
    mlabel[1].text_in_resource = true;
    mgcd[1].gd.label = &mlabel[1];
    mgcd[1].gd.handle_controlevent = GVC_OK;
    mgcd[1].creator = GButtonCreate;

    mgcd[2].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    mlabel[2].text = (unichar_t *) _("_Cancel");
    mlabel[2].text_is_1byte = true;
    mlabel[2].text_in_resource = true;
    mgcd[2].gd.label = &mlabel[2];
    mgcd[2].gd.handle_controlevent = MATH_Cancel;
    mgcd[2].creator = GButtonCreate;

    harray[0] = GCD_Glue; harray[1] = &mgcd[1]; harray[2] = GCD_Glue;
    harray[3] = GCD_Glue; harray[4] = &mgcd[2]; harray[5] = GCD_Glue;
    harray[6] = NULL;

    mboxes[2].gd.flags = gg_enabled|gg_visible;
    mboxes[2].gd.u.boxelements = harray;
    mboxes[2].creator = GHBoxCreate;

    varray[0] = &mgcd[0]; varray[1] = NULL;
    varray[2] = &mboxes[2]; varray[3] = NULL;
    varray[4] = NULL;

    mboxes[0].gd.pos.x = mboxes[0].gd.pos.y = 2;
    mboxes[0].gd.flags = gg_enabled|gg_visible;
    mboxes[0].gd.u.boxelements = varray;
    mboxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,mboxes);
    GHVBoxSetExpandableRow(mboxes[0].ret,0);
    GHVBoxSetExpandableCol(mboxes[2].ret,gb_expandgluesame);
    GMatrixEditSetColumnCompletion(mgcd[0].ret,0,MATH_GlyphNameCompletion);
    GMatrixEditSetMouseMoveReporter(mgcd[0].ret,GVC_PopupPrepare);

    /* If it's unparseable, this will give 'em nothing */
    gv = GV_FromString(NULL,old[r*cols+cols-1].u.md_str);
    GV_ToMD(mgcd[0].ret,gv);
    GlyphVariantsFree(gv);

    GHVBoxFitWindow(mboxes[0].ret);

    GDrawSetVisible(md.gw,true);

    while ( !md.done )
	GDrawProcessOneEvent(NULL);

    if ( md.ok ) {
	int rs, cs = GMatrixEditGetColCnt(mgcd[0].ret);
	struct matrix_data *stuff = GMatrixEditGet(mgcd[0].ret,&rs);
	gv = GV_ParseConstruction(NULL,stuff,rs,cs);
	ret = GV_ToString(gv);
	GlyphVariantsFree(gv);
    } else
	ret = copy( old[r*cols+cols-1].u.md_str );
    GDrawDestroyWindow(md.gw);
return( ret );
}

static char *MKChange_Dlg(GGadget *g, int r, int c) {
    MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *old = GMatrixEditGet(g,&rows);
    SplineChar *sc;

    if ( old[r*cols+0].u.md_str==NULL )
return( NULL );
    sc = SFGetChar(math->sf,-1,old[r*cols+0].u.md_str);
    if ( sc==NULL )
return( NULL );

    MathKernDialog(sc,math->def_layer);
return( NULL );
}

static int MATH_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MathDlg *math = GDrawGetUserData(GGadgetGetWindow(g));
	int err=false;
	int cid,i;
	int high,low;
	SplineFont *sf = math->sf;
	SplineChar *sc;

	/* Two passes. First checks that everything is parsable */
	for ( i=0; math_constants_descriptor[i].ui_name!=NULL; ++i ) {
	    GetInt8(math->gw,2*i+1,math_constants_descriptor[i].ui_name,&err);
	    if ( err )
return( true );
	    if ( math_constants_descriptor[i].devtab_offset >= 0 ) {
		GGadget *tf2 = GWidgetGetControl(math->gw,2*i+2);
		char *str = GGadgetGetTitle8(tf2);
		if ( !DeviceTableOK(str,&low,&high)) {
		    ff_post_error(_("Bad device table"), _("Bad device table for %s"),
			    math_constants_descriptor[i].ui_name);
		    free(str);
return( true );
		}
		free(str);
	    }
	}
	/* Now check that the various glyph lists are parseable */
	for ( cid=CID_Exten; cid<=CID_HGlyphConst; ++cid ) {
	    GGadget *g = GWidgetGetControl(math->gw,cid);
	    int rows, cols = GMatrixEditGetColCnt(g);
	    struct matrix_data *old = GMatrixEditGet(g,&rows);
	    for ( i=0; i<rows; ++i ) {
		if ( SFGetChar(sf,-1,old[i*cols+0].u.md_str)==NULL ) {
		    ff_post_error(_("Missing Glyph"), _("There is no glyph named %s (used in %s)"),
			    old[i*cols+0].u.md_str, gi_aspectnames[cid-CID_Exten]);
return( true );
		}
		if ( cid==CID_Italic || cid==CID_TopAccent ||
			cid == CID_VGlyphConst || cid == CID_HGlyphConst ) {
		    if ( !DeviceTableOK(old[i*cols+2].u.md_str,&low,&high)) {
			ff_post_error(_("Bad device table"), _("Bad device table for glyph %s in %s"),
				old[i*cols+0].u.md_str, gi_aspectnames[cid-CID_Exten]);
return( true );
		    }
		}
		if ( cid == CID_VGlyphConst || cid == CID_HGlyphConst ) {
		    if ( GV_StringCheck(sf,old[i*cols+cols-1].u.md_str)==-1 ) {
			ff_post_error(_("Bad Parts List"), _("Bad parts list for glyph %s in %s"),
				old[i*cols+0].u.md_str, gi_aspectnames[cid-CID_Exten]);
return( true );
		    }
		}
		if ( cid == CID_VGlyphVar || cid == CID_HGlyphVar ) {
		    if ( !SF_NameListCheck(sf,old[i*cols+1].u.md_str)) {
			ff_post_error(_("Bad Variants List"), _("Bad Variants list for glyph %s in %s"),
				old[i*cols+0].u.md_str, gi_aspectnames[cid-CID_Exten]);
return( true );
		    }
		}
	    }
	}

	/*********************************************/
	/* Ok, if we got this far it should be legal */
	/*********************************************/
	for ( i=0; math_constants_descriptor[i].ui_name!=NULL; ++i ) {
	    int16 *pos = (int16 *) (((char *) (math->math)) + math_constants_descriptor[i].offset );
	    *pos = GetInt8(math->gw,2*i+1,math_constants_descriptor[i].ui_name,&err);

	    if ( math_constants_descriptor[i].devtab_offset >= 0 ) {
		GGadget *tf2 = GWidgetGetControl(math->gw,2*i+2);
		char *str = GGadgetGetTitle8(tf2);
		DeviceTable **devtab = (DeviceTable **) (((char *) (math->math)) + math_constants_descriptor[i].devtab_offset );

		*devtab = DeviceTableParse(*devtab,str);
		free(str);
	    }
	}
	sf->MATH = math->math;

	/* As for the per-glyph stuff... Well the only way I can insure that */
	/* things which have been removed in the dlg are removed in the font */
	/* is to clear everything now, and start from a blank slate when I   */
	/* parse stuff. (Except for math kerning which I don't support here) */
	for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	    sc->is_extended_shape = false;
	    sc->italic_correction = TEX_UNDEF;
	    sc->top_accent_horiz  = TEX_UNDEF;
	    DeviceTableFree(sc->italic_adjusts);
	    DeviceTableFree(sc->top_accent_adjusts);
	    sc->italic_adjusts = sc->top_accent_adjusts = NULL;
	    GlyphVariantsFree(sc->vert_variants);
	    GlyphVariantsFree(sc->horiz_variants);
	    sc->vert_variants = sc->horiz_variants = NULL;
	    /* MathKernFree(sc->mathkern); sc->mathkern = NULL; */
	}
	/* Then process each table to set whatever it sets */
	for ( cid=CID_Exten; cid<=CID_HGlyphConst; ++cid ) {
	    GGadget *g = GWidgetGetControl(math->gw,cid);
	    int rows, cols = GMatrixEditGetColCnt(g);
	    struct matrix_data *old = GMatrixEditGet(g,&rows);
	    for ( i=0; i<rows; ++i ) {
		sc = SFGetChar(sf,-1,old[i*cols+0].u.md_str);
		if ( cid==CID_Exten )
		    sc->is_extended_shape = old[i*cols+1].u.md_ival;
		else if ( cid==CID_Italic ) {
		    sc->italic_correction = old[i*cols+1].u.md_ival;
		    sc->italic_adjusts = DeviceTableParse(NULL,old[i*cols+2].u.md_str);
		} else if ( cid==CID_TopAccent ) {
		    sc->top_accent_horiz = old[i*cols+1].u.md_ival;
		    sc->top_accent_adjusts = DeviceTableParse(NULL,old[i*cols+2].u.md_str);
		} else if ( cid==CID_VGlyphVar || cid==CID_HGlyphVar ) {
		    struct glyphvariants **gvp = cid == CID_VGlyphVar ?
			    &sc->vert_variants : &sc->horiz_variants;
		    char *str = old[i*cols+1].u.md_str;
		    if ( str!=NULL ) while ( *str==' ' ) ++str;
		    if ( str!=NULL && *str!='\0' ) {
			*gvp = chunkalloc(sizeof(struct glyphvariants));
			(*gvp)->variants = GlyphNameListDeUnicode( str );
		    }
		} else if ( cid==CID_VGlyphConst || cid==CID_HGlyphConst ) {
		    struct glyphvariants **gvp = cid == CID_VGlyphConst ?
			    &sc->vert_variants : &sc->horiz_variants;
		    *gvp = GV_FromString(*gvp,old[cols*i+cols-1].u.md_str);
		    if ( *gvp!=NULL && (*gvp)->part_cnt!=0 ) {
			(*gvp)->italic_correction = old[i*cols+1].u.md_ival;
			(*gvp)->italic_adjusts = DeviceTableParse(NULL,old[i*cols+2].u.md_str);
		    }
		}
	    }
	}

	/* Done! */

	math->done = true;
	math->ok = true;
    }
return( true );
}

static int math_e_h(GWindow gw, GEvent *event) {
    MathDlg *math = GDrawGetUserData(gw);

    if ( event->type==et_close ) {
	math->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/dialogs/math.html", NULL);
return( true );
	} else if ( GMenuIsCommand(event,H_("Quit|Ctl+Q") )) {
	    MenuExit(NULL,NULL,NULL);
	} else if ( GMenuIsCommand(event,H_("Close|Ctl+Shft+Q") )) {
	    math->done = true;
	}
return( false );
    }
return( true );
}

#define MAX_PAGE	9
#define MAX_ROW		12

void SFMathDlg(SplineFont *sf,int def_layer) {
    MathDlg md;
    int i, j, page, row, h;
    GGadget *g;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[MAX_PAGE][MAX_ROW][3], boxes[MAX_PAGE][2],
	    *hvarray[MAX_PAGE][MAX_ROW+1][4], *harray[7], mgcd[4],
	    *varray[6], mboxes[3], gi[8][2];
    GTextInfo label[MAX_PAGE][MAX_ROW], mlabel[3];
    GTabInfo aspects[MAX_PAGE+8+1];

    MathInit();

    memset(&md,0,sizeof(md));
    if ( sf->cidmaster ) sf = sf->cidmaster;
    md.sf = sf;
    md.def_layer = def_layer;
    md.math = sf->MATH;
    if ( md.math==NULL )
	md.math = MathTableNew(sf);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("MATH table");
    pos.x = pos.y = 0;
    pos.width = 100;
    pos.height = 100;
    md.gw = gw = GDrawCreateTopWindow(NULL,&pos,math_e_h,&md,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));
    memset(boxes,0,sizeof(boxes));
    memset(aspects,0,sizeof(aspects));
    memset(gi,0,sizeof(gi));

    page = row = 0;
    for ( i=0; math_constants_descriptor[i].ui_name!=NULL; ++i ) {
	if ( math_constants_descriptor[i].new_page ) {
	    hvarray[page][row][0] = hvarray[page][row][1] = hvarray[page][row][2] = GCD_Glue;
	    hvarray[page][row][3] = NULL;
	    hvarray[page][row+1][0] = NULL;
	    ++page;
	    if ( page>=MAX_PAGE ) {
		IError( "Too many pages" );
return;
	    }
	    row = 0;
	}

	label[page][row].text = (unichar_t *) math_constants_descriptor[i].ui_name;
	label[page][row].text_is_1byte = true;
	label[page][row].text_in_resource = true;
	gcd[page][row][0].gd.label = &label[page][row];
	gcd[page][row][0].gd.flags = gg_visible | gg_enabled;
	gcd[page][row][0].gd.popup_msg = math_constants_descriptor[i].message;
	gcd[page][row][0].creator = GLabelCreate;
	hvarray[page][row][0] = &gcd[page][row][0];

	gcd[page][row][1].gd.flags = gg_visible | gg_enabled;
	gcd[page][row][1].gd.pos.width = 50;
	gcd[page][row][1].gd.cid = 2*i+1;
	gcd[page][row][1].gd.popup_msg = math_constants_descriptor[i].message;
	gcd[page][row][1].creator = GTextFieldCreate;
	hvarray[page][row][1] = &gcd[page][row][1];

	if ( math_constants_descriptor[i].devtab_offset>=0 ) {
	    gcd[page][row][2].gd.flags = gg_visible | gg_enabled;
	    gcd[page][row][2].gd.cid = 2*i+2;
	    gcd[page][row][2].gd.popup_msg = math_constants_descriptor[i].message;
	    gcd[page][row][2].creator = GTextFieldCreate;
	    hvarray[page][row][2] = &gcd[page][row][2];
	} else
	    hvarray[page][row][2] = GCD_Glue;
	hvarray[page][row][3] = NULL;

	if ( ++row>=MAX_ROW ) {
	    IError( "Too many rows" );
return;
	}
    }
    hvarray[page][row][0] = hvarray[page][row][1] = hvarray[page][row][2] = GCD_Glue;
    hvarray[page][row][3] = NULL;
    hvarray[page][row+1][0] = NULL;

    for ( i=0; aspectnames[i]!=NULL; ++i ) {
	boxes[i][0].gd.flags = gg_enabled|gg_visible;
	boxes[i][0].gd.u.boxelements = hvarray[i][0];
	boxes[i][0].creator = GHVBoxCreate;

	aspects[i].text = (unichar_t *) aspectnames[i];
	aspects[i].text_is_1byte = true;
	aspects[i].nesting = i!=0;
	aspects[i].gcd = boxes[i];
    }
    if ( i!=page+1 ) {	/* Page never gets its final increment */
	IError( "Page miscount %d in descriptor table, but only %d names.", page+1, i );
return;
    }

    for ( j=0; mis[j].col_cnt!=0; ++j ) {
	gi[j][0].gd.flags = gg_enabled|gg_visible;
	gi[j][0].gd.u.matrix = &mis[j];
	gi[j][0].gd.cid = CID_Exten+j;
	gi[j][0].creator = GMatrixEditCreate;

	aspects[i+j].text = (unichar_t *) gi_aspectnames[j];
	aspects[i+j].text_is_1byte = true;
	aspects[i+j].gcd = gi[j];
    }

    memset(mgcd,0,sizeof(mgcd));
    memset(mlabel,0,sizeof(mlabel));
    memset(mboxes,0,sizeof(mboxes));

    mgcd[0].gd.u.tabs = aspects;
    mgcd[0].gd.flags = gg_visible | gg_enabled | gg_tabset_vert;
    /*mgcd[0].gd.cid = CID_Tabs;*/
    mgcd[0].creator = GTabSetCreate;

    mgcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    mlabel[1].text = (unichar_t *) _("_OK");
    mlabel[1].text_is_1byte = true;
    mlabel[1].text_in_resource = true;
    mgcd[1].gd.label = &mlabel[1];
    mgcd[1].gd.handle_controlevent = MATH_OK;
    mgcd[1].creator = GButtonCreate;

    mgcd[2].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    mlabel[2].text = (unichar_t *) _("_Cancel");
    mlabel[2].text_is_1byte = true;
    mlabel[2].text_in_resource = true;
    mgcd[2].gd.label = &mlabel[2];
    mgcd[2].gd.handle_controlevent = MATH_Cancel;
    mgcd[2].creator = GButtonCreate;

    harray[0] = GCD_Glue; harray[1] = &mgcd[1]; harray[2] = GCD_Glue;
    harray[3] = GCD_Glue; harray[4] = &mgcd[2]; harray[5] = GCD_Glue;
    harray[6] = NULL;

    mboxes[2].gd.flags = gg_enabled|gg_visible;
    mboxes[2].gd.u.boxelements = harray;
    mboxes[2].creator = GHBoxCreate;

    varray[0] = &mgcd[0]; varray[1] = NULL;
    varray[2] = &mboxes[2]; varray[3] = NULL;
    varray[4] = NULL;

    mboxes[0].gd.pos.x = mboxes[0].gd.pos.y = 2;
    mboxes[0].gd.flags = gg_enabled|gg_visible;
    mboxes[0].gd.u.boxelements = varray;
    mboxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,mboxes);
    GHVBoxSetExpandableRow(mboxes[0].ret,0);
    GHVBoxSetExpandableCol(mboxes[2].ret,gb_expandgluesame);
    for ( i=0; aspectnames[i]!=NULL; ++i ) {
	GHVBoxSetExpandableCol(boxes[i][0].ret,2);
	GHVBoxSetExpandableRow(boxes[i][0].ret,gb_expandglue);
    }
    for ( j=0; mis[j].col_cnt!=0; ++j )
	GMatrixEditSetColumnCompletion(gi[j][0].ret,0,MATH_GlyphNameCompletion);
    for ( h=0; h<2; ++h ) {
	g = GWidgetGetControl(md.gw,CID_VGlyphVar+2*h);
	GMatrixEditSetColumnCompletion(g,1,MATH_GlyphListCompletion);
	GMatrixEditSetMouseMoveReporter(g,MATHVar_PopupPrepare);
	g = GWidgetGetControl(md.gw,CID_VGlyphConst+2*h);
	GMatrixEditSetMouseMoveReporter(g,MATHConst_PopupPrepare);
    }
    GMatrixEditSetMouseMoveReporter(GWidgetGetControl(md.gw,CID_Italic),MATHLine_PopupPrepare);
    GMatrixEditSetMouseMoveReporter(GWidgetGetControl(md.gw,CID_TopAccent),MATHLine_PopupPrepare);
    MATH_Init(&md);
    GHVBoxFitWindow(mboxes[0].ret);

    GDrawSetVisible(md.gw,true);

    while ( !md.done )
	GDrawProcessOneEvent(NULL);
    if ( sf->MATH==NULL && !md.ok )
	MATHFree(md.math);

    GDrawDestroyWindow(md.gw);
}

/* ************************************************************************** */
/* ****************************** Math Kern Dlg ***************************** */
/* ************************************************************************** */

#define CID_TopBox	1000
#define CID_Glyph	1001
#define CID_Tabs	1002
#define CID_Corners	1003
#define CID_TopRight	1004
#define CID_TopLeft	1005
#define CID_BottomRight	1006
#define CID_BottomLeft	1007

static void MKD_SetGlyphList(MathKernDlg *mkd, SplineChar *sc) {
    SplineFont *sf = sc->parent;
    int k,cnt, gid;
    GTextInfo **tis = NULL;
    SplineChar *test;

    for ( k=0; k<2; ++k ) {
	cnt = 0;
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (test=sf->glyphs[gid])!=NULL ) {
	    if ( test==sc || test->mathkern!=NULL ) {
		if ( k ) {
		    tis[cnt] = calloc(1,sizeof(GTextInfo));
		    tis[cnt]->text = utf82u_copy(test->name);
		    tis[cnt]->userdata = test;
		    tis[cnt]->selected = test==sc;
		    tis[cnt]->fg = tis[cnt]->bg = COLOR_DEFAULT;
		}
		++cnt;
	    }
	}
	if ( !k )
	    tis = malloc((cnt+1)*sizeof(GTextInfo *));
	else
	    tis[cnt] = calloc(1,sizeof(GTextInfo));
    }
    GGadgetSetList(GWidgetGetControl(mkd->gw,CID_Glyph),tis,false);
}

static void MKDSubResize(MathKernDlg *mkd, GEvent *event) {
    int width, height;
    int i;
    GRect r;

    if ( !event->u.resize.sized )
return;

    width = (event->u.resize.size.width-4*mkd->mid_space)/4;
    height = (event->u.resize.size.height-mkd->cv_y-8);
    if ( width<70 || height<80 ) {
	if ( width<70 ) width = 70;
	width = 4*(width+mkd->mid_space);
	if ( height<80 ) height = 80;
	height += mkd->cv_y+mkd->button_height+8;
	GDrawGetSize(mkd->gw,&r);
	width += r.width-event->u.resize.size.width;
	height += r.height-event->u.resize.size.height;
	GDrawResize(mkd->gw,width,height);
return;
    }
    if ( width!=mkd->cv_width || height!=mkd->cv_height ) {
	mkd->cv_width = width; mkd->cv_height = height;
	for ( i=0; i<4; ++i ) {
	    CharView *cv = (&mkd->cv_topright)+i;
	    GDrawResize(cv->gw,width,height);
	    if ( i!=0 )
		GDrawMove(cv->gw,10+i*(mkd->cv_width+mkd->mid_space),mkd->cv_y);
	}
    }

    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    GDrawRequestExpose(mkd->cvparent_w,NULL,false);
}

static void MKDTopResize(MathKernDlg *mkd, GEvent *event) {

    if ( !event->u.resize.sized )
return;

    GGadgetMove(GWidgetGetControl(mkd->gw,CID_TopBox),4,4);
    GGadgetResize(GWidgetGetControl(mkd->gw,CID_TopBox),
	    event->u.resize.size.width-8,
	    event->u.resize.size.height-12);
}


static void MKDDraw(MathKernDlg *mkd, GWindow pixmap, GEvent *event) {
    GRect r;
    int i;

    GDrawSetLineWidth(pixmap,0);
    for ( i=0; i<4; ++i ) {
	CharView *cv = (&mkd->cv_topright)+i;

	r.x = 10+i*(mkd->cv_width+mkd->mid_space)-1; r.y=mkd->cv_y-1;
	r.width = mkd->cv_width+1; r.height = mkd->cv_height+1;
	GDrawDrawRect(pixmap,&r,0);

	GDrawSetFont(pixmap,cv->inactive ? mkd->plain : mkd->bold);
	GDrawDrawText8(pixmap,r.x,5+mkd->as,cornernames[i],-1,0);
    }
}

void MKDMakeActive(MathKernDlg *mkd,CharView *cv) {
    GRect r;
    int i;

    if ( mkd==NULL )
return;
    for ( i=0; i<4; ++i )
	(&mkd->cv_topright)[i].inactive = true;
    cv->inactive = false;
    GDrawSetUserData(mkd->gw,cv);
    GDrawSetUserData(mkd->cvparent_w,cv);
    for ( i=0; i<4; ++i )
	GDrawRequestExpose((&mkd->cv_topright)[i].v,NULL,false);
    GDrawGetSize(mkd->gw,&r);
    r.x = 0;
    r.y = 0;
    r.height = mkd->fh+10;
    GDrawRequestExpose(mkd->cvparent_w,&r,false);
}

static void MKDChar(MathKernDlg *mkd, GEvent *event) {
    int i;
    for ( i=0; i<4; ++i )
	if ( !(&mkd->cv_topright)[i].inactive )
    break;

    if ( event->u.chr.keysym==GK_Tab || event->u.chr.keysym==GK_BackTab ) {
	if ( event->u.chr.keysym==GK_Tab ) ++i; else --i;
	if ( i<0 ) i=3; else if ( i>3 ) i = 0;
	MKDMakeActive(mkd,(&mkd->cv_topright)+i);
    } else
	CVChar((&mkd->cv_topright)+i,event);
}

void MKD_DoClose(struct cvcontainer *cvc) {
    MathKernDlg *mkd = (MathKernDlg *) cvc;
    int i;

    for ( i=0; i<4; ++i ) {
	SplineChar *msc = &(&mkd->sc_topright)[i];
	SplinePointListsFree(msc->layers[0].splines);
	SplinePointListsFree(msc->layers[1].splines);
	free( msc->layers );
    }

    mkd->done = true;
}

static int mkd_sub_e_h(GWindow gw, GEvent *event) {
    MathKernDlg *mkd = (MathKernDlg *) ((CharViewBase *) GDrawGetUserData(gw))->container;

    switch ( event->type ) {
      case et_expose:
	MKDDraw(mkd,gw,event);
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    MKDSubResize(mkd,event);
      break;
      case et_char:
	MKDChar(mkd,event);
      break;
    }
return( true );
}

static int mkd_e_h(GWindow gw, GEvent *event) {
    MathKernDlg *mkd = (MathKernDlg *) ((CharViewBase *) GDrawGetUserData(gw))->container;
    int i;

    switch ( event->type ) {
      case et_char:
	MKDChar(mkd,event);
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    MKDTopResize(mkd,event);
      break;
      case et_close:
	MKD_DoClose((struct cvcontainer *) mkd);
      break;
      case et_create:
      break;
      case et_map:
	for ( i=0; i<4; ++i ) {
	    CharView *cv = (&mkd->cv_topright)+i;
	    if ( !cv->inactive ) {
		if ( event->u.map.is_visible )
		    CVPaletteActivate(cv);
		else
		    CVPalettesHideIfMine(cv);
	break;
	    }
	}
	/* mkd->isvisible = event->u.map.is_visible; */
      break;
    }
return( true );
}

static void MKDFillup(MathKernDlg *mkd, SplineChar *sc) {
    int i, j, rows;
    SplineSet *last, *cur;
    RefChar *ref;
    GTextInfo **list;

    if ( mkd->last_aspect==0 ) {
	for ( i=0; i<4; ++i ) {
	    SplineChar *msc = &(&mkd->sc_topright)[i];
	    struct mathkernvertex *mkv = sc->mathkern==NULL ? NULL : &(&sc->mathkern->top_right)[i];
	    msc->width = sc->width;
	    msc->italic_correction = sc->italic_correction;
	    msc->top_accent_horiz = sc->top_accent_horiz;
	    last = NULL;
	    SplinePointListsFree(msc->layers[0].splines);
	    SplinePointListsFree(msc->layers[1].splines);
	    msc->layers[0].splines = msc->layers[1].splines = NULL;

	    /* copy the character itself into the background */
	    last = msc->layers[0].splines = SplinePointListCopy(sc->layers[ly_fore].splines);
	    if ( last!=NULL )
		while ( last->next!=NULL ) last = last->next;
	    for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
		if ( last==NULL )
		    cur = SplinePointListCopy(ref->layers[0].splines);
		if ( last==NULL )
		    msc->layers[0].splines = cur;
		else
		    last->next = cur;
		if ( cur!=NULL )
		    for ( last=cur; last->next==NULL; last = last->next );
	    }
	    /* Now copy the dots from the mathkern vertex structure */
	    last = NULL;
	    if ( mkv!=NULL ) {
		for ( j=0; j<mkv->cnt; ++j ) {
		    cur = chunkalloc(sizeof(SplineSet));
		    cur->first = cur->last = SplinePointCreate(mkv->mkd[j].kern +
			    ((i&1)?0:sc->width) +
			    ((i&2)?0:sc->italic_correction==TEX_UNDEF?0:sc->italic_correction),
			mkv->mkd[j].height );
		    cur->first->pointtype = pt_corner;
		    if ( last==NULL )
			msc->layers[ly_fore].splines = cur;
		    else
			last->next = cur;
		    last = cur;
		}
	    }
	}
    } else {
	for ( i=0; i<4; ++i ) {
	    struct mathkernvertex *mkv = sc->mathkern==NULL ? NULL : &(&sc->mathkern->top_right)[i];
	    GGadget *list = GWidgetGetControl(mkd->gw,CID_TopRight+i);
	    int cols = GMatrixEditGetColCnt(list);
	    struct matrix_data *md;

	    if ( mkv!=NULL ) {
		md = calloc(mkv->cnt*cols,sizeof(struct matrix_data));
		for ( j=0; j<mkv->cnt; ++j ) {
		    md[j*cols+0].u.md_ival = mkv->mkd[j].height;
		    md[j*cols+1].u.md_ival = mkv->mkd[j].kern;
		    DevTabToString(&md[j*cols+2].u.md_str,mkv->mkd[j].height_adjusts);
		    DevTabToString(&md[j*cols+3].u.md_str,mkv->mkd[j].kern_adjusts);
		}
		GMatrixEditSet(list, md,mkv->cnt,false);
	    } else
		GMatrixEditSet(list, NULL,0,false);
	}
    }
    mkd->cursc = sc;

    list = GGadgetGetList(GWidgetGetControl(mkd->gw,CID_Glyph),&rows);
    for ( i=rows-1; i>=0; --i )
	if ( list[i]->userdata==sc )
    break;
    if ( i>=0 )
	GGadgetSelectOneListItem(GWidgetGetControl(mkd->gw,CID_Glyph),i);
}

static void MKDFillupRefresh(MathKernDlg *mkd, SplineChar *sc) {
    int i;

    MKDFillup(mkd, sc);
    if ( mkd->last_aspect==0 ) {
	for ( i=0; i<4; ++i ) {
	    CharView *cv = &mkd->cv_topright + i;
	    GDrawRequestExpose(cv->gw,NULL,false);
	    GDrawRequestExpose(cv->v,NULL,false);
	}
    }
}

static int bp_order_height(const void *bpp1, const void *bpp2) {
    const BasePoint *bp1 = *(const BasePoint **) bpp1;
    const BasePoint *bp2 = *(const BasePoint **) bpp2;
    if ( bp1->y > bp2->y )
return( 1 );
    else if ( bp1->y < bp2->y )
return( -1 );

return( 0 );
}

static int mkd_order_height(const void *_mkd1, const void *_mkd2) {
    const struct mathkerndata *mkd1 = (const struct mathkerndata *) _mkd1;
    const struct mathkerndata *mkd2 = (const struct mathkerndata *) _mkd2;
    if ( mkd1->height > mkd2->height )
return( 1 );
    else if ( mkd1->height < mkd2->height )
return( -1 );

return( 0 );
}

static int MKD_Parse(MathKernDlg *mkd) {
    int i, cnt, j, k;
    SplineSet *ss;
    SplinePoint *sp;
    BasePoint **bases;
    int allzeroes = true;

    if ( mkd->cursc->mathkern==NULL )
	mkd->cursc->mathkern = chunkalloc(sizeof(struct mathkern));

    if ( mkd->last_aspect==0 ) {		/* Graphical view is current */
	for ( i=0; i<4; ++i ) {
	    SplineChar *msc = &(&mkd->sc_topright)[i];
	    struct mathkernvertex *mkv = &(&mkd->cursc->mathkern->top_right)[i];

	    for ( k=0; k<2; ++k ) {
		cnt = 0;
		for ( ss = msc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
		    for ( sp=ss->first ; ; ) {
			if ( k )
			    bases[cnt] = &sp->me;
			++cnt;
			if ( sp->next == NULL )
		    break;
			sp = sp->next->to;
			if ( sp == ss->first )
		    break;
		    }
		}
		if ( !k )
		    bases = malloc(cnt*sizeof(BasePoint *));
	    }
	    qsort(bases,cnt,sizeof(BasePoint *),bp_order_height);
	    if ( cnt>mkv->cnt ) {
		mkv->mkd = realloc(mkv->mkd,cnt*sizeof(struct mathkerndata));
		memset(mkv->mkd+mkv->cnt,0,(cnt-mkv->cnt)*sizeof(struct mathkerndata));
	    }
	    for ( j=0; j<cnt; ++j ) {
		bases[j]->x = rint(bases[j]->x);
		if ( !(i&1) ) bases[j]->x -= mkd->cursc->width;
		if ( !(i&2) ) bases[j]->x -= mkd->cursc->italic_correction==TEX_UNDEF?0:mkd->cursc->italic_correction;
		bases[j]->y = rint(bases[j]->y);
		/* If we have a previous entry with this height retain the height dv */
		/* If we have a previous entry with this height and width retain the width dv too */
		for ( k=j; k<mkv->cnt; ++k )
		    if ( bases[j]->y == mkv->mkd[k].height )
		break;
		if ( k!=j ) {
		    DeviceTableFree(mkv->mkd[j].height_adjusts);
		    DeviceTableFree(mkv->mkd[j].kern_adjusts);
		    mkv->mkd[j].height_adjusts = mkv->mkd[j].kern_adjusts = NULL;
		}
		if ( k<mkv->cnt ) {
		    mkv->mkd[j].height_adjusts = mkv->mkd[k].height_adjusts;
		    if ( bases[j]->x == mkv->mkd[k].kern )
			mkv->mkd[j].kern_adjusts = mkv->mkd[k].kern_adjusts;
		    else {
			DeviceTableFree(mkv->mkd[k].kern_adjusts);
			mkv->mkd[k].kern_adjusts = NULL;
		    }
		    if ( j!=k )
			mkv->mkd[k].height_adjusts = mkv->mkd[k].kern_adjusts = NULL;
		}
		mkv->mkd[j].height = bases[j]->y;
		mkv->mkd[j].kern   = bases[j]->x;
	    }
	    for ( ; j<mkv->cnt; ++j ) {
		DeviceTableFree(mkv->mkd[j].height_adjusts);
		DeviceTableFree(mkv->mkd[j].kern_adjusts);
		mkv->mkd[j].height_adjusts = mkv->mkd[j].kern_adjusts = NULL;
	    }
	    mkv->cnt = cnt;
	    free(bases);
	    if ( cnt!=0 )
		allzeroes = false;
	}
    } else {
	int low, high;
	/* Parse the textual info */
	for ( i=0; i<4; ++i ) {
	    GGadget *list = GWidgetGetControl(mkd->gw,CID_TopRight+i);
	    int rows, cols = GMatrixEditGetColCnt(list);
	    struct matrix_data *old = GMatrixEditGet(list,&rows);

	    for ( j=0; j<rows; ++j ) {
		if ( !DeviceTableOK(old[j*cols+2].u.md_str,&low,&high) ||
			!DeviceTableOK(old[j*cols+3].u.md_str,&low,&high)) {
		    ff_post_error(_("Bad device table"), _("Bad device table for in row %d of %s"),
			    j, cornernames[i]);
return( false );
		}
	    }
	}
	for ( i=0; i<4; ++i ) {
	    struct mathkernvertex *mkv = &(&mkd->cursc->mathkern->top_right)[i];
	    GGadget *list = GWidgetGetControl(mkd->gw,CID_TopRight+i);
	    int rows, cols = GMatrixEditGetColCnt(list);
	    struct matrix_data *old = GMatrixEditGet(list,&rows);

	    for ( j=0; j<mkv->cnt; ++j ) {
		DeviceTableFree(mkv->mkd[j].height_adjusts);
		DeviceTableFree(mkv->mkd[j].kern_adjusts);
		mkv->mkd[j].height_adjusts = mkv->mkd[j].kern_adjusts = NULL;
	    }
	    if ( rows>mkv->cnt ) {
		mkv->mkd = realloc(mkv->mkd,rows*sizeof(struct mathkerndata));
		memset(mkv->mkd+mkv->cnt,0,(rows-mkv->cnt)*sizeof(struct mathkerndata));
	    }
	    for ( j=0; j<rows; ++j ) {
		mkv->mkd[j].height = old[j*cols+0].u.md_ival;
		mkv->mkd[j].kern   = old[j*cols+1].u.md_ival;
		mkv->mkd[j].height_adjusts = DeviceTableParse(NULL,old[j*cols+2].u.md_str);
		mkv->mkd[j].kern_adjusts   = DeviceTableParse(NULL,old[j*cols+3].u.md_str);
	    }
	    qsort(mkv->mkd,rows,sizeof(struct mathkerndata),mkd_order_height);
	    mkv->cnt = rows;
	    if ( rows!=0 )
		allzeroes=false;
	}
    }
    if ( allzeroes ) {
	MathKernFree(mkd->cursc->mathkern);
	mkd->cursc->mathkern = NULL;
    }
    /* The only potential error is two entries with the same height, and I don't */
    /*  check for that */
return( true );
}

static int MKD_AspectChange(GGadget *g, GEvent *e) {
    if ( e==NULL || (e->type==et_controlevent && e->u.control.subtype == et_radiochanged )) {
	MathKernDlg *mkd = (MathKernDlg *) (((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container);
	int new_aspect = GTabSetGetSel(g);

	if ( new_aspect == mkd->last_aspect )
return( true );

	GGadgetSetEnabled(mkd->mb,new_aspect==0);

	if ( new_aspect==0 ) {
	    /* We are moving from textual to graphical. Parse text, clear old */
	    /*  points, set new points */
	} else {
	    /* We are moving from graphical to textual. */
	    if ( !mkd->saved_mathkern ) {
		mkd->orig_mathkern = MathKernCopy(mkd->cursc->mathkern);
		mkd->saved_mathkern = true;
	    }
	}
	MKD_Parse(mkd);
	mkd->last_aspect = new_aspect;
	MKDFillup(mkd,mkd->cursc);
    }
return( true );
}

static int MathKernD_GlyphChanged(GGadget *g, GEvent *e) {
    MathKernDlg *mkd = (MathKernDlg *) (((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container);
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	GTextInfo *sel = GGadgetGetListItemSelected(g);

	if ( sel!=NULL && MKD_Parse(mkd)) {
	    SplineChar *sc = sel->userdata;
	    MKDFillupRefresh(mkd, sc);
	}
    }
return( true );
}

static int MathKernD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MathKernDlg *mkd = (MathKernDlg *) (((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container);
	if ( mkd->saved_mathkern ) {
	    MathKernFree(mkd->cursc->mathkern);
	    mkd->cursc->mathkern = mkd->orig_mathkern;
	}
	MKD_DoClose(((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container);
    }
return( true );
}

static int MathKernD_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MathKernDlg *mkd = (MathKernDlg *) (((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container);
	if ( MKD_Parse(mkd) ) {
	    MathKernFree(mkd->orig_mathkern);
	    mkd->orig_mathkern = NULL;
	    mkd->saved_mathkern = false;
	    MKD_DoClose( (struct cvcontainer *) mkd );
	}
    }
return( true );
}

static int MKD_Can_Navigate(struct cvcontainer *cvc, enum nav_type type) {
return( true );
}

static int MKD_Can_Open(struct cvcontainer *cvc) {
return( false );
}

static void MKD_Do_Navigate(struct cvcontainer *cvc, enum nav_type type) {
    MathKernDlg *mkd = ( MathKernDlg * ) cvc;
    SplineChar *sc = NULL;
    int pos;
    GGadget *list = GWidgetGetControl(mkd->gw,CID_Glyph);
    int32 rows;
    GTextInfo **tis;

    if ( !MKD_Parse(mkd))
return;
    MathKernFree(mkd->orig_mathkern);
    mkd->orig_mathkern = NULL;
    mkd->saved_mathkern = false;

    if ( type == nt_goto ) {
	SplineFont *sf = mkd->cursc->parent;
	int enc = GotoChar(sf,sf->fv->map,NULL);
	if ( enc==-1 || sf->fv->map->map[enc]==-1 || (sc = sf->glyphs[ sf->fv->map->map[enc] ])==NULL )
return;
	if ( sc->mathkern==NULL )
	    MKD_SetGlyphList(mkd,sc);
    } else if ( type == nt_next || type == nt_nextdef ) {
	tis = GGadgetGetList(list,&rows);
	for ( pos=rows-1; pos>=0; --pos )
	    if ( tis[pos]->selected )
	break;
	++pos;
	if ( pos==rows )
return;
	sc = tis[pos]->userdata;
    } else {
	tis = GGadgetGetList(list,&rows);
	for ( pos=rows-1; pos>=0; --pos )
	    if ( tis[pos]->selected )
	break;
	if ( pos<=0 )
return;
	--pos;
	sc = tis[pos]->userdata;
    }
    MKDFillupRefresh(mkd,sc);
}

static SplineFont *SF_Of_MKD(struct cvcontainer *foo) {
return( NULL );
}

struct cvcontainer_funcs mathkern_funcs = {
    cvc_mathkern,
    (void (*) (struct cvcontainer *cvc,CharViewBase *cv)) MKDMakeActive,
    (void (*) (struct cvcontainer *cvc,void *)) MKDChar,
    MKD_Can_Navigate,
    MKD_Do_Navigate,
    MKD_Can_Open,
    MKD_DoClose,
    SF_Of_MKD
};

static void MKDInit(MathKernDlg *mkd,SplineChar *sc) {
    int i;

    memset(mkd,0,sizeof(*mkd));
    mkd->base.funcs = &mathkern_funcs;

    for ( i=0; i<4; ++i ) {
	SplineChar *msc = &(&mkd->sc_topright)[i];
	CharView *mcv = &(&mkd->cv_topright)[i];
	msc->orig_pos = i;
	msc->unicodeenc = -1;
	msc->name = i==0 ? _("TopRight") :
		    i==1 ? _("TopLeft")  :
		    i==2 ? _("BottomRight"):
			    _("BottomLeft");
	msc->parent = &mkd->dummy_sf;
	msc->layer_cnt = 2;
	msc->layers = calloc(2,sizeof(Layer));
	LayerDefault(&msc->layers[0]);
	LayerDefault(&msc->layers[1]);
	mkd->chars[i] = msc;

	mcv->b.sc = msc;
	mcv->b.layerheads[dm_fore] = &msc->layers[ly_fore];
	mcv->b.layerheads[dm_back] = &msc->layers[ly_back];
	mcv->b.layerheads[dm_grid] = &mkd->dummy_sf.grid;
	mcv->b.drawmode = dm_fore;
	mcv->b.container = (struct cvcontainer *) mkd;
	mcv->inactive = i!=0;
    }
    mkd->dummy_sf.glyphs = mkd->chars;
    mkd->dummy_sf.glyphcnt = mkd->dummy_sf.glyphmax = 4;
    mkd->dummy_sf.pfminfo.fstype = -1;
    mkd->dummy_sf.pfminfo.stylemap = -1;
    mkd->dummy_sf.fontname = mkd->dummy_sf.fullname = mkd->dummy_sf.familyname = "dummy";
    mkd->dummy_sf.weight = "Medium";
    mkd->dummy_sf.origname = "dummy";
    mkd->dummy_sf.ascent = sc->parent->ascent;
    mkd->dummy_sf.descent = sc->parent->descent;
    mkd->dummy_sf.layers = mkd->layerinfo;
    mkd->dummy_sf.layer_cnt = 2;
    mkd->layerinfo[ly_back].order2 = sc->layers[ly_back].order2;
    mkd->layerinfo[ly_back].name = _("Back");
    mkd->layerinfo[ly_fore].order2 = sc->layers[ly_fore].order2;
    mkd->layerinfo[ly_fore].name = _("Fore");
    mkd->dummy_sf.grid.order2 = sc->layers[ly_back].order2;
    mkd->dummy_sf.anchor = NULL;

    mkd->dummy_sf.fv = (FontViewBase *) &mkd->dummy_fv;
    mkd->dummy_fv.b.active_layer = ly_fore;
    mkd->dummy_fv.b.sf = &mkd->dummy_sf;
    mkd->dummy_fv.b.selected = mkd->sel;
    mkd->dummy_fv.cbw = mkd->dummy_fv.cbh = default_fv_font_size+1;
    mkd->dummy_fv.magnify = 1;

    mkd->dummy_fv.b.map = &mkd->dummy_map;
    mkd->dummy_map.map = mkd->map;
    mkd->dummy_map.backmap = mkd->backmap;
    mkd->dummy_map.enccount = mkd->dummy_map.encmax = mkd->dummy_map.backmax = 4;
    mkd->dummy_map.enc = &custom;
}

void MathKernDialog(SplineChar *sc,int def_layer) {
    MathKernDlg mkd;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[6], boxes[4], *harray[8], *varray[5], *garray[5];
    GGadgetCreateData cgcd[4][2], tabsetgcd[2];
    GTextInfo label[6];
    GTabInfo aspects[3], corners[5];
    FontRequest rq;
    int as, ds, ld;
    int i,k;
    static GFont *mathfont = NULL, *mathbold=NULL;

    MathInit();
    MKDInit( &mkd, sc );
    mkd.def_layer = def_layer;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_isdlg|wam_restrict|wam_undercursor|wam_utf8_wtitle;
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Math Kerning");
    pos.width = 600;
    pos.height = 400;
    mkd.gw = gw = GDrawCreateTopWindow(NULL,&pos,mkd_e_h,&mkd.cv_topright,&wattrs);

    if ( mathfont==NULL ) {
	memset(&rq,0,sizeof(rq));
	rq.utf8_family_name = SANS_UI_FAMILIES;
	rq.point_size = 12;
	rq.weight = 400;
	mathfont = GDrawInstanciateFont(NULL,&rq);
	mathfont = GResourceFindFont("Math.Font",mathfont);

	GDrawDecomposeFont(mathfont, &rq);
	rq.weight = 700;
	mathbold = GDrawInstanciateFont(NULL,&rq);
	mathbold = GResourceFindFont("Math.BoldFont",mathbold);
    }
    mkd.plain = mathfont;
    mkd.bold = mathbold;
    GDrawWindowFontMetrics(mkd.gw,mkd.plain,&as,&ds,&ld);
    mkd.fh = as+ds; mkd.as = as;

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));
    memset(&aspects,'\0',sizeof(aspects));
    memset(&corners,'\0',sizeof(corners));
    memset(&cgcd,0,sizeof(cgcd));
    memset(&tabsetgcd,0,sizeof(tabsetgcd));

    for ( k=0; k<4; ++k ) {
	cgcd[k][0].gd.flags = gg_visible | gg_enabled;
	cgcd[k][0].gd.u.matrix = &mi_mathkern;
	cgcd[k][0].gd.cid = CID_TopRight+k;
	cgcd[k][0].creator = GMatrixEditCreate;

	corners[k].text = (unichar_t *) cornernames[k];
	corners[k].text_is_1byte = true;
	corners[k].gcd = cgcd[k];
    }

    tabsetgcd[0].gd.flags = gg_visible|gg_enabled|gg_tabset_vert ;
    tabsetgcd[0].gd.u.tabs = corners;
    tabsetgcd[0].gd.cid = CID_Corners;
    /*tabsetgcd[0].gd.handle_controlevent = MKD_AspectChange;*/
    tabsetgcd[0].creator = GTabSetCreate;

    k = 0;
    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.pos.height = 18; gcd[k].gd.pos.width = 20;
    gcd[k++].creator = GSpacerCreate;

    aspects[0].text = (unichar_t *) _("Graphical");
    aspects[0].text_is_1byte = true;
    aspects[0].gcd = NULL;

    aspects[1].text = (unichar_t *) _("Textual");
    aspects[1].text_is_1byte = true;
    aspects[1].gcd = tabsetgcd;

    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.u.tabs = aspects;
    gcd[k].gd.cid = CID_Tabs;
    gcd[k].gd.handle_controlevent = MKD_AspectChange;
    gcd[k++].creator = GTabSetCreate;

    gcd[k].gd.flags = gg_visible|gg_enabled ;
    gcd[k].gd.cid = CID_Glyph;
    gcd[k].gd.handle_controlevent = MathKernD_GlyphChanged;
    gcd[k++].creator = GListButtonCreate;

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_but_default;
    gcd[k].gd.handle_controlevent = MathKernD_OK;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Done");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_but_cancel;
    gcd[k].gd.handle_controlevent = MathKernD_Cancel;
    gcd[k++].creator = GButtonCreate;

    harray[0] = GCD_Glue; harray[1] = &gcd[k-2]; harray[2] = GCD_Glue;
    harray[3] = GCD_Glue; harray[4] = &gcd[k-1]; harray[5] = GCD_Glue;
    harray[6] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray;
    boxes[2].creator = GHBoxCreate;

    garray[0] = &gcd[k-3]; garray[1] = GCD_Glue; garray[2] = NULL;
    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = garray;
    boxes[3].creator = GHBoxCreate;

    varray[0] = &gcd[0];
    varray[1] = &gcd[1];
    varray[2] = &boxes[3];
    varray[3] = &boxes[2];
    varray[4] = NULL;

    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].gd.cid = CID_TopBox;
    boxes[0].creator = GVBoxCreate;

    GGadgetsCreate(gw,boxes);

    mkd.cvparent_w = GTabSetGetSubwindow(gcd[1].ret,0);
    GDrawSetEH(mkd.cvparent_w,mkd_sub_e_h);
    MKDCharViewInits(&mkd);

    MKD_SetGlyphList(&mkd, sc);
    MKDFillup( &mkd, sc );

    GHVBoxSetExpandableRow(boxes[0].ret,1);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GGadgetResize(boxes[0].ret,pos.width,pos.height);

    mkd.button_height = GDrawPointsToPixels(gw,60);
    GDrawResize(gw,1000,400);		/* Force a resize event */

    GDrawSetVisible(mkd.gw,true);

    while ( !mkd.done )
	GDrawProcessOneEvent(NULL);

    for ( i=0; i<4; ++i ) {
	CharView *cv = &mkd.cv_topright + i;
	if ( cv->backimgs!=NULL ) {
	    GDrawDestroyWindow(cv->backimgs);
	    cv->backimgs = NULL;
	}
	CVPalettesHideIfMine(cv);
    }
    GDrawDestroyWindow(mkd.gw);
}
