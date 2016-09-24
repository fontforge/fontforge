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
 * To generate the latest files, you will first need to go and get these files
 * (see below in "alphabets[]) and put them in the Unicode subdirectory:
 * wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-1.TXT
 * wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-2.TXT
 * wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-3.TXT
 * wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-4.TXT
 * wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-5.TXT
 * wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-6.TXT
 * wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-7.TXT
 * wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-8.TXT
 * wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-9.TXT
 * wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-10.TXT
 * wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-11.TXT
 * wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-13.TXT
 * wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-14.TXT
 * wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-15.TXT
 * wget http://unicode.org/Public/MAPPINGS/ISO8859/8859-16.TXT
 * wget http://unicode.org/Public/MAPPINGS/VENDORS/MISC/KOI8-R.TXT
 * mv KOI8-R.TXT koi8r.TXT
 * wget http://unicode.org/Public/MAPPINGS/OBSOLETE/EASTASIA/JIS/JIS0201.TXT
 * mv JIS0201.TXT JIS0201.txt
 * wget http://unicode.org/Public/MAPPINGS/VENDORS/ADOBE/zdingbat.txt
 * mv zdingbat.txt zapfding.TXT
 * wget http://unicode.org/Public/MAPPINGS/OBSOLETE/EASTASIA/JIS/JIS0212.TXT
 */

#include <fontforge-config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <charset.h>
#include <basics.h>

char *alphabets[] = { "8859-1.TXT", "8859-2.TXT", "8859-3.TXT", "8859-4.TXT",
    "8859-5.TXT", "8859-6.TXT", "8859-7.TXT", "8859-8.TXT", "8859-9.TXT",
    "8859-10.TXT", "8859-11.TXT", "8859-13.TXT", "8859-14.TXT", "8859-15.TXT",
    "8859-16.TXT", "koi8r.TXT", "JIS0201.txt", "WIN.TXT", "MacRoman.TXT",
    "MacSYMBOL.TXT", "zapfding.TXT", /*"MacCYRILLIC.TXT",*/ NULL };
char *alnames[] = { "i8859_1", "i8859_2", "i8859_3", "i8859_4",
    "i8859_5", "i8859_6", "i8859_7", "i8859_8", "i8859_9",
    "i8859_10", "i8859_11", "i8859_13", "i8859_14", "i8859_15",
    "i8859_16", "koi8_r", "jis201", "win", "mac",
    "MacSymbol", "ZapfDingbats", /*"MacCyrillic",*/ NULL };
int almaps[] = { em_iso8859_1, em_iso8859_2, em_iso8859_3, em_iso8859_4,
    em_iso8859_5, em_iso8859_6, em_iso8859_7, em_iso8859_8, em_iso8859_9,
    em_iso8859_10, em_iso8859_11, em_iso8859_13, em_iso8859_14, em_iso8859_15,
    em_iso8859_16, em_koi8_r, em_jis201, em_win, em_mac, em_symbol, em_zapfding,
    -1 };


char *cjk[] = { "JIS0208.TXT", "JIS0212.TXT", "BIG5.TXT", "GB2312.TXT",
	"HANGUL.TXT", "Big5HKSCS.txt", NULL };
/* I'm only paying attention to Wansung encoding (in HANGUL.TXT) which is 94x94 */
/* I used to look at OLD5601, but that maps to Unicode 1.0, and Hangul's moved */
char *adobecjk[] = { "aj16cid2code.txt", "aj20cid2code.txt", "ac15cid2code.txt",
	"ag15cid2code.txt", "ak12cid2code.txt", NULL };
/* I'm told that most of the mappings provided on the Unicode site go to */
/*  Unicode 1.* and that CJK have been moved radically since. So instead */
/*  of the unicode site's files, try using Adobe's which claim they are */
/*  up to date. These may be found in: */
/*	ftp://ftp.ora.com/pub/examples/nutshell/ujip/adobe/samples/{aj14,aj20,ak12,ac13,ag14}/cid2code.txt */
/* they may be bundled up in a tar file, I forget exactly... */
char *cjknames[] = { "jis208", "jis212", "big5", "gb2312", "ksc5601", "big5hkscs", NULL };
int cjkmaps[] = { em_jis208, em_jis212, em_big5, em_gb2312, em_ksc5601, em_big5hkscs };

unsigned long *used[256];

const char GeneratedFileMessage[] = "/* This file was generated using the program 'dump' */\n\n";
const char CantReadFile[] = "Can't find or read file %s\n";		/* exit(2) */
const char CantSaveFile[] = "Can't open or write to output file %s\n";	/* exit(1) */
const char NoMoreMemory[] = "Can't access more memory.\n";		/* exit(3) */
const char LineLengthBg[] = "Error with %s. Found line too long: %s\n";	/* exit(4) */

static add_data_comment_at_EOL(FILE *output, int counter) {
/* append an EOL index locator marker to make it easier to search for table values */
    if ( (counter & 63)==0 )
	fprintf( output, "\t/* 0x%04x */\n",counter);
    else
	fprintf( output, "\n");
}

static int dumpalphas(FILE *output, FILE *header) {
    FILE *file;
    int i,j,k, l, first, last;
    long _orig, _unicode, mask;
    unichar_t unicode[256];
    unsigned char *table[256], *plane;
    char buffer[200+1];

    fprintf(output, "#include <chardata.h>\n\n" );
    fprintf(output, "const unsigned char c_allzeros[256] = { 0 };\n\n" );

    buffer[200]='\0'; l=0;
    for ( k=0; k<256; ++k ) table[k] = NULL;

    for ( j=0; j<256 && alphabets[j]!=NULL; ++j ) {
	file = fopen( alphabets[j], "r" );
	if ( file==NULL ) {
	    fprintf( stderr, CantReadFile, alphabets[j]);
	    l = 1;
	} else {
	    for ( i=0; i<160; ++i )
		unicode[i] = i;
	    for ( ; i<256; ++i )
		unicode[i] = 0;
	    while ( fgets(buffer,sizeof(buffer)-1,file)!=NULL ) {
		if (strlen(buffer)>=199) {
		    fprintf( stderr, LineLengthBg,alphabets[j],buffer );
		    fclose(file);
		    for ( k=0; k<256; ++k ) {
                        free(table[k]);
                        free(used[k]);
		    }
return( 4 );
		}
		if ( buffer[0]=='#' )
	    continue;
		sscanf(buffer, "0x%lx 0x%lx", (unsigned long *) &_orig, (unsigned long *) &_unicode);
		unicode[_orig] = _unicode;
		if ( table[_unicode>>8]==NULL ) {
		    if ((plane = table[_unicode>>8] = calloc(256,1))==NULL) {
			fprintf( stderr, NoMoreMemory );
			fclose(file);
			for ( k=0; k<256; ++k ) {
                            free(table[k]);
                            free(used[k]);
			}
return( 3 );
		    }
		    if ( j==0 && (_unicode>>8)==0 )
			for ( k=0; k<256; ++k )
			    plane[k] = k;
		    else if ( j==0 )
			for ( k=0; k<128; ++k )
			    plane[k] = k;
		}
		table[_unicode>>8][_unicode&0xff] = _orig;
		if ( used[_unicode>>8]==NULL ) {
		    if ((used[_unicode>>8] = calloc(256,sizeof(long)))==NULL) {
			fprintf( stderr, NoMoreMemory );
			fclose(file);
			for ( k=0; k<256; ++k ) {
                            free(table[k]);
                            free(used[k]);
			}
return( 3 );
		    }
		}
		if ( almaps[j]!=-1 )
		    used[_unicode>>8][_unicode&0xff] |= (1<<almaps[j]);
	    }
	    fclose(file);

	    fprintf( header, "extern const unichar_t unicode_from_%s[];\n", alnames[j] );
	    fprintf( output, "const unichar_t unicode_from_%s[] = {\n", alnames[j] );
	    for ( i=0; i<256-8; i+=8 ) {
		fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x,",
			unicode[i], unicode[i+1], unicode[i+2], unicode[i+3],
			unicode[i+4], unicode[i+5], unicode[i+6], unicode[i+7]);
		add_data_comment_at_EOL(output, i);
	    }
	    fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n};\n\n",
		    unicode[i], unicode[i+1], unicode[i+2], unicode[i+3],
		    unicode[i+4], unicode[i+5], unicode[i+6], unicode[i+7]);

	    first = last = -1;
	    for ( k=0; k<256; ++k ) {
		if ( table[k]!=NULL ) {
		    if ( first==-1 ) first = k;
		    last = k;
		    plane = table[k];
		    fprintf( output, "static const unsigned char %s_from_unicode_%x[] = {\n", alnames[j], k );
		    for ( i=0; i<256-16; i+=16 ) {
			fprintf( output, "  0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x,",
				plane[i], plane[i+1], plane[i+2], plane[i+3],
				plane[i+4], plane[i+5], plane[i+6], plane[i+7],
				plane[i+8], plane[i+9], plane[i+10], plane[i+11],
				plane[i+12], plane[i+13], plane[i+14], plane[i+15]);
			add_data_comment_at_EOL(output, i);
		    }
		    fprintf( output, "  0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n};\n\n",
				plane[i], plane[i+1], plane[i+2], plane[i+3],
				plane[i+4], plane[i+5], plane[i+6], plane[i+7],
				plane[i+8], plane[i+9], plane[i+10], plane[i+11],
				plane[i+12], plane[i+13], plane[i+14], plane[i+15]);
		}
	    }
	    fprintf( output, "static const unsigned char * const %s_from_unicode_[] = {\n", alnames[j] );
	    for ( k=first; k<=last; ++k )
		if ( table[k]!=NULL )
		    fprintf( output, "    %s_from_unicode_%x%s", alnames[j], k, k!=last?",\n":"\n" );
		else
		    fprintf( output, "    c_allzeros,\n" );
	    fprintf( output, "};\n\n" );
	    fprintf( header, "extern struct charmap %s_from_unicode;\n", alnames[j]);
	    fprintf( output, "struct charmap %s_from_unicode = { %d, %d, (unsigned char **) %s_from_unicode_, (unichar_t *) unicode_from_%s };\n\n",
		    alnames[j], first, last, alnames[j], alnames[j]);

	    for ( k=first; k<=last; ++k ) {
		free(table[k]);
		table[k]=NULL;
	    }
	}
    }
    if ( l ) {				/* missing files, shouldn't go any further */
	for ( k=0; k<256; ++k )
            free(used[k]);
return( 2 );
    }

    fprintf( header, "\nextern unichar_t *unicode_from_alphabets[];\n" );
    fprintf( output, "unichar_t *unicode_from_alphabets[]={\n" );
    fprintf( output, "    (unichar_t *) unicode_from_win, 0,0,\n    (unichar_t *) unicode_from_i8859_1, \n" );
    for ( j=1; alphabets[j]!=NULL; ++j )
	fprintf( output, "    (unichar_t *) unicode_from_%s,\n", alnames[j] );
    fprintf( output, "    (unichar_t *) unicode_from_%s,\t/* Place holder for user-defined map */\n", alnames[0] );
    fprintf( output, "    0\n" );
    fprintf( output, "};\n" );
    fprintf( header, "extern struct charmap *alphabets_from_unicode[];\n" );
    fprintf( output, "\nstruct charmap *alphabets_from_unicode[]={ 0,0,0,\n" );
    for ( j=0; alphabets[j]!=NULL; ++j )
	fprintf( output, "    &%s_from_unicode,\n", alnames[j] );
    fprintf( output, "    &%s_from_unicode,\t/* Place holder for user-defined map*/\n", alnames[0] );
    fprintf( output, "    0\n" );
    fprintf( output, "};\n" );
	
    fprintf( header, "\n" );

    for ( i=0; i<=255; ++i )
	used[0][i] |= 1;		/* Map this plane entirely to 8859-1 */
    mask = 0;
    for ( j=0; alphabets[j]!=NULL; ++j )
	if ( almaps[j]!=-1 )
	    mask |= (1<<almaps[j]);
    for ( i=0; i<' '; ++i )
	used[0][i] |= mask;
return( 0 );				/* no errors encountered */
}

#define VERTMARK 0x1000000

static int ucs2_score(int val) {		/* Supplied by KANOU Hiroki */
    if ( val>=0x2e80 && val<=0x2fff )
return( 1 );				/* New CJK Radicals are least important */
    else if ( val>=VERTMARK )
return( 0 );				/* Then vertical guys */
			    /* only we can't handle vertical here */
    else if ( val>=0xf000 && val<=0xffff )
return( 3 );
/*    else if (( val>=0x3400 && val<0x3dff ) || (val>=0x4000 && val<=0x4dff))*/
    else if ( val>=0x3400 && val<=0x4dff )
return( 4 );
    else
return( 5 );
}

static int getnth(char *buffer, int col) {
    int i, val=0, best;
    char *end;
    int vals[10];

    if ( col==1 ) {
	/* first column is decimal, others are hex */
	if ( !isdigit(*buffer))
return( -1 );
	while ( isdigit(*buffer))
	    val = 10*val + *buffer++-'0';
return( val );
    }
    for ( i=1; i<col; ++buffer ) {
	if ( *buffer=='\t' )
	    ++i;
	else if ( *buffer=='\0' )
return( -1 );
    }
    val = strtol(buffer,&end,16);
    if ( end==buffer )
return( -1 );
    if ( *end=='v' ) {
	val += VERTMARK;
	++end;
    }
    if ( *end==',' ) {
	/* Multiple guess... now we've got to pick one */
	vals[0] = val;
	i = 1;
	while ( *end==',' && i<9 ) {
	    buffer = end+1;
	    vals[i] = strtol(buffer,&end,16);
	    if ( *end=='v' ) {
		vals[i] += VERTMARK;
		++end;
	    }
	    ++i;
	}
	vals[i] = 0;
	best = 0; val = -1;
	for ( i=0; vals[i]!=0; ++i ) {
	    if ( ucs2_score(vals[i])>best ) {
		val = vals[i];
		best = ucs2_score(vals[i]);
	    }
	}
    }

    if ( val >= VERTMARK )
return( -1 );

return( val );
}

static int dumpjis(FILE *output,FILE *header) {
    FILE *file;
    int i,j,k, first, last;
    long _orig, _unicode;
    unichar_t unicode208[94*94], unicode212[94*94];
    unichar_t *table[256], *plane;
    char buffer[400+1];

    memset(table,0,sizeof(table));
    buffer[400]='\0';

    j=0;				/* load info from aj16/cid2code.txt */
    file = fopen( adobecjk[j], "r" );
    if ( file==NULL ) {
	fprintf( stderr, CantReadFile, adobecjk[j]);
    } else {
	memset(unicode208,0,sizeof(unicode208));
	while ( fgets(buffer,sizeof(buffer)-1,file)!=NULL ) {
	    if (strlen(buffer)>=399) {
		fprintf( stderr, LineLengthBg,adobecjk[j],buffer );
		fclose(file);
		for ( k=0; k<256; ++k ) {
                    free(table[k]);
                    free(used[k]);
		}
return( 4 );
	    }
	    if ( buffer[0]=='#' )
	continue;
	    _orig = getnth(buffer,2);
	    if ( _orig==-1 )
	continue;
	    _unicode = getnth(buffer,22);
	    if ( _unicode==-1 ) {
		fprintf( stderr, "Eh? JIS 208-1997 %lx is unencoded\n", _orig );
	continue;
	    }
	    if ( _unicode>0xffff ) {
		fprintf( stderr, "Eh? JIS 208-1997 %lx is outside of BMP\n", _orig );
	continue;
	    }
	    if ( table[_unicode>>8]==NULL )
		if ((table[_unicode>>8] = calloc(256,sizeof(unichar_t)))==NULL) {
		    fprintf( stderr, NoMoreMemory );
		    fclose(file);
		    for ( k=0; k<256; ++k ) {
                        free(table[k]);
                        free(used[k]);
		    }
return( 3 );
		}
	    table[_unicode>>8][_unicode&0xff] = _orig;
	    _orig -= 0x2121;
	    _orig = (_orig>>8)*94 + (_orig&0xff);
	    if ( _orig>=94*94 )
		fprintf( stderr, "Attempt to index with %ld\n", _orig );
	    else {
		unicode208[_orig] = _unicode;
		if ( used[_unicode>>8]==NULL ) {
		    if ((used[_unicode>>8] = calloc(256,sizeof(long)))==NULL) {
			fprintf( stderr, NoMoreMemory );
			fclose(file);
			for ( k=0; k<256; ++k ) {
                            free(table[k]);
                            free(used[k]);
			}
return( 3 );
		    }
		}
		used[_unicode>>8][_unicode&0xff] |= (1<<em_jis208);
	    }
	}
	fclose(file);
    }

    j=1;				/* load info from aj20/cid2code.txt */
    file = fopen( adobecjk[j], "r" );
    if ( file==NULL ) {
	fprintf( stderr, CantReadFile, adobecjk[j]);
    } else {
	memset(unicode212,0,sizeof(unicode212));
	while ( fgets(buffer,sizeof(buffer)-1,file)!=NULL ) {
	    if (strlen(buffer)>=399) {
		fprintf( stderr, LineLengthBg,adobecjk[j],buffer );
		fclose(file);
		for ( k=0; k<256; ++k ) {
                    free(table[k]);
                    free(used[k]);
		}
return( 4 );
	    }
	    if ( buffer[0]=='#' )
	continue;
	    _orig = getnth(buffer,2);
	    if ( _orig==-1 )
	continue;
	    _unicode = getnth(buffer,7);
	    if ( _unicode==-1 ) {
		fprintf( stderr, "Eh? JIS 212-1990 %lx is unencoded\n", _orig );
	continue;
	    }
	    if ( _unicode>0xffff ) {
		fprintf( stderr, "Eh? JIS 212-1990 %lx is out of BMP U+%lx\n", _orig, _unicode );
	continue;
	    }
	    if ( table[_unicode>>8]==NULL )
		if ((table[_unicode>>8] = calloc(256,sizeof(unichar_t)))==NULL) {
		    fprintf( stderr, NoMoreMemory );
		    fclose(file);
		    for ( k=0; k<256; ++k ) {
                        free(table[k]);
                        free(used[k]);
		    }
return( 3 );
		}
	    if ( table[_unicode>>8][_unicode&0xff]==0 )
		table[_unicode>>8][_unicode&0xff] = _orig|0x8000;
	    else
		fprintf( stderr, "JIS clash at JIS212 %lx, unicode %lx\n", _orig, _unicode );	/* there are said to be a few of these, I'll just always map to 208 */
	    _orig -= 0x2121;
	    _orig = (_orig>>8)*94 + (_orig&0xff);
	    if ( _orig>=94*94 )
		fprintf( stderr, "Attempt to index JIS212 with %ld\n", _orig );
	    else {
		unicode212[_orig] = _unicode;
		if ( used[_unicode>>8]==NULL ) {
		    if ((used[_unicode>>8] = calloc(256,sizeof(long)))==NULL) {
			fprintf( stderr, NoMoreMemory );
			fclose(file);
			for ( k=0; k<256; ++k ) {
                            free(table[k]);
                            free(used[k]);
			}
return( 3 );
		    }
		}
		used[_unicode>>8][_unicode&0xff] |= (1<<em_jis212);
	    }
	}
	fclose(file);
    }

    j=0;
    fprintf( header, "extern const unichar_t unicode_from_%s[];\n", cjknames[j] );
    fprintf( output, "const unichar_t unicode_from_%s[] = {\n", cjknames[j] );
    for ( i=0; i<sizeof(unicode208)/sizeof(unicode208[0]); i+=8 ) {
	fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x,",
		unicode208[i], unicode208[i+1], unicode208[i+2], unicode208[i+3],
		unicode208[i+4], unicode208[i+5], unicode208[i+6], unicode208[i+7]);
	add_data_comment_at_EOL(output, i);
    }
    fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n};\n\n",
	    unicode208[i], unicode208[i+1], unicode208[i+2], unicode208[i+3],
	    unicode208[i+4], unicode208[i+5], unicode208[i+6], unicode208[i+7]);
    j=1;
    fprintf( header, "extern const unichar_t unicode_from_%s[];\n", cjknames[j] );
    fprintf( output, "const unichar_t unicode_from_%s[] = {\n", cjknames[j] );
    for ( i=0; i<sizeof(unicode212)/sizeof(unicode212[0]); i+=8 ) {
	fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x,",
		unicode212[i], unicode212[i+1], unicode212[i+2], unicode212[i+3],
		unicode212[i+4], unicode212[i+5], unicode212[i+6], unicode212[i+7]);
	add_data_comment_at_EOL(output, i);
    }
    fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n};\n\n",
	    unicode212[i], unicode212[i+1], unicode212[i+2], unicode212[i+3],
	    unicode212[i+4], unicode212[i+5], unicode212[i+6], unicode212[i+7]);

    first = last = -1;
    for ( k=0; k<256; ++k ) {
	if ( table[k]!=NULL ) {
	    if ( first==-1 ) first = k;
	    last = k;
	    plane = table[k];
	    fprintf( output, "static const unsigned short jis_from_unicode_%x[] = {\n", k );
	    for ( i=0; i<256-8; i+=8 ) {
		fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x,",
			plane[i], plane[i+1], plane[i+2], plane[i+3],
			plane[i+4], plane[i+5], plane[i+6], plane[i+7]);
		add_data_comment_at_EOL(output, i);
	    }
	    fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n};\n\n",
			plane[i], plane[i+1], plane[i+2], plane[i+3],
			plane[i+4], plane[i+5], plane[i+6], plane[i+7]);
	}
    }
    fprintf( output, "static const unsigned short * const jis_from_unicode_[] = {\n" );
    for ( k=first; k<=last; ++k )
	if ( table[k]!=NULL )
	    fprintf( output, "    jis_from_unicode_%x%s", k, k!=last?",\n":"\n" );
	else
	    fprintf( output, "    u_allzeros,\n" );
    fprintf( output, "};\n\n" );
    fprintf( header, "extern struct charmap2 jis_from_unicode;\n" );
    fprintf( output, "struct charmap2 jis_from_unicode = { %d, %d, (unsigned short **) jis_from_unicode_, (unichar_t *) unicode_from_%s };\n\n",
	    first, last, cjknames[j]);

    for ( k=first; k<=last; ++k )
	free(table[k]);
return( 0 );				/* no errors encountered */
}

static int dumpbig5(FILE *output,FILE *header) {
    FILE *file;
    int i,j,k, first, last;
    long _orig, _unicode;
    unichar_t unicode[0x6000];
    unichar_t *table[256], *plane;
    char buffer[400+1];

    j = 2;				/* load info from ac15/cid2code.txt */

    file = fopen( adobecjk[j], "r" );
    if ( file==NULL ) {
	fprintf( stderr, CantReadFile, adobecjk[j]);
    } else {
	buffer[400]='\0';
	memset(table,0,sizeof(table));
	memset(unicode,0,sizeof(unicode));

	while ( fgets(buffer,sizeof(buffer)-1,file)!=NULL ) {
	    if (strlen(buffer)>=399) {
		fprintf( stderr, LineLengthBg,adobecjk[j],buffer );
		fclose(file);
		for ( k=0; k<256; ++k ) {
                    free(table[k]);
                    free(used[k]);
		}
return( 4 );
	    }
	    if ( buffer[0]=='#' )
	continue;
	    _orig = getnth(buffer,3);
	    if ( /*_orig==-1*/ _orig<0xa100 )
	continue;
	    _unicode = getnth(buffer,11);
	    if ( _unicode==-1 ) {
		if ( _orig==0xa1c3 )
		    _unicode = 0xFFE3;
		else if ( _orig==0xa1c5 )
		    _unicode = 0x2cd;
		else if ( _orig==0xa1fe )
		    _unicode = 0xff0f;
		else if ( _orig==0xa240 )
		    _unicode = 0xff3c;
		else if ( _orig==0xa2cc )
		    _unicode = 0x5341;
		else if ( _orig==0xa2ce )
		    _unicode = 0x5345;
		else if ( _orig==0xa15a )
		    _unicode = 0x2574;
	    }
	    if ( _unicode==-1 ) {
		fprintf( stderr, "Eh? BIG5 %lx is unencoded\n", _orig );
	continue;
	    }
	    if ( _unicode>0xffff ) {
		fprintf( stderr, "Eh? BIG5 %lx is out of BMP U+%lx\n", _orig, _unicode );
	continue;
	    }
	    unicode[_orig-0xa100] = _unicode;
	    if ( table[_unicode>>8]==NULL )
		if ((table[_unicode>>8] = calloc(256,sizeof(unichar_t)))==NULL) {
		    fprintf( stderr, NoMoreMemory );
		    fclose(file);
		    for ( k=0; k<256; ++k ) {
                        free(table[k]);
                        free(used[k]);
		    }
return( 3 );
		}
	    table[_unicode>>8][_unicode&0xff] = _orig;
	    if ( used[_unicode>>8]==NULL ) {
		if ((used[_unicode>>8] = calloc(256,sizeof(long)))==NULL) {
		    fprintf( stderr, NoMoreMemory );
		    fclose(file);
		    for ( k=0; k<256; ++k ) {
                        free(table[k]);
                        free(used[k]);
		    }
return( 3 );
		}
	    }
	    used[_unicode>>8][_unicode&0xff] |= (1<<em_big5);
	}
	fclose(file);

	fprintf( header, "/* Subtract 0xa100 before indexing this array */\n" );
	fprintf( header, "extern const unichar_t unicode_from_%s[];\n", cjknames[j] );
	fprintf( output, "const unichar_t unicode_from_%s[] = {\n", cjknames[j] );
	for ( i=0; i<sizeof(unicode)/sizeof(unicode[0]); i+=8 ) {
	    fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x,",
		    unicode[i], unicode[i+1], unicode[i+2], unicode[i+3],
		    unicode[i+4], unicode[i+5], unicode[i+6], unicode[i+7]);
	    add_data_comment_at_EOL(output, i);
	}
	fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n};\n\n",
		unicode[i], unicode[i+1], unicode[i+2], unicode[i+3],
		unicode[i+4], unicode[i+5], unicode[i+6], unicode[i+7]);

	first = last = -1;
	for ( k=0; k<256; ++k ) {
	    if ( table[k]!=NULL ) {
		if ( first==-1 ) first = k;
		last = k;
		plane = table[k];
		fprintf( output, "static const unsigned short %s_from_unicode_%x[] = {\n", cjknames[j], k );
		for ( i=0; i<256-8; i+=8 ) {
		    fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x,",
			    plane[i], plane[i+1], plane[i+2], plane[i+3],
			    plane[i+4], plane[i+5], plane[i+6], plane[i+7]);
		    add_data_comment_at_EOL(output, i);
		}
		fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n};\n\n",
			    plane[i], plane[i+1], plane[i+2], plane[i+3],
			    plane[i+4], plane[i+5], plane[i+6], plane[i+7]);
	    }
	}
	fprintf( output, "static const unsigned short * const %s_from_unicode_[] = {\n", cjknames[j] );
	for ( k=first; k<=last; ++k )
	    if ( table[k]!=NULL )
		fprintf( output, "    %s_from_unicode_%x%s", cjknames[j], k, k!=last?",\n":"\n" );
	    else
		fprintf( output, "    u_allzeros,\n" );
	fprintf( output, "};\n\n" );
	fprintf( header, "extern struct charmap2 %s_from_unicode;\n", cjknames[j]);
	fprintf( output, "struct charmap2 %s_from_unicode = { %d, %d, (unsigned short **) %s_from_unicode_, (unichar_t *) unicode_from_%s };\n\n",
		cjknames[j], first, last, cjknames[j], cjknames[j]);

	for ( k=first; k<=last; ++k )
	    free(table[k]);
    }
return( 0 );				/* no errors encountered */
}

static int dumpbig5hkscs(FILE *output,FILE *header) {
    FILE *file;
    int i,j,k, first, last;
    long _orig, _unicode;
    unichar_t unicode[0x8000];
    unichar_t *table[256], *plane;
    char buffer[400+1];

    j=5;				/* load info from Big5HKSCS.txt */

    file = fopen( cjk[j], "r" );
    if ( file==NULL ) {
	fprintf( stderr, CantReadFile, cjk[j] );
    } else {
	buffer[400]='\0';
	memset(table,0,sizeof(table));
	memset(unicode,0,sizeof(unicode));

	while ( fgets(buffer,sizeof(buffer)-1,file)!=NULL ) {
	    if (strlen(buffer)>=399) {
		fprintf( stderr, LineLengthBg,cjk[j],buffer );
		fclose(file);
		for ( k=0; k<256; ++k ) {
                    free(table[k]);
                    free(used[k]);
		}
return( 4 );
	    }
	    if ( buffer[0]=='#' )
	continue;
	    if ( sscanf( buffer, "U+%lx: %lx", (unsigned long *) &_unicode, (unsigned long *) &_orig )!=2 )
	continue;
	    if ( _orig<0x8140 )
	continue;
	    if ( _unicode==-1 ) {
		fprintf( stderr, "Eh? BIG5HKSCS %lx is unencoded\n", _orig );
	continue;
	    }
	    unicode[_orig-0x8100] = _unicode;
	    if ( table[_unicode>>8]==NULL )
		if ((table[_unicode>>8] = calloc(256,sizeof(unichar_t)))==NULL) {
		    fprintf( stderr, NoMoreMemory );
		    fclose(file);
		    for ( k=0; k<256; ++k ) {
                        free(table[k]);
                        free(used[k]);
		    }
return( 3 );
		}
	    table[_unicode>>8][_unicode&0xff] = _orig;
	    if ( used[_unicode>>8]==NULL ) {
		if ((used[_unicode>>8] = calloc(256,sizeof(long)))==NULL) {
		    fprintf( stderr, NoMoreMemory );
		    fclose(file);
		    for ( k=0; k<256; ++k ) {
                        free(table[k]);
                        free(used[k]);
		    }
return( 3 );
		}
	    }
	    used[_unicode>>8][_unicode&0xff] |= (1<<em_big5hkscs);
	}
	fclose(file);

	fprintf( header, "/* Subtract 0x8100 before indexing this array */\n" );
	fprintf( header, "extern const unichar_t unicode_from_%s[];\n", cjknames[j] );
	fprintf( output, "const unichar_t unicode_from_%s[] = {\n", cjknames[j] );
	for ( i=0; i<sizeof(unicode)/sizeof(unicode[0]); i+=8 ) {
	    fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x,",
		    unicode[i], unicode[i+1], unicode[i+2], unicode[i+3],
		    unicode[i+4], unicode[i+5], unicode[i+6], unicode[i+7]);
	    add_data_comment_at_EOL(output, i);
	}
	fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n};\n\n",
		unicode[i], unicode[i+1], unicode[i+2], unicode[i+3],
		unicode[i+4], unicode[i+5], unicode[i+6], unicode[i+7]);

	first = last = -1;
	for ( k=0; k<256; ++k ) {
	    if ( table[k]!=NULL ) {
		if ( first==-1 ) first = k;
		last = k;
		plane = table[k];
		fprintf( output, "static const unsigned short %s_from_unicode_%x[] = {\n", cjknames[j], k );
		for ( i=0; i<256-8; i+=8 ) {
		    fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x,",
			    plane[i], plane[i+1], plane[i+2], plane[i+3],
			    plane[i+4], plane[i+5], plane[i+6], plane[i+7]);
		    add_data_comment_at_EOL(output, i);
		}
		fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n};\n\n",
			    plane[i], plane[i+1], plane[i+2], plane[i+3],
			    plane[i+4], plane[i+5], plane[i+6], plane[i+7]);
	    }
	}
	fprintf( output, "static const unsigned short * const %s_from_unicode_[] = {\n", cjknames[j] );
	for ( k=first; k<=last; ++k )
	    if ( table[k]!=NULL )
		fprintf( output, "    %s_from_unicode_%x%s", cjknames[j], k, k!=last?",\n":"\n" );
	    else
		fprintf( output, "    u_allzeros,\n" );
	fprintf( output, "};\n\n" );
	fprintf( header, "extern struct charmap2 %s_from_unicode;\n", cjknames[j]);
	fprintf( output, "struct charmap2 %s_from_unicode = { %d, %d, (unsigned short **) %s_from_unicode_, (unichar_t *) unicode_from_%s };\n\n",
		cjknames[j], first, last, cjknames[j], cjknames[j]);

	for ( k=first; k<=last; ++k )
	    free(table[k]);
    }
return( 0 );				/* no errors encountered */
}

static int dumpWansung(FILE *output,FILE *header) {
    FILE *file;
    int i,j,k, first, last;
    long _orig, _unicode, _johab;
    unichar_t unicode[94*94], junicode[0x7c00];
    unichar_t *table[256], *plane, *jtable[256];
    char buffer[400+1];
    /* Johab high=[0x84-0xf9] low=[0x31-0xfe] */

    buffer[400]='\0';

    j=4;				/* load info from ak12/cid2code.txt */

	memset(table,0,sizeof(table));
	memset(jtable,0,sizeof(jtable));

	file = fopen( adobecjk[j], "r" );
	if ( file==NULL ) {
	    fprintf( stderr, CantReadFile, adobecjk[j]);
	} else {
	    memset(unicode,0,sizeof(unicode));
	    memset(junicode,0,sizeof(junicode));
	    while ( fgets(buffer,sizeof(buffer)-1,file)!=NULL ) {
		if (strlen(buffer)>=399) {
		    fprintf( stderr, LineLengthBg,adobecjk[j],buffer );
		    fclose(file);
		    for ( k=0; k<256; ++k ) {
                        free(table[k]);
                        free(jtable[k]);
                        free(used[k]);
		    }
return( 4 );
		}
		if ( buffer[0]=='#' )
	    continue;
		_johab = getnth(buffer,7);
		_orig = getnth(buffer,2);
		_unicode = getnth(buffer,11);
		if ( _unicode==-1 ) {
		    if ( _orig>=0x2121 && (_orig&0xff)>=0x21 && _orig<=0x7e7e && (_orig&0xff)<=0x7e )
			fprintf( stderr, "Eh? Wansung %lx is unencoded\n", _orig );
		    else if ( _johab>=0x8431 && _johab<=0xf9fe )
			fprintf( stderr, "Eh? Johab %lx is unencoded\n", _johab );
	    continue;
		}
		if ( _unicode>0xffff ) {
		    if ( _orig>=0x2121 && (_orig&0xff)>=0x21 && _orig<=0x7e7e && (_orig&0xff)<=0x7e )
			fprintf( stderr, "Eh? Wansung %lx is out of BMP U+%lx\n", _orig, _unicode );
		    else if ( _johab>=0x8431 && _johab<=0xf9fe )
			fprintf( stderr, "Eh? Johab %lx is out of BMP U+%lx\n", _johab, _unicode );
	    continue;
		}
		if ( _orig>=0x2121 && (_orig&0xff)>=0x21 && _orig<=0x7e7e && (_orig&0xff)<=0x7e ) {
		    if ( table[_unicode>>8]==NULL )
			if ((table[_unicode>>8] = calloc(256,sizeof(unichar_t)))==NULL) {
			    fprintf( stderr, NoMoreMemory );
			    fclose(file);
			    for ( k=0; k<256; ++k ) {
                                free(table[k]);
                                free(jtable[k]);
                                free(used[k]);
			    }
return( 3 );
			}
		    table[_unicode>>8][_unicode&0xff] = _orig;
		    _orig -= 0x2121;
		    _orig = (_orig>>8)*94 + (_orig&0xff);
		    if ( _orig>=94*94 ) {
			fprintf( stderr, "Not 94x94\n" );
	    continue;
		    }
		    unicode[_orig] = _unicode;
		    if ( used[_unicode>>8]==NULL ) {
			if ((used[_unicode>>8] = calloc(256,sizeof(long)))==NULL) {
			    fprintf( stderr, NoMoreMemory );
			    fclose(file);
			    for ( k=0; k<256; ++k ) {
                                free(table[k]);
                                free(jtable[k]);
                                free(used[k]);
			    }
return( 3 );
			}
		    }
		    used[_unicode>>8][_unicode&0xff] |= (1<<cjkmaps[j]);
		}
		if ( _johab>=0x8431 && _johab<=0xf9fe ) {
		    if ( jtable[_unicode>>8]==NULL )
			if ((jtable[_unicode>>8] = calloc(256,sizeof(unichar_t)))==NULL) {
			    fprintf( stderr, NoMoreMemory );
			    fclose(file);
			    for ( k=0; k<256; ++k ) {
                                free(table[k]);
                                free(jtable[k]);
                                free(used[k]);
			    }
return( 3 );
			}
		    jtable[_unicode>>8][_unicode&0xff] = _johab;
		    _johab -= 0x8400;
		    junicode[_johab] = _unicode;
		    if ( used[_unicode>>8]==NULL ) {
			if ((used[_unicode>>8] = calloc(256,sizeof(long)))==NULL) {
			    fprintf( stderr, NoMoreMemory );
			    fclose(file);
			    for ( k=0; k<256; ++k ) {
                                free(table[k]);
                                free(jtable[k]);
                                free(used[k]);
			    }
return( 3 );
			}
		    }
		    used[_unicode>>8][_unicode&0xff] |= (1<<em_johab);
		}
	    }
	    fclose(file);
	}

	/* First Wansung */
	fprintf( header, "extern const unichar_t unicode_from_%s[];\n", cjknames[j] );
	fprintf( output, "const unichar_t unicode_from_%s[] = {\n", cjknames[j] );
	for ( i=0; i<sizeof(unicode)/sizeof(unicode[0]); i+=8 ) {
	    fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x,",
		    unicode[i], unicode[i+1], unicode[i+2], unicode[i+3],
		    unicode[i+4], unicode[i+5], unicode[i+6], unicode[i+7]);
	    add_data_comment_at_EOL(output, i);
	}
	fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n};\n\n",
		unicode[i], unicode[i+1], unicode[i+2], unicode[i+3],
		unicode[i+4], unicode[i+5], unicode[i+6], unicode[i+7]);

	first = last = -1;
	for ( k=0; k<256; ++k ) {
	    if ( table[k]!=NULL ) {
		if ( first==-1 ) first = k;
		last = k;
		plane = table[k];
		fprintf( output, "static unsigned short %s_from_unicode_%x[] = {\n", cjknames[j], k );
		for ( i=0; i<256-8; i+=8 ) {
		    fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x,",
			    plane[i], plane[i+1], plane[i+2], plane[i+3],
			    plane[i+4], plane[i+5], plane[i+6], plane[i+7]);
		    add_data_comment_at_EOL(output, i);
		}
		fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n};\n\n",
			    plane[i], plane[i+1], plane[i+2], plane[i+3],
			    plane[i+4], plane[i+5], plane[i+6], plane[i+7]);
	    }
	}
	fprintf( output, "static const unsigned short * const %s_from_unicode_[] = {\n", cjknames[j] );
	for ( k=first; k<=last; ++k )
	    if ( table[k]!=NULL )
		fprintf( output, "    %s_from_unicode_%x%s", cjknames[j], k, k!=last?",\n":"\n" );
	    else
		fprintf( output, "    u_allzeros,\n" );
	fprintf( output, "};\n\n" );
	fprintf( header, "extern struct charmap2 %s_from_unicode;\n", cjknames[j]);
	fprintf( output, "struct charmap2 %s_from_unicode = { %d, %d, (unsigned short **) %s_from_unicode_, (unichar_t *) unicode_from_%s };\n\n",
		cjknames[j], first, last, cjknames[j], cjknames[j]);

	if ( first==-1 )
	    fprintf( stderr, "No Hangul\n" );
	else for ( k=first; k<=last; ++k ) {
	    free(table[k]);
	    table[k]=NULL;
	}

	/* Then Johab */
	fprintf( header, "/* Subtract 0x8400 before indexing this array */\n" );
	fprintf( header, "extern const unichar_t unicode_from_johab[];\n" );
	fprintf( output, "const unichar_t unicode_from_johab[] = {\n" );
	for ( i=0; i<sizeof(junicode)/sizeof(junicode[0]); i+=8 ) {
	    fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x,",
		    junicode[i], junicode[i+1], junicode[i+2], junicode[i+3],
		    junicode[i+4], junicode[i+5], junicode[i+6], junicode[i+7]);
	    add_data_comment_at_EOL(output, i);
	}
	fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n};\n\n",
		junicode[i], junicode[i+1], junicode[i+2], junicode[i+3],
		junicode[i+4], junicode[i+5], junicode[i+6], junicode[i+7]);

	first = last = -1;
	for ( k=0; k<256; ++k ) {
	    if ( jtable[k]!=NULL ) {
		if ( first==-1 ) first = k;
		last = k;
		plane = jtable[k];
		fprintf( output, "static unsigned short johab_from_unicode_%x[] = {\n", k );
		for ( i=0; i<256-8; i+=8 ) {
		    fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x,",
			    plane[i], plane[i+1], plane[i+2], plane[i+3],
			    plane[i+4], plane[i+5], plane[i+6], plane[i+7]);
		    add_data_comment_at_EOL(output, i);
		}
		fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n};\n\n",
			    plane[i], plane[i+1], plane[i+2], plane[i+3],
			    plane[i+4], plane[i+5], plane[i+6], plane[i+7]);
	    }
	}
	fprintf( output, "static const unsigned short * const johab_from_unicode_[] = {\n" );
	for ( k=first; k<=last; ++k )
	    if ( jtable[k]!=NULL )
		fprintf( output, "    johab_from_unicode_%x%s", k, k!=last?",\n":"\n" );
	    else
		fprintf( output, "    u_allzeros,\n" );
	fprintf( output, "};\n\n" );
	fprintf( header, "extern struct charmap2 johab_from_unicode;\n" );
	fprintf( output, "struct charmap2 johab_from_unicode = { %d, %d, (unsigned short **) johab_from_unicode_, (unichar_t *) unicode_from_johab };\n\n",
		first, last );

	if ( first==-1 )
	    fprintf( stderr, "No Johab\n" );
	else for ( k=first; k<=last; ++k ) {
	    free(table[k]);
	    table[k]=NULL;
            free(jtable[k]);
	}
return( 0 );				/* no errors encountered */
}

static int dumpgb2312(FILE *output,FILE *header) {
    FILE *file;
    int i,j,k, first, last;
    long _orig, _unicode;
    unichar_t unicode[94*94];
    unichar_t *table[256], *plane;
    char buffer[400+1];

    buffer[400]='\0';
    memset(table,0,sizeof(table));

    j = 3;				/* load info from ag15/cid2code.txt */
	file = fopen( adobecjk[j], "r" );
	if ( file==NULL ) {
	    fprintf( stderr, CantReadFile, adobecjk[j]);
	} else {
	    memset(unicode,0,sizeof(unicode));
	    while ( fgets(buffer,sizeof(buffer)-1,file)!=NULL ) {
		if (strlen(buffer)>=399) {
		    fprintf( stderr, LineLengthBg,adobecjk[j],buffer );
		    fclose(file);
		    for ( k=0; k<256; ++k ) {
                        free(table[k]);
                        free(used[k]);
		    }
return( 4 );
		}
		if ( buffer[0]=='#' )
	    continue;
		_orig = getnth(buffer,2);
		if ( _orig==-1 )
	    continue;
		_unicode = getnth(buffer,14);
		if ( _unicode==-1 ) {
		    fprintf( stderr, "Eh? GB2312-80 %lx is unencoded\n", _orig );
	    continue;
		}
		if ( _unicode>0xffff ) {
		    fprintf( stderr, "Eh? GB2312-80 %lx is out of BMP U+%lx\n", _orig, _unicode );
	    continue;
		}
		if ( table[_unicode>>8]==NULL )
		    if ((table[_unicode>>8] = calloc(256,sizeof(unichar_t)))==NULL) {
			fprintf( stderr, NoMoreMemory );
			fclose(file);
			for ( k=0; k<256; ++k ) {
                            free(table[k]);
                            free(used[k]);
			}
return( 3 );
		    }
		table[_unicode>>8][_unicode&0xff] = _orig;
		_orig -= 0x2121;
		_orig = (_orig>>8)*94 + (_orig&0xff);
		unicode[_orig] = _unicode;
		if ( used[_unicode>>8]==NULL ) {
		    if ((used[_unicode>>8] = calloc(256,sizeof(long)))==NULL) {
			fprintf( stderr, NoMoreMemory );
			fclose(file);
			for ( k=0; k<256; ++k ) {
                            free(table[k]);
                            free(used[k]);
			}
return( 3 );
		    }
		}
		used[_unicode>>8][_unicode&0xff] |= (1<<cjkmaps[j]);
	    }
	    fclose(file);
	}

	fprintf( header, "extern const unichar_t unicode_from_%s[];\n", cjknames[j] );
	fprintf( output, "const unichar_t unicode_from_%s[] = {\n", cjknames[j] );
	for ( i=0; i<sizeof(unicode)/sizeof(unicode[0]); i+=8 ) {
	    fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x,",
		    unicode[i], unicode[i+1], unicode[i+2], unicode[i+3],
		    unicode[i+4], unicode[i+5], unicode[i+6], unicode[i+7]);
	    add_data_comment_at_EOL(output, i);
	}
	fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n};\n\n",
		unicode[i], unicode[i+1], unicode[i+2], unicode[i+3],
		unicode[i+4], unicode[i+5], unicode[i+6], unicode[i+7]);

	first = last = -1;
	for ( k=0; k<256; ++k ) {
	    if ( table[k]!=NULL ) {
		if ( first==-1 ) first = k;
		last = k;
		plane = table[k];
		fprintf( output, "static unsigned short %s_from_unicode_%x[] = {\n", cjknames[j], k );
		for ( i=0; i<256-8; i+=8 ) {
		    fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x,",
			    plane[i], plane[i+1], plane[i+2], plane[i+3],
			    plane[i+4], plane[i+5], plane[i+6], plane[i+7]);
		    add_data_comment_at_EOL(output, i);
		}
		fprintf( output, "  0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n};\n\n",
			    plane[i], plane[i+1], plane[i+2], plane[i+3],
			    plane[i+4], plane[i+5], plane[i+6], plane[i+7]);
	    }
	}
	fprintf( output, "static const unsigned short * const %s_from_unicode_[] = {\n", cjknames[j] );
	for ( k=first; k<=last; ++k )
	    if ( table[k]!=NULL )
		fprintf( output, "    %s_from_unicode_%x%s", cjknames[j], k, k!=last?",\n":"\n" );
	    else
		fprintf( output, "    u_allzeros,\n" );
	fprintf( output, "};\n\n" );
	fprintf( header, "extern struct charmap2 %s_from_unicode;\n", cjknames[j]);
	fprintf( output, "struct charmap2 %s_from_unicode = { %d, %d, (unsigned short **) %s_from_unicode_, (unichar_t *) unicode_from_%s };\n\n",
		cjknames[j], first, last, cjknames[j], cjknames[j]);

	if ( first==-1 )
	    fprintf( stderr, "No 94x94\n" );
	else for ( k=first; k<=last; ++k ) {
	    free(table[k]);
	    table[k]=NULL;
	}
return( 0 );				/* no errors encountered */
}

static int dumpcjks(FILE *output,FILE *header) {
    int i;

    fprintf( output, GeneratedFileMessage );

    fprintf(output, "#include <chardata.h>\n\n" );
    fprintf(output, "const unsigned short u_allzeros[256] = { 0 };\n\n" );

    if (i=dumpjis(output,header)) return( i );		/* aj16/cid2code.txt, aj20/cid2code.txt */
    if (i=dumpbig5(output,header)) return( i );		/* ac15/cid2code.txt */
    if (i=dumpbig5hkscs(output,header)) return( i );	/* Big5HKSCS.txt */
    if (i=dumpWansung(output,header)) return( i );	/* ak12/cid2code.txt */
    if (i=dumpgb2312(output,header)) return( i );	/* ag15/cid2code.txt */
return( 0 );
}

static void dumptrans(FILE *output, FILE *header) {
    unsigned long *plane;
    int k, i;

    fprintf( output, GeneratedFileMessage );

    fprintf(output, "static const unsigned long l_allzeros[256] = { 0 };\n" );
    for ( k=0; k<256; ++k ) {
	if ( used[k]!=NULL ) {
	    plane = used[k];
	    fprintf( output, "static const unsigned long unicode_backtrans_%x[] = {\n", k );
	    for ( i=0; i<256-8; i+=8 ) {
		fprintf( output, "  0x%06lx, 0x%06lx, 0x%06lx, 0x%06lx, 0x%06lx, 0x%06lx, 0x%06lx, 0x%06lx,",
			plane[i], plane[i+1], plane[i+2], plane[i+3],
			plane[i+4], plane[i+5], plane[i+6], plane[i+7]);
		add_data_comment_at_EOL(output, i);
	    }
	    fprintf( output, "  0x%06lx, 0x%06lx, 0x%06lx, 0x%06lx, 0x%06lx, 0x%06lx, 0x%06lx, 0x%06lx\n};\n\n",
			plane[i], plane[i+1], plane[i+2], plane[i+3],
			plane[i+4], plane[i+5], plane[i+6], plane[i+7]);
	}
    }
    fprintf( header, "\n/* a mask for each character saying what charset(s) it may be found in */\n" );
    fprintf( header, "extern const unsigned long * const unicode_backtrans[];\n" );
    fprintf( output, "const unsigned long *const unicode_backtrans[] = {\n" );
    for ( k=0; k<256; ++k )
	if ( used[k]!=NULL )
	    fprintf( output, "    unicode_backtrans_%x%s", k, k!=255?",\n":"\n" );
	else
	    fprintf( output, "    l_allzeros,\n" );
    fprintf( output, "};\n" );
}

int main(int argc, char **argv) {
    FILE *output, *header;
    int i;

    if (( output = fopen( "alphabet.c", "w" ))==NULL ) {
	fprintf( stderr, CantSaveFile, "alphabet.c" );
return 1;
    }
    if (( header = fopen( "chardata.h", "w" ))==NULL ) {
	fprintf( stderr, CantSaveFile, "chardata.h" );
	fclose(output);
return 1;
    }

    fprintf( output, GeneratedFileMessage );
    fprintf( header, GeneratedFileMessage );

    fprintf( header, "#include <basics.h>\n\n" );
    fprintf( header, "struct charmap {\n    int first, last;\n    unsigned char **table;\n    unichar_t *totable;\n};\n" );
    fprintf( header, "struct charmap2 {\n    int first, last;\n    unsigned short **table;\n    unichar_t *totable;\n};\n\n" );

    if ( i=dumpalphas(output,header)) {	/* load files listed in alphabets[] */
	fclose(header);
	fclose(output);
return( i );
    }

    /*dumprandom(output,header);*/
    fclose(output);

    if (( output = fopen( "cjk.c", "w" ))==NULL ) {
	fprintf( stderr, CantSaveFile, "cjk.c" );
	fclose(header);
return 1;
    }

    if ( i=dumpcjks(output,header)) {	/* load files listed in cjk[] */
	fclose(header);
	fclose(output);
return( i );
    }

    fclose(output);

    if (( output = fopen( "backtrns.c", "w" ))==NULL ) {
	fprintf( stderr, CantSaveFile, "backtrns.c" );
	fclose(header);
return 1;
    }
    dumptrans(output,header);

    /* This really should be in make ctype, but putting it there causes all */
    /*  sorts of build problems in things when they happen out of order */
    fprintf( header,"\nextern const unichar_t *const * const unicode_alternates[];\n" );

    fclose(output); fclose(header);

    for ( i=0; i<256; ++i )
        free(used[i]);
return 0;
}
