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

#ifndef _GUTILS_UNICODELIBINFO_C_
#define _GUTILS_UNICODELIBINFO_C_

#include <fontforge-config.h>

#include "unicodelibinfo.h"
#include <ustring.h>
#include <ffglib.h>
#include <glib/gprintf.h>
#include "xvasprintf.h"

#ifndef _NO_LIBUNINAMESLIST
#include <uninameslist.h>
#else
struct unicode_block {
	int start, end;
	const char *name;
};
#endif
const struct unicode_nameannot * const *const *_UnicodeNameAnnot = NULL; /* deprecated */
const struct unicode_block *_UnicodeBlock = NULL;
#ifndef _NO_LIBUNICODENAMES
#include <libunicodenames.h>
uninm_names_db names_db; /* Unicode character names and annotations database */
uninm_blocks_db blocks_db;
#endif

#if _NO_LIBUNINAMESLIST && _NO_LIBUNICODENAMES
#else
static const char *chosung[] = { "G", "GG", "N", "D", "DD", "L", "M", "B",	\
	"BB", "S", "SS", "", "J", "JJ", "C", "K", "T", "P", "H", NULL };
static const char *jungsung[] = { "A", "AE", "YA", "YAE", "EO", "E", "YEO",	\
	"YE", "O", "WA", "WAE", "OE", "YO", "U", "WEO", "WE", "WI", "YU",	\
	"EU", "YI", "I", NULL };
static const char *jongsung[] = { "", "G", "GG", "GS", "N", "NJ", "NH", "D",	\
	"L", "LG", "LM", "LB", "LS", "LT", "LP", "LH", "M", "B", "BS", "S",	\
	"SS", "NG", "J", "C", "K", "T", "P", "H", NULL };
#endif

void inituninameannot(void) {
/* Initialize unicode name-annotation library access for FontForge */

#if _NO_LIBUNINAMESLIST
    _UnicodeNameAnnot = NULL; /* libuninameslist not available */
    _UnicodeBlock = NULL;
#else
    /* backward compatibility for other programs using libuninames */
    _UnicodeNameAnnot = UnicodeNameAnnot;
    _UnicodeBlock = UnicodeBlock;
#endif

#ifndef _NO_LIBUNICODENAMES
    /* Open database file, read data for this 'local', then close. */
    char *names_db_file;
    char *blocks_db_file;

    /* Load character names and annotations that come from the Unicode NamesList.txt */
    /* This should not be done until after the locale has been set */
    names_db_file = uninm_find_names_db(NULL);
    names_db = (names_db_file == NULL) ? ((uninm_names_db) 0) : uninm_names_db_open(names_db_file);
    free(names_db_file);
    /* NOTE: you need to do uninm_names_db_close(names_db); when you exit program */

    blocks_db_file = uninm_find_blocks_db(NULL);
    blocks_db = (blocks_db_file == NULL) ? ((uninm_blocks_db) 0) : uninm_blocks_db_open(blocks_db_file);
    free(blocks_db_file);
    /* NOTE: you need to do uninm_blocks_db_close(blocks_db); when you exit program */
#endif
}

char *unicode_name(int32 unienc) {
/* Return the unicode name for the value given from a data library.	  */
/* If there's no data available for this code, or no library, return NULL */
/* User should free the return string when finished with this information */

#if _NO_LIBUNINAMESLIST && _NO_LIBUNICODENAMES
    /* no nameslist library code available to use */
    //fprintf(stderr,"no library\n");
    return( NULL );
#else
    /* have nameslist library code available to use */
    if ( unienc<0 || unienc>=0x110000 )
	return( NULL );

    char *name_data=NULL;
#ifndef _NO_LIBUNINAMESLIST
#if (_LIBUNINAMESLIST_FUN <= 3)
    /* old libuninameslist library code */
    if ( _UnicodeNameAnnot!=NULL &&
	    _UnicodeNameAnnot[unienc>>16][(unienc>>8)&0xff][unienc&0xff].name!=NULL ) {
	name_data=copy(_UnicodeNameAnnot[unienc>>16][(unienc>>8)&0xff][unienc&0xff].name);
    }
    //fprintf(stderr,"use old code library ->%s<-\n",name_data);
#else
    /* new libuninameslist library code */
    name_data=copy(uniNamesList_name(unienc));
    //fprintf(stderr,"new library ->%s<-\n",name_data\n");
#endif
#else
    /* libunicodesnames library code */
    name_data=copy(uninm_name(names_db,(unsigned int)(unienc)));
    //fprintf(stderr,"libunicodes library ->%s<-\n",name_data);
#endif

    /* George Williams' improvisation on Hangul Syllable range
     * As of Unicode 6.3.0 0xAC00 - 0xD7A3 is defined as a block
     * without individual code point names.
     * Code moved here from fontforgeexe/fontview.c
     * FIXME: maybe this belongs to lower library stack instead,
     * revisit later.
     */
    if( ( unienc >= 0xAC00 && unienc <= 0xD7A3 ) && ( name_data == NULL ) ) {
	if( ( ( unienc - 0xAC00 ) % 28 ) == 0 ) {
	    name_data = xasprintf( "Hangul Syllable %s-%s",
		    chosung [ (unienc - 0xAC00) / (21*28) ],
		    jungsung[ ((unienc - 0xAC00) / 28 ) % 21 ] );
	} else {
	    name_data = xasprintf( "Hangul Syllable %s-%s-%s",
		    chosung [ (unienc - 0xAC00) / (21*28) ],
		    jungsung[ ((unienc - 0xAC00) / 28 ) % 21 ],
		    jongsung[ (unienc - 0xAC00) % 28 ] );
	}
    }

    return( name_data );
#endif
}

#ifndef _NO_LIBUNINAMESLIST
static char *unicode_nicer(const char *from) {
/* Return nicer looking unicode annotations by changing the '*' to bullet */
/* and other markers to fancier utf8 style symbols. if empty, return NULL */
/* User should free the return string when finished with this information */
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

static int unicode_block_check(int block_i) {
#if (_LIBUNINAMESLIST_FUN <= 3)
/* Older uninameslist database needs to be checked from start since we do */
/* not know which is the last block. Currently this should be around 234. */
    if ( _UnicodeBlock!=NULL ) {
	int i;

	for ( i=0; i<block_i; ++i )
	    if ( _UnicodeBlock[i].end>=0x10ffff )
		break;
	if ( i==block_i )
	    return( i );
    }
    return( -1 );
#else
    /* libuninameslist-0.4.2014____ has a function to return block count. */
    if ( _UnicodeBlock!=NULL && block_i < uniNamesList_blockCount() )
	return( block_i );
    return( -1 );
#endif
}
#endif

char *unicode_annot(int32 unienc) {
/* Return the unicode annotation for the value given from a data library. */
/* If there's no data available for this code, or no library, return NULL */
/* User should free the return string when finished with this information */

#if _NO_LIBUNINAMESLIST && _NO_LIBUNICODENAMES
    /* no nameslist library code available to use */
    //fprintf(stderr,"no library - annotation\n");
    return( NULL );
#else
    /* have nameslist library code available to use */
    if ( unienc<0 || unienc>=0x110000 )
	return( NULL );

    char *annot_data=NULL;

#ifndef _NO_LIBUNINAMESLIST
#if (_LIBUNINAMESLIST_FUN <= 3)
    /* old libuninameslist library code */
    if ( _UnicodeNameAnnot!=NULL &&
	    _UnicodeNameAnnot[unienc>>16][(unienc>>8)&0xff][unienc&0xff].annot!=NULL ) {
	annot_data=unicode_nicer(_UnicodeNameAnnot[unienc>>16][(unienc>>8)&0xff][unienc&0xff].annot);
    }
    //fprintf(stderr,"use old code unicode_annot() - annotation ->%s<-\n",annot_data);
#else
    /* new libuninameslist library code */
    annot_data=unicode_nicer(uniNamesList_annot(unienc));
    //fprintf(stderr,"new unicode_annot() - annotation ->%s<-\n",annot_data);
#endif
#else
    /* libunicodesnames library code */
    annot_data=copy(uninm_annotation(names_db,(unsigned int)(unienc)));
    //fprintf(stderr,"libunicodes unicode_annot() - annotation ->%s<-\n",annot_data);
#endif

    return( annot_data );
#endif
}

int32 unicode_block_count(void) {
/* Return the number of unicode blocks contained in this NameList library */
#if _NO_LIBUNINAMESLIST && _NO_LIBUNICODENAMES
    /* no nameslist library available to use */
    return( -1 );
#else
    int32 unicount;

    unicount=-1;
#ifndef _NO_LIBUNINAMESLIST
#if (_LIBUNINAMESLIST_FUN >= 4)
    /* version 0.4+ have function to do this */
    unicount=uniNamesList_blockCount();
    //fprintf(stderr,"ver 0.4+ unicode_block_count(), have %d\n",unicount);
#else
    /* old libuninameslist<=0.3 library code */
    if ( _UnicodeBlock!=NULL ) {
	int i;

	for ( i=0; i<100000; ++i )
	    if ( _UnicodeBlock[i].end>=0x10ffff )
		break;
	if ( i>0 )
	    unicount = i;
	//fprintf(stderr,"old code unicode_block_count(), have %d, %d\n",i,unicount);
    }
#endif
#else
    /* libunicodesnames library code */
    unicount=uninm_num_blocks(blocks_db);
    //fprintf(stderr,"libunicodes unicode_block_count() have %d\n",unicount);
#endif

    return( unicount );
#endif
}

int32 unicode_block_start(int32 block_i) {
/* Return the unicode value for the start of next unicode block. If no	  */
/* library or data available, then return -1.				  */

#if _NO_LIBUNINAMESLIST && _NO_LIBUNICODENAMES
    /* no nameslist library available to use */
    //fprintf(stderr,"no block library\n");
    return( -1 );
#else
    int32 unistart;

    unistart=-1;
#ifndef _NO_LIBUNINAMESLIST
#if (_LIBUNINAMESLIST_FUN >= 4)
    /* version 0.4+ have function to do this */
    unistart=uniNamesList_blockStart(block_i);
    //fprintf(stderr,"ver 0.4+ code unicode_block_start(), have %d %d\n",block_i,unistart);
#else
    /* old libuninameslist<=0.3 library code */
    if ( (unistart=unicode_block_check(block_i))>=0 )
	unistart=_UnicodeBlock[unistart].start;
    //fprintf(stderr,"use old code unicode_block_start(), have %d %d\n",block_i,unistart);
#endif
#else
    /* libunicodesnames library code */
    unistart=uninm_block_start(blocks_db,(unsigned int)(block_i));
    //fprintf(stderr,"libunicodes unicode_block_start()have %d %d\n",block_i,unistart);
#endif

    return( unistart );
#endif
}

int32 unicode_block_end(int32 block_i) {
/* Return the unicode value for the end of this unicode block. If there	  */
/* is no library or data available, then return -1			  */

#if _NO_LIBUNINAMESLIST && _NO_LIBUNICODENAMES
    /* no nameslist library available to use */
    //fprintf(stderr,"no block library\n");
    return( -1 );
#else
    int32 uniend;

    uniend=-1;
#ifndef _NO_LIBUNINAMESLIST
#if (_LIBUNINAMESLIST_FUN >= 4)
    /* version 0.4+ have function to do this */
    uniend=uniNamesList_blockEnd(block_i);
    //fprintf(stderr,"ver 0.4+ code unicode_block_end(), have %d %d\n",block_i,uniend);
#else
    /* old libuninameslist<=0.3 library code */
    if ( (uniend=unicode_block_check(block_i))>=0 )
	uniend=_UnicodeBlock[uniend].end;
    //fprintf(stderr,"use old code unicode_block_end(), have %d %d\n",block_i,uniend);
#endif
#else
    /* libunicodesnames library code */
    uniend=uninm_block_end(blocks_db,(unsigned int)(block_i));
    //fprintf(stderr,"libunicodes unicode_block_end(), have %d %d\n",block_i,uniend);
#endif

    return( uniend );
#endif
}

char *unicode_block_name(int32 block_i) {
/* Return the unicode name for this unicode block. If there is no library */
/* then return NULL							  */

#if _NO_LIBUNINAMESLIST && _NO_LIBUNICODENAMES
    /* no nameslist library code available to use */
    //fprintf(stderr,"no library\n");
    return( NULL );
#else
    char *name_data=NULL;
#ifndef _NO_LIBUNINAMESLIST
#if (_LIBUNINAMESLIST_FUN >= 4)
    /* version 0.4+ have function to do this */
    name_data=copy(uniNamesList_blockName(block_i));
    //fprintf(stderr,"ver 0.4+ code unicode_block_name(), have %d ->%s<-\n",block_i,name_data);
#else
    int i;
    /* old libuninameslist<=0.3 library code */
    if ( (i=unicode_block_check(block_i))>=0 )
	name_data=copy(_UnicodeBlock[i].name);
    //fprintf(stderr,"use old code unicode_block_name(), have %d %d ->%s<-\n",i,block_i,name_data);
#endif
#else
    /* libunicodesnames library code */
    name_data=copy(uninm_block_name(blocks_db,(unsigned int)(block_i)));
    //fprintf(stderr,"libunicodes unicode_block_name() %d ->%s<-\n",block_i,name_data);
#endif

    return( name_data );
#endif
}

/* libuninameslist 0.5 and higher functions, otherwise return NULL or -1. */
#if !(_NO_LIBUNINAMESLIST) && (_LIBUNINAMESLIST_FUN >= 5)
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
#else
/* libuninameslist ver0.4 or older, or libunicodenames return NULL or -1. */
/* for these older libraries, we would need to search entire list to find */
/* about 28 or less possible names2 codes. This isn't an effective use of */
/* computer use and it's better for user to use a more recent names list. */
int32 unicode_names2cnt(void) {
    return( -1 );
}
int32 unicode_names2valFrmTab(int32 n) {
    return( -1 );
}
int32 unicode_names2getUtabLoc(int32 unienc) {
    return( -1 );
}
char *unicode_name2FrmTab(int32 n) {
    return( NULL );
}
char *unicode_name2(int32 unienc) {
    return( NULL );
}
#endif

char *unicode_library_version(void) {
/* Return the unicode version for this library. Sometimes users still use */
/* older libuninameslist or libunincodenames libraries because they don't */
/* realize that these need to be updated to keep current too but not made */
/* at same time that FontForge is released (release dates not in sync).   */

#if !(_NO_LIBUNINAMESLIST) && (_LIBUNINAMESLIST_FUN >= 3)
    /* libuninameslist-0.3.20130501-1 and later have a "version" function */
    char *version_str;

    version_str=copy(uniNamesList_NamesListVersion());

    return( version_str );
#else
    /* Libunicodenames and older versions of libuninames don't have a ver */
    /* function so we need to either test various annotations to find out */
    /* what version we have, or we keep it simple and just return nothing */
    return( NULL );
#endif
}

#endif
