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
#include "encoding.h"
#include "fontforgevw.h"
#include "namelist.h"
#include "pluginloading.h"
#include <gfile.h>
#include <time.h>
#include <sys/time.h>
#include <locale.h>
#include <unistd.h>
#include <dynamic.h>
#ifdef __Mac
# include <stdlib.h>		/* getenv,setenv */
#endif

#include <glib.h>

#include "gutils/unicodelibinfo.h"
#include "psfont.h"

int32 unicode_from_adobestd[256];
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

/* FIXME: Is this necessary or desirable, given we now are using
 * libltdl modules? */
static void initlibrarysearchpath(void) {
#ifdef __Mac
    /* If the user has not set library path, then point it at fink */
    /*  otherwise leave alone. On the mac people often use fink to */
    /*  install image libs. For some reason fink installs in a place */
    /*  the dynamic loader doesn't find */
    /* (And fink's attempts to set the PATH variables generally don't work */
//    setenv("DYLD_LIBRARY_PATH","/sw/lib",0);
#endif
}

static void initlibltdl(void) {
    char buffer[2000];
    char *userConfigDir;

    if (!plugins_are_initialized()) {
        init_plugins();
        userConfigDir = getFontForgeUserDir(Config);
        if ( userConfigDir != NULL ) {
            strcpy(buffer,userConfigDir);
            strcat(buffer,"/plugins");
            free(userConfigDir);
            lt_dladdsearchdir(buffer);
        }
    }
}

void InitSimpleStuff(void) {

    initlibrarysearchpath();
    initlibltdl();
    initrand();
    initadobeenc();

    setlocale(LC_ALL,"");
    localeinfo = *localeconv();
    coord_sep = ",";
    if ( *localeinfo.decimal_point=='.' ) coord_sep=",";
    else if ( *localeinfo.decimal_point!='.' ) coord_sep=" ";
    if ( getenv("FF_SCRIPT_IN_LATIN1") ) use_utf8_in_script=false;

    inituninameannot();	/* Note: unicodenames done after locales set */

    SetDefaults();
}

void doinitFontForgeMain(void) {
    static int inited = false;

    if ( inited )
return;
#ifdef __MINGW32__
    FindProgDir(NULL);
#endif
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
    printf( "libfontforge %d\n",
	    FONTFORGE_VERSIONDATE_RAW );
exit(0);
}
