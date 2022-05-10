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

#include "bvedit.h"
#include "cvimages.h"
#include "cvundoes.h"
#include "fontforgeui.h"
#include "fvimportbdf.h"
#include "gkeysym.h"
#include "sd.h"
#include "spiro.h"
#include "splinefill.h"
#include "ustring.h"
#include "utype.h"

#include <dirent.h>
#include <math.h>
#include <sys/types.h>

#define CID_AccTar       1000
#define CID_JoinLimitVal 1001

static void ImportPS(CharView *cv,char *path,ImportParams *ip) {
    FILE *ps = fopen(path,"r");

    if ( ps==NULL )
return;
    SCImportPSFile(cv->b.sc,CVLayer((CharViewBase *) cv),ps,false,ip);
    fclose(ps);
}

static void ImportPDF(CharView *cv,char *path,ImportParams *ip) {
    FILE *pdf = fopen(path,"r");

    if ( pdf==NULL )
return;
    SCImportPDFFile(cv->b.sc,CVLayer((CharViewBase *) cv),pdf,false,ip);
    fclose(pdf);
}

static void ImportPlate(CharView *cv,char *path,ImportParams *ip) {
    FILE *plate = fopen(path,"r");

    if ( plate==NULL )
return;
    SCImportPlateFile(cv->b.sc,CVLayer((CharViewBase *) cv),plate,false,ip);
    fclose(plate);
}

static void ImportSVG(CharView *cv,char *path,ImportParams *ip) {
    SCImportSVG(cv->b.sc,CVLayer((CharViewBase *) cv),path,NULL,0,false,ip);
}

static void ImportGlif(CharView *cv,char *path,ImportParams *ip) {
    SCImportGlif(cv->b.sc,CVLayer((CharViewBase *) cv),path,NULL,0,false,ip);
}

static void ImportFig(CharView *cv,char *path,ImportParams *ip) {
    SCImportFig(cv->b.sc,CVLayer((CharViewBase *) cv),path,false,ip);
}

static void ImportImage(CharView *cv,char *path,ImportParams *ip) {
    GImage *image;
    int layer;

    image = GImageRead(path);
    if ( image==NULL ) {
	ff_post_error(_("Bad image file"),_("Bad image file: %.100s"), path);
return;
    }
    layer = ly_back;
    if ( cv->b.drawmode!=dm_grid ) {
	if ( cv->b.sc->parent->multilayer )
	    layer = cv->b.drawmode-dm_back + ly_back;
	else if ( cv->b.layerheads[cv->b.drawmode]->background )
	    layer = CVLayer( (CharViewBase *) cv);
    }
    SCAddScaleImage(cv->b.sc,image,false,layer,ip);
}

static int BVImportImage(BitmapView *bv,char *path) {
    GImage *image;
    struct _GImage *base;
    int tot;
    uint8_t *pt, *end;
    BDFChar *bc = bv->bc;
    int i, j;

    image = GImageRead(path);
    if ( image==NULL ) {
	ff_post_error(_("Bad image file"),_("Bad image file: %.100s"), path);
return(false);
    }
    base = image->list_len==0?image->u.image:image->u.images[0];
    BCPreserveState(bc);
    BCFlattenFloat(bc);
    free(bc->bitmap);
    bc->xmin = bc->ymin = 0;
    bc->xmax = base->width-1; bc->ymax = base->height-1;
    if ( !bc->byte_data && base->image_type==it_mono ) {
	bc->bitmap = base->data;
	bc->bytes_per_line = base->bytes_per_line;

	/* Sigh. Bitmaps use a different defn of set than images do. make it consistent */
	tot = bc->bytes_per_line*(bc->ymax-bc->ymin+1);
	for ( pt = bc->bitmap, end = pt+tot; pt<end; *pt++ ^= 0xff );

	base->data = NULL;
    } else if ( base->image_type==it_mono /* && byte_data */) {
	int set = (1<<BDFDepth(bv->bdf))-1;
	bc->bytes_per_line = base->width;
	bc->bitmap = calloc(base->height,base->width);
	for ( i=0; i<base->height; ++i ) for ( j=0; j<base->width; ++j ) {
	    if ( !(base->data[i*base->bytes_per_line+(j>>3)]&(0x80>>(j&7))) )
		bc->bitmap[i*bc->bytes_per_line+j] = set;
	}
    } else if ( !bc->byte_data && base->image_type==it_true ) {
	bc->bytes_per_line = (base->width>>3)+1;
	bc->bitmap = calloc(base->height,bc->bytes_per_line);
	for ( i=0; i<base->height; ++i ) for ( j=0; j<base->width; ++j ) {
	    int col = ((Color *) (base->data+i*base->bytes_per_line))[j];
	    col = (3*COLOR_RED(col)+6*COLOR_GREEN(col)+COLOR_BLUE(col));
	    if ( col<=5*256 )
		bc->bitmap[i*bc->bytes_per_line+(j>>3)] |= (0x80>>(j&7));
	}
    } else if ( /* byte_data && */ base->image_type==it_true ) {
	int div = 255/((1<<BDFDepth(bv->bdf))-1);
	bc->bytes_per_line = base->width;
	bc->bitmap = calloc(base->height,base->width);
	for ( i=0; i<base->height; ++i ) for ( j=0; j<base->width; ++j ) {
	    int col = ((Color *) (base->data+i*base->bytes_per_line))[j];
	    col = 255-(3*COLOR_RED(col)+6*COLOR_GREEN(col)+COLOR_BLUE(col)+5)/10;
	    bc->bitmap[i*bc->bytes_per_line+j] = col/div;
	}
    } else if ( bc->byte_data /* && indexed */ ) {
	int div = 255/((1<<BDFDepth(bv->bdf))-1);
	bc->bitmap = base->data;
	bc->bytes_per_line = base->bytes_per_line;
	for ( i=0; i<base->height; ++i ) for ( j=0; j<base->width; ++j ) {
	    int col = base->clut->clut[base->data[i*base->bytes_per_line+j]];
	    col = 255-(3*COLOR_RED(col)+6*COLOR_GREEN(col)+COLOR_BLUE(col)+5)/10;
	    base->data[i*base->bytes_per_line+j] = col/div;
	}
	base->data = NULL;
    } else /* if ( mono && indexed ) */ {
	bc->bytes_per_line = (base->width>>3)+1;
	bc->bitmap = calloc(base->height,bc->bytes_per_line);
	for ( i=0; i<base->height; ++i ) for ( j=0; j<base->width; ++j ) {
	    int col = base->clut->clut[base->data[i*base->bytes_per_line+j]];
	    col = (3*COLOR_RED(col)+6*COLOR_GREEN(col)+COLOR_BLUE(col));
	    if ( col<=5*256 )
		bc->bitmap[i*bc->bytes_per_line+(j>>3)] |= (0x80>>(j&7));
	}
    }
    GImageDestroy(image);
    if ( bc->sc!=NULL )
	bc->sc->widthset = true;
    BCCharChangedUpdate(bc);
return( true );
}


static unichar_t wildimg[] = { '*', '.', '{',
#ifndef _NO_LIBUNGIF
'g','i','f',',',
#endif
#ifndef _NO_LIBPNG
'p','n','g',',',
#endif
#ifndef _NO_LIBTIFF
't','i','f','f',',',
't','i','f',',',
#endif
#ifndef _NO_LIBJPEG
'j','p','e','g',',',
'j','p','g',',',
#endif
'x','p','m',',', 'x','b','m',',', 'b','m','p', '}', '\0' };
static unichar_t wildtemplate[] = { '{','u','n','i',',','u',',','c','i','d',',','e','n','c','}','[','0','-','9','a','-','f','A','-','F',']','*', '.', '{',
#ifndef _NO_LIBUNGIF
'g','i','f',',',
#endif
#ifndef _NO_LIBPNG
'p','n','g',',',
#endif
#ifndef _NO_LIBTIFF
't','i','f','f',',',
't','i','f',',',
#endif
'x','p','m',',', 'x','b','m',',', 'b','m','p', '}', '\0' };
/* Hmm. Mac seems to use the extension 'art' for eps files sometimes */
static unichar_t wildepstemplate[] = { '{','u','n','i',',','u',',','c','i','d',',','e','n','c','}','[','0','-','9','a','-','f','A','-','F',']','*', '.', '{', 'p','s',',', 'e','p','s',',','a','r','t','}',  0 };
static unichar_t wildpdftemplate[] = { '{','u','n','i',',','u',',','c','i','d',',','e','n','c','}','[','0','-','9','a','-','f','A','-','F',']','*', '.', 'p', 'd','f',  0 };
static unichar_t wildsvgtemplate[] = { '{','u','n','i',',','u',',','c','i','d',',','e','n','c','}','[','0','-','9','a','-','f','A','-','F',']','*', '.', 's', 'v','g',  0 };
static unichar_t wildgliftemplate[] = { '{','u','n','i',',','u',',','c','i','d',',','e','n','c','}','[','0','-','9','a','-','f','A','-','F',']','*', '.', 'g', 'l','i','f',  0 };
static unichar_t wildplatetemplate[] = { '{','u','n','i',',','u',',','c','i','d',',','e','n','c','}','[','0','-','9','a','-','f','A','-','F',']','*', '.', 'p','l','a','t','e',  0 };
static unichar_t wildps[] = { '*', '.', '{', 'p','s',',', 'e','p','s',',', 'a','r','t','}', '\0' };
static unichar_t wildpdf[] = { '*', '.', 'p','d','f',  '\0' };
static unichar_t wildsvg[] = { '*', '.', 's','v','g',  '\0' };
static unichar_t wildplate[] = { '*', '.', 'p','l','a','t','e',  '\0' };
static unichar_t wildglif[] = { '*', '.', 'g','l','i','f',  '\0' };
static unichar_t wildfig[] = { '*', '.', '{', 'f','i','g',',','x','f','i','g','}',  '\0' };
static unichar_t wildbdf[] = { '*', '.', 'b', 'd','{', 'f', ',','f','.','g','z',',','f','.','Z',',','f','.','b','z','2','}',  '\0' };
static unichar_t wildpcf[] = { '*', '.', 'p', '{', 'c',',','m','}','{', 'f', ',','f','.','g','z',',','f','.','Z',',','f','.','b','z','2','}',  '\0' };
static unichar_t wildttf[] = { '*', '.', '{', 't', 't','f',',','o','t','f',',','o','t','b',',','t','t','c','}',  '\0' };
static unichar_t wildpk[] = { '*', '{', 'p', 'k', ',', 'g', 'f', '}',  '\0' };		/* pk fonts can have names like cmr10.300pk, not a normal extension */
static unichar_t wildmac[] = { '*', '{', 'b', 'i', 'n', ',', 'h', 'q', 'x', ',', 'd','f','o','n','t', '}',  '\0' };
static unichar_t wildwin[] = { '*', '{', 'f', 'o', 'n', ',', 'f', 'n', 't', '}',  '\0' };
static unichar_t wildpalm[] = { '*', 'p', 'd', 'b',  '\0' };
/* Must match enum baseviews.h:fvformats */
static unichar_t *wildfmt[] = { wildbdf, wildttf, wildpk, wildpcf, wildmac,
wildwin, wildpalm,
wildimg, wildtemplate, wildps, wildepstemplate,
wildpdf, wildpdftemplate,
wildplate, wildplatetemplate,
wildsvg, wildsvgtemplate,
wildglif, wildgliftemplate,
wildfig
};

static int IMPP_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate )
	*(int *) GDrawGetUserData(GGadgetGetWindow(g)) = true;
return( true );
}

static int impp_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	*(int *) GDrawGetUserData(gw) = true;
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/menus/filemenu.html", "#filemenu-type3-import");
return( true );
	}
return( false );
    }
return( true );
}

void _ImportParamsDlg(ImportParams *ip) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[13], boxes[4], *hvarray[13][4], *barray[10];
    GTextInfo label[13];
    char accbuf[20], jlbuf[20];
    int done = false, err = false;
    int k, he_k, cd_k, si_k, sc_k, cl_k, di_k, al_k;

    if ( no_windowing_ui )
	return;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Import Parameters");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,200));
    pos.height = GDrawPointsToPixels(NULL,200);
    gw = GDrawCreateTopWindow(NULL,&pos,impp_e_h,&done,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    k = 0;
    label[k].text = (unichar_t *) _("The following options influence how files are imported.\n"
		                    "Most are specific to one or more formats.");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = 6;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[0][0] = &gcd[k-1];
    hvarray[0][1] = GCD_ColSpan;
    hvarray[0][2] = GCD_ColSpan;
    hvarray[0][3] = NULL;

    cd_k = k;
    label[k].text = (unichar_t *) _("_Correct Direction (PS/EPS)");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+15;
    gcd[k].gd.flags = gg_enabled | gg_visible | (ip->correct_direction?gg_cb_on:0);
    gcd[k++].creator = GCheckBoxCreate;
    hvarray[1][0] = &gcd[k-1];
    hvarray[1][1] = GCD_ColSpan;
    hvarray[1][2] = GCD_ColSpan;
    hvarray[1][3] = NULL;

    he_k = k;
    label[k].text = (unichar_t *) _("Handle Erasers (PS/EPS)");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+15;
    gcd[k].gd.flags = gg_enabled | gg_visible|
	    (ip->erasers?gg_cb_on:0);
    gcd[k].gd.popup_msg = _("Certain programs use pens with white ink as erasers\nThis option attempts to simulate that effect.");
    gcd[k++].creator = GCheckBoxCreate;
    hvarray[2][0] = &gcd[k-1];
    hvarray[2][1] = GCD_ColSpan;
    hvarray[2][2] = GCD_ColSpan;
    hvarray[2][3] = NULL;

    si_k = k;
    label[k].text = (unichar_t *) _("Simplify Stroke (SVG/PS/EPS)");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+15;
    gcd[k].gd.flags = gg_enabled | gg_visible|
	    (ip->simplify?gg_cb_on:0);
    gcd[k].gd.popup_msg = _("Run Simplify after expanding stroked paths\nto reduce the number of points.");
    gcd[k++].creator = GCheckBoxCreate;
    hvarray[3][0] = &gcd[k-1];
    hvarray[3][1] = GCD_ColSpan;
    hvarray[3][2] = GCD_ColSpan;
    hvarray[3][3] = NULL;

    cl_k = k;
    label[k].text = (unichar_t *) _("Use Clip-paths (SVG)");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+15;
    gcd[k].gd.flags = gg_visible|
	    (ip->clip?gg_cb_on:0); // | gg_enabled
    gcd[k++].creator = GCheckBoxCreate;
    hvarray[4][0] = &gcd[k-1];
    hvarray[4][1] = GCD_ColSpan;
    hvarray[4][2] = GCD_ColSpan;
    hvarray[4][3] = NULL;

    sc_k = k;
    label[k].text = (unichar_t *) _("Scale to fit (Misc)");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+15;
    gcd[k].gd.flags = gg_enabled | gg_visible|
	    (ip->scale?gg_cb_on:0);
    gcd[k++].creator = GCheckBoxCreate;
    hvarray[5][0] = &gcd[k-1];
    hvarray[5][1] = GCD_ColSpan;
    hvarray[5][2] = GCD_ColSpan;
    hvarray[5][3] = NULL;

    di_k = k;
    label[k].text = (unichar_t *) _("Infer glyph width (Misc)");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+15;
    gcd[k].gd.flags = gg_enabled | gg_visible|
	    (ip->dimensions?gg_cb_on:0);
    gcd[k++].creator = GCheckBoxCreate;
    hvarray[6][0] = &gcd[k-1];
    hvarray[6][1] = GCD_ColSpan;
    hvarray[6][2] = GCD_ColSpan;
    hvarray[6][3] = NULL;

    label[k].text = (unichar_t *) _("Default Join Limit (PS/EPS/SVG):");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;
    hvarray[7][0] = &gcd[k-1];

    sprintf( jlbuf, "%g", (double) (ip->default_joinlimit) );
    label[k].text = (unichar_t *) jlbuf;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_JoinLimitVal;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.popup_msg = _(
      "The length limit for Miter and Arcs joins in units\n"
      "of 1/2 stroke-width. Set to -1 to use the format-\n"
      "specific limits of 10.0 for PostScript and 4.0 for SVG.");
    gcd[k++].creator = GTextFieldCreate;
    hvarray[7][1] = &gcd[k-1];
    hvarray[7][2] = GCD_Glue;
    hvarray[7][3] = NULL;

    label[k].text = (unichar_t *) _("Accuracy _Target:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;
    hvarray[8][0] = &gcd[k-1];

    sprintf( accbuf, "%g", (double) (ip->accuracy_target) );
    label[k].text = (unichar_t *) accbuf;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_AccTar;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.popup_msg = _(
      "The Expand Stroke algorithm will attempt to be (at\n"
      "least) this accurate, but there may be exceptions.");
    gcd[k++].creator = GTextFieldCreate;
    hvarray[8][1] = &gcd[k-1];
    hvarray[8][2] = GCD_Glue;
    hvarray[8][3] = NULL;

    al_k = k;
    label[k].text = (unichar_t *) _("_Always raise this dialog when importing");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | (ip->show_always?gg_cb_on:0);
    gcd[k++].creator = GCheckBoxCreate;
    hvarray[9][0] = &gcd[k-1];
    hvarray[9][1] = GCD_ColSpan;
    hvarray[9][2] = GCD_ColSpan;
    hvarray[9][3] = NULL;

    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = IMPP_OK;
    gcd[k++].creator = GButtonCreate;
    barray[0] = GCD_Glue;
    barray[1] = &gcd[k-1];
    barray[2] = GCD_Glue;
    barray[3] = NULL;

    boxes[2].gd.flags = gg_enabled | gg_visible;
    boxes[2].gd.u.boxelements = barray;
    boxes[2].creator = GHBoxCreate;
    hvarray[10][0] = GCD_Glue;
    hvarray[10][1] = GCD_ColSpan;
    hvarray[10][2] = GCD_ColSpan;
    hvarray[10][3] = NULL;
    hvarray[11][0] = &boxes[2];
    hvarray[11][1] = GCD_ColSpan;
    hvarray[11][2] = GCD_ColSpan;
    hvarray[11][3] = NULL;
    hvarray[12][0] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled | gg_visible;
    boxes[0].gd.u.boxelements = hvarray[0];
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);

    GDrawSetVisible(gw,true);

    while ( !done )
	GDrawProcessOneEvent(NULL);

    ip->correct_direction = GGadgetIsChecked(gcd[cd_k].ret);
    ip->erasers = GGadgetIsChecked(gcd[he_k].ret);
    ip->simplify = GGadgetIsChecked(gcd[si_k].ret);
    ip->clip = GGadgetIsChecked(gcd[cl_k].ret);
    ip->scale = GGadgetIsChecked(gcd[sc_k].ret);
    ip->dimensions = GGadgetIsChecked(gcd[di_k].ret);
    ip->default_joinlimit = GetReal8(gw,CID_JoinLimitVal,_("Default Join Limit (PS/EPS/SVG):"),&err);
    if ( err ) {
	ip->default_joinlimit = JLIMIT_INHERITED;
	err = false;
    }
    ip->accuracy_target = GetReal8(gw,CID_AccTar,_("Accuracy Target:"),&err);
    if ( err )
	ip->accuracy_target = 0.25;
    ip->show_always = GGadgetIsChecked(gcd[al_k].ret);

    GDrawDestroyWindow(gw);
}

static void ShowImportOptions(ImportParams *ip, int shown,
                              enum shown_params type) {
    if ( !shown && (!(ip->shown_mask & type) || ip->show_always) )
	_ImportParamsDlg(ip);
    ip->shown_mask |= type;
}

/****************************** Import picker *********************************/

static int last_format, flast_format, last_lpos, flast_lpos;
struct gfc_data {
    int done;
    int ret;
    int opts_shown;
    GGadget *gfc;
    GGadget *format;
    GGadget *background;
    CharView *cv;
    BitmapView *bv;
    FontView *fv;
};

static GTextInfo formats[] = {
    { (unichar_t *) N_("Image"), NULL, 0, 0, (void *) fv_image, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("EPS"), NULL, 0, 0, (void *) fv_eps, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("PDF page graphics"), NULL, 0, 0, (void *) fv_pdf, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("SVG"), NULL, 0, 0, (void *) fv_svg, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Glif"), NULL, 0, 0, (void *) fv_glif, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Raph's plate files"), NULL, 0, 0, (void *) fv_plate, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("XFig"), NULL, 0, 0, (void *) fv_fig, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};

static GTextInfo fvformats[] = {
    { (unichar_t *) N_("BDF"), NULL, 0, 0, (void *) fv_bdf, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("TTF"), NULL, 0, 0, (void *) fv_ttf, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) NU_("ΤεΧ Bitmap Fonts"), NULL, 0, 0, (void *) fv_pk, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("PCF (pmf)"), NULL, 0, 0, (void *) fv_pcf, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Mac Bitmap"), NULL, 0, 0, (void *) fv_mac, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Win FON"), NULL, 0, 0, (void *) fv_win, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("palm"), NULL, 0, 0, (void *) fv_palm, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Image"), NULL, 0, 0, (void *) fv_image, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Image Template"), NULL, 0, 0, (void *) fv_imgtemplate, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("EPS"), NULL, 0, 0, (void *) fv_eps, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("EPS Template"), NULL, 0, 0, (void *) fv_epstemplate, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("PDF page graphics"), NULL, 0, 0, (void *) fv_pdf, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("SVG"), NULL, 0, 0, (void *) fv_svg, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("SVG Template"), NULL, 0, 0, (void *) fv_svgtemplate, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Glif"), NULL, 0, 0, (void *) fv_glif, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Glif Template"), NULL, 0, 0, (void *) fv_gliftemplate, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};

static int GFD_ImportOk(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	unichar_t *ret = GGadgetGetTitle(d->gfc);
	char *temp = u2def_copy(ret);
	int lpos = GGadgetGetFirstListSelectedItem(d->format);
	int format = (intptr_t) (GGadgetGetListItemSelected(d->format)->userdata);
	GGadget *tf;

	ImportParams *ip = ImportParamsState();

	GFileChooserGetChildren(d->gfc,NULL,NULL,&tf);
	if ( *_GGadgetGetTitle(tf)=='\0' )
return( true );
	GDrawSetCursor(GGadgetGetWindow(g),ct_watch);
	if ( d->fv!=NULL ) {
	    flast_format = format;
	    flast_lpos = lpos;
	} else {
	    last_format = format;
	    last_lpos = lpos;
	}
	free(ret);
	if ( d->fv!=NULL ) {
	    int toback = GGadgetIsChecked(d->background);
	    if ( toback && strchr(temp,';')!=NULL && format<3 )
		ff_post_error(_("Only One Font"),_("Only one font may be imported into the background"));
	    else if ( format==fv_bdf )
		d->done = FVImportBDF((FontViewBase *) d->fv,temp,false, toback);
	    else if ( format==fv_ttf )
		d->done = FVImportMult((FontViewBase *) d->fv,temp,toback,bf_ttf);
	    else if ( format==fv_pk )		/* pk */
		d->done = FVImportBDF((FontViewBase *) d->fv,temp,true, toback);
	    else if ( format==fv_pcf )		/* pcf */
		d->done = FVImportBDF((FontViewBase *) d->fv,temp,2, toback);
	    else if ( format==fv_mac )
		d->done = FVImportMult((FontViewBase *) d->fv,temp,toback,bf_nfntmacbin);
	    else if ( format==fv_win )
		d->done = FVImportMult((FontViewBase *) d->fv,temp,toback,bf_fon);
	    else if ( format==fv_palm )
		d->done = FVImportMult((FontViewBase *) d->fv,temp,toback,bf_palm);
	    else if ( format==fv_image ) {
		ShowImportOptions(ip, d->opts_shown, sp_scale);
		d->done = FVImportImages((FontViewBase *) d->fv,temp,format,toback,true,ip);
	    } else if ( format==fv_imgtemplate ) {
		ShowImportOptions(ip, d->opts_shown, sp_scale);
		d->done = FVImportImageTemplate((FontViewBase *) d->fv,temp,format,toback,true,ip);
	    } else if ( format==fv_eps ) {
		ShowImportOptions(ip, d->opts_shown, sp_eps);
		d->done = FVImportImages((FontViewBase *) d->fv,temp,format,toback,true,ip);
	    } else if ( format==fv_epstemplate ) {
		ShowImportOptions(ip, d->opts_shown, sp_eps);
		d->done = FVImportImageTemplate((FontViewBase *) d->fv,temp,format,toback,true,ip);
	    } else if ( format==fv_pdf ) {
		ShowImportOptions(ip, d->opts_shown, sp_eps);
		d->done = FVImportImages((FontViewBase *) d->fv,temp,format,toback,true,ip);
	    } else if ( format==fv_pdftemplate ) {
		ShowImportOptions(ip, d->opts_shown, sp_eps);
		d->done = FVImportImageTemplate((FontViewBase *) d->fv,temp,format,toback,true,ip);
	    } else if ( format==fv_svg ) {
		ShowImportOptions(ip, d->opts_shown, sp_svg);
		d->done = FVImportImages((FontViewBase *) d->fv,temp,format,toback,true,ip);
	    } else if ( format==fv_svgtemplate ) {
		ShowImportOptions(ip, d->opts_shown, sp_svg);
		d->done = FVImportImageTemplate((FontViewBase *) d->fv,temp,format,toback,true,ip);
	    } else if ( format==fv_glif )
		d->done = FVImportImages((FontViewBase *) d->fv,temp,format,toback,true,ip);
	    else if ( format==fv_gliftemplate )
		d->done = FVImportImageTemplate((FontViewBase *) d->fv,temp,format,toback,true,ip);
	    else if ( format>=fv_pythonbase )
		d->done = FVImportImages((FontViewBase *) d->fv,temp,format,toback,true,ip);
	} else if ( d->bv!=NULL )
	    d->done = BVImportImage(d->bv,temp);
	else {
	    d->done = true;
	    if ( format==fv_image ) {
		ShowImportOptions(ip, d->opts_shown, sp_scale);
		ImportImage(d->cv,temp,ip);
	    } else if ( format==fv_eps ) {
		ShowImportOptions(ip, d->opts_shown, sp_eps);
		ImportPS(d->cv,temp,ip);
	    } else if ( format==fv_pdf ) {
		ShowImportOptions(ip, d->opts_shown, sp_eps);
		ImportPDF(d->cv,temp,ip);
	    } else if ( format==fv_plate )
		ImportPlate(d->cv,temp,ip);
	    else if ( format==fv_svg ) {
		ShowImportOptions(ip, d->opts_shown, sp_svg);
		ImportSVG(d->cv,temp,ip);
	    } else if ( format==fv_glif )
		ImportGlif(d->cv,temp,ip);
	    else if ( format==fv_fig ) {
		ImportFig(d->cv,temp,ip);
	    }
#ifndef _NO_PYTHON
	    else if ( format>=fv_pythonbase )
		PyFF_SCImport(d->cv->b.sc,format-fv_pythonbase,temp,
			CVLayer((CharViewBase *) d->cv), false);
#endif
	}
	free(temp);
	GDrawSetCursor(GGadgetGetWindow(g),ct_pointer);
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

static int GFD_Format(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int format = (intptr_t) (GGadgetGetListItemSelected(d->format)->userdata);
	if ( format<fv_pythonbase )
	    GFileChooserSetFilterText(d->gfc,wildfmt[format]);
#ifndef _NO_PYTHON
	else {
	    char *text;
	    char *ae = py_ie[format-fv_pythonbase].all_extensions;
	    unichar_t *utext;
	    text = malloc(strlen(ae)+10);
	    if ( strchr(ae,','))
		sprintf( text, "*.{%s}", ae );
	    else
		sprintf( text, "*.%s", ae );
	    utext = utf82u_copy(text);
	    GFileChooserSetFilterText(d->gfc,utext);
	    free(text); free(utext);
	}
#endif
	GFileChooserRefreshList(d->gfc);
	if ( d->fv!=NULL ) {
	    if ( format==fv_bdf || format==fv_ttf || format==fv_pcf ||
		    format==fv_mac || format==fv_win ) {
		GGadgetSetChecked(d->background,false);
		GGadgetSetEnabled(d->background,true);
	    } else if ( format==fv_pk ) {
		GGadgetSetChecked(d->background,true);
		GGadgetSetEnabled(d->background,true);
	    } else if ( format==fv_eps || format==fv_epstemplate ||
			format==fv_pdf || format==fv_pdftemplate ||
			format==fv_svg || format==fv_svgtemplate ||
			format==fv_glif || format==fv_gliftemplate ||
			format>=fv_pythonbase ) {
		GGadgetSetChecked(d->background,false);
		GGadgetSetEnabled(d->background,true);
	    } else {			/* Images */
		GGadgetSetChecked(d->background,true);
		GGadgetSetEnabled(d->background,true);
	    }
	}
    }
return( true );
}


static int GFD_Options(GGadget *g, GEvent *e) {
    if (    e->type==et_controlevent
         && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	_ImportParamsDlg(ImportParamsState());
	d->opts_shown = true;
    }
    return true;
}

static int e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct gfc_data *d = GDrawGetUserData(gw);
	d->done = true;
	d->ret = false;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type==et_map ) {
	GDrawRaise(gw);
    } else if ( event->type == et_mousemove ||
	    (event->type==et_mousedown && event->u.mouse.button==3 )) {
	struct gfc_data *d = GDrawGetUserData(gw);
	GFileChooserPopupCheck(d->gfc,event);
    } else if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
	struct gfc_data *d = GDrawGetUserData(gw);
return( GGadgetDispatchEvent((GGadget *) (d->gfc),event));
    }
return( true );
}

static void _Import(CharView *cv,BitmapView *bv,FontView *fv) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[10], boxes[4], *varray[9], *harray[7], *buttons[10];
    GTextInfo label[9];
    struct gfc_data d;
    int i, format, lpos;
    int bs = GIntGetResource(_NUM_Buttonsize), bsbigger, totwid, scalewid;
    static int done= false;
    GTextInfo *cur_formats;

    if ( !done ) {
	for ( i=0; formats[i].text!=NULL; ++i )
	    formats[i].text = (unichar_t *) _((char *) formats[i].text);
	for ( i=0; fvformats[i].text!=NULL; ++i )
	    fvformats[i].text = (unichar_t *) _((char *) fvformats[i].text);
	last_format = (int) fv_image;
	flast_format = (int) fv_bdf;
	done = true;
    }
#ifndef _NO_PYTHON
    GTextInfo *base = cur_formats = fv==NULL?formats:fvformats;
    if ( py_ie!=NULL ) {
	int cnt, extras;
	for ( cnt=0; base[cnt].text!=NULL; ++cnt );
	for ( i=extras=0; py_ie[i].name!=NULL; ++i )
	    if ( py_ie[i].import!=NULL )
		++extras;
	if ( extras!=0 ) {
	    cur_formats = calloc(extras+cnt+1,sizeof(GTextInfo));
	    for ( cnt=0; base[cnt].text!=NULL; ++cnt ) {
		cur_formats[cnt] = base[cnt];
		cur_formats[cnt].text = (unichar_t *) copy( (char *) base[cnt].text );
	    }
	    for ( i=extras=0; py_ie[i].name!=NULL; ++i ) {
		if ( py_ie[i].import!=NULL ) {
		    cur_formats[cnt+extras].text = (unichar_t *) copy(py_ie[i].name);
		    cur_formats[cnt+extras].text_is_1byte = true;
		    cur_formats[cnt+extras].userdata = (void *) (intptr_t) (fv_pythonbase+i);
		    ++extras;
		}
	    }
	}
    }
#endif
    if ( !hasspiro()) {
	for ( i=0; cur_formats[i].text!=NULL; ++i )
	    if ( ((intptr_t) cur_formats[i].userdata)==fv_plate ||
		    ((intptr_t) cur_formats[i].userdata)==fv_platetemplate )
		cur_formats[i].disabled = true;
    }

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Import");
    pos.x = pos.y = 0;
    totwid = 240;
    if ( fv!=NULL ) totwid += 60;
    scalewid = GGadgetScale(totwid);
    bsbigger = 3*bs+4*14>scalewid; scalewid = bsbigger?3*bs+4*12:scalewid;
    pos.width = GDrawPointsToPixels(NULL,scalewid);
    pos.height = GDrawPointsToPixels(NULL,255);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; gcd[0].gd.pos.width = totwid-24; gcd[0].gd.pos.height = 182;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    if ( fv!=NULL )
	gcd[0].gd.flags |= gg_file_multiple;
    gcd[0].creator = GFileChooserCreate;
    varray[0] = &gcd[0]; varray[1] = NULL;

    gcd[1].gd.pos.x = 12; gcd[1].gd.pos.y = 224-3; gcd[1].gd.pos.width = -1; gcd[1].gd.pos.height = 0;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[1].text = (unichar_t *) _("_Import");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.handle_controlevent = GFD_ImportOk;
    gcd[1].creator = GButtonCreate;
    buttons[0] = GCD_Glue; buttons[1] = &gcd[1]; buttons[2] = GCD_Glue;

    gcd[2].gd.pos.x = (totwid-bs)*100/GIntGetResource(_NUM_ScaleFactor)/2; gcd[2].gd.pos.y = 224; gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
    gcd[2].gd.flags = gg_visible | gg_enabled;
    label[2].text = (unichar_t *) _("_Filter");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.mnemonic = 'F';
    gcd[2].gd.label = &label[2];
    gcd[2].gd.handle_controlevent = GFileChooserFilterEh;
    gcd[2].creator = GButtonCreate;
    buttons[3] = GCD_Glue; buttons[4] = &gcd[2]; buttons[5] = GCD_Glue;

    gcd[3].gd.pos.x = -gcd[1].gd.pos.x; gcd[3].gd.pos.y = 224; gcd[3].gd.pos.width = -1; gcd[3].gd.pos.height = 0;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[3].text = (unichar_t *) _("_Cancel");
    label[3].text_is_1byte = true;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.handle_controlevent = GFD_Cancel;
    gcd[3].creator = GButtonCreate;
    buttons[6] = GCD_Glue; buttons[7] = &gcd[3]; buttons[8] = GCD_Glue;
    buttons[9] = NULL;

    gcd[4].gd.pos.x = 12; gcd[4].gd.pos.y = 200; gcd[4].gd.pos.width = 0; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled;
    label[4].text = (unichar_t *) _("Format:");
    label[4].text_is_1byte = true;
    gcd[4].gd.label = &label[4];
    gcd[4].creator = GLabelCreate;
    harray[0] = &gcd[4];

    gcd[5].gd.pos.x = 55; gcd[5].gd.pos.y = 194; 
    gcd[5].gd.flags = gg_visible | gg_enabled ;
    if ( bv!=NULL ) {
	gcd[5].gd.flags = gg_visible ;			/* No postscript in bitmap mode */
	last_format=0;
    }
    format = fv==NULL?last_format:flast_format;
    lpos = fv==NULL?last_lpos:flast_lpos;
    gcd[5].gd.u.list = cur_formats;
    gcd[5].gd.label = &gcd[5].gd.u.list[lpos];
    gcd[5].gd.handle_controlevent = GFD_Format;
    gcd[5].creator = GListButtonCreate;
    /* This won't iterate over python imports but those were rebuilt above */
    for ( i=0; gcd[5].gd.u.list[i].text!=NULL; ++i )
	gcd[5].gd.u.list[i].selected = false;
    /* Safe as long as python imports cannot be removed */
    gcd[5].gd.u.list[lpos].selected = true;
    harray[1] = &gcd[5];

    gcd[6].gd.flags = gg_visible | gg_enabled;
    label[6].text = (unichar_t *) _("_Options");
    label[6].text_is_1byte = true;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.handle_controlevent = GFD_Options;
    gcd[6].creator = GButtonCreate;
    harray[2] = &gcd[6]; harray[3] = GCD_Glue;

    if ( fv!=NULL ) {
	gcd[7].gd.flags = gg_visible | gg_enabled ;
	if ( format==fv_pk || format==fv_image || format==fv_imgtemplate )
	    gcd[7].gd.flags = gg_visible | gg_enabled | gg_cb_on;
	label[7].text = (unichar_t *) _("As Background");
	label[7].text_is_1byte = true;
	gcd[7].gd.label = &label[7];
	gcd[7].creator = GCheckBoxCreate;
	harray[4] = &gcd[7]; harray[5] = GCD_Glue; harray[6] = NULL;
    } else {
	harray[4] = GCD_Glue; harray[5] = NULL;
    }

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray;
    boxes[2].creator = GHBoxCreate;
    varray[2] = &boxes[2]; varray[3] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = buttons;
    boxes[3].creator = GHBoxCreate;
    varray[4] = GCD_Glue; varray[5] = NULL;
    varray[6] = &boxes[3]; varray[7] = NULL;
    varray[8] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandgluesame);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
    GGadgetSetUserData(gcd[2].ret,gcd[0].ret);

    GFileChooserConnectButtons(gcd[0].ret,gcd[1].ret,gcd[2].ret);
    GFileChooserSetFilterText(gcd[0].ret,wildfmt[format]);
    GFileChooserRefreshList(gcd[0].ret);
    GHVBoxFitWindow(boxes[0].ret);

    memset(&d,'\0',sizeof(d));
    d.cv = cv;
    d.fv = fv;
    d.bv = bv;
    d.gfc = gcd[0].ret;
    d.format = gcd[5].ret;
    if ( fv!=NULL )
	d.background = gcd[7].ret;

    if ( cur_formats!=formats && cur_formats!=fvformats )
	GTextInfoListFree(cur_formats);

    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

void CVImport(CharView *cv) {
    _Import(cv,NULL,NULL);
}

void BVImport(BitmapView *bv) {
    _Import(NULL,bv,NULL);
}

void FVImport(FontView *fv) {
    _Import(NULL,NULL,fv);
}
