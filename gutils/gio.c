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

#ifndef NODYNAMIC
# include <dynamic.h>
#endif

struct stdfuncs _GIO_stdfuncs = {
    GIOguessMimeType, _GIO_decomposeURL, _GIO_PostSuccess, _GIO_PostInter,
    _GIO_PostError, _GIO_RequestAuthorization, _GIO_LookupHost,
    NULL,			/* default authorizer */
    GIOFreeDirEntries,
#ifdef GWW_TEST
    _GIO_ReportHeaders,		/* set to NULL when not debugging */
#else
    NULL,
#endif

#ifdef HAVE_PTHREAD_H
    PTHREAD_MUTEX_INITIALIZER,
#endif
    NULL,
    NULL
};
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
typedef void *(ptread_startfunc_t)(void *);

static unichar_t err501[] = { ' ','N','o','t',' ','I','m','p','l','e','m','e','n','t','e','d', '\0' };

static int AddProtocol(unichar_t *prefix,int len) {

    if ( plen>=pmax ) {
	pmax += 20;		/* We're never going to support 20 protocols? */
	if ( plen==0 ) {
	    protocols = galloc(pmax*sizeof(struct protocols));
	} else {
	    protocols = grealloc(protocols,pmax*sizeof(struct protocols));
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
#ifdef NODYNAMIC
return( false );
#else
	char lib[300], buffer[1400];
	DL_CONST void *handle;
	void (*init)(DL_CONST void *,struct stdfuncs *,int);
	strcpy(lib,"libgio");
	cu_strncat(lib,prefix,len);
	strcat(lib,SO_EXT);
	if ( (handle = dlopen(lib,RTLD_LAZY))==NULL ) {
#ifdef LIBDIR
	    sprintf(buffer,LIBDIR "/%s",lib);
	    if ( (handle = dlopen(buffer,RTLD_LAZY))==NULL )
#endif
	    {
#ifdef GWW_TEST
 printf ( "dlopen failed: %s\n", dlerror());
#endif
return( false );
	    }
	}
	protocols[plen].handle = handle;
	protocols[plen].dispatcher = dlsym(handle,"GIO_dispatch");
	protocols[plen].cancel = dlsym(handle,"GIO_cancel");
	protocols[plen].term = dlsym(handle,"GIO_term");
	init = dlsym(handle,"GIO_init");
	if ( init!=NULL )
	    (init)(handle,&_GIO_stdfuncs,plen);
	protocols[plen].dothread = true;
#endif
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

    if ( _GIO_stdfuncs.useragent == NULL )
	_GIO_stdfuncs.useragent = copy("someone@somewhere.com");

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
#ifndef HAVE_PTHREAD_H
	    gc->return_code = 501;
	    gc->error = err501;
	    uc_strcpy(gc->status,"No support for protocol");
	    gc->done = true;
	    (gc->receiveerror)(gc);
return;
#else
	    static pthread_cond_t initcond = PTHREAD_COND_INITIALIZER;
	    static pthread_mutex_t initmutex = PTHREAD_MUTEX_INITIALIZER;
	    /* could put stuff here to queue functions if we get too many */
	    /*  threads, or perhaps even a thread pool */
	    uc_strcpy(gc->status,"Queued");
	    gc->threaddata = galloc(sizeof(struct gio_threaddata));
	    gc->threaddata->mutex = initmutex;
	    gc->threaddata->cond = initcond;
	    if ( _GIO_stdfuncs.gdraw_sync_thread!=NULL )
		(_GIO_stdfuncs.gdraw_sync_thread)(NULL,NULL,NULL);
	    pthread_create(&gc->threaddata->thread,NULL,
		    (ptread_startfunc_t *) (protocols[i].dispatcher), gc);
#endif
	}
    } else {
	gc->protocol_index = -1;
	_GIO_localDispatch(gc);
    }
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
return( gc->iodata );

return( NULL );
}

void GIOcancel(GIOControl *gc) {
#ifdef HAVE_PTHREAD_H
    if ( gc->protocol_index>=0 && protocols[gc->protocol_index].dothread &&
	    gc->threaddata!=NULL && !gc->done ) {
	void *ret;
	gc->abort = true;
	pthread_cancel(gc->threaddata->thread);
	pthread_join(gc->threaddata->thread, &ret);
    }
#endif
    if ( gc->protocol_index>=0 && protocols[gc->protocol_index].cancel!=NULL )
	/* Per connection cleanup, cancels io if not done and removes from any queues */
	(protocols[gc->protocol_index].cancel)(gc);
    if ( gc->direntrydata )
	GIOFreeDirEntries(gc->iodata);
    else
	free(gc->iodata);
    free(gc->threaddata);
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
    GIOControl *gc = gcalloc(1,sizeof(GIOControl));

    gc->path = u_copy(path);
    gc->userdata = userdata;
    gc->receivedata = receivedata;
    gc->receiveerror = receiveerror;
return(gc);
}

void GIOSetDefAuthorizer(int32 (*getauth)(struct giocontrol *)) {
    _GIO_stdfuncs.getauth = getauth;
}

void GIOSetUserAgent(unichar_t *agent) {
    free( _GIO_stdfuncs.useragent );
    _GIO_stdfuncs.useragent = cu_copy(agent);
}

void GIO_SetThreadCallback(void (*callback)(void *,void *,void *)) {
    _GIO_stdfuncs.gdraw_sync_thread = callback;
}
