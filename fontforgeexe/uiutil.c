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

#include "fontforgeui.h"
#include "gkeysym.h"
#include "gresedit.h"
#include "ustring.h"
#include "utype.h"

#include <assert.h>

extern GBox _ggadget_Default_Box;
#define ACTIVE_BORDER   (_ggadget_Default_Box.active_border)
#define MAIN_FOREGROUND (_ggadget_Default_Box.main_foreground)

static void UI_IError(const char *format,...) {
    va_list ap;
    char buffer[300];
    va_start(ap,format);
    vsnprintf(buffer,sizeof(buffer),format,ap);
    GDrawIError("%s",buffer);
    va_end(ap);
}

#define MAX_ERR_LINES	400
static struct errordata {
    char *errlines[MAX_ERR_LINES];
    int fh, as;
    GGadget *vsb;
    GWindow gw, v;
    int cnt, linecnt;
    int offtop;
    int showing;
    int start_l, start_c, end_l, end_c;
    int down;
} errdata;

GResFont errfont = GRESFONT_INIT("400 10pt " SANS_UI_FAMILIES);

static void ErrHide(void) {
    GDrawSetVisible(errdata.gw,false);
    errdata.showing = false;
}

static void ErrScroll(struct sbevent *sb) {
    int newpos = errdata.offtop;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= errdata.linecnt;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += errdata.linecnt;
      break;
      case et_sb_bottom:
        newpos = errdata.cnt-errdata.linecnt;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
      case et_sb_halfup: case et_sb_halfdown: break;
    }
    if ( newpos>errdata.cnt-errdata.linecnt )
        newpos = errdata.cnt-errdata.linecnt;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=errdata.offtop ) {
	errdata.offtop = newpos;
	GScrollBarSetPos(errdata.vsb,errdata.offtop);
	GDrawRequestExpose(errdata.v,NULL,false);
    }
}

static int ErrChar(GEvent *e) {
    int newpos = errdata.offtop;

    switch( e->u.chr.keysym ) {
      case GK_Home:
	newpos = 0;
      break;
      case GK_End:
	newpos = errdata.cnt-errdata.linecnt;
      break;
      case GK_Page_Up: case GK_KP_Page_Up:
	newpos -= errdata.linecnt;
      break;
      case GK_Page_Down: case GK_KP_Page_Down:
	newpos += errdata.linecnt;
      break;
      case GK_Up: case GK_KP_Up:
	--newpos;
      break;
      case GK_Down: case GK_KP_Down:
        ++newpos;
      break;
    }
    if ( newpos>errdata.cnt-errdata.linecnt )
        newpos = errdata.cnt-errdata.linecnt;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=errdata.offtop ) {
	errdata.offtop = newpos;
	GScrollBarSetPos(errdata.vsb,errdata.offtop);
	GDrawRequestExpose(errdata.v,NULL,false);
return( true );
    }
return( false );
}

static int warnings_e_h(GWindow gw, GEvent *event) {

    if ( errdata.vsb==NULL )
	return true; // too early
    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
return( GGadgetDispatchEvent(errdata.vsb,event));
    }

    switch ( event->type ) {
      case et_char:
return( ErrChar(event));
      break;
      case et_expose:
      break;
      case et_resize: {
	  GRect size, sbsize;
	  GDrawGetSize(gw,&size);
	  GGadgetGetSize(errdata.vsb,&sbsize);
	  GGadgetMove(errdata.vsb,size.width-sbsize.width,0);
	  GGadgetResize(errdata.vsb,sbsize.width,size.height);
	  GDrawResize(errdata.v,size.width-sbsize.width,size.height);
	  errdata.linecnt = size.height/errdata.fh;
	  GScrollBarSetBounds(errdata.vsb,0,errdata.cnt,errdata.linecnt);
	  if ( errdata.offtop + errdata.linecnt > errdata.cnt ) {
	      errdata.offtop = errdata.cnt-errdata.linecnt;
	      if ( errdata.offtop < 0 ) errdata.offtop = 0;
	      GScrollBarSetPos(errdata.vsb,errdata.offtop);
	  }
	  GDrawRequestExpose(errdata.v,NULL,false);
      } break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    ErrScroll(&event->u.control.u.sb);
	  break;
	  default: break;
	}
      break;
      case et_close:
	ErrHide();
      break;
      case et_create:
      break;
      case et_destroy:
      break;
      default: break;
    }
return( true );
}

static void noop(void *_ed) {
}

static void *genutf8data(void *_ed,int32_t *len) {
    int cnt, l;
    int s_l = errdata.start_l, s_c = errdata.start_c, e_l = errdata.end_l, e_c = errdata.end_c;
    char *ret, *pt;

    if ( s_l>e_l ) {
	s_l = e_l; s_c = e_c; e_l = errdata.start_l; e_c = errdata.start_c;
    }
    if (s_l == e_l && s_c > e_c) {
        s_c = e_c;
        e_c = errdata.start_c;
    }

    if ( s_l==-1 ) {
	*len = 0;
return( copy(""));
    }

    l = s_l;
    if ( e_l == l ) {
	*len = e_c-s_c;
return( copyn( errdata.errlines[l]+s_c, e_c-s_c ));
    }

    cnt = strlen(errdata.errlines[l]+s_c)+1;
    for ( ++l; l<e_l; ++l )
	cnt += strlen(errdata.errlines[l])+1;
    cnt += e_c;

    ret = malloc(cnt+1);
    strcpy( ret, errdata.errlines[s_l]+s_c );
    pt = ret+strlen( ret );
    *pt++ = '\n';
    for ( l=s_l+1; l<e_l; ++l ) {
	strcpy(pt,errdata.errlines[l]);
	pt += strlen(pt);
	*pt++ = '\n';
    }
    strncpy(pt,errdata.errlines[l],e_c);
    pt[e_c] = '\0';
    *len = strlen(ret);
    assert(*len == cnt);
return( ret );
}

static void MouseToPos(GEvent *event,int *_l, int *_c) {
    int l,c=0;

    GDrawSetFont(errdata.v,errfont.fi);
    l = event->u.mouse.y/errdata.fh + errdata.offtop;
    if ( l>=errdata.cnt ) {
	l = errdata.cnt-1;
	if ( l>=0 )
	    c = strlen(errdata.errlines[l]);
    } else if ( l>=0 ) {
	GDrawLayoutInit(errdata.v,errdata.errlines[l],-1,NULL);
	c = GDrawLayoutXYToIndex(errdata.v,event->u.mouse.x-3,4);
    }
    *_l = l < 0 ? 0 : l;
    *_c = c < 0 ? 0 : c;
}

static void WarnMenuCopy(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    GDrawGrabSelection(gw,sn_clipboard);
    GDrawAddSelectionType(gw,sn_clipboard,"UTF8_STRING",&errdata,1,
	    sizeof(char),
	    genutf8data,noop);
    GDrawAddSelectionType(gw,sn_clipboard,"STRING",&errdata,1,
	    sizeof(char),
	    genutf8data,noop);
}

static void WarnMenuClear(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    int i;

    for ( i=0; i<errdata.cnt; ++i ) {
	free(errdata.errlines[i]);
	errdata.errlines[i] = NULL;
    }
    errdata.cnt = 0;
    GDrawRequestExpose(gw,NULL,false);
}

#define MID_Copy	1
#define MID_Clear	2

GMenuItem warnpopupmenu[] = {
    { { (unichar_t *) N_("Cu_t"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, '\0', ksm_control, NULL, NULL, NULL, 0 },
    { { (unichar_t *) N_("_Copy"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, '\0', ksm_control, NULL, NULL, WarnMenuCopy, MID_Copy },
    { { (unichar_t *) N_("_Paste"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, '\0', ksm_control, NULL, NULL, NULL, 0 },
    { { (unichar_t *) N_("C_lear"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, 0, 0, NULL, NULL, WarnMenuClear, MID_Clear },
    GMENUITEM_EMPTY
};

static int warningsv_e_h(GWindow gw, GEvent *event) {
    int i, j, s_l, s_c, e_l, e_c;
    extern GBox _ggadget_Default_Box;

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
return( GGadgetDispatchEvent(errdata.vsb,event));
    }

    switch ( event->type ) {
      case et_expose:
	  /*GDrawFillRect(gw,&event->u.expose.rect,GDrawGetDefaultBackground(NULL));*/
	  GDrawSetFont(gw,errfont.fi);
	  s_l = errdata.start_l, s_c = errdata.start_c, e_l = errdata.end_l, e_c = errdata.end_c;
	  if ( s_l>e_l ) {
		  s_l = e_l; s_c = e_c; e_l = errdata.start_l; e_c = errdata.start_c;
	  }
	  if (s_l == e_l && s_c > e_c) {
		  s_c = e_c;
		  e_c = errdata.start_c;
	  }
	  for ( i=0, j = errdata.offtop; i<errdata.linecnt && j<errdata.cnt; ++i, ++j ) {
	      int xs, xe;
	      GRect r;
	      GDrawLayoutInit(gw,errdata.errlines[j],-1,NULL);
	      if ( j >= s_l && j <= e_l && (j < e_l || e_c > 0) ) {
		  if ( j > s_l )
		      xs = 0;
		  else {
		      GRect pos;
		      GDrawLayoutIndexToPos(gw,s_c,&pos);
		      xs = pos.x+3;
		  }
		  if ( j < e_l )
		      xe = 3000;
		  else {
		      GRect pos;
		      GDrawLayoutIndexToPos(gw,e_c-1,&pos);
		      xe = pos.x+pos.width+3;
		  }
		  r.x = xs+3; r.width = xe-xs;
		  r.y = i*errdata.fh; r.height = errdata.fh;
		  GDrawFillRect(gw,&r,ACTIVE_BORDER);
	      }
	      GDrawLayoutDraw(gw,3,i*errdata.fh+errdata.as,MAIN_FOREGROUND);
	  }
      break;
      case et_char:
return( ErrChar(event));
      break;
      case et_mousedown:
	if ( event->u.mouse.button==3 ) {
	    warnpopupmenu[1].ti.disabled = errdata.start_l == -1;
	    warnpopupmenu[3].ti.disabled = errdata.cnt == 0;
	    GMenuCreatePopupMenu(gw,event, warnpopupmenu);
	} else {
	    if ( errdata.down )
return( true );
	    MouseToPos(event,&errdata.start_l,&errdata.start_c);
	    errdata.down = true;
	}
      case et_mousemove:
      case et_mouseup:
        if ( !errdata.down )
return( true );
	MouseToPos(event,&errdata.end_l,&errdata.end_c);
	GDrawRequestExpose(gw,NULL,false);
	if ( event->type==et_mouseup ) {
	    errdata.down = false;
	    if ( errdata.start_l == errdata.end_l && errdata.start_c == errdata.end_c ) {
		errdata.start_l = errdata.end_l = -1;
	    } else {
		GDrawGrabSelection(gw,sn_primary);
		GDrawAddSelectionType(gw,sn_primary,"UTF8_STRING",&errdata,1,
			sizeof(char),
			genutf8data,noop);
		GDrawAddSelectionType(gw,sn_primary,"STRING",&errdata,1,
			sizeof(char),
			genutf8data,noop);
	    }
	}
      break;
      default:
      break;
    }
return( true );
}

static void CreateErrorWindow(void) {
    GWindowAttrs wattrs;
    GRect pos,size;
    int as, ds, ld;
    GWindow gw;
    GGadgetData gd;
    extern int _GScrollBar_Width;

    GDrawGetSize(GDrawGetRoot(NULL),&size);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_isdlg|wam_positioned;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.cursor = ct_pointer;
    wattrs.positioned = true;
    wattrs.utf8_window_title = _("Warnings");
    pos.width = GDrawPointsToPixels(NULL,GGadgetScale(400));
    pos.height = GDrawPointsToPixels(NULL,GGadgetScale(100));
    pos.x = size.width - pos.width - 10;
    pos.y = size.height - pos.height - 30;
    errdata.gw = gw = GDrawCreateTopWindow(NULL,&pos,warnings_e_h,&errdata,&wattrs);

    MiscWinInit();
    GDrawWindowFontMetrics(errdata.gw,errfont.fi,&as,&ds,&ld);
    errdata.as = as;
    errdata.fh = as+ds;

    memset(&gd,0,sizeof(gd));
    gd.pos.y = 0; gd.pos.height = pos.height;
    gd.pos.width = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gd.pos.x = pos.width-gd.pos.width;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    errdata.vsb = GScrollBarCreate(gw,&gd,&errdata);

    pos.width -= gd.pos.width;
    pos.x = pos.y = 0;
    wattrs.mask = wam_events|wam_cursor;
    errdata.v = GWidgetCreateSubWindow(gw,&pos,warningsv_e_h,&errdata,&wattrs);
    GDrawSetVisible(errdata.v,true);

    errdata.linecnt = pos.height/errdata.fh;
    errdata.start_l = errdata.end_l = -1;
}

static void AppendToErrorWindow(char *buffer) {
    int i,linecnt;
    char *pt,*end;

    if ( buffer[strlen(buffer)-1]=='\n' ) buffer[strlen(buffer)-1] = '\0';

    for ( linecnt=1, pt=buffer; (pt=strchr(pt,'\n'))!=NULL; ++linecnt )
	++pt;
    if ( errdata.cnt + linecnt > MAX_ERR_LINES ) {
	int off = errdata.cnt + linecnt - MAX_ERR_LINES;
	for ( i=0; i<off; ++i )
	    free(errdata.errlines[i]);
	for ( /*i=off*/; i<errdata.cnt; ++i )
	    errdata.errlines[i-off] = errdata.errlines[i];
	for ( ; i<MAX_ERR_LINES+off ; ++i )
	    errdata.errlines[i-off] = NULL;
	errdata.cnt -= off;
	if (( errdata.start_l -= off)< 0 ) errdata.start_l = errdata.start_c = 0;
	if (( errdata.end_l -= off)< 0 ) errdata.end_l = errdata.start_l = -1;
    }
    for ( i=errdata.cnt, pt=buffer; i<MAX_ERR_LINES; ++i ) {
	end = strchr(pt,'\n');
	if ( end==NULL ) end = pt+strlen(pt);
	errdata.errlines[i] = copyn(pt,end-pt);
	pt = end;
	if ( *pt=='\0' ) {
	    ++i;
    break;
	}
	++pt;
    }
    errdata.cnt = i;

    errdata.offtop = errdata.cnt - errdata.linecnt;
    if ( errdata.offtop<0 ) errdata.offtop = 0;
    GScrollBarSetBounds(errdata.vsb,0,errdata.cnt,errdata.linecnt);
    GScrollBarSetPos(errdata.vsb,errdata.offtop);
}

int ErrorWindowExists(void) {
return( errdata.gw!=NULL );
}

void ShowErrorWindow(void) {
    if ( errdata.gw==NULL )
return;
    GDrawSetVisible(errdata.gw,true);
    GDrawRaise(errdata.gw);
    if ( errdata.showing )
	GDrawRequestExpose(errdata.v,NULL,false);
    errdata.showing = true;
}

static void _LogError(const char *format,va_list ap) {
    char buffer[2500], nbuffer[2600], *str, *pt, *npt;
    vsnprintf(buffer,sizeof(buffer),format,ap);
    for ( pt=buffer, npt=nbuffer; *pt!='\0' && npt<nbuffer+sizeof(nbuffer)-2; ) {
	*npt++ = *pt++;
	if ( pt[-1]=='\n' && *pt!='\0' ) {
	    /* Force an indent of at least two spaces on secondary lines of a warning */
	    if ( npt<nbuffer+sizeof(nbuffer)-2 ) {
		*npt++ = ' ';
		if ( *pt==' ' ) ++pt;
	    }
	    if ( npt<nbuffer+sizeof(nbuffer)-2 ) {
		*npt++ = ' ';
		if ( *pt==' ' ) ++pt;
	    }
	}
    }
    *npt='\0';

    if ( no_windowing_ui || screen_display==NULL ) {
	str = utf82def_copy(nbuffer);
	fprintf(stderr,"%s",str);
	if ( str[strlen(str)-1]!='\n' )
	    putc('\n',stderr);
	free(str);
    } else {
	if ( !ErrorWindowExists())
	    CreateErrorWindow();
	AppendToErrorWindow(nbuffer);
	ShowErrorWindow();
    }
}

static void UI_LogError(const char *format,...) {
    va_list ap;

    va_start(ap,format);
    _LogError(format,ap);
    va_end(ap);
}

static void UI_post_notice(const char *title,const char *statement,...) {
    va_list ap;
    va_start(ap,statement);
    if ( no_windowing_ui ) {
	_LogError(statement,ap);
    } else {
	if ( GWidgetPostNoticeActive8(title))
	    _LogError(statement,ap);
	else
	    _GWidgetPostNotice8(title,statement,ap,40);
    }
    va_end(ap);
}

static char *UI_open_file(const char *title, const char *defaultfile,
	const char *initial_filter) {
return( gwwv_open_filename(title,defaultfile,initial_filter,NULL) );
}

static char *UI_saveas_file(const char *title, const char *defaultfile,
	const char *initial_filter) {
return( gwwv_save_filename(title,defaultfile,initial_filter) );
}

static void tinysleep(int microsecs) {
#if !defined(__MINGW32__)
    fd_set none;
    struct timeval timeout;

    FD_ZERO(&none);
    memset(&timeout,0,sizeof(timeout));
    timeout.tv_usec = microsecs;

    select(1,&none,&none,&none,&timeout);
#endif
}

static void allow_events(void) {
    GDrawSync(NULL);
    tinysleep(100);
    GDrawProcessPendingEvents(NULL);
}


struct ui_interface gdraw_ui_interface = {
    UI_IError,
    gwwv_post_error,
    UI_LogError,
    UI_post_notice,
    gwwv_ask_centered,
    gwwv_choose,
    gwwv_choose_multiple,
    gwwv_ask_string,
    gwwv_ask_password,
    UI_open_file,
    UI_saveas_file,
    gwwv_progress_start_indicator,
    gwwv_progress_end_indicator,
    gwwv_progress_show,
    gwwv_progress_enable_stop,
    gwwv_progress_next,
    gwwv_progress_next_stage,
    gwwv_progress_increment,
    gwwv_progress_change_line1,
    gwwv_progress_change_line2,
    gwwv_progress_pause_timer,
    gwwv_progress_resume_timer,
    gwwv_progress_change_stages,
    gwwv_progress_change_total,
    gwwv_progress_reset,

    allow_events,

    UI_TTFNameIds,
    UI_MSLangString,
    _ImportParamsDlg,
    _ExportParamsDlg,
#ifndef _NO_PYTHON
    _PluginDlg,
#endif
    UI_Ask_Multi
};

