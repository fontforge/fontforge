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

/*
 * This program generates 4 different files based on the latest UnicodeData.txt
 * obtained from http://unicode.org which is then used to build FontForge
 *
 * To generate the latest files, you will first need to go and get these 4 files
 * and put them in the Unicode subdirectory:
 *	http://unicode.org/Public/UNIDATA/LineBreak.txt
 *	http://unicode.org/Public/UNIDATA/NamesList.txt
 *	http://unicode.org/Public/UNIDATA/PropList.txt
 *	http://unicode.org/Public/UNIDATA/UnicodeData.txt
 *
 * Next, you will need to build ./makeutype before you can use it:
 * Run "make makeutype"
 * or
 * Run "gcc -s -I../inc -o makeutype makeutype.c"
 *
 * Then run the executable binary "/makeutype".
 * This will create 4 files in the same directory:
 *	ArabicForms.c, unialt.c, utype.c, utype.h
 * (please move utype.h into Fontforge's "../inc" subdirectory)
 *
 * When done building the updated files, you can clean-up by removing
 * LineBreak.txt, NamesList.txt, PropList.txt,UnicodeData.txt, and the
 * binary executable file makeutype as they are no longer needed now.
 */
#include <fontforge-config.h>


/* Build a ctype array out of the UnicodeData.txt and PropList.txt files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <basics.h>

/*#define MAXC		0x600		/* Last upper/lower case dicodomy is Armenian 0x580, er, nope. 1fff (greek and latin extended) then full-width 0xff00 */
#define MAXC	65536
#define MAXA	18

/* These values get stored within flags[unicodechar={0..MAXC}] */
#define _LOWER		1
#define _UPPER		2
#define _TITLE		4
#define _DIGIT		8
#define _SPACE		0x10
#define _PUNCT		0x20
#define _HEX		0x40
#define _ZEROWIDTH	0x80

#define _LEFT_2_RIGHT	0x100
#define _RIGHT_2_LEFT	0x200
#define _ENUMERIC	0x400
#define _ANUMERIC	0x800
#define _ENS		0x1000
#define _CS		0x2000
#define _ENT		0x4000
#define _COMBINING	0x8000

#define _BREAKBEFOREOK	0x10000
#define _BREAKAFTEROK	0x20000
#define _NONSTART	0x40000		/* small kana, close punct, can't start a line */
#define _NONEND		0x80000		/* open punct, can't end a line */
/*#define _MUSTBREAK	0x100000	/* newlines, paragraphs, etc. */
#define	_URLBREAKAFTER	0x100000	/* break after slash not followed by digits (ie. in URLs not fractions or dates) */

#define _ALPHABETIC	0x200000
#define _IDEOGRAPHIC	0x400000

#define _INITIAL	0x800000
#define _MEDIAL		0x1000000
#define _FINAL		0x2000000
#define _ISOLATED	0x4000000

#define _NOBREAK	0x8000000
#define _DecompositionNormative	0x10000000
#define _LIG_OR_FRAC	0x20000000

#define _CombiningClass		0xff
#define _Above			0x100
#define _Below			0x200
#define _Overstrike		0x400
#define _Left			0x800
#define _Right			0x1000
#define _Joins2			0x2000
#define _CenterLeft		0x4000
#define _CenterRight		0x8000
#define _CenteredOutside	0x10000
#define _Outside		0x20000
#define _RightEdge		0x40000
#define _LeftEdge		0x80000
#define _Touching		0x100000

#include "combiners.h"

char *names[MAXC];
unsigned short mytolower[MAXC];
unsigned short mytoupper[MAXC];
unsigned short mytotitle[MAXC];
unsigned char mynumericvalue[MAXC];
unsigned short mymirror[MAXC];
unsigned long flags[MAXC];			/* 32 binary flags for each unicode.org character */
unsigned long flags2[MAXC];
unichar_t alts[MAXC][MAXA+1];
unsigned long assignedcodepoints[0x120000/32];	/* 32 characters represented per each long value */

int frm, lgm, vfm;				/* identify all ligatures and fractions */
#define LG_MAX 550
#define FR_MAX 100
#define VF_MAX 50
unsigned long ligature[LG_MAX];
unsigned long fraction[FR_MAX];
unsigned long vulgfrac[VF_MAX];

const char GeneratedFileMessage[] = "\n/* This file was generated using the program 'makeutype' */\n\n";
const char CantReadFile[] = "Can't find or read file %s\n";		/* exit(1) */
const char CantSaveFile[] = "Can't open or write to output file %s\n";	/* exit(2) */
const char NoMoreMemory[] = "Can't access more memory.\n";		/* exit(3) */
const char LineLengthBg[] = "Error with %s. Found line too long: %s\n";	/* exit(4) */
const char LgFrcTooMany[] = "Error. Too many %s, stopped at %s[%d]=U+%X\n"; /* exit(5) */

static void FreeNamesMemorySpace() {
    long index;
    for ( index=0; index<MAXC ; ++index )
	free( names[index] );
}

static void FigureAlternates(long index, char *apt, int normative) {
    int alt, i;
    char *end;
    int isisolated=0, iscircled=0;

    for ( i=0; ; ++i ) {
	while ( *apt=='<' ) {
	    isisolated = strncmp(apt,"<isolated>",strlen("<isolated>"))==0;
	    iscircled = strncmp(apt,"<circle>",strlen("<circle>"))==0;
	    while ( *apt && *apt!='>' ) ++apt;
	    if ( *apt=='>' ) ++apt;
	    while ( *apt==' ' ) ++apt;
	}
	alt = strtol(apt,&end,16);
	if ( end==apt )
    break;
	apt = end;
	if ( i<MAXA ) {
	    alts[index][i] = alt;
	    alts[index][i+1] = 0;
	}
    }
    if ( iscircled && i<MAXA ) {
	alts[index][i] = 0x20dd;
	alts[index][++i] = 0;
    }
    if ( i>MAXA )
	fprintf( stderr, "%d is too many alternates for U+%04X\n", i, index );
    if ( i>0 && normative)
	flags[index] |= _DecompositionNormative;

    /* arabic isolated forms are alternates for the standard forms */
    if ( isisolated && alts[index][0]>=0x600 && alts[index][0]<0x6ff && alts[index][1]==0 &&
	    alts[alts[index][0]][0]==0 )
	alts[alts[index][0]][0] = index;
}

static int processAssignment(long index,char *pt,long *flg) {
    static long first=-1;
    long i;

    if ( index>0x11ffff ) return( 0 );	/* Skip values over unicode max */
    if ( index<0 ||			/* Error if negative code given */ \
	 *(++pt)=='\0' ) return( -1 );	/* Skip '<'. Error if empty str */
    if ( *pt!='<' ) {
	assignedcodepoints[index/32] |= (1<<(index%32));	/* This Unicode char is visible */
	/* Collect ligatures, vulgar fractions and other fractions */
	if ( strstr(pt,"LIGATURE") ) {
	    /* This codepoint index is a ligature */
	    /* fprintf( stderr, "ligature[%d]=U+%X, = %s\n", lgm, index, pt ); */
	    if ( lgm >= LG_MAX ) {
		fprintf( stderr, LgFrcTooMany, "ligatures", "ligature", lgm, index );
		return( 5 );
	    }
	    ligature[lgm] = index;
	    if ( index < MAXC ) {
		*flg |= _LIG_OR_FRAC;
	    }
	    lgm++;
	} else if ( strstr(pt,"VULGAR" /* FRACTION */) ) {
	    /* This codepoint index is a vulgar fraction */
	    /* fprintf( stderr, "vulgfrac[%d]=U+%X, = %s\n", vfm, index, pt ); */
	    if ( vfm >= VF_MAX ) {
		fprintf( stderr, LgFrcTooMany, "fractions", "vulgfrac", vfm, index );
		return( 5 );
	    }
	    vulgfrac[vfm] = index;
	    if ( index < MAXC ) {
		*flg |= _LIG_OR_FRAC;
	    }
	    vfm++;
	} else if ( strstr(pt,"FRACTION") ) {
	    /* This codepoint index is a fraction */
	    /* fprintf( stderr, "fraction[%d]=U+%X, = %s\n", frm, index, pt ); */
	    if ( frm >= FR_MAX ) {
		fprintf( stderr, LgFrcTooMany, "fractions", "fraction", frm, index );
		return( 5 );
	    }
	    fraction[frm] = index;
	    if ( index < MAXC ) {
		*flg |= _LIG_OR_FRAC;
	    }
	    frm++;
	}
    } else if ( strstr(pt,", First")!=NULL ) {			/* start of an extended charset */
	first = index;
    } else if ( strstr(pt,", Last")!=NULL ) {			/* end of an extended charset */
	if ( first==-1 || first > index )
	    fprintf( stderr,"Something went wrong, first isn't defined at last. %x\n", index );
	else if ( first>=0xd800 && first<=0xdfff )
	    /* surrogate pairs. Not assigned really */;
	else {
	    /* mark all characters visible in the range of {First...Last} */
	    for ( i=first; i<=index; ++i )
		assignedcodepoints[i/32] |= (1<<(i%32));
	}
	first = -1;
    }
    return( 0 );
}

static void readin(void) {
    char buffer[512+1], buf2[300+1], oldname[301], *pt, *end, *pt1;
    long index, lc, uc, tc, flg, val, indexend, wasfirst;
    int  cc;
    FILE *fp;
    int i,j;

    buffer[512]='\0'; buf2[0] = buf2[300]='\0'; oldname[0]='\0';
    if ((fp = fopen("UnicodeData.txt","r"))==NULL ) {
	fprintf( stderr, CantReadFile,"UnicodeData.txt" );
	exit(1);
    }
    while ( fgets(buffer,sizeof(buffer)-1,fp)!=NULL ) {
	if (strlen(buffer)>=299) {	/* previous version was linelength of 300 chars, jul2012 */
	    fprintf( stderr, LineLengthBg,"UnicodeData.txt",buffer );
	    fprintf( stderr, "\n%s\n",buffer );

	    fclose(fp);
	    FreeNamesMemorySpace();
	    exit(4);
	}
	if ( *buffer=='#' )
    continue;
	flg = 0;
	/* Unicode character value */
	index = strtol(buffer,&end,16);
	if ( processAssignment(index,end,&flg) ) {
	    fclose(fp);
	    FreeNamesMemorySpace();
	    exit(5);
	}
	if ( index>=MAXC )		/* For now can only deal with BMP !!!! */
    continue;
	pt = end;
	if ( *pt==';' ) {
	    ++pt;
	    /* buf2 = character name */
	    for ( pt1=pt; *pt1!=';' && *pt1!='\0'; ++pt1 );
	    strncpy(buf2,pt,pt1-pt); buf2[pt1-pt] = '\0'; pt = pt1;
	    if ( *pt==';' ) ++pt;
	    /* general category */
	    for ( pt1=pt; *pt1!=';' && *pt1!='\0'; ++pt1 );
	    if ( strncmp(pt,"Lu",pt1-pt)==0 )
		flg |= _UPPER|_ALPHABETIC;
	    else if ( strncmp(pt,"Ll",pt1-pt)==0 )
		flg |= _LOWER|_ALPHABETIC;
	    else if ( strncmp(pt,"Lt",pt1-pt)==0 )
		flg |= _TITLE|_ALPHABETIC;
	    else if ( strncmp(pt,"Lo",pt1-pt)==0 )
		flg |= _ALPHABETIC;
	    else if ( strncmp(pt,"Nd",pt1-pt)==0 )
		flg |= _DIGIT;
	    pt = pt1;
	    if ( *pt==';' ) ++pt;
	    /* Unicode combining classes, I do my own version later */
	    cc = strtol(pt,&end,16);
	    pt = end;
	    if ( *pt==';' ) ++pt;
	    /* Bidirectional Category */
	    for ( pt1=pt; *pt1!=';' && *pt1!='\0'; ++pt1 );
	    if ( strncmp(pt,"L",pt1-pt)==0 || strncmp(pt,"LRE",pt1-pt)==0 || strncmp(pt,"LRO",pt1-pt)==0 )
		flg |= _LEFT_2_RIGHT;
	    if ( strncmp(pt,"R",pt1-pt)==0 || strncmp(pt,"AL",pt1-pt)==0 || strncmp(pt,"RLE",pt1-pt)==0 || strncmp(pt,"RLO",pt1-pt)==0 )
		flg |= _RIGHT_2_LEFT;
	    else if ( strncmp(pt,"EN",pt1-pt)==0 )
		flg |= _ENUMERIC;
	    else if ( strncmp(pt,"ES",pt1-pt)==0 )
		flg |= _ENS;
	    else if ( strncmp(pt,"ET",pt1-pt)==0 )
		flg |= _ENT;
	    else if ( strncmp(pt,"AN",pt1-pt)==0 )
		flg |= _ANUMERIC;
	    else if ( strncmp(pt,"CS",pt1-pt)==0 )
		flg |= _CS;
	    pt = pt1;
	    if ( *pt==';' ) ++pt;
	    /* character decomposition */
	    if ( strncmp(pt,"<initial>",strlen("<initial>"))==0 )
		flg |= _INITIAL;
	    else if ( strncmp(pt,"<final>",strlen("<final>"))==0 )
		flg |= _FINAL;
	    else if ( strncmp(pt,"<medial>",strlen("<medial>"))==0 )
		flg |= _MEDIAL;
	    else if ( strncmp(pt,"<isolated>",strlen("<isolated>"))==0 )
		flg |= _ISOLATED;
	    FigureAlternates(index,pt, true);
	    while ( *pt!=';' && *pt!='\0' ) ++pt;
	    if ( *pt==';' ) ++pt;
	    /* Don't care about decimal digit value */
	    while ( *pt!=';' && *pt!='\0' ) ++pt;
	    if ( *pt==';' ) ++pt;
	    /* Don't care about digit value */
	    while ( *pt!=';' && *pt!='\0' ) ++pt;
	    if ( *pt==';' ) ++pt;
	    /* numeric value */
	    val = strtol(pt,&end,10);
	    if ( pt==end ) val = -1;
	    pt = end;
	    if ( *pt==';' ) ++pt;
	    /* Don't care about mirrored value */
	    while ( *pt!=';' && *pt!='\0' ) ++pt;
	    if ( *pt==';' ) ++pt;
	    /* Only care about old name (unicode 1.0) for control characters */
	    for ( pt1=pt; *pt1!=';' && *pt1!='\0'; ++pt1 );
	    strncpy(oldname,pt,pt1-pt); oldname[pt1-pt] = '\0';
	    if ( pt1-pt>100 ) oldname[100] = '\0'; pt = pt1;
	    if ( *pt==';' ) ++pt;
	    /* Don't care about 10646 comment field */
	    while ( *pt!=';' && *pt!='\0' ) ++pt;
	    if ( *pt==';' ) ++pt;
	    /* upper-case value */
	    uc = strtol(pt,&end,16);
	    if ( end==pt )
		uc = index;
	    pt = end;
	    if ( *pt==';' ) ++pt;
	    /* lower-case value */
	    lc = strtol(pt,&end,16);
	    if ( end==pt )
		lc = index;
	    pt = end;
	    if ( *pt==';' ) ++pt;
	    /* title-case value */
	    tc = strtol(pt,&end,16);
	    if ( end==pt )
		tc = index;
	    pt = end;
	    if ( *pt==';' ) ++pt;
	    if ( index>=MAXC )
    break;
	    mytolower[index]= lc;
	    mytoupper[index]= uc;
	    mytotitle[index]= tc;
	    mynumericvalue[index]= val;
	    flags[index] |= flg;
	    flags2[index] = cc&0xff;
	    if ( strstr(buf2," First>")!=NULL )
		wasfirst = index;
	    else if ( strstr(buf2," Last>")!=NULL ) {
		for ( ; wasfirst<index; ++wasfirst ) {
		    mytolower[wasfirst]= wasfirst;
		    mytoupper[wasfirst]= wasfirst;
		    mytotitle[wasfirst]= wasfirst;
		    mynumericvalue[wasfirst]= -1;
		    flags[wasfirst]  = flg;
		    flags2[wasfirst] = cc&0xff;
		}
	    } else {
		if ( strcmp(buf2,"<control>")==0 ) {
		    strcat(buf2, " ");
		    strcat(buf2, oldname);
		}
		names[index]= strdup(buf2);
	    }
	}
    }
    fclose(fp);

    if ((fp = fopen("LineBreak.txt","r"))==NULL ) {
	fprintf( stderr, CantReadFile, "LineBreak.txt" );
	FreeNamesMemorySpace();
	exit(1);
    }
    while ( fgets(buffer,sizeof(buffer)-1,fp)!=NULL ) {
	if (strlen(buffer)>=299) {	/* previous version was linelength of 300 chars, jul2012 */
	    fprintf( stderr, LineLengthBg,"LineBreak.txt",buffer );
	    fclose(fp);
	    FreeNamesMemorySpace();
	    exit(4);
	}
	if ( *buffer=='#' )
    continue;
	flg = 0;
	/* Unicode character value */
	indexend = index = strtol(buffer,&end,16);
	if ( index>0xffff )		/* Only BMP now !!!!!! */
    continue;
	pt = end;
	if ( *pt=='.' && pt[1]=='.' ) {
	    indexend = strtol(pt+2,&end,16);
	    if ( indexend>0xffff ) indexend = 0xffff;	/* Only BMP now !!!!! */
	    pt = end;
	}
	if ( *pt==';' ) {
	    ++pt;
	    for ( pt1=pt; *pt1!=';' && *pt1!=' ' && *pt1!='\0'; ++pt1 );
	    if ( strncmp(pt,"BK",pt1-pt)==0 || strncmp(pt,"CR",pt1-pt)==0 || strncmp(pt,"LF",pt1-pt)==0 )
		/*flg |= _MUSTBREAK*/;
	    else if ( strncmp(pt,"NS",pt1-pt)==0 || strncmp(pt,"CL",pt1-pt)==0 )
		flg |= _NONSTART;
	    else if ( strncmp(pt,"OP",pt1-pt)==0 || strncmp(pt,"CM",pt1-pt)==0 )
		flg |= _NONEND;
	    else if ( strncmp(pt,"GL",pt1-pt)==0 )
		flg |= _NONEND|_NONSTART;
	    else if ( strncmp(pt,"SP",pt1-pt)==0 || strncmp(pt,"HY",pt1-pt)==0 ||
		    strncmp(pt,"BA",pt1-pt)==0 ||
		    strncmp(pt,"ZW",pt1-pt)==0 )
		flg |= _BREAKAFTEROK;
	    else if ( strncmp(pt,"BB",pt1-pt)==0 )
		flg |= _BREAKBEFOREOK;
	    else if ( strncmp(pt,"B2",pt1-pt)==0 )
		flg |= _BREAKBEFOREOK|_BREAKAFTEROK;
	    else if ( strncmp(pt,"ID",pt1-pt)==0 )
		flg |= _BREAKBEFOREOK|_BREAKAFTEROK;
	    else if ( strncmp(pt,"SY",pt1-pt)==0 )
		flg |= _URLBREAKAFTER;
	    pt = pt1;
	    for ( ; index<=indexend; ++index )
		flags[index] |= flg;
	}
    }
    fclose(fp);

    if ((fp = fopen("PropList.txt","r"))==NULL ) {
	fprintf( stderr, CantReadFile, "PropList.txt" );
	FreeNamesMemorySpace();
	exit(1);
    }
    while ( fgets(buffer,sizeof(buffer)-1,fp)!=NULL ) {
	flg = 0;
	if (strlen(buffer)>=299) {	/* previous version was linelength of 300 chars, jul2012 */
	    fprintf( stderr, LineLengthBg,"PropList.txt",buffer );
	    fclose(fp);
	    FreeNamesMemorySpace();
	    exit(4);
	}
	if ( true || strncmp(buffer,"Property dump for:", strlen("Property dump for:"))==0 ) {
	    if ( strstr(buffer, "(Zero-width)")!=NULL || strstr(buffer, "ZERO WIDTH")!=NULL )
		flg = _ZEROWIDTH;
	    else if ( strstr(buffer, "(White space)")!=NULL || strstr(buffer, "White_Space")!=NULL )
		flg = _SPACE;
	    else if ( strstr(buffer, "(Punctuation)")!=NULL || strstr(buffer, "Punctuation")!=NULL )
		flg = _PUNCT;
	    else if ( strstr(buffer, "(Alphabetic)")!=NULL || strstr(buffer, "Alphabetic")!=NULL )
		flg = _ALPHABETIC;
	    else if ( strstr(buffer, "(Ideographic)")!=NULL || strstr(buffer, "Ideographic")!=NULL )
		flg = _IDEOGRAPHIC;
	    else if ( strstr(buffer, "(Hex Digit)")!=NULL || strstr(buffer, "Hex_Digit")!=NULL )
		flg = _HEX;
	    else if ( strstr(buffer, "(Combining)")!=NULL || strstr(buffer, "COMBINING")!=NULL )
		flg = _COMBINING;
	    else if ( strstr(buffer, "(Non-break)")!=NULL )
		flg = _NOBREAK;
	    if ( flg!=0 ) {
		if (( buffer[0]>='0' && buffer[0]<='9') || (buffer[0]>='A' && buffer[0]<='F')) {
		    index = wasfirst = strtol(buffer,NULL,16);
		    if ( buffer[4]=='.' && buffer[5]=='.' )
			index = strtol(buffer+6,NULL,16);
		    for ( ; wasfirst<=index && wasfirst<=0xffff; ++wasfirst )		/* BMP !!!!! */
			flags[wasfirst] |= flg;
		}
	    }
	}
    }
    fclose(fp);
    /* There used to be a zero width property, but no longer */
    flags[0x200B] |= _ZEROWIDTH;
    flags[0x200C] |= _ZEROWIDTH;
    flags[0x200D] |= _ZEROWIDTH;
    flags[0x2060] |= _ZEROWIDTH;
    flags[0xFEFF] |= _ZEROWIDTH;
    /* There used to be a No Break property, but no longer */
    flags[0x00A0] |= _NOBREAK;
    flags[0x2011] |= _NOBREAK;
    flags[0x202F] |= _NOBREAK;
    flags[0xFEFF] |= _NOBREAK;

    if ((fp = fopen("NamesList.txt","r"))==NULL ) {
	fprintf( stderr, CantReadFile, "NamesList.txt" );
	FreeNamesMemorySpace();
	exit(1);
    }
    while ( fgets(buffer,sizeof(buffer)-1,fp)!=NULL ) {
	flg = 0;
	if (strlen(buffer)>=511) {
	    fprintf( stderr, LineLengthBg,"NamesList.txt",buffer );
	    fclose(fp);
	    FreeNamesMemorySpace();
	    exit(4);
	}
	if ( (index = strtol(buffer,NULL,16))!=0 ) {
	    if ( strstr(buffer, "COMBINING")!=NULL )
		flg = _COMBINING;
	    else if ( strstr(buffer, "N0-BREAK")!=NULL )
		flg = _NOBREAK;
	    else if ( strstr(buffer, "ZERO WIDTH")!=NULL )
		flg = _ZEROWIDTH;

	    if ( index<0xffff )		/* !!!!! BMP */
		flags[wasfirst] |= flg;
	}
    }
    fclose(fp);

    for ( i=0; combiners[i].low!=0; ++i ) {
	for ( j=combiners[i].low; j<=combiners[i].high; ++j )
	    flags2[j] |= combiners[i].pos[j-combiners[i].low];
    }
}

static void readcorpfile(char *prefix, char *corp) {
    char buffer[300+1], buf2[300+1], *pt, *end, *pt1;
    long index;
    FILE *fp;

    buffer[300]='\0'; buf2[0] = buf2[300]='\0';
    if ((fp = fopen(corp,"r"))==NULL ) {
	fprintf( stderr, CantReadFile, corp );		/* Not essential */
return;
    }
    while ( fgets(buffer,sizeof(buffer)-1,fp)!=NULL ) {
	if (strlen(buffer)>=299) {
	    fprintf( stderr, LineLengthBg,corp,buffer );
	    fclose(fp);
	    FreeNamesMemorySpace();
	    exit(4);
	}
	if ( *buffer=='#' )
    continue;
	/* code */
	index = strtol(buffer,&end,16);
	pt = end;
	if ( *pt==';' ) {
	    ++pt;
	    while ( *pt!=';' && *pt!='\0' ) ++pt;
	    if ( *pt==';' ) ++pt;
	    /* character name */
	    for ( pt1=pt; *pt1!=';' && *pt1!='\0' && *pt1!='\n' && *pt1!='\r'; ++pt1 );
	    strncpy(buf2,pt,pt1-pt); buf2[pt1-pt] = '\0'; pt = pt1;
	    if ( *pt==';' ) ++pt;
	    /* character decomposition */
	    FigureAlternates(index,pt, false);
	    if ( index>=MAXC )
    break;
	    if ((names[index]= malloc(strlen(buf2)+strlen(prefix)+4)) == NULL) {
		fprintf( stderr, NoMoreMemory );
		fclose(fp);
		FreeNamesMemorySpace();
		exit(3);
	    }
	    strcpy(names[index],prefix); strcat(names[index],buf2);
	}
    }
    fclose(fp);
}

static int find(char *base, char *suffix) {
    char name[300+1];
    int i;

    name[300]='\0';
    strcpy(name,base);
    strcat(name,suffix);

    for ( i=0; i<MAXC; ++i )
	if ( names[i]!=NULL && strcmp(names[i],name)==0 )
return( i );

return( -1 );
}

static void dumparabicdata(FILE *header) {
    FILE *data;
    struct arabicforms {
	unsigned short initial, medial, final, isolated;
	unsigned int isletter: 1;
	unsigned int joindual: 1;
	unsigned int required_lig_with_alef: 1;
    } forms[256];
    int i, j, index;

    memset(forms,'\0',sizeof(forms));
    for ( i=0x600; i<0x700; ++i ) {
	j = i-0x600;
	if ( names[i]==NULL )
	    /* No op (not defined) */;
	else if ( strncmp(names[i],"ARABIC LETTER ",strlen("ARABIC LETTER "))!=0 )
	    /* No op (not a letter, no fancy forms) */
	    forms[j].initial = forms[j].medial = forms[j].final = forms[j].isolated = i;
	else {
	    forms[j].isletter = 1;
	    forms[j].initial = forms[j].medial = forms[j].final = forms[j].isolated = i;
	    if ( (index = find(names[i]," ISOLATED FORM"))!= -1 )
		forms[j].isolated = index;
	    if ( (index = find(names[i]," FINAL FORM"))!= -1 )
		forms[j].final = index;
	    if ( (index = find(names[i]," INITIAL FORM"))!= -1 )
		forms[j].initial = index;
	    if ( (index = find(names[i]," MEDIAL FORM"))!= -1 )
		forms[j].medial = index;
	    if ( forms[j].initial!=i && forms[j].medial!=i )
		forms[j].joindual = 1;
	}
    }
    forms[0x44/* 0x644 == LAM */].required_lig_with_alef = 1;

    fprintf(header,"\nextern struct arabicforms {\n" );
    fprintf(header,"    unsigned short initial, medial, final, isolated;\n" );
    fprintf(header,"    unsigned int isletter: 1;\n" );
    fprintf(header,"    unsigned int joindual: 1;\n" );
    fprintf(header,"    unsigned int required_lig_with_alef: 1;\n" );
    fprintf(header,"} ArabicForms[256];\t/* for chars 0x600-0x6ff, subtract 0x600 to use array */\n" );

    data = fopen( "ArabicForms.c","w");
    if (data==NULL) {
	fprintf( stderr, CantSaveFile, "ArabicForms.c" );
	FreeNamesMemorySpace();
	exit(2);
    }

    fprintf( data, "/* Copyright: 2001 George Williams */\n" );
    fprintf( data, "/* License: BSD-3-clause */\n" );
    fprintf( data, "/* Contributions: Khaled Hosny, Joe Da Silva */\n" );
    fprintf( data, GeneratedFileMessage );

    fprintf( data, "#include <utype.h>\n\n" );

    fprintf( data, "struct arabicforms ArabicForms[] = {\n" );
    fprintf( data, "\t/* initial, medial, final, isolated, isletter, joindual, required_lig_with_alef */\n");
    for ( i=0; i<256; ++i ) {
	fprintf( data, "\t{ 0x%04x, 0x%04x, 0x%04x, 0x%04x, %d, %d, %d }",
		forms[i].initial, forms[i].medial, forms[i].final, forms[i].isolated,
		forms[i].isletter, forms[i].joindual, forms[i].required_lig_with_alef);
	if ( i==255 )
	    fprintf( data, "\n");
	else
	    if ( (i & 31)==0 )
		fprintf( data, ",\t/* 0x%04x */\n",0x600+i);
	    else
		fprintf( data, ",\n");
    }
    fprintf( data, "};\n" );
    fclose( data );
}

static void buildtables(FILE *data,long unsigned int *dt,int s,int m,char *t) {
/* Build ligature/vulgar/fraction table as a condensed uint16 array for the */
/* range of 't'16{0...s} and then as uint32 for the range 't'32{(s+1)...m}. */
    int i,j;

    fprintf( data, "const uint16 %s16[] = {\n", t );
    for ( i=j=0; i<s; i=i+j ) {
	fprintf( data, "  0x%04x", dt[i] );
	for ( j=1; j<8 && i+j<s; ++j ) {
	    fprintf(data, ", 0x%04x", dt[i+j] );
	}
	if ( i+j>=s )
	    fprintf(data, "};\n\n" );
	else
	    fprintf(data, ",\n" );
    }
    if ( s<m ) {
	fprintf( data, "const uint32 %s32[] = {\n", t );
	for ( i=s; i<m; i=i+j ) {
	    fprintf( data, "  0x%08x", dt[i] );
	    for ( j=1; j<4 && i+j<m; ++j ) {
		fprintf(data, ", 0x%08x", dt[i+j] );
	    }
	    if ( i+j>=m )
		fprintf(data, "};\n\n" );
	    else
		fprintf(data, ",\n" );
	}
    }
}

static void dumpbsearch(FILE *data,int s) {
/* Customize bsearch sizes to match data table sizes for uint16 and uint32. */
    fprintf( data, "static int compare_codepoints%d(const void *uCode1, const void *uCode2) {\n", s );
    fprintf( data, "    const uint%d *cp1 = (const uint%d *)(uCode1);\n", s, s );
    fprintf( data, "    const uint%d *cp2 = (const uint%d *)(uCode2);\n", s, s );
    fprintf( data, "    return( (*cp1 < *cp2) ? -1 : ((*cp1 == *cp2) ? 0 : 1) );\n}\n\n" );
}

static void dump_getU(FILE *data,long unsigned int *dt,int s,int m,char *t,char *nam) {
/* This function is used 3x to generate customized ligature vulgar_fraction */
/* and other_fraction function call returning the 'n'th table unicode value */
/* where you call this function with:					    */
/*	'm' is the maximum size of the table,				    */
/*	's' is size of the lower partition of the table (fits in uint16).   */
/*	't' is the first part of the data type name.			    */
/*	'nam' is the function_call name base.				    */
/* and the generated functions work-as:					    */
/* Get the 'n'th unicode value in the table, where {0<=n<=(sizeof(table)-1} */
/* Treat both (uint16, uint32) tables as if it is only one table. Error=-1. */
    fprintf( data, "int32 %s_get_U(int n) {\n", nam );
    fprintf( data, "    if ( n<0 || n>=%d )\n\treturn( -1 );\n    ", m );
    if ( s<m )
	fprintf( data, "if ( n<%d )\n\t", s );
    fprintf( data, "return( (int32)(%s16[n]) );\n", t );
    if ( s<m )
    fprintf( data, "    else\n\treturn( (int32)(%s32[n-%d]) );\n", t, s );
    fprintf( data, "}\n\n" );
}

static void dumpbsearchfindN(FILE *data,long unsigned int *dt,int s,int m,char *t) {
/* Find 'n' for this unicode value. Treat both (uint16, uint32) tables as a */
/* single table. 'n' will be within {0<=n<=(sizeof(table)-1}, and Error=-1. */
    fprintf( data, "    uint16 uCode16, *p16;\n" );
    if ( s<m )
	fprintf( data, "    uint32 *p32;\n" );
    fprintf( data, "    int n=-1;\n\n" );
    fprintf( data, "    if ( uCode<0x%x || uCode>0x%x || ", dt[0], dt[m-1] );
    if ( dt[m-1]<MAXC )
	fprintf( data, "isligorfrac(uCode)==0 )\n" );
    else
	fprintf( data, "(uCode<%d && isligorfrac(uCode)==0) )\n", MAXC );
    fprintf( data, "\treturn( -1 );\n    " );
    if ( s<m )
	fprintf( data, "if ( uCode<%d ) {\n\tuCode16 = uCode;\n\t", s );
    else
	fprintf( data, "uCode16 = uCode;\n    " );
    fprintf( data, "p16 = (uint16 *)(bsearch(&uCode16, %s16, %d, \\\n", t, s );
    fprintf( data, "\t\t\t\tsizeof(uint16), compare_codepoints16));\n" );
    if ( s<m )
	fprintf( data, "\t" );
    else
	fprintf( data, "    " );
    fprintf( data, "if ( p16 ) n = p16 - %s16;\n", t );
    if ( s<m ) {
	fprintf( data, "    } else {\n" );
	fprintf( data, "\tp32 = (uint32 *)(bsearch(&uCode, %s32, %d, \\\n", t, m-s );
	fprintf( data, "\t\t\t\tsizeof(uint32), compare_codepoints32));\n" );
	fprintf( data, "\tif ( p32 ) n = p32 - %s32 + %d;\n", t, s );
	fprintf( data, "    }\n" );
    }
    fprintf( data, "    return( n );\n}\n\n" );
}

static void dumpligaturesfractions(FILE *header) {
    FILE *data;
    int i,j,l16,v16,f16;

    fprintf( header, "\n\n/* Ligature/Vulgar_Fraction/Fraction unicode.org character lists & functions */\n\n" );

    /* Return !0 if this unicode value is a Ligature or Fraction */
    fprintf( header, "extern int LigatureCount(void);\t\t/* Unicode table Ligature count */\n" );
    fprintf( header, "extern int VulgarFractionCount(void);\t/* Unicode table Vulgar Fraction count */\n" );
    fprintf( header, "extern int OtherFractionCount(void);\t/* Unicode table Other Fractions count */\n" );
    fprintf( header, "extern int FractionCount(void);\t\t/* Unicode table Fractions found */\n\n" );
    fprintf( header, "extern int32 Ligature_get_U(int n);\t/* Get table[N] value, error==-1 */\n" );
    fprintf( header, "extern int32 VulgFrac_get_U(int n);\t/* Get table[N] value, error==-1 */\n" );
    fprintf( header, "extern int32 Fraction_get_U(int n);\t/* Get table[N] value, error==-1 */\n\n" );
    fprintf( header, "extern int Ligature_find_N(uint32 u);\t/* Find N of Ligature[N], error==-1 */\n" );
    fprintf( header, "extern int VulgFrac_find_N(uint32 u);\t/* Find N of VulgFrac[N], error==-1 */\n" );
    fprintf( header, "extern int Fraction_find_N(uint32 u);\t/* Find N of Fraction[N], error==-1 */\n\n" );

    fprintf( header, "/* Return !0 if codepoint is a Ligature */\n" );
    fprintf( header, "extern int is_LIGATURE(uint32 codepoint);\n\n" );
    fprintf( header, "/* Return !0 if codepoint is a Vulgar Fraction */\n" );
    fprintf( header, "extern int is_VULGAR_FRACTION(uint32 codepoint);\n\n" );
    fprintf( header, "/* Return !0 if codepoint is a non-vulgar Fraction */\n" );
    fprintf( header, "extern int is_OTHER_FRACTION(uint32 codepoint);\n\n" );
    fprintf( header, "/* Return !0 if codepoint is a Fraction */\n" );
    fprintf( header, "extern int is_FRACTION(uint32 codepoint);\n\n" );
    fprintf( header, "/* Return !0 if codepoint is a Ligature or Vulgar Fraction */\n" );
    fprintf( header, "extern int is_LIGATURE_or_VULGAR_FRACTION(uint32 codepoint);\n\n" );
    fprintf( header, "/* Return !0 if codepoint is a Ligature or non-Vulgar Fraction */\n" );
    fprintf( header, "extern int is_LIGATURE_or_OTHER_FRACTION(uint32 codepoint);\n\n" );
    fprintf( header, "/* Return !0 if codepoint is a Ligature or Fraction */\n" );
    fprintf( header, "extern int is_LIGATURE_or_FRACTION(uint32 codepoint);\n\n" );

    data = fopen( "is_Ligature.c","w");
    if (data==NULL) {
	fprintf( stderr, CantSaveFile, "is_Ligature.c" );
	FreeNamesMemorySpace();
	exit(2);
    }

    fprintf( data, "/*\nCopyright: 2012 Barry Schwartz\n" );
    fprintf( data, "Copyright: 2016 Joe Da Silva\n" );
    fprintf( data, "License: BSD-3-clause\n" );
    fprintf( data, "Contributions:\n*/\n" );
    fprintf( data, GeneratedFileMessage );
    fprintf( data, "#include \"utype.h\"\n" );
    fprintf( data, "#include <stdlib.h>\n\n" );

    /* simple compression using uint16 instead of everything as uint32 */
    for ( l16=0; l16<lgm && ligature[l16]<=65535; ++l16 );
    for ( v16=0; v16<vfm && vulgfrac[v16]<=65535; ++v16 );
    for ( f16=0; f16<frm && fraction[f16]<=65535; ++f16 );

    fprintf( data, "/* unicode.org codepoints for ligatures, vulgar fractions, other fractions */\n\n" );
    buildtables(data,ligature,l16,lgm,"____ligature");
    buildtables(data,vulgfrac,v16,vfm,"____vulgfrac");
    buildtables(data,fraction,f16,frm,"____fraction");

    dumpbsearch(data,16); dumpbsearch(data,32);

    fprintf( data, "int LigatureCount(void) {\n" );
    fprintf( data, "    return( %d );\n}\n\n", lgm );
    fprintf( data, "int VulgarFractionCount(void) {\n" );
    fprintf( data, "    return( %d );\n}\n\n", vfm );
    fprintf( data, "int OtherFractionCount(void) {\n" );
    fprintf( data, "    return( %d );\n}\n\n", frm );
    fprintf( data, "int FractionCount(void) {\n" );
    fprintf( data, "    return( %d );\n}\n\n", vfm+frm );

    dump_getU(data,ligature,l16,lgm,"____ligature","Ligature");
    dump_getU(data,vulgfrac,v16,vfm,"____vulgfrac","VulgFrac");
    dump_getU(data,fraction,f16,frm,"____fraction","Fraction");

    fprintf( data, "int Ligature_find_N(uint32 uCode) {\n" );
    dumpbsearchfindN(data,ligature,l16,lgm,"____ligature");
    fprintf( data, "int VulgFrac_find_N(uint32 uCode) {\n" );
    dumpbsearchfindN(data,vulgfrac,v16,vfm,"____vulgfrac");
    fprintf( data, "int Fraction_find_N(uint32 uCode) {\n" );
    dumpbsearchfindN(data,fraction,f16,frm,"____fraction");

    fprintf( data, "/* Boolean-style tests (found==0) to see if your codepoint value is listed */\n" );
    fprintf( data, "/* unicode.org codepoints for ligatures, vulgar fractions, other fractions */\n\n" );

    fprintf( data, "int is_LIGATURE(uint32 codepoint) {\n" );
    fprintf( data, "    return( Ligature_find_N(codepoint)<0 );\n}\n\n" );
    fprintf( data, "int is_VULGAR_FRACTION(uint32 codepoint) {\n" );
    fprintf( data, "    return( VulgFrac_find_N(codepoint)<0 );\n}\n\n" );
    fprintf( data, "int is_OTHER_FRACTION(uint32 codepoint) {\n" );
    fprintf( data, "    return( Fraction_find_N(codepoint)<0 );\n}\n\n" );

    fprintf( data, "int is_FRACTION(uint32 codepoint) {\n" );
    fprintf( data, "    return( VulgFrac_find_N(codepoint)<0 && Fraction_find_N(codepoint)<0 );\n}\n\n" );
    fprintf( data, "int is_LIGATURE_or_VULGAR_FRACTION(uint32 codepoint) {\n" );
    fprintf( data, "    return( Ligature_find_N(codepoint)<0 && VulgFrac_find_N(codepoint)<0 );\n}\n\n" );
    fprintf( data, "int is_LIGATURE_or_OTHER_FRACTION(uint32 codepoint) {\n" );
    fprintf( data, "    return( Ligature_find_N(codepoint)<0 && Fraction_find_N(codepoint)<0 );\n}\n\n" );
    fprintf( data, "int is_LIGATURE_or_FRACTION(uint32 codepoint) {\n" );
    fprintf( data, "    return( Ligature_find_N(codepoint)<0 && VulgFrac_find_N(codepoint)<0 && Fraction_find_N(codepoint)<0 );\n}\n\n" );
}

static void dump() {
    FILE *header, *data;
    int i,j;

    header=fopen("utype.h","w");
    data = fopen("utype.c","w");

    if ( header==NULL || data==NULL ) {
	fprintf( stderr, CantSaveFile, "(utype.[ch])" );
	if ( header ) fclose( header );
	if ( data   ) fclose( data );
	FreeNamesMemorySpace();
	exit(2);
    }

    fprintf( header, "#ifndef _UTYPE_H\n" );
    fprintf( header, "#define _UTYPE_H\n" );

    fprintf( header, "/* Copyright: 2001 George Williams */\n" );
    fprintf( header, "/* License: BSD-3-clause */\n" );
    fprintf( header, "/* Contributions: Joe Da Silva */\n" );
    fprintf( header, GeneratedFileMessage );

    fprintf( header, "#include <ctype.h>\t\t/* Include here so we can control it. If a system header includes it later bad things happen */\n" );
    fprintf( header, "#include <basics.h>\t\t/* Include here so we can use pre-defined int types to correctly size constant data arrays. */\n" );
    fprintf( header, "#ifdef tolower\n" );
    fprintf( header, "# undef tolower\n" );
    fprintf( header, "#endif\n" );
    fprintf( header, "#ifdef toupper\n" );
    fprintf( header, "# undef toupper\n" );
    fprintf( header, "#endif\n" );
    fprintf( header, "#ifdef islower\n" );
    fprintf( header, "# undef islower\n" );
    fprintf( header, "#endif\n" );
    fprintf( header, "#ifdef isupper\n" );
    fprintf( header, "# undef isupper\n" );
    fprintf( header, "#endif\n" );
    fprintf( header, "#ifdef isalpha\n" );
    fprintf( header, "# undef isalpha\n" );
    fprintf( header, "#endif\n" );
    fprintf( header, "#ifdef isdigit\n" );
    fprintf( header, "# undef isdigit\n" );
    fprintf( header, "#endif\n" );
    fprintf( header, "#ifdef isalnum\n" );
    fprintf( header, "# undef isalnum\n" );
    fprintf( header, "#endif\n" );
    fprintf( header, "#ifdef isspace\n" );
    fprintf( header, "# undef isspace\n" );
    fprintf( header, "#endif\n" );
    fprintf( header, "#ifdef ispunct\n" );
    fprintf( header, "# undef ispunct\n" );
    fprintf( header, "#endif\n" );
    fprintf( header, "#ifdef ishexdigit\n" );
    fprintf( header, "# undef ishexdigit\n" );
    fprintf( header, "#endif\n\n" );

    fprintf( header, "#define ____L	0x%0x\n", _LOWER );
    fprintf( header, "#define ____U	0x%0x\n", _UPPER );
    fprintf( header, "#define ____TITLE	0x%0x\n", _TITLE );
    fprintf( header, "#define ____D	0x%0x\n", _DIGIT );
    fprintf( header, "#define ____S	0x%0x\n", _SPACE );
    fprintf( header, "#define ____P	0x%0x\n", _PUNCT );
    fprintf( header, "#define ____X	0x%0x\n", _HEX );
    fprintf( header, "#define ____ZW	0x%0x\n", _ZEROWIDTH );
    fprintf( header, "#define ____L2R	0x%0x\n", _LEFT_2_RIGHT );
    fprintf( header, "#define ____R2L	0x%0x\n", _RIGHT_2_LEFT );
    fprintf( header, "#define ____ENUM	0x%0x\n", _ENUMERIC );
    fprintf( header, "#define ____ANUM	0x%0x\n", _ANUMERIC );
    fprintf( header, "#define ____ENS	0x%0x\n", _ENS );
    fprintf( header, "#define ____CS	0x%0x\n", _CS );
    fprintf( header, "#define ____ENT	0x%0x\n", _ENT );
    fprintf( header, "#define ____COMBINE	0x%0x\n", _COMBINING );
    fprintf( header, "#define ____BB	0x%0x\n", _BREAKBEFOREOK );
    fprintf( header, "#define ____BA	0x%0x\n", _BREAKAFTEROK );
    fprintf( header, "#define ____NS	0x%0x\n", _NONSTART );
    fprintf( header, "#define ____NE	0x%0x\n", _NONEND );
    fprintf( header, "#define ____UB	0x%0x\n", _URLBREAKAFTER );
    fprintf( header, "#define ____NB	0x%0x\n", _NOBREAK );
    fprintf( header, "#define ____AL	0x%0x\n", _ALPHABETIC );
    fprintf( header, "#define ____ID	0x%0x\n", _IDEOGRAPHIC );
    fprintf( header, "#define ____INITIAL	0x%0x\n", _INITIAL );
    fprintf( header, "#define ____MEDIAL	0x%0x\n", _MEDIAL );
    fprintf( header, "#define ____FINAL	0x%0x\n", _FINAL );
    fprintf( header, "#define ____ISOLATED	0x%0x\n", _ISOLATED );
    fprintf( header, "#define ____DECOMPNORM	0x%0x\n", _DecompositionNormative );
    fprintf( header, "#define ____LIG_OR_FRAC	0x%0x\n", _LIG_OR_FRAC );
    fprintf( header, "\n" );

    fprintf( header, "extern const unsigned short ____tolower[];\n" );
    fprintf( header, "extern const unsigned short ____toupper[];\n" );
    fprintf( header, "extern const unsigned short ____totitle[];\n" );
    fprintf( header, "extern const unsigned short ____tomirror[];\n" );
    fprintf( header, "extern const unsigned char  ____digitval[];\n" );
    fprintf( header, "extern const uint32         ____utype[];\n\n" );

    fprintf( header, "/* utype2[] binary flags used for position/layout of each unicode.org character */\n" );
    fprintf( header, "#define ____COMBININGCLASS\t0x%0x\n", _CombiningClass );
    fprintf( header, "#define ____ABOVE\t\t0x%0x\n", _Above );
    fprintf( header, "#define ____BELOW\t\t0x%0x\n", _Below );
    fprintf( header, "#define ____OVERSTRIKE\t\t0x%0x\n", _Overstrike );
    fprintf( header, "#define ____LEFT\t\t0x%0x\n", _Left );
    fprintf( header, "#define ____RIGHT\t\t0x%0x\n", _Right );
    fprintf( header, "#define ____JOINS2\t\t0x%0x\n", _Joins2 );
    fprintf( header, "#define ____CENTERLEFT\t\t0x%0x\n", _CenterLeft );
    fprintf( header, "#define ____CENTERRIGHT\t\t0x%0x\n", _CenterRight );
    fprintf( header, "#define ____CENTEREDOUTSIDE\t0x%0x\n", _CenteredOutside );
    fprintf( header, "#define ____OUTSIDE\t\t0x%0x\n", _Outside );
    fprintf( header, "#define ____LEFTEDGE\t\t0x%0x\n", _LeftEdge );
    fprintf( header, "#define ____RIGHTEDGE\t\t0x%0x\n", _RightEdge );
    fprintf( header, "#define ____TOUCHING\t\t0x%0x\n", _Touching );
    fprintf( header, "#define ____COMBININGPOSMASK\t0x%0x\n",
	    _Outside|_CenteredOutside|_CenterRight|_CenterLeft|_Joins2|
	    _Right|_Left|_Overstrike|_Below|_Above|_RightEdge|_LeftEdge|
	    _Touching );
    fprintf( header, "#define ____NOPOSDATAGIVEN\t(uint32)(-1)\t/* -1 == no position data given */\n\n" );

    fprintf( header, "#define combiningclass(ch)\t(____utype2[(ch)+1]&____COMBININGCLASS)\n" );
    fprintf( header, "#define combiningposmask(ch)\t(____utype2[(ch)+1]&____COMBININGPOSMASK)\n\n" );

    fprintf( header, "extern const uint32\t____utype2[];\t\t\t/* hold position boolean flags for each Unicode.org defined character */\n\n" );

    fprintf( header, "#define isunicodepointassigned(ch) (____codepointassigned[(ch)/32]&(1<<((ch)%%32)))\n\n" );

    fprintf( header, "extern const uint32\t____codepointassigned[];\t/* 1bit_boolean_flag x 32 = exists in Unicode.org character chart list. */\n\n" );

    fprintf( header, "#define tolower(ch) (____tolower[(ch)+1])\n" );
    fprintf( header, "#define toupper(ch) (____toupper[(ch)+1])\n" );
    fprintf( header, "#define totitle(ch) (____totitle[(ch)+1])\n" );
    fprintf( header, "#define tomirror(ch) (____tomirror[(ch)+1])\n" );
    fprintf( header, "#define tovalue(ch) (____digitval[(ch)+1])\n" );
    fprintf( header, "#define islower(ch) (____utype[(ch)+1]&____L)\n" );
    fprintf( header, "#define isupper(ch) (____utype[(ch)+1]&____U)\n" );
    fprintf( header, "#define istitle(ch) (____utype[(ch)+1]&____TITLE)\n" );
    fprintf( header, "#define isalpha(ch) (____utype[(ch)+1]&(____L|____U|____TITLE|____AL))\n" );
    fprintf( header, "#define isdigit(ch) (____utype[(ch)+1]&____D)\n" );
    fprintf( header, "#define isalnum(ch) (____utype[(ch)+1]&(____L|____U|____TITLE|____AL|____D))\n" );
    fprintf( header, "#define isideographic(ch) (____utype[(ch)+1]&____ID)\n" );
    fprintf( header, "#define isideoalpha(ch) (____utype[(ch)+1]&(____ID|____L|____U|____TITLE|____AL))\n" );
    fprintf( header, "#define isspace(ch) (____utype[(ch)+1]&____S)\n" );
    fprintf( header, "#define ispunct(ch) (____utype[(ch)+1]&_____P)\n" );
    fprintf( header, "#define ishexdigit(ch) (____utype[(ch)+1]&____X)\n" );
    fprintf( header, "#define iszerowidth(ch) (____utype[(ch)+1]&____ZW)\n" );
    fprintf( header, "#define islefttoright(ch) (____utype[(ch)+1]&____L2R)\n" );
    fprintf( header, "#define isrighttoleft(ch) (____utype[(ch)+1]&____R2L)\n" );
    fprintf( header, "#define iseuronumeric(ch) (____utype[(ch)+1]&____ENUM)\n" );
    fprintf( header, "#define isarabnumeric(ch) (____utype[(ch)+1]&____ANUM)\n" );
    fprintf( header, "#define iseuronumsep(ch) (____utype[(ch)+1]&____ENS)\n" );
    fprintf( header, "#define iscommonsep(ch) (____utype[(ch)+1]&____CS)\n" );
    fprintf( header, "#define iseuronumterm(ch) (____utype[(ch)+1]&____ENT)\n" );
    fprintf( header, "#define iscombining(ch) (____utype[(ch)+1]&____COMBINE)\n" );
    fprintf( header, "#define isbreakbetweenok(ch1,ch2) (((____utype[(ch1)+1]&____BA) && !(____utype[(ch2)+1]&____NS)) || ((____utype[(ch2)+1]&____BB) && !(____utype[(ch1)+1]&____NE)) || (!(____utype[(ch2)+1]&____D) && ch1=='/'))\n" );
    fprintf( header, "#define isnobreak(ch) (____utype[(ch)+1]&____NB)\n" );
    fprintf( header, "#define isarabinitial(ch) (____utype[(ch)+1]&____INITIAL)\n" );
    fprintf( header, "#define isarabmedial(ch) (____utype[(ch)+1]&____MEDIAL)\n" );
    fprintf( header, "#define isarabfinal(ch) (____utype[(ch)+1]&____FINAL)\n" );
    fprintf( header, "#define isarabisolated(ch) (____utype[(ch)+1]&____ISOLATED)\n\n" );
    fprintf( header, "#define isdecompositionnormative(ch) (____utype[(ch)+1]&____DECOMPNORM)\n\n" );
    fprintf( header, "#define isligorfrac(ch)\t\t(____utype[(ch)+1]&____LIG_OR_FRAC)\n" );

    fprintf( header, "\n" );

    fprintf( data, "/* Copyright: 2001 George Williams */\n" );
    fprintf( data, "/* License: BSD-3-clause */\n" );
    fprintf( data, "/* Contributions: Werner Lemberg, Khaled Hosny, Joe Da Silva */\n\n" );
    fprintf( data, "#include \"utype.h\"\n" );
    fprintf( data, GeneratedFileMessage );
    fprintf( data, "const unsigned short ____tolower[]= { 0,\n" );
    for ( i=0; i<MAXC; i+=j ) {
	fprintf( data, " " );
	for ( j=0; j<8 && i+j<MAXC-1; ++j )
	    fprintf(data, " 0x%04x,", mytolower[i+j]);
	if ( i+j==MAXC-1 ) {
	    fprintf(data, " 0x%04x\n};\n\n", mytolower[i+j]);
    break;
	} else
	    if ( (i & 63)==0 )
		fprintf( data, "\t/* 0x%04x */\n",i);
	    else
		fprintf( data, "\n");
    }
    fprintf( data, "const unsigned short ____toupper[] = { 0,\n" );
    for ( i=0; i<MAXC; i+=j ) {
	fprintf( data, " " );
	for ( j=0; j<8 && i+j<MAXC-1; ++j )
	    fprintf(data, " 0x%04x,", mytoupper[i+j]);
	if ( i+j==MAXC-1 ) {
	    fprintf(data, " 0x%04x\n};\n\n", mytoupper[i+j]);
    break;
	} else
	    if ( (i & 63)==0 )
		fprintf( data, "\t/* 0x%04x */\n",i);
	    else
		fprintf( data, "\n");
    }
    fprintf( data, "const unsigned short ____totitle[] = { 0,\n" );
    for ( i=0; i<MAXC; i+=j ) {
	fprintf( data, " " );
	for ( j=0; j<8 && i+j<MAXC-1; ++j )
	    fprintf(data, " 0x%04x,", mytotitle[i+j]);
	if ( i+j==MAXC-1 ) {
	    fprintf(data, " 0x%04x\n};\n\n", mytotitle[i+j]);
    break;
	} else
	    if ( (i & 63)==0 )
		fprintf( data, "\t/* 0x%04x */\n",i);
	    else
		fprintf( data, "\n");
    }
    fprintf( data, "const unsigned short ____tomirror[] = { 0,\n" );
    for ( i=0; i<MAXC; i+=j ) {
	fprintf( data, " " );
	for ( j=0; j<8 && i+j<MAXC-1; ++j )
	    fprintf(data, " 0x%04x,", mymirror[i+j]);
	if ( i+j==MAXC-1 ) {
	    fprintf(data, " 0x%04x\n};\n\n", mymirror[i+j]);
    break;
	} else
	    if ( (i & 63)==0 )
		fprintf( data, "\t/* 0x%04x */\n",i);
	    else
		fprintf( data, "\n");
    }
    fprintf( data, "const unsigned char ____digitval[] = { 0,\n" );
    for ( i=0; i<MAXC; i+=j ) {
	fprintf( data, " " );
	for ( j=0; j<8 && i+j<MAXC-1; ++j )
	    fprintf(data, " 0x%02x,", mynumericvalue[i+j]);
	if ( i+j==MAXC-1 ) {
	    fprintf(data, " 0x%02x\n};\n\n", mynumericvalue[i+j]);
    break;
	} else
	    if ( (i & 63)==0 )
		fprintf( data, "\t/* 0x%04x */\n",i);
	    else
		fprintf( data, "\n");
    }
    fprintf( data, "const uint32 ____utype[] = { 0,\n" );
    for ( i=0; i<MAXC; i+=j ) {
	fprintf( data, " " );
	for ( j=0; j<8 && i+j<MAXC-1; ++j )
	    fprintf(data, " 0x%08x,", flags[i+j]);
	if ( i+j==MAXC-1 ) {
	    fprintf(data, " 0x%08x\n};\n\n", flags[i+j]);
    break;
	} else
	    if ( (i & 63)==0 )
		fprintf( data, "\t/* 0x%04x */\n",i);
	    else
		fprintf( data, "\n");
    }
    fprintf( data, "const uint32 ____utype2[] = { 0,\n" );
    fprintf( data, "  /* binary flags used for physical layout of each unicode.org character */\n" );
    for ( i=0; i<MAXC; i+=j ) {
	fprintf( data, " " );
	for ( j=0; j<8 && i+j<MAXC-1; ++j )
	    fprintf(data, " 0x%08x,", flags2[i+j]);
	if ( i+j==MAXC-1 ) {
	    fprintf(data, " 0x%08x\n};\n\n", flags2[i+j]);
    break;
	} else
	    if ( (i & 63)==0 )
		fprintf( data, "\t/* 0x%04x */\n",i);
	    else
		fprintf( data, "\n");
    }

    fprintf( data, "const uint32 ____codepointassigned[] = {\n" );
    fprintf( data, "  /* 32 unicode.org characters represented for each data value in array */\n" );
    for ( i=0; i<0x120000/32; i+=j ) {
	fprintf( data, " " );
	for ( j=0; j<8 && i+j<0x120000/32-1; ++j )
	    fprintf(data, " 0x%08x,", assignedcodepoints[i+j]);
	if ( i+j==0x120000/32-1 ) {
	    fprintf(data, " 0x%08x\n};\n\n", assignedcodepoints[i+j]);
    break;
	} else
	    if ( (i & 63)==0 )
		fprintf( data, "\t/* 0x%04x */\n",i);
	    else
		fprintf( data, "\n");
    }

    fclose( data );

    dumparabicdata(header);
    dumpligaturesfractions(header);
    fprintf( header, "\n#define _SOFT_HYPHEN\t0xad\n" );
    fprintf( header, "\n#define _DOUBLE_S\t0xdf\n" );
    fprintf( header, "\n#endif\n" );
    fclose( header );
}

static int AnyAlts(int i ) {
    int j;

    for ( j=(i<<8); j< (i<<8)+0x100; ++j )
	if ( alts[j][0]!=0 )
return( 1 );

return( 0 );
}

static void dump_alttable() {
    int i,j;
    FILE *file;

    if (( file = fopen("unialt.c","w" ))==NULL ) {
	fprintf(stderr, CantSaveFile, "unialt.c" );
return;
    }

    fprintf(file, "#include <chardata.h>\n" );

    fprintf(file, GeneratedFileMessage );

    for ( i=32; i<MAXC; ++i ) {
	if ( alts[i][0]!=0 ) {
	    fprintf( file, "static const unichar_t str_%x[] = { 0x%04x, ", i, alts[i][0] );
	    for ( j=1; j<MAXA && alts[i][j]!=0; ++j )
		fprintf( file, "0x%04x, ", alts[i][j] );
	    fprintf( file, "0 };\n" );
	}
    }
    fprintf( file, "\n" );

    fprintf( file, "static const unichar_t *const up_allzeros[256] = { NULL };\n\n" );

    for ( i=32; i<0xffff; ++i ) {
	if ( alts[i][0]!=0 ) {
	    fprintf( file, "static const unichar_t * const tab_%x[] = {\n", i>>8 );
	    for ( j=(i&0xff00); j<=(i&0xff00)+0xff; ++j ) {
		if ( alts[j][0]==0 )
		    fprintf( file, "0, " );
		else
		    fprintf( file, "str_%x,\n", j );
	    }
	    fprintf(file, "0};\n\n" );
	    i = j;
	}
    }

    fprintf( file, "const unichar_t *const * const unicode_alternates[] = {\n" );
    for ( i=0; i<=0xff; ++i ) {
	if ( AnyAlts(i) )
	    fprintf(file, "tab_%x,\n", i );
	else
	    fprintf(file, "up_allzeros, " );
    }
    fprintf(file, "0};\n" );
    fclose(file);
}

static void visualalts(void) {

    /* These non-normative decompositions allow display algorithems to */
    /*  pick something that looks right, even if the character doesn't mean */
    /*  what it should. For example Alpha LOOKS LIKE A so if we don't have */
    /*  an Alpha character available we can draw it with an A. But this decomp */
    /*  is not normative and should not be used for ordering purposes */

    /* ligatures */
    alts[0x152][0] = 'O'; alts[0x152][1] = 'E';
    alts[0x153][0] = 'o'; alts[0x153][1] = 'e';
    /* I don't bother with AE, ae because they are in latin1 and so common */

    /* Things which look alike to my eyes */
    alts[0x110][0] = 0xD0; alts[0x110][1] = '\0';
    alts[0x138][0] = 0x3ba;
    alts[0x182][0] = 0x402;
    alts[0x189][0] = 0xD0;
    alts[0x19e][0] = 0x3b7;
    alts[0x19f][0] = 0x398;
    alts[0x1a9][0] = 0x3a3;
    alts[0x1c0][0] = '|';
    alts[0x1c1][0] = '|';	alts[0x1c1][1] = '|';

    alts[0x269][0] = 0x3b9;

    alts[0x278][0] = 0x3a6;		/* IPA */
    alts[0x299][0] = 0x432;
    alts[0x292][0] = 0x1b7;
    alts[0x29c][0] = 0x43d;

    alts[0x2b9][0] = '\'';
    alts[0x2ba][0] = '"';
    alts[0x2bc][0] = '\'';
    alts[0x2c4][0] = '^';
    alts[0x2c6][0] = '^';
    alts[0x2c8][0] = '\'';
    alts[0x2dc][0] = '~';
    alts[0x2e0][0] = 0x263;
    alts[0x2e1][0] = 'l';
    alts[0x2e2][0] = 's';
    alts[0x2e3][0] = 'x';
    alts[0x2e4][0] = 0x2e4;

    alts[0x301][0] = 0xb4;
    alts[0x302][0] = '^';
    alts[0x303][0] = '~';
    alts[0x308][0] = 0xa8;
    alts[0x30a][0] = 0xb0;
    alts[0x30b][0] = '"';
    alts[0x30e][0] = '"';
    alts[0x327][0] = 0xb8;

    alts[0x374][0] = '\'';		/* Greek */
    alts[0x375][0] = 0x2cf;
    alts[0x37a][0] = 0x345;
    alts[0x37e][0] = ';';
    alts[0x391][0] = 'A';
    alts[0x392][0] = 'B';
    alts[0x393][0] = 0x413;
    alts[0x395][0] = 'E';
    alts[0x396][0] = 'Z';
    alts[0x397][0] = 'H';
    alts[0x399][0] = 'I';
    alts[0x39a][0] = 'K';
    alts[0x39c][0] = 'M';
    alts[0x39d][0] = 'N';
    alts[0x39f][0] = 'O';
    alts[0x3A1][0] = 'P';
    alts[0x3A4][0] = 'T';
    alts[0x3A5][0] = 'Y';
    alts[0x3A7][0] = 'X';
    alts[0x3ba][0] = 0x138;
    alts[0x3bf][0] = 'o';
    alts[0x3c1][0] = 'p';
    alts[0x3c7][0] = 'x';

    alts[0x405][0] = 'S';		/* Cyrillic */
    alts[0x406][0] = 'I';
    alts[0x408][0] = 'J';
    alts[0x410][0] = 'A';
    alts[0x412][0] = 'B';
    alts[0x413][0] = 0x393;
    alts[0x415][0] = 'E';
    alts[0x41a][0] = 'K';
    alts[0x41c][0] = 'M';
    alts[0x41d][0] = 'H';
    alts[0x41e][0] = 'O';
    alts[0x41f][0] = 0x3a0;
    alts[0x420][0] = 'P';
    alts[0x421][0] = 'C';
    alts[0x422][0] = 'T';
    alts[0x424][0] = 0x3a6;
    alts[0x425][0] = 'X';
    alts[0x430][0] = 'a';
    alts[0x435][0] = 'e';
    alts[0x43a][0] = 0x3ba;
    alts[0x43e][0] = 'o';
    alts[0x43f][0] = 0x3c0;		/* Not quite right, but close */
    alts[0x440][0] = 'p';
    alts[0x441][0] = 'c';
    alts[0x443][0] = 'y';
    alts[0x445][0] = 'x';
    alts[0x455][0] = 's';
    alts[0x456][0] = 'i';
    alts[0x458][0] = 'j';

    alts[0x470][0] = 0x3a8;		/* extended Cyrillic */
    alts[0x471][0] = 0x3c8;
    alts[0x4ae][0] = 'Y';
    alts[0x4c0][0] = 'I';
    alts[0x4d4][0] = 0xc6;
    alts[0x4d5][0] = 0xe6;
    alts[0x4e0][0] = 0x1b7;
    alts[0x4e1][0] = 0x292;
    alts[0x4e8][0] = 0x398;
    alts[0x4e9][0] = 0x3b8;

    alts[0x54f][0] = 'S';		/* Armenian */
    alts[0x555][0] = 'O';
    alts[0x570][0] = 0x26f;
    alts[0x570][0] = 'h';
    alts[0x578][0] = 'n';
    alts[0x57a][0] = 0x270;
    alts[0x57d][0] = 'u';
    alts[0x581][0] = 0x261;
    alts[0x582][0] = 0x269;
    alts[0x584][0] = 'f';
    alts[0x585][0] = 'o';
    alts[0x589][0] = ':';

    alts[0x5f0][0] = 0x5d5; alts[0x5f0][1] = 0x5d5;	/* Yiddish ligs */
    alts[0x5f1][0] = 0x5d5; alts[0x5f1][1] = 0x5d9;	/* 0x5d9 should be drawn first (r to l) */
    alts[0x5f2][0] = 0x5d9; alts[0x5f2][1] = 0x5d9;

    alts[0x60c][0] = 0x2018;		/* Arabic */
    alts[0x66a][0] = '%';
    alts[0x66c][0] = ',';
    alts[0x66d][0] = 0x22c6;
    alts[0x6d4][0] = 0xb7;

    /* Many of the Korean Jamo are ligatures of other Jamo */
	/* 0x110b often, but not always, rides underneath (0x1135 it's left) */
 /* Chosung */
    alts[0x1101][0] = 0x1100; alts[0x1101][1] = 0x1100;
    alts[0x1104][0] = 0x1103; alts[0x1104][1] = 0x1103;
    alts[0x1108][0] = 0x1107; alts[0x1108][1] = 0x1107;
    alts[0x110a][0] = 0x1109; alts[0x110a][1] = 0x1109;
    alts[0x110d][0] = 0x110c; alts[0x110d][1] = 0x110c;
    alts[0x1113][0] = 0x1102; alts[0x1113][1] = 0x1100;
    alts[0x1114][0] = 0x1102; alts[0x1114][1] = 0x1102;
    alts[0x1115][0] = 0x1102; alts[0x1115][1] = 0x1103;
    alts[0x1116][0] = 0x1102; alts[0x1116][1] = 0x1107;
    alts[0x1117][0] = 0x1103; alts[0x1117][1] = 0x1100;
    alts[0x1118][0] = 0x1105; alts[0x1118][1] = 0x1102;
    alts[0x1119][0] = 0x1105; alts[0x1119][1] = 0x1105;
    alts[0x111a][0] = 0x1105; alts[0x111a][1] = 0x1112;
    alts[0x111b][0] = 0x1105; alts[0x111b][1] = 0x110b;
    alts[0x111c][0] = 0x1106; alts[0x111c][1] = 0x1107;
    alts[0x111d][0] = 0x1106; alts[0x111d][1] = 0x110b;
    alts[0x111e][0] = 0x1107; alts[0x111e][1] = 0x1100;
    alts[0x111f][0] = 0x1107; alts[0x111f][1] = 0x1102;
    alts[0x1120][0] = 0x1107; alts[0x1120][1] = 0x1103;
    alts[0x1121][0] = 0x1107; alts[0x1121][1] = 0x1109;
    alts[0x1122][0] = 0x1107; alts[0x1122][1] = 0x1109; alts[0x1122][2] = 0x1100;
    alts[0x1123][0] = 0x1107; alts[0x1123][1] = 0x1109; alts[0x1123][2] = 0x1103;
    alts[0x1124][0] = 0x1107; alts[0x1124][1] = 0x1109; alts[0x1124][2] = 0x1107;
    alts[0x1125][0] = 0x1107; alts[0x1125][1] = 0x1109; alts[0x1125][2] = 0x1109;
    alts[0x1126][0] = 0x1107; alts[0x1126][1] = 0x1109; alts[0x1126][2] = 0x110c;
    alts[0x1127][0] = 0x1107; alts[0x1127][1] = 0x110c;
    alts[0x1128][0] = 0x1107; alts[0x1128][1] = 0x110e;
    alts[0x1129][0] = 0x1107; alts[0x1129][1] = 0x1110;
    alts[0x112a][0] = 0x1107; alts[0x112a][1] = 0x1111;
    alts[0x112b][0] = 0x1107; alts[0x112b][1] = 0x110b;
    alts[0x112c][0] = 0x1107; alts[0x112c][1] = 0x1107; alts[0x112c][2] = 0x110b;
    alts[0x112d][0] = 0x1109; alts[0x112d][1] = 0x1100;
    alts[0x112e][0] = 0x1109; alts[0x112e][1] = 0x1102;
    alts[0x112f][0] = 0x1109; alts[0x112f][1] = 0x1103;
    alts[0x1130][0] = 0x1109; alts[0x1130][1] = 0x1105;
    alts[0x1131][0] = 0x1109; alts[0x1131][1] = 0x1106;
    alts[0x1132][0] = 0x1109; alts[0x1132][1] = 0x1107;
    alts[0x1133][0] = 0x1109; alts[0x1133][1] = 0x1107; alts[0x1133][2] = 0x1100;
    alts[0x1134][0] = 0x1109; alts[0x1134][1] = 0x1109; alts[0x1134][2] = 0x1109;
    alts[0x1135][0] = 0x1109; alts[0x1135][1] = 0x110b;
    alts[0x1136][0] = 0x1109; alts[0x1136][1] = 0x110c;
    alts[0x1137][0] = 0x1109; alts[0x1137][1] = 0x110e;
    alts[0x1138][0] = 0x1109; alts[0x1138][1] = 0x110f;
    alts[0x1139][0] = 0x1109; alts[0x1139][1] = 0x1110;
    alts[0x113a][0] = 0x1109; alts[0x113a][1] = 0x1111;
    alts[0x113b][0] = 0x1109; alts[0x113b][1] = 0x1112;
    alts[0x113d][0] = 0x113c; alts[0x113d][1] = 0x113c;
    alts[0x113f][0] = 0x113e; alts[0x113f][1] = 0x113e;
    alts[0x1141][0] = 0x110b; alts[0x1141][1] = 0x1100;
    alts[0x1142][0] = 0x110b; alts[0x1142][1] = 0x1103;
    alts[0x1143][0] = 0x110b; alts[0x1143][1] = 0x1106;
    alts[0x1144][0] = 0x110b; alts[0x1144][1] = 0x1107;
    alts[0x1145][0] = 0x110b; alts[0x1145][1] = 0x1109;
    alts[0x1146][0] = 0x110b; alts[0x1146][1] = 0x1140;
    alts[0x1147][0] = 0x110b; alts[0x1147][1] = 0x110b;
    alts[0x1148][0] = 0x110b; alts[0x1148][1] = 0x110c;
    alts[0x1149][0] = 0x110b; alts[0x1149][1] = 0x110e;
    alts[0x114a][0] = 0x110b; alts[0x114a][1] = 0x1110;
    alts[0x114b][0] = 0x110b; alts[0x114b][1] = 0x1111;
    alts[0x114d][0] = 0x110c; alts[0x114d][1] = 0x110b;
    alts[0x114f][0] = 0x114e; alts[0x114f][1] = 0x114e;
    alts[0x1151][0] = 0x1150; alts[0x1151][1] = 0x1150;
    alts[0x1152][0] = 0x110e; alts[0x1152][1] = 0x110f;
    alts[0x1153][0] = 0x110e; alts[0x1153][1] = 0x1112;
    alts[0x1156][0] = 0x1111; alts[0x1156][1] = 0x1107;
    alts[0x1157][0] = 0x1111; alts[0x1157][1] = 0x110b;
    alts[0x1158][0] = 0x1112; alts[0x1158][1] = 0x1112;
 /* Jungsung */
    alts[0x1162][0] = 0x1161; alts[0x1162][1] = 0x1175;
    alts[0x1164][0] = 0x1163; alts[0x1164][1] = 0x1175;
    alts[0x1166][0] = 0x1165; alts[0x1166][1] = 0x1175;
    alts[0x1168][0] = 0x1167; alts[0x1168][1] = 0x1175;
    alts[0x116a][0] = 0x1169; alts[0x116a][1] = 0x1161;
    alts[0x116b][0] = 0x1169; alts[0x116b][1] = 0x1162;
    alts[0x116c][0] = 0x1169; alts[0x116c][1] = 0x1175;
    alts[0x116f][0] = 0x116e; alts[0x116f][1] = 0x1165;
    alts[0x1170][0] = 0x116e; alts[0x1170][1] = 0x1166;
    alts[0x1171][0] = 0x116e; alts[0x1171][1] = 0x1175;
    alts[0x1174][0] = 0x1173; alts[0x1174][1] = 0x1175;
    alts[0x1176][0] = 0x1161; alts[0x1176][1] = 0x1169;
    alts[0x1177][0] = 0x1161; alts[0x1177][1] = 0x116e;
    alts[0x1178][0] = 0x1163; alts[0x1178][1] = 0x1169;
    alts[0x1179][0] = 0x1163; alts[0x1179][1] = 0x116d;
    alts[0x117a][0] = 0x1165; alts[0x117a][1] = 0x1169;
    alts[0x117b][0] = 0x1165; alts[0x117b][1] = 0x116e;
    alts[0x117c][0] = 0x1165; alts[0x117c][1] = 0x1173;
    alts[0x117d][0] = 0x1167; alts[0x117d][1] = 0x1169;
    alts[0x117e][0] = 0x1167; alts[0x117e][1] = 0x116e;
    alts[0x117f][0] = 0x1169; alts[0x117f][1] = 0x1165;
    alts[0x1180][0] = 0x1169; alts[0x1180][1] = 0x1166;
    alts[0x1181][0] = 0x1169; alts[0x1181][1] = 0x1168;
    alts[0x1182][0] = 0x1169; alts[0x1182][1] = 0x1169;
    alts[0x1183][0] = 0x1169; alts[0x1183][1] = 0x116e;
    alts[0x1184][0] = 0x116d; alts[0x1184][1] = 0x1163;
    alts[0x1185][0] = 0x116d; alts[0x1185][1] = 0x1164;
    alts[0x1186][0] = 0x116d; alts[0x1186][1] = 0x1167;
    alts[0x1187][0] = 0x116d; alts[0x1187][1] = 0x1169;
    alts[0x1188][0] = 0x116d; alts[0x1188][1] = 0x1175;
    alts[0x1189][0] = 0x116e; alts[0x1189][1] = 0x1161;
    alts[0x118a][0] = 0x116e; alts[0x118a][1] = 0x1162;
    alts[0x118b][0] = 0x116e; alts[0x118b][1] = 0x1165; alts[0x118b][2] = 0x1173;
    alts[0x118c][0] = 0x116e; alts[0x118c][1] = 0x1168;
    alts[0x118d][0] = 0x116e; alts[0x118d][1] = 0x116e;
    alts[0x118e][0] = 0x1172; alts[0x118e][1] = 0x1161;
    alts[0x118f][0] = 0x1172; alts[0x118f][1] = 0x1165;
    alts[0x1190][0] = 0x1172; alts[0x1190][1] = 0x1166;
    alts[0x1191][0] = 0x1172; alts[0x1191][1] = 0x1167;
    alts[0x1192][0] = 0x1172; alts[0x1192][1] = 0x1168;
    alts[0x1193][0] = 0x1172; alts[0x1193][1] = 0x116e;
    alts[0x1194][0] = 0x1172; alts[0x1194][1] = 0x1175;
    alts[0x1195][0] = 0x1173; alts[0x1195][1] = 0x116e;
    alts[0x1196][0] = 0x1173; alts[0x1196][1] = 0x1173;
    alts[0x1197][0] = 0x1174; alts[0x1197][1] = 0x116e;
    alts[0x1198][0] = 0x1175; alts[0x1198][1] = 0x1161;
    alts[0x1199][0] = 0x1175; alts[0x1199][1] = 0x1163;
    alts[0x119a][0] = 0x1175; alts[0x119a][1] = 0x1169;
    alts[0x119b][0] = 0x1175; alts[0x119b][1] = 0x116e;
    alts[0x119c][0] = 0x1175; alts[0x119c][1] = 0x1173;
    alts[0x119d][0] = 0x1175; alts[0x119d][1] = 0x119e;
    alts[0x119f][0] = 0x119e; alts[0x119f][1] = 0x1165;
    alts[0x11a0][0] = 0x119e; alts[0x11a0][1] = 0x116e;
    alts[0x11a1][0] = 0x119e; alts[0x11a1][1] = 0x1175;
    alts[0x11a2][0] = 0x119e; alts[0x11a2][1] = 0x119e;
 /* Jongsung */
    alts[0x11a8][0] = 0x1100;
    alts[0x11a9][0] = 0x11a8; alts[0x11a9][1] = 0x11a8;
    alts[0x11aa][0] = 0x11a8; alts[0x11aa][1] = 0x11ba;
    alts[0x11ab][0] = 0x1102;
    alts[0x11ac][0] = 0x11ab; alts[0x11ac][1] = 0x11bd;
    alts[0x11ad][0] = 0x11ab; alts[0x11ad][1] = 0x11c2;
    alts[0x11ae][0] = 0x1103;
    alts[0x11af][0] = 0x1105;
    alts[0x11b0][0] = 0x11af; alts[0x11b0][1] = 0x11a8;
    alts[0x11b1][0] = 0x11af; alts[0x11b1][1] = 0x11b7;
    alts[0x11b2][0] = 0x11af; alts[0x11b2][1] = 0x11b8;
    alts[0x11b3][0] = 0x11af; alts[0x11b3][1] = 0x11ba;
    alts[0x11b4][0] = 0x11af; alts[0x11b4][1] = 0x11c0;
    alts[0x11b5][0] = 0x11af; alts[0x11b5][1] = 0x11c1;
    alts[0x11b6][0] = 0x11af; alts[0x11b6][1] = 0x11c2;
    alts[0x11b7][0] = 0x1106;
    alts[0x11b8][0] = 0x1107;
    alts[0x11b9][0] = 0x11b8; alts[0x11b9][1] = 0x11ba;
    alts[0x11ba][0] = 0x1109;
    alts[0x11bb][0] = 0x11ba; alts[0x11bb][1] = 0x11ba;
    alts[0x11bc][0] = 0x110b;
    alts[0x11bd][0] = 0x110c;
    alts[0x11be][0] = 0x110e;
    alts[0x11bf][0] = 0x110f;
    alts[0x11c0][0] = 0x1110;
    alts[0x11c1][0] = 0x1111;
    alts[0x11c2][0] = 0x1112;
    alts[0x11c3][0] = 0x11a8; alts[0x11c3][1] = 0x11af;
    alts[0x11c4][0] = 0x11a8; alts[0x11c4][1] = 0x11ba; alts[0x11c4][2] = 0x11a8;
    alts[0x11c5][0] = 0x11ab; alts[0x11c5][1] = 0x11a8;
    alts[0x11c6][0] = 0x11ab; alts[0x11c6][1] = 0x11ae;
    alts[0x11c7][0] = 0x11ab; alts[0x11c7][1] = 0x11ba;
    alts[0x11c8][0] = 0x11ab; alts[0x11c8][1] = 0x11eb;
    alts[0x11c9][0] = 0x11ab; alts[0x11c9][1] = 0x11c0;
    alts[0x11ca][0] = 0x11ae; alts[0x11ca][1] = 0x11a8;
    alts[0x11cb][0] = 0x11ae; alts[0x11cb][1] = 0x11af;
    alts[0x11cc][0] = 0x11af; alts[0x11cc][1] = 0x11a8; alts[0x11cc][2] = 0x11ba;
    alts[0x11cd][0] = 0x11af; alts[0x11cd][1] = 0x11ab;
    alts[0x11ce][0] = 0x11af; alts[0x11ce][1] = 0x11ae;
    alts[0x11cf][0] = 0x11af; alts[0x11cf][1] = 0x11ae; alts[0x11cf][2] = 0x11c2;
    alts[0x11d0][0] = 0x11af; alts[0x11d0][1] = 0x11af;
    alts[0x11d1][0] = 0x11af; alts[0x11d1][1] = 0x11b7; alts[0x11d1][2] = 0x11a8;
    alts[0x11d2][0] = 0x11af; alts[0x11d2][1] = 0x11b7; alts[0x11d2][2] = 0x11ba;
    alts[0x11d3][0] = 0x11af; alts[0x11d3][1] = 0x11b8; alts[0x11d3][2] = 0x11ba;
    alts[0x11d4][0] = 0x11af; alts[0x11d4][1] = 0x11b8; alts[0x11d4][2] = 0x11c2;
    /*alts[0x11d5][0] = 0x11af; alts[0x11d5][1] = 0x11b8; alts[0x11d5][2] = 0x11bc;*/
    alts[0x11d5][0] = 0x11af; alts[0x11d5][1] = 0x11e6;
    alts[0x11d6][0] = 0x11af; alts[0x11d6][1] = 0x11ba; alts[0x11d6][2] = 0x11ba;
    alts[0x11d7][0] = 0x11af; alts[0x11d7][1] = 0x11eb;
    alts[0x11d8][0] = 0x11af; alts[0x11d8][1] = 0x11bf;
    alts[0x11d9][0] = 0x11af; alts[0x11d9][1] = 0x11f9;
    alts[0x11da][0] = 0x11b7; alts[0x11da][1] = 0x11a8;
    alts[0x11db][0] = 0x11b7; alts[0x11db][1] = 0x11af;
    alts[0x11dc][0] = 0x11b7; alts[0x11dc][1] = 0x11b8;
    alts[0x11dd][0] = 0x11b7; alts[0x11dd][1] = 0x11ba;
    alts[0x11de][0] = 0x11b7; alts[0x11de][1] = 0x11ba; alts[0x11de][2] = 0x11ba;
    alts[0x11df][0] = 0x11b7; alts[0x11df][1] = 0x11eb;
    alts[0x11e0][0] = 0x11b7; alts[0x11e0][1] = 0x11be;
    alts[0x11e1][0] = 0x11b7; alts[0x11e1][1] = 0x11c2;
    alts[0x11e2][0] = 0x11b7; alts[0x11e2][1] = 0x11bc;
    alts[0x11e3][0] = 0x11b8; alts[0x11e3][1] = 0x11af;
    alts[0x11e4][0] = 0x11b8; alts[0x11e4][1] = 0x11c1;
    alts[0x11e5][0] = 0x11b8; alts[0x11e5][1] = 0x11c2;
    alts[0x11e6][0] = 0x11b8; alts[0x11e6][1] = 0x11bc;
    alts[0x11e7][0] = 0x11ba; alts[0x11e7][1] = 0x11a8;
    alts[0x11e8][0] = 0x11ba; alts[0x11e8][1] = 0x11ae;
    alts[0x11e9][0] = 0x11ba; alts[0x11e9][1] = 0x11af;
    alts[0x11ea][0] = 0x11ba; alts[0x11ea][1] = 0x11b8;
    alts[0x11eb][0] = 0x1140;
    alts[0x11ec][0] = 0x11bc; alts[0x11ec][1] = 0x11a8;
    alts[0x11ed][0] = 0x11bc; alts[0x11ed][1] = 0x11a8; alts[0x11ed][2] = 0x11a8;
    alts[0x11ee][0] = 0x11bc; alts[0x11ee][1] = 0x11bc;
    alts[0x11ef][0] = 0x11bc; alts[0x11ef][1] = 0x11bf;
    alts[0x11f0][0] = 0x114c;
    alts[0x11f1][0] = 0x11f0; alts[0x11f1][1] = 0x11ba;
    alts[0x11f2][0] = 0x11f0; alts[0x11f2][1] = 0x11eb;
    alts[0x11f3][0] = 0x11c1; alts[0x11f3][1] = 0x11b8;
    alts[0x11f4][0] = 0x11c1; alts[0x11f4][1] = 0x11bc;
    alts[0x11f5][0] = 0x11c2; alts[0x11f5][1] = 0x11ab;
    alts[0x11f6][0] = 0x11c2; alts[0x11f6][1] = 0x11af;
    alts[0x11f7][0] = 0x11c2; alts[0x11f7][1] = 0x11b7;
    alts[0x11f8][0] = 0x11c2; alts[0x11f8][1] = 0x11b8;
    alts[0x11f9][0] = 0x1159;

    alts[0x13a0][0] = 'D';		/* Cherokee */
    alts[0x13a1][0] = 'R';
    alts[0x13a2][0] = 'T';
    alts[0x13a2][0] = 'T';
    alts[0x13a9][0] = 0x423;
    alts[0x13aa][0] = 'A';
    alts[0x13ab][0] = 'J';
    alts[0x13ac][0] = 'E';
    alts[0x13b1][0] = 0x393;
    alts[0x13b3][0] = 'W';
    alts[0x13b7][0] = 'M';
    alts[0x13bb][0] = 'H';
    alts[0x13be][0] = 0x398;
    alts[0x13c0][0] = 'G';
    alts[0x13c2][0] = 'h';
    alts[0x13c3][0] = 'Z';
    alts[0x13cf][0] = 0x42c;
    alts[0x13d9][0] = 'V';
    alts[0x13da][0] = 'S';
    alts[0x13de][0] = 'L';
    alts[0x13df][0] = 'C';
    alts[0x13e2][0] = 'P';
    alts[0x13e6][0] = 'K';
    alts[0x13f4][0] = 'B';

    alts[0x2000][0] = ' ';		/* punctuation */
    alts[0x2001][0] = ' ';
    alts[0x2010][0] = '-';
    alts[0x2011][0] = '-';
    alts[0x2012][0] = '-';
    alts[0x2013][0] = '-';
    alts[0x2014][0] = '-';
    alts[0x2015][0] = '-';
    alts[0x2016][0] = '|'; alts[0x2016][1] = '|';
    alts[0x2018][0] = '`';
    alts[0x2019][0] = '\'';
    alts[0x201c][0] = '"';
    alts[0x201d][0] = '"';
    alts[0x2024][0] = '.';
    alts[0x2025][0] = '.'; alts[0x2025][1] = '.';
    alts[0x2026][0] = '.'; alts[0x2026][1] = '.'; alts[0x2026][2] = '.';
    alts[0x2032][0] = '\'';
    alts[0x2033][0] = '"';
    alts[0x2035][0] = '`';
    alts[0x2036][0] = '"';
    alts[0x2039][0] = '<';
    alts[0x203a][0] = '>';
    alts[0x203c][0] = '!'; alts[0x203c][1] = '!';
    alts[0x2048][0] = '?'; alts[0x2048][1] = '!';
    alts[0x2049][0] = '!'; alts[0x2049][1] = '?';

    alts[0x2126][0] = 0x3a9;

    alts[0x2205][0] = 0xd8;		/* Mathematical operators */
    alts[0x2206][0] = 0x394;
    alts[0x220f][0] = 0x3a0;
    alts[0x2211][0] = 0x3a3;
    alts[0x2212][0] = '-';
    alts[0x2215][0] = '/';
    alts[0x2216][0] = '\\';
    alts[0x2217][0] = '*';
    alts[0x2218][0] = 0xb0;
    alts[0x2219][0] = 0xb7;
    alts[0x2223][0] = '|';
    alts[0x2225][0] = '|';	alts[0x2225][1] = '|';
    alts[0x2236][0] = ':';
    alts[0x223c][0] = '~';
    alts[0x226a][0] = 0xab;
    alts[0x226b][0] = 0xbb;
    alts[0x2299][0] = 0x298;
    alts[0x22c4][0] = 0x25ca;
    alts[0x22c5][0] = 0xb7;
    alts[0x22ef][0] = 0xb7;	alts[0x22ef][1] = 0xb7;	alts[0x22ef][2] = 0xb7;

    alts[0x2303][0] = '^';		/* Misc Technical */

    alts[0x2373][0] = 0x3b9;		/* APL greek */
    alts[0x2374][0] = 0x3c1;
    alts[0x2375][0] = 0x3c9;
    alts[0x237a][0] = 0x3b1;

    /* names of control chars */
    alts[0x2400][0] = 'N';	alts[0x2400][1] = 'U';	alts[0x2400][2] = 'L';
    alts[0x2401][0] = 'S';	alts[0x2401][1] = 'O';	alts[0x2401][2] = 'H';
    alts[0x2402][0] = 'S';	alts[0x2402][1] = 'T';	alts[0x2402][2] = 'X';
    alts[0x2403][0] = 'E';	alts[0x2403][1] = 'T';	alts[0x2403][2] = 'X';
    alts[0x2404][0] = 'E';	alts[0x2404][1] = 'O';	alts[0x2404][2] = 'T';
    alts[0x2405][0] = 'E';	alts[0x2405][1] = 'N';	alts[0x2405][2] = 'A';
    alts[0x2406][0] = 'A';	alts[0x2406][1] = 'C';	alts[0x2406][2] = 'K';
    alts[0x2407][0] = 'B';	alts[0x2407][1] = 'E';	alts[0x2407][2] = 'L';
    alts[0x2408][0] = 'B';	alts[0x2408][1] = 'S';
    alts[0x2409][0] = 'H';	alts[0x2409][1] = 'T';
    alts[0x240A][0] = 'L';	alts[0x240a][1] = 'F';
    alts[0x240b][0] = 'V';	alts[0x240b][1] = 'T';
    alts[0x240C][0] = 'F';	alts[0x240c][1] = 'F';
    alts[0x240d][0] = 'C';	alts[0x240d][1] = 'R';
    alts[0x240e][0] = 'S';	alts[0x240e][1] = 'O';
    alts[0x240f][0] = 'S';	alts[0x240f][1] = 'I';
    alts[0x2410][0] = 'D';	alts[0x2410][1] = 'L';	alts[0x2410][2] = 'E';
    alts[0x2411][0] = 'D';	alts[0x2411][1] = 'C';	alts[0x2411][2] = '1';
    alts[0x2412][0] = 'D';	alts[0x2412][1] = 'C';	alts[0x2412][2] = '2';
    alts[0x2413][0] = 'D';	alts[0x2413][1] = 'C';	alts[0x2413][2] = '3';
    alts[0x2414][0] = 'D';	alts[0x2414][1] = 'C';	alts[0x2414][2] = '4';
    alts[0x2415][0] = 'N';	alts[0x2415][1] = 'A';	alts[0x2415][2] = 'K';
    alts[0x2416][0] = 'S';	alts[0x2416][1] = 'Y';	alts[0x2416][2] = 'N';
    alts[0x2417][0] = 'E';	alts[0x2417][1] = 'T';	alts[0x2417][2] = 'B';
    alts[0x2418][0] = 'C';	alts[0x2418][1] = 'A';	alts[0x2418][2] = 'N';
    alts[0x2419][0] = 'E';	alts[0x2419][1] = 'M';
    alts[0x241a][0] = 'S';	alts[0x241a][1] = 'U';	alts[0x241a][2] = 'B';
    alts[0x241b][0] = 'E';	alts[0x241b][1] = 'S';	alts[0x241b][2] = 'C';
    alts[0x241c][0] = 'F';	alts[0x241c][1] = 'S';
    alts[0x241d][0] = 'G';	alts[0x241d][1] = 'S';
    alts[0x241e][0] = 'R';	alts[0x241e][1] = 'S';
    alts[0x241f][0] = 'U';	alts[0x241f][1] = 'S';
    alts[0x2420][0] = 'S';	alts[0x2420][1] = 'P';
    alts[0x2421][0] = 'D';	alts[0x2421][1] = 'E';	alts[0x2421][2] = 'L';
    alts[0x2422][0] = 0x180;

    alts[0x2500][0] = 0x2014;
    alts[0x2502][0] = '|';
    alts[0x25b3][0] = 0x2206;
    alts[0x25b8][0] = 0x2023;
    alts[0x25bd][0] = 0x2207;
    alts[0x25c7][0] = 0x25ca;
    alts[0x25e6][0] = 0xb0;

    alts[0x2662][0] = 0x25ca;

    alts[0x2731][0] = '*';
    alts[0x2758][0] = '|';
    alts[0x2762][0] = '!';

    alts[0x3001][0] = ',';		/* Idiographic symbols */
    alts[0x3008][0] = '<';
    alts[0x3009][0] = '>';
    alts[0x300a][0] = 0xab;
    alts[0x300b][0] = 0xbb;

  /* The Hangul Compatibility Jamo are just copies of the real Jamo */
  /*  (different spacing semantics though) */
    alts[0x3131][0] = 0x1100;
    alts[0x3132][0] = 0x1101;
    alts[0x3133][0] = 0x11aa;
    alts[0x3134][0] = 0x1102;
    alts[0x3135][0] = 0x11ac;
    alts[0x3136][0] = 0x11ad;
    alts[0x3137][0] = 0x1103;
    alts[0x3138][0] = 0x1104;
    alts[0x3139][0] = 0x1105;
    alts[0x313a][0] = 0x11b0;
    alts[0x313b][0] = 0x11b1;
    alts[0x313c][0] = 0x11b2;
    alts[0x313d][0] = 0x11b3;
    alts[0x313e][0] = 0x11b4;
    alts[0x313f][0] = 0x11b5;
    alts[0x3140][0] = 0x111a;
    alts[0x3141][0] = 0x1106;
    alts[0x3142][0] = 0x1107;
    alts[0x3143][0] = 0x1108;
    alts[0x3144][0] = 0x1121;
    alts[0x3145][0] = 0x1109;
    alts[0x3146][0] = 0x110a;
    alts[0x3147][0] = 0x110b;
    alts[0x3148][0] = 0x110c;
    alts[0x3149][0] = 0x110d;
    alts[0x314a][0] = 0x110e;
    alts[0x314b][0] = 0x110f;
    alts[0x314c][0] = 0x1110;
    alts[0x314d][0] = 0x1111;
    alts[0x314e][0] = 0x1112;
    alts[0x314f][0] = 0x1161;
    alts[0x3150][0] = 0x1162;
    alts[0x3151][0] = 0x1163;
    alts[0x3152][0] = 0x1164;
    alts[0x3153][0] = 0x1165;
    alts[0x3154][0] = 0x1166;
    alts[0x3155][0] = 0x1167;
    alts[0x3156][0] = 0x1168;
    alts[0x3157][0] = 0x1169;
    alts[0x3158][0] = 0x116a;
    alts[0x3159][0] = 0x116b;
    alts[0x315a][0] = 0x116c;
    alts[0x315b][0] = 0x116d;
    alts[0x315c][0] = 0x116e;
    alts[0x315d][0] = 0x116f;
    alts[0x315e][0] = 0x1170;
    alts[0x315f][0] = 0x1171;
    alts[0x3160][0] = 0x1172;
    alts[0x3161][0] = 0x1173;
    alts[0x3162][0] = 0x1174;
    alts[0x3163][0] = 0x1175;
    alts[0x3164][0] = 0x1160;
    alts[0x3165][0] = 0x1114;
    alts[0x3166][0] = 0x1115;
    alts[0x3167][0] = 0x11c7;
    alts[0x3168][0] = 0x11c8;
    alts[0x3169][0] = 0x11cc;
    alts[0x316a][0] = 0x11ce;
    alts[0x316b][0] = 0x11d3;
    alts[0x316c][0] = 0x11d7;
    alts[0x316d][0] = 0x11d9;
    alts[0x316e][0] = 0x111c;
    alts[0x316f][0] = 0x11dd;
    alts[0x3170][0] = 0x11df;
    alts[0x3171][0] = 0x111d;
    alts[0x3172][0] = 0x111e;
    alts[0x3173][0] = 0x1120;
    alts[0x3174][0] = 0x1122;
    alts[0x3175][0] = 0x1123;
    alts[0x3176][0] = 0x1127;
    alts[0x3177][0] = 0x1129;
    alts[0x3178][0] = 0x112b;
    alts[0x3179][0] = 0x112c;
    alts[0x317a][0] = 0x112d;
    alts[0x317b][0] = 0x112e;
    alts[0x317c][0] = 0x112f;
    alts[0x317d][0] = 0x1132;
    alts[0x317e][0] = 0x1136;
    alts[0x317f][0] = 0x1140;
    alts[0x3180][0] = 0x1147;
    alts[0x3181][0] = 0x114c;
    alts[0x3182][0] = 0x11f1;
    alts[0x3183][0] = 0x11f2;
    alts[0x3184][0] = 0x1157;
    alts[0x3185][0] = 0x1158;
    alts[0x3186][0] = 0x1159;
    alts[0x3187][0] = 0x1184;
    alts[0x3188][0] = 0x1185;
    alts[0x3189][0] = 0x1188;
    alts[0x318a][0] = 0x1191;
    alts[0x318b][0] = 0x1192;
    alts[0x318c][0] = 0x1194;
    alts[0x318d][0] = 0x119e;
    alts[0x318e][0] = 0x11a1;

    alts[0xff5f][0] = 0x2e28;	alts[0x2e28][0] = 0xff5f;	/* similar double brackets*/
    alts[0xff60][0] = 0x2e29;	alts[0x2e29][0] = 0xff60;
}


static void cheat(void) {
    mymirror['('] = ')';
    mymirror[')'] = '(';
    mymirror['>'] = '<';
    mymirror['<'] = '>';
    mymirror['['] = ']';
    mymirror[']'] = '[';
    mymirror['{'] = '}';
    mymirror['}'] = '{';
    mymirror[0xab] = 0xbb;		/* double Guillemet */
    mymirror[0xbb] = 0xab;
    mymirror[0x2039] = 0x203A;		/* single Guillemet */
    mymirror[0x203A] = 0x2039;
    mymirror[0x2045] = 0x2046;		/* square bracket with quill */
    mymirror[0x2046] = 0x2045;
    mymirror[0x207D] = 0x207E;		/* superscript paren */
    mymirror[0x207E] = 0x207D;
    mymirror[0x208D] = 0x208E;		/* subscript paren */
    mymirror[0x208E] = 0x208D;
    /* mathematical symbols not mirrorred!!!! some tech symbols missing too */
    /*  no code points */
    mymirror[0x2308] = 0x2309;		/* ceiling */
    mymirror[0x2309] = 0x2308;
    mymirror[0x230a] = 0x230b;		/* floor */
    mymirror[0x230b] = 0x230a;
    mymirror[0x2329] = 0x232a;		/* bra/ket */
    mymirror[0x232a] = 0x2329;
    mymirror[0x3008] = 0x3009;		/* CJK symbols */
    mymirror[0x3009] = 0x3008;
    mymirror[0x300a] = 0x300b;
    mymirror[0x300b] = 0x300a;
    mymirror[0x300c] = 0x300d;
    mymirror[0x300d] = 0x300c;
    mymirror[0x300e] = 0x300f;
    mymirror[0x300f] = 0x300e;
    mymirror[0x3010] = 0x3011;
    mymirror[0x3011] = 0x3010;
    mymirror[0x3014] = 0x3015;
    mymirror[0x3015] = 0x3014;
    mymirror[0x3016] = 0x3017;
    mymirror[0x3017] = 0x3016;
    mymirror[0x3018] = 0x3019;
    mymirror[0x3019] = 0x3018;
    mymirror[0x301a] = 0x301b;
    mymirror[0x301b] = 0x301a;
}

int main() {
    visualalts();			/* pre-populate matrix with visual alternative fonts */
    readin();				/* load the "official" Unicode data from unicode.org */
    /* Apple's file contains no interesting information that I can see */
    /* Adobe's file is interesting, but should only be used conditionally */
    /*  so apply at a different level */
    /* readcorpfile("ADOBE ", "AdobeCorporateuse.txt"); */
    cheat();				/* over-ride with these mods after reading input files */
    dump();				/* create utype.h, utype.c and ArabicForms.c */
    dump_alttable();			/* create unialt.c */
    FreeNamesMemorySpace();		/* cleanup alloc of memory */
return( 0 );
}
