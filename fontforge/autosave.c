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

#include "autosave.h"

#include "baseviews.h"
#include "ffglib_compat.h"
#include "fontforgevw.h"
#include "sfd.h"
/*#include "ustring.h"*/
#include "gfile.h"
#include "gwidget.h"
#include "ustring.h"

#include "ffdir.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "ffunistd.h"

int AutoSaveFrequency=5;

#if !defined(__MINGW32__) && !defined(_MSC_VER)
# include <pwd.h>
#endif

static char *getAutoDirName(void) {
    char *buffer, *dir=getFontForgeUserDir(Config);

    if ( dir!=NULL ) {
        buffer = smprintf("%s/autosave", dir);
        free(dir);
        if ( ff_access(buffer,F_OK)==-1 )
            if ( GFileMkDir(buffer, 0755)==-1 ) {
		free(buffer);
                return NULL;
	    }
        return buffer;
    } else
	return NULL;
}

static void MakeAutoSaveName(SplineFont *sf) {
    char *autosavedir, *buffer;
    static int cnt=0;

    if ( sf->autosavename )
	return;
    autosavedir = getAutoDirName();
    if ( autosavedir==NULL )
	return;
    while ( 1 ) {
	buffer = smprintf("%s/auto%06x-%d.asfd", autosavedir, getpid(), ++cnt);
	if ( ff_access(buffer,F_OK)==-1 ) {
	    sf->autosavename = buffer;
            free(autosavedir);
	    return;
	} else {
	    free(buffer);
	}
    }
}


int DoAutoRecoveryExtended(int inquire)
{
    char *buffer;
    char *recoverdir = getAutoDirName();
    FF_Dir *dir;
    FF_DirEntry *entry;
    int any = false;
    SplineFont *sf = NULL;
    int inquire_state=0;

    if ( recoverdir==NULL )
return( false );
    if ( (dir = ff_opendir(recoverdir))==NULL ) {
        free(recoverdir);
return( false );
    }
    while ( (entry=ff_readdir(dir))!=NULL ) {
	if ( strcmp(entry->name,".")==0 || strcmp(entry->name,"..")==0 )
    continue;
	buffer = smprintf("%s/%s",recoverdir,entry->name);
	fprintf( stderr, "Recovering from %s... ", buffer);
	if ( (sf = SFRecoverFile(buffer,inquire,&inquire_state)) ) {
	    any=true;
	    if ( sf->fv==NULL )		/* Doesn't work, cli arguments not parsed yet */
		FontViewCreate(sf,false);
	    fprintf( stderr, " Done\n" );
	}
	free(buffer);
    }
    free(recoverdir);
    ff_closedir(dir);
return( any );
}

int DoAutoRecovery(int inquire )
{
    return DoAutoRecoveryExtended( inquire );
}


void CleanAutoRecovery(void) {
    char *buffer;
    char *recoverdir = getAutoDirName();
    FF_Dir *dir;
    FF_DirEntry *entry;

    if ( recoverdir==NULL )
return;
    if ( (dir = ff_opendir(recoverdir))==NULL ) {
        free(recoverdir);
return;
    }
    while ( (entry=ff_readdir(dir))!=NULL ) {
	if ( strcmp(entry->name,".")==0 || strcmp(entry->name,"..")==0 )
    continue;
	buffer = smprintf("%s/%s",recoverdir,entry->name);
	if ( ff_unlink(buffer)!=0 ) {
	    fprintf( stderr, "Failed to clean " );
	    perror(buffer);
	}
	free(buffer);
    }
    free(recoverdir);
    ff_closedir(dir);
}


static void _DoAutoSaves(FontViewBase *fvs) {
    FontViewBase *fv;
    SplineFont *sf;

    if ( AutoSaveFrequency<=0 )
return;

    for ( fv=fvs; fv!=NULL; fv=fv->next ) {
	sf = fv->cidmaster?fv->cidmaster:fv->sf;
	if ( sf->changed_since_autosave ) {
	    if ( sf->autosavename==NULL )
		MakeAutoSaveName(sf);
	    if ( sf->autosavename!=NULL )
		SFAutoSave(sf,fv->map);
	}
    }
}

void DoAutoSaves(void) {
    _DoAutoSaves(FontViewFirst());
}
