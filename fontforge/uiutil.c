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
#include <utype.h>
#include <ustring.h>
#include <gfile.h>

extern char *helpdir;

#if __CygWin
#include <unistd.h>
extern void cygwin_conv_to_full_posix_path(const char *win,char *unx);
extern void cygwin_conv_to_full_win32_path(const char *unx,char *win);
#endif

void Protest(char *label) {
    char buffer[80];
    unichar_t ubuf[80];
    sprintf( buffer, "Bad Number in %s", label );
    uc_strcpy(ubuf,buffer);
    GWidgetPostNotice(ubuf,ubuf);
}

real GetReal(GWindow gw,int cid,char *name,int *err) {
    const unichar_t *txt; unichar_t *end;
    double val;

    txt = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    val = u_strtod(txt,&end);
    if ( *end!='\0' ) {
	GTextFieldSelect(GWidgetGetControl(gw,cid),0,-1);
	Protest(name);
	*err = true;
    }
return( val );
}

int GetInt(GWindow gw,int cid,char *name,int *err) {
    const unichar_t *txt; unichar_t *end;
    int val;

    txt = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    val = u_strtol(txt,&end,10);
    if ( *end!='\0' ) {
	GTextFieldSelect(GWidgetGetControl(gw,cid),0,-1);
	Protest(name);
	*err = true;
    }
return( val );
}

void ProtestR(int labelr) {
    unichar_t ubuf[80];
    u_strcpy(ubuf,GStringGetResource(_STR_Badnumberin,NULL));
    u_strcat(ubuf,GStringGetResource(labelr,NULL));
    if ( ubuf[u_strlen(ubuf)-1]==' ' )
	ubuf[u_strlen(ubuf)-1]='\0';
    if ( ubuf[u_strlen(ubuf)-1]==':' )
	ubuf[u_strlen(ubuf)-1]='\0';
    GWidgetPostNotice(ubuf,ubuf);
}

real GetCalmRealR(GWindow gw,int cid,int namer,int *err) {
    const unichar_t *txt; unichar_t *end;
    real val;

    txt = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    val = u_strtod(txt,&end);
    if ( *txt=='-' && end==txt && txt[1]=='\0' )
return( 0 );
    if ( *end!='\0' ) {
	GTextFieldSelect(GWidgetGetControl(gw,cid),0,-1);
	ProtestR(namer);
	*err = true;
    }
return( val );
}

real GetRealR(GWindow gw,int cid,int namer,int *err) {
    const unichar_t *txt; unichar_t *end;
    real val;

    txt = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    val = u_strtod(txt,&end);
    if ( *end!='\0' ) {
	GTextFieldSelect(GWidgetGetControl(gw,cid),0,-1);
	ProtestR(namer);
	*err = true;
    }
return( val );
}

int GetIntR(GWindow gw,int cid,int namer,int *err) {
    const unichar_t *txt; unichar_t *end;
    int val;

    txt = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    val = u_strtol(txt,&end,10);
    if ( *end!='\0' ) {
	GTextFieldSelect(GWidgetGetControl(gw,cid),0,-1);
	ProtestR(namer);
	*err = true;
    }
return( val );
}

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
	fprintf( stderr, "Failed to default value of exten \"%s\".\n Error=%ld", exten, err );
	RegCloseKey(hkey_exten);
return( NULL );
    }
    RegCloseKey(hkey_exten);

    if ( RegOpenKeyEx(HKEY_CLASSES_ROOT,programindicator,0,KEY_READ,&hkey_prog)!=ERROR_SUCCESS ) {
	fprintf( stderr, "Failed to find program \"%s\"\n", programindicator );
return( NULL );
    }
    if ( RegOpenKeyEx(hkey_prog,"shell",0,KEY_READ,&hkey_shell)!=ERROR_SUCCESS ) {
	fprintf( stderr, "Failed to find \"%s->shell\"\n", programindicator );
	RegCloseKey(hkey_prog);
return( NULL );
    }
    if ( RegOpenKeyEx(hkey_shell,"open",0,KEY_READ,&hkey_open)!=ERROR_SUCCESS ) {
	fprintf( stderr, "Failed to find \"%s->shell->open\"\n", programindicator );
	RegCloseKey(hkey_prog); RegCloseKey(hkey_shell);
return( NULL );
    }
    if ( RegOpenKeyEx(hkey_open,"command",0,KEY_READ,&hkey_command)!=ERROR_SUCCESS ) {
	fprintf( stderr, "Failed to find \"%s->shell->open\"\n", programindicator );
	RegCloseKey(hkey_prog); RegCloseKey(hkey_shell); RegCloseKey(hkey_command);
return( NULL );
    }

    dlen = sizeof(programpath);
    if ( RegQueryValueEx(hkey_command,"",NULL,&type,(uint8 *)programpath,&dlen)!=ERROR_SUCCESS ) {
	fprintf( stderr, "Failed to find default for \"%s->shell->open->command\"\n", programindicator );
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
	GDrawError("Could not find a browser. Set the BROWSER environment variable to point to one" );
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
    sprintf( cmd, temp, fullspec );
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
    static char *stdbrowsers[] = { "mozilla", "opera", "galeon", "kfmclient",
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

void help(char *file) {
    char fullspec[1024], *temp, *pt;

    if ( browser[0]=='\0' )
	findbrowser();
#ifndef __CygWin
    if ( browser[0]=='\0' ) {
	GDrawError("Could not find a browser. Set the BROWSER environment variable to point to one" );
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
	GWidgetPostNoticeR(_STR_LeaveX,_STR_LeaveXLong);
    } else {
#elif __Mac
    /* This seems a bit easier... Thanks to riggle */
    if ( strcmp(browser,"open")==0 ) {
	temp = galloc(strlen(browser) + strlen(fullspec) + 20);
	sprintf( temp, "open \"%s\" &", fullspec );
	system(temp);
	GWidgetPostNoticeR(_STR_LeaveX,_STR_LeaveXLong);
    } else {
#elif __CygWin
    if ( browser[0]=='\0' ) {
	do_windows_browser(fullspec);
    } else {
#else
    {
#endif
	temp = galloc(strlen(browser) + strlen(fullspec) + 20);
	sprintf( temp, "\"%s\" \"%s\" &", browser, fullspec );
	system(temp);
    }
    free(temp);
}
