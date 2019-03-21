/* Copyright (C) 2000-2003 by George Williams */
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
#include "giofuncP.h"
#include <gfile.h>
#include <ustring.h>
#include <errno.h>

static struct protocols {
    int index;
    unichar_t *proto;
    void *handle;
    void *(*dispatcher)(GIOControl *gc);
    void (*cancel)(GIOControl *gc);
    void (*term)(void *);
    unsigned int dothread: 1;
} *protocols;
static int plen, pmax;

static unichar_t err501[] = { ' ','N','o','t',' ','I','m','p','l','e','m','e','n','t','e','d', '\0' };

static int AddProtocol(unichar_t *prefix,int len) {

    if ( plen>=pmax ) {
	pmax += 20;		/* We're never going to support 20 protocols? */
	if ( plen==0 ) {
	    protocols = (struct protocols *) malloc(pmax*sizeof(struct protocols));
	} else {
	    protocols = (struct protocols *) realloc(protocols,pmax*sizeof(struct protocols));
	}
    }
    memset(protocols+plen,0,sizeof(struct protocols));
    if ( uc_strncmp(prefix,"file",len)==0 ) {
	protocols[plen].handle = NULL;
	protocols[plen].dispatcher = _GIO_fileDispatch;
	protocols[plen].cancel = NULL;
	protocols[plen].term = NULL;
	protocols[plen].dothread = false;
    } else {
return( false );
    }
    protocols[plen].index = plen;
    protocols[plen].proto = u_copyn(prefix,len);
    ++plen;
return( true );
}

static void GIOdispatch(GIOControl *gc, enum giofuncs gf) {
    unichar_t *temp, *pt, *tpt;
    int i;

    gc->gf = gf;

    temp = _GIO_translateURL(gc->path,gf);
    if ( temp!=NULL ) {
	if ( gc->origpath==NULL )
	    gc->origpath = gc->path;
	else
	    free(gc->path);
	gc->path = temp;
    }
    if ( gc->topath!=NULL ) {
	temp = _GIO_translateURL(gc->topath,gf);
	if ( temp!=NULL ) {
	    free(gc->topath);
	    gc->topath = temp;
	}
	if ( gf==gf_renamefile ) {
	    if (( pt = uc_strstr(gc->path,"://"))== NULL )
		pt = gc->path;
	    else {
		pt=u_strchr(pt+3,'/');
		if ( pt==NULL ) pt = gc->path+u_strlen(gc->path);
	    }
	    if (( tpt = uc_strstr(gc->topath,"://"))== NULL )
		tpt = gc->topath;
	    else {
		tpt=u_strchr(tpt+3,'/');
		if ( tpt==NULL ) tpt = gc->topath+u_strlen(gc->topath);
	    }
	    if ( tpt-gc->topath!=pt-gc->path ||
		    u_strnmatch(gc->path,gc->topath,pt-gc->path)!=0 ) {
		_GIO_reporterror(gc,EXDEV);
return;
	    }
	}
    }

    pt = uc_strstr(gc->path,"://");
    if ( pt!=NULL ) {
	for ( i=0; i<plen; ++i )
	    if ( u_strnmatch(protocols[i].proto,gc->path,pt-gc->path)==0 )
	break;
	if ( i>=plen && !AddProtocol(gc->path,pt-gc->path) ) {
	    gc->protocol_index = -2;
	    gc->return_code = 501;
	    gc->error = err501;
	    uc_strcpy(gc->status,"No support for browsing: ");
	    u_strncpy(gc->status+u_strlen(gc->status), gc->path, pt-gc->path );
	    gc->done = true;
	    (gc->receiveerror)(gc);
return;
	}
	gc->protocol_index = i;
	if ( !protocols[i].dothread )
	    (protocols[i].dispatcher)(gc);
	else {
	    gc->return_code = 501;
	    gc->error = err501;
	    uc_strcpy(gc->status,"No support for protocol");
	    gc->done = true;
	    (gc->receiveerror)(gc);
return;
	}
    } else {
	gc->protocol_index = -1;
	_GIO_localDispatch(gc);
    }
}

void _GIO_PostError(GIOControl *gc) {
    gc->receiveerror(gc);
}

void _GIO_PostInter(GIOControl *gc) {
    gc->receiveintermediate(gc);
}

void _GIO_PostSuccess(GIOControl *gc) {
    gc->receivedata(gc);
}

void GIOdir(GIOControl *gc) {
    GIOdispatch(gc,gf_dir);
}

void GIOstatFile(GIOControl *gc) {
    GIOdispatch(gc,gf_statfile);
}

void GIOfileExists(GIOControl *gc) {
    /* We can probably do some optimizations here, based on caching and whatnot */
    GIOdispatch(gc,gf_statfile);
}

void GIOmkDir(GIOControl *gc) {
    GIOdispatch(gc,gf_mkdir);
}

void GIOdelFile(GIOControl *gc) {
    GIOdispatch(gc,gf_delfile);
}

void GIOdelDir(GIOControl *gc) {
    GIOdispatch(gc,gf_deldir);
}

void GIOrenameFile(GIOControl *gc) {
    GIOdispatch(gc,gf_renamefile);
}

void GIOFreeDirEntries(GDirEntry *ent) {
    GDirEntry *next;

    while ( ent!=NULL ) {
	next = ent->next;
	free(ent->name);
	free(ent->mimetype);
	free(ent);
	ent = next;
    }
}

GDirEntry *GIOgetDirData(GIOControl *gc) {

    if ( gc->direntrydata )
return( (GDirEntry *) gc->iodata );

return( NULL );
}

void GIOcancel(GIOControl *gc) {
    if ( gc->protocol_index>=0 && protocols[gc->protocol_index].cancel!=NULL )
	/* Per connection cleanup, cancels io if not done and removes from any queues */
	(protocols[gc->protocol_index].cancel)(gc);
    if ( gc->direntrydata )
	GIOFreeDirEntries((GDirEntry *) gc->iodata);
    else
	free(gc->iodata);
    free(gc->path);
    free(gc->origpath);
    free(gc->topath);
    free(gc);
}

void GIOclose(GIOControl *gc) {
    GIOcancel(gc);
}

GIOControl *GIOCreate(unichar_t *path,void *userdata,
	void (*receivedata)(struct giocontrol *),
	void (*receiveerror)(struct giocontrol *)) {
    GIOControl *gc = (GIOControl *) calloc(1,sizeof(GIOControl));

    gc->path = u_copy(path);
    gc->userdata = userdata;
    gc->receivedata = receivedata;
    gc->receiveerror = receiveerror;
return(gc);
}
