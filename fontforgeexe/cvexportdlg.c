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

#include "autohint.h"
#include "cvexport.h"
#include "cvundoes.h"
#include "fontforgeui.h"
#include "gfile.h"
#include "gicons.h"
#include "gio.h"
#include "gkeysym.h"
#include "ustring.h"
#include "utype.h"

#include <locale.h>
#include <math.h>
#include <string.h>
#include <time.h>

static char *last = NULL;
static char *last_bits = NULL;

struct sizebits {
    GWindow gw;
    int *pixels, *bits;
    unsigned int done: 1;
    unsigned int good: 1;
};

#define CID_Size	1000
#define CID_Bits	1001

static int EXPP_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate )
	*(int *) GDrawGetUserData(GGadgetGetWindow(g)) = true;
return( true );
}

static int expp_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	*(int *) GDrawGetUserData(gw) = true;
    } else if ( event->type == et_char ) {
	/*
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("XXX");
	    return( true );
	}
	*/
	return( false );
    }
    return( true );
}

void _ExportParamsDlg(ExportParams *ep) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[5], boxes[4], *hvarray[5][4], *barray[4];
    GTextInfo label[5];
    int done = false;
    int k, ut_k, al_k;

    if ( no_windowing_ui )
	return;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Export Options");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,200));
    pos.height = GDrawPointsToPixels(NULL,200);
    gw = GDrawCreateTopWindow(NULL,&pos,expp_e_h,&done,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));
    memset(&hvarray,0,sizeof(hvarray));

    k = 0;
    label[k].text = (unichar_t *) _("The following options influence how glyphs are exported.\n"
                                    "Most are specific to one or more formats.")
;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = 6;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[0][0] = &gcd[k-1];
    hvarray[0][1] = GCD_ColSpan;
    hvarray[0][2] = GCD_ColSpan;
    hvarray[0][3] = NULL;

    ut_k = k;
    label[k].text = (unichar_t *) _("_Use Transform (SVG)");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | (ep->use_transform?gg_cb_on:0);
    gcd[k].gd.popup_msg = _("FontForge previously exported glyphs using a SVG\n"
                            "transform element to flip the Y-axis rather\n"
                            "than changing the individual values. This option\n"
			    "reverts to that convention.");
    gcd[k++].creator = GCheckBoxCreate;
    hvarray[1][0] = &gcd[k-1];
    hvarray[1][1] = GCD_ColSpan;
    hvarray[1][2] = GCD_ColSpan;
    hvarray[1][3] = NULL;

    al_k = k;
    label[k].text = (unichar_t *) _("_Always raise this dialog when exporting");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+15;
    gcd[k].gd.flags = gg_enabled | gg_visible | (ep->show_always?gg_cb_on:0);
    gcd[k++].creator = GCheckBoxCreate;
    hvarray[2][0] = &gcd[k-1];
    hvarray[2][1] = GCD_ColSpan;
    hvarray[2][2] = GCD_ColSpan;
    hvarray[2][3] = NULL;

    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = EXPP_OK;
    gcd[k++].creator = GButtonCreate;
    barray[0] = GCD_Glue;
    barray[1] = &gcd[k-1];
    barray[2] = GCD_Glue;
    barray[3] = NULL;

    boxes[2].gd.flags = gg_enabled | gg_visible;
    boxes[2].gd.u.boxelements = barray;
    boxes[2].creator = GHBoxCreate;
    hvarray[3][0] = &boxes[2];
    hvarray[3][1] = GCD_ColSpan;
    hvarray[3][2] = GCD_ColSpan;
    hvarray[3][3] = NULL;
    hvarray[4][0] = NULL;

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

    ep->use_transform = GGadgetIsChecked(gcd[ut_k].ret);
    ep->show_always = GGadgetIsChecked(gcd[al_k].ret);
    GDrawDestroyWindow(gw);
}

static void ShowExportOptions(ExportParams *ep, int shown,
                              enum shown_params type) {
    if ( !shown && (!(ep->shown_mask & type) || ep->show_always) )
	_ExportParamsDlg(ep);
    ep->shown_mask |= type;
}

static int SB_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct sizebits *d = GDrawGetUserData(GGadgetGetWindow(g));
	int err=0;
	*d->pixels = GetInt8(d->gw,CID_Size,_("Pixel size:"),&err);
	*d->bits   = GetInt8(d->gw,CID_Bits,_("Bits/Pixel:"),&err);
	if ( err )
return( true );
	if ( *d->bits!=1 && *d->bits!=2 && *d->bits!=4 && *d->bits!=8 ) {
	    ff_post_error(_("The only valid values for bits/pixel are 1, 2, 4 or 8"),_("The only valid values for bits/pixel are 1, 2, 4 or 8"));
return( true );
	}
	free( last ); free( last_bits );
	last = GGadgetGetTitle8(GWidgetGetControl(d->gw,CID_Size));
	last_bits = GGadgetGetTitle8(GWidgetGetControl(d->gw,CID_Bits));
	d->done = true;
	d->good = true;
    }
return( true );
}

static int SB_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct sizebits *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = true;
    }
return( true );
}

static int sb_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct sizebits *d = GDrawGetUserData(gw);
	d->done = true;
    }
return( event->type!=et_char );
}

static int AskSizeBits(int *pixelsize,int *bitsperpixel) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8], boxes[5], *hvarray[7][4], *barray[10];
    GTextInfo label[8];
    struct sizebits sb;

    memset(&sb,'\0',sizeof(sb));
    sb.pixels = pixelsize; sb.bits = bitsperpixel;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Pixel size?");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,140));
    pos.height = GDrawPointsToPixels(NULL,100);
    sb.gw = gw = GDrawCreateTopWindow(NULL,&pos,sb_e_h,&sb,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    label[0].text = (unichar_t *) _("Pixel size:");
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 8; gcd[0].gd.pos.y = 8+6; 
    gcd[0].gd.flags = gg_enabled|gg_visible;
    gcd[0].creator = GLabelCreate;
    hvarray[0][0] = &gcd[0];

    gcd[1].gd.pos.x = 70; gcd[1].gd.pos.y = 8;  gcd[1].gd.pos.width = 65;
    label[1].text = (unichar_t *) (last==NULL ? "100" : last);
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.flags = gg_enabled|gg_visible;
    gcd[1].gd.cid = CID_Size;
    gcd[1].creator = GNumericFieldCreate;
    hvarray[0][1] = &gcd[1]; hvarray[0][2] = GCD_Glue; hvarray[0][3] = NULL;

    label[2].text = (unichar_t *) _("Bits/Pixel:");
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 8; gcd[2].gd.pos.y = 38+6; 
    gcd[2].gd.flags = gg_enabled|gg_visible;
    gcd[2].creator = GLabelCreate;
    hvarray[1][0] = &gcd[2];

    gcd[3].gd.pos.x = 70; gcd[3].gd.pos.y = 38;  gcd[3].gd.pos.width = 65;
    label[3].text = (unichar_t *) (last_bits==NULL? "1" : last_bits);
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].gd.cid = CID_Bits;
    gcd[3].creator = GNumericFieldCreate;
    hvarray[1][1] = &gcd[3]; hvarray[1][2] = GCD_Glue; hvarray[1][3] = NULL;
    hvarray[2][0] = hvarray[2][1] = hvarray[2][2] = GCD_Glue; hvarray[2][3] = NULL;

    gcd[4].gd.pos.x = 10-3; gcd[4].gd.pos.y = 38+30-3;
    gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[4].text = (unichar_t *) _("_OK");
    label[4].text_is_1byte = true;
    label[4].text_in_resource = true;
    gcd[4].gd.mnemonic = 'O';
    gcd[4].gd.label = &label[4];
    gcd[4].gd.handle_controlevent = SB_OK;
    gcd[4].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[4]; barray[2] = barray[3] = barray[4] = barray[5] = GCD_Glue;

    gcd[5].gd.pos.x = -10; gcd[5].gd.pos.y = 38+30;
    gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[5].text = (unichar_t *) _("_Cancel");
    label[5].text_is_1byte = true;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.mnemonic = 'C';
    gcd[5].gd.handle_controlevent = SB_Cancel;
    gcd[5].creator = GButtonCreate;
    barray[5] = &gcd[5]; barray[6] = GCD_Glue; barray[7] = NULL;

    boxes[2].gd.flags = gg_enabled | gg_visible;
    boxes[2].gd.u.boxelements = barray;
    boxes[2].creator = GHBoxCreate;
    hvarray[3][0] = &boxes[2]; hvarray[3][1] = GCD_ColSpan; hvarray[3][2] = GCD_Glue; hvarray[3][3] = NULL;
    hvarray[4][0] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled | gg_visible;
    boxes[0].gd.u.boxelements = hvarray[0];
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);
    GWidgetIndicateFocusGadget(GWidgetGetControl(gw,CID_Size));
    GTextFieldSelect(GWidgetGetControl(gw,CID_Size),0,-1);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !sb.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
return( sb.good );
}

static int ExportXBM(char *filename,SplineChar *sc, int layer, int format) {
    char *ans;
    int pixelsize, bitsperpixel;

    if ( format==0 ) {
	ans = gwwv_ask_string(_("Pixel size?"),"100",_("Pixel size?"));
	if ( ans==NULL )
return( 0 );
	if ( (pixelsize=strtol(ans,NULL,10))<=0 )
return( 0 );
	free(last);
	last = ans;
	bitsperpixel=1;
    } else {
	if ( !AskSizeBits(&pixelsize,&bitsperpixel) )
return( 0 );
    }
    if ( autohint_before_generate && sc->changedsincelasthinted && !sc->manualhints )
	SplineCharAutoHint(sc,layer,NULL);
return( ExportImage(filename,sc, layer, format, pixelsize, bitsperpixel));
}
struct gfc_data {
    int done;
    int ret;
    int opts_shown;
    GGadget *gfc;
    GGadget *format;
    SplineChar *sc;
    BDFChar *bc;
    int layer;
};

static GTextInfo formats[] = {
    { (unichar_t *) N_("EPS"), NULL, 0, 0, (void *) 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("XFig"), NULL, 0, 0, (void *) 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("SVG"), NULL, 0, 0, (void *) 2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Glif"), NULL, 0, 0, (void *) 3, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("PDF"), NULL, 0, 0, (void *) 4, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Raph's plate"), NULL, 0, 0, (void *) 5, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
#define BITMAP_FORMAT_START	6
    /* 0=*.xbm, 1=*.bmp, 2=*.png, 3=*.xpm, 4=*.c(fontforge-internal) */
    { (unichar_t *) N_("X Bitmap"), NULL, 0, 0, (void *) BITMAP_FORMAT_START, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("BMP"), NULL, 0, 0, (void *) (BITMAP_FORMAT_START+1), 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
#ifndef _NO_LIBPNG
    { (unichar_t *) N_("png"), NULL, 0, 0, (void *) (BITMAP_FORMAT_START+2), 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
#endif
    { (unichar_t *) N_("X Pixmap"), NULL, 0, 0, (void *) (BITMAP_FORMAT_START+3), 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("C FontForge"), NULL, 0, 0, (void *) (BITMAP_FORMAT_START+4), 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};
static int last_format = 0, blast_format = BITMAP_FORMAT_START;

static void DoExport(struct gfc_data *d,unichar_t *path) {
    char *temp;
    int format, good = 0;

    temp = cu_copy(path);
    format = (intpt) (GGadgetGetListItemSelected(d->format)->userdata);
    if ( d->bc!=NULL )
        blast_format = format;
    else
        last_format = format;
    if ( d->bc!=NULL )
	good = BCExportXBM(temp,d->bc,format-BITMAP_FORMAT_START);
    else if ( format==0 )
	good = ExportEPS(temp,d->sc,d->layer);
    else if ( format==1 )
	good = ExportFig(temp,d->sc,d->layer);
    else if ( format==2 ) {
	ExportParams *ep = ExportParamsState();
	ShowExportOptions(ep, d->opts_shown, sp_svg);
	good = ExportSVG(temp,d->sc,d->layer,ep);
    } else if ( format==3 )
	good = ExportGlif(temp,d->sc,d->layer,3);
    else if ( format==4 )
	good = ExportPDF(temp,d->sc,d->layer);
    else if ( format==5 )
	good = ExportPlate(temp,d->sc,d->layer);
    else if ( format<fv_pythonbase )
	good = ExportXBM(temp,d->sc,d->layer,format-BITMAP_FORMAT_START);
#ifndef _NO_PYTHON
    else if ( format>=fv_pythonbase )
	PyFF_SCExport(d->sc,format-fv_pythonbase,temp,d->layer);
#endif
    if ( !good )
	ff_post_error(_("Save Failed"),_("Save Failed"));
    free(temp);
    d->done = good;
    d->ret = good;
}

static void GFD_doesnt(GIOControl *gio) {
    /* The filename the user chose doesn't exist, so everything is happy */
    struct gfc_data *d = gio->userdata;
    DoExport(d,gio->path);
    GFileChooserReplaceIO(d->gfc,NULL);
}

static void GFD_exists(GIOControl *gio) {
    /* The filename the user chose exists, ask user if s/he wants to overwrite */
    struct gfc_data *d = gio->userdata;
    char *rcb[3], *temp;

    rcb[2]=NULL;
    rcb[0] =  _("_Replace");
    rcb[1] =  _("_Cancel");

    if ( gwwv_ask(_("File Exists"),(const char **) rcb,0,1,_("File, %s, exists. Replace it?"),
	    temp = u2utf8_copy(u_GFileNameTail(gio->path)))==0 ) {
	DoExport(d,gio->path);
    }
    free(temp);
    GFileChooserReplaceIO(d->gfc,NULL);
}

static int GFD_SaveOk(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *tf;
	GFileChooserGetChildren(d->gfc,NULL,NULL,&tf);
	if ( *_GGadgetGetTitle(tf)!='\0' ) {
	    unichar_t *ret = GGadgetGetTitle(d->gfc);

	    GIOfileExists(GFileChooserReplaceIO(d->gfc,
		    GIOCreate(ret,d,GFD_exists,GFD_doesnt)));
	    free(ret);
	}
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
	unichar_t *pt, *file, *f2;
	int format = (intpt) (GGadgetGetListItemSelected(d->format)->userdata);
	file = GGadgetGetTitle(d->gfc);
	f2 = malloc(sizeof(unichar_t) * (u_strlen(file)+6));
	u_strcpy(f2,file);
	free(file);
	pt = u_strrchr(f2,'.');
	if ( pt==NULL )
	    pt = f2+u_strlen(f2);
#ifndef _NO_PYTHON
	if ( format>=fv_pythonbase )
	    uc_strcpy(pt+1,py_ie[format-fv_pythonbase].extension);
#endif
	else
	    uc_strcpy(pt,format==0?".eps":
			 format==1?".fig":
			 format==2?".svg":
			 format==3?".glif":
			 format==4?".pdf":
			 format==5?".plate":
			 format==6?".xbm":
			 format==7?".bmp":
			//format==8?".png":
			 format==9?".xpm":
			 format==10?".c":
				    ".png");
	GGadgetSetTitle(d->gfc,f2);
	free(f2);
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
    GFileChooserReplaceIO(d->gfc,NULL);
}

static int GFD_Options(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
        struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	_ExportParamsDlg(ExportParamsState());
	d->opts_shown = true;
	return true;
    }
    return false;
}

static int GFD_NewDir(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
        struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
        char *newdir;
        unichar_t *utemp;

        newdir = gwwv_ask_string(_("Create directory"),NULL,_("Directory name?"));
        if ( newdir==NULL )
            return( true );
        if ( !GFileIsAbsolute(newdir)) {
            unichar_t *tmp_dir = GFileChooserGetDir(d->gfc);
            char *basedir = u2utf8_copy(tmp_dir);
            char *temp = GFileAppendFile(basedir,newdir,false);
            free(newdir); free(basedir); free(tmp_dir);
            newdir = temp;
        }
        utemp = utf82u_copy(newdir); free(newdir);
        GIOmkDir(GFileChooserReplaceIO(d->gfc,
              GIOCreate(utemp,d,GFD_dircreated,GFD_dircreatefailed)));
        free(utemp);
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

static int CanBeAPlateFile(SplineChar *sc) {
    int open_cnt;
    SplineSet *ss;

    if ( sc->parent->multilayer )
return( false );
    /* Plate files can't handle refs */
    if ( sc->layers[ly_fore].refs!=NULL )
return( false );

    /* Plate files can only handle 1 open contour */
    open_cnt = 0;
    for ( ss= sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
	if ( ss->first->prev==NULL ) {
	    if ( ss->first->next!=NULL )
		++open_cnt;
	}
    }
return( open_cnt<=1 );
}

static int _Export(SplineChar *sc,BDFChar *bc,int layer) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[9], boxes[4], *hvarray[8], *harray[7], *barray[10];
    GTextInfo label[8];
    struct gfc_data d;
    GGadget *pulldown, *files, *tf;
    char buffer[100]; unichar_t ubuf[100];
    char *ext;
    int _format, _lpos, i;
    int bs = GIntGetResource(_NUM_Buttonsize), bsbigger, totwid, scalewid;
    static int done = false;
    GTextInfo *cur_formats;

    if ( !done ) {
	for ( i=0; formats[i].text!=NULL; ++i )
	    formats[i].text= (unichar_t *) _((char *) formats[i].text);
	done = true;
    }
    if ( bc==NULL ) {
	formats[5].disabled = !CanBeAPlateFile(sc);
        cur_formats = formats;
    } else
        cur_formats = formats + BITMAP_FORMAT_START;
#ifndef _NO_PYTHON
    if ( bc==NULL && py_ie!=NULL ) {
	int cnt, extras;
	for ( cnt=0; formats[cnt].text!=NULL; ++cnt );
	for ( i=extras=0; py_ie[i].name!=NULL; ++i )
	    if ( py_ie[i].export!=NULL )
		++extras;
	if ( extras!=0 ) {
	    cur_formats = calloc(extras+cnt+1,sizeof(GTextInfo));
	    for ( cnt=0; formats[cnt].text!=NULL; ++cnt ) {
		cur_formats[cnt] = formats[cnt];
		cur_formats[cnt].text = (unichar_t *) copy( (char *) formats[cnt].text );
	    }
	    for ( i=extras=0; py_ie[i].name!=NULL; ++i ) {
		if ( py_ie[i].export!=NULL ) {
		    cur_formats[cnt+extras].text = (unichar_t *) copy(py_ie[i].name);
		    cur_formats[cnt+extras].text_is_1byte = true;
		    cur_formats[cnt+extras].userdata = (void *) (intpt) (fv_pythonbase+i);
		    ++extras;
		}
	    }
	}
    }
#endif

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Export");
    pos.x = pos.y = 0;
    totwid = 240;
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
    gcd[0].creator = GFileChooserCreate;
    hvarray[0] = &gcd[0]; hvarray[1] = NULL;

    gcd[1].gd.pos.x = 12; gcd[1].gd.pos.y = 224-3; gcd[1].gd.pos.width = -1; gcd[1].gd.pos.height = 0;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[1].text = (unichar_t *) _("_Save");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.handle_controlevent = GFD_SaveOk;
    gcd[1].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[1]; barray[2] = GCD_Glue;

    gcd[2].gd.pos.x = (totwid-bs)*100/GIntGetResource(_NUM_ScaleFactor)/2; gcd[2].gd.pos.y = 224; gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
    gcd[2].gd.flags = gg_visible | gg_enabled;
    label[2].text = (unichar_t *) _("_Filter");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.handle_controlevent = GFileChooserFilterEh;
    gcd[2].creator = GButtonCreate;
    barray[3] = GCD_Glue; barray[4] = &gcd[2]; barray[5] = GCD_Glue;

    gcd[3].gd.pos.x = -gcd[1].gd.pos.x; gcd[3].gd.pos.y = 224; gcd[3].gd.pos.width = -1; gcd[3].gd.pos.height = 0;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[3].text = (unichar_t *) _("_Cancel");
    label[3].text_is_1byte = true;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.handle_controlevent = GFD_Cancel;
    gcd[3].creator = GButtonCreate;
    barray[6] = GCD_Glue; barray[7] = &gcd[3]; barray[8] = GCD_Glue;
    barray[9] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = barray;
    boxes[2].creator = GHBoxCreate;
    hvarray[4] = &boxes[2]; hvarray[5] = NULL;

    gcd[4].gd.flags = gg_visible | gg_enabled;
    label[4].text = (unichar_t *) S_("Directory|_New");
    label[4].text_is_1byte = true;
    label[4].text_in_resource = true;
    label[4].image = &_GIcon_dir;
    label[4].image_precedes = false;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.handle_controlevent = GFD_NewDir;
    gcd[4].creator = GButtonCreate;

    gcd[5].gd.flags = gg_visible | gg_enabled;
    label[5].text = (unichar_t *) _("_Options");
    label[5].text_is_1byte = true;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.handle_controlevent = GFD_Options;
    gcd[5].creator = GButtonCreate;

    gcd[6].gd.flags = gg_visible | gg_enabled;
    label[6].text = (unichar_t *) _("Format:");
    label[6].text_is_1byte = true;
    gcd[6].gd.label = &label[6];
    gcd[6].creator = GLabelCreate;
    harray[0] = &gcd[6];

    if ( bc!=NULL ) {
	_format = blast_format;
	_lpos = _format - BITMAP_FORMAT_START;
    } else {
	_format = last_format;
	_lpos = _format;
    }
    gcd[7].gd.flags = gg_visible | gg_enabled ;
    gcd[7].gd.u.list = cur_formats;
    if ( bc!=NULL ) {
	cur_formats[0].disabled = bc->byte_data;
	if ( _lpos==0 ) _lpos=1;
    }
    gcd[7].gd.label = &cur_formats[_lpos];
    gcd[7].gd.handle_controlevent = GFD_Format;
    gcd[7].creator = GListButtonCreate;
    for ( i=0; cur_formats[i].text!=NULL; ++i )
	cur_formats[i].selected =false;
    cur_formats[_lpos].selected = true;
    harray[1] = &gcd[7];
    harray[2] = GCD_Glue;
    harray[3] = &gcd[4];
    harray[4] = GCD_Glue;
    harray[5] = &gcd[5];
    harray[6] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = harray;
    boxes[3].creator = GHBoxCreate;
    hvarray[2] = &boxes[3]; hvarray[3] = NULL;
    hvarray[6] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = hvarray;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GGadgetSetUserData(gcd[2].ret,gcd[0].ret);

    GHVBoxSetExpandableRow(boxes[0].ret,0);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);

    GFileChooserConnectButtons(gcd[0].ret,gcd[1].ret,gcd[2].ret);
#ifndef _NO_PYTHON
    if ( _format>=fv_pythonbase )
	ext = py_ie[_format-fv_pythonbase].extension;
    else
#endif
	ext = _format==0?"eps":_format==1?"fig":_format==2?"svg":
		_format==3?"glif":
		_format==4?"pdf":_format==5?"plate":
		_format==6?"xbm":_format==7?"bmp":"png";
#if defined( __CygWin ) || defined(__Mac)
    /* Windows file systems are not case conscious */
    { char *pt, *bpt, *end;
    bpt = buffer; end = buffer+40;
    for ( pt=sc->name; *pt!='\0' && bpt<end; ) {
	if ( isupper( *pt ))
	    *bpt++ = '$';
	*bpt++ = *pt++;
    }
    sprintf( bpt, "_%.40s.%s", sc->parent->fontname, ext);
    }
#else
    sprintf( buffer, "%.40s_%.40s.%s", sc->name, sc->parent->fontname, ext);
#endif
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(gcd[0].ret,ubuf);
    GFileChooserGetChildren(gcd[0].ret,&pulldown,&files,&tf);
    GWidgetIndicateFocusGadget(tf);

    if ( cur_formats!=formats && cur_formats!=formats+BITMAP_FORMAT_START )
	GTextInfoListFree(cur_formats);

    memset(&d,'\0',sizeof(d));
    d.sc = sc;
    d.bc = bc;
    d.layer = layer;
    d.gfc = gcd[0].ret;
    d.format = gcd[7].ret;

    GHVBoxFitWindow(boxes[0].ret);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
return(d.ret);
}

int CVExport(CharView *cv) {
return( _Export(cv->b.sc,NULL,CVLayer((CharViewBase *)cv)));
}

int BVExport(BitmapView *bv) {
return( _Export(bv->bc->sc,bv->bc,ly_fore));
}
