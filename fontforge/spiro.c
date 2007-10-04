/* Copyright (C) 2007 by George Williams */
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

/* Access to Raph Levien's spiro splines */

#ifdef NODYNAMIC
int hasspiro(void) {
return(false);
}

SplineSet *SpiroCP2SplineSet(spiro_cp *spiros) {
return( NULL );
}
#else
#  include <dynamic.h>

static DL_CONST void *libffspiro;
static SplineSet *(*_Spiro2SplineSet)( spiro_cp *spiros, int n);
static int inited = false, has_spiro = false;

static void initSpiro(void) {
    if ( inited )
return;

    libffspiro = dlopen("libffspiro" SO_EXT,RTLD_LAZY);
    if ( libffspiro==NULL )
#ifdef LIBDIR
	libffspiro = dlopen(LIBDIR "/libffspiro" SO_EXT,RTLD_LAZY);
#else
	libffspiro = dlopen("/usr/local/bin" "/libffspiro" SO_EXT,RTLD_LAZY);
#endif
	
    inited = true;

    if ( libffspiro==NULL ) {
#ifndef __Mac
	fprintf( stderr, "%s\n", dlerror());
#endif
return;
    }

    _Spiro2SplineSet = (SplineSet *(*)( spiro_cp *spiros, int n)) dlsym(libffspiro,"Spiro2SplineSet");
    if ( _Spiro2SplineSet==NULL ) {
	LogError("Hmm. This system has a libffspiro, but it doesn't contain the entry point\nwe care about. Must be something else.\n");
    } else
	has_spiro = true;
}

int hasspiro(void) {
    initSpiro();
return(has_spiro);
}

SplineSet *SpiroCP2SplineSet(spiro_cp *spiros) {
    int n;

    if ( spiros==NULL )
return( NULL );
    for ( n=0; spiros[n].ty!='z'; ++n );
    if ( n==0 )
return( NULL );
return( _Spiro2SplineSet(spiros,n));
}
#endif
