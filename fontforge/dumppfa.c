/* Copyright (C) 2000-2004 by George Williams */
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
#include "pfaedit.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ustring.h>
#include <utype.h>
#include <unistd.h>
#include <locale.h>
#ifdef FONTFORGE_CONFIG_GTK
# include <gtk/gtk.h>
#else
# include <pwd.h>
#endif
#include <stdarg.h>
#include <time.h>
#include "psfont.h"
#include "splinefont.h"
#ifdef FONTFORGE_CONFIG_TYPE3
 #include <gdraw.h>		/* For image defn */
#endif

#ifdef __CygWin
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <unistd.h>
#endif

extern int autohint_before_generate;

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
	    cypher = ( randombytes[0] ^ (fed->r>>8));
	    if ( cypher<'0' || (cypher>'9' && cypher<'A') || (cypher>'F' && cypher<'a') || cypher>'f' )
	break;
	    r = (cypher + r) * c1 + c2;
	    cypher = ( randombytes[1] ^ (fed->r>>8));
	    if ( cypher<'0' || (cypher>'9' && cypher<'A') || (cypher>'F' && cypher<'a') || cypher>'f' )
	break;
	    r = (cypher + r) * c1 + c2;
	    cypher = ( randombytes[2] ^ (fed->r>>8));
	    if ( cypher<'0' || (cypher>'9' && cypher<'A') || (cypher>'F' && cypher<'a') || cypher>'f' )
	break;
	    r = (cypher + r) * c1 + c2;
	    cypher = ( randombytes[3] ^ (fed->r>>8));
	    if ( cypher<'0' || (cypher>'9' && cypher<'A') || (cypher>'F' && cypher<'a') || cypher>'f' )
	break;
	    r = (cypher + r) * c1 + c2;
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

    while ( (ch = *buf++)!='\0' ) {
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

static void dumpstrn(void (*dumpchar)(int ch,void *data), void *data, char *buf, int n) {
    while ( n-->0 )
	dumpchar(*buf++,data);
}

static void dumpf(void (*dumpchar)(int ch,void *data), void *data, char *format, ...) {
    va_list args;
    char buffer[300];

    va_start(args,format);
    vsprintf( buffer, format, args);
    va_end(args);
    dumpstr(dumpchar,data,buffer);
}

static int isStdEncoding(char *encoding[256]) {
    int i;

    for ( i=0; i<256; ++i )
	if ( strcmp(encoding[i],AdobeStandardEncoding[i])!=0 )
return( 0 );

return( 1 );
}

#if 0
static void dumpsubarray(void (*dumpchar)(int ch,void *data), void *data, char *name,
	struct pschars *subarr,int leniv) {
    int i;

    dumpf(dumpchar,data,"/%s %d array\n", name, subarr->cnt );
    for ( i=0; i<subarr->cnt; ++i ) {
	if ( subarr->values[i]==NULL )
    break;
	dumpf(dumpchar,data, "dup %d %d RD ", i, subarr->lens[i]+leniv );
	encodestrout(dumpchar,data,subarr->values[i],subarr->lens[i],leniv);
	dumpstr(dumpchar,data," NP\n");
    }
    dumpstr(dumpchar,data,"ND\n");
}
#endif

static void dumpblues(void (*dumpchar)(int ch,void *data), void *data,
	char *name, real *arr, int len, char *ND) {
    int i;
    for ( --len; len>=0 && arr[len]==0.0; --len );
    ++len;
    if ( len&1 ) ++len;
    dumpf( dumpchar,data,"/%s [",name);
    for ( i=0; i<len; ++i )
	dumpf( dumpchar,data,"%g ", arr[i]);
    dumpf( dumpchar,data,"]%s\n",ND );
}

static void dumpdblmaxarray(void (*dumpchar)(int ch,void *data), void *data,
	char *name, real *arr, int len, char *modifiers, char *ND) {
    int i;
    for ( --len; len>=0 && arr[len]==0.0; --len );
    ++len;
    dumpf( dumpchar,data,"/%s [",name);
    for ( i=0; i<len; ++i )
	dumpf( dumpchar,data,"%g ", arr[i]);
    dumpf( dumpchar,data,"]%s%s\n", modifiers, ND );
}

static void dumpdblarray(void (*dumpchar)(int ch,void *data), void *data,
	char *name, double *arr, int len, char *modifiers, int exec) {
    int i;
    dumpf( dumpchar,data,"/%s %c",name, exec? '{' : '[' );
    for ( i=0; i<len; ++i )
	dumpf( dumpchar,data,"%g ", arr[i]);
    dumpf( dumpchar,data,"%c%sdef\n", exec ? '}' : ']', modifiers );
}

#if 0
static void dumpintmaxarray(void (*dumpchar)(int ch,void *data), void *data,
	char *name, int *arr, int len, char *modifiers) {
    int i;
    for ( --len; len>=0 && arr[len]==0.0; --len );
    ++len;
    dumpf( dumpchar,data,"/%s [",name);
    for ( i=0; i<len; ++i )
	dumpf( dumpchar,data,"%d ", arr[i]);
    dumpf( dumpchar,data,"]%sdef\n", modifiers );
}
#endif

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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#if defined(FONTFORGE_CONFIG_GDRAW)
	if ( !GProgressNext())
#elif defined(FONTFORGE_CONFIG_GTK)
	if ( !gwwv_progress_next())
#endif
return( false );
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
    dumpstr(dumpchar,data,"end end\nreadonly put\n");
return( true );
}

static void dumpsplineset(void (*dumpchar)(int ch,void *data), void *data,
	SplineSet *spl, int pdfopers, int forceclose ) {
    SplinePoint *first, *sp;

    for ( ; spl!=NULL; spl=spl->next ) {
	first = NULL;
	for ( sp = spl->first; ; sp=sp->next->to ) {
	    if ( first==NULL )
		dumpf( dumpchar, data, "\t%g %g %s\n", sp->me.x, sp->me.y,
			pdfopers ? "m" : "moveto" );
	    else if ( sp->prev->knownlinear )
		dumpf( dumpchar, data, "\t %g %g %s\n", sp->me.x, sp->me.y,
			pdfopers ? "l" : "lineto" );
	    else
		dumpf( dumpchar, data, "\t %g %g %g %g %g %g %s\n",
			sp->prev->from->nextcp.x, sp->prev->from->nextcp.y,
			sp->prevcp.x, sp->prevcp.y,
			sp->me.x, sp->me.y,
			pdfopers ? "c" : "curveto" );
	    if ( sp==first )
	break;
	    if ( first==NULL ) first = sp;
	    if ( sp->next==NULL )
	break;
	}
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

#ifdef FONTFORGE_CONFIG_TYPE3
static void dumpbrush(void (*dumpchar)(int ch,void *data), void *data,
	struct brush *brush, int pdfopers ) {
    if ( brush->col!=COLOR_INHERITED ) {
	int r, g, b;
	r = (brush->col>>16)&0xff;
	g = (brush->col>>8 )&0xff;
	b = (brush->col    )&0xff;
	if ( r==g && b==g )
	    dumpf(dumpchar,data,(pdfopers ? "%g g\n" : "%g setgray\n"), r/255.0 );
	else
	    dumpf(dumpchar,data,(pdfopers ? "%g %g %g rg\n" : "%g %g %g setrgbcolor\n"),
		    r/255.0, g/255.0, b/255.0 );
    }
}

/* Grumble. PDF uses different operators for colors for stroke and fill */
static void dumppenbrush(void (*dumpchar)(int ch,void *data), void *data,
	struct brush *brush, int pdfopers ) {
    if ( brush->col!=COLOR_INHERITED ) {
	int r, g, b;
	r = (brush->col>>16)&0xff;
	g = (brush->col>>8 )&0xff;
	b = (brush->col    )&0xff;
	if ( r==g && b==g )
	    dumpf(dumpchar,data,(pdfopers ? "%g G\n" : "%g setgray\n"), r/255.0 );
	else
	    dumpf(dumpchar,data,(pdfopers ? "%g %g %g RG\n" : "%g %g %g setrgbcolor\n"),
		    r/255.0, g/255.0, b/255.0 );
    }
}

static void dumppen(void (*dumpchar)(int ch,void *data), void *data,
	struct pen *pen, int pdfopers) {
    dumppenbrush(dumpchar,data,&pen->brush,pdfopers);

    if ( pen->width!=WIDTH_INHERITED )
	dumpf(dumpchar,data,(pdfopers ? "%g w\n": "%g setlinewidth\n"), pen->width );
    if ( pen->linejoin!=lj_inherited )
	dumpf(dumpchar,data,(pdfopers ? "%d j\n": "%d setlinejoin\n"), pen->linejoin );
    if ( pen->linecap!=lc_inherited )
	dumpf(dumpchar,data,(pdfopers ? "%d J\n": "%d setlinecap\n"), pen->linecap );
    if ( pen->trans[0]!=1.0 || pen->trans[3]!=1.0 || pen->trans[1]!=0 || pen->trans[2]!=0 )
	dumpf(dumpchar,data,(pdfopers ? "[%g %g %g %g 0 0] cm\n" : "[%g %g %g %g 0 0] concat\n"),
		pen->trans[0], pen->trans[1], pen->trans[2], pen->trans[3]);
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
	int ch5, ch4, ch3, ch2, ch1;
	while ( n++<4 )
	    val<<=8;
	ch5 = val%85; val /= 85;
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

static void RunLengthFilter(struct psfilter *ps,uint8 *pt, int len ) {
    uint8 *end = pt+len, *test;

    while ( pt<end ) {
	for ( test=pt; *test==*pt && test<pt+128 && test<end; ++test );
	if ( test>pt+1 ) {
	    int len = test-pt;
	    Filter(ps,257-len);
	    Filter(ps,*pt);
	    pt = test;
    continue;
	}
	for ( test=pt; test<pt+128 && test<end; ++test ) {
	    if ( test+2<end && *test==test[1] && *test==test[2] )
	break;
	    if ( test+1<end && test-pt>64 && *test==test[1] )
	break;
	}
	if ( test!=pt ) {
	    Filter(ps,test-pt-1);
	    while ( pt<test )
		Filter(ps,*pt++);
	}
    }
}

static void PSBuildImageMonoString(void (*dumpchar)(int ch,void *data), void *data,
	struct _GImage *base) {
    int i;
    struct psfilter ps;

    InitFilter(&ps,dumpchar,data);
    if ( base->bytes_per_line==(base->width+7)/8 )
	RunLengthFilter(&ps,(uint8 *) base->data, base->height*base->bytes_per_line);
    else for ( i=0; i<base->height; ++i ) {
	RunLengthFilter(&ps,(uint8 *) (base->data + i*base->bytes_per_line),
		(base->width+7)/8);
    }
    Filter(&ps,0x80);		/* EOD mark */
    FlushFilter(&ps);
}

static void PSDrawMonoImg(void (*dumpchar)(int ch,void *data), void *data,
	struct _GImage *base,int use_imagemask) {

    dumpf(dumpchar,data, "<<\n" );
    dumpf(dumpchar,data, "  /ImageType 1\n" );
    dumpf(dumpchar,data, "  /Width %d\n", base->width );
    dumpf(dumpchar,data, "  /Height %d\n", base->height );
    dumpf(dumpchar,data, "  /ImageMatrix [%d 0 0 %d 0 %d]\n",
	    base->width, -base->height, base->height);
    dumpf(dumpchar,data, "  /MultipleDataSources false\n" );
    dumpf(dumpchar,data, "  /BitsPerComponent 1\n" );
    if ( base->trans != COLOR_UNKNOWN ) {
	if ( base->trans==0 )
	    dumpf(dumpchar,data, "  /Decode [1 0]\n" );
	else
	    dumpf(dumpchar,data, "  /Decode [0 1]\n" );
    } else {
	dumpf(dumpchar,data, "  /Decode [0 1]\n" );
    }
    dumpf(dumpchar,data, "  /Interpolate true\n" );
    dumpf(dumpchar,data, "  /DataSource currentfile /ASCII85Decode filter /RunLengthDecode filter\n" );
    dumpf(dumpchar,data, ">> %s\n",
	    use_imagemask?"imagemask":"image" );
    PSBuildImageMonoString(dumpchar,data,base);
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

static void PSBuildImageIndexString(void (*dumpchar)(int ch,void *data), void *data,
	struct _GImage *base) {
    register int i;
    register uint8 *pt, *end;
    int clut_len = base->clut->clut_len;
    struct psfilter ps;

    for ( pt=base->data, end = pt+base->height*base->bytes_per_line; pt<end; )
	if ( *pt>=clut_len ) *pt = clut_len-1;

    InitFilter(&ps,dumpchar,data);
    if ( base->bytes_per_line==base->width )
	RunLengthFilter(&ps,(uint8 *) base->data, base->height*base->bytes_per_line);
    else for ( i=0; i<base->height; ++i ) {
	RunLengthFilter(&ps,(uint8 *) (base->data + i*base->bytes_per_line),
		base->width);
    }
    Filter(&ps,0x80);		/* EOD mark */
    FlushFilter(&ps);
}

static void PSBuildImageIndexDict(void (*dumpchar)(int ch,void *data), void *data,
	struct _GImage *base) {
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
    dumpf(dumpchar,data, "  /DataSource currentfile /ASCII85Decode filter\n" );
    dumpf(dumpchar,data, ">> image\n" );
    PSBuildImageIndexString(dumpchar,data,base);
}

static void PSBuildImage24String(void (*dumpchar)(int ch,void *data), void *data,
	struct _GImage *base) {
    int i;
    register long val;
    register uint32 *pt, *end;
    struct psfilter ps;

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
	    dumpf(dumpchar,data, "currentfile /ASCII85Decode filter " );
	    dumpf(dumpchar,data, "false 3 colorimage\n" );
	PSBuildImage24String(dumpchar,data,base);
    }
}

static void dumpimage(void (*dumpchar)(int ch,void *data), void *data,
	ImageList *imgl, int use_imagemask, int pdfopers ) {
    GImage *image = imgl->image;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    if ( pdfopers )		/* I'm not supporting images yet */
return;

    dumpf( dumpchar, data, "  gsave %g %g translate %g %g scale\n",
	    imgl->xoff, imgl->yoff-imgl->yscale*base->height,
	    imgl->xscale*base->width, imgl->yscale*base->height );
    if ( base->image_type==it_mono ) {
	PSDrawMonoImg(dumpchar,data,base,use_imagemask);
    } else {
	/* Just draw the image, ignore the complexity of transparent images */
	PSDrawImg(dumpchar,data,base);
    }
    dumpstr(dumpchar,data,"grestore\n");
}
#endif

void SC_PSDump(void (*dumpchar)(int ch,void *data), void *data,
	SplineChar *sc, int refs_to_splines, int pdfopers ) {
    RefChar *ref;
    real inverse[6];
    int i,j;
    SplineSet *temp;

    for ( i=ly_fore; i<sc->layer_cnt; ++i ) {
	if ( sc->layers[i].splines!=NULL ) {
	    temp = sc->layers[i].splines;
	    if ( sc->parent->order2 ) temp = SplineSetsPSApprox(temp);
#ifdef FONTFORGE_CONFIG_TYPE3
	    if ( sc->parent->multilayer ) {
		dumpstr(dumpchar,data,pdfopers ? "q" : "gsave " );
		dumpsplineset(dumpchar,data,temp,pdfopers,sc->layers[i].dofill);
		if ( sc->layers[i].dofill && sc->layers[i].dostroke ) {
		    if ( pdfopers ) {
			dumpbrush(dumpchar,data, &sc->layers[i].fill_brush,pdfopers);
			dumppen(dumpchar,data, &sc->layers[i].stroke_pen,pdfopers);
			dumpstr(dumpchar,data, "B " );
		    } else if ( sc->layers[i].fillfirst ) {
			dumpstr(dumpchar,data, "gsave " );
			dumpbrush(dumpchar,data, &sc->layers[i].fill_brush,pdfopers);
			dumpstr(dumpchar,data,"fill grestore " );
			dumppen(dumpchar,data, &sc->layers[i].stroke_pen,pdfopers);
			dumpstr(dumpchar,data,"stroke " );
		    } else {
			dumpstr(dumpchar,data, "gsave " );
			dumppen(dumpchar,data, &sc->layers[i].stroke_pen,pdfopers);
			dumpstr(dumpchar,data,"stroke grestore " );
			dumpbrush(dumpchar,data, &sc->layers[i].fill_brush,pdfopers);
			dumpstr(dumpchar,data,"fill " );
		    }
		} else if ( sc->layers[i].dofill ) {
		    dumpbrush(dumpchar,data, &sc->layers[i].fill_brush,pdfopers);
		    dumpstr(dumpchar,data,pdfopers ? "f ": "fill " );
		} else if ( sc->layers[i].dostroke ) {
		    dumppen(dumpchar,data, &sc->layers[i].stroke_pen,pdfopers);
		    dumpstr(dumpchar,data, pdfopers ? "S ": "stroke " );
		}
		dumpstr(dumpchar,data,pdfopers ? "Q\n" : "grestore\n" );
	    } else
#endif
		dumpsplineset(dumpchar,data,temp,pdfopers,true);
	    if ( sc->parent->order2 ) SplinePointListFree(temp);
	}
	if ( sc->layers[i].refs!=NULL ) {
#ifdef FONTFORGE_CONFIG_TYPE3
	    if ( sc->parent->multilayer ) {
		dumpstr(dumpchar,data,pdfopers ? "q" : "gsave " );
		if ( sc->layers[i].dofill )
		    dumpbrush(dumpchar,data, &sc->layers[i].fill_brush, pdfopers);
	    }
#endif
	    if ( refs_to_splines ) {
		for ( ref = sc->layers[i].refs; ref!=NULL; ref=ref->next ) {
		    for ( j=0; j<ref->layer_cnt; ++j ) {
			temp = ref->layers[j].splines;
			if ( sc->parent->order2 ) temp = SplineSetsPSApprox(temp);
#ifdef FONTFORGE_CONFIG_TYPE3
			if ( sc->parent->multilayer ) {
			    dumpstr(dumpchar,data,pdfopers ? "q" : "gsave " );
			    dumpsplineset(dumpchar,data,temp,pdfopers,ref->layers[j].dofill);
			    if ( ref->layers[j].dofill && ref->layers[j].dostroke ) {
				if ( pdfopers ) {
				    dumpbrush(dumpchar,data, &ref->layers[j].fill_brush,pdfopers);
				    dumppen(dumpchar,data, &ref->layers[j].stroke_pen,pdfopers);
				    dumpstr(dumpchar,data, "B " );
				} else if ( ref->layers[j].fillfirst ) {
				    dumpstr(dumpchar,data, "gsave " );
				    dumpbrush(dumpchar,data, &ref->layers[j].fill_brush,pdfopers);
				    dumpstr(dumpchar,data,"fill grestore " );
				    dumppen(dumpchar,data, &ref->layers[j].stroke_pen,pdfopers);
				    dumpstr(dumpchar,data,"stroke " );
				} else {
				    dumpstr(dumpchar,data, "gsave " );
				    dumppen(dumpchar,data, &ref->layers[j].stroke_pen,pdfopers);
				    dumpstr(dumpchar,data,"stroke grestore " );
				    dumpbrush(dumpchar,data, &ref->layers[j].fill_brush,pdfopers);
				    dumpstr(dumpchar,data,"fill " );
				}
			    } else if ( ref->layers[j].dofill ) {
				dumpbrush(dumpchar,data, &ref->layers[j].fill_brush,pdfopers);
				dumpstr(dumpchar,data,pdfopers ? "f ": "fill " );
			    } else if ( ref->layers[j].dostroke ) {
				dumppen(dumpchar,data, &ref->layers[j].stroke_pen,pdfopers);
				dumpstr(dumpchar,data, pdfopers ? "S ": "stroke " );
			    }
			    dumpstr(dumpchar,data,pdfopers ? "Q\n" : "grestore\n" );
			} else
#endif
			    dumpsplineset(dumpchar,data,temp,pdfopers,true);
			if ( sc->parent->order2 ) SplinePointListFree(temp);
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
				    ref->transform[0], ref->transform[1], ref->transform[2],
				    ref->transform[3], ref->transform[4], ref->transform[5]);
			    else
				dumpf(dumpchar,data, "    %g %g translate ",
				    ref->transform[4], ref->transform[5]);
			    dumpf(dumpchar,data, "1 index /CharProcs get /%s get exec ",
				ref->sc->name );
			    if ( inverse[0]!=1 || inverse[1]!=0 ||
				    inverse[2]!=0 || inverse[3]!=1 )
				dumpf(dumpchar,data, "[ %g %g %g %g %g %g ] concat \n",
				    inverse[0], inverse[1], inverse[2], inverse[3], inverse[4], inverse[5]
				    );
			    else
				dumpf(dumpchar,data, "%g %g translate\n",
				    inverse[4], inverse[5] );
			}
		    } else
			dumpf(dumpchar,data, "    1 index /CharProcs get /%s get exec\n", ref->sc->name );
		}
	    }
#ifdef FONTFORGE_CONFIG_TYPE3
	    if ( sc->parent->multilayer )
		dumpstr(dumpchar,data,pdfopers ? "Q\n" : "grestore\n" );
#endif
	}
#ifdef FONTFORGE_CONFIG_TYPE3
	if ( sc->layers[i].images!=NULL && !pdfopers ) { ImageList *img;
	    dumpstr(dumpchar,data,"gsave " );
	    if ( sc->layers[i].dofill )
		dumpbrush(dumpchar,data, &sc->layers[i].fill_brush,pdfopers);
	    for ( img = sc->layers[i].images; img!=NULL; img=img->next )
		dumpimage(dumpchar,data,img,sc->layers[i].dofill,pdfopers);
	    dumpstr(dumpchar,data,"grestore\n" );
	}
#endif
    }
}

static void dumpproc(void (*dumpchar)(int ch,void *data), void *data, SplineChar *sc ) {
    DBounds b;

    SplineCharFindBounds(sc,&b);
    dumpf(dumpchar,data,"  /%s { ",sc->name);
    if ( sc->dependents!=NULL )
	dumpstr(dumpchar,data,"dup -1 ne { ");
    dumpf(dumpchar,data,"%d 0 %d %d %d %d setcachedevice", 
	    (int) sc->width, (int) floor(b.minx), (int) floor(b.miny),
	    (int) ceil(b.maxx), (int) ceil(b.maxy) );
    if ( sc->dependents!=NULL )
	dumpstr(dumpchar,data," } if\n");
    else
	dumpstr(dumpchar,data,"\n");
    SC_PSDump(dumpchar,data,sc,false,false);
    dumpstr(dumpchar,data,"  } bind def\n" );
}

static int dumpcharprocs(void (*dumpchar)(int ch,void *data), void *data, SplineFont *sf ) {
    /* for type 3 fonts */
    int cnt, i;

    cnt = 0;
    for ( i=0; i<sf->charcnt; ++i )
	if ( sf->chars[i]!=NULL && strcmp(sf->chars[i]->name,".notdef")!=0 )
	    ++cnt;
    ++cnt;		/* one notdef entry */

    dumpf(dumpchar,data,"/CharProcs %d dict def\nCharProcs begin\n", cnt );
    i = 0;
    if ( SCWorthOutputting(sf->chars[0]) && SCIsNotdef(sf->chars[0],-1 ))
	dumpproc(dumpchar,data,sf->chars[i++]);
    else
	dumpf(dumpchar, data, "  /.notdef { %d 0 0 0 0 0 setcachedevice } bind def\n",
	    sf->ascent+sf->descent );
    for ( ; i<sf->charcnt; ++i ) {
	if ( SCWorthOutputting(sf->chars[i]) )
	    dumpproc(dumpchar,data,sf->chars[i]);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#if defined(FONTFORGE_CONFIG_GDRAW)
	if ( !GProgressNext())
#elif defined(FONTFORGE_CONFIG_GTK)
	if ( !gwwv_progress_next())
#endif
return( false );
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
    dumpstr(dumpchar,data,"end\ncurrentdict end\n" );
    dumpf(dumpchar, data, "/%s exch definefont\n", sf->fontname );
return( true );
}

static struct pschars *initsubrs(int needsflex,MMSet *mm) {
    extern const uint8 *const subrs[10];
    extern const int subrslens[10];
    int i;
    struct pschars *sub;

    sub = gcalloc(1,sizeof(struct pschars));
    sub->cnt = 10;
    sub->lens = galloc(10*sizeof(int));
    sub->values = galloc(10*sizeof(uint8 *));
    for ( i=0; i<5; ++i ) {
	++sub->next;
	sub->values[i] = (uint8 *) copyn((const char *) subrs[i],subrslens[i]);
	sub->lens[i] = subrslens[i];
    }
    if ( mm!=NULL ) {
	static int cnts[] = { 1,2,3,4,6 };
	for ( ; i<10 && cnts[i-5]*mm->instance_count<22; ++i ) {
	    ++sub->next;
	    sub->values[i] = (uint8 *) copyn((const char *) subrs[i],subrslens[i]);
	    sub->values[i][0] += cnts[i-5]*mm->instance_count;
	    sub->lens[i] = subrslens[i];
	}
    }
return( sub );
}

static void dumpothersubrs(void (*dumpchar)(int ch,void *data), void *data,
	int incid, int needsflex, int needscounters, MMSet *mm ) {
    extern const char *othersubrs[];
    extern const char *othersubrsnoflex[];
    extern const char *othersubrscounters[];
    extern const char *othersubrsnocounters[];
    extern const char *othersubrsend[];
    extern const char *cid_othersubrs[];
    int i;
    const char **subs;

    dumpstr(dumpchar,data,"/OtherSubrs \n" );
    subs = incid ? cid_othersubrs : needsflex ? othersubrs : othersubrsnoflex;
    for ( i=0; subs[i]!=NULL; ++i ) {
	dumpstr(dumpchar,data,subs[i]);
	dumpchar('\n',data);
    }
    if (!incid) { 
	if ( needscounters ) {
	    for ( i=0; othersubrscounters[i]!=NULL; ++i ) {
                dumpstr(dumpchar,data,othersubrscounters[i]);
                dumpchar('\n',data);
            }
        } else if ( mm!=NULL ) {
	    for ( i=0; othersubrsnocounters[i]!=NULL; ++i ) {
                dumpstr(dumpchar,data,othersubrsnocounters[i]);
                dumpchar('\n',data);
            }
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
        for ( i=0; othersubrsend[i]!=NULL; ++i ) {
            dumpstr(dumpchar,data,othersubrsend[i]);
            dumpchar('\n',data);
        }
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

static int dumpprivatestuff(void (*dumpchar)(int ch,void *data), void *data,
	SplineFont *sf, struct fddata *incid, int flags, enum fontformat format ) {
    int cnt, mi;
    real bluevalues[14], otherblues[10];
    real snapcnt[12];
    real stemsnaph[12], stemsnapv[12];
    real stdhw[1], stdvw[1];
    int flex_max;
    int i;
    int hasblue=0, hash=0, hasv=0, hasshift, hasxuid, hasbold, haslg;
    int isbold=false;
    int iscjk;
    struct pschars *subrs, *chars;
    char *ND="def";
    MMSet *mm = (format==ff_mma || format==ff_mmb)? sf->mm : NULL;

    if ( incid==NULL ) {
	flex_max = SplineFontIsFlexible(sf,flags);
	if ( (subrs = initsubrs(flex_max>0,mm))==NULL )
return( false );
	iscjk = SFIsCJK(sf);
    } else {
	flex_max = incid->flexmax;
	iscjk = incid->iscjk;
    }

    hasbold = PSDictHasEntry(sf->private,"ForceBold")!=NULL;
    hasblue = PSDictHasEntry(sf->private,"BlueValues")!=NULL;
    hash = PSDictHasEntry(sf->private,"StdHW")!=NULL;
    hasv = PSDictHasEntry(sf->private,"StdVW")!=NULL;
    hasshift = PSDictHasEntry(sf->private,"BlueShift")!=NULL;
    hasxuid = PSDictHasEntry(sf->private,"XUID")!=NULL;
    haslg = PSDictHasEntry(sf->private,"LanguageGroup")!=NULL;
    if ( sf->weight!=NULL &&
	    (strstrmatch(sf->weight,"Bold")!=NULL ||
	     strstrmatch(sf->weight,"Black")!=NULL))
	isbold = true;

#if defined(FONTFORGE_CONFIG_GDRAW)
    GProgressChangeStages(2+2-hasblue);
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_change_stages(2+2-hasblue);
#endif
    if ( autohint_before_generate && SFNeedsAutoHint(sf) &&
	    !(flags&ps_flag_nohints)) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	GProgressChangeLine1R(_STR_AutoHintingFont);
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_change_line1(_("Auto Hinting Font..."));
#endif
	SplineFontAutoHint(sf);
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#if defined(FONTFORGE_CONFIG_GDRAW)
    if ( !GProgressNextStage())
#elif defined(FONTFORGE_CONFIG_GTK)
    if ( !gwwv_progress_next_stage())
#endif
return( false );
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

    if ( !hasblue ) {
	FindBlues(sf,bluevalues,otherblues);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#if defined(FONTFORGE_CONFIG_GDRAW)
	if ( !GProgressNextStage())
#elif defined(FONTFORGE_CONFIG_GTK)
	if ( !gwwv_progress_next_stage())
#endif
return( false );
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }

    if ( !hash || !hasv )
    stdhw[0] = stdvw[0] = 0;
    if ( !hash ) {
	FindHStems(sf,stemsnaph,snapcnt);
	mi = -1;
	for ( i=0; stemsnaph[i]!=0 && i<12; ++i )
	    if ( mi==-1 ) mi = i;
	    else if ( snapcnt[i]>snapcnt[mi] ) mi = i;
	if ( mi!=-1 ) stdhw[0] = stemsnaph[mi];
    }

    if ( !hasv ) {
	FindVStems(sf,stemsnapv,snapcnt);
	mi = -1;
	for ( i=0; stemsnapv[i]!=0 && i<12; ++i )
	    if ( mi==-1 ) mi = i;
	    else if ( snapcnt[i]>snapcnt[mi] ) mi = i;
	if ( mi!=-1 ) stdvw[0] = stemsnapv[mi];
    }

    if ( incid==NULL ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	GProgressNextStage();
	GProgressChangeLine1R(_STR_CvtPS);
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_next_stage();
	gwwv_progress_change_line1(_("Converting Postscript"));
#endif
	if ( (chars = SplineFont2Chrs(sf,iscjk,subrs,flags,format))==NULL )
return( false );
#if defined(FONTFORGE_CONFIG_GDRAW)
	GProgressNextStage();
	GProgressChangeLine1R(_STR_SavingPSFont);
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_next_stage();
	gwwv_progress_change_line1(_("Saving Postscript Font"));
#endif
    }

    if ( incid==NULL ) dumpstr(dumpchar,data,"dup\n");
    cnt = 0;
    if ( !hasblue ) ++cnt;	/* bluevalues is required, but might be in private */
    if ( !hasshift && flex_max>=7 ) ++cnt;	/* BlueShift needs to be specified if flex wants something bigger than default */
    ++cnt;	/* minfeature is required */
    if ( !hasblue && (otherblues[0]!=0 || otherblues[1]!=0) ) ++cnt;
    ++cnt;	/* password is required */
    if ( sf->tempuniqueid!=0 )
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
	    dumpf(dumpchar,data,"/StdHW [%g] %s\n", stdhw[0], ND );
	if ( stemsnaph[0]!=0 )
	    dumpdblmaxarray(dumpchar,data,"StemSnapH",stemsnaph,12,"", ND);
    }
    if ( !hasv ) {
	if ( stdvw[0]!=0 )
	    dumpf(dumpchar,data,"/StdVW [%g] %s\n", stdvw[0],ND );
	if ( stemsnapv[0]!=0 )
	    dumpdblmaxarray(dumpchar,data,"StemSnapV",stemsnapv,12,"", ND);
    }
    if ( !hasshift && flex_max>=7 )
	dumpf(dumpchar,data,"/BlueShift %d def\n", flex_max+1 );
    if ( isbold && !hasbold )
	dumpf(dumpchar,data,"/ForceBold true def\n" );
    if ( !haslg && iscjk ) 
	dumpf(dumpchar,data,"/LanguageGroup 1 def\n" );
    if ( sf->tempuniqueid!=0 && sf->tempuniqueid!=-1 )
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

#if defined(FONTFORGE_CONFIG_GDRAW)
    GProgressChangeStages(1);
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_change_stages(1);
#endif
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
#if 0
	++cnt;		/* em */
	++cnt;		/* descent */
#endif
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
	dumpf(dumpchar,data," /ItalicAngle %g def\n", sf->italicangle );
	dumpf(dumpchar,data," /isFixedPitch %s def\n", SFOneWidth(sf)!=-1?"true":"false" );
	if ( sf->upos )
	    dumpf(dumpchar,data," /UnderlinePosition %g def\n", sf->upos );
	if ( sf->uwidth )
	    dumpf(dumpchar,data," /UnderlineThickness %g def\n", sf->uwidth );
	if ( sf->ascent != 8*(sf->ascent+sf->descent)/10 )
	    dumpf(dumpchar,data," /ascent %d def\n", sf->ascent );
#if 0
	dumpf(dumpchar,data," /em %d def\n", sf->ascent+sf->descent );
	dumpf(dumpchar,data," /descent %d def\n", sf->descent );
#endif
    }
    if ( format==ff_mma || format==ff_mmb ) {
	MMSet *mm = sf->mm;
	int j,k;

	dumpstr(dumpchar,data," /BlendDesignPositions [" );
	for ( j=0; j<mm->instance_count; ++j ) {
	    dumpstr(dumpchar,data," [" );
	    for ( k=0; k<mm->axis_count; ++k )
		dumpf(dumpchar,data,"%g ", mm->positions[j*mm->axis_count+k]);
	    dumpstr(dumpchar,data,"]" );
	}
	dumpstr(dumpchar,data," ] def\n" );

	dumpstr(dumpchar,data," /BlendDesignMap [" );
	for ( k=0; k<mm->axis_count; ++k ) {
	    dumpstr(dumpchar,data," [" );
	    for ( j=0; j<mm->axismaps[k].points; ++j )
		dumpf(dumpchar,data,"[%g %g] ",
			mm->axismaps[k].designs[j], mm->axismaps[k].blends[j]);
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

const char *GetAuthor(void) {
#if defined( FONTFORGE_CONFIG_GTK )
    static char *author;

    if ( author==NULL )
	author = g_get_real_name();
return( author );
#else
    struct passwd *pwd;
    static char author[200] = { '\0' };
    const char *ret = NULL, *pt;

    if ( author[0]!='\0' )
return( author );

/* Can all be commented out if no pwd routines */
    pwd = getpwuid(getuid());
#ifndef __VMS
    if ( pwd!=NULL && pwd->pw_gecos!=NULL && *pwd->pw_gecos!='\0' ) {
	strncpy(author,pwd->pw_gecos,sizeof(author));
	author[sizeof(author)-1] = '\0';
	ret = author;
    } else if ( pwd!=NULL && pwd->pw_name!=NULL && *pwd->pw_name!='\0' ) {
#else
    if ( pwd!=NULL && pwd->pw_name!=NULL && *pwd->pw_name!='\0' ) {
#endif
	strncpy(author,pwd->pw_name,sizeof(author));
	author[sizeof(author)-1] = '\0';
	ret = author;
    } else if ( (pt=getenv("USER"))!=NULL ) {
	strncpy(author,pt,sizeof(author));
	author[sizeof(author)-1] = '\0';
	ret = author;
    }
    endpwent();
/* End comment */
return( ret );
#endif
}

static void dumprequiredfontinfo(void (*dumpchar)(int ch,void *data), void *data,
	SplineFont *sf, int format ) {
    int cnt, i;
    time_t now;
    double fm[6];
    char *encoding[256];
    DBounds b;
    int uniqueid;
    const char *author = GetAuthor();
    extern char *source_version_str;

    dumpf(dumpchar,data,"%%!PS-AdobeFont-1.0: %s %s\n", sf->fontname, sf->version?sf->version:"" );
    time(&now);
    dumpf(dumpchar,data,"%%%%Title: %s\n", sf->fontname);
    dumpf(dumpchar,data,"%%%%CreationDate: %s", ctime(&now));
    if ( author!=NULL )
	dumpf(dumpchar,data,"%%%%Creator: %s\n", author);
    if ( format==ff_ptype0 && (sf->encoding_name==em_unicode || sf->encoding_name==em_unicode4))
	dumpf(dumpchar,data,"%%%%DocumentNeededResources: font ZapfDingbats\n" );
    dumpf(dumpchar,data, "%%%%DocumentSuppliedResources: font %s\n", sf->fontname );
    if ( sf->copyright!=NULL ) {
	char *pt = strchr(sf->copyright,'\n');
	if ( pt==NULL ) pt = sf->copyright + strlen(sf->copyright);
	if ( pt-sf->copyright>70 ) pt = sf->copyright+70;
	dumpf(dumpchar,data,"%% %.*s\n", pt-sf->copyright, sf->copyright );
    }
    if ( sf->comments!=NULL )
	dumpascomments(dumpchar,data,sf->comments);
    dumpf(dumpchar,data,"%% Generated by FontForge %s (http://fontforge.sf.net/)\n",
	    source_version_str);
    dumpstr(dumpchar,data,"%%EndComments\n");

    cnt = 0;
    ++cnt;		/* FID, added by definefont and not by us */
    ++cnt;		/* fonttype */
    ++cnt;		/* fontmatrix */
    if ( sf->fontname!=NULL ) ++cnt;
    ++cnt;		/* fontinfo */
    ++cnt;		/* encoding */
    ++cnt;		/* fontbb */
    if ( sf->uniqueid!=-1 ) ++cnt;
    ++cnt;		/* painttype */
    if ( format==ff_ptype3 ) {
	++cnt;		/* charprocs */
	++cnt;		/* buildglyph */
	++cnt;		/* buildchar */
    } else {
	++cnt;		/* private */
	++cnt;		/* chars */
    }
    if ( sf->xuid!=NULL ) ++cnt;
    if ( format==ff_mma || format==ff_mmb )
	cnt += 7;

    if ( sf->uniqueid==0 )
	uniqueid = 4000000 + (rand()&0x3ffff);
    else
	uniqueid = sf->uniqueid ;
    sf->tempuniqueid = uniqueid;

    if ( format!=ff_ptype3 && uniqueid!=-1 ) {
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
    SplineFontFindBounds(sf,&b);
    fm[0] = floor( b.minx);
    fm[1] = floor( b.miny);
    fm[2] = ceil( b.maxx);
    fm[3] = ceil( b.maxy);
    dumpdblarray(dumpchar,data,"FontBBox",fm,4,"readonly ", true);
    if ( uniqueid!=-1 )
	dumpf(dumpchar,data,"/UniqueID %d def\n", uniqueid );
    if ( sf->xuid!=NULL ) {
	dumpf(dumpchar,data,"/XUID %s def\n", sf->xuid );
	if ( sf->changed_since_xuidchanged )
	    SFIncrementXUID(sf);
    }
    dumpf(dumpchar,data,"/PaintType %d def\n", 0/*fd->painttype*/ );
    dumpfontinfo(dumpchar,data,sf,format);
    if ( format==ff_mma || format==ff_mmb ) {
	MMSet *mm = sf->mm;
	int j,k;
	DBounds mb[16];
	extern const char *mmfindfont[], *makeblendedfont[];

	dumpstr(dumpchar,data," /WeightVector [" );
	for ( j=0; j<mm->instance_count; ++j ) {
	    dumpf(dumpchar,data,"%g ", mm->defweights[j]);
	}
	dumpstr(dumpchar,data," ] def\n" );

	dumpstr(dumpchar,data," /$Blend {" );
	if ( mm->instance_count==2 )
	    dumpf(dumpchar,data,"%g mul add", mm->defweights[1]);
	else {
	    dumpf(dumpchar,data,"%g mul exch", mm->defweights[1]);
	    for ( j=2; j<mm->instance_count-1; ++j )
		dumpf(dumpchar,data,"%g mul add exch", mm->defweights[j]);
	    dumpf(dumpchar,data,"%g mul add add", mm->defweights[j]);
	}
	dumpstr(dumpchar,data," } bind def\n" );

	dumpstr(dumpchar,data," /Blend 3 dict dup begin\n" );
	for ( j=0; j<mm->instance_count; ++j )
	    SplineFontFindBounds(mm->instances[j],&mb[j]);
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

    for ( i=0; i<256 && i<sf->charcnt; ++i )
	if ( SCWorthOutputting(sf->chars[i]) )
	    encoding[i] = SCDuplicate(sf->chars[i])->name;
	else
	    encoding[i] = ".notdef";
    for ( ; i<256; ++i )
	encoding[i] = ".notdef";
    if ( sf->encoding_name==em_adobestandard || isStdEncoding(encoding) )
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
	dumpf(dumpchar,data,"/BuildGlyph { 2 copy exch /CharProcs get exch 2 copy known not { pop /.notdef} if get exch pop 0 exch exec pop pop %s} bind def\n",
		sf->multilayer ? "" : "fill " );
    }
}

static void dumpinitialascii(void (*dumpchar)(int ch,void *data), void *data,
	SplineFont *sf, int format ) {
    dumprequiredfontinfo(dumpchar,data,sf,format);
    dumpstr(dumpchar,data,"currentdict end\ncurrentfile eexec\n" );
}

static void dumpencodedstuff(void (*dumpchar)(int ch,void *data), void *data,
	SplineFont *sf, int format, int flags ) {
    struct fileencryptdata fed;
    void (*func)(int ch,void *data);

    func = startfileencoding(dumpchar,data,&fed,format==ff_pfb || format==ff_mmb);
    dumpprivatestuff(func,&fed,sf,NULL,flags,format);
    if ( format==ff_ptype0 ) {
	dumpstr(func,&fed, "/" );
	dumpstr(func,&fed, sf->fontname );
	dumpstr(func,&fed,"Base exch definefont pop\n mark currentfile closefile\n" );
    } else
	dumpstr(func,&fed,"dup/FontName get exch definefont pop\n mark currentfile closefile\n" );
}

static void dumpfinalascii(void (*dumpchar)(int ch,void *data), void *data) {
    int i;

    /* output 512 zeros */
    dumpchar('\n',data);
    for ( i = 0; i<8; ++i )
	dumpstr(dumpchar,data,"0000000000000000000000000000000000000000000000000000000000000000\n");
    dumpstr(dumpchar,data,"cleartomark\n{restore}if");
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

static void dumpfontdict(FILE *out, SplineFont *sf, int format, int flags ) {

/* a pfb header consists of 6 bytes, the first is 0200, the second is a */
/*  binary/ascii flag where 1=>ascii, 2=>binary, 3=>eof??, the next four */
/*  are a count of bytes between this header and the next one. First byte */
/*  is least significant */
    if ( format==ff_pfb || format==ff_mmb ) {
	FILE *temp;
	temp = tmpfile();
	dumpinitialascii((DumpChar) fputc,temp,sf,format );
	mkheadercopyfile(temp,out,1);
	temp = tmpfile();
	dumpencodedstuff((DumpChar) fputc,temp,sf,format,flags);
	mkheadercopyfile(temp,out,2);
	temp = tmpfile();
	dumpfinalascii((DumpChar) fputc,temp);
	mkheadercopyfile(temp,out,1);
/* final header, 3=>eof??? */
	dumpstrn((DumpChar) fputc,out,"\200\003",2);
    } else if ( format==ff_ptype3 ) {
	dumprequiredfontinfo((DumpChar) fputc,out,sf,ff_ptype3);
	dumpcharprocs((DumpChar) fputc,out,sf);
    } else {
	dumpinitialascii((DumpChar) (fputc),out,sf,format );
	dumpencodedstuff((DumpChar) (fputc),out,sf,format,flags);
	dumpfinalascii((DumpChar) (fputc),out);
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

static char *dumpnotdefenc(FILE *out,SplineFont *sf) {
    char *notdefname;
    int i;

    if ( 0xfffd<sf->charcnt && sf->chars[0xfffd]!=NULL )
	notdefname = sf->chars[0xfffd]->name;
    else
	notdefname = ".notdef";
    fprintf( out, "/%sBase /%sNotDef [\n", sf->fontname, sf->fontname );
    for ( i=0; i<256; ++i )
	fprintf( out, " /%s\n", notdefname );
    fprintf( out, "] ReEncode\n\n" );
return( notdefname );
}

static int somecharsused(SplineFont *sf, int bottom, int top) {
    int i;

    for ( i=bottom; i<=top && i<sf->charcnt; ++i ) {
	if ( SCWorthOutputting(sf->chars[i]) )
return( true );
    }
return( false );
}

static void dumptype0stuff(FILE *out,SplineFont *sf) {
    char *notdefname;
    int i,j;
    extern char *zapfnomen[];
    extern int8 zapfexists[];

    dumpreencodeproc(out);
    notdefname = dumpnotdefenc(out,sf);
    for ( i=1; i<256; ++i ) {
	if ( somecharsused(sf,i<<8, (i<<8)+0xff)) {
	    fprintf( out, "/%sBase /%s%d [\n", sf->fontname, sf->fontname, i );
	    for ( j=0; j<256 && (i<<8)+j<sf->charcnt; ++j )
		fprintf( out, " /%s\n",
			sf->chars[(i<<8)+j]!=NULL?sf->chars[(i<<8)+j]->name:
			notdefname );
	    for ( ; j<256; ++j )
		fprintf( out, " /%s\n", notdefname );
	    fprintf( out, "] ReEncode\n\n" );
	} else if ( i==0x27 && sf->encoding_name==em_unicode ) {
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

    fprintf( out, "/%s 20 dict dup begin\n", sf->fontname );
    fprintf( out, "/FontInfo /%sBase findfont /FontInfo get def\n", sf->fontname );
    fprintf( out, "/FontName /%s def\n", sf->fontname );
    fprintf( out, "/PaintType 0 def\n" );
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
	if ( somecharsused(sf,i<<8, (i<<8)+0xff) || (i==0x27 && sf->encoding_name==em_unicode) )
	    fprintf( out, " /%s%d findfont\n", sf->fontname, i );
	else
	    fprintf( out, " /%sNotDef findfont\n", sf->fontname );
    fprintf( out, "  ] readonly def\n" );
    fprintf( out, "end definefont pop\n" );
    fprintf( out, "%%%%EOF\n" );
}

static void dumpt1str(FILE *binary,uint8 *data, int len, int leniv) {
    if ( leniv==-1 )
	fwrite(data,sizeof(1),len,binary);
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

static FILE *gencidbinarydata(SplineFont *cidmaster,struct cidbytes *cidbytes,int flags) {
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
    cidbytes->fds = gcalloc(cidbytes->fdcnt,sizeof(struct fddata));
    for ( i=0; i<cidbytes->fdcnt; ++i ) {
	sf = cidmaster->subfonts[i];
	fd = &cidbytes->fds[i];
	fd->flexmax = SplineFontIsFlexible(sf,flags);
	fd->subrs = initsubrs(fd->flexmax>0,NULL);
	if ( fd->subrs==NULL ) {
	    int j;
	    for ( j=0; j<i; ++j )
		PSCharsFree(cidbytes->fds[j].subrs);
	    free( cidbytes->fds );
return( NULL );
	}
	fd->iscjk = SFIsCJK(sf);
	pt = PSDictHasEntry(sf->private,"lenIV");
	if ( pt!=NULL )
	    fd->leniv = strtol(pt,NULL,10);
	else
	    fd->leniv = 4;
    }
#if defined(FONTFORGE_CONFIG_GDRAW)
    GProgressChangeLine1R(_STR_CvtPS);
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_change_line1(_("Converting Postscript"));
#endif
    if ( (chars = CID2Chrs(cidmaster,cidbytes,flags))==NULL )
return( NULL );
#if defined(FONTFORGE_CONFIG_GDRAW)
    GProgressNextStage();
    GProgressChangeLine1R(_STR_SavingPSFont);
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_next_stage();
    gwwv_progress_change_line1(_("Saving Postscript Font"));
#endif

    chrs = tmpfile();
    for ( i=0; i<chars->next; ++i ) {
	if ( chars->lens[i]!=0 ) {
	    leniv = cidbytes->fds[cidbytes->fdind[i]].leniv;
	    dumpt1str(chrs,chars->values[i],chars->lens[i],leniv);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#if defined(FONTFORGE_CONFIG_GDRAW)
	    if ( !GProgressNext()) {
#elif defined(FONTFORGE_CONFIG_GTK)
	    if ( !gwwv_progress_next()) {
#endif
		fclose(chrs);
return( NULL );
	    }
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
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

    buffer = galloc(8192);

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

static int dumpcidstuff(FILE *out,SplineFont *cidmaster,int flags) {
    int i;
    DBounds res;
    FILE *binary;
    SplineFont *sf;
    struct cidbytes cidbytes;
    char buffer[4096];
    long len;
    extern char *source_version_str;

    fprintf( out, "%%!PS-Adobe-3.0 Resource-CIDFont\n" );
    fprintf( out, "%%%%DocumentNeededResources: ProcSet (CIDInit)\n" );
    fprintf( out, "%%%%DocumentSuppliedResources: CIDFont (%s)\n", cidmaster->fontname );
    fprintf( out, "%%%%IncludeResource: ProcSet (CIDInit)\n" );
    fprintf( out, "%%%%BeginResource: CIDFont (%s)\n", cidmaster->fontname );
    fprintf( out, "%%%%Title: (%s %s %s %d)\n",
	    cidmaster->fontname,
	    cidmaster->cidregistry, cidmaster->ordering, cidmaster->supplement );
    fprintf( out, "%%%%Version: %g\n\n", cidmaster->cidversion );
    if ( cidmaster->copyright!=NULL ) {
	char *pt = strchr(cidmaster->copyright,'\n');
	if ( pt==NULL ) pt = cidmaster->copyright + strlen(cidmaster->copyright);
	if ( pt-cidmaster->copyright>70 ) pt = cidmaster->copyright+70;
	fprintf( out,"%% %.*s\n", pt-cidmaster->copyright, cidmaster->copyright );
    }
    fprintf( out,"%% Generated by FontForge %s (http://fontforge.sf.net/)\n",
	    source_version_str);

    fprintf( out, "/CIDInit /ProcSet findresource begin\n\n" );

    fprintf( out, "20 dict begin\n\n" );

    fprintf( out, "/CIDFontName /%s def\n", cidmaster->fontname );
    fprintf( out, "/CIDFontVersion %g def\n", cidmaster->cidversion );
    fprintf( out, "/CIDFontType 0 def\n\n" );

    fprintf( out, "/CIDSystemInfo 3 dict dup begin\n" );
    fprintf( out, "  /Registry (%s) def\n", cidmaster->cidregistry );
    fprintf( out, "  /Ordering (%s) def\n", cidmaster->ordering );
    fprintf( out, "  /Supplement %d def\n", cidmaster->supplement );
    fprintf( out, "end def\n\n" );

    CIDFindBounds(cidmaster,&res);
    fprintf( out, "/FontBBox [ %g %g %g %g ] def\n",
	    floor(res.minx), floor(res.miny),
	    ceil(res.maxx), ceil(res.maxy));

    fprintf( out,"/UIDBase %d def\n", cidmaster->uniqueid?cidmaster->uniqueid: 4000000 + (rand()&0x3ffff) );
    if ( cidmaster->xuid!=NULL ) {
	fprintf( out,"/XUID %s def\n", cidmaster->xuid );
	/* SFIncrementXUID(cidmaster); */ /* Unique ID managment in CID fonts is too complex for this simple trick to work */
    }

    dumpfontinfo((DumpChar) fputc,out,cidmaster,ff_cid);

    if ((binary = gencidbinarydata(cidmaster,&cidbytes,flags))==NULL )
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
	fprintf( out, "14 dict\n  begin\n" );
	fprintf( out, "  /FontName /%s def\n", sf->fontname );
	fprintf( out, "  /FontType 1 def\n" );
	factor = 1.0/(sf->ascent+sf->descent);
	fprintf( out, "  /FontMatrix [ %g 0 0 %g 0 0 ] def\n",
		factor, factor );
	fprintf( out, "  /PaintType 0 def\n" );
	fprintf( out, "\n  %%ADOBeginPrivateDict\n" );
	dumpprivatestuff((DumpChar) fputc,out,sf,&cidbytes.fds[i],flags,ff_cid);
	fprintf( out, "\n  %%ADOEndPrivateDict\n" );
	fprintf( out, "  currentdict end\n%%ADOEndFontDict\n put\n\n" );
    }
    fprintf( out, "def\n\n" );

    fseek(binary,0,SEEK_END);
    len = ftell(binary);
    sprintf( buffer, "(Binary) %ld StartData ", len );
    fprintf( out, "%%%%BeginData: %ld Binary Bytes\n", (long) (len+strlen(buffer)));
    fprintf( out, buffer );

    fseek(binary,0,SEEK_SET);
    while ( (len=fread(buffer,1,sizeof(buffer),binary))>0 )
	fwrite(buffer,1,len,out);
    cidbytes.errors |= ferror(binary);
    fclose(binary);
    free(cidbytes.fds);

    fprintf( out, "\n%%%%EndData\n%%%%EndResource\n%%%%EOF\n" );
return( !cidbytes.errors );
}

int _WritePSFont(FILE *out,SplineFont *sf,enum fontformat format,int flags) {
    char *oldloc;
    int err = false;

    /* make sure that all reals get output with '.' for decimal points */
    oldloc = setlocale(LC_NUMERIC,"C");
    if ( (format==ff_mma || format==ff_mmb) && sf->mm!=NULL )
	sf = sf->mm->normal;
    if ( format==ff_cid )
	err = !dumpcidstuff(out,sf->subfontcnt>0?sf:sf->cidmaster,flags);
    else {
	dumpfontdict(out,sf,format,flags);
	if ( format==ff_ptype0 )
	    dumptype0stuff(out,sf);
    }
    setlocale(LC_NUMERIC,oldloc);
    if ( ferror(out) || err)
return( 0 );

#ifdef __CygWin
    /* Modern versions of windows want the execute bit set on a ttf file */
    /*  It might also be needed for a postscript font, but I haven't checked */
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

int WritePSFont(char *fontname,SplineFont *sf,enum fontformat format,int flags) {
    FILE *out;
    int ret;

    if (( out=fopen(fontname,"wb"))==NULL )
return( 0 );
    ret = _WritePSFont(out,sf,format,flags);
    if ( fclose(out)==-1 )
	ret = 0;
return( ret );
}
