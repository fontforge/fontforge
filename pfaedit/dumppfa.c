/* Copyright (C) 2000,2001 by George Williams */
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
#include <pwd.h>
#include <stdarg.h>
#include <time.h>
#include "psfont.h"
#include "splinefont.h"

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

    fed->olddump = dumpchar;
    fed->olddata = data;
    fed->r = 55665;
    fed->hexline = 0;
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
	char *name, double *arr, int len) {
    int i;
    for ( --len; len>=0 && arr[len]==0.0; --len );
    ++len;
    if ( len&1 ) ++len;
    dumpf( dumpchar,data,"/%s [",name);
    for ( i=0; i<len; ++i )
	dumpf( dumpchar,data,"%g ", arr[i]);
    dumpstr( dumpchar,data,"]ND\n" );
}

static void dumpdblmaxarray(void (*dumpchar)(int ch,void *data), void *data,
	char *name, double *arr, int len, char *modifiers) {
    int i;
    for ( --len; len>=0 && arr[len]==0.0; --len );
    ++len;
    dumpf( dumpchar,data,"/%s [",name);
    for ( i=0; i<len; ++i )
	dumpf( dumpchar,data,"%g ", arr[i]);
    dumpf( dumpchar,data,"]%sdef\n", modifiers );
}

static void dumpdblarray(void (*dumpchar)(int ch,void *data), void *data,
	char *name, double *arr, int len, char *modifiers) {
    int i;
    dumpf( dumpchar,data,"/%s [",name);
    for ( i=0; i<len; ++i )
	dumpf( dumpchar,data,"%g ", arr[i]);
    dumpf( dumpchar,data,"]%sdef\n", modifiers );
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
    dumpf(dumpchar,data,"/Subrs %d array\n",subrs->cnt);
    for ( i=0; i<subrs->next; ++i ) {
	dumpf(dumpchar,data,"dup %d %d RD ", i, subrs->lens[i]+leniv );
	encodestrout(dumpchar,data,subrs->values[i],subrs->lens[i],leniv);
	dumpstr(dumpchar,data," NP\n");
    }
    dumpstr(dumpchar,data,"ND\n");
}

/* Dumped within the private dict to get access to ND and RD */
static void dumpcharstrings(void (*dumpchar)(int ch,void *data), void *data,
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
	GProgressNext();
    }
    dumpstr(dumpchar,data,"end readonly put\n");
}

static void dumpsplineset(void (*dumpchar)(int ch,void *data), void *data, SplineSet *spl ) {
    SplinePoint *first, *sp;

    for ( ; spl!=NULL; spl=spl->next ) {
	first = NULL;
	for ( sp = spl->first; ; sp=sp->next->to ) {
	    if ( first==NULL )
		dumpf( dumpchar, data, "\t%g %g moveto\n", sp->me.x, sp->me.y );
	    else if ( sp->prev->islinear )
		dumpf( dumpchar, data, "\t %g %g lineto\n", sp->me.x, sp->me.y );
	    else
		dumpf( dumpchar, data, "\t %g %g %g %g %g %g curveto\n",
			sp->prev->from->nextcp.x, sp->prev->from->nextcp.y,
			sp->prevcp.x, sp->prevcp.y,
			sp->me.x, sp->me.y );
	    if ( sp==first )
	break;
	    if ( first==NULL ) first = sp;
	    if ( sp->next==NULL )
	break;
	}
	dumpstr( dumpchar, data, "\tclosepath\n" );
    }
}

static int InvertTransform(double inverse[6], double transform[6]) {
    double temp[6], val;
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

static void dumpproc(void (*dumpchar)(int ch,void *data), void *data, SplineChar *sc ) {
    DBounds b;
    RefChar *ref;
    double inverse[6];

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

    dumpsplineset(dumpchar,data,sc->splines);
    if ( sc->refs!=NULL ) {
	dumpstr(dumpchar,data,"    pop -1\n" );
	for ( ref = sc->refs; ref!=NULL; ref=ref->next ) {
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
    dumpstr(dumpchar,data,"  } bind def\n" );
}

static void dumpcharprocs(void (*dumpchar)(int ch,void *data), void *data, SplineFont *sf ) {
    /* for type 3 fonts */
    int cnt, i;

    cnt = 0;
    for ( i=0; i<sf->charcnt; ++i )
	if ( sf->chars[i]!=NULL && strcmp(sf->chars[i]->name,".notdef")!=0 )
	    ++cnt;
    ++cnt;		/* one notdef entry */

    dumpf(dumpchar,data,"/CharProcs %d dict def\nCharProcs begin\n", cnt );
    i = 0;
    if ( SCWorthOutputting(sf->chars[0]) && strcmp(sf->chars[0]->name,".notdef")==0 )
	dumpproc(dumpchar,data,sf->chars[i++]);
    else
	dumpf(dumpchar, data, "  /.notdef { %d 0 0 0 0 0 setcachedevice } bind def\n",
	    sf->ascent+sf->descent );
    for ( ; i<sf->charcnt; ++i ) {
	if ( SCWorthOutputting(sf->chars[i]) )
	    dumpproc(dumpchar,data,sf->chars[i]);
	GProgressNext();
    }
    dumpstr(dumpchar,data,"end\ncurrentdict end\n" );
    dumpf(dumpchar, data, "/%s exch definefont\n", sf->fontname );
}

static struct pschars *initsubrs(int needsflex) {
    extern const uint8 *const subrs[4];
    extern const int subrslens[4];
    int i;
    struct pschars *sub;

    sub = gcalloc(1,sizeof(struct pschars));
    sub->cnt = 4;
    sub->lens = galloc(4*sizeof(int));
    sub->values = galloc(4*sizeof(uint8 *));
    for ( i=0; i<4; ++i ) {
	++sub->next;
	sub->values[i] = (uint8 *) copyn((const char *) subrs[i],subrslens[i]);
	sub->lens[i] = subrslens[i];
    }
return( sub );
}

static void dumpothersubrs(void (*dumpchar)(int ch,void *data), void *data,
	int needsflex, int needscounters ) {
    extern const char *othersubrs[];
    extern const char *othersubrsnoflex[];
    extern const char *othersubrscounters[];
    extern const char *othersubrsend[];
    int i;
    const char **subs;

    dumpstr(dumpchar,data,"/OtherSubrs \n" );
    subs = needsflex ? othersubrs : othersubrsnoflex;
    for ( i=0; subs[i]!=NULL; ++i ) {
	dumpstr(dumpchar,data,subs[i]);
	dumpchar('\n',data);
    }
    if ( needscounters ) {
	for ( i=0; othersubrscounters[i]!=NULL; ++i ) {
	    dumpstr(dumpchar,data,othersubrscounters[i]);
	    dumpchar('\n',data);
	}
    }
    for ( i=0; othersubrsend[i]!=NULL; ++i ) {
	dumpstr(dumpchar,data,othersubrsend[i]);
	dumpchar('\n',data);
    }
    dumpstr(dumpchar,data,"def\n" );
}

static void dumpprivatestuff(void (*dumpchar)(int ch,void *data), void *data, SplineFont *sf ) {
    int cnt, mi;
    double bluevalues[14], otherblues[10];
    double snapcnt[12];
    double stemsnaph[12], stemsnapv[12];
    double stdhw[1], stdvw[1];
    int i;
    static unichar_t cvtps[] = { 'C','o','n','v','e','r','t','i','n','g',' ','P','o','s','t','s','c','r','i','p','t', '\0' };
    static unichar_t saveps[] = { 'S','a','v','i','n','g',' ','P','o','s','t','s','c','r','i','p','t',' ','F','o','n','t', '\0' };
    static unichar_t autohinting[] = { 'A','u','t','o',' ','H','i','n','t','i','n','g',' ','F','o','n','t',  '\0' };
    int hasblue=0, hash=0, hasv=0, hasshift, haso, hasxuid, hasbold, haslg;
    int flex_max;
    int isbold=false;
    int iscjk;
    struct pschars *subrs, *chars;

    flex_max = SplineFontIsFlexible(sf);
    memset(&subrs,'\0',sizeof(subrs));
    subrs = initsubrs(flex_max>0);
    iscjk = SFIsCJK(sf);

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

    GProgressChangeLine1(autohinting);
    GProgressChangeStages(2+2-hasblue);
    SplineFontAutoHint(sf);
    GProgressNextStage();

    if ( !hasblue ) {
	FindBlues(sf,bluevalues,otherblues);
	GProgressNextStage();
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

    GProgressNextStage();
    GProgressChangeLine1(cvtps);
    chars = SplineFont2Chrs(sf,true,iscjk,subrs);
    GProgressNextStage();
    GProgressChangeLine1(saveps);

    dumpstr(dumpchar,data,"dup\n");
    cnt = 0;
    if ( !hasblue ) ++cnt;	/* bluevalues is required, but might be in private */
    if ( !hasshift && flex_max>=7 ) ++cnt;	/* BlueShift needs to be specified if flex wants something bigger than default */
    ++cnt;	/* minfeature is required */
    ++cnt;	/* nd is required */
    ++cnt;	/* np is required */
    if ( !hasblue && (otherblues[0]!=0 || otherblues[1]!=0) ) ++cnt;
    ++cnt;	/* password is required */
    ++cnt;	/* rd is required */
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
    ++cnt;			/* Subrs entry */
    if ( !haso && flex_max>0 ) ++cnt;
    if ( hasbold || isbold ) ++cnt;

    dumpf(dumpchar,data,"/Private %d dict dup begin\n", cnt );
	/* These guys are required and fixed */
    dumpstr(dumpchar,data,"/RD{string currentfile exch readstring pop}executeonly def\n" );
    dumpstr(dumpchar,data,"/ND{noaccess def}executeonly def\n" );
    dumpstr(dumpchar,data,"/NP{noaccess put}executeonly def\n" );
    dumpstr(dumpchar,data,"/MinFeature{16 16}ND\n" );
    dumpstr(dumpchar,data,"/password 5839 def\n" );
    if ( !hasblue ) {
	dumpblues(dumpchar,data,"BlueValues",bluevalues,14);
	if ( otherblues[0]!=0 || otherblues[1]!=0 )
	    dumpblues(dumpchar,data,"OtherBlues",otherblues,10);
    }
    if ( !hash ) {
	if ( stdhw[0]!=0 )
	    dumpf(dumpchar,data,"/StdHW [%g] def\n", stdhw[0] );
	if ( stemsnaph[0]!=0 )
	    dumpdblmaxarray(dumpchar,data,"StemSnapH",stemsnaph,12,"");
    }
    if ( !hasv ) {
	if ( stdvw[0]!=0 )
	    dumpf(dumpchar,data,"/StdVW [%g] def\n", stdvw[0] );
	if ( stemsnapv[0]!=0 )
	    dumpdblmaxarray(dumpchar,data,"StemSnapV",stemsnapv,12,"");
    }
    if ( !hasshift && flex_max>=7 )
	dumpf(dumpchar,data,"/BlueShift %d def\n", flex_max+1 );
    if ( isbold && !hasbold )
	dumpf(dumpchar,data,"/ForceBold true def\n" );
    if ( !haslg && iscjk ) 
	dumpf(dumpchar,data,"/LanguageGroup 1 def\n" );
    if ( sf->private!=NULL ) {
	for ( i=0; i<sf->private->next; ++i ) {
	    dumpf(dumpchar,data,"/%s ", sf->private->keys[i]);
	    dumpstr(dumpchar,data,sf->private->values[i]);
	    dumpstr(dumpchar,data," def\n");
	}
    }
    dumpothersubrs(dumpchar,data,flex_max>0,iscjk);
    dumpsubrs(dumpchar,data,sf,subrs);

    dumpcharstrings(dumpchar,data,sf, chars );
    dumpstr(dumpchar,data,"end put\n");

    PSCharsFree(chars);
    PSCharsFree(subrs);
    GProgressChangeStages(1);
}

static void dumpfontinfo(void (*dumpchar)(int ch,void *data), void *data, SplineFont *sf ) {
    int cnt;

    cnt = 0;
    if ( sf->familyname!=NULL ) ++cnt;
    if ( sf->fullname!=NULL ) ++cnt;
    if ( sf->copyright!=NULL ) ++cnt;
    if ( sf->weight!=NULL ) ++cnt;
    if ( sf->version!=NULL ) ++cnt;
    ++cnt;	/* ItalicAngle */
    ++cnt;	/* isfixedpitch */
    if ( sf->upos!=0 ) ++cnt;
    if ( sf->uwidth!=0 ) ++cnt;
    ++cnt;		/* em */
    ++cnt;		/* ascent */
    ++cnt;		/* descent */

    dumpf(dumpchar,data,"/FontInfo %d dict dup begin\n", cnt );
	/* These guys are required and fixed */
    if ( sf->version )
	dumpf(dumpchar,data," /version (%s) readonly def\n", sf->version );
    if ( sf->copyright )
	dumpf(dumpchar,data," /Notice (%s) readonly def\n", sf->copyright );
    if ( sf->fullname )
	dumpf(dumpchar,data," /FullName (%s) readonly def\n", sf->fullname );
    if ( sf->familyname )
	dumpf(dumpchar,data," /FamilyName (%s) readonly def\n", sf->familyname );
    if ( sf->weight )
	dumpf(dumpchar,data," /Weight (%s) readonly def\n", sf->weight );
    dumpf(dumpchar,data," /ItalicAngle %g def\n", sf->italicangle );
    dumpf(dumpchar,data," /isFixedPitch %s def\n", SFOneWidth(sf)?"true":"false" );
    if ( sf->upos )
	dumpf(dumpchar,data," /UnderlinePosition %g def\n", sf->upos );
    if ( sf->uwidth )
	dumpf(dumpchar,data," /UnderlineThickness %g def\n", sf->uwidth );
    dumpf(dumpchar,data," /em %d def\n", sf->ascent+sf->descent );
    dumpf(dumpchar,data," /ascent %d def\n", sf->ascent );
    dumpf(dumpchar,data," /descent %d def\n", sf->descent );
    dumpstr(dumpchar,data,"end readonly def\n");
}

static void dumprequiredfontinfo(void (*dumpchar)(int ch,void *data), void *data,
	SplineFont *sf, int format ) {
    int cnt, i;
    time_t now;
    double fm[6];
    char *encoding[256];
    DBounds b;
    char *pt;
    struct passwd *pwd;

    dumpf(dumpchar,data,"%%!PS-AdobeFont-1.0: %s %s\n", sf->fontname, sf->version?sf->version:"" );
    time(&now);
    dumpf(dumpchar,data,"%%%%Title: %s\n", sf->fontname);
    dumpf(dumpchar,data,"%%%%CreationDate: %s", ctime(&now));
/* Can all be commented out if no pwd routines */
    pwd = getpwuid(getuid());
    if ( pwd!=NULL && pwd->pw_gecos!=NULL && *pwd->pw_gecos!='\0' )
	dumpf(dumpchar,data,"%%%%Creator: %s\n", pwd->pw_gecos);
    else if ( pwd!=NULL && pwd->pw_name!=NULL && *pwd->pw_name!='\0' )
	dumpf(dumpchar,data,"%%%%Creator: %s\n", pwd->pw_name);
    else if ( (pt=getenv("USER"))!=NULL )
	dumpf(dumpchar,data,"%%%%Creator: %s\n", pt);
    endpwent();
/* End comment */
    if ( format==ff_ptype0 && sf->encoding_name==em_unicode )
	dumpf(dumpchar,data,"%%%%DocumentNeededResources: font ZapfDingbats\n" );
    dumpf(dumpchar,data, "%%%%DocumentSuppliedResources: font %s\n", sf->fontname );
    if ( sf->copyright!=NULL )
	dumpf(dumpchar,data,"%% %s\n", sf->copyright );
    dumpstr(dumpchar,data,"% Generated by pfaedit 1.0\n");

    cnt = 0;
    ++cnt;		/* fonttype */
    ++cnt;		/* fontmatrix */
    if ( sf->fontname!=NULL ) ++cnt;
    ++cnt;		/* fontinfo */
    ++cnt;		/* encoding */
    ++cnt;		/* fontbb */
    /*if ( fd->uniqueid!=0 )*/ ++cnt;
    ++cnt;		/* painttype */
    if ( format==ff_ptype3 ) {
	++cnt;		/* charprocs */
	++cnt;		/* buildglyph */
	++cnt;		/* buildchar */
    } else {
	++cnt;		/* private */
	++cnt;		/* chars */
    }
    if ( sf->xuid!=NULL )
	++cnt;
#if 0
    if ( fd->languagelevel>1 ) ++cnt;
    if ( fd->wmode!=0 ) ++cnt;
    if ( fd->xuid[0]!=0 ) ++cnt;
    if ( fd->painttype==2 ) ++cnt;	/* strokewidth */
#endif

    dumpf(dumpchar,data,"%d dict begin\n", cnt );
    dumpf(dumpchar,data,"/FontType %d def\n", format==ff_ptype3?3:1 );
    fm[0] = fm[3] = 1/((double) (sf->ascent+sf->descent));
    fm[1] = fm[2] = fm[4] = fm[5] = 0;
    dumpdblarray(dumpchar,data,"FontMatrix",fm,6,"readonly ");
    if ( sf->fontname!=NULL )
	dumpf(dumpchar,data,"/FontName /%s def\n", sf->fontname );
    SplineFontFindBounds(sf,&b);
    fm[0] = floor( b.minx);
    fm[1] = floor( b.miny);
    fm[2] = ceil( b.maxx);
    fm[3] = ceil( b.maxy);
    dumpdblarray(dumpchar,data,"FontBBox",fm,4,"readonly ");
    if ( (pt=PSDictHasEntry(sf->private,"UniqueID"))==NULL )
	dumpf(dumpchar,data,"/UniqueID %d def\n", 4000000 + (rand()&0x3ffff) );
    else
	dumpf(dumpchar,data,"/UniqueID %s def\n", pt );
    if ( sf->xuid!=NULL ) {
	dumpf(dumpchar,data,"/XUID %s def\n", sf->xuid );
	SFIncrementXUID(sf);
    }
    dumpf(dumpchar,data,"/PaintType %d def\n", 0/*fd->painttype*/ );
    dumpfontinfo(dumpchar,data,sf);
#if 0
    if ( fd->xuid[0]!=0 )
	dumpintmaxarray(dumpchar,data,"XUID",fd->xuid,20,"readonly ");
    if ( fd->painttype==2 )
	dumpf(dumpchar,data,"/StrokeWidth %g def\n", fd->strokewidth );
    if ( fd->languagelevel>1 )
	dumpf(dumpchar,data,"/LanguageLevel %d def\n", fd->languagelevel );
    if ( fd->wmode!=0 )
	dumpf(dumpchar,data,"/WMode %d def\n", fd->wmode );
#endif

    for ( i=0; i<256 && i<sf->charcnt; ++i )
	if ( sf->chars[i]!=NULL )
	    encoding[i] = sf->chars[i]->name;
	else
	    encoding[i] = ".notdef";
    for ( ; i<256; ++i )
	encoding[i] = ".notdef";
    if ( isStdEncoding(encoding))
	dumpstr(dumpchar,data,"/Encoding StandardEncoding def\n");
    else {
	dumpstr(dumpchar,data,"/Encoding 256 array\n" );
	for ( i=0; i<256; ++i )
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
	dumpstr(dumpchar,data,"/BuildGlyph { 2 copy exch /CharProcs get exch 2 copy known not { pop /.notdef} if get exch pop 0 exch exec 2 pop fill } bind def\n" );
    }
}

static void dumpinitialascii(void (*dumpchar)(int ch,void *data), void *data,
	SplineFont *sf ) {
    dumprequiredfontinfo(dumpchar,data,sf,ff_pfa);
    dumpstr(dumpchar,data,"currentdict end\ncurrentfile eexec\n" );
}

static void dumpencodedstuff(void (*dumpchar)(int ch,void *data), void *data,
	SplineFont *sf, int format ) {
    struct fileencryptdata fed;
    void (*func)(int ch,void *data);

    func = startfileencoding(dumpchar,data,&fed,format==ff_pfb);
    dumpprivatestuff(func,&fed,sf);
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
    dumpstr(dumpchar,data,"cleartomark\n");
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

static void dumpfontdict(FILE *out, SplineFont *sf, int format ) {

/* a pfb header consists of 6 bytes, the first is 0200, the second is a */
/*  binary/ascii flag where 1=>ascii, 2=>binary, 3=>eof??, the next four */
/*  are a count of bytes between this header and the next one. First byte */
/*  is least significant */
    if ( format==ff_pfb ) {
	FILE *temp;
	temp = tmpfile();
	dumpinitialascii((DumpChar) fputc,temp,sf );
	mkheadercopyfile(temp,out,1);
	temp = tmpfile();
	dumpencodedstuff((DumpChar) fputc,temp,sf,format);
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
	dumpinitialascii((DumpChar) (fputc),out,sf );
	dumpencodedstuff((DumpChar) (fputc),out,sf,format);
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
    if ( sf->encoding_name == em_unicode ) {
	for ( i=1; i<256; ++i ) {
	    if ( somecharsused(sf,i<<8, (i<<8)+0xff)) {
		fprintf( out, "/%sBase /%s%d [\n", sf->fontname, sf->fontname, i );
		for ( j=0; j<256; ++j )
		    fprintf( out, " /%s\n",
			    sf->chars[(i<<8)+j]!=NULL?sf->chars[(i<<8)+j]->name:
			    notdefname );
		fprintf( out, "] ReEncode\n\n" );
	    } else if ( i==0x27 ) {
		fprintf( out, "%% Add Zapf Dingbats to unicode font at 0x2700\n" );
		fprintf( out, "%%  But only if on the printer, else use notdef\n" );
		fprintf( out, "/ZapfDingbats findfont null eq \n" );
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
    } else {
	/* 94x94 encoding of oriental characters */
	for ( i=0; i<94; ++i ) {
	    if ( somecharsused(sf,i*96, i*96+95 )) {
		fprintf( out, "/%sBase /%s%d [\n", sf->fontname, sf->fontname, i );
		for ( j=0; j<32; ++j )
		    fprintf( out, " /%s\n", notdefname );
		for ( ; j<96+32; ++j )
		    fprintf( out, " /%s\n",
			    sf->chars[i*96+j-32]!=NULL?sf->chars[i*96+j-32]->name:
			    notdefname );
		for ( ; j<256; ++j )
		    fprintf( out, " /%s\n", notdefname );
		fprintf( out, "] ReEncode\n\n" );
	    }
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
    if ( sf->encoding_name == em_unicode ) {
	fprintf( out, " /%sBase findfont\n", sf->fontname );
	for ( i=1; i<256; ++i )
	    if ( somecharsused(sf,i<<8, (i<<8)+0xff) || i==0x27 )
		fprintf( out, " /%s%d findfont\n", sf->fontname, i );
	    else
		fprintf( out, " /%sNotDef findfont\n", sf->fontname );
    } else {
	for ( i=0; i<33; ++i )
	    fprintf( out, " /%sNotDef findfont\n", sf->fontname );
	for ( i=0; i<94; ++i ) {
	    if ( somecharsused(sf,i*96, i*96+95 ))
		fprintf( out, " /%s%d findfont\n", sf->fontname, i );
	    else
		fprintf( out, " /%sNotDef findfont\n", sf->fontname );
	}
	for ( i=0; i<129; ++i )
	    fprintf( out, " /%sNotDef findfont\n", sf->fontname );
    }
    fprintf( out, "  ] readonly def\n" );
    fprintf( out, "end definefont pop\n" );
    fprintf( out, "%%%%EOF\n" );
}

int WritePSFont(char *fontname,SplineFont *sf,enum fontformat format) {
    FILE *out;
    char *oldloc;

    if (( out=fopen(fontname,"w"))==NULL )
return( 0 );
    /* make sure that all doubles get output with '.' for decimal points */
    oldloc = setlocale(LC_NUMERIC,"C");
    dumpfontdict(out,sf,format);
    if ( format==ff_ptype0 )
	dumptype0stuff(out,sf);
    setlocale(LC_NUMERIC,oldloc);
    if ( ferror(out)) {
	fclose(out);
return( 0 );
    }
    if ( fclose(out)==-1 )
return( 0 );
return( 1 );
}
