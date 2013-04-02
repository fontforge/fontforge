/* -*- coding: utf-8 -*- */
/* Copyright (C) 2013 by Jose Da Silva */
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

#include "fontforge.h"
#include <ustring.h>


#ifndef _NO_LIBUNINAMESLIST
#include <uninameslist.h>
#endif
const struct unicode_nameannot * const *const *_UnicodeNameAnnot = NULL; /* deprecated */
#ifndef _NO_LIBUNICODENAMES
#include <libunicodenames.h>
uninm_names_db names_db; /* Unicode character names and annotations database */
#endif

void inituninameannot(void) {
/* Initialize unicode name-annotation library access for FontForge */
/* TODO: Library pluginloading currently disabled - see 20120731-b */
/* for existing code if you want to re-enable this feature again.  */

#if _NO_LIBUNINAMESLIST
    _UnicodeNameAnnot = NULL; /* libuninameslist not available */
#else
    /* backward compatibility for other programs using libuninames */
    _UnicodeNameAnnot = UnicodeNameAnnot;
#endif

#ifndef _NO_LIBUNICODENAMES
    /* Open database file, read data for this 'local', then close. */
    char *names_db_file;

    /* Load character names and annotations that come from the Unicode NamesList.txt */
    /* This should not be done until after the locale has been set */
    names_db_file = uninm_find_names_db(NULL);
    names_db = (names_db_file == NULL) ? ((uninm_names_db) 0) : uninm_names_db_open(names_db_file);
    free(names_db_file);
    /* NOTE: you need to do uninm_names_db_close(names_db); when you exit program */
#endif
}

char *unicode_name(int32 unienc) {
/* Return the unicode name for the value given from a data library.	  */
/* If there's no data available for this code, or no library, return NULL */
/* User should free the return string when finished with this information */
    char *name_data;

    name_data=NULL;
#if _NO_LIBUNINAMESLIST && _NO_LIBUNICODENAMES
    /* no nameslist library code available to use */
    //fprintf(stderr,"no library\n");
    return( NULL );
#else
    /* have nameslist library code available to use */
    if ( unienc<0 || unienc>=0x110000 )
	return( NULL );

#ifndef _NO_LIBUNINAMESLIST
#ifndef _LIBUNINAMESLIST_FUN
    /* old libuninameslist library code */
    if ( _UnicodeNameAnnot!=NULL &&
	    _UnicodeNameAnnot[unienc>>16][(unienc>>8)&0xff][unienc&0xff].name!=NULL ) {
	name_data=copy(_UnicodeNameAnnot[unienc>>16][(unienc>>8)&0xff][unienc&0xff].name);
    }
    //fprintf(stderr,"use old code library ->%s<-\n",name_data\n");
#else
    /* new libuninameslist library code */
    name_data=copy(uniNamesList_name(unienc));
    //fprintf(stderr,"new library ->%s<-\n",name_data\n");
#endif
#else
    /* libunicodesnames library code */
    name_data=copy(uninm_name(names_db,(unsigned int)(unienc)));
    //fprintf(stderr,"libunicodes library ->%s<-\n",name_data\n");
#endif
#endif

    return( name_data );
}

char *unicode_annot(int32 unienc) {
/* Return the unicode annotation for the value given from a data library. */
/* If there's no data available for this code, or no library, return NULL */
/* User should free the return string when finished with this information */
    char *annot_data;

    annot_data=NULL;
#if _NO_LIBUNINAMESLIST && _NO_LIBUNICODENAMES
    /* no nameslist library code available to use */
    //fprintf(stderr,"no library - annotation\n");
    return( NULL );
#else
    /* have nameslist library code available to use */
    if ( unienc<0 || unienc>=0x110000 )
	return( NULL );

#ifndef _NO_LIBUNINAMESLIST
#ifndef _LIBUNINAMESLIST_FUN
    /* old libuninameslist library code */
    if ( _UnicodeNameAnnot!=NULL &&
	    _UnicodeNameAnnot[unienc>>16][(unienc>>8)&0xff][unienc&0xff].annot!=NULL ) {
	annot_data=copy(_UnicodeNameAnnot[unienc>>16][(unienc>>8)&0xff][unienc&0xff].annot);
    }
    //fprintf(stderr,"use old code library - annotation ->%s<-\n",annot_data);
#else
    /* new libuninameslist library code */
    annot_data=copy(uniNamesList_annot(unienc));
    //fprintf(stderr,"new library - annotation ->%s<-\n",annot_data);
#endif
#else
    /* libunicodesnames library code */
    annot_data=copy(uninm_annotation(names_db,(unsigned int)(unienc)));
    //fprintf(stderr,"libunicodes library - annotation ->%s<-\n",annot_data);
#endif
#endif

    return( annot_data );
}
