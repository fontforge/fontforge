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

#include "gdrawP.h"
#include "ustring.h"

#include <stdarg.h>

/* Preallocate an error dialog so that we can pop it up if things go really bad*/
/*  ie. if memory gets munched somehow */

#define ERR_LINE_MAX	20
static GWindow error;
enum err_type { et_info, et_warn, et_error, et_fatal };
static struct errinfo {
    unichar_t *lines[ERR_LINE_MAX];
    unsigned int dismissed: 1;
    int width;
    enum err_type err_type;
} errinfo;

static int e_h(GWindow gw, GEvent *event) {
    int line;
    int x,len, max_len;
    GRect r;
    static unichar_t ok[] = { 'O', 'K', '\0' };

    if ( event->type == et_expose ) {
	max_len = 0;
	for ( line = 0; line<ERR_LINE_MAX && errinfo.lines[line]!=NULL; ++line ) {
	    len = GDrawGetTextWidth(gw,errinfo.lines[line],-1);
	    if ( len>max_len ) max_len = len;
	}
	x = (errinfo.width-max_len)/2;
	for ( line = 0; line<ERR_LINE_MAX && errinfo.lines[line]!=NULL; ++line )
	    GDrawDrawText(gw,x, 10+10+15*line, errinfo.lines[line],-1,0x000000);

	x = (errinfo.width-(len = GDrawGetTextWidth(gw,ok,2)))/2;
	r.x = x-10; r.y = 25+15*line; r.width = len+20; r.height = 18;
	GDrawFillRect(gw,&r,0xffffff);
	GDrawDrawRect(gw,&r,0x000000);
	GDrawDrawText(gw,x,r.y+13,ok,2,0x000000);
    } else if ( event->type==et_char ) {
	if ( event->u.chr.chars[0]=='\r' || event->u.chr.chars[0]=='\33' )
	    errinfo.dismissed = true;
    } else if ( event->type==et_mouseup ) {
	errinfo.dismissed = true;
    } else if ( event->type==et_close ) {
	errinfo.dismissed = true;
    }
return( 1 );
}

static void RunError() {
    errinfo.dismissed = false;
    GDrawSetVisible(error,true);
    while ( !errinfo.dismissed )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(error,false);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
}

static void ProcessText(unichar_t *ubuf,char *buf, enum err_type et) {
    int max_len = 60, len;
    char *pt, *ept, *last_space;
    unichar_t *ue = ubuf;
    int line=0;

    pt = buf;
    for ( line=0; line<ERR_LINE_MAX && *pt; ++line ) {
	last_space = NULL;
	for ( ept = pt; *ept!='\n' && *ept!='\0' && ept-pt<max_len; ++ept )
	    if ( *ept==' ' )
		last_space = ept;
	if ( *ept!='\n' && *ept!='\0' && last_space!=NULL )
	    ept = last_space;
	errinfo.lines[line] = def2u_strncpy(ue,pt,ept-pt);
	ue[ept-pt] = '\0'; ue += (ept+1-pt);
	if ( *ept=='\n' || *ept==' ' ) ++ept;
	pt = ept;
    }
    for ( ; line<ERR_LINE_MAX ; ++line )
	errinfo.lines[line] = NULL;
    errinfo.err_type = et;

    max_len = 0;
    for ( line = 0; line<ERR_LINE_MAX && errinfo.lines[line]!=NULL; ++line ) {
	len = GDrawGetTextWidth(error,errinfo.lines[line],-1);
	if ( len>max_len ) max_len = len;
    }
    errinfo.width = max_len+30;
    GDrawResize(error,max_len+30,15*line+50);
}

GDisplay *global_gd;

void _GDraw_InitError(GDisplay *gd) {
    GRect screen, pos;
    static unichar_t title[]= { 'E', 'r', 'r', 'o', 'r', '\0' };
    static unichar_t courier[] = { 'c', 'o', 'u', 'r', 'i', 'e', 'r', '\0' };
    static GDisplay *static_gd;
    GWindowAttrs wattrs;
    FontRequest rq;

    if ( gd!=NULL )
	static_gd = gd;
    else
	screen_display = gd = static_gd;

    global_gd = static_gd;

    if ( gd==NULL )
return;

    if ( error != NULL )
return;
    GDrawGetSize(GDrawGetRoot(gd),&screen);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_positioned|wam_cursor|wam_wtitle|wam_backcol|
	    wam_restrict|wam_redirect|wam_isdlg;
    wattrs.event_masks = -1;
    wattrs.positioned = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = title;
    wattrs.background_color = 0xbbbbbb;
    wattrs.restrict_input_to_me = true;
    wattrs.redirect_chars_to_me = true;
    wattrs.is_dlg = true;
    pos.width = 300; pos.height = 180;
    pos.x = (screen.width-pos.width)/2;
    pos.y = (screen.width-pos.width)/3;
    errinfo.width = pos.width;

    error = GDrawCreateTopWindow(gd,&pos,e_h,NULL,&wattrs);

    memset(&rq,0,sizeof(rq));
    rq.family_name = courier;
    rq.point_size = -12;
    rq.weight = 400;
    rq.style = 0;
    GDrawAttachFont(error,&rq);
}

void GDrawIError(const char *fmt,...) {
  // GDrawIErrorRun was the previous version of this function.
  // This new function intercepts the calls and stashes them for future processing.
  // This avoids stack overflows in certain cases.
  GDisplay * gd = global_gd;
  char * buffer = NULL;
  va_list ap;
  va_start(ap, fmt);
  buffer = vsmprintf(fmt, ap);
  va_end(ap);
  if (buffer != NULL ) {
    if ( gd==NULL ) {
      fprintf(stderr, "%s", buffer); // If there is no display, we write to stderr.
    } else {
      if ((gd->err_flag) && (gd->err_report != NULL)) {
        if (strlen(gd->err_report) + strlen(buffer) + 1 < 2048) {
          // If there is an existing error message, we concatenate if there is space.
          char * tmp = smprintf("%s%s\n", gd->err_report, buffer);
          free(gd->err_report); gd->err_report = tmp;
        }
      } else {
        // If there is no existing error message, we copy to the right spot.
        gd->err_report = smprintf("%s\n", buffer);
      }
      gd->err_flag |= 1;
    }
  }
  free(buffer); buffer = NULL;
}

void GDrawIErrorRun(const char *fmt,...) {
    char buf[1025]; unichar_t ubuf[1025];
    va_list ap;

    strcpy(buf,"Internal Error:\n");
    va_start(ap, fmt);
    vsnprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), fmt, ap);
    va_end(ap);
    fprintf( stderr, "%s\n", buf );
    _GDraw_InitError(NULL);
    if ( error!=NULL ) {
	ProcessText(ubuf,buf,et_error);
	RunError();
    }
}

void GDrawError(const char *fmt,...) {
    char buf[1025]; unichar_t ubuf[1025];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    _GDraw_InitError(NULL);
    if ( error==NULL )
	fprintf( stderr, "%s\n", buf );
    else {
	ProcessText(ubuf,buf,et_error);
	RunError();
    }
}

void GDrawFatalError(const char *fmt,...) {
    char buf[1025]; unichar_t ubuf[1025];
    va_list ap;

    strcpy(buf,"Fatal Error:\n");
    va_start(ap, fmt);
    vsprintf(buf+strlen(buf), fmt, ap);
    va_end(ap);
	fprintf( stderr, "%s\n", buf );
    if ( error!=NULL ) {
	ProcessText(ubuf,buf,et_fatal);
	RunError();
    }
    exit(1);
}
