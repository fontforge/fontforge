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

#include <fontforge-config.h>

#include "ffglib.h"
#include "unicodelibinfo.h"
#include "uninameslist.h"
#include "ustring.h"

static const char *chosung[] = { "G", "GG", "N", "D", "DD", "L", "M", "B",	\
	"BB", "S", "SS", "", "J", "JJ", "C", "K", "T", "P", "H", NULL };
static const char *jungsung[] = { "A", "AE", "YA", "YAE", "EO", "E", "YEO",	\
	"YE", "O", "WA", "WAE", "OE", "YO", "U", "WEO", "WE", "WI", "YU",	\
	"EU", "YI", "I", NULL };
static const char *jongsung[] = { "", "G", "GG", "GS", "N", "NJ", "NH", "D",	\
	"L", "LG", "LM", "LB", "LS", "LT", "LP", "LH", "M", "B", "BS", "S",	\
	"SS", "NG", "J", "C", "K", "T", "P", "H", NULL };

/* Return the unicode name for the value given from a data library.	  */
/* If there's no data available for this code, return NULL.               */
/* User should free the return string when finished with this information */
char *unicode_name(int32 unienc) {
    char *name_data=NULL;

    if ( unienc<0 || unienc>=0x110000 )
	return( NULL );

    name_data=copy(uniNamesList_name(unienc));

    /* George Williams' improvisation on Hangul Syllable range
     * As of Unicode 6.3.0 0xAC00 - 0xD7A3 is defined as a block
     * without individual code point names.
     * Code moved here from fontforgeexe/fontview.c
     * FIXME: maybe this belongs to lower library stack instead,
     * revisit later.
     */
    if( ( unienc >= 0xAC00 && unienc <= 0xD7A3 ) && ( name_data == NULL ) ) {
	if( ( ( unienc - 0xAC00 ) % 28 ) == 0 ) {
	    name_data = smprintf( "Hangul Syllable %s-%s",
		    chosung [ (unienc - 0xAC00) / (21*28) ],
		    jungsung[ ((unienc - 0xAC00) / 28 ) % 21 ] );
	} else {
	    name_data = smprintf( "Hangul Syllable %s-%s-%s",
		    chosung [ (unienc - 0xAC00) / (21*28) ],
		    jungsung[ ((unienc - 0xAC00) / 28 ) % 21 ],
		    jongsung[ (unienc - 0xAC00) % 28 ] );
	}
    }

    return( name_data );
}

/* Return nicer looking unicode annotations by changing the '*' to bullet */
/* and other markers to fancier utf8 style symbols. if empty, return NULL */
/* User should free the return string when finished with this information */
static char *unicode_nicer(const char *from) {
    const char *pf;
    char ch,*to,*pt;
    long c;
    int l;

    if ( from==NULL )
	return( NULL );

    /* check if we need to convert some chars to bullets and symbols */
    c=l=0;
    for ( pf=from; (ch=*pf++)!='\0'; ++l ) if ( ch=='\t' ) {
	/* require extra space for these larger utf8 type chars */
	if ( *pf=='*' || *pf=='x' || *pf==':' || *pf=='#' ) ++c;
    }

    if ( (pt=to=malloc(l+c+c+1))!=NULL ) {
	if ( c ) {
	    while ( (ch=*pt++=*from++)!='\0' ) if (ch=='\t' ) {
		if ( *from=='*' ) {
		    c=0x2022; goto unicode_expand_c; /* 0x2022, bullet */
		} else if ( *from=='x' ) {
		    c=0x2192; goto unicode_expand_c; /* 0x2192, right-arrow */
		} else if ( *from==':' ) {
		    c=0x224d; goto unicode_expand_c; /* 0x224d, nearly equal */
		} else if ( *from=='#' ) {
		    c=0x2245; goto unicode_expand_c; /* 0x2245, approx equal */
unicode_expand_c:
		    ++from;
		    *pt++ =0xe0+((c>>12)&0x0f);
		    *pt++ =0x80+((c>>6)&0x3f);
		    *pt++ =0x80+(c&0x3f);
		}
	    }
	} else
	    /* simply copy information verbatim, without the need to edit */
	    while ( (*pt++=*from++)!='\0' );
    }

    return( to );
}

/* Return the unicode annotation for the value given from a data library. */
/* If there's no data available for this code, return NULL.               */
/* User should free the return string when finished with this information */
char *unicode_annot(int32 unienc) {
    if ( unienc<0 || unienc>=0x110000 )
	return( NULL );

    return unicode_nicer(uniNamesList_annot(unienc));
}

/* Return the number of unicode blocks contained in this NameList library */
int32 unicode_block_count(void) {
    return uniNamesList_blockCount();
}

/* Return the unicode value for the start of next unicode block. */
int32 unicode_block_start(int32 block_i) {
    return uniNamesList_blockStart(block_i);
}

/* Return the unicode value for the end of this unicode block. */
int32 unicode_block_end(int32 block_i) {
    return uniNamesList_blockEnd(block_i);
}

char *unicode_block_name(int32 block_i) {
/* Return the unicode name for this unicode block. */

    return copy(uniNamesList_blockName(block_i));
}

/* Get count for libuninameslist names2 table list-size available to read */
int32 unicode_names2cnt(void) {
    return( (int32)(uniNamesList_names2cnt()) );
}

/* Return unicode value for "Nth" internal table location or -1 if error. */
int32 unicode_names2valFrmTab(int32 n) {
    return( (int32)(uniNamesList_names2val(n)) );
}

/* Return "Nth" internal_table_location for this unicode value. None==-1. */
int32 unicode_names2getUtabLoc(int32 unienc) {
    return( (int32)(uniNamesList_names2getU((unsigned long)(unienc))) );
}

/* Return unicode name2 for "Nth" internal table location. NULL if error. */
/* NOTE: free() the string before exiting fontforge to avoid memory leak. */
char *unicode_name2FrmTab(int32 n) {
    int l;
    char *p;
    if ( n<0 || n>=uniNamesList_names2cnt() )
	return( NULL );
    l=uniNamesList_names2lnC((int)(n))*sizeof(char);
    if ( (p=(char *)(calloc(1,l+sizeof(char))))==NULL )
	return( NULL );
    memcpy(p,uniNamesList_names2anC((int)(n)),l);
    return( p );
}

/* Return unicode name2 for unicode "unienc" if exists, else return NULL. */
/* NOTE: free() the string before exiting fontforge to avoid memory leak. */
char *unicode_name2(int32 unienc) {
    return( unicode_name2FrmTab(unicode_names2getUtabLoc(unienc)) );
}
