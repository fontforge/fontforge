/* Copyright (C) 2001 by George Williams */
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
#include "ttfmodui.h"
#include <ustring.h>
#include <utype.h>

typedef struct os2view /* : tableview */ {
    Table *table;
    GWindow gw, v;
    struct tableviewfuncs *virtuals;
    TtfFont *font;		/* for the encoding currently used */
    struct ttfview *owner;
/* os2 specials */
} OS2View;

static int os2_processdata(TableView *tv) {
    OS2View *mv = (OS2View *) tv;
    /* Do changes!!! */
return( true );
}

static int os2_close(TableView *tv) {
    if ( os2_processdata(tv)) {
	GDrawDestroyWindow(tv->gw);
return( true );
    }
return( false );
}

static struct tableviewfuncs os2funcs = { os2_close, os2_processdata };

static int OS2_Cancel(GGadget *g, GEvent *e) {
    GWindow gw;
    OS2View *mv;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	mv = GDrawGetUserData(gw);
	os2_close((TableView *) mv);
    }
return( true );
}

static int OS2_OK(GGadget *g, GEvent *e) {
    GWindow gw;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	GDrawDestroyWindow(gw);
    }
return( true );
}

static int os2_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	OS2View *mv = GDrawGetUserData(gw);
	GDrawDestroyWindow(mv->gw);
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

void os2CreateEditor(Table *tab,TtfView *tfv) {
    OS2View *mv = gcalloc(1,sizeof(OS2View));
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[34];
    GTextInfo label[34];
    static unichar_t title[] = { 'O', 'S', '/', '2', '\0' };
    char version[20], glyphs[8], points[8], contours[8], cpoints[8],
	ccontours[8], zones[8], tpoints[8], storage[8], fdefs[8], idefs[8],
	sels[8], soi[8], cels[8], cdep[8];

    mv->table = tab;
    mv->virtuals = &os2funcs;
    mv->owner = tfv;
    mv->font = tfv->ttf->fonts[tfv->selectedfont];
    tab->tv = (TableView *) mv;

    TableFillup(tab);
    
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = title;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,270);
    pos.height = GDrawPointsToPixels(NULL,267);
    mv->gw = gw = GDrawCreateTopWindow(NULL,&pos,os2_e_h,mv,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
