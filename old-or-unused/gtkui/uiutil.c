/* Copyright (C) 2000-2008 by George Williams */
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
#include <stdarg.h>
#include <unistd.h>
#include <utype.h>
#include <ustring.h>

#if __CygWin
#include <unistd.h>
extern void cygwin_conv_to_full_posix_path(const char *win,char *unx);
extern void cygwin_conv_to_full_win32_path(const char *unx,char *win);
#endif

#if __CygWin
/* Try to find the default browser by looking it up in the windows registry */
/* The registry is organized as a tree. We are interested in the subtree */
/*  starting at HKEY_CLASSES_ROOT. This contains two different kinds of things*/
/*  Extensions and Programs. First we look up the extension and it refers us */
/*  to a program. So we look up the program, and look up shell->open->command */
/*  in it. The value of command is a path followed by potential arguments */
/*  viz: c:\program files\foobar "%1" */

/* Extensions seem to contain the ".", so ".html" not "html" */

#include <w32api/wtypes.h>
#include <w32api/winbase.h>
#include <w32api/winreg.h>

static char *win_program_from_extension(char *exten) {
    DWORD type, dlen, err;
    char programindicator[1000];
    char programpath[1000];
    HKEY hkey_prog, hkey_shell, hkey_open, hkey_exten, hkey_command;
    char *pt;

    if ( RegOpenKeyEx(HKEY_CLASSES_ROOT,exten,0,KEY_READ,&hkey_exten)!=ERROR_SUCCESS ) {
	/*fprintf( stderr, "Failed to find extension \"%s\", did it have a period?\n", exten );*/
return( NULL );
    }
    dlen = sizeof(programindicator);
    if ( (err=RegQueryValueEx(hkey_exten,"",NULL,&type,(uint8 *)programindicator,&dlen))!=ERROR_SUCCESS ) {
	LogError( _("Failed to default value of exten \"%s\".\n Error=%ld"), exten, err );
	RegCloseKey(hkey_exten);
return( NULL );
    }
    RegCloseKey(hkey_exten);

    if ( RegOpenKeyEx(HKEY_CLASSES_ROOT,programindicator,0,KEY_READ,&hkey_prog)!=ERROR_SUCCESS ) {
	LogError( _("Failed to find program \"%s\"\n"), programindicator );
return( NULL );
    }
    if ( RegOpenKeyEx(hkey_prog,"shell",0,KEY_READ,&hkey_shell)!=ERROR_SUCCESS ) {
	LogError( _("Failed to find \"%s->shell\"\n"), programindicator );
	RegCloseKey(hkey_prog);
return( NULL );
    }
    if ( RegOpenKeyEx(hkey_shell,"open",0,KEY_READ,&hkey_open)!=ERROR_SUCCESS ) {
	LogError( _("Failed to find \"%s->shell->open\"\n"), programindicator );
	RegCloseKey(hkey_prog); RegCloseKey(hkey_shell);
return( NULL );
    }
    if ( RegOpenKeyEx(hkey_open,"command",0,KEY_READ,&hkey_command)!=ERROR_SUCCESS ) {
	LogError( _("Failed to find \"%s->shell->open\"\n"), programindicator );
	RegCloseKey(hkey_prog); RegCloseKey(hkey_shell); RegCloseKey(hkey_command);
return( NULL );
    }

    dlen = sizeof(programpath);
    if ( RegQueryValueEx(hkey_command,"",NULL,&type,(uint8 *)programpath,&dlen)!=ERROR_SUCCESS ) {
	LogError( _("Failed to find default for \"%s->shell->open->command\"\n"), programindicator );
	RegCloseKey(hkey_prog); RegCloseKey(hkey_shell); RegCloseKey(hkey_open); RegCloseKey(hkey_command);
return( NULL );
    }

    RegCloseKey(hkey_prog); RegCloseKey(hkey_shell); RegCloseKey(hkey_open); RegCloseKey(hkey_command);

    pt = strstr(programpath,"%1");
    if ( pt!=NULL )
	pt[1] = 's';
return( copy(programpath));
}

static void do_windows_browser(char *fullspec) {
    char *format, *start, *pt, ch, *temp, *cmd;

    format = win_program_from_extension(".html");
    if ( format==NULL )
	format = win_program_from_extension(".htm");
    if ( format==NULL ) {
	gwwv_post_error(_("No Browser"),_("Could not find a browser. Set the BROWSER environment variable to point to one"));
return;
    }

    if ( format[0]=='"' || format[0]=='\'' ) {
	start = format+1;
	pt = strchr(start,format[0]);
    } else {
	start = format;
	pt = strchr(start,' ');
    }
    if ( pt==NULL ) pt = start+strlen(start);
    ch = *pt; *pt='\0';

    temp = malloc(strlen(start)+300+ (ch==0?0:strlen(pt+1)));
    cygwin_conv_to_full_posix_path(start,temp+1);
    temp[0]='"'; strcat(temp,"\" ");
    if ( ch!='\0' )
	strcat(temp,pt+1);
    cmd = malloc(strlen(temp)+strlen(fullspec)+8);
    if ( strstr("%s",temp)!=NULL )
	sprintf( cmd, temp, fullspec );
    else {
	strcpy(cmd,temp);
	strcat(cmd, " ");
	strcat(cmd,fullspec);
    }
    strcat(cmd," &" );
    system(cmd);
    free( cmd ); free( temp ); free( format );
}
#endif

static char browser[1025];

static void findbrowser(void) {
#if __CygWin
    static char *stdbrowsers[] = { "netscape.exe", "opera.exe", "galeon.exe", "kfmclient.exe",
	"mozilla.exe", "mosaic.exe", /*"grail",*/
	"iexplore.exe",
	/*"lynx.exe",*/
#else
/* Both xdg-open and htmlview are not browsers per se, but browser dispatchers*/
/*  which try to figure out what browser the user intents. It seems no one */
/*  uses (understands?) environment variables any more, so BROWSER is a bit */
/*  old-fashioned */
    static char *stdbrowsers[] = { "xdg-open", "htmlview", "firefox", "mozilla", "opera", "galeon", "kfmclient",
	"netscape", "mosaic", /*"grail",*/ "lynx",
#endif
	NULL };
    int i;
    char *path;

    if ( getenv("BROWSER")!=NULL ) {
	strcpy(browser,getenv("BROWSER"));
#if __CygWin			/* Get rid of any dos style names */
	if ( isalpha(browser[0]) && browser[1]==':' && browser[2]=='\\' )
	    cygwin_conv_to_full_posix_path(getenv("BROWSER"),browser);
	else if ( strchr(browser,'/')==NULL ) {
	    if ( strstrmatch(browser,".exe")==NULL )
		strcat(browser,".exe");
	    if ( (path=_GFile_find_program_dir(browser))!=NULL ) {
		snprintf(browser,sizeof(browser),"%s/%s", path, getenv("BROWSER"));
		free(path);
	    }
	}
#endif
	if ( strcmp(browser,"kde")==0 || strcmp(browser,"kfm")==0 ||
		strcmp(browser,"konqueror")==0 || strcmp(browser,"kfmclient")==0 )
	    strcpy(browser,"kfmclient openURL");
return;
    }
    for ( i=0; stdbrowsers[i]!=NULL; ++i ) {
	if ( (path=_GFile_find_program_dir(stdbrowsers[i]))!=NULL ) {
	    if ( strcmp(stdbrowsers[i],"kfmclient")==0 )
		strcpy(browser,"kfmclient openURL");
	    else
#if __CygWin
		snprintf(browser,sizeof(browser),"%s/%s", path, stdbrowsers[i]);
#else
		strcpy(browser,stdbrowsers[i]);
#endif
	    free(path);
return;
	}
    }
#if __Mac
    strcpy(browser,"open");	/* thanks to riggle */
#endif
}

static int SupportedLocale(char *fullspec,char *locale) {
    static char *supported[] = { "ja", NULL };
    int i;

    for ( i=0; supported[i]!=NULL; ++i ) {
	if ( strcmp(locale,supported[i])==0 ) {
	    strcat(fullspec,locale);
	    strcat(fullspec,"/");
return( true );
	}
    }
return( false );
}

static void AppendSupportedLocale(char *fullspec) {
    /* KANOU has provided a japanese translation of the docs */
    /* Edward Lee is working on traditional chinese */
    const char *loc = getenv("LC_ALL");
    char buffer[40], *pt;

    if ( loc==NULL ) loc = getenv("LC_MESSAGES");
    if ( loc==NULL ) loc = getenv("LANG");
    if ( loc==NULL )
return;
    strncpy(buffer,loc,sizeof(buffer));
    if ( SupportedLocale(fullspec,buffer))
return;
    pt = strchr(buffer,'.');
    if ( pt!=NULL ) {
	*pt = '\0';
	if ( SupportedLocale(fullspec,buffer))
return;
    }
    pt = strchr(buffer,'_');
    if ( pt!=NULL ) {
	*pt = '\0';
	if ( SupportedLocale(fullspec,buffer))
return;
    }
}
    
void help(char *file) {
    char fullspec[1024], *temp, *pt;

    if ( browser[0]=='\0' )
	findbrowser();
#ifndef __CygWin
    if ( browser[0]=='\0' ) {
	gwwv_post_error(_("No Browser"),_("Could not find a browser. Set the BROWSER environment variable to point to one"));
return;
    }
#endif

    if ( strstr(file,"http://")==NULL ) {
	fullspec[0] = 0;
	if ( *file!='/' )
	    strcpy(fullspec,getHelpDir());
	strcat(fullspec,file);
	if (( pt = strrchr(fullspec,'#') )!=NULL ) *pt ='\0';
	if ( !GFileReadable( fullspec )) {
	    if ( *file!='/' ) {
		strcpy(fullspec,"/usr/share/doc/fontforge/");
		strcat(fullspec,file);
		if (( pt = strrchr(fullspec,'#') )!=NULL ) *pt ='\0';
	    }
	}
	if ( !GFileReadable( fullspec )) {
	    strcpy(fullspec,"http://fontforge.sf.net/");
	    AppendSupportedLocale(fullspec);
	    strcat(fullspec,file);
	} else if ( pt!=NULL )
	    *pt = '#';
    } else
	strcpy(fullspec,file);
#if __CygWin
    if ( (strstrmatch(browser,"/cygdrive")!=NULL || browser[0]=='\0') &&
		strstr(fullspec,":/")==NULL ) {
	/* It looks as though the browser is a windows application, so we */
	/*  should give it a windows file name */
	char *pt, *tpt;
	temp = malloc(1024);
	cygwin_conv_to_full_win32_path(fullspec,temp);
	for ( pt = fullspec, tpt = temp; *tpt && pt<fullspec+sizeof(fullspec)-3; *pt++ = *tpt++ )
	    if ( *tpt=='\\' )
		*pt++ = '\\';
	*pt = '\0';
	free(temp);
    }
#endif
#if __Mac
    if ( strcmp(browser,"open")==0 )
	/* open doesn't want "file:" prepended */;
    else
#endif
    if ( strstr(fullspec,":/")==NULL ) {
	char *t1 = malloc(strlen(fullspec)+strlen("file:")+20);
#if __CygWin
	sprintf( t1, "file:\\\\\\%s", fullspec );
#else
	sprintf( t1, "file:%s", fullspec);
#endif
	strcpy(fullspec,t1);
	free(t1);
    }
#if __Mac
    /* This seems a bit easier... Thanks to riggle */
    if ( strcmp(browser,"open")==0 ) {
	temp = malloc(strlen(browser) + strlen(fullspec) + 20);
	sprintf( temp, "open \"%s\" &", fullspec );
	system(temp);
	ff_post_notice(_("Leave X"),_("A browser is probably running in the native Mac windowing system. You must leave the X environment to view it. Try Cmd-Opt-A"));
    } else {
#elif __CygWin
    if ( browser[0]=='\0' ) {
	do_windows_browser(fullspec);
	temp = NULL;
    } else {
#else
    {
#endif
	temp = malloc(strlen(browser) + strlen(fullspec) + 20);
	sprintf( temp, strcmp(browser,"kfmclient openURL")==0 ? "%s \"%s\" &" : "\"%s\" \"%s\" &", browser, fullspec );
	system(temp);
    }
    free(temp);
}

#define MAX_ERR_LINES	200
static struct errordata {
    char *errlines[MAX_ERR_LINES];
    int fh, as;
    GtkWidget *vsb;
    GtkWidget *gw, *v;
    PangoLayout *layout;
    int cnt, linecnt;
    int offtop;
    int showing;
} errdata;

static void Warning_Hide(void) {
    gdk_window_hide(errdata.gw->window);
    errdata.showing = false;
}

static void Warning_VScroll(GtkRange *vsb, gpointer user_data) {
    GtkAdjustment *sb;

    sb = gtk_range_get_adjustment(GTK_RANGE(vsb));
    if ( sb->value!=errdata.offtop) {
	int diff = sb->value-errdata.offtop;
	errdata.offtop = sb->value;
	gdk_window_scroll(GDK_WINDOW(errdata.v->window),0,diff*errdata.fh);
    }
}

static gboolean Warning_Resize(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data) {
    GtkAdjustment *sb;

    errdata.linecnt = widget->allocation.width/errdata.fh;
    sb = gtk_range_get_adjustment(GTK_RANGE(errdata.vsb));
    sb->lower = 0; sb->upper = errdata.cnt; sb->page_size = errdata.linecnt;
    sb->step_increment = 1; sb->page_increment = errdata.linecnt;

    if ( errdata.offtop>=errdata.cnt-errdata.linecnt )
        errdata.offtop = errdata.cnt-errdata.linecnt;
    if ( errdata.offtop<0 ) errdata.offtop =0;
    sb->value = errdata.offtop;
    gtk_range_set_adjustment(GTK_RANGE(errdata.vsb),sb);
return 0;
}

static gboolean Warning_Expose(GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    int i;
    for ( i=0; i<errdata.linecnt && i+errdata.offtop<errdata.cnt; ++i ) {
	pango_layout_set_text( errdata.layout, errdata.errlines[i+errdata.offtop], -1);
	gdk_draw_layout( GDK_DRAWABLE(errdata.v->window),errdata.v->style->fg_gc[GTK_STATE_NORMAL],
		3,i*errdata.fh+errdata.as, errdata.layout);
    }
return 1;
}

static void CreateErrorWindow(void) {
    GtkWidget *hbox5;
    GtkWidget *view;
    GtkWidget *vscrollbar2;
    GdkPixbuf *Warning_icon_pixbuf;
    PangoContext *context;
    PangoFont *font;
    PangoFontMetrics *fm;
    int as, ds;
    GtkRequisition desired;

    errdata.gw = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_name (errdata.gw, "Warnings");
    gtk_widget_set_events (errdata.gw, GDK_EXPOSURE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_PROPERTY_CHANGE_MASK);
    gtk_window_set_title (GTK_WINDOW (errdata.gw), _("Warnings"));
    Warning_icon_pixbuf = create_pixbuf ("fontview2.xbm");
    if (Warning_icon_pixbuf) {
	gtk_window_set_icon (GTK_WINDOW (errdata.gw), Warning_icon_pixbuf);
	gdk_pixbuf_unref (Warning_icon_pixbuf);
    }

    hbox5 = gtk_hbox_new (FALSE, 0);
    gtk_widget_set_name (hbox5, "hbox5");
    gtk_widget_show (hbox5);
    gtk_container_add (GTK_CONTAINER (errdata.gw), hbox5);

    view = gtk_drawing_area_new ();
    gtk_widget_set_name (view, "view");
    gtk_widget_show (view);
    gtk_box_pack_start (GTK_BOX (hbox5), view, TRUE, TRUE, 0);
    gtk_widget_set_size_request (view, 16*24+1, 4*24+1);

    vscrollbar2 = gtk_vscrollbar_new (GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 0, 0, 0, 0)));
    gtk_widget_set_name (vscrollbar2, "vscrollbar2");
    gtk_widget_show (vscrollbar2);
    gtk_box_pack_start (GTK_BOX (hbox5), vscrollbar2, FALSE, TRUE, 0);

    g_signal_connect ((gpointer) errdata.gw, "delete_event",
		      G_CALLBACK (Warning_Hide),
		      NULL);
    g_signal_connect ((gpointer) vscrollbar2, "value_changed",
		      G_CALLBACK (Warning_VScroll),
		      NULL);

    g_signal_connect ((gpointer) view, "configure_event",
		      G_CALLBACK (Warning_Resize),
		      NULL);
    g_signal_connect ((gpointer) view, "expose_event",
		      G_CALLBACK (Warning_Expose),
		      NULL);

    errdata.v   = view;
    errdata.vsb = vscrollbar2;
    errdata.layout = gtk_widget_create_pango_layout( view, NULL );
    pango_layout_set_width(errdata.layout, -1);		/* Don't wrap long lines */

    context = gtk_widget_get_pango_context( view );
    font = pango_context_load_font( context, pango_context_get_font_description(context));
    fm = pango_font_get_metrics(font,NULL);
    as = pango_font_metrics_get_ascent(fm);
    ds = pango_font_metrics_get_descent(fm);
    errdata.as = as / PANGO_SCALE;
    errdata.fh = (as+ds) / PANGO_SCALE;

    gtk_widget_set_size_request(view, 40*errdata.fh, 5*errdata.fh );

    gtk_widget_size_request(errdata.gw,&desired);
    /* This function is deprecated, but I can find no other way to position */
    /*  a window in the bottom right corner (or at all). So I use it */
    gtk_widget_set_uposition(errdata.gw,
	    gdk_screen_get_width(gdk_screen_get_default())-desired.width-5,
	    gdk_screen_get_height(gdk_screen_get_default())-desired.height-errdata.fh-5);

    errdata.linecnt = 5;

    gtk_widget_show(errdata.gw);
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
    Warning_Resize(errdata.gw,NULL,NULL);
}

int ErrorWindowExists(void) {
return( errdata.gw!=NULL );
}

void ShowErrorWindow(void) {
    if ( errdata.gw==NULL )
return;
    gdk_window_show(errdata.gw->window);
    if ( errdata.showing )
	gtk_widget_queue_draw(errdata.v);
    errdata.showing = true;
}

static void _LogError(const char *format,va_list ap) {
    char buffer[400];
    vsnprintf(buffer,sizeof(buffer),format,ap);
    if ( !ErrorWindowExists())
	CreateErrorWindow();
    AppendToErrorWindow(buffer);
    ShowErrorWindow();
}

static void UI_LogError(const char *format,...) {
    va_list ap;

    va_start(ap,format);
    _LogError(format,ap);
    va_end(ap);
}

static void tinysleep(int microsecs) {
    fd_set none;
    struct timeval timeout;

    FD_ZERO(&none);
    memset(&timeout,0,sizeof(timeout));
    timeout.tv_usec = microsecs;

    select(1,&none,&none,&none,&timeout);
}

static void allow_events(void) {
    g_main_context_iteration(NULL,false);
    tinysleep(100);
    g_main_context_iteration(NULL,false);
}

/* Stubs! */
static int NOUI_DefaultStrokeFlags(void) {
return( sf_correctdir );
}    

struct ui_interface gtk_ui_interface = {
    gwwv_ierror,
    gwwv_post_error,
    UI_LogError,
    gwwv_post_notice,
    gwwv_ask,
    gwwv_choose,
    gwwv_choose_multiple,
    gwwv_ask_string,
    gwwv_ask_string,			/* Should be gwwv_ask_password !!!!! */
    gwwv_open_filename,
    gwwv_saveas_filename,

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

    NOUI_TTFNameIds,
    NOUI_MSLangString,

    NOUI_DefaultStrokeFlags
};
