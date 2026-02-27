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

#include <fontforge-config.h>
#include <fontforge-version-extras.h>

#include "encoding.h"
#include "fontforgevw.h"
#include "gfile.h"
#include "scripting.h"
#include "start.h"
#include "ustring.h"

#include <locale.h>
#include <time.h>

#ifdef __Mac
# include <stdlib.h>		/* getenv,setenv */
#endif

static void _doscriptusage(void) {
    printf( "fontforge [options]\n" );
    printf( "\t-usage\t\t\t (displays this message, and exits)\n" );
    printf( "\t-help\t\t\t (displays this message, invokes a browser\n\t\t\t\t  using the BROWSER environment variable)\n" );
    printf( "\t-version\t\t (prints the version of fontforge and exits)\n" );
    printf( "\t-lang=py\t\t (use python to execute scripts\n" );
    printf( "\t-lang=ff\t\t (use fontforge's native language to\n\t\t\t\t  execute scripts)\n" );
    printf( "\t-script scriptfile\t (executes scriptfile)\n" );
    printf( "\t-c script-string\t (executes the argument as scripting cmds)\n" );
    printf( "\t-skippyfile\t\t (do not execute python init scripts)\n" );
    printf( "\t-skippyplug\t\t (do not load python plugins)\n" );
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
exit(0);
}

int fontforge_main( int argc, char **argv ) {
    int run_python_init_files = true;
    int import_python_plugins = true;
    bool quiet = false;
    char *pt;

    FindProgRoot(argv[0]);
    InitSimpleStuff();

    bind_textdomain_codeset("FontForge","UTF-8");
    bindtextdomain("FontForge", getLocaleDir());
    textdomain("FontForge");
    for ( int i=1; i<argc; ++i ) {
	pt = argv[i];

	if ( strcmp(pt,"-SkipPythonInitFiles")==0 || strcmp(pt,"-skippyfile")==0 ) {
	    run_python_init_files = false;
	} else if ( strcmp(pt,"-skippyplug")==0 ) {
	    import_python_plugins = false;
	} else if ( strcmp(pt,"-quiet")==0 ) {
	    quiet = true;
	}
    }

    if (!quiet) {
        time_t tm = FONTFORGE_MODTIME_RAW;
        struct tm* modtime = gmtime(&tm);
        fprintf( stderr, "Copyright (c) 2000-%d. See AUTHORS for Contributors.\n", modtime->tm_year+1900 );
        fprintf( stderr, " License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n" );
        fprintf( stderr, " with many parts BSD <http://fontforge.org/license.html>. Please read LICENSE.\n" );
        fprintf( stderr, " Version: %s\n", FONTFORGE_VERSION );
        fprintf( stderr, " Based on sources from %s"
#ifdef FREETYPE_HAS_DEBUGGER
            "-TtfDb"
#endif
#ifdef _NO_PYTHON
            "-NoPython"
#endif
#ifdef FONTFORGE_CONFIG_USE_DOUBLE
            "-D"
#endif
            ".\n",
            FONTFORGE_MODTIME_STR );
        // Can be empty if e.g. building from a tarball
        if (FONTFORGE_GIT_VERSION[0] != '\0') {
            fprintf( stderr, " Based on source from git with hash: %s\n", FONTFORGE_GIT_VERSION );
        }
    }

    if ( GetDefaultEncoding()==NULL )
	SetDefaultEncoding(FindOrMakeEncoding("ISO8859-1"));
    if ( GetDefaultEncoding()==NULL )
	SetDefaultEncoding(GetCustomEncoding());	/* In case iconv is broken */
    CheckIsScript(argc,argv);		/* Will run the script and exit if it is a script */
    if ( argc==2 ) {
	pt = argv[1];
	if ( *pt=='-' && pt[1]=='-' && pt[2]!='\0') ++pt;
	if ( strcmp(pt,"-usage")==0 )
	    doscriptusage();
	else if ( strcmp(pt,"-help")==0 )
	    doscripthelp();
	else if ( strcmp(pt,"-version")==0 || strcmp(pt,"-v")==0 || strcmp(pt,"-V")==0 )
	    doversion(FONTFORGE_VERSION);
    }
#  if defined(_NO_PYTHON)
    ProcessNativeScript(argc, argv,stdin);
#  else
    PyFF_Stdin(run_python_init_files, import_python_plugins);
#  endif

return( 0 );
}
