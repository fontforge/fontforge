/* Copyright (C) 2000-2006 by George Williams */
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
#include <stdarg.h>
#include <unistd.h>
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include <utype.h>
#include <ustring.h>

#if __CygWin
#include <unistd.h>
extern void cygwin_conv_to_full_posix_path(const char *win,char *unx);
extern void cygwin_conv_to_full_win32_path(const char *unx,char *win);
#endif

void Protest8(char *label) {
    char buf[80];

    snprintf( buf, sizeof(buf),_("Bad Number in %s"), label);
    if ( buf[strlen(buf)-1]==' ' )
	buf[strlen(buf)-1]='\0';
    if ( buf[strlen(buf)-1]==':' )
	buf[strlen(buf)-1]='\0';
    ff_post_notice(buf,buf);
}

real GetCalmReal8(GWindow gw,int cid,char *name,int *err) {
    char *txt, *end;
    real val;

    txt = GGadgetGetTitle8(GWidgetGetControl(gw,cid));
    val = strtod(txt,&end);
    if ( *txt=='-' && end==txt && txt[1]=='\0' )
	end = txt+1;
    if ( *end!='\0' ) {
	GDrawBeep(NULL);
	*err = true;
    }
    free(txt);
return( val );
}

real GetReal8(GWindow gw,int cid,char *name,int *err) {
    char *txt, *end;
    real val;

    txt = GGadgetGetTitle8(GWidgetGetControl(gw,cid));
    val = strtod(txt,&end);
    if ( *end!='\0' ) {
	GTextFieldSelect(GWidgetGetControl(gw,cid),0,-1);
	Protest8(name);
	*err = true;
    }
    free(txt);
return( val );
}

int GetInt8(GWindow gw,int cid,char *name,int *err) {
    char *txt, *end;
    int val;

    txt = GGadgetGetTitle8(GWidgetGetControl(gw,cid));
    val = strtol(txt,&end,10);
    if ( *end!='\0' ) {
	GTextFieldSelect(GWidgetGetControl(gw,cid),0,-1);
	Protest8(name);
	*err = true;
    }
    free(txt);
return( val );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

extern char *helpdir;

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
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("No Browser"),_("Could not find a browser. Set the BROWSER environment variable to point to one"));
#else
	gwwv_post_error(_("No Browser"),_("Could not find a browser. Set the BROWSER environment variable to point to one"));
#endif
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

    temp = galloc(strlen(start)+300+ (ch==0?0:strlen(pt+1)));
    cygwin_conv_to_full_posix_path(start,temp+1);
    temp[0]='"'; strcat(temp,"\" ");
    if ( ch!='\0' )
	strcat(temp,pt+1);
    cmd = galloc(strlen(temp)+strlen(fullspec)+8);
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
    static char *stdbrowsers[] = { "htmlview", "firefox", "mozilla", "opera", "galeon", "kfmclient",
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
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("No Browser"),_("Could not find a browser. Set the BROWSER environment variable to point to one"));
#else
	gwwv_post_error(_("No Browser"),_("Could not find a browser. Set the BROWSER environment variable to point to one"));
#endif
return;
    }
#endif

    if ( strstr(file,"http://")==NULL ) {
	fullspec[0] = 0;
	if ( *file!='/' ) {
	    if ( helpdir==NULL || *helpdir=='\0' ) {
#ifdef DOCDIR
		strcpy(fullspec,DOCDIR "/");
#elif defined(SHAREDIR)
		strcpy(fullspec,SHAREDIR "/../doc/fontforge/");
#else
		strcpy(fullspec,"/usr/local/share/doc/fontforge/");
#endif
	    } else
		strcpy(fullspec,helpdir);
	}
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
	temp = galloc(1024);
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
	char *t1 = galloc(strlen(fullspec)+strlen("file:")+20);
#if __CygWin
	sprintf( t1, "file:\\\\\\%s", fullspec );
#else
	sprintf( t1, "file:%s", fullspec);
#endif
	strcpy(fullspec,t1);
	free(t1);
    }
#if 0 && __Mac
    /* Starting a Mac application is weird... system() can't do it */
    /* Thanks to Edward H. Trager giving me an example... */
    if ( strstr(browser,".app")!=NULL ) {
	*strstr(browser,".app") = '\0';
	pt = strrchr(browser,'/');
	if ( pt==NULL ) pt = browser-1;
	++pt;
	temp = galloc(strlen(pt)+strlen(fullspec) +
		strlen( "osascript -l AppleScript -e \"Tell application \"\" to getURL \"\"\"" )+
		20);
	/* this doesn't work on Max OS X.0 (osascript does not support -e) */
	sprintf( temp, "osascript -l AppleScript -e \"Tell application \"%s\" to getURL \"%s\"\"",
	    pt, fullspec);
	system(temp);
#if defined(FONTFORGE_CONFIG_GDRAW)
	ff_post_notice(_("Leave X"),_("A browser is probably running in the native Mac windowing system. You must leave the X environment to view it. Try Cmd-Opt-A"));
#elif defined(FONTFORGE_CONFIG_GTK)
	ff_post_notice(_("Leave X"),_("A browser is probably running in the native Mac windowing system. You must leave the X environment to view it. Try Cmd-Opt-A"));
#endif
    } else {
#elif __Mac
    /* This seems a bit easier... Thanks to riggle */
    if ( strcmp(browser,"open")==0 ) {
	temp = galloc(strlen(browser) + strlen(fullspec) + 20);
	sprintf( temp, "open \"%s\" &", fullspec );
	system(temp);
#if defined(FONTFORGE_CONFIG_GDRAW)
	ff_post_notice(_("Leave X"),_("A browser is probably running in the native Mac windowing system. You must leave the X environment to view it. Try Cmd-Opt-A"));
#elif defined(FONTFORGE_CONFIG_GTK)
	ff_post_notice(_("Leave X"),_("A browser is probably running in the native Mac windowing system. You must leave the X environment to view it. Try Cmd-Opt-A"));
#endif
    } else {
#elif __CygWin
    if ( browser[0]=='\0' ) {
	do_windows_browser(fullspec);
	temp = NULL;
    } else {
#else
    {
#endif
	temp = galloc(strlen(browser) + strlen(fullspec) + 20);
	sprintf( temp, strcmp(browser,"kfmclient openURL")==0 ? "%s \"%s\" &" : "\"%s\" \"%s\" &", browser, fullspec );
	system(temp);
    }
    free(temp);
}

void IError(const char *format,...) {
    va_list ap;
#if defined( FONTFORGE_CONFIG_NO_WINDOWING_UI )
    va_start(ap,format);
    fprintf(stderr, "Internal Error: " );
    vfprintf(stderr,format,ap);
#elif defined( FONTFORGE_CONFIG_GDRAW )
    char buffer[300];
    va_start(ap,format);
    vsnprintf(buffer,sizeof(buffer),format,ap);
    GDrawIError("%s",buffer);
#elif defined( FONTFORGE_CONFIG_GTK )
    char buffer[340];
    int len;
    va_start(ap,format);
    strcpy(buffer,_("Internal Error: "));
    len = strlen(buffer);
    vsnprintf(buffer+len,sizeof(buffer)-len,format,ap);
#endif
    va_end(ap);
}

#if !defined( FONTFORGE_CONFIG_NO_WINDOWING_UI )
#define MAX_ERR_LINES	200
static struct errordata {
    char *errlines[MAX_ERR_LINES];
    GFont *font;
    int fh, as;
    GGadget *vsb;
    GWindow gw, v;
    int cnt, linecnt;
    int offtop;
    int showing;
} errdata;

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
    }
    if ( newpos>errdata.cnt-errdata.linecnt )
        newpos = errdata.cnt-errdata.linecnt;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=errdata.offtop ) {
	int diff = newpos-errdata.offtop;
	errdata.offtop = newpos;
	GScrollBarSetPos(errdata.vsb,errdata.offtop);
	GDrawScroll(errdata.v,NULL,0,diff*errdata.fh);
    }
}

static int warnings_e_h(GWindow gw, GEvent *event) {

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button==4 || event->u.mouse.button==5) ) {
return( GGadgetDispatchEvent(errdata.vsb,event));
    }

    switch ( event->type ) {
      case et_selclear:
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
	  if ( errdata.offtop + errdata.linecnt > errdata.cnt ) {
	      errdata.offtop = errdata.cnt-errdata.linecnt;
	      if ( errdata.offtop < 0 ) errdata.offtop = 0;
	      GScrollBarSetBounds(errdata.vsb,0,errdata.cnt,errdata.linecnt);
	      GScrollBarSetPos(errdata.vsb,errdata.offtop);
	  }
	  GDrawRequestExpose(errdata.v,NULL,false);
      } break;
      case et_char:
return( false );
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    ErrScroll(&event->u.control.u.sb);
	  break;
	}
      break;
      case et_close:
	ErrHide();
      break;
      case et_create:
      break;
      case et_destroy:
      break;
    }
return( true );
}

static int warningsv_e_h(GWindow gw, GEvent *event) {
    int i;

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button==4 || event->u.mouse.button==5) ) {
return( GGadgetDispatchEvent(errdata.vsb,event));
    }

    switch ( event->type ) {
      case et_expose:
	  GDrawSetFont(gw,errdata.font);
	  for ( i=0; i<errdata.linecnt && i+errdata.offtop<errdata.cnt; ++i ) {
	      GDrawDrawText8(gw,3,i*errdata.fh+errdata.as,errdata.errlines[i+errdata.offtop],-1,NULL,0x000000);
	  }
      break;
      case et_char:
return(false);
      break;
      case et_mousemove: case et_mousedown: case et_mouseup:
      break;
      case et_timer:
      break;
      case et_focus:
      break;
    }
return( true );
}

static void CreateErrorWindow(void) {
    static unichar_t sans[] = { 'h','e','l','v','e','t','i','c','a',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
    GWindowAttrs wattrs;
    FontRequest rq;
    GRect pos,size;
    int as, ds, ld;
    GWindow gw;
    GGadgetData gd;
    extern int _GScrollBar_Width;

    memset(&rq,0,sizeof(rq));
    rq.family_name = sans;
    rq.point_size = 10;
    rq.weight = 400;
    errdata.font = GDrawInstanciateFont(NULL,&rq);
    GDrawFontMetrics(errdata.font,&as,&ds,&ld);
    errdata.as = as;
    errdata.fh = as+ds;

    GDrawGetSize(GDrawGetRoot(NULL),&size);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_isdlg|wam_positioned;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.cursor = ct_pointer;
    wattrs.positioned = true;
    wattrs.utf8_window_title = _("Warnings");
    pos.width = GDrawPointsToPixels(NULL,GGadgetScale(400));
    pos.height = 5*errdata.fh;
    pos.x = size.width - pos.width - 10;
    pos.y = size.height - pos.height - 30;
    errdata.gw = gw = GDrawCreateTopWindow(NULL,&pos,warnings_e_h,&errdata,&wattrs);

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
#endif

static void _LogError(const char *format,va_list ap) {
    char buffer[400], *str;
    vsnprintf(buffer,sizeof(buffer),format,ap);
#if defined( FONTFORGE_CONFIG_NO_WINDOWING_UI )
    str = utf82def_copy(buffer);
    fprintf(stderr,"%s",str);
    free(str);
#else
    if ( no_windowing_ui ) {
	str = utf82def_copy(buffer);
	fprintf(stderr,"%s",str);
	free(str);
    } else {
	if ( !ErrorWindowExists())
	    CreateErrorWindow();
	AppendToErrorWindow(buffer);
	ShowErrorWindow();
    }
#endif
}

void LogError(const char *format,...) {
    va_list ap;

    va_start(ap,format);
    _LogError(format,ap);
    va_end(ap);
}

void ff_post_notice(const char *title,const char *statement,...) {
    va_list ap;
    va_start(ap,statement);
#if defined( FONTFORGE_CONFIG_NO_WINDOWING_UI )
    _LogError(statement,ap);
#else
    if ( no_windowing_ui ) {
	_LogError(statement,ap);
    } else {
	if ( GWidgetPostNoticeActive8(title))
	    _LogError(statement,ap);
	else
	    _GWidgetPostNotice8(title,statement,ap);
    }
#endif
    va_end(ap);
}
