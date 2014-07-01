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
#include "gio.h"
#include "gfile.h"
#include "ustring.h"


#ifdef __Mac
#include <carbon.h>
#define CHR(ch1,ch2,ch3,ch4) (((ch1)<<24)|((ch2)<<16)|((ch3)<<8)|(ch4))

static unichar_t *_GioMacMime(const char *path) {
    /* If we're on a mac, we can try to see if we've got a real resource fork */
    FSRef ref;
    FSCatalogInfo info;

    if ( FSPathMakeRef( (uint8 *) path,&ref,NULL)!=noErr )
return( NULL );
    if ( FSGetCatalogInfo(&ref,kFSCatInfoFinderInfo,&info,NULL,NULL,NULL)!=noErr )
return( NULL );
    if ( ((FInfo *) (info.finderInfo))->fdType==CHR('F','F','I','L') )
return( fontmacsuit );
    if ( ((FInfo *) (info.finderInfo))->fdType==CHR('G','I','F','f') )
return( imagegif );
    if ( ((FInfo *) (info.finderInfo))->fdType==CHR('P','N','G',' ') )
return( imagepng );
/*
    if ( ((FInfo *) (info.finderInfo))->fdType==CHR('B','M','P',' ') )
return( imagebmp );
*/
    if ( ((FInfo *) (info.finderInfo))->fdType==CHR('J','P','E','G') )
return( imagejpeg );
/*
    if ( ((FInfo *) (info.finderInfo))->fdType==CHR('T','I','F','F') )
return( imagetiff );
*/
    if ( ((FInfo *) (info.finderInfo))->fdType==CHR('T','E','X','T') )
return( textplain );

return( NULL );
}
#endif

#include "g_giomime.c"