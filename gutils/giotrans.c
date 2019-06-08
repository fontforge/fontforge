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

#include "giofuncP.h"
#include "ustring.h"

struct transtab {
    unichar_t *old;
    unichar_t *new;
/*  struct transtab *next;	/* pointer to next transtab */
    int olen;			/* length to test against */
    int gf_mask;		/* enum giofuncs */
};
static struct transtab *transtab=NULL;

unichar_t *_GIO_translateURL(unichar_t *path, enum giofuncs gf) {
    struct transtab *test;
    unichar_t *res;

    if ( transtab==NULL )
	/* Need some sort of _GIO_addURL(), otherwise you never get past here	*/
	return( NULL );

    for ( test = transtab; test->old!=NULL; ++test ) {
	if ( (test->gf_mask&(1<<gf)) && u_strncmp(path,test->old,test->olen)==0 ) {
	    if ( (res=malloc((u_strlen(path)-test->olen+u_strlen(test->new)+1)*sizeof(unichar_t)))==NULL )
		return( NULL );
	    u_strcpy(res,test->new);
	    u_strcat(res,path+test->olen);
	    return( res );
	}
    }
    return( NULL );
}
