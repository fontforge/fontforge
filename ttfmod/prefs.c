/* Copyright (C) 2001 by George Williams */
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
#include "ttfmodui.h"
#include "gfile.h"
#include "gresource.h"
#include "ustring.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <locale.h>
#include <unistd.h>
#include <pwd.h>

char *RecentFiles[RECENT_MAX];
/* int local_encoding; */		/* in gresource.c *//* not a charset */

static char *gethomedir(void) {
    static char *dir=NULL;
    int uid;
    struct passwd *pw;

    if ( dir!=NULL )
return( dir );

    if ( (dir=getenv("HOME"))!=NULL )
return( (dir=copy(dir)) );

    uid = getuid();
    while ( (pw=getpwent())!=NULL ) {
	if ( pw->pw_uid==uid ) {
	    dir = copy(pw->pw_dir);
	    endpwent();
return( dir );
	}
    }
    endpwent();
return( NULL );
}

char *getPfaEditDir(char *buffer) {
    static char *dir=NULL;

    if ( dir!=NULL )
return( dir );
    if ( gethomedir()==NULL )
return( NULL );
    sprintf(buffer,"%s/.PfaEdit", gethomedir());
    if ( access(buffer,F_OK)==-1 )
	if ( mkdir(buffer,0700)==-1 )
return( NULL );
    dir = copy(buffer);
return( dir );
}

static char *getTtfModPrefs(void) {
    static char *prefs=NULL;
    char buffer[1025];

    if ( prefs!=NULL )
return( prefs );
    if ( getPfaEditDir(buffer)==NULL )
return( NULL );
    sprintf(buffer,"%s/ttfprefs", getPfaEditDir(buffer));
    prefs = copy(buffer);
return( prefs );
}

static void CheckLang(void) {
    /*const char *loc = setlocale(LC_MESSAGES,NULL);*/ /* This always returns "C" for me, even when it shouldn't be */
    const char *loc = getenv("LC_ALL");
    char buffer[100], full[1024];

    if ( loc==NULL ) loc = getenv("LC_MESSAGES");
    if ( loc==NULL ) loc = getenv("LANG");

    if ( loc==NULL )
return;

    strcpy(buffer,"ttfmod.");
    strcat(buffer,loc);
    strcat(buffer,".ui");
    GFileBuildName(GResourceProgramDir,buffer,full,sizeof(full));
    /* Look for language_territory */
    if ( !GFileExists(full) && strlen(loc)>5 ) {
	strcpy(buffer+13,".ui");
	GFileBuildName(GResourceProgramDir,buffer,full,sizeof(full));
    }
    /* Look for language */
    if ( !GFileExists(full) && strlen(loc)>2 ) {
	strcpy(buffer+10,".ui");
	GFileBuildName(GResourceProgramDir,buffer,full,sizeof(full));
    }
    if ( !GFileExists(full)) {
	GFileBuildName("/usr/share/ttfmod",buffer,full,sizeof(full));
	if ( !GFileExists(full) && strlen(loc)>5 ) {
	    strcpy(buffer+13,".ui");
	    GFileBuildName(GResourceProgramDir,buffer,full,sizeof(full));
	}
	/* Look for language */
	if ( !GFileExists(full) && strlen(loc)>2 ) {
	    strcpy(buffer+10,".ui");
	    GFileBuildName(GResourceProgramDir,buffer,full,sizeof(full));
	}
	if ( !GFileExists(full))
return;
    }

    GStringSetResourceFile(full);
}
    
void LoadPrefs(void) {

    TtfModSetFallback();
    CheckLang();
}

void DoPrefs(void) {
}
