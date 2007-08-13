/* Copyright (C) 2000-2007 by George Williams */
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
# include "callbacks.h"
# include "support.h"
#endif
#include <time.h>
#include <sys/time.h>
#include <locale.h>
#include <unistd.h>
#if !defined(_NO_LIBUNINAMESLIST) && !defined(_STATIC_LIBUNINAMESLIST) && !defined(NODYNAMIC)
#  include <dynamic.h>
#endif
#ifdef __Mac
# include <stdlib.h>		/* getenv,setenv */
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
#ifdef FONTFORGE_CONFIG_TYPE3
    printf( "fontforge %s-ML\n", source_version_str );
#else
    printf( "fontforge %s\n", source_version_str );
#endif
exit(0);
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void _dousage(void) {
    printf( "fontforge [options] [fontfiles]\n" );
    printf( "\t-new\t\t\t (creates a new font)\n" );
    printf( "\t-last\t\t\t (loads the last sfd file closed)\n" );
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
    printf( "\t-dontopenxdevices\t (in case that fails)\n" );
    printf( "\t-sync\t\t\t (syncs the display, debugging)\n" );
    printf( "\t-keyboard ibm|mac|sun|ppc  (generates appropriate hotkeys in menus)\n" );
#endif
#if MyMemory
    printf( "\t-memory\t\t\t (turns on memory checks, debugging)\n" );
#endif
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
	#PYTHON_LIB_NAME,
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
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static void initrand(void) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    srand(tv.tv_usec);
}

static void initlibrarysearchpath(void) {
#ifdef __Mac
    /* If the user has not set library path, then point it at fink */
    /*  otherwise leave alone. On the mac people often use fink to */
    /*  install image libs. For some reason fink installs in a place */
    /*  the dynamic loader doesn't find */
    /* (And fink's attempts to set the PATH variables generally don't work */
    setenv("DYLD_LIBRARY_PATH","/sw/lib",0);
#endif
}

struct delayed_event {
    void *data;
    void (*func)(void *);
};

#ifdef FONTFORGE_CONFIG_GTK

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
	N_("In the beginning was the letter...")
/* GT: Misquotation of As You Like It, II, vii, 139-143 */
	N_("All the world's a page,\n"
	"And all the men and women merely letters;\n"
	"They have their ligatures and hyphenations,\n"
	"And one man in his time forms many words."),
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

void ShowAboutScreen(void) {
    GtkWidget *splashw, *version;
    char buffer[200];
    extern const char *source_modtime_str;
    extern const char *source_version_str;

    splashw = create_FontForgeSplash ();
    splash_window_tooltip_fun( splashw );
    version = lookup_widget(splashw,"Version");
    if ( version!=NULL ) {
#ifdef FONTFORGE_CONFIG_TYPE3
	sprintf( buffer, "Version: %s (%s-ML)", source_modtime_str, source_version_str);
#else
	sprintf( buffer, "Version: %s (%s)", source_modtime_str, source_version_str);
#endif
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
		strcmp(pt,"-recover=clean")==0 || strcmp(pt,"-recover=auto")==0 ||
		strcmp(pt,"-dontopenxdevices")==0 )
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
		fname = galloc(strlen(buffer)+strlen("glyphs/contents.plist")+1);
		strcpy(fname,buffer); strcat(fname,"glyphs/contents.plist");
		if ( GFileExists(fname)) {
		    /* It's probably a Unified Font Object directory */
		    free(fname);
		    if ( ViewPostscriptFont(buffer) )
			any = 1;
		} else {
		    strcpy(fname,buffer); strcat(fname,"/font.props");
		    if ( GFileExists(fname)) {
			/* It's probably a sf dir collection */
			free(fname);
			if ( ViewPostscriptFont(buffer) )
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
			    ViewPostscriptFont(fname);
			any = 1;	/* Even if we didn't get a font, don't bring up dlg again */
			free(fname);
		    }
		}
	    } else if ( ViewPostscriptFont(buffer)!=0 )
		any = 1;
	}
    }
    if ( !any )
	Menu_Open(NULL,NULL);
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
#ifdef FONTFORGE_CONFIG_TYPE3
    uc_strcat(pt,"-ML");
#endif
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
    static char *foolishness[] = {
/* GT: These strings are for fun. If they are offensive of incomprehensible */
/* GT: simply translate them as something dull like: "FontForge" */
	    N_("A free press discriminates\nagainst the illiterate."),
	    N_("A free press discriminates\nagainst the illiterate."),
	    N_("Gaudeamus Ligature!"),
	    N_("Gaudeamus Ligature!"),
	    N_("In the beginning was the letter..."),
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
	    GDrawResize(gw,splashimage.u.image->width,splashimage.u.image->height-24);
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
	GGadgetPreparePopup8(gw,_(foolishness[rand()%(sizeof(foolishness)/sizeof(foolishness[0]))]) );
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

static char *getLocaleDir(void) {
    static char *sharedir=NULL;
    static int set=false;
    char *pt;
    int len;

    if ( set )
return( sharedir );

    set = true;
    pt = strstr(GResourceProgramDir,"/bin");
    if ( pt==NULL ) {
#ifdef PREFIX
return( PREFIX "/share/locale" );
#else
	pt = GResourceProgramDir + strlen(GResourceProgramDir);
#endif
    }
    len = (pt-GResourceProgramDir)+strlen("/share/locale")+1;
    sharedir = galloc(len);
    strncpy(sharedir,GResourceProgramDir,pt-GResourceProgramDir);
    strcpy(sharedir+(pt-GResourceProgramDir),"/share/locale");
return( sharedir );
}

#if defined(__Mac) && !(defined(FONTFORGE_CONFIG_NO_WINDOWING_UI) || defined( X_DISPLAY_MISSING ))
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

int FontForgeMain( int argc, char **argv ) {
    extern const char *source_modtime_str;
    const char *load_prefs = getenv("FONTFORGE_LOADPREFS");
#if !defined( FONTFORGE_CONFIG_NO_WINDOWING_UI )
    int i;
    extern int splash;
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

#ifdef FONTFORGE_CONFIG_TYPE3
    fprintf( stderr, "Copyright (c) 2000-2007 by George Williams.\n Executable based on sources from %s-ML.\n",
	    source_modtime_str );
#else
    fprintf( stderr, "Copyright (c) 2000-2007 by George Williams.\n Executable based on sources from %s.\n",
	    source_modtime_str );
#endif
    setlocale(LC_ALL,"");
    localeinfo = *localeconv();
    coord_sep = ",";
    if ( *localeinfo.decimal_point=='.' ) coord_sep=",";
    else if ( *localeinfo.decimal_point!='.' ) coord_sep=" ";
    if ( getenv("FF_SCRIPT_IN_LATIN1") ) use_utf8_in_script=false;

    GResourceSetProg(argv[0]);
#if defined( FONTFORGE_CONFIG_GTK )
    gtk_set_locale ();

    home_dir = (gchar*) g_get_home_dir();
    rc_path = g_strdup_printf("%s/.fontforgerc", home_dir);
    gtk_rc_add_default_file(rc_path);
    g_free(rc_path);

    gtk_init (&argc, &argv);

    add_pixmap_directory (PIXMAP_DIR);
#endif

    GMenuSetShortcutDomain("FontForge-MenuShortCuts");
    bind_textdomain_codeset("FontForge-MenuShortCuts","UTF-8");
    bindtextdomain("FontForge-MenuShortCuts", getLocaleDir());
    bind_textdomain_codeset("FontForge","UTF-8");
    bindtextdomain("FontForge", getLocaleDir());
    textdomain("FontForge");
#if !defined( FONTFORGE_CONFIG_GTK )
    GResourceUseGetText();
#endif

#if defined(__Mac) && !(defined(FONTFORGE_CONFIG_NO_WINDOWING_UI) || defined( X_DISPLAY_MISSING ))
    /* Start X if they haven't already done so. Well... try anyway */
    /* Must be before we change DYLD_LIBRARY_PATH or X won't start */
    if ( uses_local_x(argc,argv) && getenv("DISPLAY")==NULL ) {
	system( "open /Applications/Utilities/X11.app/" );
	setenv("DISPLAY",":0.0",0);
    }
#endif

    SetDefaults();
    if ( load_prefs!=NULL && strcasecmp(load_prefs,"Always")==0 )
	LoadPrefs();
    else
	PrefDefaultEncoding();
    if ( default_encoding==NULL )
	default_encoding=FindOrMakeEncoding("ISO8859-1");
    if ( default_encoding==NULL )
	default_encoding=&custom;	/* In case iconv is broken */
    initlibrarysearchpath();
    initrand();
    initadobeenc();
    inituninameannot();
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
# if defined( FONTFORGE_CONFIG_GTK )
	if ( strcmp(pt,"-sync")==0 )
	    gwwv_x11_synchronize();
# else
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
# endif
# if MyMemory
	else if ( strcmp(pt,"-memory")==0 )
	    __malloc_debug(5);
# endif
	else if ( strcmp(pt,"-nosplash")==0 )
	    splash = 0;
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
	else if ( strcmp(pt,"-library-status")==0 )
	    dolibrary();
    }

# ifdef FONTFORGE_CONFIG_GDRAW
    GDrawCreateDisplays(display,argv[0]);
    default_background = GDrawGetDefaultBackground(screen_display);
    InitToolIconClut(default_background);
# endif
    InitCursors();
#ifndef _NO_PYTHON
    PyFF_ProcessInitFiles();
#endif

# ifdef FONTFORGE_CONFIG_GTK
    if ( splash ) {
	splashw = create_FontForgeSplash ();
	splash_window_tooltip_fun( splashw );
	notices = lookup_widget(splashw,"Notices");
	if ( notices!=NULL )
	    gtk_widget_hide(notices);
	gtk_widget_show (splashw);
    }

    gtk_timeout_add(30*1000,_DoAutoSaves,NULL);		/* Check for autosave every 30 seconds */

    args.argc = argc; args.argv = argv; args.recover = recover;
    gtk_timeout_add(100,ParseArgs,&args);
	/* Parse arguments within the main loop */

    gtk_main ();
# else	/* Gdraw */
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
    wattrs.is_dlg = true;
    pos.x = pos.y = 200;
    pos.width = splashimage.u.image->width;
    pos.height = splashimage.u.image->height-56;		/* 54 */
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
		strcmp(pt,"-recover=clean")==0 || strcmp(pt,"-recover=auto")==0 ||
		strcmp(pt,"-dontopenxdevices")==0 )
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
		fname = galloc(strlen(buffer)+strlen("/glyphs/contents.plist")+1);
		strcpy(fname,buffer); strcat(fname,"/glyphs/contents.plist");
		if ( GFileExists(fname)) {
		    /* It's probably a Unified Font Object directory */
		    free(fname);
		    if ( ViewPostscriptFont(buffer) )
			any = 1;
		} else {
		    strcpy(fname,buffer); strcat(fname,"/font.props");
		    if ( GFileExists(fname)) {
			/* It's probably a sf dir collection */
			free(fname);
			if ( ViewPostscriptFont(buffer) )
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
			    ViewPostscriptFont(fname);
			any = 1;	/* Even if we didn't get a font, don't bring up dlg again */
			free(fname);
		    }
		}
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
