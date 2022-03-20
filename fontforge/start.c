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
#include <fontforge-version-extras.h>

#include "start.h"

#include "encoding.h"
#include "ffglib.h"
#include "fontforgevw.h"
#include "gfile.h"
#include "namelist.h"
#include "psfont.h"

#include <locale.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#ifdef __Mac
# include <stdlib.h>		/* getenv,setenv */
#endif

int32_t unicode_from_adobestd[256];
struct lconv localeinfo;
const char *coord_sep = ",";
int quiet = 0;

static void initadobeenc(void) {
    int i,j;

    for ( i=0; i<0x100; ++i ) {
	if ( strcmp(AdobeStandardEncoding[i],".notdef")==0 )
	    unicode_from_adobestd[i] = 0xfffd;
	else {
	    j = UniFromName(AdobeStandardEncoding[i],ui_none,&custom);
	    if ( j==-1 ) j = 0xfffd;
	    unicode_from_adobestd[i] = j;
	}
    }
}

static void initrand(void) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    srand(tv.tv_usec);
    g_random_set_seed(tv.tv_usec);
}

void InitSimpleStuff(void) {

    initrand();
    initadobeenc();

    setlocale(LC_ALL,"");
    localeinfo = *localeconv();
    coord_sep = ",";
    if ( *localeinfo.decimal_point=='.' ) coord_sep=",";
    else if ( *localeinfo.decimal_point!='.' ) coord_sep=" ";
    if ( getenv("FF_SCRIPT_IN_LATIN1") ) use_utf8_in_script=false;

    SetDefaults();
}

void doinitFontForgeMain(void) {
    static int inited = false;

    if ( inited )
return;
    FindProgRoot(NULL);
    InitSimpleStuff();
    if ( default_encoding==NULL )
	default_encoding=FindOrMakeEncoding("ISO8859-1");
    if ( default_encoding==NULL )
	default_encoding=&custom;	/* In case iconv is broken */
    inited = true;
}

void doversion(const char *source_version_str) {
    if ( source_version_str!=NULL )
	printf( "fontforge %s\n", source_version_str );
    printf( "build date: %s\n",
	    FONTFORGE_MODTIME_STR );
exit(0);
}
