/* -*- coding: utf-8 -*- */
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
#include "fontforgeui.h"
#include <utype.h>
#include <ustring.h>
#include "nonlineartrans.h"

struct nldlg {
    GWindow gw;
    int done, ok;
};

static int nld_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct nldlg *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype==et_buttonactivate ) {
	struct nldlg *d = GDrawGetUserData(gw);
	d->done = true;
	d->ok = GGadgetGetCid(event->u.control.g);
    }
return( true );
}

void NonLinearDlg(FontView *fv,CharView *cv) {
    static unichar_t *lastx, *lasty;
    struct nldlg d;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8], boxes[4], *hvarray[3][3], *barray[9], *varray[3][2];
    GTextInfo label[8];
    struct context c;
    char *expstr;

    memset(&d,'\0',sizeof(d));
    memset(&c,'\0',sizeof(c));

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Non Linear Transform");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,200));
    pos.height = GDrawPointsToPixels(NULL,97);
    d.gw = GDrawCreateTopWindow(NULL,&pos,nld_e_h,&d,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(boxes,0,sizeof(boxes));
    memset(label,0,sizeof(label));

/* GT: an expression describing the transformation applied to the X coordinate */
    label[0].text = (unichar_t *) _("X Expr:");
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 10; gcd[0].gd.pos.y = 8;
    gcd[0].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[0].gd.popup_msg = (unichar_t *) _("These expressions may contain the operators +,-,*,/,%,^ (which means raise to the power of here), and ?: It may also contain a few standard functions. Basic terms are real numbers, x and y.\nExamples:\n x^3+2.5*x^2+5\n (x-300)*(y-200)/100\n y+sin(100*x)");
    gcd[0].creator = GLabelCreate;
    hvarray[0][0] = &gcd[0];

    if ( lastx!=NULL )
	label[1].text = lastx;
    else {
	label[1].text = (unichar_t *) "x";
	label[1].text_is_1byte = true;
    }
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 55; gcd[1].gd.pos.y = 5; gcd[1].gd.pos.width = 135;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[1].gd.popup_msg = (unichar_t *) _("These expressions may contain the operators +,-,*,/,%,^ (which means raise to the power of here), and ?: It may also contain a few standard functions. Basic terms are real numbers, x and y.\nExamples:\n x^3+2.5*x^2+5\n (x-300)*(y-200)/100\n y+sin(100*x)");
    gcd[1].creator = GTextFieldCreate;
    hvarray[0][1] = &gcd[1]; hvarray[0][2] = NULL;

/* GT: an expression describing the transformation applied to the Y coordinate */
    label[2].text = (unichar_t *) _("Y Expr:");
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 10; gcd[2].gd.pos.y = gcd[0].gd.pos.y+26;
    gcd[2].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[2].gd.popup_msg = (unichar_t *) _("These expressions may contain the operators +,-,*,/,%,^ (which means raise to the power of here), and ?: It may also contain a few standard functions. Basic terms are real numbers, x and y.\nExamples:\n x^3+2.5*x^2+5\n (x-300)*(y-200)/100\n y+sin(100*x)");
    gcd[2].creator = GLabelCreate;
    hvarray[1][0] = &gcd[2];

    if ( lasty!=NULL )
	label[3].text = lasty;
    else {
	label[3].text = (unichar_t *) "y";
	label[3].text_is_1byte = true;
    }
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = gcd[1].gd.pos.x; gcd[3].gd.pos.y = gcd[1].gd.pos.y+26;
    gcd[3].gd.pos.width = gcd[1].gd.pos.width;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[3].gd.popup_msg = (unichar_t *) _("These expressions may contain the operators +,-,*,/,%,^ (which means raise to the power of here), and ?: It may also contain a few standard functions. Basic terms are real numbers, x and y.\nExamples:\n x^3+2.5*x^2+5\n (x-300)*(y-200)/100\n y+sin(100*x)");
    gcd[3].creator = GTextFieldCreate;
    hvarray[1][1] = &gcd[3]; hvarray[1][2] = NULL; hvarray[2][0] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = hvarray[0];
    boxes[2].creator = GHVBoxCreate;
    varray[0][0] = &boxes[2]; varray[0][1] = NULL;

    gcd[4].gd.pos.x = 30-3; gcd[4].gd.pos.y = gcd[3].gd.pos.y+30;
    gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[4].text = (unichar_t *) _("_OK");
    label[4].text_is_1byte = true;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.cid = true;
    gcd[4].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[4]; barray[2] = GCD_Glue;

    gcd[5].gd.pos.x = -30; gcd[5].gd.pos.y = gcd[4].gd.pos.y+3;
    gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[5].text = (unichar_t *) _("_Cancel");
    label[5].text_is_1byte = true;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.cid = false;
    gcd[5].creator = GButtonCreate;
    barray[3] = GCD_Glue; barray[4] = &gcd[5]; barray[5] = GCD_Glue; barray[6] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = barray;
    boxes[3].creator = GHBoxCreate;
    varray[1][0] = &boxes[3]; varray[1][1] = NULL;
    varray[2][0] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray[0];
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(d.gw,boxes);
    GHVBoxSetExpandableCol(boxes[2].ret,1);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);
    GDrawSetVisible(d.gw,true);
    while ( !d.done ) {
	GDrawProcessOneEvent(NULL);
	if ( d.done && d.ok ) {
	    expstr = cu_copy(_GGadgetGetTitle(gcd[1].ret));
	    c.had_error = false;
	    if ( (c.x_expr = nlt_parseexpr(&c,expstr))==NULL )
		d.done = d.ok = false;
	    else {
		free(expstr);
		c.had_error = false;
		expstr = cu_copy(_GGadgetGetTitle(gcd[3].ret));
		if ( (c.y_expr = nlt_parseexpr(&c,expstr))==NULL ) {
		    d.done = d.ok = false;
		    nlt_exprfree(c.x_expr);
		} else {
		    free(expstr);
		    free(lasty); free(lastx);
		    lastx = GGadgetGetTitle(gcd[1].ret);
		    lasty = GGadgetGetTitle(gcd[3].ret);
		}
	    }
	}
    }
    if ( d.ok ) {
	if ( fv!=NULL )
	    _SFNLTrans((FontViewBase *) fv,&c);
	else
	    CVNLTrans((CharViewBase *) cv,&c);
	nlt_exprfree(c.x_expr);
	nlt_exprfree(c.y_expr);
    }
    GDrawDestroyWindow(d.gw);
}

static GTextInfo originx[] = {
    { (unichar_t *) N_("Glyph Origin"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Center of Selection"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
/* GT: The (x,y) position on the window where the user last pressed a mouse button */
    { (unichar_t *) N_("Last Press"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Value"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};
static GTextInfo originy[] = {
    { (unichar_t *) N_("Glyph Origin"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Center of Selection"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
/* GT: The (x,y) position on the window where the user last pressed a mouse button */
    { (unichar_t *) N_("Last Press"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) N_("Value"), NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};
#define CID_XType	1001
#define CID_YType	1002
#define CID_XValue	1003
#define CID_YValue	1004
#define CID_ZValue	1005
#define CID_DValue	1006
#define CID_Tilt	1007
#define CID_GazeDirection 1008
#define CID_Vanish	1009

static real GetQuietReal(GWindow gw,int cid,int *err) {
    const unichar_t *txt; unichar_t *end;
    real val;

    txt = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    val = u_strtod(txt,&end);
    if ( *end!='\0' )
	*err = true;
return( val );
}

static void PoV_DoVanish(struct nldlg *d) {
    double x,y,dv,tilt,dir;
    double t;
    int err = false;
    double vp;
    char buf[80];
    unichar_t ubuf[80];
extern char *coord_sep;

    x = GetQuietReal(d->gw,CID_XValue,&err);
    y = GetQuietReal(d->gw,CID_YValue,&err);
    /*z = GetQuietReal(d->gw,CID_ZValue,&err);*/
    dv = GetQuietReal(d->gw,CID_DValue,&err);
    tilt = GetQuietReal(d->gw,CID_Tilt,&err)*3.1415926535897932/180;
    dir = GetQuietReal(d->gw,CID_GazeDirection,&err)*3.1415926535897932/180;
    if ( err )
return;
    if ( GGadgetGetFirstListSelectedItem( GWidgetGetControl(d->gw,CID_XType))!=3 )
	x = 0;
    if ( GGadgetGetFirstListSelectedItem( GWidgetGetControl(d->gw,CID_YType))!=3 )
	y = 0;
    t = tan(tilt);
    if ( t<.000001 && t>-.000001 )
	sprintf(buf,"inf%sinf", coord_sep);
    else {
	vp = dv/t;
	x -= sin(dir)*vp;
	y += cos(dir)*vp;
	sprintf(buf,"%g%s%g", x, coord_sep, y );
    }
    uc_strcpy(ubuf,buf);
    GGadgetSetTitle( GWidgetGetControl(d->gw,CID_Vanish), ubuf );
}

static int PoV_Vanish(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	struct nldlg *d = GDrawGetUserData(GGadgetGetWindow(g));
	PoV_DoVanish(d);
    }
return( true );
}

int PointOfViewDlg(struct pov_data *pov, SplineFont *sf, int flags) {
    static struct pov_data def = { or_center, or_value, 0, 0, .1,
	    0, 3.1415926535897932/16, .2, 0 };
    double emsize = (sf->ascent + sf->descent);
    struct nldlg d;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[24], boxes[7], *varray[10][3], *harray1[3], *harray2[3],
	    *barray[9], *harray3[3], *harray4[3];
    GTextInfo label[24];
    int i,k,l;
    char xval[40], yval[40], zval[40], dval[40], tval[40], dirval[40];
    double x,y,z,dv,tilt,dir;
    int err;
    static int done = false;

    if ( !done ) {
	done = true;
	for ( i=0; originx[i].text!=NULL; ++i )
	    originx[i].text = (unichar_t *) _((char *) originx[i].text);
	for ( i=0; originy[i].text!=NULL; ++i )
	    originy[i].text = (unichar_t *) _((char *) originy[i].text);
    }

    *pov = def;
    pov->x *= emsize; pov->y *= emsize; pov->z *= emsize; pov->d *= emsize;
    if ( !(flags&1) ) {
	if ( pov->xorigin == or_lastpress ) pov->xorigin = or_center;
	if ( pov->yorigin == or_lastpress ) pov->yorigin = or_center;
    }

    memset(&d,'\0',sizeof(d));

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Point of View Projection");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,240));
    pos.height = GDrawPointsToPixels(NULL,216);
    d.gw = GDrawCreateTopWindow(NULL,&pos,nld_e_h,&d,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(boxes,0,sizeof(boxes));
    memset(label,0,sizeof(label));

    k=l=0;
    label[k].text = (unichar_t *) _("View Point");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = 8;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;
    varray[l][0] = &gcd[k-1]; varray[l][1] = GCD_ColSpan; varray[l++][2] = NULL;

    label[k].text = (unichar_t *) _("_X");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y + 16;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;
    harray1[0] = &gcd[k-1];

    for ( i=or_zero; i<or_undefined; ++i ) originx[i].selected = false;
    originx[pov->xorigin].selected = true;
    originx[or_lastpress].disabled = !(flags&1);
    gcd[k].gd.pos.x = 23; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.label = &originx[pov->xorigin];
    gcd[k].gd.u.list = originx;
    gcd[k].gd.cid = CID_XType;
    gcd[k++].creator = GListButtonCreate;
    harray1[1] = &gcd[k-1]; harray1[2] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray1;
    boxes[2].creator = GHBoxCreate;
    varray[l][0] = &boxes[2];

    sprintf( xval, "%g", rint(pov->x));
    label[k].text = (unichar_t *) xval;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 160; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;  gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.handle_controlevent = PoV_Vanish;
    gcd[k].gd.cid = CID_XValue;
    gcd[k++].creator = GTextFieldCreate;
    varray[l][1] = &gcd[k-1]; varray[l++][2] = NULL;

    label[k].text = (unichar_t *) _("_Y");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-3].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y + 28;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;
    harray2[0] = &gcd[k-1];

    for ( i=or_zero; i<or_undefined; ++i ) originy[i].selected = false;
    originy[pov->yorigin].selected = true;
    originy[or_lastpress].disabled = !(flags&1);
    gcd[k].gd.pos.x = gcd[k-3].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.label = &originy[pov->yorigin];
    gcd[k].gd.u.list = originy;
    gcd[k].gd.cid = CID_YType;
    gcd[k++].creator = GListButtonCreate;
    harray2[1] = &gcd[k-1]; harray2[2] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = harray2;
    boxes[3].creator = GHBoxCreate;
    varray[l][0] = &boxes[3];

    sprintf( yval, "%g", rint(pov->y));
    label[k].text = (unichar_t *) yval;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-3].gd.pos.x; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;  gcd[k].gd.pos.width = gcd[k-3].gd.pos.width;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_YValue;
    gcd[k].gd.handle_controlevent = PoV_Vanish;
    gcd[k++].creator = GTextFieldCreate;
    varray[l][1] = &gcd[k-1]; varray[l++][2] = NULL;

    label[k].text = (unichar_t *) _("Distance to drawing plane:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y + 28;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;
    varray[l][0] = &gcd[k-1];

    sprintf( zval, "%g", rint(pov->z));
    label[k].text = (unichar_t *) zval;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 160; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;  gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.handle_controlevent = PoV_Vanish;
    gcd[k].gd.cid = CID_ZValue;
    gcd[k++].creator = GTextFieldCreate;
    varray[l][1] = &gcd[k-1]; varray[l++][2] = NULL;

    label[k].text = (unichar_t *) _("Distance to projection plane:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-2].gd.pos.x; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y + 24;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;
    varray[l][0] = &gcd[k-1];

    sprintf( dval, "%g", rint(pov->d));
    label[k].text = (unichar_t *) dval;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 160; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;  gcd[k].gd.pos.width = 60;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.handle_controlevent = PoV_Vanish;
    gcd[k].gd.cid = CID_DValue;
    gcd[k++].creator = GTextFieldCreate;
    varray[l][1] = &gcd[k-1]; varray[l++][2] = NULL;

    label[k].text = (unichar_t *) _("Drawing plane tilt:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-2].gd.pos.x; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y + 24;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;
    varray[l][0] = &gcd[k-1];

    sprintf( tval, "%g", rint(pov->tilt*180/3.1415926535897932));
    label[k].text = (unichar_t *) tval;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 160; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;  gcd[k].gd.pos.width = 40;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.handle_controlevent = PoV_Vanish;
    gcd[k].gd.cid = CID_Tilt;
    gcd[k++].creator = GTextFieldCreate;
    harray3[0] = &gcd[k-1];

    label[k].text = (unichar_t *) U_("°");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+gcd[k-1].gd.pos.width+3; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;
    harray3[1] = &gcd[k-1]; harray3[2] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = harray3;
    boxes[4].creator = GHBoxCreate;
    varray[l][1] = &boxes[4]; varray[l++][2] = NULL;

    label[k].text = (unichar_t *) _("Direction of gaze:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-3].gd.pos.x; gcd[k].gd.pos.y = gcd[k-3].gd.pos.y + 24;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;
    varray[l][0] = &gcd[k-1];

    sprintf( dirval, "%g", rint(pov->direction*180/3.1415926535897932));
    label[k].text = (unichar_t *) dirval;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 160; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;  gcd[k].gd.pos.width = 40;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.handle_controlevent = PoV_Vanish;
    gcd[k].gd.cid = CID_GazeDirection;
    gcd[k++].creator = GTextFieldCreate;
    harray4[0] = &gcd[k-1];

    label[k].text = (unichar_t *) U_("°");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+gcd[k-1].gd.pos.width+3; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;
    harray4[1] = &gcd[k-1]; harray4[2] = NULL;

    boxes[5].gd.flags = gg_enabled|gg_visible;
    boxes[5].gd.u.boxelements = harray4;
    boxes[5].creator = GHBoxCreate;
    varray[l][1] = &boxes[5]; varray[l++][2] = NULL;

    label[k].text = (unichar_t *) _("Vanishing Point:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+18;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("This is the approximate location of the vanishing point.\nIt does not include the offset induced by \"Center of selection\"\nnor \"Last Press\".");
    gcd[k++].creator = GLabelCreate;
    varray[l][0] = &gcd[k-1];

    label[k].text = (unichar_t *) "123456.,123456.";
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 160; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.popup_msg = (unichar_t *) _("This is the approximate location of the vanishing point.\nIt does not include the offset induced by \"Center of selection\"\nnor \"Last Press\".");
    gcd[k].gd.cid = CID_Vanish;
    gcd[k++].creator = GLabelCreate;
    varray[l][1] = &gcd[k-1]; varray[l++][2] = NULL;

    gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+18;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = true;
    gcd[k++].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[k-1]; barray[2] = GCD_Glue;

    gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = false;
    gcd[k++].creator = GButtonCreate;
    barray[3] = GCD_Glue; barray[4] = &gcd[k-1]; barray[5] = GCD_Glue; barray[6] = NULL;

    boxes[6].gd.flags = gg_enabled|gg_visible;
    boxes[6].gd.u.boxelements = barray;
    boxes[6].creator = GHBoxCreate;
    varray[l][0] = &boxes[6]; varray[l][1] = GCD_ColSpan; varray[l++][2] = NULL;
    varray[l][0] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray[0];
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(d.gw,boxes);
    GHVBoxFitWindow(boxes[0].ret);
    PoV_DoVanish(&d);
    GDrawSetVisible(d.gw,true);
    while ( !d.done ) {
	GDrawProcessOneEvent(NULL);
	if ( d.done ) {
	    if ( !d.ok ) {
		GDrawDestroyWindow(d.gw);
return( -1 );
	    }
	    err = false;
	    x = GetReal8(d.gw,CID_XValue,_("_X"),&err);
	    y = GetReal8(d.gw,CID_YValue,_("_Y"),&err);
	    z = GetReal8(d.gw,CID_ZValue,_("Distance to drawing plane:"),&err);
	    dv = GetReal8(d.gw,CID_DValue,_("Distance to projection plane:"),&err);
	    tilt = GetReal8(d.gw,CID_Tilt,_("Drawing plane tilt:"),&err);
	    dir = GetReal8(d.gw,CID_GazeDirection,_("Direction of gaze:"),&err);
	    if ( err ) {
		d.done = d.ok = false;
    continue;
	    }
	    pov->x = x; pov->y = y; pov->z = z; pov->d = dv;
	    pov->tilt = tilt*3.1415926535897932/180;
	    pov->direction = dir*3.1415926535897932/180;
	    pov->xorigin = GGadgetGetFirstListSelectedItem( GWidgetGetControl(d.gw,CID_XType));
	    pov->yorigin = GGadgetGetFirstListSelectedItem( GWidgetGetControl(d.gw,CID_YType));
	}
    }

    GDrawDestroyWindow(d.gw);
    def = *pov;
    def.x /= emsize; def.y /= emsize; def.z /= emsize; def.d /= emsize;
return( 0 );		/* -1 => Canceled */
}

void CVPointOfView(CharView *cv,struct pov_data *pov) {
    int anysel = CVAnySel(cv,NULL,NULL,NULL,NULL);
    BasePoint origin;

    CVPreserveState((CharViewBase *) cv);

    origin.x = origin.y = 0;
    if ( pov->xorigin==or_center || pov->yorigin==or_center )
	CVFindCenter(cv,&origin,!anysel);
    if ( pov->xorigin==or_lastpress )
	origin.x = cv->p.cx;
    if ( pov->yorigin==or_lastpress )
	origin.y = cv->p.cy;
    if ( pov->xorigin!=or_value )
	pov->x = origin.x;
    if ( pov->yorigin!=or_value )
	pov->y = origin.y;

    MinimumDistancesFree(cv->b.sc->md); cv->b.sc->md = NULL;
    SPLPoV(cv->b.layerheads[cv->b.drawmode]->splines,pov,anysel);
    CVCharChangedUpdate(&cv->b);
}
