/* Copyright (C) 2001-2002 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derfved from this software without specific prior written permission.

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
#include <gkeysym.h>
#include <ustring.h>
#include <utype.h>

#define CHAR_PAD	1

extern int _GScrollBar_Width;

#define MID_Revert	2702
#define MID_Recent	2703
#define MID_Cut		2101
#define MID_Copy	2102
#define MID_Paste	2103
#define MID_SelAll	2106

static int font_processdata(TableView *tv) {

    if ( !tv->table->changed ) {
	tv->table->changed = true;
	tv->table->td_changed = true;
	GDrawRequestExpose(tv->owner->v,NULL,false);
    }
return( true );
}

static int font_close(TableView *tv) {
    FontView *fv = (FontView *) tv;
    ConicFont *cf = fv->cf;
    CharView *cv, *next;
    int i;

    if ( fv->destroyed )
return( true );
    for ( i=0; i<cf->glyph_cnt; ++i ) {
	for ( cv=cf->chars[i]->views; cv!=NULL; cv = next ) {
	    next = cv->next;
	    CVClose(cv);
	}
    }
#if 0
    static int yesnocancel[] = { _STR_Yes, _STR_No, _STR_Cancel, 0 };
    static int closecancel[] = { _STR_Close, _STR_Cancel, 0 };
    int copyover = true;
    int16 *edits;

    if ( ret ) {
	if ( ((FontView *) tv)->changed ) {
	    ret = GWidgetAskR(_STR_RetainChanges,yesnocancel,0,2,_STR_RetainChanges);
	    if ( ret==2 )
return( false );
	    if ( ret==1 )
		copyover = false;
	}
    } else {
	ret = GWidgetAskR(_STR_BadNumber,closecancel,0,1,_STR_BadNumberCloseAnyway);
	if ( ret==1 )
return( false );
    }
    if ( copyover && (edits=((FontView *) tv)->edits)!=NULL ) {
	int i;
	for ( i=0; i<tv->table->len/2; ++i )
	    ptputushort(tv->table->data+2*i,edits[i]);
	if ( !tv->table->changed ) {
	    tv->table->changed = true;
	    tv->table->td_changed = true;
	    tv->table->container->changed = true;
	    GDrawRequestExpose(tv->owner->v,NULL,false);
	}
    }
#endif
    tv->destroyed = true;
    GDrawDestroyWindow(tv->gw);
return( true );
}

static struct tableviewfuncs fontfuncs = { font_close, font_processdata };

static void font_resize(FontView *fv,GEvent *event) {
    GRect pos;
    int lh;

    /* height must be a multiple of the line height */
    /* Width a multiple too */
    if ( (event->u.resize.size.height-fv->mbh-fv->ifh)%fv->boxh!=0 ||
	    (event->u.resize.size.height-fv->mbh-fv->ifh-fv->boxh)<0 ||
	    (event->u.resize.size.width-fv->sbw)%fv->boxh!=0 ||
	    (event->u.resize.size.width-fv->sbw-fv->boxh)<0 ) {
	int lc = (event->u.resize.size.height-fv->mbh-fv->ifh+fv->fh/2)/fv->boxh;
	int wc = (event->u.resize.size.width-fv->sbw+fv->fh/2)/fv->boxh;
	if ( lc<=0 ) lc = 1;
	if ( wc<=0 ) wc = 1;
	GDrawResize(fv->gw, fv->sbw+wc*fv->boxh,fv->mbh+fv->ifh+lc*fv->boxh );
return;
    }

    pos.width = GDrawPointsToPixels(fv->gw,_GScrollBar_Width);
    pos.height = event->u.resize.size.height-fv->mbh-fv->ifh;
    pos.x = event->u.resize.size.width-pos.width; pos.y = fv->mbh+fv->ifh;
    GGadgetResize(fv->vsb,pos.width,pos.height);
    GGadgetMove(fv->vsb,pos.x,pos.y);
    pos.width = pos.x; pos.x = 0;
    GDrawResize(fv->v,pos.width,pos.height);

    fv->vheight = pos.height; fv->vwidth = pos.width;
    fv->cols = fv->vwidth/fv->boxh;
    fv->rows = lh = (fv->cf->glyph_cnt+fv->cols-1)/fv->cols;

    GScrollBarSetBounds(fv->vsb,0,lh,fv->vheight/fv->boxh);
    if ( fv->lpos + fv->vheight/fv->boxh > lh ) {
	int lpos = lh-fv->vheight/fv->boxh;
	if ( lpos<0 ) lpos = 0;
	fv->lpos = lpos;
    }
    GScrollBarSetPos(fv->vsb,fv->lpos);

    GDrawRequestExpose(fv->gw,NULL,false);
    GDrawRequestExpose(fv->v,NULL,false);
}

static void font_expose(FontView *fv,GWindow pixmap,GRect *rect) {
    int low, high;
    int i,j, y, index, ch, width;
    /*ConicFont *cf = fv->cf;*/
    struct enctab *enc = fv->font->enc;
    unichar_t temp[2];

    GDrawSetFont(pixmap,fv->gfont);

    low = rect->y/fv->boxh;
    high = (rect->y+rect->height+fv->fh)/fv->boxh;

    for ( i=low; i<=high; ++i )
	GDrawDrawLine(pixmap,0,i*fv->boxh,fv->vwidth,i*fv->boxh,0x000000);
    for ( i=0; i<=fv->cols; ++i )
	GDrawDrawLine(pixmap,i*fv->boxh,0,i*fv->boxh,fv->vwidth,0x000000);

    for ( i=low; i<high; ++i ) {
	y = i*fv->boxh + CHAR_PAD+1 ;
	for ( j=0; j<fv->cols; ++j ) {
	    index = (fv->lpos+i)*fv->cols + j;
	    if ( index>=fv->cf->glyph_cnt )
	break;
#if TT_RASTERIZE_FONTVIEW
	    if ( fv->rasters[index]==NULL )
		fv->rasters[index] = FreeType_GetRaster(fv,index);
	    if ( fv->rasters[index]!=(void *) -1 )	/* Rasterization error flag */
		FreeType_ShowRaster(pixmap,
			j*fv->boxh+(fv->boxh-fv->rasters[index]->cols)/2,
			y+20-fv->rasters[index]->as,
			fv->rasters[index]);
	    else {
#endif
		if ( enc!=NULL && index<enc->cnt )
		    ch = enc->uenc[index];
		else
		    ch = CHAR_UNDEF;
		temp[0] = ch==CHAR_UNDEF? '?' : ch;
		temp[1] = '\0';
		width = GDrawGetTextWidth(pixmap,temp,-1,NULL);
		GDrawDrawText(pixmap,j*fv->boxh+(fv->fh-width)/2,y+fv->as,temp,-1,
			NULL, ch==CHAR_UNDEF ? 0xff0000 : 0x000000);
#if TT_RASTERIZE_FONTVIEW
	    }
#endif
	}
    }
}

static void font_infoexpose(FontView *fv,GWindow pixmap,GRect *rect) {
    int uch;
    char buf[100];
    unichar_t ubuf[100];

    if ( fv->info_glyph==CHAR_UNDEF )
return;
    sprintf( buf, "Glyph: %5d", fv->info_glyph );
    uc_strcpy(ubuf,buf);
    GDrawSetFont(pixmap,fv->ifont);
    GDrawDrawText(pixmap,5,fv->mbh+fv->ias,ubuf,-1,NULL,0x000000);
    if ( fv->font->enc!=NULL && fv->info_glyph<fv->font->enc->cnt &&
	    (uch=fv->font->enc->uenc[fv->info_glyph])!=CHAR_UNDEF ) {
	sprintf( buf, "Unicode: U+%04X ", uch );
	if ( psunicodenames[uch]!=NULL )
	    strcat(buf, psunicodenames[uch]);
	uc_strcpy(ubuf,buf);
	GDrawDrawText(pixmap,100,fv->mbh+fv->ias,ubuf,-1,NULL,0x000000);
    }
}

static void CCPreparePopup(GWindow gw,struct enctab *enc, int index) {
    static unichar_t space[200];
    char cspace[40];
    int upos;

    if ( enc==NULL || index<0 || index>=enc->cnt || (upos=enc->uenc[index])==CHAR_UNDEF )
return;

    if ( UnicodeCharacterNames[upos>>8][upos&0xff]!=NULL ) {
	GGadgetPreparePopup(gw,UnicodeCharacterNames[upos>>8][upos&0xff]);
    } else {
	sprintf( cspace, "%04x %s", upos,
	    upos=='\0'				? "Null":
	    upos=='\t'				? "Tab":
	    upos=='\r'				? "Return":
	    upos=='\n'				? "LineFeed":
	    upos=='\f'				? "FormFeed":
	    upos=='\33'				? "Escape":
	    upos<160				? "Control Char":
	    upos>=0x3400 && upos<=0x4db5	? "CJK Ideograph Extension A":
	    upos>=0x4E00 && upos<=0x9FA5	? "CJK Ideograph":
	    upos>=0xAC00 && upos<=0xD7A3	? "Hangul Syllable":
	    upos>=0xD800 && upos<=0xDB7F	? "Non Private Use High Surrogate":
	    upos>=0xDB80 && upos<=0xDBFF	? "Private Use High Surrogate":
	    upos>=0xDC00 && upos<=0xDFFF	? "Low Surrogate":
	    upos>=0xE000 && upos<=0xF8FF	? "Private Use":
					      "Unencoded Unicode" );
	uc_strcpy(space,cspace);
	GGadgetPreparePopup(gw,space);
    }
}

static void font_mousemove(FontView *fv,int index) {
    GRect r;

    fv->info_glyph = index;
    r.y = fv->mbh; r.x = 0;
    r.height = fv->ifh; r.width = fv->vwidth+fv->sbw;
    GDrawRequestExpose(fv->gw,&r,false);
    CCPreparePopup(fv->gw,fv->cf->tfont->enc, index);
}

static void font_scroll(FontView *fv,struct sbevent *sb) {
    int newpos = fv->lpos;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= fv->vheight/fv->fh;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += fv->vheight/fv->fh;
      break;
      case et_sb_bottom:
        newpos = fv->rows-fv->vheight/fv->fh;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>fv->rows-fv->vheight/fv->boxh )
        newpos = fv->rows-fv->vheight/fv->boxh;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=fv->lpos ) {
	int diff = newpos-fv->lpos;
	fv->lpos = newpos;
	GScrollBarSetPos(fv->vsb,fv->lpos);
	GDrawScroll(fv->v,NULL,0,diff*fv->boxh);
    }
}

static void FontViewFree(FontView *fv) {
    fv->cf->glyphs->tv = NULL;
    fv->cf->loca->tv = NULL;
#if TT_RASTERIZE_FONTVIEW
    { int i;
	for ( i=0; i<fv->cf->glyph_cnt; ++i )
	    if ( fv->rasters[i]!=NULL && fv->rasters[i]!=(void *) -1 )
		FreeType_FreeRaster(fv->rasters[i]);
	free(fv->rasters);
    }
#endif
    free(fv);
}

static int fv_v_e_h(GWindow gw, GEvent *event) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);
    int index;

    switch ( event->type ) {
      case et_expose:
	font_expose(fv,gw,&event->u.expose.rect);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 )
	    TableHelp(fv->table->name);
	else if ( event->u.chr.chars[0]!='\0' && event->u.chr.chars[1]=='\0' &&
		fv->font->enc!=NULL ) {
	    for ( index=0; index<fv->font->enc->cnt; ++index )
		if ( event->u.chr.chars[0]==fv->font->enc->enc[index] )
	    break;
	    if ( index<fv->font->enc->cnt ) {
		int l = index/fv->cols;
		if ( l<=fv->lpos || l>=fv->lpos+fv->vheight/fv->boxh ) {
		    if ( l + fv->vheight/fv->boxh > fv->rows )
			l = fv->rows - fv->vheight/fv->boxh;
		    if ( l<0 ) l=0;
		    fv->lpos = l;
		    GScrollBarSetPos(fv->vsb,l);
		    GDrawRequestExpose(fv->v,NULL,false);
		}
	    }
	}
      break;
      case et_mousemove: case et_mousedown: case et_mouseup:
	GGadgetEndPopup();
	index = (event->u.mouse.y/fv->boxh + fv->lpos)*fv->cols +
		event->u.mouse.x/fv->boxh;
	if ( index>=fv->cf->glyph_cnt )
      break;
	if ( event->type==et_mousemove )
	    font_mousemove(fv,index);
	else if ( event->type == et_mousedown && event->u.mouse.clicks==2 ) {
	    charCreateEditor(fv->cf,index);
	    /* Invoke CharView */
	}
      break;
      case et_timer:
      break;
      case et_focus:
      break;
    }
return( true );
}

static int fv_e_h(GWindow gw, GEvent *event) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	font_infoexpose(fv,gw,&event->u.expose.rect);
      break;
      case et_resize:
	font_resize(fv,event);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 )
	    TableHelp(fv->table->name);
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    font_scroll(fv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_close:
	font_close((TableView *) fv);
      break;
      case et_destroy:
	FontViewFree(fv);
      break;
    }
return( true );
}

static void FVMenuClose(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    FontView *fv = (FontView *) GDrawGetUserData(gw);

    DelayEvent((void (*)(void *)) font_close, fv);
}

static void FVMenuSaveAs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = ((FontView *) GDrawGetUserData(gw))->owner;

    _TFVMenuSaveAs(tfv);
}

static void FVMenuSave(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = ((FontView *) GDrawGetUserData(gw))->owner;
    _TFVMenuSave(tfv);
}

static void FVMenuRevert(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = ((FontView *) GDrawGetUserData(gw))->owner;
    DelayEvent((void (*)(void *)) _TFVMenuRevert, tfv);
}

static void fllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Recent:
	    mi->ti.disabled = !RecentFilesAny();
	  break;
	}
    }
}

static GMenuItem dummyitem[] = { { (unichar_t *) _STR_Recent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'N' }, NULL };
static GMenuItem fllist[] = {
    { { (unichar_t *) _STR_Open, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'O' }, 'O', ksm_control, NULL, NULL, MenuOpen },
    { { (unichar_t *) _STR_Recent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 't' }, '\0', ksm_control, dummyitem, MenuRecentBuild, NULL, MID_Recent },
    { { (unichar_t *) _STR_Close, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'C' }, 'Q', ksm_control|ksm_shift, NULL, NULL, FVMenuClose },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Save, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'S' }, 'S', ksm_control, NULL, NULL, FVMenuSave },
    { { (unichar_t *) _STR_SaveAs, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'a' }, 'S', ksm_control|ksm_shift, NULL, NULL, FVMenuSaveAs },
    { { (unichar_t *) _STR_Revertfile, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'R' }, 'R', ksm_control|ksm_shift, NULL, NULL, FVMenuRevert, MID_Revert },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
/*    { { (unichar_t *) _STR_Prefs, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'e' }, '\0', ksm_control, NULL, NULL, MenuPrefs },*/
/*    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},*/
    { { (unichar_t *) _STR_Quit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'Q' }, 'Q', ksm_control, NULL, NULL, MenuExit },
    { NULL }
};

static GMenuItem edlist[] = {
    { { (unichar_t *) _STR_Undo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'U' }, 'Z', ksm_control, NULL, NULL },
    { { (unichar_t *) _STR_Redo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'R' }, 'Y', ksm_control, NULL, NULL },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Cut, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 't' }, 'X', ksm_control, NULL, NULL, NULL, MID_Cut },
    { { (unichar_t *) _STR_Copy, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'C' }, 'C', ksm_control, NULL, NULL, NULL, MID_Copy },
    { { (unichar_t *) _STR_Paste, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'P' }, 'V', ksm_control, NULL, NULL, NULL, MID_Paste },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_SelectAll, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'A' }, 'A', ksm_control, NULL, NULL, NULL },
    { NULL }
};

extern GMenuItem helplist[];

static GMenuItem mblist[] = {
    { { (unichar_t *) _STR_File, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'F' }, 0, 0, fllist, fllistcheck },
    { { (unichar_t *) _STR_Edit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'E' }, 0, 0, edlist },
    { { (unichar_t *) _STR_Window, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'W' }, 0, 0, NULL, WindowMenuBuild, NULL },
    { { (unichar_t *) _STR_Help, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'H' }, 0, 0, helplist, NULL },
    { NULL }
};


/* glyf,loca tables */
void fontCreateEditor(Table *tab,TtfView *tfv) {
    FontView *fv = gcalloc(1,sizeof(FontView));
    unichar_t title[60];
    GRect pos, gsize;
    GWindow gw;
    GWindowAttrs wattrs;
    FontRequest rq;
    static unichar_t monospace[] = { 'c','o','u','r','i','e','r',',','m', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',',','u','n','i','f','o','n','t', '\0' };
    int as,ds,ld;
    GGadgetData gd;
    GGadget *sb;

    fv->cf = LoadConicFont(tfv->ttf->fonts[tfv->selectedfont]);
    if ( fv->cf==NULL ) {
	GWidgetErrorR(_STR_CouldntReadGlyphs,_STR_CouldntReadGlyphs);
	free(fv);
return;
    }
    fv->table = tab = fv->cf->glyphs;
    fv->virtuals = &fontfuncs;
    fv->owner = tfv;
    fv->font = tfv->ttf->fonts[tfv->selectedfont];
    fv->cf->glyphs->tv = (TableView *) fv;
    fv->cf->loca->tv = (TableView *) fv;
    fv->cf->fv = fv;

    title[0] = (tab->name>>24)&0xff;
    title[1] = (tab->name>>16)&0xff;
    title[2] = (tab->name>>8 )&0xff;
    title[3] = (tab->name    )&0xff;
    title[4] = ' ';
    u_strncpy(title+5, fv->font->fontname, sizeof(title)/sizeof(title[0])-6);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = title;
    wattrs.icon = ttf_icon;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,200);
    pos.height = GDrawPointsToPixels(NULL,100);
    fv->gw = gw = GDrawCreateTopWindow(NULL,&pos,fv_e_h,fv,&wattrs);

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    gd.u.menu = mblist;
    fv->mb = GMenuBarCreate( gw, &gd, NULL);
    GGadgetGetSize(fv->mb,&gsize);
    fv->mbh = gsize.height;

    memset(&rq,0,sizeof(rq));
    rq.family_name = monospace;
    rq.point_size = -12;
    rq.weight = 400;
    fv->ifont = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawSetFont(fv->gw,fv->ifont);
    GDrawFontMetrics(fv->ifont,&as,&ds,&ld);
    fv->ias = as+1;
    fv->ifh = fv->ias+ds;

    gd.pos.y = fv->mbh+fv->ifh; gd.pos.height = pos.height-gd.pos.y;
    gd.pos.width = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gd.pos.x = pos.width-gd.pos.width;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    fv->vsb = sb = GScrollBarCreate(gw,&gd,fv);
    GGadgetGetSize(fv->vsb,&gsize);
    fv->sbw = gsize.width;

    wattrs.mask = wam_events|wam_cursor;
    pos.x = 0; pos.y = fv->mbh+fv->ifh;
    pos.width = gd.pos.x; pos.height = gd.pos.height;
    fv->v = GWidgetCreateSubWindow(gw,&pos,fv_v_e_h,fv,&wattrs);
    GDrawSetVisible(fv->v,true);

    rq.point_size = -24;
    fv->gfont = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawSetFont(fv->v,fv->gfont);
    GDrawFontMetrics(fv->gfont,&as,&ds,&ld);
    fv->as = as+1;
    fv->fh = fv->as+ds;
    fv->boxh = fv->fh + 2*CHAR_PAD + 1;

#if TT_RASTERIZE_FONTVIEW
    fv->rasters = gcalloc(fv->cf->glyph_cnt,sizeof(void *));
#endif

    GDrawResize(fv->gw,16*fv->boxh+fv->sbw,fv->mbh+fv->ifh+4*fv->boxh);

    GDrawSetVisible(gw,true);
}
