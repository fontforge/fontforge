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
/*#include "ustring.h"*/
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>

#ifdef FONTFORGE_CONFIG_GTK
# include <gtk/gtk.h>

static char *gethomedir(void) {
    static char *dir=NULL;

    if ( dir!=NULL )
return( dir );
return( dir = g_get_home_dir());
}
#else 
# include <pwd.h>

static char *gethomedir(void) {
    static char *dir;
    int uid;
    struct passwd *pw;

	dir = getenv("HOME");
    if ( dir!=NULL )
return( strdup(dir) );

    uid = getuid();
    while ( (pw=getpwent())!=NULL ) {
	if ( pw->pw_uid==uid ) {
	    dir = strdup(pw->pw_dir);
	    endpwent();
return( dir );
	}
    }
    endpwent();

return( NULL );
}
#endif

char *getPfaEditDir(char *buffer) {
    char *dir=gethomedir();

    if ( dir==NULL )
return( NULL );
    sprintf(buffer,"%s/.PfaEdit", dir);
    free(dir);
    if ( access(buffer,F_OK)==-1 )
	if ( mkdir(buffer,0700)==-1 )
return( NULL );
    dir = strdup(buffer);
return( dir );
}

static char *getAutoDirName(char *buffer) {
    char *dir=getPfaEditDir(buffer);

    if ( dir==NULL )
return( NULL );
    sprintf(buffer,"%s/autosave", dir);
    free(dir);
    if ( access(buffer,F_OK)==-1 )
	if ( mkdir(buffer,0700)==-1 )
return( NULL );
    dir = strdup(buffer);
return( dir );
}

static void MakeAutoSaveName(SplineFont *sf) {
    char buffer[1025];
    char *autosavedir;
    static int cnt=0;

    if ( sf->autosavename )
return;
    autosavedir = getAutoDirName(buffer);
    if ( autosavedir==NULL )
return;
    while ( 1 ) {
	sprintf( buffer, "%s/auto%06x-%d.asfd", autosavedir, getpid(), ++cnt );
	if ( access(buffer,F_OK)==-1 ) {
	    sf->autosavename = strdup(buffer);
	    free(autosavedir);
return;
	}
    }
}

int DoAutoRecovery(void) {
    char buffer[1025];
    char *recoverdir = getAutoDirName(buffer);
    DIR *dir;
    struct dirent *entry;
    int any = false;
    SplineFont *sf;

    if ( recoverdir==NULL )
return( false );
    if ( (dir = opendir(recoverdir))==NULL )
return( false );
    while ( (entry=readdir(dir))!=NULL ) {
	if ( strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0 )
    continue;
	sprintf(buffer,"%s/%s",recoverdir,entry->d_name);
	fprintf( stderr, "Recovering from %s... ", buffer);
	if ( (sf = SFRecoverFile(buffer)) ) {
	    any=true;
	    if ( sf->fv==NULL )		/* Doesn't work, cli arguments not parsed yet */
		FontViewCreate(sf);
	}
	fprintf( stderr, " Done\n" );
    }
    closedir(dir);
return( any );
}

void CleanAutoRecovery(void) {
    char buffer[1025];
    char *recoverdir = getAutoDirName(buffer);
    DIR *dir;
    struct dirent *entry;

    if ( recoverdir==NULL )
return;
    if ( (dir = opendir(recoverdir))==NULL )
return;
    while ( (entry=readdir(dir))!=NULL ) {
	if ( strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0 )
    continue;
	sprintf(buffer,"%s/%s",recoverdir,entry->d_name);
	if ( unlink(buffer)!=0 ) {
	    fprintf( stderr, "Failed to clean " );
	    perror(buffer);
	}
    }
    closedir(dir);
}

void DoAutoSaves(void) {
    FontView *fv;
    SplineFont *sf;

    for ( fv=fv_list; fv!=NULL; fv=fv->next ) {
	sf = fv->cidmaster?fv->cidmaster:fv->sf;
	if ( sf->changed_since_autosave ) {
	    if ( sf->autosavename==NULL )
		MakeAutoSaveName(sf);
	    if ( sf->autosavename!=NULL )
		SFAutoSave(sf,fv->map);
	}
    }
}
