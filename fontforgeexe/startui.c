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
#include <fontforge-version-extras.h>

#include "autosave.h"
#include "bitmapchar.h"
#include "clipnoui.h"
#include "encoding.h"
#include "ffgdk.h"
#include "ffglib.h"
#include "fontforgeui.h"
#include "gfile.h"
#include "gresedit.h"
#include "hotkeys.h"
#include "lookups.h"
#include "prefs.h"
#include "start.h"
#include "ustring.h"

#include <locale.h>
#include <stdlib.h>		/* getenv,setenv */
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#if defined(__MINGW32__)
#include <windows.h>
#define sleep(n) Sleep(1000 * (n))
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

// Must be included after png.h because it messes with setjmp
#include "scripting.h"

extern int AutoSaveFrequency;
int splash = 1;
static int localsplash;
static int unique = 0;

static void _dousage(void) {
    printf( "fontforge [options] [fontfiles]\n" );
    printf( "\t-new\t\t\t (creates a new font)\n" );
    printf( "\t-last\t\t\t (loads the last sfd file closed)\n" );
#if HANYANG
    printf( "\t-newkorean\t\t (creates a new korean font)\n" );
#endif
    printf( "\t-recover none|auto|inquire|clean (control error recovery)\n" );
    printf( "\t-allglyphs\t\t (load all glyphs in the 'glyf' table\n\t\t\t\t  of a truetype collection)\n" );
    printf( "\t-nosplash\t\t (no splash screen)\n" );
    printf( "\t-quiet\t\t\t (don't print non-essential\n\t\t\t\t  information to stderr)\n" );
    printf( "\t-unique\t\t\t (if a fontforge is already running open all\n\t\t\t\t  arguments in it and have this process exit)\n" );
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
#ifndef _NO_PYTHON
    printf( "\t-lang=py\t\t (use python for scripts (may precede -script))\n" );
#endif
#ifndef _NO_FFSCRIPT
    printf( "\t-lang=ff\t\t (use fontforge's legacy scripting language)\n" );
#endif
    printf( "\t-script scriptfile\t (executes scriptfile)\n" );
    printf( "\t\tmust be the first option (or follow -lang).\n" );
    printf( "\t\tAll others passed to scriptfile.\n" );
    printf( "\t-dry scriptfile\t\t (syntax checks scriptfile)\n" );
    printf( "\t\tmust be the first option. All others passed to scriptfile.\n" );
    printf( "\t\tOnly for fontforge's own scripting language, not python.\n" );
    printf( "\t-c script-string\t (executes argument as scripting cmds)\n" );
    printf( "\t\tmust be the first option. All others passed to the script.\n" );
    printf( "\t-skippyfile\t\t (do not execute python init scripts)\n" );
    printf( "\t-skippyplug\t\t (do not load python plugins)\n" );
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
    help("index.html", NULL);
exit(0);
}

struct delayed_event {
    void *data;
    void (*func)(void *);
};

extern GImage splashimage_legacy;
static GWindow splashw;
static GTimer *autosave_timer, *splasht;
GResFont splash_font = GRESFONT_INIT("400 10pt " SERIF_UI_FAMILIES);
GResFont splash_monofont = GRESFONT_INIT("400 10pt " MONO_UI_FAMILIES);
GResFont splash_italicfont = GRESFONT_INIT("400 10pt italic " SERIF_UI_FAMILIES);
GResImage splashresimage = GRESIMAGE_INIT("splash2019.png");
Color splashbg = 0xffffff;
Color splashfg = 0x000000;
GImage *splashimagep;
static int as,fh, linecnt;
static unichar_t msg[546];
static unichar_t *lines[32], *is, *ie;

static void SplashImageInit() {
    MiscWinInit();
    if (splashimagep == NULL)
        splashimagep = GResImageGetImage(&splashresimage);
    if (splashimagep == NULL)
	splashimagep = &splashimage_legacy;
    return;
}

void *_SplashResImageSet(char *res, void *def) {
    // Image was already set; this is a hook for side effects
    GImage *i = GResImageGetImage((GResImage *)def);
    if ( i!=NULL )
	splashimagep = i;
    return NULL;
}

void ShowAboutScreen(void) {
    static int first=1;

    if ( first ) {
	GDrawResize(splashw,splashimagep->u.image->width,splashimagep->u.image->height+linecnt*fh);
	first = false;
    }
    
    if ( splasht!=NULL )
    GDrawCancelTimer(splasht);
    splasht=NULL;

    GDrawSetVisible(splashw,true);
}

static void SplashLayout() {
    unichar_t *start, *pt, *lastspace;

    SplashImageInit();

    u_strcpy(msg, utf82u_copy("As he drew closer to completing his book on Renaissance printing (The Craft of Printing and the Publication of Shakespeare’s Works), George Williams IV suggested that his son, George Williams V, write a chapter on computer typography. FontForge—previously called PfaEdit—was his response."));

    GDrawSetFont(splashw,splash_font.fi);
    linecnt = 0;
    lines[linecnt++] = msg-1;
    for ( start = msg; *start!='\0'; start = pt ) {
	lastspace = NULL;
	for ( pt=start; ; ++pt ) {
	    if ( *pt==' ' || *pt=='\0' ) {
		if ( GDrawGetTextWidth(splashw,start,pt-start)<splashimagep->u.image->width-10 )
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

    uc_strcpy(pt," ");
    pt += u_strlen(pt);
    lines[linecnt++] = pt;

    uc_strcpy(pt, " As of 2012 FontForge development continues on GitHub.");
    pt += u_strlen(pt);
    lines[linecnt++] = pt;

    uc_strcpy(pt," ");
    pt += u_strlen(pt);
    lines[linecnt++] = pt;

    uc_strcpy(pt," Version:");
    uc_strcat(pt,FONTFORGE_VERSION);
    pt += u_strlen(pt);
    lines[linecnt++] = pt;

    // Can be empty if e.g. building from a tarball
    if (FONTFORGE_GIT_VERSION[0] != '\0') {
	uc_strcpy(pt,"  ");
	uc_strcat(pt, FONTFORGE_GIT_VERSION);
	pt += u_strlen(pt);
	lines[linecnt++] = pt;
    }

    uc_strcat(pt," Built: ");
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
#ifdef FONTFORGE_CAN_USE_GDK
    uc_strcat(pt, "-GDK3");
#else
    uc_strcat(pt,"-X11");
#endif
    pt += u_strlen(pt);
    lines[linecnt++] = pt;
    lines[linecnt] = NULL;
    is = u_strchr(msg,'(')+1;
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

    splasht = GDrawRequestTimer(splashw,7000,1000,NULL);

    localsplash = false;
}

static int splash_e_h(GWindow gw, GEvent *event) {
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
	GDrawDrawImage(gw,splashimagep,NULL,0,0);
	if ((event->u.expose.rect.y+event->u.expose.rect.height) > splashimagep->u.image->height) {
	    GDrawSetFont(gw,splash_font.fi);
	    y = splashimagep->u.image->height + as + fh/2;
	    for ( i=1; i<linecnt; ++i ) {
	    // The number 10 comes from lines[linecnt] created in the function SplashLayout. It refers
	    // to the line at which we want to make the font monospace. If you add or remove a line, 
	    // you will need to change this.
	    if (i == 10) {
		    x = 8+GDrawDrawText(gw,8,y,lines[i-1]+1,0,splashfg);
		    GDrawSetFont(gw,splash_monofont.fi);
	    GDrawDrawText(gw,8,y,lines[i-1]+1,lines[i]-lines[i-1]-1,splashfg);
	    } else if ( is>=lines[i-1]+1 && is<lines[i] ) {
		    x = 8+GDrawDrawText(gw,8,y,lines[i-1]+1,is-lines[i-1]-1,splashfg);
		    GDrawSetFont(gw,splash_italicfont.fi);
		    GDrawDrawText(gw,x,y,is,lines[i]-is,splashfg);
		} else if ( ie>=lines[i-1]+1 && ie<lines[i] ) {
		    x = 8+GDrawDrawText(gw,8,y,lines[i-1]+1,ie-lines[i-1]-1,splashfg);
		    GDrawSetFont(gw,splash_font.fi);
		    GDrawDrawText(gw,x,y,ie,lines[i]-ie,splashfg);
		} else
		    GDrawDrawText(gw,8,y,lines[i-1]+1,lines[i]-lines[i-1]-1,splashfg);
		y += fh;
	    }
	}
	GDrawPopClip(gw,&old);
      break;
      case et_map:
	// The splash screen used to gradually resize the longer it was displayed.
	// This was removed. But there is a gxdraw bug which prevents
	// the splash from being displayed properly unless a resize occurs.
	// So this forces a resize to make it display properly...
	GDrawGetSize(gw, &old);
	if (old.height < splashimagep->u.image->height) {
	    GDrawResize(gw,splashimagep->u.image->width,splashimagep->u.image->height);
	}
	break;
      case et_timer:
      if ( event->u.timer.timer==autosave_timer ) {
          DoAutoSaves();
      } else if ( event->u.timer.timer==splasht ) {
          GGadgetEndPopup();
          GDrawSetVisible(gw,false);
          GDrawCancelTimer(splasht);
          splasht = NULL;
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
      default: break;
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
    GFileMkDir( buffer, mode );
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
    const char *load_prefs = getenv("FONTFORGE_LOADPREFS");
    int i;
    int recover=2;
    int any;
    int next_recent=0;
    int run_python_init_files = true;
    int import_python_plugins = true;
    GRect pos;
    GWindowAttrs wattrs;
    char *display = NULL;
    int ds, ld;
    int openflags=0;
    int doopen=0, quit_request=0;
    bool use_cairo = true;

#if !(GLIB_CHECK_VERSION(2, 35, 0))
    g_type_init();
#endif

#ifdef __Mac
    extern void setup_cocoa_app(void);
    setup_cocoa_app();
    hotkeySystemSetCanUseMacCommand(true);
#endif

    /* Must be done before we cache the current directory */
    /* Change to HOME dir if specified on the commandline */
    for ( i=1; i<argc; ++i ) {
	char *pt = argv[i];
	if ( pt[0]=='-' && pt[1]=='-' ) ++pt;
	if (strcmp(pt,"-home")==0) {
	    if (getenv("HOME")!=NULL) chdir(getenv("HOME"));
	    break;	/* Done - Unnecessary to check more arguments */
	}
	if (strcmp(pt,"-quiet")==0)
	    quiet = 1;
    }

    if (!quiet) {
        time_t tm = FONTFORGE_MODTIME_RAW;
        struct tm* modtime = gmtime(&tm);
        fprintf( stderr, "Copyright (c) 2000-%d. See AUTHORS for Contributors.\n", modtime->tm_year+1900 );
        fprintf( stderr, " License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n" );
        fprintf( stderr, " with many parts BSD <http://fontforge.org/license.html>. Please read LICENSE.\n" );
        fprintf( stderr, " Version: %s\n", FONTFORGE_VERSION );
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
#ifdef FONTFORGE_CAN_USE_GDK
            "-GDK3"
#endif
#ifdef BUILT_WITH_XORG
            "-Xorg"
#endif
	        ".\n",
	        FONTFORGE_MODTIME_STR );
        // Can be empty if e.g. building from a tarball
        if (FONTFORGE_GIT_VERSION[0] != '\0') {
            fprintf( stderr, " Based on source from git with hash: %s\n", FONTFORGE_GIT_VERSION );
        }
    }

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

    FindProgRoot(argv[0]);
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

    GMenuSetShortcutDomain("FontForge-MenuShortCuts");
    bind_textdomain_codeset("FontForge-MenuShortCuts","UTF-8");
    bindtextdomain("FontForge-MenuShortCuts", getLocaleDir());

    bind_textdomain_codeset("FontForge","UTF-8");
    bindtextdomain("FontForge", getLocaleDir());
    textdomain("FontForge");
    {
	GGadgetSetImageDir( getPixmapDir() );
	char* path = smprintf("%s/resources/fontforge.resource", getShareDir());
	GResourceAddResourceFile(path, GResourceProgramName,false);
	free(path);
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
	    doversion(FONTFORGE_VERSION);
	else if ( strcmp(pt,"-quit")==0 )
	    quit_request = true;
	else if ( strcmp(pt,"-home")==0 )
	    /* already did a chdir earlier, don't need to do it again */;
    }
#ifdef FONTFORGE_CAN_USE_GDK
    gdk_set_allowed_backends("win32,quartz,x11");
    gdk_init(&argc, &argv);
#endif
    ensureDotFontForgeIsSetup();
#if defined(__MINGW32__) && !defined(_NO_LIBCAIRO)
    //Load any custom fonts for the user interface
    if (use_cairo) {
        const char *system_load = getShareDir();
        char *user_load = getFontForgeUserDir(Data);
        char lbuf[MAX_PATH];
        int lret;

        if (system_load != NULL) {
            //Follow the FontConfig APPSHAREFONTDIR location
            lret = snprintf(lbuf, MAX_PATH, "%s/../fonts", system_load);
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

	if ( strcmp(pt,"-SkipPythonInitFiles")==0 || strcmp(pt,"-skippyfile")==0 ) {
	    run_python_init_files = false;
	} else if ( strcmp(pt,"-skippyplug")==0 ) {
	    import_python_plugins = false;
	}
    }

    // Silence unused warnings, depending on ifdefs
    (void)run_python_init_files;
    (void)import_python_plugins;

#ifndef _NO_PYTHON
/*# ifndef GWW_TEST*/
    FontForge_InitializeEmbeddedPython(); /* !!!!!! debug (valgrind doesn't like python) */
/*# endif*/
#endif

#ifndef _NO_PYTHON
    PyFF_ProcessInitFiles(run_python_init_files, import_python_plugins);
#endif

    /* the splash screen used not to have a title bar (wam_nodecor) */
    /*  but I found I needed to know how much the window manager moved */
    /*  the window around, which I can determine if I have a positioned */
    /*  decorated window created at the beginning */
    /* Actually I don't care any more */
    wattrs.mask = wam_events|wam_cursor|wam_bordwidth|wam_backcol|wam_positioned|wam_utf8_wtitle|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.positioned = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = "FontForge";
    wattrs.border_width = 2;
    wattrs.background_color = splashbg;
#ifdef FONTFORGE_CAN_USE_GDK
    wattrs.is_dlg = true;
#endif
    pos.x = pos.y = 200;
    SplashImageInit();
    pos.width = splashimagep->u.image->width;
    pos.height = splashimagep->u.image->height-1; // See splash_e_h:et_map
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

    GDrawSetFont(splashw,splash_font.fi);

    SplashLayout();
    localsplash = splash;

   if ( localsplash )
	start_splash_screen();

    //
    // The below call will initialize the fontconfig cache if required.
    // That can take a while the first time it happens.
    //
   GDrawWindowFontMetrics(splashw,splash_font.fi,&as,&ds,&ld);
   fh = as+ds+ld;

    if ( AutoSaveFrequency>0 )
	autosave_timer=GDrawRequestTimer(splashw,2*AutoSaveFrequency*1000,AutoSaveFrequency*1000,NULL);

    GDrawProcessPendingEvents(NULL);

    any = 0;
    if ( recover==-1 )
	CleanAutoRecovery();
    else if ( recover )
	any = DoAutoRecoveryExtended( recover-1 );
			
    openflags = 0;
    for ( i=1; i<argc; ++i ) {
	char *buffer = NULL;
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
	} else if ( strcmp(pt,"-SkipPythonInitFiles")==0 ||
	            strcmp(pt,"-skippyfile")==0 ||
	            strcmp(pt,"-skippyplug")==0 ) {
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
//	    printf("else argv[i]:%s\n", argv[i] );
	    buffer = GFileGetAbsoluteName(argv[i]);
	    if ( GFileIsDir(buffer) ) {
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
			fname = GetPostScriptFontName(buffer,false,false);
			if ( fname!=NULL )
			    ViewPostScriptFont(fname,openflags);
			any = 1;	/* Even if we didn't get a font, don't bring up dlg again */
			free(fname);
		    }
		}
	    } else if ( ViewPostScriptFont(buffer,openflags)!=0 )
		any = 1;
	    free(buffer);
	}
    }
    if ( !any && !doopen )
	any = ReopenLastFonts();

#ifdef __Mac
    extern bool launch_cocoa_app(void);
    any = launch_cocoa_app() || any;
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
    // hotkeysSave();
    LastFonts_Save();

return( 0 );
}
