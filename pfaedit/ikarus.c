/* Copyright (C) 2002 by George Williams */
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
#include "pfaeditui.h"
#include <utype.h>
#include <string.h>
#include <ustring.h>

/* Ikarus formats defined in Peter Karow's book "Digital formats for typefaces" */
/*  URW Verlag, Hamburg, 1987. The IK format defined in Appendices G&I */
/* Possibly available from URW++ http://www.urwpp.de/english/home.html */

static int urwtable[2000];
static int urw_inited;

static void InitURWTable(void) {
    int i;

    if ( urw_inited )
return;
    memset(urwtable,-1,sizeof(urwtable));
    for ( i=0; i<26; ++i ) {
	urwtable[i+101] = 'A'+i;
	urwtable[i+301] = 'a'+i;
    }
    urwtable[514] = 0x192;
    urwtable[516] = 0xa5;
    urwtable[606] = 0x2026;
    urwtable[625] = '-';
    urwtable[626] = '(';
    urwtable[627] = ')';
    urwtable[628] = '[';
    urwtable[629] = ']';
    urwtable[631] = 0xa7;
    urwtable[632] = 0x2020;
    urwtable[633] = 0x2021;
    urwtable[635] = '\'';
    urwtable[636] = '"';
    urwtable[637] = '@';
    urwtable[638] = '#';
    urwtable[639] = 0xb0;
    urwtable[640] = '+';
    urwtable[641] = '-';
    urwtable[642] = 0xd7;
    urwtable[643] = 0xf7;
    urwtable[644] = '=';
    urwtable[647] = ':';
    urwtable[648] = 0xa9;
    urwtable[649] = 0xae;
    urwtable[650] = 0x2122;
    urwtable[651] = 0xb6;
    urwtable[652] = 0xa4;
    urwtable[655] = '{';
    urwtable[656] = '}';
    urwtable[657] = 0xaa;
    urwtable[658] = 0xba;
    urwtable[659] = 0xb1;
    urwtable[622] = '/';
    urwtable[677] = 0x2044;		/* Or should it be slash? */
    urwtable[700] = '\\';
	/* 795 is a big copyright */
	/* 796 is a big registered */
    urwtable[1016] = 0x2022;
    urwtable[1036] = 0xb7;
    urwtable[1049] = 0x25ca;
	/* 1059 is an apple character (which has no unicode equiv) */
    urwtable[1085] = 0x2206;
    urwtable[1101] = 0x2260;
    urwtable[1103] = 0x2261;
    urwtable[1104] = 0x221a;
    urwtable[1105] = 0x222b;
    urwtable[1108] = 0x223c;
    urwtable[1109] = 0x2248;
    urwtable[1111] = '<';
    urwtable[1112] = '>';
    urwtable[1113] = 0x2264;
    urwtable[1114] = 0x2265;
    urwtable[1117] = 0xac;
    urwtable[1124] = 0x221e;
    urwtable[1133] = 0x220f;
    urwtable[1134] = 0x2211;
    urwtable[1137] = 0x2202;
    urwtable[1151] = '^';
    urwtable[1152] = '_';
    urwtable[1199] = 0x3c0;
    urwtable[1337] = '~';
    urwtable[1342] = 0xb5;
    urwtable[1376] = '|';
	/* 1405-1408 is another set of +,-,=,divide */
    urwtable[1412] = 0x2126;

    urw_inited = true;
}

static void IkarusNameFromURWNumber(SplineChar *sc,int number) {
    int val;
    char buf[20];

    if ( !urw_inited )
	InitURWTable();

    free(sc->name); sc->name = NULL;
    if ( number<sizeof(urwtable)/sizeof(urwtable[0]) ) {
	sc->unicodeenc = val = urwtable[number];
	if ( sc->unicodeenc!=-1 ) {
	    if ( val<psunicodenames_cnt && psunicodenames[val]!=NULL )
		sc->name = copy(psunicodenames[val]);
	    else {
		sprintf(buf,"uni%04X",val);
		sc->name = copy(buf);
	    }
return;
	}
    }
    if ( number==1059 )
	sc->name = copy("apple");
    else if ( number==795 )
	sc->name = copy("copyright.big");
    else if ( number==796 )
	sc->name = copy("registered.big");
    else {
	sprintf(buf, "urw%d", number );
	sc->name = copy(buf);
    }
}

static void IkarusAddContour(SplineChar *sc,int npts,BasePoint *bps,
	uint8 *ptype, int nesting) {
    SplinePointList *spl;
    SplinePoint *last, *next;
    int i, cw;

    spl = chunkalloc(sizeof(SplinePointList));
    spl->next = sc->splines;
    sc->splines = spl;
    spl->first = spl->last = last = SplinePointCreate(bps[0].x,bps[0].y);
    last->pointtype = ptype[npts-1];
    last->nextcpdef = last->prevcpdef = true;
    for ( i=1; i<npts-1; ++i ) {
	next = SplinePointCreate(bps[i].x,bps[i].y);
	next->nextcpdef = next->prevcpdef = true;
	next->pointtype = ptype[i];
	SplineMake(last,next);
	last = next;
    }
    SplineMake(last,spl->last);
    last = spl->first;
    for ( i=1; i<npts; ++i ) {
	SplineCharDefaultPrevCP(last,last->prev->from);
	SplineCharDefaultNextCP(last,last->next->to);
	last=last->next->to;
    }

    cw = SplinePointListIsClockwise(spl);
    if ( ((nesting&1) && cw) || (!(nesting&1) && !cw) )
	SplineSetReverse(spl);
}

static void IkarusReadChar(SplineChar *sc,FILE *file) {
    int n, i, j, number, following, units, ncontours, ptmax;
    DBounds bb;
    int32 base;
    struct contour {
	int32 offset;
	int dir, nest, col, npts;
    } *contours;
    BasePoint *bps;
    uint8 *ptype;
    int x,y;

    /* record len of char = */ getushort(file);
    /* word len of char = */ getushort(file);

    n = getushort(file);
    number = getushort(file);
#if 0
    if ( enc!=sc->enc )
	fprintf( stderr, "The font header said this should be character %d, the character header says it is %d\n", sc->enc, enc );
#endif
    IkarusNameFromURWNumber(sc,number);
    following = getushort(file);
    if ( following!=0 )
	fprintf( stderr, "This character (%d) has a following part (%d). I'm not sure what that means, please send me (gww@silcom.com) a copy of this font so I can test with it.\n" );
    for ( i=3; i<n ; ++i )
	getushort(file);	/* Just in case the name section is bigger now */

    n = getushort(file);
    /* sort (leter,logo,line graphic,frame etc.) = */ getushort(file);
    /* num points = */ getushort(file);
    sc->width = getushort(file);
    /* lsb = */ getushort(file);
    /* xmax-xmin = */ getushort(file);
    /* rsb = */ getushort(file);
    bb.minx = (short) getushort(file);
    bb.maxx = (short) getushort(file);
    bb.miny = (short) getushort(file);
    bb.maxy = (short) getushort(file);
    for ( i=12 ; i<n; ++i )
	getushort(file);

    units = getushort(file);
    if ( units!=1 )
	sc->width *= units;

    n = getushort(file)*2048;
    n += getushort(file);		/* Length of contour section in words */
    ncontours = (n-2)/6;
    contours = galloc(ncontours*sizeof(struct contour));
    ptmax = 0;
    for ( i=0; i<ncontours; ++i ) {
	contours[i].offset = getushort(file)*4096;
	contours[i].offset += 2*getushort(file)-2;
	contours[i].dir = getushort(file);
	contours[i].nest = getushort(file);
	contours[i].col = getushort(file);
	contours[i].npts = getushort(file);
	if ( contours[i].npts > ptmax )
	    ptmax = contours[i].npts;
    }
    bps = galloc(ptmax*sizeof(BasePoint));
    ptype = galloc(ptmax*sizeof(uint8));

    base = ftell(file);
    /* 2 words here giving length (in records/words) of image data */

    for ( i=0; i<ncontours; ++i ) {
	fseek(file,base+contours[i].offset,SEEK_SET);
	for ( j=0; j<contours[i].npts; ++j ) {
	    x = (short) getushort(file);
	    y = (short) getushort(file);
	    if ( x<0 && y>0 )
		ptype[j] = -1;		/* Start point */
	    else if ( x<0 /* && y<0 */)
		ptype[j] = pt_corner;
	    else if ( /* x>0 && */ y>0 )
		ptype[j] = pt_curve;
	    else /* if ( x>0 && y<0 ) */
		ptype[j] = pt_tangent;
	    if ( x<0 ) x=-x;
	    if ( y<0 ) y=-y;
	    x += bb.minx-1;
	    y += bb.miny-1;
	    bps[j].x = units*x; bps[j].y = units*y;
	}
	IkarusAddContour(sc,contours[i].npts,bps,ptype,contours[i].nest);
    }

    free(contours);
    free(bps);
    free(ptype);
}

static void IkarusFontname(SplineFont *sf,char *fullname,char *fnam) {
    char *pt, *tpt;

    for ( pt=fullname; *pt==' '; ++pt );
    if ( *pt=='\0' ) {
	/* Often the fontname is blank */
	pt = strchr(fnam,'.');
	if ( pt!=NULL ) *pt='\0';
	for ( pt=fnam; *pt==' '; ++pt );
	if ( *pt!='\0' )
	    strcpy(fullname,fnam);
	else {
	    pt = strrchr(sf->origname,'/');
	    if ( pt==NULL ) pt = sf->origname-1;
	    strncpy(fullname,pt,80);
	    fullname[80] = '\0';
	    pt = strchr(fullname,'.');
	    if ( pt!=NULL ) *pt = '\0';
	}
    }

    free(sf->fullname);
    sf->fullname=copy(fullname);

    free(sf->fontname);
    sf->fontname=copy(fullname);
    for ( pt=tpt=sf->fontname; *pt; ++pt ) {
	if ( isalnum(*pt) || *pt=='-' || *pt=='_' || *pt=='$' )
	    *tpt++ = *pt;
    }
    *tpt = '\0';

    if ( (pt=strstr(fullname,"Regu"))!=NULL ||
	    (pt=strstr(fullname,"Medi"))!=NULL ||
	    (pt=strstr(fullname,"Bold"))!=NULL ||
	    (pt=strstr(fullname,"Demi"))!=NULL ||
	    (pt=strstr(fullname,"Gras"))!=NULL ||
	    (pt=strstr(fullname,"Fett"))!=NULL ||
	    (pt=strstr(fullname,"Norm"))!=NULL ||
	    (pt=strstr(fullname,"Nord"))!=NULL ||
	    (pt=strstr(fullname,"Heav"))!=NULL ||
	    (pt=strstr(fullname,"Blac"))!=NULL ||
	    (pt=strstr(fullname,"Ligh"))!=NULL ||
	    (pt=strstr(fullname,"Thin"))!=NULL ) {
	free(sf->weight);
	sf->weight = copyn(pt,4);
	*pt='\0';
    }
    while ( (pt=strstr(fullname,"Ital"))!=NULL ||
	    (pt=strstr(fullname,"Obli"))!=NULL ||
	    (pt=strstr(fullname,"Roma"))!=NULL ||
	    (pt=strstr(fullname,"Cond"))!=NULL ||
	    (pt=strstr(fullname,"Expa"))!=NULL ) {
	*pt='\0';
    }
    free(sf->familyname);
    sf->familyname = copy(fullname);
}

SplineFont *SFReadIkarus(char *fontname) {
    SplineFont *sf;
    FILE *file = fopen(fontname,"r");
    int ch1, ch2, rpos, wpos, i;
    int hlen, ilen, jlen, llen, mlen;
    int numchars, maxnum, opt_pt_size;
    double italic_angle;
    char fnam[13], fullname[81];
    int32 *offsets, *numbers;

    if ( file==NULL )
return( NULL );
    hlen = getushort(file);		/* Length of font header */
    ilen = getushort(file);		/* Length of name section */
    getushort(file);			/* Number on URW list */
    fread(fnam,1,12,file);		/* 6 words of filename */
    fread(fullname,1,80,file);		/* 40 words of fontname (human readable) */
    fnam[12] = fullname[80] = '\0';
    ch1 = getc(file);
    ch2 = getc(file);
    if ( ch1!='I' || ch2!='K' ) {
	if ( (ch1=='D' && ch2=='I') || (ch1=='V' && ch2=='C') ||
		(ch1=='V' && ch2=='S') || (ch1=='V' && ch2=='E') || 
		(ch1=='S' && ch2=='C') || (ch1=='S' && ch2=='N') || 
		(ch1=='B' && ch2=='I') || (ch1=='G' && isdigit(ch2)))
	    fprintf( stderr, "This is probably a valid URW font, but it is in a format (%c%c) which PfaEdit\ndoes not support. PfaEdit only supports 'IK' format fonts.\n" );
	fclose(file);
return( NULL );
    } else if ( ilen<55 || hlen<=ilen ) {
	fclose(file);
return( NULL );
    }
    if ( ilen!=55 )
	fprintf(stderr, "Unexpected size for name section of URW font (expected 55, got %d)\n", ilen );

    fseek(file,2*ilen+2,SEEK_SET);
    jlen = getushort(file);
    if ( jlen<12 || hlen<=jlen+ilen ) {
	fclose(file);
return( NULL );
    }
    if ( jlen!=12 )
	fprintf(stderr, "Unexpected size for font info section of URW font (expected 12, got %d)\n", ilen );
    if ( getushort(file)!=1 ) {		/* 1=> typeface */
	fclose(file);
return( NULL );
    }
    numchars = getushort(file);
    /* cap height = */ getushort(file);
    /* body size = */ getushort(file);
    /* x-height = */ getushort(file);
    /* descender? = */ getushort(file);
    /* line thickness = */ getushort(file);
    /* stroke thickness = */ getushort(file);
    italic_angle = getushort(file)/10.0 * 3.1415926535897932/180.0;
    opt_pt_size = getushort(file);
    /* average char width = */ getushort(file);

    fseek(file,2*ilen+2*jlen+2,SEEK_SET);
    llen = getushort(file);		/* the hierarchy section is unused in font files */
    if ( llen!=1 || hlen<=jlen+ilen+llen ) {
	fclose(file);
return( NULL );
    }

    mlen = getushort(file);
    /* Peter Karow's book documents that hlen==jlen+ilen+llen+mlen+1, but this*/
    /*  does not appear to be the case. */
    if ( hlen<jlen+ilen+llen+mlen+1 || mlen<3*numchars+3 ) {
	fclose(file);
return( NULL );
    }
    /* last record */ getushort(file);
    /* last word of last record */ getushort(file);

    offsets = galloc(numchars*sizeof(int32));
    numbers = galloc(numchars*sizeof(int32));
    maxnum = 0;
    for ( i=0; i<numchars; ++i ) {
	numbers[i] = getushort(file);
	if ( numbers[i]>maxnum ) maxnum = numbers[i];
	rpos = getushort(file);		/* record pos (1 record=2048 words) */
	wpos = getushort(file);		/* word pos in record */
	offsets[i] = (rpos-1)*4096 + 2*(wpos-1);
    }

    sf = SplineFontBlank(em_none,numchars/*maxnum+1*/);
    IkarusFontname(sf,fullname,fnam);
    sf->italicangle = italic_angle;
    sf->ascent = 12000; sf->descent = 3000;	/* Ikarus fonts live in a 15,000 em world */
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	sf->chars[i]->width = sf->chars[i]->vwidth = 15000;

    for ( i=0; i<numchars; ++i ) {
	fseek(file,offsets[i],SEEK_SET);
	IkarusReadChar(SFMakeChar(sf,i),file);
    }

    free(numbers); free(offsets);
    fclose(file);
return( sf );
}
