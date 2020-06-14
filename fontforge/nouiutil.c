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

#include "basics.h"
#include "splinefont.h"
#include "uiinterface.h"
#include "ustring.h"

#include <stdarg.h>
#include <stdio.h>

static void NOUI_IError(const char *format,...) {
    va_list ap;
    char buffer[400], *str;
    va_start(ap,format);
    fprintf(stderr, "Internal Error: " );
    vsnprintf(buffer,sizeof(buffer),format,ap);
    str = utf82def_copy(buffer);
    fprintf(stderr,"%s",str);
    if ( str[strlen(str)-1]!='\n' )
	putc('\n',stderr);
    free(str);
    va_end(ap);
}

static void NOUI__LogError(const char *format,va_list ap) {
    char buffer[400], *str;
    vsnprintf(buffer,sizeof(buffer),format,ap);
    str = utf82def_copy(buffer);
    fprintf(stderr,"%s",str);
    if ( str[strlen(str)-1]!='\n' )
	putc('\n',stderr);
    free(str);
}

static void NOUI_LogError(const char *format,...) {
    va_list ap;

    va_start(ap,format);
    NOUI__LogError(format,ap);
    va_end(ap);
}

static void NOUI_post_notice(const char *title,const char *statement,...) {
    va_list ap;
    va_start(ap,statement);
    NOUI__LogError(statement,ap);
    va_end(ap);
}

static void NOUI_post_error(const char *title,const char *statement,...) {
    va_list ap;
    va_start(ap,statement);
    NOUI__LogError(statement,ap);
    va_end(ap);
}

static int NOUI_ask(const char *title, const char **answers,
	int def, int cancel,const char *question,...) {
return( def );
}

static int NOUI_choose(const char *title, const char **choices,int cnt, int def,
	const char *question,...) {
return( def );
}

static int NOUI_choose_multiple(const char *title, const char **choices, char *sel,
	int cnt, char *buts[2], const char *question,...) {
return( -1 );
}

static char *NOUI_ask_string(const char *title, const char *def,
	const char *question,...) {
return( (char *) def );
}

static char *NOUI_open_file(const char *title, const char *defaultfile,
	const char *initial_filter) {
return( NULL );
}

static char *NOUI_saveas_file(const char *title, const char *defaultfile,
	const char *initial_filter) {
return( copy(defaultfile) );
}

static void NOUI_progress_start(int delay, const char *title, const char *line1,
	const char *line2, int tot, int stages) {
}

static void NOUI_void_void_noop(void) {
}

static void NOUI_void_int_noop(int useless) {
}

static int NOUI_int_int_noop(int useless) {
return( true );
}

static void NOUI_void_str_noop(const char * useless) {
}

static int NOUI_alwaystrue(void) {
return( true );
}

static void NOUI_import_params_dlg(struct importparams *ip) {
}

static void NOUI_export_params_dlg(struct exportparams *ep) {
}

static struct ui_interface noui_interface = {
    NOUI_IError,
    NOUI_post_error,
    NOUI_LogError,
    NOUI_post_notice,
    NOUI_ask,
    NOUI_choose,
    NOUI_choose_multiple,
    NOUI_ask_string,
    NOUI_ask_string,			/* password */
    NOUI_open_file,
    NOUI_saveas_file,

    NOUI_progress_start,
    NOUI_void_void_noop,
    NOUI_void_void_noop,
    NOUI_void_int_noop,
    NOUI_alwaystrue,
    NOUI_alwaystrue,
    NOUI_int_int_noop,
    NOUI_void_str_noop,
    NOUI_void_str_noop,
    NOUI_void_void_noop,
    NOUI_void_void_noop,
    NOUI_void_int_noop,
    NOUI_void_int_noop,
    NOUI_alwaystrue,

    NOUI_void_void_noop,

    NOUI_TTFNameIds,
    NOUI_MSLangString,
    NOUI_import_params_dlg,
    NOUI_export_params_dlg
};
struct ui_interface *ui_interface = &noui_interface;

void FF_SetUiInterface(struct ui_interface *uii) {
    ui_interface = uii;
}
