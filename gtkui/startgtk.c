/* Copyright (C) 2008 by George Williams */
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
#include "fontforgegtk.h"
#include <gfile.h>
#include <fontforge/libffstamp.h>
#include <time.h>
#include <sys/time.h>
#include <locale.h>
#include <unistd.h>
#include <dynamic.h>
#ifdef __Mac
# include <stdlib.h>		/* getenv,setenv */
#endif

int splash = 1;			/* A preference item too */

static GtkWidget *splashw;

static void _dousage(void) {
    printf( "fontforge [options] [fontfiles]\n" );
    printf( "\t-new\t\t\t (creates a new font)\n" );
    printf( "\t-last\t\t\t (loads the last sfd file closed)\n" );
#if HANYANG
    printf( "\t-newkorean\t\t (creates a new korean font)\n" );
#endif
    printf( "\t-recover none|auto|clean (control error recovery)\n" );
    printf( "\t-allglyphs\t\t (load all glyphs in the 'glyf' table\n\t\t\t of a truetype collection)\n" );
    printf( "\t-nosplash\t\t (no splash screen)\n" );
    printf( "\t-display display-name\t (sets the X display)\n" );
    printf( "\t-usage\t\t\t (displays this message, and exits)\n" );
    printf( "\t-help\t\t\t (displays this message, invokes a browser)\n\t\t\t\t  (Using the BROWSER environment variable)\n" );
    printf( "\t-version\t\t (prints the version of fontforge and exits)\n" );
    printf( "\t-library-status\t (prints information about optional libraries\n\t\t\t\t and exits)\n" );
    printf( "\t-lang=py\t\t use python for scripts (may precede -script)\n" );
    printf( "\t-lang=ff\t\t use fontforge's legacy scripting language\n" );
    printf( "\t-script scriptfile\t (executes scriptfile)\n" );
    printf( "\t\tmust be the first option (or follow -lang).\n" );
    printf( "\t\tAll others passed to scriptfile.\n" );
    printf( "\t-dry scriptfile\t (syntax checks scriptfile)\n" );
    printf( "\t\tmust be the first option. All others passed to scriptfile.\n" );
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

static struct library_descriptor {
    char *libname;
    char *entry_point;
    char *description;
    char *url;
    int usable;
    struct library_descriptor *depends_on;
} libs[] = {
    {
#ifdef PYTHON_LIB_NAME
	"lib" PYTHON_LIB_NAME,
#else
	"libpython-",		/* a bad name */
#endif
	dlsymmod("Py_Main"),
	"This allows users to write python scripts in fontforge",
	"http://www.python.org/",
#ifdef _NO_PYTHON
	0
#else
	1
#endif
    },
    { "libspiro", dlsymmod("TaggedSpiroCPsToBezier"), "This allows you to edit with Raph Levien's spiros.", "http://libspiro.sf.net/",
#ifdef _NO_LIBSPIRO
	0
#else
	1
#endif
    },
    { "libz", dlsymmod("deflateEnd"), "This is a prerequisite for reading png files,\n\t and is used for some pdf files.", "http://www.gzip.org/zlib/",
#ifdef _NO_LIBPNG
	0
#else
	1
#endif
    },
    { "libpng", dlsymmod("png_create_read_struct"), "This is one way to read png files.", "http://www.libpng.org/pub/png/libpng.html",
#ifdef _NO_LIBPNG
	0,
#else
	1,
#endif
	&libs[1] },
    { "libpng12", dlsymmod("png_create_read_struct"), "This is another way to read png files.", "http://www.libpng.org/pub/png/libpng.html",
#ifdef _NO_LIBPNG
	0,
#else
	1,
#endif
	&libs[1] },
    { "libjpeg", dlsymmod("jpeg_CreateDecompress"), "This allows fontforge to load jpeg images.", "http://www.ijg.org/",
#ifdef _NO_LIBPNG
	0
#else
	1
#endif
    },
    { "libtiff", dlsymmod("TIFFOpen"), "This allows fontforge to open tiff images.", "http://www.libtiff.org/",
#ifdef _NO_LIBTIFF
	0
#else
	1
#endif
    },
    { "libgif", dlsymmod("DGifOpenFileName"), "This allows fontforge to open gif images.", "http://gnuwin32.sf.net/packages/libungif.htm",
#ifdef _NO_LIBUNGIF
	0
#else
	1
#endif
    },
    { "libungif", dlsymmod("DGifOpenFileName"), "This allows fontforge to open gif images.", "http://gnuwin32.sf.net/packages/libungif.htm",
#ifdef _NO_LIBUNGIF
	0
#else
	1
#endif
    },
    { "libxml2", dlsymmod("xmlParseFile"), "This allows fontforge to load svg files and fonts and ufo fonts.", "http://xmlsoft.org/",
#ifdef _NO_LIBXML
	0
#else
	1
#endif
    },
    { "libuninameslist", dlsymmod("UnicodeNameAnnot"), "This provides fontforge with the names of all (named) unicode characters", "http://libuninameslist.sf.net/",
#ifdef _NO_LIBUNINAMESLIST
	0
#else
	1
#endif
    },
    { "libfreetype", dlsymmod("FT_New_Memory_Face"), "This provides a better rasterizer than the one built in to fontforge", "http://freetype.sf.net/",
#if _NO_FREETYPE || _NO_MMAP
	0
#else
	1
#endif
    },
    { NULL }
};

static void _dolibrary(void) {
    int i;
    char buffer[300];
    int fail, isfreetype, hasdebugger;
    DL_CONST void *lib_handle;

    fprintf( stderr, "\n" );
    for ( i=0; libs[i].libname!=NULL; ++i ) {
	fail = false;
	if ( libs[i].depends_on!=NULL ) {
	    sprintf( buffer, "%s%s", libs[i].depends_on->libname, SO_EXT );
	    lib_handle = dlopen(buffer,RTLD_LAZY);
	    if ( lib_handle==NULL )
		fail = 3;
	    else {
		if ( dlsymbare(lib_handle,libs[i].depends_on->entry_point)==NULL )
		    fail = 4;
	    }
	}
	if ( !fail ) {
	    sprintf( buffer, "%s%s", libs[i].libname, SO_EXT );
	    lib_handle = dlopen(buffer,RTLD_LAZY);
	    if ( lib_handle==NULL )
		fail = true;
	    else {
		if ( dlsymbare(lib_handle,libs[i].entry_point)==NULL )
		    fail = 2;
	    }
	}
	isfreetype = strcmp(libs[i].libname,"libfreetype")==0;
	hasdebugger = false;
	if ( !fail && isfreetype && dlsym(lib_handle,"TT_RunIns")!=NULL )
	    hasdebugger = true;
	fprintf( stderr, "%-15s - %s\n", libs[i].libname,
		fail==0 ? "is present and appears functional on your system." :
		fail==1 ? "is not present on your system." :
		fail==2 ? "is present on your system but is not functional." :
		fail==3 ? "a prerequisite library is missing." :
			"a prerequisite library is not functional." );
	fprintf( stderr, "\t%s\n", libs[i].description );
	if ( isfreetype ) {
	    if ( hasdebugger )
		fprintf( stderr, "\tThis version of freetype includes the byte code interpreter\n\t which means you can use fontforge as a truetype debugger.\n" );
	    else
		fprintf( stderr, "\tThis version of freetype does notinclude the byte code interpreter\n\t which means you cannot use fontforge as a truetype debugger.\n\t If you want the debugger you must download freetype source,\n\t enable the bytecode interpreter, and then build it.\n" );
	}
	if ( fail || (isfreetype && !hasdebugger))
	    fprintf( stderr, "\tYou may download %s from %s .\n", libs[i].libname, libs[i].url );
	if ( !libs[i].usable )
	    fprintf( stderr, "\tUnfortunately this version of fontforge is not configured to use this\n\t library.  You must rebuild from source.\n" );
    }
}

static void dolibrary(void) {
    _dolibrary();
exit(0);
}

struct delayed_event {
    void *data;
    void (*func)(void *);
};


static void splash_window_tooltip_fun( GtkWidget *splashw ) {
    static char *foolishness[] = {
/* GT: These strings are for fun. If they are offensive of incomprehensible */
/* GT: simply translate them as something dull like: "FontForge" */
/* GT: Someone said this first quote was a political slogan. If anything, it */
/* GT: is intended as a satire */
	N_("A free press discriminates\nagainst the illiterate."),
	N_("A free press discriminates\nagainst the illiterate."),
/* GT: The following are mostly misquotations of fairly well-known bits of */
/* GT:  English literature. Trying to translate them literally may miss the */
/* GT:  point. Finding something appropriate in the literature of your own */
/* GT:  language and then twisting it might be closer to the original intent. */
/* GT:  Or just translate them all as "FontForge" and don't worry about them. */
/* GT: Misquotation of an old latin drinking song */
	N_("Gaudeamus Ligature!"),
	N_("Gaudeamus Ligature!"),
/* GT: Misquotation of the Gospel of John */
	N_("In the beginning was the letter..."),
/* GT: Misquotation of The Wind in the Willows */
	N_("There is nothing, absolutely nothing,\n"
	"half so much worth doing\n"
	"as simply messing about with fonts"),
/* GT: The New Hacker's Dictionary attributes this to XEROX PARC */
/* GT: A misquotation of "Ontogeny recapitulates phylogeny" */
	N_("fontology recapitulates file-ogeny")
    };
    GtkTooltips *tips;

    tips = gtk_tooltips_new();
    gtk_tooltips_set_tip( tips, splashw, _(foolishness[rand()%(sizeof(foolishness)/sizeof(foolishness[0]))]), NULL );
}

gboolean SplashDismiss(GtkWidget *widget, GdkEventButton *event,gpointer user_data) {
    gtk_widget_hide(widget);
return( true );
}

static gboolean Splash_Changes(gpointer foo) {
    static int splash_cnt=0;
    GtkWidget *splashw = (GtkWidget *) foo;
    GtkWidget *ffsplash;

    if ( ++splash_cnt>15 ) {
	gtk_widget_hide(splashw);
return( false );
    }
    /* Slowly reveal more and more of the splash screen */
    if ( splash_cnt==2 ) {
	ffsplash = lookup_widget(splashw,"ffsplash2");
	if ( ffsplash!=NULL )
	    gtk_widget_show(ffsplash);
    }
    if ( splash_cnt==6 ) {
	ffsplash = lookup_widget(splashw,"ffsplash3");
	if ( ffsplash!=NULL )
	    gtk_widget_show(ffsplash);
    }
return( true );
}

void ShowAboutScreen(void) {
    GtkWidget *version, *notices;
    char buffer[200];
    extern const char *source_modtime_str;
    extern const char *source_version_str;

    if ( splashw==NULL ) {
	splashw = create_FontForgeSplash ();
	splash_window_tooltip_fun( splashw );
    }
    notices = lookup_widget(splashw,"Notices");
    if ( notices!=NULL )
	gtk_widget_show(notices);
    version = lookup_widget(splashw,"Version");
    if ( version!=NULL ) {
#ifdef FONTFORGE_CONFIG_TYPE3
	sprintf( buffer, "Version: %s (%s-ML)", source_modtime_str, source_version_str);
#else
	sprintf( buffer, "Version: %s (%s)", source_modtime_str, source_version_str);
#endif
	gtk_label_set_text(GTK_LABEL( version ),buffer);
    }
    version = lookup_widget(splashw,"LibVersion");
    if ( version!=NULL ) {
	sprintf( buffer, "Library Version: %s", library_version_configuration.library_source_modtime_string);
	gtk_label_set_text(GTK_LABEL( version ),buffer);
    }
    gtk_widget_show (splashw);
}

static int DoDelayedEvents(gpointer data) {
    struct delayed_event *info = (struct delayed_event *) data;

    if ( info!=NULL ) {
	(info->func)(info->data);
	chunkfree(info,sizeof(struct delayed_event));
    }
return( FALSE );		/* cancel timer */
}

void DelayEvent(void (*func)(void *), void *data) {
    struct delayed_event *info = chunkalloc(sizeof(struct delayed_event));

    info->data = data;
    info->func = func;
    
    gtk_timeout_add(100,DoDelayedEvents,info);
}

static int __DoAutoSaves( gpointer ignored ) {
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
    int openflags = 0;

    /* Wait until the UI has started, otherwise people who don't have consoles*/
    /*  open won't get our error messages, and it's an important one */
    /* Scripting doesn't care about a mismatch, because scripting interpretation */
    /*  all lives in the library, so it doesn't need this check. */
    check_library_version(&exe_library_version_configuration,true,false);

    any = 0;
    if ( recover==-1 )
	CleanAutoRecovery();
    else if ( recover )
	any = DoAutoRecovery(recover-1);

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
		if ( ViewPostScriptFont(RecentFiles[next_recent++],openflags))
		    any = 1;
	} else if ( strcmp(pt,"-nosplash")==0 || strcmp(pt,"-recover=none")==0 ||
		strcmp(pt,"-recover=clean")==0 || strcmp(pt,"-recover=auto")==0 ||
		strcmp(pt,"-recover=inquire")==0 )
	    /* Already done, needed to be before display opened */;
	else if ( strcmp(pt,"-recover")==0 && i<argc-1 )
	    ++i; /* As above, this is already done, but this one takes an */
	         /*  argument, so skip that too */
	else if ( strcmp(pt,"-allglyphs")==0 )
		openflags |= of_all_glyphs_in_ttc;
	else {
	    GFileGetAbsoluteName(argv[i],buffer,sizeof(buffer));
	    if ( GFileIsDir(buffer)) {
		char *fname;
		fname = galloc(strlen(buffer)+strlen("glyphs/contents.plist")+1);
		strcpy(fname,buffer); strcat(fname,"glyphs/contents.plist");
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
			fname = GetPostscriptFontName(buffer,false);
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
    if ( !any )
	Menu_Open(NULL,NULL);
    if ( fv_list==NULL )		/* Didn't open anything? */
exit(0);

return( FALSE );	/* Cancel timer */
}

#if defined(__Mac)
static int uses_local_x(int argc,char **argv) {
    int i;
    char *arg;

    for ( i=1; i<argc; ++i ) {
	arg = argv[i];
	if ( *arg=='-' ) {
	    if ( arg[0]=='-' && arg[1]=='-' )
		++arg;
	    if ( strcmp(arg,"-display")==0 )
return( false );		/* we use a different display */
	    if ( strcmp(arg,"-c")==0 )
return( false );		/* we use a script string, no x display at all */
	    if ( strcmp(arg,"-script")==0 )
return( false );		/* we use a script, no x display at all */
	    if ( strcmp(arg,"-")==0 )
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

static char *getLocaleDir(void) {

#ifdef PREFIX
return( PREFIX "/share/locale" );
#else
return( "share/locale" );
#endif
}

int main( int argc, char **argv ) {
    extern const char *source_modtime_str, *source_version_str;
    const char *load_prefs = getenv("FONTFORGE_LOADPREFS");
    int i;
    extern int splash;
    int recover=1;
    GtkWidget *notices, *ffsplash;
    gchar *home_dir, *rc_path;
    struct argcontext args;

#ifdef FONTFORGE_CONFIG_TYPE3
    fprintf( stderr, "Copyright (c) 2000-2012 by George Williams.\n Executable based on sources from %s-ML.\n",
	    source_modtime_str );
#else
    fprintf( stderr, "Copyright (c) 2000-2012 by George Williams.\n Executable based on sources from %s.\n",
	    source_modtime_str );
#endif
    fprintf( stderr, " Library based on sources from %s.\n", library_version_configuration.library_source_modtime_string );

    gtk_set_locale ();

    home_dir = (gchar*) g_get_home_dir();
    rc_path = g_strdup_printf("%s/.fontforgerc", home_dir);
    gtk_rc_add_default_file(rc_path);
    g_free(rc_path);

    gtk_init (&argc, &argv);

#if defined(SHAREDIR)
    add_pixmap_directory( SHAREDIR "/pixmaps");
#elif defined(PREFIX)
    add_pixmap_directory( PREFIX "/share/fontforge/pixmaps" );
#else
    add_pixmap_directory( "./pixmaps" );
#endif

    bind_textdomain_codeset("FontForge","UTF-8");
    bindtextdomain("FontForge", getLocaleDir());
    textdomain("FontForge");

#if defined(__Mac)
    /* Start X if they haven't already done so. Well... try anyway */
    /* Must be before we change DYLD_LIBRARY_PATH or X won't start */
    if ( uses_local_x(argc,argv) && getenv("DISPLAY")==NULL ) {
	system( "open /Applications/Utilities/X11.app/" );
	setenv("DISPLAY",":0.0",0);
    }
#endif

    FF_SetUiInterface(&gtk_ui_interface);
    FF_SetPrefsInterface(&gtk_prefs_interface);
    /*FF_SetSCInterface(&gtk_sc_interface);*/
    /*FF_SetCVInterface(&gtk_cv_interface);*/
    /*FF_SetBCInterface(&gtk_bc_interface);*/
    FF_SetFVInterface(&gtk_fv_interface);
    /*FF_SetFIInterface(&gtk_fi_interface);*/
    /*FF_SetMVInterface(&gtk_mv_interface);*/
    /*FF_SetClipInterface(&gtk_clip_interface);*/
#ifndef _NO_PYTHON
    PythonUI_Init();
#endif

    InitSimpleStuff();
    if ( load_prefs!=NULL && strcasecmp(load_prefs,"Always")==0 )
	LoadPrefs();
    if ( default_encoding==NULL )
	default_encoding=FindOrMakeEncoding("ISO8859-1");
    if ( default_encoding==NULL )
	default_encoding=&custom;	/* In case iconv is broken */
    CheckIsScript(argc,argv);		/* Will run the script and exit if it is a script */
					/* If there is no UI, there is always a script */
			                /*  and we will never return from the above */
    if ( load_prefs==NULL ||
	    (strcasecmp(load_prefs,"Always")!=0 &&	/* Already loaded */
	     strcasecmp(load_prefs,"Never")!=0 ))
	LoadPrefs();
    for ( i=1; i<argc; ++i ) {
	char *pt = argv[i];
	if ( pt[0]=='-' && pt[1]=='-' )
	    ++pt;

	if ( strcmp(pt,"-nosplash")==0 )
	    splash = 0;
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
	} else if ( strcmp(pt,"-help")==0 )
	    dohelp();
	else if ( strcmp(pt,"-usage")==0 )
	    dousage();
	else if ( strcmp(pt,"-version")==0 )
	    doversion(source_version_str);
	else if ( strcmp(pt,"-library-status")==0 )
	    dolibrary();
    }

    InitCursors();
#ifndef _NO_PYTHON
    PyFF_ProcessInitFiles();
#endif

    if ( splash ) {
	splashw = create_FontForgeSplash ();
	splash_window_tooltip_fun( splashw );
	notices = lookup_widget(splashw,"Notices");
	if ( notices!=NULL )
	    gtk_widget_hide(notices);
	ffsplash = lookup_widget(splashw,"ffsplash2");
	if ( ffsplash!=NULL )
	    gtk_widget_hide(ffsplash);
	ffsplash = lookup_widget(splashw,"ffsplash3");
	if ( ffsplash!=NULL )
	    gtk_widget_hide(ffsplash);
	gtk_window_set_position(GTK_WINDOW(splashw),GTK_WIN_POS_CENTER_ALWAYS);
	gtk_widget_show (splashw);
	ff_progress_allow_events();
	gtk_timeout_add(1000,Splash_Changes,splashw);
    }

    gtk_timeout_add(30*1000,__DoAutoSaves,NULL);		/* Check for autosave every 30 seconds */

    args.argc = argc; args.argv = argv; args.recover = recover;
    gtk_timeout_add(100,ParseArgs,&args);
	/* Parse arguments within the main loop */

    gtk_main ();
return( 0 );
}

struct library_version_configuration exe_library_version_configuration = {
    1 /* REPLACE_ME_WITH_MAJOR_VERSION*/,
    0 /* REPLACE_ME_WITH_MINOR_VERSION*/,
    LibFF_ModTime,
    LibFF_ModTime_Str,
    LibFF_VersionDate,
    sizeof(struct library_version_configuration),
    sizeof(struct splinefont),
    sizeof(struct splinechar),
    sizeof(struct fontviewbase),
    sizeof(struct charviewbase),
    sizeof(struct cvcontainer),

    1,

#ifdef FONTFORGE_CONFIG_TYPE3
    1,
#else
    0,
#endif

#ifdef _NO_PYTHON
    0,
#else
    1,
#endif
    0xff		/* Not currently defined */
};
