/* Copyright (C) 2000,2001 by George Williams */
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
#include "pfaeditui.h"
#include "gfile.h"
#include "gresource.h"
#include <sys/time.h>
#include <locale.h>
#include <unistd.h>

unsigned short unicode_from_adobestd[256];

static void initadobeenc(void) {
    int i,j;

    for ( i=0; i<0xa0; ++i )
	unicode_from_adobestd[i] = i;
    for ( ; i<0x100; ++i ) {
	if ( strcmp(AdobeStandardEncoding[i],".notdef")==0 )
	    unicode_from_adobestd[i] = 0xfffd;
	else {
	    for ( j=0; j<psunicodenames_cnt; ++j )
		if ( psunicodenames[j]==NULL )
		    /* Tum ty tum tum */;
		else if ( strcmp(AdobeStandardEncoding[i],psunicodenames[j])==0 )
	    break;
	    if ( j==psunicodenames_cnt ) j = 0xfffd;
	    unicode_from_adobestd[i] = j;
	}
    }
}

static void initrand(void) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    srand(tv.tv_usec);
}

static void BuildCharHook(GDisplay *gd) {
    GWidgetCreateInsChar();
}

extern GImage splashimage;
static unichar_t title[] = { 'P', 'f', 'a', 'E', 'd', 'i', 't',  '\0' };
static GWindow splashw;
static GTimer *autosave_timer, *splasht;
struct delayed_event {
    void *data;
    void (*func)(void *);
};

void DelayEvent(void (*func)(void *), void *data) {
    struct delayed_event *info = gcalloc(1,sizeof(struct delayed_event));

    info->data = data;
    info->func = func;
    GDrawRequestTimer(splashw,100,0,info);
}

static void DoDelayedEvents(GEvent *event) {
    GTimer *t = event->u.timer.timer;
    struct delayed_event *info = (struct delayed_event *) (event->u.timer.userdata);

    (info->func)(info->data);
    GDrawCancelTimer(t);
}

static int splash_e_h(GWindow gw, GEvent *event) {
    static int splash_cnt;
    GRect old;

    if ( event->type == et_expose ) {
	GDrawPushClip(gw,&event->u.expose.rect,&old);
	GDrawDrawImage(gw,&splashimage,NULL,0,0);
	GDrawPopClip(gw,&old);
    } else if ( event->type == et_map ) {
	splash_cnt = 0;
    } else if ( event->type == et_timer && event->u.timer.timer==autosave_timer ) {
	DoAutoSaves();
    } else if ( event->type == et_timer && event->u.timer.timer==splasht ) {
	if ( ++splash_cnt==1 )
	    GDrawResize(gw,splashimage.u.image->width,splashimage.u.image->height-20);
	else if ( splash_cnt==2 )
	    GDrawResize(gw,splashimage.u.image->width,splashimage.u.image->height);
	else if ( splash_cnt>=7 ) {
	    GDrawSetVisible(gw,false);
	    GDrawCancelTimer(splasht);
	}
    } else if ( event->type == et_timer ) {
	DoDelayedEvents(event);
    } else if ( event->type==et_char || event->type==et_mousedown ||
	    event->type==et_close )
	GDrawSetVisible(gw,false);
return( true );
}

static void AddR(char *prog, char *name, char *val ) {
    char *full = galloc(strlen(name)+strlen(val)+4);
    strcpy(full,name);
    strcat(full,": ");
    strcat(full,val);
    GResourceAddResourceString(full,prog);
}

int main( int argc, char **argv ) {
    int i;
    GRect pos;
    GWindowAttrs wattrs;
    extern const char *link_time_str;
    int splash = 1;
    int any;
    char *display = NULL;

    fprintf( stderr, "Copyright \251 2000,2001 by George Williams.\n Executable based on sources from %s.\n",
	    link_time_str );
    setlocale(LC_ALL,"");
    GResourceAddResourceString(NULL,argv[0]);
    LoadPrefs();

    for ( i=1; i<argc; ++i ) {
	if ( strcmp(argv[i],"-sync")==0 )
	    GResourceAddResourceString("Gdraw.Synchronize: true",argv[0]);
#if MyMemory
	else if ( strcmp(argv[i],"-memory")==0 )
	    __malloc_debug(5);
#endif
	else if ( strcmp(argv[i],"-depth")==0 && i<argc-1 )
	    AddR(argv[0],"Gdraw.Depth", argv[++i]);
	else if ( strcmp(argv[i],"-vc")==0 && i<argc-1 )
	    AddR(argv[0],"Gdraw.VisualClass", argv[++i]);
	else if ( strcmp(argv[i],"-nosplash")==0 )
	    splash = 0;
	else if ( strcmp(argv[i],"-display")==0 && i<argc-1 )
	    display = argv[++i];
    }
    initadobeenc();
    initrand();

    GDrawCreateDisplays(display,argv[0]);

    /* the splash screen used not to have a title bar (wam_nodecor) */
    /*  but I found I needed to know how much the window manager moved */
    /*  the window around, which I can determine if I have a positioned */
    /*  decorated window created at the begining */
    /* Actually I don't care any more */
    wattrs.mask = wam_events|wam_cursor|wam_bordwidth|wam_positioned|wam_wtitle|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.positioned = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = title;
    wattrs.border_width = 2;
    wattrs.is_dlg = true;
    pos.x = pos.y = 200;
    pos.width = splashimage.u.image->width;
    pos.height = splashimage.u.image->height-50;
    splashw = GDrawCreateTopWindow(NULL,&pos,splash_e_h,NULL,&wattrs);
    if ( splash ) {
	GDrawSetVisible(splashw,true);
	GDrawSync(NULL);
	GDrawProcessPendingEvents(NULL);
	InitCursors();
	GDrawProcessPendingEvents(NULL);
	splasht = GDrawRequestTimer(splashw,1000,1000,NULL);
    } else
	InitCursors();
    autosave_timer=GDrawRequestTimer(splashw,60*1000,30*1000,NULL);

    GDrawProcessPendingEvents(NULL);
    GDrawSetBuildCharHook(BuildCharHook);

    any = DoAutoRecovery();
    for ( i=1; i<argc; ++i ) {
	char buffer[1025];
	GDrawProcessPendingEvents(NULL);
	if ( strcmp(argv[i],"-new")==0 ) {
	    FontNew();
	    any = 1;
	} else if ( strcmp(argv[i],"-sync")==0 || strcmp(argv[i],"-memory")==0 ||
		strcmp(argv[i],"-nosplash")==0 )
	    /* Already done, needed to be before display opened */;
	else if ( (strcmp(argv[i],"-depth")==0 || strcmp(argv[i],"-vc")==0 ||
		    strcmp(argv[i],"-display")==0 ) &&
		i<argc-1 )
	    ++i; /* Already done, needed to be before display opened */
	else {
	    GFileGetAbsoluteName(argv[i],buffer,sizeof(buffer)); 
	    if ( ViewPostscriptFont(buffer)!=0 )
		any = 1;
	}
    }
    if ( !any )
	MenuOpen(NULL,NULL,NULL);
    GDrawEventLoop(NULL);
return( 0 );
}
