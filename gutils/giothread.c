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
#include "giofuncP.h"
#include "gfile.h"
#include "ustring.h"
#include "gresource.h"
#include "errno.h"

#include <stdarg.h>
#include <stdio.h>

#ifndef HAVE_PTHREAD_H
void _GIO_ReportHeaders(char *format, ...) {
    va_list args;

    va_start(args,format);
    vfprintf( stderr, format, args);
    va_end(args);
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

static void _GIO_AuthorizationWrapper(void *d) {
    GIOControl *gc = d;

    (_GIO_stdfuncs.getauth)(gc);
}
    
void _GIO_RequestAuthorization(GIOControl *gc) {

    gc->return_code = 401;
    if ( _GIO_stdfuncs.getauth==NULL )
return;
    _GIO_AuthorizationWrapper(gc);
}
#else
void _GIO_ReportHeaders(char *format, ...) {
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    va_list args;

    va_start(args,format);
    pthread_mutex_lock(&mutex);
    vfprintf( stderr, format, args);
    pthread_mutex_unlock(&mutex);
    va_end(args);
}

void _GIO_PostError(GIOControl *gc) {
    if ( _GIO_stdfuncs.gdraw_sync_thread!=NULL )
	(_GIO_stdfuncs.gdraw_sync_thread)(NULL,(void (*)(void *)) gc->receiveerror,gc);
}

void _GIO_PostInter(GIOControl *gc) {
    if ( _GIO_stdfuncs.gdraw_sync_thread!=NULL )
	(_GIO_stdfuncs.gdraw_sync_thread)(NULL,(void (*)(void *)) gc->receiveintermediate,gc);
}

void _GIO_PostSuccess(GIOControl *gc) {
    if ( _GIO_stdfuncs.gdraw_sync_thread!=NULL )
	(_GIO_stdfuncs.gdraw_sync_thread)(NULL,(void (*)(void *)) gc->receivedata,gc);
}

static void _GIO_AuthorizationWrapper(void *d) {
    GIOControl *gc = d;

    (_GIO_stdfuncs.getauth)(gc);
    pthread_mutex_lock(&gc->threaddata->mutex);
    pthread_cond_signal(&gc->threaddata->cond);
    pthread_mutex_unlock(&gc->threaddata->mutex);
}
    
void _GIO_RequestAuthorization(GIOControl *gc) {

    gc->return_code = 401;
    if ( _GIO_stdfuncs.getauth==NULL )
return;
    pthread_mutex_lock(&gc->threaddata->mutex);
    if ( _GIO_stdfuncs.gdraw_sync_thread!=NULL )
	(_GIO_stdfuncs.gdraw_sync_thread)(NULL,_GIO_AuthorizationWrapper,gc);
    pthread_cond_wait(&gc->threaddata->cond,&gc->threaddata->mutex);
    pthread_mutex_unlock(&gc->threaddata->mutex);
}
#endif
