/* Copyright (C) 2000-2004 by George Williams */
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
#include <gfile.h>
#ifdef FONTFORGE_CONFIG_GDRAW
# include <gresource.h>
# include <ustring.h>
#elif defined( FONTFORGE_CONFIG_GTK )
# include "interface.h"
# include "support.h"
#endif
#include <time.h>
#include <sys/time.h>
#include <locale.h>
#include <unistd.h>
#if !defined(_NO_LIBUNINAMESLIST) && !defined(_STATIC_LIBUNINAMESLIST) && !defined(NODYNAMIC)
#  include <dynamic.h>
#endif

int32 unicode_from_adobestd[256];
struct lconv localeinfo;
char *coord_sep = ",";		/* Not part of locale data */
const struct unicode_nameannot * const *const *_UnicodeNameAnnot = NULL;

static void initadobeenc(void) {
    int i,j;

    for ( i=0; i<0x100; ++i ) {
	if ( strcmp(AdobeStandardEncoding[i],".notdef")==0 )
	    unicode_from_adobestd[i] = 0xfffd;
	else {
	    j = UniFromName(AdobeStandardEncoding[i],ui_none,&custom);
	    if ( j==-1 ) j = 0xfffd;
	    unicode_from_adobestd[i] = j;
	}
    }
}

static void inituninameannot(void) {
#if _NO_LIBUNINAMESLIST
    _UnicodeNameAnnot = NULL;
#elif defined(_STATIC_LIBUNINAMESLIST) || defined(NODYNAMIC)
    extern const struct unicode_nameannot * const * const UnicodeNameAnnot[];
    _UnicodeNameAnnot = UnicodeNameAnnot;
#else
    DL_CONST void *libuninames=NULL;
    const char *loc = getenv("LC_ALL");
# ifdef LIBDIR
    char full[1024], buf[100];
# else
    char buf[100];
#endif
    int i;

    if ( loc==NULL ) loc = getenv("LC_MESSAGES");
    if ( loc==NULL ) loc = getenv("LANG");
    for ( i=0; i<4; ++i ) {
	strcpy(buf,"libuninameslist-");
	if ( i==3 )
	    buf[strlen(buf)-1] = '\0';
	    /* Use the default name */
	else if ( i==2 ) {
	    if ( loc==NULL || strlen( loc )<2 )
    continue;
	    strncat(buf,loc,2);
	} else if ( i==1 ) {
	    if ( loc==NULL || strlen( loc )<5 )
    continue;
	    strncat(buf,loc,5);
	} else if ( i==0 ) {
	    if ( loc==NULL || strlen( loc )<6 )
    continue;
	    strcat(buf,loc);
	}
	strcat(buf, SO_EXT );

# ifdef LIBDIR
#  if !defined(_NO_SNPRINTF) && !defined(VMS)
	snprintf( full, sizeof(full), "%s/%s", LIBDIR, buf );
#  else
	sprintf( full, "%s/%s", LIBDIR, buf );
#  endif
	libuninames = dlopen( full,RTLD_LAZY);
# endif
	if ( libuninames==NULL )
	    libuninames = dlopen( buf,RTLD_LAZY);
	if ( libuninames!=NULL ) {
	    _UnicodeNameAnnot = dlsym(libuninames,"UnicodeNameAnnot");
return;
	}
    }
#endif
}

void doversion(void) {
    extern const char *source_version_str;
    printf( "fontforge %s\n", source_version_str );
exit(0);
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void _dousage(void) {
    printf( "fontforge [options] [fontfiles]\n" );
    printf( "\t-new\t\t\t (creates a new font)\n" );
#if HANYANG
    printf( "\t-newkorean\t\t (creates a new korean font)\n" );
#endif
    printf( "\t-recover none|auto|clean (control error recovery)\n" );
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    printf( "\t-nosplash\t\t (no splash screen)\n" );
#endif
    printf( "\t-display display-name\t (sets the X display)\n" );
#ifdef FONTFORGE_CONFIG_GDRAW
    printf( "\t-depth val\t\t (sets the display depth if possible)\n" );
    printf( "\t-vc val\t\t\t (sets the visual class if possible)\n" );
    printf( "\t-cmap current|copy|private\t (sets the type of colormap)\n" );
    printf( "\t-sync\t\t\t (syncs the display, debugging)\n" );
    printf( "\t-keyboard ibm|mac|sun|ppc  (generates appropriate hotkeys in menus)\n" );
#endif
#if MyMemory
    printf( "\t-memory\t\t\t (turns on memory checks, debugging)\n" );
#endif
    printf( "\t-usage\t\t\t (displays this message, and exits)\n" );
    printf( "\t-help\t\t\t (displays this message, invokes a browser)\n\t\t\t\t  (Using the BROWSER environment variable)\n" );
    printf( "\t-version\t\t (prints the version of fontforge and exits)\n" );
    printf( "\t-script scriptfile\t (executes scriptfile)\n" );
    printf( "\t\tmust be the first option. All others passed to scriptfile.\n" );
    printf( "\n" );
    printf( "FontForge will read postscript (pfa, pfb, ps, cid), opentype (otf),\n" );
    printf( "\ttruetype (ttf,ttc), macintosh resource fonts (dfont,bin,hqx),\n" );
    printf( "\tand bdf and pcf fonts. It will also read it's own format --\n" );
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
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static void initrand(void) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    srand(tv.tv_usec);
}

struct delayed_event {
    void *data;
    void (*func)(void *);
};

#ifdef FONTFORGE_CONFIG_GTK

static guint autosave_timer;

static void splash_window_tooltip_fun( GtkWidget *splashw ) {
    static char *foolishness[] = {
	"A free press discriminates\nagainst the illiterate.",
	"A free press discriminates\nagainst the illiterate.",
	"Gaudeamus Ligature!",
	"Gaudeamus Ligature!",
	"In the beginning was the letter..."
    };
    GtkTooltips *tips;

    tips = gtk_tooltips_new();
    gtk_tooltips_set_tip( tips, splashw, foolishness[rand()%(sizeof(foolishness)/sizeof(foolishness[0]))], NULL );
}

void ShowAboutScreen(void) {
    GtkWidget *splashw, *version;
    char buffer[200];
    extern const char *source_modtime_str;
    extern const char *source_version_str;

    splashw = create_FontForgeSplash ();
    splash_window_tooltip_fun( splashw );
    version = lookup_widget(FontView,"Version");
    if ( version!=NULL ) {
	sprintf( buffer, "Version: %s (%s)", source_modtime_str, source_version_str);
	gtk_label_set_text(GTK_LABEL( version ),buffer);
    }
    gtk_widget_show (splashw);
}

static int DoDelayedEvents(gpointer data) {
    struct delayed_event *info = (struct delayed_event *) data;

    if ( info!=NULL ) {
	(info->func)(info->data);
	chunkfree(info);
    }
return( FALSE );		/* cancel timer */
}

void DelayEvent(void (*func)(void *), void *data) {
    struct delayed_event *info = chunkalloc(sizeof(struct delayed_event));

    info->data = data;
    info->func = func;
    
    gtk_timeout_add(100,DoDelayedEvents,info);
}

static int _DoAutoSaves( gpointer ignored ) {
    DoAutoSaves();
return( TRUE );			/* Continue timer */
}

struct argcontext {
    int argc;
    char **argv;
    int recover;
};

static int ParseArgs( gpointer data ) {
    struct argcontext *args = data;
    int argc = args->argc;
    char **argv = args->argv;
    int recover = args->recover;
    int any, i;
    int next_recent = 0;

    any = 0;
    if ( recover==-1 )
	CleanAutoRecovery();
    else if ( recover )
	any = DoAutoRecovery();

    for ( i=1; i<argc; ++i ) {
	char buffer[1025];
	char *pt = argv[i];

	if ( pt[0]=='-' && pt[1]=='-' )
	    ++pt;
	if ( strcmp(pt,"-new")==0 ) {
	    FontNew();
	    any = 1;
#if HANYANG
	} else if ( strcmp(pt,"-newkorean")==0 ) {
	    MenuNewComposition(NULL,NULL,NULL);
	    any = 1;
#endif
	} else if ( strcmp(pt,"-last")==0 ) {
	    if ( next_recent<RECENT_MAX && RecentFiles[next_recent]!=NULL )
		if ( ViewPostscriptFont(RecentFiles[next_recent++]))
		    any = 1;
	} else if ( strcmp(pt,"-sync")==0 || strcmp(pt,"-memory")==0 ||
		strcmp(pt,"-nosplash")==0 || strcmp(pt,"-recover=none")==0 ||
		strcmp(pt,"-recover=clean")==0 || strcmp(pt,"-recover=auto")==0 )
	    /* Already done, needed to be before display opened */;
	else if ( (strcmp(pt,"-depth")==0 || strcmp(pt,"-vc")==0 ||
		    strcmp(pt,"-cmap")==0 || strcmp(pt,"-colormap")==0 || 
		    strcmp(pt,"-keyboard")==0 || 
		    strcmp(pt,"-display")==0 || strcmp(pt,"-recover")==0 ) &&
		i<argc-1 )
	    ++i; /* Already done, needed to be before display opened */
	else {
	    GFileGetAbsoluteName(argv[i],buffer,sizeof(buffer));
	    if ( GFileIsDir(buffer)) {
		char *fname;
		if ( buffer[strlen(buffer)-1]!='/' ) {
		    /* If dirname doesn't end in "/" we'll be looking in parent dir */
		    buffer[strlen(buffer)+1]='\0';
		    buffer[strlen(buffer)] = '/';
		}
		fname = GetPostscriptFontName(buffer,false);
		if ( fname!=NULL )
		    ViewPostscriptFont(fname);
		any = 1;	/* Even if we didn't get a font, don't bring up dlg again */
		free(fname);
	    } else if ( ViewPostscriptFont(buffer)!=0 )
		any = 1;
	}
    }
    if ( !any )
	MenuOpen(NULL,NULL,NULL);
return( FALSE );	/* Cancel timer */
}

#elif defined( FONTFORGE_CONFIG_GDRAW )
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
static unichar_t msg[350];
static unichar_t *lines[20], *is, *ie;

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
		if ( GDrawGetTextWidth(splashw,start,pt-start,NULL)<splashimage.u.image->width-10 )
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
    uc_strcpy(pt,"  Version: ");;
    uc_strcat(pt,source_modtime_str);
    uc_strcat(pt," (");
    uc_strcat(pt,source_version_str);
    uc_strcat(pt,")");
    lines[linecnt++] = pt+u_strlen(pt);
    lines[linecnt] = NULL;
    is = u_strchr(msg,'(');
    ie = u_strchr(msg,')');
}

void DelayEvent(void (*func)(void *), void *data) {
    struct delayed_event *info = gcalloc(1,sizeof(struct delayed_event));

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

static int splash_e_h(GWindow gw, GEvent *event) {
    static int splash_cnt;
    GRect old;
    int i, y, x;
    static int foolishness[] = {
	    _STR_FreePress,
	    _STR_FreePress,
	    _STR_GaudiamusLigature,
	    _STR_GaudiamusLigature,
	    _STR_InTheBeginning,
	    _STR_LovelyFonts
    };

    if ( event->type == et_expose ) {
	GDrawPushClip(gw,&event->u.expose.rect,&old);
	GDrawDrawImage(gw,&splashimage,NULL,0,0);
	GDrawSetFont(gw,splash_font);
	y = splashimage.u.image->height + as + fh/2;
	for ( i=1; i<linecnt; ++i ) {
	    if ( is>=lines[i-1]+1 && is<lines[i] ) {
		x = 8+GDrawDrawText(gw,8,y,lines[i-1]+1,is-lines[i-1]-1,NULL,0x000000);
		GDrawSetFont(gw,splash_italic);
		GDrawDrawText(gw,x,y,is,lines[i]-is,NULL,0x000000);
	    } else if ( ie>=lines[i-1]+1 && ie<lines[i] ) {
		x = 8+GDrawDrawText(gw,8,y,lines[i-1]+1,ie-lines[i-1]-1,NULL,0x000000);
		GDrawSetFont(gw,splash_font);
		GDrawDrawText(gw,x,y,ie,lines[i]-ie,NULL,0x000000);
	    } else
		GDrawDrawText(gw,8,y,lines[i-1]+1,lines[i]-lines[i-1]-1,NULL,0x000000);
	    y += fh;
	}
	GDrawPopClip(gw,&old);
    } else if ( event->type == et_map ) {
	splash_cnt = 0;
    } else if ( event->type == et_timer && event->u.timer.timer==autosave_timer ) {
	DoAutoSaves();
    } else if ( event->type == et_timer && event->u.timer.timer==splasht ) {
	if ( ++splash_cnt==1 )
	    GDrawResize(gw,splashimage.u.image->width,splashimage.u.image->height-17);
	else if ( splash_cnt==2 )
	    GDrawResize(gw,splashimage.u.image->width,splashimage.u.image->height);
	else if ( splash_cnt>=7 ) {
	    GGadgetEndPopup();
	    GDrawSetVisible(gw,false);
	    GDrawCancelTimer(splasht);
	    splasht = NULL;
	}
    } else if ( event->type == et_timer ) {
	DoDelayedEvents(event);
    } else if ( event->type==et_char || event->type==et_mousedown ||
	    event->type==et_close ) {
	GGadgetEndPopup();
	GDrawSetVisible(gw,false);
    } else if ( event->type==et_mousemove ) {
	GGadgetPreparePopupR(gw,foolishness[rand()%(sizeof(foolishness)/sizeof(foolishness[0]))] );
    }
return( true );
}

static void AddR(char *prog, char *name, char *val ) {
    char *full = galloc(strlen(name)+strlen(val)+4);
    strcpy(full,name);
    strcat(full,": ");
    strcat(full,val);
    GResourceAddResourceString(full,prog);
}
#endif

int main( int argc, char **argv ) {
    extern const char *source_modtime_str;
    const char *load_prefs = getenv("FONTFORGE_LOADPREFS");
#if !defined( FONTFORGE_CONFIG_NO_WINDOWING_UI )
    int i;
    int splash = 1;
    int recover=1;
#endif
#ifdef FONTFORGE_CONFIG_GDRAW
    int any;
    int next_recent=0;
    GRect pos;
    GWindowAttrs wattrs;
    char *display = NULL;
    FontRequest rq;
    static unichar_t times[] = { 't', 'i', 'm', 'e', 's',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t', '\0' };
    int ds, ld;
#elif defined( FONTFORGE_CONFIG_GTK )
    GtkWidget *splashw;
    GtkWidget *notices;
    gchar *home_dir, *rc_path;
    struct argcontext args;
#elif defined( FONTFORGE_CONFIG_NO_WINDOWING_UI )
#else
# error FontForge has not been properly configured.
/* One of FONTFORGE_CONFIG_GDRAW, FONTFORGE_CONFIG_GTK, FONTFORGE_CONFIG_NO_WINDOWING_UI */
/*  must be set */
#endif

    fprintf( stderr, "Copyright (c) 2000-2004 by George Williams.\n Executable based on sources from %s.\n",
	    source_modtime_str );
    fprintf( stderr, "FontForge used to be named PfaEdit.\n" );
    setlocale(LC_ALL,"");
    localeinfo = *localeconv();
    coord_sep = ",";
    if ( *localeinfo.decimal_point=='.' ) coord_sep=",";
    else if ( *localeinfo.decimal_point!='.' ) coord_sep=" ";
#ifdef FONTFORGE_CONFIG_GDRAW
    GResourceAddResourceString(NULL,argv[0]);
#elif defined( FONTFORGE_CONFIG_GTK )
    gtk_set_locale ();

    home_dir = (gchar*) g_get_home_dir();
    rc_path = g_strdup_printf("%s/.fontforgerc", home_dir);
    gtk_rc_add_default_file(rc_path);
    g_free(rc_path);

    gtk_init (&argc, &argv);

    add_pixmap_directory (PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps");
#else
    GResourceSetProg(argv[0]);
#endif
    SetDefaults();
    if ( load_prefs!=NULL && strcasecmp(load_prefs,"Always")==0 )
	LoadPrefs();
    if ( default_encoding==NULL )
	default_encoding=FindOrMakeEncoding("ISO8859-1");
    initadobeenc();
    inituninameannot();
    initrand();
    CheckIsScript(argc,argv);		/* Will run the script and exit if it is a script */
					/* If there is no UI, there is always a script */
			                /*  and we will never return from the above */
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( load_prefs==NULL ||
	    (strcasecmp(load_prefs,"Always")!=0 &&	/* Already loaded */
	     strcasecmp(load_prefs,"Never")!=0 ))
	LoadPrefs();
    for ( i=1; i<argc; ++i ) {
	char *pt = argv[i];
	if ( pt[0]=='-' && pt[1]=='-' )
	    ++pt;
	if ( strcmp(pt,"-sync")==0 )
	    GResourceAddResourceString("Gdraw.Synchronize: true",argv[0]);
# if MyMemory
	else if ( strcmp(pt,"-memory")==0 )
	    __malloc_debug(5);
# endif
# ifdef FONTFORGE_CONFIG_GDRAW
	else if ( strcmp(pt,"-depth")==0 && i<argc-1 )
	    AddR(argv[0],"Gdraw.Depth", argv[++i]);
	else if ( strcmp(pt,"-vc")==0 && i<argc-1 )
	    AddR(argv[0],"Gdraw.VisualClass", argv[++i]);
	else if ( (strcmp(pt,"-cmap")==0 || strcmp(pt,"-colormap")==0) && i<argc-1 )
	    AddR(argv[0],"Gdraw.Colormap", argv[++i]);
	else if ( strcmp(pt,"-keyboard")==0 && i<argc-1 )
	    AddR(argv[0],"Gdraw.Keyboard", argv[++i]);
# endif
	else if ( strcmp(pt,"-nosplash")==0 )
	    splash = 0;
	else if ( strcmp(pt,"-display")==0 && i<argc-1 )
	    display = argv[++i];
	else if ( strcmp(pt,"-recover")==0 && i<argc-1 ) {
	    ++i;
	    if ( strcmp(argv[i],"none")==0 )
		recover=0;
	    else if ( strcmp(argv[i],"clean")==0 )
		recover= -1;
	    else if ( strcmp(argv[i],"auto")==0 )
		recover= 1;
	    else {
		fprintf( stderr, "Invalid argument to -recover, must be none, auto or clean\n" );
		dousage();
	    }
	} else if ( strcmp(pt,"-recover=none")==0 ) {
	    recover = 0;
	} else if ( strcmp(pt,"-recover=clean")==0 ) {
	    recover = -1;
	}
	else if ( strcmp(pt,"-help")==0 )
	    dohelp();
	else if ( strcmp(pt,"-usage")==0 )
	    dousage();
	else if ( strcmp(pt,"-version")==0 )
	    doversion();
    }

# ifdef FONTFORGE_CONFIG_GDRAW
    GDrawCreateDisplays(display,argv[0]);
# endif
    InitCursors();

# ifdef FONTFORGE_CONFIG_GTK
    if ( splash ) {
	splashw = create_FontForgeSplash ();
	splash_window_tooltip_fun( splashw );
	notices = lookup_widget(FontView,"Notices");
	if ( notices!=NULL )
	    gtk_widget_hide(notices);
	gtk_widget_show (splashw);
    }

    gtk_timeout_add(30*1000,_DoAutoSaves,NULL);		/* Check for autosave every 30 seconds */

    args.argc = argc; args.argv = argv; args.recover = recover;
    gtk_timeout_add(30*1000,_DoAutoSaves,&args);
	/* Parse arguments within the main loop */

    gtk_main ();
# else	/* Gdraw */
    /* the splash screen used not to have a title bar (wam_nodecor) */
    /*  but I found I needed to know how much the window manager moved */
    /*  the window around, which I can determine if I have a positioned */
    /*  decorated window created at the begining */
    /* Actually I don't care any more */
    wattrs.mask = wam_events|wam_cursor|wam_bordwidth|wam_backcol|wam_positioned|wam_wtitle|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.positioned = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(_STR_FontForge,NULL);
    wattrs.border_width = 2;
    wattrs.background_color = 0xffffff;
    wattrs.is_dlg = true;
    pos.x = pos.y = 200;
    pos.width = splashimage.u.image->width;
    pos.height = splashimage.u.image->height-54;
    splashw = GDrawCreateTopWindow(NULL,&pos,splash_e_h,NULL,&wattrs);
	memset(&rq,0,sizeof(rq));
	rq.family_name = times;
	rq.point_size = 12;
	rq.weight = 400;
	splash_font = GDrawInstanciateFont(NULL,&rq);
	rq.style = fs_italic;
	splash_italic = GDrawInstanciateFont(NULL,&rq);
	GDrawSetFont(splashw,splash_font);
	GDrawFontMetrics(splash_font,&as,&ds,&ld);
	fh = as+ds+ld;
	SplashLayout();
    if ( splash ) {
	GDrawSetVisible(splashw,true);
	GDrawSync(NULL);
	GDrawProcessPendingEvents(NULL);
	GDrawProcessPendingEvents(NULL);
	splasht = GDrawRequestTimer(splashw,1000,1000,NULL);
    }
    autosave_timer=GDrawRequestTimer(splashw,60*1000,30*1000,NULL);

    GDrawProcessPendingEvents(NULL);
    GDrawSetBuildCharHooks(BuildCharHook,InsCharHook);

    any = 0;
    if ( recover==-1 )
	CleanAutoRecovery();
    else if ( recover )
	any = DoAutoRecovery();

    for ( i=1; i<argc; ++i ) {
	char buffer[1025];
	char *pt = argv[i];

	GDrawProcessPendingEvents(NULL);
	if ( pt[0]=='-' && pt[1]=='-' )
	    ++pt;
	if ( strcmp(pt,"-new")==0 ) {
	    FontNew();
	    any = 1;
#  if HANYANG
	} else if ( strcmp(pt,"-newkorean")==0 ) {
	    MenuNewComposition(NULL,NULL,NULL);
	    any = 1;
#  endif
	} else if ( strcmp(pt,"-last")==0 ) {
	    if ( next_recent<RECENT_MAX && RecentFiles[next_recent]!=NULL )
		if ( ViewPostscriptFont(RecentFiles[next_recent++]))
		    any = 1;
	} else if ( strcmp(pt,"-sync")==0 || strcmp(pt,"-memory")==0 ||
		strcmp(pt,"-nosplash")==0 || strcmp(pt,"-recover=none")==0 ||
		strcmp(pt,"-recover=clean")==0 || strcmp(pt,"-recover=auto")==0 )
	    /* Already done, needed to be before display opened */;
	else if ( (strcmp(pt,"-depth")==0 || strcmp(pt,"-vc")==0 ||
		    strcmp(pt,"-cmap")==0 || strcmp(pt,"-colormap")==0 || 
		    strcmp(pt,"-keyboard")==0 || 
		    strcmp(pt,"-display")==0 || strcmp(pt,"-recover")==0 ) &&
		i<argc-1 )
	    ++i; /* Already done, needed to be before display opened */
	else {
	    GFileGetAbsoluteName(argv[i],buffer,sizeof(buffer));
	    if ( GFileIsDir(buffer)) {
		char *fname;
		if ( buffer[strlen(buffer)-1]!='/' ) {
		    /* If dirname doesn't end in "/" we'll be looking in parent dir */
		    buffer[strlen(buffer)+1]='\0';
		    buffer[strlen(buffer)] = '/';
		}
		fname = GetPostscriptFontName(buffer,false);
		if ( fname!=NULL )
		    ViewPostscriptFont(fname);
		any = 1;	/* Even if we didn't get a font, don't bring up dlg again */
		free(fname);
	    } else if ( ViewPostscriptFont(buffer)!=0 )
		any = 1;
	}
    }
    if ( !any )
	MenuOpen(NULL,NULL,NULL);
    GDrawEventLoop(NULL);
# endif
#endif
return( 0 );
}
