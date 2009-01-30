/* Copyright (C) 2000-2009 by George Williams */
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

unichar_t unknown[] = { '*','/','*', '\0' };
unichar_t textplain[] = { 't','e','x','t','/','p','l','a','i','n', '\0' };
unichar_t texthtml[] = { 't','e','x','t','/','h','t','m','l', '\0' };
unichar_t textxml[] = { 't','e','x','t','/','x','m','l', '\0' };
unichar_t textc[] = { 't','e','x','t','/','c', '\0' };
unichar_t textcss[] = { 't','e','x','t','/','c','s','s', '\0' };
unichar_t textmake[] = { 't','e','x','t','/','m','a','k','e', '\0' };
unichar_t textjava[] = { 't','e','x','t','/','j','a','v','a', '\0' };
unichar_t textps[] = { 't','e','x','t','/','p','s', '\0' };
	/* Officially registered with IANA on 14 May 2008 */
unichar_t sfdfont[] = { 'a','p','p','l','i','c','a','t','i','o','n','/','v','n','d','.','f','o','n','t','-','f','o','n','t','f','o','r','g','e','-','s','f','d', '\0' };
unichar_t textpsfont[] = { 't','e','x','t','/','f','o','n','t','p','s', '\0' };
unichar_t textbdffont[] = { 't','e','x','t','/','f','o','n','t','b','d','f', '\0' };
unichar_t imagegif[] = { 'i','m','a','g','e','/','g','i','f', '\0' };
unichar_t imagejpeg[] = { 'i','m','a','g','e','/','j','p','e','g', '\0' };
unichar_t imagepng[] = { 'i','m','a','g','e','/','p','n','g', '\0' };
unichar_t imagesvg[] = { 'i','m','a','g','e','/','s','v','g','+','x','m','l', '\0' };
unichar_t videoquick[] = { 'v','i','d','e','o','/','q','u','i','c','k','t','i','m','e', '\0' };
unichar_t audiowav[] = { 'a','u','d','i','o','/','w','a','v', '\0' };
unichar_t pdf[] = { 'a','p','p','l','i','c','a','t','i','o','n','/','p','d','f', '\0' };
unichar_t object[] = { 'a','p','p','l','i','c','a','t','i','o','n','/','x','-','o','b','j','e','c','t', '\0' };
unichar_t dir[] = { 'a','p','p','l','i','c','a','t','i','o','n','/','x','-','n','a','v','i','d','i','r', '\0' };
unichar_t core[] = { 'a','p','p','l','i','c','a','t','i','o','n','/','x','-','c','o','r','e', '\0' };
unichar_t fontttf[] = { 'a','p','p','l','i','c','a','t','i','o','n','/','x','-','f','o','n','t','-','t','t','f', '\0' };
unichar_t fontotf[] = { 'a','p','p','l','i','c','a','t','i','o','n','/','x','-','f','o','n','t','-','o','t','f', '\0' };
unichar_t fontcid[] = { 'a','p','p','l','i','c','a','t','i','o','n','/','x','-','f','o','n','t','-','c','i','d', '\0' };
unichar_t fonttype1[] = { 'a','p','p','l','i','c','a','t','i','o','n','/','x','-','f','o','n','t','-','t','y','p','e','1', '\0' };
unichar_t fontmacsuit[] = { 'a','p','p','l','i','c','a','t','i','o','n','/','x','-','m','a','c','-','s','u','i','t', '\0' };
unichar_t macbin[] = { 'a','p','p','l','i','c','a','t','i','o','n','/','x','-','m','a','c','b','i','n','a','r','y', '\0' };
unichar_t machqx[] = { 'a','p','p','l','i','c','a','t','i','o','n','/','x','-','m','a','c','-','b','i','n','h','e','x','4','0', '\0' };
unichar_t macdfont[] = { 'a','p','p','l','i','c','a','t','i','o','n','/','x','-','m','a','c','-','d','f','o','n','t', '\0' };
unichar_t compressed[] = { 'a','p','p','l','i','c','a','t','i','o','n','/','x','-','c','o','m','p','r','e','s','s','e','d', '\0' };
unichar_t tar[] = { 'a','p','p','l','i','c','a','t','i','o','n','/','x','-','t','a','r', '\0' };
unichar_t fontpcf[] = { 'a','p','p','l','i','c','a','t','i','o','n','/','x','-','f','o','n','t','-','p','c','f', '\0' };
unichar_t fontsnf[] = { 'a','p','p','l','i','c','a','t','i','o','n','/','x','-','f','o','n','t','-','s','n','f', '\0' };

#ifdef __Mac
#include </Developer/Headers/FlatCarbon/Files.h>
#define CHR(ch1,ch2,ch3,ch4) (((ch1)<<24)|((ch2)<<16)|((ch3)<<8)|(ch4))

unichar_t *_GioMacMime(const char *path) {
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

unichar_t *GIOguessMimeType(const unichar_t *path,int isdir) {
    unichar_t *pt;

    if ( isdir )
return( dir );
    path = u_GFileNameTail(path);
    pt = u_strrchr(path,'.');

    if ( pt==NULL ) {
	if ( uc_strmatch(path,"makefile")==0 || uc_strmatch(path,"makefile~")==0 )
return( textmake );
	else if ( uc_strmatch(path,"core")==0 )
return( core );
    } else if ( uc_strmatch(pt,".text")==0 || uc_strmatch(pt,".txt")==0 ||
	    uc_strmatch(pt,".text~")==0 || uc_strmatch(pt,".txt~")==0 )
return( textplain );
    else if ( uc_strmatch(pt,".c")==0 || uc_strmatch(pt,".h")==0 ||
	    uc_strmatch(pt,".c~")==0 || uc_strmatch(pt,".h~")==0 )
return( textc );
    else if ( uc_strmatch(pt,".java")==0 || uc_strmatch(pt,".java~")==0 )
return( textjava );
    else if ( uc_strmatch(pt,".css")==0 || uc_strmatch(pt,".css~")==0 )
return( textcss );
    else if ( uc_strmatch(pt,".html")==0 || uc_strmatch(pt,".htm")==0 ||
	    uc_strmatch(pt,".html~")==0 || uc_strmatch(pt,".htm~")==0 )
return( texthtml );
    else if ( uc_strmatch(pt,".xml")==0 || uc_strmatch(pt,".xml~")==0 )
return( textxml );
    else if ( uc_strmatch(pt,".pfa")==0 || uc_strmatch(pt,".pfb")==0 ||
	    uc_strmatch(pt,".pt3")==0 || uc_strmatch(pt,".cff")==0 )
return( textpsfont );
    else if ( uc_strmatch(pt,".sfd")==0 )
return( sfdfont );
    else if ( uc_strmatch(pt,".ttf")==0 )
return( fontttf );
    else if ( uc_strmatch(pt,".otf")==0 || uc_strmatch(pt,".otb")==0 ||
	    uc_strmatch(pt,".gai")==0 )
return( fontotf );
    else if ( uc_strmatch(pt,".cid")==0 )
return( fontcid );
    else if ( uc_strmatch(pt,".ps")==0 || uc_strmatch(pt,".eps")==0 )
return( textps );
    else if ( uc_strmatch(pt,".bdf")==0 )
return( textbdffont );
    else if ( uc_strmatch(pt,".pdf")==0 )
return( pdf );
    else if ( uc_strmatch(pt,".gif")==0 )
return( imagegif );
    else if ( uc_strmatch(pt,".png")==0 )
return( imagepng );
    else if ( uc_strmatch(pt,".svg")==0 )
return( imagesvg );
    else if ( uc_strmatch(pt,".jpeg")==0 || uc_strmatch(pt,".jpg")==0 )
return( imagejpeg );
    else if ( uc_strmatch(pt,".mov")==0 || uc_strmatch(pt,".movie")==0 )
return( videoquick );
    else if ( uc_strmatch(pt,".wav")==0 )
return( audiowav );
    else if ( uc_strmatch(pt,".o")==0 || uc_strmatch(pt,".obj")==0 )
return( object );
    else if ( uc_strmatch(pt,".bin")==0 )
return( macbin );
    else if ( uc_strmatch(pt,".hqx")==0 )
return( machqx );
    else if ( uc_strmatch(pt,".dfont")==0 )
return( macdfont );
    else if ( uc_strmatch(pt,".gz")==0 || uc_strmatch(pt,".tgz")==0 ||
	    uc_strmatch(pt,".Z")==0 || uc_strmatch(pt,".zip")==0 ||
	    uc_strmatch(pt,".bz2")==0 || uc_strmatch(pt,".tbz")==0 ||
	    uc_strmatch(pt,".rpm")==0 )
return( compressed );
    else if ( uc_strmatch(pt,".tar")==0 )
return( tar );
    else if ( uc_strmatch(pt,".pcf")==0 )
return( fontpcf );
    else if ( uc_strmatch(pt,".snf")==0 )
return( fontsnf );

return( unknown );
}
