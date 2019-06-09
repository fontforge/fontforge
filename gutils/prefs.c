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

#include "prefs.h"

// FIXME: Remove this file...

int cv_auto_goto = 0;

/* GKeyFile* keyfile = 0; */

/* static int get_bool(const gchar *group_name, const gchar *key, int def ) */
/* { */
/*     GError *err = NULL; */
/*     int ret = g_key_file_get_boolean( keyfile, group_name, key, &err ); */
/*     if (err != NULL) */
/*     { */
/* 	g_error_free (err); */
/* 	ret = def; */
/*     } */
/*     return ret; */
/* } */


/* void loadPrefsFiles(void) */
/* { */
/*     if( keyfile ) */
/* 	return; */
    
/*     keyfile = g_key_file_new(); */
/*     char localefn[PATH_MAX+1]; */
/*     char* sharedir = getShareDir(); */
/*     int rc = 0; */
    
/*     snprintf(localefn,PATH_MAX,"%s/preferences", sharedir ); */
/*     rc = g_key_file_load_from_file( keyfile, localefn, G_KEY_FILE_NONE, 0 ); */
    
/*     snprintf(localefn,PATH_MAX,"%s/preferences", getDotFontForgeDir() ); */
/*     rc = g_key_file_load_from_file( keyfile, localefn, G_KEY_FILE_NONE, 0 ); */
    
/*     cv_auto_goto = get_bool( "CharView", "AutoGoto", 0 ); */
/* } */


