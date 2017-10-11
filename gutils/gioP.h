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
#ifndef _GIOP_H
#define _GIOP_H

#include "gio.h"
#include <sys/types.h>

#if defined(__MINGW32__)
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

struct stdfuncs {
    char *(*decomposeURL)(const unichar_t *url,char **host, int *port,
        char **username, char **password);
    void (*PostSuccess)(GIOControl *gc);
    void (*PostInter)(GIOControl *gc);
    void (*PostError)(GIOControl *gc);
    void (*RequestAuthorization)(GIOControl *gc);
    struct hostdata *(*LookupHost)(char *name);
    int32 (*getauth)(struct giocontrol *);
    void (*FreeDirEntries)(GDirEntry *lst);
    void (*reportheaders)(char *, ...);
#ifdef HAVE_PTHREAD_H
    pthread_mutex_t hostacccess_mutex;
#endif
    char *useragent;
    void (*gdraw_sync_thread)(void *,void *,void *);
};

struct gio_threaddata {
#ifdef HAVE_PTHREAD_H
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
#else
    int foo;
#endif
};

/* One of these for each protocol (possibly) containing stuff like authorization */
/*  or ftp socket */
struct hostaccessdata {
    struct hostaccessdata *next;
    int port;
    int protocol_index;
    void *moredata;
};

struct hostdata {
    char *hostname;
    struct sockaddr_in addr;
    struct hostaccessdata *had;
    struct hostdata *next;
};

#endif
