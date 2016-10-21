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

#include "ffglib.h"

#include <fontforge-config.h>
#include "autosave.h"
#include "bitmapchar.h"
#include "clipnoui.h"
#include "encoding.h"
#include "fontforgeui.h"
#include "lookups.h"
#ifndef _NO_LIBUNICODENAMES
#include <libunicodenames.h>	/* need to open a database when we start */
extern uninm_names_db names_db; /* Unicode character names and annotations database */
extern uninm_blocks_db blocks_db;
#endif
#include <gfile.h>
#include <gresource.h>
#include <ustring.h>
#include <ltdl.h>
#include <time.h>
#include <sys/time.h>
#include <locale.h>
#include <unistd.h>
#include <dynamic.h>
#include <stdlib.h>		/* getenv,setenv */
#include <sys/stat.h>
#include <sys/types.h>
#include "../gdraw/hotkeys.h"
#include "gutils/prefs.h"


#define GTimer GTimer_GTK
#include <glib.h>
#include <glib-object.h>
#undef GTimer

#ifdef __Mac
extern void setup_cocoa_app();
#endif

#ifdef _NO_LIBPNG
#  define PNGLIBNAME	"libpng"
#else
#  include <png.h>		/* for version number to find up shared image name */
#  if !defined(PNG_LIBPNG_VER_MAJOR) || (PNG_LIBPNG_VER_MAJOR==1 && PNG_LIBPNG_VER_MINOR<2)
#    define PNGLIBNAME	"libpng"
#  else
#    define xstr(s) str(s)
#    define str(s) #s
#    define PNGLIBNAME	"libpng" xstr(PNG_LIBPNG_VER_MAJOR) xstr(PNG_LIBPNG_VER_MINOR)
#  endif
#endif
#ifdef __Mac
#  include <carbon.h>
/* For reasons obscure to me RunApplicationEventLoop is not defined in */
/*  the mac header files if we are in 64 bit mode. Strangely it seems to */
/*  be in the libraries and functional */
/*
 * It was found in Dec 2014 that using RunApplicationEventLoop() could induce strange
 * and extremely frustrating pausing issues on osx. The main generic event handling
 * seems to work just fine, so there doesn't seem to be a need for this specialized
 * Application Event Loop.
 * 
 * See this issue bringing back Breakpad usage and the issues linked in comments 2,3
 * by adrientetar:
 * https://github.com/fontforge/fontforge/issues/2120
 */
//#  if __LP64__
//extern void RunApplicationEventLoop(void);
//#  endif
#endif

#if defined(__MINGW32__)
#include <windows.h>
#define sleep(n) Sleep(1000 * (n))
#endif

#include "collabclientui.h"

extern int AutoSaveFrequency;
int splash = 1;
static int localsplash;
static int unique = 0;

/**
 * In osx versions prior to 10.9.x a special -psn_ flag was supplied
 * when fontforge was run by osx in some cases. For opening an sfd
 * file from finder we need to register the openWith event in order to
 * get the name of the file to open. So it makes sense to always
 * register for Apple events on OSX so that we can get those file
 * names as they come through.
 */
#if defined(__Mac)
    static int listen_to_apple_events = true; // This was once true, but Apple broke it.
#else
    static int listen_to_apple_events = false;
#endif
static bool ProcessPythonInitFiles = 1;

static void _dousage(void) {
    printf( "fontforge [options] [fontfiles]\n" );
    printf( "\t-new\t\t\t (creates a new font)\n" );
    printf( "\t-last\t\t\t (loads the last sfd file closed)\n" );
#if HANYANG
    printf( "\t-newkorean\t\t (creates a new korean font)\n" );
#endif
    printf( "\t-recover none|auto|inquire|clean (control error recovery)\n" );
    printf( "\t-allglyphs\t\t (load all glyphs in the 'glyf' table\n\t\t\t of a truetype collection)\n" );
    printf( "\t-nosplash\t\t (no splash screen)\n" );
    printf( "\t-quiet\t\t\t (don't print non-essential information to stderr)\n" );
    printf( "\t-unique\t\t\t (if a fontforge is already running open\n\t\t\t all arguments in it and have this process exit)\n" );
    printf( "\t-display display-name\t (sets the X display)\n" );
    printf( "\t-depth val\t\t (sets the display depth if possible)\n" );
    printf( "\t-vc val\t\t\t (sets the visual class if possible)\n" );
    printf( "\t-cmap current|copy|private\t (sets the type of colormap)\n" );
    printf( "\t-dontopenxdevices\t (in case that fails)\n" );
    printf( "\t-sync\t\t\t (syncs the display, debugging)\n" );
    printf( "\t-keyboard ibm|mac|sun|ppc  (generates appropriate hotkeys in menus)\n" );
#if MyMemory
    printf( "\t-memory\t\t\t (turns on memory checks, debugging)\n" );
#endif
#ifndef _NO_LIBCAIRO
    printf( "\t-usecairo=yes|no  Use (or not) the cairo library for drawing\n" );
#endif
    printf( "\t-help\t\t\t (displays this message, and exits)\n" );
    printf( "\t-docs\t\t\t (displays this message, invokes a browser)\n\t\t\t\t (Using the BROWSER environment variable)\n" );
    printf( "\t-version\t\t (prints the version of fontforge and exits)\n" );
    printf( "\t-library-status\t (prints information about optional libraries\n\t\t\t\t and exits)\n" );
#ifndef _NO_PYTHON
    printf( "\t-lang=py\t\t use python for scripts (may precede -script)\n" );
#endif
#ifndef _NO_FFSCRIPT
    printf( "\t-lang=ff\t\t use fontforge's legacy scripting language\n" );
#endif
    printf( "\t-script scriptfile\t (executes scriptfile)\n" );
    printf( "\t\tmust be the first option (or follow -lang).\n" );
    printf( "\t\tAll others passed to scriptfile.\n" );
    printf( "\t-dry scriptfile\t\t (syntax checks scriptfile)\n" );
    printf( "\t\tmust be the first option. All others passed to scriptfile.\n" );
    printf( "\t\tOnly for fontforge's own scripting language, not python.\n" );
    printf( "\t-c script-string\t (executes argument as scripting cmds)\n" );
    printf( "\t\tmust be the first option. All others passed to the script.\n" );
    printf( "\n" );
    printf( "FontForge will read postscript (pfa, pfb, ps, cid), opentype (otf),\n" );
    printf( "\ttruetype (ttf,ttc), macintosh resource fonts (dfont,bin,hqx),\n" );
    printf( "\tand bdf and pcf fonts. It will also read its own format --\n" );
    printf( "\tsfd files.\n" );
    printf( "If no fontfiles are specified (and -new is not either and there's nothing\n" );
    printf( "\tto recover) then fontforge will produce an open font dlg.\n" );
    printf( "If a scriptfile is specified then FontForge will not open the X display\n" );
    printf( "\tnor will it process any additional arguments. It will execute the\n" );
    printf( "\tscriptfile and give it any remaining arguments\n" );
    printf( "If the first argument is an executable filename, and that file's first\n" );
    printf( "\tline contains \"fontforge\" then it will be treated as a scriptfile.\n\n" );
    printf( "For more information see:\n\thttp://fontforge.sourceforge.net/\n" );
    printf( "Send bug reports to:\tfontforge-devel@lists.sourceforge.net\n" );
}

static void dousage(void) {
    _dousage();
exit(0);
}

static void dohelp(void) {
    _dousage();
    help("overview.html");
exit(0);
}

struct delayed_event {
    void *data;
    void (*func)(void *);
};

static void BuildCharHook(GDisplay *gd) {
    GWidgetCreateInsChar();
}

static void InsCharHook(GDisplay *gd,unichar_t ch) {
    GInsCharSetChar(ch);
}

extern GImage splashimage;
static GWindow splashw;
static GTimer *autosave_timer, *splasht;
static GFont *splash_font, *splash_italic;
static int as,fh, linecnt;
static unichar_t msg[470];
static unichar_t *lines[30], *is, *ie;

void ShowAboutScreen(void) {
    static int first=1;

    if ( first ) {
	GDrawResize(splashw,splashimage.u.image->width,splashimage.u.image->height+linecnt*fh);
	first = false;
    }
    if ( splasht!=NULL )
	GDrawCancelTimer(splasht);
    splasht=NULL;
    GDrawSetVisible(splashw,true);
}

static void SplashLayout() {
    unichar_t *start, *pt, *lastspace;
    extern const char *source_modtime_str;
    extern const char *source_version_str;

    uc_strcpy(msg, "When my father finished his book on Renaissance printing (The Craft of Printing and the Publication of Shakespeare's Works) he told me that I would have to write the chapter on computer typography. This is my attempt to do so.");

    GDrawSetFont(splashw,splash_font);
    linecnt = 0;
    lines[linecnt++] = msg-1;
    for ( start = msg; *start!='\0'; start = pt ) {
	lastspace = NULL;
	for ( pt=start; ; ++pt ) {
	    if ( *pt==' ' || *pt=='\0' ) {
		if ( GDrawGetTextWidth(splashw,start,pt-start)<splashimage.u.image->width-10 )
		    lastspace = pt;
		else
	break;
		if ( *pt=='\0' )
	break;
	    }
	}
	if ( lastspace!=NULL )
	    pt = lastspace;
	lines[linecnt++] = pt;
	if ( *pt ) ++pt;
    }
    uc_strcpy(pt, " FontForge used to be named PfaEdit.");

    pt += u_strlen(pt);
    lines[linecnt++] = pt;
    uc_strcpy(pt,"  git hash: ");
    pt += u_strlen(pt);
    lines[linecnt++] = pt;
    uc_strcat(pt, " ");
    uc_strcat(pt, FONTFORGE_GIT_VERSION);

    pt += u_strlen(pt);
    lines[linecnt++] = pt;
    uc_strcpy(pt,"  Version: ");
    uc_strcat(pt,FONTFORGE_MODTIME_STR);

    pt += u_strlen(pt);
    lines[linecnt++] = pt;
    uc_strcat(pt,"           (");
    uc_strcat(pt,FONTFORGE_MODTIME_STR);
    uc_strcat(pt,"-ML");
#ifdef FREETYPE_HAS_DEBUGGER
    uc_strcat(pt,"-TtfDb");
#endif
#ifdef _NO_PYTHON
    uc_strcat(pt,"-NoPython");
#endif
#ifdef FONTFORGE_CONFIG_USE_DOUBLE
    uc_strcat(pt,"-D");
#endif
    uc_strcat(pt,")");
    pt += u_strlen(pt);
    lines[linecnt++] = pt;
    uc_strcpy(pt,"  Lib Version: ");
    uc_strcat(pt,FONTFORGE_MODTIME_STR);
    lines[linecnt++] = pt+u_strlen(pt);
    lines[linecnt] = NULL;
    is = u_strchr(msg,'(');
    ie = u_strchr(msg,')');
}

void DelayEvent(void (*func)(void *), void *data) {
    struct delayed_event *info = calloc(1,sizeof(struct delayed_event));

    info->data = data;
    info->func = func;
    GDrawRequestTimer(splashw,100,0,info);
}

static void DoDelayedEvents(GEvent *event) {
    GTimer *t = event->u.timer.timer;
    struct delayed_event *info = (struct delayed_event *) (event->u.timer.userdata);

    if ( info!=NULL ) {
	(info->func)(info->data);
	free(info);
    }
    GDrawCancelTimer(t);
}

struct argsstruct {
    int next;
    int argc;
    char **argv;
    int any;
};

static void SendNextArg(struct argsstruct *args) {
    int i;
    char *msg;
    static GTimer *timeout;

    if ( timeout!=NULL ) {
	GDrawCancelTimer(timeout);
	timeout = NULL;
    }

    for ( i=args->next; i<args->argc; ++i ) {
	if ( *args->argv[i]!='-' ||
		strcmp(args->argv[i],"-quit")==0 || strcmp(args->argv[i],"--quit")==0 ||
		strcmp(args->argv[i],"-new")==0 || strcmp(args->argv[i],"--new")==0 )
    break;
    }
    if ( i>=args->argc ) {
	if ( args->any )
exit(0);		/* Sent everything */
	msg = "-open";
    } else
	msg = args->argv[i];
    args->next = i+1;
    args->any  = true;

    GDrawGrabSelection(splashw,sn_user1);
    GDrawAddSelectionType(splashw,sn_user1,"STRING",
	    copy(msg),strlen(msg),1,
	    NULL,NULL);

	/* If we just sent the other fontforge a request to die, it will never*/
	/*  take the selection back. So we should just die quietly */
	/*  But we can't die instantly, or it will never get our death threat */
	/*  (it won't have a chance to ask us for the selection if we're dead)*/
    timeout = GDrawRequestTimer(splashw,1000,0,NULL);
}

/* When we want to send filenames to another running fontforge we want a */
/*  different event handler. We won't have a splash window in that case, */
/*  just an invisible utility window on which we perform a little selection */
/*  dance */
static int request_e_h(GWindow gw, GEvent *event) {

    if ( event->type == et_selclear ) {
	SendNextArg( GDrawGetUserData(gw));
    } else if ( event->type == et_timer )
exit( 0 );

return( true );
}

static void PingOtherFontForge(int argc, char **argv) {
    struct argsstruct args;

    args.next = 1;
    args.argc = argc;
    args.argv = argv;
    args.any  = false;
    GDrawSetUserData(splashw,&args);
    SendNextArg(&args);
    GDrawEventLoop(NULL);
exit( 0 );		/* But the event loop should never return */
}

static void start_splash_screen(void){
    GDrawSetVisible(splashw,true);
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    GDrawProcessPendingEvents(NULL);
    splasht = GDrawRequestTimer(splashw,1000,1000,NULL);

    localsplash = false;
}

#if defined(__Mac)
static FILE *logfile;

/* These are the four apple events to which we currently respond */
static pascal OSErr OpenApplicationAE( const AppleEvent * theAppleEvent,
	AppleEvent * reply, SInt32 handlerRefcon) {
 fprintf( logfile, "OPENAPP event received.\n" ); fflush( logfile );
    if ( localsplash )
	start_splash_screen();
    system( "DYLD_LIBRARY_PATH=\"\"; osascript -e 'tell application \"X11\" to activate'" );
    if ( fv_list==NULL )
	_FVMenuOpen(NULL);
 fprintf( logfile, " event processed %d.\n", noErr ); fflush( logfile );
return( noErr );
}

static pascal OSErr ReopenApplicationAE( const AppleEvent * theAppleEvent,
	AppleEvent * reply, SInt32 handlerRefcon) {
 fprintf( logfile, "ReOPEN event received.\n" ); fflush( logfile );
    if ( localsplash )
	start_splash_screen();
    system( "DYLD_LIBRARY_PATH=\"\"; osascript -e 'tell application \"X11\" to activate'" );
    if ( fv_list==NULL )
	_FVMenuOpen(NULL);
 fprintf( logfile, " event processed %d.\n", noErr ); fflush( logfile );
return( noErr );
}

static pascal OSErr ShowPreferencesAE( const AppleEvent * theAppleEvent,
	AppleEvent * reply, SInt32 handlerRefcon) {
 fprintf( logfile, "PREFS event received.\n" ); fflush( logfile );
    if ( localsplash )
	start_splash_screen();
    system( "DYLD_LIBRARY_PATH=\"\"; osascript -e 'tell application \"X11\" to activate'" );
    DoPrefs();
 fprintf( logfile, " event processed %d.\n", noErr ); fflush( logfile );
return( noErr );
}

static pascal OSErr OpenDocumentsAE( const AppleEvent * theAppleEvent,
	AppleEvent * reply, SInt32 handlerRefcon) {
    AEDescList  docList;
    FSRef       theFSRef;
    long        index;
    long        count = 0;
    OSErr       err;
    char	buffer[2048];

 fprintf( logfile, "OPEN event received.\n" ); fflush( logfile );
    if ( localsplash )
	start_splash_screen();

    err = AEGetParamDesc(theAppleEvent, keyDirectObject,
                         typeAEList, &docList);
    err = AECountItems(&docList, &count);
    for(index = 1; index <= count; index++) {
        err = AEGetNthPtr(&docList, index, typeFSRef,
                        NULL, NULL, &theFSRef,
                        sizeof(theFSRef), NULL);// 4
	err = FSRefMakePath(&theFSRef,(unsigned char *) buffer,sizeof(buffer));
	ViewPostScriptFont(buffer,0);
 fprintf( logfile, " file: %s\n", buffer );
    }
    system( "DYLD_LIBRARY_PATH=\"\"; osascript -e 'tell application \"X11\" to activate'" );
    AEDisposeDesc(&docList);
 fprintf( logfile, " event processed %d.\n", err ); fflush( logfile );

return( err );
}

static void AttachErrorCode(AppleEvent *event,OSStatus err) {
    OSStatus returnVal;

    if ( event==NULL )
return;

    if (event->descriptorType != typeNull) {
	/* Check there isn't already an error attached */
        returnVal = AESizeOfParam(event, keyErrorNumber, NULL, NULL);
        if (returnVal != noErr ) {	/* Add success if no previous error */
            AEPutParamPtr(event, keyErrorNumber,
                        typeSInt32, &err, sizeof(err));
        }
    }
}

static AppleEvent *quit_event = NULL;
static void we_are_dead(void) {
    AttachErrorCode(quit_event,noErr);
    /* Send the reply (I hope) */
    AESendMessage(quit_event,NULL, kAENoReply, kAEDefaultTimeout);
    AEDisposeDesc(quit_event);
    /* fall off the end of the world and die */
 fprintf( logfile, " event succeded.\n"); fflush( logfile );
}

static pascal OSErr QuitApplicationAE( const AppleEvent * theAppleEvent,
	AppleEvent * reply, SInt32 handlerRefcon) {
    static int first_time = true;

 fprintf( logfile, "QUIT event received.\n" ); fflush( logfile );
    quit_event = reply;
    if ( first_time ) {
	atexit( we_are_dead );
	first_time = false;
    }
    MenuExit(NULL,NULL,NULL);
    /* if we get here, they canceled the quit, so we return a failure */
    quit_event = NULL;
 fprintf( logfile, " event failed %d.\n", errAEEventFailed ); fflush( logfile );
return(errAEEventFailed);
}

/* Install event handlers for the Apple Events we care about */
static  OSErr install_apple_event_handlers(void) {
    OSErr       err;

    err     = AEInstallEventHandler(kCoreEventClass, kAEOpenApplication,
                NewAEEventHandlerUPP(OpenApplicationAE), 0, false);
    require_noerr(err, CantInstallAppleEventHandler);

    err     = AEInstallEventHandler(kCoreEventClass, kAEReopenApplication,
                NewAEEventHandlerUPP(ReopenApplicationAE), 0, false);
    require_noerr(err, CantInstallAppleEventHandler);

    err     = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments,
                NewAEEventHandlerUPP(OpenDocumentsAE), 0, false);
    require_noerr(err, CantInstallAppleEventHandler);

    err     = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication,
                NewAEEventHandlerUPP(QuitApplicationAE), 0, false);
    require_noerr(err, CantInstallAppleEventHandler);

    err     = AEInstallEventHandler(kCoreEventClass, kAEShowPreferences,
                NewAEEventHandlerUPP(ShowPreferencesAE), 0, false);
    require_noerr(err, CantInstallAppleEventHandler);

 /* some debugging code, for now */
 if ( getenv("HOME")!=NULL ) {
  char buffer[1024];
  sprintf( buffer, "%s/.FontForge-LogFile.txt", getenv("HOME"));
  logfile = fopen("/tmp/LogFile.txt","w");
 }
 if ( logfile==NULL )
  logfile = stderr;

CantInstallAppleEventHandler:
    return err;

}

static pascal void DoRealStuff(EventLoopTimerRef timer,void *ignored_data) {
    GDrawProcessPendingEvents(NULL);
    MacServiceReadFDs();
}

static void install_mac_timer(void) {
    EventLoopTimerRef timer;

    InstallEventLoopTimer(GetMainEventLoop(),
	    .001*kEventDurationSecond,.001*kEventDurationSecond,
	    NewEventLoopTimerUPP(DoRealStuff), NULL,
	    &timer);
}
#endif

static int splash_e_h(GWindow gw, GEvent *event) {
    static int splash_cnt;
    GRect old;
    int i, y, x;
    static char *foolishness[] = {
/* GT: These strings are for fun. If they are offensive or incomprehensible */
/* GT: simply translate them as something dull like: "FontForge" */
/* GT: This is a spoof of political slogans, designed to point out how foolish they are */
	    N_("A free press discriminates\nagainst the illiterate."),
	    N_("A free press discriminates\nagainst the illiterate."),
/* GT: This is a pun on the old latin drinking song "Gaudeamus igature!" */
	    N_("Gaudeamus Ligature!"),
	    N_("Gaudeamus Ligature!"),
/* GT: Spoof on the bible */
	    N_("In the beginning was the letter..."),
/* GT: Some wit at MIT came up with this ("ontology recapitulates phylogony" is the original) */
	    N_("fontology recapitulates file-ogeny")
    };

    switch ( event->type ) {
      case et_create:
	GDrawGrabSelection(gw,sn_user1);
      break;
      case et_expose:
	GDrawPushClip(gw,&event->u.expose.rect,&old);
	GDrawDrawImage(gw,&splashimage,NULL,0,0);
	GDrawSetFont(gw,splash_font);
	y = splashimage.u.image->height + as + fh/2;
	for ( i=1; i<linecnt; ++i ) {
	    if ( is>=lines[i-1]+1 && is<lines[i] ) {
		x = 8+GDrawDrawText(gw,8,y,lines[i-1]+1,is-lines[i-1]-1,0x000000);
		GDrawSetFont(gw,splash_italic);
		GDrawDrawText(gw,x,y,is,lines[i]-is,0x000000);
	    } else if ( ie>=lines[i-1]+1 && ie<lines[i] ) {
		x = 8+GDrawDrawText(gw,8,y,lines[i-1]+1,ie-lines[i-1]-1,0x000000);
		GDrawSetFont(gw,splash_font);
		GDrawDrawText(gw,x,y,ie,lines[i]-ie,0x000000);
	    } else
		GDrawDrawText(gw,8,y,lines[i-1]+1,lines[i]-lines[i-1]-1,0x000000);
	    y += fh;
	}
	GDrawPopClip(gw,&old);
      break;
      case et_map:
	splash_cnt = 0;
      break;
      case et_timer:
	if ( event->u.timer.timer==autosave_timer ) {
	    DoAutoSaves();
	} else if ( event->u.timer.timer==splasht ) {
	    if ( ++splash_cnt==1 )
		GDrawResize(gw,splashimage.u.image->width,splashimage.u.image->height-30);
	    else if ( splash_cnt==2 )
		GDrawResize(gw,splashimage.u.image->width,splashimage.u.image->height);
	    else if ( splash_cnt>=7 ) {
		GGadgetEndPopup();
		GDrawSetVisible(gw,false);
		GDrawCancelTimer(splasht);
		splasht = NULL;
	    }
	} else {
	    DoDelayedEvents(event);
	}
      break;
      case et_char:
      case et_mousedown:
      case et_close:
	GGadgetEndPopup();
	GDrawSetVisible(gw,false);
      break;
      case et_mousemove:
	GGadgetPreparePopup8(gw,_(foolishness[rand()%(sizeof(foolishness)/sizeof(foolishness[0]))]) );
      break;
      case et_selclear:
	/* If this happens, it means someone wants to send us a message with a*/
	/*  filename to open. So we need to ask for it, process it, and then  */
	/*  take the selection back again */
	if ( event->u.selclear.sel == sn_user1 ) {
	    int len;
	    char *arg;
	    arg = GDrawRequestSelection(splashw,sn_user1,"STRING",&len);
	    if ( arg==NULL )
return( true );
	    if ( strcmp(arg,"-new")==0 || strcmp(arg,"--new")==0 )
		FontNew();
	    else if ( strcmp(arg,"-open")==0 || strcmp(arg,"--open")==0 )
		_FVMenuOpen(NULL);
	    else if ( strcmp(arg,"-quit")==0 || strcmp(arg,"--quit")==0 )
		MenuExit(NULL,NULL,NULL);
	    else
		ViewPostScriptFont(arg,0);
	    free(arg);
	    GDrawGrabSelection(splashw,sn_user1);
	}
      break;
      case et_destroy:
	IError("Who killed the splash screen?");
      break;
    }
return( true );
}

static void  AddR(char *program_name, char *window_name, char *cmndline_val) {
/* Add this command line value to this GUI resource.			*/
/* These are the command line options expected when using this routine:	*/
/*	-depth, -vc,-cmap or -colormap,-dontopenxdevices, -keyboard	*/
    char *full;
    if ((full = malloc(strlen(window_name)+strlen(cmndline_val)+4))!=NULL) {
	strcpy(full,window_name);
	strcat(full,": ");
	strcat(full,cmndline_val);
	GResourceAddResourceString(full,program_name);
	free(full);
    }
}

static int ReopenLastFonts(void) {
    char buffer[1024];
    char *ffdir = getFontForgeUserDir(Config);
    FILE *old;
    int any = 0;

    if ( ffdir==NULL ) return false;

    sprintf( buffer, "%s/FontsOpenAtLastQuit", ffdir );
    old = fopen(buffer,"r");
    if ( old==NULL ) {
        free(ffdir);
        return false;
    }
    while ( fgets(buffer,sizeof(buffer),old)!=NULL ) {
    if ( ViewPostScriptFont(g_strchomp(buffer),0)!=0 )
        any = 1;
    }
    fclose(old);
    free(ffdir);
    return any;
}

#if defined(__Mac)
/* Read a property from the x11 properties files */
/* At the moment we want to know if we get the command key, or if the menubar */
/*    eats it */
static int get_mac_x11_prop(char *keystr) {
    CFPropertyListRef ret;
    CFStringRef key, appID;
    int val;

    appID = CFStringCreateWithBytes(NULL,(uint8 *) "com.apple.x11",strlen("com.apple.x11"), kCFStringEncodingISOLatin1, 0);
    key   = CFStringCreateWithBytes(NULL,(uint8 *) keystr,strlen(keystr), kCFStringEncodingISOLatin1, 0);
    ret = CFPreferencesCopyAppValue(key,appID);
    if ( ret==NULL ) {
	/* Sigh. Apple uses a different preference file under 10.5.6 I really */
	/*  wish they'd stop making stupid, unnecessary changes */
	appID = CFStringCreateWithBytes(NULL,(uint8 *) "org.x.X11",strlen("org.x.X11"), kCFStringEncodingISOLatin1, 0);
	ret = CFPreferencesCopyAppValue(key,appID);
    }
    if ( ret==NULL )
return( -1 );
    if ( CFGetTypeID(ret)!=CFBooleanGetTypeID())
return( -2 );
    val = CFBooleanGetValue(ret);
    CFRelease(ret);
return( val );
}

static int uses_local_x(int argc,char **argv) {
    int i;
    char *arg;

    for ( i=1; i<argc; ++i ) {
	arg = argv[i];
	if ( *arg=='-' ) {
	    if ( arg[0]=='-' && arg[1]=='-' && arg[2]!='\0')
		++arg;
	    if ( strcmp(arg,"-display")==0 )
return( i+1<argc && strcmp(argv[i+1],":0")!=0 && strcmp(argv[i+1],":0.0")!=0? 2 : 0 );
	    if ( strcmp(argv[i],"-c")==0 )
return( false );		/* we use a script string, no x display at all */
	    if ( strcmp(arg,"-script")==0 )
return( false );		/* we use a script, no x display at all */
	    if ( strcmp(argv[i],"-")==0 )
return( false );		/* script on stdin */
	} else {
	    /* Is this argument a script file ? */
	    FILE *temp = fopen(argv[i],"r");
	    char buffer[200];
	    if ( temp==NULL )
return( true );			/* not a script file, so need local local X */
	    buffer[0] = '\0';
	    fgets(buffer,sizeof(buffer),temp);
	    fclose(temp);
	    if ( buffer[0]=='#' && buffer[1]=='!' &&
		    (strstr(buffer,"pfaedit")!=NULL || strstr(buffer,"fontforge")!=NULL )) {
return( false );		/* is a script file, so no need for X */

return( true );			/* not a script, so needs X */
	    }
	}
    }
return( true );
}
#endif


#if defined(__Mac)
static int hasquit( int argc, char **argv ) {
    int i;

    for ( i=1; i<argc; ++i )
	if ( strcmp(argv[i],"-quit")==0 || strcmp(argv[i],"--quit")==0 )
return( true );

return( false );
}
#endif

static void GrokNavigationMask(void) {
    extern int navigation_mask;

    navigation_mask = GMenuItemParseMask(H_("NavigationMask|None"));
}

/**
 * Create the directory basedir/dirname with the given mode.
 * Silently ignore any errors that might happen.
 */
static void ffensuredir( const char* basedir, const char* dirname, mode_t mode ) {
    const int buffersz = PATH_MAX;
    char buffer[buffersz+1];

    snprintf(buffer,buffersz,"%s/%s", basedir, dirname );
    // ignore errors, this is just to help the user aftre all.
    mkdir( buffer, mode );
}

static void ensureDotFontForgeIsSetup() {
    char *basedir = getFontForgeUserDir(Config);
    if ( !basedir ) {
	return;
    }
    ffensuredir( basedir, "",       S_IRWXU );
    ffensuredir( basedir, "python", S_IRWXU );
    free(basedir);
}

static void DoAutoRecoveryPostRecover_PromptUserGraphically(SplineFont *sf)
{
    /* Ask user to save-as file */
    char *buts[4];
    buts[0] = _("_OK");
    buts[1] = 0;
    gwwv_ask( _("Recovery Complete"),(const char **) buts,0,1,_("Your file %s has been recovered.\nYou must now Save your file to continue working on it."), sf->filename );
    _FVMenuSaveAs( (FontView*)sf->fv );
}

#if defined(__MINGW32__) && !defined(_NO_LIBCAIRO)
/**
 * \brief Load fonts from the specified folder for the UI to use.
 * This should only be used if Cairo is used on Windows, which defaults to the
 * Win32 font backend.
 * This is an ANSI version, so files which contain characters outside of the
 * user's locale will fail to be loaded.
 * \param prefix The folder to read fonts from. Currently the pixmaps folder
 *               and the folder 'ui-fonts' in the FontForge preferences folder.
 */
static void WinLoadUserFonts(const char *prefix) {
    HANDLE fileHandle;
    WIN32_FIND_DATA fileData;
    char path[MAX_PATH], *ext;
    HRESULT ret;
    int i;

    if (prefix == NULL) {
        return;
    }
    ret = snprintf(path, MAX_PATH, "%s/*.???", prefix);
    if (ret <= 0 || ret >= MAX_PATH) {
        return;
    }

    fileHandle = FindFirstFileA(path, &fileData);
    if (fileHandle != INVALID_HANDLE_VALUE) do {
        ext = strrchr(fileData.cFileName, '.');
        if (!ext || (strcasecmp(ext, ".ttf") && strcasecmp(ext, ".ttc") &&
                     strcasecmp(ext,".otf")))
        {
            continue;
        }
        ret = snprintf(path, MAX_PATH, "%s/%s", prefix, fileData.cFileName);
        if (ret > 0 && ret < MAX_PATH) {
            //printf("WIN32-FONT-TEST: %s\n", path);
            ret = AddFontResourceExA(path, FR_PRIVATE, NULL);
            //if (ret > 0) {
            //    printf("\tLOADED FONT OK!\n");
            //}
        }
    } while (FindNextFileA(fileHandle, &fileData) != 0);
}
#endif


int fontforge_main( int argc, char **argv ) {
    extern const char *source_modtime_str;
    extern const char *source_version_str;
    const char *load_prefs = getenv("FONTFORGE_LOADPREFS");
    int i;
    int recover=2;
    int any;
    int next_recent=0;
    GRect pos;
    GWindowAttrs wattrs;
    char *display = NULL;
    FontRequest rq;
    int ds, ld;
    int openflags=0;
    int doopen=0, quit_request=0;
    bool use_cairo = true;

#if !(GLIB_CHECK_VERSION(2, 35, 0))
    g_type_init();
#endif

    /* Must be done before we cache the current directory */
    /* Change to HOME dir if specified on the commandline */
    for ( i=1; i<argc; ++i ) {
	char *pt = argv[i];
	if ( pt[0]=='-' && pt[1]=='-' ) ++pt;
	if (strcmp(pt,"-home")==0 || strncmp(pt,"-psn_",5)==0) {
	    /* OK, I don't know what _-psn_ means, but to GW it means */
	    /* we've been started on the mac from the FontForge.app   */
	    /* structure, and the current directory is (shudder) "/"  */
	    if (getenv("HOME")!=NULL) chdir(getenv("HOME"));
	    break;	/* Done - Unnecessary to check more arguments */
	}
	if (strcmp(pt,"-quiet")==0)
	    quiet = 1;
    }

    if (!quiet) {
        fprintf( stderr, "Copyright (c) 2000-2014 by George Williams. See AUTHORS for Contributors.\n" );
        fprintf( stderr, " License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n" );
        fprintf( stderr, " with many parts BSD <http://fontforge.org/license.html>. Please read LICENSE.\n" );
        fprintf( stderr, " Based on sources from %s"
	        "-ML"
#ifdef FREETYPE_HAS_DEBUGGER
	        "-TtfDb"
#endif
#ifdef _NO_PYTHON
	        "-NoPython"
#endif
#ifdef FONTFORGE_CONFIG_USE_DOUBLE
	        "-D"
#endif
	        ".\n",
	        FONTFORGE_MODTIME_STR );
        fprintf( stderr, " Based on source from git with hash: %s\n", FONTFORGE_GIT_VERSION );
    }

#if defined(__Mac)
    /* Start X if they haven't already done so. Well... try anyway */
    /* Must be before we change DYLD_LIBRARY_PATH or X won't start */
    /* (osascript depends on a libjpeg which isn't found if we look in /sw/lib first */
    int local_x = uses_local_x(argc,argv);
    if ( local_x==1 && getenv("DISPLAY")==NULL ) {
	/* Don't start X if we're just going to quit. */
	/* if X exists, it isn't needed. If X doesn't exist it's wrong */
	if ( !hasquit(argc,argv)) {
	    /* This sequence is supposed to bring up an app without a window */
	    /*  but X still opens an xterm */
	    system( "osascript -e 'tell application \"X11\" to launch'" );
	    system( "osascript -e 'tell application \"X11\" to activate'" );
	}
	setenv("DISPLAY",":0.0",0);
    } else if ( local_x==1 && *getenv("DISPLAY")!='/' && strcmp(getenv("DISPLAY"),":0.0")!=0 && strcmp(getenv("DISPLAY"),":0")!=0 )
	/* 10.5.7 uses a named socket or something "/tmp/launch-01ftWX:0" */
	local_x = 0;
#endif

#if defined(__MINGW32__)
    if( getenv("DISPLAY")==NULL ) {
	putenv("DISPLAY=127.0.0.1:0.0");
    }
    if( getenv("LC_ALL")==NULL ){
	char lang[8];
	char env[32];
	if( GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, lang, 8) > 0 ){
	    strcpy(env, "LC_ALL=");
	    strcat(env, lang);
	    putenv(env);
	}
    }
#endif

    FF_SetUiInterface(&gdraw_ui_interface);
    FF_SetPrefsInterface(&gdraw_prefs_interface);
    FF_SetSCInterface(&gdraw_sc_interface);
    FF_SetCVInterface(&gdraw_cv_interface);
    FF_SetBCInterface(&gdraw_bc_interface);
    FF_SetFVInterface(&gdraw_fv_interface);
    FF_SetFIInterface(&gdraw_fi_interface);
    FF_SetMVInterface(&gdraw_mv_interface);
    FF_SetClipInterface(&gdraw_clip_interface);
#ifndef _NO_PYTHON
    PythonUI_Init();
#endif

    FindProgDir(argv[0]);
    InitSimpleStuff();

#if defined(__MINGW32__)
    {
        char path[MAX_PATH];
        unsigned int len = GetModuleFileNameA(NULL, path, MAX_PATH);
        path[len] = '\0';
        
        //The '.exe' must be removed as resources presumes it's not there.
        GResourceSetProg(GFileRemoveExtension(GFileNormalizePath(path)));
    }
#else
    GResourceSetProg(argv[0]);
#endif

#if defined(__Mac)
    /* The mac seems to default to the "C" locale, LANG and LC_MESSAGES are not*/
    /*  defined. This means that gettext will not bother to look up any message*/
    /*  files -- even if we have a "C" or "POSIX" entry in the locale diretory */
    /* Now if X11 gives us the command key, I want to force a rebinding to use */
    /*  Cmd rather than Control key -- more mac-like. But I can't do that if   */
    /*  there is no locale. So I force a locale if there is none specified */
    /* I force the US English locale, because that's the what the messages are */
    /*  by default so I'm changing as little as I can. I think. */
    /* Now the locale command will treat a LANG which is "" as undefined, but */
    /*  gettext will not. So I don't bother to check for null strings or "C"  */
    /*  or "POSIX". If they've mucked with the locale perhaps they know what  */
    /*  they are doing */
    {
	int did_keybindings = 0;
	int useCommandKey = get_mac_x11_prop("enable_key_equivalents") <= 0;

	if ( local_x && useCommandKey ) {
	    hotkeySystemSetCanUseMacCommand( 1 );

	    /* Ok, we get the command key */
	    if ( getenv("LANG")==NULL && getenv("LC_MESSAGES")==NULL ) {
		setenv("LC_MESSAGES","en_US.UTF-8",0);
	    }
	    /* Can we find a set of keybindings designed for the mac with cmd key? */
	    bind_textdomain_codeset("Mac-FontForge-MenuShortCuts","UTF-8");
	    bindtextdomain("Mac-FontForge-MenuShortCuts", getLocaleDir());
	    if ( *dgettext("Mac-FontForge-MenuShortCuts","Flag0x10+")!='F' ) {
		GMenuSetShortcutDomain("Mac-FontForge-MenuShortCuts");
		did_keybindings = 1;
	    }
	}
	if ( !did_keybindings ) {
	    /* Nope. we can't. Fall back to the normal stuff */
#endif
	    GMenuSetShortcutDomain("FontForge-MenuShortCuts");
	    bind_textdomain_codeset("FontForge-MenuShortCuts","UTF-8");
	    bindtextdomain("FontForge-MenuShortCuts", getLocaleDir());
#if defined(__Mac)
	}
    }
#endif
    bind_textdomain_codeset("FontForge","UTF-8");
    bindtextdomain("FontForge", getLocaleDir());
    textdomain("FontForge");
    GResourceUseGetText();
    {
	char shareDir[PATH_MAX];
	char* sd = getShareDir();
	strncpy( shareDir, sd, PATH_MAX );
    shareDir[PATH_MAX-1] = '\0';
	if(!sd) {
	    strcpy( shareDir, SHAREDIR );
	}

	char path[PATH_MAX];
	snprintf(path, PATH_MAX, "%s%s", shareDir, "/pixmaps" );
	GGadgetSetImageDir( path );

	snprintf(path, PATH_MAX, "%s%s", shareDir, "/resources/fontforge.resource" );
	GResourceAddResourceFile(path, GResourceProgramName,false);
    }
    hotkeysLoad();
//    loadPrefsFiles();
    Prefs_LoadDefaultPreferences();

    if ( load_prefs!=NULL && strcasecmp(load_prefs,"Always")==0 )
	LoadPrefs();
    if ( default_encoding==NULL )
	default_encoding=FindOrMakeEncoding("ISO8859-1");
    if ( default_encoding==NULL )
	default_encoding=&custom;	/* In case iconv is broken */

    // This no longer starts embedded Python unless control passes to the Python executors,
    // which exit independently rather than returning here.
    CheckIsScript(argc,argv); /* Will run the script and exit if it is a script */
					/* If there is no UI, there is always a script */
			                /*  and we will never return from the above */
    if ( load_prefs==NULL ||
	    (strcasecmp(load_prefs,"Always")!=0 &&	/* Already loaded */
	     strcasecmp(load_prefs,"Never")!=0 ))
	LoadPrefs();
    GrokNavigationMask();
    for ( i=1; i<argc; ++i ) {
	char *pt = argv[i];
	if ( pt[0]=='-' && pt[1]=='-' )
	    ++pt;
	if ( strcmp(pt,"-sync")==0 )
	    GResourceAddResourceString("Gdraw.Synchronize: true",argv[0]);
	else if ( strcmp(pt,"-depth")==0 && i<argc-1 )
	    AddR(argv[0],"Gdraw.Depth", argv[++i]);
	else if ( strcmp(pt,"-vc")==0 && i<argc-1 )
	    AddR(argv[0],"Gdraw.VisualClass", argv[++i]);
	else if ( (strcmp(pt,"-cmap")==0 || strcmp(pt,"-colormap")==0) && i<argc-1 )
	    AddR(argv[0],"Gdraw.Colormap", argv[++i]);
	else if ( (strcmp(pt,"-dontopenxdevices")==0) )
	    AddR(argv[0],"Gdraw.DontOpenXDevices", "true");
	else if ( strcmp(pt,"-keyboard")==0 && i<argc-1 )
	    AddR(argv[0],"Gdraw.Keyboard", argv[++i]);
	else if ( strcmp(pt,"-display")==0 && i<argc-1 )
	    display = argv[++i];
# if MyMemory
	else if ( strcmp(pt,"-memory")==0 )
	    __malloc_debug(5);
# endif
	else if ( strncmp(pt,"-usecairo",strlen("-usecairo"))==0 ) {
	    if ( strcmp(pt,"-usecairo=no")==0 )
	        use_cairo = false;
	    else
	        use_cairo = true;
	    GDrawEnableCairo(use_cairo);
	} else if ( strcmp(pt,"-nosplash")==0 )
	    splash = 0;
	else if ( strcmp(pt,"-quiet")==0 )
	    /* already checked for this earlier, no need to do it again */;
	else if ( strcmp(pt,"-unique")==0 )
	    unique = 1;
	else if ( strcmp(pt,"-forceuihidden")==0 )
	    cmdlinearg_forceUIHidden = 0;
	else if ( strcmp(pt,"-recover")==0 && i<argc-1 ) {
	    ++i;
	    if ( strcmp(argv[i],"none")==0 )
		recover=0;
	    else if ( strcmp(argv[i],"clean")==0 )
		recover= -1;
	    else if ( strcmp(argv[i],"auto")==0 )
		recover= 1;
	    else if ( strcmp(argv[i],"inquire")==0 )
		recover= 2;
	    else {
		fprintf( stderr, "Invalid argument to -recover, must be none, auto, inquire or clean\n" );
		dousage();
	    }
	} else if ( strcmp(pt,"-recover=none")==0 ) {
	    recover = 0;
	} else if ( strcmp(pt,"-recover=clean")==0 ) {
	    recover = -1;
	} else if ( strcmp(pt,"-recover=auto")==0 ) {
	    recover = 1;
	} else if ( strcmp(pt,"-recover=inquire")==0 ) {
	    recover = 2;
	} else if ( strcmp(pt,"-docs")==0 )
	    dohelp();
	else if ( strcmp(pt,"-help")==0 )
	    dousage();
	else if ( strcmp(pt,"-version")==0 || strcmp(pt,"-v")==0 || strcmp(pt,"-V")==0 )
	    doversion(FONTFORGE_MODTIME_STR);
	else if ( strcmp(pt,"-quit")==0 )
	    quit_request = true;
	else if ( strcmp(pt,"-home")==0 )
	    /* already did a chdir earlier, don't need to do it again */;
#if defined(__Mac)
	else if ( strncmp(pt,"-psn_",5)==0 ) {
	    /* OK, I don't know what _-psn_ means, but to GW it means */
	    /* we've been started on the mac from the FontForge.app   */
	    /* structure, and the current directory was (shudder) "/" */
	    /* (however, we changed to HOME earlier in main routine). */
	    unique = 1;
	    listen_to_apple_events = true; // This has been problematic on Mavericks and later.
	}
#endif
    }

    ensureDotFontForgeIsSetup();
#if defined(__MINGW32__) && !defined(_NO_LIBCAIRO)
    //Load any custom fonts for the user interface
    if (use_cairo) {
        char *system_load = getGResourceProgramDir();
        char *user_load = getFontForgeUserDir(Data);
        char lbuf[MAX_PATH];
        int lret;

        if (system_load != NULL) {
            //Follow the FontConfig APPSHAREFONTDIR location
            lret = snprintf(lbuf, MAX_PATH, "%s/../share/fonts", system_load);
            if (lret > 0 && lret < MAX_PATH) {
                WinLoadUserFonts(lbuf);
            }
        }
        if (user_load != NULL) {
            lret = snprintf(lbuf, MAX_PATH, "%s/%s", user_load, "ui-fonts");
            if (lret > 0 && lret < MAX_PATH) {
                WinLoadUserFonts(lbuf);
            }
            free(user_load);
        }
    }
#endif
    InitImageCache(); // This is in gtextinfo.c. It zeroes imagecache for us.
    atexit(&ClearImageCache); // We register the destructor, which is also in gtextinfo.c.
    GDrawCreateDisplays(display,argv[0]);
    atexit(&GDrawDestroyDisplays); // We register the destructor so that it runs even if we call exit without finishing this function.
    default_background = GDrawGetDefaultBackground(screen_display);
    InitToolIconClut(default_background);
    InitToolIcons();
    InitCursors();

    /**
     * we have to do a quick sniff of argv[] here to see if the user
     * wanted to skip loading these python init files.
     */
    for ( i=1; i<argc; ++i ) {
	char *pt = argv[i];

	if ( !strcmp(pt,"-SkipPythonInitFiles")) {
	    ProcessPythonInitFiles = 0;
	}
    }
    
#ifndef _NO_PYTHON
/*# ifndef GWW_TEST*/
    FontForge_InitializeEmbeddedPython(); /* !!!!!! debug (valgrind doesn't like python) */
/*# endif*/
#endif

#ifndef _NO_PYTHON
    if( ProcessPythonInitFiles )
	PyFF_ProcessInitFiles();
#endif

    /* the splash screen used not to have a title bar (wam_nodecor) */
    /*  but I found I needed to know how much the window manager moved */
    /*  the window around, which I can determine if I have a positioned */
    /*  decorated window created at the begining */
    /* Actually I don't care any more */
    wattrs.mask = wam_events|wam_cursor|wam_bordwidth|wam_backcol|wam_positioned|wam_utf8_wtitle|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.positioned = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = "FontForge";
    wattrs.border_width = 2;
    wattrs.background_color = 0xffffff;
    wattrs.is_dlg = !listen_to_apple_events;
    pos.x = pos.y = 200;
    pos.width = splashimage.u.image->width;
    pos.height = splashimage.u.image->height-56;		/* 54 */
    GDrawBindSelection(NULL,sn_user1,"FontForge");
    if ( unique && GDrawSelectionOwned(NULL,sn_user1)) {
	/* Different event handler, not a dialog */
	wattrs.is_dlg = false;
	splashw = GDrawCreateTopWindow(NULL,&pos,request_e_h,NULL,&wattrs);
	PingOtherFontForge(argc,argv);
    } else {
	if ( quit_request )
exit( 0 );
	splashw = GDrawCreateTopWindow(NULL,&pos,splash_e_h,NULL,&wattrs);
    }

    memset(&rq,0,sizeof(rq));
    rq.utf8_family_name = SERIF_UI_FAMILIES;
    rq.point_size = 12;
    rq.weight = 400;
    splash_font = GDrawInstanciateFont(NULL,&rq);
    splash_font = GResourceFindFont("Splash.Font",splash_font);
    GDrawDecomposeFont(splash_font, &rq);
    rq.style = fs_italic;
    splash_italic = GDrawInstanciateFont(NULL,&rq);
    splash_italic = GResourceFindFont("Splash.ItalicFont",splash_italic);
    GDrawSetFont(splashw,splash_font);

    SplashLayout();
    localsplash = splash;

   if ( localsplash && !listen_to_apple_events )
	start_splash_screen();

    //
    // The below call will initialize the fontconfig cache if required.
    // That can take a while the first time it happens.
    //
   GDrawWindowFontMetrics(splashw,splash_font,&as,&ds,&ld);
   fh = as+ds+ld;

    if ( AutoSaveFrequency>0 )
	autosave_timer=GDrawRequestTimer(splashw,2*AutoSaveFrequency*1000,AutoSaveFrequency*1000,NULL);

    GDrawProcessPendingEvents(NULL);
    GDrawSetBuildCharHooks(BuildCharHook,InsCharHook);

    any = 0;
    if ( recover==-1 )
	CleanAutoRecovery();
    else if ( recover )
	any = DoAutoRecoveryExtended( recover-1 );
			
    openflags = 0;
    for ( i=1; i<argc; ++i ) {
	char buffer[1025];
	char *pt = argv[i];

	GDrawProcessPendingEvents(NULL);
	if ( pt[0]=='-' && pt[1]=='-' && pt[2]!='\0')
	    ++pt;
	if ( strcmp(pt,"-new")==0 ) {
	    FontNew();
	    any = 1;
#  if HANYANG
	} else if ( strcmp(pt,"-newkorean")==0 ) {
	    MenuNewComposition(NULL,NULL,NULL);
	    any = 1;
#  endif
	} else if ( !strcmp(pt,"-SkipPythonInitFiles")) {
	    // already handled above.
	} else if ( strcmp(pt,"-last")==0 ) {
	    if ( next_recent<RECENT_MAX && RecentFiles[next_recent]!=NULL )
		if ( ViewPostScriptFont(RecentFiles[next_recent++],openflags))
		    any = 1;
	} else if ( strcmp(pt,"-sync")==0 || strcmp(pt,"-memory")==0 ||
		    strcmp(pt,"-nosplash")==0 || strcmp(pt,"-recover=none")==0 ||
		    strcmp(pt,"-recover=clean")==0 || strcmp(pt,"-recover=auto")==0 ||
		    strcmp(pt,"-dontopenxdevices")==0 || strcmp(pt,"-unique")==0 ||
		    strncmp(pt,"-usecairo",strlen("-usecairo"))==0 ||
		    strcmp(pt,"-home")==0 || strcmp(pt,"-quiet")==0
		    || strcmp(pt,"-forceuihidden")==0 )
	    /* Already done, needed to be before display opened */;
	else if ( strncmp(pt,"-psn_",5)==0 )
	    /* Already done */;
	else if ( (strcmp(pt,"-depth")==0 || strcmp(pt,"-vc")==0 ||
		    strcmp(pt,"-cmap")==0 || strcmp(pt,"-colormap")==0 ||
		    strcmp(pt,"-keyboard")==0 ||
		    strcmp(pt,"-display")==0 || strcmp(pt,"-recover")==0 ) &&
		i<argc-1 )
	    ++i; /* Already done, needed to be before display opened */
	else if ( strcmp(pt,"-allglyphs")==0 )
	    openflags |= of_all_glyphs_in_ttc;
	else if ( strcmp(pt,"-open")==0 )
	    doopen = true;
	else {
	    printf("else argv[i]:%s\n", argv[i] );
	    if ( strstr(argv[i],"://")!=NULL ) {		/* Assume an absolute URL */
		strncpy(buffer,argv[i],sizeof(buffer));
		buffer[sizeof(buffer)-1]= '\0';
	    } else
		GFileGetAbsoluteName(argv[i],buffer,sizeof(buffer));
	    if ( GFileIsDir(buffer) || (strstr(buffer,"://")!=NULL && buffer[strlen(buffer)-1]=='/')) {
		char *fname;
		fname = malloc(strlen(buffer)+strlen("/glyphs/contents.plist")+1);
		strcpy(fname,buffer); strcat(fname,"/glyphs/contents.plist");
		if ( GFileExists(fname)) {
		    /* It's probably a Unified Font Object directory */
		    free(fname);
		    if ( ViewPostScriptFont(buffer,openflags) )
			any = 1;
		} else {
		    strcpy(fname,buffer); strcat(fname,"/font.props");
		    if ( GFileExists(fname)) {
			/* It's probably a sf dir collection */
			free(fname);
			if ( ViewPostScriptFont(buffer,openflags) )
			    any = 1;
		    } else {
			free(fname);
			if ( buffer[strlen(buffer)-1]!='/' ) {
			    /* If dirname doesn't end in "/" we'll be looking in parent dir */
			    buffer[strlen(buffer)+1]='\0';
			    buffer[strlen(buffer)] = '/';
			}
			fname = GetPostScriptFontName(buffer,false);
			if ( fname!=NULL )
			    ViewPostScriptFont(fname,openflags);
			any = 1;	/* Even if we didn't get a font, don't bring up dlg again */
			free(fname);
		    }
		}
	    } else if ( ViewPostScriptFont(buffer,openflags)!=0 )
		any = 1;
	}
    }
    if ( !any && !doopen )
	any = ReopenLastFonts();

    collabclient_ensureClientBeacon();
    collabclient_sniffForLocalServer();
#ifndef _NO_PYTHON
    PythonUI_namedpipe_Init();
#endif

#if defined(__Mac)
    if ( listen_to_apple_events ) {
	install_apple_event_handlers();
	install_mac_timer();
	setup_cocoa_app();

	
	// WARNING: See declaration of RunApplicationEventLoop() above as to
	// why you might not want to call that function anymore.
	// RunApplicationEventLoop();
	
    } else
#endif
    if ( doopen || !any )
	_FVMenuOpen(NULL);
    GDrawEventLoop(NULL);
    GDrawDestroyDisplays();

#ifndef _NO_PYTHON
/*# ifndef GWW_TEST*/
    FontForge_FinalizeEmbeddedPython(); /* !!!!!! debug (valgrind doesn't like python) */
/*# endif*/
#endif

    // These free menu translations, mostly.
    BitmapViewFinishNonStatic();
    MetricsViewFinishNonStatic();
    CharViewFinishNonStatic();
    FontViewFinishNonStatic();

    ClearImageCache(); // This frees the contents of imagecache.
    hotkeysSave();
    LastFonts_Save();

#ifndef _NO_LIBUNICODENAMES
    uninm_names_db_close(names_db);	/* close this database before exiting */
    uninm_blocks_db_close(blocks_db);
#endif

    lt_dlexit();

return( 0 );
}
