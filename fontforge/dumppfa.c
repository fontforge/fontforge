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

#include "dumppfa.h"

#include "autohint.h"
#include "bvedit.h"
#include "fontforge.h"
#include "fvfonts.h"
#include "http.h"
#include "parsepfa.h"
#include "psread.h"
#include "splineorder2.h"
#include "splinesave.h"
#include "splinesaveafm.h"
#include "splineutil.h"
#include "splineutil2.h"
#include "tottf.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ustring.h>
#include <utype.h>
#include <unistd.h>
#include <locale.h>
#if !defined(__MINGW32__)
# include <pwd.h>
#endif
#include <stdarg.h>
#include <gutils.h>
#include "psfont.h"
#include "splinefont.h"
#include <gdraw.h>		/* For image defn */
#include "print.h"		/* For makePatName */

#ifdef __CygWin
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <unistd.h>
#endif

extern int autohint_before_generate;
char *xuid=NULL;

typedef void (*DumpChar)(int ch,void *data);
struct fileencryptdata {
    DumpChar olddump;
    void *olddata;
    unsigned short r;
    int hexline;
};
#define c1	52845
#define c2	22719

static void encodehex(int plain, void *d) {
    struct fileencryptdata *fed = d;
    unsigned char cypher = ( plain ^ (fed->r>>8));

    fed->r = (cypher + fed->r) * c1 + c2;
    {
	int ch1, ch2;

	ch1 = cypher>>4;
	if ( ch1<=9 )
	    (fed->olddump)('0'+ch1,fed->olddata);
	else
	    (fed->olddump)('A'-10+ch1,fed->olddata);
	ch2 = cypher&0xf;
	if ( ch2<=9 )
	    (fed->olddump)('0'+ch2,fed->olddata);
	else
	    (fed->olddump)('A'-10+ch2,fed->olddata);
	fed->hexline += 2;
	if ( fed->hexline>70 ) {
	    (fed->olddump)('\n',fed->olddata);
	    fed->hexline = 0;
	}
    }
}

static void encodebin(int plain, void *d) {
    struct fileencryptdata *fed = d;
    unsigned char cypher = ( plain ^ (fed->r>>8));

    fed->r = (cypher + fed->r) * c1 + c2;
    (fed->olddump)(cypher,fed->olddata);
}

static DumpChar startfileencoding(DumpChar dumpchar,void *data,
	struct fileencryptdata *fed,int dobinary) {
    static unsigned char randombytes[4] = { 0xaa, 0x55, 0x3e, 0x4d };
    DumpChar func;

    randombytes[0] += 3;
    randombytes[1] += 5;
    randombytes[2] += 7;
    randombytes[3] += 11;

    fed->olddump = dumpchar;
    fed->olddata = data;
    fed->r = 55665;
    fed->hexline = 0;

    if ( dobinary ) { unsigned short r; unsigned char cypher;
	while ( 1 ) {
	    /* find some random bytes where at least one of them encrypts to */
	    /*  a non hex character */
	    r = 55665;
	    cypher = ( randombytes[0] ^ (r>>8));
	    if ( isspace(cypher) )
	goto try_again;
	    if ( cypher<'0' || (cypher>'9' && cypher<'A') || (cypher>'F' && cypher<'a') || cypher>'f' )
	break;
	    r = (cypher + r) * c1 + c2;
	    cypher = ( randombytes[1] ^ (r>>8));
	    if ( cypher<'0' || (cypher>'9' && cypher<'A') || (cypher>'F' && cypher<'a') || cypher>'f' )
	break;
	    r = (cypher + r) * c1 + c2;
	    cypher = ( randombytes[2] ^ (r>>8));
	    if ( cypher<'0' || (cypher>'9' && cypher<'A') || (cypher>'F' && cypher<'a') || cypher>'f' )
	break;
	    r = (cypher + r) * c1 + c2;
	    cypher = ( randombytes[3] ^ (r>>8));
	    if ( cypher<'0' || (cypher>'9' && cypher<'A') || (cypher>'F' && cypher<'a') || cypher>'f' )
	break;

	try_again:
	    randombytes[0] += 3;
	    randombytes[1] += 5;
	    randombytes[2] += 7;
	    randombytes[3] += 11;
	}
    }

    func = dobinary ? encodebin : encodehex;
    func(randombytes[0],fed);
    func(randombytes[1],fed);
    func(randombytes[2],fed);
    func(randombytes[3],fed);
return( func );
}

/* Encode a string in adobe's format. choose a different set of initial random*/
/*  bytes every time. (the expected value of leniv is 4. we have some support */
/*  for values bigger than 5 but not as much as for values <=4) */
static void encodestrout(void (*dumpchar)(int ch,void *data), void *data,
	unsigned char *value, int len, int leniv) {
    unsigned short r = 4330;
    unsigned char plain, cypher;
    static unsigned char randombytes[10] = { 0xaa, 0x55, 0x3e, 0x4d, 0x89, 0x98, 'a', '4', 0, 0xff };

    randombytes[0] += 3;
    randombytes[1] += 5;
    randombytes[2] += 7;
    randombytes[3] += 11;
    randombytes[4] += 13;

    while ( leniv>0 ) {
	plain = randombytes[leniv--%10];
	cypher = ( plain ^ (r>>8));
	r = (cypher + r) * c1 + c2;
	dumpchar(cypher,data);
    }
    while ( len-->0 ) {
	plain = *value++;
	cypher = ( plain ^ (r>>8));
	r = (cypher + r) * c1 + c2;
	dumpchar(cypher,data);
    }
}

static void dumpstr(void (*dumpchar)(int ch,void *data), void *data, const char *buf) {
    while ( *buf )
	dumpchar(*buf++,data);
}

static void dumpcarefully(void (*dumpchar)(int ch,void *data), void *data, const char *buf) {
    int ch;

    while ( (ch = *(unsigned char *) buf++)!='\0' ) {
	if ( ch<' ' || ch>=0x7f || ch=='\\' || ch=='(' || ch==')' ) {
	    dumpchar('\\',data);
	    dumpchar('0'+(ch>>6),data);
	    dumpchar('0'+((ch>>3)&0x7),data);
	    dumpchar('0'+(ch&0x7),data);
	} else
	    dumpchar(ch,data);
    }
}

static void dumpascomments(void (*dumpchar)(int ch,void *data), void *data, const char *buf) {
    int ch;

    dumpchar('%',data);
    dumpchar(' ',data);
    while ( (ch = *buf++)!='\0' ) {
	if ( ch=='\n' || ch=='\r' ) {
	    dumpchar('\n',data);
	    if ( ch=='\r' && *buf=='\n' ) ++buf;
	    if ( *buf=='\0' )
return;
	    dumpchar('%',data);
	    dumpchar(' ',data);
	} else
	    dumpchar(ch,data);
    }
    dumpchar('\n',data);
}

static void dumpstrn(void (*dumpchar)(int ch,void *data), void *data, const char *buf, int n) {
    while ( n-->0 )
	dumpchar(*buf++,data);
}

static void dumpf(void (*dumpchar)(int ch,void *data), void *data, const char *format, ...) {
    va_list args;
    char buffer[300];

    va_start(args,format);
    vsprintf( buffer, format, args);
    va_end(args);
    dumpstr(dumpchar,data,buffer);
}

static int isStdEncoding(const char *encoding[256]) {
    int i;

    for ( i=0; i<256; ++i )
	if ( strcmp(encoding[i],".notdef")==0 )
	    /* that's ok */;
	else if ( strcmp(encoding[i],AdobeStandardEncoding[i])!=0 )
return( 0 );

return( 1 );
}

static void dumpblues(void (*dumpchar)(int ch,void *data), void *data,
	const char *name, real *arr, int len, const char *ND) {
    int i;
    for ( --len; len>=0 && arr[len]==0.0; --len );
    ++len;
    if ( len&1 ) ++len;
    dumpf( dumpchar,data,"/%s [",name);
    for ( i=0; i<len; ++i )
	dumpf( dumpchar,data,"%g ", (double) arr[i]);
    dumpf( dumpchar,data,"]%s\n",ND );
}

static void dumpdblmaxarray(void (*dumpchar)(int ch,void *data), void *data,
	const char *name, real *arr, int len, const char *modifiers, const char *ND) {
    int i;
    for ( --len; len>=0 && arr[len]==0.0; --len );
    ++len;
    dumpf( dumpchar,data,"/%s [",name);
    for ( i=0; i<len; ++i )
	dumpf( dumpchar,data,"%g ", (double) arr[i]);
    dumpf( dumpchar,data,"]%s%s\n", modifiers, ND );
}

static void dumpdblarray(void (*dumpchar)(int ch,void *data), void *data,
	const char *name, double *arr, int len, const char *modifiers, int exec) {
    int i;
    dumpf( dumpchar,data,"/%s %c",name, exec? '{' : '[' );
    for ( i=0; i<len; ++i )
	dumpf( dumpchar,data,"%g ", arr[i]);
    dumpf( dumpchar,data,"%c%sdef\n", exec ? '}' : ']', modifiers );
}

struct psdict *PSDictCopy(struct psdict *dict) {
    struct psdict *ret;
    int i;

    if ( dict==NULL )
return( NULL );

    ret = calloc(1,sizeof(struct psdict));
    ret->cnt = dict->cnt; ret->next = dict->next;
    ret->keys = calloc(ret->cnt,sizeof(char *));
    ret->values = calloc(ret->cnt,sizeof(char *));
    for ( i=0; i<dict->next; ++i ) {
	ret->keys[i] = copy(dict->keys[i]);
	ret->values[i] = copy(dict->values[i]);
    }

return( ret );
}

int PSDictFindEntry(struct psdict *dict, const char *key) {
    int i;

    if ( dict==NULL )
return( -1 );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
return( i );

return( -1 );
}

char *PSDictHasEntry(struct psdict *dict, const char *key) {
    int i;

    if ( dict==NULL )
return( NULL );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
return( dict->values[i] );

return( NULL );
}

int PSDictSame(struct psdict *dict1, struct psdict *dict2) {
    int i;

    if ( (dict1==NULL || dict1->cnt==0) && (dict2==NULL || dict2->cnt==0))
return( true );
    if ( dict1==NULL || dict2==NULL || dict1->cnt!=dict2->cnt )
return( false );

    for ( i=0; i<dict1->cnt; ++i ) {
	char *val = PSDictHasEntry(dict2,dict1->keys[i]);
	if ( val==NULL || strcmp(val,dict1->values[i])!=0 )
return( false );
    }
return( true );
}

int PSDictRemoveEntry(struct psdict *dict, const char *key) {
    int i;

    if ( dict==NULL )
return( false );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
    break;
    if ( i==dict->next )
return( false );
    free( dict->keys[i]);
    free( dict->values[i] );
    --dict->next;
    while ( i<dict->next ) {
	dict->keys[i] = dict->keys[i+1];
	dict->values[i] = dict->values[i+1];
	++i;
    }

return( true );
}

int PSDictChangeEntry(struct psdict *dict, const char *key, const char *newval) {
    int i;

    if ( dict==NULL )
return( -1 );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
    break;
    if ( i==dict->next ) {
	if ( dict->next>=dict->cnt ) {
	    dict->cnt += 10;
	    dict->keys = realloc(dict->keys,dict->cnt*sizeof(char *));
	    dict->values = realloc(dict->values,dict->cnt*sizeof(char *));
	}
	dict->keys[dict->next] = copy(key);
	dict->values[dict->next] = NULL;
	++dict->next;
    }
    free(dict->values[i]);
    dict->values[i] = copy(newval);
return( i );
}

static void dumpsubrs(void (*dumpchar)(int ch,void *data), void *data,
	SplineFont *sf, struct pschars *subrs ) {
    int leniv = 4;
    int i;
    char *pt;

    if ( subrs==NULL )
return;
    if ( (pt=PSDictHasEntry(sf->private,"lenIV"))!=NULL )
	leniv = strtol(pt,NULL,10);
    dumpf(dumpchar,data,"/Subrs %d array\n",subrs->next);
    for ( i=0; i<subrs->next; ++i ) {
	dumpf(dumpchar,data,"dup %d %d RD ", i, subrs->lens[i]+leniv );
	encodestrout(dumpchar,data,subrs->values[i],subrs->lens[i],leniv);
	dumpstr(dumpchar,data," NP\n");
    }
    dumpstr(dumpchar,data,"ND\n");
}

/* Dumped within the private dict to get access to ND and RD */
static int dumpcharstrings(void (*dumpchar)(int ch,void *data), void *data,
	SplineFont *sf, struct pschars *chars ) {
    int leniv = 4;
    int i;
    char *pt;

    if ( (pt=PSDictHasEntry(sf->private,"lenIV"))!=NULL )
	leniv = strtol(pt,NULL,10);
    dumpf(dumpchar,data,"2 index /CharStrings %d dict dup begin\n",chars->cnt);
    for ( i=0; i<chars->next; ++i ) {
	if ( chars->keys[i]==NULL )
    break;
	dumpf(dumpchar,data,"/%s %d RD ", chars->keys[i], chars->lens[i]+leniv );
	encodestrout(dumpchar,data,chars->values[i],chars->lens[i],leniv);
	dumpstr(dumpchar,data," ND\n");
	if ( !ff_progress_next())
return( false );
    }
    dumpstr(dumpchar,data,"end end\nreadonly put\n");
return( true );
}

static void dumpsplineset(void (*dumpchar)(int ch,void *data), void *data,
	SplineSet *spl, int pdfopers, int forceclose, int makeballs,
	int do_clips ) {
    SplinePoint *first, *sp;

    for ( ; spl!=NULL; spl=spl->next ) {
	if ( do_clips ^ spl->is_clip_path )
    continue;
	first = NULL;
	for ( sp = spl->first; ; sp=sp->next->to ) {
	    if ( first==NULL )
		dumpf( dumpchar, data, "\t%g %g %s\n", (double) sp->me.x, (double) sp->me.y,
			pdfopers ? "m" : "moveto" );
	    else if ( sp->prev->knownlinear )
		dumpf( dumpchar, data, "\t %g %g %s\n", (double) sp->me.x, (double) sp->me.y,
			pdfopers ? "l" : "lineto" );
	    else
		dumpf( dumpchar, data, "\t %g %g %g %g %g %g %s\n",
			(double) sp->prev->from->nextcp.x, (double) sp->prev->from->nextcp.y,
			(double) sp->prevcp.x, (double) sp->prevcp.y,
			(double) sp->me.x, (double) sp->me.y,
			pdfopers ? "c" : "curveto" );
	    if ( sp==first )
	break;
	    if ( first==NULL ) first = sp;
	    if ( sp->next==NULL )
	break;
	}
	if ( makeballs && (spl->first->next==NULL || (spl->first->next->to==spl->first)) )
	    dumpstr( dumpchar, data, pdfopers ? "\th\n" : "\tclosepath\n" );
	if ( forceclose || spl->first->prev!=NULL )
	    dumpstr( dumpchar, data, pdfopers ? "\th\n" : "\tclosepath\n" );
    }
}

static int InvertTransform(real inverse[6], real transform[6]) {
    real temp[6], val;
    int i;

    for ( i=0; i<6; ++i ) temp[i] = transform[i];

    inverse[0] = inverse[3] = 1;
    inverse[1] = inverse[2] = inverse[4] = inverse[5] = 0;

    if ( temp[0]==0 && temp[2]==0 )
return( false );		/* Not invertable */
    if ( temp[0]==0 ) {
	val = temp[0]; temp[0] = temp[2]; temp[2] = val;
	val = temp[1]; temp[1] = temp[3]; temp[3] = val;
	inverse[0] = inverse[3] = 0;
	inverse[1] = inverse[2] = 1;
    }
    val = 1/temp[0];
    temp[1] *= val; inverse[0] *= val; inverse[1]*=val;
    val = temp[2];
    temp[3] -= val*temp[1]; inverse[2] -= val*inverse[0]; inverse[3] -= val*inverse[1];
    val = temp[4];
    temp[5] -= val*temp[1]; inverse[4] -= val*inverse[0]; inverse[5] -= val*inverse[1];
    if ( temp[3]==0 )
return( false );
    val = 1/temp[3];
    inverse[2] *= val; inverse[3]*=val;
    val = temp[1];
    inverse[0] -= val*inverse[2]; inverse[1] -= val*inverse[3];
    val = temp[5];
    inverse[4] -= val*inverse[2]; inverse[5] -= val*inverse[3];
return( true );
}

static void dumpGradient(void (*dumpchar)(int ch,void *data), void *data,
	struct gradient *grad, RefChar *ref, SplineChar *sc, int layer, int pdfopers, int isstroke ) {

    if ( pdfopers ) {
	char buffer[200];
	dumpf(dumpchar,data,"/Pattern %s\n", isstroke ? "CS" : "cs" );
	makePatName(buffer,ref,sc,layer,isstroke,true);
	dumpf(dumpchar,data,"/%s %s\n", buffer, isstroke ? "SCN" : "scn" );
	/* PDF output looks much simpler than postscript. It isn't. It's just */
	/*  that the equivalent pdf dictionaries need to be created as objects*/
	/*  and can't live in the content stream, so they are done elsewhere */
    } else {
	dumpf(dumpchar,data, "<<\n  /PatternType 2\n  /Shading <<\n" );
	dumpf(dumpchar,data, "    /ShadingType %d\n", grad->radius==0?2:3 );
	dumpf(dumpchar,data, "    /ColorSpace /DeviceRGB\n" );
	if ( grad->radius==0 ) {
	    dumpf(dumpchar,data, "    /Coords [%g %g %g %g]\n",
		    grad->start.x, grad->start.y, grad->stop.x, grad->stop.y);
	} else {
	    dumpf(dumpchar,data, "    /Coords [%g %g 0 %g %g %g]\n",
		    grad->start.x, grad->start.y, grad->stop.x, grad->stop.y,
		    grad->radius );
	}
	dumpf(dumpchar,data, "    /Extend [true true]\n" );	/* implies pad */
	dumpf(dumpchar,data, "    /Function <<\n" );
	dumpf(dumpchar,data, "      /FunctionType 0\n" );	/* Iterpolation between samples */
	dumpf(dumpchar,data, "      /Domain [%g %g]\n", grad->grad_stops[0].offset,
		grad->grad_stops[grad->stop_cnt-1].offset );
	dumpf(dumpchar,data, "      /Range [0 1.0 0 1.0 0 1.0]\n" );
	dumpf(dumpchar,data, "      /Size [%d]\n", grad->stop_cnt==2?2:101 );
	dumpf(dumpchar,data, "      /BitsPerSample 8\n" );
	dumpf(dumpchar,data, "      /Decode [0 1.0 0 1.0 0 1.0]\n" );
	dumpf(dumpchar,data, "      /DataSource <" );
	if ( grad->stop_cnt==2 ) {
	    unsigned col = grad->grad_stops[0].col;
	    if ( col==COLOR_INHERITED ) col = 0x000000;
	    dumpf(dumpchar,data,"%02x",(col>>16)&0xff);
	    dumpf(dumpchar,data,"%02x",(col>>8 )&0xff);
	    dumpf(dumpchar,data,"%02x",(col    )&0xff);
	    col = grad->grad_stops[1].col;
	    dumpf(dumpchar,data,"%02x",(col>>16)&0xff);
	    dumpf(dumpchar,data,"%02x",(col>>8 )&0xff);
	    dumpf(dumpchar,data,"%02x",(col    )&0xff);
	} else {
	    int i,j;
	    /* Rather than try and figure out the minimum common divisor */
	    /*  off all the offsets, I'll just assume they are all percent*/
	    (dumpchar)('\n',data);
	    for ( i=0; i<=100; ++i ) {
		unsigned col;
		double t = grad->grad_stops[0].offset +
			(grad->grad_stops[grad->stop_cnt-1].offset-
			 grad->grad_stops[0].offset)* i/100.0;
		for ( j=0; j<grad->stop_cnt; ++j )
		    if ( t<=grad->grad_stops[j].offset )
		break;
		if ( j==grad->stop_cnt )
		    col = grad->grad_stops[j-1].col;
		else if ( t==grad->grad_stops[j].offset )
		    col = grad->grad_stops[j].col;
		else {
		    double percent = (t-grad->grad_stops[j-1].offset)/ (grad->grad_stops[j].offset-grad->grad_stops[j-1].offset);
		    uint32 col1 = grad->grad_stops[j-1].col;
		    uint32 col2 = grad->grad_stops[j  ].col;
		    int red, green, blue;
		    if ( col1==COLOR_INHERITED ) col1 = 0x000000;
		    if ( col2==COLOR_INHERITED ) col2 = 0x000000;
		    red   = ((col1>>16)&0xff)*(1-percent) + ((col2>>16)&0xff)*percent;
		    green = ((col1>>8 )&0xff)*(1-percent) + ((col2>>8 )&0xff)*percent;
		    blue  = ((col1    )&0xff)*(1-percent) + ((col2    )&0xff)*percent;
		    col = (red<<16) | (green<<8) | blue;
		}
		if ( col==COLOR_INHERITED ) col = 0x000000;
		dumpf(dumpchar,data,"%02x",(col>>16)&0xff);
		dumpf(dumpchar,data,"%02x",(col>>8 )&0xff);
		dumpf(dumpchar,data,"%02x",(col    )&0xff);
	    }
	}
	dumpf(dumpchar,data,">\n");
	dumpf(dumpchar,data, "    >>\n" );
	dumpf(dumpchar,data, "  >>\n" );
	dumpf(dumpchar,data, ">> matrix makepattern setpattern\n" );
    }
}

static void dumpPattern(void (*dumpchar)(int ch,void *data), void *data,
	struct pattern *pat, RefChar *ref, SplineChar *sc, int layer, int pdfopers, int isstroke ) {
    SplineChar *pattern_sc = SFGetChar(sc->parent,-1,pat->pattern);
    DBounds b;
    real scale[6], result[6];

    if ( pdfopers ) {
	char buffer[200];
	dumpf(dumpchar,data,"/Pattern %s\n", isstroke ? "CS" : "cs" );
	makePatName(buffer,ref,sc,layer,isstroke,false);
	dumpf(dumpchar,data,"/%s %s\n", buffer, isstroke ? "SCN" : "scn" );
	/* PDF output looks much simpler than postscript. It isn't. It's just */
	/*  that the equivalent pdf dictionaries need to be created as objects*/
	/*  and can't live in the content stream, so they are done elsewhere */
    } else {
	if ( pattern_sc==NULL )
	    LogError(_("No glyph named %s, used as a pattern in %s\n"), pat->pattern, sc->name);
	PatternSCBounds(pattern_sc,&b);

	dumpf(dumpchar,data, "<<\n" );
	dumpf(dumpchar,data, "  /PatternType 1\n" );
	dumpf(dumpchar,data, "  /PaintType 1\n" );		/* The intricacies of uncolored tiles are not something into which I wish to delve */
	dumpf(dumpchar,data, "  /TilingType 1\n" );
	dumpf(dumpchar,data, "  /BBox [%g %g %g %g]\n", b.minx, b.miny, b.maxx, b.maxy );
	dumpf(dumpchar,data, "  /XStep %g\n", b.maxx-b.minx );
	dumpf(dumpchar,data, "  /YStep %g\n", b.maxy-b.miny );
	dumpf(dumpchar,data, "  /PaintProc { begin\n" );	/* The begin pops the pattern dictionary off the stack. Don't really use it, but do need the pop */
	SC_PSDump(dumpchar,data, pattern_sc, true, false, ly_all );
	dumpf(dumpchar,data, "  end }\n" );
	memset(scale,0,sizeof(scale));
	scale[0] = pat->width/(b.maxx-b.minx);
	scale[3] = pat->height/(b.maxy-b.miny);
	MatMultiply(scale,pat->transform, result);
	dumpf(dumpchar,data, ">> [%g %g %g %g %g %g] makepattern setpattern\n",
		result[0], result[1], result[2],
		result[3], result[4], result[5]);
    }
}

static void dumpbrush(void (*dumpchar)(int ch,void *data), void *data,
	struct brush *brush, RefChar *ref, SplineChar *sc, int layer, int pdfopers ) {
    if ( brush->gradient!=NULL )
	dumpGradient(dumpchar,data,brush->gradient,ref,sc,layer,pdfopers,false);
    else if ( brush->pattern!=NULL )
	dumpPattern(dumpchar,data,brush->pattern,ref,sc,layer,pdfopers,false);
    else if ( brush->col!=COLOR_INHERITED ) {
	int r, g, b;
	r = (brush->col>>16)&0xff;
	g = (brush->col>>8 )&0xff;
	b = (brush->col    )&0xff;
	if ( r==g && b==g )
	    dumpf(dumpchar,data,(pdfopers ? "%g g\n" : "%g setgray\n"), r/255.0 );
	else
	    dumpf(dumpchar,data,(pdfopers ? "%g %g %g rg\n" : "%g %g %g setrgbcolor\n"),
		    r/255.0, g/255.0, b/255.0 );
	if ( pdfopers && (double)brush->opacity<1.0 && brush->opacity>=0 )
	    dumpf(dumpchar,data,"/gs_fill_opacity_%g gs\n", (double)brush->opacity );
    }
}

/* Grumble. PDF uses different operators for colors for stroke and fill */
static void dumppenbrush(void (*dumpchar)(int ch,void *data), void *data,
	struct brush *brush, RefChar *ref, SplineChar *sc, int layer, int pdfopers ) {
    if ( brush->gradient!=NULL )
	dumpGradient(dumpchar,data,brush->gradient,ref,sc,layer,pdfopers,true);
    else if ( brush->pattern!=NULL )
	dumpPattern(dumpchar,data,brush->pattern,ref,sc,layer,pdfopers,true);
    else if ( brush->col!=COLOR_INHERITED ) {
	int r, g, b;
	r = (brush->col>>16)&0xff;
	g = (brush->col>>8 )&0xff;
	b = (brush->col    )&0xff;
	if ( r==g && b==g )
	    dumpf(dumpchar,data,(pdfopers ? "%g G\n" : "%g setgray\n"), r/255.0 );
	else
	    dumpf(dumpchar,data,(pdfopers ? "%g %g %g RG\n" : "%g %g %g setrgbcolor\n"),
		    r/255.0, g/255.0, b/255.0 );
	if ( pdfopers && (double)brush->opacity<1.0 && brush->opacity>=0 )
	    dumpf(dumpchar,data,"/gs_stroke_opacity_%g gs\n", (double)brush->opacity );
    }
}

static void dumppen(void (*dumpchar)(int ch,void *data), void *data,
	struct pen *pen, RefChar *ref, SplineChar *sc, int layer, int pdfopers) {
    dumppenbrush(dumpchar,data,&pen->brush,ref,sc,layer,pdfopers);

    if ( pen->width!=WIDTH_INHERITED )
	dumpf(dumpchar,data,(pdfopers ? "%g w\n": "%g setlinewidth\n"), (double)pen->width );
    if ( pen->linejoin!=lj_inherited )
	dumpf(dumpchar,data,(pdfopers ? "%d j\n": "%d setlinejoin\n"), pen->linejoin );
    if ( pen->linecap!=lc_inherited )
	dumpf(dumpchar,data,(pdfopers ? "%d J\n": "%d setlinecap\n"), pen->linecap );
    if ( pen->trans[0]!=1.0 || pen->trans[3]!=1.0 || pen->trans[1]!=0 || pen->trans[2]!=0 )
	dumpf(dumpchar,data,(pdfopers ? "%g %g %g %g 0 0 cm\n" : "[%g %g %g %g 0 0] concat\n"),
		(double) pen->trans[0], (double) pen->trans[1], (double) pen->trans[2], (double) pen->trans[3]);
    if ( pen->dashes[0]!=0 || pen->dashes[1]!=DASH_INHERITED ) {
	size_t i;
	dumpchar('[',data);
	for ( i=0; i<sizeof(pen->dashes)/sizeof(pen->dashes[0]) && pen->dashes[i]!=0; ++i )
	    dumpf(dumpchar,data,"%d ", pen->dashes[i]);
	dumpf(dumpchar,data,pdfopers ? "] 0 d\n" : "] 0 setdash\n");
    }
}

struct psfilter {
    int ascii85encode, ascii85n, ascii85bytes_per_line;
    void (*dumpchar)(int ch,void *data);
    void *data;
};

static void InitFilter(struct psfilter *ps,void (*dumpchar)(int ch,void *data), void *data) {
    ps->ascii85encode = 0;
    ps->ascii85n = 0;
    ps->ascii85bytes_per_line = 0;
    ps->dumpchar = dumpchar;
    ps->data = data;
}

static void Filter(struct psfilter *ps,uint8 ch) {
    ps->ascii85encode = (ps->ascii85encode<<8) | ch;
    if ( ++ps->ascii85n == 4 ) {
	int ch5, ch4, ch3, ch2, ch1;
	uint32 val = ps->ascii85encode;
	if ( val==0 ) {
	    (ps->dumpchar)('z',ps->data);
	    ps->ascii85n = 0;
	    if ( ++ps->ascii85bytes_per_line >= 76 ) {
		(ps->dumpchar)('\n',ps->data);
		ps->ascii85bytes_per_line = 0;
	    }
	} else {
	    ch5 = val%85; val /= 85;
	    ch4 = val%85; val /= 85;
	    ch3 = val%85; val /= 85;
	    ch2 = val%85;
	    ch1 = val/85;
	    dumpf(ps->dumpchar, ps->data, "%c%c%c%c%c",
		    ch1+'!', ch2+'!', ch3+'!', ch4+'!', ch5+'!' );
	    ps->ascii85encode = 0;
	    ps->ascii85n = 0;
	    if (( ps->ascii85bytes_per_line+=5) >= 80 ) {
		(ps->dumpchar)('\n',ps->data);
		ps->ascii85bytes_per_line = 0;
	    }
	}
    }
}

static void FlushFilter(struct psfilter *ps) {
    uint32 val = ps->ascii85encode;
    int n = ps->ascii85n;
    if ( n!=0 ) {
	int ch4, ch3, ch2, ch1;
	while ( n++<4 )
	    val<<=8;
	/* ch5 = val%85; */ val /= 85;
	ch4 = val%85; val /= 85;
	ch3 = val%85; val /= 85;
	ch2 = val%85;
	ch1 = val/85;
	(ps->dumpchar)(ch1+'!',ps->data);
	(ps->dumpchar)(ch2+'!',ps->data);
	if ( ps->ascii85n>=2 )
	    (ps->dumpchar)(ch3+'!',ps->data);
	if ( ps->ascii85n>=3 )
	    (ps->dumpchar)(ch4+'!',ps->data);
    }
    (ps->dumpchar)('~',ps->data);
    (ps->dumpchar)('>',ps->data);
    (ps->dumpchar)('\n',ps->data);
}

/* Inside a font, I can't use a <stdin> as a data source. Probably because */
/*  the parser doesn't know what to do with those data when building the char */
/*  proc (as opposed to executing) */
/* So I can't use run length filters or other compression technique */

static void FilterStr(struct psfilter *ps,uint8 *pt, int len ) {
    uint8 *end = pt+len;

    while ( pt<end )
	Filter(ps,*pt++);
}

static void PSDumpBinaryData(void (*dumpchar)(int ch,void *data), void *data,
	uint8 *bytes,int rows, int bytes_per_row, int useful_bytes_per_row) {
    struct psfilter ps;
    int i,j,cnt,group_cnt;
    const int max_string = 65536;

    if ( useful_bytes_per_row*rows<max_string ) {
	/* It all fits in one string. Easy peasy */
	dumpf(dumpchar,data, "{<~" );
	InitFilter(&ps,dumpchar,data);
	if (bytes_per_row==useful_bytes_per_row )
	    FilterStr(&ps,(uint8 *) bytes, rows*bytes_per_row);
	else for ( i=0; i<rows; ++i ) {
	    FilterStr(&ps,(uint8 *) (bytes + i*bytes_per_row),
		    useful_bytes_per_row);
	}
	FlushFilter(&ps);
	dumpchar('}',data);
    } else {
	cnt = (max_string-1)/useful_bytes_per_row;
	if ( cnt==0 ) cnt=1;
	group_cnt = -1;
	for ( i=0; i<rows; ) {
	    if ( i+cnt>=rows )
		dumpf(dumpchar,data, "{currentdict /ff-image-cnt undef <~" );
	    else {
		dumpf(dumpchar,data, "{{/ff-image-cnt %d def <~", i/cnt );
		group_cnt = i/cnt;
	    }
	    InitFilter(&ps,dumpchar,data);
	    for ( j=0; j<cnt && i<rows; ++i, ++j ) {
		FilterStr(&ps,(uint8 *) (bytes + i*bytes_per_row),
			useful_bytes_per_row);
	    }
	    FlushFilter(&ps);
	    dumpf(dumpchar,data,"}\n");
	}
	for ( i=group_cnt-1; i>=0; --i ) {
	    dumpf(dumpchar,data,"ff-image-cnt %d eq 3 1 roll ifelse}\n", i );
	}
	dumpf(dumpchar,data,"currentdict /ff-image-cnt known not 3 1 roll ifelse}\n" );
    }
}

static void PSDump24BinaryData(void (*dumpchar)(int ch,void *data), void *data,
	struct _GImage *base ) {
    struct psfilter ps;
    int i,j,cnt,group_cnt;
    register uint32 val;
    register uint32 *pt, *end;
    const int max_string = 65536;

    if ( 3*base->width*base->height<max_string ) {
	/* It all fits in one string. Easy peasy */
	dumpf(dumpchar,data, "{<~" );
	InitFilter(&ps,dumpchar,data);
	for ( i=0; i<base->height; ++i ) {
	    pt = (uint32 *) (base->data + i*base->bytes_per_line);
	    end = pt + base->width;
	    while ( pt<end ) {
		val = *pt++;
		Filter(&ps,COLOR_RED(val));
		Filter(&ps,COLOR_GREEN(val));
		Filter(&ps,COLOR_BLUE(val));
	    }
	}
	FlushFilter(&ps);
	dumpchar('}',data);
    } else {
	cnt = (max_string-1)/(3*base->width);
	if ( cnt==0 ) cnt=1;
	group_cnt = -1;
	for ( i=0; i<base->height; ) {
	    if ( i+cnt>=base->height )
		dumpf(dumpchar,data, "{currentdict /ff-image-cnt undef <~" );
	    else {
		dumpf(dumpchar,data, "{{/ff-image-cnt %d def <~", i/cnt );
		group_cnt = i/cnt;
	    }
	    InitFilter(&ps,dumpchar,data);
	    for ( j=0; j<cnt && i<base->height; ++i, ++j ) {
		pt = (uint32 *) (base->data + i*base->bytes_per_line);
		end = pt + base->width;
		while ( pt<end ) {
		    val = *pt++;
		    Filter(&ps,COLOR_RED(val));
		    Filter(&ps,COLOR_GREEN(val));
		    Filter(&ps,COLOR_BLUE(val));
		}
	    }
	    FlushFilter(&ps);
	    dumpf(dumpchar,data,"}\n");
	}
	for ( i=group_cnt-1; i>=0; --i ) {
	    dumpf(dumpchar,data,"ff-image-cnt %d eq 3 1 roll ifelse}\n", i );
	}
	dumpf(dumpchar,data,"currentdict /ff-image-cnt known not 3 1 roll ifelse}\n" );
    }
}

static void PSDrawMonoImg(void (*dumpchar)(int ch,void *data), void *data,
	struct _GImage *base,int use_imagemask) {

    dumpf(dumpchar,data, " %d %d ", base->width, base->height );
    if ( base->trans==1 )
	dumpf(dumpchar,data, "false ");
    else
	dumpf(dumpchar,data, "true ");
    dumpf(dumpchar,data, "[%d 0 0 %d 0 %d]\n",
	    base->width, -base->height, base->height);
    PSDumpBinaryData(dumpchar,data,(uint8 *) base->data,base->height,base->bytes_per_line,(base->width+7)/8);

    dumpf(dumpchar,data, "%s\n",
	    use_imagemask?"imagemask":"image" );
}

static void PSSetIndexColors(void (*dumpchar)(int ch,void *data), void *data,
	GClut *clut) {
    int i;

    dumpf(dumpchar,data, "[/Indexed /DeviceRGB %d <\n", clut->clut_len-1 );
    for ( i=0; i<clut->clut_len; ++i )
	dumpf(dumpchar,data, "%02X%02X%02X%s", COLOR_RED(clut->clut[i]),
		COLOR_GREEN(clut->clut[i]), COLOR_BLUE(clut->clut[i]),
		i%11==10?"\n":" ");
    dumpf(dumpchar,data,">\n] setcolorspace\n");
}

static void PSBuildImageIndexDict(void (*dumpchar)(int ch,void *data), void *data,
	struct _GImage *base) {
    /* I need an image dict, otherwise I am restricted to grey scale */
    dumpf(dumpchar,data, "<<\n" );
    dumpf(dumpchar,data, "  /ImageType 1\n" );
    dumpf(dumpchar,data, "  /Width %d\n", base->width );
    dumpf(dumpchar,data, "  /Height %d\n", base->height );
    dumpf(dumpchar,data, "  /ImageMatrix [%d 0 0 %d 0 %d]\n",
	    base->width, -base->height, base->height);
    dumpf(dumpchar,data, "  /MultipleDataSources false\n" );
    dumpf(dumpchar,data, "  /BitsPerComponent 8\n" );
    dumpf(dumpchar,data, "  /Decode [0 255]\n" );
    dumpf(dumpchar,data, "  /Interpolate false\n" );
    dumpf(dumpchar,data, "  /DataSource " );
    PSDumpBinaryData(dumpchar,data,base->data,base->height,base->bytes_per_line,
	    base->width);
    dumpf(dumpchar,data, ">> image\n" );
}

static void PSDrawImg(void (*dumpchar)(int ch,void *data), void *data,
	struct _GImage *base) {

    if ( base->image_type == it_index ) {
	PSSetIndexColors(dumpchar,data,base->clut);
	PSBuildImageIndexDict(dumpchar,data,base);
	dumpf(dumpchar,data, "[/DeviceRGB] setcolorspace\n" );
    } else {
	dumpf(dumpchar,data, "%d %d 8 [%d 0 0 %d 0 %d] ",
		base->width, base->height,  base->width, -base->height, base->height);
	PSDump24BinaryData(dumpchar,data,base);
	dumpf(dumpchar,data, "false 3 colorimage\n" );
    }
}

static void dumpimage(void (*dumpchar)(int ch,void *data), void *data,
	ImageList *imgl, int use_imagemask, int pdfopers,
	int layer, int icnt, SplineChar *sc ) {
    GImage *image = imgl->image;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];

    if ( pdfopers ) {
	dumpf( dumpchar, data, "  q 1 0 0 1 %g %g cm %g 0 0 %g 0 0 cm\n",
		(double) imgl->xoff, (double) (imgl->yoff-imgl->yscale*base->height),
		(double) (imgl->xscale*base->width), (double) (imgl->yscale*base->height) );
	dumpf( dumpchar, data, "/%s_ly%d_%d_image", sc->name, layer, icnt );
	dumpf( dumpchar, data, " Do " );	/* I think I use this for imagemasks too... */
	dumpstr(dumpchar,data,"Q\n");
    } else {
	dumpf( dumpchar, data, "  gsave %g %g translate %g %g scale\n",
		(double) imgl->xoff, (double) (imgl->yoff-imgl->yscale*base->height),
		(double) (imgl->xscale*base->width), (double) (imgl->yscale*base->height) );
	if ( base->image_type==it_mono ) {
	    PSDrawMonoImg(dumpchar,data,base,use_imagemask);
	} else {
	    /* Just draw the image, ignore the complexity of transparent images */
	    PSDrawImg(dumpchar,data,base);
	}
	dumpstr(dumpchar,data,"grestore\n");
    }
}

void SC_PSDump(void (*dumpchar)(int ch,void *data), void *data,
	SplineChar *sc, int refs_to_splines, int pdfopers, int layer ) {
    RefChar *ref;
    real inverse[6];
    int i, last, first;
    SplineSet *temp;

    if ( sc==NULL )
return;
    last = first = layer;
    if ( layer==ly_all )
	first = last = ly_fore;
    if ( sc->parent->multilayer ) {
	first = ly_fore;
	last = sc->layer_cnt-1;
    }
    for ( i=first; i<=last; ++i ) {
	if ( sc->layers[i].splines!=NULL ) {
	    temp = sc->layers[i].splines;
	    if ( sc->layers[i].order2 ) temp = SplineSetsPSApprox(temp);
	    if ( sc->parent->multilayer ) {
		dumpstr(dumpchar,data,pdfopers ? "q " : "gsave " );
		if ( SSHasClip(temp)) {
		    dumpsplineset(dumpchar,data,temp,pdfopers,true, false, true);
		    dumpstr(dumpchar,data,pdfopers ? "W n " : "clip newpath\n" );
		}
		dumpsplineset(dumpchar,data,temp,pdfopers,sc->layers[i].dofill,
			sc->layers[i].dostroke && sc->layers[i].stroke_pen.linecap==lc_round,
			false);
		if ( sc->layers[i].dofill && sc->layers[i].dostroke ) {
		    if ( pdfopers ) {
			dumpbrush(dumpchar,data, &sc->layers[i].fill_brush,NULL,sc,i,pdfopers);
			dumppen(dumpchar,data, &sc->layers[i].stroke_pen,NULL,sc,i,pdfopers);
			dumpstr(dumpchar,data, "B " );
		    } else if ( sc->layers[i].fillfirst ) {
			dumpstr(dumpchar,data, "gsave " );
			dumpbrush(dumpchar,data, &sc->layers[i].fill_brush,NULL,sc,i,pdfopers);
			dumpstr(dumpchar,data,"fill grestore " );
			dumppen(dumpchar,data, &sc->layers[i].stroke_pen,NULL,sc,i,pdfopers);
			dumpstr(dumpchar,data,"stroke " );
		    } else {
			dumpstr(dumpchar,data, "gsave " );
			dumppen(dumpchar,data, &sc->layers[i].stroke_pen,NULL,sc,i,pdfopers);
			dumpstr(dumpchar,data,"stroke grestore " );
			dumpbrush(dumpchar,data, &sc->layers[i].fill_brush,NULL,sc,i,pdfopers);
			dumpstr(dumpchar,data,"fill " );
		    }
		} else if ( sc->layers[i].dofill ) {
		    dumpbrush(dumpchar,data, &sc->layers[i].fill_brush,NULL,sc,i,pdfopers);
		    dumpstr(dumpchar,data,pdfopers ? "f ": "fill " );
		} else if ( sc->layers[i].dostroke ) {
		    dumppen(dumpchar,data, &sc->layers[i].stroke_pen,NULL,sc,i,pdfopers);
		    dumpstr(dumpchar,data, pdfopers ? "S ": "stroke " );
		}
		dumpstr(dumpchar,data,pdfopers ? "Q\n" : "grestore\n" );
	    } else
		dumpsplineset(dumpchar,data,temp,pdfopers,!sc->parent->strokedfont,false,
			false);
	    if ( sc->layers[i].order2 ) SplinePointListsFree(temp);
	}
	if ( sc->layers[i].refs!=NULL ) {
	    if ( sc->parent->multilayer ) {
		dumpstr(dumpchar,data,pdfopers ? "q " : "gsave " );
		if ( sc->layers[i].dofill )
		    dumpbrush(dumpchar,data, &sc->layers[i].fill_brush, NULL, sc, i, pdfopers);
	    }
	    if ( refs_to_splines ) {
		if ( !pdfopers || !sc->parent->multilayer ) {
		    /* In PostScript, patterns are transformed by the page's */
		    /*  transformation matrix. In PDF they are not. */
		    /* Of course if we have no patterns we can still use this code */
		    for ( ref = sc->layers[i].refs; ref!=NULL; ref=ref->next ) {
			dumpstr(dumpchar,data,pdfopers ? "q " : "gsave " );
			if ( !MatIsIdentity(ref->transform) ) {
			    dumpf(dumpchar,data,pdfopers ? "%g %g %g %g %g %g cm " : "[%g %g %g %g %g %g] concat ",
				    (double) ref->transform[0], (double) ref->transform[1], (double) ref->transform[2],
				    (double) ref->transform[3], (double) ref->transform[4], (double) ref->transform[5]);
			}
			SC_PSDump(dumpchar,data,ref->sc,refs_to_splines,pdfopers,ly_all);
			dumpstr(dumpchar,data,pdfopers ? "Q\n" : "grestore\n" );
		    }
		} else {
		    /* If we get here we are outputting pdf, are type3, refs_2_splines */
/*  We need different gradients and patterns for different transform */
/*  matrices of references */
		    int j;
		    for ( ref = sc->layers[i].refs; ref!=NULL; ref=ref->next ) {
			for ( j=0; j<ref->layer_cnt; ++j ) {
			    temp = ref->layers[j].splines;
			    /*if ( sc->layers[i].order2 )
				temp = SplineSetsPSApprox(temp);*/
			    dumpstr(dumpchar,data,pdfopers ? "q" : "gsave " );
			    dumpsplineset(dumpchar,data,temp,pdfopers,ref->layers[j].dofill,
				    ref->layers[j].dostroke && ref->layers[j].stroke_pen.linecap==lc_round,
			    false);
			    if ( ref->layers[j].dofill && ref->layers[j].dostroke ) {
				dumpbrush(dumpchar,data, &ref->layers[j].fill_brush,ref,ref->sc,j,pdfopers);
				dumppen(dumpchar,data, &ref->layers[j].stroke_pen,ref,ref->sc,j,pdfopers);
				dumpstr(dumpchar,data, "B " );
			    } else if ( ref->layers[j].dofill ) {
				dumpbrush(dumpchar,data, &ref->layers[j].fill_brush,ref,ref->sc,j,pdfopers);
				dumpstr(dumpchar,data,pdfopers ? "f ": "fill " );
			    } else if ( ref->layers[j].dostroke ) {
				dumppen(dumpchar,data, &ref->layers[j].stroke_pen,ref,ref->sc,j,pdfopers);
				dumpstr(dumpchar,data, pdfopers ? "S ": "stroke " );
			    }
			    dumpstr(dumpchar,data,pdfopers ? "Q\n" : "grestore\n" );
			    /* if ( sc->layers[layer].order2 )
				SplinePointListsFree(temp);*/
			}
		    }
		}
	    } else {
		dumpstr(dumpchar,data,"    pop -1\n" );
		for ( ref = sc->layers[i].refs; ref!=NULL; ref=ref->next ) {
		    if ( ref->transform[0]!=1 || ref->transform[1]!=0 || ref->transform[2]!=0 ||
			    ref->transform[3]!=1 || ref->transform[4]!=0 || ref->transform[5]!=0 ) {
			if ( InvertTransform(inverse,ref->transform)) {
			    if ( ref->transform[0]!=1 || ref->transform[1]!=0 ||
				    ref->transform[2]!=0 || ref->transform[3]!=1 )
				dumpf(dumpchar,data, "    [ %g %g %g %g %g %g ] concat ",
				    (double) ref->transform[0], (double) ref->transform[1], (double) ref->transform[2],
				    (double) ref->transform[3], (double) ref->transform[4], (double) ref->transform[5]);
			    else
				dumpf(dumpchar,data, "    %g %g translate ",
				    (double) ref->transform[4], (double) ref->transform[5]);
			    dumpf(dumpchar,data, "1 index /CharProcs get /%s get exec ",
				ref->sc->name );
			    if ( inverse[0]!=1 || inverse[1]!=0 ||
				    inverse[2]!=0 || inverse[3]!=1 )
				dumpf(dumpchar,data, "[ %g %g %g %g %g %g ] concat \n",
				    (double) inverse[0], (double) inverse[1], (double) inverse[2], (double) inverse[3], (double) inverse[4], (double) inverse[5]
				    );
			    else
				dumpf(dumpchar,data, "%g %g translate\n",
				    (double) inverse[4], (double) inverse[5] );
			}
		    } else
			dumpf(dumpchar,data, "    1 index /CharProcs get /%s get exec\n", ref->sc->name );
		}
	    }
	    if ( sc->parent->multilayer )
		dumpstr(dumpchar,data,pdfopers ? "Q\n" : "grestore\n" );
	}
	if ( sc->layers[i].images!=NULL  ) { ImageList *img; int icnt=0;
	    dumpstr(dumpchar,data,pdfopers ? "q\n" : "gsave\n" );
	    if ( sc->layers[i].dofill )
		dumpbrush(dumpchar,data, &sc->layers[i].fill_brush,NULL,sc,i,pdfopers);
	    for ( img = sc->layers[i].images; img!=NULL; img=img->next, ++icnt )
		dumpimage(dumpchar,data,img,sc->layers[i].dofill,pdfopers,i,icnt,sc);
	    dumpstr(dumpchar,data,pdfopers ? "Q\n" : "grestore\n" );
	}
    }
}

static int SCSetsColor(SplineChar *sc) {
    int l;
    RefChar *r;
    ImageList *img;

    for ( l=ly_fore ; l<sc->layer_cnt; ++l ) {
	if ( sc->layers[l].fill_brush.col != COLOR_INHERITED )
return( true );
	if ( sc->layers[l].fill_brush.gradient != NULL || sc->layers[l].fill_brush.pattern != NULL )
return( true );
	if ( sc->layers[l].stroke_pen.brush.col != COLOR_INHERITED )
return( true );
	if ( sc->layers[l].stroke_pen.brush.gradient != NULL || sc->layers[l].stroke_pen.brush.pattern != NULL )
return( true );
	for ( img = sc->layers[l].images; img!=NULL; img=img->next ) {
	    GImage *image = img->image;
	    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
	    if ( base->image_type!=it_mono )
return( true );
	    if ( !sc->layers[l].dofill )
return( true );
	}
	for ( r = sc->layers[l].refs; r!=NULL; r = r->next )
	    if ( SCSetsColor(r->sc) )
return( true );
    }
return( false );
}

static void dumpproc(void (*dumpchar)(int ch,void *data), void *data, SplineChar *sc ) {
    DBounds b;

    SplineCharFindBounds(sc,&b);
    dumpf(dumpchar,data,"  /%s { ",sc->name);
    if ( sc->dependents!=NULL )
	dumpstr(dumpchar,data,"dup -1 ne { ");
    if ( !SCSetsColor(sc) ) {
	dumpf(dumpchar,data,"%d 0 %d %d %d %d setcachedevice",
		(int) sc->width, (int) floor(b.minx), (int) floor(b.miny),
		(int) ceil(b.maxx), (int) ceil(b.maxy) );
    } else {
	/* can't cache it if we set colour/grey within */
	dumpf(dumpchar,data,"%d 0 setcharwidth",
		(int) sc->width );
    }
    if ( sc->dependents!=NULL )
	dumpstr(dumpchar,data," } if\n");
    else
	dumpstr(dumpchar,data,"\n");
    SC_PSDump(dumpchar,data,sc,false,false,ly_all);
    dumpstr(dumpchar,data,"  } bind def\n" );
}

static int dumpcharprocs(void (*dumpchar)(int ch,void *data), void *data, SplineFont *sf ) {
    /* for type 3 fonts */
    int cnt, i, notdefpos = -1;

    cnt = 0;
    notdefpos = SFFindNotdef(sf,-2);
    for ( i=0; i<sf->glyphcnt; ++i )
	if ( SCWorthOutputting(sf->glyphs[i]) ) {
	    if ( strcmp(sf->glyphs[i]->name,".notdef")!=0 )
		++cnt;
	}
    ++cnt;		/* one notdef entry */

    dumpf(dumpchar,data,"/CharProcs %d dict def\nCharProcs begin\n", cnt );
    i = 0;
    if ( notdefpos!=-1 )
	dumpproc(dumpchar,data,sf->glyphs[notdefpos]);
    else {
	dumpf(dumpchar, data, "  /.notdef { %d 0 0 0 0 0 setcachedevice } bind def\n",
	    sf->ascent+sf->descent );
	if ( sf->glyphs[0]!=NULL && strcmp(sf->glyphs[0]->name,".notdef")==0 )
	    ++i;
    }
    for ( ; i<sf->glyphcnt; ++i ) if ( i!=notdefpos ) {
	if ( SCWorthOutputting(sf->glyphs[i]) )
	    dumpproc(dumpchar,data,sf->glyphs[i]);
	if ( !ff_progress_next())
return( false );
    }
    dumpstr(dumpchar,data,"end\ncurrentdict end\n" );
    dumpf(dumpchar, data, "/%s exch definefont\n", sf->fontname );
return( true );
}

extern const uint8 *const subrs[10];
extern const int subrslens[10];
static struct pschars *initsubrs(MMSet *mm) {
    int i;
    struct pschars *sub;

    sub = calloc(1,sizeof(struct pschars));
    sub->cnt = 10;
    sub->lens = malloc(10*sizeof(int));
    sub->values = malloc(10*sizeof(uint8 *));
    for ( i=0; i<5; ++i ) {
	++sub->next;
	sub->values[i] = (uint8 *) copyn((const char *) subrs[i],subrslens[i]);
	sub->lens[i] = subrslens[i];
    }
    sub->next = 5;
    if ( mm!=NULL ) {
	static int cnts[] = { 1,2,3,4,6 };
	for ( ; i<10 && cnts[i-5]*mm->instance_count<22; ++i ) {
	    ++sub->next;
	    sub->values[i] = (uint8 *) copyn((const char *) subrs[i],subrslens[i]);
	    sub->values[i][0] += cnts[i-5]*mm->instance_count;
	    sub->lens[i] = subrslens[i];
	}
	sub->next = 10;
    }
return( sub );
}

extern const char **othersubrs_copyright[];
extern const char **othersubrs[];
extern const char *cid_othersubrs[];
static void dumpothersubrs(void (*dumpchar)(int ch,void *data), void *data,
	int incid, int needsflex, int needscounters, MMSet *mm ) {
    int i,j;

    dumpstr(dumpchar,data,"/OtherSubrs \n" );
    if ( incid ) {
	for ( i=0; cid_othersubrs[i]!=NULL; ++i ) {
	    dumpstr(dumpchar,data,cid_othersubrs[i]);
	    dumpchar('\n',data);
	}
    } else {
	int max_subr, min_subr;

	/* I assume I always want the hint replacement subr, (it's small) */
	/*  but the flex subrs are large, and if I can omit them, I shall */
	if ( needsflex ) {
	    min_subr = 0;
	    max_subr = 3;
	} else {
	    min_subr = 3;
	    max_subr = 3;
	}
	if ( needscounters )
	    max_subr = 13;
	for ( i=0; othersubrs_copyright[0][i]!=NULL; ++i ) {
	    dumpstr(dumpchar,data,othersubrs_copyright[0][i]);
	    dumpchar('\n',data);
	}
	dumpstr(dumpchar,data,"[ ");	/* start array */
	for ( j=0; j<min_subr; ++j )
	    dumpstr(dumpchar,data," {}\n");
	for ( ; j<=max_subr; ++j )
	    for ( i=0; othersubrs[j][i]!=NULL; ++i ) {
		dumpstr(dumpchar,data,othersubrs[j][i]);
		dumpchar('\n',data);
	    }
        if ( mm!=NULL ) {
	    /* MM other subrs begin at 14, so skip anything up till then */
	    for ( ; j<=13; ++j )
		dumpstr(dumpchar,data," {}\n");
	}
	if ( mm!=NULL ) {
	    /* the code for the multiple master subroutines depends on */
	    /*  the number of font instances, so we can't just blithely copy */
	    /*  an example from Adobe (and they don't provide one anyway) */
	    dumpf(dumpchar, data, "{ %d 1 roll $Blend } bind\n", mm->instance_count );
	    if ( 2*mm->instance_count<22 )
		dumpf(dumpchar, data, "{ exch %d %d roll $Blend exch %d 2 roll $Blend } bind\n",
		    2*mm->instance_count, 1-mm->instance_count,
		    mm->instance_count+1);
	    if ( 3*mm->instance_count<22 )
		dumpf(dumpchar, data, "{ 3 -1 roll %d %d roll $Blend 3 -1 roll %d %d roll $Blend 3 -1 roll %d 2 roll $Blend } bind\n",
		    3*mm->instance_count, 1-mm->instance_count,
		    2*mm->instance_count+1, 1-mm->instance_count,
		    mm->instance_count+2);
	    if ( 4*mm->instance_count<22 )
		dumpf(dumpchar, data, "{ 4 -1 roll %d %d roll $Blend 4 -1 roll %d %d roll $Blend 4 -1 roll %d %d roll $Blend 4 -1 roll %d 3 roll $Blend } bind\n",
		    4*mm->instance_count, 1-mm->instance_count,
		    3*mm->instance_count+1, 1-mm->instance_count,
		    2*mm->instance_count+2, 1-mm->instance_count,
		    mm->instance_count+3);
	    if ( 6*mm->instance_count<22 )
		dumpf(dumpchar, data, "{ 6 -1 roll %d %d roll $Blend 6 -1 roll %d %d roll $Blend 6 -1 roll %d %d roll $Blend 6 -1 roll %d %d roll $Blend 6 -1 roll %d %d roll $Blend 6 -1 roll %d 5 roll $Blend } bind\n",
		    6*mm->instance_count, 1-mm->instance_count,
		    5*mm->instance_count+1, 1-mm->instance_count,
		    4*mm->instance_count+2, 1-mm->instance_count,
		    3*mm->instance_count+3, 1-mm->instance_count,
		    2*mm->instance_count+4, 1-mm->instance_count,
		    mm->instance_count+5);
	}
	dumpstr(dumpchar,data,"] ");	/* End array */
    }
    dumpstr(dumpchar,data,incid?"def\n":"ND\n" );
}

static char *dumptospace(void (*dumpchar)(int ch,void *data), void *data,
	char *str) {

    while ( *str!=' ' && *str!=']' && *str!='\0' )
	dumpchar(*str++,data);
return( str );
}

static void dumpmmprivatearr(void (*dumpchar)(int ch,void *data), void *data,
	char *privates[16], int instance_count) {
    int j;

    for ( j=0; j<instance_count; ++j )
	while ( *privates[j]==' ' ) ++privates[j];

    dumpchar('[',data);
    if ( *privates[0]=='[' ) {
	/* It's an array */
	for ( j=0; j<instance_count; ++j )
	    ++privates[j];
	while ( *privates[0]!=']' && *privates[0]!='\0' ) {
	    for ( j=0; j<instance_count; ++j )
		while ( *privates[j]==' ' ) ++privates[j];
	    if ( *privates[0]==']' || *privates[0]=='\0' )
	break;
	    dumpchar('[',data);
	    privates[0] = dumptospace(dumpchar,data,privates[0]);
	    for ( j=1; j<instance_count; ++j ) {
		dumpchar(' ',data);
		privates[j] = dumptospace(dumpchar,data,privates[j]);
	    }
	    dumpchar(']',data);
	}
    } else {
	/* It's not an array */
	dumpstr(dumpchar,data,privates[0]);
	for ( j=1; j<instance_count; ++j ) {
	    dumpchar(' ',data);
	    dumpstr(dumpchar,data,privates[j]);
	}
    }
    dumpchar(']',data);
}

static void dumpmmprivate(void (*dumpchar)(int ch,void *data), void *data,MMSet *mm) {
    char *privates[16];
    int j,k, missing, allsame;
    struct psdict *private = mm->instances[0]->private;

    if ( private==NULL )
return;

    dumpstr(dumpchar,data,"3 index /Blend get /Private get begin\n");
    for ( k=0; k<private->next; ++k ) {
	privates[0] = private->values[k];
	missing = false; allsame = true;
	for ( j=1; j<mm->instance_count; ++j ) {
	    privates[j] = PSDictHasEntry(mm->instances[j]->private,private->keys[k]);
	    if ( privates[j]==NULL ) {
		missing = true;
	break;
	    } else if ( strcmp(privates[j],privates[0])!=0 )
		allsame = false;
	}
	if ( missing || allsame )
    continue;
	dumpf(dumpchar,data," /%s ", private->keys[k]);
	dumpmmprivatearr(dumpchar,data,privates,mm->instance_count);
	dumpstr(dumpchar,data, " def\n" );
    }
    dumpstr(dumpchar,data,"end\n");
}

static double FindMaxDiffOfBlues(char *pt, double max_diff) {
    char *end;
    double p1, p2;

    while ( *pt==' ' || *pt=='[' ) ++pt;
    for (;;) {
	p1 = strtod(pt,&end);
	if ( end==pt )
    break;
	pt = end;
	p2 = strtod(pt,&end);
	if ( end==pt )
    break;
	if ( p2-p1 >max_diff ) max_diff = p2-p1;
	pt = end;
    }
return( max_diff );
}

double BlueScaleFigureForced(struct psdict *private_,real bluevalues[], real otherblues[]) {
    double max_diff=0;
    char *pt;
    int i;

    pt = PSDictHasEntry(private_,"BlueValues");
    if ( pt!=NULL ) {
	max_diff = FindMaxDiffOfBlues(pt,max_diff);
    } else if ( bluevalues!=NULL ) {
	for ( i=0; i<14 && (bluevalues[i]!=0 || bluevalues[i+1])!=0; i+=2 ) {
	    if ( bluevalues[i+1] - bluevalues[i]>=max_diff )
		max_diff = bluevalues[i+1] - bluevalues[i];
	}
    }
    pt = PSDictHasEntry(private_,"FamilyBlues");
    if ( pt!=NULL )
	max_diff = FindMaxDiffOfBlues(pt,max_diff);

    pt = PSDictHasEntry(private_,"OtherBlues");
    if ( pt!=NULL )
	max_diff = FindMaxDiffOfBlues(pt,max_diff);
    else if ( otherblues!=NULL ) {
	for ( i=0; i<10 && (otherblues[i]!=0 || otherblues[i+1]!=0); i+=2 ) {
	    if ( otherblues[i+1] - otherblues[i]>=max_diff )
		max_diff = otherblues[i+1] - otherblues[i];
	}
    }
    pt = PSDictHasEntry(private_,"FamilyOtherBlues");
    if ( pt!=NULL )
	max_diff = FindMaxDiffOfBlues(pt,max_diff);
    if ( max_diff<=0 )
return( -1 );
    if ( 1/max_diff > .039625 )
return( -1 );

    return rint(240.0*0.99/max_diff)/240.0;
}

double BlueScaleFigure(struct psdict *private_,real bluevalues[], real otherblues[]) {
    if ( PSDictHasEntry(private_,"BlueScale")!=NULL )
return( -1 );
    return BlueScaleFigureForced(private_, bluevalues, otherblues);
}

static int dumpprivatestuff(void (*dumpchar)(int ch,void *data), void *data,
	SplineFont *sf, struct fddata *incid, int flags, enum fontformat format,
	EncMap *map, int layer ) {
    int cnt, mi;
    real bluevalues[14], otherblues[10];
    real snapcnt[12];
    real stemsnaph[12], stemsnapv[12];
    real stdhw[1], stdvw[1];
    int flex_max;
    int i;
    int hasblue=0, hash=0, hasv=0, hasshift/*, hasxuid*/, hasbold, haslg;
    int isbold=false;
    int iscjk;
    struct pschars *subrs = NULL, *chars = NULL;
    const char *ND="def";
    MMSet *mm = (format==ff_mma || format==ff_mmb)? sf->mm : NULL;
    double bluescale;

    if ( incid==NULL ) {
	flex_max = SplineFontIsFlexible(sf,layer,flags);
	if ( (subrs = initsubrs(mm))==NULL )
return( false );
	iscjk = SFIsCJK(sf,map);
    } else {
	flex_max = incid->flexmax;
	iscjk = incid->iscjk;
    }

    hasbold = PSDictHasEntry(sf->private,"ForceBold")!=NULL;
    hasblue = PSDictHasEntry(sf->private,"BlueValues")!=NULL;
    hash = PSDictHasEntry(sf->private,"StdHW")!=NULL;
    hasv = PSDictHasEntry(sf->private,"StdVW")!=NULL;
    hasshift = PSDictHasEntry(sf->private,"BlueShift")!=NULL;
    /*hasxuid = PSDictHasEntry(sf->private,"XUID")!=NULL;*/
    haslg = PSDictHasEntry(sf->private,"LanguageGroup")!=NULL;
    if ( sf->weight!=NULL &&
	    (strstrmatch(sf->weight,"Bold")!=NULL ||
	     strstrmatch(sf->weight,"Heavy")!=NULL ||
	     strstrmatch(sf->weight,"Black")!=NULL ||
	     strstrmatch(sf->weight,"Grass")!=NULL ||
	     strstrmatch(sf->weight,"Fett")!=NULL))
	isbold = true;
    if ( sf->pfminfo.pfmset && sf->pfminfo.weight>=700 )
	isbold = true;

    ff_progress_change_stages(2+2-hasblue);
    if ( autohint_before_generate && SFNeedsAutoHint(sf) &&
	    !(flags&ps_flag_nohints)) {
	ff_progress_change_line1(_("Auto Hinting Font..."));
	SplineFontAutoHint(sf,layer);
    }
    if ( !ff_progress_next_stage())
    {
        PSCharsFree(subrs);
        return( false );
    }

    otherblues[0] = otherblues[1] = bluevalues[0] = bluevalues[1] = 0;
    if ( !hasblue ) {
	FindBlues(sf,layer,bluevalues,otherblues);
	if ( !ff_progress_next_stage())
return( false );
    }
    bluescale = BlueScaleFigure(sf->private,bluevalues,otherblues);

    if ( !hash || !hasv )
    stdhw[0] = stdvw[0] = 0;
    if ( !hash ) {
	FindHStems(sf,stemsnaph,snapcnt);
	mi = -1;
	for ( i=0; i<12 && stemsnaph[i]!=0; ++i )
	    if ( mi==-1 ) mi = i;
	    else if ( snapcnt[i]>snapcnt[mi] ) mi = i;
	if ( mi!=-1 ) stdhw[0] = stemsnaph[mi];
    }

    if ( !hasv ) {
	FindVStems(sf,stemsnapv,snapcnt);
	mi = -1;
	for ( i=0; i<12 && stemsnapv[i]!=0 ; ++i )
	    if ( mi==-1 ) mi = i;
	    else if ( snapcnt[i]>snapcnt[mi] ) mi = i;
	if ( mi!=-1 ) stdvw[0] = stemsnapv[mi];
    }

    if ( incid==NULL ) {
	ff_progress_next_stage();
	ff_progress_change_line1(_("Converting PostScript"));
	if ( (chars = SplineFont2ChrsSubrs(sf,iscjk,subrs,flags,format,layer))==NULL )
        {
            PSCharsFree(subrs);
            return( false );
        }
	ff_progress_next_stage();
	ff_progress_change_line1(_("Saving PostScript Font"));
    }

    if ( incid==NULL ) dumpstr(dumpchar,data,"dup\n");
    cnt = 0;
    if ( !hasblue ) ++cnt;	/* bluevalues is required, but might be in private */
    if ( !hasshift && flex_max>=7 ) ++cnt;	/* BlueShift needs to be specified if flex wants something bigger than default */
    if ( bluescale!=-1 ) ++cnt;
    ++cnt;	/* minfeature is required */
    if ( !hasblue && (otherblues[0]!=0 || otherblues[1]!=0) ) ++cnt;
    ++cnt;	/* password is required */
    if ( sf->tempuniqueid!=0 && sf->use_uniqueid )
	++cnt;	/* UniqueID should be in both private and public areas */
    if ( incid==NULL ) {
	++cnt;	/* nd is required */
	++cnt;	/* np is required */
	++cnt;	/* rd is required */
    }
    if ( !haslg && iscjk ) ++cnt;
    if ( !hash ) {
	if ( stdhw[0]!=0 ) ++cnt;
	if ( stemsnaph[0]!=0 ) ++cnt;
    }
    if ( !hasv ) {
	if ( stdvw[0]!=0 ) ++cnt;
	if ( stemsnapv[0]!=0 ) ++cnt;
    }
    cnt += sf->private!=NULL?sf->private->next:0;
    if ( incid==NULL ) {
	++cnt;			/* Subrs entry */
	++cnt;			/* Other Subrs */
    } else {
	cnt += 3;		/* subrmap, etc. */
	++cnt;			/* Other Subrs */
    }
    if ( flex_max>0 ) ++cnt;
    if ( hasbold || isbold ) ++cnt;

    dumpf(dumpchar,data,"/Private %d dict dup begin\n", cnt );
	/* These guys are required and fixed */
    if ( incid==NULL ) {
	dumpstr(dumpchar,data,"/RD{string currentfile exch readstring pop}executeonly def\n" );
	dumpstr(dumpchar,data,"/ND{noaccess def}executeonly def\n" );
	dumpstr(dumpchar,data,"/NP{noaccess put}executeonly def\n" );
	ND = "ND";
    }
    dumpf(dumpchar,data,"/MinFeature{16 16}%s\n", ND );
    dumpstr(dumpchar,data,"/password 5839 def\n" );
    if ( !hasblue ) {
	dumpblues(dumpchar,data,"BlueValues",bluevalues,14,ND);
	if ( otherblues[0]!=0 || otherblues[1]!=0 )
	    dumpblues(dumpchar,data,"OtherBlues",otherblues,10,ND);
    }
    if ( !hash ) {
	if ( stdhw[0]!=0 )
	    dumpf(dumpchar,data,"/StdHW [%g] %s\n", (double) stdhw[0], ND );
	if ( stemsnaph[0]!=0 )
	    dumpdblmaxarray(dumpchar,data,"StemSnapH",stemsnaph,12,"", ND);
    }
    if ( !hasv ) {
	if ( stdvw[0]!=0 )
	    dumpf(dumpchar,data,"/StdVW [%g] %s\n", (double) stdvw[0],ND );
	if ( stemsnapv[0]!=0 )
	    dumpdblmaxarray(dumpchar,data,"StemSnapV",stemsnapv,12,"", ND);
    }
    if ( !hasshift && flex_max>=7 )
	dumpf(dumpchar,data,"/BlueShift %d def\n", flex_max+1 );
    if ( bluescale!=-1 )
	dumpf(dumpchar,data,"/BlueScale %g def\n", bluescale );
    if ( isbold && !hasbold )
	dumpf(dumpchar,data,"/ForceBold true def\n" );
    if ( !haslg && iscjk )
	dumpf(dumpchar,data,"/LanguageGroup 1 def\n" );
    if ( sf->tempuniqueid!=0 && sf->tempuniqueid!=-1 && sf->use_uniqueid )
	dumpf(dumpchar,data,"/UniqueID %d def\n", sf->tempuniqueid );
    if ( sf->private!=NULL ) {
	for ( i=0; i<sf->private->next; ++i ) {
	    dumpf(dumpchar,data,"/%s ", sf->private->keys[i]);
	    dumpstr(dumpchar,data,sf->private->values[i]);
	    if ( strcmp(sf->private->keys[i],"BlueValues")==0 ||
		    strcmp(sf->private->keys[i],"OtherBlues")==0 ||
		    strcmp(sf->private->keys[i],"StdHW")==0 ||
		    strcmp(sf->private->keys[i],"StdVW")==0 ||
		    strcmp(sf->private->keys[i],"StemSnapH")==0 ||
		    strcmp(sf->private->keys[i],"StemSnapV")==0 )
		dumpf(dumpchar,data," %s\n", ND);
	    else
		dumpstr(dumpchar,data," def\n");
	}
    }

    if ( mm!=NULL )
	dumpmmprivate(dumpchar,data,mm);

    dumpothersubrs(dumpchar,data,incid!=NULL,flex_max>0,iscjk,mm);
    if ( incid!=NULL ) {
	dumpf(dumpchar,data," /SubrMapOffset %d def\n", incid->subrmapoff );
	dumpf(dumpchar,data," /SDBytes %d def\n", incid->sdbytes );
	dumpf(dumpchar,data," /SubrCount %d def\n", incid->subrcnt );
	dumpstr(dumpchar,data,"end def\n");
    } else {
	dumpsubrs(dumpchar,data,sf,subrs);
	dumpcharstrings(dumpchar,data,sf, chars );
	dumpstr(dumpchar,data,"put\n");

	PSCharsFree(chars);
	PSCharsFree(subrs);
    }

    ff_progress_change_stages(1);
return( true );
}

static void dumpfontinfo(void (*dumpchar)(int ch,void *data), void *data, SplineFont *sf,
	enum fontformat format) {
    int cnt;

    cnt = 0;
    if ( sf->familyname!=NULL ) ++cnt;
    if ( sf->fullname!=NULL ) ++cnt;
    if ( sf->copyright!=NULL ) ++cnt;
    if ( sf->weight!=NULL ) ++cnt;
    if ( sf->pfminfo.fstype!=-1 ) ++cnt;
    if ( sf->subfontcnt==0 ) {
	if ( sf->version!=NULL ) ++cnt;
	++cnt;	/* ItalicAngle */
	++cnt;	/* isfixedpitch */
	if ( sf->upos!=0 ) ++cnt;
	if ( sf->uwidth!=0 ) ++cnt;
	/* Fontographer also generates em, ascent and descent */
	/*  em is redundant, we can get that from the fontmatrix */
	/*  given em we only need one of ascent or descent */
	/*  On the off chance that fontlab objects to them let's not generate them */
	if ( sf->ascent != 8*(sf->ascent+sf->descent)/10 )
	    ++cnt;		/* ascent */
    }
    if ( format==ff_mma || format==ff_mmb )
	cnt += 3;

    dumpf(dumpchar,data,"/FontInfo %d dict dup begin\n", cnt );
    if ( sf->subfontcnt==0 && sf->version )
	dumpf(dumpchar,data," /version (%s) readonly def\n", sf->version );
    if ( sf->copyright ) {
	dumpf(dumpchar,data," /Notice (");
	dumpcarefully(dumpchar,data,sf->copyright);
	dumpf(dumpchar,data,") readonly def\n" );
	if ( strchr(sf->copyright,'\n')!=NULL || strchr(sf->copyright,'\r')!=NULL )
	    dumpascomments(dumpchar,data,sf->copyright);
    }
    if ( sf->fullname ) {
	dumpf(dumpchar,data," /FullName (" );
	dumpcarefully(dumpchar,data,sf->fullname);
	dumpf(dumpchar,data,") readonly def\n" );
    }
    if ( sf->familyname ) {
	dumpf(dumpchar,data," /FamilyName (" );
	dumpcarefully(dumpchar,data,sf->familyname);
	dumpf(dumpchar,data,") readonly def\n" );
    }
    if ( sf->weight )
	dumpf(dumpchar,data," /Weight (%s) readonly def\n", sf->weight );
    if ( sf->pfminfo.fstype!=-1 )
	dumpf(dumpchar,data," /FSType %d def\n", sf->pfminfo.fstype );
    if ( sf->subfontcnt==0 ) {
	dumpf(dumpchar,data," /ItalicAngle %g def\n", (double) sf->italicangle );
	dumpf(dumpchar,data," /isFixedPitch %s def\n", SFOneWidth(sf)!=-1?"true":"false" );
	if ( format==ff_type42 || format==ff_type42cid ) {
	    if ( sf->upos )
		dumpf(dumpchar,data," /UnderlinePosition %g def\n", (double) (sf->upos/(sf->ascent+sf->descent)) );
	    if ( sf->uwidth )
		dumpf(dumpchar,data," /UnderlineThickness %g def\n", (double) (sf->uwidth/(sf->ascent+sf->descent)) );
	} else {
	    if ( sf->upos )
		dumpf(dumpchar,data," /UnderlinePosition %g def\n", (double) sf->upos );
	    if ( sf->uwidth )
		dumpf(dumpchar,data," /UnderlineThickness %g def\n", (double) sf->uwidth );
	}
	if ( sf->ascent != 8*(sf->ascent+sf->descent)/10 )
	    dumpf(dumpchar,data," /ascent %d def\n", sf->ascent );
    }
    if ( format==ff_mma || format==ff_mmb ) {
	MMSet *mm = sf->mm;
	int j,k;

	dumpstr(dumpchar,data," /BlendDesignPositions [" );
	for ( j=0; j<mm->instance_count; ++j ) {
	    dumpstr(dumpchar,data," [" );
	    for ( k=0; k<mm->axis_count; ++k )
		dumpf(dumpchar,data,"%g ", (double) mm->positions[j*mm->axis_count+k]);
	    dumpstr(dumpchar,data,"]" );
	}
	dumpstr(dumpchar,data," ] def\n" );

	dumpstr(dumpchar,data," /BlendDesignMap [" );
	for ( k=0; k<mm->axis_count; ++k ) {
	    dumpstr(dumpchar,data," [" );
	    for ( j=0; j<mm->axismaps[k].points; ++j )
		dumpf(dumpchar,data,"[%g %g] ",
			(double) mm->axismaps[k].designs[j], (double) mm->axismaps[k].blends[j]);
	    dumpstr(dumpchar,data,"]" );
	}
	dumpstr(dumpchar,data," ] def\n" );

	dumpstr(dumpchar,data," /BlendAxisTypes [" );
	for ( k=0; k<mm->axis_count; ++k )
	    dumpf(dumpchar,data,"/%s ", mm->axes[k]);
	dumpstr(dumpchar,data," ] def\n" );
    }
    dumpstr(dumpchar,data,"end readonly def\n");
}

static void dumpfontcomments(void (*dumpchar)(int ch,void *data), void *data,
	SplineFont *sf, int format ) {
    time_t now;
    const char *author = GetAuthor();

    now = GetTime();
    /* Werner points out that the DSC Version comment has a very specific */
    /*  syntax. We can't just put in a random string, must be <real> <int> */
    /* So we can sort of do that for CID fonts (give it a <revsion> of 0 */
    /*  but for type1s just use a comment rather than a DSC statement */
    if (( format==ff_cid || format==ff_cffcid || format==ff_type42cid ) &&
	    sf->cidregistry!=NULL ) {
	dumpf(dumpchar,data, "%%%%Title: (%s %s %s %d)\n",
		sf->fontname, sf->cidregistry, sf->ordering, sf->supplement );
	dumpf(dumpchar,data, "%%%%Version: %g 0\n", (double)sf->cidversion );
	/* CID keyed fonts are language level 3 */
    } else {
	dumpf(dumpchar,data,"%%%%Title: %s\n", sf->fontname);
	dumpf(dumpchar,data,"%%Version: %s\n", sf->version);
    }
    dumpf(dumpchar,data,"%%%%CreationDate: %s", ctime(&now));
    if ( author!=NULL )
	dumpf(dumpchar,data,"%%%%Creator: %s\n", author);

    if ( format==ff_cid || format==ff_cffcid || format==ff_type42cid ||
	    format==ff_cff || format==ff_type42 )
	dumpf(dumpchar,data, "%%%%LanguageLevel: 3\n" );
    else if ( sf->multilayer && format==ff_ptype3 ) {
	int gid, ly;
	SplineChar *sc;
	int had_pat=0, had_grad=0;
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL ) {
	    for ( ly=ly_fore; ly<sc->layer_cnt; ++ly ) {
		if ( sc->layers[ly].fill_brush.gradient!=NULL || sc->layers[ly].stroke_pen.brush.gradient!=NULL ) {
		    had_grad = true;
	    break;
		}
		if ( sc->layers[ly].fill_brush.gradient!=NULL || sc->layers[ly].stroke_pen.brush.gradient!=NULL )
		    had_pat = true;
	    }
	}
	if ( had_grad )
	    dumpf(dumpchar,data, "%%%%LanguageLevel: 3\n" );
	else if ( had_pat )
	    dumpf(dumpchar,data, "%%%%LanguageLevel: 2\n" );
    }

    if ( sf->copyright!=NULL ) {
	char *pt, *strt=sf->copyright, *npt;
	while ( *strt!='\0' ) {
	    pt = strt;
	    while ( pt<strt+60 && *pt ) {
		npt = strpbrk(pt,"\n\t\r ");
		if ( npt==NULL ) npt = strt+strlen(strt);
		if ( npt<strt+60 || pt==strt ) {
		    pt = npt;
		    if ( isspace(*pt)) {
			++pt;
			if ( pt[-1]=='\n' || pt[-1]=='\r' )
	    break;
		    }
		} else
	    break;
	    }
	    dumpstr(dumpchar,data,strt==sf->copyright ? "%Copyright: ": "%Copyright:  ");
	    dumpstrn(dumpchar,data,strt,*pt?pt-strt-1:pt-strt);
	    dumpchar('\n',data);
	    strt = pt;
	}
    }
    if ( sf->comments!=NULL )
	dumpascomments(dumpchar,data,sf->comments);
    dumpf(dumpchar,data,"%% Generated by FontForge %d (http://fontforge.sf.net/)\n", FONTFORGE_VERSIONDATE_RAW);
    dumpstr(dumpchar,data,"%%EndComments\n\n");
}

extern const char *mmfindfont[], *makeblendedfont[];
static void dumprequiredfontinfo(void (*dumpchar)(int ch,void *data), void *data,
	SplineFont *sf, int format, EncMap *map, SplineFont *fullsf, int layer ) {
    int cnt, i;
    double fm[6];
    const char (*encoding[256]);
    DBounds b;
    int uniqueid;
    char buffer[50];

    dumpf(dumpchar,data,"%%!PS-AdobeFont-1.0: %s %s\n", sf->fontname, sf->version?sf->version:"" );
    if ( format==ff_ptype0 && (map->enc->is_unicodebmp || map->enc->is_unicodefull))
	dumpf(dumpchar,data,"%%%%DocumentNeededResources: font ZapfDingbats\n" );
/*  dumpf(dumpchar,data, "%%%%DocumentSuppliedResources: font %s\n", sf->fontname );*/
/* Werner says the above is not appropriate */
    dumpfontcomments(dumpchar,data,sf,format);

    cnt = 0;
    ++cnt;		/* FID, added by definefont and not by us */
    ++cnt;		/* fonttype */
    ++cnt;		/* fontmatrix */
    if ( sf->fontname!=NULL ) ++cnt;
    ++cnt;		/* fontinfo */
    ++cnt;		/* encoding */
    ++cnt;		/* fontbb */
    if ( sf->uniqueid!=-1 && sf->use_uniqueid ) ++cnt;
    ++cnt;		/* painttype */
    if ( sf->strokedfont )
	++cnt;		/* StrokeWidth */
    if ( format==ff_ptype3 ) {
	++cnt;		/* charprocs */
	++cnt;		/* buildglyph */
	++cnt;		/* buildchar */
    } else {
	++cnt;		/* private */
	++cnt;		/* chars */
    }
    if ( sf->xuid!=NULL && sf->use_xuid ) ++cnt;
    if ( format==ff_mma || format==ff_mmb )
	cnt += 7;

    if ( sf->uniqueid==0 )
	uniqueid = 4000000 + (rand()&0x3ffff);
    else
	uniqueid = sf->uniqueid ;
    sf->tempuniqueid = uniqueid;

    if ( format!=ff_ptype3 && uniqueid!=-1 && sf->use_uniqueid ) {
	dumpf(dumpchar,data,"FontDirectory/%s known{/%s findfont dup/UniqueID known{dup\n", sf->fontname, sf->fontname);
	dumpf(dumpchar,data,"/UniqueID get %d eq exch/FontType get 1 eq and}{pop false}ifelse\n", uniqueid );
	dumpf(dumpchar,data,"{save true}{false}ifelse}{false}ifelse\n" );
    }

    dumpf(dumpchar,data,"%d dict begin\n", cnt );
    dumpf(dumpchar,data,"/FontType %d def\n", format==ff_ptype3?3:1 );
    fm[0] = fm[3] = 1.0/((sf->ascent+sf->descent));
    fm[1] = fm[2] = fm[4] = fm[5] = 0;
    dumpdblarray(dumpchar,data,"FontMatrix",fm,6,"readonly ",false);
    if ( sf->fontname!=NULL )
	dumpf(dumpchar,data,"/FontName /%s def\n", sf->fontname );
    SplineFontLayerFindBounds(fullsf==NULL?sf:fullsf,layer,&b);
    fm[0] = floor( b.minx);
    fm[1] = floor( b.miny);
    fm[2] = ceil( b.maxx);
    fm[3] = ceil( b.maxy);
    dumpdblarray(dumpchar,data,"FontBBox",fm,4,"readonly ", true);
    if ( uniqueid!=-1 && sf->use_uniqueid )
	dumpf(dumpchar,data,"/UniqueID %d def\n", uniqueid );
    if ( sf->xuid!=NULL && sf->use_xuid ) {
	dumpf(dumpchar,data,"/XUID %s def\n", sf->xuid );
	if ( sf->changed_since_xuidchanged )
	    SFIncrementXUID(sf);
    }
    dumpf(dumpchar,data,"/PaintType %d def\n", sf->strokedfont?2:0 );
    if ( sf->strokedfont )
	dumpf(dumpchar,data,"/StrokeWidth %g def\n", (double) sf->strokewidth );
    dumpfontinfo(dumpchar,data,sf,format);
    if ( format==ff_mma || format==ff_mmb ) {
	MMSet *mm = sf->mm;
	int j,k;
	DBounds mb[16];

	dumpstr(dumpchar,data," /WeightVector [" );
	for ( j=0; j<mm->instance_count; ++j ) {
	    dumpf(dumpchar,data,"%g ", (double) mm->defweights[j]);
	}
	dumpstr(dumpchar,data," ] def\n" );

	dumpstr(dumpchar,data," /$Blend {" );
	if ( mm->instance_count==2 )
	    dumpf(dumpchar,data,"%g mul add", (double) mm->defweights[1]);
	else {
	    dumpf(dumpchar,data,"%g mul exch", (double) mm->defweights[1]);
	    for ( j=2; j<mm->instance_count-1; ++j )
		dumpf(dumpchar,data,"%g mul add exch", (double) mm->defweights[j]);
	    dumpf(dumpchar,data,"%g mul add add", (double) mm->defweights[j]);
	}
	dumpstr(dumpchar,data," } bind def\n" );

	dumpstr(dumpchar,data," /Blend 3 dict dup begin\n" );
	for ( j=0; j<mm->instance_count; ++j )
	    SplineFontLayerFindBounds(mm->instances[j],layer,&mb[j]);
	dumpstr(dumpchar,data,"  /FontBBox{" );
	for ( k=0; k<4; ++k ) {
	    dumpstr(dumpchar,data,"{" );
	    for ( j=0; j<mm->instance_count; ++j )
		dumpf(dumpchar,data,"%g ", k==0 ? floor(mb[j].minx) :
					   k==1 ? floor(mb[j].miny) :
					   k==2 ? ceil(mb[j].maxx) :
						  ceil(mb[j].maxy));
	    dumpstr(dumpchar,data,"}" );
	}
	dumpstr(dumpchar,data,"} def\n" );

	dumpf(dumpchar,data,"  /Private %d dict def\n", sf->private->next+10 );
	dumpstr(dumpchar,data," end def		%End of Blend dict\n" );

	for ( j=0; makeblendedfont[j]!=NULL; ++j ) {
	    dumpstr(dumpchar,data,makeblendedfont[j]);
	    (dumpchar)('\n',data);
	}

	dumpstr(dumpchar,data,"\n /NormalizeDesignVector\n" );
	dumpstr(dumpchar,data, mm->ndv );
	dumpstr(dumpchar,data," bind def\n" );

	dumpstr(dumpchar,data," /ConvertDesignVector\n" );
	dumpstr(dumpchar,data, mm->cdv );
	dumpstr(dumpchar,data," bind def\n\n" );

	for ( j=0; mmfindfont[j]!=NULL; ++j ) {
	    dumpstr(dumpchar,data,mmfindfont[j]);
	    (dumpchar)('\n',data);
	}
    }

    for ( i=0; i<256 && i<map->enccount; ++i )
	if ( map->map[i]!=-1 && SCWorthOutputting(sf->glyphs[map->map[i]]) )
	    encoding[i] = sf->glyphs[map->map[i]]->name;
	else
	    encoding[i] = ".notdef";
    for ( ; i<256; ++i )
	encoding[i] = ".notdef";
    if ( isStdEncoding(encoding) )
	dumpstr(dumpchar,data,"/Encoding StandardEncoding def\n");
    else {
	dumpstr(dumpchar,data,"/Encoding 256 array\n" );
	    /* older versions of dvipdfm assume the following line is present.*/
	    /*  Perhaps others do too? */
	dumpstr(dumpchar,data," 0 1 255 { 1 index exch /.notdef put} for\n" );
	for ( i=0; i<256; ++i )
	    if ( strcmp(encoding[i],".notdef")!=0 )
		dumpf(dumpchar,data,"dup %d/%s put\n", i, encoding[i] );
	dumpstr(dumpchar,data,"readonly def\n" );
    }
    if ( format==ff_ptype3 ) {
	dumpstr(dumpchar,data,"/BuildChar { 1 index /Encoding get exch get 1 index /BuildGlyph get exec } bind def\n" );
	dumpstr(dumpchar,data,"% I call all my CharProcs with two arguments, the top of the stack will be\n" );
	dumpstr(dumpchar,data,"%  0 and then next thing is the fontdict. If the tos is zero the char will\n" );
	dumpstr(dumpchar,data,"%  do a setcachedevice, otherwise (for referenced chars) it will not. The\n" );
	dumpstr(dumpchar,data,"%  fontdict argument is so a char can invoke a referenced char. BuildGlyph\n" );
	dumpstr(dumpchar,data,"%  itself will remove the arguments from the stack, the CharProc will leave 'em\n" );
	if ( sf->multilayer )
	    *buffer = '\0';
	else if ( sf->strokedfont )
	    sprintf( buffer, "%g setlinewidth stroke", (double) sf->strokewidth );
	else
	    strcpy(buffer, "fill");
	dumpf(dumpchar,data,"/BuildGlyph { 2 copy exch /CharProcs get exch 2 copy known not { pop /.notdef} if get exch pop 0 exch exec pop pop %s} bind def\n",
		buffer );
    }
}

static void dumpinitialascii(void (*dumpchar)(int ch,void *data), void *data,
	SplineFont *sf, int format, EncMap *map, SplineFont *fullsf, int layer ) {
    dumprequiredfontinfo(dumpchar,data,sf,format,map,fullsf,layer);
    dumpstr(dumpchar,data,"currentdict end\ncurrentfile eexec\n" );
}

static void dumpencodedstuff(void (*dumpchar)(int ch,void *data), void *data,
	SplineFont *sf, int format, int flags, EncMap *map, int layer ) {
    struct fileencryptdata fed;
    void (*func)(int ch,void *data);

    func = startfileencoding(dumpchar,data,&fed,format==ff_pfb || format==ff_mmb);
    dumpprivatestuff(func,&fed,sf,NULL,flags,format,map,layer);
    if ( format==ff_ptype0 ) {
	dumpstr(func,&fed, "/" );
	dumpstr(func,&fed, sf->fontname );
	dumpstr(func,&fed,"Base exch definefont pop\n mark currentfile closefile\n" );
    } else
	dumpstr(func,&fed,"dup/FontName get exch definefont pop\n mark currentfile closefile\n" );
}

static void dumpfinalascii(void (*dumpchar)(int ch,void *data), void *data,
	SplineFont *sf, int format) {
    int i;
    int uniqueid = sf->uniqueid ;

    /* output 512 zeros */
    dumpchar('\n',data);
    for ( i = 0; i<8; ++i )
	dumpstr(dumpchar,data,"0000000000000000000000000000000000000000000000000000000000000000\n");
    dumpstr(dumpchar,data,"cleartomark\n");
    if ( format!=ff_ptype3 && uniqueid!=-1 && sf->use_uniqueid )
	dumpstr(dumpchar,data,"{restore}if\n");
}

static void mkheadercopyfile(FILE *temp,FILE *out,int headertype) {
    char buffer[8*1024];
    int len;

    /* output the file header */
    putc('\200',out);
    putc(headertype,out);
    len = ftell(temp);		/* output byte count */
    putc(len&0xff,out);
    putc(((len>>8)&0xff),out);
    putc(((len>>16)&0xff),out);
    putc(((len>>24)&0xff),out);

    fseek(temp,0,SEEK_SET);
    while ((len=fread(buffer,sizeof(char),sizeof(buffer),temp))>0 )
	fwrite(buffer,sizeof(char),len,out);
    fclose(temp);		/* deletes the temporary file */
}

static void dumptype42(FILE *out, SplineFont *sf, int format, int flags,
	EncMap *map,int layer) {
    double fm[6];
    DBounds b;
    int uniqueid;
    int i,cnt,gid,hasnotdef;
    SplineFont *cidmaster;

    cidmaster = sf->cidmaster;
    if ( sf->subfontcnt!=0 ) {
	cidmaster = sf;
	sf = sf->subfonts[0];
    }
    if ( format==ff_type42 )
	cidmaster = NULL;

    if ( format == ff_type42cid ) {
	fprintf( out, "%%!PS-Adobe-3.0 Resource-CIDFont\n" );
	fprintf( out, "%%%%DocumentNeededResources: ProcSet (CIDInit)\n" );
	fprintf( out, "%%%%IncludeResource: ProcSet (CIDInit)\n" );
	fprintf( out, "%%%%BeginResource: CIDFont %s\n", (cidmaster!=NULL?cidmaster:sf)->fontname );
    } else
	fprintf( out, "%%!PS-TrueTypeFont\n" );	/* Ignore the ttf version info */
	    /* Too hard to do right, and if done right doesn't mean much to */
	    /* a human observer */
    dumpfontcomments((DumpChar)fputc,out,cidmaster!=NULL?cidmaster:sf,format);

    if ( format == ff_type42cid ) {
	fprintf( out, "/CIDInit /ProcSet findresource begin\n\n" );
	fprintf( out, "16 dict begin\n" );
    } else
	fprintf( out, "12 dict begin\n" );
    fprintf( out, "  /FontName /%s def\n", sf->fontname );
    if ( format==ff_type42cid ) {
	fprintf( out, "  /CIDFontType 2 def\n" );
	/* Adobe Technical Note 5012.Type42_Spec.pdf Sec 5 CIDFontType 2 CID Fonts */
	/*  page 12 says that the FontType (not CIDFontType) of a TrueType CID font */
	/*  should be 42. The PLRM (v3) Chap 5 page 370 says the FontType should */
	/*  11.  Ain't that just grand? */
	fprintf( out, "  /FontType 11 def\n" );
	fprintf( out, "  /CIDFontName /%s def\n", sf->fontname );
	/* 5012 doesn't mention this, but we probably need these... */
	fprintf( out, "  /CIDSystemInfo 3 dict dup begin\n" );
	if ( cidmaster!=NULL ) {
	    fprintf( out, "    /Registry (%s) def\n", cidmaster->cidregistry );
	    fprintf( out, "    /Ordering (%s) def\n", cidmaster->ordering );
	    fprintf( out, "    /Supplement %d def\n", cidmaster->supplement );
	} else {
	    fprintf( out, "    /Registry (Adobe) def\n" );
	    fprintf( out, "    /Ordering (Identity) def\n" );
	    fprintf( out, "    /Supplement 0 def\n" );
	}
	fprintf( out, "  end def\n\n" );
    } else
	fprintf( out, "  /FontType 42 def\n" );
    fprintf( out, "  /FontMatrix [1 0 0 1 0 0] def\n" );
    fprintf( out, "  /PaintType 0 def\n" );
    SplineFontLayerFindBounds(sf,layer,&b);
/* The Type42 spec says that the bounding box should be scaled by the */
/*  1/emsize (to be numbers near 1.0). The example in the spec does not do */
/*  this and gives numbers you'd expect to see in a type1 font */
    fm[0] = b.minx / (sf->ascent+sf->descent);
    fm[1] = b.miny / (sf->ascent+sf->descent);
    fm[2] = b.maxx / (sf->ascent+sf->descent);
    fm[3] = b.maxy / (sf->ascent+sf->descent);
    fprintf( out, "  ");
    dumpdblarray((DumpChar) fputc,out,"FontBBox",fm,4,"readonly ", true);
    dumpfontinfo((DumpChar) fputc,out,sf,format);

    if ( sf->uniqueid==0 )
	uniqueid = 4000000 + (rand()&0x3ffff);
    else
	uniqueid = sf->uniqueid ;
    sf->tempuniqueid = uniqueid;
    if ( uniqueid!=-1 && sf->use_uniqueid )
	fprintf( out, "  /UniqueID %d def\n", uniqueid );
    if ( sf->xuid!=NULL && sf->use_xuid ) {
	fprintf(out,"  /XUID %s def\n", sf->xuid );
	if ( sf->changed_since_xuidchanged )
	    SFIncrementXUID(sf);
    }
    if ( format==ff_type42 ) {
	fprintf(out,"  /Encoding 256 array\n" );
	    /* older versions of dvipdfm assume the following line is present.*/
	    /*  Perhaps others do too? */
	fprintf(out,"   0 1 255 { 1 index exch /.notdef put} for\n" );
	for ( i=0; i<256 && i<map->enccount; ++i ) if ( (gid = map->map[i])!=-1 )
	    if ( SCWorthOutputting(sf->glyphs[gid]) )
		fprintf( out, "    dup %d/%s put\n", i, sf->glyphs[gid]->name);
	fprintf( out, "  readonly def\n" );
    }
    fprintf( out, "  /sfnts [\n" );
    _WriteType42SFNTS(out,sf,format,flags,map,layer);
    fprintf( out, "  ] def\n" );
    if ( format==ff_type42 ) {
	hasnotdef = false;
	for ( i=cnt=0; i<sf->glyphcnt; ++i ) {
	    if ( sf->glyphs[i]!=NULL && SCWorthOutputting(sf->glyphs[i])) {
		++cnt;
		if ( strcmp(sf->glyphs[i]->name,".notdef")==0 )
		    hasnotdef = true;
	    }
	}
	fprintf( out, "  /CharStrings %d dict dup begin\n", cnt+1 );
	/* Why check to see if there's a notdef char in the font? If there is */
	/*  we can define the dictionary entry twice */
	/* We can, yes, but FreeType gets confused if we do. So let's check */
	if ( !hasnotdef )
	    fprintf( out, "    /.notdef 0 def\n" );
	for ( i=0; i<sf->glyphcnt; ++i )
	    if ( sf->glyphs[i]!=NULL && SCWorthOutputting(sf->glyphs[i]))
		fprintf( out, "    /%s %d def\n", sf->glyphs[i]->name, sf->glyphs[i]->ttf_glyph );
	fprintf( out, "  end readonly def\n" );
	fprintf( out, "FontName currentdict end definefont pop\n" );
    } else {
	if ( cidmaster!=NULL ) {
	    for ( i=cnt=0; i<sf->glyphcnt; ++i )
		if ( sf->glyphs[i]!=NULL && SCWorthOutputting(sf->glyphs[i]))
		    ++cnt;
	    fprintf( out, "  /CIDMap %d dict dup begin\n", cnt );
	    for ( i=0; i<sf->glyphcnt; ++i )
		if ( sf->glyphs[i]!=NULL && SCWorthOutputting(sf->glyphs[i]))
		    fprintf( out, "    %d %d def\n", i, sf->glyphs[i]->ttf_glyph );
	    fprintf( out, "  end readonly def\n" );
	    fprintf( out, "  /CIDCount %d def\n", sf->glyphcnt );
	    fprintf( out, "  /GDBytes %d def\n", sf->glyphcnt>65535?3:2 );
	} else if ( flags & ps_flag_identitycidmap ) {
	    for ( i=cnt=0; i<sf->glyphcnt; ++i )
		if ( sf->glyphs[i]!=NULL && cnt<sf->glyphs[i]->ttf_glyph )
		    cnt = sf->glyphs[i]->ttf_glyph;
	    fprintf( out, "  /CIDCount %d def\n", cnt+1 );
	    fprintf( out, "  /GDBytes %d def\n", cnt+1>65535?3:2 );
	    fprintf( out, "  /CIDMap 0 def\n" );
	} else {	/* Use unicode */
	    int maxu = 0;
	    for ( i=cnt=0; i<sf->glyphcnt; ++i )
		if ( sf->glyphs[i]!=NULL && SCWorthOutputting(sf->glyphs[i]) &&
			sf->glyphs[i]->unicodeenc!=-1 && sf->glyphs[i]->unicodeenc<0x10000 ) {
		    ++cnt;
		    if ( sf->glyphs[i]->unicodeenc > maxu )
			maxu = sf->glyphs[i]->unicodeenc;
		}
	    fprintf( out, "  /CIDMap %d dict dup begin\n", cnt );
	    fprintf( out, "    0 0 def\n" );		/* .notdef doesn't have a unicode enc, will be missed. Needed */
	    if ( map->enc->is_unicodebmp || map->enc->is_unicodefull ) {
		for ( i=0; i<map->enccount && i<0x10000; ++i ) if ( (gid = map->map[i])!=-1 ) {
		    if ( SCWorthOutputting(sf->glyphs[gid]))
			fprintf( out, "    %d %d def\n", i, sf->glyphs[gid]->ttf_glyph );
		}
	    } else {
		for ( i=0; i<sf->glyphcnt; ++i )
		    if ( sf->glyphs[i]!=NULL && SCWorthOutputting(sf->glyphs[i]) &&
			    sf->glyphs[i]->unicodeenc!=-1 && sf->glyphs[i]->unicodeenc<0x10000 )
			fprintf( out, "    %d %d def\n", sf->glyphs[i]->unicodeenc, sf->glyphs[i]->ttf_glyph );
	    }
	    fprintf( out, "  end readonly def\n" );
	    fprintf( out, "  /GDBytes %d def\n", maxu>65535?3:2 );
	    fprintf( out, "  /CIDCount %d def\n", maxu+1 );
	}
	fprintf( out, "currentdict end dup /CIDFontName get exch /CIDFont defineresource pop\nend\n" );
	fprintf( out, "%%%%EndResource\n" );
	fprintf( out, "%%%%EOF\n" );
    }
}

static void dumpfontdict(FILE *out, SplineFont *sf, int format, int flags,
	EncMap *map, SplineFont *fullsf, int layer ) {

/* a pfb header consists of 6 bytes, the first is 0200, the second is a */
/*  binary/ascii flag where 1=>ascii, 2=>binary, 3=>eof??, the next four */
/*  are a count of bytes between this header and the next one. First byte */
/*  is least significant */
    if ( format==ff_pfb || format==ff_mmb ) {
	FILE *temp;
	temp = tmpfile();
	dumpinitialascii((DumpChar) fputc,temp,sf,format,map,fullsf,layer );
	mkheadercopyfile(temp,out,1);
	temp = tmpfile();
	dumpencodedstuff((DumpChar) fputc,temp,sf,format,flags,map,layer);
	mkheadercopyfile(temp,out,2);
	temp = tmpfile();
	dumpfinalascii((DumpChar) fputc,temp,sf,format);
	mkheadercopyfile(temp,out,1);
/* final header, 3=>eof??? */
	dumpstrn((DumpChar) fputc,out,"\200\003",2);
    } else if ( format==ff_ptype3 ) {
	dumprequiredfontinfo((DumpChar) fputc,out,sf,ff_ptype3,map,NULL,layer);
	dumpcharprocs((DumpChar) fputc,out,sf);
    } else if ( format==ff_type42 || format==ff_type42cid ) {
	dumptype42(out,sf,format,flags,map,layer);
    } else {
	dumpinitialascii((DumpChar) (fputc),out,sf,format,map,fullsf,layer );
	dumpencodedstuff((DumpChar) (fputc),out,sf,format,flags,map,layer);
	dumpfinalascii((DumpChar) (fputc),out,sf,format);
    }
}

static void dumpreencodeproc(FILE *out) {

    fprintf( out, "\n/reencodedict 10 dict def\n" );
    fprintf( out, "/ReEncode\n" );
    fprintf( out, "  { reencodedict begin\n" );
    fprintf( out, "\t/newencoding exch def\n" );
    fprintf( out, "\t/newfontname exch def\n" );
    fprintf( out, "\tfindfont /basefontdict exch def\n" );
    fprintf( out, "\t/newfont basefontdict maxlength dict def\n" );
    fprintf( out, "\tbasefontdict\n" );
    fprintf( out, "\t  { exch dup dup /FID ne exch /Encoding ne and\n" );
    fprintf( out, "\t\t{ exch newfont 3 1 roll put }\n" );
    fprintf( out, "\t\t{ pop pop }\n" );
    fprintf( out, "\t\tifelse\n" );
    fprintf( out, "\t  } forall\n" );
    fprintf( out, "\tnewfont /FontName newfontname put\n" );
    fprintf( out, "\tnewfont /Encoding newencoding put\n" );
    fprintf( out, "\tnewfontname newfont definefont pop\n" );
    fprintf( out, "\tend\n" );
    fprintf( out, "  } def\n" );
    fprintf( out, "\n" );
}

static const char *dumpnotdefenc(FILE *out,SplineFont *sf) {
    const char *notdefname;
    int i;

    /* At one point I thought the unicode replacement char 0xfffd */
    /*  was a good replacement for notdef if the font contained */
    /*  no notdef. Probably not a good idea for a PS font */
    notdefname = ".notdef";
    fprintf( out, "/%sBase /%sNotDef [\n", sf->fontname, sf->fontname );
    for ( i=0; i<256; ++i )
	fprintf( out, " /%s\n", notdefname );
    fprintf( out, "] ReEncode\n\n" );
return( notdefname );
}

static int somecharsused(SplineFont *sf, int bottom, int top, EncMap *map) {
    int i;

    for ( i=bottom; i<=top && i<map->enccount; ++i ) {
	if ( map->map[i]!=-1 && SCWorthOutputting(sf->glyphs[map->map[i]]) )
return( true );
    }
return( false );
}

extern char *zapfnomen[];
extern int8 zapfexists[];
static void dumptype0stuff(FILE *out,SplineFont *sf, EncMap *map) {
    const char *notdefname;
    int i,j;

    dumpreencodeproc(out);
    notdefname = dumpnotdefenc(out,sf);
    for ( i=1; i<256; ++i ) {
	if ( somecharsused(sf,i<<8, (i<<8)+0xff, map)) {
	    fprintf( out, "/%sBase /%s%d [\n", sf->fontname, sf->fontname, i );
	    for ( j=0; j<256 && (i<<8)+j<map->enccount; ++j )
		if ( map->map[(i<<8)+j]!=-1 && SCWorthOutputting(sf->glyphs[map->map[(i<<8)+j]]) )
		    fprintf( out, " /%s\n", sf->glyphs[map->map[(i<<8)+j]]->name );
		else
		    fprintf( out, "/%s\n", notdefname );
	    for ( ; j<256; ++j )
		fprintf( out, " /%s\n", notdefname );
	    fprintf( out, "] ReEncode\n\n" );
	} else if ( i==0x27 && (map->enc->is_unicodebmp || map->enc->is_unicodefull)) {
	    fprintf( out, "%% Add Zapf Dingbats to unicode font at 0x2700\n" );
	    fprintf( out, "%%  But only if on the printer, else use notdef\n" );
	    fprintf( out, "%%  gv, which has no Zapf, maps courier to the name\n" );
	    fprintf( out, "%%  so we must check a bit more than is it null or not...\n" );
/* gv with no ZapfDingbats installed does weird stuff. */
/*  If I do "/ZapfDingbats findfont" then it returns "/Courier findfont" the */
/*  first time, but the second time it returns null */
/* So even if the printer thinks it's got Zapf we must check to make sure it's*/
/*  the real Zapf. We do that by examining the name. If it's ZapfDingbats all*/
/*  should be well, if it's Courier, then that counts as non-existant */
	    fprintf( out, "/ZapfDingbats findfont pop\n" );
	    fprintf( out, "/ZapfDingbats findfont null eq\n" );
	    fprintf( out, "{ true }\n" );
	    fprintf( out, " { /ZapfDingbats findfont /FontName get (ZapfDingbats) ne }\n" );
	    fprintf( out, " ifelse\n" );
	    fprintf( out, "{ /%s%d /%sNotDef findfont definefont pop }\n",
		    sf->fontname, i, sf->fontname);
	    fprintf( out, " { /ZapfDingbats /%s%d [\n", sf->fontname, i );
	    for ( j=0; j<0xc0; ++j )
		fprintf( out, " /%s\n",
			zapfexists[j]?zapfnomen[j]:".notdef" );
	    for ( ;j<256; ++j )
		fprintf( out, " /%s\n", ".notdef" );
	    fprintf( out, "] ReEncode\n\n" );
	    fprintf( out, "  } ifelse\n\n" );
	}
    }

    fprintf( out, "/%s 21 dict dup begin\n", sf->fontname );
    fprintf( out, "/FontInfo /%sBase findfont /FontInfo get def\n", sf->fontname );
    fprintf( out, "/PaintType %d def\n", sf->strokedfont?2:0 );
    if ( sf->strokedfont )
	fprintf( out, "/StrokeWidth %g def\n", (double) sf->strokewidth );
    fprintf( out, "/FontType 0 def\n" );
    fprintf( out, "/LanguageLevel 2 def\n" );
    fprintf( out, "/FontMatrix [1 0 0 1 0 0] readonly def\n" );
    fprintf( out, "/FMapType 2 def\n" );
    fprintf( out, "/Encoding [\n" );
    for ( i=0; i<256; ++i )
	fprintf( out, " %d\n", i );
    fprintf( out, "] readonly def\n" );
    fprintf( out, "/FDepVector [\n" );
    fprintf( out, " /%sBase findfont\n", sf->fontname );
    for ( i=1; i<256; ++i )
	if ( somecharsused(sf,i<<8, (i<<8)+0xff, map) ||
		(i==0x27 && (map->enc->is_unicodebmp || map->enc->is_unicodefull)) )
	    fprintf( out, " /%s%d findfont\n", sf->fontname, i );
	else
	    fprintf( out, " /%sNotDef findfont\n", sf->fontname );
    fprintf( out, "  ] readonly def\n" );
    fprintf( out, "end definefont pop\n" );
    fprintf( out, "%%%%EOF\n" );
}

static void dumpt1str(FILE *binary,uint8 *data, int len, int leniv) {
    if ( leniv==-1 )
	fwrite(data,sizeof(uint8),len,binary);
    else
	encodestrout((DumpChar) fputc,binary,data,len,leniv);
}

static void dump_index(FILE *binary,int size,int val) {

    if ( size>=4 )
	putc((val>>24)&0xff,binary);
    if ( size>=3 )
	putc((val>>16)&0xff,binary);
    if ( size>=2 )
	putc((val>>8)&0xff,binary);
    if ( size>=1 )			/* Yes, size may be 0 for the fd index */
	putc((val)&0xff,binary);
}

static FILE *gencidbinarydata(SplineFont *cidmaster,struct cidbytes *cidbytes,
	int flags,EncMap *map,int layer) {
    int i,j, leniv, subrtot;
    SplineFont *sf;
    struct fddata *fd;
    FILE *chrs, *subrs, *binary;
    char *buffer;
    long offset;
    char *pt;
    struct pschars *chars;
    int len;

    memset(cidbytes,'\0',sizeof(struct cidbytes));
    cidbytes->fdcnt = cidmaster->subfontcnt;
    cidbytes->fds = calloc(cidbytes->fdcnt,sizeof(struct fddata));
    for ( i=0; i<cidbytes->fdcnt; ++i ) {
	sf = cidmaster->subfonts[i];
	fd = &cidbytes->fds[i];
	fd->flexmax = SplineFontIsFlexible(sf,layer,flags);
	fd->subrs = initsubrs(NULL);
	if ( fd->subrs==NULL ) {
	    int j;
	    for ( j=0; j<i; ++j )
		PSCharsFree(cidbytes->fds[j].subrs);
	    free( cidbytes->fds );
return( NULL );
	}
	fd->iscjk = SFIsCJK(sf,map);
	pt = PSDictHasEntry(sf->private,"lenIV");
	if ( pt!=NULL )
	    fd->leniv = strtol(pt,NULL,10);
	else
	    fd->leniv = 4;
    }
    ff_progress_change_line1(_("Converting PostScript"));
    if ( (chars = CID2ChrsSubrs(cidmaster,cidbytes,flags,layer))==NULL )
return( NULL );
    ff_progress_next_stage();
    ff_progress_change_line1(_("Saving PostScript Font"));

    chrs = tmpfile();
    for ( i=0; i<chars->next; ++i ) {
	if ( chars->lens[i]!=0 ) {
	    leniv = cidbytes->fds[cidbytes->fdind[i]].leniv;
	    dumpt1str(chrs,chars->values[i],chars->lens[i],leniv);
	    if ( !ff_progress_next()) {
		PSCharsFree(chars);
		fclose(chrs);
return( NULL );
	    }
	    if ( leniv>0 )
		chars->lens[i] += leniv;
	}
    }
    subrs = tmpfile(); subrtot = 0;
    for ( i=0; i<cidbytes->fdcnt; ++i ) {
	fd = &cidbytes->fds[i];
	leniv = fd->leniv;
	for ( j=0; j<fd->subrs->next; ++j ) {
	    dumpt1str(subrs,fd->subrs->values[j],fd->subrs->lens[j],leniv);
	    if ( leniv>0 )
		fd->subrs->lens[j] += leniv;
	}
	fd->subrcnt = j;
	subrtot += j;
    }

    cidbytes->fdbytes = ( cidbytes->fdcnt==1 )? 0 :
			( cidbytes->fdcnt<256 )? 1 : 2;
    if ( (cidbytes->cidcnt+1)*(cidbytes->fdbytes+3) +		/* size of the CID map region */
	    (subrtot+1) * 3 +					/* size of the Subr map region */
	    ftell(subrs) +					/* size of the subr region */
	    ftell(chrs) < 0x1000000 )				/* size of the charstring region */ /* Are all our offsets less than 3 bytes? */
	cidbytes->gdbytes = 3;			/* Adobe's convention is to use 3, so don't bother checking for anything less */
    else
	cidbytes->gdbytes = 4;			/* but I suppose we might need more... */

    cidbytes->errors = ferror(chrs) || ferror(subrs);

    offset = (cidbytes->cidcnt+1)*(cidbytes->fdbytes+cidbytes->gdbytes) +
	    (subrtot+1) * cidbytes->gdbytes + ftell(subrs);
    binary = tmpfile();
    for ( i=0; i<cidbytes->cidcnt; ++i ) {
	dump_index(binary,cidbytes->fdbytes,cidbytes->fdind[i]);
	dump_index(binary,cidbytes->gdbytes,offset);
	offset += chars->lens[i];
    }
    dump_index(binary,cidbytes->fdbytes,-1);		/* Adobe says undefined */
    dump_index(binary,cidbytes->gdbytes,offset);
    if ( ftell(binary) != (cidbytes->cidcnt+1)*(cidbytes->fdbytes+cidbytes->gdbytes))
	IError("CIDMap section the wrong length" );

    offset = (cidbytes->cidcnt+1)*(cidbytes->fdbytes+cidbytes->gdbytes) +
	    (subrtot+1) * cidbytes->gdbytes;
    for ( i=0; i<cidbytes->fdcnt; ++i ) {
	fd = &cidbytes->fds[i];
	fd->subrmapoff = ftell(binary);
	fd->sdbytes = cidbytes->gdbytes;
	fd->subrcnt = fd->subrs->next;
	for ( j=0; j<fd->subrcnt; ++j ) {
	    dump_index(binary,fd->sdbytes,offset);
	    offset += fd->subrs->lens[j];
	}
	PSCharsFree(fd->subrs);
    }
    dump_index(binary,cidbytes->gdbytes,offset);
    if ( ftell(binary) != (cidbytes->cidcnt+1)*(cidbytes->fdbytes+cidbytes->gdbytes) +
	    (subrtot+1) * cidbytes->gdbytes )
	IError("SubrMap section the wrong length" );

    buffer = malloc(8192);

    rewind(subrs);
    while ( (len=fread(buffer,1,8192,subrs))>0 )
	fwrite(buffer,1,len,binary);
    fclose(subrs);

    rewind(chrs);
    while ( (len=fread(buffer,1,8192,chrs))>0 )
	fwrite(buffer,1,len,binary);
    fclose(chrs);

    PSCharsFree(chars);
    free( cidbytes->fdind ); cidbytes->fdind = NULL;
    free( buffer );

    cidbytes->errors |= ferror(binary);
return( binary );
}

static int dumpcidstuff(FILE *out,SplineFont *cidmaster,int flags,EncMap *map,int layer) {
    int i;
    DBounds res;
    FILE *binary;
    SplineFont *sf;
    struct cidbytes cidbytes;
    char buffer[4096];
    long len;

    fprintf( out, "%%!PS-Adobe-3.0 Resource-CIDFont\n" );
    fprintf( out, "%%%%DocumentNeededResources: ProcSet (CIDInit)\n" );
/*  fprintf( out, "%%%%DocumentSuppliedResources: CIDFont (%s)\n", cidmaster->fontname ); */
/* Werner says this is inappropriate */
    fprintf( out, "%%%%IncludeResource: ProcSet (CIDInit)\n" );
    fprintf( out, "%%%%BeginResource: CIDFont (%s)\n", cidmaster->fontname );
    dumpfontcomments((DumpChar) fputc, out, cidmaster, ff_cid );

    fprintf( out, "/CIDInit /ProcSet findresource begin\n\n" );

    fprintf( out, "20 dict begin\n\n" );

    fprintf( out, "/CIDFontName /%s def\n", cidmaster->fontname );
    fprintf( out, "/CIDFontVersion %g def\n", (double)cidmaster->cidversion );
    fprintf( out, "/CIDFontType 0 def\n\n" );

    fprintf( out, "/CIDSystemInfo 3 dict dup begin\n" );
    fprintf( out, "  /Registry (%s) def\n", cidmaster->cidregistry );
    fprintf( out, "  /Ordering (%s) def\n", cidmaster->ordering );
    fprintf( out, "  /Supplement %d def\n", cidmaster->supplement );
    fprintf( out, "end def\n\n" );

    CIDLayerFindBounds(cidmaster,layer,&res);
    fprintf( out, "/FontBBox [ %g %g %g %g ] def\n",
	    floor(res.minx), floor(res.miny),
	    ceil(res.maxx), ceil(res.maxy));

    if ( cidmaster->use_uniqueid ) {
	fprintf( out,"/UIDBase %d def\n", cidmaster->uniqueid?cidmaster->uniqueid: 4000000 + (rand()&0x3ffff) );
	if ( cidmaster->xuid!=NULL && cidmaster->use_xuid ) {
	    fprintf( out,"/XUID %s def\n", cidmaster->xuid );
	    /* SFIncrementXUID(cidmaster); */ /* Unique ID managment in CID fonts is too complex for this simple trick to work */
	}
    }

    dumpfontinfo((DumpChar) fputc,out,cidmaster,ff_cid);

    if ((binary = gencidbinarydata(cidmaster,&cidbytes,flags,map,layer))==NULL )
return( 0 );

    fprintf( out, "\n/CIDMapOffset %d def\n", cidbytes.cidmapoffset );
    fprintf( out, "/FDBytes %d def\n", cidbytes.fdbytes );
    fprintf( out, "/GDBytes %d def\n", cidbytes.gdbytes );
    fprintf( out, "/CIDCount %d def\n\n", cidbytes.cidcnt );

    fprintf( out, "/FDArray %d array\n", cidbytes.fdcnt );
    for ( i=0; i<cidbytes.fdcnt; ++i ) {
	double factor;
	sf = cidmaster->subfonts[i];
	/* According to the PSRef Man, v3. only FontName, FontMatrix & Private*/
	/*  should be defined here. But adobe's example fonts have a few */
	/*  extra entries, so I'll put them in */
	fprintf( out, "dup %d\n", i );
	fprintf( out, "\n%%ADOBeginFontDict\n" );
	fprintf( out, "15 dict\n  begin\n" );
	fprintf( out, "  /FontName /%s def\n", sf->fontname );
	fprintf( out, "  /FontType 1 def\n" );
	factor = 1.0/(sf->ascent+sf->descent);
	fprintf( out, "  /FontMatrix [ %g 0 0 %g 0 0 ] def\n",
		factor, factor );
	fprintf( out, "/PaintType %d def\n", sf->strokedfont?2:0 );
	if ( sf->strokedfont )
	    fprintf( out, "/StrokeWidth %g def\n", (double) sf->strokewidth );
	fprintf( out, "\n  %%ADOBeginPrivateDict\n" );
	dumpprivatestuff((DumpChar) fputc,out,sf,&cidbytes.fds[i],flags,ff_cid,map,layer);
	fprintf( out, "\n  %%ADOEndPrivateDict\n" );
	fprintf( out, "  currentdict end\n%%ADOEndFontDict\n put\n\n" );
    }
    fprintf( out, "def\n\n" );

    fseek(binary,0,SEEK_END);
    len = ftell(binary);
    sprintf( buffer, "(Binary) %ld StartData ", len );
    fprintf( out, "%%%%BeginData: %ld Binary Bytes\n", (long) (len+strlen(buffer)));
    fputs( buffer, out );

    fseek(binary,0,SEEK_SET);
    while ( (len=fread(buffer,1,sizeof(buffer),binary))>0 )
	fwrite(buffer,1,len,out);
    cidbytes.errors |= ferror(binary);
    fclose(binary);
    free(cidbytes.fds);

    fprintf( out, "\n%%%%EndData\n%%%%EndResource\n%%%%EOF\n" );
return( !cidbytes.errors );
}

int _WritePSFont(FILE *out,SplineFont *sf,enum fontformat format,int flags,
	EncMap *map, SplineFont *fullsf,int layer) {
    int err = false;

    if ( format!=ff_cid && format!=ff_ptype3 &&
	    (othersubrs[0]==NULL || othersubrs[0][0]==NULL ||
		(othersubrs[0][1]==NULL && strcmp(othersubrs[0][0],"{}")==0)))
	flags &= ~ps_flag_noflex;

    /* make sure that all reals get output with '.' for decimal points */
    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.
    if ( (format==ff_mma || format==ff_mmb) && sf->mm!=NULL )
	sf = sf->mm->normal;
    if ( format==ff_cid )
	err = !dumpcidstuff(out,sf->subfontcnt>0?sf:sf->cidmaster,flags,map,layer);
    else {
	dumpfontdict(out,sf,format,flags,map,fullsf,layer);
	if ( format==ff_ptype0 )
	    dumptype0stuff(out,sf,map);
    }
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
    if ( ferror(out) || err)
return( 0 );

#ifdef __CygWin
    /* Modern versions of windows want the execute bit set on a ttf file */
    /*  It might also be needed for a postscript font, but I haven't checked */
    /* It isn't needed on the pfb file, but is on the pfm, so this code */
    /*  isn't really useful */
    /* I've no idea what this corresponds to in windows, nor any idea on */
    /*  how to set it from the windows UI, but this seems to work */
    {
	struct stat buf;
	fstat(fileno(out),&buf);
	fchmod(fileno(out),S_IXUSR | buf.st_mode );
    }
#endif

return( true );
}

int WritePSFont(char *fontname,SplineFont *sf,enum fontformat format,int flags,
	EncMap *map,SplineFont *fullsf,int layer) {
    FILE *out;
    int ret;

    if ( strstr(fontname,"://")!=NULL ) {
	if (( out = tmpfile())==NULL )
return( 0 );
    } else {
	if (( out=fopen(fontname,"wb"))==NULL )
return( 0 );
    }
    ret = _WritePSFont(out,sf,format,flags,map,fullsf,layer);
    if ( strstr(fontname,"://")!=NULL && ret )
	ret = URLFromFile(fontname,out);
    if ( fclose(out)==-1 )
	ret = 0;
return( ret );
}

static void dumpimageproc(FILE *file,BDFChar *bdfc,BDFFont *font) {
    SplineFont *sf = font->sf;
    double scale = (sf->ascent+sf->descent)/font->pixelsize;
    int width = bdfc->xmax-bdfc->xmin+1, height = bdfc->ymax-bdfc->ymin+1;
    int i;
    struct psfilter ps;

    /*			   wx wy ix iy urx ury setcachdevice */
    fprintf( file, "  /%s { %d 0 %d %d %d %d setcachedevice \n",
	  bdfc->sc->name, (int) rint(bdfc->width*scale),
	  (int) rint(bdfc->xmin*scale), (int) rint(bdfc->ymin*scale),
	  (int) rint((1+bdfc->xmax)*scale), (int) rint((1+bdfc->ymax)*scale));
    fprintf( file, "\t%g %g translate %g %g scale %d %d true [%d 0 0 %d 0 0] {<~\n",
	   bdfc->xmin*scale, bdfc->ymax*scale,	/* tx tx Translate */
	   width*scale, height*scale,		/* x y Scale */
	   width, height,
	   width, -height);
    InitFilter(&ps,(DumpChar) fputc,file);
    if ( bdfc->bytes_per_line==(width+7)/8 )
	FilterStr(&ps,(uint8 *) bdfc->bitmap, height*bdfc->bytes_per_line);
    else for ( i=0; i<height; ++i )
	FilterStr(&ps,(uint8 *) (bdfc->bitmap + i*bdfc->bytes_per_line),
		(width+7)/8);
    FlushFilter(&ps);
    fprintf(file,"} imagemask } bind def\n" );
}

int PSBitmapDump(char *filename,BDFFont *font, EncMap *map) {
    char buffer[300];
    FILE *file;
    int i, notdefpos, cnt;
    int ret = 0;
    SplineFont *sf = font->sf;

    if ( filename==NULL ) {
	sprintf(buffer,"%s-%d.pt3", sf->fontname, font->pixelsize );
	filename = buffer;
    }
    file = fopen(filename,"w" );
    if ( file==NULL )
	LogError( _("Can't open %s\n"), filename );
    else {
    	for ( i=0; i<font->glyphcnt; i++ ) if ( font->glyphs[i] != NULL )
    	    BCPrepareForOutput( font->glyphs[i],true );
	dumprequiredfontinfo((DumpChar) fputc, file, sf, ff_ptype3, map,NULL,ly_fore);

	cnt = 0;
	notdefpos = SFFindNotdef(sf,-2);
	for ( i=0; i<sf->glyphcnt; ++i )
	    if ( font->glyphs[i]!=NULL ) {
		if ( strcmp(font->glyphs[i]->sc->name,".notdef")!=0 )
		    ++cnt;
	    }
	++cnt;		/* one notdef entry */

	fprintf(file,"/CharProcs %d dict def\nCharProcs begin\n", cnt );

	if ( notdefpos!=-1 && font->glyphs[notdefpos]!=NULL)
	    dumpimageproc(file,font->glyphs[notdefpos],font);
	else
	    fprintf( file, "  /.notdef { %d 0 0 0 0 0 setcachedevice } bind def\n",
		sf->ascent+sf->descent );

	for ( i=0; i<sf->glyphcnt; ++i ) if ( i!=notdefpos ) {
	    if ( font->glyphs[i]!=NULL )
		dumpimageproc(file,font->glyphs[i],font);
	}
	fprintf(file,"end\ncurrentdict end\n" );
	fprintf(file,"/%s exch definefont\n", sf->fontname );
	ret = ferror(file)==0;
	if ( fclose(file)!=0 )
	    ret = false;
    	for ( i=0; i<font->glyphcnt; i++ ) if ( font->glyphs[i] != NULL )
    	    BCRestoreAfterOutput( font->glyphs[i] );
    }
return( ret );
}
