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

#include "bitmapcontrol.h"
#include "fontforgeui.h"
#include "gkeysym.h"
#include "gwidget.h"
#include "splinefill.h"
#include "ustring.h"

#include <math.h>

typedef struct createbitmapdlg {
    CreateBitmapData bd;
    GWindow gw;
} CreateBitmapDlg;

static int oldusefreetype=1;
int oldsystem=0 /* X11 */;

static GTextInfo which[] = {
    { (unichar_t *) N_("All Glyphs"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' },
    { (unichar_t *) N_("Selected Glyphs"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' },
    { (unichar_t *) N_("Current Glyph"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' },
    GTEXTINFO_EMPTY
};

#define CID_Which	1001
#define CID_Pixel	1002
#define CID_75		1003
#define CID_100		1004
#define CID_75Lab	1005
#define CID_100Lab	1006
#define CID_X		1007
#define CID_Win		1008
#define CID_Mac		1009
#define CID_FreeType	1010
#define CID_RasterizedStrikes	1011

static int GetSystem(GWindow gw) {
    if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_X)) )
return( CID_X );
    if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Win)) )
return( CID_Win );

return( CID_Mac );
}
    
static int32_t *ParseList(GWindow gw, int cid,int *err, int final) {
    const unichar_t *val = _GGadgetGetTitle(GWidgetGetControl(gw,cid)), *pt;
    unichar_t *end, *end2;
    int i;
    real *sizes;
    int32_t *ret;
    int system = GetSystem(gw);
    int scale;

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
    sizes = malloc((i+1)*sizeof(real));
    ret = malloc((i+1)*sizeof(int32_t));

    for ( i=0, pt = val; *pt!='\0' ; ) {
	sizes[i]=u_strtod(pt,&end);
	if ( *end=='@' )
	    ret[i] = (u_strtol(end+1,&end,10)<<16);
	else
	    ret[i] = 0x10000;
	if ( sizes[i]>0 ) ++i;
	if ( *end!=' ' && *end!=',' && *end!='\0' ) {
	    free(sizes); free(ret);
	    if ( final )
		GGadgetProtest8(_("Pixel Sizes:"));
	    *err = true;
return( NULL );
	}
	while ( *end==' ' || *end==',' ) ++end;
	pt = end;
    }
    sizes[i] = 0; ret[i] = 0;

    if ( cid==CID_75 ) {
	scale = system==CID_X?75:system==CID_Win?96:72;
	for ( i=0; sizes[i]!=0; ++i )
	    ret[i] |= (int) rint(sizes[i]*scale/72);
    } else if ( cid==CID_100 ) {
	scale = system==CID_X?100:system==CID_Win?120:100;
	for ( i=0; sizes[i]!=0; ++i )
	    ret[i] |= (int) rint(sizes[i]*scale/72);
    } else
	for ( i=0; sizes[i]!=0; ++i )
	    ret[i] |= (int) rint(sizes[i]);
    free(sizes);
return( ret );
}

static int CB_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	int err;
	GWindow gw = GGadgetGetWindow(g);
	CreateBitmapData *bd = GDrawGetUserData(gw);
	int32_t *sizes = ParseList(gw, CID_Pixel,&err, true);
	if ( err )
return( true );
	oldusefreetype = GGadgetIsChecked(GWidgetGetControl(gw,CID_FreeType));
	oldsystem = GetSystem(gw)-CID_X;
	bd->rasterize = true;
	if ( bd->isavail<true )
	    bd->which = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,CID_Which));
	if ( bd->isavail==1 )
	    bd->rasterize = GGadgetIsChecked(GWidgetGetControl(gw,CID_RasterizedStrikes));
	BitmapsDoIt(bd,sizes,oldusefreetype);
	free(sizes);
	SavePrefs(true);
    }
return( true );
}

static int CB_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CreateBitmapData *bd = GDrawGetUserData(GGadgetGetWindow(g));
	bd->done = true;
    }
return( true );
}

static unichar_t *GenText(int32_t *sizes,real scale) {
    int i;
    char *cret, *pt;
    unichar_t *uret;

    for ( i=0; sizes[i]!=0; ++i );
    pt = cret = malloc(i*10+1);
    for ( i=0; sizes[i]!=0; ++i ) {
	if ( pt!=cret ) *pt++ = ',';
	sprintf(pt,"%.1f",(double) ((sizes[i]&0xffff)*scale) );
	pt += strlen(pt);
	if ( pt[-1]=='0' && pt[-2]=='.' ) {
	    pt -= 2;
	    *pt = '\0';
	}
	if ( (sizes[i]>>16)!=1 ) {
	    sprintf(pt,"@%d", (int) (sizes[i]>>16) );
	    pt += strlen(pt);
	}
    }
    *pt = '\0';
    uret = uc_copy(cret);
    free(cret);
return( uret );
}

static void _CB_TextChange(CreateBitmapData *bd, GGadget *g) {
    int cid = (int) GGadgetGetCid(g);
    unichar_t *val;
    int err=false;
    int32_t *sizes = ParseList(((CreateBitmapDlg *) bd)->gw,cid,&err,false);
    int ncid;
    int system = GetSystem(((CreateBitmapDlg *) bd)->gw);
    int scale;

    if ( err )
return;
    for ( ncid=CID_Pixel; ncid<=CID_100; ++ncid ) if ( ncid!=cid ) {
	if ( ncid==CID_Pixel )
	    scale = 72;
	else if ( ncid==CID_75 )
	    scale = system==CID_X?75: system==CID_Win?96 : 72;
	else
	    scale = system==CID_X?100: system==CID_Win?120 : 100;
	val = GenText(sizes,72./scale);
	GGadgetSetTitle(GWidgetGetControl(((CreateBitmapDlg *) bd)->gw,ncid),val);
	free(val);
    }
    free(sizes);
return;
}

static int CB_TextChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	CreateBitmapData *bd = GDrawGetUserData(GGadgetGetWindow(g));
	_CB_TextChange(bd,g);
    }
return( true );
}

static void _CB_SystemChange(CreateBitmapData *bd) {
    GWindow gw = ((CreateBitmapDlg *) bd)->gw;
    int system = GetSystem(gw);
    GGadgetSetTitle8(GWidgetGetControl(gw,CID_75Lab),
	    system==CID_X?_("Point sizes on a 75 dpi screen"):
	    system==CID_Win?_("Point sizes on a 96 dpi screen"):
			    _("Point sizes on a 72 dpi screen"));
    GGadgetSetTitle8(GWidgetGetControl(gw,CID_100Lab),
	    system==CID_Win?_("Point sizes on a 120 dpi screen"):
			    _("Point sizes on a 100 dpi screen"));
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_100Lab),system!=CID_Mac);
    GGadgetSetEnabled(GWidgetGetControl(gw,CID_100),system!=CID_Mac);
    _CB_TextChange(bd,GWidgetGetControl(gw,CID_Pixel));
}

static int CB_SystemChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	CreateBitmapData *bd = GDrawGetUserData(GGadgetGetWindow(g));
	_CB_SystemChange(bd);
    }
return( true );
}

static int bd_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	CreateBitmapData *bd = GDrawGetUserData(gw);
	bd->done = true;
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("ui/menus/elementmenu.html", "#elementmenu-bitmaps");
return( true );
	}
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

void BitmapDlg(FontView *fv,SplineChar *sc, int isavail) {
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[18], boxes[8], *varray[32], *harray[6][7];
    GTextInfo label[18];
    CreateBitmapDlg bd;
    int i,j,k,y;
    int32_t *sizes;
    BDFFont *bdf;
    static int done= false;

    if ( !done ) {
	for ( i=0; which[i].text!=NULL; ++i )
	    which[i].text = (unichar_t *) _((char *) which[i].text);
	done = true;
    }

    bd.bd.fv = (FontViewBase *) fv;
    bd.bd.sc = sc;
    bd.bd.layer = fv!=NULL ? fv->b.active_layer : ly_fore;
    bd.bd.sf = fv->b.cidmaster ? fv->b.cidmaster : fv->b.sf;
    bd.bd.isavail = isavail;
    bd.bd.done = false;

    for ( bdf=bd.bd.sf->bitmaps, i=0; bdf!=NULL; bdf=bdf->next, ++i );
/*
    if ( i==0 && isavail )
	i = 2;
*/
    sizes = malloc((i+1)*sizeof(int32_t));
    for ( bdf=bd.bd.sf->bitmaps, i=0; bdf!=NULL; bdf=bdf->next, ++i )
	sizes[i] = bdf->pixelsize | (BDFDepth(bdf)<<16);
/*
    if ( i==0 && isavail ) {
	sizes[i++] = 12.5;
	sizes[i++] = 17;
    }
*/
    sizes[i] = 0;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = isavail==1 ? _("Bitmap Strikes Available") :
				isavail==0 ? _("Regenerate Bitmap Glyphs") :
					    _("Remove Bitmap Glyphs");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,190));
    pos.height = GDrawPointsToPixels(NULL,252);
    bd.gw = GDrawCreateTopWindow(NULL,&pos,bd_e_h,&bd,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    k=0;
    if ( isavail==1 ) {
	label[0].text = (unichar_t *) _("The list of current pixel bitmap sizes");
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5; 
	gcd[0].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[0].creator = GLabelCreate;
	varray[k++] = &gcd[0]; varray[k++] = NULL;

	label[1].text = (unichar_t *) _(" Removing a size will delete it.");
	label[1].text_is_1byte = true;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.pos.x = 5; gcd[1].gd.pos.y = 5+13;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].creator = GLabelCreate;
	varray[k++] = &gcd[1]; varray[k++] = NULL;

	if ( bd.bd.sf->onlybitmaps && bd.bd.sf->bitmaps!=NULL )
	    label[2].text = (unichar_t *) _(" Adding a size will create it by scaling.");
	else
	    label[2].text = (unichar_t *) _(" Adding a size will create it.");
	label[2].text_is_1byte = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = 5+26;
	gcd[2].gd.flags = gg_enabled|gg_visible;
	gcd[2].creator = GLabelCreate;
	varray[k++] = &gcd[2]; varray[k++] = NULL;
	j = 3; y = 5+39+3;
    } else {
	if ( isavail==0 )
	    label[0].text = (unichar_t *) _("Specify bitmap sizes to be regenerated");
	else
	    label[0].text = (unichar_t *) _("Specify bitmap sizes to be removed");
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5; 
	gcd[0].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[0].creator = GLabelCreate;
	varray[k++] = &gcd[0]; varray[k++] = NULL;

	if ( bdfcontrol_lastwhich==bd_current && sc==NULL )
	    bdfcontrol_lastwhich = bd_selected;
	gcd[1].gd.label = &which[bdfcontrol_lastwhich];
	gcd[1].gd.u.list = which;
	gcd[1].gd.pos.x = 5; gcd[1].gd.pos.y = 5+13;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].gd.cid = CID_Which;
	gcd[1].creator = GListButtonCreate;
	which[bd_current].disabled = sc==NULL;
	which[bdfcontrol_lastwhich].selected = true;
	harray[0][0] = &gcd[1]; harray[0][1] = GCD_Glue; harray[0][2] = NULL;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = harray[0];
	boxes[2].creator = GHBoxCreate;
	varray[k++] = &boxes[2]; varray[k++] = NULL;

	j=2; y = 5+13+28;
    }

    label[j].text = (unichar_t *) _("X");
    label[j].text_is_1byte = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 10; gcd[j].gd.pos.y = y;
    gcd[j].gd.flags = gg_enabled|gg_visible;
    gcd[j].gd.cid = CID_X;
    gcd[j].gd.handle_controlevent = CB_SystemChange;
    gcd[j++].creator = GRadioCreate;
    harray[1][0] = &gcd[j-1];

    label[j].text = (unichar_t *) _("Win");
    label[j].text_is_1byte = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 50; gcd[j].gd.pos.y = y;
    gcd[j].gd.flags = gg_enabled|gg_visible;
    gcd[j].gd.cid = CID_Win;
    gcd[j].gd.handle_controlevent = CB_SystemChange;
    gcd[j++].creator = GRadioCreate;
    harray[1][1] = &gcd[j-1];

    label[j].text = (unichar_t *) _("Mac");
    label[j].text_is_1byte = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 90; gcd[j].gd.pos.y = y;
    gcd[j].gd.flags = gg_enabled|gg_visible;
    gcd[j].gd.cid = CID_Mac;
    gcd[j].gd.handle_controlevent = CB_SystemChange;
    gcd[j++].creator = GRadioCreate;
    y += 26;
    gcd[j-3+oldsystem].gd.flags |= gg_cb_on;
    harray[1][2] = &gcd[j-1]; harray[1][3] = GCD_Glue; harray[1][4] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = harray[1];
    boxes[3].creator = GHBoxCreate;
    varray[k++] = &boxes[3]; varray[k++] = NULL;
    varray[k++] = GCD_Glue; varray[k++] = NULL;

    label[j].text = (unichar_t *) _("Point sizes on a 75 dpi screen");
    label[j].text_is_1byte = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = y;
    gcd[j].gd.flags = gg_enabled|gg_visible;
    gcd[j].gd.cid = CID_75Lab;
    gcd[j++].creator = GLabelCreate;
    varray[k++] = &gcd[j-1]; varray[k++] = NULL;
    y += 13;

    label[j].text = GenText(sizes,oldsystem==0?72./75.:oldsystem==1?72/96.:1.);
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 15; gcd[j].gd.pos.y = y;
    gcd[j].gd.pos.width = 170;
    gcd[j].gd.flags = gg_enabled|gg_visible;
    gcd[j].gd.cid = CID_75;
    gcd[j].gd.handle_controlevent = CB_TextChange;
    gcd[j++].creator = GTextFieldCreate;
    y += 26;
    harray[2][0] = GCD_HPad10; harray[2][1] = &gcd[j-1]; harray[2][2] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = harray[2];
    boxes[4].creator = GHBoxCreate;
    varray[k++] = &boxes[4]; varray[k++] = NULL;

    label[j].text = (unichar_t *) _("Point sizes on a 100 dpi screen");
    label[j].text_is_1byte = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = y;
    gcd[j].gd.flags = gg_enabled|gg_visible;
    gcd[j].gd.cid = CID_100Lab;
    gcd[j++].creator = GLabelCreate;
    y += 13;
    varray[k++] = &gcd[j-1]; varray[k++] = NULL;

    label[j].text = GenText(sizes,oldsystem==1?72./120.:72/100.);
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 15; gcd[j].gd.pos.y = y;
    gcd[j].gd.pos.width = 170;
    gcd[j].gd.flags = gg_enabled|gg_visible;
    if ( oldsystem==2 )
	gcd[j].gd.flags = gg_visible;
    gcd[j].gd.cid = CID_100;
    gcd[j].gd.handle_controlevent = CB_TextChange;
    gcd[j++].creator = GTextFieldCreate;
    y += 26;
    harray[3][0] = GCD_HPad10; harray[3][1] = &gcd[j-1]; harray[3][2] = NULL;

    boxes[5].gd.flags = gg_enabled|gg_visible;
    boxes[5].gd.u.boxelements = harray[3];
    boxes[5].creator = GHBoxCreate;
    varray[k++] = &boxes[5]; varray[k++] = NULL;

    label[j].text = (unichar_t *) _("Pixel Sizes:");
    label[j].text_is_1byte = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = y;
    gcd[j].gd.flags = gg_enabled|gg_visible;
    gcd[j++].creator = GLabelCreate;
    y += 13;
    varray[k++] = &gcd[j-1]; varray[k++] = NULL;

    label[j].text = GenText(sizes,1.);
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 15; gcd[j].gd.pos.y = y;
    gcd[j].gd.pos.width = 170;
    gcd[j].gd.flags = gg_enabled|gg_visible|gg_cb_on;
    gcd[j].gd.cid = CID_Pixel;
    gcd[j].gd.handle_controlevent = CB_TextChange;
    gcd[j++].creator = GTextFieldCreate;
    y += 26;
    harray[4][0] = GCD_HPad10; harray[4][1] = &gcd[j-1]; harray[4][2] = NULL;

    boxes[6].gd.flags = gg_enabled|gg_visible;
    boxes[6].gd.u.boxelements = harray[4];
    boxes[6].creator = GHBoxCreate;
    varray[k++] = &boxes[6]; varray[k++] = NULL;

    label[j].text = (unichar_t *) _("Use FreeType");
    label[j].text_is_1byte = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 10; gcd[j].gd.pos.y = y;
    if ( !hasFreeType() || bd.bd.sf->onlybitmaps)
	gcd[j].gd.flags = gg_visible;
    else if ( oldusefreetype )
	gcd[j].gd.flags = gg_enabled|gg_visible|gg_cb_on;
    else
	gcd[j].gd.flags = gg_enabled|gg_visible;
    gcd[j].gd.cid = CID_FreeType;
    gcd[j].gd.handle_controlevent = CB_SystemChange;
    gcd[j++].creator = GCheckBoxCreate;
    y += 26;
    varray[k++] = &gcd[j-1]; varray[k++] = NULL;

    if ( isavail==1 ) {
	label[j].text = (unichar_t *) _("Create Rasterized Strikes (Not empty ones)");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 10; gcd[j].gd.pos.y = y;
	gcd[j].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[j].gd.cid = CID_RasterizedStrikes;
	gcd[j++].creator = GCheckBoxCreate;
	y += 26;
	varray[k++] = &gcd[j-1]; varray[k++] = NULL;
    }
    varray[k++] = GCD_Glue; varray[k++] = NULL;

    gcd[j].gd.pos.x = 20-3; gcd[j].gd.pos.y = 252-32-3;
    gcd[j].gd.pos.width = -1; gcd[j].gd.pos.height = 0;
    gcd[j].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[j].text = (unichar_t *) _("_OK");
    label[j].text_is_1byte = true;
    label[j].text_in_resource = true;
    gcd[j].gd.mnemonic = 'O';
    gcd[j].gd.label = &label[j];
    gcd[j].gd.handle_controlevent = CB_OK;
    gcd[j++].creator = GButtonCreate;
    harray[5][0] = GCD_Glue; harray[5][1] = &gcd[j-1]; harray[5][2] = GCD_Glue;

    gcd[j].gd.pos.x = -20; gcd[j].gd.pos.y = 252-32;
    gcd[j].gd.pos.width = -1; gcd[j].gd.pos.height = 0;
    gcd[j].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[j].text = (unichar_t *) _("_Cancel");
    label[j].text_is_1byte = true;
    label[j].text_in_resource = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.mnemonic = 'C';
    gcd[j].gd.handle_controlevent = CB_Cancel;
    gcd[j++].creator = GButtonCreate;
    harray[5][3] = GCD_Glue; harray[5][4] = &gcd[j-1]; harray[5][5] = GCD_Glue;
    harray[5][6] = NULL;

    boxes[7].gd.flags = gg_enabled|gg_visible;
    boxes[7].gd.u.boxelements = harray[5];
    boxes[7].creator = GHBoxCreate;
    varray[k++] = &boxes[7]; varray[k++] = NULL;
    varray[k] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(bd.gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    if ( isavail<true )
	GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[7].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);
    which[bdfcontrol_lastwhich].selected = false;
    _CB_SystemChange(&bd.bd);

    GDrawSetVisible(bd.gw,true);
    while ( !bd.bd.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(bd.gw);
}
