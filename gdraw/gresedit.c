/* Copyright (C) 2008-2012 by George Williams */
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
#include <ggadget.h>
#include "ggadgetP.h"
#include "../gdraw/gdrawP.h"		/* Need resolution of screen */
#include <gwidget.h>
#include <gresedit.h>
#include <string.h>
#include <stdlib.h>
#include <ustring.h>

struct tofree {
    GGadgetCreateData *gcd;
    GTextInfo *lab;
    GGadgetCreateData mainbox[2], flagsbox, colbox, extrabox, ibox, fontbox,
	sabox;
    GGadgetCreateData *marray[11], *farray[5][6], *carray[12][9], *(*earray)[8];
    GGadgetCreateData *iarray[4], *fontarray[5], *saarray[5];
    char *fontname;
    char **extradefs;
    char bw[20], padding[20], rr[20];
    GResInfo *res;
    int startcid, fontcid, btcid;
};

typedef struct greseditdlg {
    struct tofree *tofree;
    GWindow gw;
    GGadget *tabset;
    const char *def_res_file;
    void (*change_res_filename)(const char *);
    int done;
} GRE;

static GTextInfo bordertype[] = {
    { (unichar_t *) "None", NULL, 0, 0, (void *) (intpt) bt_none, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "Box", NULL, 0, 0, (void *) (intpt) bt_box, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "Raised", NULL, 0, 0, (void *) (intpt) bt_raised, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "Lowered", NULL, 0, 0, (void *) (intpt) bt_lowered, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "Engraved", NULL, 0, 0, (void *) (intpt) bt_engraved, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "Embossed", NULL, 0, 0, (void *) (intpt) bt_embossed, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "Double", NULL, 0, 0, (void *) (intpt) bt_double, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};

static GTextInfo bordershape[] = {
    { (unichar_t *) "Rect", NULL, 0, 0, (void *) (intpt) bs_rect, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "Round Rect", NULL, 0, 0, (void *) (intpt) bs_roundrect, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "Elipse", NULL, 0, 0, (void *) (intpt) bs_elipse, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "Diamond", NULL, 0, 0, (void *) (intpt) bs_diamond, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};

static void GRE_RefreshAll(GRE *gre) {
    GDrawRequestExpose(gre->gw,NULL,false);
    GDrawRequestExpose(GTabSetGetSubwindow(gre->tabset,GTabSetGetSel(gre->tabset)),NULL,false);
}

static char *GFontSpec2String(GFont *font) {
    int len;
    char *fontname;
    FontRequest rq;

    if ( font==NULL )
return( copy(""));

    GDrawDecomposeFont(font,&rq);
    if ( rq.family_name!=NULL )
	len = 4*u_strlen(rq.family_name);
    else
	len = strlen(rq.utf8_family_name);
    len += 6 /* point size */ + 1 +
	    5 /* weight */ + 1 +
	    10 /* style */;
    fontname = galloc(len);
    if ( rq.family_name!=NULL ) {
	char *utf8_name = u2utf8_copy(rq.family_name);
	sprintf( fontname, "%d %s%dpt %s", rq.weight,
	    rq.style&1 ? "italic " : "",
	    rq.point_size,
	    utf8_name );
	free(utf8_name );
    } else
	sprintf( fontname, "%d %s%dpt %s", rq.weight,
	    rq.style&1 ? "italic " : "",
	    rq.point_size,
	    rq.utf8_family_name );
return( fontname );
}

static void GRE_Reflow(GRE *gre,GResInfo *res) {
    if ( res->examples!=NULL &&
	    ( res->examples->creator==GHBoxCreate ||
	      res->examples->creator==GVBoxCreate ||
	      res->examples->creator==GHVBoxCreate ))
	GHVBoxReflow(res->examples->ret);
    GRE_RefreshAll(gre);
}

static void GRE_FigureInheritance( GRE *gre, GResInfo *parent, int cid_off_inh,
	int cid_off_data, int is_font, void *whatever,
	void (*do_something)(GRE *gre, int child_index, int cid_off_data, void *whatever)) {
    /* Look through the gadget tree for gadgets which inherit from parent */
    /*  (which just changed) then check if this child ggadget actually does */
    /*  inherit this field. If it does, set the field to the new value, and */
    /*  look for any grandchildren which inherit from the child, and so forth */
    GResInfo *child;
    int i;

    for ( i=0; (child = gre->tofree[i].res)!=NULL; ++i ) {
	if ( child->inherits_from==parent &&
		(( is_font && child->font!=NULL ) ||
		 ( !is_font && child->boxdata!=NULL)) ) {
	     /* Fonts may have a different cid offset depending on wether */
	     /* The ResInfo has a box or not */
	    if (( is_font && GGadgetIsChecked(GWidgetGetControl(gre->gw,gre->tofree[i].fontcid-2)) ) ||
		    ( !is_font && GGadgetIsChecked(GWidgetGetControl(gre->gw,gre->tofree[i].startcid+cid_off_inh)) )) {
		(do_something)(gre,i,cid_off_data,whatever);
		GRE_FigureInheritance(gre,child,cid_off_inh,cid_off_data,is_font,whatever,do_something);
	    }
	}
    }
}

static void inherit_color_change(GRE *gre, int childindex, int cid_off,
	void *whatever) {
    GGadget *g = GWidgetGetControl(gre->gw,gre->tofree[childindex].startcid+cid_off);
    Color col = (Color) (intpt) whatever;

    GColorButtonSetColor(g,col);
    *((Color *) GGadgetGetUserData(g)) = col;
}

static void inherit_list_change(GRE *gre, int childindex, int cid_off,
	void *whatever) {
    GGadget *g = GWidgetGetControl(gre->gw,gre->tofree[childindex].startcid+cid_off);
    int sel = (intpt) whatever;

    GGadgetSelectOneListItem(g,sel);
    *((uint8 *) GGadgetGetUserData(g)) = sel;
}

static void inherit_byte_change(GRE *gre, int childindex, int cid_off,
	void *whatever) {
    GGadget *g = GWidgetGetControl(gre->gw,gre->tofree[childindex].startcid+cid_off);
    int val = (intpt) whatever;
    char buf[20];

    sprintf( buf, "%d", val );
    GGadgetSetTitle8(g,buf);
    *((uint8 *) GGadgetGetUserData(g)) = val;
}

static void inherit_flag_change(GRE *gre, int childindex, int cid_off,
	void *whatever) {
    GGadget *g = GWidgetGetControl(gre->gw,gre->tofree[childindex].startcid+cid_off);
    int on = (intpt) whatever;
    int flag = (intpt) GGadgetGetUserData(g);
    GResInfo *res = gre->tofree[childindex].res;

    if ( on )
	res->boxdata->flags |= flag;
    else
	res->boxdata->flags &= ~flag;
    GGadgetSetChecked(g,on);
}

struct inherit_font_data { char *spec; GFont *font; };

static void inherit_font_change(GRE *gre, int childindex, int cid_off,
	void *whatever) {
    GGadget *g = GWidgetGetControl(gre->gw,gre->tofree[childindex].fontcid);
    struct inherit_font_data *ifd = (struct inherit_font_data *) whatever;

    if ( ifd->font!=NULL )
	*(gre->tofree[childindex].res->font) = ifd->font;
    GGadgetSetTitle8(g,ifd->spec);
}

static int GRE_InheritColChange(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GRE *gre = GDrawGetUserData(GGadgetGetWindow(g));
	int cid = GGadgetGetCid(g), on = GGadgetIsChecked(g);
	GGadgetSetEnabled(GWidgetGetControl(gre->gw,cid+1),!on);
	g = GWidgetGetControl(gre->gw,cid+2);
	GGadgetSetEnabled(g,!on);
	if ( on ) {
	    int index = GTabSetGetSel(gre->tabset);
	    GResInfo *res = gre->tofree[index].res;
	    int offset = ((char *) GGadgetGetUserData(g)) - ((char *) (res->boxdata));
	    Color col = *((Color *) (((char *) (res->inherits_from->boxdata))+offset));
	    if ( col!= *(Color *) GGadgetGetUserData(g) ) {
		int cid_off = cid - gre->tofree[index].startcid;
		GColorButtonSetColor(g,col);
		*((Color *) GGadgetGetUserData(g)) = col;
		GRE_FigureInheritance(gre,res,cid_off,cid_off+2,false,
			(void *) (intpt) col, inherit_color_change);
		GRE_RefreshAll(gre);
	    }
	}
    }
return( true );
}

static int GRE_InheritListChange(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GRE *gre = GDrawGetUserData(GGadgetGetWindow(g));
	int cid = GGadgetGetCid(g), on = GGadgetIsChecked(g);
	GGadgetSetEnabled(GWidgetGetControl(gre->gw,cid+1),!on);
	g = GWidgetGetControl(gre->gw,cid+2);
	GGadgetSetEnabled(g,!on);
	if ( on ) {
	    int index = GTabSetGetSel(gre->tabset);
	    GResInfo *res = gre->tofree[index].res;
	    int offset = ((char *) GGadgetGetUserData(g)) - ((char *) (res->boxdata));
	    int sel = *((uint8 *) (((char *) (res->inherits_from->boxdata))+offset));
	    if ( sel != *(uint8 *) GGadgetGetUserData(g) ) {
		int cid_off = cid - gre->tofree[index].startcid;
		GGadgetSelectOneListItem(g,sel);
		*((uint8 *) GGadgetGetUserData(g)) = sel;
		GRE_FigureInheritance(gre,res,cid_off,cid_off+2,false,
			(void *) (intpt) sel, inherit_list_change);
		GRE_Reflow(gre,res);
	    }
	}
    }
return( true );
}

static int GRE_InheritByteChange(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GRE *gre = GDrawGetUserData(GGadgetGetWindow(g));
	int cid = GGadgetGetCid(g), on = GGadgetIsChecked(g);
	GGadgetSetEnabled(GWidgetGetControl(gre->gw,cid+1),!on);
	g = GWidgetGetControl(gre->gw,cid+2);
	GGadgetSetEnabled(g,!on);
	if ( on ) {
	    int index = GTabSetGetSel(gre->tabset);
	    GResInfo *res = gre->tofree[index].res;
	    int offset = ((char *) GGadgetGetUserData(g)) - ((char *) (res->boxdata));
	    int val = *((uint8 *) (((char *) (res->inherits_from->boxdata))+offset));
	    if ( val != *(uint8 *) GGadgetGetUserData(g) ) {
		int cid_off = cid - gre->tofree[index].startcid;
		char buf[20];
		sprintf( buf, "%d", val );
		GGadgetSetTitle8(g,buf);
		*((uint8 *) GGadgetGetUserData(g)) = val;
		GRE_FigureInheritance(gre,res,cid_off,cid_off+2,false,
			(void *) (intpt) val, inherit_byte_change);
		GRE_Reflow(gre,res);
	    }
	}
    }
return( true );
}

static int GRE_InheritFlagChange(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GRE *gre = GDrawGetUserData(GGadgetGetWindow(g));
	int cid = GGadgetGetCid(g), on = GGadgetIsChecked(g);
	g = GWidgetGetControl(gre->gw,cid+1);
	GGadgetSetEnabled(g,!on);
	if ( on ) {
	    int index = GTabSetGetSel(gre->tabset);
	    GResInfo *res = gre->tofree[index].res;
	    int flag = (intpt) GGadgetGetUserData(g);
	    if ( (res->boxdata->flags&flag) != (res->inherits_from->boxdata->flags&flag)) {
		int cid_off = cid - gre->tofree[index].startcid;
		int on = (res->inherits_from->boxdata->flags&flag)?1:0 ;
		GGadgetSetChecked(g,on);
		if ( on )
		    res->boxdata->flags |= flag;
		else
		    res->boxdata->flags &= ~flag;
		GRE_FigureInheritance(gre,res,cid_off,cid_off+2,false,
			(void *) (intpt) on, inherit_flag_change);
		GRE_Reflow(gre,res);
	    }
	}
    }
return( true );
}

static int GRE_InheritFontChange(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GRE *gre = GDrawGetUserData(GGadgetGetWindow(g));
	int cid = GGadgetGetCid(g), on = GGadgetIsChecked(g);
	GGadgetSetEnabled(GWidgetGetControl(gre->gw,cid+1),!on);
	g = GWidgetGetControl(gre->gw,cid+2);
	GGadgetSetEnabled(g,!on);
	if ( on ) {
	    int index = GTabSetGetSel(gre->tabset);
	    GResInfo *res = gre->tofree[index].res;
	    int pi;
	    for ( pi=0; gre->tofree[pi].res!=NULL && gre->tofree[pi].res!=res->inherits_from; ++pi );
	    if ( gre->tofree[pi].res!=NULL ) {
		struct inherit_font_data ifd;
		int cid_off = cid - gre->tofree[index].startcid;

		ifd.spec = GGadgetGetTitle8(GWidgetGetControl(gre->gw,gre->tofree[pi].fontcid));
		ifd.font = *gre->tofree[pi].res->font;
		*res->font = ifd.font;
 
		GGadgetSetTitle8(g,ifd.spec);
		GRE_FigureInheritance(gre,res,cid_off,cid_off+2,false,
			(void *) &ifd, inherit_font_change);
		free( ifd.spec );
	    }
	}
    }
return( true );
}

static int GRE_FlagChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GRE *gre = GDrawGetUserData(GGadgetGetWindow(g));
	int index = GTabSetGetSel(gre->tabset);
	GResInfo *res = gre->tofree[index].res;
	int cid_off = GGadgetGetCid(g) - gre->tofree[index].startcid;
	int on;
	if ( (on = GGadgetIsChecked(g)) )
	    res->boxdata->flags |= (int) (intpt) GGadgetGetUserData(g);
	else
	    res->boxdata->flags &= ~(int) (intpt) GGadgetGetUserData(g);
	GRE_FigureInheritance(gre,res,cid_off-1,cid_off,false,
		(void *) (intpt) on, inherit_flag_change);
	GRE_Reflow(gre,res);
    }
return( true );
}

static int GRE_ListChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	GRE *gre = GDrawGetUserData(GGadgetGetWindow(g));
	int index = GTabSetGetSel(gre->tabset);
	GResInfo *res = gre->tofree[index].res;
	int cid_off = GGadgetGetCid(g) - gre->tofree[index].startcid;
	int sel = GGadgetGetFirstListSelectedItem(g);

	*((uint8 *) GGadgetGetUserData(g)) = sel;
	GRE_FigureInheritance(gre,res,cid_off-2,cid_off,false,
		(void *) (intpt) sel, inherit_list_change);
	GRE_Reflow(gre,res);
    }
return( true );
}

static int GRE_BoolChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	*((int *) GGadgetGetUserData(g)) = GGadgetIsChecked(g);
    }
return( true );
}

static int GRE_ByteChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GRE *gre = GDrawGetUserData(GGadgetGetWindow(g));
	char *txt = GGadgetGetTitle8(g), *end;
	int val = strtol(txt,&end,10);
	int index = GTabSetGetSel(gre->tabset);
	GResInfo *res = gre->tofree[index].res;
	if ( *end=='\0' && val>=0 && val<=255 ) {
	    int cid_off = GGadgetGetCid(g) - gre->tofree[index].startcid;
	    *((uint8 *) GGadgetGetUserData(g)) = val;
	    GRE_FigureInheritance(gre,res,cid_off-2,cid_off,false,
		    (void *) (intpt) val, inherit_byte_change);
	    GRE_Reflow(gre,res);
	}
	free(txt);
    }
return( true );
}

static int GRE_IntChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GRE *gre = GDrawGetUserData(GGadgetGetWindow(g));
	char *txt = GGadgetGetTitle8(g), *end;
	int val = strtol(txt,&end,10);
	int index = GTabSetGetSel(gre->tabset);
	GResInfo *res = gre->tofree[index].res;
	if ( *end=='\0' ) {
	    *((int *) GGadgetGetUserData(g)) = val;
	    GRE_Reflow(gre,res);
	}
	free(txt);
    }
return( true );
}

static int GRE_DoubleChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	char *txt = GGadgetGetTitle8(g), *end;
	double val = strtod(txt,&end);
	if ( *end=='\0' )
	    *((double *) GGadgetGetUserData(g)) = val;
	free(txt);
    }
return( true );
}

static int GRE_ColorChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GRE *gre = GDrawGetUserData(GGadgetGetWindow(g));
	int index = GTabSetGetSel(gre->tabset);
	GResInfo *res = gre->tofree[index].res;
	int cid_off = GGadgetGetCid(g) - gre->tofree[index].startcid;
	Color col = GColorButtonGetColor(g);

	*((Color *) GGadgetGetUserData(g)) = col;
	GRE_FigureInheritance(gre,res,cid_off-2,cid_off,false,
		(void *) (intpt) col, inherit_color_change);
	GRE_RefreshAll(gre);
    }
return( true );
}

static int GRE_ExtraColorChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	Color col = GColorButtonGetColor(g);

	*((Color *) GGadgetGetUserData(g)) = col;
    }
return( true );
}

static int GRE_FontChanged(GGadget *g, GEvent *e) {
    struct inherit_font_data ifd;

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GRE *gre = GDrawGetUserData(GGadgetGetWindow(g));
	int index = GTabSetGetSel(gre->tabset);
	GResInfo *res = gre->tofree[index].res;
	int cid_off = GGadgetGetCid(g) - gre->tofree[index].startcid;
	char *fontdesc = GGadgetGetTitle8(g);
	ifd.spec = fontdesc;
	ifd.font = NULL;

	GRE_FigureInheritance(gre,res,cid_off-2,cid_off,true,
		(void *) &ifd, inherit_font_change);
	free(fontdesc);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_textfocuschanged &&
	    !e->u.control.u.tf_focus.gained_focus ) {
	GRE *gre = GDrawGetUserData(GGadgetGetWindow(g));
	if ( gre->tabset!=NULL ) {
	    int index = GTabSetGetSel(gre->tabset);
	    GResInfo *res = gre->tofree[index].res;
	    int cid_off = GGadgetGetCid(g) - gre->tofree[index].startcid;
	    char *fontdesc = GGadgetGetTitle8(g);
	    GFont *new = GResource_font_cvt(fontdesc,NULL);

	    if ( new==NULL )
		gwwv_post_error(_("Bad font"),_("Bad font specification"));
	    else {
		ifd.spec = fontdesc;
		ifd.font = new;

		*((GFont **) GGadgetGetUserData(g)) = new;
		GRE_FigureInheritance(gre,res,cid_off-2,cid_off,true,
			(void *) &ifd, inherit_font_change);
		GRE_Reflow(gre,res);
	    }
	    free(fontdesc);
	}
    }
return( true );
}

static void GRE_ParseFont(GGadget *g) {
    char *fontdesc = GGadgetGetTitle8(g);
    GFont *new = GResource_font_cvt(fontdesc,NULL);

    if ( new==NULL )
	gwwv_post_error(_("Bad font"),_("Bad font specification"));
    else {
	*((GFont **) GGadgetGetUserData(g)) = new;
    }
    free(fontdesc);
}

static int GRE_ExtraFontChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_textfocuschanged &&
	    !e->u.control.u.tf_focus.gained_focus ) {
	GRE *gre = GDrawGetUserData(GGadgetGetWindow(g));
	if ( gre->tabset!=NULL ) {
	    GRE_ParseFont(g);
	}
    }
return( true );
}

static int GRE_StringChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	/*GRE *gre = GDrawGetUserData(GGadgetGetWindow(g));*/
    }
return( true );
}

static int GRE_ImageChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GRE *gre = GDrawGetUserData(GGadgetGetWindow(g));
	GResImage **ri = GGadgetGetUserData(g);
	char *new;
	GImage *newi;
	GRect size;
	new = gwwv_open_filename_with_path("Find Image",
		(*ri)==NULL?NULL: (*ri)->filename,
		"*.{png,jpeg,jpg,tiff,bmp,xbm}",NULL,
		_GGadget_GetImagePath());
	if ( new==NULL )
return( true );
	newi = GImageRead(new);
	if ( newi==NULL ) {
	    gwwv_post_error(_("Could not open image"),_("Could not open %s"), new );
	    free( new );
	} else if ( *ri==NULL ) {
	    *ri = gcalloc(1,sizeof(GResImage));
	    (*ri)->filename = new;
	    (*ri)->image = newi;
	    GGadgetSetTitle8(g,"...");
	} else {
	    free( (*ri)->filename );
	    (*ri)->filename = new;
	    if ( !_GGadget_ImageInCache((*ri)->image ) )
		GImageDestroy((*ri)->image);
	    (*ri)->image = newi;
	}
	((GButton *) g)->image = newi;
	GGadgetGetDesiredSize(g,&size,NULL);
	GGadgetResize(g,size.width,size.height);
	GRE_RefreshAll(gre);
    }
return( true );
}

static void GRE_DoCancel(GRE *gre) {
    int i;

    for ( i=0; gre->tofree[i].res!=NULL; ++i ) {
	GResInfo *res = gre->tofree[i].res;
	struct resed *extras;
	GResImage **_ri, *ri;
	if ( res->boxdata!=NULL )
	    *res->boxdata = res->orig_state;
	if ( res->extras!=NULL ) {
	    for ( extras = res->extras; extras->name!=NULL; extras++ ) {
		switch ( extras->type ) {
		  case rt_bool:
		  case rt_int:
		  case rt_color:
		  case rt_coloralpha:
		    *(int *) (extras->val) = extras->orig.ival;
		  break;
		  case rt_double:
		    *(int *) (extras->val) = extras->orig.dval;
		  break;
		  case rt_font:
		    *(GFont **) (extras->val) = extras->orig.fontval;
		  break;
		  case rt_image:
		    _ri = extras->val;
		    ri = *_ri;
		    if ( extras->orig.sval==NULL ) {
			if ( ri!=NULL ) {
			    free(ri->filename);
			    if ( ri->image!=NULL )
				GImageDestroy(ri->image);
			    free(ri);
			    *_ri = NULL;
			}
		    } else {
			if ( strcmp(extras->orig.sval,ri->filename)!=0 ) {
			    GImage *temp;
			    temp = GImageRead(extras->orig.sval);
			    if ( temp!=NULL ) {
				if ( !_GGadget_ImageInCache(ri->image ) )
				    GImageDestroy(ri->image);
				ri->image = temp;
			        free(ri->filename);
			        ri->filename = copy(extras->orig.sval);
			    }
			}
		    }
		  break;
		  case rt_string: case rt_stringlong:
		    /* These don't change dynamically */
		    /* Nothing to revert here */
		  break;
		}
	    }
	}
    }
    gre->done = true;
}

static int GRE_ChangePane(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GRE *gre = GDrawGetUserData(GGadgetGetWindow(g));
	GResInfo *new = GGadgetGetUserData(g);
	int i;
	for ( i=0; gre->tofree[i].res!=NULL && gre->tofree[i].res != new; ++i );
	if ( gre->tofree[i].res!=NULL )
	    GTabSetSetSel(gre->tabset,i);
    }
return( true );
}

static int GRE_Cancel(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GRE *gre = GDrawGetUserData(GGadgetGetWindow(g));
	GRE_DoCancel(gre);
    }
return( true );
}

static int TogglePrefs(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	*((int *) GGadgetGetUserData(g)) = GGadgetIsChecked(g);
    }
return( true );
}

static int GRE_Save(GGadget *g, GEvent *e) {
    static char *shapes[] = { "rect", "roundrect", "elipse", "diamond", NULL };
    static char *types[] = { "none", "box", "raised", "lowered", "engraved",
	    "embossed", "double", NULL };

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GRE *gre = GDrawGetUserData(GGadgetGetWindow(g));
	char *filename;
	FILE *output;
	GGadgetCreateData gcd[2], *gcdp=NULL;
	GTextInfo lab[1];
	int update_prefs = true;
	int i,j;
	static char *flagnames[] = { "BorderInner", "BorderOuter", "ActiveInner",
		"ShadowOuter", "DoDepressedBackground", "DrawDefault",
		"GradientBG", NULL };
	static char *colornames[] = { "NormalForeground", "DisabledForeground", "NormalBackground",
		"DisabledBackground", "PressedBackground", "GradientStartCol", "BorderBrightest",
		"BorderBrighter", "BorderDarker", "BorderDarkest", "BorderInnerCol", "BorderOuterCol",
		"ActiveBorder",
		NULL };
	static char *intnames[] = { "BorderWidth", "Padding", "Radius", NULL };
	struct resed *extras;

	if ( gre->change_res_filename != NULL ) {
	    memset(gcd,0,sizeof(gcd));
	    memset(lab,0,sizeof(lab));
	    lab[0].text = (unichar_t *) _("Store this filename in preferences");
	    lab[0].text_is_1byte = true;
	    gcd[0].gd.label = &lab[0];
	    gcd[0].gd.flags = gg_visible|gg_enabled|gg_cb_on;
	    gcd[0].gd.handle_controlevent = TogglePrefs;
	    gcd[0].data = &update_prefs;
	    gcd[0].creator = GCheckBoxCreate;
	    gcdp = gcd;
	}

	filename = gwwv_save_filename_with_gadget(_("Save Resource file as..."),gre->def_res_file,NULL,gcdp);
	if ( filename==NULL )
return( true );
	output = fopen( filename,"w" );
	if ( output==NULL ) {
	    gwwv_post_error(_("Open failed"), _("Failed to open %s for output"), filename );
return( true );
	}
	for ( i=0; gre->tofree[i].res!=NULL; ++i ) {
	    GResInfo *res = gre->tofree[i].res;
	    int cid = gre->tofree[i].startcid;
	    if ( res->boxdata!=NULL ) {
		for ( j=0; flagnames[j]!=NULL; ++j ) {
		    if ( !GGadgetIsChecked( GWidgetGetControl(gre->gw,cid)) ||
			    (res->override_mask&(1<<j)) ) {
			fprintf( output, "%s.%s.Box.%s: %s\n",
				res->progname, res->resname, flagnames[j],
			        GGadgetIsChecked( GWidgetGetControl(gre->gw,cid+1))?"True" : "False" );
		    }
		    cid += 2;
		}
		for ( j=0; colornames[j]!=NULL; ++j ) {
		    if ( !GGadgetIsChecked( GWidgetGetControl(gre->gw,cid))
			    || (res->override_mask&(1<<(j+16))) ) {
			fprintf( output, "%s.%s.Box.%s: #%06x\n",
				res->progname, res->resname, colornames[j],
			        GColorButtonGetColor( GWidgetGetControl(gre->gw,cid+2)) );
		    }
		    cid += 3;
		}
		if ( !GGadgetIsChecked( GWidgetGetControl(gre->gw,cid))
			|| (res->override_mask&omf_border_type) ) {
		    fprintf( output, "%s.%s.Box.BorderType: %s\n",
			    res->progname, res->resname,
			    types[ GGadgetGetFirstListSelectedItem(
				     GWidgetGetControl(gre->gw,cid+2)) ] );
		}
		cid += 3;
		if ( !GGadgetIsChecked( GWidgetGetControl(gre->gw,cid))
			|| (res->override_mask&omf_border_shape) ) {
		    fprintf( output, "%s.%s.Box.BorderShape: %s\n",
			    res->progname, res->resname,
			    shapes[ GGadgetGetFirstListSelectedItem(
				     GWidgetGetControl(gre->gw,cid+2)) ] );
		}
		cid += 3;
		for ( j=0; intnames[j]!=NULL; ++j ) {
		    if ( !GGadgetIsChecked( GWidgetGetControl(gre->gw,cid))
			    || (res->override_mask&(1<<(j+10))) ) {
			char *ival = GGadgetGetTitle8( GWidgetGetControl(gre->gw,cid+2));
			char *end;
			int val = strtol(ival,&end,10);
			if ( *end!='\0' || val<0 || val>255 )
			    gwwv_post_error(_("Bad Number"), _("Bad numeric value for %s.%s"),
				    res->resname, intnames[j]);
			fprintf( output, "%s.%s.Box.%s: %s\n",
				res->progname, res->resname, intnames[j], ival );
			free(ival);
		    }
		    cid += 3;
		}
	    }
	    if ( res->font!=NULL ) {
		if ( !GGadgetIsChecked( GWidgetGetControl(gre->gw,gre->tofree[i].fontcid-1))
			|| (res->override_mask&omf_font) ) {
		    char *ival = GGadgetGetTitle8( GWidgetGetControl(gre->gw,gre->tofree[i].fontcid));
		    fprintf( output, "%s.%s.Font: %s\n",
			    res->progname, res->resname, ival );
		    free(ival);
		}
	    }
	    if ( res->extras!=NULL ) for ( extras=res->extras; extras->name!=NULL; ++extras ) {
		GGadget *g = GWidgetGetControl(gre->gw,extras->cid);
		switch ( extras->type ) {
		  case rt_bool:
		    fprintf( output, "%s.%s%s%s: %s\n",
			    res->progname, res->resname, *res->resname=='\0'?"":".",extras->resname,
			    GGadgetIsChecked(g)?"True":"False");
		  break;
		  case rt_int: {
		    char *ival = GGadgetGetTitle8( g );
		    char *end;
		    (void) strtol(ival,&end,10);
		    if ( *end!='\0' )
			gwwv_post_error(_("Bad Number"), _("Bad numeric value for %s.%s"),
				res->resname, extras->name );
		    fprintf( output, "%s.%s%s%s: %s\n",
			    res->progname, res->resname, *res->resname=='\0'?"":".", extras->resname,
			    ival );
		    free(ival);
		  } break;
		  case rt_double: {
		    char *dval = GGadgetGetTitle8( g );
		    char *end;
		    (void) strtod(dval,&end);
		    if ( *end!='\0' )
			gwwv_post_error(_("Bad Number"), _("Bad numeric value for %s.%s"),
				res->resname, extras->name );
		    fprintf( output, "%s.%s%s%s: %s\n",
			    res->progname, res->resname, *res->resname=='\0'?"":".", extras->resname,
			    dval );
		    free(dval);
		  } break;
		  case rt_color:
		  case rt_coloralpha:
		    fprintf( output, "%s.%s%s%s: #%06x\n",
			    res->progname, res->resname, *res->resname=='\0'?"":".", extras->resname,
			    GColorButtonGetColor(g) );
		  break;
		  case rt_font: {
		    char *fontdesc = GGadgetGetTitle8(g);
		    if ( *fontdesc!='\0' )
			fprintf( output, "%s.%s%s%s: %s\n",
				res->progname, res->resname, *res->resname=='\0'?"":".", extras->resname,
				fontdesc );
		    free(fontdesc);
		  } break;
		  case rt_image: {
		    GResImage *ri = *((GResImage **) (extras->val));
		    if ( ri!=NULL && ri->filename!=NULL ) {
			char **paths = _GGadget_GetImagePath();
			int i;
			for ( i=0; paths[i]!=NULL; ++i ) {
			    if ( strncmp(paths[i],ri->filename,strlen(paths[i]))==0 ) {
				char *pt = ri->filename+strlen(paths[i]);
			        while ( *pt=='/' ) ++pt;
				fprintf( output, "%s.%s%s%s: %s\n",
					res->progname, res->resname, *res->resname=='\0'?"":".", extras->resname,
					pt );
			break;
			    }
			}
			if ( paths[i]==NULL )
			    fprintf( output, "%s.%s%s%s: %s\n",
				    res->progname, res->resname, *res->resname=='\0'?"":".", extras->resname,
				    ri->filename );
		    }
		  } break;
		  case rt_string: case rt_stringlong: {
		    char *sval = GGadgetGetTitle8( g );
		    fprintf( output, "%s.%s%s%s: %s\n",
			    res->progname, res->resname, *res->resname=='\0'?"":".", extras->resname,
			    sval );
		    free(sval);
		  } break;
		}
	    }
	    fprintf( output, "\n" );
	}
	if ( ferror(output))
	    gwwv_post_error(_("Write failed"),_("An error occurred when writing the resource file"));
	fclose(output);
	if ( gre->change_res_filename != NULL && update_prefs )
	    (gre->change_res_filename)(filename);
	free(filename);
    }
return( true );
}

static int GRE_OK(GGadget *g, GEvent *e) {
    int i;
    struct resed *extras;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GRE *gre = GDrawGetUserData(GGadgetGetWindow(g));
	/* Handle fonts, strings and images */
	for ( i=0; gre->tofree[i].res!=NULL; ++i ) {
	    GResInfo *res = gre->tofree[i].res;
	    if ( res->boxdata!=NULL ) {
		static char *names[] = { N_("Border Width"), N_("Padding"), N_("Radius"), NULL };
		int j;
		for ( j=0; j<3; ++j ) {
		    char *sval = GGadgetGetTitle8(GWidgetGetControl(gre->gw,gre->tofree[i].btcid+6+3*j));
		    char *end;
		    int val = strtol(sval,&end,10);
		    if ( *end!='\0' || val<0 || val>255 ) {
			gwwv_post_error(_("Bad Number"), _("Bad numeric value for %s.%s must be between 0 and 255"),
				res->resname, _(names[j]) );
			free(sval);
return( true );
		    }
		    free(sval);
		}
	    }
	    if ( res->font!=NULL ) {
		/* We try to do this as we go along, but we wait for a focus */
		/*  out event to parse changes. Now if the last thing they were*/
		/*  doing was editing the font, then we might not get a focus */
		/*  out. So parse things here, just in case */
		GRE_ParseFont(GWidgetGetControl(gre->gw,gre->tofree[i].fontcid));
	    }
	    if ( res->extras!=NULL ) {
		
		for ( extras = res->extras; extras->name!=NULL; extras++ ) {
		    switch ( extras->type ) {
		      case rt_int: {
			/* Has already been parsed and set -- unless there were */
			/*  an error. We need to complain about that */
			char *ival = GGadgetGetTitle8( GWidgetGetControl(gre->gw,extras->cid) );
			char *end;
			(void) strtol(ival,&end,10);
			if ( *end!='\0' ) {
			    gwwv_post_error(_("Bad Number"), _("Bad numeric value for %s.%s"),
				    res->resname, extras->name );
			    free(ival);
return( true );
			}
			free(ival);
		      } break;
		      case rt_double: {
			/* Has already been parsed and set -- unless there were */
			/*  an error. We need to complain about that */
			char *ival = GGadgetGetTitle8( GWidgetGetControl(gre->gw,extras->cid) );
			char *end;
			(void) strtod(ival,&end);
			if ( *end!='\0' ) {
			    gwwv_post_error(_("Bad Number"), _("Bad numeric value for %s.%s"),
				    res->resname, extras->name );
			    free(ival);
return( true );
			}
			free(ival);
		      } break;
		      case rt_bool:
		      case rt_color:
		      case rt_coloralpha:
		      case rt_image:
			/* These should have been set as we went along */
		      break;
		      case rt_font:
			GRE_ParseFont(GWidgetGetControl(gre->gw,extras->cid));
		      break;
		      case rt_string: case rt_stringlong:
		      {
			char *spec = GGadgetGetTitle8(GWidgetGetControl(gre->gw,extras->cid));
			/* We can't free the old value, because sometimes it is a */
			/*  static string, not something allocated */
			if ( *spec=='\0' ) { free( spec ); spec=NULL; }
			*(char **) (extras->val) = spec;
		      } break;
		    }
		}
	    }
	}
	gre->done = true;
    }
return( true );
}

static int gre_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	GRE *gre = GDrawGetUserData(gw);
	GRE_DoCancel(gre);
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

static void GResEditDlg(GResInfo *all,const char *def_res_file,void (*change_res_filename)(const char *)) {
    GResInfo *res, *parent;
    int cnt;
    GGadgetCreateData topgcd[4], topbox[3], *gcd, *barray[10], *tarray[3][2];
    GTextInfo *lab, toplab[4];
    GTabInfo *panes;
    struct tofree *tofree;
    struct resed *extras;
    int i,j,k,l,cid;
    static GBox small_blue_box;
    extern GBox _GGadget_button_box;
    GRE gre;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;

    memset(&gre,0,sizeof(gre));
    gre.def_res_file = def_res_file;
    gre.change_res_filename = change_res_filename;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = true;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("X Resource Editor");
    pos.x = pos.y = 10;
    pos.width = pos.height = 100;
    gre.gw = gw = GDrawCreateTopWindow(NULL,&pos,gre_e_h,&gre,&wattrs);

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
		small_blue_box.main_background = small_blue_box.main_background;
    }

    for ( res=all, cnt=0; res!=NULL; res=res->next, ++cnt );

    panes = gcalloc(cnt+1,sizeof(GTabInfo));
    gre.tofree = tofree = gcalloc(cnt+1,sizeof(struct tofree));
    cid = 0;
    for ( res=all, i=0; res!=NULL; res=res->next, ++i ) {
	tofree[i].res = res;
	tofree[i].startcid = cid+1;

	cnt = 0;
	if ( res->extras!=NULL )
	    for ( extras=res->extras, cnt = 0; extras->name!=NULL; ++cnt, ++extras );
	tofree[i].earray = gcalloc(cnt+1,sizeof(GGadgetCreateData[8]));
	tofree[i].extradefs = gcalloc(cnt+1,sizeof(char *));
	cnt *= 2;
	if ( res->initialcomment!=NULL )
	    ++cnt;
	if ( res->boxdata!=NULL )
	    cnt += 3*18 + 8*2;
	if ( res->font )
	    cnt+=3;
	if ( res->inherits_from!=NULL )
	    cnt+=2;
	if ( res->seealso1!=NULL ) {
	    cnt+=2;
	    if ( res->seealso2!=NULL )
		++cnt;
	}

	tofree[i].gcd = gcd = gcalloc(cnt,sizeof(GGadgetCreateData));
	tofree[i].lab = lab = gcalloc(cnt,sizeof(GTextInfo));

	j=k=l=0;
	if ( res->initialcomment!=NULL ) {
	    lab[k].text = (unichar_t *) S_(res->initialcomment);
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].marray[j++] = &gcd[k-1];
	}
	if ( res->examples!=NULL )
	    tofree[i].marray[j++] = res->examples;
	if ( res->inherits_from != NULL ) {
	    lab[k].text = (unichar_t *) _("Inherits from");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].iarray[0] = &gcd[k-1];

	    lab[k].text = (unichar_t *) S_(res->inherits_from->name);
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_dontcopybox;
	    gcd[k].gd.box = &small_blue_box;
	    gcd[k].data = res->inherits_from;
	    gcd[k].gd.handle_controlevent = GRE_ChangePane;
	    gcd[k++].creator = GButtonCreate;
	    tofree[i].iarray[1] = &gcd[k-1];

	    tofree[i].iarray[2] = GCD_Glue; tofree[i].iarray[3] = NULL;
	    tofree[i].ibox.gd.flags = gg_visible|gg_enabled;
	    tofree[i].ibox.gd.u.boxelements = tofree[i].iarray;
	    tofree[i].ibox.creator = GHBoxCreate;
	    tofree[i].marray[j++] = &tofree[i].ibox;
	} else if ( res->boxdata!=NULL ) {
	    lab[k].text = (unichar_t *) _("Does not inherit from anything");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].marray[j++] = &gcd[k-1];
	}

	if ( res->boxdata!=NULL ) {
	    res->orig_state = *res->boxdata;

	    lab[k].text = (unichar_t *) _("Inherit");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritFlagChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].farray[l][0] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Outline Inner Border");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    if ( res->boxdata->flags & box_foreground_border_inner )
		gcd[k].gd.flags |= gg_cb_on;
	    gcd[k].gd.handle_controlevent = GRE_FlagChanged;
	    gcd[k].data = (void *) (intpt) box_foreground_border_inner;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].farray[l][1] = &gcd[k-1];
	    if ( res->inherits_from==NULL )
		gcd[k-2].gd.flags &= ~gg_enabled;
	    else if ( (res->inherits_from->boxdata->flags&box_foreground_border_inner) == (res->boxdata->flags&box_foreground_border_inner) ) {
		gcd[k-2].gd.flags |= gg_cb_on;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

	    lab[k].text = (unichar_t *) _("Inherit");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritFlagChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].farray[l][2] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Outline Outer Border");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    if ( res->boxdata->flags & box_foreground_border_outer )
		gcd[k].gd.flags |= gg_cb_on;
	    gcd[k].gd.handle_controlevent = GRE_FlagChanged;
	    gcd[k].data = (void *) (intpt) box_foreground_border_outer;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].farray[l][3] = &gcd[k-1];
	    if ( res->inherits_from==NULL )
		gcd[k-2].gd.flags &= ~gg_enabled;
	    else if ( (res->inherits_from->boxdata->flags&box_foreground_border_outer) == (res->boxdata->flags&box_foreground_border_outer) ) {
		gcd[k-2].gd.flags |= gg_cb_on;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }
	    tofree[i].farray[l][4] = GCD_Glue;
	    tofree[i].farray[l++][5] = NULL;


	    lab[k].text = (unichar_t *) _("Inherit");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritFlagChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].farray[l][0] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Show Active Border");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    if ( res->boxdata->flags & box_active_border_inner )
		gcd[k].gd.flags |= gg_cb_on;
	    gcd[k].gd.handle_controlevent = GRE_FlagChanged;
	    gcd[k].data = (void *) (intpt) box_active_border_inner;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].farray[l][1] = &gcd[k-1];
	    if ( res->inherits_from==NULL )
		gcd[k-2].gd.flags &= ~gg_enabled;
	    else if ( (res->inherits_from->boxdata->flags&box_active_border_inner) == (res->boxdata->flags&box_active_border_inner) ) {
		gcd[k-2].gd.flags |= gg_cb_on;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

	    lab[k].text = (unichar_t *) _("Inherit");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritFlagChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].farray[l][2] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Outer Shadow");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    if ( res->boxdata->flags & box_foreground_shadow_outer )
		gcd[k].gd.flags |= gg_cb_on;
	    gcd[k].gd.handle_controlevent = GRE_FlagChanged;
	    gcd[k].data = (void *) (intpt) box_foreground_shadow_outer;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].farray[l][3] = &gcd[k-1];
	    if ( res->inherits_from==NULL )
		gcd[k-2].gd.flags &= ~gg_enabled;
	    else if ( (res->inherits_from->boxdata->flags&box_foreground_shadow_outer) == (res->boxdata->flags&box_foreground_shadow_outer) ) {
		gcd[k-2].gd.flags |= gg_cb_on;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }
	    tofree[i].farray[l][4] = GCD_Glue;
	    tofree[i].farray[l++][5] = NULL;


	    lab[k].text = (unichar_t *) _("Inherit");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritFlagChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].farray[l][0] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Depressed Background");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    if ( res->boxdata->flags & box_do_depressed_background )
		gcd[k].gd.flags |= gg_cb_on;
	    gcd[k].gd.handle_controlevent = GRE_FlagChanged;
	    gcd[k].data = (void *) (intpt) box_do_depressed_background;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].farray[l][1] = &gcd[k-1];
	    if ( res->inherits_from==NULL )
		gcd[k-2].gd.flags &= ~gg_enabled;
	    else if ( (res->inherits_from->boxdata->flags&box_do_depressed_background) == (res->boxdata->flags&box_do_depressed_background) ) {
		gcd[k-2].gd.flags |= gg_cb_on;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

	    lab[k].text = (unichar_t *) _("Inherit");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritFlagChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].farray[l][2] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Outline Default Button");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    if ( res->boxdata->flags & box_draw_default )
		gcd[k].gd.flags |= gg_cb_on;
	    gcd[k].gd.handle_controlevent = GRE_FlagChanged;
	    gcd[k].data = (void *) (intpt) box_draw_default;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].farray[l][3] = &gcd[k-1];
	    if ( res->inherits_from==NULL )
		gcd[k-2].gd.flags &= ~gg_enabled;
	    else if ( (res->inherits_from->boxdata->flags&box_draw_default) == (res->boxdata->flags&box_draw_default) ) {
		gcd[k-2].gd.flags |= gg_cb_on;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }
	    tofree[i].farray[l][4] = GCD_Glue;
	    tofree[i].farray[l++][5] = NULL;


	    lab[k].text = (unichar_t *) _("Inherit");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritFlagChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].farray[l][0] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Background Gradient");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    if ( res->boxdata->flags & box_gradient_bg )
		gcd[k].gd.flags |= gg_cb_on;
	    gcd[k].gd.handle_controlevent = GRE_FlagChanged;
	    gcd[k].data = (void *) (intpt) box_gradient_bg;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].farray[l][1] = &gcd[k-1];
	    if ( res->inherits_from==NULL )
		gcd[k-2].gd.flags &= ~gg_enabled;
	    else if ( (res->inherits_from->boxdata->flags&box_gradient_bg) == (res->boxdata->flags&box_gradient_bg) ) {
		gcd[k-2].gd.flags |= gg_cb_on;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }
	    tofree[i].farray[l][2] = GCD_Glue;
	    tofree[i].farray[l][3] = GCD_Glue;
	    tofree[i].farray[l][4] = GCD_Glue;
	    tofree[i].farray[l++][5] = NULL;
	    tofree[i].farray[l][0] = NULL;

	    tofree[i].flagsbox.gd.flags = gg_enabled | gg_visible;
	    tofree[i].flagsbox.gd.u.boxelements = tofree[i].farray[0];
	    tofree[i].flagsbox.creator = GHVBoxCreate;
	    tofree[i].marray[j++] = &tofree[i].flagsbox;

	    l = 0;

/* GT: "I." is an abreviation for "Inherits" */
	    lab[k].text = (unichar_t *) _("I.");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritColChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].carray[l][0] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Normal Text Color:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].carray[l][1] = &gcd[k-1];

	    gcd[k].gd.u.col = res->boxdata->main_foreground;
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_ColorChanged;
	    gcd[k].data = &res->boxdata->main_foreground;
	    gcd[k++].creator = GColorButtonCreate;
	    tofree[i].carray[l][2] = &gcd[k-1];
	    tofree[i].carray[l][3] = GCD_Glue;
	    if ( res->inherits_from==NULL )
		gcd[k-3].gd.flags &= ~gg_enabled;
	    else if ( res->inherits_from->boxdata->main_foreground == res->boxdata->main_foreground ) {
		gcd[k-3].gd.flags |= gg_cb_on;
		gcd[k-2].gd.flags &= ~gg_enabled;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

/* GT: "I." is an abreviation for "Inherits" */
	    lab[k].text = (unichar_t *) _("I.");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritColChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].carray[l][4] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Disabled Text Color:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].carray[l][5] = &gcd[k-1];

	    gcd[k].gd.u.col = res->boxdata->disabled_foreground;
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_ColorChanged;
	    gcd[k].data = &res->boxdata->disabled_foreground;
	    gcd[k++].creator = GColorButtonCreate;
	    tofree[i].carray[l][6] = &gcd[k-1];
	    if ( res->inherits_from==NULL )
		gcd[k-3].gd.flags &= ~gg_enabled;
	    else if ( res->inherits_from->boxdata->disabled_foreground == res->boxdata->disabled_foreground ) {
		gcd[k-3].gd.flags |= gg_cb_on;
		gcd[k-2].gd.flags &= ~gg_enabled;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

	    tofree[i].carray[l][7] = GCD_Glue;
	    tofree[i].carray[l++][8] = NULL;

/* GT: "I." is an abreviation for "Inherits" */
	    lab[k].text = (unichar_t *) _("I.");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritColChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].carray[l][0] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Normal Background:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].carray[l][1] = &gcd[k-1];

	    gcd[k].gd.u.col = res->boxdata->main_background;
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_ColorChanged;
	    gcd[k].data = &res->boxdata->main_background;
	    gcd[k++].creator = GColorButtonCreate;
	    tofree[i].carray[l][2] = &gcd[k-1];
	    tofree[i].carray[l][3] = GCD_Glue;
	    if ( res->inherits_from==NULL )
		gcd[k-3].gd.flags &= ~gg_enabled;
	    else if ( res->inherits_from->boxdata->main_background == res->boxdata->main_background ) {
		gcd[k-3].gd.flags |= gg_cb_on;
		gcd[k-2].gd.flags &= ~gg_enabled;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

/* GT: "I." is an abreviation for "Inherits" */
	    lab[k].text = (unichar_t *) _("I.");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritColChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].carray[l][4] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Disabled Background:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].carray[l][5] = &gcd[k-1];

	    gcd[k].gd.u.col = res->boxdata->disabled_background;
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_ColorChanged;
	    gcd[k].data = &res->boxdata->disabled_background;
	    gcd[k++].creator = GColorButtonCreate;
	    tofree[i].carray[l][6] = &gcd[k-1];
	    if ( res->inherits_from==NULL )
		gcd[k-3].gd.flags &= ~gg_enabled;
	    else if ( res->inherits_from->boxdata->disabled_background == res->boxdata->disabled_background ) {
		gcd[k-3].gd.flags |= gg_cb_on;
		gcd[k-2].gd.flags &= ~gg_enabled;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

	    tofree[i].carray[l][7] = GCD_Glue;
	    tofree[i].carray[l++][8] = NULL;

/* GT: "I." is an abreviation for "Inherits" */
	    lab[k].text = (unichar_t *) _("I.");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritColChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].carray[l][0] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Depressed Background:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].carray[l][1] = &gcd[k-1];

	    gcd[k].gd.u.col = res->boxdata->depressed_background;
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_ColorChanged;
	    gcd[k].data = &res->boxdata->depressed_background;
	    gcd[k++].creator = GColorButtonCreate;
	    tofree[i].carray[l][2] = &gcd[k-1];
	    tofree[i].carray[l][3] = GCD_Glue;
	    if ( res->inherits_from==NULL )
		gcd[k-3].gd.flags &= ~gg_enabled;
	    else if ( res->inherits_from->boxdata->depressed_background == res->boxdata->depressed_background ) {
		gcd[k-3].gd.flags |= gg_cb_on;
		gcd[k-2].gd.flags &= ~gg_enabled;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

/* GT: "I." is an abreviation for "Inherits" */
	    lab[k].text = (unichar_t *) _("I.");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritColChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].carray[l][4] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Background Gradient:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].carray[l][5] = &gcd[k-1];

	    gcd[k].gd.u.col = res->boxdata->gradient_bg_end;
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_ColorChanged;
	    gcd[k].data = &res->boxdata->gradient_bg_end;
	    gcd[k++].creator = GColorButtonCreate;
	    tofree[i].carray[l][6] = &gcd[k-1];
	    if ( res->inherits_from==NULL )
		gcd[k-3].gd.flags &= ~gg_enabled;
	    else if ( res->inherits_from->boxdata->gradient_bg_end == res->boxdata->gradient_bg_end ) {
		gcd[k-3].gd.flags |= gg_cb_on;
		gcd[k-2].gd.flags &= ~gg_enabled;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

	    tofree[i].carray[l][7] = GCD_Glue;
	    tofree[i].carray[l++][8] = NULL;

/* GT: "I." is an abreviation for "Inherits" */
	    lab[k].text = (unichar_t *) _("I.");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritColChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].carray[l][0] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Brightest Border:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].carray[l][1] = &gcd[k-1];

	    gcd[k].gd.u.col = res->boxdata->border_brightest;
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_ColorChanged;
	    gcd[k].data = &res->boxdata->border_brightest;
	    gcd[k++].creator = GColorButtonCreate;
	    tofree[i].carray[l][2] = &gcd[k-1];
	    tofree[i].carray[l][3] = GCD_Glue;
	    if ( res->inherits_from==NULL )
		gcd[k-3].gd.flags &= ~gg_enabled;
	    else if ( res->inherits_from->boxdata->border_brightest == res->boxdata->border_brightest ) {
		gcd[k-3].gd.flags |= gg_cb_on;
		gcd[k-2].gd.flags &= ~gg_enabled;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

/* GT: "I." is an abreviation for "Inherits" */
	    lab[k].text = (unichar_t *) _("I.");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritColChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].carray[l][4] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Brighter Border:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].carray[l][5] = &gcd[k-1];

	    gcd[k].gd.u.col = res->boxdata->border_brighter;
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_ColorChanged;
	    gcd[k].data = &res->boxdata->border_brighter;
	    gcd[k++].creator = GColorButtonCreate;
	    tofree[i].carray[l][6] = &gcd[k-1];
	    if ( res->inherits_from==NULL )
		gcd[k-3].gd.flags &= ~gg_enabled;
	    else if ( res->inherits_from->boxdata->border_brighter == res->boxdata->border_brighter ) {
		gcd[k-3].gd.flags |= gg_cb_on;
		gcd[k-2].gd.flags &= ~gg_enabled;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

	    tofree[i].carray[l][7] = GCD_Glue;
	    tofree[i].carray[l++][8] = NULL;


/* GT: "I." is an abreviation for "Inherits" */
	    lab[k].text = (unichar_t *) _("I.");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritColChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].carray[l][0] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Darker Border:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].carray[l][1] = &gcd[k-1];

	    gcd[k].gd.u.col = res->boxdata->border_darker;
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_ColorChanged;
	    gcd[k].data = &res->boxdata->border_darker;
	    gcd[k++].creator = GColorButtonCreate;
	    tofree[i].carray[l][2] = &gcd[k-1];
	    tofree[i].carray[l][3] = GCD_Glue;
	    if ( res->inherits_from==NULL )
		gcd[k-3].gd.flags &= ~gg_enabled;
	    else if ( res->inherits_from->boxdata->border_darker == res->boxdata->border_darker ) {
		gcd[k-3].gd.flags |= gg_cb_on;
		gcd[k-2].gd.flags &= ~gg_enabled;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

/* GT: "I." is an abreviation for "Inherits" */
	    lab[k].text = (unichar_t *) _("I.");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritColChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].carray[l][4] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Darkest Border:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].carray[l][5] = &gcd[k-1];

	    gcd[k].gd.u.col = res->boxdata->border_darkest;
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_ColorChanged;
	    gcd[k].data = &res->boxdata->border_darkest;
	    gcd[k++].creator = GColorButtonCreate;
	    tofree[i].carray[l][6] = &gcd[k-1];
	    if ( res->inherits_from==NULL )
		gcd[k-3].gd.flags &= ~gg_enabled;
	    else if ( res->inherits_from->boxdata->border_darkest == res->boxdata->border_darkest ) {
		gcd[k-3].gd.flags |= gg_cb_on;
		gcd[k-2].gd.flags &= ~gg_enabled;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

	    tofree[i].carray[l][7] = GCD_Glue;
	    tofree[i].carray[l++][8] = NULL;


/* GT: "I." is an abreviation for "Inherits" */
	    lab[k].text = (unichar_t *) _("I.");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritColChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].carray[l][0] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Inner Border:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].carray[l][1] = &gcd[k-1];

	    gcd[k].gd.u.col = res->boxdata->border_inner;
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_ColorChanged;
	    gcd[k].data = &res->boxdata->border_inner;
	    gcd[k++].creator = GColorButtonCreate;
	    tofree[i].carray[l][2] = &gcd[k-1];
	    tofree[i].carray[l][3] = GCD_Glue;
	    if ( res->inherits_from==NULL )
		gcd[k-3].gd.flags &= ~gg_enabled;
	    else if ( res->inherits_from->boxdata->border_inner == res->boxdata->border_inner ) {
		gcd[k-3].gd.flags |= gg_cb_on;
		gcd[k-2].gd.flags &= ~gg_enabled;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

/* GT: "I." is an abreviation for "Inherits" */
	    lab[k].text = (unichar_t *) _("I.");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritColChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].carray[l][4] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Outer Border:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].carray[l][5] = &gcd[k-1];

	    gcd[k].gd.u.col = res->boxdata->border_outer;
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_ColorChanged;
	    gcd[k].data = &res->boxdata->border_outer;
	    gcd[k++].creator = GColorButtonCreate;
	    tofree[i].carray[l][6] = &gcd[k-1];
	    if ( res->inherits_from==NULL )
		gcd[k-3].gd.flags &= ~gg_enabled;
	    else if ( res->inherits_from->boxdata->border_outer == res->boxdata->border_outer ) {
		gcd[k-3].gd.flags |= gg_cb_on;
		gcd[k-2].gd.flags &= ~gg_enabled;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

	    tofree[i].carray[l][7] = GCD_Glue;
	    tofree[i].carray[l++][8] = NULL;


/* GT: "I." is an abreviation for "Inherits" */
	    lab[k].text = (unichar_t *) _("I.");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritColChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].carray[l][0] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Active Border:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].carray[l][1] = &gcd[k-1];

	    gcd[k].gd.u.col = res->boxdata->active_border;
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_ColorChanged;
	    gcd[k].data = &res->boxdata->active_border;
	    gcd[k++].creator = GColorButtonCreate;
	    tofree[i].carray[l][2] = &gcd[k-1];
	    tofree[i].carray[l][3] = GCD_Glue;
	    if ( res->inherits_from==NULL )
		gcd[k-3].gd.flags &= ~gg_enabled;
	    else if ( res->inherits_from->boxdata->active_border == res->boxdata->active_border ) {
		gcd[k-3].gd.flags |= gg_cb_on;
		gcd[k-2].gd.flags &= ~gg_enabled;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

	    tofree[i].carray[l][4] = GCD_Glue;
	    tofree[i].carray[l][5] = GCD_Glue;
	    tofree[i].carray[l][6] = GCD_Glue;
	    tofree[i].carray[l][7] = GCD_Glue;
	    tofree[i].carray[l++][8] = NULL;

/* GT: "I." is an abreviation for "Inherits" */
	    lab[k].text = (unichar_t *) _("I.");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritListChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].carray[l][0] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Border Type:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].carray[l][1] = &gcd[k-1];

	    gcd[k].gd.u.list = bordertype;
	    gcd[k].gd.label = &bordertype[res->boxdata->border_type];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = tofree[i].btcid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_ListChanged;
	    gcd[k].data = &res->boxdata->border_type;
	    gcd[k++].creator = GListButtonCreate;
	    tofree[i].carray[l][2] = &gcd[k-1];
	    tofree[i].carray[l][3] = GCD_ColSpan;
	    if ( res->inherits_from==NULL )
		gcd[k-3].gd.flags &= ~gg_enabled;
	    else if ( res->inherits_from->boxdata->border_type == res->boxdata->border_type ) {
		gcd[k-3].gd.flags |= gg_cb_on;
		gcd[k-2].gd.flags &= ~gg_enabled;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

/* GT: "I." is an abreviation for "Inherits" */
	    lab[k].text = (unichar_t *) _("I.");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritListChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].carray[l][4] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Border Shape:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].carray[l][5] = &gcd[k-1];

	    gcd[k].gd.u.list = bordershape;
	    gcd[k].gd.label = &bordershape[res->boxdata->border_shape];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_ListChanged;
	    gcd[k].data = &res->boxdata->border_shape;
	    gcd[k++].creator = GListButtonCreate;
	    tofree[i].carray[l][6] = &gcd[k-1];
	    if ( res->inherits_from==NULL )
		gcd[k-3].gd.flags &= ~gg_enabled;
	    else if ( res->inherits_from->boxdata->border_shape == res->boxdata->border_shape ) {
		gcd[k-3].gd.flags |= gg_cb_on;
		gcd[k-2].gd.flags &= ~gg_enabled;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

	    tofree[i].carray[l][7] = GCD_Glue;
	    tofree[i].carray[l++][8] = NULL;

/* GT: "I." is an abreviation for "Inherits" */
	    lab[k].text = (unichar_t *) _("I.");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritByteChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].carray[l][0] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Border Width:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].carray[l][1] = &gcd[k-1];

	    sprintf( tofree[i].bw, "%d", res->boxdata->border_width );
	    lab[k].text = (unichar_t *) tofree[i].bw;
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.pos.width = 50;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_ByteChanged;
	    gcd[k].data = &res->boxdata->border_width;
	    gcd[k++].creator = GNumericFieldCreate;
	    tofree[i].carray[l][2] = &gcd[k-1];
	    tofree[i].carray[l][3] = GCD_ColSpan;
	    if ( res->inherits_from==NULL )
		gcd[k-3].gd.flags &= ~gg_enabled;
	    else if ( res->inherits_from->boxdata->border_width == res->boxdata->border_width ) {
		gcd[k-3].gd.flags |= gg_cb_on;
		gcd[k-2].gd.flags &= ~gg_enabled;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

/* GT: "I." is an abreviation for "Inherits" */
	    lab[k].text = (unichar_t *) _("I.");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritByteChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].carray[l][4] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Padding:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].carray[l][5] = &gcd[k-1];

	    sprintf( tofree[i].padding, "%d", res->boxdata->padding );
	    lab[k].text = (unichar_t *) tofree[i].padding;
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.pos.width = 50;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_ByteChanged;
	    gcd[k].data = &res->boxdata->padding;
	    gcd[k++].creator = GNumericFieldCreate;
	    tofree[i].carray[l][6] = &gcd[k-1];
	    if ( res->inherits_from==NULL )
		gcd[k-3].gd.flags &= ~gg_enabled;
	    else if ( res->inherits_from->boxdata->padding == res->boxdata->padding ) {
		gcd[k-3].gd.flags |= gg_cb_on;
		gcd[k-2].gd.flags &= ~gg_enabled;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

	    tofree[i].carray[l][7] = GCD_Glue;
	    tofree[i].carray[l++][8] = NULL;

/* GT: "I." is an abreviation for "Inherits" */
	    lab[k].text = (unichar_t *) _("I.");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritByteChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].carray[l][0] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Radius:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].carray[l][1] = &gcd[k-1];

	    sprintf( tofree[i].rr, "%d", res->boxdata->rr_radius );
	    lab[k].text = (unichar_t *) tofree[i].rr;
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.pos.width = 50;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_ByteChanged;
	    gcd[k].data = &res->boxdata->rr_radius;
	    gcd[k++].creator = GNumericFieldCreate;
	    tofree[i].carray[l][2] = &gcd[k-1];
	    tofree[i].carray[l][3] = GCD_ColSpan;
	    if ( res->inherits_from==NULL )
		gcd[k-3].gd.flags &= ~gg_enabled;
	    else if ( res->inherits_from->boxdata->rr_radius == res->boxdata->rr_radius ) {
		gcd[k-3].gd.flags |= gg_cb_on;
		gcd[k-2].gd.flags &= ~gg_enabled;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }

	    tofree[i].carray[l][4] = GCD_Glue;
	    tofree[i].carray[l][5] = GCD_Glue;
	    tofree[i].carray[l][6] = GCD_Glue;
	    tofree[i].carray[l][7] = GCD_Glue;
	    tofree[i].carray[l++][8] = NULL;
	    tofree[i].carray[l][0] = NULL;

	    tofree[i].colbox.gd.flags = gg_enabled | gg_visible;
	    tofree[i].colbox.gd.u.boxelements = tofree[i].carray[0];
	    tofree[i].colbox.creator = GHVBoxCreate;
	    tofree[i].marray[j++] = &tofree[i].colbox;
	}

	if ( res->font!=NULL ) {
	    tofree[i].fontname = GFontSpec2String( *res->font );

	    lab[k].text = (unichar_t *) _("I.");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
	    gcd[k].gd.popup_msg = (unichar_t *) _("Inherits for same field in parent");
	    gcd[k].gd.cid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_InheritFontChange;
	    gcd[k++].creator = GCheckBoxCreate;
	    tofree[i].fontarray[0] = &gcd[k-1];

	    lab[k].text = (unichar_t *) _("Font:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = ++cid;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].fontarray[1] = &gcd[k-1];

	    lab[k].text = (unichar_t *) tofree[i].fontname;
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k].gd.cid = tofree[i].fontcid = ++cid;
	    gcd[k].gd.handle_controlevent = GRE_FontChanged;
	    gcd[k].data = res->font;
	    gcd[k++].creator = GTextFieldCreate;
	    tofree[i].fontarray[2] = &gcd[k-1];
	    if ( res->inherits_from==NULL )
		gcd[k-3].gd.flags &= ~gg_enabled;
	    else if ( *res->inherits_from->font == *res->font ) {
		gcd[k-3].gd.flags |= gg_cb_on;
		gcd[k-2].gd.flags &= ~gg_enabled;
		gcd[k-1].gd.flags &= ~gg_enabled;
	    }
	    tofree[i].fontarray[3] = GCD_Glue;
	    tofree[i].fontarray[4] = NULL;

	    tofree[i].fontbox.gd.flags = gg_enabled | gg_visible;
	    tofree[i].fontbox.gd.u.boxelements = tofree[i].fontarray;
	    tofree[i].fontbox.creator = GHBoxCreate;
	    tofree[i].marray[j++] = &tofree[i].fontbox;
	}
	if ( res->seealso1 != NULL ) {
	    lab[k].text = (unichar_t *) _("See also:");
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled;
	    gcd[k++].creator = GLabelCreate;
	    tofree[i].saarray[0] = &gcd[k-1];

	    lab[k].text = (unichar_t *) S_(res->seealso1->name);
	    lab[k].text_is_1byte = true;
	    gcd[k].gd.label = &lab[k];
	    gcd[k].gd.flags = gg_visible|gg_enabled|gg_dontcopybox;
	    gcd[k].gd.box = &small_blue_box;
	    gcd[k].data = res->seealso1;
	    gcd[k].gd.handle_controlevent = GRE_ChangePane;
	    gcd[k++].creator = GButtonCreate;
	    tofree[i].saarray[1] = &gcd[k-1];

	    if ( res->seealso2!=NULL ) {
		lab[k].text = (unichar_t *) S_(res->seealso2->name);
		lab[k].text_is_1byte = true;
		gcd[k].gd.label = &lab[k];
		gcd[k].gd.flags = gg_visible|gg_enabled|gg_dontcopybox;
		gcd[k].gd.box = &small_blue_box;
		gcd[k].data = res->seealso2;
		gcd[k].gd.handle_controlevent = GRE_ChangePane;
		gcd[k++].creator = GButtonCreate;
		tofree[i].saarray[2] = &gcd[k-1];
	    } else
		tofree[i].saarray[2] = GCD_Glue;

	    tofree[i].saarray[3] = GCD_Glue; tofree[i].saarray[4] = NULL;
	    tofree[i].sabox.gd.flags = gg_visible|gg_enabled;
	    tofree[i].sabox.gd.u.boxelements = tofree[i].saarray;
	    tofree[i].sabox.creator = GHBoxCreate;
	    tofree[i].marray[j++] = &tofree[i].sabox;
	}

	if ( res->extras!=NULL ) {
	    int hl=-1, base=3;
	    for ( l=0, extras = res->extras ; extras->name!=NULL; ++extras, ++l ) {
		if ( base==3 ) {
		    base=0;
		    ++hl;
		} else
		    base=3;
		if ( base==3 ) {
		    tofree[i].earray[hl][6] = GCD_Glue;
		    tofree[i].earray[hl][7] = NULL;
		}
		switch ( extras->type ) {
		  case rt_bool:
		    extras->orig.ival = *(int *) (extras->val);
		    lab[k].text = (unichar_t *) S_(extras->name);
		    lab[k].text_is_1byte = true;
		    gcd[k].gd.label = &lab[k];
		    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
		    if ( extras->orig.ival )
			gcd[k].gd.flags |= gg_cb_on;
		    if ( extras->popup!=NULL )
			gcd[k].gd.popup_msg = (unichar_t *) _(extras->popup);
		    gcd[k].gd.cid = extras->cid = ++cid;
		    gcd[k].data = extras->val;
		    gcd[k].gd.handle_controlevent = GRE_BoolChanged;
		    gcd[k++].creator = GCheckBoxCreate;
		    tofree[i].earray[hl][base] = &gcd[k-1];
		    tofree[i].earray[hl][base+1] = GCD_ColSpan;
		    tofree[i].earray[hl][base+2] = GCD_ColSpan;
		  break;
		  case rt_int:
		    extras->orig.ival = *(int *) (extras->val);
		    lab[k].text = (unichar_t *) S_(extras->name);
		    lab[k].text_is_1byte = true;
		    gcd[k].gd.label = &lab[k];
		    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
		    if ( extras->popup!=NULL )
			gcd[k].gd.popup_msg = (unichar_t *) _(extras->popup);
		    gcd[k++].creator = GLabelCreate;
		    tofree[i].earray[hl][base] = &gcd[k-1];

		    tofree[i].extradefs[l] = galloc(20);
		    sprintf( tofree[i].extradefs[l], "%d", extras->orig.ival );
		    lab[k].text = (unichar_t *) tofree[i].extradefs[l];
		    lab[k].text_is_1byte = true;
		    gcd[k].gd.pos.width = 60;
		    gcd[k].gd.label = &lab[k];
		    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
		    if ( extras->popup!=NULL )
			gcd[k].gd.popup_msg = (unichar_t *) S_(extras->popup);
		    gcd[k].gd.cid = extras->cid = ++cid;
		    gcd[k].data = extras->val;
		    gcd[k].gd.handle_controlevent = GRE_IntChanged;
		    gcd[k++].creator = GNumericFieldCreate;
		    tofree[i].earray[hl][base+1] = &gcd[k-1];
		    tofree[i].earray[hl][base+2] = GCD_ColSpan;
		  break;
		  case rt_double:
		    extras->orig.dval = *(double *) (extras->val);
		    lab[k].text = (unichar_t *) S_(extras->name);
		    lab[k].text_is_1byte = true;
		    gcd[k].gd.label = &lab[k];
		    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
		    if ( extras->popup!=NULL )
			gcd[k].gd.popup_msg = (unichar_t *) _(extras->popup);
		    gcd[k++].creator = GLabelCreate;
		    tofree[i].earray[hl][base] = &gcd[k-1];

		    tofree[i].extradefs[l] = galloc(40);
		    sprintf( tofree[i].extradefs[l], "%g", extras->orig.dval );
		    lab[k].text = (unichar_t *) tofree[i].extradefs[l];
		    lab[k].text_is_1byte = true;
		    gcd[k].gd.label = &lab[k];
		    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
		    if ( extras->popup!=NULL )
			gcd[k].gd.popup_msg = (unichar_t *) _(extras->popup);
		    gcd[k].data = extras->val;
		    gcd[k].gd.cid = extras->cid = ++cid;
		    gcd[k].gd.handle_controlevent = GRE_DoubleChanged;
		    gcd[k++].creator = GTextFieldCreate;
		    tofree[i].earray[hl][base+1] = &gcd[k-1];
		    tofree[i].earray[hl][base+2] = GCD_ColSpan;
		  break;
		  case rt_coloralpha:
		  case rt_color:
		    extras->orig.ival = *(int *) (extras->val);
		    lab[k].text = (unichar_t *) S_(extras->name);
		    lab[k].text_is_1byte = true;
		    gcd[k].gd.label = &lab[k];
		    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
		    if ( extras->popup!=NULL )
			gcd[k].gd.popup_msg = (unichar_t *) _(extras->popup);
		    gcd[k++].creator = GLabelCreate;
		    tofree[i].earray[hl][base] = &gcd[k-1];

		    gcd[k].gd.u.col = extras->orig.ival;
		    if ( (extras->orig.ival&0xff000000)==0 && extras->type==rt_coloralpha )
			gcd[k].gd.u.col |= 0xfe000000;
		    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
		    if ( extras->popup!=NULL )
			gcd[k].gd.popup_msg = (unichar_t *) _(extras->popup);
		    gcd[k].gd.cid = extras->cid = ++cid;
		    gcd[k].data = extras->val;
		    gcd[k].gd.handle_controlevent = GRE_ExtraColorChanged;
		    gcd[k++].creator = GColorButtonCreate;
		    tofree[i].earray[hl][base+1] = &gcd[k-1];
		    tofree[i].earray[hl][base+2] = GCD_ColSpan;
		  break;
		  case rt_stringlong:
		    if ( base==3 ) {
			tofree[i].earray[hl][3] = GCD_Glue;
			tofree[i].earray[hl][4] = GCD_Glue;
			tofree[i].earray[hl][5] = GCD_Glue;
			tofree[i].earray[hl][6] = GCD_Glue;
			tofree[i].earray[hl][7] = NULL;
			base=0; ++hl;
		    }
		  /* Fall through */
		  case rt_string:
		    extras->orig.sval = *(char **) (extras->val);
		    lab[k].text = (unichar_t *) S_(extras->name);
		    lab[k].text_is_1byte = true;
		    gcd[k].gd.label = &lab[k];
		    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
		    if ( extras->popup!=NULL )
			gcd[k].gd.popup_msg = (unichar_t *) _(extras->popup);
		    gcd[k++].creator = GLabelCreate;
		    tofree[i].earray[hl][base] = &gcd[k-1];

		    if ( extras->orig.sval != NULL ) {
			lab[k].text = (unichar_t *) extras->orig.sval;
			lab[k].text_is_1byte = true;
			gcd[k].gd.label = &lab[k];
		    }
		    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
		    if ( extras->popup!=NULL )
			gcd[k].gd.popup_msg = (unichar_t *) _(extras->popup);
		    gcd[k].data = extras->val;
		    gcd[k].gd.cid = extras->cid = ++cid;
		    gcd[k].gd.handle_controlevent = GRE_StringChanged;
		    gcd[k++].creator = GTextFieldCreate;
		    tofree[i].earray[hl][base+1] = &gcd[k-1];
		    tofree[i].earray[hl][base+2] = GCD_ColSpan;
		    if ( extras->type==rt_stringlong ) {
			tofree[i].earray[hl][3] = GCD_ColSpan;
			tofree[i].earray[hl][4] = GCD_ColSpan;
			tofree[i].earray[hl][5] = GCD_ColSpan;
			tofree[i].earray[hl][6] = GCD_ColSpan;
			base=3;
		    }
		  break;
		  case rt_font:
		    if ( base==3 ) {
			tofree[i].earray[hl][3] = GCD_Glue;
			tofree[i].earray[hl][4] = GCD_Glue;
			tofree[i].earray[hl][5] = GCD_Glue;
			tofree[i].earray[hl][6] = GCD_Glue;
			tofree[i].earray[hl][7] = NULL;
			base=0; ++hl;
		    }
		    extras->orig.fontval = *(GFont **) (extras->val);
		    lab[k].text = (unichar_t *) S_(extras->name);
		    lab[k].text_is_1byte = true;
		    gcd[k].gd.label = &lab[k];
		    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
		    if ( extras->popup!=NULL )
			gcd[k].gd.popup_msg = (unichar_t *) _(extras->popup);
		    gcd[k++].creator = GLabelCreate;
		    tofree[i].earray[hl][base] = &gcd[k-1];

		    if ( extras->orig.fontval != NULL ) {
			tofree[i].extradefs[l] = GFontSpec2String( extras->orig.fontval );
			lab[k].text = (unichar_t *) tofree[i].extradefs[l];
			lab[k].text_is_1byte = true;
			gcd[k].gd.label = &lab[k];
		    }
		    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
		    if ( extras->popup!=NULL )
			gcd[k].gd.popup_msg = (unichar_t *) _(extras->popup);
		    gcd[k].data = extras->val;
		    gcd[k].gd.cid = extras->cid = ++cid;
		    gcd[k].gd.handle_controlevent = GRE_ExtraFontChanged;
		    gcd[k++].creator = GTextFieldCreate;
		    tofree[i].earray[hl][base+1] = &gcd[k-1];
		    tofree[i].earray[hl][base+2] = GCD_ColSpan;

		    /* Font names (with potentially many families) tend to be long */
		    tofree[i].earray[hl][3] = GCD_ColSpan;
		    tofree[i].earray[hl][4] = GCD_ColSpan;
		    tofree[i].earray[hl][5] = GCD_ColSpan;
		    tofree[i].earray[hl][6] = GCD_ColSpan;
		    base=3;
		  break;
		  case rt_image: {
		    GResImage *ri = *(GResImage **) (extras->val);
		    extras->orig.sval = copy( ri==NULL ? NULL : ri->filename );
		    lab[k].text = (unichar_t *) S_(extras->name);
		    lab[k].text_is_1byte = true;
		    gcd[k].gd.label = &lab[k];
		    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
		    if ( extras->popup!=NULL )
			gcd[k].gd.popup_msg = (unichar_t *) _(extras->popup);
		    gcd[k++].creator = GLabelCreate;
		    tofree[i].earray[hl][base] = &gcd[k-1];

		    if ( ri != NULL && ri->image!=NULL ) {
			lab[k].text = (unichar_t *) "...";
			lab[k].image = ri->image;
		    } else
			lab[k].text = (unichar_t *) "? ...";
		    lab[k].text_is_1byte = true;
		    gcd[k].gd.label = &lab[k];
		    gcd[k].gd.flags = gg_visible|gg_enabled|gg_utf8_popup;
		    if ( extras->popup!=NULL )
			gcd[k].gd.popup_msg = (unichar_t *) _(extras->popup);
		    gcd[k].data = extras->val;
		    gcd[k].gd.cid = extras->cid = ++cid;
		    gcd[k].gd.handle_controlevent = GRE_ImageChanged;
		    gcd[k++].creator = GButtonCreate;
		    tofree[i].earray[hl][base+1] = &gcd[k-1];
		    tofree[i].earray[hl][base+2] = GCD_ColSpan;
		  } break;
		}
	    }
	    if ( base==0 ) {
		tofree[i].earray[hl][3] = GCD_Glue;
		tofree[i].earray[hl][4] = GCD_Glue;
		tofree[i].earray[hl][5] = GCD_Glue;
		tofree[i].earray[hl][6] = GCD_Glue;
		tofree[i].earray[hl][7] = NULL;
	    }
	    tofree[i].earray[hl+1][0] = NULL;
	    tofree[i].extrabox.gd.flags = gg_visible|gg_enabled;
	    tofree[i].extrabox.gd.u.boxelements = tofree[i].earray[0];
	    tofree[i].extrabox.creator = GHVBoxCreate;
	    tofree[i].marray[j++] = &tofree[i].extrabox;
	}
	tofree[i].marray[j++] = GCD_Glue;
	tofree[i].marray[j++] = NULL;
	tofree[i].mainbox[0].gd.flags = gg_visible|gg_enabled;
	tofree[i].mainbox[0].gd.u.boxelements = tofree[i].marray;
	tofree[i].mainbox[0].creator = GVBoxCreate;
	panes[i].text = (unichar_t *) S_(res->name);
	panes[i].text_is_1byte = true;
	panes[i].gcd = &tofree[i].mainbox[0];
	for ( parent=res; parent!=NULL; parent=parent->inherits_from, ++panes[i].nesting );
	if ( k>cnt )
	    GDrawIError( "ResEdit Miscounted, expect a crash" );
    }

    memset(topgcd,0,sizeof(topgcd));
    memset(topbox,0,sizeof(topbox));
    memset(toplab,0,sizeof(toplab));

    topgcd[0].gd.flags = gg_visible|gg_enabled|gg_tabset_vert|gg_tabset_scroll;
    topgcd[0].gd.u.tabs = panes;
    topgcd[0].creator = GTabSetCreate;

    toplab[1].text = (unichar_t *) _("_OK");
    toplab[1].text_is_1byte = true;
    toplab[1].text_in_resource = true;
    topgcd[1].gd.label = &toplab[1];
    topgcd[1].gd.flags = gg_visible|gg_enabled | gg_but_default;
    topgcd[1].gd.handle_controlevent = GRE_OK;
    topgcd[1].creator = GButtonCreate;

    toplab[2].text = (unichar_t *) _("_Save As...");
    toplab[2].text_is_1byte = true;
    toplab[2].text_in_resource = true;
    topgcd[2].gd.label = &toplab[2];
    topgcd[2].gd.flags = gg_visible|gg_enabled;
    topgcd[2].gd.handle_controlevent = GRE_Save;
    topgcd[2].creator = GButtonCreate;

    toplab[3].text = (unichar_t *) _("_Cancel");
    toplab[3].text_is_1byte = true;
    toplab[3].text_in_resource = true;
    topgcd[3].gd.label = &toplab[3];
    topgcd[3].gd.flags = gg_visible|gg_enabled | gg_but_cancel;
    topgcd[3].gd.handle_controlevent = GRE_Cancel;
    topgcd[3].creator = GButtonCreate;

    barray[0] = GCD_Glue; barray[1] = &topgcd[1]; barray[2] = GCD_Glue;
    barray[3] = GCD_Glue; barray[4] = &topgcd[2]; barray[5] = GCD_Glue;
    barray[6] = GCD_Glue; barray[7] = &topgcd[3]; barray[8] = GCD_Glue;
    barray[9] = NULL;

    topbox[2].gd.flags = gg_visible | gg_enabled;
    topbox[2].gd.u.boxelements = barray;
    topbox[2].creator = GHBoxCreate;

    tarray[0][0] = &topgcd[0]; tarray[0][1] = NULL;
    tarray[1][0] = &topbox[2]; tarray[1][1] = NULL;
    tarray[2][0] = NULL;

    topbox[0].gd.pos.x = topbox[0].gd.pos.y = 2;
    topbox[0].gd.flags = gg_visible | gg_enabled;
    topbox[0].gd.u.boxelements = tarray[0];
    topbox[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,topbox);
    gre.tabset = topgcd[0].ret;

    GHVBoxSetExpandableRow(topbox[0].ret,0);
    GHVBoxSetExpandableCol(topbox[2].ret,gb_expandgluesame);

    for ( res=all, i=0; res!=NULL; res=res->next, ++i ) {
	GHVBoxSetExpandableRow(tofree[i].mainbox[0].ret,gb_expandglue);
	if ( res->examples!=NULL &&
		( res->examples->creator==GHBoxCreate ||
		  res->examples->creator==GVBoxCreate ||
		  res->examples->creator==GHVBoxCreate )) {
	    GHVBoxSetExpandableCol(res->examples->ret,gb_expandglue);
	    GHVBoxSetExpandableRow(res->examples->ret,gb_expandglue);
	}
	if ( tofree[i].ibox.ret!=NULL )
	    GHVBoxSetExpandableCol(tofree[i].ibox.ret,gb_expandglue);
	if ( tofree[i].flagsbox.ret!=NULL )
	    GHVBoxSetExpandableCol(tofree[i].flagsbox.ret,gb_expandglue);
	if ( tofree[i].colbox.ret!=NULL )
	    GHVBoxSetExpandableCol(tofree[i].colbox.ret,gb_expandglue);
	if ( tofree[i].extrabox.ret!=NULL )
	    GHVBoxSetExpandableCol(tofree[i].extrabox.ret,gb_expandglue);
	if ( tofree[i].sabox.ret!=NULL )
	    GHVBoxSetExpandableCol(tofree[i].sabox.ret,gb_expandglue);
	if ( tofree[i].fontbox.ret!=NULL )
	    GHVBoxSetExpandableCol(tofree[i].fontbox.ret,2);
	if ( res->boxdata!=NULL ) {
	    GGadgetSelectOneListItem(GWidgetGetControl(gw,tofree[i].btcid),res->boxdata->border_type);
	    GGadgetSelectOneListItem(GWidgetGetControl(gw,tofree[i].btcid+3),res->boxdata->border_shape);
	}
    }

    GHVBoxFitWindowCentered(topbox[0].ret);
    GDrawSetVisible(gw,true);
    
    while ( !gre.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);

    for ( res=all, i=0; res!=NULL; res=res->next, ++i ) {
	free(tofree[i].gcd);
	free(tofree[i].lab);
	free(tofree[i].earray);
	free(tofree[i].fontname);
	if ( res->extras!=NULL ) {
	    for ( l=0, extras = res->extras ; extras->name!=NULL; ++extras, ++l ) {
		free( tofree[i].extradefs[l]);
	    }
	}
	free(tofree[i].extradefs);
    }
    free( tofree );
    free( panes );
}

static double _GDraw_Width_cm, _GDraw_Width_Inches;
static Color _GDraw_fg, _GDraw_bg;
static struct resed gdrawcm_re[] = {
    {N_("Default Background"), "Background", rt_color, &_GDraw_bg, N_("Default background color for windows"), NULL, { 0 }, 0, 0 },
    {N_("Default Foreground"), "Foreground", rt_color, &_GDraw_fg, N_("Default foreground color for windows"), NULL, { 0 }, 0, 0 },
    {N_("Screen Width in Centimeters"), "ScreenWidthCentimeters", rt_double, &_GDraw_Width_cm, N_("Physical screen width, measured in centimeters\nFor this to take effect you must save the resource data (press the [Save] button)\nand restart fontforge"), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};
static struct resed gdrawin_re[] = {
    {N_("Default Background"), "Background", rt_color, &_GDraw_bg, N_("Default background color for windows"), NULL, { 0 }, 0, 0 },
    {N_("Default Foreground"), "Foreground", rt_color, &_GDraw_fg, N_("Default foreground color for windows"), NULL, { 0 }, 0, 0 },
    {N_("Screen Width in Inches"), "ScreenWidthInches", rt_double, &_GDraw_Width_Inches, N_("Physical screen width, measured in inches\nFor this to take effect you must save the resource data (press the [Save] button)\nand restart fontforge"), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};
static GResInfo gdraw_ri = {
    NULL, NULL,NULL, NULL,
    NULL,
    NULL,
    NULL,
    gdrawcm_re,
    N_("GDraw"),
    N_("General facts about the windowing system"),
    "",
    "Gdraw",
    false,
    0,
    NULL,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};

static int refresh_eh(GWindow cover,GEvent *event) {
    if ( event->type==et_expose )
	GDrawDestroyWindow(cover);
return( true );
}
    
void GResEdit(GResInfo *additional,const char *def_res_file,void (*change_res_filename)(const char *)) {
    GResInfo *re_end, *re;
    static int initted = false;
    char *oldimagepath;
    extern char *_GGadget_ImagePath;

    if ( !initted ) {
	initted = true;
	_GDraw_Width_Inches = screen_display->groot->pos.width / (double) screen_display->res;
	_GDraw_Width_cm = _GDraw_Width_Inches * 2.54;
	_GDraw_bg = GDrawGetDefaultBackground(NULL);
	_GDraw_fg = GDrawGetDefaultForeground(NULL);
	if ( getenv("LC_MESSAGES")!=NULL ) {
	    if ( strstr(getenv("LC_MESSAGES"),"_US")!=NULL )
		gdraw_ri.extras = gdrawin_re;
	} else if ( getenv("LANG")!=NULL ) {
	    if ( strstr(getenv("LANG"),"_US")!=NULL )
		gdraw_ri.extras = gdrawin_re;
	}
	gdraw_ri.next = _GProgressRIHead();
	for ( re_end = _GProgressRIHead(); re_end->next!=NULL; re_end = re_end->next );
	re_end->next = _GGadgetRIHead();
	for ( re_end = _GGadgetRIHead(); re_end->next!=NULL; re_end = re_end->next );
	re_end->next = _GButtonRIHead();
	for ( re_end = _GButtonRIHead(); re_end->next!=NULL; re_end = re_end->next );
	re_end->next = _GRadioRIHead();
	for ( re_end = _GRadioRIHead(); re_end->next!=NULL; re_end = re_end->next );
	re_end->next = _GTextFieldRIHead();
	for ( re_end = _GTextFieldRIHead(); re_end->next!=NULL; re_end = re_end->next );
	re_end->next = _GListRIHead();
	for ( re_end = _GListRIHead(); re_end->next!=NULL; re_end = re_end->next );
	re_end->next = _GScrollBarRIHead();
	for ( re_end = _GScrollBarRIHead(); re_end->next!=NULL; re_end = re_end->next );
	re_end->next = _GLineRIHead();
	for ( re_end = _GLineRIHead(); re_end->next!=NULL; re_end = re_end->next );
	re_end->next = _GHVBoxRIHead();
	for ( re_end = _GHVBoxRIHead(); re_end->next!=NULL; re_end = re_end->next );
	re_end->next = _GMenuRIHead();
	for ( re_end = _GMenuRIHead(); re_end->next!=NULL; re_end = re_end->next );
	re_end->next = _GMatrixEditRIHead();
	for ( re_end = _GMatrixEditRIHead(); re_end->next!=NULL; re_end = re_end->next );
	re_end->next = _GDrawableRIHead();
	for ( re_end = _GDrawableRIHead(); re_end->next!=NULL; re_end = re_end->next );
	re_end->next = _GTabSetRIHead();
	for ( re_end = _GTabSetRIHead(); re_end->next!=NULL; re_end = re_end->next );
    }
    if ( additional!=NULL ) {
	for ( re_end=additional; re_end->next!=NULL; re_end = re_end->next );
	re_end->next = &gdraw_ri;
    } else {
	additional = &gdraw_ri;
	re_end = NULL;
    }
    oldimagepath = copy( _GGadget_ImagePath );
    GResEditDlg(additional,def_res_file,change_res_filename);
    if (( oldimagepath!=NULL && _GGadget_ImagePath==NULL ) ||
	    ( oldimagepath==NULL && _GGadget_ImagePath!=NULL ) ||
	    ( oldimagepath!=NULL && _GGadget_ImagePath!=NULL &&
		    strcmp(oldimagepath,_GGadget_ImagePath)!=0 )) {
	char *new = _GGadget_ImagePath;
	_GGadget_ImagePath = oldimagepath;
	GGadgetSetImagePath(new);
    } else
	free( oldimagepath );
    for ( re=additional; re!=NULL; re=re->next ) {
	if ( (re->override_mask&omf_refresh) && re->refresh!=NULL )
	    (re->refresh)();
    }
    if ( re_end!=NULL )
	re_end->next = NULL;

    /* Now create a window which covers the entire screen, and then destroy it*/
    /*  point is to force a refresh on all our windows (it'll refresh everyone's*/
    /*  else windows too, but oh well */
    {
	GWindowAttrs wattrs;
	GWindow root = GDrawGetRoot(screen_display);
	GRect screen;
	static GWindow gw;

	GDrawGetSize(root,&screen);
	wattrs.mask = wam_events|wam_nodecor|wam_positioned|wam_bordwidth|wam_backcol;
	wattrs.event_masks = -1;
	wattrs.nodecoration = true;
	wattrs.positioned = true;
	wattrs.border_width = 0;
	wattrs.background_color = COLOR_UNKNOWN;
	gw = GDrawCreateTopWindow(screen_display,&screen,refresh_eh,&gw,&wattrs);
	GDrawSetVisible(gw,true);
    }
}
    
void GResEditFind( struct resed *resed, char *prefix) {
    int i;
    GResStruct *info;

    for ( i=0; resed[i].name!=NULL; ++i );

    info = gcalloc(i+1,sizeof(GResStruct));
    for ( i=0; resed[i].name!=NULL; ++i ) {
	info[i].resname = resed[i].resname;
	info[i].type = resed[i].type;
	if ( info[i].type==rt_stringlong )
	    info[i].type = rt_string;
	else if ( info[i].type==rt_coloralpha )
	    info[i].type = rt_color;
	info[i].val = resed[i].val;
	info[i].cvt = resed[i].cvt;
	if ( info[i].type==rt_font ) {
	    info[i].type = rt_string;
	    info[i].cvt = GResource_font_cvt;
	}
    }
    GResourceFind(info,prefix);
    for ( i=0; resed[i].name!=NULL; ++i )
	resed[i].found = info[i].found;
    free(info);
}
