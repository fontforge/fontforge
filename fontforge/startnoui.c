/* Copyright (C) 2007-2012 by George Williams */
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
#include "fontforgevw.h"
#include <gfile.h>
#include <ustring.h>
#include <time.h>
#include <sys/time.h>
#include <locale.h>
#include <unistd.h>
#include <dynamic.h>
#ifdef __Mac
# include <stdlib.h>		/* getenv,setenv */
#endif

static char *GResourceProgramDir;

static void FindProgDir(char *prog) {
#if defined(__MINGW32__)
    char  path[MAX_PATH+4];
    char* c = path;
    char* tail = 0;
    unsigned int  len = GetModuleFileNameA(NULL, path, MAX_PATH);
    path[len] = '\0';
    for(; *c; *c++){
	if(*c == '\\'){
	    tail=c;
	    *c = '/';
	}
    }
    if(tail) *tail='\0';
    GResourceProgramDir = copy(path);
#else
    GResourceProgramDir = _GFile_find_program_dir(prog);
    if ( GResourceProgramDir==NULL ) {
	char filename[1025];
	GFileGetAbsoluteName(".",filename,sizeof(filename));
	GResourceProgramDir = copy(filename);
    }
#endif
}

static char *getLocaleDir(void) {
    static char *sharedir=NULL;
    static int set=false;
    char *pt;
    int len;

    if ( set )
return( sharedir );

    set = true;

#if defined(__MINGW32__)

    len = strlen(GResourceProgramDir) + strlen("/share/locale") +1;
    sharedir = galloc(len);
    strcpy(sharedir, GResourceProgramDir);
    strcat(sharedir, "/share/locale");
    return sharedir;

#else

    pt = strstr(GResourceProgramDir,"/bin");
    if ( pt==NULL ) {
#ifdef SHAREDIR
return( sharedir = SHAREDIR "/../locale" );
#elif defined( PREFIX )
return( sharedir = PREFIX "/share/locale" );
#else
	pt = GResourceProgramDir + strlen(GResourceProgramDir);
#endif
    }
    len = (pt-GResourceProgramDir)+strlen("/share/locale")+1;
    sharedir = galloc(len);
    strncpy(sharedir,GResourceProgramDir,pt-GResourceProgramDir);
    strcpy(sharedir+(pt-GResourceProgramDir),"/share/locale");
return( sharedir );

#endif
}
static void _doscriptusage(void) {
    printf( "fontforge [options]\n" );
    printf( "\t-usage\t\t\t (displays this message, and exits)\n" );
    printf( "\t-help\t\t\t (displays this message, invokes a browser)\n\t\t\t\t  (Using the BROWSER environment variable)\n" );
    printf( "\t-version\t\t (prints the version of fontforge and exits)\n" );
    printf( "\t-lang=py\t\t use python to execute scripts\n" );
    printf( "\t-lang=ff\t\t use fontforge's old language to execute scripts\n" );
    printf( "\t-script scriptfile\t (executes scriptfile)\n" );
    printf( "\t-c script-string\t (executes the argument as scripting cmds)\n" );
    printf( "\n" );
    printf( "If no scriptfile/string is given (or if it's \"-\") FontForge will read stdin\n" );
    printf( "FontForge will read postscript (pfa, pfb, ps, cid), opentype (otf),\n" );
    printf( "\ttruetype (ttf,ttc), macintosh resource fonts (dfont,bin,hqx),\n" );
    printf( "\tand bdf and pcf fonts. It will also read its own format --\n" );
    printf( "\tsfd files.\n" );
    printf( "Any arguments after the script file will be passed to it.\n");
    printf( "If the first argument is an executable filename, and that file's first\n" );
    printf( "\tline contains \"fontforge\" then it will be treated as a scriptfile.\n\n" );
    printf( "For more information see:\n\thttp://fontforge.sourceforge.net/\n" );
    printf( "Send bug reports to:\tfontforge-devel@lists.sourceforge.net\n" );
}

static void doscriptusage(void) {
    _doscriptusage();
exit(0);
}

static void doscripthelp(void) {
    _doscriptusage();
    /*help("overview.html");*/
exit(0);
}

int main( int argc, char **argv ) {
    extern const char *source_version_str;
    extern const char *source_modtime_str;

    fprintf( stderr, "Copyright (c) 2000-2012 by George Williams.\n Executable based on sources from %s"
#ifdef FONTFORGE_CONFIG_TYPE3
	    "-ML"
#endif
#ifdef FREETYPE_HAS_DEBUGGER
	    "-TtfDb"
#endif
#ifdef _NO_PYTHON
	    "-NoPython"
#endif
#ifdef FONTFORGE_CONFIG_USE_LONGDOUBLE
	    "-LD"
#elif defined(FONTFORGE_CONFIG_USE_DOUBLE)
	    "-D"
#endif
#ifndef FONTFORGE_CONFIG_DEVICETABLES
	    "-NoDevTab"
#endif
	    ".\n",
	    source_modtime_str );
    fprintf( stderr, " Library based on sources from %s.\n", library_version_configuration.library_source_modtime_string );

    /* I don't bother to check that the exe's exectations of the library are */
    /*  valid. The exe only consists of this file, and so it doesn't care. */
    /*  as long as the library is self consistant, all should be well */
    /* check_library_version(&exe_library_version_configuration,true,false); */

    FindProgDir(argv[0]);
    InitSimpleStuff();

    bind_textdomain_codeset("FontForge","UTF-8");
    bindtextdomain("FontForge", getLocaleDir());
    textdomain("FontForge");

    if ( default_encoding==NULL )
	default_encoding=FindOrMakeEncoding("ISO8859-1");
    if ( default_encoding==NULL )
	default_encoding=&custom;	/* In case iconv is broken */
    CheckIsScript(argc,argv);		/* Will run the script and exit if it is a script */
    if ( argc==2 ) {
	char *pt = argv[1];
	if ( *pt=='-' && pt[1]=='-' ) ++pt;
	if ( strcmp(pt,"-usage")==0 )
	    doscriptusage();
	else if ( strcmp(pt,"-help")==0 )
	    doscripthelp();
	else if ( strcmp(pt,"-version")==0 )
	    doversion(source_version_str);
    }
#  if defined(_NO_PYTHON)
    ProcessNativeScript(argc, argv,stdin);
#  else
    PyFF_Stdin();
#  endif
return( 0 );
}
